// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// psychro.h -- include file for psychro.cpp psychrometric functions
//==========================================================================

#if !defined( _I_PSYCHRO_H_)
#define	_I_PSYCHRO_H_

//--- Constants and publics
// limits of temperature range over which psychro functions work, rob 5-95
// (particularly psyPsat, used internally in many others)(must match psychro1:psyPsTab[])
const int PSYCHROMINT = -100;			// temp for 1st psyPsTab entry (OK for range check)
const int PSYCHROMAXTX = +300;			// temp for last psyPsTab entry
const int PSYCHROMAXT = PSYCHROMAXTX-1;	// max temp for range check (must be < last entry)
// The units of these constants defines the unit system of the
// psychro functions.  SI equivalents are given for general interest.
// To convert to SI, redefine these constants and check code.
const float PsyMwRatio    = 0.62472f;
			// Ratio of molecular weights of water vapor and dry air,
			// including enhancement adjustment factor of 1.0044 which is OK around standard
			// pressure and room temperatures; perfect gas value is 0.621945.
			// The enhancement adjustment approximates fs correction described in
			// ASHRAE Brochure on Psychrometry, p. 28 (Table 10) and also ASHRAE Fundamentals.
			// When using PsyMwRatio, make sure adjusted value is appropriate (else use PsyMwRatioX).
const float PsyMwRatioX   = 0.621945f;	// perfect gase molecular weight ratio (water / dry air)
const float PsyShDryAir   = 0.240f;	// Specific heat of dry air, Btu/lb-F (1.006 kJ/kg-C SI)
const float PsyShWtrVpr   = 0.444f;	// Specific heat of water vapor, Btu/lb-F.  (1.805 kJ/kg-C SI)
const float PsyHCondWtr   = 1061.f;	// Heat of condensation of water Btu/lb.  (2500.4 kJ/kg SI)
const float PsyTAbs0      = 459.67f;	// Absolute temp (Rankine) corresponding to 0 F.   (273.15 K SI)
const float PsyRAir       = 0.75435f;	// Gas constant for dry air, (in Hg)ft3/lb-mole-R (287.055 J/kg-K SI)
#if 0	// no current uses 5-17-89
x  const float PsyRWtr     = 1.20359f;// Gas constant for water vapor. (561.52 SI)
#endif

const float PsyPBarSeaLvl = 29.921f;	// standard sea level barometric pressure, in. Hg.
										// (101.325 kPa SI)
extern float PsyPBar;	// Barometric pressure (in. Hg).  Initialized value
						//  is standard sea level pressure, reset by psyElevation()
const float PsyWmin = 0.00000001f;	// minimum humidity ratio handled or returned


/*-------------------------------- OPTIONS --------------------------------*/
#undef PSYDETAIL	// if defined, detailed versions of some functions
			   		// 	are included.  These were used to develop the
					//  faster and less accurate versions that are
					//  generally used.

/* -------------------------- PUBLIC FUNCTIONS ---------------------------- */
float psyElevation( float elev);
float psyElevForP( float bp);
float calc_outside_rh( float db, float wb);
float calc_outside_wb( float db, float rh);
float psyHumRat1( float ta, float tw);
float psyHumRatX( float ta, float& tw);
float psyHumRat2( float ta, float rh);
float psyPsat( float t);
float psyTDewPoint( float w);
float psyHumRatMinTdp();
float psyEnthalpy( float ta, float w);
float psyEnthalpy2( float t, float mu, float& w);
float psyEnthalpy3( float t, float rh, float& w);
float psySatEnthalpy( float ts);
float psySatEnthSlope( float ts);
float psyTWEnthMu( const float h, const float mu, float& w);
float psyTWEnthRh( const float h, const float rh, float& w);
float psyRelHum( float ta, float td);
float psyRelHum2( float ta, float tw);
float psyRelHum3( float w, float wSat);
float psyRelHum4( float ta, float w);
int psyLimitRh( float rh, float& ta, float& w);
inline int psyLimitRh( float rh, double& ta, double& w)
{	float taf=ta; float wf=w;
	int ret = psyLimitRh( rh, taf, wf);
	ta = taf; w = wf;
	return ret;
}
float psyTWetBulb( float ta, float w);
float psyTDryBulb( float w, float h);
float psySpVol( float ta, float w);
#if 0	// density rework, 10-18-2012
x double psyDensity( float ta, float w, double pDelta=0.);
#else
inline double psyDenDryAir(	float ta, float w, double pDelta=0.)
{	return (PsyPBar+pDelta)/(PsyRAir*(1.+w/PsyMwRatio)*(ta+PsyTAbs0)); }
inline double psyDenDryAir2( double rhoMoistAir, float w)
{	return rhoMoistAir / (1.+w); }
inline double psyDenMoistAir( float ta, float w, double pDelta=0.)
{	return psyDenDryAir( ta, w, pDelta) * (1.+w); }
inline double psyDenMoistAir2( double rhoDryAir, float w)
{	return rhoDryAir * (1.+w); }
#endif
float psyHumRat3( float td);
float psyHumRat4( float ta, float h);
float psyTsat( float ps);
#ifdef PSYDETAIL
  void   psattbl( void);
  double psyTsatX( double ps);
  double psyPsatX( double tc);
#endif

// psychro-related unit conversions
inline double InHgToLbSf( double p) { return p/.0141390; }
inline double LbSfToInHg( double p) { return p*.0141390; }


#endif // _I_PSYCHRO_H_

// end of psychro.h
