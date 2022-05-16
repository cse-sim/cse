// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// pvcalc.cpp -- interface to PVWATTS
///////////////////////////////////////////////////////////////////////////////

#include "CNGLOB.H"
#include "ANCREC.H" 	// record: base class for rccn.h classes
#include "TDPAK.H"

#include "rccn.h"
#include "CNGUTS.H"
#include "EXMAN.H"
#include "solar.h"


static const float airRefrInd = 1.f;
static const float glRefrInd = 1.526f;

//-----------------------------------------------------------------------------
PVARRAY::PVARRAY( basAnc *b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
{
	FixUp();
}	// PVARRAY::PVARRAY
//-----------------------------------------------------------------------------
/*virtual*/ void PVARRAY::FixUp()	// set parent linkage
{	pv_g.gx_SetParent( this);
}
//-----------------------------------------------------------------------------
/*virtual*/ void PVARRAY::Copy( const record* pSrc, int options/*=0*/)
{	// bitwise copy of record
	record::Copy( pSrc, options);	// calls FixUp()
	// copy SURFGEOM heap subobjects
	pv_g.gx_CopySubObjects();
}	// PVARRAY::Copy
//-----------------------------------------------------------------------------
/*virtual*/ PVARRAY& PVARRAY::CopyFrom(
	const record* src,
	int copyName/*=1*/,
	int dupPtrs/*=0*/)
{
	record::CopyFrom( src, copyName, dupPtrs);		// calls FixUp()
	pv_g.gx_CopySubObjects();
#if defined( _DEBUG)
	Validate( 1);	// 1: check SURFGEOMDET also
#endif
	return *this;
}		// PVARRAY::CopyFrom
//-----------------------------------------------------------------------------
/*virtual*/ RC PVARRAY::Validate(
	int options/*=0*/)		// options bits
							//  1: check child SURFGEOMDET also
{
	RC rc = record::Validate( options);
	if (rc == RCOK)
		rc = pv_g.gx_Validate( this, "PVARRAY", options);
	return rc;
}		// PVARRAY::Validate
//-----------------------------------------------------------------------------
int PVARRAY::pv_HasPenumbraShading() const
// returns
//   0 iff no geometry or unsupported type
//   1 iff has geometry and shading possible
{	return !pv_g.gx_IsEmpty() && pv_AxisCount()==0;
}		// PVARRAY::pv_HasPenumbraShading
//-----------------------------------------------------------------------------
int PVARRAY::pv_AxisCount() const
// returns 0 (=fixed), 1 (=1-axis tracking), or 2 (=2-axis tracking
{	return (pv_arrayType == C_PVARRCH_FXDOR || pv_arrayType == C_PVARRCH_FXDRF) ? 0
         : pv_arrayType == C_PVARRCH_2AXT ? 2
	     :                                  1;
}		// PVARRAY::pv_AxisCount
//-----------------------------------------------------------------------------
RC PVARRAY::pv_CkF()
{
	RC rc = RCOK;

	const char* pvArrTyTx = getChoiTx(PVARRAY_ARRAYTYPE, 1);
	const char* whenAT = strtprintf("when pvArrayType=%s", pvArrTyTx);

	int axisCount = pv_AxisCount();		// 0 (fixed), 1, or 2

	// process geometry
	RC rcGeom = pv_g.gx_CheckAndMakePolygon( 0, PVARRAY_G);
	rc |= rcGeom;
	bool bDetailGeom = !rcGeom && !pv_g.gx_IsEmpty();
	if (bDetailGeom)
	{	if (axisCount > 0)
			rc |= oer("Cannot use pvVertices detailed geometry %s", whenAT);
		else
		{	// pvVertices provided -- force pv_azm and pv_tilt to match
			float dgAzm=0.f, dgTilt=0.f;
			pv_g.gx_GetAzmTilt( dgAzm, dgTilt);
			if (IsSet( PVARRAY_AZM) && frDiff( dgAzm, pv_azm) > .005f)
				oer( "Array azimuth derived from pvVertices (=%0.2f) does not match pvAzm (=%0.2f)."
				     " Omit pvAzm to use pvVertices value as default.",
					DEG( dgAzm), DEG( pv_azm));
			pv_azm = dgAzm;
			fStat( PVARRAY_AZM) |= FsVAL;
			if (IsSet( PVARRAY_TILT) && frDiff( dgTilt, pv_tilt) > .005f)
				oer( "Array tilt derived from pvVertices (=%0.2f) does not match pvTilt (=%0.2f)."
				     " Omit pvTilt to use pvVertices value as default.",
					DEG( dgTilt), DEG( pv_tilt));
			pv_tilt = dgTilt;
			fStat( PVARRAY_TILT) |= FsVAL;
		}
	}
	else
	{	// geometry not given, check azm and tilt against array type
		if (axisCount != 2)
			rc |= requireN( whenAT, PVARRAY_AZM, PVARRAY_TILT, 0);
		else
			rc |= ignoreN( whenAT, PVARRAY_AZM, PVARRAY_TILT, 0);

		if (axisCount != 1)
			rc |= ignore( whenAT, PVARRAY_GCR);
	}

	const char* pvModTyTx = getChoiTx(PVARRAY_MODULETYPE, 1);
	const char* whenMT = strtprintf("when pvModuleType=%s", pvModTyTx);

	if (pv_moduleType != C_PVMODCH_CST) {
		rc |= disallowN(whenMT, PVARRAY_TEMPCOEFF, PVARRAY_COVREFRIND, 0);
	}
	else {
		rc |= requireN(whenMT, PVARRAY_TEMPCOEFF, PVARRAY_COVREFRIND, 0);
		if (pv_tempCoeff > 0.f) {
			rc |= oWarn("Temperature coefficient (%0.4f) is positive. Values are typically negative.", pv_tempCoeff);
		}
	}

	return rc;

}	// PVARRAY::pv_CkF

RC PVARRAY::pv_Init()		// init for run
{
	RC rc = pv_g.gx_Init();

	switch (pv_moduleType)
	{
	case C_PVMODCH_STD:
		pv_tempCoeff = -0.37f*NPten[2] * 0.5556f;  // convert %/C to frac/F
		pv_covRefrInd = 1.3f;  // same as air (i.e. no coating)
		break;
	case C_PVMODCH_PRM:
		pv_tempCoeff = -0.35f*NPten[2] * 0.5556f;  // convert %/C to frac/F
		pv_covRefrInd = 1.3f;
		break;
	case C_PVMODCH_THF:
		pv_tempCoeff = -0.32f*NPten[2] * 0.5556f;  // convert %/C to frac/F
		pv_covRefrInd = 1.3f;  // same as air (i.e. no coating)
		break;
	case C_PVMODCH_CST:
		break;  // Set by input
	}

	switch (pv_arrayType)
	{
	case C_PVARRCH_FXDRF: pv_inoct = DegCtoF(49.f); break;
	default: pv_inoct = DegCtoF(45.f);
	}

	rc |= pv_CalcNOCT();

	// Calculate cover transmittance at normal incidence
	float nAR = airRefrInd / pv_covRefrInd;
	float nGl = pv_covRefrInd / glRefrInd;

	// limit as theta approaches zero for snell + bougher
	float tauARNorm = 1 - pow2(1 - nAR) / pow2(1 + nAR);
	float tauGlNorm = 1 - pow2(1 - nGl) / pow2(1 + nGl);

	pv_tauNorm = tauARNorm*tauGlNorm;

	return rc;
}	// PVARRAY::pv_Init

RC PVARRAY::pv_DoHour()
{

	RC rc = RCOK;

	rc |= pv_CalcPOA();

	if (Top.isBegRun)
	{
		pv_tCellLs = Top.tDbOHrAv;
		pv_radILs = pv_radI;
	}

	// Use top level grndRefl if not set for the PV array
	if (!IsSet( PVARRAY_GRNDREFL ))
		pv_grndRefl = Top.grndRefl;

	// Calculate cell temperture
	rc |= pv_CalcCellTemp();

	// Calculate DC output
	const float tRef = DegCtoF(25.f);
	const float iRef = IrSItoIP(1000.f);
	float dcMax = pv_dcCap*Pten[3] * BtuperWh;  // Btu (fix if moving to subhour timesteps)
	pv_dcOut = pv_radTrans * dcMax / iRef*(1 + pv_tempCoeff*(pv_tCell - tRef));

	// Apply system losses
	pv_dcOut *= 1 - pv_sysLoss;

	// Apply inverter efficiency
	float plf = pv_dcOut / dcMax;
	const float effRef = 0.9637f;
	float acMax = dcMax / pv_dcacRat;
	float eff;
	if (plf <= 0.f) {
		eff = 0.f;
	}
	else {
		eff = pv_invEff / effRef*(-0.0162f*plf - 0.0059f / plf + 0.9858f);
	}

	eff = min(1.f, max(0.f, eff));  // bound efficiency between 0 and 1

	pv_acOut = eff*pv_dcOut;
	pv_acOut = min(acMax, pv_acOut);  // clip output if it exceeds AC nameplate

	if (pv_pMtrElec)
		MtrB.p[pv_elecMtri].H.mtr_Accum(pv_endUse, -pv_acOut);

	pv_tCellLs = pv_tCell;
	pv_radILs = pv_radI;

	return rc;
}	// PVARRAY::pv_DoHour
//-----------------------------------------------------------------------------
RC PVARRAY::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup(pSrc, options);
	pv_SetMTRPtrs();
	return rc;
}	// PVARRAY::RunDup
//-----------------------------------------------------------------------------
void PVARRAY::pv_SetMTRPtrs()		// set runtime pointers to meters
// WHY: simplifies runtime code
{
	pv_pMtrElec = MtrB.GetAtSafe(pv_elecMtri);		// elec mtr or NULL
}		// PVARRAY::pv_SetMTRPtrs
//-----------------------------------------------------------------------------
RC PVARRAY::pv_CalcRefr(	// combine Snell's and Bougher's laws
	float n1,  // first index of refraction
	float n2,  // second index of refraction
	float theta1, // angle of incidence, rad
	float& theta2,  // refracted angle, rad
	float& tau)  // transmittance (neglecting absorption extinction exponent)
{
	RC rc = RCOK;

	if (theta1 == 0.f) theta1 = 0.000001f;

	theta2 = asin(n1 / n2*sin(theta1));
	tau = 1.f - 0.5f*(pow2(sin(theta2 - theta1)) / pow2(sin(theta2 + theta1)) + pow2(tan(theta2 - theta1)) / pow2(tan(theta2 + theta1)));
	return rc;
}
//-----------------------------------------------------------------------------
RC PVARRAY::pv_CalcPOA()
{
	RC rc = RCOK;

	// Some routines borrowed from elsewhere in code. Should be updated to share routines with XSURF and SBC.

	// Calculate horizontal incidence
	static float dchoriz[3] = { 0.f, 0.f, 1.f };	// dir cos of a horiz surface
	float verSun = 0.f;				// hour's vertical fract of beam (cosi to horiz); init 0 for when sun down.
	float azm, cosz;
	int sunup = 					// nz if sun above horizon this hour
		slsurfhr(dchoriz, Top.iHrST, &verSun, &azm, &cosz);

#if 0
0 // incomplete effort to set up global values for solar position
0	if (sunup != Wthr.d.wd_sunup || azm != Wthr.d.wd_slAzm)
0		printf( "Solar ismatch");
#endif
	float DNI = Wthr.d.wd_DNI;	// use unmodified irradiance
	float DHI = Wthr.d.wd_DHI;	//    (else potentially double aniso)

	// Don't bother if sun down or no solar from weather;
	if (!sunup || (DNI <= 0.f && DHI <= 0.f))
	{	pv_ClearShading();
		pv_poa = 
		pv_radIBeam =
		pv_radIBeamEff =
		pv_radI =
		pv_radIEff =
		pv_radTrans = 0.f;
		pv_aoi = kPiOver2;
		return rc;
	}

	switch (pv_arrayType)
	{
	case C_PVARRCH_1AXT:
	{
		// Based on Rotation Angle for the Optimum Tracking of One-Axis Trackers by William F.Marion and Aron P.Dobos
		const float sinz = sin(acos(cosz));  // sin of zenith angle
		float azm_delta = azm - pv_azm;

		// normalize azm_delta between -Pi and Pi
		if (azm_delta >= kPi)
			azm_delta = azm_delta - k2Pi;

		if (azm_delta <= -kPi)
			azm_delta = azm_delta + k2Pi;

		const float x = (sinz*sin(azm_delta)) / (sinz*cos(azm_delta)*sin(pv_tilt) + cosz*cos(pv_tilt));  // from Equation #7
		float psi = 0.f;

		if (x < 0.f && azm_delta > 0.f) {
			psi = kPi;
		}
		else if (x > 0.f && azm_delta < 0.f){
			psi = -kPi;
		}
		
		pv_panelRot = atan(x) + psi;  // Equation #7

		const float rlim = 0.25f*kPi;
		pv_panelRot = max(-rlim, min(rlim, pv_panelRot));
		pv_panelTilt = acos(cos(pv_panelRot)*cos(pv_tilt));  // Equation #1
		if (pv_panelTilt == 0.f) {
			pv_panelAzm = pv_azm;
		}
		else
		{	float const asrx =
			  asin(bracket(-1., sin(double( pv_panelRot)) / sin(double( pv_panelTilt)), 1.));
			if (pv_panelRot >= -kPiOver2 && pv_panelRot <= kPiOver2)
				pv_panelAzm = pv_azm + asrx;        // Equation #2
			else if (pv_panelRot >= -kPi && pv_panelRot < -kPiOver2)
				pv_panelAzm = pv_azm - asrx - kPi;  // Equation #3
			else // if (pv_panelRot > kPiOver2 && pv_panelRot <= kPi)
				pv_panelAzm = pv_azm - asrx + kPi;  // Equation #4
		}
	}
		break;
	case C_PVARRCH_2AXT:
		pv_panelTilt = acos(cosz);
		pv_panelAzm = azm;
		break;
	default:
		pv_panelTilt = pv_tilt;
		pv_panelAzm = pv_azm;
		break;
	}

	// Set up properties of array
	double cosTilt = cos(pv_panelTilt);
	float vfGrndDf = .5 - .5*cosTilt;  // view factor to ground
	float vfSkyDf = .5 + .5*cosTilt;  // view factor to sky


	// Calculate plane-of-array incidence
	int sunupSrf;	// nz iff
	float cosi, fBeam; // cos(incidence) and unshaded fraction
	if (pv_HasPenumbraShading() && Top.tp_PumbraAvailability() > 0)
		sunupSrf = pv_CalcBeamShading( cosi, fBeam);
	else
	{	// tracking: Penumbra shading not supported
		//   azm and tilt vary
		float dcos[3]; // direction cosines
		slsdc( pv_panelAzm, pv_panelTilt, dcos);
		sunupSrf = slsurfhr( dcos, Top.iHrST, &cosi);
		fBeam = 1.f;
	}
	if (sunupSrf < 0)
		return RCBAD;	// shading error

	if (sunupSrf)	
	{
		pv_poaBeam = DNI*cosi;
		pv_radIBeam = pv_poaBeam*fBeam;  // incident beam (including shading)
		pv_radIBeamEff = max(pv_poaBeam*(1.f - pv_sif * (1.f - fBeam)),0.f);
		pv_aoi = acos(cosi);
	}
	else
	{	pv_poaBeam = pv_radIBeam = pv_radIBeamEff = 0.f;  // incident beam
		pv_aoi = kPiOver2;
	}

	float poaDiffI, poaDiffC, poaDiffH, poaDiffG;  // Components of diffuse solar (isotropic, circumsolar, horizon, ground reflected)

	bool usePerez = true;

	if (usePerez)
	{
		// Modified Perez sky model
		float zRad = acos(cosz);
		float zDeg = DEG(zRad);
		const float kappa = 5.534e-6f;
		float e = ((DHI + DNI) / (DHI + 0.0001f) + kappa*pow3(zDeg)) / (1 + kappa*pow3(zDeg));
		float am0 = 1 / (cosz + 0.15f*pow(93.9f - zDeg, -1.253f));  // Kasten 1966 air mass model
		float delt = DHI*am0 / IrSItoIP(1367.f);

		int bin = e < 1.065f ? 0 :
			e >= 1.065f && e < 1.23f ? 1 :
			e >= 1.23f && e < 1.5f ? 2 :
			e >= 1.5f && e < 1.95f ? 3 :
			e >= 1.95f && e < 2.8f ? 4 :
			e >= 2.8f && e < 4.5f ? 5 :
			e >= 4.5f && e < 6.2f ? 6 : 7;

		// Perez sky model factors
		static const float f11[8] = { -0.0083117f, 0.1299457f, 0.3296958f, 0.5682053f, 0.873028f, 1.1326077f, 1.0601591f, 0.677747f };
		static const float f12[8] = { 0.5877285f, 0.6825954f, 0.4868735f, 0.1874525f, -0.3920403f, -1.2367284f, -1.5999137f, -0.3272588f };
		static const float f13[8] = { -0.0620636f, -0.1513752f, -0.2210958f, -0.295129f, -0.3616149f, -0.4118494f, -0.3589221f, -0.2504286f };
		static const float f21[8] = { -0.0596012f, -0.0189325f, 0.055414f, 0.1088631f, 0.2255647f, 0.2877813f, 0.2642124f, 0.1561313f };
		static const float f22[8] = { 0.0721249f, 0.065965f, -0.0639588f, -0.1519229f, -0.4620442f, -0.8230357f, -1.127234f, -1.3765031f };
		static const float f23[8] = { -0.0220216f, -0.0288748f, -0.0260542f, -0.0139754f, 0.0012448f, 0.0558651f, 0.1310694f, 0.2506212f };

		float F1 = max(0.f, f11[bin] + delt*f12[bin] + zRad*f13[bin]);
		float F2 = f21[bin] + delt*f22[bin] + zRad*f23[bin];

		if (zDeg >= 0.f && zDeg <= 87.5f) {
			poaDiffI = (1.f - F1)*vfSkyDf;
			poaDiffC = F1* max(0.f, cosi) / max(cos(RAD(85.f)), cosz);
			poaDiffH = F2*sin(pv_panelTilt);
		}
		else {
			poaDiffI = vfSkyDf;
			poaDiffC = 0.f;
			poaDiffH = 0.f;
		}
	}
	else {
		poaDiffI = vfSkyDf;
		poaDiffC = 0.f;
		poaDiffH = 0.f;
	}

	poaDiffG = pv_grndRefl*vfGrndDf;

	float poaDiff = DHI*(poaDiffI + poaDiffC + poaDiffH + poaDiffG);  // sky diffuse and ground reflected diffuse
	float poaGrnd = DNI*pv_grndRefl*vfGrndDf*verSun;  // ground reflected beam
	pv_poa = pv_poaBeam + poaDiff + poaGrnd;
	pv_radI = pv_radIBeam + poaDiff + poaGrnd;
	pv_radIEff = pv_radIBeamEff + poaDiff + poaGrnd;

	// Correct for off-normal transmittance
	float theta1 = pv_aoi;
	float theta2, tauAR, theta3, tauGl;

	rc |= pv_CalcRefr(airRefrInd, pv_covRefrInd, theta1, theta2, tauAR);
	rc |= pv_CalcRefr(pv_covRefrInd, glRefrInd, theta2, theta3, tauGl);

	float tauC = tauAR*tauGl;

	pv_radTrans = pv_radIBeamEff *tauC / pv_tauNorm + poaDiff + poaGrnd;

	return rc;
}

float PVARRAY::pv_CalcConv(
	float tCell,  // Cell temperature, K
	float tAir,  // Ambient air temperature K
	float vWind)  // Weather file windspeed m/s
	// Returns total convection coefficient (forced + natural), W/m2-K
{
	const float xLen = 0.5f;  // m, assumed length of panel

	float tAve = 0.5f*(tCell + tAir);  // K
	float dAir = 0.003484f * 101325.f / tAve;  // kg/m3
	float vAir = 0.24237e-6f * pow(tAve, 0.76f) / dAir;  // m2/s
	float kAir = 2.1695e-4f * pow(tAve, 0.84f); // W/m-K
	float re = vWind*xLen / vAir;

	float hf;  // W/m2-K
	if (re > 1.25e5f) {
		hf = 0.0282f / pow(re, 0.2f)*dAir*vWind*1007.f / pow(0.71f, 0.4f);
	}
	else {
		hf = 0.86f / sqrt(re)*dAir*vWind*1007.f / pow(0.71f, 0.67f);
	}
	float gr = 9.8f / tAve*abs(tCell - tAir)*pow3(xLen) / pow2(vAir)*0.5f;
	float hn = 0.21f*pow((gr*0.71f), 0.32f)*kAir / xLen;  // W/m2-K
	float hc = pow033(pow3(hn) + pow3(hf));  // W/m2-K
	return hc;
}

float PVARRAY::pv_CalcSkyTemp(
	float tAir)  // outdoor dry bulb, K
	// Returns effective sky temperature, K
{
	return 0.68f*(0.0552f*tAir*sqrt(tAir)) + 0.32f*tAir;
}

RC PVARRAY::pv_CalcNOCT()
{
	RC rc = RCOK;

	/* Routine from Fuentes 1987 article: A Simplified Thermal Model for Flat Plate Photovoltaic Arrays
	Used to be consistent with PVWatts.
	Calculates the convection ratio and ground temperature ratio under NOTC conditions to be used during simulation.
	Calculations performed in SI units. */

	float tINOCT = DegFtoK(pv_inoct);  // K
	float tAirNOCT = DegCtoK(20.f);  // K
	const float iNOTC = 800.f;  // W/m2
	float tSkyNOCT = pv_CalcSkyTemp(tAirNOCT);
	const float vWindNOCT = 1.f;  // m/s


	// Calculate thermal capacitance
	if (tINOCT > 321.15f) pv_thermCap = 11000.f*(1.f + (tINOCT - 321.15f) / 12.f);
	else pv_thermCap = 11000.f;

	const float emiss = 0.84f;
	const float absorp = 0.83f;

	float hcNOCT = pv_CalcConv(tINOCT, tAirNOCT, vWindNOCT);
	float hgr = emiss*sigmaSBSI*(pow2(tINOCT) + pow2(tAirNOCT))*(tINOCT + tAirNOCT);
	float rB = (absorp*iNOTC - emiss*sigmaSBSI*(pow4(tINOCT) - pow4(tSkyNOCT))
		- hcNOCT*(tINOCT - tAirNOCT)) / ((hgr + hcNOCT)*(tINOCT - tAirNOCT));
	float tGrnd = pow(pow4(tINOCT) - rB*(pow4(tINOCT) - pow4(tAirNOCT)), 0.25f);
	if (tGrnd > tINOCT) tGrnd = tINOCT;
	if (tGrnd < tAirNOCT) tGrnd = tAirNOCT;
	pv_tGrndRatio = (tGrnd - tAirNOCT) / (tINOCT - tAirNOCT);
	pv_convRatio = (absorp*iNOTC - emiss*sigmaSBSI*(2.f*pow4(tINOCT) - pow4(tSkyNOCT) - pow4(tGrnd))) / (hcNOCT*(tINOCT - tAirNOCT));

	return rc;
}

RC PVARRAY::pv_CalcCellTemp()
{
	RC rc = RCOK;

	/* Routine from Fuentes 1987 article: A Simplified Thermal Model for Flat Plate Photovoltaic Arrays
	   Used to be consistent with PVWatts.
	   Calculations performed in SI units. */
	const float emiss = 0.84f;
	const float absorp = 0.83f;
	const float ts = 3600.f;  // s (change if moving to subhour timesteps)

	float tMod0 = DegFtoK(pv_tCellLs);  // K
	float tAir = DegFtoK(Top.tDbOHrAv);  // K
	float vWindWS = VIPtoSI(Top.windSpeedHrAv);  // m/s
	float iSol0 = IrIPtoSI(pv_radILs)*absorp;  // W/m2
	float iSol = IrIPtoSI(pv_radI)*absorp;  // W/m2

	float tMod = tMod0;  // initialize cell temperature, K
	const float htMod = 5.f;  // m, assumed by PVWatts
	const float htWS = 10.f;  // m, assumed assumed by PVWatts (assumption is not explicit)
	float vWind = vWindWS*pow(htMod / htWS, 0.2f) + 0.0001f;  // m/s
	float tSky = pv_CalcSkyTemp(tAir);  // K

	const int maxIt = 10;
	const float tol = NPten[2];  // K

	float tModPv = tMod0;
	int it = 0;

	bool iterate = true;
	while (iterate) {
		float hc = pv_convRatio*pv_CalcConv(tMod, tAir, vWind);
		float hsk = emiss*sigmaSBSI*(pow2(tMod) + pow2(tSky))*(tMod + tSky);
		float tGrnd = tAir + pv_tGrndRatio*(tMod - tAir);
		float hgr = emiss*sigmaSBSI*(pow2(tMod) + pow2(tAir))*(tMod + tAir);
		float eigen = -(hc + hsk + hgr) / pv_thermCap * ts;
		float ex = 0.f;
		if (eigen > -10.f) ex = exp(eigen);
		tMod = tMod0*ex +
			((1.f - ex)*(hc*tAir + hsk*tSky + hgr*tGrnd + iSol0 + (iSol - iSol0) / eigen) + iSol - iSol0)
			/ (hc + hsk + hgr);
		if (it == maxIt || abs(tModPv - tMod) < tol) {
			iterate = false;
		}
		else {
			it++;
			tModPv = tMod;
		}
	}

	pv_tCell = DegKtoF(tMod);

	return rc;
}