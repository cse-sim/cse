// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

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
#include <cnglob.h>

#include <gmpak.h> /* gmcos2alt() */

#include <slpak.h> /* declarations for this file; SLLOCDAT */

/*-------------------------------- DEFINES --------------------------------*/
/* solar constant (Btuh/sf): solar extraterrestial beam radiation before
   variations during year. */
#ifdef SERI
const double SOLCON = 433.33; // SERI value: 1367 w/m2 ( 1 Btuh/sf = 3.1546 w/m2
#else
const double SOLCON = 434.; // previous value, source unknown
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
  t slday(doy, SLTMSOLAR);
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
    t slday(doy, SLTMSOLAR);
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
        t slday(doy, SLTMSOLAR);
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
        t slday(doy, SLTMSOLAR);
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
                      //   SLTMSOLAR:   Solar time
                      //   SLTMLST:     Local standard
                      //   SLTMLDT:     Local daylight time
  int options /*=0*/) // option bits
                      //   1: skip setup if SLLOCDAT.doy == doy

/* sets members re declination of earth's axis, time / hour angle conversion
   (.tmconst), hourly sunupf[], dircos[], slazm[], etc. */
{
  if (!slloccur)
    err(PABT, "NULL slloccur");

  if ((options & 1) && slloccur->doy == doy && slloccur->timetype == timetype)
    return; // already set for doy

#if 0 && defined(_DEBUG)
	printf("\nslday %d %d", doy, timetype);
#endif

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
                / (float)HA;                   // convert to hours
  *extbm =                                     // extrater beam intensity (Btuh/sf)
    (float)SOLCON                              // solar constant
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

  *extbm = (float)(SOLCON * (1. + 0.033 * c1));
  /* today's et beam rad is solar constant + variation */
#endif /* SERI */

} /* sldaydat */
//======================================================================
void FC sldec(/* Set declination-related data in current SLLOCDAT struct */

              float dec,    // declination of earth's axis, today, in radians
              int timetype) // time system for setting solar tables:
                            //  SLTMSOLAR, SLTMLST, or SLTMLDT (slpak.h).
{
  double hasr, hass; /* hour angles of sun rise, sun set (0 = solar noon) */
  double hsr, hss;   /* hours of sun rise, sun set (curr time system) */
  SI ihsr, ihss;     /* .. truncated to integers */
  double tmconst;    /* add to hour/HA to get hour angle */
  double sindec;
  double ha, fup, r;
  float alt;
  register SI ihr;

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
  if (timetype != SLTMSOLAR)          /* if not solar, convert local to solar */
    tmconst += HA                     /* radians per hour: convert to angle*/
                 * (slloccur->eqtime  /* equation of time */
                    + slloccur->tzn)  /* time zone (longitude of zn ctr).. */
               - slloccur->rlong;     /* .. less actual longitude */
  if (timetype == SLTMLDT)            /* if daylight local time */
    tmconst -= HA;                    /* 1 hour earlier */
  slloccur->tmconst = (float)tmconst; /* note tmconst also used below, also
                 slha calls below use .tmconst. */

  /* compute, for 24 hours of selected time type of day,
     Solar direction cosines, azimuth, and fraction of hour sun up. */

  hsr = (hasr - tmconst) / HA; /* convert hour angle to hour */
  ihsr = (SI)hsr;              /* truncate sunrise hour to integer */
  hss = (hass - tmconst) / HA; /* sunset ... */
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
  float c1, c2, fc1, fc2, faz1, faz2;

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

//======================================================================
float slha( // Calculate hour angle for hour of day

  float hour) // hour for which hour angle is reqd

/* hour can be fractional and is solar, local std, or local daylight
   time per last call to slday. */

/* must call slday() for day first (uses slloccur->tmconst) */

/* Returns hour angle (radians, 0 = solar noon) */
{
  return hour * HA            // HA: earth rotation per hr (slpak.h)
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
  // astCor = eqtime - (rlong/HA - tzn)
  // use precalculated tmconst (which also includes DST)
  return (slloccur->tmconst + kPi) / HA;
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
void FC slaniso(/* Adjust beam and diffuse radiation values
       according to Hay anisotropic sky model */

                float *pbeam, /* Pointer to beam value (Btu/sf): used/replaced. */
                float *pdiff, /* Pointer to diffuse value (Btu/sf): used/replaced.*/
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
  SI pos1, pos2;
  SI ihx;
  float c1, c2, cosi, f, fb, fd;

  if (*pbeam > 5.f) /* if a little beam */
  {
    if ((f = slloccur->sunupf[ihr]) > .05f) /* if sun up > 3 mins */
    {
      ihx = (ihr + 1) % 24;
      c1 = slloccur->dircos[ihr][2];
      c2 = slloccur->dircos[ihx][2];
      pos1 = (c1 >= 0.f);
      pos2 = (c2 >= 0.f);
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

#include "ASHRAE/solar.cpp"

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
}	// slPerezSkyModel2

//==========================================================================

  /************************ rest of file is if-outs *************************/

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

  // end of slpak.cpp
