// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#include "cnglob.h"

#include "ancrec.h"
#include "rccn.h"

#include "cnguts.h"

//----------------------------------------------------------------------------
RC BATTERY::bt_CkF()
{
	RC rc = RCOK;
	return rc;
}	// BATTERY::bt_CkF
//----------------------------------------------------------------------------
RC BATTERY::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	return rc;
}	// BATTERY::RunDup
//----------------------------------------------------------------------------
RC BATTERY::bt_Init()
{
	RC rc = RCOK;
	bt_soe = bt_initSOE;
	bt_soeBegIvl = bt_initSOE;
	bt_cycles = bt_initCycles;
	bt_energy = bt_initSOE * bt_maxCap;
	bt_loadSeen = 0.0;
	return rc;
}	// BATTERY::bt_Init
//-----------------------------------------------------------------------------
float BATTERY::bt_CalcAdjLoad(		// default load
	const MTR& m) const	// source meter
// returns sum of all end uses, kW
{
	return float( m.H.mtr_NetBldgLoad() / 3412.142);
}		// bt_CalcAdjLoad
//-----------------------------------------------------------------------------
RC BATTERY::bt_DoHour(
	int stage)		// 0: pre-calc
					// 1: calcs
					// 2: after reports
{
	static const float kW_to_btuh = 3412.142f;
	static const float dt = 1.f;	// timestep duration, hr

	RC rc = RCOK;

	if (stage == 0)
	{	// stage 0: after meter accum, before EVENDIVL expression eval
		//          bt_loadSeen is thus available at EVENDIVL
		if (Top.tp_isBegMainSim && IsSet( BATTERY_INITSOE))
		// Ensure battery is reinitialized
		// after warmup and/or autosizing
		// ONLY IF bt_initSOE was set by user
			rc = bt_Init();

		// building + PV net from meter if available
		bt_loadSeen	= bt_meter ?  bt_CalcAdjLoad( MtrB.p[bt_meter]) : 0.f;
	}
	else if (stage == 1)
	{	// stage 1: after EVENDIVL expression eval
		//          results have *p (EVPSTIVL) stage (available in reports)

		float tolerance(bt_maxCap * 1e-6);	// tolerance for capacity calculations [kW]
											//  used to prevent underflow issues

		// battery requested discharge power[ kW]
		float P_bt_req 	= bt_useUsrChg == C_NOYESCH_YES
				? bt_chgReq			// user-specified discharge request
				: -bt_loadSeen;		// default control strategy

		// battery maximum (charge) power based on capacity [kW]
		float P_bt_max_cap = (bt_maxCap * (1.f - bt_soe)) / (dt * bt_chgEff);
		if (P_bt_max_cap < tolerance)
			P_bt_max_cap = 0.f;

		// battery minimum (discharge) power based on capacity [kW]
		float P_bt_min_cap = -1.0 * (bt_maxCap * bt_soe * bt_dschgEff) / dt;
		if (P_bt_min_cap > (-tolerance))
			P_bt_min_cap = 0.f;

		// battery discharge power [kW]
		float P_bt = bracket<float>( max( P_bt_min_cap, -bt_maxDschgPwr), P_bt_req, min( P_bt_max_cap, bt_maxChgPwr));

		// battery apparent power change [kWh]
		float dE_bt_app = P_bt * dt;

		// efficiency of battery
		float eff = dE_bt_app >= 0.f ? bt_chgEff : (1.f / bt_dschgEff);

		// actual battery discharged energy [kWh]
		// update energy
		float dE_bt = dE_bt_app * eff;
		float energyPrior = bt_energy;
		bt_energy = bracket(0.f, bt_energy + dE_bt, bt_maxCap);
		dE_bt = bt_energy - energyPrior;	// force consistency in case bracket() changed bt_energy

		// state of energy
		bt_soe = bt_energy / bt_maxCap;

		// soeBegIvl is used to provide probe access to SOE for the beginning of the next interval
		// during expression evaluation
		bt_soeBegIvl = bt_soe;

		// we divide by 2 because one full discharge and one full charge = one cycle
		bt_cycles += fabs( dE_bt / bt_maxCap) / 2.f;
		if (bt_meter)
			MtrB.p[bt_meter].H.mtr_Accum(bt_endUse, P_bt * kW_to_btuh);
	}
	else
	{	// stage == 2 (after reports): copy current hour results to xxxlh
		//    re availability for probing in e.g. next hour control expressions
		bt_soelh = bt_soe;
		bt_energylh = bt_energy;
		bt_cycleslh = bt_cycles;
	}
	return rc;
}	// BATTERY::bt_DoHour

// battery.cpp end
