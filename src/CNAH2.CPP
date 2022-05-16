// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnah2.cpp -- hvac airHandler simulation routines for CSE -- part 2

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// USI LI dtypes.h cndefns.h

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// AH TU ZNR
#include <MSGHANS.H>	// MH_R1270

#include <PSYCHRO.H>	// psyHumRat3 PSYCHROMINT
#include <EXMAN.H>	// rer
#include <CUL.H>	// FsSET

#include <CNGUTS.H>	// Decls for this file; ZrB MDS_FLOAT


//-------------------------------- OPTIONS ----------------------------------
//search each, delete unused, comment matches

#define FANREDO	// move resizeFansIf into puteRa & coordinate setFrFanOn. 7-21-95.

#define LOGRO	// for code in antRatTs to increase tuVfMx even when ah ts is on wrong side of zone sp,
				// so that coil can grow (cncoil.cpp:heat/coolCapThing limits in proportion to flow increase).
				// Originally fixed BUG0089 convergence (now converges without); seems a good idea. 6-17-97.

#undef ZNJUST	// in setTsSp1, limit ZN/2 fcc supply temp to just meet load.
				// 6-17-97: made BUG0090 converge more slowly, with ok result. Try on other files if need found.
				// requres TSPOSS defined.

#undef TSPOSS	// replaces tsfo (fan-only ts) with tsPoss (also can approximate coils) and changes calls.
				// Needed for options using tsPoss, such as ZNJUST just above. Check decls in cnrecs.def. 6-97.

// finer convergence tests while autoSizing experiments 6-97 ...
#undef TRY3a	// ... in converger
#undef TRY3b	// ... in binClip
#undef TRY3b1	// ... ... with preference for positive increments: NO, not without heat/cool distinction.
	// CNEP: TRY3a and TRY3b together: BUG0090 nonconverge not fixed. slight effect.
	//       TRY3b1 only or with TRY3a: not fixed.
	//       TRY3a and TRY3b together again, recoded for /10 in both: not fixed.
#undef TRY3c	// ... aTs in iter4Fs
	// TRY3c with divisor 100 or 1: worse for BUG0090. Pass 2 nonconverge.

#undef TRY2		/* try not extrapolating when resize, 6-97. Wasn't the (primary) problem,
	and need binClip during autosize to converge when fan too large and can't get frFanOn=1
	and don't want coil to grow to acheive tsSp when frFanOn <<1. */

#undef HUMX2		/* for humXtrap2(): trial extrapolating w even when clipping.
			to remove also search for humXtrap2 (eg in comments, cnrecs.def).
					3-rdW version was never seen to save iters; 2-rdW version sometimes saved but more often added.
Benefit not clear. Tested on only some files, full test not done yet 5-95. */

#undef CEECLIP   	/* define to restore flow change limiting (binary search) code, added 5-2-95 re a bug it didn't fix.
To remove, also search for ceeClip in comments; to restore, get iter4Fs if-outs from eof or OUT459B.ZIP.
Interacting clippers probably won't be useful: c depends on t, already clipped by binClip.
TESTED 5-10-95, with ACT=8: No advantage. Of 243 test runs, NONE saved iterations,
8 got added ah-tu convergence errors during warmup, 2 added iterations or other changes. */

#undef WSCLIP		/* define to restore binClip humidity change limiting.  Added 10-92 for problem it didn't fix; kept then.
5-95: Believe w binary search not good cuz ws usually doesn't oscillate once flow settled,
but may still need to change a lot, while binClip holds t/c, preventing humXtrap; seen grealty
slowing convergence when limit got too small while converging flow (during warmup, T166d, 5-11-95).
	Removed 5-11-95 (after adding humXtrap). TESTED removal added 3 iterations net total to 243 runs. */

/*-------------------------------- DEFINES --------------------------------*/


// initial extremes for limits of probably-linear coil and zone ranges generated in antRatTs and doCoils.
//   for variables tsmLLim, tsmULim, texLLim, texULim.  Made symbolic 5-17-95.
	const int LLLIM = 0;		// matches default ahTsMn.
const int UULIM = 250;		// 200-->250 to match default ahTsMx, after PSYCHROMAXT increased from 200 to 300, 5-17-95.
#if LLLIM < PSYCHROMINT || UULIM > PSYCHROMAXT
#error LLLIM..UULIM range exceeds range of pyschro functions
#endif

/******************************************************************************************************************************
*  HVAC: AHs (AIRHANDLERS), continued from cnah1.cpp
******************************************************************************************************************************/
// class declaration including member fcn decls is in rccn.h, from app\cnrecs.def via rcdef.exe.
//-----------------------------------------------------------------------------------------------------------------------------
// AH::iter4Fs (next function) is called by ahCompute, which is in cnah1.cpp.
//-----------------------------------------------------------------------------------------------------------------------------
//--- constants, also hard-coded in cncult2.cpp for Top.absTol; also in cnah1.cpp:
const float RELoverABS = .01f;			// relative to absolute tolerance ratio: +-1 corrsponds to +- 1%.
//const float ABSoverREL = 1.f/RELoverABS;	no uses 5-95	// reciprocal thereof
//-----------------------------------------------------------------------------------------------------------------------------
//---- ahCompute callees (iter4Fs - antRatTs - etc) local variables
LOCAL DBL NEAR tsmLLim, NEAR tsmULim;	// antRatTs narrows these to next temp up and down from ts where zone mode changes
LOCAL DBL NEAR texLLim, NEAR texULim;	/* doCoils narrows these bounds per desired temp and range over which
					   current coil behavior is valid basis for extrapolation
					   (eg to next major power change or curve knees) */
LOCAL DBL NEAR dTrdTs;     		// dts/dtr: approx slope of tr2 as ts varies, iter4Fs to getTsXtrapBounds

LOCAL SI NEAR nItTot;			// total # iterations since iter4Fs entered

LOCAL BOO NEAR ffc;   			// non-0 if fanF changed last iteration (tested where tsHis set)
LOCAL DBL NEAR fanFThresh;    		// upper limit of fanF that will have any effect on current flows, set by antRatTs
LOCAL DBL NEAR fanFChmult;   		// fanF change multiplier: reduced from 1.0 if changes alternate sign
LOCAL SI NEAR nFanFChrev;   		// number of fanF change sign reversals (direction reversals)
LOCAL SI NEAR lastFanFCh;		// 1 for fanF increase, -1 for decrease

#ifdef CEECLIP	// if including flow-change-limiting feature, 5-95
* LOCAL SI NEAR ncRev;			// number of c change direction reversals, init in iter4Fs, used in ceeClip
* LOCAL DBL NEAR cMin, cMax;		// smallest & largest c's seen, for init'ing maxCIncr
* LOCAL DBL NEAR cOld, cIncr1;		// prior c and c delta, set/used in ceeClip
* LOCAL DBL NEAR maxCIncr;		// c increment limit, set and used in binClip
* LOCAL BOO NEAR weClippedCr;		// TRUE if clipped cr1Nx this iteration
#endif

LOCAL SI NEAR ntRev;			// number of ts change direction reversals, init in iter4Fs, used in binClip
LOCAL DBL NEAR maxTsIncr;		// ts increment limit, set and used in binClip
LOCAL BOO NEAR weFudgedTs;		// TRUE if significantly extrapolated ts or ws this iteration
#ifdef WSCLIP // if limiting ws in binClip()
LOCAL SI NEAR nwRev;			// number of ts change direction reversals, init in iter4Fs, used in binClip
LOCAL DBL NEAR maxWsIncr;		// ws increment limit, set and used in binClip
#endif

#ifndef DEBUG2				// cnglob.h define, undef for release version
 const int NHIS=4;			// code only uses [3] or (I think) [3]; more takes time (is memmove'd)
#else
 const int NHIS=24;
#endif

// t(c) points history, for interpolation/extrapolation and to watch while debugging
LOCAL
struct TSHIS
{
	DBL t;			// computed temp value (aTs)
	DBL c;			// flow (cmix) value for which .t computed
	DBL dT;			// .t - prior .t, or 0 if either t absent, not coil-limited, extrapolated, or fanF changed
	DBL rdT;		// .dT/prior .dT, or 0 if either dT 0 including for any of above reasons
	DBL w;			// computed humidity ratio value (aWs)
#ifdef HUMX2
*   DBL dW;			// .w - prior .w, even if .x or not .gud, or 0 if very small
*   DBL rdW;		// .dW/prior .dW, or 0 if small or either dW 0
#else
	DBL dW;			// .w - prior .w, even if not .gud, or 0 if very small
	DBL rdW;		// .dW/prior .dW, or 0 if small, either dW 0, current or prior t or w extrapolated, etc
#endif
	char x;			// 'c' if converger()-in/extrapolated, or 'k' if binClip'd, or
					// 'w' if humXtrap'd (t unchanged), or 'W' if humXtrap2'd, or 'f' if ceeClip changed c.
					// To view in debugger; code tests only re w xtrap.
	char gud;		// 1 if coil-limited & comparable; 2 if no coil used (4-95) & comparable;
					// 0 if coil part load, extrapolated ts, just changed fanF, or in any way non-comparable.
} NEAR tsHis[NHIS];

// fanF history, for extrapolation to geometric series limit, and as debug aid
LOCAL
struct FHIS
{
	DBL f; 			// fanF value
	DBL dF; 		// .f - prior .f, or 0 if no prior
	DBL rdF; 		// .dF/prior .dF, or 0 if not 2 priors or EITHER .dF 0
	char x;			// 'g' if extrapolated -- not valued point for further xtrap
	char gud;		// 0 if invalidated or extrapolated: believed redundant 4-92.
} NEAR fHis[NHIS];

//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::iter4Fs( BOO &cvgFail)  	// ah iteration loop to determine fanF, aTs, cmix, and other ah variables;
// determines terminal flows and zone temps as a side effect.
// returns FALSE if ah has been shut off.
// else returns TRUE, with arg cvgFail non-0 on convergence failure.
{
#undef QUIK	// define for testing speed
#ifdef QUIK
x  #define MAXITER 50	// change to speed testing. REVERT!
#else
#define MAXITER 200	// message seen @ 10, 50 (T47), 100 (T5).  No msg @ 200, 6-25-92.  Increase more later anyway?  400.
	// 400->200 when changed to silent recall-flagging return, 11-1-92.  Try reducing to 100?
#endif

	AhresB.p[ss].S.nIter2++;		// in ah subhour results subrecord, count times ah computed

	// initialize static vbls shared with antRatTs etc
	nItTot = 0;
	tsmLLim=LLLIM, tsmULim=UULIM; 	// antRatTs narrows these limits to ts's that produce zone mode changes
	dTrdTs = 0.;			// dtr/dts slope not known yet

	ffc = FALSE;			// fanF not changed during last iteration
	fanFThresh = fanFMax;		// init fanF threshold to safe (but slow) value in case used (from iter4Fs) b4 antRatTs
	fanFChmult = 1.;			// fanF change multiplier: reduced from 1.0 if changes alternate sign
	nFanFChrev = 0;			// no of fanF change sign reversals (direction reversals) yet
	lastFanFCh = 0;			// 1 for fanF increase, -1 for decrease

	ntRev = 0;				// number of ts change direction reversals, used in binClip().
#ifdef WSCLIP
	nwRev = 0;				// number of ws change direction reversals, used in binClip().
#endif

	memset( tsHis, 0, sizeof(tsHis));	// no ts history
	memset( fHis, 0, sizeof(fHis));	// no fanF history

	DBL dt2 /*=.1*/;			// delta-t thru airHandler, not set till after cr1Nx determined; not allowed to be 0.
	DBL sink = 0.;

	// identify zone-controlled constant volume cases: convergence critical since terminal can't adjust flow to trim output.
	BOO isKV = FALSE;
	if (isZNorZN2)						// if ah sp cm is ZN or ZN2 this hour (set in AH::begHour)
	{
		TU *ctu = TuB.p + ahCtu;					// point control terminal to whose zone ah ts responds
		if ( ctu->cMn > .99 * ( (ctu->useAr & (uMxH|uStH))	// if min flow > 99% of applicable max flow
								? ctu->cMxH : ctu->cMxC ) )	// .. (test known sloppy re uMn (expected), uSo (not expected))
			isKV = TRUE;						// consider it constant volume for our purposes here
	}								// caution: isKV can be different each call, if zn mode changes.

#if 0 // use AH::bVfDs, 7-19-95
x    // remember fan design flow at entry, so increases not used at convergence can be taken back. 7-19-95.
x    float sfanVfDsWas = sfan.vfDs;
#endif

#ifndef FANREDO
x #ifdef AUTOSIZE
x // AUTOSIZE: increase capacity of fan(s) being autoSized to flow during pass 1
x     resizeFansIf( cr1Nx*Top.auszHiTol2, bVfDs);	// mbr fcn, cnfan.cpp. cr1Nx set by zRat from caller ahCompute (or antRatTs below).
x 						// autoSized fan should always be big enough, expect no fanF/fanlimited in pass 1.
x #endif
#endif

// iteration loop -- till converged & stable enuf

	for (SI iter = 0; ; iter++)
	{

		// determine supply temp setpoint for methods where not done by runtime expression

		// must be done each iteration cuz of airxTs change, at least for const vol ZN/ZN2, 4-95.
		if (!setTsSp1( FALSE, tsSp1))	// set tsSp1 to numeric supply temp setpoint -- does C_TSCMNC_RA, _ZN, _ZN2, _WZ, _CZ
			return FALSE;			// if ah shut off (under ZN tscm), done

		// on non-convergence, flag & return, hope zone changes allow later call to converge, else hvacSubhrIter issues msg.

		/* case 11-1-92: ZN2, low heat load, tsSp1 imprecise --> get float mode (0 or 2), coil not in use, aTs increases with flow
		   due to fan heat, aTs oscillating above & below tz, causing antRatTs to alternate max & min flow (sutter\SA11B13).
		   TESTED 11-2-92: works for all my tests & SA11B13, but could certain inputs still cause failure?? */

		if (iter > MAXITER)			/* smarter (faster) non-cvg test desirable if this is normal occurence:
       						   maxTs/WsIncr min'd & ts alternating for n iterations? */
		{
			flagTus();				// flag to insure recompute: set ztuCf for served zones, and Top.ztuKf.
			Top.ahKf2 = TRUE;			// say an AH is flagged (ahPtf2) for recall only after zones recomputed
			cvgFail = TRUE;			// tell caller to set this->ahPtf2 (otherwise he clears it) (ref arg)
#ifdef DEBUG2	// omit message from release version. TESTED 5-12-95: enabled msg did not occur in any CKALL tests.
			if ( Top.iter >= 2 			// conditional warning. 2 non-cnvs seen in SA11B13, consider normal.
					&&  Top.iter < 12 )			// crude way to limit to 10 messages in one subhour
				rWarn( (char *)MH_R1294, name, (INT)Top.iter,	// "airHandler '%s' not yet converged. Top.iter=%d\n"
					   tr, wr, cr, frFanOn,   			// "   tr=%g  wr=%g  cr=%g  frFanOn=%g\n"
					   po, aTs, aWs, fanF, (INT)ahMode,		// "   po=%g  ts=%g  ws=%g  fanF=%g  ahMode=%d\n"
					   trNx, wrNx, crNx );				// "   trNx=%g  wrNx=%g  crNx=%g\n"
			// "   DISREGARD THIS DEBUGGING MESSAGE UNLESS FOLLOWED BY\n"
			// "       \"Air handler - Terminals convergence failure\" error message."
#endif
			return TRUE;				// don't say ah shut off
		}

		AhresB.p[ss].S.nIter4++;			// in ah subhour results subrecord, count times ah computation iterates
		nItTot++;				// count all iterations for info & conditional breakpointing 4-92
		weFudgedTs = 0;				// no ts/ws extrapolation done yet this iteration

		// HOLD FLOW AT MAX while fan limited or whenever flow tries to exceed max

		/* 1. While fan limited, find proper fanF first: it is the independent variable.
		      fix flow at max, compute ts, compute flow, adjust fanF (clearing fanLimited if no longer applicable), repeat.
		      (if fanLimited, expect prior flow at entry to be max anyway.)

		       2. If algorithm 'hunting' yields flow over max, instead get a ts for a possible flow.
		          If this yields another flow over max, fanLimited will be set at bottom of loop.
		          If not, reducing flow here will have accelerated convergence. */

		// Top.tp_sizing TRUE when increasing sizes: autoSizing pass 1A/B, and 2B, or as changed.
		if (!(Top.tp_sizing && asFlow))	/* but while autoSizing flow, just let flow increase to meet demand so tu(s) and possibly
					   ah coils can be autoSized correctly -- primarily when autoSizing tu's & not sfan,
					   when resizeFansIf (from puteRa) doesn't increase sfan.cMx. 7-95. */
			if ( fanLimited  		// keep max flow while fanLimited (cleared below if cr1-->ts-->cr1  yields cr1Nx < max.)
					||  cr1Nx > sfan.cMx )    	// if flow too hi, use max only, get ts for real cr; fanLimited will be set below.
			{
				// Note no tolerances used at top of loop.
				cr1Nx = sfan.cMx;		// use exactly max ah flow.  fanLimited will set or cleared at foot of loop.
				frFanOnNx = 1.0;		// if fan is cycling, run it exactly 100% of time.
			}

		// save start-of-iteration conditions for endtest.  Also tr2 saves tr2Nx, cr1 saves cr1Nx.

		DBL tsWas = aTs;
		DBL wsWas = aWs;
		DBL airxTsWas = airxTs;   //DBL tsSp1Was = tsSp1; no longer used 5-95.

		// compute SUPPLY TEMP ts and flow cmix for current return flow (cr1Nx) and return temp (tr2Nx)

		pute4Fs();		// just below.  updates sfan.cMx and many variables.
		// commits to iteration: copies -Nx members to plain members.
		ahRun++;			/* say aTs has been computed: ok for TU::resizeIf (eg from antRatTs below)
				   to resize terminals based on it. Set FALSE at start subhour. */

		// keep history for extrapolators, and as debug aid

		memmove( tsHis+1, tsHis, sizeof(tsHis) - sizeof(tsHis[0]) );	// copy up, then put current in [0]
		memset( tsHis, 0, sizeof(tsHis[0]));				// init new entry all 0: x, dT, rdT
		tsHis[0].t = aTs;
		tsHis[0].c = cmix;				// t value and input c for which computed

		// only points on zero-power curve or full-power curve are used for extrapolation (they converge slowly)
		//   see eg OUT459B.ZIP for if'd out (!TRY4) code to use converger only at full, not 0, power, 5-95.
		if (!ffc)						// don't xtrap from ts if fanF just changed
			if (coilLimited)            tsHis[0].gud = 1;   	// full-power point
			else if (coilUsed==cuNONE)  tsHis[0].gud = 2;   	// no-power point, eg ZN2 floating between heat and cool
		// else leave .gud, dT, rdT 0: code assumes.

		/* compute dT and rdT for xtrapGeo if a point on full power or zero power curve and fanF did not change
		   (don't want interaction with fanF's xtrapolator even tho xtrapGeo with consistently changing fanF might be valid) */
		if (tsHis[0].gud)						// if this point good for extrapolaiton
		{
			if (tsHis[1].gud==tsHis[0].gud  &&  tsHis[1].t > 0.)   	// if have good prior point on same full or 0-power curve
			{
				tsHis[0].dT = tsHis[0].t - tsHis[1].t;    			// delta-T; nz implies comparable
				if (fabs(tsHis[1].dT) <= 1.e-10)  				// if near 0,
					tsHis[1].dT = 0.;  					// store 0: simplify later tests
				else if (tsHis[1].dT != 0)					// if have prior dT (thus 2nd prior non-xtrap ts)
				{
					tsHis[0].rdT = tsHis[0].dT/tsHis[1].dT;			// ratio of deltas, else leave 0 (code assumes)
					if (fabs(tsHis[0].rdT) <= 1.e-10)			// if rdT very small
						tsHis[0].rdT = 0.;					// make 0: simplify later tests
				}
			}
		}
		tsHis[0].w = aWs;

		// compute dW and rdW for humXtrap, 4-95.
		if (!ffc)						// not if fanF changed: flows might affect w
		{
			// .gud not checked here: user checks if desired.
			if (tsHis[1].w > 0.) 					// if have prior w. User checks tsHis[1].x if desired
			{
				tsHis[0].dW = tsHis[0].w - tsHis[1].w;		// delta-W. tsHis[0].x not set yet, must check at use.
#ifdef HUMX2
*         if (fabs(tsHis[0].dW) <= Top.absHumTol/10)  	// if already well converged (wild guess value)
*            tsHis[0].dW = 0.;				// store 0: suppress extrapolation
*         else if (tsHis[1].dW != 0.)  			// if have prior dW
#else
				if (fabs(tsHis[0].dW) <= Top.absHumTol/4)  	// if already well converged
					tsHis[0].dW = 0.;				// store 0: suppress extrapolation
				else if ( tsHis[1].dW != 0.  			// if have prior dW
						  &&  !tsHis[1].x  				// and no t or w extrap that iteration
						  &&  tsHis[0].gud==tsHis[1].gud )		// and 0/part/full power matches, else not comparable
#endif
				{
					tsHis[0].rdW = tsHis[0].dW/tsHis[1].dW;		// ratio of deltas. else leave 0.
					if (fabs(tsHis[0].rdW) < .1)			// if converging rapidly anyway
						tsHis[0].rdW = 0.;				// store 0: suppress extrapolation
				}
			}
		}

		// conditionally expedite/assist convergence

		if (!binClip())     				// conditionally limit ts change after multiple reversals
			if (converger())				// if didn't, then conditionally extrapolate ts to accelerate convergence
				binClip(); 				// and conditionally limit that ts change
#ifdef HUMX2
*    if (!humXtrap())    				// conditionally extrapolate aWs to accelerate convergence. below.
*       humXtrap2();					// if didn't, try the other extrapolation function
#else
		humXtrap();					// conditionally extrapolate aWs to accelerate convergence. below.
#endif

		// update volumetric specific heat of air @ ts and tu/ah limits based on it. Also done when fanF changes. Unconditional 4-95.

#if 1// 7-18-95  else ahVshNLims can cut flow back after new tuVfMxH/C reached.
		if (!fanLimited)					// if not fan-flow-limited  7-18-95
			setToMax( fanF, fanFMax);			/* be sure fanF large enuf to not limit flows. Initialization insurance;
					          	   and during autoSizing, fanFMax can increase in ztuMode and antRatTs. */
#endif
		ahVshNLims( aTs, sink, sink);			// below in this file

		// determine RETURN AIR TEMP tr2Nx AND FLOW cr1Nx expected for this supply temp, and more

		antRatTs();					// determines active tu aCv's, float tu aTz's, tr2Nx/cr1Nx/frFanOnNx.
		// may update sfan.vfDs if autoSizing, 7-21-95
		// sets globals tsmLLim, tsmULim, fanFThresh, etc.
		if (fabs(aTs - tsWas) > .001)
			dTrdTs = (tr2Nx - tr2)/(aTs - tsWas);    	// approx tr2/ts slope, for getTsXtrapBounds.
		dt2 = aTs - tr2Nx;    				// delta-T thru airHandler, tested [above,] below, and at end loop
		if (fabs(dt2) < Top.relTol)			// apply a floor for no /0 and to prevent spurious iterations
			dt2 = Top.relTol;				// sign does not matter: all uses are fabs'd
		/* Top.relTol (.001 default) is used here as small constant that changes with "tol" input.
		   TESTED 8-17-92: .001 yields results like old code (changed dt2==0, only, to .1);
		         .01  disturbed results at least when sfan on, coils off (T9F (T9 w ZN2 & min flow)).*/

		if (weFudgedTs)  continue;			// be sure in/extrapolated ts is checked by doCoils: skip fanF/endtest

		// fanF history (for fanXtrap)

		memmove( fHis+1, fHis, sizeof(fHis) - sizeof(fHis[0]) );	// copy up, then put current in [0]
		memset( fHis, 0, sizeof(fHis[0]));			// init entry all 0
		fHis[0].f = fanF;					// store current value
		if (fHis[1].f)						// leave .dF and .rdF 0 if no prior value (code assumes)
		{
			fHis[0].dF = fHis[0].f - fHis[1].f;			// delta fanF
			if (fabs(fHis[0].dF) <= 1.e-10)  			// if near 0,
				fHis[0].dF = 0;					// store 0 to simplify later tests
			else if (fHis[1].dF)					// if have prior delta (else leave .rdF 0: code assumes)
			{
				fHis[0].rdF = fHis[0].dF/fHis[1].dF;  		// ratio of deltas; 0 for 0 or infinite
				if (fabs(fHis[0].rdF) <= 1.e-10)  fHis[0].rdF = 0.;	// 0 for near 0 to simplify later tests
			}
		}
		fHis[0].gud = 1;						// and .x left 0.

#ifndef FANREDO	// move into puteRa from antRatTs for better coord with fanFrOn.
x #ifdef AUTOSIZE
x    // AUTOSIZE: increase capacity of fan(s) being autoSized to flow during pass 1
x        resizeFansIf( cr1Nx*Top.auszHiTol2, bVfDs);	// mbr fcn, cnfan.cpp. cr1Nx set by zRat from ahCompute or antRatTs above.
x 							// bVfDs: subr start sfan.vfDs from hvacSubhrIter, as backtracking limit.
x 						// autoSized fan should always be big enough, expect no fanF/fanlimited in pass 1.
x #endif
#endif

		// FAN UNDERLOAD and OVERLOAD

		ffc = FALSE;					// say no fanF change
		if ( fanLimited  				// if terminal max's are reduced -- normally FALSE
				&&  cr1Nx < sfan.cMx * Top.loTol)   		// if flow less than possible
		{

			// fan underload.  With fanLimited TRUE, cr1Nx has been determined for ts determined for max cr1.

			if (incrFanF())				// increase fan flow some & recalc cr1Nx/tr2Nx for same ts / if did it
			{
				//  clears fanLimited if appropriate.
				ffc = TRUE;				// say changed fanF: don't extrap ts next iteration
				// if cr1Nx still out of range, or tr2Nx or tsSp1 change requires ts recalc, regular endtest will not terminate.
			}
		}
		else if ( cr1Nx > sfan.cMx * Top.hiTol       	// if flow exceeds maximum fan can deliver
				  &&  !(Top.tp_sizing && asFlow) )			// and not autoSizing flow, esp tu's & not sfan, 7-95
		{

			// fan overload.  cr1Nx has been determined for ts determined for non-excessive cr1.

			/* each fanF change will be too little or too much depending on portion of flow which was pegged,
			     and impact on ts, if coilLimited, via tr2Nx, which depends on relative tz of zones cut.
			   So change frequently but conservatively each time. */

			if (decrFanF())			// decrease fan flow some & recalc cr1Nx/tr2Nx for same ts. Sets fanLimited.
				ffc = TRUE;			// if did it, say changed fanF: don't extrap ts next iteration
			// if cr1Nx still out of range, or tr2Nx or tsSp1 change requires ts recalc, regular endtest will not terminate.
		}

		// endtest, in the form of multiple continue-tests.  Also some conditions continue above.

		//DBL dt2 = aTs - tr2Nx;    	is above		delta-T thru airHandler, forced non-0.

		if (cmix != 0.)		/* if there is no flow (not even duct leak), don't take time to converge temps & hum, 6-16-92
       				   -- but do set them for probes, and ah results if not filtered out. */
		{
			// supply air temp not change-checked: change shows up in return air temp to the extent that it requires iteration.

			/* cr1Nx/tr2Nx calculated last.
			   Return air temp change requires iteration, particularly to finish floating whole system up or down at load change
			   (eg weather change) when no few/setpoint zones and flow is fixed or held constant by terminal or fan capacity.
			   tr2 change relative to dt2 is sensitive @ small dt2, when the most iterations req'd to move whole system temp. */
			if (fabs((tr2Nx - tr2)/dt2) >= Top.relTol)
				continue;

			/* flow changes fast (til limit) while temps change slowly when converging at low dt2.
			   Also, flow change requires iteration for correct ah/coil results output even if ts/ws for tu's converged, 6-92.
			   And, iterate if flow fudged by ceeClip wasn't kept, 5-94. */
			if (RELCHANGE( cr1Nx, cr1) >= Top.relTol1)		// relTol1 cuz relTol in fanF tests above (hang observed 8-14-92)
				continue;						// relTol1 = 1.x * relTol, set in cncult2.cpp

			// flow cannot be over max, and must be maximum value when fan limited flag is on
			if (!(Top.tp_sizing && asFlow))					// except if autoSizing, especially tu's & not sfan, 7-95
				if (cr1Nx > sfan.cMx || fanLimited)   			// flow < max ok if 'fanLimited' off
					if (fabs(cr1Nx - sfan.cMx)/sfan.cMx >= Top.relTol1)	// supply fan overload or uncorrected underload
						continue;						// relTol1: insurance re relTol in fanF tests above, 8-92.

			// supply humidity change requires iteration to converge interconnected supply/zone humidities -- w has no setpoint.
			/* TESTED: tried larger tol here if tsHis[0].w=='w', presuming xtrap'd aWs ALWAYS as good as limited tests showed:
			   saved iterations, but unbalanced ah cuz not pute'd for load ws. 4-95. */
			if (fabs(aWs - wsWas) >= Top.absHumTol)
				continue;

			// supply temp: usually reflected in return temp; 4-17-95 tried this as insurance; need not demonstrated; removed.
			//    if (fabs(aTs - tsWas) > Top.absTol)
			//       continue;
#ifdef TRY3c // experiment 6-16-97
*          if (Top.tp_autoSizing)
*             if (fabs(aTs - tsWas) > Top.absTol)
*                continue;
#endif

			// supply air specific heat change requires iteration, at least for const vol ZN/ZN2 cases with leaks/losses, 4-95.
			if (isKV)							// if const vol ZN or ZN2
			{
				if (fabs(airxTs - airxTsWas) >= .02 * Top.relTol)		// airTx values are around 0.9.
					/* ^^^ tested bug0056 4-95 ZN2 const vol w hi fan heat: .1 bad (some tz's .01 wrong),
					   .01 ok, .04 bad case found, .03 ok, use .02 for margin. REVERT TO .02 OR LESS. */
					continue;		/* This check found nec to get setpoint tz's in const-vol ZN2 with leaks/losses,
					   even though setTsSp1, pute4Fs purport to not use airxTs.
					   Maybe its cuz sfan.cMx is used in setTsSp1 then set in pute4Fs. 4-17-95. */
			}

			// coil models can signal convergence not complete (re autoSizing)
			if (iter < 20)				// insurance
				if (ahhc.notDone || ahcc.notDone)		// flags set in cncoil.cpp
					continue;
		}

		// // supply temp setpoint (WZ/CZ/ZN/ZN2) change requires iteration (changes at bottom of loop)
		//   removed: no longer changes except at top of loop. TESTED made no difference in 243 runs, 5-11-95.
		//     if (ahMode & (ahHEATBIT|ahCOOLBIT))			// if heat or cool enabled (sp moot if just ahFAN (ahTsSp = ZN2))
		//        if (fabs((tsSp1 - tsSp1Was)/dt2) >= Top.relTol)  	// measure relative to dT thru airHandler
		//           continue;

		break;							// done!  Another break above, after convergence failure msg.

	}    // for ; ;

	return TRUE;					// say completed, ah still on.  Also 4+ other returns above.
}				// iter4Fs
//-----------------------------------------------------------------------------------------------------------------------------
void AH::pute4Fs()			// compute SUPPLY TEMP aTs for current flow and return temp, for iter4Fs

// inputs (set by puteRa, from zRat or antRatTs):  frFanOnNx, tr2Nx, wr1Nx, cr1Nx

// outputs: tr2,wr1,cr1, po,tmix,cmix,coilLockout, wen,ten,tex1, tSen,aWs,aTs, dtMixEn,dtExSen,dtSens, sfan.cMx, texLLim,texULim
{
// commit to another iteration: copy next-iteration values to current-iteration-input valuses
	cMxfcc = cMxnx;
	leakCOn = leakCOnNx;	// currently members as debug aid only
	frFanOn = frFanOnNx;			// fraction fan on (1.0 or cr/max possible flow)
	tr = trNx;
	wr = wrNx;
	cr = crNx;      	// conditions at return from zones/plenum
	tr1 = tr1Nx;
	wr1 = wr1Nx;
	cr1 = cr1Nx; 		// conditions after return duct
	tr2 = tr2Nx;    	// after return fan if any (w,c same)
	rfanQ = rfan.q;		// for results, save ret fan power actually used (puteRa overwrites nxt iter value)

// preliminary update of supply fan: sfan.dT needed for economizer, dtMixEn, dtExSen.
	// uses latest return flow (cmix will probably be same) and prior temp at fan.
	sfan.fn_pute( cr1, sfan.t, frFanOn);			// sets sfan.dT (and other stuff, reset when new temp at fan known)

// loop to do outside air & repeat if fan heat changes much. loop added 4-22-95.

	DBL sfanDtWas;
	SI j=0;
	do
	{
		sfanDtWas = sfan.dT;

		// OUTSIDE AIR/ECONOMIZER: given tr2,wr1,cr1  sets tmix,cmix,wen,po,ecoEnabled (used??),coilLockout
		doOa();						// member function below. uses sfan.dT.

		// compute blowthru fan, supply fan heat, coil entrance temp
		if (sfan.fanTy==C_FANTYCH_BLOWTHRU)
		{
			sfan.fn_pute( cmix, tmix, frFanOn);   		// set sfan.cMx, .p, .q, .dT, etc. cnfan.cpp.
			dtMixEn = sfan.dT;				// fan heat delta-T if before coils (else leave dtMixEn 0)
		}
		else						// C_FANTYCH_DRAWTHRU
		{
			dtExSen = sfan.dT;				// (preliminary) fan heat if fan is after coils (else leave dtExSen 0)
			DBL dtExSenWas;
			SI k = 0;
			// loop to determine now what drawthru fan heat will be at ts setpoint
			do			/* Added 4-22-95 to get exact texWant b4 doing coils, to fix ah non-convergence when
				   a slightly off-setpoint ts crossed to wrong side of setpoint of (lightly-loaded) tu and
				   maximized tu's flow, changing flow and sfan.cMx and coil-off ts (with duct leaks),
				   changing ZN2 tsSp1, causing thrash which didn't converge with wild fan curve, eg T136E.
				   TESTED: really does eliminate ah convergence failures; also! ah-tu convergence failures. */
			{
				dtExSenWas = dtExSen;
				sfan.fn_pute( cmix, tsSp1-dtExSen, frFanOn);	// compute fan heat @ entry temp that would produce setpoint at ts sensor
				dtExSen = sfan.dT;				// updated drawthru fan heat
			}
			while ( fabs(dtExSen - dtExSenWas) > ABOUT0	// if it changed, redo cuz fan entry temp changes (ABOUT0: .001, cnglob.h)
					&&  ++k < 3 );				/* max 3? iterations in case wild fan curve doesn't converge
   	       						   (expect little sfan.dT - sfan.t dependence) */
			/* if dtMixEn/dtExSen changed, also change tmixWant in doEco so eco
			   does not produce air requiring unnec cooling or spurious reheat.*/
		}
	}
	while ( fabs(sfan.dT - sfanDtWas) > ABOUT0		// repeat doOa if sfan.dT changed: get tmix right b4 doing coils (slow).
			&& ++j < 3 );				// repeat at most twice. little sfan.dT - temperature dependence expected.

	ten = tmix + dtMixEn;				// temperature at entry to coils

// relief fan
	if (rfan.fanTy==C_FANTYCH_RELIEF)			// now can compute relief fan (for power consumption)
		rfan.fn_pute( cmix * po, tr1Nx, frFanOn);		// relief fan flow equals outside air taken in, at least for now

// HEAT/COOL COILS: determine tex1 and aWs
	DBL texWant = tsSp1					// desired exit temp (setpoint for coils)
				  - dtExSen;				// if fan after coils, reduce for (prelim) fan heat: simulate sensor after fan
	texLLim = LLLIM;
	texULim = UULIM;			/* doCoils narrows these bounds per desired temp and range over which
    							   current coil behavior is valid basis for extrapolation
    							   (eg next major power changes or curve knees) */
#ifdef TRY2	// not extrapolating from resize, 6-97
*  if (doCoils( texWant, texLLim, texULim)) 		/* compute coil model, set tex1, aWs, perhaps limits. cncoil.cpp.
*							   can use/set many ah members; for AHP ahhc uses tsFullFlow and dtExSen.*/
*     tsHis[0].gud = 0;				// if resized coil (autosizing), don't extrapolate from before here. 6-97.
#else
	doCoils( texWant, texLLim, texULim);  		// compute coil model, set tex1, aWs, perhaps limits. cncoil.cpp.
	// can use/set many ah members; for AHP ahhc uses tsFullFlow and dtExSen.
#endif
// supply fan and its heat if drawthru
	if (sfan.fanTy==C_FANTYCH_DRAWTHRU)
	{
		sfan.fn_pute( cmix, tex1, frFanOn);     		// set sfan.cMx, .p, .q, .dT, etc. cnfan.cpp.
		dtExSen = sfan.dT;				// update fan heat (dtExSen left 0 for blowthru sfan)
	}
	tSen = tex1 + dtExSen;				// temperature at supply temp sensor: ah output temp except duct

// supply temp and flow at delivery to individual terminal ducts -- after possible shared supply duct.  w remains aWs.
	// air leaking out of duct does not change temp or w; is flow-linear: can disregard frFanOn.
	// supply flow after duct leakage not explicitly computed here (is equal to sum of terminal flows).

	aTs = tSen * (1. - ahSOLoss) + Top.tDbOSh * ahSOLoss;   	// heat loss (or gain) by conduction from duct to outside
	dtSenS = aTs - tSen;					// in case needed elsewhere in estimating
	// MAY ALSO be computed in setTsSp1.
	// aTs is copied into ts at ahCompute exit.
}							// AH::pute4Fs
//-----------------------------------------------------------------------------------------------------------------------------
#ifdef CEECLIP	// if including feature, 5-95
*/* #undef CEECLIP is at start of file.
*		TESTED 5-10-95, with ACT=8: No advantage. Of 243 test runs, NONE saved iterations,
*		  8 got added ah-tu convergence errors during warmup, 2 added iterations or other changes.
*		In initial tests 5-2-95 with ACT=6, seemed to be getting in the way & didn't fix target problem,
*		  which was a discontinuity in c-t relationship. */
*BOO AH::ceeClip( 	// conditionally limit flow (crNx) change
*
*    BOO canClip )	// TRUE to enable, FALSE to prevent clipping (must be called anyway to save stuff)
*
*	/* To assist convergence when t convergence aids fail by getting stuck on a discontinuity in t - c relationship
*	     eg that makes flow alternate min and max eg when ts near zone sp.
*	   Acts slowly and with delay, to give t convergence aids time to work first. 5-1-95. */
*
*	// not called if fan limited or flow over sfan.cMx.
*	// caller may call puteRa() afterwards to update variables derived from crNx.
*
*// returns TRUE and sets weClippedCr if crNx altered.
*{
*    #define ACT 8	// number of c change direction reversals before starting to clip. 6 initially tried; later 8.
*
*							// previous value or -1 is in cOld and previous increment or 0 in cIncr1.
*    DBL cIncr0 = (cOld >= 0 ? crNx - cOld : 0); 	// current increment, or 0 if no prior value (-1)
*
*// accumulate info only thru first n reversals of flow delta
*    if (ncRev <= ACT)
*    {	setToMin( cMin, crNx);
*		setToMax( cMax, crNx);		// accumulate smallest and largest cr seen
*       if (ncRev < ACT)					// up to action level
*          if (cIncr0*cIncr1 < 0)   			// if two flow deltas are non-0 and of opposite sign
*             ncRev++;					// count a reversal
*
*// then watch for a big c change (~50%) from a small t change (~.1F)
*       if (ncRev==ACT)
*          if (crNx + cOld)							// /0 protection
*             if ( fabs(cIncr0)/(crNx + cOld)					// relatve c change
*                  / ((fabs(tsHis[0].dT)+.1)/(tsHis[0].t + 459))  >  150 ) 	// relative t change. .1: /0 protection
*             {
*                maxCIncr = cMax - cMin + 1;    		// init maximum increment to range seen. +1 is insurance.
*                ncRev++;				// enable limiting (next 'if')
*			  }
*	}
*
*// then limit large c changes. May not clip for a while cuz limit shrinks slowly.
*    BOO rv = FALSE;
*    if (ncRev > ACT)					// no 'else' - can change in prior 'if'
*    {  if (cIncr0*cIncr1 < 0)    			// if a another reversal
*          maxCIncr *= .7;				// reduce maximum change
*       if ( canClip 					// if caller said ok to limit c change
*        &&  fabs(cIncr0) > maxCIncr )			// and current change is too big
*       {
*          crNx = cOld + (cIncr0 > 0 ? maxCIncr : -maxCIncr);	// limit c change from prior value
*
*          ntRev = 0;					// make binClip start over -- let real t be used for this c
*          rv = TRUE;					// say we acted
*		}
*	}
*
*// keep our history in file-global variables for use on next call. Iter4Fs inits them.
*    cOld = crNx;				// save c value
*    if (cIncr0)					// don't store 0's cuz we would not see reversals separated by them
*       cIncr1 = cIncr0;				// save delta
*    weClippedCr = rv;
*	 return rv;		// TRUE if we did anything. caller flags tsHis with .x='f' when entry made.
*
*#undef ACT
*}			// AH::ceeClip
#endif // ifdef CEECLIP
//-----------------------------------------------------------------------------------------------------------------------------
// variables shared by converger, getTsXtrapBounds, etc
LOCAL DBL NEAR tsxLLim, NEAR tsxULim;	// bounds for ts extrapolation (combination of zone mode change, coil, etc)
// set by getTsXtrapBounds
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::converger()		// interpolate/extrapolate ts (and ws) for iter4Fs

// cases: normally: accelerate completion, by jumping ts to final value or near it.
//        diverging in one direction: accelerate ts divergence to get to mode change.
//        diverging, alternate sign changes: desired solution is the unstable equilibrium from which
//             ts is diverging. Is approximated here, binClip forces completion.

// returns TRUE if alters aTs, with weFudgedTs set unless change small 4-25-95. Also Xtrap's aWs iff aTs changed.
{
#define XWS	// define to extrapolate ws as well as temp.  If undef can remove tsHis[].w and code that sets it.
	// TESTED 6-22-92: saves iterations & improves result with humMeth = Rob and humTolF = .0001 (T46, T47).
	//                 less clear @.0003 (present default).

// exit if can't do
	DBL r = tsHis[0].rdT;
	if (r==0)  return FALSE;		// if too few iterations or anything funny in last 2 or 3 iterations, can't do it.

// bounds
	DBL d=10.;
	getTsXtrapBounds(d);    		// set tsxLLim and tsxULim for +-d, mode, coil, max flow knee, etc ts xtrap bounds
	setToMin( tsxLLim, tsHis[1].t);   	// include prior ts in range: anywhere between ok for weird cases that reverse direction
	setToMax( tsxULim, tsHis[1].t);	// ...(getTsXtrapBounds already included aTs===tsHis[0].t in bounds.)

// tentative new ts
	DBL nuTs;
	if (r >= 1)						// r==1: parallel lines and r > 1: diverging (growing stairsteps)
		// no solution; go to mode change (max t, max c, vav damper peg, etc)
		nuTs = aTs + (tsHis[0].dT > 0 ? 10. : - 10.);	// move it 10 degrees the way it is going, subject to bounds.
	//  observed cases have finished fast once delta gets over a few degrees.
	else	 		// 0 < r < 1:  converging stairsteps (the common case); accelerate completion.
		// -1 < r < 0: converging square spiral (unseen?); accelerate completion
		// r < -1:     diverging (growing) square spiral: want the point it is spiraling AWAY FROM
		//               (unstable equilibrium); this expression is intended to approximate it
		//               (binClip probably usually finishes such a case).
		//   derivation: the t we want is that for which  t(c(t))==t  or  t[i+1] = t[i].
		//   write equation for t[i+1] as a function of t[i]:
		//       t[i+1] = tsHis[0].t + (t[i] - tsHis[1].t)*r;        where r = tsHis[0].rdT is slope.
		//       t      = tsHis[0].t + (t    - tsHis[1].t)*r;        solve.
	{
		nuTs = (tsHis[0].t - tsHis[1].t*r)/(1. - r);

		// before humXtrap, nuWs was computed here. See eg OUT459B.ZIP for if'd out code (!TRY8). 5-95.
	}

// limit ts
	setToMax( nuTs, tsxLLim);     	  			// keep within all known linearity bounds to prevent unnec coil modelling,
	setToMin( nuTs, tsxULim);   				//  xtrap beyond a knee in any curve, nonconvergent chaos, etc.

// if changed, also extrapolate w; store results.

	if (nuTs != tsHis[0].t)				// unless no change (cuz of bounds or lack of significance)
	{

#ifdef XWS
		// when extrapolating t, also extrapolate w in manner similar to convergent t. Else leave w for humXtrap.
		// note there are fewer conditions here than in humXtrap cuz we are extrapolating this tsHis entry already.
		// unlike humXtrap, negative rdW is used here: try to improve w that has been oscillating with oscillating t and c.
		DBL nuWs=tsHis[0].w;
		DBL wr2 = tsHis[0].rdW;   			// ratio of last 2 dW's, like rdT. 0 if not valid, just xtrap'd, etc.
		if ( wr2 != 0					// if have 3 w points, 2 non-0 dW's, & other conditions
				&&  ( coilUsed != cuCOOL			// only when cooling, because condensation on coil nonlinear,
					  || !tsHis[2].x				//  can't start from xtrap'd w
					  && tsHis[1].gud==tsHis[2].gud ) )	//  initial 0/part/full load must match
		{
			setToMin( wr2, .8);  				// limit non-alternating change in case a big oscillation, not a trend
			//setToMax( wr2, .7);    				I'm unclear about negative wr2 limit, 4-29-95.
			nuWs = (tsHis[0].w - tsHis[1].w*wr2)/(1. - wr2);			// inter/extrapolate w in same manner as t
			setToMax( nuWs, min( tsHis[0].w, .0002));				// don't xtrap below ws below .0002
			if (nuWs > tsHis[0].w)						// speed check
				setToMin( nuWs, max( tsHis[0].w, 					// don't xtrap into possible saturation
								 psyHumRat3( min( max( DBL(PSYCHROMINT), nuTs), 	// -100 floor prevents psychro err (T51, 1-95)
												  tsHis[0].t ))));			// .. (or apply tsxLLim sooner?)
			// on unexpected -1 return for over boiling, limits to tsHis.w.
		}
#endif

		// set flag to prevent iter4Fs exit unless changes very small.
		/* Extending converger to 0-power as well as full-power operation 4-95 sometimes saved many iterations
		   but frequently added 1 iter. Tolerances added here 4-26-95 to remove those 1 added iters. */
#ifdef TRY3a	// trial 6-16-97
*       if ( fabs(nuTs - aTs) > ( Top.tp_autoSizing ? Top.absTol/100	// .001 dflt: require tighter convergence when autosizing
*                                                : Top.absTol/10 ) )	// .01 dflt.  if changing ts "significantly"
#elif defined(TRY3a)	// earlier trial from
*       if ( fabs(nuTs - aTs) > Top.absTol/10		// .01 dflt.  if changing ts "significantly"
*        ||  Top.tp_autoSizing )				// or if autosizing: require tighter convergence when sizes changing
#else
		if (fabs(nuTs - aTs) > Top.absTol/10)		// .01 dflt.  if changing ts "significantly"
#endif
			weFudgedTs++;
#ifdef XWS
		// aWs change causes another iteration only via iter4Fs's regular endtest, 4-95.

		aWs = nuWs;					// store new value
#endif
		aTs = nuTs;					// store new value

		// make ts history info entry, flagged to prevent use as extrap basis
		memmove( tsHis+1, tsHis, sizeof(tsHis) - sizeof(tsHis[0]) );	// copy up, then put current in [0]
		memset( tsHis, 0, sizeof(tsHis[0]));				// set all members 0
		tsHis[0].t = aTs;   						// value
		tsHis[0].w = aWs;
		tsHis[0].c = cmix;  						// input on which based
		tsHis[0].x = 'c';						// visible "extrapolated" flag
		// .gud left 0 to prevent use for t extrapolation.
		// .dW left 0 to redundantly prevent use for w extrapolation.
		// air volumetric heat cap: iter4Fs calls ahVshNLims right after we return.

		tsmLLim = LLLIM;
		tsmULim = UULIM;    		// reset antRatTs-returned limits so can pass mode-change ts after here
		//  above may now unnec here as antRatTs now clears if ts passes
		return TRUE;					// return TRUE iff ts changed
	}
	return FALSE;
#undef XWS
}			// AH::converger
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::binClip()		// conditionally limit ts, ws changes to act like binary search

// halves increment it allows with each ts or ws change sign reversal

/* ts case: when leak/loss q exceeds coil power, possible (heat) ts can INCREASE as flow increases;
            when flow falls at setpoint ts then falls, restoring full flow; iter4Fs would loop forever.
            In this case t(c(t)) oscillates divergently: slight rise in t makes bigger fall in t(c(t)).
            Limiting t change here allows finding answer (t for which t(c(t))==t) even tho unstable. Spring 1992. */

// ws case: none known where this cures non-convergence, but should add to robustness 10-92.

// returns TRUE if alters aTs or aWs, with weFudgedTs set unless both changes very small
{
	if (tsHis[2].t==0.)   return FALSE;				// no action if not yet 3rd iteration
	BOO rv = FALSE;						// return value: TRUE if any change

// if ts reversed direction of change, init or halve max increment
	DBL tsIncr0 = tsHis[0].t - tsHis[1].t;			// current ts increment
	DBL tsIncr1 = tsHis[1].t - tsHis[2].t;			// previous ditto
	DBL thisMaxTsIncr = maxTsIncr;				// max increment to use: fudged at reversal
	if (tsIncr0 * tsIncr1 < 0.)					// if sign of change changed, is a reversal
		if (ntRev++==0)						// if first reversal
			thisMaxTsIncr = 					// set current limit same so not limited on fallthru
				maxTsIncr = max( fabs(tsIncr0), fabs(tsIncr1), .5);	// init max incr to larger of last 2 incrs, at least .5 degrees.
	// TESTED 11-92: minimum max incr seems to save a few iterations.
		else 							// at each successive reversal
			if (maxTsIncr > 1.e-11)					// unless incr might get insignificant when added to aTs, 5-29-92
			{
				maxTsIncr /= 2.;  					// halve maximum increment
				thisMaxTsIncr = 3.*maxTsIncr/2.;			// at reversal use 3/2 new incr (3/4 old): stagger points tested
			}

#ifdef WSCLIP	// if including code re aWs (undef'd at head file)
// if ws reversed direction of change, init or halve max increment
	DBL wsIncr0 = tsHis[0].w - tsHis[1].w;			// current ws increment
	DBL wsIncr1 = tsHis[1].w - tsHis[2].w;			// previous ditto
	DBL thisMaxWsIncr = maxWsIncr;				// max increment to use: fudged at reversal
	if (wsIncr0 * wsIncr1 < 0.)					// if sign of change changed, is a reversal
		if (nwRev++==0)						// if first reversal
			thisMaxWsIncr = 					// set current limit same so not limited on fallthru
				maxWsIncr = max( fabs(wsIncr0), fabs(wsIncr1),.001);	// init max incr to larger of last 2 incrs, not < .001.
	/* TESTED 10-21-92: w/o any minimum, observed starting @ 1e-7
	   & not converging (T18) or add many iterations (T16). */
		else 							// at each successive reversal
			if (maxWsIncr > 1.e-16)					// unless incr might get insignificant when added to aWs
			{
				// 1e-16 is early wild guess, decrease if necess.
				maxWsIncr /= 2.;  					// halve maximum increment
				thisMaxWsIncr = 3.*maxWsIncr/2.;			// at reversal use 3/2 new incr (3/4 old): stagger points tested
			}
#endif

//tried 5-9-95. failed: Added net iterations to T133e, T143f.
// // allow larger increment if will let converger() act this iteration
//    if (tsHis[0].rdT)						// if converger (called next) will act if we don't
//       if (fabs(tsIncr0) < 2*thisMaxTsIncr)			// if ts increment < than 2 x what we would allow
//          if (fabs(wsIncr0) < 2*thisMaxWsIncr)			// ws ditto
//             return rv;  					// then don't limit value chagnes

// limit ts change if > max increment
	if ( ntRev > 0						// if reversal(s) have occurred (otherwise max incr not set)
			&&  fabs(tsIncr0) > thisMaxTsIncr )   	  		// if current increment too big
	{
		aTs = (tsIncr0 > 0.) ? tsHis[1].t + thisMaxTsIncr   	// limit to maximum increment from previous value
			  : tsHis[1].t - thisMaxTsIncr;
#if defined(TRY3b1)	// NO! not without heat/cool distinction to bias toward smaller coils/fans.
*       if ( fabs(aTs - tsHis[0].t)
*            >  ( !(Top.tp_autoSizing && tsIncr0 < 0) ? Top.relTol		// absTol/100.  if we changed ts "significantly"
*                                                  : Top.relTol/100 ) )	// when autoSizing & size decreasing, tighten convergence
#elif defined(TRY3b)	// trial 6-16-97
*       if ( fabs(aTs - tsHis[0].t) > ( Top.tp_autoSizing ? Top.relTol/10	// when autoSizing, require tighter convergence
*                                                      : Top.relTol ) )	// absTol/100.  if we changed ts "significantly"
#elif defined(TRY3b) // older form (/100 no t/10)
*       if ( fabs(aTs - tsHis[0].t) > (!Top.tp_autoSizing ? Top.relTol		// absTol/100.  if we changed ts "significantly"
*                                                      : Top.relTol/100) )	// when autoSizing, require tighter convergence
#else
		if (fabs(aTs - tsHis[0].t) > Top.relTol)			// absTol/100.  if we changed ts "significantly"
#endif
			weFudgedTs++;						/* say don't exit yet: clipped ts not known achievable.
       								   else allow iter4Fs' regular endtest to exit.
       								   CAUTION: don't always set weFudgedTs: diverging case
       								   always clips or reverses near end: never exits. */
		tsHis[0].t = aTs;
		tsHis[0].x = 'k';					// say Klipped
		tsHis[0].gud = 0;					// not good for extrapolation
#ifndef HUMX2
		tsHis[0].dW = 0;    					// redundantly prevent w extrapolation from this entry
#endif
		rv = TRUE;						// return TRUE to say we did change value
	}

#ifdef WSCLIP	// if including code re aWs
// limit ws change if > max increment
	if ( nwRev > 0						// if reversal(s) have occurred (otherwise max incr not set)
			&&  fabs(wsIncr0) > thisMaxWsIncr )   	  		// if current increment too big
	{
		aWs = (wsIncr0 > 0.) ? tsHis[1].w + thisMaxWsIncr   	// limit to maximum increment from previous value
			  : tsHis[1].w - thisMaxWsIncr;
		if (fabs(aWs - tsHis[0].w) > Top.absHumTol/10.)		// if we changed ws "significantly". /10 is a wild guess.
			weFudgedTs++;						/* say don't exit yet: clipped ws not known acheivable.
       								   else allow iter4Fs' regular endtest to exit.
       								   CAUTION: don't always set weFudgedTs: diverging case
       								   may always clip or reverse near end, never exiting. */
		tsHis[0].w = aWs;
		tsHis[0].x = 'k';					// say Klipped. prevents use for w extraplation.
		tsHis[0].gud = 0;					// not good for t or w extrapolation
		tsHis[0].dW = 0;						// redundantly prevent w extrapolation from this entry
		rv = TRUE;						// return TRUE to say we did change value
	}
#endif
	return rv;
}			// AH::binClip
//-----------------------------------------------------------------------------------------------------------------------------
// getTsXtrapBounds (next) variables, in statics purely for ease of debug examination
DBL tsMaxFlo;   		// estimated aTs at which flow will reach max, allowing for estimated tr2Nx sink
DBL dTrdTsX;			// how much tr2Nx might sink: dTrdTs extrapolated to geometric limit
//-----------------------------------------------------------------------------------------------------------------------------
void AH::getTsXtrapBounds(DBL d)		// set aTs extrapolation bounds tsxLLim and tsxULim for converger()
{
// start with ts +- d degrees as given by caller
	tsxLLim = aTs - d;
	tsxULim = aTs + d;

// limit to ts values at which nearest zone mode changes occur (shape of curves changes)
	setToMax( tsxLLim, tsmLLim);
	setToMin( tsxULim, tsmULim);				// apply bounds as returned by antRatTs

// limit to range of good and uniform coil behavior -- don't go past ts sp, don't go past where model needs rerunning, etc
	setToMax( tsxLLim, texLLim - (tex1-aTs));
	setToMin( tsxULim, texULim - (tex1-aTs));	// apply doCoil's tex1 bounds, xlated to ts

// don't extrapolate ts into excessive ah flow -- just wastes time settling back (was this before flow limited at top loop??)

	// note: if current sum of terminal cMx's is less than sfan.cMx, tsmL/ULim will reflect a bound at corress ts.

	if ( coilUsed != cuNONE				// don't do if no coil used: tsMaxFlo===aTs, preventing any xtrap. 4-95.
			&&  fabs(tsHis[0].c - tsHis[1].c) > .01 )		/* don't do if flow not changing: found to be much to restrictive.
    							   Don't understand why -- all numbers and logic look good 4-6-92,
    							   But clearly interferes with convergence (eg file T5, where flow
    							   limited by terminal cMx, thus constant when floating).
    							   Does current form work in original cases? */
		// asFlow 7-95: looks moot & harmless for (autoSizing) flows > sfan.cMx.
	{
		dTrdTsX = dTrdTs/(1. - min(dTrdTs,.99)); 	/* effect of ts change on tr2: extrap 1-iteration observed tr2/ts
							   change to its own geometric limit to account for rest of the
							   iterations.  Always pertinent? */

		tsMaxFlo = ten + (tex1 - ten)*cmix/sfan.cMx + dtExSen + dtSenS;	/* approx ts of max flow: assumes no tmix change,
									   const coil power, fan heats not recomputed. */

		tsMaxFlo += dTrdTsX * (tsMaxFlo - aTs);		// adjustment for tr2 sink at last observed dtr/dts slope
		// without this, extrapolator useless in (some) floating cases.
		// economizer: assume dTrdTsX reflects it. BUT DOES NOT! 4-27-92

		if (coilUsed==cuHEAT)				// if heating, this is a lower supply temp limit
			setToMax( tsxLLim, tsMaxFlo);   			// combine with other bounds
		else
			setToMin( tsxULim, tsMaxFlo);     		// ..
	}

// insurance: include present ts in bounds
	setToMin( tsxLLim, aTs);
	setToMax( tsxULim, aTs);

}		// AH::getTsXtrapBounds
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::humXtrap()	// conditionally extrapolate aWs toward geometric series limit to accelerate convergence, 4-95

// returns TRUE if changes aWs.

/* Intended to accelerate completion, mainly when ts already converged,
   as when a small (eg leak) coupling to outdoors causes aWs's response to change
   in outdoor w to otherwise take many iterations to settle. 4-28-95. */

/* TESTED 4-28-95: helps with coil-off floats (as intended);
   often adds an iteration in ordinary cases (by making w change exceed tolerance), deemed ok due to accuracy gain. */
{
#ifdef HUMX2		// reduces checks made before setting rdW non-0
*// don't extrap here on the basis of inconsistent or extrapolated information
*    if ( tsHis[0].x || tsHis[1].x 		// if converger, binClip, humXtrap, etc modified either of last two t's or w's
*     ||  tsHis[0].gud != tsHis[1].gud )  	// if coil off/part/full load status changed
*       return FALSE;
#else
// don't extrap if another function has already extrapolated this tsHis entry's t and w (occurs after tsHis[0] set)
	if (tsHis[0].x)  return FALSE;	// but ok if .gud (consistently) 0: do extrap w for partial coil power.
#endif

// don't extrap if can't, don't need to, or w is diverging, or oscillating (sign of dW is changing, r < 0).
	/* dW might oscillate with oscillating zone flows and/or outside air, when t is oscillating.
	   Let t/flows converge enuf so w doesn't oscillate b4 xtrap'ing w. */
	DBL r = tsHis[0].rdW;		/* ratio of latest delta-W to preceding one. 0 if haven't done 3 iterations
					   w/o extrap, or some other condition make xtrap invalid or unnecessary. */
	//if (r < .1)			small rdW already changed to 0 in iter4Fs.
	if (r <= 0.)			// if too little data, don't need to or can't xtrap, or sign of delta-W changed (r < 0)
		return FALSE;			// don't extrap now.
	if (r >= 1.0)			// if not converging (dW's increasing)
		return FALSE;			// don't extrap now. Converging w's expected when t/flows settled.
	// could fall thru to accelerate divergence if case found where valid and useful; meanwhile be safe.

// TESTED 5-2-95: in 243 test runs affected only 3 runs, by only 1 iteration apiece. Don't bother:
// // don't extrap w now if might prevent converger() from acting next iteration
//    if ( fabs(tsHis[0].dT) 		// if have t delta, thus tsHis[0].gud == tsHis[1].gud and both non-0
//         > Top.absTol/10 		// and temp isn't well converged (/n cuz may DIVERGE if converger() doesn't act)
//                   //^^^^  tune: try /4, /1, if kept.
//     &&  !tsHis[2].gud )		// and tsHis[2].gud is 0 (part load or xtrap'd), thus tsHis[0].rdT zero
//       return FALSE;			/* don't use humXtrap's ability to operate on one less iteration after xtrap or part load,
//  					   let converger() act to converge t first */

	/* 4-95 tests re whether to act on one 'r' or await two that agree: acting on one accelerated the target
	   slow-converging-w float cases, but often added 1 iteration (+ accuracy) to ordinary cases that would
	   have exited iter4Fs if we hadn't changed w here. Decided on following speculative hybrid... */

// additional conditions when cooling, based on speculation about nonlinearity of condensing cool coils, 4-28-95.

	if (coilUsed==cuCOOL)		// if cooling. Note rdW's are 0 if coil use changed.
	{
		if (tsHis[2].x)    		// don't use rdW starting from xtrap'd w: condensation nonlinear.
			return FALSE;
		if (tsHis[1].gud != tsHis[2].gud)	//  initial 0/part/full load must match: condensation nonlinear.
			return FALSE;
		DBL r1 = tsHis[1].rdW;		// ratio of preceding dW to the one before it
		if (r1 <= 0)			// if too little data, condition precluding xtrap, sign change, etc
			return FALSE;			// don't xtrap now, and don't /0. This requires an addl iteration b4 cooling humXtrap.
		DBL rr = fabs(r/r1);		// 1.0 if r is constant
		if (rr > 1.05 || rr < .9)  	// if r is changing (wild guess values)
			return FALSE;			// don't xtrap now, iterate till converged into a linear zone
		setToMin( r, r1);			// use the smaller of current and preceding rdW's
	}

// common function extrapolates, limits, stores.
	return humXtrapI( r, 'w');
}				// AH::humXtrap
//-----------------------------------------------------------------------------------------------------------------------------
#ifdef HUMX2
*BOO AH::humXtrap2()	// conditionally extrapolate aWs #2, toward geometric series limit to accelerate convergence, 4-95
*
*	// this function is intended to accelerate consistent aWs moves even when binClip is active.
*	// benefit not found --> with w no longer binClip'd, there may not be aWs moves that aren't complete when t is converged.
*
*	// call after humXtrap, if that function does not act.
*
*// returns TRUE if changes aWs.
*{
*// on enough consistent w changes, help w along, despite action of other extrapolators.
*// intended to act while binClip active: eg to complete aWs change at big wO change, while binClip holds aTs converged.
*// (won't act after converger() cuz it inserts a tsHis entry with 0 dW).
*// believe ok act on data whose next-to-last w was extrapolated, cuz w change should make a small or negative following rdW.
*
* #if 0	// more conservative:
* *
* *   DBL r = min( tsHis[0].rdW, tsHis[1].rdW, tsHis[2].rdW);	/* smallest of 3 dW ratios (based on 5 w's).
* *   								   rdW's 0 if not enuf data yet, etc. */
* #else	// 2 not 3 rdW's increased effect, but added more iterations than removed. 5-12-95.
*
*    DBL r = min( tsHis[0].rdW, tsHis[1].rdW);			/* smallest of 2 dW ratios (based on 4 w's).
*    								   rdW's 0 if not enuf data yet, etc. */
* #endif
*    if (r <= 0)   return FALSE;					// no extrap if no consistent move in same direction
*
*    return humXtrapI( r, 'W');					// common function (next) finishes checks & does extrapolation
*}				// AH::humXtrap2
#endif
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::humXtrapI(		// aWs extrapolation common inner function

	DBL r,	// ratio of successive dW's (geo series term multiplier) on the basis of which to extrapolate
	char x )	// 'w', 'W', etc indicator to store in tsHis.x

// returns TRUE if did extrapolate. for humXtrap, humXtrap2
{
	setToMin( r, .95);			// use not more than .95 (extrap's by 19 times dW).  r's near .85 commonly observed 4-95.

// extrapolate: compute new ws as sum of geometric series -- assume rdW would remain constant.

	//DBL nuWs = tsHis[0].w + tsHis[0].dW * r / (1 - r);	geo series sum. simplifies to ..
	DBL nuWs =   tsHis[1].w + tsHis[0].dW / (1 - r);		// tentative new aWs: geometric limit if rdW stays same.
	// extremely small changes eliminated cuz iter4Fs zeroes small dW's and rdW's.

// limit the change (like converger())

	setToMax( nuWs, min( tsHis[0].w, .0002));					// don't xtrap below ws below .0002
	if (nuWs > tsHis[0].w)							// speed check
		setToMin( nuWs, max( tsHis[0].w, psyHumRat3(min(aTs,tsHis[0].t)) ) );	// don't xtrap into possible saturation
	if (nuWs==tsHis[0].w)							// if no change after limiting
		return FALSE;								// forget it

	aWs = nuWs;							// store new ws value

// update tsHis. Don't make another tsHis entry to record this transaction cuz would interfere with t interpolation.

	tsHis[0].w = aWs;			// store in history to view in debugger -- else believe moot since flagged.
	tsHis[0].x = x;			// 'w' or 'W': say an extrapolated w -- don't use as basis for more extrapolation
	// don't zero .gud: validity of t in this entry for t interpolation believed unaffected.
	tsHis[0].dW = 0;			// redundantly suppress w extrapolation from this tsHis

	/* don't set weFudgedTs -- normal iter4Fs endtests detect need to iterate.
	   (If it doesn't iterate, ah will have imbalance -- tolerate some for speed.)
	   Note that change made here had same sign as delta, thus always increasing change seen by endtest. */

	return TRUE;
}			// AH::humXtrapI
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::incrFanF()			// increase fanF some (increase flow)

// call conditionally, between cr1Nx determination and next aTs determination, if cr1Nx is too low and fanLimited is on.

// determines new cr1Nx/tr2Nx, using old cr1Nx and ts.  clears fanLimited if flow no longer limited by fan.

// returns TRUE if it does anything -- fanF and flow increased.
{
#undef OUTER	// define for outer loop.  If false, caller MUST call conditionally.
#define INNER	// seems to work better on here, tho mainly that's true re decrFanF.  Primarily file T24, 4-4-92.
	// If INNER permanently edited out, code out dcdf, possCr, increaseFlows arg.

	if (fanF >= fanFMax)     			// if there is no increase available
	{
		fanLimited = FALSE;    			// not fan-limited: insurance
		return FALSE;
	}
	BOO didReduce = FALSE;
	if (lastFanFCh < 0)					// if last decreased fan flow (this ah call)
	{
		if (nFanFChrev++)   				// count fanF direction reversals
			fanFChmult *= .7;				// if 1st or later reversal, reduce amount of change per call
		// increase method (using dfdc) more effective than decrease, so taper more & sooner on increase.
	}
	lastFanFCh = 1;					// say inreased
#ifdef OUTER
x    while (cr1Nx <= sfan.cMx * Top.loTol)
x	{
x       if (cr1Nx==0.)  break;       			// insurance
#endif
		AhresB.p[ss].S.nIterFan++;			// in ah subhour results subrecord, count times fan adjusted (up or down)
		DBL cIncr = 0., sink = 0.;			// for possible flow increase, sink for unused poss flow decrase
		DBL dcdf = cr1Nx/fanF;				// dc/df: flow change for fanF change: initially assume all tus at limit,
		//        actual may be less but not greater
#ifdef INNER
		for (SI n = 0; ; n++)				// local loop to increase fanF til flow possibly increased enuf.
		{
			// fewer iterations as nFanFChrev increases; always one.
			DBL fanFWas = fanF, cIncrWas = cIncr;
#endif
			if (dcdf==0.f)       				// if ahVshNLims below got NO cr1Nx change,
				fanF = fanFMax;				// just remove all limit
			else
			{
				fanF += fanFChmult 			// temper changes per global oscillation-damper
				* (sfan.cMx - cr1Nx - cIncr)/dcdf;	// increase fanF for (remaining) underload at current df/dc.
				fanXtrap(1);				// conditionally increase fanF to value extrapolated from history
			}
			setToMin( fanF, fanFMax);				// don't increase over effective limit
			ahVshNLims( aTs, cIncr, sink);   		// update (air vsh and) tu limits for new fanF, get poss flow increase.
			tsHis[0].gud = 0;				// t(c) not [forward-]comparable: vsh changed. [.ffc invalidates cHis[0])]
			// flow increases less than fanF as tu limit does not increase once fanF > tu's vfMx/vfDs,
			// and flow may not increase with limit in tstat-ctrl'd terminals.
#ifdef INNER
			if (fanF >= fanFMax)  			// done if no more increase avail. fanLimited will be cleared above.
				break;
			if (cr1Nx + cIncr/fanFChmult >= sfan.cMx * Top.loTol)	// done if have enough possible flow increase --
				break;     					// time to antRatTs to see what tstat tu's do.
#define MAXIINNER 5				// tested 4-92: reducing to 5 does not impact test files.
			/* increase method, using dfdc, more effective than decrease, so use
			   fewer increase reps (expect it to commonly finish on 2nd rep). */
			if (n + nFanFChrev >= MAXIINNER)		// fewer repetitions as reversals increase: intent is to taper hard-to-
				break;					// find change faster than fanFChmult as its effect can be radical.
			dcdf = (cIncr - cIncrWas)/(fanF - fanFWas);	// slope of possible cr1Nx vs fanF, for next iteration.
			// will remain same or get less as tu c's fall off max-pegs.
		}
#endif
		DBL cr1Was = cr1Nx;
		antRatTs();  	  				// determine flow for active tu's, thence cr1Nx/tr2Nx, for new limits.
		if (cr1Nx != cr1Was)
			didReduce++;					// say flow changed
		else  						// if NO (more) change: all dampers already unpegged
			if (fanF < fanFMax)   				// can this happen here?
			{

				// remove all fan flow limitation

				fanF = fanFMax;				// set too big to do anything
				ahVshNLims( aTs, cIncr, sink);  		// update (air vsh and) tu limits and fixed flows for new fanF
				tsHis[0].gud = 0;				// t(c) not [fwd-]comparable: vsh changed. [(.ffc invalidates cHis[0]).]
				// if (didReduce)  antRatTs(); 		if need can be shown (curr cr1Nx may be assumed on TRUE return)
			}
		if (fanF >= fanFMax)				// if no limitation remains (from just above or farther above)
		{
			fanLimited = FALSE;				// say fan not limiting flow
#ifdef OUTER
x          break;
#endif
		}
#ifdef OUTER
x       if (fanFChmult < 1.)				// if changes reduced cuz thrash detected
x          break;					// disable loops, let caller call repeatedly
x	}							// loop till fan within tol of full load or restriction removed.
#endif
	return didReduce;
}				// AH::incrFanF
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::decrFanF()		// reduce fan factor to reduce flow some

// does some decrease and redetermines cr1Nx for same ts; caller must iterate till flow reduction sufficient.

// conditionally called from iter4Fs, after cr1Nx determination, b4 next ts determination.
{
// seems better to do minimal reduction repeatedly in regular iteration, 4-1-92.  We think cuz tr2Nx rises cuz coldest zone cut.
#undef OUTER
#define INNER	// 4-4-92 works better defined: fewer total iterations despite occasional backtracks (file T24).
	// if INNER is edited out, also remove reduCr and arg to reduceFlows.

	BOO didReduce = FALSE;

	// note increase method (using dfdc) more effective than decrease, so reduce fanFChmult on increase only.
	lastFanFCh = -1;					// say last fanF change a decrease: see incrFanF re fanFChmult.
#ifdef OUTER
x    while (cr1Nx > sfan.cMx * Top.hiTol)
x	 {
#endif
		fanLimited = TRUE;				// say fan limited. cleared in incrFanF.
		AhresB.p[ss].S.nIterFan++;			// in ah subhour results subrecord, count times fan adjusted (up or down)
		setToMin( fanF, fanFThresh); 			// reduce fanF to where it will take effect (from antRatTs),
		DBL sink = 0., cDecr = 0.;			// for poss cr1Nx increase (unused) and (negative) decrease
#ifdef INNER
		//can't do the slope thing done when increasing flow without limiting to next limit-hit, as slope will INCREASE.
		for (SI n = 0; ; n++)				// local loop to reduce fanF til flow possibly reduced enuf.
		{
			// fewer iterations as nFanFChrev increases; always once.
#endif
			fanF -= fanFChmult 					// reduce fan factor by oscillation damping factor times
			* fanF * (1. - sfan.cMx/(cr1Nx + cDecr));	// remaining possible overload

			// flow falls less than fanF cuz hier fanF's don't effect limits of tu's with lo vfMx/vfDs ratio,
			// and limit change does not reduce flow if flow was already < new limit.
			// BUT: if flow reduction is to coldest zns, tr2Nx can rise, thence later ts, reducing other flows.

			fanXtrap(-1);						// conditionally decrease fanF to value extrapolated from history
			ahVshNLims( aTs, sink, cDecr);  			// update tu limits for new fanF (and air vsh), get poss flow decr
#ifdef INNER
			if (cr1Nx + cDecr/fanFChmult <= sfan.cMx * Top.hiTol)	// if enuf reduced, leave inner loop:
				break;      					// time to do antRatTs to confirm changes.
#define MAXDINNER 10				// tested: 5 began adding fan-down iterations, eg file T24,22 4-92
			// increase method, using dfdc, more effective than decrease, so use more decrease reps.
			if (n + nFanFChrev >= MAXDINNER)		// fewer repetitions as reversals increase: intent is to taper  hard-to-
				break;					// .. find change faster than fanFChmult as its effect can be radical.
		}
#endif
		DBL cr1Was = cr1Nx;
		antRatTs(); 				// determine flow for setpoint terminals, thence cr1Nx/tr2Nx, for reduced limits.
		if (cr1Nx != cr1Was)
			didReduce++;
#ifdef OUTER
x       if (fanFChmult < 1.)				// if changes reduced cuz thrash detected
x          break;					// disable loops, let caller call repeatedly
x	}
#endif
	return didReduce;
}				// AH::decrFanF
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::fanXtrap(		// conditionally increase fanF change by extrapolation from history

	SI whichWay )		// 1 for increase, -1 for decrease.  And uses file-global fHis[].

// This function may extrapolate a fanF value.
// new value will be stored in "fanF" if more changed from prior than current fanF value --
//  caller may pre-calculate value to use if more changed than conservative extrapolation here

/* Intended to speed fanF convergence in cases otherwise requiring many iterations.
   Note that this is adaptive in a way that our other code is not to relationship between fanF and tr2Nx/ts/cr1Nx
   -- how much damper motion and return air temp changes accentuate or cancel effects of fanF changes. */

/* somewhat sloppy approximation useful here as tolerance applies to final volume */
{
	if (!fanLimited)
		return FALSE;
	if (fHis[0].dF * whichWay <= 0.)			// if no last change or wrong sign
		return FALSE;
	if (fHis[0].rdF <= 0. || fHis[0].rdF > .99)		// if delta changed sign, or delta got larger (divergent or confused)
		return FALSE;					// ... or fHis[0 or 1].dF is zero (0 stored for very small)
	if (fabs(fHis[0].dF/fHis[0].f) > .15)		// if last change more than 15%, iterate again to be surer of info
		return FALSE;					// -- intent here is to accelerate completion once in ball park
	DBL r = fHis[0].rdF;

	// extrapolate conservatively.  Overshoot observed from direct extrapolation here from one rdF.

	SI mR = 0;						// multiple-R's flag
	if (fHis[1].rdF <= 0.)  				// if don't have additional non-0 rdF of same sign
	{
		if (fHis[1].rdF < 0)  				// if prior fanF change was a direction reversal
			return FALSE;					// don't extrapolate yet
		else 						// 0: no prior fanF change or it was an extrapolation
			r *= .9;					// use only this much of single ratio (based on 2 differences, 3 points)
		// reduce if bad cases found.  On file T24, using .8 --> more iterations.
	}
	else
	{
		mR++;						// more than 1 rdF considered
		setToMin( r, fHis[1].rdF);				// have previous rdF (4 fanF's): use it as upper limit also
		if (fHis[2].rdF <= 0.)  				// if don't have 2nd addl rdF
			r *= .95;					// use only this much of ratio derived from smallest of 2 .rdF's
		else
			setToMin( r, fHis[2].rdF);			// have 2nd previous rdF (5 fanF's): use it as upper limit also
		// smallest of 3: use it all: assume geometric extrapolation valid to the extent that 3 change ratios match.
	}
	setToMin( r, .8);					// restrict ratio -- insurance -- expected ratios around 2/3 or less
	r *= fanFChmult;					// times 1.0, or less to damp changes if too many sign changes detected

	DBL tFanF = fHis[1].f + fHis[0].dF / (1. - r);	// approximate as limit of geometric progression of changes

	setToMax( tFanF, fHis[0].f * .5);			// limit changes -- insurance
	setToMin( tFanF, fHis[0].f * 1.5);			// -- intent here is to get last sig digits once in ball park
	if (tFanF==fHis[0].f)				// if no change (due to significance or ??)
		return FALSE;
	// if extrapolation adds less than 20% (1 rdF) or 10% (2+ rdF's) don't bother, gather data instead.
	if (fanF != fHis[0].f)						// /0 protection
		if ((tFanF - fanF)/(fanF - fHis[0].f)  <  (mR ? .1 : .2))
			return FALSE;							// (opp'ty cost: can't extrapolate again for 2-3 itrns)
	if ((tFanF - fanF) * whichWay <= 0.)   		// if caller has already changed fanF more in indicated direction
		return FALSE;					// this test believed redundant except protects re wrong whichWay.

	fanF = tFanF;					// store our more-changed extrapolated fanF

// update fanF history to suppress extrapolation from/accross this point

	memmove( fHis+1, fHis, sizeof(fHis) - sizeof(fHis[0]) );	// copy up, then put current in [0]
	memset( fHis, 0, sizeof(fHis[0]));				// init entry all 0
	fHis[0].f = fanF;						// store current value
	// leave .dF and .rdF 0 in this entry: not valid extrapolation basis.
	fHis[0].x = 'g';						// and .gud left 0.

	return TRUE;
}			// AH::fanXtrap
//----------------------------------------------------------------------------------------------------------------------------
// variables for airHandler outside air/economizer: for doOa and doEco.
LOCAL DBL NEAR mnPo;			// current minimum fraction outside air
LOCAL DBL NEAR mxPo;			// current maximum ditto
//----------------------------------------------------------------------------------------------------------------------------
void AH::doOa()		// do outside air for airHandler

// uses: tr2,wr1,cr1, .

// sets: po:          economizer fraction outside air
//       ecoEnabled:  TRUE if economizer present and enabled
//       coilLockout: TRUE when non-integrated economizer disables cooling coil
//       tmix, wen:   mixed air conditions (mixed w same as coil entry w)

// caller: pute4Fs
{
	DBL cnv = Top.airX(tr2);  			// cfm to heat cap flow conversion factor @ ret air temp: ft3/min --> Btu/F-hr.
	DBL cr1On = frFanOn != 0. ?  cr1/frFanOn			// use fan-on flow re const vol outside air & leaks
				:  cr1==0. ? 0. : sfan.cMx;	// /0 protection: insurance; unexpected.

// current min and max outside air fractions
	DBL tem = 					// current min outside air, for design flow if FRAC control method, cfm
		oaMnFrac   			// scheduled fraction outside air to use now (eg to shut off during warmup)
		* oaVfDsMn;			// design min outside air, cfm
	if (oaMnCm==C_OAMNCH_VOL)			// if min outside air specified by volume
		mnPo = (cr1On != 0.) ? tem * cnv / cr1On	// conv min flow to heat cap units (to match cr1On), div by cr1On to get fraction.
			   : (tem != 0.) ? 1.	// no flow thru airHandler. if non-0 min outside air, say use all outside air.
			   : 0.;	// 0 flow and 0 min outside air ---> 0 min ouside air fraction.
	else					// min outside air is specified by fraction
		mnPo = tem / sfan.vfDs;			// divide min flow by design flow (cfm) to get fraction
	mxPo = 1.;					// max outside air: no user control, but return air leakage may make < 1.

// oa/ra damper leakage: limits min and max outside air
	if (!(Top.tp_pass1A && asFlow))		// if autoSizing flow on pass 1 with open-ended models,
 										// omit capacity-based leaks cuz sfan.vfDs is being determined.
	{
		tem = oaEcoTy==C_ECOTYCH_NONE ? oaVfDsMn : sfan.vfDs;	// oaOaLeak is frac dsn flow if eco, else frac min flow
		DBL oaLeakC = tem * oaOaLeak * Top.tp_airxOSh;		// outside air leak. convert cfm to hc at outside temp.
		DBL raLeakC = sfan.vfDs * oaRaLeak * cnv;    		// return air leak, heat cap units
		if (oaLeakC + raLeakC)					// if there is any leakage
			if (oaLeakC + raLeakC >= cr1On)    			// if more leakage than flow (incl cr1On==0)
				mnPo = mxPo = oaLeakC/(oaLeakC+raLeakC);		// apportion flow in ratio of leakages
			else							// leak less than flow: normal case; implies cr1On non-0
			{
				setToMax( mnPo, oaLeakC/cr1On);   			// oa leakage gives min outside air flow
				setToMin( mxPo, 1. - raLeakC/cr1On);			// ra leakage gives max outside air flow
			}
	}

// fraction outside air if no economizer or disabled
	setToMin( mnPo, 1.);				// min outside air never > 100%
	setToMax( mxPo, mnPo);				// min overrules max (insurance: believe cannot occur)
	po = mnPo;					// init to min outside air flow
	ecoEnabled = FALSE;				// init to economizer disabled
	coilLockout = FALSE;			// init to cooling coil not disabled by non-integ economizer

// if economizer present and enabled, determine fraction outside air
	if (oaEcoTy != C_ECOTYCH_NONE)		// if have economizer
		doEco();					// determine tmixWant, ecoEnabled, po, coilLockout

	DBL toa = Top.tDbOSh;
	DBL woa = Top.wOSh;

// if heat recovery present do that
	if (IsSet(AH_OAHX + HEATEXCHANGER_SENEFFH))
	{
		AIRFLOW hxSupOutAF = ah_doHR();
		toa = hxSupOutAF.as_tdb;
		woa = hxSupOutAF.as_w;
	}

// mixed air conditions
	tmix = (1. - po) * tr2 + po * toa;
	wen  = (1. - po) * wr1 + po * woa;
	cmix = cr1; 				/* flow out of eco is same as flow into it.  ---> if this assumption changed,
    						   check all the compares of cr1Nx/cr1 to sfan.cMx: flow at sfan is cmix. */
}				// AH:doOa
//----------------------------------------------------------------------------------------------------------------------------
void AH::doEco()			// do econimizer for ah, called only if eco present.

// inputs include: mnPo, mxPo, tsSp1, tr2, wr1, .
// assumed at entry: po = mnPo, ecoEnabled = coilLockout = FALSE.
// outputs: po, ecoEnabled, coilLockout.

// caller: doOa
{
	/* disable eco when cooling disabled in ahMode: occurs under ahTsSp=ZN or ZN2 when running fan only or heating
	   (under other ctrl methods AH doesn't know whether heating or cooling, and cooling is enabled in ahMode whenever AH is on).
	   1. don't run when fan only so modelling independent of setpoint definition when not heating nor cooling.
	   2. don't run eco when heating cuz eco IS typically disabled in single zone system if not cooling
								   (phone conversation with Bruce Wilcox, 8-18-92). */
	if (!(ahMode & ahCOOLBIT))
		return;

// disable eco if outside air too warm
	if (ISNUM(oaLimT))
	{
		if (Top.tDbOSh >= oaLimT)
			return;
	}
	else if (CHN(oaLimT)==C_RANC_RA)
	{
		if (Top.tDbOSh >= tr2)
			return;
	}
#if defined( _DEBUG)
	else
	{
		rer( PWRN,(char *)MH_R1277,VD oaLimT);    // "airHandler '%s': bad oaLimT 0x%lx"
		return;
	}
#endif

// disable eco if outside air too enthalpic
	if (ISNUM(oaLimE))
	{
		if (Top.hOSh >= oaLimE)
			return;
	}
	else if (CHN(oaLimE)==C_RANC_RA)
	{
		if (Top.hOSh >= psyEnthalpy( tr2, wr1))
			return;
	}
#ifdef DEBUG
	else
	{
		rer( PWRN,(char *)MH_R1278,VD oaLimE);    // "airHandler '%s': bad oaLimE 0x%lx"
		return;
	}
#endif

	ecoEnabled = TRUE;					// if here, economizer is present and enabled

// determine economizer setpoint
	DBL tmixWant = tsSp1				// mixed air "setpoint": supply air setpoint..
				   - sfan.dT;				/* ..less supply fan heat (whether drawthru or blowthru),
                      					   to simulate sensor after fan & coils. */

// compute fraction outside air and whether coil locked out
	if (Top.tDbOSh >= tr2)  				// if here when too hot out, fallthru wd divide by 0 or go wrong way
	{
		if (tmixWant < tr2)      			// if want mixed air cooler than return air,
		{
			po = mxPo;					//  actual equipment uses use max oa even tho warmer
			if (oaEcoTy != C_ECOTYCH_INTEGRATED)		// unless integrated economizer (or perhaps TWO_STAGE, later)
				coilLockout = TRUE;			// full outside air locks out coil
		}
		// .. else: want warmer than tr2: min po and coilLockout=FALSE are preset.
	}
	else
	{
		po = (tmixWant - tr2)/(Top.tDbOSh - tr2);  	// compute fraction outside air to yield tmixWant
		if (po >= mxPo)					// if want max (or more) outside air
		{
			po = mxPo;					// limit to max
			if (oaEcoTy != C_ECOTYCH_INTEGRATED)		// unless integrated economizer (or perhaps TWO_STAGE, later)
				coilLockout = TRUE;			// full outside air locks out coil
			//min oa must not lock out coil; in between does not matter as ts setpoint is acheived by eco.
		}
		setToMax( po, mnPo);				// not less than minimum
	}
}			// AH::doEco
//-----------------------------------------------------------------------------------------------------------------------------
AIRFLOW AH::ah_doHR()			// do heat recovery for ah

// inputs include: tsSp1, tr2, wr1
// outputs: heat exchanger outlet conditions.

// caller: doOa
{
	DBL mOA = sfan.c / Top.tp_airSH * po;

	AIRFLOW inletAF(mOA, Top.tDbOSh, Top.wOSh);  // TODO: Make record member?

	if (po == 0.f)
	{	// Skip if there is no outdoor air
		return inletAF;
	}

	AIRFLOW exhInletAF(mOA, tr2, wr1);

	// determine mixed air setpoint
	DBL tmixWant = tsSp1				// mixed air "setpoint": supply air setpoint..
				   - sfan.dT;				/* ..less supply fan heat (whether drawthru or blowthru),
                      					   to simulate sensor after fan & coils. */
	
	// Desired temperature from outdoor air system
	DBL toaWant = (tmixWant - tr2*(1.f - po))/po;

	ah_oaHx.hx_begSubhr(inletAF, exhInletAF, toaWant);
	return ah_oaHx.hx_supOutAF;
	
}			// AH::ah_doHR
//-----------------------------------------------------------------------------------------------------------------------------
DBL AH::getTsMnFo(	// get supply temp zone-controlled ah will deliver at control terminal's minimum flow with coils off.

	BOO outsideCall )		// TRUE if called not from within air handler

// should not be called if ctu->cMn is 0.

// called in fairly rare ZN/ZN2 cases in cnztu.cpp:ZNR::ztuMode, and in setTsSp1 for estimating ZN/2.
// Calculates value only when needed and only 1st time per iteration.
{
	if (!tsMnFoOk)		// flag cleared where return conditions change: ahEstimate, puteRa, possibly setTsSp1, .
	{
		tsMnFo = tsFo( 0, outsideCall);	// calculate fan-only supply temp with min ctu flow (below)
		tsMnFoOk = TRUE;
	}
	return tsMnFo;
}
//-----------------------------------------------------------------------------------------------------------------------------
DBL AH::getTsMxFo(	// get supply temp zone-controlled ah will deliver at control terminal's maximum flow with coils off.

	BOO outsideCall )		// TRUE if called not from within air handler

// called in fairly rare ZN/ZN2 cases in cnztu.cpp:ZNR::ztuMode.
// Calculates value only when needed and only 1st time per iteration.
{
	if (!tsMxFoOk)		// flag cleared where return conditions change: ahEstimate, puteRa, possibly setTsSp1, .
	{
		tsMxFo = tsFo( 1.f, outsideCall);  	// calculate fan-only supply temp with full ctu flow (just below)
		tsMxFoOk = TRUE;
	}
	return tsMxFo;
}
//-----------------------------------------------------------------------------------------------------------------------------
DBL AH::tsFo(		// get supply temp zone-controlled ah will deliver at specified approach to ctu max flow
	float approach,
	[[maybe_unused]] BOO outsideCall )		// TRUE if called not from within air handler

// should not be called for 0 flow (0 approach when ctu min flow is 0)

// for getTsMnFo, getTsMxFo. Including during .asFlow autoSizing 7-95.
{
	DBL tsFo = aTs;				// init result to current supply temp re terminal duct leak.
	DBL crx = crNx, trx = trNx;     		// ah inputs to adjust to control terminal flow
	TU * ctu = TuB.p + ahCtu;      		// point control terminal record (if none, points TuB.p[0]... safe)
	ZNR * czp = ZrB.p + ctu->ownTi;		// point zone record for control terminal
	DBL tsFoWas;
	SI i = 0;
	do   			// iterate cuz output is an input re terminal supply duct leak to return duct
	{
		if (ahCtu)						// insurance: no flow adjustment if ah not tu-controlled
		{
			DBL cMx= ahMode==ahCOOLING ? ctu->cMxC : ctu->cMxH;	// max flow for ah mode
			DBL cM = ctu->cMn + approach * (cMx - ctu->cMn);	// ctu flow at vav box at requested approach to maximum
			DBL czM = ctu->znC(cM);				// ctu flow at zone for vav flow (after leak)

			if ( !TuB.p[tu1].nxTu4a				// test 'next' of first tu to detect only 1 terminal
					&&  tu1==ahCtu )					// redundant: if 1st terminal is control terminal
			{
				// usual case: only one terminal, compute crx and trx anew
				crx = cM;     					// use ctu's flow as total return flow: exact
				if (crx)      					// don't divide by 0
					trx = ( czp->tz * czM      			// return air temp: contribution from zone
							+ tsFo * ctu->rLeakC(cM)		//  contribution from ctu supply duct leakage to return duct
						  ) / crx;
				// TESTED didn't see difference in results from adding this 1-tu case here,
				// but following ASSERTs failed, indicating there is a difference, so kept, 5-10-95:
				//  ASSERT( fabs(crNx - ctu->aCv) < .01);
				//  if (crNx)  ASSERT( fabs(trNx - (czp->aTz * ctu->aCz() + aTs * ctu->aRLeakC())/crNx) < .001);
			}
			else     						// multiple terminals
			{
				// adj crx & trx for ctu min flow.  May be inexact if aCv,tz changed since crNx/trNx calc'd.
				crx = crNx + cM - ctu->aCv;			// adjust total return air flow for min control terminal flow.
				if (crx)    					// don't divide by 0
					trx = ( trNx * crNx     				// adjust return temp for flow change
							+ czp->tz * czM  -  czp->aTz * ctu->aCz()    	//  adjust zone temp * flow
							+ tsFo * ctu->rLeakC(cM) - aTs * ctu->aRLeakC()	//  adjust leakage contribution (see zRat/antRatTs)
						  ) / crx;						//  divide new sigma(t*c) by new return flow
			}
			/* wrx: not needed by tsfo without economizer
			 * wrx = ( wrNx * crNx					// adjust return hum for flow change
			 *         + czp->aWz * (czM - ctu->aCz())    		//  adjust zone's contribution
			 *         + aWs * (ctu->rLeakC(cM) - ctu->aRLeakC())	//  ... (aWs, aWz assumed same)
			 *       ) / crx;   					//  divide new sigma(w*c) by new return flow */
		}
		tsFoWas = tsFo;
#ifdef TSPOSS
		tsFo = trNx;				// if no flow (unexpected), use previous return air temp
		if (crx)					// don't divide by 0
			tsPoss( trx, 0/*wrx*/, crx, 		// calc fan-only ts: like entire ah calc except coils, iteration. just below.
					-1,				//   want: -1 says don't use coils or economizer (wrx not needed).
					tsFo );			//   receives cal'd ts
#else
		tsFo = crx ? tsfo( trx, crx)             // calc fan-only ts: like entire air handler calc except coils, iteration
			   : trNx;                       // if no flow (unexpected), use previous return air temp
#endif
	}
	while ( ctu->tuSRLeak != 0     		// iterate if terminal supply duct is leaky cuz supply air leaks into return
			&&  fabs(tsFo-tsFoWas) > Top.absTol	// while tsFo changed by more than tolerance
			// ^^^^^^^^   TESTED larger tol here (.5) messed up a low-demand case (T131c,d 21:0) 5-8-95
			&&  ++i <= 4 );    			// max 4 times: speed / bug tolerance; if 10% effect, makes approx .01%, right?

	return tsFo;
}			// AH::tsFo
//-----------------------------------------------------------------------------------------------------------------------------
#ifdef TSPOSS	// def'd at top file. Use tsPoss() if something calls it... else shorter tsfo (next) is sufficient. Rob 6-97.

BOO AH::tsPoss(		// detailed computation of possible supply temp. Derived from tsfo (follows), 6-97

	DBL _tr, DBL _wr, DBL _cr,   	/* use trNx, wrNx, crNx   or   tr, wr, cr   modified as desired.
					   _wr value does not matter if want is -1 (used for eco control, coil input). */
	int want,				// 0 heating, 1 (ONE) cooling, -1 neither - coils disabled
	DBL &_ts )				// receives supply temp for given inputs. quite precise if fan only, approx with coils.
//DBL &_ws				add if return desired - and put proper wr in calls.
// w very sloppily modelled and currently (6-97) not returned anyway

// other runtime inputs used include sfan.cMx, fcc, .

// returns FALSE if can't do. only possible if want not -1. eg non-autosizable coil model, or no coil size yet, etc.

// for wzczSp special case; getTsMnFo; setTsSp1 fcc estimating addition 6-97.
// if kept, otta share code with regular computation --> put the -Nx and regular variables in a substruct block? 5-2-95.
{
	// save fan members -- some are used elsewhere, all might be probed for reports.
	// instead add mbr fcn that returns dT without altering members? 5-95.
	float  sfanCMx = sfan.cMx,  sfanC = sfan.c,  sfanT = sfan.t,    sfanFrOn = sfan.frOn,
		   sfanP = sfan.p,      sfanQ = sfan.q,  sfanDT = sfan.dT,  sfanQAround = sfan.qAround;
	float  rfanCMx = rfan.cMx,  rfanC = rfan.c,  rfanT = rfan.t,    rfanFrOn = rfan.frOn,
		   rfanP = rfan.p,      rfanQ = rfan.q,  rfanDT = rfan.dT,  rfanQAround = rfan.qAround;


// return leaks/losses/fan heat like puteRa()


	DBL _tr1 = _tr, _tr2 = _tr, _wr1 = _wr, _cr1, _frFanOn = 1.;		// sets these using _tr, _wr, _cr, fcc, .
	{
		// init leakage. story in puteRa.
#ifdef AUTOSIZE//7-95
		DBL leakFracOn = Top.tp_pass1A && asFlow  		// fan-on leakage as fraction of supply fan design flow
						   ? 0							// use 0 during autoSizing pass 1 part A: determining fan design flow
						   : (ahROLeak + ahSOLeak)/2.;	// use average of user's supply & return leaks, for balance for now.
#else
x       DBL leakFracOn = (ahROLeak + ahSOLeak)/2.;   	// fan-on leakage as fraction of supply fan design flow
#endif
		leakCOn = sfan.vfDs * leakFracOn * Top.tp_airxOSh;	// fan-on leakage in heat cap units (conv cfm to Btu/hr-F @ tDbOSh)
		setToMin( leakCOn, sfan.cMx/* *.99? */);		// don't leak more than fan can blow even if airx @ fan << outdoors.

		// determine fan fraction on now: uses fan-on leakeage; needed to determine average leakage. 6-24-92.
		if (fcc)						// if fan cycles this hour (set in AH::begHour; else _frFanOn preset 1.)
			// set _frFanOn given _cr, sfan.cMx, uUseAr, leakCOn, ctu cMx's. see story in setFrFanOn().
		{
			// fan-on flow === max poss avg flow = smaller of ctl tu max flow, and supply fan's max flow with overrun less leakage.
			DBL _cMx = 0.;			// .. typical sensible input wd be sfan.cMx==ctu->cMxC==cMxH; not enforced.
			if ( uUseAr != uNONE			// unused ah can occur at startup or ?if tu requesting no air.. _cMx is 0.
					&&  uUseAr != uMn )			// min flow: ctu->cMn required 0 (elsewhere) with fanCycles. _cMx is 0.
			{
				TU* ctu = TuB.p + ahCtu;			// point ah's control terminal (ahCtu verified non-0 in AH::begHour)
				_cMx = 					// flow for frFanOn=1. this subhour for this ah/tu is smaller of tu, sfan
					min( uUseAr & (uStC|uMxC)   	// test whether ah is on cuz terminal called for cooling or heat
						 ? ctu->cMxC : ctu->cMxH,	// tu cool or heat max flow per current use (set by AH::vshNLims)
						 sfan.cMx			// supply fan max flow, incl overrun (FAN::pute sets).
						 - leakCOn );		// less fan-on leak-in (==leak-out) to get poss tu flow at max sfan flow
			}
			// next iteration fraction fan on === fraction possible power requested for subhour by terminal === _cr/_cMx.
#ifndef AUTOSIZE
x          _frFanOn = _cMx==0. ? 0.		// if no flow is possible, don't run fan, don't divide by 0 (_cr is 0)
x                              : _cr/_cMx;	// fraction on = fraction of ah poss output needed = requested flow / max flow
#else
			_frFanOn = _cMx==0. 			// if no flow is possible, don't divide by 0,
					   ?  _cr ? 1. : 0.	//   but run fan anyway if there is demand (possible at start of autoSize).
		   :  _cr/_cMx;		// fraction on = fraction of ah poss output needed = requested flow / max flow
			setToMin( _frFanOn, 1.0);			// if autoSizing tu's & not supply fan, flow allowed to exceed sfan.cMx. 7-95.
#endif
		}				// end of code like setFrFanOn

		// return duct leak and loss (loss tentatively done second)
		DBL leakC = leakCOn * _frFanOn;   		// average leakage, heat cap units
		_cr1 = _cr + leakC;    				// specified amount of air leaks INTO return duct (duct is under vacuum)
		if (_cr1 != 0.)					// if there is flow
		{
			_wr1 = (_wr * _cr + Top.wOSh * leakC)/_cr1;		// hum rat after leakage: needed if economizer modelled
			_tr1 = (_tr * _cr + Top.tDbOSh * leakC)/_cr1; 	// temp after leakage
			_tr1 = (1. - ahROLoss) * _tr1 + ahROLoss * Top.tDbOSh;	// temp after conduction loss to outside
		}
		else if (ahROLoss != 0.)				// no flow (including no leak). If there is conductive loss,
			_tr1 = Top.tDbOSh;					/* set temp to outdoor for results 6-16-92 (known inconsistent:
   								   ahROLoss definition is "fraction of temp difference") */
		// else: no flow, no conduction: leave _tr1 unchanged.
	}

	float sfanQAroundWas;		// saves value for loop endtest
	SI l = 0;				// repetition count so can limit
	do					// repeated if sfan.qAround changes. End of loop near end of fcn.
	{
		sfanQAroundWas = sfan.qAround;	// save to compare after sfan.pute way below.

		{
			// code like puteRa() continues...

			// return fan heat, and return or relief fan motor heat if in air stream
			DBL _dtRfan;
			if (rfan.fanTy==C_FANTYCH_RETURN)		// if have return fan (relief fan done after eco, when flow known)
			{
				rfan.fn_pute( _cr1, _tr1, _frFanOn);		// compute fan cMx, p, q, dT, etc for flow and temp. cnfan.cpp.
				_dtRfan = rfan.dT;				// temp change to return air due to return fan work and motor heat
			}
			else						// NONE or C_FANTYCH_RELIEF: leave 0: relief fan heats exhaust air only.
				_dtRfan = 0;    				// (relief fan heats exhausted air only)
			if (sfan.motPos==C_MOTPOSCH_INRETURN)		// if supply fan motor is in return air
				if (_cr1)					// don't divide by 0
					_dtRfan += sfan.qAround/_cr1;		// supply fan motor heat warms return air. qAvg/cAvg===qOn/cOn.
			_tr2 = _tr1 + _dtRfan;    			// next-iteration value for tr2.
			// pute4Fs() copies _tr2 to tr2; thus tr2 unchanged if exit now.
		}	// end of code like puteRa()


		// outside air like doOa()


		DBL _tmix, _wmix, _cmix;		// sets _tmix, _wmix, _cmix,  using _tr2, _cr, _wr1, _frFanOn; sfan.cMx.
		BOO _coilLockout = FALSE;
		{
			DBL cnv = Top.airX(_tr2);						// cfm to heat cap factor for return air
			DBL cr1On = _frFanOn != 0 ? _cr1/_frFanOn : _cr1==0. ? 0. : sfan.cMx;	// fan-on flow

			// min/max oa fractions
			DBL tem = oaMnFrac * oaVfDsMn;    		// current min outside air, cfm
			DBL _mnPo = 					// current min fraction outside air...
				oaMnCm==C_OAMNCH_VOL    			//  if oa spec'd by volume
				? (cr1On != 0.) ? tem * cnv / cr1On    	//    compute fraction of return flow
				: (tem != 0.) ? 1. : 0.		//    if no flow, any min cfm is 100% fraction
				: tem / sfan.vfDs;          //  oa by fraction: compute fraction of supply fan cfm
			DBL _mxPo = 1.;    				// max outside air -- not needed without economizer 5-5-95

			// oa/ra damper leakage
			if (!(Top.tp_pass1A && asFlow))				/* if autoSizing flow on pass 1 with open-ended models,
	 							   omit capacity-based leaks cuz sfan.vfDs is being determined.*/
			{
				tem = oaEcoTy==C_ECOTYCH_NONE ? oaVfDsMn : sfan.vfDs;   	// oaOaLeak is frac dsn flow if eco, else frac min flow
				DBL oaLeakC = tem * oaOaLeak * Top.tp_airxOSh;   		// outside air leak. convert cfm to hc at outside temp.
				DBL raLeakC = sfan.vfDs * oaRaLeak * cnv;			// return air leak, heat cap units
				if (oaLeakC + raLeakC)    					// if there is any leakage
					if (oaLeakC + raLeakC >= cr1On)				// if more leakage than flow (incl cr1On==0)
						_mnPo = _mxPo= oaLeakC/(oaLeakC+raLeakC);     	// apportion flow in ratio of leakages
					else   							// leak less than flow: normal case; implies cr1On non-0
					{
						setToMax( _mnPo, oaLeakC/cr1On);   			// oa leakage gives min outside air flow
						setToMin( _mxPo, 1. - raLeakC/cr1On);			// ra leakage gives max outside air flow
					}
			}

			// fraction outside air if no economizer or disabled
			setToMin( _mnPo, 1.);				// min outside air never > 100%
			floor (_mxPo, _mnPo);  			// min overrules max (insurance: believe cannot occur)
			DBL _po = _mnPo;   				// init to min outside air flow

			// if economizer present and enabled, determine fraction outside air, like doEco().
			// uses _mnPo, _mxPo, _po, _tr2, _wr1, ecoLow. Sets _po, _coilLockout.
			if (want > 0)					// if caller enabled cooling coil use -- no eco use for fan only or heat
				if (oaEcoTy != C_ECOTYCH_NONE)			// if have economizer
					if (ahMode & ahCOOLBIT)				// if cooling now enabled
					{
						if ( ISNUM(oaLimT) ? Top.tDbOSh >= oaLimT     	// disable eco if outside air too warm
								: CHN(oaLimT)==C_RANC_RA
								&& Top.tDbOSh >= _tr2 )
							;
						else if ( ISNUM(oaLimE) ? Top.hOSh >= oaLimE		// disable eco if outside air to enthalpic
								  : CHN(oaLimE)==C_RANC_RA
								  && Top.hOSh >= psyEnthalpy( _tr2, _wr1) )
							;
						else				// eco enabled. compute fraction outside air
#if 1
						{
							_po = _mxPo;		// if here, want max cooling; actual equip uses max outdoor even if warmer.
							if (oaEcoTy != C_ECOTYCH_INTEGRATED)	// unless integrated economizer (or perhaps TWO_STAGE, later)
								_coilLockout = TRUE;			// full outside air locks out coil
						}
#else // from commented code activated 6-97; don't know what original plan for ecoLow was.
x           if (Top.tDbOSh >= _tr2)    	// if too hot out
x           {  if (ecoLow)      		// if want mixed air cooler than return air (else mnPo preset),
x                 _po = _mxPo;    	//  actual equipment uses use max oa even tho warmer
x			}
x           else if (ecoLow)   		// if lowest supply temp desired
x              _po = _mxPo;   		// use max outside air. Else mnPo preset.
#endif
					}				// end of code like doEco()

			// mixed air temp
			_tmix = (1. - _po) * _tr2 + _po * Top.tDbOSh;
			_wmix = (1. - _po) * _wr1 + _po * Top.wOSh;		// might be used by estCoils

			_cmix = _cr1;    			// flow out of eco is same as flow into it
		}				// end of code like doOa


// fan heats and coils, like pute4Fs. No loop-back cuz want extreme value rather than to meet a setpoint


		// determine _ts, (_ws) using _tmix, _wmix, _cmix, _frFanOn, want
		{
			// blowthru fan
			DBL _ten = _tmix;
			if (sfan.fanTy==C_FANTYCH_BLOWTHRU)
			{
				sfan.fn_pute( _cmix, _tmix, _frFanOn);
				_ten += sfan.dT;
			}

			// coils
			DBL _tex = _ten, _wex = _wmix;
			if (want >= 0)				// not if caller requested fan-only value
				// estimate possible effect of coils.
				// determines approx extreme tex for heat or cool coil, per want.
				// last computed captRat may be used - varies for coils with complex models. Changes no members.
				// w modelling VERY approximate (and we don't use it anyway). cncoils.cpp. 6-97.
				if (!estCoils( _ten, _wmix, _cmix, _frFanOn, _coilLockout, 	// inputs
							   want, 					// which coil to use: non-0 for cooling
							   _tex, _wex ) )			// outputs (ref args)
					return FALSE;				// eg if non-autosizable coil type, or coil has no size yet

			// drawthru fan
			DBL _tSen = _tex;
			if (sfan.fanTy==C_FANTYCH_DRAWTHRU)
			{
				sfan.fn_pute( _cmix, _tex, _frFanOn);
				_tSen += sfan.dT;
			}

			// adjust for supply duct conduction loss to outdoors
			_ts = _tSen * (1. - ahSOLoss) + Top.tDbOSh * ahSOLoss;	// sets ref arg
			//_ws = _wex * (1. - ahSOLoss) + Top.wOSh * ahSOLoss;  	if want to return _ws
		}

	}										// repeat from _tr2 calc to here
	while ( sfan.motPos==C_MOTPOSCH_INRETURN					// if supply fan motor in return air stream
			&&  _cr1 && fabs((sfan.qAround - sfanQAroundWas)/_cr1) > ABOUT0	// and resulting dT changed,
			&&  ++l <= 3 );							// up to 3 times.

	// restore fan members, cuz fans were pute'd here for wrong conditions
	sfan.cMx = sfanCMx;
	sfan.c = sfanC;
	sfan.t = sfanT;
	sfan.frOn = sfanFrOn;
	sfan.p = sfanP;
	sfan.q = sfanQ;
	sfan.dT = sfanDT;
	sfan.qAround = sfanQAround;
	rfan.cMx = rfanCMx;
	rfan.c = rfanC;
	rfan.t = rfanT;
	rfan.frOn = rfanFrOn;
	rfan.p = rfanP;
	rfan.q = rfanQ;
	rfan.dT = rfanDT;
	rfan.qAround = rfanQAround;

	return TRUE;
}			// AH::tsPoss
#endif // TSPOSS
//-----------------------------------------------------------------------------------------------------------------------------
#ifndef TSPOSS	// undef'd at top file if tsPoss not needed. Also change decl(s) in cnrecs.def.

DBL AH::tsfo(  			// detailed computation of fan-only supply temp

	DBL _tr, /*DBL _wr,*/ DBL _cr  	// use trNx, wrNx, crNx   or   tr, wr, cr   modified as desired.
	/*, BOO ecoLow*/ )

// other runtime inputs used include sfan.cMx, fcc, .

// returns fan-only supply temp for given inputs.

// for wzczSp special case; getTsMnFo.
// if kept, otta share code with regular computation --> put the -Nx and regular variables in a substruct block? 5-2-95.
{
	// save fan members -- some are used elsewhere, all might be probed for reports.
	// instead add mbr fcn that returns dT without altering members? 5-95.
	float  sfsvCMx = sfan.cMx,  sfsvC = sfan.c,  sfsvT = sfan.t,    sfsvFrOn = sfan.frOn,
		   sfsvP = sfan.p,      sfsvQ = sfan.q,  sfsvDT = sfan.dT,  sfsvQAround = sfan.qAround;
	float  rfsvCMx = rfan.cMx,  rfsvC = rfan.c,  rfsvT = rfan.t,    rfsvFrOn = rfan.frOn,
		   rfsvP = rfan.p,      rfsvQ = rfan.q,  rfsvDT = rfan.dT,  rfsvQAround = rfan.qAround;


// return leaks/losses/fan heat like puteRa()
	DBL _tr1=1, _tr2, /*_wr1,*/ _cr1, _frFanOn=1.;		// sets these using _tr, _wr, _cr, fcc, .
	{
		// init leakage. story in puteRa.
#ifdef AUTOSIZE//7-95
		DBL leakFracOn = Top.tp_pass1A && asFlow  		// fan-on leakage as fraction of supply fan design flow
						 ?  0				// use 0 during autoSizing pass 1 part A: determining fan design flow
						 :  (ahROLeak + ahSOLeak)/2.;	// use average of user's supply & return leaks, for balance for now.
#else
x       DBL leakFracOn = (ahROLeak + ahSOLeak)/2.;   	// fan-on leakage as fraction of supply fan design flow
#endif
		leakCOn = sfan.vfDs * leakFracOn * Top.tp_airxOSh;	// fan-on leakage in heat cap units (conv cfm to Btu/hr-F @ tDbOSh)
		setToMin( leakCOn, sfan.cMx/* *.99? */);				// don't leak more than fan can blow even if airx @ fan << outdoors.

		// determine fan fraction on now: uses fan-on leakeage; needed to determine average leakage. 6-24-92.
		if (fcc)						// if fan cycles this hour (set in AH::begHour; else _frFanOn preset 1.)
			// set _frFanOn given _cr, sfan.cMx, uUseAr, leakCOn, ctu cMx's. see story in setFrFanOn().
		{
			// fan-on flow === max poss avg flow = smaller of ctl tu max flow, and supply fan's max flow with overrun less leakage.
			DBL _cMx = 0.;			// .. typical sensible input wd be sfan.cMx==ctu->cMxC==cMxH; not enforced.
			if ( uUseAr != uNONE			// unused ah can occur at startup or ?if tu requesting no air.. _cMx is 0.
					&&  uUseAr != uMn )			// min flow: ctu->cMn required 0 (elsewhere) with fanCycles. _cMx is 0.
			{
				TU* ctu = TuB.p + ahCtu;			// point ah's control terminal (ahCtu verified non-0 in AH::begHour)
				_cMx = 					// flow for frFanOn=1. this subhour for this ah/tu is smaller of tu, sfan
					min( uUseAr & (uStC|uMxC)   	// test whether ah is on cuz terminal called for cooling or heat
						 ? ctu->cMxC : ctu->cMxH,	// tu cool or heat max flow per current use (set by AH::vshNLims)
						 sfan.cMx			// supply fan max flow, incl overrun (FAN::pute sets).
						 - leakCOn );		// less fan-on leak-in (==leak-out) to get poss tu flow at max sfan flow
			}
			// next iteration fraction fan on === fraction possible power requested for subhour by terminal === _cr/_cMx.
#ifndef AUTOSIZE
x          _frFanOn = _cMx==0. ? 0.		// if no flow is possible, don't run fan, don't divide by 0 (_cr is 0)
x                              : _cr/_cMx;	// fraction on = fraction of ah poss output needed = requested flow / max flow
#else
			_frFanOn = _cMx==0. 			// if no flow is possible, don't divide by 0,
					   ?  _cr ? 1. : 0.	//   but run fan anyway if there is demand (possible at start of autoSize).
		   :  _cr/_cMx;		// fraction on = fraction of ah poss output needed = requested flow / max flow
			setToMin( _frFanOn, 1.0);			// if autoSizing tu's & not supply fan, flow allowed to exceed sfan.cMx. 7-95.
#endif
		}				// end of code like setFrFanOn

		// return duct leak and loss (loss tentatively done second)
		DBL leakC = leakCOn * _frFanOn;   		// average leakage, heat cap units
		_cr1 = _cr + leakC;    				// specified amount of air leaks INTO return duct (duct is under vacuum)
		if (_cr1 != 0.)					// if there is flow
		{
			//_wr1 = (_wr * _cr + Top.wOSh * leakC)/_cr1;		// hum rat after leakage: needed if economizer modelled
			_tr1 = (_tr * _cr + Top.tDbOSh * leakC)/_cr1; 	// temp after leakage
			_tr1 = (1. - ahROLoss) * _tr1 + ahROLoss * Top.tDbOSh;	// temp after conduction loss to outside
		}
		else if (ahROLoss != 0.)				// no flow (including no leak). If there is conductive loss,
			_tr1 = Top.tDbOSh;					/* set temp to outdoor for results 6-16-92 (known inconsistent:
   								   ahROLoss definition is "fraction of temp difference") */
		// else: no flow, no conduction: leave _tr1 unchanged.

	}

	DBL _tsFO;				// set in doOa-like code below, needed outside of following loop.
	float sfanQAroundWas;		// saves value for loop endtest
	SI l = 0;				// repetition count so can limit
	do					// repeated if sfan.qAround changes. End of loop near end of fcn.
	{
		sfanQAroundWas = sfan.qAround;	// save to compare after sfan.pute way below.

		{
			// code like puteRa() continues...

			// return fan heat, and return or relief fan motor heat if in air stream
			DBL _dtRfan;
			if (rfan.fanTy==C_FANTYCH_RETURN)		// if have return fan (relief fan done after eco, when flow known)
			{
				rfan.fn_pute( _cr1, _tr1, _frFanOn);		// compute fan cMx, p, q, dT, etc for flow and temp. cnfan.cpp.
				_dtRfan = rfan.dT;				// temp change to return air due to return fan work and motor heat
			}
			else						// NONE or C_FANTYCH_RELIEF: leave 0: relief fan heats exhaust air only.
				_dtRfan = 0;    				// (relief fan heats exhausted air only)
			if (sfan.motPos==C_MOTPOSCH_INRETURN)		// if supply fan motor is in return air
				if (_cr1)					// don't divide by 0
					_dtRfan += sfan.qAround/_cr1;		// supply fan motor heat warms return air. qAvg/cAvg===qOn/cOn.
			_tr2 = _tr1 + _dtRfan;    			// next-iteration value for tr2.
			// pute4Fs() copies _tr2 to tr2; thus tr2 unchanged if exit now.
		}	// end of code like puteRa()


		// outside air like doOa()


		DBL _tmix, _cmix;		// sets _tmix, _cmix,  using _tr2, _cr, /*_wr1,*/ _frFanOn; sfan.cMx.
		{
			DBL cnv = Top.airX(_tr2);						// cfm to heat cap factor for return air
			DBL cr1On = _frFanOn != 0 ? _cr1/_frFanOn : _cr1==0. ? 0. : sfan.cMx;	// fan-on flow

			// min/max oa fractions
			DBL tem = oaMnFrac * oaVfDsMn;    		// current min outside air, cfm
			DBL _mnPo = 					// current min fraction outside air...
				oaMnCm==C_OAMNCH_VOL    			//  if oa spec'd by volume
				? (cr1On != 0.) ? tem * cnv / cr1On    	//    compute fraction of return flow
				: (tem != 0.) ? 1. : 0.				//    if no flow, any min cfm is 100% fraction
				: tem / sfan.vfDs;              	//  oa by fraction: compute fraction of supply fan cfm
			//DBL _mxPo = 1.;    				// max outside air -- not needed without economizer 5-5-95

			// oa/ra damper leakage
			if (!(Top.tp_pass1A && asFlow))				/* if autoSizing flow on pass 1 with open-ended models,
	 							   omit capacity-based leaks cuz sfan.vfDs is being determined.*/
			{
				tem = oaEcoTy==C_ECOTYCH_NONE ? oaVfDsMn : sfan.vfDs;   	// oaOaLeak is frac dsn flow if eco, else frac min flow
				DBL oaLeakC = tem * oaOaLeak * Top.tp_airxOSh; 		// outside air leak. convert cfm to hc at outside temp.
				DBL raLeakC = sfan.vfDs * oaRaLeak * cnv;			// return air leak, heat cap units
				if (oaLeakC + raLeakC)    					// if there is any leakage
					if (oaLeakC + raLeakC >= cr1On)				// if more leakage than flow (incl cr1On==0)
						_mnPo = /*_mxPo=*/ oaLeakC/(oaLeakC+raLeakC);     	// apportion flow in ratio of leakages
					else   							// leak less than flow: normal case; implies cr1On non-0
					{
						setToMax( _mnPo, oaLeakC/cr1On);   			// oa leakage gives min outside air flow
						//setToMin( _mxPo, 1. - raLeakC/cr1On);			// ra leakage gives max outside air flow
					}
			}

			// fraction outside air if no economizer or disabled
			setToMin( _mnPo, 1.);				// min outside air never > 100%
			//floor (_mxPo, _mnPo);  			// min overrules max (insurance: believe cannot occur)
			DBL _po = _mnPo;   				// init to min outside air flow

/* delete economizer code, cuz eco not used when coils off.
* to restore, restore commented _wr1 and _mxPo code above and add "DBL _wr" and "BOO ecoLow" args. 5-5-95.
*   // if economizer present and enabled, determine fraction outside air, like doEco().
*       // uses _mnPo, _mxPo, _po, _tr2, _wr1, ecoLow. Sets _po.
*       if (oaEcoTy != C_ECOTYCH_NONE)			// if have economizer
*       {  if (ahMode & ahCOOLBIT)				// if cooling enabled
*          {
*             if ( ISNUM(oaLimT) ? Top.tDbOSh >= oaLimT	// disable eco if outside air too warm
*                                : CHN(oaLimT)==C_RANC_RA
*                                  && Top.tDbOSh >= _tr2 )
*                ;
*             else
*             if ( ISNUM(oaLimE) ? Top.hOSh >= oaLimE		// disable eco if outside air to enthalpic
*                                : CHN(oaLimE)==C_RANC_RA
*                                  && Top.hOSh >= psyEnthalpy( _tr2, _wr1) )
*                ;
*             else				// eco enabled. compute fraction outside air
*             if (Top.tDbOSh >= _tr2)    	// if too hot out
*             {  if (ecoLow)      		// if want mixed air cooler than return air (else mnPo preset),
*                   _po = _mxPo;    		//  actual equipment uses use max oa even tho warmer
*             }
*             else if (ecoLow)   		// if lowest supply temp desired
*                _po = _mxPo;   		// use max outside air. Else mnPo preset.
*       }  }				// end of code like doEco()
*/
			// mixed air temp
			_tmix = (1. - _po) * _tr2 + _po * Top.tDbOSh;

			_cmix = _cr1;    			// flow out of eco is same as flow into it
		}				// end of code like doOa


// fan heats (and no coil outputs), like pute4Fs


		// fan heat: like dtMixEn or dtExSen -- fan position doesn't matter when no temp change thru coils.
		sfan.fn_pute( _cmix, _tmix, _frFanOn); 			// determine sfan.dT. fcn in cnfan.cpp.

		// air temp at sensor, after fan (and unused coils), is mixed air temp plus fan heat
		DBL _tSen = _tmix + sfan.dT;

		// adjust for supply duct conduction loss to outdoors
		_tsFO = _tSen * (1. - ahSOLoss) + Top.tDbOSh * ahSOLoss;

	}										// repeat from _tr2 calc to here
	while ( sfan.motPos==C_MOTPOSCH_INRETURN					// if supply fan motor in return air stream
			&&  _cr1 && fabs((sfan.qAround - sfanQAroundWas)/_cr1) > ABOUT0	// and resulting dT changed,
			&&  ++l <= 3 );							// up to 3 times.

	// restore fan members, cuz fans were pute'd here for wrong conditions
	sfan.cMx = sfsvCMx;
	sfan.c = sfsvC;
	sfan.t = sfsvT;
	sfan.frOn = sfsvFrOn;
	sfan.p = sfsvP;
	sfan.q = sfsvQ;
	sfan.dT = sfsvDT;
	sfan.qAround = sfsvQAround;
	rfan.cMx = rfsvCMx;
	rfan.c = rfsvC;
	rfan.t = rfsvT;
	rfan.frOn = rfsvFrOn;
	rfan.p = rfsvP;
	rfan.q = rfsvQ;
	rfan.dT = rfsvDT;
	rfan.qAround = rfsvQAround;

	return _tsFO;
}			// AH::tsfo
#endif // ifndef TSPOSS
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_SetMode()		// set ahMode per ts sp control method and supply fan schedule

{
	// callers 8-92: AH::estimate, AH::ahCompute
	AHMODE ahModeWas = ahMode;
	if (CHN(ahSch)==C_AHSCHVC_OFF)
		ahMode = ahOFF;					// if supply fan is off, ah is off
	else
	{
		if (isZNorZN2)					// if ah under zone control this hour, set ahMode
		{
			ahMode = TuB.p[ahCtu].wantMd;			// ahHEATING/-OFF/-FAN/-COOLING ahMode request set by TU::tuEstimate()
			//4-1-92: cncult5 no longer enforces ahCtu having a setpoint (in defaulted tu case);
			// add runtime check as appropriate. 8-92: believed done in setTsSp1.
		}
		else						// non-ZN, non-ZN2 ts sp or ts sp cm
			ahMode = ahON;   				// airHandler is on, ts sp further worked on later
	}

	if ((ahMode==ahOFF) != (ahModeWas==ahOFF))   	/* if airHandler stopped or started.
    							   Changes amoung ah- HEATING/COOLING/FAN/ON: don't care here. */
	{
		flagTus();					// set ztuCf for each served zone
		if (ahMode != ahOFF)				// if went on
			ahPtf++;					// say full ah computation required -- state of -Pr's unknown. 4-17-92.
	}
	return RCOK;
}			// AH::ah_SetMode
//-----------------------------------------------------------------------------------------------------------------------------
/* Notes re AH ZN and ZN2 supply temp control methods, Rob 5-95. Also at start of cncult5.cpp and app\cnah1.cpp.
 *
 * 1. ZN and ZN2 are needed to simulate single zone constant volume systems.
 *    VAV is still allowed but could be removed (says Bruce in fax 5-95) if hard-to-fix problems
 *    continue to be encountered. As of 5-95, VAV ZN/ZN2 believed to work, but accurate simulation
 *    has been hard to achieve, especially when min vol is neither 0 nor same as max. Much of the
 *    difficulty has been in setTsSp1 and wzczSp (which follow), in determining the setpoint
 *    which will make the zone temperature exactly right in low-load or "backwards" situations,
 *    when leaks/losses/extreme weather make the relation between flow and return temp strange.
 *    Cases with multiple zhx's at same sp (additional ah for zone, lh, etc) have not been
 *    much tested with ZN/ZN2 as of 5-95.
 *
 * 2. Additional terminals on zone-controlled AH are probably not disallowed but are untested and not needed --
 *    disallow when reason found -- then look for find-control-terminal code to simplify?.
 */
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::setTsSp1(				// get numeric supply temp setpoint or ts estimate per AH::ahTsSp

	BOO est,		// TRUE to estimate acheivable ts (and suppresses some error messages),
	// FALSE to return ts setpoint (tsSp1 value)
	DBL &tsOut )		// receives result: supply temp if est, else tsSp1.

// this function does control methods C_TSCMNC_RA (return air), ZN/ZN2 (zone), WZ (warmest zone), and CZ (coolest zone),
//   and applies min and max ts inputs to all control methods.
// if no demand: if ZN, turns off airHandler, else sets tsSp1 to tr2.

// returns TRUE unless airHandler has been turned off.
{
	// callers 4-95: AH::estimate (2 calls), AH::iter4Fs.
	RC rc=RCOK;
	BOO cooling = FALSE;			// assume heating (used in ZN/ZN2 and WZ/CZ cases else NOT VALID)
	TU *ctu = NULL;				// for ZN/ZN2 ts sp cm's, points to control terminal; remains NULL for WZ/CZ

// establish ts limits now so can terminate upon reaching a limit

	DBL llim = ahTsMn, ulim = ahTsMx;		// start with user's limits (defaulted to wide range if none given)

// avoid thrash when estimating by keeping ts estimate limits within estimated coil limits as set by doCoils

	/* This makes it possible to estimate ts here (getting updated ts when load changes and not overloaded),
	   then do terminals, then do ahCompute. */
	if (est)						// CAUTION: 'cooling' not valid here; don't know if heating or cooling
	{
		// adjustment from ts sp to ts estimate: add supply duct loss: done at exit; compute tSen (temp at sensor) limits here.

		if ( CHN(ahcc.sched) != C_OFFAVAILVC_AVAIL		// if no cooling coil or not scheduled avaiable
				|| !(ahMode & ahCOOLBIT)  				// if cooling not enabled (ZN/ZN2 ts sp control)
				|| coilLockout )					// if cooling coil (was) locked out by non-integrated economizer
		{
			DBL llim1 = min( tr2Nx, (DBL)Top.tDbOSh)   		// supply air can't get cooler than cooler of return air, outdoors
						+ sfan.dT;				//   plus supply fan heat (whether drawthru or blowthru)
			setToMax( llim, min( tSen, llim1) );    			// don't let tSen go lower, but let it stay if already lower.
		}
		else if (tPossC)						// if cooling (low) tex1 limit has been set
		{
			DBL llim2 = tPossC + dtExSen;				// estimate tSen correspoding to tex1 value, using prior fan heat
			setToMax( llim, min( tSen, llim2) );			// don't let estimated tSen go lower unless already was lower
		}
		if ( CHN(ahhc.sched) != C_OFFAVAILVC_AVAIL		// if no heating coil or not scheduled avaiable
				|| !(ahMode & ahHEATBIT) )  				// if heating not enabled (ZN/ZN2 ts sp control)
		{
#if 0 // worked for years, til BAW used very small CAir with condensation in humtst2c, 5-97
x          DBL ulim1 = tr2Nx + sfan.dT;     			// supply air can't get warmer than return air + fan heat
x          setToMin( ulim, max( tSen, ulim1) );   			// don't let tSen go hier, but let it stay if already hier.
#elif 0 // 5-97. Nope, still prevents ztuMode from converging.
x          DBL ulim1 = sfan.dT 					// supply air can't get warmer than fan heat dT + return air temp
x                      + ( cr1Nx ? tr2Nx				// tr2Nx is good if there is flow (worked historically 5-97)
x                                : max( tr2, tr2Nx) );		/* but if no flow, tr2Nx may be outdoor temp (see AH::puteRa),
x								   which may be too cold, so ?? use larger of prior iteration,
x								   upcoming iteration return air temp 5-97. */
x          setToMin( ulim, max( tSen, ulim1) );   			// don't let tSen go hier, but let it stay if already hier.
#else // try 5-97. Good.
			DBL ulim1 = tr2Nx + sfan.dT;     			// supply air can't get warmer than return air + fan heat
			if (cr1Nx)						/* if no flow b4, tr2Nx can be tDbOSh (can be far too cold),
								   and tSen can be too cold on mode change, so don't limit, 5-97*/
				setToMin( ulim, max( tSen, ulim1) );   		// don't let tSen go hier, but let it stay if already hier.
			/* TESTED making the above setToMin( conditional made a run (Bruce's humtst2c)
			   complete that had failed to converge in ztuMode in warmup, and added just a few
			   iterations to only 4 of my many tests. Rob 5-97. */
#endif
		}
		else if (tPossH)
		{
			DBL ulim2 = tPossH + dtExSen;
			setToMin( ulim, max( tSen, ulim2) );
		}
	}

// determine tTsSp: ts setpoint, or for estimating ts by adding duct loss.

	DBL tTsSp;     				// temp for new supply temp setpoint (or tSens estimate if est'ing)
	if (!ISNCHOICE(ahTsSp))			// if ahTsSp is numeric
		tTsSp = ahTsSp;				// user has specified setpoint (possibly a runtime expr, already eval'd if here)
	else 					// else is a setpoint control method choice value
	{
		// init supply temp sp to approx coil-off ts, for estimating no-demand or wrong-sign-demand WZ/CZ/ZN/2 cases, or RA error.
		//  Re-init or refined in some cases below.
		tTsSp = (1. - mnPo) * tr2Nx + mnPo * Top.tDbOSh	// prior return air temp (tr2Nx), w min outside air (like AH::doOa)
				+ dtMixEn + dtExSen;			// plus prior fan heat (one of dt's is 0)
		// duct loss (ahSOLoss) not pertinent here cuz it is after ts sensor.

		// set tTsSp per control method
		switch (CHN(ahTsSp))
		{
		default:
			rer( PABT, (char *)MH_R1279,	// "Internal error: Airhandler '%s': unrecognized ts sp control method 0x%lx"
				 name, VD ahTsSp);

		case C_TSCMNC_RA:			// return air supply temp setpoint control

#if 0		// No, I think we should just let RA operate normally during sizing - no ah ts changes. Rob 7-2-95.
*     // can't use RA with autoSize part A model as don't know whether to use Hi or Lo supply temp.
*     if (asFlow && Top.tp_pass1A)				// must check at run time cuz ahTsSp is hourly variable.
*        rc |= rer( "airHandler '%s': Cannot use \"ahTsSp = RA\"\n"
*                   "    when autoSizing supply fan or any connected terminal", name);	// NUMS
#endif

			// limits of return air range over which ts sp varies must be given. fatal 6-13-92
			if (!(sstat[AH_AHTSRAMN] & FsSET))
				rc |= rer(ABT,(char *)MH_R1280,name);		// "airHandler '%s': ahTsSp is RA but no ahTsRaMn has been given"
			if (!(sstat[AH_AHTSRAMX] & FsSET))
				rc |= rer(ABT,(char *)MH_R1281,name);		// "airHandler '%s': ahTsSp is RA but no ahTsRaMx has been given"

			// ts sp limits, otherwise optional, must also be given: these specify range over which ts sp varies
			if (!(sstat[AH_AHTSMN] & FsSET))
				rc |= rer( ABT, (char *)MH_R1282, name);   	// "airHandler '%s': ahTsSp is RA but no ahTsMn has been given"
			if (!(sstat[AH_AHTSMX] & FsSET))
				rc |= rer( ABT, (char *)MH_R1283, name);   	// "airHandler '%s': ahTsSp is RA but no ahTsMx has been given"

			if (!rc)							// if error (missing input), let ts sp be default set above
				tTsSp = ahTsMx						// when tr2Nx is ahTsRaMn, ts sp is Mx
						+  (ahTsMn - ahTsMx)/(ahTsRaMx - ahTsRaMn)  	// slope
						* (tr2Nx - ahTsRaMn)/(ahTsRaMx - ahTsRaMn);	// fraction 0 for tr2Nx==ahTsRaMn, 1 for Mx.
			break;							// ts is limited to range  ahTsMn..ahTsMx  below.

		case C_TSCMNC_ZN2:		// the other ZN method 8-92: WZ or CZ or fan only -- fan runs even when coils disabled
			if (!(ahMode & (ahHEATBIT|ahCOOLBIT)))	// if neither heat nor cool desired now (ahFAN)
				break;					// let tsSp1 remain tr2Nx
			//else fall thru

		case C_TSCMNC_ZN:  		// ZN method: WZ or CZ or off according to what zone of terminal ahCtu requires
			// tuEstimate thence ah_SetMode have already set ahMode to ahHEATING or ahCOOLING.

			if (ahCtu)					// no control terminal possible if ahTsSp is runtime expr
				ctu = &TuB.p[ahCtu];			// point to specified control terminal for ZN/ZN2 methods
			else   					// fatal error 6-13-92
				rer( ABT, (char *)MH_R1284, name);	// "ahTsSp for airHandler '%s' is ZN or ZN2 but no ahCtu has been given"
			if (ahMode & ahCOOLBIT)			// heating & cooling exclusive here
				cooling = TRUE;				// if cooling, say so (low setpoint); leave FALSE for heating (hi sp).
			/* limitation: if ah has TWO terminals in zone (nothing yet disallows this if not fcc?),
			   nothing guarantees that following code will find ahCtu, not the other one. 4-92. */

			if (fcc)				// fanCyles with ZN or ZN2: setpoint is ts limit; shutdown if no flow.
			{
				// more fcc code: AH::begHour; AH::setFrFanOn
				tTsSp = cooling ? ahTsMn : ahTsMx;	// use ts limits as setpoints to get coils to run near full power, 6-15-92.
				//  note capacity limits applied above after 1st time, if estimating.
				if (TuB.p[ahCtu].cv==0.)		// if control terminal wants no flow, shut down airHandler
				{
					//  (expected here only if ah's interact or input or ausz made 0-flow tu).
					if (!est)			// if not estimating: interferes with turning on, cuz cMx is still 0 on 1st call
					{
						ahMode = ahOFF;   		// turn airHandler off
						ctu->wantMd = ahOFF;		// turn off terminal's request to be sure ah stays off til needed. needed?
						// ahFAN for ZN2: fanCycles with ZN2 disallowed in AH::begHour (contradictory).
						flagTus();			// redo terminals in zones served
						return FALSE;			// tell caller not to continue ah execution
					}
				}
				// require ahTsMn/Mx with ZN and fancycles, since used as setpoints. .460, 5-95.
				if (!(sstat[AH_AHTSMN] & FsSET))
					rer( ABT, (char *)MH_R1295, name);   	/* "airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)"
                   						   "    but no ahTsMn has been given." */
				if (!(sstat[AH_AHTSMX] & FsSET))
					rer( ABT, (char *)MH_R1296, name);   	/* "airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)"
                   						   "    but no ahTsMx has been given." */
#ifdef ZNJUST	// (un)def'd at top file, 6-97.
				/* for ZN/ZN2 autosizing, use only ts that coil can now produce or zone needs, to get large frFanOn.
				   Intended to combat tendency to size coil & fan too large with frFanOn unnec small at peak load,
				   especially eg when fan oversize for heat has already been forced by cooling requirements. 6-16-97. */
				if (Top.tp_autoSizing)
				{
					DBL canGet = tTsSp;				// init: precaution.
					if ( !tsPoss( trNx, wrNx, crNx, 		// calc approx ts current coil size can produce. above.
								  cooling, 			//   0 max heat ts, 1 max cool ts.
								  canGet ) )			//   receives result
						break;					// certain not-covered cases leave tTsSp unchanged
					DBL loadNeeds = cooling ? ulim : llim;	// init to opposite extreme
					FLOAT cznCSink;
					wzczSp( est, cooling, FALSE, llim, ulim,  	 // calc ts needed to meet load. below.
							&TuB.p[ahCtu], loadNeeds, cznCSink );
					FLOAT m = est ? 0 : 2.;			// .1 ah/tu nonconvergence (BUG0090); explore values more.
					// 1. ditto warning; 2. good; 4. not as good as 2. 6-16-97.
					if (cooling)
						setToMax( tTsSp, min( canGet-m, loadNeeds));	// limit to lower of present coil, load req't
					else
						setToMin( tTsSp, max( canGet+m, loadNeeds));	// .. higher ..
				}
#endif
				break;				// done for fcc
			}
			// not fcc: fall thru

		case C_TSCMNC_CZ:   		// coldest zone: ts sp hi enuf for control zone needing most heat (implies heating)
		case C_TSCMNC_WZ:   		// warmest zone: ts sp lo enuf for control zone needing most cold (implies cooling)
			// Method: find the tSen for which the largest control zone VAV flow will be 90% of max.

			FLOAT cznC;   					// receives ctrl zn tu flow, re ah shutdown under ZN/ZN2 cm's
			if (CHN(ahTsSp)==C_TSCMNC_WZ)  cooling = TRUE;   	// for WZ: say cooling; set above for ZN/ZN2; else is FALSE (CZ)

			// for autoSizing part A const temp model, use design cooling or heating supply temp

			if (asFlow && Top.tp_pass1A)			// if flow-sizing this ah('s sfan) or any connected tu
			{
				// note if (fcc) already exited with constant ahTsMn/Mx.
				tTsSp = cooling ? ahTsDsC : ahTsDsH;	// use constant supply temp.
				break;					// don't call wzczSp: uses cMxH/C which may not be determined yet.
			}

			// initialize tTsSp (value set above used for WZ/CZ est)

			if (est)					// if estimating supply temperature, not determining tSens setpoint
			{
				if (isZNorZN2)				// if C_TSCMNC_ZN or ZN2 (flag set at start hour)
				{
					// get better coil-off ts estimate, assuming control terminal @ min flow, for CONST VOL cases
					/* (for vav, don't know what flow makes min ts & accuracy less critical cuz tu can adjust flow,
					   so stay with preset value based on previous flow & fan heats) */
					if (ctu->cMn > .99 * ( (ctu->useAr & (uMxH|uStH))		// if min flow > 99% of applicable max flow
										   ? ctu->cMxH : ctu->cMxC ) )		// .. (test sloppy re uMn (expected), uSo (not))
					{
						//tsMnFoOk = FALSE;    			to force fresh calculation even if just calc'd eg from ztuMode
						// TESTED above line had NO EFFECT on 48 const-vol ZN/ZN2 test runs, 5-10-95.
						tTsSp = getTsMnFo(FALSE);						// do much of ah calc for ctu min flow
						tTsSp = (tTsSp - ahSOLoss*Top.tDbOSh)/(1. - min(ahSOLoss,.9f));	// reverse-calc supply duct loss
					}
				}
			}
			else					// not estimating. init setpoint to hi for cool, lo for heat,
				tTsSp = cooling ? ulim : llim;		// ... so ah doesn't waste energy matching any estimating inaccuracies.

			wzczSp( est, cooling, FALSE, llim, ulim, ctu, tTsSp, cznC);	// determine required setpoint tTsSp. next.

			// wz/cz/zn/zn2... if no demand under ZN or ZN2 ctrl method, go to off or fanOnly. Expected here only when ah's interact.

			if (cznC==0.)  		/* if no control zone wanted any flow (per ztuMode): if there is flow, ah may be heating
       					   or cooling zone via supply duct gain/loss even if coils not in use, so leave on.
					   OBSERVED 5-16-92, file T9.  Probably makes next test redundant: */
#if 0 // believe not needed, or if is needed, test for tTsSp value saved b4 wzczSp call just above. 7-95.
x			// removing made NO DIFFERENCE in all my tests, rob 7-12-95.
x			if (tTsSp==tr2Nx) 	// if no control zone wanted air warmer/cooler than return air
#endif
					if (!est)   				// if not est'ing: interferes with turn-on, cuz cMx is still 0 on 1st call
						if (CHN(ahTsSp)==C_TSCMNC_ZN)	// if ZN control method (if not ZN or ZN2, ah stays on til scheduled off)
						{
							ahMode = ahOFF;   		// turn airHandler off
							ctu->wantMd = ahOFF;		// turn off terminal's request to be sure ah stays off til needed. needed?
							flagTus();			// redo terminals in zone(s) served
							return FALSE;			// tell caller not to continue ah execution
						}
						else if (CHN(ahTsSp)==C_TSCMNC_ZN2)	// if ZN2 control method
						{
							ahMode = ahFAN;			// turn airHandler coils off, leave fan on
							ctu->wantMd = ahFAN;   		// set request same to be sure coils stay off til needed. needed?
							flagTus();   			// redo terminals in zone(s) served
						}   					// fall thru intended to return tr2Nx... in tsOut.
			break;
		}			// switch (CHN(ahTsSp))  ts sp control method switch (cases set tTsSp)
	}

// apply limits, store

	setToMax( tTsSp, llim);
	setToMin( tTsSp, ulim);
	if (est)							// if estimating ts (not determining ts sp), add loss after sensor
		tTsSp = tTsSp * (1. - ahSOLoss) + Top.tDbOSh * ahSOLoss;	/* like tTsSp += dtSenS, but estimate duct loss after sensor
       								   for actual tTsSp and new outdoor temp. */
	tsOut = tTsSp;						// store result. tolerance check on this store removed 10-31-95.

// conditionally determine ts for full flow re AHP suppl heat stability

	/* used in cncoil.cpp re regulating air source heat pump supplemental resistance heat, to just meet load when frFanOn is 1.0.
	   1. suppl heat is only used at frFanOn = 1.0, so that full compr output is used first.
	   2. thus must use just enuf suppl heat, or ts rises, frFanOn falls, suppl heat cuts out, oscillation, non-convergence.*/

	if ( ahhc.coilTy==C_COILTYCH_AHP	// only needed for AHP heat coils. don't compute if unneeded: slow.
			&&  !cooling			// not needed if cooling.
			&&  !est				// not needed if estimating: iter4Fs calls setTsSp1 b4 doCoils. 7-95.
			&&  fcc				// not needed if fan not cycling this subhour -- frFanOn stays 1.0 anyway
			&&  !(asFlow && Top.tp_pass1A) )	// not if now flow-sizing: don't call wzczSp; coil doesn't use tsFullFlow in part A. 6-95.
	{
		tsFullFlow = tr2Nx;		/* approx no-demand ts, from about 1992.
					   No-demand ts estimate (tTsSp) much improved above in 1995; believe
					   no improvement needed here cuz tsFullFlow is used only in high-demand cases,
					   when heat pump compressor can't meet load w/o supplemental heat. 7-95. */
		FLOAT cSink;
		wzczSp( est, cooling, TRUE, llim, ulim, NULL, tsFullFlow, cSink);  	// determine ts for full flow. next.

		/* what if not WZ/CZ/ZN/ZN2? Unlikely but not disallowed with fcc?
		   Should tsFullFlow be tsOut not tr2Nx to let suppl heat work?  Pursue if case found. 7-95. */
	}

	return TRUE;				// FALSE return above
}				// AH::setTsSp1
//-----------------------------------------------------------------------------------------------------------------------------
void AH::wzczSp( 			// determine required supply temp for warmest zone/coolest zone/ZN/ZN2 ah ts cm's

	BOO est,		// TRUE if estimating
	BOO cooling, 	// TRUE if cooling, FALSE if heating
	BOO maxIt,		// TRUE for supply temp for full flows tu->cMxH/C (for tsFullFlow);
					// normally FALSE for 90% approach from cMn to cMx.
	DBL llim, DBL ulim,	// ts limits: ahTsMn/Mx, reduced per coil capacity etc. Terminate here when tTsSp reaches either.
	TU *ctu,		// NULL for WZ/CZ, control terminal for ZN/ZN2 (used re no control zone error message)
	DBL &tTsSp,		// caller inits based on tr; on return, contains required sp if any requirement found.
	FLOAT &cznC ) 	// receives total flow of control zones: 0 indicates ah may be shut down

// internal function for setTsSp1 (just above). 2+ calls therein. Code removed from setTsSp1 10-25-92.
{
	// Method: find the tSen for which the largest control zone relative VAV flow will be 90% of max.

	BOO foundCzn = FALSE;
	cznC = 0;							// init ctrl zn tu flow, re ah shutdown under ZN/ZN2 cm's
	for (ZNR *czp = nullptr;  nxTsCzn(czp); )			// loop over ahCtu zone or wz/cz control zones for airHandler
	{

		/* wz/cz/zn/zn2.  To get setpoint and terminal, search zone mdSeq for zhx for this zone and airHandler.
		   Start from current zone mode (for speed, and cuz multiple terminals for ah and zone not disallowed),
		   but search all in case zone is in wrong mode (eg when estimating (may still miss: old mdSeq may be wrong)). */

		TI xi = 0;
		ZHX *x = nullptr;
		TI spMd = czp->md;
		if (czp->nMd)						// insurance: if zone currently has any hvac modes
			for ( ; ; )      					// mode search loop, 2 endtests in loop
			{
				xi = czp->mdSeq[spMd];				// active zhx subscript or MDS_FLOAT
				//test if found
				if (xi != MDS_FLOAT)   				// if a float mode, keep looking
				{
					x = ZhxB.p + xi;    				// point active zhx for mode
					if (x->ai==ss)					// if zhx on wrong airHandler, keep looking
						if (x->zhxTy==(cooling ? ArStC : ArStH))	// if found heat when want cooling, etc, keep looking
							break;   					// found it, xi and x set.
				}
				//loop increment: next mode up (toward warmer modes) for cooling, down for heat, with wrap-around
				if (cooling)
				{
					if (++spMd >= czp->nMd)  spMd = 0;
				}
				else
				{
					if (spMd-- <= 0)         spMd = czp->nMd - 1;
				}
				if (spMd==czp->md)
				{
					xi = 0;     // done if back to initial mode. xi=0: not found.
					break;
				}
			}
		if (!xi)						// if no zhx found, disregard this control zone
			continue;    					// could happen if zone not currently being conditioned?
		foundCzn = TRUE;					// prevent "no control zone found" message
		// may yet be no demand if zone already warm/cool enuf.
		TU *tu = TuB.p + x->ui;				// point terminal for zone for airHandler
		cznC += tu->cv;					// accumulate control zone flow according to ztuCompute, 5-16-92

		// wz/cz/zn/zn2... get this ctrl zone's desired supply temp according to found zhx's terminal's current use (thus zone's mode)

		float approach = .9f;		// fraction of way from min to max flow at which to determine supply temp needed.
		float bsF = -.9f;		// binary search incr for reDo loop. -.9 searches 0 < approach <= .9; 0 done separately.
		DBL bestTtTsSp = 0;		// best ttTsSp yet found in reDo loop
		float bestScore = -9999;		// best ttTsSp's "score"
		float lastScore = -9999;		// previous iteration's "score"
		BOO reDo, reDone = FALSE;
		do			// loop that may repeat to try a different approach in special case. Ends at "while (reDo);".
		{
			reDo = FALSE;		// set TRUE within loop if repetition desired

			switch (tu->useAr)
			{
			default:
#if 1	   // 0 observed 8-92. SOMETIME investigate whether we can assume what useAr will become and go thru setpoint/max case
				//			     for speed (otherwise no est setpoint --> max's out, then iterate's to setpoint mode.
				//			     (file SUTTER\GROFIX01.INP reproduces the case.)
				if (!est || tu->useAr != uNONE)		// when turning on ah, when estimating, 0 is expected
#endif
					rer( PWRN,
						 (char *)MH_R1285, 		//"Unexpected use 0x%x for terminal [%d] for control zone [%d] for ah [%d]"
						 (UI)tu->useAr, (INT)tu->ss, (INT)czp->ss, (INT)ss );
				continue;

				/* terminal at min flow: although some other tu usually rules, consider this tu's needed supply temp
				     in case only, or all, tus at min flow.  Else, ts can drop back to return temp & iterate up again:
				     a) wastes an iteration b) const vol tuVfMn==tuVfMx cases may loop tu & ah's forever (observed 8-20-92).
				   Note tu IS under set temp control (not set output) if here, cuz of how found above. */
			case uMn:    // fall thru

				// terminal under setpoint control: compute needed supply temp and apply to tTsSp.
				// terminal at max flow: a float mode or another terminal's setpoint mode:
				//    determine most desired ts sp: that which would let this ah bring the zone to setpoint thru this tu.
			case uStH:
			case uStC:
			case uMxH:
			case uMxC:

				// get a and b for zone excluding this terminal for zone mode where this terminal is active
				DBL aZn, bZn, snk;
				if ( est    					// if estimating, do ztuAbs to get a,b poss some newer ts's
						||  tu->useAr & (uMxH|uMxC|uMn) )    		/* if pegged, always do ztuAbs to get desired a, b, as may be 0
                   						   or may reflect too much other tu flow for different mode */
					czp->ztuAbs( x->mda, 0, aZn, bZn, snk,snk,snk);	/* get a, b contributions of OTHER terminals for mode where
                   						   this terminal active (x->mda===xi, right?). */
				else						// active terminal; not estimating
				{
					aZn = czp->zn_aqHvO;
					bZn = czp->zn_bHvO;		// use values from ztuCompute for speed
				}
				aZn += czp->zn_aqLdSh;
				bZn += czp->zn_bLdSh;		// add load components to complete a and b.  If estimating,
                     					// this gets newer values than reflected in tu->aqO, bO.
				// get setpoint temp
				DBL sp = ZhxB.p[czp->mdSeq[spMd]].sp;		// fetch setpoint from zhx found

				// what TERMINAL is doing if clearly different (eg heating when ah cools to counter fan heat).
#if 0 // (added 4-22-95; 5-95 I'm not entirely clear this is appropriate... undone 5-6-96)
*   BOO tuCooling = cooling;			// in unclear cases assume tu doing same thing as ah
*   xi = czp->mdSeq[czp->md];   			// xi for CURRENT zone mode (vs mode found above for ah sp)
*   if (xi != MDS_FLOAT)				// if not a float mode (float xi is -1, not a ZHX subscript)
*   {  x = ZhxB.p + xi;				// point zhx for this zone mode
*      if (x->ui==tu->ss)				// if zhx uses the same terminal as we are considering
*         if (x->zhxTy==ArStC)			// if terminal setpoint cools in zone's mode
*            tuCooling = TRUE;			// then use tu's cooling flow, etc, regardless of ah
*         else if (x->zhxTy==ArStH)
*            tuCooling = FALSE;
*	}
#endif

			// get zone flow czM at which to determine ts to meet zone load. If-outs here removed 5-2-95; see cnah.cp2.
#if 0
x    DBL cMx = tuCooling ? tu->cMxC : tu->cMxH;	// applicable tu max flow, at vav box
#else//5-6-95
			DBL cMx = cooling ? tu->cMxC : tu->cMxH;	// applicable tu max flow, at vav box
#endif
			//
			// limit flow as fanF will for 1 terminal (for multiple terminals, iterating will finish applying fanF)
			if (!(Top.tp_sizing && asFlow))		// don't limit per sfan.cMx when autoSizing flow,
												// particularly if autoSizing tu's but not sfan. 7-95.
				{
					DBL fMx = sfan.cMx * Top.hiTol;   		// central fan capacity plus tolerance
					fMx -= leakCOnNx * frFanOnNx;   		// subtract air that leaks into return duct 4-22-95
					setToMax( fMx, .001*sfan.cMx);			// insurance: keep value positive (leakCOnNx newer than cMx).
					setToMin( cMx, fMx);   				// anticipate fanF some, or completely for 1-terminal case.
				}
				// flow at which to determine supply temp: use fraction 'approach', eg 90%, of way from min to max.
				// must work if max==min (user spec'd const vol), or cvMx < min (user, sfan.cMx, or fanF reduced max)
				DBL cM = maxIt     				// flow at which to determine ts
						 ?  cMx						// if determining tsFullFlow, use full flow
						 :  approach * cMx + (1.f - approach) * tu->cMn;	// target flow is 'approach' fraction of interval

#if 0//4-30-95. Undone. Worked as intended, but didn't like result: continuing use of coil in float mode at min flow.
x     /* instead added code in ztuMode to use sp mode even with wrong-sign dT,
x 	   and code below to determine ts for lower flow if nec to keep dT sign wrong. */
x
x              // BUT determine ts setpoint for MINIMUM flow when:                         4-30-95 for eg T155d hour 22.
x              //   *  zone's float temperature (a/b) already above heat sp and/or below cool sp,
x              //   *  and there is a zone float mode between the heating and cooling modes.
x              //   Coil use is required in these cases to partly counter fan heat/leaks/losses when fan-only ts
x              //     at non-0 min flow overheats/cools zone. Want min energy ts to keep zone between the heat/cool
x              //     setpoints, at min flow, in the float mode. Without min flow ts, unnec heat/cool is modelled.
x              // If min flow is 0, no coil use required -- zone will float between sp's at 0 flow.
x              // If there is no float zone, determine ts as usual. Zone will heat while ah cools or vice versa.
x              //
x              if ( (cooling ? a/b < sp : a/b > sp)			// if, without this ah, zone doesn't need heat/cool
x               &&  czp->mdSeq[spMd + (cooling ? -1 : 1)]==MDS_FLOAT )	// & zone uses a float mode (min flow) when sp is met
x                 cM = tu->cMn;    					/* then determine supply temp for min flow,
x		   							   or use no coil if min flow is 0 -- checked below. */
x              // convert flow at vav box to flow at zone (after leaks)
x              DBL czM = tu->znC(cM);
x
x              // if flow 0, no heat/cool needed for this control terminal
x              if (czM==0.)  			// occurs when float temp meets sp and min flow 0,
x                 continue;			// and protects against /0 during ah turn-on or in other (unexpected) cases.
#else
				//
				// convert flow at vav box to flow at zone (after leaks)
				DBL czM = tu->znC(cM);

				// if flow 0, no heat/cool needed for this control terminal
				if (czM==0.)  				// (unexpected except maybe during ah turn-on)
					continue;				// don't divide by 0!
#endif
				//#ifdef DEBUG2
				//              dblSpare1 = cM;		// to watch in debugger 4-95
				//#endif

				// ts needed to hold tz at setpoint at flow czM:
				//   tz = (a + q)/b  --->  q = sp * b - a.
				//   substitute  q = c*(ts - sp)  and  c = czM   yielding   ts = sp + (sp * b - a)/czM.
				//   adjust for air vsh change now so won't iterate:
				//     multiply czM by airxTsNew/airxTs  or  Top.airXK/((ts + 459.67) * airxTs)  yeilding
				//          ts = sp + (sp * b - a) * airxTs * (ts + 459.67) / (Top.airXK * czM)
				//     gather knowns as j = (sp * b - a) * airxTs / (Top.airXK * czM)  yeilding
				//          ts = sp + j * (ts + 459.67)
				//     solve
				//          ts = (sp + 459.67*j)/(1-j)
				//
				//   NB vsh falls with temp --> req'd ts can be infinite. In that case j >= 1.
				//	TESTED 4-20-92: this vsh adj often saves an iteration.
				//
				DBL q = sp * bZn - aZn;			// heat required to zone, also used below
				DBL j = q * airxTs / (Top.airXK * czM); 	// subexpression used twice
				if (j >= 1.)					/* if infinite ts needed,
             							   fallthru yields wrong sign due to /(1-j) */
				{
					tTsSp = ulim;				// so go right to high ts limit
					/* TESTED 1-7-94: replacing  "tTsSp = cooling ? llim : ulim;"  with the above
					                       fixed ts going 0 (llim) & overcooling in mode hunt at 80-->300 cooling sp change
					                       while changing none of my rob\ tests (then fixed in cnztu.cpp) */
					goto breakBreak;				// skip rest of zones
				}
				DBL ttTsSp = (sp + 459.67*j)/(1. - j);		// ok. zone's req'd wz/cz/zn/zn2 ts sp, adj for expected vsh chg.

				// check for problem with excessive ts due to losses when float temp meets setpoint but lossy-ah min flow does not.
				//  case (described for heating)
				//  *  zone's float temperature (a/b) above zone setpoint
				//  *  tu has a min flow
				//  *  fan-only ts overcools zone at min flow, so heat coil is used to counter SOME OF leak/loss.
				//  *  tu "backwards heats": uses (only) enuf supply air below sp to lower zone temp to heat sp
				//  *  required flow INCREASES as ts rises toward zone sp.
				//  *  ideal modeling (not fully implemented) would run fan up to normal flow b4 using coil:
				//     use 90% appr flow if poss; if overheats w/o coil, reduce flow til load just met.
				//  problem case:
				//  *  after determining heat ts sp for 90% approach, ts comes out above sp due to fanHeat/leaks/losses
				//     and can't "cool" to heating sp in zone heat mode at any flow in range.
				//  *  positive feedback likely: more flow, more fan heat, hier ts if coil off, more flow ... til can't meet sp
				//  *  ztuMode would rise into float mode or cool mode, incorrectly modelling equip
				//     which will back off fan when load is met b4 letting temp rise or using cool coil.
				//     In float mode, will use heat mode ts @ lower flow, resulting in tz > sp.
				//  solution (sometimes): determine a ts to use less flow, for less fan heat and different leaks/losses.
				//  *  here, if ts determined is exceeded with coil off, try a lower flow (cM) if tu->cMn permits.
				//
				// test for case
				if ( isZNorZN2				// only for ZN/ZN2 (expect WZ/CZ to have small enuf min flow)
						&&  !maxIt				// not if caller asked for full flow <--- added 7-7-95
#if 0
x						&&  (tuCooling ? q > 0 : q < 0)   	/* if, without this ah, zone doesn't need heat/cool
x	 					   (and increasing (heat) ts INCREASES flow) */
#else//5-6-95
						&&  (cooling ? q > 0 : q < 0)   	/* if, without this zhx, zone doesn't need heat/cool
		 					   (and increasing (heat) ts INCREASES flow) */
#endif
						&&  tu->cMn				/* if min flow non-0: else 0 flow will handle the case --
							   expect zone to go to 0-flow float mode if any. */
						&&  tu->cMn < .9*cMx			// if there is some flow range to work with (else can't reduce cM)
						//
						// ts for low flow won't work out while tu max'd; ah may not converge, so don't do it
						&&  !(tu->useAr & (uMxH|uMxC))
#if 1 //5-6-95. ztuMode similarly.
						//
						// not if there is an sp cool mode above heat mode: use full flow here, let tu change, if ts exceeded,
						// from backwards heating to more-robust normal cooling at same setpoint. Vice versa if cooling.
						// Do do it if float mode next -- next sp will be different! do it i guess if something else next.
						// MATCH ztuMode. 5-6-95.
						&&  !( cooling
							   ?  czp->mdSeq[spMd-1] != MDS_FLOAT  &&  ZhxB.p[czp->mdSeq[spMd-1]].zhxTy==ArStH
							   :  czp->mdSeq[spMd+1] != MDS_FLOAT  &&  ZhxB.p[czp->mdSeq[spMd+1]].zhxTy==ArStC ) )
#endif
				{
					// process case: Test if ttTsSp bad, if so use reDo loop to try other approaches; when done put back best found.
					//
					// calculate fan-only flow & supply temp @ ctu flow cM/czM. Slow, avoid doing more times than necessary.
					//   use pessimistic ts, with tolerance, tz so pute4F's calculation won't come out worse:
					DBL pessTs = cooling ? min( aTs, ttTsSp - Top.absTol) 	// worse of curr and proposed supply temp
								 : max( aTs, ttTsSp + Top.absTol);	// (used re trx leak adj, next)
					DBL pessTz = cooling ? min( czp->aTz, sp - Top.absTol) 	// worse of curr and setpoint zone temps
								 : max( czp->aTz, sp + Top.absTol);
					DBL crx, trx = 0;						// return flow & temp for cM @ ctu
					//  if only 1 tu (usual case), calc crx/trx anew, else adjust crNx/trNx:
					if ( !TuB.p[tu1].nxTu4a				// if ah serves only 1 terminal (no 2nd in chain)
							&&  tu1==ahCtu					// if it is control terminal (believe redundant)
							&&  tu1==tu->ss )					// if it is tu we are processing (believe redundant)
					{
						crx = cM;						// use tu's flow as total return flow: exact
						if (crx)						// don't divide by 0
							trx = ( pessTz * czM				// return air temp: contribution from zone
									+ pessTs * tu->rLeakC(cM)		//  contrib from tu supply duct leakage to return duct
								  ) / crx;
						// TESTED didn't see difference in results from adding this 1-tu case here,
						// but following ASSERTs failed, indicating there is a difference, so kept, 5-10-95:
						//  ASSERT( fabs(crNx - tu->aCv) < .01);		// verifying 1-tu case not needed...
						//  if (crNx)  ASSERT( fabs(trNx - (czp->aTz * ctu->aCz() + aTs * tu->aRLeakC())/crNx) < .001);
					}
					else		// multiple tu's. adjusted values might be inexact if aCv,tz have changed since crNx/trNx calc'd.
					{
						crx = crNx - tu->aCv + cM;				// adjust ah flow for tu's flow
						trx = ( trNx * crNx					// adjust return temp for flow and temp changes
								- czp->aTz * tu->aCz() - aTs * tu->aRLeakC()	//  subtract zone's part (see zRat/antRatTs)
								+ pessTz * czM + pessTs * tu->rLeakC(cM)  	//  and add adjusted zone values to sum
							  ) / crx;						//  divide new sigma(t*c) by new return flow
						/* wrx not needed without eco in tsfo/tsPoss, 5-95.
						 * DBL wrx = ( wrNx * crNx					// adjust return hum for flow change
						 *             + czp->aWz * (czM - tu->aCz())		//  adjust zone's contribution
						 *             + aWs * (tu->rLeakC(cM) - tu->aRLeakC())   	//  ... (aWs, aWz assumed same)
						 *           ) / crx;   					//  divide new sigma(w*c) by new return flow */
					}
					//  calc fan-only ts: like entire air handler calc except coils, iteration:
#ifdef TSPOSS // if tsfo generalized to tsPoss. (un)def'd at top file. 6-17-97.
					DBL foTs = trx;			// trx: bug precaution only
					tsPoss( trx, 0/*wrx*/, crx,		// compute supply temp for given inputs, above.
							-1, 				//   want: -1 disables use of coils and economizer (making wrx unneeded)
							foTs );  			//   receives result.
#else
					DBL foTs = tsfo( trx, crx);  	// compute fan-only supply temp for trx, crx. above.
#endif
					//#ifdef DEBUG2
					//   spare1 = foTs;				// to watch in debugger 4-95
					//#endif

					// evaluate foTs for this approach
#define MEETTOL .01			// score by approach above this margin (also tols in pessTx, pessTz above)
					/* ^^^^ TESTED: using Top.absTol not .01 --> less flow & more coil (less desirable) in T155d,
					   NO CHANGE in all other tests, so keep small til counterexample found. 5-10-95. */
#define GUDENUF .905				// .9 + tol - fuzz: if 1st isn't good enuf, do whole search
					// ^^^^ TESTED .92 instead of .905 made NO CHANGE in results 5-10-95.
					float score = cooling ? foTs - ttTsSp 	// score by amount above ts setpoint for cooling
								  : ttTsSp - foTs;	// or below heat for heat: want large (inverted) dT
					if (score > MEETTOL)  			// if can safely meet this setpoint
						score = MEETTOL + approach;		/* then score by approach irrespective of margin:
                      						   try to stay at hi flow if clear of discontinuity @ cMx. */
					// may need a bonus for same as prior approach as convergence aid **** no need yet found, 5-10-95.

					// save if new best
					if (score > bestScore)			// if best yet seen (or first)
					{
						bestTtTsSp = ttTsSp;			// save the ts setpoint
						bestScore = score;			// save score to identify next best one
						//#ifdef DEBUG2
						//      spare2 = bestTtTsSp;			// to watch in debugger
						//#endif
					}

					// endtest
					BOO enuf = fabs((cMx - tu->cMn) * bsF) < .04*cMx;	// TRUE if have searched to resolution of 4%? of max flow
					if ( bestScore > GUDENUF			// if have a good enough point
							||  enuf && bestScore > MEETTOL		// if have searched enuf points and have an adequate one 5-3-95
							||  bsF==0 )				// if just tested approach=0 after all other points (see below)
					{
						ttTsSp = bestTtTsSp;			// restore best value found
					}   						// and fall thru to stop search
					else			// initiate next test
					{
						// search for best-scoring ts. Binary search may not work well if score curve is concave down
						if (enuf)		 			// if have tested enuf points (& none adequate if here)
						{
							approach = 0;				// finish by testing cMn
							bsF = 0;				// say stop (above) after next iteration
						}
						else
						{
							bsF *= score >= lastScore ? .5 	// halve incr (init to -range, thus can't exceed range)
								   : -.5;	// reverse search direction if result worse (1st always better)
							if (score > MEETTOL)  			// if can safely meet this setpoint 5-3-95
								if (approach - bsF < 1) 		// redundant protection (ok 1st should exit per GUDENUF)
									bsF = fabs(bsF);		// then search upward only: want highest approach
							approach += bsF;   			// next fraction of way from min to max flow.
						}
						lastScore = score;			// save score for comparison on next iteration
						reDo++;					// tell the do..while (reDo) to repeat
						reDone++;					// say not original result of outer loop 7-95
						continue;					// go to do..while(reDo) endTest, skip rest of switch case
					}
				}

				// have supply temp needed at TERMINAL.
				// Reverse-adjust for common supply duct loss to get temp needed at ah ts setpoint sensor.
				// like dtSenS, but computed from ts not tSen, and latest Top.tDbOSh.
				// ttTsSp1 = ttTsSp0 - ahSOLoss*(Top.tDbOSh - ttTsSp1);  solved for ttTsSp1 in terms of ttTsSp0, with /0 protection
				//
				ttTsSp = (ttTsSp - ahSOLoss*Top.tDbOSh)/(1. - min(ahSOLoss,.9f));

#ifdef AUTOSIZE // cnglob.h 7-95. BINGO! had exactly desired effect on first try.
				// when autoSizing keep ts in design range if possible: use max flow before exceeding design range. 7-7-95.
				// otherwise, autoSize pass 2 may shrink 90%-used tu's and use higher ts's than input intended,
				//   perhaps leaving cooling (nonlinear) coil larger than necessary,
				//   if user gives ahTsDsH or -C without giving equal ahTsMx/H.
				//
				if ( Top.tp_autoSizing 					// if now in autoSizing phase
						&&  (asFlow || cooling ? ccAs.az_active : hcAs.az_active)	// if autoSizing flow or pertinent coil
						&&  cooling ? ttTsSp < ahTsDsC : ttTsSp > ahTsDsH	// if tSens for .9 approach out of design range
						&&  tu->cMn < .9*cMx					// if there is some flow range -- else don't bother
						&&  !reDone						// not if special-case loop above changed approach
						&&  !maxIt )						// not if already used 100% approach
				{
					//terse repetition of code above; commented-out parts use existing values
					// cMx = cooling ? tu->cMxC : tu->cMxH;				// pertinent max tu flow
					// if (!asFlow)							// if not autoSizing flow
					//    setToMin( cMx, max( sfan.cMx * Top.hiTol - leakCOnNx * frFanOnNx, .001*sfan.cMx));	// limit flow per sfan
#if 1
					cM = .995 * cMx + .005 * tu->cMn; 					// use 99.5% approach
#else//7-19 tried less approach re ZN2 autoSize size drift-up problem, reverted
x					cM = .985 * cMx + .015 * tu->cMn; 					// use 99.5% approach
#endif
					czM = tu->znC(cM); 							// flow that gets to zone after leak
					// q = sp * b - a;							// heat needed at zone
					j = q * airxTs / (Top.airXK * czM);					// subexpression
					DBL tttTsSp = (j >= 1.) ? ulim : (sp + 459.67*j)/(1. - j);		// ts at tu to meet load @ flow czM
					tttTsSp = tttTsSp*(1. - min(ahSOLoss,.9f)) + ahSOLoss*Top.tDbOSh;	// adjust temp back to ts sensor
					ttTsSp = cooling ? min( ahTsDsC, tttTsSp) 				// if this temp within design range
							 : max( ahTsDsH, tttTsSp);				// use design limit to reduce approach
				}
#endif

				// also allow for tu duct loss (tuSRLoss) when impl *******

				// combine with zones already done
				//
				if (cooling)
				{
					setToMin( tTsSp, ttTsSp);    			// for cooling use coldest sp req'd by any zone
					if (tTsSp <= llim)   goto breakBreak;    	// if already lowest possible or allowed, skip rest of zones
				}
				else
				{
					setToMax( tTsSp, ttTsSp);    			// for heating use hottest sp of those zones require
					if (tTsSp >= ulim)   goto breakBreak;   	// if already max'd, don't need to finish loop
				}
				break;

			} // switch (tu->useAr)
		}
		while (reDo);	// repeat to look for better approach in obscure case. Normally don't repeat.

	} // for (czp=   zone control terminals loop for ZN/ZN2/WZ/CZ cases

breakBreak:
	;

// wz/cz/zn/zn2... message if no control zone found and not estimating

	if (!foundCzn  &&  !est)
	{
		char *whatSp = cooling ? "tuTC" : "tuTH";
		char *cmtx =    CHN(ahTsSp)==C_TSCMNC_ZN2  ?  "ZN2"
						:  CHN(ahTsSp)==C_TSCMNC_ZN   ?  "ZN"
						:  CHN(ahTsSp)==C_TSCMNC_CZ   ?  "coolest zone"
						:  "warmest zone";

		if ( isZNorZN2							// if ZN or ZN2 ts control method (set in AH::begHour)
				&&  ctu								// and control terminal not missing
				&&  !(ctu->sstat[cooling ? TU_TUTC : TU_TUTH] & FsSET) )   	// and needed ctrl tu sp not set, specific msg
			rer( ABT, (char *)MH_R1286,					// "airHandler '%s': ahTsSp is '%s'\n"
				 name, cmtx, ctu->name, whatSp );			// "    but control terminal '%s' has no %s"
		else								// else general msg
			rer( PABT, (char *)MH_R1287,					// "airHandler '%s': ahTsSp is '%s'\n"
				 name, cmtx, whatSp );				// "    but no control zone with terminal with %s found"
	}
}		// AH::wzczSp
//-----------------------------------------------------------------------------------------------------------------------------
void AH::zRat()			// compute ah return air temp, flow, hum ratio per current zone/terminal info

// uses: aTs, aWs, zone aTz/aWs (not tz/wz), tu aCv (not cv/cz), .

// sets: crNx, cr1Nx;
//       unless flow is 0: trNx, tr1Nx, tr2Nx, wrNx, wr1Nx;
//       uUseAr;
//       dtRfan, r/sfan.dt.

// is only tu scan called in ahCompute before 1st cond'l exit.
// caller: ahCompute(), at entry; possibly 2 calls in ahEstimate().
{

// c, t, w sums
	TCUSE _uUseAr = uNONE;					// for union of terminal useAr's (eg for detecting pegged uses)
	DBL _cr = 0., tcr = 0., wcr = 0.;    // init accumulators
	//cria = 0.f;  tcria = 0.f;				// member accumulators const flow to const tz portion to not repeat in antRatTs.
	for (TU *tu = NULL;  nxTu(tu);  )    	// loop terminals for airHandler
	{
		_uUseAr = TCUSE( _uUseAr | tu->useAr);		// 'or' together terminal useAr's
		ZNR *zp = ZrB.p + tu->ownTi;		// zone for terminal
		// use working copy variables  zp->aTz for zp->tz  and  tu->aCv for tu->cv  to get any prior ah updates since ztuCompute.
		// return flow attributable to terminal consists of:
		//   tu->aCz() (flow to zone)  and  tu->aRLeakC() (supply leakage to plenum or return).  sum === tu->aCv.
		_cr += tu->aCv;				// sum flow: add terminal's flow at vav box, working copy.
		//if (zp->i.plenumRet)
		//{  // here compute plenum temp and w if not previously computed.  consider tuSRLoss when implemented (also in ztuCompute).
		//   tcr +=  zp->aTz * tu->aCz()  +  <plenum temp> * tu->aRLeakC();	// sum temp * flow
		//   wcr +=  zp->aWz * tu->aCz()  +  <plenum w>    * tu->aRLeakC();	// sum hum ratio * flow
		//}
		//else
		{
			tcr +=  zp->aTz * tu->aCz()  +  aTs * tu->aRLeakC(); 		// sum temp * flow.
			wcr +=  zp->aWz * tu->aCz()  +  aWs * tu->aRLeakC(); 		// sum hum ratio * flow.
		}
	}

// t and w weighted averages
	if (_cr != 0.)					// if flow.  else trNx UNCHANGED.
	{
		trNx = tcr/_cr;   				// return air temp: weighted average
		wrNx = wcr/_cr;					// return air hum ratio: weighted average
	}
	crNx = _cr;
	uUseAr = _uUseAr;      		// store results in members for use in puteRa

// compute return losses and leakages and fans
	puteRa();						// uses trNx,crNx,wrNx;  sets frFanOnNx,cr1Nx,wr1Nx,tr1Nx,tr2Nx,dtRfan,
}				// AH::zRat
//-----------------------------------------------------------------------------------------------------------------------------
void AH::antRatTs()			// anticipated return air conditions for current (changed) aTs

//inputs/outputs: ZNR.aTz (working zone temp),
//                TU.aCv (working flow, updated for active terminals), many limits,
//                AH:cr1Nx,tr2Nx ((next) return flow and temp),
//outputs in file-globals:
//   DBL tsmLLim, &tsmULim	May be updated to limits of no-additional-zone-mode-changes ts range (caller presets wide)
//   DBL fanFThresh		Receives "fanF threshold": smallest fanF that will not reduce any current flow

// caller must first adjust setout & pegged flows if limits have changed.

// uses: aTs; tu info, .

/* sets: crNx, cr1Nx;
         unless flow is 0: trNx, tr1Nx, tr2Nx, wrNx, wr1Nx;
         dtRfan, r/sfan.dt. */
// clean up these comments!

// 5-9-95: see version at end file with 500 lines of if'd out history from 1992 and 1995.

// callers: iter4Fs and its callees
{
	fanFThresh = 0.;				// init vbl to receive smallest fanF that won't reduce flow

	if (aTs < tsmLLim)  tsmLLim = LLLIM;	// reset any limits that ts has [arrived at or] passed, 4-6-92
	if (aTs > tsmULim)  tsmULim = UULIM; 	/* (not unconditionally resetting here is insurance during oscillations
    						   cuz some cases here set only the limit ts moves toward (for speed)) */

	DBL _cr = 0.f, tcr = 0.f, wcr = 0.f;   	// init accumulators

	for (TU *tu = NULL;  nxTu(tu);  )		// loop terminals for ah
	{
		ZNR *zp = ZrB.p + tu->ownTi;
		DBL tz = zp->aTz;			// zone temp as modified by prior iterations and/or other ah's
		DBL aCv = tu->aCv;    		// (prior) terminal vav flow.  Stored again below after possible change.
		DBL cz = tu->aCz();			// flow getting to zone: aCv less leaks to return
		// following if-elseif cases update tz and/or cz & aCv as appropriate for use afterwards.
		if (tu->useAr & (uSo|uMn|uMxH|uMxC))	// if constant flow
		{
			if (tu->bO==0.f)		// to a fixed temp zone (some other tu's setpoint) (as set up in ztuCompute)
				;					// just accumulate (below). cz, aCv and tz are set.
									// can't monitor its zone mode changes without getting into its SETPOINT's AH's
									// flow etc; don't know if that ah computed yet this iteration.
			else     				// bO non-0: const flow to floating temp zone (as set up in ztuCompute)
			{
				// antRatTs...

				// FIXED-FLOW (set out/pegged) tu in floating zn: compute tz, cond'ly limit to mode limits, cond'ly adj cz to next mode.

				/* TESTED 11-1-92: anticipation of flow change due to mode change to next setpoint mode,
				   found a ts that worked when iteration of separate ah-tu computations doesn't converge,
				   in some cases when low-demand heating ts rises with flow due to fan heat and there was a min flow:
				   separate computations alternated min/max tu flow forever (file sutter\SA11B13.INP 11-1-92).

				   BUT doesn't yet fix cases that oscillate cuz aTs goes above & below tz
				   (Next try letting them return & hope cnztu.cpp changes conditions;
				   when that doesn't work, may need to again attempt forcing convergence to intermediate flow here, 11-1-92).

				   BUT only covers case where next setpoint mode is on same AH; might drop if find more general solutions
				   (converger for ts at hvacIterSubhr level? try when case that fails found). 1992. */

				// reset flow so prior changes here don't "stick" unless clearly still desired.
				switch (tu->useAr)					// .. (assumes OTHER ah's haven't changed tu->aCv from tu->cv)
				{
				case uSo:
				case uMn:
					aCv = tu->cMn;
					break;	// .. fetching from tu gets latest ahVshNLims value.
				case uMxH:
					aCv = tu->cMxH;
					break;	// .. aCv stored in tu->aCv below.
				case uMxC:
					aCv = tu->cMxC;
					break;	// COORDINATE changes here with any changes in cnztu.cpp:ztuMdSets.
				}
				cz = tu->znC(aCv);					// .. zone flow for vav flow, after duct leak
				tz = (tu->aqO + aTs*cz)/(tu->bO + cz);		// compute floating zone temp at given flow
				SI md = zp->md;					// zone's mode, used in following checks
#ifdef DEBUG
				if (zp->mdSeq[md] != MDS_FLOAT)
					rer( PWRN, (char *)MH_R1288, name);	// "airHandler %s: antRatTs: internal error: not float mode as expected"
#endif
				DBL cMx = aCv;					// max aCv: set to tu->cMxH or C in some paths below
				if (md > 0)					// if not mode 0, there is sp mode below the float.
				{
					ZHX *x = ZhxB.p + zp->mdSeq[md-1];    		// next cooler (more heat needed) mode's zhx
					if (tz < x->sp)      				// if computed zone temp below setpoint of next cooler mode
					{
						tz = x->sp; 					// clamp tz to setpoint: where will be held in next mode
						if (x->ai==ss && x->ui==tu->ss)			// if this next sp mode uses this ah (and tu: insurance)
						{
							BOO cooling = x->zhxTy & StC;			// TRUE if a cooling mode
							// don't divide by 0, don't heat with too cold air (here) (-->max flow then oscillation) & vica versa,
							// don't use a heat/cool zone mode if corress ah mode off (let cnztu.cpp change ahMode. ts may change.)
							// doesn't model all quadrants of q/dT plot; improve when need found 5-95.
#if 0//was in use
x      if ( cooling  &&  aTs < tz  &&  ahMode & ahCOOLBIT
x           ||           aTs > tz  &&  ahMode & ahHEATBIT )
#else//but believe this was intended 5-5-95
							if ( cooling  ?  aTs < tz  &&  ahMode & ahCOOLBIT
									:  aTs > tz  &&  ahMode & ahHEATBIT )
#endif
							{
								cz = (tz*tu->bO - tu->aqO)/(aTs - tz);   	// adjust zone flow to what will occur in this mode
								aCv = tu->vavC(cz);				// corress flow at vav box, b4 duct leak
								cMx = cooling ? tu->cMxC : tu->cMxH;    	// get appropriate hi aCv limit, applied below.
							}
						}
					}
					else 	  					// not already out of zn's mode into next mode down
						if (tz < zp->aTz)					// speed: only needed: if gets colder, once per ahCompute.
							if (cz)						// don't /0
							{
								DBL tsmdc = (x->sp * (tu->bO + cz) - tu->aqO)/cz;	// supply temp that will put this zone into next mode down
								if (tsmdc < aTs)					// don't store mode change AT current ts
									setToMax( tsmLLim, tsmdc);			// ts extrapolation limit: tell caller
							}
				}
				if (md < zp->nMd-1)   				// if there is a mode above float mode, it is an sp mode.
				{
					ZHX *x = ZhxB.p + zp->mdSeq[md+1];		// zhx of next warmer (less heat/more cool needed) mode
					if (tz > x->sp)    				// if computed zone temp above setpoint of next warmer mode
					{
						tz = x->sp; 					// clamp tz to setpoint: where will be held in next mode
						if (x->ai==ss && x->ui==tu->ss)         		// if this next sp mode uses this ah (and tu: insurance)
						{
							BOO cooling = x->zhxTy & StC;			// TRUE if a cooling mode
							// don't divide by 0, don't heat with too cold air (here) (-->max flow then oscillation) & vica versa,
							// don't use a heat/cool zone mode if corress ah mode off (let cnztu.cpp change ahMode. ts may change.)
							// doesn't model all quadrants of q/dT plot; improve when need found 5-95.
#if 0//was in use
x							if ( cooling  &&  aTs < tz  &&  ahMode & ahCOOLBIT
x							 ||           aTs > tz  &&  ahMode & ahHEATBIT )
#else//but believe this was intended 5-5-95
							if ( cooling  ?  aTs < tz  &&  ahMode & ahCOOLBIT
									:  aTs > tz  &&  ahMode & ahHEATBIT )
#endif
							{
								cz = (tz*tu->bO - tu->aqO)/(aTs - tz);  	// adjust zone flow to what will occur in this mode
								aCv = tu->vavC(cz);				// corress flow at vav box, b4 duct leak
								cMx = cooling ? tu->cMxC : tu->cMxH;      	// get appropriate high aCv limit, used below
							}
						}
					}
					else							// not already out of zn's mode into next mode up
						if (tz > zp->aTz)					// speed: only needed if gets warmer, once per ahCompute
							if (cz)      					// don't /0
							{
								DBL tsmdc = (x->sp * (tu->bO + cz) - tu->aqO)/cz;	// supply temp that will put this zone into next mode up
								if (tsmdc > aTs)					// don't store mode change AT current ts
									setToMin( tsmULim, tsmdc);				// ts extrapolation limit: tell caller
							}
				}
				if (aCv > cMx)
				{
					aCv = cMx;         // apply aCv limits & recompute cz,
					cz = tu->znC(aCv);
				}
				if (aCv < tu->cMn)
				{
					aCv = tu->cMn;     // for cases above that change aCv
					cz = tu->znC(aCv);
				}
			}
		}
		else if (tu->useAr & (uStH|uStC))	// if flow is controlled by this terminal's setpoint
		{
			BOO tuCooling = (tu->useAr==uStC);				// true if terminal is cooling (ah may differ)
			ZHX *x = ZhxB.p + (tuCooling ? tu->xiArC : tu->xiArH);	// point zhx in use
#ifdef DEBUG2								// internal consistency check, omit in release version
			if (x->mda != zp->md)
				rer( PWRN, (char *)MH_R1289, name, tu->name);		// "airHandler %s: Internal error in antRatTs: \n"
			// "    terminal %s is not terminal of zone's active zhx"
#endif
			/* autoSize flow-tried-to-run-away flag: clear here if ts on "right" side of sp;
			   set below if flow tries to get too large; hold prior value while ts on "wrong" side of sp. 7-16-95. */
			if (tuCooling ? aTs < x->sp : aTs > x->sp)		// if cooling & ts cool enuf or heating & hot enuf to heat
				tu->aDtLoTem = FALSE;				// clear flag: ts is ok or flag will be set below.

			// antRatTs: ACTIVE TERMINAL

			tz = x->sp;						// get setpoint temp from zhx. Note used zp->tz til 4-17-95.
			DBL q = tz*tu->bO - tu->aqO;				// heat which will make zone temp be setpoint tz
			DBL dT = aTs - tz;					// supply temp delta from zone setpoint
			DBL cMn = tu->cMn; 					// min flow at terminal
			DBL cMx = tuCooling ? tu->cMxC : tu->cMxH;		// get applicable max flow at terminal
			DBL czMn = tu->znC(cMn), czMx = tu->znC(cMx);		// min and max flows at zone, after leak/loss
			DBL tzMn = (tu->aqO + aTs*czMn)/(tu->bO + czMn);	// what zone temp would be at terminal for min flow ...
			DBL tzMx = (tu->aqO + aTs*czMx)/(tu->bO + czMx);	// ... for max flow     assuming any other ts's to zone hold.
			SI mD = 0;						// for anticipated zone mode delta
			BOO resizeOk = FALSE;					// TRUE if resizing tu for larger flow may be ok, 7-95

			// test for overtemp/undertemp cases to handle 'wrong' sign q or dT

			/* CAUTION: discontinuities at 'backwards' modes flow limits: flow jumps between min and max and ah may not converge.
			   Temperature tolerances are used here with intent of moving the discontinuity to a flow where ztuMode
			   would not have zone in uStC/uStH mode; coordinate changes with ztuMode. */

			if (min( tzMn, tzMx) > tz + 1.e-6)		// if zone will be too warm
			{
				mD = +1;				// ztuMode will use next higher zone mode
				if (!tuCooling)       		// for heating
					goto minPeg;			// use min flow (a discontinuity if 'backwards' (if q < 0 && dT < 0))
				if (q > 0 && dT > 0)  		// for backwards cooling, tu will use max, but here use min to avoid jump
					goto minPeg;			// (moves discontinuity to q==0 which isn't crossed in ah cuz only dT varies)
				// TESTED fixed ah non-convergence at jump during tu-ah cvgnce, T136c, 5-6-96.
#ifdef AUTOSIZE // 7-95
				// recoded 7-22-95 see cnah2.cp8 for old
				if (q < 0 && dT < 0)		// if normal cool case: zone needs coolth and ah is delivering coolth
					resizeOk = TRUE; 		// say ok to conditionally resize tu to meet setpoint, if autoSizing, below.
				else
#ifdef LOGRO // rob 6-97  cooling analogue of heating LOGRO just below.
					if ( q < 0  &&  dT > 0		// if need coolth but ts is on wrong size of zone setpoint (dT is aTs - sp here)
							&&  aTs < zp->aTz )		// but aTs is cooler than zone: will cool
					{
						/* Zone needs cooling and ts is above zone setpoint but will cool current tz.
						   When autosizing, need to let tu get resized larger else system "sticks" too small with zone too warm,
						   cuz cncoil:coolCapThing prevents coil size increase without flow increase, then ah "converges" cuz no change.
						   Observed with (for heat, below) large duct loss (BUG0089, 6-97). */
						aCv = 1.02 * cMx;		/* if resizing, increase tu max flow (fallthru maxPegs if can't resize).
						   Only 2%: iterate tr up so coil size doesn't overshoot lots.
						   tz and cz are recomputed on fallthru whether resizes or not. */
						// TRIED 6-97, didn't converge as above does: compute flow needed based on dT from CURRENT zone temp:
						//          dT = aTs - zp->aTz;   cz = q/dT;   aCv = tu->vavC(cz);  simplifies to:
						//aCv = tu->vavC(q/(aTs - zp->aTz));	// tz and cz are recomputed on fallthru
						//setToMax( aCv, cMx * 1.1f);
						resizeOk = TRUE;		// say ok to conditionally resize tu to this flow, if autosizing, below.
#ifndef TRY2 // is below if TRY2
						tsHis[0].gud = 0;		/* say don't extrapolate ts from this iteration - nonlinear if here.
						   TESTED 6-97 - made BUG0089 converge (tho slowly). */
#endif
						goto spaghetti;			// go resize if can, else maxPeg.
					}
					else				// ts <= tz  (presuming q < 0 doesn't get here)
#endif
#endif
						goto maxPeg;			// other cooling cases uses max flow if too warm
			}
			else if (max (tzMn, tzMx) < tz - 1.e-6)	// if zone will be too cold
			{
				mD = -1;						// ztuMode will use next lower mode
				if (tuCooling)       		// for cooling
					goto minPeg;			// use min flow (a discontinuity if 'backwards' (if q > 0 && dT > 0))
				if (q < 0 && dT < 0)  		// for backwards heating, tu will use max, but here use min to avoid jump
					goto minPeg;			// (moves discontinuity to q==0 which isn't crossed in ah cuz only dT varies)
#ifdef AUTOSIZE // 7-95
				// recoded 7-22-95 see cnah2.cp8 for old
				if (q > 0 && dT > 0)		// if normal heat case: zone needs heat and ah is delivering heat
					resizeOk = TRUE; 		// say ok to conditionally resize tu to meet setpoint, if autoSizing, below.
				else
#ifdef LOGRO // rob 6-97. GOOD: fixed BUG0089 problem (now covered elsewhere but this appears to be good insurance).
					if ( q > 0  &&  dT < 0		// if need heat but ts is on wrong size of zone setpoint (dT is aTs - sp here)
							&&  aTs > zp->aTz )		// but aTs is warmer than zone: will heat
					{
						/* Zone needs heat and ts is below zone setpoint but will heat current tz.
						   When autosizing, need to let tu get resized larger else system "sticks" too small with zone too cold,
						   cuz cncoil:heatCapThing prevents coil size increase without flow increase, then ah "converges" cuz no change.
						   Observed with large duct loss (BUG0089, 6-97). */
						aCv = 1.02 * cMx;		/* if resizing, increase tu max flow (fallthru maxPegs if can't resize).
						   Only 2%: iterate tr up so coil size doesn't overshoot lots.
						   tz and cz are recomputed on fallthru whether resizes or not. */
						// TRIED 6-97, didn't converge as above does: compute flow needed based on dT from CURRENT zone temp:
						//          dT = aTs - zp->aTz;   cz = q/dT;   aCv = tu->vavC(cz);  simplifies to:
						//aCv = tu->vavC(q/(aTs - zp->aTz));	// tz and cz are recomputed on fallthru
						//setToMax( aCv, cMx * 1.1f);
						resizeOk = TRUE;		// say ok to conditionally resize tu to this flow, if autosizing, below.
#ifndef TRY2 // is below if TRY2
						tsHis[0].gud = 0;		/* say don't extrapolate ts from this iteration - nonlinear if here.
						   TESTED 6-97 - made BUG0089 converge (tho slowly). */
#endif
						goto spaghetti;			// go resize if can, else maxPeg.
					}
					else				// ts <= tz  (presuming q < 0 doesn't get here)
#endif
#endif
						goto maxPeg;			// other heating cases uses max flow if too cold
			}
			else 						// a flow in range can meet setpoint
				if (tuCooling ? dT > 0 : dT < 0)		// if 'backwards setpoint' case: heating with air colder than zone, etc.
				{
					// the condtional 'backwards' or 'third quadrant' case: described for heating:
					//   q and dT both < 0 and possible to find flow in range that meets setpoint: 0-flow temp above sp, flow COOLs zone.
					// for ZN/2, if here, use the flow that just lowers tz to sp. setTsSp1 has determined ts for this.
					//   Expected when heat coil must be used to counter some of leak/loss at non-0 cMn when deadband above.
					//   (if min flow 0, expect tu uMn in deadband, if no deadband, expect tu cooling (thus not bkwds) while ah heats).
					// If not ZN/2, use min flow as ztuMode will.
					// CAUTION: coordinate code with cnah.cpp:AH::setTsSp1() and cnztu.cpp:ZNR::ztuMode().

					if ( isZNorZN2			// if ZN or ZN2 ts sp control (set in AH::begHour)
							&&  tu->ctrlsAi==this->ss    	// if this is the control terminal
							&&  Top.iter < 4 )		/* if early in global convergence. Might not be possible (eg too much fan heat)
						   and might mode-hunt, or ah might not converge near flow jump to min @ max.
						   MATCHES test in cnztu.cpp:ztuMode, 2 places, 5-95. */
						;				/* special case. Fall thru to compute flow as usual (q, dt signs both reversed)
						   but reverse sense of mode changes at limits. CAUTION: discontinuities! */
					else				// normally don't do 'backwards' heating or cooling
					{
						mD = tuCooling ? -1 : 1;
						goto minPeg;			// just go to min flow
					}
				}

			// don't divide by 0! (didn't get a dT==0 here til 7-4-95!)
			if (dT==0)				// if aTs==tz (and tz very near sp if here), don't divide by 0
				goto minPeg;			/* use min flow 7-4-95: match cnztu.cpp:ztuMode() (2 places).
						   Previously used max flow but interfered with autoSize peak recording. */

			// compute flow aCv @ ts to keep this zone at setpoint.  q = tz*b - a;  c = q/(ts - tz)  --->  c = (tz*b - a)/(ts - tz).

			cz = q/dT;				// active terminal flow to zone to meet setpoint (quadrants 1 and 3)
			aCv = tu->vavC(cz);			// vav flow req'd to allow for leaks

			// if terminal (newly) pegged & float mode next, solve for its zone's temp instead, using:   tz = (a + ts*c)/(b + c)

			if (aCv < cMn)   			// if flow at vav box < minimum (due to tolerances above)
			{
				mD = tuCooling ? -1 : 1;			// anticipate zone mode change
minPeg:
				aCv = cMn;   				// use min flow
				goto label;				// mD = tuCooling ? -1 : 1  preset above for min flow
			}
			else if (aCv > cMx)   			// if flow needed at vav > maximum
			{
spaghetti: 	// for #ifdef LOGRO, rob 6-97
#if 0 // 6-97. Looks nice, but make BUG0090 not converge (with TRY3 and TRY5 in cncoil, 6-17-97).
*     // to retry, let fraction of change thru anyway so coil can grow.
*          if ( tu->ss==ahCtu			// if this tu is control terminal for this ah
*           &&  isZNorZN2			// if ZN or ZN2 ah ts cm this hour - probably redundant
*           &&  fcc 				// if fan is cycling this hour
*           &&  frFanOn < 1 )  		// if fan not on all the time
*		{
*         // don't increase size to compensate for low frFanOn (eg just after estimate) -- let frFanOn increase first
*             // relative power for frFanOn = 1.0: cMx/frFanOn.
*             // addtional:                        cMx/frFanOn - cMx  =  cMx*(1/frFanOn - 1)
*             aCv -= cMx * (1/frFanOn - 1);	// reduce flow request by power full frFanOn will add
*             if (aCv <= cMx)			// if this leaves no increase
*                goto maxPeg;			// go use max (sets aCv)
*
*         // limit increase per iteration to limit overshoot & thrash as needed coil size found
*             setToMin( aCv, cMx * sqrt( aCv/cMx));	// .. * Top.auszHiTol2  if need found
*		}
#endif
		// recoded here 7-22-95, big deletions, see cnah2.cp8.
		if ( resizeOk			// set above if a case where size increase appropriate if autoSizing
				&&  asFlow 			// if autoSizing this ah's flow with open-ended models: believe redundant
				&&  !fanLimited 			// if flow not reduced by fanF (unexpected; insurance 7-95)
				// not if in ZN2 fan-only mode with coil present & available to turn on now:
				// in that case let ztuMode turn on coil b4 increasing flow, to not get spurious flow increases.
				// similar test in TU::resizeIf. 7-19...22-95.
				&&  !(ahMode==ahFAN  &&  CHN(tuCooling ? ahcc.sched : ahhc.sched)==C_OFFAVAILVC_AVAIL)
				&&  tu->resizeIf(			// conditionally resize / if done. checks Top.tp_sizing, tu->ah/cAs.az_active, etc.
					aCv, 				// desired heat cap flow, MAY BE ALTERED (ref arg).
					tuCooling, 			// 0 if heating
					aTs * tu->bO - tu->aqO,		// runaway flow limiting expr. see derivation with TU::resizeIf.
					FALSE ) )
		{
			// resizeIf has changed tuVfMxH and/or C and cMxH and/or C. cMx now wrong.
			Top.ztuKf++;
			zp->ztuCf++;	// be sure ztuMode is called again [to record the new high flow]
#ifdef TRY2 // disable extrapolator at size change, 6-11-97
*             tsHis[0].gud = 0;		// after resizing say don't extrapolate ts from b4 this ts/cr point. 6-97.
#endif
					goto label;			// go recompute cz and tz, cuz aCv may be changed & setpoint may not be held.
		}
		else				// aCv > cMx and not resizing this function now
		{
			mD = tuCooling ? 1 : -1;   		// anticipate zone mode change
maxPeg:
			aCv = cMx;     				// use the max flow
label:
			cz = tu->znC(aCv);   			// flow to zone, after leaks
			if (zp->mdSeq[zp->md + mD]==MDS_FLOAT)	// if expect a float mode next (else assume tz = setpoint held)
			{
				tz = (tu->aqO + aTs*cz)/(tu->bO + cz);	// estimate float tz for zp->aTz, assuming zn's other ts's hold
			}
		}
	}
	else		// cMn <= aCv <= cMx
	// active terminal... not pegged, determine how far ts is from pegging it, for caller
	{
		DBL _c = (aCv > tu->aCv) ? cMx : cMn;			// check only limit aCv moved toward (speed)
		if (_c != 0.f)
		{
			DBL _cz = tu->znC(_c);   				// zone flow corressponding to vav limit
			DBL tsmdc = (tz * (tu->bO + _cz) - tu->aqO)/_cz;	// supply temp that will max-peg or min-peg tu
			if (tsmdc < aTs)
				setToMax( tsmLLim, tsmdc);
			else if (tsmdc > aTs)   				// don't store if ==
				setToMin( tsmULim, tsmdc);
		}
	}

		// active terminal... all cases... conditionally undo previous larger autoSize this subhour eg so frFanOn can stay 1.0.

		tu->resizeIf( aCv, tuCooling, 0, TRUE);	/* called here to reduce tuVfMxH/C & cMxH/C per aCv,
						   if now larger, but not below start subhour value. ztuMode.cpp. */
	}
	else 					// end of if tu->useAr...else if...
		rer( PWRN, (char *)MH_R1290, 			// printf-->rer 10-92
			 name, (UI)tu->useAr );   		// "airHandler %s: Internal error in antRatTs: unexpected tu->useAr 0x%x"


	// antRatTs: ALL TU CASES merge here with tz, cz, aCv set to prior or updated values

	tu->aCv = aCv;						// store possibly-updated tu vav flow and zone temp
	zp->aTz = tz;						// .. in ah's working variables

	// humidity
	zp->aWz = zp->znW(Top.humMeth == C_HUMTHCH_PHIL
						? tu->wcO + cz * (wsls + aWs) / 2. 	// use average supply hum
						: tu->wcO + cz * aWs,      		// C_HUMTHCH_ROB: use latest supply hum
						tu->cO + cz,
						tz);					// this is .aTz. arg added 10-96.

		// fanF threshold and max
#ifndef AUTOSIZE
x       setToMax( fanFThresh, aCv / (tu->tuVfDs * airxTs));	/* increase to value that wont quite limit this c,
x       							   using air Btuh/cfm-F @ ts. */
#else
		float tuVfDs = tu->tuVfDs;				// terminal design air flow
		if (tuVfDs==0)  						// 0 allowed during autoSizing
			tuVfDs = max( tu->tuVfMn, tu->tuVfMxH, tu->tuVfMxC);	// if 0, do dynamic default 6-95 for autoSizing
		if (tuVfDs)						// if 0, don't divide by it
		{
			setToMax( fanFThresh, aCv / (tuVfDs * airxTs));		/* increase to value that wont quite limit this c,
       								   using air Btuh/cfm-F @ ts. */
			setToMax( fanFMax, aCv / (tuVfDs * airxTs));		/* if new terminal autoSizing max flow, update max fanF
 								   that could reduce flow to prevent premature limiting 7-18-95*/
		}
		else							// tuVfDs is zero
			setToMax( fanFThresh, .1);				// insurance: ?? don't let fanFThresh end up zero 6-95
#endif

		// sum flow, t * flow, w * flow.
		/* return flow attributable to terminal consists of:
		     tu->aCz() (flow to zone, in cz here)  and  tu->aRLeakC() (supply leakage to plenum or return).  sum is aCv = tu->aCv.*/
		_cr += aCv;							// sum flow
		// ??? does aWz need recomputing ? perhaps for aCv / aWs? 4-92
		//if (zp->i.plenumRet)
		//{  // here compute plenum temp and w if not previously computed.  consider tuSRLoss when implemented (also in ztuCompute).
		//   tcr +=  zp->aTz * tu->aCz()  +  <plenum temp> * tu->aRLeakC();	// sum temp * flow
		//   wcr +=  zp->aWz * tu->aCz()  +  <plenum w>    * tu->aRLeakC();	// sum hum rat * flow
		//}
		//else
		{
			tcr +=  tz      * cz  +  aTs * tu->aRLeakC(); 		// sum temp * flow.  note aTs, not ts.
			wcr +=  zp->aWz * cz  +  aWs * tu->aRLeakC(); 		// sum hum rat * flow.
		}
	}  // for (tu=


// compute weighted average return air temp and hum rat

	if (_cr != 0.f)					// if have flow -- else trNx, wrNx UNCHANGED
	{
		// must match zRat exit code
		trNx = tcr/_cr;   				// return temp: weighted average
		wrNx = wcr/_cr;				// return hum rat: weighted average
	}
	crNx = _cr;				// store results in members.


// compute return losses and leakages and fans

	puteRa();				// uses trNx,crNx,wrNx; sets frFanOnNx,cr1Nx,wr1Nx,tr1Nx,tr2Nx,dtRfan,
}			// AH::antRatTs
//-----------------------------------------------------------------------------------------------------------------------------
void AH::puteRa()			// compute return air leaks & losses, some fan heats, for next iteration

// runtime inputs: trNx, crNx, wrNx;

// sets: frFanOnNx, cr1Nx, wr1Nx, tr1Nx, dtRfan, tr2Nx, .  pute's return fan, not relief fan.

// common exit code for zRat, antRatTs 4-92; also called from ceeClip 5-95.
{
	// does any part of plenum computation go here?

// because caller just changed TrNx etc, say getTsMnFo and getTsMxFo, if called, should calculate new values. 5-95.
	tsMnFoOk = tsMxFoOk = FALSE;

	/* init leakage
	     Average supply and return leak to conserve air for now, pending infiltration, zone exhaust fans, etc. --
	     ie have SAME AMOUNT of supply air implicitly leak out.  Supply outLeak currently 6-92 not represented anywhere in simulation.
	     Leak is fraction of supply fan rated flow when fan on --> * frFanOn for time-average. */

#ifdef AUTOSIZE//7-95
	DBL leakFracOn = Top.tp_pass1A && asFlow  		// fan-on leakage as fraction of supply fan design flow
					 ?  0				// use 0 during autoSizing pass 1 part A: determining fan design flow
					 :  (ahROLeak + ahSOLeak)/2.;	// use average of user's supply & return leaks, for balance for now.
#else
x       DBL leakFracOn = (ahROLeak + ahSOLeak)/2.;   	// fan-on leakage as fraction of supply fan design flow
#endif
#ifndef FANREDO
x#ifdef SHINTERP	// subhour-interpolated weather data, 1-95. cndefns.h (via cnglob.h)
x    leakCOnNx = sfan.vfDs * leakFracOn * Top.tp_airxOSh;	// fan-on leakage in heat cap units (conv cfm to Btu/hr-F @ tDbOSh)
x#elif 1//use airxO 1-95
xo    leakCOnNx = sfan.vfDs * leakFracOn * Top.airxO;	// fan-on leakage in heat cap units (conv cfm to Btu/hr-F @ tDbO)
x#endif
x    setToMin( leakCOnNx, sfan.cMx/* *.99? */);		// don't leak more than fan can blow even if airx @ fan << outdoors.
x							/* Note with this leak model with drawthru fan, at high ts leak can
x							   approach 100%, fanF approaches 0, ts runs away to ahTsMx. 4-95. */
x// determine fan fraction on now: uses fan-on leakeage; needed to determine average leakage. 6-24-92.
x    if (fcc)						// if fan cycles this hour (set in AH::begHour; else frFanOnNx preset 1.0)
x       setFrFanOn(leakCOnNx);				// set frFanOnNx given crNx, sfan.cMx, leakage argument, ctu cMx's. below.
x
x// return duct leak and loss (loss tentatively done second)
x    DBL leakC = leakCOnNx * frFanOnNx;			// average leakage, heat cap units
x    cr1Nx = crNx + leakC;    				// specified amount of air leaks INTO return duct (duct is under vacuum)
#else
	DBL leakC;						// average leakage, heat cap units, used in/after loop
	float sfanCMxWas;
	SI i = 0;  			// used in loop endtest
	do  						// cond'ly repeat this part if sfan.vfDs changes
	{

		// leakage...
		leakCOnNx = sfan.vfDs * leakFracOn * Top.tp_airxOSh;	// fan-on leakage in heat cap units (conv cfm to Btu/hr-F @ tDbOSh)
		setToMin( leakCOnNx, sfan.cMx/* *.99? */);		// don't leak more than fan can blow even if airx @ fan << outdoors.
		/* Note with this leak model with drawthru fan, at high ts leak can
		   approach 100%, fanF approaches 0, ts runs away to ahTsMx. 4-95. */

		// determine fan fraction on now: uses fan-on leakeage; needed to determine average leakage. 6-24-92.
		if (fcc)						// if fan cycles this hour (set in AH::begHour; else frFanOnNx preset 1.0)
			setFrFanOn(leakCOnNx);   			// set frFanOnNx given crNx, sfan.cMx, leakage argument, ctu cMx's. below.

		// return duct leak, flow entering air handler
		leakC = leakCOnNx * frFanOnNx;			// average leakage, heat cap units
		cr1Nx = crNx + leakC;   				// specified amount of air leaks INTO return duct (duct is under vacuum)

		// if autoSizing, condtionally adjust supply fan size now to match flow: may change frFanOn.
#if 0 // no, size it for the average as before!!, let frFanOn increase. 7-22-95.
x		DBL cr1NxOn = frFanOnNx != 0. ?  cr1Nx/frFanOnNx   	// fan-on flow, to size fan, 7-21-95
x                  :  cr1Nx==0. ? 0. : sfan.cMx;			// /0 protection: insurance; unexpected.
#endif
		sfanCMxWas = sfan.cMx;
#ifdef TRY2	// not extrapolating from resize, 6-97
*		if (resizeFansIf(				// mbr fcn, cnfan.cpp. If autoSizing this ah's fan(s) now,
*                       cr1Nx*Top.auszHiTol2,    	// increase fan cfm (vfDs) to this heat cap value if smaller, or decrease
*                       bVfDs ) )   			// ... but not below this start-subhour value.  And reSetup fan (sets cMx).
*        tsHis[0].gud = FALSE;   			// if did change size, don't extrapolate from before here. 6-97.
#else
		resizeFansIf( 					// mbr fcn, cnfan.cpp. If autoSizing this ah's fan(s) now,
			cr1Nx*Top.auszHiTol2, 		// increase fan cfm (vfDs) to this heat cap value if smaller, or decrease
			bVfDs );					// ... but not below this start-subhour value.  And reSetup fan (sets cMx).
#endif
	}
	while ( RELCHANGE( sfan.cMx, sfanCMxWas) > Top.relTol/10  	// repeat stuff using sfan.cMx if changed by resizeFansIf,
			&&  ++i < 3 );					// at most 3 times
#endif

// return duct leak and loss ... (loss tentatively done after leak)
	if (cr1Nx != 0.)					// if there is flow
	{
		wr1Nx = (wrNx * crNx + Top.wOSh * leakC)/cr1Nx;		// hum rat after leakage
		tr1Nx = (trNx * crNx + Top.tDbOSh * leakC)/cr1Nx; 	// temp after leakage
		tr1Nx = (1. - ahROLoss) * tr1Nx + ahROLoss * Top.tDbOSh;	// temp after conduction loss to outside
	}
	else if (ahROLoss != 0.)				// no flow (including no leak). If there is conductive loss,
		tr1Nx = Top.tDbOSh;					/* set temp to outdoor for results 6-16-92 (known inconsistent:
       								   ahROLoss definition is "fraction of temp difference") */
	// else: no flow, no conduction: leave tr1Nx unchanged.

// return fan heat, and return or relief fan motor heat if in air stream
	DBL dtRfan;
	if (rfan.fanTy==C_FANTYCH_RETURN)			// if have return fan (relief fan done after eco, when flow known)
	{
		rfan.fn_pute( cr1Nx, tr1Nx, frFanOnNx);  		// compute fan cMx, p, q, dT, etc for flow and temp
		dtRfan = rfan.dT;				// temp change to return air due to return fan work and motor heat
	}
	else						// NONE or C_FANTYCH_RELIEF: leave 0: relief fan heats exhaust air only.
		dtRfan = 0;					// (relief fan heats exhausted air only)
	if (sfan.motPos==C_MOTPOSCH_INRETURN)		// if supply fan motor is in return air
		if (cr1Nx)    					// don't divide by 0
			dtRfan += sfan.qAround/cr1Nx;			// supply fan motor heat warms return air. qAvg/cAvg===qOn/cOn.
	tr2Nx = tr1Nx + dtRfan; 				// next-iteration value for tr2.
	// pute4Fs() copies tr2Nx to tr2; thus tr2 unchanged if exit now.
	//related code: rfan.q --> rfanQ in pute4Fs, then --> ahres.qFan in AH::endSubr: last used value for results.
}				// AH::puteRa()
//-----------------------------------------------------------------------------------------------------------------------------
//if-outs removed 6-24-92; old at eof:
void AH::setFrFanOn( 			// determine fan on fraction this subhour.  Call only if fcc.

	DBL _leakCOn )		// leakage in heat cap units: difference of sfan and tu flows
// currently same as "leakCOnNx" member (debug aid, or cd remove this arg) 6-25-92.

// uses: crNx, sfan.cMx, uUseAr, control terminal cMn/cMxC/H
// sets: frFanOnNx
{
	// called from puteRa
//
// --- FanCycles General Comments ---
//
// Used to simulate systems such as PTAC and RESYS where fan runs only on demand from zone thermostat; these real systems
//    generally do not have distinct airHandler and terminal, but they are simulated in CSE with an AH and 1 TU.
//
// Under fanCycles, the supply fan and coils runs only for the fraction of the subhour needed to satisfy the load.
//    When on, the fan runs at full flow and coils typlically run at or near full power (under tsSp1 control as usual).
//    The fraction on required is determined here as the fraction of possible flow requested by the terminal
//    (fraction of full flow for whole subhr xfers same heat as full flow for same fraction of subhr @ same supply temp).
//
// fcc conventions:
//    airHandler temperatures and humidities represent fan-on values -- they are undefined when there is no flow.
//    flows and powers represent time average over subhour unless "fan-on" is indicated.
//    any flow can be converted to fan-on value where needed by DIVIDING BY frFanOn.
//    "maximum flow" is smaller of maximum for terminal or airHandler.
//
// if NOT fcc:  frFanOn is 1.0.  "Average" and "fan-on" values are the same.
//
// Note that compressor/coil part load cycling is a separate phenomenon that occurs during the fan-on time.
//
// Restrictions (enforced in setup): only 1 terminal (addl terminals would require code to "slave" their relative flows)
//
// Hourly setup: "fcc" is set in AH::begHour per hourly-variable ahTsSp and ahFanCycles; when false, it sets frFanOnNx to 1.0.


// fan-on flow === max poss avg flow = smaller of ctl tu max flow, and supply fan's max flow with overrun less leakage.

	//DBL cMxnx;  made member				// .. typical sensible input wd be sfan.cMx==ctu->cMxC==cMxH; not enforced.
	TU* ctu = TuB.p + ahCtu;			// point ah's control terminal (ahCtu verified non-0 in AH::begHour)
#if 1 // 4-25-95 after zRat call added in ahEstimate
	if (uUseAr==uNONE)					// can occur at startup (ahEstimate b4 1st ztuCompute), 4-95.
		cMxnx = 0.;					// no flow if control terminal not (yet) requesting any air
	else
#endif
		if (uUseAr==uMn)  				// min flow but ah not off: occurs if ahTsSp != ZN/ZN2 && ahFanCycles==YES.
		{
			// require 0 min flow cuz dunno whether to use cMxC or cMxH re cMxnx.
			if (ctu->cMn != 0.)					// insurance check here: begHour cks vfDsMn==0.
				rer( WRN, (char *)MH_R1291, ctu->name, ctu->cMn );	// "Terminal '%s': tuVfMn is %g, not 0, when ahFanCyles = YES"
			cMxnx = 0.;						// allow no flow; fall thru
		}
		else
		{
#ifdef DEBUG
			if (!(uUseAr & (uStH|uStC|uMxH|uMxC)))		// devel aid check. uUseAr is set in zRat.
				rer( PWRN, (char *)MH_R1292, 			// "AH::setFrFanOn: airHandler '%s': unexpected 'uUseAr' 0x%x"
					 name, (UI)uUseAr );  		// after msg fall thru (assumes heating)
#endif
			cMxnx = 						// flow for frFanOn=1.0 this subhour for this ah/tu is smaller of tu, sfan
				min( uUseAr & (uStC|uMxC)   		// test whether ah is on cuz terminal called for cooling or heat
					 ? ctu->cMxC : ctu->cMxH,     	// tu cool or heat max flow per current use (set by AH::vshNLims)
					 sfan.cMx				// supply fan max flow, incl overrun (FAN::pute sets).
					 - _leakCOn );			// less fan-on leak-in (==leak-out) to get poss tu flow at max sfan flow
		}
	if (cMxnx==0. && crNx != 0.)      			// fatal error if there is flow when no flow is possible
#ifdef AUTOSIZE
		if (!Top.tp_sizing)					/* in autoSizing can have flow here b4 tu and/or sfan cMx made
							   non-0 to reflect the demand. Fall thru & make frFanOnNx 1. 7-95. */
#endif
			rer( PABT, (char *)MH_R1293, name, crNx );	//"AH::setFrFanOn: airHandler '%s': \n    cMxnx is 0 but crNx is non-0: %g"
	/* cMxnx==0 cd occur if ctu->cMxH/C scheduled 0 to disable function, and there were no leaks.  But then there shd be no flow.
	   (Only 1 tu allowed under fcc.) (Review re future infil/exfil imbalance.) */

// next iteration fraction fan on === fraction possible power requested for subhour by terminal === crNx/cMxnx.

#ifndef AUTOSIZE//cnglob.h
x    frFanOnNx = cMxnx==0. ? 0.		// if no flow is possible, don't run fan, don't divide by 0 (crNx is 0: checked above).
x                          : crNx/cMxnx;	// fraction on = fraction of ah poss output needed = requested flow / max flow
x					// TESTED limiting frFanOnNx to 1.0; NO DIFFERENCE (T9, T52, 6-16-92) <-- b4 autoSize.
#else
	frFanOnNx = cMxnx==0. 		// if no flow is possible, don't divide by 0,
				?  crNx ? 1. : 0.	//  but run fan anyway if flow demanded (possible if autoSizing, else err above).
			:  crNx/cMxnx; 	// fraction on = fraction of ah poss output needed = requested flow / max flow
	// TESTED limiting frFanOnNx to 1.0; NO DIFFERENCE (T9, T52, 6-16-92) <-- b4 autoSize.
	/* Again TESTED limiting frFanOn to 1.0 here & in tsfo: Trivial effect
	   on some runs, eg T167e,f, T168e,f, T93a,b,c, 7-95. Change kept, for autoSize. */
	// if (Top.tp_autoSizing && asFlow)	believe ok to do always.
	setToMin( frFanOnNx, 1.);		/* limit to 1.0: during flow autoSizing when not autoSizing supply fan,
					   flow is allowed to exceed sfan.cMx and cMxnx. 7-95. */
#endif
}		// AH::setFrFanOn
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::flagTusIf()		// if ah output changed, flag served terminals' zones and return TRUE

// propogates air handler changes, for ahEstimate and ahCompute (called AFTER ts & ws updated).
// Moved here from hvacIterSubhr loop, 10-15-92.
{
	DBL dt = ah_tSup - trNx;					// current delta-T thru air handler
	DBL tsChg = ABSCHANGE(ah_tSup, tsPr);		// change in supply temp ts, current vs prior
	if ( tsChg > fabs(dt) * RELoverABS			// ts change vs ah's dt: keep iterating while system floating up or down
			||  tsChg > Top.absTol				// ts (supply temp) change > .1 degree (or as changed)
			||  ABSCHANGE( ws, wsPr) > Top.absHumTol		// ws (supply humidity) change > .00003 (or as changed)
			||  RELCHANGE( dt, dtPr) > Top.relTol )  		// delta-T change: important when coilLimited
	{
		flagTus();					// set ztuCf for served zones
		/* 10-92 Now that it is possible to ahCompute twice between ztuComputes, ideally the following Priors would be updated
		   at ztuCompute exit -- this way, they can differ from values last used in ztuCompute by as much as tolerance.
		   Would require making them tu members and having tu loop here. Do when need found. */
		tsPr = ah_tSup;					// update comparators (change reference)
		wsPr = ws;					// ..
		dtPr = dt;					// ..
		return TRUE;					// say did flag them
	}
	return FALSE;
}			// AH::flagTusIf
//-----------------------------------------------------------------------------------------------------------------------------
void AH::flagChanges()		// conditionally flag zone for zone temp/w and tu flow changes from ztuCompute output.
// do once at end ah to minimize spurious flagging if hunt and get back to old value
//  yet flag cumulative effect of multiple small but non-cancelling changes.
{
	/* tolerances tried: .01*relTol: adds final tu (only, not ah) iteration in some files, changing
			     temp by .01 in a few of those cases.  10*relTol: dropped a few more tz iterations
			     with no temp changes detected. 4-7-92. */
	for (TU *tu = NULL;  nxTu(tu);  )
	{
		ZNR *zp = ZrB.p + tu->ownTi;
		if (zp->ztuCf)
			continue;					// speed
		if ( RELCHANGE( tu->aCv, tu->cv) > Top.relTol		// if flow changes .1% (or as relTol changed)
				||  ABSCHANGE( zp->aTz, zp->tz) > Top.absTol		// if zone temp changes .1 F (or as input) (shd be smaller?) 5-92
				||  ABSCHANGE( zp->aWz, zp->wz) > Top.absHumTol )	// if zone humidity changes .00003 (or as input or changed)
		{
			zp->ztuCf++;						// flag zone: say its terminals need recomputing
			Top.ztuKf++;						// say check for flagged zones
		}
	}
}		// AH::flagChanges
//-----------------------------------------------------------------------------------------------------------------------------
void AH::upCouple( DBL _ts, DBL tsWas, DBL _ws, DBL wsWas)

// update zone's coupling to OTHER AIRHANDLERS for specified ts / ws changes and flow changes

/* intended to speed convergence by adjusting aqO/bO of OTHER-AH terminals in this AH's zones for supply temp change,
   for other AH's done before ztuCompute next executed. */

/* believed will never be c-based changes as flow only changes for setpoint terminal, in setpoint mode
   ---> zn's other tu's have 0 a/b's.  But code present anyway. 3-92. Now needed for fan-limit c changes? */
{

	if ( fabs(_ts-tsWas) < .0001 * Top.absTol		// do not bother for 0 or VERY SMALL changes
			&&  fabs(_ws-wsWas) < .0001 * Top.absHumTol )	// .. VERY VERY SMALL !!
		return;

	for (TU *tu = NULL;  nxTu(tu);  )  ZrB.p[tu->ownTi].tuCz = tu->aCz(); 	/* temp put flows in ZONES for ez access here;
    										   get zone flow (after leaks) for vav box flow */
	// note that prior flows are in zp->tuCzWas.
	for (ZNR *zp = NULL;  nxZn(zp);  )			// loop ah's served zones, only!
	{
		for (tu = 0;  zp->nxTu(tu);  )			// loop zone's tus
		{
			if (tu->ai==ss)   continue;			// skip terminals on THIS airHandler
			if (tu->bO != 0.f)  				// skip ts for terminals with constant flow and tz in zn's current mode
			{
				DBL aqO = tu->aqO - zp->tuCzWas*tsWas + zp->tuCz*_ts;	/* adjust sigma(tu->cz*ts) for ts and/or (unexpected (??))
             								   cz change, using tu cz's TEMPORARILY stored in zones */
				DBL bO = tu->bO - zp->tuCzWas + zp->tuCz;   		// adjust sigma(cz) (plus more) for (unexpected) cz change
				if (aqO > 0.  &&  bO > 0.)					// paranoia (always some a/b from CAIR & UA, right?)
				{
					if ( RELCHANGE(aqO,tu->aqO) > Top.relTol		// if "significantly" changed
							||  RELCHANGE(bO,tu->bO) > Top.relTol )		// ..
					{
						zp->ztuCf++;					// flag zone to redo ztuCompute
						Top.ztuKf++;  				// tell hvacIterSubhr to check for flagged zones
					}
					tu->aqO = aqO;					// store even if did not flag: improve accuraccy of other AH's;
					tu->bO = bO;					/* ... believe won't get into zone results or cause imbalances
								       if ztuCompute not reexecuted. */
				}
			}
			if (tu->cO != 0.f)				// skip _ws for terminals on zones with no other flows (needed?)
			{
				DBL wcO = 0;				// sigma(tu->cz*ws) adjusted for this ah ws and/or cz change,
				switch (Top.humMeth)		// ...computed using average or latest ws according to humidity calculation method
				{
				case C_HUMTHCH_PHIL:
					wcO = tu->wcO - zp->tuCzWas*(wsWas + wsls)/2. + zp->tuCz*(_ws + wsls)/2.;	// average
					break;
				case C_HUMTHCH_ROB:
					wcO = tu->wcO - zp->tuCzWas*wsWas             + zp->tuCz*_ws;    		// latest
					break;
				}
				DBL cO = tu->cO - zp->tuCzWas + zp->tuCz;			// sigma(c) adjusted for (unexpected here) c change
				if (wcO > 0.  &&  cO > 0.)					// paranoia
				{
					if ( RELCHANGE(wcO,tu->wcO) > Top.relTol		// if "significantly" changed
							||  RELCHANGE(cO,tu->cO) > Top.relTol )		// ..
					{
						zp->ztuCf++;					// flag zone to redo ztuCompute
						Top.ztuKf++;  				// tell hvacIterSubhr to check for flagged zones
					}
					tu->wcO = wcO;					// store even if did not flag: improve accuraccy of other AH's
					tu->cO = cO;
				}
			}
		}
	}
}		// upCouple
//-----------------------------------------------------------------------------------------------------------------------------
void AH::upPriors()	// update (terminal) prior values for airHandler: called at ahCompute exit, 6-22-92.
{
	for (TU *tu = NULL;  nxTu(tu);  )  		// loop air air hander's terminals
	{
		ZNR *zp = ZrB.p + tu->ownTi;		// zone served by this terminal
		// if following change, cnztu.cpp:ztuMdSets flags ah to be CALLED so it can test whether OVERALL tr/wr/cr change > tolerance.
		tu->tzPr = zp->aTz;		// tu's zone's temp for which tu's ah last computed
		tu->wzPr = zp->aWz;		// hum likewise
		tu->czPr = tu->aCz();		// terminal flow (at zone, after leak) for which tu's ah last computed
		// if following change, cnztu.cpp:ztuMdSets may flag ah to be COMPUTED as ah entry tests may not check wzO.
		tu->wzOPr = tu->wzO;		// what zone hum wd be w/o this ah (znW(wcO/cO), for test in cnztu.cpp:ztuMdSets.
		//tu->tzOPr = tu->tzO;		// what zone temp wd be w/o this ah (0 or aqO/bO), ditto. NOT USED 10-92.
	}
}		// upPriors
//-----------------------------------------------------------------------------------------------------------------------------
/*
   ausz debugging notes 7-19-95... distribute or delete when fully resolved.
   1. When explicit min flow specified, tuVfDs went non-dynamic. fanF stayed 1.0, limiting flow spuriously.
   2. RESPONSE: dynamic fanFMax increase in antRatTs; setToMax( fanF, fanFMax) b4 ahVshNLims call, 7-18-95.
   3. worked for case at hand, but:
      aCM's * fanF change ratio caused spurious aCM INCREASE --> runaway flow for wrong dT in antRatTs.
   4. TWO PROPOSALS:
      A. carry aVF's as well as aCM's, so fanF can be correctly applied, non-limited vf--> limited c.
      B. Don't apply fanF when not fanLimited.
         a. don't apply lim in ahVshnLims when not fanLimited
         b. message if fanLimited && Top.tp_sizing && asFlow
         Like B: simpler, revert to A if necessary.
          THEN the dynamic fanFMax stuff (begHour, antRatTs) probably moot; probably COMMENT it thus and leave it.
          (If fanF were used, believe converges even if start too small... problem is spurious flow limit when not fanLimited.)
      C. Don't apply fanF to aCM's, keep rest the same
         a. take relative fanF out of relAdj in ahVshNLimes
         b. message in ahVshNLims if Top.tp_sizing & asFlow & fanLimited.
      DONE: C.

   7-22-95: dynamic upsizing & reversion to beg subhr values; aCM's removed; problem gone cuz vfM's & cM's always correspond.
*/
//-----------------------------------------------------------------------------------------------------------------------------
void AH::ahVshNLims(	// update supply air vsh and terminal/ah heat cap flow limits based on it

	DBL _ts,     	// (supply) temp at which to determine volumetric heat capacity of air for computing c's from vf's.
	// reference w (humidity ratio) established at setup is used.
	DBL &cIncr,  	// receives total sure + possible INCREASE in flows
	DBL &cDecr )	// receives total sure + possible DECREASE in flows

/* also does: increases max flow to min.
	      adjusts working copy flows (tu->aCv) that are set or mode-pegged to limits that are adjusted
			      (does not adjust pegged setpoint flows, cuz antRatTs always calculates). */

// uses fanF, old airxTs, .

// called by: AH::ahEstimate; AH::iter4Fs and callees, including places where fanF changed.
{
#if 1 // 7-19-95
// autoSizing fanLimited message. Unexpected, not coded & debugged to handle it, runaway probable.
	if ( Top.tp_sizing  &&  asFlow  &&  fanLimited )
		rer( "Program error: cnah2.cpp:AH::ahVshNLims(): \n    fanLimited found on during autoSizing."); 	// NUMS
#endif

	DBL cnv = Top.airX(_ts);				/* compute 60 * vsh @ ts, using ref w & pressure set up at startup.
							   Btuh/cfm-F: heat xfer of flow. */
	airxTs = cnv;					// store new cnv in member.

	DBL tFanF = this->fanF;				/* fetch flow reduction factor re fan overload.
							   >= fanFMax if fan not overloaded. (formerly argument, 4-95) */
	if (tFanF >= fanFMax)  setToMax( tFanF, fanFPr); 	// don't reduce limits if not reducing enuf to matter **
	fanFPr = tFanF;					// update comparator: remember fanF for which tu limits set.

// terminal loop

	for (TU *tu = NULL;  nxTu(tu);  )			// loop terminals for this airHandler
	{
#ifdef AUTOSIZE // cnglob.h 6-95
		float tuVfDs = tu->tuVfDs;				// RQD or defaulted in setup except if autoSizing
		if (tuVfDs==0)						// if 0 must be autoSizing
			tuVfDs = max( tu->tuVfMn, tu->tuVfMxH, tu->tuVfMxC);	/* default dynamically to largest of min, max flows:
								   These may be hourly exprs, or may be being set by autoSize.
								   0 should be ok at start autosize cuz in pass 1 ztuMode
								   increases flow/size as req'd regardless of limits. */
		DBL lim = tuVfDs * tFanF;				// limit to apply to this tu's cMn, cMxH, cMxC.
#else
x       DBL lim = tu->tuVfDs * tFanF;				// limit to apply to this tu's cMn, cMxH, cMxC.
#endif
		DBL m;   						// for new tu cMxH/cMxC/cMn value while comparing to old

		// terminal max (heat capacity) heating flow

		if (tu->cmAr & cmStH)					// if tu CAN set-temp heat, need heat limit, else skip: speed
		{
			m = cnv * min( max(tu->tuVfMxH, tu->tuVfMn), lim);	// htg tu max flow: min( max( Mn, Mx), fan-overload-derated design)
			if ( tu->useAr==uStH					// if tu IS NOW setpoint heating, and flow ..
					&&  (tu->aCv > m  ||  tu->aCv >= tu->cMxH) ) 	// now > new or >= old limit, flow WILL/MAY change
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;				// accumulate possible or sure flow increase
				else
					cDecr += m - tu->aCv;				// accumulate possible or sure flow decrease
			}							//	but if both old & new Mx > flow, no flag
			else if (tu->useAr==uMxH)				// if IS NOW heating at max in this zone mode
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;				// accumulate flow increase
				else
					cDecr += m - tu->aCv;				// accumulate flow decrease
				tu->aCv = m;					// flow WILL change to new Mx
			}
			tu->cMxH = m;						// done comparing, store result
		}

		// terminal max cooling (heat cap) flow

		if (tu->cmAr & cmStC)					// skip if does cmStH only, or cmSo, for speed
		{
			m = cnv * min( max( tu->tuVfMxC, tu->tuVfMn), lim);	// cooling max flow (new cMxC)
			if ( tu->useAr==uStC					// if tu is now setpoint cooling, and flow ..
					&&  (tu->aCv > m  ||  tu->aCv >= tu->cMxC) )   	// now > new or >= old limit, flow WILL/MAY change
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;
				else
					cDecr += m - tu->aCv;
			}
			else if (tu->useAr==uMxC)				// if cooling at max in this zone mode
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;
				else
					cDecr += m - tu->aCv;
				tu->aCv = m;   					// flow WILL change to new Mx
			}
			tu->cMxC = m;						// done comparing, store result
		}

		// terminal minimum (heat cap) flow

		if (tu->cmAr & (cmSo|cmStBOTH))				// don't need min limit if cmNONE
		{
#ifndef AUTOSIZE
x          m = cnv * min( (DBL)tu->tuVfMn, lim);    		// min flow
#else//6-95
			if (asFlow && Top.tp_pass1A)				// under flow-autoSizing part A const temp model
				m = 0;						// min flow is zero
			else
				m = cnv * min( (DBL)tu->tuVfMn, lim);    		// min flow (new cMn)
#endif
			if ( tu->useAr & (uStH|uStC)				// if now setpoint heating or cooling & flow ...
					&&  (tu->aCv < m  ||  tu->aCv <= tu->cMn) )   	// now < new or <= old limit, flow WILL/MAY change
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;
				else
					cDecr += m - tu->aCv;
			}
			//else if (tu->useAr & (uMxH|uMxC))			// if now operating at mode-pegged-max heat or cool
			//   stuff done above per cMxH/cMxC change
			else if (tu->useAr & (uSo|uMn))			// if terminal now operating at mode-pegged minimum or set output
			{
				if (m > tu->aCv)
					cIncr += m - tu->aCv;
				else
					cDecr += m - tu->aCv;
				tu->aCv = m;					// update flow to new limit
			}
			tu->cMn = m;						// done comparing, store result
		}

		// terminal fan backflow (still future, '95)
		if (tu->tfan.fanTy != C_FANTYCH_NONE)		// if terminal has fan, set backflow when off, else leave 0. 5-92.
		{
			tu->tfanBkC = cnv * tu->tfan.vfDs * tu->tfanOffLeak;
		}
	}
}			// AH::ahVshNLims
//-----------------------------------------------------------------------------------------------------------------------------
void AH::flagTus()		// set zp->ztuCf for each zone served by airHandler
{
	Top.ztuKf++;				// tell hvacSubhrIter to look for zones with ztuCf set
	for (TU *tu = NULL;  nxTu(tu); )		// loop terminals for ah
		ZrB.p[tu->ownTi].ztuCf++;
}				// AH::flagTus
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::nxTu(TU *&tu)		// first/next terminal for airHandler.  set tu NULL b4 1st call!
// usage:   for (TU *tu = NULL;  ah->nxTu(tu);  )  { ... }
{
	TI ui = tu ? tu->nxTu4a : tu1;
	if (ui)
	{
		tu = TuB.p + ui;
		return TRUE;
	}
	return FALSE;
}			// AH::nxTu
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::nxZn( ZNR *&zp)		// first/next zone served by (having a terminal connected to) airHandler

// returns FALSE if no (more) zones for airHandler.   Usage:  for (ZNR *zp = 0;  nxZn(zp);  )  { ... }
// CAUTION: don't add zones or otherwise alter ZrB.p between calls.

{
	// callers: cnah:upcouple, cncult5.cpp, .

// loop over zones / on reentry, start with zone AFTER last one returned

	for (zp = zp ? zp + 1 : ZrB.p+ZrB.mn;   zp <= ZrB.p + ZrB.n;   zp++)	// ** RLUP macro expanded & modified **
		if (zp->gud > 0)								// ..
		{
			TU *tu /*=NULL*/;
			for (TI ui = zp->tu1;  ui;  ui = tu->nxTu4z) 	// search zone's TERMINALs for one served by airHandler
			{
				tu = TuB.p + ui;
				if (tu->gud > 0)				// insurance
					if (tu->ai==ss)				// if TERMINAL is served by 'this' airHandler then ZONE is
						return TRUE;				// have first/next served zone
			}
		}
	return FALSE;					// no (more) zones for airHandler
}			// AH::nxZn
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::nxTsCzn( ZNR *&z)		// first/next ZN/ZN2/warmest zone/coolest zone supply temp control zone for ah


// usage: for (ZNR *z = NULL;  nxTsCzn(z);  )  {...}.   [do not call if !ISNCHOICE(ahTsSp).] caller: setTsSp1.
{
// if ZN or ZN2 control method, use zone of control terminal

	if (isZNorZN2)					// if ah is under ZN or ZN2 ts sp control this hour (set in begHour)
	{
		if (z)  						// if not first call
			return FALSE;					// there are no more control zones
		z = &ZrB.p[TuB.p[ahCtu].ownTi];			// point zone of control terminal
		return TRUE;
	}

// else WZ or CZ.  Choose appropriate array of control zone subscripts.

	TI *czns0 = CHN(ahTsSp)==C_TSCMNC_CZ  ?  ahCzCzns  :  ahWzCzns;	// if not cooling, assume is heating if here

// point to first or next zone per array contents.  z is NULL if first call.

	TI *czns;
	switch (*czns0)					// first array element is TI_ALL, TI_ALLBUT, or a zone subscript
	{
	case TI_ALL:
		return nxZn(z);			// all zones: return first/next zone for airHandler

	case TI_ALLBUT:
		while (nxZn(z))			// all zones but list: first, get first/next zone for ah
			for (czns = czns0 + 1;  ; czns++)	// search but-list for subscript of zone z
				if (*czns==z->ss)		// if czns subscript matches
					break;			// leave loop, continue outer loop to try next zone for ah
				else if (*czns <= 0)		// if end of but-list then
					return TRUE;			// z is not in but-list so it is a control zone
		return FALSE;				// if here, no more zones for ah

	default:           // list of zone subscripts; return first or next
		czns = czns0;				// init ptr to start of zone subscript list
		if (z)  				// NULL z means czns[0], else find z in czns then return next
			while (z->ss != *czns++) 		// next: search for match / point czns after matching subscript
				if (czns >= czns0 + 16)		// insurance: if beyond end
					return FALSE;			// say no more control zones. **Bug**. Issue msg if need found.
		if (*czns <= 0)  return FALSE;		// if no more zone subscripts in czns[]
		z = ZrB.p + *czns;			// point zone record for subscript of first or next zone in list
		return TRUE;
	}
}		// AH::nxTsCzn
//-----------------------------------------------------------------------------------------------------------------------------

// end of cnah2.cpp
