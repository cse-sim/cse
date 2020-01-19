// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
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
#include "SLPAK.H"
#include "solarflatplatecollector.h"

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

RC DHWSOLARSYS::sw_Init() {
	RC rc = RCOK;

	sw_pMtrElec = MtrB.GetAtSafe(sw_elecMtri);

	sw_tankT = 60.f;  // F initial guess
	sw_tankTInlet = sw_tankT;
	sw_tankTOutlet = sw_tankT;
	sw_tInlet = sw_tankT;
	sw_tOutlet = sw_tankT;

	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;

	DHWSYS* pWS;
	DHWHEATER* pWH;
	RLUPC(WsR, pWS, pWS->ws_pDHWSOLARSYS)
	{
		if (pWS->ws_pDHWSOLARSYS->ss == ss)
		{
			sw_lastWS = pWS->ss;
			RLUPC(WhR, pWH, pWH->ownTi == pWS->ss)
			{
				sw_lastWH = pWH->ss;
			}
		}
	}

	return rc;
}

RC DHWSOLARSYS::sw_DoHour()
{
	RC rc = RCOK;

	sw_tankQGain = 0.f; // restart accumulator for new hour

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{
		rc |= pSC->sc_DoHour();
	}
	return rc;
}

RC DHWSOLARSYS::sw_DoHourEnd()
{
	RC rc = RCOK;

	// Add parasitics to meter
	if (sw_pMtrElec)
	{
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec);
	}

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{
		rc |= pSC->sc_DoHourEnd();
	}
	return rc;
}

FLOAT DHWSOLARSYS::sw_GetAvailableTemp()
{
	return sw_tankTOutlet;
}


RC DHWSOLARSYS::sw_DoSubhrTick(
	double vol,  // Volume of draw, gal
	float tInlet,  // inlet temperature (after DWHR), F
	TI ws_ss,  // subscript of calling DHWSYS
	TI wh_ss,  // subscript of calling DHWHEATER
	float& tOutlet)  // calculated outlet temperature, F
{
	RC rc = RCOK;

	sw_tickVol += vol;
	sw_tickVolT += vol * tInlet;

	if (vol > 0.f)
	{
		DHWSYS* pWS = WsR.GetAtSafe(ws_ss);
		pWS->ws_SSFAnnualSolar += vol * (sw_tankTOutlet - sw_tankTInlet);
		pWS->ws_SSFAnnualReq += vol * (pWS->ws_tUse - sw_tankTInlet);
	}

	// if not the last DHWSYS and DHWHEATER calling, just aggregate
	if (ws_ss != sw_lastWS || wh_ss != sw_lastWH) {
		return rc;
	}

	// Calculate outlet temperature of all collectors
	// Using volume weighted average
	DHWSOLARCOLLECTOR* pSC;

	float sumVolT = 0.f;
	float sumVol = 0.f;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{
		rc |= pSC->sc_DoSubhrTick();
		sumVolT += pSC->sc_tickVol*pSC->sc_tOutlet;
		sumVol += pSC->sc_tickVol;
	}

	if (sumVol > 0.f) {
		sw_tOutlet = sumVolT / sumVol;
		float heat_gain = sw_tankHXEff * sumVol*sw_fluidVolSpHt*(sw_tOutlet - sw_tankT); // Btu
		sw_tankQGain += heat_gain;
		sw_tankT = sw_tankT + heat_gain / (sw_tankVol*waterRhoCp);
		sw_tInlet = sw_tOutlet - heat_gain / (sumVol*sw_fluidVolSpHt);
	}
	else
	{
		sw_tOutlet = sw_tankT;
		sw_tInlet = sw_tankT;
	}

	// Calculate tank changes
	if (sw_tickVolT > 0)
	{
		sw_tankTInlet = sw_tickVolT / sw_tickVol;
	}


	// First order tank approximation:
	if (sw_tickVol > sw_tankVol)
	{
		// Entire tank contents (plus some)
		sw_tankT = (sw_tankT * sw_tankVol + sw_tankTInlet * (sw_tickVol - sw_tankVol))/ sw_tickVol;
		sw_tankTOutlet = sw_tankT;
	}
	else
	{
		// skimming off the top
		sw_tankTOutlet = sw_tankT;
		// then mix for next tick
		sw_tankT = (sw_tankT * (sw_tankVol - sw_tickVol) + sw_tankTInlet * sw_tickVol)/ sw_tankVol;
	}

	tOutlet = sw_tankTOutlet;
	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;

	return rc;
}

DHWSOLARCOLLECTOR::~DHWSOLARCOLLECTOR() {
	delete sc_collector;
	sc_collector = NULL;
}

RC DHWSOLARCOLLECTOR::sc_CkF() {
	RC rc = RCOK;
	if (!IsSet(DHWSOLARCOLLECTOR_PUMPFLOW))
	{
		sc_pumpFlow = 0.04*sc_area*sc_mult;  // initial rule of thumb: 0.04 gpm per ft2
	}
	if (!IsSet(DHWSOLARCOLLECTOR_PUMPPWR))
	{
		sc_pumpPwr = 34.1f*sc_pumpFlow;  // roughly 10 W / gpm -- don't know how good this assumption is
	}

	return rc;
}

RC DHWSOLARCOLLECTOR::sc_Init() {
	RC rc = RCOK;
	
	DHWSOLARSYS* pSS = SwhR.GetAtSafe(ownTi);

	sc_collector = new SolarFlatPlateCollector(AIPtoSI(sc_area*sc_mult),
		                                         sc_tilt, 
		                                         sc_azm, 
		                                         sc_FRTA, 
		                                         UIPtoSI(sc_FRUL),
		                                         SHIPtoSI(pSS->sw_fluidVolSpHt)/DSItoIP(1000.0) * 7.48,  // Btu/gal-F * J/kg-K * lb-F/Btu * m3 / kg * ft3 / lb * kg / m3 * gal / ft3 = J/kg-K
		                                         1000.0);  // Split doesn't really matter

	sc_tickOp = false;

	return rc;
}

RC DHWSOLARCOLLECTOR::sc_DoHour() {
	RC rc = RCOK;
	sc_pumpInElec = 0.f;
	sc_Qpoa = sc_Qfluid = sc_eff = 0.0;
	return rc;
}

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


RC DHWSOLARCOLLECTOR::sc_DoSubhrTick() {
	RC rc = RCOK;

	DHWSOLARSYS* pSWHSys = SwhR.GetAtSafe(ownTi);

	float tInlet = pSWHSys->sw_tInlet;

	float pump_energy = sc_pumpPwr * Top.tp_tickDurHr;  // Btuh * hr
	float pump_vol = sc_pumpFlow * Top.tp_tickDurMin;  // gal
	float pump_dt = pump_energy / (pump_vol * pSWHSys->sw_fluidVolSpHt);  // delta F

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

FLOAT DHWSOLARCOLLECTOR::sc_CalculateOutletTemp(float pump_dt) {
	// Calculate incidence
	double plane_incidence {0.0f};
	slPerezSkyModel(sc_tilt, sc_azm, Top.iHrST, Top.radBeamHrAv, Top.radDiffHrAv, Top.grndRefl, plane_incidence);
	sc_poa = static_cast<FLOAT>(plane_incidence);

	sc_Qpoa += sc_poa * sc_area *sc_mult*Top.tp_tickDurHr;  // Btu/h-ft2 * ft2 * h = Btu

	// Collector inlet temperature (from tank heat exchanger)
	double tInlet = SwhR.GetAtSafe(ownTi)->sw_tInlet + pump_dt;
	
	// Mass flow rate
	static const double gpmTom3ps = 0.00006309;
	static const double m3tokg = 1000;
	double m_dot_SI = sc_pumpFlow * gpmTom3ps* m3tokg;

	// Calculate outlet temperature
	sc_collector->calculate(IrIPtoSI(plane_incidence), m_dot_SI, DegFtoK(Top.tDbOHrAv), DegFtoK(tInlet));

	return static_cast<FLOAT>(DegKtoF(sc_collector->outlet_temp()));
}