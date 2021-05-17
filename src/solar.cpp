////////////////////////////////////////////////////////////////////////////
// solar.cpp: solar calculations
//=============================================================================
// Copyright (c) 2014, ASHRAE.
//   This work is a product of ASHRAE 1383-RP.
// All rights reserved.
 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of ASHRAE, the University of California, Berkeley nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//==========================================================================


#include "cnglob.h"

#include "tdpak.h"

#include "solar.h"

//===========================================================================
// Notes
// Time: hour 0 herein is midnight, 12=noon.  In some cases float time is used
//   e.g. 18.25 = 6:15 PM.  These conventions may DIFFER from other time uses.
//   Variables are often named iT ("time") as opposed to iH ("hour").
// Hour Angle: time as angle from solar noon at longitude (+ = afternoon)
//   Adjusted for longitude from center of time zone and from daylight or
//   local time to solar (and date still changes at local/daylite midnite),
//   so range is bit more than -PI to PI. (mainly used when sun is up.)
// Direction cosine coordinate system: +X=east, +Y=north, +Z=up
//   Right-handed coordinate system, same as EnergyPlus and many other
//   building applications
//===========================================================================

#undef REFGLSDOE2  		// #define to use RefGlsDOE2 else RefGlsASHRAE)
						//  DOE2 ref glass calcs cause some SHGF values
						//  to differ by 1 Btuh/ft2 from ASHRAE table
						//  values --> use ASHRAE.  11-15-00
#undef TESTSHADING		// #define to compare results to un-improved
						//  version of shadwin() (see below)

/////////////////////////////////////////////////////////////////////////////
// struct OHFIN -- window/ovhg/fin geometry, any consistent units
/////////////////////////////////////////////////////////////////////////////
struct OHFIN
{   float wwidth;	// window width
    float wheight;	// window height
    float ohdepth;	// depth of overhang
    float ohwd;		// distance from top of window to overhang
    float ohlx;		// distance overhang extends beyond left edge of window
    float ohrx;		// distance overhang extends beyond right edge of window
    float ohflap;	// depth of vertical projection at the end of overhang
    float fldepth;	// depth of left fin
    float fltx;		// distance left fin extends above top of window
    float flwd;		// distance from left edge of window to left fin
    float flwbx;	// distance left fin stops short above bottom of window
    float frdepth;	// depth of right fin
    float frtx;		// distance right fin extends above top of window
    float frwd;		// distance from right edge of window to right fin
    float frwbx;	// distance right fin stops short above bottom of window
};	// struct OHFIN
//===========================================================================

// local functions
static void RefGlsDOE2( double eta, float& trans, float& abso);
static void RefGlsASHRAE( double eta, float& trans, float& abso);
static double DCToAzm( double dirCos[ 3]);
static float ShadeWin( const OHFIN *p, float gamma, float cosz);
#if defined( TESTSHADING)
 static float shadwin( const OHFIN *p, float gamma, float cosz);
#endif

//-------------------------------------------------------------------------
// data for 21st of each month
static struct
{	float radXT;		// extraterrestrial irradiance on a normal surf
						//   (btuh/ft2)
	float eqTime;		// equation of time (min)
	float decl;			// declination (deg)
	float A;			// coefficient A (Btuh/ft2)
	float B;			// coefficient B (dimless)
	float C;			// coefficient C (dimless)
} ASHRAEData[ 2][ 13] =
{{	// ASHRAE HOF 1997 table 8, p. 29.14
	448.8f,  -11.2f,  -20.0f,   390.f,  0.142f,  0.058f,
    444.2f,  -13.9f,  -10.8f,   385.f,  0.144f,  0.060f,
	437.7f,   -7.5f,    0.0f,   376.f,  0.156f,  0.071f,
	429.9f,    1.1f,   11.6f,   360.f,  0.180f,  0.097f,
 	423.6f,    3.3f,   20.0f,   350.f,  0.196f,  0.121f,
	420.2f,   -1.4f,   23.45f,  345.f,  0.205f,  0.134f,
	420.3f,   -6.2f,   20.6f,   344.f,  0.207f,  0.136f,
	424.1f,   -2.4f,   12.3f,   351.f,  0.201f,  0.122f,
	430.7f,    7.5f,    0.0f,   365.f,  0.177f,  0.092f,
	437.3f,   15.4f,  -10.5f,   378.f,  0.160f,  0.073f,
	445.3f,   13.8f,  -19.8f,   387.f,  0.149f,  0.063f,
	449.1f,    1.6f,  -23.45f,  391.f,  0.142f,  0.057f,
	448.8f,  -11.2f,  -20.0f,   390.f,  0.142f,  0.058f		// jan repeated re (future) interpolation
},
{	// updated ASHRAE HOF 2005 table 7, p. 31.14
	448.8f, -11.2f,  -20.0f, 381.f, 0.141f, 0.103f,
	444.2f, -13.9f,  -10.8f, 376.f, 0.142f, 0.104f,
	437.7f,  -7.5f,    0.0f, 369.f, 0.149f, 0.109f,
	429.9f,   1.1f,   11.6f, 358.f, 0.164f, 0.120f,
	423.6f,   3.3f,   20.0f, 351.f, 0.177f, 0.130f,
	420.2f,  -1.4f,  23.45f, 346.f, 0.185f, 0.137f,
	420.3f,  -6.2f,   20.6f, 346.f, 0.186f, 0.138f,
	424.1f,  -2.4f,   12.3f, 351.f, 0.182f, 0.134f,
	430.7f,   7.5f,    0.0f, 360.f, 0.165f, 0.121f,
	437.3f,  15.4f,  -10.5f, 370.f, 0.152f, 0.111f,
	445.3f,  13.8f,  -19.8f, 377.f, 0.144f, 0.106f,
	449.1f,   1.6f, -23.45f, 382.f, 0.141f, 0.103f,
	448.8f, -11.2f,  -20.0f, 381.f, 0.141f, 0.103f		// jan repeated re (future) interpolation
}};	// ASHRAEData
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRLOC -- time independent solar info for location
/////////////////////////////////////////////////////////////////////////////
SLRLOC::SLRLOC()		// default c'tor
{	sl_Init( 0.f, 0.f, 0.f, 0.f, 0);
}
//---------------------------------------------------------------------------
SLRLOC::SLRLOC( float lat, float lng, float tzn, float elev)
{	sl_Init( lat, lng, tzn, elev, 0);
}		// SLRLOC::SLRLOC
//---------------------------------------------------------------------------
/*virtual*/ SLRLOC::~SLRLOC()
{
}		// SLRLOC::~SLRLOC
//---------------------------------------------------------------------------
void SLRLOC::sl_Init(			// initialize SLRLOC
	float lat,		// latitude (degN, <0 = southern hemisphere)
	float lng,		// longitude (degE, <0 = degW)
	float tzn,		// time zone (GMT +X, e.g. EST = -5, PST = -8,
					//	Paris = +1)
	float elev,		// elevation (ft above sea level)
	int SN,			// weather database station number, 0 if not known
	int options/*=0*/,	// option bits
						//   slropSIMPLETIME: use "simple" time scheme
						//     (no longitude/tzn in solar calcs (assumes long=tzn*15))
	int oMsk/*=0*/)		// DbPrintf mask, re calclog
{
	sl_options = options;
	BOOL bSimpleTime = (options & slropSIMPLETIME) != 0;

	sl_lat = lat;
	sl_lng = bSimpleTime ? tzn*15.f : lng;
	sl_tzn = tzn;
	sl_elev = elev;
	sl_SN = SN;

	sl_sinLat = sin( RAD( sl_lat));
	sl_cosLat = cos( RAD( sl_lat));

	if (oMsk)
	{	// log only "real" calls (not e.g. default c'tor)
		if ((lat != 0.f || lng != 0.f || tzn != 0.f || elev != 0.f) && DbShouldPrint( oMsk))
			DbDump();
	}
}		// SLRLOC::Init
//---------------------------------------------------------------------------
SLRLOC& SLRLOC::Copy( const SLRLOC& sl)
{	sl_options = sl.sl_options;
	sl_lat = sl.sl_lat;
	sl_lng = sl.sl_lng;
	sl_tzn = sl.sl_tzn;
	sl_elev = sl.sl_elev;
	sl_SN = sl.sl_SN;
	sl_sinLat = sl.sl_sinLat;
	sl_cosLat = sl.sl_cosLat;
	return *this;
}		// SLRLOC::Copy
//---------------------------------------------------------------------------
BOOL SLRLOC::IsEqual( const SLRLOC& sl) const
{	return sl_SN == sl.sl_SN
	    && sl_options == sl.sl_options
		&& sl_lat == sl.sl_lat
		&& sl_lng == sl.sl_lng
		&& sl_tzn == sl.sl_tzn
		&& sl_elev == sl.sl_elev
		&& sl_sinLat == sl.sl_sinLat
		&& sl_cosLat == sl.sl_cosLat;
}		// SLRLOC::IsEqual
//---------------------------------------------------------------------------
int SLRLOC::CalcDOYForMonth(			// representative day
	int iMon) const		// month 1 - 12
// returns doy having approximately average declination over month;
//   this day is used for once-a-month solar geometry calcs.
//   Future: might become function of location (re arctic?)
{
static int repDays[ 13] = 		// representative doy by month
#if 1	// change days to agree with Duffie & Beckman, 7-10-08
// source: Duffie & Beckman, Table 1.6.1
{	-1, 17, 46, 75, 105, 135, 162, 198, 228, 258, 288, 318, 344 };
#else
x {	-1, 17, 46, 75, 105, 135, 162, 198, 228, 259, 289, 319, 345 };
#endif
	return repDays[ iMon];
}		// SLRLOC::CalcDOYForMonth
//---------------------------------------------------------------------------
void SLRLOC::DbDump() const
{
	DbPrintf(
	 _T("SLRLOC:  Lat=%.2f  Long=%.2f  TZN=%.2f  elev=%.0f  options=0x%x  SN=%d  sinLat=%0.3f  cosLat=%0.3f\n"),
		sl_lat, sl_lng, sl_tzn, sl_elev, sl_options, sl_SN, sl_sinLat, sl_cosLat);
}		// SLRLOC::DbDump
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRDAY -- solar info for a single day
/////////////////////////////////////////////////////////////////////////////
SLRDAY::SLRDAY( SLRLOC* pSL /*=NULL*/)
   : sd_pSL( pSL)
// NOTE: most mbrs not initialized, call Setup()
{
}		// SLRDAY::SLRDAY
//---------------------------------------------------------------------------
/*virtual*/ SLRDAY::~SLRDAY()
{
}		// SLRDAY::~SLRDAY
//---------------------------------------------------------------------------
void SLRDAY::SetupCalcDOY(			// set up representative calc day
	int iMon,		// month (1-12)
	int sltm /*=sltmLST*/,		// time convention: sltmSLR/LST/LDT
	SLRLOC* pSL /*=NULL*/)		// solar location (not used if NULL)
{
	if (pSL)
		sd_pSL = pSL;
	int doy = sd_pSL
		? sd_pSL->CalcDOYForMonth( iMon)
		: 162;	// bad sd_pSL, will be msg'd in Setup2()
	Setup( doy, sltm);
}		// SLRDAY::SetupCalcDOY
//---------------------------------------------------------------------------
void SLRDAY::Setup(
	int doy,		// day of year (1-365)
	int sltm /*=sltmLST*/,		// time convention: sltmSLR/LST/LDT
	SLRLOC* pSL /*=NULL*/)		// solar location (not used if NULL)

{
	if (pSL)
		sd_pSL = pSL;
	sd_doy = doy;
	sd_iM = Calendar.GetMonDoy( sd_doy) - 1;	// 0-based month
	sd_sltm = sltm;
	sd_iCSM = 1;	// "new" clear sky model used

// Story: SERI methods use Fourier series for declination, equation of time
//    and earth/sun distance.  The general form is:
// 	x = c0 + c1*cos(t) + c2*sin(t) + c3*cos(2t) + c4*sin(2t) ...
//    where t is the "day angle" = 2*Pi*(doy-1)/365.  In this implementation,
//    the array sc is set to cos(t), sin(t), cos(2t), sin(2t), cos(3t), and
//    sin(3t).
// Solar Energy Related Calculations and Reference Data, Branch 215
// Consensus Materials, SERI 1987?, obtained from Tom Stoffel, 8-8-90.

static double cDec[ 7] =		// coefficients for declination
{ 0.006918, -0.399912, 0.070257, -0.006758, 0.000907, -0.002697,  0.001480 };
static double cEtm[ 5] =	// coefficients for equation of time
{  0.000075,   0.001868, -0.032077, -0.014615, -0.040849 };
static double cEsd[ 5] =	// coefficients for earth/sun distance
{  1.000100,   0.034221,  0.001280, 0.000719,  0.000077 };

	// set sc array to cos/sin values of "day angle"
	double sc[ 6];
	for (int i=0; i<3; i++)
	{	double t = (i+1) * (sd_doy-1) * k2Pi / 365.;
		sc[ 2*i]     = cos( t);		// cos(t), cos(2t), cos(3t)
		sc[ 2*i + 1] = sin( t);		// sin(t), sin(2t), sin(3t)
	}

	sd_decl 			// declination (degrees)
		= float( DEG( cDec[ 0] + DotProd( sc, cDec+1, 6)));
	sd_eqTime 		// equation of time (hours)
		= float( cEtm[ 0] + DotProd( sc, cEtm+1, 4) / HaPerHr);
	sd_extBm 		// ext bm (solar const * earth/sun dist correction)
		= float( SolarConstant * (cEsd[ 0] + DotProd( sc, cEsd+1, 4) ));

	Setup2();
}			// SLRDAY::Setup
//---------------------------------------------------------------------------
void SLRDAY::SetupASHRAE(		// initialize using ASHRAE methods
	int iMon,		// month (1-12) ... values are for 21st of month
	int options,	// option bits
					//	slropCSMOLD: use "old" coefficients and ASHRAE ABC model
					//  slropCSMTAU: use tau model if data available
					//         else: use "new" (2005) coefficients and ASHRAE ABC model
					//  slropCSMTAU13: with slropCSMTAU, use 2013 coefficients
					//         else use 2009
	int sltm /*=sltmLST*/,	// time convention: sltmSLR/LST/LDT
	SLRLOC* pSL /*=NULL*/,	// solar location (not used if NULL)
	float tauB	/*=0.f*/,	// for tau clear sky model, beam pseudo-optical depth
	float tauD /*=0.f*/)	//   ditto diffuse
{
	if (pSL)
		sd_pSL = pSL;
	sd_iM = iMon-1;
	sd_doy = Calendar.GetDoy( iMon, 21);
	sd_sltm = sltm;
	// map options to clear sky model choice
	sd_iCSM = (options & slropCSMTAU13)  ? 3	// 2013 tau model
			: (options & slropCSMTAU)	 ? 2	// 2009 tau model
		    : (options & slropCSMOLD)==0 ? 1	// use "new" (2005) coeffs
		    :                              0;	// "old" coeffs
	if (sd_iCSM >= 2)
	{	sd_tauB = tauB;
		sd_tauD = tauD;
		if (sd_tauB < .001f || sd_tauD < .001f)
			sd_iCSM = 1;		// no tau data, revert to "new"
	}
	if (sd_iCSM < 2)
		sd_tauB = sd_tauD = 0.f;

	// basic data
	if (sd_iCSM >= 2)
		// use 2009 methods (unchanged in HOF 2013)
		SetupDOYValuesASHRAE2009();
	else
	{	// use 2005 or order methods
		sd_eqTime = ASHRAEData[ sd_iCSM][ sd_iM].eqTime / 60.;
		sd_decl = ASHRAEData[ sd_iCSM][ sd_iM].decl;
		sd_extBm = ASHRAEData[ sd_iCSM][ sd_iM].radXT;
	}

	Setup2();
}			// SLRDAY::SetupASHRAE
//---------------------------------------------------------------------------
void SLRDAY::SetupDOYValuesASHRAE2009()
// day-of-year values using ASHRAE 2009 methods
// see Chapter 14, Fundamentals
{
	// extraterrestrial beam irradiance (Btuh/ft2)
	sd_extBm = float( SolarConstant * (1. + 0.033*cos((sd_doy-3)*k2Pi/365.)));

	// equation of time (hrs)
	double gamma = (sd_doy-1)*k2Pi/365.;
	double etMin = 2.2918*(.0075 + .1868*cos( gamma) - 3.2077*sin( gamma)
		                  - 1.4615*cos( 2.*gamma) - 4.089*sin( 2.*gamma));
	sd_eqTime = etMin / 60.;

	// declination (degrees)
	sd_decl = 23.45*sin( (sd_doy+284)*k2Pi/365.);	// declination (deg)

}		// SLRDAY::SetupDOYValuesASHRAE2009
//---------------------------------------------------------------------------
/*private*/ BOOL SLRDAY::Setup2() 	// day set up, common sections
{
	if (!sd_pSL)
		return mbIErr( _T("SLRDAY::Setup2"), _T("NULL sd_pSL"));

	sd_haConst = float( -kPi);
	if (sd_sltm != sltmSLR)
	{	sd_haConst +=
			HaPerHr * (sd_eqTime - sd_pSL->sl_tzn) + RAD( sd_pSL->sl_lng);
		if (sd_sltm == sltmLDT)
			sd_haConst -= HaPerHr;	// Daylight savings: 1 hour earlier
	}

	// declination
	sd_sinDecl = sin( RAD( sd_decl));
	sd_cosDecl = cos( RAD( sd_decl));

	sd_sdsl = sd_sinDecl * sd_pSL->sl_sinLat;
	sd_cdsl = sd_cosDecl * sd_pSL->sl_sinLat;
	sd_sdcl = sd_sinDecl * sd_pSL->sl_cosLat;
	sd_cdcl = sd_cosDecl * sd_pSL->sl_cosLat;

	// sunset hour angle (handle arctic/antarctic cases)
    sd_cosSunset = max( -1., min( 1., -sd_sdsl/sd_cdcl));
	sd_haSunset = float( acos( sd_cosSunset));

	// daily total irradiation on horiz surface
	sd_H0 = ExtRadHoriz( -sd_haSunset, sd_haSunset);
	// test code: float H0t = ExtRadHoriz( RAD( -7.5f), RAD( +7.5f));

	// coefficients re hour fraction of daily total
	//   See Duffie & Beckman Eqns 2.13.2 and 2.20.5
	sd_aCoeff = 0.409  + .5016*sin( sd_haSunset - RAD( 60.));
	sd_bCoeff = 0.6609 - .4767*sin( sd_haSunset - RAD( 60.));
	sd_dCoeff = sin( sd_haSunset) - sd_haSunset * cos( sd_haSunset);

	// conditions for solar noon
	sd_HSlrNoon.ha = 0.f;
	sd_HSlrNoon.fUp = sd_haSunset > 0.0001f;	// handle arctic case
	sd_HSlrNoon.Calc( this);

	// compute, for 24 hours of selected time type of day,
	//  solar direction cosines, azimuth, and fraction of hour sun up.
    double tSR  = TmForHa( -sd_haSunset);
	int iTSR = int( tSR);
	double tSS = TmForHa( sd_haSunset);
	int iTSS = int( tSS);

	for (int iT=0; iT < 24; iT++)
    {	if (iT >= iTSR && iT <= iTSS+1)		// if sun up in hr or hr b4
       	{	if (iT == iTSR)		// sunrise hour
			{	sd_H[ iT].ha = float( -sd_haSunset);	// compute for sunrise
             	sd_H[ iT].fUp = (float)(iTSR+1 - tSR);	// fractn of hour sun is up
			}
			else if (iT == iTSS)		// hour during which sun sets
			{	sd_H[ iT].ha = float( HaForTm( iT));	// ha = start of hr as usual
				sd_H[ iT].fUp = float( tSS - iTSS);		// fractn of hour sun is up
			}
			else if (iT == iTSS+1)		// hr starting after sunset
			{	sd_H[ iT].ha = float( sd_haSunset);	// gets dircos etc. for sunset
           		sd_H[ iT].fUp = 0.f;				// sun not up
			}
			else			// hour entirely in daytime
			{	sd_H[ iT].ha = float( HaForTm( iT));
				sd_H[ iT].fUp = 1.f;
			}
		}
		else	// sun down during hour (1st night hour done above)
		{ 	sd_H[ iT].ha = -99.f; 	// hour angle: not known
			sd_H[ iT].fUp = 0.f;		// fraction up: none
		}
		sd_H[ iT].Calc( this);
	}
	return TRUE;
}		// SLRDAY::Setup2
//---------------------------------------------------------------------------
int SLRDAY::SolarTimeShift() const
// Load calcs done in LST or LDT iH = 0-23 (and solar data set up accordingly)
// return shift for LST/LDT access to profiles indexed by solar time --
//	  X[ iH] = XSolarTime[ (iH+iHShift)%24]
// Note: returned value >= 0 (23 returned, not -1)
{
	double fT = TmForHa( 0.);	// time of solar noon
	return (iRound( 12.-fT)+24)%24;
}		// SLRDAY::SolarTimeShift
//---------------------------------------------------------------------------
float SLRDAY::ExtRadHoriz(			// extraterrestrial horiz surface irradiation
	double ha1,			// beg hour angle, radians
	double ha2) const	// end hour angle, radians (>= ha1)
// Method: Duffie and Beckman, 1991, eqn 1.10.4
// returns integrated extraterrestrial irradiation on horiz surface, Btu/ft2
{
	if (ha2 > sd_haSunset)
		ha2 = sd_haSunset;
	if (ha1 > ha2)
		ha1 = ha2;
	else if (ha1 < -sd_haSunset)
		ha1 = -sd_haSunset;
	double fx = (12./kPi) * (sd_cdcl*(sin( ha2)-sin( ha1)) + (ha2-ha1)*sd_sdsl);
	return float( fx * sd_extBm);
}		// SLRDAY::ExtRadHoriz
//-------------------------------------------------------------------------
float SLRDAY::KT(			// daily clearness index
	float H) const		// daily total irradiation, Btu/ft2
// returns clearness index (0 - 1)
{
	return sd_H0 > 0.f ? H / sd_H0 : 0.f;
}		// SLRDAY::KT
//-------------------------------------------------------------------------
float SLRDAY::MonthDiffuseFract(		// long-term average diffuse fraction
	float KTBar) const	// average clearness index for month
// Implements Erbs correlation, per Duffie and Beckman (1991), section 2.12
//   NOTE: current day must be mean day for month (so sd_H0 = monthly mean)
// returns fraction of HBar that is diffuse
{
	float Kt = bracket( .3f, KTBar, .8f);
	float Fd = sd_haSunset <= RAD( 81.4f)
		? 1.391f - 3.560f*Kt + 4.189f*Kt*Kt - 2.137f*Kt*Kt*Kt
		: 1.311f - 3.022f*Kt + 3.427f*Kt*Kt - 1.821f*Kt*Kt*Kt;
	return Fd;
}		// SLRDAY::MonthDiffuseFract
//-----------------------------------------------------------------------------
void SLRDAY::HourFracts(		// hourly / daily ratios
	double haFSN,		// hour angle away from solar noon, radians
						//   (always > 0)
	float& rt,			// returned: hourly total / daily total
	float& rd) const	// returned: hourly diffuse / daily diffuse
// method: D&B section 2.13
{
	if (haFSN > sd_haSunset)
		haFSN = sd_haSunset;	// will return 0 for both
	double top = cos( haFSN) - sd_cosSunset;
	rt = float( (sd_aCoeff + sd_bCoeff*cos( haFSN)) * top * kPi / (24.*sd_dCoeff));
	rd = float( kPi*top/(24.*sd_dCoeff));
}		// SLRDAY::HourFracts
//-----------------------------------------------------------------------------
void SLRDAY::DbDump(
	int oMsk,
	const TCHAR* hdg/*=""*/) const
{
	if (!DbShouldPrint( oMsk))
		return;
	DbPrintf( oMsk, _T("%sSLRDAY  mon=%d  doy=%d  slTm=%d  eqTm=%0.5f  haConst=%0.5f  slrNoon=%.2f slrTmShift=%d tauB/taud=%.3f/%.3f\n"),
			hdg, sd_iM+1, sd_doy, sd_sltm, sd_eqTime, sd_haConst, TmForHa( 0.f), SolarTimeShift(), sd_tauB, sd_tauD);
	DbPrintf( oMsk, _T("hr        ha     fUp     dc0     dc1     dc2     azm  dirN  difH  totH\n"));
	for (int iT=0; iT < 24; iT++)
		sd_H[ iT].DbDump( oMsk, WStrPrintf( _T("%2.2d"), iT).c_str());
	sd_HSlrNoon.DbDump( oMsk, _T("SN"));	// solar noon
}		// SLRDAY::DbDump
//------------------------------------------------------------------------------
void SLRHR::DbDump(
	int oMsk,				// mask
	const TCHAR* tag) const	// line tag (e.g. hour of day)
{
	DbPrintf( oMsk, _T("%s %9.5f %7.4f %7.4f %7.4f %7.4f %7.4f %5.1f %5.1f %5.1f\n"),
		tag, ha, fUp, dirCos[ 0], dirCos[ 1], dirCos[ 2], azm,
		radDir, radDif, radHor);
}		// SLRHR::DbDump
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRHR -- solar info for an hour, substruct of SLRDAY
/////////////////////////////////////////////////////////////////////////////
void SLRHR::Calc(				// calc derived values for hour
	SLRDAY* pSD)		// SLRDAY containing this SLRHR
{
	if (!IsSunUp())
	{	sinHa = 0.;
		cosHa = 0.;
		VZero( dirCos, 3);
		azm = 0.;
		radDir = 0.f;
		radDif = 0.f;
		radHor = 0.f;
	}
	else
	{ 	sinHa = sin( ha);
		cosHa = cos( ha);
		dirCos[ 0] = -pSD->sd_cosDecl * sinHa;
		dirCos[ 1] = pSD->sd_sdcl - pSD->sd_cdsl * cosHa;
		dirCos[ 2] = max( 0., pSD->sd_sdsl + pSD->sd_cdcl * cosHa);
						// max drops -1e-8 values at sunrise/sunset
		azm = DCToAzm( dirCos);

		// ASHRAE incident solar
		int iM = pSD->sd_iM;
		int iCSM = pSD->sd_iCSM;
		if (iCSM >= 2)
		{	// ASHRAE tau model
			radHor = ASHRAETauModel(
						iCSM==3,	// 1 iff tau 2013
						pSD->GetExtBm(), float( GetZenAngle()),
						pSD->sd_tauB, pSD->sd_tauD,
						radDir, radDif);
		}
		else
		{	radDir = SinAlt() < 0.01
					? 0.f
					: float( ASHRAEData[ iCSM][ iM].A
								* exp( -ASHRAEData[ iCSM][ iM].B / SinAlt()));
			radDif = ASHRAEData[ iCSM][ iM].C * radDir;
			radHor = float( radDir * CosZen() + radDif);
		}
	}
}		// SLRHR::Calc
//---------------------------------------------------------------------------
double SLRHR::RelAzm(			// sun-surface azimuth
	double srfAzm) const	// surface azimuth (deg, 0=N, 90=east,
// returns sun azimuth (radians) relative to surface normal
//   0 = sun normal to surface
//   positive cw looking down; -Pi - +Pi
//     Pi if sun is down
{
	return IsSunUp()
		? fmod( azm-RAD( srfAzm) + 2.5 * k2Pi, k2Pi) - kPi
		: kPi;
}			// SLRHR::RelAzm
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRSURF -- time-independent surface info
/////////////////////////////////////////////////////////////////////////////
SLRSURF::SLRSURF()
#if 1
{
#else
	: ss_oaSSD( NULL)
{
	ss_oaSSD.SetSLRSURF( this);
#endif
}		// SLRSURF::SLRSURF

//---------------------------------------------------------------------------
SLRSURF::SLRSURF(
	DWORD key,		// key re grouped surfaces (see MakeKey)
  	float azm,		// surf azimuth (deg, 0=N, 90=E, )
	float tilt,		// surf tilt (deg, 0=horiz up, 90=vert, 180=horiz down)
	float grRef,	// ground reflectivity as seen by surface
					//   (dimless, 0-1)
	int options /*=0*/)	// options (future)
#if 1
{
#else
		: ss_oaSSD( NULL)
{
	ss_oaSSD.SetSLRSURF( this);
#endif
	Init( key, azm, tilt, grRef, options);
}		// SLRSURF::SLRSURF
//---------------------------------------------------------------------------
void SLRSURF::Init(
	DWORD key,		// key re grouped surfaces (see MakeKey)
  	float azm,		// surf azimuth (deg, 0=N, 90=E, )
	float tilt,		// surf tilt (deg, 0=horiz up, 90=vert, 180=horiz down)
	float grRef,	// ground reflectivity as seen by surface
					//   (dimless, 0-1)
	int options /*=0*/)	// options (future)
{
	options;
	ss_key = key;
	ss_azm = azm;
	ss_tilt = tilt;

	SetOrWithSnap( ss_azm, ss_tilt, ss_dirCos, ss_sinAzm, ss_cosAzm, ss_sinTilt);

	ss_difX = float( (1. + CosTilt()) / 2.);
	SetGrRef( grRef);		// sets ss_grfX
}		// SLRSURF::Init
//-----------------------------------------------------------------------------
/*static*/ void SLRSURF::SLDirCos( 	// direction cosines from azm/tilt of a surface
  	float azm,			// surf azimuth (deg, 0=N, 90=E, )
	float tilt,			// surf tilt (deg, 0=horiz up, 90=vert, 180=horiz down)
    double dirCos[3] )	// returned: direction cosines
{
	double sinAzm, cosAzm, sinTilt;		// unused
	SetOrWithSnap( azm, tilt, dirCos, sinAzm, cosAzm, sinTilt);
}		// SLRSURF::SLDirCos
//-------------------------------------------------------------------
/*static*/ BOOL SLRSURF::SetOrWithSnap(		// set w/ snap
  	float& azm,			// surf azimuth (deg, 0=N, 90=E, )
						//   may be returned snapped
	float& tilt,		// surf tilt (deg, 0=horiz up, 90=vert, 180=horiz down)
						//   may be returned snapped
    double dirCos[3],	// returned: direction cosines
	double& sinAzm,		// returned: sin( azm)
	double& cosAzm,		// returned: cos( azm)
	double& sinTilt)	// returned: sin( tilt)
// returns TRUE iff input angle(s) have been snapped
{
	BOOL bSnap = SnapAngle( tilt, sinTilt, dirCos[ 2]) >= 0;
	if (sinTilt < .001)
	{	if (azm != 0.f)
			bSnap = TRUE;
		azm = 0.f;		// horiz: azm meaningless
	}

	if (SnapAngle( azm, sinAzm, cosAzm) >= 0)
		bSnap = TRUE;

	dirCos[ 0] = sinAzm * sinTilt;
	dirCos[ 1] = cosAzm * sinTilt;

	return bSnap;
}		// SLRSURF::SetOrWithSnap
//-----------------------------------------------------------------------------
/*static*/ int SLRSURF::SnapAngle(		// snap angle to 0, 45, 90, ...
	float& ang,		// angle, deg. 0 <= ang < 360
					//   may be returned snapped
	double& sinAng,	// returned: sin( ang) after any snap
	double& cosAng,	// returned: cos( ang) after any snap
	float tol /*=.3f*/)		// snap tolerance (default = 0.3 deg)
// returns -2: ang unchanged, sinAng/cosAng calc'd from ang
//         -1: ang already exactly on a snap value
//          0..7: idx of 45 incr snaps
{
// table of snap-to angles and exact values for sin()/cos()
const double R2 = sqrt( 2.)/2.;
static struct
{	float ang; double sinAng; double cosAng; }
snapTbl[] =
{	{	0.f,  0.,  1. },
	{  45.f,  R2,  R2 },
	{  90.f,  1.,  0. },
	{ 135.f,  R2, -R2 },
	{ 180.f,  0., -1. },
	{ 225.f, -R2, -R2 },
	{ 270.f, -1.,  0. },
	{ 305.f, -R2,  R2 },
	{ -999.f }
};
	int ret = -2;
	int i;
	for (i=0; snapTbl[ i].ang > -998.f; i++)
	{	if (fabs( ang - snapTbl[ i].ang) < tol)
		{	ang = snapTbl[ i].ang;
			sinAng = snapTbl[ i].sinAng;
			cosAng = snapTbl[ i].cosAng;
			ret = ang == snapTbl[ i].ang ? -1 : i;
			break;
		}
	}
	if (ret == -2)
	{	sinAng = sin( RAD( ang));
		cosAng = cos( RAD( ang));
	}
	return ret;
}		// SLRSURF::SnapAngle
//-------------------------------------------------------------------------
void SLRSURF::SetGrRef(			// set mbrs dependent on ground refl
	float grRef)	// ground reflectivity as seen by surface
					//   (dimless, 0-1)
{
	ss_grRef = grRef;
	ss_grfX = CalcGrfX( ss_grRef);
}		// SLRSURF::SetGrRef
//-------------------------------------------------------------------------
float SLRSURF::CalcGrfX(		// derive ground reflectance factor
	float grRef) const	// ground reflectance (0 - 1)
{	return float( bracket( 0.f, grRef, 1.f) * (1. - CosTilt()) / 2.);
}		// SLRSURF::CalcGrfX
//-------------------------------------------------------------------------
/*static*/ DWORD SLRSURF::MakeKey(
	float azm, float tilt, float grRef,		// actual surface characteristics
	float& ssAzm, float& ssTilt)			// returned: surface orientation for
											//   SLRSURF (0 - 360, whole degrees)
											//   note: grRef not altered (assumed to be 0-1)
// returns key for ss_Map lookup
{
	// canonicalize to 0 <= x < 360
	double dAzm  = fmod( double( azm)  + 3600., 360.);
	double dTilt = fmod( double( tilt) + 3600., 360.);

	// default: group by 1 deg
	ssAzm = float( fmod( floor( dAzm + .5), 360.));
	ssTilt = float( fmod( floor( dTilt + .5), 360.));

	return iRound( grRef*100.)*1000000 + DWORD( ssAzm)*1000 + DWORD( ssTilt);
}		// SLRSURF::MakeKey
//--------------------------------------------------------------------------
double SLRSURF::RelAzm(			// surface-sun azimuth
	double sunAzm) const	// sun azimuth (radians, 0=N, Pi/2=east,
// TODO: does not handle sun-not-up case? 2-15-08
//       only caller SLRSURFDAY::InitSim() sets result to PI if sun not up.
// returns relative azimuth (radians), -Pi - +Pi
{
	double relAzm = sunAzm - RAD( ss_azm);
	while (relAzm < -kPi)
		relAzm += k2Pi;
	return relAzm;
}		// SLRSURF::RelAzm
#if 0
//---------------------------------------------------------------------------
SLRSURFDAY* SLRSURF::GetSLRSURFDAY(
	int iCD) const	// loads calc day or IDHSSIM (for hourly simulator)
{	return ss_oaSSD.GetAtSafe( ISSD( iCD));	// IDHSSIM = -1
}		// SLRSURF::GetSLRSURFDAY
//---------------------------------------------------------------------------
SLRSURFDAY* SLRSURF::AddOrGetSLRSURFDAY(
	int iCD,	// loads calc day or IDHSSIM (for hourly simulator)
	SLRDAY* pSD /*=NULL*/)	// day, passed to AddOrGet
{	return ss_oaSSD.AddOrGet( ISSD( iCD), pSD);	// IDHSSIM = -1
}		// SLRSURF::GetSLRSURFDAY
#endif
//---------------------------------------------------------------------------
void SLRSURF::DbDump(
	int oMsk,
	const TCHAR* hdg/*=""*/) const
{
	if (!DbShouldPrint( oMsk))
		return;
#if 0
	int nSSD = ss_oaSSD.GetSize();
	DbPrintf( _T("%sSLRSURF key=%d  azm=%0.1f  tilt=%0.1f  grRef=%0.2f  nSSD=%d\n"),
			hdg, ss_key, ss_azm, ss_tilt, ss_grRef, nSSD);
	for (int i=0; i < nSSD; i++)
	{	DbPrintf( _T("  %2.2d"), i);
		SLRSURFDAY* pSSD = ss_oaSSD.GetAt( i);
		if (!pSSD)
			DbPrintf( _T(" (null)\n"));
		else
			pSSD->DbDump();
	}
#endif
}		// SLRSURF::DbDump
//===========================================================================
#if 0
COA_SLRSURF::COA_SLRSURF( RSPROJ* pProj /*=NULL*/)
	: COA< SLRSURF>( _T("SurfDay"), TRUE),		// no CM
	  ssa_pProj( pProj), ssa_curSLRLOC(), ssa_iLM( -1)
{
	ssa_Map.InitHashTable( 101);
}		// COA_SLRSURF::COA_SLRSURF
//-------------------------------------------------------------------------
/*virtual*/ COA_SLRSURF::~COA_SLRSURF()
{	Clear();
}		// COA_SLRSURF::~COA_SLRSURF
//-------------------------------------------------------------------------
void COA_SLRSURF::Clear()			// remove all
{
	ssa_Map.RemoveAll();
	DeleteAll();
	ssa_iLM = -1;
	ssa_curSLRLOC.sl_Init( 0.f, 0.f, 0.f, 0.f, 0);
}		// COA_SLRSURF::Clear
//-----------------------------------------------------------------------------
BOOL COA_SLRSURF::IsUsable() const
// returns TRUE if SLRSURFs can be used in calcs
{	return GetSize() > 0;		// basic test
}		// COA_SLRSURF::IsUsable
//-----------------------------------------------------------------------------
SLRSURF* COA_SLRSURF::FindOrAdd(			// look for associated SLRSURF, add if needed
	float azm,		// azmimuth, deg, 0=N, 90=E,
	float tilt,		// tilt deg, 0=horiz up (ceiling), 90=vert, 180=horiz down (floor)
	float grRef,	// ground reflectance seem by this surface (0 - 1), typically .2
	int options/*=0*/)	// options (passed to SLRSURF::Init)
// returns ptr to SLRSURF
//         NULL on error (unexpected)
{
	grRef = bracket( 0.f, grRef, 1.f);		// insurance
	float ssAzm, ssTilt;
	DWORD key = SLRSURF::MakeKey( azm, tilt, grRef, ssAzm, ssTilt);
	int iSS = 0;
	if (!ssa_Map.Lookup( key, iSS))
	{	SLRSURF* pS = new SLRSURF();
		pS->Init( key, ssAzm, ssTilt, grRef, options);
		iSS = Add( pS, WRN|coaALLOWDUPS);	// coaALLOWDUPS = faster
		if (iSS >= 0)
			ssa_Map.SetAt( key, iSS);
	}
	return GetAtSafe( iSS);
}			// COA_SLRSURF::FindOrAdd
//---------------------------------------------------------------------------
void COA_SLRSURF::InitCalcDaysIf(
	SLRDAY* SD[],		// SLRDAY* for each day to be calc'd
	int nCD,			// # of calc days
	int iLM,			// current CALCMETH
	int oMsk/*=0*/)		// DbPrintf mask, re calclog-ing
// NOTE: SLRDAYs must have coomon SLRLOC
{
	if (nCD <= 0)
		return;		// no days, nothing to do
	int iCD;
#if defined( _DEBUG)
	for (iCD=0; iCD < nCD; iCD++)
	{	if (SD[ iCD]->GetSLRLOC() != SD[ 0]->GetSLRLOC())
			mbIErr( _T("COA_SLRSURF::InitCalcDaysIf"), _T("Inconsistent SLRLOC (%d)"), iCD);
	}
#endif
	// detect CALCMETH or SLRLOC change (must fully re-init)
	BOOL bNewLoc =
			   iLM != ssa_iLM
			|| !SD[ 0]->GetSLRLOC()->IsEqual( ssa_curSLRLOC);
	if (bNewLoc)
	{	// update current values
		ssa_iLM = iLM;
		ssa_curSLRLOC.Copy( *SD[ 0]->GetSLRLOC());
	}

	// loop SLRSRFs, init
#if 0
	for (int iSS=0; iSS<GetSize(); iSS++)
	{	SLRSURF* pSS = GetAt( iSS);
		for (iCD=0; iCD < nCD; iCD++)
		{	SLRSURFDAY* pSSD = pSS->ss_oaSSD.AddOrGet( SLRSURF::ISSD( iCD), SD[ iCD]);
			pSSD->InitDayIf( 1, bNewLoc ? SD[ iCD] : NULL);	// options=1: init for loads calc
															//   force full init if bNewLoc
		}
		if (oMsk)
			pSS->DbDump( oMsk);
	}
#endif
}		// COA_SLRSURF::InitCalcDaysIf
//===========================================================================
#endif


/////////////////////////////////////////////////////////////////////////////
// class SLRSURFDAY -- summarized solar info for a surface for day
// class COA_SLRSURFDAY -- collection of SLRSURFDAYs
/////////////////////////////////////////////////////////////////////////////
SLRSURFDAY::SLRSURFDAY( SLRSURF* pSS/*=NULL*/, SLRDAY* pSD/*=NULL*/)
: ssd_pSS( pSS)
{
	Init( -1, pSD);		// init most to 0
						//   WHY: members selectively used re HS, loads, etc.
						//        uninited mbrs cause trouble for e.g. test compares
}		// SLRSURFDAY::SLRSURFDAY
//---------------------------------------------------------------------------
void SLRSURFDAY::Init(
	int options,	// options:
					// -1: clear all for c'tor (mostly 0)
					//  0: geometric init only (call InitDayIf later)
					//	1: init for ASHRAE (load) calcs
					//  2: init for simulator calcs
					//	3: both
	SLRDAY* pSD/*=NULL*/,	// current day (SLRLOC must have been called
							//   ssd_pSD not changed if NULL
	float grRef /*=-1.f*/)	// day-specific ground reflectivity as seen by surface
							//   dimless, 0-1; <0 = use SOLARSRF.ss_grRef
{
	ssd_doy = -1;			// force InitDayIf
	if (options == -1)
	{	ssd_pSD = pSD;	// NULL or not per caller
		// ssd_pSS; leaved, done in c'tor
		ssd_doy = -1;
		ssd_grRef = -1.f;
		ssd_grfX = 0.f;
		ssd_shgfDifX = 0.f;
		ssd_shgfMax = 0.f;
		ssd_shgfShdMax = 0.f;
		ssd_sgxDif = 0.;
		ssd_iHSgxDirBeg = -1;
		ssd_iHSgxDirEnd = -1;
		for (int i=0; i<24; i++)
		{	ssd_cosInc[ i] = 0.;
			ssd_angInc[ i] = 0.;
			ssd_angProfV[ i] = 0.;
			ssd_angProfH[ i] = 0.;
			ssd_gamma[ i] = 0.;
			ssd_cosIncX[ i] = 0.;
			ssd_cosZenX[ i] = 0.;
			ssd_gammaX[ i] = 0.;
			ssd_sgxDir[ i] = 0.;
			ssd_rad[ i].Clear();
		}
		return;
	}

	if (pSD)
		ssd_pSD = pSD;		// InitDay will set, insurance

	// diffuse trans/abs: not orientation dependent
	float trans;
	float abso;
#if defined( REFGLSDOE2)
	RefGlsDOE2( -1., trans, abso);
#else
	RefGlsASHRAE( -1., trans, abso);
#endif
	ssd_shgfDifX = trans + 0.267f * abso;

	if (options)
		InitDayIf( options, pSD, grRef);
	else
		SetDayGrRef( grRef, 0);
}		// SLRSURFDAY::Init
//--------------------------------------------------------------------------
BOOL SLRSURFDAY::InitDayIf( 	// conditionally initialize day-dependent values
	int options,			// options: 1: init for ASHRAE (load) calcs
							//          2: init for simulator calcs
							//			3: both
	SLRDAY* pSD/*=NULL*/,	// current day (SLRLOC must have been called)
							//   ssd_pSD not changed if NULL
							// non-NULL forces init, use this when
							//   SLRLOC etc changes)
	float grRef /*=-1.f*/)	// day-specific ground reflectivity as seen by surface
							//   dimless, 0-1; <0 = use SOLARSRF.ss_grRef

// returns FALSE if error (missing SLRSURF or SLRDAY)
//         TRUE iff OK (successful init or doy unchanged)

{
	if (!ssd_pSS)
		return mbIErr( _T("SLRSURFDAY::InitDayIf"), _T("NULL ssd_pSS"));
	if (pSD)
	{	ssd_pSD = pSD;
		ssd_doy = -1;		// new SD, force calc
	}
	else if (!ssd_pSD)
		return mbIErr( _T("SLRSURFDAY::InitDayIf"), _T("NULL ssd_pSD"));

	if (ssd_doy == ssd_pSD->sd_doy)
		return TRUE;

	ssd_doy = ssd_pSD->sd_doy;

	// common hourly values
	for (int iT=0; iT < 24; ++iT)
	{	if (!SolarHr( iT).IsSunUp())
		{	ssd_cosInc[ iT] = -1.;
			ssd_angInc[ iT] = kPiOver2;
		}
		else
		{	
#if 1	// bug fix (saw dot prod *very* slightly > 1), 4-3-2013
			ssd_cosInc[ iT] = bracket( -1., DotProd3( SolarHr( iT).dirCos, DirCos()), 1.);
#else
x			ssd_cosInc[ iT] = DotProd3( SolarHr( iT).dirCos, DirCos());
#endif
			ssd_angInc[ iT] = acos( max( 0., ssd_cosInc[ iT]));
		}
	}

	SetDayGrRef( grRef, options);

	return TRUE;
}		// SLRSURFDAY::InitDayIf
//-------------------------------------------------------------------------
void SLRSURFDAY::SetDayGrRef(		// set mbrs dependent on ground refl
	float grRef,			// day-specific ground reflectivity as seen by surface
							//   dimless, 0-1; <0 = use SOLARSRF.ss_grRef
	int options/*=0*/)		// options: 1: init for ASHRAE (load) calcs
							//          2: init for simulator calcs
							//			3: both
{
	ssd_grRef = grRef;
	ssd_grfX = ssd_pSS->CalcGrfX( ssd_grRef);	// 0 if ssd_grRef < 0

	if (options & 1)
		InitASHRAE();		// uses GrfX();
	if (options & 2)
	{	InitSim();
		RefGlsGainSetup();	// uses GrfX();
	}
}		// SLRSURFDay::SetDayGrRef
//-------------------------------------------------------------------------
float SLRSURFDAY::GrRef() const			// ground reflectance
// returns ground reflectance, possibly day-specific (else from SLRSURF)
{	return ssd_grRef < 0.f
			? ssd_pSS->ss_grRef		// use surface default
			: ssd_grRef;
}		// SLRSURFDAY::GrRef
//-------------------------------------------------------------------------
float SLRSURFDAY::GrfX(				// ground reflectance factor
	float grRef/*=-1.f*/) const	// alternative ground reflectance
								//   (does not alter *this)
// returns ground reflectance factor, possibly day-specific (else from SLRSURF)
{	return grRef >= 0.f     ? GetSLRSURF()->CalcGrfX( grRef)	// use alternative
         : ssd_grRef < 0.f	? GetSLRSURF()->ss_grfX					// use surface default
		 :                    ssd_grfX;					// use day-specific
}		// SLRSURFDAY::GrfX
//--------------------------------------------------------------------------
void SLRSURFDAY::InitASHRAE()		// initial mbrs req'd for ASHRAE load calcs
{
	IncRadASHRAE();		// uses GrfX()
	ssd_shgfMax = -999.f;
	ssd_shgfShdMax = -999.f;
	for (int iT=0; iT < 24; ++iT)
	{	ssd_gamma[ iT] = SolarHr( iT).RelAzm( Azm());
		ssd_angProfH[ iT] = bracket( -kPiOver2, ssd_gamma[ iT], kPiOver2);
		float shgfShd;
		float shgf = CalcSHGF( iT, shgfShd);
		if (shgf > ssd_shgfMax)
			ssd_shgfMax = shgf;
		if (shgfShd > ssd_shgfShdMax)
			ssd_shgfShdMax = shgfShd;
	}
}			// SLRSURFDAY::InitASHRAE
//-----------------------------------------------------------------------
void SLRSURFDAY::IncRadASHRAE()		// calc incident solar radiation
// sets ssd_rad[] (Btuh/ft2) per ASHRAE HOF 1997 methods
{
	for (int iT=0; iT<24; iT++)
	{	if (!SolarHr( iT).IsSunUp())
		{	ssd_rad[ iT].Clear();
			continue;
		}

		ssd_rad[ iT].dir = ssd_cosInc[ iT] > 0.
				? float( ssd_cosInc[ iT] * SolarHr( iT).radDir)
				: 0.f;

		if (fAboutEqual( Tilt(), 90.f))
		{	float Y = float( ssd_cosInc[ iT] > -0.20
							? 0.55 + 0.437*ssd_cosInc[ iT]
							       + 0.313*ssd_cosInc[ iT]*ssd_cosInc[ iT]
							: 0.45);
			ssd_rad[ iT].dif = float( SolarHr( iT).radDif * Y);
		}
		else
			ssd_rad[ iT].dif = SolarHr( iT).radDif * DifX();

		ssd_rad[ iT].grf = SolarHr( iT).radHor * GrfX();
	}
}		// SLRSURFDAY::IncRadASHRAE
//-----------------------------------------------------------------------
void SLRSURFDAY::InitSim()			// init for simulator

// NOTES --
// ssd_cosIncX[ iH] receives the approx average cosine of angle of incidence of
// solar beam on surface during the hour.  If the sun is not above the surface
// for the entire hour, this is the average over the fraction of the hour
// that surface is exposed to the sun, times (weighted by) that fraction.
//
// This value is intended for direct use with solar beam value.  It is
// adjusted (reduced) for time sun is behind the surface but *NOT* adjusted
// for time sun is below horizon, since solar beam data already includes that
// effect.
//
// ssd_cosZenX[ iH] and ssd_gammaX[ iH] are required for calling FSunlit
// and are evaluated at the approximate mid point of the time the sun
// is above the surface.  They should thus be consistent with ssd_cosIncX[ iH].
//
// Note that SLRDAY::Setup() must be called for desired day prior to this.
{
	for (int iH=0; iH < 24; iH++)
	{	int iT1 = iH;			// iT for beg of hour
		int iT2 = (iH+1)%24;	// iT for end of hour

		// if sun below surf, no need to calculate
		if (SolarHr( iT1).fUp < ABOUT0		// sun below horizon
		 || (ssd_cosInc[ iT1] < 0. && ssd_cosInc[ iT2] < 0.))	// sun below surf
		{	ssd_cosIncX[ iH] = 0.;
			ssd_gammaX[ iH] = kPi;	// arbitrary behind surface value
			ssd_cosZenX[ iH] = 0.;	// zenith angle: 0
									//   NOTE: 0 even when sun above horizon
			continue;
    	}

		// working copies of cos( inc) at hour beg/end
		double c1 = ssd_cosInc[ iT1];
		double c2 = ssd_cosInc[ iT2];

		// Note: if sun above horizon only part of hour, SLRDAY::Setup2 put
		// sunrise/sunset dirCos at the hour limits, so values computed here
		// are average positions over time sun above horizon in hour
		// (code elsewhere may have to multiply ENERGY values by fUp[ iH])

		// Derive weighting factors for calculating averaged/weighted results
		double faz1, faz2;	// for azimuth: half way between beg, end of time
	   						//   above surf
		double fc1, fc2;	// for cosine of incident angle: also weight for
		   					//   fraction of hour sun above surface in same
							//   computation; use the fact that cosi is 0 at
							//   rise/set (vs surface).
		if (c1 >= 0. && c2 >= 0.)		// if above surface entire hour
			faz1 = fc1 = fc2 = 0.5;		//   use equal parts of start, end
		else if (c1 >= 0.)		// if sets (re surface) during hour
		{	fc1 = 0.5 * c1/(c1-c2);
			// fractn of starting cosi to use is avg of 2 items (other cosi
			// is 0) times fractn of hour sun above surf: weight cosi result
			// for duration
			fc2 = 0.;			// other input to cosi average is 0 at
	   						  	// sunset (use none of negative c2)
			// faz2 = fc1  falls out of  faz2 = 1. - faz1  below:
			//  if sun up whole hour, use 1/2 ending azm, else less
			faz1 = 1.- fc1;		// and the rest starting azm to get
								//  (start + set) / 2
		}
		else // c2 >= 0.	// sun rises over surface during hour
		{	fc1 = 0.;		// use none of start-hr cosi (below surface)
			faz1 = fc2 = 0.5 * c2/(c2-c1);
						 	// fract of starting azimuth and fract ending cosi
							// = avg * times fract of hr sun above surf
			// faz2 = 1. - faz1 is below
		}

		ssd_cosIncX[ iH] = fc1 * c1 + fc2 * c2;
		faz2 = 1. - faz1;
		double sunAzm
			= faz1 * SolarHr( iT1).azm + faz2 * SolarHr( iT2).azm;
		ssd_gammaX[ iH] = RelAzm( sunAzm);
		ssd_cosZenX[ iH]
			= faz1 * SolarHr( iT1).CosZen() + faz2 * SolarHr( iT2).CosZen();
	}
}		// SLRSURFDAY::InitSim
//--------------------------------------------------------------------------
float SLRSURFDAY::IRadSim(		// incident solar for sim hr
	int iH,			// hour
	float glrad,	// global horiz (used only re ground refl)
	float bmrad,	// beam normal
	float dfrad,	// diff horiz
	float grRef /*=-1.f*/)
{
	double r = bmrad * ssd_cosIncX[ iH]
		     + dfrad * DifX()
			 + glrad * GrfX( grRef);
	return float( r);
}		// SLRSURFDAY::IRadSim
//--------------------------------------------------------------------------
float SLRSURFDAY::CalcSHGF(			// ASHRAE solar heat gain factor
	int iT,				// time (0=midnight, 12=noon; time base per
						//   SLRDAY::Setup)
	float& shgfShd)		// returned: shaded shgf (no direct)
// returns SHGF = gain (Btuh/ft2) transmitted by reference DSS glass
//   for day/time
{
	if (ssd_rad[ iT].Tot() < 0.01f)
		return 0.f;

	float trans, abso;
#if defined( REFGLSDOE2)
	RefGlsDOE2( ssd_cosInc[ iT], trans, abso);
#else
	RefGlsASHRAE( ssd_cosInc[ iT], trans, abso);
#endif
	shgfShd = ssd_shgfDifX * (ssd_rad[ iT].dif + ssd_rad[ iT].grf);
	return (trans + 0.267f * abso) * ssd_rad[ iT].dir + shgfShd;
			// 0.267 = ASHRAE Ni (inward flowing fraction)
}		// SLRSURFDAY::CalcSHGF
//-----------------------------------------------------------------------------
float SLRSURFDAY::GetSHGF(		// retrieve SHGF (solar heat gain factor)
	float& shgfShd) const	// returned: peak shaded gain (no direct), Btuh/ft2)
// ASHRAE SHGF = peak hour gain through single pane double-strength glass
//   (see also SURFACE::GetPSF() for ACCA methods)
// returns SHGF (Btuh/ft2)
{
	shgfShd = ssd_shgfShdMax;
	return ssd_shgfMax;
}		// SLSSURFDAY::GetSHGF
//--------------------------------------------------------------------------
void SLRSURFDAY::RefGlsGainSetup()
{
 	// diffuse gain factor = ref glass gain/ft2 per unit diffuse irradiance
	ssd_sgxDif = ssd_shgfDifX * (DifX() + GrfX());

	// direct gain factor = ref glass gain/ft2 per unit direct normal irrad
	ssd_iHSgxDirBeg = -1;		// idx range of nz values for AccumGain() (below)
	ssd_iHSgxDirEnd = -1;
	for (int iH=0; iH < 24; iH++)
	{	ssd_sgxDir[ iH] = ssd_shgfDifX * GrfX() *
			(SolarHr( iH).CosZen() + SolarHr((iH+1)%24).CosZen()) / 2.;
							// ground reflected due to direct, use cos( zen)
							//   for midpoint of hour (0 when sun down)
		if (ssd_cosIncX[ iH] > 0.)		// sun above surface
		{	float trans, abso;
#if defined( REFGLSDOE2)
			RefGlsDOE2( ssd_cosIncX[ iH], trans, abso);
#else
			RefGlsASHRAE( ssd_cosIncX[ iH], trans, abso);
#endif
			ssd_sgxDir[ iH] += ssd_cosIncX[ iH] * (trans + 0.267 * abso);
					// 0.267 = ASHRAE Ni (inward flowing fraction)
		}
		if (ssd_sgxDir[ iH] > 0.)
		{	if (ssd_iHSgxDirBeg < 0)
				ssd_iHSgxDirBeg = iH;
			if (ssd_iHSgxDirEnd < iH)
				ssd_iHSgxDirEnd = iH+1;
		}
	}
}		// SLRSURFDAY::RefGlsGainSetup
//--------------------------------------------------------------------------
void SLRSURFDAY::AccumGain(			// accumulate gain for sim setup
	double gainF[ 25],		// gain factors, returned updated
							//  [0-23] = direct gain by hour
							//  [24] = diffuse gain
	double areaSC,			// area * SC product
	float ohDepth,			// overhang depth (ft) (from plane of glazing)
	float ohWD,				// overhang-window distance
	float wHgt)				// window height
{

	for (int iH=ssd_iHSgxDirBeg; iH < ssd_iHSgxDirEnd; iH++)
		gainF[ iH]
			+= areaSC * FSunlit( iH, ohDepth, ohWD, wHgt) * ssd_sgxDir[ iH];
	gainF[ 24] += areaSC * ssd_sgxDif;
}			// SLRSURFDAY::AccumGain
//---------------------------------------------------------------------------
float SLRSURFDAY::FSunlit(				// sunlit fraction
	int iT,					// time (0-23)
							//    ASHRAE: 0=midnight, 1=1AM,
							//    Sim: 0 = 0-1AM, 1=1-2AM,
	float ohDepth,			// overhang depth (ft) (from plane of glazing)
	float ohWD,				// overhang-window distance
	float wHgt,				// window height
	int options /*=0*/,		// option bits
							//   1: nz = do ASHRAE calcs (else sim)
	int* pHow /*=NULL*/)	// optionally returned:
							//   -1 = sun behind surface (fSunlit=1)
							//    0 = no overhang (fSunlit=1)
							//    1 = overhang exists, fSunlit calculated
// returns fraction sunlit (0 - 1)

{
static OHFIN ohfin = { 0.f };		// all 0

	float fSunlit = 1.f;
	int how = ohDepth > 0.01f;		// 0 or 1
	if (how == 1)
	{	BOOL bASHRAE = options & 1;
		double gamma = bASHRAE ? ssd_gamma[ iT] : ssd_gammaX[ iT];
		if (fabs( gamma) > kPiOver2)
		{
#if 1	// more useful return for behind-surface case, 2-14-08
			fSunlit = 1.f;		// sun behind surface
#else
x			fSunlit = 0.f;		// sun behind surface
#endif
			how = -1;
		}
		else
		{	double cosz = bASHRAE ? SolarHr( iT).CosZen() : ssd_cosZenX[ iT];
			ohfin.wheight = wHgt;
			ohfin.ohdepth = ohDepth;
			ohfin.ohwd = ohWD;
			ohfin.wwidth  = 5.f;		// width ... any value is OK
			ohfin.ohlx = 100.f;			// infinite overhang
			ohfin.ohrx = 100.f;
			fSunlit = ShadeWin( &ohfin, float( gamma), float( cosz));
#if defined( TESTSHADING)
			float fSunlitOld = shadwin( &ohfin, float( gamma), cosz);
			if ( fabs( fSunlit - fSunlitOld) > 0.001f)
			mbIErr( "SLRSURFDAY::FSunlit", "Fraction disagreement: new = %.3f, old = %.3f",
					fSunlit, fSunlitOld);
#endif
		}
	}
	if (pHow)
		*pHow = how;
	return fSunlit;
}		// SLRSURFDAY::FSunlit
//-------------------------------------------------------------------------
float SLRSURFDAY::SolAirTemp(		// surface sol-air temp at specific time
	int iT,					// time (0-23) 0=midnight, 1=1AM,
	float tDb,				// outdoor dry-bulb temp at time iT, F
	float abs,				// surface solar absorptance (0-1)
	float hOut,				// combined radiant/convective surface conductance
							//   Btuh/ft2-F
	float fSunlit/*=1.f*/,	// fraction sunlit (0 - 1) (1 = full direct,
							//   0 = no direct)
	float lwCor/*=-999.f*/) const
							// long wave temp correction, F (generally <= 0)
							//   = - LW emittance * deltaR / hOut)
							//   default = 0 for vertical, -7 for horizontal
							//   (see 2005 HOF p. 30.22, Eqn 28 etc)
{
	hOut = max( .01f, hOut);	// insurance
	if (lwCor < -998.f)
		// default per ASHRAE assumptions
		//   -7 for horizontal, 0 for vertical (or beyond)
		//   interpolate using sky view factor for other tilts
		lwCor = float( -7. * 2. * max( 0., DifX()-.5));
	float absOverH = abs / hOut;
	return tDb + absOverH * ssd_rad[ iT].Tot( fSunlit) + lwCor;
}		// SLRSURFDAY::SolAirTemp
//-------------------------------------------------------------------------
void SLRSURFDAY::SolAirTemp(		// surface sol-air temp (24 hours)
	double tSolAir[ 24],	// returned: hourly sol-air temp for surface, F
							//    NOTE: idx'd by loads hour (0 = 1 AM)
	const float tDb[ 24],	// hourly outdoor dry-bulb temp, F
							//    NOTE: idx'd by loads hour (0 = 1 AM)
	float abs,				// surface solar absorptance (0-1)
	float hOut,				// combined radiant/convective surface conductance
							//   Btuh/ft2-F
	float fSunlit,			// fraction sunlit (0 - 1)
							//   if <0, use hourly values (next arg)
	const float* pHrFSunlit /*=NULL*/,	// hourly fraction sunlit (0 - 1),
										//    NOTE: idx'd by time (0 = midnight)
	float lwCor/*=-999.f*/) const		// see prior SolAirTemp() variant
{
	if (fSunlit < 0.f && !pHrFSunlit)
	{
#if defined( _DEBUG)
		mbIErr( _T("SLRSURFDAY::SolAirTemp"), _T("NULL pHrFSunlit"));
#endif
		fSunlit = 1.f;		// crash proof
	}
	float fSunlitX( fSunlit);		// daily fSunlit (or < 0)
	for (int iH=0; iH<24; iH++)
	{	int iT = ITForIH( iH);
		if (fSunlit < 0.f)
			fSunlitX = pHrFSunlit[ iT];		// hourly fSunlit
		tSolAir[ iH] = SolAirTemp( iT, tDb[ iH], abs, hOut, fSunlitX, lwCor);
	}
}		// SLRSURFDAY::SolAirTemp
//-------------------------------------------------------------------------
void SLRSURFDAY::RadRatios(		// f-chart irradiation ratios for surface
	float HBar,		// average daily horiz irradiation, Btu/ft2
	float& KTBar,	// returned: HBar / H0Bar
	float& RBar,	// returned: HTBar / HBar
					//   daily total surf / daily total horiz
					//   derived using KT method, D&B section 2.20
	float& Rn,		// returned: noon surf / noon horiz
	float& rtn)		// returned: noon hour fraction (I-noon / HBar)

// RBar: "K-T" method per Duffie and Beckman, section 2.20
// Rn: geometric calc
{
	KTBar = -1.f;
	RBar = -1.f;
	Rn = -1.f;
	rtn = -1.f;

	SLRSURF* pS = GetSLRSURF();
	SLRDAY* pD = GetSLRDAY();
	if (!pS || !pD)
		return;
	SLRLOC* pL = pD->GetSLRLOC();
	if (!pL)
		return;

	float rdn;
	pD->HourFracts( 0., rtn, rdn);	// noon ratios
	KTBar = pD->KT( HBar);
	float fDiff = pD->MonthDiffuseFract( KTBar);
	if (pS->IsHoriz())
	{	RBar = 1.f;		// exactly horizontal
		Rn = 1.f;
	}
	else
	{	// calc order per Duffie and Beckman example 2.20
		double A = pS->CosTilt() + pL->TanLat() * pS->CosAzmS() * pS->SinTilt();

		double B = pD->sd_cosSunset*pS->CosTilt()
				 + pD->TanDecl()*pS->SinTilt()*pS->CosAzmS();

		double C = pS->SinTilt()*pS->SinAzmS() / max( .001, pL->sl_cosLat);

		double A2C2 = A*A + C*C;
#if 1	// fix sun-never-rises bug, 9-26-08
		double haSS_H = pD->sd_haSunset;		// horiz surface sunset hour angle
		double haSR = 0.;
		double haSS = 0.;
		if (B*B < A2C2)
		{	double CSQR = C * sqrt( A2C2 - B*B);
			double haT = acos( (A*B + CSQR)/A2C2);
			haSR = fabs( min( haSS_H, haT));
			haT = acos( (A*B - CSQR)/A2C2);
			haSS = fabs( min( haSS_H, haT));
			if ((A > 0. && B > 0.) || (A >= B))
				haSR = -haSR;
			else
				haSS = -haSS;
		}
		double D = 0.;
		if (fabs( haSS - haSR) > .001)	// if sun above surface for finite time
		{	double ap = pD->sd_aCoeff - fDiff;
			D = haSS >= haSR
				?   GFunc( haSS,   haSR,   A, B, C, ap)
				:   GFunc( haSS,  -haSS_H, A, B, C, ap)
				  + GFunc( haSS_H, haSR,   A, B, C, ap);
			if (D < 0.)
				D = 0.;
		}
#else
x		BOOL bSR1 = (A > 0. && B > 0.) || (A >= B);
x		double CSQR = C * sqrt( A2C2 - B*B);
x		float haSS_H = pD->sd_haSunset;		// horiz surface sunset hour angle
x
x		double haT = acos( (A*B + CSQR)/A2C2);
x		double haSR = fabs( min( haSS_H, haT));
x		if (bSR1)
x			haSR = -haSR;
x
x		haT = acos( (A*B - CSQR)/A2C2);
x		double haSS = fabs( min( haSS_H, haT));
x		if (!bSR1)
x			haSS = -haSS;
x		double ap = pD->sd_aCoeff - fDiff;
x		double D = haSS >= haSR
x			?   GFunc( haSS,   haSR,   A, B, C, ap)
x			:   GFunc( haSS,  -haSS_H, A, B, C, ap)
x			  + GFunc( haSS_H, haSR,   A, B, C, ap);
x		if (D < 0.)
x			D = 0.;
#endif	// 9-26-08

		float rbDiff = fDiff*DifX();
		float rbGrRef = GrfX();

		RBar = float( D + rbDiff + rbGrRef);

		double RdRt = rdn * fDiff / rtn;
		Rn = float( (1.-RdRt)*RbSlrNoon() + RdRt*DifX() + GrfX());
	}

}		// SLRSURFDAY::RadRatios
//-------------------------------------------------------------------------
double SLRSURFDAY::GFunc(		// KT method integral
	double ha1, double ha2,			// integration limits (hour angles)
	double A, double B, double C,	// KT method A, B, C values
	double ap)						// a-prime (eqn 2.20.5d)
// Method: D&B Eqn. 2.20.5c
// returns D = integrated surface solar
{
	SLRDAY* pD = GetSLRDAY();

	double b = pD->sd_bCoeff;
	double d = pD->sd_dCoeff;

	double s1 = sin( ha1);
	double s2 = sin( ha2);
	double c1 = cos( ha1);
	double c2 = cos( ha2);

	double t1 = (b*A/2. - ap * B) * (ha1 - ha2);
	double t2 = (ap * A - b * B) * ( s1 - s2);
	double t3 = ap*C*(c1 - c2);
	double t4 = (b*A/2.)*(s1*c1 - s2*c2);
	double t5 = (b*C/2.)*(s1*s1 - s2*s2);

	return (1./(2.*d))*(t1 + t2 - t3 + t4 + t5);

}		// SLRSURFDAY::GFunc
//-------------------------------------------------------------------------
double SLRSURFDAY::RbSlrNoon() const
// returns ratio Ibn/Ibn,horiz
//   Note: could be generalized to any hour angle (make SLRHR on fly)
// 0 if sun not up relative to horizon or surface
{
	double Rb = 0.;
	const SLRDAY* pD = GetSLRDAY();
	if (pD)
	{	const SLRHR& HSN = pD->sd_HSlrNoon;
		if (HSN.IsSunUp())		// if sun above horizon (not arctic winter)
		{	double cosI = DotProd3( HSN.dirCos, DirCos());
			if (cosI > 0.)		// if sun above surface
				Rb = cosI / max( .00001, HSN.CosZen());
		}
	}
	return Rb;
}		// SLRSURFDAY::RbSlrNoon
//-------------------------------------------------------------------------
void SLRSURFDAY::DbDump() const		// debug print (unconditional)
{
#if 0	// enhanced format, 9-21-2011
	DbPrintf( _T(" %3.3d (%s)  %5.1f %5.1f   %5.3f\n"),
				ssd_doy, Calendar.FmtDOY( ssd_doy),
				ssd_shgfMax, ssd_shgfShdMax, ssd_shgfDifX);
	for (int iT=0; iT<24; iT++)
	{	DbPrintf( _T("   %2.2d  %5.1f  %5.1f  %5.1f    %5.1f    %5.1f\n"),
			iT, AngIncDeg( iT), AngProfHDeg( iT), AngProfVDeg( iT), IRadDir( iT), IRadDif( iT));
	}
#else
	// traditional format
	DbPrintf( _T(" %3.3d (%s)  %5.1f %5.1f   %5.3f\n"),
				ssd_doy, Calendar.FmtDOY( ssd_doy),
				ssd_shgfMax, ssd_shgfShdMax, ssd_shgfDifX);
#endif
}		// SLRSURFDAY::DbDump
//=========================================================================
#if 0
COA_SLRSURFDAY::COA_SLRSURFDAY( SLRSURF* pSS)
	: COA< SLRSURFDAY>( _T("SlrSurfDay"), TRUE),		// no CM
	  cssd_pSS( pSS)
{
}		// COA_SLRSURFDAY::COA_SLRSURFDAY
//-------------------------------------------------------------------------
/*virtual*/ COA_SLRSURFDAY::~COA_SLRSURFDAY()
{
}		// COA_SLRSURFDAY::~COA_SLRSURFDAY
//-------------------------------------------------------------------------
SLRSURFDAY* COA_SLRSURFDAY::AddOrGet( int iSSD, SLRDAY* pSD/*=NULL*/)
{
	SLRSURFDAY* pSSD = GetAtSafe( iSSD);
#if 0 && defined( _DEBUG)
x	if (iSSD == 0)
x		mbErr( "Yak");
#endif
	if (!pSSD)
	{	pSSD = new SLRSURFDAY( cssd_pSS);
		SetAtGrow( iSSD, pSSD);
	}
	if (pSD != pSSD->ssd_pSD)
		pSSD->Init( 0, pSD);		// different day, Init()
	return pSSD;
}	// COA_SLRSURFDAY::AddOrGet
//===========================================================================
#endif

#if 0
/////////////////////////////////////////////////////////////////////////////
// class SLRCALC: packaged classes useful for RSR solar calcs
//    Used both in Right-Energy simulator and hourly (AED) loads calcs
/////////////////////////////////////////////////////////////////////////////
SLRCALC::SLRCALC()		// c'tor
	: sc_SL(),
	  sc_SD( &sc_SL)
{
}		// SLRCALC::SLRCALC
//---------------------------------------------------------------------------
SLRCALC::SLRCALC(			// c'tor
	float lat,		// latitude (degN, <0 = southern hemisphere)
	float lng,		// longitude (degE, <0 = degW)
	float tzn,		// time zone (GMT +X, e.g. EST = -5, PST = -8,
					//	Paris = +1)
	float elev,		// elevation (ft above sea level)
	int options/*=0*/,	// option bits (for SLRLOC::Init())
						//   slropSIMPLETIME: use "simple" time scheme
						//     (no longitude/tzn in solar calcs (assumes long=tzn*15))
	int oMsk/*=0*/)		// DbPrintf mask, re calclog (for SLRLOC::Init())
	: sc_SL(), sc_SD( &sc_SL)
{
	sc_SL.sl_Init( lat, lng, tzn, elev, options, oMsk);
}		// SLRCALC::SLRCALC
//--------------------------------------------------------------------------
/*virtual*/ SLRCALC::~SLRCALC()		// d'tor
{
}		// SLRCALC::~SLRCALC
//--------------------------------------------------------------------------
BOOL SLRCALC::InitLOC(			// initialize re location
	float lat,		// latitude (degN, <0 = southern hemisphere)
	float lng,		// longitude (degE, <0 = degW)
	float tzn,		// time zone (GMT +X, e.g. EST = -5, PST = -8,
					//	Paris = +1)
	float elev,		// elevation (ft above sea level)
	int options/*=0*/,	// option bits (for SLRLOC::Init())
						//   slropSIMPLETIME: use "simple" time scheme
						//     (no longitude/tzn in solar calcs (assumes long=tzn*15))
	int oMsk/*=0*/)		// DbPrintf mask, re calclog (for SLRLOC::Init())
// returns TRUE iff SLRLOC changed due to this InitLOC
{
	SLRLOC saveSL( sc_SL);
	sc_SL.sl_Init( lat, lng, tzn, elev, 0, options, oMsk);
	return sc_SL != saveSL;
}		// SLRCALC::InitLOC
//===========================================================================
#endif

/////////////////////////////////////////////////////////////////////////////
// static and public functions
/////////////////////////////////////////////////////////////////////////////
float AirMass(			// air mass
	float sunAlt)	// solar altitude, radians
// Kasten and Young method as used in HOF 2009
// returns M, dimless
{
	float M;
	if (sunAlt <= 0.f)
		M = 100.f;
	else if (sunAlt >= kPiOver2)
		M = 1.f;
	else
	{	double sunAltD = DEG( sunAlt);
		M = float( 1./(sin( sunAlt) + 0.50572*pow( 6.07995 + sunAltD, -1.6364)));
	}
	return M;
}		// :: AirMass
//------------------------------------------------------------------------------
float ASHRAETauModel(	// ASHRAE "tau" clear sky model
	int options,		// option bits
						//	 1: use 2013/2017 coefficients (else 2009)
	float extBm,		// extraterrestrial normal irradiance, aka E0
						//   any suitable units, e.g. Btuh/ft2 or W/m2
	float sunZen,		// solar zenith angle, radians
	float tauB,			// ASHRAE beam "pseudo optical depth" (from database)
	float tauD,			// ditto diffuse
	float& radDirN,		// returned: direct normal irradiance, units per extBm
	float& radDifH)		// returned: diffuse horizontal irradiance, units per extBm

// calc is done for specific day and time implicit in extBm, sunZen, tauB, and tauD.
//   See 2009 HOF p. 14.9 and 2013 HOF p. 14.9

// returns radGlbH, units per extBm
{
	float radGlbH;
	float sunAlt = float( max( kPiOver2 - sunZen, 0.));
	if (sunAlt <= 0. || tauB <= 0.f || tauD <= 0.f)
	{	radDirN = 0.f;
		radDifH = 0.f;
		radGlbH = 0.f;
	}
	else
	{	double M = AirMass( sunAlt);
		double ab,ad;
		if (options & 1)
		{	// 2013 form
			ab = 1.454 - 0.406*tauB - 0.268*tauD - 0.021*tauB*tauD;
			ad = 0.507 + 0.205*tauB - 0.080*tauD - 0.190*tauB*tauD;
		}
		else
		{	// 2009
			ab = 1.219 - 0.043*tauB - 0.151*tauD - 0.204*tauB*tauD;
			ad = 0.202 + 0.852*tauB - 0.007*tauD - 0.357*tauB*tauD;
		}
		radDirN = float( extBm * exp( -tauB * pow( M, ab)));
		radDifH = float( extBm * exp( -tauD * pow( M, ad)));
		radGlbH = float( radDirN * cos( sunZen) + radDifH);
#if defined( _DEBUG)
		if (radDirN > .9 * extBm)
		{	mbIErr( _T("ASHRAETauModel"), _T("Dubious radDirN=%.f (extBm=%.f)"),
				radDirN, extBm);
			AirMass( sunAlt);		// debug aid
		}
#endif
	}
	return radGlbH;
}		// ::ASHRAETauModel
//-----------------------------------------------------------------------------
#if defined( REFGLSDOE2)
static void RefGlsDOE2(  	// Calculate trans/abs of ref glass, DOE method

	double eta,		// cosine of direct angle of incidence
					//   < 0 to return values for diffuse
	float& trans,	// receives transmissivity
	float& abso)	// receives absorptivity

// This function is based on DOE-2 (2.1B?)
{
static float c[10] =
{  -0.01325f,  3.08716f, -3.68232f,  1.48683f, 0.077366f,
   -0.02718f, -0.002691f, 0.040548f, 0.79901f, 0.05435f
};
	if (eta >= 0.)		// beam, eta is cosine incidence angle
	{	double eta2 = eta * eta;
		double eta3 = eta * eta2;
		trans = float( max( 0., c[0] + c[1]*eta + c[2]*eta2 +c[3] * eta3));
		abso  = float( max( 0., c[4] + c[5]*eta + (c[6]/( c[7]+eta))) );
	}
	else			// for diffuse radiation
	{  trans = c[ 8];
	   abso = c[ 9];
	}
}		// RefGlsDOE2
#endif
//---------------------------------------------------------------------------
#if !defined( REFGLSDOE2)
static void RefGlsASHRAE( 	// Calculate trans/abs of ref glass, ASHRAE HOF 1997

	double eta,		// cosine of direct angle of incidence
					//   < 0 to return values for diffuse
	float& trans,	// receives transmissivity
	float& abso )	// receives absorptivity

// Per ASHRAE HOF 1997, p 29.37
{
// coefficients, Table 23
static double t[ 6] =
{  -0.00885, 2.71235, -0.62062, -7.07329, 9.75995, -3.89922 };
static double a[ 6] =
{	0.01154, 0.77674, -3.94657, 8.57881, -8.38135, 3.0118  };

	double tX = t[ 0];
	double aX = a[ 0];
	if (eta >= 0.)
	{	double etaX = eta;
		for (int j=1; j < 6; j++)
		{	tX += t[ j] * etaX;
			aX += a[ j] * etaX;
			etaX *= eta;
		}
	}
	else	// diffuse
	{	for (int j=1; j < 6; j++)
		{	tX += 2. * t[ j] / (j+2);
			aX += 2. * a[ j] / (j+2);
		}
	}
	trans = float( tX);
	abso = float( aX);
}		// RefGlsASHRAE
#endif
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
void TestSHGF()
// generates table formatted as A89, Chapter 26, p 26.39
// (values match as of 1-10-08)
{
	static const TCHAR* shdTag[ 2] = { _T("sunlit"), _T("shaded") };
	SLRLOC sLoc;
	SLRDAY sDay( &sLoc);
	SLRSURF sSrf;
	SLRSURFDAY sSD( &sSrf, &sDay);
	float shgf[ 10];
	for (int iShd=0; iShd<2; iShd++)
	for (int iLat=0; iLat<17; iLat++)
	{	float lat = iLat*4.f;
		sLoc.sl_Init( lat, 0.f, 0.f, 0.f, 0);
		DbPrintf( _T("\nSHGF Lat=%0.0f (%s)\n---------------\n")
			_T("            NNE    NE   ENE     E   ESE    SE   SSE\n")
			_T("        N   NNW    NW   WNW     W   WSW    SW   SSW     S   HOR\n"),
			lat, shdTag[ iShd]);
		for (int iM=1; iM<13; iM++)
		{	sDay.SetupASHRAE( iM, slropCSMOLD, sltmSLR);
			for (int iOr=0; iOr<10; iOr++)
			{	float azm  = iOr < 9 ? iOr * 22.5f : 0.f;
			    float tilt = iOr < 9 ? 90.f        : 0.f;
				sSrf.Init( 0, azm, tilt, .2f);
				sSD.Init( 1);
				shgf[ iOr] = iShd ? sSD.ssd_shgfShdMax : sSD.ssd_shgfMax;
			}
			VDbPrintf( dbdALWAYS, Calendar.GetMonAbbrev( iM), shgf, 10, _T("%6.0f"));
		}
	}

}		// ::TestSHGF
#endif
//---------------------------------------------------------------------------
#if 0 // replaced by SLRSURFDAY::FSunlit(), retain for possible testing, 11-7-01
tint ShadeF(						// return sunlit fraction
t	SLRSURFDAY* pSRFD,		// solar info re surface
t	int iT,				// time
t	float ohDepth,		// overhang depth (ft) (from plane of glazing)
t	float ohWD,			// overhang-window distance
t	float wHgt,			// window height
t	float& shadeF)		// returned: fraction shaded
t
t// returns -1 = sun behind surface (shadeF=0)
t//          0 = no overhang (shadeF=0)
t//          1 = overhang exists, shadeF calculated
t
t{
tstatic OHFIN ohfin = { 0.f };		// all 0
t
t	shadeF = 0.f;
t#if	1	// reverse order of check, re compare to FSunlit, 11-07-01
t	if (ohDepth < 0.01f)
t		return 0;
t	double gamma = pSRFD->ssd_gamma[ iT];
t	if (fabs( gamma) > kPiOver2)
t		return -1;	// sun is behind surface
t#else
tx	double gamma = pSRFD->ssd_gamma[ iT];
tx	if (fabs( gamma) > kPiOver2)
tx		return -1;	// sun is behind surface
tx	if (ohDepth < 0.01f)
tx		return 0;
t#endif
t	ohfin.wheight = wHgt;
t	ohfin.ohdepth = ohDepth;
t	ohfin.ohwd = ohWD;
t	ohfin.wwidth  = 5.f;		// width ... any value is OK
t	ohfin.ohlx = 100.f;			// infinite overhang
t	ohfin.ohrx = 100.f;
t
t	float cosz = float( pSRFD->ssd_pSD->ssd_H[ iT].cosZen());
t
t	shadeF = 1.f - ShadeWin( &ohfin, float( gamma), cosz);
t#if defined( TESTSHADING)
t	float shadeFOld = 1.f - shadwin( &ohfin, float( gamma), cosz);
t	if ( fabs( shadeF - shadeFOld) > 0.001f)
t		mbIErr( "ShadeF", "Fraction disagreement: new = %.3f, old = %.3f",
t					shadeF, shadeFOld);
t#endif
t	return 1;
t}		// ::ShadeF
#endif // 11-7-01
//---------------------------------------------------------------------------
static float ShadeWin( 		// Calculate sunlit fraction of window shaded
							//   with fins and/or overhang
   const struct OHFIN *p,	// struct of dimensions of window/ovhg/fins
   float gamma,				// azimuth of sun less azimuth of window:
							//   angle 0 if sun normal to window,
							//   positive cw looking down (radians)
   float cosz)				// cosine of solar zenith angle

// window is rectangular, in vertical surface.
// 	sun assumed at/above horizon.
// 	overhang is horizontal rectangle covering full window width or more
// 	overhang may have "flap" hanging down at outer edge
// 	there may be one fin on each side of window.
// 	  top of fins is at/above top of window;
// 	  bottom of fins is at/above bottom of window (but below top).
//
// generally caller checks data --
//    this function does not verify most of its geometric assumptions.
//    Negative values are not expected in any of the arguments;
//      use 0 depth to omit a fin or overhang.

// returns: fraction of area sunlit, 0 to 1.f
{
//--------------------------------------------------------------------------
// Members of OHFIN structure and associated working variables
// member   variable    description
//  .wwidth     W      window width
//  .wheight    H      window height
//  .ohdepth           depth of overhang
//  .ohwd       U      distance from top of window to overhang
//  .ohlx       E*     distance overhang extends beyond left edge of window
//  .ohrx              distance overhang extends beyond right edge of window
//  .ohflap            depth of vertical projection at the end of overhang
//  .fldepth    FD*    depth of left fin
//  .fltx       FU*    distance left fin extends above top of window
//  .flwd       FO*    distance from left edge of window to left fin
//  .flwbx      FBU*   distance left fin stops short above bottom of window
//  .frdepth           depth of right fin
//  .frtx              distance right fin extends above top of window
//  .frwd              distance from right edge of window to right fin
//  .frwbx             distance right fin stops short above bottom of window
//            * set to left or right fin/overhang end per sun position.
// comment comments
//     "near" = toward sun (horizontally), "far" or beyond = away from sun.
//     "ray" = extended slanted shadow line from top of fin or near (sunward)
//             end of overhang.
//     _1 variables are for hor overhang,
//     _2  are for vertical flap at end of overhang,
//     _3  are for fin. */
// flow control
int K;		// 0 normal, 1 when doing fin-flap shadow overlap
int L;		// 1 if overhang horiz shadow edge is in window, else 0
// angles
float cosg, sing;	// sin and cos of gamma (angle from window normal
			//  to sun, + for sun to left when looking at window)
float ver, hor; 	// components of shadow on wall of unit projection
// window
float H, W;	// hite, wid of window (later of flap shadow)
float WAREA;	// whole window area
// overhang
float E, U;	// near (to sun) flap horiz extension; dist flap above win
float HU, WE;	// H+U;  W+E: Width + Extension
float X1, Y1;	// components hor oh shad slantEdge -- how far over & down
float AREAO;
float H2, W2;	// fin flap shadow hite and width
float AREAV;
// fin
float FD;	// fin depth: how far out fin projects
float FU;	// "fin up": fin top extension above top win
float FO;	// "fin over": how far sunward from window to fin
float FBU;	// "fin bottom up": how far above bot win fin ends
float X3;	// x compon fin shadow slantEdge: hor far over shadow goes
float ARintF, AREA1;		// shadow area & working copy
// shadow ray intercepts for oh then fin.
//  NB these relate to "ray"; may be beyond actual shadow corner
float NY, FY;	// how far down slant ray is at near, far win edge intercepts
// result
float fsunlit;

/* triangle area for specific altitude and global hor and ver components.
   Divide by 0 protection: most alt's are 0 if ver is 0. */
#define TRIA(alt) ( (alt)==(float)0.f ? (float)0.f : (alt)*(alt)*0.5f*hor/ver )
			/* the (float)s really are necessary to prevent data
			   conversion warnings in MSC 5.1. rob 1-24-90.*/

/* init */
	K = 0;			/* 1 later during fin-flap overlap */
	cosg = (float)cos(gamma);
	if (cosg <= 0.f)		// if sun behind wall
		return 0.f;			//   window fully in shadow
	sing = (float)sin(gamma);		/* ... solar azimuth rel to window */
	if (cosz >= 1.f)			/* sun at zenith: no sun enters win.*/
		return 0.f;			/* (rob 1-90 prevents div by 0) */
	   					/* following line compiles incorrectly
					   with default optimization -Oswr;
					   expression produces reciprocal of
					   correct result.  #pragma opt off
					   added above.  8-5-90 chip */
	ver = float( cosz/sqrt(1.-cosz*cosz)/cosg);	/* tan(zenith angle) / cos(gamma) */
	hor = (float)fabs(sing)/cosg;	/* abs(tan(gamma)): unit fin shade */
	AREAO = 0.f;  		/* hor overhang shadow area */
	AREAV = 0.f;  		/* ver overhang flap shadow area */
	ARintF = 0.f;  		/* fin shadow area */
	AREA1 = 0.f;  		/* working copies of above */
	H = p->wheight;
	W = p->wwidth;
	WAREA = H * W;		/* whole window area */
	W2 = H2 = 0.f;		/* init width & hite of oh flap shadow */
/* horiz overhang setup */
	E= gamma < 0.f ? p->ohrx : p->ohlx;	/* pertinent (near sun) oh extension*/
	U = p->ohwd; 			/* distance from top win to overhang*/
	HU = H + U;				/* from overhang to bottom window */
	WE = W + E;  			/* from near end oh to far side wind*/
	Y1 = p->ohdepth * ver;		/* how far down oh shadow comes */
	X1 = p->ohdepth * hor;	    	/* hor component oh shad slantedge
						   = hor displ corners oh shadow */
	/* NOTE: X1, Y1 are components of actual length (to corner) of shadow
		        slanted edge, based on oh depth.
	         NY, FY relate to slanted ray intercepts with window edges,
	            irrespective of whether beyond actual corner. */
	L = Y1 > U  &&  Y1 < HU  &&  X1 < WE;	/* true if near shad bot corn
							(hence hor edge) is in win */
	if (p->ohdepth <= 0.f)		/* if no overhang */
	   goto fin37;			/* go do fins.  flap is ignored. */

/*----- horizontal overhang shadow area "AREAO" -----*/

	if (Y1 > U			/* if shadow comes below top window */
	 && (hor==0.f ||		/* /0 protection */
	     (FY=WE*ver/hor) > U))	/* and edge passes below top far win corn */
			/* (set FY to how far down slant ray hits far side) */
	{  if (Y1 > HU)		/* chop shadow at bot win to simplify */
	      Y1 = HU;		/* (adjusting X1 to match doesn't matter
	      			   in any below-win cases) */
   /* area: start with window rectangle above shadow bottom */
	   AREAO = W * (Y1 - U);	/* as tho shadow corner sunward of window */

   /* subtract unshaded portions of rectangle if shad corner is in win */
	   if (hor != 0.f)		/* else done and don't divide by 0 */
	   {  NY = E * ver/hor;   	/* "near Y": distance down from oh to where
					   slant ray hits near (to sun) win edge */
	  /* if corner not sunward of window, subtract unshaded triangle area */
	      if (NY < Y1)  		/* if slant hits nr side abov sh bot*/
	      {  AREAO -= TRIA(Y1 - NY); 	/* triangle sunward of edge */
	     /* add back possible corners thereof hanging out above & beyond win*/
	         if (NY < U)
	        /* slantedge passes thru top, not near side, of window */
	            AREAO += TRIA(U - NY);	/* add back trian ABOVE win */
	     FY = WE * ver/hor;  	/* "far Y": distance down to where
	    				   slant ray hits far win edge */
	     if (FY < Y1)			/* if hits abov shad bot */
	        AREAO += TRIA(Y1 - FY); 	/* add back trian BEYOND win*/
	  }
	   }
	}
	if (AREAO >= WAREA)			/* if fully shaded */
	   goto exit68;			/* go exit */

/*----- flap (vertical projection at end of horiz overhang) area AREAV -----*/

/* flapShad width: from farther of oh extension, slant x to end window */
	W2 = WE - max( E, X1);
/* flapShad hite: from: lower of hor oh shad bot, window top.
	          to: hier of hor oh shad bot + flap ht, bottom window. */
	H2 = min( Y1 + p->ohflap, HU) - max( Y1, U);
/* if hite or wid <= 0, no shadow, AREAV stays 0, H2 and W2 don't matter. */
	if (H2 > 0.f && W2 > 0.f)
	{  AREAV = H2 * W2;			/* area shaded by flap */
	   if (AREAO+AREAV >= WAREA)	/* if now fully shaded */
	      goto exit68;			/* go exit */
	}

/*----- FINS: select sunward fin and set up -----*/
fin37:			/* come here if no overhang */
	if (gamma==0.f)  		/* if sun straight on, no fin shadows, exit */
	   goto exit68;		/* (and prevent division by hor: is 0) */
	if (gamma > 0.f)	    /* if sun coming from left */
	{  FD = p->fldepth;		/* fin depth: how far out fin projects */
	   FU = p->fltx; 		/* "fin up": fin top extension above top win*/
	   FO = p->flwd; 		/* "fin over": how far sunward from window */
	   FBU = p->flwbx; 		/* "fin bottom up": how far above bot win */
	}
	else  		    /* else right fin shades toward window */
	{  FD = p->frdepth; 	/* fin depth */
	   FU = p->frtx; 		/* top extension */
	   FO = p->frwd; 		/* hor distance from window */
	   FBU = p->frwbx; 		/* bottom distance above windowsill */
	}
	if (FD <= 0.f)		/* if fin depth 0 */
	   goto exit68;		/* no fin, go exit */

	X3 = FD*hor;		/* how far over (antisunward) shadow comes */

/*---- FIN... Adjustments to not double-count overlap
		     of fin's shadow with horizontal overhang's shadow ----*/
	if (AREAO > 0.f)			/* if have overhang */
	{ float FUO;
	   FUO = U + (FO - E)*ver/hor;	/* how far up fin would extend for fin
	   					   and oh shad slantEdges to just meet
	   					   for this sun posn.  Can be < 0. */
	   if (FU > FUO)			/* if fin shadow overlaps oh shadow */
	   {  if (L==0			/* if oh shad has no hor edge in win*/
	       || X3 - FO < X1 - E)  	/* or fin shad corn (thus ver edge)
	    				   nearer than oh shad corner */
	      {  /* fin shadow ver edge intersects oh shadow slanted edge
	            (or intersects beyond window): overlap is parallelogram */
	         FU = FUO;		/* chop off top of fin that shades oh shadow.
	         			   FU < 0 (large E) appears ok 1-90. */
	         if (H + FU <= FBU) 	/* if top of fin now below bottom */
	            goto exit68;		/* no fin shadow */
	      }
	      else		/* vert edge fin shadow intersects hor edge oh shad */
	      {  /* count fin shadow area in chopped-off part of window now:
	        A triangle or trapezoid nearer than oh shadow;
	        compute as all of that area not oh shadow. */
	         AREA1 = W*(Y1 - U) - AREAO;
	         /* shorten win to calc fin shad belo hor edge of oh shad only */
	         H -= Y1 - U;	/* reduce window ht to below area oh shades */
	         FU += Y1 - U;	/* keep top of fin same */
	     /*  rob 1-90: might be needed (unproven), else harmless: */
	     U = Y1;		/* keep overhang position same: U may be used
	     			   below re fin - flap shadow interaction */
	      }
	   }
	}

/*----- FIN... shadow area on window (K = 0) or overhang flap (K = 1) ----*/

fin73:	/* come back here with window params changed to match flap shadow,
	   to compute fin shadow on flap shadow, to deduct. */
	/* COMPUTE: AREA1, ARintF: fin shadow area assuming fin extends to bot win;
	        ARSH1, ARSHF: lit area to deduct due to short fin
	        note these variables already init; add to them here. */
	NY = FO*ver/hor;	/* "near Y":  y component slant ray distance from
				   top fin to near side win */
	if (X3 > FO		/* else shadow all sunward of window */
	  && NY < H + FU)	/* else top shadow slants below window */
	{
 /* chop non-overlapping areas off of shadow and window: Simplifies later code;
	ok even tho rerun vs flap shadow because latter is within former. */

   /* far (from sun) side: chop larger of shadow, window to smaller */
	   if (X3 > W + FO)
	      X3 = W + FO;
	   else		/* window goes beyond shadow */
	      W = X3 - FO;
	   FY = (W + FO)*ver/hor;	/* "far Y":  y component slant ray
	   				   distance from top fin to far side win */
	   /* HFU = H + FU;	* top fin to bot window distance *  not used */
   /* subtract any lit rectangle and triang above shad.  nb FU can be < 0. */
	   if (FY > FU)		/* if some of top of win lit */
	   { float FYbelowWIN;
	      if (NY <= FU)			/* if less than full width lit */
	     AREA1 -= TRIA(FY-FU);	    /* subtract top far triangle */
	      else				/* full wid (both top corners) lit */
	         AREA1 -=  W * (NY-FU)	    /* subtract lit top rectangle */
		       +  TRIA(FY-NY);	    /* and lit far triangle below it*/
	  FYbelowWIN = FY - (H + FU);
	  if (FYbelowWIN > 0.f)		/* if far lit triang goes below win */
	     AREA1 += TRIA(FYbelowWIN);	    /* add back area belo win: wanna
	     				       subtract a trapezoid only */
	   }
   /* subtract any lit rectangle and triangle below fin shadow */
	   if (NY < FBU)		/* if some of bottom of win lit */
	      if (FY >= FBU)		/* if less than full width lit */
	         AREA1 -= TRIA(FBU-NY);	    /* subtract bot near triangle */
	      else				/* full wid (both bot corners) lit */
	         AREA1 -=  W * (FBU-FY)	    /* subtact lit bottom rectangle */
	         	       +  TRIA(FY-NY);	    /* and lit near triang above it */
   /* add in entire area as tho shaded (LAST for best precision: largest #) */
	   AREA1 += H * W;
	}

/*---- FIN... done this shadow; sum and proceed from window to flap ----*/

	if (K)			/* if just computed shadow on flap shadow */
	   AREA1 = -AREA1;		/* negate fin shadow on flap shadow */
	ARintF += AREA1;		/* combine shadow areas */
	if (K==0 && AREAV > 0.f)	/* if just did window and have flap shadow */
	{
	  float HbelowFlapShad;
  /* change window geometry to that of oh flap shadow,
	 and go back thru fin shadow code: Computes fin shadow on flap shadow(!!),
	 which is then deducted (just above) to undo double count. */
	   K++;				/* K = 1: say doing fin on flap */

	   AREA1 = 0.f;			/* re-init working areas */
	   /* adjust fin-over to be relative to near side flap shadow */
	   if (X1 > E)		/* if near edge flap shadow is in window */
	  FO += X1 - E; 	/* adjust FO to keep relative */
	   /* else near edge of area H2*W2 IS near edge window, no FO change */
	   HbelowFlapShad = HU - Y1 - p->ohflap;
	   if (HbelowFlapShad > 0.f)	/* if bot flap shadow above bot win */
	   {  FBU -= HbelowFlapShad;	/* adjust fin-bottom-up to keep fin
	   					   effectively in same place */
	      if (FBU < 0.f)
	         FBU = 0.f;
	   }
	   if (Y1 > U)		/* if flap shadow top below top win */
	      FU += Y1 - U;		/* make fin-up relative to flap shadow */
	   H = H2;			/* replace window ht and wid with */
	   W = W2;			/* ... those of flap shadow */
	   goto fin73;
	}

exit68:  		// come here to exit
	fsunlit = 1.f - (AREAO + AREAV + ARintF ) / WAREA;
	// return 0 if result so small that is probably just roundoff errors;
	//  in particular, return 0 for small negative results.
	return fsunlit > 1.e-7f ? fsunlit : 0.f;

}	// ShadeWin
//--------------------------------------------------------------------------
#if defined( TESTSHADING)
t // original shadwin(), for comparing results to revised version
t static float shadwin(		// Calculate sunlit fraction of window shaded
t 			//   with fins and/or overhang
t    const struct OHFIN *p,	// struct of dimensions of window/ovhg/fins
t    				//   (see above and shade.h)
t    float gamma,			// azimuth of sun less azimuth of window:
t 				//   angle 0 if sun normal to window,
t 				//   positive cw looking down.
t    float cosz)			// cosine of solar zenith angle
t
t // returns: fraction of area sunlit, 0 to 1.f
t {
t // comment comments
t //     "near" = toward sun (horizontally), "far" or beyond = away from sun.
t //     "ray" = extended slanted shadow line from top of fin or near (sunward)
t //             end of overhang.
t //     _1 variables are for hor overhang,
t //     _2  are for vertical flap at end of overhang,
t //     _3  are for fin. */
t // flow control
t int K;		// 0 normal, 1 when doing fin-flap shadow overlap
t int L;		// 1 if overhang horiz shadow edge is in window, else 0
t // angles
t float cosg, sing;	// sin and cos of gamma (angle from window normal
t 			//  to sun, + for sun to left when looking at window)
t float ver, hor; 	// components of shadow on wall of unit projection
t // window
t float H, W;	// hite, wid of window (later of flap shadow)
t float WAREA;	// whole window area
t // overhang
t float E, U;	// near (to sun) flap horiz extension; dist flap above win
t float HU, WE;	// H+U;  W+E: Width + Extension
t float X1, Y1;	// components hor oh shad slantEdge -- how far over & down
t float AREAO;
t float H2, W2;	// fin flap shadow hite and width
t float AREAV;
t // fin
t float FD;  	// fin depth: how far out fin projects
t float FU; 	// "fin up": fin top extension above top win
t float FO; 	// "fin over": how far sunward from window to fin
t float FBU;	// "fin bottom up": how far above bot win fin ends
t float X3;	// x compon fin shadow slantEdge: hor far over shadow goes
t float ARintF, AREA1;		// shadow area & working copy
t // shadow ray intercepts for oh then fin.
t //  NB these relate to "ray"; may be beyond actual shadow corner
t float NY, FY;	// how far down slant ray is at near, far win edge intercepts
t // result
t float fsunlit;
t
t /* triangle area for specific altitude and global hor and ver components.
t    Divide by 0 protection: most alt's are 0 if ver is 0. */
t #define TRIA(alt) ( (alt)==(float)0.f ? (float)0.f : (alt)*(alt)*0.5f*hor/ver )
t 			/* the (float)s really are necessary to prevent data
t 			   conversion warnings in MSC 5.1. rob 1-24-90.*/
t
t /* init */
t 	K = 0;			/* 1 later during fin-flap overlap */
t 	cosg = (float)cos(gamma);		/* sin and cos of ... */
t 	sing = (float)sin(gamma);		/* ... solar azimuth rel to window */
t 	if (cosg <= 0.f) 			/* if sun behind wall */
t 	   return 0.f;			/* window fully in shadow */
t 	if (cosz >= 1.f)			/* sun at zenith: no sun enters win.*/
t 	   return 0.f;			/* (rob 1-90 prevents div by 0) */
t 	   					/* following line compiles incorrectly
t 					   with default optimization -Oswr;
t 					   expression produces reciprocal of
t 					   correct result.  #pragma opt off
t 					   added above.  8-5-90 chip */
t 	ver = float( cosz/sqrt(1.-cosz*cosz)/cosg);	/* tan(zenith angle) / cos(gamma) */
t 	hor = (float)fabs(sing)/cosg;	/* abs(tan(gamma)): unit fin shade */
t 	AREAO = 0.f;  		/* hor overhang shadow area */
t 	AREAV = 0.f;  		/* ver overhang flap shadow area */
t 	ARintF = 0.f;  		/* fin shadow area */
t 	AREA1 = 0.f;  		/* working copies of above */
t 	H = p->wheight;
t 	W = p->wwidth;
t 	WAREA = H * W;		/* whole window area */
t 	W2 = H2 = 0.f;		/* init width & hite of oh flap shadow */
t /* horiz overhang setup */
t 	E= gamma < 0.f ? p->ohrx : p->ohlx;	/* pertinent (near sun) oh extension*/
t 	U = p->ohwd; 			/* distance from top win to overhang*/
t 	HU = H + U;				/* from overhang to bottom window */
t 	WE = W + E;  			/* from near end oh to far side wind*/
t 	Y1 = p->ohdepth * ver;		/* how far down oh shadow comes */
t 	X1 = p->ohdepth * hor;	    	/* hor component oh shad slantedge
t 						   = hor displ corners oh shadow */
t 	/* NOTE: X1, Y1 are components of actual length (to corner) of shadow
t 		        slanted edge, based on oh depth.
t 	         NY, FY relate to slanted ray intercepts with window edges,
t 	            irrespective of whether beyond actual corner. */
t 	L = Y1 > U  &&  Y1 < HU  &&  X1 < WE;	/* true if near shad bot corn
t 							(hence hor edge) is in win */
t 	if (p->ohdepth <= 0.f)		/* if no overhang */
t 	   goto fin37;			/* go do fins.  flap is ignored. */
t
t /*----- horizontal overhang shadow area "AREAO" -----*/
t
t 	if (Y1 > U			/* if shadow comes below top window */
t 	 && (hor==0.f ||		/* /0 protection */
t 	     (FY=WE*ver/hor) > U))	/* and edge passes below top far win corn */
t 			/* (set FY to how far down slant ray hits far side) */
t 	{  if (Y1 > HU)		/* chop shadow at bot win to simplify */
t 	      Y1 = HU;		/* (adjusting X1 to match doesn't matter
t 	      			   in any below-win cases) */
t    /* area: start with window rectangle above shadow bottom */
t 	   AREAO = W * (Y1 - U);	/* as tho shadow corner sunward of window */
t
t    /* subtract unshaded portions of rectangle if shad corner is in win */
t 	   if (hor != 0.f)		/* else done and don't divide by 0 */
t 	   {  NY = E * ver/hor;   	/* "near Y": distance down from oh to where
t 					   slant ray hits near (to sun) win edge */
t 	  /* if corner not sunward of window, subtract unshaded triangle area */
t 	      if (NY < Y1)  		/* if slant hits nr side abov sh bot*/
t 	      {  AREAO -= TRIA(Y1 - NY); 	/* triangle sunward of edge */
t 	     /* add back possible corners thereof hanging out above & beyond win*/
t 	         if (NY < U)
t 	        /* slantedge passes thru top, not near side, of window */
t 	            AREAO += TRIA(U - NY);	/* add back trian ABOVE win */
t 	     FY = WE * ver/hor;  	/* "far Y": distance down to where
t 	    				   slant ray hits far win edge */
t 	     if (FY < Y1)			/* if hits abov shad bot */
t 	        AREAO += TRIA(Y1 - FY); 	/* add back trian BEYOND win*/
t 	  }
t 	   }
t 	}
t 	if (AREAO >= WAREA)			/* if fully shaded */
t 	   goto exit68;			/* go exit */
t
t /*----- flap (vertical projection at end of horiz overhang) area AREAV -----*/
t
t /* flapShad width: from farther of oh extension, slant x to end window */
t 	W2 = WE - max( E, X1);
t /* flapShad hite: from: lower of hor oh shad bot, window top.
t 	          to: hier of hor oh shad bot + flap ht, bottom window. */
t 	H2 = min( Y1 + p->ohflap, HU) - max( Y1, U);
t /* if hite or wid <= 0, no shadow, AREAV stays 0, H2 and W2 don't matter. */
t 	if (H2 > 0.f && W2 > 0.f)
t 	{  AREAV = H2 * W2;			/* area shaded by flap */
t 	   if (AREAO+AREAV >= WAREA)	/* if now fully shaded */
t 	      goto exit68;			/* go exit */
t 	}
t
t /*----- FINS: select sunward fin and set up -----*/
t fin37:			/* come here if no overhang */
t 	if (gamma==0.f)  		/* if sun straight on, no fin shadows, exit */
t 	   goto exit68;		/* (and prevent division by hor: is 0) */
t 	if (gamma > 0.f)	    /* if sun coming from left */
t 	{  FD = p->fldepth;		/* fin depth: how far out fin projects */
t 	   FU = p->fltx; 		/* "fin up": fin top extension above top win*/
t 	   FO = p->flwd; 		/* "fin over": how far sunward from window */
t 	   FBU = p->flwbx; 		/* "fin bottom up": how far above bot win */
t 	}
t 	else  		    /* else right fin shades toward window */
t 	{  FD = p->frdepth; 	/* fin depth */
t 	   FU = p->frtx; 		/* top extension */
t 	   FO = p->frwd; 		/* hor distance from window */
t 	   FBU = p->frwbx; 		/* bottom distance above windowsill */
t 	}
t 	if (FD <= 0.f)		/* if fin depth 0 */
t 	   goto exit68;		/* no fin, go exit */
t
t 	X3 = FD*hor;		/* how far over (antisunward) shadow comes */
t
t /*---- FIN... Adjustments to not double-count overlap
t 		     of fin's shadow with horizontal overhang's shadow ----*/
t 	if (AREAO > 0.f)			/* if have overhang */
t 	{ float FUO;
t 	   FUO = U + (FO - E)*ver/hor;	/* how far up fin would extend for fin
t 	   					   and oh shad slantEdges to just meet
t 	   					   for this sun posn.  Can be < 0. */
t 	   if (FU > FUO)			/* if fin shadow overlaps oh shadow */
t 	   {  if (L==0			/* if oh shad has no hor edge in win*/
t 	       || X3 - FO < X1 - E)  	/* or fin shad corn (thus ver edge)
t 	    				   nearer than oh shad corner */
t 	      {  /* fin shadow ver edge intersects oh shadow slanted edge
t 	            (or intersects beyond window): overlap is parallelogram */
t 	         FU = FUO;		/* chop off top of fin that shades oh shadow.
t 	         			   FU < 0 (large E) appears ok 1-90. */
t 	         if (H + FU <= FBU) 	/* if top of fin now below bottom */
t 	            goto exit68;		/* no fin shadow */
t 	      }
t 	      else		/* vert edge fin shadow intersects hor edge oh shad */
t 	      {  /* count fin shadow area in chopped-off part of window now:
t 	        A triangle or trapezoid nearer than oh shadow;
t 	        compute as all of that area not oh shadow. */
t 	         AREA1 = W*(Y1 - U) - AREAO;
t 	         /* shorten win to calc fin shad belo hor edge of oh shad only */
t 	         H -= Y1 - U;	/* reduce window ht to below area oh shades */
t 	         FU += Y1 - U;	/* keep top of fin same */
t 	     /*  rob 1-90: might be needed (unproven), else harmless: */
t 	     U = Y1;		/* keep overhang position same: U may be used
t 	     			   below re fin - flap shadow interaction */
t 	      }
t 	   }
t 	}
t
t /*----- FIN... shadow area on window (K = 0) or overhang flap (K = 1) ----*/
t
t fin73:	/* come back here with window params changed to match flap shadow,
t 	   to compute fin shadow on flap shadow, to deduct. */
t 	/* COMPUTE: AREA1, ARintF: fin shadow area assuming fin extends to bot win;
t 	        ARSH1, ARSHF: lit area to deduct due to short fin
t 	        note these variables already init; add to them here. */
t 	NY = FO*ver/hor;	/* "near Y":  y component slant ray distance from
t 				   top fin to near side win */
t 	if (X3 > FO		/* else shadow all sunward of window */
t 	  && NY < H + FU)	/* else top shadow slants below window */
t 	{
t  /* chop non-overlapping areas off of shadow and window: Simplifies later code;
t 	ok even tho rerun vs flap shadow because latter is within former. */
t
t    /* far (from sun) side: chop larger of shadow, window to smaller */
t 	   if (X3 > W + FO)
t 	      X3 = W + FO;
t 	   else		/* window goes beyond shadow */
t 	      W = X3 - FO;
t 	   FY = (W + FO)*ver/hor;	/* "far Y":  y component slant ray
t 	   				   distance from top fin to far side win */
t 	   /* HFU = H + FU;	* top fin to bot window distance *  not used */
t    /* subtract any lit rectangle and triang above shad.  nb FU can be < 0. */
t 	   if (FY > FU)		/* if some of top of win lit */
t 	   { float FYbelowWIN;
t 	      if (NY <= FU)			/* if less than full width lit */
t 	     AREA1 -= TRIA(FY-FU);	    /* subtract top far triangle */
t 	      else				/* full wid (both top corners) lit */
t 	         AREA1 -=  W * (NY-FU)	    /* subtract lit top rectangle */
t 		       +  TRIA(FY-NY);	    /* and lit far triangle below it*/
t 	  FYbelowWIN = FY - (H + FU);
t 	  if (FYbelowWIN > 0.f)		/* if far lit triang goes below win */
t 	     AREA1 += TRIA(FYbelowWIN);	    /* add back area belo win: wanna
t 	     				       subtract a trapezoid only */
t 	   }
t    /* subtract any lit rectangle and triangle below fin shadow */
t 	   if (NY < FBU)		/* if some of bottom of win lit */
t 	      if (FY >= FBU)		/* if less than full width lit */
t 	         AREA1 -= TRIA(FBU-NY);	    /* subtract bot near triangle */
t 	      else				/* full wid (both bot corners) lit */
t 	         AREA1 -=  W * (FBU-FY)	    /* subtact lit bottom rectangle */
t 	         	       +  TRIA(FY-NY);	    /* and lit near triang above it */
t    /* add in entire area as tho shaded (LAST for best precision: largest #) */
t 	   AREA1 += H * W;
t 	}
t
t /*---- FIN... done this shadow; sum and proceed from window to flap ----*/
t
t 	if (K)			/* if just computed shadow on flap shadow */
t 	   AREA1 = -AREA1;		/* negate fin shadow on flap shadow */
t 	ARintF += AREA1;		/* combine shadow areas */
t 	if (K==0 && AREAV > 0.f)	/* if just did window and have flap shadow */
t 	{
t 	  float HbelowFlapShad;
t   /* change window geometry to that of oh flap shadow,
t 	 and go back thru fin shadow code: Computes fin shadow on flap shadow(!!),
t 	 which is then deducted (just above) to undo double count. */
t 	   K++;				/* K = 1: say doing fin on flap */
t
t 	   AREA1 = 0.f;			/* re-init working areas */
t 	   /* adjust fin-over to be relative to near side flap shadow */
t 	   if (X1 > E)		/* if near edge flap shadow is in window */
t 	  FO += X1 - E; 	/* adjust FO to keep relative */
t 	   /* else near edge of area H2*W2 IS near edge window, no FO change */
t 	   HbelowFlapShad = HU - Y1 - p->ohflap;
t 	   if (HbelowFlapShad > 0.f)	/* if bot flap shadow above bot win */
t 	   {  FBU -= HbelowFlapShad;	/* adjust fin-bottom-up to keep fin
t 	   					   effectively in same place */
t 	      if (FBU < 0.f)
t 	         FBU = 0.f;
t 	   }
t 	   if (Y1 > U)		/* if flap shadow top below top win */
t 	      FU += Y1 - U;		/* make fin-up relative to flap shadow */
t 	   H = H2;			/* replace window ht and wid with */
t 	   W = W2;			/* ... those of flap shadow */
t 	   goto fin73;
t 	}
t
t exit68:  		/* come here to exit */
t 	fsunlit = 1.f - (AREAO + AREAV + ARintF ) / WAREA;
t /* return 0 if result so small that is probably just roundoff errors;
t    in particular, return 0 for small negative results.  rob 1-90. */
t 	return fsunlit > 1.e-7f ? fsunlit : 0.f;
t
t }	// shadwin
#endif		// TESTSHADING

//--------------------------------------------------------------------------
static double DCToAzm(			// convert direction cosines to azimuth
	double dirCos[ 3])			// direction cosines
// returns value 0 <= x < 2Pi, 0 = N
{
	if ( fabs( dirCos[ 0]) < 0.00001 && fabs( dirCos[ 1]) < 0.00001)
		return kPi;		// return exact PI (south) for overhead case
	double azm = atan2( dirCos[ 0], dirCos[ 1] );
	return azm >= 0. ? azm : azm + k2Pi;
}		// ::DCToAzm
//===========================================================================
#if 0	// possibly useful
0 VOID gmcos2alt( 		// get altitude and azimuth for direction cosines
0
0     float *dcos,	// Array of cosines
0     float *altp,	// receives altitude (+-Pi/2 for horiz plane, 0 for vertical; COMPLEMENT of TILT used elsewhere).
0     float *azmp,	/* receives azimuth, in compass range -Pi to Pi.
0 			   Pi is returned if altitude is so near Pi/2 that azimuth is undefined. */
0 {
0 double val, azm;
0
0    /* altitude.  > 1 and < -1 execptions insure against domain errors if input
0 		 is a snerd out of range (no case actually seen -- rob).
0 		 (do NOT use fAboutEqual: would 'snap' too far!) */
0
0 		*altp = dcos[2] >= 1.F ? PiOverTwo
0 	      		: dcos[2] <= -1.F ? -PiOverTwo
0 	      : (float)asin( (double) dcos[2]);
0
0 		// for more accuracy near vertical consider:
0 		//   atan2( dcos[2], sqrt(dcos[0]^2+dcos[1]^2) );
0 		//   and could then eliminate the +-1. exceptions.
0
0    /* azimuth: if both x and y direction cosines are too near 0, surface is
0       horizontal and azimuth is undefined (and atan2 will error). */
0
0 	// this is a stricter test (Pi returned less often) than testing
0 	// dcos[z] for being nearly 1.0; probably about the same as testing
0 	   dcos[z]^2 for nearly 1.0. */
0
0        if (fAboutEqual( dcos[0], 0.F) && fAboutEqual( dcos[1], 0.F))
0        {	  *azmp = Pi;                   /* arbitrarily return Pi */
0        }
0        else
0        {
0 	   /* Need to 'snap' to exactly 90 degree multiples when near same?
0 	      I don't think so, but if so test dcos[x] and [y] for being near
0 	      0.0 -- tests of near +- 1.0 would snap from too far away (flat
0 	      derivative).  Note that calling code in/for tkadj depends on
0 	      accurate azimuth where z's will be determined from x and y on
0 	      nearly vertical plane for knee walls; snap would mess this up. */
0
0     /* compute angle for dcos[x] and dcos[y] using 2-arg arctangent(y/x). This
0        fcn handles sign for 4 quadrants, does not require us to adjust for
0        dcos[z], and is believed to be uniformly accurate in all quadrants */
0
0 	  		*azmp = (float) atan2( (double)dcos[0], /*-*/(double)dcos[1] );
0 	   			/* arg interchange [and - sign] is to produce
0 	   			   compass values (clockwise from y axis). */
0        }
0 }               /* gmcos2alt */
#endif
//===========================================================================

// solar.cpp end
