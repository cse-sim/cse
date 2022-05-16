// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncoil.cpp -- coil simulation routines for hvac airHandlers for CSE

// see cnztu for overall hvac and zone/terminals hvac simulation,
//     cnah for airHandler simulation (calls functions here)

// 7-95: search AUSZ for comments about autoSize loose ends.

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// AH TU ZNR
#include <MSGHANS.H>	// MH_R1310

#include <PSYCHRO.H>	// psyEnthalpy PSYCHROMAXT
#include <EXMAN.H>	// rer
#include <CUL.H>	// FsSET oer

#include <CNGUTS.H>	// Decls for this file; ZrB MDS_FLOAT


/*-------------------------------- DEFINES --------------------------------*/
#define CPW 1.0		// heat capacity of water, Btu/lb; may not be included everywhere it otta be.

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
// none


/******************************************************************************************************************************
*  HVAC: COILS for AIRHANDLERS
******************************************************************************************************************************/

//-----------------------------------------------------------------------------------------------------------------------------
// coil autoSizing stuff
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::resizeHeatCoilIf( 		// conditionally resize heat coil to given rated capacity

	float _capt, 	/* desired new capacity, stored in .captRat if coil is resized & reSetup.
			   Caller should multiply by Top.auszHiTol2 for resizing, not for restoring. */
	float _captWas )	// lower limit for size reduction (eg start subhour value).

/* returns TRUE if coil resized,
   FALSE if not autoSizing pass 1, not autoSizing this coil, or not a new larger size. */

{
	/* called from coil models to conditionally increase coil size:
	       doElecHeatCoil, doHWCoil, doGasOrOilFurnace, doAhpHeatCoil yet 7-95.
	       possibly called from cnztu.cpp:hvacIterSubhr to undo any unused size increase due to thrashing during convergence. */

	/* Note without tolerance added to coil size, observed in pass 2 a flow of 1.00014 * pass 1's
	   (an increase << simulator torances!) causing flow to run out to full sfan overrun, thus
	   doing pass 2 under other than intended conditions for ah tho zone only .1 degree below setpoint.
	   7-6-95 (that was before size increases done in pass 2). */

	if ( hcAs.resizeIf(_capt, FALSE)			// if autoSizing this coil now, if new max size, store and return TRUE
			||  hcAs.unsizeIf( max( _capt, _captWas)) )	// or if not bigger, cond'ly decrease to as little as _captWas arg
		if (ahhc.reSetup(this)==RCOK)			// resize coil to new size (fetch captRat cuz resizeIf may alter it).
			return TRUE;					// If fails, coil remains unresized.
	return FALSE;
}			// AH::resizeHeatCoilIf
//-----------------------------------------------------------------------------------------------------------------------------
RC AHHEATCOIL::reSetup( 	// re set up coil for changed total capacity (.captRat) during autoSizing

	AH *ah )		// pointer to air handler, for access to field status

{
	// callers 7-5-95: cnah1:AH::begP1aDsdIter, cnah1:AH::ah_pass1AtoB; cncoil:AH::resizeHeatCoilIf.

	switch ( Top.tp_pass1A && ah->hcAs.az_active		// during autoSize pass 1 part A, for coil being autoSized,
			 ?  C_COILTYCH_ELEC  :  coilTy )		// find load by using "electric" model.
	{
	case C_COILTYCH_GAS:
	case C_COILTYCH_OIL:		// gas or oil furnace
	case C_COILTYCH_HW:      	// hot water coil
	case C_COILTYCH_AHP:
	case C_COILTYCH_ELEC:
		break;		// electric resistance heat. nothing to do. captRat preset by caller.
	}
	return setup(ah);					// (re) initialize coil model
}			// AHHEATCOIL::reSetup
//-----------------------------------------------------------------------------------------------------------------------------
RC AHHEATCOIL::setup( 	// set up heat coil per current capacity in record -- initially or at change while autosizing
	AH *ah				// parent AH pointer
)

{
	if (ah->hcAs.az_active)   if (ISNANDLE(captRat))   return RCOK;	// nop if autoSizing NAN preset.

	RC rc{RCOK};

	if (coilTy == C_COILTYCH_AHP)
	{
		if (!ah->IsSet(AH_AHHC + AHHEATCOIL_CAP17))
		{
			cap17 = capRat1747 * captRat;
		}

		in47 = captRat / COP47;
		in17 = cap17 / COP17;

		// input power without crankcase heater
		in17c = in17 - ah->cch.p17;
		in47c = in47 - ah->cch.p47;
#if 0  // Disable C_d modications for now. Creates challenges for autosizing that still need to be resolved. 4-2022
		if (in47 > ah->cch.p47Off)
		{
			// determine eiCyc47 (Niles eInCyc47). Niles V-K-5-b.

			//Niles form: eiCyc47 = (in47 - AHR->cch.p47) * tmCycOn + AHR->cch.e47On + AHR->cch.e47Off;
			//eliminating subtraction and addition of time-on crankcase heat yeilds: eiCyc47 = in47 * tmCycOn + AHR->cch.e47Off;
			//DBL eiCyc47 =    				// energy input during one cycle of ARI 47F cycling test. Niles eInCyc47.
			//            in47 * tmCycOn 		// steady-state input (incl cch) times time-on (latter may be #define in rccn.h)
			//             + AHR->cch.e47Off;		// plus cch (crankcase heater) input during off-time (set in CCH::setup)
			//converting energy to power here & at use just below yields:
			DBL pCyc47 =    					// power input during one cycle of ARI 47F cycling test
				in47 * tmCycOn / tmCycPer		// steady-state input (incl cch) times cycle on time fraction
				+ ah->cch.p47Off;			// plus cch (crankcase heater) input during off-time (set in CCH::setup)

			// determine cdm: cycling degradation coeff modified for compr only (cch removed). Niles V-K-5-b concluded.

			//Niles form using energy for 1 47F cycle
			//cdm = (in47/eiCyc47 - cd/tmCycPer - (1. - cd)/tmCycOn)/(in47/eiCyc47 - 1./tmCycPer);
			//my form using power:
			cdm = (in47 / pCyc47 - cd - (1. - cd) * tmCycPer / tmCycOn) / (in47 / pCyc47 - 1.);  		// /0 prot needed??

			if (cdm < 0. || cdm >= 1.0)
				rc |= ah->oer((char*)MH_S0673, cdm); 	/* "cdm (%g) not in range 0 <= cdm < 1\n"
							"    (cdm is ahpCd modified internally to remove crankcase heater effects)" */
			else if (cdm > cd)
				ah->oWarn((char*)MH_S0674, cdm, cd);	/* "cdm (%g) not <= ahpCd (%g): does this mean bad input?\n"
									"    (cdm is ahpCd modified internally to remove crankcase heater effects)" */
		}
		else
		{
			cdm = cd;
		}
#else
		cdm = cd;
#endif		

		// defrost capacity. (future) should check or default somehow vs ahp cooling inputs if given?
		if (!ah->IsSet(AHHEATCOIL_DFRCAP))			// if user did not give ahpDfrCap (defrost capacity)
			dfrCap = 2 * cap17;					// default to twice 17 degree heat capacity
		dfrCap = -fabs(dfrCap);				// make defrost capacity negative internally

		// 35F heating capacity
		DBL terp35 = cap17 + (captRat - cap17) * (35. - 17.) / (47. - 17.);	// linear interp capacity @ 35: no frost/dfr efcts
		if (!ah->IsSet(AH_AHHC + AHHEATCOIL_CAP35))				// if user did not ahpCap35
			cap35 = terp35 * fd35Df;				// default to interpolated value times frost/defrost degradation factor
		else									// ahpCap35 given (not expected)
			if (cap35 > terp35)
				rc |= ah->ooer(AH_AHHC + AHHEATCOIL_CAP35, (char*)MH_S0672, cap35, terp35, cap17, captRat);
		/* "ahpCap35 (%g) cannot be > proportional value (%g) between \n"
		   "    ahpCap17 (%g) and ahhcCaptRat (%g) -- \n"
		   "    that is, frost/defrost effects cannot increase capacity." */
	}
	return rc;
}			// AHHEATCOIL::setup
//-----------------------------------------------------------------------------------------------------------------------------
/* for DX do we need a function to compute the captRat for given load,
   to call to get arg for reSetup in cnah1:AH::ah_pass1AtoB and in dx model here? 7-95. */
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::resizeCoolCoilIf(  		// conditionally resize cool coil to given rated capacity

	float _capt,   	/* negative. Stored in .captRat if coil is resized.
			   Caller should multiply by Top.auszHiTol2 for resizing, not for restoring. */
	float _captWas )	// lower limit for size reduction (eg start subhour value).

//recoded calls so caller always adds tol DELETE
//    BOO goose /*=TRUE*/ )	// FALSE to add no tolerance (caller has); normally TRUE.

/* returns TRUE if coil resized,
   FALSE if not autoSizing now, not autoSizing this coil, or size not changed (eg _capt < _captWas). */

{
	/* called from coil models to conditionally increase coil size:
	   doElecCoolCoil, doDxCoil (2), not doChwCoil yet, 7-95.
	       possibly called from cnztu.cpp:hvacIterSubhr to undo any unused size increase due to thrashing during convergence. */

	if ( ccAs.resizeIf( _capt, FALSE)			// if autoSizing this coil now, if new max size, store and return TRUE.
			||  ccAs.unsizeIf( min( _capt, _captWas)) )	// or if not bigger, cond'ly decrease to as little as _captWas arg (neg!!)
		if (ahcc.reSetup(this)==RCOK)			// resize coil to new size (fetch in case resizeIf alters it).
			return TRUE;					// If fails, coil remains unresized.
	return FALSE;
}			// AH::resizeCoolCoilIf
//-----------------------------------------------------------------------------------------------------------------------------
RC COOLCOIL::reSetup( 		// re set up coil for changed total capacity (.captRat) during autoSizing

	AH *ah )		// pointer to air handler, for access to non-coil input info and to field status array

{
	// callers 7-5-95: cnah1:AH::begP1aDsdIter, cnah1:AH::ah_pass1AtoB; cncoil:AH::resizeCoolCoilIf.

	switch ( Top.tp_pass1A && ah->ccAs.az_active		// during autoSize pass 1 part A, for coil being autoSized,
			 ?  C_COILTYCH_ELEC  :  coilTy )		// find load by using "electric" model.
	{
	case C_COILTYCH_ELEC:
		capsRat = captRat;	// "electric" model (testing artifice)
		break;			// sensible capacity is same as total capacity

	case C_COILTYCH_DX:      			// Direct eXpansion cooling coil
		capsRat = SHRRat * captRat;   	// determine sensible capacity per Sens Heat Ratio input
		break;				// dxSetup is called after switch.

	case C_COILTYCH_CHW:
		return rer( PWRN, "COOLCOIL::reSetup: cannot autoSize CHW coils.");	// insurance

		/* CHW issues: Does not use captRat. I guess use a different AUTOSIZE input and different ccAs.px.
		   size-dependent inputs: gpmDs, vfR. Is gpmDs what we autoSize?
		   Need better vfR default: per-size like DX, dynamically coupled to autoSizing sfanVfDs, and/or what?
		   setup code: menR for vfR @ sea level moved here from cncult5; need better vfR defaulting.
		   reSize: model will probably need to determine increase factor needed then call reSetup. */
	}
	return setup(ah);					// (re) initialize coil model: do dxSetup, chwSetup, etc.

}	// COOLCOIL::reSetup
//-----------------------------------------------------------------------------------------------------------------------------
RC COOLCOIL::setup( AH *ah)	// set up heat coil per current capacity in record -- initially or at change while autosizing

{
// CAUTION: at first call if resizing, captRat may contain 0 or a NAN.
	if (ah->ccAs.az_active)   if (ISNANDLE(captRat))   return RCOK;	// nop if autoSizing NAN preset.
	// setup routines must handle 0.
	RC rc = RCOK;
	switch ( Top.tp_pass1A && ah->ccAs.az_active			// during autoSize pass 1 part A, for coil being autoSized,
			 ?  C_COILTYCH_ELEC  :  coilTy )			// use "electric" model.
	{
	case C_COILTYCH_ELEC:
		capt = captRat;		// "electric" model (testing artifice)
		//caps = capsRat;			// set .capt [and .caps] mbrs (set subHrly by other coilTys)
		break;

	case C_COILTYCH_DX:
		rc = dxSetup(ah);
		break;	// set up DX coil, below

	case C_COILTYCH_CHW:
		rc = chwSetup(ah);
		break;	// set up CHW coil, below
	}
	return rc;
}			// COOLCOIL::setup
//-----------------------------------------------------------------------------------------------------------------------------
// coil models estimating runtime function for air handler
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::estCoils( 	// compute approx acheivable coil exit temp/hum without size increase or member variable change,
	// for estimating during autosizing
	DBL _ten, DBL _wen, DBL _cen, DBL _frFanOn, BOO _coilLockout,
	int cooling,		// non-0 for full power cooling, 0 for heating
	DBL &_tex, DBL &_wex )	// receive coil exit temp & (very crude) hum
// no bounds (?)

// intended to not change any member variables. Similar to subset of doCoils and all called functions.

// does not cover all cases.
// returns FALSE if cannot be determined, eg: autosize coil has 0 size; non-autosizable coil type not modeled here (AHP, CHW); etc.

{
	// to be called from chan2:AH::tsPoss when tsWant is given, for call from setTsSp1, 6-97.

	_tex = _ten;
	_wex = _wen; 			// init to output = input

	if ( !cooling				// if heat desired
			&&  CHN(ahhc.sched)==C_OFFAVAILVC_AVAIL	// and heat coil present and scheduled available
			&&  ahMode & ahHEATBIT )
	{
		if (ahhc.captRat==0 && Top.tp_sizing && hcAs.az_active)    	// if coil is being autoSized and now has 0 capacity
			return FALSE;   					// can't approximate here til given some capacity elsewhere

		// for elec, gas, oil, ahp, hw (assuming hpOn):
		if (_cen != 0)						// don't divide by 0
			_tex = _ten + ahhc.captRat * _frFanOn / _cen;		// full power exit temp
	}
	else if ( cooling					// if cooling desired
			  &&  CHN(ahcc.sched)==C_OFFAVAILVC_AVAIL		// and cool coil present and scheduled available
			  &&  ahMode & ahCOOLBIT
			  &&  !_coilLockout ) 				// and coil not locked out by full open non-integrated economizer
	{
		if (ahcc.captRat==0 && Top.tp_sizing && ccAs.az_active)	// if coil is being autosized and now has 0 capacity)
			return FALSE;   					// can't approximate here til given some capacity elsewhere
		if (ahcc.coilTy==C_COILTYCH_CHW)				// if a CHW coil (does not use captRat, is not autoSizable 10-96)
			return FALSE;   					// can't approximate here and do not expect call

		if (_cen != 0)
		{
			_cen *= (1. - ahccBypass);   				// portion of air that does not bypass coil

			_tex = _ten * ahcc.capsRat * _cen * _frFanOn / _cen;	// full-power exit temp (elec, or dx per caps last computed)
			if (ahcc.coilTy==C_COILTYCH_DX)
			{
				setToMax( _tex, ahcc.minTEvap);				// temp not below inputtable mininum

				DBL _men = _cen/(PsyShDryAir + PsyShWtrVpr * _wen);	// mass flow of entering air
				DBL capl = min( 0.f, ahcc.captRat - ahcc.capsRat);		// last computed DX coil latent power, negative
				_wex = _wen  +  capl * _frFanOn / (_men * PsyHCondWtr);	// exit humidity
				setToMax( _wex, min( _wen, .003));				// w not absurdly low (satW at freezing is .0038)
			}
			_tex = ahccBypass * _ten + (1.-ahccBypass) * _tex;	// merge coil output with bypassing air
			_wex = ahccBypass * _wen + (1.-ahccBypass) * _wex;	// ..
		}
	}
	return TRUE;
}			// AH::estCoils
//-----------------------------------------------------------------------------------------------------------------------------
// coil models runtime function for air handler
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doCoils( 		// compute tex1 and aWs that applicable coil can and will produce.

	DBL texWant, 		// desired exit temp (subject to AHP suppl rh fcc exception)
	DBL &lLim, DBL &uLim )	/* these ts bounds are narrowed per desired temp and range over which current coil behavior
   				   is valid basis for extrapolation (eg next major power changes or curve knees).
   				   Caller inits to wide temp range; extrapolators use to bound ts extraplolation. */

// addl inputs:  frFanOn, ten, wen, cmix, .   AHP when fanCycles: tsFullFlow, dtExSen.
// outputs:      (tex), tex1, (wex), aWs, coiLimited .
//	as appropriate to coil used (if any) and coil type:
//		re crankcase heater:		      ahhc.-,ahcc.frCprOn (0 if coil not used or coil not DX, AHP, ).
//		re estimating/endtests:               tPossH, tPossC, .
//		re results reporting (coilsEndSubhr): ahhc. , ahcc.capt, ahcc.tDbCnd, .

// for coil used: compare tex1 : ten
// power delivered: compute later as  (tex1 - ten) * cmix  or  (tex - ten) * cen  (should be same).

// returns TRUE if resized either coil to a larger size (6-11-97)

{
	// caller: app\cnah2:AH::pute4Fs 7-95.

	coilLimited = FALSE;			// set true if coil present and avail and can't produce texWant
	tex1 = tex = ten;				// exit temp: init to entry temp for no-coil-used cases
	aWs = wex = wen;				// exit hum rat likewise; set supply hum rat since fan does not change w.
	cen = cmix;					// coil flow is eco flow for heat coil and no-coil cases (don't leave unset)
	ahcc.wantQflag = (texWant < ten);		/* non-0 if cooling desired, irrespective of schedules, etc.
	         				   used eg in cpEstimate re conditional cpPtf++'s. */
	ahhc.qWant = 0.;				// init to no heat desired from (OFF HW) heat coil, for HEATPLANT
	//ahcc.qWant = 0.;			if member added: init to no cold desired from (OFF) cool coil
	ahcc.chwQ = 0.;				// init to no heat desired from (OFF CHW) cool coil, for COOLPLANT
	coilUsed = cuNONE;				// init to neither coil used
	ahhc.notDone = ahcc.notDone = FALSE; 	// clear both coil "need another iteration" flags, tested in iter4Fs
	//following done in coilsEndSubhr so 0'd if ah called then shut off, 10-92:
	// ahhc.pCpr = ahhc.pSh = 0.;		// init to no power input to AHP compressor & resistance heat, 10-25-92.
	// ahhc.frCprOn = 0.;			// init to say AHP compressor did not run in subhour, for cleaner probe reports.
	// cch.frCprOn = 0.;			// init to "compressor did not run" for crankcase heater 10-25-92
	BOO ret = FALSE;

	if ( texWant > ten 				// if heat desired
			&&  CHN(ahhc.sched)==C_OFFAVAILVC_AVAIL	// and heat coil present and scheduled available
			&&  ahMode & ahHEATBIT			// and ahMode allows heating (both always ok except ZN or ZN2 ts cm)
#if 1 // 10-96 for 0 autoSized coil size
			&&  ( ahhc.captRat != 0			// and either the coil has non-0 capacity (autoSize can make 0, 10-96)
				  || Top.tp_sizing && hcAs.az_active ) )   	//  or is now being autoSized
#endif
	{
		// heat coil modelling
		//cen = cmix;		set above	// for heat coil, economizer output is coil input
		//men =  if needed
		if (cen==0.f)				// if no flow
			tex = texWant;			// don't divide by 0; let exit temp be desired temp.
		else
		{
			/* ahhc.cap = ahhc.captRat		for coil types whose current cap is constant, coilsEndSubhr uses captRat.
						no cap = captRat in cncult5 setup like cool coils cuz exprs allowed: slow. */
			coilUsed = cuHEAT;				// tell coilsEndSubhr that heat coil is in use, 12-3-92
#ifdef AUTOSIZE //cnglob.h 7-95
			if (Top.tp_pass1A && hcAs.az_active)			// in autoSizing pass 1 part A, if coil is being autoSized,
				doElecHeatCoil( texWant, lLim, uLim );    	// all coil types use ELECTRIC model to get approx size
			else
#endif
				switch (ahhc.coilTy)			// cases use ten, cen, set tex.
				{
				case C_COILTYCH_GAS:								// gas furnace
				case C_COILTYCH_OIL:
					ret = doGasOrOilFurn( texWant, lLim, uLim );
					break;  	// oil furnace. fcn below.

				case C_COILTYCH_HW:
					ret = doHWCoil( texWant, lLim, uLim);
					break;		// hot water coil. fcn just below.

				case C_COILTYCH_AHP:
					ret = doAhpHeatCoil( texWant, lLim, uLim );
					break;  	/* air source heat pump, fcn below.
												   also uses tsFullFlow, dtExSen.*/
				case C_COILTYCH_ELEC:
					ret = doElecHeatCoil( texWant, lLim, uLim );
					break;  	// elec resis heat, fcn below
				}
		}
		tex1 = tex;					// copy output temp: heat coil has no bypassing air to mix
		//aWs: preset to wen: no w modelling for heat coils.
	}
#ifdef AUTOSIZE //cnglob.h 7-95
	else 						// not using heat coil. undo any autoSize size increases this subhr.
		if ( !ISNANDLE(ahhc.captRat)			// insurance
				&&  ahhc.captRat > ahhc.bCaptRat )				// if size increased: prevents call for non-autoSizable type
			resizeHeatCoilIf( ahhc.bCaptRat, ahhc.bCaptRat);	// reset heat coil size to start subhr value if larger. fcn above.
#endif

	if ( texWant < ten 					// if cooling desired
			&&  CHN(ahcc.sched)==C_OFFAVAILVC_AVAIL		// and cool coil present and scheduled available
			&&  ahMode & ahCOOLBIT				// and ahMode allows cooling (both always ok unless ZN or ZN2 ts cm)
			&&  !coilLockout 					// and coil not locked out by full open non-integrated economizer
#if 1 // 10-96 for 0 autoSize coil size results
			&&  ( ahcc.captRat != 0				// and either coil has non-0 capacity (autoSizing can leave 0, 10-96)
				  || Top.tp_sizing && ccAs.az_active 		//  or is now being autoSized
				  || ahcc.coilTy==C_COILTYCH_CHW ) )		//  or is a CHW coil (does not use captRat, is not autoSizable 10-96)
#endif
	{
		// part of air can bypass cooling coil per user input (default 0) (needs testing 5-29-92)
		cen = cmix * (1. - ahccBypass);   		// portion of air that does not bypass coil
		men = cen/(PsyShDryAir + PsyShWtrVpr*wen);	/* same flow in mass units (lb/hr), used in cool coil code.
		       					   Heat capacity (Btuh/F) / specific heat (Btuh/lb-F) = mass (lb).
							   Conversion at actual wen (not tp_refW as in Top.tp_airSH)
							   avoids hex-tex inconsistencies in (dx) coil stuff (TESTED) */
		// cool coil modelling
		if (cen==0.f)				// if no flow, don't divide by 0.
			tex = texWant;   //wex = wen;     	// exit temp and hum = entry.   wex = wen preset above.
		else
		{
			DBL texWant1 = ten + (texWant - ten)/(1. - ahccBypass);	// adjust setpoint to compensate for bypass
			coilUsed = cuCOOL;						// tell coilsEndSubhr that cool coil is in use, 12-3-92
#ifdef AUTOSIZE //cnglob.h 7-95
			if (Top.tp_pass1A && ccAs.az_active)					// in autoSizing pass 1 part A, if coil being autoSized,
				doElecCoolCoil( texWant1, texWant, lLim, uLim );    	// all coil types use "electric" model to get approx size
			else
#endif
				switch (ahcc.coilTy)					// cases use cen, men, ten, wen; set tex, wex.
				{
				case C_COILTYCH_DX:
					ret = doDxCoil( texWant1, texWant, lLim, uLim );
					break;	// fcn below in this file

				case C_COILTYCH_CHW:
					ret = doChwCoil( texWant1, texWant, lLim, uLim );
					break;	// CHilled Water coil. fcn below.

				case C_COILTYCH_ELEC:
					ret = doElecCoolCoil( texWant1, texWant, lLim, uLim );  	// "elec cooler" (testing artifice)
					break;							//   fcn below
				}
		}
		// merge coil output flow with bypassing air 5-29-92
		tex1 = ahccBypass * ten + (1.-ahccBypass) * tex;
		aWs =  ahccBypass * wen + (1.-ahccBypass) * wex;
	}
#ifdef AUTOSIZE //cnglob.h 7-95
	else 						// not using cool coil. undo any autoSize size increases this subhr.
		if ( !ISNANDLE(ahcc.captRat)			// insurance
				&&  ahcc.captRat < ahcc.bCaptRat )			// if size (NEGATIVE to cool) increased: don't call if non-autoSizable.
			resizeCoolCoilIf( ahcc.bCaptRat, ahcc.bCaptRat);	// reset cool coil size to start subhr value if larger. fcn above.
#endif

	// if neither: texWant==ten or desired coil not available:  tex1 = tex = ten, aWs = wex = wen preset at entry.

	return ret;
}				// AH::doCoils
//-----------------------------------------------------------------------------------------------------------------------------
// heating coils
//-----------------------------------------------------------------------------------------------------------------------------
float AH::heatCapThing( 	// heat coil autoSizing power request adjusments

	float captWant )	// power to meet setpoint texWant when fan on  ((texWant - ten) * cen)

// returns adjusted captWant: tolerance added, reduced to prevent excess size when fanCycles, etc.
{
// add tolerance, but not if fanCycles.
	if (!fcc)						// TESTED if (!fcc) improved fanCycles (cooling) results, 7-21-95
		captWant *= Top.auszHiTol2; 			// adjustment for target plr of 1 - half of autoSizing tolerance

	/* fanCycles autoSize problems:
	   1. always meeting fixed ahTsMn/Mx setpoint temp makes coil too big: wanna do peak at setpoint (determines cfm vs Btus);
	      off peak, use less dT (less coil power) and more frFanOn if leak/loss/eco/latent load less favorable.
	   2. cen may be for old ts -- much too large -- so need to limit increases and to undo unnec increases on later calls.
	   3. frFanOn can be stale here, 6-97.
	   Problems observed for cooling (eco, latent load); '95 solutions mimiced here for heating.
	   NOT FULLY SOLVED as of 7-22-95 as tested for cooling (see coolCapThing).
	   Encountered heat autoSize convergence and coil overcap problems 6-97 when coiling requirements made fan oversize.
	   PROGRESS made 6-97: limiting increases over prior pass 2 value, lots of fiddles here and elsewhere. */

// reduce request in proportion to time fan on: Intended to help prevent & reverse unnec increases during subhour iteration.
	// TESTED 6-97 this is good even when fan is oversize for cooling (BUG0089,90).
	captWant *= min( frFanOn, 1.);

// measure power increase really needed by requested flow increase, limit larger requests. THIS WORKS, well, pretty well.
	if ( fcc  						// if fan cycles this hour
			&&  ahhc.captRat					// don't do at first request for power cuz multiplicative
			&&  bVfDs )					// don't divide by 0
	{
		// limit power increase VS PREVIOUS ITERATATION in proportion to flow increase VS START SUBHOUR
#if 0	// historical (former TRY3)
x #define FRAC .5	// .5 better than 1. or .2, T223 (cooling) 7-22-95 <-- see coolCapThing for 7-23-95 notes; apply here if need found, rob 6-97.
x		// TESTED 6-97 1.0 made BUG0090 not converge.
x		float prop = (FRAC * sfan.vfDs/bVfDs + (1 - FRAC));	// allow only FRAC of proportional change per iter: dont overshoot.
x		//prop *= frFanOn;				// historical 7-95..6-97. TESTED 6-97 better without (with *frFanOn above).
#else	// 6-97. looks nice, not clearly better (nor clearly worse).
		float prop =  sfan.vfDs/bVfDs;	// allow only proportional change per iteration, to not overshoot.
		prop = sqrt(prop);				// allow captRat increse proportional to half of small flow
										//   increase, less for larger flow increases
#endif
		setToMin( captWant, 					// limit requested power
			  ahhc.captRat * prop );			// to current power times proportion computed just above
	}

// strongly resist increase over max actually used in preceding autosizing pass 2 (former TRY5)
	// TESTED 6-97 makes small coils even with oversize (for cool) fan, without nonconvergence. BUG0089,90.
	if (hcAs.plrPkAs)					// if a pass 2 yet completed & peak saved
		setToMin( captWant,					// limit requrest to larger of
			  max( ahhc.captRat * Top.auszHiTol2,  	//   current (possibly downsized at end pass) + tolerance,
				   hcAs.plrPkAs * hcAs.xPkAs ) ); 	//   capacity ACTUALLY USED at prior peak

	return captWant;
}			// AH::heatCapThing
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0	// above before removal of #if's, 6-17-97
x	#define TRY3	/* sqrt in heatCapThing. now implies TRIAL. seems good; little difference on BUG0090.
x			   old: with TRY4 & TRY5 in cnah2, this seemed to be a magic bullet for BUG0090. */
x
x	#undef TRIAL	// define to use all not FRAC of proporitional change in heatCapThing. 6-11-97...
x			// 6-17-97: caused BUG0090 pass 2 noncvg.
x
x	#define TRY5	// limit hc expansion per plrPkAs. Try same in cooling!
x			// YES! fixes BUG0089,90 better than anything else without ONPLR or other TRY's. 6-17-97.
x
x	// TRY2 and 4: TRY4 on is good, TRY2 makes little difference but off better. BUG0089/90. 6-17-97.
x	#define TRY4	// remove frFanOn in second limit, vs previous, in heatCapThing.
x	#undef TRY2	/* change first frFanOn limit: see heatCapThing. If kept, look at doing same in coolCapThing.
x			   Old form: Minor improvement in BUG0090; look for more significant change. */
x//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
xfloat AH::heatCapThing( 	// heat coil autoSizing power request adjusments
x
x   float captWant )	// power to meet setpoint texWant when fan on  ((texWant - ten) * cen)
x
x// returns adjusted captWant: tolerance added, reduced to prevent excess size when fanCycles, etc.
x{
x// add tolerance, but not if fanCycles.
x    if (!fcc)						// TESTED if (!fcc) improved fanCycles (cooling) results, 7-21-95
x       captWant *= Top.auszHiTol2; 			// adjustment for target plr of 1 - half of autoSizing tolerance
x
x/* fanCycles autoSize problems:
x   1. always meeting fixed ahTsMn/Mx setpoint temp makes coil too big: wanna do peak at setpoint (determines cfm vs Btus);
x      off peak, use less dT (less coil power) and more frFanOn if leak/loss/eco/latent load less favorable.
x   2. cen may be for old ts -- much too large -- so need to limit increases and to undo unnec increases on later calls.
x   Problems observed for cooling (eco, latent load); solutions mimiced here for heating.
x   NOT FULLY SOLVED as of 7-22-95 as tested for cooling (see coolCapThing). */
x
x#ifndef TRY2	// historical. Reduces power too much when cooling fan size won't let heat run at frFanOn=1.
x		// but GOOD with the frFanOn taken out of next test.
x// reduce request in proportion to time fan on: Intended to help prevent & reverse unnec increases during subhour iteration.
x    // TESTED 6-97 this is good even when fan is oversize for cooling (BUG0089,90).
x    captWant *= min( frFanOn, 1.);
x#elif 0	// improvement not significant
x*/* reduce requested increase over start subhour in proportion to time fan on:
x*   Intended to help prevent & reverse unnec increases during subhour iteration,
x*   without reliquishing all increase since frFanOn may be stale here. */
x*    setToMin(  captWant,  ahhc.bCaptRat + min(frFanOn,1.f) * (captWant - ahhc.bCaptRat));
x#elif 0 // idea not tested
x*/* reduce requested increase over current in proportion to time fan on:
x*   Intended to help prevent & reverse unnec increases during subhour iteration,
x*   without reliquishing all increase since frFanOn may be stale here. */
x*    setToMin(  captWant,  ahhc.captRat + min(frFanOn,1.f) * (captWant - ahhc.captRat));
x#else
x*   ; // try leaving this out. BUG0090: a bit better!
x#endif
x
x// measure power increase really needed by requested flow increase, limit larger requests. THIS WORKS, well, pretty well.
x#if !defined(TRIAL) && !defined(TRY3)	// TRIAL: use FRAC 1.0, as in coolCapThing, rob 6-11-97
x    #define FRAC .5	// .5 better than 1. or .2, T223 (cooling) 7-22-95 <-- see coolCapThing for 7-23-95 notes; apply here if need found, rob 6-97.
x#endif
x	if ( fcc  						// if fan cycles this hour
x     &&  ahhc.captRat					// don't do at first request for power cuz multiplicative
x     &&  bVfDs )					// don't divide by 0
x	{  // limit power increase VS PREVIOUS ITERATATION in proportion to flow increase VS START SUBHOUR
x
x       float ratio =
x#if defined(TRIAL) || defined(TRY3)
x             sfan.vfDs/bVfDs				// allow only proportional change per iteration, to not overshoot.
x#else
x             (FRAC * sfan.vfDs/bVfDs + (1 - FRAC))	// allow only FRAC of proportional change per iteration, to not overshoot.
x#endif
x#ifndef TRY4 // better without this (BUG0089,90 at least)
x*             * frFanOn;					/* & limit in proportion to fraction time fan on.
x*							   (reason for frFanOn < 1 with flow > bVfDs may be fixed 7-21-95,
x*							   but helped orginally; keep as insurance). */
x#else
x	     ;		// try omitting
x#endif
x#ifdef TRY3
x  #if 1	// some good
x       ratio = sqrt(ratio);				// limit larger change requests more
x  #elif 0 // worse than above (on BUG0090)
x  x     if (ratio > 1)
x  x        setToMin( ratio, sqrt( 1.5 * ratio - .5) );	// limit larger change requests more
x  #elif 0 // also worse
x  x     ratio = sqrt(.5*ratio+.5);
x  #endif
x#endif
x
x       setToMin( captWant, 					// limit requested power
x             ahhc.captRat * ratio );			// to current power times ratio computed just above
x	}
x
x#ifdef TRY5	// 6-17-95 this is a good one. KEEP, try same in cool.
x// strongly resist increase over max actually used in preceding autosizing pass 2.
x    // TESTED 6-97 makes small coils even with oversize (for cool) fan, without nonconvergence. BUG0089,90.
x    if (hcAs.plrPkAs)					// if a pass 2 yet completed & peak saved
x       setToMin( captWant,					// limit requrest to larger of
x             max( ahhc.captRat * Top.auszHiTol2,  	//   current (possibly downsized at end pass) + tolerance,
x                  hcAs.plrPkAs * hcAs.xPkAs ) ); 	//   capacity ACTUALLY USED at prior peak
x#endif
x    return captWant;
x
}			// AH::heatCapThing
#endif // 0
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doHWCoil(		// compute tex that HW heating coil can and will produce

	DBL texWant,			// desired coil exit temp
	DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten

// addl inputs:  ten, cen, frFanOn, hp->capF.
// outputs:      tex, coilLimited, ahhc.qWant, ahhc.q.  wex remains wen, as preset by caller.
//	re estimating/endtests:  tPossH

// returns TRUE if resized coil

{
	// called from doCoils.  Only called if texWant > ten and cen > 0 and coil scheduled available and ahhc.captRat != 0.

#ifdef AUTOSIZE
	/* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
	   May undo increases since start this subhour's iteration if not needed now,
	   eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
	BOO ret =
		resizeHeatCoilIf(   				// cond'ly adjust coil size and reSetup. above in this file.
			heatCapThing(				// add tol, limits when fanCycles, etc.
				(texWant - ten) * cen ), 	// size to meet texWant when fan on
			ahhc.bCaptRat );				// don't make smaller than this start-subhour size
#endif

	/* fanCycles modelling: since coil model involves no flow and no temp, and boiler water has lotsa heat capacity,
	   full coil avg output assumed available at any fan duty cycle. 9-22-92. */

	// .ahhc.qWant: desired average coil output (per texWant): we set for HEATPLANT to use.
	// .ahhc.q:     actual (average, not fan-on) output: we set now for HEATPLANT to use before coilsEndSubhr is called.

	HEATPLANT *hp = HpB.p + ahhc.hpi;		// heatplant serving this coil

	// or use mbr ahhc.cap if want this as divisor for plr in coilsEndSubhr.
	if (!hp->hpOn())				// if heatplant is off (turns it on if scheduled AVAIL) (cnhp.cpp)
	{
		//rWarn?
		rer( (char *)MH_R1310, 		// "AirHandler %s's heat coil is scheduled on, \n    but heatPlant %s is scheduled off."
			 name, hp->name );
		ahhc.co_capMax = 0.;				// coil capac is 0 when plant is off. fall thru to set tex, ahhc.q, coilLimited.
		// should coilLimited be TRUE when plant OFF? probably not, as not on when coil off, but unimportant now since error 9-92.
	}
	else       					// HEATPLANT is ON
#ifdef AUTOSIZE
		if (Top.tp_autoSizing)				// during autoSizing
			ahhc.co_capMax = ahhc.captRat;    			/* use full coil capacity even if HEATPLANT overloaded
						   (overloaded plant issues message in endAutoSize()). */
		else
#endif
			ahhc.co_capMax = ahhc.captRat * hp->capF;    	// coil available avg capac: capF is 1.0, reduced by HEATPLANT when overloaded.

	tPossH = ten + ahhc.co_capMax*frFanOn/cen;      	// poss full-power tex = ten + power/heat cap flow.  /0: cen==0 does not get here.
	// Used in cnah.cpp:AH::setTsSp1 to limit estimate to save iterations.

	ahhc.qWant = (texWant - ten) * cen;  	// HW coil power output needed to meet load: desired power: for HEATPLANT to use

	// q and tex
	if (ahhc.qWant > ahhc.co_capMax)			// if desired power > avail power
	{
		tex = tPossH;				// best temp we can get
		coilLimited = TRUE;			// can't get desired temp
		ahhc.q = ahhc.co_capMax;	// current average (not fan-on) output is current capacity
	}
	else    						// can get desired temp
	{
		tex = texWant;   			// limit output temp to desired
		ahhc.q = ahhc.qWant;		// output is desired output
	}

	if (RELCHANGE(ahhc.q,ahhc.qPr) > Top.relTol)	// if q used changed since hp computed
	{
		Top.hpKf++;					// say a hp is flagged
		hp->hpClf++;					/* say must CALL this hp so it can check if overall load change
       							   exceeds tolerance, and, if so, recompute. */
	}
	setToMin( uLim, texWant);  			// tex won't go above texWant
	setToMax( lLim, ten);				// nor below entering temp
	//wex: no change: heat coil does not change w.
	return ret;
}			// AH::doHWCoil
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doElecHeatCoil(		// compute tex that electric heating coil can and will produce

	DBL texWant,			// desired coil exit temp
	DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten

// addl inputs:  ten, wen, cen, frFanOn, .
// outputs:      tex, coilLimited.  wex remains wen, as preset by caller.
//	re estimating/endtests:  tPossH

// returns TRUE if resized coil

{
	// called from doCoils.  Only called if texWant > ten and cen > 0 and ahhc.captRat != 0.

#ifdef AUTOSIZE
	/* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
	   May undo increases since start this subhour's iteration if not needed now,
	   eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
	BOO ret =
		resizeHeatCoilIf(   				// cond'ly adjust coil size and reSetup. above in this file.
			heatCapThing(				// add tol, limits when fanCycles, etc.
				(texWant - ten) * cen ), 	// size to meet texWant when fan on
			ahhc.bCaptRat );				// don't make smaller than this start-subhour size
#endif

	ahhc.co_capMax = ahhc.captRat;

	tPossH = ten + ahhc.co_capMax*frFanOn/cen; 	// poss full-power tex = ten + power/heat cap flow.  /0: cen==0 does not get here.
	// Used in cnah.cpp:AH::setTsSp1 to limit estimate to save iterations.
	if (texWant > tPossH)			// if can't heat to desired temp
	{
		tex = tPossH;				// best temp we can get
		coilLimited = TRUE;  			// can't get desired temp
	}
	else    					// can get desired temp
		tex = texWant;   			// limit output temp to desired

	setToMin( uLim, texWant);  			// tex won't go above texWant
	setToMax( lLim, ten);				// nor below entering temp
	//wex: no change: heat coil does not change w.
	return ret;
}			// AH::doElecHeatCoil
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doGasOrOilFurn(		// compute tex that gas or oil furnace "coil" can and will produce

	DBL texWant,			// desired coil exit temp
	DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten

// addl inputs:  ten, wen, cen, frFanOn, .
// outputs:      tex, coilLimited.  wex remains wen, as preset by caller.
//	re estimating/endtests:  tPossH

/* calculation here currently 7-92 identical to doElecHeatCoil.
   Additional furnace calculation is done in coilsEndSubhr: flueLoss, plr, plrAv, q, p, eir,
   and general coil aux energy inputs, which are used to simulate pilot, draft fan, etc. */

/* Expect fanCycles to be on for furnace; makes no difference here but changes fan power, leakage.
   Furnace flame assumed to cycle @ full power for any part load -- makes no difference here but assumed in
   flueLoss and auxOn/Off computations in coilsEndSubhr (and thus flueLoss, auxOn, auxOff independent of fanCycles) */

// called from doCoils.  Only called if texWant > ten and cen > 0 and ahhc.captRat != 0.
{
#ifdef AUTOSIZE
	/* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
	   May undo increases since start this subhour's iteration if not needed now,
	   eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
	BOO ret =
		resizeHeatCoilIf(   				// cond'ly adjust coil size and reSetup. above in this file.
			heatCapThing(				// add tol, limits when fanCycles, etc.
				(texWant - ten) * cen ), 	// size to meet texWant when fan on
			ahhc.bCaptRat );				// don't make smaller than this start-subhour size
#endif

	ahhc.co_capMax = ahhc.captRat;
	tPossH = ten + ahhc.co_capMax*frFanOn/cen; 	// poss full-power tex = ten + power/heat cap flow.  /0: cen==0 does not get here.
	// Used in cnah.cpp:AH::setTsSp1 to limit estimate to save iterations.
	if (texWant > tPossH)			// if can't heat to desired temp
	{
		tex = tPossH;				// best temp we can get
		coilLimited = TRUE;  			// can't get desired temp
	}
	else    					// can get desired temp
		tex = texWant;   			// limit output temp to desired

	setToMin( uLim, texWant);  			// tex won't go above texWant
	setToMax( lLim, ten);				// nor below entering temp
	//wex: no change: heat coil does not change w.
	return ret;
}			// AH::doGasOrOilFurn
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doAhpHeatCoil(		// compute tex, etc that air source heat pump heating coil will produce: interface to doAhpHeat.

	DBL texWant,			// desired coil exit temp
	DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten

// addl inputs:  ten, wen, cen, frFanOn, tmix (used for AHP "indoor air temp").  when fanCycles: tsFullFlow, dtExSen.
// outputs:      tex, coilLimited.  wex remains wen, as preset by caller.
//	re estimating/endtests:  tPossH

// returns TRUE if resized coil (doesn't, so returns FALSE, 6-97)

{
	// called from doCoils.  Only called if texWant > ten and cen > 0.
#ifdef AUTOSIZE // cnglob.h 7-95.
	// autoSizing caution: tsFullFlow not set by AH::setTsSp1 during part A. And this fcn isn't called -- elec model used. 7-95.
	// And (10-96) will need to not call here from doCoils if capacity (what member?) is 0 after autoSize complete.

	/* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
		May undo increases since start this subhour's iteration if not needed now,
		eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
	BOO ret =
        resizeHeatCoilIf(   				// cond'ly adjust coil size and reSetup. above in this file.
                   heatCapThing(				// add tol, limits when fanCycles, etc.
                                 (texWant - ten) * cen ), 	// size to meet texWant when fan on
		     ahhc.bCaptRat );				// don't make smaller than this start-subhour size
#endif

	ahhc.tIa = tmix;				/* "indoor air temp" for heat pump model for adjusting cap and input:
    						   air temp entering heat pump box, before sfan (which is part of heat pump):
    						   use "tmix", after any (unexpected) return fan and economizer. */
	ahhc.qWant = (texWant - ten) * cen;		// average desired heat to meet setpoint: to member for doAhp to use
	if (fcc)					// special suppl heat limit to keep fanFrOn at 1.0 to converge with suppl heat
	{
		DBL texSupLim = tsFullFlow 		// ts to maintain full flow, conditionally set in cnah:setTsSp1.
						- dtExSen;		// compute back to exit temp, same as texWant computed in cnah.cpp.
		ahhc.qSupLim = (texSupLim - ten) * cen;	// compute heat output to produce this supply temp. cen is average flow.
	}
	else					// if fan not cycling, frFanOn stays 1.0 anyway, and tsFullFlow is not set.
		ahhc.qSupLim = ahhc.qWant;		// special limit not needed, set to regular value.
	ahhc.frFanOn = frFanOn;			// pass ah member into heat coil object member

	ahhc.doAhpHeat( coilLimited, this);		// execute heat pump heating model, sets ahhc.cap, .q, .frCprOn, .pCpr, .qSh, .pSh.

	tex = ten + ahhc.q*frFanOn/cen;		// actual exit temp (doAhp decided output level).
	tPossH = ten + ahhc.co_capMax*frFanOn/cen;  	// poss full-power tex = ten + power/heat cap flow.  /0: cen==0 does not get here.
	// Used in cnah.cpp:AH::setTsSp1 to limit estimate to save iterations.
	setToMin( uLim, texWant);  			// tex won't go above texWant
	setToMax( lLim, ten);				// nor below entering temp
	//wex: no change: heat coil does not change w.
	return ret;
}			// AH::doAhpHeatCoil
//----------------------------------------------------------------------------------------------------------------------------
DBL AHHEATCOIL::cprCapCon( FLOAT tout)   	// ahp compressor capacity as linear fcn outdoor temp:
// b4 consideration of: frost/defrost, cycling, indoor temp != 70.
// Niles CapSs, aka pCapSs. V-K-6-a.
{
	return cap17 + (captRat - cap17)*(tout - 17.)/(47. - 17.);
}
//----------------------------------------------------------------------------------------------------------------------------
void AHHEATCOIL::doAhpHeat(		// execute heat pump heating mode model

	BOO &coilLimited,		// set TRUE if ahp cannot meet load (qWant), else not changed.
	AH* ah )			// air handler: name is used in error messages.

// member runtime inputs include: .qWant: desired average output; .tIa: indoor air temp; .frFanOn;
//				      .qSupLim: limit on TOTAL OUTPUT when suppl heat in use: caller sets to keep frFanOn 1.0.
// member outputs: .cap (current capacity), .q (heat delivered), .frCprOn (fraction of subhour compressor ran),
//		       .pCpr (compressor power in), qSh (supp heat deslivered).

// does not get called if qWant or flow (thus frFanOn) is 0.

{
	// called from doAhpHeatCoil, just above

	float tDbO = Top.tDbOSh;		// fetch outdoor temp -- use subhour end interpolated value 1-95

// update lo temp compressor lockout. part of Niles V-K-6-b-4, rearranged by Rob.

	if (tDbO < tOff)       		// if too cold
		loTLockout = TRUE;		// lock out compressor: uneconomic; does it also fail to work?
	else if (tDbO >= tOn)  		// if warm enuf
		loTLockout = FALSE;		// re-enable compressor if locked out
	// else: temp in hysteresis region: loTLockout remains unchanged.

// determine capCon: steady state capacity for outdoor & indoor temps, with frost/defrost, excluding dfr & reg rh. Niles V-K-6-b.
// And frTmDfr: fraction of time defrosting, in continuous operation.  And qDfrhCon, avg heat output to dfr heater in contin op.

	float frTmDfr;			// fraction of time defrosting, this subhour. Niles tmFrDef.
	if (loTLockout)				// if compressor locked out in cold weather. part of Niles V-K-6-b-4, moved.
	{
		capCon = 0.;			// continuous output power: none when compressor locked out
		frTmDfr = 0.;			// fraction of time defrosting in continuous operation: none
	}
	else if (tDbO > tFrMx)			// outdoor temp above max frost temp, no defrosting. Niles V-K-6-b-1.
	{
		capCon = cprCapCon(tDbO);   	// continuous output power is linear extrap from cap17 & cap47. adj for tIa below.
		frTmDfr = 0.;   			// fraction of time defrosting in continuous operation: none
	}
	else if (tDbO <= tFrMn)			// below min frost temp: min defrosting, no frost. Niles V-K-6-b-3.
	{
		capCon =   cprCapCon(tDbO)*(1.-dfrFMn)	// capacity is linear xtrap from cap17/47 when running (adj for tIa below)
				   + dfrCap * dfrFMn;		// plus (negative) cooling to zone when defrosting (min time)
		frTmDfr = dfrFMn;			// fraction of time defrosting in continuous operation: minimum
	}
	else  //tFrMn < tDbO <= tFrMx		Frost and defrost temp range. Note includes TfrMx. Niles V-K-6-b-1.
	{
		//pCapDef(tout) = cprCapCon(tout)*(1. - dfrFMn) + dfrCap*dfrFMn	   Niles' fcn is expanded inline below, then simplified.
		// frost degradation factor @ min & max frost temps (no frost buildup effect), from given frac of time spent defrosting
		DBL fdMn = (1. - dfrFMn) + dfrCap*dfrFMn/cprCapCon(tFrMn);				// dfrCap neg!
		DBL fdMx = (1. - dfrFMn) + dfrCap*dfrFMn/cprCapCon(tFrMx);
		// frost degradation factor @ 35F, from 35Fcap input (usually defaulted from fd35Df)
		DBL fd35 = cap35/cprCapCon(35.);
		/* frost degradation factor @ tFrPk: extrapolate from fd35 and fdMn or fdMx (whichever temp is on far side of 35),
		   thus defining triangular degradation fcn going thru fdMn, fd35 & fdPk, fdMx, with corner at fdPk, not fd35. */
		DBL fdPk;
		if (tFrPk < 35.)
			fdPk = fdMx + (fd35 - fdMx)*(tFrMx - tFrPk)/(tFrMx - 35.);
		else // >= 35
			fdPk = fdMn + (fd35 - fdMn)*(tFrMn - tFrPk)/(tFrMn - 35.);
		// Now evaluate that triangular fcn, and get frost degradation factor @ current outdoor temp
		DBL frToPk;		// fractional height of triangular fcn: either end = 0; peak = 1. Also used re frTmDfr below.
		DBL fdTout;
		if (tDbO < tFrPk)
		{
			frToPk = (tDbO - tFrMn)/(tFrPk - tFrMn);
			fdTout = fdMn + (fdPk - fdMn)*frToPk;
		}
		else // >= tFrPk
		{
			frToPk = (tDbO - tFrMx)/(tFrPk - tFrMx);
			fdTout = fdMx + (fdPk - fdMx)*frToPk;
		}
		capCon = fdTout*cprCapCon(tDbO);     		// degrade continuous capacity; adj for tIa below.
		frTmDfr = dfrFMn + frToPk * (dfrFMx - dfrFMn);	// time defrosting: same triang fcn as degradation factor
	}
	capCon *= (1. - capIa * (tIa - 70.));  		// adjust capacity for indoor air temp difference from 70. Niles V-K-7.
	qDfrhCon =     					// average continuous operation heat output to defrost heater
		frTmDfr     				// is computed fraction of time defrosting
		* capSupHeat; //dfrRh;  	// times user-input capacity of heater

#if 1//rework to relate to frFanOn, 10-24-92

// determine output and fraction run time for load. Niles V-K-8, modified to consider externally-determined fan fraction runtime.
//     sets cap, q, qSh, frCprOn. q may be recomputed in coilsEndSubhr from tex computed below.
//     note output of BOTH supplemental heaters is included BOTH in q and qSh and pSh.
//     fan runtime is predetermined externally, rather than here from frCprOn, to fit existing program organization 10-92.

	// enable supplementary resistance heat only when fan always on, with hysteresis: use full compressor output first.
	if (frFanOn >= Top.loTol)			// if (just about) time on, allow suppl heat use
		supOn = TRUE;
	else if (frFanOn < Top.loTol*Top.loTol) 	// if clearly not full time on, use compressor only, at least while frFanOn rises.
		supOn = FALSE;
	//else, in between, supOn remains unchanged, so small noise does not change suppl heat, for stability & convergence.

	co_capMax = capCon + qDfrhCon + capSupHeat;		// full cap'y this subhour: full compr cap, + suppl heat. set for caller.

	//determine cpr heat available with compressor running only when fan on
	//   derivation of fraction output available for fraction runtime f:
	//      fraction output hlf = fraction runtime * relative COP = f * plf   (1)   relative COP is plf: definition.
	//      definition:  cdm = (1 - plf)/(1 - hlf).   solve it for plf: plf = 1 - cdm + hlf*cdm.   (2)
	//      substitute (2) into (1): hlf = f * (1 - cdm + hlf*cdm)
	//      solve for hlf: hlf = f*(1 - cdm)/(1 - f*cdm).
	//   capFon = capCon * hlf + pDfrCon * frFanOn;			capacity of compressor in fan-on time, w/o suppl heat
	//   substitute for hlf, substitute frFanOn for f:
	//
	DBL capFOn = capCon*frFanOn*(1 - cdm)/(1 - frFanOn*cdm) 	// compr capacity if runtime is frFanOn: adjust for degradation
				 + qDfrhCon * frFanOn;				// defrost reaheat (negative?): no degradation.

	if (qWant >= capFOn)			// if desired heat is >= cpr can deliver (b4 considering suppl heat)
	{
		frCprOn = frFanOn;			// cmpr runs all the time that fan runs
		q = capFOn;				// output is (at least) compressor's output
		if ( supOn				// if suppl heat enabled: enabled only at full fan flow, with hysteresis, above
				&&  qSupLim > q)			// and heat left under special limit
		{
			DBL supAv = min( capSupHeat, 		// suppl heat we can use: smaller of heater rated output,
							 qSupLim - q);	/* remainder of special heat limit that keeps ts down so frFanOn stays 1.0 when
                           			   fan cycling, so ah converges. See caller doAhpHeatCoil and cnah:setTsSp1.*/
			if (q + supAv >= qWant)		// will it meet load? (frFan is 1.0 so ignored here)
				qSh = qWant - q;			// yes, use just enough to meet load.
			else
			{
				qSh = supAv;
				coilLimited = TRUE;	// won't meet load, use it all and tell caller coil-limited
			}
			q += qSh;				// add supl heat to output
		}
		else					// suppl heat disabled or special limit already used up
		{
			coilLimited = TRUE;			// tell caller air handler is coil-limited
			qSh = 0.;				// no suppl heat
		}
		qSh += qDfrhCon * frFanOn; // add defrost make-up heat portion of q to suppl qSh, for meter/reports
	}
	else					// heat pump can meet load in time fan runs, w/o suppl heat.  determine runtime.
	{
		DBL hlf = 0; 		// heating load factor: hp heat used/capCon, excluding reg & dfr rh. frCprOn may be larger.
		DBL plf = 0; 		// part load factor: factor by which COP falls due to cycling.
		/* and have cdm = modified (from ahpCd, at setup time) cycling degradation coefficient
		   = ratio of COP reduction to capacity reduction for cycling.
		   = (1 - plf)/(1 - hlf) */
		// determine compressor run time (frCprOn) to meet part load qWant. Run time is more than proportional
		// to fraction output needed, cuz COP degrades, and added defrost for added run time complicates it.
		//   given:  qWant = hlf * capCon + frCprOn*qDfrhCon   and   frCprOn = hlf/plf   and   plf = 1 - cdm*(1 - hlf).
		//   substitute 3rd eqn into 2nd, then 2nd into first, move all to left, gather terms, get:
		//     hlf^2 * cdm * capCon  +  hlf * (capCon * (1 - cdm) + qDfrhCon - cdm * qWant)  -  qWant * (1 - cdm)  =  0
		//   write it as A*hlf^2 + b*hlf + C = 0, and compute the coefficients:
		DBL A = cdm * capCon;
		DBL B = capCon * (1 - cdm) + qDfrhCon - cdm * qWant;
		DBL C = -qWant * (1 - cdm);
		//apply quadratic formula, use the positive root.
		DBL rootArg = B*B - 4*A*C;
		if (rootArg >= 0.)					// prevent sqrt neg # runtime library error. msg below.
		{
			hlf = (-B + sqrt(rootArg))/(2*A);			/* frac of capCon that will meet load when defr rh is added
								   this is the positive root, cuz A is positive.*/
			plf = 1 - cdm * (1. - hlf);				// COP degradation (partLoadCOP/fullLoadCOP) for this hlf
			if (plf > 0)						// prevent /0; msg below
			{
				frCprOn = hlf/plf;					/* run time: hlf increased for reduction in COP,
          							   ie by amount of addl input required to get rel output hlf */
				qSh = frCprOn * qDfrhCon; 				// defrost heat used for this fraction run time
				q = hlf * capCon + qSh;   				// heat delivered: compressor + dfr heat
			}
		}
		pSh = qSh / effSupHeat;

		// debug aid checks. display only 1st message: other errors might be consequential, or vbls unset.
		if (rootArg < 0.)   					// if wd have gotten sqrt neg # runtime lib err
			rer( (char *)MH_R1341, ah->name,   			//   "airHandler %s: Internal error in doAhpCoil: \n"
				 rootArg,   					//   "    arg to sqrt (%g) for quad formula hlf not >= 0.\n"
				 cdm, qDfrhCon, capCon, qWant,   			//   "        cdm %g   qDfrhCon %g   capCon %g   qWant %g\n"
				 A, B, C );					//   "        A %g   B %g   C %g\n"
		else if (hlf <= 0. || hlf > Top.hiTol*qWant/capCon)	// hlf excludes dfr rh --> less than qWant/capCon.
			rer( (char *)MH_R1342, ah->name, 			//   "airHandler %s: Internal error in doAhpCoil: \n"
				 hlf, qWant/capCon );				//   "    hlf (%g) not in range 0 < hlf <= qWant/capCon (%g)"
		else if (plf <= 0.)					// prevent /0, report < 0 (bug)
			rer( (char *)MH_R1343, ah->name, plf);		//   "airHandler %s: Internal error in doAhpCoil: \n"
		//   "    plf (%g) not > 0"
		else if (frCprOn > frFanOn * Top.hiTol)			// cpr shd run < fan ( >= uses other case above)
			rer( (char *)MH_R1344, ah->name, frCprOn, frFanOn);	//   "airHandler %s: Internal error in doAhpCoil: \n"
		//   "    frCprOn (%g) > frFanOn (%g)"
		else if (RELCHANGE( q, qWant) > Top.relTol)		// q should come out heat desired
			rer( (char *)MH_R1345, ah->name, q, qWant);		//   "airHandler %s: Internal error in doAhpCoil: \n"
		//   "    q = %g but qWant is %g -- should be the same"
	}
#else//worked in brief tests, but did not relate to flow or frfanOn -- runs cpr more than caller runs fan, etc.
x
x// determine output and fraction run time for load. Niles V-K-8.
x    //sets hlf, qSh, q, frCprOn. q may be recomputed in coilsEndSubhr from tex computed below.
x
x    cap = capCon + qDfrhCon + capSupHeat;		// cap'y this subhour: full compr cap, + suppl heat. set for caller.
x    if (capCon + qDfrhCon <= qWant)		// if can't meet (or just meets) load without suppl heat
x    {  if (cap < qWant)				// if can't meet load with suppl heat
x       {  q = cap;				// deliver all the possible heat
x          qSh = qDfrhCon + capSupHeat;   		// supplemental heat output: defrost suppl + full reg suppl.
x          coilLimited = TRUE;			// tell caller (ref arg) ahp is max'd out
x		}
x       else					// hp with suppl heat can meet load
x       {  q = qWant;				// deliver desired heat
x          qSh = qWant - capCon;    		// supplemental heat: defrost, plus needed q from regular suppl heater
x		}
x       hlf = 1.;				// heating load factor: full load
x       plf = 1.;				// part load factor: COP not reduced at full load
x       frCprOn = 1.;   				// compressor runs continuously
x	}
x	else					// heat pump can meet load w/o suppl heat, therefore is cycling
x	{
x       // determine compressor run time (frCprOn) to meet part load qWant. Run time is more than proportional
x       // to fraction output needed, cuz COP degrades, and added defrost for added run time complicates it.
x       //   given:  qWant = hlf * capCon + frCprOn*qDfrhCon   and   frCprOn = hlf/plf   and   plf = 1 - cdm*(1 - hlf).
x       //   substitute 3rd eqn into 2nd, then 2nd into first, move all to left, gather terms, get:
x       //     hlf^2 * cdm * capCon  +  hlf * (capCon * (1 - cdm) + qDfrhCon - cdm * qWant)  -  qWant * (1 - cdm)  =  0
x       //   write it as A*hlf^2 + b*hlf + C = 0, and compute the coefficients:
x       DBL A = cdm * capCon;
x       DBL B = capCon * (1 - cdm) + qDfrhCon - cdm * qWant;
x       DBL C = -qWant * (1 - cdm);
x       //apply quadratic formula, use the positive root.
x       DBL rootArg = B*B - 4*A*C;
x       RC rrc = RCOK;								// re suppressing consequential error messages
x       if (rootArg < 0)								// prevent sqrt neg # runtime library error
x       {	rrc = rer( "airHandler %s: Internal error in doAhpCoil: \n"
x                     "    arg to sqrt (%g) for quad formula hlf not >= 0.\n"
x                     "        cdm %g   qDfrhCon %g   capCon %g   qWant %g\n"
x                     "        A %g   B %g   C %g\n",
x                     ah->name,   rootArg,   cdm, qDfrhCon, capCon, qWant,   A, B, C );
x          rootArg = 0.;								// use 0 and fall thru
x		}
x       hlf = (-B + sqrt(rootArg))/(2*A);				// frac compr cap that meets load with defrost rh added
x       //i claim above is the larger and more positive root, cuz A is positive. But Niles changed roots if necess:
x       //if (hlf < 0.)  hlf = -hlf - B/*A;				restore if need found
x       if (hlf <= 0. || hlf > Top.hiTol*qWant/capCon)			// hlf excludes dfr rh --> less than qWant/capCon.
x          rrc = rer( "airHandler %s: Internal error in doAhpCoil: \n"
x                     "    hlf (%g) not in range 0 < hlf <= qWant/capCon (%g)",
x                     ah->name, hlf, qWant/capCon );
x       plf = 1 - cdm * (1. - hlf);					// COP degradation for this hlf
x       if (plf <= 0.)							// prevent /0, report < 0 (bug)
x          rrc = rer( "airHandler %s: Internal error in doAhpCoil: \n"
x                     "    plf (%g) not > 0", ah->name, plf );
x       else
x          frCprOn = hlf/plf;					// run time: increase hlf for reduction in COP (reduc in output)
x       qSh = frCprOn * qDfrhCon;   				// defrost heat used for this fraction run time
x       q = hlf * capCon + qSh;   				// heat delivered: compressor + dfr heat
x       if (RELCHANGE( q, qWant) > Top.relTol)			// q should come out heat desired
x          if (rrc==RCOK)						// suppress msg if any other msg just above
x             rer( "airHandler %s: Internal error in doAhpCoil: \n"
x                  "    q = %g but qWant is %g -- should be the same",
x                  ah->name, q, qWant );
x}
#endif	// if 1...else  frFanOn rework

// compressor energy input. Niles V-K-9. .pCpr is copied to .p in coilsEndSubhr. hmm... could do this there.

	/* linear fcn of outdoor temp going thru in17 and in47 with cch input subtracted,
	   times adjustment for indoor air temp per aphInIa, times fraction of time running. */

	pCpr =  (in17c + (in47c - in17c)*(tDbO - 17.)/(47.- 17.))     	// p for outdoor temp
			*  (1. + inIa*(tIa - 70.)) 					// adj for indoor temp
			*  frCprOn;							// times fraction time on

// note cch heater input, Niles V-K-10, is in CCH::endSubhr, below in this file?

	//on return, q is set, caller determines tex etc. also frCprOn, cap, qSh, pSh, pCpr.
}		// AHHEATCOIL::doAhpHeat
//-----------------------------------------------------------------------------------------------------------------------------
// cooling coils
//-----------------------------------------------------------------------------------------------------------------------------
float AH::coolCapThing( 	// cool coil autoSizing power request adjusments

	float captWant )	// power to meet setpoint texWant when fan on  ((texWant - ten) * cen),  negative for cooling

// returns adjusted captWant: tolerance added, reduced to prevent excess size when fanCycles, etc.
{
// add tolerance, but not if fanCycles.
	if (!fcc)						// TESTED if (!fcc) improved fanCycles results, 7-21-95
		captWant *= Top.auszHiTol2; 			// adjustment for target plr of 1 - half of autoSizing tolerance

	/* fanCycles autoSize problems:
	   1. always meeting fixed ahTsMn/Mx setpoint temp makes coil too big: wanna do peak at setpoint (determines cfm vs Btus);
	      off peak, use less dT (less coil power) and more frFanOn if leak/loss/eco/latent load less favorable.
	   2. cen may be for old ts -- much too large -- so need to limit increases and to undo unnec increases on later calls.
	   NOT FULLY SOLVED as of 7-22-95: in run T223.inp, ZN makes autoSized coil & fan about 10% bigger
	   than numeric supply temp equal to ahTsMn and ahTsDsC used with ZN. */

// reduce request in proportion to time fan on: Intended to help prevent & reverse unnec increases during subhour iteration.
	// TESTED 6-97 (for heat): this is good even when fan is oversize.
	captWant *= min( frFanOn, 1.);

// failed: non-convergent flow runaway 7-22:
	//limit power in proportion to time fan on, insofar as previously increased this subhour:
	//if (frFanOn < Top.loTol)				// if fan not running (almost) all the time, have execess captRat now
	//   if (ahcc.captRat)
	//      setToMax( captWant, ahcc.captRat * frFanOn * Top.hiTol);	/* reduce req'd power (NEGATIVE) to frFanOn of curr power

#if 1 // 7-23-95 removal makes nonconvergent: peaks at fractional frFanOn, plr never big enuf.
	// inclusion makes T223 (elec coil) flow come out a bit too big. Not perfected yet 7-95.
// measure power increase really needed by requested flow increase, limit larger requests. THIS WORKS (fairly) well.
	if ( fcc  						// if fan cycles this hour
			&&  ahcc.captRat					// don't do at first request for power cuz multiplicative
			&&  bVfDs )					// don't divide by 0
	{
		// limit power increase VS PREVIOUS ITERATATION in proportion to flow increase VS START SUBHOUR
		float prop = sfan.vfDs/bVfDs;			// fractional flow increase this subhour
		float frac = prop;				// multiplier to use fraction of this proportion, = prop for all.
		//float frac = 1 + .8 * (prop - 1);		// tried various fractions. Thought .5 better than 1 or .2, 7-22-95.
		//      ^^^  				7-23 on T223: 1.0 yet better than 1.2 or .8. .8 better than .5.
#if 1 // ok in heat 6-97; trying here.  Good. TEST3.
		frac = sqrt(frac);
#endif
		setToMax( captWant, 				// requested power. NEGATIVE for cooling
			   ahcc.captRat				// current power
			   * frac					// use fraction of proportional change per iteration
#if 0 // historical 6-97, found better to remove for heat, trying removing here.  removal Good. TEST2.
*            * frFanOn  				/* & limit in proportion to fraction time fan on.
*							   (reason for frFanOn < 1 with flow > bVfDs may be fixed 7-21-95,
*							   but helped orginally; keep as insurance). */
#endif
			 );
		// * Top.auszHiTol2		NO! tried adding margin; got coil 10% bigger and flow 30% bigger, 7-22-95.
	}
#endif
#if 1 // really good with heat with oversize fan 6-97; trying here. TEST1. GOOD.
// strongly resist increase over max actually used in preceding autosizing pass 2 (former TRY5)
	// TESTED 6-97 makes small coils even with oversize (for cool) fan, without nonconvergence. BUG0089,90.
	if (ccAs.plrPkAs)					// if a pass 2 yet completed & peak saved
		setToMin( captWant,					// limit requrest to larger of
			  max( ahcc.captRat * Top.auszHiTol2,  	//   current (possibly downsized at end pass) + tolerance,
				   ccAs.plrPkAs * ccAs.xPkAs ) ); 	//   capacity ACTUALLY USED at prior peak
#endif

	/* TEST TESTS: TEST2 in, TEST1,3 out yields old coil sizes, as expected.
	               TEST2 out (TEST1 in or out) yields smaller coils for some CKS15.BAT runs.
	               TEST3 also in yields yet slightly smaller coils. 6-97. Keep these changes. */

	return captWant;
}			// AH::coolCapThing
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doElecCoolCoil(		// compute tex [and wex] that "electric cooling coil" can and will produce.

	DBL texWant1,		// desired coil exit temp (flow=cen)
	DBL texWant,			// desired temp after re-mixing bypassing air, for use re lLim, uLim, tPossC (flow=cmix).
	DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten

// addl inputs:  frFanOn, ten, wen, cen, ahccBypass, .
// outputs:      tex, coilLimited, tPossC.  wex remains wen, as preset by caller.
//  note capt is set to captRat in setup function above.

// returns TRUE if resized coil larger

{
	// called from doCoils.  Only called if texWant < ten and cen > 0 and ahcc.captRat != 0.

#ifdef AUTOSIZE
	/* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
	   May undo increases since start this subhour's iteration if not needed now,
	   eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
	// deletions here 4-22-95 see cncoil.cp8.
#if 0 // for trial just below 7-23-95.
*   float captRatWas = ahcc.captRat;
#else // for new trial just below 10-23-96
	float captRatWas = ahcc.captRat;
	BOO resizedIt = FALSE;
#endif
	float captWant = (texWant - ten) * cen;		// size (negative for cooling) to meet texWant when fan on
	float captWant1 = coolCapThing(captWant);		// fcn just above: add tol, limit change when fan cycles, etc.
	if (resizeCoolCoilIf( captWant1,  			// cond'ly adjust coil size and reSetup / if does it. above in this file.
						  ahcc.bCaptRat ) )		// don't make smaller than this start-subhour size
	{
#if 0 // tried 7-23-95. Minor detrimental effect on T223 CNEP results, interacting with frac value in coolCapThing(),
*      //  but ausz'd fanCycles flow still oversize with elec coil (T223) even tho same as vav with DX (T224).
*      // Elec coils NOT PERFECTED YET. This may be part of an eventual perfect solution.
*
*      // Conditionally say ah not converged after coil change
*         /* If fan cycling, if coil is enlarged just now and flow is at a new high, condtionally force another
*            air handler iteration to allow flow to fall back if actually coil-limited not fan-limited.
*            Because when fanCycles, coolCapThing lets coil get bigger only in interations with increased flow. */
*         /* In pursuit of case (T223) where fan is coming out too big after addition of stuff to keep
*            coil from coming out too big by meeting ts setpoint off peak when fanCycles. 7-23-95. */
*      if ( fcc    						// only when fan is cycling: coolCapThing limits coil growth
*   #if 1
*       &&  sfan.vfDs > bVfDs    				// if air flow is increased this SUBHOUR
*   #else // worse?
*   x    &&  sfan.vfDs/Top.auszHiTol2 > bVfDs  			// if air flow is (almost) increased this SUBHOUR
*   #endif
*       &&  captRatWas						// don't divide by 0 first time
*       &&  (captRatWas - ahcc.captRat)/captRatWas > 1.e-6 )	// if coil made BIGGER this ITERATION
*         ahcc.notDone++;					/* tell AH::iter4Fs not to terminate this iteration.
*            							   May be ignored after n iterations. */
*      /* if this works, consider putting it in other coil types. but:
*         * heat coils have not yet shown the problem,
*         * cool coils have not yet shown the problem (T224 better than T223 -- why?)
*         * DX coils have internal iteration (so don't put this in coolCapThing) */
#else // 10-23-96 something like the above IS needed, set flag for more restricted test below.
		resizedIt = TRUE;				// say changed coil size
#endif
	}
#endif

	ahcc.co_capMax = ahcc.captRat;

	DBL texf = ten + ahcc.co_capMax*frFanOn/cen;  	// possible full-power tex = ten + power/heat cap flow. captRat negative!
	// /0: cen==0 does not get here.
	if (texWant1 < texf )				// if can't cool to desired temp
	{
		tex = texf;					// best temp we can get
		coilLimited = TRUE;  				// can't get desired temp
#ifdef AUTOSIZE
#if 1 // 10-23-96 another try at iterating ah after size increase, re BUG0085.

		/* After coil size increase cond'ly request another iteration of ah, thus of terminals too if ts then changes.

		   Case (10-23-96): Weather increases load, tu's take more flow,
		   coolCapThing limits single-iteration captRat increase such that already-bad ts stays UNCHANGED,
		   AH::iter4Fs() exits prematurely when it should iterate for more captRat increase, better ts, less flow.
		   Result: made T223 better (fan about 3% smaller), 10-23-96. */

		if ( resizedIt  					// if coil size just changed
				// extra conditions cuz of bad experience with similar code 7-23-95, and to make inf loop less likely:
				&&  ahcc.captRat < captRatWas			// precaution: if increased (negative)
				//&&  texWant1 < texf		true if here	// precaution: if not big enuf to meet setpoint at current flow
				&&  captWant1 > captWant )			// precaution: if coolCapThing disallowed enuf incr to get texWant @ cen
			ahcc.notDone++;				// tell AH::iter4Fs not to terminate this iteration

		/* if this works, consider putting it in other coil types. but:
		   * heat coils have not yet shown the problem,
		   * DX coils have internal iteration (so don't put this in coolCapThing) */
#endif
#endif
	}
	else    						// can get desired temp
		tex = texWant1;   				// limit output temp to desired

	tPossC = ahccBypass * ten + (1. - ahccBypass) * texf;	/* achievable temp after bypass air remixed.  Used in
    								   cnah.cpp:AH::setTsSp1 to limit estimate to save iterations. */
	setToMax( lLim, texWant);  				// tex1 won't go below texWant
	setToMin( uLim, ten);					// nor above entering temp (5-29-92)
	return resizedIt;
}			// AH::doElecCoolCoil
//-----------------------------------------------------------------------------------------------------------------------------
// AUSZ: CHW setup, uses vfR defaulted to sfanVfDs.
//       Need to defer setup til needed, and default vfR per current sfanVfDs.     <-- done for const sfanVfDs
//       And I guess conditionally re-setup each time sfanVfDs changed.   7-3-95.  <-- not done, 7-5-95.
//-----------------------------------------------------------------------------------------------------------------------------
RC COOLCOIL::chwSetup( AH* ah)		// set up CHW coil using values already in record, at start run or (future) autoReSize

{
	// called from COOLCOIL::setup above; former cncult5.cpp code.

// design (rating) air (volume) flow, ahccVfR.

	/* code moved here from cncult5 to be ready for addition of dynamic vfR
	   using autoSizing sfan.vfDs and/or captRat times an added input member. 7-95. */

	RC rc = RCOK;
	if (!(ah->sstat[AH_AHCC + COOLCOIL_VFR] & FsSET))		// if ahccVfR not given in input
	{
		if (ah->sstat[AH_SFAN + FAN_VFDS] & FsVAL)		// if sfanVfDs has a value (not expr, not autoSizing)
			vfR = ah->sfan.vfDs;					// use sfan design flow as CHW cool coil design flow
		else							// ahccVfr not given and sfanVfDs not usable
			// AUSZ: when supporting CHW autoSize will need a better solution...
			//  re set up when fan or coil changes? cfm/capacity ratio input?
			if (ah->sfan.ausz)					// specific msg if due to sfan autoSize
				rc |= ah->oer( "ahccVfR for CHW cooling coil must be given when autoSizing supply fan");		// NUMS
			else
				rc |= ah->require( AH_AHCC+COOLCOIL_VFR);		// require it. Input time msg fcn should work at run time
	}

// design (rating) air mass flow @ rated temp, @ sea level 1-95. Similar thing done in cncoil.cpp::dxSetup for DX.

	//compute air density @ sea level: like cncult2:topPsychro() but using local variables not Top members.
	psyElevation(0);						// set psychro pak altitude: sets psyPBar, affects psyHumRat2, &c.
	float refW = psyHumRat2( Top.refTemp, Top.refRH);   	// reference w @ sea level. If over boiling,
															// cncult2:topPsychro will issue error msg & stop run.
	//refW = psyHumRat1( dsTDbEn, dsTWbEn);			to let user specify design entering wet bulb temp
	float airVK = PsyRAir*(1.f + refW/PsyMwRatio)/PsyPBar;	// air specific volume per degree Kelvin @ sea level
	float airDens = 1 / (airVK * (dsTDbEn + 459.67));   	// air density @ rated entering temp @ sea level

	menR = vfR * airDens * 60.;  				// convert volume flow (cfm) to mass flow (lb/hr) @ rated conditns
	psyElevation(Top.elevation);    				// restore psychro altitude to site elevation for use elsewhere!

	return rc;
}		// COOLCOIL::chwSetup
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doChwCoil( 	// compute tex, etc, that CHW cooling coil can and will produce. 10-92.
	DBL texWant1, 		// desired coil exit temp, flow = cen.
	DBL texWant,   		// ditto before adj for bypassing air (ie after remix), for use re lLim, uLim, tPossC. flow cmix.
	DBL &lLim, DBL &uLim )	/* these bounds are narrowed per desired temp and range over which current coil behavior
   				   is valid basis for extrapolation (eg next major power changes or curve knees).
   				   Caller inits to wide temp range; extrapolators use to bound ts extraplolation. */

// addl inputs:  frFanOn, ten, wen, cen, men, ahccBypass, .
// outputs:      tex, wex, coilLimited, chwQ, .  Note: coilsEndSubhr computes q, ql, qs.
//		 re estimating/endtests: tPossC

// if not called: doCoils 0'd ahcc.chwQ. coilsEndSubhr 0's ahcc.ql, qs, q.

// returns TRUE if resized coil larger (doesn't resize 6-97, so returns FALSE)

{
	// called from doCoils.  Only called if texWant < ten and cen > 0.

#ifdef AUTOSIZE
#if 0		// for when CHW autoSizing supported
*
*		This method may not work as CHW does uses captRat as derived not input value ...
*		   or redefine somehow to store peak load in captRat & derive actual parameters here?
*		In any case, will need to increment it below when cannot meet load.
*		And (10-96) will need to not call here from doCoils if capacity (what member?) is 0 after autoSize complete.
*
*  /* if autoSizing this coil now, conditionally adjust size to value needed now. 7-95.
*     May undo increases since start this subhour's iteration if not needed now,
*     eg when flow change changes fan/leak/loss heat or old wrong ts used in figuring flow for first call. */
*  BOO ret =
*       resizeCoolCoilIf(   			// cond'ly adjust coil size and reSetup. above in this file.
*                   coolCapThing(			// add tol, limits when fanCycles, etc.
*                            (texWant - ten) * cen ),	// size (negative for cooling) to meet texWant when fan on
*		      ahcc.bCaptRat );			// don't make smaller than this start-subhour size
#else
	BOO ret = FALSE;
#endif
#endif

	/* This version does not model chilled water flow and leaving temperature.
	   See plans2\CHW3.TXT (1992) for a first cut at pseudo-code to implement all of Niles model that determines flow and temp.
	   Requires additional record members including hswi,texd,ntuO,coilIsWet,te,hex,
	   plus COOLPLANT code to make use of the info and check for excess pump overrun. */

#define ROB1	// rob's changes 10-7-92. Eliminates hex (must restore to compute water flow & return temp). Never compiled undef.
	// edit out when works & ?after getting Niles to review.

#define TEWDTOLMUL 10		/* If tewd changes more than this many times Top.absTol, repeat compuations based on prior tewd.
	This iteration Rob's idea. Niles did analysis showing texd sensitivity to tewd 1/15 in
	a typical case (memo BARNABY7.DOC, 8-92). Rob used 10 not 15 to be safer. */

// conditionally turn on off plant; exit if plant can't be turned on

	COOLPLANT *cp = CpB.p + ahcc.cpi;
	if (!cp->cpOn())				// if coolplant is off (turns it on if scheduled AVAIL) (cncp.cpp)
	{
		//rWarn?
		rer( (char *)MH_R1311, 		// "AirHandler %s's cool coil is scheduled on, \n    but coolPlant %s is scheduled off."
			 name, cp->name );
		//plant off. set output variables for plant off, and return. doCoils has done: tex=ten, wex=wen, chwQ=0.
		//should coilLimited be TRUE when plant OFF? probably not, as not on when coil off, but unimportant now since error 9-92.
		tPossC = ten;				// plant-off achievable temp (after bypass air remixed) is entry temp
		setToMax( lLim, ten);  			// plant-off tex1 won't go below entering temp
		setToMin( uLim, ten);			// nor above it
		return ret;					// tex not set, so ahcc.q and plr will be 0 as desired.
	}

#undef DUMMYCHW  	// also independently definable in cncp.cpp, cntp.cpp.
#ifdef DUMMYCHW		// define for temp simple version for initial test of interfaces, 10-92
*
*// full-load exit temp assuming 100% effectiveness and ignoring humidity
*    // rob's derivation:            max power = max water cap flow * (water temp - entering air temp);                  (1)
*    //       and, assuming neg dt:  min air exit temp = max( water temp, entering air temp + max power/air cap flow); (2)
*    //       subs (1) into (2):     min air exit temp =
*    //               max( water temp, entering air temp + max water cap flow * (water temp - entering air temp)/air cap flow);
*    //       use actual variables:  max( cp->cpTs, ten + (cp->cpTs - ten)*ahcc.mwDs*CPW/(cen/frFanOn));
*
*    DBL texd = max( cp->cpTs, ten + (cp->cpTs - ten)*ahcc.mwDs*CPW*frFanOn/cen);	// lowest poss full-load fan-on exit temp
*    ahcc.capt = cen * (texd - ten);							// full-flow (full-load) capacity
*
*// part or full sensible load; w ignored (wex is preset to wen)
*    if (texWant1 < texd )				// if can't cool to desired temp
*    {  tex = texd;					// best temp we can get
*       coilLimited = TRUE;  				// can't get desired temp
*	 }
*    else    						// can get desired temp
*       tex = texWant1;   				// limit output temp to desired
*
#else // not DUMMYCHW

	DBL menFOn = men/frFanOn;		// fan-on entering air mass flow, same as men if fan does not cycle (if frFanOn 1.0)
	DBL cenFOn = cen/frFanOn;		// fan-on heat cap flow likewise
	DBL ntuO = ahcc.ntuoDs*pow(menFOn/ahcc.menR,double( ahcc.k1));	// outside transfer units at this air flow (fan on)
	DBL efecO = 1. - exp(-ntuO);				// outside effectiveness, this air flow, fan on, full water flow
	DBL hexd;				// enthalpy of air leaving coil at full water flow
#ifndef ROB1	// else not needed for part load; member fullLoadWet added 10-92
x   BOO coilIsWet;  			// non-0 if coil is wet @ actual load this subhour
#endif
#ifndef ROB1
	DBL hex; 	 			// exiting air enthalpy, at least in full load and wet coil cases.
#endif
	DBL hewd;
	float wewd;		// enthalpy and w at coil effective wet surface conditions, full water flow (fan on)
	DBL tewdWas;			// prior temp at eff coil wet surf conditions, full water flow (fan on)
	SI nIter = 0;			// iteration count for following loop

// full water flow === full power === full load  exit conditions for current entering conditions

	do				// repeat if tewd changes too much: Rob's idea to insure stable as well as accurate results.
	{
		// effectiveness with wet surface and full water flow, at current air flow (fan on, if cycling). Niles V-I-1.

		DBL cs = psySatEnthSlope(ahcc.tewd);		// slope of saturated dh/dt at last full-water-flow effective temp
		DBL mstar = menFOn*cs/(ahcc.mwDs*CPW);   	// sat air flow heatcap/water ditto
		//ntuI is ntuiDs at full water flow.
		DBL ntuWet = ntuO/(1. + mstar*(ntuO/ahcc.ntuiDs));	// combined number of transfer units: like series conductances
		DBL subExp = exp(-ntuWet*(1.-mstar));
		DBL efecWet = (1. - subExp)/(1. - mstar*subExp);	// wet coil effectiveness, full water flow

		// full water flow coil exit air conditions (fan on). Niles V-I-2. First, determine tewd, iterate if changed a lot.

		// determine full water flow effective h,w,t of coil surface assuming wet
		DBL hswi = psySatEnthalpy(cp->cpTs);		// enth sat air at water inlet temp. lib\psychro2.cpp.
		ahcc.hen = psyEnthalpy(ten,wen);			// enth entering air. lib\psychro2.cpp. hen:AH member, but not preset.
		hexd = ahcc.hen + efecWet*(hswi - ahcc.hen);	// enth leaving wet coil, by def'n of effectiveness
		hewd = ahcc.hen + (hexd - ahcc.hen)/efecO;	// enth sat air at wet coil eff surf temp
		tewdWas = ahcc.tewd;
		ahcc.tewd = psyTWEnthMu( hewd, 1.0, wewd);		// temp & w at wet coil eff surf enth.  -1 for > boiling unexpected.
														// tewd used above.
	}
#if 0
x    while (fabs(ahcc.tewd - tewdWas) > TEWDTOLMUL*Top.absTol);	// repeat while tewd changed more than 10*.1 degree (or as changed)
x
#elif 0	// oscillating, as debug test force tight convergence here. 10-20-92.  DOES NOT FIX IT BY ITSELF.
x    while (fabs(ahcc.tewd - tewdWas) > Top.absTol/10.);	// repeat while tewd changed more than .1/10. degree (or as changed)
#elif 0	// try even smaller... pursuing t81... temporary 10-21-92. Used thru 12-7-92.
x    while (fabs(ahcc.tewd - tewdWas) > .0001 && ++nIter < 10);		// repeat up to 10 times while tewd changed more this.
x				// tewd has granularity of .0005; alternates even if ten, wen FROZEN externally
x				// ---> can't fix t81 without fixing the 2 degree jump from wet coil to dry coil.
x				// THEN loosen this test as far as practical. 10-21-92.
#elif 0	// 12-7-92 (CSE .334), after hysteresis added below: loosening to test sensitivity.
x	// .001, .01: no effect on T81 test results. .1: untried.
x	// 1.0 (equiv to original code): very minor effect on T81 reports.
x
x    while (fabs(ahcc.tewd - tewdWas) > 1.0 && ++nIter < 10);		// repeat up to 10 times while tewd changed more this.
x
#else	// 12-7-92 make like original.
	// TESTED 12-7-92 smaller tolerances made little difference (T81).
	while ( fabs(ahcc.tewd - tewdWas) > TEWDTOLMUL*Top.absTol	// repeat while tewd changed more than 10*.1 degree (or as changed)
			&&  ++nIter < 10 );					// max 10 iterations: insurance.
	/* note that tewd changes about .0005 if recomputed even for
	   SAME ten, wen inputs (noise in pych fcns?), thus hysteresis
	   is used below in wet-dry decision cuz of discontinuity there.*/
#endif
#if 1//10-22-92
// decide whether coil wet at full flow: dry if entering air drier than wewd.

	/* use hysteresis so noise eg in psych fcns doesn't make it jiggle back and forth,
	   cuz 10-92 model's output has major (10%) discontinuity when it goes dry and air handler won't converge (TESTED 10-21-92).
	   Discontinuity is cuz, for simplicity and speed, model assumes coil ENTIRELY wet if wet at all. */

#define HYSMUL 20		// coil-wet hysteresis tolerance multiplier. 1 didn't work. 10 works (file T81).
	// 20 for margin; does not alter reported results (T81).

	if (wen > wewd + HYSMUL * Top.absHumTol)
		ahcc.fullLoadWet = TRUE;				// wet @ full water flow, may set dry below.
	else if (wen < wewd - HYSMUL * Top.absHumTol)
		ahcc.fullLoadWet = FALSE;   			// else leave unchanged so small (noise) changes don't flip discontinuity
#undef HYSMUL

// finish computing full water flow fan-on coil exit contitions, for wet or dry coil

	DBL texd, wexd;		// t,w of air leaving coil at full water flow, fan on ("texf","wexf" for other coil types).
	if (ahcc.fullLoadWet)   				// if wet at full water flow
#else
x// finish computing full water flow fan-on coil exit contitions
x
x    // coil is dry if entry air is drier than wewd.
x    DBL texd, wexd;		// t,w of air leaving coil at full water flow, fan on ("texf","wexf" for other coil types).
x    if (wen >= wewd)					// if coil wet at full water flow (may still be dry at part load)
#endif
	{
		// finish wet coil analysis
		texd = ten + (ahcc.tewd - ten)*efecO;		// exit t,w by effectiveness
		wexd = wen + (wewd - wen)*efecO;			// ..
#if 0
x       ahcc.fullLoadWet = TRUE;
#endif
	}
	else	// coil is dry
	{
		// dry coil analysis, per standard counterflow sensible heat exchanger practice
		DBL cStar = cenFOn/(ahcc.mwDs*CPW);		// air/water flow heat cap ratio
		DBL ntuDry = ntuO/(1. + cStar*ntuO/ahcc.ntuiDs);	// # transfer units
		DBL subExp = exp(-ntuDry*(1. - cStar));
		DBL efecDry = (1. - subExp)/(1. - cStar*subExp);	// dry coil effectiveness
		texd = ten - efecDry*(ten - cp->cpTs);		// full-water-flow exit temp
		wexd = wen;					// exit w is entry w
#ifndef ROB1
		hexd = psyEnthalpy( texd, wexd);			// exit h
#endif
#if 0
x      ahcc.fullLoadWet = FALSE;
#endif
	}
	ahcc.capt =   cen * (texd - ten) 			// full-water-flow capacity
				  + men * (wexd - wen) * PsyHCondWtr;	// (used in coilsEndSubhr to compute plr, otherwise not needed)

	ahcc.co_capMax = ahcc.capt; // TODO: Replace?

// Actual fan-on coil exit air conditions. Niles VI-I-3.

#ifndef ROB1
x    coilIsWet = ahcc.fullLoadWet;   	// init actual-load wetness to full-load
#endif
	if (texd > texWant1)		// if full-water-flow exit temp too hi, cannot meet load
	{
		tex = texd;
		wex = wexd;				// temp, w, h that can be supplied
#ifndef ROB1
		hex = hexd;
#endif
		coilLimited = TRUE;
	}
	else				// can meet load --> operate at part load
	{
		//coilLimited = FALSE;				// preset FALSE by doCoils.
		tex = texWant1;					// desired temp will be delivered
		wex = wen;					// exit w if coil dry [, init for coil wet code]
		if (ahcc.fullLoadWet)   				// if coil wet at full water flow, determine wetness for part flow
		{
			// determine eff surf temp to yield exit temp by solving this for te:  tex = te + (ten - te)*(1 - efecO).
			DBL te = (tex - ten*(1. - efecO))/efecO;	// effective surface temp, wet coil, actual part load
#ifdef ROB1
			DBL we = psyHumRat3(te);			// saturation humidity @ te. -1 return for over boiling not expected.
			if (wen >= we)				// if air is wet enough to be saturanged at te
				wex = wen + (we - wen)*efecO;		// exit humidity by effectiveness
			// part load wet/dry here has no discontinuity, so no hysteresis is used; whether wet is not remembered.
#else //old
x          if (wen >= psyHumRat3(te))			// if air is wet enough to be saturanged at te
x			{
x             DBL he = psySatEnthalpy(te);		// enthalpy at effective coil surf conditions
x             hex = ahcc.hen + (he - ahcc.hen)*efecO;	// exit enthalpy by effectiveness, used in doChwCoilWater
x             wex = psyHumRat4( tex, hex);		// exit humidity
x			}
x          else						// air drier than saturation @ effective temp
x             coilIsWet = 0;				// means coil is dry
#endif
		}						// tex is not set for dry coil part load cases!
#ifndef ROB1
		//if (!coilIsWet)
		//   hex = psyEnthalpy( tex, wex);    		part load dry coil exit air enthalpy -- if needed
#endif
	}

#endif // DUMMYCHW else

// compute (negative) power taken from water, for COOLPLANT. Average when fan cycling: cen/men = average, tex/wex = fan-on.
	// same as power to air (ahcc.q), by heat balance. ahcc.q,ql,qs are computed from tex,wex by coilsEndSubhr.
	ahcc.chwQ =   cen * (tex - ten) 			// sensible part
				  + men * (wex - wen) * PsyHCondWtr;	// latent part. PsyHCondWtr is 1061.

// water return temp, ? used only here
	ahcc.tr = cp->cpTs - ahcc.chwQ/ahcc.mwDs;			/* H2O return temp to cp. Assume full design flow,
    								   as model does not compute flow (conservative) */
// error if water clearly frozen (eg when texWant < 32 cuz of hi dT of following poorly-spec'd drawthru fan, 12-3-92)
	if (ahcc.tr < 32.  &&  cp->cpTs >= 32.)	// but if entering "water" temp < 32 (assume cncp.cpp msg'd), omit per-coil msg here.
		rer( (char *)MH_R1340,	// "airHandler %s: frozen CHW cooling coil: leaving water temperature computed as %g F or less"
			 name, ahcc.tr );

// conditionally call-flag coolplant serving coil

	// plant must be recomputed if (overall) return temp changed, to assure iteration to convergence when overloaded.
	// power change? believe temp monitoring @ assumed constant flow sufficient.
	// flow change? flow not modelled; range of poss cp flow limited by assumed bypass value and setup time overrun limit check.
	if (RELCHANGE(ahcc.tr - cp->cpTs, ahcc.trPr - cp->cpTs) > Top.relTol)	// if return temp rel to current supply temp changed
		// || RELCHANGE(ahcc.chwQ,ahcc.qPr) > Top.relTol)			// restore if need to monitor power demonstrated, 10-92
	{
		Top.cpKf++;						// say a cp is flagged
		cp->cpClf++;						// say must CALL this cp so it can check if overall change > tol
	}								// ... and recompute if so.
#ifdef DEBUG2
	else
		endCp();  						// cncp.cpp: place to put breakpoint to reach at end of iteration
#endif

// save prior values
	//ahcc.trPr = ahcc.tr;			set in cp: coil return temp when cp last computed, re call-flagging cp here
	ahcc.cpTsPr = cp->cpTs;			// supply temp for which last computed, re compute-flagging coil in cp

// reduce extrapolation bounds for the rest of the program, etc

	tPossC = ahccBypass * ten + (1. - ahccBypass) * texd;	/* achievable temp after bypass air remixed.  Used in
    								   cnah.cpp:AH::setTsSp1 to limit estimate to save iterations. */
	setToMax( lLim, texWant);  				// tex1 won't go below texWant
	setToMin( uLim, ten);					// nor above entering temp

	//plant-off error return is above
	return ret;
}    					// AH::doChwCoil
//-----------------------------------------------------------------------------------------------------------------------------
RC COOLCOIL::dxSetup(AH* ah)		// setup direct expansion cooling coil, at start run or autoReSize

{
	// called from COOLCOIL::setup above 7-95; contains former cncult5.cpp code + former cncult5-called dxSetup.

#ifdef AUTOSIZE // cnglob.h 7-95
// don't set up now if rated capacity is zero: 0 captRat--> 0 vfR, menR, cenR, division by zero.
	if (captRat==0)
		return RCOK;
#endif

#define MAXITER 40	// 5: fails, errTe -.027;  10: -.00013; 20: no msg seen 6-25-92.  Increase more later anyway?

//-------- default vfR (rating/design cfm). Varies at runtime if autoSizing. --------

	BOO vfRgiven = ah->sstat[AH_AHCC + COOLCOIL_VFR] & FsSET;	// non-0 if ahccVfR given in input file
	if (!vfRgiven)						// if ahccVfR not given in input file
		vfR = vfRperTon						// default ahccVfR is cfm/ton (default 400)
			  * (-captRat/12000.);				// times coil tons (a ton is defined as 12000 Btuh).

// 5-97: increase 20% tolerance to 25%, per Taylor via Bruce. cnah1.cpp and cncoil.cpp.
// warn if vfR differs from sfanVfDs by more than 25%, here if constant inputs (thus only once cuz only called once).
	BOO perTonGiven = ah->sstat[AH_AHCC+COOLCOIL_VFRPERTON] & FsSET;
	if ( ah->sstat[AH_SFAN+FAN_VFDS] & FsVAL			// if sfanVfDs has value (not autoSizing)
			&&  ah->sstat[AH_AHCC+COOLCOIL_CAPTRAT] & FsVAL )		// if captRat has value (not autoSizing)
		if ( 1.251 * vfR < ah->sfan.vfDs
				||  vfR > 1.251 * ah->sfan.vfDs )			// if supply fan rating 25% different from coil rating flow
			ah->oWarn((char *)MH_S0647, 				/* "%scoiling coil rating air flow (%g) differs \n"
								   "    from supply fan design flow (%g) by more than 25%" */
							  vfRgiven || perTonGiven ? "" : "defaulted ",	// NUMS no use moved from cncult5.
							  vfR,
							  ah->sfan.vfDs );

//-------- precompute derived values that do not change during run ---------

// input energy correction at min of unloading range.  MUST FOLLOW pydxEir defaulting.
	eirMinUnldPlr = pydxEirUl.val(minUnldPlr);		// value to use for false-loading and pro-rate for cycling

// humidity ratio and enthalpy of saturated air at minimum evaporator temperature (at actual elevation)
	wsatMinTEvap = psyHumRat3(minTEvap);		// w for dew point temp, lib\psychro2.cpp
	hsatMinTEvap = psySatEnthalpy(minTEvap);		// sat enthalpy for temp, lib\psychro2.cpp
#if 0
x
x// mass flow at rating cfm flow, for rated effectiveness (here) and actual-conditions effectiveness part load (doDxCoil).
x    menR = vfR * Top.airDens(dsTDbEn) * 60.;  			// entering mass flow @ rated ten (lb/hr).
x								// same conversion done in cncult5.cpp for CHW coil.
x
x//------- rest of fcn: coil effectiveness and thus ntu at rated conditions --------
x
x// entering conditions, per rating conditions
x    DBL cenR = menR * Top.tp_airSH;			// entering heat capacity flow (btu/hr-F)
x    DBL tenR = dsTDbEn;						// entering temp: rated
x    DBL wenR = psyHumRat1( tenR, dsTWbEn);		// entering w.  w for drybulb, wetbulb, lib\psychro2.cpp.
x    DBL henR = psyEnthalpy( tenR, wenR);		// entering enthalpy
x    if (henR > psySatEnthalpy(tenR))
x       return oer( ah, (char *)MH_R1312);		// "DX coil rated entering conditions are supersaturated."
x
x
x// exit state at rated entry conditions and full rated load
#else//redo for sea level capacity computations 1-5-95

//------- rest of fcn: menR (mass flow at rating conditions), and coil effectiveness and thus ntu at rated conditions --------

// entering conditions, per rating conditions. rating conditions include SEA LEVEL altitude, 1-95.
	psyElevation(0);						/* set psychro pak altitude to SEA LEVEL:
    								   sets psyPBar; affects fcns psyHumRat1, psySatEnthalpy, etc.
								   Must restore to Top.elevation before return. lib\psychr1.cpp.*/
	DBL tenR = dsTDbEn;						// entering temp: rated
	DBL wenR = psyHumRat1( tenR, dsTWbEn);			/* entering w.  w for drybulb, wetbulb, sea level.
    								   rets -1 if tenR over boiling (unexprected). lib\psychro2.cpp.*/
	DBL henR = psyEnthalpy( tenR, wenR);			// entering enthalpy
	if (henR > psySatEnthalpy(tenR))
	{
		ah->oer( (char *)MH_R1312); 				// "DX coil rated entering conditions are supersaturated."
		psyElevation(Top.elevation);				// restore altitude in psychro package to building site elevation
		return RCBAD;						// bad return
	}

// compute air properties @ rating conditions: similar to cncult2:topPsychro but for sea level, tenR, wenR.
	float airVK = PsyRAir*(1. + wenR/PsyMwRatio)/PsyPBar;	// air specific volume per degree Kelvin @ wenR @ sea level
	float airDens = 1. / (airVK * (tenR + 459.67));		// air density @ tenR and wenR @ sea level
	float airSH = PsyShDryAir + PsyShWtrVpr*wenR;		// specific heat of air (using lib\psychro1.cpp constants)

// mass flow at rating cfm flow, for rated effectiveness (here) and actual-conditions effectiveness part load (doDxCoil).
	menR = vfR * airDens * 60.;  				// entering mass flow @ rated ten,wen (lb/hr) @ sea level (member).
	// similar conversion done in cncult5.cpp for CHW coil.

// exit state at rated entry conditions and full rated load
	DBL cenR = menR * airSH;					// entering heat capacity flow (btu/hr-F) (local sea level airSH)
#endif
	DBL texR = tenR + capsRat/cenR;   				// exit temp (F)
	DBL hexR = henR + captRat/menR;     			// exit enthalpy (Btu/lb)
	DBL hSatR = psySatEnthalpy(texR);				// enthalpy of saturated air @ rated exit temp
	if (hexR > hSatR)						// if exit state not in moist air region of psychro chart
	{
		ah->oer(							// issue message, stop run, do not attempt to continue setup
			 (char *)MH_R1313,			//"DX coil capacity specifications yield supersaturated air at coil exit:\n"
			 hexR,   				//"    full-load exit enthalpy (%g) greater than saturated air enthalpy\n"
			 hSatR, texR);			//"    (%g) at full-load exit temp (%g) at rated conditions."
		psyElevation(Top.elevation);				// restore altitude in psychro package to building site elevation
		return RCBAD;						// bad return
	}
	//? error if wexR would be < 0: rated flow too low
	//? if (texR < minTEvap) warning?

//-- effective coil temperature @ rated conditions

	/* definition: for modelling purposes, coil is treated as all at one "effective" temp, and assumed wet.
	               on psychro chart (graph of w vs h) draw line thru entry (hen,wen) & exit conditions (hex,wex)
	               and extend to saturation line: this intersection gives te,he,we.
	   method:     use    he = hen + (hex - hen)/(tex - ten)*(te - ten)	(ie assume that line is straight)
	               search in range 0 <= te < ten for a te such that
	                      he (above) == hsat(te) per psychrometric function "psySatEnthalpy(te)". */

	// simplify known values: put  he = hen + (hex - hen)/(tex - ten)*(te - ten)  in form  he = k + te*slope
	DBL slope = (hexR - henR)/(texR - tenR);  				// combine constants...
	DBL k = henR - tenR*slope;						// ... for = k + te*slope
#define ENTHERR(te) (psySatEnthalpy(te) - k - (te)*slope)		// positive if trial te is too large.
	//te search only works in limited range and assumes hsat(te) steeper than 'slope'.

	// init and check limits of range.
	DBL hi = min(tenR,DBL(PSYCHROMAXT)); 		// Note PSYCHROMAXT (upper psySatEnthalpy limit) 200-->300 5-95.
	DBL lo = 0.;					// PSYCHROMINT is -100, not 0. lo could be reduced. Should it?? Rob 5-95.
	DBL errHi = ENTHERR(hi), errLo = ENTHERR(lo);
	if (errHi <= 0)					// if solution > ten, ten is oversaturated, don't bother to search
	{
		ah->oer( (char *)MH_R1314 );			/* "Program Error: DX coil effective temp (te) at rated conditions\n"
							   "    seems to be greater than entering air temp"
       psyElevation(Top.elevation);			// restore altitude in psychro package to building site elevation
       return RCBAD;					// bad return
    }
    if (errLo > 0)
    {  oer( ah, (char *)MH_R1315); 		/* "DX coil effective temperature (intersection of coil process line with\n"
						   "    saturation line on psychro chart) at rated conditions is less than 0F.\n"
						   "    Possibly your ahccCaptRat is too large relative to ahccCapsRat." */
		psyElevation(Top.elevation);			// restore altitude in psychro package to building site elevation
		return RCBAD;					// bad return
	}
	// search interval lo..hi for te where he=k+te*slope and he=hsat(te) curves cross
	DBL teR;						// receives result
	for (SI iter=0; ; iter++)				// search until break
	{
		// here, errLo and errHi are known to have opposite signs.
		teR = lo + errLo/(errLo - errHi) * (hi - lo); 	// next te by straight line approximation
		DBL errTe = ENTHERR(teR);				// hsat(te) - he(te)
		if (fabs(errTe) < 1.e-5)    			// if small enuf, te found, done.
			break;					// don't reduce tolerance: errTe of 2.5e-6 seen. (tol .5e-5 tested ok.)
		if (errTe <= 0.)
		{
			lo = teR;     // if error negative, solution is > te, set lo and iterate
			errLo = errTe;
		}
		else
		{
			hi = teR;     // else solution < te, set hi & iterate
			errHi = errTe;
		}
		if (iter > MAXITER)
		{
			err( PWRN, (char *)MH_R1316, 			// "airHandler '%s': \n"
				 ah->name, errTe );			// "    DX coil setup effective point search convergence failure\n"
			break;					// "        errTe=%g"
		}
	}
#undef ENTHERR

// effectiveness @ rated conditions: fraction of temp difference transferred to air.  Is same for temp, w, h.
	efecOR = (tenR - texR)/(tenR - teR);		// definition of effectiveness
	if (efecOR > 1. || efecOR <= 0.)
	{
		ah->oer( (char *)MH_R1317, efecOR );		// "Program Error: DX coil effectiveness %g not in range 0 to 1"
		psyElevation(Top.elevation);			// restore altitude in psychro package to building site elevation
		return RCBAD;					// bad return
	}

// number of transfer units @ rated conditions: like # time constants; # times tdiff is divided by e=2.71828 as air goes thru coil
	ntuR = -log(1. - efecOR);				// during run, this is adj to entry conditions and used.

#if 1 //1-4-95. also (un-if'd) in error exits.
	psyElevation(Top.elevation);				// restore altitude in psychrometric package to building site elevation
#endif
	return RCOK;
}				// COOLCOIL::dxSetup
//-----------------------------------------------------------------------------------------------------------------------------
// note: input errors: oer(this,msg,...   or   ooer(this,field #,msg,...    or    ooer(this,field #,field #,msg,...
//-----------------------------------------------------------------------------------------------------------------------------
// Niles' quadratic approx to temperature of saturated air @ enthalpy.
//    #define TSAT(h) (-2.4739294 + 3.4058627*h - .0399257*h*h)	// 0 to 60 degree fit
//    #define TSAT(h) (-1.056517 + 3.2194287*h - .0345749*h*h)	// 20 to 60 degree fit
//    #define TSAT(h) (  .53969 + 3.05970*h - .0307386*h*h)   	// 40 to 60 degree fit
// Tried 5-92 in doDxCoil in place of psyTWSat( h, 1., &w), did not save much time.
//-----------------------------------------------------------------------------------------------------------------------------
BOO AH::doDxCoil( 		// compute tex, etc, that dx cooling coil/compressor can and will produce.

	DBL texWant1, 		// desired exit temp
	DBL texWant,			// ditto before adj for bypassing air, for use re lLim, uLim, tPossC.
	DBL &lLim, DBL &uLim )	/* these bounds are narrowed per desired temp and range over which current coil behavior
   				   is valid basis for extrapolation (eg next major power changes or curve knees).
   				   Caller inits to wide temp range; extrapolators use to bound ts extraplolation. */

// addl inputs:  frFanOn, ten, wen, cen, men, ahccBypass, .
// outputs:      tex, wex, coilLimited, frCprOn
//		re estimating/endtests: 	      tPossC
//		re results reporting (coilsEndSubhr): ahcc.tDbCnd, .

// returns TRUE if resized coil larger

{
	// called from doCoils.  Only called if texWant < ten and cen > 0 and ahcc.captRat != 0.
#define MAXITER1 10	// msg seen @ 2, 6-92.  No msg seen @ 5, 6-92.  Msg @ 10 addressed with nIter in endTest, 8-28-95.
#define MAXITER2 100	// this loop now has CONVERGER as some inputs alternate w/o converging, 10-92.

#define MINTDBO   70.	// minimum outdoor (condenser) drybulb arg for pydxCaptT,pydxEirT, per DOE2.  Possible input?
#define MINTWBEN  60.	// minimum entering wetbulb arg for pydxEirT, per DOE2. Possible input?

#define RER  rWarn				// warning messages for now; possibly rer's (stop run) later. 6-92.


// if entering air below minimum temp, coil does nothing
	if (ten <= ahcc.minTEvap)				// if entering air colder than coil can get (insurance)
		return FALSE;					// caller has set tex=ten, wex=wen for 0 coil output: ahcc.q = .co_plr = 0.

#if 1 // 5-97: fix supersaturation at constant enthalpy. Also wen & ten changed to wen0 & ten0 in rest of function.
	DBL wen0 = wen, ten0 = ten;				// wen and ten to adjust if supersaturated, for internal use, 10-96
	DBL wSatEn = psyHumRat3(ten0);			// get saturation w for entering air temp
	if (wen <= wSatEn)					// if ok
		ahcc.xLGain = 0;   				// clear reporting variable
	else						// wen > wSatEn
	{
		// condense enough water so heat of condensation heats air til air with remaining w isn't supersaturated.
		// (ignores portion of the heat that will go elsewhere thru duct surface on which water condenses, etc)
		DBL h = psyEnthalpy( ten, wen);   		// entering air enthalpy.  lib\psychro2.cpp.
		float w0;
		ten0 = psyTWEnthMu( h, 				// t and w for h at % saturation.  lib\psychro2.cpp.
						 1.f,				//  100% saturation
						 w0 );				//  float to receive w
		wen0 = w0;					//  move to our double
		ahcc.xLGain= (wen - wen0) * PsyHCondWtr * men;	/* compute how much heat of condensation this was
							   (is not a loss or gain, just a transfer from water vapor to air) */
		// if ahcc.xLGain is non-0 at end subhour convergence, coilsEndSubhr will accumulate it to xLGainYr.
#if 0 // debug msg, showed plausible results, delete.
x     rInfo( "DX desat  %g, %g  --->  %g, %g", ten, wen, ten0, wen0);
#endif
	}
#elif 1 //10-96. Also wen changed to wen0 in rest of function.
x	// if entering air supersaturated, adjust
x	/* supersaturated zone air is prevented (in ZHR::znW, 10-96) but supersaturation might occur here
x       due to mixing of zones' return airs, return duct loss, or economizer. */
x	DBL wen0 = wen;					// wen to reduce if supersaturated, for internal use, 10-96
x	DBL wSatEn = psyHumRat3(ten0);			// get saturation w for entering air temp
x	if (wen <= wSatEn)					// if ok
x		ahcc.xLGain = 0;   				// clear reporting variable
x	else						// wen > wSatEn
x	{	ahcc.xLGain= (wen - wSatEn) * PsyHCondWtr * men;	// compute latent loss needed to prevent supersaturation
x		wen0 = wSatEn;					// run model with 100% humidity; no error message yet,
x	}							// msg issued in coilsEndSubhr if xLGain nz at ah-tu convergence.
#else // old 10-96
x	// if entering air supersaturated, message & do nothing
x	if (wen > psyHumRat3(ten0))   			// (-1 for ten0 > boiling not expected)
x	{	rer( (char *)MH_R1320, 				// "airHandler '%s': Air entering DX cooling coil is supersaturated:\n"
x            name, ten0, wen);				// "    ten = %g  wen = %g.  Coil model won't work."
x       return;						// caller has set tex=ten, wex=wen for 0 coil output: ahcc.q = .co_plr = 0.
x	}
#endif

#ifdef AUTOSIZE
// if rated capacity still 0 set up coil if autoSizing. Normal coil setup is at start run and in autoSize ah_pass1AtoB.
	BOO ret = FALSE;
	if (ahcc.captRat==0)			// would occur here if no demand til after autoSize pass 1 part A.
		ret = resizeCoolCoilIf( 			// if autoSizing this coil now, conditionally set capacity
				  (texWant - ten0) * cen,	/* size (negative for cooling): use sens load as approximate initial captRat;
						   is increased or decreased below if nec */
				  ahcc.bCaptRat );  	// lower limit: start subhour value saved in hvacIterSubhr: should be 0 here
	if (ahcc.captRat==0)			/* precaution: 0 capacity is illegal input, but can result from autoSizing,
      						   but does not call here from doCoils, 10-96. */
	{
		rer( "airHandler '%s': DX cool coil has ahccCaptRat = 0.", name);  	// bug. NUMS.
		return ret;				// don't continue: would /0.
	}

	// note autoSize resizes below, after determining capacity @ current conditions.
#endif

// re entering conditions

	ahcc.tWbEn = psyTWetBulb( ten0, wen0);  		// wet bulb temp for entering db and hum rat. lib\psychro2.cpp.
	// above used only in coilsEndSubhr; move there?
	ahcc.hen = psyEnthalpy( ten0, wen0);   		// entering air enthalpy.  lib\psychro2.cpp.
	ahcc.tDbCnd = max( Top.tDbOSh,  			// condenser temp is outdoor temp (future water option?)
					   MINTDBO );			// .. but not less than 70 (or as changed), per DOE2.
	// under fcc, temps, w's, h's are fan-on values, but flows (men, cen) are averages...
	DBL menFOn = men/frFanOn;		// fan-on entering air mass flow, same as men if fan does not cycle (if frFanOn 1.0)
	DBL cenFOn = cen/frFanOn;		// fan-on heat cap flow likewise

#ifdef AUTOSIZE // cnglob.h 7-95

//--- head of loop to repeat if capacity must be increased during autoSizing, 7-95

	DBL texf = 0;						// full-load exit temp for current entry conditions
	DBL wexf = 0;						// full-load exit hum rat, for interest & consistency & debug chks
#if 0
	x    DBL wena = max( wen0, ahcc.wsatMinTEvap);    	// wena: artificial wen; increase it if coil dry
#else //11-1-95: move wena init into autoSize loop below
	DBL wena = 0;					    	// wena: artificial wen: increased if coil dry
#endif
	DBL te = 0, he;
	FLOAT we = 0;
#ifdef DEBUG
	SI cs1=0, cs2=0;					/* removed redundant init when BCC32 4.5 warned, 12-94.
							   But init appears non-redundant, restored 7-95. */
#endif
	for (SI j = 1;  j++;  )
	{
#endif

//--- coil effectiveness at actual entering air conditions

		DBL plrM = menFOn/ahcc.menR;				/* fan-on part mass flow factor.
    								   Will be > 1 if flow > rated flow or air denser. */
		ahcc.efecO = 1. - exp(-ahcc.ntuR * pow( plrM, double( ahcc.k1)));	// ntu = ntuR * plrM^k1. k1 is negative. mbr for probing.
		if (ahcc.efecO <= 0. || ahcc.efecO > 1.)
			rer( (char *)MH_R1321, ahcc.efecO); 		// "airHandler '%s': DX coil effectiveness %g not in range 0 to 1"

//--- DX coil capacity and full-load exit state for current entering air conditions

		DBL vfEnFOn = menFOn / (Top.airDens(ten0) * 60.);  	// fan-on volumetric flow entering coil: lbm/hr --> cfm @ temp
		ahcc.plrVf = vfEnFOn / ahcc.vfR; 			// fan-on flow part load ratio: fractn of coil's rated vol flow
		DBL pydxCaptFval = ahcc.pydxCaptF.val(ahcc.plrVf);	// total capacity adj factor for flow: does not change in loop
#if 1 // 8-28-95
		setToMin( pydxCaptFval, (DBL)ahcc.pydxCaptFLim); 		/* limit capacity adj factor so cap'y does not go to infinity
								   as flow goes to infinity (dflt poly is .8 + .2*plrVf)
								   eg when autoSizing. 8-95. */
#endif
		/* Coil effective state: assume coil wet --> te, we at intersection of he and sat line.
			       If this makes we > wen, then coil is dry and must iteratively increase wen
			            till process line horizontal (wen==we) to determine sensible capacity.
			            Evaluate caps poly at wet-coil condition for correct capacity since not doing max(caps,capt).*/
#if 1 //11-1-95: move wena init into autoSize loop from above
		wena = max( wen0, ahcc.wsatMinTEvap);			// wena: artificial wen; increase it if coil dry
#endif
		DBL wena1=0., we1=0., hena;
		for (SI nIter = 0; ; nIter++)
		{
			// effective point is intersection of process line with saturation line or minTEvap line

			DBL pydxCaptTval = ahcc.pydxCaptT.val( psyTWetBulb(ten0,wena), ahcc.tDbCnd);	// total capacity adj factor for temps
			ahcc.capt = ahcc.captRat * pydxCaptTval * pydxCaptFval;     			// total cap @ entry conditions. < 0.
			hena = psyEnthalpy( ten0, wena);   						// entering enthalpy @ wena
			he = hena + ahcc.capt/(menFOn * ahcc.efecO);		// trial effective enthalpy (capt negative!)
			if (he > ahcc.hsatMinTEvap)				// if full-load enthalpy above that of saturation @ min coil temp
			{
				te = psyTWEnthMu( he, 1.0, we);   			// get temp & w for h & 1.0 saturation, -1 if boiling,psychro2.cpp.
				/* note tried using quadratic approximation instead for speed:  te = TSAT(he); we = psyHumRat3(te);
				   didn't seem to make much difference, even b4 adding converger. 5-29-92. */
				cs1 = 1;
			}
			else							// full output too cold, use min coil temp
			{
				cs1 = 2;
				he = ahcc.hsatMinTEvap;
				te = ahcc.minTEvap;
				we = ahcc.wsatMinTEvap;
				//ahcc.capt = (ahcc.hsatMinTEvap - hena) * menFOn * ahcc.efecO;		// reduced capacity
			}
#if defined( _DEBUG)
			DBL he1 = psyEnthalpy(te, we);   			// yet another consistency check
			if (RELCHANGE( he, he1) > Top.relTol/10.)		// /20 gets messages from cs=1. Is psyTWSat() limited.
				RER( (char *)MH_R1322,				// "airHandler '%s' DX coil inconsistency:\n"
					 name, he, te,we, he1,  wena, (INT)cs1 );   	// "    he is %g but h(te=%g,we=%g) is %g.  wena=%g. cs1=%d"
#endif
			// endtest: done if line horizontal, or, on first iteration, slopes down toward effective point
			if ( !nIter && we <= wena  ||  fabs(we-wena) <   .000003 		// TESTED 5-92: .000001 vs .00001 adds but 1 iter.
					+ .0000003*nIter )	/* get looser cuz believe TLVF.INP nonCvg 8-28-95
										   due to precision limits eg in psychro. */
				break;

			// next trial wena for finding horiz process line 	// TESTED 5-92: old wena = we+.00001 loop --> 8-9 iterations;
			//  this converver, file T17, .000001 tol: max 4 iters; usually 2.
			//  slight overshoot & backtrack DOES sometimes occur.
			DBL nuWena = we;   					// next wena >= we: raising wena always raises we.
			if (nIter >= 1  &&  wena != wena1)			// on 2nd and later iter, if won't divide by 0
			{
				DBL slope = (we - we1)/(wena - wena1);		// slope of line thru (wena,we) and (wena1,we1) on we/wena graph
				if (slope != 1.)					// if won't divide by 0, intersection of that line and we=wena
					setToMax( nuWena, (we1 - slope*wena1)/(1.-slope));	// .. is new (minimum) next trial wena
			}
			if (nIter > MAXITER1)					// non-convergence check & msg, while max info avail
			{
				rer( (char *)MH_R1323, name,   			// "airHandler '%s':\n"
					 // "    DX coil full-load exit state convergence failure.\n"
					 ten0, wen0, ahcc.plrVf, 			// "      entry conditions: ten=%g  wen=%g  plrVf=%g\n"
					 te, we, we1,					// "      unfinished results: te=%g  we=%g  last we=%g\n"
					 wena, wena1, nuWena );   			// "          wena=%g  last wena=%g  next wena=%g"
				break;
			}
			wena1 = wena;
			we1 = we;
			wena = nuWena;   		// save input and output for next extrapolation, update wena
		}

		// full-load exit state...   exit humidities; adjust for actual entry humidity

		if (we <= wen0)						// if coil removed moisture
		{
			cs2 = 1;
			wexf = wen0 + (we - wen0)*ahcc.efecO;			// compute full-load exit humidity
		}
		else					// no w removed --> wena may be artificially increased to get proper poly values
		{
			cs2 = 2;
			we = wexf = wen0;   					// effective and exit hums are entry hum.  we probably unneeded.

#if 1 //works; removing changes zone temp .1 degree! in some hours & .01 for year in T17, T18 runs. Keep? memo to Niles. 6-92.

			// if line was lifted, translate it down to actual wen (occasionally unlifted here, due to tolerance I guess)
			if (hena > ahcc.hen)				// TESTED this 'if' eliminated 2 inconsistency msgs in t17a full-year run.
			{
				te = psyTDryBulb( wen0, ahcc.hen + he - hena);  	// delta-t will increase slightly for delta-h cuz air drier.
				setToMax( te, ahcc.minTEvap);				// but not below minimum
				//capt: recomputed below.  he: not needed.
			}
#endif
		}

		/* full-load exit state...  texf, caps, capt.
		   capt is computed here after minTEvap limitation and assuming air heat cap constant
		   (capt determined in some cases above allowed (in psychro fcns) for change in heat capac for w reduction,
		   thus too large for use in later constant-heat-cap computations: plr would come out too small.) */

		texf = ten0 + (te  - ten0) * ahcc.efecO;		/* full-load exit temp for effective temp and effectiveness,
      							   used re actual temp, and for tPossC (limits estimates). */
		ahcc.caps = (texf - ten0) * cenFOn;   		// sens capac for current entry conditions & no cpp adj for w.  Negative!
		DBL capl = (wexf - wen0) * menFOn * PsyHCondWtr;	// 1061. latent capacity.  menFOn is lb dry air -- ignores w.  Negative.
		ahcc.capt = capl + ahcc.caps;   			// constant-heat-cap total capacity, usually nearer 0 than hexf-hen.
		//DBL hexf = psyEnthalpy( texf, wexf);		INCONSISTENT with capt as includes .444*w*t term.
		ahcc.co_capMax = ahcc.capt; // TODO: Replace?

#ifdef AUTOSIZE

//--- if autoSizing, conditionally adjust coil capacity and repeat computation

		// don't increase capacity without limit if minTEvap is limiting tex
		// but do increase to the customary size of 400cfm/ton cuz bigger coil more effective, lowers tex some. Rob 7-95.

		if (te <= ahcc.minTEvap)				// if effective temperature determined by limit not by power
			if ( sfan.vfDs/(-ahcc.captRat) >= 400/12000	// if supply fan cfm/coil ton (12000 Btu) >= 400
					||  ccAs.az_active )				//  or not autoSizing coil - for the message checks
			{
				if (cen > .99 * sfan.cMx)			// next 2 messages only if condition occurs at full flow
				{
					if (ahTsMn < ahcc.minTEvap + 5)	// no message if 5 deg or more texWant depression due to hi fan heat
						ahcc.minTLtd++;		// evap temp, not min ts, limited output: flag for msg at end autoSize and/or run

					if (ccAs.az_active && !asFlow)	// if autoSizing cool coil but not sfan nor any connected terminals
						ahcc.cfm2Few++; 		// flag for msg at end ausz or run: not enuf flow to size coil to meet load.
				}
				break;				// don't autosize larger now (break makes no difference if not autoSizing).
			}

		// compute capacity adjustment needed for autoSizing & determine whether to adjust

		float captF = (texWant1 - ten0)/(texf - ten0);	// factor by which sensible capacity must change to exactly load
		if (captF*Top.auszHiTol2 >= 1 && captF <= 1)	// forget oversizes up to 1/2 autosizing tolerance
			break;					// ... gotta leave this loop sometime!
		// resize also gets ignored, by resizeCoolCoilIf below, if cap'y hasn't been increased this subhr and captF <= 1.

		// wet coil latent load changes faster than sensible. Increase change some now to reduce # iterations.
		if (wexf < wen0)						// if coil is wet
			captF = .875 * captF + .5 * max(captF - .75, 0.);	// add 3/8 of portion of captF over .75

		// add tolerance and restrict size changes to prevent oversizing when fanCycles.
		// tol added makes this loop likely to terminate above on small oversize, if not fanCycles.
		float captWant = coolCapThing(captF * ahcc.captRat);		// fcn above in this file

		// forget small changes: be sure loop ends.
		if (fabs( (captWant - ahcc.captRat)/ahcc.captRat) < j * 1.e-5)	// "small" increases with # iterations
			break;

		// if autoSizing this coil now, conditionally resize, call dxSetup (above), and return TRUE;

		if (!resizeCoolCoilIf( captWant, ahcc.bCaptRat))			// don't make smaller than start subhr bCaptRat. fcn above.
			break;							// if no size change, leave loop
		ret = TRUE;
	}
#endif

#ifdef DEBUG2	// can remove when works

// checks

	// due to .444*w*t term, hexf is not consistent with capt as computed for use below. don't check hexf. don't even compute hexf.
	//DBL capt2 = (hexf - ahcc.hen) * menFOn;			// compute capt for check
	//if (RELCHANGE( ahcc.capt, capt2) > Top.relTol/10.)
	//{
	// // investigating why this message occurs
	//   DBL hexf2 = psyEnthalpy(texf,wexf);
	//   DBL hen2 = psyEnthalpy(ten0,wen0);
	//   DBL capt3 = (wexf - wen0) * menFOn * 1061 + (texf - ten0) * cenFOn;
	//   DBL hh = (hena - ahcc.hen) * menFOn;
	//   RER( "airHandler '%s' DX cool coil capt inconsistency: \n"
	//        "    capt = %g, but menFOn*(hexf-hen) = %g.\n"
	//        "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g;  cs=%d,%d\n"
	//        "           hexf=%g hexf2=%g  hen=%g hen2=%g  (hena-hen)*menFOn=%g",
	//        name, ahcc.capt, capt2,  wen0, wena, we, wexf,  ten0, texf,  (INT)cs1,(INT)cs2,
	//        hexf, hexf2,  ahcc.hen, hen2,  capt3,  hh );
	//}

	// same as current computation above... weak check.
	//DBL capl2 = (wexf-wen0) * menFOn * 1061.;   			// compute latent capacity for check
	//if (RELCHANGE( ahcc.capt, ahcc.caps + capl2) > Top.relTol/10.)
	//   RER( "airHandler '%s' DX cool coil caps-capt inconsistency: \n"
	//        "    capt = %g, but caps=%g + 1061*menFOn*(wexf-wen)=%g = %g.\n"
	//        "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g;  cs=%d,%d",
	//        name, ahcc.capt, ahcc.caps, capl2, ahcc.caps + capl2,  wen0, wena, we, wexf,  ten0, texf,  (INT)cs1,(INT)cs2 );

	// restore if slopee-slopef test gives messages
	//if (te != ten0)
	//{  DBL efecT = (texf - ten0)/(te - ten0);
	//   if (RELCHANGE( ahcc.efecO, efecT) > Top.relTol/10.)
	//      RER( "airHandler '%s' DX cool coil inconsistency: \n"
	//           "    efecO = %g but (texf - ten)/(te - ten) = %g.\n"
	//           "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g;  cs=%d,%d",
	//           name, ahcc.efecO, efecT,  wen0, wena, we, wexf,  ten0, texf,  (INT)cs1,(INT)cs2 );
	//}

	// restore if slopee-slopef test gives messages
	//if (we < wen0 - .000003)							// == divides by 0
	//{  DBL efecW = (wexf - wen0)/(we - wen0);
	//   if (RELCHANGE( ahcc.efecO, efecW) > Top.relTol/10.)
	//      RER( "airHandler '%s' DX cool coil inconsistency: \n"
	//           "    efecO = %g but (wexf - wen)/(we - wen) = %g. \n"
	//           "                   wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g;  cs=%d,%d",
	//           name, ahcc.efecO, efecW,  wen0, wena, we, wexf,  ten0, texf,  (INT)cs1,(INT)cs2 );
	//}

	if ( we <= wen0					// if we > wen, wexf == wen, slopes not ==.
			&&  te != ten0  &&  texf != ten0 )
	{
		DBL slopee = (we - wen0)/(te - ten0);
		DBL slopef = (wexf - wen0)/(texf - ten0);
		if (RELCHANGE( slopef, slopee) > Top.relTol/10.)
			RER( (char *)MH_R1324, name, 			// "airHandler '%s' DX cool coil inconsistency: \n"
				 slopef, slopee,				// "    slopef = %g but slopee = %g.\n"
				 wen0, wena, we, wexf,  			// "           wen=%g wena=%g we=%g wexf=%g;\n"
				 ten0, texf,  (INT)cs1,(INT)cs2 );   	// "           ten=%g texf=%g;  cs=%d,%d"
	}
#endif

//--- DX coil actual exit temp and sensible part load ratio

	DBL plrSensWas = ahcc.plrSens;		// save prior sensible part load ratio: used re estimating plr
	if (texWant1 < texf)				// if can't cool to desired temp
	{
		ahcc.plrSens = 1.;  				// full fan-on sensible load
		tex = texf;					// best temp we can get
		coilLimited = TRUE;  				// can't get desired temp
	}
	else    						// can get desired temp
	{
		tex = texWant1;   				// limit output temp to desired
		ahcc.plrSens = (texWant1 - ten0)/(texf - ten0);	// fan-on sensible part load ratio
		//coilLimited = FALSE;  				preset by caller doCoils
	}

//--- DX coil exit humidity ratio  (also determines plr,qs,ql,q; but these are recomputed from wex/tex/cen in coilsEndSubhr)

	// Under part load, compressor & coil may cycle; must be modelled here to get correct wex cuz non-linear.
	// Note that partly-loaded coil/compressor cycles here during what is "coil on" time re fanCycles for rest of program.

	ahcc.qs = (tex - ten0) * cen;			// cool coil avg sens output: deltaT * avg flow: doesn't change in loop.
	DBL plrEst = (plrSensWas > 0. && ahcc.co_plr > 0.)		// don't div by 0, nor produce 0 output from non-0 input
				 ?  ahcc.co_plr * ahcc.plrSens / plrSensWas	// estimate part total load ratio per prior and plrSens change
				 :  ahcc.plrSens;				// else est ditto as plrSens: non-0 if here
	DBL frCOn = 					// compressor-on time fraction, for internal use & for cch, 10-92.
		plrEst < ahcc.minFsldPlr		// don't exceed 1, don't divide by 0
		?  plrEst/ahcc.minFsldPlr		// estimate cycling coil/compressor fraction on time (of time fan is on)
		:  1.;   				// not cycling, including minFsldPlr = 0., 9-92.
	DBL teCOn, weSatCOn;    				// actual-load effective temp, and saturated hum rat @ te, when coil on
	DBL frCOnWas;
	SI nIter1 = 0;
	DBL wexWas = 0.;			// re non-convergence message

	for (CONVERGER cvg; ; cvg.doit( frCOn, nIter1))	// iterate til plr converged: plr & wex are interdependent.
	{
		// do converger thing (cnah.cpp) after endtest (below) after each iteration.
		// TESTED 10-92: adding CONVERGER cures non-convergence here.
		// effective temp, and w if coil wet
		teCOn = ten0 + (tex - ten0)/(frCOn*ahcc.efecO);	// actual-load effective temp when coil on
		weSatCOn = psyHumRat3( max( teCOn,		// saturated w when coil on: get w for dew point temp: lib\psychro2.cpp
									-100. ) );		// don't exceed psyHumRat3 domain eg on small initial frCOn estimate 8-95

		// fan-on exit hum rat: when coil on, fraction efecO of wen in excess of weSatCOn is removed; when coil off, wex = wen.
		if (wen0 <= weSatCOn)				// if entering air too dry to condense at effective temp
			wex = wen0;					// coil is dry, removes no moisture
		else						// coil is wet
			wex= wen0 - ahcc.efecO*frCOn*(wen0-weSatCOn);	// removes fraction On time * effectiveness * w over saturation @ te

		// load and thence part total load ratio
		//ahcc.qs = (tex - ten0) * cen;			done above loop
		ahcc.ql = men * (wex - wen0) * PsyHCondWtr;	// average latent output: avg flow * flow-on delta-W * 1061. NEGATIVE.
		ahcc.q = ahcc.qs + ahcc.ql;			// cool coil total output, NEGATIVE.
		ahcc.co_plr = ahcc.q/(ahcc.capt*frFanOn);		// current fan-on fraction of full total load
		frCOnWas = frCOn;   				// save prior compressor-on time fraction for endtest
		frCOn = ahcc.co_plr < ahcc.minFsldPlr		// don't divide by 0, nor exceed 1.0.
				?  ahcc.co_plr/ahcc.minFsldPlr		// recompute frCOn for new plr, compressor cycling
				:  1.;				// frCOn is 1.0 when compressor not cycling
		if ( RELCHANGE( frCOnWas, frCOn)     			// if frCOn changed, iterate to compute wex/ql/q/plr/frCOn again
				< (teCOn < te ? Top.relTol/10. : Top.relTol) )	// teCOn < full-load te ("impossible" bad answer) --> smaller tol.
			break;						/* TESTED 11-1-92: above test eliminated 3/4 of SA11B13.INP
          							   consistency msgs (b4 tolerances increased) */
		if (nIter1 > MAXITER2)				// non-convergence check & msg
		{
			rer( (char *)MH_R1325, name,   		// "airHandler '%s':\n"
				 // "    DX coil exit humidity ratio convergence failure.\n"
				 ten0, wen0, tex,   			// "      inputs:  ten=%g  wen=%g  tex=%g\n"
				 wex, wexWas,   				// "      unfinished result: wex=%g  last wex=%g\n"
				 teCOn, weSatCOn,   			// "          teCOn=%g    weSatCOn=%g\n"
				 ahcc.co_plr, frCOn, frCOnWas );		// "          plr=%g  frCOn=%g  last frCOn=%g"
			break;
		}
		wexWas = wex;
	}

//--- compressor-on time fraction for subhour for crankcase heater. Copied to cch.frCprOn in coilsEndSubhr.

	ahcc.frCprOn = frFanOn 		// fraction of time fan is on (1.0 if not cycling): coil inactive when fan off
				   * frCOn;		// times fraction of that time compressor runs, 1.0 if plr cycling region.

#if defined( DEBUG2)
// devel aid only; remove most "soon".
// Still getting new warnings here, 10-92.
// made DEBUG2 4-22

// checks

	//if (!Top.isWarmup)  			/* increase the .0001's as necess to suppress uninteresting messages.
	//						   actual significance level is maybe .01 or .1. */
	{
		if (ahcc.capt >= 0.)
			RER( (char *)MH_R1326, name, ahcc.capt);	// "airHandler %s: Inconsistency #1: total capacity (%g) not negative"
		if (ahcc.caps >= 0.)
			RER( (char *)MH_R1327, name, ahcc.caps);	// "airHandler %s: Inconsistency #1a: sensible capacity (%g) not negative"
		if (ahcc.caps < ahcc.capt - .01)
			RER( (char *)MH_R1328, name, ahcc.caps, ahcc.capt);	// "airHandler %s: Inconsistency #2: sensible capacity (%g)\n"
		// "    larger than total capacity (%g)"
		if (wex > wen0 + .000001)
			RER( (char *)MH_R1329, name, wex, wen0);		// "airHandler %s: Inconsistency #3: wex (%g) > wen (%g)"
		if (ten0 < tex - .00001)
			RER( (char *)MH_R1330, name, tex, ten0);  		// "airHandler %s: Inconsistency #4: tex (%g) > ten (%g)"
		if (teCOn < te - .040)				/* .0001 ok most files 3..10-92; larger seen with sutter\SA11B13.INP.
       							   .002 is 4e-5 relative, ok cuz a psy fcn (psyHumRat3) is used re teCOn.
       							   -->.003 after seeing .0022 in AUSZ testing 7-7-95.
       							   -->.005 after seeing .0046 in saturation testing (T17) rob 5-97.
       							   -->.040 after Bruce got a .01 in one of his tests. */
			RER( (char *)MH_R1331, name, teCOn, te,		// "airHandler %s: Inconsistency #5: teCOn (%g) < te (%g)\n"
				 wen0, wena, we, wexf,  ten0, texf,  		// "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
				 ahcc.qs, ahcc.ql, ahcc.q,  			// "           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
				 ahcc.co_plr, ahcc.plrSens,  (INT)cs1,(INT)cs2 );
		if (weSatCOn < we - .000010)			/* .0000001 ok most files 3..10-92; larger seen with sutter\SA11B13.INP;
							   .0000012 seen in testing condensation 5-97 (humtst2.inp).
							   .0000025 produced by Bruce, 6-97. */
			RER( (char *)MH_R1332, name, weSatCOn, we,		// "airHandler %s: Inconsistency #6: weSatCOn (%g) < we (%g)\n"
				 wen0, wena, we, wexf,  ten0, texf,  		// "           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
				 ahcc.qs, ahcc.ql, ahcc.q,  			// "           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
				 ahcc.co_plr, ahcc.plrSens,  (INT)cs1,(INT)cs2 );
		if (wex < wen0)
		{
			if (wex < weSatCOn - .000001)
				RER( (char *)MH_R1333, name, wex, weSatCOn);	// "airHandler %s: Inconsistency #7: wex (%g) < weSatCOn (%g)"
			if (wex < we - .00001)
				RER( (char *)MH_R1334, name, wex, we);		// "airHandler %s: Inconsistency #8: wex (%g) < we (%g)"
			if (wex < wexf - .00001)
				RER( (char *)MH_R1335, name, wex, wexf);		// "airHandler %s: Inconsistency #9: wex (%g) < wexf (%g)"
		}
	}
#endif

//--- reduce iter4Fs xtrapolation bounds etc

	tPossC = ahccBypass * ten0 + (1. - ahccBypass) * texf;	/* achievable temp after bypass air remixed.  Used in
    								   cnah.cpp:AH::setTsSp1 to limit estimates to save iterations. */
	setToMax( lLim, min( texWant, (DBL)ahcc.minTEvap)); 	// tell caller exit temp won't go below desired value, nor min coil temp
	setToMin( uLim, ten0);					// nor above entering temp (5-29-92)
	return ret;
}			// AH::doDxCoil
//-----------------------------------------------------------------------------------------------------------------------------
void AH::coilsEndSubhr()		// coil end subhour calculations, for AH::endSubHr.

// sets: ahhc.q, .flueLoss, .p, .co_plr, .co_plrAv, .pAux.
//       ahcc.q, .qs, .ql,  .p, .co_plr, .co_plrAv, .pAux.
//       cch.p.
{
#if 0 // 10-96, 5-97 ... moved into dx coil case, below in this fcn
x// message now if (dx) coil had to adjust its input to prevent supersaturation, at convergence of tu's & ah's
x
x    if (ahcc.xLGain != 0.)			// if doDxCoil had to un-supersaturate entering air
x    {  ahcc.nSubhrsLX++;			// count subhours in which this occurred, for msg at end run
x       ahcc.xLGainYr += ahcc.xLGain;		// accumulate heat of condensation added to air to cure supersat, for msg.
x   #if 0 // 10-96 msg, now not error, wording wrong for 5-97 version.
x   x    rer( "airHandler '%s': Excess humidity: \n"
x   x         "    CSE had to assume a latent loss of %g Btu/hr \n"
x   x         "    to eliminate supersaturation of air entering the cooling coil. \n"
x   x         "    CSE does not simulate condensation. In your input, please specify \n"
x   x         "    sufficient infiltration or ventilation for this airHandler's zones.", 	// NUMS
x   x         name, ahcc.xLGain );
x   x    // continue now; run ended elsewhere if too many errors.
x   #endif
x       /* possible future 10-96: simulate condensation by checking saturation and adding at least some
x	  of heat of condensation (without error) at: tr/wr, tr1/wr1, tmix/wen, tex1/wex.
x	  5-97: did this at DX entry only - dx model won't run with sat air. */
x}
#endif

// init to no coil input or output energy

	// here 0 variables set below. Additional vbl's not set in if's below may be 0'd in else's.
	ahhc.q = ahhc.co_plr = ahhc.co_plrAv = ahhc.flueLoss = 0.;   		// init to no output from either coil
	ahcc.qs = ahcc.ql = ahcc.q = ahcc.co_plr = ahcc.co_plrAv = 0.;   		// ..
	ahhc.p = ahcc.p = ahhc.eir = ahcc.eir = 0.;				// init to no input for either coil
	ahhc.pAux = 0.;				// init to no coil-on auxiliary power for either coil
	ahcc.pAux = 0.;
	cch.frCprOn = 0;					// compr fraction on for crankcase htr: set in some coil cases below.

// calculate coil energy output

if (ahMode != ahOFF)				// if ON, either coil might be used regardless of ahHEATING/ahCOOLing
	{
		// (if ah off, may be leftover garbage in members such as tex, ten, cen.)
		if (coilUsed==cuHEAT)				// if heating coil is in use (set in doCoils)
			ahhc.q = (tex - ten) * cen;			// average (not fan-on) heat coil output = deltaT times average flow
		else if (coilUsed==cuCOOL)			// if cooling coil is in use
		{
			/* note this duplicates doDxCoil computation (except wen not wen0, 10-96);
			   needed for exceptions (no-flow, coilLockout, etc) and other coil types. */
			ahcc.qs = (tex - ten) * cen;			// cool coil average sensible output: deltaT times flow: NEGATIVE
			ahcc.ql = 1061. * men * (wex - wen);		// cool coil average latent output: NEGATIVE
			ahcc.q = ahcc.qs + ahcc.ql;			// cool coil average (not fan-On) total output, NEGATIVE.
		}
	}
	else						// ahMode==ahOFF: air handler off.
		coilUsed = cuNONE;				// no coil used: overwrite possible leftover value.

// calculate coil energy input and energy input ratio

#if 1
	if (ahhc.q > 0.)					// if heat used
#else	// isn't this better, given that we now have coilUsed? 12-3-92. UNDONE cuz didn't work well for cool, below. 12-3-92.
x    if (coilUsed==cuHEAT)				// if heat coil used, 12-3-92
#endif
	{
		switch ( Top.tp_pass1A && hcAs.az_active			// during autoSize pass 1 part A, if coil being autoSized,
				 ?  C_COILTYCH_ELEC  :  ahhc.coilTy )		// "electric" model was used.
		{
			// cases set .p and .co_plrAv, at least

		case C_COILTYCH_OIL:			// oil furnace "coil"
		case C_COILTYCH_GAS:			// gas furnace "coil"
			// runtime checks cuz exprs may be allowed for captRat, eirRat eg re flue loss simulation 7-6-92.
			if (ahhc.captRat <= 0)  rer( (char *)MH_R1336,		// "airHandler '%s':\n"
											 name, ahhc.captRat ); 	// "    heat coil capacity 'ahhcCaptRat' (%g) not > 0"
			if (ahhc.eirRat < 1.0)  rer( (char *)MH_R1337, 		// "airHandler '%s':\n"
											 name, ahhc.eirRat );		// "    heat coil full-load energy input ratio 'ahhcEirR' (%g) not > 1.0"

			// optional DOE2 gas/oil furnace part-load flueLoss.  Skip for speed if none (default 0; .flueLoss 0'd above).
			if (sstat[AH_AHHC + HEATCOIL_STACKEFFECT] & FsSET)	// if stack effect coeff given by user (unusual; fast test)
				ahhc.flueLoss 					// addl infil load due to hot flue when furnace off a la DOE2
				= (ahhc.captRat - ahhc.q)		//  = unused capacity (===total cap * fraction flame off)
				  * ahhc.stackEffect;		//    * value of user's input -- fcn of outdoor temp expected.
			// limits mechanism issues errMsg in exman.cpp if value not in range 0 to 1.
			// note implicit assumption that flame cycles at full power for part load.

			// gas/oil plr is figured with flueLoss added to load here, per DOE2 (rather than by adding it to zone infiltration)
			ahhc.co_plrAv = (ahhc.q + ahhc.flueLoss)/ahhc.captRat;	// (subhr average) part load ratio = fraction rated output
			//ahhc.co_plr = ahhc.co_plrAv/frFanOn;		(after switch)	furnace-on plr, expect 1.0 unless flow too low for ts sp.

			ahhc.p = ahhc.captRat			// gas furnace input power = rated output capacity,
					 * ahhc.eirRat			// times energy input ratio (heat input ratio) at rated load
					 * ahhc.pyEi.val(ahhc.co_plrAv);	// times value of polynomial to adjust to part load.
			// note default poly has a constant part for pilot, cycling cost, etc.
			break;					// (DOE2 method puts pilot input in auxFullOff).

		case C_COILTYCH_ELEC:     		// electric heat coil
			ahhc.p = ahhc.q;  				// input is output: 100% efficient === eir = 1.0.
			ahhc.co_plrAv = ahhc.q / ahhc.captRat;	// avg (not fan-on) part load ratio is fraction of rated power output
			//ahhc.co_plr = ahhc.co_plrAv/frFanOn;		is after switch
			break;

		case C_COILTYCH_HW:			// hot water coil. same as elec, optimizer will combine code.
			ahhc.p = ahhc.q;				// input (from HEATPLANT, do not post to meter!) is output
			ahhc.co_plrAv = ahhc.q / ahhc.captRat;	// avg part load ratio is fraction of rated power output
			/* for plrAv as fraction of capacity under current heatplant load,
			   set ahhc.cap in doHWCoil & divide by that here, 7-95. */
			//ahhc.co_plr = ahhc.co_plrAv/frFanOn;		is after switch
			break;

		case C_COILTYCH_AHP:			// air source heat pump
			ahhc.p = ahhc.pCpr;			// copy input power from member doAhpHeat sets
			//how define plr? fraction output: .q/.cap. fraction run time (larger): .frCprOn.
			ahhc.co_plrAv = ahhc.q / ahhc.capCon;
			cch.frCprOn = ahhc.frCprOn;		// copy compressor runtime for crankcase heater to use
			break;

			//case C_COILTYCH_NONE:		// if no coil, shd be no q
		default:
			rer( PWRN, (char *)MH_R1338, (UI)ahhc.coilTy);	// "cncoil.cpp:AH::coilsEndSubhr: Bad heat coil type 0x%x"
		}
		//if (ahhc.q)					add /0 protection add if need found
		ahhc.eir = (ahhc.p + ahhc.pSh + cch.p)/ahhc.q;	/* energy input ratio: input/output:
							   input includes AHP resistance heat, and crankcase heater input.*/
		//if (frFanOn)					add /0 protection if need found
		ahhc.co_plr = ahhc.co_plrAv/frFanOn;			// fan-on part load ratio (frac full pwr, or flame fractn on, when fan on)
		ahhc.pAux = ahhc.aux;
	}
	else						// no heat used -- ahhc.q is 0
	{
		if ( ahhc.captRat==0.				// if coil has 0 capacity (can result from autoSize)
				&&  !(Top.tp_sizing && hcAs.az_active)		// and isn't being autoSized
				&&  ahcc.coilTy != C_COILTYCH_AHP )		// and isn't AHP (not autoSizable and captRat may be unset)
		{
			// 0 size coils consume no auxiliary power, rob 11-1-96
			ahhc.pAux =     0.;			// no aux power
		}
		//zero variables otherwise unset (those set in above switch are 0'd before it)
		ahhc.pCpr = 0.;					// for cleanliness: pCpr used above in switch
		ahhc.pSh = 0.;					// necessary: pSh used on return to AH::endSubhr
		ahhc.qSh = 0.;					// necessary?
		ahhc.frCprOn = 0;				// for cleanliness
		ahhc.co_plr = ahhc.co_plrAv = 0;			// 7-95 now used in AH::endSubhr.
	}

#if 1
	if (ahcc.q != 0.)					// if cold used (or possibly warmth from cool coil, 12-3-92)
#else	// isn't this better, given that we now have coilUsed? 12-3-92. NO, UNDONE: q seen 0 when coil used (tex=ten=cpTs).
	// .. but then again, shouldn't aux's etc be as tho coil used when its output is 0 cuz cpTs==ten, 11-96?
	x    if (coilUsed==cuCOOL)				// if cool coil used (even if it heated the air), 12-3-92
#endif
	{
		ahcc.co_plrAv = ahcc.q/ahcc.capt;		// subhour average relative load (part load ratio).  q and capt both negative.
		ahcc.co_plr = ahcc.co_plrAv/frFanOn;		// fan-on rel load, same unless fan cycling (note capt = captRat @ setup for elec)

		switch ( Top.tp_pass1A && ccAs.az_active			// during autoSize pass 1 part A, if coil being autoSized,
				 ?  C_COILTYCH_ELEC  :  ahcc.coilTy )		// "electric" model was used.
		{
			// cases determine ahcc.p, at least.

		case C_COILTYCH_CHW:				// chilled water coil, supported by a COOLPLANT
			ahcc.p = -ahcc.chwQ;				// input is input from plant, made pos. Do not charge to meter!
			break;

		case C_COILTYCH_ELEC: 				// "electric": testing artifice.
			ahcc.p = fabs(ahcc.q);
			break;			// input is output: 100% efficient.

			//case C_COILTYCH_NONE:			// if no coil, expect no q --> don't get here.
		default:
			rer( PWRN, (char *)MH_R1339, (UI)ahcc.coilTy);	// "cncoil.cpp:AH::coilsEndSubhr: Bad cool coil type 0x%x"
			break;

		case C_COILTYCH_DX:       			// DX coil
			DBL plrCr =   /* energy input correction factor for partial load (during fan-on time)
		              (corrects full-load input; 'eir' in names is misnomer, claims rob, 5-92) */
				ahcc.co_plr <  ahcc.minFsldPlr  ?  ahcc.eirMinUnldPlr		// cycling: prorate false loading factor
				* ahcc.co_plr/ahcc.minFsldPlr	// .. by fraction of time compr on
				:  ahcc.co_plr <= ahcc.minUnldPlr  ?  ahcc.eirMinUnldPlr  		// false loading range: const load
				:  ahcc.co_plr <  1.               ?  ahcc.pydxEirUl.val(ahcc.co_plr)	// unloading, use polynomial
				:  1.0;
			ahcc.p = fabs(ahcc.capt) * ahcc.eirRat			// curr power input = full-load input (output * rated eir),
					 * frFanOn  					// times portion of subhour fan & coil are on 6-16-92
					 * plrCr						// times part-load correction factor computed just above,
					 * ahcc.pydxEirT.val( 				// times off-rated-temperatures correction factor.
						 max( ahcc.tWbEn, MINTWBEN),	//   use entering wetbulb of at least 60 (or as changed)
						 ahcc.tDbCnd);
#if 1 // 5-97
			if (ahcc.xLGain != 0.)			// if doDxCoil had to un-supersaturate entering air (5-97)
			{
				ahcc.nSubhrsLX++;			// count subhours in which this occurred, for msg at end run
				ahcc.xLGainYr += ahcc.xLGain;		// accumulate heat of condensation added to air to cure supersat, for msg.
			}
#endif
			cch.frCprOn = ahcc.frCprOn;		// copy compressor runtime for crankcase heater to use
			break;
		}
		//if (ahcc.q)						add /0 protection if need found
		ahcc.eir = (ahcc.p + cch.p)/fabs(ahcc.q);		/* energy input ratio: input/output. crankcase heater input
								   included in running coil, even tho also nz when no coil runs.*/
		/* use of plrAv in following aux computations yields results proportional to DX compressor on time ONLY if compressor
		   cycles at any fan-on plr < 1 (no unloading or false loading).  But anticipate no DX auxOn/auxOff uses anyway. 7-92. */
		ahcc.pAux = ahcc.aux;
	}
	else							// no coolth used: ahcc.q==0.
	{
		if ( ahcc.captRat==0.				// if coil has 0 capacity (can result from autoSize)
				&&  !(Top.tp_sizing && ccAs.az_active)		// and isn't being autoSized
				&&  ahcc.coilTy != C_COILTYCH_CHW )		// and isn't CHW (not autoSizable and captRat not used)
		{
			// 0 size coils consume no auxiliary power, rob 11-1-96
			ahcc.pAux =     0.;			// no aux power
		}
		//here 0 any vbls not 0'd above that shd be 0 if coil did not run.
		ahcc.frCprOn = 0.;				// cleanliness
		ahcc.co_plr = ahcc.co_plrAv = 0;			// 7-95 now used in AH::endSubhr.
		ahcc.xLGain = 0;					// 5-97 no condensation at entry to coil coil when off
	}

// compute crankcase heater input

	if (cch.cchCM != C_CCHCM_NONE)			// save time if cch absent (usual case)
		cch.endSubhr();					// uses cch.frCprOn, set by some coil type cases above, else 0.
}				// AH::coilsEndSubhr
//-------------------------------------------------------------------------------------------------------------
void CCH::endSubhr()			// determine crankcase heater energy input

// input used: cch.frCprOn, set per coil types that expect crankcase heater (AHP, DX, )
// output: cch.p, posted to meter in AH::endSubhr.
// called from AH::coilsEndSubhr
{
	switch(cchCM)		// determine power input to crankcase heater this subhour
	{
	case C_CCHCM_CONSTANT:
		p = pMx;
		break;			// always on

	case C_CCHCM_CONSTANT_CLO:
		p = pMx * (1. - frCprOn);
		break;	// on except when compr runs

	case C_CCHCM_PTC:           			// proportional oil-temperature control
	case C_CCHCM_PTC_CLO:				// .. with compressor lock-out
		if (frCprOn > 0.) 					// if ran at all in subhour, assume oil warm enuf for min heat
			p = (cchCM==C_CCHCM_PTC_CLO)  ?  pMn * (1. - frCprOn)   	// with lockout, runs at min when compr off, 0 when on
				:  pMn;   			// else runs at min entire subhour
		else							// compr off entire subhour
		{
			DBL tOil = Top.tDbOSh + dt;					// assume oil warmer than outdoors by user input "cchDt"
			if (tOil <= tMx)       p = pMx;				// if oil temp below lo setpoint, full cch power
			else if (tOil >= tMn)  p = pMn;				// oil above hi sp, min. >= prevents /0 if tMx==tMn.
			else p = pMn + (pMx - pMn)*(tOil - tMn)/(tMx - tMn);	// temp between, proportional power.
		}
		break;

	case C_CCHCM_TSTAT:         			// outdoor temp thermostat control & off when compressor on
		p = 0;						// heater input 0 if compr on or thermostat off
		if (frCprOn <= 0.)					// if compr did not run at all in subhour
		{
			if (Top.tDbOSh > tOff)      tState = FALSE; 	// update thermostat state, with differential
			else if (Top.tDbOSh <= tOn) tState = TRUE;		// ..
			if (tState)            p = pMx;			// if tstat on, cch is on
		}
		else tState = FALSE;		// running compr reverts hysteresis to OFF ?? <--- Rob's idea, run it past Bruce/Phil
		// cuz tstat is on compr output pipe (to implement lockout).
		break;
	}

	// cch.p is posted to cch.mtri in AH:endSubhr.

}		// CCH::endSubhr


/******************************************************* IF-OUTS *************************************************************/

// see cncoil.ifo for stuff removed 5-30-92, including prior DX coil code just replaced.
// see cncoil.bk1, 6-6-92

#if 0	// pre-frFanOn version, 6-15-92:
x//-----------------------------------------------------------------------------------------------------------------------------
xvoid AH::doElecHeatCoil(		// compute tex that electric heating coil can and will produce.
x
x   DBL texWant, 		// desired coil exit temp
x   DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten
x
x	// addl inputs:  ten, wen, cmix, .
x	// outputs:      tex, coilLimited.  wex remains wen, as preset by caller.
x	//	re estimating/endtests:  cPoss, tPossH
x
x	// called from doCoils.  Only called if texWant < ten and cen > 0.
x{
x    tPossH = ten + ahhc.captRat/cen;		/* possible exit temp = entry temp + power/heat cap flow.
x    						   used in cnah:AH::setTsSp1. */
x    cPoss = ahhc.captRat/(texWant-ten);      	/* possible flow = power/requested delta T, 4-19-92.
x    						   used in cnah:AH::iter4Fs endtest. */
x    if (tPossH < texWant)
x    {	tex = tPossH; 				// use best available temp
x       cPoss = cen; 				// this flow is possible (probably don't need to store when coilLimited)
x       coilLimited = TRUE;   			// can't get desired temp
x	}
x    else     					// can get desired temp at current flows, and flows up to cPoss.
x       tex = texWant;				// have enuf capacity; limit output temp to desired
x    setToMin( uLim, texWant);			// tex won't go above texWant
x    setToMax( lLim, ten);				// nor below ten
x    //wex: no change: heat coil does not change w.
x}				// AH::doElecHeatCoil
#endif
#if 0// old version of above. worked 6-14-92 in initial context.
x//-----------------------------------------------------------------------------------------------------------------------------
xvoid AH::doElecCoolCoil(		// compute tex [and wex] that "electric cooling coil" can and will produce.
x
x   DBL texWant1,		// desired coil exit temp (ignored if fcc (fan cycles with coil))
x   DBL texWant,			// ditto before adj for bypassing air, for use re lLim, uLim, tPossC, cPoss.
x   DBL &lLim, DBL &uLim )	// ts extrapolation bounds: narrowed here to range texWant..ten
x
x	// addl inputs:  fcc, ten, wen, cmix, .
x	// outputs:      tex, coilLimited.  wex remains wen, as preset by caller.
x	//	re estimating/endtests:  cPoss, tPossC
x
x	// called from doCoils.  Only called if texWant < ten and cen > 0.
x{
x    if (fcc) 	/* if fan runs only when coil on -- and:
x		   fan and coil run full power when on, and are on "frFanOn" of the subhour;
x    		   cen is subhour AVERAGE FLOW, thus cen/frFanOn is fan-on flow;
x    		   tex is COIL-ON TEMP. texWant is not used.
x    		   frFanOn (fraction time on) determined here as cen/rated max:
x    		     terminal requests fraction of (sfan) flow @ full power; we interpret as fraction of time on @ full power.
x    		   Only one terminal (enforced in setup): allowing addl terminals would require code to force same cpp/cMx. */
x    {
x       TU * ctu = &TuB.p[ahCtu];			// ah's control terminal
x       if (!ahCtu)							// ** enforce in cncult5, unless fcc becomes run-variable
x          rer( PABT, "fanCyCoil for airHandler '%s' is YES but no ahCtu has been given", name);		// no return
x
x       cPoss = min( ctu->cMxC, sfan.cMx);	// ruling max flow is smaller of tu's current cool max and sfan's (with overrun)
x       						// tu->cMxC is set in AH::ahVshNLims; sfan->cMx is set in FAN::pute.
x       						// sfan->pute is called AFTER coils if blowthru; xtra iter4Fs convergence ck needed?
x       						// add setup code to insure sfan.cMx is non-0 even for blowthru fan
x						// cPoss is used below and in cnah:AH::setTsSp1; is actual fan-on flow here.
x
x       if (cPoss==0.)  rer( PABT, "doElecCoolCoil: cPoss is 0");	// prevent /0; remove check here when insured elsewhere.
x
x       if (cen > cPoss) rWarn( PWRN, "doElecCoolCoil: cen (%g) > cPoss (%g)", cen, cPoss);	// tolerance??
x       						// dev aid warning cuz possible balance problems if cen/frFanOn != cPoss ??
x
x       frFanOn = min( 1.0, cen/cPoss);		// fraction on = fraction of ah poss output needed = requested flow / ah max flow
x
x       tPossC = ten + ahcc.c.captRat/cPoss;	// possible elec-coil full-power, full flow tex. May be used in cnah:AH::setTsSp1.
x						// /0: if cMx is 0, cen should be 0 --> don't get here.
x       tex = tPossC;				/* lowest possible tex is actual fan-on tex under fcc.
x       						   used in cnah:AH::iter4Fs endtest. */
x       coilLimited = (ctu->useAr==uMxC);   	/* if control terminal is pegged at max cooling, then coil capacity
x       						   (@ smaller of sfan, tu->cMx) is limiting zone temp */
x       setToMax( lLim, tex);			// tex won't go lower than lowest possible.
x	}
x    else 	// fan runs all the time; VAV modelled (or however terminals are defined)
x    {
x       tPossC = ten + ahcc.c.captRat/cen;   	/* possible delta T = power/heat cap flow.  captRat negative!
x       						   used in cnah:AH::setTsSp1. */
x       cPoss = ahcc.c.captRat/(texWant1 - ten);	/* possible flow = power/requested delta T. captRat negative!
x       						   used in cnah:AH::iter4Fs endtest. */
x       if (tPossC > texWant1)
x       {  tex = tPossC;				// best temp we can get
x          cPoss = cen;				// this flow is possible (probably don't need to store when coilLimited)
x          coilLimited = TRUE;  			// can't get desired temp
x		}
x       else    					// can get desired temp up to cPoss
x          tex = texWant1;   			// have enuf capacity; limit output temp to desired
x       setToMax( lLim, texWant);  			// tex1 won't go below texWant
x	}
x    setToMin( uLim, ten);				// nor above entering temp (5-29-92)
x    // possible error return above
x}				// AH::doElecCoolCoil
#endif

// end of cncoil.cpp
