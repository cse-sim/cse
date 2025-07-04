// Copyright (c) 1997-2023 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnloads.cpp -- Hourly simulation loads modelling calculations for a single hour for CSE

/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"		// global defines

#include "srd.h"
#include "ancrec.h"		// record: base class for rccn.h classes
#include "rccn.h"		// IZXRAT SGRAT ZNR
#include "irats.h"
#include "timer.h"

#include "hvac.h"		// CoolingSHR and Heat Pump 95/47 sizing
#include "curvemap.h"
#include "psychro.h"	// psyHCondWtr
#include "cgwthr.h"		// Wthr.nZ .zD

#include "cueval.h"
#include "exman.h"
#include "cse.h"
#include "cnguts.h"
#include "mspak.h"
#include "foundation.h"
#include "nummeth.h"

#include <array>
#include <btwxt/btwxt.h>

#ifdef COMFORT_MODEL
#include "comfort/comfort.h"
#endif

#include "ashwface.h"		// #includes xmodule.h

/*-------------------------------- OPTIONS --------------------------------*/
// 7-92 MARG1 and MARG2 2.0 made NO DIFFERENCE even in # itertions (q2, q3, q4)
//   suspect means humidity is limiting condition.
#define MARG1 1.	// tolerance multiplier for some tests in loadsHourBeg.
#define MARG2 1.	// tolerance multiplier for some tests in loadsSubhr.
#undef ZNHVACFCONV	// define to activate consideration of
					//    HVAC heat conv/radiant ratio (incomplete)
					//    Disabled 1-2021 pending elaboration.

/*-------------------------------- DEFINES --------------------------------*/

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
static RC loadsIzxSh1();
static RC loadsIzxSh2();
static RC loadsSurfaces( BOO subhrly);
static RC loadsXFans();

/*------------------------------- CONSTANTS -------------------------------*/
static constexpr double tol_tF = 1.e-12; // temperature tolerance (degF)

/*------------------------ The MAIN EQUATION story ------------------------*/
// rob 12-89 prelim
/*
Start with the first law of thermodynamics: the change in enthalpy
(temperature (Tz) times heat capacity hc) of an object (such as the air
in a zone) is equal to the sum of the heat flows (powers) Qj into it:

	dTz
	--- * hc = sigma(Qj)
	dt

Change the derivative to a delta for numerical approx purposes, and write:

	(Tz - Tzold) * hc / t = sigma(Qj)

where Tzold is the prior cycle temperature and t is the time interval.

Now express the heat flows to other objects (masses, air in other zones,
etc) as a coupling coefficient UA times the temperature difference to
the object:  UAi * (Ti - Tz)  for the ith object.
Let Qheat represent the heat supplied by the furnace.
Redefine sigma(Qj) to represent solar heat and any other flows.  Then:

    (Tz - Tzold) * hc / t  =  sigma(UAi * (Ti - Tz)) + Qheat + sigma(Qj)

Solve for Tz:

    (Tz - Tzold) * hc / t  =  sigma(UAi * (Ti - Tz)) + Qheat + sigma(Qj)
    Tz*hc/t - Tzold*hc/t = sigma(UAi*Ti) - Tz*sigma(UAi) + Qheat + sigma(Qj)
    Tz*hc/t + Tz*sigma(UAi) = sigma(UAi*Ti) + Tzold*hc/t + Qheat + sigma(Qj)
    Tz * (hc/t + sigma(UAi)) = ditto

	  sigma(UAi*Ti) + Tzold*hc/t + Qheat + sigma(Qj)
    Tz =  ----------------------------------------------
                      sigma(UAi) + hc/t

Define "a" as the numerator less Qheat, and "b" as the denominator.
The MAIN EQUATION can then be written in the following two forms:

	     a + Qheat
	Tz = ---------           Qheat = Tz *z * b - a
		b

where  a =  sigma(UAi*Ti) + sigma(Qj) + Tzold*hc/t

		The sum of the heat flows thru conductances plus solar
		and other heat flows plus the old temperature * hc/t.

and    b =  sigma(UAi) + hc/t

		The sum of the conductances to other objects plus hc/t.


Application to ZONES: The above formulas are used to simulate zone (air)
temperatures.  When Tz is fixed, Qheat is computed; when Qheat is 0
(Tz floating between heat and cool thermostat settings, or in an
unconditioned space), Tz is computed.

Zone temperatures are computed in cnloads.cpp/cnhvac.cpp, using various subexpressions
(members of the ZNR struct) precomputed in cnguts.cpp.  They are computed
several times an hour; t is Top.tp_subhrDur.  For "hc" the CAIR is used --
the "air heat capacity" which has all the not-very-massive heat capacity of
the space (walls, furniture) lumped into it for modelling purposes.

MASSes: It turns out that the model must account for the fact that the
temperature is not uniform thru a massive/resistive/non-homogenous
object with different changing temperatures on its two sides, and the
numerical approximations are not exactly as above.  A multi-node model
is used that simulates multi-layer masses as r-c networks.  See mspak.cpp.

Masses are assumed to change temperature slowly, and to have much higher
heat capacities than the air.  A full equilibrium is NOT done between the
masses and the zones.  Instead, the air temperatures are computed assuming
the mass temperatures are constant, then the NEXT hour's mass temperatures
are computed using THIS hour's air temperatures (and solar gains).
Masses are computed only once an hour regardless of Top.tp_nSubSteps.
But Masses now all changed 1-95.
*/
//=============================================================================
RC ZNR::zn_RddInit()
// initialization common to main simulation run and each autosize design day
{
	// Set up initial and constant values
	//  some redundant (not needed each design day) but cheap
	zn_md = 1;					// zone hvac mode (control mode)
	tz = aTz = tzls = tzlh = 73.f; 		// zone air temps, incl ah working copy
	zn_tr = zn_trls = zn_trlh = 73.f;	// zone radiant temps
	wz = aWz = wzls = Top.tp_refW;		// zone humidity ratios 5-25-92
	zn_rho0ls = zn_Rho0();				// zone air density at tz, wz, pz0
	zn_bcon 							// init constant part of main equation denom:
		= zn_ua    						//  sum uval*area of zone ambient lite surfs
		  + zn_uaSpecT;					//  sum uval*area of zn specT lite surfs (hsu)
	i.znCAirSh = i.znCAir / Top.tp_subhrDur;		// commonly used in subhr code

	// Mass->ha's done in ms_RddInit
	// aMassHr = aMassSh = 0.;   object is pre-0'd but may be re-used in autoSizing
	//							 but believe these don't need init for start-interval masses
	haMass = 0.;				// +='d in ms_rddInit. pre-0'd object may be re-used in autoSizing.
	znXLGain = znXLGainLs = 0;	// no condensation heat leftover from prior iteration, rob 6-11-97
	zn_ebErrCount = 0;			// count of short-interval energy balance errors
	zn_pz0WarnCount[0] = zn_pz0WarnCount[1] = 0;

	// HVAC convective delivery fraction
	//   needs elaboration for radiant systems
	zn_fConvH = zn_fConvC = zn_fConv = 1.f;	// used iff ZNHVACFCONV defined

	return RCOK;
}		// ZNR::zn_RddInit()
//-----------------------------------------------------------------------------
RC ZNR::zn_RddDone(		// called at end of simulation and each autosize design day
	bool isAusz)	// true: autosize
					// false: simulation
// duplicate calls harmless
// NOTE: clears zn_pz0WarnCount[]
// 
// returns RCOK iff all OK
{
	RC rc = RCOK;

	// report count of AirNet pressure warnings
	//   see AIRNET_SOLVER::an_CheckResults() and ZNR::zn_CheckAirNetPressure()
	if (zn_pz0WarnCount[0] > 0 || zn_pz0WarnCount[1] > 0)
	{
		warn("Zone '%s': %s unreasonable pressure warning counts --"
			"\n    mode 0 = %d / mode 1 = %d",
			Name(),
			isAusz ? strtprintf("%s autosizing", Top.tp_AuszDoing()) : "Simulation",
			zn_pz0WarnCount[0], zn_pz0WarnCount[1]);
		zn_pz0WarnCount[0] = zn_pz0WarnCount[1] = 0;	// prevent duplicate msg
	}

	return rc;
}		// ZNR::zn_RddDone
//====================================================================
RC FC loadsHourBeg()		// start of hour loads stuff: solar gains, hourly masses, zones init, .

// non-RCOK return means terminate run.  Message already issued.
{
	/* CALCULATE HOURLY SOLAR GAINS (most are subhourly, done elsewhere)
	  Set "targets" in [zones and] masses to their solar gains for hour, calculated by combining
              hour's weather data and control variable values (window shade positions) with the
	  precalculated (cgsolar.cpp) factors in the entries in the (currently selected) Solar Gain Rat.
	  The factors already reflect the size, orientation, and absorptivity of each XSURF and the
	  sun's position this month.   cnrecs.def/rccn.h structures. */
	SGRAT* sge;
	RLUP( SgR, sge)
	{	if (!sge->sg_isSubhrly)
			sge->sg_ToTarg( Top.radBeamHrAv, Top.radDiffHrAv);
	}

	// ducts
	DUCTSEG* ds;
	RLUP( DsR, ds)
		ds->ds_BegHour();

	IZXRAT* ize;
	RLUP( IzxR, ize)
		ize->iz_BegHour();		// start-hour zone transfer / airnet (ignore return)

#if defined( ZONE_XFAN)
	// SIMULATE zone exhaust fans (xfans)
	//    xfan flow controlled hourly by schedule
	//    do before masses (may alter convective xfer (future))
	//    do before airnet (so flows are known)
	loadsXFans();
#endif

	// SIMULATE HOURLY MASSES  Updates their surface temperatures.
	//      (All light ("quick") are subhourly.)
	//		Uses hourly mass solar gains (set in loop above)
	//      sets zone aMassHr's (used in loop below).
	loadsSurfaces( FALSE);		// below. FALSE to do hourly masses.

	// zones
	ZNR* zp;
	RLUP( ZrB, zp)
		zp->zn_BegHour2();

	RSYS* rs;
	RLUP( RsR, rs)
		rs->rs_BegHour();

#if 0 // !defined( SHINTERP) else done subhrly below
o// MASSES LOOP:
o// Sum mass solar gains to their zone .qMsSg (0'd in loop above).  Used in cnztu.cpp::ZNR::ztuEndSubhr re results qMass.
o
o    MSRAT *mse;
o    RLUP( MsR, mse)					// for mse = mass 1 to n
o    {  if (mse->inside.ty==MSBCZONE)     			// if surf's inside is adjacent to a zone
o			ZrB.p[ mse->inside.zi ].qMsSg += mse->inside.sg;   	// add surf's inside solar gain to adjacent zone's .qMsSg
o       if (mse->outside.ty==MSBCZONE)   			// if surf's outside is adjacent to a zone
o			ZrB.p[ mse->outside.zi ].qMsSg += mse->outside.sg;	// add surf's outside solar gain to adjacent zone's .qMsSg
o	 }
#endif

	return RCOK;		// error return above
}		// loadsHourBeg
//-----------------------------------------------------------------------------
void SGRAT::sg_ToTarg(			// apply solar gain to target
	float bmRad,	// beam radiation on normal surface, Btuh/ft2
	float dfRad)	// diffuse radiation on horizontal surface, Btuh/ft2
// can be called hourly or subhourly, args must appropriately correspond to time
{
	if (bmRad <= 0. && dfRad <= 0.)	// if no insolation this hour
	{	sg_pTarg->st_bm = 0.;			// just zero all targets.  Unnecessary to test sge->addIt.
		sg_pTarg->st_df = 0.;
	}
	else
	{	double bmGain = bmRad * sg_bmXBmF[ Top.iHrST];
		double dfGain = dfRad * sg_dfXDfF[ Top.iHrST] + bmRad * sg_dfXBmF[ Top.iHrST];
		if (sg_pControl)					// if this SGRAT has a control
		{	bmGain *= *sg_pControl;			// multiply gain by it. eg zone shades-closed fraction.
			dfGain *= *sg_pControl;
		}
		if (sg_addIt==0)			// if 1st for target
		{	sg_pTarg->st_bm = bmGain;		// store gain, initializing target value
			sg_pTarg->st_df = dfGain;
		}
		else					// if additional for target
		{	sg_pTarg->st_bm += bmGain;		// add gain to previous
			sg_pTarg->st_df += dfGain;
		}
		// note: addIt != 0 probably now 3-90 corresponds to control != NULL
		//   but we don't depend on that assumption.
	}
	sg_pTarg->st_tot = sg_pTarg->st_bm + sg_pTarg->st_df;		// total gain
}	// SGRAT::sg_ToTarg
//-----------------------------------------------------------------------------
void SGRAT::sg_DbDump() const
{
	DbPrintf( "\nSGDIST '%s': isSubhrly=%d  addIt=%d\n%s   targ=%p  control=%p",
		Name(), sg_isSubhrly, sg_addIt,
		Top.tp_RepTestPfx(), sg_pTarg, sg_pControl);
	for (int iH=0; iH<24; iH++)
	{	if (iH%4 == 0)
			DbPrintf( "\n");
		DbPrintf("%8.2d  %6.2f  %6.2f  %6.2f",
			iH, sg_dfXDfF[ iH], sg_dfXBmF[ iH], sg_bmXBmF[ iH]);
	}
}		// sg_DbDump
//-----------------------------------------------------------------------------
RC ZNR::zn_BegHour1()	// "early" hourly initializations
// 0s gains and other values
// done *after* expressions, before dependent inits
{
	qrIgTot = qrIgTotO = qrIgTotIz = qrIgAir = 0.f;
	znSGain = znLGain = znLitDmd = znLitEu = 0.;

	if (Top.isBegRun)
	{	// prior hour setpoints
		zn_tzspHlh = i.znTH;
		zn_tzspDlh = i.znTD;
		zn_tzspClh = i.znTC;
	}
	return RCOK;
}	// ZNR::zn_BegHour1
//-----------------------------------------------------------------------------
RC ZNR::zn_BegHour2()		// beginning-of-hour calcs for zone
{
	zn_SetAirRadXArea();	// update air temp/relHum dependent radiant exchange values
							//   for convective/radiant zone model;
							//   small impact, not worth substep cost

	// hour's sensible internal gain: scheduled value, if any.
	//      Used for znaqLdHr, zp->qsIg, and to apportion to subhr results.
	qsIgHr = znSGain;	// sensible gain totalled from GAIN records
						// also: qrIgAir: NOT included here0

	// Non-hvac hour-constant portions of MAIN EQUATION "Tz = (a + q)/b" (story at start file)

	// sum ua*t for specified-temp-exposure surfaces
	XSRAT* xs /*= NULL*/;
	zn_uaXSpecT = 0.;
	for (TI xi = xsSpecT1;  xi != 0;  xi = xs->nxXsSpecT)	// these xsurfs have own chain.  loop over them.
	{	xs = XsB.p + xi;						// access XSRAT record xi
		zn_uaXSpecT += xs->x.xs_uval * xs->x.xs_area * xs->x.sfExT;		// u * a * outside temp (hourly vbl)
		/*>>>> there are 2 hourly and 1 subhourly use of uval*area --> otta precompute it,
		  maybe also use in cnguts re ua -SpecT.  FLOAT gud enuf for the latter? rob 3-91. */
	}

	// zn_aqLdHr: hour-constant non-hvac a's and q's for numerator.  a = sigma(UAi*Ti) + Tzold*hc/t.
	zn_aqLdHr =
		aMassHr					// hourly masses UAT (from loadsSurfaces call just above)
		+ qsIgHr  	    		// q's: sched sens internal gain from GAINS (znSGAIN) & wthr file.
		+ qrIgAir				// q's: radiant internal gain to zone CAir (light surfaces) 11-95.
		+ zn_uaXSpecT;			// uaT total from just above
								// all other components (infil etc) done subhourly

	// zn_bLdHr: hour-constant non-hvac b's for denominator.   b = sigma(UA) + hc/t.
	zn_bLdHr = zn_bcon; 		// just the run-const UAs

	/* zn_xqHr: q = b * t - a  using only hourly components, for change tests (just below) only:
	        Intended to be appropriately sensitive to changes when b*a almost equal to a,
	        eg when aMassHr settling & all else constant,
	        cuz terminal loads are of form  b * t - a  (with added subhrly components).
	        Fixed problems with bug39a.inp run (1 mass surf, const temps). 12-5-94. */
	zn_xqHr = zn_bLdHr * tz - zn_aqLdHr;		// member only cuz saved to -Pr in loadsSubhr.

	// check/warn alternate model values
	//   However: don't correct value -- consumers use max(), min() as approp.
	//      (Changed value can persist due to expression eval optimization.)
	if (i.znQMxH < -0.01f)
		orWarn( "znQMxH (%0.f) taken as 0 (s/b >= 0)", i.znQMxH);
	if (i.znQMxC > 0.01f)
		orWarn( "znQMxC (%0.f) taken as 0 (s/b <= 0)", i.znQMxC);

	/* hourly-only load change checks:
		zn_xqHr:     Hourly parts of  b * t - a, just above.
	    znLGain: latent gain rate (set per GAINs by cnguts.cpp:doHourgains, used in cnZtu.cpp:ZNR::znW).
		         Note: for subsequent subhrs, wz change sets ztuCf in loadsSubhr.
	    -Pr's used here are updated in loadsSubhr, every time ztuCf is on. */
	// zn_aqLdHr and zn_bLdHr are covered by zn_xqHr check & no longer checked separately 12-94.
	//   Removing aq- and zn_bLdHr checks made NO difference in test suites 12-94
	if (
		// .02 found better than .05,.1,.2 in bug39a.inp run (1 mass surf, const temps) (12-94):
		RELCHANGE( zn_xqHr, zn_xqHrPr) > .02*Top.relTol*MARG1  	// if hourly part of difference b*t-aq (for loads) changed
		||  ABSCHANGE( znLGain, znLGainPr)		// if zone moisture GAIN rate (Btuh) changed:
			> PsyHCondWtr*Top.absHumTol )		//  test abs lb/hr change after conversion
	{
		ztuCf++;					// say zone load changed: must do ztuCompute.
									// ztuCf causes -Pr's to be updated.
	}
	return RCOK;
}		// ZNR::zn_BegHour2
//-----------------------------------------------------------------------------
RC ZNR::zn_BegSubhr1()			// zone start of subhour, part 1
// call *before* airnet and possible others needing prior-step (lagged) values
{
	memset( &ZnresB.p[ss].curr.S, 0, sizeof( ZNRES_IVL_SUB) );	// access subhr results struct in ZNRES via subscr in ZrB & 0 it.
	ZnresB.p[ss].curr.S.nSubhr = 1L;	// count subhours by setting to 1 at subhr level & accumulating

	// zone-specific wind velocity pressure (re AirNet)
	// zn_windFLkg defaults from zn_eaveZ and zn_infShld
	//   else is input (subhour variability)
	zn_windPresV = Top.tp_WindPresV( i.zn_windFLkg * Top.windSpeedSh);

	if (!nMd)				// if zone has no modes (eg startup with no terminals)
		spCf++;				// tell ztuCompute to build zone mode sequence

	// initialize DHW heat transfer totals
	//   TODO: could be done in DHW code iff needed
	//   Win? -- generally less DHWHEATERs than ZONEs
	//   OTOH, this is foolproof and simple, 2-16
	zn_qDHWLoss = zn_qDHWLossAir = zn_qDHWLossRad = zn_qHPWH = zn_hpwhAirX = 0.;

	// vent utility
	zn_anVentEffect = 0;	// # of IZXRATs that *could* impact this
							//   zone when operated in vent mode.
							//   see IZXRAT::iz_HasVentEffect()

	return RCOK;
}			// ZNR::zn_BegSubhr1
//-----------------------------------------------------------------------------
RC ZNR::zn_BegSubhr2()			// zone start of subhour, part 2
// call *after* lagged values used
{
	// forced convection coeff, Btuh/ft2-F; re C_CONVMODELCH_UNIFIED
	//   based on lagged value of zone total air change rate
	//   combined with surface-specific sb_hcNat, see SBC::sb_SetCoeffs()
	//   must be done *after* DHW calcs re HPWH air motion
	zn_hcAirXComb = zn_hcAirXls + zn_hpwhAirX;	// combined ACH, all sources
	zn_hcFrc = Top.tp_hConvF * i.zn_hcFrcF * pow( max( zn_hcAirXComb, 0.f), 0.8f);
	zn_airNetI[ 0].af_Init();
	zn_airNetI[ 1].af_Init();

	zn_qIzXAnSh = 0.;						// subhour non-airnet xfers
	zn_qIzSh = 0.;							// total

	qMsSg = 0.f;	// zone's mass solar and radiant internal gains.  Accum'd during mass calcs
	qrIgMs = 0.f;

	znXLGain = 0.;	// sensible gain due to excess latent "condensation"

	// heat balance terms for convective/radiant model
	//  later accum additional from mass, airnet, etc
	zn_nAirSh = zn_qDuctCondAir		// duct conduction: lagged value from end of prior step
		      + zn_qDHWLossAir		// DHW losses into zone: current step
			  + zn_qHPWH			// DHW heat pump water heater extraction: current step
			  + zn_sysDepAirIls.af_AmfCpT();	// prior step system-dependent
												//   air flow into zone (duct leaks, OAV relief,)
	zn_dAirSh = zn_sysDepAirIls.af_AmfCp();

	zn_nRadSh = zn_qDuctCondRad		// duct conduction: lagged value
		      + zn_qDHWLossRad;		// DHW losses into zone
	zn_dRadSh = 0.;
	zn_cxSh = 0.;

	// duct conduction
	//   save total for ZNRES balance
	//   init air and rad re cur step accum if system runs
	zn_qDuctCond = zn_qDuctCondAir + zn_qDuctCondRad;
	zn_qDuctCondAir = zn_qDuctCondRad = 0.;

	zn_ductLkI.af_Init();	// air flows in / out
	zn_ductLkO.af_Init();
	zn_sysAirI.af_Init();
	zn_sysAirO.af_Init();
	zn_OAVRlfO.af_Init();
	zn_rsAmfSup = zn_rsAmfRet = 0.;

	// NO! CNE zone model can skip step calc and use prior result
	//  initialization done later for CZM zone model
	// zn_qsHvac = 0.;
	// zn_qlHvac = 0.;

	zn_fVentPrf = zn_fVent = 0.f;	// vent fraction
									//  = actual / possible vent flow

	return RCOK;
}			// ZNR::zn_BegSubhr2
//============================================================================
RC FC loadsSubhr()		// loads stuff for all zones for subhour.  Call BEFORE hvac code.

// Assumes zone subhour results pre-0'd: done in zn_BegSubhr
{
	RC rc = RCOK;
	ZNR *zp;

	/* CALCULATE SUBHOURLY SOLAR GAINS
	  Set subhourly "targets" in zones and masses to their solar gains for subhour, calculated by combining
              subhour's weather data and control variable values (window shade positions) with the
	  precalculated (cgsolar.cpp) factors in the entries in the (currently selected) Solar Gain Rat.
	  The factors already reflect the size, orientation, and absorptivity of each XSURF and the
	  sun's position this month.   cnrecs.def/rccn.h structures. */
	SGRAT* sge;
#if 1
	RLUP( SgR, sge)						// loop SgR records, setting sge to point at each
	{	if (sge->sg_isSubhrly)			// hourly sg's done elsewhere -- in loadsHourBeg
			sge->sg_ToTarg( Top.radBeamShAv, Top.radDiffShAv);
	}
#else
x	SI sunup = Top.radDiffShAv > 0.F || Top.radBeamShAv > 0.F;	// 0 if no sun this subhour
x	RLUP( SgR, sge)						// loop SgR records, setting sge to point at each
x	{	if (!sge->sg_isSubhrly)			// hourly sg's done elsewhere -- in loadsHourBeg
x			continue;
x		if (!sunup)						// if no sun this hour, just 0 all subhourly gains
x			*sge->sg_targ = 0.f;		// zero the target. testing sge->addIt unnecessary.
x		else
x		{
x#ifdef SOLAVNEND // undef in cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
xo       float gain =   					// gain consists of diffuse & hour's beam values
xo              sge->isEndIvl    				// TRUE to use end-subhr not subhr-avg, 1-95
xo                    ?    Top.radDiffSh * sge->diff[Top.iHrST] 	// combine weather data
xo                       + Top.radBeamSh * sge->beam[Top.iHrST]	// with this sge's gain factors
xo                    :    Top.radDiffShAv * sge->diff[Top.iHrST]
xo                       + Top.radBeamShAv * sge->beam[Top.iHrST];
x#else
x#if 1
x		float gain =   Top.radDiffShAv * sge->sg_diff[Top.iHrST]   	// gain consists of subhour average weather data values
x					 + Top.radBeamShAv * sge->sg_beam[Top.iHrST];   // ... combined with hour's gain factors in sge
x#else // temp backtrack 2-95, undone
xx     float gain =   Top.radDiffShAv * sge->diff[Top.iHr]   	// gain consists of subhour average weather data values
xx                  + Top.radBeamShAv * sge->beam[Top.iHr];   	// ... combined with hour's gain factors in sge
x#endif
x#endif
x			// (gain fixed for diff[24], standard time 2-95)
x			if (sge->sg_control)					// if sge has pointer to control factor
x				gain *= *sge->sg_control;			// apply control. eg fraction zone shades closed.
x			if (sge->sg_addIt==0)						// if first sge for target
x				*sge->sg_targ = gain;					// initialize by storing not adding gain
x			else
x				*sge->sg_targ += gain;					// accumulate additional gains into target by adding
x
x			/* *sge->targ is ZrB.p[i].qSgAir or .qSgTotSh, or subhrly MsR.p[i].outside.sg or .inside.sg,
x			   as targeted in cgsolar.cpp:sgrGet(). */
x			// note: addIt != 0 probably now 3-90 corresponds to control != NULL, but we don't depend on that assumption.
x		}
x	}
#endif

// SIMULATE SUBHOURLY SURFACES
// Must follow (any future) subhouly solar gain determination, precede zone loop. Uses Top.tp_subhrDur.
	loadsSurfaces( TRUE);		// below. TRUE = do subhourly (not hourly masses)

	TMRSTART( TMR_ZONE);

// COMPUTE INTERZONE TRANSFERS and base (infil only) airnet
	CSE_E( loadsIzxSh1() )

	bool bDbPrint = DbDo( dbdZM);
#if defined( _DEBUG)
	[[maybe_unused]] const ZNR* zp1 = ZrB.GetAtSafe( 1);
#endif

	// GLOBAL LOAD CHANGE CHECK: outdoor humidity
	//   change changes all zone loads, but does not show in a,b.
	BOO azCf = Top.tp_wOShChange;

	RLUP(ZrB, zp)
	{
		rc |= zp->zn_InitSubhr();		// final zone init for loads calc
										//   sets values for all models
#if defined( CRTERMAH)
		rc |= zp->zn_SetZtuCfIf(azCf);
#endif

	}

	if (!Top.tp_bAllCR)
	{	// all CNE zones (all CR or all CNE enforced)
#if !defined( CRTERMAH)
		RLUP( ZrB, zp)
			rc |= zp->zn_LoadsSubhr( azCf);	// non-CR stuff
											//   change-check subhour zone loads, etc.
#else
			// may extrapolate tz if load unchanged
			// subhour terminal/airhandler hvac
		rc |= hvacIterSubhr();
		RLUP(ZrB, zp)
		{
			zp->zn_MapTerminalResults();
			zp->zn_AirXMoistureBal(true);
		}
#endif
		return rc;
	}

	// current floating temp of non-terminal zones
	RLUPC( ZrB, zp, !zp->zn_HasTerminal())
		zp->zn_TAirFloatCR();

	// convective-radiant model multizone vent control

	// TODO ideas
	// * Overall vent available flag Top.? (allows scheduling vent)
	// * Better estimate of available vent supply temp.
	//     Use prior step? (run vent at substep 0 even if not needed?)

#if 0 && defined( _DEBUG)
	bool bReportVent = Top.jDay == 178 /* && !Top.isWarmup */;
#endif

	int ventAvail = Top.tp_GetVentAvail();		// overall vent availability

	Top.tp_fVent = 0.f;	// consensus whole building vent fraction (if not RSYSOAV)
						//     = fraction of full vent flow to use
						//       determined by zone that wants the least vent
						//  or -1 for zonal vent control

	if (ventAvail == C_VENTAVAILVC_WHOLEBLDG || ventAvail == C_VENTAVAILVC_ZONAL)
	{
		int nVentUseful = 0;	// ventilation usefulness
						//   >0: vent potentially useful for 1 or more zones
						//   =0: vent not useful
						//   <0: vent forbidden by conditions in 1 or more zones
						//       e.g. heating or cooling required
		RLUPC(ZrB, zp, !zp->zn_HasTerminal())
		{
			int nVU1 = zp->zn_AssessVentUtility();
#if 0 && defined(_DEBUG)
			if (bReportVent)
			{	// printf("\n%s Hr=%d, Zone '%s': VU=%d", Top.dateStr.CStr(), Top.iHr, zp->Name(), nVU1);
				if (nVU1 < 0)
					zp->zn_AssessVentUtility();		// call again for debug
			}
			// break NO: zn_AssessVentUtility() needed for all zones
#endif	
			if (nVU1 > 0 || ventAvail != C_VENTAVAILVC_ZONAL)
				nVentUseful += nVU1;
		}

		int bAllNoVent = 1;
		if (nVentUseful <= 0)
		{	if (Top.tp_pAirNet)
				Top.tp_pAirNet->an_ClearResults(1);
		}
		else
		{	// vent might be useful for at least 1 zone
			loadsIzxSh2();			// AirNet "vents open"
			Top.tp_fVent = 1.f;		// try full vent and limit
			RLUP(ZrB, zp)
			{
				if (!zp->zn_IsUZ())
				{
					int vRet = zp->zn_FVentCR();
					if (ventAvail == C_VENTAVAILVC_WHOLEBLDG)
					{	// whole building control
						//   if vent bad for any zone, all off
						//   else vent fract = minimum preferred
						if (vRet < 0)
						{	Top.tp_fVent = 0.f;	// vent is harmful for this zone: off for all
							break;
						}
						else if (vRet > 0)
						{	if (zp->zn_fVentPrf < Top.tp_fVent)
								Top.tp_fVent = zp->zn_fVentPrf;
							bAllNoVent = 0;
						}
						// else vRet == 0
						//   this zone does not care, do nothing
					}
					else 
					{	// zonal control: each zone gets preferred
						zp->zn_fVent = zp->zn_fVentPrf;
					}
				}
			}
		}
		if (bAllNoVent)
			Top.tp_fVent = 0.f;		// whole-building vent fraction

#if 0 && defined( _DEBUG)
		if (bReportVent && !Top.isWarmup)
			printf("\n%s Hr=%d: NVU=%d  ANV=%d  TFvnt=%0.3f", Top.dateStr.CStr(), Top.iHr, nVentUseful, bAllNoVent, Top.tp_fVent);
#endif

	}

#if defined( DEBUGDUMP)
	if (bDbPrint)
		DbPrintf( "\n");
#endif

	RSYS* rs;
	if (ventAvail == C_VENTAVAILVC_RSYSOAV)
	{	// attempt OAV for each system
		// if successful, rs_mode = (zones)->zn_hcMode = rsmOAV (prevents other RSYS modeling)
		// fVent = 0 if here
		RLUP( RsR, rs)
			rs->rs_OAVAttempt();			// ignore return (zn_hcMode / rs_mode retain outcome)
	}

	RLUP( ZrB, zp)
		rc |= zp->zn_CondixCR( ventAvail);	// duct leakage can add cross-zone air
											//   do all zones before zn_AirXMoistureBal() etc.

	RLUP( RsR, rs)
		rc |= rs->rs_AllocateZoneAir();		// NOP if OAV

#if defined( CRTERMAH)
// may extrapolate tz if load unchanged
// subhour terminal/airhandler hvac
	rc |= hvacIterSubhr();

#if 0 && defined( _DEBUG)
	if (Top.jDay == 1)
	{
		if (zp1 && abs( zp1->zn_qsHvac) < 0.01)
			printf("\nloadSubhr: tz=%0.3f  qsHvac=%0.1f", zp1->tz, zp1->zn_qsHvac);


	}
#endif
#endif	// CRTERMAH

	RLUP( ZrB, zp)
		rc |= zp->zn_CondixCR2();			// finalize zone for all modes (including OAV)

	RLUP( RsR, rs)
		rc |= rs->rs_FinalizeSh();

	RLUP( ZrB, zp)
	{	
		if (zp->zn_IsConvRad())
			zp->zn_AirXMoistureBal(true);
		rc |= zp->zn_ComfortCR();
#if !defined( CRTERMAH)
		rc |= zp->zn_LoadsSubhr( azCf);	// insurance, may not be needed for CR
#endif
#if defined( DEBUGDUMP)
		if (bDbPrint)
			zp->zn_DbDump();
#endif
	}

#if defined( DEBUGDUMP)
	if (DbDo(dbdAIRNET | dbdIZ) && ZrB.GetCount() > 0)
	{	DbPrintf("\nZone airnet results\n"
			"Zone                  pz0W[0]  pz0W[1]      pz0      qIzSh    AmfCp     AmfCpT\n");
		RLUP(ZrB, zp)
			DbPrintf("%-20.20s %8.5f %8.5f %8.5f %10.2f %8.2f %10.1f\n",
				zp->Name(), zp->zn_pz0W[0], zp->zn_pz0W[1], zp->zn_pz0, zp->zn_qIzSh,
				zp->zn_AnAmfCp(0), zp->zn_AnAmfCpT(0));
	}
#endif

	TMRSTOP( TMR_ZONE);

	return rc;
}			// loadsSubhr
//----------------------------------------------------------------------------
RC ZNR::zn_InitSubhr()
{
	// derive working setpoints.
	// done unconditionally altho not always used.
	if (Top.tp_autoSizing)
	{	// avoid setpoint step changes when autosizing
		// assume zn_tzspXbs changes hourly (altho they have subhourly variability)
		// ramp from prior value over the hour
		float f = float(Top.iSubhr + 1) / Top.tp_nSubSteps;
		zn_tzspH = (1.f - f) * zn_tzspHlh + f * i.znTH;
		zn_tzspD = (1.f - f) * zn_tzspDlh + f * i.znTD;
		zn_tzspC = (1.f - f) * zn_tzspClh + f * i.znTC;
	}
	else
	{	// not autosizing: use setpoints as input
		zn_tzspH = i.znTH;
		zn_tzspD = i.znTD;
		zn_tzspC = i.znTC;
	}

	if (zn_UsesZoneSetpoints() && zn_tzspH >= zn_tzspC)
		orer("Impossible setpoints -- znTH (%0.2f) >= znTC (%.2f)", zn_tzspH, zn_tzspC);

	// subhr's Infil UA (Btuh/F)
#if 1	// 4-17-2013
	// infiltration UA based on AMF of *zone* air
	//   UA (Btuh/F) = dry amf (lbm/hr) * spec ht (Btu/lbmDry-F)
	//   (could derive spec ht from current humrat, but effect is minor)
	//   (formerly based on outdoor air state, 4-17-2013)
	zn_uaInfil = zn_NonAnIVAmf() * Top.tp_airSH;
#if 0 && defined( _DEBUG)
	float uaInfilX = Top.tp_airxOSh   	// Btuh/cfm-F air flow heat xfer at current outdoor temp
			  * ( i.znVol 			// air changes/hr part: zone volume (ft3), times...
				  * i.infAC / 60.	// user input air changes per hour, hourly variable, /60 for per minute.
				  + i.infELA		// ELA part: user input effective leakage area (ft2), hourly variable
				  * sqrt( 			// flow is proportional to square root of
					  zn_stackc * fabs( tzlh - Top.tDbOSh) + 	//  stack coeff (see ZNR.zn_InfilSetup) * delta t
					  zn_windc * Top.windSpeedSquaredSh ) 		//  wind coeff (ditto) * wind speed ^ 2
				);
	if (FEQX( float( zn_uaInfil), uaInfilX) > .01)
		printf( "uaInfil mismatch!\n");
#endif
#else
	zn_uaInfil = Top.tp_airxOSh   	// Btuh/cfm-F air flow heat xfer at current outdoor temp
			  * ( i.znVol 			// air changes/hr part: zone volume (ft3), times...
				  * i.infAC / 60.	// user input air changes per hour, hourly variable, /60 for per minute.
				  + i.infELA		// ELA part: user input effective leakage area (ft2), hourly variable
				  * sqrt( 			// flow is proportional to square root of
					  zn_stackc * fabs( tzlh - Top.tDbOSh) + 	//  stack coeff (see ZNR.zn_InfilSetup) * delta t
					  zn_windc * Top.windSpeedSquaredSh ) 		//  wind coeff (ditto) * wind speed ^ 2
				);
#endif

	// for hvac code, compute a+q, b contributions this subhour for everything but hvac, for use in "Tz = (a + q)/b"
	// a+q =  sigma(UAi*Ti) + other q's + Told*hc/t
	zn_qSgAir = zn_sgAirTarg.st_tot;	// solar gain to air
	zn_aqLdSh = zn_aqLdHr 			// hour-constant part: intl gain, solar gain, cond to ambient & specT's, etc.
			 + aMassSh				// subhourly masses UAT (from loadsSurfaces)
			 + Top.tDbOSh * (zn_ua + zn_uaInfil) 	// temp * (constUA + infilUA)
			 + zn_qSgAir				// q's: solar gain (power, Btuh) to zone (air)
			 + zn_qIzSh					// interzone transfers to zone (q's), just set by loadsIzxSubhr
										//   (included zn_qIzXAnSh + zn_anAmfCp[ 0]*deltaT)
			 + tzls * i.znCAirSh		// Told*hc/t
			 + znXLGainLs;				// excess latent gain last subhr === heat of condensation. 5-97.

	// b  =  sigma(UAi) + hc/t
	zn_bLdSh = zn_bLdHr			// non-hvac UA's constant for hour
			+ zn_uaInfil 		// + infil UA this subhr
			+ i.znCAirSh;    	// + hc/t: .znCAirSh is hc.
	// "cair" is heat capac of everything in zone not modelled as "mass":
	// user const & per-ft2 parts for air, furniture, walls, etc).

#if !defined( CRTERMAH)
	/* zn_xqSh: q = b * t - a  using components avail here, for change tests only:
	        Intended to be appropriately sensitive to changes when b*a almost equal to a,
	        since terminal loads are of form  b * t - a.
	        Insurance 12-94 -- need was not proven at subhour level when all masses hourly (b4 aMassSh added).
	        But it did something: eliminated undercooling in hour 13 in S1LOH test run, 12-5-94, so keep. */
	zn_xqSh = zn_bLdSh * tzls - zn_aqLdSh;
#endif

	// convective-radiant zone terms
	double nAirSh =
		  i.znCAirSh*tzls		// air heat cap
		+ qsIgHr				// convective internal gains
		+ zn_AnAmfCpT( 0)
		+ zn_qIzXAnSh			// non-airnet interzone
		+ znXLGainLs			// excess latent gain last subhr === heat of condensation
		+ zn_uaInfil*Top.tDbOSh;	// non-airnet infiltration
	zn_nAirSh += nAirSh;

	double dAirSh =				// associated denominator terms
		i.znCAirSh
		+ zn_AnAmfCp( 0)
		+ zn_uaInfil;
	zn_dAirSh += dAirSh;

	double nRadSh =
		qrIgTot;			// radiant internal gains

	zn_nRadSh += nRadSh;

	double dRadSh = 0.;			// nothing additional
	// zn_dRadSh += dRadSh;

	zn_airCx = zn_airCxF*pow3( DegFtoR( tzls));
	zn_cxSh += zn_airCx;
	if (zn_cxSh < .00001)
		zn_cxSh = .00001;

	// useful combinations of terms
	zn_dRpCx  = zn_dRadSh + zn_cxSh;
	zn_nRxCx  = zn_nRadSh * zn_cxSh;
	zn_dRxCx  = zn_dRadSh * zn_cxSh;

	zn_balC1 = zn_nAirSh*zn_dRpCx + zn_nRxCx;
	zn_balC2 = zn_dAirSh*zn_dRpCx + zn_dRxCx;

#if defined( CRTERMAH)
	if (zn_IsConvRad())
	{	// convective/radiant: set up CNE-style zone factors
		zn_aqLdSh = zn_balC1 / zn_dRpCx;
		zn_bLdSh = zn_balC2 / zn_dRpCx;
	}

	// zn_xqSh: q = b * t - a  using components avail here, for change tests only:
	//  Intended to be appropriately sensitive to changes when b*a almost equal to a,
	//  since terminal loads are of form  b * t - a.
	//  Insurance 12-94 -- need was not proven at subhour level when all masses hourly (b4 aMassSh added).
	//  But it did something: eliminated undercooling in hour 13 in S1LOH test run, 12-5-94, so keep. */
	zn_xqSh = zn_bLdSh * tzls - zn_aqLdSh;
#endif

	zn_hcMode = RSYS::rsmOFF;

#if defined( DEBUGDUMP)
	if (DbDo( dbdRCM))
		DbPrintf( "%s air  qsIgHr=%0.1f  qrIgTot=%0.1f  uaInfil=%0.1f\n"
		          "      nAir=%0.2f  dAir=%0.2f  nRad=%0.2f  dRad=%0.2f  CxF=%0.6g  CX=%0.2f\n",
			Name(), qsIgHr, qrIgTot, zn_uaInfil,
			nAirSh, dAirSh, nRadSh, dRadSh, zn_airCxF, zn_airCx);
#endif
	return RCOK;
}		// ZNR::zn_InitSubhr
//-----------------------------------------------------------------------------
RC ZNR::zn_SetZtuCfIf(
	BOO azCf)		// TRUE iff "all-zones change" e.g. outdoor w changed
{

	// set change flag for terminal module if zone loads changed.  Cleared in ZNR::ztuCompute.

	// monitor subhourly-changing loads inputs to zone heat and moisture balances except subhrDur:
	//  heat balance:     aqLdSh: tzls, zn_qIzSh; zn_aqLdHr is hourly; znCair is constant.  zn_bLdSh: changes only per subhrDur.
	//                    zn_xqSh
	//  moisture balance: wzls.  znLGain is hourly.
	//  other:            md

	if (ztuCf 				// if a flag already set, skip tests, update priors to avoid unnec flagging later.
	 || azCf							// if all-zones change detected (outdoor w, above)
	 || spCf							// spCf implies ztuCf (and more), 6-92.
	 || RELCHANGE(zn_xqSh, zn_xqShPr) > .02 * Top.relTol * MARG2		// b*t - a as known here: difference sensitive.
		 // .02 copied from hour case. Probably new dominant term;
		 // may make some of the other checks unnec 12-5-94.
	 || ABSCHANGE(tzls, tzlsPr) > Top.relTol * 2. * MARG2	// Dominant term.  MATCH above. use relTol as small absTol.
		 // historical 7-13-92 (b4 zn_bLdHr ck above): .002
		 // .005 (w/o mdPr check) --> .0001 ebal errs (T12) 4-92.
		 // (without tz += tzlsDelta below, < .001 required here)
	 || ABSCHANGE(wzls, wzlsPr) > .2 * Top.absHumTol  	// MATCH tolerance above. dflt .000006, or as changed.
		 // .00002 (vs what??) adds iterations;
		 // .00001 adds even more; otherwise unexplored.
	 || zn_md != zn_mdPr		// if zn hvac mode changed last subhr, don't skip tu's now:
								// eg be sure tz right on change into a setpoint mode.
	 || RELCHANGE(zn_qIzSh, qIzShPr) > .00005)
	{
		ztuCf++;					// say zone's load changed.
		qIzShPr = zn_qIzSh;		// reset comparators (Prior values), whenever (and only when) flag set.
		tzlsPr = tzls;
		wzlsPr = wzls;
		zn_mdPr = zn_md;
		zn_xqHrPr = zn_xqHr;		// ditto;   hourly part of b*t - a  12-5-94.
		zn_xqShPr = zn_xqSh;		// ditto;   as much of  b*t - a  as known here 12-5-94
		znLGainPr = znLGain;		// ditto
	}
	else 					// load not changed
	{
		// extrapolate zone temp in case ztuCompute not invoked elsewhere:
		// this is zone/terminal item that changes most when not zone changing much;
		// other items, such as zn_qsHvac, remain approx same for same tz rate of change. 4-92
		aTz =								// ah working copy of tz: init same as tz
			tz += tzlsDelta;				// extrapolate zone temp

		// Also extraploate zone w cuz znLGain and qlMech continue.
		// Tried removing in 6-92, restoring seemed to reduce # subhours & # iterations in test runs.
		aWz =								// ah working copy
			wz += wzlsDelta;    			// also extrap zone humidity ratio
	}

	return RCOK;

}		// ZNR::zn_SetZtuCfIf
//-----------------------------------------------------------------------------
void ZNR::zn_CoupleDHWLossSubhr(
	double qLoss,		// loss rate from DHWHEATER, DHWTANK, etc., Btuh 
	float fR /*=0.5f*/)	// fraction radiant
// incorporates zone heat gain (calc'd by DHW models) into heat balance terms
{
	// TODO: 50/50 conc/rad split is approx at best
	zn_qDHWLossRad += qLoss * fR;
	zn_qDHWLossAir += qLoss * (1.f - fR);
	
	zn_qDHWLoss = zn_qDHWLossRad + zn_qDHWLossAir;

}		// ZNR::zn_CoupleDHWLossSubhr
//-----------------------------------------------------------------------------
void ZNR::zn_DbDump() const
{
	// int nhour = (Top.jDay-1)*24 + Top.iHr;
	// int stepno = Top.iSubhr+1;
	DbPrintf("%s %s: anMCp/T[ 0]=%.2f/%.1f  anMCp/T[ 1]=%.2f/%.1f  ventUt=%d\n",
		Name(), zn_IsUZ() ? "UZ" : "CZ",
		zn_AnAmfCp( 0), zn_AnAmfCpT( 0), zn_AnAmfCp( 1), zn_AnAmfCpT( 1),
		zn_ventUt);
	DbPrintf("     Nair=%.2f  Dair=%.2f  Nrad=%.2f  Drad=%.2f  CX=%.2f  airX=%.3f\n",
		zn_nAirSh, zn_dAirSh, zn_nRadSh, zn_dRadSh, zn_cxSh, i.zn_hcAirX);
	if (!zn_IsUZ())
	{	DbPrintf("     TH=%.2f  TD=%.2f  TC=%.2f",
				zn_tzspH, zn_tzspD, zn_tzspC);
		if (!zn_HasRSYS())
				DbPrintf("  qhCap=%.f  qcCap=%.f\n", i.znQMxH, i.znQMxC);
		else
			DbPrintf("  tSP=%.2f  md=%d  amfReq=%.f  amfSup=%.f   tSup=%.2f\n",
				zn_tzsp, zn_hcMode, zn_rsAmfSysReq[ 0], zn_sysAirI.af_amf, zn_sysAirI.as_tdb);
	}
	DbPrintf( "     ta=%.2f  tr=%.2f  qIzSh=%.f  fvent=%.3f  pz0=%.4f  qsHvac=%.f\n",
			tz, zn_tr, zn_qIzSh, zn_fVent, zn_pz0, zn_qsHvac);
}		// zn_DbDump
//-----------------------------------------------------------------------------
RSYS* ZNR::zn_GetRSYS()
{
	RSYS* rs = RsR.GetAtSafe( i.zn_rsi);
	return rs;
}		// ZNR::zn_GetRSYS
//-----------------------------------------------------------------------------
const RSYS* ZNR::zn_GetRSYS() const
{
	const RSYS* rs = RsR.GetAtSafe( i.zn_rsi);
	return rs;
}		// ZNR::zn_GetRSYS
//-----------------------------------------------------------------------------
int ZNR::zn_IsHCAvail(		// determine system availability
	[[maybe_unused]] int rsMode) const		// capability required: rsmHEAT, rsmCOOL
// returns 1 if specified capability is available
{	return !zn_IsUZ();		// TODO: schedule availability?
}		// ZNR::zn_HCAvail
//-----------------------------------------------------------------------------
float ZNR::zn_VentTSup()		// vent supply temp under current condtions
// returns vent supply temp (at zone), F
{
	return Top.tDbOSh;		// TODO not quite right (fan heat?)
}		// ZNR::zn_VentTSup
//----------------------------------------------------------------------------
int ZNR::zn_AssessVentUtility()	// assess vent utility

// tz assumed set to current floating zone temp

// do not call if vent not possible

// returns venting utility for this zone (and sets zn_ventUt)
//      -9999: forbid (no zone and or should vent)
//          0: don't care (try venting if useful for other zones)
//             always returns 0 for unconditioned zones (even if vent off)
//          1: might be helpful
{
const int vForbid = -9999;
	int ventUt = 0;		// init to "don't care"
	if (!zn_IsUZ() && zn_anVentEffect > 0)
	{
		if (zn_tzspD < 0.f)
		{
			orer("znTD is needed re vent control but has not been set.");
			ventUt = vForbid;
		}
		else if (tz < zn_tzspD)
			ventUt = vForbid;		// vent off or zone temp below TD (any vent would hurt)
		else if (tz > zn_tzspD)
		{	// zone temp is above TD
			//  vent might be useful
			float tvSup = zn_VentTSup();
			if (tvSup > tz)
				ventUt = vForbid;	// venting would add heat, forbid
			else if (tvSup > zn_tzspC && zn_IsHCAvail( RSYS::rsmCOOL))
				ventUt = vForbid;	// venting cannot prevent cooling
			else
				ventUt = 1;			// venting might help get to TD
		}
		// else don't care
	}
	zn_ventUt = max(ventUt, -1);
	return ventUt;
}	// ZNR::zn_AssessVentUtility
//-----------------------------------------------------------------------------
int ZNR::zn_FVentCR()			// find zone's preferred vent fraction
// assume tz is at floating temp
// sets zn_fVentPrf = best vent fraction for this zone in isolation (0 - 1)
// returns -1: vent is harmful
//          0: no effect / don't care
//          1: useful
{
#if 0 && defined( _DEBUG)
	if (Top.jDay == 178)
		printf("\nzn_FVentCR '%s'", Name());
#endif

	int ret = 0;
	zn_fVentPrf = 0.f;

	double anAmfCpT  = zn_AnAmfCpT( 1) - zn_AnAmfCpT( 0);
	if (fabs( anAmfCpT) < .001)
	{	// zn_fVentPrf = 0.f;		// other zones free to use max
		zn_tzVent = 0.f;	// vent air temp not known
		ret = 0;			// vent has no effect on this zone
	}
	else
	{	double anAmfCp = zn_AnAmfCp(1) - zn_AnAmfCp(0);
		zn_tzVent = zn_TAirCR( anAmfCpT, anAmfCp);	// zone air temp at full vent (do not set tz)
		if (zn_tzVent > zn_tzspC && zn_IsHCAvail( RSYS::rsmCOOL))
			ret = -1;	// zn_fVentPrf = 0
		else if (zn_tzVent > zn_tzspD)
		{	zn_fVentPrf = 1.f;
			ret = 1;
		}
		else
		{	double amfDT = anAmfCpT - anAmfCp * zn_tzspD;
			if (amfDT != 0.)
			{	double qVent = zn_QAirCR( zn_tzspD); // q required to keep tz at zn_tzspD
				zn_fVentPrf = qVent / amfDT;
				if (zn_fVentPrf < 0.f)
				{
#if 0 && defined( _DEBUG)
					orWarn("fVent < 0\n");
#endif
					zn_fVentPrf = 0.f;
				}
				else if (zn_fVentPrf > 1.f)
					zn_fVentPrf = 1.f;
			}
			ret = 1;
		}
	}
	return ret;
}	// ZNR::zn_FVentCR
//-----------------------------------------------------------------------------
RC ZNR::zn_CondixCR(		// zone conditions part 1, convective/radiant model
	int ventAvail)		// vent availability
						//   C_VENTAVAILVC_WHOLEBLDG, C_VENTAVAILVC_ZONAL

// determines tz, tr, zn_qsHvac, zn_qIzSh
// returns RCOK or ...
{
	RC rc = RCOK;

#if 0 && defined( _DEBUG)
x	if (!Top.isWarmup)
x	{	if (Top.jDay==31 && strMatch( Name(), "SDuctZone"))
x			printf( "Hit\n");
x	}
#endif

#if defined( CRTERMAH)
	zn_rsAmfSysReq[ 0] = zn_rsAmfSysReq[ 1] = 0.;	// RSYS air requests

	zn_anAmfCpVent = 0.;		// full vent heat rate, Btuh/F
	zn_anAmfCpTVent = 0.;		// full vent heat addition, Btuh

	if (zn_HasTerminal())
	{
		zn_fVent = 0.f;		// no vent
		zn_pz0 = zn_pz0W[0];

		return rc;
	}

	// zn_fVent = fVent	-- don't change until end (zone value used herein)
	zn_qsHvac = 0.;
	zn_qlHvac = 0.;
#if defined( ZNHVACFCONV)
	zn_fConv = 1.f;		// HVAC convective fraction
#endif

#else
	// zn_fVent = fVent	-- don't change until end (zone value used herein)
	zn_qsHvac = 0.;
	zn_qlHvac = 0.;
#if defined( ZNHVACFCONV)
	zn_fConv = 1.f;		// HVAC convective fraction
#endif
	zn_rsAmfSysReq[0] = zn_rsAmfSysReq[1] = 0.;	// RSYS air requests

	zn_anAmfCpVent = 0.;		// full vent heat rate, Btuh/F
	zn_anAmfCpTVent = 0.;		// full vent heat addition, Btuh
#endif

	bool bUZ = zn_IsUZ();
	float znfVent = ventAvail == C_VENTAVAILVC_ZONAL ? zn_fVentPrf : Top.tp_fVent;
	if (bUZ || znfVent > 0.)
	{	// float temp
		// * unconditioned: HVAC not possible
		// * conditioned: HVAC is off if vent is on
		zn_tzsp = 0.f;
		if (znfVent > 0.f)
		{
			zn_anAmfCpVent  = znfVent*(zn_AnAmfCp( 1)  - zn_AnAmfCp( 0));
			zn_anAmfCpTVent = znfVent*(zn_AnAmfCpT( 1) - zn_AnAmfCpT( 0));

			zn_pz0 = znfVent*zn_pz0W[ 1] + (1.f-znfVent)*zn_pz0W[ 0];	// zone pressure

			tz = zn_TAirCR( zn_anAmfCpTVent, zn_anAmfCpVent);

			if (!bUZ && znfVent < 1.f && znfVent == zn_fVentPrf)
			{	// equal fVents: current zone is "control"
				// tz s/b exactly TD, fix if very close
				//    else error
				if (fabs( zn_tzspD - tz) < .001)
					tz = zn_tzspD;
#if defined( _DEBUG)
				else
					printf( "Zone '%s': control zone vent mismatch\n", Name());
#endif
			}
		}
		else
		{	// tz already known
			zn_pz0 = zn_pz0W[ 0];		// no venting
#if defined( _DEBUG)
			float tzx = zn_TAirCR( 0., 0.);
			if (fabs( tz - tzx) > .0001)
				printf( "Zone '%s': floating temp mismatch\n", Name());
#endif
		}

#if defined( _DEBUG)
		if (!bUZ && zn_anVentEffect > 0)
		{	// check conditioned zone outcome
			//   don't complain about zones with no vents
			if (tz < zn_tzspD - .001f)
			{	if (znfVent > .001 && zn_anAmfCpTVent > .001)
					orWarn("vent tz (%.1f F) below TD (%.1f F)", tz, zn_tzspD);
				if (tz < zn_tzspH)
					orWarn("floating tz (%.1f F) below TH (%.1f F)", tz, zn_tzspH);
			}
			else if (tz > zn_tzspC + .1f)
				orWarn("floating tz (%.1f F) above TC (%.1f F)", tz, zn_tzspC);
		}
#endif
		zn_fVent = znfVent;		// final vent fraction
	}

	else
	{	// HVAC may be required

		zn_fVent = 0.f;		// no vent
		zn_pz0 = zn_pz0W[ 0];

		RSYS* rs = zn_GetRSYS();
		if (rs)
		{	// this zone is served by an RSYS
			if (zn_hcMode == RSYS::rsmOFF)	// if not already handled (e.g. rsmOAV)
			{	zn_tzsp = 0.f;
				if (tz > zn_tzspC)
				{	zn_tzsp = zn_tzspC;
					zn_hcMode = RSYS::rsmCOOL;
#if defined( ZNHVACFCONV)
					zn_fConv = zn_fConvC;
#endif
				}
				else if (tz < zn_tzspH)
				{	zn_tzsp = zn_tzspH;
					zn_hcMode = RSYS::rsmHEAT;
#if defined( ZNHVACFCONV)
					zn_fConv = zn_fConvH;
#endif
				}
				if (zn_hcMode != RSYS::rsmOFF)
				{
#if 0 && defined( _DEBUG)
x					if (Top.tp_pass1B)
x						printf( "1B\n");
#endif
					int rsAvail = rs->rs_SupplyAirState( zn_hcMode);

					if (rsAvail >= 2)
					{	// mode is possible
						rc |= zn_AirRequest( rs, 1);	// determine air requirement given rs_asSup
														//   1: msg if bad supply temp
						// zone temp calc'd in zn_CondixCR2()
					}
					else if (rsAvail == 1)
					{	// autosizing but not requested mode
						// hold temp at set point, ignore HVAC air (TODO?)
						tz = zn_tzsp;
#if defined( ZNHVACFCONV)
						zn_QsHvacCR( tz, zn_fConv);
#else
						zn_QAirCR(tz);
#endif
					}
					// else system not available
					// leave tz = floating and zn_rsAmfSysReq[] = 0
				}
			}
		}
		else
		{	// non-RSYS zone ("magic" heating and cooling)
			zn_IdealHVAC();
		}
	}

#if defined( _DEBUG)
	if (tz < -100.f || tz > 200.f)
		warn( "Zone '%s': unreasonable air temp %0.2f\n", Name(), tz);
#endif

	return rc;
}		// ZNR::zn_CondixCR
//-----------------------------------------------------------------------------
RC ZNR::zn_AirRequest(		// determine air requirement given rs_asSup
	RSYS* rs,			// zone's RSYS
	[[maybe_unused]] int options /*=0*/)		// option bits
							//   1: report "flipped" supply temps
// Prereq: rs->rs_SupplyAirState() at current speed
// returns RCOK iff success
{
	RC rc = RCOK;
	double tSup0 = rs->rs_asSup.as_tdb;
#if 1 && defined( _DEBUG)
	if ((options & 1) && !Top.isWarmup && rs->rs_speedF > 0.99f
		&& (   (zn_hcMode == RSYS::rsmCOOL && tSup0 >= zn_tzsp)
		    || (zn_hcMode == RSYS::rsmHEAT && tSup0 <= zn_tzsp && rs->rs_effHt > 0.f)))
	{
		orWarn("Flipped tSup  RSYS='%s', tPln=%0.3f, tSup=%0.3f, tSP=%0.1f, tZn=%0.3f\n",
			rs->Name(), rs->rs_asOut.as_tdb, tSup0, zn_tzsp, tz);
	}
#endif

	double amfSup0 = zn_AmfHvacCR(zn_tzsp, tSup0);	// dry air mass flow rate required to hold
#if 0 && defined(_DEBUG)
	double qLoad = zn_QAirCR(zn_tzsp);
	// load at system, Btuh
	double qLoad0 = rs->rs_asOut.as_QSenDiff2(rs->rs_asIn, amfSup0);
	if (fabs(qLoad - qLoad0) > 5.)
		printf("\nzn_AirRequest mismatch");
#endif

	zn_rsAmfSysReq[0] = rs->rs_ZoneAirRequest(amfSup0, 0);	// notify system of requirement
	CHECKFP(zn_rsAmfSysReq[0]);		// check for NaN etc (debug only)

	if (zn_hcMode == RSYS::rsmHEAT && rs->rs_CanHaveAuxHeat() && rs->rs_speedF==1.f)
	{	// HP heating full speed: repeat calc with full aux
		double tSup1 = rs->rs_asSupAux.as_tdb;
		double amfSup1 = zn_AmfHvacCR(zn_tzsp, tSup1);
		zn_rsAmfSysReq[1] = rs->rs_ZoneAirRequest(amfSup1, 1);
#if defined( _DEBUG)
		double qHt0 = amfSup0 * (tSup0 - zn_tzsp);
		double qHt1 = amfSup1 * (tSup1 - zn_tzsp);
		// zn_AmfHvacCR can return DBL_MAX
		if (qHt0 > 0. && qHt0 < 1.e10 && qHt1 > 0. && qHt1 < 1.e10
			&& frDiff(qHt0, qHt1) > .001)
			printf("\nqHt mismatch");
#endif
	}
	return rc;
}		// ZNR::zn_AirRequest
//-----------------------------------------------------------------------------
void ZNR::zn_SetRSYSAmf(		// set RSYS air flow
	float fSize,		// fraction of zone request that system can meet
	int iAux)			// [ 0]=main, [ 1]=main+aux
{
	RSYS* rs = zn_GetRSYS();

	// supply / return adjusted for duct leakage
	int iHC = rs->rs_DsHC();	// select duct config (0=htg, 1=clg)
	zn_rsAmfSup = fSize * zn_rsAmfSysReq[ iAux] * rs->rs_ducts[ iHC].ductLkXF[ 0];
	zn_rsAmfRet = fSize * zn_rsAmfSysReq[ iAux] * rs->rs_ducts[ iHC].ductLkXF[ 1];

	zn_rsFSize = fSize;

}	// ZNR::zn_SetRSYSAmf
//----------------------------------------------------------------------------
void ZNR::zn_SetRSYSAmfFromTSup()	// set RSYS-to-zone air flow given tSup
// supply temp = rs_asSup.as_tdb
// call ONLY when RSYS capacity sufficient
{
	if (zn_hcMode != RSYS::rsmOFF)
	{	RSYS* rs = zn_GetRSYS();
		int iHC = rs->rs_DsHC();
		zn_rsAmfSup = zn_AmfHvacCR( zn_tzsp, rs->rs_asSup.as_tdb);
		zn_rsAmfRet = zn_rsAmfSup * rs->rs_ducts[ iHC].ductLkXF[ 1] / rs->rs_ducts[ iHC].ductLkXF[ 0];
		zn_rsFSize = 1.f;
	}
	// else zn_rsAmfxxx = 0
}		// ZNR::zn_SetRSYSAmfFromTSup
//-----------------------------------------------------------------------------
void ZNR::zn_IdealHVAC()		// idealized space conditioning
// applies "magic" heating or cooling
{
	if (tz > zn_tzspC && i.znQMxC < 0.f)
	{
		zn_hcMode = RSYS::rsmCOOL;
		tz = zn_tzsp = zn_tzspC;
	#if defined( ZNHVACFCONV)
		zn_fConv = zn_fConvC;
		zn_qsHvac = zn_QsHvacCR(tz, zn_fConv);
	#else
		zn_qsHvac = zn_QAirCR(tz);
	#endif
		if (fabs(zn_qsHvac) > fabs(i.znQMxC))
		{	zn_qsHvac = i.znQMxC;
	#if defined( ZNHVACFCONV)
			tz = zn_TAirCR(zn_qsHvac * zn_fConv, 0., zn_qsHvac * (1.f - zn_fConv));
	#else
			tz = zn_TAirCR(zn_qsHvac, 0.);
	#endif
		}
		// else capacity sufficient
	}
	else if (tz < zn_tzspH && i.znQMxH > 0.f)
	{	// tz < TH: air needs heating
		zn_hcMode = RSYS::rsmHEAT;
		tz = zn_tzsp = zn_tzspH;
#if defined( ZNHVACFCONV)
		zn_fConv = zn_fConvH;
		zn_qsHvac = zn_QsHvacCR(tz, zn_fConv);
#else
		zn_qsHvac = zn_QAirCR(tz);
#endif
		if (zn_qsHvac > i.znQMxH)
		{	// heat at heating capacity, find resulting tz.
			zn_qsHvac = i.znQMxH;
#if defined( ZNHVACFCONV)
			tz = zn_TAirCR(zn_qsHvac * zn_fConv, 0., zn_qsHvac * (1.f - zn_fConv));
#else
			tz = zn_TAirCR(zn_qsHvac, 0.);
#endif
		}
		// else capacity sufficient
	}
}		// ZNR::zn_IdealHVAC
//-----------------------------------------------------------------------------
void ZNR::zn_MapTerminalResults()	// transfer terminal model outcomes to working mbrs
{

	double cSup = 0.;
	double cRet = 0.;
	double tSup = 0.;
	double wSup = 0.;

	for (TU* tu = nullptr; nxTu(tu); )
	{
		const AH* ah = AhB.GetAtSafe(tu->ai);
		if (!ah)
		{	// no terminal air flow w/o AH
			continue;
		}

		if (ah->ahcc.q < 0.)
			zn_hcMode = RSYS::rsmCOOL;
		else if (ah->ahhc.q > 0.)
			zn_hcMode = RSYS::rsmHEAT;

#if defined( _DEBUG)
		double aCz = tu->aCz();
		if (frDiff(tu->cz, aCz) > 0.001)
			printf("\nFlow mismatch -- tu->cz=%.2f   tu->aCz()=%.2f",
				tu->cz, aCz);
#endif

		if (tu->cz > 0.)
		{	// accumulate supply flow and state
			cSup+= tu->cz;									// supply flow (at zone), Btuh/F
			cRet += tu->cz * (1.-ah->oaZoneLeakF * ah->po);	// return flow (at zone), Btuh/F
			tSup += tu->cz * ah->ah_tSup;
			wSup += tu->cz * ah->ah_wSup;
		}
	}

#if defined( _DEBUG)
	if (frDiff( cSup, double( cSum)) > 0.001)
		printf("\nTotal flow mismatch");
#endif
	if (cSup > 0.)
	{
		tSup /= cSup;
		wSup /= cSup;

		zn_rsAmfSup = cSup / Top.tp_airSH;

		if (!Top.tp_airNetActive)
			cRet = cSup;	// prevent unbalanced if no airnet
							//   (no relief holes available)

		zn_rsAmfRet = cRet / Top.tp_airSH;

		zn_sysAirI.af_AccumDry(zn_rsAmfSup, tSup, wSup);
		zn_sysAirO.af_AccumDry(-zn_rsAmfRet, tz, wzls);	// w finalized in zn_AirXMoistureBal
	}

}		// ZNR::zn_MapTerminalResults
//-----------------------------------------------------------------------------
RC ZNR::zn_CondixCR2()		// zone conditions, part 2
{
	RSYS* rs = zn_GetRSYS();
	if (rs)
	{	if (zn_rsAmfSup > 0.)
		{	// this zone is served by RSYS and receives air
			// RSYS has determined amf available in rs_AllocateZoneAir()
			//   zn_rsAmfSup, zn_rsAmfRet, and zn_rsFSize are set
#if defined( _DEBUG)
			if (!Top.isWarmup && rs->rs_mode != zn_hcMode)
				// zone mode should match RSYS mode
				// don't check during warmup (including autosize)
				printf( "Zone '%s': Mode mismatch\n", Name());
#endif
			double mCp = zn_rsAmfSup*Top.tp_airSH;
			double tSup = rs->rs_asSup.as_tdb;
			tz = zn_TAirCR( mCp*tSup, mCp);
			zn_qsHvac = mCp*(tSup - tz);
			// zn_qsHvac = zn_QAirCR(tzls); experiment, same results 11-21
			zn_sysAirI.af_AccumDry( zn_rsAmfSup, rs->rs_asSup);
			if (zn_hcMode == RSYS::rsmOAV)
				zn_OAVRlfO.af_AccumDry( -zn_rsAmfSup, tz, wzls);	// w not used
			else
				zn_sysAirO.af_AccumDry( -zn_rsAmfRet, tz, wzls);	// w finalized in zn_AirXMoistureBal
#if defined( _DEBUG)
			if (tz < -100.f || tz > 200.f)
				warn( "Zone '%s': unreasonable air temp %0.2f\n", Name(), tz);
#endif
		}

#if defined( _DEBUG)
		// test against setpoint (with a little tolerance)
		if (!Top.isWarmup)		// skip if warmup (including autosize)
		{	if (zn_hcMode == RSYS::rsmHEAT)
			{	if (tz > zn_tzspH + .001f)
					printf( "\nOverheat");
			}
			else if (zn_hcMode == RSYS::rsmCOOL)
			{	if (tz < zn_tzspC - .001f)
					printf( "\nOvercool");
			}
		}
#endif
	}
#if defined( CRTERMAH)
	else if (zn_HasTerminal())
	{
		zn_MapTerminalResults();


	}
#endif
	// else all known
#if defined( ZNHVACFCONV)
	zn_tr = zn_TRadCR( tz, zn_qsHvac*(1.-zn_fConv));
#else
	zn_tr = zn_TRadCR( tz);
#endif

	zn_qIzSh = zn_anAmfCpTVent + zn_AnAmfCpT( 0) + zn_sysDepAirIls.af_AmfCpT()
		- (zn_anAmfCpVent + zn_AnAmfCp( 0) + zn_sysDepAirIls.af_AmfCp())*tz
		+ zn_qIzXAnSh;

	return RCOK;

}		// ZNR::zn_CondixCR2
//----------------------------------------------------------------------------
#if defined(_DEBUG)
RC ZNR::zn_AirFlowVsTsup()
{
	RC rc = RCOK;
	if (!Top.isEndHour)
		return rc;		// skip all except last substep

	RSYS* rs = zn_GetRSYS();
	if (!rs)
		return RCBAD;

	static FILE* f = NULL;
	if (!f)
	{
		f = fopen(strtprintf("AirCurves.csv"), "wt");
		if (!f)
			return RCBAD;		// can't open file
		fprintf(f, "%s\n", rs->rs_desc.CStrIfNotBlank( "Curves"));

		fprintf(f, "Mon,Day,Hr,Sh,fAmf,amf,tReg,amfNeeded\n");

	}

	// RSYS rsSave( rs);		// save for restore at exit
								//   (we alter mbrs here)


	AIRSTATE asSup;		// air state at register
	for (int iAf = 0; iAf < 10; iAf++)
	{
		float fAmf = 1.f - float(iAf) / 10.f;

		float amf = rs->rs_TSupVarFlow(fAmf, asSup);

		double amfNeed = zn_AmfHvacCR(70. /*tz*/, asSup.as_tdb);

		fprintf(f, "%d,%d,%d,%d,%.1f,%.0f,%0.1f,%.0f\n",
			Top.tp_date.month, Top.tp_date.mday, Top.iHrST + 1,Top.iSubhr,
			fAmf, amf, asSup.as_tdb, amfNeed);

	}

	return rc;
}	// ZNR::zn_AirFlowVsTsup
//-----------------------------------------------------------------------------
float RSYS::rs_TSupVarFlow(
	float fAmf,
	AIRSTATE& asSup)
{

	asSup = rs_asOut;

	float amf = rs_amf * fAmf;

	return rs_SupplyDSEAndDucts(asSup, amf);

}		// RSYS::rs_TSupVarFlow
#endif
//----------------------------------------------------------------------------
double ZNR::zn_TAirCR(			// zone air temp w/ add'l radiant heat
  	double mCpT,	// add'l air heat to zone, Btuh
					//   either heat added to air ("qAir")
					//   or mass flow*Tsup (with associated mCp)
  	double mCp,		// add'l air heat rate, Btuh/F
	double qRad) const	// add'l radiant heat to zone, Btuh
// returns zone air temp, F
{
	double tza = (mCpT*zn_dRpCx + zn_balC1 + zn_cxSh*qRad)
					/ (mCp*zn_dRpCx + zn_balC2);
	return tza;
}	// ZNR::zn_TAirCR
//----------------------------------------------------------------------------
double ZNR::zn_TAirCR(			// zone air temp
	double mCpT,	// add'l air heat to zone, Btuh
					//   either heat added to air ("qAir")
					//   or mass flow*Tsup (with associated mCp)
	double mCp) const // add'l air heat rate, Btuh/F
// returns zone air temp, F
{
	double tza = (mCpT*zn_dRpCx + zn_balC1)
		/ (mCp*zn_dRpCx + zn_balC2);
	return tza;
}	// ZNR::zn_TAirCR
//----------------------------------------------------------------------------
// Note float w/ no add'l gains (cnrecs.def inline)
// void zn_TAirFloatCR() { tz = zn_balC1 / zn_balC2; }
//----------------------------------------------------------------------------
double ZNR::zn_TRadCR(		// zone radiant temp w/ add'l radiant heat
	double tza,			// zone air temp, F
	double qRad) const	// add'l radiant heat to zone
// returns zone radiant temp, F
{
	double tzr = (zn_nRadSh + qRad + zn_cxSh*tza)/zn_dRpCx;
	return tzr;
}	// ZNR::zn_TRadCR
//----------------------------------------------------------------------------
double ZNR::zn_TRadCR(		// zone radiant temp
	double tza) const		// zone air temp, F
// returns zone radiant temp, F
{
	double tzr = (zn_nRadSh + zn_cxSh * tza) / zn_dRpCx;
	return tzr;
}	// ZNR::zn_TRadCR
//------------------------------------------------------------------------------
double ZNR::zn_QAirCR(		// air heating/cooling requirements w/ add'l radiant heat
	double tza,			// zone air temp, F
	double qRad) const	// add'l radiant heat
// returns zone air heating/cooling (Btuh) required to maintain tza
{
	double q = (zn_balC2*tza - zn_balC1 - zn_cxSh*qRad) / zn_dRpCx;
	return q;
}		// ZNR::zn_QAirCR
//------------------------------------------------------------------------------
double ZNR::zn_QAirCR(		// air heating/cooling requirements
	double tza) const			// zone air temp, F
// returns zone air heating/cooling (Btuh) required to maintain tza
{
	double q = (zn_balC2*tza - zn_balC1) / zn_dRpCx;
	return q;
}		// ZNR::zn_QAirCR
//------------------------------------------------------------------------------
double ZNR::zn_QsHvacCR(		// sensible hvac heat/cool requirements
								//   (with possible conv/radiant mix)
    double tza,			// zone air temp, F
	float fConv) const	// convective faction of available source
// assume no vent (HVAC active)
// does NOT change ZNR state
// returns sensible power rqd to hold tza, Btuh
{
	double qs = (zn_balC2*tza - zn_balC1) / (fConv*zn_dRadSh + zn_cxSh);
	return qs;
}		// ZNR::zn_QsHvacCR
//------------------------------------------------------------------------------
double ZNR::zn_AmfHvacCR(	// sensible hvac air requirements w/ add'l radiant heat
    double tza,			// zone air temp, F
	double tSup,		// available supply air temp, F
	double qRad) const	// add'l radiant heat to zone
// assume no vent (HVAC active)
// does NOT change ZNR state
// returns dry-air mass flow rate required to hold tza, lbm/hr
{
	double amf = (fabs( tza - tSup) < tol_tF)
					? DBL_MAX
					: (zn_balC1 - zn_balC2*tza + zn_cxSh*qRad)
				       / (zn_dRpCx * (tza - tSup) * Top.tp_airSH);
	return amf;
}		// ZNR::zn_AmfHvacCR
//------------------------------------------------------------------------------
double ZNR::zn_AmfHvacCR(		// sensible hvac air requirements
	double tza,			// zone air temp, F
	double tSup) const	// available supply air temp, F
// assume no vent (HVAC active)
// does NOT change ZNR state
// returns dry-air mass flow rate required to hold tza, lbm/hr
{
	double amf = (fabs(tza - tSup) < tol_tF)
		? DBL_MAX
		: (zn_balC1 - zn_balC2 * tza)
			/ (zn_dRpCx * (tza - tSup) * Top.tp_airSH);
	return amf;
}		// ZNR::zn_AmfHvacCR
//-----------------------------------------------------------------------------
double ZNR::zn_AnAmf() const		// current AirNet mass flow rate
// returns dry air mass flow rate into zone (lbm/hr) due to AirNet exchanges, lbm/hr
{
	double amf = (1.f - zn_fVent) * zn_airNetI[ 0].af_amf
	            + zn_fVent        * zn_airNetI[ 1].af_amf;
	return amf;
}		// ZNR::zn_AnAmf
//-----------------------------------------------------------------------------
double ZNR::zn_NonAnIVAmf(	// non-airnet dry air mass flow rate
	double dryAirMass /*=-1.*/) const	// current zone dry air mass, lbm
										//   default = zn_dryAirMass
// Note: non-airnet flows contribute to heat and moisture balance
//       but *NOT* pressure balance (assumed balanced)
// returns dry air mass flow rate (lbm/hr) into zone due to non-AirNet sources
{
	if (dryAirMass < 0.)
		dryAirMass = zn_dryAirMass;
	double amf = i.infAC * dryAirMass;
	if (i.infELA > 0.f)
	{	float amfELA =
	     Top.tp_rhoDryOSh
		  * 60.f
		  * i.infELA		// user input effective leakage area (ft2), hourly variable
		  * sqrt( 			// flow is proportional to square root of
			  zn_stackc * fabs( tzls - Top.tDbOSh) + 	//  stack coeff (see ZNR.zn_InfilSetup) * delta t
			  zn_windc * Top.windSpeedSquaredSh ); 		//  wind coeff (ditto) * wind speed ^ 2

		amf += amfELA;
	}
	return amf;
}		// ZNR::zn_NonAnIVAmf
//------------------------------------------------------------------------------
double ZNR::zn_AirXMoistureBal(		// air change rate and zone moisture balance
	bool bFinal,				// true: set ZNR members to calculated zone state
								// false: use temporary vars only
	double amfX /*=0.*/,		// additional system flow (e.g. air handler), lbm/h
	double mwX /*=0.*/,			// air handler moisture flow, lbm/hr
	double _tz /*=-99.*/,		// alternative air temp, F; default = ZNR tz
	double* pXLGain /*=nullptr*/)	// if non-null, return condensation heat gain, Btu

// see also ZNR::znW() re CNE zone moisture balance

// returns humidity ratio

{
	if (_tz < -98.)
		_tz = tz;

	double rho = psyDenMoistAir(_tz, wzls);		// moist air density, lbm/ft3
	double dryAirMass = max(i.znVol * psyDenDryAir2( rho, wzls), .1);	// dry air mass, lbm

	// air changes due to infil + vent
	// includes flows induced by ducts / HVAC
	//   but not those flows
	// TODO: improve other infil models
	double nonAnIVAmf = zn_NonAnIVAmf( dryAirMass);
	double ivAirX = (zn_AnAmf() + nonAnIVAmf) / dryAirMass;
	if (ivAirX < 0.f)
		ivAirX = 0.f;

	double amfSys = zn_ductLkI.af_amf + zn_sysAirI.af_amf + amfX;
	double airX = ivAirX + amfSys / dryAirMass;

	// internal gain
	double mwIG = znLGain / PsyHCondWtr;		// latent internal gains, lbm/hr

	// non-airnet infil, lbm/hr
	double mwInf = nonAnIVAmf * Top.wOSh;

	// IZXFER (airnet) gains not including duct leakage and HVAC
	double mwAN = (1.f - zn_fVent) * zn_airNetI[0].af_Wmf()
		+ (zn_fVent)*zn_airNetI[1].af_Wmf();

	double mw = mwIG + mwInf + mwAN + mwX	// total water vapor mass flow rate, lbm/hr
		+ zn_ductLkI.af_Wmf() + zn_sysAirI.af_Wmf();

	// TODO: HPWH moisture removal? 2-16

	[[maybe_unused]] int wCase = 0;	// debug aid
					//   0 = time constant OK, result OK
					//   1 = short time constant (steady state sln used), result OK
					//   2 = time constant OK, result limited
					//   3 = short time constant (steady state sln used), result clamped

	double dryAirMassEff = dryAirMass * i.zn_HIRatio;	// effective dry air mass
														// may be adjusted below re short time steps
	float f = airX * Top.tp_subhrDur / i.zn_HIRatio;
	if (f > 1.f)
	{	dryAirMassEff *= f;
		f = 1.f;
		wCase += 1;
	}
	double _wz = mw * Top.tp_subhrDur / dryAirMassEff  + wzls * (1. - f);

	double XLGain = 0.;	// excess gain to space due to "condensation" or "evaporation", Btuh
						// if predicted humidity ratio is physically impossible, assume
						//   magic (sensible) heat transfer
						// + = into zone (= "condensation")
	double wSat = psyHumRat3(_tz);   // maximum w air at this temp can hold
	double wzClamp = bracket(double(PsyWmin), _wz, wSat);
	if (_wz != wzClamp)
	{	XLGain = (_wz - wzClamp) * dryAirMassEff * PsyHCondWtr / Top.tp_subhrDur;
		_wz = wzClamp;
		wCase += 2;		// limits applied
	}
	if (pXLGain)
		*pXLGain = XLGain;

	if (bFinal)
	{	// set ZNR members
		zn_rho = rho;
		zn_dryAirMass = dryAirMass;
		zn_dryAirMassEff = dryAirMassEff;
		zn_ivAirX = ivAirX;
		zn_airX = airX;

		// modified airX re convection coefficients
		if (!i.zn_hcAirXIsSet)	// if not fixed by user input
		{	i.zn_hcAirX = zn_airX;		// note: does NOT include zn_hpwhAirX
			if (zn_IsAirHVACActive() && !zn_HasAirHVAC())
				i.zn_hcAirX += 4.8f;		// additional "virtual" air flow for non-air zones
		}

		wz = _wz;
		znXLGain = XLGain;			// zone total excess latent gain, Btuh

		// latent gains to space
		zn_qlHvac = zn_sysAirI.af_QLat(wzls);
		if (fabs(zn_qlHvac) < .1)
			zn_qlHvac = 0.;		// drop tiny values (re report aesthetics)
#if 0 && defined( _DEBUG)
		if (zn_qlHvac > 0.)
			printf("\nqlHvac > 0");
#endif
		zn_qlIz = (1.f - zn_fVent) * zn_airNetI[0].af_QLat(wzls)
			+ (zn_fVent)*zn_airNetI[1].af_QLat(wzls)
			+ zn_ductLkI.af_QLat(wzls);

		zn_twb = psyTWetBulb(_tz, _wz);
		zn_relHum = psyRelHum3(_wz, wSat);

		// zone humidity ratio now known
		//  finalize return air state
		zn_sysAirO.as_w = wz;

#if 0
x		float relHum = psyRelHum2(tz, zn_twb);
x		if (tz > 50 && fabs(zn_relHum - relHum) > .0001)
x			printf("hit");
#endif

#if 0 && defined( _DEBUG)
x		moisture balance experiment
x		generally does not balance, 5 - 10 - 2012
x		double znLDelta = zn_dryAirMass * (wz - wzls) * PsyHCondWtr;
x		double znLX = (zn_qlHvac + znLGain) * Top.tp_subhrDur;
x		if (frDiff(znLDelta, znLX) > .01)
x			printf("Mismatch\n");
#endif

#if defined( DEBUGDUMP)
		if (DbDo(dbdZM))
		{
			DbPrintf("%s W:  mwIG=%0.3f  mwInf=%0.3f  mwAN=%0.3f  mwDuctLk=%0.3f  mwSys=%0.3f  mwSum=%0.3f\n"
				"     tdb=%0.2f  airX=%0.3f  hcAirX=%0.3f dryAirMass=%0.2f  XLGain=%0.2f  W=%0.6f  twb=%0.2f  rh=%0.4f\n",
				Name(), mwIG, mwInf, mwAN, zn_ductLkI.af_Wmf(), zn_sysAirI.af_Wmf(), mw,
				_tz, zn_airX, i.zn_hcAirX, zn_dryAirMass, znXLGain, wz, zn_twb, zn_relHum);
			DbPrintf("     rho=%0.4f  rho0ls=%0.4f  wzls=%0.6f  dryAirMassEff=%0.2f  qlHvac=%0.2f  qlIz=%0.2f\n",
				zn_rho, zn_rho0ls, wzls, zn_dryAirMassEff, zn_qlHvac, zn_qlIz);
#endif

		}
	}	// bFinal

	return _wz;

}		// ZNR::zn_AirXMoistureBal
//----------------------------------------------------------------------------------
#if defined( OLDHUM)
RC ZNR::zn_AirX(		// total air exchanges etc
	double amfX /*=0.*/,	// additional air flow, lbm
	double _tz /*=-99.*/)	// zone air dry bulb, F
							//   default = current tz
{
	RC rc = RCOK;

	if (_tz < -98.)
		_tz = tz;

	zn_rho = psyDenMoistAir(_tz, wzls);		// moist air density
	zn_dryAirMass = max(i.znVol * psyDenDryAir2(zn_rho, wzls), .1);	// dry air mass, lbm
	zn_dryAirMassEff = zn_dryAirMass;			// effective dry air mass, lbm
												//   zn_AirXMoistureBal() may adjust re short time steps

	// TODO: improve other infil models?
	zn_ivAirX = (zn_AnAmf() + zn_NonAnIVAmf()) / zn_dryAirMass;
	// air changes due to infil + vent
	// includes flows induced by ducts / HVAC
	//   but not those flows
	if (zn_ivAirX < 0.f)
		zn_ivAirX = 0.f;

	double amfSys = zn_ductLkI.af_amf + zn_sysAirI.af_amf + amfX;
	zn_airX = zn_ivAirX + amfSys / zn_dryAirMass;

	// modified airX re convection coefficients
	if (!i.zn_hcAirXIsSet)	// if not fixed by user input
	{
		i.zn_hcAirX = zn_airX;		// note: does NOT include zn_hpwhAirX
		if (zn_IsAirHVACActive() && !zn_HasAirHVAC())
			i.zn_hcAirX += 4.8f;		// additional "virtual" air flow for non-air zones
	}

	return rc;
}		// ZNR::zn_AirX
//-----------------------------------------------------------------------------
double ZNR::zn_HumRat(		// zone moisture ratio
	double amfX,		// additional system flow (e.g. air handler), lbm/h
	double mwX,			// air handler moisture flow, lbm/hr
	double _tz,
	double* pXLGain /*=nullptr*/)

	// returns zone humidity ratio


	// see also znW() re CNE zone moisture balance
{

	if (_tz < -98.)
		_tz = tz;

#if 1
	RC rc = zn_AirX(amfX, _tz);
#else

	double rho = psyDenMoistAir(_tz, wzls);		// moist air density
	double dryAirMass = max(i.znVol * psyDenDryAir2(rho, wzls), .1);	// dry air mass, lbm
	zn_dryAirMass = dryAirMass;
	zn_dryAirMassEff = dryAirMass;			// effective dry air mass, lbm
												//   zn_AirXMoistureBal() may adjust re short time steps

	// TODO: improve other infil models?
	zn_ivAirX = (zn_AnAmf() + zn_NonAnIVAmf()) / dryAirMass;
	// air changes due to infil + vent
	// includes flows induced by ducts / HVAC
	//   but not those flows
	if (zn_ivAirX < 0.f)
		zn_ivAirX = 0.f;

	double amfSys = zn_ductLkI.af_amf + zn_sysAirI.af_amf + amfX;
	zn_airX = zn_ivAirX + amfSys / dryAirMass;

	// modified airX re convection coefficients
	if (!i.zn_hcAirXIsSet)	// if not fixed by user input
	{
		i.zn_hcAirX = zn_airX;		// note: does NOT include zn_hpwhAirX
		if (zn_IsAirHVACActive() && !zn_HasAirHVAC())
			i.zn_hcAirX += 4.8f;		// additional "virtual" air flow for non-air zones
	}
#endif

	// internal gain
	double mwIG = znLGain / PsyHCondWtr;		// latent internal gains, lbm/hr

	// non-airnet infil, lbm/hr
	double mwInf = zn_NonAnIVAmf() * Top.wOSh;

	// IZXFER (airnet) gains not including duct leakage and HVAC
	double mwAN = (1.f - zn_fVent) * zn_airNetI[0].af_Wmf()
		+ (zn_fVent)*zn_airNetI[1].af_Wmf();

	double mw = mwIG + mwInf + mwAN + mwX		// total water vapor mass flow rate, lbm/hr
		+ zn_ductLkI.af_Wmf() + zn_sysAirI.af_Wmf();

	// TODO: HPWH moisture removal? 2-16

	int wCase = 0;	// debug aid
					//   0 = time constant OK, result OK
					//   1 = short time constant (steady state sln used), result OK
					//   2 = time constant OK, result limited
					//   3 = short time constant (steady state sln used), result clamped


	float f = zn_airX * Top.tp_subhrDur / i.zn_HIRatio;
	if (f > 1.f)
	{
		zn_dryAirMassEff *= f;
		f = 1.f;
		wCase += 1;
	}
	double _wz = mw * Top.tp_subhrDur / (zn_dryAirMassEff * i.zn_HIRatio) + wzls * (1. - f);

	double wSat = psyHumRat3(_tz);   // maximum w air at this temp can hold
	double XLGain = 0.;	// excess gain to space due to "condensation" or "evaporation", Btuh
						// if predicted humidity ratio is physically impossible, assume
						//   magic (sensible) heat transfer
						// + = into zone (= "condensation")
	double wzClamp = bracket(double(PsyWmin), _wz, wSat);
	if (_wz != wzClamp)
	{
		XLGain = (_wz - wzClamp) * zn_dryAirMassEff * i.zn_HIRatio * PsyHCondWtr / Top.tp_subhrDur;
		_wz = wzClamp;
		wCase += 2;		// limits applied
	}

	if (pXLGain)
		*pXLGain = XLGain;

	return _wz;

}		// ZNR::zn_HumRat
#endif

//------------------------------------------------------------------------------
RC ZNR::zn_ComfortCR()		// calculate comfort conditions, conv/radiant model
// sets zn_comfPMV7730 and zn_comfPPD7730
{
	RC rc = RCOK;
#ifdef COMFORT_MODEL
	if (zn_pComf)
	{
		float wComf;
		if (i.znComfUseZoneRH)
		{	wComf = wz;
			i.znComfRh = zn_relHum;
		}
		else
			wComf = psyHumRat2( tz, i.znComfRh);

		int ret = zn_pComf->CalcPMV_IP( tz, tr,
						wComf,	i.znComfAirV,
						i.znComfMet, i.znComfClo);
		if (ret==0)
		{	zn_comfPMV7730 = zn_pComf->GetPMV();
			zn_comfPPD7730 = zn_pComf->GetPPD() * 100.f;
		}
		else
			rc = errCrit( WRN, "Zone '%s': Comfort calculation failure", Name());
	}
#endif
	return rc;
}		// ZNR::zn_ComfortCR
//-----------------------------------------------------------------------------
bool ZNR::zn_IsAirHVACActive() const		// determine air motion
// used re convective coefficient determination
//   see SBC::sb_HCZone case C_CONVMODELCH_ASHRAE
// returns true iff air system is operating (causing air motion)
// Note: relies on zn_qsHvac not initialized (prior step value persists)
{
	// TODO: could enhance re other air movers (e.g. whole house fans)
	return zn_qsHvac != 0.;
}		// ZNR::zn_IsAirMovingMech
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// class RSYS
///////////////////////////////////////////////////////////////////////////////

/*virtual*/ RSYS::~RSYS()
{
	rs_DeleteWorkingSubObjects();
	delete rs_pCHDHW;
	rs_pCHDHW = nullptr;

}	// RSYS::~RSYS
//----------------------------------------------------------------------------
void RSYS::rs_DeleteWorkingSubObjects()	// delete runtime subobjects
// called from d'tor and as insurance from Copy()
{
	delete rs_pPMACCESS[0];
	delete rs_pPMACCESS[1];
	rs_pPMACCESS[0] = rs_pPMACCESS[1] = nullptr;
	delete rs_pCHDHW;
	rs_pCHDHW = nullptr;
}		// RSYS::rs_DeleteWorkingSubOjects
//----------------------------------------------------------------------------
/*virtual*/ void RSYS::Copy( const record* pSrc, int options/*=0*/)
{
	rs_DeleteWorkingSubObjects();
	rs_desc.Release();
	record::Copy( pSrc, options);
	rs_desc.FixAfterCopy();
}		// RSYS::Copy
//-------------------------------------------------------------------------------
RC RSYS::rs_CkF()
{
	static_assert(RSYSMODES == rsmCOUNT - 1);		// cndefn.h #define must be consistent
													//   with cnrecs.def enum
	int rc = RCOK;

	rc |= rs_CkFHeating();

	rc |= rs_CkFCooling();

	// fan
	// fanTy check not great: unsupported choices such as RELIEF are recognized.
	//  If unknown type is given, the error msg lists unsupported types.
	//  Deemed good enough.
	if (rs_fan.fanTy != C_FANTYCH_BLOWTHRU
	 && rs_fan.fanTy != C_FANTYCH_DRAWTHRU)
		rc |= oer("rsFanTy must be BLOWTHRU or DRAWTHRU");

	// OAV
	if (rs_OAVType == C_RSYSOAVTYCH_NONE)
		rc |= disallowN( "when rsOAVType=None",
				RSYS_OAVRELIEFZI, RSYS_OAVTDBINLET, RSYS_OAVTDIFF,
				RSYS_OAVAVFDS, RSYS_OAVAVFMINF, RSYS_OAVFANSFP,
				0);
	else
	{	const char* whenOAV = "when rsOAVType is given";
		rc |= requireN( whenOAV, RSYS_OAVRELIEFZI, RSYS_OAVAVFDS, 0);
		if (IsVal( RSYS_OAVTDIFF) && rs_OAVTdiff < 2.f)
			oWarn( "Dubious rsOAVTdiff (%0.2f F) may cause inadvertent vent heating."
				   "\n    rsOAVTdiff is typically >= 5 F to allow for fan heat and duct gains",
					rs_OAVTdiff);
		// rsOAVFanSFP (W/cfm): error if >5, warn if >2
		rc |= limitCheck( RSYS_OAVFANSFP, 0., 5., 0., 2.);
	}

	return rc;
}	// RSYS::rs_CkF
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFHeating()
{
	RC rc = RCOK;

	rc |= rs_CkFAuxHeat();		// check aux heat inputs

	// ASHP heating FNs (all types)
	static constexpr int16_t ASHP_HtgFNs[]{ RSYS_HSPF, RSYS_CAP47, RSYS_COP47,
		RSYS_CAP35, RSYS_COP35, RSYS_CAP17, RSYS_COP17, RSYS_ASHPLOCKOUTT,
		RSYS_CAPRAT1747, RSYS_CAPRAT9547, RSYS_CAPRATCH, RSYS_CDH,
		RSYS_DEFROSTMODEL, RSYS_PERFMAPHTGI, 0 };

	// ASHPPM (VCHP3) active FNs
	static constexpr int16_t ASHPPM_HtgFNs[]{ 0 };

	if (!rs_CanHeat())
	{
		const char* whenTyNoHt = strtprintf("when rsType=%s (heating not available)",
			getChoiTx(RSYS_TYPE));
		rc |= ignoreX(whenTyNoHt, RSYS_CAPH, RSYS_FANSFPH, RSYS_FXCAPHTARG,
			RSYS_AFUE, RSYS_CAPNOMH, RSYS_TDDESH, RSYS_FEFFH, RSYS_CHDHWSYSI,
			ASHP_HtgFNs, ASHPPM_HtgFNs);
		FldSet(RSYS_CAPNOMH, 0.f);		// insurance

		return rc;
	}
	const char* whenTy = strtprintf("when rsType=%s", getChoiTx(RSYS_TYPE));

	if (!rs_IsCHDHW())
	{
		disallow(whenTy, RSYS_CHDHWSYSI);
	}

	if (rs_IsFanCoil())
	{	// fancoil htg
		rc |= require(whenTy, RSYS_CAPH);
		rc |= ignoreX(whenTy, RSYS_AFUE, RSYS_CAPNOMH, RSYS_FEFFH,
			ASHP_HtgFNs, ASHPPM_HtgFNs);
		rs_AFUE = 0.f;
	}
	else if (rs_IsWSHP())
	{
		rc |= requireN(whenTy, RSYS_TDBOUT, 0);

		rc |= disallowN(whenTy, RSYS_AFUE, 0);

		if (!IsSet(RSYS_CAPH) && !IsSet(RSYS_CAP95))
			rc |= oer("at least one of rsCapH and rsCapC must be specified %s", whenTy);

		if (IsSetCount(RSYS_CAPH, RSYS_CAP95, RSYS_CAPRATCH, 0) == 3
		 && !(IsAusz(RSYS_CAPH) && IsAusz(RSYS_CAP95)))
			// cannot give all 3 values unless both caps are autosized
			rc |= oer("rsCapH, rsCapC, and rsCapRatCH cannot all be specfied %s", whenTy);

		rc |= ignoreX(whenTy, RSYS_HSPF, RSYS_CAP47,
			RSYS_CAP35, RSYS_COP35, RSYS_CAP17, RSYS_COP17, RSYS_ASHPLOCKOUTT,
			RSYS_CAPRAT1747, RSYS_CAPRAT9547, RSYS_DEFROSTMODEL, ASHPPM_HtgFNs);
	}
	else if (rs_IsASHP())
	{
		if (!IsSet(RSYS_CAP47) && !IsSet(RSYS_CAP95))
			rc |= oer("at least one of rsCap47 and rsCapC must be specified %s", whenTy);

		if (IsSetCount(RSYS_CAP47, RSYS_CAP95, RSYS_CAPRAT9547, 0) == 3
		 && !(IsAusz(RSYS_CAP47) && IsAusz(RSYS_CAP95)))
			// cannot give all 3 values unless both caps are autosized
			rc |= oer("rsCap47, rsCapC, and rsCapRat9547 cannot all be specfied %s", whenTy);

		if (rs_IsASHPHydronic())
		{	// ASHPHydronic: air-to-water heat pump
			//   approximated with air-to-air model
			rc |= requireN(whenTy, RSYS_COP47, RSYS_COP17, 0);
			rc |= disallowN(whenTy, RSYS_HSPF, RSYS_CAPH, RSYS_AFUE, RSYS_DEFROSTMODEL, 0);
			if (!IsSet(RSYS_FANSFPH))
				rs_fanSFPH = 0.f;		// hydronic: no fan power
		}
		else
		{	// ASHP other than ASHPHydronic

			rc |= disallowN(whenTy, RSYS_CAPH, RSYS_AFUE, 0);

			if (rs_IsASHPPkgRoom())
			{
				rc |= disallowN(whenTy, RSYS_HSPF,
					RSYS_CAP17, RSYS_COP17, RSYS_CAP35, RSYS_COP35, 0);
				rc |= requireN(whenTy, RSYS_COP47, 0);

				if (!IsSet(RSYS_FANSFPH))
					rs_fanSFPH = 0.f;	// fan power is included in primary by default

				// other defaults derived in rs_TopRSys1()
			}
			else
			{	// ASHP (non-hydronic, non-pkgroom): only HSPF required
				//   capacities defaulted from cooling cap95
				//   COPs defaulted from HSPF
				//   if both COP47 and COP17 are specified, HSPF is not used

				rc |= requireN(whenTy, RSYS_HSPF, 0);

				if (rs_IsASHPPM())
				{
					require(whenTy, RSYS_PERFMAPHTGI);

					rc |= disallowN( whenTy,
						RSYS_CAP17, RSYS_COP17, RSYS_CAPRAT1747,
						RSYS_CAP35, RSYS_COP35,
						RSYS_COP47,	0);
				}
				else
				{
					if (IsAusz(RSYS_CAP47))
						rc |= disallowN("when rsCap47 is AUTOSIZE",
						RSYS_CAP17, RSYS_CAP35, 0);
					else if (IsSet(RSYS_CAP17))
							rc |= disallowN("when rsCap17 is given", RSYS_CAPRAT1747, 0);
				}
			}
		}
	
		// all air source heat pumps
		if (!IsAusz(RSYS_CAP47))
			// rs_cap47 not AUTOSIZEd (altho may be expression), use it as rs_capNomH default
			//   see DefaultCapNomsIf()
			FldCopyIf(RSYS_CAP47, RSYS_CAPNOMH);
		// else bug if additional run(s) have autosize: rs_capNomH has lingering value from prior rs_cap47
	}
	else if (rs_IsCHDHW())
	{	// combined heat and DHW
		rc |= requireX(whenTy, RSYS_CHDHWSYSI);
		rc |= disallowX(whenTy, RSYS_TDDESH);
		if (IsAusz( RSYS_CAPH))
			rc |= oer("rsCapH cannot be AUTOSIZE %s", whenTy);
		else
			rc |= ignoreX(whenTy, RSYS_CAPH);
		// rs_CdH?
		rc |= disallowX("when rsType is CombinedHeatDHW or ACCombinedHeatDHW",
				ASHP_HtgFNs, ASHPPM_HtgFNs);
	}
	else
	{	// not CHDHW or HP of any type
		rc |= disallowX( whenTy, ASHP_HtgFNs, ASHPPM_HtgFNs);

		// default AFUE (CULT default = 0)
		if (!IsSet(RSYS_AFUE))
			rs_AFUE = rs_IsElecHeat() ? 1.f : 0.9;

		if (!IsAusz(RSYS_CAPH))
			// rs_capH not AUTOSIZEd (altho may be expression), use it as rs_capNomH default
			FldCopyIf(RSYS_CAPH, RSYS_CAPNOMH);
	}

	return rc;
}	// RSYS::rs_CkFHeating
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFCooling()
{
	// compression cooling FNs
	static constexpr int16_t Clg_FNs[] = { RSYS_SEER, RSYS_EER95, RSYS_COP95,
		RSYS_FCHGC, RSYS_CDC, 0 };

	// FNs meaningful only for variable capacity
	//   ignored for non-VC ASHP
	//   disallowed for others
	static constexpr int16_t PM_ClgFNs[] = { RSYS_PERFMAPCLGI, 0 };

	int rc = RCOK;

	if (!rs_CanCool())
	{	// no cooling capability
		const char* whenNoCl = strtprintf("when rsType=%s (cooling not available)",
			getChoiTx(RSYS_TYPE));
		rc |= ignoreX(whenNoCl, RSYS_CAP95, RSYS_FANSFPC, RSYS_CAPNOMC, RSYS_VFPERTON,
			Clg_FNs, PM_ClgFNs);
		FldSet(RSYS_CAPNOMC, 0.f);		// insurance
		return rc;
	}

	const char* whenTy = strtprintf("when rsType=%s", getChoiTx(RSYS_TYPE));
	if (rs_IsPkgRoom())
	{
		rc |= requireN(whenTy, RSYS_EER95, 0);
		rc |= disallowN(whenTy, RSYS_SEER, RSYS_COP95, 0);
		if (!IsSet(RSYS_FANSFPC))
			rs_fanSFPC = 0.f;	// fan power is included in primary by default
	}
	else if (rs_IsFanCoil())
	{	// fancoil cooling
		rc |= require(whenTy, RSYS_CAP95);
		rc |= ignoreX(whenTy, Clg_FNs, PM_ClgFNs);
	}
	else if (rs_IsWSHP())
	{	// note checks of RSYS_TDBOUT and RSYS_CAP95 in rs_CkFHeating()

		rc |= requireN(whenTy, RSYS_EER95, 0);

		rc |= ignoreX(whenTy, RSYS_SEER, PM_ClgFNs);
	}
	else
		rc |= requireN(whenTy, RSYS_SEER, 0);

	if (!IsAusz(RSYS_CAP95))
	{	// rs_cap95 not AUTOSIZEd (altho may be expression), use as rs_capNomC default
		FldCopyIf(RSYS_CAP95, RSYS_CAPNOMC);
	}

	if (rs_IsPMClg())
	{
		require(whenTy, RSYS_PERFMAPCLGI);
		disallowN(whenTy, RSYS_EER95, RSYS_COP95, 0);
	}
	else
	{
		rc |= disallowX("when rsType is not ASHPPM, ACPM, ACPMFurnace, or ASPMResistance",
			PM_ClgFNs);
	}

	return rc;

}		// RSYS::rs_CkFCooling
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFCooling2()		// additional cooling checks
// call after all values set (including calculated defaults)
// returns RCOK if all check passed
//         else nz value
{
	RC rc = RCOK;

	if (!rs_IsFanCoil() && !rs_IsWSHP() && !rs_IsPMClg())
	{	// check that SEER and EER are plausible
		if (rs_SEER <= rs_EER95)
			rc |= oer("rsSEER (%g) must be > rsEER (%g)", rs_SEER, rs_EER95);
	}

	return rc;
}	// RSYS::rs_CkFCooling2
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFCd(	// default and check Cd values
	int mode)	// rsmCOOL or rsmHEAT
// returns RCOK iff rs_CdX is acceptable
//    else !RCOK (user input out of range)
{
	RC rc = RCOK;
	if (mode == rsmCOOL)
	{	if (!IsSet(RSYS_CDC))
			rs_CdC = rs_IsPMClg() || rs_IsWSHP()  ? 0.25f : 0.f;
		else
			rc |= limitCheckFix(RSYS_CDC, 0.f, .5f);
	}
	else
	{	// not rsmCOOL, assume rsmHEAT
		if (!IsSet(RSYS_CDH))
			rs_CdH = rs_IsASHPHydronic() || rs_IsASHPPkgRoom() || rs_IsASHPPM() || rs_IsWSHP()
				? 0.25f		// hydronic / pkgRoom / WSHP: no HSPF source for default
							// ASHPPM: relationship to HSPF not known
				: bracket(.05f, .25f - 0.2f*(rs_HSPF - 6.8f) / (10.f - 6.8f), .25f);
		else
			rc |= limitCheckFix(RSYS_CDH, 0.f, .5f);
	}
	return rc;
}		// RSYS::rs_CkFCd
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFAuxHeat()	// check aux heat
{
	RC rc = RCOK;
	rs_capAuxHInp = 0.f;
	rs_effAuxH = 1.f;		// insurance
	if (!rs_CanHaveAuxHeat())
	{	rc |= disallowN("when rsType is not ASHP, ASHPPM, ASHPHydronic, or ASHPPkgRoom",
			RSYS_TYPEAUXH, RSYS_CAPAUXH, RSYS_FXCAPAUXHTARG, RSYS_AFUEAUXH,
		    RSYS_CTRLAUXH, 0);
		rs_AFUEAuxH = 0.f;		// clear possibly confusing default
	}
	else if (rs_typeAuxH == C_AUXHEATTY_NONE)
	{	rc |= disallowN("when rsTypeAuxH = None",
			RSYS_CTRLAUXH, RSYS_CAPAUXH, RSYS_FXCAPAUXHTARG, RSYS_AFUEAUXH, 0);
		rs_capAuxH = 0.f;
	}
	else if (!IsAusz(RSYS_CAPAUXH) && rs_capAuxH == 0.f)
	{	// no auxiliary
		rs_typeAuxH = C_AUXHEATTY_NONE;
	}
	else
	{	// save input value of rs_capAuxH
		//   WHY: rs_capAuxH changed during HP autosize
		//        saved value used to restore original
		rs_capAuxHInp = rs_capAuxH;
		if (IsAusz(RSYS_CAPAUXH))
			rs_capAuxH = 0.f;		// overwrite NANDLE
		if (rs_IsFuelAuxH())
		{
			if (!IsSet(RSYS_AFUEAUXH))
				rs_AFUEAuxH = 0.9f;		// change CULT default (1) to value
										//  appropriate for furnace
			rs_effAuxH = rs_AFUEAuxH;
			if (!IsSet(RSYS_CTRLAUXH))
				rs_ctrlAuxH = C_AUXHEATCTRL_ALT;
		}
	}

	return rc;
}		// RSYS::rs_CkFAuxHeat
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFRatio(
	int fn1,	// field 1
	int fn2,	// field 2
	int fnRat,	// alternate: ratio field
	float vMin,	// min allowed value for v1/v2 or vRat
	float vMax)	// max allowed value for v1/v2 or vRat
// Expects either (fn1 and fn2) OR fnRat to be set (but not all 3)
// Prior checking must verify that (see e.g. rs_CkF)
//       mutua
{
	RC rc = IsSet( fn1)
		    ? limitCheckRatio(fn1, fn2, vMin, vMax)
			: limitCheck(fnRat, vMin, vMax);
	return rc;
}		// RSYS::rs_CkFRatio
//-----------------------------------------------------------------------------
#if 0
// incomplete idea, 11-21
RC RSYS::rs_CheckCapAuxH()		// check for sufficient aux heat capacity
// issues msg if rs_capAuxH is too small to be useful
{
	RC rc = RCOK;
	if (rs_capAuxH > 0.f && rs_ctrlAuxH != C_AUXHEATCTRL_CYCLE)
	{	// 1. user has provided aux capacity
		// 2. aux and primary will not run simultaneously
		float capMax = std::max({ rs_cap47, rs_cap35, rs_cap17, rs_cap05 });

		if (rs_capAuxH <= capMax)
			rc |= oWarn("Insufficient auxiliary capacity");
	}
	return rc;
}		// RSYS::rs_CheckCapAuxH
#endif
//-----------------------------------------------------------------------------
RC RSYS::rs_GetAndCheckPERFORMANCEMAP(
	int fn,						// field number of PERFORMANCEMAP reference
	const PERFORMANCEMAP*& pPM)

{
	RC rc = RCOK;

	int iPM = FldValInt(fn);

	pPM = PerfMapB.GetAtSafe( iPM);

	if (!pPM)
		// unexpected due to require() in rs_CkfHeating / rs_CkfCooling
		rc = oer("Internal error: PERFORMANCEMAP referenced by %s not found (TI=%d)",
			mbrIdTx(fn), iPM);

	return rc;
}		// RSYS::rs_GetAndCheckPERFORMANCEMAP
//-----------------------------------------------------------------------------
RC RSYS::rs_TopRSys1()		// check RSYS, initial set up for run
{
	RC rc = RCOK;

	if (!IsSet(RSYS_TDDESH))
		rs_tdDesH = rs_IsHP()    ? 30.f	// lower default temp rise for ASHP
		                         : 50.f;	// (changed later for CHDHW)

	if (rs_IsASHPPkgRoom())
	{	if (!IsSet(RSYS_ASHPLOCKOUTT))
			rs_ASHPLockOutT = 45.f;		// pkg room ASHP: use resistance at lower temps
		rc |= rs_SetupASHP();
	}

	if (rs_IsVC())
	{	// VCHP default motor type is BPM (others are PSC from cult)
		if (!IsSet(RSYS_FAN + FAN_MOTTY))
			rs_fan.fn_motTy = C_MOTTYCH_BPM;
	}

	if (rs_CanCool())
	{	// cooling model air flow correlations have limited validity range
		//   verify air flow 150 - 550 cfm/ton (per Proctor Engineering)
		rc |= limitCheck(RSYS_VFPERTON, 150., 550.);

		// rsFanPwrC (W/cfm): error if >5, warn if >2
		rc |= limitCheck(RSYS_FANSFPC, 0., 5., 0., 2.);

		if (rs_IsFanCoil())
		{	// nothing to check?

		}
		else
		{	// compression cooling

			rc |= rs_CkFCd(rsmCOOL);

			if (rs_IsWSHP())
			{
				rc |= limitCheck(RSYS_CAPRATCH, 0.3, 2.);

			}
			else if (rs_IsPMClg())
			{	// variable speed: most info is embodied in performance map
				rc |= rs_CheckAndSetupPMClg( pmCHECKONLY);
			}
			else
			{	// default/harmonize 95 F COP
				//   inter-default rs_COP95 <-> rs_EER95
				//   if both input, values can be different
				//   rs_COP95 used for VCHP2, rs_EER95 used for single speed
				//   EER can default from SEER
				if (rs_IsPkgRoom())
				{	// pkg: derive SEER from EER
					//   abram conant fit, 6-20
					rs_SEER = 1.07f * rs_EER95;
					// rs_COP95 not allowed
				}
				else if (IsSet(RSYS_EER95))
				{
					if (!IsSet(RSYS_COP95))
						rs_COP95 = rs_EER95 / BtuperWh;
				}
				else
				{
					if (IsSet(RSYS_COP95))
						rs_EER95 = rs_COP95 * BtuperWh;
					else
					{	// estimate missing EER from SEER
						//   California ACM method
						rs_EER95 = rs_SEER < 13.f ? 10.f + 0.84f * (rs_SEER - 11.5f)
							: rs_SEER < 16.f ? 11.3f + 0.57f * (rs_SEER - 13.f)
							: 13.f;
						rs_COP95 = rs_EER95 / BtuperWh;
					}
				}
			}
		}

		// final rs_CanCool() checks: EER vs SEER and cap82 vs cap95 vs cap115
		rc |= rs_CkFCooling2();
	}

	if (IsAusz( RSYS_CAP95))
	{	rs_auszC.az_active = TRUE;
		rs_fxCapCAsF = 1.2f;	// working oversize factor
								//   ensures sufficient capacity
								//   during autosize search
	}

	if (rs_CanHeat())
	{
		// rsFanPwrH (W/cfm): error if >5, warn if >2
		rc |= limitCheck(RSYS_FANSFPH, 0., 5., 0., 2.);

		if (rs_IsHP())
		{
			rc |= rs_CkFCd(rsmHEAT);

			if (IsAusz(RSYS_CAPH) || IsAusz(RSYS_CAP47) || IsAusz(RSYS_CAPAUXH))
			{	// rs_capH for WSHP, rs_cap47 for ASHP
				rs_auszH.az_active = TRUE;	// ASHP autosizes rs_capH
											//   capacities derived in rs_AuszFinal
				rs_fxCapHAsF = 1.2f;		// working oversize factor
			}

			if (rs_IsASHP())
			{
				if (rs_IsPMHtg())
					rc |= rs_CheckAndSetupPMHtg(pmCHECKONLY);
				else
					rc |= rs_CkFRatio(RSYS_CAP17, RSYS_CAP47, RSYS_CAPRAT1747, .2f, 1.2f);
			}
		}
		else if (IsAusz(RSYS_CAPH))
		{	// other checking?
			rs_auszH.az_active = TRUE;
			rs_fxCapHAsF = 1.4f;
		}
	}

	// loop all zones served by this RSYS
	rs_areaServed = 0.;
	rs_zonesServed = 0;
	ZNR* zp;
	RLUPC( ZrB, zp, rs_IsZoneServed( zp))
	{	rs_areaServed += zp->i.znArea;
		rs_zonesServed++;
	}

	return rc;
}		// RSYS::rs_TopRSys1
//-----------------------------------------------------------------------------
RC RSYS::rs_TopRSys2()		// final set up for run
{
	RC rc = RCOK;

	rs_SetWorkingPtrs();		// MTR and other inter-object pointers

	// Initialize DUCTSEGs / check for consistency with rs_DSEH/rs_DSEC
	// link DUCTSEGs to this RSYS
	DUCTSEG* ds;
	memset( rs_ducts, 0, sizeof( rs_ducts));
	RLUP( DsR, ds)
	{	ds->ds_SetRunConstants();
		if (ds->ownTi == ss)
		{	// if duct segment part of this system
			int iSR = !ds->ds_IsSupply();	// 1=return 0=supply
			if (!rs_CanHaveDucts( 0) && !rs_CanHaveDucts( 1))
			{	ZNR* zpx = ds->ds_GetExZone();
				if (zpx && ds->ds_exArea > 0.f)
					oWarn( "DUCTSEG '%s' is unused (rsDSEH and rsDSEC both given)\n"
					       "    but its surface area is included in ZONE '%s'",
						ds->Name(), zpx->Name());
				else
					oInfo( "DUCTSEG '%s' is unused (rsDSEH and rsDSEC both given)\n"
						"    and has no effect",
						ds->Name());
			}
			else for (int iHC=0; iHC<2; iHC++)
			{	int iDS = rs_Dsi( iSR, iHC);
				if ( iDS > 0)
				{	rc |= oer( "DUCTSEG trouble -- more than one %s duct",
							iSR ? "return" : "supply");
					break;
				}
				if (rs_CanHaveDucts( iHC))
					rs_ducts[ iHC].dsi[ iSR] = ds->ss;
			}
		}
	}
	
	// combined heat / DHW
	if (rs_IsCHDHW())
		rc |= rs_SetupCHDHW();

	if (rc == RCOK)
		rs_SetRunConstants();

	return rc;
}		// RSYS::rs_TopRSys2
//-----------------------------------------------------------------------------
RC RSYS::rs_FazInit(		// init before autosize (once) and main sim
	int isAusz)		// TRUE = autosize, FALSE = main simulation
{
	RC rc = RCOK;

	// heating
	rs_isAuszH = rs_auszH.az_fazInit(&rs_capH, FALSE, this, RSYS_CAPH, isAusz, "RSYS[%s] capH");
	rc |= rs_SetupCapH( -1.f, rs_isAuszH ? 1 : 0);

	// cooling
	rs_isAuszC = rs_auszC.az_fazInit(&rs_cap95, FALSE, this, RSYS_CAP95, isAusz, "RSYS[%s] cap95");
	rc |= rs_SetupCapC( -1.f, rs_isAuszC ? 1 : 0);

	return rc;
}		// RSYS::rs_FazInit
//-----------------------------------------------------------------------------
RC RSYS::rs_RddInit( int isAusz)	// init before each autosize design day and main sim
{
	isAusz;
	return RCOK;
}		// RSYS::rs_RddInit
//-----------------------------------------------------------------------------
void RSYS::rs_RddiInit()		// init before each autosize design day ITERATION
{
// TODO: redundant calls occur and must be harmless!

	int auszMode = rs_IsAutoSizing();
	if (auszMode == rsmHEAT)		// if autosizing something
	{	if (Top.tp_pass1A)
		{	// pass1A: warmup with fixed air flow
			float cfmPerFt2 = rs_IsHP() ? 0.6f : 0.4f;
			rs_SetupCapH( rs_areaServed * cfmPerFt2);		// rs_capH derived from AVF
		}
		else
		{	// pass1B: rs_capH has latest value
			if (Top.tp_auszDsDayItr < 2)
				rs_capH = rs_auszH.az_a;	// 1st iteration cap may be 0 (see rs_pass1AtoB)
			rs_SetupCapH();
		}
	}
	else if (auszMode == rsmCOOL)
	{	if (Top.tp_pass1A)
		{	// pass1A: warmup with fixed air flow
			constexpr float cfmPerFt2 = 0.6f;
			rs_amfC = AVFtoAMF( rs_areaServed * cfmPerFt2);
			rs_SetupCapC( rs_areaServed * cfmPerFt2);		// rs_cap95 derived from AVF
		}
		else
		{	// pass1B: rs_cap95 has latest value
			if (Top.tp_auszDsDayItr < 2)
				rs_cap95 = rs_auszC.az_a;	// 1st iteration cap may be 0 (see rs_pass1AtoB)
			rs_SetupCapC();
		}
	}
}		// RSYS::rs_RddiInit
//-----------------------------------------------------------------------------
RC RSYS::rs_endP1DsdIter(		// autosizing end of day
	int auszMode)		// rsmHEAT, rsmCOOL
{
	RC rc = RCOK;
	if (auszMode == rsmHEAT)
	{	// note: rs_fxCapHDay = 20 if no load
		float f = rs_fxCapHAsF / max( rs_fxCapHDay, .01f);
		if (Top.tp_pass1A)
		{	// pass1A (warmup): system always has fixed size
			//   after 1st iter, change rs_capH at will (used only for convergence test)
			//   rs_capH re-inited in rs_RddiInit
			if (Top.tp_auszDsDayItr > 1)
				rs_capH *= f;
		}
		else
		{	// pass1B: rs_capH sizes system
			//   change in small steps (can be unstable)
			rs_capH *= bracket( 0.9f, f, 1.1f);

			// never allow smaller than 1000 Btuh or any prior pass1B result
			if (rs_capH < 1000.f)
				rs_capH = 1000.f;
			else if (rs_capH < rs_auszH.az_b)
				rs_capH = rs_auszH.az_b;
		}

		setToMax( rs_auszH.ldPk, rs_capH);	// peak

		rs_amfH = rs_AMFForHtgCap( rs_capH);	// consistent AMF
	}
	else if (auszMode == rsmCOOL)
	{	// note: rs_fxCapCDay = 20 if no load
		float f = rs_fxCapCAsF / max( rs_fxCapCDay, .01f);
		if (Top.tp_pass1A)
		{	// pass1A (warmup): system always has fixed size
			//   after 1st iter, change rs_cap95 at will (used only for convergence test)
			//   rs_cap95 re-inited in rs_RddiInit
			if (Top.tp_auszDsDayItr > 1)
				rs_cap95 *= f;
		}
		else
		{	// pass1B: rs_cap95 sizes system
			//   change in small steps (can be unstable)
			// when f is close to 1, move very slowly
			//   err = 0.01 -> fX = .001
			//   err = 0.02 -> fX = .004
			//   err = 0.1 -> fX = .1
			float err = f - 1.f;
			float errX = err * fabs( err) * 10.f;
			float fX = bracket( .9f, errX+1.f, 1.1f);
			rs_cap95 *= fX;

			float cap95min = max(rs_auszC.az_b, 1200.f);
			if (rs_cap95 < cap95min)
				rs_cap95 = cap95min;	// never < 0.1 ton
										// never < previous design day peak
		}

		setToMax( rs_auszC.ldPk, rs_cap95);	// peak

		rs_amfC = rs_AMFForClgCap( rs_cap95);	// consistent AMF
	}
	return rc;

}	// RSYS::rs_endP1DsdIter
//-----------------------------------------------------------------------------
RC RSYS::rs_pass1AtoB()	// call at transition from autoSize pass 1 part A to part B for each design day

// called between iterations of a design day (between tp_SimDays()'s without reInit)
// at the change from const-temp size-finding open-ended models to the real models.
{
	// at entry, value is set to max converged part A value (.a).

	// here do any optimizations to make value a better estimate of rated size.

	// on return, caller will increase value to any larger converged part B value (.b)
	// then call RSYS::begP1b, next.

	int auszMode = rs_IsAutoSizing();
	if (auszMode == rsmCOOL)
		rs_cap95 = 0.f;
	else
		rs_capH = 0.f;

	return RCOK;
}			// RSYS::rs_pass1AtoB
//-----------------------------------------------------------------------------------------------------------------------------
RC RSYS::rs_begP1b()		// called b4 start of pass 1 part b iterations for des day, after value set to max part B value seen.
{
// reSetup models being autoSized, cuz AUSZ stuff and/or rs_pass1AtoB above may have changed values.
#if 0
x	RC rc = rs_SetupSizes();			// just above
x	return rc;
#else
	return RCOK;
#endif
}			// RSYS::rs_begP1b
//-----------------------------------------------------------------------------
RC RSYS::rs_endAutosize()	// call at end successful autoSize for possible additional checks/messages
{
	return RCOK;
}		// rs_endAutosize
//-----------------------------------------------------------------------------
void RSYS::rs_AuszFinal()		// called at end of successful autosize (after all xx_endAutosize)
{

	RSYS* rsi = RSiB.GetAtSafe( ss);

	// member function sets _As, _AsNov, and input record members for each AUSZ's 'x'.

	if (rs_isAuszC)
		rs_cap95 = rs_fxCapCTarg * rs_auszC.ldPkAs1 / rs_fxCapCAsF;
			// max of design days w/o working oversize factor
			//    apply user oversize factor

	if (rs_isAuszH)
	{	float capHBase = rs_auszH.ldPkAs1 / rs_fxCapHAsF;	// max of all design days
															//  remove working cap factor

		float capHDes = capHBase * rs_fxCapHTarg;		// apply user's oversize factor

		if (rs_IsASHP())
		{
			if (IsAusz(RSYS_CAP47))
			{	// find cap47 that produces required output
				// (re-derives dependent values)
				rs_speedF = rs_GetSpeedFRated(0);
				rs_SizeHtASHP(capHDes, Top.heatDsTDbO);	// sets rs_capH
#if defined( _DEBUG)
				// capacity consistency check
				float capCheck = rs_CapEffASHP(Top.heatDsTDbO);
				float capExpected = Top.heatDsTDbO >= rs_ASHPLockOutT
					? capHDes
					: rs_fanHeatH;	// fan heat only when lockout
				if (frDiff(capCheck, capExpected, 1.f) > .001f)
					oWarn("ASHP heating capacity (%.1f) autosize mismatch (expected %.1f)",
						capCheck, capExpected);
#endif
			}
		}
		else
			rs_capH = capHDes;		// non-heat pump

		if (IsAusz(RSYS_CAPAUXH))
		{	// ASHP aux heat autosize cap = full load w/ user oversize
			rs_capAuxH = capHBase * rs_fxCapAuxHTarg;
		}
	}

	// re rs_capAuxH NOT autosized
	//  restore input value
	//  WHY: HP autosize process modifies
	if (!IsAusz(RSYS_CAPAUXH))
		rs_capAuxH = rs_capAuxHInp;		// value as input
	rsi->rs_capAuxH = rs_capAuxH;	// copy back to input record

	if (rs_IsASHP())
	{	if (rs_isAuszC)
		{	if (IsAusz( RSYS_CAP47))
			{	// both autosized
				ASHPConsistentCaps( rs_cap95, rs_cap47, IsSet(RSYS_CAPRAT9547), rs_capRat9547);
			}
			// else leave rs_cap95 autosized, rs_cap47 as input
		}
		rs_SetupASHP();		// default other params as needed
		rsi->rs_cap47 = rs_cap47;	// copy back to input record  WHY?

	}
	else if (rs_IsWSHP())
	{	if (rs_isAuszC)
		{
			if (IsAusz(RSYS_CAPH))
			{	// both autosized
				WSHPPerf.whp_ConsistentCaps(rs_cap95, rs_capH, IsSet(RSYS_CAPRATCH), rs_capRatCH);
			}
			// else leave rs_cap95 autosized, rs_capH as input
		}

		rs_SetupWSHP();		// default other params as needed

	}

	// all capacities now known
	if (rs_isAuszC)
	{	float coolOver = 1.f;			// no coiling oversize (rs_fxCapC included in process)
		rs_auszC.az_final( this, rsi, coolOver);
	}
	if (rs_isAuszH)
	{	float heatOver = 1.f;			// ditto heating
		rs_auszH.az_final( this, rsi, heatOver);
	}

#if 0
x NO -- leave to allow reporting of load at design temp
x	// insurance: rs_capH used during ASHP autosize but s/b 0 for simulation
x	if (rs_IsASHP())
x		rsi->rs_capH = rs_capH = 0.f;
#endif

	rs_DefaultCapNomsIf();		// capture possible nominal cap change(s)
								//   no calc effect
								//   insurance: no rs_auszX.final() changes expected
								//      (coolOver = heatOver = 1)

}		// RSYS::rs_AuszFinal
//-----------------------------------------------------------------------------
float RSYS::rs_ClgCapNomTons(	// nominal cooling capacity
	float nearest /*=-1.f*/)		// if >0, round to nearest specified increment
									//  e.g. 0.5 = round to nearest .5 ton
// returns capacity, tons (1 ton = 12000 Btuh)
{
	float capNomTons = fabs( rs_cap95)/12000.f;
	if (nearest > 0.f)
		capNomTons = nearest*floor( 0.5f + capNomTons / nearest);
	return capNomTons;
}		// RSYS::rs_ClgCapNomTons
//-----------------------------------------------------------------------------
float RSYS::rs_ClgCapForAMF(
	float amf) const	// air mass flow rate, lbm/hr
// returns rated total capacity at 95F, Btuh
{	float avf = AMFtoAVF( amf);		// vol flow, cfm std air
	float cap95 = 12000.f * avf / rs_vfPerTon;
	return cap95;
}		// RSYS::rs_ClgCapForAMF
//-----------------------------------------------------------------------------
float RSYS::rs_AMFForClgCap(
	float cap95) const	// total cooling capacity at 95F, Btuh
// returns air mass flow, lbm/hr
{
	float avf = cap95 * rs_vfPerTon / 12000.f;
	float amf = AVFtoAMF( avf);
	return amf;
}		// RSYS::rs_AMFForClgCap
//-----------------------------------------------------------------------------
float RSYS::rs_HtgCapForAMF(
	float amf) const	// air mass flow rate, lbm/hr
// returns rated heating capacity, Btuh
{
	float capH = rs_IsPMHtg()
		? 12000.f * AMFtoAVF( amf) / rs_vfPerTon
		: amf * rs_tdDesH * Top.tp_airSH;
	return capH;
}		// RSYS::rs_HtgCapForAMF
//-----------------------------------------------------------------------------
float RSYS::rs_AMFForHtgCap(
	float capH) const	// heating capacity
// returns air mass flow, lbm/hr
{
	float amf = rs_IsPMHtg()
		? AVFtoAMF(capH * rs_vfPerTon / 12000.f)
		: capH / (rs_tdDesH * Top.tp_airSH);
	return amf;
}		// RSYS::rs_AMFForHtgCap
//-----------------------------------------------------------------------------
float RSYS::rs_AMFOperating(
	int rsMode,	// rsmHEAT, rsmCOOL, rsmOAV
	float amfNom,	// nominal or full speed AMF, lbm/hr
	float speedF)	// current speed, rs_speedFMin - 1
// if CHDHW, sets some RSYS mbrs
// returns current operating AMF, lbm/hr
{
	float amf = 0.f;
	int iHC = rsMode != rsmHEAT;
	if (rsMode != rsmOAV && rs_pPMACCESS[iHC])
	{
		double flowFactor = rs_pPMACCESS[iHC]->pa_GetRatedFanFlowFactor(speedF);
		amf = amfNom * flowFactor;
	}
	else if (rsMode == rsmHEAT && rs_IsCHDHW())
	{	// CHDHW heating
		rs_capHt = rs_CurCapHtCHDHW( speedF);
		float avf;
		float fanPwr;	// unused
		rs_pCHDHW->hvt_BlowerAVFandPower(rs_capHt, avf, fanPwr);
		amf = AVFtoAMF(avf);
	}
	else
		amf = amfNom * speedF;

	return amf;

}	// RSYS::rs_AMFOperating
//-----------------------------------------------------------------------------
bool RSYS::rs_CanHaveDucts(
	int iHC) const	// 0=htg, 1=clg
// return nz iff this RSYS may have ducts
{
	bool bRet = !IsSet( iHC ? RSYS_DSEC : RSYS_DSEH);
	return bRet;
}		// RSYS::rs_CanHaveDucts
//-----------------------------------------------------------------------------
bool RSYS::rs_HasDucts(
	int iHC) const // 0=htg, 1=clg
// return true iff this RSYS serves zone(s) via ducts
{
	bool hasDucts =
		   rs_CanHaveDucts(iHC)	// ducts are allowed
		&& (rs_ducts[iHC].dsi[0] + rs_ducts[iHC].dsi[1]) > 0;	// ducts are present

	return hasDucts;

}		// RSYS::rs_HasDucts
//-----------------------------------------------------------------------------
ZNR* RSYS::rs_GetOAVReliefZn() const		// get relief zone for OAV
// returns pointer to ZNR where this RSYS discards zone return air
// NULL if RSYS cannot OAV
{	ZNR* zp = rs_CanOAV()
				? ZrB.GetAtSafe( rs_OAVReliefZi)
				: NULL;
	return zp;
}		// RSYS::rs_GetOAVReliefZn
//-----------------------------------------------------------------------------
void RSYS::rs_OAVSetup()			// 1 time setup for OAV
{
	if (rs_OAVType == C_RSYSOAVTYCH_NONE)
	{	// insurance
		rs_OAVAvfDs = rs_OAVFanSFP = 0.f;
	}
	else
	{
		[[maybe_unused]] ZNR* zp = rs_GetOAVReliefZn();
		if (rs_OAVType == C_RSYSOAVTYCH_VARFLOW)
		{	// variable flow (aka "NightBreeze")

			// rs_OAVAvfDs is required

			if (!IsSet( RSYS_OAVFANSFP))
			{	// W/cfm per DEG Eq 1 (updated 11-20-2013)
				// curve is good only to 1600 cfm
				float avfX = bracket( 1.f, rs_OAVAvfDs, 1600.f);
				rs_OAVFanSFP = 44.616f * pow( 1.00175684f, avfX) / avfX;
			}
		}
		else
		{	// fixed flow (aka "SmartVent")

			// rs_OAVAvfDs is required

			if (!IsSet( RSYS_OAVFANSFP))
				rs_OAVFanSFP = rs_fanSFPC;		// same as cooling
		}
	}
}		// RSYS::rs_OAVSetup
//-----------------------------------------------------------------------------
void RSYS::rs_OAVAirFlow()		// OAV air flow calcs
// call at beg of each day
// sets rs_OAVAvfD and rs_OAVFanPwrD
{
	rs_avfOAV = rs_fanHeatOAV = 0.f;

	if (rs_OAVType == C_RSYSOAVTYCH_VARFLOW)
	{	// NightBreeze algorithm, reference =
		//   "Method for Developing Fan Energy Use For a Variable
		//    Speed Ventilation Cooling Title 24 Measure"
		//    Davis Energy Group
		// 11/20/2013 "Updated" version
		// Air flow and fan power based on prior days average tDb
		//  use double due to potentially huge exp( x)
		float afUnlim = float( 1./pow( 1.+exp( 653.-8.152*Wthr.d.wd_taDbAvg), 0.010654));
		float af = max( rs_OAVAvfMinF, afUnlim);
		rs_avfOAV = af * rs_OAVAvfDs;	// cfm
		rs_fanHeatOAV = rs_OAVAvfDs * rs_OAVFanSFP * pow( af, 2.767f) * 3.413f;
#if defined( _DEBUG)
		// 9-29-2010 model for comparison
		float tMax = Wthr.d.wd_taDbPvMax;
		[[maybe_unused]] float afOld = 0.f;
		if (tMax > .0000001f)
		{	double d = 17.91554 - 3.67538*log( tMax);
			afOld = 1.f/max( 1.f, float( d));
		}
		// float d = af - afOld;
		if (rs_fanHeatOAV > 4000.f)
			printf( "Excess OAV fan heat\n");
#endif
	}
	else if (rs_OAVType == C_RSYSOAVTYCH_FIXEDFLOW)
	{	rs_avfOAV = rs_OAVAvfDs;
		rs_fanHeatOAV = rs_avfOAV * rs_OAVFanSFP * 3.413f;	// fan power, Btuh
	}
	rs_amfOAV = AVFtoAMF( rs_avfOAV);		// mass flow

	// else rs_OAVAvfD = rs_OAVFanPwrD = 0.f from above
}		// RSYS::rs_OAVAirFlow
//-----------------------------------------------------------------------------
int RSYS::rs_OAVAttempt()
// returns
//    -1 not possible

{
	if (rs_SupplyAirState( RSYS::rsmOAV) != 3)
	{	rs_mode = rsmOFF;
		return -1;			// OAV not available
	}

	double tSup = rs_asSup.as_tdb;

	ZNR* zp;
	int okOAV = 0;
	RLUPC( ZrB, zp, rs_IsZoneServed( zp))
	{	okOAV += zp->zn_OAVAttempt( tSup);
		if (okOAV < 0)
			break;
	}
	if (okOAV > 0)
	{	// at least 1 zone wants OAV
		// find zone air temp with available air
		// OAV not OK if any zone above zn_tzspC
		double fSize = rs_amfReq[ 0] > 0.		// fraction of total air that is avaailable
				? min( rs_amf / rs_amfReq[ 0], 1.)
				: 0.;
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
		{	// set zn_rsAmfSup, zn_rsAmfRet, and zn_rsFSize
			zp->zn_SetRSYSAmf( fSize, 0);
			double mCp = zp->zn_rsAmfSup*Top.tp_airSH;
			double tzx = zp->zn_TAirCR( mCp*tSup, mCp, 0.);
			if (tzx > zp->zn_tzspC /* && RSYS can cool */)
			{	// OAV cannot hold tz below cooling set point, disable
				okOAV = 0;
				break;
			}
		}
	}

	if (okOAV <= 0)
	{	// OAV not OK, revert any partial results
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
		{	zp->zn_rsAmfSup = zp->zn_rsAmfRet = 0.;
			zp->zn_rsFSize = 0.;
				// zp->tz at floating temp
			zp->zn_tzsp = 0.f;
			zp->zn_rsAmfSysReq[ 0] = 0.f;
			ZnresB[ zp->ss].curr.S.nShVentH = 0;	// vent heating not possible if no vent
		}
		rs_mode = rsmOFF;
		return -1;
	}

	// OAV can hold acceptable temp in all zones
	// leave rs_mode as rsmOAV
	RLUPC( ZrB, zp, rs_IsZoneServed( zp))
	{	zp->zn_hcMode = rsmOAV;		// say this zone has been successfully OAVed
									// most further this-step HVAC modelling is skipped
		if (ZnresB[ zp->ss].curr.S.nShVentH)
		{	const int WARNMAX = 20;
			int warnCount = ZnresB[ zp->ss].zr_GetAllIntervalTotal( ZNRES_IVL_SUB_NSHVENTH);
			if (warnCount <= WARNMAX)
				warn( "Zone '%s', %s: unhelpful vent heating (supply temp = %0.2f)%s",
					zp->Name(), Top.When( C_IVLCH_S), tSup,
					warnCount == WARNMAX
						? "\n  Suppressing further vent heating messages for this zone" : "");
		}
	}

	return 0;
}		// RSYS::rs_OAVAttempt
//-----------------------------------------------------------------------------
int ZNR::zn_OAVAttempt(
	double tSup)		// available supply air temp (at register), F

// sets zn_tzsp and zn_rsAmfSysReq[ 0]

// returns
//   -9999  zone needs heating or cool or OAV harmful, disable OAV
//       0  no OAV required, zone temp OK
//       1  OAV useful
{
	zn_rsAmfSysReq[ 0] = 0.;	// insurance: no air requested

	if (tz < zn_tzspH /* && RSYS could heat*/)
		return -9999;	// zone needs heat, don't OAV

	RSYS* rs = zn_GetRSYS();
#if 1	// spec change, 10-9-2013
	zn_tzsp = max( zn_tzspD, rs->rs_OAVTdbInlet + rs->rs_OAVTdiff);
#else
x	zn_tzsp = max( zn_tzspD, tSup + rs->rs_OAVTdiff);
#endif
	if (zn_tzsp > zn_tzspC /*&& RSYS could cool*/)
		return -9999;	// no amount of OAV will prevent cooling
	if (tz < zn_tzsp)
		return 0;		// no OAV required

	// dry air mass flow to hold at set point
	double amfOAV;
	if (tSup >= zn_tzsp)
	{	// OAV causes heating, run full volume
#if defined( _DEBUG)
		if (tSup > 100.)
			printf( "Excessive OAV supply temp %0.1f\n", tSup);
#endif
		ZnresB[ ss].curr.S.nShVentH = 1;
		amfOAV = DBL_MAX;
	}
	else
		amfOAV = zn_AmfHvacCR( zn_tzsp, tSup);
	zn_rsAmfSysReq[ 0] = rs->rs_ZoneAirRequest( amfOAV, 0);

	return 1;	// OAV is possible

}		// ZNR::zn_OAVAttempt
//-----------------------------------------------------------------------------
void RSYS::rs_SetRunConstants()
// set model vals that do not vary during simulation
//     *and* do not depend on (possibly) autosized capacities
// redundant calls OK
{
	// operating temp rise due to cooling fan, F
	// constant even if capacity is changed (e.g. during autosize)
	//  (cuz air flow is proportional to capacity via rs_vfPerTon)
	rs_fanDeltaTC = rs_fanSFPC * 3.413f / (60.f * .075f * Top.tp_airSH);

	rs_speedFMin = 1.f;		// altered iff variable speed

	// specific fan power included in rated capacities full speed capacities, W/cfm
	//   ASHP: H1 capacity (cap47)   AC: A capacity (cap95)
	rs_fanSFPRtd[ 0] = rs_FanSpecificFanPowerRated(0);
	rs_fanSFPRtd[ 1] = rs_FanSpecificFanPowerRated(1);

	// duct non-leak fractions
	//  Note: never 0!
	for (int iHC=0; iHC<2; iHC++)
		for (int iSR=0; iSR<2; iSR++)
		{	int iDS = rs_Dsi( iSR, iHC);
			rs_ducts[ iHC].ductLkXF[ iSR] = iDS > 0
					  ? 1.f - min( DsR[ iDS].ds_leakF, .99f)
		              : 1.f;
		}

	rs_OAVSetup();

}		// rs_SetRunConstants

#define FCHG_COP		// defined: apply charge factors to input / COP
						// undefined: apply charge factors to capacity

//-----------------------------------------------------------------------------
void RSYS::rs_SetWorkingPtrs()		// set runtime pointers to meters etc.
// WHY: simplifies runtime code
{
	rs_pMtrElec = MtrB.GetAtSafe( rs_elecMtri);			// elec mtr or NULL
	rs_pMtrFuel = MtrB.GetAtSafe( rs_fuelMtri);			// fuel mtr or NULL
	rs_pMtrHeat = rs_IsElecHeat() ? rs_pMtrElec : rs_pMtrFuel;	// heat mtr or NULL
	rs_pMtrAux = rs_IsFuelAuxH() ? rs_pMtrFuel : rs_pMtrElec;
	rs_pLoadMtr[ 0] = LdMtrR.GetAtSafe(rs_loadMtri);
	rs_pLoadMtr[ 1] = LdMtrR.GetAtSafe(rs_htgLoadMtri);
	rs_pLoadMtr[ 2] = LdMtrR.GetAtSafe(rs_clgLoadMtri);
	rs_pSrcSideLoadMtr[ 0] = LdMtrR.GetAtSafe(rs_srcSideLoadMtri);
	rs_pSrcSideLoadMtr[ 1] = LdMtrR.GetAtSafe(rs_htgSrcSideLoadMtri);
	rs_pSrcSideLoadMtr[ 2] = LdMtrR.GetAtSafe(rs_clgSrcSideLoadMtri);
	rs_pCHDHWSYS = WsR.GetAtSafe(rs_CHDHWSYSi);
}		// RSYS::rs_SetMTRPtrs
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupSizes(		// derive capacity-dependent values
	BOOL bAlways /*=TRUE*/)		// TRUE: always
								// FALSE: iff autosize
// CAUTION: only call is with bAlways=TRUE
//      Review re autosize if used elsewhere 12-2012
{
	RC rc = RCOK;
	if (bAlways || rs_isAuszH)
		rc |= rs_SetupCapH();
	if (bAlways || rs_isAuszC)
		rc |= rs_SetupCapC();
	return rc;
}		// RSYS::rs_SetupSizes
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupCapH(		// set heating members that do not vary during simulation
	float avfH /*=-1*/,	// heating AVF, cfm std air if known
						//   else derived from rs_capH
	int options /*=0*/)	// option bits
						//   1: autosizing fazInit (limited setup)
						//   2: assume not autosize (ignore Top.tp_autoSizing)
// returns RCOK iff success
{
	RC rc = RCOK;

	bool bAutosizeFazInit = options & 1;
	bool bAssumeNotAutosizing = options & 2;

	if (rs_IsPMHtg())
	{	// heating performance map
		rc |= rs_CheckAndSetupPMHtg( 0);
		if (rc)
			return rc;
	}

	if (avfH > 0.f)
	{	rs_amfH = AVFtoAMF( avfH);
		rs_capH = rs_HtgCapForAMF( rs_amfH);
	}
	else
	{	
		float nomCap;
		if (rs_IsASHP())
		{	if (!bAssumeNotAutosizing && Top.tp_autoSizing)
			{	nomCap = rs_capH;	// ASHP autosize derived from rs_capH
			}
			else
			{	rc |= rs_SetupASHP();	// set default capacities/efficiencies, derive constants
										//  multiple calls OK (e.g. during autosizing)
										//  final call needed after autosize complete
				nomCap = rs_cap47;
			}
		}
		else if (rs_IsWSHP())
		{
			rc |= rs_SetupWSHP();		// set default capacities/efficiencies, derive constants
										//  multiple calls OK (e.g. during autosizing)
										//  final call needed after autosize complete 
			nomCap = rs_capH;
		}
		else if (rs_IsCHDHW())
		{
			nomCap = rs_capH;

		}
		else
		{	// non-HP, non-CHDHW
			nomCap = rs_capH;
			if (!rs_CanHaveAuxHeat())
				rs_capAuxH = 0.f;	// insurance
		}
		rs_amfH = rs_AMFForHtgCap(nomCap);	// nominal full speed dry-air mass flow rate, lb/hr
		avfH = AMFtoAVF( rs_amfH);
		rs_capH = nomCap;
	}
	rs_fanHeatH = rs_FanPwrOperating( 0, rs_capH, PMSPEED::RATED);

	if (!rs_CanHaveAuxHeat())
		rs_capAuxH = 0.f;	// insurance

	rs_DefaultCapNomsIf();		// update nominal capacities (no calc effect)

	return rc;

}	// RSYS::rs_SetupCapH
//-----------------------------------------------------------------------------
float RSYS::rs_FanSpecificFanPowerRated(
	int iHC) const	// 0=htg, 1=clg
// applies to both heating and cooling
// returns fan power assumed to be included in net ratings, W/cfm
{
	float sfpRated = 0.f;
	if (rs_IsPM( iHC))
	{	// if performance map type, assume variable capacity
		//   use updated assumptions
		sfpRated = rs_fan.fn_motTy == C_MOTTYCH_PSC ? 0.414f	// permanent split capacitor
			     : rs_HasDucts( iHC) ? 0.281f	// ducted brushless permanent magnet (BPM aka ECM)
			     : 0.171f;						// ductless BPM
	}
	else
	{	// pre-2024 assumptions for non-ASHPPM types
		// reverse-calc from prior hard-coded Btuh/ton values
		float fanHeatPerTon =
			  rs_IsWSHP() ? 212.f		// WSHP
			: rs_IsPkgRoom() ? 0.f		// package room: no fan power adjustment
			: rs_fan.fn_motTy == C_MOTTYCH_PSC ? 500.	// PSC: .365 W/cfm
			:                                   283.f;	// brushless permanent magnet (BPM aka ECM)
		sfpRated = (fanHeatPerTon / BtuperWh) / 400.f;

	}

	return sfpRated;

}		// RSYS::rs_FanSpecificFanPowerRated
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrRatedFullSpeed(		// fan heat at full speed (= rating speed)
	int iHC,			// 0=htg, 1=clg
	float capRef) const	// reference capacity (typically rs_cap47 or rs_cap95)

// returns fan heat included in ratings at rated (reference) speed, Btuh
{
	float fanHeat = rs_adjForFanHt == C_NOYESCH_YES
		? rs_fanSFPRtd[ iHC] * BtuperWh * 400.f * abs(capRef) / 12000.f
		: 0.f;

	return fanHeat;

}		// RSYS::rs_FanPwrRatedFullSpeed
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrRated(
	int iHC,		// 0=htg, 1=clg
	float capRef,	// reference capacity (typically rs_cap47 or rs_cap95)
	PMSPEED whichSpeed) const	// speed selector
							// (PMSPEED::MIN, ::RATED, ::MAX)
{
	float speedF = rs_pPMACCESS[iHC] ? rs_pPMACCESS[iHC]->pa_GetSpeedF(whichSpeed) : 1.f;
	return rs_FanPwrRatedAtSpeedF(iHC, capRef, speedF);
}	// RSYS::rs_FanPwrRated
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrRatedAtSpeedF(
	int iHC,		// 0=htg, 1=clg
	float capFS,	// full speed (=rated) capacity (typically rs_cap47 or rs_cap95)
	float speedF) const
{
	float fanHeatFS = rs_FanPwrRatedFullSpeed(iHC, capFS);
	return rs_FanPwrAtSpeedF(iHC, fanHeatFS, speedF);
}	// RSYS::rs_FanPwrRatedAtSpeedF
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrAtSpeedF(
	int iHC,		// 0=htg, 1=clg
	float fanHeatFS,	// fan heat at full (=rated) speed, Btuh
	float speedF) const
{
	if (rs_pPMACCESS[iHC])
	{
		double flowFactor = rs_pPMACCESS[iHC]->pa_GetRatedFanFlowFactor(speedF);

		double powerFactor = FanVariableSpeedPowerFract(flowFactor, rs_fan.fn_motTy, rs_HasDucts( iHC));

		fanHeatFS *= powerFactor;

	}
	return fanHeatFS;
}		// RSYS::rs_FanPwrAtSpeedF
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrOperating(
	int iHC,		// 0=htg, 1=clg
	float capRef,	// reference capacity (typically rs_cap47 or rs_cap95)
	PMSPEED whichSpeed) const	// speed selector
							// (PMSPEED::MIN, ::RATED, ::MAX)
{
	float speedF = rs_pPMACCESS[iHC] ? rs_pPMACCESS[iHC]->pa_GetSpeedF(whichSpeed) : 1.f;
	return rs_FanPwrOperatingAtSpeedF(iHC, capRef, speedF);
}	// RSYS::rs_FanPwrOperating
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrOperatingAtSpeedF(		// operating fan heat
	int iHC,	// 0=heating, 1=cooling
	float capRef,		// reference (rated) capacity, Btuh
	float speedF) const	// current speed fraction
// returns fan heat, Btuh
{
	float amfRef;	// air mass flow at capRef
	float sfp;		// specific fan power, W/cfm
	if (iHC == 0)
	{
		amfRef = rs_AMFForHtgCap(capRef);
		sfp = rs_fanSFPH;
	}
	else
	{
		amfRef = rs_AMFForClgCap(capRef);
		sfp = rs_fanSFPC;
	}
	float avfRef = AMFtoAVF(amfRef);

	float fanHeatRef = avfRef * sfp * BtuperWh;

	return rs_FanPwrAtSpeedF(iHC, fanHeatRef, speedF);

}		// RSYS::rs_FanPwrOperatingAtSpeedF
//-----------------------------------------------------------------------------
float RSYS::rs_FanPwrOperatingAtSpeedF(		// operating fan heat
	int iHC,	// 0=heating, 1=cooling
	float speedF) const	// current speed fraction
// uses rs_fanHeatH and rs_fanHeatC
// returns fan heat, Btuh
{
	float amfFS;	// air mass flow full (=rated) speed
	float sfp;		// specific fan power, W/cfm
	if (iHC == 0)
	{
		amfFS = rs_amfH;
		sfp = rs_fanSFPH;
	}
	else
	{
		amfFS = rs_amfC;
		sfp = rs_fanSFPC;
	}

	float fanPwrFS = AMFtoAVF( amfFS) * sfp * BtuperWh;

	float fanPwr = rs_FanPwrAtSpeedF(iHC, fanPwrFS, speedF);

	return fanPwr;

}		// RSYS::rs_FanPwrOperatingAtSpeedF
//-----------------------------------------------------------------------------
void RSYS::rs_SetupFanC(		// derive fan cooling fan info
	float avfC /*=-1.f*/)	// cooling AVF, cfm std air if known
							//   else derived from rs_cap95
// sets rs_amfC, rs_fanHeatC, rs_fanRtdC
//   also can set rs_cap95 (if avfC > 0)
{
	if (avfC > 0.f)
	{	rs_amfC = AVFtoAMF( avfC);
		rs_cap95 = rs_ClgCapForAMF( rs_amfC);
		// linkage to PMACCESS
	}
	else
	{	rs_amfC = rs_AMFForClgCap(rs_cap95);
		avfC = AMFtoAVF(rs_amfC);
	}

	rs_fanHeatC  = rs_FanPwrOperating( 1, rs_cap95, PMSPEED::RATED);	// rated or full speed operating fan power, Btuh

	rs_fanHRtdC = rs_FanPwrRated( 1, rs_cap95, PMSPEED::RATED);	// fan heat included in rated capacity, Btuh
	// rs_fanHRtdH is derived independently

}		// RSYS::rs_SetupFanC
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupCapC(		// derive constants that depend on capacity
	float avfC /*=-1.f*/,	// air flow if known, passed to rs_SetupFanC
	int options /*=0*/)		// option bits
							//  1: autosize FazInit (do minimal setup)
// sets RSYS cooling members that do not vary during simulation
{
	RC rc = RCOK;
	bool bAutosizeFazInit = options & 1;

	if (rs_IsPMClg())
	{	// cooling performance map
		//   currently (4-24) used for rs_IsVC() only
		rc |= rs_CheckAndSetupPMClg( bAutosizeFazInit ? 0 : pmSETRATINGS);
	}

	if (!rc && !bAutosizeFazInit)
	{
		rs_SetupFanC(avfC);	// sets rs_cap95 if avfC > 0., else sets rs_amfC
							// derives rs_fanHRtdC

		// rs_cap95 now known = net total cooling cap at 95 F (> 0)

		// notes re capacity manipulations
		// load = capSensT = capnf*shr - qFanOp
		// capnf = (cap95 + qfanRat)*fChg*fSize*fCondCap
		// let F = fChg*fCondCap*shr
		// load = (cap95 + qFanRat)*F - qFanOp
		// load = (cap95*(1 + fanRatX))*F - cap95*fanOpX
		// load = cap95*(F*(1+fanRatX)-fanOpX
		// cap95 = (load + fanOpX)/(F*(1 + fanRatX))

		// rs_capnfX used by several models
		float cap95nf = -(fabs(rs_cap95) + rs_fanHRtdC);	// coil (gross) total capacity at 95 F, Btuh (< 0)

#if defined( FCHG_COP)
		rs_capnfX = min(cap95nf, -10.f);	// prevent 0 capacity

		if (rs_Is1Spd())
		{
			// ratings for 82 F and 115 F (reporting only)
			float SHR = CoolingSHR(82.f, 80.f, 67.f, 400.f);
			rs_cap82 = rs_cap95 * CoolingCapF1Spd(SHR, 82.f, 80.f, 67.f, 400.f);
			rs_COP82 = rs_fChgC * rs_SEER / 3.413f;
			
			SHR = CoolingSHR(115.f, 80.f, 67.f, 400.f);
			rs_cap115 = rs_cap95 * CoolingCapF1Spd(SHR, 115.f, 80.f, 67.f, 400.f);
			float inpFSEER;
			float inpFEER = CoolingInpF1Spd(SHR, 115.f, 80.f, 67.f, 400.f, inpFSEER);
			float inp115 = inpFEER * rs_cap95 / (rs_EER95/3.413f);
			rs_COP115 = rs_fChgC * rs_cap115 / inp115;

			// base value for SEERnf and EERnf calculations
			//   used only for single speed
			float inpX = 1.09f * rs_cap95 / rs_SEER - rs_fanHRtdC / 3.413f;	// input power, W
			rs_SEERnfX = rs_fChgC *
				(inpX > 0.f ? (1.09f * rs_cap95 + rs_fanHRtdC) / inpX : rs_SEER);

			inpX = rs_cap95 / rs_EER95 - rs_fanHRtdC / 3.413f;		// 95F rated compressor input power, W
			rs_EERnfX = rs_fChgC * (inpX > 0.f ? -rs_capnfX / inpX : rs_EER95);	// gross total EER
		}

#else
		rs_capnfX = min(cap95nf * rs_fChgC, -10.f);			// apply charge adjustment

		if (rs_Is1Spd())
		{
			// ratings for 82 F and 115 F (reporting only)
			float SHR = CoolingSHR(82.f, 80.f, 67.f, 400.f);
			rs_cap82 = rs_cap95 * CoolingCapF1Spd(SHR, 82.f, 80.f, 67.f, 400.f);
			rs_COP82 = rs_SEER / 3.413f;
			
			SHR = CoolingSHR(115.f, 80.f, 67.f, 400.f);
			rs_cap115 = rs_cap95 * CoolingCapF1Spd(SHR, 115.f, 80.f, 67.f, 400.f);
			float inpFSEER;
			float inpFEER = CoolingInpF1Spd(SHR, 115.f, 80.f, 67.f, 400.f, inpFSEER);
			float inp115 = inpFEER * rs_cap95 / (rs_EER95/3.413f);
			rs_COP115 = rs_cap115 / inp115;

			// base value for SEERnf and EERnf calculations
			//   used only for single speed
			float inpX = 1.09f * rs_cap95 / rs_SEER - rs_fanHRtdC / 3.413f;	// input power, W
			rs_SEERnfX = inpX > 0.f ? rs_fChgC * (1.09f * rs_cap95 + rs_fanHRtdC) / inpX
				: rs_SEER;

			inpX = rs_cap95 / rs_EER95 - rs_fanHRtdC / 3.413f;		// 95F rated compressor input power, W
			rs_EERnfX = inpX > 0.f ? -rs_capnfX / inpX : rs_EER95;	// gross total EER
		}
#endif

		rs_DefaultCapNomsIf();		// update nominal capacities (no calc effect)
	}

	return rc;

}		// RSYS::rs_SetupCapC
//----------------------------------------------------------------------------
#if 0
RC RSYS::rs_SetDuctSizes()
{
	for (int iSR=0; iSR<2; iSR++)
	{	int iDS = rs_Dsi( iSR);
		if (iDS > 0)
		{	DUCTSEG* pDS = DsR[ iDS];
		}
	}
}		
#endif
//----------------------------------------------------------------------------
int RSYS::rs_IsAutoSizing() const
// returns rsmOFF, rsmHEAT, or rsmCOOL
{
	return (Top.tp_dsDay == 1 && rs_isAuszH) ? rsmHEAT
		 : (Top.tp_dsDay == 2 && rs_isAuszC) ? rsmCOOL
		 :									   rsmOFF; // = 0
}		// RSYS::rs_IsAutoSizing
//----------------------------------------------------------------------------
RC RSYS::rs_BegHour()
{	RC rc = RCOK;

	if (Top.isBegDay)
	{	// init capacity ratios for day
		//   track smallest fxCap for heating and cooling
		//     = largest load relative to capacity
		//   thus init to large value
		rs_fxCapHDay = rs_fxCapCDay = 20.f;

		// testing/debugging aids
		//   insurance: do at sim beg so autosizing complete
		if (Top.tp_isBegMainSim)
		{
#if 0 && defined( _DEBUG)
			if (rs_IsASHP())
			{	// back-derive HSPF (self-check)
				rs_HSPFCheckASHP( 0x100 + 0);
				rs_PerfDataASHP();  // write CSV file of performance points
			}
#endif
			if (rs_generatePerfMap == C_NOYESCH_YES)
				rs_GeneratePerfMapAC();		// make performance map
		}

		// OAV daily setup
		rs_OAVAirFlow();
	}

	return rc;
}	// RSYS::rs_BegHour
//----------------------------------------------------------------------------
RC RSYS::rs_BegSubhr()
{
	RC rc = RCOK;

	rs_tSupLs = rs_asSup.as_tdb;
	rs_mode = rsmOFF;					// mode not yet known
	rs_ClearSubhrResults();
	RsResR[ss].curr.S.rsr_Zero();		// clear associated RSYSRES results

	rs_fxCap[ 0] = rs_fxCap[ 1] = 0.f;	// excess capacity not known

	if (!IsSet( RSYS_TDBOUT))
		rs_tdbOut = Top.tDbOSh;		// default outdoor temp
	// else set via input expression

	if (!IsSet( RSYS_OAVTDBINLET))
		rs_OAVTdbInlet = Top.tDbOSh;
	// else set via input expression

	return rc;
}		// RSYS::rs_BegSubhr
//----------------------------------------------------------------------------
RC RSYS::rs_EndSubhr()
{
	RC rc = RCOK;

	// track excess capacity for day
	//   used in re autosizing
	if (rs_fxCap[ 0] > 0.f)
	{	if (rs_mode == rsmHEAT)
		{	if (rs_fxCap[ 0] < rs_fxCapHDay)
				rs_fxCapHDay = rs_fxCap[ 0];
		}
		else if (rs_mode == rsmCOOL)
		{	if (rs_fxCap[ 0] < rs_fxCapCDay)
				rs_fxCapCDay = rs_fxCap[ 0];
		}
	}

	// populate results
	// Note: MTRs and LOADMTs are cleared in ::doBegIvl()
	RSYSRES_IVL_SUB& R = RsResR[ ss].curr.S;

	R.fhTot = R.fhParasitic = rs_parFuel * Top.tp_subhrDur;	// assign fuel parasitics to heating
	if (rs_parElec > 0.f)
	{	// assign electrical parasitics per current mode or last active mode
		float ePar = rs_parElec * BtuperWh * Top.tp_subhrDur;
		int modeX = rs_mode != rsmOFF ? rs_mode : rs_modeLastActive;
		if (modeX <= rsmHEAT)
			R.ehTot = R.ehParasitic = ePar;		// rsmOFF and rsmHEAT -> heating
		else if (modeX == rsmCOOL)
			R.ecTot = R.ecParasitic = ePar;
		else
			R.evTot = R.evParasitic = ePar;
	}

	R.hrsOn = rs_runF * Top.tp_subhrDur;
	float qFan = rs_outFan * Top.tp_subhrDur;
	float eFan = rs_inFan * Top.tp_subhrDur;

	if (rs_mode == rsmHEAT)
	{
		R.hrsOnAux = rs_runFAux * Top.tp_subhrDur;
		R.qhPrimary = rs_outSen * Top.tp_subhrDur;
		R.qhDefrost = rs_outDefrost * Top.tp_subhrDur;
		R.qhAux = rs_outAux * Top.tp_subhrDur;
		R.qhFan = qFan;
		R.qhNet = R.qhPrimary + R.qhDefrost + R.qhAux + R.qhFan;

		// heating input electricity and fuel (rs_xxx values are Btuh)
		(rs_IsElecHeat() ? R.ehPrimary : R.fhPrimary) = rs_inPrimary * Top.tp_subhrDur;
		if (rs_IsFuelAuxH())
		{	R.fhDefrost = rs_inDefrost * Top.tp_subhrDur;
			R.fhAux = rs_inAux * Top.tp_subhrDur;
		}
		else
		{	R.ehDefrost = rs_inDefrost * Top.tp_subhrDur;
			R.ehAux = rs_inAux * Top.tp_subhrDur;
		}
		R.ehFan = eFan;
		R.ehTot += R.ehPrimary + R.ehDefrost + R.ehAux + R.ehFan;
					/* + R.ehParasitic see above */
		R.fhTot += R.fhPrimary + R.fhDefrost + R.fhAux /* + R.fhParasitic*/;

		for (int iM = 0; iM < 2; iM++)
		{	// accumulate values to LOADMETERs
			//   [0] accums both heating and cooling
			//   [1] accums heating
			if (rs_pLoadMtr[ iM])	// primary = coil loads
				rs_pLoadMtr[ iM]->S.qHtg += R.qhPrimary;
			if (rs_pSrcSideLoadMtr[ iM])	// source-side (= "outdoor coil" or environment source)
				// + = from env: >0 during heating = coil heating - compressor electricity
				rs_pSrcSideLoadMtr[ iM]->S.qHtg += R.qhPrimary - R.ehPrimary;
		}
	}
	else if (rs_mode == rsmCOOL)
	{
		R.qcSen = rs_outSen * Top.tp_subhrDur;
		R.qcLat = rs_outLat * Top.tp_subhrDur;
		R.qcFan = qFan;
		R.qcSenNet = R.qcSen + R.qcFan;

		R.ecPrimary = rs_inPrimary * Top.tp_subhrDur;
		R.ecFan = eFan;

		R.ecTot += R.ecPrimary + R.ecFan /* + R.ecParasitic, see above */;

		for (int iM = 0; iM < 3; iM += 2)
		{	// accumulate values to LOADMETERs
			//   [0] accums both heating and cooling
			//   [2] accums cooling
			if (rs_pLoadMtr[iM])	// primary = coil loads
				rs_pLoadMtr[iM]->S.qClg += R.qcSen + R.qcLat;
			if (rs_pSrcSideLoadMtr[iM])	// source-side (= "outdoor coil" or environment source)
				// + = from env: <0 when cooling = total cooling + compressor electricity
				rs_pSrcSideLoadMtr[iM]->S.qClg += R.qcSen + R.qcLat - R.ecPrimary;
		}
	}
	else if (rs_mode == rsmOAV)
	{	// sensible "output" = fan power
		R.evFan = eFan;
		R.evTot += R.evFan; /* + R.evParasitic, see above */
	}

	// verify RSYSRES_IVL_SUB layout at compile time
	// fixed sequence allows array access by rs_mode (see code below)
	// rsmHEAT/rsmCOOL/rsmOAV definitions must be consistent with member sequences.
#if 0 // TODO: Fix for non-MSVC compilers
#define QZONECHK( m, oDif) static_assert( &(((RSYSRES_IVL_SUB *)0)->m)-&(((RSYSRES_IVL_SUB *)0)->qhZoneSen) == oDif, "Bad seq " #m)
	QZONECHK(qhZoneSen, (rsmHEAT - 1) * 2);
	QZONECHK(qhZoneLat, (rsmHEAT - 1) * 2 + 1);
	QZONECHK(qcZoneSen, (rsmCOOL - 1) * 2);
	QZONECHK(qcZoneLat, (rsmCOOL - 1) * 2 + 1);
	QZONECHK(qvZoneSen, (rsmOAV - 1) * 2);
	QZONECHK(qvZoneLat, (rsmOAV - 1) * 2 + 1);
	static_assert(rsmCOUNT == 4, "Bad rsm enum");
#undef QZONECHK
#endif
	if (rs_mode != rsmOFF)
	{ 	// heat transfers from this RSYS to all zones (including duct losses)
		const ZNR* zp;
		int iMX = 2 * (rs_mode - 1);
		RLUPC(ZrB, zp, rs_IsZoneServed(zp))
		{
			(&R.qhZoneSen)[iMX] += float(zp->zn_qsHvac * Top.tp_subhrDur);
			(&R.qhZoneLat)[iMX] += float(zp->zn_qlHvac * Top.tp_subhrDur);
		}
	}

	return rc;
}		// RSYS::rs_EndSubhr
//-----------------------------------------------------------------------------
RC RSYS::rs_AfterSubhr()
// end-subhour after-exprs/reports stuff for loads
// set 'prior interval' variables etc: if done sooner, probes come out wrong.
{
	rs_modeLs = rs_mode;
	if (rs_mode != rsmOFF)
		rs_modeLastActive = rs_mode;
	return RCOK;
}	// RSYS::rs_AfterSubhr
//-----------------------------------------------------------------------------
RC RSYS::rs_AfterHour()
// end-hour after-exprs/reports stuff for loads
// set 'prior interval' variables etc: if done sooner, probes come out wrong.

{
	RC rc = RCOK;

	if (Top.isEndDay)
	{	// last step of day
		int auszMode = rs_IsAutoSizing();
		if (auszMode != rsmOFF)
			rc = rs_endP1DsdIter( auszMode);
	}

	// prior interval values: none

	return rc;
}		// RSYS::rs_AfterHour
//-----------------------------------------------------------------------------
void RSYS::rs_HeatingOutletAirState(
	int auszMode /*=rsmOff*/)	// active autosizing, if any
								//   rsmOFF: none (normal simulation)
								//   rsmHEAT: heating
								//   rsmCOOL: do not call

// derives RSYS outlet conditions (before distribution losses) for heating mode

// call:   rs_asIn = entering state (return duct leaving state)
//		   rs_asOut = rs_asIn (initialized for calc)
//		   rs_twb = entering air wet bulb, F (consistent with rs_asIn)
//         rs_tdbOut = dry-bulb temp at outdoor unit, F
//		   rs_speedF = current speed fraction
//         rs_amf = current dry air mass flow rate, lbm/hr (at rs_speedF)
// return: rs_asOut = outlet (leaving) air state (including fan heat)
//         (plus intermediate correlation results)

{
	// preconditions
	assert(auszMode == rsmOFF || auszMode == rsmHEAT);


	// determine current heating capacity and efficiency

	rs_capHt = -1.f;	// heat to be added to airstream (=net sensible capacity)
						//  <0 = not yet known

	if (auszMode == rsmHEAT)
	{	if (Top.tp_pass1A)
		{	// autosize warmup: assume fixed temp rise at room (duct losses and fan details ignored)
			rs_capHt = rs_asOut.as_CalcQSen2(rs_asRet.as_tdb + rs_tdDesH, rs_amf);
			rs_effHt = 1.f;		// need nz value, else ASHP assumes compressor off
		}
		else if (rs_IsASHP())
		{	// ASHP heat autosize (based on rs_capH)
				rs_effHt = 1.f;
				rs_capHt = rs_capH;
				rs_capAuxH = rs_capH;		// same cap for aux during autosizing
				//   used below if needed

		}
		// other autosize: use full model (why?)
	}
	
	if (rs_capHt < 0.f)
	{
		// fan heat: coil entering conditions
		// rs_asOut = rs_asIn per caller
		// see analoguous cooling code in rs_CoolingOutletAirState
		//   rs_twbCoilIn not needed for heating
		rs_tdbCoilIn = rs_asOut.as_tdb;
		if (rs_fan.fanTy == C_FANTYCH_BLOWTHRU)
			rs_tdbCoilIn += rs_asOut.as_DeltaTQSen(rs_fanPwr, rs_amf);

		if (rs_IsASHP())
		{
			float capHtGross, inpHtGross;
			rs_CapEffASHP2( capHtGross, inpHtGross, rs_capDfHt);	// sets rs_capHt and rs_EffHt
		
			// use correlations to get adjustments for entering air state
			rs_fCondCap = rs_fCondInp = 1.f;
			if (rs_IsPMHtg())
				rs_HeatingEnteringAirFactorsVC(rs_fCondCap, rs_fCondInp);

			rs_capHt = capHtGross * rs_fCondCap + rs_fanPwr;
			rs_inpHt = inpHtGross * rs_fCondInp + rs_fanPwr;
	
		}
		else if (rs_IsWSHP())
		{
			const float airMassFlowF = 1.f;  // temporary assumption
			/*rc |=*/ WSHPPerf.whp_HeatingFactors(rs_fCondCap, rs_fCondInp, rs_tdbOut, rs_tdbCoilIn, airMassFlowF);
			float capHtGross = (rs_capH - rs_fanHRtdH) * rs_fCondCap;
			rs_capHt = capHtGross + rs_fanPwr;		// net capacity
			float inpX = (capHtGross / rs_COP47) * rs_fCondInp;  // gross input power
			rs_effHt = capHtGross / inpX * rs_fEffH;	// adjusted gross efficiency
		}
		else if (rs_IsCHDHW())
		{
			rs_capHt = rs_CurCapHtCHDHW(rs_speedF);
			rs_effHt = 1.f;
		}
		else
		{	// not heat pump of any type
			rs_effHt = rs_IsFanCoil() ? 1.f : rs_AFUE * rs_fEffH;
			rs_capHt = rs_capH;		// includes fan heat
		}
	}

	// add heat to air stream
	rs_asOut.as_AddQSen2( rs_capHt, rs_amf);

	// auxiliary heat supply air state
	//  rs_capAuxH is known
	if (rs_CanHaveAuxHeat() && rs_speedF == 1.f)
	{
		// double tdbWas = rs_asOutAux.as_tdb;
		float fanHeatAux;
		if (rs_ctrlAuxH == C_AUXHEATCTRL_CYCLE)
		{	// aux cycles: aux out = prim out + aux heat
			//     fan heat already in prim out
			rs_asOutAux = rs_asOut;
			fanHeatAux = 0.f;
		}
		else
		{	// aux alternate or locked out: prim off
			//    fan heat must be added
			rs_asOutAux = rs_asIn;
			fanHeatAux = rs_fanHeatH;
		}

		rs_asOutAux.as_AddQSen2(rs_capAuxH + fanHeatAux, rs_amf);
		rs_asSupAux = rs_asOutAux;
		rs_SupplyDSEAndDucts(rs_asSupAux);
	}

#if defined( RSYS_FIXEDOAT)
	rs_asSup.as_Set(120., .001);
#endif
	return;
}		// RSYS::rs_HeatingOutletAirState
//-----------------------------------------------------------------------------
void RSYS::rs_CoolingSHR()		// derive cooling sensible heat ratio
// call:   rs_tdbCoilIn = coil entering dry bulb, F
//         rs_twbCoilIn = coil entering wet bulb, F
//		   rs_tdbOut = outdoor drybulb seen by condenser, F
//         rs_vfPerTon = indoor std air cfm / ton
// return: rs_SHR = SHR under current conditions
{
#if 0 && defined( _DEBUG)
x	if (!Top.isWarmup)
x		printf("Hit\n");
#endif
#if 0 && defined( _DEBUG)
	if (!Top.isWarmup)
	{
		if (fabs(rs_asIn.as_tdb - 80.) < .2
		 && fabs(rs_twbCoilIn - 67.) < 1.
		 && fabs(rs_tdbOut - 95.f) < .2)
			printf("Hit\n");
	}
#endif
#if 0 && defined( _DEBUG)
	rs_tdbOut = 95.f;
	rs_tdbCoilIn = 80.f;
	rs_twbCoilIn = 67.f;
	rs_vfPerTon = 400.f;
#endif

	rs_SHR = CoolingSHR(rs_tdbOut, rs_tdbCoilIn, rs_twbCoilIn, rs_vfPerTon);

}		// RSYS::rs_CoolingSHR
//-----------------------------------------------------------------------------
float RSYS::rs_CoolingCapF1Spd(		// capacity factor for 1 spd model
	float tdbOut /*=0.f*/)
// return: fCondCap = current conditions factor for total gross capacity
{
	if (tdbOut == 0.f)
		tdbOut = rs_tdbOut;

	rs_fCondCap = CoolingCapF1Spd(rs_SHR, tdbOut, rs_tdbCoilIn, rs_twbCoilIn, rs_vfPerTon);
	return rs_fCondCap;

}		// RSYS::rs_CoolingCapF1Spd
//-----------------------------------------------------------------------------
float RSYS::rs_CoolingEff1Spd(		// cooling efficiency, 1 spd model
	float tdbOut /*=0.f*/)

// sets rs_fCondSEER, rs_SEERnf, rs_fCondEER, rs_EERnf, rs_EERt

// returns input power modifier
{
	if (tdbOut == 0.f)
		tdbOut = rs_tdbOut;

	float fInpSEER;
	float fInpEER = CoolingInpF1Spd(rs_SHR, tdbOut, rs_tdbCoilIn, rs_twbCoilIn, rs_vfPerTon, fInpSEER);

	rs_fCondSEER = rs_fCondCap / fInpSEER;
	rs_SEERnf = rs_SEERnfX * rs_fCondSEER;

	rs_fCondEER = rs_fCondCap / fInpEER;
	rs_EERnf = rs_EERnfX * rs_fCondEER;

	rs_EERt = tdbOut < 82.f ? rs_SEERnf
		: tdbOut < 95.f ? rs_SEERnf + (tdbOut - 82.f) * (rs_EERnf - rs_SEERnf) / 13.f
		: rs_EERnf;

	return fInpEER;

}		// RSYS::rs_CoolingEff1Spd
//-----------------------------------------------------------------------------
void RSYS::rs_CoolingEnteringAirFactorsVC(		// adjustments for entering (indoor) air state
	float& capF,			// returned: capacity factor
	float& inpF) const		// returned: compressor input power factor
{
	float capFN, eirFN;
	// CoolingAdjust(95.f, 67.f, 400.f, capFN, eirFN);
	CoolingAdjust(rs_tdbOut, 67.f, 400.f, capFN, eirFN);
	float capFD, eirFD;
	CoolingAdjust(rs_tdbOut, rs_twbCoilIn, rs_vfPerTon, capFD, eirFD);

	capF = capFD / capFN;

	inpF = capF * eirFD / eirFN;

}		// RSYS::rs_CoolingEnteringAirFactorsVC
//------------------------------------------------------------------------------
void RSYS::rs_HeatingEnteringAirFactorsVC(		// adjustments for entering (indoor) air state
	float& capF,			// returned: capacity factor
	float& inpF) const		// returned: compressor input power factor
{
	float capFN, eirFN;
	// HeatingAdjust( 47.f, 70.f, 400.f, capFN, eirFN): should yield capFN=eirFN=1
	HeatingAdjust( rs_tdbOut, 70.f, 400.f, capFN, eirFN);

	float capFD, eirFD;
	HeatingAdjust(rs_tdbOut, rs_tdbCoilIn, rs_vfPerTon, capFD, eirFD);

	capF = capFD / capFN;

	inpF = capF * eirFD / eirFN;

}		// RSYS::rs_HeatingEnteringAirFactorsVC
//-----------------------------------------------------------------------------
void RSYS::rs_CoolingOutletAirState(		// cooling outlet (leaving) air state
	int auszMode /*=rsmOff*/)	// active autosizing, if any
								//   rsmOFF: none (normal simulation)
								//   rsmCOOL: cooling
								//   rsmHEAT: do not call

// derives RSYS outlet conditions (before supply distribution losses) for cooling mode

// call:   rs_asIn = entering state (return duct leaving state)
//		   rs_asOut = rs_asIn (initialized for calc)
//		   rs_twb = entering air wet bulb, F (consistent with rs_asIn)
//         rs_tdbOut = dry-bulb temp at outdoor unit, F
//		   rs_speedF = current speed fraction
//         rs_amf = current dry air mass flow rate, lbm/hr (at rs_speedF)
// return: rs_asOut = leaving state (including fan heat)
//         rs_tdbCoilIn = coil entering dry bulb, F
//         rs_twbCoilIn = coil entering wet bulb, F
//         (plus intermediate correlation results)
{

#if 0 && defined( _DEBUG)
	static bool bDoneExport = false;
	if (!bDoneExport)
	{
		rs_ExportCorrelationValues();
			bDoneExport = true;
	}
#endif

#if 0 && defined( _DEBUG)
x	if (Top.jDay == 242 && Top.iHr == 14)
x		printf("\nhit");
#endif

	// fan heat: coil entering conditions
	// rs_asOut = rs_asIn per caller
	if (rs_fan.fanTy == C_FANTYCH_BLOWTHRU)
	{	rs_asOut.as_AddQSen(rs_fanPwr, rs_amf);
		rs_twbCoilIn = rs_asOut.as_Twb();
	}
	else
		rs_twbCoilIn = rs_twbIn;		// use known value
	rs_tdbCoilIn = rs_asOut.as_tdb;

	if (rs_IsFanCoil())
	{
		rs_SHR = rs_SHRtarget;		// user input (may be expression)
		rs_capnfX = rs_capTotCt = -rs_cap95;
		rs_fCondCap = 1.f;
		rs_capSenCt = rs_capTotCt * rs_SHR;
		// rs_inpCt = rs_effCt = 0.f;
	}
	else
	{	// compression cooling

		rs_CoolingSHR();

		// no fan (coil-only) capacities at current conditions (fan heat elsewhere)
		//  total, sensible
		if (auszMode == rsmCOOL && Top.tp_pass1A)
		{	// autosizing warmup: assume coil can produce design delta-T
			//  (ignore sign of rs_tdDesC)
			//  why: produces (more) stable supply air state
			rs_CoolingCapF1Spd();		// use 1 spd model for both 1 spd and VC
			float coilDT =		// temp diff across coil (< 0)
				-fabs(rs_tdDesC) * rs_fCondCap - rs_fanDeltaTC;
			rs_capSenCt = coilDT * rs_amf * Top.tp_airSH;
			rs_capTotCt = rs_capSenCt / rs_SHR;		// total, Btuh (note rs_SHR>0 always)
			// rs_inpCt = rs_SHR = 0.f
		}
		else if (rs_IsWSHP())
		{
			const float airMassFlowF = 1.f;  // temporary assumption
			float capF, capSenF, inpF;
			/* rc |= */ WSHPPerf.whp_CoolingFactors(capF, capSenF, inpF,
				rs_tdbOut, rs_tdbCoilIn, rs_twbCoilIn, airMassFlowF);
			rs_capTotCt = rs_capnfX * capF;		// total gross capacity at current conditions, Btuh
			rs_capSenCt = rs_SHRtarget * rs_capTotCt * capSenF;  // SHR set using air-to-air approach
			if (rs_capSenCt > rs_capTotCt)
			{
				rs_capSenCt = rs_capTotCt;
			}
			rs_SHR = rs_capSenCt / rs_capTotCt;
			rs_inpCt = ((rs_cap95 / (rs_EER95 / BtuperWh)) - rs_fanHRtdC) * inpF;
		}
		else if (rs_IsPMClg())
		{
			// performance map
			// total capacities and input, Btuh
			//   include rated fan power
			//   cap values are net Btuh, <0 (w/o rs_fChg adjust)
			float capClg, inpClg;
			/* RC rc = */ rs_pPMACCESS[1]->pa_GetCapInp( rs_tdbOut, rs_speedF,
				capClg, inpClg);
			rs_SetSpeedFMin();

			ASSERT(capClg <= 0.f);

			float fanHRtdCAtSpeed = rs_FanPwrRatedAtSpeedF(1, rs_cap95, rs_speedF);

			// use correlations to get adjustments for entering air state
			rs_CoolingEnteringAirFactorsVC(rs_fCondCap, rs_fCondInp);

#if defined( FCHG_COP)
			rs_capTotCt =		// gross total coil capacity at current conditions and speed, Btuh
				(capClg - fanHRtdCAtSpeed) * rs_fCondCap;
			rs_inpCt = (inpClg - fanHRtdCAtSpeed) * rs_fCondInp / rs_fChgC;	// full speed input power at current conditions w/o fan

#else
			rs_capTotCt	=	// gross total coil capacity at current conditions and speed, Btuh
				(capClg - fanHRtdCAtSpeed) * rs_fChgC * rs_fCondCap;
			rs_inpCt = (inpClg - fanHRtdCAtSpeed) * rs_fCondInp;	// full speed input power at current conditions w/o fan

#endif
			rs_capSenCt = rs_SHR * rs_capTotCt;		// sensible coil capacity at current conditions and speed, Btuh
		}
		else
		{	// 1 spd, not autosizing warmup
			rs_CoolingCapF1Spd();	// sets rs_fCondCap
			rs_capTotCt = rs_capnfX * rs_fCondCap;		// total gross capacity at current conditions, Btuh
			rs_capSenCt = rs_SHR * rs_capTotCt;			// sensible gross capacity at current conditions, Btuh

			rs_CoolingEff1Spd();	// derive rs_EERt (includes rs_fChgC)

			rs_inpCt = abs(rs_capTotCt) * BtuperWh / rs_EERt;	// compressor input power
		}
	}

	rs_capLatCt = rs_capTotCt - rs_capSenCt;	// latent, Btuh

	// outlet (leaving) conditions
#if defined( _DEBUG)
	float hIn = rs_asIn.as_Enthalpy();
#endif

#if 0
x	// test code
x	AIRSTATE asSav( rs_asOut);
x	float qSen = 2.f * rs_capSenCt;
x	float qLat = rs_capLatCt;
x	int rx = rs_asOut.as_AddQSenLat( qSen, qLat, rs_amf, 0.95f);
x	rs_asOut = asSav;
#endif
	int ret = rs_asOut.as_AddQSenLat( rs_capSenCt, rs_capLatCt, rs_amf, 0.95f);
	if (ret == 1)
	{	// as_AddQSenLat() applied humidity limit
		//   update capacities
		rs_capTotCt = rs_capSenCt + rs_capLatCt;
		rs_fCondCap = rs_capTotCt / rs_capnfX;		// back-calc consistent value
		rs_SHR = rs_capTotCt != 0.f ? rs_capSenCt / rs_capTotCt : 0.f;
	}

	if (rs_inpCt > 0.f)
		rs_effCt = abs( rs_capTotCt) * rs_fEffC / rs_inpCt;

	// draw-thru fan heat
	if (rs_fan.fanTy != C_FANTYCH_BLOWTHRU)
		rs_asOut.as_AddQSen( rs_fanPwr, rs_amf);

#if defined( RSYS_FIXEDOAT)
	rs_asOut.as_Set(50., .001);
#endif

	// speedF?
	rs_capSenNetFS = rs_capSenCt / (rs_speedF > 0.f ? rs_speedF : 1.f) + rs_fanPwr;		// net full speed sensible capacity

#if defined( _DEBUG)
	if (!Top.isWarmup)
	{	double tSup = rs_asOut.as_tdb;
		if (tSup < 20. || tSup > 200.)
			orWarn("unreasonable cooling supply temp %.2f F", tSup);
#if 0
		if (rs_asIn.as_tdb - tSup < 10.)
			orWarn("low cooling deltaT -- tRet=%.2f F, tSup=%.2f, SHR=%.3f",
				rs_asIn.as_tdb, tSup, rs_SHR);
#endif
		float hOut = rs_asOut.as_Enthalpy();
		float hDelta = rs_amf * (hOut - hIn);
		float capTot = rs_capSenCt + rs_capLatCt + rs_fanPwr;
		float fDiff = frDiff( hDelta, capTot);
		if (fDiff > .0002f)
			orWarn( "coil enthalpy mismatch (%.5f)", fDiff);	
	}
#endif

}		// RSYS::rs_CoolingOutletAirState
//----------------------------------------------------------------------------
void RSYS::rs_GetPerfClg(	// derive current cooling performance values
	float& capTot,			// total net capacity, kBtuh
	float& capSen,			// sensible net capacity, kBtuh
	float& pwrIn) const		// input power, kW
// Used only for reporting (not simulation) 10-2018
// All returned values include fan heat / power
{
	pwrIn = (fabs( rs_capTotCt / rs_effCt) + rs_fanHeatC)
									/ (BtuperWh * 1000.f);
	capTot = (fabs( rs_capTotCt)-rs_fanHeatC)/1000.f;
	capSen = (fabs( rs_capSenCt)-rs_fanHeatC)/1000.f;

#if 0
x	// check: derived total capacity from air stream enthalpy change
x	float capTotX = rs_amf * (rs_asOut.as_Enthalpy() - rs_asIn.as_Enthalpy());
#endif

}		// RSYS::rs_GetPerfClg
//-------------------------------------------------------------------------------
RC RSYS::rs_GeneratePerfMapAC()		// generate AC cooling performance map
// testing aid
{
#if 1
// Goodman
static float fAF[] = { .550f, .700f, .875f, 1.f, 1.125f, 0.f };
static float tdbI[] = { 70.f, 75.f, 80.f, 85.f, 0.f };
static float twbI[] = { 59.f, 63.f, 67.f, 71.f };
static float tdbO[] = { 65.f, 75.f, 85.f, 95.f, 105.f, 115.f, 0.f };
#else
// Carrier
static float fAF[] = { .875f, 1.f, 1.125f, 0.f };
static float tdbI[] = { 70.f, 75.f, 80.f, 0.f };
static float twbI[] = { 57.f, 62.f, 67.f, 72.f };
static float tdbO[] = { 75.f, 85.f, 95.f, 105.f, 115.f, 125.f, 0.f };
#endif

	// write to file PM_<name>.csv
	const char* nameX = name;
	if (IsBlank( nameX))
		nameX = "RSYS";
	FILE* f = fopen( strtprintf( "PM_%s.csv", nameX), "wt");
	if (!f)
		return RCBAD;		// can't open file

	RSYS rsSave( *this);		// save for restore at exit
								//   (we alter mbrs here)

	fprintf(f, "%s\n", rs_desc.CStrIfNotBlank(nameX));

	fprintf( f, "cfm,IDB (F),IWB (F),ODB (F),CapTot (kBtuh),CapSen (kBtuh),Pwr (kW)\n");

	float capNomTonsX = rs_ClgCapNomTons();		// exact
	float capNomTons = rs_ClgCapNomTons( .5f);	// nearest .5 ton
	float vfPerTonNom = 400.f * capNomTons / capNomTonsX;
	if (Top.tp_airSH <= 0.f)
		Top.tp_Psychro();		// set air constants

	for (int iAF=0;fAF[ iAF]>0.f; iAF++)
	{	rs_vfPerTon = vfPerTonNom * fAF[ iAF];
		float cfm = rs_vfPerTon * capNomTonsX;
		rs_SetRunConstants();
		rs_SetupSizes();
		for (int idbI=0; tdbI[ idbI] > 0.f; idbI++  )
		{	for (int iwbI=0; twbI[ iwbI] > 0.f; iwbI++)
			{	if (twbI[ iwbI] >= tdbI[ idbI])
					continue;
				// entering conditions
				rs_twbIn = twbI[ iwbI];
				float w = psyHumRat1( tdbI[ idbI], rs_twbIn);
				rs_asIn.as_Set( tdbI[ idbI], w);
				for (int idbO=0; tdbO[ idbO] > 0.f; idbO++)
				{	// outdoor conditions
					rs_asOut = rs_asIn;
					rs_tdbOut = tdbO[ idbO];
					rs_CoolingOutletAirState();
					float capTot, capSen, pwrIn;
					rs_GetPerfClg( capTot, capSen, pwrIn);
					fprintf( f, "%.f,%.f,%.f,%.f,%.2f,%.2f,%.2f\n",
						cfm, tdbI[ idbI], rs_twbIn, rs_tdbOut,
							capTot, capSen, pwrIn);
				}
			}
		}
	}

	fclose( f);

	*this = rsSave;		// restore entry state

	return RCOK;
}		// RSYS::rs_GeneratePerfMapAC
//-------------------------------------------------------------------------------
RC RSYS::rs_ExportCorrelationValues()	// write CSV file containing values from RSYS correlations
// testing aid
{
	static float fAF[] = { .550f, .700f, .875f, 1.f, 1.125f, 0.f };
	static float tdbI[] = { 70.f, 72.f, 74.f, 76.f, 78.f, 80.f, 85.f, 0.f };
	static float twbI[] = { 54.f, 56.f, 58.f, 60.f, 62.f, 64.f, 67.f, 71.f, 0.f };
	static float tdbO[] = { 65.f, 75.f, 82.f, 85.f, 95.f, 105.f, 115.f, 0.f };

	// write to file PM_<name>.csv
	const char* nameX = name;
	if (IsBlank(nameX))
		nameX = "RSYS";
	FILE* f = fopen(strtprintf("CORVALS_%s.csv", nameX), "wt");
	if (!f)
		return RCBAD;		// can't open file

	RSYS rsSave(*this);		// save for restore at exit
								//   (we alter mbrs here)

	fprintf(f, "%s\n", rs_desc.CStrIfNotBlank(nameX));

	fprintf(f, "cfm/ton,ODB (F),EDB (F),EWB (F),RH,SHR,fCap,fEER,fInp\n");

	float vfPerTonNom = 400.f;

	if (Top.tp_airSH <= 0.f)
		Top.tp_Psychro();		// set air constants

	for (int iAF = 0; fAF[iAF] > 0.f; iAF++)
	{
		rs_vfPerTon = vfPerTonNom * fAF[iAF];
		char sVf[20];
		snprintf(sVf, sizeof(sVf), "%.f", rs_vfPerTon);
		for (int idbO = 0; tdbO[idbO] > 0.f; idbO++)
		{	// outdoor conditions
			rs_tdbOut = tdbO[idbO];
			char sDbO[20];
			snprintf(sDbO, sizeof(sDbO), "%.f", rs_tdbOut);

			for (int idbI = 0; tdbI[idbI] > 0.f; idbI++)
			{	// entering dry bulb
				rs_tdbCoilIn = tdbI[idbI];
				char sDbI[20];
				snprintf(sDbI, sizeof(sDbI), "%.f", rs_tdbCoilIn);
				for (int iwbI = 0; twbI[iwbI] > 0.f; iwbI++)
				{	// entering wet bulb conditions
					rs_twbCoilIn = twbI[iwbI];
					if (rs_twbCoilIn > rs_tdbCoilIn)
						continue;
					float rh = psyRelHum(rs_tdbCoilIn, rs_twbCoilIn);
					rs_CoolingSHR();
					rs_CoolingCapF1Spd();
					float fCondInp = rs_CoolingEff1Spd();
					fprintf(f, "%s,%s,%s,%.f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
						sVf, sDbO, sDbI, rs_twbCoilIn, rh, rs_SHR, rs_fCondCap, rs_fCondEER, fCondInp);
				}
			}
		}
	}

	fclose(f);

	*this = rsSave;		// restore entry state

	return RCOK;
}		// RSYS::rs_ExportCorrelationValues
//-----------------------------------------------------------------------------
#if 0
// out of service, 4-15-2024
RC RSYS::rs_PerfDataASHP()
{
	// write to file PD_<name>.csv
	const char* nameX = name;
	int iHSPF = int( 0.5f + 10.f*rs_HSPF);
	if (IsBlank( nameX))
		nameX = "RSYS";
	FILE* f = fopen( strtprintf( "PD_%s%d.csv", nameX, iHSPF), "wt");
	if (!f)
		return RCBAD;		// can't open file

	RSYS rsSave( *this);		// save for restore at exit
								//   (we alter mbrs here)

	fprintf( f, "%s,All values include rating fan power,,%s,CSE %s\n",
			rs_desc.CStrIfNotBlank( nameX),
			Top.runDateTime.CStr(),
			Top.tp_progVersion.CStr());

	fprintf( f, "HSPF=%0.2f, HSPF CSE=%0.3f,,HSPF MP=%0.3f,,HSPF ESL=%0.3f,,HSPF E+=%0.3f\n",
		rs_HSPF,
		rs_HSPFCheckASHP( 0), rs_HSPFCheckASHP( 1), rs_HSPFCheckASHP( 2), rs_HSPFCheckASHP( 3));

	fprintf( f, "tODB,cap CSE,inp CSE,COP CSE,cap MP,inp MP,COP MP,cap ESL,inp ESL,COP ESL,cap E+,inp E+,COP E+\n");

	for (int iR=0; iR<3; iR++)
	{	float tRat[] = { 17, 35, 47 };
		rs_PerfDataASHP1( f, tRat[ iR]);
	}

	fprintf( f, "\n");

	for (int iDB = -10; iDB<63; iDB++)
		rs_PerfDataASHP1( f, float( iDB));

	fprintf( f, "\ncap17/cap47 vs HSPF\n");

	for (iHSPF=65; iHSPF<140; iHSPF++)
	{	rs_HSPF = float( iHSPF)/10.f;
		fprintf( f, "%0.3f,%0.3f\n", rs_HSPF, rs_CapRat1747());
	}

	// loop over range of HSPFs and cap47s, derive back-calc HSPF
	fprintf( f, "\nBack-Calculated HSPF\n");
	const float cds[] = { 0.f, .125f, .25f, -1.f };
	const float hspfs[] = { 6.5f, 7.0f, 7.5f, 8.0f, 8.5f, 9.0f, 9.5f, 10.0f, 10.5f, 11.0f, 11.5f, 12.f, 12.5f, 13.f, -1. };
	const float cap47s[] = { 18000.f, 24000.f, 54999.f, 55000.f, -1.f };

	ClrSet( RSYS_COP47);
	ClrSet( RSYS_CAP35);
	ClrSet( RSYS_COP35);
	ClrSet( RSYS_CAP17);
	ClrSet( RSYS_COP17);

	for (int iH=0; hspfs[ iH] > 0.f; iH++)
	{	rs_HSPF = hspfs[ iH];
		for (int iC=0; cap47s[ iC] > 0.f; iC++)
		{	rs_cap47 = cap47s[ iC];
			for (int iD=0; cds[ iD] >= 0.f; iD++)
			{	rs_CdH = cds[ iD];
				rs_SetupASHP();
				if (iD==0)
					fprintf( f, "%0.3f,%0.f", rs_HSPF, rs_cap47);
				float bcHSPF = rs_HSPFCheckASHP( 0);
				fprintf( f, ",%0.3f", bcHSPF);
			}
			fprintf( f, "\n");
		}
	}

	fclose( f);
	*this = rsSave;		// restore entry state

	return RCOK;
}		// RSYS::rs_PerfDataASHP
//-----------------------------------------------------------------------------
void RSYS::rs_PerfDataASHP1(		// test aid: ASHP performance from alt models
	FILE* f,			// where to write
	float tdbOut)		// outdoor temp, F
// all values include rating fan power
{
	// CSE "original"
	float COP;
	float cap = rs_PerfASHP( 0+0x100, tdbOut, COP, 0.f);
	float inp = cap / COP;

	// Micropas
	float COPMP;
	float capMP = rs_PerfASHP( 1+0x100, tdbOut, COPMP, 0.f);
	float inpMP = capMP / COPMP;

	// ESL
	float COPESL;
	float capESL = rs_PerfASHP( 2+0x100, tdbOut, COPESL, rs_fanHRtdH);
	float inpESL = capESL / COPESL;

	// E+
	float COPEP;
	float capEP = rs_PerfASHP( 3+0x100, tdbOut, COPEP, rs_fanHRtdH);
	float inpEP = capEP / COPEP;

	fprintf( f, "%.f, %.f, %.f, %.3f, %.f, %.f, %.3f, %.f, %.f, %.3f, %.f, %.f, %.3f\n",
		tdbOut, cap, inp, COP,
				capMP, inpMP, COPMP,
		        capESL, inpESL, COPESL,
				capEP, inpEP, COPEP);

}	// RSYS::rs_PerfDataASHP1
#endif
//-----------------------------------------------------------------------------
float RSYS::rs_PerfASHP(		// ASHP performance (simplified call)
	int ashpModel,		// model options
						//  0x100: do NOT model defrost heating
						//         else model defrost if available
	float tdbOut,		// outdoor dry bulb, F
	float speedF,
	float& COP,			// returned: compressor-only full speed COP at tdbOut
	float fanHAdj /*=0.f*/)		// fan power adjustment, Btuh
						// removed from capacity and input (before COP calc)
// returns total heating capacity (compressor + capDefrostHt), Btuh
{
	float capHt, inpHt, capDfHt;
	COP = rs_PerfASHP2(ashpModel, tdbOut, speedF, fanHAdj,
		capHt, inpHt, capDfHt);

	return capHt;

}	// RSYS::rs_PerfASHP
//------------------------------------------------------------------------------
float RSYS::rs_PerfASHP2(		// ASHP performance
	int ashpModel,		// model options
						//  0x100: do NOT model defrost heating
						//           else model defrost if available
						// Note: variable speed supported for CSE only
	float tdbOut,		// outdoor dry bulb, F
	float speedF,		// speed, 0 - 1
	float fanHAdj,		// fan power adjustment, Btuh
						// removed from capacity and input (before COP calc)
	float& capHtGross,	// returned: gross (compressor-only) full speed heating capacity
	float& inpHtGross,	// returned: gross (compressor-only) full speed input power, Btuh
	float& capDfHt,		// returned: defrost full speed aux heat output, Btuh
						//    = strip/furnace output to make up compressor cooling
						//    (depends on rs_defrostModel)
	float COPAdjF /*=1.f*/)	// COP adjustment factor
							//   multiplies final COP result
// returns gross (compressor-only) full-speed COP (as adjusted by COPAdjF)
{
	RC rc = RCOK;
	capDfHt = 0.f;
	bool bDoDefrostAux = (ashpModel & 0x100) == 0
		           && rs_defrostModel == C_RSYSDEFROSTMODELCH_REVCYCLEAUX;

	if (rs_IsPMHtg())
	{	// rated net capacity
		rc |= rs_pPMACCESS[0]->pa_GetCapInp( tdbOut, speedF, capHtGross, inpHtGross);

		// Note: heating rated fan heat is based on rs_cap95 (cooling capacity)
		float fanHeatRated = rs_FanPwrRatedAtSpeedF(0, rs_cap95, speedF);

#if defined( FCHG_COP)
		capHtGross -= fanHeatRated;		// convert to gross
		inpHtGross = (inpHtGross - fanHeatRated) / rs_fChgH;
#else
		capHtGross = (capHtGross - fanHeatRated) / rs_fChgH;	// convert to gross
		inpHtGross -= fanHeatRated;
#endif

		// adjust capHt and inpHt for re defrost
		float capHtGrossNoDefrost = capHtGross;
		float defrostRunFraction = rs_DefrostAdjustCapInp(tdbOut, capHtGross, inpHtGross);

		if (bDoDefrostAux && defrostRunFraction > 0.f)
		{	// enough auxilary to cancel reverse cycle penalty
			capDfHt = capHtGrossNoDefrost - capHtGross;
		}
	}
	else
	{
		// CSE AHRI 210/240 model
		// input power and heating capacity at current conditions (Btuh)
		int iDefrost = rs_HasDefrost() && tdbOut > 17.f && tdbOut < 45.f;
		float tdbM17 = tdbOut - 17.f;
#if defined( FCHG_COP)
		inpHtGross = (rs_inp17 + rs_ASHPInpF[iDefrost] * tdbM17 - fanHAdj) / rs_fChgH;
		capHtGross = rs_cap17 + rs_ASHPCapF[iDefrost] * tdbM17 - fanHAdj;
#else
		inpHtGross = rs_inp17 + rs_ASHPInpF[iDefrost] * tdbM17 - fanHAdj;
		capHtGross = (rs_cap17 + rs_ASHPCapF[iDefrost] * tdbM17 - fanHAdj) * rs_rChgH;
#endif
		if (iDefrost && bDoDefrostAux)
		{	// C_RSYSDEFROSTMODELCH_REVCYCLEAUX
			// include sufficient aux heat to make up for ASHP cooling
			// limit to available aux heat capacity
			// Note: does NOT over-commit aux because compressor
			//   and aux do not run simultaneously.
			capDfHt = (rs_ASHPCapF[0] - rs_ASHPCapF[1]) * tdbM17;
		}
	}

	if (capDfHt > rs_capAuxH)
		capDfHt = rs_capAuxH;

	float COP;
	if (capHtGross < 0.01f || inpHtGross < 0.01f)
	{	COP = capHtGross = inpHtGross = 0.f;
	}
	else
		COP = COPAdjF * capHtGross / inpHtGross;	// compressor-only efficiency

	capHtGross += capDfHt;		// include defrost heat in capacity

	return COP;
}		// RSYS::rs_PerfASHP2
//-----------------------------------------------------------------------------
float RSYS::rs_DefrostAdjustCapInp(		// runtime-based defrost model
	float tdbOut,		// outdoor dry-bulb, F
	float& capGrossHt,	// gross heating capacity, Btuh (returned updated)
	float& inpGrossHt) const	// gross heating input, Btuh (returned updated)
// ResNet model
// returns fraction of operating time in defrost (0 - 0.08)
{
	float defrostRunFraction = 0.f;
	if (rs_HasDefrost())
	{
		defrostRunFraction = bracket(0.f, 0.134f-.003f*tdbOut, 0.08f);
		capGrossHt *= (1.f - 1.8f*defrostRunFraction);
		inpGrossHt *= (1.f - 0.3f*defrostRunFraction);
	}

	return defrostRunFraction;
}	// RSYS::rs_DefrostAdjustCapInp
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupASHP()		// set ASHP defaults and derived parameters
// caution: all required args assumed present and >0
//   call during autosize or later
// redundant calls OK
{
	RC rc = RCOK;

	// inter-default cap95 / cap47
	//   at least one is present (see rs_CkF)
	if (!IsSet(RSYS_CAP95))
		rs_cap95 = ASHPCap95FromCap47(rs_cap47, IsSet(RSYS_CAPRAT9547), rs_capRat9547);
	else if (!IsSet(RSYS_CAP47))
		rs_cap47 = ASHPCap47FromCap95(rs_cap95, IsSet(RSYS_CAPRAT9547), rs_capRat9547);

	if (rs_IsPMHtg())
		rc |= rs_CheckAndSetupPMHtg(pmSETRATINGS);

	// fan power included in heating ratings
	// derive independently of cooling even though actually the same fan
	//   WHY: CA procedures allow separate heating / cooling sizing
	// say 0 for air-to-water
	rs_fanHRtdH = rs_IsASHPHydronic()
		? 0.f
		: rs_FanPwrRated(0, rs_cap47, PMSPEED::RATED);

	if (!rs_IsVCHtg())
	{	// capacities
		if (!IsSet(RSYS_CAP17) || rs_IsPkgRoom())
			rs_cap17 = max(rs_CapRat1747()*rs_cap47, 1.f);

		if (!IsSet(RSYS_CAP35))
			rs_cap35 = rs_Cap35Default(rs_cap47, rs_cap17);

		// COPs
		if (rs_IsPkgRoom())
		{
			rs_COP17 = 0.6870f * rs_COP47;
			// rs_cap17 set above
		}
		else if (!rs_IsASHPHydronic())
		{	// COP47: per HSPF regression
			// COP17 / COP35: adjust so HSPF is matched
			if (!IsSet(RSYS_COP47))
				rs_COP47 = 0.3225f * rs_HSPF + 0.9099f;
			if (!IsSet(RSYS_COP17))
			{
				rs_COP17 = 0.2186f * rs_HSPF + 0.6734f;
				int iTry;
				RC rc1;
				const int nTry = 40;
				for (iTry = 0; iTry<nTry; iTry++)
				{
					rc1 = rs_HSPFMatchASHP();	// adjust COP17 to be
					//   consistent with rs_HSPF
					if (rc1 || rs_COP17 < rs_COP47 || IsSet(RSYS_COP47))
						break;	// accept rs_COP17 if < rs_COP47 or rs_COP47 is fixed
					rs_COP47 += 0.1f;	// try again with higher rs_COP47
				}
				if ((rc1 || iTry==nTry) && !rs_isAuszH)
					rc |= oer("No reasonable value found for rsCOP17 and/or rsCOP47."
							  "\n   Check rsHSPF and other heating inputs.");
			}
		}
	
		// setup ASHP runtime data
		//   makes slope data and Btwxt perf map
		rc |= rs_SetHeatingASHPConstants1Spd();
	}

#if 0
	// issue message if rs_capAuxH <= compressor cap
	if (!rs_isAuszH)
		rc |= rs_CheckCapAuxH();
#endif

#if 0 && defined( _DEBUG)
	// back-calc checks
	if (rc == RCOK)
	{
		float COP;
		[[ maybe_unused]] float capHt = rs_PerfASHP(0, 47.f, 1.f, COP);
		capHt = rs_PerfASHP(0, 17.f, 1.f, COP);
	}
#endif

	return rc;
}		// RSYS::rs_SetupASHP

//----------------------------------------------------------------------------
float RSYS::rs_CapRat1747() const
// returns ratio cap17 / cap47
{
	// multi-segment model from Abram Conant, 5-31-2013
	//  includes HSPF < 7.5
	float capRat =
		           IsSet( RSYS_CAPRAT1747) ? rs_capRat1747
		         : rs_IsPkgRoom()      ? .6184f		// per Abram Conant, 6-4-2020
				 : rs_IsASHPHydronic() ? .63f		// plausible result when HSPF not known
				 : rs_HSPF < 7.5f    ? .1113f * rs_HSPF - .22269
				 : rs_HSPF < 9.5567f ? .017f  * rs_HSPF + .4804f
		         : rs_HSPF < 10.408f ? .0982f * rs_HSPF - .2956f
				 :                     .0232f * rs_HSPF + .485f;
	return capRat;
}		// RSYS::rs_CapRat1747
//-----------------------------------------------------------------------------
float RSYS::rs_Cap35Default(		// default 35 F heating capacity
	float cap47,		// 47 F heating capacity, any power units
	float cap17) const	// 17 F heating capacity
// returns 35 F heating capacity, consistent units
{
	float cap35 = cap17 + 0.6f * (cap47 - cap17);
	if (rs_HasDefrost())
		cap35 *= 0.9f;	// capacity reduction for defrost
	return cap35;
}	// RSYS::rs_Cap35Default
//-----------------------------------------------------------------------------
float RSYS::rs_Inp35Default(		// default 35 F input power
	float inp47,		// 47 F input power, any power units
	float inp17) const	// 17 F input power
// returns 35 F input power, consistent units
{
	float inp35 = inp17 + 0.6f * (inp47 - inp17);
	if (rs_HasDefrost())
		inp35 *= 0.985f;		// defrost power reduction
	return inp35;
}	// RSYS::rs_Inp35Default
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupWSHP()		// set WSHP defaults and derived parameters
// caution: all required args assumed present and >0
//   call during autosize or later
// redundant calls OK
{
	RC rc = RCOK;

	// inter-default cap95 (aka capC) <-> capH
	//   at least one is present (see rs_CkF)
	if (!IsSet(RSYS_CAP95))
		rs_cap95 = WSHPPerf.whp_CapCFromCapH(rs_capH, IsSet(RSYS_CAPRATCH), rs_capRatCH);
	else if (!IsSet(RSYS_CAPH))
		rs_capH = WSHPPerf.whp_CapHFromCapC(rs_cap95, IsSet(RSYS_CAPRATCH), rs_capRatCH);

	rs_fanHRtdH = rs_FanPwrRated(0, rs_capH, PMSPEED::RATED);

	return rc;
}		// RSYS::rs_SetupWSHP
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupCHDHW()		// check/set up combined heat / DWH
// call from rs_TopRSys2()
//    *after* rs_SetWorkingPtrs()
//    *after* topDHW() -- DHWSYS, DHWHEATER, etc must exist

// returns RCOK iff CHDHW config is valid
{
	RC rc = RCOK;

	DHWSYS* pWS = rs_GetCHDHWSYS();

	if (!pWS)
		rc |= oer("Missing rsCHDHWSYS");	// impossible? due to prior checks
	else
		rc |= pWS->ws_CheckCHDHWConfig(	this);

	if (!rc)
	{
		rs_pCHDHW = new CHDHW( this);
		float ratedSFP = float(rs_pCHDHW->hvt_GetRatedSpecificFanPower());
		if (!IsSet(RSYS_FANSFPH))
			rs_fanSFPH = ratedSFP;
		rc |= rs_pCHDHW->hvt_Init(rs_fanSFPH);

		rs_tdDesH = rs_pCHDHW->hvt_GetTRise();
		rs_capH = rs_pCHDHW->hvt_GetRatedCap();

		// rated fan heat (unused?)
		rs_fanHRtdH = ratedSFP * rs_pCHDHW->hvt_GetRatedBlowerAVF() * BtuperWh;
	}

	return rc;
}		// RSYS::rs_SetupCHDHW
//----------------------------------------------------------------------------
float RSYS::rs_CurCapHtCHDHW(		// current CHDHW heating cap etc
	float speedF)		// current speed, 0 - 1
// sets (if needed) rs_tCoilEW, rs_capHtFS, and rs_speedFMin
// returns capacity at speedF, Btuh
{

	if (rs_capHtFS == 0.f)
	{	// set up full speed info once per step
		DHWSYS* pWS = rs_GetCHDHWSYS();
		rs_tCoilEW = pWS->ws_GetCHDHWTSupply();
		float capHtMin;
		rs_pCHDHW->hvt_CapHtgMinMax(rs_tCoilEW, capHtMin, rs_capHtFS);
		rs_speedFMin = capHtMin / rs_capHtFS;
	}
	return rs_capHtFS * speedF;
}		// RSYS::rs_CurCapHtCHDHW
//-----------------------------------------------------------------------------
// helper performance point re btwxt setup
// = temperature + array of perf values
struct VSPERFP
{
	enum { ppCAPHS, ppINPHS, ppCAPLS, ppINPLS, ppCOUNT };
	VSPERFP(float temp, float capHS, float COPHS, float capLS, float COPLS)
		: pp_temp(temp)
	{
		pp_data[ppCAPHS] = capHS;
		pp_data[ppINPHS] = abs(capHS / max(COPHS, .1f));
		pp_data[ppCAPLS] = capLS;
		pp_data[ppINPLS] = abs(capLS / max(COPLS, .1f));
	}

	double pp_temp;		// temp for this point, F
	std::array< double, ppCOUNT> pp_data{ 0. };	// data
	double operator [](int i) const { return pp_data[i]; }
	static int NData() { return ppCOUNT; }
};	// struct VSPERFP
//-----------------------------------------------------------------------------
RC RSYS::rs_SetHeatingASHPConstants1Spd()	// finalize constant data for 1 spd simulation
// uses: rs_cap/COP 47/35/17/05
// sets: rs_inp17/35/47, rs_COP35 (if needed), slopes for 1 spd model

// returns RCOK iff success
{
	RC rc = RCOK;
#if defined( _DEBUG)
	if (rs_IsPMHtg())
		return oer("rs_SetHeatingASHPConstants1Spd(): bad call on PM type");
#endif
	
	rs_inp47 = rs_cap47 / max(rs_COP47, .1f);	// full speed input power, Btuh
	rs_inp17 = rs_cap17 / max(rs_COP17, .1f);

	if (!IsSet(RSYS_COP35))
	{
		rs_inp35 = rs_Inp35Default(rs_inp47, rs_inp17);
		rs_COP35 = rs_cap35 / max(rs_inp35, .1f);
	}
	else
		rs_inp35 = rs_cap35 / max(rs_COP35, .1f);

	// line slopes for cap vs odb and inp vs odb
	rs_ASHPCapF[0] = (rs_cap47-rs_cap17) / (47.f-17.f);
	rs_ASHPInpF[0] = (rs_inp47-rs_inp17) / (47.f-17.f);

	rs_ASHPCapF[1] = (rs_cap35-rs_cap17) / (35.f-17.f);
	rs_ASHPInpF[1] = (rs_inp35-rs_inp17) / (35.f-17.f);

	// Not VC, set min speed values to same as full speed
	//  (prevents 0s in reports)
	rs_COPMin47 = rs_COP47;
	rs_COPMin35 = rs_COP35;
	rs_COPMin17 = rs_COP17;
	rs_cap05 = rs_PerfASHP(0, 5.f, 1.f, rs_COP05);
	rs_COPMin05 = rs_COP05;
	return rc;
}		// RSYS::rs_SetHeatingASHPConstants1Spd
//-----------------------------------------------------------------------------
RC RSYS::rs_CheckAndSetupPMHtg(		// one-time setup for variable capacity
	int options)			// 0
// if full setup and rc==0, rs_perfmapAccessHtg object prepared for runtime use
// returns RCOK on success
//    else RCxx, errors msg'd
{
	RC rc = RCOK;

	const PERFORMANCEMAP* pPM = nullptr;

	if (rs_IsPMHtg())
		rc |= rs_GetAndCheckPERFORMANCEMAP(RSYS_PERFMAPHTGI, pPM);
	else
		rc |= oer("Not PM");

	if (rc || options & pmCHECKONLY)
		return rc;		// bad or only check

	ASSERT(pPM != nullptr);		// program error if !pPM

	// setup performance map
	delete rs_pPMACCESS[0];
	rs_pPMACCESS[0] = new PMACCESS();
	rc |= rs_pPMACCESS[0]->pa_Init(pPM, this, "Heating", rs_cap47);

	if (!rc && options & pmSETRATINGS)
		rc |= rs_SetRatingsPMHtg();

	return rc;
}		// RSYS::rs_CheckAndSetupPMHtg
//-----------------------------------------------------------------------------
RC RSYS::rs_SetRatingsPMHtg()		// derive heating ratings
// most (all?) of the derived values are for reporting only
// no uses in calcs
{
	RC rc = RCOK;

	if (rs_IsPMHtg())
	{
		float capSink;
		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP( 5.f, rs_cap05, rs_COP05);
		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP( 5.f, capSink, rs_COPMin05, PMSPEED::MIN);

		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(17.f, rs_cap17, rs_COP17);
		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(17.f, capSink, rs_COPMin17, PMSPEED::MIN);

		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(35.f, rs_cap35, rs_COP35);
		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(35.f, capSink, rs_COPMin35, PMSPEED::MIN);

		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(47.f, rs_capH, rs_COP47);
		rc |= rs_pPMACCESS[0]->pa_GetRatedCapCOP(47.f, capSink, rs_COPMin47, PMSPEED::MIN);

		rc |= rs_CheckPMRatingConsistency(rs_cap47, rs_capH, "cap47", rs_perfMapHtgi);
	}
	else
		rc |= oer("rs_SetRatingsPMHtg() bad call -- rsType is not PM");

	return rc;
}	// RSYS::rs_SetRatingsPMHtg
//-----------------------------------------------------------------------------
RC RSYS::rs_CheckAndSetupPMClg(	// one-time setup for variable capacity
	int options)	// option bits
					//   pmCHECKONLY: verify input only (no btwxt setup)
					//   pmSETRATINGS: derive ratings for 82 F, 95 F, 115 F
// if full setup and rc==0, rs_perfmapAccessClg object prepared for runtime use
// returns RCOK on success
//    else RCxx, errors msg'd
{
	RC rc = RCOK;

	const PERFORMANCEMAP* pPM = nullptr;

	if (rs_IsPMClg())
		rc |= rs_GetAndCheckPERFORMANCEMAP(RSYS_PERFMAPCLGI, pPM);
	else
		rc = oer("Not PM");

	if (rc || options & pmCHECKONLY)
		return rc;		// bad or only check

	ASSERT(pPM != nullptr);		// program error if !pPM

	// setup performance map
	delete rs_pPMACCESS[1];
	rs_pPMACCESS[1] = new PMACCESS();
	// make sure ref cap is < 0
	rc |= rs_pPMACCESS[1]->pa_Init(pPM, this, "Cooling", -abs( rs_cap95));

	if (!rc && options & pmSETRATINGS)
		rc |= rs_SetRatingsPMClg();

	return rc;
}		// RSYS::rs_CheckAndSetupPM
//-----------------------------------------------------------------------------
RC RSYS::rs_SetRatingsPMClg()		// derive cooling ratings
{
	RC rc = RCOK;

	if (rs_IsPMClg())
	{	float cap;	// temporary for return values
		
		rc |= rs_pPMACCESS[1]->pa_GetRatedCapCOP(82.f, cap, rs_COP82);
		rs_cap82 = abs(cap);

		rc |= rs_pPMACCESS[1]->pa_GetRatedCapCOP(95.f, cap, rs_COP95);
		rs_EER95 = rs_COP95 * BtuperWh;
		rc |= rs_CheckPMRatingConsistency(rs_cap95, abs( cap), "cap95", rs_perfMapClgi);

		rc |= rs_pPMACCESS[1]->pa_GetRatedCapCOP(115.f, cap, rs_COP115);
		rs_cap115 = abs(cap);

	}
	else
		rc = oer("rs_SetRatingsPMClg() bad call -- rsType is not PM");

	return rc;

}	// RSYS::rs_SetRatingsPMClg
//-----------------------------------------------------------------------------
RC RSYS::rs_CheckPMRatingConsistency(	// verify performance map <-> rating consistency
	float ratingExp,	// expected value
	float ratingPM,		// value generated from performance map
	const char* what,	// name of rating, e.g. "cap47"
	TI iPM) const		// source performance map
// returns RCOK iff performance map generated value matches expected
//    else nz (performance map not correctly normalized?)
{
	RC rc = RCOK;

	if (frDiff(ratingPM, ratingExp) > 0.001f)
	{
		const PERFORMANCEMAP* pPM = PerfMapB.GetAtSafe(iPM);
		const char* pmName = pPM ? pPM->Name() : "?";
		rc |= oer("Performance map result for %s (%.3f) does not match rated value (%.3f).\n"
					"    Check PERFORMANCEMAP '%s' input.",
					what, ratingPM, ratingExp, pmName);
	}

	return rc;
}		// RSYS::rs_CheckPMRatingConsistency
//-----------------------------------------------------------------------------
float RSYS::rs_CapEffASHP(			// performance at current conditions
	float tdbOut/*=-999.f*/,	// outdoor dry-bulb temp, F
								//   default = rs_tdbOut (weather file value or expression)
	float speedF /*=-1.f*/,
	int ashpModel /*=0*/,		// alternative model
								//   0=CSE, 1=MP, 2=ESL, 3=E+
								//   +0x100: do NOT model defrost heating
								//   +0x200: ignore rs_fEffH (efficiency adjustment)
	float fanHRtd /*=-1.f*/,	// fan power included in rating, Btuh
								//   default = rs_fanHRtdH
	float fanHOpr /*=-1.f*/,	// operating fan power, Btuh
								//   default = rs_fanHeatH
	float COPAdjF /*=-1.f*/)	// COP adjustment factor
								//   default = rs_fEffH
// model does not depend on indoor conditions
// sets rs_effHt, rs_capHt, and rs_capDfHt
// returns rs_capHt, Btuh = total heating capacity including fan and defrost if any
{
	if (tdbOut < -998.f)
		tdbOut = rs_tdbOut;
	if (speedF < 0.f)
		speedF = rs_speedF;
	if (fanHRtd < 0.f)
		fanHRtd = rs_fanHRtdH;
	if (fanHOpr < 0.f)
		fanHOpr = rs_fanHeatH;
	if (COPAdjF < 0.f)
		COPAdjF = rs_fEffH;

	if (tdbOut < rs_ASHPLockOutT)
	{	rs_effHt = 0.f;				// compressor is unavailable
		rs_capHt = fanHOpr;	// compressor does nothing
		rs_inpHt = 0.f;
	}
	else
	{	rs_effHt = rs_PerfASHP2(ashpModel, tdbOut, speedF, fanHRtd, rs_capHt, rs_inpHt, rs_capDfHt,
			COPAdjF);
		rs_capHt += fanHOpr;
	}
	return rs_capHt;
}		// RSYS::rs_CapEffASHP
//-----------------------------------------------------------------------------
void RSYS::rs_CapEffASHP2(	// performance at current conditions (no defaults)
	float& capHtGross,
	float& inpHtGross,
	float& capDfHt)
// main simulation variant of rs_CapEffASHP() (see above)
{
	if (rs_tdbOut < rs_ASHPLockOutT)
	{	rs_effHt = 0.f;		// compressor is unavailable
		capHtGross = 0.f;	// compressor does nothing
		inpHtGross = 0.f;
		capDfHt = 0.f;
	}
	else
	{	rs_effHt = rs_PerfASHP2( 0, rs_tdbOut, rs_speedF, rs_fanHRtdH, capHtGross, inpHtGross, capDfHt,
			rs_fEffH);
		// add operating fan heat/power
		ASSERT(rs_capH == rs_cap47);	// check setup
#if defined( _DEBUG)
		if (rs_fanPwr != rs_FanPwrOperatingAtSpeedF(0, rs_speedF))
			printf("\nHeating fanPwr mismatch");
#endif
	}
}		// RSYS::rs_CapEffASHP2
//-----------------------------------------------------------------------------
/*static*/ double RSYS::rs_CallCalcHSPF( void* pO, double& COP17)
// callback function for secant()
//    calcs HSPF using current COP17
//    re search for COP17 consistent with specified HSPF
// returns HSPF
{
	RSYS* pRS = (RSYS *)pO;
	if (COP17 < 1.01)
		COP17 = 1.01;		// prevent impossible
	pRS->rs_COP17 = COP17;
	pRS->rs_SetHeatingASHPConstants1Spd();
	int ashpModel = 0;
	double hspf = pRS->rs_HSPFCheckASHP( ashpModel);
	return hspf;
}		// RSYS::rs_CallCalcHSPF
//----------------------------------------------------------------------------
RC RSYS::rs_HSPFMatchASHP()			// adjust COP17 to yield user's HSPF
// uses rs_HSPF, rs_cap47, rs_COP47, rs_cap35, rs_COP35, rs_cap17 (at least)
// adjusts rs_COP17 so calculated HSPF = rs_HSPF
// returns RCOK iff COP17 found
{
	int rc = RCOK;
	rs_SetHeatingASHPConstants1Spd();		// ensure consistent starting point
	int ashpModel = 0;
	double hspf = rs_HSPFCheckASHP( ashpModel);
	double COP17 = rs_COP17;
	float dCOP17 = hspf > rs_HSPF ? -.1f : +.1f;
	int ret = secant( rs_CallCalcHSPF, this, rs_HSPF, .001,
					COP17, hspf,			// x1, f1
					COP17 + dCOP17);		// x2 (f2)
	if (ret != 0)
		rc = RCBAD;		// search failed (caller must handle)

	return rc;

}		// RSYS::HSPFMatchASHP
//----------------------------------------------------------------------------
float RSYS::rs_HSPFCheckASHP(		// calculate region 4 HSPF
	int options/*=0*/)		// low bits = ashpModel
							//   0x100 = DbPrintf info for all bins
// testing aid, "back calculates" HSPF from ASHP ratings
//    method = AHRI 210/240 single speed
// Prerequisite: rs_CalcInputsAndSlopesASHP() or equivalent
//               (sets values used by rs_CapEffASHP())
// returns HSPF calculated from RSYS properties
{
// bin hours for region 4
static int nHrReg4[] =
//   62  57  52 ...
{	297,250,232,209,225,245,283,196,124, 81, 58, 29, 14,  5,  2, 0 };
static double fHrReg4[] =
//   62      57    52 ...
{	0.132,0.111,0.103,0.093,0.100,0.109,0.126,0.087,0.055,0.036,0.026,0.013,0.006,0.002,0.001, 0. };

	if (!rs_IsASHP())
		return -1.f;		// not ASHP!

	int ashpModel = options & 0x1F;
	int bDoPrint = (options & 0x100) != 0;

	[[ maybe_unused]] int nHrTot = 0;	// total bin hours (s/b 2250 for reg 4)
	const float tDbIn = 65.f;	// indoor design temp
	const float tDbDes = 5.f;	// outdoor design temp
	const float DHR				// design heating requirement per AHRI
			= rs_DHR( rs_cap47 * (tDbIn - tDbDes) / 60.f);
	double outTot = 0.;		// total heat delivered (including fan heat), Btu
	double inpTot = 0.;		// total input (including fan), Btu
	const float C = 0.77f;	// AHRI "correction factor"
	const float cd = rs_CdH; // cyclic degradation coefficient
	const float tOff = 0.f;	// low temp cut-out "off" temp
	const float tOn  = 5.f;	// low temp cut-out "on" temp
	float lockOutTSave = rs_ASHPLockOutT;
	rs_ASHPLockOutT = -999.f;	// disable our lockout model
								//   handled via delta in loop below

	rs_CapEffASHP( 47.f, ashpModel+0x100);		// debug aid (check values with fan power)
	rs_CapEffASHP( 17.f, ashpModel+0x100);

	if (bDoPrint)
		DbPrintf( "\nHSPF calc   model=%d\n", ashpModel);

	for (int iBin=0; nHrReg4[ iBin] > 0; iBin++)
	{	nHrTot += nHrReg4[ iBin];
		float tDbBin = float( 62 - iBin*5);

		rs_CapEffASHP( tDbBin, 1.f, ashpModel+0x100, 0.f, 0.f, 1.f);	// derive capacity and efficiency for bin temp
																// no fan heat adjustments
																// no COP adjust
		// load
		float f = (tDbBin < tDbIn && tDbDes < tDbIn)
					? (tDbIn - tDbBin) / (tDbIn - tDbDes)
					: 0.f;
		double BL = f * C * DHR;		// building heating load (Eqn 4.2-2)
										//  always >= 0

		// other factors
		double X = rs_capHt > 0.f
					    ? min( BL/rs_capHt, 1.)
						: double( BL > 0.);		// 1. iff load else 0.
		double PLF = 1. - cd * ( 1. - X);

		double Eh = rs_effHt > 0.
						? rs_capHt / rs_effHt		// input, Btuh
						: 0.;
		[[maybe_unused]] double EhW = Eh / 3.413;		// input, W (check value)

		// low temp cut-out
		float delta = tDbBin <= tOff || rs_effHt < 1. ? 0.f
			        : tDbBin > tOn                    ? 1.f
			        :                                   0.5f;

		double eh = X * Eh * delta / PLF;
		double ehBin = eh * fHrReg4[ iBin];

		double RH = BL - X * delta * rs_capHt;		// backup
		double RHBin = RH * fHrReg4[ iBin];

		double inpBin = ehBin + RHBin;
		[[maybe_unused]] double inpBinWh = inpBin / 3.413;

		double outBin = BL * fHrReg4[ iBin];
		inpTot += inpBin;
		outTot += outBin;
		if (bDoPrint)
			DbPrintf( "%4.f %6.f %6.f %6.3f %6.f %6.f %6.f\n",
				tDbBin, outBin, rs_capHt, rs_effHt, ehBin, RHBin, inpBin);
	}

	double inpTotWh = inpTot/3.413;
	float HSPF = outTot / inpTotWh;

	if (bDoPrint)
		DbPrintf( "\noutTot=%0.f  inpTot=%0.f  inpTotWh=%0.f  HSPF=%0.3f\n",
			outTot, inpTot, inpTotWh, HSPF);

	rs_ASHPLockOutT = lockOutTSave;		// restore lockout input

	return HSPF;

}		// RSYS::rs_HSPFCheckASHP
//-------------------------------------------------------------------------------
/*static*/ float RSYS::rs_DHR(		// AHRI nominal DHR
	float capHt)		// heating capacity, Btuh
// rounds capacity per AHRI procedures, see AHRI 210/240 2008 section 4.2 ish
// used *only* for checking ratings etc (not in simulation)
// returns rounded DHR, Btuh
{
#if 1	// creative extension to handle small capacities, 10-16-2014
	float roCap = capHt <=   5000.f ?  1000.f
				: capHt <=  40000.f ?  5000.f
                : capHt <= 110000.f ? 10000.f
                :                     20000.f;
	float DHR = max( floor( 0.5f + capHt / roCap) * roCap, 1000.f);
#else
x	float roCap = capHt <=  40000.f ?  5000.f
x               : capHt <= 110000.f ? 10000.f
x               :                     20000.f;
x	float DHR = max( floor( 0.5f + capHt / roCap) * roCap, 5000.f);
#endif
	return DHR;
}		// float RSYS_rsDHR
//-------------------------------------------------------------------------------
/*static*/ double RSYS::rs_CallCalcCapHt( void* pO, double& cap47)
// callback function for secant()
//    resizes per cap47
//    calcs capacity (including any defrost) at rs_tdbOut
// returns capHt, Btuh
{
	RSYS* pRS = (RSYS *)pO;
	double capHt = pRS->rs_CapHtForCap47( cap47);
	return capHt;
}		// RSYS::rs_CallCalcHSPF
//-------------------------------------------------------------------------------
double RSYS::rs_CapHtForCap47(			// inner function re ASHP sizing
	float cap47)	// proposed cap47, Btuh
// note: resizes ASHP, alters many RSYS values
// returns total heating output at rs_tdbOut, Btuh
{
	rs_cap47 = cap47;
	rs_SetupCapH( -1.f, 2);	// set derived values from rs_cap47
							//   calls rs_SetupASHP() -> sets cap17 and cap35
							//   sets rs_amfH, rs_fanHeatH
	int ashpModel = 0;
	float speedF = rs_pPMACCESS[0] ? rs_pPMACCESS[ 0]->pa_GetSpeedF(PMSPEED::RATED) : 1.f;
	float capHt, inpHt, capDfHt;
	rs_PerfASHP2(ashpModel, rs_tdbOut, speedF, rs_fanHRtdH,
		capHt, inpHt, capDfHt);
	return capHt + rs_fanHeatH;
}		// RSYS::rs_CapHtForCap47
//-------------------------------------------------------------------------------
RC RSYS::rs_SizeHtASHP(			// size ASHP
	float dsnLoad,	// required output, Btuh
	float tdbOut)	// outdoor dry-bulb temp, F

// sets rs_cap47 (and dependent members) such that
//   heating capacity at tdbOut matches design load

{
	rs_tdbOut = tdbOut;
	RC rc = RCOK;

	// (crudely) estimated size (no defrost etc)
	float cap47Est = rs_EstimateCap47FromDsnLoad(dsnLoad, tdbOut);

	// refine / finalize cap47 using secant()
	//   fan power and defrost included
	double cap47 = cap47Est;
	double f1 = DBL_MIN;
	int ret = secant( rs_CallCalcCapHt, this, 
						dsnLoad, 20.f,	// target, tolerance
						cap47, f1,		// x1, f1
						cap47+100.);	// x2 (f2)
	if (ret != 0)
	{	rc = oer( "cap47 for design load fail (return code = %d)", ret);
		cap47 = cap47Est;
	}
	rs_cap47 = cap47;		// redundant?

	rs_DefaultCapNomsIf();	// update nominal capacities (no calc effect)

	return rc;

}	// RSYS::rs_SizeHtASHP
//------------------------------------------------------------------------------
float RSYS::rs_EstimateCap47FromDsnLoad(
	float dsnLoad,		// required output, Btuh
	float tdbOut) const // outdoor dry-bulb design temp, F
// returns *approximate* cap47, Btuh
{
	// estimated cap47 ignoring defrost and fan power
	//   use standard straight line fits solved for cap47
	float r17 = min( rs_CapRat1747(), 1.f);		// cap17/cap47
	float tF = (tdbOut - 17.f) / (47.f - 17.f);
	float d = (r17 + tF * (1.f - r17));
	float cap47Est = dsnLoad / d;

#if 0
// prior code that accounts for defrost
// unnecessary complication since result must be refined anyway
// save in case simplified version proves inadequate, 7-24
	float tdbM17 = tdbOut - 17.f;
	float tF;
	float d;
	if (rs_HasDefrost() && tdbOut > 17.f && tdbOut < 45.f)
	{	// defrost regime: cap based on 17 - 35 slope
		//   pkg ASHP does not defrost
		tF = tdbM17 / (35.f - 17.f);
		float r35 = 0.9f * (r17 + 0.6f*(1.f - r17));	// cap35/cap47
		d = (r17 + tF * (r35 - r17));
	}
	else
	{	// non-defrost: cap based on 17 - 47 slope
		tF = tdbM17 / (47.f - 17.f);
		d = (r17 + tF * (1.f - r17));
	}
	float cap47Est = dsnLoad / d;
#endif

	return cap47Est;

}		// RSYS::rs_EstimateCap47FromDsnLoad
//-------------------------------------------------------------------------------
void RSYS::rs_DefaultCapNomsIf()  // nominal capacities
// update nominal heating and cooling capacities
// sets rs_capNomH and rs_capNomC if not fixed by user input
// rs_capNomH and rs_capNomC are reporting only
{
// uses non-NaN values if available, else arbitrary default
// autosized rs_capXXX are initially NaN
// does NOT set FsVAL field status, tricky to know when values are final.

	if (!IsVal( RSYS_CAPNOMH))
		rs_capNomH = rs_IsASHP()
						? ifNotNaN( rs_cap47, 18000.f)
						: ifNotNaN( rs_capH, 18000.f);

	if (!IsVal( RSYS_CAPNOMC))
		rs_capNomC = ifNotNaN( rs_cap95, 18000.f);

}		// RSYS::rs_DefaultCapNomsIf()
//-------------------------------------------------------------------------------
void RSYS::rs_SetModeAndSpeedF(		// set mode / clear prior-step results
	int rsModeNew,				// new mode
	float speedF /*=1.f*/)		// new speed
{
	rs_mode = rsModeNew;
	rs_SetSpeedFMin();
	rs_speedF = speedF;
	if (ifBracket(rs_speedFMin, rs_speedF, 1.f))
		// speedF is invalid, report logic error
		orer("rs_SetModeAndSpeedF -- bad speedF (%0.3f) for mode %d",
			speedF, rs_mode);

}		// RSYS::rs_SetModeAndSpeedF
//-----------------------------------------------------------------------------
void RSYS::rs_SetSpeedFMin()		// determine current minimum speedF
// sets rs_speedFMin only
{
	if (rs_mode == rsmOFF)
		rs_speedFMin = 0.f;
	else if (!(rs_IsCHDHW() && rs_mode==rsmHEAT))
	{
		if (rs_IsVCMode(rs_mode))
			rs_speedFMin = rs_pPMACCESS[rs_mode == rsmCOOL]->pa_GetSpeedFMin();
		else
			rs_speedFMin = 1.f;
	}
	// else CHDHW heating: don't change

}	// RSYS::rs_SetSpeedFMin
//-----------------------------------------------------------------------------
float RSYS::rs_GetSpeedFRated(
	int iHC) const	// 0 = htg, 1=clg
{
	return rs_pPMACCESS[iHC] ? rs_pPMACCESS[iHC]->pa_GetSpeedF(PMSPEED::RATED) : 1.f;
}		// RSYS::rs_GetSpeedFRated
//-----------------------------------------------------------------------------
void RSYS::rs_ClearSubhrResults(
	int options /*= 0*/)	// option bits
							//  all 0: full clear (e.g. subhr beg)
							//  1: limited clear for speed retry
{
	rs_amf = 0.;
	rs_fanPwr = 0.f;
	rs_amfReq[0] = rs_amfReq[1] = 0.;
	rs_znLoad[0] = rs_znLoad[1] = 0.;
	rs_asRet.as_Init();
	rs_asIn.as_Init();
	rs_twbIn = 0.;
	rs_asOut.as_Init();
	rs_asSup.as_Init();
	rs_asOutAux.as_Init();
	rs_asSupAux.as_Init();

	if (options & 1)
		return;

	// all modes
#if defined( RSYSLOADF)
	rs_loadF = 
#endif
		rs_PLR = rs_runF = rs_speedF = rs_runFAux = rs_PLF = rs_capSenNetFS = 0.f;
	rs_outSen = rs_outLat = rs_outFan = rs_outAux = rs_outDefrost
		= rs_outSenTot = rs_inPrimary = rs_inFan = rs_inAux = rs_inDefrost = 0.;

	// heating
	rs_effHt = rs_COPHtAdj = rs_tCoilEW = 0.f;
	rs_capHtFS = rs_capHt = rs_capDfHt = 0.f;

	// cooling
	rs_tdbCoilIn = 0.f;
	rs_twbCoilIn = 0.f;
	rs_SHR = 0.f;
	rs_fCondCap = rs_fCondInp = 0.f;
	rs_capTotCt = rs_capSenCt = rs_capLatCt = rs_inpCt = 0.f;
	rs_fCondSEER = rs_fCondEER = 0.f;
	rs_SEERnf = rs_EERnf = rs_EERt = rs_effCt = 0.f;
	
}		// RSYS::rs_ClearSubhrResults
//----------------------------------------------------------------------------
int RSYS::rs_IsModeAvailable(		// mode availability
	int rsMode)	const	// desired mode (rsmHEAT, rsmCOOL, rsmOAV)

// returns 1 iff RSYS can operate in desired mode
//         0 if not (mode fixed and different)
//        -1 if not (system off)
{
	int ret;
	if (rs_mode != rsmOFF)
		ret = rs_mode == rsMode;	// RSYS mode already known
									// OK iff desired mode is same
	else
	{
		int modeCtrl = CHN(rs_modeCtrl);	// retrieve choicn
							// choicn used to allow expressions for rsModeCtrl
		switch (modeCtrl)
		{
		case C_RSYSMODECTRL_HEAT:
			ret = rsMode == rsmHEAT && rs_CanHeat();
			break;

		case C_RSYSMODECTRL_COOL:
			ret = rsMode == rsmCOOL && rs_CanCool();
			break;

		case C_RSYSMODECTRL_AUTO:
			// control is auto and system is currently off
			ret = (rsMode == rsmHEAT && rs_CanHeat())
				|| (rsMode == rsmCOOL && rs_CanCool())
				|| (rsMode == rsmOAV && rs_CanOAV());
			break;

		case C_RSYSMODECTRL_OFF:
		default:
			ret = -1;		// off: no can do
		}
	}

	return ret;
}		// RSYS::rs_IsModeAvailable
//----------------------------------------------------------------------------
void RSYS::rs_EnteringAirState()	// RSYS entering air state

// rs_mode s/b set by caller (and not rsmOFF)
// rs_amf s/b set by caller and > 0

// rsMode determines assumed air source
//   rsmOAV: outdoor air (as modified via rs_OAVTdbInlet)
//   else: mixes air from zones plus return duct leakage
// then adds return duct conduction and leakage

// returns
//  rs_asIn = state of air entering RSYS (after return ducts)
//  rs_rhIn = corresponding relative humidity (0 - 1)
//  rs_twbIn = corresponding wet bulb temp, F
{

	if (rs_mode == rsmOAV)
		// outdoor air vent: set state from possibly modified outdoor conditions
		//   ignore possibility of impossible humidity ratio
		rs_asRet.as_Set( rs_OAVTdbInlet, Top.wOSh);

	else
	{	ZNR* zp;
		AIRFLOW afRet;		// air flow at return duct inlet
							//   magically mixed from all zones
							//     served by this system
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
		{	// determine return air state
			//   mix zone states weighted by prior flow
			//   use area as proxy if prior flow not known
			double amfX = rs_mode == rs_modeLs
							? zp->zn_rsAmfRetLs
							: zp->i.znArea;
			// return grille temp = zone air temp from last step
			afRet.af_AccumDry( amfX, zp->tzls, zp->wzls);
#if defined( _DEBUG)
			if ((afRet.as_tdb < 30. || afRet.as_tdb > 110.f) && !rs_IsAutoSizing())
				orWarn("dubious return temp (%.1f F)", afRet.as_tdb);
#endif
		}
		rs_asRet.as_Set( afRet);		// state = flow w/o amf
	}

	// adjust state for return duct
	int iDS = rs_Dsi( 1);	// idx of ductseg
	rs_asIn = iDS > 0 && rs_amf > 0.
				? DsR[ iDS].ds_CalcFL( rs_asRet, rs_amf)
				: rs_asRet;


#if defined( _DEBUG)
	if (rs_mode != rsmOAV
	 && (rs_asIn.as_tdb < 30. || rs_asIn.as_tdb > 110.f))
	{	if (!Top.isWarmup)
			orWarn( "Dubious asIn tdb (%0.2f)", rs_asIn.as_tdb);
		// repeat return duct calc (debug aid)
		// int iDS = rs_Dsi( 1); above
		if (iDS > 0)
			rs_asIn = DsR[ iDS].ds_CalcFL( rs_asRet, rs_amf);
	}
#endif

	if (IsSet( RSYS_RHINTEST))
	{	rs_asIn.as_SetWFromRh( rs_rhInTest);	// testing aid: set w from rh (tdb unchanged)
		rs_rhIn = rs_rhInTest;
	}
	else
		rs_rhIn = rs_asIn.as_RelHum();		// else calc rh
	rs_twbIn = rs_asIn.as_Twb();

}	// RSYS::rs_EnteringAirState
//----------------------------------------------------------------------------
int RSYS::rs_SupplyAirState(		// current conditioning capabilities
	int rsMode,		// desired mode (do not call with rsMode == rsmOFF)
	float speedF /*=1.f*/)		// speed fraction
								//   sets rs_speedF to max( speedF, min
// NOTE: many side effects -- changes rs_mode, clears prior step, ...

//	returns 3: mode is available, rs_asSup set
//			2: mode is being actively autosized, rs_asSup set
//			1: mode is inactive (another mode is being actively autosized)
//			0: mode not available
{
	// TODO: not returning right value!

	float speedFWas = rs_speedF;

	// mode availability
	rs_amf = 0.;
	if (rs_IsModeAvailable(rsMode) <= 0)
		return 0;

	int rsAvail = rs_SetAirFlowAndFanPwr(rsMode, speedF);
	if (rsAvail < 2)
		return rsAvail;

	if (rs_mode == rsmOFF || speedF != speedFWas)		// if was off or speed has changed
	{	// first zone requesting this mode during this step
		// speedF?  rs_speedF and rs_amf can be inconsistent?
		rs_SetModeAndSpeedF(rsMode, speedF);

		// determine entering air state
		//   does return duct calcs, uses rs_amf
		rs_EnteringAirState();

		rs_asOut = rs_asIn;		// init entering state

		int auszMode = rs_IsAutoSizing();
		if (rs_mode == rsmCOOL)
		{	++rs_calcCount[1];
			rs_CoolingOutletAirState(auszMode);
		}
		else if (rs_mode == rsmHEAT)
		{	++rs_calcCount[0];
			// rs_asOut = rs_asIn;		// above
			rs_HeatingOutletAirState( auszMode);
		}
		else
		{	// OAV: add fan heat
			// rs_asOut = rs_asIn;		// above
			rs_asOut.as_AddQSen2( rs_fanHeatOAV, rs_amf);
		}

		// outlet air state rs_asOut now known for all cases
		// calc supply air state at zone(s)
		rs_asSup = rs_asOut;
		rs_SupplyDSEAndDucts(rs_asSup);

	}
	// else supply air state known

	return rsAvail;
}	// RSYS::rs_SupplyAirState
//----------------------------------------------------------------------------
float RSYS::rs_GetAMFFullSpeed( int rsMode) const
{
	float amf = 0.f;
	switch (rsMode)
	{
		case rsmHEAT: amf = rs_amfH; break;
		case rsmCOOL: amf = rs_amfC; break;
		case rsmOAV:  amf = rs_amfOAV; break;
		default: break;
	}
	return amf;
}		// RSYS::rs_GetAMFFullSpeed
//------------------------------------------------------------------------------
int RSYS::rs_SetAirFlowAndFanPwr(		// set (or not) rs_amf and rs_fanHeat
	int rsMode,		// mode in play
	float speedF)
	//	returns 3: mode is available, rs_amf set per speedF
	//			2: mode is being actively autosized, rs_amf = full speed
	//			1: mode is inactive
	//             another mode is being actively autosized, rs_amf unchanged
	//			0: no amf available, rs_amf = 0
{
	// set mode-specific full speed air flow
	// must be set even if mode not changed
	//   due to possible speedF modifications
	float amfFS = rs_GetAMFFullSpeed(rsMode);
	if (amfFS < .0001f)
	{	rs_amf = 0.;
		return 0;
	}

	int rsAvail = 3;
	int auszMode = rs_IsAutoSizing();
	if (auszMode != rsmOFF)
	{	// autosizing underway
		if (auszMode != rsMode)
			return 1;	// another mode is being autosized (rs_amf unchanged)
		rsAvail = 2; 	// current mode now being autosized
	}
	// else rsAvail = 3;

	rs_amf = rs_AMFOperating(rsMode, amfFS, speedF);

	rs_fanPwr = rsMode == rsmOAV ? rs_OAVFanSFP
		          : rs_FanPwrOperatingAtSpeedF(rsMode != 1, speedF);

	return rsAvail;

}		// RSYS::rs_SetAirFlowAndFanPwr
//----------------------------------------------------------------------------
float RSYS::rs_SupplyDSEAndDucts(	// apply supply duct and DSE losses
	AIRSTATE& as,	// air state
					//    call: state at RSYS outlet
					//    return: state at room register
	float amf /*=-1*/)	// dry air mass flow rate, lbm/hr
						// default: rs_amf

// returns room register air mass flow rate, lbm/hr (after leakage if any)
{
	if (amf < 0.f)
		amf = rs_amf;

	// DSE: discard (1-DSE) of heat/moisture added by system
	int iHC = rs_DsHC();		// returns nz iff cooling
	float DSE = iHC ? rs_DSEC : rs_DSEH;
	if (DSE > 0.f)
	{	if (DSE < 1.f)
			as.as_MixF( 1.f - DSE, rs_asIn);
		// else leave as unchanged (no duct losses)
	}
	else
	{	// supply duct (if any) to get supply state
		int iDS = rs_Dsi( 0, iHC);	// retrieve supply duct idx
		if (iDS > 0)				// if supply duct present
		{	as = DsR[iDS].ds_CalcFL(as, amf);
			amf *= (1.f - DsR[iDS].ds_leakF);
		}
	}
	return amf;
}		// RSYS_SupplyDSEAndDucts
//----------------------------------------------------------------------------
void RSYS::rs_SupplyDSEAndDuctsRev(	// reverse supply duct and DSE losses
	AIRSTATE& as)	// air state
					//    call: state at room register
					//    return: state at RSYS outlet
{
	int iHC = rs_DsHC();		// returns nz iff cooling
	float DSE = iHC ? rs_DSEC : rs_DSEH;
	if (DSE > 0.f)
	{	// distribution system efficiency
		//   1-DSE is discarded
		if (DSE < 1.f)
			as.as_MixF( -(1.f - DSE)/DSE, rs_asIn);
		// else leave as unchanged (no duct losses)
	}
	else
	{	// supply duct (if any)
		int iDS = rs_Dsi( 0);
		if (iDS > 0)		// if supply duct present
		{	AIRSTATE asSup( as);
			as = DsR[ iDS].ds_CalcFLRev( asSup);
		}
	}
}		// RSYS_SupplyDSEAndDuctsRev
//----------------------------------------------------------------------------
double RSYS::rs_ZoneAirRequest(		// air quantity needed by zone
	double znSupReq,	// dry-air mass flow rate required at supply register, lbm/hr
	int iAux /*=0*/)	// 0 = primary mode (compressor)
						// 1 = aux mode (aux alone or main+aux (ASHP heating only)
// each zone needing conditioning requests air
// returns RSYS (not supply register) amf needed to provide zone requirement
{
	// handle impossible requests
	//   supplyDT = tz - tSup
	if (znSupReq > 1.e10)	// znSupReq = DBL_MAX if supplyDT is tiny
		znSupReq = rs_amf;
	else if (znSupReq < 0.)
		// reverse flow
		//   caused by "flipped" supply DT
		//   due to e.g. big duct losses
		znSupReq =
#if 1	// attempt to fix small load sizing, 7-16-2021
			2. * rs_amf;		// request "a lot"
#else
#if 1	// unknown date
			Top.tp_autoSizing
#else
			Top.tp_pass1A
#endif
					? 1.				// autosize warmup: ignore
					: 2. * rs_amf;		// simulation: request "a lot"
#endif

	// air required at system
	double znAmfSys = znSupReq / rs_ducts[ rs_DsHC()].ductLkXF[ 0];
	rs_amfReq[ iAux] += znAmfSys;		// all-zone total at system

	// sensible load at system, Btuh
	rs_znLoad[ iAux] += 
		(iAux ? rs_asOutAux : rs_asOut).as_QSenDiff2(rs_asIn, znAmfSys);

#if 0 && defined( _DEBUG)
	if (iAux == 1)
	{
		const char* msg = fabs(rs_znLoad[0] - rs_znLoad[1]) > 1.f
			? "rs_znLoad mismatch"
			: "rs_znLoad OK";
		printf("\n%s   rs_amf=%0.1f   rs_amfReq[ 0]=%0.1f  rs_amfReg[ 1]=%0.1f",
			msg, rs_amf, rs_amfReq[0], rs_amfReq[1]);
	}
#if 0
	if (rs_amfReq[ iAux] > 200000. && !Top.isWarmup)
		printf( "rs_ZoneAirRequest: Excessive AMF=%0.f  aux=%d\n",
			rs_amfReq[ iAux], iAux);
#endif
#endif
	return znAmfSys;
}		// RSYS::rs_ZoneAirRequest
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
#define RSYSITERCOUNT
#if defined( RSYSITERCOUNT)
// dev aid statistic re minimizing aux heating iterations
int rsysPartAuxCount = 0;	// # of part-load auxiliary substeps
int rsysIterCount = 0;		// count secant method call-backs
#endif
#endif
//-----------------------------------------------------------------------------
RC RSYS::rs_AllocateZoneAir()	// finalize zone air flows
// 
{
	if (rs_mode == rsmOAV)
		return RCOK;			// OAV: allocation done by rs_OAVAttempt

#if defined( _DEBUG)
	if (rs_speedF < 0.f || rs_speedF > 1.f)
		printf("\nrs_speedF = %0.f", rs_speedF);
#endif
	RC rc = RCOK;

	ASSERT(rs_fxCap[0] == 0.f && rs_fxCap[1] == 0.f);
	if (rs_amfReq[ 0] > 0.)						// if any air requested (by any zone)
		rs_fxCap[ 0] = rs_amf / rs_amfReq[ 0];	// >1 = excess capacity
	// else rs_fxCap = 0
	double fSize = min( rs_fxCap[ 0], 1.);		// limit to available

	// bAux: aux possible and needed
	//   note: rs_capAuxH>0 possible for ASHP *only* (but can be =0 for ASHP)
	bool bAux = rs_mode == rsmHEAT && rs_capAuxH > 0.f
		     && (rs_effHt == 0.f || (fSize > 0. && fSize < 1.));

	if (bAux && rs_ctrlAuxH != C_AUXHEATCTRL_CYCLE)
	{	// check that aux will be helpful
		//   _CYCLE: any rs_capAuxH > 0 helps (> 0.f check is above)
		//   _LO, _ALT: aux supply temp must be greater than primary
		if (rs_asSupAux.as_tdb <= rs_asSup.as_tdb)
		{	orWarn("Aux heat supply temperature (%0.1f F) <= primary supply temperature (%0.1f F)."
			   "\n    Aux heat is not helpful.  rscapAuxH should be increased.",
				rs_asSupAux.as_tdb, rs_asSup.as_tdb);
			// bAux = false;  no: real controls would run aux even if not helpful
		}
	}

	ZNR* zp;
	if (!bAux)
	{	// aux not available or not needed
		if (rs_IsVCMode( rs_mode) && fSize == 1. && !rs_IsAutoSizing())
		{	// variable capacity with excess capacity
			// find intermediate speed
#if 0
			float tSupUseful;
			bool bUseful = rs_IsSupplyAirTempUseful(rs_mode, rs_asSup.as_tdb, tSupUseful);
			if (!bUseful)
				printf("\nNot useful");
#endif
			rc |= rs_FindRequiredSpeedF();
			// now rs_speedFMin <= rs_speedF < 1
			fSize = min(rs_fxCap[0], 1.);
		}
		RLUPC(ZrB, zp, rs_IsZoneServed(zp))
			zp->zn_SetRSYSAmf(fSize, 0);
	}
	else if (rs_amfReq[ 1] > rs_amf)
	{	// full aux insufficient
		//  allocate shortfall across zones
		rs_fxCap[ 1] = rs_amf / rs_amfReq[ 1];	// x/0 impossible

		// finalize supply conditions
		//  recalc duct loss
		//  WHY: duct temps must be current re surrounding zone xfer
		rs_asSup = rs_asOut = rs_asOutAux;
		rs_SupplyDSEAndDucts( rs_asSup);
#if defined( _DEBUG)
		// result s/b same as initial calc!
		if (rs_asSup != rs_asSupAux)
			printf( "rs_asSupAux mismatch!\n");
#endif
		rs_runFAux = 1.;	// aux is at full cap
		if (rs_ctrlAuxH == C_AUXHEATCTRL_LO)
			rs_effHt = 0.f;		// force compressor off
		// rs_speedF = 0.;	// dicey: fan is at full speed
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
			zp->zn_SetRSYSAmf( rs_fxCap[ 1], 1);	// tell zones how much they get
	}
	else if (rs_effHt == 0.f || rs_ctrlAuxH == C_AUXHEATCTRL_LO)
	{	// compressor is unavailable
		// meet load with aux alone
#if 0 && defined( _DEBUG)
		if (Top.jDay == 336)
			printf("\nHit %s", Top.dateStr.CStr());
#endif
		rs_effHt = 0.f;		// force compressor off (for _LO case)
		rs_fxCap[ 1] = rs_amf / rs_amfReq[ 1];	// x/0 impossible
		rs_runFAux = 1.f / rs_fxCap[ 1];
#if defined( _DEBUG)
		if (ifBracket( 0.f, rs_runFAux, 1.f))
			printf( "rs_AllocateZoneAir: rs_runFAux > 1\n");
#endif

		// finalize supply conditions
		//  recalc duct loss
		//  WHY: duct temps must be current re surrounding zone xfer
		rs_asSup = rs_asOut = rs_asOutAux;
		rs_SupplyDSEAndDucts( rs_asSup);
#if defined( _DEBUG)
		// result s/b same as initial calc!
		if (rs_asSup != rs_asSupAux)
			printf( "rs_asSupAux mismatch!\n");
#endif
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
			zp->zn_SetRSYSAmfFromTSup();
	}
	else
	{	// partial aux required
		//   C_AUXHEATCTRL_CYCLE: full compressor + cycling aux
		//   C_AUXHEATCTRL_ALT: compressor / aux alternate
		//   find supply temp that satisfies all zones at full volume
		//   Use secant method to find inverse of 1/amf = f( tSup)
		//   1/amf is linear-ish with tSup, reduces iterations
#if defined( RSYSITERCOUNT)
		rsysPartAuxCount++;
#endif
#if 0 && defined( _DEBUG)
		if (Top.jDay == 336)
			printf("\nHit %s", Top.dateStr.CStr());
#endif
		double tSup = max(rs_asSup.as_tdb, rs_tSupLs);	// use last result as guess
		double amfX = DBL_MIN;
		double amfXTarg = 1. / rs_amf;		// f = target function value
		int ret = secant(
			[](void* pO, double& tSup)
			{ 	// returns 1/reqAMF, 1.e10 iff reqAMF = 0
#if defined( RSYSITERCOUNT)
				rsysIterCount++;
#endif
				double amf = ((RSYS*)pO)->rs_AmfRequired(tSup);
				return amf != 0. ? 1. / amf : 1.e10;
		    },
			this, amfXTarg, .0001*amfXTarg,
			tSup, amfX,				// x1, f1
			rs_asSupAux.as_tdb, DBL_MIN);	// x2, f2
		if (ret != 0) {
                  tSup = rs_asSup.as_tdb;
		  oWarn("Failed to solve for ASHP aux heat supply temperature to deliver %g lb/hr.\n"
                        "    Resuming with previous value of tSup=%.2f.", 1. / amfXTarg, tSup);
		}
#if defined( _DEBUG)
		// check tSup -- should be between noAux and fullAux temps
		double tSupX = bracket(rs_asSup.as_tdb, tSup, rs_asSupAux.as_tdb);
		if (tSupX != tSup)
		{
			printf("rs_AllocateZoneAir: tSup out of range\n");
			tSup = tSupX;
		}
#endif
		AIRSTATE asOutNoAux(rs_asOut);
		rs_asSup.as_tdb = tSup;		// required supply air state
		rs_asOut = rs_asSup;
		rs_SupplyDSEAndDuctsRev(rs_asOut);

#if defined( _DEBUG)
		// reverse calc, should match supply
		AIRSTATE asSupX(rs_asOut);
		rs_SupplyDSEAndDucts(asSupX);
		if (asSupX != rs_asSup)
			printf("rs_AllocateZoneAir: inconsistent air state\n");
#endif
		double qAux;
		if (rs_ctrlAuxH == C_AUXHEATCTRL_ALT)
		{	// aux alternates with primary
			rs_runFAux = (rs_asOut.as_tdb - asOutNoAux.as_tdb) / (rs_asOutAux.as_tdb - asOutNoAux.as_tdb);
#if defined( _DEBUG)
			if (rs_runFAux < 0.f)
				printf("\nrs_runFAux < 0");
			if (rs_runFAux > 1.f)
				printf("\nrs_runFAux > 1");
#endif
			float fFan = rs_capAuxH / (rs_capAuxH + rs_fanHeatH);
			qAux = rs_runFAux * rs_capAuxH * fFan;
		}
		else
		{	// aux cycles: determine added heat rqd in addition to prim
			qAux = rs_asOut.as_QSenDiff2(asOutNoAux, rs_amf);

			if (qAux < 0.)
			{
#if defined( _DEBUG)
				if (qAux < -.1)
					printf("rs_AllocateZoneAir: qAux < 0 (%.2f)\n", qAux);
#endif
				qAux = 0.;
			}
			rs_runFAux = qAux / rs_capAuxH;
		}
#if defined( _DEBUG)
		if (ifBracket( 0.f, rs_runFAux, 1.f))
			printf( "rs_AllocateZoneAir: rs_runFAux > 1\n");
#endif
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
			zp->zn_SetRSYSAmfFromTSup();
	}	// end partial aux

	return rc;
}	// RSYS::rs_AllocateZoneAir
//----------------------------------------------------------------------------
#if 0 && defined( _DEBUG)
#define TRACEFINDSPEEDF
#endif
//----------------------------------------------------------------------------
RC RSYS::rs_FindRequiredSpeedF()	// find VC intermediate speed
// at call: RSYS in full speed state (excess capacity, rs_fxCap[ 0]==1)
// returns RCBAD iff search fail (unexpected)
//    else RCOK and RSYS at intermediate speed state
//         if rs_fxCap[ 0] == 1, pegged at rs_speedFMin (must cycle below min)
{
#if defined( _DEBUG)
	if (rs_fxCap[0] < 1.f)
		printf("\nrs_AllocateZoneAirVC: rs_fxCap[ 0] < 1");
#endif
	float fxCapMaxSpd = rs_fxCap[0];		// save rs_speedF=1 (max speed) fxCap
	RC rc = RCOK;

	assert(!rs_IsAutoSizing());

	// try minimum speed
	rc |= rs_TotalAirRequestForSpeedF( rs_speedFMin);

	// note: rs_speedF > rs_speedFMin is possible here
	//  rs_TotalAirRequestForSpeedF() increases speed to avoid useless supply temp

	if (rc || rs_fxCap[0] >= 1.f)
		return rc;		// error or rs_speedFMin is sufficient

	// rs_speedFMin not sufficient, search for intermediate speed

	// initial guess based on min/max fxCap
	//   TODO: could be improved cuz fxCap not necessary linear with speed
	float fxCapMinSpd = rs_fxCap[0];

	float speedFEst = rs_speedF + (1.f - rs_speedF) * (1.f - fxCapMinSpd) / (fxCapMaxSpd - fxCapMinSpd);

	double speedF = min(1., speedFEst * 1.2);

#if defined( TRACEFINDSPEEDF)
	printf("\nStart speedFMin=%0.4f  fxCapMinSpd=%0.4f  fxCapFullSpd=%0.4f  speedFEst=%0.4f",
		rs_speedFMin, fxCapMinSpd, fxCapFullSpd, speedFEst);
#endif

	// search for speedF that produces fxCap = 1.
	//   this RSYS is left in the corresponding state via rc_TotalAirRequestForSpeed() calls
	//   (secant results not directly returned)
	int ret = regula([](void* pO, double& speedF) { return ((RSYS*)pO)->rs_FxCapForSpeedF(speedF); },
						this,
						1., .0005,	// target=rs_fxCap[0]=1 within .0005
						speedF,
						rs_speedFMin, 1.);
	if (ret)	// if regual failed (unexpected)
		// leave RSYS is speedFEst state (= semi-decent result)
		rc |= rs_TotalAirRequestForSpeedF(speedF/*Est*/);

#if defined( TRACEFINDSPEEDF)
	if (ret)
		printf("\nrs_FindRequiredSpeedF: regula ret=%d  rsMode=%d", ret, rs_mode);
	if (rs_speedF < rs_speedFMin || rs_speedF > 1.f)
		printf("\nrs_FindRequiredSpeedF bad rs_speedF=%0.2f", rs_speedF);
	printf("\nDone  speedF=%0.4f  fxCap=%0.4f", rs_speedF, rs_fxCap[0]);
#endif

	return rc;

}	// RSYS::rs_FindRequiredSpeedF()
//-----------------------------------------------------------------------------
bool RSYS::rs_IsSupplyAirTempUseful(	// determine utility of supply air
	int rsMode,		// mode of interest
	float tSup,		// proposed supply temp
	float& tSupUseful) const	// returned: temp that would be useful

// returns true iff proposed supply air temp

// sets rs_TSupLimit
{
	bool bRet = false;
	float tspMaxH = -999.f;
	float tspMinC = 999.f;
	ZNR* zp;
	RLUPC(ZrB, zp, rs_IsZoneServed(zp))
	{	
		setToMax(tspMaxH, zp->zn_tzspH);
		setToMin(tspMinC, zp->zn_tzspC);
	}
	if (rsMode == rsmHEAT)
	{
		tSupUseful = tspMaxH + .1f;
		bRet = tSup >= tSupUseful;
	}
	else
	{
		tSupUseful = tspMinC - .1f;
		bRet = tSup <= tSupUseful;
	}
	return bRet;
}		// RSYS::rs_IsSupplyAirTempUseful
//-----------------------------------------------------------------------------
RC RSYS::rs_TotalAirRequestForSpeedF(		// all-zone air request at speedF
	float speedF,			// speed to attempt
	int options /*=0*/)		// option bits
							//   1: report zn_AirRequest errors
// returns RCOK iff valid result (rs_fxCap[ 0] set)
//    else RCBAD, rs_fxCap[ 0] = 0
{
	RC rc = RCOK;

	if (speedF != rs_speedF)
	{
		rs_ClearSubhrResults(1);	// init for speed re-try
		/* int ret = */ rs_SupplyAirState(rs_mode, speedF);

		float tSupUseful;
		if (!rs_IsSupplyAirTempUseful(rs_mode, rs_asSup.as_tdb, tSupUseful))
		{	// adjust rs_speedF to produce useful supply air temp
			rs_FindSpeedFForTSup(tSupUseful);
			// printf("\nSpeedF = %0.3f", rs_speedF);

		}
		ZNR* zp;
		RLUPC(ZrB, zp, rs_IsZoneServed(zp))
			rc |= zp->zn_AirRequest(this, options);

		if (rc || rs_amfReq[0] <= 0.)
		{
#if defined( _DEBUG)
			printf("\nrs_TotalAirRequestForSpeed: rs_amfReq[ 0] <= 0.");
#endif
			rs_fxCap[0] = 0.f;
			rc = RCBAD;
		}
		else
			rs_fxCap[0] = rs_amf / rs_amfReq[0];	// >1 = excess capacity
	}

	return rc;
}		// RSYS::rs_TotalAirRequestForSpeedF
//-----------------------------------------------------------------------------
double RSYS::rs_FxCapForSpeedF(		// call-back fcn for regula method
	double& speedF)		// trial speedF (may be returned modified)
// returns required rs_fxCap[ 0] (= rs_amf / amfReq), >1 = excess capacity
{
#if defined( _DEBUG)
	double speedFWas = speedF;
	float rsSpeedFWas = rs_speedF;
#endif
	if (speedF < FLT_MIN)
		speedF = 0.;
	else if (speedF > 1.)
		speedF = 1.;
#if defined( _DEBUG)
	if (speedF != speedFWas)
		printf("\nrs_FxCapForSpeedF limit: speedFWas=%0.6g  speedF=%0.4f",
			speedFWas, speedF);
#endif

	// options: report bad supply temps iff near solution
	//   (when far from solution errors are common and not meaningful
	int arOptions = abs(rs_fxCap[0] - 1.f) < .01f;

	RC rc = rs_TotalAirRequestForSpeedF(float(speedF), arOptions);

	// if rc != RCOK, rs_fxCap[0] is 0

#if defined( TRACEFINDSPEEDF)
	static double speedFLastCall = 0.;
	static float fxLastCall = 0.;
	bool bDup = speedF != speedFLastCall && rs_fxCap[0] == fxLastCall;
	printf("\n    speedF=%0.4f  f=%0.4f tSup=%0.3f rs_amf=%0.3f rs_amfReq=%0.3f (speedFLastCall=%0.4f rsSpeedFWas=%0.4f)%s",
			speedF, rs_fxCap[0], rs_asSup.as_tdb, rs_amf, rs_amfReq[ 0], speedFLastCall, rsSpeedFWas, bDup ? " Dup" : "");
	if (bDup)
	{ 
		rs_speedF = rsSpeedFWas;
		rs_TotalAirRequestForSpeedF(float(speedF), arOptions);
	}

	if (rc)
		printf("\nrs_FxCapForSpeedF: rs_TotalAirRequestForSpeedF() fail");
	speedFLastCall = speedF;
	fxLastCall = rs_fxCap[0];
#else
	// ignore rc, return rs_fxCap[ 0] = 0
#endif

	return double(rs_fxCap[0]);
}	// RSYS::rs_FxCapForSpeedF
//-----------------------------------------------------------------------------
RC RSYS::rs_FindSpeedFForTSup(		// find speedF that will deliver specified supply temp
	double tSup)
// returns with rs_speedF set to speedF found
{
	RC rc = RCOK;

	double speedF = 0.5;

	// search for speedF that produces fxCap = 1.
	//   this RSYS is left in the corresponding state via rc_TotalAirRequestForSpeed() calls
	//   (regula results not directly returned)
	int ret = regula([](void* pO, double& speedF) { return ((RSYS*)pO)->rs_TSupForSpeedF(speedF); },
						this,
						tSup, .005,	// target=tSup within .01 degF
						speedF,
						rs_speedFMin, 1.);
	if (ret != 0)
		printf("\nrs_FindSpeedFForTSup: ret=%d  tSup=%0.2f  speedF=%0.3f", ret, tSup, speedF);

	// rs_speedF set in called rs_SupplyAirState
	return rc;

}		// RSYS::rs_FindSpeedFForTSup
//-----------------------------------------------------------------------------
double RSYS::rs_TSupForSpeedF(
	double& speedF)
{
#if defined( _DEBUG)
	double speedFWas = speedF;
	// float rsSpeedFWas = rs_speedF;
#endif
	if (speedF < rs_speedFMin)
		speedF = rs_speedFMin;		// insurance
	else if (speedF > 1.)
		speedF = 1.;
#if defined( _DEBUG)
	if (speedF != speedFWas)
		printf("\nrs_TSupForSpeedF limit: speedFWas=%0.6g  speedF=%0.4f",
			speedFWas, speedF);
#endif

	// rs_ClearSubhrResults(1);	// init for speed re-try
	/*int ret = */ rs_SupplyAirState(rs_mode, speedF);

	return rs_asSup.as_tdb;

}		// RSYS::rs_TSupForSpeedF
//-----------------------------------------------------------------------------
#if 0	// incomplete/unused ideas
void RSYS::rs_AvgZoneCond()
{
	const ZNR* zp;
	AIRFLOW asZn;
	RLUPC(ZrB, zp, rs_IsZoneServed(zp))
	{	asZn.af_Mix(zp->zn_sysAirO);
	}
	rs_asZn = ??
}	// RSYS::rs_AvgZoneCond
//----------------------------------------------------------------------------
float RSYS::rs_SenHeatDelivered()
{
	float qSenTot = 0.f;
	const ZNR* zp;
	RLUPC(ZrB, zp, rs_IsZoneServed(zp))
	{
		qSenTot += zp->zn_qsHvac;
	}
	return qSenTot * Top.tp_subhrDur;
}	// RSYS::rs_SenHeatDelivered
//----------------------------------------------------------------------------
void RSYS::rs_HeatDelivered(	// heat delivered to zone(s) by this RSYS
	float& qhSen,	// heating sensible, Btu (>= 0)
	float& qcSen,	// cooling sensible, Btu (< 0)
	float& qcLat)	// cooling latent, Btu
					//   (generally <0, can be >0 due to duct leakage
{
	qhSen = qcSen = qcLat = 0.f;
	const ZNR* zp;
	RLUPC(ZrB, zp, rs_IsZoneServed(zp))
	{
		if (zp->zn_qsHvac >= 0.)
			qhSen += float(zp->zn_qsHvac);
			// ignore latent if no sensible cooling
		else
		{	qcSen += float(zp->zn_qsHvac);
			qcLat += float(zp->zn_qlHvac);
		}
	}
	qhSen *= Top.tp_subhrDur;
	qcSen *= Top.tp_subhrDur;
	qcLat *= Top.tp_subhrDur;
}	// RSYS::rs_SenHeatDelivered
#endif
//----------------------------------------------------------------------------
double RSYS::rs_AmfRequired(		// find all-zone total AMF required for tSup
	double tSup)		// proposed supply air temp, F
{
	double rsAmf = 0.;
	ZNR* zp;
	RLUPC( ZrB, zp, rs_IsZoneServed( zp) && zp->zn_hcMode != RSYS::rsmOFF)
	{	double znAmf = zp->zn_AmfHvacCR( zp->zn_tzsp, tSup);
#if defined( _DEBUG)
		if (znAmf < 0.)
			printf( "Zone '%s': Neg amf\n", zp->Name());
#endif
		rsAmf += znAmf;
	}
	float ductLkXF = rs_ducts[rs_DsHC()].ductLkXF[0];
	return rsAmf / ductLkXF;
}		// RSYS::rs_AmfRequired
//----------------------------------------------------------------------------
#if 0
x unused idea
x float RSYS::rs_ACReqCap95(		// required rated capacity to meet sensible load
x	float senLoad)	// sensible load, Btuh
x // returns required total capacity required to meet load, Btuh
x //         (adjusted to AHRI rating conditions)
x {
x   worry about x/0
x   is rs_fChg handled correctly?
x	float cap95 = senLoad / (rs_fChg * rs_SHR * rs_fCondCap);
x	float fanHRtdRat = rs_fanHRtdC / cap95;		// rated fan heat ratio, (Btuh fan)/(Btuh coil)
x	float capAdjF = rs_fChg * (1.f - fanHRtdRat);
x	if (rs_capAdjF > 0.)
x		cap95 /= capAdjF;
x	return cap95;
x
x }		// RSYS::rs_ACReqCap95
#endif
//---------------------------------------------------------------------------
RC RSYS::rs_FinalizeSh()
// rs_speedF known
{
	// determine run fraction
	//  Note: rs_runF modified below for ASHP C_AUXHEATCTRL_ALT
	double amfRun = min( rs_amfReq[ 0], rs_amf);
	if (amfRun < .0001)
		rs_mode = rsmOFF;		// no air actually delivered
	if (rs_mode == rsmOFF)
		rs_SetModeAndSpeedF( rsmOFF, 0.f);
		// rs_runF = 0.f;		// rs_ClearSubhrResults
	else if (rs_amf > 0.)
		// rs_SetModeAndSpeedF() called in rs_SupplyAirState
		rs_runF = amfRun / rs_amf;
	// else rs_runF = 0.f

#if 0
	// restore if needed
	rs_AvgZoneCond();
#endif

	// record peaks or calc energy
	//   Probably unnecessary for autosizing
	//   TODO: could skip accounting if autoSizing
	//       BUT may be side effects
	float runFFan = 0.f;		// subhour fan run fraction
	if (rs_mode == rsmCOOL)
	{
		rs_outSen = rs_runF * rs_capSenCt;		// average output w/o fan (= gross cooling), Btuh (< 0)
		rs_outLat = rs_runF * rs_capLatCt;
		rs_outFan = rs_inFan = rs_runF * rs_fanPwr;	// all fan heat to air
		runFFan = rs_runF;
		rs_outSenTot = rs_outSen + rs_outFan;	// net cooling output, Btuh (generally < 0)

		if (rs_IsFanCoil())
		{	// rs_inPrimary = 0
		}
		else
		{
#if defined(_DEBUG)
			if (fabs(rs_runF - 1.f) > 0.001f && rs_speedF < 1.f && rs_speedF > rs_speedFMin)
				printf("\nUnexpected rs_runF (%0.3f), expected 1.", rs_runF);
#endif
			rs_PLF = 1.f - rs_CdC * (1.f - rs_runF);
			rs_inPrimary = fabs(rs_outSen + rs_outLat)/ max(.01f, rs_effCt * rs_PLF);
		}

		if (rs_pMtrElec)
		{	rs_pMtrElec->H.clg += rs_inPrimary * Top.tp_subhrDur;	// compressor energy for step, Btu
			rs_pMtrElec->H.fanC += rs_inFan * Top.tp_subhrDur;
#if defined( _DEBUG)
			if (!rs_IsFanCoil() && rs_pMtrElec->H.clg < .01f && rs_pMtrElec->H.fanC > .01f)
				printf("\nFanC > 0");
#endif
		}
	}
	else if (rs_mode == rsmHEAT)
	{
		if (rs_IsHP())
		{	// rs_runFAux known at this point
			if (rs_effHt == 0.f)		// if compressor is off
			{	// rs_inPrimary = 0.;
				// rs_COPHtAdj = 0.f;
				runFFan = rs_runFAux;
				rs_outFan = rs_runFAux * rs_fanHeatH;
				rs_outAux = rs_runFAux * rs_capAuxH;
				rs_runF = rs_PLF = 0.f;
				rs_capHt = rs_capHtFS = rs_capDfHt = 0.f;	// clear values for reports
				// aux may be on, handled below
			}
			else
			{
				float fFanPwrPrim;		// run fraction for both fan and primary
				if (rs_runFAux > 0.f && rs_ctrlAuxH == C_AUXHEATCTRL_ALT)
				{	// aux/primary alternate: primary runs when aux does not
					rs_runF = 1.f - rs_runFAux;
					fFanPwrPrim = rs_runF;		// fan heat allocation to primary
					runFFan = 1.f;
#if defined( _DEBUG)
					if (rs_speedF != 1.f)
						printf("\nUnexpected rs_speedF = %0.3f", rs_speedF);
#endif
				}
				else
				{
					runFFan = rs_runF;
					fFanPwrPrim = 1.f;
				}

				rs_capSenNetFS = rs_capHt - rs_capDfHt;	// net capacity
														// includes fan heat; does not include defrost

				double outTot = rs_runF * rs_capHt;

				rs_outAux = rs_runFAux * rs_capAuxH;

				rs_outFan = min(outTot+rs_outAux, runFFan * rs_fanPwr);	// fan output, Btuh
				// insurance: limit to total output
				rs_outSen = outTot - rs_outFan*fFanPwrPrim;	// compressor sensible output (note defrost adjustment below)
				// rs_outLat = 0.;								// total latent output

#if defined( _DEBUG)
				// some checking if aux is running
				if (rs_runFAux > 0.f)
				{
					if (rs_ctrlAuxH == C_AUXHEATCTRL_LO)
					{
						if (rs_runF > 0.f)
							printf("\nAux LO: compressor running");
					}
					else if (rs_ctrlAuxH == C_AUXHEATCTRL_ALT)
					{
						if (abs(rs_runFAux + rs_runF - 1.f) > 0.0001f)
							printf("\nAux alt: bad runF+runFAux");
					}
					else
					{
						if (rs_runF != 1.f || rs_speedF != 1.f)
							printf("\nAux cycle: not full speed");
					}
				}
#endif

				rs_outDefrost = rs_runF * rs_capDfHt;
				rs_outSen -= rs_outDefrost;
				rs_PLF = 1.f - rs_CdH * (1.f - rs_runF);
				rs_COPHtAdj = rs_effHt * rs_PLF;		// rs_effHt > 0, see test above
				rs_inPrimary = rs_outSen / rs_COPHtAdj;

			}
			rs_inAux = rs_outAux / (rs_effAuxH * rs_fEffAuxHBackup);
			rs_inDefrost = rs_outDefrost / (rs_effAuxH * rs_fEffAuxHDefrost);

			if (rs_pMtrAux)
				rs_pMtrAux->H.hpBU += (rs_inAux + rs_inDefrost) * Top.tp_subhrDur;
		}
		else if (rs_IsCHDHW())
		{	// cycling
			// pump power
			rs_outSenTot = rs_runF * rs_capHt;
			float avf;
			float fanPwr;
			rs_pCHDHW->hvt_BlowerAVFandPower(rs_outSenTot, avf, fanPwr);
			// if (rs_runF < 1.) cycle?

			rs_outFan = rs_runF * fanPwr * Top.tp_subhrDur * BtuperWh;
			rs_outSen = max(0., rs_outSenTot - rs_outFan);		// net -> gross
			runFFan = rs_runF;

			// flow (gpm) needed for gross output
			float waterVolFlow = rs_pCHDHW->hvt_WaterVolFlow(rs_outSen, rs_tCoilEW);
			float vol = rs_runF * waterVolFlow * Top.tp_subhrDur * 60.f;

			// return temp based on gross (coil) output
			float tRet = rs_tCoilEW - rs_outSen * Top.tp_subhrDur / (vol * waterRhoCp);

			// apply draw to DHWSYS
			DHWSYS* pWS = rs_GetCHDHWSYS();
			pWS->ws_AccumCHDHWFlowSh(vol, tRet);

			// rs_inPrimary = 0.;
		}
		else
		{	// non-ASHP, non-CHDHW
			rs_capSenNetFS = rs_capHt;				// net capacity, Btuh
			double outTot = rs_runF * rs_capHt;		// total output (incl fan), Btuh
			// rs_outLat = 0.;						// total latent output
			rs_outFan = min( outTot, rs_runF * rs_fanPwr);	// fan output, Btuh
															// insurance: limit to total output
			runFFan = rs_runF;
			rs_outSen = outTot - rs_outFan;
			rs_PLF = 1.f;
			if (rs_IsFanCoil())
				;		// nothing needed, energy supplied externally
			else
			{	// not fancoil, not CHDHW, not AHSP
				rs_inPrimary = rs_outSen / rs_effHt;
			}
		}

		if (rs_pMtrHeat)
			rs_pMtrHeat->H.htg += rs_inPrimary * Top.tp_subhrDur;
		rs_inFan = rs_outFan;	// fan input, Btuh (in = out, all fan heat into air)
		if (rs_pMtrElec)
			rs_pMtrElec->H.fanH += rs_inFan * Top.tp_subhrDur;

		rs_outSenTot = rs_outSen + rs_outDefrost + rs_outAux + rs_outFan;
	}
	else if (rs_mode == rsmOAV)
	{	runFFan = rs_runF;
		rs_outSenTot = rs_outFan = rs_inFan = rs_fanHeatOAV * rs_runF;
		if (rs_pMtrElec)
			rs_pMtrElec->H.fanC += rs_inFan * Top.tp_subhrDur;
	}
	// else if (rs_Mode == rsmOFF)

	if (rs_capSenNetFS != 0.f)
	{	
		rs_PLR = rs_znLoad[0] / rs_capSenNetFS;
	}

	// parasitic consumption
	if (rs_pMtrElec)
		rs_pMtrElec->H.aux += rs_parElec * Top.tp_subhrDur * BtuperWh;
	if (rs_pMtrFuel)
		rs_pMtrFuel->H.aux += rs_parFuel * Top.tp_subhrDur;

	if (runFFan > 0.f)
	{	// duct conduction and leakage to adjacent zones
		for (int iSR=0; iSR<2; iSR++)
		{	int iDS = rs_Dsi( iSR);
			if (iDS>0)
				DsR[ iDS].ds_FinalizeSh( runFFan);
		}
	}

	return RCOK;

}		// RSYS::rs_FinalizeSh
//============================================================================
static RC loadsIzxSh1()  	// interzone transfers, part 1
// does all non-AirNet IZXFERs + infil AirNet

// accumulates zone .zn_qIzShXAnSh's, assumed pre-0'd.
{
	RC rc=RCOK;

	if (Top.tp_airNetActive && Top.tp_pAirNet)		// if any AirNet
	{	rc = Top.tp_pAirNet->an_Calc(0);	// find all-zone pressure balance
		if (rc == RCNOP)	// if nothing done (no zones)
			rc = RCOK;		// treat as OK
	}

	IZXRAT* ize;
	RLUP( IzxR, ize)
	{	if (ize->iz_IsAirNet())
			ize->iz_ZoneXfers( 0);	// accum heat flows to zone
		else
			ize->iz_Calc();		// calc non-AirNet transfers
								//   accum to ZNR.zn_qIzXAnSh's
	}

	ZNR* zp;
	RLUPC( ZrB, zp, !zp->zn_IsConvRad())
		zp->zn_qIzSh = zp->zn_qIzXAnSh
						+ zp->zn_AnAmfCpT( 0) - zp->zn_AnAmfCp( 0)*zp->tzls;
		// else zn_qIzXAnSh used in CR heat balance eqns elsewhere

	return rc;
}		// ::loadsIzxSh1
//-----------------------------------------------------------------------------
static RC loadsIzxSh2()   		// interzone transfers, part 2
// calcs AirNet vent

// accumulates zone .zn_qIzSh's, assumed pre-0'd.
{
	RC rc=RCOK;

	if (Top.tp_airNetActive && Top.tp_pAirNet)
	{	rc = Top.tp_pAirNet->an_Calc( 1);	// find pressure balance
		if (rc == RCNOP)
			rc = RCOK;
		if (rc == RCOK)
		{	IZXRAT* ize;
			RLUP(IzxR, ize)
			{	if (ize->iz_IsAirNet())
					ize->iz_ZoneXfers(1);	// accum heat flows to zone
			}
		}
	}

#if defined( DEBUGDUMP)
	if (DbDo( dbdAIRNET|dbdIZ))
	{	ZNR* zp;
		DbPrintf("\n");
		RLUP( ZrB, zp)
			DbPrintf( "%s vent:  MCpVent=%.2f   MCpTVent=%.1f\n",
				zp->Name(),
				zp->zn_AnAmfCp( 1), zp->zn_AnAmfCpT( 1));
	}
#endif
	return rc;
}		// ::loadsIzxSh2
//--------------------------------------------------------------------
static RC loadsXFans()	// SIMULATE zone exhaust fans (xfans)
{
	RC rc = RCOK;
	ZNR* zp;
	RLUP( ZrB, zp)
		rc |= zp->zn_XFan();
	return rc;
}	// ::loadsXFans
//--------------------------------------------------------------------
RC ZNR::zn_XFan()		// zone exhaust fan calcs (hourly)
{
	float fOn = i.xfanFOn;
	i.xfan.fn_puteVf( i.xfan.vfDs*fOn, tzlh, fOn);

	if (i.xfan.fn_mtri)
		MtrB.p[ i.xfan.fn_mtri].H.fan += i.xfan.p;

	return RCOK;

}	// ZNR::zn_XFan
//====================================================================
static RC loadsSurfaces( 		// surface runtime simulation
	BOO subhrly)	// 0 to do hourly masses, 1 to do subhourly surfaces/masses
// called before zone balances
{
	RC rc = RCOK;

	// Init zone surface heat xfers to 0, loops below sums to them
	ZNR* zp;
	RLUP( ZrB, zp)					// for zp = ZNR record 1 to .n
	{	if (subhrly)
		{	zp->zn_ieMassls = zp->zn_ieMass;	// re determination of mass internal energy change
			zp->aMassSh = zp->zn_hcATMsSh = zp->zn_hrATMsSh = zp->zn_hcAMsSh = zp->zn_hrAMsSh
				= zp->zn_qCondQS = zp->zn_qCondMS = zp->zn_ieMass = 0.;
			zp->qSgTotSh = zp->zn_sgTotShTarg.st_tot;
						// total solar gain to zone, Btuh (redundant re reporting and bal checks)
						//  add'l may be added by surfaces, e.g. ASHWAT inward flowing fraction
		}
		else
		{	zp->aMassHr = 0.;
			zp->qSgTot = zp->zn_sgTotTarg.st_tot;	// see qSgTotSh comments just above
		}
	}

	if (subhrly)
	{	XSRAT* xr;
		TMRSTART( TMR_BC);
		RLUP (XsB, xr)
			rc |= xr->x.xs_SubhrBC();
		TMRSTOP( TMR_BC);

		TMRSTART( TMR_AWTOT);	// total ASHWAT time (incl setup)
								// see also TMR_AWCALC
		RLUPC(XsB, xr, xr->x.xs_IsASHWAT())
			rc |= xr->x.xs_ASHWAT();
		TMRSTOP( TMR_AWTOT);

		RLUPC(XsB, xr, xr->x.xs_ty != CTMXWALL)
			rc |= xr->x.xs_SubhrQS();
	}

	float dur, tDbO;
	if (subhrly)
	{	dur = Top.tp_subhrDur;			// interval duration (hours)
		tDbO = Top.tDbOShAv;		// outdoor temp averaged over time interval
	}
	else			// hourly
	{	dur = 1.f;
		tDbO = Top.tDbOHrAv;
	}

	// masses
	TMRSTART( TMR_COND);
	MSRAT* mse;
	RLUPC( MsR, mse, subhrly == mse->isSubhrly)	// do matching interval
	{	if (mse->ms_isFD)
			mse->ms_StepFD();		// FD (forward_difference) model is only subhourly
		else
			mse->ms_StepMX( dur, tDbO);
	} // mass loop
	TMRSTOP( TMR_COND);

	// kiva instances
	TMRSTART(TMR_KIVA);

	BOO kivaIsSubhourly = Top.tp_grndTimeStep == C_TSCH_SH;

	if (subhrly == kivaIsSubhourly) // Only enter if correct interval
	{
		for (auto& kiva : kivas)
		{
			kiva.kv_Step(dur);
		}

		// Loop over surfaces to assign weighted values and accummulate to zone balance
		XSRAT* xr;
		RLUP(XsB, xr)
		{
			rc |= xr->xr_ApplyKivaResults();
		}

	}

	// Loop over surfaces to accummulate to zone balance
	XSRAT* xr;
	RLUP(XsB, xr)
	{
		rc |= xr->xr_KivaZoneAccum();
	}

	TMRSTOP(TMR_KIVA);


	return rc;
}			 // loadsSurfaces
//-----------------------------------------------------------------------------
RC XSURF::xs_SubhrBC()		// subhour: surface boundary conditions
{
	RC rc = RCOK;

	// set surface coefficients
	double areaEff = xs_IsASHWAT() ? xs_AreaGlazed() : xs_area;
	xs_sbcI.sb_SetCoeffs( areaEff, uC);
	xs_sbcO.sb_SetCoeffs( areaEff, uC);
	if (xs_IsASHWAT())
		xs_pFENAW[ 0]->fa_FrameBC();
	return rc;
}		// XSURF::xs_SubbrBC
//-----------------------------------------------------------------------------
RC XSURF::xs_ASHWAT()		// subhour calcs for ASHWAT fenestration
// do NOT call for if !xs_IsASHWAT()
{
	RC rc = RCOK;

	ZNR& z = ZrB[ xs_GetZi(0)];		// zone on inside of surface

	// interior shades: determine fraction closed
	float fSC = xs_HasControlledShade() ? z.i.znSC : 0.f;	// fraction closed
	int bDoFrm = 1;
	if (fSC < .999f)
	{	// shades open
		xs_pFENAW[ 0]->fa_Subhr( 1.f - max( fSC, 0.f), bDoFrm);
		bDoFrm = 0;		// do frame only once!
	}
	if (fSC > .001f)
	{	// shades closed
		xs_pFENAW[ 1]->fa_Subhr( min( fSC, 1.f), bDoFrm);
	}
	FPCHECK;

	return rc;
}		// XSURF::xs_ASHWAT
//-----------------------------------------------------------------------------
RC XSURF::xs_SubhrQS()		// subhour quick surface
{
	// Only call for quick surfaces!
	RC rc = RCOK;

	ZNR& z = ZrB[ xs_GetZi(0)];		// zone on inside of surface
	if (xs_IsASHWAT())
		xs_pFENAW[ 0]->fa_FrameQS();
	else
	{	z.zn_AccumBalTermsQS( xs_area, xs_sbcI, xs_sbcO);
		if (xs_sbcO.sb_zi)
		{	// interzone quick surface: handle adjacent zone
			ZNR& zx = ZrB[ xs_sbcO.sb_zi];
			zx.zn_AccumBalTermsQS( xs_area, xs_sbcO, xs_sbcI);
		}
	}
	
	FPCHECK;
	return rc;
}		// XSURF::xs_SubhrQS
//-----------------------------------------------------------------------------
RC XSURF::xs_EndSubhr()		// end-of-subhr calcs for surface
{
	RC rc = RCOK;

	// update adjacent temps from zone heat bal outcome
	//   (nop if not adjacent to zone)

	xs_sbcI.sb_SetTx();
	xs_sbcO.sb_SetTx();

	ZNR& z = ZrB[ xs_GetZi( 0)];

	if (xs_IsASHWAT())
	{	xs_pFENAW[ 0]->fa_FrameEndSubhr();
		float fSC = xs_HasControlledShade() ? z.i.znSC : 0.f;	// fraction closed
		if (fSC < .999f)
			xs_pFENAW[ 0]->fa_EndSubhr( 1.f - max( fSC, 0.f));
		if (fSC > .001f)
			xs_pFENAW[ 1]->fa_EndSubhr( min( fSC, 1.f));
	}
	else if (xs_msi != 0)
	{	MSRAT& ms = MsR[ xs_msi];
		xs_sbcI.sb_qSrf = ms.ms_pMM->mm_qSrf[ 0];
		xs_sbcO.sb_qSrf = ms.ms_pMM->mm_qSrf[ 1];

		// accum to zone
		//  zn_ieMass and zn_qCondMS are experimental value re energy balance checking
		//  unused? as of 12-21-10
		z.zn_ieMass += ms.ms_InternalEnergy();
		double qSrfA = xs_area*xs_sbcO.sb_qSrf;		// conduction at outside surface, Btuh
		z.zn_qCondMS += qSrfA;
		if (sfAdjZi)
		{	ZNR& zx = ZrB[ sfAdjZi];
			zx.zn_qCondMS -= qSrfA;
		}

		if (Top.isEndHour)
		{	// last step of hour: store layer boundary temps for probes
			MASSMODEL* pMM = ms.ms_pMM;
			pMM->mm_FillBoundaryTemps( xs_tLrB, XSMXTLRB);
		}
	}
	else if (xs_IsKiva())
	{
		// Kiva accounting?
	}
	else
		z.zn_EndSubhrQS( xs_area, uC, xs_sbcI, xs_sbcO);

	return rc;

}	// XSURF::xs_EndSubhr
//-----------------------------------------------------------------------------
void XSURF::xs_AfterSubhr()		// end-of-subhr calcs for surface
{
	// last step surface temps (w/o *e, re probes)
	xs_sbcI.sb_tSrfls = xs_sbcI.sb_tSrf;
	xs_sbcO.sb_tSrfls = xs_sbcO.sb_tSrf;
}	// XSURF::xs_AfterSubhr
//-----------------------------------------------------------------------------
void XSURF::xs_AfterHour()		// end-of-hour calcs for surface
{
#if 0
	if (xs_msi != 0)
	{	MSRAT& ms = MsR[ xs_msi];
		MASSMODEL* pMM = ms.ms_pMM;
		pMM->mm_FillBoundaryTemps( xs_tLrB, XSMXTLRB);
	}
#endif
}	// XSURF::xs_AfterHour
//-----------------------------------------------------------------------------
RC FC loadsAfterSubhr()
// end-subhour after-exprs/reports stuff for loads: set 'prior interval' variables etc
// if done sooner, probes come out wrong.
{
	RC rc=RCOK;

	// zones
	ZNR* zp;
	RLUP( ZrB, zp)
		rc |= zp->zn_AfterSubhr();

	// surfaces
	XSRAT* xr;
	RLUP( XsB, xr)
		xr->x.xs_AfterSubhr();

	// systems
	RSYS* rs;
	RLUP( RsR, rs)
		rc |= rs->rs_AfterSubhr();

	DUCTSEG* ds;
	RLUP( DsR, ds)
		ds->ds_AfterSubhr();

	return rc;
}		// loadsAfterSubhr
//-------------------------------------------------------------------------------------
RC ZNR::zn_AfterSubhr()
// end-subhour after-exprs/reports stuff for loads: set 'prior interval' variables etc
// if done sooner, probes come out wrong.
{
#if 0 && defined( _DEBUG)
	if (Top.iHr == 7)
		printf("\niHr=%d", Top.iHr);
#endif
	tzlsDelta = tz - tzls;		// last subhour t delta: may be used to extrapolate next tz
	tzls = tz;					// last subhr zone air temp for next subHour
	wzlsDelta = wz - wzls;		// last subhour w delta: may be used to extrapolate next wz
	wzls = wz;					// last subhr zone hum rat for next subHour
	znXLGainLs = znXLGain;		// last subhour excess latent gain: next subhour sensible condensation gain. 5-97.
	// pz0 (pressure relative to patm at nomimal z=0) initially 0, then known from
	//   prior AirNet solution
	zn_rho0ls = zn_Rho0();		// last subhour moist air density at tz, wz, pz0
	zn_trls = zn_tr;			// last subhour radiant temp
	zn_relHumls = zn_relHum;	// lass subhour relative humdity

	zn_hcAirXls = i.zn_hcAirX;	// last subhour air exchange rate
								//    for convection coefficient models

	// ducts: nothing needed (current step values are used (lagged) at beg
	//        of next step, then initialized)

#if 0 // 7-17-92 FAILS cuz solar gains recalculated hourly only.
x   // default zone shade closure (former shutter fraction): whether movable window shades open or closed for next subhour.
x   // note default is determined subhourly, whereas CALRES was hourly (used zrh->nHrCool; move to loadsAfterHour).
x       if (!znSCF)						// not if expression given by user
x       {
x			ZNRES_IVL_SUB *zrs = &ZnresB.p[ ss].curr.S;	// point zone current subhour results in resrat
x			i.znSC = (float)(zrs->qsMech < 0.);		// 1.0 if cooling in previous subhour, else 0.
x		}
#endif

	zn_sysDepAirIls = zn_ductLkI;			// total duct leak flow into this zone

	zn_rsAmfRetLs = zn_rsAmfRet;

	return RCOK;
}		// ZNR::zn_AfterSubhr
//-------------------------------------------------------------------------------------
RC FC loadsAfterHour()
// end-hour after-exprs/reports stuff for loads
// set 'prior interval' variables etc: if done sooner, probes come out wrong.
{

	RC rc=RCOK;

	// surfaces
	XSRAT* xr;
	RLUP( XsB, xr)
		xr->x.xs_AfterHour();

	// zones
	ZNR* zp;
	RLUP( ZrB, zp)
		rc |= zp->zn_AfterHour();

	// systems
	RSYS* rs;
	RLUP( RsR, rs)
		rc |= rs->rs_AfterHour();

	return rc;
}		// loadsAfterHour
//-----------------------------------------------------------------------------
RC ZNR::zn_AfterHour()
// end-hour after-exprs/reports stuff for loads
// set 'prior interval' variables etc: if done sooner, probes come out wrong.
{

	// next hour's "end last hour" zone air temp
	tzlh = tzls;		// air temp from final step of hour (NOT average here).
						// needed? used in main eqn in loadsHourBeg, but .tzls cd be used directly 1-92.

	zn_trlh = zn_trls;			// ditto radiant temp

	zn_relHumlh = zn_relHumls;	// ditto relative humidity

	// end of hour set points
	// used re ramped setpoints for autosizing
	zn_tzspHlh = zn_tzspH;
	zn_tzspDlh = zn_tzspD;
	zn_tzspClh = zn_tzspC;


#if 0 // nHrCool is present but not set: no longer works.
x   // zone shutter fraction for next hour: determine whether movable window shades open or closed next hour
x	ZNRES_IVL_SUB *zrh = &ZnresB.p[ ss].curr.H;	// point zone current hour results in resrat
x	shtf = (float)(zrh->nHrCool);		// 1 if cooling, else 0
x
x	// ?? This is *NOT* the full CEC current algorithm. Where is the 1 degF hysteresis?
#else // 7-17-92

	// default zone shade closure (former shutter fraction): whether movable window shades open or closed for next hour.
	// note this code responds to net cooling, whereas CALRES responded to ANY cooling (zrh->nHrCool, not now implemented).
	// subhourly defaulting desirable (rob's opinion); must duplicate loadsHourBeg sg code in loadsSubhr w znSC-change trigger.
	if (!znSCF)						// not if expression given by user
	{
		ZNRES_IVL_SUB *zrh = &ZnresB.p[ ss].curr.H;	// point zone current subhour results in resrat
		i.znSC = (float)(zrh->qsMech < -ABOUT0);	// 1.0 if cooling (more than heating) in previous hour, else 0.
													// -ABOUT0: no cooling if near 0 so not random when hvac off, 8-92.
	}
#endif
	return RCOK;
}		// ZNR::zn_AfterHour

// end of cnloads.cpp
