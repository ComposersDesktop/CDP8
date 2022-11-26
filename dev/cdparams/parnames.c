#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <logic.h>
#include <processno.h>
#include <modeno.h>
#include <pnames.h>
#include <cdparams.h>
#include <globcon.h>

/******************************* GET_PARAM_NAMES *******************************/

int get_param_names(int process,int mode,int total_params,aplptr ap)
{
	if((ap->param_name = (char **)malloc((total_params) * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: to get_param_names\n"); 
		return(MEMORY_ERROR);
	}

	switch(process) {

	case(ACCU):
		ap->param_name[ACCU_DINDEX] 		= "DECAY_RATE";
		ap->param_name[ACCU_GINDEX] 		= "GLISS_RATE";
		break;

	case(ALT):
		break;

	case(ARPE):
		ap->param_name[ARPE_WTYPE] 			= "WAVETYPE";
		ap->param_name[ARPE_ARPFRQ] 		= "RATE_OF_SWEEP";
		ap->param_name[ARPE_PHASE] 			= "START_PHASE";
		ap->param_name[ARPE_LOFRQ] 			= "LOFRQ_LIMIT";
		ap->param_name[ARPE_HIFRQ] 			= "HIFRQ_LIMIT";
		ap->param_name[ARPE_HBAND] 			= "BANDWIDTH";
		ap->param_name[ARPE_AMPL] 			= "AMPLIFICATION";
		if(mode==ON || mode==BOOST || mode==ABOVE_BOOST || mode==BELOW_BOOST) {
			ap->param_name[ARPE_NONLIN] 	= "NONLINEAR_DECAY";
			ap->param_name[ARPE_SUST] 		= "SUSTAIN_WINDOWS";
			break;
		}
		break;

	case(AVRG):
		ap->param_name[AVRG_AVRG] 			= "AVERAGING_SPAN";
		break;

	case(BARE):
		break;

	case(BLTR):
		ap->param_name[BLUR_BLURF] 			= "BLURRING_FACTOR";
		ap->param_name[BLTR_TRACE] 			= "TRACING_INDEX";
		break;

	case(BLUR):
		ap->param_name[BLUR_BLURF] 			= "BLURRING_FACTOR";
		break;

	case(BRIDGE):
		ap->param_name[BRG_STIME] 			= "STARTTIME_IN_FILE1";
		ap->param_name[BRG_ETIME] 			= "ENDTIME_IN_FILE1";
		ap->param_name[BRG_OFFSET] 			= "2ND_FILE_OFFSET";
		ap->param_name[BRG_SF2] 			= "FRQ_INTERP_AT_START";
		ap->param_name[BRG_SA2] 			= "AMP_INTERP_AT_START";
		ap->param_name[BRG_EF2] 			= "FRQ_INTERP_AT_END";
		ap->param_name[BRG_EA2] 			= "AMP_INTERP_AT_END";
		break;

	case(CHANNEL):
		ap->param_name[CHAN_FRQ] 			= "FREQUENCY";
		break;

	case(CHORD):
	case(MULTRANS):
		ap->param_name[CHORD_LOFRQ] 		= "BOTTOM_FRQ";
		ap->param_name[CHORD_HIFRQ] 		= "TOP_FRQ";
		break;

	case(CHORUS):
		ap->param_name[CHORU_AMPR] 			= "AMP_SPREAD";
		ap->param_name[CHORU_FRQR] 			= "FRQ_SPREAD";
		break;

	case(CLEAN):
		switch(mode) {
		case(FROMTIME):
		case(ANYWHERE):	ap->param_name[CL_SKIPT] = "SKIPTIME";		break;

		case(FILTERING):ap->param_name[CL_FRQ] 	 = "LOW_FRQ_LIMIT";	break;
		}
		ap->param_name[CL_GAIN] 			= "NOISE_PREGAIN";
		break;

	case(CROSS):
		ap->param_name[CROS_INTP] 			= "INTERPOLATION";
		break;

	case(CUT):
		ap->param_name[CUT_STIME] 			= "START_TIME";
		ap->param_name[CUT_ETIME] 			= "END_TIME";
		break;

	case(DIFF):
		ap->param_name[DIFF_CROSS] 			= "CROSSOVER";
		break;

	case(DRUNK):
		ap->param_name[DRNK_RANGE] 			= "RANGE_IN_WINDOWS";
		ap->param_name[DRNK_STIME] 			= "STARTTIME_IN_FILE";
		ap->param_name[DRNK_DUR] 			= "OUTFILE_DURATION";
		break;								   

	case(EXAG):
		ap->param_name[EXAG_EXAG] 			= "EXAGGERATION";
		break;

	case(FILT):
		switch(mode) {
		case(F_HI):		 case(F_HI_NORM): case(F_LO): case(F_LO_NORM):
		case(F_HI_GAIN): case(F_LO_GAIN):
			ap->param_name[FILT_FRQ1] 		= "FREQUENCY";
			break;
		default:
			ap->param_name[FILT_FRQ1] 		= "LOW_FRQ_LIMIT";
			break;
		}
		ap->param_name[FILT_FRQ2] 			= "HIGH_FRQ_LIMIT";
		ap->param_name[FILT_QQ] 			= "FILTER_EDGE(S)_WIDTH";
		ap->param_name[FILT_PG] 			= "GAIN";
		break;

	case(FMNTSEE):
		break;

	case(FOCUS):
		ap->param_name[FOCU_PKCNT] 			= "MAX_PEAKS_TO_FIND";
		ap->param_name[FOCU_BW] 			= "FILTER_BANDWIDTH";
		ap->param_name[FOCU_LOFRQ] 			= "BOTTOM_FRQ";
		ap->param_name[FOCU_HIFRQ] 			= "TOP_FRQ";
		ap->param_name[FOCU_STABL] 			= "WINDOWCNT_FOR_STABILITY";
		break;								   

	case(FOLD):
		ap->param_name[FOLD_LOFRQ] 			= "LOW_FRQ_LIMIT";
		ap->param_name[FOLD_HIFRQ] 			= "HIGH_FRQ_LIMIT";
		break;

	case(FORM):
		ap->param_name[FORM_FTOP] 			= "HIGH_FRQ_LIMIT";
		ap->param_name[FORM_FBOT] 			= "LOW_FRQ_LIMIT";
		ap->param_name[FORM_GAIN] 			= "GAIN";
		break;

	case(FORMANTS):
		break;

	case(FORMSEE):
		break;

	case(FREEZE):
	case(FREEZE2):
		break;

	case(FREQUENCY):
		ap->param_name[FRQ_CHAN] 			= "CHANNEL";
		break;

	case(GAIN):
		ap->param_name[GAIN_GAIN] 			= "GAIN";
		break;

	case(GLIDE):
		ap->param_name[GLIDE_DUR] 			= "DURATION";
		break;

	case(GLIS):
		ap->param_name[GLIS_RATE] 			= "GLISS_RATE";
		ap->param_name[GLIS_HIFRQ] 			= "HIGH_FRQ_LIMIT";
		if(mode==INHARMONIC) {
			ap->param_name[GLIS_SHIFT] = "PARTIAL_SPACING_(HZ)";	
		}
		break;

	case(GRAB):
		ap->param_name[GRAB_FRZTIME] 		= "FREEZE_MOMENT";
		break;

	case(GREQ):
		break;

	case(INVERT):
		break;

	case(LEAF):
		ap->param_name[LEAF_SIZE] 			= "LEAFSIZE_(WINDOWS)";
		break;

	case(LEVEL):
		break;

	case(MAGNIFY):
		ap->param_name[MAG_FRZTIME] 		= "FREEZE_MOMENT";
		ap->param_name[MAG_DUR] 			= "OUTFILE_DURATION";
		break;

	case(MAKE):
		break;

	case(MAX):
		break;

	case(MEAN):
		ap->param_name[MEAN_LOF] 			= "LOW_FRQ_LIMIT";
		ap->param_name[MEAN_HIF] 			= "HIGH_FRQ_LIMIT";
		ap->param_name[MEAN_CHAN] 			= "CHANNELS_TO_COMPARE";
		break;

	case(MORPH):
		ap->param_name[MPH_ASTT] 			= "AMP_INTERP_START";
		ap->param_name[MPH_AEND] 			= "AMP_INTERP_END";
		ap->param_name[MPH_FSTT] 			= "FRQ_INTERP_START";
		ap->param_name[MPH_FEND] 			= "FRQ_INTERP_END";
		ap->param_name[MPH_AEXP] 			= "AMP_INTERP_EXPONENT";
		ap->param_name[MPH_FEXP] 			= "FRQ_INTERP_EXPONENT";
		ap->param_name[MPH_STAG] 			= "2nd_FILE_ENTRY_TIME";
		break;

	case(NOISE):
		ap->param_name[NOISE_NOIS] 			= "NOISE_SATURATION";
		break;

	case(OCT):
		ap->param_name[OCT_HMOVE] 			= "TRANSPOSITION_RATIO";
		ap->param_name[OCT_BREI] 			= "BASS_BOOST";
		break;

	case(OCTVU):
		ap->param_name[OCTVU_TSTEP] 		= "TIMESTEP_(MS)";
		ap->param_name[OCTVU_FUND] 			= "FUNDAMENTAL_FRQ";
		break;

	case(P_APPROX):
		ap->param_name[PA_PRANG] 			= "PITCH_SCATTER_RANGE_(SEMIT)";
		ap->param_name[PA_TRANG] 			= "TIME_SCATTER_RANGE_(MS)";
		ap->param_name[PA_SRANG] 			= "CONTOUR_SCAN_RANGE_(MS)";
		break;

	case(P_CUT):
		ap->param_name[PC_STT] 				= "STARTTIME";
		ap->param_name[PC_END] 				= "ENDTIME";
		break;

	case(P_EXAG):
		ap->param_name[PEX_MEAN] 			= "MEANPITCH_(MIDI)";
		ap->param_name[PEX_RANG] 			= "INTERVAL_MULTIPLIER";
		ap->param_name[PEX_CNTR] 			= "CONTOUR_EXAGGERATION";
		break;

	case(P_FIX):
		ap->param_name[PF_SCUT] 			= "PITCH_REMOVAL_STARTTIME";
		ap->param_name[PF_ECUT] 			= "PITCH_REMOVAL_ENDTIME";
		ap->param_name[PF_LOF] 				= "FRQ_BELOW_WHICH_TO_REMOVE_PITCH";
		ap->param_name[PF_HIF] 				= "FRQ_ABOVE_WHICH_TO_REMOVE_PITCH";
		ap->param_name[PF_SMOOTH] 			= "NUMBER_OF_SMOOTHINGS";
		ap->param_name[PF_SMARK] 			= "FRQ_TO_FORCE_AT_START";
		ap->param_name[PF_EMARK] 			= "FRQ_TO_FORCE_AT_END";
		break;								   

	case(P_HEAR):
		ap->param_name[PH_GAIN] 			= "GAIN";
		break;

	case(P_INFO):
		break;

	case(P_INVERT):
		ap->param_name[PI_MEAN] 			= "INVERSION_CENTRE_(MIDI)";
		ap->param_name[PI_BOT] 				= "LOW_PITCH_LIMIT_(MIDI)";
		ap->param_name[PI_TOP] 				= "HIGH_PITCH_LIMIT_(MIDI)";
		break;

	case(P_QUANTISE):
		break;

	case(P_RANDOMISE):
		ap->param_name[PR_MXINT] 			= "MAX_INTERVAL_OF_STRAY";
		ap->param_name[PR_TSTEP] 			= "TIMESTEP_(MS)";
		ap->param_name[PR_SLEW]  			= "RANGE_SLEW";
		break;

	case(P_SEE):
		ap->param_name[PSEE_SCF] 			= "SCALE_FACTOR";
		break;

	case(P_SMOOTH):
		ap->param_name[PS_TFRAME] 			= "TIMEFRAME_(MS)";
		ap->param_name[PS_MEAN] 			= "MEANPITCH";
		break;
	case(P_SYNTH):	
	case(P_INSERT):	
	case(P_SINSERT):	
	case(P_PTOSIL):	
	case(P_NTOSIL):	
	case(ANALENV):	
	case(P_BINTOBRK):	
	case(MAKE2):
	case(P_INTERP):
		break;
	case(P_VOWELS):
		ap->param_name[PV_HWIDTH] 			= "FORMANT_HALF-WIDTH";
		ap->param_name[PV_CURVIT] 			= "FORMANT_PEAK_STEEPNESS";
		ap->param_name[PV_PKRANG] 			= "FORMANT_PEAK_RANGE";
		ap->param_name[PV_FUNBAS] 			= "EMPHASIS_ON_FUNDAMENTAL";
		ap->param_name[PV_OFFSET] 			= "HOARSENESS";
		break;
	case(VFILT):
		ap->param_name[PV_HWIDTH] 			= "FORMANT_HALF-WIDTH";
		ap->param_name[PV_CURVIT] 			= "FORMANT_PEAK_STEEPNESS";
		ap->param_name[PV_PKRANG] 			= "FORMANT_PEAK_RANGE";
		ap->param_name[VF_THRESH] 			= "THRESHOLD";
		break;
	case(P_GEN):
		ap->param_name[PGEN_SRATE] 			= "SAMPLE_RATE_OF_GOAL_SNDFILE";
		ap->param_name[PGEN_CHANS_INPUT]  	= "ANALYSIS_POINTS";
		ap->param_name[PGEN_WINOVLP_INPUT]  = "ANALWINDOW_OVERLAP";			
		break;
	case(P_TRANSPOSE):
		ap->param_name[PT_TVAL] 			= "TRANSPOSITION_(SEMIT)";
		break;

	case(P_VIBRATO):
		ap->param_name[PV_FRQ] 				= "VIBRATO_FRQ";
		ap->param_name[PV_RANG] 			= "VIBRATO_RANGE (SEMIT)";
		break;

	case(P_WRITE):
		ap->param_name[PW_DRED] 			= "DATA_REDUCTION_(SEMIT)";
		break;

	case(P_ZEROS):
		break;

	case(PEAK):
		ap->param_name[PEAK_CUTOFF]  		= "LOW_FRQ_LIMIT";
		ap->param_name[PEAK_TWINDOW] 		= "TIME_WINDOW_(SECS)";
		ap->param_name[PEAK_FWINDOW] 		= "FRQ_WINDOW_(SEMIT)";
		break;

	case(PICK):
		ap->param_name[PICK_FUND] 			= "FUNDAMENTAL_FRQ";
		ap->param_name[PICK_LIN]  			= "FRQ_STEP";
		ap->param_name[PICK_CLAR] 			= "CLARITY";
		break;

	case(PITCH):
		ap->param_name[PICH_RNGE] 			= "IN-TUNE_RANGE_(SEMIT)";
		ap->param_name[PICH_VALID] 			= "MIN_WINDOWS_TO_CONFIRM_PITCH";
		ap->param_name[PICH_SRATIO] 		= "SIGNAL_TO_NOISE_RATIO_(dB)";
		ap->param_name[PICH_MATCH] 			= "VALID_HARMONICS_COUNT";
		ap->param_name[PICH_LOLM] 			= "LOW_PITCH_LIMIT_(HZ)";
		ap->param_name[PICH_HILM] 			= "HIGH_PITCH_LIMIT_(HZ)";
		if(mode==PICH_TO_BRK) {			   
			ap->param_name[PICH_DATAREDUCE] = "DATAREDUCTION_(SEMIT)";
		}
		break;

	case(PLUCK):
		ap->param_name[PLUK_GAIN] 			= "PLUCK_GAIN";
		break;

	case(PRINT):
		ap->param_name[PRNT_STIME] 			= "START_TIME";
		ap->param_name[PRNT_WCNT] 			= "WINDOW_COUNT";
		break;

	case(REPITCH):
		break;

	case(REPITCHB):
		ap->param_name[RP_DRED] 			= "DATAREDUCTION(SEMITONES)";
		break;

	case(REPORT):
		ap->param_name[REPORT_PKCNT] 		= "PEAKS_TO_FIND";
		ap->param_name[REPORT_LOFRQ] 		= "LOW_FRQ_LIMIT";
		ap->param_name[REPORT_HIFRQ] 		= "HIGH_FRQ_LIMIT";
		ap->param_name[REPORT_STABL] 		= "WINDOWS_TO_AVERAGE";
		break;

	case(SCAT):
		ap->param_name[SCAT_CNT] 			= "FRQ-BLOCKS_TO_KEEP";
		ap->param_name[SCAT_BLOKSIZE] 		= "FRQ_RANGE_OF_BLOCK";
		break;

	case(SHIFT):
		ap->param_name[SHIFT_SHIF] 			= "LINEAR_FRQ_SHIFT";
		switch(mode) {
		case(SHIFT_ALL):
			break;
		case(SHIFT_ABOVE):
		case(SHIFT_BELOW):
			ap->param_name[SHIFT_FRQ1] 		= "FRQ_DIVIDE";
			break;

		case(SHIFT_BETWEEN):
		case(SHIFT_OUTSIDE):
			ap->param_name[SHIFT_FRQ1] 		= "LOW_FRQ";
			ap->param_name[SHIFT_FRQ2] 		= "HIGH_FRQ";
			break;
		}
		break;

	case(SHIFTP):
		ap->param_name[SHIFTP_FFRQ] 		= "FRQ_DIVIDE";
		ap->param_name[SHIFTP_DEPTH] 		= "DEPTH";
		switch(mode) {

		case(P_OCT_UP):
		case(P_OCT_DN):
		case(P_OCT_UP_AND_DN):
			break;							   
		case(P_SHFT_UP):
		case(P_SHFT_DN):
			ap->param_name[SHIFTP_SHF1] 	= "TRANSPOSITION_(SEMIT)";
			break;

		case(P_SHFT_UP_AND_DN):
			ap->param_name[SHIFTP_SHF1] 	= "TRANSPOSITION_1_(SEMIT)";
			ap->param_name[SHIFTP_SHF2] 	= "TRANSPOSITION_2_(SEMIT)";
			break;
		}
		break;

	case(SHUFFLE):
		ap->param_name[SHUF_GRPSIZE] 		= "GROUPSIZE_(WINDOWS)";
		break;

	case(SPLIT):
		break;

	case(SPREAD):
		ap->param_name[SPREAD_SPRD] 		= "DEGREE_OF_SPREADING";
		break;

	case(STEP):
		ap->param_name[STEP_STEP] 			= "TIMESTEP";
		break;

	case(STRETCH):
		ap->param_name[STR_FFRQ] 			= "FRQ_DIVIDE";
		ap->param_name[STR_SHIFT] 			= "TOP_OF_SPECTRUM_STRETCH";
		ap->param_name[STR_EXP] 			= "STRETCH_EXPONENT";
		ap->param_name[STR_DEPTH] 			= "PROCESS_DEPTH";
		break;								   

	case(SUM):
		ap->param_name[SUM_CROSS] 			= "CROSSOVER";
		break;

	case(SUPR):
		ap->param_name[SUPR_INDX] 			= "CHANNELS_TO_SUPRESS";
		break;

	case(S_TRACE):
		ap->param_name[TRAC_INDX] 			= "CHANNELS_TO_RETAIN";
		ap->param_name[TRAC_LOFRQ] 			= "LOW_FRQ_LIMIT";
		ap->param_name[TRAC_HIFRQ] 			= "HIGH_FRQ_LIMIT";
		break;

	case(TRACK):
		ap->param_name[TRAK_PICH] 			= "ESTIMATED_STARTPITCH_(HZ)";
		ap->param_name[TRAK_RNGE] 			= "IN-TUNE_RANGE_(SEMIT)";
		ap->param_name[TRAK_VALID] 			= "MIN_WINDOWS_TO_CONFIRM_PITCH";
		ap->param_name[TRAK_SRATIO] 		= "SIGNAL_TO_NOISE_RATIO_(dB)";
		ap->param_name[TRAK_HILM] 			= "HIGH_PITCH_LIMIT_(HZ)";
		if(mode==TRK_TO_BRK) {			   
			ap->param_name[TRAK_DATAREDUCE] = "DATAREDUCTION_(SEMITONES)";
		}
		break;

	case(TRNSF):
		ap->param_name[TRNSF_LOFRQ] 		= "MINFRQ";
		ap->param_name[TRNSF_HIFRQ] 		= "MAXFRQ";
		break;

	case(TRNSP):
		ap->param_name[TRNSP_LOFRQ] 		= "MINFRQ";
		ap->param_name[TRNSP_HIFRQ] 		= "MAXFRQ";
		break;

	case(TSTRETCH):
		ap->param_name[TSTR_STRETCH] 		= "TIMESTRETCH_MULTIPLIER";
		break;

	case(TUNE):
		ap->param_name[TUNE_FOC] 			= "FOCUS";
		ap->param_name[TUNE_CLAR] 			= "CLARITY";
		ap->param_name[TUNE_INDX] 			= "TRACE_INDEX";
		ap->param_name[TUNE_BFRQ] 			= "LOW_FRQ_LIMIT";
		break;

	case(VOCODE):
		ap->param_name[VOCO_LOF] 			= "LOW_FRQ_LIMIT";
		ap->param_name[VOCO_HIF] 			= "HIGH_FRQ_LIMIT";
		ap->param_name[VOCO_GAIN] 			= "GAIN";
		break;

	case(WARP):
		ap->param_name[WARP_PRNG] 			= "PITCHWARP_RANGE_(SEMIT)";
		ap->param_name[WARP_TRNG] 			= "TIMEWARP_RANGE_(MS)";
		ap->param_name[WARP_SRNG] 			= "CONTOUR_SCAN_RANGE_(MS)";
		break;

	case(WAVER):
		ap->param_name[WAVER_VIB] 			= "VIBRATO_FRQ";
		ap->param_name[WAVER_STR] 			= "MAX_SPECTRAL_STRETCH";
		ap->param_name[WAVER_LOFRQ] 		= "BASE_FRQ_OF_STRETCH";
		ap->param_name[WAVER_EXP] 			= "EXPONENT";
		break;

	case(LIMIT):
		ap->param_name[LIMIT_THRESH] 		= "THRESHOLD_AMPLITUDE";
		break;

 				/****************** GROUCHO *****************/

	case(DISTORT):
		ap->param_name[DISTORT_POWFAC] 		= "EXAGGERATION";
		break;

	case(DISTORT_ENV):
		ap->param_name[DISTORTE_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTORTE_TROF] 		= "TROUGHING";
		ap->param_name[DISTORTE_EXPON] 		= "RISE-DECAY_EXPONENT";
		break;

	case(DISTORT_AVG):
		ap->param_name[DISTORTA_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTORTA_MAXLEN] 	= "MAX_WAVELEN";
		ap->param_name[DISTORTA_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_OMT):
		ap->param_name[DISTORTO_OMIT] 		= "A";
		ap->param_name[DISTORTO_KEEP] 		= "B";
		break;

	case(DISTORT_MLT):
		ap->param_name[DISTORTM_FACTOR] 	= "MULTIPLIER";
		break;

	case(DISTORT_DIV):
		ap->param_name[DISTORTM_FACTOR] 	= "DIVISOR";
		break;

	case(DISTORT_HRM):
		ap->param_name[DISTORTH_PRESCALE] 	= "PRE-ATTENUATION";
		break;

	case(DISTORT_FRC):
		ap->param_name[DISTORTF_SCALE] 		= "SCALING_FACTOR";
		ap->param_name[DISTORTF_AMPFACT] 	= "RELATIVE_LOUDNESS";
		ap->param_name[DISTORTF_PRESCALE] 	= "PRE-ATTENUATION";
		break;

	case(DISTORT_REV):
		ap->param_name[DISTORTR_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		break;

	case(DISTORT_SHUF):
		ap->param_name[DISTORTS_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTORTS_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_RPTFL):
		ap->param_name[DISTRPT_CYCLIM] 		= "HIGH_FRQ_LIMIT";
		/* fall thro */
	case(DISTORT_RPT):
	case(DISTORT_RPT2):
		ap->param_name[DISTRPT_MULTIPLY] 	= "MULTIPLIER";
		ap->param_name[DISTRPT_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTRPT_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_INTP):
		ap->param_name[DISTINTP_MULTIPLY] 	= "MULTIPLIER";
		ap->param_name[DISTINTP_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_DEL):
		ap->param_name[DISTDEL_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTDEL_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_RPL):
		ap->param_name[DISTRPL_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTRPL_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_TEL):
		ap->param_name[DISTTEL_CYCLECNT] 	= "CYCLE-GROUP_COUNT";
		ap->param_name[DISTTEL_SKIPCNT] 	= "SKIP_CYCLES";
		break;

	case(DISTORT_FLT):
		ap->param_name[DISTFLT_LOFRQ_CYCLELEN] = "LOW_FRQ_LIMIT";
		ap->param_name[DISTFLT_HIFRQ_CYCLELEN] = "HIGH_FRQ_LIMIT";
		ap->param_name[DISTFLT_SKIPCNT] 	   = "SKIP_CYCLES";
		break;

	case(DISTORT_INT):
		break;

	case(DISTORT_CYCLECNT):
		break;

	case(DISTORT_PCH):
		ap->param_name[DISTPCH_OCTVAR] 		= "TRANSPOSITION_RANGE_IN_OCTAVES";
		ap->param_name[DISTPCH_CYCLECNT] 	= "MAX_CYCLECNT_BTWN_TRANSPOS_VALS";
		ap->param_name[DISTPCH_SKIPCNT] 	= "SKIPCYCLES";
		break;								   

	case(DISTORT_OVERLOAD):
		ap->param_name[DISTORTER_MULT] 		= "GATE_LEVEL_WHERE_SIGNAL_IS_CLIPPED";
		ap->param_name[DISTORTER_DEPTH] 	= "DEPTH_OF_DISTORTION_GLAZE";
		if(mode==OVER_SINE)
			ap->param_name[DISTORTER_FRQ] 	= "FREQUENCY_OF_DISTORT_SIGNAL";
		break;								   

	case(DISTORT_PULSED):
		if(mode==PULSE_SYNI)
			ap->param_name[PULSE_STARTTIME] = "APPROX_START-SAMPLE_OF_PULSE_STREAM_IN_SRC_SOUND";
		else
			ap->param_name[PULSE_STARTTIME] = "PULSE_STREAM_START-TIME_IN_SRC_SOUND";
		ap->param_name[PULSE_DUR] 			= "DURATION_OF_IMPULSE_STREAM";
		ap->param_name[PULSE_FRQ] 			= "FREQUENCY_OF_IMPULSES_(HZ)";
		ap->param_name[PULSE_FRQRAND]		= "RANDOMISATION_OF_FRQ_OF_IMPULSES_IN_SEMITONES";
		ap->param_name[PULSE_TIMERAND] 		= "RANDOMISATION_OF_TIME_CONTOUR_OF_AN_IMPULSE";
		ap->param_name[PULSE_SHAPERAND] 	= "RANDOMISATION_OF_AMPLITUDE_CONTOUR_OF_AN_IMPULSE";
		if(mode==PULSE_SYN)
			ap->param_name[PULSE_WAVETIME] 	= "DURATION_OF_WAVECYCLES_TO_GRAB_AS_SOUND_SRC_INSIDE_IMPULSES";
		else if(mode==PULSE_SYNI)
			ap->param_name[PULSE_WAVETIME] 	= "NUMBER_OF_WAVECYCLES_TO_GRAB_AS_SOUND_SRC_INSIDE_IMPULSES";
		ap->param_name[PULSE_TRANSPOS] 		= "SEMITONE_TRANSPOSITION_CONTOUR_OF_SOUND_INSIDE_EACH_IMPULSE";
		ap->param_name[PULSE_PITCHRAND] 	= "SEMITONE_RANGE_OF_RANDOMISATION_OF_TRANSPOSITION_CONTOUR";
		break;

	case(ZIGZAG):
		ap->param_name[ZIGZAG_START] 	= "ZIGZAGGING_START_TIME";
		ap->param_name[ZIGZAG_END] 		= "ZIGZAGGING_END_TIME";
		ap->param_name[ZIGZAG_DUR] 		= "MIN_DURATION_OUTFILE";
		ap->param_name[ZIGZAG_MIN] 		= "MIN_ZIG_LENGTH";
		ap->param_name[ZIGZAG_SPLEN] 	= "SPLICE_LENGTH_(MS)";
		if(mode==ZIGZAG_SELF) {
			ap->param_name[ZIGZAG_MAX] 	= "MAX_ZIG_LENGTH";
			ap->param_name[ZIGZAG_RSEED]= "RANDOM_SEED";
		}
		break;

	case(LOOP):
		ap->param_name[LOOP_OUTDUR] 	= "OUTFILE_DURATION";
		ap->param_name[LOOP_REPETS] 	= "LOOP_REPEATS";
		ap->param_name[LOOP_START] 		= "STARTTIME_OF_LOOPING";
		ap->param_name[LOOP_LEN] 		= "LOOP_LENGTH_(MS)";
		ap->param_name[LOOP_STEP] 		= "ADVANCE_BETWEEN_LOOPS_(MS)";
		ap->param_name[LOOP_SPLEN] 		= "SPLICE_LENGTH_(MS)";
		ap->param_name[LOOP_SRCHF] 		= "SEARCHFIELD_(MS)";
		break;

	case(SCRAMBLE):
		switch(mode) {
		case(SCRAMBLE_RAND):
			ap->param_name[SCRAMBLE_MIN] = "MIN_CHUNKLEN";
			ap->param_name[SCRAMBLE_MAX] = "MAX_CHUNKLEN";
			break;
		case(SCRAMBLE_SHRED):
			ap->param_name[SCRAMBLE_LEN] = "CHUNK_LENGTH";
			ap->param_name[SCRAMBLE_SCAT]= "SCATTER_COEFFICIENT";
			break;
		default:
			sprintf(errstr,"Unknown case in SCRAMBLE: get_param_names()\n");
			return(PROGRAM_ERROR);
		}
		ap->param_name[SCRAMBLE_DUR] 		= "OUTPUT_DURATION";
		ap->param_name[SCRAMBLE_SPLEN] 		= "SPLICE_LENGTH_(MS)";
		ap->param_name[SCRAMBLE_SEED] 		= "SEED_VALUE";
		break;

	case(ITERATE):
		switch(mode) {
		case(ITERATE_DUR):		ap->param_name[ITER_DUR] 	 = "OUTPUT_DURATION";	break;
		case(ITERATE_REPEATS):	ap->param_name[ITER_REPEATS] = "NUMBER_OF_REPEATS";	break;
		default:
			sprintf(errstr,"Unknown case in ITERATE: get_param_names()\n");
			return(PROGRAM_ERROR);
		}
		ap->param_name[ITER_DELAY] 	= "DELAY";
		ap->param_name[ITER_RANDOM] = "RANDOMISATION_OF_DELAY";
		ap->param_name[ITER_PSCAT] 	= "PITCH_SCATTER";
		ap->param_name[ITER_ASCAT] 	= "AMPLITUDE_SCATTER";
		ap->param_name[ITER_FADE] 	= "PROGRESSIVE_FADE";
		ap->param_name[ITER_GAIN] 	= "OVERALL_GAIN";
		ap->param_name[ITER_RSEED] 	= "SEED_RANDOM_GENERATOR";
		break;

	case(ITERATE_EXTEND):
		switch(mode) {
		case(ITERATE_DUR):		ap->param_name[ITER_DUR] 	 = "DURATION_OF_OUTPUT_FILE";	break;
		case(ITERATE_REPEATS):	ap->param_name[ITER_REPEATS] = "NUMBER_OF_REPEATS";	break;
		default:
			sprintf(errstr,"Unknown case in ITERATE_EXTEND: get_param_names()\n");
			return(PROGRAM_ERROR);
		}
		ap->param_name[ITER_DELAY] 	= "DELAY";
		ap->param_name[ITER_RANDOM] = "RANDOMISATION_OF_DELAY";
		ap->param_name[ITER_PSCAT] 	= "PITCH_SCATTER";
		ap->param_name[ITER_ASCAT] 	= "AMPLITUDE_SCATTER";
		ap->param_name[CHUNKSTART] 	= "START_OF_TIME_FREEZE";
		ap->param_name[CHUNKEND] 	= "END_OF_TIME_FREEZE";
		ap->param_name[ITER_LGAIN]	= "FROZEN_SEGMENT_GAIN";
		ap->param_name[ITER_RRSEED] = "SEED_RANDOM_GENERATOR";
		break;

	case(DRUNKWALK):
		ap->param_name[DRNK_TOTALDUR] 	 = "OUTFILE_DURATION";
		ap->param_name[DRNK_LOCUS] 	 	 = "LOCUS";
		ap->param_name[DRNK_AMBITUS] 	 = "AMBITUS";
		ap->param_name[DRNK_GSTEP] 		 = "STEP";
		ap->param_name[DRNK_CLOKTIK] 	 = "CLOCKTICK";
		ap->param_name[DRNK_MIN_DRNKTIK] = "MIN_CLOKTIKS_BTWN_SOBER_MOMENTS";
		ap->param_name[DRNK_MAX_DRNKTIK] = "MAX_CLOKTIKS_BTWN_SOBER_MOMENTS";
		ap->param_name[DRNK_SPLICELEN]   = "SPLICELENGTH_(MS)";
		ap->param_name[DRNK_CLOKRND]   	 = "CLOCK_RANDOMISATION";
		ap->param_name[DRNK_OVERLAP]   	 = "SEGMENT_OVERLAY";	
		ap->param_name[DRNK_RSEED]   	 = "SEED_VALUE";
		if(mode==HAS_SOBER_MOMENTS) {	
			ap->param_name[DRNK_MIN_PAUS]= "MIN_LENGTH_SOBER_MOMENTS";
			ap->param_name[DRNK_MAX_PAUS]= "MAX_LENGTH_SOBER_MOMENTS";
		}									
		break;

	case(SIMPLE_TEX): 	
	case(TIMED):	  
	case(GROUPS):	  
	case(TGROUPS):	  
	case(DECORATED):	
	case(PREDECOR):	
	case(POSTDECOR):
	case(ORNATE):	  		
	case(PREORNATE):  
	case(POSTORNATE): 
	case(MOTIFS):	  	
	case(TMOTIFS):	  
	case(MOTIFSIN):   
	case(TMOTIFSIN):  
		ap->param_name[TEXTURE_DUR] 	= "OUTPUT_DURATION";
		switch(process) {
		case(SIMPLE_TEX):	
			ap->param_name[TEXTURE_PACK]= "EVENT_PACKING";					
			break;
		case(GROUPS):		
			ap->param_name[TEXTURE_PACK]= "SKIPTIME_BETWEEN_GROUP_ONSETS";		
			break;
		case(MOTIFS):	
		case(MOTIFSIN):		
			ap->param_name[TEXTURE_PACK]= "SKIPTIME_BETWEEN_MOTIF_ONSETS";		
			break;
		default:
			ap->param_name[TEXTURE_PACK]= "PAUSE_BEFORE_LINE_REPEATS";	
			break;
		}
		ap->param_name[TEXTURE_SCAT] 	= "EVENT_SCATTER";
		ap->param_name[TEXTURE_TGRID] 	= "TIME_GRID_UNIT_(MS)";
		ap->param_name[TEXTURE_INSLO] 	= "FIRST_SND-IN-LIST_TO_USE";
		ap->param_name[TEXTURE_INSHI] 	= "LAST_SND-IN-LIST_TO_USE";
		ap->param_name[TEXTURE_MINAMP] 	= "MIN_EVENT_GAIN_(MIDI)";
		ap->param_name[TEXTURE_MAXAMP] 	= "MAX_EVENT_GAIN_(MIDI)";
		ap->param_name[TEXTURE_MINDUR] 	= "MIN_EVENT_SUSTAIN";
		ap->param_name[TEXTURE_MAXDUR] 	= "MAX_EVENT_SUSTAIN";
		ap->param_name[TEXTURE_MINPICH] = "MIN_PITCH_(MIDI)";
		ap->param_name[TEXTURE_MAXPICH] = "MAX_PITCH_(MIDI)";
		if(process == SIMPLE_TEX)
			ap->param_name[TEX_PHGRID] 		= "PROPORTION_OF_SILENT_EVENTS";
		else
			ap->param_name[TEX_PHGRID] 		= "TIME_GRID_FOR_NOTEGROUPS_(MS)";
 		ap->param_name[TEX_GPSPACE] 	= "GROUP_SPATIALISATION_TYPE";
 		ap->param_name[TEX_GRPSPRANGE] 	= "GROUP_SPATIALISATION_RANGE";
 		ap->param_name[TEX_AMPRISE] 	= "GROUP_AMPLITUDE_CHANGE";
 		ap->param_name[TEX_AMPCONT] 	= "GROUP_AMPCONTOUR_TYPE";
		ap->param_name[TEX_GPSIZELO] 	= "MIN_GROUPSIZE";
		ap->param_name[TEX_GPSIZEHI] 	= "MAX_GROUPSIZE";
		ap->param_name[TEX_GPPACKLO] 	= "MIN_GROUP-PACKING-TIME_(MS)";
		ap->param_name[TEX_GPPACKHI] 	= "MAX_GROUP-PACKING-TIME_(MS)";
		switch(mode) {				   
		case(TEX_NEUTRAL):
			ap->param_name[TEX_GPRANGLO]= "MIN_GROUP_PITCHRANGE";
			ap->param_name[TEX_GPRANGHI]= "MAX_GROUP_PITCHRANGE";
			break;
		default:
			ap->param_name[TEX_GPRANGLO]= "MIN_HFIELD_NOTES_RANGE";
			ap->param_name[TEX_GPRANGHI]= "MAX_HFIELD_NOTES_RANGE";
			break;
		} 
		ap->param_name[TEX_MULTLO] 			= "MIN_MOTIF-DUR-MULTIPLIER";
		ap->param_name[TEX_MULTHI] 			= "MAX_MOTIF-DUR-MULTIPLIER";
		ap->param_name[TEX_DECPCENTRE] 		= "PITCH_CENTRING";
		ap->param_name[TEXTURE_ATTEN] 		= "OVERALL_ATTENUATION";
		ap->param_name[TEXTURE_POS] 		= "SPATIAL_POSITION";
		ap->param_name[TEXTURE_SPRD] 		= "SPATIAL_SPREAD";
		ap->param_name[TEXTURE_SEED] 		= "SEED";
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
		ap->param_name[GR_BLEN]			     = "MAX_TIME_BETWEEN_GRAINS";
		ap->param_name[GR_GATE]			     = "GATE_LEVEL";
		ap->param_name[GR_MINTIME]		     = "MIN_GRAIN-OFF_TIME";
		ap->param_name[GR_WINSIZE]		     = "ENVELOPE_WINDOW-SIZE_(MS)";
		switch(process) {
		case(GRAIN_OMIT):	    
			ap->param_name[GR_KEEP]	    = "GRAINS_TO_KEEP";
			ap->param_name[GR_OUT_OF]	= "OUT_OF_THIS_MANY";
			break;

		case(GRAIN_DUPLICATE):  
			ap->param_name[GR_DUPLS]	=  "REPETITION_COUNT";
			break;

		case(GRAIN_TIMEWARP):
			ap->param_name[GR_TSTRETCH]	=  "TIME-STRETCH_RATIO";
			break;

		case(GRAIN_POSITION):
			ap->param_name[GR_OFFSET]	=  "GRAINTIME_OFFSET";
			break;

		case(GRAIN_ALIGN):
			ap->param_name[GR_OFFSET]	=  "GRAINTIME_OFFSET";
			ap->param_name[GR_GATE2]	=  "GATE_LEVEL_FOR_2nd_FILE";
			break;
		default:
			break;
		}
		break;

	case(ENV_CREATE):
	case(ENV_BRKTOENV):
	case(ENV_DBBRKTOENV):
	case(ENV_IMPOSE):
	case(ENV_PROPOR):
	case(ENV_REPLACE):
	case(ENV_EXTRACT):
		ap->param_name[ENV_WSIZE] 			= "WINDOW_SIZE_(MS)";
		if(process==ENV_EXTRACT && mode==ENV_BRKFILE_OUT) {
			ap->param_name[ENV_DATAREDUCE]	= "DATA_REDUCTION";
		}
		break;

	case(ENV_ENVTOBRK):
	case(ENV_ENVTODBBRK):
		ap->param_name[ENV_DATAREDUCE] 		= "DATA_REDUCTION";
		break;

	case(ENV_WARPING):		
		ap->param_name[ENV_WSIZE] 			= "WINDOW_SIZE_(MS)";
		switch(mode) {
		case(ENV_NORMALISE):
		case(ENV_REVERSE):	
		case(ENV_CEILING):	
			break;

		case(ENV_DUCKED):
			ap->param_name[ENV_GATE]    	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]   = "THRESHOLD_LEVEL";
			break;

		case(ENV_EXAGGERATING):
			ap->param_name[ENV_EXAG]    	= "EXAGGERATION";
			break;

		case(ENV_ATTENUATING):
			ap->param_name[ENV_ATTEN]    	= "ATTENUATION";
			break;

		case(ENV_LIFTING):				
			ap->param_name[ENV_LIFT]     	= "LIFT";
			break;

		case(ENV_FLATTENING):
			ap->param_name[ENV_FLATN]    	= "AVERAGING_FACTOR";
			break;

		case(ENV_TSTRETCHING):
			ap->param_name[ENV_TSTRETCH] 	= "TIMESTRETCH_FACTOR";
			break;

		case(ENV_GATING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_INVERTING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_MIRROR]		= "MIRROR_LEVEL";
			break;

		case(ENV_LIMITING):
			ap->param_name[ENV_LIMIT]    	= "LIMIT_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			break;

		case(ENV_CORRUGATING):
			ap->param_name[ENV_TROFDEL]		= "WINDOW-DELETES_PER_TROUGH";
			/* fall thro */
		case(ENV_PEAKCNT):
			ap->param_name[ENV_PKSRCHWIDTH] = "MIN_WINDOWS_BETWEEN_PEAKS";
			break;							   

		case(ENV_EXPANDING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_TRIGGERING):
			ap->param_name[ENV_GATE]		= "GATE_LEVEL";
			ap->param_name[ENV_TRIGDUR]  	= "MAX_TRIGGER_DURATION_(MS)";
			ap->param_name[ENV_TRIGRISE] 	= "MIN_TRIGGER_LEVEL-RISE";
			break;
		default:
			sprintf(errstr,"Unknown case for ENVWARP,RESHAPING or REPLOTTING: get_param_names()\n");
			return(PROGRAM_ERROR);
		}				  
		break;		


	case(ENV_RESHAPING):
		switch(mode) {
		case(ENV_NORMALISE):
		case(ENV_REVERSE):	
		case(ENV_CEILING):	
			break;

		case(ENV_DUCKED):
			ap->param_name[ENV_GATE]    	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]   = "THRESHOLD_LEVEL";
			break;

		case(ENV_EXAGGERATING):
			ap->param_name[ENV_EXAG]    	= "EXAGGERATION";
			break;

		case(ENV_ATTENUATING):
			ap->param_name[ENV_ATTEN]    	= "ATTENUATION";
			break;

		case(ENV_LIFTING):				
			ap->param_name[ENV_LIFT]     	= "LIFT";
			break;

		case(ENV_FLATTENING):
			ap->param_name[ENV_FLATN]    	= "AVERAGING_FACTOR";
			break;

		case(ENV_TSTRETCHING):
			ap->param_name[ENV_TSTRETCH] 	= "TIMESTRETCH_FACTOR";
			break;

		case(ENV_GATING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_INVERTING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_MIRROR]		= "MIRROR_LEVEL";
			break;

		case(ENV_LIMITING):
			ap->param_name[ENV_LIMIT]    	= "LIMIT_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			break;

		case(ENV_CORRUGATING):
			ap->param_name[ENV_TROFDEL]		= "WINDOW-DELETES_PER_TROUGH";
			/* fall thro */
		case(ENV_PEAKCNT):
			ap->param_name[ENV_PKSRCHWIDTH] = "MIN_WINDOWS_BETWEEN_PEAKS";
			break;							   

		case(ENV_EXPANDING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_TRIGGERING):
			ap->param_name[ENV_GATE]		= "GATE_LEVEL";
			ap->param_name[ENV_TRIGDUR]  	= "MAX_TRIGGER_DURATION_(MS)";
			ap->param_name[ENV_TRIGRISE] 	= "MIN_TRIGGER_LEVEL-RISE";
			break;

		default:
			sprintf(errstr,"Unknown case for ENVWARP,RESHAPING or REPLOTTING: get_param_names()\n");
			return(PROGRAM_ERROR);
		}				  
		break;

	case(ENV_REPLOTTING):	
		ap->param_name[ENV_WSIZE] 			= "WINDOW_SIZE_(MS)";
		ap->param_name[ENV_DATAREDUCE] 	    = "DATA_REDUCTION";
		switch(mode) {
		case(ENV_NORMALISE):
		case(ENV_REVERSE):	
		case(ENV_CEILING):	
			break;

		case(ENV_DUCKED):
			ap->param_name[ENV_GATE]    	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]   = "THRESHOLD_LEVEL";
			break;

		case(ENV_EXAGGERATING):
			ap->param_name[ENV_EXAG]    	= "EXAGGERATION";
			break;

		case(ENV_ATTENUATING):
			ap->param_name[ENV_ATTEN]    	= "ATTENUATION";
			break;

		case(ENV_LIFTING):				
			ap->param_name[ENV_LIFT]     	= "LIFT";
			break;

		case(ENV_FLATTENING):
			ap->param_name[ENV_FLATN]    	= "AVERAGING_FACTOR";
			break;

		case(ENV_TSTRETCHING):
			ap->param_name[ENV_TSTRETCH] 	= "TIMESTRETCH_FACTOR";
			break;

		case(ENV_GATING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_INVERTING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_MIRROR]		= "MIRROR_LEVEL";
			break;

		case(ENV_LIMITING):
			ap->param_name[ENV_LIMIT]    	= "LIMIT_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			break;

		case(ENV_CORRUGATING):
			ap->param_name[ENV_TROFDEL]		= "WINDOW-DELETES_PER_TROUGH";
			/* fall thro */
		case(ENV_PEAKCNT):
			ap->param_name[ENV_PKSRCHWIDTH] = "MIN_WINDOWS_BETWEEN_PEAKS";
			break;							   

		case(ENV_EXPANDING):
			ap->param_name[ENV_GATE]     	= "GATE_LEVEL";
			ap->param_name[ENV_THRESHOLD]	= "THRESHOLD_LEVEL";
			ap->param_name[ENV_SMOOTH]   	= "SMOOTHING_FACTOR";
			break;

		case(ENV_TRIGGERING):
			ap->param_name[ENV_GATE]		= "GATE_LEVEL";
			ap->param_name[ENV_TRIGDUR]  	= "MAX_TRIGGER_DURATION_(MS)";
			ap->param_name[ENV_TRIGRISE] 	= "MIN_TRIGGER_LEVEL-RISE";
			break;
		default:
			sprintf(errstr,"Unknown case for ENVWARP,RESHAPING or REPLOTTING: get_param_names()\n");
			return(PROGRAM_ERROR);
		}				  
		break;

	case(ENV_DOVETAILING):
		ap->param_name[ENV_STARTTRIM]		= "INFADE_DURATION";		
		ap->param_name[ENV_ENDTRIM]   		= "OUTFADE_DURATION";
		ap->param_name[ENV_STARTTYPE] 		= "SLOPE_EXPONENTIAL:_START";
		ap->param_name[ENV_ENDTYPE]   		= "SLOPE_EXPONENTIAL:_END";
		ap->param_name[ENV_TIMETYPE]  		= "TIME_TYPE";
		break;

	case(ENV_CURTAILING):
		ap->param_name[ENV_STARTTIME] 		= "START_OF_FADE";	 
		if(mode==ENV_START_AND_DUR || mode==ENV_START_AND_DUR_D)
			ap->param_name[ENV_ENDTIME]   	= "DURATION_OF_FADE";
		else
			ap->param_name[ENV_ENDTIME]   	= "END_OF_FADE";
		ap->param_name[ENV_ENVTYPE]   		= "SLOPE_EXPONENTIAL";
		ap->param_name[ENV_TIMETYPE]  		= "TIME_TYPE";
		break;

	case(ENV_PLUCK):
		ap->param_name[ENV_PLK_ENDSAMP]	    = "PLUCK_ENDSAMPLE_POSITION";
		ap->param_name[ENV_PLK_WAVELEN]	    = "SAMPLES_IN_PLUCK_WAVECCYLE";
		ap->param_name[ENV_PLK_CYCLEN]		= "NO_OF_CYCLES_IN_PLUCK";
		ap->param_name[ENV_PLK_DECAY]		= "PLUCK_DECAY_RATE";
		break;								   

	case(ENV_TREMOL):
		ap->param_name[ENV_TREM_FRQ]		= "TREMOLO_FREQUENCY";
		ap->param_name[ENV_TREM_DEPTH]		= "TREMOLO_DEPTH";
		ap->param_name[ENV_TREM_AMP]		= "OVERALL_GAIN";
		break;

	case(ENV_SWELL):
		ap->param_name[ENV_PEAKTIME]        = "PEAKING_TIME";
		ap->param_name[ENV_ENVTYPE]   		= "SLOPE_EXPONENTIAL";
		break;

	case(ENV_ATTACK):
		ap->param_name[ENV_ATK_GAIN]  		= "ATTACK_GAIN";
		ap->param_name[ENV_ATK_ONSET] 		= "ATTACK_ONSET_DUR_(MS)";
		ap->param_name[ENV_ATK_TAIL]  		= "ATTACK_DECAY_DUR_(MS)";
		ap->param_name[ENV_ATK_ENVTYPE] 	= "SLOPE_EXPONENTIAL";
		switch(mode) {
		case(ENV_ATK_GATED):		
			ap->param_name[ENV_ATK_GATE]  	= "GATE_LEVEL";
			break;
		case(ENV_ATK_TIMED):		
			ap->param_name[ENV_ATK_ATTIME]  = "APPROX_ATTACK_TIME";
			break;
		case(ENV_ATK_XTIME):		
			ap->param_name[ENV_ATK_ATTIME]  = "EXACT_ATTACK_TIME";
			break;
		case(ENV_ATK_ATMAX):		
			break;
		}				  
		break;

	case(ENV_DBBRKTOBRK):	
	case(ENV_BRKTODBBRK):
		break;

	case(MIX):
		ap->param_name[MIX_ATTEN]			= "ATTENUATION";
		/* fall thro */
	case(MIXMAX):
		ap->param_name[MIX_START]			= "MIXING_STARTTIME";
		ap->param_name[MIX_END]				= "MIXING_ENDTIME";
		break;

	case(MIXTWO):
		ap->param_name[MIX_STAGGER] = "STAGGER_FILE2_ENTRY";
		ap->param_name[MIX_SKIP]    = "SKIP_INTO_2ND_FILE";
		ap->param_name[MIX_SKEW]    = "AMPLITUDE_SKEW";
		ap->param_name[MIX_STTA]    = "START_MIX_AT";
		ap->param_name[MIX_DURA]    = "CUT_OFF_MIX_AT";
		break;

	case(MIXMANY):
		break;

	case(MIXBALANCE):
		ap->param_name[MIX_STAGGER] = "BALANCE_FUNCTION";
		ap->param_name[MIX_SKIP]    = "START_TIME_OF_MIXING";
		ap->param_name[MIX_SKEW]    = "END_TIME_OF_MIXING";
		break;

	case(MIXCROSS):
		ap->param_name[MCR_STAGGER]	= "STAGGER_FILE2_ENTRY";
		ap->param_name[MCR_BEGIN]	= "CROSSFADE_START";
		ap->param_name[MCR_END]		= "CROSSFADE_END";
		if(mode==MCCOS) {
			ap->param_name[MCR_POWFAC]	= "CROSSFADE_SKEW";
		}
		break;

	case(MIXINBETWEEN):
		switch(mode) {
		case(INBI_COUNT):
			ap->param_name[INBETW]       = "FILES_BETWEEN";
			break;
		case(INBI_RATIO):
			break;
		default:
			sprintf(errstr,"Unknown case for MIXINBETWEEN: get_param_names()\n");
			return(PROGRAM_ERROR);
		}				  
		break;

	case(MIXTEST):
	case(MIXFORMAT):
	case(MIXDUMMY):
	case(MIXINTERL):
	case(MIXSYNC):
		break;
	case(MIX_ON_GRID):
	case(ADDTOMIX):
		break;
	case(AUTOMIX):
		ap->param_name[0] 			= "MIX_ENVELOPE_FUNCTION";
		break;
	case(MIX_PAN):
		ap->param_name[0] 			= "PAN_POSITION";
		break;
	case(MIX_AT_STEP):
		ap->param_name[0] 			= "TIME_STEP";
		break;
	case(SHUDDER):
		ap->param_name[SHUD_STARTTIME] = "START_TIME";
		ap->param_name[SHUD_FRQ]       = "FREQUENCY";
		ap->param_name[SHUD_SCAT]	   = "RANDOMISATION";
		ap->param_name[SHUD_SPREAD]	   = "SPATIAL_SPREAD";
		ap->param_name[SHUD_MINDEPTH]  = "LOUDNESS_DEPTH_MINIMUM";
		ap->param_name[SHUD_MAXDEPTH]  = "LOUDNESS_DEPTH_MAXIMUM";
		ap->param_name[SHUD_MINWIDTH]  = "EVENT_WIDTH_MINIMUM";
		ap->param_name[SHUD_MAXWIDTH]  = "EVENT_WIDTH_MAXIMUM";
		break;

	case(MIXSYNCATT):
		ap->param_name[MSY_WFAC] 			= "WINDOW_DIVIDER";
		break;

	case(MIXGAIN):	   	   
		ap->param_name[MIX_GAIN]  			= "GAIN";
		ap->param_name[MSH_STARTLINE]		= "START_LINE";
		ap->param_name[MSH_ENDLINE]			= "END_LINE";
		break;

	case(MIXTWARP):
		if(mode==MTW_TIMESORT)
			break;
		ap->param_name[MSH_STARTLINE]	= "START_LINE";
		ap->param_name[MSH_ENDLINE]		= "END_LINE";

		switch(mode) {
		case(MTW_REVERSE_T):  case(MTW_REVERSE_NT):
		case(MTW_FREEZE_T):	  case(MTW_FREEZE_NT):
		case(MTW_TIMESORT):
			break;

		case(MTW_SCATTER):  
			ap->param_name[MTW_PARAM]  		= "SCATTER";
			break;

		case(MTW_DOMINO):  
			ap->param_name[MTW_PARAM]  		= "DISPLACEMENT_(SECS)";
			break;

		case(MTW_ADD_TO_TG):  
		case(MTW_CREATE_TG_1): 	case(MTW_CREATE_TG_2):  case(MTW_CREATE_TG_3): 	case(MTW_CREATE_TG_4): 
		case(MTW_ENLARGE_TG_1): case(MTW_ENLARGE_TG_2): case(MTW_ENLARGE_TG_3): case(MTW_ENLARGE_TG_4): 
			ap->param_name[MTW_PARAM]  		= "PARAMETER";
			break;
		}
		break;

	case(MIXSWARP):	   	   
		if(mode==MSW_TWISTALL)	{
			break;
		}
		else if(mode==MSW_TWISTONE) {
			ap->param_name[MSW_TWLINE]  	= "TWISTED_LINE";
			break;
		}
		ap->param_name[MSH_STARTLINE]	= "START_LINE";
		ap->param_name[MSH_ENDLINE]		= "END_LINE";

		switch(mode) {
		case(MSW_NARROWED):					
			ap->param_name[MSW_NARROWING] 	= "NARROWING";
			break;

		case(MSW_LEFTWARDS): 
		case(MSW_RIGHTWARDS): 
		case(MSW_RANDOM): 
		case(MSW_RANDOM_ALT):
			ap->param_name[MSW_POS1] 		= "EDGE_OF_SPACE_RANGE";
 			ap->param_name[MSW_POS2] 		= "OTHER_EDGE_OF_SPACE_RANGE";
			break;							   

		case(MSW_FIXED):
			ap->param_name[MSW_POS1] 		= "POSITION";
			break;
		}
		break;

	case(MIXSHUFL):	   	   
		ap->param_name[MSH_STARTLINE]		= "START_LINE";
		ap->param_name[MSH_ENDLINE]			= "END_LINE";
		break;

	case(EQ):
		switch(mode) {
		case(FLT_PEAKING):
			ap->param_name[FLT_BW]			= "BANDWIDTH";
			/* fall thro */
		default:
			ap->param_name[FLT_BOOST]		= "BOOST_OR_ATTEN";
			ap->param_name[FLT_ONEFRQ]		= "FREQUENCY";
			ap->param_name[FLT_PRESCALE]	= "PRESCALE";
			ap->param_name[FILT_TAIL]		= "DECAY_TAIL_DURATION";
			break;
		}
		break;

	case(LPHP):
		ap->param_name[FLT_GAIN]			= "ATTENUATION(dB)";
		ap->param_name[FLT_PASSFRQ] 		= "PASSBAND";
		ap->param_name[FLT_STOPFRQ] 		= "STOPBAND";
		ap->param_name[FLT_PRESCALE]		= "PRESCALE";
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(FSTATVAR):
		ap->param_name[FLT_Q]				= "ACUITY";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_ONEFRQ]			= "FREQUENCY";
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(FLTBANKN):		
		ap->param_name[FLT_Q]				= "Q";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_RANDFACT]		= "FRQ_RANDOMISATION";
		/* fall thro */
	case(FLTBANKC):
		ap->param_name[FLT_LOFRQ]			= "LOW_FRQ";
		ap->param_name[FLT_HIFRQ]			= "HIGH_FRQ";
		switch(mode) {
		case(FLT_LINOFFSET):
			ap->param_name[FLT_OFFSET]		= "FRQ_OFFSET";
			break;
		case(FLT_EQUALSPAN):
			ap->param_name[FLT_INCOUNT]		= "NUMBER_OF_FILTERS";
			break;
		case(FLT_EQUALINT):
			ap->param_name[FLT_INTSIZE]	    = "INTERVAL_(SEMITONES)";
			break;
		}
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(FLTBANKU):		
		ap->param_name[FLT_Q]				= "Q";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(FLTBANKV):
		ap->param_name[FLT_Q]				= "Q";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_HARMCNT]         = "NUMBER_OF_HARMONICS";
		ap->param_name[FLT_ROLLOFF]  		= "ROLL_OFF_(dB)";
		ap->param_name[FILT_TAILV]			= "DECAY_TAIL_DURATION";
		break;
	case(FLTBANKV2):
		ap->param_name[FLT_Q]				= "Q";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FILT_TAILV]			= "DECAY_TAIL_DURATION";
		break;

	case(FLTSWEEP):
		ap->param_name[FLT_Q]				= "ACUITY";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_LOFRQ]			= "LOW_FRQ";
		ap->param_name[FLT_HIFRQ]			= "HIGH_FRQ";
		ap->param_name[FLT_SWPFRQ] 			= "SWEEP_FRQ";
		ap->param_name[FLT_SWPPHASE] 		= "STARTPHASE_OF_SWEEP";
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(FLTITER):
		ap->param_name[FLT_Q]				= "Q";
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_DELAY]  			= "DELAY";
		ap->param_name[FLT_OUTDUR] 			= "OUTFILE_DURATION";
		ap->param_name[FLT_PRESCALE] 		= "PRESCALE";
		ap->param_name[FLT_RANDDEL]  		= "DELAY_RANDOMISATION";
		ap->param_name[FLT_PSHIFT]   		= "MAX_PITCH_SHIFT";
		ap->param_name[FLT_AMPSHIFT] 		= "MAX_LOUDNESS_SHIFT";
		break;

	case(ALLPASS):		
		ap->param_name[FLT_GAIN]			= "GAIN";
		ap->param_name[FLT_DELAY]  			= "DELAY_(MS)";
		ap->param_name[FLT_PRESCALE] 		= "PRESCALE";
		ap->param_name[FILT_TAIL]			= "DECAY_TAIL_DURATION";
		break;

	case(MOD_LOUDNESS):
		ap->param_name[LOUD_GAIN] 			= "GAIN";
		ap->param_name[LOUD_LEVEL] 			= "LEVEL";
		break;

	case(MOD_SPACE):
		switch(mode) {
		case(MOD_PAN):

			ap->param_name[PAN_PAN] 			= "PAN_POSITION";
			ap->param_name[PAN_PRESCALE] 		= "PRESCALING";
			break;
		case(MOD_NARROW):
			ap->param_name[NARROW] 				= "NARROWING";
			break;
		default:
			break;
		}
		break;

	case(SCALED_PAN):
		ap->param_name[PAN_PAN] 			= "PAN_POSITION";
		ap->param_name[PAN_PRESCALE] 		= "PRESCALING";
		break;

	case(MOD_PITCH):
		switch(mode) {
		case(MOD_ACCEL):
			ap->param_name[ACCEL_ACCEL] 	= "ACCELERATION";
			ap->param_name[ACCEL_GOALTIME] 	= "GOAL_TIME";
			ap->param_name[ACCEL_STARTTIME] = "START_TIME";
		 	break;
		case(MOD_VIBRATO):
			ap->param_name[VIB_FRQ] 	    = "CYCLES_PER_SECOND";
			ap->param_name[VIB_DEPTH] 		= "SEMITONE_DEPTH";
		 	break;
		default:
			ap->param_name[VTRANS_TRANS] 	= "TRANSPOSITION";
		 	break;
		}
		break;
	case(MOD_REVECHO):
		if(mode==MOD_STADIUM) {
			ap->param_name[STAD_PREGAIN]		= "INPUT_GAIN";
			ap->param_name[STAD_ROLLOFF]		= "LEVEL_LOSS_WITH_DISTANCE";
			ap->param_name[STAD_SIZE]			= "STADIUM_SIZE_MULTIPLIER";
			ap->param_name[STAD_ECHOCNT]		= "NUMBER_OF_ECHOS";
		} else {
			ap->param_name[DELAY_DELAY]			= "DELAY_(MS)";
			ap->param_name[DELAY_MIX]			= "DELAYED_SIGNAL_IN_MIX";
			ap->param_name[DELAY_FEEDBACK]		= "FEEDBACK";
			ap->param_name[DELAY_LFOMOD]		= "LOW_FRQ_MODULATION_DEPTH";
			ap->param_name[DELAY_LFOFRQ]		= "MODULATION_FRQ:_-ve_FOR_RANDOM";
			ap->param_name[DELAY_LFOPHASE]		= "PHASE_OF_MODULATION";
			ap->param_name[DELAY_LFODELAY]		= "ENTRY_DELAY_OF_MODULATION";
			ap->param_name[DELAY_TRAILTIME]		= "LENGTH_OF_TAIL_TO_SOUND";
			ap->param_name[DELAY_PRESCALE]		= "INPUT_PRESCALING";
			if(mode==MOD_VDELAY)
				ap->param_name[DELAY_SEED]		= "RANDOM_SEED";
		}
			break;
	case(MOD_RADICAL):		
		switch(mode) {
		case(MOD_REVERSE):
			break;
		case(MOD_SHRED):
			ap->param_name[SHRED_CNT]   		= "NUMBER_OF_SHREDS";			
			ap->param_name[SHRED_CHLEN]   		= "AVERAGE_CHUNKLENGTH";			
			ap->param_name[SHRED_SCAT]   		= "CUT_SCATTER";			
			break;

		case(MOD_SCRUB):
			ap->param_name[SCRUB_TOTALDUR]   	= "MINIMUM_OUTPUT_DURATION";			
			ap->param_name[SCRUB_MINSPEED]   	= "LOWEST_DOWNWARD_TRANSPOSITION";			
			ap->param_name[SCRUB_MAXSPEED]   	= "HIGHEST_UPWARD_TRANSPOSITION";			
			ap->param_name[SCRUB_STARTRANGE]   	= "SCRUBS_START_NO_LATER_THAN";			
			ap->param_name[SCRUB_ESTART]   		= "SCRUBS_END_NO_EARLIER_THAN";			
			break;

		case(MOD_LOBIT):
			ap->param_name[LOBIT_BRES]   		= "BIT_RESOLUTION";			
			ap->param_name[LOBIT_TSCAN]   		= "SAMPLING_RATE_DIVISION";			
			break;
		case(MOD_LOBIT2):
			ap->param_name[LOBIT_BRES]   		= "BIT_RESOLUTION";			
			break;
		case(MOD_RINGMOD):
			ap->param_name[RM_FRQ]   			= "MODULATING_FREQUENCY";			
			break;

		case(MOD_CROSSMOD):
			break;

		}
		break;
	case(BRASSAGE):		
		switch(mode) {
		case(GRS_PITCHSHIFT):
			ap->param_name[GRS_PITCH]   		= "PITCHSHIFT_(SEMIT)";			
			break;

		case(GRS_TIMESTRETCH):
			ap->param_name[GRS_VELOCITY]  		= "TIMESHRINK";
			break;

		case(GRS_REVERB):
			ap->param_name[GRS_DENSITY]  		= "DENSITY";
			ap->param_name[GRS_PITCH]   		= "PITCHSHIFT_(SEMIT)";			
			ap->param_name[GRS_AMP]   			= "GRAIN_LOUDNESS_RANGE";			
			ap->param_name[GRS_SRCHRANGE] 		= "SEARCHRANGE_(MS)";			
			break;

		case(GRS_SCRAMBLE):
	    	ap->param_name[GRS_GRAINSIZE]  		= "GRAINSIZE_(MS)";
			ap->param_name[GRS_SRCHRANGE] 		= "SEARCHRANGE_(MS)";			
			break;

		case(GRS_GRANULATE):
			ap->param_name[GRS_DENSITY]  		= "DENSITY";
			break;

		case(GRS_BRASSAGE):
		case(GRS_FULL_MONTY):
			ap->param_name[GRS_VELOCITY]  		= "TIMESHRINK";
			ap->param_name[GRS_DENSITY]  		= "DENSITY";
			if(mode==GRS_FULL_MONTY) {
	    		ap->param_name[GRS_HVELOCITY]  	= "TIMESHRINK_LIMIT";
	    		ap->param_name[GRS_HDENSITY]   	= "DENSITY_LIMIT";
			}
		    ap->param_name[GRS_GRAINSIZE]  		= "GRAINSIZE_(MS)";
			ap->param_name[GRS_PITCH]   		= "PITCHSHIFT_(SEMIT)";			
	    	ap->param_name[GRS_AMP]       		= "GRAIN_LOUDNESS_RANGE";
			ap->param_name[GRS_SPACE]     		= "SPATIAL_POSITION";
		    ap->param_name[GRS_BSPLICE]    		= "STARTSPLICE_(MS)";			
		    ap->param_name[GRS_ESPLICE]    		= "ENDSPLICE_(MS)";			
			if(mode==GRS_FULL_MONTY) {						
			    ap->param_name[GRS_HGRAINSIZE] 	= "GRAINSIZE_LIMIT_(MS)";
			    ap->param_name[GRS_HPITCH]  	= "PITCHSHIFT_LIMIT_(SEMIT)";			
			    ap->param_name[GRS_HAMP]    	= "LOUDNESS_RANGE_LIMIT";									
				ap->param_name[GRS_HSPACE]  	= "SPATIAL_POSITION_LIMIT";
			    ap->param_name[GRS_HBSPLICE]	= "STARTSPLICE_LIMIT_(MS)";
			    ap->param_name[GRS_HESPLICE]	= "ENDSPLICE_LIMIT_(MS)";
			}
	 		ap->param_name[GRS_SRCHRANGE] 		= "SEARCHRANGE_(MS)";			
		   	ap->param_name[GRS_SCATTER]    		= "SCATTER";
	    	ap->param_name[GRS_OUTLEN]   		= "OUTPUT_LENGTH_(SECS)";
	    	ap->param_name[GRS_CHAN_TO_XTRACT] 	= "INPUT_CHAN_TO_EXTRACT_(-ve_N:_OUTPUT_SPATIALISE_TO_N_CHANS)";
			break;
		}
		break;

	case(SAUSAGE):		
		ap->param_name[GRS_VELOCITY]  		= "TIMESHRINK";
		ap->param_name[GRS_DENSITY]  		= "DENSITY";
   		ap->param_name[GRS_HVELOCITY]  		= "TIMESHRINK_LIMIT";
   		ap->param_name[GRS_HDENSITY]   		= "DENSITY_LIMIT";
	    ap->param_name[GRS_GRAINSIZE]  		= "GRAINSIZE_(MS)";
		ap->param_name[GRS_PITCH]   		= "PITCHSHIFT(SEMITONES)";			
    	ap->param_name[GRS_AMP]       		= "GRAIN_LOUDNESS_RANGE";
		ap->param_name[GRS_SPACE]     		= "SPATIAL_POSITION";
	    ap->param_name[GRS_BSPLICE]    		= "STARTSPLICE_(MS)";			
	    ap->param_name[GRS_ESPLICE]    		= "ENDSPLICE_(MS)";			
	    ap->param_name[GRS_HGRAINSIZE] 		= "GRAINSIZE_LIMIT_(MS)";
	    ap->param_name[GRS_HPITCH]  		= "PITCHSHIFT_LIMIT_(SEMIT)";			
	    ap->param_name[GRS_HAMP]    		= "LOUDNESS_RANGE_LIMIT";									
		ap->param_name[GRS_HSPACE]  		= "SPATIAL_POSITION_LIMIT";
	    ap->param_name[GRS_HBSPLICE]		= "STARTSPLICE_LIMIT_(MS)";
	    ap->param_name[GRS_HESPLICE]		= "ENDSPLICE_LIMIT_(MS)";
 		ap->param_name[GRS_SRCHRANGE] 		= "SEARCHRANGE_(MS)";			
	   	ap->param_name[GRS_SCATTER]    		= "SCATTER";
    	ap->param_name[GRS_OUTLEN]   		= "OUTPUT_LENGTH_(SECS)";
    	ap->param_name[GRS_CHAN_TO_XTRACT] 	= "INPUT_CHAN_TO_EXTRACT_(-ve_N:_OUTPUT_SPATIALISE_TO_N_CHANS)";						
		break;

/* TEMPORARY TEST ROUTINE */
	case(WORDCNT):
/* TEMPORARY TEST ROUTINE */
		break;

	case(PVOC_ANAL):
		ap->param_name[PVOC_CHANS_INPUT]  	= "ANALYSIS_POINTS";
		ap->param_name[PVOC_WINOVLP_INPUT]  = "ANALWINDOW_OVERLAP";			
		break;

	case(PVOC_SYNTH):
		break;

	case(PVOC_EXTRACT):
		ap->param_name[PVOC_CHANS_INPUT]  	= "ANALYSIS_POINTS";
		ap->param_name[PVOC_WINOVLP_INPUT]  = "ANALWINDOW_OVERLAP";			
		ap->param_name[PVOC_CHANSLCT_INPUT] = "SELECT_A_CHANNEL";			
		ap->param_name[PVOC_LOCHAN_INPUT]   = "BOTTOM_RESYNTH_CHANNEL";			
		ap->param_name[PVOC_HICHAN_INPUT]   = "TOP_RESYNTH_CHANNEL";			
		break;

	case(EDIT_CUT):
		ap->param_name[CUT_CUT]  	= "TIME_OF_STARTCUT";
		ap->param_name[CUT_END]  	= "TIME_OF_ENDCUT";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(EDIT_CUTMANY):
		ap->param_name[CM_SPLICELEN]  = "SPLICELEN_(MS)";
		break;
	case(STACK):
		ap->param_name[STACK_CNT]    = "NUMBER_OF_ITEMS";
		ap->param_name[STACK_LEAN]   = "STACK_LEAN";
		ap->param_name[STACK_OFFSET] = "ATTACK_TIME";
		ap->param_name[STACK_GAIN]   = "OVERALL_GAIN";
		ap->param_name[STACK_DUR]    = "HOW_MUCH_OF_OUPUT_TO_MAKE";
		break;

	case(EDIT_CUTEND):
		ap->param_name[CUT_CUT]  	= "LENGTH_TO_KEEP";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(EDIT_ZCUT):
		ap->param_name[CUT_CUT]  	= "TIME_OF_STARTCUT";
		ap->param_name[CUT_END]  	= "TIME_OF_ENDCUT";
		break;

	case(EDIT_EXCISE):
		ap->param_name[CUT_CUT]  	= "TIME_OF_STARTCUT";
		ap->param_name[CUT_END]  	= "TIME_OF_ENDCUT";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(EDIT_EXCISEMANY):
	case(INSERTSIL_MANY):
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(EDIT_INSERT):
		ap->param_name[CUT_CUT]  	= "INSERTION_TIME";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		ap->param_name[INSERT_LEVEL]= "INSERT_LEVEL";
		break;

	case(EDIT_INSERT2):
		ap->param_name[CUT_CUT]  	= "INSERTION_TIME";
		ap->param_name[CUT_END]  	= "END_TIME_OF_OVERWRITE";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		ap->param_name[INSERT_LEVEL]= "INSERT_LEVEL";
		break;

	case(EDIT_INSERTSIL):
		ap->param_name[CUT_CUT]  	= "INSERTION_TIME";
		ap->param_name[CUT_END]  	= "DURATION_OF_SILENCE";
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(JOIN_SEQ):
		ap->param_name[MAX_LEN]  	= "MAX_NUMBER_OF_ITEMS_IN_PATTERN";
		/* fall thro */
	case(EDIT_JOIN):
	case(JOIN_SEQDYN):
		ap->param_name[CUT_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(HOUSE_COPY):
		switch(mode) {
		case(COPYSF):
			break;
		case(DUPL):
			ap->param_name[COPY_CNT]  	= "NUMBER_OF_DUPLICATES";
			break;
		}
		break;

	case(HOUSE_DEL):
		break;

	case(HOUSE_CHANS):
		switch(mode) {
		case(HOUSE_CHANNEL):
			ap->param_name[CHAN_NO]  	= "CHANNEL_TO_GET";
			break;
		case(HOUSE_ZCHANNEL):
			ap->param_name[CHAN_NO]  	= "CHANNEL_TO_ZERO";
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
			ap->param_name[SORT_SMALL] = "MAX_DURATION_SMALLEST_GROUPING";
			ap->param_name[SORT_LARGE] = "MAX_DURATION_LARGEST_GROUPING";
			ap->param_name[SORT_STEP]  = "STEP_BETWEEN_GROUPINGS";
			break;
		default:
			break;
		}
		break;

	case(HOUSE_SPEC):
		switch(mode) {
		case(HOUSE_REPROP):
			ap->param_name[HSPEC_SRATE] = "SAMPLE_RATE";
			ap->param_name[HSPEC_CHANS] = "CHANNELS";
			ap->param_name[HSPEC_TYPE]  = "SAMPLE_TYPE";
			break;
		case(HOUSE_RESAMPLE):
			ap->param_name[HSPEC_SRATE] = "SAMPLE_RATE";
			break;
		default:
			break;
		}
		break;

	case(HOUSE_EXTRACT):
		switch(mode) {
		case(HOUSE_CUTGATE):
			ap->param_name[CUTGATE_SPLEN]  		= "SPLICE_LENGTH_(MS)";
			/* fall thro */
		case(HOUSE_ONSETS):
			ap->param_name[CUTGATE_GATE]  		= "GATE_LEVEL";
			ap->param_name[CUTGATE_ENDGATE]  	= "ENDGATE_LEVEL";
			ap->param_name[CUTGATE_THRESH]  	= "THRESHOLD";
			if(mode == HOUSE_CUTGATE)
				ap->param_name[CUTGATE_SUSTAIN] = "RETAIN_TO";
			ap->param_name[CUTGATE_BAKTRAK]  	= "BAKTRAK_BY";
			ap->param_name[CUTGATE_INITLEVEL]  	= "INITIAL_LEVEL";
			ap->param_name[CUTGATE_MINLEN]  	= "MINIMUM_DURATION_(SECS)";
			ap->param_name[CUTGATE_WINDOWS]  	= "GATE_WINDOWS";
			break;
		case(HOUSE_TOPNTAIL):
			ap->param_name[TOPN_GATE]  	= "GATE_LEVEL";
			ap->param_name[TOPN_SPLEN]  = "SPLICE_LENGTH_(MS)";
			break;
		case(HOUSE_RECTIFY):
			ap->param_name[RECTIFY_SHIFT]  	= "SHIFT";
			break;
		case(HOUSE_BYHAND):
			break;
		default:
			break;
		}
		break;

	case(TOPNTAIL_CLICKS):
		ap->param_name[TOPN_GATE]  	= "GATE_LEVEL";
		ap->param_name[TOPN_SPLEN]  = "SPLICE_LENGTH_(MS)";
		break;

	case(HOUSE_BAKUP):
	case(HOUSE_DUMP):
		break;
	case(HOUSE_GATE):
		ap->param_name[GATE_ZEROS]  = "MIN_ZERO_CNT_FOR_CUT_POINT";
		break;
	case(HOUSE_RECOVER):
		ap->param_name[DUMP_OK_CNT]  = "HEADER_COPIES_RETRIEVED";
		ap->param_name[DUMP_OK_PROP] = "MARKER_VALIDITY";
		ap->param_name[DUMP_OK_SAME] = "HEADER_CONSISTENCY";
		ap->param_name[SFREC_SHIFT]  = "SIGNAL_SHIFT_TO_APPLY";
		break;
	case(HOUSE_DISK):

	case(INFO_PROPS):
	case(INFO_SFLEN):
	case(INFO_TIMELIST):
	case(INFO_LOUDLIST):
	case(INFO_TIMEDIFF):
	case(INFO_LOUDCHAN):
	case(INFO_MAXSAMP):
		break;
	case(INFO_MAXSAMP2):
		ap->param_name[MAX_STIME] = "START_TIME_OF_SEARCH";
		ap->param_name[MAX_ETIME] = "END_TIME_OF_SEARCH";
		break;

 	case(INFO_PRNTSND):
		ap->param_name[PRNT_START]  = "START_TIME_IN_SOUND";
		ap->param_name[PRNT_END]  	= "END_TIME_IN_SOUND";
		break;

 	case(INFO_MUSUNITS):
		switch(mode) {
		case(MU_NOTE_TO_FRQ):
		case(MU_NOTE_TO_MIDI):
		case(MU_INTVL_TO_FRQRATIO):
		case(MU_INTVL_TO_TSTRETCH):
			break;
		default:
			switch(mode) {
			case(MU_MIDI_TO_FRQ):
				ap->param_name[MUSUNIT]  = "MIDI_VALUE";
				break;
			case(MU_FRQ_TO_MIDI):
				ap->param_name[MUSUNIT]  = "FRQ_VALUE_(HZ)";
				break;
			case(MU_FRQRATIO_TO_SEMIT):
			case(MU_FRQRATIO_TO_INTVL):
			case(MU_FRQRATIO_TO_OCTS):
			case(MU_FRQRATIO_TO_TSTRETH):
				ap->param_name[MUSUNIT]  = "RATIO_OF_FREQUENCIES";
				break;
			case(MU_SEMIT_TO_FRQRATIO):
			case(MU_SEMIT_TO_OCTS):
			case(MU_SEMIT_TO_INTVL):
			case(MU_SEMIT_TO_TSTRETCH):
				ap->param_name[MUSUNIT]  = "INTERVAL_IN_SEMITONES";
				break;
			case(MU_OCTS_TO_FRQRATIO):
			case(MU_OCTS_TO_SEMIT):
			case(MU_OCTS_TO_TSTRETCH):
				ap->param_name[MUSUNIT]  = "INTERVAL_IN_OCTAVES";
				break;
			case(MU_TSTRETCH_TO_FRQRATIO):
			case(MU_TSTRETCH_TO_SEMIT):
			case(MU_TSTRETCH_TO_OCTS):
			case(MU_TSTRETCH_TO_INTVL):
				ap->param_name[MUSUNIT]  = "TIME_RATIO";
				break;
			case(MU_GAIN_TO_DB):
				ap->param_name[MUSUNIT]  = "GAIN_MULTIPLIER";
				break;
			case(MU_DB_TO_GAIN):
				ap->param_name[MUSUNIT]  = "dB_GAIN";
				break;
			case(MU_FRQ_TO_NOTE):
				ap->param_name[MUSUNIT]  = "FRQ_VALUE_(HZ)";
				break;
			case(MU_MIDI_TO_NOTE):
				ap->param_name[MUSUNIT]  = "MIDI_VALUE";
				break;
			}
			break;
		}
		break;

 	case(INFO_DIFF):
 	case(INFO_CDIFF):
		ap->param_name[SFDIFF_THRESH]  	= "MAX_ACCEPTABLE_WANDER";
		ap->param_name[SFDIFF_CNT]  	= "MAX_DIFFERENCES_TO_ACCEPT";
		break;

 	case(INFO_FINDHOLE):
		ap->param_name[HOLE_THRESH]  	= "THRESHOLD_LEVEL";
		break;

 	case(INFO_TIMETOSAMP):
		ap->param_name[INFO_TIME]  	= "TIME_IN_SECONDS";
		break;

 	case(INFO_SAMPTOTIME):
		ap->param_name[INFO_SAMPS]  	= "TIME_AS_SAMPLE_COUNT";
		break;

 	case(INFO_TIMESUM):
		ap->param_name[TIMESUM_SPLEN]  	= "SPLICELEN_(MS)";
		break;

	case(SYNTH_WAVE):
		ap->param_name[SYN_SRATE]  		= "SAMPLING_RATE";
		ap->param_name[SYN_CHANS]  		= "NUMBER_OF_CHANNELS";
		ap->param_name[SYN_DUR]  		= "DURATION";
		ap->param_name[SYN_FRQ]  		= "FREQUENCY";
		ap->param_name[SYN_AMP]  		= "AMPLITUDE";
		ap->param_name[SYN_TABSIZE]  	= "WAVEFORM_TABLESIZE";
		break;

	case(MULTI_SYN):
		ap->param_name[SYN_SRATE]  		= "SAMPLING_RATE";
		ap->param_name[SYN_CHANS]  		= "NUMBER_OF_CHANNELS";
		ap->param_name[SYN_DUR]  		= "DURATION";
		ap->param_name[SYN_AMP]  		= "AMPLITUDE";
		ap->param_name[SYN_TABSIZE]  	= "WAVEFORM_TABLESIZE";
		break;

 	case(SYNTH_NOISE):
		ap->param_name[SYN_SRATE]  		= "SAMPLING_RATE";
		ap->param_name[SYN_CHANS]  		= "NUMBER_OF_CHANNELS";
		ap->param_name[SYN_DUR]  		= "DURATION";
		ap->param_name[SYN_AMP]  		= "AMPLITUDE";
		break;

 	case(SYNTH_SIL):
		ap->param_name[SYN_SRATE]  		= "SAMPLING_RATE";
		ap->param_name[SYN_CHANS]  		= "NUMBER_OF_CHANNELS";
		ap->param_name[SYN_DUR]  		= "DURATION";
		break;

 	case(SYNTH_SPEC):
		ap->param_name[SS_DUR]  		= "DURATION";
		ap->param_name[SS_CENTRFRQ]  	= "CENTRE_FREQUENCY";
		ap->param_name[SS_SPREAD]  		= "BAND_SPREAD";
		ap->param_name[SS_FOCUS]  		= "BAND_FOCUS_(MAX)";
		ap->param_name[SS_FOCUS2]  		= "BAND_FOCUS_(MIN)";
		ap->param_name[SS_TRAND]  		= "TIME_VARIATION";
		ap->param_name[SS_SRATE]  		= "SAMPLE_RATE";
		break;

 	case(RANDCUTS):
	ap->param_name[RC_CHLEN]   = "AVERAGE_CHUNKLENGTH";			
	ap->param_name[RC_SCAT]    = "SCATTERING";
		break;

 	case(RANDCHUNKS):
		ap->param_name[CHUNKCNT]  	= "NUMBER_OF_CHUNKS";
		ap->param_name[MINCHUNK]  	= "MIN_CHUNK_LENGTH";
		ap->param_name[MAXCHUNK] 	= "MAX_CHUNK_LENGTH";
		break;

 	case(SIN_TAB):
		ap->param_name[SIN_FRQ]  	= "CYCLE_LENGTH";
		ap->param_name[SIN_AMP]  	= "PAN_WIDTH";
		ap->param_name[SIN_DUR] 	= "DURATION";
		ap->param_name[SIN_QUANT] 	= "QUANTISATION";
		ap->param_name[SIN_PHASE] 	= "START_PHASE";
		break;

 	case(ACC_STREAM):
		ap->param_name[ACC_ATTEN] 	= "ATTENUATION";
		break;

 	case(HF_PERM1):
 	case(HF_PERM2):
		ap->param_name[HP1_SRATE] 		= "SAMPLE_RATE";
		ap->param_name[HP1_ELEMENT_SIZE]= "DURATION_OF_CHORDS";
		ap->param_name[HP1_GAP_SIZE]	= "DURATION_OF_PAUSES_BETWEEN_CHORDS";
		ap->param_name[HP1_GGAP_SIZE]	= "DURATION_PAUSES_BETWEEN_CHORD-GROUPS";
		ap->param_name[HP1_MINSET] 		= "MIN_NUMBER_OF_NOTES_PER_CHORD";
		ap->param_name[HP1_BOTNOTE]		= "BOTTOM_NOTE_OF_RANGE";
		ap->param_name[HP1_BOTOCT]		= "OCTAVE_OF_BOTTOM_NOTE";
		ap->param_name[HP1_TOPNOTE]		= "TOP_NOTE_OF_RANGE";
		ap->param_name[HP1_TOPOCT]		= "OCTAVE_OF_TOP_NOTE";
		ap->param_name[HP1_SORT1]		= "SORT_CHORDS_BY";
		break;
 	case(DEL_PERM):
		ap->param_name[HP1_SRATE] 	= "SAMPLE_RATE";
		ap->param_name[DP_DUR]		= "INITAL_DURATION_OF_NOTES";
		/* fall thro */
 	case(DEL_PERM2):
		ap->param_name[DP_CYCCNT]	= "CYCLES_OF_PERMUTATION";
		break;
 	case(TWIXT):
 	case(SPHINX):
		ap->param_name[IS_SPLEN] 	= "SPLICE_LENGTH_(MS)";
		ap->param_name[IS_SEGCNT]	= "NUMBER_OF_SEGMENTS_TO_OUTPUT";
		if(process != TWIXT || mode != TRUE_EDIT)
			ap->param_name[IS_WEIGHT]	= "WEIGHTING_OF_FIRST_FILE";
		break;
 	case(NOISE_SUPRESS):
		ap->param_name[NOISE_SPLEN] = "SPLICE_LENGTH_(MS)";
		ap->param_name[NOISE_MINFRQ] = "FRQ_ABOVE_WHICH_SIGNAL_COULD_BE_NOISE_(HZ)";
		ap->param_name[MIN_NOISLEN]	= "MAXIMUM_DURATION_OF_NOISE_TO_KEEP_(MS)";
		ap->param_name[MIN_TONELEN]	= "MINIMUM_DURATION_OF_TONE_TO_KEEP_(MS)";
		break;
 	case(TIME_GRID):
		ap->param_name[GRID_COUNT]  = "NUMBER_OF_GRIDS";
		ap->param_name[GRID_WIDTH]  = "WIDTH_OF_GRID_WINDOWS";
		ap->param_name[GRID_SPLEN]  = "SPLICE_LENGTH_(MS)";
		break;
 	case(SEQUENCER2):
		ap->param_name[SEQ_SPLIC]  = "SPLICE_(MS)";
		/* fall thro */
	case(SEQUENCER):
		ap->param_name[SEQ_ATTEN]  = "SOURCE_ATTENUATION";
		break;
	case(CONVOLVE):
		switch(mode) {
		case(CONV_NORMAL):
			break;
		case(CONV_TVAR):
			ap->param_name[CONV_TRANS]  = "SEMITONE_TRANSPOSITION_OF_CONVOLVING_FILE";
			break;
		}
		break;
	case(BAKTOBAK):
		ap->param_name[BTOB_CUT]   = "TIME_OF_JOIN";
		ap->param_name[BTOB_SPLEN] = "SPLICE_LENGTH_(MS)";
		break;
	case(FIND_PANPOS):
		ap->param_name[PAN_PAN]   = "AT_TIME";
		break;
	case(CLICK):
		switch(mode) {
		case(CLICK_BY_TIME):
			ap->param_name[CLIKSTART] = "MAKE_CLICK_FROM_TIME";
			ap->param_name[CLIKEND]   = "MAKE_CLICK_TO_TIME";
			break;
		case(CLICK_BY_LINE):
			ap->param_name[CLIKSTART] = "MAKE_CLICK_FROM_DATA_LINE";
			ap->param_name[CLIKEND]   = "MAKE_CLICK_TO_DATA_LINE";
			break;
		}
		ap->param_name[CLIKOFSET] = "DATA_LINE_WHERE_MUSIC_STARTS";
		break;
	case(DOUBLETS):
		ap->param_name[BTOB_CUT]   = "SEGMENTATION_LENGTH";
		ap->param_name[BTOB_SPLEN] = "SEGMENT_REPEATS";
		break;
	case(SYLLABS):
		ap->param_name[SYLLAB_DOVETAIL]   = "DOVETAIL_TIME_(MS)";
		ap->param_name[SYLLAB_SPLICELEN] = "SPLICE_LENGTH_(MS)";
		break;
	case(MAKE_VFILT):
	case(MIX_MODEL):
		break;
	case(BATCH_EXPAND):
		ap->param_name[BE_INFILE]   = "INFILE_COLUMN_NUMBER_IN_BATCHFILE";
		ap->param_name[BE_OUTFILE]  = "OUTFILE_COLUMN_NUMBER_IN_BATCHFILE";
		ap->param_name[BE_PARAM]    = "PARAM_COLUMN_NUMBER_IN_BATCHFILE";
		break;
	case(CYCINBETWEEN):
		ap->param_name[INBETW]		= "NUMBER_OF_INTERMEDIATE_FILES";
		ap->param_name[BTWN_HFRQ]	= "HIGH_FREQUENCY_CUTOFF";
		break;
	case(ENVSYN):
		ap->param_name[ENVSYN_WSIZE]	  = "ENVELOPE_WINDOW_SIZE_(MS)";
		ap->param_name[ENVSYN_DUR]		  = "DURATION_OF_OUTFILE";
		ap->param_name[ENVSYN_CYCLEN]	  = "DURATION_OF_REPEATING_UNIT";
		ap->param_name[ENVSYN_STARTPHASE] = "START_POINT_WITHIN_FIRST_UNIT";
		switch(mode) {
		case(ENVSYN_RISING):
			ap->param_name[ENVSYN_TROF]  = "HEIGHT_OF_ENVELOPE_TROUGH";
			ap->param_name[ENVSYN_EXPON] = "ENVELOPE_RISE_EXPONENT";
			break;
		case(ENVSYN_FALLING):
			ap->param_name[ENVSYN_TROF]  = "HEIGHT_OF_ENVELOPE_TROUGH";
			ap->param_name[ENVSYN_EXPON] = "ENVELOPE_DECAY_EXPONENT";
			break;
		case(ENVSYN_TROFFED):
			ap->param_name[ENVSYN_TROF]  = "HEIGHT_OF_ENVELOPE_TROUGH";
			ap->param_name[ENVSYN_EXPON] = "ENVELOPE_DECAY_AND_RISE_EXPONENT";
			break;
		}
		break;
	case(RRRR_EXTEND):
		if(mode == 1) {
			ap->param_name[RRR_GATE]	  = "GATE_LEVEL_BELOW_WHICH_SIGNAL_ENVELOPE_IGNORED";
			ap->param_name[RRR_SKIP]	  = "NUMBER_OF_UNITS_AT_ITERATIVE_START_TO_OMIT";
			ap->param_name[RRR_GET]	      = "MINIMUM_NO_OF_SEGMENTS_TO_FIND_IN_SRC";
			ap->param_name[RRR_GRSIZ]	  = "APPROX_SIZE_OF_GRANULE_(MS)";
			ap->param_name[RRR_AFTER]	  = "TIME_IN_EXTENDED_ITERATIVE_BEFORE_RITARNDO";
			ap->param_name[RRR_TEMPO]	  = "EVENT_SEPARATION_IN_FULLY_SLOWED_ITERATIVE";
			ap->param_name[RRR_AT]		  = "TIME_IN_EXTENDED_ITERATIVE_WHERE_RIT_ENDS";
		} else {
			ap->param_name[RRR_START]	  = "START_OF_SECTION_TO_BE_EXTENDED";
			ap->param_name[RRR_END]		  = "END_OF_SECTION_TO_BE_EXTENDED";
			ap->param_name[RRR_GET]	      = "ANTICIPATED_NO_OF_SEGMENTS_TO_FIND_IN_SRC";
		}
		ap->param_name[RRR_RANGE]		  = "APPROX_RANGE_OF_ITERATIVE_SOUND_(OCTAVES)";
		if(mode != 2) {
			ap->param_name[RRR_STRETCH]	  = "TIMESTRETCH_OF_SECTION";
			ap->param_name[RRR_REPET]	  = "MAX_ADJACENT_OCCURENCES_OF_ANY_SEG_IN_OUTPUT";
			ap->param_name[RRR_ASCAT]	  = "AMPLITUDE_SCATTER_(MULTIPLIER)";
			ap->param_name[RRR_PSCAT]	  = "PITCH_SCATTER_(SEMITONES)";
		}
		break;
	case(SSSS_EXTEND):
		ap->param_name[SSS_DUR]		 = "OUTPUT_DURATION";
		ap->param_name[NOISE_MINFRQ] = "MIN_FRQ_(HZ)_JUDGED_TO_REPRESENT_NOISE";
		ap->param_name[MIN_NOISLEN]	 = "MIN_DURATION_OF_USUABLE_BLOCK_OF_NOISE_IN_SIGNAL_(MS)";
		ap->param_name[MAX_NOISLEN]	 = "MAX_DURATION_OF_USUABLE_BLOCK_OF_NOISE_IN_SIGNAL_(SECS)";
		ap->param_name[SSS_GATE]	 = "NOISE_MUST_BE_ABOVE_THIS_GATE_LEVEL";
		break;
	case(HOUSE_GATE2):
		ap->param_name[GATE2_DUR]	  = "MAX_GLITCH_DURATION_(MS)";
		ap->param_name[GATE2_ZEROS]	  = "MIN_GLITCH_SEPARATION_(MS)";
		ap->param_name[GATE2_LEVEL]	  = "GATING_LEVEL_AT_GLITCH_EDGE";
		ap->param_name[GATE2_SPLEN]	  = "SPLICE_LENGTH_(MS)";
		ap->param_name[GATE2_FILT]	  = "SEARCH_WINDOW_(MS)";
		break;
	case(GRAIN_ASSESS):
		break;
	case(ZCROSS_RATIO):
		ap->param_name[ZC_START]	= "START_TIME";
		ap->param_name[ZC_END]		= "END_TIME";
		break;
	case(GREV):
		ap->param_name[GREV_WSIZE]		= "ENVELOPE_WINDOW_SIZE_(MS)";
		ap->param_name[GREV_TROFRAC]	= "DEPTH_OF_TROUGHS_AS_PROPORTION_OF_PEAK_HEIGHT";		
		ap->param_name[GREV_GPCNT]		= "NO_OF_GRAINS_GROUPED_AS_ONE_UNIT";
		switch(mode) {
		case(GREV_TSTRETCH):
			ap->param_name[GREV_TSTR]	= "TIME_STRETCH_RATIO";		
			break;
		case(GREV_DELETE):
		case(GREV_OMIT):
			ap->param_name[GREV_KEEP]	= "NO_OF_UNITS_TO_DELETE";
			ap->param_name[GREV_OUTOF]	= "OUT_OF";
			break;
		case(GREV_REPEAT):
			ap->param_name[GREV_REPETS]	= "NO_OF_UNIT_REPETITIONS";
			break;
		}
		break;
	case(MANY_ZCUTS):
		break;
	default:
		sprintf(errstr,"Unknown case: get_param_names()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}


