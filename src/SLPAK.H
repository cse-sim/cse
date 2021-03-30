// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

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

const double HA = k2Pi/24.;  	// earth rotation, rad/hr
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
x  void FC slaltfromazm( float azm);
x  void FC slhourfromalt( float alt);
#endif
void   FC slsdc( float azm, float tilt, float dcos[3]);
float slVProfAng( float relAzm, float cosZen);
SLLOCDAT * FC slinit ( float rlat, float rlong, float tzn, float siteElev);
SLLOCDAT * FC slselect( SLLOCDAT *pSlr);
void   FC slfree( SLLOCDAT **ppSlr);
void   FC slday( DOY doy, int timetype, int options=0);
void   FC sldec( float sldec, int timetype);
int slsurfhr( float sdircos[3], int ihr, float* pCosi, float* pAzm=NULL,
                    float* pCos=NULL);
void   FC slaltazm( float fhour, float* altp, float* azmp);
void   FC slaniso( float* pbeam, float* pdiff, SI ihr);
float slha( float fhour);
float slASTCor();

float slASHRAETauModel(	float sunZen, float tauB, float tauD, float& radDirN, float& radDifH);
bool slASHRAETauModelInv( double sunZen, double Ebn, double Edh, float& tauB, float& tauD);

#if 0
x RC slPerezSkyModel(double tilt, double azimuth, short iHrST, double direct_normal_beam, double horizontal_diffuse,
x	double ground_reflectivity, double& plane_incidence);
#else
RC slPerezSkyModel(float tilt, float azimuth, int iHrST, float direct_normal_beam, float horizontal_diffuse,
	float ground_reflectivity, float& incA, float& plane_beam, float& plane_diffuse_sky, float& plane_diffuse_ground);
#endif

  // end of slpak.h
