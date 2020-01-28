// Copyright (c) 1997-2020 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// DHWSolar.cpp -- Solar Hot Water model implementation
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"

#include "cse.h"
#include "ancrec.h" 	// record: base class for rccn.h classes
#include "rccn.h"
#include "irats.h"
#include "cnguts.h"
#include "cuparse.h"
#include "SLPAK.H"
#include "solarflatplatecollector.h"

///////////////////////////////////////////////////////////////////////////////
// DHWSOLARSYS: represents a solar water heating system
//              1 or more collectors + tank + pump
///////////////////////////////////////////////////////////////////////////////
/*virtual*/ void DHWSOLARSYS::ReceiveRuntimeMessage(const char* msg)
{
	pInfo("%s: HPWH message (%s):\n  %s",
		objIdTx(), Top.When(C_IVLCH_S), msg);
}		// DHWSOLARSYS::ReceiveRuntimeMessage
//------------------------------------------------------------------------------

RC DHWSOLARSYS::sw_CkF() {
	RC rc = RCOK;

	// tank surrounding zone or temp: zone illegal if temp given
	if (IsSet(DHWSOLARSYS_TANKTEX))
		rc |= disallowN("when 'swTankTEx' is specified", DHWSOLARSYS_TANKZNTI, 0);

	if (!rc)
	{
		if (!IsSet(DHWSOLARSYS_TANKUA))
		{
			float tsa = DHWTANK::TankSurfArea_CEC(sw_tankVol);
			if (sw_tankInsulR < 0.01f)
			{
				rc |= ooer(DHWSOLARSYS_TANKINSULR, "swTankInsulR must be >= 0.01 so valid wtUA can be calculated");
				sw_tankInsulR = 0.01;
			}
			sw_tankUA = tsa / sw_tankInsulR;
		}
	}
	return rc;
}
//---------------------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_Init()
{
	RC rc = RCOK;

	sw_pMtrElec = MtrB.GetAtSafe(sw_elecMtri);

	// DHWSOLARCOLLECTOR
	sw_scCount = 0;
	sw_scAreaTot = 0.f;
	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ss == ss)
	{	rc |= pSC->sc_Init();
		sw_scCount++;
		sw_scAreaTot += pSC->sc_areaTot;
	}
	if (!IsSet(DHWSOLARSYS_TANKVOL))
		sw_tankVol = 1.5f * sw_scAreaTot;		// default 1.5 gal/ft2

	sw_tankHxT = 60.f;  // F initial guess
	sw_tankTInlet = sw_tankHxT;
	sw_tankTOutlet = sw_tankHxT;
	sw_scTInlet = sw_tankHxT;
	sw_scTOutlet = sw_tankHxT;

	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;

	// tank setup
	//   use HPWH multi-node tank model w/o heater
	sw_tankpZn = ZrB.GetAtSafe(sw_tankZnTi);
	rc |= sw_tank.hw_Init( this);
	rc |= sw_tank.hw_InitTank( sw_tankVol, sw_tankUA);
	// finalize tank, -1 = don't set inlet height
	rc |= sw_tank.hw_InitFinalize( -1.f, -1.f);
	sw_tank.hw_SetNQTXNodes( 3);	// use bottom 3 nodes
									//   solar collector HX

	return rc;

}		// DHWSOLARSYS::sw_Init
//--------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoHour()
{
	RC rc = RCOK;

	sw_tankQGain = 0.f; // restart accumulator for new hour
	sw_tankQLoss = 0.f;
	sw_drawVol = 0.f;

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{
		rc |= pSC->sc_DoHour();
	}
	return rc;
}
//-----------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoSubhrStart(
	int iTk0)
{
	RC rc = RCOK;

	// tank ambient temp
	//   set temp from linked zone (else leave expression/default value)
	if (sw_tankpZn)
		sw_tankTEx = sw_tankpZn->tzls;

	rc |= sw_tank.hw_DoSubhrStart(sw_tankTEx);

	return rc;
}		// DHWSOLARSYS::sw_DoSubhrStart
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoSubhrEnd()
{
	RC rc = RCOK;

	double totOut = 0.;		//
	sw_tank.hw_DoSubhrEnd(1.f, sw_tankpZn, NULL, totOut);
	sw_tankQLoss += sw_tank.hw_qLoss;
#if 0
	wh_qEnv = wh_HPWH.hw_qEnv;
	wh_balErrCount = wh_HPWH.hw_balErrCount;
	if (pWS->ws_tUse - wh_HPWH.hw_tHWOut > 1.f)
		wh_unMetSh++;	// unexpected, XBU should maintain temp
						//   will happen if ws_tUse changes (e.g. via expression)
						//   during a period of no draw			

	// energy use accounting, electricity only (assume no fuel)
	wh_HPWHxBU = wh_HPWH.hw_HPWHxBU;

	wh_inElecSh = wh_HPWH.hw_inElec[1] * BtuperkWh + wh_parElec * BtuperWh*Top.subhrDur;
	wh_inElecBUSh = wh_HPWH.hw_inElec[0] * BtuperkWh;
	wh_inElecXBUSh = wh_HPWHxBU;
#endif

	return rc;

}		// DHWSOLARSYS::sw_DoSubhrEnd
//-----------------------------------------------------------------------------
RC DHWSOLARSYS::sw_EndIvl(
	IVLCH ivl)	// C_IVLCH_Y, _M, _D, _H (do not call for _S)
{
	RC rc = RCOK;

	// Add parasitics to meter
	if (sw_pMtrElec)
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec * BtuperWh);

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
		rc |= pSC->sc_DoHourEnd();

	if (ivl == C_IVLCH_Y)
	{
		double SSFNum = 0.;
		double SSFDen = 0.;

		DHWSYS* pWS;
		RLUPC(WsR, pWS, pWS->ws_pDHWSOLARSYS == this)
		{
			SSFNum += pWS->ws_SSFAnnualSolar;
			SSFDen += pWS->ws_SSFAnnualReq;
		}
		sw_SSFAnnual = SSFDen > 0. ? float( SSFNum / SSFDen) : 0.f;
				
	}
	return rc;
}
//-----------------------------------------------------------------------------
FLOAT DHWSOLARSYS::sw_GetAvailableTemp()
{
	return sw_tankTOutlet;
}
//-----------------------------------------------------------------------------
void DHWSOLARSYS::sw_TickStart()
{
	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;
}
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_TickAccumDraw(			// accumulate draw for current tick
	DHWSYS* pWS,	// source DHWSYS
	float vol,		// volume, gal
	float tInlet)	// solar tank inlet temp from source DHWSYS, F
					//   after e.g. DWHR
{
	RC rc = RCOK;
	if (vol > 0.f)
	{	sw_tickVol += vol;
		sw_tickVolT += vol * tInlet;
		if (pWS)
		{	pWS->ws_SSFAnnualSolar += vol * (sw_tankTOutlet - tInlet);
			pWS->ws_SSFAnnualReq += vol * (pWS->ws_tUse - tInlet);
		}
	}

	return rc;
}	// DHWSOLARSYS::sw_TickAccumDraw
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_TickCalc()
{
	RC rc = RCOK;

	// Calculate outlet temperature of all collectors
	// Using volume weighted average
	DHWSOLARCOLLECTOR* pSC;

	float sumVolT = 0.f;
	float sumVol = 0.f;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{	rc |= pSC->sc_DoSubhrTick();
		sumVolT += pSC->sc_tickVol*pSC->sc_tOutlet;
		sumVol += pSC->sc_tickVol;
	}

	// tank heat exchange temp
	sw_tankHxT = sw_tank.hw_GetTankQTXTemp();

	float scQGain = 0.f;		// collector heat to be added to tank, Btu
	if (sumVol > 0.f)
	{	// collector flow > 0
		sw_scTOutlet = sumVolT / sumVol;
		scQGain = sw_tankHXEff * sumVol*sw_scFluidVolSpHt*(sw_scTOutlet - sw_tankHxT); // Btu
		sw_tankQGain += scQGain;
		sw_scTInlet = sw_scTOutlet - scQGain / (sumVol*sw_scFluidVolSpHt);
	}
	else
	{
		sw_scTInlet = sw_tankHxT;		// no flow
		sw_scTOutlet = sw_scTInlet;
	}

	sw_tank.hw_SetQTX( scQGain);	// pass gain to tank model

	if (sw_tickVol > 0.f)
	{
		sw_tankTInlet = sw_tickVolT / sw_tickVol;
		sw_drawVol += sw_tickVol;
	}
	
	float tOut;
	rc |= sw_tank.hw_DoSubhrTick( sw_tickVol, sw_tankTInlet, &tOut);

	if (tOut > 0.f)
		sw_tankTOutlet = tOut;

	return rc;
}
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWSOLARCOLLECTOR: represents a solar collector
//                    child of DHWSOLARSYS
///////////////////////////////////////////////////////////////////////////////
DHWSOLARCOLLECTOR::~DHWSOLARCOLLECTOR() {
	delete sc_collector;
	sc_collector = NULL;
}
//-----------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_CkF()
{
	RC rc = RCOK;
	
	// defaults are derived in sc_Init()
	// later derivation better supports input expressions with probes

	return rc;
}
//-----------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_Init()
{
	RC rc = RCOK;
	
	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	sc_areaTot = sc_area * sc_mult;

	if (!IsSet(DHWSOLARCOLLECTOR_PUMPFLOW))
		sc_pumpFlow = 0.03*sc_areaTot;  // initial rule of thumb: 0.03 gpm per ft2
	
	if (!IsSet(DHWSOLARCOLLECTOR_PUMPPWR))
		sc_pumpPwr = 10.f*sc_pumpFlow;  // don't know how good this assumption is

	delete sc_collector;		// insurance: delete prior, if any

	sc_collector = new SolarFlatPlateCollector(
		AIPtoSI( sc_areaTot),
		sc_tilt, 
		sc_azm, 
		sc_FRTA, 
		UIPtoSI(sc_FRUL),
		SHIPtoSI(pSW->sw_scFluidVolSpHt)/DSItoIP(1000.0) * 7.48,  // Btu/gal-F * J/kg-K * lb-F/Btu * m3 / kg * ft3 / lb * kg / m3 * gal / ft3 = J/kg-K
		1000.0);  // Split doesn't really matter

	sc_tickOp = false;

	return rc;
}
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHour()
{
	RC rc = RCOK;
	sc_pumpInElec = 0.f;
	sc_Qfluid = sc_eff = 0.0;

	// plane incidence: use hourly values
	double plane_incidence;
	rc |= slPerezSkyModel(sc_tilt, sc_azm, Top.iHrST, Top.radBeamHrAv, Top.radDiffHrAv, Top.grndRefl, plane_incidence);

	sc_poa = static_cast<FLOAT>(plane_incidence);
	sc_Qpoa = sc_poa * sc_areaTot;  // Btu/h-ft2 * ft2 = Btu

	return rc;
}		// DHWSOLARCOLLECTOR::sc_DoHour
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHourEnd() {
	RC rc = RCOK;

	sc_eff = sc_eff / Top.tp_NHrTicks();

	DHWSOLARSYS* pSWHSys = SwhR.GetAtSafe(ownTi);

	// Add pump energy to meter
	if (pSWHSys->sw_pMtrElec)
	{
		pSWHSys->sw_pMtrElec->H.mtr_Accum(pSWHSys->sw_endUse, sc_pumpInElec);
	}

	return rc;
}
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoSubhrTick() {
	RC rc = RCOK;

	DHWSOLARSYS* pSWHSys = SwhR.GetAtSafe(ownTi);

	float tInlet = pSWHSys->sw_scTInlet;

	float pump_energy = sc_pumpPwr * BtuperWh * Top.tp_tickDurHr;  // Btuh * hr
	float pump_vol = sc_pumpFlow * Top.tp_tickDurMin;  // gal
	float pump_dt = pump_energy / (pump_vol * pSWHSys->sw_scFluidVolSpHt);  // delta F

	// Calculate potential outlet temperature

	sc_tOutletP = sc_CalculateOutletTemp(pump_dt);

	// Collector status

	// Already running
	bool runningAboveShutOff = sc_tickOp && (sc_tOutletP > tInlet + sc_pumpOffDeltaT);
	bool offAboveTurnOn = !sc_tickOp && (sc_tOutletP > tInlet + sc_pumpOnDeltaT);

	if (runningAboveShutOff || offAboveTurnOn) {
		sc_tickVol = pump_vol;
		sc_pumpInElec += pump_energy;
		sc_tOutlet = sc_tOutletP;
		sc_tickOp = true;
		sc_eff += sc_collector->efficiency();
		sc_Qfluid += sc_collector->heat_gain()*BtuperWh*Top.tp_tickDurHr; // W * Btu/W-h * hr = Btu
	}
	else
	{
		sc_tickVol = 0.f;
		sc_tOutlet = tInlet;
		sc_tickOp = false;
	}

	return rc;
}
//------------------------------------------------------------------------------------------
FLOAT DHWSOLARCOLLECTOR::sc_CalculateOutletTemp(
	float pump_dt)	// pump temp rise, R
{
	// Collector inlet temperature (from tank heat exchanger)
	double tInlet = SwhR.GetAtSafe(ownTi)->sw_scTInlet + pump_dt;
	
	// Mass flow rate
	static const double gpmTom3ps = 0.00006309;
	static const double m3tokg = 1000;
	double m_dot_SI = sc_pumpFlow * gpmTom3ps* m3tokg;

	// Calculate outlet temperature
	sc_collector->calculate(IrIPtoSI( sc_poa), m_dot_SI, DegFtoK(Top.tDbOHrAv), DegFtoK(tInlet));

	return static_cast<FLOAT>(DegKtoF(sc_collector->outlet_temp()));
}

//=============================================================================