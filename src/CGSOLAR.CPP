// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cgsolar.cpp -- hour's solar gains tabulator for CSE hourly simulator

// some if's cleaned out 2-14,16-95... see cgsolar.cp5, .cp7.

/* notes for future, 2-95:

   1. Present implementation believed good without SGDISTs;
      may be slow with SGDISTs & many windows & surfaces;
      does not support out-of-zone SGDISTs with correct shade control.
      But NREL not using SGDISTs as of 2-95.

   2. For more speed for SGDISTs in large buildings, chain surface-sides by zone at setup (cncult3).
      4 chains: XSRAT CTMEXTWALL and CTMINTWALL surface insides in zone,
                XSRAT CTMEXTWALL and CTMINTWALL surface outsides in zone,
                MSRAT .insides in zone,
                MSRAT .outsides in zone.
      4 chain heads in ZNR, 2 chains in XSRAT, 1 in MASSBC (with MSRAT subscripts).
      Use in SgThruWin::doit(), and re enhanced toZoneCav() (next).

       3. Permitting SGDISTs to zones other than window's zone:
          toZoneCav() must work with control zone != target zone: to do so, it must loop over
          all surfaces in the zone and distribute to zone air per cavAbs (with split for each surface).
          Probably do new code such that can also be used for normal distribution
          and delete the global split-adjustment and cavity-distribution stuff at end of makHrSgt().

       4. Are the mass & znCAir accumulator member variables worth the complexity they add,
          especially with #3? (#3 could eliminate cavity accumulator). */


//------------------------------- INCLUDES ----------------------------------

#include <cnglob.h>

#include <ancrec.h>		// record: base class for rccn.h classes
#include <rccn.h>
#include <msghans.h>	// MH_R0160

#include "solar.h"

#include "ashwface.h"

#include <exman.h>		// rer

#include <cnguts.h>	// declarations for this file, shad, WshadR, Top, ...

// DEBUG AIDS --------------------------------------------------------------
#if 0 && defined( _DEBUG)
static int TrapSurf( const char* name, int si=-1)
{
static const char* trapName = "Ceiling";
static int trapSi = 2;	// 0=inside, 1=outside, -1=either, 2=never
	return _stricmp( name, trapName)==0
		&& trapSi < 2 && (si == trapSi || trapSi == -1 || si == -1);
}
#endif

//-------------------------------- DEFINES ----------------------------------
//(none now)

//--------------------------- PUBLIC VARIABLES ------------------------------
#ifdef SLRDEBUGPRINT
*  FILE * Fslrdbg = NULL;	// public so others can write to file too.
#endif



int slrCalcJDays[13] =	// julian day of year for which to do solar table calculations for each month
{
	// used: locally, cnguts.cpp and/or cnausz.cpp.
	356,							// Dec 21 for generic month 0: may be used for heat design day.
	17, 46, 75, 105, 135, 162, 198, 228, 259, 289, 319, 345 	// months 1-12: main sim, autosize cooling design days
	//16  14  15   14   14   10   16   15   15   15   14   10	(days of months)
};

#if (SLRCALCS & 1)
static SLLOCDAT* Locsolar = NULL;	// Ptr to solar info structure for current calculation.
									// Released by slfree from tp_LocDone().
									// Address saved by slinit for other slpak calls.
									// Daily info set by slpak:slday from cgwthr.cpp and cgsolar.cpp at least, 1-95. */
#endif
#if (SLRCALCS & 2)
SLRLOC SlrLoc;
SLRDAY SlrDay;
#endif

//------------------------------------------------------------------------
RC TOPRAT::tp_LocInit()		// location-initializer for CSE
// inits slpak.cpp using location info in Top members (gotten weather file and/or user input in cncultx.cpp)
// returns RCOK if ok, else message already issued.
{
	// init slpak.cpp: alloc SLLOCDAT and init for location.
	//   More of Locsolar is set each day by slday(). It is used by most other slpak calls.
	//   slpak remembers its location internally.

#if (SLRCALCS & 1)
	Locsolar = slinit(RAD(latitude),	// latitude (to radians)
					   RAD(longitude), // longitude
					   timeZone, 		// time zone
					   elevation);		// site altitude (ft)
#endif
#if (SLRCALCS & 2)
	SlrLoc.sl_Init(latitude, longitude, timeZone, elevation, 0);
#endif
	return RCOK;
}		// TOPRAT::tp_LocInit
//------------------------------------------------------------------------
RC TOPRAT::tp_LocDone()		// Free location related dm stuff: Locsolar
{
#if (SLRCALCS & 1)
	slfree(&Locsolar);  		// free solar data struct, null ptr, slpak.cpp. Redundant call ok.
#endif
	return RCOK;
}			// TOPRAT::tp_LocDone
//===========================================================================================================


/*-------------------------- SOLAR GAIN OVERVIEW ----------------------------

Sun falls on XSURFs (opaque surfaces and windows),
and heats TARGETs (zone air, mass inside and outside surfaces):

Targetting is handled approx as follows: Sun on windows: heats (insides of)
masses per user input and (1995) automatic distribution; rest heats zone air
("CAIR", .znCAir); Sun on os: heavy: heats outside of the mass; light: some
heats zone air.

NEWSGT: In 1995 the window solar gain targetting was changed to a cavity
absorptance model in which gain is distributed to surfaces in proportion to
their area and absorptivity, allowing for portions of reflected sun that
go back out windows and that are later absorbed after reflection. All surfaces
are assumed to have uniform views of each other, as though the zone were a
single room and the room interior were spherical. Since the distribution
depends on the surface absorptivities and the glazing Solar Heat Gain
Coefficients, both monthly-hourly variable, the apportionment amoung surfaces
is computed here. Insolation automatically apportioned to massive surfaces
is directed thereto (a change, 1995); only insolation automatically apportioned
to light surfaces now goes to the zone "air".

The solar gain of each target depends on the following factors (at least):
    Fixed for of run:
        latitude of location;
        area, orientation, and shading of each surface or opening
           (more than one may heat the same target).
    Approximated monthly:
        declination of earth's axis (sun's position in sky for date)
    Change hourly but predictably (same for at least a month):
        sun's position in sky;
        absorptance of each surface (monthly-hourly schedulable).
        distribution of sun on masses per user SGDIST input
    Change hourly:
        whether operable window shades are closed (which changes window
           SHGC and SGDIST solar gain distibution);
	hourly weather from file: beam and diffuse radiation for hour.

The diffuse solar gain factor, and each hour's beam solar gain factor,
to each target and for each control variable (zone window shade positions),
are tabulated in SGRAT SgR by code here.  The hourly simulation cycle
(cnloads.cpp called by cnguts.cpp, 1995) then multiplies these values by
weather data radiation values and adds the products to the respective targets.

Shades recap: when a window has movable shades, XSURF.scc (from wn/gtSMSC)
differs from XSURF.sco (from wn/gtSMSO). Code here then tabulates the factors
for shades open, and the shades closed difference. The cnloads.cpp then uses
a combination of the factors according to the hour's shade position, via
the "control" pointer in the SGRAT struct (next topic). */

/*------------------------ SOLAR GAIN TABLES STORY -----------------------

The SGRAT record structure (app\cnrecs.def) plus the weather data allow
computing the gain to one target under control of one control variable,
and is valid for one month.  Approx:

  struct SGRAT
  {  ...
     SI addIt;		* flag nz to add gain, 0 to store (target's 1st entry)
     SGTARG *target;	* (float *) where to store/add gain
     float *control;	* NULL or 0..1 to multiply gain, eg zn shades position
     float beam[24];	* multiply by hourly beam radiation for gain
     float diff[24];	* multiply by hourly diffuse radiation ditto
  };

"control"s in the above: normally NULL, gain not controlled.
A second SGRAT record is used when:

   a window's gain can be reduced by closing its shades (XSURF.scc < .sco), or

   solar gain distribution changes when shades closed (SGDIST.sd_FSC < .sd_FSO)

The second SGRAT record's beam and diff contain the DIFFERENCE in gain,
usually negative, when the shades are closed. and its .control points
to its zone's ZNR.i.znSC (shade closure, former shutter fraction).
Each zone's .znSC may be input by user (subhourly expr), or defaults to 1.0
when heating, 0.0 when cooling (set in cnloads:loadsAfterSubhr).

Solar gain data is valid for a month: 24 hourly beam and diff values are
stored; inputs vary little (sun position) or not allowed to vary (SHGC's,
sfExAbs, etc) during month. Solar gain data is computed on the
first day of each month, then used for the entire month (or until an input
changes -- believe no such at present 2-95).  cnguts.cpp calls makHrSgt at
each hour of the simulation cycle on days when solar data must be computed. */

/*---------------------------------------------------------------------------
Proof that makHrSgt can compute "split" as uval/uX, 2-95

  given: uI = outside film conductance, uC = wall construction cond, uI = inside film cond.

  to compute "split", the fraction of sun falling on outside of wall that gets to inside air.

                  conductance to inside                  series( uC, uI)
  split =  ----------------------------------  =  ------------------------------
           parallel( cond to in, cond to out)     parallel( uX, series( uC, uI))

                                                     1 / (1/uC + 1/uI)
                                               =  ----------------------
                                                  uX + 1 / (1/uC + 1/uI)

  numerator and denominator times uC*uI/uC*uI:       uC*uI / (uC + uI)
                                               =  ----------------------
                                                  uX + uC*uI / (uC + uI)

  numerator and denominator times (uC + uI):              uC*uI
                                               =  ---------------------
                                                  uX*uC + uX*uI + uC*uI

  divide numerator & denominator by uX*uC*uI:            1/uX
                                               =  -------------------
                                                  1/uI + 1/uC + 1/uX

  write as                                        1/(1/uI + 1/uC + 1/uX)
                                               =  ----------------------
                                                          uX
  numerator is series(uI,uC,uX) which is uval:    uval
                                               =  ----
                                                   uX               QED. */

//----------------------- LOCAL FUNCTION DECLARATIONS -----------------------
LOCAL RC sgrAdd( int targTy, TI targTi, float* pCtrl, double bmBm, double dfDf, double bmDf=0.);
LOCAL void sgrPut( const char* sgName, SGTARG* pTarg, float* pCtrl, BOO isSubhrly,
#ifdef SOLAVNEND // undef in cndefns.h (via cnglob.h) 1-18-94
	BOO isEndIvl,
#endif
	double bmBm, double dfDf, double bmDf=0.);
LOCAL void FC cgRefGls( float eta, float* trans, float* abso);
LOCAL void toSurfSide( TI xsi, SI si, TI czi, const double sgf[ socCOUNT][ sgcCOUNT]);
LOCAL void toZoneCAir( TI zi, TI czi, float bmo, float dfo, float bmc, float dfc);

//---------------------------------------------------------------------------

/*=========================================================================*/
/* 					AND see "solar gain overview" and "solar tables story" at start of file.

----- makHrSgt (next) in CSE notes -----

calling context:
    CSE input includes 1 or more ZONES (ZNRs) each w a chain of exterior
       wall & window components (XSURFs in XSRAT).
    Solar gain targetting is specified in XSURF's array sgdist[]
       of fractional solar distributions to zones and masses.

makHrSgt() is called for each hour of the day, during the simulation cycle,
    on days when the solar tables need to be calculated: whenever an input
    has changed: new month (no others 2-95?).

one makHrSgt() call does:
    Integrates XSURF size/orientation/shading/absorptivity/schedulable SHGC
    etc information and SGDIST explicit solar distribution information with
    sun's position for date and hour, plus ground reflectivity, etc,
    to produce solar gain info for ALL TARGETS for shades OPEN and the
    difference for shades CLOSED for one hour of one month.
    Multiple gains to the same target are summed into one table.
    Each gain table includes beam values for 24 hours and is used (combined
    with hourly weather data every hour) for a month or until another input
    changes;  24 calls are thus needed to fill the table.

A Solar Gain Table entry (SGRAT record) contains:
	a pointer to a TARGET (float * place to add gain to)
	a pointer to a control variable to multiply gain, or NULL
	1 single diffuse and 24 hourly beam RADIATION MULTIPLIERS.

The possible targets (2-95) for gain are:
		ZNR.qSgAir	zone air
		ZNR.qSgTot	zone total (redundant check/report value)
		ZNR.qSgTotSh	ditto, subhourly portion
		MASSBC[].sg	mass inside [0], mass outside [1].

Control variables: each target has an SGRAT record with NULL .control.
In addition, if there is a controlled portion of its gain, it contains
ANOTHER SGRAT entry with the appropropriate control pointer and radiation
multipliers for the DIFFERENCE in gain when the control variable is 1.0.

At present (1992-95), controls are only used for operable window shades:
if a window's xsurf's .scc < .sco or if SGDIST.sd_FSC != .sd_FSO,
an additional SGRAT entry is made, containing the change in gain (usually
NEGATIVE radiation multipliers) and a pointer to its zone's ZNR.i.znSC.
User-inputtable ZNR.i.znSC (shade closure, formerly shutter fraction) defaults
to 1. when heating and 0. when cooling (cnloads.cpp:loadsAfterSubhr). */


//======================================================================
// window gain-targeting local helper class
//----------------------------------------------------------------------
class SgThruWin
{
//given by caller
	XSRAT* tw_xr;			// window whose gain is to be distributed
#if 1
								// units of tw_tXX are ft2, e.g. effective aperature
								//   transmitted gain = ext solar * tw_tBm = Btuh/ft2 * ft2 = Btuh
								// Note this representation cannot separately account for beam.
	double tw_tDf[ socCOUNT];		// full-area transmitted diffuse gain per unit exterior diffuse
									//   shades open/closed
	double tw_tBm[ socCOUNT];		// full-area transmitted beam+diffuse gain per unit exterior beam
									//   shades open/closed
#else
x	double gBm, gDf;	// direct (beam) and diffuse gain per unit area, already determined by caller.
x						// This gain is apportioned amoung targets here and used in creating SGRAT SgR records
x						// that are used to apportion gain per weather data amoung targets during simulation.
x	double sgf[ 2];		// window solar gain factors (area * fMult * SHGC)
x						// to multiply by gBm/gDf and specific target weight.
x						// After SGDISTs done, may represent non-SGDIST'd gain only.
#endif

public:
	SgThruWin( XSRAT* xr, double tDf1[ socCOUNT], double tBm1[ socCOUNT]);
	void tw_Doit();
private:
	void tw_HitsSurfSide( TI sfi, int si, TI czi, float fo, float fc);
	void tw_ToZoneCAir( TI zi, TI czi, float fo, float fc);
	void tw_ToZoneCav( TI zi, TI czi, float fo, float fc);
	BOOL tw_HasSGDIST( int targTy, TI targTi, int sgiEnd=-1);
};		// class SgThruWin
//=============================================================================

//======================================================================
void FC makHrSgt(				// make solar tables for an hour for current month
	float* pVerSun /*=NULL*/)	// Returns: cosine of solar incidence angle to horizontal surface for hour --
								//   caller uses to compute beam radiation on horizontal surface

/* Uses: Top.iHr:    hour, daylight time, 0-23, to identify 1st call of day (0)
	     Top.iHrST:  hour, standard time, 0-23, re storing results (1st call of day may be 23).
	     Top.tp_date.month: month, 1-12 (use daylight month: either same as ST or correct for all but 1st (dark) hour of month).
	     Top.jDayST: Julian date 1-365.
	     All XSRAT XsB entries (surfaces and windows) (these include info from explicit SGDIST inputs).
	     Mass-modelled surface information (MSRAT MsR) and Zone information (ZNR ZrB).
	     For autoSizing, Top.tp_date.month and jDayST must be appropriately set for the design day.

	Sets: SGRAT SgR entries: creates needed entries (clears SgR 1st if iHr==0)
	                         and accumulates into .beam[Top.iHrST] and beam[] members.

	For internal use, NEWSGT 1995 version determines cavity absorptance
	of each massive surface-side and of zone air, and uses members of zone
	record (ZNR ZrB) to accumulate information before entering into SgR.

	Called each hour on first day of month and on days when any input with
	(if allowed in future) daily or hourly (as opposed to monthly-hourly) variability. */

/* Combines:
 size, orientation, and shading of XSURFs (walls & windows),
 user input (window to masses), implicit solar dist of XSURFs,
 multiple exposures per target (zone air, zone total, mass in & out),
 and the hourly sun position on standard day of given month.
   To produce:
 This hour's beam and diffuse radiation multipliers
    for each target,
    for each control variable e.g. window shades position.

   The solar tables are combined with weather data and ctrl variables
   (zone .i.znSC for shades closure) to compute solar gains in the hourly
   simulation cycle (cnguts.cpp:tp_SimHour(), calls cnloads.cpp).
   More notes just above and at start file. */
{

//----- makHrSgt: Initialization -----

#ifdef SLRDEBUGPRINT
*// Set up for debug print if required.	File is opened but never closed.  Subsequent calculations will append to file.
*    if (Fslrdbg == NULL)
*       Fslrdbg = fopen( "SGAIN.VMT", "wt");
*    fprintf( Fslrdbg, "\n\n\n=============== Month = %d  Day = %d\n",
*             INT(Top.tp_date.month-1), INT(slrCalcJDays[Top.tp_date.month]) );
#endif

// Initialize slpak for a standard day near middle of month.  slselect(Locsolar) in effect. Note restored at exit.

	slday( slrCalcJDays[ Top.tp_date.month],	// julian date for this month's solar calcs
					sltmLST );					// say use local standard time
	/* slpak.cpp: set up curr SLLOCDAT struct for given day:
	   sets info re declination of earth's axis, hourly sunupf[], dircos[], slazm[], etc.
	   Used by most other slpak calls. */

// init...  verSun: hour's sun fraction on horiz surf (for ground)

	static float dchoriz[3] = { 0.f, 0.f, 1.f};	// dir cos of a horiz surface
	float verSun=0.f;				// hour's vertical fract of beam (cosi to horiz); init 0 for when sun down.
	int sunup = 					// nz if sun above horizon this hour
	  slsurfhr( dchoriz, Top.iHrST, &verSun, NULL, NULL);
									//	for (horiz) surf dircos & std time hour get cosine
    								//  avg incidence angle on (horiz) surf, slpak.cpp.
									//	rets TRUE if sun up over horizon (& surface)
	if (pVerSun)				// if nonNULL pointer given (arg added 9-94)
		*pVerSun = verSun;		// return versun to caller's hourly array for use thruout month

// initialize SGRAT, where solar gains to targets are accumulated
	if (Top.iHr==0) 			// 1st call of day: clear table and start over
		SgR.al( SgR.n, ABT);	// clear existing records; keep any alloc'd size to minimize fragmentation. ancrec.cpp.

// makHrSgt: Determine totals needed to determine cavity absorptances of surfaces
//   These must be determined each call because they depend on monthly-hourly surface absorptance
//   inputs, as well as on monthly-hourly window properties (re re-radiation out of windows)

	// clear accumulator variables in all zones
	ZNR* zp;  			// zone record pointer
	RLUP( ZrB, zp)			// loop over ZNR zone records in basAnc ZrB, setting zp to point at each
		zp->zn_SGClearTots();

// total area-weighted surface absorptivity in zone (modelled as one "room") must be
//  accumulated from all surfaces sides

// accumulate absortivity of light surface-sides & zone "air", and transmission (back out) of windows
	XSRAT* xr;
	RLUP( XsB, xr)			// all surfaces, all zones
		xr->xr_SGAccumAbsTrans();

	RLUP( XsB, xr)
		xr->xr_SGIncTrans( sunup, verSun);		// calc gains to / thru each surface for hour

	// compute cavity absorptance for zones
	RLUP( ZrB, zp)
		zp->zn_SGCavAbs();

	// loop surfaces
	//   final gain adjustments (re cavity abs, )
	//   generate SGRAT entries for runtime use
	RLUP( XsB, xr)		// all surfaces (all zones)
		xr->xr_SGMakeSGRATs();

	// distribute zone quick-surface gain to zone CAir, and make SgR entries for zone.
	//  Done once from zone accumulators for speed.
	RLUP( ZrB, zp)
	{
		if (!zp->zn_IsConvRad())
			zp->zn_SGToZone();
#if 0 && defined( DEBUGDUMP)
0		// not very useful; makHrSgt is called hourly producing verbose DbDump
0		if (DbDo( dbdRADX))
0			zp->zn_DbDumpSGDIST( "month?");
#endif
	}

//----- makHrSgt: Termination -----

#if defined( DEBUGDUMP)
	if (Top.iHr==23 && DbDo( dbdSGDIST))
	{	DbPrintf( "SGDIST for day %d\n", slrCalcJDays[ Top.tp_date.month]);
		SGRAT* sg;
		RLUP( SgR, sg)
			sg->sg_DbDump();
	}
#endif

	// Re-initialize slpak for current day as assumed by cgwfread() called from cnguts.cpp each hour b4 this fcn.
	// Don't know a) if other parts of program depend on it (else do in wswfread each call)
	//            b) if would be faster to push & pop the data it sets.
	slday(Top.jDayST,		// julian date of simulation (standard time), cnguts.cpp.  Use of ST matches call in cgwthr.cpp.
			sltmLST,		// say use local standard time
			1);				// skip if no day change
		// slpak.cpp: set up curr SLLOCDAT struct (which is cse.cpp:Locsolar) for given day:
		// sets info re declination of earth's axis, hourly sunupf[], dircos[], slazm[], etc.
		// Used by most other slpak calls.

}	// makHrSgt
//-----------------------------------------------------------------------------
void ZNR::zn_SGClearTots()		// clear SG totals
{
	rmAbs = 0.;					// for area-weighted absorptance
	rmAbsCAir = 0.;				// for zone CAir portion of absorptance
	for (int oc = 0; oc < 2; oc++)			// loop shades open [0], closed [1]
	{	rmTrans[oc]= 0.;  					// for reflected solar going out window
		sgfCavBm[oc]= sgfCavDf[oc]= 0.;		// for zn's hr's untarg'd beam & diff gain, to distrib amoung surfs & CAir
		sgfCAirBm[oc]= sgfCAirDf[oc]= 0.;	// for hr's gain distributed to zn's CAir
		sgSaBm[oc] = sgSaDf[oc] = 0.;		// adjustment accumulators
	}
}		// ZNR::zn_SGClearTots
//-----------------------------------------------------------------------------
void XSRAT::xr_SGAccumAbsTrans()	// accumulate zone surface absorptance and transmittance totals
// also some by-surface initialization
{
#if defined( _DEBUG)
	x.xs_Validate( 0x100);
#endif

	// initialize solar gain accumulators (inside and outside)
	xr_SGFInit();

	// zone-adjacent surfaces: calc area-weighted absorp and accum to zone
	//   does nothing for other exposures (e.g. ambient)
	x.xs_sbcI.sb_CalcAwAbsSlr();	// inside (always zone)
	x.xs_sbcO.sb_CalcAwAbsSlr();	// outside (may be zone)

	if (x.xs_ty == CTWINDOW)		// (exterior) window
	{
		// fraction of light reflected in zone that is lost via this window
		//   = not reflected = trans + abs
		double tauAbsB[ 2];		// back (room-side) diffuse loss
		if (x.xs_IsASHWAT())
		{	// TODO: fa_mSolar ??
			tauAbsB[ 0] = 1. - x.xs_pFENAW[ 0]->fa_DfRhoB();
			tauAbsB[ 1] = x.xs_pFENAW[ 1]
							? 1. - x.xs_pFENAW[ 1]->fa_DfRhoB()
							: tauAbsB[ 0];
		}
		else
		{	GT* gt = GtB.GetAt( x.gti);		// point glazeType record
			tauAbsB[ 0] = x.xs_SHGC * gt->gtDMRBSol * x.sco;	// sco and scc have m-h variability
			tauAbsB[ 1] = x.xs_SHGC * gt->gtDMRBSol * x.scc;
		}
		// sum area weighted tau for room
		ZNR* zp = ZrB.GetAt( ownTi);	// zone containing "inside" of this window
		for (int oc=0; oc<socCOUNT; oc++)
			zp->rmTrans[ oc] += tauAbsB[ oc] * x.xs_fMult * x.xs_area / zp->zn_surfA;
	}
}		// XSRAT::xr_SGAccumAbsTrans
//----------------------------------------------------------------------
void XSRAT::xr_SGIncTrans(			// hour exterior incident and transmitted solar gains for surface
	int sunup,				// nz iff sun is above horizon
	float verSun)			// cos( inc angle to horiz)
{

	if (!x.xs_CanHaveExtSlr())
		return;			// this surface cannot receive exterior solar gain
						//   (CTINTWALL, CTPERIM, ...)

	// view factors. For beam radiation, (window) shading is computed in CSE (SunlitFract call, below).
	//   For diffuse, no shading computed in CSE; user may, for windows, input view factors so can precompute shading
	// sky diffuse view factor is x.vfSkyDf, inputtable as wnVfSkyDf, m-h vbl, default  .5f + .5*cos(x.tilt)
	// ground diffuse view factor (incl for ground-reflected beam), times reflectivity, is grx:
	double grx = x.grndRefl     // grnd refl, m-h vbl, dfl .2: wnGrndRefl,sfGrndRefl,Top.grndRefl,etc.
			   * x.vfGrndDf;	// ground view factor, wnVfGrndDf for windows (m-h vbl), default  .5f - .5f*cos(tilt)
	double sgf[ sgcCOUNT];		// surface irradiance factors
	sgf[ sgcDFXDF] = x.vfSkyDf + grx;	// surface diffuse-diffuse irradiance factor, dimless
										//   = unit area surface irradiance per unit diffuse horizontal irradiance
										//     Note: does not include surface area or absorptance
	sgf[ sgcDFXBM] = grx*verSun;		// surface beam-diffuse irradiance factor, dimless
										//   = unit area surface diffuse irrad per unit normal beam irrad

	float cosi = 0;		// cosine( incidence angle)
	float incA;		// angle of incidence, radians (Pi/2 if !sunup)
	float cosz = 0;		// cos( zenith angle), 0 if !sunup
	float azm = 0;		// solar azimuth, radians (0=south)
	float relAzm;	// solar azimuth relative to surface (Pi if !sunupSrf)
	int sunupSrf	// 1 iff sun up relative to surface
		= sunup			// if sun over horizon
		  && slsurfhr( x.xs_dircos, Top.iHrST, &cosi, &azm, &cosz);
						// for surf dircos & hr, get cos avg inc angle
						//   (0 behind), solar azm, cos slr zenith angle
	double fsunlit;		// fraction sunlit
	if (!sunupSrf)
	{	cosi = 0.f;
		incA = kPiOver2;
		cosz = 0.f;
		azm = 0.f;
		relAzm = kPi;
		fsunlit = 0.;
		sgf[ sgcBMXBM] = 0.;
	}
	else
	{	// incident beam possible
		incA = acos( cosi);
		relAzm = azm - x.azm;	// sun azm wrt wall
		WSHADRAT* pS = WshadR.GetAtSafe(x.iwshad);
		if (!pS)
			fsunlit = 1.f;
		else
			fsunlit = pS->SunlitFract(relAzm, cosz);
		sgf[ sgcBMXBM] = fsunlit*cosi;	// surface beam irradiance factor, dimless
										//   = unit area surface beam irradiance per unit beam normal irradiance
	}

	xr_SGFAccum( sgf);		// accum solar irradiance to exterior
							//   note: Btuh/ft2, abs and area applied at point of use

	if (x.xs_ty == CTWINDOW)
	{
		// windows can get two SGRAT entries:
		//  [0], no control variable, gets shades-open gain.
		//  [1], controlled by ZNR.i.znSC, gets reduction (negative) when shades closed,
		// omitted if no reduction.  ONLY USE of > 1 SGRAT entry per XSURF.


		// unit area transmitted gain per unit exterior solar, dimless, shades open/closed
		// Note: area not included here (see tw_DoIt())
		double tDf1[ 2];		// transmitted diff gain per unit ext diffuse
		double tBm1[ 2];		// transmitted beam+diff gain per unit ext beam
		tDf1[ 0] = tDf1[ 1] = tBm1[ 0] = tBm1[ 1] = 0.;
		if (x.xs_IsASHWAT())
		{	float vProfA = slVProfAng( relAzm, cosz);
			float hProfA = relAzm;
			for (int oc=0; oc<socCOUNT; oc++)
			{	if (!x.xs_pFENAW[ oc])
				{	if (oc == 0 || x.xs_HasControlledShade())
						errCrit( WRN, "Surface '%s': FUBAR ASHWAT!", name);
					else
					{	tDf1[ oc] = tDf1[ 0];		// shade closed = shade open
						tBm1[ oc] = tBm1[ 0];
					}
				}
				else
				{	tDf1[ oc] =   sgf[ sgcDFXDF] * x.xs_pFENAW[ oc]->fa_DfTauF();
					x.xs_pFENAW[ oc]->fa_BmSolar( Top.iHrST, incA, vProfA, hProfA);
					tBm1[ oc] =   sgf[ sgcDFXBM] * x.xs_pFENAW[ oc]->fa_DfTauF()
						        + sgf[ sgcBMXBM] * x.xs_pFENAW[ oc]->fa_BmTauF( Top.iHrST);
					// solar multiplier adjustment (reconciles gain to A*SHGC)
					tDf1[ oc] *= x.xs_pFENAW[ oc]->fa_mSolar;
					tBm1[ oc] *= x.xs_pFENAW[ oc]->fa_mSolar;
				}
			}
		}
		else
		{
			TI gti = x.gti;				// window's glazeType subscript
			GT* gt = GtB.GetAt( gti);	// point to glazeType

			double SHGCdf = x.xs_SHGC * gt->gtDMSHGC;
			double gDf = sgf[ sgcDFXDF] * SHGCdf;			// inc * diffuse SHGC

			// gBm: beam gain per area (applied below: shades, constant factor of new-model SHGC's)
			double gBm = 0.;
			if (sunup)
				gBm =  sgf[ sgcDFXBM] * SHGCdf		// diffuse transmitted per unit beam (e.g. ground reflection)
													//  (NB can be nz even if sun behind surface)
					 + sgf[ sgcBMXBM] * x.xs_SHGC * gt->gtPySHGC.val1( cosi);

			// window...: check that scc <= sco: runtime variable expressions
			if (x.scc > x.sco + ABOUT0)
				// note wnSMSO/C values may be from gtSMSO/C.
				rer( (char *)MH_R0163, name, x.scc, x.sco);
						// "Window '%s': wnSMSC (%g) > wnSMSO (%g):\n"
						// "    SHGC Multiplier for Shades Closed must be <= same for Shades Open"
			tDf1[ 0] = gDf * x.sco;
			tBm1[ 0] = gBm * x.sco;
			tDf1[ 1] = gDf * x.scc;
			tBm1[ 1] = gBm * x.scc;
		}

		// window...: distribute gain
		SgThruWin sgtw( this, tDf1, tBm1);
		sgtw.tw_Doit();
	}
}		// XSRAT::xr_SGIncTrans
//-----------------------------------------------------------------------------
void XSRAT::xr_SGFInit()		// initialize gain accumulators
{
	x.xs_sbcI.sb_SGFInit();
	x.xs_sbcO.sb_SGFInit();

}		// XSRAT::xr_SGFInit
//-----------------------------------------------------------------------------
void XSRAT::xr_SGFAccum(		// outside surface irrad accumulation
	const double sgf[ sgcCOUNT])	// unit area irrad per unit solar, dimless
{
	// no explicit targeting and no shade closure for opaque surfaces.

	// shades open / closed: values same
	VAccum( x.xs_sbcO.sb_sgf[ socOPEN], sgcCOUNT, sgf);
	VAccum( x.xs_sbcO.sb_sgf[ socCLSD], sgcCOUNT, sgf);

	ZNR* zp = ZrB.GetAt( ownTi);
	double sgx = x.xs_area * x.xs_AbsSlr( 1);	// area * outside slr absrp, VMHLY
	if (sgx > 0. && x.xs_ty == CTEXTWALL && !zp->zn_IsConvRad())
	{	// CTEXTWALL (quick), non-IsConvRad: solar gain to quick-model opaque surface goes to zone air.
		// total absorb for full surf area
		double gDf = sgx*sgf[ sgcDFXDF];					// gain per diffuse
		double gBm = sgx*(sgf[ sgcDFXBM] + sgf[ sgcBMXBM]);	// gain per beam

		// portion of insolation on outside that gets to inside air
		//     = conductance to inside air / parallel conductance from outside surface to air in & out
		//     = series conductance thru wall / outside surf conductance (see algebra at start of file)
		// note x.uval set so that this is also valid for CTMXWALLs,
		// but code below already does for accumulated value, 2-95
		double split = min( x.xs_uval / x.uX, 1.);	// cond thru wall: uX series uC series uI.
													// min = insurance: could happen if a differnt .uX used in .uval.
		for (int oc = 0; oc < socCOUNT; oc++)		// loop shades open [0], closed [1] (no difference for os)
		{	zp->sgfCAirBm[oc] += split * gBm;  		// accumulate zone beam sg factor
			zp->sgfCAirDf[oc] += split * gDf;  		// diffuse...  sgrAdd'd to znCAir below.
		}
	}
}		// XSRAT::xr_SGFAccum
//-----------------------------------------------------------------------------
void XSRAT::xr_SGMakeSGRATs()		// final gain adjustments / generate SGRAT entries
{
	int oc, si;
	for (si = 0;  si < 2;  si++)			// loop over sides 0 (inside) and 1 (outside)
	{	SBC& sbc = x.xs_SBC( si);
		int mx = 1;
		if (sbc.sb_zi)
		{	ZNR* zp = ZrB.GetAt( sbc.sb_zi);		// point to zone info for zone to which side is exposed
			if (!zp->zn_IsConvRad() && !x.xs_IsDelayed())
				mx = -1;							// if CSE zone model, do NOT add matching zone balance entry
													//   for quick surfaces
			for (oc = 0;  oc < 2;  oc++)     		// control: 0 is shades open, 1 is shades closed diff if any
			{	// distribute portion of zone gain to surface-sides in zone per cavity absorptance
				float cavAbs =   					// compute cavity absorptance for shade position
						zp->adjRmAbs[oc] > 0.
							? sbc.sb_awAbsSlr			// "awAbsSlr": area-weighted abs: computed above
								/ zp->adjRmAbs[oc]		// this increases cavAbs due to re-reflection
							: 0.f;
				sbc.sb_sgf[ oc][ sgcBMXBM] += cavAbs * zp->sgfCavBm[ oc];	// side's cavAbs times zone's factors summed over windows, beam
				sbc.sb_sgf[ oc][ sgcDFXDF] += cavAbs * zp->sgfCavDf[ oc];	// ... diffuse. Add to previous values eg from initial strike.
			}
		}
		// make SgR entries for accumulated gain for surface-side, including exterior surface-sides
		toSurfSide( mx*ss,			// -ss sez no matching zone energy balance entry (devel. expedient)
					si,
					sbc.sb_zi, 				// zone whose shade closure to use
					sbc.sb_sgf);
	}

	ZNR* zp = ZrB.GetAt( ownTi);		// point to zone containing ("inside" of) this wall or window
	if (zp->zn_IsConvRad())
		return;

	float splitCavAbs[2];	// above times surface's cavity absorptance, shades open and closed

	ZNR* zp2 = ZrB.GetAtSafe( x.sfAdjZi);	// point to "outside" zone
											//  NULL if not exposed to a zone


	switch (x.xs_ty)		// cases by XSURF type (cnguts.h defines)
	{
	case CTINTWALL:			// interior surface (wall,ceiling,floor,or door)
		// do "outside"
		if (x.sfExCnd==C_EXCNDCH_ADJZN) 	// if zone on "outside" (might also be eg adiabatic)
		{	float split = x.xs_uval/x.uX;		// frac solar gain going to air on other side (see algebra at top file)
			for (oc = 0;  oc < 2;  oc++)		// loop shades open, shades closed
			{
				splitCavAbs[oc] =
					zp2->adjRmAbs[ oc] > 0.f
						? split *  						// split times cavity absorptance for shade position
							  x.xs_sbcO.sb_awAbsSlr / zp2->adjRmAbs[oc]	// /adjRmAbs increases awAbs for reflected light to yield cavAbs
						: 0.f;

				// from this side's zone, remove "split" of surface's gain from this side zone
				zp2->sgSaBm[oc] -= splitCavAbs[oc] * zp2->sgfCavBm[oc];	// (sgSaBm/Df equivalent to sgfCavBm/Df,
				zp2->sgSaDf[oc] -= splitCavAbs[oc] * zp2->sgfCavDf[oc];	// ... could just use those & drop these)
			}
			// to other side's zone, add "split" of surf's gain from this side zone, using THIS SIDE's shades.
			// can't use accumulator variables for different-zone shades control.
			toZoneCAir( ownTi, x.sfAdjZi,
						splitCavAbs[0] * zp2->sgfCavBm[0],  splitCavAbs[0] * zp2->sgfCavDf[0],
						splitCavAbs[1] * zp2->sgfCavBm[1],  splitCavAbs[1] * zp2->sgfCavDf[1] );
		}
		// fall thru to do inside
	case CTEXTWALL:			// exterior surface: outside outdoors [or adiabatic].
		// do inside
		if (x.sfExCnd != C_EXCNDCH_ADIABATIC)	// if adiabatic on other side, all gain goes to this side
		{
			float split = x.xs_uval/x.uI;		// frac solar gain going to air on other side (see algebra at top file)
			for (oc = 0;  oc < 2;  oc++)		// loop shades open, shades closed
			{
				splitCavAbs[oc] =
					zp->adjRmAbs[ oc] > 0.f
						? split * 							// compute split times cavity absorptance for shade position
							x.xs_sbcI.sb_awAbsSlr / zp->adjRmAbs[oc]	// awAbs, /adjRmAbs for reflected light, = cavAbs.
						: 0.f;

				// from this side's zone, remove "split" of surface's gain from this side zone
				zp->sgSaBm[oc] -= splitCavAbs[oc] * zp->sgfCavBm[oc];
				zp->sgSaDf[oc] -= splitCavAbs[oc] * zp->sgfCavDf[oc];
			}
			// to other side's zone, add "split" of surf's gain from this side zn, using THIS SIDE's shades.
			// can't use accumulator variables for different-zone shades control.
			if (x.sfExCnd==C_EXCNDCH_ADJZN)   		// if zone on other side (if exterior gain just lost)
				toZoneCAir( x.sfAdjZi, ownTi,
							splitCavAbs[0] * zp->sgfCavBm[0],  splitCavAbs[0] * zp->sgfCavDf[0],
							splitCavAbs[1] * zp->sgfCavBm[1],  splitCavAbs[1] * zp->sgfCavDf[1] );
		}

	case CTMXWALL:
		if (x.xs_msi)
		{	MSRAT* ms = MsR.GetAt( x.xs_msi);
			for (si = 0;  si < 2;  si++)			// loop over sides 0 (inside) and 1 (outside)
			{
				MASSBC* side = si ? &ms->outside : &ms->inside;    	// point to substructure for side
				SBC& sbc = x.xs_SBC( si);
#if 0 && defined( _DEBUG)
				if (TrapSurf( ms->name, si))
					printf( "Hit xr_SGMakeSGRATs\n");
#endif
				float split = min( side->bc_h * side->bc_rsurf, 1.f);	// fraction of gain going to adjacent air, not to mass.
																		// min = insurance
				if (sbc.sb_zi && split > 0.f)
				{
					// gain to zone air, when rsurf != 0
					toZoneCAir( sbc.sb_zi, sbc.sb_zi,
						split*sbc.sb_sgf[ socOPEN][ sgcBMXBM],			// bmo
						split*sbc.sb_sgf[ socOPEN][ sgcDFXDF],			// dfo
						split*sbc.sb_sgf[ socCLSD][ sgcBMXBM],			// bmc
						split*sbc.sb_sgf[ socCLSD][ sgcDFXDF]);			// dfc
				}
			}
		}
		break;

	}
	// note return(s) above
}		// XSRAT:xr_SGMakeSGRATs
//-----------------------------------------------------------------------------
void ZNR::zn_SGCavAbs()			// zone cavity absorption
{
	for (int oc = 0;  oc < 2;  oc++)			// control: 0 is shades open, 1 is shades closed diff if any
	{
		// divisor to adjust cavAbs's for reabsorption of reflected light
		//   except what goes out windows. used re CAir, mass sides.
		adjRmAbs[oc] = 1. - (1. - rmAbs)*(1. - rmTrans[oc]);

		// zone air cavity absorptance
		if (adjRmAbs[ oc] > 0.f)
			cavAbsCAir[oc] = rmAbsCAir 			// area-weighted sum of abs's in zone
								 / adjRmAbs[oc];		// increase for re-absorption of reflected energy
		ASSERT( cavAbsCAir[oc] <= 1.0001);
		ASSERT( cavAbsCAir[oc] >= 0);			// our assert macro: fatal error exit if arg false ifndef NDEBUG.
	}
}	// ZNR::zn_SGCavAbs
//-----------------------------------------------------------------------------
void ZNR::zn_SGToZone()		// finalize gains to zone, make zone air SGRAT
{
	for (int oc = 0;  oc < 2;  oc++)		// control: 0 is shades open, 1 is shades closed diff if any
	{
		// distribute quick-surface portion of "cavity" gain to zone air. Some went to masses above; some is lost back out windows.
		sgfCAirBm[oc] += 					// to any previous value from above add
			sgfCavBm[oc] * cavAbsCAir[oc]	// sum from window gains * zone's quick-surface cavity absorptance
			+ sgSaBm[oc];    				// plus "split" adjustments
		sgfCAirDf[oc] +=
			sgfCavDf[oc] * cavAbsCAir[oc] + sgSaDf[oc];	// .. diffuse likewise ..
	}
	// make SGRAT SgR entries for zone CAir solar gain
	toZoneCAir( ss, ss, sgfCAirBm[0], sgfCAirDf[0], sgfCAirBm[1], sgfCAirDf[1] );
}		// ZNR::zn_SGToZone

//======================================================================
// window gain-targeting helper class
//----------------------------------------------------------------------
SgThruWin::SgThruWin(			// c'tor
	XSRAT* xr,				// window
	double tDf1[ socCOUNT],	// unit area diffuse transmitted per unit exterior diffuse, dimless
	double tBm1[ socCOUNT])	// unit area beam+diffuse transmitted per unit exterior beam, dimless
		: tw_xr( xr)
{
	double Ag = tw_xr->x.xs_AreaGlazed();
	for (int oc=0; oc<socCOUNT; oc++)
	{	tw_tDf[ oc] = Ag*tDf1[ oc];		// scale by area
		tw_tBm[ oc] = Ag*tBm1[ oc];
	}
}		// SgThruWin::SgThruWin
//----------------------------------------------------------------------
void SgThruWin::tw_Doit()
{
	TI zi = tw_xr->ownTi;					// windows zone

	// if gain not explicitly targeted, just put it all to window's zone cavity
	//   controlled by that zone's shades
	if (tw_xr->x.nsgdist <= 0)				// if no SGDISTs given by user for window
		tw_ToZoneCav( zi, zi, 1, 1);		// all gain goes to "zone cavity" now for later distribution
       										//   to mass surfaces and zone air.
	else

	// when some gain is explicitly targeted,
	// do initial absorption and reflection for each target from this window now,
	// because distribution can vary by window and differ from cavity absorption weights
	{
		ZNR* zp = ZrB.GetAt( zi);				// window's zone

		// determine gain and area NOT targetted by SGDISTs

		double undistF[2];
		undistF[0]= undistF[1]= 1.;				// init undistributed-by-SGDISTs gain fractions for shades open & closed
		double undistArea = zp->zn_surfASlr;	// init surface area to which no gain yet distributed

		for (int sgi=0; sgi < tw_xr->x.nsgdist; sgi++)	// explicit user-entered targets
		{
			SGDIST* sgd = tw_xr->x.sgdist + sgi; 		// point to sgdist

			// untargeted area
			// determine if new target (search prior SGDISTs)
			BOOL bSeenTarg = tw_HasSGDIST( sgd->sd_targTy, sgd->sd_targTi, sgi);

			// if new target, subtract its area from zone total
			if (!bSeenTarg)
			{	switch (sgd->sd_targTy)	// subtract target area if new target
				{
				case SGDTTZNAIR: case SGDTTZNTOT:
					rer( PWRN, "cgsolar.cpp:makHrSgt(): misplaced obsolete sgdist to zone");
					break;
				case SGDTTSURFO: case SGDTTSURFI:
					undistArea -= XsB[ sgd->sd_targTi].x.xs_area;
					break;
				}
			}

			// untargeted gain
			undistF[0] -= sgd->sd_FSO;			// - gain elsewhere, shades open (VMHLY)
			undistF[1] -= sgd->sd_FSC;			// - gain elsewhere, shades closed (VMHLY)

			// check for excess distribution: sd_FSO/C are runtime-variable
			for (int oc = 0; oc < 2; oc++)			// for shades open, shades closed
				if (undistF[oc] < 0.f)			// issue runtime error msg (exman.cpp) with day/hour:
				{
					rer( (char *)MH_R0162,			// "%g percent of %s solar gain for window '%s'\n" Also used just below
						 // "    of zone '%s' distributed: more than 100 percent.  Check SGDISTs."
						 (1.f - undistF[oc]) * 100.f,		// eg 110%
						 oc ? "shades-closed" : "shades-open",
						 tw_xr->name, zp->name );			// continue here.  aborts elsewhere on too many errors.
					undistF[oc] = 0.;   				// partial fix for fallthru
				}
		}

		// target gain for SGDISTs

		for (int sgi = 0; sgi < tw_xr->x.nsgdist; sgi++)	// explicit user-entered targets
		{
			SGDIST* sgd = tw_xr->x.sgdist + sgi; 		// point to sgdist
			if (sgd->sd_targTy == SGDTTSURFI || sgd->sd_targTy == SGDTTSURFO)
			{	int si = sgd->sd_targTy == SGDTTSURFO;
				tw_HitsSurfSide(  sgd->sd_targTi, si, zi, sgd->sd_FSO, sgd->sd_FSC);
			}
		}

		// target remaining gain if any to remaining surface area if any
		//  loop quick & mass surfaces, do both sides of each
		// adjust to represent unSGDIST'd only
		for (int oc=0; oc<2; oc++)
		{	tw_tDf[ oc] *= undistF[ oc];
			tw_tBm[ oc] *= undistF[ oc];
		}
		double unHitF = 1.;			// fraction of unSGDIST'd gain not yet dist'd here
		if (tw_tDf[ 0] + tw_tDf[ 1] + tw_tBm[ 0] + tw_tBm[ 1] > ABOUT0)	// if any gain remains to distribute
		{
			BOO foundUntSurf = FALSE;		// true if any untargeted surfaces found: unHitF should then be 0.
			if (undistArea > ABOUT0)		// if any area remains to distribute gain to (else to zone air below)
			{
				// targetting un-SGDIST'd gain...: loop quick surfs
				XSRAT* xr1;					// and "xr" points to window's surface.
				RLUP( XsB, xr1)				// loop surfaces to find sides those in zone
				{
					if (!xr1->x.xs_CanBeSGTarget())
						continue;		// skip windows, perims, ..

					if ( xr1->x.sfExCnd==C_EXCNDCH_ADJZN		// if "outside" in a zone (believe redundant: zi 0 if not)
					 &&  xr1->x.sfAdjZi==zi )

					{	if (!tw_HasSGDIST( SGDTTSURFO, xr1->ss)) 		// if not targetted by an SGDIST from this window
						{	foundUntSurf++;
							float w = xr1->x.xs_area / undistArea;		// fraction of remaining gain for this surface
							ASSERT(w <= 1.0001);
							unHitF -= w;				// update remaining fraction
							tw_HitsSurfSide( xr1->ss, 1, zi, w, w);	// distribute gain hitting surface-side (mbr fcn)
						}
					}
					if (xr1->ownTi==zi)				// if "inside" is in window's zone
					{	if (!tw_HasSGDIST( SGDTTSURFI, xr1->ss))   	// if not targetted by an SGDIST
						{	foundUntSurf++;
							float w = xr1->x.xs_area / undistArea;	// fraction of remaining gain for this surface
							ASSERT(w <= 1.0001);
							unHitF -= w;				// update remaining fraction
							tw_HitsSurfSide( xr1->ss, 0, zi, w, w);	// distribute gain hitting surface-side (mbr fcn)
						}
					}
				}
			}   // if any area

#ifdef DEBUG
			// if found any untargeted surface(s) to rcv untargeted gain,
			//  should have found surf for all gain to strike
			if ( foundUntSurf && unHitF > ABOUT0  ||  unHitF < -ABOUT0 )
				rer( (char *)MH_R0164, zp->name, tw_xr->name, unHitF);
				/* "cgsolar.cpp:SgThruWin::doit(): zone \"%s\", window \"%s\":\n"
					"    undistributed gain fraction is %g (should be 0)." */
#endif

			// gain that stuck no surface goes to zone air -- when SGDISTs target all surfaces with less than 100% of gain
			tw_ToZoneCAir( zi, zi, unHitF, unHitF);		// implicit absorptivity 1.0 !?

		}  // if any gain
	}  // if no SGDISTs ... else ...
}					// SgThruWin::tw_Doit
//----------------------------------------------------------------------
void SgThruWin::tw_HitsSurfSide(		// distribute gain that initially falls on side of a surface
	// uses accumulators for speed where applicable
	TI sfi,			// which surface (XsR subscript)
	int si,			// which side: 0 = inside, 1 = outside
	TI czi,			// control zone (zone whose shades position used)
	float fo, float fc )  	// weights (fractions of window's gain), shades open and closed

{
	XSRAT* xr = XsB.GetAt( sfi);
	SBC& sbc = xr->x.xs_SBC( si);
	TI zi = sbc.sb_zi;					// side's zone subsript, if in a zone (else 0)
	float abs = xr->x.xs_AbsSlr( si);	// fraction absorbed.

// if side is in a zone, initial reflection to other surfaces goes to zone "cavity"
	if (zi)
	{	ZNR* zp = ZrB.GetAt( zi);				// side's zone
		tw_ToZoneCav( zi, 	 					// target: side's zone
				   czi, 						// control zone (shades): per caller
				   fo * (1 - abs) * (1 - zp->rmTrans[0]),   	// abs is portion absorbed, ...
				   fc * (1 - abs) * (1 - zp->rmTrans[1]) );	// ... rmTrans is portion that goes out windows.
	}

	if (!xr->x.xs_IsDelayed())
	{	// portion of gain going thru: because of surface film, some of insolation on surface goes thru to air on other side.
		// See algebraic proof before code in cgsolar.cpp re computation as uval/uX
		float split = min( 1.f,
			               si                                 ?  xr->x.xs_uval/xr->x.uX	// portion from outside to inside
				         : xr->x.sfExCnd==C_EXCNDCH_ADIABATIC ? 0.f
				         :                                      xr->x.xs_uval/xr->x.uI);	// portion from inside to outside

		// if side in a zone, that zone receives all but "split" of insolation
		if (si == 0)
			tw_ToZoneCAir( zi, czi, (1 - split) * fo * abs, (1 - split) * fc * abs);

		// if other side in a zone, that zone receives "split" of insolation
		if ( si==1					// other side of outside is inside: always in a zone
			||  xr->x.sfExCnd==C_EXCNDCH_ADJZN )	// if outside in zone (CTINTWALL), both sides in zones
			tw_ToZoneCAir( si ? xr->ownTi : xr->x.sfAdjZi,  czi,  split * fo * abs,  split * fc * abs);
	}
	else if (zi==czi)
	{	// when controlled by same zone's shade position, just accumulate for now
		// caller later makes SgR entries
		sbc.sb_sgf[ 0][ sgcBMXBM] += tw_tBm[ 0] * fo * abs;
		sbc.sb_sgf[ 0][ sgcDFXDF] += tw_tDf[ 0] * fo * abs;
		sbc.sb_sgf[ 1][ sgcBMXBM] += tw_tBm[ 1] * fc * abs;
		sbc.sb_sgf[ 1][ sgcDFXDF] += tw_tDf[ 1] * fc * abs;
	}
	else
	{	warn( "Unhandled case");

	}
}		// SgThruWin::tw_HitsSurfSide
//----------------------------------------------------------------------
void SgThruWin::tw_ToZoneCAir( 		// put gain to zone air
	TI zi, 			// target zone
	TI czi,			// control zone (zone whose shades position used)
	float fo, float fc )  	// weights (fractions of window's gain), shades open and closed
{
// when control zone matches target zone, add gain to zone accumulators.
// caller then calls sgrAdd once (faster).
	if (czi==zi)
	{
		ZNR * zp = ZrB.p + zi;
		zp->sgfCAirBm[ 0] += tw_tBm[ 0] * fo;
		zp->sgfCAirDf[ 0] += tw_tDf[ 0] * fo;
		zp->sgfCAirBm[ 1] += tw_tBm[ 1] * fc;
		zp->sgfCAirDf[ 1] += tw_tDf[ 1] * fc;
	}
	else

// for different control zone, add SgR entry now. Happens eg for "split" thru walls.

	::toZoneCAir( zi, czi, tw_tBm[ 0] * fo, tw_tDf[ 0] * fo, tw_tBm[ 1] * fc, tw_tDf[ 1] * fc);

}			// SgThruWin::tw_ToZoneCAir
//----------------------------------------------------------------------
void SgThruWin::tw_ToZoneCav( 		// put gain to zone cavity
	TI zi, 			// target zone
	TI czi,			// control zone (zone whose shades position used)
	float fo, float fc )  	// weights (fractions of window's gain), shades open and closed
{
	ZNR * zp = ZrB.p + zi;					// point target zone record
	if (czi != zi)						// if use of shades in a different zone requested
	{
		// non-matching control zone only expected in future if SGDISTs into other zones allowed.
		// to implement, must distribute to target zone using control zone's znSC in sgrAdd calls.
		// meanwhile, issue message and fall thru to use wrong control zone.

		rer( PWRN, (char *)MH_R0165, ZrB[ czi].name, zp->name);	/* "cgsolar.cpp:SgThruWin::toZoneCav:\n"
								   "    control zone (\"%s\") differs from target zone (\"%s\")."*/
	}

// when control zone matches target zone, add gain to zone member accumulators for later distribution (faster)
	zp->sgfCavBm[0] += tw_tBm[ 0] * fo;		// sgfCavBm and -Df are distributed among surfaces
	zp->sgfCavDf[0] += tw_tDf[ 0] * fo;	 	// ... and sometimes CAir by caller after all windows processed.
	zp->sgfCavBm[1] += tw_tBm[ 1] * fc;
	zp->sgfCavDf[1] += tw_tDf[ 1] * fc;
}				// SgThruWin::tw_ToZoneCav
//----------------------------------------------------------------------
BOOL SgThruWin::tw_HasSGDIST(		// determine if window has specified SGDIST
	int targTy,			// target type
	TI targTi,			// target idx
	int sgiEnd /*=-1*/)	// optional search end (default = all)
{
	if (sgiEnd < 0)
		sgiEnd = tw_xr->x.nsgdist;
	SGDIST* sgd = tw_xr->x.sgdist;
	for (int sgi=0;  sgi < sgiEnd;  sgi++)			// loop window's explicit SGDISTs
		if (sgd[ sgi].sd_targTi==targTi && sgd[ sgi].sd_targTy==targTy)
			return TRUE;		// match
	return FALSE;
}			// SgThruWin::tw_HasSGDIST
//=============================================================================
void ZNR::zn_DbDumpSGDIST(		// dump zone solar gain distribution values
	const char* tag) const
{
	DbPrintf( "%s  %s  rmAbs=%0.3f  rmAbsCAir=%0.3f\n"
		"     rmTrans[ 0]=%0.3f   sgfCavBm[ 0]=%0.3f   sgfCavDf[ 0]=%0.3f   sgfCAirBm[ 0]=%0.3f  sgfCAirDf[ 0]=%0.3f\n"
		"     rmTrans[ 1]=%0.3f   sgfCavBm[ 1]=%0.3f   sgfCavDf[ 1]=%0.3f   sgfCAirBm[ 1]=%0.3f  sgfCAirDf[ 1]=%0.3f\n",
		tag, name, rmAbs, rmAbsCAir,
		rmTrans[ 0], sgfCavBm[ 0], sgfCavDf[ 0], sgfCAirBm[ 0], sgfCAirDf[ 0],
		rmTrans[ 1], sgfCavBm[ 1], sgfCavDf[ 1], sgfCAirBm[ 1], sgfCAirDf[ 1]);

}	// ZNR::zn_DbDumpSGDIST
//=============================================================================

//=============================================================================
// non-member solar gain functions
//----------------------------------------------------------------------
LOCAL void toSurfSide( 	// add SgR entries for solar gain to surface side

	TI xsi, 			// which surface (XsR subscript)
						//   if <0, no matching zone energy bal entry
	SI si, 				// which side: 0 = inside, 1 = outside
	TI czi, 			// control zone subscript (zone whose shade closure .znSC to use)
	const double sgf[ socCOUNT][ sgcCOUNT])		// gain factors
// does not use accumulators -- is used to distribute accumulated gain.
{
	RC rc;
	// shades open
	rc = sgrAdd( 					// create or add to SGRAT SgR record; zone total also; nop if gains small or zero.
		si ? SGDTTSURFO : SGDTTSURFI,     	// add gain to surface outside or inzide
		xsi,								// surface subscript
		NULL,  								// no control variable (shades open)
		sgf[ socOPEN][ sgcBMXBM], sgf[ socOPEN][ sgcDFXDF], sgf[ socOPEN][ sgcDFXBM]);

	// shades closed
	float* ctrl		// control variable pointer for shades-closed difference
		= czi ? &ZrB[ czi].i.znSC : NULL;
	rc = sgrAdd(
		si ? SGDTTSURFO : SGDTTSURFI,     	// add gain to mass outside or inzide
		xsi,								// surface subscript
		ctrl,
		sgf[ socCLSD][ sgcBMXBM] - sgf[ socOPEN][ sgcBMXBM],
		sgf[ socCLSD][ sgcDFXDF] - sgf[ socOPEN][ sgcDFXDF],
		sgf[ socCLSD][ sgcDFXBM] - sgf[ socOPEN][ sgcDFXBM]);

#if defined( _DEBUG)
	if (rc == RCOK && !ctrl)
		// RCOK means non-0 shade closed values (else return = RCNOP)
		//  in which case, there should be a control zone
		printf( "toSurfSide inconsistency\n");
#endif

}		// toSurfSide
//----------------------------------------------------------------------
LOCAL void toZoneCAir( 		// make SGRAT SgR entries to put gain to zone "air" heat capacity
	TI zi, 				// which zone
	TI czi, 			// control zone subscript (zone whose shade closure .znSC to use)
	float bmo, float dfo, 	// beam & diffuse gain for shades open
	float bmc, float dfc )	// ditto for shades closed
// does not use accumulators -- is used to distribute accumulated gain.
{
// shades open
	sgrAdd( 					// add gains to SgR entry (create if nec) / add to zone total also
		SGDTTZNAIR, zi,			// target type (cnguts.h) and zone subscript
		NULL, 					// control ptr: none for shades-open values
		bmo, dfo );   			// beam & diffuse values for shades open

// shades closed DIFFERENCE, gains multiplied by zone shade fraction during run
	sgrAdd( 					// nop if beam and diffuse are 0 (if no shades closed same as open)
		SGDTTZNAIR, zi,
		&ZrB[ czi].i.znSC, 		// control ptr: control zone shade closure fraction
		bmc - bmo, dfc - dfo );		// beam & diffuse: shades closed difference
}					// toZoneCAir
//----------------------------------------------------------------------
LOCAL RC sgrAdd(	// add solar gain, including appropriate zone total.
	int targTy,  		// target type (cnguts.h): determines basAnc and member of record to receive gain
	TI targTi,  		// target record subscript within basAnc's records
						//   if <0, no matching zone energy balance
	float* ctrl, 		// NULL or pointer to control variable (0..1 multiplies gain during simulation)
						// gain to add target per unit exterior irrad (Btuh or Btuh/ft2 per caller)
	double bmBm,			// beam or total per unit beam
	double dfDf,			// diffuse per unit diffuse
	double bmDf /*=0.*/)	// diffuse per unit beam (if needed)
// also uses Top.iHrST
// returns
//    RCNOP: did nothing (all gains 0)
//    RCBAD: inappropiate call (msg issued)
//    RCOK:  success
{

// ignore insignificant gains
//   save time of finding or adding entry here, save time of using it each hour or subhour of run.
//   many 0's expected, eg when there is no shade closure difference.
//   fabs's required because negative values expected for shade closure difference.
	if (fabs(bmBm) + fabs(dfDf) + fabs( bmDf) < .0005)
		return RCNOP;

	BOOL bMatching = targTi > 0;
	targTi = abs( targTi);

// determine address of target (all reallocation of ZNR, MSRAT done), and other stuff

	TI tzi = targTi;		// target zone is targTi for zone targets
	BOO isSubhrly = TRUE;	// TRUE for ZNAIR
#ifdef SOLAVNEND // undef in cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
o   BOO isEndIvl = FALSE;	// non-0 for end-time-interval gain (zone air), 0 for interval-average (mass)
#endif
	ZNR* zp = NULL;
	SGTARG* pTarg = NULL; 	// beam target address
	const char* sgName = NULL;
	switch (targTy)		// see .targTy comments in cnguts.h
	{
	case SGDTTSURFO:
	case SGDTTSURFI:
		{	int si = targTy == SGDTTSURFO;
			XSRAT* xr = XsB.GetAt( targTi);
			SBC& sbc = xr->x.xs_SBC( si);
			pTarg = &sbc.sb_sgTarg;		// target
			tzi = sbc.sb_zi;   			// get zone containing SBC, or 0
#if 0 && defined( _DEBUG)
			if (TrapSurf( xr->name, si))
				printf( "Hit surf\n");
#endif
			// zp = ZrB.GetAt( xr->ownTi);
			sgName = strtprintf( "S%c %s", "IO"[ si], xr->name);
		}
		break;

	case SGDTTZNTOT:   					// zone total: can be internally generated only, 2-95
		rer( PWRN, (char *)MH_R0166, targTy);	// "cgsolar.cpp:sgrAdd(): called for SGDTT %d"
		return RCBAD;

	case SGDTTZNAIR:
		zp = ZrB.GetAt( targTi);
		pTarg = &zp->zn_sgAirTarg;		// zone air: always subhourly (preset)
		sgName = strtprintf( "ZA %s", zp->name);
#ifdef SOLAVNEND
o		isEndIvl = TRUE;
#endif
		break;
	}

// add specified gain: creates record if necessary, else adds to members of existing record. Just below.
	sgrPut(
		sgName,			// name (documentation for e.g. DbDump)
		pTarg,			// gain "target" pointer: where to add gain each hour or subhour
		ctrl, 			// NULL or control variable (0..1) pointer -- multiples gain
		isSubhrly,
#ifdef SOLAVNEND
o           isEndIvl,
#endif
		bmBm, dfDf, bmDf);

// add matching gain to zone total used for zeb report and ebal check
	if (tzi && bMatching)
	{	zp = ZrB.GetAt( tzi);
		sgrPut(
			strtprintf("ZT %s", zp->name),
			isSubhrly ? &zp->zn_sgTotShTarg : &zp->zn_sgTotTarg,	// target: zone subhourly or hourly total accumulator
			ctrl,							// remaining args same as above so gain matches
			isSubhrly,
#ifdef SOLAVNEND
o           isEndIvl,
#endif
			bmBm, dfDf, bmDf);
	}

	return RCOK;
}				// sgrAdd
//======================================================================
LOCAL void sgrPut(
// add gain to Solar Gains record (SGRAT entry) for given target and control variable
// create record if necessary.

	const char* sgName,		// doc-only name for SGRAT; used *only* when SGRAT is created
							//  not checked when accuming to existing
	SGTARG* pTarg,			// target beam variable to which gain is to be added
	float* pControl, 		// NULL pointer to control variable
	BOO isSubhrly,			// non-0 for a subhourly gain distribution, FALSE for hourly.
#ifdef SOLAVNEND // undef in cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
o   BOO isEndIvl,			// non-0 for end-time-interval gain, 0 for interval-average
#endif
							// gain to this target (Btuh or Btuh/ft2 per caller)
	double bmBm,			//   beam or total per unit exterior beam
	double dfDf,			//   diffuse per unit exterior diffuse
	double bmDf /*=0.*/)	//   diffuse per unit exterior beam
// also uses Top.iHrST.   CAUTION can move other sge's.
{
#if 0
	if (!Top.isWarmup && Top.iHrST == 12)
	{	DbPrintf( "%s: %d  %p %p  %0.4f %0.4f %0.4f\n",
			sgName, isSubhrly, pTarg, pControl, bmBm, dfDf, bmDf);
	}
#endif

// Look for existing SGRAT entries for same target and control
	SGRAT* sge;
	BOO found = FALSE;
	BOO addIt = FALSE; 		// tentatively, 1st entry for targ
							// note: addIt != 0 probably now 3-90 corresponds to control != NULL
							//    but implementation kept independent of that.
	RLUP( SgR, sge)			// for sge = solar gain record 1 to n
	{
		if (sge->sg_pTarg==pTarg)		// does this entry have our target?
		{ 	addIt++;							// yes, if make new entry, say ADD gain to target during simulation
			if (sge->sg_isSubhrly==isSubhrly)	// does this entry have our subhourly-ness?
			{
#ifdef SOLAVNEND	// never compiled nor run since recoded here, 2-95.
o				if (sge->isEndIvl==isEndIvl)
#endif
				{
					if (sge->sg_pControl==pControl)    	// yes, does it also have our control?
					{	found++;			// yes, say found,
						break;  			// and stop searching
					}
				}
			}
		}
	}

// If not found, add a new SGRAT record
	if (!found)
	{	SgR.add( &sge, ABT);			// add all-0 record, return ptr to it. ancrec.cpp. CAN MOVE SgR.p.
		sge->sg_pTarg = pTarg;  		// init members of new record: our target ptr,
		sge->sg_pControl = pControl;	//  control variable ptr
		sge->sg_isSubhrly = isSubhrly;	//  subhourly flag
#ifdef SOLAVNEND
o     sge->isEndIvl = isEndIvl;
#endif
		sge->sg_addIt = addIt;			//  non-0 if NOT first entry for targ: makes sg_ToTarg add not store.
		if (sgName)
			sge->SetName( sgName);
	}

#if 0
x	if ((Top.iHr == 0 && DbDo( dbdSGDIST)) || DbShouldPrint( dbdSGDIST))
x	{	if (strMatch( sgName, "SI Front"))
x			DbPrintf( "iHr=%d iHrST=%d  sg_dfDfF[ iHrSt]=%.3f  sg_dfDfF[ 23]=%.3f    dfDf=%.3f\n",
x				Top.iHr, Top.iHrST, sge->sg_dfDfF[ Top.iHrST], sge->sg_dfDfF[ 23], dfDf);
x	}
#endif
// add gains given by caller. Note SgR.add zeroed record at creation.
	sge->sg_bmXBmF[ Top.iHrST] += bmBm;
	sge->sg_dfXBmF[ Top.iHrST] += bmDf;
	sge->sg_dfXDfF[ Top.iHrST] += dfDf;

}				// sgrPut
//======================================================================

// end of cgsolar.cpp

