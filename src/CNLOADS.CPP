// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnloads.cpp -- Hourly simulation loads modelling calculations for a single hour for CSE

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>		// global defines

#include "SRD.H"
#include <ANCREC.H>		// record: base class for rccn.h classes
#include <rccn.h>		// IZXRAT SGRAT ZNR
#include "IRATS.H"
#include "TIMER.H"

#include <PSYCHRO.H>	// psyHCondWtr
#include <CGWTHR.H>		// Wthr.nZ .zD

#include "CUEVAL.H"
#include "EXMAN.H"
#include "CSE.h"
#include "CNGUTS.H"
#include "mspak.h"
#include "NUMMETH.H"

#include "array"
#include "btwxt.h"

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
	md = 1;					// zone hvac mode (control mode)
	tz = aTz = tzls = tzlh = 80.f; 	// zone air temps, incl ah working copy
	tr = trls = trlh = 80.f;		// zone radiant temps
	wz = aWz = wzls = Top.tp_refW;	// zone humidity ratios 5-25-92
	zn_rho0ls = zn_Rho0();			// zone air density at tz, wz, pz0
	bcon 							// init constant part of main equation denom:
		= zn_ua    					//  sum uval*area of zone ambient lite surfs
		  + zn_uaSpecT;				//  sum uval*area of zn specT lite surfs (hsu)
	i.znCAirSh = i.znCAir / Top.tp_subhrDur;		// commonly used in subhr code

	// Mass->ha's done in ms_RddInit
	// aMassHr = aMassSh = 0.;   object is pre-0'd but may be re-used in autoSizing
	//							 but believe these don't need init for start-interval masses
	haMass = 0.;				// +='d in ms_rddInit. pre-0'd object may be re-used in autoSizing.
	znXLGain = znXLGainLs = 0;	// no condensation heat leftover from prior iteration, rob 6-11-97
#if 0 // no, not reporting condensation in autosize (wd need to rerun all days) 5-97. Mbrs not added. DELETE.
x	znXLGainYr = 0;				// init cumulative excess latent gain (condensation), rob 5-97
x	znNSubhrXLG = 0;			// init # subhours with XLG, rob 5-97. May be reusued in autosizing.
#endif
	zn_ebErrCount = 0;			// count of short-interval energy balance errors


	// HVAC convective delivery fraction
	//   needs elaboration for radiant systems
	zn_fConvH = zn_fConvC = zn_fConv = 1.f;	// used iff ZNHVACFCONV defined

	return RCOK;
}		// ZNR::zn_RddInit()
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
		name, sg_isSubhrly, sg_addIt,
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
	zn_anVentEffect = 0;	// # of IZXRATs that *could* impact this
							//   zone when operated in vent mode.
							//   see IZXRAT::iz_HasVentEffect()
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

	// bLdHr: hour-constant non-hvac b's for denominator.   b = sigma(UA) + hc/t.
	bLdHr = bcon; 					// just the run-const UAs

	/* xqh: q = b * t - a  using only hourly components, for change tests (just below) only:
	        Intended to be appropriately sensitive to changes when b*a almost equal to a,
	        eg when aMassHr settling & all else constant,
	        cuz terminal loads are of form  b * t - a  (with added subhrly components).
	        Fixed problems with bug39a.inp run (1 mass surf, const temps). 12-5-94. */
	xqh = bLdHr * tz - zn_aqLdHr;		// member only cuz saved to -Pr in loadsSubhr.

	// check/warn alternate model values
	//   However: don't correct value -- consumers use max(), min() as approp.
	//      (Changed value can persist due to expression eval optimization.)
	if (i.znQMxH < -0.01f)
		warn( "Zone '%s', %s: znQMxH %0.f taken as 0 (s/b >= 0)",
				name, Top.When( C_IVLCH_H), i.znQMxH);
	if (i.znQMxC > 0.01f)
		warn( "Zone '%s', %s: znQMxC %0.f taken as 0 (s/b <= 0)",
				name, Top.When( C_IVLCH_H), i.znQMxC);

	/* hourly-only load change checks:
		xqh:     Hourly parts of  b * t - a, just above.
	    znLGain: latent gain rate (set per GAINs by cnguts.cpp:doHourgains, used in cnZtu.cpp:ZNR::znW).
		         Note: for subsequent subhrs, wz change sets ztuCf in loadsSubhr.
	    -Pr's used here are updated in loadsSubhr, every time ztuCf is on. */
	/* aqLdHr and bLdHr are covered by xqh check & no longer checked separately 12-94.
	   Removing aq- and bLdHr checks made NO difference in test suites 12-94. */
	if (
		// .02 found better than .05,.1,.2 in bug39a.inp run (1 mass surf, const temps) (12-94):
		RELCHANGE( xqh, xqhPr) > .02*Top.relTol*MARG1  	// if hourly part of difference b*t-aq (for loads) changed
		||  ABSCHANGE( znLGain, znLGainPr)		// if zone moisture GAIN rate (Btuh) changed:
			> PsyHCondWtr*Top.absHumTol )		//  test abs lb/hr change after conversion
	{
		ztuCf++;							// say zone load changed: must do ztuCompute.
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

#if 0	// moved to zn_BegSubhr2() (below), 2-11-2016
x	// forced convection coeff, Btuh/ft2-F; re C_CONVMODELCH_UNIFIED
x	//   based on lagged value of zone total air change rate
x	//   combined with surface-specific sb_hcNat, see SBC::sb_SetCoeffs()
x	zn_hcFrc = Top.tp_hConvF * i.zn_hcFrcF * pow( max( zn_hcAirXls, 0.f), 0.8f);
#endif

	if (!nMd)				// if zone has no modes (eg startup with no terminals)
		spCf++;				// tell ztuCompute to build zone mode sequence

	zn_unMetCDH = zn_unMetHDH = 0.f;	// unmet temp diffs

	// initialize DHW heat transfer totals
	//   TODO: could be done in DHW code iff needed
	//   Win? -- generally less DHWHEATERs than ZONEs
	//   OTOH, this is foolproof and simple, 2-16
	zn_qDHWLoss = zn_qDHWLossAir = zn_qDHWLossRad = zn_qHPWH = zn_hpwhAirX = 0.;

	return RCOK;
}			// ZNR::zn_BegSubhr1
//-----------------------------------------------------------------------------
RC ZNR::zn_BegSubhr2()			// zone start of subhour, part 2
// call *after* lagged values used
{
#if 1	// moved from zn_BegSubhr1 (above), 2-11-2016
	// forced convection coeff, Btuh/ft2-F; re C_CONVMODELCH_UNIFIED
	//   based on lagged value of zone total air change rate
	//   combined with surface-specific sb_hcNat, see SBC::sb_SetCoeffs()
	//   must be done *after* DHW calcs re HPWH air motion
	zn_hcFrc = Top.tp_hConvF * i.zn_hcFrcF * pow( max( zn_hcAirXls + zn_hpwhAirX, 0.f), 0.8f);
#endif
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

// SIMULATE SUBHOURLY SURFACES. Must follow (any future) subhouly solar gain determination, precede zone loop. Uses Top.tp_subhrDur.
	loadsSurfaces( TRUE);					// below. TRUE = do subhourly (not hourly masses)

	TMRSTART( TMR_ZONE);

// COMPUTE INTERZONE TRANSFERS and base (infil only) airnet
	CSE_E( loadsIzxSh1() )

// GLOBAL LOAD CHANGE CHECK: outdoor humidity -- change changes all zone loads, but does not show in a,b.
	BOO azCf = FALSE;   			// TRUE for all-zones load change
	static float wOShPr = 0.;
	if (fabs(Top.wOSh - wOShPr) > Top.absHumTol) 	// if outdoor w changed .00003 (or as changed) or more (typ w = .0020++)
		/* TESTED 6-13-92 (hourly wO): ztuCf for wO change improved simulation
		   (and added iterations) when wO changes and all else constant. */
	{
		wOShPr = Top.wOSh; 		// save for comparison at start next subhour
		azCf++;					// say set zp->ztuCf for all zones (passed to zn_LoadsSubHr() below)
	}

	BOOL bDbPrint = DbDo( dbdZM);

	RLUP( ZrB, zp)
		rc |= zp->zn_InitSubhr();		// final zone init for loads calc
										//   sets values for all models

	if (!Top.tp_bAllCR)
	{	RLUP( ZrB, zp)
			rc |= zp->zn_LoadsSubhr( azCf);	// non-CR stuff
											//   change-check subhour zone loads, etc.
		return rc;
	}

	// current floating temp of all zones
	RLUP( ZrB, zp)
		zp->zn_TAirFloatCR();

	// convective-radiant model multizone vent control

	// TODO ideas
	// * Overall vent available flag Top.? (allows scheduling vent)
	// * Better estimate of available vent supply temp.
	//     Use prior step? (run vent at substep 0 even if not needed?)

#if 0 && defined( _DEBUG)
	bool bReportVent = Top.jDay == 178 /* && !Top.isWarmup */;
#endif

	int nVentUseful = 0;	// ventilation usefulness
							//   >0: vent potentially useful for 1 or more zones
							//   =0: vent not useful
							//   <0: vent forbidden by conditions in 1 or more zones
							//       e.g. heating or cooling required

	ULI ventAvail = Top.tp_GetVentAvail();		// overall vent availability

	Top.tp_fVent = 0.f;	// consensus whole building vent fraction (if not RSYSOAV)
						//     = fraction of full vent flow to use
						//       determined by zone that wants the least vent
						//  or -1 for zonal vent control

	if (ventAvail == C_VENTAVAILVC_WHOLEBLDG || ventAvail == C_VENTAVAILVC_ZONAL)
	{
		nVentUseful = 0;	// ventilation usefulness
						//   >0: vent potentially useful for 1 or more zones
						//   =0: vent not useful
						//   <0: vent forbidden by conditions in 1 or more zones
						//       e.g. heating or cooling required
		RLUP(ZrB, zp)
		{	int nVU1 = zp->zn_AssessVentUtility();
#if 0 && defined(_DEBUG)
			if (bReportVent)
			{	// printf("\n%s Hr=%d, Zone '%s': VU=%d", Top.dateStr, Top.iHr, zp->name, nVU1);
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
			printf("\n%s Hr=%d: NVU=%d  ANV=%d  TFvnt=%0.3f", Top.dateStr, Top.iHr, nVentUseful, bAllNoVent, Top.tp_fVent);
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
											//   do all zones before zn_AirX() etc.

	RLUP( RsR, rs)
		rc |= rs->rs_AllocateZoneAir();		// NOP if OAV

	RLUP( ZrB, zp)
		rc |= zp->zn_CondixCR2();			// finalize zone for all modes (including OAV)

	RLUP( RsR, rs)
		rc |= rs->rs_FinalizeSh();

	RLUP( ZrB, zp)
	{	rc |= zp->zn_AirX();
		rc |= zp->zn_MoistureBalCR();
		rc |= zp->zn_ComfortCR();
		rc |= zp->zn_LoadsSubhr( azCf);	// insurance, may not be needed for CR
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
				zp->name, zp->zn_pz0W[0], zp->zn_pz0W[1], zp->zn_pz0, zp->zn_qIzSh,
				zp->zn_AnAmfCp(0), zp->zn_AnAmfCpT(0));
	}
#endif

	TMRSTOP( TMR_ZONE);

	return rc;
}			// loadsSubhr
//----------------------------------------------------------------------------
RC ZNR::zn_InitSubhr()
{
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
	zn_bLdSh = bLdHr			// non-hvac UA's constant for hour
			+ zn_uaInfil 		// + infil UA this subhr
			+ i.znCAirSh;    	// + hc/t: .znCAirSh is hc.
	// "cair" is heat capac of everything in zone not modelled as "mass":
	// user const & per-ft2 parts for air, furniture, walls, etc).

	/* zn_xqs: q = b * t - a  using components avail here, for change tests only:
	        Intended to be appropriately sensitive to changes when b*a almost equal to a,
	        since terminal loads are of form  b * t - a.
	        Insurance 12-94 -- need was not proven at subhour level when all masses hourly (b4 aMassSh added).
	        But it did something: eliminated undercooling in hour 13 in S1LOH test run, 12-5-94, so keep. */
	zn_xqs = zn_bLdSh * tzls - zn_aqLdSh;

	// convective-radiant zone terms
	double nAirSh =
		  i.znCAirSh*tzls		// air heat cap
		+ qsIgHr				// convective internal gains
		+ zn_AnAmfCpT( 0)
		+ zn_qIzXAnSh			// non-airnet interzone
		+ zn_uaInfil*Top.tDbOSh;	// non-airnet infiltration
	zn_nAirSh += nAirSh;

	double dAirSh =				// associated denominator terms
		i.znCAirSh
		+ zn_AnAmfCp( 0)
		+ zn_uaInfil;
	zn_dAirSh += dAirSh;

	double nRadSh =
#if 0	// excess latent gain ignored in CR model, 5-15-2012
x		znXLGainLs +		// excess latent gain last subhr == heat of condensation
#endif

		qrIgTot;			// radiant internal gains

	zn_nRadSh += nRadSh;

	double dRadSh = 0.;			// nothing additional
	// zn_dRadSh += dRadSh;

	zn_airCx = zn_airCxF*pow3( DegFtoR( tzls));
	zn_cxSh += zn_airCx;
	if (zn_cxSh < .001)
		zn_cxSh = .001;

	// useful combinations of terms
	zn_dRpCx  = zn_dRadSh + zn_cxSh;
	zn_nRxCx  = zn_nRadSh * zn_cxSh;
	zn_dRxCx  = zn_dRadSh * zn_cxSh;

	zn_balC1 = zn_nAirSh*zn_dRpCx + zn_nRxCx;
	zn_balC2 = zn_dAirSh*zn_dRpCx + zn_dRxCx;

	zn_hcMode = RSYS::rsmOFF;

#if defined( DEBUGDUMP)
	if (DbDo( dbdRCM))
		DbPrintf( "%s air  qsIgHr=%0.1f  qrIgTot=%0.1f  uaInfil=%0.1f\n"
		          "      nAir=%0.2f  dAir=%0.2f  nRad=%0.2f  dRad=%0.2f  CxF=%0.6g  CX=%0.2f\n",
			name, qsIgHr, qrIgTot, zn_uaInfil,
			nAirSh, dAirSh, nRadSh, dRadSh, zn_airCxF, zn_airCx);
#endif
	return RCOK;
}		// ZNR::zn_InitSubhr
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
		name, zn_IsUZ() ? "UZ" : "CZ",
		zn_AnAmfCp( 0), zn_AnAmfCpT( 0), zn_AnAmfCp( 1), zn_AnAmfCpT( 1),
		zn_ventUt);
	DbPrintf("     Nair=%.2f  Dair=%.2f  Nrad=%.2f  Drad=%.2f  CX=%.2f  airX=%.3f\n",
		zn_nAirSh, zn_dAirSh, zn_nRadSh, zn_dRadSh, zn_cxSh, i.zn_hcAirX);
	if (!zn_IsUZ())
	{	DbPrintf("     TH=%.2f  TD=%.2f  TC=%.2f",
				i.znTH, i.znTD, i.znTC);
		if (!zn_HasRSYS())
				DbPrintf("  qhCap=%.f  qcCap=%.f\n", i.znQMxH, i.znQMxC);
		else
			DbPrintf("  tSP=%.2f  md=%d  amfReq=%.f  amfSup=%.f   tSup=%.2f\n",
				zn_tzsp, zn_hcMode, zn_rsAmfSysReq[ 0], zn_sysAirI.af_amf, zn_sysAirI.as_tdb);
	}
	DbPrintf( "     ta=%.2f  tr=%.2f  qIzSh=%.f  fvent=%.3f  pz0=%.4f  qsHvac=%.f\n",
			tz, tr, zn_qIzSh, zn_fVent, zn_pz0, zn_qsHvac);
}		// zn_DbDump
//-----------------------------------------------------------------------------
RC ZNR::zn_LoadsSubhr(
	BOO azCf)		// TRUE iff "all-zones change" e.g. outdoor w changed
{

	// set change flag for terminal module if zone loads changed (other than subhour length change).  Cleared in ZNR::ztuCompute.

	// monitor subhourly-changing loads inputs to zone heat and moisture balances except subhrDur:
	//  heat balance:     aqLdSh: tzls, zn_qIzSh; zn_aqLdHr is hourly; znCair is constant.  bLdSh: changes only per subhrDur.
	//                    xqs
	//  moisture balance: wzls.  znLGain is hourly.
	//  other:            md

	if ( ztuCf 				// if a flag already set, skip tests, update priors to avoid unnec flagging later.
	 ||  azCf							// if all-zones change detected (outdoor w, above)
	 ||  spCf							// spCf implies ztuCf (and more), 6-92.
	 ||  RELCHANGE( zn_xqs, xqsPr) > .02*Top.relTol*MARG2		// b*t - a as known here: difference sensitive.
	     // .02 copied from hour case. Probably new dominant term;
	     // may make some of the other checks unnec 12-5-94.
	 ||  ABSCHANGE( tzls, tzlsPr) > Top.relTol*2.*MARG2	// Dominant term.  MATCH above. use relTol as small absTol.
		 // historical 7-13-92 (b4 bLdHr ck above): .002
		 // .005 (w/o mdPr check) --> .0001 ebal errs (T12) 4-92.
		 // (without tz += tzlsDelta below, < .001 required here)
	 ||  ABSCHANGE( wzls, wzlsPr) > .2*Top.absHumTol  	// MATCH tolerance above. dflt .000006, or as changed.
		 // .00002 (vs what??) adds iterations;
		 // .00001 adds even more; otherwise unexplored.
	 ||  md != mdPr				// if zn hvac mode changed last subhr, don't skip tu's now:
								// eg be sure tz right on change into a setpoint mode.
	 ||  RELCHANGE( zn_qIzSh,  qIzShPr) > .00005 )
	{
		ztuCf++;					// say zone's load changed.
		qIzShPr =   zn_qIzSh;		// reset comparators (Prior values), whenever (and only when) flag set.
		tzlsPr  =   tzls;
		wzlsPr  =   wzls;
		mdPr    =   md;
		xqhPr = xqh;				// ditto;   hourly part of b*t - a  12-5-94.
		xqsPr = zn_xqs;				// ditto;   as much of  b*t - a  as known here 12-5-94
		znLGainPr = znLGain;			// ditto
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

#if 0
x	// init zone's mass solar and radiant internal gains. Mass loop after this zone loop sums into them.
x	qMsSg = 0.f;
x	qrIgMs = 0.f;
#endif

	return RCOK;

}		// ZNR::zn_LoadsSubhr
//-----------------------------------------------------------------------------
RSYS* ZNR::zn_GetRSYS()
{
	RSYS* rs = RsR.GetAtSafe( i.zn_rsi);
	return rs;
}		// ZNR::zn_GetRSYS
//-----------------------------------------------------------------------------
const RSYS* ZNR::zn_GetRSYS() const
{
	RSYS* rs = RsR.GetAtSafe( i.zn_rsi);
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
	int ventUt = 0;
	if (!zn_IsUZ() && zn_anVentEffect > 0)
	{	if (tz < i.znTD)
			ventUt = vForbid;		// vent off or zone temp below TD (any vent would hurt)
		else if (tz > i.znTD)
		{	// zone temp is above TD
			//  vent might be useful
			float tvSup = zn_VentTSup();
			if (tvSup > tz)
				ventUt = vForbid;	// venting would add heat, forbid
			else if (tvSup > i.znTC && zn_IsHCAvail( RSYS::rsmCOOL))
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
		printf("\nzn_FVentCR '%s'", name);
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
		if (zn_tzVent > i.znTC && zn_IsHCAvail( RSYS::rsmCOOL))
			ret = -1;	// zn_fVentPrf = 0
		else if (zn_tzVent > i.znTD)
		{	zn_fVentPrf = 1.f;
			ret = 1;
		}
		else
		{	double amfDT = anAmfCpT - anAmfCp * i.znTD;
			if (amfDT != 0.)
			{	double qVent = zn_QAirCR( i.znTD); // q required to keep tz at i.znTD
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
	ULI ventAvail)		// vent availability
						//   C_VENTAVAILVC_WHOLEBLDG, C_VENTAVAILVC_ZONAL

// determines tz, tr, zn_qsHvac, zn_qIzSh
// returns RCOK or ...
{
	RC rc = RCOK;

#if 0 && defined( _DEBUG)
x	if (!Top.isWarmup)
x	{	if (Top.jDay==31 && strMatch( name, "SDuctZone"))
x			printf( "Hit\n");
x	}
#endif

	// zn_fVent = fVent	-- don't change until end (zone value used herein)
	zn_qsHvac = 0.;
	zn_qlHvac = 0.;
#if defined( ZNHVACFCONV)
	zn_fConv = 1.f;		// HVAC convective fraction
#endif
	zn_rsAmfSysReq[ 0] = zn_rsAmfSysReq[ 1] = 0.;	// RSYS air requests

	zn_anAmfCpVent = 0.;		// full vent heat rate, Btuh/F
	zn_anAmfCpTVent = 0.;		// full vent heat addition, Btuh

	int bUZ = zn_IsUZ();
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
				if (fabs( i.znTD - tz) < .001)
					tz = i.znTD;
#if defined( _DEBUG)
				else
					printf( "Zone '%s': control zone vent mismatch\n", name);
#endif
			}
		}
		else
		{	// tz already known
			zn_pz0 = zn_pz0W[ 0];		// no venting
#if defined( _DEBUG)
			float tzx = zn_TAirCR( 0., 0.);
			if (fabs( tz - tzx) > .0001)
				printf( "Zone '%s': floating temp mismatch\n", name);
#endif
		}

#if defined( _DEBUG)
		if (!bUZ && zn_anVentEffect > 0)
		{	// check conditioned zone outcome
			//   don't complain about zones with no vents
			if (tz < i.znTD - .001f)
			{	if (znfVent > .001 && zn_anAmfCpTVent > .001)
					orWarn("vent tz (%.1f F) below TD (%.1f F)", tz, i.znTD);
				if (tz < i.znTH)
					orWarn("floating tz (%.1f F) below TH (%.1f F)", tz, i.znTH);
			}
			else if (tz > i.znTC + .1f)
				orWarn("floating tz (%.1f F) above TC (%.1f F)", tz, i.znTC);
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
				if (tz > i.znTC)
				{	zn_tzsp = i.znTC;
					zn_hcMode = RSYS::rsmCOOL;
#if defined( ZNHVACFCONV)
					zn_fConv = zn_fConvC;
#endif
				}
				else if (tz < i.znTH)
				{	zn_tzsp = i.znTH;
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
			double qhCap =  max( 0., i.znQMxH);
			double qcCap =  min( 0., i.znQMxC);
			if (tz > i.znTC)
			{	zn_hcMode = RSYS::rsmCOOL;
				tz = zn_tzsp = i.znTC;
#if defined( ZNHVACFCONV)
				zn_fConv = zn_fConvC;
				zn_qsHvac = zn_QsHvacCR(tz, zn_fConv);
#else
				zn_qsHvac = zn_QAirCR(tz);
#endif
				if (fabs( zn_qsHvac) > fabs( qcCap))
				{	zn_qsHvac = qcCap;
#if defined( ZNHVACFCONV)
					tz = zn_TAirCR( zn_qsHvac*zn_fConv, 0., zn_qsHvac*(1.f-zn_fConv));
#else
					tz = zn_TAirCR(zn_qsHvac, 0.);
#endif
					if (Top.isEndHour)
						zn_unMetCDH = tz - i.znTC;		// temp excursion above setpoint
				}
				// else capacity sufficient
			}
			else if (tz < i.znTH)
			{	// tz < TH: air needs heating
				zn_hcMode = RSYS::rsmHEAT;
				tz = zn_tzsp = i.znTH;
#if defined( ZNHVACFCONV)
				zn_fConv = zn_fConvH;
				zn_qsHvac = zn_QsHvacCR(tz, zn_fConv);
#else
				zn_qsHvac = zn_QAirCR( tz);
#endif
				if (zn_qsHvac > qhCap)
				{	// heat at heating capacity, find resulting tz.
					zn_qsHvac = qhCap;
#if defined( ZNHVACFCONV)
					tz = zn_TAirCR( zn_qsHvac*zn_fConv, 0., zn_qsHvac*(1.f-zn_fConv));
#else
					tz = zn_TAirCR(zn_qsHvac, 0.);
#endif

					if (Top.isEndHour)
						zn_unMetHDH = tz - i.znTH;	// temp excursion below setpoint (< 0)
				}
				// else capacity sufficient
			}
		}
	}

#if defined( _DEBUG)
	if (tz < -100.f || tz > 200.f)
		warn( "Zone '%s': unreasonable air temp %0.2f\n", name, tz);
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
		    || (zn_hcMode == RSYS::rsmHEAT && tSup0 <= zn_tzsp)))
	{
		orWarn("Flipped tSup  RSYS='%s', tPln=%0.3f, tSup=%0.3f, tSP=%0.1f, tZn=%0.3f\n",
			rs->name, rs->rs_asOut.as_tdb, tSup0, zn_tzsp, tz);
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

	if (zn_hcMode == RSYS::rsmHEAT && rs->rs_IsASHP() && rs->rs_speedF==1.f)
	{	// ASHP heating full speed: repeat calc with full aux
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
				printf( "Zone '%s': Mode mismatch\n", name);
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
				zn_sysAirO.af_AccumDry( -zn_rsAmfRet, tz, wzls);	// w finalized in zn_MoistureBalCR
#if defined( _DEBUG)
			if (tz < -100.f || tz > 200.f)
				warn( "Zone '%s': unreasonable air temp %0.2f\n", name, tz);
#endif
		}

		// track unmet
		// test against setpoint (with a little tolerance)
		if (!Top.isWarmup)		// skip if warmup (including autosize)
		{	if (zn_hcMode == RSYS::rsmHEAT)
			{	if (tz < zn_tzsp-.001)
				{	if (Top.isEndHour)
						zn_unMetHDH = tz - zn_tzsp;		// hour end temp excursion below set point
#if 0 && defined( _DEBUG)
					if (zn_rsFSize > .9999)
						// air request received: temp should hold
						printf( "Heating temp error\n");
#endif
				}
#if defined( _DEBUG)
				else if (tz > zn_tzsp + .001)
					printf( "\nOverheat");
#endif
			}
			else if (zn_hcMode == RSYS::rsmCOOL)
			{	if (tz > zn_tzsp+.001)
				{	if (Top.isEndHour)
						zn_unMetCDH = tz - zn_tzsp;		// hour end temp excursion above set point
#if 0 && defined( _DEBUG)
					if (zn_rsFSize > .9999)
						// air request received: temp should hold
						printf( "Cooling temp error\n");
#endif
				}
#if defined( _DEBUG)
				else if (tz < zn_tzsp - .001)
					printf( "\nOvercool");
#endif
			}
		}
	}
	// else all known
#if defined( ZNHVACFCONV)
	tr = zn_TRadCR( tz, zn_qsHvac*(1.-zn_fConv));
#else
	tr = zn_TRadCR( tz);
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
		fprintf(f, "%s\n", rs->rs_desc ? rs->rs_desc : "Curves");

		fprintf(f, "Mon,Day,Hr,Sh,fAmf,amf,tReg,amfNeeded\n");

	}

	// RSYS rsSave( rs);		// save for restore at exit
								//   (we alter mbrs here)


	AIRSTATE asSup;		// air state at register
	for (int i = 0; i < 10; i++)
	{
		float fAmf = 1.f - float(i) / 10.f;

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
	double amf = (fabs( tza - tSup) < .00001)
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
	double amf = (fabs(tza - tSup) < .00001)
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
double ZNR::zn_NonAnIVAmf() const	// non-airnet dry air mass flow rate
// Note: non-airnet flows contribute to heat and moisture balance
//       but *NOT* pressure balance (TODO?)
// returns dry air mass flow rate (lbm/hr) into zone due to non-AirNet sources
{
	double amf = i.infAC * zn_dryAirMass;
	if (i.infELA > 0.f)
	{	float amfELA =
	     Top.tp_rhoDryOSh
		  * 60.f
		  * i.infELA		// user input effective leakage area (ft2), hourly variable
		  * sqrt( 			// flow is proportional to square root of
			  zn_stackc * fabs( tzlh - Top.tDbOSh) + 	//  stack coeff (see ZNR.zn_InfilSetup) * delta t
			  zn_windc * Top.windSpeedSquaredSh ); 		//  wind coeff (ditto) * wind speed ^ 2

		amf += amfELA;
	}
	return amf;
}		// ZNR::zn_NonAnIVAmf
//-----------------------------------------------------------------------------
RC ZNR::zn_AirX()		// total air exchanges etc
{
	RC rc = RCOK;

	// air motion flag; indicates mechanical air moving in space
	//   0=no, 1=yes; re evaluation of convective coefficients
	// TODO: improve zn_airMode, does not handle e.g. whole house fan 10-10
	zn_airMode = i.znModel == C_ZNMODELCH_CNE
					? cTot != 0.		// CSE: check system air flow
					: zn_qsHvac != 0.;	// CZM: assume any load means moving air

	// subhour results: total air exchange

	zn_rho = psyDenMoistAir( tz, wz);		// moist air density
	zn_dryAirMass = max( i.znVol * psyDenDryAir2( zn_rho, wz), .1);	// dry air mass, lbm
	zn_dryAirMassEff = zn_dryAirMass;			// effective dry air mass, lbm
												//   zn_MoistureBalCR() may adjust re short time steps

	// TODO: improve other infil models?
	zn_ivAirX = (zn_AnAmf() + zn_NonAnIVAmf()) / zn_dryAirMass;
												// air changes due to infil + vent
												// includes flows induced by ducts / HVAC
												//   but not those flows
	if (zn_ivAirX < 0.f)
		zn_ivAirX = 0.f;

	double amfSys = zn_ductLkI.af_amf + zn_sysAirI.af_amf;
	zn_airX = zn_ivAirX + amfSys / zn_dryAirMass;
	if (!i.zn_hcAirXIsSet)
	{	i.zn_hcAirX = zn_airX;		// note: does NOT include zn_hpwhAirX
		if (zn_airMode && !i.zn_rsi)
			i.zn_hcAirX += 4.8;		// additional "virtual" air flow for non-RSYS zones
	}
	return rc;
}		// ZNR::zn_AirX
//------------------------------------------------------------------------------
RC ZNR::zn_MoistureBalCR()		// moisture balance for convective/radiant zones
// see also znW() re CNE zone moisture balance
{
	RC rc = RCOK;

	// internal gain
	double mwIG = znLGain/PsyHCondWtr;		// latent internal gains, lbm/hr

	// non-airnet infil, lbm/hr
	double mwInf = zn_NonAnIVAmf()*Top.wOSh;

	// IZXFER (airnet) gains not including duct leakage and HVAC
	double mwAN = (1.f-zn_fVent) * zn_airNetI[ 0].af_Wmf()
		        + (zn_fVent)     * zn_airNetI[ 1].af_Wmf();

	double mw = mwIG + mwInf + mwAN		// total water vapor mass flow rate, lbm/hr
			  + zn_ductLkI.af_Wmf() + zn_sysAirI.af_Wmf();

	// TODO: HPWH moisture removal? 2-16

	zn_wCase = 0;	// debug aid
					//   0 = time constant OK, result OK
					//   1 = short time constant (steady state sln used), result OK
					//   2 = time constant OK, result limited
					//   3 = short time constant (steady state sln used), result clamped

	float f = zn_airX*Top.tp_subhrDur / i.zn_HIRatio;
	if (f > 1.f)
	{	zn_dryAirMassEff *= f;
		f = 1.f;
		zn_wCase += 1;
	}
	wz = mw * Top.tp_subhrDur/(zn_dryAirMassEff*i.zn_HIRatio) + wzls*(1. - f);

	double wSat = psyHumRat3( tz);   // maximum w air at this temp can hold
	double XLGain = 0.;	// excess gain to space due to "condensation" or "evaporation", Btuh
						// if predicted humidity ratio is physically impossible, assume
						//   magic (sensible) heat transfer
						// + = into zone (= "condensation")
	double wzClamp = bracket( double( PsyWmin), wz, wSat);
	if (wz != wzClamp)
	{	XLGain = (wz-wzClamp) * zn_dryAirMassEff * i.zn_HIRatio * PsyHCondWtr / Top.tp_subhrDur;
		znXLGain += XLGain;		// zone total excess latent gain, Btuh
		wz = wzClamp;
		zn_wCase += 2;		// limits applied
	}

	// latent gains to space
	zn_qlHvac = zn_sysAirI.af_QLat( wzls);
	if (fabs( zn_qlHvac) < .1)
		zn_qlHvac = 0.;		// drop tiny values (re report aesthetics)
	zn_qlIz = (1.f-zn_fVent) * zn_airNetI[ 0].af_QLat( wzls)
	        + (zn_fVent)     * zn_airNetI[ 1].af_QLat( wzls)
			+ zn_ductLkI.af_QLat( wzls);

	zn_twb = psyTWetBulb( tz, wz);
	zn_relHum = psyRelHum3( wz, wSat);

	// zone humidity ratio now known
	//  finalize return air state
	zn_sysAirO.as_w = wz;

#if 0
x	float relHum = psyRelHum2( tz, zn_twb);
x	if (tz > 50 && fabs( zn_relHum - relHum) > .0001)
x		printf( "hit");
#endif

#if 0 && defined( _DEBUG)
x moisture balance experiment
x generally does not balance, 5-10-2012
x	double znLDelta = zn_dryAirMass*(wz - wzls)*PsyHCondWtr;
x	double znLX = (zn_qlHvac + znLGain)*Top.tp_subhrDur;
x	if (frDiff( znLDelta, znLX) > .01)
x		printf( "Mismatch\n");
#endif

#if defined( DEBUGDUMP)
	if (DbDo( dbdZM))
	{	DbPrintf( "%s W:  mwIG=%0.3f  mwInf=%0.3f  mwAN=%0.3f  mwDuctLk=%0.3f  mwSys=%0.3f\n"
			"     tdb=%0.2f  airX=%0.3f  dryAirMass=%0.2f  XLGain=%0.2f  W=%0.6f  twb=%0.2f  rh=%0.4f\n",
			name, mwIG, mwInf, mwAN, zn_ductLkI.af_Wmf(), zn_sysAirI.af_Wmf(),
			tz, zn_airX, zn_dryAirMass, XLGain, wz, zn_twb, zn_relHum);
	}
#endif

	return rc;
}		// zn_MoisturBalCR
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
			rc = errCrit( WRN, "Zone '%s': Comfort calculation failure", name);
	}
#endif
	return rc;
}		// ZNR::zn_ComfortCR
//=============================================================================
/*virtual*/ RSYS::~RSYS()
{
	cupfree( DMPP( rs_desc));
	delete rs_pRgiHtg[0];
	delete rs_pRgiHtg[1];
	rs_pRgiHtg[0] = rs_pRgiHtg[1] = nullptr;
	delete rs_pRgiClg;
	rs_pRgiClg = nullptr;

}	// RSYS::~RSYS
//----------------------------------------------------------------------------
/*virtual*/ void RSYS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	record::Copy( pSrc);
	cupIncRef( DMPP( rs_desc));   		// incr reference counts of dm strings if nonNULL
}		// RSYS::Copy
//-------------------------------------------------------------------------------
RC RSYS::rs_CkF()
{
	ASSERT( RSYSMODES == rsmCOUNT-1);		// cndefn.h #define must be consistent
											//   with cnrecs.def enum
	int rc = RCOK;
	const char* when;
	rs_capAuxHInp = 0.f;		// insurance, see ASHP below
	if (rs_CanCool())
	{	when = strtprintf( "when rsType=%s",
			getChoiTx( RSYS_TYPE));
		
		if (rs_IsPkgRoom())
		{	rc |= requireN(when, RSYS_EER95, 0);
			rc |= disallowN(when, RSYS_SEER, RSYS_COP95, 0);
			if (!IsSet(RSYS_FANPWRC))
				rs_fanPwrC = 0.f;	// fan power included in primary by default
		}
		else
			rc |= requireN(when, RSYS_SEER, 0);


		if (!IsAusz(RSYS_CAP95))
		{	// rs_cap95 not AUTOSIZEd (altho may be expression), use as rs_capNomC default
			FldCopyIf(RSYS_CAP95, RSYS_CAPNOMC);

			if (IsSet(RSYS_CAP82))
				rc |= disallowN("when rsCap82 is given", RSYS_CAPRAT8295, 0);
			if (IsSet(RSYS_CAP115))
				rc |= disallowN("when rsCap115 is given", RSYS_CAPRAT11595, 0);
		}
		else
		{	// rsCap95 is autosize

			rc |= disallowN("when rsCap95 is AUTOSIZE",
					RSYS_CAP82, RSYS_CAP115, 0);

		}

		if (!rs_IsASHPVC())
		{
			rc |= ignoreN("when rsType is not ASHPVC (VCHP2)",
				RSYS_COPMIN05, RSYS_COPMIN17, RSYS_COPMIN35, RSYS_COPMIN47,
				RSYS_LOADFMIN05, RSYS_LOADFMIN17, RSYS_LOADFMIN47, 0);
				// cooling?
			rs_loadFMin05 = rs_loadFMin17 = rs_loadFMin47 = 1.f;
		}
		else
		{
			FldCopyIf(RSYS_LOADFMIN95, RSYS_LOADFMIN82);
			FldCopyIf(RSYS_LOADFMIN95, RSYS_LOADFMIN115);
		}

	}
	else
	{	// no cooling capability
		when = strtprintf( "when rsType=%s (cooling not available)",
			getChoiTx( RSYS_TYPE));
		rc |= disallowN( when, RSYS_SEER, RSYS_EER95,
				RSYS_CAP95, RSYS_COP95, RSYS_LOADFMIN95, RSYS_COPMIN95,
				RSYS_CAP82, RSYS_COP82, RSYS_CAPRAT8295, RSYS_LOADFMIN82, RSYS_COPMIN82,
				RSYS_CAP115, RSYS_COP115, RSYS_CAPRAT11595, RSYS_LOADFMIN115, RSYS_COPMIN115,
				RSYS_FCHG, RSYS_FSIZE, RSYS_VFPERTON, RSYS_FANPWRC, RSYS_CDC,
			    RSYS_CAPNOMC, 0);
		FldSet( RSYS_CAPNOMC, 0.f);		// insurance
	}

	rc |= rs_CkFAuxHeat();		// check aux heat inputs
	
	if (!rs_CanHeat())
	{	when = strtprintf( "when rsType=%s (heating not available)",
			getChoiTx( RSYS_TYPE));
		rc |= disallowN( when, RSYS_CAPH, RSYS_AFUE,
				RSYS_HSPF, RSYS_CAP47, RSYS_COP47,
				RSYS_CAP35, RSYS_COP35, RSYS_CAP17, RSYS_COP17,
				RSYS_ASHPLOCKOUTT,
				RSYS_FANPWRH, RSYS_CDH, RSYS_LOADFMIN47, RSYS_CAPRAT1747, 0);
		FldSet( RSYS_CAPNOMH, 0.f);		// insurance
	}
	// else
	// {	// general heating checks
	// }
		
	if (rs_IsASHP())
	{
		when = strtprintf("when rsType=%s",	getChoiTx(RSYS_TYPE));

		rs_AFUE = 0.f;		// drop possibly confusing default

		if (rs_IsASHPHydronic())
		{	// ASHPHydronic: air-to-water heat pump
			//   approximated with air-to-air model
			when = "when rsType=ASHPHydronic";
			rc |= requireN(when, RSYS_COP47, RSYS_COP17, 0);
			rc |= disallowN(when, RSYS_HSPF, RSYS_CAPH, RSYS_AFUE, RSYS_DEFROSTMODEL, 0);
			if (!IsSet(RSYS_CAP47) && !IsSet(RSYS_CAP95))
				rc |= oer("at least one of rsCap47 and rsCapC must be specified %s", when);
			if (!IsSet(RSYS_FANPWRH))
				rs_fanPwrH = 0.f;		// hydronic: no fan power
			if (!IsAusz(RSYS_CAP47))
				// rs_cap47 not AUTOSIZEd (altho may be expression), use it as rs_capNomH default
				FldCopyIf(RSYS_CAP47, RSYS_CAPNOMH);
		}
		else
		{	// ASHP other than ASHPHydronic

			rc |= disallowN(when, RSYS_CAPH, RSYS_AFUE, 0);
			if (!IsSet(RSYS_CAP47) && !IsSet(RSYS_CAP95))
				rc |= oer("at least one of rsCap47 and rsCapC must be specified %s", when);

			if (!IsAusz(RSYS_CAP47))
				// rs_cap47 not AUTOSIZEd (altho may be expression), use it as rs_capNomH default
				//   see DefaultCapNomsIf()
				FldCopyIf(RSYS_CAP47, RSYS_CAPNOMH);

			if (rs_IsPkgRoom())
			{
				rc |= disallowN(when, RSYS_HSPF, RSYS_CAP05, RSYS_COP05,
					RSYS_CAP17, RSYS_COP17, RSYS_CAP35, RSYS_COP35, 0);
				rc |= requireN(when, RSYS_COP47, 0);

				if (!IsSet(RSYS_FANPWRH))
					rs_fanPwrH = 0.f;	// fan power included in primary by default

				// other defaults derived in rs_TopRSys1()
			}
			else
			{	// ASHP (non-hydronic, non-pkgroom): only HSPF required
				//   capacities defaulted from cooling cap95
				//   COPs defaulted from HSPF
				//   if both COP47 and COP17 are specified, HSPF is not used

				rc |= requireN(when, RSYS_HSPF, 0);

				if (IsAusz(RSYS_CAP47))
					rc |= disallowN("when rsCap47 is AUTOSIZE",
						RSYS_CAP05, RSYS_CAP17, RSYS_CAP35, 0);
				else
				{	if (IsSet(RSYS_CAP17))
						rc |= disallowN("when rsCap17 is given", RSYS_CAPRAT1747, 0);
					if (IsSet(RSYS_CAP05))
						rc |= disallowN("when rsCap05 is given", RSYS_CAPRAT0547, 0);
				}

				if (!rs_IsASHPVC())
				{
					rc |= ignoreN("when rsType is not ASHPVC (VCHP2)",
						RSYS_COPMIN05, RSYS_COPMIN17, RSYS_COPMIN35, RSYS_COPMIN47,
						RSYS_LOADFMIN05, RSYS_LOADFMIN17, RSYS_LOADFMIN47, 0);
					rs_loadFMin05 = rs_loadFMin17 = rs_loadFMin47 = 1.f;
				}
				else
				{
					if (!IsSet(RSYS_LOADFMIN17))
						rs_loadFMin17 = rs_loadFMin47;
					if (!IsSet(RSYS_LOADFMIN05))
						rs_loadFMin05 = rs_loadFMin47;
				}
			}
		}
	}
	else
	{	// not ASHP of any type
		rc |= disallowN( "when rsType is not ASHP, ASHPHydronic, ASHPPkgRoom, or ASHPVC",
				RSYS_HSPF, RSYS_CAP47, RSYS_COP47,
				RSYS_CAP35, RSYS_COP35, RSYS_CAP17, RSYS_COP17,
			    RSYS_CAP05, RSYS_COP05,
				RSYS_COPMIN17, RSYS_COPMIN35, RSYS_COPMIN47,
				RSYS_ASHPLOCKOUTT, RSYS_DEFROSTMODEL, RSYS_CDH, 0);

		if (!IsSet( RSYS_AFUE))
			rs_AFUE = rs_IsElecHeat() ? 1.f : 0.9;
		if (!IsAusz( RSYS_CAPH))
			// rs_cap47 not AUTOSIZEd (altho may be expression), use it as rs_capNomH default
			FldCopyIf( RSYS_CAPH, RSYS_CAPNOMH);
	}

#if 0
x Parasitics are accumlated to RSYSRES also, 11-2021
x	// parasitics -- issue info message if no associated meter
x	if (IsSet( RSYS_PARELEC) && !IsSet( RSYS_ELECMTRI))
x		oInfo( "rsElecMtr not specified -- rsParElec has no effect");
x	if (IsSet( RSYS_PARFUEL) && !IsSet( RSYS_FUELMTRI))
x		oInfo( "rsFuelMtr not specified -- rsParFuel has no effect");
#endif

	// OAV
	if (rs_OAVType == C_RSYSOAVTYCH_NONE)
		rc |= disallowN( "when rsOAVType=None",
				RSYS_OAVRELIEFZI, RSYS_OAVTDBINLET, RSYS_OAVTDIFF,
				RSYS_OAVAVFDS, RSYS_OAVAVFMINF, RSYS_OAVFANPWR,
				0);
	else
	{	when = "when rsOAVType is given";
		rc |= requireN( when, RSYS_OAVRELIEFZI, RSYS_OAVAVFDS, 0);
		if (IsVal( RSYS_OAVTDIFF) && rs_OAVTdiff < 2.f)
			oWarn( "Dubious rsOAVTdiff (%0.2f F) may cause inadvertent vent heating."
				   "\n    rsOAVTdiff is typically >= 5 F to allow for fan heat and duct gains",
					rs_OAVTdiff);
		// rsOAVFanPwr (W/cfm): error if >5, warn if >2
		rc |= limitCheck( RSYS_OAVFANPWR, 0., 5., 0., 2.);
	}

	return rc;
}	// RSYS::rs_CkF
//-----------------------------------------------------------------------------
RC RSYS::rs_CkFCd(	// default and check Cd values
	int mode)	// rsmCOOL or rsmHEAT
// returns RCOK iff rs_CdX is acceptable
//    else !RCOK (user input out of range)
{
	RC rc = RCOK;
	if (mode == rsmCOOL)
	{	if (!IsSet(RSYS_CDC))
			rs_CdC = rs_IsVCClg() ? 0.25f : 0.f;
		else
			rc |= limitCheckFix(RSYS_CDC, 0.f, .5f);
	}
	else
	{	// not rsmCOOL, assume rsmHEAT
		if (!IsSet(RSYS_CDH))
			rs_CdH = rs_IsASHPHydronic() || rs_IsASHPPkgRoom() || rs_IsASHPVC()
				? 0.25f		// hydronic / pkgRoom: no HSPF source for default
							// ASHPVC: relationship to HSPF not known
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
	rs_effAuxH = 1.f;		// insurance
	if (!rs_CanHaveAuxHeat())
	{	rc |= disallowN("when rsType is not ASHP, ASHPHydronic, ASHPPkgRoom, or VCHP2",
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
		//   WHY: rs_capAuxH changed during rs_cap47 autosize
		//        saved value used to restore original
		rs_capAuxHInp = rs_capAuxH;
		if (IsAusz(RSYS_CAPAUXH))
			rs_capAuxH = 0.f;		// overwrite NANDLE
		if (rs_IsFuelAuxH())
		{	rs_effAuxH = rs_AFUEAuxH;
			if (!IsSet(RSYS_CTRLAUXH))
				rs_ctrlAuxH = C_AUXHEATCTRL_ALT;
		}
	}

	return rc;
}		// RSYS::rs_CkFAuxHeat
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
RC RSYS::rs_TopRSys1()		// check RSYS, initial set up for run
{
	RC rc = RCOK;

	if (!IsSet(RSYS_TDDESH))
		rs_tdDesH = rs_IsASHP() ? 30.f : 50.f;	// lower default temp rise for ASHP

	if (rs_IsASHPPkgRoom())
	{	if (!IsSet(rs_ASHPLockOutT))
			rs_ASHPLockOutT = 45.f;		// pkg room ASHP: use resistance at lower temps
		rc |= rs_SetupASHP();
	}

	if (rs_CanCool())
	{	// cooling model air flow correlations have limited validity range
		//   verify air flow 150 - 550 cfm/ton (per Proctor Engineering)
		rc |= limitCheck(RSYS_VFPERTON, 150., 550.);

		// rsFanPwrC (W/cfm): error if >5, warn if >2
		rc |= limitCheck(RSYS_FANPWRC, 0., 5., 0., 2.);

		rc |= rs_CkFCd(rsmCOOL);
		
		// default/harmonize 95 F COP
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
		{	if (!IsSet(RSYS_COP95))
				rs_COP95 = rs_EER95 / BtuperWh;
		}
		else
		{	if (IsSet(RSYS_COP95))
				rs_EER95 = rs_COP95 * BtuperWh;
			else
			{	// estimate missing EER from SEER
				//   California ACM method
				rs_EER95 = rs_SEER < 13.f ? 10.f + 0.84f * (rs_SEER - 11.5)
					: rs_SEER < 16.f ? 11.3f + 0.57f * (rs_SEER - 13)
					: 13.f;
				rs_COP95 = rs_EER95 / BtuperWh;
			}
		}
		if (rs_SEER <= rs_EER95)
			rc |= oer("rsSEER (%g) must be > rsEER (%g)", rs_SEER, rs_EER95);	
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
		rc |= limitCheck(RSYS_FANPWRH, 0., 5., 0., 2.);

		if (rs_IsASHP())
			rc |= rs_CkFCd(rsmHEAT);

		if (IsAusz(RSYS_CAP47) || IsAusz(RSYS_CAPAUXH))
		{
			rs_auszH.az_active = TRUE;	// ASHP autosizes rs_capH
								//   capacities derived in rs_AuszFinal
			rs_fxCapHAsF = 1.2f;		// working oversize factor
		}

		else if (IsAusz(RSYS_CAPH))
			// other checking?
		{
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
						ds->name, zpx->name);
				else
					oInfo( "DUCTSEG '%s' is unused (rsDSEH and rsDSEC both given)\n"
						"    and has no effect",
						ds->name);
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


	if (rc == RCOK)
		rs_SetRunConstants();

	return rc;
}		// RSYS::rs_TopRSys2
//-----------------------------------------------------------------------------
RC RSYS::rs_FazInit(		// init before autosize (once) and main sim
	int isAusz)		// TRUE = autosize, FALSE = main simulation
{
	RC rc = RCOK;

	// autosizing init
	// set handy local flag indicating autosizing now underway
	rs_isAuszH = rs_auszH.fazInit(  &rs_capH, FALSE, this, RSYS_CAPH, isAusz);
	rs_isAuszC = rs_auszC.fazInit(  &rs_cap95, FALSE, this, RSYS_CAP95, isAusz);

	if (!rs_isAuszH)
		rc |= rs_SetupCapH();

	if (!rs_isAuszC)
		rc |= rs_SetupCapC();

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
			float cfmPerFt2 = rs_IsASHP() ? 0.6f : 0.4f;
			rs_SetupCapH( rs_areaServed * cfmPerFt2);		// rs_capH derived from AVF
		}
		else
		{	// pass1B: rs_capH has latest value
			if (Top.dsDayNIt < 2)
				rs_capH = rs_auszH.az_a;	// 1st iteration cap may be 0 (see rs_pass1AtoB)
			rs_SetupCapH();
		}
	}
	else if (auszMode == rsmCOOL)
	{	if (Top.tp_pass1A)
		{	// pass1A: warmup with fixed air flow
			static const float cfmPerFt2 = 0.6f;
			rs_amfC = AVFtoAMF( rs_areaServed * cfmPerFt2);
			rs_SetupCapC( rs_areaServed * cfmPerFt2);		// rs_cap95 derived from AVF
		}
		else
		{	// pass1B: rs_cap95 has latest value
			if (Top.dsDayNIt < 2)
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
			if (Top.dsDayNIt > 1)
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
			if (Top.dsDayNIt > 1)
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

	if (rs_isAuszH)
	{	rs_capH = rs_auszH.ldPkAs1 / rs_fxCapHAsF;	// max of all design days
													//  remove working cap factor
		if (rs_IsASHP())
		{	if (IsAusz( RSYS_CAP47))
			{	// find cap47 that produces required output
				// (re-derives dependent values)
				rs_SizeASHP( rs_capH*rs_fxCapHTarg, Top.heatDsTDbO);
#if defined( _DEBUG)
				// capacity consistency check
				float capCheck = rs_CapEffASHP(Top.heatDsTDbO);
				float capExpected = Top.heatDsTDbO >= rs_ASHPLockOutT
											? rs_capH * rs_fxCapHTarg
											: rs_fanHeatH;	// fan heat only when lockout
				if (frDiff(capCheck, capExpected, 1.f) > .001f)
					oWarn("ASHP heating capacity (%.1f) autosize mismatch (expected %.1f)",
						capCheck, capExpected);
#endif
			}
			if (IsAusz( RSYS_CAPAUXH))
				rs_capAuxH = rs_capH * rs_fxCapAuxHTarg;
		}
		else
			rs_capH *= rs_fxCapHTarg;		// not ASHP: apply user's oversize factor
	}

	if (rs_isAuszC)
		rs_cap95 = rs_fxCapCTarg * rs_auszC.ldPkAs1 / rs_fxCapCAsF;
							// max of design days w/o working oversize factor
							//    apply user oversize factor

	if (rs_IsASHP())
	{	if (rs_isAuszC)
		{	if (IsAusz( RSYS_CAP47))
			{	// both autosized
				float cap47min = 0.75f * rs_cap47;
				float cap95x = rs_Cap95FromCap47ASHP( cap47min);
				if (rs_cap95 > cap95x)
				{	// cooling dominated
					// rs_cap95: per autosize
					rs_cap47 = rs_Cap47FromCap95ASHP( rs_cap95);
				}
				else
				{	// heating dominated
					rs_cap95 = cap95x;
					rs_cap47 = cap47min;
				}
			}
			// else leave rs_cap95 autosized, rs_cap47 as input
		}
		rs_SetupASHP();		// default other params as needed
		rsi->rs_cap47 = rs_cap47;	// copy back to input record

		// rs_capAuxH
		//   autosize: derive from rs_capH
		//   else: restore input value
		//         why: cap47 autosize changes rs_capAuxH
		if (IsAusz( RSYS_CAPAUXH))
			// ASHP aux heat autosize cap = full load w/ user oversize
			rs_capAuxH = rs_capH * rs_fxCapAuxHTarg;
		else
			rs_capAuxH = rs_capAuxHInp;		// value as input
		rsi->rs_capAuxH = rs_capAuxH;	// copy back to input record
	}

	// all capacities now known
	if (rs_isAuszC)
	{	float coolOver = 0.f;			// no coiling oversize (rs_fxCapC included in process)
		rs_auszC.final( this, rsi, coolOver);
	}
	if (rs_isAuszH)
	{	float heatOver = 0.f;			// ditto heating
		rs_auszH.final( this, rsi, heatOver);
	}

	// insurance: rs_capH used during ASHP autosize but s/b 0 for simulation
	if (rs_IsASHP())
		rsi->rs_capH = rs_capH = 0.f;

	rs_DefaultCapNomsIf();		// capture possible cap change(s)
								//   insurance: no rs_auszX.final() changes expected
								//      (coolOver = heatOver = 1)

}		// RSYS::rs_AuszFinal
//-----------------------------------------------------------------------------
float RSYS::rs_Cap47FromCap95ASHP(	// force ASHP htg/clg consistency (cooling dominated)
	float cap95) const	// 95 F cooling capacity
// returns cap47 consistent with cap95
//   TODO: net or gross?
{	
// note: default ratio per A. Conant
//       based on data set of split systems
	float cap47 = IsSet( RSYS_CAPRAT9547)
		? cap95 / rs_capRat9547		// user specified ratio
		: 0.98f * cap95 + 180.f;	// default
	return cap47;
}	// RSYS::rs_Cap47FromCap95ASHP
//-----------------------------------------------------------------------------
float RSYS::rs_Cap95FromCap47ASHP(		// force ASHP htg/clg consistency (heating dominated)
	float cap47) const	// 47F net heating capacity
// returns cap95 consistent with cap47
{
// note: default ratio per A. Conant
//       based on data set of split systems
	float cap95 = IsSet( RSYS_CAPRAT9547)
		? cap47 * rs_capRat9547		// user specified ratio
		: max(cap47 - 180., 0.f) / 0.98f;	// default
	return cap95;
}	// RSYS::rs_Cap95FromCap47ASHP
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
	float capH = amf * rs_tdDesH * Top.tp_airSH;
	return capH;
}		// RSYS::rs_ClgCapForAMF
//-----------------------------------------------------------------------------
float RSYS::rs_AMFForHtgCap(
	float capH) const	// heating capacity
// returns air mass flow, lbm/hr
{
	float amf = capH / (rs_tdDesH * Top.tp_airSH);
	return amf;
}		// RSYS::rs_AMFForHtgCap
//-----------------------------------------------------------------------------
int RSYS::rs_CanHaveDucts(
	int iHC) const	// 0=htg, 1=clg
// return nz iff
{
	int ret = !IsSet( iHC ? RSYS_DSEC : RSYS_DSEH);
	return ret;
}		// RSYS::rs_CanHaveDucts
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
		rs_OAVAvfDs = rs_OAVFanPwr = 0.f;
	}
	else
	{
		[[maybe_unused]] ZNR* zp = rs_GetOAVReliefZn();
		if (rs_OAVType == C_RSYSOAVTYCH_VARFLOW)
		{	// variable flow (aka "NightBreeze")

			// rs_OAVAvfDs is required

			if (!IsSet( RSYS_OAVFANPWR))
			{	// W/cfm per DEG Eq 1 (updated 11-20-2013)
				// curve is good only to 1600 cfm
				float avfX = bracket( 1.f, rs_OAVAvfDs, 1600.f);
				rs_OAVFanPwr = 44.616f * pow( 1.00175684f, avfX) / avfX;
			}
		}
		else
		{	// fixed flow (aka "SmartVent")

			// rs_OAVAvfDs is required

			if (!IsSet( RSYS_OAVFANPWR))
				rs_OAVFanPwr = rs_fanPwrC;		// same as cooling
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
		rs_fanHeatOAV = rs_OAVAvfDs * rs_OAVFanPwr * pow( af, 2.767f) * 3.413f;
#if defined( _DEBUG)
		// 9-29-2010 model for comparison
		float tMax = Wthr.d.wd_taDbPvPk;
		float afOld = 0.f;
		if (tMax > .0000001f)
		{	double d = 17.91554 - 3.67538*log( tMax);
			afOld = 1.f/max( 1.f, float( d));
		}
		float d = af - afOld;
		if (rs_fanHeatOAV > 4000.f)
			printf( "Excess OAV fan heat\n");
#endif
	}
	else if (rs_OAVType == C_RSYSOAVTYCH_FIXEDFLOW)
	{	rs_avfOAV = rs_OAVAvfDs;
		rs_fanHeatOAV = rs_avfOAV * rs_OAVFanPwr * 3.413f;	// fan power, Btuh
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
		// OAV not OK if any zone above znTC
		double fSize = rs_amfReq[ 0] > 0.		// fraction of total air that is avaailable
				? min( rs_amf / rs_amfReq[ 0], 1.)
				: 0.;
		RLUPC( ZrB, zp, rs_IsZoneServed( zp))
		{	// set zn_rsAmfSup, zn_rsAmfRet, and zn_rsFSize
			zp->zn_SetRSYSAmf( fSize, 0);
			double mCp = zp->zn_rsAmfSup*Top.tp_airSH;
			double tzx = zp->zn_TAirCR( mCp*tSup, mCp, 0.);
			if (tzx > zp->i.znTC /* && RSYS can cool */)
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
			int warnCount = ZnresB[ zp->ss].zr_GetRunTotalLI( ZNRES_IVL_SUB_NSHVENTH);
			if (warnCount <= WARNMAX)
				warn( "Zone '%s', %s: unhelpful vent heating (supply temp = %0.2f)%s",
					zp->name, Top.When( C_IVLCH_S), tSup,
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

	if (tz < i.znTH /* && RSYS could heat*/)
		return -9999;	// zone needs heat, don't OAV

	RSYS* rs = zn_GetRSYS();
#if 1	// spec change, 10-9-2013
	zn_tzsp = max( i.znTD, rs->rs_OAVTdbInlet + rs->rs_OAVTdiff);
#else
x	zn_tzsp = max( i.znTD, tSup + rs->rs_OAVTdiff);
#endif
	if (zn_tzsp > i.znTC /*&& RSYS could cool*/)
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
	// temp rise due to cooling fan, F
	// constant even if capacity is changed (e.g. during autosize)
	//  (cuz air flow is proportional to capacity via rs_vfPerTon)
	rs_fanDeltaTC = rs_fanPwrC * 3.413f / (60.f * .075f * Top.tp_airSH);

	rs_speedFMin = 1.f;		// altered iff variable speed

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

	rs_SetMTRPtrs();

}		// rs_SetRunConstants
//-----------------------------------------------------------------------------
void RSYS::rs_SetMTRPtrs()		// set runtime pointers to meters
// WHY: simplifies runtime code
{
	rs_pMtrElec = MtrB.GetAtSafe( rs_elecMtri);			// elec mtr or NULL
	rs_pMtrFuel = MtrB.GetAtSafe( rs_fuelMtri);			// fuel mtr or NULL
	rs_pMtrHeat = rs_IsElecHeat() ? rs_pMtrElec : rs_pMtrFuel;	// heat mtr or NULL
	rs_pMtrAux = rs_IsFuelAuxH() ? rs_pMtrFuel : rs_pMtrElec;
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
RC RSYS::rs_SetupCapH(
	float avfH /*=-1*/,	// heating AVF, cfm std air if known
						//   else derived from rs_capH
	int options /*=0*/)	// option bits
						//   1: assume not autosize (ignore Top.tp_autoSizing)
// sets RSYS heating members that do not vary during simulation
{
	RC rc = RCOK;
	if (avfH > 0.f)
	{	rs_amfH = AVFtoAMF( avfH);
		rs_capH = rs_amfH * Top.tp_airSH * rs_tdDesH;
	}
	else
	{	float nomCap;
		if (rs_IsASHP())
		{	if (Top.tp_autoSizing && !(options&1))
			{	nomCap = rs_capH;	// ASHP autosize derived from rs_capH
			}
			else
			{	rc = rs_SetupASHP();	// set default capacities/efficiencies, derive constants
										//  multiple calls OK (e.g. during autosizing)
										//  final call needed after autosize complete
				nomCap = rs_cap47;
			}
		}
		else
		{	// non-ASHP
			nomCap = rs_capH;
			rs_capAuxH = 0.f;	// insurance
		}
		rs_amfH = nomCap / (rs_tdDesH * Top.tp_airSH);	// nominal dry-air mass flow rate, lb/hr
		avfH = AMFtoAVF( rs_amfH);
	}
	rs_fanHeatH  = avfH * rs_fanPwrH * BtuperWh;
	return rc;

}	// RSYS::rs_SetupCapH
//-----------------------------------------------------------------------------
float RSYS::rs_FanHRtdPerTon(		// fan heat included in ratings
	float capNomTons)	// total cooling capacity, tons
						//   (used for heating also)
// returns fan heat included in ratings, Btuh
{
	float fanHRtdPerTon =
		rs_IsPkgRoom() || rs_adjForFanHt == C_NOYESCH_NO      ? 0.f		// PkgRoom or user override: no fan adjustment
	                    : rs_fan.fn_motTy == C_MOTTYCH_PSC  ? 500.f	    // PSC = Permanent Split Capacitor
	                    :									  283.f;	// BPM = Brushless Permanent Magnet (aka ECM)

	return capNomTons * fanHRtdPerTon;

}		// RSYS::rs_FanHRtdPerTon
//-----------------------------------------------------------------------------
void RSYS::rs_SetupFanC(		// derive fan cooling fan info
	float avfC /*=-1.f*/)	// cooling AVF, cfm std air if known
							//   else derived from rs_cap95
// sets rs_amfC, rs_fanHeatC, rs_fanRtdC
//   also can set rs_cap95 (if avfC > 0)
{
	float capNomTons;
	if (avfC > 0.f)
	{	rs_amfC = AVFtoAMF( avfC);
		rs_cap95 = rs_ClgCapForAMF( rs_amfC);
		capNomTons = rs_ClgCapNomTons();
	}
	else
	{	capNomTons = rs_ClgCapNomTons();
		avfC = rs_vfPerTon * capNomTons;		// avf (standard air)
		rs_amfC = AVFtoAMF( avfC);				// amf using standard air density, lbm/hr
	}

	rs_fanHeatC  = avfC * rs_fanPwrC * BtuperWh;

	rs_fanHRtdC = rs_FanHRtdPerTon( capNomTons);	// fan heat included in rated capacity, Btuh
	// rs_fanHRtdH is derived independently

}		// RSYS::rs_SetupFanC
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupCapC(		// derive constants that depend on capacity
	float avfC /*=-1*/,	// air flow if known, passed to rs_SetupFanC
	[[maybe_unused]] int options /*=0*/)	// option bits TBD
// sets RSYS cooling members that do not vary during simulation
{
	RC rc = RCOK;

	rs_SetupFanC( avfC);

	float cap95nf = -( fabs( rs_cap95) + rs_fanHRtdC);	// coil rated total capacity, Btuh

	// load = capSensT = capnf*shr - qFanOp
	// capnf = (cap95 + qfanRat)*fChg*fSize*fCondCap
	// let F = fChg*fSize*fCondCap*shr
	// load = (cap95 + qFanRat)*F - qFanOp
	// load = (cap95*(1 + fanRatX))*F - cap95*fanOpX
	// load = cap95*(F*(1+fanRatX)-fanOpX
	// cap95 = (load + fanOpX)/(F*(1 + fanRatX))

	if (fabs( cap95nf) > 10.f)
	{	// if meaningful capacity
		rs_capnfX = cap95nf * rs_fChg * rs_fSize;		// apply constant adjustment factors

		float fanHRtdRat = rs_fanHRtdC / cap95nf;		// rated fan heat ratio, (Btuh fan)/(Btuh coil)

		rs_capAdjF = rs_fChg * rs_fSize * (1.f - fanHRtdRat);
	}
	else
	{	rs_capnfX = 0.f;
		rs_capAdjF = rs_fChg * rs_fSize;
	}

	// base value for SEERnf calculation
	float inpX = 1.09f * rs_cap95 / rs_SEER - rs_fanHRtdC/3.413f;
	rs_SEERnfX = inpX > 0.f ? rs_fChg * rs_fSize * (1.09f * rs_cap95 + rs_fanHRtdC) / inpX
							: rs_SEER;

	inpX = rs_cap95 / rs_EER95 - rs_fanHRtdC / 3.413f;
	rs_EERnfX =  inpX > 0.f ? -rs_capnfX / inpX : rs_EER95;

	if (!IsSet(RSYS_CAP82))
		rs_cap82 = rs_capRat8295 * rs_cap95;

	if (!IsSet(RSYS_CAP115))
		rs_cap115 = rs_capRat11595 * rs_cap95;

#if 0
#if 0
		if (!IsSet(rs_cap115))
			rs_cap115 = rs_Cap115Default(rs_cap95, rs_cap82);
#endif
		if (!IsSet(rs_COP115))
		{
			float inp82 = rs_cap82 / rs_COP82;
			float inp95 = rs_cap95 / rs_COP95;
			rs_COP115 = rs_cap115 / rs_Inp115Default(inp95, inp82);
		}
#endif

	rc |= rs_SetupBtwxtClg();

	rs_DefaultCapNomsIf();

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
			if (rs_perfMap == C_NOYESCH_YES)
				rs_PerfMapAC();		// make performance map
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
	}
	else if (rs_mode == rsmOAV)
	{	// sensible "output" = fan power
		R.evFan = eFan;
		R.evTot += R.evFan; /* + R.evParasitic, see above */
	}

	// verify RSYSRES_IVL_SUB layout at compile time
	// fixed sequence allows array access by rs_mode (see code below)
	// rsmHEAT/rsmCOOL/rsmOAV definitions must be consistent with member sequences.
#define QZONECHK( m, oDif) static_assert( &((RSYSRES_IVL_SUB *)0)->m-&((RSYSRES_IVL_SUB *)0)->qhZoneSen == oDif, "Bad seq " #m)
	QZONECHK(qhZoneSen, (rsmHEAT - 1) * 2);
	QZONECHK(qhZoneLat, (rsmHEAT - 1) * 2 + 1);
	QZONECHK(qcZoneSen, (rsmCOOL - 1) * 2);
	QZONECHK(qcZoneLat, (rsmCOOL - 1) * 2 + 1);
	QZONECHK(qvZoneSen, (rsmOAV - 1) * 2);
	QZONECHK(qvZoneLat, (rsmOAV - 1) * 2 + 1);
	static_assert(rsmCOUNT == 4, "Bad rsm enum");
#undef QZONECHK

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
	rs_loadFLs = rs_loadF;
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

	// sensible heat ratio
	static const float ASHR = 0.0242020f;
	static const float BSHR = -0.0592153f;
	static const float CSHR = 0.0012651f;
	static const float DSHR = 0.0016375f;
	static const float ESHR = 0.f;
	static const float FSHR = 0.f;
	static const float GSHR = 0.f;
	static const float HSHR = -0.0000165f;
	static const float ISHR = 0.f;
	static const float JSHR = 0.0002021f;
	static const float KSHR = 0.f;
	static const float LSHR = 1.5085285f;

	float SHR = ASHR * rs_tdbCoilIn +
		BSHR * rs_twbCoilIn +
		CSHR * rs_tdbOut +
		DSHR * rs_vfPerTon +
		ESHR * rs_tdbCoilIn * rs_tdbOut +
		FSHR * rs_tdbCoilIn * rs_vfPerTon +
		GSHR * rs_twbCoilIn * rs_tdbOut +
		HSHR * rs_twbCoilIn * rs_vfPerTon +
		ISHR * rs_tdbOut * rs_vfPerTon +
		JSHR * rs_twbCoilIn * rs_twbCoilIn +
		KSHR / rs_vfPerTon +	// OK for 1 spd or VC (air flow and cap scale together)
		LSHR;

	// limit result
	// 0.5f = arbitrary low limit, retains sensible capacity in unrealistic cases
	rs_SHR = bracket(0.5f, SHR, 1.f);

}		// RSYS::rs_CoolingSHR
//-----------------------------------------------------------------------------
void RSYS::rs_CoolingCapF1Spd()		// capacity factor for 1 spd model
// call:   rs_tdbCoilIn = coil entering dry bulb, F
//         rs_twbCoilIn = coil entering wet bulb, F
//		   rs_tdbOut = outdoor drybulb seen by condenser, F
//         rs_vfPerTon = indoor std air cfm / ton
//         rs_bWetCoil = nz iff coil is wet
// return: rs_fCondCap = current conditions factor for total capacity
{

	// capacity: adjust rated capacity re current conditions
									//   wet coil	     dry coil
		static const float ACAP[2] = { 0.f,			 0.009483100f };
		static const float BCAP[2] = { 0.009645900f,   0.f };
		static const float CCAP[2] = { 0.002536900f,	-0.000600600f };
		static const float DCAP[2] = { 0.000171500f,	-0.000148900f };
		static const float ECAP[2] = { 0.f,			-0.000032600f };
		static const float FCAP[2] = { 0.f,			 0.000011900f };
		static const float GCAP[2] = { -0.000095900f,	 0.f };
		static const float HCAP[2] = { 0.000008180f,	 0.f };
		static const float ICAP[2] = { -0.000007550f,	-0.000005050f };
		static const float JCAP[2] = { 0.000105700f,	 0.f };
		static const float KCAP[2] = { -53.542300000f, -52.561740000f };
		static const float LCAP[2] = { 0.381567150f,   0.430751600f };

		int iDry = !(rs_wetCoil);

		rs_fCondCap = ACAP[iDry] * rs_tdbCoilIn +
			BCAP[iDry] * rs_twbCoilIn +
			CCAP[iDry] * rs_tdbOut +
			DCAP[iDry] * rs_vfPerTon +
			ECAP[iDry] * rs_tdbCoilIn * rs_tdbOut +
			FCAP[iDry] * rs_tdbCoilIn * rs_vfPerTon +
			GCAP[iDry] * rs_twbCoilIn * rs_tdbOut +
			HCAP[iDry] * rs_twbCoilIn * rs_vfPerTon +
			ICAP[iDry] * rs_tdbOut * rs_vfPerTon +
			JCAP[iDry] * rs_twbCoilIn * rs_twbCoilIn +
			KCAP[iDry] / rs_vfPerTon +
			LCAP[iDry];

}		// RSYS::rs_CoolingCapF1Spd
//----------------------------------------------------------------------------
void RSYS::rs_Cooling(
	int auszMode /*=rsmOff*/)	// active autosizing, if any
								//   rsmOFF: none
								//   rsmCOOL: cooling
								//   rsmHEAT: do not call

// call:   rs_asIn = entering state (return duct leaving state)
//		   rs_twb = entering air wet bulb, F (consistent with rs_asIn)
//         rs_tdbOut = dry-bulb temp at outdoor unit, F
//		   rs_speedF = current speed fraction
//         rs_amf = current dry air mass flow rate, lbm/hr (at rs_speedF)
// return: rs_asOut = leaving state (including fan heat)
//         rs_asSup = supply register state (after DSE/ducts if any)
//         rs_tdbCoilIn = coil entering dry bulb, F
//         rs_twbCoilIn = coil entering wet bulb, F
//         (plus intermediate correlation results)
{

#if 0 && defined( _DEBUG)
x	if (Top.jDay == 242 && Top.iHr == 14)
x		printf("\nhit");
#endif
	// init leaving conditions and find coil entering conditions
	rs_asOut = rs_asIn;
	if (rs_fan.fanTy == C_FANTYCH_BLOWTHRU)
	{	rs_asOut.as_AddQSen(rs_speedF*rs_fanHeatC, rs_amf);
		rs_twbCoilIn = rs_asOut.as_Twb();
	}
	else
		rs_twbCoilIn = rs_twbIn;		// use known value
	rs_tdbCoilIn = rs_asOut.as_tdb;


	rs_CoolingSHR();
	rs_wetCoil = rs_SHR < 1.f;

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
	}
	else if (!rs_IsVCClg())
	{	// 1 spd, not autosizing warmup, use full model
		rs_CoolingCapF1Spd();		// use 1 spd model for both 1 spd
		rs_capTotCt = rs_capnfX * rs_fCondCap;		// total capacity at current conditions, Btuh
		rs_capSenCt = rs_SHR * rs_capTotCt;			// sensible capacity at current conditions, Btuh
	}
	else
	{	// total capacities and input, Btuh
		//   include rated fan power
		//   cap values are net Btuh, <0
		float capClg, inpClg, capClgMin, inpClgMin;
		[[maybe_unused]] RC rc = rs_GetPerfBtwxt(rs_pRgiClg, rs_tdbOut,
			capClg, inpClg, capClgMin, inpClgMin);
		rs_speedFMin = capClg >= 0.f ? 1.f : capClgMin / capClg;
		float fanHAdj = rs_fanHRtdC;	// fan heat adjustment, Btuh
		// adjust for fan power (= convert to gross cooling)
		capClg += fanHAdj;
		inpClg -= fanHAdj;
		capClgMin += fanHAdj * rs_speedFMin;
		inpClgMin -= fanHAdj * rs_speedFMin;

		rs_capTotCt = capClg * rs_speedF;		// total coil capacity (= gross) at current conditions and speed, Btuh
		rs_capSenCt = rs_SHR * rs_capTotCt;		// sensible coil capacity at current conditions and speed, Btuh

		// input at current speed (Btuh)
		float inpX = rs_speedF == 1.f
			? inpClg
			: inpClgMin
			   + (inpClg - inpClgMin) * (rs_speedF - rs_speedFMin) / (1.f - rs_speedFMin);

		rs_effCt = abs( rs_capSenCt) / inpX;		// compressor-only COP
	}
	rs_capLatCt = rs_capTotCt - rs_capSenCt;	// latent, Btuh

	// leaving conditions
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
		rs_wetCoil = rs_SHR < 1.f;
	}
	if (rs_fan.fanTy != C_FANTYCH_BLOWTHRU)
		rs_asOut.as_AddQSen( rs_speedF*rs_fanHeatC, rs_amf);
	rs_asSup = rs_asOut;
	rs_SupplyDSEAndDucts( rs_asSup);
	rs_capSenNetFS = rs_capSenCt / rs_speedF + rs_fanHeatC;		// net full speed sensible capacity

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
		float capTot = rs_capSenCt + rs_capLatCt + rs_speedF*rs_fanHeatC;
		float fDiff = frDiff( hDelta, capTot);
		if (fDiff > .0002f)
			orWarn( "coil enthalpy mismatch (%.5f)", fDiff);	
	}
#endif

#if defined( RSYS_FIXEDSAT)
	rs_asOut.as_Set( 50., .001);
#endif

	if (!rs_IsVCClg())
	{
		rs_CoolingEff1Spd();	// derive rs_EERt

		rs_effCt = rs_SHR * rs_fEffC * rs_EERt / BtuperWh;	// compressor efficiency (COP)
														//   = (Btuh sensible)/(Btuh compressor)
	}

}		// RSYS::rs_Cooling
//-----------------------------------------------------------------------------
void RSYS::rs_CoolingEff1Spd()		// cooling efficiency, 1 spd model
// sets rs_fCondSEER, rs_SEERnf, rs_fCondEER, rs_EERnf, rs_EERt
{

	//          wet             dry
	static const float ASEER[2] = { 0.f,	        0.0046103f };
	static const float BSEER[2] = { -0.0202256f,	    0.f };
	static const float CSEER[2] = { 0.0236703f,	    0.0125598f };
	static const float DSEER[2] = { -0.0006638f,    -0.000512f };
	static const float ESEER[2] = { 0.f,	       -0.0000357f };
	static const float FSEER[2] = { 0.f,            0.0000105f };
	static const float GSEER[2] = { -0.0001841f,	    0.f };
	static const float HSEER[2] = { 0.0000214f,	    0.f };
	static const float ISEER[2] = { -0.00000812f,	0.f };
	static const float JSEER[2] = { 0.0002971f,	    0.f };
	static const float KSEER[2] = { -27.95672f,	    0.f };
	static const float LSEER[2] = { 0.209951063f,  -0.316172311f };

	int iDry = !rs_wetCoil;

	if (rs_tdbOut < 95.f)
	{
		float fSEERX = ASEER[iDry] * rs_tdbCoilIn +
			BSEER[iDry] * rs_twbCoilIn +
			CSEER[iDry] * rs_tdbOut +
			DSEER[iDry] * rs_vfPerTon +
			ESEER[iDry] * rs_tdbCoilIn * rs_tdbOut +
			FSEER[iDry] * rs_tdbCoilIn * rs_vfPerTon +
			GSEER[iDry] * rs_twbCoilIn * rs_tdbOut +
			HSEER[iDry] * rs_twbCoilIn * rs_vfPerTon +
			ISEER[iDry] * rs_tdbOut * rs_vfPerTon +
			JSEER[iDry] * rs_twbCoilIn * rs_twbCoilIn +
			KSEER[iDry] / rs_vfPerTon +
			LSEER[iDry];
		rs_fCondSEER = rs_fCondCap / fSEERX;
		rs_SEERnf = rs_SEERnfX * rs_fCondSEER;
	}
	else
		rs_fCondSEER = rs_SEERnf = 0.f;

	//        wet            dry
	static const float AEER[2] = { 0.f,	         0.004610300f };
	static const float BEER[2] = { -0.020225600f,	 0.f };
	static const float CEER[2] = { 0.023670300f,	 0.012559800f };
	static const float DEER[2] = { -0.000663800f, -0.000512000f };
	static const float EEER[2] = { 0.f,	        -0.000035700f };
	static const float FEER[2] = { 0.f,	         0.000010500f };
	static const float GEER[2] = { -0.000184100f,	 0.f };
	static const float HEER[2] = { 0.000021400f,	 0.f };
	static const float IEER[2] = { -0.000008120f,	 0.f };
	static const float JEER[2] = { 0.000297100f,	 0.f };
	static const float KEER[2] = { -27.956720000f,	 0.f };
	static const float LEER[2] = { 0.015003100f, -0.475306500f };

	if (rs_tdbOut > 82.f)
	{
		rs_fCondEER = 1. /
			(AEER[iDry] * rs_tdbCoilIn +
			  BEER[iDry] * rs_twbCoilIn +
			  CEER[iDry] * rs_tdbOut +
			  DEER[iDry] * rs_vfPerTon +
			  EEER[iDry] * rs_tdbCoilIn * rs_tdbOut +
			  FEER[iDry] * rs_tdbCoilIn * rs_vfPerTon +
			  GEER[iDry] * rs_twbCoilIn * rs_tdbOut +
			  HEER[iDry] * rs_twbCoilIn * rs_vfPerTon +
			  IEER[iDry] * rs_tdbOut * rs_vfPerTon +
			  JEER[iDry] * rs_twbCoilIn * rs_twbCoilIn +
			  KEER[iDry] / rs_vfPerTon +
			  LEER[iDry]);
		rs_EERnf = rs_EERnfX * rs_fCondEER * rs_fCondCap;
		rs_EERt = rs_tdbOut < 95.f
			? rs_SEERnf + (rs_tdbOut - 82.f) * (rs_EERnf - rs_SEERnf) / 13.f
			: rs_EERnf;
	}
	else
	{
		rs_EERnf = 0.f;
		rs_EERt = rs_SEERnf;
	}
}		// RSYS::rs_CoolingEff1Spd
//-----------------------------------------------------------------------------
void RSYS::rs_GetPerfClg(	// derive current cooling performance values
	float& capTot,			// total capacity, kBtuh
	float& capSen,			// sensible capacity, kBtuh
	float& pwrIn) const		// input power, kW
// Used only for reporting (not simulation) 10-2018
// All returned values include fan heat / power
{
	pwrIn = (fabs( rs_capSenCt / rs_effCt) + rs_fanHeatC)
									/ (BtuperWh * 1000.f);

	capTot = (fabs( rs_capTotCt)-rs_fanHeatC)/1000.f;
	capSen = (fabs( rs_capSenCt)-rs_fanHeatC)/1000.f;

#if 0
x	// check: derived total capacity from air stream enthalpy change
x	float capTotX = rs_amf * (rs_asOut.as_Enthalpy() - rs_asIn.as_Enthalpy());
#endif

}		// RSYS::rs_GetPerfClg
//-------------------------------------------------------------------------------
RC RSYS::rs_PerfMapAC()		// generate AC cooling performance map
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

	fprintf( f, "%s\n", rs_desc ? rs_desc : nameX);

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
					rs_tdbOut = tdbO[ idbO];
					rs_Cooling();
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
}		// RSYS::rs_PerfMapAC
//-----------------------------------------------------------------------------
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
			rs_desc ? rs_desc : nameX,
			Top.runDateTime,
			Top.tp_progVersion);

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
//-----------------------------------------------------------------------------
float RSYS::rs_PerfASHP(		// ASHP performance (simplified call)
	int ashpModel,		// 0=CSE, 1=MP, 2=ESL, 3=Cutler
						//  + 0x100: do NOT model defrost heating
						//           else model defrost if available
	float tdbOut,		// outdoor dry bulb, F
	float& COP,			// returned: compressor-only full speed COP at tdbOut
	float fanHAdj /*=0.f*/)		// fan power adjustment, Btuh
						// removed from capacity and input (before COP calc)
// returns total heating capacity (compressor + capDefrostHt), Btuh
{
	float capHt, inpHt, capDfHt, capHtMin, inpHtMin, capDfHtMin;
	COP = rs_PerfASHP2(ashpModel, tdbOut, fanHAdj,
		capHt, inpHt, capDfHt, capHtMin, inpHtMin, capDfHtMin);

	return capHt;

}	// RSYS::rs_PerfASHP
//------------------------------------------------------------------------------
float RSYS::rs_PerfASHP2(		// ASHP performance
	int ashpModel,		// 0=CSE, 1=MP, 2=ESL, 3=Cutler
						//  + 0x100: do NOT model defrost heating
						//           else model defrost if available
						// Note: variable speed supported for CSE only
	float tdbOut,		// outdoor dry bulb, F
	float fanHAdj,		// fan power adjustment, Btuh
						// removed from capacity and input (before COP calc)
	float& capHt,		// returned: compressor-only full speed heating capacity
	float& inpHt,		// returned: compressor-only full speed input power, Btuh
	float& capDfHt,		// returned: defrost full speed aux heat output, Btuh
						//    = strip/furnace output to make up compressor cooling
						//    (depends on rs_defrostModel)
	float& capHtMin,	// returned: compressor-only min speed heating capacity, Btuh
						//    (= capHt if single speed)
	float& inpHtMin,	// returned: compressor-only min speed input power, Btuh
						//    (= inpHt if single speed)
	float& capDfHtMin,	// returned: defrost min speed aux heat output, Btuh
						//    (= capDfHt if single speed)
	float COPAdjF /*=1.f*/)	// COP adjustment factor
							//   multiplies final COP result
// returns compressor-only full-speed COP (as adjusted by COPAdjF)
{
	capDfHt = capDfHtMin = 0.f;
	BOOL bDoDefrost = (ashpModel & 0x100) == 0
		           && rs_defrostModel == C_RSYSDEFROSTMODELCH_REVCYCLEAUX;
	ashpModel &= 0xff;

	if (ashpModel == 0)
	{
		int iDefrost = rs_HasDefrost() && tdbOut > 17.f && tdbOut < 45.f;	// 1 if defrost, else 0
		if (rs_IsASHPVC())
		{	// retrieve min/max cap and input at tdbOut
			//   values include fan heat/power
			/* rc = */ rs_GetPerfBtwxt(rs_pRgiHtg[ 0], tdbOut, capHt, inpHt, capHtMin, inpHtMin);
#if defined( _DEBUG)
			if (capHt < capHtMin)
				printf("\ncapHt < capHtMin");
#endif
			rs_speedFMin = capHt <= 0.f ? 1.f : capHtMin / capHt;
			if (iDefrost && bDoDefrost)
			{	// C_RSYSDEFROSTMODELCH_REVCYCLEAUX
				// include sufficient aux heat to make up for ASHP cooling
				//   = steady-state cap - integrated cap
				// limit to available aux heat capacity
				// Note: does NOT over-commit aux because compressor
				//   and aux do not run simultaneously.
				float capHtSS, capHtSSMin, inpSink;
				/* rc = */ rs_GetPerfBtwxt(rs_pRgiHtg[1], tdbOut, capHtSS, inpSink, capHtSSMin, inpSink);
				capDfHt = bracket(0.f, capHtSS - capHt, rs_capAuxH);
				capDfHtMin = bracket(0.f, capHtSSMin - capHtMin, rs_capAuxH);
			}
			capHt -= fanHAdj;
			inpHt -= fanHAdj;
			capHtMin -= fanHAdj * rs_speedFMin;
			inpHtMin -= fanHAdj * rs_speedFMin;
		}
		else
		{
			// CSE AHRI 210/240 model
			// input power and heating capacity at current conditions (Btuh)
			float tdbM17 = tdbOut - 17.f;
			inpHt = rs_inp17 + rs_ASHPInpF[iDefrost] * tdbM17 - fanHAdj;
			capHt = rs_cap17 + rs_ASHPCapF[iDefrost] * tdbM17 - fanHAdj;
			if (iDefrost && bDoDefrost)
			{	// C_RSYSDEFROSTMODELCH_REVCYCLEAUX
				// include sufficient aux heat to make up for ASHP cooling
				// limit to available aux heat capacity
				// Note: does NOT over-commit aux because compressor
				//   and aux do not run simultaneously.
				capDfHt = (rs_ASHPCapF[0] - rs_ASHPCapF[1]) * tdbM17;
				if (capDfHt > rs_capAuxH)
					capDfHt = rs_capAuxH;
			}
		}
	}
	else
	{	// DOE-2 curve fit methods
		float tdbOut2 = tdbOut * tdbOut;
		float tdbOut3 = tdbOut2 * tdbOut;

		float capNet;		// capacity with fan
		double fCap, fEIR;
		if (ashpModel == 1)
		{	// Micropas curves from Ken Nittler 5-2013)
			// Believed to be from DOE-2.1E?
			// DOE-2 curves should be applied to compressor-only
			//   but it appears the Micropas implementation applied to values with fan

			fCap = 0.302 + 0.0121*tdbOut + 0.0000759*tdbOut2 - 0.000000384*tdbOut3;
			capNet = fCap * rs_cap47;	// capacity with fan
			capHt = capNet - fanHAdj;	// capacity w/o fan

			float EIR47 = 1./(rs_HSPF*0.4f);	// EIR at 47F w/fan power
			fEIR = 1.9 - 0.025*tdbOut + 0.000134*tdbOut2 - 0.000000213*tdbOut3;
			inpHt = capNet * fEIR * EIR47 - fanHAdj;	// input w/o fan
		}
		else if (ashpModel == 2)
		{	// ESL curves from Juan-Carlos Baltazar (ESL, Texas A&M), 5-2013
			// From DOE-2.1E?

			// capacity (curve SDL-C51)
			fCap = 0.34148808 + 0.00894102*tdbOut + 0.00010787 * tdbOut2;
			capHt = fCap * rs_cap47;

			// EIR (curve SDL-C56)
			fEIR = 2.03914613 - 0.03906753*tdbOut + 0.00045617*tdbOut2 - 0.00000203*tdbOut3;

			// fan / no-fan issue

			// ESL method used in their RESNET runs
			//  approximates HSPF w/o fans
			//  however, they discovered after the fact that RESNET procedure is based on
			//    rated HSPF (with fans)
			float HSPFX = 1./((1./rs_HSPF) - .01095);
			float EIR47X = .582 * (1./(HSPFX/3.413));	// compressor-only EIR @ 47 F

			// capNet?
			inpHt = capHt * fEIR * EIR47X + fanHAdj;	// input w/o fan
		}
		else if (ashpModel == 3)
		{	// EnergyPlus model with Cutler/NREL coefficients

// Data from tables 12 and 13 of NREL/TP-5500-56354 (Cutler et al.) January 2013
//   organization: epXXX[ iHS][ iCoeff]
//      iHS: 0=cooling, 1 spd; 1=cooling, 2 spd lo; 2= cooling, 2 spd hi
//           3=heating, 1 spd; 4=heating, 2 spd lo; 5= heating, 2 spd hi
//      iCoeff: 0=a, 1=b, 2=c, 3=d, 4=e, 5=f
// capacity coefficents
static const double epCap[ 6][ 6] = {
{ 3.68637657,  -0.098352478,  0.000956357, 0.005838141, -0.0000127,  -0.000131702 },
{ 3.998418659, -0.108728222,  0.001056818, 0.007512314, -0.0000139,  -0.000164716 },
{ 3.466810106, -0.091476056,  0.000901205, 0.004163355, -0.00000919, -0.000110829 },
{ 0.566333415, -0.000744164, -0.0000103,   0.009414634,  0.0000506,  -0.00000675  },
{ 0.335690634,  0.002405123, -0.0000464,   0.013498735,  0.0000499,  -0.00000725  },
{ 0.306358843,  0.005376987, -0.0000579,   0.011645092,  0.0000591,  -0.0000203   }};
// EIR coefficients
static const double epEIR[ 6][ 6] = {
{ -3.437356399,  0.136656369, -0.001049231, -0.0079378,   0.000185435, -0.0001441   },
{ -4.282911381,  0.181023691, -0.001357391, -0.026310378, 0.000333282, -0.000197405 },
{ -3.557757517,  0.112737397, -0.000731381,  0.013184877, 0.000132645, -0.000338716 },
{  0.718398423,  0.003498178,  0.000142202, -0.005724331, 0.00014085,  -0.000215321 },
{  0.36338171,   0.013523725,  0.000258872, -0.009450269, 0.000439519, -0.000653723 },
{  0.981100941, -0.005158493,  0.000243416, -0.005274352, 0.000230742, -0.000336954 }};


			tdbOut2 = tdbOut * tdbOut;
			float tdbEnt = 70.f;
			float tdbEnt2 = tdbEnt*tdbEnt;
			float tdbX = tdbOut*tdbEnt;
			const int iHS = 3;		// heating, 1 spd

			fCap = epCap[ iHS][ 0]
			            + epCap[ iHS][ 1]*tdbEnt
					    + epCap[ iHS][ 2]*tdbEnt2
						+ epCap[ iHS][ 3]*tdbOut
						+ epCap[ iHS][ 4]*tdbOut2
						+ epCap[ iHS][ 5]*tdbX;
			capHt = fCap * rs_cap47;

			fEIR = epEIR[ iHS][ 0]
			            + epEIR[ iHS][ 1]*tdbEnt
					    + epEIR[ iHS][ 2]*tdbEnt2
						+ epEIR[ iHS][ 3]*tdbOut
						+ epEIR[ iHS][ 4]*tdbOut2
						+ epEIR[ iHS][ 5]*tdbX;
			inpHt = capHt * fEIR / rs_COP47;
		}
#if defined( _DEBUG)
		else
			ASSERT( 1);		// missing case
#endif
#if 0
0		capHt = fCap * rs_cap47;
0		inpHt = capNet * fEIR * EIR47 - fanHRtd;
#endif
	}

	float COP;
	if (capHt < 0.01f || inpHt < 0.01f)
	{	COP = 0.f;
		capHt = 0.f;
	}
	else
		COP = COPAdjF * capHt / inpHt;	// compressor-only efficiency

	capHt += capDfHt;		// include defrost heat in capacity
	capHtMin += capDfHtMin;

	if (!rs_IsASHPVC())
	{	// single speed
		capHtMin = capHt;
		inpHtMin = inpHt;
		capDfHtMin = capDfHt;
	}

	return COP;
}		// RSYS::rs_PerfASHP2
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupASHP()		// set ASHP defaults and derived parameters
// caution: all required args assumed present and >0
//   call during autosize or later
// redundant calls OK
{
	RC rc = RCOK;

	// inter-default cap95 / cap47
	//   at least one is present (see rs_CkF)
	if (!IsSet( RSYS_CAP95))
		rs_cap95 = rs_Cap95FromCap47ASHP( rs_cap47);
	else if (!IsSet( RSYS_CAP47))
		rs_cap47 = rs_Cap47FromCap95ASHP( rs_cap95);

	// fan power included in heating ratings
	// derive independently of cooling even though actually the same fan
	//   WHY: CA procedures allow separate heating / cooling sizing
	// say 0 for air-to-water
	rs_fanHRtdH = rs_IsASHPHydronic()
					? 0.f
					: rs_FanHRtdPerTon( rs_cap47 / 12000.f);

	if (!IsSet( RSYS_CAP17) || rs_IsPkgRoom())
		rs_cap17 = max( rs_CapRat1747()*rs_cap47, 1.f);

	if (!IsSet( RSYS_CAP05))
		rs_cap05 = max(rs_CapRat0547() * rs_cap47, 1.f);

	if (!IsSet(RSYS_CAP35))
		rs_cap35 = rs_Cap35Default(rs_cap47, rs_cap17);

#if ASHP_COPREG == 1
	// "traditional" model
	if (!IsSet( RSYS_COP47))
		rs_COP47 = 0.3038073f * rs_HSPF - 1.984475f*rs_cap17/rs_cap47 + 2.360116f;
	if (!IsSet( RSYS_COP17))
		rs_COP17 = 0.2359355f * rs_HSPF + 1.205568f*rs_cap17/rs_cap47 - 0.1660746;
#elif ASHP_COPREG == 2
	// Revised 5-31-2013
	if (!IsSet( RSYS_COP47))
		rs_COP47 = 0.3225f * rs_HSPF + 0.9099f;
	if (!IsSet( RSYS_COP17))
		rs_COP17 = 0.2186f * rs_HSPF + 0.6734f;
#elif ASHP_COPREG == 3
	// COP/HSPF fit 6-4-2013
	if (!IsSet( RSYS_COP47))
		rs_COP47 = -0.0129 * rs_HSPF * rs_HSPF + 0.5397 * rs_HSPF;
	if (!IsSet( RSYS_COP17))
		rs_COP17 = -.00970 * rs_HSPF * rs_HSPF + 0.3805 * rs_HSPF;
#elif ASHP_COPREG == 4
	// kW per ton fit 6-4-2013
	if (!IsSet( RSYS_COP47))
	{	float kwPerTon = -0.089 * rs_HSPF + 1.7236;
		rs_COP47 = 12000.f / (kwPerTon * 3413.f);
	}
	if (!IsSet( RSYS_COP17))
	{	float kwPerTon = -0.0722 * rs_HSPF + 1.4896;
		rs_COP17 = 12000.f * rs_cap17 / (rs_cap47 * kwPerTon * 3413.f);
	}
#elif ASHP_COPREG == 5
	// force COP17 = 1.8 at HSPF = 6.8  6-4-2013
	//   same as ASHP_COPREG == 2 except for COP17 when HSPF < 8
	if (!IsSet( RSYS_COP47))
		rs_COP47 = 0.3225f * rs_HSPF + 0.9099f;
	if (!IsSet( RSYS_COP17))
		rs_COP17 = rs_HSPF < 8
					? 0.5183f * rs_HSPF - 1.7244f
			        : 0.2186f * rs_HSPF + 0.6734f;
#elif ASHP_COPREG == 6
	if (rs_IsPkgRoom())
	{	rs_COP17 = 0.6870f * rs_COP47;
		// rs_cap17 set above
	}
	else if (!rs_IsASHPHydronic())
	{	// COP47: default as per ASHP_COPREG == 2
		// COP17 / COP35: adjust so HSPF is matched
		if (!IsSet( RSYS_COP47))
			rs_COP47 = 0.3225f * rs_HSPF + 0.9099f;
		if (!IsSet( RSYS_COP17))
		{	rs_COP17 = 0.2186f * rs_HSPF + 0.6734f;
			int iTry;
			RC rc1;
			const int nTry = 40;
			for (iTry=0; iTry<nTry; iTry++)
			{	rc1 = rs_HSPFMatchASHP();	// adjust COP17 to be
											//   consistent with rs_HSPF
				if (rc1 || rs_COP17 < rs_COP47 || IsSet( RSYS_COP47))
					break;	// accept rs_COP17 if < rs_COP47 or rs_COP47 is fixed
				rs_COP47 += 0.1f;	// try again with higher rs_COP47
			}
			if ((rc1 || iTry==nTry) && !rs_isAuszH)
				rc |= err("RSYS '%s': No reasonable value found for rsCOP17 and/or rsCOP47."
					      "\n   Check rsHSPF and other heating inputs.", name);
		}
	}

#else
#error Missing ASHP COPREG case
#endif

	// setup ASHP runtime data
	//   makes slope data and Btwxt perf map
	rc |= rs_SetRunConstantsASHP();

#if 0
	// issue message if rs_capAuxH <= compressor cap
	if (!rs_isAuszH)
		rc |= rs_CheckCapAuxH();
#endif

#if defined( _DEBUG)
	// back-calc checks
	float COP;
	float capHt = rs_PerfASHP(0, 47.f, COP);
	capHt = rs_PerfASHP(0, 17.f, COP);
#endif

	return rc;
}		// RSYS::rs_SetupASHP
//----------------------------------------------------------------------------
float RSYS::rs_CapRat1747() const
// returns ratio cap17 / cap47
{
#if defined( ASHP_CAPRAT2)
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
#else
x		// revised model per eMail from Abram Conant, 5-21-2013
x		//   constant eliminated
x		//   so: cap17/cap47 cancel in COP vs. HSPF correlations (just below)
x		//   and: COPs predicted from only HSPF and cap47 do not depend on capacity
x	float capRat = 0.6280951f;
#endif
	return capRat;
}		// RSYS::rs_CapRat1747
//-----------------------------------------------------------------------------
float RSYS::rs_CapRat0547() const
// returns ratio cap05 / cap47
{
	// TODO: develop better default
	return rs_capRat0547;
}		// RSYS::rs_CapRat0547
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
float RSYS::rs_Cap05Default(		// default 5 F heating capacity
	float cap47,		// 47 F heating capacity, any power units
	float cap17) const	// 17 F heating capacity
// extrapolates using 47-to-17 line
// returns 5 F heating capacity, consistent units
{
	float cap05 = cap17 + (cap17-cap47)* (5.f-47.f)/(17.f-47.f);
	return cap05;
}	// RSYS::rs_Cap05Default
//-----------------------------------------------------------------------------
float RSYS::rs_Cap115Default(		// default 115 F (total) cooling capacity
	float cap95,		// 95 F cooling capacity, any power units
	float cap82) const	// 82 F cooling capacity
// method: extrapolation using 82 - 95 slope
// returns 115 F cooling capacity, consistent units
{
	float cap115 = cap82 + (33.f/13.f) * (cap95 - cap82);
	return cap115;
}	// RSYS::rs_Cap115Default
//-----------------------------------------------------------------------------
float RSYS::rs_Inp115Default(		// default 115 F input power
	float inp95,		// 47 F input power, any power units
	float inp82) const	// 17 F input power
// method: extrapolation using 82 - 95 slope
// returns 115 F input power, consistent units
{
	float inp115 = inp82 + (33.f/13.f) * (inp95 - inp82);
	return inp115;
}	// RSYS::rs_Inp115Default
//-----------------------------------------------------------------------------
RC RSYS::rs_SetDefaultsClg()
{
	RC rc = RCOK;

	if (rs_IsVCClg())
	{	if (!IsSet(rs_cap115))
			rs_cap115 = rs_Cap115Default(rs_cap95, rs_cap82);
		float inp82 = rs_cap82 / rs_COP82;
		float inp95 = rs_cap95 / rs_COP95;
		if (!IsSet(rs_COP115))
			rs_COP115 = rs_cap115 / rs_Inp115Default(inp95, inp82);
	}

	return rc;

}		// RSYS::rs_SetDefaultsClg()
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
RC RSYS::rs_SetRunConstantsASHP()	// finalize constant data for simulation
// uses: rs_cap/COP 47/35/17/05
// sets: rs_inp17/35/47, rs_COP35 (if needed), slopes, performance map(s),

// returns RCOK iff success
{
	RC rc = RCOK;

	// min speed defaults
	//  WHY: multiple calls possible with different rs_COP17 (at least)
	if (!IsSet(RSYS_COPMIN47))
		rs_COPMin47 = rs_COP47;
	if (!IsSet(RSYS_COPMIN17))
		rs_COPMin17 = rs_COP17;
	if (!IsSet(RSYS_COPMIN05))
		rs_COPMin05 = rs_COP05;

	rs_inp47 = rs_cap47 / max( rs_COP47, .1f);	// full speed input power, Btuh
	rs_inp17 = rs_cap17 / max( rs_COP17, .1f);

	if (!IsSet( RSYS_COP35))
	{	rs_inp35 = rs_Inp35Default( rs_inp47, rs_inp17);
		rs_COP35 = rs_cap35 / max( rs_inp35, .1f);
	}
	else
		rs_inp35 = rs_cap35 / max( rs_COP35, .1f);

	rs_ASHPCapF[ 0] = (rs_cap47-rs_cap17) / (47.f-17.f);
	rs_ASHPInpF[ 0] = (rs_inp47-rs_inp17) / (47.f-17.f);

	rs_ASHPCapF[ 1] = (rs_cap35-rs_cap17) / (35.f-17.f);
	rs_ASHPInpF[ 1] = (rs_inp35-rs_inp17) / (35.f-17.f);

	// Btwxt performance map setup
	// initially used for ASHPVC only
	float capMin35 = rs_Cap35Default(rs_CapMin47(), rs_CapMin17());

	if (!IsSet(RSYS_COPMIN35))
	{
		float inpMin47 = rs_CapMin47() / max(rs_COPMin47, .1f);
		float inpMin17 = rs_CapMin17() / max(rs_COPMin17, .1f);
		float inpMin35 = rs_Inp35Default(inpMin47, inpMin17);
		rs_COPMin35 = capMin35 / max(inpMin35, .1f);
	}

	// integrated performance (including defrost degradation)
	std::vector< VSPERFP> ppV;
	ppV.emplace_back(5.f, rs_cap05, rs_COP05, rs_CapMin05(), rs_COPMin05);
	ppV.emplace_back(17.f, rs_cap17, rs_COP17, rs_CapMin17(), rs_COPMin17);
	ppV.emplace_back(35.f, rs_cap35, rs_COP35, capMin35, rs_COPMin35);
	ppV.emplace_back(47.f, rs_cap47, rs_COP47, rs_CapMin47(), rs_COPMin47);
	rc |= rs_SetupBtwxt(rs_pRgiHtg[ 0], ppV);

	// steady-state performance (no defrost degradation)
	//   linear w/o 35 F point
	//   used iff temp in 17 - 45 F range
	ppV.clear();
	ppV.emplace_back(17.f, rs_cap17, rs_COP17, rs_CapMin17(), rs_COPMin17);
	ppV.emplace_back(47.f, rs_cap47, rs_COP47, rs_CapMin47(), rs_COPMin47);
	rc |= rs_SetupBtwxt(rs_pRgiHtg[1], ppV);

	return rc;
}		// RSYS::rs_SetRunConstantsASHP
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupBtwxtClg()
{
	RC rc = RCOK;

	// vector of performance points
	//  note cooling capacities <0
	std::vector< VSPERFP> ppV;
	ppV.emplace_back(82.f, -rs_cap82, rs_COP82, -rs_CapMin82(), rs_COPMin82);
	ppV.emplace_back(95.f, -rs_cap95, rs_COP95, -rs_CapMin95(), rs_COPMin95);
	ppV.emplace_back(115.f, -rs_cap115, rs_COP115, -rs_CapMin115(), rs_COPMin115);

	// Populate Btwxt interpolator grid data from performance points
	rc |= rs_SetupBtwxt(rs_pRgiClg, ppV);

	return rc;
}		// RSYS::rs_SetupBtwxtClg
//-----------------------------------------------------------------------------
float RSYS::rs_CapHtCurSpeedF() const	// heating cap at current speed
// valid for rs_speedFMin <= rs_speedF <= 1
// returns non-cycling heating capacity (including fan and defrost aux), Btuh
{
// cap = rs_capHtMin
//		+ (rs_capHt - rs_capHtMin) * (rs_speedF - rs_speedFMin) / (1.f - rs_speedFMin);
// and rs_speedFMin = rs_capHtMin / rs_capHt
//  thus cap reduces to ...
	return rs_capHt * rs_speedF;
}	// RSYS::rs_CapHtCurSpeedF
//-----------------------------------------------------------------------------
float RSYS::rs_CapDfHtCurSpeedF() const
{
	float capDfHt = rs_speedF == 1.f
		? rs_capDfHt
		: rs_capDfHtMin
		  + (rs_capDfHt - rs_capDfHtMin) * (rs_speedF - rs_speedFMin) / (1.f - rs_speedFMin);
	return capDfHt;
}		// RSYS::rs_capDfHtCurSpeedF()
//-----------------------------------------------------------------------------
float RSYS::rs_InpHtCurSpeedF() const
{
	float inpHt = rs_speedF == 1.f
			? rs_inpHt
			: rs_inpHtMin
			  + (rs_inpHt - rs_inpHtMin) * (rs_speedF - rs_speedFMin) / (1.f - rs_speedFMin);
	return inpHt;
}		// RSYS::rs_InpHtCurSpeedF()
//-----------------------------------------------------------------------------
RC RSYS::rs_SetupBtwxt(	// init/populate btwxt for heating runtime interpolation
	Btwxt::RegularGridInterpolator* & pRgi,		// returned: heap ptr to Btwxt interpolator object
												//   note: any prior contents deleted
	const std::vector< VSPERFP> ppV)	// performance point vector
{
	RC rc = RCOK;

	delete pRgi;		// delete prior if any

	int nD = VSPERFP::NData();
	int nPts = ppV.size();
	std::vector< double> gridODB;
	std::vector< std::vector<double>> values;
	values.resize(nD);

	for (int iPt = 0; iPt<nPts; iPt++)
	{
		const VSPERFP& pp = ppV[iPt];
		gridODB.push_back(pp.pp_temp);
		// values[iPt].resize(nD);
		for (int iD = 0; iD < nD; iD++)
		{
			values[iD].push_back(pp[iD]);
		}
	}

	// single grid variable = dry-bulb temp (CONSTANT sez "don't extrapolate")
	Btwxt::GridAxis dbtRange(gridODB, Btwxt::Method::CONSTANT);
	std::vector<Btwxt::GridAxis> dbt{ dbtRange };

	Btwxt::GriddedData perfMapHtg(dbt, values);

	pRgi = new Btwxt::RegularGridInterpolator( perfMapHtg);

#if 0
	// test code
	std::vector< double> targ{ 17. };
	auto result = (*rs_pRgiHtg)(targ);

	targ[0] = 11.;

	result = (*rs_pRgiHtg)(targ);
#endif

	return rc;

}	// RSYS::rs_SetupBtwxt
//-----------------------------------------------------------------------------
RC RSYS::rs_GetPerfBtwxt(		// retrieve performance info from btwxt map
	Btwxt::RegularGridInterpolator* pRgi,	// interpolation data
	float tdbOut,		// outdoor dry-bulb temp, F
	float& cap,			// returned: full speed capacity
	float& inp,			// returned: full speed input power, Btuh
	float& capMin,		// returned: min speed capacity, Btuh
	float& inpMin)		// returned: min speed input power, Btuh
// Note: data generally net (include fan heat/power) per setup input
// returns RCOK iff success
//    else ?
{
	RC rc = RCOK;
#if 0 && defined( _DEBUG)
	if (tdbOut > 33.f && tdbOut < 34.f)
		printf("\nCold");
#endif

	std::vector< double> targ{ tdbOut };
	auto result = (*pRgi)(targ);
	cap = result[VSPERFP::ppCAPHS];
	inp = result[VSPERFP::ppINPHS];
	capMin = result[VSPERFP::ppCAPLS];
	inpMin = result[VSPERFP::ppINPLS];
	return rc;
}	// RSYS::rs_GetPerfBtwxt
//-----------------------------------------------------------------------------
float RSYS::rs_CapEffASHP(			// performance at current conditions
	float tdbOut/*=-999.f*/,	// outdoor dry-bulb temp, F
								//   default = rs_tdbOut (weather file value or expression)
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
	if (fanHRtd < 0.f)
		fanHRtd = rs_fanHRtdH;
	if (fanHOpr < 0.f)
		fanHOpr = rs_fanHeatH;
	if (COPAdjF < 0.f)
		COPAdjF = rs_fEffH;

	if (tdbOut < rs_ASHPLockOutT)
	{	rs_effHt = 0.f;				// compressor is unavailable
		rs_capHt = rs_capHtMin = fanHOpr;	// compressor does nothing
		rs_inpHt = rs_inpHtMin = 0.f;
	}
	else
	{	rs_effHt = rs_PerfASHP2(ashpModel, tdbOut, fanHRtd, rs_capHt, rs_inpHt, rs_capDfHt,
			rs_capHtMin, rs_inpHtMin, rs_capDfHtMin, COPAdjF);
		rs_capHt += fanHOpr;
		rs_capHtMin += fanHOpr * rs_speedFMin;
	}
	return rs_CapHtCurSpeedF();
}		// RSYS::rs_CapEffASHP
//-----------------------------------------------------------------------------
float RSYS::rs_CapEffASHP2()	// performance at current conditions (no defaults)
// main simulation variant of rs_CapEffASHP() (see above)
{
	if (rs_tdbOut < rs_ASHPLockOutT)
	{	rs_effHt = 0.f;							// compressor is unavailable
		rs_capHt = rs_capHtMin = rs_fanHeatH;	// compressor does nothing
		rs_inpHt = rs_inpHtMin = 0.f;
	}
	else if (rs_capHt == 0.f)
	{	rs_effHt = rs_PerfASHP2( 0, rs_tdbOut, rs_fanHRtdH, rs_capHt, rs_inpHt, rs_capDfHt,
			rs_capHtMin, rs_inpHtMin, rs_capDfHtMin, rs_fEffH);
		// add operating fan heat/power
		rs_capHt += rs_fanHeatH;
		rs_capHtMin += rs_fanHeatH * rs_speedFMin;
	}
	return rs_CapHtCurSpeedF();
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
	pRS->rs_SetRunConstantsASHP();
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
	rs_SetRunConstantsASHP();		// ensure consistent starting point
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

	int nHrTot = 0;			// total bin hours (s/b 2250 for reg 4)
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

		rs_CapEffASHP( tDbBin, ashpModel+0x100, 0.f, 0.f, 1.f);	// derive capacity and efficiency for bin temp
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
	rs_SetupCapH( -1.f, 1);	// set derived values from rs_cap47
							//   calls rs_SetupASHP() -> sets cap17 and cap35
							//   sets rs_amfH, rs_fanHeatH
	int ashpModel = 0;
	float capHt, inpHt, capDfHt, capHtMin, inpHtMin, capDfHtMin;
	rs_PerfASHP2(ashpModel, rs_tdbOut, rs_fanHRtdH,
		capHt, inpHt, capDfHt, capHtMin, inpHtMin, capDfHtMin);
	return capHt + rs_fanHeatH;
}		// RSYS::rs_CapHtForCap47
//-------------------------------------------------------------------------------
RC RSYS::rs_SizeASHP(			// size ASHP
	float dsnLoad,	// required output, Btuh
	float tdbOut)	// outdoor dry-bulb temp, F

// sets rs_cap47 (and dependent members) such that
//   heating capacity at tdbOut matches design load

{
	rs_tdbOut = tdbOut;
	RC rc = RCOK;

	// calculate estimated cap47 ignoring defrost and fan power
	//   use standard straight line fits solved for cap47
	float r17 = min( rs_CapRat1747(), 1.f);		// cap17/cap47
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

	// finalize cap47 result using secant()
	//   fan power and defrost included
	double cap47 = cap47Est;
	double f1 = DBL_MIN;
	int ret = secant( rs_CallCalcCapHt, this, dsnLoad, 20.f,
						cap47, f1,		// x1, f1
						cap47+100.);	// x2 (f2)
	if (ret != 0)
	{	rc = err( "RSYS '%s': Cap47 for design load fail (return code=%d)", name, ret);
		cap47 = cap47Est;
	}
	rs_cap47 = cap47;		// redundant?

	rs_DefaultCapNomsIf();

	return rc;

}	// RSYS::rs_SizeASHP
//-------------------------------------------------------------------------------
void RSYS::rs_DefaultCapNomsIf()
// update nominal heating and cooling capacities
// sets rs_capNomH and rs_capNomC if not fixed by user input
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
void RSYS::rs_SetModeAndClear(		// set mode / clear prior-step results
	int rsModeNew,
	float speedF /*=1.f*/)		// new speed
// uses rs_mode to skip unneeded
{
#if defined( _DEBUG)
	if (speedF < 0.f || speedF > 1.f)
		printf("\nrs_SetModeAndClear() speedF=%0.2f", speedF);
#endif
	rs_speedF = speedF;
	rs_mode = rsModeNew;
}		// RSYS::rs_SetModeAndClear
//-----------------------------------------------------------------------------
void RSYS::rs_ClearSubhrResults(
	int options /*= 0*/)	// option bits
							//  all 0: full clear (e.g. subhr beg)
							//  1: limited clear for speed retry
{
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
	rs_loadF = rs_PLR = rs_runF = rs_speedF = rs_runFAux = rs_PLF = rs_capSenNetFS = 0.f;
	rs_outSen = rs_outLat = rs_outFan = rs_outAux = rs_outDefrost
		= rs_outSenTot = rs_inPrimary = rs_inFan = rs_inAux = rs_inDefrost = 0.;

	// heating
	rs_effHt = rs_COPHtAdj = 0.f;
	rs_capHt = rs_capHtMin = rs_capDfHt = rs_capDfHtMin = 0.f;

	// cooling
	rs_tdbCoilIn = 0.f;
	rs_twbCoilIn = 0.f;
	rs_SHR = 0.f;
	rs_fCondCap = 0.f;
	rs_capTotCt = rs_capSenCt = rs_capLatCt = 0.f;
	rs_fCondSEER = rs_fCondEER = 0.f;
	rs_SEERnf = rs_EERnf = rs_EERt = rs_effCt = 0.f;
	
}		// RSYS::rs_ClearSubhrResults
//----------------------------------------------------------------------------
int RSYS::rs_IsModeAvailable(
	int rsMode)		// desired mode (rsmHEAT or rsmCOOL)

// returns 1 iff RSYS can operate in desired mode
//               rs_amf = max amf for mode
//						  (rs_amfH, rs_amfC, or rs_amfOAV)
//						  caller modifies re rs_speedF
//         0 if not (mode fixed and different, rs_amf=0)
//        -1 if not (system off, rs_amf=0)
{
	int ret;
	if (rs_mode != rsmOFF)
		ret = rs_mode == rsMode;	// RSYS mode already known
									// OK iff desired mode is same
	else
	{
		switch (rs_modeCtrl)
		{
		case C_RSYSMODECTRLCH_HEAT:
			ret = rsMode == rsmHEAT && rs_CanHeat();
			break;

		case C_RSYSMODECTRLCH_COOL:
			ret = rsMode == rsmCOOL && rs_CanCool();
			break;

		case C_RSYSMODECTRLCH_AUTO:
			// control is auto and system is currently off
			ret = (rsMode == rsmHEAT && rs_CanHeat())
				|| (rsMode == rsmCOOL && rs_CanCool())
				|| (rsMode == rsmOAV && rs_CanOAV());
			break;

		case C_RSYSMODECTRLCH_OFF:
		default:
			ret = -1;		// off: no can do
		}
	}

	if (ret > 0)
	{
		// set mode-specific full speed air flow
		// must be set even if mode not changed
		//   due to possible speedF modifications
		rs_amf =  rsMode == rsmHEAT ? rs_amfH
				: rsMode == rsmCOOL ? rs_amfC
				: rsMode == rsmOAV  ? rs_amfOAV
				:                     0.;
		if (rs_amf < .0001)
			ret = -1;		// no air available
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
	rs_asIn = iDS > 0
				? DsR[ iDS].ds_CalcFL( rs_asRet, rs_amf)
				: rs_asRet;


#if defined( _DEBUG)
	if (rs_mode != rsmOAV
	 && (rs_asIn.as_tdb < 30. || rs_asIn.as_tdb > 110.f))
	{	if (!Top.isWarmup)
			orWarn( "Dubious asIn tdb (%0.2f)", rs_asIn.as_tdb);
		// repeat return duct calc (debug aid)
		int iDS = rs_Dsi( 1);
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
	// if success, sets rs_amf = mode-specific full speed amf
	if (rs_IsModeAvailable( rsMode) <= 0)
		return 0;		// mode not possible

	int rsAvail = 3;
	int auszMode = rs_IsAutoSizing();
	if (auszMode == rsMode)
		rsAvail = 2;		// current mode now being autosized
	else if (auszMode != rsmOFF)
		return 1;			// another mode is being autosized

	rs_amf *= speedF;		// set speed

	if (rs_mode == rsmOFF || speedF != speedFWas)		// if was off or speed has changed
	{	// first zone requesting this mode during this step
		rs_SetModeAndClear( rsMode, speedF);	// clear results from prior calc

		// determine entering air state
		//   does return duct calcs, uses rs_amf
		rs_EnteringAirState();

		if (rs_mode == rsmCOOL)
		{	++rs_calcCount[1];
			rs_Cooling(auszMode);
		}
		else if (rs_mode == rsmHEAT)
		{	++rs_calcCount[0];
			rs_asOut = rs_asIn;		// init to entering state
			if (auszMode == rsmHEAT && Top.tp_pass1A)
			{	// autosize warmup: assume fixed temp rise
				rs_asOut.as_tdb = rs_asRet.as_tdb + rs_tdDesH;
				rs_effHt = 1.;		// need nz value, else ASHP assumes compressor off
			}
			else if (rs_IsASHP())
			{
				float capHt;	// working heating capacity
				if (auszMode == rsmOFF)
				{	// ASHP autosize is based on rs_capH
					// run full model only during simulation
					capHt = rs_CapEffASHP2();
				}
				else
				{	// ASHP heat autosize
					rs_effHt = 1.f;
					capHt = rs_capHt = rs_capH;
					rs_capAuxH = rs_capH;
				}
				rs_asOut.as_AddQSen2( capHt, rs_amf);

				// auxiliary heat (ASHP only)
				//   add aux heat and apply duct losses
				if (rs_speedF == 1.f)	// skip if not full speed
				{
					[[maybe_unused]] double tdbWas = rs_asOutAux.as_tdb;
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
			}
			else
			{	// non-ASHP
				rs_effHt = rs_AFUE * rs_fEffH;
				rs_capHt = rs_capH;		// includes fan heat
				rs_asOut.as_AddQSen2( rs_capHt, rs_amf);
			}
			rs_asSup = rs_asOut;
			rs_SupplyDSEAndDucts( rs_asSup);
#if defined( RSYS_FIXEDSAT)
			rs_asSup.as_Set( 120., .001);
#endif
		}
		else
		{	// OAV: add fan heat and duct gains
			rs_asOut = rs_asIn;
			rs_asOut.as_AddQSen2( rs_fanHeatOAV, rs_amf);
			rs_asSup = rs_asOut;
			rs_SupplyDSEAndDucts( rs_asSup);
			if (rs_asSup.as_tdb > 74.f)
			{	rs_asSup = rs_asOut;
				rs_SupplyDSEAndDucts( rs_asSup);
			}
		}
	}
	// else leaving air state known

	return rsAvail;
}	// RSYS::rs_SupplyAirState
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

	// load at system, Btuh
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
			   "\n    Aux heat is not helpful.  rs_capAuxH should be increased.",
				rs_asSupAux.as_tdb, rs_asSup.as_tdb);
			// bAux = false;  no: real controls would run aux even if not helpful
		}
	}

	ZNR* zp;
	if (!bAux)
	{	// aux not available or not needed
		if (rs_IsVC() && fSize == 1. && !rs_IsAutoSizing())
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
			printf("\nHit %s", Top.dateStr);
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
			printf("\nHit %s", Top.dateStr);
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
			rs_asSupAux.as_tdb, 1. / rs_amfReq[1]);	// x2, f2
		if (ret != 0)
		{
			warn("RSYS '%s': ASHP aux heat supply temp fail (%d)", name, ret);
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
	float fxCapFullSpd = rs_fxCap[0];		// save full speed cap rat
	RC rc = RCOK;

	assert(!rs_IsAutoSizing());

	// try minimum speed
	rc |= rs_TotalAirRequestForSpeedF( rs_speedFMin);

	// note: rs_speedF > rs_speedFMin is possible
	//  rs_TotalAirRequestForSpeedF() increases speed to avoid useless supply temp

	if (rc || rs_fxCap[0] >= 1.f)
		return rc;		// error or rs_speedFMin is sufficient

	// rs_speedFMin not sufficient, search for intermediate speed

	float fxCapMinSpd = rs_fxCap[0];

	float speedFEst = rs_speedF + (1.f - rs_speedF) * (1.f - fxCapMinSpd) / (fxCapFullSpd - fxCapMinSpd);

	double speedF, f1;
#if 1
	speedF = min(1., speedFEst * 1.2);
	f1 = DBL_MIN;
#elif 0
	if (speedFEst > 0.5f * rs_speedF + 0.5f)
	{
		speedF = 1.;
		f1 = fxCapFullSpd;
	}
	else
	{
		speedF = rs_speedF;
		f1 = fxCapMinSpd;
	}

#elif 0
	speedF = rs_speedFMin;
	f1 = fxCapMinSpd;
#else
	speedF = double( 0.5f * rs_speedFMin + 0.5f);
	f1 = DBL_MIN;
#endif

#if defined( TRACEFINDSPEEDF)
	printf("\nStart speedFMin=%0.4f  fxCapMinSpd=%0.4f  fxCapFullSpd=%0.4f  speedFEst=%0.4f",
		rs_speedFMin, fxCapMinSpd, fxCapFullSpd, speedFEst);
#endif

	// search for speedF that produces fxCap = 1.
	//   this RSYS is left in the corresponding state via rc_TotalAirRequestForSpeed() calls
	//   (secant results not directly returned)
	int ret = secant([](void* pO, double& speedF) { return ((RSYS*)pO)->rs_FxCapForSpeedF(speedF); },
						this,
						1., .0005,	// target=rs_fxCap[0]=1 within .0005
						speedF, f1,	// x1, f1
						speedFEst, DBL_MIN);	// x2, f2 (not known)
	if (ret)	// if secant failed (unexpected)
#if 0
		rc |= rs_FindSpeedFForTSup(80.);
#else
		// leave RSYS is speedFEst state (= semi-decent result)
		rc |= rs_TotalAirRequestForSpeedF(speedFEst);
#endif

#if defined( TRACEFINDSPEEDF)
	if (ret)
		printf("\nrs_FindRequiredSpeedF: secant ret=%d  rsMode=%d", ret, rs_mode);
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
		setToMax(tspMaxH, zp->i.znTH);
		setToMin(tspMinC, zp->i.znTC);
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
		[[maybe_unused]] int ret = rs_SupplyAirState(rs_mode, speedF);

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
double RSYS::rs_FxCapForSpeedF(		// call-back fcn for secant method
	double& speedF)		// trial speedF (may be returned modified)
// returns required rs_fxCap[ 0] (= rs_amf / amfReq), >1 = excess capacity
{
#if defined( _DEBUG)
	double speedFWas = speedF;
	float rsSpeedFWas = rs_speedF;
#endif
	speedF = bracket(double(rs_speedFMin), speedF, 1.);
#if defined( _DEBUG)
	if (speedF != speedFWas)
		printf("\nrs_FxCapForSpeedF limit: speedFWas=%0.4f  speedF=%0.4f",
			speedFWas, speedF);
#endif

	// options: report bad supply temps iff near solution
	//   (when far from solution errors are common and not meaningful
	int arOptions = abs(rs_fxCap[0] - 1.f) < .01f;

	[[maybe_unused]] RC rc = rs_TotalAirRequestForSpeedF(float(speedF), arOptions);

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
{
	RC rc = RCOK;

	double speedFLo = rs_speedFMin;
	double speedFHi = 1.;
	double f1 = DBL_MIN;

	// search for speedF that produces fxCap = 1.
//   this RSYS is left in the corresponding state via rc_TotalAirRequestForSpeed() calls
//   (secant results not directly returned)
	[[maybe_unused]] int ret = secant([](void* pO, double& speedF) { return ((RSYS*)pO)->rs_TSupForSpeedF(speedF); },
						this,
						tSup, .005,	// target=tSup within .005 degF
						speedFLo, f1,	// x1, f1
						speedFHi, DBL_MIN);	// x2, f2

	return rc;

}		// RSYS::rs_FindSpeedFForTSup
//-----------------------------------------------------------------------------
double RSYS::rs_TSupForSpeedF(
	double& speedF)
{
#if defined( _DEBUG)
	double speedFWas = speedF;
	float rsSpeedFWas = rs_speedF;
#endif
	speedF = bracket(double(rs_speedFMin), speedF, 1.);
#if defined( _DEBUG)
	if (speedF != speedFWas)
		printf("\nrs_TSupForSpeedF limit: speedFWas=%0.4f  speedF=%0.4f",
			speedFWas, speedF);
#endif


	// rs_ClearSubhrResults(1);	// init for speed re-try
	[[maybe_unused]] int ret = rs_SupplyAirState(rs_mode, speedF);

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
			printf( "Zone '%s': Neg amf\n", zp->name);
#endif
		rsAmf += znAmf;
	}
	float ductLkXF = rs_ducts[rs_DsHC()].ductLkXF[0];
	return rsAmf / ductLkXF;
}		// RSYS::rs_AmfRequired
//----------------------------------------------------------------------------
float RSYS::rs_ACReqCap95(		// required rated capacity to meet sensible load
	float senLoad)	// sensible load, Btuh
// returns required total capacity required to meet load, Btuh
//         (adjusted to AHRI rating conditions)
{
	float cap95 = senLoad / (rs_SHR * rs_fCondCap);
	if (rs_capAdjF > 0.)
		cap95 /= rs_capAdjF;
	return cap95;
}		// RSYS::rs_ACReqCap95
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
		rs_SetModeAndClear( rsmOFF, 0.f);
		// rs_runF = 0.f;		// rs_ClearSubhrResults
	else if (rs_amf > 0.)
		// rs_SetModeAndClear() called in rs_SupplyAirState
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
		rs_outFan = rs_inFan = rs_runF * rs_speedF * rs_fanHeatC;	// all fan heat to air
		runFFan = rs_runF;
		rs_outSenTot = rs_outSen + rs_outFan;	// net cooling output, Btuh (generally < 0)

		if (!rs_IsVCClg() || rs_speedF == 1.f)
		{	rs_PLF = 1.f - rs_CdC * (1.f - rs_runF);
			rs_inPrimary = fabs(rs_outSen / max(.01f, rs_effCt * rs_PLF));		// compressor power, Btuh
		}
		else if (rs_speedF <= rs_speedFMin)
		{	// variable capacity: cycling at min speed
			rs_PLF = 1.f - rs_CdC * (1.f - rs_runF);
			rs_inPrimary = fabs(rs_outSen / max(.01f, rs_effCt * rs_PLF));

		}
		else
		{
			// variable capacity: intermediate speed
#if defined(_DEBUG)
			if (fabs(rs_runF - 1.f) > 0.001f)
				printf("\nUnexpected rs_runF");
#endif
			rs_PLF = 1.f;
			rs_inPrimary = fabs(rs_outSen / max(.01f, rs_effCt));		// compressor power, Btuh
		}

		if (rs_pMtrElec)
		{	rs_pMtrElec->H.clg += rs_inPrimary * Top.tp_subhrDur;	// compressor energy for step, Btu
			rs_pMtrElec->H.fanC += rs_inFan * Top.tp_subhrDur;
#if defined( _DEBUG)
			if (rs_pMtrElec->H.clg < .01f && rs_pMtrElec->H.fanC > .01f)
				printf("\nFanC > 0");
#endif
		}
	}
	else if (rs_mode == rsmHEAT)
	{	if (rs_IsASHP())
		{	// rs_runFAux known at this point
			if (rs_effHt == 0.f)		// if compressor is off
			{	// rs_inPrimary = 0.;
				// rs_COPHtAdj = 0.f;
				runFFan = rs_runFAux;
				rs_outFan = rs_runFAux * rs_fanHeatH;
				rs_outAux = rs_runFAux * rs_capAuxH;
				rs_runF = rs_PLF = 0.f;
				rs_capHt = rs_capHtMin = rs_capDfHt = rs_capDfHtMin = 0.f;	// clear values for reports
				// aux may be on, handled below
			}
			else
			{
				float fFanHeatPrim;		// run fraction for both fan and primary
				if (rs_runFAux > 0.f && rs_ctrlAuxH == C_AUXHEATCTRL_ALT)
				{	// aux/primary alternate: primary runs when aux does not
					rs_runF = 1.f - rs_runFAux;
					fFanHeatPrim = rs_runF;		// fan heat allocation to primary
					runFFan = 1.f;
#if defined( _DEBUG)
					if (rs_speedF != 1.f)
						printf("\nUnexpected rs_speedF = %0.3f", rs_speedF);
#endif
				}
				else
				{	runFFan = rs_runF;
					fFanHeatPrim = 1.f;
				}

				rs_capSenNetFS = rs_capHt - rs_capDfHt;		// full-speed net capacity

				double outTot = rs_runF * rs_CapHtCurSpeedF();	// total primary output (incl fan and defrost), Btuh

				rs_outAux = rs_runFAux * rs_capAuxH;
				
				rs_outFan = min( outTot+rs_outAux, runFFan * rs_speedF * rs_fanHeatH);	// fan output, Btuh
																			// insurance: limit to total output
				rs_outSen = outTot - rs_outFan*fFanHeatPrim;	// compressor sensible output (note defrost adjustment below)
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
						if (abs( rs_runFAux + rs_runF - 1.f) > 0.0001f)
							printf("\nAux alt: bad runF+runFAux");
					}
					else
					{
						if (rs_runF != 1.f || rs_speedF != 1.f)
							printf("\nAux cycle: not full speed");
					}
				}
#endif
				if (!rs_IsASHPVC() || rs_speedF == 1.f)
				{	// single speed or variable speed at max
					// variable speed can cycle at max due to C_AUXHEATCTRL_ALT
					rs_outDefrost = rs_runF * rs_capDfHt;
					rs_outSen -= rs_outDefrost;
					rs_PLF = 1.f - rs_CdH * (1.f - rs_runF);
					rs_COPHtAdj = rs_effHt * rs_PLF;	// full load efficiency modified by PLF
					rs_inPrimary = rs_outSen / rs_COPHtAdj;
					ASSERT(rs_speedF == 1.f);
				}
				else if (rs_speedF <= rs_speedFMin)
				{	// variable capacity: cycling at min speed
					rs_outDefrost = rs_runF * rs_capDfHtMin;
					rs_outSen -= rs_outDefrost;
					rs_PLF = 1.f - rs_CdH * (1.f - rs_runF);
					rs_COPHtAdj = rs_inpHtMin <= 0.f
								? 2.5f		// rs_inpHtMin can be 0 during autosize
							    : rs_fEffH * rs_capHtMin * rs_PLF / rs_inpHtMin;
					rs_inPrimary = rs_outSen / rs_COPHtAdj;
				}
				else
				{	// variable capacity: intermediate speed
					rs_outDefrost = rs_CapDfHtCurSpeedF();
					rs_outSen -= rs_outDefrost;
					rs_inPrimary = rs_InpHtCurSpeedF() / rs_fEffH;
					rs_COPHtAdj = rs_inPrimary <= 0.f
									? 2.5f		// rs_inPrimary can be 0 during autosize
									: rs_outSen / rs_inPrimary;
					rs_PLF = 1.f;
				}
#if 0
x Not maintained, 8-2020
x#undef ASHP_COMPAREMP		// define to enable Micropas comparison code for ASHP
x							//   experimental, 5-9-2013
x#if defined( ASHP_COMPAREMP) || defined( _DEBUG)
x				// DOE-2.1E curve (from Micropas)
x				float fMP = 0.0856522
x					+ 0.9388137*rs_runF - 0.1834361*pow2( rs_runF) + 0.1589702*pow3( rs_runF);
x				// Henderson "typical" AC curve (from ESL)
x				float fESL = 0.000352822
x					+ 1.19199*rs_runF - 0.246716*pow2( rs_runF) + 0.0546566*pow3( rs_runF);
x#endif
x#if defined( ASHP_COMPAREMP)
x				rs_PLF = max( .01, rs_runF / fMP);
x#else
x				rs_PLF = 1.f - rs_CdH * ( 1.f - rs_runF);
x#endif
x
x				rs_inPrimary = rs_outSen / (rs_effHt * rs_PLF);
#endif
			}

			rs_inAux = rs_outAux / rs_effAuxH;
			rs_inDefrost = rs_outDefrost / rs_effAuxH;
			if (rs_pMtrAux)
				rs_pMtrAux->H.hpBU += (rs_inAux + rs_inDefrost) * Top.tp_subhrDur;
		}
		else
		{	// non-ASHP
			rs_capSenNetFS = rs_capHt;				// net capacity, Btuh
			double outTot = rs_runF * rs_capHt;		// total output (incl fan), Btuh
			// rs_outLat = 0.;						// total latent output
			rs_outFan = min( outTot, rs_runF * rs_fanHeatH);	// fan output, Btuh
																// insurance: limit to total output
			runFFan = rs_runF;
			rs_outSen = outTot - rs_outFan;
			rs_PLF = 1.f;
			rs_inPrimary = rs_outSen / rs_effHt;
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
	{	rs_loadF = (rs_outSen + rs_outFan) / rs_capSenNetFS;
		rs_PLR = rs_znLoad[0] / rs_capSenNetFS;
#if defined( _DEBUG)
		if (rs_loadF < 0.f)
			printf("\nNeg rs_loadF");
#endif
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

	if (Top.tp_pAirNet)					// if any AirNet
		Top.tp_pAirNet->an_Calc( 0);	// find all-zone pressure balance

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

	if (Top.tp_pAirNet)
	{	Top.tp_pAirNet->an_Calc( 1);	// find pressure balance
		IZXRAT* ize;
		RLUP( IzxR, ize)
		{	if (ize->iz_IsAirNet())
				ize->iz_ZoneXfers( 1);	// accum heat flows to zone
		}
	}

#if defined( DEBUGDUMP)
	if (DbDo( dbdAIRNET|dbdIZ))
	{	ZNR* zp;
		DbPrintf("\n");
		RLUP( ZrB, zp)
			DbPrintf( "%s vent:  MCpVent=%.2f   MCpTVent=%.1f\n",
				zp->name,
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
		KIVA* ki;
		RLUP(KvR, ki)
		{
			ki->kv_Step(dur);
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
	trls = tr;					// last subhour radiant temp
	zn_relHumls = zn_relHum;	// lass subhour relative humdity

#if 0	// unused, 9-12
x	zn_dryAirMassls = zn_dryAirMass;	// last subhour dry air mass, lbm
x	zn_qsHvacls = zn_qsHvac;	// last subhour hvac sensible power
x	zn_qlHvacls = zn_qlHvac;	// last subhour hvac latent power
#endif

	zn_hcAirXls = i.zn_hcAirX;			// last subhour air exchange rate

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

	trlh = trls;				// ditto radiant temp

	zn_relHumlh = zn_relHumls;	// ditto relative humidity

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
