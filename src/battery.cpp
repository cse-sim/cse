// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#include "CNGLOB.H"

#include "ANCREC.H"
#include "rccn.h"
#include "EXMAN.H"

#include "CNGUTS.H"

static const float kW_to_btuh = 3412.142f;

// Note: a more robust life model will need not only cycle counts but
// cycles by depth of discharge to capture "shallow cycling" vs
// "deep cycling".
// A further enhancement is to capture "time at temperature".
// We may want to look into the battery literature and also more into
// this application to better understand what kind of life model may
// be a good fit in terms of information requirements vs fidelity.

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
float BATTERY::bt_CalcAdjLoad(		// default load for current hour
	const MTR& m) const	// source meter
// returns sum of all end uses (including PV), kW
{
	return float( m.H.mtr_NetBldgLoad() / 3412.142);
}		// bt_CalcAdjLoad
//-----------------------------------------------------------------------------
RC BATTERY::bt_DoHour(
	int stage)		// 0: pre-calc
					// 1: calcs
					// 2: after reports
{
	
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
		ULI controlAlg = CHN(bt_controlAlg);	// choicn stored as nan float
		if (controlAlg == C_BATCTRLALGVC_TDVPEAKSAVE)
		{
			if (!Wthr.d.wd_HasTdvData())
			{
				rer(ABT, "BATTERY '%s': No TDV values available for bt_ControlAlg=TDVPeakSave.\n"
					"    Use Top.tdvFName to specify TDV data file.", name);
				controlAlg = 0;
			}
		}
		float P_bt_req =
			controlAlg == C_BATCTRLALGVC_TDVPEAKSAVE ? bt_ChgReqTDVPeakSave()
		  : IsSet( BATTERY_CHGREQ)	                 ? bt_chgReq
		  :                                            -bt_loadSeen;
		
		// battery maximum (charge) power based on capacity [kW]
		float P_bt_max_cap = (bt_maxCap * (1.f - bt_soe)) / (dt * bt_chgEff);
		if (P_bt_max_cap < tolerance)
			P_bt_max_cap = 0.f;

		// battery minimum (discharge) power based on capacity [kW]
		float P_bt_min_cap = -(bt_maxCap * bt_soe * bt_dschgEff) / dt;
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
//-----------------------------------------------------------------------------
float BATTERY::bt_ChgReqTDVPeakSave()
// Implements the California TDVPeakSave battery control algorithm
// Do not call if TDV data not present (per !WDHR.wd_HasTdvData())
// Assumes 1 hour time step
// returns current hour charge request
{
	int iHr1 = Top.iHr + 1;		// 1-based hour of day

	// available PV (0 if no meter specified)
	float pv = bt_meter
		? MtrB.p[bt_meter].H.pv / kW_to_btuh
		: 0.f;

	// non-peak hour default request: charge from PV
	float chgReq = -pv;

	// available energy not committed to future peak hours
	float uncommitted_energy = bt_energy * bt_dschgEff;

	// max number of peak hours to check
	//   = time to discharge full capacity
	int nRMax = int(bt_maxCap * bt_dschgEff / bt_maxDschgPwr) + 1;

	// check hours in TDV rank order
	for (int iR = 1; iR<=nRMax; iR++)
	{	int iHrRank = Wthr.d.wd_tdvElecHrRank[iR];	// 1-based hour of this rank
		if (iHrRank == iHr1)
		{	chgReq = -uncommitted_energy;	// current hour: uncommitted is available
											//   will be limited by bt_maxDschgPwr
			break;
		}
		else if (iHrRank > iHr1)	// if peak hour not passed
		{	uncommitted_energy -= bt_maxDschgPwr;	// save enough for it
			if (uncommitted_energy <= 0.f)
				break;				// all committed, stop
		}
	}

	return chgReq;

}		// BATTERY::bt_ChgReqTDVPeakSave
//==============================================================================



// battery.cpp end
