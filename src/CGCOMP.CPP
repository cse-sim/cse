// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cgcomp.c: Utility functions for CSE simulator component models

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>
#include "MSGHANS.H"
#include "PSYCHRO.H"
#include "NUMMETH.H"

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>
#include "TIMER.H"
#include <EXMAN.H>
#include <CVPAK.H>

#include <IRATS.H>	// input RATs
#include <CNGUTS.H>	// decls for this file, IzxR


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL float FC NEAR cgnveffa( float a1, float a2);

/* ********************* FAN VENTILATION **************************** */
#ifdef OLDFV // cndefns.h
o//======================================================================
oVOID FC cgfvinit(z)
o
o/* fan vent setup: derive values that do not change during run */
o
o	ZNR *z;	/* Hs zone runtime record:
o	   Uses:  .i.fvCfm	fan full volume flow (cfm)
o		  .i.fvKW 	full volume power (Btuh) (formerly KW 2-91)
o		  .i.fvFz  	fraction of fan heat which
o		 		ends up in supply stream to zone
o		  .i.fvTD	control temp difference
o	   Sets:  .havmax	heat xfer at full flow (Btu/F)
o		  .i.fvTD 	limited to > 0
o		  .tRise  	supply air temp rise (F) */
o
o{
o    z->havmax = 			// max ht xfer at full flow (Btu/F)
o    	Airxcfm 			// air heat xfer, set in cnguts:cgInit() (vhc * 60, Btuh/cfm-F)
o    	* z->i.fvCfm;
o    z->tRise  = z->i.fvFz * z->i.fvKW / z->havmax;	// temp rise in supply air stream
o    if (z->i.fvTD < 0.01F)
o       z->i.fvTD = 0.01F;			// force tdiff > 0; elims x/0 cases */
o}		           // cgfvinit
#endif
#ifdef OLDFV
o/* *********************** NATURAL VENTILATION ********************** */
o//----------------------------------------------------------------------------
oVOID FC cgnvinit(z)
o
o/* Init nat vent: set derived members that do not change during run */
o
o			/* see also: cgnvhinit(): hourly natv init */
o
oZNR *z; 	/* pointer to zone runtime record.
o		   Uses:  .i.nvAHi, .i.nvALo, .i.nvHD, .i.nvAzmI,
o			  .i.nvStEM, .i.nvDdEM, .i.nvDiEM.
o		   Sets:  .dieff, .iazm, .fwind, .fstack. */
o
o{
ofloat earea, earea2;
o
o    z->iazm = ((SI)(z->i.nvAzmI)+720)%360;
o    z->dieff = z->i.nvDiEM * 0.55;
o
o    earea = cgnveffa( z->i.nvAHi, z->i.nvALo);
o    earea2 = earea * earea;
o
o/* Pre-square these factors to save time later */
o    z->fwind = (float)( 88.f * 88.f * earea2
o    		          * Airxcfm2 );		// air heat xfer squared, cnguts.c
o    z->fstack = z->i.nvHD * 9.4f * 9.4f *
o	      		z->i.nvStEM * z->i.nvStEM * earea2 * Airxcfm2;
o}       /* cgnvinit */
#endif
//----------------------------------------------------------------------------
LOCAL float FC NEAR cgnveffa(	/* effective area of vents in series */

	float a1,	// vent areas (any consistent units)
	float a2 )

// returns effective area (same units as a1, a2) for flow in series.

{
	if (a2 > a1)			/* if a2 is the larger */
	{
		float t = a2;
		a2 = a1;
		a1 = t;	/* swap so a1 is the larger */
	}
	return a1 <= 0.f
		   ? 0.f				// 0 (or negative) areas: return 0
		   : a2 * sqrt( 2.f/(1.f+(a2*a2)/(a1*a1)) );	/* non-0 areas:
	  							a1 == a2: return a2
								a1 >> a2: return a2*sqrt(2) */
}		// cgnveffa

#ifdef OLDNV
o//-----------------------------------------------------------------------------
oVOID FC cgnvhnit(z)
o
o/* Hourly initialization of nat vent for zone */
o
oZNR *z;
o
o/* Returns with hourly factors set up for use in the simulation.
o   Uses global weather values. */
o{
ofloat eff;
o SI daz;
o
o/* Direction independent effectiveness */
o    eff = z->dieff;
o
o/* Add direction dependent if it is enabled */
o    if (z->i.nvDdEM > 0.F)
o    {	daz = abs(z->iazm-(SI)(Top.windDirDeg);
o       if (daz > 180)
o			daz = 360 - daz;
o       eff +=
o			(float)fabs((double)(z->i.nvDdEM)*(.006111*(double)daz - .55));
o}
o
o/* Combined hourly coeff, used in cgzone() */
o    z->chour = z->fwind*Top.windSpeedSquared*eff*eff;
o
o}		     /* cgnvhnit */
#endif

/* *********************** INFILTRATION ************************** */
//-----------------------------------------------------------------------------
RC ZNR::zn_InfilSetup()		// Initialize infiltration values for zone

// uses: i.zn_infShld     shielding 1-5
//       i.zn_infStories  stories 1-3
//       Top.windF		  wind factor to cancel since Sherman-Grimsrud model has its own.

// sets: zn_stackc:       stack coefficient
//       zn_windc:        wind coefficient

// Constants & methods in this fcn derived from ASHRAE HOF, 1985, Chapter 22
{
	int st = bracket( 1, int( i.zn_infStories), 3);		// limit stories 1 - 3
	int sh = bracket( 1, int( i.zn_infShld), 5);		// limit shielding 1 - 5

#if 0
x	// wind factor: V( eaveZ) = zn_fWindAN * Vmet
x	zn_fWindAN = Top.tp_WindFactor( zn_HeightZ( 1.f), sh);
#endif

#if 0 && defined( Z_BYZONE)
// stories  1	    2	      3
static float _windc[5][3] =
 { { 0.0119F,  0.0157F,  0.0184F },  	// shielding 1
   { 0.0092F,  0.0121F,  0.0143F },
   { 0.0065F,  0.0086F,  0.0101F },
   { 0.0039F,  0.0051F,  0.0060F },
   { 0.0012F,  0.0016F,  0.0018F }};	// shielding 5
static float _stackc[3] =
   { 0.0156F,  0.0313F,  0.0471F };

	float f813 = Top.tp_WindFactor( 8.f, 1, 3);
	float fx = Top.tp_WindFactor( i.zn_eaveZ, sh);

	float stx = 144. * 144. *.0119 * fx / f813;

// look up stack and wind coefficients, and convert values intended for in2 for infELA now stored in ft2.
	zn_stackc = _stackc[st-1] * 144. * 144.;			// stack coefficient
	zn_windc  = _windc[sh-1][st-1] * 144. * 144.;	// wind coeff
#else
// stories  1	    2	      3
static float _windc[5][3] =
 { { 0.0119F,  0.0157F,  0.0184F },  	// shielding 1
   { 0.0092F,  0.0121F,  0.0143F },
   { 0.0065F,  0.0086F,  0.0101F },
   { 0.0039F,  0.0051F,  0.0060F },
   { 0.0012F,  0.0016F,  0.0018F }};	// shielding 5
static float _stackc[3] =
   { 0.0156F,  0.0313F,  0.0471F };

// look up stack and wind coefficients, and convert values intended for in2 for infELA now stored in ft2.
	zn_stackc = _stackc[st-1] * 144. * 144.;			// stack coefficient
	zn_windc  = _windc[sh-1][st-1] * 144. * 144.;	// wind coeff

// adjust windc to cancel windfactor applied to weather data, cuz model has its own.
	float windF = max( Top.windF, .001F);	// don't divide by 0
	zn_windc /= windF * windF;				// is multiplied by hourly windSpeed squared.
#endif	// Z_BYZONE

// NOTE: infAC and infELA have hourly variability, cannot pre-calc derived factors
//       See ZNR.zn_LoadsSubhr()

// *** Airnet-based infiltration ***
// TODO: generate AirNet elements from zone ELA 9-10

	return RCOK;

}           // ZNR::zn_InfilSetup
//-------------------------------------------------------------------------------
float TOPRAT::tp_WindFactor(				// local wind factor
	float Z,						// height above ground for factor, ft
									//   if Z <=0, returns 0.
	int shieldClass,				// shielding class (1 - 5)
	int terrainClass /*=0*/) const	// terrain class (1 - 5)
									// default = use tp_TerrainClass

// returns WF: V(Z) = WF * Vmet
{
static float fTerrain[ 5][ 2] =
{
// gamma   alpha
   0.10f,  1.30f,	// ocean or other body of water with at least 5 km unrestriced expanse
   0.15f,  1.00f,	// flat terrain with some isolated obstacles (buildings or trees well separated)
   0.20f,  0.85f,	// rural areas with low buildings, trees, etc.
   0.25f,  0.67f,	// urban, industrial, or forest areas
   0.35f,  0.47f	// center of large city
};
static float fShield[ 5][ 2] =
{
//     C'    SC
   0.324f,  1.000f,	// no obstructions or local shielding
   0.285f,  0.880f,	// light local shielding with few obstructions
   0.240f,  0.741f,	// moderate local shielding, some obstructions within two house heights
   0.185f,  0.571f,	// heavy shielding, obstructions around most of the perimeter
   0.102f,  0.315f	// very heavy shielding, large obstructions surrounding the perimeter
					//   within two house heights
};

	if (Z <= 0.f)
		return 0.f;

	float SC = fShield[ bracket( 1, shieldClass, 5)-1][ 1];
	if (terrainClass <= 0)
		terrainClass = tp_terrainClass;
	float* pTer = fTerrain[ bracket( 1, terrainClass, 5)-1];
	float gamma = pTer[ 0];
	float alpha = pTer[ 1];

	float f = SC * alpha * pow( Z / 32.8f, gamma);
	return f;
}		// TOPRAT::tp_WindFactor
//-------------------------------------------------------------------------------
float TOPRAT::tp_WindPresV(			// wind velocity pressure
	float windV) const			// wind velocity at eave height, mph
// returns wind velocity pressure, lbf / ft2
{
	float vx = windV * 5280.f / 3600.f;		// adjusted wind vel, ft/sec
	return float( .5 * tp_rhoMoistOSh * vx * vx / g0Std);
}	// TOPRAT::tp_WindPresV
//===============================================================================

///////////////////////////////////////////////////////////////////////////////
// AFMTR_IVL, AFMTR: accumulates air mass flow by category
///////////////////////////////////////////////////////////////////////////////
/*static*/ const int AFMTR_IVL::NAFCATS
	= (sizeof(AFMTR_IVL) - offsetof( AFMTR_IVL, amt_total)) / sizeof(float);
// NAFCATS s/b same as AFCAT choices + 1 (for total) - 1 (for C_AFCAT_NONE)
static_assert(AFMTR_IVL::NAFCATS == C_AFCAT_COUNT+1-1, "Inconsistent AFMTR constants");
//-----------------------------------------------------------------------------
void AFMTR_IVL::amti_Copy(			// copy 
	const AFMTR_IVL* sIvl,	// source
	float mult /*=1.f*/)	// optional multiplier
{
	if (mult == 1.f)
		memcpy(this, sIvl, sizeof(AFMTR_IVL));
	else
	{	amt_count = sIvl->amt_count;
		VCopy(&amt_total, NAFCATS, &sIvl->amt_total, double(mult));
	}
}	// AFMTR_IVL::amt_Copy
//-----------------------------------------------------------------------------
void AFMTR_IVL::amti_Accum(			// accumulate
	const AFMTR_IVL* sIvl,		// source
	int firstFlg,				// true iff first accum into this (beg of ivl)
	int lastFlg,				// true iff last accum into this (end of ivl)
	int options /*=0*/)			// option bits
								//   1: use sIvl.amt_count to scale totals
								//      effectively averages by day for annual values
								//   2: sum only (do not average)
{
	int count = (options & 1) ? sIvl->amt_count : 1;
	if (firstFlg)
	{	amti_Copy(sIvl, float( count));
		amt_count = count;
	}
	else
	{	VAccum(&amt_total, NAFCATS, &sIvl->amt_total, float( count));
		amt_count += count;
	}

	// average unless caller says don't
	//  Note: VMul1() nops if amt_count==1
	if (lastFlg && !(options & 2))
		VMul1(&amt_total, NAFCATS, 1.f / amt_count);
}		// AFMTR_IVL
//-----------------------------------------------------------------------------
RC AFMTR::amt_CkF()
{
	return RCOK;
}
//-----------------------------------------------------------------------------
RC AFMTR::amt_BegSubhr()		// init at beg of subhr
{
	S[ 0].amti_Clear();
	S[ 1].amti_Clear();
	return RCOK;
}		// AFMTR::amt_BegSubhr
//-----------------------------------------------------------------------------
void AFMTR::amt_AccumCat(		// accumulate air flow value for current subhour
	AFCAT afCat,	// air flow category
	float amf)		// air mass flow, lbm/sec
{
	int iNeg = amf < 0.f;
	S[ iNeg].amti_AccumCat(afCat, AMFtoAVF2(amf));
}		// AFMTR::amt_AccumCat
//-----------------------------------------------------------------------------
AFMTR_IVL* AFMTR::amt_GetAFMTR_IVL(
	IVLCH ivl,		// interval
	int iPN /*=0*/)	// flow direction 0: pos (in), 1: neg (out)
{
	return Y + 2 * (ivl - C_IVLCH_Y) + iPN;
}
//-----------------------------------------------------------------------------
void AFMTR::amt_Accum(
	IVLCH ivl,		// destination interval: hour/day/month/year
					// Accumulates from subhour/hour/day/month.  Not Top.ivl!
	int firstFlg,	// iff TRUE, destination will be initialized before values are accumulated into it
	int lastFlg)	// iff TRUE, destination averages will be computed as needed
{
	AFMTR_IVL* dIvl0 = amt_GetAFMTR_IVL( ivl);	// point destination substruct for interval
													// ASSUMES interval members ordered like DTIVLCH choices
	AFMTR_IVL* sIvl0 = amt_GetAFMTR_IVL(ivl + 1);	// source: next shorter interval

	int options =
		ivl == C_IVLCH_Y ? 1	// average by day for non-sum-of year values
								//   handles diffs due to month length
		:                  0;	// default: track average
	for (int iPN=0; iPN<2; iPN++)
		dIvl0[ iPN].amti_Accum(sIvl0+iPN, firstFlg, lastFlg, options);

}		// AFMTR::amt_Accum
//=============================================================================


/////////////////////////////////////////////////////////////////////////////////
// AIRSTATE
/////////////////////////////////////////////////////////////////////////////////
void AIRSTATE::as_Set(const AIRFLOW& af)
// copy AIRFLOW state (ignore af_amf)
{
	as_tdb = af.as_tdb;
	as_w = af.as_w;
}	// AIRSTATE::as_Set
//------------------------------------------------------------------------------
int AIRSTATE::as_IsEqual( const AIRSTATE& as, double tol/*=.001*/) const
{	return frDiff( as_tdb, as.as_tdb) < tol
        && frDiff( as_w,   as.as_w)   < tol;
}		// AIRSTATE_as_IsEqual
//------------------------------------------------------------------------------
double AIRSTATE::as_AddQSen(		// add sensible heat to an air stream
	double q,		// heat to be added, Btuh (+ = into air stream)
	double amf)		// dry air mass flow rate, lbm/hr
// air specific heat adjusted per as_w
// returns updated dry-bulb temp, F
{
	if (amf != 0.)
		as_tdb += q / fabs( amf*(PsyShDryAir + as_w*PsyShWtrVpr));
	return as_tdb;
}		// AIRSTATE::as_AddQSen
//-------------------------------------------------------------------------------
double AIRSTATE::as_AddQSen2(		// add sensible heat to an air stream (constant specific heat)
	double q,		// heat to be added, Btuh (+ = into air stream)
	double amf)		// dry air mass flow rate, lbm/hr
// air specific heat: constant Top.tp_airSH
// returns updated dry-bulb temp, F
{
	if (amf != 0.)
		as_tdb += q / fabs( amf * Top.tp_airSH);
	return as_tdb;
}		// AIRSTATE::as_AddQSen2
//------------------------------------------------------------------------------
double AIRSTATE::as_QSenDiff2( const AIRSTATE& as, double amf) const
{	return amf * Top.tp_airSH * (as_tdb - as.as_tdb);
}		// AIRSTATE::as_QSenDiff2
//------------------------------------------------------------------------------
double AIRSTATE::as_AddQLat(		// add latent heat to an air stream
	double q,		// heat to be added, Btuh (+ = into air stream)
	double amf)		// dry air mass flow rate, lbm/hr
// returns updated humidity ratio, (lbm water)/(lbm dry air)
{
	if (amf != 0.)
	{
		double mw = q / PsyHCondWtr;		// water addition rate, lbm/hr
		as_w += mw / fabs( amf);			// direction of flow does not matter
		if (as_w < PsyWmin)
		{
#if 0 && defined( _DEBUG)
			warn( "w too small!");
#endif
			as_w = PsyWmin;
		}
	}
	return as_w;
}		// AIRSTATE::as_AddQLat
//------------------------------------------------------------------------------
int AIRSTATE::as_AddQSenLat(		// add sensible and latent heat to an air stream
	float& qSen,		// sensible heat to be added, Btuh (+ = into air stream)
	float& qLat,		// latent heat to be added, Btuh (+ = into air stream)
	double amf,			// dry air mass flow rate, lbm/hr
	float rhMax/*=1.f*/)	// rh limit for leaving air
// updates as_tdb and as_w
// if result exceeds rhMax, qLat and qSen are adjusted
// returns -1: can't calc (amf = 0 etc.)
//          0: all OK
//          1: qSen and/or qLat have been adjusted
{
	double absAmf = fabs( amf);		// flow direction does not matter
	if (absAmf < 0.00001)
		return -1;		// no flow

#if defined( _DEBUG)
	float hIn = as_Enthalpy();
#endif
	int ret = 0;
	double wIn = as_w;
	double hSenIn = as_tdb*(PsyShDryAir + as_w*PsyShWtrVpr);
	as_w += qLat / (absAmf*PsyHCondWtr);		// water addition rate, lbm/hr
	if (as_w < PsyWmin)
	{	float qX = absAmf*(as_w - PsyWmin)*PsyHCondWtr;	// convert to sensible
		as_w = PsyWmin;
		qLat -= qX;
		qSen += qX;
		ret = 1;
	}
	as_tdb = (hSenIn + qSen/absAmf) / (PsyShDryAir + as_w*PsyShWtrVpr);
	if (as_w > PsyWmin)
	{	if (psyLimitRh( rhMax, as_tdb, as_w))
		{	qSen = absAmf * (as_tdb * (PsyShDryAir + as_w*PsyShWtrVpr) - hSenIn);
			qLat = absAmf * (as_w - wIn) * PsyHCondWtr;
			ret = 1;
		}
	}
#if defined( _DEBUG)
	float hOut = as_Enthalpy();
	double qTot = qSen + qLat;
	if (frDiff( absAmf*(hOut-hIn), qTot) > .0001)
	{	if (!Top.isWarmup)
			warn( "as_AddQSenLat: enthalpy mismatch, ret=%d", ret);
	}
#endif
	return ret;
}		// AIRSTATE::as_AddQSenLat
//------------------------------------------------------------------
AIRSTATE& AIRSTATE::as_MixF(			// fractional mix
	float f,				// mass fraction of source (0 - 1)
	double tdb,				// source dry-bulb temp, F
	double w)				// source humidity ratio
// returns updated air state
{
	as_tdb = f*tdb + (1.f - f)*as_tdb;
	as_w   = f*w   + (1.f - f)*as_w;
	return *this;
}		// AIRSTATE::as_MixF
//-----------------------------------------------------------------------------
int AIRSTATE::as_HX(			// heat exchange (re e.g. HERV)
	const AIRSTATE& asSup,
	float amfSup,
	const AIRSTATE& asExh,
	float amfExh,
	float effS,		// sensible effectiveness
	float effL)		// latent effectiveness
{
	double amfMin = min( amfSup, amfExh);

	if (amfMin < .00001)
	{	*this = asSup;
		return 0;
	}

	double effX = amfMin / amfSup;
	as_tdb = as_HXEff( asSup.as_tdb, asExh.as_tdb, effX*effS);
	as_w = as_HXEff( asSup.as_w, asExh.as_w, effX*effL);
	return 1;
}		// AIRSTATE::as_HX
//===============================================================================

///////////////////////////////////////////////////////////////////////////////
// class AIRFLOW -- mass flow, dry-bulb temp, humidity ratio
//                  many mbrs inline
///////////////////////////////////////////////////////////////////////////////
void AIRFLOW::af_SetAirState(
	const AIRSTATE& as, // new state
	float& qSen, // Added (+) or removed (-) sensible heat required to get to new state
	float& qLat) // Added (+) or removed (-) latent heat required to get to new state 
{
	qSen = af_AmfCp()*(as.as_tdb - as_tdb);
	qLat = af_amf*(as.as_w - as_w)*PsyHCondWtr;
	as_Set( as.as_tdb, as.as_w);
}
//-----------------------------------------------------------------------------
void AIRFLOW::af_Mix(		// mix AIRFLOWs
	const AIRFLOW& af)	// flow to be mixed with *this
// returns *this with total flow and state
{
	if (af_amf == 0.)
		af_Copy( af);
	else if (af.af_amf != 0.)
	{	if (af_amf*af.af_amf < 0.)
			warn( "AIRFLOW::af_Mix", "flow reversal");
		else
		{	double amfX = af_amf + af.af_amf;		// always > 0
			as_tdb = (af_amf*as_tdb + af.af_amf*af.as_tdb) / amfX;
			as_w   = (af_amf*as_w + af.af_amf*af.as_w) / amfX;
			af_amf = amfX;
		}
	}
}	// AIRFLOW::af_Mix
//-----------------------------------------------------------------------------
void AIRFLOW::af_AccumDry(		// add mass flow
	double amf,		// added air mass flow rate, (lbm dry air)/hr
					//   all accum'd flows should have same sign
	double tdb,		// added air dry-bulb temp, F
	double w)		// added air humidity ratio, (lbm water) / (lbm dry air)
// see also af_AccumMoist()
{
	if (af_amf == 0.)			// if current flow is 0
		af_Init( amf, tdb, w);	// handle 0+0 and 0+X cases
	else if (amf != 0.)
	{	if (amf*af_amf < 0.)
			warn( "AIRFLOW::af_AccumDry -- flow reversal");
		else
		{	double amfX = af_amf + amf;		// always != 0
			as_tdb = (af_amf*as_tdb + amf*tdb) / amfX;
			as_w   = (af_amf*as_w + amf*w) / amfX;
			af_amf = amfX;
		}
	}
#if defined( _DEBUG)
	if (_isnan( af_amf))
		printf( "af_AccumDry: NAN amf!\n");
#endif
}		// AIRFLOW::af_AccumDry
//=============================================================================


/////////////////////////////////////////////////////////////////////////////////
// INTERZONE TRANSFER
/////////////////////////////////////////////////////////////////////////////////
double ZNR::zn_Rho0() const		// zone air density
// returns moist air density lb/ft3 at nominal z=0 for current zone conditions
//   Note: not *dry* air
{
	double rho = psyDenMoistAir( tz, wz, LbSfToInHg( zn_pz0));
	if (isnan( rho) || rho < .0001)
	{	orMsg(ERRRT, "invalid moist air density (<=0 or nan)");
		rho = 0.01;		// set to small but physically possible value
	}
	return rho;
}	// ZNR::zn_Rho0
//------------------------------------------------------------------------------
void ZNR::zn_AccumAirFlow(		// accumulate zone bal values due to air flow
	int iV,				// mode
	double mDot,		// air mass flow rate into zone, lb/sec
						//   Note: do not call for outward flow
	const AIRSTATE& as,	// state of air being added
	double td /*=0.*/)	// additional temp rise (added to as_tdb), F
						//   e.g. fan heat
{
	if (mDot <= 0.f)
		return;		// insurance

#if defined( AIRNET_COMPARE)
x	fix AIRNET_COMPARE if needed
x	double mCp = 3600. * .244 * mDot;		// heat gain rate, Btuh/F
#elif 0
x	double mCp = 3600. * Top.tp_airSH * mDot;	// heat capacity flow rate, Btuh/F
#endif

	zn_airNetI[ iV].af_AccumMoist( mDot*3600., as.as_tdb + td, as.as_w);

}		// ZNR::zn_AccumAirFlow
//==============================================================================

// #undef AIRNET_COMPARE		// #define in cndefns.h to use methods identical to those of AirNet.bas
								//   allows detailed comparison of results
#if defined( AIRNET_COMPARE)
const double G0STDX = 32.2;		// Niles accel. of gravity value
static double DensityNiles( double t)
{
	// exact airnet.bas code (from niles) for comparison testing
	const double patm = 1.;
	double rho = patm*14.7*144./(53.34*(t+460.));
	return rho;
}		// DensityNiles
#else
const double G0STDX = g0Std;	// CSE accel. of gravity, ft/sec2
#endif
//===============================================================================
// class ANDAT -- airnet data
void ANDAT::ad_ClearResults()
{
	ad_delP = ad_dmdp = 0.;
	ad_mdotP = ad_mdotB = ad_mdotX = 0.;
	// ad_xDelpF = ad_xMbm = 0.;	do not clear (values retained throughout hour)
	ad_tdFan = ad_pFan = 0.;

}	// ANDAT::ad_ClearResults
//-----------------------------------------------------------------------------
void ANDAT::ad_Setup(			// one-time setup at run beg
	IZXRAT* pIZXRAT)		// parent pointer
{
	ad_pIZXRAT = pIZXRAT;
}		// ANDAT::ad_Setup
//-------------------------------------------------------------------------------
void ANDAT::ad_SetupPresDep()	// set members for pressure dependent flow calcs
// call when e.g. vent area is changed
{
	if (ad_Ae <= 0.)
	{	// this vent is off
		ad_AeLin = 0.;
		ad_delP = 0.;
		ad_mdotP = 0.;
		ad_dmdp = 0.;
		ad_mdotB = 0.;
		ad_mdotX = 0.;
	}
	else
		ad_AeLin = ad_Ae * pow( IZXRAT::delPLinear, ad_pIZXRAT->iz_exp-1.);
}		// ANDAT::ad_SetupPresDep
//-------------------------------------------------------------------------------
RC ANDAT::ad_MassFlow(
	double delP)	// pressure diff (+=into zone 1)
{
	ad_delP = delP;
	double sp = xsign( ad_delP);	// 1 if pres2>pres1; 0 if same; -1 if pres2<pres1
	double rhoIn2 = ad_pIZXRAT->iz_rho1*(1.-sp)+ad_pIZXRAT->iz_rho2*(1.+sp);	// 2 * (density into vent)
	// use average if pres diff = 0
#if defined( _DEBUG)
	if (rhoIn2 < 0.)
	{	printf( "Vent '%s': Neg rhoIn2\n", ad_pIZXRAT->name);
		rhoIn2 = 0.07;
	}
#endif
	double srDen = sqrt( G0STDX * rhoIn2);		// G0STDX re AIRNET_COMPARE

	// compute flow and deriv thereof
	//   mdotP = lb/sec, + = into z1
	//   dmdp = d( mdot) / d( delP)
	if (fabs( ad_delP) < IZXRAT::delPLinear)
	{	// delP near 0, use linear form to avoid unbounded dmdp
		// AeLin is effective area to avoid discontinuity at delPLinear
		//    see ad_BegHour
		ad_mdotP = ad_AeLin * srDen * ad_delP;
		ad_dmdp  = ad_AeLin * srDen;
	}
	else
	{	// delP not "small", use non-linear form
#if 1	// recode to use single pow() call, 6-13-2014
		double nx = ad_pIZXRAT->iz_exp;
		double pnx = ad_Ae * srDen * pow( fabs( ad_delP), nx);
		ad_mdotP = sp * pnx;
		// ad_dmdp  = ad_Ae * srDen * nx * pow( fabs( ad_delP), nx-1.);
		//   use division to get pow( p, nx-1) (avoids 2nd pow() call)
		ad_dmdp  = nx * pnx / fabs( ad_delP);	// ad_delP != 0 always
#if 0 && defined( _DEBUG)
x		double mdotP = ad_Ae * srDen * sp * pow( fabs( ad_delP), nx);
x		double dmdp  = ad_Ae * srDen * nx * pow( fabs( ad_delP), nx-1.);
x		if (frDiff( mdotP, ad_mdotP) > 0.000001
x		 || frDiff( dmdp, ad_dmdp)   > 0.000001)
x			printf( "mdotP mismatch!");
#endif
#else
x		double nx = ad_pIZXRAT->iz_exp;
x		ad_mdotP = ad_Ae * srDen * sp * pow( fabs( ad_delP), nx);
x		ad_dmdp  = ad_Ae * srDen * nx * pow( fabs( ad_delP), nx-1.);
#endif
	}
	return RCOK;
}		// ANDAT::ad_MassFlow
//-------------------------------------------------------------------------------
#if 0	// unused, not maintained, 5-2013
x void IZXRAT::iz_SetFromMCp(
x	double mCp,				// heat cap flow, Btuh/F
x	double mCpT /*=0.*/)	// mCp*T, Btuh (used only if mCp > 0)
x {
x	double t2 = mCp > 0. ? mCpT / mCp : 0.;
x	iz_air2.as_Set( t2, 0.);		// TODO: w
x	double mdot = mCp / (3600. * Top.tp_airSH);	// convert to lb/sec
x #if 0 && defined( _DEBUG)
x	if (mdot > 0.)
x		printf( "Hit\n");
x #endif
x	iz_ad[ 0].ad_mdotP = mdot;
x	iz_ad[ 1].ad_mdotP = mdot;
x }		// IZXRAT::iz_SetFromMCp
#endif
//-------------------------------------------------------------------------------
void IZXRAT::iz_SetFromAF(		// set from known air mass flow and state
	const AIRFLOW* pAF)
{
	if (iz_IsOAVRelief())
	{	// OAV relief vent -- set free area from flow
		// relief air goes to relief zone (typically attic)
		// airnet determines modified zone pressures
		float reliefVentArea;
		if (pAF->af_amf != 0.)
		{
			static const float OAVGRILLEVELOCITY = 500.f;	// recommended return grille velocity, fpm
															//   (ACCA Manual D recommendation)
			float avf = AMFtoAVF( fabs( pAF->af_amf));
			reliefVentArea = avf / OAVGRILLEVELOCITY;	// required free area, ft2
			iz_GetZn2Conditions();
		}
		else
		{	reliefVentArea = 0.f;
			iz_ClearZn2Conditions();
		}
		if (reliefVentArea != iz_a1)
		{	// area has changed
			iz_a1 = reliefVentArea;
			iz_SetupPresDep();		// update coeffs for pressure-dependent calcs
		}
	}
	else
	{	if (pAF->af_amf > 0.)	// if flow into zone
			pAF->af_GetAirState( iz_air2);		// source air state
		else
			iz_air2.as_Set( 0., 0.);	// flow is out of zone
		iz_ad[ 0].ad_mdotP = iz_ad[ 1].ad_mdotP = pAF->af_amf / 3600.;	// convert to lbm/sec
	}
}		// IZXRAT::iz_SetFromAF
//-------------------------------------------------------------------------------
void ANDAT::ad_SetFromFixedAVF(			// set mbrs re fixed vol flow rate
	double avf)	// fixed volume flow rate, cfm
{
	// do calcs even if no vf, ensures all mbrs consistent
	// (also, fan could use power at 0 flow?)
	// if (fabs( avf) < .01)
	//	  avf = 0.	maybe?
	BOOL in = avf > 0.;		// if flow into zone 1, inlet conditions are zone 2
	float rho = ad_pIZXRAT->iz_Rho( in);	// entering air density
	ad_mdotP = ad_mdotX = avf * rho / 60.;	// convert to lb/sec
#if defined( DEBUGDUMP)
	if (DbDo( dbdAIRNET))
		DbPrintf( "Fixed AVF '%s': avf=%.2f  rho=%.5f  mdotP=%.5f\n",
			ad_pIZXRAT->name, avf, rho, ad_mdotP);
#endif
	if (ad_pIZXRAT->iz_IsFan())
	{	double c = fabs( ad_mdotP) * Top.tp_airSH * 3600.;		// fan flow in heat cap units (Btuh/F)
		ad_tdFan = ad_pIZXRAT->iz_fan.fn_pute( c, ad_pIZXRAT->iz_T( in));
		ad_pFan  = ad_pIZXRAT->iz_fan.p;		// fan power (for meter)
	}
	if (ad_pIZXRAT->iz_IsDOAS())
	{
		DOAS* pDOAS = doasR.GetAt(ad_pIZXRAT->iz_doas);
		FAN* fan = ad_pIZXRAT->iz_vfMin > 0 ? &pDOAS->oa_supFan : &pDOAS->oa_exhFan;
		ad_tdFan = fan->dT;
	}
}	// ANDAT::ad_SetFromFixedAVF
//-------------------------------------------------------------------------------
RC IZXRAT::iz_CalcHERV()			// set mbrs re HERV model
// call subhourly
// sets iz_ad[ 0] and iz_ad[ 1] + fan members
{
//  TODO: dry-air vs. moist air, 5-17-2013

	RC rc = RCOK;

	// check inputs (subhourly variability)
	// report run time error with date/time (queries abort or continue)
	if (iz_vfMin < 0.f)
		rc |= rer("IZXFER '%s': invalid HERV izVfMin=%0.f (must be >= 0)",
				name, iz_vfMin);
	if (iz_EATR >= 1.f)
		rc |= rer("IZXFER '%s': invalid HERV izEATR=%0.2f (must be < 1)",
				name, iz_EATR);
	if (iz_ASRE < iz_SRE)
		rc |= rer("IZXFER '%s': invalid HERV izASRE (%0.2f) must be >= izSRE (%0.2f)",
			name, iz_ASRE,iz_SRE);

	// current operating supply AVF (into z1)
	//  always >= 0 (checks just above)
	double avfGross = rc == RCOK
				? iz_vfMin / (1.f - iz_EATR)		// can be 0
				: 0.;

	ANDAT& ad = iz_ad[ 0];

	ad.ad_mdotP = avfGross * iz_rho2 / 60.;	// mass flow into zone, lb/sec (use prior-step density)

	// exhaust air source
	//  may be same as z1
	ZNR* zpx = ZrB.GetAt( iz_zi2);
	AIRSTATE asX;
	zpx->zn_GetAirStateLs( asX);
	float rhoX = zpx->zn_rho0ls;
	ad.ad_mdotX = avfGross * iz_vfExhRat * rhoX / 60.;

#if 0 && defined( DEBUGDUMP)
	if (DbDo( dbdAIRNET))
		DbPrintf( "HERV '%s' in: avf=%.2f  rho=%.5f  mdotP=%.5f\n   out: avf=%.2f  rho=%.5f  mdotP=%.5f\n",
			name, avfGross, iz_rho2, ad.ad_mdotP, avfGross*iz_vfExhRat, rhoX, ad.ad_mdotX);
#endif
	
	// iz_air2 / iz_rho2 = air state into z1
	// Note: calc HX with moist air mass flow rates
	//       some sources say dry AMF, not fully understood
	//       minor difference in any case, 5-16-2013
	// TODO: condensation and defrost, 5-16-2013
	AIRSTATE asIn( Top.tDbOSh, Top.wOSh);
	iz_air2.as_HX( asIn, ad.ad_mdotP,	// supply air (=ambient)
				   asX,  ad.ad_mdotX,	// exhaust air (=z1 or some other zone)
				   iz_SRE > 0.f ? iz_SRE : iz_ASEF, 
				   iz_LEF);
	iz_rho2 = iz_air2.as_RhoMoist();

	// HERV always has fan
	double c = ad.ad_mdotP * Top.tp_airSH * 3600.;		// fan flow in heat cap units (Btuh/F)
	float tdFan = iz_fan.fn_pute(c, iz_air2.as_tdb);
	ad.ad_pFan = iz_fan.p;		// fan power (for meter), Btuh
	if (iz_SRE > 0.f)
 		ad.ad_tdFan = (iz_ASRE - iz_SRE) * 38.f;
	else
		ad.ad_tdFan = iz_RVFanHeatF * tdFan;

	// HERV non-vent / vent are the same
	iz_ad[ 1].ad_DupHERV( ad);

	return rc;
}	// IZXRAT::iz_CalcHERV
//-------------------------------------------------------------------------------
void ANDAT::ad_DupHERV( const ANDAT& ad)		// dup HERV values
// WHY: HERV does not have non-vent / vent modes
//      Duplicate values so both modes get same result
{	ad_mdotP = ad.ad_mdotP;
	ad_mdotX = ad.ad_mdotX;
	ad_tdFan = ad.ad_tdFan;
	ad_pFan = ad.ad_pFan;
}	// ANDAT::ad_DupHERV
//===============================================================================
RC HEATEXCHANGER::hx_setup(
	record *r,			// Pointer to parent record containing HEATEXCHANGER SUBSTRUCT
	SI parentFieldNum)	// Field number of parent record for HEATEXCHANGER SUBSTRUCT (e.g., AH_OAHX)
{
#define HX(m) (parentFieldNum + HEATEXCHANGER_##m)

	RC rc = RCOK;
	// heat recovery
	if (r->IsSet(HX(SENEFFH)))
	{
		//const char* when = strtprintf( "when %s is set", mbrIdTx(parentFieldNum + HEATEXCHANGER_SENEFFH));
	}
	else
	{
		const char* when = strtprintf( "when %s is not set", r->mbrIdTx(HX(SENEFFH)));
		rc |= r->ignoreN(when, HX(VFDS), HX(F2), HX(SENEFFH+1), HX(LATEFFH),
			HX(LATEFFH+1), HX(SENEFFC), HX(SENEFFC+1), HX(LATEFFC),
			HX(LATEFFC+1), HX(BYPASS), HX(AUXPWR), 0);
	}

	hx_pAuxMtr = MtrB.GetAtSafe( hx_auxMtri);

	return rc;
#undef HX
}
//===============================================================================
void HEATEXCHANGER::hx_calc ()
{
	hx_bypassAF = hx_supInAF;
	hx_bypassAF.af_amf *= hx_bypassFrac;
	hx_hxInAF = hx_supInAF;
	hx_hxInAF.af_amf *= (1.f - hx_bypassFrac);

	// Calculate effectivenesses
	float VfAvg = (hx_hxInAF.af_amf + hx_exhInAF.af_amf)*0.5/( Top.tp_rhoMoistOSh * 60.f);

	float ff = VfAvg / hx_VfDs;

	float weight = (ff - hx_f2)/(1.0 - hx_f2);

	if (hx_hxInAF.as_tdb < hx_exhInAF.as_tdb) {
		// HX is heating
		hx_sensEff =  hx_senEffH[1] + (hx_senEffH[0] - hx_senEffH[1])*weight;
		hx_latEff =  hx_latEffH[1] + (hx_latEffH[0] - hx_latEffH[1])*weight;
	}
	else
	{
		// HX is cooling
		hx_sensEff =  hx_senEffC[1] + (hx_senEffC[0] - hx_senEffC[1])*weight;
		hx_latEff =  hx_latEffC[1] + (hx_latEffC[0] - hx_latEffC[1])*weight;
	}

	hx_hxOutAF.af_HX(hx_hxInAF, hx_exhInAF, hx_sensEff, hx_latEff);

	hx_supOutAF = hx_hxOutAF;

	hx_supOutAF.af_Mix(hx_bypassAF);

}
//===============================================================================
double HEATEXCHANGER::hx_calcBypass(
	float bypassFraction)	// bypass fraction used to calculate HX performance
//    calcs HX supply outlet temperature using bypass fraction
//    used to search for bypass fraction consistent with desired supply outlet temperature
//    use regula-falsi search method since secant fails at low bypass fractions
// returns outlet temperature, F
{
	hx_bypassFrac = bypassFraction;
	hx_calc();
	return hx_supOutAF.as_tdb;
}

//===============================================================================
void HEATEXCHANGER::hx_begSubhr(
	AIRFLOW supInletAF, // Supply inlet AIRFLOW (typically at outdoor air conditions)
	AIRFLOW exhInletAF,	// Exhaust inlet AIRFLOW (typically at return/exhaust air conditions + fan heat)
	DBL tWant)			// Desired supply (hx + bypass) air drybulb temperature, F
	// returns AIRFLOW of air after mixing bypass air
{ 
	hx_supInAF = supInletAF;
	hx_exhInAF = exhInletAF;
	hx_tSet = tWant;

	// Start with maximum possible outlet AIRFLOW conditions (assumes no bypass)
	hx_calcBypass(0.f);

	if (hx_bypass==C_NOYESCH_NO || hx_supInAF.as_tdb == hx_supOutAF.as_tdb)
	{	// if bypass is disabled or the heat exchanger has no sensible effect
		return;
	}
	// Initial guess of bypass fraction
	// < 1.0 when hx helps get closer to twant
	// < 0 when hx helps, but can't do any more
	// > 1.0 when outooor air is closer to twant than possible hx outlet
	hx_bypassFrac = (hx_tSet - hx_supOutAF.as_tdb)/(hx_supInAF.as_tdb - hx_supOutAF.as_tdb);

	hx_bypassFrac = bracket(0.f, hx_bypassFrac, 1.f);

	if (hx_bypassFrac == 0.f)
	{	// If bypass doesn't help achieve twant, return max possible heat recovery
		hx_calcBypass(hx_bypassFrac);
	}
	else if (hx_bypassFrac == 1.f)
	{	// If hx doesn't help achieve twant, return inlet conditions
		hx_calcBypass(hx_bypassFrac);
	}
	else
	{	// Calculate bypass fraction needed to acheive twant
		// Iterate since effectiveness depends on flow rate
		double x1{ hx_bypassFrac };
		int rc = regula( [](void* pO, double& bf) { return ((HEATEXCHANGER*)pO)->hx_calcBypass(bf); },
						this, hx_tSet, .001,
						x1,			// x1
						0.,			// xMin
						1.);		// xMax
	}
}
//===============================================================================
RC DOAS::oa_CkfDOAS()	// input checks
{
	RC rc = RCOK;

	rc |= oa_hx.hx_setup(this, DOAS_HX);

	// heating coil
	if (IsSet(DOAS_SUPTH)) {
		//const char* when = strtprintf( "when %s is set", mbrIdTx(DOAS_SUPTH));
	}
	else
	{
		const char* when = strtprintf( "when %s is not set", mbrIdTx(DOAS_SUPTH));
		rc |= ignoreN(when, DOAS_EIRH, 0);
	}

	// cooling coil
	if (IsSet(DOAS_SUPTC)) {
		// const char* when = strtprintf( "when %s is set", mbrIdTx(DOAS_SUPTC));
	}
	else
	{
		const char* when = strtprintf( "when %s is not set", mbrIdTx(DOAS_SUPTC));
		rc |= ignoreN(when, DOAS_EIRC, DOAS_SHRTARGET, 0);
	}


	return rc;
}
//-----------------------------------------------------------------------------
RC DOAS::oa_Setup(DOAS* iRat)
{
	RC rc = RCOK;
	*this = *iRat;		// copy record, incl name, excl internal front overhead.

	rc |= oa_CkfDOAS();		// check run record

	// Setup meter pointers
	oa_pCoilCMtr = MtrB.GetAtSafe( oa_coilCMtri);
	oa_pCoilHMtr = MtrB.GetAtSafe( oa_coilHMtri);
	oa_pLoadMtr = LdMtrR.GetAtSafe( oa_loadMtri);

	// Calculate total design flow rates from child IXFERS
	float supFanVfDemand = 0.f;
	float exhFanVfDemand = 0.f;

	IZXRAT* izx;
	RLUP( IzxR, izx)
	{	
		if (izx->iz_IsDOAS() && izx->iz_doas == ss)
		{
			if (izx->iz_vfMin > 0.f)
			{
				supFanVfDemand += izx->iz_vfMin;
			}
			else
			{
				exhFanVfDemand -= izx->iz_vfMin;
			}
		}	
	}

	// If fan design flows are not set, set them from IZXFER flows
	//   Note: This may not be reliable if iz_vfMin is an expression
	// Warn if vfDs is not large enough to meet IZXFER demand
	bool supFanVfDsIsSet = IsSet(DOAS_SUPFAN + FAN_VFDS);
	bool exhFanVfDsIsSet = IsSet(DOAS_EXHFAN + FAN_VFDS);

	if (!supFanVfDsIsSet)
	{
		oa_supFan.vfDs = supFanVfDemand;
	}
	else
	{
		float supFlowRatio = oa_supFan.vfDs / supFanVfDemand;
		if(supFlowRatio < 0.99f)
		{
			oWarn( "Sum of IZXFER supply flows referencing DOAS is larger than vfDs.\n"
				"    IZXFER supply flows will be reduced proportionately"
			);

			// Adjust IZXFER flows
			IZXRAT* izx;
			RLUP( IzxR, izx)
			{	
				if (izx->iz_IsDOAS() && izx->iz_doas == ss)
				{
					if (izx->iz_vfMin > 0.f)
					{
						izx->iz_vfMin *= supFlowRatio;
					}
				}	
			}
		}
	}

	if (!exhFanVfDsIsSet)
	{
		oa_exhFan.vfDs = exhFanVfDemand;
	}
	else
	{
		float exhFlowRatio = oa_exhFan.vfDs / exhFanVfDemand;
		if(exhFlowRatio < 0.99f)
		{
			oWarn( "Sum of IZXFER exhaust flows referencing DOAS is larger than vfDs\n"
				"    IZXFER exhaust flows will be reduced proportionately"
			);

			// Adjust IZXFER flows
			IZXRAT* izx;
			RLUP( IzxR, izx)
			{	
				if (izx->iz_IsDOAS() && izx->iz_doas == ss)
				{
					if (izx->iz_vfMin < 0.f)
					{
						izx->iz_vfMin *= exhFlowRatio;
					}
				}	
			}
		}
	}

	// Setup fan records
	rc |= oa_supFan.fn_setup( C_FANAPPCH_DSFAN, this, DOAS_SUPFAN, oa_supFan.vfDs, NULL);	// checks fan input, does defaults
	rc |= oa_exhFan.fn_setup( C_FANAPPCH_DEFAN, this, DOAS_EXHFAN, oa_exhFan.vfDs, NULL);	// checks fan input, does defaults

	// Check inputs in absence of fan(s)
	if (oa_supFan.fanTy == C_FANTYCH_NONE)
	{
#define ZFAN(m) (DOAS_SUPFAN + FAN_##m)
#define HX(m) (DOAS_HX + HEATEXCHANGER_##m)
		const char* when = "when there is no supply fan";
		rc |= ignoreN( when, ZFAN(VFDS), ZFAN(PRESS), ZFAN(EFF), ZFAN(SHAFTPWR),
				 ZFAN(ELECPWR), ZFAN(MTRI), ZFAN(ENDUSE), ZFAN(CURVEPY+PYCUBIC_K),
				 HX(SENEFFH), HX(VFDS), HX(F2), HX(SENEFFH+1), HX(LATEFFH),
				 HX(LATEFFH+1), HX(SENEFFC), HX(SENEFFC+1), HX(LATEFFC),
				 HX(LATEFFC+1), HX(BYPASS), HX(AUXPWR), DOAS_SUPTH, DOAS_SUPTC, 
				 DOAS_EIRH, DOAS_EIRC, DOAS_SHRTARGET, 0);
#undef ZFAN
	}
	if (oa_exhFan.fanTy == C_FANTYCH_NONE)
	{
#define ZFAN(m) (DOAS_EXHFAN + FAN_##m)
		const char* when = "when there is no exhaust fan";
		rc |= ignoreN( when, ZFAN(VFDS), ZFAN(PRESS), ZFAN(EFF), ZFAN(SHAFTPWR),
				 ZFAN(ELECPWR), ZFAN(MTRI), ZFAN(ENDUSE), ZFAN(CURVEPY+PYCUBIC_K),
				 HX(SENEFFH), HX(VFDS), HX(F2), HX(SENEFFH+1), HX(LATEFFH),
				 HX(LATEFFH+1), HX(SENEFFC), HX(SENEFFC+1), HX(LATEFFC),
				 HX(LATEFFC+1), HX(BYPASS), HX(AUXPWR), 0);
#undef ZFAN
	}

	// Setup HX
	if (IsSet(HX(SENEFFH)))
	{
		if (!IsSet(HX(VFDS)))
		{
			oa_hx.hx_VfDs = (oa_supFan.vfDs + oa_exhFan.vfDs)*0.5f;
		}
	}
#undef HX

	return rc;
}
//-----------------------------------------------------------------------------
RC DOAS::oa_BegSubhr()
{
	RC rc = RCOK;
	// Get total air flow rates and calculate mixed exhaust air state
	oa_supAF.af_amf = 0.;
	oa_exhAF.af_amf = 0.;
	float supVf = 0.f;
	float exhVf = 0.f;
	IZXRAT* izx;
	RLUP( IzxR, izx)
	{	
		if (izx->iz_IsDOAS() && izx->iz_doas == ss)
		{
			if (izx->iz_vfMin > 0.f)
			{
				supVf += izx->iz_vfMin;
			}
			else
			{
				ZNR* zpx = ZrB.GetAt( izx->iz_zi1);
				AIRSTATE zoneAir;
				zpx->zn_GetAirStateLs( zoneAir);
				float rhoX = zpx->zn_rho0ls;
				exhVf -= izx->iz_vfMin;
				AIRFLOW zoneExhAF(-izx->iz_vfMin * rhoX * 60.f, zoneAir);
				oa_exhAF.af_Mix(zoneExhAF);
			}
		}	
	}
	oa_supAF.af_amf = supVf * Top.tp_rhoMoistOSh * 60.f;

	// Calculate supply air conditions

	// Inlet (OA) conditions
	AIRFLOW inletAF(oa_supAF.af_amf, Top.tDbOSh, Top.wOSh);

	// Exhuast air conditions
	double exhC = oa_exhAF.af_AmfCp();		// fan flow in heat cap units (Btuh/F)
	oa_exhFan.fn_pute(exhC, oa_exhAF.as_tdb);

	// Fan outlet conditions
	AIRFLOW exhFanOutAF(oa_exhAF);
	exhFanOutAF.as_AddDeltaT(oa_exhFan.dT);

	// Supply air conditions
	double supC = inletAF.af_AmfCp();		// fan flow in heat cap units (Btuh/F)
	float tdFan = oa_supFan.fn_pute(supC, inletAF.as_tdb);

	// Calculate target setpoint and determine if HX should be bypassed
	if (IsSet(DOAS_SUPTH) && IsSet(DOAS_SUPTC))
	{
		if (oa_supTH > oa_supTC)
		{
			rWarn("%s is greater than %s; %s will be set equal to %s",
			mbrIdTx(DOAS_SUPTH), mbrIdTx(DOAS_SUPTC), mbrIdTx(DOAS_SUPTC), mbrIdTx(DOAS_SUPTH));
			oa_supTC = oa_supTH;
		}
	}
	float heatSP{-100.f};
	float coolSP{200.f};
	double tWant{inletAF.as_tdb};
	if (IsSet(DOAS_SUPTH))  // Heating setpoint
	{
		heatSP = oa_supTH - tdFan;
		if (inletAF.as_tdb < heatSP)
		{
			tWant = heatSP;
		}
	}
	if (IsSet(DOAS_SUPTC))  // Cooling setpoint
	{
		coolSP = oa_supTC - tdFan;
		if (inletAF.as_tdb > coolSP)
		{
			tWant = coolSP;
		}
	}
	if (!IsSet(DOAS_SUPTH) && !IsSet(DOAS_SUPTC))
	{	// Force HX to use no bypass and maximize heat exchange
		tWant = exhFanOutAF.as_tdb;
	}

	AIRFLOW hxOutAF(inletAF);
	// Calculate HX (if it exists)
	if (IsSet(DOAS_HX + HEATEXCHANGER_SENEFFH))
	{
		oa_hx.hx_begSubhr(inletAF, exhFanOutAF, tWant);
		hxOutAF = oa_hx.hx_supOutAF;
	}

	// tempering
	AIRFLOW tempOutAF(hxOutAF);	

	oa_supQSen = 0.f;
	oa_supQLat = 0.f;

	// Heating Coil (if setpoint is set)
	if (IsSet(DOAS_SUPTH))
	{
		if (hxOutAF.as_tdb < heatSP)  // Entering air below heating setpoint
		{
			// Set air state to meet setpoint
			tempOutAF.af_SetAirState(AIRSTATE(heatSP, hxOutAF.as_w), oa_supQSen, oa_supQLat);
		}
	}

	// Cooling Coil (if setpoint is set)
	if (IsSet(DOAS_SUPTC))
	{
		if (hxOutAF.as_tdb > coolSP)  // Entering air above cooling setpoint
		{
			oa_supQSen = hxOutAF.af_AmfCp()*(coolSP - hxOutAF.as_tdb);
			oa_supQLat = oa_supQSen/oa_SHRtarget - oa_supQSen;
			tempOutAF.af_AddQSenLat(oa_supQSen, oa_supQLat);  // TODO: Preserve Qsen
			oa_SHR = oa_supQSen/(oa_supQSen + oa_supQLat);
		}
	}

	oa_supAF = tempOutAF; 	// This is the flow before the fan because the temperature rise due
	                        // to the fan is applied at the IZXFER level
	return rc;

}
//-----------------------------------------------------------------------------
RC DOAS::oa_EndSubhr()
{
	RC rc = RCOK;
	// Accummulate meters

	// Fan meters
	if (oa_supFan.fn_mtri)
	{
		MtrB.p[ oa_supFan.fn_mtri].H.mtr_Accum(
			oa_supFan.fn_endUse,			// user-specified end use (default = "fan")
			oa_supFan.p * Top.tp_subhrDur);		// electrical energy, Btu (*not* Wh)
	}
	if (oa_exhFan.fn_mtri)
	{
		MtrB.p[ oa_exhFan.fn_mtri].H.mtr_Accum(
			oa_exhFan.fn_endUse,			// user-specified end use (default = "fan")
			oa_exhFan.p * Top.tp_subhrDur);		// electrical energy, Btu (*not* Wh)
	}

	// Coil energy meters
	if (oa_pCoilCMtr && oa_EIRC > 0.f)  // Cooling Coil
	{
		if (oa_supQSen < 0.f)
		{
			oa_pCoilCMtr->H.clg -= (oa_supQSen*oa_EIRC*Top.tp_subhrDur);
		}
		if (oa_supQLat < 0.f)
		{
			oa_pCoilCMtr->H.clg -= (oa_supQLat*oa_EIRC*Top.tp_subhrDur);
		}
	}
	if (oa_pCoilHMtr && oa_EIRH > 0.f)  // Heating Coil
	{
		if (oa_supQSen > 0.f)
		{
			oa_pCoilHMtr->H.htg += (oa_supQSen*oa_EIRH*Top.tp_subhrDur);
		}
		if (oa_supQLat > 0.f)
		{
			oa_pCoilHMtr->H.htg += (oa_supQLat*oa_EIRH*Top.tp_subhrDur);
		}
	}

	// HX meter
	if (oa_hx.hx_pAuxMtr)
	{
		oa_hx.hx_pAuxMtr->H.aux += oa_hx.hx_auxPwr * Top.tp_subhrDur * BtuperWh;
	}

	// Load meter
	if (oa_pLoadMtr)
	{
		if (oa_supQSen > 0.f)
		{
			oa_pLoadMtr->S.qHtg += oa_supQSen*Top.tp_subhrDur;
		}
		else
		{
			oa_pLoadMtr->S.qClg += oa_supQSen*Top.tp_subhrDur;
		}
		if (oa_supQLat > 0.f)
		{
			oa_pLoadMtr->S.qHtg += oa_supQLat*Top.tp_subhrDur;
		}
		else
		{
			oa_pLoadMtr->S.qClg += oa_supQLat*Top.tp_subhrDur;
		}

	}

	return rc;
}
//===============================================================================
/*static*/ const double IZXRAT::delPLinear = .000001;	// pres dif below which eqns made linear, lbf/sf
//-----------------------------------------------------------------------------
IZXRAT::IZXRAT( basAnc *b, TI i, SI noZ/*=0*/)		// c'tor
	: record( b, i, noZ)
{	FixUp();
}		// IZXRAT::IZXRAT
//-----------------------------------------------------------------------------
/*virtual*/ void IZXRAT::FixUp()
{	iz_ad[ 0].ad_Setup( this);		// relink sub objects
	iz_ad[ 1].ad_Setup( this);
}	// IZXRAT::FixUp
//-----------------------------------------------------------------------------
/*virtual*/ RC IZXRAT::Validate(
	int options/*=0*/)		// options bits
{
	RC rc = record::Validate( options);
	if (rc == RCOK)
	{	if (iz_ad[ 0].ad_pIZXRAT != this
		 || iz_ad[ 1].ad_pIZXRAT != this)
			rc |= errCrit( WRN, "IZXRAT::Validate: Bad ANDAT linkage");
	}
	return rc;
}		// IZXRAT::Validate
//-----------------------------------------------------------------------------
#define ZFAN(m) (IZXRAT_FAN + FAN_##m)		// re IZXFER fan check and setup
RC IZXRAT::iz_CkfIZXFER()	// input checks
// called from izStarCkf (at input) *and* from iz_Setup() just below
{
	RC rc = RCOK;

	// type as text (for msgs)
	const char* izTy = getChoiTx( IZXRAT_NVCNTRL, 1);
	const char* when = strtprintf( "when izNVTYPE=%s", izTy);

	// other zone
	if (iz_IsIZ())
		rc |= require( when, IZXRAT_ZI2);

	// UA coupling only
	if (iz_nvcntrl == C_IZNVTYCH_NONE)
	{	rc |= disallowN( (char *)MH_S0474, 			// "when izNVType is NONE or omitted"
							  IZXRAT_A1, IZXRAT_A2, 		// error if user gave any of these fields
							  IZXRAT_HZ, IZXRAT_CD, 0);
		// clear unused defaults
		iz_cd = 0.f;
		iz_exp = 0.f;
	}

	if (!iz_CanHaveAFCat())
		ignore( when, IZXRAT_AFCATI);

	// horizontal opening
	//   stairangle defaults
	//   L1 and L2 required else disallowed
	if (iz_nvcntrl == C_IZNVTYCH_ANHORIZ)
		rc |= requireN( when, IZXRAT_L1, IZXRAT_L2, 0);
	else
	{	rc |= disallowN( when, IZXRAT_L1, IZXRAT_L2, IZXRAT_STAIRANGLE, 0);
		iz_stairAngle = 0.f;
	}

	if (!iz_HasFan())
	{	rc |= disallowN( when, ZFAN(VFDS), ZFAN(PRESS), ZFAN(EFF), ZFAN(SHAFTPWR),
				 ZFAN(ELECPWR), ZFAN(MTRI), ZFAN(ENDUSE), ZFAN(CURVEPY+PYCUBIC_K), 0);
		iz_fan.fn_setup2( -1);	// clear all fan mbrs
	}

	return rc;
}	// IZXRAT::iz_CkfIZXFER
//-----------------------------------------------------------------------------
RC IZXRAT::iz_Setup(			// set up run record
	IZXRAT* izie)		// input record
// caller must allocate this (in IzxR)
{
	RC rc = RCOK;

	*this = *izie;		// copy record, incl name, excl internal front overhead.

	rc |= iz_CkfIZXFER();		// check run record
								//   insurance: not known to be needed

	// TODO re input
	//   z1 != z2
	//   no hx in airnet?
	//   at least one zone connected to outside
	//   no isolated zones???

	// type as text (for msgs)
	const char* izTy = getChoiTx( IZXRAT_NVCNTRL, 1);

	// zone linkage
	//  izZn1 is RQD, missing or bad izZn1 detected earlier and does not get here
	//  insurance: recheck here
	//     if bad, enter in IzxR anyway; message stops run.
	ZNR* zp1;
	ZNR* zp2 = NULL;
	rc |= ckRefPt( &ZrB, iz_zi1, "izZn1", NULL, (record **)&zp1);
	if (iz_IsIZ() || (iz_IsHERV() && IsSet( IZXRAT_ZI2)))
		// if interzone type, izZn2 must be valid
		rc |= ckRefPt( &ZrB, iz_zi2, "izZn2", NULL, (record **)&zp2);

	if (iz_IsHERV())
	{	if (!IsSet( IZXRAT_ZI2))
		{	iz_zi2 = iz_zi1;
			zp2 = zp1;
		}
		iz_rho2 = .075f;	// HERV model uses prior-step iz_rho2
							//  init to plausible nz value
		rc |= disallowN( "when izNVType is AirNetHERV", IZXRAT_VFMAX, 0);
	}
	else
		rc |= disallowN( "when izNVType is not AirNetHERV",
				IZXRAT_ASEF, IZXRAT_LEF, IZXRAT_VFEXHRAT, IZXRAT_EATR,
				IZXRAT_RVFANHEATF, 0);

	if (!zp2)
		iz_zi2 = -1;	// insurance

	// say both ZI1 and ZI2 are known (allows probing)
	fStat( IZXRAT_ZI1) |= FsSET | FsVAL;
	fStat( IZXRAT_ZI2) |= FsSET | FsVAL;

	if (!iz_IsAirNet())
	{
		switch (iz_nvcntrl)
		{
		case C_IZNVTYCH_NONE:
		case C_IZNVTYCH_ONEWAY:
			break;

		case C_IZNVTYCH_TWOWAY:
			rc |= require((char*)MH_S0475, IZXRAT_HZ); 	// specific message if izHD omitted
														//   (no ventilation is modelled with no height difference)"
			rc |= requireN((char*)MH_S0476,			// "when izNVType is TWOWAY"
									IZXRAT_A1, IZXRAT_A2,  // error if user OMITTED any of
									IZXRAT_HZ, 0);		// ... these fields. cncult2.cpp.
			rc |= disallowN((char*)MH_S0476, 			// "when izNVType is TWOWAY"
							IZXRAT_VFMIN, IZXRAT_VFMAX, IZXRAT_DOAS, ZFAN(VFDS), 0); 				// error if user gave any of
			break;

		default:
			ooer(IZXRAT_NVCNTRL, (char*)MH_S0473, izTy, iz_nvcntrl);	// "Internal error: bad izNVType 0x%x"
			break;
		}

		iz_SetupNonAirNet();		// calc izxfer derived values
	}
	else
	{	// AirNet types
		Top.tp_airNetActive = TRUE;		// say there is user input airnet 

		switch (iz_nvcntrl)
		{

		case C_IZNVTYCH_ANHORIZ:
#if 0	// spec change, 7-14-2014
x          WHY : zn_floorZs of adjacent zones do not matter;
x			    Signfificant inputs are
x                  * zone above
x                  * zone below
x                  * absolute opening height
x			if (!rc)
x			{ if (zp2->i.zn_floorZ <= zp1->i.zn_floorZ)
x					rc |= oer("izZn2 '%s' znFloorZ (%0.2f) must be > izZn1 '%s' znFloorZ (%0.2f)",
x						zp2->name, zp2->i.zn_floorZ, zp1->name, zp1->i.zn_floorZ);
x			}
#endif
			// fall thru

		case C_IZNVTYCH_ANIZ:
			rc |= disallowN("when izNVType is not exterior AIRNET",
					IZXRAT_CPR, ZFAN(VFDS), ZFAN(MTRI), ZFAN(ENDUSE), IZXRAT_DOAS, 0);
			// fall thru

		case C_IZNVTYCH_ANEXT:
			rc |= disallowN("when izNVType is not flow or fan AIRNET",
						  IZXRAT_VFMIN, IZXRAT_VFMAX, ZFAN(VFDS), ZFAN(MTRI), ZFAN(ENDUSE), IZXRAT_DOAS, 0);
			break;

		case C_IZNVTYCH_ANIZFAN:
		case C_IZNVTYCH_ANEXTFAN:
		case C_IZNVTYCH_ANHERV:
		{	rc |= disallowN("when izNVType is fan AIRNET",
						  IZXRAT_A1, IZXRAT_A2, IZXRAT_HZ, IZXRAT_CD,
						  IZXRAT_CPR, IZXRAT_EXP, IZXRAT_DOAS, 0);

		float vfDsInp = iz_fan.vfDs;		// save input vfDs before possible alteration
		if (iz_IsHERV())
		{	// HERV: fan design flow input is net (= actual fresh air)
			//   but fan flow is gross (= larger flow, compensates for exhaust air transfer)
			//   no change if vfDs = 0
			if (iz_EATR < 1.f)
				iz_fan.vfDs *= 1.f / (1.f - iz_EATR);
			// else iz_EATR limit check at runtime (subhourly variability)
		}

		// check fan input, set defaults
		//   does not alter vfDs
		//   if vfDs = 0, sets fanTy = C_FANTYCH_NONE (moves air w/o using electricity)
		iz_fan.fn_setup(C_FANAPPCH_ZFAN, this, IZXRAT_FAN, 0.f, NULL);	// checks fan input, does defaults

		// default operating flows unless no fan
		if (iz_fan.fanTy != C_FANTYCH_NONE)
		{
			if (!IsSet(IZXRAT_VFMIN))
			{
				iz_vfMin = vfDsInp;
				fStat(IZXRAT_VFMIN) |= FsSET;
			}
			// iz_vfMax is runtime defaulted to iz_vfMin, see iz_BegSubhr()
		}
		}
		break;

		case C_IZNVTYCH_ANDOAS:
		{
			const char* when = "when izNVType is AirNetDOAS";
			rc |= disallowN(when,
						IZXRAT_A1, IZXRAT_A2, IZXRAT_HZ, IZXRAT_CD,
						ZFAN(VFDS), IZXRAT_VFMAX, 0);
			rc |= require(when, IZXRAT_DOAS);
		}
		break;
		case C_IZNVTYCH_ANIZFLOW:
		case C_IZNVTYCH_ANEXTFLOW:
			rc |= disallowN("when izNVType is flow AIRNET",
						  IZXRAT_A1, IZXRAT_A2, IZXRAT_HZ, IZXRAT_CD,
						  ZFAN(VFDS), IZXRAT_DOAS, 0);
			break;

		default:
			ooer(IZXRAT_NVCNTRL, (char*)MH_S0473, izTy, iz_nvcntrl);	// "Internal error: bad izNVType 0x%x"
			break;
		}
	}

	iz_SetupAfMtrs();			// AFMETER (air flow meter) setup

	return rc;
}	// IZXRAT::iz_Setup
#undef ZFAN
//-------------------------------------------------------------------
void IZXRAT::iz_SetupAfMtrs()
{
	// Air flow categories (set iz_afMtrCat1 and iz_afMtrCat2)
	iz_AfMtrCats();

	// AFMTR ptrs: NULL if no meter specified -> no air flow accounting
	//  one pointer for positive flows, one for negative
	iz_pAfMtr1 = iz_pAfMtr2 = NULL;	// insurance
	if (iz_afMtrCat1 != C_AFCAT_NONE)
	{	// set up ptrs to AFMTR(s)
		const ZNR* zp;
		if (iz_zi1 > 0)
		{	zp = ZrB.GetAt(iz_zi1);
			iz_pAfMtr1 = AfMtrR.GetAtSafe(zp->i.zn_afMtri);
		}
		if (iz_zi2 > 0)
		{	// interzone transfers recorded iff to a different meter
			zp = ZrB.GetAt(iz_zi2);
			AFMTR* pAM = AfMtrR.GetAtSafe(zp->i.zn_afMtri);
			if (pAM == iz_pAfMtr1  && !iz_IsHERV())
				iz_pAfMtr1 = NULL;		// same meter on both sides of IZ
										//   don't record IZ within meter
										//   EXCEPT for HERV
			else
				iz_pAfMtr2 = pAM;		// z2 meter specified and differs from z1
		}
	}
	iz_doingAfMtr					// say this IZXRAT has AFMTR (speedier test)
		= iz_pAfMtr1 != NULL || iz_pAfMtr2 != NULL;	
				
}		// IZXRAT::iz_SetupAfMtrs
//-----------------------------------------------------------------------------
int IZXRAT::iz_IsCZ(	// detect conditioned / unconditioned adjacent zones
	int iZn) const	// 1: check inside zone, else checkout outside
// returns -1 (does not exist), 0=unconditioned, 1=conditioned
{
	int zi = iZn==1 ? iz_zi1 : iz_zi2;
	const ZNR* zp = ZrB.GetAtSafe( zi);
	return !zp ? -1 : !zp->zn_IsUZ();

}	// IZXRAT::iz_IsCZ
//-----------------------------------------------------------------------------
void IZXRAT::iz_AfMtrCats()		// finalize runtime air flow categories
// Tricky: interzone flow UZ/CZ categories differ depending on side of vent
{
	iz_afMtrCat1 = iz_afMtrCat2 = -1;	// flag as unset
	if (!iz_CanHaveAFCat())
		iz_afMtrCat1 = C_AFCAT_NONE;		// not trackable
	else if (IsSet(IZXRAT_AFCATI))
		iz_afMtrCat1 = iz_afCatI;	// use user input (not checked for consistency with IZXRAT type)
	else if (iz_IsSysAir())
		iz_afMtrCat1 = C_AFCAT_HVAC;
	else if (iz_IsDuctLk())
		iz_afMtrCat1 = C_AFCAT_DUCTLK;
	else if (iz_IsHERV())
		iz_afMtrCat1 = C_AFCAT_FANEX;
	else
	{	const static int afCats[3][3] =
		{	{ C_AFCAT_FANEX, C_AFCAT_VNTEX, C_AFCAT_INFEX },
			{ C_AFCAT_FANUZ, C_AFCAT_VNTUZ, C_AFCAT_INFUZ },
			{ C_AFCAT_FANCZ, C_AFCAT_VNTCZ, C_AFCAT_INFCZ }
		};
		
		// fan / vent / infil
		//   Note: vent guess is approx, explicit izAFCat input may be necessary
		int izMode = iz_IsFixedFlow() ? 0 : iz_MightBeNatVent() ? 1 : 2;
		if (iz_IsExterior())
			iz_afMtrCat1 = afCats[0][izMode];
		else if (iz_IsAirNetIZ())
		{	// interzone: set category per *other* side
			//   iz_IsCZ() returns -1 (no zone, not expected), 0 (uncond), 1 (cond)
			int iCZ1 = iz_IsCZ(1);	// inside
			int iCZ2 = iz_IsCZ(2);	// outside
			iz_afMtrCat1 = afCats[iCZ2 + 1][izMode];
			iz_afMtrCat2 = afCats[iCZ1 + 1][izMode];
		}
		else if (iz_IsDOAS())
		{
			iz_afMtrCat1 = C_AFCAT_FANEX; // TODO: Make new DOAS Category?
		}
		else
		{
			iz_afMtrCat1 = 0;  // UKNOWN
		}
	}

	if (iz_afMtrCat2 < 0)
		iz_afMtrCat2 = iz_afMtrCat1;

}		// IZXRAT::iz_AfMtrCats
//-----------------------------------------------------------------------------
RC IZXRAT::iz_SetupNonAirNet()		// interzone transfer one-time initialization

// returns nvcoeff set to constant factor used in interzone vent calc (Btuh/sqrt(dt))

/* Interzone vent story.  Consider two spaces separated by a partition
   containing low and high vents.  Define:

   	t1, t2:	space absolute temps (deg R).  Assume t1>t2 during this
		derivation; code does NOT make this assumption.
        h:	center-to-center height difference between vents (ft).
	ea:	effective area of vents (ft2, as calculated by cgnveffa()).
	cd:	orifice coefficient for vents, typically 0.8 (dimensionless).

   Then q (nat vent heat flow rate from space 1 into space 2, Btuh) is
   (this expression can be derived from PV = nRT):

	q = vhc * ea * cd * sqrt( g*h*(t1-t2)/((t1+t2)/2) ) * (t1-t2)

   where the following constants are required:

	vhc:	volumetric heat capacity of air (about .02 Btu/ft3-R)
	g:	acceleration of gravity (32.17 ft/sec2 = 416,923,200 ft/hr2)

   In this implementation, q is evaluated each subhour using t1 and t2 from
   previous subhour:

	q = C * sqrt( t1-t2 ) * (t1-t2)

   where C is a constant evaluated in here as follows:

	C = vhc * ea * cd * sqrt( g*h/((t1+t2)/2) )

   In this case,

	vhc =	AIRX/60 -- AIRX is vhc*60 for units convenience elsewhere.
	ea =	derived from user input vent areas
	cd =	user input orifice coefficient (dimensionless)
	g =	acceleration of gravity 32.17 ft/sec2 = 416,923,200 ft/hr2
	h =	user input height difference (ft)
	(t1+t2)/2 =	average absolute temperature of spaces.  Approximated
		here as 550 R (90 F) to yield CONSERVATIVE air flow rate.
		Value of C is not very sensitive to this assumption.

    Combining all constant terms, we get:

	C = AIRX/60 * sqrt( 416923200 / 550 ) * ea * cd * sqrt( h )

	C = 14.5 * AIRX * ea * cd * sqrt( h )

    The value C stored in IZXRAT.nvcoeff */

// returns RCOK (future errors possible)
{
#define AIRX (Top.airX( 70.))
		// flow heat capacity calculated @ user's ref W and location elevation, Btuh/cfm-F.
		// temp 70 yields value 1.08 @ sea level and default ref w (but note 90 assumed above).

	iz_nvcoeff = 0.f;		// insurance
	if (iz_nvcntrl == C_IZNVTYCH_ONEWAY || iz_nvcntrl == C_IZNVTYCH_TWOWAY)
		// pre-2010 hi-low vent
		iz_nvcoeff = 					// "C" in comments above
			14.5f						// constant, see comments above
			* AIRX						// air vhc * 60 minutes/hour to get Btuh from cfm.
			* cgnveffa( iz_a1, iz_a2)	// vent eff. area
			* iz_cd						// user input orifice coefficient
			* sqrt( fabs( iz_hz));		// user input vent height diff

	return RCOK;
}           // IZXRAT::iz_SetupNonAirNet
//-----------------------------------------------------------------------------
bool IZXRAT::iz_MightBeNatVent() const	// detect possible controlled nat vent
// returns true iff this looks like typical controlled vent (e.g. openable window)
// used re default iz_afCat
{
	bool ventIsh = 	!iz_IsFixedFlow() && iz_a1 == 0.f && IsSet(IZXRAT_A2);

	return ventIsh;
}		// IZXRAT::iz_MightBeVent
//-----------------------------------------------------------------------------
bool IZXRAT::iz_HasVentEffect() const	// determine whether this IZXRAT can "vent"
// can vary during run due to expressions
// returns 1 iff iz vent mode (iz_ad[ 1]) differs from infil-only iz_ad[ 0]
{
	bool bVentEffect =
		  iz_IsFixedFlow() ? iz_ad[1].ad_mdotP != iz_ad[0].ad_mdotP
		:                    iz_ad[1].ad_Ae != iz_ad[0].ad_Ae;

	return bVentEffect;

}	// IZXRAT::iz_HasVentEffect
//-----------------------------------------------------------------------------
void ZNR::zn_AddIZXFERs()		// add generated IZXFERs (for RSYS, )
{

	if (zn_HasAirHVAC())
	{	// generate IZXFERs for associated supply/return air flow
		zn_AddIZXFER( C_IZNVTYCH_ANSYSAIR, "SysI", &zn_sysAirI);
		zn_AddIZXFER( C_IZNVTYCH_ANSYSAIR, "SysO", &zn_sysAirO);
		// OAV: add conditional hole
		const RSYS* rs = zn_GetRSYS();
		if (rs && rs->rs_CanOAV())
		{	// this RSYS may operated in OAV mode
			// add interzone vent whose size depends on flow
			zn_AddIZXFER( C_IZNVTYCH_ANOAVRLF, "SysOAV", &zn_OAVRlfO);
		}
	}
}		// ZNR::zn_AddIZXFERs
//------------------------------------------------------------------------------
TI ZNR::zn_FindOrAddIZXFER(		// locate or added IZXFER coupled to this zone
	IZNVTYCH _ty,		// C_IZNVTYCH_ANDUCTLKIN etc
	const char* nmSfx,	// IZXFER name = <znName>-<nmSfx>
						//   e.g. "Attic-DLkI"
	const AIRFLOW* pAF /*=NULL*/)
// returns TI of existing or added IZXFER
{
	IZXRAT* ize;
	RLUP( IzxR, ize)
	{	if (ize->iz_nvcntrl == _ty && ize->iz_zi1 == ss
	      && (pAF == NULL || pAF == ize->iz_pAF))
			return ize->ss;
	}
	return zn_AddIZXFER( _ty, nmSfx, pAF);
}		// ZNR::zn_FindOrAddIZXFER
//------------------------------------------------------------------------------
TI ZNR::zn_AddIZXFER(	// add IZXFER coupled to this zone
	IZNVTYCH _ty,			// C_IZNVTYCH_ANDUCTLKIN etc
	const char* nmSfx,		// IZXFER name = <znName>-<nmSfx>
							//   e.g. "Attic-DLkI"
	const AIRFLOW* pAF /*=NULL*/)
// returns idx of added IZXFER
{
	IZXRAT* ize;
	IzxR.add( &ize, ABT);

	ize->SetName( name);
	// append suffix
	int catOption = 1;		// attempt to truncate name size exceeded
	strCatIf( ize->name, sizeof( ize->name), "-", nmSfx, catOption);
	ize->iz_zi1 = ss;		// idx of this zone
	ize->iz_nvcntrl = _ty;
	ize->iz_pAF = pAF;
	if (_ty == C_IZNVTYCH_ANOAVRLF)
	{	RSYS* rs = zn_GetRSYS();
		ize->iz_zi2 = rs->rs_OAVReliefZi;		// relief zone
		ize->iz_hz = i.zn_floorZ + i.zn_ceilingHt;	// assume in ceiling
		ize->iz_cd = 0.6f;
		ize->iz_exp = 0.5f;
	}
	else
		ize->iz_zi2 = -1;		// no other zone

	ize->iz_SetupAfMtrs();		// default iz_afCat, set up AFMTR linkage

	// more init?

	return ize->ss;
}		// ZNR::zn_AddIZXFER
//------------------------------------------------------------------------------
RC IZXRAT::iz_BegHour()		// set hour constants
{
	if (!iz_IsAirNet())
		return RCOK;
#if defined( _DEBUG)
	Validate();
#endif
	if (!iz_IsFixedFlow())
		iz_SetupPresDep();		// vent area can vary hourly
								//   update coefficients
	// else
	//   nothing needed

	if (iz_HasVentEffect())		// if this IZXRAT can "vent"
	{	ZNR* zp;
		if (iz_zi1 > 0)
		{	zp = ZrB.GetAt(iz_zi1);
			zp->zn_anVentEffect++;
		}
		if (iz_zi2 > 0)
		{	zp = ZrB.GetAt(iz_zi2);
			zp->zn_anVentEffect++;
		}
	}
	return RCOK;
}		// IZXRAT::iz_BegHour
//-----------------------------------------------------------------------------
void IZXRAT::iz_SetupPresDep()		// set up constants re pressure-dependent flow
// do not call for fixed flow IZXRATs
{
	// pressure-driven flow (= exterior or interzone hole)
	if (iz_nvcntrl == C_IZNVTYCH_ANHORIZ)
	{	// TODO: do we need hi/lo for horiz?
		ANDAT& ad = iz_ad[ 0];
		// large horizontal opening (e.g. stairwell)
		//   stair angle: 90 for open hole, 0 prevents flow
		float AeHoriz =		// effective area
			float( iz_L1*iz_L2*sin( RAD( iz_stairAngle))*(1.+cos( RAD( iz_stairAngle))));
		if (AeHoriz > 0.f)
		{	float Dhyd = 2.*AeHoriz/(iz_L1+iz_L2);
			float Cs = 0.942f * min( iz_L1 / iz_L2, iz_L2 / iz_L1);
			ad.ad_xDelpF = Cs*Cs * pow( Dhyd, 5) / (2.*AeHoriz*AeHoriz);
			ad.ad_xMbm = 0.055 * sqrt( G0STDX * pow( Dhyd, 5));	// G0STDX re AIRNET_COMPARE
			ad.ad_Ae = AeHoriz * iz_cd;	// pressure-driven flow uses standard scheme
		}
		else
			ad.ad_Ae = 0.;
		iz_ad[ 1] = iz_ad[ 0];
	}
	else
	{	if (!(sstat[ IZXRAT_A2] & FsSET))
			iz_a2 = iz_a1;		// default at runtime so function values propogate
		iz_ad[ 0].ad_Ae = iz_a1 * iz_cd;
		iz_ad[ 1].ad_Ae = iz_a2 * iz_cd;
	}
	iz_ad[ 0].ad_SetupPresDep();
	iz_ad[ 1].ad_SetupPresDep();
}		// IZXRAT::iz_SetupPresDep
//------------------------------------------------------------------------------
RC IZXRAT::iz_BegSubhr()		// set subhr constants
// Determine
//	1) temp and density on either side of element (these do not vary during pressure iteration)
//  2) effective area
{
	RC rc = RCOK;
	if (!iz_IsAirNet())
		return rc;		// nothing to do for non-Airnet

	// this zone
	ZNR* zp1 = ZrB.GetAt( iz_zi1);
	zp1->zn_GetAirStateLs( iz_air1);
	iz_rho1 = zp1->zn_rho0ls;			// density, lbm/cf
#if defined( _DEBUG)
	if (isnan(iz_rho1))
		printf("\nNAN");
	if ((iz_IsSysOrDuct() || iz_IsOAVRelief()) != (iz_pAF != NULL))
		err( PWRN, "IZXRAT '%s': inconsistent iz_pAF", name);
#endif

	if (iz_pAF)
		iz_SetFromAF( iz_pAF);
	else if (iz_IsHERV())
		rc |= iz_CalcHERV();
	else
	{	if (iz_IsExterior())
			// zone 2 is exterior
			iz_GetExteriorConditions( zp1->zn_windPresV);
		else if (iz_IsAirNetIZ())
			// zone 2 is zone
			iz_GetZn2Conditions();
		else if (iz_IsDOAS())
		    // zone 2 is DOAS
			iz_GetDOASConditions();
	#if defined( _DEBUG)
		else
			errCrit( ABT, "Missing IZXRAT::iz_BegSubhr() code");
	#endif

		if (iz_IsFan() || iz_IsFlow() || iz_IsDOAS())
		{	// fixed air flow
			//   massflows depend on iz_rho1/2, must be derived subhourly
			if (!IsSet( IZXRAT_VFMAX))
				iz_vfMax = iz_vfMin;	// default at runtime so function values propogate
			// fan mass flow set in iz_BegSubhr
			iz_ad[ 0].ad_SetFromFixedAVF( iz_vfMin);
			iz_ad[ 1].ad_SetFromFixedAVF( iz_vfMax);
		}
	}

#if defined( AIRNET_COMPARE)
	// recompute air densities using exact AirNet.bas methods
	iz_rho1 = DensityNiles( iz_air1.as_tdb);
	iz_rho2 = DensityNiles( iz_air2.as_tdb);
#endif
	return rc;
}		// IZXRAT::iz_BegSubhr
//------------------------------------------------------------------------------
void IZXRAT::iz_GetZn2Conditions()
{
	ZNR* zp2 = ZrB.GetAt( iz_zi2);
	zp2->zn_GetAirStateLs( iz_air2);
	iz_rho2 = zp2->zn_rho0ls;
	iz_pres2 = -iz_hz * iz_rho2;	// stack
}		// IZXRAT::iz_GetZn2Conditions
//-------------------------------------------------------------------------------
void IZXRAT::iz_ClearZn2Conditions()
{
	iz_air2.as_Set( 0., 0.);
	iz_rho2 = 0.f;
	iz_pres2 = 0.f;
}		// IZXRAT::iz_ClearZn2Conditions
//------------------------------------------------------------------------------
void IZXRAT::iz_ClearResults(
	int iV)	// mode: 0=infil 1=infil+vent
{
	iz_ad[iV].ad_ClearResults();
}		// IZXRAT::iz_ClearResults
//-----------------------------------------------------------------------------
void IZXRAT::iz_GetExteriorConditions(
	float windPresV)		// wind velocity pressure, lbf/ft2
							//   if relevant
// set source conditions to ambient
{
	iz_air2.as_Set(Top.tDbOSh, Top.wOSh);
	iz_rho2 = Top.tp_rhoMoistOSh;
	iz_pres2 =		// exterior pressure constant for subhr
		iz_cpr * windPresV	// pressure coeff * velocity pressure
		- iz_hz * iz_rho2;	// plus stack
}	// IZXRAT::iz_GetExteriorConditions
//-----------------------------------------------------------------------------
void IZXRAT::iz_GetDOASConditions()
// set source conditions to DOAS supply
{
	DOAS* pDOAS = doasR.GetAt(iz_doas);
	iz_air2 = static_cast<AIRSTATE>(pDOAS->oa_supAF);
	iz_rho2 = iz_air2.as_RhoMoist();
	iz_pres2 = 0.f;  // Not used for fixed flows
}	// IZXRAT::iz_GetDOASConditions
//------------------------------------------------------------------------------
RC IZXRAT::iz_Calc()
// pre-2010 hi-low vent
{
	if (iz_IsAirNet())
		return RCBAD;
	ZNR* zp1 = ZrB.GetAt( iz_zi1);
	ZNR* zp2 = ZrB.GetAt( iz_zi2);
	float tdif = zp1->tzls - zp2->tzls;   	// temp diff between zones
	if (iz_nvcntrl != C_IZNVTYCH_ONEWAY || tdif > 0.f)
	{	double hx = iz_ua;
		if (iz_nvcntrl != C_IZNVTYCH_NONE)
			hx += iz_nvcoeff * sqrt( fabs( tdif));
		double qx = hx * tdif;
		zp1->zn_qIzXAnSh -= qx;		// accumulate heat/hour to zone
		zp2->zn_qIzXAnSh += qx;		// same for other zone, opposite sign: positive into zone.
	}
	return RCOK;
}	// IZXRAT::iz_Calc
//--------------------------------------------------------------------------------
RC IZXRAT::iz_MassFlow(		// airnet mass flow for this vent
	int iV)		// vent mode
// used within iteration: zone pressures deemed known
{
	if (!iz_IsAirNet())
		return RCBAD;

	int rc = RCOK;

#if 0 && defined( _DEBUG)
	ZNR* zp2 = NULL;
	if (ZrB.GetAt(iz_zi1)->IsNameMatch("F5 Res Zn-zn")
		&& !Top.isWarmup && !iz_IsExterior() && Top.iHr == 18)
	{
		zp2 = ZrB.GetAt(iz_zi2);
		if (zp2->tzls > 90.f)
			rc = rc;
	}
#endif
	if (!iz_IsFixedFlow() && iz_ad[ iV].ad_Ae > 0.)
	{	// pressure-dependent flow
		double pres1 = ZrB.GetAt( iz_zi1)->zn_pz0W[ iV] - iz_hz*iz_rho1;	// "this" zone pressure

		double pres2 = iz_pres2;		// "other" zone or exterior ambient pressure
										//   no iteration change if exterior
		if (!iz_IsExterior())
			pres2 += ZrB.GetAt( iz_zi2)->zn_pz0W[ iV];	// if zone, include zone pressure

		iz_ad[ iV].ad_MassFlow( pres2 - pres1);
	}
	return rc;
}		// IZXRAT::iz_MassFlow
//-----------------------------------------------------------------------------
RC IZXRAT::iz_ZoneXfers(		// calc transfers due to airnet mass flow
	int iV)
// call *once* per subhr per mode when flows rates and zone pressures are known
{
#if defined( DEBUGDUMP)
	// re debug dump headings (see just below)
	static int dbgDoneHorizHdg[ 2] = { 0 };
	if (ss == 1)
		dbgDoneHorizHdg[ 0] = dbgDoneHorizHdg[ 1] = 0;
#endif

	ZNR* zp1 = ZrB.GetAt( iz_zi1);
	ZNR* zp2 = iz_zi2 > 0 ? ZrB.GetAt( iz_zi2) : NULL;

	ANDAT& ad = iz_ad[ iV];		// active ANDAT

	// add air flow heat to receiving zone
	if (iz_IsHERV())
	{	// flow is always into zone 1
		// iz_air2 = zone entering condition
		zp1->zn_AccumAirFlow( iV, ad.ad_mdotP, iz_air2, ad.ad_tdFan);
	}
	else if (!iz_IsSysOrDuct())	// system and duct heat gain handled by zone model
	{	if (ad.ad_mdotP > 0.)
			// flow is into zone 1
			zp1->zn_AccumAirFlow( iV, ad.ad_mdotP, iz_air2, ad.ad_tdFan);
		else if (zp2)
			// flow is into zone 2
			zp2->zn_AccumAirFlow( iV, -ad.ad_mdotP, iz_air1, ad.ad_tdFan);
		// else heat is going to e.g. ambient, discard
	}

	// horiz opening (stairwell) buoyancy flow
	//   iff sufficient density diff, equal and opposite flows through opening
	//   no pressure balance effect so do calc after massflow interation
	if (iz_nvcntrl == C_IZNVTYCH_ANHORIZ)
	{	// zone 2 is above zone 1 (see input checks)
		double delRho = iz_rho2 - iz_rho1;
		double delpF;
#if defined( _DEBUG)
		if (delRho > 0. && iz_rho1 > 0. && iz_rho2 > 0.)
#else
		if (delRho > 0.)
#endif
		{	// upper zone denser (else no buoyancy flow)
			delpF = ad.ad_xDelpF * delRho;				// flooding pressure
			double pRat = fabs( ad.ad_delP) / delpF;	// pressure diff ratio
			if (pRat < 1.)		// if buoyancy can overcome pressure diff
			{	ad.ad_mdotB = (1.-pRat) * ad.ad_xMbm * sqrt( delRho*(iz_rho1+iz_rho2)/2.);
				// buoyancy heat xfer (equal and opposite)
				zp1->zn_AccumAirFlow( iV, ad.ad_mdotB, iz_air2);
				if (zp2)
					zp2->zn_AccumAirFlow( iV, -ad.ad_mdotB, iz_air1);
			}
		}
		else
		{	delpF = 0.;
			ad.ad_mdotB = 0.;
		}
#if defined( DEBUGDUMP)
		if (DbDo( dbdAIRNET))
		{	if (!dbgDoneHorizHdg[ iV])
			{	DbPrintf( "\nAirNet horiz  mode = %d\n"
						  "vent                   delRho      delpF       mDotB\n", iV);
				dbgDoneHorizHdg[ iV]++;
			}
			DbPrintf( "%-20.20s %10.5f  %10.5f  %10.5f\n", name, delRho, delpF, ad.ad_mdotB);
		}
#endif
	}
	return RCOK;
}		// IZXRAT::iz_ZoneXfers
//-----------------------------------------------------------------------------
RC IZXRAT::iz_EndSubhr()			// end-of-subhour vent calcs
{
	ZNR* zp = ZrB.GetAt( iz_zi1);	// zone
	float fVent = zp->zn_fVent;
	if (iz_HasFan() && iz_fan.fn_mtri)
	{	// calc fan electricity and accum to meter
		// need venting fraction to determine fan operation
		//   use this vent's zone; not general for multizone case TODO
		//   also direct scaling by fVent assumes linear polynomial (true 2-11)
		//      if generalized polynomial support is added, rework required
		// ad_pFan is electrical input power, Btuh (*not* W)
		double fanPwr = (1.f-fVent)*iz_ad[ 0].ad_pFan + fVent*iz_ad[ 1].ad_pFan;
		MtrB.p[ iz_fan.fn_mtri].H.mtr_Accum(
			iz_fan.fn_endUse,			// user-specified end use (default = "fan")
			fanPwr * Top.tp_subhrDur);		// electrical energy, Btu (*not* Wh)
	}

	// air flow accounting
	iz_amfNom // incoming nominal flow, lbm/sec
		= (1.f - fVent)*iz_ad[0].ad_mdotP + fVent * iz_ad[1].ad_mdotP;
	if (iz_nvcntrl == C_IZNVTYCH_ANHORIZ)
		iz_amfNom
			+= (1.f - fVent)*iz_ad[0].ad_mdotB + fVent * iz_ad[1].ad_mdotB;
	if (iz_doingAfMtr && iz_amfNom != 0.f)
	{	if (iz_pAfMtr1)
			iz_pAfMtr1->amt_AccumCat(iz_afMtrCat1, iz_amfNom);
		if (iz_pAfMtr2)
		{	// flow is opposite direction from z2 POV
			// HERV: iz_pAfMtr2 points to exhaust source zone (may be z1)
			//       HERV same for vent / no vent (use iz_ad[ 0] w/o fVent)
			iz_pAfMtr2->amt_AccumCat(iz_afMtrCat2, 
					iz_IsHERV() ? -iz_ad[0].ad_mdotX : -iz_amfNom);
		}
	}

	return RCOK;
}		// IZXRAT::iz_EndSubhr
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// struct AIRNET
//   finds zone pressures that achieve balanced mass flows
///////////////////////////////////////////////////////////////////////////////
AIRNET::AIRNET()
   : an_jac( NULL), an_V1( NULL), an_V2( NULL), an_mdotAbs( NULL), an_didLast( NULL),
     an_nz(0)
{	an_resultsClear[0] = an_resultsClear[1] = 0;
}	// AIRNET::AIRNET
//-----------------------------------------------------------------------------
/*virtual*/ AIRNET::~AIRNET()
{	delete[] an_jac;
	an_jac = NULL;
	delete[] an_V1;
	an_V1 = NULL;
	delete[] an_V2;
	an_V2 = NULL;
	delete[] an_mdotAbs;
	an_mdotAbs = NULL;
	delete[] an_didLast;
	an_didLast = NULL;
}	// AIRNET::~AIRNET
//-----------------------------------------------------------------------------
void AIRNET::an_ClearResults(			// airnet clear results
	int iV)			// vent mode
					//  0 = minimum vent areas (generally infil only)
					//  1 = maximum (e.g. all vents open)
{
	if (!an_resultsClear[iV])
	{	IZXRAT* ize;
		RLUP(IzxR, ize)
			ize->iz_ClearResults(iV);
		an_resultsClear[iV]++;
	}
}		// AIRNET::an_ClearResults
//-----------------------------------------------------------------------------
RC AIRNET::an_Calc(			// airnet flow balance
	int iV)			// vent mode
					//  0 = minimum vent areas (generally infil only)
					//  1 = maximum (e.g. all vents open)
// Net mass flow to each zone must be 0
// Iteratively find zone pressures that produce that balance
{
// re convergence testing
#if 0
0	// extra-tight for testing
0	//   minimal impact on results for many-vent case, 4-12
0	const double ResMax = .0001;	// absolute residual max, lbm/sec
0	const double ResErr = .00001;	// fractional error (see code)
#else
	const double ResAbs = Top.tp_ANTolAbs;	// absolute residual max, lbm/sec
											//   dflt=.00125 (about 1 cfm)
	const double ResRel = Top.tp_ANTolRel;	// fractional error (see code)
											//   dflt=.0001
#endif

#if defined( DEBUGDUMP)
	const int ANDBZMAX = 5;
	double rVSave[ ANDBZMAX];
	double jacSave[ ANDBZMAX*ANDBZMAX];
	bool bDbPrint = DbDo( dbdAIRNET);
#endif

	TMRSTART( TMR_AIRNET);
	RC rc = RCBAD;
	an_resultsClear[iV] = 0;	// set flag for an_ClearResults()

	if (!an_jac)
	{	// allocate working arrays
		an_nz = ZrB.GetCount();
		if (an_nz < 1)
			goto done;		// insurance
		an_jac = new double[ an_nz * an_nz];	// jacobian matrix
		an_V1 = new double[ an_nz];				// residual / correction vector #1
		an_V2 = new double[ an_nz];				// residual / correction vector #2
		an_mdotAbs = new double[ an_nz];		// total abs mass flow by zone
		an_didLast = new int[ an_nz];			// flag re relaxation scheme
	}
#if defined( DEBUGDUMP)
	int nzDb = min( an_nz, ANDBZMAX);		// # zones limited by debug array size
#endif

	// pointers to working vectors: allows swap w/o copy (see below)
	double* rV = an_V1;
	double* rVLast = an_V2;
	int zi;
	int zi0 = ZrB.GetSS0();		// zone subscript offset (re 0-based arrays used here)
	BOOL bConverge = FALSE;		// set TRUE when converged
	int iter;
	for (iter = 0; iter < 20 ; iter++)
	{	VZero( &an_JAC( 0, 0), an_nz*an_nz);
		VZero( rV, an_nz);
		// don't 0 rVLast, an_didLast (save info from last step, init'd below)
		VZero( an_mdotAbs, an_nz);
#if defined( DEBUGDUMP)
		if (bDbPrint)
			DbPrintf( "%s\nAirNet mode = %d   iter = %d\n"
				"vent                     rho1      rho2       delP       mDotP       mDotX         dmdp\n",
				  iter==0 ? "\n-------------" : "",
				  iV, iter);
#endif
		IZXRAT* ize;
		RLUP( IzxR, ize)
		{
			int zi1 = ize->iz_zi1 - zi0;
			if (ize->iz_IsFixedFlow())
			{	rV[ zi1] += ize->iz_ad[ iV].ad_mdotP;
				an_mdotAbs[ zi1] += fabs( ize->iz_ad[ iV].ad_mdotP);
				if (ize->iz_zi2 >= 0)
				{	// off-diagonal (if not outdoors)
					int zi2 = ize->iz_zi2 - zi0;
					rV[ zi2] -= ize->iz_ad[ iV].ad_mdotX;
					an_mdotAbs[ zi2] += fabs( ize->iz_ad[ iV].ad_mdotX);
				}
			}
			else if (ize->iz_MassFlow( iV) == RCOK)		// determine _mdot, _dmdp
			{	// diagonal element
				an_JAC( zi1, zi1) += ize->iz_ad[ iV].ad_dmdp;
				rV[ zi1] += ize->iz_ad[ iV].ad_mdotP;
				an_mdotAbs[ zi1] += fabs( ize->iz_ad[ iV].ad_mdotP);
				if (ize->iz_zi2 >= 0)
				{	// off-diagonal (if not outdoors)
					int zi2 = ize->iz_zi2 - zi0;
					an_JAC( zi2, zi2) += ize->iz_ad[ iV].ad_dmdp;
					an_JAC( zi2, zi1) -= ize->iz_ad[ iV].ad_dmdp;
					an_JAC( zi1, zi2) -= ize->iz_ad[ iV].ad_dmdp;
					rV[ zi2] -= ize->iz_ad[ iV].ad_mdotP;
					an_mdotAbs[ zi2] += fabs( ize->iz_ad[ iV].ad_mdotP);
				}
			}
#if defined( DEBUGDUMP)
			if (bDbPrint)
			{	const ANDAT& ad = ize->iz_ad[ iV];
				DbPrintf( "%-20.20s %d %6.4f %9.4f %10.6f  %10.5f  %10.5f  %11.5f\n",
					ize->name, iV, ize->iz_rho1, ize->iz_rho2, ad.ad_delP, ad.ad_mdotP, ad.ad_mdotX, ad.ad_dmdp);
			}
#endif
		}

		bConverge = TRUE;
		for (zi=0; zi < an_nz; zi++)
		{	// Net mass flow to each zone s/b 0
			// Solution not converged if netAMF > max( ResAbs, totAMF*ResRel)
			//  Using larger of the two tolerances means zones with small abs flow converge loosely.
			//  This avoids extra iterations to converge zones that have minor impact.
			if (fabs( rV[ zi]) > ResAbs && fabs( rV[ zi]) > ResRel*an_mdotAbs[ zi])
			{	bConverge = FALSE;
				break;		// this zone not balanced, no need to check further
			}
		}
		if (bConverge)
		{
#if defined( DEBUGDUMP)
			if (bDbPrint)
			{	DbPrintf( "\nAirNet mode = %d converged (%d iter)\n"
					  "zone                 znPres        mDotP\n", iV, iter+1);
				for (zi=0; zi < an_nz; zi++)
				{	ZNR* zp = ZrB.GetAt( zi+zi0);
					DbPrintf( "%-20.20s %8.5f  %11.8f\n", zp->name, zp->zn_pz0W[ iV], rV[ zi]);
				}
			}
#endif
			break;		// all balanced, we're done
		}
#if defined( DEBUGDUMP)
		// save info for debug print (changed by gaussjb())
		if (bDbPrint)
		{	VCopy(rVSave, nzDb, rV);
			for (zi = 0; zi < nzDb; zi++)
				VCopy(&jacSave[zi*nzDb], nzDb, &an_jac[zi*an_nz]);
		}
#endif
		// solve for pressure corrections

#if defined( _DEBUG)
		int gjRet =
#endif
			gaussjb(an_jac, an_nz, rV, 1);
#if defined( _DEBUG)
		if (gjRet)
			err( PWRN,				// display program error msg, wait for key
				"AIRNET::an_Calc: matrix is %s",
					gjRet == 1 ? "singular" : "too large");
#endif

#if 0 && defined( DEBUGDUMP)
		if (bDbPrint)
		{	DbPrintf( "\nzone                 znPres       mDotP    corr     jac ...\n");
			for (zi=0; zi < nzDb; zi++)
			{	ZNR* zp = ZrB.GetAt( zi+zi0);
				DbPrintf( "%-20.20s %8.5f  %10.5f %8.5f  ", zp->name, zp->zn_pz0W[ iV], rVSave[ zi], rV[ zi]);
				VDbPrintf( NULL, &jacSave[ zi*nzDb], nzDb, " %10.3f");
			}
		}
#endif

		double relax = .75;
		for (zi=0; zi < an_nz; zi++)
		{	if (iter == 0 || rVLast[ zi] == 0.)
			{	relax = .75;
				an_didLast[ zi] = 0;
			}
			else
			{	// ratio current correction to prior (check for oscillation)
				double r = rV[ zi] / rVLast[ zi];
				if (r < -.5 && !an_didLast[ zi])
				{	// this correction is opposite sign
					// relax = common ratio of geometric progression
					relax = 1./(1.-r);
					an_didLast[ zi] = 1;
				}
				else
				{	relax = 1.;
					an_didLast[ zi] = 0;
				}
			}
			ZNR* zp = ZrB.GetAt( zi+zi0);
			zp->zn_pz0W[ iV] += relax * rV[ zi];	// update zone pressure with correction
													//   (possibly w/ relaxation)
		}
		double* rVt = rV;
		rV = rVLast;
		rVLast = rVt;	// swap working vectors
		//  (save corrections from this iter)
	}

	if (!bConverge)
		err( WRN, "%s: AirNet convergence failure", Top.When( C_IVLCH_S));
	else
	{	for (zi=0; zi < an_nz; zi++)
		{	ZNR* zp = ZrB.GetAt( zi+zi0);
			if (fabs( zp->zn_pz0W[ iV]) > 3.)
			{	// zone pressure > 3 lb/ft2 (= 150 Pa approx)
				//   notify user, wait for keypress
				//   ignore during early autosizing --
				//      transient unreasonable values have been seen
				if (!Top.tp_autoSizing || Top.tp_pass2)
					err( WRN,
						"Zone '%s', %s: unreasonable mode %d pressure %0.2f lb/ft2\n",
						zp->name, Top.When( C_IVLCH_S), iV, zp->zn_pz0W[ iV]);
			}
		}
	}
	rc = RCOK;
done:
	TMRSTOP( TMR_AIRNET);
	return rc;

}	// AIRNET::an_Calc
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// struct DBC -- duct outside surface boundary boundary conditions
// class DUCTSEG -- single segment of duct system
///////////////////////////////////////////////////////////////////////////////
void DBC::sb_Init( DUCTSEG* pDS)
{
	sb_pDS = pDS;
}		// DBC::sb_Init
//-----------------------------------------------------------------------------
/*virtual*/ double DBC::sb_AreaNet() const		// *outside* duct area
// returns exposed (heat transfer) area
{	return sb_pDS ? sb_pDS->ds_exArea : 0.;
}	// DBC::sb_Area
//-----------------------------------------------------------------------------
/*virtual*/ const char* DBC::sb_ParentName() const
{	return sb_pDS ? sb_pDS->name : "?";
}		// SBC::sb_ParentName
//-----------------------------------------------------------------------------
/*virtual*/ int DBC::sb_Class() const
{	return sfcDUCT;
}	// DBC::sb_Class
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class DUCTSEG: duct segment
///////////////////////////////////////////////////////////////////////////////
DUCTSEG::DUCTSEG( basAnc* b, TI i, SI noZ/*=0*/)
	: record( b, i, noZ)
{	ds_Init( 1);
}		// DUCTSEG::DUCTSEG
//-----------------------------------------------------------------------------
/*virtual*/ DUCTSEG::~DUCTSEG()
{
}	// DUCTSEG::~DUCTSEG
//-----------------------------------------------------------------------------
void DUCTSEG::ds_Init(			// init DUCTSEG
	[[maybe_unused]] int options/*=0*/)
{
	ds_sbcO.sb_Init( this);
#if 0
x	if ((options&1) == 0)
x	{
x
x	}
#endif
}		// ds_Init
//-----------------------------------------------------------------------------
/*virtual*/ void DUCTSEG::Copy( const record* pSrc, int options/*=0*/)
{
	record::Copy( pSrc, options);
	ds_Init( 1);
}		// DUCTSEG::Copy
//-----------------------------------------------------------------------------
ZNR* DUCTSEG::ds_GetExZone() const	// zone outside duct
// returns zone through which duct segment runs
// NULL if no zone (exterior, adiabatic, )
{
	TI zi = ds_sbcO.sb_zi;
	ZNR* zp = zi > 0 ? ZrB.GetAt( zi) : NULL;
	return zp;
}		// DUCTSEG::ds_GetExZone
//-----------------------------------------------------------------------------
RSYS* DUCTSEG::ds_GetRSYS() const
{
	RSYS* pRS = RsR.GetAt( ownTi);
	return pRS;
}		// DUCTSEG::ds_GetRSYS
//-----------------------------------------------------------------------------
DUCTSEGRES* DUCTSEG::ds_GetDUCTSEGRES() const
{
	DUCTSEGRES* pDSR = DsResR.GetAt( ss);
	return pDSR;
}		// DUCTSEG::ds_GetDUCTSEGRES
//-----------------------------------------------------------------------------
bool DUCTSEG::ds_IsRound() const
{	// if ds_exArea is input, assume flat area-only model
	// else round
	return !IsSet( DUCTSEG_EXAREA);
}	// DUCTSEG::ds_IsRound
//-----------------------------------------------------------------------------
RC DUCTSEG::ds_TopDS(			// set up run record
	const DUCTSEG* pDSi)		// input record
// caller must allocate this (in DsR)
{
	RC rc = RCOK;

	Copy( pDSi);	// copy record, incl name, excl internal front overhead.

	if (ds_IsRound())
	{	if (!IsSet( DUCTSEG_DIAM))
		{	if (ds_len > 0.f)
				ds_diam = ds_inArea / (kPi * ds_len);
		}
		if (ds_diam < .001f)
			rc |= oer( "cannot determine dsDiam");
		else if (!IsSet( DUCTSEG_INAREA))
			ds_inArea = kPi * ds_diam * ds_len;		// OK if 0
		else if (!IsSet( DUCTSEG_LEN))
			ds_len = ds_inArea / (kPi * ds_diam);	// OK if 0
	}

	// insulation material and thickness
	// initialize to bare duct
	ds_insulKA = 0.;
	ds_insulKB = 0.;
	ds_insulThkEff = 0.f;

	if (ds_insulR != 0.f		// if duct insulation specified
	 || ds_insulMati > 0)
	{	if (ds_insulMati == 0)
		{	// no material: assume typical fiberglass
			MAT::mt_CondAB( .025f, .00418f, 70.f, ds_insulKA, ds_insulKB);
			ds_insulThk = ds_InsulK( 70.f) * ds_insulR;
		}
		else
		{	MAT* pM;
			rc |= ckRefPt( &MatiB, ds_insulMati, "dsInsulMat", NULL, (record **)&pM);
			if (rc != RCOK)
				return rc;		// give up
			pM->mt_CondAB( ds_insulKA, ds_insulKB);
			double insulK = ds_InsulK( 70.f);
			if (IsSet( DUCTSEG_INSULR))
				ds_insulThk = insulK * ds_insulR;
			else if (pM->mt_thk <= 0.f)
				rc |= oer( "cannot determine insulation thickness (no matThk in material '%s')",
						pM->name);
			else
			{	ds_insulThk = pM->mt_thk;
				ds_insulR = ds_insulThk / insulK;
			}
		}
	}
	// here: if RCOK, then ds_insulThk is known (may be 0)

	ds_RconvIn = 0.4;		// approx inside convection resistance, ft2-F/Btuh
							//   derived per HOF correlations per typical diam, temp, vel
	if (rc == RCOK)
	{	if (ds_IsRound())
		{	// round configuration
			double diamOut = ds_diam + 2.*ds_insulThk;
			double diamRat = diamOut / ds_diam;	// ratio of outside to inside diameter

			ds_exArea = ds_inArea * diamRat;		// outside surface area (may be 0)

			ds_insulThkEff = diamOut * log( diamRat) / 2.;	// effective thickness
															//   corrects for curvature
			ds_RconvIn *= diamRat;		// effective inside convection resistance
		}
		else
		{	// flat configuration
			ds_insulThkEff = ds_insulThk;

			// ds_RconvIn: no adjustment required
		}
	}

	// nominal conduction UA
	double insulK = ds_InsulK( 70);
	double Rduct = ds_RconvIn + .68;	// inside + typical outside surface coeff
	if (insulK > 0.)
		Rduct += ds_insulThkEff / insulK;
	double UANom = ds_exArea / Rduct;

	ZNR* zpx = ds_GetExZone();
	if (zpx)
	{	zpx->zn_ductCondUANom += UANom;
		if (ds_leakF > 0.)
		{	if (ds_IsSupply())
				zpx->zn_FindOrAddIZXFER( C_IZNVTYCH_ANDUCTLK, "DLkI", &zpx->zn_ductLkI);
			else
				zpx->zn_FindOrAddIZXFER( C_IZNVTYCH_ANDUCTLK, "DLkO", &zpx->zn_ductLkO);
		}
	}

	return rc;
}		// DUCTSEG::ds_TopDS
//-----------------------------------------------------------------------------
void DUCTSEG::ds_SetRunConstants()		// set constants that do not vary during simulation
// call *after* TopZn2() -- uses zone radiant exhange info
{
	ZNR* zpx = ds_GetExZone();		// zone outside duct (NULL if none)
	ds_sbcO.sb_SetRunConstants( zpx);
}		// DUCTSEG::ds_SetRunConstants
//----------------------------------------------------------------------------
#if 0
RC DUCTSEG::ds_SetSizeFromAVF(		// adjust diameter based on flow
	float avf)		// design air volume flow, cfm
{
	const RSYS* pRS = ds_GetRSYS();

	// # of runs
	int nR = pRS->rs_areaServed / ds_desAirVel;

	float avfPerRun = avf / ds_nDuctRun;

	float ductArea = avfPerRun / ds_airVelDs;

	ds_diam = 2.f * sqrt( ductArea / kPi);


}	// DUCTSEG::ds_SetSizeFromAVF
//-----------------------------------------------------------------------------
int DUCTSEG::ds_GetDuctRunInfo(
	float& runLen)	// average run length, ft
// returns # of runs
{	
	const RSYS* pRS = ds_GetRSYS();

	// # of runs
	int nR = max( 1, iRound( pRS->rs_areaServed / ds_CFAperRun));
}	// DUCTSEG::ds_GetRunInfo
#endif
//-----------------------------------------------------------------------------
RC DUCTSEG::ds_BegHour()
{
	// overall resistance duct air to outside surface
	float T = (ds_air[ 2].as_tdb + ds_sbcO.sb_tSrf) / 2.f;	// avg insul temp
	double insulK = ds_InsulK( T);		// temp-adjusted insulation conductivity

	ds_Rduct = ds_RconvIn;		// inside surface convection resistance
								//   (constant, corrected for in/out area ratio)
	if (insulK > 0.)
		ds_Rduct += ds_insulThkEff / insulK;

	ds_Uduct = ds_Rduct > 0. ? 1./ds_Rduct : 0.;
	return RCOK;
}		// DUCTSEG::ds_BegHour
//-----------------------------------------------------------------------------
RC DUCTSEG::ds_BegSubhr()
{
	RC rc=RCOK;

	ZNR* zpx = ds_GetExZone();

	ds_sbcO.sb_SetCoeffs( zpx);

	// if duct is within a zone, accumulate air/radiant coupling
	if (zpx)
		zpx->zn_cxSh += ds_exArea * ds_sbcO.sb_cx;

	ds_uaTot = ds_exArea * ds_sbcO.sb_fRat;

	ds_qCondFL = ds_qCond = ds_qCondRad = ds_qCondAir
		= ds_qLeakSen = ds_qLeakLat = 0.;
	ds_GetDUCTSEGRES()->curr.S.dsr_Clear();

	return rc;
}		// DUCTSEG::ds_BegSubhr
//-----------------------------------------------------------------------------
void DBC::sb_SetRunConstants(		// duct boundary constants
	ZNR* zpx)
// set mbrs that do not change during simulation
// see also SBC::sb_SetRunConstants
// sb_Fp must be set
{
	if (zpx)
		sb_frRad = sigmaSB4 * sb_Fp;
	else if (sb_pDS->ds_exCnd == C_EXCNDCH_ADIABATIC
		  || sb_pDS->ds_exCnd == C_EXCNDCH_SPECT)
	{	// nothing required

	}
	else if (sb_pDS->ds_exCnd == C_EXCNDCH_AMBIENT)
	{	float tilt = RAD( 45.f);	// re LW to sky: assume tilt = 45 deg
		double vfSkyLW  = .5 + .5*cos( tilt);
		double vfGrndLW  = .5 - .5*cos( tilt);
		double cosHalfTilt = cos( tilt/2.);
		sb_fSky = vfSkyLW * cosHalfTilt;
		sb_fAir = vfSkyLW * (1. - cosHalfTilt) + vfGrndLW;
		sb_frRad = sigmaSB4 * sb_epsLW;
	}
	else
		errCrit( WRN, "Missing SBC::sb_SetRunConstants() code");

}		// DBC::sb_SetRunConstants
//-----------------------------------------------------------------------------
void DBC::sb_SetCoeffs(
	ZNR* zpx)		// zone containinng ducts, NULL if none
{
	if (zpx)
	{	sb_txa = zpx->tzls;
		sb_w = zpx->wzls;
		sb_txr = zpx->zn_trls;
		sb_hxr = sb_frRad*pow3( DegFtoR( 0.5*(sb_tSrf+sb_txr)));
		// sb_hxa from input
	}
	else if (sb_pDS->ds_exCnd == C_EXCNDCH_AMBIENT)
	{	sb_txa = Top.tDbOSh;
		sb_w = Top.wOSh;
		double hrSky = sb_frRad * sb_fSky * pow3( DegFtoR( 0.5*(sb_tSrf + Top.tSkySh)));
		double hrAir = sb_frRad * sb_fAir * pow3( DegFtoR( 0.5*(sb_tSrf + Top.tDbOSh)));
		sb_hxr = hrSky + hrAir;
		sb_txr = sb_hxr > 0.f ? (hrSky*Top.tSkySh + hrAir*Top.tDbOSh) / sb_hxr : 0.f;
		// sb_hxa from input
	}
	else if (sb_pDS->ds_exCnd == C_EXCNDCH_SPECT)
	{	// sb_txa from input
		// sb_hxa from input
		sb_txr = sb_txa;		// assume radiant = air
	}
	// else adiabatic: nothing to do

	sb_hxtot = sb_hxa + sb_hxr;		// frequently used sum

	sb_uRat = 1./(1. + sb_pDS->ds_Rduct*sb_hxtot);
	sb_fRat = sb_uRat * sb_hxtot;
	sb_cx = sb_hxa*sb_hxr/(sb_hxtot+sb_pDS->ds_Uduct);

	// adjacent environmental temp
	sb_txe = sb_hxtot > 0. ? (sb_txa*sb_hxa + sb_txr*sb_hxr) / sb_hxtot : 0.f;

}		// DBC::sb_SetCoeffs
//-----------------------------------------------------------------------------
AIRSTATE DUCTSEG::ds_CalcFL(		// current performance at full load
	const AIRSTATE& as,		// entering air state
	const double amf)		// dry air mass flow rate, lbm/hr
// sets ds_beta (used by next ds_CalcFLRev() call, if any)
//      ds_air[ ] air state at several locations
//			[ 0]=entering
//			[ 1]=after leak addition (return only)
//			[ 2]=average (consistent w/ conduction loss)
//			[ 3]=leaving

// returns leaving air state (ds_air[ 3])
{
	ds_amfFL = amf;
	ds_air[ 1] = ds_air[ 0] = as;

	if (!ds_IsSupply() && ds_leakF > 0.f)
	{	// return duct: leakage is into duct
		ds_air[ 1].as_MixF( ds_leakF, ds_sbcO.sb_txa, ds_sbcO.sb_w);
	}

	ds_sbcO.sb_tSrf = ds_sbcO.sb_txe;	// outside surface temp
										//  adjusted in ds_FinalizeSh() if conduction

	if (ds_uaTot > 0.)
	{
		// conduction loss: duct = heat exchanger
		double mCp = ds_amfFL*Top.tp_airSH;		// capacity flow rate (moist air), Btuh/F
		// beta = 1 - (approach to surround)
		//   0: leaving air db = surround temp
		//   1: leaving air = entering air
		ds_beta = mCp > .1f
						? exp( max( -80., -ds_uaTot/mCp))
						: 0.f;		// very small air flow
		ds_air[ 3].as_Set(
				ds_beta*ds_air[ 1].as_tdb + (1.f-ds_beta)*ds_sbcO.sb_txe,
				ds_air[ 1].as_w);
	}
	else
	{	// no conduction
		ds_beta = 1.;
		ds_air[ 3] = ds_air[ 1];
	}

	return ds_air[ 3];
}		// DUCTSEG::ds_CalcFL
//-----------------------------------------------------------------------------
AIRSTATE DUCTSEG::ds_CalcFLRev(		// calc entering state given leaving
	const AIRSTATE& as)		// leaving air state
// call *only* for supply ducts
// assume prior call to ds_CalcFL() which sets ds_beta
// returns entering air state
{
	ds_air[ 3] = as;

	double tIn = ds_beta > 0.f
					? (ds_air[ 3].as_tdb - (1.f-ds_beta)*ds_sbcO.sb_txe) / ds_beta
					: ds_air[ 3].as_tdb;

	ds_air[ 1].as_Set( tIn, ds_air[ 3].as_w);
	ds_air[ 0] = ds_air[ 1];
	return ds_air[ 0];
}		// DUCTSEG::ds_CalcFLRev
//-----------------------------------------------------------------------------
void DUCTSEG::ds_FinalizeSh(		// finish subhour calcs
	float runF)		// run fraction
{
	ZNR* zpx = ds_GetExZone();		// zone containing ductseq (NULL if not in zone)

	// finalize conduction
	//   ds_air[ 0], ds_air[ 1], and ds_air[ 3] must be known
	ds_air[2] = ds_air[1];
	double mCpFL = ds_amfFL * Top.tp_airSH;
	// ds_qCondFL = ds_qCond = ds_qCondAir = ds_qCondRad = 0. per ds_BegSubhr
	if (ds_uaTot > 0.)
	{
		ds_qCondFL =		// conduction loss rate, Btuh; + = out of duct
			mCpFL * (ds_air[1].as_tdb - ds_air[3].as_tdb);
		ds_air[2].as_tdb = ds_qCondFL / ds_uaTot + ds_sbcO.sb_txe;	// mean temp when flow
		ds_sbcO.sb_qSrf = ds_qCondFL / ds_exArea;	// average conduction
												//   ds_exArea > 0 since uaTot > 0)
		if (ds_sbcO.sb_hxtot > 0.)
			ds_sbcO.sb_tSrf += ds_sbcO.sb_qSrf / ds_sbcO.sb_hxtot;
#if defined( _DEBUG)
		// if (ds_Uduct > 0.)
		{	double tdb2 = ds_sbcO.sb_qSrf / ds_Uduct + ds_sbcO.sb_tSrf;
		double tErrF = frDiff(ds_air[2].as_tdb, tdb2, .01);
		if (tErrF > .001)
			orMsg(WRNRT, "avg temp inconsistency (%.5f)", tErrF);
		}
#endif

		if (runF > 0.f)
		{
			ds_qCond = ds_qCondFL * runF;	// conduction loss, Btuh (+ = out of duct)
			ds_qCondAir = runF * ds_exArea * ds_sbcO.sb_hxa * (ds_sbcO.sb_tSrf - ds_sbcO.sb_txa);
			ds_qCondRad = runF * ds_exArea * ds_sbcO.sb_hxr * (ds_sbcO.sb_tSrf - ds_sbcO.sb_txr);
#if defined( _DEBUG)
			// compare separately calc'd conduction, small diffs are common
			double condErrF = frDiff(ds_qCond, ds_qCondAir + ds_qCondRad,
				.1);				// .01 -> .1  8-9-2013
			if (condErrF > .002)	// .001 -> .002 8-9-2013
				orMsg(WRNRT, "conduction mismatch (%.5f)", condErrF);
#endif
			if (zpx)
			{	// conduction gains to zone (for next step)
				zpx->zn_qDuctCondAir += ds_qCondAir;
				zpx->zn_qDuctCondRad += ds_qCondRad;
			}
		}
	}

	// leakage
	const RSYS* pRS = ds_GetRSYS();
	if (runF > 0.f && ds_leakF > 0.f)
	{
		double amfLk = runF * ds_amfFL * ds_leakF;	// leak amf, lb/hr
		double tz = pRS->rs_asRet.as_tdb;
		double wz = pRS->rs_asRet.as_w;
		if (ds_IsSupply())
		{	// supply (leakage: + = out of duct, heating loss > 0, cooling loss < 0)
			ds_qLeakSen = amfLk * Top.tp_airSH * (ds_air[3].as_tdb - tz);
			ds_qLeakLat = amfLk * PsyHCondWtr * (ds_air[3].as_w - wz);
			if (zpx)
				zpx->zn_ductLkI.af_AccumDry(amfLk, ds_air[3]);
		}
		else
		{	// return
			//   leakage flow is into the duct -- that is, negative per sign convention
			//   ds_amfFL is always > 0, so amfLk must be negated
			if (zpx)
				zpx->zn_ductLkO.af_AccumDry(-amfLk, zpx->tz, zpx->wz);
			// return leakage losses: duct flow * deltaT across leak
			//  heating loss > 0, cooling loss < 0
			ds_qLeakSen = runF * ds_amfFL * Top.tp_airSH * (ds_air[0].as_tdb - ds_air[1].as_tdb);
			ds_qLeakLat = runF * ds_amfFL * PsyHCondWtr * (ds_air[0].as_w - ds_air[1].as_w);
		}
	}
	// else ds_qLeakSen = ds_qLeakLat = 0. per ds_BegSubHr()

	// populate results
	//   results zeroed in DUCTSEG::ds_BegSubhr()
	DUCTSEGRES_IVL_SUB& R = ds_GetDUCTSEGRES()->curr.S;
	if (pRS->rs_mode == RSYS::rsmHEAT)
	{	R.qhCond = ds_qCond * Top.tp_subhrDur;
		R.qhLeakSen = ds_qLeakSen * Top.tp_subhrDur;
		R.qhTotSen = R.qhCond + R.qhLeakSen;
	}
	else
	{
		R.qcCond = ds_qCond * Top.tp_subhrDur;
		R.qcLeakSen = ds_qLeakSen * Top.tp_subhrDur;
		R.qcTotSen = R.qcCond + R.qcLeakSen;
		R.qcLeakLat = ds_qLeakLat * Top.tp_subhrDur;
	}


#if defined( DEBUGDUMP)
	if (DbDo(dbdDUCT))
		ds_DbDump();
#endif
}		// DUCTSEG::ds_FinalizeSh
//-----------------------------------------------------------------------------
void DUCTSEG::ds_AfterSubhr()
{
	// last step surface temps (w/o *e, re probes)
	// ds_sbcI does not exist (not modeled)
	ds_sbcO.sb_tSrfls = ds_sbcO.sb_tSrf;
}		// DUCTSEG::ds_AfterSubhr
//-----------------------------------------------------------------------------
void DUCTSEG::ds_DbDump() const
{
	float runF = ds_GetRSYS()->rs_runF;

	const char* loc = ds_exCnd == C_EXCNDCH_ADJZN
						? ds_GetExZone()->name
						: getChoiTx( DUCTSEG_EXCND);

	DbPrintf("DUCTSEG '%s':  ty=%s  loc=%s  txa=%0.2f  txr=%0.2f  txe=%0.2f\n"
		"     hc=%0.3f  hr=%0.3f  htot=%0.3f  cx=%0.3f  Rduct=%0.3f  Uduct=%0.4f\n"
		"     amfFL=%.1f  amfLeak=%.1f  uaTot=%0.4f  beta=%0.3f  tSrf=%0.2f\n"
		"     runF=%0.3f  qCondAir=%0.2f  qCondRad=%0.2f, qCondTot=%0.2f\n"
		"     ta0=%.2f  ta1=%.2f  ta2=%.2f  ta3=%.2f\n",
		name, getChoiTx( DUCTSEG_TY), loc,
		ds_sbcO.sb_txa, ds_sbcO.sb_txr, ds_sbcO.sb_txe,
		ds_sbcO.sb_hxa, ds_sbcO.sb_hxr, ds_sbcO.sb_hxtot, ds_sbcO.sb_cx, ds_Rduct, ds_Uduct,
		ds_amfFL, runF*ds_amfFL*ds_leakF,
		ds_uaTot, ds_beta, ds_sbcO.sb_tSrf,
		runF, ds_qCondAir, ds_qCondRad, ds_qCond,
		ds_air[ 0].as_tdb, ds_air[ 1].as_tdb, ds_air[ 2].as_tdb, ds_air[ 3].as_tdb);
}		// DUCTSEG::ds_DbDump
//=============================================================================

// end of cgcomp.cpp
