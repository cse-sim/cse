// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#include "cnglob.h"

#if 1
#include "ancrec.h"
#include "rccn.h"

#include "cnguts.h"

RC BATTERY::bt_CkF()
{
	RC rc = RCOK;
	return rc;
}	// BATTERY::bt_CkF

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
	return float( m.H.mtr_NetBldgLoad()) / 3412.142;
		float btuh_to_kW(1.0/3412.142);
}		// bt_CalcAdjLoad
//-----------------------------------------------------------------------------
RC BATTERY::bt_DoHour(
	int stage)		// 0: pre-calc
					// 1: calcs
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
		return rc;
	}

	// stage 1: after EVENDIVL expression eval
	//          results have *p (EVPSTIVL) stage (available in reports)

	float tolerance(bt_maxCap * 1e-6); // tolerance for capacity calculations [kW] -- used to prevent underflow issues

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

	return rc;

}	// BATTERY::bt_DoHour


#else
#include "cnglob.h"
#include "ancrec.h"
#include "rccn.h"

#include "cnguts.h"
#include <cmath>

RC BATTERY::bt_CkF()
{
	RC rc = RCOK;
	return rc;
}	// BATTERY::bt_CkF

RC BATTERY::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup(pSrc, options);
	return rc;
}	// BATTERY::RunDup

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

// MTR -> float
// Given a meter (MTR) object, calculate all loads and return as power in kW
float BATTERY::bt_CalcAdjLoad(MTR m)
{
	float btuh_to_kW(1.0/3412.142);
	return (m.H.clg
		+ m.H.htg
		+ m.H.hp
		+ m.H.dhw
		+ m.H.dhwBU
		+ m.H.fanC
		+ m.H.fanH
		+ m.H.fanV
		+ m.H.fan
		+ m.H.aux
		+ m.H.proc
		+ m.H.lit
		+ m.H.rcp
		+ m.H.ext
		+ m.H.refr
		+ m.H.dish
		+ m.H.dry
		+ m.H.wash
		+ m.H.cook
		+ m.H.usr1
		+ m.H.usr2
		+ m.H.pv) * btuh_to_kW;
}

RC BATTERY::bt_DoHour()
{
	RC rc = RCOK;
	float P_load_adj(0.0); // building + PV load, power [kW]
	float P_bt_req(0.0); // battery requested discharge power [kW]
	float P_bt_max_cap(0.0); // battery maximum (charge) power based on capacity [kW]
	float P_bt_min_cap(0.0); // battery minimum (discharge) power based on capacity [kW]
	float P_bt(0.0); // battery discharge power [kW]
	float dE_bt_app(0.0); // battery apparent power change [kWh]
	float dE_bt(0.0); // actual battery discharged energy [kWh]
	float eff(1.0); // efficiency of the battery [fraction]
	float dt(1.0); // hours in this timestep [hr]
	float dsoe(0.0); // change in SOE
	float kW_to_btuh(3412.142);
	float tolerance(bt_maxCap * 1e-6); // tolerance for capacity calculations [kW] -- used to prevent underflow issues
	bt_loadSeen = 0.0;
	if (bt_meter) {
		P_load_adj = bt_CalcAdjLoad(MtrB.p[bt_meter]);
		bt_loadSeen = P_load_adj;
	}
	P_bt_req = (bt_useUsrChg == C_NOYESCH_NO) ? -1.0 * P_load_adj : bt_chgReq;
	P_bt_max_cap = (bt_maxCap * (1.0 - bt_soe)) / (dt * bt_chgEff);
	if (P_bt_max_cap < tolerance)
		P_bt_max_cap = 0.0;
	P_bt_min_cap = -1.0 * (bt_maxCap * bt_soe * bt_dschgEff) / dt;
	if (P_bt_min_cap > (-1.0 * tolerance))
		P_bt_min_cap = 0.0;
	P_bt = bracket<float>(max(P_bt_min_cap, -1.0 * bt_maxDschgPwr), P_bt_req, min(P_bt_max_cap, bt_maxChgPwr));
	dE_bt_app = P_bt * dt;
	eff = (dE_bt_app >= 0.0) ? bt_chgEff : (1.0 / bt_dschgEff);
	dE_bt = dE_bt_app * eff;
	bt_energy = bt_energy + dE_bt;
	dsoe = dE_bt / bt_maxCap;
	bt_soe = bt_soe + dsoe;
	// soeBegIvl is used to provide probe access to SOE for the beginning of the next interval
	// during expression evaluation
	bt_soeBegIvl = bt_soe;
	// we devide by two because one full discharge and one full charge = one cycle
	bt_cycles = bt_cycles + (std::abs(dsoe) / 2.0);
	if (bt_meter)
		MtrB.p[bt_meter].H.mtr_Accum(bt_endUse, P_bt * kW_to_btuh);
	return rc;
}	// BATTERY::bt_DoHour()

#endif

// battery.cpp end
