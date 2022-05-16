// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnfan.cpp -- fan simulation routines for hvac airHandlers for CSE.

// file created 7-8-95.

// see cnztu for overall hvac and zone/terminals hvac simulation,
//     cnah for airHandler simulation (calls functions here)

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// FAN decls for member functions in this file

#include <CNGUTS.H>	// Top; put non-mbr decls for this file here if any.


/******************************************************************************************************************************
*  HVAC: FANs for AIRHANDLERS
******************************************************************************************************************************/
BOO AH::resizeFansIf( 		// conditionally change fan size(s) if being autoSized

	float _cMx,  		// new supply fan heat cap flow. Caller should have added tol (Top.auszHiTol2).
	float vfDsWas )		/* prior sfan.vfDs value (eg subHour start or iter4Fs entry): size may be reduced to this
    				   so momemtary increases during ah convergence thrash don't stick */

// returns TRUE if resized (6-97)

{
	// callers 7-95: xxx cnah2.cpp:iter4Fs (2 calls); possibly NO also cnztu.cpp:hvacIterSubhr.
	// caller 7-95: tentatively changing to cnah2.cpp:AH::puteRa().

	if (!Top.tp_sizing)  return FALSE;	// nop if not a part of autoSizing when incr'ing sizes (pass 1A & B, or 2B, or as changed)
	if (!sfan.ausz)  return FALSE;	// nop if not autoSizing at least the supply fan

	// caller has already included tolerance in new value in _cMx, to make comparable to vfDsWas.

	BOO ret = FALSE;
	float vfDsNu = _cMx / Top.airX(sfan.t);		// convert to cfm
	if ( fanAs.resizeIf( vfDsNu, FALSE)			// if autoSizing, if bigger, store & return TRUE
			||  fanAs.unsizeIf( max( vfDsNu, vfDsWas)) )	// or if not bigger, cond'ly decrease to old sfan.vfDs value 7-19-95
	{
		sfan.fn_setup2();					// if new size, re-setup, using vfDs as stored by resizeIf.
		sfan.fn_pute( sfan.c*sfan.frOn, sfan.t, sfan.frOn);	// recompute p q dT qAround for new capacity and prior c, t, frOn
		ret = TRUE;
	}

	if (rfan.ausz)				// if also autoSizing return fan
	{
		// rfan does not have own AUSZ structure.
		if (sfan.vfDs != rfan.vfDs)
		{
			rfan.vfDs = sfan.vfDs;
			rfan.fn_setup2();						// re-setup derived mbrs, using .vfDs, .t (function just below)
			rfan.fn_pute( rfan.c*rfan.frOn, rfan.t, rfan.frOn);	// recompute p q dT qAround for new capacity and prior c, t, frOn
			return TRUE;
		}
	}
	return ret;
}		// AH::resizeFansIf
//-----------------------------------------------------------------------------------------------------------------------------
void FAN::fn_setup2( 		// set up fan derived values, initially or during autoSize
	int options /*=0*/)		// option flags / bits
							// -1: clear all (for e.g. non-fan IZXFER)
							//  1: derive eff from shaftPwr (else derive shaftPwr from eff) (added 8-10)
							//  2: derive shaftPwr from elecPwr (implies option 1) (added 8-10)

// uses: .vfDs, .t -- as preset by caller else prior value.

// sets .outPower, .shaftPwr (or .eff), .inPower, .airPower, (.cMx).

// CAUTION: leaves prior values in .t, .c, .p, .q, .dT, .qAround.

// Notes:  this fcn corresponds to both setup and reSetup for coils.
//	   redundant calls or calls for absent fans ok.
//	   0 or NAN vfDs must work: may occur for absent fan, or b4 autoSizing begins.

{
	if (options == -1)
	{	// re unused FAN: set all to 0
		memset( this, 0, sizeof( FAN));
		return;
	}

	// nop if NAN in vfDs -- eg start of autoSizing, before 1st demand on fan. Note 0 falls thru ok.
	if (ISNANDLE(vfDs))
		return;

	// decode options
	int bShaftPwrInput = (options&1) != 0;
	int bElecPwrInput = (options&2) != 0;

	// setup derived numbers
	outPower =					// fan output: mechanical power pushing air. mbr checked in cncult5.cpp.
		vfDs * press 			// pressure * volume/time = power. units:  cfm*(in H2O)    or   ft3*(in H2O)/min
		* 5.2022f 				// times (lb/ft2)/(in H2O)    yielding     ft3*(lb/ft2)/min  or  ft*lb/min
		* .0012854f 			// times Btu/ft-lb            yielding     Btu/min
		* 60.f;					// times min/hr               yeilding     Btuh
#if 0	// incomplete experiment: allow elecPwr to be 0
	if (bElecPwrInput)
	{	if (elecPwr <= 0.f)
		{	shaftPwr = outPower / eff;
		}
		else
		{	shaftPwr = elecPwr*vfDs*3.413f * motEff;
			eff = outPower / shaftPwr;	// implied efficency (shaftPwr > 0 per input check)

	}
	if (bShaftPwrInput)
		eff = outPower / shaftPwr;	// implied efficency (shaftPwr > 0 per input check)
	else
		shaftPwr = outPower / eff;	// shaft power for output and fan efficiency (eff > 0 per input check)
	float inPower = shaftPwr / motEff;		// motor input power consumption at design flow (motEff > 0 per input check)
	if (!bElecPwrInput)
		elecPwr = vfDs > 0.f ? (inPower/3.413f) / vfDs : 0.f;
	airPower = motPos==C_MOTPOSCH_INFLO 	// heat to air at fan at design flow, used in pute()
			   ?  inPower			// motor in airflow: all input: motor ineff + fan ineff + fan output
			   :  shaftPwr;			// motor elsewhere:  fan power (fan ineff + fan output) only.
	// note modelling of fan output as heat added to air AT FAN.
	// heat to air around motor (eg if INRETURN): inPower - airPower.
#else
	if (bElecPwrInput)
	{	shaftPwr = elecPwr*vfDs*3.413f * motEff;
		bShaftPwrInput = TRUE;
	}
	if (bShaftPwrInput)
		eff = outPower / shaftPwr;	// implied efficency (shaftPwr > 0 per input check)
	else
		shaftPwr = outPower / eff;	// shaft power for output and fan efficiency (eff > 0 per input check)
	float inPower = shaftPwr / motEff;		// motor input power consumption at design flow (motEff > 0 per input check)
	if (!bElecPwrInput)
		elecPwr = vfDs > 0.f ? (inPower/3.413f) / vfDs : 0.f;
	airPower = motPos==C_MOTPOSCH_INFLO 	// heat to air at fan at design flow, used in pute()
			   ?  inPower			// motor in airflow: all input: motor ineff + fan ineff + fan output
			   :  shaftPwr;			// motor elsewhere:  fan power (fan ineff + fan output) only.
	// note modelling of fan output as heat added to air AT FAN.
	// heat to air around motor (eg if INRETURN): inPower - airPower.
#endif

// set cMx now so it isn't 0 -- insurance -- varies with air temp at run time
	cMx = vfDs * vfMxF * Top.airX(t);   	// convert max flow with overrun to heat cap, at last t used or init value (member)
}					// FAN::fn_setup2
//-----------------------------------------------------------------------------
double FAN::fn_puteVf(			// compute runtime variables for fan based on volume flow
	DBL /*CFLOW*/ vf,   	// current TIME-AVERAGE flow, cfm
	DBL /*TEMP*/  _t,		// approx temp (F) of air fan is blowing (needed to conv Btuh/F to cfm), when on
	DBL /*FRAC*/ _frOn /*=1.*/)	// fraction of time on: divide vf by this to get operating volume flow
// returns temp rise across fan (=.dT), F
{
	DBL _c = vf * Top.airX( _t);
	return fn_pute( _c, _t, _frOn);
}		// fn_puteVf
//-----------------------------------------------------------------------------
double FAN::fn_pute(     		// compute runtime variables for fan

	DBL /*CFLOW*/ _c,   	// current TIME-AVERAGE flow in heat capacity units (Btuh/F)
	DBL /*TEMP*/  _t,		// approx temp (F) of air fan is blowing (needed to conv Btuh/F to cfm), when on
	DBL /*FRAC*/ _frOn /*=1.*/)	// fraction of time on: divide _c by this
// sets: FAN .cMx, t, c, p, q, dT, qAround.
// note: does NOT check for flow in excess of vfDs * vfMxF.
// returns temp rise across fan (=.dT), F
{
	// callers: AH::pute4Fs (5), AH::tsfo (2), FAN::reSetup, AH::puteRa. 7-1-95.

	DBL cDs = vfDs * Top.airX(_t);		// design flow in heat cap units @ current air temp

// compute max flow (cMx) in heat cap units @ _t, allowing for "overrun".  Used by caller.

	cMx = cDs * vfMxF;				// flow can exceed design flow by factor that defaults to 1.3.

// compute power consumption (p) and air heat (q, dT).  NB no check here for flow > cMx.

	c = _c;
	t = _t;
	frOn = _frOn;		// save inputs, for probing, eg for test purposes
	if (_frOn != 0.)				// don't divide by 0 (if 0, expect _c is also 0, no p, no q.)
		_c /= _frOn;				// adjust to fan-on flow in case power curve not linear (see *= frOn afterwards)
	float relFlow = cDs ? _c/cDs		// current fraction of design flow
					    : 0;			// if no des flow yet, eg autoSize start. shaftPwr and airPower will also be 0.
#ifdef AUTOSIZE
	/* In part of autoSize that uses open-ended models and increases capacities as needed,
	   use a linear fan curve (motor load proportional to flow) so dT is constant & thus independent of cMx & vfDs,
	   else dT change at vfDs increase might complicate air handler convergence. 7-1-95. */
	float relLoad;				// fraction of full fan load per curve
	if ( ausz					// if autoSizing this fan (implies AH::asFlow and AH::fanAs.active)
			&&  Top.tp_pass1A )			// if now using open-ended const-temp models to determine size
		relLoad = relFlow; 			// keep motor load proportional to flow (even over 1.0) to make dT constant.
	else
#endif
	if (relFlow >= 1.0)			// if more than design flow (CALLER limits flow to vfMxF * vfDs)
		relLoad = 1.0;			// assume pressure drops and power remains constant at full load value
	else						// else get relative input for current partial flow,
		relLoad = curvePy.val( relFlow);		// by evaluating user-input polynomial using its value member function.

	relLoad *= _frOn;				// adjust p's and q's for fraction of time fan running (note _c /=_frOn above)
	float inPower = shaftPwr / motEff;		// design flow input power
	p = inPower * relLoad;			// input power, subhr average -- to charge to meter
	q = airPower * relLoad;			// power that heats air being blown by fan, subhr average. used?
	if (_c)						// if 0 flow, don't divide by 0. prior value ok: useful for est when flow resumes.
		dT = q/c;				// fan-on temp rise in blown air due to q.  note c not _c.  qOn/cOn===qAvg/cAvg.
	qAround = (inPower - airPower) * relLoad;	// power to air around motor if not in flow, eg for supply motor in return air
	return dT;
}		// FAN::fn_pute
//-----------------------------------------------------------------------------------------------------------------------------

// end of cnfan.cpp
