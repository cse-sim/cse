// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncult2.cpp: cul user interface tables and functions for CSE
//   part 2: fcns called from top table & their subfcns.

// search NUMS for messages just added 6-95...  exman.cpp too. grep NUMS!

/*------------------------------- INCLUDES --------------------------------*/
#include "CNGLOB.H"	// USI SI LI. includes cndefns.h, for ZHX_TULHC,

#include "SRD.H"	// SFIR,
#include "ANCREC.H"	// record: base class for rccn.h classes
#include "rccn.h"	// TOPRATstr ZNRstr SFIstr SDIstr
#include "MSGHANS.H"	// MH_S0470
#include "MESSAGES.H"	// msgIsHan
#include "EXMAN.H"
#include "TDPAK.H"		// tdHoliDate tddtis tddyi
#include "CVPAK.H"		// getChoiTx
#include "WFPAK.H"		// wfOpen wfClose 1-95
#include "PSYCHRO.H"	// psyAltitude PsyRAir
#include "solar.h"
#include "ENVPAK.H"		// ensystd
#include "IMPF.H"

#include "CNGUTS.H"	// Top ZrB Wfile

#ifdef COMFORT_MODEL
#include "comfort/comfort.h"
#endif

#include "PP.H"    	// ppFindFile
#include "CUPARSE.H"	// per
#include "CUL.H"	// CULT oer ooer oWarn
#include "CUEVAL.H"	// cupfree cupIncRef
#include "CUEVF.H"	// evf's and variabilities: EVF____ defines: EVFFAZ

// declaration for this file:
#include "CNCULT.H"	// use classes, globally used functions
#include "IRATS.H"	// declarations of input record arrays (rats)
#include "CNCULTI.H"	// cncult internal functions shared only amoung cncult,2,3,4.cpp

#include "Foundation.h"	// access to Kiva objects


/*-------------------------------- DEFINES --------------------------------*/

/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL RC topCkfI( int re);
LOCAL RC topDC();
LOCAL RC topZn( int re);
LOCAL RC topIz();
LOCAL RC topDs();
LOCAL RC topMat();
LOCAL RC topCon1();
LOCAL RC topLr();
LOCAL RC topCon2();
LOCAL RC topFnd();
LOCAL RC topGt();
LOCAL RC topMtr();
LOCAL RC topGain();
LOCAL RC topRSys1();
LOCAL RC topRSys2();
LOCAL RC topDHW();
LOCAL RC topInverse();
LOCAL RC topPV();
LOCAL RC topBT();
LOCAL RC topSX();
LOCAL RC topRadGainDist();
LOCAL RC badRefMsg( BP toBase, record* fromRec, TI mbr, const char* mbrName, record* ownRec );


//===========================================================================
void cncultClean(	// cleanup fcn for all the cncultn.cpp files

	[[maybe_unused]] CLEANCASE cs )		// ENTRY, DONE, CRASH (cnglob.h)

// added for DLL version, which should free every bit of its heap memory, ideally even on error exits
{
//cncult.cpp
	iRatsFree();		// free all input basAnc records

//cncult4.cpp
	freeHdrFtr();		// free header/footer strings in heap
	clearImpf();		// special clear re import files 2-94


	// 12-3-93 a check of all cncult.cpp files revealed no additional local variables.

}		// cncultClean

/*=========== functions ref'd by entries in top table follow ==========*/


//===========================================================================
RC topStarItf([[maybe_unused]] CULT *c, [[maybe_unused]] void *p, [[maybe_unused]] void *p2, [[maybe_unused]] void *p3)   /*ARGSUSED */

// top *-entry IniTialization Function

// called BEFORE CLEAR before start of data input, on initial cul() entry only

// see also topStarPrf: called after input rats cleared.
{
	return RCOK;			// currently does nothing
}		// topStarItf
//===========================================================================
RC topStarPrf( CULT *c, void *p, void *p2, void *p3)   /*ARGSUSED */

// top *-entry (startup) PRe-Input function / top clear pre-input fcn

/* called before start of data input, after data cleared, on initial cul() entry,
   AND ALSO, via separate CULT table entry, after CLEAR command (ie used as TopClearPrf) (NOT TESTED 11-91 in this use).*/

// see also topStarItf: called before input rats cleared.
{
	// say "Top" (as opposed to Topi) not valid
	// eg to prevent use of old (poss dmfree'd) runTitle ptr in cncult4.cpp til tp_fazInit (from topCkf) (pre rigorous ref counts)
	Top.ck5aa5 = 0;

// say various texts should be (re)generated before use as may have new, more, or better inputs
	freeRepTexts();		// cncult4.cpp

// generate default rat entries
	topPrfRep();		// primary/default REPORTFILE and EXPORTFILE rat entries, and default REPORTS.  cncult4.cpp.
	topPrfHday();		// default HDAY (holiday) rat entries, cncult4.cpp

// also do recall pre-input init at startup or clear
	topStarPrf2( c, p, p2, p3); 	// call reentry pre-input fcn (called by cul per table entry on reentry only)

// import files have special clear fcn cuz IFFNM records in IffnmB may be used both for compile support and run.
	clearImpf();			// cncult4.cpp 2-94.
	return RCOK;
}		// topStarPrf
//===========================================================================
RC topStarPrf2([[maybe_unused]] CULT *c, [[maybe_unused]] void *p, [[maybe_unused]] void *p2, [[maybe_unused]] void *p3)   /*ARGSUSED */

// top *-entry recall pre-input function, also called (from topStarPrf above) on startup and clear. 7-13-92.

{
// get run serial number into Topi so probable: must NOT reset on clear
	Topi.runSerial = cnRunSerial;		// cnguts.cpp variable.
	// note updated by cnguts:cgInit with ++ or per (future) status file, 7-92.
// get run date and time string for bin res file, probes, reports (cgresult.cpp:cgZrExHd, cncult4:getFooterText). Here 9-94.
	dmfree( DMPP( Topi.runDateTime));		// free any prior run's date/time. dmpak.cpp.
	IDATETIME idt;
	ensystd(&idt);						// get current date/time in IDATETIME form. envpak.cpp
	Topi.runDateTime = strsave( 		// save string in dm, strpak.cpp
						   tddtis( &idt, NULL) );	// IDATETIME ---> string
	return RCOK;
}			// topStarPrf2
//===========================================================================
RC topClearItf([[maybe_unused]] CULT *c, [[maybe_unused]] void *p, [[maybe_unused]] void *p2, [[maybe_unused]] void *p3)   /*ARGSUSED */

// called by CLEAR at top level BEFORE data deleted
{
	/* This function must do any needed cleanup of input RATs and free
	   any dm data other than DAT TYSTR's: free dm pointers without cult
	   entries or not DAT & TYSTR, re-init derived information without
	   cult entries (eg stuff set by user functions in TopiR or non-cult
	   storage), etc. */

	/* After this function returns, caller will walk RATs and free TYSTRs
	   per cult entries, then free all nested RAT entries. */

	// to re-initialize AFTER caller clears, put code in topClearPrf or topClearCkf.

	/* Note that TopiR may NOT be re-0'd by caller, so any members w/o cult
	   defaults should be re-init here if not covered by regular fcns. */

	return RCOK;
}		// topClearItf
//===========================================================================
// topClearPrf(): note that topStarPrf above is recalled for this purpose.
//---------------------------------------------------------------------------

//===========================================================================
RC topCkf([[maybe_unused]] CULT *c, [[maybe_unused]] void *p, [[maybe_unused]] void *p2, [[maybe_unused]] void *p3)   /*ARGSUSED */

// finish/check/set up function entry called via cnTopCult at top level "RUN" command
{
	return topCkfI( FALSE);
}
//===========================================================================
RC topReCkf()
// finish/check/set up function entry called to re-set-up for main simulation after autosize, 6-95
// to be called from cnguts.cpp.
{
	return topCkfI( TRUE);
}
//===========================================================================
LOCAL RC topCkfI(	// finish/check/set up inner function
	int re)		// 0: when RUN seen
				// nz: 2nd call when main simulation follows autosize

/* does:
     checking that must be deferred til multiple inputs present
        new-code trend is to defer ALL checking to here to minimize expression complications
         recalls object check functions as checks during input can be incomplete eg if a TYPE used.
     copies input basAnc records to run basAnc records (same format in most cases) --
        input records preserve runtime expression NANDLEs for addl RUNs; run records receive values (elsewhere).

       assumes already done:
         deferred record references (TYREF) resolved
         end-of-input (but not FAZLY) expressions evaluated, values stored in input records. */
// called from topCkf and topReCkF.
{
	RC rc=RCOK;	// rc is used in E macros

	/* Required (RQD) members notes: omitted required data is messaged at end of each object,
	   but cul might proceed to call this fcn anyway to get more msgs in one run.
	   Unset RQD members may contain UNSET (cnglob.h), which generates floating point error
	   (FPE, cryptic fatal message) if accessed as a float (as opposed to a NANDAT).
	   Before first access to each RQD member, check for FsSET and return non-RCOK if off.
	   (If member accepts runtime expressions (VINITLY or greater), also check for FsVAL before accessing as number.) */

	/* Return codes note: any error message stops run, via error count
	   (rmkerr.cpp error count is ++'d via err, oer, per, etc, also by caller cul:cuf() if this fcn rets nz).
	   The most serious errors in called functions return non-RCOK to terminate this function, for example:
	   omitted RQD data (later fcn might get FPE on UNSET)
	   bad reference or other internal error that might cause many spurious messages in later fcns
	   out of memory (continuation proliferates errors) */


#ifdef AUTOSIZE
	// 're' is TRUE on recall for run after autosize. Pass to called fcns as need found, 6-95.

//--- flag (run) basAncs whose records will persist from autoSizing to main sim if both are performed.
	// flag use: exman.cpp: retain expr use info between phases for records in these basAncs 10-95, .
	// list of basAncs must match: those not re-setup here if re;
	//                             those not cleared by cnguts:runFazDataFree() (use RFPERSIS there?)
	DvriB.ba_flags |= RFPERSIS;
	RcolB.ba_flags |= RFPERSIS;
	XcolB.ba_flags |= RFPERSIS;

//--- first determine whether autosizing or main simulation will occur next, set $autosize, evaluate FAZLY expressions.
	// default or check/fix chAutoSize (input "doAutoSize = YES | NO" or as changed)
	if (!re)							// on initial call only
	{
		if (!(Topi.sstat[TOPRAT_CHAUTOSIZE] & FsSET))		// if not given, set to default:
			Topi.chAutoSize = Topi.bAutoSizeCmd
					? C_NOYESCH_YES	//  YES if any AUTOSIZE commands in input
					: C_NOYESCH_NO;	//  NO if no AUTOSIZE commands in input
		else if ( Topi.chAutoSize==C_NOYESCH_YES		// if YES given (and no other errors if here)
			 &&  !Topi.bAutoSizeCmd )   				// but no AUTOSIZE commands in input
		{
			Topi.chAutoSize = C_NOYESCH_NO;   			// change it to no -- no autosizing will be done
			pWarn( "'DOAUTOSIZE = YES' will be disregarded because \n"
				   "    no 'AUTOSIZE <x>' commands found in input." );  	// NUMS
		}
		if (Topi.IsSetCount( TOPRAT_COOLDSDAY, TOPRAT_COOLDSMO, TOPRAT_COOLDSCOND, 0) > 1)
			// can specify at most 1 cooling design day scheme
			rc |= Topi.oer( "only 1 of 'coolDsDay', 'coolDsMon', and 'coolDsCond' can be given");
	}

// set autosizing flag: TRUE if setting up now for autosizing, FALSE if setting up for main simulation or input check only.
	Top.tp_autoSizing =     					// also set now in Top run record now for $autoSizing in exprs
		Topi.tp_autoSizing = !re  				// recall always for main sim
	  &&  (Topi.chAutoSize==C_NOYESCH_YES);	// 1st call is for autoSize if (still) requested (after check)

// now evaluate EVFFAZ exprs: may use @Top.tp_autoSizing or $autoSizing; need values for rest of check/setup processing.
	exEvEvf( 			// exman.cpp. Stores values into input basAnc records. On error, errCount++'s but may return RCOK.
		EVFFAZ,			// exprs that depend on phase (autosize or main simulation).
		0xffff );		// all use classes.
	if (errCount())		// if error in exEvEvf (or previously) (message already issued) (rmkerr.cpp fcn)
		return RCBAD;		// terminate run
#endif

//---- derived Top members, and some defaults that must be done in code
//        do after .chAutoSize and .autoSizing above.
	rc |= Topi.tp_SetDerived();		// calls tp_SetOptions() plus more

//---- set up Top from Topi
	Top = Topi;
	CSE_E( Top.tp_FazInit())	// reads weather file, inits air properties, 

	CSE_E( topDC() )			// finalize DESCONDs

//--- zones next: reports store into zone run records
	CSE_E( topZn( re) )			// do zones. E: returns if error.

//--- do meters early: ref'd by reports and maybe many other future things.
	CSE_E( topMtr() )			// check/dup all types of meters (energy, dhw, air flow)

//--- do stuff re reports next -- much used even if error suppresses run. cncult4.cpp.
	if (!re)			// report/exports persist from autosize thru main simulation 6-95.
	{
		// thus cannot use VFAZLY (*f) members, nor persistent Top members.
		// DvriB, XcolB, rColB are not free'd after autosize like most run data,
		//  and their expr use info is retained (exman.cpp) per RFPERSIS flag set above.
		// ZNI/R.vrZdd requires special consideration in cncult4.cpp.
		CSE_E( topRep() )   		// check rep___ members / set up page header and footer. cncult4.cpp
		CSE_E( topRf() )			// check REPORTFILEs cncult4.cpp
		CSE_E( topXf() )			// check EXPORTFILEs cncult4.cpp
		CSE_E( topCol(0) ) 			// check REPORTCOLS / build RcolB.  cncult4.cpp
		CSE_E( topCol(1) ) 			// check EXPORTCOLS / build XcolB.  cncult4.cpp
		CSE_E( topRxp() )			// check REPORTS and EXPORTS / build dvri info.  cncult4.cpp
		CSE_E( buildUnspoolInfo() )	// build virtual report unspooling specifications.  cncult4.cpp
	}

//--- other input data
	CSE_E( topImpf() )			// IMPORTFILEs
	CSE_E( topHday() )			// holidays. cncult4.cpp.
	topPr();				// perimeters. cncult3.cpp.  can we move this to just above topSf1 1-92?
	CSE_E( topIz() )   			// do interzone transfers. do b4 topSf2 and topDs
	CSE_E( topHp1() )			// heatplants 1: create run recs. do TU's,AH's,BOILERS,HPLOOPs tween topHp1 & 2. cncult6.cpp.
	CSE_E( topCp1() )			// coolplants 1: create run recs. must do AH's,CHILLERs tween topCp1 & 2. cncult6.cpp.
	CSE_E( topTp1() )			// towerplants 1: create run recs. must to topCp2 between TopTp1 & 2. cncult6.cpp.
	CSE_E( topAh1() )			// do air handlers part 1, before terminals.  cncult5.cpp.
	CSE_E( topTu() )			// do terminal units.  cncult5.cpp.  Be sure error return propogates.
	CSE_E( topAh2() )			// do air handlers part 2, after terminals and topHp1, topCp1. cncult5.cpp.
	CSE_E( topBlr() )			// do boilers b4 topHp2. cncult6.cpp.
	CSE_E( topCh() )			// do chillers b4 topCp2. cncult6.cpp.
// HPLOOPs when impl must be above here
	CSE_E( topHp2() )			// heatplants 2: complete check/setup after loads chained & boilers to records. cncult6.cpp.
	CSE_E( topCp2() )			// coolplants 2: complete check/setup after loads chained & chillers to records. cncult6.cpp.
	CSE_E( topTp2() )  			// towerplants 2: after topCp2: chains cp's to tp's; generates default cap info. cncult6.cpp.
	CSE_E( topMat() )			// materials.  local.
	CSE_E( topCon1() )			// constr pass 1: 0 accumulators.  local.
	CSE_E( topLr() )			// layers, & accumulate to constructions.  local.
	CSE_E( topCon2() )			// constructions pass 2.  local.
	CSE_E( topFnd() )			// foundations 04-18. local.
	CSE_E( topGt() )			// glazeTypes 12-94. local.
	CSE_E( topSf1() )			// surfaces pass 1.  cks/dflts stuff used for doors/windows (azm,tilt,H's). cncult3.
	CSE_E( topMsCon())			// mass & masstype stuff between topSf1 and 2.  cncult3.cpp
	CSE_E( topSg() )		   	// sgdists: do after windows pass 1. sets gz.sgdist[]: do b4 topSf2( , WINDOW). cncult3.
	CSE_E( topSh() )			// shades: do after windows pass 1. sets gz.x.iwshad: do b4 topSf2( , WINDOW). cncult3.
	CSE_E( topRSys1() )			// RSYS basic setup: before topDs
	CSE_E( topDs() )   			// duct segments: before topZn2
	CSE_E( topZn2() )			// zones: accum total surf areas etc.
	CSE_E( topSf2() )  			// surfs pass 2: make XSRATs and MSRATs.
	CSE_E( topSg2() )			// solar gains pass 2
	CSE_E( topDHW() )			// DHW (must be before topGain)
	CSE_E( topGain() )			// check gains / copy to run rat.  local fcn.
	CSE_E( topRadGainDist() )	// set up distribution of radiant internal gains. Must follow most surface/zone stuff

	CSE_E( topRSys2() )			// RSYS final setup
	CSE_E( topPV() )			// PVArrays
	CSE_E( topBT() )            // Batteries
	CSE_E( topSX() )			// ShadeX external shading

	CSE_E( topZn3() )			// zones: add generated IZXFERs,

	CSE_E( topInverse())		// inverse objects


#ifdef AUTOSIZE
	//--- message if input check only
	if ( Top.chAutoSize != C_NOYESCH_YES
	 &&  Top.chSimulate != C_NOYESCH_YES )
		// NUMS. pInfo() added for this, 6-95.
		pInfo("Input check complete. \n""    Neither autosize nor main simulation requested.");
#endif
	return rc; 				// error returns above including each E macro
}			// topCkfI
//===========================================================================
RC TOPRAT::tp_SetCheckTimeSteps()	// initialize timesteps
// NOTE: call at RUN, can query presence of items in model
{
	RC rc = RCOK;

	// DHW models uses shorter "tick" step
	bool bUsesTicks = WSiB.GetCount() > 0;

	int maxSubStepsThisRun = bUsesTicks ? 60 : MAXSUBSTEPS;

	if (tp_nSubSteps > maxSubStepsThisRun)
	{
		rc |= oer("nSubSteps (%d) exceeds maximum allowed (%d)",
			tp_nSubSteps, maxSubStepsThisRun);
		tp_nSubSteps = maxSubStepsThisRun;		// set to legal value for remainder of init
	}

	// Init subhour duration
	tp_subhrDur = 1.f / tp_nSubSteps;			// subhr duration

	// substep ticks = even shorter steps for e.g. HPWH simulation
	tp_nSubhrTicks = max(1, 60 / tp_nSubSteps);
	tp_tickDurHr = 1. / (tp_nSubhrTicks * tp_nSubSteps);
	tp_tickDurMin = tp_tickDurHr * 60.;

	if (tp_tickDurMin != 1. && bUsesTicks)
		rc |= oer("nSubSteps=%d is invalid because it yields a non-integral substep duration (%0.3f min)."
			   "\n    An integral duration is required when DHWSYS(s) are present.",
			tp_nSubSteps, tp_tickDurMin);

	return rc;
}	// TOPRAT::tp_SetCheckTimeSteps()
//----------------------------------------------------------------------------
RC TOPRAT::tp_SetDerived()
// set derived (runtime) values from input
// errors stop run via error count
{
	RC rc =RCOK;

//---- derived Top members, and some defaults that must be done in code

	// also .chAutoSize and .autoSizing are processed by caller -- must be done first.

//set stuff based on command line options, 12-95.
	tp_SetOptions();		// sets top mbrs for bin res file cmd line flags, etc.

	// Timestep setup
	rc |= tp_SetCheckTimeSteps();

	//# days in run
	nDays = tp_endDay - tp_begDay + 1;
	if (nDays <= 0) 	 		// for eg Dec..Feb run, or Jul 1..Jun 30.
		nDays += 365;			// fix day count

	//run "year"
	year = -((jan1DoW + 5) % 7) - 1;	/* map jan 1 day of week (1=sun..7=sat, data\dtypes.def DOWCH type) to
						   generic year starting on that day (-1=mon..-7=sun, lib\tdpak.cpp). */

	// default and check daylight time start/end dates
	//   modified re 2007 law revision, 7-11
	if (!DTBegDay)									// if daylight time start date not given
		DTBegDay = tdHoliDate( year, C_HDAYCASECH_SECOND, 0/*Sun*/, 3/*Mar*/);	// default to 2nd Sun in Mar.
	if (!DTEndDay)									// if daylight time end date not given
		DTEndDay = tdHoliDate( year, C_HDAYCASECH_FIRST, 0/*Sun*/, 11/*Nov*/);	// default to 1st Sun in Nov.
	if (DTBegDay==DTEndDay)
		per( (char *)MH_S0470);  				// "Daylight saving time should not start and end on same day"
	// notes: if DT end day < start day, southern hemisphere summer assumed.

	// precision of hvac computations (app\cnztu.cpp, cnah.cpp, perhaps cnloads.cpp)
	// checks? disallow tol over .05, warn over .01?
	relTol = tp_tol;				// relative tolerance is as input, default .001, 5-92.
	relTol1 = relTol * 1.125f;	// slightly larger relTol to assure exit testing a value with +-relTol slop, 8-92.
	absTol = relTol * 100.f;		// absolute quantity tolerance, eg for temperatutes: .1% corress .1 degree;
											//   scaled eg by .001 for hum rats.
	absHumTol = absTol * humTolF;	// precompute absolute humidity ratio (w) tolerance: smaller by factor.
	hiTol = 1.f + relTol;		// 1 + relative tolerance for runtime convenience. 6-92.
	loTol = 1.f - relTol;		// 1 - rel tol likewise.

	//default runTitle to "" (in heap)
	if (!runTitle)				// if now NULL, change to "" for convenience of users eg cgresult.cpp
		runTitle = strsave("");		// use a "" in heap so can dmfree it without complications OBSOLETE CONSIDERATION

#ifdef BINRES
//check/clean up binary results inputs, in topi so messages for fixed errors don't repeat. Rob 12-2-93.
	brFileCk();				// below
#endif
	return rc;
}		// TOPRAT::tp_SetDerived
//---------------------------------------------------------------------------------------------
RC TOPRAT::tp_FazInit()	// start-of-phase init
// call at start of autosizing (if any) and again at start of main simulation
// returns RCOK iff success
//         do not run if !RCOK
{
	RC rc = RCOK;

#if defined( DEBUGDUMP)
	DbDo(dbdSTARTRUN);		// init debug print scheme (re heading control)
	tp_SetDbMask();		// debug printout mask (dbdXXX), controls internal val dumping
							//   (can change hourly, set initial value here re print from setup code)
#endif

//---- find/read weather file name / get lat, long, tz, default elevation, default autosizing stuff. 1-95.
	int hotMo = 7;				// hottest month 1-12, wthr file hdr to topAusz, July if not set by topWfile
	CSE_E(tp_Wfile(&hotMo))		// just below. call b4 topAusz.

//---- autosizing setup. call after topWfile. 6-95.
	CSE_E(tp_Ausz(hotMo))

//---- say various texts should be (re)generated before (any further) use as may have new runTitle, repHdrL/R.
	freeRepTexts();				// cncult4.cpp

	tp_pAirNet = new AIRNET();

	return rc;
}			// TOPRAT::tp_fazInit
//---------------------------------------------------------------------------
LI TOPRAT::tp_SetDbMask()		// evaluate and set current debug print mask
// WHY: need value available during setup but also with hourly variability
//      so separate constant and variable parts are used.
// returns resulting mask value
{
	LI dbgPrintMask = tp_dbgPrintMaskC;		// constant portion (known at EOI)
	if (!ISNANDLE( tp_dbgPrintMask))
		dbgPrintMask |= tp_dbgPrintMask;	// possibly variable portion (ignore if not yet set)
	DbSetMask( dbgPrintMask);
	return dbgPrintMask;
}		// TOPRAT::tp_SetDbMask
//===========================================================================
RC TOPRAT::tp_Wfile(		// find/read weather file / init top members with data from it
	int* pHotMo )			// receives hottest month 1-12 if avail in wthr file hdr, if autoSizing
// return RCOK iff success
{
	// wf stuff here & cnguts.cpp redone for run
	// after autosize, allowing wfName etc to change.

// find weather file, store full path
	ppFindFile( tp_wfName);		// find file, update tp_wfName to full path
	if (tp_TDVfName)
		ppFindFile( tp_TDVfName);		// auxiliary TDV (time dependent value) file if any

// open weather file
	RC rc = Wfile.wf_Open( tp_wfName, tp_TDVfName);	// reports err if can't open.

	if (!rc)					// if opened and read ok
	{	// fetch latitude, longitude, time zone to Top members
		latitude = Wfile.lat;			// fetch latitude and longitude and time zone
		longitude = Wfile.lon;			// ..
		timeZone = Wfile.tz;				// .. used re solar init from cse.cpp.
		if (!IsSet(TOPRAT_ELEVATION))	// if elevation not given by user
			elevation = Wfile.elev;			// fetch elevation from weather file

		rc |= tp_LocInit();		// init solar re location

		rc |= tp_Psychro();		// psychrometric constants

		// read weather file
		if (!rc)
			rc = Wfile.wf_FillWDYEAR(WRN);		// read/cache entire weather file
												//   TODO: handle partial years
	}

	if (!rc)
	{	// check that weather file includes all run dates
		if (!tp_autoSizing)				// don't need run date data for autosizing
		{
			int jd1 = Wfile.jd1;					// fetch first wthr Julian date 1-365, or -1 if not known
			int jdl = Wfile.jdl >= jd1 ? Wfile.jdl : Wfile.jdl + 365;	// last wthr file date, make >= 1st date
			if ( jd1 > 0						// if wf dates known
					&&  jd1 + 364 != jdl )   				// if not a full year weather file (365-day file has all dates!)
			{
				int begDay = tp_begDay;						// fetch first Julian date 1-365 of run
				int endDay = tp_endDay >= begDay ? tp_endDay : tp_endDay + 365;	// last run Julian date: make >= 1st date
				if (begDay < Wfile.jd1) 						// eg for Feb-Mar run vs Nov-June wthr file
				{
					begDay += Wfile.jd1;  						// adjust run dates into wf dates range
					endDay += Wfile.jd1;						// (or else beyond)
				}
				if (begDay < jd1 || endDay > jdl)
					rc = per( (char *)MH_S0501,				/* "Weather file does not include all run dates:\n"
									   "    run is %s to %s,\n"
									   "    but '%s' dates are %s to %s.\n" */
							  tddys( tp_begDay),  tddys( tp_endDay),
							  Top.tp_wfName,  tddys( Wfile.jd1),  tddys( Wfile.jdl) );
			}
		}
		else
		{	// is autosizing
			// default some autosizing inputs default weather file header
			if (!IsSet( TOPRAT_HEATDSTDBO))		// if heating outdoor design temp not given by user
				switch ( Wfile.wFileFormat)		// default according to weather file type
				{
				case ET1:
					heatDsTDbO = Wfile.win97TDb;
					break;

				default:
					rc |= notGivenEr( TOPRAT_HEATDSTDBO, "when weather file is not ET1");
					break;	// NUMS
				}

			if (pHotMo)						// if pointer for return given
				switch (Wfile.wFileFormat)				// find hottest month for caller (re sorting coolDsMo)
				{
					// note caller pre-defaults.
					case ET1:
						*pHotMo = Wfile.sumMonHi;
					default:
						break;
				}

			BOOL badAuszWthr = FALSE;
			if (IsSet( TOPRAT_COOLDSMO))
				badAuszWthr = Wfile.wFileFormat != ET1;
			else if (!IsSetCount( TOPRAT_COOLDSDAY, TOPRAT_COOLDSCOND, 0))
			{	// no user input for any of TOPRAT_COOLDSxxx
				//   default to ET1 header info if available
				//   else error
				if (Wfile.wFileFormat == ET1)
				{	tp_coolDsMo[0] = Wfile.sumMonHi;
					tp_coolDsMo[1] = 0;
				}
				else
					badAuszWthr = TRUE;
			}
			if (badAuszWthr)
				rc |= per( "'coolDsDay' or 'coolDsCond' (not 'coolDsMo') is required"
							" when autosizing and weather file is not ET1");
			else if (tp_AuszWthrSource()==TOPRAT_COOLDSMO)
				warn( "Derived weather values (e.g. tGrnd and tMains) are missing when autosizing\n"
					"   conditions are specified via 'coolDsMo' (as opposed to 'coolDsDay').");
		}
	}

	// Other site related top inputs
	if (!IsSet(TOPRAT_DEEPGRNDT))
		tp_deepGrndT = Wfile.taDbAvgYr;

	if (IsSet(TOPRAT_SOILDIFF))
	{	oInfo("soilDiff is used as part of the simple ground model, which is no longer supported.\n"
			  "    Use soilCond, soilSpHt, and soilDens instead.");
		if (IsSet(TOPRAT_SOILCOND) || IsSet(TOPRAT_SOILSPHT) || IsSet(TOPRAT_SOILDENS)) {
			ignore(TOPRAT_SOILDIFF, "when soilCond and/or soilSpHt and/or soilDens are set.\n"
			"    soilDiff will be set to soilCond/(soilDens*soilSpHt)");
			tp_soilDiff = tp_soilCond / (tp_soilDens*tp_soilSpHt);
		}
	}

	return rc;
}			// TOPRAT::tp_Wfile
//-----------------------------------------------------------------------------
int TOPRAT::tp_AuszWthrSource() const		// weather source for cooling autosize
// returns TOPRAT_COOLDSMO: generated design day data for month (e.g. from ET1 header)
//         TOPRAT_COOLDSDAY: specific day read from weather file
//         TOPRAT_COOLDSCOND: user-defined DESCOND info
//         0: no source specified (assume not autosizing)
{
	int iAuszWthrSource =
		  tp_coolDsDay[ 0] > 0  ? TOPRAT_COOLDSDAY
		: tp_coolDsCond[ 0] > 0 ? TOPRAT_COOLDSCOND
		: tp_coolDsMo[ 0] > 0   ? TOPRAT_COOLDSMO
		:                         0;
	return iAuszWthrSource;
}		// TOPRAT::tp_AuszWthrSource
//-----------------------------------------------------------------------------
RC TOPRAT::tp_Ausz(		// Top autosizing setup. call after tp_Wfile
	int hotMo )		// hottest month 1-12, re sorting cooling design months
{
	RC rc = RCOK;
	tp_nDesDays = 0;
	if (!tp_autoSizing)			// not needed if not setting up for autoSizing
		return rc;

	int iAuszWthrSource = tp_AuszWthrSource();
	if (iAuszWthrSource == TOPRAT_COOLDSMO)
	{
		// count design days: 1 for heat, plus # cooling design months
		tp_nDesDays = 1;
		for (int i = 0;  tp_coolDsMo[ i]; i++)
			tp_nDesDays++;

		// sort the cooling design days to do hottest ones first. 7-14-94
		for (int i = 0;  tp_coolDsMo[ i];  i++)    	// for 0 to # cool des days
			for (int j = 1;  tp_coolDsMo[j];  j++)	// for 1 to # cool des days (NOP if < 2)
			{
				int dfi = abs(tp_coolDsMo[i] - hotMo);	// # months from month i to hottest month
				if (dfi > 6)  dfi = 12 - dfi;		// if > 6 months then measure the other way around
				int dfj = abs(tp_coolDsMo[j] - hotMo);	// j ..
				if (dfj > 6)  dfi = 12 - dfi;		// ..
				if (dfj < dfi)				// if [j] is closer to the hottest month than [i]
				{
					int tem = tp_coolDsMo[i]; 		// swap months i and j
					tp_coolDsMo[i] = tp_coolDsMo[j];
					tp_coolDsMo[j] = tem;
				}
			}
	}
	else if (iAuszWthrSource == TOPRAT_COOLDSDAY)
	{	// count design days: 1 for heat, plus # cooling design days
		tp_nDesDays = 1;
		for (int i = 0;  tp_coolDsDay[ i]; i++)
			tp_nDesDays++;
	}
	else if (iAuszWthrSource == TOPRAT_COOLDSCOND)
	{	// count design days: 1 for heat, plus # cooling design days
		tp_nDesDays = 1;
		for (int i = 0;  tp_coolDsCond[ i]; i++)
			tp_nDesDays++;
	}
	else
		rc = RCBAD;

	// derived tolerances
	auszTol2 = auszTol/2;		// half-tolerance -- half added to values, half used in convergence tests
	auszSmTol = auszTol/5;		// "small" tolerance -- apply several times w/o exceeding auszTol.
	auszHiTol2 = 1 + auszTol/2;	// multiplicative half-tolerance

	return rc;
}			// TOPRAT::tp_Ausz
//===========================================================================
RC TOPRAT::tp_Psychro()		// initialize air properties members of top

// call tp_Wfile first to default Top.elevation.
{
	// see Top member psychro functions (cnrecs.def) for uses of this stuff

// Get barometric (inches Hg) pressure for elevation: is an input to air properties
	tp_presAtm = psyElevation( elevation);	// Also sets PsyPBar for many psychro internal uses.
		// note elevation may changed in cncult5.cpp and/or cncoil.cpp
		// re sea level coil capacity calculations (then restored).

	// "reference humidity ratio" used in computing air heat capacity and enthalpy.
	// STORY: effect of typical humidity ratio (w) range on heat capacity of air is +- < 1%, small enuf to neglect
	// in this application, so all heat capacities are computed for the same w, which is computed here.
	// Inputs default to 70F, 60%RH; user might change for climate or for heat-only or cool-only application. */

	tp_refW = psyHumRat2( refTemp, refRH);	// hum rat for temp, rel humidity
	if (tp_refW <= 0.)
	{	per( (char *)MH_S0500, refTemp, elevation);	// "refTemp (%g) seems to be over boiling for elevation (%g)"
		tp_refW = PsyWmin;
	}
	tp_refWX = 1./(1.+tp_refW);		// commonly used

// specific heat of air @ refW: .240 * .444*w Btu/lb-F
// using public constants in psychro.cpp
	tp_airSH = PsyShDryAir + PsyShWtrVpr*tp_refW;

// factors for air specific volume, density, and volumetric specific heat for temperature, @ tp_wRef.
	// (Rair + wRef * RWtrVpr) / P
	airVK = PsyRAir*(1.f + tp_refW/PsyMwRatio)/PsyPBar;	// specific volume per temp: multiply by abs temp to get ft3/lb
	airRhoK = 1.f/airVK;			// divide this by abs temp to get density lb/ft3.
	airVshK = airRhoK*tp_airSH; 	// divide by abs temp to get heat capacity per volume: Btu/ft3-F
	airXK = airVshK*60.f;			// divide by abs temp for heat transfer of flow: Btuh/cfm-F. 60: min/hr.

//
	tp_hConvF = tp_hConvMod == C_NOYESCH_YES ? 0.24f + 0.76f * tp_presAtm / PsyPBarSeaLvl : 1.0;
						//   convection coefficient modification factor
						//   used in some surface transfer models; reduces convection as function of elevation
						//   source = ASHRAE 1199-RP Barnaby et al. 2004

	return RCOK;
}					// TOPRAT::tp_Psychro
//===========================================================================
#ifdef BINRES // cnglob.h
RC TOPRAT::brFileCk()	// check/clean up inputs re binary results files, rob 12-2-93
{
// error if hourly results req'd w eg Jan 15..Jan 14 run: can't save hourly results for same month twice. 12-94.
	if ( tp_brHrly==C_NOYESCH_YES			// ok if no bin results; believe ok for basic bin res only
			&&  nDays > 365-31 )			// eg Jan 1 - Jan 31 run is ok
	{
		IDATE iDateBeg, iDateEnd;
		tddyi( tp_begDay, -1, &iDateBeg);		// break down Julian date, tdpak.cpp
		tddyi( tp_endDay, -1, &iDateEnd);
		if (iDateBeg.month==iDateEnd.month)	// per() stops run due to global error count
			per( (char *)MH_S0502);		/* "Run over a month long cannot end in starting month\n"
						   "    when hourly binary results are requested." */
		return RCBAD;				// omit following warnings after fatal error
	}

// clean up binary results file name if given

	if ( sstat[TOPRAT_BRFILENAME] & FsVAL  	// if binary result filename given (and stored -- insurance)
			&&  *tp_brFileName )				// and not just "" to negate filename in earlier run
	{
		char *s = strffix( tp_brFileName, "");	// standardize: deblank, uppercase. "": no default extension. to TmpStr.
		char *dot = strrchr( s, '.');		// point last period in pathName
		if (dot > strchr( s, '\\') && *(dot+1))	// if file extension given (not . in a dir name, not final . )
		{
			// warning for extension given (won't be used)

			pWarn( (char *)MH_S0503,		// "Extension given in binary results file name \"%s\" \n"
				   tp_brFileName );			// "    will not be used -- extensions \".brs\" and \".bhr\" are always used."
			*(dot + 1) = '\0';			// remove the extension so warning does not repeat
		}
		if (!*strpathparts( s, STRPPDRIVE|STRPPDIR))	// if contains no drive nor directory (strpak.cpp fcn)
			s = strtPathCat( InputDirPath, s);		// default to INPUT FILE path (rundata.cpp variable) 2-95
		if (strcmpi( s, tp_brFileName))		/* store only if different: reduce fragmentation if no change,
						   if 2nd run, if redundant call, etc. (strcmpI 11-94; believe moot.) */
		{
			cupfree( DMPP( tp_brFileName));	// dmfree it if not a pointer to "text" embedded in pseudocode. cueval.cpp.
			tp_brFileName = strsave(s);		// copy s to new heap block, store pointer thereto.
		}

// warnings if file name given when it won't be used (why tp_SetOptions() must be called first, 12-94)

		if (tp_brs != C_NOYESCH_YES && tp_brHrly != C_NOYESCH_YES)
			pWarn( (char *)MH_S0504, 		// "You have given a binary results file name with\n"
				   tp_brFileName );			// "        \"BinResFileName = %s\",\n"
		// "    but no binary results file will be written as you have not given\n"
		// "    any of the following commands:\n"
		// "        BinResFile = YES;  BinResFileHourly = YES;\n"
		// "    nor either of the following command line switches:\n"
		// "        -r   -h\n"
		else if (tp_brMem && !tp_brDiscardable)
			pWarn( (char *)MH_S0505, 		// "You have given a binary results file name with\n"
				   tp_brFileName );			// "        \"BinResFileName = %s\",\n"
		// "    but no binary results file will be written as you have also specified\n"
		// "    memory-only binary results with the -m DLL command line switch."
	}
	return RCOK;
}		// TOPRAT::brFileCk
#endif
//===========================================================================
/*virtual*/ void TOPRAT::Copy(const record* pSrc, int options/*=0*/)
{
	options;
	if (gud)		// if record already in use (eg 2nd run) (insurance).  Note record must be constructed b4 operator=.
		freeDM();

	record::Copy( pSrc);				// verifies class (rt) same, copies whole derived class record. ancrec.cpp.

	cupIncRef( DMPP(tp_wfName));   	// incr reference counts of dm strings if nonNULL
	cupIncRef( DMPP(tp_TDVfName));  
	cupIncRef( DMPP(runTitle));		//  cupIncRef: cueval.cpp: if pointer is not NANDLE nor pointer to
	cupIncRef( DMPP(runDateTime));	//   "text" embedded in pseudocode, call dmpak:dmIncRef to increment
	cupIncRef( DMPP(repHdrL));		//   block's reference count (or dup block).
	cupIncRef( DMPP(repHdrR));
	cupIncRef( DMPP(dateStr));
	cupIncRef( DMPP( tp_repTestPfx));
	cupIncRef( DMPP( tp_progVersion));
	cupIncRef( DMPP( tp_HPWHVersion));
	cupIncRef( DMPP( tp_exePath));
	cupIncRef( DMPP( tp_exeInfo));
	cupIncRef( DMPP( tp_cmdLineArgs));
#ifdef BINRES
	cupIncRef( DMPP( tp_brFileName));
#endif
	// monStr does not point into dm.

}				// TOPRAT::Copy
//===========================================================================
TOPRAT::~TOPRAT()
{
	freeDM();
}			// TOPRAT::~TOPRAT
//-----------------------------------------------------------------------------
void TOPRAT::freeDM()		// free child objects in DM
{
	cupfree( DMPP( tp_wfName));			// decRef or free dm (heap) strings
	cupfree( DMPP( tp_TDVfName));		// decRef or free dm (heap) strings
	cupfree( DMPP( runTitle));			//  cupfree: cueval.cpp: if pointer is not NANDLE nor pointer to
	cupfree( DMPP( runDateTime));  		//   "text" embedded in pseudocode, call dmpak:dmfree to free its block
	cupfree( DMPP( repHdrL)); 			//   or decrement its ref count, and NULL the pointer here.
	cupfree( DMPP( repHdrR));
	cupfree( DMPP( dateStr));
	cupfree( DMPP( tp_repTestPfx));
	cupfree( DMPP( tp_progVersion));
	cupfree( DMPP( tp_HPWHVersion));
	cupfree( DMPP( tp_exePath));
	cupfree( DMPP( tp_exeInfo));
	cupfree( DMPP( tp_cmdLineArgs));
	// monStr is not in heap.
#ifdef BINRES
	cupfree( DMPP(tp_brFileName));
#endif
	delete tp_pAirNet;
	tp_pAirNet = NULL;

	tp_PumbraDestroy();		// cleanup / delete Penumbra shading machinery

	// monStr does not point into dm.
}		// TOPRAT::freeDM
//---------------------------------------------------------------------------
const char* TOPRAT::When( IVLCH ivl) const		// date / time doc for error messages
// result is in TmpStr[] = transient / do not delete
{
	const char* dateStrX = dateStr ? dateStr : "(no date)";
	const char* s = NULL;
	switch (ivl)
	{
	case C_IVLCH_Y:
		 s = strtmp( "year");
		break;
	case C_IVLCH_M:
		s = strtmp( monStr);
		break;
	case C_IVLCH_D:
		s = strtmp( dateStrX);
		break;
	case C_IVLCH_H:
		s = strtprintf( "%s hour %d", dateStrX, iHr+1 );
		break;
	case C_IVLCH_S:
		s = strtprintf( "%s hour %d subHour %d", dateStrX, iHr+1, iSubhr);
		break;
	default:
		s = strtmp( "TOPRAT::When() bug!");
	}
	return s;
}			// TOPRAT::When
//-----------------------------------------------------------------------------
LOCAL RC topDC()		// copy input DESCONDs to run records
{	RC rc = DcR.RunDup( DCiB);
	return rc;
}	// topDC
//-----------------------------------------------------------------------------
LOCAL RC topZn(		// all-zone initialization
	int re)		// 0: initial setup
				// 1: resetup (after autosize)
{
	ZNI* zip;
	RC rc=RCOK;	// rc is used in E macro

// clr XSURFs, ZHXs, zones run RATs: insurance: poss prior run leftovers, etc

	// traditional code clears after run; could remove this duplication.
	XsB.free();					// clear XSURFs. ratpak.cpp.  records are added to XsB by cncult3.cpp:cnuCompAdd.
	CSE_E( ZrB.al( ZiB.n, WRN)	)	// zones: delete old records, alloc nec size now for minimum fragmentation
	// E macro: if returns bad, return bad to caller.
	// CAUTION: may not errCount++, be sure error return propogated back to cul:cuf.
	// zones are not owned (names must be unique); leave .ownB 0.

	ZhxB.free();		// clear ZHXs
	ZhxB.ownB = &ZrB;	// zhx's are 'owned' by zones even tho ambiguous (and have terminal name if for tu).
						//   .ownB's ALSO SET in cnguts:cgPreInit, for -p (thus redundant here); change both places.
    					//	 And see: zhx.ownTi set in cncult5:addZhx

	// zone loop: defaults, copy input data, initialize infil, vent, fans
	//   counts of zones by type (for local error checking)
	int nCR = 0;	//  # convective / radiant
	int nALL = 0;	//  # any type
	RLUP( ZiB, zip)			// loop input zones (ancrec.h macro)
	{
		nALL++;
		nCR += zip->i.zn_IsConvRad();
		rc |= zip->zi_Top( re);
	}	// zones loop

	Top.tp_bAllCR = nCR > 0;
	if (nCR > 0 && nCR != nALL)
		rc = err( "Illegal znModel mix -- must be all CNE or all convective/radiant");

	return rc;				// error returns above in each E macro
}			// topZn
//-----------------------------------------------------------------------------
RC ZNI::zi_Top(			// top-level input check and defaults
	int re)		// re-setup flag
				//  0: initial setup (after input)
				//  1: re-setup after autosize
// returns RCOK
{
	UCH* fs = fStat();		// zone's field status byte array

	i.zn_hcAirXIsSet = IsSet( ZNI_I+ZNISUB_HCAIRX);	// 1 iff zn_hcAirX is set
										//  WHY: ZNI status bytes not  passed to ZNR
										//       Need status in zn_AirX()

	if (!(fs[ ZNI_I+ZNISUB_ZNAREA] & FsSET))
		ooer( ZNI_I+ZNISUB_ZNAREA, "no znArea given");

	if (!(fs[ ZNI_I+ZNISUB_ZNVOL] & FsSET))
		ooer( ZNI_I+ZNISUB_ZNVOL, "no znVol given");

	// default znCAir based on area
	if ( !(fs[ZNI_I+ ZNISUB_ZNCAIR] & FsSET))	// if znCAir not input
		i.znCAir = 3.5f * i.znArea; 	// CAir = 3.5 * Area
	// note: other mass can be added to znCAir (e.g. sf_TopSf2() quick wall mass)

	// default ceiling height based on volume and area
	if ( !(fs[ZNI_I+ ZNISUB_CEILINGHT] & FsSET)		// if znCeilingHt not input
			&& i.znArea > 0.f)
		i.zn_ceilingHt = i.znVol / i.znArea;

	// repeat checks done by input ckf's
	// in case skipped due to expr, and as gel insurance.  ooer minimizes duplicate msgs.
	if (i.zn_infShld<1 || i.zn_infShld>5)
		ooer( ZNI_I + ZNISUB_INFSHLD, (char *)MH_S0471, i.zn_infShld);	// "zn_infShld = %d: not in range 1 to 5"

	if (i.zn_infStories<1 || i.zn_infStories>3)
		ooer( ZNI_I + ZNISUB_INFSTORIES, (char *)MH_S0472, i.zn_infStories); 	// "infStories = %d: not in range 1 to 3"

	// default eave height
	if (!(fs[ZNI_I + ZNISUB_EAVEZ] & FsSET))
		i.zn_eaveZ = i.zn_floorZ + i.zn_infStories * 8.f;

	if (!(fs[ ZNI_I + ZNISUB_WINDFLKG] & FsSET))
		// wind factor for infiltration and Airnet
		//  0 if zn_eaveZ < 0
		i.zn_windFLkg = Top.tp_WindFactor( i.zn_HeightZ( 1.f), i.zn_infShld);

#if defined( ZONE_XFAN)
	i.xfan.fn_setup(
		C_FANAPPCH_XFAN,		// application
		this,					// record containing fan
		ZNI_I+ZNISUB_XFAN,		// idx of initial fan status
		-1.f,					// default flow
		NULL);					// source FAN for default curve
#endif
	if (!re)
		i.vrZdd = 0;		// additional runs re-allocate VRs, clear any existing
							//   (but not between autosize and sim!)

	// copy data from zones input rat.  Specify subscripts to insure match.
	ZNR* zp;
	ZrB.add( &zp, ABT, ss );		// add zone run record. expect no error due to ZrB.al above.
	zp->CopyFrom( this);	// copy record except internal front overhead.  Not '=': records different.
							// DEPENDS ON start of ZNR being same as ZNI.  ---> DOES NOT handle DM strings.

	// zero surface area summations and other totals re view factors and solar gain targetting
	//   values are accumulated later in zone and surface setup
    zp->zn_ductCondUANom = 0.;  // zone total duct conduction nominal UA (accum'd in ds_TopDS())
	zp->zn_InitSurfTotals();

	// zone Shade closure: set flag if given, else active default is done at run time.
	if (fs[ZNI_I + ZNISUB_ZNSC] & FsSET)	// if entered by user (whether constant or variable)
		zp->znSCF = 1;						// non-0 flag suppresses setting in cnloads.cpp

	// init zone's infiltration
	zp->zn_InfilSetup();			// infil setup, cgcomp.cpp

#ifdef COMFORT_MODEL
	if (fs[ZNI_I + ZNISUB_ZNCOMFCLO] & FsSET)
	{	// if not CZM zone, error?
		zp->zn_pComf = new CThermalComfort;
		zp->i.znComfUseZoneRH = !(fs[ ZNI_I+ZNISUB_ZNCOMFRH] & FsSET);
	}
#endif

	return RCOK;

}		// ZNI::zi_Top
//==============================================================================
#if 0	// not needed til find or add members needing cupIncRef/cupfree, 7-17-92
x record& ZNR::operator=(record& src)	// overrides record::operator=, decl must match. for same record type only. use unexpected.
x{
x}	// ZNR::operator=
x //===========================================================================
x record& ZNR::CopyFrom( record& src, int copyName=1, int dupPtrs=0)
x // copy contents & front info from a record possibly of different deriv class.  Overrides record::CopyFrom (decl must match)
x // used to copy ZNI into ZNR to init larger zone run records from zone input records
x{
x}	// ZNR::CopyFrom
x //===========================================================================
x ZNI::~ZNI()
x{
x}	// ZNI::~ZNI
#endif //11-95, at addition of ZNR.rIgDist.
//===========================================================================
ZNR::ZNR( basAnc* b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ), zn_sbcList()
{
}	// ZNR::ZNR
//---------------------------------------------------------------------------
ZNR::~ZNR()		// zone runtime info record destructor
{
	zn_sbcList.clear();			// clean up surface property array
	dmfree( DMPP( rIgDist));	// dec ref count or free Radiant Internal Gain Distribution info table heap block
#ifdef COMFORT_MODEL
	delete zn_pComf;			// comfort model interface object (if allocated)
	zn_pComf = NULL;
#endif
	rIgDistNAl = 0;				// insurance: say no allocated entries in rIgDist table
	rIgDistN = 0;				// insurance: say no used entries in rIgDist table
}			// ZNR::~ZNR
//---------------------------------------------------------------------------
void ZNR::Copy( const record* pSrc, int options/*=0*/)
{
// first free pointed-to heap objects in destination
	if (gud)  				// if a constructed record
	{
		dmfree( DMPP( rIgDist));
	}
	zn_sbcList.clear();

// use base class Copy
	record::Copy( pSrc, options);		// verifies class (rt) same, copies whole derived class record. ancrec.cpp.

// increment ref counts of pointed-to objects
	dmIncRef( DMPP(rIgDist));	// Radiant Int Gain Distribution info table in heap. Not string-->CupIncRef not appropriate

#ifdef COMFORT_MODEL
	zn_pComf = NULL;			// prevent duplicate pointer to same CComfortCalc
								//   will be reallocated if needed
#endif
}				// ZNR::Copy
//===========================================================================
LOCAL RC topIz()		// do interzone transfers

// must be called before topSf2 and topDs (which also make IzxR entries)
// to be sure subscripts matching IzxiB are available
{
	RC rc = RCOK;

	// clear izxfer run RAT.   nothing to dmfree for izxfers, right?
	CSE_E( IzxR.al( IzxiB.n, WRN) );		// delete old records, alloc to needed size now for min fragmentation.

	// leave IzxR.ownB 0 because interzone transfers are not owned (right?).
	// CAUTION 1-7-91 record format looks like rcdef may consider them to be owned (by first zone);
	// thus input and runtime ambiguous name handling may differ. */

	// copy izxfer input rat IzxiB entries to runtime rat IzxR, check, initialize

	IZXRAT* izie;
	IZXRAT* ize;
	RLUP( IzxiB, izie)		// loop input interzone transfers RAT records
	{	IzxR.add( &ize, ABT, izie->ss);   	// add specified izxfer run record (same subscript)
		rc |= ize->iz_Setup( izie);			// copy input to run record; check and initialize
	}	// izxfer loop
	return rc;
}		// topIz
//===========================================================================
LOCAL RC topDs()		// do duct segments

// must be called after topIz
{
	RC rc = RCOK;

	// clear DUCTSEG run RAT.
	CSE_E( DsR.al( DsiB.n, WRN) );		// delete old records, alloc to needed size now for min fragmentation.

	// copy input rat entries to runtime rat, check, initialize
	DUCTSEG* pDSi;
	DUCTSEG* pDS;
	RLUP( DsiB, pDSi)		// loop input records
	{	DsR.add( &pDS, ABT, pDSi->ss);	// add specified run record (same subscript)
		rc |= pDS->ds_TopDS( pDSi);		// copy input to run record; check and initialize
	}
	return rc;
}		// topDs
//===========================================================================
LOCAL RC topMat()

// process MATerials at RUN verify conductivity set, compute vhc,
{
	MAT *mat;
	RC rc=RCOK;

	RLUP( MatiB, mat)			// loop MATerial records
	{
		// verify RQD conductivity set, to prevent FPE in topCon1() --
		// in case called even if messages before RUN encountered. */
		rc |= mat->CkSet( MAT_COND);	// msg iff unset and msg count is 0.

		// volumetric heat capacity = specific heat * density (0 defaults)
		mat->mt_vhc = mat->mt_spHt * mat->mt_dens;
	}
	return rc;
}		// topMat
//===========================================================================
LOCAL RC topCon1()

// at RUN, constructions pass 1:
// zero members to which topLr() accumulates.
{
	CON *con;
	RC rc=RCOK;

// loop over CONstruction records, clearing members topLr accumulates into */

	RLUP( ConiB, con)		// loop CONstruction input RAT records
	{
		con->nLr = 0;		// number of layers
		con->nFrmLr = 0;	// number of framed layers (> 1 is error)
		con->r = 0.f;		// resistance: sum of layers (weighted for framed layer), possibly -1 if infinite.
		con->hc = 0.f;		// heat capacity per unit area
		con->rNom = 0.f;  	// nominal (insulation) r-value: sum layers
	}
	return rc;
}		// topCon1
//===========================================================================
LOCAL RC topLr()	// process layers at RUN
{
	RC rc=RCOK;
	LR* lr;
	RLUP( LriB, lr)				// loop LAYER records
		rc |= lr->lr_TopLr();
	return rc;
}		// topLr
//-----------------------------------------------------------------------------
RC LR::lr_TopLr()
{
	CON *con;
	MAT *mat, *frmMat=NULL;
	RC rc=RCOK;

	int thkSet = IsSet( LR_THK);  			// nz iff layer thickness given
	int matSet = IsSet( LR_MATI);			// nz iff matrl given
	int frmMatSet = IsSet( LR_FRMMATI);		// nz if framing matrl given
	int frmFracSet = IsSet( LR_FRMFRAC);	// nz if fram fraction given
	float frmFrac = frmFracSet ? lr_frmFrac : 0.f;	// framing frac or 0
	int framed = frmFrac > 0.f;				// nz if fram frac > 0 given, (optional mbr, dfl 0)

	// layer: check/access owning construction.  Message & set rc if bad.

	if (ckOwnRefPt( &ConiB, this, (record **)&con, &rc) )
		return rc;			// bad ref (msg issued, rc set, run stopped)

	// layer: check/access (primary) material

	if (matSet)						// if material given (RQD)
	{
		if (ckRefPt( &MatiB, lr_mati, "lrMat", NULL, (record **)&mat, &rc) )
			return rc;						// on bad matl, proceed to next layer
	}
	else	// layer material not given.  Already msg'd at end of material object, unless
			//  material is part of a construction type or copy;  issue another message to be sure. */
	{
		rc |= ooer( LR_MATI, (char *)MH_S0477);  	// "lrMat not given". msg only if no msg yet for this field.
		return rc;						// do other layers, then end session
	}
	int matThkSet = mat->IsSet( MAT_THK);		// nz if mt_thk given

	// layer: check/access framing material if given

	int frmMatThkSet = 0;
	if (frmMatSet)					// if framing material given
	{
		if (ckRefPt( &MatiB, lr_frmMati, "lrFrmMat", NULL, (record **)&frmMat, &rc) )
			return rc;
		// nominal R value in framing unexpected & not used. Rob.
		if (frmMat->mt_rNom > 0.f)
			oWarn( (char *)MH_S0478,			// "Ignoring unexpected mt_rNom=%g of framing material '%s'"
				   frmMat->mt_rNom, frmMat->name );
		// test whether thickness given in framing material
		frmMatThkSet = frmMat->IsSet( MAT_THK);	// non-0 if framing material thickness given
	}

	// layer: framing fraction / framing material given/omitted consistency.  duplicates lrStarCkf check, as required.

	if (framed && !frmMatSet)
		rc |= oer( (char *)MH_S0479, frmFrac);			// "Non-0 lrFrmFrac %g given but lrFrmMat omitted."
	// continue here now, but end session upon return (due to rc).
	else if (!frmFracSet && frmMatSet)
		oer( (char *)MH_S0480);					// "lrFrmMat given, but lrFrmFrac omitted"
	// and proceed w framing fraction 0 (oer prevented RUN).
	// else: framing fraction of 0 given: framing material optional.

	/* layer: resolve/check thicknesses: thickness may be given in layer,
	   material, or framing material; if more than 1 given, they must match.*/

	if (thkSet)					// if layer thickness given
	{
		if ( matThkSet				// if mat'l thk also given
		  && mat->mt_thk != lr_thk )		// must be the same
			oer( (char *)MH_S0481,			// "Material thickness %g does not match lrThk %g"
				 mat->mt_thk, lr_thk );
		// continue for now; oer() prevents RUN.
		if ( frmMatThkSet				// if frmMat & its thk given
		  && frmMat->mt_thk != lr_thk )		// it too must be the same
			oer( (char *)MH_S0482, 		// "Framing material thickness %g does not match lrThk %g"
				 frmMat->mt_thk, lr_thk );
		// continue for now; oer() prevents RUN.
	}
	else if (matThkSet)
	{
		lr_thk = mat->mt_thk;					// copy mat'l thk to layer
		if ( frmMatThkSet						// if framing mat'l thk given
	      && frmMat->mt_thk != mat->mt_thk )				// must be the same
			oer( (char *)MH_S0483, frmMat->mt_thk, mat->mt_thk);	/* "Framing material thickness %g does not match \n"
								   "    material thickness %g" */
		// continue for now; oer() prevents RUN.
	}
	else if (frmMatThkSet)			// if framing mat'l thickness given
	{
		lr_thk = frmMat->mt_thk;		// use it
	}
	else					// have no thickness
		rc |= oer( (char *)MH_S0484,				// "No thickness given in layer, nor in its material%s"
				   frmMatSet ? ", nor its framing material" : "" );

	// layer: message if > 1 framed layers in construction

	if (framed)
	{
		if (con->nFrmLr)
			oer( (char *)MH_S0485);  			// "Only 1 framed layer allowed per construction"
		// keep processing -- but oer has prevented run.
	}

	// layer: compute (framed) conductivity and vhc
	// don't get here if mat'l mt_cond not given (see topMat())
	if (framed && frmMatSet)
	{
		lr_uvy = mat->mt_cond * (1.f - frmFrac) + frmMat->mt_cond * frmFrac;
		lr_vhc = mat->mt_vhc * (1.f - frmFrac) + frmMat->mt_vhc * frmFrac;
	}
	else
	{
		lr_uvy = mat->mt_cond;		// conductivity: u for 1 foot thick
		lr_vhc = mat->mt_vhc;		// volumetric heat capacity
	}

	// layer: compute conductance and resistance for thickness
	// don't get here if lr_thk or matConds not set and > 0 (LMGZ); thus if here uvy is also > 0.f.
	if (lr_thk <= 0.f)		// insurance: not expected.
	{
		lr_r = 0;			// no resistance
	}
	else				// > 0 thickness
	{
		float u = lr_uvy/lr_thk;	// u for actual thickness
		if (u <= 0.f)			// insurance - unexpected
			lr_r = FHuge;		// no u, huge r
		else					// > 0 conductivity
			lr_r = 1.f/u;		// r for actual thickness
	}

	// layer: accumulate construction values: layer counts, r-value, rNom

	con->nLr++;			// count layers in construction
	if (framed)
		con->nFrmLr++;		// count framed layers (flag; error if > 1)
	con->r += lr_r;			// accumulate resistance (series r is faster to compute than u)
	con->hc += lr_vhc * lr_thk;		// heat capac per unit area
	if (mat->mt_rNom > 0.f)			// mat nominal R-val: -1 if not given
		// layer nominal r value = mt_rNom * lr_thk;  con rNom = sum layers.
		//   rNom from cavity only used. Warned above if frmMat rNom given.
		con->rNom += mat->mt_rNom * lr_thk;

	return rc;
}		// LR::lr_TopLr
//===========================================================================
LOCAL RC topCon2()		// constructions pass 2, at RUN
{
	CON *con;
	RC rc=RCOK;

	RLUP( ConiB, con)			// loop CONstruction records
	{
		UCH* fs = (UCH *)con + ConiB.sOff;	// field status byte array

		// construction: error if > 1 framed layer

		if (con->nFrmLr > 1)		// counted in topLr.  REDUNDANT MSG
			con->oer( (char *)MH_S0486);  // "More than 1 framed layer in construction"

		// construction: error if both or neither layer subobjects and .conU given.  duplicates conStarCkf check.

		if (fs[ CON_CONU ] & FsSET)	// if .conU given
		{
			if (con->nLr)			// if layers given (counted in topLr)
				con->oer( (char *)MH_S0487);	// "Both conU and layers given"
			// proceed, but oer() has prevented run.
		}
		else				// .conU not given
			if (con->nLr==0)			// if no layers given
				con->oer( (char *)MH_S0488);	// "Neither conU nor layers given"
		// proceed, but oer() has prevented run.

		// construction: if layers given, compute U from r accumulated in topLay()

		if (con->nLr)			// if layer subobjects given
			if (con->r > 0.f)		// insurance
				con->conU = 1.f / con->r;	// conductance = 1/resistance
			else 				// 0 resistance (unexpected)
				con->conU = FHuge;		// large value for inf conductance
		/* else leave conU as input (checked above) if 0 layers. */

#if 0	// 1-95: incorporate in topSf1 where actual surface film h's known
x   // construction: classify re possible surface models (for topSf1())
x       con->conClass = conClass(con);	// local fcn below
#endif
	}
	return rc;
}		// topCon2
		//===========================================================================
LOCAL RC topFnd()		// foundations
{
	FOUNDATION *fnd;
	RC rc = RCOK;

	RLUP(FndiB, fnd)
	{
		// populate base Kiva foundation object (will be copied for specific instances of Kiva)
		if (!fnd->fd_kivaFnd)
		{
			fnd->fd_kivaFnd = new Kiva::Foundation();
			if (!fnd->fd_kivaFnd)
				rc = RCBAD;		// not expected
		}

		Kiva::Foundation* kFnd = fnd->fd_kivaFnd;
		kFnd->soil = kivaMat(Top.tp_soilCond, Top.tp_soilDens, Top.tp_soilSpHt);
		kFnd->grade.absorptivity = 1 - Top.grndRefl;
		kFnd->grade.emissivity = Top.tp_grndEmit;
		kFnd->grade.roughness = LIPtoSI(Top.tp_grndRf);
		kFnd->farFieldWidth = LIPtoSI(Top.tp_farFieldWidth);
		kFnd->deepGroundDepth = LIPtoSI(Top.tp_deepGrndDepth);
		
		kFnd->deepGroundBoundary = Top.tp_deepGrndCnd == C_DGCH_ZEROFLUX ?
			Kiva::Foundation::DGB_ZERO_FLUX :
			Kiva::Foundation::DGB_FIXED_TEMPERATURE;

		kFnd->wall.heightAboveGrade = LIPtoSI(fnd->fd_wlHtAbvGrd);
		kFnd->wall.depthBelowSlab = LIPtoSI(fnd->fd_wlDpBlwSlb);

		kFnd->mesh.minCellDim = LIPtoSI(Top.tp_grndMinDim);
		kFnd->mesh.maxNearGrowthCoeff = Top.tp_grndMaxGrthCoeff;
		kFnd->mesh.maxDepthGrowthCoeff = Top.tp_grndMaxGrthCoeff;
		kFnd->mesh.maxInteriorGrowthCoeff = Top.tp_grndMaxGrthCoeff;
		kFnd->mesh.maxExteriorGrowthCoeff = Top.tp_grndMaxGrthCoeff;
	}
	return rc;
}		// topFnd
		//===========================================================================
LOCAL RC topGt()	// check/default GTs (glazing types) and copy records to run basAnc.
{
	RC rc /*=RCOK*/;

// clear run basAnc, alloc to needed size
	CSE_E( GtB.al( GtiB.n, WRN))		// delete old records, alloc at once to needed size for min fragmentation.

// check/default input records and copy to runtime records

	GT *gti;
	RLUP( GtiB, gti)			// loop input meter RAT records (cnglob.h macro, skips not-guds)
	{
		// recall end-of-object check function: insurance (don't get here if it error'd earlier, but might not have if defTyping)
		RC rc1 = gtStarCkf( NULL, (void *)gti, NULL, NULL);	// RCOK (0) if ok. cncult.cpp.
		rc |= rc1;
		if (rc1)   continue;					// if error. message already issued.

#if 1//6-1-95 split gtSHGCO/C into gtSHGC and wn/gtSMSO/C. gtSMSO was RQD, now defaults to 1.0 in CULT table.
		// check or default gtSMSC: Shades-Closed Solar heat gain coefficient Multiplier

		if (gti->sstat[GT_GTSMSC] & FsSET)   		// if optional gtSMSC given (SHGC multiplier for shades closed)
		{
			if ( !ISNANDLE(gti->gtSMSO)			// if expressions given,
					&&  !ISNANDLE(gti->gtSMSC)			// .. don't check: FPE's!!!
					&&  gti->gtSMSC > gti->gtSMSO )
				gti->oer( (char *)MH_S0506,		/* "gtSMSC (%g) > gtSMSO (%g):\n"
							   "    Shades-Closed Solar heat gain coeff Multiplier\n"
							   "      must be <= shades-Open value." */
					 gti->gtSMSC, gti->gtSMSO );
		}
		else  						// optional gtSMSC not given
			VD gti->gtSMSC = VD gti->gtSMSO;     		// default it to gtSMSO.  Move as NANDAT not FLOAT so won't FPE.
#else
x   // check or default shades-closed SHGC (Solar Heat Gain Coefficient)
x
x       if (gti->ckSet( GT_GTSHGCO)==RCOK)		// verify RQD gtSHGCO set (else FPE!) (SHGC, shades open)
x		{
x          if (gti->sstat[GT_GTSHGCC] & FsSET)		// if optional gtSHGCC given (SHGC, shades closed)
x          { if ( !ISNANDLE(gti->gtSHGCO)		// if expressions given,
x              &&  !ISNANDLE(gti->gtSHGCC)		// .. don't check: FPE's!!!
x              &&  gti->gtSHGCC > gti->gtSHGCO )
x                oer( gti, "gtSHGCC (%g) > gtSHGCO (%g):\n"
x			     "    shades-Closed Solar Heat Gain Coeff must be <= shades-Open SHGC.",
x		          gti->gtSHGCC, gti->gtSHGCO );
x			}
x			else  					// optional gtSHGCC not given
x             V gti->gtSHGCC = V gti->gtSHGCO;     	// default it to gtSHGCO.  Move as NANDAT not FLOAT so won't FPE.
x		}
#endif

		// copy data to run basAnc, same subscript
		GT *gt;
		GtB.add( &gt, ABT, gti->ss);	// add glazing type run record. error not expected due to preceding al.
		*gt = *gti;			// copy entire record incl name
	}
	return rc;
}		// topGt
//===========================================================================
LOCAL RC topMtr()	// check/dup all types of meters (energy, dhw, airflow)
// copy to run rat.  Create sum-of-meters records as needed
{
	RC rc;

	CSE_E( MtrB.RunDup(MtriB, NULL, 1));	// 1 extra for sum

// initialize sum-of-meters record -- last record in meters run RAT.
	MTR* mtr;
	MtrB.add( &mtr, ABT, MtriB.n+1, "sum_of_meters");

// DHW meters
	CSE_E(WMtrR.RunDup(WMtriB, NULL, 1));

// Air flow meters
	CSE_E( AfMtrR.RunDup(AfMtriB, NULL, 1));
	AFMTR* pAM;
	AfMtrR.add(&pAM, ABT, AfMtriB.n + 1, "sum_of_AFMETERs");

	return rc;
}		// topMtr
//-----------------------------------------------------------------------------
LOCAL RC topRSys1()	// check RSYS / copy to run rat
{
	RC rc = RCOK;
	CSE_E( RsR.al( RSiB.n, WRN) )	// delete old records, alloc to needed size now for min fragmentation
	if (rc == RCOK)
	{	RSYS* pRSi;
		RLUP( RSiB, pRSi)
		{	RSYS* pRS;
			RsR.add( &pRS, ABT, pRSi->ss); 	// add run RSYS record.  error not expected due to preceding al.
			*pRS = *pRSi;					// copy input record to run record including name
			rc |= pRS->rs_TopRSys1();
		}
	}
	return rc;
}	// ::topRSys1
//-----------------------------------------------------------------------------
LOCAL RC topRSys2()	// additional RSYS check / setup
{
	RC rc = RCOK;
	RSYS* pRS;
	// loop run records (added by topRSys2())
	RLUP( RsR, pRS)
		rc |= pRS->rs_TopRSys2();
	return rc;
}	// ::topRSys2
//=============================================================================
LOCAL RC topDHW()		// check DHWSYS/DHWHEATER ... copy to run rat
{
	RC rc = RCOK;

	CSE_E( WsR.RunDup( WSiB));
	CSE_E(WhR.RunDup(WHiB, &WsR))
	CSE_E(WlhR.RunDup(WLHiB, &WsR))
	CSE_E(WtR.RunDup(WTiB, &WsR))
	CSE_E(WrR.RunDup(WRiB, &WsR))
	CSE_E(WpR.RunDup(WPiB, &WsR))
	CSE_E(WlR.RunDup(WLiB, &WsR))
	CSE_E(WgR.RunDup(WGiB, &WlR))
	CSE_E(WbR.RunDup(WBiB, &WgR))
	CSE_E(WlpR.RunDup(WLPiB, &WlR))
	CSE_E(WduR.RunDup(WDUiB, &WsR))
	CSE_E(WuR.RunDup(WUiB, &WduR))
	CSE_E(SwhR.RunDup(SWHiB))
	CSE_E(ScR.RunDup(SCiB, &SWHiB))

	// DHWDAYUSE one-time inits
	//   2 passes req'd (see wdu_Init())
	DHWDAYUSE* pWDU;
	RLUP(WduR, pWDU)
		rc |= pWDU->wdu_Init( 0);
	RLUP(WduR, pWDU)
		rc |= pWDU->wdu_Init( 1);
	// if (!rc)
	//    return rc	 ... no, keep going ws_Init() checks are independent

	// DHWSYS initialization -- do *last* -- all children in place
	//   multiple passes re e.g. wsLoadShareDHWSYS
	//   give up at end of pass if error
	DHWSYS* pWS;
	RLUP( WsR, pWS)
		rc |= pWS->ws_Init( 0);
	if (!rc) RLUP( WsR, pWS)
		rc |= pWS->ws_Init( 1);
	if (!rc) RLUP( WsR, pWS)
		rc |= pWS->ws_Init( 2);


	// DHWSOLARSYS
	//   sw_Init() calls child DHWSOLARCOLLECTOR sc_Init
	DHWSOLARSYS* pSW;
	RLUP(SwhR, pSW)
		rc |= pSW->sw_Init();

	return rc;
}	// ::topDHW
//=============================================================================
LOCAL RC topInverse()		// inverses
{
	RC rc = RCOK;
	INVERSE* pI;
	RLUP(IvB, pI)
		rc |= pI->iv_Init();

	return rc;

}	// ::topInverse
//=============================================================================
LOCAL RC topPV()		// check PVARRAY ... copy to run rat
{
	RC rc = RCOK;

	CSE_E(PvR.RunDup(PViB));

	PVARRAY* pPV;
	RLUP(PvR, pPV)
		rc |= pPV->pv_Init();

	return rc;
}	// ::topPV
//=============================================================================
LOCAL RC topBT()		// check BATTERY ... copy to run rat
{
	RC rc = RCOK;

	CSE_E(BtR.RunDup(BTiB));

	BATTERY* pBT;
	RLUP(BtR, pBT)
		rc |= pBT->bt_Init();

	return rc;
}	// ::topBT
//=============================================================================
LOCAL RC topSX()		// SHADEX copy to run rat
{
	RC rc = RCOK;

	CSE_E(SxR.RunDup(SXiB));

	SHADEX* pSX;
	RLUP( SxR, pSX)
		rc |= pSX->sx_Init();
	return rc;
}	// ::topSX
//=============================================================================
LOCAL RC topGain()	// check GAINs / copy to run rat
{

	RC rc = RCOK;

	CSE_E (GnB.RunDup( GniB, &ZrB));

	return rc;
}		// topGain
//===========================================================================
LOCAL RC topRadGainDist()	// set up radiant internal gain distribution tables for all zones. Added 11-95.

/* All surfaces are assumed to have SAME absorptivity, even windows.
   Room is assumed spherical. Thus all energy will be absorbed adn energy will be absorbed uniformly.
   Thus absorptivities can be assumed 1.0 and reflections can be ignored.
   (Bruce says actual is .9 for most building materials and .84 for glass). */
{
	// note addRIgDist automatically makes matching znTot calls to self, for ebal check / ZEB report IgnS column.

// loop xSurfs, directing their share of their zone's radiant internal gains: some goes thru to outdoors or other zone

	XSRAT * xs;  			// XSURF record pointer
	RLUP( XsB, xs)			// loop "XSURF"s, including windows and quick-model surfaces, for all zones
	{
		switch (xs->x.xs_ty)
		{
		default:
			continue;		// CTMXWALL, CTPERIM don't receive internal gain. Doors are CTEXTWALLs.

		case CTINTWALL:		// interior surface (wall,ceiling,floor,or door) or adiabatic surface.
		case CTEXTWALL:		// exterior surface: outside outdoors [or adiabatic]. Inside always in a zone.
		case CTWINDOW:	 ;		// (exterior) window. ASSUMED as OPAQUE as walls for rIg purposes.
			// fall thru
		}

		// do both sides when other side is also in a zone

		for (SI side = 0;  side < 2;  side++)		// for "inside", and "outside" if in another zone
		{
			ZNR *zp, *adjZp;				// 'this side' and 'other side' zone pointers
			float uI;					// 'this side' surface film conductance
			if (!side)					// if doing inside
			{
				zp = ZrB.p + xs->ownTi;
				adjZp = ZrB.p + xs->x.sfAdjZi;
				uI = xs->x.uI;
			}
			else						// doing outside
			{
				if ( xs->x.xs_ty != CTINTWALL
				 ||  xs->x.sfExCnd != C_EXCNDCH_ADJZN )	// if not wall with "outside" also in a zone
					break;					// then "outside" receives no internal gain
				zp = ZrB.p + xs->x.sfAdjZi;		// 'this side' zone
				adjZp = ZrB.p + xs->ownTi;		// 'other side' zone
				uI = xs->x.uX;					// 'this side' surface film
			}

			// fraction of zone's radiant internal gain that falls on this surface
			float frac = xs->x.xs_area / zp->zn_surfA;

			// other/same side split per surface film and construction U's
			float split =					// fraction of energy falling on inside that goes thru surface:
				xs->x.sfExCnd==C_EXCNDCH_ADIABATIC ? 0.f 	// none if outside adiabatic
				             : xs->x.xs_uval/uI;	// else series( uC, uX) / parallel( uI, series( uC, uX)).
													// Comments in cgsolar.cpp show algebra to prove can compute thus.
													//  Note xs_uval is series( uI, uC, uX)
			float outFrac = frac * split;			// fraction of zone's rad ig that goes thru this surface
			float inFrac = frac * (1 - split);		// frac of zone's rad ig falls on this surf & stays in zone

			// portion that stays in zone
			zp->addRIgDist( znCAir, zp->ss, inFrac);	// portion that stays in zone for quick surfs goes to zn CAir
			// also makes znTot entry for each call.

			// portion that goes thru surface to other zone or outdoors
			if (xs->x.xs_ty==CTINTWALL)   			// interzone wall
			{
				zp->addRIgDist( znCAir,  adjZp->ss, outFrac); 	// gain thru wall goes to other zone
				// add interzone transfer to show in both zone ZEB Izone columns, and to balance the energy balance.
				zp->addRIgDist( znTotIz, adjZp->ss, -outFrac);	// negative in zone energy goes TO
				zp->addRIgDist( znTotIz, zp->ss,    outFrac); 	// note sign reversed at accum to results
			}
			else						// exterior wall or window
				zp->addRIgDist( znTotO, zp->ss, outFrac);		// gain thru wall goes outdoors (for Conduction in ebal)
		}
	}

// loop masses. Where exposed to zone, direct share of zone's rad int gain to appropriate side of mass.

	MSRAT *ms;
	RLUP( MsR, ms)
	{
		if (IsMSBCZone( ms->inside.bc_ty))				// if mass "inside" is in a zone
		{
			ZNR *zp = ZrB.p + ms->inside.bc_zi;			// point to the zone
			zp->addRIgDist( massI, ms->ss, ms->ms_area/zp->zn_surfA);	// inside rcvx gain in proportion to fraction of zone surf area
			// side is always in the zone its in --> no iz case needed here
		}
		if (IsMSBCZone( ms->outside.bc_ty))				// if mass "outside" is in a zone
		{
			ZNR *zp = ZrB.p + ms->outside.bc_zi;		// point to the zone
			zp->addRIgDist( massO, ms->ss, ms->ms_area/zp->zn_surfA);	// outside rcvs gain in proportion to fraction of zone surf area
			// side is always in the zone its in --> no iz case needed here
		}
	}

#ifdef DEBUG
	// possible consistency checks: each zn znTot should be 1.0; total zone fr 2.0. 11-95.
#endif

	return RCOK;
}			// topRadGainDist

//===========================================================================
void ZNR::addRIgDist( 	// add radiant internal gain distribution for gain originating in this zone, 11-95.

	RIGTARGTY targTy, TI targTi, float targFr )

// CAUTION: can move rIgDist table -- store no pointers into it.
// for topRadGainDist. 11-95.
{
	if (targFr==0.)
		return;		// nop if fraction is 0

// make matching zone total entry for all other entries.
// the total is used for energy balance checks, and shows in the ZEB report "IgnS" column.
// when gain is coming thru wall from another zone, these should cancel out.
	if (targTy != znTot)
		addRIgDist( znTot, this->ss, targFr);		// call self

// compute pointer to target.
// pointing now assumes no target will move before (or during) run.
// there's enough info in entry to repoint later if need found.
	RIGTARG * p = NULL;
	switch (targTy)
	{
	case znTot:
		p = &ZrB.p[targTi].qrIgTot;
		break;
	case znTotO:
		p = &ZrB.p[targTi].qrIgTotO;
		break;
	case znTotIz:
		p = &ZrB.p[targTi].qrIgTotIz;
		break;
	case znCAir:
		p = &ZrB.p[targTi].qrIgAir;
		break;
	case massI:
		p = &MsR.p[targTi].inside.rIg;
		break;
	case massO:
		p = &MsR.p[targTi].outside.rIg;
		break;
	}

// see if zone already has rIg distribution table entry for type/ti: if so, we will add to it
	RIGDIST * e = NULL;
	if (rIgDist)				// if zone has table allocated: insurance
		for (int i = 0; i < rIgDistN; i++)	// loop over used entries
		{
			RIGDIST * e1 = rIgDist + i;		// point entry
			if ( e1->targTy==targTy		// if entry matches
					&&  e1->targTi==targTi		// ..
					&&  e1->targP==p )			// testing targP is redundant
			{
				e = e1;
				break;
			}
		}

// if entry not found, add it / allocate table if necessary
	if (!e)					// if matching entry not found
	{
		// allocate if nec
		if (!rIgDist || rIgDistN >= rIgDistNAl)	// if table not allocated or allocated space full
		{
			USI nuNAl = rIgDistNAl + 10;		// add this many entries at a time
			RIGDIST * nu;
			dmal( DMPP( nu), nuNAl*sizeof(RIGDIST), ABT); 	// alloc memory (dmpak.cpp), abort program (no return) on failure
			memcpy( (void *)nu, (void *)rIgDist, 		// copy old contents to the new storage
					sizeof(RIGDIST)*rIgDistN );
			dmfree( DMPP( rIgDist));		// free the old storage (dmpak.cp)
			rIgDist = nu;				// set pointer to new
			rIgDistNAl = nuNAl;			// store new allocated size
		}
		// init table entry
		e = rIgDist + rIgDistN++;		// point first free entry / say it is used
		e->targTy = targTy;
		e->targTi = targTi;
		e->targP = p;
		e->targFr = 0;
	}

// add given targFr to found or created table entry
	e->targFr += targFr;
}				// ZNR::addRIgDist
//-----------------------------------------------------------------------------
XSRAT* ZNR::zn_FindXSRAT(			// find XSRAT within zone
	const char* xsName)		// surface name sought
// NOTE: does not find non-exposed surfaces (adiabatic,)
{
	XSRAT* xs;
	RLUP (XsB, xs)				// loop over all XSRATs
	{	if (xs->ownTi != ss)		// if wrong inside zone
			continue;				// skip
		if (strcmpi( xs->name, xsName) == 0)
			return xs;
	}
	return NULL;
}		// ZNR::zn_FindXSRAT
//-----------------------------------------------------------------------------
MSRAT* ZNR::zn_FindMSRAT(			// find MSRAT within zone
	const char* xsName)		// surface name sought
{	MSRAT* pMS = NULL;
	RLUP( MsR, pMS)
	{	if (pMS->inside.bc_zi == ss
		 && strcmpi( pMS->name, xsName) == 0)
			return pMS;
	}
	return NULL;
}	// ZNR::zn_FindMSRAT
//-----------------------------------------------------------------------------
int ZNR::zn_IsSFModelSupported(
	int sfModel) const
// returns TRUE iff surface model is supported by zone model
// WHY: C_SFMODELCH_DELAYED etc. not possible with convective/radiant model
{
	BOOL bRet = SFI::sf_IsFD( sfModel);
	if (!zn_IsConvRad())
		bRet = !bRet;
	return bRet;
}		// ZNR::zn_IsSFModelSupported
//-----------------------------------------------------------------------------
float ZNISUB::zn_HeightZ(		// Z height relative to ground
	float f) const		// dimensionless height (0 = floor, 1=eaves)
// returns Z at fractional position f (can be <= 0)
{
	return zn_floorZ + f * (zn_eaveZ - zn_floorZ);

}	// ZNISUB::zn_HeightZ
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

#if 0	// 1-95: incorporate in topSf1 where actual surface film h's known
x //======================= Load Modelling Subfunction ========================
x
x //===========================================================================
x LOCAL SI conClass( CON *con)
x
x /* classify construction re applicable surface models
x
x returns: 0 massless (no layers or no hc): can't do delayed model (0 divide)
x 	 1 light: use QUICK model for C_SFMODEL_AUTO
x 	 2 heavy: use DELAYED model for C_SFMODEL_AUTO */
x
x{
x    // code here mimics cr\crsimsrf.cpp\siMsty(), 2-91.
x
x // "massless" if no layers or (almost) no heat capacity (thermal mass)
x     if ( con->nLr==0		// if no layers
x       || con->hc < .01f )	// too little heatcap (after crsimsrf.cpp)
x        return 0;		// "massless": cannot do delayed model
x
x /* "Treat reasonably light framed constructins as massless so normal
x    2x4 / 2x6 walls don't get classified as massive" -- crsimsrf.cpp.
x    Note this usually isn't as stupid as it looks: TYPICALLY such constructions
x    have 2 surface layers separated by a cavity of rval >> surface film,
x    so typically actually only about half the thermal mass is thermally coupled
x    to each zone, which would come out light by next test anyway -- rob 2-91.
x    But a cleverer test would be better.  And may wanna determine portion of
x    thermal mass coupled to each zone and put it in its znCAir. */
x
x     if ( con->nFrmLr > 0  	// if framed
x      && con->hc <= 2.0f )	// if thermal t.cpp. < 2.0/1.3 = ~1.54
x        return 1;		// "light": use quick model if auto specified
x
x /* "Time constant = hc/h should be greater than an hour to be massive"
x    Here h value is the u of surface film and hc is that of entire con:
x    assumes resistances within wall << surface film, a marginal assumption
x    for walls with framed cavity and/or insulation (but typically not off by
x    much more than a factor of 2, rob guesses 2-91).
x    Use generic surface film h (1.3) from t24pak.h: actual not known here.*/
x
x     // if (con->hc / 1.3f < 1.0f)  simplify: gather constants:
x     // if (con->hc < 1.0f * 1.3f)  simplify: drop 1.0:
x     if (con->hc < 1.3f)		// if time constant < 1 hour
x        return 1;		// "light": use quick model if auto specified
x
x     // *** caller may want to reconsider using surface's actual film h's.
x
x     return 2;			// "heavy": use delayed if auto specified
x}		// conClass
#endif

//========================== Supporting Functions ===========================

//===========================================================================
#if 0	// superceded by basAnc::getChoiTx, 12-12-11
x const char* chTx( 	// return text of given value for a choice data type, for error messages
x
x	BP b, 		// Pointer to basAnc, eg &AhuiB.  Used to retreive field info.
x	SI fn, 		// field number. rccn.h define, eg AHU_TSU.  Field must have a "choicb" data type.
x	SI chan )	// value in field, or CHN(value) if a CHOICN type.
x				// Possible internal values are C_xxxxxCH_yy defines in dtypes.h;
x				// each internal value has a string that this fcn retreives. */
x
x// fcn return: string representing value; displayable even if error.
x
x/* if chan is out of range (but b and fn valid), displays no errMsg, so can be called b4 checking chan.
x   Recommend that caller not display returned text b4 checking for valid chan value. */
x{
x	// add more checks to this code if errors tend to happen...
x
x	USI fdTy = b->fir[fn].fdTy;				// get field type (sFdtab subscript) from rat's "Fields In Record" table
x	USI dt = sFdtab[fdTy].dtype;			// get data type from small Fields info Table
x
x	chan &= ~NCNAN;					// clear special bits in nchoice value to make checks work
x	// added 3-1-94 as insurance -- no choicn calls at present.
x	if (chan==0)  					// if unset
x		return "<0>";					// bypass getChoiTx's error messages
x	if (chan <= 0 || chan > GetDttab(dt).nchoices)	// if call good but field contents garbage (GetDttab: srd.h)
x		return "<out of range>";				// issue no console error message here (caller already generating errmsg).
x
x	return getChoiTx( dt, chan);		/* get text for value 'chan' for data type dt.
x   						   Errors issue messages but also return displayable value. */
x}				// chTx
#endif
//===========================================================================
RC ckOwnRefPt(	// check / access ref to owning record

	BP toBase,			// ptr to BASE of RAT in which mbr is a subscript
	record * fromRec,   	// ptr to RAT record containing mbr (for errmsg)
	record **pp/*=NULL*/,    	// NULL or receives ptr to record
	RC *prc/*=NULL*/ )		// NULL or is set non-RCOK iff error

// shorter call to ckRefPt for checking/pointing to owning record
{
	return
		ckRefPt( 			// just below
			toBase, fromRec,
			((record *)fromRec)->ownTi,	// owning record ti always here
			"", 				// no user mbr name (usually nested group input)
			NULL, 		// owning rec not known yet (this call is getting it) & not desired redundantly in msg anyway
			pp, prc );
}			// ckOwnRefPt
//========================================================================
RC record::ckRefPt(	// check / access ref from one RAT to another
	BP toBase,				// ptr to BASE of RAT in which mbr is a subscript
	TI mbr,					// subscript of toBase record to access
	const char* mbrName/*=""*/, // "" or member name text (for errmsg)
	record* ownRec/*=NULL*/,   	// NULL or owning record (if non-owner ref) for errmsg.
								//  Should normally be NULL else msg redundant.
	record** pp/*=NULL*/,     	// NULL or receives ptr to record
	RC* prc /*=NULL*/ )			// NULL or is set non-RCOK iff error
{
	return ::ckRefPt( toBase, this, mbr, mbrName, ownRec, pp, prc);
}
//-------------------------------------------------------------------------
RC ckRefPt(	// check / access ref from one RAT to another
	BP toBase,				// ptr to BASE of RAT in which mbr is a subscript
	record* fromRec,   		// ptr to RAT record containing mbr (for errmsg)
	TI mbr,					// subscript of toBase record to access
	const char* mbrName/*=""*/, // "" or member name text (for errmsg)
	record* ownRec/*=NULL*/,   	// NULL or owning record (if non-owner ref) for errmsg.
								//  Should normally be NULL else msg redundant.
	record** pp/*=NULL*/,     	// NULL or receives ptr to record
	RC* prc /*=NULL*/ )			// NULL or is set non-RCOK iff error

// checks here are mainly as debug aids and to contain damage from other bugs,
//  or in case execution not terminated at earlier detection of bad reference

// returns RCBAD if reference is bad
{
	RC rc = RCOK;
	record* p = NULL;

// check argument
	if (toBase->validate( "cncult2.cpp:ckRefPt", WRN))
		rc = RCBAD;
	else
	{
		if (mbr <= 0 					// min subscript is 1
				|| mbr > toBase->n 			// bad if > max subscript in given RAT
				|| ( p = &toBase->rec( mbr),	// ok so far; point to record ...
					 p->gud <= 0) )			// bad if rec unused/bad (poss future)
		{
			rc = badRefMsg( toBase, fromRec, mbr, mbrName, ownRec);	// issue message, return bad
			p = NULL;
		}
	}
	if (pp)			// if pointer given
		*pp = p;	// return ref'd record ptr to caller
	if (prc)
		*prc = rc;	// copy of return value
	return rc;		// good return; another return above
}		// ckRefPt
//========================================================================
LOCAL RC badRefMsg( 	// message for bad reference (rat subscript)

	BP toBase,			// ptr to BASE of RAT for which mbr is bad subscript
	record* fromRec,	// ptr to RAT record containing mbr
	TI mbr,				// the bad subscript
	const char* mbrName, // "" or member name text
	record *ownRec )	// NULL or owning record: clarifies msg for non-owner reference, esp if fromRec unnamed.
						// now normally make NULL to avoid ugly redundancy in message -- oer used here.

// if get to this call, typically an internal error or a place where execution continued after error originally detected

// returns RCBAD for convenience
{
	BP fromBase = fromRec->b;

	// >>> using oer, can we drop ownRec arg? review calls. date?   Does make observed redundancy in messages, 1-92.

	return fromRec->oer(
				(char *)MH_S0494,   			// "bad %s index %d in%s %s '%s' [%d]%s"
				(char *)((BP)toBase)->what,
				mbr,
				mbrName && *mbrName ? strtprintf(" %s of", mbrName) : "",
				(char *)fromBase->what,
				fromRec->name,
				fromRec->ss,
				ownRec
					?  strtprintf( " of %s '%s'",  (char *)ownRec->b->what, ownRec->name )
					:  "" );
}	// badRefMsg


/*********************************************** IF-OUTS ****************************************************/

//topZn deletions:
#ifdef OLDNV // 1-92 cndefns.h
o   // init zone's natvent
o       // want erMsg (at end zone) for only 1 of ahigh, alow > 0
o       zp->natvflg = (zp->i.nvAHi > 0.F && zp->i.nvALo > 0.F);	// whether nat vent possible in zone
o       if (zp->natvflg)  					// if yes
o          cgnvinit(zp);  					// init nat vent, cgcomp.cpp: derives addl mbrs
#endif
#ifdef OLDFV
o   // init zone's fanvent
o#if 1 /* indirect evap cooling, 4-23-91 */
o       zp->fanvflg = zp->i.fvCfm > 0.F;	// whether fan vent poss in zone
o       if (zp->fanvflg)   		// if fan vent poss, note capabilities: 1=vent, 2=dir evap, 3=ind evap, 4=ind/dir evap.
o       {  if (zp->i.fvDECeff > 0.f)
o             zp->fanvflg += 1;		//   fan vent can do direct evap
o	  if (zp->i.fvIECeff > 0.f)
o	     zp->fanvflg += 2;		//   fan vent can do indirect evap
o
}
o#else
ox       zp->fanvflg = (zp->i.fvCfm > 0.F);	// whether fan vent poss in zone
o#endif /* 4-23-91 */
o       if (zp->fanvflg)
o          cgfvinit(zp); 	// init fan vent, cgcomp.cpp: derives addl mbrs.
#endif
#ifdef OLDCF
o   // init zone's ceilfan
o       zp->qZone = zp->i.cppfKW * zp->i.cfFz; 	// heat to zn
o       zp->ceilFanFlg = (zp->qZone > 0.F);   	// whether ceil fan exists
#endif

#if 0 //ifdef OLDAT	// undefined in cnglob.h, 5.92
//===========================================================================
o LOCAL RC topTux()		// do terminal units
o
o{
o TUX *tui, *tu; ZNR *zp; RC rc=RCOK;
o
o // clear terminal unit run RAT.   nothing to dmfree for TUs, right?
o
o     E( ratCre( (RBP)&TuxB, TuxiB.n, WRN) )	// delete old entrys, alloc to needed size now for min fragmentation
o     // CAUTION: run .ownB's also set in cnguts:cgPreInit; if changed here, change there for showProbeNames.
o     TuxB.ownB = &ZrB;			/* terminals are owned by zones and zone run subscripts match zone input subscripts.
o 					   Setting run rat .ownB enables use of zone subscript from input context
o 					   for resolution of ambiguous references by name at runtime (probes) 1-92. */
o
o // copy tu input rat TuxiB entries to runtime rat TuxB and initialize
o
o     RLUP( TuxiB, tui)		// loop input tu RAT records
o     {  // skips not-guds
o     UCH *fs = (UCH *)tui + TuxiB.sOff;		// field status byte array
o
o        zp = NULL;					// owning zone not known yet
o        ckOwnRefPt( &ZrB, tui, (record **)&zp );	// insurance check of ref to tu's zone, get zone ptr
o
o    // copy data from tu input rat, same subscript
o        ratAddN( (RBP)&TuxB, tui->ss, VV&tu, ABT);	// add run rat entry
o        ratCpy( tu, tui, 1);				// copy entire entry incl any name. cul.c.
o
o    // add tu to zones tu chain
o        if (zp)
o        {	tu->tuxNxTu = zp->tux1;	// tu pts to prior chain (or 0 if no prior)
o           zp->tux1 = tu->ss;	// zone chain hd pts to cur tu
o		 }
o
o    // add local heating ZXH if needed
o        if (fs[TUX_TUXMXLH] & FsSET)		// if there is local heating capacity
o           E( addZhx( 				/* add Zone HVAC xfer for it (below) */
o 		    tu->ss,			/*   owner's ss */
o 		    &(tu->tuxZhxx1),		/*   owner's ZHXX chain */
o 		    fs[ TUX_TUXTLH] & FsSET 	/*   if set point unset */
o 		       ? ZHX_TULHC		/*      tstat controlled */
o 		       : ZHX_TULHS,		/*      else set output */
o 		    tu->tuxTLH, 0) )		// set point/pri (if tstat)
o
o    // add air heating/cooling ZHX if needed
o        if ( !tu->tuxAhi)		// if this TU has an air handler
o        {  // verify no input for other mbrs?
o		 }
o        else	// have air handler
o        {
o        SI gotH, gotC;
o           gotH = fs[ TUX_TUXTH] & FsSET;
o           gotC = fs[ TUX_TUXTC] & FsSET;
o           if ( !gotH && !gotC)
o 			{	if ( fs[ TUX_TUXCMXH] & FsSET && fs[ TUX_TUXCMXC] & FsSET)
o                 oer( tu, "Can't have both set output heating and cooling "
o                          "on same terminal.");
o 				else
o                 E( addZhx(	 		/* add Zone HVAC xfer (below) */
o 	                  tu->ss,		/*   owner's ss */
o 					  &(tu->tuxZhxx1),		/*   owner's ZHX chain */
o 					fs[ TUX_TUXTH] & FsSET	/*   if set point unset */
o 		             ? ZHX_TUAHS	/*      this is set output */
o 			     : ZHX_TUACS,	/*      else tstat controlled */
o 		          0.f, 0) )		//   set point/pri (unused)
o			}
o 			else
o 			{	if ( gotH)
o                  E( addZhx(	 		/* add Zone HVAC xfer for it (below) */
o 		           tu->ss,		/*   owner's ss */
o 		           &(tu->tuxZhxx1), 	/*   owner's ZHX chain */
o 			   ZHX_TUAHC,		/*   ty: tstat controlled heating */
o 		           tu->tuxTH, 0) )	//   set point/pri
o 	      if ( gotC)
o                  E( addZhx( 			/* add Zone HVAC xfer for it (below) */
o 		           tu->ss,		/*   owner's ss */
o 	        	   &(tu->tuxZhxx1), 	/*   owner's ZHX chain */
o 			   ZHX_TUACC,		/*   ty: tstat controlled cooling */
o 		           tu->tuxTC, 0) )	//   set point/pri
o		}
o	  }
o	}	// tu loop
o   return RCOK;				// error returns above including each E macro
o}		// topTu
o //==============================================================================
o LOCAL RC addZhx(		// add ZHXX (Zone HVAC transfer) for TU or ZNR natvent
o
o    TI owni,	// owner's (TU or ZNR) for back-ref in ZHX
o    SI *zhx1,	// ptr to owner's zhx chain head, new ZHX is added at head of chain
o    SI ty,	// ZHX type (cndefns.h ZHX_xxx)
o    FLOAT sp,	// tstat set point or 0.f if ty needs no sp
o    SI spPri )	// tstat set point priority or 0 (determines activation order of ZHXs with same .sp)
o
o{
o	ZHXX *zhx;	RC rc=RCOK;	// rc is used in E macro
o
o // make a ZHX
o     E( ratAdd(			/* add new rat entry (and RatCre if nec), ratpak.c */
o          (RBP)&ZhxxB,			/*   ZHX rat base (cnguts.cpp) */
o          (record **)&zhx, WRN ) )	//   returns zhx; msg and error return if no memory
o 	       			// leave RATFRONT mbrs .ty, .li, .file, .line unset (0).  may cause runtime errmsg troubles?
o 				// CAUTION: may not errCount++, be sure error return propogated back to cul:cuf.
o
o // owner
o     zhx->ownTi = owni;		/* can be in ZnR or TuB ---> not suitable for ambiguous probe name resolution
o     				   (unless ratLudo changed to check uniqueness even when .ownTi matches) 1-6-91 */
o
o // maintain owner's ZHX chain
o     zhx->nxZhxx = *zhx1;		// new ZHX pts to prior chain or 0
o     *zhx1 = zhx->ss;		// owner's head pts to new ZHX
o
o // add caller's data
o     zhx->tyZhxx = ty;
o     zhx->zhxxSp = sp;
o     zhx->zhxxSpPri = spPri;
o     return rc;				// each E macro also can return
o }			// addZhx
o //===========================================================================
o LOCAL RC topAhux()		// do air handler units
o
o {
o AHUX *ahui, *ahu; RC rc=RCOK;
o
o // clear run RAT.   nothing to dmfree for AHUs, right?
o
o     E( ratCre( (RBP)&AhuxB, AhuxiB.n, WRN) )	// delete old entrys, alloc to needed size now for min fragmentation
o 						// air handlers are not owned (no ti after name, as of 1-92), leave .ownB 0.
o
o // copy ahu input rat Ahui entries to runtime rat AhuB and initialize
o
o     RLUP( AhuxiB, ahui)		// loop input ahu RAT records
o     {  // skips not-guds
o     // UCH *fs = (UCH *)ahui + AhuiB.sOff; field status byte array (future?)
o
o    // copy data from input rat, same subscript
o        ratAddN( (RBP)&AhuxB, ahui->ss, (record **)&ahu, ABT);	// add run rat entry.  error not expected due to preceding ratCre
o        ratCpy( ahu, ahui, 1);					// copy entire entry incl any name. cul.cpp.
o
o	  }	// ahu loop
o     return rc;
o }		// topAhux
#endif

#if 0
x //===========================================================================
x LOCAL RC topFuel()	// init runtime fuel use rat
x
x // many things (future 1-92) may use these records and records might be added as use of fuel is encountered --> do early.
x
x {
x FUSECH fui;  	// fuel use choice value == fuel record subscript
x FU *fu; 	// record pointer
x RC rc;
x #define NFUELS  C_FUELCH_STEAM		// app\cndtypes.def.  if it keeps changing, dig it out of Dttab[].
x
x // clear rat and allocate required number of fuel records now, to minimize fragmentation
x     E( ratCre( (RBP)&FuB, NFUELS+1, WRN) )	// E macro: if error, returns bad to caller.  +1: record 0 not used.
x     // fuel uses are not owned, leave .ownB zero.
x
x /* discussion: should all fuel use records be added, or should those actually used be added as uses encountered
x    (in GAINS and other (future) objects)?
x    *  Adding only those used would permit distinguishing at report time
x       fuels building does not use from fuels not used under particular run conditions.
x    *  Having all records present eliminates a source of errors in runtime probes
x    *  Having all records present requires omitting results output for those with 0 use
x    *  Adding records as fuel used would require more setup code and would
x       require adding code each new place fuel was used,
x       so lets add all records now for now, 1-92. */
x
x // add fuel use records for all fuels now, subscripted by fuel use choice data type
x
x     for (fui = 1;  fui <= NFUELS;  fui++)	// for all fuels (1-based, 0 not used)
x     {
x        ratAddN( (RBP)&FuB, fui, &fu, ABT);	// add FU (fuel use) runtime record.  Error not expected as ratCre'd above.
x        strcpy( fu->name, 				// for probing, set fuel use record name to
x                getChoiTx( DTFUELCH, fui, NULL) );	// text for the fuel choice value used as its subscript (cvpak.cpp)
x	  }
x
x     return RCOK;
x #undef NFUELS
x}			// topFuel
#endif

// end of cncult2.cpp