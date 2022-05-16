// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnausz.cpp - autoSizing routines for CSE, 6-95

/* DEBUGGING PRINT 6-97 on Top.verbose = 3, 4, 5 ...
   Format very add hoc, info shown just enough to solve immediate problem, add as needed.
      verbose = 3   shows stuff for FIRST item NOT converged in each convergence test
      verbose = 4   shows stuff for EVERY item NOT converged
      verbose = 5   shows stuff for every item in every convergence test
*/

/*
ausz design decisions 6-21-95
   import files: opened phasely. name can change after autosize. rewind each ausz day, when coded.
   reports/exports: want common report file for both phases, so:
     data persists thru phases: not re-setup in cncultn, not free'd in runFazDataFree, unspooled once in cse.

ausz notes
   if Top.isWarmup remains on thru autoSizing, some Top.tp_autoSizing tests may be deletable in cnguts.cpp.
   Currently (6-95) keeping uncommitted; might clear isWarmup for poss future final iteration of des days
   to generate sub/hourly data for reports.
*/


//------------------------------- INCLUDES ----------------------------------
#include <CNGLOB.H>
#include "TDPAK.H"

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// WSHADRATstr ZNRstr

#include <MSGHANS.H>	// MH_

#include <RMKERR.H>	// screen

#include <EXMAN.H>	// rer

#include <CNGUTS.H>	// decls for this file

//-------------------------------- OPTIONS ----------------------------------


//-------------------------------- DEFINES ----------------------------------

//------------------------- FILE-GLOBAL VARIABLES ---------------------------

//----------------------- LOCAL FUNCTION DECLARATIONS -----------------------

LOCAL RC   FC cgAuszI();
// general
LOCAL void callAllAuszs( void( AUSZ::*pvf)(), const char *what=NULL);
// pass 1
LOCAL RC   FC pass1();
LOCAL RC   FC begP1aDsdIter();
LOCAL RC   FC p1aCvgTest();
LOCAL RC   FC p1bCvgTest();
LOCAL RC   FC pass1AtoB();
// pass 2
LOCAL RC   FC pass2();
LOCAL void FC p2Init();	// 7-11-95
LOCAL RC   FC begP2DsdIter();
LOCAL void FC endP2Dsd();
LOCAL RC   FC p2CvgTest();
LOCAL RC   FC p2EndTest();


//===========================================================================================================
//  AutoSizing comments
//===========================================================================================================
// there are more overview comments in cnah1.cpp. merge?
//-----------------------------------------------------------------------------------------------------------
/* Design day story: input specifies a heating outdoor temperature and one or more cooling design days
   (from weather file) identified by month names. The heat design day is constant temperature, no insolation,
   as synthesized in cgwthr.cpp. */
//-----------------------------------------------------------------------------------------------------------
/* Iteration story: day must be iterated until "warmed up": until masses in building model have (nearly)
   reached their steady-state temperatures, etc. Warmup convergence is detected by measuring stability of
   autoSizing results. Results after convergence detected are used -- things being measured are cleared at
   start of each iteration, but building model state (zone and mass temperatures, etc) is not reinitialized. */
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
//  CSE autoSizing functions
//===========================================================================================================
void TOPRAT::tp_ClearAuszFlags()		// clear all autosizing flags
{
	tp_pass1 = tp_pass1A = tp_pass1B = tp_pass2 = tp_sizing = FALSE;
}		// tp_ClearAuszFlags
//-----------------------------------------------------------------------------
const char* TOPRAT::tp_AuszDoing() const	// current autosizing activity
// for e.g. screen display of autosizing progress
// returns string (CAUTION: may be TmpStr)
{
	const char* p = "?";
	if (!tp_dsDayI)
		p = "Heat";
	else
	{	int iDs = tp_dsDayI - 1;
		int iAWS = tp_AuszWthrSource();
		if (iAWS == TOPRAT_COOLDSMO)
			p = tddMonName( tp_coolDsMo[ iDs]);
		else
		{	int auszDoy = 0;
			if (iAWS == TOPRAT_COOLDSCOND)
				auszDoy = DcR[ tp_coolDsCond[ iDs]].dc_GetDOY();
			else if (iAWS == TOPRAT_COOLDSDAY)
				auszDoy = tp_coolDsDay[ iDs];
			if (auszDoy > 0)
				p = tddys( auszDoy);	// e.g. "21-jul"
			// else bug, leave p="?"
		}
	}
	return p;
}		// TOPRAT::tp_AuszDoing
//-----------------------------------------------------------------------------
RC FC cgAusz()		// Autosizing entry point

// inputs are in Top and other globals.
// returns RCOK if ok, other value if error or ^C, message already issued. ^C returns MH_C0100 (msghans.h).
{

// init and do autosizing
	RC rc = cgAuszI();	// call inner fcn: separate fcn so errors can return directly yet do cgrunend().
						// exact rc is returned re ^C detection
	Top.tp_ClearAuszFlags();		// error return insurance

// end phase (autosize or main sim) stuff done even after initialization or run error
	RC rc2 = cgRddDone(TRUE);		// cleanup also done (from tp_EndDesDay) after each design day. Redundant call ok. cnguts.cpp.
	rc2 |= cgFazDone(TRUE);		// cleanup done once after all design days. cnguts.cpp.  Empty fcn 7-95.
	return rc ? rc : rc2;		// return exact rc from cgAuszI re ^C detection
}				// cgAusz
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC cgAuszI()		// Autosizing inner routine

// called once per run; separate fcn so errors can return, not break out of many loops

// inputs are in Top and other globals and run basAncs, mostly in rundata.cpp.
// returns RCOK if ok, other value if error or ^C (returns MH_C0100), message already issued.
// caller: cgAusz
{
	RC rc;
	if (Top.verbose > 0)		// if autoSize will display any detailed screen messages (1 or 2, not 0)
		screen( 0, "   ");		// start new screen line new line (no NONL) and indent (+1 space in each text)
								//   note: QUIETIF and verbose>0 are mutually exclusive
	screen( NONL|QUIETIF, " AutoSizing"); 	// screen msg on current line: beg of warmup.
    							//  If not verbose, continues line started in cse.cpp.
	Top.tp_ClearAuszFlags();

	CSE_EF( cgFazInit(TRUE));	// do initialization done once for all design days:
								// Inits autoSizing & peak recording stuff in all objects.
								// Calls AH::, TU::, HEATPLANT:: etc fazInit's, which call AUSZ::fazInit's
								// (below, sets .az_px's, etc) & do addl object-specific init.

// pass 1: find large enuf sizes, with open-ended models then transition to real model.
	CSE_E( pass1());		// E macro returns exact non-RCOK code on error, re detecting ^C (rc = MH_C0100).

// pass 2: reduce any oversize sizes, or as changed.
	CSE_E( pass2());

	// might be a pass 3 in the future to generate reports or hourly information of any sort.
	// might want some final call to cnguts for month-end, year-end stuff.

// additional stuff after successful autoSize
	TU* tu;
	RLUP( TuB, tu)  rc |= tu->tu_endAutosize();		// eg warnings for probably input errors. cn
	AH* ah;
	RLUP( AhB, ah)  rc |= ah->ah_endAutosize();
	HEATPLANT *hp;
	RLUP( HpB, hp)  rc |= hp->hp_endAutosize();		// heatplants: overload warning msg. cnhp.cpp.
	RSYS* rs;
	RLUP( RsR, rs)	rc |= rs->rs_endAutosize();

// finish: apply oversizes, set _As members and input record members
	// these fcns could be combined with xx_endAutosize fcns
	// ... but might we want to do all the former 1st & exit if error?
	RLUP( TuB, tu)  tu->tu_auszFinal();
	RLUP( AhB, ah)  ah->ah_auszFinal();
	RLUP( RsR, rs)	rs->rs_AuszFinal();

	return rc;
}			// cgAuszI
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
//  CSE autoSizing general functions (as opposed to pass1 or pass 2 fcns)
//===========================================================================================================
RC TOPRAT::tp_BegDesDay()	// init many things before start of repetitions for design day

// uses tp_dsDayI.  calls cgRddInit.

// sets, to remain thru design day repetition: Top .tp_dsDay, .auszMon, .jDay, .xJDay, DT stuff, etc.
// inits, may change during simulation:       Top .isBegRun, .isLastDay, .ivl.
// inits, changed by caller:                  Top.dsDayNIt.

{
	// init des day repetition iteration counter / keep 0 when not iterating
	dsDayNIt = 0;

#if 1	// from below 12-20-2012
	// init derived date/time variables
	// tp_dsDayI already set -- is index of caller's loop.
	tp_dsDay = 1 + (tp_dsDayI != 0);			// 1st desDay is heating (1), others cooling (2).
#endif

	// init simulator to stardard state before each design day -- 70F zone temps, etc.
	RC rc;
	CSE_E( cgRddInit(TRUE));// init repeated for (main sim run or) each autoSize design day, cnguts.cpp.
							// sets initial state of zones & systems; calls AH::rddInit's, etc.
							// also allocs results records not used in autosizing, etc, ... .
							// calls asRddiInit in this file: clears peaks.

#if 0	// moved above 12-20-2012
x	// init derived date/time variables
x	// tp_dsDayI already set -- is index of caller's loop.
x	tp_dsDay = 1 + (tp_dsDayI != 0);			// 1st desDay is heating (1), others cooling (2).
#endif

	if (tp_dsDayI == 0)
	{	// autosizing heating
		auszMon = 0;
		jDay = slrCalcJDays[ 0];	// solar day = december
	}
	else if (tp_AuszWthrSource() == TOPRAT_COOLDSDAY)
	{	// autosizing cooling from specific weather file day
		jDay = tp_coolDsDay[ tp_dsDayI-1];
		tddyi(tp_date, jDay);
		auszMon = tp_date.month;
	}
	else if (tp_AuszWthrSource() == TOPRAT_COOLDSCOND)
	{	const DESCOND* pDC = DcR.GetAtSafe( tp_coolDsCond[ tp_dsDayI-1]);
		jDay = pDC->dc_GetDOY();
		tddyi( tp_date, jDay);
		auszMon = tp_date.month;
	}
	else
	{	// autosizing from monthly design conditions
		//   (only available in ET1 files 12-12)
		auszMon = tp_coolDsMo[ tp_dsDayI-1];	// fetch cooling month 1-12
		jDay = slrCalcJDays[ auszMon];			// Julian date: fetch mid-month value as used for solar calcs.
	}
	// note date.month will be same as auszMon except when that is 0.
	xJDay = 512 + 16 * (tp_dsDayI != 0) + auszMon;	// encoding 512=ausz, 16=cool, + month 1-12 or 0 if generic.
	tp_DTInit();					// initialize isDT. Uses tp_dsDayI.
	isLastWarmupDay = FALSE;		// insurance; believe never set non-0.
	isWarmup = TRUE;	// remains on in autoSize, but coding so isWarmup moot, 6-95.
						//	 Might 0 for poss future extra des day iter for sub/hrly reports, 7-95. */
	isBegRun = TRUE;	// say beginning of run: on for 1st subhr. tp_SimHour clears.

	// causes cnguts to set isSolarCalcDay, .isBegMonth, .
	isLastDay = FALSE;			// makes cnguts clear isEndMonth,

	ivl = C_IVLCH_Y;			// say start of simulation for this design day: be sure all exprs evaluated.
	// many more isXxxx's set by tp_DoDateDowStuff from doBegIvl from tp_SimHour.

	return rc;
}			// TOPRAT::tp_BegDesDay
//-----------------------------------------------------------------------------------------------------------
RC TOPRAT::tp_EndDesDay()	// call when done with design day

// calls cgRddDone: closes weather file, imports, binRes, etc.

{
// keep des day repetition iteration counter 0 when not iterating
	dsDayNIt = 0;		// used eg in exman.cpp::rerIV

// end run stuff: close weather file, import files, binRes, location, etc. cnguts.cpp.
	RC rc = cgRddDone(TRUE);  			// may also be called at completion of phase; redundant call ok.

	return rc;
}			// TOPRAT::tp_EndDesDay
//-----------------------------------------------------------------------------------------------------------
void AUSZ::az_CallPVF(
	void( AUSZ::*pvf)(),			// function
	const char* fmt, ...)	// optional "doing" description
{
	if (Top.verbose > 2 && fmt)
	{	va_list ap;
		va_start( ap, fmt);
		az_doing = (char *)( strtvprintf( fmt, ap));
	}
	else
		az_doing = NULL;
	(this->*pvf)();		// call function
}		// AUSZ::az_CallPVF
//-----------------------------------------------------------------------------
LOCAL void callAllAuszs(		// call one autosize method for all autosizable
	void( AUSZ::*pvf)(),		// member function pointer (method to be called)
	const char* what /*=NULL*/)		// caller's context (for Top.verbose > 2)

// call an AUSZ member function for all AUSZ instances in CSE

// example call: callAllAuszs( AUSZ::fcnName);
{
	const char* whatx = what ? what : "?";		// NULL-proof

	AH* ah;
	RLUP( AhB, ah)		// loop air handlers
	{
		ah->hcAs.az_CallPVF( pvf, "%s Ah[%d] hc", whatx, ah->ss);		// heat coil
		ah->ccAs.az_CallPVF( pvf, "%s Ah[%d] cc", whatx, ah->ss);		// cool coil
		ah->fanAs.az_CallPVF( pvf, "%s Ah[%d] fan", whatx, ah->ss);		// fan(s)
	}

	TU* tu;
	RLUP( TuB, tu)		// loop terminals
	{	tu->hcAs.az_CallPVF( pvf, "%s Tu[%d] hc", whatx, tu->ss);		// heat coil
		tu->vhAs.az_CallPVF( pvf, "%s Tu[%d] vh", whatx, tu->ss);		// air heat max flow
		tu->vcAs.az_CallPVF( pvf, "%s Tu[%d] vc", whatx, tu->ss);		// air cool max flow
	}

	RSYS* rs;
	RLUP( RsR, rs)		// loop RSYS
	{	rs->rs_auszH.az_CallPVF( pvf, "%s RSYS[%s] capH",  whatx, rs->name);
		rs->rs_auszC.az_CallPVF( pvf, "%s RSYS[%s] cap95", whatx, rs->name);
	}

	if (what && Top.verbose > 2)
		screen( 0, "");
}			// callAllAuszs
//-----------------------------------------------------------------------------------------------------------
#ifdef needed
*LOCAL void callAllAuszs( void( AUSZ::*pvf)(BOO aBooleanArg), BOO theBooleanArg)
*
*	// call an AUSZ member function (void, no args) for all AUSZ instances in CSE
*
*	// example call: callAllAuszs( AUSZ::fcnName);
*{
*    AH *ah; TU *tu;
*    RLUP( AhB, ah)		// loop air handlers
*    {	(ah->hcAs.*pvf)(theBooleanArg);	// heat coil
*       (ah->ccAs.*pvf)(theBooleanArg);	// cool coil
*       (ah->fanAs.*pvf)(theBooleanArg);	// fan(s)
*	 }
*    RLUP( TuB, tu)		// loop terminals
*    { 	(tu->hcAs.*pvf)(theBooleanArg);	// local heat coil
*       (tu->vhAs.*pvf)(theBooleanArg);	// air heat max flow
*       (tu->vcAs.*pvf)(theBooleanArg);	// air cool max flow
*	 }
*}		// callAllAuszs
#endif
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
//  CSE autoSizing pass 1 (parts A and B) functions
//===========================================================================================================
LOCAL RC FC pass1()	// do autosizing pass 1: find large enuf sizes, with open-ended models.

// returns RCOK if ok, other value if error or ^C (returns MH_C0100), message already issued.
// called from cgAusz.
{
	RC rc=RCOK;
	Top.tp_pass1 = TRUE;
	Top.tp_sizing = TRUE;				// say sizes can be increased now: tested many places
	if (Top.verbose > 0)				// if autoSize to display detailed screen messages (1 or 2, not 0)
		screen( NONL, " Pass 1");		// continue line started by caller with " AutoSizing"

// loop over design days

	for (Top.tp_dsDayI = 0;  Top.tp_dsDayI < Top.tp_nDesDays;  Top.tp_dsDayI++)			// 0 heating, 1..n cooling
	{
		if (Top.verbose > 0)		// if messages
		{
			if (getScreenCol() > (Top.verbose >= 2 ? 60 : 74))
				screen( 0, "     ");					// conditional newline & indent
			screen( NONL, " %s", Top.tp_AuszDoing());	// display month name or " heat"
		}

		// init cse for repetition of design day
		CSE_EF( Top.tp_BegDesDay());	// calls cgRddInit: inits simulator state, calls asRddiInit, which calls other rddiInit's.
										// inits .tp_dsDay,.auszmon,.jDay,.isBegRun, etc. Uses Top.tp_dsDayI. local.
		// init all AUSZ's to start a pass 1 design day
		callAllAuszs( &AUSZ::begP1Dsd);	// do mbr fcn for all AUSZ's, local.

#if 0 && defined( _DEBUG)
#define DBGPRINT		// controls hard-coded debug printing below
#endif

		// pass 1A for this design day
		Top.tp_pass1A = TRUE;	// say pass1A: idealized open-ended const-supply temp models, finding load
								//  pass1A is "warmup" for this design day
		for ( ; ; )				// iterate design day until converged: until results are stable.
		{
			Top.dsDayNIt++;		// count iterations / 1-based while iterating (0'd in begDesDay)
			if (Top.verbose >= 2)
				screen( NONL," a");
			CSE_EF( begP1aDsdIter());	// init all AUSZ's for new iteration of a pass 1 design day
			CSE_EF( asRddiInit());		// clear peaks, particularly ldPk, for ldPkAs1, for sfan, 8-15-95
			CSE_E( Top.tp_SimDay());	// simulate a day, Sets Top.iHr, uses many things!. Rewinds import files
			// E macro returns exact non-RCOK code on error, re detecting ^C (rc = MH_C0100).
			CSE_EF( p1aCvgTest() );		// test all active AUSZ's for pass 1A convergence. Sets Top.notDone.
			if (!Top.notDone)			// leave loop if done
				break;
#if defined( DBGPRINT)
			if (Top.dsDayNIt > 15)
				Top.tp_dbgPrintMask |= dbdDUCT;		// enable duct dump (shows many interesting values)
#endif
			if (Top.dsDayNIt >= 30)
			{
				rWarn( "AutoSize pass %s has not converged.  Abandoning after %d iterations.",
					   "1A", Top.dsDayNIt );
				break;
			}
		}
#if defined( DBGPRINT)
		Top.tp_dbgPrintMask &= ~128;
#endif
		Top.tp_pass1A = FALSE;			// done pass 1A
		Top.tp_pass1B = TRUE;			// now pass 1B: real models, finding rated capacity

		// pass 1B: continue iterating same design day -- don't warm up again
		CSE_EF( pass1AtoB());	// transition all active AUSZ's to pass 1B simulation
								// Top.tp_pass1A is FAL
		Top.dsDayNIt = 0;		// restart iteration count

		for ( ; ; )			// iterate design day until converged
		{
			Top.dsDayNIt++;		// count iterations / 1-based while iterating (0'd in begDesDay)
			if (Top.verbose >= 2)
				screen( NONL," b");
			CSE_EF( asRddiInit());		// clear peaks, particularly ldPk, for ldPkAs1, for sfan, 8-15-95
			CSE_E( Top.tp_SimDay());	// simulate a day, Sets Top.iHr, uses many things!. Rewinds import files
			// E macro returns exact non-RCOK code on error, re detecting ^C (rc = MH_C0100).
			CSE_EF( p1bCvgTest() );		// test all active AUSZ's for pass 1 part B convergence. Sets Top.notDone.
			if (!Top.notDone)
				break;
#if defined( DBGPRINT)
			if (Top.dsDayNIt > 97)
				Top.tp_dbgPrintMask |= 128;
#endif
			if (Top.dsDayNIt >= 100)
			{
				rWarn( "AutoSize pass %s has not converged. Abandoning after %d iterations.",
					   "1B", Top.dsDayNIt );
				break;
			}
		}
#if defined( DBGPRINT)
		Top.tp_dbgPrintMask &= ~128;
#endif

		// after design day done stuff
		callAllAuszs( &AUSZ::endP1bDsd);	// record new max rated sizes in all active AUSZ's
		CSE_EF( Top.tp_EndDesDay() )		// calls cgRddDone: closes weather file, imports, binRes, etc. local.
	}
	Top.tp_pass1 = Top.tp_pass1B = Top.tp_sizing = FALSE;
	return rc;
}		// pass1
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC begP1aDsdIter()	// autoSizing pass1A begin an ITERATION OF a design day

// sets values to largest found on a previous design day (0 if first design day).
// resetting values each iteration prevents warmup load from causing oversize.
{
	callAllAuszs( &AUSZ::begP1aDsdIter);   		// do x = a for all active AUSZ's
	RC rc = RCOK;
	AH* ah;
	RLUP( AhB, ah)  rc |= ah->ah_SetupSizes();	// reInit models to size
	TU* tu;
	RLUP( TuB, tu)  rc |= tu->tu_SetupSizes();	// reinit terminal for additional stuff
#if 0
x	RSYS* rs;
x	RLUP( RsR, rs)  rc |= rs->rs_SetupSizes();	// reinit to size
#endif
	return rc;
}						// begP1aDsdIter
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC p1aCvgTest()		// pass 1a design day iteration convergence test. k2
// also used for part 1b and for pass 2, 7-95.

// sets Top.notDone if any autosized thing not converged (warmed up).
// may return non-RCOK on fatal error.
{
	RC rc = RCOK;

	Top.notDone = FALSE;				// tentatively say done
	callAllAuszs( &AUSZ::cvgTest, "cvgTest");	// do convergence test for each active autosizer
												// sets Top.notDone if not converged.
	// add any exceptions as required for specific models
	return rc;
}			// p1aCvgTest
//-----------------------------------------------------------------------------------------------------------
LOCAL RC   FC p1bCvgTest()
{
	return p1aCvgTest();    // use p1a code for p1b also
}
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC pass1AtoB()		// transition from pass 1A to pass 1B for a design day

// does accounting, converts measured loads to best-estimate rated values (null conversions 7-95), reinits models.
// On return, iteration of same des day continues with real rather than pass 1A models (due to flags elsewhere).

// returns non-RCOK on error that should terminate run.
{
	RC rc=RCOK;

// make each value being autoSized the largest pass 1A value seen (increase x to .az_a if higher)
	callAllAuszs( &AUSZ::endP1a);

// object pass1AtoB fcns may convert load value to rated capacity value for speed (none do yet, 7-95)
	AH* ah;
	RLUP( AhB, ah)   rc |= ah->ah_pass1AtoB();		// could convert DX load to cap'y @ ARI conditions
	TU* tu;
	RLUP( TuB, tu)   rc |= tu->tu_pass1AtoB();
	RSYS* rs;
	RLUP( RsR, rs)   rc |= rs->rs_pass1AtoB();


// make value the largest part B value seen (increase x to .b if hier) and init re part B eg cvg test
	callAllAuszs( &AUSZ::begP1b);

// object begP1b fcns reInit models cuz value has usually been changed
	RLUP( AhB, ah)   rc |= ah->ah_begP1b();
	RLUP( TuB, tu)   rc |= tu->tu_begP1b();
	RLUP( RsR, rs)   rc |= rs->rs_begP1b();
	return rc;
}			// pass1AtoB
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
//  CSE autoSizing pass 2 functions
//===========================================================================================================
LOCAL RC FC pass2()	// do autosizing pass 2: measure loads & plr, reduce any excessive sizes

// returns RCOK if ok, other value if error or ^C (returns MH_C0100), message already issued.
// called from cgAusz.
{
	RC rc /*=RCOK*/;
	Top.tp_pass2 = TRUE;
	Top.tp_sizing = TRUE;

// outer loop repeats "pass 2" over all design days until all sizes ok

	for (int nItO = 1; ; nItO++)
	{
		if (Top.verbose > 0)				// if autoSize to display detailed screen messages
		{
			if ( Top.verbose >= 2			// if autoSize displaying most detailed screen messages (2)
					||  getScreenCol() > 59)		// if too full to hold " Pass 2 Heat Jul"+1
				screen( 0, "  ");			// start new screen line new line (no NONL) and indent (+TWO spaces in this text)
			screen( NONL, "  Pass 2");		// display "Pass 2". repeats if pass repeated.
		}
		p2Init();				/* init done each "pass 2" pass thru des days:
       						   eg zero load peaks, remember sizes for increase check. 7-11-95.*/

		// loop over design days: simulate each using pass 1 sizes til converged, record peaks loads & part load ratios.

		for (Top.tp_dsDayI = 0;  Top.tp_dsDayI < Top.tp_nDesDays;  Top.tp_dsDayI++)	// 0 heating, 1..n cooling
		{
			if (Top.verbose > 0)   		// if autosize screen messages
			{
				if (getScreenCol() > (Top.verbose >= 2 ? 66 : 74))
					screen( 0, "     ");					// conditional newline & indent
				screen( NONL, " %s", Top.tp_AuszDoing());	// display cooling month or " heat"
			}
			// init for repetition of design day
			CSE_EF( Top.tp_BegDesDay());	// calls cgRddInit: inits simulator state, calls asRddiInit, which calls other rddiInit's.
											// inits .tp_dsDay,.auszmon,.jDay,.isBegRun, etc. Uses Top.tp_dsDayI. local. */
			// init all AUSZ's to start a pass 2 design day
			callAllAuszs( &AUSZ::begP2Dsd);	// do mbr fcn for all AUSZ's. local.

			// iterate design day pass 2 until converged
			for ( ; ; )
			{
				Top.dsDayNIt++;			// count iterations / 1-based while iterating (0'd in begDesDay)
				if (Top.verbose >= 2)
					screen( NONL," .");
				CSE_EF( begP2DsdIter());  		// init all AUSZ's for new iteration of a pass 2 design day: init sizes
				CSE_EF( asRddiInit());			// clear peaks each pass 2 des day iteration
				CSE_E( Top.tp_SimDay());		// simulate a day. Sets Top.iHr, uses many things!. Rewinds imports.
				// E macro returns exact non-RCOK code on error, re detecting ^C (rc = MH_C0100).
				CSE_EF( p2CvgTest() );			// test all active AUSZ's for pass 2 convergence. Sets Top.notDone.
				if (!Top.notDone)			// leave loop if done
					break;
				else if (Top.dsDayNIt >= 30)	// 30-->40 rob 6-97 .487 BUG0089. UNDONE: still didn't converge.
				{
					rWarn( "AutoSize pass %s for %s has not converged.\n"
						   "    Abandoning after %d iterations.",
						   "2", Top.dateStr, Top.dsDayNIt );
					break;
				}
			}

#if 0
x      // conditional pass 2 part B for design day: continue iterating while any value increases
x          if (need2b())				// if any item being autosized needs or might need increaseing
x			{
x             Top.part2B = TRUE;
x             Top.tp_sizing = TRUE;				// say sizes can be increased now: tested many places
x             ZNR *zp; RLUP( ZrB, zp)  zp->spCf++;	/* flag all zones so (mode seq will be rebuilt and) mode init
x							   to middle so setpoint modes can enlarge tu cap'y
x							   (if already pegged, autoSize increase doesn't (yet) work 7-95) */
x             for ( ; ; )
x             {	if (Top.verbose >= 2)
x                   screen( NONL," b");
x                CSE_EF( asRddiInit());		// clear peaks each pass 2 des day iteration
x                CSE_E( Top.tp_SimDay());	// simulate a day, cnguts.cpp
x                CSE_EF( p2CvgTest() );		// test all active AUSZ's for pass 2 convergence. Sets Top.notDone.
x                if (!Top.notDone)				// if (still or again) converged per p2CvgTest
x                {  //if (no increase [over smallTol?] occurred)	<--- implement flag from resizeIf() if need found, 7-95
x                   break;
x				 }
x                else if (Top.dsDayNIt >= 10)
x                {  rWarn( "AutoSize pass %s for %s has not converged.\n"
x                          "    Abandoning after %d iterations.",
x                          "2 part B", Top.dateStr, Top.dsDayNIt );
x                   break;
x			}	}
x            Top.part2B = Top.tp_sizing = FALSE;
x}
#endif

			// after des day done stuff
			endP2Dsd();					// record new high sizes/loads/plr's
			CSE_EF( Top.tp_EndDesDay() )	// calls cnguts.cpp:cgRddDone: closes weather file, imports, binRes, etc. local.
		}

		// pass 2 outer loop endtest: done if all sizes ok, else do another complete pass

		CSE_E( p2EndTest());	// if need to repeat pass 2, adjust size and set Top.notDone.
								// Also sets Top.notDone if any size increased more than tolerance.
		if (!Top.notDone)			// leave loop if done
		{
			/* future: here or caller:
			   might want to set some flag and do another pass e.g. to generate (sub/hourly) report data,
			   and/or make some call to cnguts to do an additional doEndIvl().
			   At present, no end-of-month nor end-of-year (run) actions are performed during autoSizing
			   and no reports are generated in autoSize phase. 7-95. */
			// (5-97: added kludge from cse.cpp to issue AHSIZE and TUSIZE reports, only, after autosize phase if no main sim.)
			break;
		}
		else if (nItO >= 15)	// 10-->15 rob 6-97 .487 BUG0089 (then fixed basic problem, 15 iterations not needed).
		{
			rWarn( "AutoSize pass 2 outer loop has not convergened.\n"
				   "    Abandoning after %d iterations.",
				   nItO);
			break;
		}
	}
	Top.tp_pass2 = Top.tp_sizing = FALSE;
	return rc;
}		// pass2
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC p2Init()		// init repeated before each "pass 2" pass thru design days.  7-11-95.
{
	callAllAuszs( &AUSZ::p2Init);			// for each AUSZ, do eg ldPkAs = plrPkAs = 0.
	TU *tu;
	RLUP( TuB, tu)  tu->tu_p2Init();		// terminals. eg qhPkAs = qcPkAs = 0.
	AH *ah;
	RLUP( AhB, ah)  ah->ah_p2Init();		// air handlers: qcPkSAs = qcPkTDbOAs = 0, etc
	//HEATPLANT *hp;  RLUP( HpB, hp)  hp->p2Init();		// heatplants.
}		// p2Init
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC begP2DsdIter()		// autoSizing pass 2 begin an ITERATION OF a design day

// sets values to largest found in pass 1 or on a previous design day in pass 2
// resetting values each iteration prevents warmup load from causing oversize.
{
	callAllAuszs( &AUSZ::begP2DsdIter);   		// do x = b for all active AUSZ's
	RC rc = RCOK;
	AH* ah;
	RLUP( AhB, ah)  rc |= ah->ah_SetupSizes();	// call member function of each ah eg to reInit models to size
	TU* tu;
	RLUP( TuB, tu)  rc |= tu->tu_SetupSizes();	// call member function of each terminal for additional stuff
	return rc;
}						// begP1aDsdIter
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC endP2Dsd()		// end pass 2 design day, after converged: record new high sizes, loads & plr's
{
	callAllAuszs( &AUSZ::endP2Dsd);			// record new high sizes/loads/plr's for each AUSZ
	TU *tu;
	RLUP( TuB, tu)  tu->endP2Dsd();		// terminals. eg qhPkAs, qcPkAs (negative value).
	AH *ah;
	RLUP( AhB, ah)  ah->endP2Dsd();		// air handlers.
	HEATPLANT *hp;
	RLUP( HpB, hp)  hp->endP2Dsd();		// heatplants. eg qPkAs.
}
//-----------------------------------------------------------------------------------------------------------
LOCAL RC   FC p2CvgTest()
{
	return p1aCvgTest();    // pass 2 convergence test: use p1a code
}
//-----------------------------------------------------------------------------------------------------------
LOCAL RC FC p2EndTest()		// check each object for need to redo pass 2 (and change size if so)
{
	Top.notDone = FALSE;

// set Top.notDone if any x INCREASED much
	callAllAuszs( &AUSZ::p2EndTest, "p2EndTest");

// each object function checks plr's
// if too small, adjusts size and sets Top.notDone to cause another pass.
	RC rc = RCOK;
	TU *tu;
	RLUP( TuB, tu)  rc |= tu->tu_p2EndTest();		// terminals
	AH *ah;
	RLUP( AhB, ah)  rc |= ah->ah_p2EndTest();		// air handlers

// get values into AUSZ's in case reduced by object p2EndTest fcns
	callAllAuszs( &AUSZ::afterP2EndTest);
	return rc;
}					// p2EndTest
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
//  cse MAIN SIMULATION support function(s) that use AUSZ stuff
//===========================================================================================================
RC FC asFazInit([[maybe_unused]] int isAusz)			// main sim run or autoSize phase init: portion in this file

{
	// caller: cnguts:cgFazInit() 6-95
	return RCOK;			// empty fcn, 7-95
}			// asFazInit
//-----------------------------------------------------------------------------------------------------------
RC FC asRddiInit()		// init at start main sim run or each autoSize design day ITERATION: clear peaks

{
// callers 7-95: pass1() (2 calls) and pass2(): each ITERATION of each design day;
// cnguts:cgRddInit() (called for main sim, and by begDesDay() at start of pass 1 or pass 2 des day,
//     believed harmlessly redundant 8-15-95).
// Redundant calls must work ok.

// init peak load mbrs in all autosizers
	callAllAuszs( &AUSZ::rddiInit);		// eg zero .ldPk, .plrPk.  .ldPk used for lpPkAs1 on pass 1.

// additional init for some objects
	TU *tu;
	RLUP( TuB, tu)  tu->rddiInit();		// terminals. eg init qhPk, qcPk (negative value).
	AH *ah;
	RLUP( AhB, ah)  ah->rddiInit();		// air handlers.
	HEATPLANT *hp;
	RLUP( HpB, hp)  hp->rddiInit();		// heatplants. eg init qPk.
	RSYS* rs;
	RLUP( RsR, rs)  rs->rs_RddiInit();	// RSYS

	return RCOK;
}			// asRddiInit
//-----------------------------------------------------------------------------------------------------------


//===========================================================================================================
// member functions of struct AUSZ (cnrecs.def)
//===========================================================================================================
int AUSZ::fazInit(	 // initialize AUSZ & store pointer to value being autosized, at start of autoSize or main sim

	float* xptr, 		// pointer to value 'x', followed by x_As and x_AsNov. Is stored.
	BOO negFlag,		// non-0 if a negative quantity
	record* r,			// pointer to record containing x
	SI fn,				// x's field number (rccn.h RECORD_FIELD define), for access to field status bits
	int isAusz )		// TRUE if autoSize setup. Call for both phases.

// returns nz iff this value is being autosized during current phase

// x is ASSUMED to be followed by:
//   x_As:    receives x as autoSized, or user's x at end autoSize phase if not autoSized
//   x_AsNov: rcvs x, w/o oversize factor if autoSized. Used in "sizing" reports, 7-95.

{
	// called from application code eg in cnztu.cpp, cnah1.cpp.

	if (az_active)   		// being-autosized flag, set during setup
		az_px = xptr;		// store ptr at runtime: minimize risk of record being moved if program changed.
	isNeg = negFlag;		// store negative-value flag

	// for main sim set x_As and x_AsNov to x, etc, if constant inputs given.
	//  1. For values appearing in "sizing" report, even if no autoSize phase.
	//     Thus 0's appear if non-constant, non-autoSized.
	//  2. So @x_As probes work even if item not autoSized, for convenience.

	if (!isAusz)			// leave 0 during autoSizing for consistency with values being autoSized
	{
		UCH * fs = (UCH *)r + r->b->sOff;   		// point field status array for record
		if (fs[fn] & FsVAL)				// if x has a value. Implies FsSET, not expr, not FsAS.
		{
			if (xptr)					// if non-NULL ptr given -- insurance
			{
				xptr[1] = *xptr;				// copy x to x_As -- first float after x
				xptr[2] = *xptr;				// copy x to x_AsNov -- 2nd float after x
			}
		}
	}

// init internal variables
	if (isAusz)			// -As variables must last through main simulation for reports (6-95)
	{
		az_a = az_b = 0.f;		// init autoSizing working variables (leave ausz values if main sim)
		ldPkAs1 = 0;			// init peak load on pass 1, for reporting certain overloads
		// note primary init of following is now in AUSZ::p2Init, 7-11-95
		ldPkAs = plrPkAs = xPkAs = 0;   	// init peak load & part load ratio at end converged pass 2 design day
	}
	return isAusz && az_active;
}			// AUSZ::fazInit
//-----------------------------------------------------------------------------------------------------------
void AUSZ::rddiInit()	// init at start main sim run or each autoSize design day iteration: clear peaks
{
// called 7-95 via asRddiInit() from cnguts:cgRddInit() and cnausz:pass1 (2 calls) and pass2.
// Calls to AH::, TU::, etc rddiInit's follow. Redundant calls occur.

	ldPk = plrPk = xPk = 0;
}
//-----------------------------------------------------------------------------------------------------------
void AUSZ::begP1Dsd()	// autoSizer pass 1 design day init
{
	az_e1 = az_e2 = -1.f;	// init old values for convergence tests to negative to say not run yet.
}
//-----------------------------------------------------------------------------------------------------------
void AUSZ::begP1aDsdIter()	// begin iteration of pass 1A design day
{
	if (az_active && az_px)
		*az_px = az_a;	// set x to max of already-cvgd's days (0 1st day)
					//   so warmup load won't yield excess size.

	// caller then does xx_SetupSizes for objects
}
//-----------------------------------------------------------------------------------------------------------
BOO AUSZ::resizeIf( 		// model calls with size required to conditionally resize

	float x,		// new size required. Different for part A vs B for most models. Tolerance may be added here.
	BOO goose )		// TRUE to add a small margin. Must be FALSE for constant-volume terminals!

// returns TRUE if a new maximum, so caller can reinit model for size (CAUTION fetch new x to get tolerance added here).
// returns FALSE if not new max or cannot resize now (not autoSizing, item not active, wrong pass, etc)
{
	if (Top.tp_sizing)				// if can increase sizes now
		if (az_active && az_px)		// if this item being autoSized
		{
			if (goose)				// add tolerance on caller flag only
				x *= Top.auszHiTol2;   		// add tol always for smooth change, cuz of how cvgTest() extrapolates.
			if (isNeg ? (x < *az_px) : (x > *az_px))	// if a new maximum
			{
				*az_px = x;   			// store it
				return TRUE;   		// tell caller in case he needs to set any dependent variables
			}
		}
	return FALSE;				// no new value stored
}			// AUSZ::resizeIf
//-----------------------------------------------------------------------------------------------------------
// evolution notes 7-22-95: tolerance stuff moving to caller code; complete & remove from resizeIf?
//  resizeIf, unsizeIf tend to be used together; combine? (ztuMode still needs reorganization to do both from same place).
//-----------------------------------------------------------------------------------------------------------
BOO AUSZ::unsizeIf( float x)		// reduce size to x with same conditions as resizeIf. No tolerance here. 7-19-95.
{
	if (Top.tp_sizing && az_active && az_px)
		if (isNeg ? (x > *az_px) : (x < *az_px))	// if a new MINIMUM
		{	*az_px = x;			// store it
			return TRUE;		// say we did: caller may need to re-init model using x
		}
	return FALSE;				// no new value stored
}			// AUSZ::unsizeIf
//-----------------------------------------------------------------------------------------------------------
void AUSZ::endP1a()		// called at end of pass 1A iterations for des day, b4 object pass1AtoB's called
{
	if (az_active  &&  az_px)
	{
		setToMinOrMax( az_a, *az_px, isNeg);		// remember max pass 1A size
		*az_px = az_a;			// set value to max pass 1A size
	}
	// object may now modify value in pass1AtoB fcn to convert pass 1a size (max load) to pass 1b size (rated capacity).
}		// AUSZ::endP1a
//-----------------------------------------------------------------------------------------------------------
void AUSZ::begP1b()		// called at start of pass 1B iterations for des day, after object pass1AtoB's called

// CAUTION: after return, object needs to reinit all models being autoSized, eg in object begP1b fcn.
{
	if (az_active && az_px)
	{
		setToMinOrMax( az_b, *az_px, isNeg);
		*az_px = az_b;			// set value to hiest pass 1B size. Object must reinit model.

		az_e1 = az_e2 = -1;	// init prior values for pass 1B convergence test
		az_orig = az_b;		// enable original-value based cvg test to permit termination after 1 iteration if NO change
	}
}		// AUSZ::begP1b
//-----------------------------------------------------------------------------------------------------------
void AUSZ::endP1bDsd()		// pass 1B end design day: record new peak sizes
{
	// update .az_b: size as increased by model
	if (az_active && az_px)
		setToMinOrMax( az_b, *az_px, isNeg);
	// peak pass 1 load, even if not active
	// eg to report excess supply fan demand while sizing terminals, 7-6-95.
	setToMinOrMax( ldPkAs1, ldPk, isNeg);
}		// AUSZ::endP1bDsd
//-----------------------------------------------------------------------------------------------------------
void AUSZ::p2Init()		// init done before each "pass 2" pass thru des days: zero peaks. 7-11-95.
{
// remember value at start pass for testing if another pass needed 7-13-95
	if (az_px)
		az_orig = *az_px;

	// It is necessary to re-init peak info each pass because load can fall if item resized,
	// for example ZN/ZN2/WZ/CZ tu flow: ts set for 90% flow, thus flow can fall with tuVfMx til ts reaches limit. 7-11-95.
	ldPkAs = plrPkAs = xPkAs = 0;	   	// init peak load & part load ratio at end converged pass 2 design day
}				// AUSZ::p2Init
//-----------------------------------------------------------------------------------------------------------
void AUSZ::begP2Dsd()		// autoSizer pass 2 design day init -- currently 6-95 same as begP1Dsd
{
	az_e1 = az_e2 = -1.f;	// init old values for convergence tests to negative to say not run yet
}
//-----------------------------------------------------------------------------------------------------------
void AUSZ::begP2DsdIter()	// begin iteration of pass 2 design day
{
	if (az_active && az_px)
		*az_px = az_b;	// set x to max of pass 1, already-cvgd pass 2 days:
					// backtrack any increases due to warmup load -- if previous day not converged

	// caller then does xx_SetupSizes() for each AH, TU, etc object
}
//-----------------------------------------------------------------------------------------------------------
#if 0
x void AUSZ::need2b()		// set top.notDone if quantity might need increasing. uses plrPk.
x{
x     if (!az_active)   return;		// nop if not autoSizing this member
x     if (!az_px)   return;			// insurance
x
x     // most items except const vol tu flows are sized tol/2 larger (resizeIf 'goose' arg TRUE).
x     if (plrPk > 1 - Top.auszTol/4)	// if load uses half that margin, do a part 2B iter to see if increase needed
x        Top.notDone++;			// ... if its an item without margin, 1 part 2B iteration always done.
x}			// AUSZ::need2b
#endif
//-----------------------------------------------------------------------------------------------------------
void AUSZ::endP2Dsd()		// end pass 2 design day, after converged: record new high autoSize sizes, loads & plr's
{
	// remember any size increases on final (warmed up === converged) pass 2 iteration of design day: update .b
	if (az_active && az_px)
		setToMinOrMax( az_b, *az_px, isNeg);

	// remember peak load and plr on converged day
	setToMinOrMax( ldPkAs, ldPk, isNeg);

	if (plrPk > plrPkAs)		// if a new high plr
	{
		plrPkAs = plrPk;		// remember it
		xPkAs = xPk;			// remember rated size to which the plr relates, 6-97
	}
}		// AUSZ::endP2Dsd
//-----------------------------------------------------------------------------------------------------------
void AUSZ::p2EndTest() 		// set Top.notDone if another pass 2 thru design days needed
// Optionally displays debug info to screen per Top.verbose
// < 3		no printout
//   3		prints info for "not done" if top.notDone 0 at entry
//   4		prints info for "not done" regardless of top.notDone
//   5		prints info for each call, whether done or not
// az_doing points to caller's description of current action (or NULL)
{
	const char* doing = az_doing ? az_doing : "?";
	if (az_active && az_px)
		if (!Top.notDone || Top.verbose > 3)
			if (az_orig != 0.f)
			{	float fChange = fabs((*az_px - az_orig) / az_orig);
				if (fChange > Top.auszTol2)		// if value changed by > 1/2 tolerance
				{	if (Top.verbose > 2)
						screen( 0, "      %s: %8g %8g  not done: changed", doing, *az_px, az_orig);
					Top.notDone++;
					return;				// do another pass thru design days
				}
			}
			else if (*az_px)						// if went from 0 to non-0
			{
				if (Top.verbose > 2)					// believe redudant - not called if not
					screen( 0, "      %s: %8g %8g  not done: 0 became non-0", doing, *az_px, az_orig);
				Top.notDone++;
				return;					// do another pass thru design days
			}
			else if (Top.verbose > 4)
				screen( 0, "      %s: %8g %8g  done", doing, *az_px, az_orig);

	// in addition AH::, TU::, etc object member functions test for oversize capacities: plr too small.
}			// AUSZ::p2EndTest
//-----------------------------------------------------------------------------------------------------------
void AUSZ::afterP2EndTest()		// store possibly reduced values after object p2EndTest fcns called. 7-16-95.
{
	if (az_active && az_px)
		az_b = *az_px;		// get any change, to put back into x at start next des day iter
}
//-----------------------------------------------------------------------------------------------------------
void AUSZ::cvgTest()		// autoSizer warmup convergence endtest
// Optionally displays debug info to screen per Top.verbose
// < 3		no printout
//   3		prints info for "not done" if top.notDone 0 at entry
//   4		prints info for "not done" regardless of top.notDone
//   5		prints info for each call, whether done or not
// az_doing points to caller's description of current action (or NULL)

// sets Top.notDone if NOT converged, else caller uses results from des day iteration just completed

{
	if (!az_active)   return;		// nop if not autoSizing this member
	if (!az_px)   return;			// insurance

// update info
	float e3 = az_e2;		// copy 2nd prior result for internal use
	az_e2 = az_e1;			// remember history for later convergence tests
	az_e1 = Top.tp_pass2
		 ? ldPk 		// get value: pass 2 converges day peak load
						//    (not plr as cap'y can change in part 2B)
		 : *az_px;			// pass 1 converges x
	if (isNeg)
		az_e1 = -az_e1;		// make positive cuz -1 is unset indicator

#if 1 // delete when want to see all the notDone++'s to faciliate debugging. 7-95.
	if (Top.verbose < 4)		// 4 or more makes stuff print for all calls
		if (Top.notDone)  return;		// skip computation if some member already found not converged
#endif

	const char* doing = az_doing ? az_doing : "?";

// handle 0 value
	if (az_e1==0)				// if most recent value zero
	{
		if (az_e2 || e3)			// consider three zeroes to be a convergence (e's non-0 for iterations not done)
		{
			Top.notDone++;		// 1 or 2 zeroes: consider not done: might need warmup to exceed some threshold.
			if (Top.verbose > 2)		// redundant test
				screen( 0, "      %s: %8g %8g %8g  not done: 0", doing, az_e1, az_e2, e3);
		}
		else if (Top.verbose > 4)	// 5 or more prints info for the done's also
			screen( 0, "      %s: %8g %8g %8g  done: 0 3 times", doing, az_e1, az_e2, e3);
		return;				// return if az_e1==0: won't work in relative change tests below
	}

// handle 1st iteration
	if (az_e2 < 0)  			// if not at least 2nd iteration (e's init to -1)
	{
		if (Top.tp_pass1B)		// only in pass 1B: already warmup up (in pass 1A, several iterations
								//    might occur before demand becomes non-0, so don't stop too soon)
			if (az_e1==az_orig)	// if identical to starting value (pass 1b), and non-0 if here, consider it done 7-3-95
			{
				if (Top.verbose > 4)
					screen( 0, "      %s: %8g %8g %8g  done: 1B no change", doing, az_e1, az_e2, e3);
				return;		// ... but if it changed at all, convergence test only on the basis of e's.
							// Especially, don't use r = (az_e1-az_e2)/(az_orig-az_e2) cuz could be erroneously small.
			}
		if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: 1st iteration", doing, az_e1, az_e2, e3);
		Top.notDone++;
		return; 		// no comparator, can't test whether converged, so not done
	}

#define TOL Top.auszTol2		// tolerance to use in this convergence test: 1/2 of user tol cuz 1/2 used in resizeIf.
#define SMTOL Top.auszSmTol		// smaller tolerance, eg given tolerance/10.

// look at change from previous value
	float d1 = az_e1 - az_e2;	// most recent delta-x
	if (fabs(d1) > TOL*az_e1)  	// if delta > tolerance, consider it not converged
	{
		if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: delta %g too large", doing, az_e1, az_e2, e3, d1);
		Top.notDone++;
		return;
	}

	if (fabs( d1) < SMTOL*az_e1 && Top.dsDayNIt > 2) // if delta very small, but value non-0 if here
	{
		if (Top.verbose > 4)
			screen( 0, "      %s: %8g %8g %8g  done: delta %g very small", doing, az_e1, az_e2, e3, d1);
		return;				// consider it converged without looking at previous delta
	}

// look at previous change
	if (e3 < 0)  			// if not at least 3rd iteration
	{	if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: 2nd iteration", doing, az_e1, az_e2, e3);
		Top.notDone++;
		return; 			// no previous comparator, can't complete test, so not done
	}
	float d2 = az_e2 - e3;		// next most recent delta
	if (d2==0)  			// avoid divide by 0
	{
		if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: previous change 0", doing, az_e1, az_e2, e3);
		Top.notDone++;
		return;		// current value was a change & previous not a change: not done.
	}
	float r = d1/d2;		// ratio of latest to previous delta-x
	if (r >= 1)  			// if change getting bigger, consider it not converged, don't divide by 0
	{
		if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: diverging: r=%g", doing, az_e1, az_e2, e3, r);
		Top.notDone++;
		return;
	}
	float sum = d1 * r/(1-r);		// what all remaining deltas will add up to if x is behaving as geometric series
	if (fabs(sum) >= TOL*az_e1)		// if remaining change too big
	{
		if (Top.verbose > 2)
			screen( 0, "      %s: %8g %8g %8g  not done: extrapolated remaining delta %g", doing, az_e1, az_e2, e3, sum);
		Top.notDone++;
		return;
	}
	if (Top.verbose > 4)
		screen( 0, "      %s: %8g %8g %8g  DONE", doing, az_e1, az_e2, e3);
	// sum will be > d1 if r > 0, less if r negative.
	// to prevent premature exit on r < 0 caused eg by initial thrash, d1 > TOL set notDone & exited above.
}			// AUSZ::cvgTest
//-----------------------------------------------------------------------------------------------------------
void AUSZ::final( 		// final autoSizing stuff: copy members to copies & input record; apply oversize.
	record* r, 				// record containing this AUSZ
	record* ir, 			// NULL or input record which will be used to re-Setup for main sim run
	float fOverSize )		// overSize factor (1.0 = no adjustment)

{
	// called from application code in cnah1.cpp, cnztu.cpp, etc.

	/* Copy autoSizing results to:
	   1) member of same object with same name + _As.
	      This member has minimal variation to facilitate probing for main simulation input.
	   2) before multiplying by oversize, copy to member with same name + _AsNov, for use in "sizing" report.
	      This value is consistent with the other report columns, that show peak load etc during autoSizing.
	   3) INPUT RECORDS, so they will appear in regenerated runtime records used for main simulation.
	      (if no main sim requested, input records may already have been deleted). */

	/* Note if NOT autoSized, fazInit's set _As and _AsNov from constant x's so
	   happens if no ausz phase, and so get 0 not last value for expr. */

	/* code here assumes:
	   x is a float
	   after x is x_As, to receive a copy of x after oversize added.
	   after that is x_AsNov, to receive a copy of x b4 oversize added. */

	if (az_px && az_active)		// if have pointer to 'x' being autoSized. insurance. az_px is float *.
	{
		az_px[2] = *az_px;			// set x_AsNov (2nd float after x) to x value before oversized
		*az_px *= fOverSize;		// apply oversize factor in place -- program may use x in place
		az_px[1] = *az_px;			// set x_As (1st float after x) to oversized x value

		if (ir)				// if input record pointer given
		{
			USI xOff = (USI)((UCH *)az_px - (UCH *)r);		// compute member offset of x in record
			float *pxi = (float *)((UCH *)ir + xOff);	// point to same offset in input record
			pxi[0] = az_px[0];				// copy x to input record
			pxi[1] = az_px[1];				// copy x_AsNov ditto
			pxi[2] = az_px[2];				// copy x_As ditto
		}
	}

	if (ir)					// if input record pointer given
	{
		USI aOff = (USI)((UCH *)this - (UCH *)r); 	// offset of AUSZ substruct in its record
		AUSZ* ia = (AUSZ *)((UCH *)ir + aOff);		// point to AUSZ at same offset in input record
		memcpy( ia, this, sizeof(AUSZ));			// copy entire AUSZ substruct to input record
													//  to save eg ldPkAs and plrPkAs for reports.
		ia->az_px = NULL;					// pointer to x not valid in record at different location
	}
}		// AUSZ::final
//-----------------------------------------------------------------------------------------------------------

// end of cnausz.cpp
