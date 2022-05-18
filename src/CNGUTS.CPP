// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnguts.cpp  - Hourly simulation main routine for CSE

//------------------------------- INCLUDES ----------------------------------
#include <CNGLOB.H>	// CSE global defines: USI LI ASSERT dtypes.h cndefns.h

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// WSHADRATstr ZNRstr

#include <MSGHANS.H>	// MH_C0100

#include <VRPAK.H>	// vrOpen vrClose
#include <TDPAK.H>	// tddtdis tddyi tddis
#include "solar.h"
#include <XIOPAK.H>	// xfopen

#include <CGWTHR.H>	// cgwfxxxx functions
#include <ENVPAK.H>	// ensystd

#include <CUEVF.H>	// EVFINIT EVFMON
#include <CUPARSEX.H>	// maxErrors. only, 5-97.
#include <EXMAN.H>	// exEvEvf
#include <CNCULT.H>	// UENDIVL
#include <IRATS.H>	// RiB XiB
#include <mspak.h>
#include "TIMER.H"
#include "CSE.h"
#include <IMPF.H>	// impfStart impfIvl 2-94
#ifdef WINorDLL
#include <windows.h>	// probably needed before brfw.h
#ifdef BINRES //cnglob.h
#include <cnewin.h>		// BrHans -- if needed here
#endif
#endif
#include <BRFW.H>	// ResfWriter

#include <CNGUTS.H>	// decls for this file

//-------------------------------- DEFINES ----------------------------------


//------------------------- FILE-GLOBAL VARIABLES ---------------------------

#ifdef BINRES //cnglob.h
static BOO NEAR brf = FALSE;	// set if generating either binary results file: if Top.brs or .brHrly is set.

static ResfWriter brfw;  	// object used to write binary results files, brfw.h/cpp. NEAR removed 6-95: DGROUP full.

static BOO NEAR dtStart = FALSE;	// Daylight Time Start hour flag 12-31-93:
  					   // set by tp_DoDTStuff to tell doIvlAccum to duplicate next hour's data for the skipped hour
  					   // (and when DT goes off, the extra hour's data is lost (overwritten)).
#endif


//----------------------- LOCAL FUNCTION DECLARATIONS -----------------------

LOCAL RC   FC NEAR doBegIvl();
LOCAL RC   FC NEAR doEndIvl();
LOCAL RC   FC NEAR cgAfterWarmup();
LOCAL RC   FC NEAR doHourGains();
LOCAL RC   FC NEAR doIvlExprs( int ivlStage);
LOCAL void FC NEAR doIvlAccum();
LOCAL void FC NEAR mtrsAccum( IVLCH ivl, int firstflg, int lastflg);
LOCAL void FC NEAR doIvlFinalize();
LOCAL void FC NEAR mtrsFinalize( IVLCH ivl, int firstflg);
LOCAL void FC NEAR accumZr( ZNRES_IVL_SUB *res1, ZNRES_IVL_SUB *res2, BOO firstflg, BOO lastflg);
LOCAL void FC NEAR accumAhr( AHRES_IVL_SUB *res1, AHRES_IVL_SUB *res2, BOO firstflg, BOO lastflg);
LOCAL void FC NEAR doIvlPrior();
LOCAL void FC NEAR setPriorRes( ZNRES_IVL_SUB* resCurr);
LOCAL void FC NEAR doIvlReports();
LOCAL void FC NEAR binResInit( int isAusz);
LOCAL void FC NEAR binResFinish();

//-----------------------------------------------------------------------------------------------------------
void FC cgClean( 		// cg overall init/cleanup routine

	CLEANCASE cs )	// [STARTUP,], ENTRY, DONE, or CRASH (type in cnglob.h)

{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */

	//****** incomplete: do all cg modules from (end of) this fcn

// clean weather stuff 1-94
	cgWthrClean(cs);

// clean rundata.cpp/cnguts.cpp variables

	//InputFileName: 		ptr to argv[], free not needed.
	//InputFilePath:		ptr to cse.cpp stack, free not needed.

	if (cs != ENTRY)			// not at entry: may be set b4 call & used after.
		dmfree( DMPP( exePath));		// free exe/dll;exe path, 2-95.
	dmfree( DMPP( InputFilePathNoExt));	// free input file name with any .ext removed. sets NULL, NOP if already NULL. dmpak.cpp.
	dmfree( DMPP( InputDirPath));		// free drive/dir path to input file, 2-95.
	dmfree( DMPP( cmdLineArgs));

	vrInfoClean((VROUTINFO*)&PriRep);	// clean primary report file virt rpt info. Depends on NULL terminator.
	memset(&PriRep, 0, sizeof(PriRep));	// code assumes PriRep starts all 0.

	cnRunSerial = 0;			// run serial number currently starts at 1 (++'d in cgInit) for each CSE subr pkg call

	vrInfoClean(UnspoolInfo);		// clean contents of virt rpt unspooling info: free filenames. nop if NULL.
	// depends on no .fName free'd (thus NULL'd) in middle of block.
	dmfree( DMPP( UnspoolInfo));	// if not NULL, free dm block, set ptr NULL. dmpak.cpp.

	cgDone();				// simulator cleanup, below, normally redundant
	runFazDataFree();		// free run basAnc records, etc. below, normally redundant. 6-95.

	Top.tp_LocDone();		// clean up location stuff: free Locsolar. redundant call ok.

#if defined( BINRES)
	brfw.clean();			// clean the only ResfWriter object. local object, brfw.cpp code.
#endif

	IffnmB.free();	   	// Import File Field Names info: used for autosize & run & further compile support til CLEAR;
						// may not be free'd in runFazDataFree(). 2-94.

// other CSE simulator guts modules

	// *** do other cg modules (all files in cnguts.h) from here ****

}		// cgClean
//-----------------------------------------------------------------------------------------------------------
void FC cgPreInit()		// preliminary cnguts.cpp initialization needed before showProbeNames

/* 'showProbeNames' displays member names of all registered rats for CSE -p.
   It is executed before most initialization and can be executed without doing a run. */

{
// init the 1-record runtime "Top basAnc" of one-instance-only input parameters (do b4 showProbeNames call)
	TopR.statSetup( Top, 1, 0/*noZ*/); 	// init cnguts:TopR as basAnc for 1-record static tbl "Top";
	// init cnguts:Top as record of basAnc: init front members, does not alter data members.
	// "Top" can then be accessed staticly, or via general routines that work thru TopR.
	// See also cncult.cpp:TopiR.  ancrec.cpp fcn.
// init 1-record static weather info basAncs similarly, 1-94
	WfileR.statSetup( Wfile, 1, 0);	// open weather file info
	WthrR.statSetup( Wthr, 1, 0);	// current hour weather info
	WthrNxHrR.statSetup( WthrNxHr, 1, 0);		// next hour weather info, for read-ahead
// set run basAnc record ownership relations -- anticipate cncult.2,3,4 code for showProbeNames.
	GnB.ownB  = &ZrB;			// gains are owned by zones  1-92
	ZhxB.ownB  = &ZrB;			// zone hvac transfers are (ambiguously) owned by zones 3-92
	RcolB.ownB = &RiB;			// columns owned by reports. no run basAnc so point to input; .what is same. (cncult4.cpp)
	XcolB.ownB = &XiB;			// export columns are owned by exports ...
	DvriB.ownB = &ZrB;			// date dependent virtual reports belong to zones (cncult4.cpp) (?? 1-92)
}			// cgPreInit
//-----------------------------------------------------------------------------------------------------------
void FC cgInit()	/* Hourly simulator initialization done before data input for each run:
			   stuff done ONCE for both autosize and main simulation phases */

{
// callers: cse.cpp.  hmm... put remaining code inline there?

// ^C interrupt init: is done at startup by config.cpp
// and re-done at each ^C in tp_MainSim() (or other place ^C intercepted).

// Next run serial number, in lieu of future status file
	cnRunSerial++;		// data init to 0; copied to Topi.runSerial by cncult2:topStarPrf2.
}                       // cgInit
//-----------------------------------------------------------------------------------------------------------
void FC cgDone()

// Hourly simulator cleanup routine: cnguts stuff done ONCE for both autosize and main simulation phases

// Call when leaving simulator, even in cases where cgAusz() or Top::tp_MainSim() not called due to errors.

{
// clear the "non-phasely" run data -- that which persists thru autosize and main sim
	//reports/exports run data:
	// note: runtime info for RE/EXPORTFILEs: all there is in UnspoolInfo.
	// note: runtime info for RE/EXPORTs is in DvriB and RcolB/XcolB, various globals and Top and ZnR members, and UnspoolInfo.p
	//basAncs here must match those not cleared in runFazDataFree()(next), and those flagged RFPERSIS in cncult2.cpp, 10-95
	DvriB.free(); 	// Date-dependent virtual reports spooling info
	RcolB.free(); 	// User-defined report column info
	XcolB.free(); 	// User-defined export column info
	// dmfree( DMPP( UnspoolInfo));	is done in cse.cpp after use after call to here.

	//IffnmB.free();	   Import File Field Names info: also used for compile support, tho not via CULTs,
	//			   data must be retained for next run until CLEAR. 2-94.

	// terminate use of any external modules
}		// cgDone
//-----------------------------------------------------------------------------------------------------------
void FC runFazDataFree()

// free CSE runtime basAnc records and storage.
// These records are built from input records during check/setup by cncult2:topCkf and its callees.
// Call after each phase (autosize and main sim)
//    and when leaving CSE even when cgAusz/tp_MainSim not called due to errors.

{
	// Clear/free the "phasely" basAnc's into which building description, etc, is put.
	// Destructors also free pointed-to blocks. These basAncs are in rundata.cpp.

	// Note: entries are put into most of these in input code in cncult2.cpp:topCkf and its subfcns.

	ZrB.free(); 	// Zones runtime info
	ZnresB.free();	// zone simulation results
	RsResR.free();	// RSYS results
	DsResR.free();	// DUCTSEG results
	WsResR.free();	// DHWSYS results
	IzxR.free();	// interzone transfers
	GnB.free();   	// (zone) gains
	MtrB.free();  	// meters
	TuB.free();  	// Terminal units
	AhB.free();  	// Air handling units
	ZhxB.free(); 	// Zone HVAC transfers
	AhresB.free(); 	// Air handler results
	HpB.free(); 	// HeatPlants
	BlrB.free();	// Boilers
	CpiB.free();	// COOLPLANTs run info
	ChiB.free();	// CHILLERs run info
	TpiB.free();	// TOWERPLANTs run info
	XsB.free(); 	// runtime XSURFs
	WshadR.free();	// windows overhang/fin info
	MsR.free(); 	// Masses
	SgR.free(); 	// Solar gain tables
	GtB.free();		// GlazeTypes 12-94
	HdayB.free(); 	// Holidays

	ImpfB.free();	// IMPORTFILEs run info 2-94
	//IffnmB.free();	   Import File Field Names info: also used for compile support, tho not via CULTs,
	//			   data must be retained for autosize/main sim phases, and for next run until CLEAR. 2-94.

	//following basAncs persist from autosize thru main sim, not re-setup, thus free'd in cgDone above, not here.
	// note: runtime info for RE/EXPORTFILEs: all there is in UnspoolInfo.
	// note: runtime info for RE/EXPORTs is in DvriB and RcolB/XcolB, various globals and Top and ZnR members, and UnspoolInfo.p
	// these must be same basAncs flagged RFPERSIS in cncult2.cpp, 10-95.
	//DvriB.free(); 	// Date-dependent virtual reports spooling info
	//RcolB.free(); 	// User-defined report column info
	//XcolB.free(); 	// User-defined export column info

}				// runFazDataFree
//-----------------------------------------------------------------------------
BOOL DbDo(				// handy DbShouldPrint + headings
	DWORD oMsk,		// dbdXXX debug category
					// or special values
					//    dbdSTARTRUN: init at run beg
					//    dbdSTARTSTEP: init at timestep beg
	int options /*=0*/)		// options
							//   dbdoptNOHDGS: suppress headings
// WHY: triggers headings upon 1st actual print
// return nz iff caller should print
{
	int ret = 0;
#if defined( DEBUGDUMP)
	static int dbgDoneRunHdg = FALSE;
	static int dbgDoneStepHdg = FALSE;
	bool bDoHdgs = !(options & dbdoptNOHDGS);
	if (oMsk == dbdSTARTRUN)
		dbgDoneRunHdg = dbgDoneStepHdg = FALSE;
	else if (oMsk == dbdSTARTSTEP)
		dbgDoneStepHdg = FALSE;
	else if (DbShouldPrint( oMsk))
	{	// this debug category is selected
		if (bDoHdgs && !dbgDoneRunHdg)
		{	DbPrintf( "################\n%s%s %s %s   %s\n",
				  Top.tp_RepTestPfx(),
				  ProgName, ProgVersion, ProgVariant, Top.runDateTime );
			dbgDoneRunHdg = TRUE;
		}
		if (bDoHdgs && !dbgDoneStepHdg)
		{	const char* tAusz="";
			if (Top.tp_autoSizing)
			{	const char* tPass =
					  Top.tp_pass1A ? "1A"
					: Top.tp_pass1B ? "1B"
					: Top.tp_pass2  ? "2"
					:                 "?";
				tAusz = strtprintf( " pass=%s itr=%d", tPass, Top.dsDayNIt);
			}
			DbPrintf( "\n\n================\n%s%s  hr=%d subhr=%d\n----------------\n",
					Top.dateStr ? Top.dateStr : "(No date)",
					tAusz,
					Top.iHr, Top.iSubhr);
			dbgDoneStepHdg = TRUE;
		}
		ret = 1;	// yes, caller should print
	}
#endif
	return ret;
}		// ::DbDo
//-----------------------------------------------------------------------------------------------------------
RC TOPRAT::tp_MainSim()		// Main hourly simulation entry point

// inputs are in Top and other globals.
// returns RCOK if ok, other value if error or ^C, message already issued. ^C returns MH_C0100 (msghans.h).

{
// init and do run
	RC rc = tp_MainSimI();	// call inner fcn: separate fcn so errors can return directly yet do cgrunend().  next.
							// exact rc is returned re ^C detection.
// end main sim run (or autosize design day) cleanup stuff done even after initialization or run error
	RC rc2 = cgRddDone(FALSE);	// (part done for each autosize design day -- not distinct here)
	rc2 |= cgFazDone(FALSE);	// (part done for autosize only after all design days)(empty fcn 7-95)
	return rc ? rc : rc2;	// return exact rc from tp_MainSimI re ^C detection
}		// TOPRAT::tp_MainSim()
//-----------------------------------------------------------------------------------------------------------
RC TOPRAT::tp_MainSimI()		// Main hourly simulation inner routine

// called once per run; separate fcn so errors can return, not break out of many loops

// inputs are in Top and other globals and run basAncs, mostly in rundata.cpp.
// returns RCOK if ok, other value if error or ^C (returns MH_C0100), message already issued.

{
	// caller: cgMainsim
	RC rc=RCOK;

// initialization not repeated after warmup

	// set "doing" values before initialization
	tp_dsDay = 0;				// say main sim, not autosizing design days.
	tp_ClearAuszFlags();		// no autosizing underway

	CSE_EF( cgFazInit(FALSE) );	// main Sim or autosize phase initialization. local function below.
								// Inits autoSizing and peak-recording stuff in AH's, TU's, etc. 6-95.
	CSE_EF( cgRddInit(FALSE) );	// more init (what is repeated for each des day for autoSize). local below.
								// CSE_EF: if error, return bad now.

	CSE_EF( tp_ExshRunInit());	// Penumbra external shading initialization

	screen( NONL|QUIETIF, " Warmup");	// screen msg on current line (rmkerr.cpp): beg of warmup.
    									//   Continues line started in cse.cpp.

	tp_ebErrCount = 0;		// count of short-interval energy balance errors

// loop over warmup and run

	isBegRun = TRUE;		// say beginning of run: on for 1st subhr of warmup,
							//   or of run if no warmup. tp_SimHour clears.
	for (isWarmup = 2;   isWarmup-- > 0;  )	// 1 warmup cycle and 1 run
	{
		if (!isWarmup)				// if start of run (warmup done)
		{	if (getScreenCol() + 4*((nDays+30)/31) > 54)	// unless all months will fit current line
				screen( 0, "   "); 		// new screen line: newline (no NONL) & indent (+space in texts).
										//  12 month names and " Reports" can fit one line.
										//  Coordinate with monthly screen msg in doBegIvl().
			tp_isBegMainSim = TRUE;		// say beg of simulation (non-warmup, non-autozize)
		}
		ivl = C_IVLCH_Y;			// say start of run
		jDay = tp_begDay - 1;		// set day to be tp_begDay after ++ at loop head
		if (isWarmup)				// if doing initialization warmup that precedes run
		{
			// for warmup, usually use days just before run
			jDay -= wuDays;			// set to warmup using days before run (neg fixed below)

			// conditionally adjust warmup start forward to weather file start when known
			Wfile.wf_FixJday(jDay, tp_begDay);
		}
		tp_DTInit();					// init daylight/standard time stuff.  Uses jDay.
		int dayDownCount = isWarmup ? wuDays : nDays;	// number of days in warmup or run
		while (dayDownCount-- > 0)				// loop over days
		{
			if (++jDay==366)	   			// next day; if 365+1
				jDay = 1;    				// wrap at end year
			xJDay = jDay;					// extended jDay same when not autosizing
			isLastWarmupDay = isWarmup && (dayDownCount==0);	// flag TRUE on last warmup day
			CSE_E( tp_SimDay());			// simulate for one day, next. On error return exact rc re ^C.
#if 0
x			if (jDay == 35)		// test for abort handling
x				err( PABT, "Die die die");
#endif

		}
		if (isWarmup > 0)
			CSE_EF( cgAfterWarmup())		// stuff done after main sim warmup. CSE_EF: if error, return bad now.
	}
	// CAUTION isWarmup is normally -1 not 0 here

// end of run stuff
	return rc;			// many addl returns above including in E, CSE_EF macros
}		// TOPRAT::tp_MainSimI
//-----------------------------------------------------------------------------------------------------------
RC TOPRAT::tp_SimDay()		// Simulate for one day

// non-RCOK return means end run, including MH_C0100 for keyboard interrupt. Msg already issued.

{
	RC rc = RCOK;

// simulate 24 hours
	for (iHr = 0;  iHr < 24;  iHr++)	// loop over (Daylight Time) hours 0 to 23
	{
		CSE_E( tp_SimHour());// simulate for hour, next.  Alters iHr if DT starts or ends.
		//  on error return exact rc re ^C detection (MH_C0100).
		yielder();			// if under Windows, let other applications run. rmkerr.cpp. 2-94.
	}						//   note daily yield (after } ) got complaints from ET beta testers, 5-95.

	// for autosizing, rewind import files
	// Thus each repetion of day uses same data. Also all days use same data,
	//   and main sim must use a different file or start with same day of data.
	if ( tp_autoSizing)
		CSE_E( impfAfterWarmup());		// position file for next rep of day, or for warmup of main sim. impf.ccp.
	return rc;
}		// TOPRAT::tp_SimDay
//-----------------------------------------------------------------------------------------------------------
RC TOPRAT::tp_SimHour()		// Simulate for current hour

// non-RCOK return means end run, including MH_C0100 for keyboard interrupt. Msg already issued.

{
	RC rc=RCOK;
	XSRAT* xr;
	BOILER *blr;   /*COOLPLANT *cp;*/
	CHILLER *ch;
	TOWERPLANT *tp;
	do  					// subHour loop
	{
		// start subhour, and start coincident hour,day,month,run.
		CSE_EF( doBegIvl())		// start-interval ( Top.ivl) processing, next.
								// sets much, wthr data, exprs, solar calcs, etc.
		AH* ah; RLUP( AhB, ah)   CSE_EF( ah->begSubhr() )			// start-subhour air handler stuff.  cnah.cpp
		TU* tu; RLUP( TuB, tu)   CSE_EF( tu->tu_BegSubhr() )		// start-subhour terminal stuff 7-16-95.  cnztu.cpp
		ZNR* zp; RLUP( ZrB, zp)   CSE_EF( zp->zn_BegSubhr1() )		// start subhr init (leaves lagged values)
		DOAS* doas; RLUP( doasR, doas) CSE_EF( doas->oa_BegSubhr() )	// start-subhour DOAS
		IZXRAT* ize; RLUP( IzxR, ize) CSE_EF( ize->iz_BegSubhr() )	// start-subhour zone transfer / airnet
		CSE_EF( DHWSubhr())			// all DHW (including solar water heating)
		RLUP( ZrB, zp)   CSE_EF( zp->zn_BegSubhr2() )
		DUCTSEG* ds; RLUP( DsR, ds) CSE_EF( ds->ds_BegSubhr() )		// start-subhour duct segments
		RSYS* rs; RLUP( RsR, rs) CSE_EF( rs->rs_BegSubhr() )		// start-subhour RSYS HVAC systems

		// subhour loads (see also loadsHourBeg, from doBegIvl)
		CSE_EF( loadsSubhr() )

#if !defined( CRTERMAH)
		// may extrapolate tz if load unchanged
		// subhour terminal/airhandler hvac
		CSE_EF( hvacIterSubhr() )	// does iterative portion of subhour hvac ONLY.
#endif

		// end subhour
		RLUP( XsB, xr)   CSE_EF( xr->x.xs_EndSubhr() )	// surface temp updates, accum conduction totals to zones,
		RLUP( IzxR, ize) CSE_EF( ize->iz_EndSubhr() )	// end-subhour vent, fan power accounting
		RLUP( doasR, doas) CSE_EF( doas->oa_EndSubhr() )	// end-subhour DOAS, fan power accounting
		RLUP( ZrB, zp)   CSE_EF( zp->ztuEndSubhr() )   	// by-zone terminals end-subhour stuff, and zone exhaust fan
		RLUP( RsR, rs)	 CSE_EF( rs->rs_EndSubhr() )	// end-subhour RSYS HVAC, mode management
		RLUP( TuB, tu)   CSE_EF( tu->tu_EndSubhr() )	// terminal end-subhour stuff.  cnztu.cpp.
		RLUP( AhB, ah)   CSE_EF( ah->endSubhr() )		// air handlers end-subhour stuff.  cnah.cpp.
		RLUP( BlrB,blr)  CSE_EF( blr->endSubhr() )		// BOILERs end-subhour stuff: energy use.  cnhp.cpp.
		//RLUP( CpB, cp)   CSE_EF( cp->endSubhr() )		// coolplants end-subhour stuff.  cncp.cpp.
		RLUP( ChB, ch)   CSE_EF( ch->endSubhr() )		// chillers end-subhour stuff.  cncp.cpp.
		RLUP( TpB, tp)   CSE_EF( tp->endSubhr() )		// towerplants end-subhour stuff.  cntp.cpp.

		CSE_E( doEndIvl());	// end interval stuff: update Top.ivl, results accum, reports,etc. local.
							// Also ends hour,day,month,run ending with this subhour.
							// On error, return exact code re ^C detection (rc = MH_C0100)
		if (ebTolSubhr > 0.f)			// if tolerance given for subhourly energy balance check 3-28-92
			if (!isWarmup)				// no energy balance error messages during warmup
				if (!tp_autoSizing)			// tentatively no energy balance checks during autoSize
					cgenbal( C_IVLCH_S, Top.ebTolSubhr);	// check zone energy balance, msg if bad.  cgenbal.cpp.

		// increment to next interval: do after cgenbal.
		//			Top.ivl: doEndIvl updated it.  iHr, jDay: looped by tp_MainSimI.  Month etc etc incr'd by doBegIvl per jDay.
		// 			Top.isEndHour: set in doBegIvl.
		iSubhr++;					// count subhours for hour (0'd in doBegIvl at start hour)
		isBegHour = FALSE;			// no longer 1st subhour of hour (or, if new hour, doBegIvl will set it).
		isBegRun = FALSE;			// no longer 1st subhour of run
		tp_isBegMainSim = FALSE;	// no longer 1st subhour of main simulation
	}
	while (!isEndHour);				// set in doBegIvl.

	return rc;			// many returns above including in E, CSE_EF macros
}		// TOPRAT::tp_SimHour
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC NEAR doBegIvl()	// simulation run start-of-interval processing: init, solar calcs, exprs, etc.

// uses Top.ivl: starting interval: year (run), month, day, hour, subhour,  set in tp_MainSimI/doEndIvl.

// For each interval, also does stuff for smaller intervals: days and hours for month, etc.

// return value: non-RCOK if error, message already issued: terminate run.
{
// CAUTION 3-92: currently Top.iSubhr remains n+1 and .isEndHour remain TRUE til _H case in switch(Top.ivl).

	Top.shoy = (Top.xJDay*24 + Top.iHr)*4;	/* extended subhour of year for recording eg demand peaks.
    						   xJDay is jDay for main sim, special values for autosize design days,
    						   1-based to leave 0 for unset. */


// initialize for this interval.  Independent variables  isWarmup, jDay, and iHr  are init and incr'd by caller.

	switch (Top.ivl)			/*lint -e616  cases fall thru */
	{
	case C_IVLCH_Y:    			// at first call of run or of warmup
		// Top.isFirstDay = TRUE;			restore if vbl needed
		Top.isFirstMon = TRUE;   			// say first month of year

	case C_IVLCH_M:    			// at first call of each month. CAUTION: tp_DoDateDowStuff not yet called.
	case C_IVLCH_D:    			// at first call of each day (per daylight saving time)
		// date, day of week, holiday flags, etc initialization.  Daylight time -- ST variables updated below.
		Top.tp_DoDateDowStuff();
		// CAUTION: Top.iHr may be --'d or ++'d in tp_DoDTStuff in C_IVLCH_H case if DT starts or ends.
		Top.isSolarCalcDay = Top.isBegMonth;     		// do solar calcs on warmup/run/mon 1st day and dsn day 1st rep.
		Top.tp_WthrBegDay();
		cgReportsDaySetup();		// init zones & Top re date-dependent reports and exports.
				     				// set zp-> lists; may print heads.  Uses jDay. cgresult.cpp.

		// monthly things done after tp_DoDateDowStuff (updates Top.tp_date, Top.monStr, etc)
		if (!Top.isWarmup)					// not in warmup (may also exclude autoSizing)
			if (Top.isBegMonth)				// set in tp_DoDateDowStuff
			{
				// monthly screen message. See conditional initial newline in tp_MainSimI.
				if (!Top.tp_autoSizing)
				{
					if (getScreenCol() > 54)			// start new line if line pretty full (12 months = 48 cols)
						screen( QUIETIF, "   ");    			// newline (no NONL) and indent (+ 1 space in each text)
					screen( NONL|QUIETIF, " %s", Top.monStr);	// display month name on current screen line, rmkerr.cpp
				}
#ifdef BINRES
				if (brf)					// if writing binary results file(s) (flag in this file)
					//if (!Top.isWarmup)				tested above	// no results file output during warmup
					//if (!Top.tp_autoSizing)				brf off if autoSizing	// no res file output for ausz yet 6-95
				{
					ASSERT(Top.tp_date.month > 0);		// be sure 0 heat autosize month does not get here, 6-95
					brfw.startMonth(Top.tp_date.month-1);	// start new month. 0-based month arg (cnguts object, fcns in brfw.cpp).
				}
#endif
			}

	case C_IVLCH_H:				// at (first) call of each hour
		Top.isBegDay = (Top.iHr==0);				// beginning of new day indicator
		Top.isEndDay = (Top.iHr==23);				// end ..  for cgwfread.

		Top.isBegHour = TRUE;			// first subhr of hour
		//Top.isEndHour = FALSE;		// not last subhr of hour: is cleared below in subHr case.
		Top.iSubhr = 0;					// init subhour number for hour

		// standard time stuff: DT start/end (alters iHr), Standard Time time/date update
		Top.tp_DoDTStuff();				// update DT/ST stuff, local, below

		// weather data
		CSE_EF( Top.tp_WthrBegHour() );	// read weather file, set Wthr, and Top hourly weather members, eg tDbOHr. cgwthr.cpp.

		CSE_EF( Top.tp_ExshBegHour());	// Penumbra external shading: sets sun position

	default:  // case C_IVLCH_S:	// at start of each subhour, incl longer intervals (fall in)
		Top.isEndHour = (Top.iSubhr + 1 >= Top.tp_nSubSteps);	// set flag if last subHour of hour

#if defined( DEBUGDUMP)
		DbDo( dbdSTARTSTEP);		// nothing yet debug-printed for this subhour
									//   (sets flag re on-demand heading printing)
#endif
		CSE_EF( Top.tp_WthrBegSubhr() ); // set Top subhourly weather members, eg tDbOSh. Mostly interpolated. cgwthr.cpp.

		// LOADMTR: clear subhour values
		LOADMTR* pLM;
		RLUP(LdMtrR, pLM)
			CSE_EF(pLM->lmt_BegSubhr());

		// AFMTR: clear subhour values
		AFMTR* pAM;
		RLUP(AfMtrR, pAM)
			CSE_EF(pAM->amt_BegSubhr());
	}

// import files: read next record as appropriate
	impfIvl( Top.ivl);			// impf.cpp.

// evaluate and stuff user's variable expressions that are or may be calc inputs
	CSE_EF( doIvlExprs( EVBEGIVL) ) // do all start-of-interval expressions for this interval. uses Top.ivl. local, below.

// stuff done hourly (& longer intervals) only, after expressions
	if (Top.ivl < C_IVLCH_S)	// if start of hour or longer interval, not start subhour only
	{
#if defined( DEBUGDUMP)
		Top.tp_SetDbMask();		// debug printout mask (dbdXXX), controls internal val dumping
								//   (assumed not to change subhourly)
#endif
		// masses: init heat flow totals
		//   (after tp_SetDbMask() re correct db printing)
		CSE_EF( msBegIvl( Top.ivl));	// CSE_EF()=insurance, RCOK expected

		// conditional hourly solar gain calculations
		if (Top.isSolarCalcDay) 		// on for 24 hours of first day of month, run [, season].  Set above, cleared doEndIvl.
			makHrSgt();	// calculate solar gains to each target, ready for tp_SimHour to combine with wthr data.
		   				//  Sets SGRAT entries, valid for a month (has hourly storage slots).

		// zone "early" inits -- zone gains etc.
		ZNR* zp;
		RLUP(ZrB, zp)
			CSE_EF( zp->zn_BegHour1())

		// zero meters for hour
		//  must precede DHW
		MTR* mtr;
		RLUP( MtrB, mtr)
			mtr->mtr_HrInit();		// 0 end use values
		DHWMTR* pWM;
		RLUP( WMtrR, pWM)
			CSE_EF( pWM->wmt_Init( Top.ivl) )	// 0s hour values

		// domestic water heating (including solar water heating)
		//   draws and other setup done hourly
		//   most calcs done subhour / subhourtick
		//   must be done before gains due to possible DHW linkage
		CSE_EF( DHWBegIvl( Top.ivl))

		// distrubute GAINs to zones and meters
		CSE_EF( doHourGains() )	// sets hour's znSGain's and znLGain's and accumulates hour's metered energy use from GAIN records.
								// and sets zone qrIgAir's and mass rIg's per zone .rIgDist table, 11-95.

		// photovoltaic arrays
		//   calcs done hourly using PV Watts routines in SAM Simulation Core ssc.dll
		PVARRAY* pv;
		RLUP(PvR, pv) CSE_EF(pv->pv_DoHour())


		// hourly part of loads computation (most of it). Must call after gains.
		CSE_EF( loadsHourBeg() )   		// start-hour loads stuff -- solar gains, masses, zones. cnloads.cpp.

		// hvac hourly init
		TU *tu;
		RLUP( TuB, tu)   CSE_EF( tu->tu_BegHour() )   		// looks for various tu input changes. call after exprs.  cnztu.cpp.
		AH *ah;
		RLUP( AhB, ah)   CSE_EF( ah->begHour() )   		// determines fanFMax.  call after expressions.  cnah.cpp.

	}

// terminate run on too many errors (that continue execution at their source)
	// check here 5-97 cuz traditional check in cnztu.cpp:hvacIterSubhr is bypassed if no hvac.
	if (errCount() > maxErrors)   	// if too many total errors, msg & ret RCBAD.
		// errCount(): error count ++'d by calls to err, rer, etc. rmkerr.cpp.
		return rInfo( 			// display runtime "Information" message, exman.cpp. returns RCBAD.
				   (char *)MH_R1251, 		// "More than %d errors.  Terminating run."
				   (INT)maxErrors );  	// maxErrors: cuparse.cpp. Data init, accessible as $maxErrors.

	return RCOK;		// many error returns above, including E and CSE_EF macros.
}		    // doBegIvl
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC NEAR doEndIvl() 		// simulation run end-of-interval processing: results accumulation, reports, etc

// uses/UPDATES Top.ivl to interval that ended, which is same as interval that starts next.

// does processing required after that interval's simulation: results, end-ivl exprs, reports, etc, etc

// includes shorter intervals -- hour when day ends, etc

// return value: non-RCOK if error, message already issued: terminate run;
//                        MH_C0100 (msghans.h) if interrupted from keyboard.
{

	/* autoSizing design day completion note while under construction 6-95:
	   if caller does not set isLastDay & do extra day rep of design day, never get end-month and end-year stuff,
	      including eg the following, beleived irrelevant anyway at present for autoSizing:
	         monthly egenbal, end-month & end-year exprs and reports, vpRxportsFinish, brfw.endMonth. */

// update Top.ivl to interval that is ending now === interval that will start next
	Top.ivl = C_IVLCH_S;		// assume a subhour
	if (Top.isEndHour)			// if last subhour of hour (set by doBegIvl)
	{	Top.ivl = C_IVLCH_H;
		if (Top.isEndDay)    		// if last hour of day (set by doBegIvl)
		{	Top.ivl = C_IVLCH_D;
			// for autoSizing 6-95 following don't occur unless caller sets isLastDay & does extra rep of design day:
			if (Top.isEndMonth) 		// if last day of main sim month (doBegIvl).  CAUTION: not on for last isWarmup day.
			{	Top.ivl = C_IVLCH_M;
				if (Top.isLastDay)		// if last day of main sim run (doBegIvl)
					Top.ivl = C_IVLCH_Y;
			}
		}
	}

// interval-dependent stuff before exprs/reports

	switch (Top.ivl)
	{

	case C_IVLCH_Y:    		// last call of run
	case C_IVLCH_M:    		// last call of month
	case C_IVLCH_D:    		// last call of day
		// Clear do-solar-calcs-each-hour-today flag
		if (Top.jDay != Top.DTBegDay)	// but if daylight time went on today, 2AM was skipped, repeat it all tommorrow.
			Top.isSolarCalcDay = 0;		// set in doBegIvl, tested in doBegIvl, doIvlExprs.

	//case C_IVLCH_H:   		// (last) call of hour

	default:  ;  //case C_IVLCH_S

	}

// finalize water heating
	if (Top.ivl <= C_IVLCH_H)
		CSE_EF(DHWEndIvl(Top.ivl));


// Mass end-of-interval enthalpy accounting
	msEndIvl( Top.ivl);

// results accumulation for interval: simulation and meters
	doIvlAccum();					// [subhr to hour], hour to day, day to month, month to run as pertinent

	if (Top.ivl < C_IVLCH_S)
	{	// battery stage 0 hourly
		//   *AFTER* meter accum
		//   *BEFORE* EVENDIVL expressions (sets probable values)
		BATTERY* bt;
		RLUP(BtR, bt)
			CSE_EF(bt->bt_DoHour( 0))
	}

// energy balance check.  After results accumulation.
	if (Top.isEndHour) 					// not if end of subhour interval only
	{	if (!Top.isWarmup)    				// if not warming up (may also exclude autoSizing)
		{	if (!Top.tp_autoSizing)				// tentatively no energy balance checks during autoSize
			{	if (Top.ebTolHour > 0.f)			// if tolerance given for hourly check 3-92
					cgenbal(C_IVLCH_H, Top.ebTolHour);    	// check zone and mass energy balance, msg if bad.  cgenbal.cpp.
				if (Top.isEndDay)				// if last hour of day
				{	if (Top.ebTolDay > 0.f)			// if tolerance given for daily check
						cgenbal(C_IVLCH_D, Top.ebTolDay);	// check zone and mass energy balance, msg if bad.  cgenbal.cpp.
					if (Top.isEndMonth)    			// if last day of month (probably never set for autoSizing 6-95)
						cgenbal(C_IVLCH_M, Top.ebTolMon);	// monthly check.
				}
			}
		}
	}

// now evaluate and stuff exprs from user input that are used in reports -- may show results.  Others done at interval start.
	CSE_EF( doIvlExprs( EVENDIVL) )				// do all end-of-interval expressions for this interval. Uses top.ivl.

// post-processing steps (may depend on end-of-interval (*e) values)
	if (Top.ivl < C_IVLCH_S)
	{	// battery stage 1 hourly
		//   *AFTER* EVENDIVL function eval
		BATTERY* bt;
		RLUP(BtR, bt)
			CSE_EF(bt->bt_DoHour(1))
	}

	doIvlFinalize();				// finalize meters etc

	CSE_EF( doIvlExprs( EVPSTIVL))		// do all post load management expressions for this interval

// do reports due for intervals ending now
	doIvlReports();					// uses not-yet-updated Top.iSubhr.

	// copy interval results to prior interval results.
	// Do after end-interval exprs & reports, so 'prior' shows prior even if
	// probed at end interval (might happen in expr also ref'ing cur results? )
	doIvlPrior();					// local fcn, below

// loads/hvac subhour-after-reports stuff, such as setting prior-value variables, that must be deferred cuz of probes.
	loadsAfterSubhr();					// loads (including RSYS), cnloads.cpp
	AH* ah;
	RLUP( AhB, ah)
		ah->ah_AfterSubhr();			// air handlers, cnah.cpp 5-92.
	// RLUP( TuB, tu)   CSE_EF( tu->AfterSubhr() )    	when needed

// stuff done only at end hour (or longer interval), after reports

	if (Top.ivl < C_IVLCH_S)
	{
		// loads/hvac end-hour-after-reports stuff that must be deferred cuz of probes
		//   e.g. setting prior-value variables

		loadsAfterHour();			// cnloads.cpp
		hvacAfterHour(); 			// may be inline {} if nothing to do.
		BATTERY* bt;
		RLUP(BtR, bt)
			bt->bt_DoHour( 2);		// battery wrap-up

		INVERSE* pI;
		RLUP(IvB, pI)
			CSE_EF(pI->iv_Calc(Top.ivl))
		
		// interval-dependent stuff after reports

		switch (Top.ivl)				/*lint -e616  cases fall thru */
		{
		case C_IVLCH_Y:        		// last call of run
			// note weather file is closed by cgDone (so done even after error).
			// notify user if condensation occurred, 5-97:
		{
			const ZNRES* zr;
			RLUP( ZnresB, zr)
			if (zr->ss < ZnresB.n)		// skip last (sum-of-zones) record
			{
				const ZNR& Z = ZrB[zr->ss];
				const ZNRES_IVL_SUB& R = zr->curr.Y;

				if (R.nSubhrLX)	// condensation
					warn( 			// not rWarn: no day/hour etc (at least not til also done for ausz) 5-97
						"Zone '%s': Condensation occurred in %d subhours of run.\n"
						"    Total condensation heat = %g kBtu.",
						zr->name, R.nSubhrLX,
						R.qlX/1000 );				// convert Btu to kBtu
				if (R.nShVentH)		// unhelpful ventilation
					warn("Zone '%s': Unhelpful ventilation heating occurred during %d subhours of run.",
						zr->name, R.nShVentH);

				if (Z.i.znModel != C_ZNMODELCH_CZM)
					continue;		// unmet tracking meaningful for CZM only

				if (R.unMetHrs[ 0] > 0.001f || R.unMetHrs[ 1] > 0.001f)
				{	// some unmet hours (with tolerance 0)
					float avgErr[2];	// average tz excursion, [0]=htg, [1]=clg
					for (int iHC=0; iHC<2; iHC++)
						avgErr[ iHC] = R.unMetHrs[iHC] > 0.f ? R.unMetShDH[iHC] / R.unMetHrs[iHC] : 0.f;

					const char* fmt = "Zone '%s': Temp control outcomes\n"
						"            Miss setpoint (hr)  Avg Excursion (F) Max Excursion (F)  Miss > %0.1f F tol (hr)\n"
						"    Heating      %6.1f            %7.2f           %7.2f            %6.1f\n"
						"    Cooling      %6.1f            %7.2f           %7.2f            %6.1f";

					// "Info" msg to report only
					issueMsg(2+NOSCRN+NOERRFILE, fmt, zr->name, Top.tp_unMetTzTol,
						R.unMetHrs[ 0], avgErr[ 0], R.unMetMaxTD[ 0], R.unMetHrsTol[ 0],
						R.unMetHrs[ 1], avgErr[ 1], R.unMetMaxTD[ 1], R.unMetHrsTol[ 1]);
				}

				// issue warning(s) if unmet hrs per tp_unMetTzTol exceed threshold
				if (R.unMetHrsTol[ 0] > Top.tp_unMetTzTolWarnHrs)
					warn ("Zone '%s': Air temp more than %0.1f F below heating setpoint during %0.1f hours of run",
						zr->name, Top.tp_unMetTzTol, R.unMetHrsTol[ 0]);
				if (R.unMetHrsTol[ 1] > Top.tp_unMetTzTolWarnHrs)
					warn("Zone '%s': Air temp more than %0.1f F above cooling setpoint during %0.1f hours of run",
						zr->name, Top.tp_unMetTzTol, R.unMetHrsTol[ 1]);
			}
			RLUP( AhB, ah)
			if (ah->ahcc.nSubhrsLX)
				warn( "Airhandler \"%s\": Supersaturated air occurred at DX coil \n"
					  "    entry in %ld subhours of run.  A total of %g kBtu \n"
					  "    of condensation heat was added to the entering air.",
					  ah->name, (long)ah->ahcc.nSubhrsLX, ah->ahcc.xLGainYr/1000 );

			DHWHEATER* wh;
			RLUP(WhR, wh)		// primary heaters
				wh->wh_ReportBalErrorsIf();
			RLUP(WlhR, wh)		// loop heaters
				wh->wh_ReportBalErrorsIf();
			DHWSOLARSYS* pSW;
			RLUP(SwhR, pSW)
				pSW->sw_ReportBalErrorsIf();
		}
		vpRxportsFinish(); 		// finish terminate reports/exports, cgresult.cpp: eg be sure cond rpt footers done.

		case C_IVLCH_M:        		// last call of month
			Top.isFirstMon = FALSE;    	// no longer first month of run
#ifdef BINRES
			if (brf)							// if writing binary results file(s)
				//if (!Top.isWarmup)				// no results files during warmup: brf is off 6-95.
				//if (!Top.tp_autoSizing)			// no res file output for ausz yet 6-95: brf is off.
				brfw.endMonth(Top.tp_date.month-1);	// terminate month, 0-based argument
#endif

			//case C_IVLCH_D:         		// last call of day

		default: ;  //case C_IVLCH_H:    	// (last) call of hour

			// hourly (6-8-92) ^C check
			if (enkichk())				// if ^C pressed (envpak.cpp). Only call in program 1-95.
			{
				enkimode( KICLEAR | KIBEEP);		// clear ^C / say beep on ^C
				return err( WRN, (char *)MH_C0100);	// display message, get keypress, rmkerr.cpp: " *** Interrupted *** ".
				// returns MH_C0100 -- callers may test.
			}
		}
	}  //if (Top.ivl < subhr)

// increment to next interval
	// Top.ivl updated above; tp_MainSimI loops iHr and jDay; doBegIvl increments month etc etc per jDay.
	// caller tp_SimHour increments iSubhr and clears isBegHour.
	// Top.isEndHour is set by doBegIvl.

#if 0	// add when need found, 4-92
*// terminate run on too many errors (that continue execution at their source)
*    if (errCount() > maxErrors)	// if too many total errors, msg & ret RCBAD.
*					       // errCount(): error count ++'d by calls to rer, err, etc. rmkerr.cpp.
*					       // maxErrors: cuparse.cpp. Data init, accessible as $maxErrors.
*       return rInfo( "More than %d errors.  Terminating run.", (INT)maxErrors );  	// runtime "Information" message, exman.cpp
#endif

	return RCOK;
}		// doEndIvl
//-----------------------------------------------------------------------------------------------------------
RC FC cgFazInit(	// Perform initialization common to main simulation and autoSizing but not each design day

	int isAusz )	// TRUE = autosize
					// FALSE = main simulation phase.

// inits autosizing and peak-recording stuff in all objects, .

// call after input decoded & run records built.

// Returns RCOK if all ok, or error code, message already issued.
{
	RC rc = RCOK;

	// portion in cgausz.cpp
	rc = asFazInit( isAusz); 					// (empty 7-95)

// per-phase init of objects: these do AUSZ::fazInits (set .px) and addl object-specific stuff
	TU* tu;
	RLUP( TuB, tu)  tu->fazInit(isAusz);		// terminals. eg zero lhMxMx
	AH* ah;
	RLUP( AhB, ah)  ah->fazInit(isAusz);		// air handlers
	HEATPLANT* hp;
	RLUP( HpB, hp)  hp->fazInit(isAusz);		// HEATPLANTs
	RSYS* rs;
	RLUP( RsR, rs) rc |= rs->rs_FazInit( isAusz);

	return rc;
}		// cgFazInit
//-----------------------------------------------------------------------------
RC FC cgRddInit(	// Perform initialization common to main simulation run and each autosize design day.
	int isAusz )	// TRUE for autosize, FALSE for main simulation phase.

// call after input decoded, run records built, and cgFazInit() done.

// Returns RCOK if all ok, else error code, message already issued.
{
	// CAUTION this fcn called BEFORE setting of time/date variables Top.jDay, Top.isWarmup, Top.isDT, etc.

	// note tp_DTInit() called by caller after initing jDay, for warmup then for run, 1995.

	/* re Top.year: always generic non-leap year, beginning per Top.jan1DoW.
	   If a leap year >= 0, tddyi() allows for Feb 29.  For this to work, must find / check / fix
	   all conversions to use same year, including those in input e.g. run start & end month-day. */

	RC rc = RCOK;

// Initialize for location
	CSE_EF( Top.tp_LocInit() )	// calls slinit() w wthr file location data (via Top).
								// inits Locsolar (comments below)
								// CSE_EF: return RCBAD on any non-RCOK return.

// Set up weather file, TDV file, and DESCONDs
//   Do each phase, allowing file or location changes.
//   Messages errors.
	CSE_EF( Top.tp_WthrInit() )	// also uses Top .tp_wfName, .skyModel [,.isDT]

#if 0	// enable if needed
0	// pre-run DHW init
0	DHWSYS* pWS;
0	RLUP(WsR, pWS)
0		rc |= pWS->ws_RddInit();
#endif

// Open import files used in this run, if any
	CSE_EF( impfStart() )					// impf.cpp. related code: cncult4.cpp, cuparse.cpp.

// portion in cgausz.cpp: eg init peak load/plr mbrs (for autosize, may be redone elsewhere each design day iteration)
	CSE_EF( asRddiInit() )					// does AUSZ::, TU::, AH::, rddiInit's.

	ZNR* zp;
#if defined( _DEBUG)
	// Zone UA initialization loop: sum conductance of light exterior surfaces and perimeters
	// calc moved to topSf2() 3-4-2013
	// recalc here and compare results (devel aid)
	double tUA[ 200];
	double tUASpecT[ 200];
	RLUP( ZrB, zp)							// loop zone run records
		tUA[ zp->ss] = tUASpecT[ zp->ss] = 0.;		// init to zero
	XSRAT *xs;
	RLUP( XsB, xs)					// loop XSRAT records
	{	int iz = xs->ownTi;
		if (xs->x.xs_ty != CTMXWALL)  						// only non-light type
		{	if (xs->x.sfExCnd==C_EXCNDCH_SPECT)				// if exposed to specified temp
				tUASpecT[ iz] += xs->x.xs_area * xs->x.xs_uval;	// separate hsu specT lite surf ua sum
			else if (xs->x.sfExCnd==C_EXCNDCH_AMBIENT)			// else if exposed to ambient (not ADJZN nor ADIABATIC, 2-95)
				tUA[ iz] += xs->x.xs_area * xs->x.xs_uval;	    // sum ambient light surf & perimeter UA's to zone->ua.
		}
	}
	RLUP( ZrB, zp)						// loop zone run records
	{	int iz = zp->ss;
		if (frDiff( zp->zn_ua, tUA[ iz]) > .0001
	       || frDiff(zp->zn_uaSpecT, tUASpecT[ iz]) > .0001)
		   warn( "Zone '%s': UA mismatch", zp->name);
	}
#endif

	// Zone initialization loop
	RLUP( ZrB, zp)			// for zp = each (gud) zone, 1 to n. cnglob.h macro.
		zp->zn_RddInit();

	// RSYS init
	RSYS* rs;
	RLUP( RsR, rs)
		rc |= rs->rs_RddInit( isAusz);

// Mass initialization loop
	MSRAT* mse;
	RLUP( MsR, mse)			// for gud masses 1 to n
		mse->ms_RddInit( 70.);

// Kiva initialization loop
	KIVA *kv;
	RLUP(KvR, kv)
		kv->kv_RddInit();

// Apply kiva results to initial boundary conditions
	XSRAT* xr;
	RLUP(XsB, xr)
		rc |= xr->xr_ApplyKivaResults();

// Air handler initialization loop, 4-20-95
	AH *ah;
	RLUP( AhB, ah)
		rc |= ah->rddInit( isAusz);	// do run-or-design-day init for each air handler

	// allocate results records paired with run records
	//   do for both autoSize and run
	//      no accum for autoSize, but code does incidental stores into xxRES
	CSE_EF(ZnresB.AllocResultsRecs(ZrB));	// ZNRES <-> ZNR
	ZNRES* pZR;
	RLUP(ZnresB, pZR)
		pZR->zr_InitPrior();	// provide plausible ZNRES.prior values for use in
								// possible expressions that are evaluated at step beg

	CSE_EF(RsResR.AllocResultsRecs(RsR));	// RSYSRES <-> RSYS

	CSE_EF(DsResR.AllocResultsRecs(DsR));	// DUCTSEGRES <-> DUCTSEG

	CSE_EF(WsResR.AllocResultsRecs(WsR));	// DHWSYSRES <-> DHWSYS

	CSE_EF(AhresB.AllocResultsRecs(AhB));	// AHRES <-> AH

#ifdef BINRES
// Init re binary results output files
	// move call to cgInit (and review code) if file is to persist (stay open) thru autosize and main sim run.
	binResInit(isAusz);				// if in use, opens files and sets local flag brf. local fcn below.
#endif

	return rc;
}                   // cgRddInit
//-----------------------------------------------------------------------------------------------------------
RC FC cgRddDone(	// Perform cleanup done after main sim run and each autoSize design day

	[[maybe_unused]] int isAusz )	// TRUE for autosize, FALSE for main simulation phase

// Returns RCOK or error code, message already issued.
{
// close any open import files. Redundant calls ok. impf.cpp. 2-94.
	impfEnd();

#if 1//6-95 move to cgRddDone from cgMainsim so also done after error
#ifdef BINRES	// cnglob.h
// close binary results files if open
	// move call to cgDone (and review code) if file is to persist (stay open) thru autosize and main sim run.
	binResFinish();			// local fcn below. nop if not in use or called redundantly (if local flag brf clear).
#endif
#endif

// zone and ah results records are not free'd here, but cgRddInit code that allocates them repeats ok anyway, 6-95.

// clean up location data/solar stuff
	Top.tp_LocDone();		// eg free Locsolar. redundant call ok.
	return RCOK;
}                   // cgRddDone
//-----------------------------------------------------------------------------------------------------------
RC FC cgFazDone(	// Perform cleanup common to main simulation and autoSizing but not each design day.

	[[maybe_unused]] int isAusz )	// TRUE for autosize, FALSE for main simulation phase.

// call after cgRddDone.  ALSO call freeRunRecs.
{
	cgWthrClean(DONE);	// close wthr file / clean weather data structures
	return RCOK;		// empty function 7-95.
}		// cgFazDone
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC NEAR cgAfterWarmup()

// Perform initialization done after warmup, before run.

// Returns RCOK if ok.  Caller: tp_MainSimI().
{
	RC rc /* = RCOK*/;

// rewind import files. Files must be long enough for warmup even if run shorter. 2-94.
	rc = impfAfterWarmup();		// impf.cpp

#if 0	// 7-19-2016
x ? Clear results accumulated during warmup
x Believed unnecessary due to firstflg initialization scheme in accumZr
x	ZNRES* pZR;
x	RLUP( ZnresB, pZR)
x		pZR->zr_InitCurr();	// 0 ZNRES.curr
#endif

	return rc;
}                   // cgAfterWarmup
//-----------------------------------------------------------------------------------------------------------
void TOPRAT::tp_DTInit()		// Init Top.isDT, Top.jDayST, Top.iHrST at start warmup or run or autosizing design day.

// Uses Top.tp_autoSizing, Top.jDay.

{
	// called from tp_MainSimI at start warmup and run, or from a cgAuszI callee for each new design day.

	if (DT != C_NOYESCH_YES)		// if no DT to be used in this run
		isDT = 0;					// DT is now off
	else
	{	// note for autoSizing, jDay is already suitably set for month of design day.
		if (DTEndDay >= DTBegDay)  	// if normal (Northern Hemisphere) (for equal dates, isDT = 0 falls out).
			isDT =    jDay > DTBegDay && jDay <= DTEndDay; 	// 1 (exactly) if DT in use at hr 0 of 1st day of run, else 0
		else						// if DT starts later in year than ends: possible in Southern Hemisphere
			isDT =    jDay >  DTBegDay || jDay <= DTEndDay; 	// 1 if DT in use at hr 0 of 1st day of run
	}
	if (isDT)					// if DT in use
	{
		iHrST = 23-1;   			// initial standard time will be 11PM (23) after tp_DoDTStuff ++'s it.
		jDayST = 					// initial standard time date
			tp_autoSizing ? jDay	// autoSizing ST date same cuz no ++ at midnite (days wrap to self!)
		  : jDay > 1      ? jDay-1 	// initial main sim ST date is day before run start
		  :	                365; 	// .. which is Dec 31 (no leap years) if run starts on Jan 1
	}
	// else: DT not in use: 1st tp_DoDTStuff call will init ST variables.
}			// TOPRAT::tp_DTInit
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC NEAR doIvlExprs( 		// evaluate and store user-input variable expressions for interval

	int ivlStage)	// interval stage
					//   EVBEGIVL: beginning
					//   EVENDIVL: end (before e.g. battery and load management, all building values known)
					//   EVPSTIVL: post (after load management etc, e.g. results probe for reports)

// also uses: Top.ivl: year,month,day,hour,subhour: interval starting or ending:
//							exprs whose variabilites are this or shorter are eval'd.
//            Top.isBegMonth:     if on, include monthly-hourly exprs with hourly.

// return value: non-RCOK to end run.  error message(s) already issued.
{

// interval to global for possible probe checking in cueval.cpp
	int ucl;		// which use classes to evaluate
	if (ivlStage == EVBEGIVL)
	{	Top.isBegOf = Top.ivl;
		ucl = ~EVXBEGIVL;		// all bits except end/post
	}
	else
	{	Top.isEndOf = Top.ivl;		// isEndLvl set for both EVENDIVL and EVPSTIVL stages
		ucl = ivlStage;
		if (ivlStage == EVPSTIVL && Top.ivl == C_IVLCH_D)
			Top.tp_tmrSnapshot();		// update probably copies of timing data
	}

	// removed 9-95 cuz no exprs are USOL:
	// if (!Top.isSolarCalcDay)	// unless doing solar calcs today (on for year,month calls + day/hr/subhr if IsBegMonth)
	//    ucl &= ~USOL;		// exclude solar-related exprs for speed (expected to be hourly or monthly-hourly, start of ivl)

// which evaluation frequencies to evaluate

#if 1	// believe following is best if if exEvEvf issues fatal error on forward ref first time. 12-6-91.
	// (makes sure dependency rule is straightforward "backward in file ok")
	// 12-13-91: exEvEvf now reorders; still believe this is ok.

	// note: exEvEvf calls are combined so that expressions are checked for forward references
	// irrespective of variability on first call (can only check on 1st call, when nans still present;
	// 1st runtime call is year call which does all runtime exprs).  Also, single exEvEvf call is faster.

	int evf = 0;
	switch (Top.ivl)		/*lint -e616  cases fall thru -- each interval also does shorter intervals */
	{
	case C_IVLCH_Y:
		evf |= EVFRUN; 	// say evaluate start-of-run or end-of-run varying exprs
	case C_IVLCH_M:
		evf |= EVFMON; 	// each month, evaluate monthly-varying expressions
	case C_IVLCH_D:
		evf |= EVFDAY; 	// .. daily-varying
	case C_IVLCH_H:
		if (Top.isBegMonth)	// if first day of month, run, or season
			evf |= EVFMH; 	// also do monthly-hourly on the 24 hours of this day
		evf |= EVFHR;   	// say eval hourly-varying exprs
	default:
		/*case C_IVLCH_S:*/
		evf |= EVFSUBHR; 	// .. subhourly-varying
	}  /*lint +e616 */

// do exprs

	CSE_EF( exEvEvf( evf, ucl) );	// evaluate user-input expressions whose variability matches evf and whose
									// use class matches ucl, and store results at all locations per tables. exman.cpp.
									// CSE_EF: return RCBAD now on non-RCOK return.


#else	// KEEP: believe following is right if exEvEvf reorders evaluation to handle forward references. unrun.
w	// 12-13-91 exEvEvf now reorders, but not convinced need this with other checks that have been added.
w	// Looks bigger and slower, and how can an EVFMON expr ref an EVFDAY without becoming EVFDAY?
w	/* (if exEvEvf does NOT reorder, this produces hard-for-users-to-understand dependency rule:
w	   "exprs may use expr with less variability, or earlier in file if same variability" ) */
w
w // do exprs other than start and end run varying expressions
w
w    /* note: exEvEvf calls are NOT combined so that expressions are evaluated 1st time
w       in same sequence as when intervals do not coincide, to detect shorter interval references
w       (only checked 1st time, when nan still present; 1st call is year call).
w       Otherwise, dependencies on shorter intervals might not be detected and might unpredictably get prior or curr value. */
w
w    evf = 0;
w    switch (Top.ivl)		/*lint -e616  cases fall thru -- each interval also does shorter intervals */
w	{
w      case C_IVLCH_Y:
w      case C_IVLCH_M:		CSE_EF( exEvEvf( EVFMON, ucl) );	// evaluate monthly-varying exprs for ucl and store where used. exman.cpp
w      case C_IVLCH_D:		CSE_EF( exEvEvf( EVFDAY, ucl) );	// .. daily-varying
w      case C_IVLCH_H:		if (Top.isBegMonth)			// if first day of month
w								CSE_EF( exEvEvf( EVFMH, ucl) );	// do monthly-hourly exprs for 24 hours on 1st day of month/run/season.
w							CSE_EF( exEvEvf( EVFHR, ucl) );		// evaluate hourly-varying exprs
w      case C_IVLCH_S:		CSE_EF( exEvEvf( EVFSUBHR, ucl) );	// .. subhourly-varying
w      default:		;
w	}  /*lint +e616 */
#endif

	Top.isBegOf = Top.isEndOf = 0;		// no longer start nor end of an interval
	return RCOK;
}			// doIvlExprs
//-----------------------------------------------------------------------------
LOCAL RC FC NEAR doHourGains()	// set hour's zone & mass gains & daylighting stuff and metered energy use from GAIN records
// call AFTER expression evaluation as zone gain input members accept hourly expressions
{
	RC rc = RCOK;

// zero mass (radiant) gains
	MSRAT* mse;
	RLUP( MsR, mse)
		mse->inside.rIg = mse->outside.rIg = 0.f;

// loop GAIN records
	GAIN* gn;
	RLUP( GnB, gn)
		rc |= gn->gn_DoHour();

	return rc;
}			// doHourGains
//-----------------------------------------------------------------------------
 /*virtual*/ RC GAIN::RunDup(
	 const record* pSrc,
	 int options /*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	rc |= gn_CkF( 1);
	return rc;
 }		// GAIN::RunDup
//-----------------------------------------------------------------------------
RC GAIN::gn_CkF(
	int options)	// option bits
					//  1: run setup check (all refs s/b good)
					//     else end-of-GAIN check
{
	RC rc = RCOK;

	int bSetup = options & 1;

	const UCH* fs = fStat(); 		// point field status byte array

	// meter linkage
	if (IsSet( GAIN_MTRI))
	{	// meter specified -- gnEndUse required
		if (!IsSet( GAIN_GNENDUSE))
			oer( (char *)MH_S0489);  	// "No gnEndUse given"
	}
	else if (IsSet( GAIN_GNENDUSE))
	{	// gnEndUse w/o gnMeter
		if (!IsSet( GAIN_GNDLFRPOW))	// and no daylighting fraction power given
     									// (permit end use without meter for daylighting, cuz litDmd/litEu
         								// accumulation to zones only happens if eu=lit, 9-94)
			oer( (char *)MH_S0490);  	// "No gnMeter given (required when gnEndUse is given)"
	}
	if (bSetup)
	{	// runtime check of meter reference
		if (mtri)								// if meter given
			rc |= ckRefPt( &MtrB, mtri,"gnMeter");
	}

	// controlling DHWSYS or DHWMTR linkage
	//   DHWENDUSE is optional (defaults to _TOTAL)
	//   but cannot be provided w/o DHWSYSI or DHWMTRI
	int nCtrl = IsSetCount(GAIN_DHWSYSI, GAIN_DHWMTRI, 0);
	if (nCtrl == 0)
	{	if (IsSet(GAIN_DHWENDUSE))
			oer("gnCtrlDHWEndUse cannot be given without gnCtrlDHWSYS or gnCtrlDHWMETER");
	} 
	else if (nCtrl == 2)
		oer("Only 1 of gnCtrlDHWSYS or gnCtrlDHWMETER can be given");

	if (bSetup)
	{	// final (redundant? runtime check of controlling DHWSYS reference
		if (gn_dhwsysi)		// if given
			rc |= ckRefPt( &WsR, gn_dhwsysi, "gnCtrlDHWSYS");
		else if (gn_dhwmtri)		// if given
			rc |= ckRefPt(&WMtrR, gn_dhwmtri, "gnCtrlDHWMETER");
	}

	// check that sum of fractions of heat to zone, plenum, return is not > 1, insofar as not unevaluated expressions

	float t =  ((fs[GAIN_GNFRZN] & (FsSET|FsVAL))==FsSET		// test fraction-to-zone field status
			  ?  0.f 								// if uneval'd expr use 0 here
			  :  gnFrZn )   						// if const given, or defaulted, use actual value
			 + ((fs[GAIN_GNFRPL]  & (FsSET|FsVAL))==FsSET ? 0.f : gnFrPl )	// fraction-to-plenum similarly
			 + ((fs[GAIN_GNFRRTN] & (FsSET|FsVAL))==FsSET ? 0.f : gnFrRtn );	// fraction-to-return similarly
	if (t > 1.f)						// if more than all of gain assigned
		oer( 							// error message, prevents run
			(char *)MH_S0491,			/* "More than 100 percent of gnPower distributed:\n"
							   "    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n"
							   "        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g%s" */
			gnFrZn,  gnFrPl,  gnFrRtn,
			gnFrZn + gnFrPl + gnFrRtn,
			!IsSet( GAIN_GNFRZN)		// if gain to zone not given
			?  strtprintf("\n    (note that gnFrZn is defaulted to %g)",	// insert reminder
						gnFrZn )		// 1.0 (or as changed)
			:  "" );

		// check that sum of radiant plus latent fractions is not > 1, insofar as not unevaluated expressions, 11-95

	t =  ((fs[GAIN_GNFRRAD] & (FsSET|FsVAL))==FsSET				// test fraction radiant field status
			?  0.f 								// if uneval'd expr use 0 here
			:  gnFrRad )   						// if const given, or defaulted, use actual value
			+ ((fs[GAIN_GNFRLAT] & (FsSET|FsVAL))==FsSET ? 0.f : gnFrLat );	// fraction latent similarly
	if (t > 1.f)								// if more than all of gain assigned
		oer( 								// error message, prevents run
				"More than 100 percent of gnPower distributed:\n"		// NEWMS
				"    Total of fractions gnFrRad and gnFrlat exceeds 1.0:\n"	// "100%%" --> "100 percent" 10-23-96.
				"        gnFrRad (%g) + gnFrLat (%g) = %g",			//   100%% printed "1000f" in win16, crashed win32.
				gnFrRad, gnFrLat, 					//   100\% printed bad numbers: used an arg.
				gnFrRad + gnFrLat );

	if (fs[GAIN_GNDLFRPOW] & FsSET)			// if daylighting fraction power given
	{
		if (!gnEndUse)				// if end use not given
			oWarn( (char *)MH_S0507);		// No gnEndUse given when gnDlFrPow given.\n"
											// "    gnEndUse=\"Lit\" is usual with gnDlFrPow."
		else if (gnEndUse != C_ENDUSECH_LIT)	// if end use isn't lighting
			oWarn( (char *)MH_S0508);		// gnEndUse other than \"Lit\" given when gnDlFrPow given.\n"
											// "    gnEndUse=\"Lit\" is usual with gnDlFrPow."
	}
	return rc;
}		// GAIN::gn_CkF
//------------------------------------------------------------------------------------------
RC GAIN::gn_DoHour() const		// derive and apply hourly heat gains
{
	RC rc = RCOK;
	ZNR* zp = ownTi > 0 ? ZrB.p + ownTi : NULL;		// gain's zone record

	// check that not more than 100% of gain is distributed.  runtime check needed as hourly expr input accepted.
	if (gnFrZn + gnFrPl + gnFrRtn > 1.f)	// "For GAIN '%s': More than 100 percent of gnPower distributed:\n"
		rc |= rer( (char *)MH_C0101, 		// "    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n"
				   name,					// "        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g"
				   gnFrZn,  gnFrPl,  gnFrRtn,  gnFrZn + gnFrPl + gnFrRtn );
	if (gnFrRad + gnFrLat > 1.f)						// 11-95
		rc |= rer( "For GAIN '%s': More than 100 percent of gnPower distributed:\n"	// NEWMS. %% --> percent 10-23-96.
				   "    Total of fractions gnFrRad and gnFrlat exceeds 1.0:\n"
				   "        gnFrRad (%g) + gnFrLat (%g) = %g",
				   name, gnFrRad, gnFrLat, gnFrRad + gnFrLat );

	// accumulate zone lighting power before and after daylighting reduction, for binary results file and/or custom reports
#if 0
	if (Top.iHr == 0)
		printf("\nBeg day");
#endif
	double gnPX = gnPower;				// fetch gain power level for modification
	if (gnEndUse==C_ENDUSECH_LIT && zp)	// if gain has "lighting" end use
		zp->znLitDmd += gnPX;		// accumulate zone total lighting "demand" -- b4 daylighting reduction.
          							// This is for binary results file and/or custom reports (via ZNRES).
	if (gn_dhwsysi)
	{	float gal = WsR[ gn_dhwsysi].ws_fxUseMix.wmt_GetByEUX( gn_dhwEndUse);
		gnPX *= gal;
	}
	else if (gn_dhwmtri)
	{	float gal = WMtrR[gn_dhwmtri].curr.H.wmt_GetByEUX( gn_dhwEndUse);
		gnPX *= gal;
	}
	gnPX *= gnDlFrPow;				// reduce power per daylighting or whatever
       								//   (if user gives no gnDlFrPow, default is 1.0)

	if (gnPX != 0.)		// do accounting iff nz result
	{
#if 0 && defined( _DEBUG)
		if (gnPX < 0.)
		{	// neg gain allowed but not expected
			//  flag in debug build for investigation
			orWarn( "gain < 0 (%0.1f)", gnPX);
		}
#endif
		// accumulate DL-reduced energy consumption by meter
		if ( mtri > 0)				// if meter given
			// add the gain to it, reduced by any daylighting fraction (dflt 1.0)
			MtrB.p[ mtri].H.mtr_Accum( gnEndUse, gnPX);

		if (zp)		// if associated zone
		{	if (gnEndUse==C_ENDUSECH_LIT)		// if end use is "lighting", separately accumulate
				zp->znLitEu += gnPX;			// dl-reduced zone lighting energy use (cuz meters not by zone).

			// accumulate DL-reduced sensible/latent gain to zones
			zp->znLGain += gnPX * gnFrZn * gnFrLat;			// total gain * fraction to zone * fraction latent
			zp->znSGain += gnPX * gnFrZn * (1.f - gnFrRad - gnFrLat);	// sensible gain: non-radiant, non-latent part


			// accumulate radiant power to zones and masses using distribution table generated at setup time
			if (zp->rIgDist)
			{	DBL rIg = gnPX * gnFrZn * gnFrRad;	// radiant portion of fraction to zone
				if ( rIg != 0.)					// if any radiant gain
					for (int i = 0; i < zp->rIgDistN; i++)	// loop table entries
					{	RIGDIST* p = zp->rIgDist + i;		// point ith table entry
						if (p->targP)				// insurance
							*p->targP += RIGTARG( rIg * p->targFr);		// add fraction of gain to zone or mass member
						// table contains much addl info for debugging, in case need to repoint here, etc.
					}
			}
		}
	}
	return rc;
}		// GAIN::gn_DoHour
//-----------------------------------------------------------------------------
ZNR* ZNRES::zr_GetZone() const		// zone of ZNRES
{	return ZrB.GetAtSafe( ss);
}		// ZNRES::zr_GetZone
//-----------------------------------------------------------------------------
LI ZNRES::zr_GetRunTotalLI( int fn) const
// return total of ZNRES LI values = cummulative count for run so far
{
	int dt = DType( ZNRES_CURR + ZNRES_SUB_M + fn);
	LI sum = 0;
	if (dt != DTLI)
		err( PERR, "ZNRES::zr_GetRunTotalLI '%s': bad fn = %d",
			name, fn);
	else
	{	sum =   *(LI *)field( ZNRES_CURR + ZNRES_SUB_Y + fn )
			  + *(LI *)field( ZNRES_CURR + ZNRES_SUB_M + fn )
			  + *(LI *)field( ZNRES_CURR + ZNRES_SUB_D + fn )
			  + *(LI *)field( ZNRES_CURR + ZNRES_SUB_H + fn )
			  + *(LI *)field( ZNRES_CURR + ZNRES_SUB_S + fn );
	}
	return sum;
}	// ZNRES::zr_GetRunTotalLI
//-----------------------------------------------------------------------------
void ZNRES::zr_InitPrior()		// initialize curr mbrs
// WHY initialize prior values
//  ZNRES.prior mbrs can be probed in step-beg expressions
//  Provide plausible values here for use in 1st step
//  Subsequent values copied from ZNRES.curr
{
	const ZNR* zp = zr_GetZone();
	prior.S.zr_Init1( 1, zp);
	prior.H.zr_Init1( 1, zp);
	prior.D.zr_Init1( 1, zp);
	prior.M.zr_Init1( 1, zp);
	prior.Y.zr_Init1( 1, zp);

}		// ZNRES::zr_InitPrior
//-----------------------------------------------------------------------------
void ZNRES::zr_InitCurr()		// initialize curr mbrs
// set all curr mbrs to 0 (after warmup at start of sim)
{
	curr.S.zr_Init1();
	curr.H.zr_Init1();
	curr.D.zr_Init1();
	curr.M.zr_Init1();
	curr.Y.zr_Init1();
}		// ZNRES::zr_InitCurr
//=============================================================================
#define ZRi1 nHrHeat							// 1st SI member
#define ZRnI ((oRes(nHrCeilFan) - oRes(nHrHeat))/sizeof(SI) + 1)       // # SI members.
  // SIs: # hours
  //#define ZRnh1 nHrHeat						// 1st "# of hours" in SI members.  unused 12-91.
  //#define ZRnNH ((oRes(nHrCeilFan) - oRes(nHrHeat))/sizeof(SI) + 1)	// # of "# of hours": all the SIs. unused 12-91.
// LIs
#define ZRl1 nIter							// 1st LI member.
#define ZRnL ((oRes(nSubhrLX) - oRes(nIter))/sizeof(LI) + 1)		// # LI members
// floats: all
#define ZRf1 tAir							// 1st of avgs & sum float mbrs
#define ZRnF ((oRes(litEu) - oRes(tAir))/sizeof(float) + 1)      	// total # float members to sum.  
// floats: to average
#define ZRa1 tAir							// 1st float to average in float mbrs.
#define ZRnA ((oRes(wAir) - oRes(tAir))/sizeof(float) + 1) // # of floats to average
// floats: to sum:
#define ZRs1 qCond							// first one
#define ZRnS ((oRes(litEu) - oRes(qCond))/sizeof(float) + 1)		// #
// floats to sum: sensible heat balance (cgenbal.cpp)
#define ZRq1 qCond							// 1st float sum & 1st heat flow.
#define ZRnQ ((oRes(qsMech) - oRes(qCond))/sizeof(float) + 1)     	// # of floats to sum / # heat flows
// floats to sum: latent heat balance
#define ZRw1 qlInfil						// first one
#define ZRnW ((oRes(qlMech) - oRes(qlInfil))/sizeof(float) + 1)	// #
// floats to track min
#define ZRmin1 unMetMaxTD[ 0]
#define ZRminN ((oRes(unMetMaxTD[ 0]) - oRes(unMetMaxTD[ 0]))/sizeof(float) + 1)
// floats to track max
#define ZRmax1 unMetMaxTD[ 1]
#define ZRmaxN ((oRes(unMetMaxTD[ 1]) - oRes(unMetMaxTD[ 1]))/sizeof(float) + 1)
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR doIvlAccum()

// simulation results and metered energy use accumulation for ending intervals for each zone and meter and sums thereof

// uses: Top.ivl: interval subhour, hour, day, month, year.  Coinciding shorter intervals also done.

{
// always accum during warmup / autosize
// re possible expression use and prior values

// accumulate subhour zone and ah results to hour [and subhour sum_of_zones].  There are no subhour meter results nor sums.

	ZNRES* pZR;
	RLUP( ZnresB, pZR)				// loop sim results for zones; last record is for sum of zones.  ancrec.h macro.
	{
#if 0 && defined( _DEBUG)
		if (pZR->curr.H.tAir == 0.)
			printf( "\n'%s' tAir == 0", pZR->name);
#endif
		// generate subhour "balance" sums: these are accumulated, and checked for near 0 by cgenbal.cpp at longer intervals.
		pZR->curr.S.qsBal = VSum<float, double>( &pZR->curr.S.ZRq1, ZRnQ);  	// net sens heat: sum sensible heats. cnguts.h defines.
		pZR->curr.S.qlBal = VSum<float, double>( &pZR->curr.S.ZRw1, ZRnW);  	// net latent heat: sum latent heats.

#if !defined( SUPPRESS_ENBAL_CHECKS) && defined( _DEBUG)
		// debug aid -- check latent balance
		if (fabs(pZR->curr.S.qlBal) > .02f)
		{	float qlAbsSum = VAbsSum<float, double>(&pZR->curr.S.ZRw1, ZRnW);
			if (qlAbsSum > 1.f)
			{	float qlErrF = pZR->curr.S.qlBal / qlAbsSum;
				if (fabs( qlErrF) > 0.0001f)
				{	ZNR* zp = pZR->zr_GetZone();
					zp->orWarn("latent unbal = %.2f Btu (%.5f)",
							pZR->curr.S.qlBal, qlErrF);
				}
			}
		}
#endif

		// accumulate zone sim results subhr to hr: add one ZNRES_IVL_SUB into another. fcn below.
		accumZr(  &pZR->curr.S, &pZR->curr.H,	//   src & dest ZNRES substrs: accumulate zone's subhr to same zone's hour
				  Top.isBegHour,    		//   non-0 if 1st call of hour: init: copy don't add.
				  Top.isEndHour );			//   non-0 if last call of hour: compute averages

#ifdef SHSZ	// if keeping sum-of-subhours results. cnguts.h. untested. undef 6-92 for speed, flexibility.
*     if (pZR->ss < ZnresB.n)							// except for last (sum-of-zones) record
*        accumZr( &pZR->curr.S, &azResp->curr.S, pZR->ss==1, pZR->ss==ZrB.n);	// add zone's subhr results to sum of zones
*	       // note use of regular accum fcn to get unweighted average temp/humidity values.
*	       /* CAUTION: above first-last detection could fail if input language could leave
*	          unused zone subscripts, perhaps after deletion, etc.  If nec, preset a last-zone
*	          flag in zone record during setup, and use a first-iteration flag in loop. */
#endif
	}

	RSYSRES* pRSRSum = RsResR.p + RsResR.n;
	RSYSRES* pRSR;
	RLUP( RsResR, pRSR)						// loop RSYSRES, last is sum
	{	if (pRSR->ss < RsResR.n)
			pRSRSum->curr.S.rsr_Accum( &pRSR->curr.S, pRSR->ss == 1, (pRSR->ss == RsResR.n-1)+0x100);
		pRSR->curr.H.rsr_Accum( &pRSR->curr.S, Top.isBegHour, Top.isEndHour+0x100);	// accumulate subhr to hour
																					//  0x100: limit hrsOn to 1
	}

	DUCTSEGRES* pDSRSum = DsResR.p + DsResR.n;
	DUCTSEGRES* pDSR;
	RLUP(DsResR, pDSR)						// loop DUCTSEGRES, last is sum
	{	if (pDSR->ss < DsResR.n)
			pDSRSum->curr.S.dsr_Accum(&pDSR->curr.S, pDSR->ss == 1, (pDSR->ss == DsResR.n - 1));
		pDSR->curr.H.dsr_Accum(&pDSR->curr.S, Top.isBegHour, Top.isEndHour);	// accumulate subhr to hour
	}

	DHWSYSRES* pWSRSum = WsResR.p + WsResR.n;
	DHWSYSRES* pWSR;
	RLUP(WsResR, pWSR)						// loop DHWSYSRES, last is sum
	{
		if (pWSR->ss < WsResR.n)
			pWSRSum->H.wsr_Accum(&pWSR->S, pWSR->ss == 1, pWSR->ss == WsResR.n - 1);
		pWSR->H.wsr_Accum(&pWSR->S, Top.isBegHour, Top.isEndHour);
	}

	AHRES* ahres;
	RLUP( AhresB, ahres)						// loop simulation results for air handlers. last is sum.
		accumAhr( &ahres->S, &ahres->H, Top.isBegHour, Top.isEndHour);	// accumulate subhr to hour ah sim results, local, below.

	// all-AFMTR subhr total plus accum subhour->hour
	AFMTR* pAMSum = AfMtrR.p + AfMtrR.n;
	AFMTR* pAM;
	RLUP(AfMtrR, pAM)						// loop AFMTRs
	{	if (pAM->ss < AfMtrR.n)
		{	// sum subhours to sum_of_AFMETERs w/o averaging
			pAMSum->S[0].amti_Accum(pAM->S, pAM->ss == 1, pAM->ss == AfMtrR.n - 1, 2);
			pAMSum->S[1].amti_Accum(pAM->S + 1, pAM->ss == 1, pAM->ss == AfMtrR.n - 1, 2);
		}
		// subhour->hour, all AFMTRs (including sum_of_AFMETERs)
		pAM->amt_Accum(C_IVLCH_H, Top.isBegHour, Top.isEndHour);
	}

	LOADMTR* pLMSum = LdMtrR.p + LdMtrR.n;
	LOADMTR* pLM;
	RLUP(LdMtrR, pLM)						// loop LOADMTRs
	{	if (pLM->ss < LdMtrR.n)
			pLMSum->S.lmt_Accum(&pLM->S, pLM->ss == 1, pLM->ss == LdMtrR.n - 1);
		pLM->lmt_Accum(C_IVLCH_H, Top.isBegHour, Top.isEndHour);
	}

	if (Top.ivl > C_IVLCH_H)						// if subhour call, done
		return;

// hour -> day

#ifdef BINRES
#ifdef TWOWV//bfr.h 9-94
	float dlwv = 0.f, dlwv2 = 0.f;			// ET daylighting weather variables
#else
	float dlwv = 0.f;						// ET daylighting weather variable
#endif
	float euClg = 0.f, euHtg = 0.f, euFan = 0.f, euX = 0.f;	// whole-bldg energy uses (all meters)
	if (brf)							// if writing binary results file(s)
		//if (!Top.tp_autoSizing)					// no res file output for ausz yet 6-95: tested above; brf is off.
	{
		// sum selected energy uses for all meters for this hour for brf
		MTR *mtr;
		RLUP( MtrB, mtr)				// loop (good) meter records
		{
			MTR_IVL_SUB* h = &mtr->H;		// point hour stuff for meter
			euClg += h->clg;			// cooling
			euHtg += h->htg + h->hp;		// heat energy use: incl heat pump backup heat
			euFan += h->fan + h->aux;		// fan: include auxiliary
			euX += h->dhw + h->proc + h->lit + h->rcp + h->ext + h->usr1 + h->usr2 + h->pv + h->bt;	// "extra": everything else?
		}
		// set daylighting weather variable(s) for brf
		switch (Wfile.wFileFormat)
		{
			// case ... future differences
		case ET1:	// include 1 case to avoid compiler warning
		default:	// for weather file formats PC, PCdemo,..
			dlwv = Top.radBeamHrAv 	// wv 1: beam (direct) irradiance 9-4-94, Btuh/ft2 === average Btu/ft2
				   * verSun[Top.iHr]	// take component on horizontal surface (cosine incidence angle)
				   * 32.f;		/* convert to luminous foot-candles using constant from BW's faxes of 9-19 and 9-22-94
                			   based on Ross McKluney paper (Egy & Bldgs, 1984) provided by Rob Hitchcock, LBL.*/
#ifdef TWOWV
			dlwv2 = Top.radDiffHrAv 	// 2nd daylighting weather variable: diffuse irradiance, Btuh/ft2===avg Btu/ft2.
					* 36.f;		// convert Btuh/ft2 to luminous foot-candles using constant from same source.
			// no horiz component conversion needed -- diffuse data is already for horiz surface.
#endif
			break;
		}
	}
#endif
	ZNRES* azResp = ZnresB.p + ZnresB.n;   	// last results record is for sum_of record
	RLUP( ZnresB, pZR)				// loop sim results for zones; last record is for sum of zones.  ancrec.h macro.
	{
		ZNRES_IVL_SUB *rh = &pZR->curr.H;
		accumZr(  				// accumulate zone sim results: add one ZNRES_IVL_SUB into another. next.
			rh, &pZR->curr.D,		//   src & dest ZNRES_IVL_SUB substrs: accumulate zone's hour to same zone's day
			Top.isBegDay,    		//   non-0 if 1st call: init (ie copy don't add)
			Top.isEndDay );   		//   last call: compute averages.  Next call is a new "first call".
#ifndef SHSZ	// unless doing via subhours
		if (pZR->ss < ZnresB.n)							// except for last (sum-of-zones) record
			accumZr( rh, &azResp->curr.H, pZR->ss==1, pZR->ss==ZrB.n);   	// add zone's hour results to sum of zones
		/* CAUTION: above first-last detection could fail if input language could leave
		   unused zone subscripts, perhaps after deletion, etc.  If nec, preset a last-zone
		   flag in zone record during setup, and use a first-iteration flag in loop. */
		else									// last (sum-of-zones) record
#else
*       // do this subhrly too?
*       if (pZR->ss==ZnresB.n)							// for last (sum-of-zones) record
#endif
		{
			// determine peak heating and cooling hours for entire building. Get results with probes. Rob 12-2-93.
			// note brfw determines peaks itself from hourly info.
			if (fabs(rh->qcMech) > fabs(Top.qcPeak))
			{
				Top.qcPeak = rh->qcMech;		// total cooling Btu's to zones in this peak hour
				Top.qcPeakH = Top.iHr+1;		// hour 1-24
				Top.qcPeakD = Top.tp_date.mday;	// day of month 1-31
				Top.qcPeakM = Top.tp_date.month;	// month 1-12
			}
			if (rh->qhMech > Top.qhPeak)
			{
				Top.qhPeak = rh->qhMech;		// total heating Btu's to zones in this peak hour
				Top.qhPeakH = Top.iHr+1;		// hour 1-12
				Top.qhPeakD = Top.tp_date.mday;	// day of month 1-31
				Top.qhPeakM = Top.tp_date.month;	// month 1-12
			}
		}
#ifdef BINRES
		if (brf)						// if outputting any binary results this run
			//if (!Top.tp_autoSizing)				// no res file output for ausz yet 6-95: tested above; and brf is off.
		{
			ASSERT(Top.shoy <= (24*365+23)*4U);	// brfw may need changes for autosize extended xJDay shoy values 6-95.
			if (dtStart)			// if daylight saving time just went on (see tp_DoDTStuff())
			{
				// DUPLICATE this hour's data for preceding non-existent hour:
				if (pZR->ss < ZnresB.n)				// output as below but using iHr-1.
					brfw.setHourZoneInfo( pZR->ss-1,
										  Top.tp_date.month-1, Top.tp_date.mday-1, Top.iHr-1,
										  Top.shoy, rh->tAir, rh->qSlr, rh->qsIg + rh->qlIg, rh->qcMech, rh->qhMech,
										  rh->litDmd, rh->litEu );
				else
					brfw.setHourInfo( Top.tp_date.month-1, Top.tp_date.mday-1, Top.iHr-1,
									  Top.shoy,
									  Top.tDbOHrAv, dlwv,				// outdoor temp: use hour average 1-95
#ifdef TWOWV//bfr.h
									  dlwv2,
#endif
									  euClg, euHtg, euFan, euX,
									  rh->qcMech, rh->qhMech,
									  rh->litDmd, rh->litEu );
			}
			if (pZR->ss < ZnresB.n)				// except for last (sum-of-zones) record
				brfw.setHourZoneInfo( 		// output hourly zone info
					pZR->ss-1, 				// 0-based zone number
					Top.tp_date.month-1, Top.tp_date.mday-1, Top.iHr, 	// when
					Top.shoy,					// when as subhour-of-year
					rh->tAir, rh->qSlr,			// data: avg temp, rest are sums
					rh->qsIg + rh->qlIg,		// .. combine latent & sens internal gain
					rh->qcMech, rh->qhMech,		// ..
					rh->litDmd, rh->litEu );	// zone lighting demand & after-dl egy use, 9-94.
			else								// sum-of-zones record: output non-zone info
				brfw.setHourInfo( Top.tp_date.month-1, Top.tp_date.mday-1, Top.iHr, 	// when
								  Top.shoy,					// ..
								  Top.tDbOHrAv, dlwv,				// weather: temp: use hr avg(?); dl wthr vbl 8-94
#ifdef TWOWV//bfr.h
								  dlwv2,				//    second daylighting weather variable 9-94
#endif
								  euClg, euHtg, euFan, euX, 			// selected bldg energy use info 8-94
								  rh->qcMech, rh->qhMech,				// sum-of-zones heat flows (bldg peak detection)
								  rh->litDmd, rh->litEu );		// sum-of-zones ltg dmd & egy use (re peaks... brfw computes svgs)
		}
#endif
	}


	RLUP( RsResR, pRSR)							// loop RSYS simulation results incl sum_of.
		pRSR->curr.D.rsr_Accum( &pRSR->curr.H, Top.isBegDay, Top.isEndDay);		// accumulate day from hour

	RLUP(DsResR, pDSR)							// loop DUCTSYS simulation results incl sum_of.
		pDSR->curr.D.dsr_Accum(&pDSR->curr.H, Top.isBegDay, Top.isEndDay);		// accumulate day from hour

	RLUP(WsResR, pWSR)							// loop DHWSYSRES results incl sum_of.
		pWSR->D.wsr_Accum(&pWSR->H, Top.isBegDay, Top.isEndDay);

	AHRES* allAhres = AhresB.p + AhresB.n;
	RLUP( AhresB, ahres)							// loop air handers simulation results incl sum_of.
	{
		accumAhr( &ahres->H, &ahres->D, Top.isBegDay, Top.isEndDay);		// accumulate hour ah results to day. local, below.
		if (ahres->ss < AhresB.n)						// if not the sum record (last)
			accumAhr( &ahres->H, &allAhres->H, ahres->ss==1, ahres->ss==AhB.n);	// also accumulate each hour to sum_of_ahs
	}

	mtrsAccum( C_IVLCH_D, Top.isBegDay, Top.isEndDay);  	// Meters: finish hour as needed, sum to day

	if (Top.ivl > C_IVLCH_D)			// if hour call, done
		return;

	// day -> month
	RLUP( ZnresB, pZR)								// loop sim results for each zone and sum-of-zones
		accumZr( &pZR->curr.D, &pZR->curr.M, Top.isBegMonth, Top.isEndMonth);	// add day results to month's for each zone & sum-of-zones.
	RLUP( RsResR, pRSR)							// loop RSYS simulation results incl sum_of.
		pRSR->curr.M.rsr_Accum( &pRSR->curr.D, Top.isBegMonth, Top.isEndMonth);		// accumulate month from day
	RLUP(DsResR, pDSR)							// loop DUCTSEG imulation results incl sum_of.
		pDSR->curr.M.dsr_Accum(&pDSR->curr.D, Top.isBegMonth, Top.isEndMonth);		// accumulate month from day
	RLUP(WsResR, pWSR)							// loop DHWSYS simulation results incl sum_of.
		pWSR->M.wsr_Accum(&pWSR->D, Top.isBegMonth, Top.isEndMonth);		// accumulate month from day
	RLUP( AhresB, ahres)							// loop air handers sim results
		accumAhr( &ahres->D, &ahres->M, Top.isBegMonth, Top.isEndMonth);	// accumulate day ah results to month
	mtrsAccum( C_IVLCH_M, Top.isBegMonth, Top.isEndMonth);					// accum METERs, DHWMETERs, AFMETERS, LOADMTRs: day to month
#ifdef BINRES
	if (brf)									// if outputting binary results for this run
		//if (!Top.tp_autoSizing)						// not for ausz yet 6-95: tested above, and brf is off.
	{
		ASSERT(Top.dowh < 8);						// brfw may need recoding for autoSizing dowh values 6-95.
		brfw.setDayInfo( Top.tp_date.month-1, Top.tp_date.mday-1, Top.dowh );      	// output day type 0..6 = Sun..Sat, or 7 = Holiday
	}
#endif
	if (Top.ivl > C_IVLCH_M)							// if day call, done.
		return;

// Month -> year

	RLUP( ZnresB, pZR)							// loop over sim results for zones and sum-of-zones
	{
		ZNRES_IVL_SUB *rm = &pZR->curr.M;
		accumZr( rm, &pZR->curr.Y, Top.isFirstMon, Top.isLastDay);	// add month to year for each zone and sum-of-zones
#ifdef BINRES
		if (brf)						// if outputting any binary results this run
			if (pZR->ss < ZnresB.n)				// except for last (sum-of-zones) record
				brfw.setMonZoneInfo(				// output monthly zone info
					pZR->ss-1, 					// 0-based zone number
					Top.tp_date.month-1,  			// 0-based month number
					rm->tAir, rm->qSlr, 			// data: avg temp, rest are sums
					rm->qsIg + rm->qlIg,			// .. combine latent & sens internal gain
					rm->qcMech, rm->qhMech,			// ..
					rm->qsInfil + rm->qlInfil,		// .. combine latent & sens infiltration
					rm->qCond,						// ..
					rm->qMass + rm->qlAir,			// .. mass: incl latent heat of H2O removed from zn air
					rm->qsIz,						// ..
					rm->litDmd, rm->litEu );		// .. lighting demand and energy use 9-4-94
#endif
	}
	RLUP( AhresB, ahres)							// loop air handers simulation results
		accumAhr( &ahres->M, &ahres->Y, Top.isFirstMon, Top.isLastDay);		// accumulate month ah results to run (year)
	RLUP( RsResR, pRSR)							// loop RSYS simulation results incl sum_of.
		pRSR->curr.Y.rsr_Accum( &pRSR->curr.M, Top.isFirstMon, Top.isLastDay);		// accumulate year from month
	RLUP(DsResR, pDSR)							// loop DUCTSEG simulation results incl sum_of.
		pDSR->curr.Y.dsr_Accum(&pDSR->curr.M, Top.isFirstMon, Top.isLastDay);		// accumulate year from month
	RLUP(WsResR, pWSR)							// loop DHWSYSRES results incl sum_of.
		pWSR->Y.wsr_Accum(&pWSR->M, Top.isFirstMon, Top.isLastDay);
	mtrsAccum( C_IVLCH_Y, Top.isFirstMon, Top.isLastDay);					// accumulate metered energy use: month to year
#ifdef BINRES
	if (brf)						// if outputting any binary results this run
	{
		MTR *mtr;
		RLUP( MtrB, mtr)					    // loop (good) meter records
		brfw.setMonMeterInfo( 				// output monthly energy use info
			mtr->ss-1, 			//  0-based meter subscript
			Top.tp_date.month-1,  		//  0-based month number
			mtr );				//  ptr to entire meter record; brfw uses monthly part
	}
#endif
	//if (Top.ivl > C_IVLCH_Y)							// if month call, done
	//   return;
	// year (end of run) accumulation: nothing to do
}							// doIvlAccum
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR doIvlFinalize()

// finalize meters (and ?) after load management (battery, ) stage at end of interval

// uses: Top.ivl: interval subhour, hour, day, month, year.  Coinciding shorter intervals also done.

{
// always accum during warmup / autosize
// re possible expression use and prior values

	if (Top.ivl > C_IVLCH_H)						// if subhour call, done
		return;

	mtrsFinalize( C_IVLCH_D, Top.isBegDay);  	// sum energy uses to hour's total, and accumulate hour meter use to day. local.

	if (Top.ivl > C_IVLCH_D)					// if hour call, done
		return;

	mtrsFinalize( C_IVLCH_M, Top.isBegMonth);	// accum metered energy: day to month. local,below.

	if (Top.ivl > C_IVLCH_M)					// if day call, done.
		return;

	// accumulate month results to year

	mtrsFinalize( C_IVLCH_Y, Top.isFirstMon);	// accumulate metered energy use: month to year

	//if (Top.ivl > C_IVLCH_Y)					// if month call, done
	//   return;
	// year (end of run) accumulation: nothing to do
}							// doIvlFinalize
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR accumZr(

// Accumulate zone simulation results: add contents of an zone interval results struct into another

	ZNRES_IVL_SUB *res1,    	// source interval results substruct (in ZNRES record)
	ZNRES_IVL_SUB *res2,    	// destination: next longer interval, or same interval for sum-of-zones
	BOO firstflg, 		// If TRUE, destination will be initialized before values are accumulated into it
	BOO lastflg )  		// If TRUE, destination means will be computed after values are accumulated.

{
// if first call, copy source into destination
	if (firstflg)
	{
		memcpy( res2, res1, sizeof(ZNRES_IVL_SUB) );
		res2->n = 0;				// reset call count
	}

// not first, add source into destination
	else
	{
		/* integer members notes 6-92: currently no code sets.  currently all defined are # hours counters.
		   --> may want to either generate at hour level, or 'or' in at subhr-->hr accumulation level. */
		VAccum( &res2->ZRi1, ZRnI, &res1->ZRi1); 	// sum integer values (cnguts.h defines; lib\vecpak.cpp functions)
		VAccum( &res2->ZRl1, ZRnL, &res1->ZRl1); 	// sum long values
		VAccum( &res2->ZRf1, ZRnF, &res1->ZRf1); 	// sum all float values, including members to be averaged
		VAccumMin( &res2->ZRmin1, ZRminN, &res1->ZRmin1);	// track float mins
		VAccumMax( &res2->ZRmax1, ZRmaxN, &res1->ZRmax1);	// track float maxs
	}

// count call in destination
	res2->n++;					// divisor for averages

// on last call, calculate averages members to be averaged (temperature and humidity values)
	if (lastflg)
	{
		float fn = res2->n;				// number of calls summed into res2
		float *fp2 = &res2->ZRa1;		// ptr to 1st average in fV (cnguts.h)
		for (int i = 0; i < ZRnA; i++)
			*(fp2++) /= fn;				// divide each by # of calls
	}
}               // accumZr
//-----------------------------------------------------------------------------
double ZNRES_IVL_SUB::zr_TotAbsSen() const
// sum of absolute values of sensible zone fluxes
// used e.g. as denominator in energy balance error determination
{
	return VAbsSum< float, double>( &ZRq1, ZRnQ);
}		// ZNRES_IVL_SUB::zr_TotAbsSen
//-----------------------------------------------------------------------------
double ZNRES_IVL_SUB::zr_TotAbsLat() const
// sum of absolute values of latent zone fluxes
// used e.g. as denominator in energy balance error determination
{
	return VAbsSum< float, double>( &ZRw1, ZRnW);
}		// ZNRES_IVL_SUB::zr_TotAbsLat
//-----------------------------------------------------------------------------
void ZNRES_IVL_SUB::zr_Init1(
	int options/*=0*/,	// bitwise:
						//	1: after 0-ing, init mbrs to plausible prior values
	const ZNR* zp/*=NULL*/)	// source zone (re options=1)
							// NULL if none (unknown or sum-of-zones)
{
	zr_Zero();		// set all mbrs to 0 (bitwise)
	if (options & 1)
	{	if (zp)
		{	// use consistent values from zone
			tAir = zp->tz;
			tRad = zp->zn_tr;
			wAir = zp->wz;
		}
		else
		{	tAir = tRad = 80.f;
			wAir = Top.tp_refW;
		}
	}
}		// ZNRES_IVL_SUB::zr_Init1
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// RSYSRES: RSYS results
///////////////////////////////////////////////////////////////////////////////
// float members to sum
#define rsysRes(m) offsetof( RSYSRES_IVL_SUB, m)
#define RRs1 hrsOn								// first float member to sum (energies)
#define RRnS ((sizeof( RSYSRES_IVL_SUB) - rsysRes( RRs1))/sizeof(float))
//-----------------------------------------------------------------------------
void RSYSRES_IVL_SUB::rsr_Accum( 		// Accumulate RSYS results

	const RSYSRES_IVL_SUB* src,  	// source: next longer interval, or same interval for sum_of_
	int firstflg, 		// nz: *this will be initialized before values are accumulated into it
	int lastflg )  		// nz: *this end-of-interval calcs as needed
						//     +0x100: limit hrsOn to 1 (for H record)
{
	// first call: copy source into destination
	if (firstflg)
	{	memcpy(this, src, sizeof(RSYSRES_IVL_SUB));
		n = 0;				// reset call count. used?
	}

	// not first: add source into destination
	else
		VAccum( &RRs1, RRnS, &src->RRs1);	// float values not averages
											// add each to corress dest member
	
	// count call in destination
	n++;					// used?

	// last call
	if (lastflg & 0xff)
	{	if (lastflg & 0x100)	// limit hrsOn (for H record only)
			if (hrsOn > 1.f)
				hrsOn = 1.f;
	}
}               // RSYSRES_IVL_SUB::rsr_Accum
//-----------------------------------------------------------------------------
void RSYSRES_IVL_SUB::rsr_SetPrior() const 		// copy to prior
{
	BYTE* d = 
		((BYTE *)this + (offsetof(RSYSRES,prior) - offsetof(RSYSRES,curr)));
	memcpy( d, this, sizeof( RSYSRES_IVL_SUB) );
}	// RSYSRES_IVL_SUB::rsr_SetPrior
//=============================================================================
// 
// ///////////////////////////////////////////////////////////////////////////////
// DUCTSEGRES: DUCTSEG results
///////////////////////////////////////////////////////////////////////////////
// float members to sum
#define ductsegRes(m) offsetof( DUCTSEGRES_IVL_SUB, m)
#define DRs1 qhCond								// first float member to sum
#define DRnS ((ductsegRes(qcLeakLat) - ductsegRes( DRs1))/sizeof(float) + 1)		// number of float members to sum
//-----------------------------------------------------------------------------
void DUCTSEGRES_IVL_SUB::dsr_Accum( 		// Accumulate RSYS results

	const DUCTSEGRES_IVL_SUB* src,  	// source: next longer interval, or same interval for sum_of_
	int firstflg, 		// nz: *this will be initialized before values are accumulated into it
	[[maybe_unused]] int lastflg)  		// nz: *this end-of-interval calcs as needed
						//     +0x100: limit hrsOn to 1 (for H record)
{
	// first call: copy source into destination
	if (firstflg)
	{
		memcpy(this, src, sizeof(DUCTSEGRES_IVL_SUB));
		n = 0;				// reset call count. used?
	}

	// not first: add source into destination
	else
		VAccum(&DRs1, DRnS, &src->DRs1);	// float values not averages
											// add each to corress dest member

	// count call in destination
	n++;					// used?

#if 0
	// last call
	if (lastflg & 0xff)
	{
		if (lastflg & 0x100)	// limit hrsOn (for H record only)
			if (hrsOn > 1.f)
				hrsOn = 1.f;
	}
#endif
}               // DUCTSEGRES_IVL_SUB::dsr_Accum
//-----------------------------------------------------------------------------
#if 0		// future: if needed, 11-21
// add suitable calls to ::doIvlPrior()
void DUCTSEGRES_IVL_SUB::dsr_SetPrior() const 		// copy to prior
{
	BYTE* d =
		((BYTE*)this + (offsetof(DUCTSEGRES, prior) - offsetof(DUCTSEGRES, curr)));
	memcpy(d, this, sizeof(DUCTSEGRES_IVL_SUB));
}	// DUCTSEGRES_IVL_SUB::dsr_SetPrior
#endif
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// air handler results
///////////////////////////////////////////////////////////////////////////////
#define ARl1 nSubhr							// first li member. summed.
#define ARnL ((oaRes(nIterFan) - oaRes(nSubhr))/sizeof(LI) + 1)		// number of li members.
#define ARf1 tDbO							// first of all float members, incl avgs and sums
#define ARnF ((oaRes(hrsOn) - oaRes(tDbO))/sizeof(float) + 1)		// total number of float members
#define ARa1 tDbO							// first float member to average (temps, flows)
#define ARnA ((oaRes(vf) - oaRes(tDbO))/sizeof(float) + 1)		// number of float members to average
#define ARs1 qh								// first float member to sum (energies)
#define ARnS ((oaRes(hrsOn) - oaRes(qh))/sizeof(float) + 1)		// number of float members to sum

//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR accumAhr( 		// Accumulate air handler simulation results

// add contents of an ah interval results struct into another, with time-on-weighting for average values

	AHRES_IVL_SUB *ahres1,    	// source interval results substruct (in AHRES record)
	AHRES_IVL_SUB *ahres2,    	// destination: next longer interval, or same interval for sum_of_ahs
	BOO firstflg, 		// If TRUE, destination will be initialized before values are accumulated into it
	BOO lastflg )  		// If TRUE, destination means will be computed after values are accumulated.

{

// if first call, copy source into destination, weigh averages in place
	if (firstflg)
	{
		memcpy( ahres2, ahres1, sizeof(AHRES_IVL_SUB) );
		ahres2->n = 0;					// reset call count. used?
		float *fp2 = &ahres2->ARa1;			// point 1st average in destination
		float t = ahres1->hrsOn;          		// weighting factor: time interval: sigma subhrDur from source
		for (int i = 0;  i++ < ARnA; )
			*(fp2++) *= t;				// averages: times interval; divided by total time at end.
	}

// not first, add source into destination
	else
	{
		/*lint -e507 (for offsetof's in macros) */
		VAccum( &ahres2->ARl1, ARnL, &ahres1->ARl1); 	// sum longs, even when ah off: count iterations involved in deciding

		if (ahres1->hrsOn != 0.)				// if hrsOn is 0 (indicating ah off), skip floats: they will all be 0.
		{
			float *fp1 = &ahres1->ARa1, *fp2 = &ahres2->ARa1;	// ptrs to 1st average in src and dest (cnguts.h defines)
			float t = ahres1->hrsOn;          			// weighting factor: time interval: sigma subhrDur from source
			for (int i = 0;  i++ < ARnA; )
				*(fp2++) += *(fp1++) * t;				// averages: times interval & add

			VAccum( &ahres2->ARs1, ARnS, &ahres1->ARs1); 	// float values not averages: add each to corress dest member
		}
	}  /*lint +e507 */

// count call in destination
	ahres2->n++;					// used?

// on last call, calculate averages for temperature and cfm values
	if ( lastflg
			&&  ahres2->hrsOn != 0.)			// not if accumulated time is 0: all values already 0 and would divide by 0.
	{
		float t = ahres2->hrsOn;			// divisor is accumulated time in destination
		float *fp2 = &ahres2->ARa1;		// ptr to 1st float to average (cnguts.h)
		for (int i = -1; ++i < ARnA; )
			*(fp2++) /= t;			// divide each by total time (each value mult by its own time as accumulated)
	}
}               // accumAhr
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR mtrsAccum( 	// Accumulate metered results: add interval to next, + tot and sum.
								// acts on METERs, DHWMTRs, LOADMTRs, and AFMTRs
	IVLCH ivl,		// destination interval: day/month/year.  Accumulates from hour/day/month.  Not Top.ivl!
	int firstflg, 	// iff TRUE, destination will be initialized before values are accumulated into it
	int lastflg)	// iff TRUE, destination will be finalized (e.g. averages computed)
					//   Note also mtrsFinalize()

// Not called with ivl = C_IVLCH_H
{
	MTR* mtr;				// a meter record
	int firstRec = 1;
	RLUP( MtrB, mtr)		// loop (good) meter records
	{	
#if defined( _DEBUG)
		int bTrc = 0; // strMatch( mtr->name, "ElecMtrInitNo");
		if (bTrc)
		   printf( "\nAccum Day=%d  hr=%d  mtr='%s' ivl=%d  ff=%d", Top.jDay, Top.iHr, mtr->name, ivl, firstflg);
#endif
		
		MTR_IVL_SUB* mtrSub2 = &mtr->Y + (ivl - C_IVLCH_Y);	// point destination meter interval substruct for interval
												// ASSUMES MTR interval members ordered like DTIVLCH choices
		MTR_IVL_SUB* mtrSub1 = mtrSub2 + 1;		// source: next shorter interval

		// if hour-to-day call, compute total use, demand, and costs, then generate hour sum-of-uses record.

		if (ivl==C_IVLCH_D)
		{
			// compute hour's total load in each record.  .allEU then propogates to D, M, Y.
			//  sum members .clg .. usr2 to .allEU, exclude .pv and .bt
			mtrSub1->allEU = VSum<float,double>( &mtrSub1->clg, NENDUSES-2);  	

			// compute sum of uses record (last record).  .sum record then propogates to D, M, Y.
			if (mtr->ss < MtrB.n)   				// don't add the sum record into itself
			{	MTR_IVL_SUB& mtrSum = MtrB.p[MtrB.n].H;
				mtrSum.mtr_Accum1( mtrSub1, ivl, 0+(firstRec!=0));
			}
			firstRec = 0;
		}

		// accumulate: copy on first call (in lieu of 0'ing destination).
		// Note: doHourGains 0's MTR hour info at start hour.
		mtrSub2->mtr_Accum1( mtrSub1, ivl, 0 + (firstflg!=0));
	}

	// LOADMTRs
	//   Accumulate all (including last record = "sum_of_loadmeters")
	LOADMTR* pLM;
	RLUP(LdMtrR, pLM)
		pLM->lmt_Accum(ivl, firstflg, lastflg);

	// DHWMTRs
	DHWMTR* pWM;
	RLUP( WMtrR, pWM)
		pWM->wmt_Accum( ivl, firstflg);

	// AFMTRs
	AFMTR* pAM;
	RLUP(AfMtrR, pAM)
	{	// if (pAM->ss < AfMtrR.n)
			pAM->amt_Accum(ivl, firstflg, lastflg);
	}
}               // mtrsAccum
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR mtrsFinalize( 	// Finalize meters (after post-stage calcs e.g. battery)

	IVLCH ivl,		// destination interval: day/month/year.  Accumulates from hour/day/month.  Not Top.ivl!
	int firstflg ) 	// If TRUE, destination will be initialized before values are accumulated into it
{
	MTR* mtr;				// a meter record
	int firstRec = 1;
	RLUP( MtrB, mtr)					// loop (good) meter records
	{
#if defined( _DEBUG)
		int bTrc = 0;   // strMatch( mtr->name, "ElecMtrInitNo");
		if (bTrc)
		   printf( "\nFinal Day=%d  hr=%d  mtr='%s' ivl=%d  ff=%d", Top.jDay, Top.iHr, mtr->name, ivl, firstflg);
#endif

		MTR_IVL_SUB* mtrSub2 = &mtr->Y + (ivl - C_IVLCH_Y);	// point destination meter interval substruct for interval
												// ASSUMES MTR interval members ordered like DTIVLCH choices
		MTR_IVL_SUB* mtrSub1 = mtrSub2 + 1;		// source: next shorter interval

		// if hour-to-day call, compute total use, demand, and costs, then generate hour sum-of-uses record.

		if (ivl==C_IVLCH_D)
		{
			// compute hour's total use in each record.  .tot then propogates to D, M, Y.
			mtrSub1->tot = mtrSub1->allEU + mtrSub1->pv + mtrSub1->bt;

			// init hour's demand info in each record.  rob 11-93.
			mtrSub1->dmd = mtrSub1->tot;		// total use this hour, copy for demand logic
			mtrSub1->dmdShoy = Top.shoy;		// date & time as subhour of year

			// compute costs per rates. rob 11-93.
			mtrSub1->cost = mtrSub1->tot * mtr->rate;
			mtrSub1->dmdCost = mtrSub1->dmd * mtr->dmdRate;

#if defined( _DEBUG)
			if (!Top.isWarmup)
				// small errors seen during autosize, not understood
				// disable check, investigate TODO, 3-2022
				mtrSub1->mtr_Validate(mtr, ivl);
#endif

			// compute sum of uses record (last record).  .sum record then propogates to D, M, Y.
			if (mtr->ss < MtrB.n)   				// don't add the sum record into itself
			{	MTR_IVL_SUB& mtrSum = MtrB.p[MtrB.n].H;
				if (firstRec)
				{	mtrSum.tot = mtrSub1->tot;
					mtrSum.bt = mtrSub1->bt;
					mtrSum.dmd = mtrSub1->dmd;
					mtrSum.dmdShoy = mtrSub1->dmdShoy;
					mtrSum.cost = mtrSub1->cost;
					mtrSum.dmdCost = mtrSub1->dmdCost;
				}
				else								// additional records: accumulate ...
					mtrSum.mtr_Accum1( mtrSub1, ivl, 2);			// treatment of demand not necessarily sensible.
#if defined( _DEBUG)
				if (!Top.isWarmup)
					mtrSub1->mtr_Validate(mtr, ivl);
#endif
			}
			firstRec = 0;
		}


#if 0 && defined( _DEBUG)
		// MTR_IVL_SUB mtrSub2Was( *mtrSub2);
		// if (bTrc)
		{	float xTot = VSum<float,double>( &mtrSub1->clg, NENDUSES);
			if (frDiff( xTot, mtrSub1->tot) > .001)
				printf( "\n    Bad sub1");
		}

#endif

	// accumulate: copy on first call (in lieu of 0'ing destination).
	//   handles dmd
	// Note: doHourGains 0's MTR hour info at start hour.
		mtrSub2->mtr_Accum1( mtrSub1, ivl, 2 + (firstflg!=0));

#if	0 && defined( _DEBUG)
		// if (bTrc)
		{	float xTot = VSum<float,double>( &mtrSub2->clg, NENDUSES);
			if (frDiff(xTot, mtrSub2->tot, 1.f) > .001)
				printf("\n    Bad sub2 after");	//Top.jDay, Top.iHr);
		}
#endif
	}
}		// mtrsFinalize
//-----------------------------------------------------------------------------------------------------------
RC MTR_IVL_SUB::mtr_Validate(		// validity checks w/ message(s)
	const MTR* mtr,		// parent meter
	IVLCH ivl) const	// interval being checked
// for ad hoc tests of meter validity
// used only in development _DEBUG builds
// not called in RELEASE, 6-20
{
	RC rc{ RCOK };
#if 0
	if (clg < 0.01f && fanC > 0.01f)
		printf("\nMeter '%s': inconsistent cooling fan energy",
			mtrName);
#endif

	char msgs[2000] = { 0 };

	float xTot = VSum<float, double>(&clg, NENDUSES);
	float diff = xTot - tot;
	if (fabs(diff) > 1.f)
		sprintf( msgs, "Tot (%0.1f) != VSum() (%0.1f), diff = %0.1f",
			tot, xTot, diff);

	xTot = allEU + pv + bt;
	diff = xTot - tot;
	if (fabs(diff) > 1.f)
		strCatIf( msgs, sizeof( msgs), "\n    ",
			strtprintf( "Tot(% 0.1f) != allEU + pv + bt (% 0.1f), diff = % 0.1f",
				tot, xTot, diff));

	if (msgs[ 0])
		rc |= mtr->orWarn( msgs);

	return rc;
}		// MTR_IVL_SUB::mtr_Validate
//-----------------------------------------------------------------------------------------------------------
void MTR_IVL_SUB::mtr_Accum1( 	// accumulate of one submeter-interval into another

	const MTR_IVL_SUB* mtrSub1,	// source interval usage/demand/cost substruct in MTR record
	IVLCH ivl,					// destination interval: day/month/year.  Accumulates from hour/day/month.  Not Top.ivl!
	int options /*=0*/)			// option bits
								//   1: copy to *this (re firstflg)
								//      else accumulate
								//   2: do "finalize" accumulation (after batttery calcs)
								//      else end-of-calc
// accums/copies mtrSub1 into *this
{
	bool bCopy     = (options & 1) != 0;
	bool bFinalize = (options & 2) != 0;

	if (!bFinalize)
	{	// end of calcs / before battery
		// omit all *p (EVPSTIVL) mbrs (tot, bt, cost, dmd, dmdCost, dmdShoy)
		if (bCopy)
		{	VCopy( &clg, NENDUSES-2, &mtrSub1->clg);
			pv = mtrSub1->pv;
			allEU = mtrSub1->allEU;
		}
		else
		{	VAccum( &clg, 	// add 2nd arg float vector to 1st arg
				NENDUSES-2,			// vector length: # end uses w/o pv and bt
				&mtrSub1->clg);		// source
			pv += mtrSub1->pv;
			allEU += mtrSub1->allEU;
		}
	}
	else
	{	// after load management (e.g. battery)
		// handle all mbrs w/ *p variability
		if (bCopy)
			memcpy( this, mtrSub1, sizeof( MTR_IVL_SUB));
		else
		{	tot += mtrSub1->tot;
			bt += mtrSub1->bt;
			cost += mtrSub1->cost;
			if (ivl==C_IVLCH_Y)		// sum .dmdCost if month-to-year (thru month, largest
			                		//    dmdCost is used, not sum, by code just below).
				dmdCost += mtrSub1->dmdCost;

			// keep peak demand, and its cost thru month level
			if (mtrSub1->dmd > dmd)		// if source demand (peak use) greater
			{	dmd  = mtrSub1->dmd;		// update largest hourly demand in destination
				dmdShoy = mtrSub1->dmdShoy;	// update peak date & time (subhr of year)
				if (ivl != C_IVLCH_Y)			// thru month level, keep largest demand cost
					dmdCost = mtrSub1->dmdCost;	// .. (but for month to year, dmdCost is summed, above).
			}
		}
	}
}		// MTR_IVL_SUB::mtr_Accum1
//-----------------------------------------------------------------------------------------------------------
double MTR_IVL_SUB::mtr_NetBldgLoad() const	// building load (includes PV, excludes BT)
// valid only AFTER mtrs mtrsAccum has been called
{
	return allEU + pv;
}		// MTR_IVL_SUB::mtr_NetBldgLoad
//-----------------------------------------------------------------------------------------------------------
void MTR::mtr_HrInit()			// init prior to hour accumulation
{
	memset( &H.tot, 0, (NENDUSES+2)*sizeof(float));
						// zero H.tot, H.clg, H.htg, H.hp, H.shw, ..., H.allEU
       					// ASSUMES the NENDUSES end use members follow .tot.
						//         and .allEU follows last end use (=.pv)
}		// MTR::mtr_HrInit


///////////////////////////////////////////////////////////////////////////////
// LOADMTR_IVL, LOADMTR: accumulates heating and cooling loads
///////////////////////////////////////////////////////////////////////////////
/*static*/ const size_t LOADMTR_IVL::lmt_NFLOAT
		= (sizeof(LOADMTR_IVL) - offsetof(LOADMTR_IVL, qHtg)) / sizeof(float);
//-----------------------------------------------------------------------------
void LOADMTR_IVL::lmt_Copy(			// copy 
	const LOADMTR_IVL* sIvl,	// source
	float mult /*=1.f*/)		// optional multiplier
{
	if (mult == 1.f)
		memcpy(this, sIvl, sizeof(LOADMTR_IVL));
	else
	{
		lmt_count = sIvl->lmt_count;
		VCopy(&qHtg, lmt_NFLOAT, &sIvl->qHtg, double(mult));
	}
}	// LOADMTR_IVL::lmt_Copy
//-----------------------------------------------------------------------------
void LOADMTR_IVL::lmt_Accum(			// accumulate
	const LOADMTR_IVL* sIvl,		// source
	int firstFlg,				// true iff first accum into this (beg of ivl)
	int lastFlg,				// true iff last accum into this (end of ivl)
	int options /*=0*/)			// option bits
								//   1: use sIvl.lmt_count to scale totals
								//      effectively averages by day for annual values
								//   2: sum only (do not average)
{
	int count = (options & 1) ? sIvl->lmt_count : 1;
	if (firstFlg)
	{
		lmt_Copy(sIvl, float(count));
		lmt_count = count;
	}
	else
	{
		VAccum(&qHtg, lmt_NFLOAT, &sIvl->qHtg, float(count));
		lmt_count += count;
	}

	// LOADMTR contains no averages
	lastFlg;	// unused
}		// LOADMTR_IVL
//-----------------------------------------------------------------------------
RC LOADMTR::lmt_CkF()
{
	return RCOK;
}
//-----------------------------------------------------------------------------
RC LOADMTR::lmt_BegSubhr()		// init at beg of subhr
{
	S.lmt_Clear();		// clear subhour values
	return RCOK;
}		// LOADMTR::lmt_BegSubhr
//-----------------------------------------------------------------------------
LOADMTR_IVL* LOADMTR::lmt_GetLOADMTR_IVL(
	IVLCH ivl)		// interval
{
	return &Y + (ivl - C_IVLCH_Y);
}		// LOADMTR::lmt_GetLOADMTR_IVL
//-----------------------------------------------------------------------------
void LOADMTR::lmt_Accum(
	IVLCH ivl,		// destination interval: hour/day/month/year
					// Accumulates from subhour/day/month.  Not Top.ivl!
	int firstFlg,	// iff TRUE, destination will be initialized before values are accumulated into it
	int lastFlg)	// iff TRUE, destination averages will be computed as needed
{
	LOADMTR_IVL* dIvl0 = lmt_GetLOADMTR_IVL(ivl);	// point destination substruct for interval
													// ASSUMES interval members ordered like DTIVLCH choices
	LOADMTR_IVL* sIvl0 = lmt_GetLOADMTR_IVL(ivl + 1);	// source: next shorter interval

	dIvl0->lmt_Accum(sIvl0, firstFlg, lastFlg);

}		// LOADMTR::lmt_Accum
//=============================================================================

//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR doIvlPrior()		// set prior interval results for ending intervals for each zone and for sum-of-zones.

// uses: Top.ivl: interval subhour, hour, day, month, year.  coinciding shorter intervals also done.

{
// always maintain prior values
// re possible expression use during warmup / autosize

	// subhour results -> prior subhour results
	ZNRES *pZR;
	RLUP( ZnresB, pZR)			// loop over results records for zones; last is for all-zones
#ifdef SHSZ				// if have subhr sum-of-zones results. cnguts.h. undefined 6-92, for speed/flexibility.
*       if (pZR->ss < ZnresB.n)    	// skip last record: keeping no all-zones subhour results
#endif
		setPriorRes( &pZR->curr.S);	// copy zone's subhour results to prior subhour results, for probing during next subhour

	RSYSRES* pRSR;
	RLUP( RsResR, pRSR)
		pRSR->curr.S.rsr_SetPrior();

	if (Top.ivl > C_IVLCH_H)
		return;				// if subhour call, done

// copy sum-of-zones and each zone's hour results to prior hour results
	RLUP( ZnresB, pZR)			// loop over results records for zones; last is for sum-of-zones
		setPriorRes( &pZR->curr.H);	// copy zone's current hour results to prior hour results
	RLUP( RsResR, pRSR)
		pRSR->curr.H.rsr_SetPrior();
	DHWMTR* pWMT;
	RLUP(WMtrR, pWMT)
		pWMT->curr.H.wmt_SetPrior();

	if (Top.ivl > C_IVLCH_D)
		return;				// if hour call, done

// copy sum-of-zones and each zone's day results to prior day results.
	RLUP( ZnresB, pZR)			// loop over results records for zones; last is for sum-of-zones
		setPriorRes( &pZR->curr.D);	// copy this zone's, or sum-of-zones, current day results to prior day results
	RLUP( RsResR, pRSR)
		pRSR->curr.D.rsr_SetPrior();
	RLUP(WMtrR, pWMT)
		pWMT->curr.D.wmt_SetPrior();

	if (Top.ivl > C_IVLCH_M)
		return;				// if day call, done

// copy sum-of-zones and each zone's month results to prior month results.
	RLUP( ZnresB, pZR)			// loop over results records for zones; last is for sum-of-zones
		setPriorRes( &pZR->curr.M);	// copy this zone's month results to prior month results
	RLUP( RsResR, pRSR)
		pRSR->curr.M.rsr_SetPrior();
	RLUP(WMtrR, pWMT)
		pWMT->curr.M.wmt_SetPrior();

	// if (Top.ivl > C_IVLCH_Y)
	//  return;				// if month call, done

// prior run results are not set -- there is nothing that could use them.

}			// doIvlPrior
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR setPriorRes( 	// set "prior" results from "current" results for an interval

	ZNRES_IVL_SUB* resCurr ) 			// pointer to interval's current results, within a ZNRES record
{
	memcpy( (char *)resCurr + (offsetof(ZNRES,prior) - offsetof(ZNRES,curr)),	// offset from .curr to .prior
			resCurr,
			sizeof(ZNRES_IVL_SUB) );
}	// setPriorRes
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR doIvlReports()	// virtual print reports for ending intervals for each zone and for Top

// uses: Top.ivl: interval subhour, hour, day, month, year.  shorter intervals also done.

{
#if defined( AUSZ_REPORTS)
	if (Top.isWarmup && !Top.tp_autoSizing)
		return;
#else
	if (Top.isWarmup)  return;		// no reporting during warmup (may also exclude autoSizing)
	if (Top.tp_autoSizing)  return;	// no reporting during autoSize 7-95.
#endif

	if (Top.shrxFlg)  			// if this zone/top has active subhourly or subhr+hr re/exports (speed check)
		vpRxports(C_IVLCH_S);		// do subhr's line for active SUBHOURLY re/exports, and
          							//   subhr's line for active HOURLY+SUBHOURLY re/exports
	if (Top.ivl > C_IVLCH_H)	// if subhourly call, done
		return;

	if (Top.hrxFlg)     		// if this zone/top has active hourly or subhr+hr reports/exports (speed check)
		vpRxports(C_IVLCH_H);   // do hour's line for active HOURLY re/exports, and
          						//   hour's line for active HOURLY+SUBHOURLY re/exports
	if (Top.ivl > C_IVLCH_D)		// if hourly or subhourly call, done
		return;

	if (Top.dvriD)			// if this zone or top has any active daily reports or exports (speed check)
		vpRxports(C_IVLCH_D);      	// do today's line for all daily reports/exports active today
	if (Top.ivl > C_IVLCH_M)		// if daily, hourly, or subhourly call, done
		return;

	if (Top.dvriM)			// if this zone/top has any active monthly reports/exports (speed check)
		vpRxports(C_IVLCH_M);      	// do month's line for all monthly reports/exports active today
	if (Top.ivl > C_IVLCH_Y)		// if montlhly...subhourly call, done
		return;

	if (Top.dvriY)			// if this zone/top has any yearly reports/exports (speed check)
		vpRxports(C_IVLCH_Y);       	// do all yearly reports/exports.  cgresult.cpp.
}				// doIvlReports
//-----------------------------------------------------------------------------------------------------------
#if 0 // calls & ifdef MEMREP stuff ripped out, 6-95
*     //   CAUTION: don't enable in production version cuz "dirty" re multiple calls or DLL, 10-93.
* void FC cgMemRep(	// Report current memory use to file "cgmem.rep"
*
*    char *s )	// String to print with message
*
* 	// debugging fnc: several conditional calls in this file, some in cgtest.cpp.
* 	// CAUTION: not multi-run or DLL safe: does not close file, nor reset info at new entry, 10-93.
*{
*    static XFILE *memF = NULL;
*    static ULI dmubeg;   	// Dmused at start
*    static ULI dmuold;   	// Dmused at last htMemCk
*
*    if (memF==NULL)						// if first call
*    {	memF = xfopen( "cgmem.rep", O_WT, WRN, FALSE, NULL);
*       dmuold = dmubeg = Dmused;				// init memory use variables (AFTER xfopen dmallocs)
*	 }
*    xfprintf( memF, "\n%-20s Dmused = %-6ld   Dbeg = %-6ld   Dprev = %-+6ld", s, Dmused, Dmused-dmubeg, Dmused-dmuold );
*    dmuold = Dmused;
*}			// cgMemRep
#endif
//-----------------------------------------------------------------------------------------------------------
void TOPRAT::tp_DoDateDowStuff()	// do date, day of week, and holiday stuff for new day

// uses .jDay, .isWarmup (set in tp_MainSimI); .tp_begDay (user input);
//      .autoSizing, .tp_dsDay, .auszMon.
// sets .tp_date, .isBegMonth, .isEndMonth, .isLastDay, .dateStr, .monStr, (used internally)
//      .isHoliday, .isWeekend, etc (for user probes / $variables), more, more.

{
	// called by doBegIvl at start of day

	// note .xJDay set by callers: used to set .shoy early every subhour.

// allocate date string storage once. Small, error unlikely, abort ok. dmpak.cpp.
	if (!dateStr)
		dmal( DMPP( dateStr), max( TDDATELENMAX+1U, strlen("ddd Jan xxx cooling design day xx")), ABT); 	// small, error unlikely, let program abort.

	if (tp_autoSizing)				// set at start setup, cncult2.cpp
	{
		// get autosizing Julian day, month, mday, wday. Preset by callers: (tp_dsDayI), .tp_dsDay, .auszMon, .jDay, (.xJDay).
		tddyi( tp_date, jDay, year);	// convert (caller's solar) Julian date to month-day-wday.
		tp_date.wday = 3;					// change day of week to Wednesday during autosizing.

		// autosizing beginning of month flag, month and date strings
		isBegMonth = isBegRun;					// set for first repetition of a design day
		if (isBegMonth)
		{	if (auszMon)
			{
				strncpy0( monStr, tddMonAbbrev( tp_date.month), sizeof( monStr));		// eg "Jan"
				const char* t;
				if (tp_AuszWthrSource() != TOPRAT_COOLDSMO)		// if autosizing specific day
					t = strtprintf( "%2.2d-%s", 		// format dd-mon
						tp_date.mday, tddMonAbbrev( tp_date.month));
				else
					t = monStr;
				strcat( strcpy( dateStr, t), " cooling design day");	// eg "Jan cooling design day"
			}
			else									// heating design day. Not month-specific.
			{
				strncpy0( monStr, "heating design day", sizeof( monStr));		// inconsistent with cooling just above
				strcpy( dateStr, "heating design day");
			}
		}

		// autosizing end of month and end of run flags
		// isLastDay must be set by looping routine if a day with isLastDay, isEndMonth, ... desired for design day.
		isEndMonth = isLastDay; 				// may never happen!
	}
	else	// main simulation -- not autosizing
	{
		static DOY jdMend;		// end day-of-year of current month, or of run if sooner

		// date: get month, mday, wday; convert to string.  jDay is primary independent variable, set in tp_MainSimI().

		tddyi( tp_date, jDay, year);		// convert current simulation julian date to month-day, tdpak.cpp.
		// sets tp_date.month (1-12), .mday (1-31), .wday (0=Sun).
		tddis( tp_date, dateStr);				// convert to string for rpt hdrs. tdpak.cpp.

		// main sim beginning of month flag, month string, local end of month date

		isBegMonth = (tp_date.mday==1 || jDay==tp_begDay || isBegRun);	// true if 1st day of month, run, or warmup
		if (isBegMonth)						// do on 1st day of mon or run incl warmup, after setting date.
		{
			strncpy0( monStr, tddMonAbbrev( tp_date.month), sizeof( monStr)); // month name string
			jdMend = tddDoyMonEnd( tp_date.month);   		// month's last day
			if (tp_endDay < jdMend && tp_endDay >= jDay) 	// if sooner
				jdMend = tp_endDay;   						// run's last day
		}

		// main sim end of month and end of run flags. Also, isLastWarmupDay is set by tp_MainSimI().

		isEndMonth = (jDay==jdMend && !isWarmup);     	// true if last day of month or of run (BUT not of warmup)
		isLastDay =  (jDay==tp_endDay && !isWarmup); 	// true if last day of run
	}

// workday, holiday, weekend, etc Booleans: for user probes and/or $variables (per tables in cuparse.cpp)

	// possible bug: isBegWeek, isBegWorkWeek may be wrong on first day of run if last warmup day was not preceding date. 5-95.

	// test for observed or true holiday
	HDAY *hd;
	isHoliday = isHoliTrue = 0;
	if (!tp_autoSizing)					// no holidays while autosizing
	{	RLUP( HdayB, hd)				// loop holidays, set up is in cncult4.cpp.
		{	if (jDay==hd->hdDateObs)   isHoliday++;   	// today is observed date of a holiday
			if (jDay==hd->hdDateTrue)  isHoliTrue++;   	// today is true date of a holiday
		}
	}

	// traditional mon-fri weekday/weekend variables
	isWeekend = (tp_date.wday==0 || tp_date.wday==6);	// Sunday or Saturday. Not autoSizing -- always Wednesday.
	isWeekday = !isWeekend;
	BOO yesterWeHol = isWeHol;				// save for today's isBegWeek
	isWeHol = isWeekend || isHoliday;
	isBegWeek = !isWeHol && yesterWeHol;  		// weekDay after non-workDay
	// tp_dsDay set by callers to keep semantics in cnausz.cpp:
	// tp_dsDay = autoSizing   				// .autoSizing set in setup in cncult2.cpp
	//            ?  (tp_dsDayI ? 2 : 1)    // .tp_dsDayI set in each dsn day loop in cnausz.cpp
	//            :  0;						// 2 cooling, 1 heating (tp_dsDayI==0), 0 not autosizing. for $dsDay.
	dowh = tp_autoSizing ? tp_dsDay + 7		// 8 if autosizing heat dsn day, 9 cool, else
		 : isHoliday     ? 7				// 7 holiday
		 :                 tp_date.wday;	// else weekday 0-6, for $dowh (1 added elsewhere)

	// user-definable workweek variables, 5-95
	USI dayBit = tp_autoSizing ? tp_dsDay*256			// workDayMask bit for day: autosizing: 256 heat, 512 cool;
			   : isHoliday     ? 128				    // not autosizing: 128 for any observed holiday;
			   :                 (1 << tp_date.wday); 	// non-holidays: Sun=1, Mon=2, Tu=4, .. Sat=64.
	isWorkDay = (workDayMask & dayBit) != 0;		// 1 if a workday, for $isWorkDay
	BOO yesterNonWork = isNonWorkDay;			// save for isBegWorkWeek
	isNonWorkDay = !isWorkDay;				// for $isNonWorkSay
	isBegWorkWeek = isWorkDay && yesterNonWork;		// workDay after non-workDay, for $isBegWorkWeek
}						// TOPRAT::tp_DoDateDowStuff
//-----------------------------------------------------------------------------------------------------------
void TOPRAT::tp_DoDTStuff()	// update .isDT, possibly .iHr, and STANDARD TIME time & date variables

// uses    .DT, .DTBeg/EndDay, .iHr, .jDay, .tp_date.
// sets    .isDT, .iHrST, .jDayST,
// when DT starts/ends, alters .isDT, .iHr.

{
	// called each hour by doBegIvl, AFTER tp_DoDateDowStuff called

// start/end Daylight Time at appropriate hour
#ifdef BINRES
	dtStart = FALSE;				// init to NOT start hour of daylight time.
#endif
	if (!tp_autoSizing)			// DT does not go on/off while autoSizing a design day, cuz date stays same.
		if (DT==C_NOYESCH_YES)  		// if DT in use this run
		{
			if ( isDT				// if DT on now
					&&  jDay==DTEndDay		// if DT end date
					&&  iHr==3 )			// if 3AM daylight
			{
				isDT = 0;  			// end daylight time
				iHr--;    			// repeat 2AM hour: THIS SIMULATED DAY HAS 25 HOURS.
			}
			else					// DT off now
				if ( jDay==DTBegDay		// if DT start date
						&&  iHr==2    			// if 2AM
						&&  DTBegDay != DTEndDay )	// (if silly user sets dates equal, leave DT off)
				{
					isDT = 1;  			// start daylight time.  isDT SHOULD BE EXACTLY 1.
					iHr++;    			// skip to 3AM: THIS SIMULATED DAY HAS 23 HOURS.
#ifdef BINRES
					dtStart = TRUE;			// say duplicate next hour's binres data. tested in doIvlAccum. 12-31-93.
#endif
				}
		}

// if standard time date changes this hour, handle it
	if (iHr==isDT)			// ST date changes at hour 1 under DT, hour 0 if not DT
	{
		// when ST date changes, it changes TO same date as DT date, already set.
		jDayST = jDay;			// copy julian date (1-365) to standard time copy
		iHrST = 0;				// is first hour of the new standard time day
	}
// increment standard time hour
	else
		iHrST++;
}			// TOPRAT::tp_doDTStuff
//-----------------------------------------------------------------------------------------------------------
void TOPRAT::tp_tmrSnapshot()		// probe-able copies of timing data
// call e.g. daily
{
	tp_tmrInput = tmrCurTot( TMR_INPUT);
	tp_tmrAusz  = tmrCurTot( TMR_AUSZ);
	tp_tmrRun   = tmrCurTot( TMR_RUN);
	tp_tmrTotal = tmrCurTot( TMR_TOTAL);
	// skip TMR_REPORTS: occurs after any possible probe
#if defined( DETAILED_TIMING)
	tp_tmrAirNet = tmrCurTot( TMR_AIRNET);
	tp_tmrAWTot = tmrCurTot( TMR_AWTOT);
	tp_tmrAWCalc = tmrCurTot( TMR_AWCALC);
	tp_tmrCond   = tmrCurTot( TMR_COND);
	tp_tmrKiva   = tmrCurTot( TMR_KIVA);
	tp_tmrBC     = tmrCurTot( TMR_BC);
	tp_tmrZone   = tmrCurTot( TMR_ZONE);
#endif
}		// TOPRAT::tp_tmrSnapshot

///////////////////////////////////////////////////////////////////////////////
// class INVERSE: function inverter
///////////////////////////////////////////////////////////////////////////////
RC INVERSE::iv_CkF()
{
	RC rc = RCOK;

	return rc;

}		// INVERSE::iv_CkF
//----------------------------------------------------------------------------
RC INVERSE::iv_Init()
{
	RC rc = RCOK;

	return rc;

}		// INVERSE::iv_Init
//----------------------------------------------------------------------------
RC INVERSE::iv_Calc(
	IVLCH ivl)
{
	static int nCall = 1;
	RC rc = RCOK;
	if (ivl == C_IVLCH_Y)
	{
		nCall++;
		iv_XEst = nCall * 1.f;
	}

	return rc;

}		// INVERSE::iv_Calc
//============================================================================
#ifdef BINRES
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR binResInit( int isAusz)	// initialize & open binary results (if to be used) at start run

// sets local flag brf to write to bin res files this phase.  Caller 6-95: cgRddInit.
{
	brf = isAusz
			? FALSE				// tentatively main sim only, 6-95
			: ( Top.tp_brs==C_NOYESCH_YES
			  || Top.tp_brHrly==C_NOYESCH_YES );	// local flag merges Top flags for basic & hourly bin res files
	if (brf)
	{
		const char* fName = 					// file name to use, if one is used. brfw.create() changes .ext(s).
			Top.tp_brFileName && *Top.tp_brFileName 	// name given with BinResFileName =, if any (and not "")
				?  Top.tp_brFileName : InputFilePath;   // otherwise input file pathname (ext gets changed)
		if (brfw.create(			// initialize ResfWriter object and open its output file(s) and/or memory block(s)
					(Top.tp_brMem && !Top.tp_brDiscardable)
						?  NULL  :  fName,		// unless using memory output only, pass filename (brfw changes .ext)
					Top.tp_brDiscardable,			// non-0 to return Windows discardable memory blocks AND files
					Top.tp_brs==C_NOYESCH_YES, 		// flag for basic binary results file
					Top.tp_brHrly==C_NOYESCH_YES,		// .. hourly ..
					Top.tp_brMem || Top.tp_brDiscardable, 	// whether options call for memory handle return (Windows only)
					Top.tp_begDay, Top.tp_endDay, Top.jan1DoW,
					Top.workDayMask,			/* work day bits: Sun=1..Sat=64,Hol=128,htDsDay=256,coolDsDay=512:
							   MATCHES brfw & opk encodings (no ausz ds days yet in bin res, 6-95) */
					ZrB.n,					// number of zones
					MtrB.n - 1,				// # ery srcs: -1 to omit sum_of_meters rcd, added by cncult2: topMtr().
					Top.repHdrL, Top.repHdrR,		// report top left and right header texts
					Top.runDateTime, 			// run date & time string, CSE .431, 8-94.
					/* ***** TEMPORARY weather file info for initial DL test graphs ***** 9-4-94
					   Make names match values passed to setHourInfo() in doIvlAccum()!
					   Expect will depend on wf type.
					   Coordinate via tp_WthrInit(xxx called in this fcn about 100 lines up) ***** */
#ifdef TWOWV // bfr.h 9-94
					2, "Est Direct", "Est Diffuse" ) )	// # dl weather vbls and their names. Estimated from CEC wthr file for now.
#else
					1, "Est Direct" ) )  			// # dl weather vbl and its name. Estimated from CEC wthr file for now.
#endif
			brf = FALSE;			// if open failed, msg has been issued and files closed. clear bin res output flag.
		dtStart = FALSE;			// clear binres Daylight Time start hour flag. redundant insurance.
	}
}			// binResInit
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR binResFinish()	// complete and close binary results files at end run. rob 11-93.
{
	// caller 6-95: cgRddDone.

// nop if bin res files not in use
	if (!brf)  return;

// output stuff for each zone. could do at start run.
	ZNR *zp;
	RLUP( ZrB, zp)				// loop zone run records. cnglob.h macro.
	brfw.setResZone( zp->ss-1, 		//  form 0-based zone number from CSE's 1-based subscript
					 zp->name,		//  pass zone name
					 zp->i.znArea );  	//  pass zone conditioned floor area

// output stuff for each energy source
	MTR *mtr;
	RLUP( MtrB, mtr)				// loop (good) meter records
	brfw.setMeterInfo( mtr->ss-1, 		//  0-based meter subscript
					   mtr );		//  ptr to entire record; brfw takes most or all of it for basic file.

// close the file(s)
	brfw.close( 				// close binary results files
#ifdef WINorDLL
		Top.tp_brMem || Top.tp_brDiscardable 	// if options call for memory handle return (Windows only)
			?  hans  :				// pass pointer to our block in which to return handles (or NULL if none)
#endif
			   NULL );				// pass NULL to be sure any handles are free'd, not returned

// say binary results files not in use so no error messages if redudant call occurs. 6-95.
	brf = FALSE;
}			// binResFinish
#endif // BINRES

/***************************************** rest of file is if-outs *****************************************/

#if 0	// not needed with time-on-weighting, 6-15-92
x //=================================================================
x LOCAL void FC NEAR accumAhrShh(
x
x // Accumulate air handler simulation results, subhour to hour only: uses weighted averaging for vbl subhours
x
x     AHRES_IVL_SUB *ahres1,    	// source interval results substruct (in AHRES record)
x     AHRES_IVL_SUB *ahres2,    	// destination: next longer interval, or same interval for sum-of-ahs
x     FLOAT subhrDur,		// fraction weight for this subhour's results; weights in hour should add to 1.
x     BOO firstflg ) 		// If TRUE, destination will be initialized before values are accumulated into it
x     //BOO lastflg )  	arg not needed	// If TRUE, destination means will be computed after values are accumulated.
x
x{
x // if first call, copy source into destination, weight averages
x     if (firstflg)
x     {
x        memcpy( ahres2, ahres1, sizeof(AHRES_IVL_SUB) );	// copy entire subrecord bitwise
x        float *fp2 = &ahres2->ARa1;			// point 1st average in destination
x        for (int i = 0;  i++ < ARnA; )
x           *(fp2++) *= subhrDur;				// averages: times fraction of hour so result is weighted average.
x        ahres2->n = 0;					// reset call count / subhour count (used here?)
x	  }
x
x // not first, add source into destination, doing weighted average for average members (temperatures and flows)
x     else
x     {
x        float *fp1 = &ahres1->ARa1, *fp2 = &ahres2->ARa1;	// ptrs to 1st average in src and dest (cnguts.h defines)
x        for (int i = 0;  i++ < ARnA; )
x           *(fp2++) += *(fp1++) * subhrDur;		// averages: times fraction of hour & add, so result is weighted avg.
x
x        VAccum( &ahres2->ARs1, ARnS, &ahres1->ARs1); 	// float values other than averages: add each to corress dest member
x
x        VAccum( &ahres2->ARl1, ARnL, &ahres1->ARl1); 	// add each long value to corress destination member (cnguts.h defines)
x	  }
x
x // count call in destination
x     ahres2->n++;					// subhour count.  make use of it?
x
x     // subhrDur's are assumed to add to 1.0 so final weighted average(s) result without a final division.
x}               // accumAhrShh
#endif

// cnguts.cpp end
