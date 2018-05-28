// Copyright (c) 1997-2018 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// Foundation.cpp -- interface to Kiva
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"
#include "ancrec.h"
#include "rccn.h"

#include "cnguts.h"

#include "foundation.h"

RC KIVA::kv_Create(
	Kiva::Foundation fnd,
	TI flri,
	std::vector<TI> wallIDs,
	double weight
)
{
	kv_floor = flri;
	kv_walls = wallIDs;
	kv_perimWeight = weight;

	// Clumsy copy
	if (!kv_fnd)
	{
		kv_fnd = new Kiva::Foundation();
		if (!kv_fnd)
			return RCBAD; // oer?
		*kv_fnd = fnd;
	}

	// create output map for ground instance. Calculate average temperature, flux, and convection for each surface
	Kiva::GroundOutput::OutputMap outputMap;

	outputMap[Kiva::Surface::ST_SLAB_CORE] = { Kiva::GroundOutput::OT_FLUX, Kiva::GroundOutput::OT_TEMP, Kiva::GroundOutput::OT_CONV };

	if (kv_fnd->hasPerimeterSurface) {
		outputMap[Kiva::Surface::ST_SLAB_PERIM] = { Kiva::GroundOutput::OT_FLUX, Kiva::GroundOutput::OT_TEMP,
			Kiva::GroundOutput::OT_CONV };
	}

	if (kv_fnd->foundationDepth > 0.0) {
		outputMap[Kiva::Surface::ST_WALL_INT] = { Kiva::GroundOutput::OT_FLUX, Kiva::GroundOutput::OT_TEMP,
			Kiva::GroundOutput::OT_CONV };
	}

	if (!kv_ground)
	{
		kv_ground = new Kiva::Ground(*kv_fnd, outputMap);
		if (!kv_ground)
		{
			return RCBAD; // oer?
		}
		kv_ground->buildDomain();
	}
	return RCOK;
}

KIVA::~KIVA()
{
	delete kv_fnd;
	kv_fnd = NULL;
	delete kv_ground;
	kv_ground = NULL;
	delete kv_bcs;
	kv_bcs = NULL;
}

RC KIVA::kv_InitGround()
{
	int numAccTimeSteps = 3;
	int accTimeStep = 30;	// days
	DOY accDate = Top.tp_begDay - Top.wuDays - accTimeStep*(numAccTimeSteps + 1) - 1; // date of initial conditions (last time step from the day before)

	Wfile.wf_FixJday(accDate, Top.tp_begDay);
	
	// Initialize with steady state before accelerated timestepping
	kv_ground->foundation.numericalScheme = Kiva::Foundation::NS_STEADY_STATE;
	kv_SetInitBCs(accDate);
	kv_ground->calculate(*kv_bcs);
	accDate += accTimeStep;
	accDate %= 365;		// adjust to 0-364 if >= 365

	// Accelerated timestepping
	for (int i = 0; i < numAccTimeSteps; i++)
	{
		kv_ground->foundation.numericalScheme = Kiva::Foundation::NS_STEADY_STATE;
		kv_SetInitBCs(accDate);
		kv_ground->calculate(*kv_bcs);
		accDate += accTimeStep;
		accDate %= 365;		// adjust to 0-364 if >= 365
	}

	// Calculate averages so they can be used by CSE surfaces
	kv_ground->calculateSurfaceAverages();

	// Reset numerical scheme
	kv_ground->foundation.numericalScheme = Kiva::Foundation::NS_ADI;

	// Reset emissivity to use CSE's IR model instead
	kv_ground->foundation.slab.emissivity = 0.0;
	kv_ground->foundation.wall.interiorEmissivity = 0.0;


	return RCOK;

}

RC KIVA::kv_SetInitBCs(DOY jDay)
{
	// Read weather file into temp weather data
	WDHR iW;

	// Get weather data for last hour on day (should really be last subhour, but it's a good enough approx for Kiva initialization.
	Wfile.wf_Read(jDay, 23, &iW, WRN);

	kv_bcs->outdoorTemp = DegFtoK(iW.wd_db);
	kv_bcs->localWindSpeed = VIPtoSI(iW.wd_wndSpd);
	kv_bcs->skyEmissivity = pow4(DegFtoK(iW.wd_tSky)/ DegFtoK(iW.wd_db));

	return RCOK;
}

RC KIVA::kv_SetBCs() 
{
	return RCOK;
}

RC KIVA::kv_Calculate()
{
	kv_SetBCs();
	kv_ground->calculate(*kv_bcs, 0.0); // TODO add timestep

	// Set sbc h and T for each surface

	// Accumulate to zone heat balance

	return RCOK;
}

int XSURF::xs_IsKiva() const
{
	return xs_ty == CTKIVA;
}