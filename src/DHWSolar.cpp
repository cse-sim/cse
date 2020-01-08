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
	return rc;
}

RC DHWSOLARSYS::sw_Init() {
	RC rc = RCOK;

	sw_pMtrElec = MtrB.GetAtSafe(sw_elecMtri);

	return rc;
}

RC DHWSOLARSYS::sw_DoHour() {
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
		rc |= pSC->sc_DoHour();
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

	// First order approximation:
	// - Tank has zero volume
	// - Tank water leaves at collector outlet temperature (i.e., 100% effective transfer)
	sw_tankTOutlet = sw_tOutlet;

	// Add parasitics to meter
	if (sw_pMtrElec)
	{
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec);
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

	float tInlet = pSWHSys->sw_tankTInlet; // TODO: Calculate using real tank

	// Calculate potential outlet temperature
	float potentialToutlet;

	potentialToutlet = 125.f;  // Change this to calculate for the specific collector

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

	// Add pump energy to meter
	if (pSWHSys->sw_pMtrElec)
	{
		pSWHSys->sw_pMtrElec->H.mtr_Accum(pSWHSys->sw_endUse, sc_pumpInElec);
	}

	return rc;
}
