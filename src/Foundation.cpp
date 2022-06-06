// Copyright (c) 1997-2018 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// Foundation.cpp -- interface to Kiva
///////////////////////////////////////////////////////////////////////////////

#include "CNGLOB.H"
#include "ANCREC.H"
#include "rccn.h"

#include "CNGUTS.H"

#include <IRATS.H>
#include "Foundation.h"

RC KIVA::kv_Create()
{
	if (!kv_instance->bcs)
	{
		kv_instance->bcs = std::make_shared<Kiva::BoundaryConditions>();
		if (!kv_instance->bcs)
		{
			return RCBAD; // oer?
		}
	}

	kv_instance->create();
	return RCOK;
}

KIVA::~KIVA()
{
	delete kv_instance;
	kv_instance = NULL;
}

RC KIVA::kv_RddInit()
{
	// Initialize ground temperatures
	int numAccTimeSteps = 3;
	int accTimeStep = 30;	// days
	DOY accDate = Top.tp_begDay - Top.wuDays - accTimeStep*(numAccTimeSteps + 1) - 1; // date of initial conditions (last time step from the day before)

	Wfile.wf_FixJday(accDate, Top.tp_begDay);
	
	// Initialize with steady state before accelerated timestepping
	std::shared_ptr<Kiva::Foundation> fnd = kv_instance->foundation;

	fnd->numericalScheme = Kiva::Foundation::NS_STEADY_STATE;
	kv_SetInitBCs(accDate);
	kv_instance->calculate();
	accDate += accTimeStep;
	accDate %= 365;		// adjust to 0-364 if >= 365

	// Accelerated timestepping
	for (int i = 0; i < numAccTimeSteps; i++)
	{
		fnd->numericalScheme = Kiva::Foundation::NS_IMPLICIT;
		kv_SetInitBCs(accDate);
		kv_instance->calculate(accTimeStep * 24 * 60 * 60);
		accDate += accTimeStep;
		accDate %= 365;		// adjust to 0-364 if >= 365
	}

	// Calculate averages so they can be used by CSE surfaces
	kv_instance->calculate_surface_averages();

	// Set numerical scheme appropriate for design days vs. main sim
	if (Top.tp_autoSizing) {
		fnd->numericalScheme = Kiva::Foundation::NS_STEADY_STATE;
	}
	else {
		fnd->numericalScheme = Kiva::Foundation::NS_ADI;
	}

	return RCOK;

}

RC KIVA::kv_SetInitBCs(DOY jDay)
{
	// Read weather file into temp weather data
	WDHR iW;

	// Get weather data for last hour on day 
	// should really be last subhour, but it's a good enough approx for Kiva initialization.
	Wfile.wf_Read(&iW, jDay, 23, WRN);

	std::shared_ptr<Kiva::BoundaryConditions> kv_bcs = kv_instance->bcs;

	kv_bcs->outdoorTemp = DegFtoK(iW.wd_db);
	kv_bcs->localWindSpeed = VIPtoSI(iW.wd_wndSpd)*Top.tp_WindFactor(kv_instance->foundation->grade.roughness,0,Top.tp_terrainClass);
	kv_bcs->skyEmissivity = pow4(DegFtoK(iW.wd_tSky)/ DegFtoK(iW.wd_db));
	kv_bcs->solarAzimuth = 3.14;
	kv_bcs->solarAltitude = 0.0;
	kv_bcs->directNormalFlux = 0.0;
	kv_bcs->diffuseHorizontalFlux = 0.0;
	kv_bcs->slabAbsRadiation = 0.0;
	kv_bcs->wallAbsRadiation = 0.0;
	kv_bcs->deepGroundTemperature = DegFtoK(Top.tp_deepGrndT);

	// Asssumed indoor temperature during warm-up
	// Could use zone setpoints? how to evaluate expressions outside of main simulation?
	kv_bcs->slabConvectiveTemp = kv_bcs->wallConvectiveTemp = kv_bcs->slabRadiantTemp = kv_bcs->wallRadiantTemp = DegFtoK(70.0);

	return RCOK;
}

RC KIVA::kv_SetBCs() 
{
	ZNR* z = kv_GetZone();
	std::shared_ptr<Kiva::BoundaryConditions> kv_bcs = kv_instance->bcs;
	kv_bcs->slabConvectiveTemp = kv_bcs->wallConvectiveTemp = DegFtoK(z->tz);
	kv_bcs->slabRadiantTemp = kv_bcs->wallRadiantTemp = DegFtoK(z->zn_tr); // TODO should be Tr of all other surfaces (excluding this surface)
	kv_bcs->outdoorTemp = DegFtoK(Top.tDbOSh);
	kv_bcs->localWindSpeed = VIPtoSI(Top.windSpeedSh)*Top.tp_WindFactor(kv_instance->foundation->grade.roughness, 0, Top.tp_terrainClass); // TODO Set wind factor once?
	kv_bcs->windDirection = RAD(Top.windDirDegHr);
	kv_bcs->skyEmissivity = pow4(DegFtoK(Top.tSkySh) / DegFtoK(Top.tDbOSh));
	kv_bcs->solarAzimuth = Wthr.d.wd_slAzm; // TODO Set at Top? Make subhourly?
	kv_bcs->solarAltitude = Wthr.d.wd_slAlt; // TODO Set at Top? Make subhourly?
	kv_bcs->directNormalFlux = IrIPtoSI(Top.radBeamShAv);
	kv_bcs->diffuseHorizontalFlux = IrIPtoSI(Top.radDiffShAv);
	kv_bcs->deepGroundTemperature = DegFtoK(Top.tp_deepGrndT);

	auto flrSf = XsB.GetAt(kv_floor);
	kv_bcs->slabAbsRadiation = IrIPtoSI(flrSf->x.xs_sbcI.sb_qrAbs);

	// Calculate area weighted average for walls
	float Qtotal = 0.f;
	float Atotal = 0.f;
	for (auto &wlI : kv_walls)
	{
		auto wlSf = XsB.GetAt(wlI);

		Qtotal += wlSf->x.xs_sbcI.sb_qrAbs*wlSf->x.xs_area;
		Atotal += wlSf->x.xs_area;
	}

	if (Atotal > 0.f)
	{
		kv_bcs->wallAbsRadiation = IrIPtoSI(Qtotal/Atotal);
	}
	else
	{
		kv_bcs->wallAbsRadiation = 0.0;
	}

	return RCOK;
}

RC KIVA::kv_Step(float dur)
{
	kv_SetBCs();
	kv_instance->calculate(dur*3600.f);
	kv_instance->calculate_surface_averages();

	return RCOK;
}

ZNR* KIVA::kv_GetZone() const
{
	return ZrB.GetAt(XsB.GetAt(kv_floor)->x.xs_sbcI.sb_zi); // Zone at inside surface
}
RC XSRAT::xr_ApplyKivaResults()
{
	if (x.xs_IsKiva())
	{
		auto& sbc = x.xs_sbcI;

		// Aggregate results across all kiva instances representing this surface
		xr_kivaAggregator->calc_weighted_results();

		auto &r = xr_kivaAggregator->results;

		// Set aggreagate surface properties
		sbc.sb_tSrf = DegKtoF(r.Tconv);
		sbc.sb_hxa = USItoIP(r.hconv);
		sbc.sb_hxr = USItoIP(r.hrad);
		sbc.sb_qSrf = IrSItoIP(r.qtot);
	}

	return RCOK;
}

RC XSRAT::xr_KivaZoneAccum()
{
	if (x.xs_IsKiva())
	{
		auto& sbc = x.xs_sbcI;
		ZNR& z = ZrB[sbc.sb_zi];
		double A = x.xs_area;

		// Accummulate up to zone (no CX terms because heat balance is at massless surface node)
		z.zn_nAirSh += A*sbc.sb_hxa*sbc.sb_tSrf;
		z.zn_dAirSh += A*sbc.sb_hxa;
		z.zn_nRadSh += A*sbc.sb_hxr*sbc.sb_tSrf;
		z.zn_dRadSh += A*sbc.sb_hxr;

		// Accumulate mass values for energy balance report
		z.zn_hcAMsSh += A*sbc.sb_hxa;
		z.zn_hcATMsSh += A*sbc.sb_hxa*sbc.sb_tSrf;
		z.zn_hrAMsSh += A*sbc.sb_hxr;
		z.zn_hrATMsSh += A*sbc.sb_hxr*sbc.sb_tSrf;
	}

	return RCOK;
}