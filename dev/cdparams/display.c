#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<structures.h>
#include 	<tkglobals.h>
#include 	<cdparams.h>
#include	<globcon.h>
#include	<localcon.h>
#include	<processno.h>
#include	<modeno.h>
#include	<pnames.h>
#include 	<sfsys.h>

#define	SUBRANGE	(TRUE)

void setup_display(int paramno,int dtype,int subrang,double lo,double hi,aplptr ap);

/****************************** ESTABLISH_DISPLAY *********************************/

int establish_display(int process,int mode,int total_params,float frametime,double duration,aplptr ap)
{
 	double mintime;

	if((ap->display_type = (char *)malloc((total_params)* sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: for display_type array\n");
		return(MEMORY_ERROR);
	}

	if((ap->has_subrange = (char *)malloc((total_params) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: for subrange array\n");
		return(MEMORY_ERROR);
	}

	if((ap->lolo = (double *)malloc((total_params) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: for lo subrange vals\n");
		return(MEMORY_ERROR);
	}

	if((ap->hihi = (double *)malloc((total_params) * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: for hi subrange vals\n");
		return(MEMORY_ERROR);
	}

 		/******************************* SPEC *******************************/
 		/******************************* SPEC *******************************/
 		/******************************* SPEC *******************************/

	switch(process) {
	case(ACCU):
		setup_display(ACCU_DINDEX,LINEAR,0,0.0,0.0,ap);
		setup_display(ACCU_GINDEX,LINEAR,SUBRANGE,-1.0,1.0,ap);
		break;
	case(ALT):		
		break;
	case(ARPE):
		setup_display(ARPE_WTYPE,WAVETYPE,0,0.0,0.0,ap);	
		setup_display(ARPE_ARPFRQ,LINEAR,0,0.0,0.0,ap);

		if(mode==ABOVE_BOOST || mode==ONCE_ABOVE)
			setup_display(ARPE_PHASE,NUMERIC,0,0.0,0.0,ap);
		else
			setup_display(ARPE_PHASE,NUMERIC,0,0.0,0.0,ap);



		setup_display(ARPE_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(ARPE_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(ARPE_HBAND,LINEAR,SUBRANGE,ap->lo[ARPE_HBAND],min(ap->hi[ARPE_HBAND],200.0),ap); 
		setup_display(ARPE_AMPL,LINEAR,SUBRANGE,ap->lo[ARPE_AMPL],64.0,ap);
		if(mode==ON 
		|| mode==BOOST 
		|| mode==BELOW_BOOST 
		|| mode==ABOVE_BOOST) {
			setup_display(ARPE_NONLIN,LOG,SUBRANGE,0.25,4.0,ap);
			setup_display(ARPE_SUST,LINEAR,SUBRANGE,ap->lo[ARPE_SUST],min(ap->hi[ARPE_SUST],64.0),ap);
		}
		break;
	case(AVRG):
		setup_display(AVRG_AVRG,LINEAR,0,0.0,0.0,ap);
		break;
	case(BARE):		
		break;
	case(BLUR):
		setup_display(BLUR_BLURF,LINEAR,SUBRANGE,ap->lo[BLUR_BLURF],min(ap->hi[BLUR_BLURF],64.0),ap);
		break;
	case(BLTR):
		setup_display(BLUR_BLURF,LINEAR,SUBRANGE,ap->lo[BLUR_BLURF],min(ap->hi[BLUR_BLURF],64.0),ap);
		setup_display(BLTR_TRACE,LOG,0,0.0,0.0,ap);
		break;
	case(BRIDGE):
		setup_display(BRG_OFFSET,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_SF2,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_SA2,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_EF2,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_EA2,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_STIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(BRG_ETIME,NUMERIC,0,0.0,0.0,ap);
		break;
	case(CHANNEL):
		setup_display(CHAN_FRQ,NUMERIC,0,0.0,0.0,ap);
		break;
	case(CHORD):
	case(MULTRANS):
		setup_display(CHORD_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(CHORD_HIFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(CHORUS):
		if(mode==CH_AMP
		|| mode==CH_AMP_FRQ 
		|| mode==CH_AMP_FRQ_UP 
		|| mode==CH_AMP_FRQ_DN)
			setup_display(CHORU_AMPR,LOG,0,0.0,0.0,ap);
		if(mode==CH_FRQ     
		|| mode==CH_FRQ_UP     
		|| mode==CH_FRQ_DN 
		|| mode==CH_AMP_FRQ 
		|| mode==CH_AMP_FRQ_UP 
		|| mode==CH_AMP_FRQ_DN)
			setup_display(CHORU_FRQR,LINEAR,0,0.0,0.0,ap);
		break;
	case(CLEAN):
		switch(mode) {
		case(FROMTIME):
		case(ANYWHERE):
			setup_display(CL_SKIPT,NUMERIC,0,0.0,0.0,ap);
			break;
		case(FILTERING):
			setup_display(CL_FRQ,PLOG,0,0.0,0.0,ap);
			break;
		}
		setup_display(CL_GAIN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(CROSS):
		setup_display(CROS_INTP,LINEAR,0,0.0,0.0,ap);
		break;
	case(CUT):
		setup_display(CUT_STIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_ETIME,NUMERIC,0,0.0,0.0,ap);
		break;
	case(DIFF):
		setup_display(DIFF_CROSS,LINEAR,0,0.0,0.0,ap);
		break;
	case(DRUNK):
		setup_display(DRNK_RANGE,LINEAR,SUBRANGE,ap->lo[DRNK_RANGE],min(ap->hi[DRNK_RANGE],64.0),ap);
		setup_display(DRNK_STIME,NUMERIC,0,0.0,0.0,ap);
/* OLD		setup_display(DRNK_DUR,NUMERIC,0,0.0,0.0,ap); */
		setup_display(DRNK_DUR,LINEAR,SUBRANGE,0.0,duration * 4.0,ap);
		break;
	case(EXAG):
		setup_display(EXAG_EXAG,LOG,SUBRANGE,0.25,4.0,ap);
		break;
	case(FILT):
		setup_display(FILT_PG,LINEAR,SUBRANGE,ap->lo[FILT_PG],16.0,ap);
		setup_display(FILT_FRQ1,PLOG,0,0.0,0.0,ap);
		setup_display(FILT_QQ,LOG,0,0.0,0.0,ap); 
		setup_display(FILT_FRQ2,PLOG,0,0.0,0.0,ap);
		break;
	case(FMNTSEE):	
	case(FORMANTS):	
	case(FORMSEE):	
		break;
	case(FOCUS):
		setup_display(FOCU_PKCNT,NUMERIC,0,0.0,0.0,ap);
		setup_display(FOCU_BW,LOG,0,0.0,0.0,ap);
		setup_display(FOCU_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FOCU_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FOCU_STABL,LINEAR,SUBRANGE,ap->lo[FOCU_STABL],64.0,ap);
		break;
	case(FOLD):
		setup_display(FOLD_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FOLD_HIFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(FORM):
		setup_display(FORM_FTOP,PLOG,0,0.0,0.0,ap);
		setup_display(FORM_FBOT,PLOG,0,0.0,0.0,ap);
		setup_display(FORM_GAIN,LINEAR,SUBRANGE,ap->lo[FORM_GAIN],16.0,ap);
		break;
	case(FREEZE):	
	case(FREEZE2):	
		break;
	case(FREQUENCY):
		setup_display(FRQ_CHAN,LINEAR,SUBRANGE,ap->lo[FRQ_CHAN],64.0,ap);
		break;
	case(GAIN):
		setup_display(GAIN_GAIN,LINEAR,SUBRANGE,ap->lo[GAIN_GAIN],16.0,ap);
		break;
	case(GLIDE):
		setup_display(GLIDE_DUR,NUMERIC,0,0.0,0.0,ap);
		break;
	case(GLIS):
		setup_display(GLIS_RATE,LINEAR,SUBRANGE,-4.0,4.0,ap);
		setup_display(GLIS_HIFRQ,PLOG,0,0.0,0.0,ap);
		if(mode==INHARMONIC)
			setup_display(GLIS_SHIFT,LINEAR,SUBRANGE,ap->lo[GLIS_SHIFT],min(ap->hi[GLIS_SHIFT],200.0),ap);
		break;															 
	case(GRAB):
		setup_display(GRAB_FRZTIME,NUMERIC,0,0.0,0.0,ap);
		break;
	case(GREQ):		
		break;
	case(INVERT):	
		break;
	case(LEAF):
		setup_display(LEAF_SIZE,LINEAR,SUBRANGE,ap->lo[LEAF_SIZE],min(ap->hi[LEAF_SIZE],16.0),ap);
		break;
	case(LEVEL):	
		break;
	case(MAGNIFY):
		setup_display(MAG_FRZTIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(MAG_DUR,LINEAR,SUBRANGE,0.0,60.0,ap);
		break;
	case(MAKE):		
		break;
	case(MAX):		
		break;
	case(MEAN):
		setup_display(MEAN_LOF,PLOG,0,0.0,0.0,ap);
		setup_display(MEAN_HIF,PLOG,0,0.0,0.0,ap);
		setup_display(MEAN_CHAN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MORPH):
		setup_display(MPH_ASTT,NUMERIC,0,0.0,0.0,ap);
		setup_display(MPH_AEND,NUMERIC,0,0.0,0.0,ap);
		setup_display(MPH_FSTT,NUMERIC,0,0.0,0.0,ap);
		setup_display(MPH_FEND,NUMERIC,0,0.0,0.0,ap);
		setup_display(MPH_AEXP,LOG,SUBRANGE,0.25,4.0,ap);
		setup_display(MPH_FEXP,LOG,SUBRANGE,0.25,4.0,ap);
		setup_display(MPH_STAG,NUMERIC,0,0.0,0.0,ap);
		break;
	case(NOISE):
		setup_display(NOISE_NOIS,LINEAR,0,0.0,0.0,ap);
		break;
	case(OCT):
		setup_display(OCT_HMOVE,LINEAR,SUBRANGE,ap->lo[OCT_HMOVE],64.0,ap);	
		setup_display(OCT_BREI,LINEAR,0,0.0,0.0,ap);
		break;
	case(OCTVU):
		mintime = min(ap->hi[OCTVU_TSTEP],DEFAULT_TIME_STEP * 2.0);
		mintime = min(mintime,duration * SECS_TO_MS);
		setup_display(OCTVU_TSTEP,LINEAR,SUBRANGE,ap->lo[OCTVU_TSTEP],mintime,ap);
		setup_display(OCTVU_FUND,PLOG,0,0.0,0.0,ap);
		break;
	case(P_APPROX):
		setup_display(PA_PRANG,LINEAR,SUBRANGE,ap->lo[PA_PRANG],12.0,ap);
		setup_display(PA_TRANG,LINEAR,SUBRANGE,ap->lo[PA_TRANG],min(ap->hi[PA_TRANG],200.0),ap);
		setup_display(PA_SRANG,LINEAR,SUBRANGE,ap->lo[PA_SRANG],min(ap->hi[PA_SRANG],200.0),ap);
		break;
	case(P_CUT):
		setup_display(PC_STT,NUMERIC,0,0.0,0.0,ap);
		setup_display(PC_END,NUMERIC,0,0.0,0.0,ap);	 
		break;
	case(P_EXAG):
		setup_display(PEX_RANG,LINEAR,0,0.0,0.0,ap);
		setup_display(PEX_MEAN,LINEAR,0,0.0,0.0,ap);
		setup_display(PEX_CNTR,LINEAR,0,0.0,0.0,ap);
		break;
	case(P_FIX):
		setup_display(PF_SCUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(PF_ECUT,NUMERIC,0,0.0,0.0,ap);	
		setup_display(PF_LOF,PLOG,0,0.0,0.0,ap);
		setup_display(PF_HIF,PLOG,0,0.0,0.0,ap);
		setup_display(PF_SMOOTH,LINEAR,SUBRANGE,ap->lo[PF_SMOOTH],8.0,ap);
		setup_display(PF_SMARK,PLOG,0,0.0,0.0,ap);
		setup_display(PF_EMARK,PLOG,0,0.0,0.0,ap);
		break;
	case(P_HEAR):
		setup_display(PH_GAIN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(P_INFO):	
		break;
	case(P_INVERT):
		setup_display(PI_MEAN,LINEAR,0,0.0,0.0,ap);
		setup_display(PI_TOP,NUMERIC,0,0.0,0.0,ap);
		setup_display(PI_BOT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(P_QUANTISE):	
		break;
	case(P_RANDOMISE):
		setup_display(PR_MXINT,LINEAR,SUBRANGE,ap->lo[PR_MXINT],12.0,ap);
		setup_display(PR_TSTEP,LINEAR,SUBRANGE,ap->lo[PR_TSTEP],min(ap->hi[PR_TSTEP],200.0),ap);
		setup_display(PR_SLEW,LINEAR,SUBRANGE,-8,8.0,ap);
		break;
	case(P_SEE):
		setup_display(PSEE_SCF,LINEAR,SUBRANGE,ap->lo[PSEE_SCF],200.0,ap);
		break;
	case(P_SMOOTH):
		setup_display(PS_TFRAME,LINEAR,SUBRANGE,ap->lo[PS_TFRAME],min(ap->hi[PS_TFRAME],200.0),ap);
		setup_display(PS_MEAN,LINEAR,0,0.0,0.0,ap);
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
		setup_display(PV_HWIDTH,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(PV_CURVIT,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(PV_PKRANG,LINEAR,0,0.0,0.0,ap);
		setup_display(PV_FUNBAS,LINEAR,0,0.0,0.0,ap);
		setup_display(PV_OFFSET,LINEAR,0,0.0,0.0,ap);
		break;
	case(VFILT):
		setup_display(PV_HWIDTH,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(PV_CURVIT,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(PV_PKRANG,LINEAR,0,0.0,0.0,ap);
		setup_display(VF_THRESH,LINEAR,0,0.0,0.0,ap);
		break;
	case(P_GEN):
		setup_display(PGEN_SRATE,SRATE,0,0.0,0.0,ap);
		setup_display(PGEN_CHANS_INPUT,POWTWO,0,0.0,0.0,ap);
		setup_display(PGEN_WINOVLP_INPUT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(P_TRANSPOSE):
		setup_display(PT_TVAL,LINEAR,SUBRANGE,-12.0,12.0,ap);
		break;
	case(P_VIBRATO):
		setup_display(PV_FRQ,LINEAR,0,0.0,0.0,ap);
		setup_display(PV_RANG,LINEAR,SUBRANGE,ap->lo[PV_RANG],4.0,ap);
		break;
	case(P_WRITE):
		setup_display(PW_DRED,LINEAR,SUBRANGE,ap->lo[PW_DRED],1.0,ap);
		break;
	case(P_ZEROS):		
		break;
	case(PEAK):
		setup_display(PEAK_CUTOFF,PLOG,0,0.0,0.0,ap);
		setup_display(PEAK_TWINDOW,LINEAR,SUBRANGE,ap->lo[PEAK_TWINDOW],min(ap->hi[PEAK_TWINDOW],1.0),ap);
		setup_display(PEAK_FWINDOW,LINEAR,SUBRANGE,ap->lo[PEAK_FWINDOW],min(ap->hi[PEAK_FWINDOW],200.0),ap);
		break;
	case(PICK):
		setup_display(PICK_FUND,PLOG,0,0.0,0.0,ap);
		setup_display(PICK_LIN,LINEAR,SUBRANGE,ap->lo[PICK_LIN],min(ap->hi[PICK_LIN],200.0),ap);
		setup_display(PICK_CLAR,LINEAR,0,0.0,0.0,ap);
		break;
	case(PITCH):
		setup_display(PICH_RNGE,NUMERIC,0,0.0,0.0,ap);
		setup_display(PICH_VALID,LINEAR,SUBRANGE,ap->lo[PICH_VALID],min(ap->hi[PICH_VALID],8.0),ap);
		setup_display(PICH_SRATIO,NUMERIC,0,0.0,0.0,ap);
		setup_display(PICH_MATCH,NUMERIC,0,0.0,0.0,ap);
		setup_display(PICH_HILM,PLOG,0,0.0,0.0,ap);
		setup_display(PICH_LOLM,PLOG,0,0.0,0.0,ap);
		if(mode==PICH_TO_BRK)
			setup_display(PICH_DATAREDUCE,LINEAR,SUBRANGE,ap->lo[PICH_DATAREDUCE],1.0,ap);
		break;
	case(PLUCK):
		setup_display(PLUK_GAIN,LINEAR,SUBRANGE,ap->lo[PLUK_GAIN],16.0,ap);
		break;
	case(PRINT):
		setup_display(PRNT_STIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(PRNT_WCNT,LINEAR,SUBRANGE,ap->lo[PRNT_WCNT],min(ap->hi[PRNT_WCNT],16.0),ap);
		break;
	case(REPITCH):		
		break;
	case(REPITCHB):
		setup_display(RP_DRED,LINEAR,SUBRANGE,ap->lo[RP_DRED],1.0,ap);
		break;
	case(REPORT):
		setup_display(REPORT_PKCNT,NUMERIC,0,0.0,0.0,ap);
		setup_display(REPORT_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(REPORT_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(REPORT_STABL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(SCAT):
		setup_display(SCAT_CNT,LINEAR,0,0.0,0.0,ap);
		setup_display(SCAT_BLOKSIZE,LINEAR,SUBRANGE,ap->lo[SCAT_BLOKSIZE],min(ap->hi[SCAT_BLOKSIZE],200.0),ap);
		break;
	case(SHIFT):
		setup_display(SHIFT_SHIF,LINEAR,SUBRANGE,max(ap->lo[SHIFT_SHIF],-200.0),min(ap->hi[SHIFT_SHIF],200.0),ap);
		setup_display(SHIFT_FRQ1,PLOG,0,0.0,0.0,ap);
		setup_display(SHIFT_FRQ2,PLOG,0,0.0,0.0,ap);
		break;
	case(SHIFTP):
		setup_display(SHIFTP_FFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(SHIFTP_SHF1,LINEAR,SUBRANGE,-24.0,24.0,ap);
		setup_display(SHIFTP_SHF2,LINEAR,SUBRANGE,-24.0,24.0,ap);
		setup_display(SHIFTP_DEPTH,LINEAR,0,0.0,0.0,ap);
		break;
	case(SHUFFLE):
		setup_display(SHUF_GRPSIZE,LINEAR,SUBRANGE,ap->lo[SHUF_GRPSIZE],16.0,ap);
		break;
	case(SPLIT):		
		break;
	case(SPREAD):
		setup_display(SPREAD_SPRD,LINEAR,0,0.0,0.0,ap);
		break;
	case(STEP):
		setup_display(STEP_STEP,LINEAR,SUBRANGE,ap->lo[STEP_STEP],min(ap->hi[STEP_STEP],1.0),ap);
		break;
	case(STRETCH):
		setup_display(STR_FFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(STR_SHIFT,LOG,SUBRANGE,.25,4.0,ap);
		setup_display(STR_EXP,LOG,SUBRANGE,.25,4.0,ap);
		setup_display(STR_DEPTH,LINEAR,0,0.0,0.0,ap);
		break;
	case(SUM):
		setup_display(SUM_CROSS,LINEAR,0,0.0,0.0,ap);
		break;
	case(SUPR):
		setup_display(SUPR_INDX,LINEAR,0,0.0,0.0,ap);
		break;
	case(S_TRACE):
		setup_display(TRAC_INDX,LINEAR,0,0.0,0.0,ap);
		setup_display(TRAC_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(TRAC_HIFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(TRACK):
		setup_display(TRAK_PICH,PLOG,0,0.0,0.0,ap);
		setup_display(TRAK_RNGE,LINEAR,0,0.0,0.0,ap);
		setup_display(TRAK_VALID,LINEAR,SUBRANGE,ap->lo[TRAK_VALID],min(ap->hi[TRAK_VALID],16.0),ap);
		setup_display(TRAK_SRATIO,LINEAR,0,0.0,0.0,ap);
		setup_display(TRAK_HILM,PLOG,0,0.0,0.0,ap);
		if(mode==TRK_TO_BRK)
			setup_display(TRAK_DATAREDUCE,LINEAR,SUBRANGE,ap->lo[PICH_DATAREDUCE],1.0,ap);
		break;
	case(TRNSF):
		setup_display(TRNSF_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(TRNSF_LOFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(TRNSP):
		setup_display(TRNSP_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(TRNSP_LOFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(TSTRETCH):
		setup_display(TSTR_STRETCH,LOG,SUBRANGE,0.0625,16.0,ap);
		break;
	case(TUNE):
		setup_display(TUNE_FOC,LINEAR,0,0.0,0.0,ap);
		setup_display(TUNE_CLAR,LINEAR,0,0.0,0.0,ap);
		setup_display(TUNE_INDX,LINEAR,0,0.0,0.0,ap);
		setup_display(TUNE_BFRQ,PLOG,0,0.0,0.0,ap);
		break;
	case(VOCODE):
		setup_display(VOCO_LOF,PLOG,0,0.0,0.0,ap);
		setup_display(VOCO_HIF,PLOG,0,0.0,0.0,ap);
		setup_display(VOCO_GAIN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(WARP):
		setup_display(WARP_PRNG,LINEAR,SUBRANGE,ap->lo[WARP_PRNG],12.0,ap);
		setup_display(WARP_TRNG,LINEAR,SUBRANGE,ap->lo[WARP_TRNG],min(ap->hi[WARP_TRNG],200.0),ap);
		setup_display(WARP_SRNG,LINEAR,SUBRANGE,ap->lo[WARP_SRNG],min(ap->hi[WARP_SRNG],200.0),ap);
		break;
	case(WAVER):
		setup_display(WAVER_VIB,LINEAR,SUBRANGE,ap->lo[WAVER_VIB],min(ap->hi[WAVER_VIB],64.0),ap);
		setup_display(WAVER_STR,LINEAR,SUBRANGE,ap->lo[WAVER_STR],min(ap->hi[WAVER_STR],4.0),ap);
		setup_display(WAVER_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(WAVER_EXP,LOG,SUBRANGE,0.25,4.0,ap);
		break;
	case(WEAVE):	
		break;
	case(WINDOWCNT):
		break;
	case(LIMIT):
		setup_display(LIMIT_THRESH,LINEAR,0,0.0,0.0,ap);
		break;

 		/******************************* GROUCHO *******************************/
		/******************************* GROUCHO *******************************/
		/******************************* GROUCHO *******************************/

	case(DISTORT):
		setup_display(DISTORT_POWFAC,LOG,SUBRANGE,0.25,4.0,ap);
		break;
	case(DISTORT_ENV):
		setup_display(DISTORTE_CYCLECNT,LINEAR,0,0.0,0.0,ap);
		setup_display(DISTORTE_TROF,LINEAR,0,0.0,0.0,ap);	
		setup_display(DISTORTE_EXPON,LOG,SUBRANGE,0.25,4.0,ap);	
		break;
	case(DISTORT_AVG):
		setup_display(DISTORTA_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTORTA_CYCLECNT],64.0,ap);
		setup_display(DISTORTA_MAXLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(DISTORTA_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTORTA_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_OMT):
		setup_display(DISTORTO_OMIT,LINEAR,SUBRANGE,ap->lo[DISTORTO_OMIT],63.0,ap);
		setup_display(DISTORTO_KEEP,LINEAR,SUBRANGE,ap->lo[DISTORTO_KEEP],64.0,ap);
		break;
	case(DISTORT_MLT):
	case(DISTORT_DIV):
		setup_display(DISTORTM_FACTOR,LINEAR,0,0.0,0.0,ap);
		break;
	case(DISTORT_HRM):
		setup_display(DISTORTH_PRESCALE,LINEAR,SUBRANGE,0.0625,16.0,ap);
		break;
	case(DISTORT_FRC):
		setup_display(DISTORTF_SCALE,LINEAR,SUBRANGE,ap->lo[DISTORTF_SCALE],min(ap->hi[DISTORTF_SCALE],64.0),ap);
		setup_display(DISTORTF_AMPFACT,LINEAR,SUBRANGE,0.0625,1.0,ap);
		setup_display(DISTORTF_PRESCALE,LINEAR,SUBRANGE,0.0625,1.0,ap);
		break;
	case(DISTORT_REV):
		setup_display(DISTORTR_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTORTR_CYCLECNT],64.0,ap);
		break;
	case(DISTORT_SHUF):
		setup_display(DISTORTS_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTORTS_CYCLECNT],64.0,ap);
		setup_display(DISTORTS_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTORTS_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_RPTFL):
		setup_display(DISTRPT_CYCLIM,LOGNUMERIC,0,0.0,0.0,ap);
		/* fall thro */
	case(DISTORT_RPT):
	case(DISTORT_RPT2):
		setup_display(DISTRPT_MULTIPLY,LINEAR,SUBRANGE,ap->lo[DISTRPT_MULTIPLY],64.0,ap);
		setup_display(DISTRPT_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTRPT_CYCLECNT],64.0,ap);
		setup_display(DISTRPT_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTRPT_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_INTP):
		setup_display(DISTINTP_MULTIPLY,LINEAR,SUBRANGE,ap->lo[DISTINTP_MULTIPLY],64.0,ap);
		setup_display(DISTINTP_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTINTP_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_DEL):
		setup_display(DISTDEL_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTDEL_CYCLECNT],64.0,ap);
		setup_display(DISTDEL_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTDEL_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_RPL):
		setup_display(DISTRPL_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTRPL_CYCLECNT],64.0,ap);
		setup_display(DISTRPL_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTRPL_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_TEL):
		setup_display(DISTTEL_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTTEL_CYCLECNT],64.0,ap);
		setup_display(DISTTEL_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTTEL_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_FLT):
		setup_display(DISTFLT_LOFRQ_CYCLELEN,PLOG,0,0.0,0.0,ap);
		setup_display(DISTFLT_HIFRQ_CYCLELEN,PLOG,0,0.0,0.0,ap);
		setup_display(DISTFLT_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTFLT_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_INT):
		break;
	case(DISTORT_CYCLECNT):
		break;
	case(DISTORT_PCH):
		setup_display(DISTPCH_OCTVAR,LOG,SUBRANGE,ap->lo[DISTPCH_OCTVAR],8.0,ap);
		setup_display(DISTPCH_CYCLECNT,LINEAR,SUBRANGE,ap->lo[DISTPCH_CYCLECNT],64.0,ap);
		setup_display(DISTPCH_SKIPCNT,LINEAR,SUBRANGE,ap->lo[DISTPCH_SKIPCNT],64.0,ap);
		break;
	case(DISTORT_OVERLOAD):
		setup_display(DISTORTER_MULT,LINEAR,0,0.0,0.0,ap);
		setup_display(DISTORTER_DEPTH,LINEAR,0,0.0,0.0,ap);
		if(mode==OVER_SINE)
			setup_display(DISTORTER_FRQ,LOG,0,0.0,0.0,ap);
		break;
	case(DISTORT_PULSED):
		setup_display(PULSE_STARTTIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(PULSE_DUR,NUMERIC,0,0.0,0.0,ap);
		setup_display(PULSE_FRQ,PLOG,0,0.0,0.0,ap);
		setup_display(PULSE_FRQRAND,NUMERIC,0,0.0,0.0,ap);
		setup_display(PULSE_TIMERAND,NUMERIC,0,0.0,0.0,ap);
		setup_display(PULSE_SHAPERAND,NUMERIC,0,0.0,0.0,ap);
		if(mode==PULSE_SYN)
			setup_display(PULSE_WAVETIME,LOGNUMERIC,0,0.0,0.0,ap);
		else if(mode==PULSE_SYNI)
			setup_display(PULSE_WAVETIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(PULSE_TRANSPOS,LINEAR,0,0.0,0.0,ap);
		setup_display(PULSE_PITCHRAND,NUMERIC,0,0.0,0.0,ap);
		break;

	case(ZIGZAG):
		setup_display(ZIGZAG_START,NUMERIC,0,0.0,0.0,ap);
		setup_display(ZIGZAG_END,NUMERIC,0,0.0,0.0,ap);
		setup_display(ZIGZAG_DUR,NUMERIC,0,0.0,0.0,ap);
		setup_display(ZIGZAG_MIN,LINEAR,SUBRANGE,ap->lo[ZIGZAG_MIN],min(ap->hi[ZIGZAG_MIN],0.25),ap);
		setup_display(ZIGZAG_SPLEN,NUMERIC,0,0.0,0.0,ap);
		if(mode==ZIGZAG_SELF) {
			setup_display(ZIGZAG_MAX,LINEAR,SUBRANGE,ap->lo[ZIGZAG_MAX],min(ap->hi[ZIGZAG_MAX],2.0),ap);
			setup_display(ZIGZAG_RSEED,LINEAR,SUBRANGE,ap->lo[ZIGZAG_RSEED],64.0,ap);
		}
		break;
	case(LOOP):
		setup_display(LOOP_OUTDUR,LINEAR,SUBRANGE,0,60.0,ap);
		setup_display(LOOP_REPETS,LINEAR,SUBRANGE,ap->lo[LOOP_REPETS],256.0,ap);
		setup_display(LOOP_START,NUMERIC,0,0.0,0.0,ap);
		setup_display(LOOP_LEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(LOOP_STEP,LINEAR,SUBRANGE,ap->lo[LOOP_STEP],min(ap->hi[LOOP_STEP],1000.0),ap);
		setup_display(LOOP_SPLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(LOOP_SRCHF,LINEAR,SUBRANGE,ap->lo[LOOP_SRCHF],min(ap->hi[LOOP_SRCHF],1000.0),ap);
		break;
	case(SCRAMBLE):
		switch(mode) {
		case(SCRAMBLE_RAND):
			setup_display(SCRAMBLE_MIN,NUMERIC,0,0.0,0.0,ap);
			setup_display(SCRAMBLE_MAX,NUMERIC,0,0.0,0.0,ap);
			break;
		case(SCRAMBLE_SHRED):
			setup_display(SCRAMBLE_LEN,NUMERIC,0,0.0,0.0,ap);
			setup_display(SCRAMBLE_SCAT,LINEAR,SUBRANGE,ap->lo[SCRAMBLE_SCAT],min(ap->hi[SCRAMBLE_SCAT],duration*2.0),ap);
			break;
		}
		setup_display(SCRAMBLE_DUR,LINEAR,SUBRANGE,0,60.0,ap);
		setup_display(SCRAMBLE_SPLEN,LINEAR,SUBRANGE,5.0,200.0,ap);
		setup_display(SCRAMBLE_SEED,LINEAR,SUBRANGE,ap->lo[SCRAMBLE_SEED],64.0,ap);
		break;
	case(ITERATE):
		switch(mode) {
		case(ITERATE_DUR):     
			setup_display(ITER_DUR,LINEAR,SUBRANGE,0,60.0,ap); 	
			break;
		case(ITERATE_REPEATS): 
			setup_display(ITER_REPEATS,LINEAR,SUBRANGE,ap->lo[ITER_REPEATS],64.0,ap);
			break;
		}
		setup_display(ITER_DELAY,LINEAR,SUBRANGE,ap->lo[ITER_DELAY],min(ap->hi[ITER_DELAY],duration),ap);
		setup_display(ITER_RANDOM,LINEAR,0,0.0,0.0,ap);
		setup_display(ITER_PSCAT,LINEAR,0,0.0,0.0,ap);		
		setup_display(ITER_ASCAT,LINEAR,0,0.0,0.0,ap);
		setup_display(ITER_FADE,LINEAR,0,0.0,0.0,ap);
		setup_display(ITER_RSEED,LINEAR,SUBRANGE,ap->lo[ITER_RSEED],min(ap->hi[ITER_RSEED],64.0),ap);
		setup_display(ITER_GAIN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(ITERATE_EXTEND):
		switch(mode) {
		case(ITERATE_DUR):     
			setup_display(ITER_DUR,LINEAR,SUBRANGE,0,60.0,ap); 	
			break;
		case(ITERATE_REPEATS): 
			setup_display(ITER_REPEATS,LINEAR,SUBRANGE,ap->lo[ITER_REPEATS],64.0,ap);
			break;
		}
		setup_display(ITER_DELAY,LINEAR,SUBRANGE,ap->lo[ITER_DELAY],min(ap->hi[ITER_DELAY],duration),ap);
		setup_display(ITER_RANDOM,LINEAR,0,0.0,0.0,ap);
		setup_display(ITER_PSCAT,LINEAR,0,0.0,0.0,ap);		
		setup_display(ITER_ASCAT,LINEAR,0,0.0,0.0,ap);
		setup_display(CHUNKSTART,NUMERIC,0,0.0,0.0,ap);
		setup_display(CHUNKEND,NUMERIC,0,0.0,0.0,ap);
		setup_display(ITER_LGAIN,NUMERIC,0,0.0,0.0,ap);
		setup_display(ITER_RRSEED,LINEAR,SUBRANGE,ap->lo[ITER_RRSEED],min(ap->hi[ITER_RRSEED],64.0),ap);
		break;
	case(DRUNKWALK):
		setup_display(DRNK_TOTALDUR,LINEAR,SUBRANGE,0.0,60.0,ap);
		setup_display(DRNK_LOCUS,LINEAR,0,0.0,0.0,ap);
		setup_display(DRNK_AMBITUS,LINEAR,SUBRANGE,ap->lo[DRNK_AMBITUS],min(ap->hi[DRNK_AMBITUS],1.0),ap);
		setup_display(DRNK_GSTEP,LINEAR,SUBRANGE,ap->lo[DRNK_GSTEP],min(ap->hi[DRNK_GSTEP],1.0),ap);
		setup_display(DRNK_CLOKTIK,LINEAR,SUBRANGE,ap->lo[DRNK_CLOKTIK],min(ap->hi[DRNK_CLOKTIK],1.0),ap);
		setup_display(DRNK_MIN_DRNKTIK,LINEAR,SUBRANGE,ap->lo[DRNK_MIN_DRNKTIK],64.0,ap);
		setup_display(DRNK_MAX_DRNKTIK,LINEAR,SUBRANGE,ap->lo[DRNK_MAX_DRNKTIK],64.0,ap);

		setup_display(DRNK_SPLICELEN,LINEAR,SUBRANGE,ap->lo[DRNK_SPLICELEN],min(ap->hi[DRNK_SPLICELEN],500.0),ap);
		setup_display(DRNK_CLOKRND,LINEAR,0,0.0,0.0,ap);
		setup_display(DRNK_OVERLAP,LINEAR,0,0.0,0.0,ap);
		setup_display(DRNK_RSEED,LINEAR,SUBRANGE,ap->lo[DRNK_RSEED],64.0,ap);
		if(mode==HAS_SOBER_MOMENTS) {
			setup_display(DRNK_MIN_PAUS,LINEAR,SUBRANGE,ap->lo[DRNK_MIN_PAUS],min(ap->hi[DRNK_MIN_PAUS],2.0),ap);
			setup_display(DRNK_MAX_PAUS,LINEAR,SUBRANGE,ap->lo[DRNK_MAX_PAUS],min(ap->hi[DRNK_MAX_PAUS],2.0),ap);
		}
		break;

	case(SIMPLE_TEX):	case(TIMED):    case(GROUPS):	case(TGROUPS):
	case(DECORATED):	case(PREDECOR): case(POSTDECOR):
	case(ORNATE):   	case(PREORNATE):case(POSTORNATE):
	case(MOTIFS):   	case(MOTIFSIN):	case(TMOTIFS): 	case(TMOTIFSIN):
		setup_display(TEXTURE_DUR,LINEAR,SUBRANGE,0.0,60.0,ap);

		switch(process) {
		case(SIMPLE_TEX):	case(GROUPS):	case(MOTIFS):	case(MOTIFSIN):
			setup_display(TEXTURE_PACK,LOG,SUBRANGE,0.002,2.0,ap);
			break;
		case(TIMED):		case(TGROUPS):		case(TMOTIFS):		case(TMOTIFSIN):
		case(DECORATED):	case(PREDECOR):		case(POSTDECOR):	
		case(ORNATE):		case(PREORNATE):	case(POSTORNATE):
			setup_display(TEXTURE_SKIP,LINEAR,SUBRANGE,0.0,10.0,ap);
			break;
		}
		setup_display(TEXTURE_SCAT,LINEAR,SUBRANGE,ap->lo[TEXTURE_SCAT],2.0,ap);
		setup_display(TEXTURE_TGRID,LINEAR,SUBRANGE,ap->lo[TEXTURE_TGRID],100.0,ap);
		setup_display(TEXTURE_INSLO,LINEAR,SUBRANGE,ap->lo[TEXTURE_INSLO],64.0,ap);
		setup_display(TEXTURE_INSHI,LINEAR,SUBRANGE,ap->lo[TEXTURE_INSHI],64.0,ap);
		setup_display(TEXTURE_MAXAMP,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_MINAMP,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_MAXDUR,LINEAR,SUBRANGE,ap->lo[TEXTURE_MAXDUR],min(ap->hi[TEXTURE_MAXDUR],2.0),ap);
		setup_display(TEXTURE_MINDUR,LINEAR,SUBRANGE,ap->lo[TEXTURE_MINDUR],min(ap->hi[TEXTURE_MINDUR],2.0),ap);
		setup_display(TEXTURE_MAXPICH,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_MINPICH,LINEAR,0,0.0,0.0,ap);
		if(process == SIMPLE_TEX)
			setup_display(TEX_PHGRID,LINEAR,0,0.0,0.0,ap);
		else
			setup_display(TEX_PHGRID,LINEAR,SUBRANGE,ap->lo[TEX_PHGRID],min(ap->hi[TEX_PHGRID],100.0),ap);
		setup_display(TEX_GPSPACE,LINEAR,0,0.0,0.0,ap);
		setup_display(TEX_GRPSPRANGE,LINEAR,0,0.0,0.0,ap);
		setup_display(TEX_AMPRISE,LINEAR,0,0.0,0.0,ap);
		setup_display(TEX_AMPCONT,LINEAR,0,0.0,0.0,ap);
		setup_display(TEX_GPSIZELO,LINEAR,SUBRANGE,ap->lo[TEX_GPSIZELO],min(ap->hi[TEX_GPSIZELO],64.0),ap);
		setup_display(TEX_GPSIZEHI,LINEAR,SUBRANGE,ap->lo[TEX_GPSIZEHI],min(ap->hi[TEX_GPSIZEHI],64.0),ap);
		setup_display(TEX_GPPACKLO,LINEAR,SUBRANGE,20,1000,ap);
		setup_display(TEX_GPPACKHI,LINEAR,SUBRANGE,20,1000,ap);
		if(mode==TEX_NEUTRAL) { /* midipitches */
			setup_display(TEX_GPRANGLO,LINEAR,SUBRANGE,ap->lo[TEX_GPRANGLO],min(ap->hi[TEX_GPRANGLO],MAXMIDI),ap);
			setup_display(TEX_GPRANGHI,LINEAR,SUBRANGE,ap->lo[TEX_GPRANGHI],min(ap->hi[TEX_GPRANGHI],MAXMIDI),ap);
		} else { /* notes of hfield */
			setup_display(TEX_GPRANGLO,LINEAR,SUBRANGE,ap->lo[TEX_GPRANGLO],min(ap->hi[TEX_GPRANGLO],64.0),ap);
			setup_display(TEX_GPRANGHI,LINEAR,SUBRANGE,ap->lo[TEX_GPRANGHI],min(ap->hi[TEX_GPRANGHI],64.0),ap);
		}
		setup_display(TEX_MULTLO,LOG,SUBRANGE,0.125,8.0,ap);
		setup_display(TEX_MULTHI,LOG,SUBRANGE,0.125,8.0,ap);
		setup_display(TEX_DECPCENTRE,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_ATTEN,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_POS,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_SPRD,LINEAR,0,0.0,0.0,ap);
		setup_display(TEXTURE_SEED,LINEAR,SUBRANGE,ap->lo[TEXTURE_SEED],64.0,ap);
		break;

	case(GRAIN_COUNT):		case(GRAIN_OMIT):		case(GRAIN_DUPLICATE):
	case(GRAIN_REORDER):	case(GRAIN_REPITCH):	case(GRAIN_RERHYTHM):
	case(GRAIN_REMOTIF):	case(GRAIN_TIMEWARP):	case(GRAIN_POSITION):
	case(GRAIN_ALIGN):		case(GRAIN_GET):		case(GRAIN_REVERSE):
		setup_display(GR_BLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(GR_GATE,LINEAR,0,0.0,0.0,ap);
		setup_display(GR_MINTIME,LINEAR,SUBRANGE,ap->lo[GR_MINTIME],min(ap->hi[GR_MINTIME],0.2),ap);
		setup_display(GR_WINSIZE,LINEAR,SUBRANGE,ap->lo[GR_WINSIZE],min(ap->hi[GR_WINSIZE],1000.0),ap);			
		switch(process) {
		case(GRAIN_OMIT):	    
			setup_display(GR_KEEP,LINEAR,0,0.0,0.0,ap);
			setup_display(GR_OUT_OF,LINEAR,0,0.0,0.0,ap);	
			break;
		case(GRAIN_DUPLICATE):  
			setup_display(GR_DUPLS,LINEAR,SUBRANGE,ap->lo[GR_DUPLS],64.0,ap);
			break;
		case(GRAIN_TIMEWARP):
			setup_display(GR_TSTRETCH,LOG,SUBRANGE,0.25,4.0,ap);
			break;
		case(GRAIN_POSITION):
			setup_display(GR_OFFSET,NUMERIC,0,0.0,0.0,ap);
			break;
		case(GRAIN_ALIGN):
			setup_display(GR_OFFSET,NUMERIC,0,0.0,0.0,ap);
			setup_display(GR_GATE2,LINEAR,0,0.0,0.0,ap);
			break;
		}
		break;

	case(ENV_CREATE):
		setup_display(ENV_WSIZE,LOG,SUBRANGE,ap->lo[ENV_WSIZE],min(ap->hi[ENV_WSIZE],1000.0),ap);
		break;
	case(ENV_BRKTOENV):
	case(ENV_DBBRKTOENV):
	case(ENV_IMPOSE):
	case(ENV_PROPOR):
	case(ENV_REPLACE):
		setup_display(ENV_WSIZE,LINEAR,SUBRANGE,ap->lo[ENV_WSIZE],min(ap->hi[ENV_WSIZE],1000.0),ap);
		break;
	case(ENV_EXTRACT):
		setup_display(ENV_WSIZE,LINEAR,SUBRANGE,ap->lo[ENV_WSIZE],min(ap->hi[ENV_WSIZE],1000.0),ap);
		if(mode==ENV_BRKFILE_OUT)
			setup_display(ENV_DATAREDUCE,LINEAR,0,0.0,0.0,ap);
		break;
	case(ENV_ENVTOBRK):
	case(ENV_ENVTODBBRK):
		setup_display(ENV_DATAREDUCE,NUMERIC,0,0.0,0.0,ap);
		break;
	case(ENV_WARPING):
	case(ENV_REPLOTTING):
		setup_display(ENV_WSIZE,LINEAR,SUBRANGE,ap->lo[ENV_WSIZE],min(ap->hi[ENV_WSIZE],1000.0),ap);
		/* fall thro */
	case(ENV_RESHAPING):
		switch(mode) {
		case(ENV_NORMALISE):
		case(ENV_REVERSE):	
		case(ENV_CEILING):	
			break;
		case(ENV_DUCKED):	
			setup_display(ENV_GATE,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_THRESHOLD,LINEAR,0,0.0,0.0,ap);
			break;
		case(ENV_EXAGGERATING):
			setup_display(ENV_EXAG,LOG,SUBRANGE,0.125,8.0,ap);
			break;
		case(ENV_ATTENUATING):
			setup_display(ENV_ATTEN,LINEAR,0,0.0,0.0,ap);
			break;
		case(ENV_LIFTING):				
			setup_display(ENV_LIFT,LINEAR,0,0.0,0.0,ap);
			break;
		case(ENV_FLATTENING):
			setup_display(ENV_FLATN,LINEAR,SUBRANGE,ap->lo[ENV_FLATN],16.0,ap);
			break;
		case(ENV_TSTRETCHING):
			setup_display(ENV_TSTRETCH,LOG,SUBRANGE,0.125,8.0,ap);
			break;
		case(ENV_GATING):
			setup_display(ENV_GATE,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_SMOOTH,LINEAR,SUBRANGE,ap->lo[ENV_SMOOTH],16.0,ap);
			break;
		case(ENV_INVERTING):
			setup_display(ENV_GATE,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_MIRROR,LINEAR,0,0.0,0.0,ap);
			break;
		case(ENV_LIMITING):
			setup_display(ENV_LIMIT,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_THRESHOLD,LINEAR,0,0.0,0.0,ap);
			break;
		case(ENV_CORRUGATING):
			setup_display(ENV_TROFDEL,LINEAR,SUBRANGE,ap->lo[ENV_TROFDEL],8.0,ap);
			/* fall thro */
		case(ENV_PEAKCNT):
			setup_display(ENV_PKSRCHWIDTH,LINEAR,SUBRANGE,ap->lo[ENV_PKSRCHWIDTH],64.0,ap);
			break;
		case(ENV_EXPANDING):
			setup_display(ENV_GATE,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_THRESHOLD,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_SMOOTH,LINEAR,SUBRANGE,0.0,64.0,ap);
			break;
		case(ENV_TRIGGERING):
			setup_display(ENV_GATE,LINEAR,0,0.0,0.0,ap);
			setup_display(ENV_TRIGRISE,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_TRIGDUR,LINEAR,SUBRANGE,ap->lo[ENV_TRIGDUR],ENV_DEFAULT_TRIGDUR * 2.0,ap);
			break;
		}				  
		if(process==ENV_REPLOTTING)
			setup_display(ENV_DATAREDUCE,LINEAR,0,0.0,0.0,ap);
		break;
	case(ENV_DOVETAILING):
		switch(mode) {
		case(DOVE):
			setup_display(ENV_STARTTRIM,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_ENDTRIM,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_STARTTYPE,CHECKBUTTON,0,0.0,0.0,ap);
			setup_display(ENV_ENDTYPE,CHECKBUTTON,0,0.0,0.0,ap);
			setup_display(ENV_TIMETYPE,TIMETYPE,0,0.0,0.0,ap);
			break;
		case(DOVEDBL):
			setup_display(ENV_STARTTRIM,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_ENDTRIM,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_TIMETYPE,TIMETYPE,0,0.0,0.0,ap);
			break;
		}
		break;
	case(ENV_CURTAILING):
		switch(mode) {
		case(ENV_START_AND_END):
		case(ENV_START_AND_DUR):
		case(ENV_START_ONLY):
			setup_display(ENV_STARTTIME,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_ENDTIME,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_ENVTYPE,CHECKBUTTON,0,0.0,0.0,ap);
			setup_display(ENV_TIMETYPE,TIMETYPE,0,0.0,0.0,ap);
			break;
		case(ENV_START_AND_END_D):
		case(ENV_START_AND_DUR_D):
		case(ENV_START_ONLY_D):
			setup_display(ENV_STARTTIME,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_ENDTIME,NUMERIC,0,0.0,0.0,ap);
			setup_display(ENV_TIMETYPE,TIMETYPE,0,0.0,0.0,ap);
			break;
		}
		break;
	case(ENV_SWELL):
		setup_display(ENV_PEAKTIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(ENV_ENVTYPE,CHECKBUTTON,0,0.0,0.0,ap);
		break;
	case(ENV_ATTACK):
		switch(mode) {
		case(ENV_ATK_GATED):		
			setup_display(ENV_ATK_GATE,NUMERIC,0,0.0,0.0,ap);
			break;
		case(ENV_ATK_TIMED):		
		case(ENV_ATK_XTIME):		
			setup_display(ENV_ATK_ATTIME,NUMERIC,0,0.0,0.0,ap);
			break;
		case(ENV_ATK_ATMAX):		
			break;
		}
		setup_display(ENV_ATK_GAIN,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(ENV_ATK_ONSET,LINEAR,SUBRANGE,ap->lo[ENV_ATK_ONSET],200.0,ap);
		setup_display(ENV_ATK_TAIL,LINEAR,SUBRANGE,ap->lo[ENV_ATK_TAIL],min(ap->hi[ENV_ATK_TAIL],200.0),ap);
		setup_display(ENV_ATK_ENVTYPE,CHECKBUTTON,0,0.0,0.0,ap);
		break;
	case(ENV_PLUCK):
		setup_display(ENV_PLK_ENDSAMP,NUMERIC,0,0.0,0.0,ap);
		setup_display(ENV_PLK_WAVELEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(ENV_PLK_CYCLEN,LINEAR,SUBRANGE,ap->lo[ENV_PLK_CYCLEN],64.0,ap);
		setup_display(ENV_PLK_DECAY,NUMERIC,0,0.0,0.0,ap);
		break;
	case(ENV_TREMOL):
		setup_display(ENV_TREM_FRQ,LINEAR,SUBRANGE,ap->lo[ENV_TREM_FRQ],64.0,ap);
		setup_display(ENV_TREM_DEPTH,LINEAR,0,0.0,0.0,ap);
		setup_display(ENV_TREM_AMP,LINEAR,0,0.0,0.0,ap);
		break;
	case(ENV_DBBRKTOBRK):
	case(ENV_BRKTODBBRK):
		break;

	case(MIX):
		setup_display(MIX_ATTEN,NUMERIC,0,0.0,0.0,ap);
		/* fall thro */
	case(MIXMAX):
		setup_display(MIX_START,NUMERIC,0,0.0,0.0,ap);
		setup_display(MIX_END,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MIXTWO):
		setup_display(MIX_STAGGER,NUMERIC,0,0.0,0.0,ap);
		setup_display(MIX_SKIP,LINEAR,SUBRANGE,0.0,duration * 8.0,ap);
		setup_display(MIX_SKEW,LOG,SUBRANGE,0.25,8.0,ap);
		setup_display(MIX_DURA,NUMERIC,0,0.0,0.0,ap);
		setup_display(MIX_STTA,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MIXMANY):
		break;
	case(MIXBALANCE):
		setup_display(MIX_STAGGER,LINEAR,0,0.0,0.0,ap);
		setup_display(MIX_SKIP,NUMERIC,0,0.0,0.0,ap);
		setup_display(MIX_SKEW,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MIXCROSS):
		setup_display(MCR_STAGGER,NUMERIC,0,0.0,0.0,ap);
		setup_display(MCR_BEGIN,NUMERIC,0,0.0,0.0,ap);
		setup_display(MCR_END,LINEAR,SUBRANGE,ap->lo[MCR_END],min(ap->hi[MCR_END],duration * 2.0),ap);
		if(mode==MCCOS)
			setup_display(MCR_POWFAC,LOG,0,0.0,0.0,ap);
		break;
	case(MIXINBETWEEN):
		switch(mode) {
		case(INBI_COUNT):
			setup_display(INBETW,LINEAR,SUBRANGE,ap->lo[INBETW],32.0,ap);
			break;
		case(INBI_RATIO):
			break;
		}				  
		break;
	case(MIXTEST):
	case(MIXFORMAT):
	case(MIXDUMMY):
	case(MIXINTERL):
	case(MIXSYNC):
	case(MIX_ON_GRID):
	case(ADDTOMIX):
		break;
	case(AUTOMIX):
		setup_display(0,LINEAR,SUBRANGE,0.0,2.0,ap);
		break;
	case(MIX_PAN):
		setup_display(PAN_PAN,LINEAR,SUBRANGE,-1.0,1.0,ap);
		break;
	case(MIX_AT_STEP):
		setup_display(MIX_STEP,NUMERIC,0,0.0,0.0,ap);
		break;
	case(SHUDDER):
		setup_display(SHUD_STARTTIME,NUMERIC,0,0.0,0.0,ap);
		setup_display(SHUD_FRQ,LOG,0,0.0,0.0,ap);
		setup_display(SHUD_SCAT,LINEAR,0,0.0,0.0,ap);
		setup_display(SHUD_SPREAD,LINEAR,0,0.0,0.0,ap);
		setup_display(SHUD_MINDEPTH,LINEAR,0,0.0,0.0,ap);
		setup_display(SHUD_MAXDEPTH,LINEAR,0,0.0,0.0,ap);
		setup_display(SHUD_MINWIDTH,LOG,0,0.0,0.0,ap);
		setup_display(SHUD_MAXWIDTH,LOG,0,0.0,0.0,ap);
		break;
	case(MIXSYNCATT):
		setup_display(MSY_WFAC,TWOFAC,0,0.0,0.0,ap);
		break;
	case(MIXGAIN):	   	   
		setup_display(MIX_GAIN,NUMERIC,0,0.0,0.0,ap);
		setup_display(MSH_STARTLINE,NUMERIC,0,0.0,0.0,ap);
		setup_display(MSH_ENDLINE,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MIXTWARP):
		switch(mode) {
		case(MTW_REVERSE_T):  case(MTW_REVERSE_NT):
		case(MTW_FREEZE_T):	  case(MTW_FREEZE_NT):
		case(MTW_TIMESORT):
			break;
		case(MTW_SCATTER):  
			setup_display(MTW_PARAM,NUMERIC,0,0.0,0.0,ap);
			break;
		case(MTW_DOMINO):  
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,-4.0,4.0,ap);
			break;
		case(MTW_ADD_TO_TG):  
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,-2.0,16.0,ap);
			break;
		case(MTW_CREATE_TG_1): 	
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],64.0,ap);
			break;
		case(MTW_CREATE_TG_2):  
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],16.0,ap);
			break;
		case(MTW_CREATE_TG_3): 	
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],1.0,ap);
			break;
		case(MTW_CREATE_TG_4):
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],1.1,ap);
			break;
		case(MTW_ENLARGE_TG_1): 
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],64.0,ap);
			break;
		case(MTW_ENLARGE_TG_2): 
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],1.0,ap);
			break;
		case(MTW_ENLARGE_TG_3): 
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],1.0,ap);
			break;
		case(MTW_ENLARGE_TG_4): 
			setup_display(MTW_PARAM,LINEAR,SUBRANGE,ap->lo[MTW_PARAM],1.1,ap);
			break;
		}
		if(mode!=MTW_TIMESORT) {
			setup_display(MSH_STARTLINE,NUMERIC,0,0.0,0.0,ap);
			setup_display(MSH_ENDLINE,NUMERIC,0,0.0,0.0,ap);
		}
		break;
	case(MIXSWARP):	   	   
		switch(mode) {
		case(MSW_TWISTALL):
			break;
		case(MSW_TWISTONE):
			setup_display(MSW_TWLINE,NUMERIC,0,0.0,0.0,ap);
			break;
		case(MSW_NARROWED):					
			setup_display(MSW_NARROWING,NUMERIC,0,0.0,0.0,ap);
			break;
		case(MSW_LEFTWARDS): 
		case(MSW_RIGHTWARDS): 
		case(MSW_RANDOM): 
		case(MSW_RANDOM_ALT):
			setup_display(MSW_POS1,NUMERIC,0,0.0,0.0,ap);
 			setup_display(MSW_POS2,NUMERIC,0,0.0,0.0,ap);
			break;
		case(MSW_FIXED):
			setup_display(MSW_POS1,NUMERIC,0,0.0,0.0,ap);
			break;
		}
		if(mode!=MSW_TWISTALL && mode!=MSW_TWISTONE) {
			setup_display(MSH_STARTLINE,NUMERIC,0,0.0,0.0,ap);
			setup_display(MSH_ENDLINE,NUMERIC,0,0.0,0.0,ap);
		}
		break;
	case(MIXSHUFL):	   	   
		setup_display(MSH_STARTLINE,NUMERIC,0,0.0,0.0,ap);
		setup_display(MSH_ENDLINE,NUMERIC,0,0.0,0.0,ap);
		break;

	case(EQ):
		switch(mode) {
		case(FLT_PEAKING):
			setup_display(FLT_BW,LOG,0,0.0,0.0,ap);
			/* fall thro */
		default:
			setup_display(FLT_BOOST,NUMERIC,0,0.0,0.0,ap);
			setup_display(FLT_ONEFRQ,PLOG,0,0.0,0.0,ap);
			setup_display(FLT_PRESCALE,LOG,0,0.0,0.0,ap);
			setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
			break;
		}
		break;
	case(LPHP):
		setup_display(FLT_GAIN,NUMERIC,0,0.0,0.0,ap);
		setup_display(FLT_PRESCALE,LOG,0,0.0,0.0,ap);
		switch(mode) {
		case(FLT_HZ):
			setup_display(FLT_PASSFRQ,PLOG,0,0.0,0.0,ap);
			setup_display(FLT_STOPFRQ,PLOG,0,0.0,0.0,ap);
			break;
		case(FLT_MIDI):
			setup_display(FLT_PASSFRQ,LINEAR,0,0.0,0.0,ap);
			setup_display(FLT_STOPFRQ,LINEAR,0,0.0,0.0,ap);
			break;
		}
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FSTATVAR):
		setup_display(FLT_Q,LINEAR,SUBRANGE,0.01,1.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FLT_ONEFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTBANKN):		
		setup_display(FLT_Q,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		/* fall thro */
	case(FLTBANKC):
		setup_display(FLT_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FLT_HIFRQ,PLOG,0,0.0,0.0,ap);
		switch(mode) {
		case(FLT_LINOFFSET):
			setup_display(FLT_OFFSET,LINEAR,SUBRANGE,-200.0,200.0,ap);
			break;
		case(FLT_EQUALSPAN):
			setup_display(FLT_INCOUNT,LINEAR,SUBRANGE,ap->lo[FLT_INCOUNT],64.0,ap);
			break;
		case(FLT_EQUALINT):
			setup_display(FLT_INTSIZE,LINEAR,SUBRANGE,ap->lo[FLT_INTSIZE],12.0,ap);
			break;
		}
		setup_display(FLT_RANDFACT,NUMERIC,0,0.0,0.0,ap);
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTBANKU):		
		setup_display(FLT_Q,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTBANKV):
		setup_display(FLT_Q,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FLT_HARMCNT,LINEAR,SUBRANGE,ap->lo[FLT_HARMCNT],64.0,ap);
		setup_display(FLT_ROLLOFF,LINEAR,0,0.0,0.0,ap);
		setup_display(FILT_TAILV,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTBANKV2):
		setup_display(FLT_Q,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FILT_TAILV,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTSWEEP):
		setup_display(FLT_Q,LINEAR,SUBRANGE,.01,1.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FLT_LOFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FLT_HIFRQ,PLOG,0,0.0,0.0,ap);
		setup_display(FLT_SWPFRQ,LINEAR,0,0.0,0.0,ap);
		setup_display(FLT_SWPPHASE,LINEAR,0,0.0,0.0,ap);
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;
	case(FLTITER):
		setup_display(FLT_Q,LINEAR,SUBRANGE,1.0,64.0,ap);
		setup_display(FLT_GAIN,LOG,SUBRANGE,0.02,50.0,ap);
		setup_display(FLT_DELAY,LINEAR,SUBRANGE,ap->lo[FLT_DELAY],2.0,ap);
		setup_display(FLT_OUTDUR,NUMERIC,0,0.0,0.0,ap);
		setup_display(FLT_PRESCALE,NUMERIC,0,0.0,0.0,ap);
		setup_display(FLT_RANDDEL,NUMERIC,0,0.0,0.0,ap);
		setup_display(FLT_PSHIFT,LINEAR,0,0.0,0.0,ap);
		setup_display(FLT_AMPSHIFT,LINEAR,0,0.0,0.0,ap);
		break;
	case(ALLPASS):		
		setup_display(FLT_GAIN,LINEAR,0,0.0,0.0,ap);
		setup_display(FLT_DELAY,LINEAR,SUBRANGE,ap->lo[FLT_DELAY],min(ap->hi[FLT_DELAY],64.0),ap);
		setup_display(FLT_PRESCALE,NUMERIC,0,0.0,0.0,ap);
		setup_display(FILT_TAIL,NUMERIC,0,0.0,0.0,ap);
		break;

	case(MOD_LOUDNESS):		
		switch(mode) {
		case(LOUDNESS_GAIN): 
			setup_display(LOUD_GAIN,LINEAR,SUBRANGE,0,20.0,ap);	
			break;
		case(LOUDNESS_DBGAIN): 
			setup_display(LOUD_GAIN,LINEAR,0,0.0,0.0,ap);	
			break;
		case(LOUDNESS_NORM): 
		case(LOUDNESS_SET): 
			setup_display(LOUD_LEVEL,NUMERIC,0,0.0,0.0,ap);	
			break;
		}
		break;		
	case(MOD_SPACE):		
		switch(mode) {
		case(MOD_PAN): 
			setup_display(PAN_PAN,LINEAR,SUBRANGE,-1.0,1.0,ap);	
			setup_display(PAN_PRESCALE,NUMERIC,0,0.0,0.0,ap);	
			break;
		case(MOD_NARROW): 
			setup_display(NARROW,NUMERIC,0,0.0,0.0,ap);	
			break;
		}
		break;		
	case(SCALED_PAN):		
		setup_display(PAN_PAN,LINEAR,SUBRANGE,-1.0,1.0,ap);	
		setup_display(PAN_PRESCALE,NUMERIC,0,0.0,0.0,ap);	
		break;
	case(MOD_PITCH):		
		switch(mode) {
		case(MOD_TRANSPOS):
		case(MOD_TRANSPOS_INFO):
			setup_display(VTRANS_TRANS,LOG,SUBRANGE,0.25,4.0,ap);	
			break;
		case(MOD_TRANSPOS_SEMIT):
		case(MOD_TRANSPOS_SEMIT_INFO):
			setup_display(VTRANS_TRANS,LINEAR,SUBRANGE,-12.0,12.0,ap);	
			break;
		case(MOD_ACCEL):
			setup_display(ACCEL_ACCEL,LOG,SUBRANGE,0.5,2.0,ap);	
			setup_display(ACCEL_GOALTIME,NUMERIC,0,0.0,0.0,ap);	
			setup_display(ACCEL_STARTTIME,NUMERIC,0,0.0,0.0,ap);	
			break;
		case(MOD_VIBRATO):
			setup_display(VIB_FRQ,LINEAR,SUBRANGE,0.0,50.0,ap);	
			setup_display(VIB_DEPTH,LINEAR,SUBRANGE,0.0,12.0,ap);	
			break;
		}
		break;		
	case(MOD_REVECHO):		
		switch(mode) {
		case(MOD_DELAY):
		case(MOD_VDELAY):
			setup_display(DELAY_DELAY,LOG,SUBRANGE,ap->lo[DELAY_DELAY],SECS_TO_MS,ap);	
			setup_display(DELAY_MIX,LINEAR,0,0.0,0.0,ap);	
			setup_display(DELAY_FEEDBACK,LINEAR,0,0.0,0.0,ap);	
			if(mode==MOD_VDELAY) {
				setup_display(DELAY_LFOMOD,LINEAR,0,0.0,0.0,ap);	
				setup_display(DELAY_LFOFRQ,LINEAR,0,0.0,0.0,ap);	
				setup_display(DELAY_LFOPHASE,LINEAR,0,0.0,0.0,ap);	
				setup_display(DELAY_LFODELAY,LINEAR,SUBRANGE,0.0,0.5,ap);	
			}
			setup_display(DELAY_TRAILTIME,LINEAR,SUBRANGE,0.0,2.0,ap);	
			setup_display(DELAY_PRESCALE,LOG,0,0.0,0.0,ap);	
			if(mode==MOD_VDELAY)
				setup_display(DELAY_SEED,LINEAR,0,0.0,0.0,ap);	
			break;
		case(MOD_STADIUM):
			setup_display(STAD_PREGAIN,LINEAR,0,0.0,0.0,ap);	
			setup_display(STAD_ROLLOFF,LINEAR,0,0.0,0.0,ap);	
			setup_display(STAD_SIZE,LOG,SUBRANGE,0.1,10.0,ap);	
			setup_display(STAD_ECHOCNT,LINEAR,SUBRANGE,2,REASONABLE_ECHOCNT,ap);	
			break;
		}
		break;		
	case(MOD_RADICAL):		
		switch(mode) {
		case(MOD_REVERSE):		
			break;
		case(MOD_SHRED):		
			setup_display(SHRED_CNT,LINEAR,SUBRANGE,1.0,2000.0,ap);
			setup_display(SHRED_CHLEN,LINEAR,0,0.0,0.0,ap);
			setup_display(SHRED_SCAT,LINEAR,SUBRANGE,0.0,1.0,ap);
			break;
		case(MOD_SCRUB):		
			setup_display(SCRUB_TOTALDUR,LINEAR,SUBRANGE,ap->lo[SCRUB_TOTALDUR],duration*2.0,ap);
			setup_display(SCRUB_MINSPEED,LINEAR,SUBRANGE,SCRUB_MINSPEED_DEFAULT,0.0,ap);
			setup_display(SCRUB_MAXSPEED,LINEAR,SUBRANGE,0.0,SCRUB_MAXSPEED_DEFAULT,ap);
			setup_display(SCRUB_STARTRANGE,LINEAR,0,0.0,0.0,ap);
			setup_display(SCRUB_ESTART,LINEAR,0,0.0,0.0,ap);
			break;
		case(MOD_LOBIT):		
			setup_display(LOBIT_BRES,LINEAR,0,0.0,0.0,ap);
			setup_display(LOBIT_TSCAN,LINEAR,0,0.0,0.0,ap);
			break;
		case(MOD_LOBIT2):		
			setup_display(LOBIT_BRES,LINEAR,0,0.0,0.0,ap);
			break;
		case(MOD_RINGMOD):		
			setup_display(RM_FRQ,PLOG,SUBRANGE,16.0,400.0,ap);
			break;
		case(MOD_CROSSMOD):		
			break;
		}
		break;		
	case(BRASSAGE):		
		setup_display(GRS_VELOCITY,LINEAR,SUBRANGE,ap->lo[GRS_VELOCITY],8.0,ap);
		switch(mode) {
		case(GRS_REVERB):	
			setup_display(GRS_DENSITY,LOG,SUBRANGE,0.125,8.0,ap);	
			break;
	    default:			
	    	setup_display(GRS_DENSITY,LOG,SUBRANGE,0.125,min(ap->hi[GRS_DENSITY],8.0),ap);			
	    	break;
		}
	    setup_display(GRS_HVELOCITY,LINEAR,SUBRANGE,ap->lo[GRS_HVELOCITY],min(ap->hi[GRS_HVELOCITY],8.0),ap);
	    setup_display(GRS_HDENSITY,LOG,SUBRANGE,0.125,8.0,ap);
	    setup_display(GRS_GRAINSIZE,LINEAR,SUBRANGE,ap->lo[GRS_GRAINSIZE],min(ap->hi[GRS_GRAINSIZE],200.0),ap);
		switch(mode) {
		case(GRS_REVERB):	
			setup_display(GRS_PITCH,LINEAR,0,0.0,0.0,ap);			
	    	break;
	    default:			
			setup_display(GRS_PITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);			
	    	break;
		}
		setup_display(GRS_AMP,LINEAR,0,0.0,0.0,ap);
		setup_display(GRS_SPACE,LINEAR,0,0.0,0.0,ap);	
	    setup_display(GRS_BSPLICE,LINEAR,SUBRANGE,ap->lo[GRS_BSPLICE],min(ap->hi[GRS_BSPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);			
	    setup_display(GRS_ESPLICE,LINEAR,SUBRANGE,ap->lo[GRS_ESPLICE],min(ap->hi[GRS_ESPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);			
	    setup_display(GRS_HGRAINSIZE,LINEAR,SUBRANGE,ap->lo[GRS_GRAINSIZE],min(ap->hi[GRS_GRAINSIZE],200.0),ap);
	    setup_display(GRS_HPITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);			
	    setup_display(GRS_HAMP,LINEAR,0,0.0,0.0,ap);									
		setup_display(GRS_HSPACE,LINEAR,0,0.0,0.0,ap);		
	    setup_display(GRS_HBSPLICE,LINEAR,SUBRANGE,ap->lo[GRS_BSPLICE],min(ap->hi[GRS_BSPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);
	    setup_display(GRS_HESPLICE,LINEAR,SUBRANGE,ap->lo[GRS_ESPLICE],min(ap->hi[GRS_ESPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);
		switch(mode) {
		case(GRS_BRASSAGE):
		case(GRS_FULL_MONTY):
		    setup_display(GRS_SCATTER,LINEAR,0,0.0,0.0,ap);
		    setup_display(GRS_OUTLEN,LINEAR,SUBRANGE,0.0,60.0,ap);
		    setup_display(GRS_CHAN_TO_XTRACT,LINEAR,0,0.0,0.0,ap);						
			/* fall thro */
		case(GRS_REVERB):
		case(GRS_SCRAMBLE):
			setup_display(GRS_SRCHRANGE,LINEAR,SUBRANGE,ap->lo[GRS_SRCHRANGE],min(ap->hi[GRS_SRCHRANGE],200.0),ap);			
			break;
		}
		break;
	case(SAUSAGE):		
		setup_display(GRS_VELOCITY,LINEAR,SUBRANGE,ap->lo[GRS_VELOCITY],8.0,ap);
    	setup_display(GRS_DENSITY,LOG,SUBRANGE,0.125,min(ap->hi[GRS_DENSITY],8.0),ap);			
	    setup_display(GRS_HVELOCITY,LINEAR,SUBRANGE,ap->lo[GRS_HVELOCITY],min(ap->hi[GRS_HVELOCITY],8.0),ap);
	    setup_display(GRS_HDENSITY,LOG,SUBRANGE,0.125,8.0,ap);
	    setup_display(GRS_GRAINSIZE,LINEAR,SUBRANGE,ap->lo[GRS_GRAINSIZE],min(ap->hi[GRS_GRAINSIZE],200.0),ap);
		setup_display(GRS_PITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);			
		setup_display(GRS_AMP,LINEAR,0,0.0,0.0,ap);
		setup_display(GRS_SPACE,LINEAR,0,0.0,0.0,ap);	
	    setup_display(GRS_BSPLICE,LINEAR,SUBRANGE,ap->lo[GRS_BSPLICE],min(ap->hi[GRS_BSPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);			
	    setup_display(GRS_ESPLICE,LINEAR,SUBRANGE,ap->lo[GRS_ESPLICE],min(ap->hi[GRS_ESPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);			
	    setup_display(GRS_HGRAINSIZE,LINEAR,SUBRANGE,ap->lo[GRS_GRAINSIZE],min(ap->hi[GRS_GRAINSIZE],200.0),ap);
	    setup_display(GRS_HPITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);			
	    setup_display(GRS_HAMP,LINEAR,0,0.0,0.0,ap);									
		setup_display(GRS_HSPACE,LINEAR,0,0.0,0.0,ap);		
	    setup_display(GRS_HBSPLICE,LINEAR,SUBRANGE,ap->lo[GRS_BSPLICE],min(ap->hi[GRS_BSPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);
	    setup_display(GRS_HESPLICE,LINEAR,SUBRANGE,ap->lo[GRS_ESPLICE],min(ap->hi[GRS_ESPLICE],GRS_DEFAULT_SPLICELEN * 4.0),ap);
	    setup_display(GRS_SCATTER,LINEAR,0,0.0,0.0,ap);
	    setup_display(GRS_OUTLEN,LINEAR,SUBRANGE,0.0,60.0,ap);
	    setup_display(GRS_CHAN_TO_XTRACT,LINEAR,0,0.0,0.0,ap);						
		setup_display(GRS_SRCHRANGE,LINEAR,SUBRANGE,ap->lo[GRS_SRCHRANGE],min(ap->hi[GRS_SRCHRANGE],200.0),ap);			
		break;
	case(PVOC_ANAL):
		setup_display(PVOC_CHANS_INPUT,POWTWO,0,0.0,0.0,ap);
		setup_display(PVOC_WINOVLP_INPUT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(PVOC_SYNTH):
		break;
	case(PVOC_EXTRACT):
		setup_display(PVOC_CHANS_INPUT,LOG,0,0.0,0.0,ap);
		setup_display(PVOC_WINOVLP_INPUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(PVOC_CHANSLCT_INPUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(PVOC_LOCHAN_INPUT,LINEAR,SUBRANGE,0.0,1024.0,ap);
		setup_display(PVOC_HICHAN_INPUT,LINEAR,SUBRANGE,0.0,1024.0,ap);
		break;
	case(EDIT_CUT):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_END,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(EDIT_CUTMANY):
		setup_display(CM_SPLICELEN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(STACK):
		setup_display(STACK_CNT,   NUMERIC,0,0.0,0.0,ap);
		setup_display(STACK_LEAN,  LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(STACK_OFFSET,NUMERIC,0,0.0,0.0,ap);
		setup_display(STACK_GAIN,  LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(STACK_DUR,   NUMERIC,0,0.0,0.0,ap);
		break;
	case(EDIT_CUTEND):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(EDIT_ZCUT):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_END,NUMERIC,0,0.0,0.0,ap);
		break;
	case(EDIT_EXCISE):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_END,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(EDIT_INSERT):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		setup_display(INSERT_LEVEL,LOG,0,0.0,0.0,ap);
		break;
	case(EDIT_INSERT2):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_END,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		setup_display(INSERT_LEVEL,LOG,0,0.0,0.0,ap);
		break;
	case(EDIT_INSERTSIL):
		setup_display(CUT_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_END,NUMERIC,0,0.0,0.0,ap);
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(EDIT_EXCISEMANY):
	case(INSERTSIL_MANY):
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(JOIN_SEQ):
		setup_display(MAX_LEN,LOGNUMERIC,0,0.0,0.0,ap);
		/* fall thro */
	case(EDIT_JOIN):
	case(JOIN_SEQDYN):
		setup_display(CUT_SPLEN,LINEAR,SUBRANGE,0.0,50.0,ap);
		break;
	case(HOUSE_COPY):
		if(mode==DUPL)
			setup_display(COPY_CNT,LINEAR,SUBRANGE,0.0,100.0,ap);
		break;
	case(HOUSE_CHANS):
		switch(mode) {
		case(HOUSE_CHANNEL):
		case(HOUSE_ZCHANNEL):
			setup_display(CHAN_NO,NUMERIC,0,0.0,0.0,ap);
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
			setup_display(SORT_SMALL,LOG,SUBRANGE,0.25,16.0,ap);
			setup_display(SORT_LARGE,LOG,SUBRANGE,0.25,16.0,ap);
			setup_display(SORT_STEP,LOG,0,0.0,0.0,ap);
			break;
		default:
			break;
		}
		break;
	case(HOUSE_SPEC):
		switch(mode) {
		case(HOUSE_REPROP):
			setup_display(HSPEC_CHANS,NUMERIC,0,0.0,0.0,ap);
			setup_display(HSPEC_TYPE,NUMERIC,0,0.0,0.0,ap);
			/* fall thro */
		case(HOUSE_RESAMPLE):
			setup_display(HSPEC_SRATE,SRATE,0,0.0,0.0,ap);
			break;
		default:
			break;
		}
		break;
	case(HOUSE_EXTRACT):
		switch(mode) {
		case(HOUSE_CUTGATE):
			setup_display(CUTGATE_SPLEN,NUMERIC,0,0.0,0.0,ap);
			/* fall thro */
		case(HOUSE_ONSETS):
			setup_display(CUTGATE_GATE,LINEAR,SUBRANGE,0.0,0.15,ap);
			setup_display(CUTGATE_ENDGATE,LINEAR,SUBRANGE,0.0,0.15,ap);
			setup_display(CUTGATE_THRESH,LINEAR,SUBRANGE,0.0,0.06,ap);
			if(mode == HOUSE_CUTGATE)
				setup_display(CUTGATE_SUSTAIN,NUMERIC,0,0.0,0.0,ap);
			setup_display(CUTGATE_BAKTRAK,NUMERIC,0,0.0,0.0,ap);
			setup_display(CUTGATE_INITLEVEL,LINEAR,SUBRANGE,0.0,0.06,ap);
			setup_display(CUTGATE_MINLEN,LINEAR,SUBRANGE,0.0,min(duration,5.0),ap);
			setup_display(CUTGATE_WINDOWS,NUMERIC,0,0.0,0.0,ap);
			break;
		case(HOUSE_CUTGATE_PREVIEW):
			break;
		case(HOUSE_TOPNTAIL):
			setup_display(TOPN_GATE,LINEAR,SUBRANGE,0.0,0.15,ap);
			setup_display(TOPN_SPLEN,NUMERIC,0,0.0,0.0,ap);
			break;
		case(HOUSE_RECTIFY):
			setup_display(RECTIFY_SHIFT,LINEAR,SUBRANGE,-0.001,0.001,ap);
			break;
		case(HOUSE_BYHAND):
			break;
		}
		break;
	case(TOPNTAIL_CLICKS):
		setup_display(TOPN_GATE,LINEAR,SUBRANGE,0.0,0.15,ap);
		setup_display(TOPN_SPLEN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(HOUSE_BAKUP):
	case(HOUSE_DUMP):
		break;
	case(HOUSE_GATE):
		setup_display(GATE_ZEROS,NUMERIC,0,0.0,0.0,ap);
		break;
	case(HOUSE_RECOVER):
		setup_display(DUMP_OK_CNT,NUMERIC,0,0.0,0.0,ap);
		setup_display(DUMP_OK_PROP,NUMERIC,0,0.0,0.0,ap);
		setup_display(DUMP_OK_SAME,NUMERIC,0,0.0,0.0,ap);
		setup_display(SFREC_SHIFT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(HOUSE_DISK):
		break;
	case(INFO_TIMESUM):
		setup_display(TIMESUM_SPLEN,LINEAR,SUBRANGE,0.0,200.0,ap);
		break;	
	case(INFO_SAMPTOTIME):
		setup_display(INFO_SAMPS,NUMERIC,0,0.0,0.0,ap);
		break;
	case(INFO_TIMETOSAMP):
		setup_display(INFO_TIME,NUMERIC,0,0.0,0.0,ap);
		break;
	case(INFO_FINDHOLE):
		setup_display(HOLE_THRESH,NUMERIC,0,0.0,0.0,ap);
		break;	
	case(INFO_DIFF):
	case(INFO_CDIFF):
		setup_display(SFDIFF_THRESH,NUMERIC,0,0.0,0.0,ap);
		setup_display(SFDIFF_CNT,LINEAR,SUBRANGE,1.0,64.0,ap);
		break;	
	case(INFO_PRNTSND):
		setup_display(PRNT_START,NUMERIC,0,0.0,0.0,ap);
		setup_display(PRNT_END,NUMERIC,0,0.0,0.0,ap);
		break;	
	case(INFO_MUSUNITS):
		switch(mode) {
		case(MU_MIDI_TO_FRQ):
		case(MU_MIDI_TO_NOTE):			setup_display(MUSUNIT,NUMERIC,0,0.0,0.0,ap);	break;

		case(MU_FRQ_TO_MIDI):
		case(MU_FRQ_TO_NOTE):			setup_display(MUSUNIT,PLOG,0,0.0,0.0,ap);		break;

		case(MU_GAIN_TO_DB):			setup_display(MUSUNIT,LINEAR,0,0.0,0.0,ap);				break;
		case(MU_DB_TO_GAIN):			setup_display(MUSUNIT,LINEAR,SUBRANGE,-96.0,0.0,ap);	break;


		case(MU_FRQRATIO_TO_SEMIT):		
		case(MU_FRQRATIO_TO_INTVL):
		case(MU_FRQRATIO_TO_OCTS):
		case(MU_FRQRATIO_TO_TSTRETH):	setup_display(MUSUNIT,LOG,SUBRANGE,0.25,4.0,ap);	break;

		case(MU_SEMIT_TO_FRQRATIO):
		case(MU_SEMIT_TO_OCTS):			
		case(MU_SEMIT_TO_INTVL):		
		case(MU_SEMIT_TO_TSTRETCH):		setup_display(MUSUNIT,NUMERIC,0,0.0,0.0,ap);	break;

		case(MU_OCTS_TO_FRQRATIO):
		case(MU_OCTS_TO_SEMIT):			
		case(MU_OCTS_TO_TSTRETCH):		setup_display(MUSUNIT,NUMERIC,0,0.0,0.0,ap);		break;

		case(MU_TSTRETCH_TO_FRQRATIO):	
		case(MU_TSTRETCH_TO_SEMIT):		
		case(MU_TSTRETCH_TO_OCTS):		
		case(MU_TSTRETCH_TO_INTVL):		setup_display(MUSUNIT,LOG,0,0.0,0.0,ap);		break;

		case(MU_INTVL_TO_FRQRATIO):
		case(MU_INTVL_TO_TSTRETCH):
		case(MU_NOTE_TO_FRQ):
		case(MU_NOTE_TO_MIDI):			  /* dealt with by special.c */
			break;
		}
		break;
	case(INFO_PROPS):
	case(INFO_SFLEN):
	case(INFO_TIMELIST):
	case(INFO_LOUDLIST):
	case(INFO_TIMEDIFF):
	case(INFO_LOUDCHAN):
	case(INFO_MAXSAMP):
		break;
	case(INFO_MAXSAMP2):
		setup_display(MAX_STIME,LINEAR,0,0.0,0.0,ap);
		setup_display(MAX_ETIME,LINEAR,0,0.0,0.0,ap);
		break;
	case(SYNTH_WAVE):
		setup_display(SYN_TABSIZE,LOG,0,0.0,0.0,ap);
		setup_display(SYN_FRQ,PLOG,SUBRANGE,16.0,4000.0,ap);
		/* fall thro */
	case(SYNTH_NOISE):
		setup_display(SYN_AMP,LINEAR,0,0.0,0.0,ap);
		/* fall thro */
	case(SYNTH_SIL):
		setup_display(SYN_SRATE,SRATE,0,0.0,0.0,ap);
		setup_display(SYN_CHANS,NUMERIC,0,0.0,0.0,ap);
		setup_display(SYN_DUR,LOG,0,0.0,0.0,ap);
		break;
	case(MULTI_SYN):
		setup_display(SYN_TABSIZE,LOG,0,0.0,0.0,ap);
		setup_display(SYN_AMP,NUMERIC,0,0.0,0.0,ap);
		setup_display(SYN_SRATE,SRATE,0,0.0,0.0,ap);
		setup_display(SYN_CHANS,NUMERIC,0,0.0,0.0,ap);
		setup_display(SYN_DUR,LOG,0,0.0,0.0,ap);
		break;
	case(SYNTH_SPEC):
		setup_display(SS_DUR,LOG,0,0.0,0.0,ap);
		setup_display(SS_CENTRFRQ,LOG,0,0.0,0.0,ap);
		setup_display(SS_SPREAD,LINEAR,0,0.0,0.0,ap);
		setup_display(SS_FOCUS,LINEAR,0,0.0,0.0,ap);
		setup_display(SS_FOCUS2,LINEAR,0,0.0,0.0,ap);
		setup_display(SS_TRAND,LINEAR,0,0.0,0.0,ap);
		setup_display(SS_SRATE,SRATE,0,0.0,0.0,ap);
		break;
	case(RANDCUTS):
		setup_display(RC_CHLEN,LINEAR,0,0.0,0.0,ap);
		setup_display(RC_SCAT,LINEAR,0,0.0,0.0,ap);
		break;
	case(RANDCHUNKS):
		setup_display(CHUNKCNT,LINEAR,SUBRANGE,2,20,ap);
		setup_display(MINCHUNK,LINEAR,0,0.0,0.0,ap);
		setup_display(MAXCHUNK,LINEAR,0,0.0,0.0,ap);
		break;
	case(SIN_TAB):
		setup_display(SIN_FRQ,LOG,0,0.0,0.0,ap);
		setup_display(SIN_AMP,LINEAR,0,0.0,0.0,ap);
		setup_display(SIN_DUR,LINEAR,SUBRANGE,1,60,ap);
		setup_display(SIN_QUANT,LOG,0,0.0,0.0,ap);
		setup_display(SIN_PHASE,LINEAR,0,0.0,0.0,ap);
		break;
	case(ACC_STREAM):
		setup_display(ACC_ATTEN,LINEAR,0,0.0,0.0,ap);
		break;
	case(HF_PERM1):
	case(HF_PERM2):
		setup_display(HP1_SRATE,SRATE,0,0.0,0.0,ap);
		setup_display(HP1_ELEMENT_SIZE,NUMERIC,0,0.0,0.0,ap);
		setup_display(HP1_GAP_SIZE,NUMERIC,0,0.0,0.0,ap);
		setup_display(HP1_GGAP_SIZE,NUMERIC,0,0.0,0.0,ap);
		setup_display(HP1_MINSET,NUMERIC,0,0.0,0.0,ap);
		setup_display(HP1_BOTNOTE,MIDI,0,0.0,0.0,ap);
		setup_display(HP1_BOTOCT,OCTAVES,0,0.0,0.0,ap);
		setup_display(HP1_TOPNOTE,MIDI,0,0.0,0.0,ap);
		setup_display(HP1_TOPOCT,OCTAVES,0,0.0,0.0,ap);
		setup_display(HP1_SORT1,CHORDSORT,0,0.0,0.0,ap);
		break;
	case(DEL_PERM):
		setup_display(HP1_SRATE,SRATE,0,0.0,0.0,ap);
		setup_display(DP_DUR,NUMERIC,0,0.0,0.0,ap);
		setup_display(DP_CYCCNT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(DEL_PERM2):
		setup_display(DP_CYCCNT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(TWIXT):
	case(SPHINX):
		setup_display(IS_SPLEN,NUMERIC,0,0.0,0.0,ap);
		if(process != TWIXT || mode != TRUE_EDIT)
			setup_display(IS_WEIGHT,NUMERIC,0,0.0,0.0,ap);
		setup_display(IS_SEGCNT,LOG,0,0.0,0.0,ap);
		break;
	case(NOISE_SUPRESS):
		setup_display(NOISE_SPLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(NOISE_MINFRQ,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(MIN_NOISLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(MIN_TONELEN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(TIME_GRID):
		setup_display(GRID_COUNT,NUMERIC,0,0.0,0.0,ap);
		setup_display(GRID_WIDTH,LOG,SUBRANGE,0.002,0.2,ap);
		setup_display(GRID_SPLEN,LOG,SUBRANGE,2.0,50.0,ap);
		break;
	case(SEQUENCER2):
		setup_display(SEQ_SPLIC,NUMERIC,0,0.0,0.0,ap);
		/* fall thro */
	case(SEQUENCER):
		setup_display(SEQ_ATTEN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(CONVOLVE):
		switch(mode) {
		case(CONV_NORMAL):	
			break;
		case(CONV_TVAR):	
			setup_display(CONV_TRANS,LINEAR,SUBRANGE,-12.0,12.0,ap);	
			break;
		}
		break;
	case(BAKTOBAK):
		setup_display(BTOB_CUT,NUMERIC,0,0.0,0.0,ap);
		setup_display(BTOB_SPLEN,LOGNUMERIC,SUBRANGE,0.0001,100.0,ap);
		break;
	case(FIND_PANPOS):
		setup_display(PAN_PAN,NUMERIC,0,0.0,0.0,ap);
	case(CLICK):
		setup_display(CLIKSTART,NUMERIC,0,0.0,0.0,ap);
		setup_display(CLIKEND,NUMERIC,0,0.0,0.0,ap);
		setup_display(CLIKOFSET,NUMERIC,0,0.0,0.0,ap);
		break;
	case(DOUBLETS):
		setup_display(SEG_DUR,LOG,0,0.0,0.0,ap);
		setup_display(SEG_REPETS,NUMERIC,0,0.0,0.0,ap);
		break;
	case(SYLLABS):
		setup_display(SYLLAB_DOVETAIL,NUMERIC,0,0.0,0.0,ap);
		setup_display(SYLLAB_SPLICELEN,NUMERIC,0,0.0,0.0,ap);
		break;
	case(MAKE_VFILT):
	case(MIX_MODEL):
		break;
	case(BATCH_EXPAND):
		setup_display(BE_INFILE,NUMERIC,0,0.0,0.0,ap);
		setup_display(BE_OUTFILE,NUMERIC,0,0.0,0.0,ap);
		setup_display(BE_PARAM,NUMERIC,0,0.0,0.0,ap);
		break;
	case(CYCINBETWEEN):
		setup_display(INBETW,NUMERIC,0,0.0,0.0,ap);
		setup_display(BTWN_HFRQ,LOGNUMERIC,0,0.0,0.0,ap);
		break;
	case(ENVSYN):
		setup_display(ENVSYN_WSIZE,LINEAR,SUBRANGE,ap->lo[ENVSYN_WSIZE],min(ap->hi[ENVSYN_WSIZE],1000.0),ap);
		setup_display(ENVSYN_DUR,LOG,0,0.0,0.0,ap);
		setup_display(ENVSYN_CYCLEN,LOG,0,0.0,0.0,ap);
		setup_display(ENVSYN_STARTPHASE,NUMERIC,0,0.0,0.0,ap);
		if(mode != ENVSYN_USERDEF) {
			setup_display(ENVSYN_TROF,LINEAR,0,0.0,0.0,ap);
			setup_display(ENVSYN_EXPON,LOG,0,0.0,0.0,ap);
		}
		break;	
	case(RRRR_EXTEND):
		if(mode == 0) {
			setup_display(RRR_START,NUMERIC,0,0.0,0.0,ap);
			setup_display(RRR_END,NUMERIC,0,0.0,0.0,ap);
		} else {
			setup_display(RRR_GATE,NUMERIC,0,0.0,0.0,ap);
			setup_display(RRR_SKIP,NUMERIC,0,0.0,0.0,ap);
			setup_display(RRR_GRSIZ,LINEAR,0,0.0,0.0,ap);
			setup_display(RRR_AFTER,NUMERIC,0,0.0,0.0,ap);
			setup_display(RRR_TEMPO,LOGNUMERIC,0,0.0,0.0,ap);
			setup_display(RRR_AT,NUMERIC,0,0.0,0.0,ap);
		}
		setup_display(RRR_STRETCH,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(RRR_GET,NUMERIC,0,0.0,0.0,ap);
		setup_display(RRR_RANGE,LINEAR,0,0.0,0.0,ap);
		setup_display(RRR_REPET,LINEAR,SUBRANGE,1.0,3.0,ap);
		setup_display(RRR_ASCAT,LINEAR,0,0.0,0.0,ap);
		setup_display(RRR_PSCAT,LINEAR,SUBRANGE,0.0,1.0,ap);
		break;
	case(SSSS_EXTEND):
		setup_display(SSS_DUR,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(NOISE_MINFRQ,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(MIN_NOISLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(MAX_NOISLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(SSS_GATE,NUMERIC,0,0.0,0.0,ap);
		break;
	case(HOUSE_GATE2):
		setup_display(GATE2_DUR,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(GATE2_ZEROS,LOGNUMERIC,0,0.0,0.0,ap);
		setup_display(GATE2_LEVEL,NUMERIC,0,0.0,0.0,ap);
		setup_display(GATE2_SPLEN,NUMERIC,0,0.0,0.0,ap);
		setup_display(GATE2_FILT,NUMERIC,0,0.0,0.0,ap);
		break;
	case(GRAIN_ASSESS):
		break;
	case(ZCROSS_RATIO):
		setup_display(ZC_START,NUMERIC,0,0.0,0.0,ap);
		setup_display(ZC_END,NUMERIC,0,0.0,0.0,ap);
		break;
	case(GREV):
		setup_display(GREV_WSIZE,NUMERIC,0,0.0,0.0,ap);
		setup_display(GREV_TROFRAC,NUMERIC,0,0.0,0.0,ap);
		setup_display(GREV_GPCNT,LINEAR,0,0.0,0.0,ap);
		switch(mode) {
		case(GREV_TSTRETCH):
			setup_display(GREV_TSTR,LOG,0,0.0,0.0,ap);
			break;
		case(GREV_DELETE):
		case(GREV_OMIT):
			setup_display(GREV_KEEP,LINEAR,SUBRANGE,1.0,63.0,ap);
			setup_display(GREV_OUTOF,LINEAR,SUBRANGE,2.0,64.0,ap);
			break;
		case(GREV_REPEAT):
			setup_display(GREV_REPETS,LINEAR,0,0.0,0.0,ap);
			break;
		}
		break;
	case(MANY_ZCUTS):
		break;
	}
	return(FINISHED);
}

/****************************** SETUP_DISPLAY *********************************/

void setup_display(int paramno,int dtype,int subrang,double lo,double hi,aplptr ap)
{
	ap->display_type[paramno]	= (char)dtype;
	ap->has_subrange[paramno]	= (char)subrang;
	ap->lolo[paramno]		 	= lo;
	ap->hihi[paramno] 		 	= hi;
}

