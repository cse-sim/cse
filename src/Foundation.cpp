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

#include <irats.h>
#include "foundation.h"

RC KIVA::kv_Create()
{
	// create output map for ground instance. Calculate average temperature, flux, and convection for each surface
	Kiva::GroundOutput::OutputMap outputMap;

	outputMap.push_back(Kiva::Surface::ST_SLAB_CORE);

	if (kv_fnd->hasPerimeterSurface) {
		outputMap.push_back(Kiva::Surface::ST_SLAB_PERIM);
	}

	if (kv_fnd->foundationDepth > 0.0) {
		outputMap.push_back(Kiva::Surface::ST_WALL_INT);
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

	if (!kv_bcs)
	{
		kv_bcs = new Kiva::BoundaryConditions;
		if (!kv_bcs)
		{
			return RCBAD; // oer?
		}
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

RC KIVA::kv_RddInit()
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
	kv_ground->foundation.slab.interior.emissivity = 0.0;
	kv_ground->foundation.wall.interior.emissivity = 0.0;


	return RCOK;

}

RC KIVA::kv_SetInitBCs(DOY jDay)
{
	// Read weather file into temp weather data
	WDHR iW;

	// Get weather data for last hour on day 
	// should really be last subhour, but it's a good enough approx for Kiva initialization.
	Wfile.wf_Read(jDay, 23, &iW, WRN);

	kv_bcs->outdoorTemp = DegFtoK(iW.wd_db);
	kv_bcs->localWindSpeed = VIPtoSI(iW.wd_wndSpd)*Top.tp_WindFactor(kv_fnd->grade.roughness,0,Top.tp_terrainClass);
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
	kv_bcs->indoorTemp = DegFtoK(70.0);
	kv_bcs->slabRadiantTemp = kv_bcs->wallRadiantTemp = kv_bcs->indoorTemp;

	return RCOK;
}

RC KIVA::kv_SetBCs() 
{
	ZNR* z = kv_GetZone();
	kv_bcs->indoorTemp = DegFtoK(z->tz);
	kv_bcs->slabRadiantTemp = kv_bcs->wallRadiantTemp = DegFtoK(z->tr); // TODO should be Tr of all other surfaces (excluding this surface)
	kv_bcs->outdoorTemp = DegFtoK(Top.tDbOSh);
	kv_bcs->localWindSpeed = VIPtoSI(Top.windSpeedSh)*Top.tp_WindFactor(kv_fnd->grade.roughness, 0, Top.tp_terrainClass); // TODO Set wind factor once?
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
	kv_ground->calculate(*kv_bcs, dur*3600.f);
	kv_ground->calculateSurfaceAverages();

	return RCOK;
}

ZNR* KIVA::kv_GetZone() const
{
	return ZrB.GetAt(XsB.GetAt(kv_floor)->x.xs_sbcI.sb_zi); // Zone at inside surface
}

int XSURF::xs_IsKiva() const
{
	return xs_ty == CTKIVA;
}

RC XSRAT::xr_ApplyKivaResults()
{
	if (x.xs_ty == CTKIVA)
	{
		Kiva::Surface::SurfaceType st;

		if (fAboutEqual(DEG(x.tilt), 180.f))
		{
			st = Kiva::Surface::ST_SLAB_CORE;
		}
		else
		{
			st = Kiva::Surface::ST_WALL_INT;
		}

		auto& sbc = x.xs_sbcI;

		// Set surface temp and convection coeff
		float hc = 0.0, hr = 0.0, q = 0.0;
		float Tz = DegFtoK(sbc.sb_txa);
		float Tr = DegFtoK(sbc.sb_txr);
		for (auto ki : xr_kivaInstances)
		{
			KIVA* k = KvR.GetAt(ki);
			if (!k->kv_ground)
			{
				oer("Invalid kiva instance!");
				return RCBAD;
			}

			float p = k->kv_perimWeight;
			float hci = k->kv_ground->getSurfaceAverageValue({ st, Kiva::GroundOutput::OT_CONV });
			float hri = k->kv_ground->getSurfaceAverageValue({ st, Kiva::GroundOutput::OT_RAD });
			float Ts = k->kv_ground->getSurfaceAverageValue({ st, Kiva::GroundOutput::OT_TEMP });

			if (!isfinite(Ts))
			{
				oer("Kiva is not giving realistic results!");
				return RCBAD;
			}

			hc += p*hci;
			hr += p*hri;
			q += hc*(Tz - Ts) + hr*(Tr - Ts);
		}

		// Set aggreagate surface properties
		sbc.sb_tSrf = DegKtoF(Tz - q/hc);
		sbc.sb_hxa = USItoIP(hc);
		sbc.sb_hxr = USItoIP(hr);
		sbc.sb_qSrf = IrSItoIP(q);

	}

	return RCOK;
}

RC XSRAT::xr_KivaZoneAccum()
{
	if (x.xs_ty == CTKIVA)
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