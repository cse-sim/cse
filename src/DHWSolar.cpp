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

	DHWSYS* pDHW;

	float sumT = 0.f;
	unsigned count = 0u;

	RLUPC(WsR, pDHW, pDHW->ws_swTi == ss)
	{
		sumT += pDHW->ws_tInletX;
		count += 1;
	}
	sw_tankT = sumT / count;

	return rc;
}

RC DHWSOLARSYS::sw_DoHour()
{
	RC rc = RCOK;

	// Add parasitics to meter
	if (sw_pMtrElec)
	{
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec);
	}


	return rc;
}

FLOAT DHWSOLARSYS::sw_GetAvailableTemp()
{
	return sw_tankTOutlet;
}


RC DHWSOLARSYS::sw_DoSubhrTick(
	double vol,  // Volume of draw, gal
	float tInlet)  // inlet temperature (after DWHR), F
{
	RC rc = RCOK;

	// Get inlet from all DHWSYS using this DHWSOLARSYS
	DHWSYS* pDHW;

	float sumMDotT = 0.f;
	float sumMDot = 0.f;

	RLUPC(WsR, pDHW, pDHW->ws_swTi == ss)
	{
		sumMDotT += pDHW->ws_tInletX;
		sumMDot += 1.0;  // TODO: Fix
	}
	sw_tankTInlet = sumMDotT / sumMDot;

	// Calculate outlet temperature of all collectors
	DHWSOLARCOLLECTOR* pSC;

	sumMDotT = 0.f;
	sumMDot = 0.f;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{
		rc |= pSC->sc_DoSubhrTick();
		sumMDotT += pSC->sc_mdot*pSC->sc_tOutlet;
		sumMDot += pSC->sc_mdot;
	}

	if (sumMDot > 0.f) {
		sw_tOutlet = sumMDotT / sumMDot;
	}
	else
	{
		sw_tOutlet = sw_tInlet;
	}

	// First order tank approximation:
	if (vol > sw_tankVol)
	{
		// Entire tank contents (plus some)
		sw_tankT = (sw_tankT * sw_tankVol + tInlet * (vol - sw_tankVol))/vol;
		sw_tankTOutlet = sw_tankT;
	}
	else
	{
		// skimming off the top
		sw_tankTOutlet = sw_tankT;
		// then mix for next tick
		sw_tankT = (sw_tankT * (sw_tankVol - vol) + tInlet * vol)/ sw_tankVol;
	}

	return rc;
}


RC DHWSOLARCOLLECTOR::sc_CkF() {
	RC rc = RCOK;
	return rc;
}

RC DHWSOLARCOLLECTOR::sc_Init() {
	RC rc = RCOK;
	
	// 

	return rc;
}

RC DHWSOLARCOLLECTOR::sc_DoHour() {
	RC rc = RCOK;

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

	float tInlet = pSWHSys->sw_tankT; // TODO: Calculate using real tank

	// Calculate potential outlet temperature
	float potentialToutlet;

	potentialToutlet = 90.f;  // Change this to calculate for the specific collector

	// If collector will heat water, run pump
	if (potentialToutlet > tInlet) {
		sc_mdot = 1.0;
		sc_pumpInElec = sc_pumpPwr; // TODO: *convert to energy
		// TODO: Add pump heat to fluid stream
		sc_tOutlet = potentialToutlet;
	}
	else
	{
		sc_mdot = 0.f;
		sc_pumpInElec = 0.f;
		sc_tOutlet = tInlet;
	}

	return rc;
}
