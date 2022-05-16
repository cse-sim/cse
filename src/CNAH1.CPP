// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnah1.cpp -- hvac airHandler simulation routines for CSE -- part 1

// see cnah2 for iter4Fs and callees -- guts of ah computation
// (not all in one file cuz RC complained of segment too big, 5-95)

// see cnztu for overall hvac and zone & terminals hvac simulation


// fanCycles check-laters (1992?): fan overload, coilLockout, any place frFanOn = 0 cd /0;
//       frFanOn to iter4Fs endtest? doesn't flow cover it? (maybe note there)
//       cnah convergence: drawthru: if fan heat changes do we need to loop back to doCoils?

/* 5-8-92 do-maybe-soon list
	return fan: check capacity / limit ah to match?  --> use smaller (current) cMx after Taylor check */
// undone item 4-92: does anything check rfan vfDs big enuf? ignored here.


/* Notes re AH ZN and ZN2 supply temp control methods, Rob 5-95:

   1. These are needed to simulate single zone constant volume systems.
      VAV is still allowed but could be removed (says Bruce 5-95) if
      hard-to-fix problems continue to be encountered. (As of 5-95, VAV
      ZN/ZN2 believed to work, but accurate simulation has been hard to
      achieve, especially when min vol is neither 0 nor same as max.

   2. Additional terminals are probably not disallowed on AH but are
      untested and not needed -- disallow when reason found. */

/* Notes re AH ZN and ZN2 control and zero possible flow, Rob 10-96:

   What if 0 ah sfan.vfDs results from autosizing (illegal as input)?
     Don't ever turn ah on: checked several places in cnztu.cpp: TU::tuEstimate, ZNR::ztuCompute, ZNR::ztuAbs.

   What if ah has flow but control terminal has no possible flow (legal as input, also could result from autoSizing)?
     10-96 code changes made several places in cnztu.cpp: TU::tuEstimate, ZNR::ztuCompute, ZNR::ztuAbs
       	to never set it to heating or cooling, but will still set to ahFAN for ZN2,
       	as though 0 scheduled or ausz'd tuVf values imply zone thermostat disabled.
     Alternative (more realistic?): let ah run anyway: remove some 10-96 cnztu.cpp tu flow checks,
        and at least in cnah2:setTsSp1() make the checks that shut down ah on zero tu->cz
        not do so if zone wants heat and flow is 0 only cuz of tuVfMxH/C/Mn values (details not worked out). */


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// USI LI dtypes.h cndefns.h

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// AH TU ZNR
#include <MSGHANS.H>	// MH_R1270

#include <HVAC.H>   // ashpConsistentCaps
#include <PSYCHRO.H>	// psyEnthalpy
#include <EXMAN.H>	// rer
#include <IRATS.H>	// AhiB

#include <CNGUTS.H>	// Decls for this file; ZrB MDS_FLOAT

//-------------------------------- OPTIONS ----------------------------------
#undef ONPLR		// define to use fan-on plr not average in coils autosizing.
// believe high average plr impossible eg for heat when cool has forced large fan.
// implies xPk stuff. Rob 6-97.  fixes BUG0089, 90, etc hc convergence problems.
// BUT: also currently controls reporting which must be based on co_plrAv.
// TRY5 in cncoil.cpp fixes problems without this. DELETE.

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

	/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
// none

	/******************************************************************************************************************************
	*  HVAC: AHs (AIRHANDLERS)
	******************************************************************************************************************************/
// class declaration including member fcn decls is in rccn.h, from app\cnrecs.def via rcdef.exe.
//-----------------------------------------------------------------------------------------------------------------------------
	/*   AIR HANDLER AUTOSIZING overview comments ---

	  AutoSize does pass 1 for each design day, then pass 2 for each design day, repeating same day until converged.
	  Pass 1 does part A for a design day until converged, then shifts to part B, CONTINUING same design day, until reconverged.

	  flags: .az_active: one for each autoSizable thing.
	         .asFlow: TRUE if autoSizing supply fan or ANY CONNECTED TERMINAL's flow: invokes supply temp/min flow model changes.

	  Pass 1 - find large enough sizes

	    thruout part: autoSized fans & coils: increase capacity as necessary.

	    Part A - open-ended simplified models, no capacity-scaled losses, constant supply temperatures, variable volume.

	       supply temp: WZ/CZ/ZN/ZN2 become constant ahTsDsH/C.
	       leaks/losses: no capacity-based leaks/losses: as tho oaOaLeak, oaRaLeak, ahSOLeak, ahROLeak all zero.
	       fans being autoSized - fan heat constant as tho 100% flow.
	       coils being autoSized - use "ELECTRIC" model.
	       terminal flows (here and cnztu.cpp): min 0; increase max's as required.

	    Part B - continuing same design day with real models, normal supply temp and leakage modelling

	  Pass 2 - measurements; reduce excess sizes. All in cnausz.cpp, no distinctions here (?).

	    all object models normal
	    each object measures its plr and load (for reports)
	    (future?) code reduces excess sizes and re-runs if necessary,
	      eg because part A const supply temp exagerates q of min flows, or lack of leaks increases load.

	  Notes
	      Autosizing supply fan or any connected terminal sets .asFlow to invoke in part A: constant supply temp,
	    vav terminals, no capacity-based leaks/losses, fan heat as tho flow max. If coil is being autoSized without
	    autoSizing fan or any tu, all the preceding remains normal, but coil nevertheless uses ELEC model in part A.

	      Objects NOT being autoSized use normal capacity (so can autoSize eg local heat) & models (although
	    in part A normal coil model can run under non-normal supply temps).

	    The deal on PLANTS, tentative 7-6-95, doing for HEATPLANT/HW lh/HW ahhc: coil being autosized acts like
	      infinite plant capacity available, during BOTH pass 1 and pass 2. Plant issues message in endAutoSize()
	      if peak demand exceeded capacity. Plant takes its course -- if mixed ausz/non load, non-ausz may get
	      disproportionately small q while ausz overloads plant...

	    Messages: issue message when plant overloaded. Not trying to detect all cases of tu/sfan/coil autoSizing
	    where a non-autosized tu/sfan/coil prevents proper sizing: eg not monitoring for ts trying to go out of range,
	    nor for volume runaway when coil-limited (hmm... but probably should limit the latter).
	*/
//-----------------------------------------------------------------------------------------------------------------------------
	// next 6 or so fcns out of order because they are (or once were) short:
//-----------------------------------------------------------------------------------------------------------------------------
	void AH::fazInit( int isAusz)		// airHandler beginning of phase initialization  6-95

// called once each for main simulation and autoSize phases
{
// init AUSZ substructs re variables that may be autoSized.  Note setup (cncult5.cpp) set .az_active's per user AUTOSIZE cmds.
	hcAs.fazInit(  &ahhc.captRat, FALSE, this, AH_AHHC+AHHEATCOIL_CAPTRAT, isAusz);	// heat coil
	ccAs.fazInit(  &ahcc.captRat, TRUE,  this, AH_AHCC+COOLCOIL_CAPTRAT,   isAusz);	// cool coil: captRat. negative quantity.
	fanAs.fazInit( &sfan.vfDs,    FALSE, this, AH_SFAN+FAN_VFDS,           isAusz);	// fan(s): sfanVfDs.

// other stuff: for main sim set _As & _AsNov variables not in AUSZ's when NOT being autoSized.
	// for convenience if probed, and ? for "sizing" report with no autoSizing phase.
	// here so set even if no autoSizing phase.
	if (!isAusz)
	{
		if (sstat[AH_AHCC+COOLCOIL_CAPSRAT] & FsVAL)		// if ahccCapsRat has value (not expr, not autoSizing, not omitted)
			ahcc.capsRat_As = ahcc.capsRat_AsNov = ahcc.capsRat;	// copy to places where autoSize would leave value
		if (sstat[AH_RFAN+FAN_VFDS] & FsVAL)			// if rfanVfDs has a value (not expr, not autoSizing, not omitted)
			rfan.vfDs_As = rfan.vfDs_AsNov = rfan.vfDs;		// copy to places where autoSize would leave value
	}

// other stuff: init autoSizing peaks not in AUSZ's
	if (isAusz)			// -As variables must last through main simulation for reports (6-95)
	{
		// clear autoSizing peak values: max value of same name without -As at the end of a converged pass 2 design day
		// primary init of the following is now in AH::ah_p2Init, 7-11-95
		qcPkSAs = qcPkLAs = 0;
		qcPkHAs = qcPkDAs = qcPkMAs = 0;
		qcPkTDbOAs = qcPkWOAs = 0;
		qcPkTenAs = qcPkWenAs = qcPkTexAs = qcPkWexAs = 0;
	}
}		// AH::fazInit
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::rddInit(int /*isAusz*/)	// init done ONCE for main sim and once for each autoSize design day: set inital state. 7-95.

// see also rddiInit, next: confusing names.   Called from cnguts.cpp:cgRddInit, 7-95
{
	tr2Nx = 	// initialize return air temp (used b4 1st ahCompute) to same value as zone temperatures.
				// If tr2Nx left 0, 1st ztuMode call has (had) trouble converging using estimated ah ts's that
				// don't relate to zones and which may be eg 0 for cooling but 40 (ahTsMn) for heating. 4-20-95
		ah_tSup = aTs = 70.f; 		// initing supply temp, eg re tu duct leakage to return, should speed warmup, 5-95.
	ws = aWs = Top.tp_refW;	// init supply humidity

	// set up coils and fans. For those to be autoSized, initial size may be 0 or NAN; will be reSetup elsewhere during pass 1.
	RC rc = ahhc.setup(this); 		// check/set up heat coil if needed 7-95
	rc   |= ahcc.setup(this); 		// check/set up cool coil if needed, eg set any derived inputs. 7-95.
	sfan.fn_setup2();			// set up supply fan derived members, 7-95
	rfan.fn_setup2();			// set up return fan derived members, 7-95

	// init reporting accumulators for DX coil supersaturated entering air correction, 5-97
	ahcc.xLGainYr = 0;			// Btu accumulator
	ahcc.nSubhrsLX = 0;			// counter

	return rc;
}			// AH::rddInit
//-----------------------------------------------------------------------------------------------------------------------------
void AH::rddiInit()	// initialization done ONCE for main sim or EACH DES DAY ITERATION for ausz: clear peaks. 6-95.

// see also rddInit, just above: confusing names.
// called from cnausz:asRddiInit after calling AUSZ::rddiInit's, 7-95.
#if 0 // do clear peaks each pass 1 des day iter for more accurate lkPkAs1 re sfan, 8-15-95 cnausz, cnah1, cnztu.cpp.
x // not called per des day iteration in pass 1, 7-95. Redundant calls occur.
#else
// redundant calls occur and must be harmless.
#endif
{
	// clear values set at time of peak cool coil load (max ccAs.ldPk) for run or des day
	qcPkS = qcPkL = 0;
	qcPkH = qcPkD = qcPkM = 0;
	qcPkTDbO = qcPkWO = 0;
	qcPkTen = qcPkWen = qcPkTex = qcPkWex = 0;
}						// AH::rddiInit
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_SetupSizes()   	// init object's models for possibly changed sizes being autoSized
// xx init for each ITERATION OF design day in autoSizing pass 1 part A

// called from cnausz.cpp:begP1aDsdIter, AH::ah_pass1AtoB, cnausz.cpp:begP2DsdIter, AH::ah_p2EndTest, etc
// after sizes possibly changed.
{
	RC rc = RCOK;

	if (hcAs.az_active)			// if autosizing heat coil
	{
		rc |= ahhc.reSetup(this);		// set coil up for current size (eg AUSZ::begP1aDsdIter changed it!)
		ahPtf++;					/* force full air handler computation at next call, cuz captRat
						   may be changed (perhaps to 0!), even tho load & all -Pr's may be same,
						   eg on a const-temp heat design day. */
	}

	if (ccAs.az_active)			// if autosizing cool coil
	{
		rc |= ahcc.reSetup(this);  		// set coil up for current size (eg AUSZ::begP1aDsdIter changed it!)
		ahPtf++;					// force full air handler computation at next call: captRat changed
	}

	if (fanAs.az_active)			// if autosizing fan(s)
	{
		sfan.fn_setup2();		// reinitialize fan model for this capacity

		if (asRfan)				// if autosizing return fan to match supply fan
		{
			rfan.vfDs = sfan.vfDs;		// set return fan size: has no separate AUSZ
			rfan.fn_setup2();			// reinitialize fan model for this capacity
		}
		ahPtf++;					// force full air handler computation at next call: vfDs's changed
	}

	// change flags note
	//ahClf++; Top.ahKf++;		ahEstimate sets these anyway at start subHour
	//ahPtf++;				set above if anything being resized.

	return rc;
}		// AH::ah_SetupSizes
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_pass1AtoB()	// call at transition from autoSize pass 1A to pass 1B for each design day

// called between iterations of a design day (between tp_SimDay()'s without reInit)
// at the change from const-temp size-finding open-ended models to the real models
// (between AUSZ::endP1a and AUSZ::begP1b fcns).
{
// at entry, values are largest part A values yet seen (.a).

// object-specific adjustments
	// here do optimizations eg DX reverse ARI computation to make value be a better estimate of rated size.
	// continuation will further adjust the value, both for model changes and interactions not yet manifest.

// on return, caller increases value to largest cvg'd part B value (.b) if larger, or updates .b if value is larger,
// then call begP1b (next).
	return RCOK;
}			// AH::ah_pass1AtoB
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_begP1b()		// call at beginning of pass 1 part B iterations for design day, after value set to max part B value seen
{
// reSetup models being autoSized, cuz ah_pass1AtoB above and/or autoSize code may have changed values.
	RC rc = ah_SetupSizes();	// just above

// change flags
	ahPtf++;			/* set flag to force full ah computation even if load & weather unchanged:
				   if autoSizing anything in ah (redundant: ah_setupSizes does),
				   if asFlow (ts change if sizing tu's),
				   if AH contains any coils (cuz part A may have run electric model only), ... ALWAYS! */
	//ahClf++; Top.ahKf++;	ahEstimate sets these anyway at start subHour
	if (asFlow)			// if part A used const-supply-temp models
		ahTsSpPr = -999;		// force ahEstimate to re-setTsSp1
	return rc;
}		// AH::ah_begP1b
//-----------------------------------------------------------------------------------------------------------------------------
void AH::ah_p2Init()		// init repeated before each "pass 2" pass thru design days: 0 peaks
{
	/* it is necessary to re-init peak info each pass because load can fall if item resized,
	   for example ZN/ZN2/WZ/CZ TU flow: ts set for 90% flow, flow can with tuVfMx til ts reaches limit.*/

// caller has already done p2Init() for AUSZ's: ldPkAs = plrPkAs = xPkAs = 0.

// other stuff:
	// init autoSizing peaks not in AUSZ's: each is max value of same name without -As at the end of a converged pass 2 design day
	qcPkSAs = qcPkLAs = 0;
	qcPkHAs = qcPkDAs = qcPkMAs = 0;
	qcPkTDbOAs = qcPkWOAs = 0;
	qcPkTenAs = qcPkWenAs = qcPkTexAs = qcPkWexAs = 0;
}							// AH::ah_p2Init
//-----------------------------------------------------------------------------------------------------------------------------
void AH::endP2Dsd()	// called at end pass 2 design day, after converged. records new high sizes, loads, & plr's not in AUSZ's.
{
// record non-AUSZ-member autosizing peak values. plrPkAs and xPkAs set in AUSZ::endP2Dsd.
	if (ccAs.plrPk >= ccAs.plrPkAs)	// if this design day had new autosizing peak cool coil part load ratio
	{
		// >= needed cuz already copied in AUSZ::endP2Dsd.
#if 1 // xPk stuff, 6-97
		ccAs.xPkAs = ccAs.xPk;		// just in case not copied in AUSZ::endP2Dsd (plrPk==plrPkAs)
#endif
		qcPkSAs =   qcPkS; 		// record the associated sensible peak even if not itself a new maximum
		qcPkLAs =   qcPkL; 		// .. cool coil latent peak ..
		qcPkHAs =   qcPkH; 		// time of peak: hour 1-24,
		qcPkDAs =   qcPkD; 		//  day 1-31, unused for design day
		qcPkMAs =   qcPkM; 		//  month 1-12, 0 for heat design month
		qcPkTDbOAs= qcPkTDbO; 		// outdoor drybulb
		qcPkWOAs =  qcPkWO; 		//  hum rat
		qcPkTenAs = qcPkTen; 		// coil entering air drybulb
		qcPkWenAs = qcPkWen; 		//  hum rat
		qcPkTexAs = qcPkTex; 		// exiting
		qcPkWexAs = qcPkWex; 		//  ..
	}
}				// AH::endP2Dsd
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_p2EndTest()		// autoSize pass 2 end test

// if any plr too small, adjust corresponding size and set Top.notDone to invoke repetition of pass
// (in addition, caller invokes another pass if any quantity increased)
{
// in addition to endtest, 7-11-95:
// if fan not overrun on pass 1
// then pass 1 pk demand not applicable to real models & should not cause warning in ah_endAutosize.

	if (fanAs.ldPkAs < sfan.vfDs * Top.loTol)	// dubious test for marginal cases, especially vfDsMxF==1.0.
    											// better to monitor min fanF at end subhour?
		setToMin( fanAs.ldPkAs1, fanAs.ldPkAs);


// endtest: if any plr too small, adjust corresponding size and set Top.notDone to invoke repetition of pass

	/* oversize sizes can result from the modified models used in pass 1 part A, particularly if AH::asFlow. Examples:
	   1. fan heat always max'd would make cooling coil oversize if coil peak wasn't at flow peak.
	   2. min (eg ventilation) flow on cooling terminal on WZ ah may have exagerated effect under fixed atTs
	      in pass 1 part A, resulting in oversize heating facilities for the zone. */

	// hmm.. if fan & coil both oversize, ideally we'd look at peak Ts vs ahTsDsH/C to decide which to downSize how much. 7-95.

	float auszLoTol  = 1.f - Top.auszTol;		// 1 - autoSizing tolerance: subexpr used multiple places below.

	// heating coil
	if ( hcAs.az_active				// if autosizing heat coil
			&&  ahhc.captRat				// and cap is non-0: else unused (and would divide by 0)
			&&  hcAs.plrPkAs				// divide by 0 protection
			&&  hcAs.plrPkAs < auszLoTol 	// if heat coil's part load ratio < 1 - tolerance
#if 1 // xPk stuff, 6-97
			&&  hcAs.plrPkAs * hcAs.xPkAs/ahhc.captRat < auszLoTol )	/* .. even when plr mapped to current size, in case model
								      REDUCES size or size smaller last day, 6-97 */
#endif
	{
		Top.notDone++;						// plr too small. say must repeat pass 2.
		DBL was = ahhc.captRat;					// for debugging message, rob 6-97
#if 1 // xPk stuff, 6-97
#if 1 // former TRIAL, 6-97. or 5/6 like cc and fan?
		float reducedCap = hcAs.xPkAs * (hcAs.plrPkAs + 1.f) / 2.f;
		setToMin(ahhc.captRat, reducedCap);	// reduce (precaution) cap to bring plr half way to 1
											// (just half way in case relation between capt & plr not linear)
#else // historical, but many convergence problems 6-97 with oversize fan (BUG0089,90).
x      setToMin( ahhc.captRat, 					// reduce (precaution) cap to bring plr to 1 - half of tolerance
x            hcAs.xPkAs * hcAs.plrPkAs/(1 - .5*Top.auszTol) );	// (presumes relation between capt & plr is linear for ahhc)
#endif
#else
x #ifndef TRIAL // historically good code (pre condensation), 6-97
x       ahhc.captRat *= hcAs.plrPkAs/(1 - .5*Top.auszTol);	/* adjust cap to bring plr to 1 - half of tolerance
x     								   (expect relation between capt & plr is linear for ahhc). */
x #elif 1
x       /* despite the fact that relation between capt & plr should be linear, cases (BUG0089 (condensation), BUG0090)
x          have been observed where reduction per plr just causes another increase. See if less aggressive adjustment helps. */
x       ahhc.captRat *= (hcAs.plrPkAs+1)/2;	// adjust cap to bring plr half way to 1
x
x #else // Bruce found a convergence problem in which ausz pass 2 hc (and fan, but less) keep thrashing. Try... 6-9-97
x *       /* despite the fact that relation between capt & plr should be linear, cases (Bruce, condensation, BUG0089)
x *          have been observed that thrash -- increase for an iteration or 2, then get 20% smaller.
x *          See if less aggressive adjustment helps. */
x *       // try 1/2 adjustment. Helped in the one case... not fully tested... undone to pursue what may be real problem first.
x *       ahhc.captRat *= (hcAs.plrPkAs+1)/(2 - Top.auszTol);	// adjust cap to bring cap half way to 1 - half of tolerance
x *
x *       // or try - see ahcc commments below for this:
x *       //ahhc.captRat  *= 6 * hcAs.plrPkAs / (5 - 2*Top.auszTol + hcAs.plrPkAs);
x #endif
#endif
		if ( Top.verbose > 2 && !Top.notDone 		// at verbose = 3 show the first not done thing, rob 6-97
				||  Top.verbose > 3)				// at verbose = 4 show all not dones
			screen( 0, "      ah[%d] p2EndTest  hc  plr%6.3g  capt%8g -->%8g", ss, hcAs.plrPkAs, was, ahhc.captRat);
	}

	// cooling coil: may be limited either by sensible or latent portion of its capacity
	if ( ccAs.az_active				// if cooling coil being autoSized
			&&  ahcc.captRat				// and it has been given a non-0 size (else unused)
			&&  ahcc.capsRat				//   divide-by-0 protection
			&&  ccAs.ldPkAs				// and a non-0 load has been found: divide-by-0 protection
			&&  ccAs.plrPkAs				// and its plr has been found non-0: divide-by-0 protection
			&&  ahcc.SHRRat && 1 - ahcc.SHRRat )		// yet more divide-by-0 insurance
	{
#if 0//SHR let elec coil be oversized
x       float plrs = ccAs.plrPkAs * qcPkSAs/ccAs.ldPkAs / ahcc.SHRRat;   	// plr * sensible load fraction / sens cap frac
x       float plrl = ccAs.plrPkAs * qcPkLAs/ccAs.ldPkAs / (1 - ahcc.SHRRat);	// plr * latent load fraction / latent cap frac
#else//7-21-95
		float plrs  = ccAs.plrPkAs * (qcPkSAs/ccAs.ldPkAs) / (ahcc.capsRat/ahcc.captRat);	// sensible part load ratio
		//  plr      *  sens load fraction   /  sens cap frac
		float plrl = ahcc.captRat <= ahcc.capsRat			// don't /0 if no latent cap ("electric" coil), but let
					 ?  (qcPkLAs ? 1 : 0)				// any (unexpected) load mean non-existent cap fully used
						 :  ccAs.plrPkAs * (qcPkLAs/ccAs.ldPkAs) / ((ahcc.captRat - ahcc.capsRat)/ahcc.captRat);
		//  plr      * latent load fraction  / latent cap frac
#endif
		float plr = max( plrs, plrl, 		 			// use largest of sensible & latent part load ratios
						 ccAs.plrPkAs );					// overall plr included as insurance
		if ( plr < auszLoTol		 	// if cool coil's part load ratio < 1 - tolerance
#if 1 // xPk 6-97
				&&  plr * ccAs.xPkAs / ahcc.captRat )	/* .. even when plr refigured for current size, in case smaller last des day
						   or model REDUCES size, rob 6-97 */
#endif
		{
			Top.notDone++;
			DBL was = ahcc.captRat;					// for debugging message, rob 6-97

			// cool coil non-linear: reducing cap may make coil colder, increasing latent load for same sensible output.
			// so make only part of large changes, as re-increase may require TWO complete passes thru des days, 7-95.
			// Let's correct 5/6  of the way to  1 - tol/3, eg:
			//  plr--          new plr if linear--    variation in actual change b4 another iteration--
			//    1 - tol            1 - .5*tol          +- 100%        tolerates much variation
			//    1 - 2*tol          1 - .67*tol         -25% to +50%   favors > than linear change
			//    1 - 4*tol          1 - tol             -0%  to +33%   expects >= linear
			//    1 - 10*tol         1 - 2*tol           +12% to +25%   large changes, expect > than linear
			//    1 - 22*Tol         1 - 4*tol           ..             up to here, 1 more iteration completes if >= linear.
			// To make new plr be       1 - tol/3 - (1 - plr)/6
			//   (if linear), multiply captRat by plr/that:
			//  captRat   *=     plr / (1 - tol/3 - (1 - plr)/6)
			//  simplify  *= 6 * plr / (6 - 2*tol -  1 + plr)
			//  ..        *= 6 * plr / (5 - 2*tol      + plr)
			//
#if 1 // xPk 6-97
			setToMax( ahcc.captRat, 						// reduce (precaution) NEGATIVE captRat
				   ccAs.xPkAs * 6 * plr / (5 - 2*Top.auszTol + plr) );	// as derived just above
#else
x          ahcc.captRat  *= 6 * plr / (5 - 2*Top.auszTol + plr);
#endif
			if ( Top.verbose > 2 && !Top.notDone 		// at verbose = 3 show the first not done thing, rob 6-97
					||  Top.verbose > 3)				// at verbose = 4 show all not dones
				screen( 0, "      ah[%d] p2EndTest  cc  plr%6.3g  capt%8g -->%8g", ss, ccAs.plrPkAs, was, ahcc.captRat);
		}
	}

	// fan(s)
	if ( fanAs.az_active				// if autosizing supply fan (and perhaps return fan)
			&&  sfan.vfDs 				// if fan capacity is non-0: else unused ah and would divide by 0
			&&  fanAs.plrPkAs				// and plr is non-0: divide by 0 protection
			&&  fanAs.plrPkAs < auszLoTol 				// if fan's part load ratio < 1 - tolerance
#if 1 // xPk stuff, 6-97
			&&  fanAs.plrPkAs * fanAs.xPkAs / sfan.vfDs < auszLoTol )	// .. even when adjusted to current size, in case model
																		//    REDUCED size or last des day size smaller, 6-97
#endif
	{
		Top.notDone++;				// fan plr too small. say must repeat pass 2.

		DBL was = sfan.vfDs;				// for debugging message, rob 6-97
		// effect of fan capacity on its peak load might be non-linear eg because of fan heat curve, so make smallish change.
		// correct 5/6 of the way to  plr = 1 - tol/3  as derived above for ahcc.captRat:
#if 0
x       sfan.vfDs *= 6 * fanAs.plrPkAs / (5 - 2*Top.auszTol + fanAs.plrPkAs);
#else // xPk stuff, 6-97
		setToMin( sfan.vfDs, 								// ceil not = is a precaution
			  fanAs.xPkAs * 6 * fanAs.plrPkAs / (5 - 2*Top.auszTol + fanAs.plrPkAs) );
#endif
		if ( Top.verbose > 2 && !Top.notDone 		// at verbose = 3 show the first not done thing, rob 6-97
				||  Top.verbose > 3)				// at verbose = 4 show all not dones
			screen( 0, "      ah[%d] p2EndTest fan  plr%6.3g  vfMx%8g -->%8g", ss, fanAs.plrPkAs, was, sfan.vfDs );

		if (asRfan)				// if also autosizing return fan
			rfan.vfDs = sfan.vfDs;		// set its capacity to that of supply fan
	}

// reinitialze models for possibly changed sizes
	RC rc = ah_SetupSizes();	// mbr fcn above. Believe redundant -- begP2DsdIter also calls.

	return rc;
}			// AH::p2EndTest
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_endAutosize()			// airHandler stuff at end successful autosize  6-95
{

// warn if ah cfm exceeded supply fan capacity during autoSizing

	float pk = max( fanAs.ldPkAs1, fanAs.ldPkAs);		/* max of pass 1 and regular pass 2 peak.
								   Currently (7-6-95) it's ldPkAs1 that can exceed sfan.vfDs, cuz
								   asFlow && pass1 let fan overrun when sizing tu's & not fan.*/
	if (pk > sfan.vfDs * sfan.vfMxF * Top.hiTol) 		// if peak supply fan cfm exceeded rated size times overrun
#if 0 //8-17-95 believe no longer needed with temp for cfm correction in AH::endSubhr, 8-16-96. Restore if need found.
xx     &&  asFlow )						/* and autoSizing flow -- else not interested
xx								   (and have observed excess flows > hiTol, 7-95) */
#endif
	{
		oWarn( "Peak air flow during autoSizing (%g) exceeded %s\n"
			   "        supply fan design flow (%g) times overrun (%g) = %g.\n"
			   "    For your autoSized coils and/or terminals to be effective,\n"
			   "        you may need at least  sfanVfDs = %g .",
			   pk,   fanAs.az_active ? "" : "(non-autoSized)",		// expect always non-autoSized
			   sfan.vfDs,   sfan.vfMxF,  sfan.vfDs * sfan.vfMxF,
			   pk/sfan.vfMxF );

#if 0//msgs occurred spuriously b4 8-16-95; compiler assert macro does not terminate DDE right.
xx       //assert(asFlow);			claimed can only happen if autoSizing flow -- thus tu(s). NOT TRUE for tol used.
xx       /*assert(!fanAs.az_active);			claimed can only happen if not autoSizing supply fan:
xx        * 					 NOT TRUE for tol=Top.hiTol, BUG0065, 8-15-95. */
#elif 1//believe spurious messages fixed by using correct temp for fan cfm in endSubhr, 8-16-95.
		//tentatively restore checks, using oWarn to let run complete.

		if (!asFlow)
			oWarn( "INTERNAL ERROR: INCONSISTENCY:\n"
				   "    Peak supply fan cfm exceeded design cfm times overrun\n"
				   "    when neither supply fan nor any connected terminal cfm being autosized" );
		if (fanAs.az_active)
			oWarn( "INTERNAL ERROR: INCONSISTENCY:\n"
				   "    Peak supply fan cfm exceeded design cfm times overrun\n"
				   "    when supply fan being autosized" );

#else// could use later when established that assertions do not usually fail: shorter; fatal error insures message noticed.
*       ASSERT(asFlow);				// claim can only happen if autoSizing flow -- thus tu(s)
*       ASSERT(!fanAs.az_active);			// claim can only happen if not autoSizing supply fan
#endif
	}

#if 1 //8-28-95
// warn if DX coil rating flow too different from supply fan design flow, cuz DX model not good way out of range
	if (ahcc.coilTy==C_COILTYCH_DX)				// if DX cooling coil
	if ( !( sstat[AH_SFAN+FAN_VFDS] & FsVAL			// not if checked at initialization in dxSetup()
				&&  sstat[AH_AHCC+COOLCOIL_CAPTRAT] & FsVAL ) )	// .. (reverse of test there)
			if ( ahcc.vfR						// not if no rated flow (eg if no cooling dmd, coil never set up)
					&&  ahcc.captRat )					// .. or no capacity (10-96, cuz vfR seen nz when 0 dmd)
		{
			BOO vfRgiven = sstat[AH_AHCC+COOLCOIL_VFR] & FsSET
							   || sstat[AH_AHCC+COOLCOIL_VFRPERTON] & FsSET;	// nz if ahccVfR or ahccVfRperTon given
				if (sfan.vfDs > 1.501 * ahcc.vfR)					// if sfan rating cfm 50% > DX coil rating cfm
					oWarn( "%ssupply fan design flow (%g) exceeds \n"
						   "    %sDX coiling coil rating air flow (%g) by more than 50%.\n"
						   "    CSE's DX coil model may not be valid for such excessive flows.",	// NUMS
						   fanAs.az_active ? "autoSized " : "",  sfan.vfDs,
						   vfRgiven ? "" : "defaulted ",     ahcc.vfR );
#if 0
x				else if ( 1.201 * ahcc.vfR < sfan.vfDs
x                 ||  ahcc.vfR > 1.201 * sfan.vfDs )				// if sfan rating cfm 20% different from DX coil's
x					oWarn( "%scoiling coil rating air flow (%g) differs \n"
x					 "    from %ssupply fan design flow (%g) by more than 20%.",		// NUMS
#else // 5-97: increase 20% tolerance to 25%, per Taylor via Bruce. cnah1.cpp and cncoil.cpp.
				else if ( 1.251 * ahcc.vfR < sfan.vfDs
						  ||  ahcc.vfR > 1.251 * sfan.vfDs )				// if sfan rating cfm 25% different from DX coil's
					oWarn( "%scoiling coil rating air flow (%g) differs \n"
						   "    from %ssupply fan design flow (%g) by more than 25%%.",		// NUMS
#endif
										  vfRgiven ? "" : "defaulted ",     ahcc.vfR,
										  fanAs.az_active ? "autoSized " : "",  sfan.vfDs );
			}
#endif

	// this is a general message; could show after run
	if (ahcc.minTLtd)
		oWarn( "Cool coil output was limited by minimum coil temp (ahccMinTEvap=%g) \n"	// NUMS
			"    before meeting the minimum supply temperature (ahTsMn=%s).\n"		// %s intended
			"    You may want to give a higher ahTsMn or a lower ahccMinTEvap.",
		   ahcc.minTEvap,
		   (sstat[AH_AHTSMN] & FsVAL) ? strtprintf("%g",ahTsMn) : "<variable>" );

	if (ahcc.cfm2Few)
		oWarn( "When autosizing cooling coil without autosizing supply fan & terminals flows,\n"
			   "    there was not enough air flow to permit sizing the cooling coil \n"
			   "    to meet the load at the minimum coil temperature (ahccMinTEvap=%g).\n"
			   "    Increase sfanVfDs and/or terminal tuVfMxC's.",					// NUMS
			   ahcc.minTEvap );
	return RCOK;
}			// AH::ah_endAutosize
//-----------------------------------------------------------------------------------------------------------------------------
void AH::ah_auszFinal() 			// called after autoSize to complete & store results

{
	// called from cnausz.cpp:cgAuszI()

	/* Copy autoSizing results to:
	   1) member of same object with same name + _As.
	      This member has minimal variation to facilitate probing for main simulation input.
	   2) before multiplying by oversize, copy to member with same name + _AsNov, for use in "sizing" report.
	      This value is consistent with the other report columns, that show peak load etc during autoSizing.
	   3) INPUT RECORDS, so they will appear in regenerated runtime records used for main simulation.
	      (if no main sim requested, input records may already have been deleted).

	   Note if NOT autoSized, fazInit's set _As and _AsNov from constant x's so
	   happens if no ausz phase, and so get 0 not last value for expr. */

	AH* ahi = (ss <= AhiB.n) ? AhiB.p + ss	// point to input record for this runtime record
			  : NULL;		// if ah's subscript invalid, record is gone.

	// AHP cooling/heating coil capacity consistency
	if (ahhc.coilTy == C_COILTYCH_AHP)
	{
		if (ccAs.az_active && hcAs.az_active)
		{
			// Adjust for sizing factors
			float absAdjCoolCap = -ahcc.captRat*ah_fxCapC; // Cooling = negative, make positive
			float absAdjHeatCap = ahhc.captRat*ah_fxCapH;
			ashpConsistentCaps( absAdjCoolCap, absAdjHeatCap, IsSet(AH_AHHC + AHHEATCOIL_CAPRAT9547), ahhc.capRat9547);
			// Remove sizing factors since they will be applied again in AUSZ::final()
			ahcc.captRat = -absAdjCoolCap/ah_fxCapC;
			ahhc.captRat = absAdjHeatCap/ah_fxCapH;
		}

	}

// member function sets _As, _AsNov, and input record members for each AUSZ's 'x'.
	hcAs.final( this, ahi, ah_fxCapH);
	ccAs.final( this, ahi, ah_fxCapC);
	fanAs.final( this, ahi, ah_fxVfFan);

// additional stuff: cool coil capsRat: equal to captRat * SHRRat etc per coil model, no separate AUSZ substruct.
	if (ccAs.az_active)				// if autoSized cool coil, ahcc.capsRat was autoSized too.
	{
		ahcc.capsRat_AsNov = ahcc.capsRat;	// save non-oversized copy (for report, probes)
		ahcc.capsRat *= ah_fxCapC;			// apply oversize factor
		ahcc.capsRat_As = ahcc.capsRat;		// save copy of size, including oversize
	}

// additional stuff: return fan: same size as supply fan, no separate AUSZ substruct.
	if (asRfan)					// if autoSized return fan
	{
		rfan.vfDs_AsNov = rfan.vfDs;	// save non-oversized copy (for consistency; not in report 7-95).
		rfan.vfDs *= ah_fxVfFan;		// apply oversize factor
		rfan.vfDs_As = rfan.vfDs;		// save copy of size, including oversize
	}

// additional stuff: copy to input record, to persist thru re-setup for main sim
	if (ahi)					// if input record still exists
	{
		if (ccAs.az_active)					// if autoSized cool coil, ahcc.capsRat was autoSized too.
		{
			ahi->ahcc.capsRat = ahcc.capsRat;   		// copy ahcc.capsRat stuff
			ahi->ahcc.capsRat_As = ahcc.capsRat_AsNov;	// ..
			ahi->ahcc.capsRat_AsNov= ahcc.capsRat_AsNov;	// ..
		}
		if (asRfan)					// if autoSized return fan -- else don't overwrite poss input exprs, etc
		{
			ahi->rfan.vfDs = rfan.vfDs;			// copy rfan.vfDs stuff
			ahi->rfan.vfDs_As = rfan.vfDs_AsNov;		// ..
			ahi->rfan.vfDs_AsNov = rfan.vfDs_AsNov;	// ..
		}
		// save info about time of and load at peak autoSizing cool coil load (latter itself is in AUSZ, copied by .final above)
		ahi->qcPkSAs =   qcPkSAs;	// cool coil sens load @ total load peak
		ahi->qcPkLAs =   qcPkLAs;	// .. latent ..
		ahi->qcPkHAs =   qcPkHAs;  	// time of peak: hour 1-24,
		ahi->qcPkDAs =   qcPkDAs;  	//  day 1-31, unused for design day
		ahi->qcPkMAs =   qcPkMAs;  	//  month 1-12, 0 for heat design month
		ahi->qcPkTDbOAs= qcPkTDbOAs;  	// outdoor drybulb
		ahi->qcPkWOAs =  qcPkWOAs;  	//  hum rat
		ahi->qcPkTenAs = qcPkTenAs;  	// coil entering air drybulb
		ahi->qcPkWenAs = qcPkWenAs;  	//  hum rat
		ahi->qcPkTexAs = qcPkTexAs;  	// exiting
		ahi->qcPkWexAs = qcPkWexAs;  	//  ..
	}
}			// AH::ah_auszFinal
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::begSubhr()			// airHandler stuff done at start of subhour
{
	timesReEst = 0;			// zero counter used in ztuCompute to prevent inf lup repeatedly re-ahEstimating

	memset( &AhresB.p[ss].S, 0, sizeof(AHRES_IVL_SUB) );	// zero ah results subhour record.  same subscript as ah.

	return RCOK;
}			// AH::begSubhr
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::endSubhr() 		// airHandler stuff done at end subhr
{

// calculate actual coil output, and input power
	coilsEndSubhr();				// sets cch/ahhc/ahcc .p/.q, and other members for probes/results.  cncoil.cpp.

#ifdef AUTOSIZE//cnglob.h
// --- record converged load and part load ratio peaks, for autoSizing and for loads reports.

	if (Top.tp_autoSizing || !Top.isWarmup)	// don't let main sim warmup exagerate main sim results
	{

		// heat coil
		if (ahhc.coilTy != C_COILTYCH_NONE)	// if no heat coil, initial value may indicate capability absent
		{
			setToMax( hcAs.ldPk, ahhc.q);		// record new high peak load (Btuh coil output). set in cncoils.cpp:coilsEndSubhr.
#ifdef ONPLR	// believe must use fan-on plr for when frFanOn < 1 forced eg by fan larger for cooling. 6-97.
*           if (ahhc.plr > hcAs.plrPk)		// if new high part load ratio (fraction current capacity).
*		    {          					//   use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
*						//   use fan-on plr cuz eg fan sized for cooling can force frFanOn < 1. Rob 6-97.
*				 hcAs.plrPk = ahhc.plr;		// record the new high plr
*				hcAs.xPk = ahhc.captRat;		// record the capacity of which it is a fraction, 6-97
*			}
#elif 1 // xPk, 6-97. No magic improvement, but harmless.
			if (ahhc.co_plrAv > hcAs.plrPk)		// if new high part load ratio (fraction current capacity).
			{
				//   use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
				/*   using co_plrAv forces barely large enuf coil
				     (makes frFanOn approach 1 even when cooling-sized fan is oversize for heat),
				     but has had a history of convergence problems, 6-97. */
				hcAs.plrPk = ahhc.co_plrAv;		// record the new high plr
				hcAs.xPk = ahhc.captRat;		// record the capacity of which it is a fraction, 6-97
			}
#elif 1//7-95
x			setToMax( hcAs.plrPk, ahhc.co_plrAv);	/* .. part load ratio (fraction current capacity).
x	          				   use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time */
#else//7-21-95 Must use coil-on plr for correct ZN operation. Try reverting.
x			setToMax( hcAs.plrPk, ahhc.plr); 	/* .. part load ratio (fraction current capacity).
x	          				   use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
x	          				   Using fan-on plr for correct fanCycles operation: pass 2 may not cvg w co_plrAv.*/
#endif
		}
		// cool coil
		if (ahcc.coilTy != C_COILTYCH_NONE)	// [if no heat coil, initial value may indicate capability absent]
		{
#ifdef ONPLR	// believe must use fan-on plr for when frFanOn < 1 forced eg by fan larger for cooling. 6-97.
*			if (ahcc.plr > ccAs.plrPk)		// if a new peak plr, remember associated load and time info
*			{            					// note use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
*						//   use fan-on plr cuz eg fan sized for heat might force frFanOn < 1. Rob 6-97.
*				ccAs.plrPk = ahcc.plr;		//   part load ratio peak value (curr total load / cap'y @ current conditions)
*  #if 1 // xPk, 6-97
*				ccAs.xPk = ahcc.captRat;		//   record capacity plr relates to in AUSZ member, 6-97
*  #endif
#elif 1//7-95
			if (ahcc.co_plrAv > ccAs.plrPk)		// if a new peak plr, remember associated load and time info
			{
				//   note use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
				/*   using co_plrAv forces barely large enuf coil (makes frFanOn approach 1),
				     and is necessary to save correct info for AHSIZE report,
				     but has had a history of convergence problems (re heat), 6-97. */
				ccAs.plrPk = ahcc.co_plrAv;		//   part load ratio peak value (curr total load / cap'y @ current conditions)
#if 1 // xPk, 6-97
				ccAs.xPk = ahcc.captRat;		//   record capacity plr relates to in AUSZ member, 6-97
#endif
#else//7-21-95 Must use coil-on plr for correct ZN operation. Try reverting.
x				if (ahcc.plr > ccAs.plrPk)		// if a new peak plr, remember associated load and time info
x            		// note use .plr fan-on or .co_plrAv=plr*frFanOn time-average incl fan-off time.
x			{
x	          	/* Using fan-on plr for correct fanCyles autoSizing:
x	          	   pass 2 may not converge w co_plrAv, 7-21-95 (used plr b4 today). */
x					ccAs.plrPk = ahcc.plr;		//   part load ratio peak value (curr total load / cap'y @ current conditions)
#endif
			ccAs.ldPk = ahcc.q;		//   total load at time of peak plr (negative)
			qcPkS = ahcc.qs;			//   .. sensible ..
			qcPkL = ahcc.ql;			//   .. latent ..
			qcPkH = Top.iHr+1;			// time of peak: hour 1-24,
			qcPkD = Top.tp_date.mday;		//  day 1-31, not output for design day
			qcPkM = Top.tp_date.month;		//  month 1-12, 0 for heat design month
			qcPkTDbO = Top.tDbOSh;		// outdoor drybulb
			qcPkWO = Top.wOSh;   		//  hum rat
			qcPkTen = ten;			// coil entering air drybulb
			qcPkWen = wen;			//  hum rat
			qcPkTex = tex;			// exiting, before mixing with bypass air
			qcPkWex = wex;			//   ..
		}
	}
	// fans. Supply fan always present.
#if 0
x	if (airxTs)				// heatCap/cfm @ supply temp: might be 0 if ah scheduled off since start run
x	{
x	float vf = cmix / airxTs;		// (supply) fan load: flow out of economizer. convert to cfm @ supply temp.
#else // use temp at FAN!, 8-15-95
	  // TESTED changes peak flow & fan size in some runs, eliminates spurious cfm > vfDs messages in ah_endAutosize().
		if (sfan.t)				// /0 insurance, eg if ah scheduled off since start run (believed redundant).
		{
			float vf = cmix / Top.airX(sfan.t);	// (supply) fan load: flow out of economizer. convert to cfm @ temp at fan.
#endif
			setToMax( fanAs.ldPk, vf);		// record new peak fan load, cfm
			if (sfan.vfDs)			// supply fan cfm: might be 0 if ah hasn't gone on yet this run
#if 0
x				setToMax( fanAs.plrPk, vf/sfan.vfDs);	// record new fan peak fan part load ratio
#else // xPk stuff, 6-97
				if (vf/sfan.vfDs > fanAs.plrPk)	// if a new high fan peak part load ratio (fraction capacity used)
				{	fanAs.plrPk = vf/sfan.vfDs;	// record high plr
					fanAs.xPk = sfan.vfDs;		// record capacity it relates to
				}
#endif
		}
	}
#endif


// --- airHandler subhour results record ---   used for: see accumulation in cnguts.cpp, reporting in cgresult.cpp.

	AHRES_IVL_SUB *ahres = &AhresB.p[ss].S;   	// subhour part of ah results record: same record subscript as ah.
	if (ahMode != ahOFF)			// if on: output no non-0 results when ah off, except certain aux energies (below)
	{
		ahres->hrsOn = Top.tp_subhrDur;			// accumulate time on for weighting averages

		// temps and humRats (wetBulbs may be computed when/if formatted to print) and flows
		ahres->tDbO = Top.tDbOSh;
		ahres->wO = Top.wOSh;	// outdoors (for convenience in generating report)
		ahres->tr = tr;
		ahres->wr = wr;     	// return (after plenum, before duct).  Use inputs to completed iteration.
		ahres->tmix = tmix;
		ahres->wmix = wen;	// mixed
		ahres->ts = ah_tSup;
		ahres->ws = ws;     	// supply (after supply duct, at terminals)
		ahres->po = po;     				// fraction outside air
		ahres->frFanOn = frFanOn;			// fraction of time fan ran (1.0 unless fanCyles)
		ahres->vf = cmix / Top.airX(tSen);		// volumetric flow (after supply fan & coils == at entry to supply duct)

		// ah results: coil output energies
		ahres->qh = ahhc.q * Top.tp_subhrDur;
		ahres->qc = ahcc.q * Top.tp_subhrDur;
		ahres->qs = ahcc.qs * Top.tp_subhrDur;
		ahres->ql = ahcc.ql * Top.tp_subhrDur;
		ahres->qFan = (rfanQ + sfan.q)*Top.tp_subhrDur;	// rfanQ is rfan.q of completed iteration (rfan.q may now be next value)

		// ah results: leaks & losses, zone energy, balance
		/* account leaks & exhaust in enthalpy relative to RETURN CONDITIONS 6-9-92,
		   to relate to hvac energy needed: so (future) air exhausted by zone @ outside air temp does not show. */
		/* exact computations would be:
		   *  #define HH(t,w,c)  ((psyEnthalpy(t,w) - hr)*(c)/SH(w))	total enthalpy difference from ret conditions for flow c
		   *  #define SH(w)      (PsyShDryAir + PsyShWtrVpr*(w))		specific heat @ w
		   *  DBL hr =           psyEnthalpy(tr,wr);			enthalpy @ return conditions */
		// approximations consistent with rest of program 6-92:
#define HH(t,w,c) ( ((t) + (w)*PsyHCondWtr/SH(w) - hrc)*(c) )	// total enth diff from ret conditions for cap flow c
#define SH(w)     (Top.tp_airSH)					// specific heat: @ ref w, as used to get c's from m's
		DBL hrc =         tr + wr*PsyHCondWtr/SH(wr); 	// enthalpy/airSh @ return conditions: compute once; use curr inputs.
#define CS crNx				// total supply flow is (next) return flow for now 6-92 (later cmix less leak)
		//DBL hhr = 0.;				// total enthalpy of return flow: 0: tr/wr are our reference t/w.
		DBL hhr1 =  HH(tr1,wr1,cr1);		// total enth (rel tr/wr) entering rtn fan (if here), after rtn duct leak/loss
		DBL hhr2 =  HH(tr2,wr1,cr1);		// .. entering eco, after poss fan
		DBL hhmix = HH(tmix,wen,cmix);		// .. exiting economizer
		DBL hhSen = HH(tSen,ws,cmix);		// .. entering supply duct, after coil(s) and supply fan
		DBL hhs =   HH(ah_tSup,ws,CS);   	// .. getting to terminals, after supply duct
		ahres->qO = (hhmix - hhr2) * Top.tp_subhrDur;  	// subhr outside air q: enthalpy (rel tr/wr) change of flow thru eco
		ahres->qLoss =  					// leak and loss q this subhour
			( hhr1 /* - hhr=0*/  			//   ret duct leak-in and cond loss: enth (rel tr/wr) change thru it
			  + hhs - hhSen ) * Top.tp_subhrDur;	//   + sup duct leak-out and cond loss: enthalpy change thru it
		ahres->qLoad = (/*hhr=0*/ - hhs) * Top.tp_subhrDur;	/* subhr load q: zone load, tu duct loss, plenum loss =
       							   enth change thru tus/zns/plenum (use prior return to get balance) */
		ahres->qBal = ahres->qh + ahres->qc + ahres->qO + ahres->qFan + ahres->qLoss + ahres->qLoad;	// should add to near 0
#undef CS
#undef HH
#undef SH

		// ah results: input energies
		ahres->pc = ahcc.p * Top.tp_subhrDur;
		ahres->ph = ahhc.p * Top.tp_subhrDur;
		ahres->pFan = sfan.p*Top.tp_subhrDur + rfan.p*Top.tp_subhrDur;

		// ah results: iteration counts: most are ++'d directly from code
		ahres->nSubhr = 1L;					// 1 subhour each subhour; let accumulation do its thing

		//if (ahMode != ahOFF)...

// --- charge energy inputs to meters where specified --- (interaction of plant vs coil type tbd)

		if (sfan.fn_mtri)      					// if meter specified for supply fan
			MtrB.p[sfan.fn_mtri].H.fan += sfan.p*Top.tp_subhrDur;	// charge power consumption to that meter, end use "fans"
		if (rfan.fn_mtri)      					// if meter specified for return/relief fan
			MtrB.p[rfan.fn_mtri].H.fan += rfan.p*Top.tp_subhrDur;	// charge power consumption to that meter, end use "fans"

		if (ahhc.mtri)							// if meter specified for heat coil
			if (ahhc.coilTy != C_COILTYCH_HW   				// if not a coil type that uses a plant
					/*&& ahhc.coilTy != C_COILTYCH_HHP*/ )  			// .. (redundant: meter disallowed)
			{
				MtrB.p[ahhc.mtri].H.htg  += ahhc.p*Top.tp_subhrDur;		// charge heating coil input power to meter, end use "htg"
			}
		if (ahhc.supHeatMtri)							// if meter specified for heat coil
			MtrB.p[ahhc.supHeatMtri].H.hpBU += ahhc.pSh*Top.tp_subhrDur;		// charge AHP suppl res heat to end use "hpbu" (0 if not AHP)
		if (ahcc.mtri)								// if meter specified for cool coil
			if (ahcc.coilTy != C_COILTYCH_CHW   					// if not a coil type that uses a plant
					/*&& ahcc.coilTy != C_COILTYCH_HHP*/ )    				// .. (redundant: meter disallowed)
				MtrB.p[ahcc.mtri].H.clg += ahcc.p*Top.tp_subhrDur;			// charge cool coil input power, end use "cooling"

		// HX meter
		if (ah_oaHx.hx_pAuxMtr)
		{
			ah_oaHx.hx_pAuxMtr->H.aux += ah_oaHx.hx_auxPwr * Top.tp_subhrDur * BtuperWh;
		}

	}


// --- result input energies and meter charges that can be non-0 even when ah is off

	ahres->pAuxC = ahcc.pAux * Top.tp_subhrDur;  	// cool aux energy input (ah-on-only portion)
	ahres->pAuxH = ahhc.pAux * Top.tp_subhrDur;  	// heat aux energy input (ditto)
	if (ahhc.auxMtri)							// heating aux energy
		MtrB.p[ahhc.auxMtri].H.aux += ahhc.pAux*Top.tp_subhrDur;

	if (ahcc.auxMtri)
		MtrB.p[ahcc.auxMtri].H.aux += ahcc.pAux*Top.tp_subhrDur;

	if (cch.mtri)							// if crankcase htr has a meter
		MtrB.p[cch.mtri].H.aux += cch.p*Top.tp_subhrDur;			// post its input (set by CCH:endSubhr from coilsEndSubhr)

	return RCOK;

}				// AH::endSubHr
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ah_AfterSubhr() 		// airHandler stuff done between subhours: after reports/exports
{
	wsls = ws;			// set last subhour supply humidity 5-92
	return RCOK;
}			// AH::ah_AfterSubhr
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::begHour() 				// airHandler stuff done at start of hour, after expression evaluation
{

// check hourly variables
	if (ahTsMn > ahTsMx)
		rer( (char *)MH_R1270, name, ahTsMn, ahTsMx); 		// "airHandler '%s': ahTsMn (%g) > ahTsMx (%g)"

#ifdef AUTOSIZE // cnglob.h
// default autoSizing design supply temps (hourly) ahTsMn/Mx, and check. 6-95.
	if (!(sstat[AH_AHTSDSH] & FsSET))			// if ahTsDsH not given
		ahTsDsH = ahTsMx;   					// use max ts (hourly variable --> copy at runtime. dfl 250).
	if (!(sstat[AH_AHTSDSC] & FsSET))			// if ahTsDsC not given
		ahTsDsC = ahTsMn;   					// use min ts (hourly variable --> copy at runtime. dfl 40).
	if (Top.tp_autoSizing)							// else moot, error message would just cause confusion.
		if (ahTsDsC > ahTsDsH)
			rer( "airHandler '%s': ahTsDsC (%g) > ahTsDsMx (%g).", name, ahTsDsC, ahTsDsH);
#endif

// set member flag if ah supply temp setpoint is under ZN (heat/off/cool) or ZN2 (heat/fanOnly/cool) control, this hour. 5-95.
	isZNorZN2 = (ISNCHOICE(ahTsSp)  &&  (CHN(ahTsSp)==C_TSCMNC_ZN || CHN(ahTsSp)==C_TSCMNC_ZN2));

// fanCycles default & setup according to this hour's ahTsSp 6-15-92.  Related code: see setTsSp1; cncoils.cpp.

	if (sstat[AH_AHFANCYCLES] & FsSET)				// if user gave ahFanCycles in input (not usually expected)
		fcc = CHN(ahFanCycles)==C_NOYESVC_YES;			// convert current user input value to 0/1
	else							// normal case: ahFanCycles not given by user
		fcc = (ISNCHOICE(ahTsSp) && CHN(ahTsSp)==C_TSCMNC_ZN);	// default: fanCycles only under ZN (not ZN2) ts sp ctrl mthd

	if (!fcc)							// if supply fan always on
		frFanOnNx = 1.0;						// fan-fraction-on always 1.0
	else							// supply fan runs only when ctu thermostat calls for heat/cold.
	{
		// frFanOnNx will be set each subhour by puteRa.
		// fanCyles and ZN2 are contradictory
		if (ISNCHOICE(ahTsSp) && CHN(ahTsSp)==C_TSCMNC_ZN2)
			rer( PABT, (char *)MH_R1271, name);		// "airHandler '%s': ahFanCycles=YES not allowed when ahTsSp is ZN2"

		// only one terminal is supported, because we are using znTerminal's fraction flow to represent ah's fraction time on.
		// (for additional terminals, would need to force same flow fraction as control terminal.)
		if (TuB.p[tu1].nxTu4a)						// test 'next' of first to detect > 1 terminal
			return rer( PWRN, (char *)MH_R1272, name );			/* "airHandler '%s': more than one terminal\n"
					                        	   "    not allowed when fan cycles" */
		// require control terminal
		if (!ahCtu)					// (insurance: msg shd not occur cuz ahCtu defaults to tu when only one)
			rer( PABT, (char *)MH_R1273, name );		/* "airHandler '%s': fan cycles, but no control terminal:\n"
							   "    ahCtu = ... apparently missing" */

		// check that minimum flow is 0 (cuz at min don't know whether heating or cooling).  hourly vbl --> runtime check required.
		TU *ctu = TuB.p + ahCtu;
		if (ahCtu)
			if (ctu->tuVfMn != 0.)
#ifdef AUTOSIZE
				if (ctu->sstat[TU_TUVFMN] & FsAS)			/* if non-0 because of autoSize give specific message 7-95
								   (errors at input time if inputs constant; may be ok at runtime
								   if fcc always on: goes non-0 in cnztu.cpp only when fcc off) */
					rer( PWRN, "Control terminal '%s' of airHandler '%s':\n"
						 "    can't AUTOSIZE tuVfMn when air handler fan cycles:\n"
						 "    When fan cycles, terminal minimum flow must be zero, but \n"
						 "    AUTOSIZE tuVfMn makes minumum flow equal to maximum flow.",	// NUMS
						 ctu->name, name, ctu->tuVfMn );
				else
#endif
					rer( PWRN, (char *)MH_R1274,			//"Control terminal '%s' of airHandler '%s':\n"
						 ctu->name, name, ctu->tuVfMn );	//"    terminal minumum flow (tuVfMn=%g) must be 0 when fan cycles"
	}

// determine upper limit of fanF that could reduce ah flow this hour:
//  largest ratio of vfMx/vfDs for any terminal, where vfMx is max( vfMn, vfMxH, vfMxC as pertinent).

	// during autoSizing, fanFMax is updated when tuVfMxH or C is increased, in cnztu.cpp:ZNR::ztuMode and cnah2.cpp:AH::antRatTs.

	// CAUTION: reducing fanF from this value may not immed reduce flow as MxC, MxH both used here tho only 1 ends up pertinent.

	DBL _fanFMax = 0.f;
	TU *tu;
	RLUP( TuB, tu)
	{
		if (tu->tuVfDs)						// could be 0 during autoSize / don't divide by 0
		{
			// Assume SET OUTPUT or SET TEMP air h/c else tu wd not have ah.
			DBL v = tu->tuVfMn;   				// min, for SET OUTPUT; also rules if > max for SET TEMP.
			if (tu->cmAr & cmStH)  setToMax( v, (DBL)tu->tuVfMxH);	// use heating max (unless smaller) if may do SET TEMP heating
			if (tu->cmAr & cmStC)  setToMax( v, (DBL)tu->tuVfMxC);	// use cooling flow if larger
			setToMax( _fanFMax, v/tu->tuVfDs );			// accumulate largest max/design for ah
		}
#ifdef AUTOSIZE // cnglob.h 6-95
		else				// bug or autoSizing the terminal without tuVfDs given, 6-95
			setToMax(_fanFMax, 1.f);		// max(tuVfMn,MxH,C) used in place of 0 tuVfDs during autoSizing --> max/design is 1.0.
		// CAUTION: 1.0 is not fanF upper limit in general: max can be >> design.
#endif
	}

	// change-monitor here or in ahEstimate?  ***

	fanFMax = _fanFMax;				// store in member
	return RCOK;
}			// AH::begHour
//-----------------------------------------------------------------------------------------------------------------------------
RC AH::ahEstimate()	// set ahMode and supply temp for terminal computations b4 initial ahCompute call for subhr

// outputs for ztuCompute: AH.ahMode, AH.ah_tSup

{
	// callers: cnztu.cpp:hvacIterSubhr: before tu-ah loop for subhour
	//          cnztu.cpp:ztuMode: to turn on ZN/ZN2 or adjust sp for ZN/ZN2/WZ/CZ.
	//          cnztu.cpp:ztuAbs: to turn on ZN/ZN2 when zone mode says should be max'd, or off if ZN min'd, 4-95.

	RC rc /*=RCOK*/;		// (removed redundant init per BCC32 4.5 warning, 12-94)

// flag ah to be CALLED (to check cr,tr,etc) at start subhour and if ztu re-ahEstimates's after 1st iteration.
// convention is: all hvac devices (but tu's?) called at start subhour.
//    and TRIED REMOVING 11-3-92: eliminated many calls, some computations; changed Yr wetbulb by .01 degree for many runs.
//    Apparently need more wz monitoring elsewhere to consider removing.

	ahClf++;		// tell subhrHvacIter to call ahCompute for this ah
	Top.ahKf++;  	// tell subhrHvacIter to check for AH's with ahClf or ahPtf on. ahPtf set in many cases below.
	// If removed, be sure ahKf set where ahPtf set below.

// determine 'ahMode' including for ahTsSp==ZN,ZN2
	AHMODE ahModeWas = ahMode;
	CSE_E( ah_SetMode() )				// sets ahMode (ahHEATING/-COOLING/both(usually)/-FAN/-OFF)
	// if ah goes on or off, sets zones ztuCf's.
	if (ahMode==ahOFF)  return RCOK;			// if ah off, done

// if ah just turned on, determine return air temp etc from zones (if already on, current trNx etc are up-to-date). 4-95.
	if (ahModeWas==ahOFF)	// if just started ah, may have last run at different zone temps, so ...
		zRat();				// compute crNx,cr1Nx; if flow: trNx,tr1Nx,tr2Nx, etc. Below.
							// Uses aTs, aWs, zone aTz's, tu aCv's, etc. Below.
       						// Note ztuMode() presets aTz, zCv etc for this call. 4-95.
// estimate new ts from ahTsSp -- or leave same
	DBL tsNu = ah_tSup;
	{
		if (VD ahTsSp != VD ahTsSpPr)				// if ahTsSp changed (numerical tolerance applied just below)
		{
			if (ISNUM(ahTsSp))						// if numeric (or 0 for none given (no eco/coil))
			{
				if ( ISNUM(ahTsSpPr)  &&  fabs(ahTsSp-ahTsSpPr) < 4.f   	// if numeric before and little changed
						&&  (VD ahhc.sched==VD hcSchPr || !(ahMode & ahHEATBIT))  	// and applicable coil sched(s) unchanged
						&&  (VD ahcc.sched==VD ccSchPr || !(ahMode & ahCOOLBIT)) )
					tsNu += (ahTsSp - ahTsSpPr);   		/* TRACK sp changes in estimated ts: assume any prior difference
	  						   eg due to fan overload is a good estimate of any new difference. */
				else             		// much changed numeric ts sp, or not numeric before, or applicable coil(s) went on or off
				{
					tsNu = ahTsSp;				// assume load will be met
					setToMax( fanF, fanFMax);			// undo any fan overload (fanFMax is set by AH::begHour)
					ahPtf++;    				// say full airHandler computation needed (tested in ahCompute)
					// note also set by exman chaf's on inputs, & internally eg in zRat, etc.
				}
			}						//  Top.ahKf set above.
			else						// new non-number ts sp
			{
				coilLockout = FALSE;   			// undo eco coil lockout (may be an input to setTsSp1)
				if (setTsSp1( TRUE, tsNu))			/* estimate ts, esp for ahTsSp== C_TSCMNC_RA, WZ, CZ, ZN, or ZN2.
							   Here estimates using some zone prior subhour info */
				{
					// not if setTsSp1 turned ah off, [or estimating error]
					setToMax( fanF, fanFMax);			// undo any fan overload (fanFMax is set by AH::begHour)
					ahPtf++;   				// say do full airHandler compute (tested in ahCompute). ahKf set above.
				}
			}
		}
		else						// setpoint number or control method unchanged
			setTsSp1( TRUE, tsNu); 			/* re-estimate numeric value anyway now, esp for WZ, CZ, ZN, ZN2.
       							   May update WZ/CZ/RA value enuf to save an iteration.
       							   Applies new min/max limits if changed.
       							   Has some estimating limits -- tries not to exceed capacity. */
	}

// recompute ah if "significant" change
	if (ABSCHANGE( tsNu, ah_tSup) > Top.absTol)  		// if estimated ts changed "significantly" 11-92
		ahPtf++;						// set this ah's "must compute" flag.  Top.ahKf set above.

	ah_tSup = tsNu;						// store updated supply temp
	aTs = ah_tSup;						/* also set "air handler working copy" ts, for use eg in zRat
							   at AH::ahCompute entry. Makes having two copies historical. 5-95. */

// update supply air volumetric heat capacity @ ts and reference w, and terminal limits based on it
	DBL sink = 0.;
	ahVshNLims( ah_tSup, sink, sink); 			/* update ah->airxTs and convert tu mn/mx cfm flows to Btuh/F.
							   Note: sfan/rfan.cMx, .dT, etc: prior values used as estimate. */
	// don't attempt refinement here, eg don't do setTsSp1 again for new airxTs, cuz eg sfan.cMx isn't current, 4-95.

// if ah output changed, set zp->ztuCf for served terminals. 10-15-92.
	flagTusIf();

// recompute return air temp here for use in getTsMnFo? But getTsMnFo adjusts tr, cr for control (usually only) terminal & ts.
// TESTED 5-95: DOES change results (reported tr when off; small addl effect on some subhrs of some runs)
//              couldn't get convinced it was better, so keep it simple instead.
//    zRat();						/* re-compute return air temp from zones, using new ts re terminal leakage.
//							   Sets tr2Nx (if any flow), etc. Uses ts, aTz's, aCv's, etc. Below.
//							   Note ztuMode() may set stuff for this call. */
//    //tsMnFoOk = FALSE;		puteRa from zRat does: say getTsMnFo, if called, must calculate new tsMnFo value.

	tsMnFoOk = tsMxFoOk = FALSE; 	// say getTsMnFo and getTsMxFo, if called eg from ztuMode, must calculate new values.

	return RCOK;
}				// AH::ahEstimate
//-----------------------------------------------------------------------------------------------------------------------------
//--- constants, also hard-coded in cncult2.cpp for Top.absTol; also in cnah2.cpp:
//const float RELoverABS = .01f;		use commented out 5-95	// relative to absolute tol ratio: +-1 corrsponds to +- 1%.
//const float ABSoverREL = 1.f/RELoverABS;	no uses 5-95		// reciprocal thereof
//---------------------------------------------------------------------------------------------------------------------------
RC AH::ahCompute()			// airHandler full computation ("refine"), after terminals have been estimated

// (old) preceding call sequence: tuEstimate, ahEstimate, ztuCompute.
{
// AH::ahPtf sets include: by exman chafs for input changes (ahTsSp, ahSch, ahhc.sched, ahcc.sched, );
//  AH::ahEstimate, ahCompute, ah_SetMode, ; ZNR::ztuMdSets (from ztuCompute).
//	        tested: cnztu.cpp:hvacSubhrIter(), AH::compute. cleared: AH::ahCompute.

	RC rc /*=RCOK*/;
	AhresB.p[ss].S.nIter1++;				// in ah subhour results subrecord, count times ah entered

// redetermine 'ahMode' including for ahTsSp==ZN,ZN2
	CSE_E( ah_SetMode() )			// sets ahMode to ahHEATING/-COOLING/both(usually)/-FAN/-OFF
	if (ahMode==ahOFF)				// if ah now off
		return RCOK;				// nothing more to do for off airHandler
	// note 10-96: ahClf stays on when return here. Clear here when/if need found.

// return air temp/flow per zone temps and terminal flows; plenum loss and return fan heat
		zRat();			// sets .tr2Nx,cr1Nx,wr1Nx, more. uses aTs/aWs.
						// Note ahEstimate sets aTs, in addition to ah_tSup, for this call

	/* Evolving control philosopy 10-92: any input change CLEARLY requiring computation should set ahPtf where detected.
	   Changes that MAY require recomputation set ahClf where detected; here we compute derived values & test if compute needed.
	     derived values include tr2Nx/wr1Nx/cr1Nx (computed by zRat from zone t/w/c's, which set ahClf upon change); dt2, . */

// done if no change found
	DBL dt2 = ah_tSup - tr2Nx;    				/* air temp change thru airHandler after return duct and return fan
    							   (already recomputed) (old ts, new tr2Nx per new tr from zones) */
	DBL tr2Chg = ABSCHANGE( tr2Nx, tr2);		// how much new tr2 changed from START prior ah iteration
	DBL cmixNx = cr1Nx;					/* new cmix will be new cr1. ONLY VALID if eco does not change flow.
    							   Later may need to doOa or simulate some of its code here. 6-92. */
	if ( !ahPtf  						// don't need to recompute if must-compute flag off, and
			&&  tr2Chg < Top.absTol					// if tr2 "unchanged" on absolute basis,
			//beleived redundant since dt is monitored: <-- is more sensitive, consider restoring 4-5-92, but add const tolerance 4-20-95.
			//&&  tr2Chg <= fabs(dt2) * RELoverABS   			if tr2 "unchanged" vs dt (small tr chg is big Q chg @ small dt)
			&&  RELCHANGE( cr1Nx, cr1NxPr) < Top.relTol    		// if flow "unchanged" <--- test vs cr1 not -NxPr 6-24-92???
			&&  RELCHANGE( dt2, tsAhPr - tr2NxPr) < Top.relTol  	/* delta-T change detects Q change when dT is small (coilLimited)
		     						   (tr2 change < absTol) and cr1 may be tu-limited */
			&&  ABSCHANGE( wr1Nx, wr1NxPr) < Top.absHumTol      	// if return humidity "unchanged" (tol .00003 or as changed)

			&&  ABSCHANGE( Top.tDbOSh, ah_tDbOShPr) * (po + oaOaLeak + ahROLeak)  <  Top.absTol
			/* if outdoor air temp "unchanged" relative to use: obscure-case
			   insurance; makes no difference in existing tests 6-24-92. */
			&&  ABSCHANGE( Top.wOSh, ah_wOShPr) * (po + oaOaLeak + ahROLeak)  <  Top.absHumTol
			/* if oudoor w "unchanged" relative to use.
			   insurance added 1-95 in preparation for SHINTERP.
			   TESTED 1-95: adds an occasional iteration in Rob's tests,
			    little or no results change. */
			//do these tests near last as most flow changes detected above:
			&&  cmixNx < sfan.cMx * Top.hiTol  			// if fan not too much overloaded
			&&  (!fanLimited || cmixNx >= sfan.cMx * Top.loTol) )  	// nor too much underloaded if is limiting thing
		return RCOK;
	// other changes include: plant supply temp to coil: .ahPtf set externally.

// save entry values
	for (TU *tu = 0;  nxTu(tu);  )  		// temp save entry flows in ZONE for upCouple/antRatTs, b4 any changes stored
	{
		ZrB.p[tu->ownTi].tuCzWas = tu->aCz();  	// aCz: znC(tu->aCv): working zone flow
#if 0//7-17-95: intended to delete 7-16, right?
		x       tu->aDtLoTem = FALSE;			// clear flag set in antRatTs if autoSizing flow gets huge as ts approaches sp
#endif
	}

// do it: determine supply temp that can be delivered -- and many other variables
	// note aTs==ts (set in ahEstimate or ahCompute), aWs==ws cuz ws is only set from aWs.
	BOO cvgFail = FALSE;		// convergence failure: flag self at exit

	if (!iter4Fs(cvgFail))		// do it: updats aTs, aWs, etc, etc.

		return RCOK;			// done if airHandler turned off (occurs under ZN control (not ZN2))
	// CAUTION -Pr's & ts random: assumes .ahPtf set when turned ON again
#ifdef DEBUG
	if (frFanOn > Top.hiTol /* + Top.relTol if necess */) 	// no msg if within vsh tolerances
		rWarn( PWRN, (char *)MH_R1275, name, frFanOn);		// devel aid warning "airHandler '%s': frFanOn (%g) > 1.0"
#endif

	/* set flags re tu_endAutosize message if autoSizing flow got huge in antRatTs as ts approached sp,
	   and ts ended up on wrong side of sp. Cuz ztuMode won't detect if ts already on wrong side of sp. */
	if (asFlow)								// else not autoSizing any tu's on ah
		for (tu = 0;  nxTu(tu);  )  					// loop ah's tu's
			if (tu->useAr & uStH)							// if this terminal is set temp heating
				tu->aDtLoHSh |= tu->aDtLoTem  &&  aTs <= ZhxB.p[tu->xiArH].sp;	// TU::begSubhr clears when ts > sp.
			else if (tu->useAr & uStC)						// if terminal is set temp cooling
				tu->aDtLoCSh |= tu->aDtLoTem  &&  aTs >= ZhxB.p[tu->xiArC].sp;	// TU::begSubhr clears when ts < sp

// outputs and change-flagging
	upCouple( aTs, ah_tSup, aWs, ws);	/* adjust aqO/bO of tu's on OTHER AH's in zones served by this ah for changes
    					   in ts and/or zone flow: anticipate ztuCompute for other ah's, to speed convergence.
    					   uses zp->tuCzWas's.  note here ts, ws are OLD values. */
	ah_tSup = aTs;
	ws = aWs;		// store working supply temp and hum ratio into ts and ws for ztuCompute
	if (!flagTusIf())			// if ah output changed, set zp->ztuCf for served terminals. uses new ts/ws! 10-15-92.
		flagChanges();			// else flag zones whose temps or terminal flows have changed.
	// (flagChanges is skipped if flagTusIf already flagged all served zones.)
#if 0//7-20,22-95 moved into iter4Fs for testing out of antRatTs
x    ahRun = !cvgFail;			/* say has been run (or not if convergence failure). Set FALSE at start subhr;
x					   TRUE may enable some autoSizing stuff eg in ztuMode. 7-20-95. */
#endif
	//tsMnFoOk = tsMxFoOk = FALSE;	puteRa from antRatTs from iter4Fs does: say getTsMnFo & getTsMxFo must recalculate.

// set Pr's (prior values)
	tr2NxPr = tr2Nx;
	cr1NxPr = cr1Nx;
	wr1NxPr = wr1Nx;
	ah_tDbOShPr = Top.tDbOSh;
	ah_wOShPr = Top.wOSh;
	upPriors();				// update TU -Pr's for tests in ZNR.ztuMdSets
	tsAhPr = ah_tSup;
	VD hcSchPr = VD ahhc.sched;
	VD ccSchPr = VD ahcc.sched;  	// used in AH::estimate; also input-monitored by chaf's
	VD ahTsSpPr = VD ahTsSp;					// update comparator used in AH::estimate

// clear flags, at exit to not stay on when set by hpEstimate from HW coil model (cncoil.cpp), 9-92.
	ahClf = FALSE;			// call-flag: set to call & test whether to compute
	ahPtf = FALSE;			// compute-flag: set to call & compute unconditionally
	ahPtf2 = cvgFail;			/* secondary compute-flag: recall only after zones computed again
					   (else hvacSubhrIter may call a second time b4 redoing zones -- slow):
					   TRUE if failed to converge (and Top.ahKf2 set in iter4Fs). */
	return RCOK;
}			// AH::ahCompute
//-----------------------------------------------------------------------------------------------------------------------------
//  AH member functions called by ahEstimate, ahCompute, etc continue in CNAH2.CPP.
//-----------------------------------------------------------------------------------------------------------------------------

/******************************************************************************************************************************
*  CONVERGER iteration convergence aid class: does extrapolation and binary search on one value, v.
******************************************************************************************************************************/
//in cnrecs.def, thence rccn.h:
//class CONVERGER			// iteration convergence aid class
//{ public:
//    CONVERGER( DBL _maxIncr = 1., DBL _minMaxIncr = 1.e-11, DBL _minSigChange = 1.e-4)
//        { init(_maxIncr, minMaxIncr, _minSigChange); }
//    void init( DBL _maxIncr = 1., DBL _minMaxIncr = 1.e-11, DBL _minSigChange = 1.e-4);	// optional (re)initializer
//    BOO doit( DBL &v, SI &nIter);							// do it: call each iteration
//  private:
//    BOO isSetup;  				// TRUE if init called since doit
//    DBL minSigChange;				// doit returns TRUE if it changes value more than this
//    DBL minMaxIncr; 				// 2 times smallest change to restrict v to in searching for solution
//    DBL maxIncr; 				// current max change to allow in value
//    DBL v2, v1, v0; 				// current (v0) and 2 previous values
//    DBL d1, d0; 				// current (d0) and previous value deltas
//    //DBL r; 					// 0 or ratio of deltas: d0/d1. not always set, NOW LOCAL in doit.
//    SI nRev;					// number of delta sign reversals
//    SI nitSince;				// number of calls since v altered
//};			// class CONVERGER
//----------------------------------------------------------------------------------------------------------------------------
void CONVERGER::init( 			// initializer. if not called, doit calls at iteration 0, using default args.

	DBL _maxIncr /*=1.*/, 		// initial minimum value increment limit (increased to d1 or d0 at 1st reversal)
	DBL _minMaxIncr /*=1.e-11*/, 	/* twice smallest incr limit: choose so can't become insignificant,
    					   ie max v * 1.e-14 or so < minMaxIncr < resolution needed */
	DBL _minSigChange /*=1.e-4*/)	/* doit returns TRUE if it changes v by more than this. TRUE return generally used to
    					   prevent terminating calling loop; thus, must be >> minMaxIncr to assume termination
    					   (diverging values may never not be altered near completion) */
{
	v2 = v1 = v0 = d1 = d0 = /*r=*/ 0.;
	nRev = nitSince = 0; 				// zero variables
	maxIncr = _maxIncr;
	minMaxIncr = _minMaxIncr;
	minSigChange = _minSigChange;	// store args
	isSetup = TRUE;									// prevent auto-init in doit
}			// CONVERGER::init
//----------------------------------------------------------------------------------------------------------------------------
BOO CONVERGER::doit( 			// do converger: save value, possibly "improve" it.
	DBL &v, 			// caller's value, possibly updated
	SI &nIter )			// caller's iteration count, incremented; we default-init here if 0 if uninit.

// returns TRUE if v changed by more than minSigChange (init() argument) -- generally tells caller to keep iterating
// uses combined techniques of cnah.cpp:converger and binClip (with special interfaces and ws/ts interlocks removed).
{
// count iterations; default-initialize on iteration 0 if init() has not been called since doit.

	nitSince++;				// count times entered since v altered
	if (nIter++==0) 			// increment caller's iteration counter
		if (!isSetup) 			// if 1st call & not init'd, init with default maxIncr, minMaxIncr, minSigChange args.
			init();
	isSetup = FALSE;			// clear flag so autoinits next cycle

// 3 values, 2 deltas

	v2 = v1;
	v1 = v0;
	v0 = v;		// v0 is newest (current) v, v2 is oldest
	d1 = d0;
	d0 = v0 - v1;		// d0 is current delta, d1 is prior
	DBL r;				// change ratio: d0/d1: computed below where needed.

// if direction of change reversed, initialize or halve max increment for binary search

	DBL mxInc = maxIncr;					// local copy of maximum increment: is adjusted at reversal.
	if (d0*d1 <= -1.)						// if a reversal and change not clearly shrinking by itself
	{
		if (nRev++==0)						// if first reversal
			setToMax( maxIncr, max( fabs(d0), fabs(d1)) );		//  increase max incr to larger of last 2 incrs.
		//  preset minimum initial value insures not starting too small.
		else 							// at each successive reversal
			if (maxIncr > minMaxIncr)				// if > minimum (so won't get insignificant compared to value)
			{
				maxIncr /= 2.;  					//  halve maximum increment
				mxInc = 3.*maxIncr/2.;				//  at reversal use 3/2 new incr (3/4 old): stagger points tested
			}
	}

// limit value change to max increment, resulting in binary search

	if ( nRev > 0						// require a reversal for mxInc init
			&&  fabs(d0) > mxInc )  					// if change too big
		v0 = (d0 > 0.) ? v1 + mxInc : v1 - mxInc;		// make v0 differ from prior by mxInc in proper direction.

	/* else conditionally add limit of geometric series of changes, a good next guess for the following 3 cases:
	          0 < r < 1:  shrinking steps, same sign: changes are geometric series insofar as r constant.
	         -1 < r < 0:  shrinking steps, alternating sign: ditto
	              r < -1: growing steps, alternating sign: this expr is the unstable equilibrium from which v is diverging,
	                      insofar as r constant; binary search (above) insures completion.
	        where r = ratio of change to prior change = (v0 - v1)/(v1 - v2) = d0/d1 */

	else if ( nitSince >= 3		// require 3 entries since v altered here: need 3 real v's for valid r.
			  &&  d1 != 0.			// skip if would divide by 0
			  &&  (r = d0/d1) < 1. ) 	// not if r >= 1: const or growing steps, same sign: diverging: don't treat here.
	{
		v0 += d0*r/(1. - r);		// to value, add limit of series of changes (what limit would be if r constant)
		// note this is algebraically equiv to cnah.cpp:converger form, v0 = (v0 - v1*r)/(1. - r).
		if (nRev > 0)					// if change-limiter (binary search) is initialized
			if (fabs(v0 - v1) > mxInc)			// apply it to this change too .. conservative insurance, ..
				v0 = (v0 > v1) ? v1 + mxInc : v1 - mxInc;	// .. since if mxInc small, nitSince not likely to get to 3.
	}
	else
		return FALSE;			// say did not alter v

// get here if v altered

	nitSince = 0;				// counts to 3 before we extrapolate: do that only on unFudged data.
	BOO sig = (fabs(v - v0) > minSigChange);	/* TRUE to return if change "significant" per initialization parameter value.
       						   (don't always return TRUE: diverging cases might never return FALSE) */
	v = v0;					// now return new value to caller (reference arg)
	return sig;
}  			// CONVERGER::doit
//----------------------------------------------------------------------------------------------------------------------------

// end of cnah1.cpp
