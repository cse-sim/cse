////////////////////////////////////////////////////////////////////////////
// solar.h: defns and decls for solar calculations
//=============================================================================
// 
// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// Portions copyright (c) 2014, ASHRAE.
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

#if 1 && defined( _DEBUG)
#define SLRCALCS 3		// transition aid re migration to solar.h style calcs
						//    1: CSE old style
						//    2: solar.h style
						//    3: both
#else
#define SLRCALCS 1
#endif

#if 0
#include <coa.h>		// for SLRCALC (below)
#endif

// constants
const double HaPerHr = kPi/12.;			// earth rotation per hr (rad)
					  					//   for converting time to ha
const float SolarConstant = 433.33f;	// solar constant (Btuh/ft2) =
										//   ext solar irradiance on normal
										//   surface at mean earth/sun dist.
										//   SERI value: 1367 w/m2
										//   (1 Btuh/ft2 = 3.1546 w/m2 )
const int sltmSLR = 1;			// calcs done in solar time
const int sltmLST = 2;			//  LST
const int sltmLDT = 3;			//  LDT

const int slropSIMPLETIME = 1;	// SLRLOC::Init() option: use "simple" time
const int slropCSMOLD = 2;		// SLRDAY::SetupASHRAE() option: use old clear sky model
const int slropCSMTAU = 4;		// SLRDAY::SetupASHRAE() option: use ASHRAE tau clear sky model
								//    if data available else use "new"
const int slropCSMTAU13 = 1024;	// SLRDAY::SetupASHRAE() option: use 2013 tau coefficients
								//    iff slropCSMTAU also set



// slpak.cpp solar related definitions


/*-------------------------------- DEFINES --------------------------------*/

// Time types -- see slday().  All are 0..24, 0 at midnite.
#define SLTMSOLAR 1       // solar time
#define SLTMLST 2         // local standard time
#define SLTMLDT 3         // local daylight savings time

/* note: Hour Angle defined: time as angle from solar noon at longitude.
		 Adjusted for longitude from center of time zone and from daylight or
		 local time to solar (and date still changes at local/daylite midnite),
	 so range is bit more than -Pi to Pi. (mainly used when sun is up.) */

const double HA = k2Pi / 24.;  	// earth rotation, rad/hr
								// for converting hours to hour angle
								//   then add SLLOCDAT.tmconst.

/*--------------------------------- TYPES ---------------------------------*/

// SLLOCDAT: Location/day specific solar data.
//	These structures are alloc/init by slinit();
//	daily values are set by slday().

struct SLLOCDAT
{
	// location items, set once, by slinit()
	float rlat;			// latitude of location in radians
	float rlong;		// longitude in radians
	float tzn;			// time zone, hours from GMT, + = west
	float sinlat;		// sin of latitude
	float coslat;		// cos of latitude
	float siteElev;		// site elevation, ft above sea level
	float pressureRatio;	// ratio of surface pressure to sea level pressure (used only in dirsimx.c, 8-9-90)
	// remaining members set daily by slday(), or sldec() from slday.
	// declination: angle of earth's axis from vertical, + in summer (?) in N hemisphere, computed each day by sldaydat.
	int doy;			// day of year (1 - 365)
	float rdecl;		// declination this day in radians
	float cosdec;		// cos of declination
	float sdsl;			// sindec * sinlat
	float sdcl;			// sindec * coslat
	float cdsl;			// cosdec * sinlat
	float cdcl;			// cosdec * coslat
	// hour angle: local time as angle, 0 at solar noon at this longitude
	float hasunset;		// solar hour angle of sunset = -sunrise.
	// equation of time: add to local time to get solar.
	float eqtime;		// equation of time, hours, set in slday().
	// time to hour angle conversion constant: add to hr/HA to make noon 0 & adjust curr type time to solar
	float tmconst;		// set in sldec(), used in slha,
	float extbm;		// normal intensity of extraterrestrial beam for current day (Btu/ft2), set in slday().
	int timetype;		// time type of following values: SLTMSOLAR, SLTMLST, SLTMDT
	float sunupf[24];		// Sun up fraction of each hour (0 - 1); entry 0 is for midnight - 1 AM, etc.
	float dircos[24][3]; 	// Direction cosines of sun for day, on the hour, or at sunrise/sunset for last and 1st night hour
	float slrazm[24];		// Solar azimuth evaluated on the hour (or sunrise/sunset) for current day
};


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// slpak.c
#ifdef TEST
x  void FC slaltfromazm(float azm);
x  void FC slhourfromalt(float alt);
#endif
void   FC slsdc(float azm, float tilt, float dcos[3]);
float slVProfAng(float relAzm, float cosZen);
SLLOCDAT* FC slinit(float rlat, float rlong, float tzn, float siteElev);
SLLOCDAT* FC slselect(SLLOCDAT* pSlr);
void   FC slfree(SLLOCDAT** ppSlr);
void   FC slday(DOY doy, int timetype, int options = 0);
void   FC sldec(float sldec, int timetype);
int slsurfhr(float sdircos[3], int ihr, float* pCosi, float* pAzm = NULL,
					float* pCos = NULL);
float slSunupf(int ihr);
int slSunupBegEnd(int ihr, float& hrBeg, float& hrEnd, float sunupf = -1.f);
void   FC slaltazm(float fhour, float* altp, float* azmp);
void   FC slaniso(float* pbeam, float* pdiff, SI ihr);
float slha(float fhour);
float slASTCor();

float slASHRAETauModel(float sunZen, float tauB, float tauD, float& radDirN, float& radDifH);
bool slASHRAETauModelInv(double sunZen, double Ebn, double Edh, float& tauB, float& tauD);

#if 0
x RC slPerezSkyModel(double tilt, double azimuth, short iHrST, double direct_normal_beam, double horizontal_diffuse,
x	double ground_reflectivity, double& plane_incidence);
#else
RC slPerezSkyModel(float tilt, float azimuth, int iHrST, float direct_normal_beam, float horizontal_diffuse,
	float ground_reflectivity, float& incA, float& plane_beam, float& plane_diffuse_sky, float& plane_diffuse_ground);
#endif


class SLRSURF;
class COA_SLRSURF;

/////////////////////////////////////////////////////////////////////////////
// class SLRLOC -- time independent solar info for location
/////////////////////////////////////////////////////////////////////////////
class SLRLOC
{
friend class SLRCALC;
friend class SLRDAY;
friend class SLRSURFDAY;
	int sl_options;		// option bits
						//   slropSIMPLETIME: use "simple" time
						//      (no longitude/tzn in solar calcs (assumes long=tzn*15))
	int sl_SN;			// weather database station number, 0 if not known
	float sl_lat;		// latitude (degN, <0 = southern hemisphere)
	float sl_lng;		// longitude (degE, <0 = degW)
	float sl_tzn;		// time zone (GMT +X, e.g. EST = -5, PST = -8,
						//	Paris = +1)
	float sl_elev;		// elevation (ft above sea level)

	double sl_sinLat;	// sin( lat)
	double sl_cosLat;	// cos( lat)

public:
	SLRLOC();
	SLRLOC( float lat, float lng, float tzn, float elev);
	SLRLOC( const SLRLOC& sl)
	{	Copy( sl);	}
	virtual ~SLRLOC();
	void sl_Init( float lat, float lng, float tzn, float elev, int SN, int options=0,
		int oMsk=0);
	SLRLOC& Copy( const SLRLOC& sl);
	SLRLOC& operator=( const SLRLOC& sl)
	{	return Copy( sl); }
	BOOL IsEqual( const SLRLOC& sl) const;
	BOOL operator==( const SLRLOC& sl) const { return IsEqual( sl); }
	BOOL operator!=( const SLRLOC& sl) const { return !IsEqual( sl); }
	int CalcDOYForMonth( int iMon) const;
	float GetLat() const { return sl_lat; }
	float GetElev() const { return sl_elev; }
	double TanLat() const { return sl_sinLat / max( .00001, sl_cosLat); }
	void DbDump() const;
};	// class SLRLOC
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRHR -- solar info for an hour, substruct of SLRDAY
/////////////////////////////////////////////////////////////////////////////
class SLRHR
{
public:
	float ha;			// hour angle (radians) from solar noon
						//   0 at solar noon, <0 when sun east (morning);
						//   -99.f if sun down for entire hour,
						//   calc'd at sunrise/sunset for partial hrs
	float fUp;			// fraction (dimless) sun is up during hour beginning
						//   at cur time
	double sinHa;		// sin( ha), 0 if sun not up
	double cosHa;		// cos( ha), 0 if sun not up
	double dirCos[ 3];	// direction cosines of sun (0 if sun not up)
				 		//  for exact time iT (0=midnight etc)
				 		//  EXCEPT if sun rises during hour beginning at iT
						//  or sets during hour ending at iT, in which case
						//  at sunrise/sunset time.

	double azm;			// sun azimuth (radians) CW angle from N (0 - 2PI)
						//   (e.g. east = Pi/2)
						//   0 if sun down or straight overhead
						// NOTE: SLRSURFDAY::InitSim() interpolation depends
						//   on azm >= 0
	double SinAlt() const {	return dirCos[ 2]; }
	double CosZen() const {	return dirCos[ 2]; }

						// surface-independent clear sky values for loads calcs
	float radDir;		//   direct normal beam irradiance (Btuh/ft2)
	float radDif;		//   diffuse irradiance on horiz surface (Btuh/ft2)
	float radHor;		//	 total irradiance on horiz surface (Btuh/ft2)

	void Calc( class SLRDAY* pSD);
	BOOL IsSunUp() const { return ha != -99.f; }
	double RelAzm( double srfAzm) const;
	double GetZenAngle() const { return IsSunUp() ? acos( dirCos[ 2]) : kPi; }
	void DbDump( int oMsk, const TCHAR* tag) const;

};		// class SLRHR
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRDAY -- solar info for a single day
/////////////////////////////////////////////////////////////////////////////
class SLRDAY
{
friend class SLRHR;
friend class SLRSURFDAY;
friend class SLRCALC;
	SLRLOC* sd_pSL;			// location data

	int sd_iM;				// month idx (0-11)
	int sd_doy;				// day of year currently set up (1-365)
	int sd_iCSM;			// ASHRAE clear sky model idx
							//   0: 1997 coefficients ("old")
							//   1: 2005 coefficients (Machler-Iqbal "new" coefficents)
							//   2: 2009 tau model
							//   3: 2013 tau model

	int sd_sltm;			// time convention, sltmSLR/LST/LDT
	double sd_eqTime;		// equation of time (hrs)
	double sd_haConst;		// hour angle constant (radians), includes
							//   effects of equation of time, longitude,
							//   and time zone

	double sd_decl;  		// declination (deg)
	double sd_sinDecl;		// sin( sd_decl)
	double sd_cosDecl;  	// cos( sd_decl);
    double sd_sdsl;			// sindec * sinlat
    double sd_sdcl;			// sindec * coslat
    double sd_cdsl;			// cosdec * sinlat
    double sd_cdcl;			// cosdec * coslat

	double sd_haSunset;		// sunset hour angle (radians)
	double sd_cosSunset;	// cos( haSunset)

							// re hourly fraction of daily total
							//   see Duffie & Beckman ref'd equations
	double sd_aCoeff;		//  coefficient 'a', Eqn 2.13.2b
	double sd_bCoeff;		//  coefficient 'b', Eqn 2.13.2c
	double sd_dCoeff;		//  coefficient 'd', Eqn 2.20.4d

							// pseudo-optical depths for 2009 clear sky model
	float sd_tauB;			//	beam
	float sd_tauD;			//  diffuse
	float sd_extBm;			// extraterrestrial beam irradiance (Btuh/ft2)
							//   -1 if not known
	float sd_H0;			// daily total extraterrestrial (beam) irradiation
							//	 on horiz surface, Btu/ft2

public:
	SLRHR sd_HSlrNoon;		// conditions for solar noon
							//   same for all time conventions
	SLRHR sd_H[ 24];		// hourly derived data
							//   values depend on time convention

	SLRDAY( SLRLOC* pSL=NULL);
	virtual ~SLRDAY();
	SLRLOC* GetSLRLOC() { return sd_pSL; }
	void SetSLRLOC( SLRLOC* pSL) { sd_pSL = pSL; }
	void Setup( int doy, int sltm=sltmLST, SLRLOC* pSL=NULL);
	void SetupCalcDOY( int iMon, int sltm=sltmLST, SLRLOC* pSL=NULL);
	void SetupASHRAE( int iMon, int options=0, int sltm=sltmLST, SLRLOC* pSL=NULL,
		float tauB=0.f, float tauD=0.f);
	void SetupDOYValuesASHRAE2009();
private:
	BOOL Setup2();
public:
	SLRHR* GetSLRHR( int iH) { return sd_H+bracket( 0, iH, 23); }
	double TanDecl() const { return sd_sinDecl / max( .001, sd_cosDecl); }
	double HaForTm( double fT) const		// hour angle
	{	return fT * HaPerHr + sd_haConst; }
	double HaForTm( int iT) const
	{	return HaForTm( double( iT)); }
	double TmForHa( double ha) const
	{	return (ha - sd_haConst) / HaPerHr; }
	int SolarTimeShift() const;
	void DbDump( int oMsk, const TCHAR* hdg="") const;
	float GetExtBm() const { return sd_extBm; }
	float ExtRadHoriz( double ha1, double ha2) const;
	float KT( float H) const;
	float MonthDiffuseFract( float KTBar) const;
	void HourFracts( double ha, float& rt, float& rd) const;

};		// SLRDAY
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRSURFDAY -- solar info for a surface for one day
/////////////////////////////////////////////////////////////////////////////
struct RADSURF		// helper
{	float dir;			// direct irradiance on surface (Btuh/ft2)
	float dif;			// diffuse irradiance on surface (Btuh/ft2)
	float grf;			// ground reflected irradiance on surface (Btuh/ft2)
	float Tot( float fSunlit=1.f) const { return fSunlit * dir + dif + grf; }
	float Dir( float fSunlit=1.f) const { return fSunlit * dir; }
	float Dif() const { return dif; }
	float Grf() const { return grf; }
	float DifGrf() const { return dif + grf; }
	void Clear() { dir = dif = grf = 0.f; }
};	// struct RADSURF
//-----------------------------------------------------------------------
class SLRSURFDAY
{
friend class SLRCALC;
public:
	SLRSURF* ssd_pSS;		// surface (see below)
	SLRDAY* ssd_pSD;		// solar info for this day
	int ssd_doy;			// day of year currently set up (1-365), used
							//   by InitDayIf to detect ssd_pSD change
	float ssd_grRef;		// day-specific ground reflectance
							//   if <0, use ssd_pSS->ss_grRef
							//   see SetDayGrRef()
	float ssd_grfX;			// day-specific ground refl factor = grRef*(1-cos( tilt))/2
							//   valid iff ssd_grRef set, see SetDayGrRef()
	float ssd_shgfDifX;		// diffuse shgf factor = transDif + 0.267*absDif
							//   for ref glass, used in both ASHRAE and
							//   simulator calcs

	double ssd_cosInc[ 24];	// cos( solar inc angle) by time.  -1 if sun is
							//  down, else may be <0 if sun is behind surf
							// values are for exact time iT (0=midnight etc)
							// EXCEPT if sun rises (rel to horizon, not
							// surf) during hour beginning at iT or sets
							// during hour ending at iT, in which case values
							// are at sunrise/sunset time.
	double ssd_angInc[ 24];	// angle of beam incidence, radians
							//    PI/2 if sun down or behind surface
							//    0 if normal
							// beam profile angles (relative to surface), radians
							//    PI/2 if sun down or behind surface
	double ssd_angProfV[ 24];	// vertical
	double ssd_angProfH[ 24];	// horizontal

						// re ASHRAE load calc (NOTE: [ iT] is as defined per
						//  solar defn, access with care!)
	float ssd_shgfMax;  	// ASHRAE SHGF (Btu/ft2) = daily max solar gain
							//   through ref glass
	float ssd_shgfShdMax;	// SHGF value (Btuh/ft2) for externally shaded
							//   glass (shgf calc'd with direct beam = 0)
	double ssd_gamma[ 24]; 	// solar/surface relative azimuth (radians)
							//   0 = sun azm normal to surface
							//   positive cw looking down; -Pi - +Pi
	RADSURF ssd_rad[ 24];	// unshaded surface irradiance by hour (dir, dif, grf)
							//   Btuh/ft2

						// re simulation: values are for hour iH = from
						//   time iT=iH - iH+1
	double ssd_cosIncX[ 24];// approx avg cos( inc angle) over portion of
							//   hour sun is above surface
	double ssd_cosZenX[ 24];// approx avg cos( zenith angle), used for
							//   shading calcs, 0 when sun down or behind surf
	double ssd_gammaX[ 24];	// approx avg solar/surf azimuth (radians)
							//   0 = sun normal to surface, positive
							//   positive cw looking down, -Pi - +Pi
	double ssd_sgxDif;		// diffuse gain factor = ref glass gain/ft2 per
							//   unit diffuse irradiance
	double ssd_sgxDir[ 24];	// direct gain factor = ref glass gain/ft2 per
							//   unit direct normal irradiance
							// idx range of nz ssd_sgxDir nz values
	int ssd_iHSgxDirBeg;	// 	 1st nz value
	int ssd_iHSgxDirEnd;	//   (last+1) nz value

public:
	SLRSURFDAY( SLRSURF* pSS=NULL, SLRDAY* pSD=NULL);
	virtual ~SLRSURFDAY() {}
	void SetSLRSURF( SLRSURF* pSS) { ssd_pSS = pSS; }
	SLRSURF* GetSLRSURF() const { return ssd_pSS; }
	void SetSLRDAY( SLRDAY* pSD) { ssd_pSD = pSD; }
	void Init( int options, SLRDAY* pSD=NULL, float grRef=-1.f);
	void SetDayGrRef( float grRef, int options=0);
	inline float Tilt() const;
	inline float Azm() const;
	inline double* DirCos() const;
	inline double CosTilt() const;
	inline float DifX() const;
	float GrRef() const;
	float GrfX( float grRef=-1.f) const;
	inline double RelAzm( double sunAzm) const;
	inline BOOL IsSameOrientation( float azm, float tilt) const;
	SLRDAY* GetSLRDAY() { return ssd_pSD; }
	const SLRDAY* GetSLRDAY() const { return ssd_pSD; }
	SLRLOC* GetSLRLOC() { return ssd_pSD->GetSLRLOC(); }

	BOOL InitDayIf( int options, SLRDAY* pSD=NULL, float grRef=-1.f);
	void InitASHRAE();
	void IncRadASHRAE();
	void InitSim();
	float IRadSim( int iH, float glrad, float bmrad, float dfrad, float grRef=-1.f);
	void InitDay( int options, SLRDAY* pSD=NULL);
	SLRHR& SolarHr( int iT) { return ssd_pSD->sd_H[ iT]; }
	float CalcSHGF( int iT, float& shgfShd);
	float GetSHGF( float& shgfShd) const;
	void RefGlsGainSetup();
	void AccumGain(	double gainF[ 25], double areaSC,
		float ohDepth, float ohWD, float wHgt);
	void RadRatios( float HBar, float& KTBar, float& RBar, float& Rbn, float& rtn);
	double GFunc( double ha1, double ha2, double A, double B, double C, double ap);
	double RbSlrNoon() const;
	float FSunlit( int iT, float ohDepth, float ohWD, float wHgt,
		int options=0, int* pHow=NULL);
	float SolAirTemp( int iT, float tDb, float abs,
		float hOut,	float fSunlit=1.f, float lwCor=-999.f) const;
	void SolAirTemp( double tSolAir[ 24], const float tDb[ 24], float abs,
		float hOut,	float fSunlit, const float* pHrFSunlit=NULL, float lwCor=-999.f) const;
	void DbDump() const;

	float IRadTot( int iT, float fSunlit=1.f) const { return ssd_rad[ iT].Tot( fSunlit); }
	float IRadDir( int iT, float fSunlit=1.f) const { return ssd_rad[ iT].Dir( fSunlit); }
	float IRadDif( int iT) const { return ssd_rad[ iT].Dif(); }
	float IRadGrf( int iT) const { return ssd_rad[ iT].Grf(); }
	float IRadDifGrf( int iT) const { return ssd_rad[ iT].DifGrf(); }
	float AngIncDeg( int iT) const { return float( DEG( ssd_angInc[ iT])); }
	float AngProfVDeg( int iT) const { return float( DEG( ssd_angProfV[ iT])); }
	float AngProfHDeg( int iT) const { return float( DEG( ssd_angProfH[ iT])); }

};		// class SLRSURFDAY
//---------------------------------------------------------------------------
#if 0
class COA_SLRSURFDAY : public COA< SLRSURFDAY>
{ 	SLRSURF* cssd_pSS;	// SLRSURF for these SLRSURFDAYs

public:
	COA_SLRSURFDAY( SLRSURF* pSS);
	virtual ~COA_SLRSURFDAY();
	void SetSLRSURF( SLRSURF* pSS) { cssd_pSS = pSS; }
	SLRSURFDAY* AddOrGet( int iSSD, SLRDAY* pSD=NULL);
};	// class COA_SLRSURFDAY
#endif
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class SLRSURF: time-independent surface info
// class COA_SLRSURF: collection of SLRSURF
/////////////////////////////////////////////////////////////////////////////
const int IDHSSIM = -1;		// special GetSLRSURFDAY() day for hourly simulator
							//   Note: ISSD() assumes value is -1
//---------------------------------------------------------------------------
class SLRSURF
{
public:
	DWORD ss_key;			// re surface grouping, see MakeKey()
	float ss_azm;			// surface azimuth (deg, 0=N, 90=E, 180=S, 270=W)
	float ss_tilt;			// surface tilt (deg, 0=horiz up, 90=vert,
							//   180=horiz down)
	float ss_grRef;			// ground reflectivity as seen by surface
							//   (dimless, 0-1)

	double ss_dirCos[ 3];	// direction cosines of surface
	double ss_sinTilt;		// sin( ss_tilt)
	double ss_sinAzm;		// sin( ss_azm)
	double ss_cosAzm;		// cos( ss_azm)
	double CosTilt() const {  return ss_dirCos[ 2]; }
	double SinTilt() const {  return ss_sinTilt; }
	double SinAzmS() const {  return -ss_sinAzm; }
	double CosAzmS() const {  return -ss_cosAzm; }

	float ss_difX;			// diffuse factor = (1+cos( tilt)) / 2
	float ss_grfX;			// ground refl factor = grRef*(1-cos( tilt))/2
#if 0
	COA_SLRSURFDAY ss_oaSSD;	// OA of SLRSURFDAYs set up for all calc days
								//   [ 0]  set up at beg of each month during hourly simulation (resim.cpp)
								//         SLRLOC from simulator hourly weather file
								//         SLRDAY from simulation month
								//   [1..] set up at beg of loads calc for each calc day
								//         SLRLOC from current location (weather data possibly overridden)
								//         SLRDAYs from calc days
								// Note: ISSD( iD) handles iD+1 issues
#endif

public:
	SLRSURF();
	SLRSURF( DWORD key, float azm, float tilt, float grRef, int options=0);
	void Init( DWORD key, float azm, float tilt, float grRef, int options=0);
	static void SLDirCos( float azm, float tilt, double dirCos[ 3]);
	static BOOL SetOrWithSnap( float& azm, float& tilt, double dirCos[ 3],
		double& sinAzm, double& cosAzm, double& sinTilt);
	static int SnapAngle( float& ang, double& sinAng, double& cosAng,
		float tol=0.3f);
	void SetGrRef( float grRef);
	float CalcGrfX( float grRef) const;
	double RelAzm( double sunAzm) const;
	BOOL IsSameOrientation( float azm, float tilt) const
	{	return fAboutEqual( azm, ss_azm) && fAboutEqual( tilt, ss_tilt); }
	static int ISSD( int iD) { return iD+1; }
	BOOL IsVert() const { return fabs( SinTilt()) > 0.999999; }		// about 89.9 deg
	BOOL IsHoriz() const { return fabs( CosTilt()) > 0.999999; }	// about .1 deg
	SLRSURFDAY* GetSLRSURFDAY( int iD) const;
	SLRSURFDAY* AddOrGetSLRSURFDAY( int iD, SLRDAY* pSD=NULL);
	static DWORD MakeKey( float azm, float tilt, float grRef, float& ssAzm, float& ssTilt);
	void DbDump( int oMsk, const char* hdg="") const;
};		// class SLRSURF

#if 0
//---------------------------------------------------------------------------
class COA_SLRSURF : public COA< SLRSURF>
{
	class RSPROJ* ssa_pProj;				// parent project
	CMap< DWORD, DWORD, int, int&> ssa_Map;
							// detection of need for full re-init
							// see InitCalcDaysIf()
	int ssa_iLM;			//   CALCMETH at last full re-init	
	SLRLOC ssa_curSLRLOC;	//   SLRLOC at last full re-init

public:
	COA_SLRSURF( class RSPROJ* pProj=NULL);
	virtual ~COA_SLRSURF();
	void Clear();
	BOOL IsUsable() const;
	class RSPROJ* P() const { return ssa_pProj; }	// returns NULL if weather utility
	SLRSURF* FindOrAdd( float azm, float tilt, float grRef, int options=0);
	SLRSURF* GetSLRSURF( int iSS) const
	{	return GetAtSafe( iSS); }
	void InitCalcDaysIf( SLRDAY* SD[], int nCD, int iLM, int oMsk=0);
};		// class COA_SLRSURF
//=============================================================================
#endif

#if 0
/////////////////////////////////////////////////////////////////////////////
// class SLRCALC: combined SLRLOC / SLRDAY
/////////////////////////////////////////////////////////////////////////////
class SLRCALC
{
	SLRLOC sc_SL;
	SLRDAY sc_SD;

public:
	SLRCALC();
	SLRCALC( float lat, float lng, float tzn, float elev,
		int options=0, int oMsk=0);
	virtual ~SLRCALC();
	BOOL InitLOC( float lat, float lng, float tzn, float elev,
		int options=0, int oMsk=0);
	SLRDAY* GetSLRDAY() { return &sc_SD; }
	SLRLOC* GetSLRLOC() { return &sc_SL; }
	void Setup( int doy, int sltm=sltmLST)
	{	sc_SD.Setup( doy, sltm); }
	void SetupCalcDOY( int iMon, int sltm=sltmLST)
	{	sc_SD.SetupCalcDOY( iMon, sltm);	}
};		// class SLRCALC
//==========================================================================
#endif

/////////////////////////////////////////////////////////////////////////////
// forward inlines
/////////////////////////////////////////////////////////////////////////////
inline float SLRSURFDAY::Tilt() const { return ssd_pSS->ss_tilt; }
inline float SLRSURFDAY::Azm() const { return ssd_pSS->ss_azm; }
inline double* SLRSURFDAY::DirCos() const { return ssd_pSS->ss_dirCos; }
inline double SLRSURFDAY::CosTilt() const { return ssd_pSS->CosTilt(); }
inline float SLRSURFDAY::DifX() const { return ssd_pSS->ss_difX; }
inline double SLRSURFDAY::RelAzm( double sunAzm) const { return ssd_pSS->RelAzm( sunAzm); }
inline BOOL SLRSURFDAY::IsSameOrientation( float azm, float tilt) const
{	return ssd_pSS->IsSameOrientation( azm, tilt); }
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
// public functions
/////////////////////////////////////////////////////////////////////////////
inline int ITForIH( int iH) { return (iH+1)%24; }
float ASHRAETauModel(int options, float extBm, float sunZen,
	float tauB, float tauD, float& radDirN, float& radDirH);
float AirMass(float sunAlt);

#if defined( _DEBUG)
 void TestSHGF();
#endif

// solar.h end

