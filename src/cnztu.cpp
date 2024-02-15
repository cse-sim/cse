// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnztu.cpp -- hvac overall and zone & terminal unit simulation routines for CSE

// see cnah.cpp for air handlers, cncoil.cpp for coils.

/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"

#include "ancrec.h"	// record: base class for rccn.h classes
#include "rccn.h"	// AH TU ZNR
#include "msghans.h"	// MH_R1250

#include "psychro.h"	// psyEnthalpy
#include "cuparsex.h"	// maxErrors. only, 4-92.
#include "exman.h"	// rer
#include "irats.h"	// TuiB

#include "cnculti.h"
#include "cuevf.h"

#include "cnguts.h"	// ZrB MDS_FLOAT

/*-------------------------------- DEFINES --------------------------------*/
#ifndef ER
#define ER(f)  { rc = (f); if (rc) goto er; } 	// if fcn f returns error, exit thru label "er:". Note RCOK is 0.
#endif

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL BOO FC tuHasFlow( TU *tu, int md);


/******************************************************************************************************************************
*  SIMULATION OVERALL CONTROL / HVAC GENERAL
******************************************************************************************************************************/

/* The tuSmart smart terminal problem:
	3-92 smart terminals dropped.
	7-92 TERMINAL.tuSmart edited out.
Intent was to model terminals that shut off flow when supply temp is on wrong side of zone temp.
BUT what happens when supply temp is in a float range where terminal is pegged-on??

  1.  Must split mode at tz==tSup, as for tz < tSup, flow is on (heat), but for tz > tSup, flow is off.

  2.  When tz==tSup, do what??  Volume has no effect on tz but will probably have 2nd-order effect of unpredictable sign on tSup.

      If reducing volume reduces tSup (eg less fan heat), (heat) volume 'snaps' to min or max -- stable, ok.

      BUT if reducing ah volume increases tSup, that would restore zone volume, and our current program might not converge.
      The equilibrium would be at partial flow where air handler can just deliver tSup = tz: need to organize so in mode
      air handler calculates flow it can deliver at tSup==tz, then set the zone's flow accordingly.  Further, need to organize
      to detect need to solve this way or somehow make work whether the feedback is positive or negative.

WHAT if we modify the smart terminal to shut off flow if supply temp is on wrong side of SETPOINT?
  Doing that just moves the problem to the setpoint temp: will still need to solve for volume at which ah can deliver tSup.

  ****** UNTIL details thought thru, don't bother to try to implement smart terminals. *******   */

//-----------------------------------------------------------------------------------------------------------------------------
//--- constants, also hard-coded in cncult2.cpp for Top.absTol; also in cnah.cpp:
const float RELoverABS = .01f;			// relative to absolute tolerance ratio: +-1 corrsponds to +- 1%.
const float ABSoverREL = 1.f/RELoverABS;	// reciprocal thereof
//---------------------------------------------------------------------------------------------------------------------------
RC FC hvacIterSubhr()

// Iterative (estimate-refine) part of hvac subhour computations for all zones/terminals/airHandlers

// caller does loads first, and various start-end subhour functions.
{
//error if more than this many iterations here
#undef QUIK
#ifdef QUIK
* #define MAXITER 70		// tired of waiting for 200 while debugging, 5-95...
#else
#define MAXITER 200		// T81 gets ok errors @50, not 100, 11-92,5-94. T44 gets ok error @50, not @70. 200 for margin.
#endif

#if defined( _DEBUG)
	const ZNR* zp1 = ZrB.GetAtSafe( 1);
#endif

	RC rc=RCOK;
	TU *tu;
	AH *ah;
	HEATPLANT *hp;
	COOLPLANT *cp;
	TOWERPLANT *tp;   //HPLOOP *hl;

// flags to indicate need to check xxClf's and xxPtf's of each class; when all clear, can exit.
// In each case setter also sets call-flag or compute-flag of specific device. */
	Top.ztuKf = TRUE;		// unconditionally check the terminals once: many ztuCf/spCf sets don't ztuKf++;
							// some are done via exman change-flag mechanism, where setting two flags would be slow.
// don't need to init the rest: FALSE from prior subhr; xxEstimate usually sets to insure doing class once.
//    Top.ztuKf=FALSE;		// set by ah or hp (local heat) if change may impact a terminal
//    Top.ahKf=FALSE;		// set by hp/cp/hl if need to redo AH's
//    Top.cpKf=FALSE;	   	// set by towerplant if need to redo coolplants
//    //Top.hlKf=FALSE;		// set by hp or towerplant if need to redo HPLOOPs upward
//    Top.tpKf=FALSE;		// set if COOLPLANT/[HPLOOP] change may require recomputing TOWERPLANTs

// save entry values of autoSizable quantities re conditional backtrack of unstable increases at end subHour, & other init
	// with this subhr-level logic the ah aCMn,xH,C members are probably not needed <-- removing 7-22-95.
	RLUP( TuB, tu)
	{	tu->bVfMn = tu->tuVfMn;     // add lh when need found.
		tu->bVfMxH= tu->tuVfMxH;
		tu->bVfMxC= tu->tuVfMxC;
	}
	RLUP( AhB, ah)
	{	ah->bVfDs = ah->sfan.vfDs;
		ah->ahhc.bCaptRat = ah->ahhc.captRat;
		ah->ahcc.bCaptRat = ah->ahcc.captRat;
		ah->ahRun = FALSE;	// say ah not computed yet this subHr: tSup is old or ahEstimated.
							// May disable autoSize increases in TU::resizeIf Set in AH::iter4Fs.
	}		

// estimate all hvac classes. ER macro does "goto er" (below) if error returned
	RLUP( TuB, tu)  ER( tu->tuEstimate() )	// estimate terminals (re ZN and ZN2 for ah's, else may do nothing)
	RLUP( AhB, ah)  ER( ah->ahEstimate() )	// estimate air handlers: establishes initial tSup input for terminals
	RLUP( HpB, hp)  ER( hp->hpEstimate() )	// estimate HEATPLANTs: turn on & flag loads if scheduled to turn on
	RLUP( CpB, cp)  ER( cp->cpEstimate() )	// estimate COOLPLANTs: turn on & flag loads if scheduled to turn on, est tSup.
	RLUP( TpB, tp)  ER( tp->tpEstimate() )	// estimate TOWERPLANTs: est tSup.

// compute (refine) til stable enuf
	for (Top.iter = 0; ; )
	{	// conditionally compute terminals, by zone
		if (Top.ztuKf)
		{	Top.ztuKf = FALSE;
			ZNR* zp;
			RLUP(ZrB, zp)
			{
				if (zp->spCf || zp->ztuCf)			// if setpoint change or other zone/load/terminals change
				{
					ER(zp->ztuCompute())  		// refine (compute) all terminals in zone. ER: goto er; if not RCOK.
						ZnresB.p[zp->ss].curr.S.nIter++;   	// count zone (tu) iterations at subhour level
				}
			}
		}

		/* do air handlers and plants in "forward" order, exit if no changes pending
		   Each sets class xxKf flag, and object xxClf or xxPtf flag, if it changes something affecting an object.
		   Also, coil loads of demand-driven plants call cpOn()/hpOn() to turn on plant, if nec, to avoid an iteration. */

		//conditional forward calls new 10-15-92, watch for bugs due to lacking flagging.

		if (Top.ahKf || Top.ahKf2)
		{	Top.ahKf = Top.ahKf2 = FALSE; 			// -2 flags: recall ah after ztuCompute
			RLUP(AhB, ah)
			{
				if (ah->ahPtf || ah->ahPtf2 || ah->ahClf)
					ER(ah->ahCompute())
			}
		}
		if (Top.cpKf)
		{	Top.cpKf = FALSE;
			RLUP( CpB, cp) if (cp->cpPtf || cp->cpClf)
				ER( cp->cpCompute())
		}
		//if (Top.hlKf) { Top.hlKf = FALSE; RLUP( HplB,hl) if (hl->hlPtf || hl->hlClf) ER( hl->hlCompute()) }
		if (Top.hpKf)
		{	Top.hpKf = FALSE;
			RLUP( HpB, hp) if (hp->hpPtf || hp->hpClf) ER( hp->hpCompute())
		}
		if (Top.tpKf)
		{	Top.tpKf = FALSE;
			RLUP( TpB, tp) if (tp->tpPtf || tp->tpClf) ER( tp->tpCompute())
		}

		// Conditionally do flagged plants & ah's in reverse order to accel info flow back to terminals
		// trying both-directions info flow, 9-29-92. HEATPLANTs worked without this; faster esp w TOWERPLANTs??
		// TESTED 9-29-92 .327: heatplants only, or no plants: insignif difference (T0,4,27,44,75).
		// NEEDS TEST on runs with CHW/COOLPLANT/TOWERPLANT, when impl -- otta add speed.
		//   hp's don't need doing in reverse order: intervening objects (tp) don't affect them.
		//if (Top.hlKf) { Top.hlKf = FALSE; RLUP( HplB,hl) if (hl->hlPtf || hl->hlClf) ER( hl->hlCompute()) }
		if (Top.cpKf)
		{	Top.cpKf = FALSE;
			RLUP( CpB, cp) if (cp->cpPtf || cp->cpClf)
				ER( cp->cpCompute())
		}
		if (Top.ahKf)
		{	Top.ahKf = FALSE;    // not ahKf2 here
			RLUP( AhB, ah)
				if (ah->ahPtf || ah->ahClf)
					ER( ah->ahCompute())
		}
		//TESTED 9-29-92: removing "if (Top.ahKf)" slows performance (hp only, T0,4,27,44,75).

		// if no check-class flags on, now done.
		if ( !Top.ztuKf && !Top.ahKf && !Top.ahKf2 && !Top.cpKf /*&& !Top.hlKf*/ && !Top.tpKf )
			break;

		// terminate loop after too many iterations
		if (++Top.iter >= MAXITER)			// rer: display runtime message with date/time. errCount++'s. exman.cpp.
		{	rer( MH_R1250, Top.iter);
			break;   	// "Air handler - Terminals convergence failure: %d iterations"
		}

		// terminate run on too many errors (that continue execution at their source -- eg from AH::setTsSp1)
		if (errCount() > maxErrors)			// errCount(): error count ++'d by calls to err, rer, etc. rmkerr.cpp.
		{
			rc = rInfo( 					// display runtime "Information" message, exman.cpp. returns RCBAD.
					 MH_R1251, 		// handle for text "More than %d errors.  Terminating run."
					 maxErrors );		// maxErrors: cuparse.cpp. Data init, accessible as $maxErrors.
			goto er;
		}
	}
er:			// errors come here via ER macros to do following exit stuff

// undo autoSizing size increases that aren't used in the final equilibrium
	if (Top.tp_sizing)				// if autoSizing now -- else values not changed since saved
	{	// terminals. No nonlinearities-->downSize can be deferred to end subhour, ie here.
		RLUP (TuB, tu)
		{	ah = AhB.p + tu->ai;
			if (ah->airxTs)					// don't divide by 0
			{	float vf = tu->cv/ah->airxTs;			// current terminal cfm: convert from heat cap units
				// if (tu->asHcSame)  assume bVfMxH and C already same.
				if (tu->vhAs.az_unsizeIf( max( tu->bVfMxH, vf))) 	// if being autosized, downsize to larger of entry max
					tu->cMxH = tu->tuVfMxH * ah->airxTs;		//   or current cfm, if larger than both.
				if (tu->vcAs.az_unsizeIf( max( tu->bVfMxC, vf))) 	// same for cooling max cfm
					tu->cMxC = tu->tuVfMxC * ah->airxTs;		// if did it, recompute corress limit in heat cap units from cfm
				if ( tu->asKVol   					// if autoSizing constant volume
						&&  !Top.tp_pass1A					// in part A, leave tuVfMn 0: insurance
						&&  !ah->fcc )					/* if ah fan cycles, leave tuVfMn 0 here
								   (disallowed at input time except certain ahTsSp=<expr> cases) */
				{
					tu->tuVfMn = max( tu->vhAs.az_active ? tu->tuVfMxH : 0.f,  	// min flow is the larger max flow being autoSized
									  tu->vcAs.az_active ? tu->tuVfMxC : 0.f );	// .. asHcSame also on if both being autoSized.
					tu->cMn = tu->tuVfMn * ah->airxTs; 				// heat cap units value for tuVfMn (cfm value)
				}
			}
		}

		// air handlers
		// to keep modelling accurate, fan and coils are dynamically sized down to beginning subhour value,
		// as well as upward, as needed, from ahCompute & its subrs. Thus downsize is not needed here --
		// and (for coils) would be hard to do -- cap'y to deliver load must be determined.
#if 0	// 7-95. fan part ran, coils never completed.
x		RLUP (AhB, ah)
x		{	if (ah->fanAs.az_active)
x				ah->resizeFansIf( ah->sfan.vfDs 			// reduce (or increase) to this (current) size
x                             * Top.airX(ah->sfan.t),   	//   converted to heat cap units as resizeFansIf expects
x                             ah->bVfDs );			// but not less than this (start subhr) size (cfm)
x			if (ah->hcAs.az_active)
x				ah->resizeHeatCoilIf( * * *  need size that will deliver current load
x                                 ah->ahhc.bCaptRat );		// but not less than this (start subr) size
x			if (ah->ccAs.az_active)
x				ah->resizeCoolCoilIf( * * * need size that will deliver current load
x                                 ah-.ahcc.bCaptRat );		// but not less than this (start subr) size
x		}
#endif
	}

	return rc;					// error returns above, including E macros
}		// hvacIterSubhr

/******************************************************************************************************************************
*  HVAC: ZONES AND TERMINALS
******************************************************************************************************************************/
//  Terminal autoSizing stuff (mainly) called from autoSizing stuff in cnausz.cpp
//=============================================================================================================================
/*virtual*/ RC TU::RunDup(		// duplicate 
	const record* pSrc,		// source (input) record
	int options /*=0*/)		// options
{
	RC rc = record::RunDup(pSrc, options);

	if (!rc)
		rc |= tu_Setup();

	return rc;

}		// TU::RunDup
//-----------------------------------------------------------------------------
static RC addZhx(	// add ZHX (Zone HVAC transfer) for TU, probably later ZNR (vent)

	TI ui,		// owning terminal if HVAC, 0 for vent
	TI zi,		// zone for terminal or vent (ie always)
	TI ai,		// air handler subscript for Ar HVAC zhx's, else 0
	ZHXTY ty,	// LhSo, LhStH, ArSo, ArStH, ArStC, ..   cndtypes.def/dtypes.h (via rcdef) or as changed
	NANDAT sp,	// tstat set point if needed.  Cast float argument with "V".
	SI spPri,	// tstat set point priority if needed (determines activation order of ZHXs with same .sp)
	const char* name,	// NULL or name (eg of terminal) to copy into zhx
	TI* xip)	// NULL or receives subscript of added zhx

// includes chaining the record appropriately for u1, zi, ai, ty (assumes tu, zone, ah RUN records all present).

// CAUTION: may not errCount++, be sure error return propogated back to cul:cuf.

// caller does: sets .xiCool, .xiHeat if non-0, .

{
	RC rc /*=RCOK*/; 	// rc is used in CSE_E macro

	// make a ZHX
	ZHX* zhx;
	CSE_E(ZhxB.add(&zhx, WRN))	// add a ZHX record to base ZhxB, allocating block if necessary. lib\ancrec.cpp.
	// leave front mbrs .ty, .li, .file, .line unset (0).  does this cause runtime errmsg troubles?
	// CAUTION: al may not errCount++, be sure error return propogated back to cul:cuf.

	// record owner is zone
	zhx->ownTi = zi;		// tentative 3-92 with added ratLudo ambiguous check

// store caller's data
	if (name) 
		zhx->name = name;
	zhx->zhxTy = ty;
	CSE_V zhx->sp = sp;		// CSE_V macro stores into float as tho it is a void * to prevent float error on NAN (expr handle)
	zhx->spPri = spPri;
	zhx->ui = ui;
	zhx->zi = zi;
	zhx->ai = ai;

	TI xi = zhx->ss;					// fetch subscript for several uses

// chain by zone, including for terminals, with addl chain for cmStxx (tstat-ctrl'd) zhx's only
	if (zi)									// insurance: zone subscript always expected
	{
		ZNR* zp = ZrB.p + zi;								// point to zhx's zone's record
		zhx->nxZhx4z = zp->zhx1;
		zp->zhx1 = xi;  					// put on (head of) zone zhx1 chain
		if (ty & St)
		{
			zhx->nxZhxSt4z = zp->zhx1St;      // put on (head of) zone cmStxx chain
			zp->zhx1St = xi;
		}
	}

	// chain Ar zhx's by air handler
	if (ai)									// if this zhx is associated with air handler
	{
		AH* ah = AhB.p + ai;				// point to zhx's ah's RUN record
		zhx->nxZhx4a = ah->zhx1;
		ah->zhx1 = xi;  					// put on (head of) ah all-zhx chain
	}

	// return subscript
	if (xip)  *xip = xi;
	return rc;				// each CSE_E macro also can return
}			// addZhx
//-----------------------------------------------------------------------------
RC TU::tu_Setup()			// check and set up terminal record: call for each terminal after input

{
	RC rc = RCOK;
	AH* ah = NULL;
	ZNR* zp = NULL;
	rc |= ckOwnRefPt(&ZrB, this, (record**)&zp); 	// insurance check of ref to tu's zone, get zone ptr zp
	float tfanVfDs = -1.f;				/* for conditional tfan design flow dflt if air heat/cool & needed inputs
								   given, constant, and > 0. (no default if variable, 0, or if lh only) */

#if !defined( CRTERMAH)
	if (zp->zn_IsConvRad())
	{
		oer("Zone has znModel=%s and thus cannot be served by a terminal",
		zp->getChoiTx(ZNI_I + ZNISUB_ZNMODEL));
	}
#endif


	//============== determine capabilities specified ===============

	if ((sstat[TU_TUTLH] | sstat[TU_TUQMNLH]) & FsSET)		// terminal has local heat if lh setpoint or min q given
		cmLh = (sstat[TU_TUTLH] & FsSET) ? cmStH : cmSo;	// if sp given, is set temp (tstat ctrl), else set output
	else
		cmLh = cmNONE;  						// else no local heat. say so (value 0)

	cmAr = cmNONE;  							// say no air heat/cool / init to 0 for bit setting
	if ((sstat[TU_TUTH] | sstat[TU_TUTC] | sstat[TU_TUVFMN]) & FsSET)	// has air heat and/or cool if min flow or either sp given
		if ((sstat[TU_TUTH] | sstat[TU_TUTC]) & FsSET)		// if either setpoint given, is tstat controlled, set bits
		{
			cmAr = cmNONE;						// insurance
			if (sstat[TU_TUTH] & FsSET)  cmAr = TCCM(cmAr | cmStH);    	// if heating sp given, set "set temp heating" bit
			if (sstat[TU_TUTC] & FsSET)  cmAr = TCCM(cmAr | cmStC);  	// .. cooling bit.  both form cmStBOTH.
		}
		else
			cmAr = cmSo;   					// no setpoints means constant output
	//else cmAr = cmNONE;   					// else no air heat/cool: value 0, preset above


	//============= check local heat members ================

	if (cmLh == cmNONE)
	{

		// no-local-heat checks: errors for lh members given

		rc |= disallowN( 						// prohibit entry of list of fields
				  MH_S0600,				/* "when terminal has no local heat\n"
									   "    (when neither tuTLh nor tuQMnLh given)" */
				  TU_TUQMXLH, TU_TUPRILH, TU_TULHNEEDSFLOW, 0);

		if (sstat[TU_TUHC + HEATCOIL_CAPTRAT] & FsAS)				// if "AUTOSIZE tuhcCaptRat" given
			oer("Cannot autoSize tuhcCaptRat when terminal has no local heat\n"
				 "    (when neither tuTLh nor tuQMnLh given)");		// NUMS

		rc |= tuhc.setup(C_COILAPPCH_TUHC, this, TU_TUHC, 0, 1);	// check coil variables for prohibited terminal coil
	}
	else
	{
		// local-heat-present checks

		tuhc.setup(C_COILAPPCH_TUHC, this, TU_TUHC, 1, 0);		// check/set up required terminal local heat coil

		if ((sstat[TU_TUQMXLH] & (FsSET | FsVAL)) == (FsSET | FsVAL))			// if max lh power given & set (constant)
			if ((sstat[TU_TUHC + HEATCOIL_CAPTRAT] & (FsSET | FsVAL)) == (FsSET | FsVAL))	// if coil power given and set (constant)
				if (tuQMxLh > tuhc.captRat)						// do the user a favor
					oWarn(
						   MH_S0601,		// "tuQMxLh = %g will be ineffective because tuhcCaptRat is less: %g"
						   tuQMxLh, tuhc.captRat);

				if (cmLh == cmSo)
				{

					// set output local heat checks

					rc |= disallowN(MH_S0603,		/* "when local heat is not thermostat controlled\n"
												   "    (when tuTLh is not given)" */
									 TU_TUQMXLH, TU_TUPRILH, 0);

					if (sstat[TU_TUHC + HEATCOIL_CAPTRAT] & FsAS)		// if "AUTOSIZE tuhcCaptRat" given
						oer("Cannot autoSize tuhcCaptRat when local heat is not thermostat controlled\n"
							 "    (when tuTLh is not given)");					// NUMS
				}
				else
				{

					// setpoint controlled local heat checks / setup

					/* require max heat spec: else omission yields mysteriously inoperative local heat.
					   wouldn't it be nicer to default it to the coil capacity if given? 12-92. */
					rc |= require(MH_S0604, TU_TUQMXLH);  // "when terminal has thermostat-controlled local heat\n"
																   // "    (when tuTLh is given)"
					// terminal local heat coil autoSizing
					if (sstat[TU_TUHC + HEATCOIL_CAPTRAT] & FsAS)		// if "AUTOSIZE tuhcCaptRat" given
					{
						switch (tuhc.coilTy)					// check now for implemented type
						{
						case C_COILTYCH_HW:
						case C_COILTYCH_ELEC:
							hcAs.az_active = TRUE;
							break;	// for autoSizable coil type(s) set runtime Boolean
							//case C_COILTYCH_NONE:
						default:
							break;
						}
						// terminal won't use over tuQMxLh coil Btu's, so if const 0 give error now instead of mysterious failure.
						// (and if tuQMxLh < tuhcCaptRat during run, msg issued after autoSizing.)
						if (Top.tp_autoSizing)							// if autoSizing phase 7-21-95
							if ((sstat[TU_TUQMXLH] & (FsSET | FsVAL)) == (FsSET | FsVAL))		// if max lh power given & set (constant)
								if (tuQMxLh == 0)
									oer("Cannot autoSize tuhcCaptRat with tuQMxLh = 0.");	// NUMS
					}
				}
	}

	//================== disallow members for absent air heat/cool ================

	// non-tstat-air-heat, non-tstat-air-cool checks

	if (!(cmAr & cmStH))							// any value but cmStH or cmStBOTH
	{
		rc |= disallowN(MH_S0605, TU_TUVFMXH, TU_TUPRIH, 0); 	// "when tuTH not given"
		if (sstat[TU_TUVFMXH] & FsAS)
			rc |= oer("Cannot autoSize tuVfMxH when tuTH not given.");	// NUMS
	}
	if (!(cmAr & cmStC))							// any value but cmStC or cmStBOTH
	{
		rc |= disallowN(MH_S0606, TU_TUVFMXC, TU_TUPRIC, 0);  	// "when tuTC not given"
		if (sstat[TU_TUVFMXC] & FsAS)
			rc |= oer("Cannot autoSize tuVfMxC when tuTC not given.");	// NUMS
	}

	if (sstat[TU_TUVFMN] & FsAS)						// if AUTOSIZE tuVfMn given
	{
		if (!(cmAr & (cmStH | cmStC)))							// no set temp air heat nor cool
			rc |= oer("Cannot autoSize tuVfMn when neither tuTH nor tuTC not given."); 		// NUMS
		if (!(sstat[TU_TUVFMXH] & FsAS) && !(sstat[TU_TUVFMXC] & FsAS)) 		// if AUTOSIZING neither air heat nor cool
			rc |= oer("Cannot autoSize tuVfMn when autoSizing neither tuVfMxH nor tuVfMxC.");	// NUMS
	}

	if (sstat[TU_TUVFMXHC] & FsSET)						// if  tuVfMxHC = same|different  given
		//if (tuVfMxHC==C_DIFFSAMECH_SAME)					for errors only if 'same' given... & reword msgs
	{
		if ((cmAr & cmStBOTH) != cmStBOTH)					// if not both set temp air heat and cool
			rc |= oer("Cannot give tuVfMxHC without both tuTH and tuTC.");	// NUMS
		else if (!((sstat[TU_TUVFMXH] & FsAS)
			&& (sstat[TU_TUVFMXC] & FsAS))) 				// if NOT AUTOSIZING BOTH air heat and cool
			rc |= oer("Cannot give tuVfMxHC without \n"
					   "    both \"AUTOSIZE tuVfMxH\" and \"AUTOSIZE tuVfMxC\".");	// NUMS
	}

	if (!IsAusz( TU_TUHC + HEATCOIL_CAPTRAT))
		rc |= disallow("when tuhcCaptRat is not autoSized", TU_FXCAPH);	// NUMS

	if (!IsAusz(TU_TUVFMXH) && !IsAusz(TU_TUVFMXC)) 	// if NO air flow autosizes for tu
		rc |= disallow("when no terminal air flows are being autoSized", TU_FXVFHC);  		// NUMS

	if (cmAr == cmNONE)   						// no air heat nor air cool
	{
		rc |= disallowN(MH_S0607,	// "when terminal has no air heat nor cool\n"
											// "    (when none of tuTH, tuTC, or tuVfMn is given)"
						 TU_AI, TU_TUSRLEAK, TU_TUSRLOSS,
						 TU_TFANSCH, TU_TFANOFFLEAK, TU_TFAN + FAN_FANTY, 0);
	}
	else
	{
		//=============== check present air heat and/or cool ================

		rc |= requireN(MH_S0608,				/* "when terminal has air heat or cool\n"
									   "    (when any of tuTH, tuTC, or tuVfMn are given)" */
						TU_AI, 0);					// member(s) required for air heat
		//if (!rc)?
		rc |= ckRefPt(&AhB, ai, "tuAh", NULL, (record**)&ah);		// check air handler ref, to an ah RUN record

		// default design flow and tfan Ds flow to larger of min, heat mx, cool mx flow given constant or autoSized values.

#if 1 //7-10-95 & code setting/using ausz
		/* story: tuVfDs defaulting to support autoSizing and require no additional input 7-10-95:
				1) don't expect fan overload during autoSize phase: if tu being autoSized, AH::asFlow is on;
		  if not, there is tuVfDs default or required input.
				2) but just in case, during autoSizing ah code defaults 0 tuVfDs dynamically to max( tuVfMn, MxH, MxC).
				3) at end autoSize a default value for main sim may be put in tu->tuVfDs (without FsSET|FsVAL).
				4) fall thru here again at reSetup, any constant default also applied, nz value remains for ah. 6-95. */

		BOO ausz = FALSE;					// TRUE if any autoSize's given: defer default
#endif
		SI vbl = FALSE;					// TRUE if any variable exprs given: don't default
		float def = 0.f;					// use only > 0 values
		if (sstat[TU_TUVFMN] & FsSET)     			// if minimum flow given
			if (sstat[TU_TUVFMN] & FsAS)			// if being autoSized 7-95, its not constant nor variable
				ausz++;						// set flag
			else if (sstat[TU_TUVFMN] & FsVAL)			// else if set now (thus constant)
				def = tuVfMn;					// use it as default (if > 0)
			else  vbl++;					// not set, not autoSized --> must be variable, can't default
		if (cmAr & cmStH)					// heat max only pertinent with tstat ctrl'd heat
			if (sstat[TU_TUVFMXH] & FsSET)   			// if heat max flow given
				if (sstat[TU_TUVFMXH] & FsAS)			// if being autoSized 7-95, its not constant nor variable
					ausz++;					// set flag
				else if (sstat[TU_TUVFMXH] & FsVAL)		// if set now (thus constant)
					setToMax(def, tuVfMxH);  			// use it as default if larger
				else  vbl++;					// not set --> must be variable, can't default
		if (cmAr & cmStC)					// cool max only pertinent with tstat ctrl'd cool
			if (sstat[TU_TUVFMXC] & FsSET)   			// if cool max flow given
				if (sstat[TU_TUVFMXC] & FsAS)			// if being autoSized 7-95, its not constant nor variable
					ausz++;					// set flag
				else if (sstat[TU_TUVFMXC] & FsVAL)		// if set now (thus constant)
					setToMax(def, tuVfMxC);  			// use it as default if larger
				else  vbl++;					// not set --> must be variable, can't default
		/* if any exprs given, don't use the constants or autoSizes -- require input.
		   if only constants or autoSizes given, use max for main sim.
		   if both constants & autoSizes given, don't store the constants b4 ausz phase: leave 0 for dynamic default.
		   ausz results applied elsewhere (tu_auszFinal()) at end ausz phase. */
		if (!vbl						// if found NO variable exprs for tuVfMn, -MxH, or -MxC
				&& (def > 0.f 					// and found a constant value usable as default
					|| ausz))					//     or an autosized value (will be applied to dflt b4 main sim)
		{
			// then tuVfDs need not be input
						setToMax(tfanVfDs, def);				// use default for terminal fan default (max: insurance)
						// complete tfanVfDs code like tuVfDs when tfan implemented. 7-95.
						if (!(sstat[TU_TUVFDS] & FsSET))      		// if design flow not given
				if (ausz)					// when constant default and also autosize default,

					if (Top.tp_autoSizing)	// don't apply constant default at start autoSizing:
											//   leave 0 for dynamic default in cnah2.cpp.
						setToMax(tuVfDs, def);  			// apply constant default found to design air flow
						// use 'floor' to take max cuz tu_auszFinal() may put max
						// autoSized tuVfMxH,C back into INPUT RECORD tuVfDs as partial main sim default. 6-95.
		}
			else   						// no tuVfDs default found or variable exprs also found
			rc |= require(TU_TUVFDS);				// design flow must be input when no clear default. explain?


		// require max heating cfm tuVfMxH if heat setpoint tuTH given, else disallow. and tuVfMxC likewise.

		if (sstat[TU_TUTH] & FsSET)
			rc |= require(MH_S0691, TU_TUVFMXH);	// "\n    when terminal has thermostat-controlled air heat "
															// "(when tuTH is given)"
		else
			rc |= disallow(MH_S0605, TU_TUVFMXH);	// "when tuTH not given"
		if (sstat[TU_TUTC] & FsSET)
			rc |= require(MH_S0692, TU_TUVFMXC);	// "\n    when terminal has thermostat-controlled air cooling "
															// "(when tuTC is given)"
		else
			rc |= disallow(MH_S0606, TU_TUVFMXC);	// "when tuTC not given"

		// disallow tuSRLoss if return not ducted

		if (zp)						// if zone reference ok
			if (!zp->i.plenumRet)				// if ducted return zone, S to R loss disallowed
			{
				disallow(MH_S0609, TU_TUSRLOSS);		// error if given  *** check wording when plenums done ***
															// "when zone plenumRet is 0"
				tuSRLoss = 0.f;					// set to 0 (is RUN RECORD!) (CULT defaults non-0)
			}

		if (cmAr == cmSo)
		{

			// set output air heat/cool checks.  tuVfMn is set (it got us here), but may be 0 at runtime.

			if ((sstat[TU_TUVFMN] & (FsSET | FsVAL)) == (FsSET | FsVAL))	// if tuVfMn given by user and now set (thus constant)
				tfanVfDs = tuVfMn;					// use it as terminal fan flow default
		}
		//else { tstat controlled air heat and/or cool checks ... could go here. }

		}	// end of  "if ... else  / check present air heat and/or cool"

	if (rc)   return rc;		// enough for this terminal if any error

	//================= checks re terminal fan ===================

	// note don't know if fan is for LH or air heat/cool til tfanSched evaluated at run time.

	if (cmLh == cmNONE && cmAr == cmNONE)    			// if useless terminal (and no error if here)
	{
		rc |= disallowN(MH_S0610,   	/* "when terminal has no capabilities: \n"
						"    no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given"*/
						 TU_TFANSCH, TU_TFANOFFLEAK,
						 TU_TFAN + FAN_FANTY, 0);		// only tfanType checked; if user deletes that,
		// will get to FAN::setup for rest of fan subrecord member msgs.
		if (rc)  return rc;			// don't setup fan if tfanType may be set without adding "prohibited" argument
	}
	rc |= tfan.fn_setup(C_FANAPPCH_TFAN, this, TU_TFAN, tfanVfDs, NULL);	// check/set up opt'l tu fan; pass dflt its ds flow

	if (tfan.fanTy == C_FANTYCH_NONE)				// if no fan specified
	{
		rc |= disallowN(MH_S0611, 			// "when tfanType = NONE"
						 TU_TFANSCH, TU_TFANOFFLEAK, 0);	// disallow these variables
		tfanOffLeak = 0.f;					// no fan back-leakage (this is RUN record!)(insurance)
		CHN(tfanSch) = C_TFANSCHVC_OFF;			// schedule of absent fan is OFF (cnah code may assume)
	}
	else							// have fan
	{
		// tfanOffLeak is defaulted non-0 by CULT.
		rc |= require(MH_S0612, TU_TFANSCH); 	// schedule required "when tfanType not NONE"
	}

	//=============== useless terminal check ===============

	if (rc == RCOK && cmLh == cmNONE && cmAr == cmNONE)
	{
		oWarn(MH_S0613);	/* "Ignoring terminal with no capabilities: \n"
					   "    no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given" */
		return rc;
	}

	//=============== set up terminal, with more checks ===============

	if (rc)   return rc;					// do not attempt setup with bad members, bad zp, bad ah, etc

	// add zhx (zone hvac transfer) records for each capability of terminal.  cncult2.cpp:topZn cleared ZhxB.

	if (cmLh == cmSo)
		CSE_E(addZhx(ss, ownTi, 0, LhSo, 0, 0, Name(), &xiLh))    		// local heat, set output
	else if (cmLh == cmStH)
		CSE_E(addZhx(ss, ownTi, 0, LhStH, CSE_V tuTLh, tuPriLh, Name(), &xiLh))	// local heat, set temp

		if (cmAr == cmSo)
			CSE_E(addZhx(ss, ownTi, ai, ArSo, 0, 0, Name(), &xiArH))    		// air heat/cool set out. Will be linked as heat.
		else
		{
			if (cmAr & cmStH)
				CSE_E(addZhx(ss, ownTi, ai, ArStH, CSE_V tuTH, tuPriH, Name(), &xiArH))  	// set temp air heat
				if (cmAr & cmStC)
					CSE_E(addZhx(ss, ownTi, ai, ArStC, CSE_V tuTC, tuPriC, Name(), &xiArC))  	// set temp air cool: can have both zhx's
		}
	if (xiLh)
	{
		ZhxB.p[xiLh].xiArH = xiArH;     // cross-link
		ZhxB.p[xiLh].xiArC = xiArC;
	}
	if (xiArH)
	{
		ZhxB.p[xiArH].xiLh = xiLh;      // ..
		ZhxB.p[xiArH].xiArC = xiArC;
	}
	if (xiArC)
	{
		ZhxB.p[xiArC].xiLh = xiLh;      // ..
		ZhxB.p[xiArC].xiArH = xiArH;
	}

	// terminal air heat/cool autosizing setup. Don't get here on inappropriate inputs as checked above.

	if ((sstat[TU_TUVFMXH] & FsAS) || (sstat[TU_TUVFMXC] & FsAS))	// if air heat or cool autoSize requested
	{
		if (sstat[TU_TUVFMXH] & FsAS)  vhAs.az_active = TRUE;  	// set runtime flags
		if (sstat[TU_TUVFMXC] & FsAS)  vcAs.az_active = TRUE;

		ah->asFlow = TRUE;				// say a tu of ah's is being flow-sized: may change part A supply temp.

		if (tuVfMxHC == C_DIFFSAMECH_SAME)		// heat/cool max air flow same/different choice (default _DIFF)
		{
			asHcSame = TRUE;   			// convert to Boolean for convenience
			// note don't get here if not autoSizing both tuVfMxH and -C: error'd above.
		}
		if (sstat[TU_TUVFMN] & FsAS)			// request to autoSize min air flow means to autoSize for const volume.
		{
			asKVol = TRUE;							// set runtime constant volume flag.

			if ((sstat[TU_TUVFMXH] & FsAS) && (sstat[TU_TUVFMXC] & FsAS))	// if autoSizing both heat and cool
				asHcSame = TRUE;   						// constant volume implies heat and cool the same.

			/* error if if ahFanCycles is runtime variable,
			   cuz when FALSE, autoSized tuVfMn is tuVfMx, but when TRUE, non-0 tuVfMn causes runtime error.
			   (See AH::setup for errors for autoSize tuVfMn with const ahFanCycles=YES or ahTsSp=ZN.
				If ahFanCycles varies cuz default varies cuz ahTsSp varies, there is only the runtime check.) */
			if ((ah->sstat[AH_AHFANCYCLES] & (FsSET | FsVAL)) == FsSET)		// if set but doesn't have value yet
			{
				USI evf;
				if (exInfo(EXN(ah->ahFanCycles), &evf, NULL, NULL) != RCOK	// get expr's evaluation freq, exman.cpp / if fail
						|| evf > EVFRUN)						// if varies more than at start of run
					rc |= oer("When autoSizing tuVfMn,\n"
							   "    airhandler %s's ahFanCycles value cannot be runtime variable.",
							   ah->Name());
			}
		}
	}

	// chain TU's by zone
	if (zp)						// insurance
	{
		nxTu4z = zp->tu1;  				// tu pts to prior chain (or 0 if no prior)
		zp->tu1 = ss;					// zone chain hd pts to cur tu
		if (tu_IsAirTerminal())
			zp->zn_airTerminalCount++;
	}
	// chain by air handler
	if (ah)
	{
		nxTu4a = ah->tu1;
		ah->tu1 = ss;						// all-tu's-for-ah chain
	}

	// set up change flag setting by expression manager -- makes table entries if given fields contain non-constant expressions

	// set zone setpoint change flag on changes in ...
	chafN(&ZrB, zp->ss, offsetof(ZNR, spCf), TU_TUTH, TU_TUTC, TU_TUTLH, 0);

	// limit changes set "zone terminal change" flag
	//   causes tu recompute even if limit change does not change flow: ok, as expect few scheduled limit changes.
	//   if tu capability absent, will be no exprs.
	chafN(&ZrB, zp->ss, offsetof(ZNR, ztuCf), TU_TUQMNLH, TU_TUQMXLH, TU_TUVFMN, TU_TUVFMXH, TU_TUVFMXC, 0);

	return rc;				// additional returns above
}				// TU::tu_Setup
//--------------------------------------------------------------------------------------------------

void TU::fazInit( int isAusz)		// terminal beginning of phase initialization  6-95
{
// initialize AUSZ substructs re stuff that may be autoSized. Note setup (cncult5.cpp) set .az_active's per user AUTOSIZE cmds.
	hcAs.az_fazInit( &tuhc.captRat, FALSE, this, TU_TUHC+HEATCOIL_CAPTRAT, isAusz, "TU[%s] hc");	// local heat coil
	vhAs.az_fazInit( &tuVfMxH,      FALSE, this, TU_TUVFMXH,               isAusz, "TU[%s] vh");   	// air heat max flow
	vcAs.az_fazInit( &tuVfMxC,      FALSE, this, TU_TUVFMXC,               isAusz, "TU[%s] vc");   	// air cool max flow

// other stuff: for main sim set _As & _AsNov variables not in AUSZ's when NOT being autoSized.
	// for convenience if probed, and ? for "sizing" report with no autoSizing phase.
	// here so set even if no autoSizing phase.
	if (!isAusz)
	{	if (sstat[TU_TUVFMN] & FsVAL)		// if tuVfMn has value (not expr, not being autoSized, not omitted)
			tuVfMn_As = tuVfMn_AsNov = tuVfMn;	// copy to places where autoSize would leave value
	}

// other stuff: init autoSizing peaks not in AUSZ's
	if (isAusz)				// -As variables must last through main simulation for reports (6-95)
	{	// primary init of the following is now in TU::p2Init, 7-11-95
		qhPkAs = 0;			// init autosizing air heat peak heat flow (as opposed to cfm recorded via AUSZ .vhAs)
		qcPkAs = 0;			// ditto cooling. negative quantity, +1 indicates unset.
		dtLoHAs = dtLoCAs = FALSE;	// flags set if to little supply temp to autoSize tu flow in any converged des day
	}

// other stuff
	lhMxMx = 0.;			// init terminal max tuQMxLh's re warning at end autoSize if not > tuhcCaptRat.
}			// TU::fazInit
//-----------------------------------------------------------------------------------------------------------------------------
void TU::rddiInit()	// initialization done ONCE for main sim or EACH DES DAY ITERATION for ausz: clear peaks. 6-95.

{
	// called from cnausz:asRddiInit after calling AUSZ::rddiInit's, 7-95.
	// redundant calls occur and must be harmless.
// clear non-AUSZ-member peak variables
	qhPk = 0;			// init air heat peak heat flow (as opposed to cfm which is handled via AUSZ .vhAs).
	qcPk = 0;			// cooling ditto. negative quantity.
	dtLoH = dtLoC = FALSE;	// flags set if to little supply temp to autoSize tu flow in any subHr this day iteration 7-14-95
}		// TU::rddiInit
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_SetupSizes()	// init object's models for possibly changed sizes being autoSized

// called from cnausz.cpp:begP1aDsdIter, TU::ah_pass1AtoB, cnausz.cpp:begP2DsdIter, TU::tu_p2EndTest
// etc after sizes possibly changed.
{
	// terminals don't require setup after size changed (not even coils), but handle tuVfMn and asHcSame.

// if autoSizing for constant volume, set min flow: has no AUSZ object
	if (asKVol)					// if autoSizing constant volume
	{
		AH* ah = AhB.p + ai;
		if (Top.tp_pass1A)
		{	// in pass 1 part A, vav const-temp modeling always used.
			//tuVfMn = 0.f;	//   ztuMode may not use tuVfMn, but want to overwrite the NAN
			tuVfMn = ah->oaVfDsMn / tu_fxVfHC;
		}
		else
		{	// in pass 1 part B, and pass 2, constant volume is used if requested

			// if air handler fan cycles, leave min flow 0 or unchanged here.
			// (autoSize tuVfMn with ahFanCyles=YES or expr with evf >= EVFRUN error'd in cncult5.cpp.
			// But believe default variation cuz ahTsSp varies between ZN and other value is not detected
			if (ah->fcc)				// if ah fan cycles this hour
				;						// leave tu min flow 0; or, if non-0 (due to varying fcc),
										//  gets runtime error on cnah1.cpp:AH::begHour.
			else							// fan does not cycle
			{
				tuVfMn = max(vhAs.az_active ? tuVfMxH : 0.f,  	// min flow is the larger max flow being autoSized
							vcAs.az_active ? tuVfMxC : 0.f);	// .. asHcSame also on if both being autoSized.
#if 1	// 5-15-2022
				// limit TU min flow to AH min OA flow
				//  TODO: enhance to handle multiple TUs served by AH
				if (tuVfMn < ah->oaVfDsMn)
					tuVfMn = ah->oaVfDsMn / tu_fxVfHC;
#endif
			}
		}
	}

// enforce autoSizing air heat and cool to same max flow
	if (asHcSame)				// if autoSizing both air heat and cooling and to same value
	{
		if (tuVfMxH < tuVfMxC)			// increase the smaller value to the larger
			tuVfMxH = tuVfMxC;			// .. insurance here -- model (ztuMode) should already have done this.
		else
			tuVfMxC = tuVfMxH;

		// Also, make sure they are not lower than minimum based on OA requirements
		if (tuVfMxH < tuVfMn)
		{
			tuVfMxH = tuVfMn;
			tuVfMxC = tuVfMn;
		}
	}

// change flags etc
	ZrB.p[ownTi].ztuCf++;			/* set tu's zone's terminal-change flag to insure recompute
						   after possible changes here or by caller.
						   unconditional set may be unnecessary but is good insurance. */
	return RCOK;
}			// TU::tu_SetupSizes
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_pass1AtoB()	// call at transition from autoSize pass 1 part A to part B for each design day

// called between iterations of a design day (between tp_SimDay()'s without reInit)
// at the change from const-temp size-finding open-ended models to the real models. */
{
	// at entry, value is set to max converged part A value (.a).

	// here do any optimizations to make value a better estimate of rated size.
	// But for terminals, rated size is same as load, which is the part a value. Nothing to do.

	// on return, caller will increase value to any larger converged part B value (.b), then call TU::begP1b, next.
	return RCOK;
}			// TU::tu_pass1AtoB
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_begP1b()		// called b4 start of pass 1 part b iterations for des day, after value set to max part B value seen.
{
// reSetup models being autoSized, cuz AUSZ stuff and/or tu_pass1AtoB above may have changed values.
	RC rc = tu_SetupSizes();			// just above

// change flags
	//ZrB.p[ownTi].ztuCf++;				tu_SetupSizes did this.
	return rc;
}			// TU::tu_begP1b
//-----------------------------------------------------------------------------------------------------------------------------
void TU::tu_p2Init()		// init repeated before each "pass 2" pass thru design days: 0 peaks
{
	/* it is necessary to re-init peak info each pass because load can fall if item resized,
	   for example ZN/ZN2/WZ/CZ TU flow: tSup set for 90% flow, flow can with tuVfMx til tSup reaches limit. */

// caller has already done p2Init() for AUSZ's.

// other stuff: init autoSizing peaks not in AUSZ's
	qhPkAs = 0;				// init autosizing air heat peak heat flow (as opposed to cfm recorded via AUSZ .vhAs)
	qcPkAs = 0;				// ditto cooling. negative quantity, +1 indicates unset.
	dtLoHAs = dtLoCAs = FALSE;		// flags set if to little supply temp to autoSize tu flow in any converged des day 7-14-95
}			// TU::tu_p2Init
//-----------------------------------------------------------------------------------------------------------------------------
void TU::endP2Dsd()	// called at end pass 2 design day, after converged. records new high sizes, loads, & plr's not in AUSZ's.
{
// record non-AUSZ-member autosizing peak values
	setToMax( qhPkAs, qhPk); 		// tu autoSizing peak air heat heat flow (as opposed to cfm)
	setToMin( qcPkAs, qcPk);   		// cooling ditto. Negative value.
	dtLoHAs |= dtLoH;			// flag set if too little dT to size tu heat vf on a cvgd des day iter
	dtLoCAs |= dtLoC;			// .. cool ..    merge final des day iter flag with AutoSizing-result flag
}			// TU::endP2Dsd
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_p2EndTest()		// autoSize pass 2 end test

// if any plr too small, adjust corresponding size and set Top.tp_auszNotDone to invoke repetition of pass
// (in addition, caller invokes another pass if any quantity increased)
{
	/* oversize sizes can result from the modified models used in pass 1 part A, particularly if AH::asFlow. Examples:
	   1. fan heat always max'd would make cooling coil oversize if coil peak wasn't at flow peak.
	   2. min (eg ventilation) flow on cooling terminal on WZ ah may have exagerated effect under fixed atTs
	      in pass 1 part A, resulting in oversize heating facilities for the zone. */

	float auszLoTol  = 1.f - Top.auszTol;		// 1 - autoSizing tolerance: subexpr used multiple places below.
	float auszLoTol2 = 1.f - .5f*Top.auszTol;		// 1 - half of autoSizing tolerance

// local heat
	if ( hcAs.az_active					// if autoSizing local heat coil
			&&  hcAs.plrPkAs					// and value's plr is non-0: divide by 0 protection
			&&  tuhc.captRat 					// and value is non-0: else unused resource, don't adjust, would /0.
			&&  hcAs.plrPkAs < auszLoTol				// if value's part load ratio < 1 - tolerance
			&&  hcAs.plrPkAs * hcAs.xPkAs / tuhc.captRat < auszLoTol )	/* .. even when adjusted to current size
								   in case smaller last des day or model reduced captRat, 6-97 */
	{
		hcAs.az_NotDone();				// plr too small. say must repeat pass 2.
		DBL was = tuhc.captRat;			// for debugging message, 6-97
		setToMin( tuhc.captRat, hcAs.xPkAs * hcAs.plrPkAs / auszLoTol2); 	// adjust coil size to bring plr to 1 - half tolerance
		// ... (expect tuhc plr linearly related to capacity).
		if ( Top.verbose > 2 && !Top.tp_auszNotDone // at verbose = 3 show the first not done thing, rob 6-97
				||  Top.verbose > 3)				// at verbose = 4 show all not dones, rob 6-97
			screen( 0, "      TU[%s] p2EndTest  hc  plr%6.3g  capt%8g -->%8g", Name(), hcAs.plrPkAs, was, tuhc.captRat );
	}

// max air flows
	if (asHcSame)	// if sizing air heat and cool max flows together
	{
		float hcPlr = max( vhAs.plrPkAs, vcAs.plrPkAs);				// adjust size per larger part load ratio
		float hcXPk = vhAs.plrPkAs > vcAs.plrPkAs ?  vhAs.xPkAs : vcAs.xPkAs;	// rated flow at which this plr saved
		float hcVfMx = max( tuVfMxH, tuVfMxC);					// use larger flow (expect same; insurance)
		if ( vhAs.az_active && vcAs.az_active			// if autoSizing heat & cool (insurance)
				&&  hcPlr  &&  hcVfMx				// and plr and flow non-0
				&&  hcPlr < auszLoTol				// if part load ratio < 1 - tolerance
				&&  hcPlr * hcXPk / hcVfMx < auszLoTol )	// even when adjusted to current size, in case smaller, 6-97
		{
			vcAs.az_NotDone();  			// plr too small. say must repeat pass 2.
			DBL was = tuVfMxH;				// for debugging message
			tuVfMxH= tuVfMxC= hcXPk * hcPlr / auszLoTol2;	// adj max flows to bring plr to 1 - tol/2 (linearity expected)
			if ( Top.verbose > 2 && !Top.tp_auszNotDone // at verbose = 3 show the first not done thing, rob 6-97
					||  Top.verbose > 3)				// at verbose = 4 show all not dones
				screen( 0, "      TU[%s] p2EndTest vh+c plr%6.3g  vfMx%8g -->%8g", Name(), vhAs.plrPkAs, was, tuVfMxH );
		}
	}
	else		// heat and cool being sized separately
	{
		if ( vhAs.az_active					// if autoSizing air heat
				&&  vhAs.plrPkAs  &&  tuVfMxH 			// and value and its plr are non-0
				&&  vhAs.plrPkAs < auszLoTol				// if air heat's part load ratio < 1 - tolerance
				&&  vhAs.plrPkAs * vhAs.xPkAs / tuVfMxH < auszLoTol )	// .. even when adjusted to current flow (in case less) 6-97
		{
			vhAs.az_NotDone();  			// plr too small. say must repeat pass 2.
			DBL was = tuVfMxH;				// for debugging message
			tuVfMxH = vhAs.xPkAs * vhAs.plrPkAs / auszLoTol2;	// adj max flow to bring plr to 1 - half of tol (expect linear).
			if ( Top.verbose > 2 && !Top.tp_auszNotDone // at verbose = 3 show the first not done thing, rob 6-97
					||  Top.verbose > 3)				// at verbose = 4 show all not dones
				screen( 0, "      TU[%s] p2EndTest  vh  plr%6.3g  vfMx%8g -->%8g", Name(), vhAs.plrPkAs, was, tuVfMxH );
		}
		if ( vcAs.az_active					// if autoSizing air cool
				&&  vcAs.plrPkAs  &&  tuVfMxC			// and value and its plr are non-0
				&&  vcAs.plrPkAs < auszLoTol				// if air cool's part load ratio < 1 - tolerance
				&&  vcAs.plrPkAs * vcAs.xPkAs / tuVfMxC < auszLoTol )	// even when adjusted to current size (in case now less) 6-97
		{
			vcAs.az_NotDone();  			// plr too small. say must repeat pass 2.
			DBL was = tuVfMxC;				// for debugging message
			tuVfMxC = vcAs.xPkAs * vcAs.plrPkAs / auszLoTol2;	// adj max flow to bring plr to 1 - half of tol (expect linear).
			if ( Top.verbose > 2 && !Top.tp_auszNotDone // at verbose = 3 show the first not done thing, rob 6-97
					||  Top.verbose > 3)				// at verbose = 4 show all not dones
				screen( 0, "      TU[%s] p2EndTest  vc  plr%6.3g  vfMx%8g -->%8g", Name(), vcAs.plrPkAs, was, tuVfMxC );
		}
	}

// min flow and re set up models for possibly changed sizes
	RC rc = tu_SetupSizes();				// above. Sets tuVfMn if asKVol, does any other reInit.
	// believe redundant: begP2DsdIter also calls.
	return rc;
}			// TU::tu_p2EndTest
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_endAutosize()			// terminal stuff at end successful autosize  6-95
{
// warnings for probable input errors. Warn if tuQMxLh apparently limited autoSized tuhcCaptRat.
	if (hcAs.az_active)
		if (lhMxMx <= tuhc.captRat)
			oWarn( "AutoSized tuhcCaptRat (%g) limited by tuQMxLh,\n"
				   "    which never exceeded %g during autoSizing.",
				   tuhc.captRat, lhMxMx );

// ... Warn if dT fell below 1 degree F (and flow would have run away) while autoSizing tuVfMxH
	if (vhAs.az_active)				// believe redundant
		if (dtLoHAs)				// from dtLoH, from dtLoHSh, from ztuMode().
		{
			AH *ah = AhB.p + ai;			// point tu's air handler
			const char* sub{ ""};
			if (ah->ahhc.coilTy==C_COILTYCH_NONE)				// if no heat coil
				sub = "Perhaps this is because airHandler %s has no heat coil.";			// NUMS's
			else if (!(ah->sstat[AH_AHHC+AHHEATCOIL_CAPTRAT] & FsAS))		// if ah's heat coil not autoSized
				sub = "Probably airHandler %s's non-autoSized heat coil\n"
					  "    is too small. Try increasing ahhcCaptRat.";
			else								// else what?
				sub = "Check whether airhandler %s's heat coil\n"		// ?
					  "    is scheduled off when needed.";
			oWarn(
				   "Couldn't autoSize tuVfMxH correctly:\n"
				   "    there was not enough supply heat from airHandler to meet demand.\n"
				   "    %s",
				   strtprintf( sub, ah->Name()) );				// format air handler name into subMessage
		}

// ... Warn if dT fell below 1 degree F (and flow would have run away) while autoSizing tuVfMxC
	if (vcAs.az_active)				// believe redundant
		if (dtLoCAs)				// from dtLoC, from dtLoCSh, from ztuMode().
		{
			AH *ah = AhB.p + ai;			// point tu's air handler
			const char* sub{ "" };
			if (ah->ahcc.coilTy==C_COILTYCH_NONE)				// if no cool coil
				sub = "Perhaps this is because airHandler %s has no cooling coil.";			// NUMS's
			else if (!(ah->sstat[AH_AHCC+COOLCOIL_CAPTRAT] & FsAS))	// if cool coil not autoSized
				sub = "Probably airHandler %s's non-autoSized cooling coil\n"
					  "    is too small. Try increasing ahccCaptRat and ahccCapsRat.";
			else								// else what?
				sub = "Check whether airhandler %s's cooling coil\n"	// ?
					  "    is scheduled off when needed.";
			oWarn(
				   "Couldn't autoSize tuVfMxC correctly:\n"
				   "    there was not enough supply coldness from airHandler to meet demand.\n"
				   "    %s",
				   strtprintf( sub, ah->Name()) );				// format air handler name into subMessage
		}

	return RCOK;
}			// TU::tu_endAutosize
//-----------------------------------------------------------------------------------------------------------------------------
void TU::tu_auszFinal() 			// called after autoSize to complete & store results

{
	// called from cnausz.cpp:cgAusz(), 6-95

	/* Copy autoSizing results to:
	   1) member of same object with same name + _As.
	      This member has minimal variation to facilitate probing for main simulation input.
	   2) before multiplying by oversize, copy to member with same name + _AsNov, for use in "sizing" report.
	      This value is consistent with the other report columns, that show peak load etc during autoSizing.
	   3) INPUT RECORDS, so they will appear in regenerated runtime records used for main simulation.
	      (if no main sim requested, input records may already have been deleted).

	   Note if NOT autoSized, fazInit's set _As and _AsNov from constant x's so
	   happens if no ausz phase, and so get 0 not last value for expr. */

	TU* tui = (ss <= TuiB.n) ? TuiB.p + ss	// point to input record for this runtime record
			  : NULL;		// if tu's subscript invalid, record is gone.

// member function sets _As, _AsNov, and input record members for each AUSZ's 'x'.
	hcAs.az_final( this, tui, tu_fxCapH);
	vhAs.az_final( this, tui, tu_fxVfHC);
	vcAs.az_final( this, tui, tu_fxVfHC);

// additional stuff: tuVfMn (has no AUSZ)
	if (asKVol)					// if autoSized for const volume, tuVfMn was autoSized
	{
		tuVfMn_AsNov = tuVfMn;			// save non-oversized copy (for consistency; not in report 7-95).
		tuVfMn *= tu_fxVfHC;			// apply oversize factor
		tuVfMn_As = tuVfMn;				// save copy of size, including oversize
	}

// additional stuff: copy to input record, to persist thru re-setup for main sim
	if (tui)					// if input record still exists
	{	if (asKVol)			// if autoSized for const volume, tuVfMn was autoSized. Else don't disturb input.
		{
			tui->tuVfMn = tuVfMn;     		// copy tuVfMn stuff
			tui->tuVfMn_As = tuVfMn_AsNov;	// ..
			tui->tuVfMn_AsNov = tuVfMn_AsNov;	// ..
		}
		tui->qcPkAs = qcPkAs;  			// copy peak heat flows (Btuh) (where AUSZ's handle cfm)
		tui->qhPkAs = qhPkAs;			//   .. values 0 & moot if not autoSized.
		//qcPk, qhPk: need not be copied: will be overwritten by main sim run (if none, no re-setup)

		// include autoSized tuVfMxH,C in tuVfDs default for main simulation

		// to make main sim faster & more consistent than the dynamic default that allows omitting tuVfSs for autoSize, 6-95.

		// tfanVfDs, when implemented, will require similar defaulting.

		if (!(tui->sstat[TU_TUVFDS] & FsSET))   			// if tuVfDs not given in input
		{	if (vhAs.az_active)  setToMax( tui->tuVfDs, tuVfMxH); 	// if heat max cfm ausz'd, use as design flow default if largest
			if (vcAs.az_active)  setToMax( tui->tuVfDs, tuVfMxC); 	// if cool max cfm ausz'd, use as design flow default if largest
			/* cncult5.cp:TU::setup now defaults tuvfDs with setToMax() function
			   so effective main sim default is to largest of input and autoSized tuVfMn, MxH, MxC. 6-95. */
		}
	}
}			// TU::tu_auszFinal
//=============================================================================================================================
// Terminal autoSizing stuff called from modelling function
//=============================================================================================================================
// Derivation of 'cfLim' argument to TU::resizeIf (next):
//    Limit nonconvergent tuVfMxH runaway when eg ah heat coil not being autosized is too small
//    by heating only to 1 degree below supply temp.
//         cz  =   q/dT  =   (sp * b - a)/(ts - sp)          zone flow computation
//                     ((ts - 1) * b - a)/(ts - (ts - 1))    replaced sp with (ts - 1) for heating
//                      (ts - 1) * b - a                     simplified
//                            ts * b - a                     -1 omitted: close enough. heating result.
//    Similarly for cooling, cool only to 1 degree above supply temp
//         cz  =   q/dT  =   (sp * b - a)/(ts - sp)          zone flow computation
//                     ((ts + 1) * b - a)/(ts - (ts + 1))    replaced sp with (ts + 1) for cooling
//                     ((ts + 1) * b - a)/(-1)               simplified
//                      a - (ts + 1) * b                     ..
//                            a - ts * b                     +1 omitted: close enough
//							note that cooling value is negative of heating value.
//-----------------------------------------------------------------------------------------------------------------------------
BOO TU::resizeIf( 			// conditionally resize terminal to use given flow

	DBL &_cv, 		// desired flow in heat cap units. May be reduced eg to prevent runaway.
	BOO isCooling,	// TRUE to resize cMxC/tuVfMxC, FALSE for H's.
	DBL cvLim, 		/* 0 or flow limit to prevent runaway when tSup approaches zone temp for underpowered coil.
			   compute as "tSup * b - a"; negated here for cooling; must be consistent amoung callers.
			   If limits flow, cv altered and dtLoH/CSh set to invoke tu_endAutosize message.
			   Not used if 0 or wrong sign (as when tSup already on wrong side of tz). */
	BOO decrOnly )	/* TRUE to downsize only.  In particular, to not increase to max + tolerance
			   --> creeping size increase, ah/tu non-convergence, fp error. 8-28-95. */

// terminal may be resised (changing cMxH/C and tuVfMxH/c) to larger flows,
//   or to smaller flows not less than start-subhour values.
// also can update cMn (if const vol), and other of H and C (if autoSizing heat & cool the same).

// returns TRUE if ok to proceed with flow cv (possibly modified: ref arg: caller must recompute cz etc.)
// returns FALSE if not a change, not autoSizing function now, etc.
//  on FALSE return, caller should restrict cv to existing cMn <= cv <= cMxH/C range.

{
	// callers 8-95: cnztu:ZNR::ztuMode (4 calls); cnah2:AH::antRatTs (2 calls)

// checks, adjustments, accesses...
	AUSZ& as = isCooling ? vcAs : vhAs;		// autoSizing substruct for air cool or air heat
	if (!as.az_active)
		return FALSE;				// not autoSizing this function
	if (ai==0)
		return FALSE;				// tu has no ah (unexpected)
	AH *ah = AhB.p + ai;			// point terminal's air handler
	if (ah->airxTs==0)
		return FALSE;				// prevent divide by 0 (unexpected): say limit to existing max flow for now

	float vf = _cv/ah->airxTs;			// convert heat cap flow to cfm @ current ah supply temp, as in ahVshNLims.
	if (!asKVol)  				// unless autosizing const vol (max===min)
		vf *= Top.auszHiTol2; 			// add a margin (if cv is pegged at max, presume decrOnly flag given).

	float& vfMx =  isCooling ? tuVfMxC : tuVfMxH;	// volumetric flow (cfm) limit to change, for air cool or heat
	float& bVfMx = isCooling ? bVfMxC : bVfMxH;		// beginning-subhour value, set in hvacIterSubhr: lower limit of decrease
	DBL& cMx =   isCooling ? cMxC : cMxH;		// heat cap units flow (like cv) that goes with vfMx

// limitations on increases
	if (vf > vfMx)		// if this request is to increase tu size, not backtrack change no longer in use
	{
		/* Don't upSize on basis of estimated or old tSup: if ah has not computed tSup this subHour:
		   can (or could previously, b4 az_unsizeIf stuff completed) add size that is not used,
			  or unnecessarily increase WZ/CZ/ZN/2 flow in AH::setTsSp1(), complicating autoSize convergence. 7-20-95.
			  TRY REMOVING WHEN ALL KNOWN TESTS WORK. */
		if (!ah->ahRun)			// cleared at beg subhour, set in AH::iter4Fs after aTs first computed
		{
			Top.ahKf++;
			ah->ahPtf++;	// be sure this ah is recomputed -- probably usually redundant
			return TRUE;			/* tell caller (ztuMode) to proceed with this flow anyway,
					   computing tz & staying in ArStH/C mode so AH::antRatTs will do right thing.
					   antRatTs will then resize and/or ztuMode will be recalled with ahRun TRUE. */
		}
#if 0 // old code was like this; unrun here
*     /* Don't upSize b4 turning on coil in ZN2 air handler: Can (or could, b4 b4 az_unsizeIf stuff completed)
*        add size then not use it, complicating autoSize convergence or spuriously setting dtLoH/CSh flag. 7-20-95.
*        Note ztuMode normally turns on coil if needed b4 calling tu->resizeif; this is insurance.
*        Original impl helped but didn't fully fix problem; not sure still needed. */
*     if ( ah->ahMode==ahFAN							// if in fan-only mode. believe ahFAN implies ZN2
*      &&  CHN(ah->(isCooling ? ahcc : ahhc).sched)==C_OFFAVAILVC_AVAIL )	// if coil present and sched available
*		{
*        return TRUE;		/* tell caller ztuMode to proceed with this flow anyway,
*				   computing tz & staying in ArStH/C mode so AH::antRatTs will do right thing.*/
*        // note antRatTs may test ahFAN itself to NOT use cv > cMx if coil not on.
*		}
#else // looks best 7-22-95. Revert if necessary. If kept returning FALSE, can remove ahFAN test from antRatTs.
		/* Don't upsize b4 turning on coil in ZN2 air handler,
		   to be sure don't get huge flows if there are subhours where load could be met by leak/loss/ventilation.
		   Note ztuMode normally turns coil on if needed b4 getting here, but antRatTs does not. */
		if ( ah->ahMode==ahFAN							// if in fan-only mode. believe ahFAN implies ZN2
				&&  CHN(isCooling ? ah->ahcc.sched : ah->ahhc.sched)==C_OFFAVAILVC_AVAIL )	// if coil present and sched available
			return FALSE;								// tell caller to limit flow to cMx at this time

#endif
		/* Prevent runaway flow (and floating point crash) when coil is underpowered:
		   don't increase flow beyond that which would be used if supply temp was 1 degree from zone setpoint.
		   Caller supplies value of "tSup * b - a", as derived in comments above, or 0 to disable. */
		if (isCooling)
			cvLim = -cvLim;				// for cooling expression must be negated
		if (cvLim > 0)					// if non-0 and correct sign, use cvLim
		{
			setToMax( cvLim, cMx);				// don't limit below max already established
			if (_cv > cvLim)   				// if vav flow to meet setpoint exceeds limit
			{
				_cv = cvLim;				// use limit. UPDATES CALLER's CV via reference arg.
				isCooling ? dtLoCSh : dtLoHSh = TRUE; 	/* set flag. -->dtLoH/C-->dtLoH/CAs-->endAutoSize() msg.
							   cleared in TU::begSubhr; remains set if tSup > sp. */
#if 1 // wasn't this intended? found missing 8-28-95.
				vf = _cv/ah->airxTs;			// recompute cfm for reduced heat cap flow
				if (!asKVol)  				// unless autosizing const vol (max===min)
					vf *= Top.auszHiTol2;			// add a margin
#endif
			}
		}
	}

// do it

	if ( !decrOnly && as.az_resizeIf( vf, FALSE)	// cond'ly increase tuVfMxH or C to vf. cnausz.cpp.
			||  as.az_unsizeIf( max( vf, bVfMx)) )	// else cond'ly decrease but not below start subhr value.
	{
		// if changed by either function
		cMx = vfMx * ah->airxTs;				// recompute heat cap flow limit for cfm @ current supply temp
		if (asKVol) 					// if autoSizing for constant volume
		{
			tuVfMn = vfMx;
			cMn = cMx;  			//  keep min same as max
		}
		if (asHcSame) 					// if autoSizing constant volume
		{
			isCooling ? tuVfMxH : tuVfMxC = vfMx;  	//  keep other of heat, cool the same
			isCooling ? cMxH : cMxC = cMx; 		//  (assume only 1 used at a time)
		}
		if (tuVfDs)					// if tu design flow is non-0 (else dynamic)
			setToMax( ah->fanFMax, vfMx/tuVfDs);		// update max ratio of max flow to des flow 7-18-95.
		// fanFMax doesn't get decreased on reductions; believe works ok (just slower) if oversize.
		return TRUE;					// change to (possibly updated) cv accepted & stored.
	}
	return FALSE;				// not a change or can't do it now
}			// TU::resizeIf
//=============================================================================================================================
// Zone and terminal hvac modelling functions
//=============================================================================================================================

RC TU::tu_BegHour()		// terminal stuff done at start hour, eg change tests or flag propogation.  call after exprs.
// also called from HEATPLANT if capAv changes, to recompute lhMn/Mx, conditionally flag tu's zone.
// callers: cnguts.cpp:doBegIvl, hp.cpp:hpCompute.
{
	ZNR *zp = ZrB.p + ownTi;

// set local heat limits per houly input expressions and coil capacity, possibly reduced by overload.
	lhMn = tuQMnLh;			// units are same.
	lhMx = max( tuQMxLh, tuQMnLh);	// if max is < min, min overrules.
	if (Top.tp_autoSizing && hcAs.az_active)	// if autoSizing this terminal coil 6-95
		;				// don't limit lhMx per captRat while determining captRat. See ztuMode().
	else
		if (lhMx > 0.)			// else moot, skip for speed
		{	DBL capAv = tuhc.captRat;			// current coil capacity starts as coil rated capacity
			switch (tuhc.coilTy)				// adjust coil capac to curr conditions per coil type
			{
			case C_COILTYCH_HW:   				// HW (Hot Water) coil
				HEATPLANT *hp = HpB.p + tuhc.hpi;		// access its HEATPLANT
				capAv *= hp->capF;    			// capAv is 1.0 if not overloaded, or fraction of capac's avail
				break;					//  .. for this HEATPLANT's loads
				/* if plant off, set lhMn,Mx as though on, so code can detect request for local heat when plant off.
				   error message is then issued in tuEndSubhr. */
			}
			setToMin( lhMn, capAv);			// current coil capacity is absolute upper limit for both min and max
			setToMin( lhMx, capAv);			// new 9-25-92: formerly I think tuhcCaptRat was ignored.
		}
	setToMax( lhMxMx, lhMx);		// record largest lhMx since 0'd at start autoSize, re tuQMxLh < tuhc.captRat warning.
	// note if capAv changes during hour, lhMn/Mx must be recomputed. Tentatively, HEATPLANT redoes this whole function, 9-92.

// air cap flow limits: are set by air handler (AH::ahVshNLims) per air temp and fan overload.

	/* changes in air heat hourly min, max's ---> zp->ztuCf:
	   1) cMn/Mx level (changes with inputs, fanF, air density) monitored in AH::ahVshNLims (incl from AH::estimate).
	   2) hourly inputs are monitored by exman chaf's, setting SAME flags */

	/* changes in local heat hourly min, max ---> zp->ztuCf. note same flag:
	   1) input changes monitored by exman chafs
	   2) other (plant) changes: monitor here for now; be sure AFTER plant overload when implemented */

	if (lhMn != lhMnPr)			// local heat min or set out level. no tolerance: user input: don't expect trivial changes
	{
		// lhMnPr reset in tuSOs/ztuMdSets.
		// if set temp and actual & desired (b4 overload) heat both > both new min & min when tu's last done, change is moot.
		if ( cmLh==cmSo						// if not SET OUTPUT, assume SET TEMP: would not change if unused.
				//||  min(tuhc.q, tuhc.qWant) <= max( lhMn, (DBL)lhMnPr) )		q always <= qWant, so just use qWant for min
				||  tuhc.qWant <= max( lhMn, (DBL)lhMnPr) )
		{
			zp->ztuCf++;   				// say a tu change.
			Top.ztuKf++;   				// say look for tu changes (needed here if not start subhr, from cnhp.cpp.)
		}
	}
	if (lhMx != lhMxPr)   			// local heat max.  no tolerance on user input: don't expect trivial changes
	{
		// must be SET TEMP: 0 for disallowed input would not change
		// if actual and desired (b4 overload) heat both < both new max and max when tu's last done, change makes no difference
		//if (max(tuhc.q, tuhc.qWant) ..			qWant always >= q, so just use qWant for max
		if (tuhc.qWant >= min( lhMx, (DBL)lhMxPr))
		{
			zp->ztuCf++;					// say a tu change.
			Top.ztuKf++;					// say look for tu changes (needed here if not start subhr, from cnhp.cpp.)
		}
	}   						// lhMxPr reset in ztuMdSets.

// terminal min flow==0 if control terminal and fcc: enforced in AH:begHour, after setting fcc.

	return RCOK;
}			// TU::tu_BegHour
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_BegSubhr() 		// terminal stuff done at start subhr
{

// conditionally clear "insufficient supply temp to autoSize tu flow" flags

	// These flags are set (in ztuMode, antRatTs) when tSup approaches close to zone temp & autoSized flow run away.
	// They are NOT set if tSup is already on "wrong" side of sp, cuz eg it might be scheduled thus.
	// Hence if setpoint stays on wrong side (due to leak or fan heat @ runaway flow), flag if set must remain set
	//   even from design day iteration to iteration, to be set in converged day, to invoke msg in endAutoSize. 7-16-95.

	AH *ah = AhB.p + ai;
	if (cmAr & cmStH)					// if terminal does setpoint air heat
		if (ah->ah_tSup > ZhxB.p[xiArH].sp)	// if supply temp will now heat, flags will be reset if doesn't heat enough,
			dtLoHSh = 					// so clear subHour flag set in ztuMode,
				aDtLoHSh = FALSE;			// and clear subHour flag set in AH::antRatTs.
	if (cmAr & cmStC)				// if terminal does setpoint air cool
		if (ah->ah_tSup < ZhxB.p[xiArC].sp)		// if supply temp will now cool, flags will be reset if doesn't cool enough,
			dtLoCSh = 				// so clear subHour flag set in ztuMode,
				aDtLoCSh = FALSE;			// and clear subHour flag set in AH::antRatTs.

	return RCOK;
}		// TU::tu_BegSubhr
//-----------------------------------------------------------------------------------------------------------------------------
//RC TU::AfterSubhr() { return RCOK; }		// terminal stuff done between subhours: after reports/exports
//-----------------------------------------------------------------------------------------------------------------------------
// note not in order called (becuase it is short):
//-----------------------------------------------------------------------------------------------------------------------------
RC ZNR::ztuEndSubhr()	// end-subhour (after iterations) hvac checks/computations for given zone and its terminals

// computations not involved in finding heat balance equilibrium (thus deferred to speed any iteration).
// store results in run results records.

// returns non-RCOK for error that should terminate run (message issued).
{

//----- zone's simulation results for subhour.   Values + for heat TO zone.
//      Note q to CAir is subtracted from qMass for balance.
	RC rc = RCOK;

	ZNRES_IVL_SUB* zrs = &ZnresB.p[ss].curr.S;	// zone's subhour results in ZNRES
												//  0d in ZNR::zn_BegSubhr1
	zrs->tAir = tz;				// subhour result air temp
	zrs->tRad = zn_tr;			// subhour result radiant temp
	zrs->wAir = wz;				// subhour result air humidity ratio
#ifdef COMFORT_MODEL
	zrs->PMV7730 = zn_comfPMV7730;		// subhour PMV (comfort)
	zrs->PPD7730 = zn_comfPPD7730;		// subhour PPD (comfort)
#endif
	zrs->pz0 = zn_pz0;			// substep zone pressure

// subhour results: hvac output
	double qsHvac = zn_qsHvac * Top.tp_subhrDur;		// subhour sensible hvac energy, Btu
	zrs->qsMech = qsHvac + zn_qHPWH * Top.tp_subhrDur;	// subhour sensible mechanical energy, Btu
														//   (includes HPWH heat extraction)

	zrs->qlMech = zn_qlHvac * Top.tp_subhrDur;	// subhour latent hvac energy (power * time = energy)

	double qTotMech		// combined sensible and latent energy (netting out if signs different)
		= qsHvac + zn_qlHvac * Top.tp_subhrDur;

	if (zn_hcMode == RSYS::rsmOAV)
		zrs->qvMech = qTotMech;
	else
	{	(qTotMech < 0. ? zrs->qcMech : zrs->qhMech) = qTotMech;
		(qsHvac < 0.f ? zrs->qscHvac : zrs->qshHvac) = qsHvac;
		if (i.zn_loadMtri)
		{
			LOADMTR_IVL& lm = LdMtrR[i.zn_loadMtri].S;
			lm.qHtg += zrs->qshHvac;
			lm.qClg += zrs->qscHvac;
		}
	}

	zrs->eqfVentHr = zn_fVent * Top.tp_subhrDur;	// equivalent full vent operation, hr

	if (!zn_IsConvRad())
	{	
#if defined( OLDHUM)
		never successfully run
		see all-CNE case in loadsSubhr()
		rc |= zn_AirX();		// called earlier for CR
#endif

		// subhour results:
		//   heat transferred out of masses, incl heat transferred out of "cair" (accounted as a mass heat),
		//				     incl heat "to" sun, for balance, cuz zrs->qSlr includes qMsSg.
		//				     incl heat "to" rad int gain, for bal, cuz zrs->qsIg incl qrIgMs 11-95.
		//                                   incl heat of condensation for last subhour's excess moisture (heat from masses or air) 5-97
		// traditional (CSE) a/b zone model
		// t = (a + q)/b  -->  q = b * t - a.  qzones = b * t - a - qsolar.  - for q TO zones.  *time for energy.  add in cair energ.

		zrs->qMass = 			// subhour energy transferred out of masses to zone
			( aMassHr + aMassSh    	// subhr energy from MASSes: power is "a" for zn's hrly & subhrly masses
				  - qMsSg 			//       - zn's masses solar gain (Btuh) this subhr: energy to mass.
				  - qrIgMs			//       - zn's masses radiant intn'l gain (Btuh) this hour: energy to mass. 11-95.
				  - haMass * tz     //       - coupling to ms's ("b") * temp
				  + znXLGainLs )	//       + last subhr excess latent gain: heat of condensation to zone
				* Top.tp_subhrDur      		//    all times time interval to get energy
				+ i.znCAir * (tzls - tz);	// subhr energy from "cair":  hc times temp diff
		// (CAIR models all non-mass hc in zone: air, furniture, light walls, etc)
	}
	else
	{	// convective/radiant model
		zrs->qMass = 		// subhour energy transferred out of masses
			(   zn_hcATMsSh - zn_hcAMsSh * tz		// convective coupling to mass
			  + zn_hrATMsSh - zn_hrAMsSh * zn_tr	// radiant coupling to mass
			  - qMsSg 				// - zn's masses solar gain (Btuh) this subhr: energy to mass.
			  - qrIgMs				// - zn's masses radiant intn'l gain (Btuh) this hour: energy to mass
			  + znXLGainLs		// + last subhr excess latent gain: heat of condensation to zone
			) * Top.tp_subhrDur      	//    all times time interval to get energy
			+ i.znCAir * (tzls - tz);	// subhr energy from "cair":  hc times temp diff
	}

	// subhour results: latent heat transfered out of air in zone (analogous to qMass)
	zrs->qlAir = (wzls - wz) * zn_dryAirMassEff * PsyHCondWtr
				 - znXLGain * Top.tp_subhrDur;				// add loss assumed to prevent supersaturation (but it has other sign)
														// == heat of condensation of moisture assumed removed (see znW)
	zrs->qlIz = zn_qlIz * Top.tp_subhrDur;

	zrs->ivAirX = zn_ivAirX;	// znres: airX is airnet infil/vent only

// subhour results: internal gain
	zrs->qsIg =   ( qsIgHr			// sens intl gain, set in cnloads:loadsHourBeg from znSGain + wthr file.
					+ qrIgTot )		// total radiant int gain originating in zone.
									//   non-CAir parts subtracted out of qMass above and qsIz & qCond below. 11-95.
				  * Top.tp_subhrDur; 		// convert power to energy
	// Next two for daylighting simulation for NREL E10 program using GAINs with gnDlFrPow.
	// Set in cnguts.cpp:doHourGains; accumulated in cnguts to H,D,M,Y; outputtable only via probes or binary results files (9-94).
	// "* Top.tp_subhrDur" converts powers to energy.
	zrs->litDmd = znLitDmd * Top.tp_subhrDur;		// zone lighting demand (energy that would be used without daylighting)
	zrs->litEu =  znLitEu * Top.tp_subhrDur;		// zone lighting actual energy use, after any daylighting reduction.
	zrs->qlIg = znLGain * Top.tp_subhrDur;			// latent intl gain (Btuh), set from GAINs by cnguts.cpp:doHourGains

// subhour results: solar gain
	zrs->qSlr = (qSgTot + qSgTotSh) * Top.tp_subhrDur;
								//	slr gain. qSgTot & qStTotSh are targets set per SgR entries in cnloads.
								//     mass part qMsSg subtracted out of qMass above

// subhour results: infiltration and conduction
	zrs->qsInfil = zn_uaInfil * (Top.tDbOSh - tz) * Top.tp_subhrDur;	// infiltration sensible gain
	if (zn_IsConvRad())
	{	// CSE zone
		zrs->qlInfil = zn_NonAnIVAmf()		// dry-air mass flow
			         * (Top.wOSh - wzls)	// * water added (forward difference, delta from last step)
					 * PsyHCondWtr			// * 1061 Btu/lb
					 * Top.tp_subhrDur;		// * portion of hour --> energy (Btu)
											//   (convert power to energy)

		zrs->qCond = (zn_qCondQS + zn_qDuctCond + zn_qDHWLoss) * Top.tp_subhrDur;
		// quick surface conduction + duct gain + DHW losses to zone
	}
	else
	{	// CNE zone
		DBL wDiff = 0;
		switch (Top.humMeth)			// compute infil air w diff from displaced air consistently with w calc method
		{
		case C_HUMTHCH_PHIL:
			wDiff = //(Top.wOSh - wz);			does not balance when wz determined using (ws - wsls)/2.
				(Top.wOSh - (wz + wzls)/2.);
			break;	// use average wz
		case C_HUMTHCH_ROB:
			wDiff = (Top.wOSh - wz);
			break;	// use latest wz
		}
		zrs->qlInfil = 				// latent infiltration gain energy
			zn_uaInfil/Top.tp_airSH	// heat cap infil air flow / spec ht = mass flow (lb/hr)
			* wDiff	 				// * water added: entering w - average displaced w (ratios)
			* PsyHCondWtr 			// * 1061 Btu/lb --> power (Btuh)
			* Top.tp_subhrDur;			// * portion of hour --> energy (Btu) (convert power to energy)
		// 6-10-92 is above right 1) considering w not computed in a/b form (see znW); 2) when exfil != infil.

		zrs->qCond = (zn_ua * (Top.tDbOSh - tz)	// conduction thru walls to zone per temp diff
				- qrIgTotO)				// less rad int gain lost to outdoors thru light surfaces 11-95
			* Top.tp_subhrDur;				// convert power to energy

		// conduction thru specified-temp-exposure surfaces 3-91
		XSRAT* xs /*=NULL*/;
		for (TI xi = xsSpecT1; xi != 0; xi = xs->nxXsSpecT)	// these xsurfs have own chain
		{
			xs = XsB.p + xi;						// access XSRAT record xi
			zrs->qCond += xs->x.xs_uval * xs->x.xs_area	// ua
				* (xs->x.sfExT - tz) 				// * tdif
				* Top.tp_subhrDur;					// * hr frac
		}
	}


// subhour results: interzone

	zrs->qsIz = ( zn_qIzSh 			// heat flows to zone per IZXFER inputs (set in cnloads:loadsIzxSubhr)
				 - qrIgTotIz )		// less net radiant internal gain lost thru light surfaces to other zones' CAirs 11-95
			   * Top.tp_subhrDur;		// convert power to energy

// subhour results: record excess latent gain, turned into sensible gain (simulating condensation) to prevent supersaturation

	zrs->qlX = znXLGain * Top.tp_subhrDur;			// convert power to energy. Used for probes, msgs, poss future reporting.
	zrs->nSubhrLX = (zrs->qlX != 0.f);			// 1 if non-0, 0 if 0: accumulates to # subhrs with condensation.

	if (!Top.isWarmup && !Top.tp_autoSizing)
		/* bool bUnMet = */ zn_TrackUnmetLoads();

	return rc;
}			// ZNR::ztuEndSubhr
//-----------------------------------------------------------------------------
bool ZNR::zn_TrackUnmetLoads() const	// unmet load statistics
{

// subhour results: unmet load
// Values tracked
//  - unMetHrs = count of hours when tz error > 0
//  - unMetHrsTol = count of hours when tz error > tp_unMetTzTol
//  - unMetShDH = deg-hr excursion accumulated each subhour
//  - unMetHrDH = deg-hr excursion evaluated at end of hour
//  - unMetMaxTD = largest excursion, F

	if (zn_hcMode != RSYS::rsmHEAT
	 && (zn_hcMode != RSYS::rsmCOOL))
		return false;

	static_assert(RSYS::rsmHEAT == 1);
	static_assert(RSYS::rsmCOOL == 2);

	int iHC = zn_hcMode - 1;

#if 0 && defined(_DEBUG)
	if (zn_HasTerminal())
	{
		if ((iHC == 0 && zn_tzsp != 68.f) || (iHC == 1 && zn_tzsp != 78.f))
			printf("\nMiss");
	}
#endif
	
	float tzErr = tz - zn_tzsp;		// temp excursion above/below setpoint

	bool bUnMet = (iHC == 0 && tzErr < -0.001f)
		       || (iHC == 1 && tzErr > 0.001f);
	if (bUnMet)
	{	// zone temperature outside setpoint range
		ZNRES_IVL_SUB& zrs = ZnresB.p[ss].curr.S;
		zrs.unMetHrs[iHC] = Top.tp_subhrDur;
		if (abs(tzErr) > Top.tp_unMetTzTol)
			zrs.unMetHrsTol[iHC] = Top.tp_subhrDur;
		zrs.unMetShDH[iHC] = tzErr * Top.tp_subhrDur;
		zrs.unMetMaxTD[iHC] = tzErr;	// min/max tracking done in accumZr()
		if (Top.isEndHour)
			zrs.unMetHrDH[iHC] = tzErr /* * 1.f */;
	}

	return bUnMet;
}		// ZNR::zn_TrackUnmetLoads
//-----------------------------------------------------------------------------------------------------------------------------
// also note not in order called, because short:
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tu_EndSubhr() 	// terminal stuff done at end subhr: record load; checks and results stuff not needed during iteration
{
// record converged load values, increase capacities, compute part load ratio and plr peaks
	if (Top.tp_autoSizing || !Top.isWarmup)	// don't let main sim warmup exagerate main sim results
	{	// local heat. tuhc.q is set in ztuMode, ztuMdSets.  For terminal, load === capacity.
		if (cmLh)
		{
			hcAs.az_resizeIf( tuhc.q, TRUE);		// conditionally increase tuhc.captRat, plus margin
			setToMax( hcAs.ldPk, tuhc.q);			// record local heat coil peak load
			if (tuhc.captRat)					// might be 0 at start autoSizing. don't /0.
				if (tuhc.q/tuhc.captRat > hcAs.plrPk)		// if a new peak part load ratio (plr)
				{
					hcAs.plrPk = tuhc.q/tuhc.captRat;		// record peak plr (fraction capacity used)
					hcAs.xPk = tuhc.captRat;			// record capacity at time of peak plr
				}
		}
		ZNR *zp = ZrB.p + ownTi;			// terminal's zone
		AH *ah = AhB.p + ai;			// terminal's air handler if any

		// air heat/cool
		/* report terminal at min flow as heating or cooling cuz eg a zone-controlled const-vol tu can be
		   in the float mode just above or below the setpoint mode when at setpoint (moot b4 TUSIZE/LOAD rpts added 7-95)
		   (but ignore set output (uSo) here -- a different capability) */
		if ( useAr & (uStH|uMxH)						// if terminal is air-heating at setpoint or max flow
				||  useAr & uMn  &&  cmAr==cmStH				// or is at min flow and only has heating capability
				||  useAr & uMn  &&  cmAr==cmStBOTH  &&  ah->ah_tSup >= zp->tz )	// or is at min flow and supply temp >= zone temp
		{
			// air heat. cv and cz set in ztuMode and ztuMdSets. resizeIf done. For terminal, capacity===max cfm load.
			if (ah->airxTs)				// believe might be 0 if ah scheduled off
			{
				float vf = cv / ah->airxTs;  		// heat cfm load @ vav box: heat cap flow * conv factor @ supply temp.
				setToMax( vhAs.ldPk, vf);   		// cfm peak
				if (tuVfMxH)				// might be 0 at start autoSize. don't /0.
					if (vf/tuVfMxH > vhAs.plrPk)		// if a new peak part load ratio (plr) (fraction of rated flow used)
					{
						vhAs.plrPk = vf/tuVfMxH;		// remember peak plr
						vhAs.xPk = tuVfMxH;			// remember rated flow at which peak plr occurred, 6-97
					}
				setToMax( qhPk, (ah->ah_tSup - zp->tz) * cz);	// record new high Btuh peak to zone. @ zone ?. Negative if not heating.
				// merge in subhour final values of flags set if too little dT to autoSize tuVfMxH (eg weak coil)
				dtLoH |= 					// invokes msg in tu_endAutosize (via dtLoHAs) if set on converged day
					dtLoHSh				// merge flag set in ztuMode + ztuAbs
					| aDtLoHSh;			// merge flag set in ahCompute + antRatTs
			}
		}
		// no else: for tSup==tz for tu that can both heat & cool, tentatively count flow re both heat and cool 7-9
		if ( useAr & (uStC|uMxC)						// if terminal is air-cooling at setpoint or max flow
				||  useAr & uMn  &&  cmAr==cmStC				// or is at min flow and only has cooling capability
				||  useAr & uMn  &&  cmAr==cmStBOTH  &&  ah->ah_tSup <= zp->tz )	// or is at min flow and suply temp <= zone temp
		{
			// air cool. cv and cz set in ztuMode and ztuMdSets. resizeIf done. For terminal, capacity===max cfm load.
			if (ah->airxTs)				// believe might be 0 if ah scheduled off
			{
				float vf = cv / ah->airxTs;  		// cool cfm load @ vav box: heat cap flow * conv factor @ supply temp.
				setToMax( vcAs.ldPk, vf);   			// record new cfm peak
				if (tuVfMxC)				// don't /0
					if (vf/tuVfMxC > vcAs.plrPk)		// if a new peak plr @ vav box
					{
						vcAs.plrPk = vf/tuVfMxC;		// record peak plr
						vcAs.xPk = tuVfMxC;			// record rated flow at which peak plr occurred, rob 6-97
					}
				setToMin( qcPk, (ah->ah_tSup - zp->tz) * cz);	// record new peak cool Btuh load @ zone. @ zone?  normally Negative.
				// merge in subhour final values of flags set if too little dT to autoSize tuVfMxC (eg weak coil)
				dtLoC |= 			// invokes msg in tu_endAutosize (via dtLoHAs) if set on converged day
					dtLoCSh				// merge flag set in ztuMode + ztuAbs
					| aDtLoCSh;			// merge flag set in ahCompute + antRatTs
			}
		}
	}

// check for local heat on when required plant is off
	if (qLhWant > tuhc.q)			// if lh desired but not gotten
		switch (tuhc.coilTy)			// by coil type
		{
		case C_COILTYCH_HW:
			HEATPLANT *hp = HpB.p + tuhc.hpi;
			if (hp->hpMode != C_OFFONCH_ON)		// heatplant must be scheduled OFF, or would have come on from ztuCompute.
				// rWarn? shd be ok to continue: 0 lh used when plant off.
				rer( MH_R1252,				// "heatPlant %s is scheduled OFF, \n"
					 hp->Name(), Name(), tuQMnLh, tuQMxLh );	// "    but terminal %s's local heat is NOT scheduled off: \n"
			break;						// "        tuQMnLh = %g   tuQMxLh = %g"
		}

// check for local heat requiring air flow scheduled on when no flow
	// necessary because don't believe checks elsewhere cover all cases yet (3-92)

	if (tuhc.q > 0.f)				// if lh output is non-0
		if (tuLhNeedsFlow==C_NOYESCH_YES)   	// and terminal's lh needs flow
			if ( cz==0.f 				// if terminal has no air heat/cool flow (in excess of leaks/backflow)
					&&  CHN(tfanSch)==C_TFANSCHVC_OFF )	// and fan is off *COMPLETE THIS*  TFAN CODE INCOMPLETE .. HEATING,VAV ...
				return rer( MH_R1253,		// "Local heat that requires air flow is on without flow\n"
							Name(), Name());			// "    for terminal '%s' of zone '%s'.\n"
	// "    Probable internal error, \n"
	// "    but perhaps you can compensate by scheduling local heat off."

	// check desirable for local heat disabled due to no flow when there actually could be flow.. wd need flag[nMd] in TERMINAL!

// calculate and charge heat coil energy use to specified meter. tuhc.q set by ztuMode or ztuMdSets.
	switch (tuhc.coilTy)
	{
	case C_COILTYCH_HW:
		tuhc.p = tuhc.q;  	// HW coil's input from HEATPLANT is its output. used?
		break;     		// but not charged to meter here -- HEATPLANT input goes to meter

	case C_COILTYCH_ELEC:
		tuhc.p = tuhc.q;				// electric coil's input is its output
		if (tuhc.mtri)						// if meter specified for terminal coil
			MtrB.p[tuhc.mtri].H.htg += tuhc.p * Top.tp_subhrDur;	// post coil input energy to use "heating"
		// "*Top.tp_subhrDur" added 9-25-92.
		break;
	}

// charge terminal fan energy use to specified meter
	// ??? no code yet calculates and sets tfan.p, 4-11-92 **********
	if (tfan.fn_mtri)						// if meter specified for terminal fan
		MtrB.p[tfan.fn_mtri].H.fan += tfan.p * Top.tp_subhrDur;	// charge fan power consumption to that meter, end use "fans"
	return RCOK;
}				// TU::tu_EndSubhr
//-----------------------------------------------------------------------------------------------------------------------------
RC TU::tuEstimate() 			// terminal "estimate" function to call at start of subhour

// sets terminal wantMd (desired AH.ahMode) if air handler tSup cm (ahTsSp) is zone-controlled (ZN or ZN2), else nop
{
	if (ctrlsAi)				// if this terminal is "control terminal" for an air handler
	{	AH *ah = AhB.p + ctrlsAi;
		if (ah->isZNorZN2)   			// if air handler is under ZN or ZN2 setpoint control
		{
			// turning on 0-flow air handler wastes time (just turns off again) & might find other problems. precaution 10-96.
			if ( ah->sfan.vfDs > 0		// if fan flow non-0 (0 is illegal input but might come from autoSize)
					||  Top.tp_sizing && ah->fanAs.az_active )	// or if fan is being autoSized (thus initially 0)
			{	// estimate whether zone wants heat or cold, using prior tz and current setpoints

				/* Responds to sp changes.
				   Since old tz is used, does not respond to load changes til next subhr, so ztuCompute turns on heat or cool if nec.
				   When heat/cool no longer needed, turned off here next subhour, sooner in AH::setTsSp1 in some cases. */

				/* Should ah heat or cool if tuVfMx is 0?
				   Current code several places in cnztu.cpp assumes no. More comments at start of cnah1.cpp. 10-24-96. */

				DBL tz = ZrB.p[ownTi].tz;
				if ((sstat[TU_TUTH] & FsSET)        &&  tz < tuTH - ABOUT0)	// if zone temp clearly below heat setpoint
				{	if (max( tuVfMn, tuVfMxH) > 0 || Top.tp_sizing && vhAs.az_active)	// if tu has heating flow or autosizing it 10-96
						wantMd = ahHEATING;						// say control zone wants ah to heat
				}
				else if ((sstat[TU_TUTC] & FsSET)   &&  tz > tuTC + ABOUT0)	// if zone temp clearly above cool setpoint
				{	if (max( tuVfMn, tuVfMxC) > 0 || Top.tp_sizing && vcAs.az_active)	// if tu has cooling flow or autosizing it 10-96
						wantMd = ahCOOLING;						// say zone wants ah to cool
				}
				else if ( (!(sstat[TU_TUTH] & FsSET) || tz > tuTH + ABOUT0) 	// (ABOUT0: .001, cnglob.h)
						  &&  (!(sstat[TU_TUTC] & FsSET) || tz < tuTC - ABOUT0) )	// if control zone wants neither heat nor cool,
					wantMd =  CHN(ah->ahTsSp)==C_TSCMNC_ZN2  ?  ahFAN 		//  ZN2 leaves supply fan on
							  :  ahOFF;		//  ZN shuts off fan and thus air handler
				//else   this "else" removed when no-tu-flow if's added above, 10-96.
				if (CHN(ah->ahTsSp)==C_TSCMNC_ZN2)					// if ZN2 control (cool/fan/heat) this hour
					if (wantMd==ahOFF)						// change request for "off" (incl initial 0)
						wantMd = ahFAN;						// to "fan only": ZN2 never goes off.
				// this starts fan when no heat/cool req'd and tuTH/C same as initial zone temp (70), 4-95.
				// else: if on tuTH or tuTC, no change here.  Is changed later if nec from tu mode search in ztuMode().
				// TU::wantMd is used in AH::ah_SetMode().
			}
		}
	}
	return RCOK;
}			// TU::tuEstimate
//-----------------------------------------------------------------------------------------------------------------------------
RC ZNR::ztuCompute() 		// Compute ("refine") all terminals for zone: subhour computations that may be iterated

// expects air handlers to have been estimated first (AH::ahEstimate) to establish supply temp tSup;
// may call AH::ahEstimate again for various mode change and increase-tSup cases re CZ, WZ, ZN, and ZN2 ahTsSp ctrl. */

// input-output lists desirable here: merge those of callees.

// returns non-RCOK for error that should terminate run (message issued)
{
	RC rc=RCOK;

// control flags:   All these are cleared here.
//   ZNR.spCf:		A SETPOINT changed: Must rebuild zone mode sequence and do everything else.
//					set by exman chafs (input-change monitoring) [or TU::begHour]
//   ZNR.ztuCf:		load (aqLdSh, bLdSh, or w-related) changed,
//			or any of many things about terminals, other than setpoint, changed: tSup, limits, etc.
//			places set include: cnloads.cpp; chafs, tuSOs (this file), TU::begHour,AH::ahVshNLims,flagTus, .

#if defined( CRTERMAH)
	if (i.znModel == C_ZNMODELCH_CNE || zn_HasTerminal())
#else
	if (i.znModel == C_ZNMODELCH_CNE)
#endif
	{ 	// "traditional" CSE zone model (conditioned by terminals)
		if (spCf)  			// if a setpoint has changed
		{	CSE_E( ztuMdSeq() )	// Build zone mode sequence table for zone.  Check setpoint relations.
							// Init to a middle mode so will pass thru setpoint modes b4 max'ing out, for autoSize.
			ztuCf++;		// sp change can change all q's
		}

		if (ztuCf)
			CSE_E( ztuMode() )	// Determine zone mode md, temp tz, tstat-ctrl'd tu outputs, etc.
								//  resets some .ztuCf priors. may do ah->ahEstimate.
	}
	// else other zone models elsewhere

// Clear this fcn's changed-input flags
	spCf = ztuCf = FALSE;

	return rc;
}			// ZNR::ztuCompute
//----------------------------------------------------------------------------------------------------------------------------
RC ZNR::ztuMode()		// ztuCompute inner fcn: determine zone mode, zone temp, terminal outputs, etc.

// expects air handlers to have been estimated first (AH::ahEstimate) to establish supply temp tSup;
// may call AH::ahEstimate again for various mode change and increase-tSup cases re CZ, WZ, ZN, and ZN2 ahTsSp ctrl.

// inputs:  ZNR.aqLdSh, bLdSh, md, nMd, mdSeq[*],
//          TU.mnLh,mxLh,cMn,cMxH/C,
//          AH.ah_tSup, ... .
// outputs: ZNR.md, tz, aTz, qM, cM, aqO, bO, cO, c,
//          TU.cv, aCv, .cz, .QLh,  .aqO, .bO,

// must also set (when tfan implemented): tfanRunning -- already tested by AH::tfanBackC(), 5-92.
{
	RC rc=RCOK;
	int md = zn_md;   			// working copy of zone mode (mdSeq[] subscript): start with prior or as init elsewhere

	/* AutoSizing note 7-95: autoSizing currently implemented only in setpoint modes
	   -- if desired q or flow > max, max is increased.
	   Pegged float modes do not autoSize (yet 7-95), but ztuMdSeq() initializes md to
	   between heat & cool modes so setpoint mode should be seen first. */

// Adjust search start zone mode to not initially max-out (from ztuAbs) a tu-controlled ah in mode it isn't in.

	/* Intended to save time of changing ah to match zone (from ztuAbs) then back (2 unnec ah reEstimates), only
	   just after tuEstimate + ahEstimate have changed ah mode eg at sp change, for expected simple cases, or if mdSeq changed.
	   Added 4-95, when ztuAbs improved to activate/change ah mode of ZN/ZN2 ah's pegged in zone mode. */

	// Don't change re active mode, cuz sometimes ah must heat to cancel loss when zone is cooling -- cases below handle.

	int xi = 0;					// receives subscript of zhx to change to, if any; corress zone mode found below.
	int iMd;
	for (iMd = 1;  iMd < nMd;  iMd++)		// search +iMd and -iMd from current mode to make nearest change if more than one
	{	ZHX *x;
		AH* ah;
		TU* tu;

		// if hier mode heats but its zn-ctrl'd ah is set to cool, start zone mode search at mode where that ah cools.
		int j = md + iMd;  					// iMd'th mode up from last mode used
		if ( j < nMd  					// if mode in range
				&&  mdSeq[j] != MDS_FLOAT )			// if not a float mode (float modes (-1) have no zhx's)
		{	x = ZhxB.p + mdSeq[j];
			tu = TuB.p + x->ui;
			ah = AhB.p + x->ai;	// point mode's zhx, zhx's terminal, zhx's ah

			if ( tu->ctrlsAi && tu->ctrlsAi==x->ai	// if zhx's tu controls zhx's air handler
					&&  x->zhxTy==ArStH  &&  x->xiArC		// if an air heat zhx for terminal that can also cool
					&&  ah->isZNorZN2				// if ah supply temp setpoint is under ZN or ZN2 ctrl this hour
					&&  ah->ahMode & ahCOOLBIT 			// if ah is now set to cool and
					&& !(ah->ahMode & ahHEATBIT) )		//   not heat (ie exclude ahON: redundant check for ZN/ZN2)
			{
				xi = x->xiArC;		// start zone mode search using zhx for setpoint cooling from this ah -- mode found below
				break;			// do only nearest change; let unexpected (but not disallowed) complex cases be slow
			}
		}

		// if lower mode cools but its zn-ctrl'd ah is set to heat, start zone mode search at mode where that ah heats.
		j = md - iMd;  					// iMd'th mode down from last mode used
		if ( j >= 0  					// if mode in range
				&&  mdSeq[j] != MDS_FLOAT )			// if not a float mode (float modes (-1) have no zhx's)
		{	x = ZhxB.p + mdSeq[j];
			tu = TuB.p + x->ui;
			ah = AhB.p + x->ai;	// point mode's zhx, zhx's terminal, zhx's ah

			if ( tu->ctrlsAi && tu->ctrlsAi==x->ai	// if zhx's tu controls zhx's air handler
					&&  x->zhxTy==ArStC  &&  x->xiArH		// if an air cool zhx for terminal that can also heat
					&&  ah->isZNorZN2				// if ah supply temp setpoint is under ZN or ZN2 ctrl this hour
					&&  ah->ahMode & ahHEATBIT 			// if ah is now set to heat and
					&& !(ah->ahMode & ahCOOLBIT) )		//   not cool (ie exclude ahON: redundant check for ZN/ZN2)
			{
				xi = x->xiArH;		// start zone mode search using zhx for setpoint heat from this ah -- mode found below
				break;			// do only nearest change; let unexpected (but not disallowed) complex cases be slow
			}
		}
	}
	if (xi > 0)				// if found a ZHX whose mode to use
	{	for (iMd = 0; iMd < nMd; iMd++)  	// find mode that uses this zhx
			if (mdSeq[iMd]==xi)
				md = iMd;			// update zone mode at which following search starts
	}

	DBL a, bZn;   		// a and b for all but active terminal; become ZNR.aqO/bqO, and TU.aqO/bO for that tu.
	// variables init'd to 0 while pursuing obscure floating point bug on Scott's machine only, 1-31-94.
	DBL t=0., q=0.;     // temps for zone temp, and for heat needed then heat delivered in active mode.
	// "cv", local below, is 0 or vav heat cap flow rate of active (air) mode.
	DBL cz=0.;			// 0 or zone flow of active terminal: cv less leaked air
	DBL tSup=0.;		// 0 or air handler supply temp
	float sp = 0.f;		// 0 or active setpoint
	float spWas = 0.f;	// 0 or prior mode active setpoing
	DBL lhq;			// 0 or Q of active local heat mode
	DBL tFuzz = 1.e-6f;   	// temperature tolerance for staying in setpoint modes.  1.e-5?
	DBL tFuzz2 = 1.e-4f; 		/* larger temperature tol for tSup re-estimating (trying to get closer wastes time)
					   and for ah mode changes that can alternate forever in certain cases where
					   big leak or loss causes tSup to get rapidly more favorable as flow falls. 4-95.
					   TESTED: using 1e-5 ADDED two iterations to four of the cks11.bat runs, 4-95. */
	for (SI j = Top.iter; j > 0; j--)	// double tFuzz2 tolerance each top-level ah-tu iteration
		tFuzz2 *= 2;			// ... so repeating ah mode changes initiated here will eventually stop.
	/* TESTED increasing tolerance let program continue (with zone temp a fraction of
	   a degree off) instead of issue convergence error message for T136c, 4-95.
	   Subsequent cnah changes also fix T136c but this seems like good insurance.
	   Also removes 2 iterations from two of the cks11.bat runs, 4-95. */

	int priorMdd = 0, iterCount = 0, huntCount = 0;	// re convergence detection: see end of loop

// find applicable zone mode and compute tz, cM, and qM for that mode

	for ( ; ; )					// until zone mode md not changed in loop
	{
		int mdWas = md;				// save zone mode to test if changed
		// >> wsSum unused 6-12-92.  later delete member and rename wsSum1 back.
		ztuAbs( md, 1, zn_aqHvO, zn_bHvO, wcSum, wcSum1, cSum);	// set zone members aqHvO, bHvO, wcSum1, wcSum, cSum.
		// active-tu contribution is added to wcSum, wcSum1 and cSum below.
		// may start ZN/ZN2 ah's as implied by mode.
		lhq = 0.;		// no local heat q unless an lh zone mode
		spWas = sp;		// prior active setpoint if any
		sp = 0.f;		// no active setpoint

		// a  =  sigma(UAi*Ti) + other q's + Told*hc/t
		a = zn_aqLdSh				// non-hvac portion for subhour, set in cnloads.cpp:loadsSubhr()
			+ zn_aqHvO;				// Other-terminal HVAC part: q of lh's, c*t for air.

		// b  =  sigma(UAi) + hc/t
		bZn = zn_bLdSh   			// non-hvac UA's portion for subhour, set in cnloads.cpp:loadsSubhr()
			+ zn_bHvO;				// + other-tu hvac UA: set output and pegged TU cap rate (Btuh/F)

		// following code must either: change md (causing another iteration),
		// or set cz, q, t (if not setpoint), tu->cv, tu->cz & tu->aCv, or lhq & tu->tuhc.q & .qWant where pertinent,
		//    and update wcSum, wcSum1 & cSum by adding any tu contribution.
		if (mdSeq[md]==MDS_FLOAT)      		// if a floating mode (MDS_FLOAT is < 0; otherwise mdSeq is zhx subscript)
		{
			// floating modes
			if (bZn==0.)
				return rer(MH_R1254, Name());	// "Internal error: 'b' is 0, cannot determine float temp for zone '%s'"
			t = a/bZn; 					// zone temp: with no tstat-ctrl'd zhx, there is no q not already in a
			// change modes if temp out of range.
			// tFuzz prevents getting stuck in inf loop cuz of accumulated roundoff errors
			if (md > 0  &&  t < ZhxB.p[mdSeq[md-1]].sp - tFuzz)   	// if cooler than setpoint of next lower mode, go to lower mode
				md--;
			else if (md < nMd-1  &&  t > ZhxB.p[mdSeq[md+1]].sp + tFuzz)	// if warmer than sp of next hier mode, go to hier mode
				md++;
			else   	  			// no terminal heat used in float mode (no 'active' terminal)
			{
				cz = 0.;
				q = 0.;
				tSup = 0.;	// all tu->aqO, ->bO, ->c are set by ztuMdSets for float modes.
							// wcSum, wcSum1, cSum: ztuAbs stored appropriate values.
				sp = 0.f;	// insurance: no active setpoint
			}
		}
		else
		{
			// setpoint modes

			int timesReTemped = 0;			// init # times ah re-estimated just to change supply temp, not mode, this call
reDo:				// come here to redo after reEstimating ah from ArStH and ArStC cases
			ZHX *x = ZhxB.p + mdSeq[md];		// active zhs subscript for non-floating mode
			TU *tu = TuB.p + x->ui;    		// point active terminal
			AH *ah = AhB.p + x->ai;    		// point air handler if any
			// test for ah supply temp under control of this zone
			BOO isZNorZN2 = tu->ctrlsAi  &&  tu->ctrlsAi==x->ai	// if zhx's terminal controls zhx's air handler
							&&  ah->isZNorZN2;   			// and ah tSup sp cm is ZN or ZN2 this hour (set in AH::begHour)
			// pre-test for ZN/ZN2 control terminal for ArStH/C cases (does not include WZ/CZ) 4-95
			BOO isCtu =  isZNorZN2				// if ZN or ZN2 supply temp control from this zone
						 &&  CHN(ah->ahSch) != C_AHSCHVC_OFF;	// ah sched ON (or future WARMUP) (reEstimate nop if off)
			tSup = ah->ah_tSup;  			// get air handler's supply temp (0. from ah[0] if tu has no ah)
			sp = x->sp;  				// setpoint, will be zone temp if load exactly met
			q = sp * bZn - a;    		// heat power req'd from this zhx to hold sp (may be replaced w ht delivered)
			DBL dT = tSup - sp;			// air heat/cool supply temp delta from zone
			DBL ft = a/bZn; 			// zone float temp: temp if zero output from this tstat-controlled zhx.
			DBL cMn = tu->cMn;			// fetch terminal min flow
			if (tu->asKVol)             		// if autoSizing terminal for constant volume
				if (Top.tp_pass1A)			// in autoSizing pass 1 part A, model is changed to const temp,
					cMn = 0.f;			// change min flow to zero.
			// redundant: cnah2:ahVshNLims sets tu->cMn 0 in this case.7-95.
			DBL czMn = tu->znC(cMn);		// flow at zone for min flow at terminal.
			DBL cv /*=0.*/;			// for 0 or vav heat cap flow rate of active (air) mode
			// (redundant init removed 12-94 when BCC32 4.5 warned)
#define MAXREEST 3			// max number of times to re-estimate ah in subhour (see also canReTemp).
			// historically 1, without canReTemp, 4-95; 1 tested ok 4-18-95 .458
			// 7-->3 4-22-95. 7 was random larger value for use canReTemp, 4-18-95.
			// TESTED 7-->3 no difference in cks11.bat tests.
			BOO canReEst = 			// can change ah mode or supply temp (re ZN/ZN2/WZ/CZ) if ...
				ah->timesReEst < MAXREEST		//  re-estimate count not used up (0'd in AH::begSubhr)
				&&  ( Top.tp_sizing && ah->fanAs.az_active	//  and ah supply fan is now being autoSized or has ...
					  || ah->sfan.vfDs > 0 );		/*      non-0 capacity. (ausz could make 0 vfDs; turning on 0-flow ah
								could cause ah-tu nonconvergence. Precaution 10-96.) */

		BOO canReTemp = canReEst 			// limit to one ahReEst per call to just adjust supply temp when mode ok...
							&& (timesReTemped < 1);	/* cuz won't necess ever get supply temp within any particular tolerance,
							   and want to save reEsts for mode changes. */
			BOO tuSizing = Top.tp_sizing;		// TRUE if autoSizing now, for ArStH/C. cases clear if not autoSizing their fcn.
			// set in cases:
//#ifdef DEBUG2					/* clear for handy debugging; remove when bcc32 4.5 complains, 5-95.
//						   bcc32 4.52 complained; removed 1-96. */
//          DBL czMx=0, tzMn=0, tzMx=0;		// max flow at terminal; zone temps that would result from min and max flows.
//#else
			DBL czMx, tzMn, tzMx;			// max flow at terminal; zone temps that would result from min and max flows.
//#endif

			// macro to get ZN/2 zone temp for min flow & coils off.
			// ah->getTsMnFo calcs ah min flow coil off tSup in detail (slow) on 1st call.
#define TZMNFO ( /*tzMnFo=*/ czMn ? (czMn * ah->getTsMnFo(TRUE) + a)/(bZn + czMn) : a/bZn )	// just a/b if no min flow

			// macro to get refined estimate of variable volume ZN/2 zone temp for max flow & coils off.
			// if constant volume, ah->ah_tSup already as accurate as we can get, so just use tzMx.
			// ah->getTsMxFo calcs ah max flow coil off tSup in detail (slow) on 1st call.
#define TZMXFO ( /*tzMxFo=*/ czMx > .99*czMn ? (czMx * ah->getTsMxFo(TRUE) + a)/(bZn + czMx) : tzMx )

			switch (x->zhxTy)
			{

			case LhStH:				// thermostat controlled local heat
				if ( tu->tuLhNeedsFlow==C_NOYESCH_YES	// if local heat requires air flow
						&&  !tuHasFlow( tu, md) )		// and there isn't any (below)
				{
					if (ft > sp)  md++;			// change modes: if float temp above setpoint, change up;
					else   md--;				// else, usually, change down a mode.
					break;
				}
				//q = sp * b - a;			above. heat required to hold setpoint
				t = sp;					// zone temp will be setpoint if no mode change (with no tFuzz, 4-95)
				// 4-95 compute temps & test with tFuzz b4 changing modes here if need found.
				if (q < tu->lhMn)       md++;		// change modes if out of range: if min heat is too much, change up;
				else if (q > tu->lhMx)  md--;		// ... max heat is not enuf, change down.
				else 					// q is ok
				{
					if ( tu->tuhc.coilTy==C_COILTYCH_HW	// if HW coil (plant-dependent)
							&&  !HpB.p[tu->tuhc.hpi].hpOn() )	// and its plant OFF and cannot be turned on (cnhp.cpp)
					{
						tu->qLhWant = q;			// request plant-off msg from tuEndSubhr
						if (ft > sp)  md++;  		// change modes: if float temp above setpoint, change up;
						else   md--;			// else, usually, change down a mode.
						break;
					}
					tu->qLhWant = 0.;			// clear plant-off message request
					tu->useLh = uStH;			// say terminal's lh is actively heating (setpoint control)
					tu->tuhc.qWant = tu->tuhc.q  	// q is actual heat coil output, and desired (HW coil) output if here
									 = lhq = q;				// lhq is for zone 'a' below
					if (tu->tuhc.coilTy==C_COILTYCH_HW)			// if a hot water coil (uses a plant)
						if (RELCHANGE(tu->tuhc.qPr,tu->tuhc.q) > Top.relTol)	// if q used changed since hp computed
						{
							Top.hpKf++;					// say a hp is flagged
							HpB.p[tu->tuhc.hpi].hpClf++;			/* say must CALL this hp so it can check if overall load
       							   		   change exceeds tolerance, and recompute if so. */
						}
					cz = 0.;  				// lh does not use air flow
					// lh does not use tu->aqO, bO. if tu also has air, ztuMdSets will set them.
				}
				break;			// end of LhStH case


			case ArStH:			// thermostat controlled air heat
				/* summary:  heat q = sp * b - a;  flow = q/dT;  if flow out of range, change modes.
				   plus stuff for wrong dT sign and temp tolerances for mode changes
				   and ah's controlled by zone req'ts. */
				/* CAUTION: when zone needs just a little heat,
				   ah may cool to counter fan heat or loss while this ztuMode case is used,
				   cuz the fan heat etc isn't in a/b/ft. 4-95. */
				// recoded 5-5-95, old is at end file.

				if (!tu->vhAs.az_active)
					tuSizing = FALSE;	// clear flag if not autoSizing this function

				if ( !tuSizing					// if not autoSizing tu heat flow
						&&  max( tu->tuVfMn, tu->tuVfMxH) <= 0 )	// if tu has 0 heat flow (poss from ausz, also allowed as input)
				{
					canReEst = FALSE;				// don't change ah mode: causes ah-tu nonconvergence 10-96
					canReTemp = FALSE;				// don't try to change ah's supply temp
				}

				// if air handler off, turn on or leave mode (when off, no flow, thus no q available)
				if (ah->ahMode==ahOFF)
				{
					if ( ft > sp  				// if heat not needed (if a/b > sepoint)
							||  ft >= sp && mdSeq[md+1]==MDS_FLOAT )	// if exact ==, go only to float mode: ==sp ArStC would loop.
						md++;					// go up a zone mode: float at/above sp, or ArStC.
					else if (isCtu && canReEst)			// if zhx's tu controls ah & can reEst now
					{
						/* turning ah on under ZN control: for input to estimating loop,
						   set terminal variables as tho setpoint met, for use in AH:zRat from AH:ahEstimate.
						   Else, 0 flow from float-above-sp may cause use of old tr, probably hier (from cooling),
						   causing up-mode again & possible convergence problems, or anyway wasting iteration. 4-25-95. */
						tu->useAr = uStH;					// say tu is actively (setpoint control) heating
						zn_md = md;						// terminal mode: AH:wzczSp() may use
						tu->aCv = max( .1*cMn + .9*tu->cMxH, cMn);	// ZN flow: 90% of way from min to max, not < min
						aTz = sp;						// zone temp: presume setpoint acheived
						// aWs: believe not used: wrNx recalc'd at ahCompute entry. qM: believe not used. aqO, bO: stored below.
						goto ahReModeH;				// try to turn ah on. for ZN; OFF unexpected for ZN2.
					}
					else						// else, usually
						md--;					// change down a zone mode: float below this heat sp, or use Lh.
					break;
				}

				// if zone too warm or too cold, conditionally re-estimate air handler for WZ/CZ/ZN/2 or change zone mode.

				//dT = tSup - sp				above
				//q = sp * b - a;    			above: heat power required from this zhx to hold setpoint
				czMx = tu->znC(tu->cMxH);		// max flow for mode. czMn set above switch.
				tzMn = (a + czMn * tSup)/(bZn + czMn);	// zone temp with min ah flow.   t = (a + q)/b, q = (tSup-tz)*c.
				tzMx = (a + czMx * tSup)/(bZn + czMx);	// zone temp with min ah flow @ current tSup
				// if's are arranged so temperatures within tolerance stay in current mode -- else can thrash

				if ( tzMn > sp + tFuzz			// if min flow makes zone clearly TOO WARM
						&&  tzMx > sp /* -tFuzz/2 ? */ )   	// and max flow makes zone at all too warm (backwards case)
				{
					// no (or neg?) bkwds tol cuz ah nonconvergent at bkwds sp mode limits: alternates min/max flow.

					// ArStH zone TOO WARM. If zone that controls ah needs SOME heat, use larger tol and try to get better supply temp
					if ( isCtu				// if doing control terminal of ZN/ZN2 controlled ah
							&&  q > 0				// if zone needs heat (not "bkwds" heat). TESTED makes T135e cnvrge 5-95.
							&&  TZMNFO < sp - tFuzz )		// if md++ (coil off, min flow) would make zone clearly too cold
					{
						if ( tzMn > sp + tFuzz2		// if min flow makes zn too warm with larger tol for reEst
								//&& tzMx > sp + ...		needed to use different tolerance from outer 'if'
								&&  canReTemp )			// if can re-estimate here now for supply temp change
							goto ahReTempH;     		// go re-estimate heating supply temp: may get lower

						else	// ZN/2 needs some flow but not as much heat as min flow at current tSup, but in tol or can't reEst.
							;	/* STAY IN MODE despite overheat: FALL THRU to compute (min) flow after if-else.
	                           ahCompute should improve tSup, but assuming any particular accuracy might loop.
	                           uneeded ZN/2 md++/md-- disruptive cuz shuts off coil & maybe fan. */
					}
					else					// most cases where min or max flow makes zone too warm
					{
						md++;
						break;			// go to next warmer mode: float above sp @ min flow, cool, etc.
					}
				}
				else if ( tzMx < sp - tFuzz		// if max flow clearly makes zone TOO COLD
						  &&  tzMn < sp /* -tFuzz/2 ? */ )	// or min flow makes zone too cold (backwards case)
				{
					// no (or neg?) bkwds tol cuz ah nonconvergent at bkwds sp mode limits: alternates min/max flow.

					// ArStH zone TOO COLD

					if (tzMx < sp - tFuzz2)   			// if too cold by larger re-estimating tolerance
						//&& tzMn > sp - ...			needed to use different tolerance from outer 'if'
					{
						//includes all backwards cases that get to the 'if'

						/* If heat coil off and needed, turn on now.  Don't just let md--, md++ turn on coil cuz slow and
									ah may not converge under uMxH if full heat not needed. 5-95.*/
						if ( !(ah->ahMode & ahHEATBIT)			// if heat coil not enabled (but fan on if here)
								&&  CHN(ah->ahhc.sched)==C_OFFAVAILVC_AVAIL	// if heat coil present and sched available (redundant?)
								&&  isCtu && canReEst )				// if tu controls ah & can reEst now
							/* reEst for bkwds? Ok, I think ah convergence problems similar for
							   unneeded uMxH and bkwds sp min limit: both alternate min/max. */
							goto ahReModeH;  				// reEst to turn coil on for ZN/ZN2

						{
							if (canReTemp)				// if haven't used up supply temp reEstimate count limit
							{
								// conditionally re-est ZN/ZN2 supply temp
								if (isCtu)				// if this terminal controls air handler
									goto ahReTempH;			// go try to get a better supply temp now: may save an iteration.

								// conditionally increase coolest-zone-supply temp to save an iteration.
								// note no conditional decrease for CZ cuz don't expect constant volume (cMn < cMxH for CZ).
								if ( !ISNUM(ah->ahTsSp) 			// if using "coolest zone" supply temp control
										&&  CHN(ah->ahTsSp)==C_TSCMNC_CZ		// ..
										&&  !ah->coilLimited 			// and not at full output (last subhr)(else don't bother)
										&&  ah->tsSp1 < ah->ahTsMx   			// nor max supply temp ditto
										&&  CHN(ah->ahhc.sched)==C_OFFAVAILVC_AVAIL )	// and heat coil present and scheduled available
									goto ahReTempH;   				// go reEstimate: try to increase CZ supply temp
							}
							if (!tuSizing)		// if not autoSizing air heat now (if so, fall thru to increase max flow below)
							{
								md--;   				// most too-cold cases go to down next colder mode
								break;   				// ... go to backup heat, float below sp @ full power, etc.
							}
						}
					}
					else 					// not enuf too cold to reEstimate, not backwards
						if (isCtu && !(ah->ahMode & ahHEATBIT))	// if zone-ctrl'd ah isn't heating

							;		/* tu controls ah that isn't heating and temp not much low.
				   STAY IN MODE (FALL THRU) despite underheat to assist tu-ah convergence
				   when leak makes tSup get rapidly better as flow falls (else can loop). 4-22-95. */

					/* Except for this case, ZN/2 too cold doesn't say in mode like too warm,
					   cuz ah is already heating and unneeded md-- md++ smoother than md++ md--:
					   md-- from here max's out heating ah without discontinuity,
					   but md++ can turn off or switch to cool. */
						else
							if (!tuSizing)		// if not autoSizing air heat now (if so, fall thru to increase max flow below)
							{
								md--;
								break;   				// most only-a-little-too-cold cases go to next colder mode
							}
				}
				else		// SETPOINT CASE: a flow between min and max can come within tolerance of setpoint.
				{
					// if here and dT > 0, q also > 0: regular case.
					if (dT < 0)		// if heating supply air cools
					{
						// backwards setpoint heating case: q < 0, dT < 0: ft above heating setpoint.
						/* when min flow non-0 and ah is zone-controlled, adjust flow to meet setpoint:
						   AH::setTsSp1 establishes tSup to apply minimum necessary heat to air cooled by leaks/losses,
						   and we "heat" with air colder than zone. But just go to 0 flow (via md++) when allowed. */
						if ( !isCtu			// if not ZN/ZN2 control terminal, don't backwards heat

								// do bkwds only early in global convergence: not always possible (estimating not perfect!); insurance.
								||  Top.iter >= 4		// abandon after 4 overall ah-tu iterations. MATCH in cnah.cpp:antRatTs.

								/* if next mode is setpoint cool, just use normal cool not bkwds heat: yields same temp if sp same.
								   TESTED saves iterations, T136a, 5-6-95. */
								||  mdSeq[md+1] != MDS_FLOAT		// if there's a float mode, next sp different, stay here
								&&  ZhxB.p[mdSeq[md+1]].zhxTy==ArStC	// if there's anything but sp cooling, i dunno, so say here

								/* let zone drop from setpoint mode out of backwards heating when heat coil no longer needed --
								   else may stick at hi flow in sp mode due to fan heat. TESTED really needed, T155d 5-95.
								   DOES hold, once here, when coil only countering leak/loss, when ZN cd be off: believe realistic. */
								||  ah->coilUsed != cuHEAT 	// if ah didn't use heat coil (not valid after reEst)
								&&  TZMNFO > sp - tFuzz 	/* if zone temp not too cold without heat coil:
                            				   to let zone get INTO bkwd heat after reEst even tho coilUsed unset
                            				   (else md++'s and may run coil in float mode). */

								/* don't use backwards setpoint mode if max heat-coil-off flow won't clearly cool zone enuf.
								   macro gets a more precise (& slower) tSup estimate. 5-11-95. TESTED fixes (cooling) non-converges. */
								||  TZMXFO >= sp )		// no tolerance cuz ah non-convergent near bkwd sp max flow
						{
							md++;
							break;		// backwards conditions not met, go up mode, to min flow above sp, or to cool, etc
						}
						// fall thru to calculate "backwards heating" flow as usual (dT, q both have reversed signs).
					}
				}

				// stay in mode, compute flow & resulting zone temperature (may be a little off due to mode-change tolerances above)

				// assorted cases fall thru from various places. dT and q can have any sign combination here.
				// min/max jumps w/o hysteresis remain near origin; if nec add code to change toward a float mode for stability.
				// If any mode changes added, go other way if dT < 0.
				if (dT==0)				// if tSup==sp and in this mode, don't divide by 0
					goto minHF;				/* use min flow 7-4-95. (b4 autoSize, used max flow; interfered with
							   dmd measurements. 90% approach would be ok if more flow needed.
							   Also in cooling below and in cnah2.andRatTs.
							   TESTED no change difference in all tests 7-8-95.) */
				else if (dT < 0 && q > 0) 		// if tSup won't heat and warmth wanted, use max flow
					goto maxHF;
				else 					// normal case, backwards case; also dT > 0 && q < 0 --> min flow.
				{
					cz = q/dT;  				// flow at zone to meet setpoint
					cv = tu->vavC(cz);			// vav box flow to get zone flow despite leak
				}				 	// flow might be a little out of range due to temperature tolerance
				if (cv < cMn)				// if flow came out < vav minimum
				{
minHF:
					cv = cMn;  				// use min
					goto hf1;
				}
				else if (cv > tu->cMxH)			// if flow came out > vav max for heating
				{
					// recoded here 7-22-95; big deletion; see cnztu.cp8.
					if ( tuSizing			// if sizing this function now (redundant)
							&&  tu->resizeIf( cv, FALSE, 	// cond'ly resize tuVfMxH and cMxH / if done
											  tSup * bZn - a,	//  runaway flow limiter, see derivation with TU::resizeIf
											  FALSE ) )
						;					// CAUTION cv may be changed, must compute cz and t after here.
					else					// not resized: not resizing, not a new max (unexpected), etc
maxHF:
						cv = tu->cMxH; 			// use max vav flow
hf1:
					cz = tu->znC(cv);			// zone flow resulting from cv
					t = (a + cz * tSup)/(bZn + cz);     	// zone temp this flow produces: may be off sp due to tolerances.
					q = (tSup - t) * cz;      		// power delivered by this zhx at this zone temp. === t * b - a.
				}
				else
					t = sp;				// flow in range: temp is setpoint, q is as previously set.
				tu->useAr = uStH;			// say terminal is actively air heating (setpoint control)
				tu->cv = tu->aCv = cv;   		// store terminal vav flow, 2 copies, aCv for ah to modify.
				tu->cz = cz;				// flow that gets to zone.
				tu->aqO = a;
				tu->bO = bZn;		// a and b for air handler to determine c's for active terminal
				// autoSize: undo prior increases this subhour if this flow is less.
				//  for example, so ah frFanOn can remain 1.0 when fanCycles.
				tu->resizeIf( cv, FALSE, 0, TRUE);	// above. intended to reduce tuVfMxH/cMxH, if > cv, toward bVfMxH.
				break;					// end of ArStH case


			case ArStC:			// thermostat controlled air cooling
				/* summary:  heat q = sp * b - a;  flow = q/dT;  if flow out of range, change modes.
				   plus stuff for wrong dT sign and ah's controlled by zone req'ts.*/
				/* CAUTION: when zone needs just a little coolth,
				   ah may heat to counter duct loss while this ztuMode case is used,
				   cuz the loss etc isn't in a/b/ft. 4-95. */
				// recoded 5-5-95, old is at end file.

				if (!tu->vcAs.az_active)  tuSizing = FALSE;	// clear flag if not autoSizing this function

#if 1 // 10-24-96
				if ( !tuSizing					// if not autoSizing cool tu flow
						&&  max( tu->tuVfMn, tu->tuVfMxC) <= 0 )	// if tu has 0 cool flow (poss from ausz, also allowed as input)
				{
					canReEst = FALSE;				// don't change ah mode: causes ah-tu nonconvergence 10-96
					canReTemp = FALSE;				// don't try to change ah's supply temp
				}
#endif

				// if air handler off, turn on or leave mode (when off, no flow, thus no q available)
				if (ah->ahMode==ahOFF)
				{
					if ( ft < sp  				// if coolth not needed (if a/b < sepoint)
							||  ft <= sp && mdSeq[md+1]==MDS_FLOAT )	// if exact ==, go only to float mode: ==sp ArStH would loop.
						md--;					// go down a zone mode: float at/below sp, or ArStH.
					else if (isCtu && canReEst)			// if zhx's tu controls ah & can reEst now
					{
						/* turning ah on under ZN control: for input to estimating loop,
						   set terminal variables as tho setpoint met, for use in AH:zRat from AH:ahEstimate.
						   Else, 0 flow from float-below-sp may cause use of old tr, probably lower (from heating),
						   causing down-mode again & possible convergence problems, or anyway wasting iteration. 4-25-95. */
						tu->useAr = uStC;					// say tu is actively (setpoint control) cooling
						zn_md = md;						// terminal mode: AH:wzczSp() may use
						tu->aCv = max( .1*cMn + .9*tu->cMxC, cMn);	// ZN flow: 90% of way from min to max, not < min
						aTz = sp;						// zone temp: presume setpoint acheived
						// aWs: believe not used: wrNx recalc'd at ahCompute entry. qM: believe not used. aqO, bO: stored below.
						goto ahReModeC;				// try to turn ah on. for ZN; OFF unexpected for ZN2.
					}
					else						// else, usually
						md++;					// change up a zone mode: float above this cool sp, etc.
					break;
				}

				// if zone too cold or warm, conditionally re-estimate air handler for WZ/CZ/ZN/2 or change zone mode.

				//dT = tSup - sp				above
				//q = sp * b - a;    			above: heat power required from this zhx to hold setpoint: < 0 to cool
				czMx = tu->znC(tu->cMxC);		// max flow for mode. czMn set above switch.
				tzMn = (a + czMn * tSup)/(bZn + czMn);	// zone temp with min ah flow.   t = (a + q)/b, q = (tSup-tz)*c.
				tzMx = (a + czMx * tSup)/(bZn + czMx);	// zone temp with min ah flow @ current tSup
				// if's are arranged so temperatures within tolerance stay in current mode -- else can thrash

				if ( tzMn < sp - tFuzz			// if min flow makes zone clearly TOO COLD
						&&  tzMx < sp /* +tFuzz/2 ? */ )   	// and max flow makes zone at all too cold (backwards case)
				{
					// no (or reverse?) bkwds tol cuz ah nonconvrgnt at bkwds sp mode limits: alternates min/max flow.

					// ArStC zone TOO COLD. If zone that controls ah needs SOME coolth, use larger tol and try to get better supply temp
					if ( isCtu				// if doing control terminal and ZN/ZN2 ctrl'd ah
							&&  q < 0				// if coolth needed (not "bkwds"). TESTED makes T135e cnvrge 5-95.
							&&  TZMNFO > sp + tFuzz )		// if md-- (coil off, min flow) would make zone clearly too warm
					{
						if ( tzMn < sp - tFuzz2		// if min flow makes zn too cold with larger tol for reEst
								//&& tzMx < sp - ...		needed to use different tolerance from outer 'if'
								&&  canReTemp )			// if can re-estimate here now for supply temp change
							goto ahReTempC;     		// go re-estimate supply temp: may get higher

						else	// ZN/2 needs some flow but not as much coolth as min flow at curr tSup, but in tol or can't reEst.
							;	/* STAY IN MODE despite overcool: FALL THRU to compute (min) flow after if-else.
	                           ahCompute should improve tSup, but assuming any particular accuracy might loop.
	                           uneeded ZN/2 md--/md++ disruptive cuz shuts off coil & maybe fan. */
					}
					else					// most cases where min or max flow makes zone too cold
					{	md--;
						break;			// go to next cooler mode: float below sp @ min flow, heat, etc.
					}
				}
				else if ( tzMx > sp + tFuzz		// if max flow clearly makes zone TOO WARM
						  &&  tzMn > sp /* +tFuzz/2 ? */ )	// or min flow makes zone too warm (backwards case)
				{
					// no (or reverse?) bkwds tol cuz ah nonconvrgnt at bkwds sp mode limits: alternates min/max flow.

					// ArStC zone TOO WARM

					if (tzMx > sp + tFuzz2)   			// if too warm by larger re-estimating tolerance
						//&& tzMn > sp - ...			needed to use different tolerance from outer 'if'
					{
						//includes all backwards cases that get to the 'if'

						/* If cool coil off and needed, turn on now.  Don't just let md++, md-- turn on coil cuz slow and
									ah may not converge under uMxC if full cool not needed. 5-95.*/
						if ( !(ah->ahMode & ahCOOLBIT)			// if cool coil not enabled (but fan on if here)
								&&  CHN(ah->ahcc.sched)==C_OFFAVAILVC_AVAIL	// if cool coil present and sched available (redundant?)
								&&  isCtu && canReEst )				// if tu controls ah & can reEst now
							/* reEst for bkwds? Ok, I think ah convergence problems similar for
							   unneeded uMxC and bkwds sp min limit: both alternate min/max. */
							goto ahReModeC;   				// reEst to turn coil on for ZN/ZN2

						{
							if (canReTemp)				// if haven't used up supply temp reEstimate count limit
							{
								// conditionally re-est ZN/ZN2 supply temp
								if (isCtu)				// if this terminal controls air handler
									goto ahReTempC;			// go try to get a better supply temp now: may save an iteration.

								// conditionally decrease warmest-zone-supply temp to save an iteration.
								// note no conditional decrease for WZ cuz don't expect constant volume (cMn < cMxC for WZ).
								if ( !ISNUM(ah->ahTsSp) 			// if using "warmest zone" supply temp control
										&&  CHN(ah->ahTsSp)==C_TSCMNC_WZ		// ..
										&&  !ah->coilLimited   			// and not at full output (last subhr)(else don't bother)
										&&  ah->tsSp1 > ah->ahTsMn   			// nor min supply temp ditto
										&&  CHN(ah->ahcc.sched)==C_OFFAVAILVC_AVAIL )	// and cool coil present and scheduled available
									goto ahReTempC;    				// go reEstimate to reduce WZ supply temp
							}
#if 1	// with if 0 above. match heat when works.
							if (!tuSizing)		// if not autoSizing air cool now (if so, fall thru to increase max flow below)
#endif
							{
								md++;				// most too-warm cases go to up next warmer mode
								break;				// ... go float above sp @ full power, use next tu, etc.
							}
						}
					}
					else 					// not enuf too warm to reEstimate, not backwards
						if (isCtu && !(ah->ahMode & ahCOOLBIT))	// if zone-ctrl'd ah isn't cooling

							;		/* tu controls ah that isn't cooling and temp not much high.
				   STAY IN MODE (FALL THRU) despite undercool to assist tu-ah convergence
				   when leak makes tSup get rapidly better as flow falls (else can loop). 4-22-95. */

					/* Except for this case, ZN/2 too warm doesn't say in mode like too cold,
					   cuz ah is already cooling and unneeded md++ md-- smoother than md-- md++:
					   md++ from here max's out cooling ah without discontinuity,
					   but md-- can turn off or switch to cool. */
						else
							if (!tuSizing)		// if not autoSizing air cool now (if so, fall thru to increase max flow below)
							{	md++;
								break;   				// most only-a-little-too-warm cases go to next warmer mode
							}
				}
				else		// SETPOINT CASE: a flow between min and max can come within tolerance of setpoint.
				{
					// if here and dT < 0, q also < 0: regular case.
					if (dT > 0)		// if cooling supply air heats
					{
						// backwards setpoint cooling case: q > 0, dT > 0: ft below cooling setpoint.
						/* when min flow non-0 and ah is zone-controlled, adjust flow to meet setpoint:
						   AH::setTsSp1 establishes tSup to apply minimum necessary coolth to air cooled by leaks/losses,
						   and we "cool" with air warmer than zone. But just go to 0 flow (via md++) when allowed. */
						if ( !isCtu			// if not ZN/ZN2 control terminal, don't backwards cool

								// do bkwds only early in global convergence: not always possible (estimating not perfect!); insurance.
								||  Top.iter >= 4		// abandon after 4 overall ah-tu iterations. MATCH in cnah.cpp:antRatTs.

								/* if next mode is setpoint heat, just use normal heat not bkwds cool: yields same temp if sp same.
								   TESTED saves iterations, T136a, 5-6-95. */
								||  mdSeq[md-1] != MDS_FLOAT		// if there's a float mode, next sp different, stay here
								&&  ZhxB.p[mdSeq[md-1]].zhxTy==ArStH	// if there's anything but sp cooling, i dunno, so say here

								/* let zone drop from setpoint mode out of backwards cooling when no cool coil longer needed --
								   else may stick at hi flow in sp mode due to leak/loss. TESTED really needed, T155d 5-95.
								   DOES hold, when coil only countering fanHeat/leak/loss, when ZN2 cd be off: believe realistic. */
								||  ah->coilUsed != cuCOOL	// if ah didn't use cool coil (not valid after reEst)
								&&  TZMNFO < sp + tFuzz 	/* if zone temp not too warm without cool coil:
                            				   to let zone get INTO bkwd cool after reEst even tho coilUsed unset
                            				   (else md--'s and may run coil in float mode). */

								/* don't use backwards setpoint mode if max cool-coil-off flow won't clearly heat zone enuf.
								   macro gets a more precise (& slower) tSup estimate. 5-11-95. TESTED fixes T166d & T154f non-cvgs, 5-95. */
								||  TZMXFO <= sp )		// no tolerance cuz ah non-convergent near bkwd sp max flow
						{
							md--;
							break;		// bkwds conditions not met, go down mode, to min flow below sp, or to heat, etc
						}
						// fall thru to calculate "backwards cooling" flow as usual (dT, q both have reversed signs).
					}
				}

				// stay in mode, compute flow & resulting zone temperature

				// assorted cases fall thru from various places. dT and q can have any sign combination.
				// min/max jumps w/o hysteresis remain near origin; if nec add code to change toward a float mode for stability.
				// If any mode changes added, go other way if dT < 0.
				if (dT==0)				// if tSup==sp and in this mode, don't divide by 0
					goto minCF;				/* use min flow 7-4-95. (b4 autoSize, used max ...
							   Also in heating above and in cnah2.andRatTs.) */
				else if (dT > 0 && q < 0)		// if tSup won't cool and coolth wanted, use max flow
					goto maxCF;
				else 					// normal case, backwards case; also dT < 0 && q > 0 --> min flow.
				{
					cz = q/dT;  				// flow at zone to meet setpoint
					cv = tu->vavC(cz);			// vav box flow to get zone flow despite leak
				}				 	// flow might be a little out of range due to temperature tolerance
				if (cv < cMn)				// if flow came out < vav minimum
				{
minCF:
					cv = cMn;  				// use min
					goto cf1;
				}
				else if (cv > tu->cMxC)			// if flow came out > vav max for cooling
				{
					// recoded here 7-22-95; big deletion; see cnztu.cp8.
					if ( tuSizing			// if sizing this function now (redundant)
							&&  tu->resizeIf( cv, TRUE, 	// cond'ly resize tuVfMxC and cMxC / if done
											  tSup * bZn - a,	//  runaway flow limiter, see derivation with TU::resizeIf
											  FALSE ) )
						;					// CAUTION cv may be changed, must compute cz and t after here.
					else					// not resized: not resizing, not a new max (unexpected), etc
maxCF:
						cv = tu->cMxC; 			// use max vav flow
cf1:
					cz = tu->znC(cv);			// zone flow resulting from cv
					t = (a + cz * tSup)/(bZn + cz);     	// zone temp this flow produces: may be off sp due to tolerances.
					q = (tSup - t) * cz;      		// power delivered by this zhx at this zone temp. === t * b - a.
				}
				else
					t = sp;				// flow in range: temp is setpoint, q is as previously set.
				tu->useAr = uStC;			// say terminal is actively air cooling (setpoint control)
				tu->cv = tu->aCv = cv;   		// store terminal vav flow, 2 copies, aCv for ah to modify.
				tu->cz = cz;				// flow that gets to zone.
				tu->aqO = a;
				tu->bO = bZn;		// a and b for air handler to determine c's for active terminal
				// autoSize: undo prior increases this subhour if this flow is less.
				//  for example, so ah frFanOn can remain 1.0 when fanCycles.
				tu->resizeIf( cv, TRUE, 0, TRUE);  	// above. intended to reduce tuVfMxC/cMxC, if > cv, toward bVfMxC.
				break;					// end of ArStC case


ahReTempH:			// come here to re-estimate ah heating supply temp from ArStH case
				timesReTemped++;			// say don't re-est again just for temperature this call
ahReModeH:   		// come here to re-estimate ah to change to heating from ArStH case
				if (isCtu)				// for ZN/ZN2 specify ah mode desired
					tu->wantMd = ahHEATING;		// (not used for other tSup sp control methods eg WZ/CZ/(RA))
				tu->useAr = uStH;			// set stuff ah->Estimate will use: terminal use: active air heat
				goto ahReEst;

ahReTempC:			// come here to re-estimate ah cooling supply temp from ArStC case
				timesReTemped++;			// say don't re-est again just for temperature this call
ahReModeC:   		// come here to re-estimate ah to change to cooling from ArStC case
				if (isCtu)				// for ZN/ZN2 specify ah mode desired
					tu->wantMd = ahCOOLING;		// (not used for other tSup sp control methods eg WZ/CZ/(RA))
				tu->useAr = uStC;			// set stuff ah->Estimate will use: terminal use: active air cool

ahReEst:		// come here to re-estimate ah supply temperature in current mode
				tu->aqO = a;
				tu->bO = bZn;		// a and b for computing flow and/or supply temp required
				ah->timesReEst++;			// count times re-estimated re loop prevention
				ah->ahEstimate();			// re-estimate ah: for ZN/ZN2/WZ/CZ/(RA), sets ahMode and ah->tSup. cnah.cpp.
				goto reDo;				/* redo setpoint case: eg tSup, already fetched, may be changed,
							   (but believe don't need to repeat ztuAbs()) */
				/* note there is no aggressive turn-OFF of unneeded air handler in ztuMode,
				   cuz don't understand that it is urgent re results, and cuz might loop if eg fan heat makes discontinuity.
				   Turnoff happens: AH::setTsSp1 (some 0-flow cases), TU::tuEstimate (next subhour),
				   and now in ZNR::ztuAbs at most once per subhour. 4-95. */

			} // end switch zhxTy

			// sums for humidity, for setpoint modes: add active-terminal contribution if any to sums from ztuAbs
			cSum += cz;					// total ah flow to zone, used in wb in ZNR::wZn.
			switch (Top.humMeth)				// sum c * latest or average ah_wSup per humidity method, for computing wz
			{
			case C_HUMTHCH_PHIL:
				wcSum1 += cz * (ah->ah_wSupLs + ah->ah_wSup)/2.;
				break;  	// flow * avg supply hum
			case C_HUMTHCH_ROB:
				wcSum1 += cz * ah->ah_wSup;
				break;	// flow * latest supply hum
			}
			wcSum += cz * ah->ah_wSup;    	// flow * hum, not averaged, possibly for zn_qlHvac (not used 6-92 ****).

		}  // end of if float... else (setpoint) ...

		// done if zone mode not changed
		if (md==mdWas) 
			break;

		// detect non-convergence
		int mdd = md - mdWas;						// mode delta: sign is of interest here
		if (iterCount++)  if ((mdd ^ priorMdd) < 0)  huntCount++;	// after 2 iterations, count mode delta sign changes
		priorMdd = mdd;
		if (huntCount==3)		// 3 sign changes is inf loop
		{
			tFuzz *= 10.f;    		// make tests looser
			setToMax( tFuzz2, tFuzz);	// ..
			huntCount = 0;		// start non-convergence detection over. added 5-97
			static SI count2 = 0;
			if (count2++ < 100)  				// rer (runtime error) errCount++'s: stops run upon ret to a higher level.
				rer( MH_R1255, tFuzz, int(md));	// "Internal error: ztuMode is increasing tFuzz to %g (mode %d)"
		}
	}  // for ( ; ; )

// store zone results.  No change-check here cuz ah computes and checks tr and cr from tz's and c's.
	if (t < PSYCHROMINT || t > PSYCHROMAXT)
	{
		rer( "Zone '%s': unreasonable air temp = %0.1f", Name(), t);
		t = max( DBL( PSYCHROMINT), min( t, DBL( PSYCHROMAXT)));		// constrain result (else psychro trouble)
	}
	tz = t;			// store zone temp, our copy for results (see ztuEndSubhr).
	aTz = t;		// zone temp: air handler working copy: repeatedly modified during ah computations.
	cM = cz;		// 0. or flow (heat cap units) of active zhx -- for probing.
	qM = q;			// 0. or power of active zhx
	zn_md = md;		// mode number (mdSeq index): for accessing zhx; starting point next time.
	zn_tzsp = sp != 0.f ? sp : spWas > 0.f ? spWas : zn_tzsp;
	//also cSum, wcSum, wcSum1 stored directly above.

// overall zone a and b's (used by ztuMdSets for other-tu tu->aqO and bO)
	this->zn_aq = a + cz * tSup + lhq;  // for active terminal, add  flow * tSup  or  q  (other is 0; both 0 if floating)
	this->zn_b = bZn + cz;				// add flow, or 0

#if defined( _DEBUG)
	// check consistency of total flow to zone
	// zn_bHvO is sum of c'z of other terminals
	if (frDiff( cSum, double(zn_bHvO + cz)) > 0.0001)
		printf("\ncSum mismatch");
#endif
	// SAME AS cSum -- verify and delete one? 5-92.
// zone humidity ratio
	aWz =					// working copy of wz for air handler to modify
		wz = znW( wcSum1, cSum, tz, &znXLGain);	// set members: wz to zone humidity ratio, znXLGain to excess latent gain if any.
												// note use of special wcSum. fcn below.

// store outputs of terminals other than the active terminal for this zone mode, flag ah's that need calling
	ztuMdSets(md);				// set tu tuhc.q's and c's; .use's; aqO/bO/tzO, wcO/cO/wzO, possibly ahPtf.
	// 10-92: conditionally sets ahClf using tz, wz, tu cz, Pr's.

// zone's TOTAL (sensible) hvac power.        q = sigma(lhQ) + sigma(Ts - tz) * c     	where c is cap flow rate, Btuh/F.
//                                     thus   q = sigma(lhQ) + sigma(Ts*c) - tz*sigma(c)
	zn_qsHvac = q				// power of active (tstat ctrl'd, not pegged) zhx  [qM, tz not stored yet]
			 + zn_aqHvO	   		// sigma(lhQ+Ts*c) for set output and pegged zhx's
			 - t * zn_bHvO;    	// - tz * sigma(c) for set output and pegged zhx's
	// why not compute zn_qsHvac as  bLdSh * t - aqLdSh?  Cuz throws in all roundoff error, putting many .000's in reports.
	// [note zn_qsHvac may be updated by AH:antRatTs when it adjusts tz.]

	if (!zn_IsConvRad())
	{	// zone's latent power, similarly.      Moisture inflow = sigma(c * (ah_wSup - wz)) = sigma(c*ah_wSup) - wz*sigma(c).
		DBL wDiffC = 0;
		switch (Top.humMeth)	// use average or latest values per humidity calc method, else zone latent gain does not balance
		{
		case C_HUMTHCH_PHIL:
			wDiffC = wcSum1 - (wz + wzls) / 2. * cSum;	// sigma(c * (average ah_wSup - average wz)), as in znW.
			break;    					// balance fails unless both ah_wSup and wz are average.
		case C_HUMTHCH_ROB:
			wDiffC = wcSum1 - wz * cSum;
			break;   	// sigma(c*ah_wSup) - wz*sigma(c).  note humMeth changes wcSum1.
		}
		zn_qlHvac = wDiffC / Top.tp_airSH	// moisture inflow * heat cap flow / spec heat to get mass flow (lb/hr)
			* PsyHCondWtr;				// * 1061 to get power (Btuh)
	}

#ifdef DEBUG2//cnglob.h
// but can't power be computed more easily this way??  assumes no vent etc not in zn_qsHvac.
// due to subtraction, difference in 3rd digit common for small q's.  But which is more right? above is more direct.
	static SI testCount = 0;
	if (++testCount <= 10  &&  !Top.isWarmup)
	{
		DBL q2 = zn_bLdSh * t - zn_aqLdSh;				// q = b*t - a:  I claim this should be same (and its easier)
		float scale = fabs(zn_bLdSh * t + zn_aqLdSh)+1.;	// measure accuracy as fraction of inputs (note '+'), /0 protect
		if (ABSCHANGE(zn_qsHvac/scale,q2/scale) >  1.e-12) 	// 1.e-10: no msgs. 1.e-11: no msgs. 1.e-12: no msgs 1992-.
			rInfo(MH_R1256, zn_qsHvac, q2);			// "Inconsistency in ztuMode: zn_qsHvac (%g) != q2 (%g)"
	}
#endif

	return rc;			// error return(s) above
}			// ZNR::ztuMode
//----------------------------------------------------------------------------------------------------------------------------
RC ZNR::ztuMdSeq()				// build zone hvac terminal mode sequence table

// also checks for equal setpoints with equal priorities, and for cooling setpoint > heating setpoint
// sets: ZNR.nMd, ZNR.mdSeq; sorts zone SET TEMP zhx chain.
{
	ZHX *x, *nx;
	SI swaps;
	RC rc=RCOK;

// SORT zone's SET TEMP zhx chain in ascending zone mode order

	// zn's SET TEMP zhx's were chained during setup; here Bubble sort into order; expect few swaps after first time.
	do					// passes til found already in order
	{
		swaps = 0;
		TI xi;
		for (TI *prev = &zhx1St;  (xi = *prev) != 0;  prev = &x->nxZhxSt4z) 		// loop SET TEMP zhx's for zone,
		{
			// with indirection to permit relinking.
			x = ZhxB.p + xi;
			TI nxi = x->nxZhxSt4z;
			if (!nxi)  break;					// done pass if no zhx after x
			nx = ZhxB.p + x->nxZhxSt4z;				// access following zhx

			// compare zhx's, continue if no swap needed
			if (nx->sp > x->sp)   continue;			// greater setpoint: already in order
			if (nx->sp==x->sp)					// equal setpoints
			{
				BOO xCool = x->zhxTy & (StC|nv);	 		// TRUE if x does cooling (includes nat vent (?) )
				if (!xCool != !(nx->zhxTy & (StC|nv)))		// if x cools .xor. nx cools (! to booleanize)
				{
					if (!xCool)  continue;				// heating mode always precedes cooling mode
				}
				else if (x->spPri==nx->spPri)		// "Equal setpoints (%g) with equal priorities (%d) in zone '%s' -- \n"
				{
					return rer( MH_R1257,		// "    %s and %s"
								x->sp, x->spPri, Name(),
								x->ui  ? strtprintf( MH_R1258, TuB.p[x->ui].Name())  : "natvent",	// "terminal '%s'"
								nx->ui ? strtprintf( MH_R1258, TuB.p[nx->ui].Name()) : "natvent" );	// ditto
				}
				else if (!xCool)    				// if both heating
				{
					if (x->spPri >= nx->spPri)   continue;		// higher priority is lower mode (ie used last)
				}
				else						// both cooling
					if (x->spPri <= nx->spPri)   continue;		// lower pri is lower mode (ie used first)
			}
			//else nx->sp < x->sp: lesser setpoint: unconditionally do swap.

			// swap zhx's by rearranging list linkage
			x->nxZhxSt4z = nx->nxZhxSt4z;
			nx->nxZhxSt4z = xi;
			*prev = nxi;
			swaps++;
		}
	}
	while (swaps);

// runtime CHECK for cooling setpoint < heating setpoint (efficient place to do it)

	for (x = NULL;  nxZhxSt(x);  )				// loop over the now sorted SET TEMP zhx's for zone
	{
		ZHX *xArH = ZhxB.p + x->xiArH;				// zhx[0] or corress heating zhx for a cooling zhx
		if ( x->zhxTy==ArStC 					// if a set temp air cool zhx
				&&  x->sp < xArH->sp )					// if terminal also heats (note), with larger setpoint
			return rer( MH_R1259,			// "Cooling setpoint temp (%g) is less than heating setpoint (%g)\n"
						x->sp, xArH->sp, 			// "    for terminal '%s' of zone '%s'"
						TuB.p[x->ui].Name(), ZrB.p[x->zi].Name());	/* note: if terminal does not also heat, x->xiArH will be 0.
								   Zhx record 0 is all 0's.  x->sp assumed >= 0 F. */
	}

// FORM MODE SEQUENCE by inserting floats between sorted zhx's and filling mdSeq[] array. And find top heat & first cool mode.

	SI iMd = 0;					// local mode sequence table subscript/counter
	SI hi = 0, ci = 32767;			// for highest-numbered heat mode, lowest-numbered cool mode
	mdSeq[iMd++] = MDS_FLOAT;			// mode 0 is always float below all setpoints
	for (x = NULL;  nxZhxSt(x);  )		// loop over the now sorted SET TEMP zhx's for zone
	{
		TI nxi = x->nxZhxSt4z;
		ZHX* pNx = ZhxB.p + nxi;
		if      (x->zhxTy & StH)  setToMax( hi, iMd);	// determine highest heat mode
		else if (x->zhxTy & StC)  setToMin( ci, iMd);	//  and lowest cool mode, 7-3-95
		x->mda = iMd;  				// set zhx's active mode (mode where it is tstat ctrl'd, not pegged)
		mdSeq[iMd++] = x->ss;			// mode sequence table SET TEMP entry is zhx subscript
		if (!nxi || pNx->sp > x->sp)   		// if end or next sp hier, add FLOAT mode b4 nxt SET TEMP mode
			mdSeq[iMd++] = MDS_FLOAT;   		// mode sequence table FLOAT entry is a negative constant
	}
	nMd = iMd;					// store # modes in zn member
	setToMin( ci, iMd);

// initialize mode to between heating and cooling, 7-3-95.

// intended to make sure any modes being autoSized are seen first as setpoint modes (where autoSize occures)
// before being pegged (for which autoSize is not yest implemented). 7-3-95.

	zn_md = (ci + hi)/2;		// should yield float mode tween heat & cool, or top heat sp mode if equal heat & cool setpoints.

	return rc;
}			// ZNR::ztuMdSeq
//----------------------------------------------------------------------------------------------------------------------------
RC ZNR::ztuAbs( 			// compute stuff for zn's terminals in given zone mode

	int md, 				// desired zone mode (may differ from zp->zn_md)
	BOO fromZtu,  			// 0 from AH::setTsSp1: don't activate ah's from each other (insurance)
	DBL &aqHvO,  DBL &bHvO,	// receive summed a and b contributions from terminals other than active (tstat-ctrl'd)
    						// terminal in zone for mode -- all terminals if in a float mode.
	DBL &wcO,				// receives sum of latest w * flow, for all terminals but active, for wcSum.
	// >> wsSum unused 6-12-92.  later delete this arg, and member, and rename wsSum1 back.
	DBL &wcO1, 				// receives sum of flow * w (latest or average per Top.humMeth), for wcSum1 for calculating hum.
	DBL &cO )				// receives sum of flows (heat cap units) of all terminals but active, for cSum.

// callers: ZNR::ztuCompute, AH::setTsSp1
{
	// note: if Mn > Mx, Mn is used. done for lh in TU::begHour (for now 3-92), ar in AH::ahVshNLims.

	RC rc=RCOK;
	DBL aSum = 0., bSum = 0., wc = 0.;		// zero local accumulators (shorter/faster code than accum to arguments (?))
	DBL wc1 = 0.;

	for (ZHX *x = NULL;  nxZhx(x);  )		// for each ZHX serving this zone
	{
		TU *tu = TuB.p + x->ui;		// zhx's terminal
		AH *ah = AhB.p + x->ai; 	// point air handler if any
		DBL tSup = ah->ah_tSup;   	// fetch air handler supply temp
		SI mda = x->mda;			// zone mode in which this SET TEMP zhx is tstat-controlled
		DBL cz;						// local for terminal flow at zone (after leaks) in air cases
		switch (x->zhxTy)
		{
		case LhStH:					// set temp local heat.  q's do not contribute to bHvO.
			if (md==mda)  break;				// believe needed, but was not present 4-95.
			if ( ( tu->tuLhNeedsFlow != C_NOYESCH_YES 	// if flow not needed
					|| tuHasFlow( tu, md) )				//    or flow present -- else 0
					&& ( tu->tuhc.coilTy != C_COILTYCH_HW 		// and not an HW coil
						 || HpB.p[tu->tuhc.hpi].hpOn() ) )  	//    or its plant is (now) on
				aSum += (md < mda) ? tu->lhMx : tu->lhMn;     	// in lower (cooler) mode use full local heat, else min
			break;	  					// ..in hier mode, use min.  m==mda does not get here.

		case LhSo:					// set output local heat (lu->lhMn)
			if ( ( tu->tuLhNeedsFlow != C_NOYESCH_YES 		// if flow not needed
					|| tuHasFlow( tu, md) )			//    or flow present -- else 0
					&& ( tu->tuhc.coilTy != C_COILTYCH_HW 		// and not an HW coil
						 || HpB.p[tu->tuhc.hpi].hpOn() ) )  		//    or its plant is (now) on
				aSum += tu->lhMn;  		// this is a Q not a UAT: accumulate Q for aqHvO; no flow for bHvO.
			break;						// local heat makes no humidity contribution.

		case ArSo:					// set output air heat/cool (heat/cool not differentiated when no setpoint)
			if (ah->ahMode != ahOFF)  				// if air handler is running (don't care if ahHEATING or -COOLING)
			{
				cz = tu->znC(tu->cMn);				// compute min flow after duct leaks beyond vav
				goto haveFlow;					// go accumulate flow, flow * t, flow * w
			}
			break;

		case ArStH:					// set temp air heat
			if (md==mda)  break;				// active zone mode zhx is handled by caller
			// conditionally start ZN/ZN2 tu-controlled air handler whose operation is implied by requested zone mode, 4-95.
			// note logic at start of ztuMode to adjust search start mode to minimize thrash from this startup.
			if ( fromZtu					// not if AH::setTsSp1 called: insurance: might thrash or loop?
					&&  md < mda			// if ah should be max heating in this zone mode
					&&  !(ah->ahMode & ahHEATBIT)			// but ah heat coil isn't on (ah may be off, fan only, or cool)
					&&  CHN(ah->ahSch) != C_AHSCHVC_OFF		// ah sched ON (or future WARMUP)
					&&  tu->ctrlsAi && tu->ctrlsAi==x->ai	// if zhx's tu controls zhx's air handler 5-2-95
					&&  ah->isZNorZN2   					// if ah supply temp is under ZN or ZN2 control this hour
					// don't turn on ah if no flow possible: just turns itself off again --> ah-tu nonconvergence, 10-96.
					&&  (max( tu->tuVfMn, tu->tuVfMxH) > 0 || Top.tp_sizing && tu->vhAs.az_active)	// tuVfMxH==0 is allowed input.
					&&  (ah->sfan.vfDs > 0 || Top.tp_sizing && ah->fanAs.az_active) )		// vfDs==0 could result from autoSizing.
			{
				TU *ctu = TuB.p + ah->ahCtu;			// point ZN/ZN2 control terminal for this air handler
				ctu->wantMd = ahHEATING;			// have terminal tell ah that it wants heat
				ctu->useAr = uMxH;				// tu use: max heat. setTsSp1,ahVshNLims (from ahEstimate) may use.
				ah->ahEstimate();				// attempt to turn on air handler and estimate its supply temp
				tSup = ah->ah_tSup;				// re-fetch supply temp: may have changed
			}
			if (ah->ahMode==ahOFF)				// if air handler is not running (not even fan)
				break;						// it has no output and no humidity contribution
			// lower (cooler) mode gets max heat
			if (md < mda)					// m==mda does not get here
			{
				cz = tu->znC(tu->cMxH);				// max heating flow at zone, after leaks after vav
				goto haveFlow;					// go accumulate flow, flow * t, flow * w
			}
			// not max heating.  If terminal can also cool and is now cooling thru its other zhx, nothing here.
			if (x->xiArC)					// if terminal also has a cooling zhx
				if (md >= ZhxB.p[x->xiArC].mda)  		// if other zhx [active or] pegged cooling in this zone mode
					break;					// done here.  do tu when we get to its other zhx.
			// hier (warmer) mode: min flow (or ZN off).  Includes heat/cool terminal floating between setpoints.
			goto minFlow;					// min flow same whether heating or cooling; join cool zhx case

		case ArStC:					// set temp air cool
			if (md==mda)
				break;			// active mode zhx is handled by caller
			// conditionally start ZN/ZN2 tu-controlled air handler whose operation is implied by requested zone mode, 4-95.
			if ( fromZtu					// not if AH::setTsSp1 called: insurance: might thrash or loop?
					&&  md > mda			// if ah should be max cooling in this zone mode
					&&  !(ah->ahMode & ahCOOLBIT)			// but ah cool coil isn't on (ah may be off, fan only, or cool)
					&&  CHN(ah->ahSch) != C_AHSCHVC_OFF		// ah sched ON (or future WARMUP)
					&&  tu->ctrlsAi && tu->ctrlsAi==x->ai		// if zhx's tu controls zhx's air handler 5-2-95
					&&  ah->isZNorZN2   				// if ah supply temp is under ZN or ZN2 control this hour
					// don't turn on ah if no flow possible: just turns itself off again --> ah-tu nonconvergence, 10-96.
					&&  (max( tu->tuVfMn, tu->tuVfMxC) > 0 || Top.tp_sizing && tu->vcAs.az_active)	// tuVfMxC==0 is allowed input.
					&&  (ah->sfan.vfDs > 0 || Top.tp_sizing && ah->fanAs.az_active) )		// vfDs==0 could result from autoSizing.
			{
				TU *ctu = TuB.p + ah->ahCtu;		// point ZN/ZN2 control terminal for this air handler
				ctu->wantMd = ahCOOLING;			// have terminal tell ah that it wants coolth
				ctu->useAr = uMxC;				// tu use: max cool. setTsSp1,ahVshNLims (from ahEstimate) may use.
				ah->ahEstimate();				// attempt to turn on air handler and estimate its supply temp
				tSup = ah->ah_tSup;				// re-fetch supply temp: may have changed
			}
			if (ah->ahMode==ahOFF)				// if air handler is not running
				break;						// it has no output
			// hier (warmer) mode gets max cooling
			if (md > mda)					// m==mda does not get here
			{
				cz = tu->znC(tu->cMxC);				// max cooling flow at zone, after leaks after vav
				goto haveFlow;					// go accumulate flow, flow * t, flow * w
			}
			// not max cooling; if terminal also heats, do nothing here now (float was done for its cool zhx)
			if (x->xiArH)
				break;
			// lower (cooler) mode, or mode between heat and cool (goto from case ArStH), gets min flow.
minFlow: ;					// heat zhx case joins here when between heat and cool
			// conditionally shut down ZN or ZN2 tu-controlled air handler which should not operate in requested zone mode, 4-95.
			// note mode hysteresis in ztuMode to minimize ah's going on and off.
			// for more ah turnoff stuff see AH::setTsSp1, TU::tuEstimate, .
			if ( fromZtu					// not if AH::setTsSp1 called: insurance: might thrash or loop?
					&&  tu->ctrlsAi && tu->ctrlsAi==x->ai		// if zhx's tu controls zhx's air handler 5-2-95
					&&  ah->isZNorZN2 )   				// if ah supply temp is under ZN or ZN2 control this hour
			{
				AHMODE wantMd = CHN(ah->ahTsSp)==C_TSCMNC_ZN2 ? ahFAN : ahOFF;	// for ZN2 fan stays on
				if (wantMd != ah->ahMode)					// if ah not already in desired mode
				{
					/* In some cases ah with non-0 min flow must stay on in float mode:
					      if heat needed but coil-off min flow fan heat more than needed heat,
					      if cool needed but coil-off min flow leakage cooling more than needed cooling, etc.
					   Prevent looping in these cases by turning off only once per subhour. */
					if (ah->timesReEst==0)			// if air handler has not been re-estimated this subhour.
					{
						TU *ctu = TuB.p + ah->ahCtu;  		// point ZN/ZN2 control terminal for this air handler
						ctu->wantMd = wantMd;   			// set desired ah mode in control terminal: off or fan
						ctu->useAr = uNONE; 			// no tu air use. setTsSp1,ahVshNLims (from ahEstimate) may use.
						ah->timesReEst++;				// prevent repetition
						ah->ahEstimate();				// attempt to change ah mode per ctu->wantMd
						if (ah->ahMode==ahOFF) 			// if air handler turned off (as expected for ZN, not for ZN2)
							break;					// it has no output and no humidity contribution
					}
				}
			}
			cz = tu->znC(tu->cMn);				// compute min flow (after any leaks after vav)
haveFlow: ;					// other air handler cases join here with flow cz set
			bSum += cz;						// accumulate flow, for bHvO
			aSum += cz * tSup;				// accumulate supply t * flow, for aqHvO
			wc1 += cz * (ah->ah_wSupLs + ah->ah_wSup)/2.;		// accumulate average supply hum rat * flow, for wcO1
			wc += cz * ah->ah_wSup;					// accumulate latest supply hum rat * flow
			break;
		}
	}

	aqHvO = aSum;
	bHvO = bSum;			// return results via reference args
	cO = bSum;
	wcO = wc;  				// ...  note cO is same as bHvO: total flow
	switch (Top.humMeth)			// sum flow times average or latest supply w per humidity calc method
	{
	case C_HUMTHCH_PHIL:
		wcO1 = wc1;
		break; 	// sigma( c * average ah_wSup) for calculating wz by Niles' central difference
	case C_HUMTHCH_ROB:
		wcO1 = wc;
		break;    	// sigma( c * ah_wSup) for calculating wz Rob's way, as aw/bw
	default:
		rerErOp( PWRN, MH_R1260, Top.humMeth);   	// "Bad Top.humMeth %d in cnztu.cpp:ZNR::ztuAbs"
	}

	// ... So and StO members can now be combined?

	return rc;
}		// ZNR::ztuAbs
//----------------------------------------------------------------------------------------------------------------------------
void ZNR::ztuMdSets( int md)	// set outputs of all zone's terminals but active one, and zone hum rat, and some change checks

// sets:  for each terminal in zone: tuhc.q & qWant, cz, cv, aCv, aqO, bO, tzO, wcO, cO, wzO.
//        for terminal's air hander: ahPtf, if tzO or wzO changed. 6-22-92.
//                                   ahClf, if zone tz or wz or tu cz changed, 10-92.

// related code: cnah.cpp:antRatTs may in some cases redetermine aCv same as here: coordinate any changes.
{
	// decisions in this loop are same as in ztuAbs;
	//  outputs include tu data dependent on sums from ztuAbs and intervening mode desision.

	for (ZHX *x = NULL;  nxZhx(x);  )			// for each ZHX serving this zone
	{
		TU *tu = TuB.p + x->ui;				// terminal for zhx
		AH *ah = AhB.p + x->ai;				// air handler for terminal, if any
		DBL tSup = ah->ah_tSup; 			// supply temp of air handler for terminal
		SI mda = x->mda;					// mode in which this zhx is tstat-controlled if SET TEMP
		BOO floatMode = mdSeq[md]==MDS_FLOAT;		// in floating modes we will set tu->aqO/bO.
		DBL cz;						// temporary for flow in air cases
		switch (x->zhxTy)
		{
		case LhSo:				// set output local heat
			tu->lhMnPr = tu->lhMn;			// reset comparator used in TH::begHour to set ZNR.ztuCf to get here
			tu->qLhWant = 0.;					// clear "plant off" message check request to tuEndSubhr
			if ( tu->tuLhNeedsFlow==C_NOYESCH_YES		// if flow needed
					&&  !tuHasFlow( tu, md) )				// and not present
			{
lhOff:
				tu->useLh = uNONE;
				tu->tuhc.qWant = tu->tuhc.q = 0.f;		// no local heat
				if (tu->tuhc.coilTy==C_COILTYCH_HW)			// if a hot water coil (uses a plant)
					if (RELCHANGE(tu->tuhc.qPr,tu->tuhc.q) > Top.relTol)	// if q used changed since hp computed
					{
						Top.hpKf++;   					// say a hp is flagged
						HpB.p[tu->tuhc.hpi].hpClf++;			/* say must CALL this hp so it can check if overall load
       							   		   change exceeds tolerance, and recompute if so. */
					}
				break;
			}
			tu->tuhc.qWant = tu->tuQMnLh;			// desired heat: possibly larger if heatPlant overloaded
			tu->useLh = uSo;
			tu->tuhc.q = tu->lhMn;				// set out local heat output is spec'd by 'minimum' input (Btuh)
			//lh does not use aqO/bO/wcO/cO -- and members might be used by ah in same tu.
checkPlant:
			if ( tu->tuhc.coilTy==C_COILTYCH_HW 		// if an HW coil
					&&  !HpB.p[tu->tuhc.hpi].hpOn() )  		// and its plant isn't on and won't turn on
			{
				tu->qLhWant = tu->tuhc.q;			// request 'plant off' msg check from tuEndSubhr if non-0
				goto lhOff;					// go set no lh
			}
			if (tu->tuhc.coilTy==C_COILTYCH_HW)			// if a hot water coil (uses a plant)
				if (RELCHANGE(tu->tuhc.qPr,tu->tuhc.q) > Top.relTol)	// if q used changed since hp computed
				{
					Top.hpKf++;						// say a hp is flagged
					HpB.p[tu->tuhc.hpi].hpClf++;     			/* say must CALL this hp so it can check if overall load
       							   		   change exceeds tolerance, and recompute if so. */
				}
			break;

		case LhStH:					// set temp local heat
			tu->lhMnPr = tu->lhMn;				// reset comparators used in tu::begHour ..
			tu->lhMxPr = tu->lhMx;				// .. to set ZNR.ztuCf to get here
			tu->qLhWant = 0.;					// clear "plant off" message check request to tuEndSubhr
			if (mda==md)
				break;						// for zhx active in current mode, ztuMode sets tuhc.q.
			if ( tu->tuLhNeedsFlow==C_NOYESCH_YES		// if flow needed
					&&  !tuHasFlow( tu, md) )				// and not present
				goto lhOff;					// go set no local heat
			if (md < mda) 					// max heat in cooler modes
			{
				tu->tuhc.qWant = tu->tuQMxLh;			// desired heat: possibly larger if heatPlant overloaded
				tu->useLh = uMxH;
				tu->tuhc.q = tu->lhMx;				// max local heat
			}							// lh does not use aqO/bO/wcO/cO.
			else               				// min in warmer modes
			{
				tu->tuhc.qWant = tu->tuQMnLh;			// desired heat: possibly larger if heatPlant overloaded
				tu->useLh = uMn;
				tu->tuhc.q = tu->lhMn;				// min local heat
			}
			goto checkPlant;					// but turn lh off if needed plant not on

		case ArSo:					// set output air heat/cool (not differentiated when no setpoint)
			//found unused 4-17-95, mbr deleted:
			// tu->cMnPr = tu->cMn;				// comparator used in AH::ahVshNLims to set ZNR.ztuCf to get here
			if (ah->ahMode==ahOFF) 				// if air handler off, no flow.
				goto ahOff;	  				// go set uNONE and zero aqO, bO, cv, cz.
			tu->useAr = uSo;					// ah running, heat/cool not differentiated
			tu->cv = tu->aCv = tu->cMn;			// vav flow (heat cap units, same units as UA) is "minimum"
			cz = tu->znC(tu->cv);				// portion of flow that gets to zone (some leaks to return)
			goto floatOrPeg;					// join other non-setpoint cases, cz set

		case ArStH:					// set temp air heat
			//found unused 4-17-95, mbrs deleted
			// tu->cMnPr  = tu->cMn;				// reset comparators used in AH::ahVshNLims ..
			// tu->cMxHPr = tu->cMxH;				// .. to set ZNR.ztuCf to get here
			if (mda==md)					// for zhx active in current mode, ztuMode sets cv,cz,aqO,bO.
				goto setpoint;				// join other case to do hum stuff
			if (ah->ahMode==ahOFF) 			// if air handler off, no flow.
				goto ahOff;	  				// go set uNONE and zero aqO, bO, cv, cz.
			if (md < mda)
			{
				tu->useAr = uMxH;				// lower (cooler) mode: max heat
				tu->cv = tu->aCv = tu->cMxH; 			// terminal vav flow: our copy and air handler working copy
				cz = tu->znC(tu->cv);				// flow that gets to zone
				goto floatOrPeg;				// join other non-setpoint cases, cz set
			}
			// not max heating; if this terminal is now max output cooling, no output for its air heat zhx
			if (x->xiArC)					// if tu also cools
				if (md >= ZhxB.p[x->xiArC].mda)    		// if its cooling zhx is [active or] max-cooling in this md
					break;					// no output now: we will get to its cooling zhx presently
			// hier (warmer) mode gets min flow, incl heat/cool terminal floating between setpoints
			goto minFlow;

		case ArStC:					// set temp air cool
			//found unused 4-17-95, mbrs deleted
			// tu->cMnPr  = tu->cMn;				// reset comparators used in AH::ahVshNLims ..
			// tu->cMxCPr = tu->cMxC;				// .. to set ZNR.ztuCf to get here
			if (mda==md)					// for zhx active in current mode, ztuMode sets c's and O's.
			{
setpoint:
				;
				cz = tu->cz;  					// fetch active tu flow (at zone) computed in ztuMode
				goto tzOhum;					// go do humidity stuff. aqO, bO already set by ztuCompute case.
			}
			if (ah->ahMode==ahOFF) 				// if air handler off, no flow, [no q].
			{
ahOff:
				tu->useAr = uNONE;
				tu->cv = tu->aCv = tu->cz = 0.f;
				tu->aqO = tu->bO = 0.f;				// insurance
				//tu->wcO = wcSum1; tu->cO = cSum;		believe values don't matter when ah off
				break;
			}
			if (md > mda)
			{
				tu->useAr = uMxC; 				// hier (warmer) mode: max cool
				tu->cv = tu->aCv = tu->cMxC;  	// terminal flow at vav box is cooling maximum
				cz = tu->znC(tu->cv);			// portion of flow that gets to zone (some leaks to return)
				goto floatOrPeg;				// join other non-setpoint cases, cz set
			}
			// not max cooling; if terminal also heats, do nothing here now
			if (x->xiArH)					// if same terminal has air heating capability
				break;						// flow was computed for its heat zhx (cmSo or cmStxx)
			// lower (cooler) mode, or mode between heat and cool (via goto from ArStH), gets min flow.
minFlow:
			tu->useAr = uMn;
			tu->cv = tu->aCv = tu->cMn;  			// tu flow at vav box, and ah working copy: set to min
			cz = tu->znC(tu->cv);				// tu flow that gets to zone, after leaks after vav box
floatOrPeg:
			;
			tu->cz = cz;					// store tu flow that gets to zone
			if (floatMode)					// if mode is not tstat-ctrl'd temp
			{
				tu->aqO = zn_aq - cz * tSup;   			// a and b excluding this tu, so ah can update tz
				tu->bO = zn_b - cz;          			// ... when tSup (if coil limited) or c (if fan limited) change
			}
			else
			{
				tu->aqO = tu->bO = 0.f; 			// 0 says (some other) sp ctrls tz, stays same til mode change
			}
tzOhum: ;		// common exit for air cases. cz contains tu->cz.

			// if return flow or conditions from zone to this tu's ah changes, flag ah to be called.

			if (!ah->ahPtf && !ah->ahClf)			// skip for speed if ah already flagged
				if ( ABSCHANGE(wz, tu->wzPr) > Top.absHumTol	// if tu's zone's hum changed..
						||  ABSCHANGE(tz, tu->tzPr) > Top.absTol	// tu's zone temp.. These Pr's updated in cnah.cpp:ah::upPriors.
						||  RELCHANGE(cz, tu->czPr) > Top.relTol )	// tu's flow ah-->zone, thus zone-->ah.
				{
					Top.ahKf++;					// say an ah needs calling
					ah->ahClf++;				// say CALL this ah so it will recompute if OVERALL return...
				}  						//  flow/conditions changed out of tolerance

			// set tu members for tz,wz,cz without this tu, re monitoring & updating when only this tu or its ah changes

			tu->tzO = tu->bO != 0. ? tu->aqO/tu->bO 		// what zone's temp wd be w/o this ah, for change-monitoring,6-92
					  : 0.; 			// .. or 0 if zone temp controlled by some other setpoint tu.
			// tzO unused 6-22-92, rip it out later if still unused. *********
			switch (Top.humMeth)		// set zone total flow * ah_wSup excluding this tu, for ah to use with new ah_wSup*cz.
			{
			case C_HUMTHCH_PHIL:
				tu->wcO = wcSum1 - cz * (ah->ah_wSup + ah->ah_wSupLs)/2.;
				break;	// use average ah_wSup's
			case C_HUMTHCH_ROB:
				tu->wcO = wcSum1 - cz * ah->ah_wSup;
				break;	// use latest ah_wSup's
			}
			tu->cO = cSum - cz;				// zone total flow excluding this tu: for ah use with updated cz.
			tu->wzO = znW( tu->wcO, tu->cO, tz);	// what zone's w wd be w/o this ah, for change-monitoring 6-92.

			// set ah must-compute flag on change in zone w excluding this terminal

			/* Intended to insure doing ahCompute() when wO alone changes:
			   wO can control wz via infil, but other tests to do ahCompute may not respond to wO change alone when wz is
			   dominated by ah_wSup, as ah_wSup is updated only when ahCompute() is done (and, usually, iterated several times).
			   Eg: CZ12RV2.CEC Jan 1 1-6AM, esp in test files T5 & T47, 6-22-92.
			     TESTED: fixes non-response on wO-only change; also adds ah executions which have no affect visible in reports.
			   Test tzO as well as wzO?? TESTED 6-22-92 (pre TOLF): many addl ahCompute executions; didnt seem to affect results.
			   These -Prs updated in AH::upPriors from AH::ahCompute. */

#define TOLF 2	/* tolerance multiplier for fewer flaggings, 2 cuz 4 gets response to .0004 wO change, 6 does not;
			want response to .0002 wO change with current humTolF (.00003, probably too big, 6-22-92).
			MAKE BIGGER if .0004 step (T5) detected some other way. */
				if ( //ABSCHANGE(tu->tzO, tu->tzOPr) > Top.absTol ||     		zone temp likewise: wasted time 6-22-92
					ABSCHANGE(tu->wzO, tu->wzOPr) > Top.absHumTol*TOLF )		// if zone hum excluding this tu changes,
				{
					Top.ahKf++;							// say an air handler needs calling
					ah->ahPtf++;							// say this air handler needs recomputing
				}
			break;
#undef TOLF
		}
	}
}				// ZNR::ztuMdSets
//---------------------------------------------------------------------------------------------------------------------------
/*  Notes on getting condensation right, 5-97.
Problem is getting znXLGain right for latent energy balance (so qlAir matches qlInfil in ZNR::ztuEndSubhr above, when no hvac).
Test case: T60X 12 Dec: hum driven by Infil, when getting warmer & wetter outsize.
1. Original method: compute w as before, condense excess at znW exit: made 10-15% too little znXLGain to balance. T60x.r3.
2. Condensing out excess from wzls and wo at znW entry made about 250% too much znXLGain and undersaturated the zone. T60x.r4.
3. Compute, if supersaturated, condense the excess out of wzls and iterate. BINGO! Perfect balance in T60x test case (no hvac).
   But uses all 10 iterations coded; gains about 9x each iteration. Need to find requirement. T60x.r5.
   a. 4 iterations, 1e-8.  T60x.r5a: Yr BalL 2.79 on 23376 Btu; Dec 12 .447.
   b. 5 iterations, 1e-8.  T60x.r5b:  .318  .040.
   c. 6 iterations, 1e-8.  T60x.r5c:  .176, .050 ...  diminishing returns for # iterations with 1e-8.
   d. 6 iterations, 1e-9.  T60x.r5d:  .035, .006.
   e. 6 iterations, 1e-10. T60x.r5e:  .034, .005 ...  diminishing returns for magnitude of change with 10 iterations.
   f. 7 iterations, 1e-10. T60x.r5f:  .004, .001
   g. 8 iterations, 1e-10. T60x.r5g:  .002, .001
   h. 8 iterations, 1e-11. T60x.r5h:  .001, 0.
   i. 9 iterations, 1e-11. T60x.r5i:  0, 0.
   j. 9 iterations, 1e-10. T60x.r5j:  .002, .001. These are thousanths of Btu's.
   SO, revert to i to be sure. And, 5 iterations, 1e-9 probably plenty good, but otta test with more input files. 5-97.
*/
//---------------------------------------------------------------------------------------------------------------------------
DBL ZNR::znW( 			// compute zone humidity ratio

	DBL wc,		// humMeth = Rob:  sum of flows times w's into zone:  sigma(ah->ah_wSup * tu->cz) 	(or ah->aWs * tu->aCz, etc)
				// humMeth = Phil: sum of AVERAGE humrats times flows into zone:
				//				sigma( (ah->ah_wSup+ah->ah_wSupLs)/2. * tu->cz) 	(or ah->aWs, tu->aCz, etc)
	DBL c,		// sum of air handler capacity flows into zone, Btuh/F
	DBL _tz,		// zone temperature: .tz, or .aTz from antRatTs.
	DBL* pXLGain /*=nullptr*/) 	// optionally receives excess latent gain power rejected to prevent supersaturation, eg znXLGain.
// above are args so ah can pass unstored updated working values.
// Also uses: zone: .unInfil, .znLGain, .i.znVol.
// returns (does NOT store) zone w.

// humMeth = Phil notes 6-92: to get latent energy balance, must also use average ah_wSup AND wz to compute qlMech,
//           and average wz to compute qlInfil.
{
//inputs:
// uaInfil: heat capacity of air infiltrating into zone (Btuh/F): divide it by specific heat (Btu/lb-F) to get mass flow (lb/hr).
// znLGain: moisture gain (Btuh) from user input GAIN records via cnguts.cpp:doHourGains called from cnguts.cpp:doBegIvl.
// wc:      Rob:  sigma tu->c * ah_wSup
//          Phil: sigma tu->c*(ah_wSup + ah_wSupLs)/2.
// c:       sigma tu->c: total supply flow into zone, heat cap units.

#if defined( CRTERMAH)
	if (zn_IsConvRad())
	{	// use CR formulation for CR zones
		double amfX = c / Top.tp_airSH;	// lbm/hr
		double mwX = wc / Top.tp_airSH;

		double _wz;
#if defined( OLDHUM)
		if (Top.tp_dbgFlag == 1)
			_wz = zn_HumRat(amfX, mwX, _tz, pXLGain);
		else if (Top.tp_dbgFlag == 2)
		{
			double _wz1 = zn_HumRat(amfX, mwX, _tz, pXLGain);
			_wz = zn_AirXMoistureBal(false, amfX, mwX, _tz, pXLGain);
			if (frDiff(_wz, _wz1) > 0.0001)
				printf("\nMismatch");
		}
		else
#endif
			_wz = zn_AirXMoistureBal(true, amfX, mwX, _tz, pXLGain);
		return _wz;
	}
#endif
	
	DBL M = i.znVol * Top.airDens(_tz);		// mass of air in zone (use average temp (_tz+tzls)/2. ?? )
	DBL _wz = 0;
	float _wo = Top.wOSh;		// outdoor humidity
	DBL _wzls = wzls;			// zone humidity last subhour
	double xLGain = 0.;			// init to no supersaturation, no excess humidity to reject (ref arg)
	DBL satW = psyHumRat3(_tz);   	// maximum w air at this temp can hold. lib\psycho2.cpp function.

	for (int j = 0;  ;  )		// may repeat if supersaturated, per endtest below
	{
		// compute humidity
		switch (Top.humMeth)		// compute zone humidity by selected method.  Note that argument "wc" differs by method.
		{
			DBL aw, bw; 				// a and b for humidity balance equation
		case C_HUMTHCH_ROB:  			// Rob's basic aw/bw method, like temperature
			bw = M/Top.tp_subhrDur   		// denominator.  M/t,
				 + (c + zn_uaInfil)/Top.tp_airSH;		//   sigma(m): divide by airSH to get mass flow from heat cap flow.
			aw = _wzls*M/Top.tp_subhrDur					// numerator.  wLast * M / t,
				 + (wc + zn_uaInfil*_wo)/Top.tp_airSH	//   sigma(w*m): hums * mass flows, for air handlers and infiltration,
				 + znLGain/PsyHCondWtr;					//   w gain independent of wz: scheduled gain.
			_wz = aw/bw;   								// wz = aw / bw.
			break;

		case C_HUMTHCH_PHIL:			// Niles central difference method using all sorts of averages
			if (c==0.)					// if no flow (faster)
				// || (single zone ah && no outside air && coil not on) <-- add if can figure out tests (forward) to skip iterations ****
			{
				// supply air does not affect zone humidity, omit wc from calculation: speed convergence
				bw = zn_uaInfil / Top.tp_airSH / M;
				aw = bw * _wo + znLGain/ PsyHCondWtr / M;   	// aw = (uaInfil*Top.wOSh / Top.airSh + znLGain/1061) / M;
			}
			else	// general case
			{
				aw = ((zn_uaInfil*_wo + wc)/Top.tp_airSH + znLGain/PsyHCondWtr) / M;
				bw = (zn_uaInfil + c) / Top.tp_airSH / M;
			}
			DBL bwt2 = bw*Top.tp_subhrDur/2.;				// subexpr
			_wz = ((1. - bwt2)*_wzls + aw*Top.tp_subhrDur)/(1. + bwt2);	// compute zone w: central difference per Niles sect I-H.
			break;
		}

		// if supersaturated, condense out excess moisture from zone and conditionally iterate.
		// iteration found really necessary for latent balance (so qlInfil matches qlAir when no hvac), 5-97
		// each iteration got about .12 the excess as previous in a traced case.
		DBL xw = _wz - satW;		// humidity in excess of saturation
		if (xw <= 0)				// if not supersaturated
			break;					// done, leave loop
		xLGain += 					// heat from condensing the excess moisture
			xw * M 					// excess hum rat * mass of air in zone = excess water (lbs)
			* PsyHCondWtr 			//  * heat of condensation = energy (Btu)
			/ Top.tp_subhrDur;			//   / time interval = power (Btuh)
		if ( xw > 1.e-11    		// if change was in approx 8th sig dig of w ...  TUNING ITEM, notes above fcn.
				&&  ++j < 9 )		//  and haven't done 9 iterations (@ not quite 1 more sig dig each). TUNING ITEM
		{
			_wzls -= xw;
			continue;  			// reduce previous zone hum by excess much and iterate
		}
		_wz -= xw;				// reduce computed zone hum by excess
		break;					// and call it good enough
	}

	if (pXLGain)
		*pXLGain = xLGain;

	return _wz;

}			// ZNR::znW
//---------------------------------------------------------------------------------------------------------------------------
LOCAL BOO FC tuHasFlow( TU *tu, int md)  	// test whether air would flow thru terminal in zone mode 'md'

// for ztuMode and tuMdStOab and -cq to call when tuLhNeedsFlow.
{
	if (CHN(tu->tfanSch)==C_TFANSCHVC_ON)  return TRUE;	// if terminal fan is on, there is flow
	// to complete when tfan completed
	// if (CHN(tu->tfanSch)==C_TFANSCHVC_HEATING)  ..must add code (elsewhere) to turn it on..  return TRUE;

	if ( AhB.p[tu->ai].ahMode & ahON)			// if air handler on (Ah 0 is always ahOFF)
	{
		if (tu->znC(tu->cMn) > 0.f)			// if tu has minimum flow (after leaks)
			return TRUE;					//   there is flow
		if ( tu->xiArH		 			// if tu has air heat [or air settout]
				&&  tu->znC(tu->cMxH) > 0.f			//  with max flow not (now scheduled or leaked to) 0
				&&  md <= ZhxB.p[tu->xiArH].mda )   		//  and tu's air heat will be on in this mode (cuz cMn is 0 if here)
			return TRUE;					//   there is flow
	}

	return FALSE;					// no flow found
}
//-----------------------------------------------------------------------------------------------------------------------------
BOO ZNR::nxTu(TU *&tu)		// first/next terminal for zone.  set tu NULL b4 1st call!
// usage:  for (TU *tu = NULL;  zp->nxTu(tu);  )  { ... }
{
	TI ui = tu ? tu->nxTu4z : tu1;
	if (ui)
	{
		tu = TuB.p + ui;
		return TRUE;
	}
	return FALSE;
}
//-----------------------------------------------------------------------------------------------------------------------------
BOO ZNR::nxZhx( ZHX *&x)		// first/next zhx for zone.  set x NULL b4 1st call!
// usage:  for (ZHX *x = NULL;  zp->nxZhx(x);  )  { ... }
{
	TI xi = x ? x->nxZhx4z : zhx1;
	if (xi)
	{
		x = ZhxB.p + xi;
		return TRUE;
	}
	return FALSE;
}
//-----------------------------------------------------------------------------------------------------------------------------
BOO ZNR::nxZhxSt( ZHX *&x)		// first/next SET TEMP zhx for zone
{
	TI xi = x ? x->nxZhxSt4z : zhx1St;
	if (xi)
	{
		x = ZhxB.p + xi;
		return TRUE;
	}
	return FALSE;
}
//-----------------------------------------------------------------------------------------------------------------------------

// cnztu.cpp end
