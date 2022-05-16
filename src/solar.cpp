////////////////////////////////////////////////////////////////////////////
// solar.cpp: solar calculations
//==========================================================================
// 
// Copyright (c) 1997-2021 The CSE Authors. All rights reserved.
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


#include "CNGLOB.H"

#include "TDPAK.H"

#include "solar.h"

#include "GMPAK.H" // gmcos2alt()


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

// local functions
static void RefGlsDOE2( double eta, float& trans, float& abso);
static void RefGlsASHRAE( double eta, float& trans, float& abso);
static double DCToAzm( double dirCos[ 3]);

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


/* slpak.cpp -- solar functions */

/* cleanup and some revisions made to provide for weather data
   work done for CEC 8-90 (chip):
   - new SLLOCDAT members .siteAlt and .pressureRatio added
   - some formulas revised according to SERI Consensus Summary
     #ifdef SERI enables these features.
   - all functions converted to ANSI format */

/*-------------------------------- OPTIONS --------------------------------*/
/* #define SERI	* enable code which uses formulas from SERI Consensus Summary
       (Solar Energy Related Calculations and Reference Data,
       Branch 215 Consensus Materials, SERI 1987?, obtained from
       Tom Stoffel, 8-8-90.  if not defined, use previous methods
       from other sources (see individual cases). */

/*------------------------------- INCLUDES --------------------------------*/

// solar constant (Btuh/sf): ext solar irradiance on normal
//   surface at mean earth/sun dist. SERI value: 1367 w/m2 (1 Btuh/ft2 = 3.1546 w/m2 )
#ifdef SERI
const float SolarConstant = 433.33f; // SERI value: 1367 w/m2 ( 1 Btuh/sf = 3.1546 w/m2
#else
const float SolarConstant = 434.f;	// previous value, source unknown
#endif

/*---------------------------- LOCAL VARIABLES ----------------------------*/

// pointer to current solar/location info thing.  Set by slinit() / slselect().
// Used by slday(), sldec, slsurfhr, slha, sldircos, slaniso
LOCAL SLLOCDAT *NEAR slloccur;

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

LOCAL void FC NEAR sldaydat(DOY doy, float *dec, float *eqtime, float *extbm);
LOCAL void FC NEAR sldircos(float ha, float dcos[3]);

/*------------------------------- COMMENTS --------------------------------*/

/* SOLAR CALCULATION ROUTINES :

PARAMETER DEFINITIONS :
float rlat      latitude in radians, + = N of equator, -Pi to Pi
float rlong     longitude in radians, W of Greenwich, -Pi to Pi
float tzn       hours west of GMT, -12 to 12 (e.g. PST = 8)
SI timetype     time system: solar(TMSOL), local standard(TMLST)
                             or local daylight savings(TMLDS) (slpak.h)
SI doy  	Julian day of year, non-leap years only, Jan-1=1, Dec-31=365
SI ihr          integer hour of day, 0 - 23, 0 = midnite
float fhour	time, 18.25F = 6:15 PM etc, in current time system.
    Fractions used eg for sunrise and sunset.
float ha	hour angle in radians, 0 = solar noon for date at longitude;
    same in all time systems.
*/

/* ***************************** TEST CODE ********************************* */
/* #define TEST */
#ifdef TEST
t UNMAINTAINED 8 - 9 - 90 t main() t
{
  t SLLOCDAT *ptr;
  t float lat, dlong, tzn, cq, ci, hour, alt, azm;
  t float sdcos[3], me, ms, mse;
  t double dec, eqtime, extbm;
  t SI doy, i, jd1, jd2, jdinc;
  t static float mjdat[6][4] = t {t {RAD(25.0F), 0.83F, 1.89F, 10.1F},
                                  t {RAD(30.0F), 0.83F, 1.63F, 5.40F},
                                  t {RAD(35.0F), 0.82F, 1.41F, 3.55F},
                                  t {RAD(40.0F), 0.81F, 1.25F, 2.60F},
                                  t {RAD(45.0F), 0.80F, 1.13F, 2.05F},
                                  t {RAD(50.0F), 0.79F, 1.01F, 1.70F} t};
  t t dminit();
  t ptr = slinit(RAD(40.0), RAD(122.2), 8.);
  t doy = 172;
  t slday(doy, sltmSLR);
  t gmcos2alt(ptr->dircos[12], &alt, &azm, 1);
  t slsdc(RAD(180.), RAD(90.), sdcos);
  t jd1 = 208;
  t jd2 = 300;
  t jdinc = 1;
  t for (doy = jd1; doy < jd2; doy += jdinc) t
  {
    t #if 0 t sldaydat(doy, &dec, &eqtime, &extbm);
    t printf("Day %d   Decl = %f\n", (INT)doy, DEG(dec));
    t #elseif 0 t printf("\n%d  \n", (INT)doy);
    t slday(doy, sltmSLR);
    t for (i = 0; i < 24; i++) t
    {
      cq = slcoshr(sdcos, i);
      t ci = slcosinc(sdcos, (float)i + .5);
      t printf("   %d    %f   %f\n", (INT)i, cq, ci);
      t
    }
    t #else t while (1) t
    {
      doy = 208;
      t printf("\nHour: ");
      t scanf("%f", &hour);
      t if (hour <= 0.) t break;
      t for (lat = 25.; lat < 51.; lat += 5.) t
      {
        ptr = slinit(RAD(lat), RAD(122.2), 8.);
        t slday(doy, sltmSLR);
        t slaltazm(hour, &alt, &azm);
        t printf("\nLat: %2.0f  MJ mult: %.2f     Alt: %f  Azm: %f",
                 t lat,
                 TANSP(alt),
                 DEG(alt),
                 DEG(azm));
        t
      }
      t
    }
    t break;
    t #endif t #if 0 t while (1) t
    {
      printf("\nJday: ");
      t scanf("%hd", &doy);
      t if (doy <= 0) t break;
      t for (i = 0; i < 6; i++) t
      {
        ptr = slinit(mjdat[i][0], RAD(122.2), 8.);
        t printf("\nLat: %.0f", DEG(mjdat[i][0]));
        t slday(doy, sltmSLR);
        t if (gcnw() > 0) t break;
        t slhourfromalt(ATANSP(mjdat[i][1]));
        t slhourfromalt(ATANSP(mjdat[i][2]));
        t slhourfromalt(ATANSP(mjdat[i][3]));
        t
      }
      t
    }
    t break;
    t #endif t
  }
  t
} /* main */
t t /* #ifdef TEST ... */
  t //======================================================================
    t void FC slaltfromazm(azm) t t /* test routine that calculates the altitude based on the
                                  azimuth t    for a the current day */
  t t float azm;
t t
{
  t float azmx, alt, hour, lasthour, hourx, hoursmall = 1., hourbig = 12.;
  t SI small, loop;
  t t hour = 1.;
  t lasthour = hour;
  t for (loop = 0, small = YES; loop < 20; loop++) t
  {
    slaltazm(hour, &alt, &azmx);
    t if (fAboutEqual(azmx, azm)) t break;
    t hourx = hour;
    t if (azmx < azm) t
    {
      hoursmall = hour;
      t if (small) t hour = (hour + hourbig) / 2.;
      t else t hour = (hour + lasthour) / 2.;
      t small = YES;
      t
    }
    t else t
    {
      hourbig = hour;
      t if (!small) t hour = (hour + hoursmall) / 2.;
      t else t hour = (hour + lasthour) / 2.;
      t small = NO;
      t
    }
    t lasthour = hourx;
    t
  }
  t printf("\n   Hour: %f (azm: %f)  Alt: %f   Mult: %f", t hour, DEG(azmx), DEG(alt), TANSP(alt));
  t
} /* slaltfromazm */
t t /* #ifdef TEST ... */
  t //======================================================================
    t void FC slhourfromalt(alt) t t /* test routine that calculates the altitude based on the
                                   azimuth t    for a the current day */
  t t float alt;
t t
{
  t float azm, altx, hour, lasthour, hourx, hoursmall = 1., hourbig = 12.;
  t SI small, loop;
  t t hour = 1.;
  t lasthour = hour;
  t for (loop = 0, small = YES; loop < 20; loop++) t
  {
    slaltazm(hour, &altx, &azm);
    t if (fAboutEqual(altx, alt)) t break;
    t hourx = hour;
    t if (altx < alt) t
    {
      hoursmall = hour;
      t if (small) t hour = (hour + hourbig) / 2.;
      t else t hour = (hour + lasthour) / 2.;
      t small = YES;
      t
    }
    t else t
    {
      hourbig = hour;
      t if (!small) t hour = (hour + hoursmall) / 2.;
      t else t hour = (hour + lasthour) / 2.;
      t small = NO;
      t
    }
    t lasthour = hourx;
    t
  }
  t printf("\n   Hour: %6.2f (azm: %6.2f)  Alt: %6.2f   Mult: %6.2f",
           t hour,
           DEG(azm),
           DEG(altx),
           TANSP(altx));
  t
} /* slhourfromalt */
t
t /* yet another test main() */
t main()
t
{
  t DOY doy;
  t float dec, eqtime, extbm;
  t t for (doy = 0; ++doy <= 365;) t
  {
    sldaydat(doy, &dec, &eqtime, &extbm);
    t printf("%3.3d,%8.4f,%8.4f,%8.3f\n", t doy, DEG(dec), eqtime, extbm);
    t
  }
  t
}
#endif /* TEST */

//======================================================================
void FC slsdc(   // direction cosines from azm/tilt of a surface
  float azm,     // Surface azm (radians)
  float tilt,    // Surface tilt (radians)
  float dcos[3]) // Array for returning direction cosines
{
  double st = sin(tilt);
  dcos[0] = float(sin(azm) * st);
  dcos[1] = float(cos(azm) * st);
  dcos[2] = cos(tilt);
} // slsdc
//-----------------------------------------------------------------------------
float slVProfAng( // vertical profile angle
  float relAzm,   // relative azimuth of beam to surface normal
                  //   if (fabs( relAzm) > Pi/2), then sun is behind surface
  float cosZen)   // cos( solar zenith angle)
                  //  1=sun at zenith
                  //  0=sun at horizon
                  // <0=sun below horizon

// NOTE: surface assumed vertical
// returns vertical profile angle, radians
{
  // TODO: non-vert surfaces, horiz profile, 12-30-08
  float profAng;
  if (cosZen >= .9999f || cosZen < .001f)
    profAng = kPiOver2; // sun down or at zenith
  else
  {
    float cosRA = cos(relAzm);
    if (cosRA <= .001f)
      profAng = kPiOver2; // sun behind wall
    else
      profAng = atan(cosZen / sqrt(1. - cosZen * cosZen) / cosRA);
  }
  return profAng;
} // slVProfAng
//======================================================================
SLLOCDAT *FC
slinit(/* Allocate, init, and select a SLLOCAT structure */

       float rlat,     // latitude, +=N, radians
       float rlong,    // longitude, +=W (non-standard), radians
       float tzn,      // time zone (hrs west of GMT. EST=5, PST=8, etc. )
       float siteElev) // site elevation (ft above sea level).
                       // Used only with global/direct functions (eg dirsimx.c, gshift.c)
                       // SO passing 0 is generally harmless.

/* Returns pointer to SLLOCDAT structure initialized for specified location.
   This location is now also the current location for subsequent
   calls to slpak routines.  Selection can be changed with slselect().
   When done, caller must free SLLOCDAT with slfree(). */
{
  SLLOCDAT *pSlr = new SLLOCDAT;                    // allocate a SLLOCDAT object on heap
  if (!pSlr)                                        // if failed
    err(ABT, "Out of memory (slpak.cpp:slinit())"); // issue message, abort program (no return)
  memset(pSlr, 0, sizeof(SLLOCDAT));                // zero the SLLOCDAT object
  pSlr->rlat = rlat;                                /* store arguments */
  pSlr->sinlat = (float)sin(rlat);                  /* and derived quantities. */
  pSlr->coslat = (float)cos(rlat);
  pSlr->rlong = rlong;       /* longitude */
  pSlr->tzn = tzn;           /* time zone, hours west of GMT */
  pSlr->siteElev = siteElev; /* site elevation, ft */
  pSlr->pressureRatio = (float)exp(-0.0001184 * 0.3048 * siteElev);
  /* site pressure ratio: nominal ratio of surface pressure to sea level pressure.
     Formula appears in Perez code (dirsim.c) and is documented in SERI Consensus
   Summary (full reference above), p. 17.
               NOTE: needs reconciliation with psychro1.c:psyAltitude. 8-9-90 */
  return slselect(
    pSlr); // make current: ptr for other slpak fcns. ret ptr, for later slselects and slfree
} // slinit
//======================================================================
SLLOCDAT *FC slselect(/* Sets current solar/location data to that given by arg */

                      SLLOCDAT *pSlr) /* pointer returned by an earlier slinit call */

/* returns pSlr as convenience (new feature 8-9-90) */
{
  return (slloccur = pSlr); /* set file-global for slpak fcns to use */
} /* slselect */

//======================================================================
void FC slfree( // Free a SLLOCDAT structure

  SLLOCDAT **ppSlr) // ptr to ptr. nop if ptr is NULL; ptr is set NULL: redundant calls ok.
{
  delete *ppSlr; // free heap storage occupied by object
  *ppSlr = NULL; // erase no-longer-valid pointer
} // slfree

//======================================================================
void FC slday( // set up daily data in current SLLOCDAT

  DOY doy,            // day of year, Jan-1 = 1, Dec-31 = 365
  int timetype,       // time system assumed for hourly tables (slpak.h)
                      //   sltmSLR:   Solar time
                      //   sltmLST:     Local standard
                      //   sltmLDT:     Local daylight time
  int options /*=0*/) // option bits
                      //   1: skip setup if SLLOCDAT.doy == doy

/* sets members re declination of earth's axis, time / hour angle conversion
   (.tmconst), hourly sunupf[], dircos[], slazm[], etc. */
{
  if (!slloccur)
    err(PABT, "NULL slloccur");

  if ((options & 1) && slloccur->doy == doy && slloccur->timetype == timetype)
    return; // already set for doy

  slloccur->doy = doy;

  float dec;
  sldaydat(                     /* compute values, does not ref slloccur directly, local */
           doy,                 /*   current day of year */
           &dec,                /*   declination returned */
           &(slloccur->eqtime), /*   equation of time returned */
           &(slloccur->extbm)); /*   extraterrestrial beam intensity retrnd */

  sldec(dec, timetype); // set declination-related slloccur members, below.

} // slday
//======================================================================
LOCAL void FC NEAR sldaydat(

  /* Calculate declination, equation of time,
     and extraterrestrial beam intensity for a day */

  DOY doy,       /* Day of year, Jan-1 = 1 */
  float *dec,    /* Receives declination of earth's axis (radians) */
  float *eqtime, /* .. Equation of time (hrs to add to local for solar time) */
  float *extbm)  /* .. Extraterrestial beam rediation intensity (Btuh/sf)*/
{

#ifdef SERI

  /* Story: SERI methods use Fourier series for declination, equation of time
     and earth/sun distance.  The general form is:
    x = c0 + c1*cos(t) + c2*sin(t) + c3*cos(2t) + c4*sin(2t) ...
     where t is the "day angle" = 2*Pi*(doy-1)/365.  In this implementation,
     the array sc is set to cos(t), sin(t), cos(2t), sin(2t), cos(3t), and
     sin(3t).  VIProd() is used form sum( c[i] * sc[i]) */

  SI i;
  double t;
  float sc[6]; /* sine/cosine array */
  /* coefficients: from SERI Consensus Summary (see full ref at file beg) */
  static float cDec[7] = /* coefficients for declination */
    {0.006918f, -0.399912f, 0.070257f, -0.006758f, 0.000907f, -0.002697f, 0.001480f};
  static float cEtm[5] = /* coefficients for equation of time */
    {0.000075f, 0.001868f, -0.032077f, -0.014615f, -0.040849f};
  static float cEsd[5] = /* coefficients for earth/sun distance */
    {1.000100f, 0.034221f, 0.001280f, 0.000719f, 0.000077f};

  /* set sc array to cos/sin values of "day angle" */
  for (i = -1; ++i < 3;)
  {
    t = (i + 1) * 0.0172142 *
        (doy - 1); // angle 0 to 2Pi (0.0172142 = 2Pi/365) over year plus 2nd and 3rd harmonic
    sc[2 * i] = (float)cos(t);     // cos(t), cos(2t), cos(3t)
    sc[2 * i + 1] = (float)sin(t); // sin(t), sin(2t), sin(3t)
  }

  // calculate values using VIProd()
  *dec = cDec[0] + fVecIprod(sc, cDec + 1, 6); // declination (radians)
  *eqtime =                                    // equation of time (hours
    cEtm[0] + VIProd(sc, 4, cEtm + 1)          // radians
                / (float)HaPerHr;                   // convert to hours
  *extbm =                                     // extrater beam intensity (Btuh/sf)
    (float)SolarConstant                              // solar constant
    * (cEsd[0] + VIProd(sc, 4, cEsd + 1));     // earth/sun distance correction factor

#else  /* prior non-SERI code follows */

  double gam, gam2, s1, c1, s2, c2, sindec;
  /* Declination and equation of time.
     These curves are from SOLMET Vol. 2 - Final Report, page 49 - 53. */
  gam = 0.017203 * (doy - 1); /* angle 0 to 2Pi over year */
  gam2 = 2. * gam;            /* twice that for "2nd harmonic" */
  s1 = sin(gam);
  c1 = cos(gam);
  s2 = sin(gam2);
  c2 = cos(gam2);
  sindec = /* sine of declination */
    0.39785 * sin(4.885784 + gam + 0.033420 * s1 - 0.001388 * c1 + 0.000348 * s2 - 0.0000283 * c2);
  *dec = (float)asin(sindec);
  *eqtime = (float)(-0.123570 * s1 + 0.004289 * c1 - 0.153809 * s2 - 0.060783 * c2);

  *extbm = SolarConstant * (1.f + 0.033f * c1);
  /* today's et beam rad is solar constant + variation */
#endif /* SERI */

} /* sldaydat */
//======================================================================
void FC sldec(/* Set declination-related data in current SLLOCDAT struct */

              float dec,    // declination of earth's axis, today, in radians
              int timetype) // time system for setting solar tables:
                            //  sltmSLR, sltmLST, or sltmLDT (slpak.h).
{
  double hasr, hass; /* hour angles of sun rise, sun set (0 = solar noon) */
  double hsr, hss;   /* hours of sun rise, sun set (curr time system) */
  SI ihsr, ihss;     /* .. truncated to integers */
  double tmconst;    /* add to hour/HaPerHr to get hour angle */
  double sindec;
  double ha, fup, r;
  float alt;
  SI ihr;

  /* Store args */
  slloccur->rdecl = dec;         /* declination in radians */
  slloccur->timetype = timetype; /* store arg */

  /* Quantities derived from declination */
  slloccur->cosdec = (float)cos(dec);
  sindec = sin(dec);
  slloccur->sdsl = (float)(sindec * slloccur->sinlat);
  slloccur->sdcl = (float)(sindec * slloccur->coslat);
  slloccur->cdsl = (float)(slloccur->cosdec * slloccur->sinlat);
  slloccur->cdcl = slloccur->cosdec * slloccur->coslat;

  /* Sunset hour angle (handle arctic/antarctic cases) */
  r = -slloccur->sdsl / slloccur->cdcl;
  if (r < -1.0)
    r = -1.0;
  else if (r > 1.0)
    r = 1.0;
  hasr       /* hour angle sun rise: -sunset */
    = -(hass /* hour angle sun set */
        = acos(r));
  slloccur->hasunset = (float)hass;

  /* Time conversion constant: add to hour angle to convert current type system
             to solar and make 0 at noon */
  tmconst = -kPi; /* converts 0 at midnite to 0 at noon*/
  /* converting solar to local: add equation of time, and
     adjust for longitude relative to center of time zone */
  if (timetype != sltmSLR)          /* if not solar, convert local to solar */
    tmconst += HaPerHr                     /* radians per hour: convert to angle*/
                 * (slloccur->eqtime  /* equation of time */
                    + slloccur->tzn)  /* time zone (longitude of zn ctr).. */
               - slloccur->rlong;     /* .. less actual longitude */
  if (timetype == sltmLDT)            /* if daylight local time */
    tmconst -= HaPerHr;                    /* 1 hour earlier */
  slloccur->tmconst = (float)tmconst; /* note tmconst also used below, also
                 slha calls below use .tmconst. */

  /* compute, for 24 hours of selected time type of day,
     Solar direction cosines, azimuth, and fraction of hour sun up. */

  hsr = (hasr - tmconst) / HaPerHr; /* convert hour angle to hour */
  ihsr = (SI)hsr;              /* truncate sunrise hour to integer */
  hss = (hass - tmconst) / HaPerHr; /* sunset ... */
  ihss = (SI)hss;
#ifdef PRINTSTUFF
  x printf("\nSunrise = %f    Sunset = %f  Hours up = %f", x hsr, hss, hss - hsr);
#endif
  for (ihr = -1; ++ihr < 24;) /* hours of day */
  {
    if (ihr >= ihsr && ihr <= ihss + 1) /* if sun up in hr or hr b4 */
    {
      if (ihr == ihsr) /* sunrise hour */
      {
        ha = hasr;                     /* compute for sunrise */
        fup = (float)(ihsr + 1) - hsr; /* fractn of hour sun is up */
      }
      else if (ihr == ihss) /* hour during which sun sets */
      {
        ha = slha((float)ihr);   /* ha = start of hr as usual */
        fup = hss - (float)ihss; /* fractn of hour sun is up */
      }
      else if (ihr == ihss + 1) /* hr starting after sunset */
      {
        ha = hass; /* gets sunset dircos / azm */
        fup = 0.;  /* sun not up */
      }
      else /* hour entirely in daytime */
      {
        ha =                /* hour angle for computations below */
          slha((float)ihr); /* hour to hour angle. local.*/
        fup = 1.;           /* sun up entire hour */
      }
      /* Calculate direction cosines and azimuth for time "ha" */
      sldircos(ha, slloccur->dircos[ihr]); /* direction cosines, local */
      gmcos2alt(                           /* Alt and azm from dircos (gmutil.c) */
                slloccur->dircos[ihr],     /* Dircos just calc'd */
                &alt,                      /* Altitude, not stored */
                &(slloccur->slrazm[ihr]),  /* Azm, save for possible use
                                               in slcoshr() below */
                1);                        /* Use improved accuracy version */
      if (slloccur->slrazm[ihr] < 0.F)
        slloccur->slrazm[ihr] += (float)k2Pi;
      /* Keep solar azimuth in 0 - 2Pi range
         so slsurfhr interpolation works */
    }
    else /* sun down during hour (1st night hour done above). */
    {
      /* explicit zeroing probably nec as night hours change during year */
      fup = 0.;                    /* fraction up: none */
      slloccur->slrazm[ihr] = 0.F; /* 0 azimuth */
      memset(slloccur->dircos[ihr], 0, 3 * sizeof(float));
      /* zero direction cosines */
    }
    slloccur->sunupf[ihr] = (float)fup; /* store fraction hr sun up */
  }                                     /* for ihr 0 to 23 */
#ifdef PRINTSTUFF
  x for (ihr = 0; ihr < 24; ihr++) x printf("\n%d  %f  %f  %f  %f  %f",
                                            (INT)ihr,
                                            slloccur->sunupf[ihr],
                                            x slloccur->dircos[ihr][0],
                                            slloccur->dircos[ihr][1],
                                            x slloccur->dircos[ihr][2],
                                            slloccur->slrazm[ihr]);
  x printf("\n");
#endif
} /* sldec */

//======================================================================
int slsurfhr( // Calculate solar values for a surface for an hour

  float sdircos[3],       // Direction cosines for surface
  int ihr,                // Hour for which to calculate. 0 = midnight - 1AM etc.
                          //   Time depends on last setup call to slday().
  float *pCosi,           // NULL or pointer for returning approximate average
                          //   cosine of incident angle.  See notes
  float *pAzm /*=NULL*/,  // NULL or pointer for returning solar azimuth
                          //   (radians, 0 = north, Pi/2 = east etc.) for midpoint
                          //   of time sun is above surface during hour.
  float *pCosz /*=NULL*/) // NULL or receives cosine of solar zenith angle for
                          //   midpoint of time sun is above the surf during hr

/* *pCosi receives the approx average cosine of angle of incidence of solar
   beam on surface during the hour.  If the sun is not above the surface for
   the entire hour, this is the average over the fraction of the hour that
   surface is exposed to the sun, times (weighted by) that fraction.

   This value is intended for direct use with solar beam value.  It is adjusted
   (reduced) for time sun is behind the surface but *NOT* adjusted for time
   sun is below horizon, since solar beam data already includes that effect.

   *pAzm and *pCosz are required for calling the CALRES shading routine
   (shadow), and are evaluated at the approximate mid point of the time the
   sun is above the surface.  They should thus be consistent with *pCosi.

   Note that slday() must be called for desired day prior to this.

   Returns TRUE  if sun is up with respect to surface (and horizon) at any
                 time during hour.  *pCosi, *pAzm, and *pCosz are meaningful.
           FALSE if no direct solar on surface this hour.
                 *pCosi = 0, *pAzm and *pCosz undefined. */
{
  float c1, c2, fc1 = 0, fc2 = 0, faz1 = 0, faz2;

  // If sun below HORIZON for the entire hour, no need to calculate
  if (slloccur->sunupf[ihr] == 0.F) // if sun below horizon entire hour
  {
    if (pCosi)
      *pCosi = 0.f;
    return FALSE;
  }
  /* Note: if sun above horizon only part of hour, sldec put sunrise/sunset
     dircos[]/slrazm[] values at the hour limits, so values computed here
     come out to be average positions over time sun above horizon in hour
     (code elsewhere must multiply by sunupf[ihr] to weight ENERGY values).*/

  // Compute dot products at beg and end of hour.  Set pos flags TRUE if
  // product is positive (meaning sun is above surface at beg/end of hour)
  int ihx = ihr < 23 ? ihr + 1 : 0;
  int pos1 = (c1 = DotProd3(sdircos, slloccur->dircos[ihr])) >= 0.f;
  int pos2 = (c2 = DotProd3(sdircos, slloccur->dircos[ihx])) >= 0.f;

  // if sun below SURFACE for the entire hour, stop
  if (!pos1 && !pos2)
  {
    if (pCosi)
      *pCosi = 0.f;
    return FALSE;
  }

  /* Derive weighting factors for calculating averaged/weighted results:
         faz1, faz2:  for azimuth: half way between beg, end of time above surf
         fc1, fc2:    for cosine of incident angle: also weight for fraction of
                hour sun above surface in same computation; use the fact
                that cosi is 0 at rise/set (vs surface). */
  if (pos1 && pos2)          // if above surface entire hour
    faz1 = fc1 = fc2 = 0.5f; // use equal parts of start, end
  else if (pos1)             // if sets (re surface) during hour
  {
    fc1 =                        /* fractn of starting cosi to use is */
      (float)(0.5                /* avg of 2 items (other cosi is 0) */
              * c1 / (c1 - c2)); /* times fractn of hour sun above surf:
                          weight cosi result for duration */
    fc2 = 0.F;                   /* other input to cosi average is 0 at
                                sunset (use none of negative c2) */
    /* faz2 = fc1  falls out of  faz2 = 1.F - faz1  below:
     * if sun up whole hour, use 1/2 ending azm, else less */
    faz1 = 1.F - fc1; /* and the rest starting azm to get
                 (start + set) / 2 */
  }
  else if (pos2) /* sun rises over surface during hour */
  {
    fc1 = 0.F;                     /* use none of start-hr cosi (below surface) */
    faz1                           /* fraction of starting azimuth */
      = fc2                        /* fraction ending cosi */
      = (float)(0.5                /* 1/2: averaging 2 items */
                * c2 / (c2 - c1)); /* times fract of hr sun above surf */
                                   /* faz2 = 1.F - faz1  is below */
  }

  /* Calculate results */
  if (pCosi)                      // no return if NULL ptr (rob 3-91)
    *pCosi = fc1 * c1 + fc2 * c2; /* cos(incident angle) is weighted
                 average of dircos dot products */
  faz2 = 1.F - faz1;
  if (pAzm)
    *pAzm = faz1 * slloccur->slrazm[ihr]    // Azimuth is calc'd
            + faz2 * slloccur->slrazm[ihx]; //   from slday values
  if (pCosz)
    *pCosz = faz1 * slloccur->dircos[ihr][2] // z dircos is cosz
             + faz2 * slloccur->dircos[ihx][2];

#ifdef PRINTSTUFF
  x printf("\n%d  %f  %f  %f", (INT)ihr, *pCosi, *pAzm, *pCosz);
#endif
  return TRUE;
} // slsurfhr
//----------------------------------------------------------------------
float slSunupf(int ihr)
{
    return slloccur->sunupf[ihr];
}       // slSunupf

//-----------------------------------------------------------------------------
int slSunupBegEnd(		// beg / end times when sun is up during hour
    int ihr,		// hour (0-23)
    float& hrBeg,	// returned: start of sun up period
    float& hrEnd,   // returned: end of sun up period
    float sunupf/*=-1.f*/)  // fraction sun up during hour
                            //   if < 0., call slSunupf
// returns 0 iff sun is down for entire hour
//         1 iff sunrise hour
//         2 iff sun is up for entire hour
//         3 iff sunset hour
{
    if (sunupf < 0.f)
        sunupf = slSunupf(ihr);
    int ret;
    if (sunupf < 0.0001f)
    {   hrBeg = hrEnd = 0.f;
        ret = 0;
    }
    else
    {   hrBeg = float(ihr);
        hrEnd = float(ihr + 1);
        if (sunupf < 1.f)
        {   if (ihr < 12)
            {   hrBeg = hrEnd - sunupf;
                ret = 1;
            }
            else
            {   hrEnd = hrBeg + sunupf;
                ret = 3;
            }
        }
        else
            ret = 2;
    }
    return ret;
}       // slSunupBegEnd
//======================================================================
float slha( // Calculate hour angle for hour of day

  float hour) // hour for which hour angle is reqd

/* hour can be fractional and is solar, local std, or local daylight
   time per last call to slday. */

/* must call slday() for day first (uses slloccur->tmconst) */

/* Returns hour angle (radians, 0 = solar noon) */
{
  return hour * HaPerHr            // HaPerHr: earth rotation per hr (slpak.h)
         + slloccur->tmconst; // .tmconst: adjust current time system to
                              // solar, make noon 0. set by sldec(). */
} // ::slha
//----------------------------------------------------------------------
float slASTCor() // apparent solar time correction
// AST = LxT + astCor (X = S or D)
// returns time diff
{
  // note non-standard definitions of rlong and tzn (+ = W)
  // test example: location W of time meridian
  //               -> rlong > tzn
  //               -> astCor < 0 (neglecting eqtime)
  //               -> AST earlier than LST as expected
  // astCor = eqtime - (rlong/HaPerHr - tzn)
  // use precalculated tmconst (which also includes DST)
  return (slloccur->tmconst + kPi) / HaPerHr;
} // ::slASTCor
//======================================================================
LOCAL void FC NEAR sldircos(

  /* Compute direction cosines of sun for current day and location */

  float ha,      /* Hour angle (radians, 0 = solar noon, + = afternoon) */
  float dcos[3]) /* Array to store direction cosines */

/* Declination-related constants must be set up in SLLOCDAT structure */
{
  float sinhang, coshang;

  sinhang = (float)sin(ha);
  coshang = (float)cos(ha);
  dcos[0] = -slloccur->cosdec * sinhang;
  dcos[1] = slloccur->sdcl - slloccur->cdsl * coshang;
  dcos[2] = slloccur->sdsl + slloccur->cdcl * coshang;

} /* sldircos */
//======================================================================
void FC slaltazm(/* Calculate solar altitude and azimuth for hour */

                 float hour,  /* hour of day: solar, local std, or lcl daylt time
                           per last call to slday, 0.F = midnite, fracs ok. */
                 float *altp, /* ptr for returning solar altitude */
                 float *azmp) /* ptr for returning solar azimuth */

/* must call slday() for day first (uses slloccur->tmconst) */
{
  float dcos[3];

  sldircos(            /* compute solar dir cosines, just above */
           slha(hour), /* convert time to hour angle.
                      uses .tmconst. local */
           dcos);      /* receives result */
  gmcos2alt(           /* compute alt-azm for dircos. gmpak.c */
            dcos,
            altp,
            azmp,
            1); /* use rob's deflaked method 7-88 */
} /* slaltazm */
//======================================================================
void FC slaniso(		// Adjust beam and diffuse radiation values
						// according to Hay anisotropic sky model
	float& beam,	// beam value (Btu/sf): used/replaced
	float& diff,	// diffuse value (Btu/sf): used/replaced
	int ihr)		// Hour of day (0 - 23, 0 = midnight to 1 AM).
					//   Meaning of hour (LST, solar time, etc.)
					//   depends on how SLLOCDAT was set up by slday()

/* must call slday() for day first (uses slloccur members) */

/* Adjusted values of beam and diffuse *REPLACE* the current values */

/* Story: Code assumes sky is modelled as istropic (uniform) hemisphere of
	sky radiation plus direct sun beam.  But actually, sky is brighter
	near sun, near horizon, etc.  Hay model approximates reality better
	with old code by increasing beam to approximate some of the extra sky
	brightness near sun.  Rob per Chip, 12-89. */

	/* Recoded 2-10-89 based on cp4sim equivalent function.  Features --
	1.  Doesn't bother unless there is a little beam
	2.  Constrains fb to be .8 max.  This prevents wild values from
		early morning observations.
	3.  Derives fd (diffuse factor) from fb rather than from basic
		Hay formula, giving same result unless fb hit .8 limit.  In all
		cases, total horiz is preserved */
{
	if (beam > 5.f) // if a little beam
	{	float f = slloccur->sunupf[ihr];
		if (f > .05f)	// if sun up > 3 mins
		{	int ihx = (ihr + 1) % 24;
			float c1 = slloccur->dircos[ihr][2];
			float c2 = slloccur->dircos[ihx][2];
			bool pos1 = (c1 >= 0.f);
			bool pos2 = (c2 >= 0.f);
			float cosi;
			if (pos1 && pos2)
				cosi = 0.5f * (c1 + c2);
			else if (pos1)
				cosi = 0.5f * c1 * c1 / (c1 - c2);
			else if (pos2)
				cosi = 0.5f * c2 * c2 / (c2 - c1);
			else
				cosi = 0.f;

			float fb = beam / (f * slloccur->extbm);	// Beam factor
			if (fb > .8f)
		        fb = .8f;		// limit to 0.8
			float fd = (diff * f * fb) / (beam * cosi);
					// derive diff. fctr from beam
			beam *= (1.f + fd); // adjust caller's values
			diff *= (1.f - fb);
		}
	}
}		// slansio
//=============================================================================================
void FC slaniso(/* Adjust beam and diffuse radiation values
       according to Hay anisotropic sky model */

    float* pbeam, /* Pointer to beam value (Btu/sf): used/replaced. */
    float* pdiff, /* Pointer to diffuse value (Btu/sf): used/replaced.*/
    SI ihr)       /* Hour of day (0 - 23, 0 = midnight to 1 AM).
               Meaning of hour (LST, solar time, etc.)
               depends on how SLLOCDAT was set up by slday(). */

               /* must call slday() for day first (uses slloccur members) */

               /* Adjusted values of beam and diffuse *REPLACE* the current values */

               /* Story: Code assumes sky is modelled as istropic (uniform) hemisphere of
                  sky radiation plus direct sun beam.  But actually, sky is brighter
                  near sun, near horizon, etc.  Hay model approximates reality better
                  with old code by increasing beam to approximate some of the extra sky
                  brightness near sun.  Rob per Chip, 12-89. */

                  /* Recoded 2-10-89 based on cp4sim equivalent function.  Features --
                    1.  Doesn't bother unless there is a little beam
                    2.  Constrains fb to be .8 max.  This prevents wild values from
                        early morning observations.
                    3.  Derives fd (diffuse factor) from fb rather than from basic
                        Hay formula, giving same result unless fb hit .8 limit.  In all
                        cases, total horiz is preserved */
{
    SI ihx;
    float c1, c2, cosi = 0, f, fb, fd;

    if (*pbeam > 5.f) /* if a little beam */
    {
        if ((f = slloccur->sunupf[ihr]) > .05f) /* if sun up > 3 mins */
        {
            ihx = (ihr + 1) % 24;
            c1 = slloccur->dircos[ihr][2];
            c2 = slloccur->dircos[ihx][2];
            bool pos1 = (c1 >= 0.f);
            bool pos2 = (c2 >= 0.f);
            if (pos1 && pos2)
                cosi = (float)(0.5f * (c1 + c2));
            else if (pos1)
                cosi = (float)(0.5f * c1 * c1 / (c1 - c2));
            else if (pos2)
                cosi = (float)(0.5f * c2 * c2 / (c2 - c1));

            fb = *pbeam / (f * slloccur->extbm); /* Beam factor */
            if (fb > .8F)
                fb = .8F; /* Limit to .8 */
            fd = (*pdiff * f * fb) / (*pbeam * cosi);
            /* derive diff. fctr from beam */
            *pbeam *= (1.F + fd); /* adjust caller's values */
            *pdiff *= (1.F - fb);
        }
    }
} /* slansio */


////////////////////////////////////////////////////////////////////////////
// ASHRAE solar
////////////////////////////////////////////////////////////////////////////

float slASHRAETauModel( // ASHRAE "tau" clear sky model
	float sunZen,         // solar zenith angle, radians
	float tauB,           // ASHRAE beam "pseudo optical depth"
	float tauD,           // ditto diffuse
	float &radDirN,       // returned: direct normal irradiance, units per extBm
	float &radDifH)       // returned: diffuse horizontal irradiance, units per extBm

// returns radGlbH, units per extBm
{
	float extBm = slloccur->extbm;

	return ASHRAETauModel( // ASHRAE "tau" clear sky model
		1,
		extBm,
		sunZen,
		tauB,
		tauD,
		radDirN,
		radDifH);
} // slASHRAETauModel
//-----------------------------------------------------------------------------
static bool slASHRAETauModelInv( // derive tauB/tauD from irradiance
	double Ebn,                    // beam normal irradiance (at m)
	double Edh,                    // diffuse horizontal irradiance (at m)
	double E0,                     // exterrestrial normal irradiance
	double m,                      // air mass, dimless
	double &tauB,                  // returned
	double &tauD)                  // returned
// Ebn, Edh, and E0 should have same units
// Based on Python implementation by Michael Roth
// ASHRAE 2017 coefficients implicit
{
  double Kb = Ebn / E0;
  double Kd = Edh / E0;

  double logm = log(m);
  double loglogKb = log(log(1.0 / Kb));
  double loglogKd = log(log(1.0 / Kd));
  double tol = 1e-8;

  // First guess
  tauB = 0.75;
  tauD = 1.5;

	bool bConverge = false;
	for (int iT = 0; iT < 20 && !bConverge; iT++)
	{ // update ab & ad
		double ab = 1.454 - 0.406 * tauB - 0.268 * tauD + 0.021 * tauB * tauD;
		double ad = 0.507 + 0.205 * tauB - 0.080 * tauD - 0.190 * tauB * tauD;

		// Calculate F
		// Note: Took log twice of equations (17) and (18). F is the
		// difference between LHS & RHS.  We want F to approach zero.
		double Fb = log(tauB) + ab * logm - loglogKb;
		double Fd = log(tauD) + ad * logm - loglogKd;

		// calculate Jacobian
		double Jbb = 1.0 / tauB + logm * (-0.406 + 0.021 * tauD);
		double Jbd = logm * (-0.268 + 0.021 * tauB);
		double Jdb = logm * (+0.205 - 0.190 * tauD);
		double Jdd = 1.0 / tauD + logm * (-0.080 - 0.190 * tauB);

		// solve system {-F} = [J]{dtau} using Cramer's rule
		double detJ = Jbb * Jdd - Jdb * Jbd;
		double dtauB = (-Fb * Jdd + Fd * Jbd) / detJ;
		double dtauD = (-Jbb * Fd + Jdb * Fb) / detJ;

		// update taus
		tauB += dtauB;
		tauD += dtauD;

		bConverge = abs(dtauB) <= tol && abs(dtauD) <= tol;
	}
	return bConverge;
} // slASHRAETauModelInv
//--------------------------------------------------------------------------
bool slASHRAETauModelInv( // ASHRAE tau model inverse
	double sunZen,          // solar zenith angle, radians
	double Ebn,             // beam normal irradiance (typically solar noon)
	double Edh,             // diffuse horizontal irradiance (ditto)
	float &tauB,            // returned
	float &tauD)            // returned
{

	float extBm = slloccur->extbm;

	double m = AirMass(kPiOver2 - sunZen);

	double tauBd, tauDd;
	bool bRet = slASHRAETauModelInv(Ebn, Edh, extBm, m, tauBd, tauDd);
	tauB = float(tauBd);
	tauD = float(tauDd);
	return bRet;
} // slASHRAETauModelInv
//--------------------------------------------------------------------------
#if 0
// revised interface below
RC slPerezSkyModel(
	double tilt, 
	double azimuth, 
	short iHrST,
	double direct_normal_beam, 
	double horizontal_diffuse,
	double ground_reflectivity, 
	double& plane_incidence)
{
	RC rc = RCOK;

	// Some routines borrowed from elsewhere in code. Should be updated to share routines with XSURF and SBC.

	// Calculate horizontal incidence
	static float dchoriz[3] = { 0.f, 0.f, 1.f };	// dir cos of a horiz surface
	float verSun = 0.f;				// hour's vertical fract of beam (cosi to horiz); init 0 for when sun down.
	float solar_azm, cosz;
	int sunup = 					// nz if sun above horizon this hour
		slsurfhr(dchoriz, iHrST, &verSun, &solar_azm, &cosz);

	plane_incidence = 0.0;              // W/m^2
	double plane_beam { 0.0 };          // W/m^2
	double angle_of_incidence { kPiOver2 };  // radians
	// Don't bother if sun down or no solar from weather
	if (!sunup || (direct_normal_beam <= 0.f && horizontal_diffuse <= 0.f))
	{
		return rc;
	}

	// Set up properties of array
	double cosTilt = cos(tilt);
	float vfGrndDf = .5 - .5*cosTilt;  // view factor to ground
	float vfSkyDf = .5 + .5*cosTilt;  // view factor to sky

	// Calculate plane-of-array incidence
	float dcos[3]; // direction cosines
	slsdc( azimuth, tilt, dcos);
	float cosi; // cos(incidence)
	int sunupSrf = slsurfhr( dcos, iHrST, &cosi);
	if (sunupSrf < 0)
		return RCBAD;	// shading error

	if (sunupSrf)	
	{
		plane_beam = direct_normal_beam*cosi;
		angle_of_incidence = acos(cosi);
	}
	else
	{	
    // No change from default values if sun is up but not incident
	}

	// Components of diffuse solar (isotropic, circumsolar, horizon, ground reflected)
	float poaDiffI { vfSkyDf };
	float poaDiffC { 0.0f };
	float poaDiffH { 0.0f };
	float poaDiffG { 0.0f };

	// Modified Perez sky model
	float zRad = acos(cosz);
	float zDeg = DEG(zRad);
	float a = max(0.f, cosi);
	float b = max(cos(RAD(85.f)), cosz);
	const float kappa = 5.534e-6f;
	float e = ((horizontal_diffuse + direct_normal_beam) / (horizontal_diffuse + 0.0001f) + kappa*pow3(zDeg)) / (1 + kappa*pow3(zDeg));
	float am0 = 1 / (cosz + 0.15f*pow(93.9f - zDeg, -1.253f));  // Kasten 1966 air mass model
	float delt = horizontal_diffuse*am0 / IrSItoIP(1367.f);

	unsigned short bin = e < 1.065f ? 0 :
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
		poaDiffC = F1*a / b;
		poaDiffH = F2*sin(tilt);
	}

	poaDiffG = ground_reflectivity*vfGrndDf;

	float plane_diffuse = horizontal_diffuse*(poaDiffI + poaDiffC + poaDiffH + poaDiffG);  // sky diffuse and ground reflected diffuse
	float plane_ground_beam = direct_normal_beam*ground_reflectivity*vfGrndDf*verSun;  // ground reflected beam
	plane_incidence = plane_beam + plane_diffuse + plane_ground_beam; // W/m^2

	return rc;
}		// slPerezSkyModel
#endif
//-----------------------------------------------------------------------------
RC slPerezSkyModel(			// Perez transposition model (alt interface)
	float tilt,				// surface tilt, radians (0=horiz, pi/2=vert)
	float azimuth,			// surface azm, radians, (0=N, pi/2=E, pi=S)
	int iHrST,				// hour of day
	float direct_normal_beam,		// solar beam irradiance, any pwr/area
	float horizontal_diffuse,		// solar diffuse irradiance
	float ground_reflectivity,		// ground reflectivity (0 - 1)
	float& angle_of_incidence,		// beam angle of incidence, radians
									//   pi/2 if no beam
	float& plane_beam,				// returned: plane beam
	float& plane_diffuse_sky,		// returned: plane diffuse from sky
	float& plane_diffuse_ground)	// returned: plane diffuse from ground
{
	RC rc = RCOK;

	// Some routines borrowed from elsewhere in code. Should be updated to share routines with XSURF and SBC.

	// Calculate horizontal incidence
	static float dchoriz[3] = { 0.f, 0.f, 1.f };	// dir cos of a horiz surface
	float verSun = 0.f;				// hour's vertical fract of beam (cosi to horiz); init 0 for when sun down.
	float solar_azm, cosz;
	int sunup = 					// nz if sun above horizon this hour
		slsurfhr(dchoriz, iHrST, &verSun, &solar_azm, &cosz);

	plane_beam = plane_diffuse_sky = plane_diffuse_ground = 0.f;
	angle_of_incidence = kPiOver2;  // radians

	// Don't bother if sun down or no solar from weather
	if (!sunup || (direct_normal_beam <= 0.f && horizontal_diffuse <= 0.f))
	{
		return rc;
	}

	// Set up properties of array
	double cosTilt = cos(tilt);
	float vfGrndDf = .5 - .5*cosTilt;  // view factor to ground
	float vfSkyDf = .5 + .5*cosTilt;  // view factor to sky

	// Calculate plane-of-array incidence
	float dcos[3]; // direction cosines
	slsdc(azimuth, tilt, dcos);
	float cosi; // cos(incidence)
	int sunupSrf = slsurfhr(dcos, iHrST, &cosi);
	if (sunupSrf < 0)
		return RCBAD;	// shading error

	if (sunupSrf)
	{
		plane_beam = direct_normal_beam * cosi;
		angle_of_incidence = acos(cosi);
	}

	// Modified Perez sky model
	float zRad = acos(cosz);
	float zDeg = DEG(zRad);
	float a = max(0.f, cosi);
	float b = max(cos(RAD(85.f)), cosz);
	const float kappa = 5.534e-6f;
	float e = ((horizontal_diffuse + direct_normal_beam) / (horizontal_diffuse + 0.0001f) + kappa * pow3(zDeg)) / (1 + kappa * pow3(zDeg));
	float am0 = 1 / (cosz + 0.15f*pow(93.9f - zDeg, -1.253f));  // Kasten 1966 air mass model
	float delt = horizontal_diffuse * am0 / IrSItoIP(1367.f);

	int bin = e < 1.065f ? 0
		: e < 1.23f ? 1
		: e < 1.5f ? 2
		: e < 1.95f ? 3
		: e < 2.8f ? 4
		: e < 4.5f ? 5
		: e < 6.2f ? 6
		:            7;

	// Perez sky model factors
	static const float f11[8] = { -0.0083117f, 0.1299457f, 0.3296958f, 0.5682053f, 0.873028f, 1.1326077f, 1.0601591f, 0.677747f };
	static const float f12[8] = { 0.5877285f, 0.6825954f, 0.4868735f, 0.1874525f, -0.3920403f, -1.2367284f, -1.5999137f, -0.3272588f };
	static const float f13[8] = { -0.0620636f, -0.1513752f, -0.2210958f, -0.295129f, -0.3616149f, -0.4118494f, -0.3589221f, -0.2504286f };
	static const float f21[8] = { -0.0596012f, -0.0189325f, 0.055414f, 0.1088631f, 0.2255647f, 0.2877813f, 0.2642124f, 0.1561313f };
	static const float f22[8] = { 0.0721249f, 0.065965f, -0.0639588f, -0.1519229f, -0.4620442f, -0.8230357f, -1.127234f, -1.3765031f };
	static const float f23[8] = { -0.0220216f, -0.0288748f, -0.0260542f, -0.0139754f, 0.0012448f, 0.0558651f, 0.1310694f, 0.2506212f };

	float F1 = max(0.f, f11[bin] + delt * f12[bin] + zRad * f13[bin]);
	float F2 = f21[bin] + delt * f22[bin] + zRad * f23[bin];

	// Components of diffuse solar (isotropic, circumsolar, horizon, ground reflected)
	float poaDiffI{ vfSkyDf };
	float poaDiffC{ 0.0f };
	float poaDiffH{ 0.0f };
	
	if (zDeg >= 0.f && zDeg <= 87.5f) {
		poaDiffI = (1.f - F1)*vfSkyDf;
		poaDiffC = F1 * a / b;
		poaDiffH = F2 * sin(tilt);
	}

	float poaDiffG = ground_reflectivity * vfGrndDf;

	plane_diffuse_sky = horizontal_diffuse * (poaDiffI + poaDiffC + poaDiffH);  // sky diffuse and ground reflected diffuse
	plane_diffuse_ground = horizontal_diffuse * poaDiffG + direct_normal_beam * ground_reflectivity*vfGrndDf*verSun; 

	return rc;
}	// slPerezSkyModel

//==========================================================================

#ifdef WANTED /* no calls 6-12-88 rob */
x //======================================================================
x float FC slcosinc(sdircos, fhour)
x x /* Return instantaneous cosine of solar angle of incidence on a surface */
x x float sdircos[3]; /* Direction cosines for surface */
x float fhour;              /* Hour of day.  0. = midnight, 12.5 = 12:30 pm etc.
x                        This is taken to be solar time, local standard time
x                        or local daylight time depending on last call to
x                        slday() */
x x  /* Returns 0. if the sun is down relative to the surface or to the
x    world.  In other words, the return value can be safely multiplied
x    times the current beam intensity without fear of trouble. */
x
{
x float ha, dcos[3], cosinc;
x x ha = slha(fhour);
x if (fabs(ha) > slloccur->hasunset) x return 0.f;
x sldircos(slha(fhour), dcos);
x cosinc = DotProd(dcos, sdircos);
x return (cosinc > 0.f ? cosinc : 0.f);
x x
}    /* slcosinc */
#endif /* WANTED */

#if 0 /* before 10-23-88 recode to return azimuth and cos(zenith) */
x //======================================================================
x float FC slcoshr( sdircos, ihr)
x
x /* Calculate approx. cosine of incidence angle for an hour */
x
x float sdircos[3];      /* Direction cosines for surface */
x SI ihr;                /* Hour for which to calculate.
x                           0 = midnight - 1 AM etc.  Time depends on
x                           last setup call to slday(). */
x
x /* Returns approximate average cosine of angle of incidence of solar
x    beam for time that surface is exposed to the sun.  This value is
x    intended for direct use with solar beam value, thus it is adjusted
x    to account for time sun is behind the surface but *NOT* adjusted for
x    time sun is down, since solar beam data already includes that effect.
x
x    Note that slday() must be called prior to this for a given day.
x */
x{
x float c1, c2, t;
x SI ihx, pos1, pos2;
x
x /* No need to even attempt calculation if sun is not up at all
x    during the hour */
x     if (slloccur->sunupf[ihr] == 0.f)
x        return 0.f;
x
x     ihx  = ihr<23 ? ihr+1 : 0;
x     pos1 = (c1 = DotProd(sdircos,slloccur->dircos[ihr])) >= 0.f;
x     pos2 = (c2 = DotProd(sdircos,slloccur->dircos[ihx])) >= 0.f;
x
x     if (pos1 && pos2)  return ((float)(0.5*(c1+c2)));
x     if (pos1)          return ((float)(0.5*c1*c1/(c1-c2)));
x     if (pos2)          return ((float)(0.5*c2*c2/(c2-c1)));
x     return 0.f;
x}				/* slcoshr */
#endif


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
	 "SLRLOC:  Lat=%.2f  Long=%.2f  TZN=%.2f  elev=%.0f  options=0x%x  SN=%d  sinLat=%0.3f  cosLat=%0.3f\n",
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
		return mbIErr( "SLRDAY::Setup2", "NULL sd_pSL");

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
	DbPrintf( oMsk, "%sSLRDAY  mon=%d  doy=%d  slTm=%d  eqTm=%0.5f  haConst=%0.5f  slrNoon=%.2f slrTmShift=%d tauB/taud=%.3f/%.3f\n",
			hdg, sd_iM+1, sd_doy, sd_sltm, sd_eqTime, sd_haConst, TmForHa( 0.f), SolarTimeShift(), sd_tauB, sd_tauD);
	DbPrintf( oMsk, "hr        ha     fUp     dc0     dc1     dc2     azm  dirN  difH  totH\n");
	for (int iT=0; iT < 24; iT++)
		sd_H[ iT].DbDump( oMsk, WStrPrintf( "%2.2d", iT).c_str());
	sd_HSlrNoon.DbDump( oMsk, "SN");	// solar noon
}		// SLRDAY::DbDump
//------------------------------------------------------------------------------
void SLRHR::DbDump(
	int oMsk,				// mask
	const TCHAR* tag) const	// line tag (e.g. hour of day)
{
	DbPrintf( oMsk, "%s %9.5f %7.4f %7.4f %7.4f %7.4f %7.4f %5.1f %5.1f %5.1f\n",
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
	[[maybe_unused]] const TCHAR* hdg/*=""*/) const
{
	if (!DbShouldPrint( oMsk))
		return;
#if 0
	int nSSD = ss_oaSSD.GetSize();
	DbPrintf( "%sSLRSURF key=%d  azm=%0.1f  tilt=%0.1f  grRef=%0.2f  nSSD=%d\n",
			hdg, ss_key, ss_azm, ss_tilt, ss_grRef, nSSD);
	for (int i=0; i < nSSD; i++)
	{	DbPrintf( "  %2.2d", i);
		SLRSURFDAY* pSSD = ss_oaSSD.GetAt( i);
		if (!pSSD)
			DbPrintf( " (null)\n");
		else
			pSSD->DbDump();
	}
#endif
}		// SLRSURF::DbDump
//===========================================================================
#if 0
COA_SLRSURF::COA_SLRSURF( RSPROJ* pProj /*=NULL*/)
	: COA< SLRSURF>( "SurfDay", TRUE),		// no CM
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
			mbIErr( "COA_SLRSURF::InitCalcDaysIf"), "Inconsistent SLRLOC (%d)", iCD);
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
		return mbIErr( "SLRSURFDAY::InitDayIf", "NULL ssd_pSS");
	if (pSD)
	{	ssd_pSD = pSD;
		ssd_doy = -1;		// new SD, force calc
	}
	else if (!ssd_pSD)
		return mbIErr( "SLRSURFDAY::InitDayIf", "NULL ssd_pSD");

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
			// insurance: dot prod sometimes *very* slightly > 1)
			ssd_cosInc[ iT] = bracket( -1., DotProd3( SolarHr( iT).dirCos, DirCos()), 1.);
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
#if 0
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
		}
	}
	if (pHow)
		*pHow = how;
	return fSunlit;
}		// SLRSURFDAY::FSunlit
#endif
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
		mbIErr( "SLRSURFDAY::SolAirTemp", "NULL pHrFSunlit");
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
	DbPrintf( " %3.3d (%s)  %5.1f %5.1f   %5.3f\n",
				ssd_doy, Calendar.FmtDOY( ssd_doy).c_str(),
				ssd_shgfMax, ssd_shgfShdMax, ssd_shgfDifX);
	for (int iT=0; iT<24; iT++)
	{	DbPrintf( "   %2.2d  %5.1f  %5.1f  %5.1f    %5.1f    %5.1f\n",
			iT, AngIncDeg( iT), AngProfHDeg( iT), AngProfVDeg( iT), IRadDir( iT), IRadDif( iT));
	}
#else
	// traditional format
	DbPrintf( " %3.3d (%s)  %5.1f %5.1f   %5.3f\n",
				ssd_doy, Calendar.FmtDOY( ssd_doy).c_str(),
				ssd_shgfMax, ssd_shgfShdMax, ssd_shgfDifX);
#endif
}		// SLRSURFDAY::DbDump
//=========================================================================
#if 0
COA_SLRSURFDAY::COA_SLRSURFDAY( SLRSURF* pSS)
	: COA< SLRSURFDAY>( "SlrSurfDay", TRUE),		// no CM
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
		{	mbIErr( "ASHRAETauModel", "Dubious radDirN=%.f (extBm=%.f)",
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
	static const TCHAR* shdTag[ 2] = { "sunlit", "shaded" };
	SLRLOC sLoc;
	SLRDAY sDay( &sLoc);
	SLRSURF sSrf;
	SLRSURFDAY sSD( &sSrf, &sDay);
	float shgf[ 10];
	for (int iShd=0; iShd<2; iShd++)
	for (int iLat=0; iLat<17; iLat++)
	{	float lat = iLat*4.f;
		sLoc.sl_Init( lat, 0.f, 0.f, 0.f, 0);
		DbPrintf( "\nSHGF Lat=%0.0f (%s)\n---------------\n"
			"            NNE    NE   ENE     E   ESE    SE   SSE\n"
			"        N   NNW    NW   WNW     W   WSW    SW   SSW     S   HOR\n",
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
			VDbPrintf( dbdALWAYS, Calendar.GetMonAbbrev( iM), shgf, 10, "%6.0f");
		}
	}

}		// ::TestSHGF
#endif
//---------------------------------------------------------------------------
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
