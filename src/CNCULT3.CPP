// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncult3.cpp: cul user interface tables and functions for CSE
//   part 3: fcns for checking surfaces etc, called from cncult2.cpp:topCkf.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>		// record: base class for rccn.h classes
#include <rccn.h>
#include <MSGHANS.H>	// MH_S0510


#include "solar.h"
#include "PSYCHRO.H"
#include "CNGUTS.H"
#include "mspak.h"
#include "Foundation.h"
#include "ashwface.h"

#include <CUPARSE.H>	// perl
#include <CUL.H>		// FsSET oer oWarn quifnn classObjTx
#include "CVPAK.H"

//declaration for this file:
#include <IRATS.H>		// declarations of input record arrays (rats)
#include <CNCULTI.H>	// cncult internal functions shared only amoung cncult,2,3,4,5.cpp


/*-------------------------------- DEFINES --------------------------------*/

/*---------------------------- LOCAL VARIABLES ----------------------------*/

static TI msiLast = 0;	// last used SFI.msi: MsR subscr for delayed-model surface.
						//  (assign b4 creating mass records as used in sgdists b4 final net area known.)
						//  topCkf(), topSf1(), topMsCon(), topSf2().

static anc<SFI>* SFIAncs[] = { &SfiB, &DriB, &GziB, NULL };		// anchors for all SFI collections
																//   facilitates all-SFI iteration

/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL FLOAT FC seriesU( FLOAT u1, FLOAT u2);
LOCAL RC    FC cnuCompAdd( const XSURF* x, const char* name, TI zi, TI* pxi, XSRAT** pxr);
LOCAL void  FC cnuSgDist( XSURF* x, SI targTy, TI targTi, NANDAT fso, NANDAT fsc);


// FUNCTIONS FOR CHECKING SURFACES, etc =============================================

void FC topPr()		// check perimeters
{
	PRI *pr;
	RLUP( PriB, pr)				// loop SFI records
	{
		// check/access zone for perimeter

		if (ckOwnRefPt( &ZrB, pr ) )
			continue;				// bad ref (msg issued, rc set, no run).

		// perimeter: copy members for inclusion in zone UA in cnguts:cgRunInit (sep input mbrs used for correct units)

		if (pr->CkSet( PRI_PRLEN)==RCOK)	// be sure set: insurance
			pr->x.xs_area = pr->prLen;  	// perim length goes in XSURF .area
		if (pr->CkSet( PRI_PRF2)==RCOK)
		{	pr->x.xs_uval = pr->prF2;   	// F2 factor goes in .uval
			pr->x.xs_UNom = pr->prF2;
		}

		// perimeter: add traditional XSURF to zone

		cnuCompAdd( &pr->x, pr->name, pr->ownTi, &pr->xi, NULL);		// add XSURF to zone, below.
	}
}		// topPr
//===========================================================================
//  surface-classifying time constant (hc/surf_h) constants for topSf1
//---------------------------------------------------------------------------
#if 0//old: conMass set in cncult2:topCon, surf_h not known
x
x #define MIN_MS_TC 0.25f	// shortest time constant (hc/surf_h) for mass model. 15 minutes determined by Niles for Chip, 2-26-91.
x
#elif 0//1-95 interim uncompiled version
x
x /* history notes: massiveness was decided b4 1-95 in cncult2:conClass based on
x    construction's hc/1.3 (actual surf_h not known).  Comments there included:
x
x    * "Treat reasonably light framed constructins as massless so normal
x       2x4 / 2x6 walls don't get classified as massive" -- crsimsrf.cpp.
x       Note this usually isn't as stupid as it looks: TYPICALLY such constructions
x       have 2 surface layers separated by a cavity of rval >> surface film,
x       so typically actually only about half the thermal mass is thermally coupled
x       to each zone, which would come out light by next test anyway -- rob 2-91.
x       But a cleverer test would be better.  And may wanna determine portion of
x       thermal mass coupled to each zone and put it in its znCAir. *
x
x    * "Time constant = hc/h should be greater than an hour to be massive"
x       Here h value is the u of surface film and hc is that of entire con:
x       assumes resistances within wall << surface film, a marginal assumption
x       for walls with framed cavity and/or insulation (but typically not off by
x       much more than a factor of 2, rob guesses 2-91).
x       Use generic surface film h (1.3) from t24pak.h: actual not known here.*
x
x    Here we now recode to do similar tests using tc computed with actual surf_h.
x
x */
x #define TC_MASS 1.f		// time constant above which to use mass for AUTO surface
x #define TC_FRAMED_MASS 1.54f	// ditto if construction is framed
x
x #define MIN_TC_MS 0.25f	// shortest time constant (hc/surf_h) for mass model. 15 minutes determined by Niles for Chip, 2-26-91.
x
#else//1-95: subhourly masses; rationalized classification here

/* history notes: massiveness was decided b4 1-95 in cncult2:conClass based on
   construction's hc/1.3 (actual surf_h not known).  Comments there included:

   * "Treat reasonably light framed constructins as massless so normal
     2x4 / 2x6 walls don't get classified as massive" -- crsimsrf.cpp.
     Note this usually isn't as stupid as it looks: TYPICALLY such constructions
     have 2 surface layers separated by a cavity of rval >> surface film,
     so typically actually only about half the thermal mass is thermally coupled
     to each zone, which would come out light by next test anyway -- rob 2-91.
     But a cleverer test would be better.  And may wanna determine portion of
     thermal mass coupled to each zone and put it in its znCAir. *

   * "Time constant = hc/h should be greater than an hour to be massive"
     Here h value is the u of surface film and hc is that of entire con:
     assumes resistances within wall << surface film, a marginal assumption
     for walls with framed cavity and/or insulation (but typically not off by
     much more than a factor of 2, rob guesses 2-91).
     Use generic surface film h (1.3) from t24pak.h: actual not known here.*

   Here 1-9-95 we have recoded to do similar tests using tc computed with actual surf_h,
   tentatively using proportionately smaller time constants for the new subhourly
   masses. It is intended than normal 2x4 / 2x6 walls may come out massive.

   Framing distinction dropped 1-10-95 cuz expected framings come out as desired
   anyway (Phil says sheet rock and surface film alone have tc > .25 hour)
   & criteria still too crude to bother perfecting.

   Hourly masses left in for exploratory testing, not used by default 1-10-95
   cuz tc using all hc in construction likely to be misleadingly large in cases
   such as sheet rock, insulation, masonry.

   In quick tests 1-95, subhourly not hourly did not add grossly to simulation time
   and did change results even at tc=5 hours.
*/

// criterion for SFMODEL=AUTO decisions
#define TC_SHMASS        (1.f/Top.tp_nSubSteps)	// below this use quick model, above use subhourly

// time constants below which to issue warnings
#define TC_SHMASS_WARN   (1.f/Top.tp_nSubSteps)
#define TC_HMASS_WARN    1.f

#endif
//===========================================================================
RC topSf1()

// do all surfaces, doors, and windows, part 1

// checks/defaults/derived items, some used elsewhere

// uses/updates: file-global msiLast,

// serious errors that should stop caller return non-RCOK:
//     eg errors that might make many later spurious messages,
//     or missing RQD data (might cause FPE on UNSET in a later fcn),
//     or bad data later copied without check, as from surface to door.
//  (Any error message prevents RUN via error count in rmkerr.cpp;
//   RCOK returned if ok to proceed to next subfcn of topCkf() )
{
	RC rc=RCOK;
	msiLast = 0;	// init last used delayed model surf MsR subscr
					//  for use in sgdists & topSf2(). cncult3.

// topSf1: Loop over surfaces (walls/floors/ceilings), doors, and windows
	SFI *sf;				// surface OR DOOR OR WINDOW being processed
	for (int ib=0; SFIAncs[ ib]; ib++)
	{	RLUP( *SFIAncs[ ib], sf)
			rc |= sf->sf_TopSf1();		// do each, accum rc and keep going
	}
	return rc;
}		// ::topSf1
//-----------------------------------------------------------------------------
RC SFI::sf_TopSf1()
{
	RC rc = RCOK;
	RC rc1 = 0;

#if defined( _DEBUG)
	Validate();
#endif

	UCH* fs = fStat();  	// record's field status byte array
	ZNR *zp;

	SFCLASS sfc = SFCLASS( sfClass);	// sfcSURF, sfcWINDOW, sfcDOOR

	// sf_TopSf1: surfaces only

	if (sfc==sfcSURF)
	{
		// surface: default ground reflectance from Top, 5-95.
		if (!IsSet( SFX( GRNDREFL)))	// if sfGrndRefl not given for this surface
			VD x.grndRefl = VD Top.grndRefl;  	// get global value from Top object. m-h variable. default .2.

		// surface: get/check zone
		if (ckOwnRefPt( &ZrB, this, (record **)&zp, &rc) )
			return rc;					// bad ref (msg issued). go to next surface.
	}
	else		// is door or window
	{
		// topSf1: doors & windows only

		// door/window: get/check owning surface and zone
#if 0 && defined( _DEBUG)
0		if (strMatch( name, "North_Door"))
0			printf( "Hit\n");
#endif
		SFI* ownSf;
		if (ckOwnRefPt( &SfiB, this, (record **)&ownSf, &rc) )
			return rc;							// bad ref (msg issued, rc set, no run).
		if (::ckRefPt( &ZrB, ownSf, ownSf->ownTi, "", this, (record **)&zp, &rc) )
			/* ? 1-92 says "bad zone index in surface 'x' of door 'y'
				   (reversing ownership relation in message) */
			return rc;							// bad ref (msg issued, rc set, no run).

		// topSf1: door/window: copy or default some members from owning surface.
		//  These were checked as part of surface; not rechecked; fld stat not copied.

		x.tilt = ownSf->x.tilt;
		x.azm = ownSf->x.azm;
		x.sfExCnd = ownSf->x.sfExCnd;		// exterior condition
		VD x.sfExT = VD ownSf->x.sfExT;		// exterior temp for SPECT. move as NANDAT: expr handle possible.
		x.sfAdjZi = ownSf->x.sfAdjZi;		// adjacent zone for ADJZN.

		if (!IsSet( SFX( UX)))	// exterior surface H
			x.uX = ownSf->x.uX;			// default to owning surface's: drExH or wnExH = sfExH

		if (!IsSet( SFX( GRNDREFL)))	// if wnGrndRefl not given for window, or if door (not inputtable)
			VD x.grndRefl = VD ownSf->x.grndRefl;  	// default ground reflectivity from owning surface. m-h variable.

		// inherit convective model
		if (!IsSet( IsSet( SFXI( HCMODEL))))
			x.xs_sbcI.sb_hcModel = ownSf->x.xs_sbcI.sb_hcModel;
		if (!IsSet( IsSet( SFXO( HCMODEL))))
			x.xs_sbcO.sb_hcModel = ownSf->x.xs_sbcO.sb_hcModel;

		if (sfc==sfcDOOR)				// note wnIhH now defl'd to 10000 (near-0 R) by CULT, 10-28-92. BRUCEDFL.
		{
			if ( !IsSet( SFX( UI)))	// interior surface H
				x.uI = ownSf->x.uI;			// default to owning surface's: drInH = sfInH
			if ( !IsSet( SFXO( ABSSLR)))	// exterior absorptivity
				VD x.xs_AbsSlr( 1) = VD ownSf->x.xs_AbsSlr( 1);	// default to owning surface's: drExAbs = sfExAbs
		}														// V (cnglob.h): don't move as float: (monthly-hourly) expr allowed. 4-92.


		else if (sfc==sfcWINDOW)
		{	// sf_topSf1: window only
			// disallow in interior wall, rob 2-95: we don't have appropriate radiative modelling (conductive part probobly ok).
			// note ok & receives sun in adiabatic or specT wall.
			if (ownSf->x.sfExCnd==C_EXCNDCH_ADJZN)
				oer( (char *)MH_S0530);				// "Window in interior wall not allowed"

			// always use conductance model for windows
			x.xs_model = C_SFMODELCH_QUICK;

			// default area to hite*width; apply multiplier
			rc1 = CkSet( SFI_HEIGHT) | CkSet( SFI_WIDTH);	// non-RCOK if RQD ht or wid unset (msgd at end of win object)
			if (rc1==RCOK)						// if ht & wid set (else FPE!)
				// window area may be given explicitly, to allow using old test files where area != height * width. 2-91.
				if ( !IsSet( SFI_SFAREA) )			// if area not given
					sfArea = sf_height * sf_width;			// compute window area
			sfArea *= mult;				// multiplier (dfl 1.0) for giving multiple similar windows in surf
			// sfArea is copied to x.area below
		}

		// topSf1: door/window: subtract area from area of owning surface

		if (sfc==sfcDOOR)			// rc1 set just above for window
			rc1 = CkSet( SFI_SFAREA);	// verify RDQ member given

		// door/win area: sfArea
		// surf net area: ownSf->x.area
		// surf gross area: ownSf->sfArea
		if (rc1==RCOK)				// insurance
		{
			if (ownSf->x.xs_area >= sfArea)		// if surface large enuf
				ownSf->x.xs_area -= sfArea;		// net child out of surf area
			else if (ownSf->x.xs_area > 0.f)  			// if no msg yet (input must be > 0) (also unset ownSf->sfArea protection)
			{	// child bigger than parent
				oer(
					ownSf->x.xs_area >= ownSf->sfArea	// if nothing subtracted yet
						?  (char *)MH_S0511		// "Area of %s (%g) is greater than \n    area of %s's surface%s (%g)"
						:  (char *)MH_S0512,	// "Area of %s (%g) is greater than \n"
												// "    remaining area of %s's surface%s \n"
												//    (%g after subtraction of previous door areas)"
						classObjTx(), sfArea,
						(char *)b->what, quifnn( ownSf->name),	// quote if not ""
						ownSf->x.xs_area );
				ownSf->x.xs_area = 0.f;				// use 0 / no more msgs
			}
		}
	}	// if (sfc==sfcSURF) ... else ...


	// topSf1: recall object check function

	/* wnStarCkf / drStarCkf / sfStarCkf was called at end of object, to get msgs near source.
	   But in defType it suppresses later-correctible messages and there is no check function call at use
	   if surface is nested in zone type and is not ALTERed at type use. */
	/* [May cause duplicate messages -- would like a "checked" flag or logic to Ckf all subobjects at end of type-based object.
	   12-91 added FsERR flag, but its field-specific. No, don't get here if preceding error -- topCkf not called! 12-94.] */

	rc1 = (sfc==sfcWINDOW ? wnStarCkf : sfc==sfcDOOR ? drStarCkf : sfStarCkf)( NULL, this, NULL, NULL);
	// fcns in cncult.cpp. Only use arg 2. defTyping=0 now.
	rc |= rc1;  				// if msg(s), stop topCkf when we return.
	if (rc1)     				// if msg(s) occurred,
		return rc;				//   skip to next surf/door & finish this fcn.

	// topSf1: surface/door/window: initialization to redo if RUN again w same data (---> can't do in sfStarCkf)

	// move .sfArea / init surface net area to gross
	if (sfc==sfcWINDOW			// window: not nec user-given
	 || CkSet( SFI_SFAREA)==RCOK)	// verify RQD mbr given (FPE protection)
		x.xs_area = sfArea;	// move area to where used at run. For surface, b4 XSURF is added to zn,
							// window and door areas are subtracted from .x.area,
    						// by code above in this fcn, on later call to this fcn.

	// init SGDIST count
	x.nsgdist = 0;			// delete any sgdists for surface, eg left from prior run

	// topSf1: surf/door/window: checks/defaults/derived values requiring other objects (--> can't do in sfStarCkf)

	// set or default view factors re diffuse radiation per tilt. Used in cgsolar.cpp. 5-95.

	double cosTilt = cos( x.tilt);		// do once: might save time. tilt set above in topSf1 for door,window.
	if (!IsSet( SFX( VFSKYDF)))			// unless explicitly input (possible for window only: wnVfSkyDf)
		x.vfSkyDf = .5 + .5*cosTilt;	//   .5 if vertical, 1.0 if roof, 0 if floor
	if (!IsSet( SFX( VFGRNDDF)))		// unless explicitly input (possible for window only: wnVfGrndDf)
		x.vfGrndDf = .5 - .5*cosTilt;	//   .5 if vertical, 0 if roof, 1.0 if floor

	// long-wave view factors, no input yet (added 10-10)
	x.vfSkyLW  = .5 + .5*cosTilt;
	x.vfGrndLW = .5 - .5*cosTilt;

	// surface: default roughness factor
	if (!IsSet( SFX( RF)))
	{	x.xs_Rf = sfc == sfcWINDOW ? 1.f
		    : x.xs_sbcO.sb_hcModel == C_CONVMODELCH_WINKELMANN ? 1.66f
		    : 2.17f;
		// re alternative outside convection models
		// (add input as needed)
		// From Niles CZM documentation and Walton 1981
		//		Roughness Index		Rf		Example
		//		1 (very rough)		2.17	Stucco
		//		2 (rough)			1.67	Brick
		//		3 (medium rough)	1.52	Concrete
		//		4 (Medium smooth)	1.13	Clear pine
		//		5 (Smooth)			1.11	Smooth plaster
		//		6 (Very Smooth)		1		Glass
	}

	// surface: conditionally default sfExH to global default in 'Top'

	if (sfc==sfcSURF)  				// if surface (door/win copied from this after dflt)
		if ( !IsSet( SFX( UX))		// if no sfExH entered
		 && ( x.sfExCnd==C_EXCNDCH_AMBIENT	// if ext cond ambient (dflt)
		   || x.sfExCnd==C_EXCNDCH_SPECT ) )	/* or specified Temp (for zone, 1.5 is coded in table;
						   for adiabatic, extH unneeded,rite??)*/
		{
			x.uX = Topi.dflExH;			// use Top global default
		}

	// topSf1: window: check glazing type (gt) and conditionally get wnU, wnFMult, SC's from glazing type (or old-model inputs)

	GT *gt = NULL;
	if (sfc==sfcWINDOW)
	{
		rc1 = RCOK;					// local code for several error checks
		SI gtSet = IsSet( SFX( GTI));
		if (gtSet)					// if glazing type given (always FALSE for non-Window)
		{
			if (ckRefPt( &GtiB, x.gti, "wnGt", NULL, (record **)&gt, &rc) )	// check reference / point to it
				return rc;								// bad ref, msg issued, rc set, no run.

			// default SMSO/C (SHGC Multiplier for Shades Open/Closed).
			// Inputs are wnSMSO/C (placed in x.sco/c) else gtSMSO/C.
			// Values are multiplied at runtime (cgsolar.cpp) by common part (normal transmissivity gtSHGC). */
			if (!IsSet( SFX( SCO))) 	// if x.sco not given (as wnSMSO) for window
				VD x.sco = VD gt->gtSMSO;  		// use gt shades-Open SMS, m-h variable, default 1.0.
			if (!IsSet( SFX( SCC))) 	// if x.scc not given (as wnSMSC) for window
				if (IsSet( SFX( SCO)))	// if x.sco given (as wnSMSO) for window
					VD x.scc = VD x.sco;  		// use window shades-open value, checked/defaulted just above
			// note that this makes shades inoperable even if gtSMSC != gtSMSO.
				else
					VD x.scc = VD gt->gtSMSC;  	// use gt shades-Closed SMS. checked/defaulted in cncult2:topGt(). m-h.

			// default window values from GT
			if (!IsSet( SFX( FMULT))) 	// if wnFMult (frame/mullion multplier) not given for window
				x.xs_fMult = gt->gtFMult;				// get from gt. defaulted there. Combine with m-h var sco/scc at runtime.
			if (!IsSet( SFX( DIRTLOSS))) 	// dirtloss
				x.xs_dirtLoss = gt->gtDirtLoss;
			if (!IsSet( SFX( INSHD)))
				x.xs_inShd = gt->gtInShd;
			if (!IsSet( SFX( EXSHD)))
				x.xs_exShd = gt->gtExShd;
			if (!IsSet( SFX( FENMODEL)))
				x.xs_fenModel = gt->gtFenModel;

			if (!IsSet( SFX( UNFRC)))
			{	if (gt->IsSet( GT_GTUNFRC))
				{	x.xs_UNFRC = gt->gtUNFRC;
					fs[ SFX( UNFRC)] |= FsSET;
				}
			}

			if (!IsSet( SFX( SHGC)))
			{	if (gt->IsSet( GT_GTSHGC))
				{	x.xs_SHGC = gt->gtSHGC;
					fs[ SFX( SHGC)] |= FsSET;
				}
			}

			if (!IsSet( SFX( NGLZ)))
			{	if (gt->IsSet( GT_GTNGLZ))
				{	x.xs_NGlz = gt->gtNGlz;
					fs[ SFX( NGLZ)] |= FsSET;
				}
			}

			if (!IsSet( SFI_SFU))		// if wnU not given in window
				if (gt->IsSet( GT_GTU))		// if gtU given in glazing type
				{	sfU = gt->gtU;			// use u-value given in glazing type
					fs[ SFI_SFU] |= FsSET;	// say sfU set, so it will be used below. 2-17-95.
				}
				else
					rc1 = oer( (char *)MH_S0531, gt->name);  	// "No U-value given: neither wnU nor glazeType '%s' gtU given"
		}
		else
		{	// no wnGT
			if (IsSet( SFX(FENMODEL)) && !x.xs_IsASHWAT())
				rc1 = oer( "No GT, fenModel must be ASHWAT");
			else
			{	x.xs_fenModel = C_FENMODELCH_ASHWAT;
				// x.sco defaults to 1
				if (!IsSet( SFX( SCC))) 	// if x.scc not given (as wnSMSC) for window
					VD x.scc = VD x.sco;  		// use window shades-open value, checked/defaulted just above
			}
		}
		if (x.xs_IsASHWAT())
		{	const char* when = "when wnModel=ASHWAT";
			rc1 = requireN( when, SFX( UNFRC), SFX( SHGC), SFX( NGLZ), 0);
		}

		// error if window shades-closed shading coefficient > shades-open value

		if ( rc1==RCOK				// if no message just above for this window
		 &&  !ISNANDLE(x.scc)			// if expressions given, don't check: FPE's!!!
		 &&  !ISNANDLE(x.sco)			// .. (also checked at runtime in cgsolar.cpp)
		 &&  x.scc > x.sco )			// if shades closed admit more light than shades open
		{	oer(
					  (char *)MH_S0532,		/* "wnSMSC (%g) > wnSMSO (%g):\n"
					   "    shades-closed solar heat gain coefficient multiplier must be <= \n"
					   "    shades-open solar heat gain coefficient multiplier." */
				x.scc, x.sco );	// note defaults gtSMSC & gtSMSO were checked in topGt()
			// rc1 *not* set, can safely continue after scc > sco error
		}

		rc |= rc1;					// merge error code into global
		if (rc1)					// after several checks for window, if error,
			return rc;					// done this window: fall thru might FPE on a NAN
	}		// end of "if window"

	// topSf1: check / access construction if given

	SI conSet = fs[ SFI_SFCON] & FsSET; 		// nz if construction spec'd (always false for window)
	CON *con = nullptr;
	if (conSet)
		if (ckRefPt( &ConiB, sfCon, "sfCon", NULL, (record **)&con, &rc) )
			return rc;			// bad ref (msg issued, rc set, no run).

	// topSf1: get uval w/o films ('construction uval'??) from input or construction

	if (conSet)				// if constr spec'd (false if window)
		x.uC = con->conU;	// get uval from construction
	else					// sfCon not given --> sfU given (or obtained from window glazetype above).
		if (IsSet( SFI_SFU))	// protect against FPE if unset (unset sfU already msg'd: srStarCkf/drStarCkf
								// checked for either sfCon or sfU given; handled above or in wnStarCkf for window)
			x.uC = sfU;			// get uval from input member (or as set above for Window)

	// topSf1: now we can set derived conductances (for quick model; [non-intmasses])

	float uCX = seriesU( x.uC, x.uX);	// cons & ext film. used??
	x.xs_uval = seriesU( x.uI, uCX);	// all 3, to traditional uval mbr used by cn, 2-91.

	x.xs_SetUNom();		// nominal U-factor (with combined surface coefficients)
						//   (documentation only)

#if 0 && defined( _DEBUG)
x	if (strMatch( name, "North_Door"))
x		printf( "Hit\n");
#endif
	// topSf1: surface/door: determine time constant of construction

	float tc = 0.f;
	if (conSet)			// if construction spec'd (not window)
	{
		// time constant to consider is that of surf's entire thermal mass
		// coupled to zone by surface film: hc/h: if this is too short,
		// mass model (iterated once per hour) works poorly.

		tc = con->hc 		// heat capacity per sq ft of construction
			/ x.uI;	// interior surface film conductance: LMGZ, default 1.5: can't be 0 or UNSET.
					// [ >> ?? for intmass2 use BOTH films in parallel? use smaller? ]
	}

	// topSf1: surface/door/window: determine surface model
	switch (x.xs_model)
	{
	default:
		break;				// for lint; use quick (preset) if occurs.

	case C_SFMODELCH_QUICK:
		break;		// quick requested, use it (preset).  always used for windows.
		// [not allowed for intmass, checked in sfStarCkf.]

	case C_SFMODELCH_AUTO:			// automatic choice (default for surfaces and doors)
		if (!conSet)          break;				// if no construction, must use quick (preset)
		if (con->nLr <= 0)    break;				// if con has no layers (just uval), must use quick
		if (tc <= TC_SHMASS)  break;				// if time const short (constant above), use quick
		//if decision gets more complicated, change to fall thru to share code with AUTO.
		x.xs_modelr = C_SFMODELCH_DELAYED_SUBHOUR;     		// use subhourly calculated mass. no auto hour case.
		break;

	case C_SFMODELCH_DELAYED:
	case C_SFMODELCH_DELAYED_SUBHOUR:
	case C_SFMODELCH_DELAYED_HOUR:
	case C_SFMODELCH_FD:
	case C_SFMODELCH_KIVA:
		// common checks
		if (!conSet)				// if no constr specified
		{	// checked in sfStarCkf; redundant insurance here.
			rc |= oer( (char *)MH_S0513,	// "Can't use delayed (massive) sfModel=%s without giving sfCon"
						getChoiTx( SFX( MODEL)));
			break;							// (is preset to quick)
		}
		if (con->nLr==0)				// can't do delayed with uval only
		{	rc |= oer( (char *)MH_S0514,	// "delayed (massive) sfModel=%s selected\n"
											// "    but surface's construction, '%s', has no layers"
			getChoiTx( SFX( MODEL)),
			con->name );
			break;					// (is preset to quick)
		}
		if (x.xs_modelr == C_SFMODELCH_KIVA)	// if model is already set to Kiva, break
		{
			break;
		}
		switch (x.xs_model)	// nested switch for differences
		{
			//case C_SFMODELCH_AUTO:			if it gets here
			case C_SFMODELCH_DELAYED:   		// delayed (massive) requested. Mainly for old files 1-95.
			// Choose hour or subhr like auto does, ie (currently) use subhourly.
			//if (tc > TC_HMASS) goto hour case if desired

			case C_SFMODELCH_DELAYED_SUBHOUR:
				if (tc < TC_SHMASS_WARN)		// warn if time const too small for computation interval - then do anyway
					oWarn( (char *)MH_S0533,	/* "Time constant %g probably too short for delayed (massive) sfModel.\n"
						   "    Use of sfModel=Delayed, Delayed_Subhour, or Delayed_Hour \n"
						   "        is NOT recommened when time constant is < %g.\n"
						   "    Results may be strange. \n"
						   "    (Time constant = constr heat cap (%g) / sfInH (%g) )." */
						tc, TC_SHMASS_WARN, con->hc, x.uI );
				x.xs_modelr = C_SFMODELCH_DELAYED_SUBHOUR;
				break;

			case C_SFMODELCH_DELAYED_HOUR:
				if (tc < TC_HMASS_WARN)		// warn if time const too small for computation interval - then do anyway
					oWarn( (char *)MH_S0534,	/* "Time constant %g probably too short for sfModel=Delayed_Hour.\n"
						   "    Use of sfModel=Delayed_Hour \n"
						   "        is NOT recommened when time constant is < %g.\n"
						   "    Results may be strange. \n"
						   "    (Time constant = constr heat cap (%g) / sfInH (%g) )." */
						tc, TC_HMASS_WARN, con->hc, x.uI );
				x.xs_modelr = C_SFMODELCH_DELAYED_HOUR;
				break;

			case C_SFMODELCH_FD:
				x.xs_modelr = x.xs_model;		// pass FD through
				break;
		}
		break;
	}

	// topSf1: if delayed (massive) then ...
	if (SFI::sf_IsDelayed( x.xs_modelr))
	{
		// if surface model delayed, assign mass subscr now, for use eg in sgdists. (topSf2 makes MsR entry, when net area known.)
		sf_msi = ++msiLast;			// next value per global, init in topCkf()
	}

	// topSf1: be sure sfAdjZi is 0 if no zone
	if (x.sfExCnd != C_EXCNDCH_ADJZN)
		x.sfAdjZi = 0;			// insurance: be sure 0 if no zone so could skip sfExCnd test at runtime, 2-95.

	// topSf1: init to no (window) shading: needed if reRUN.  see check in topSh().
	x.iwshad = 0;			// 0 subscript of shading RUN record

	return rc;

}		// SFI::sf_TopSf1
//===========================================================================
RC FC topMsCon()

// massive construction stuff between topSf1 and topSf2:
// clear mass and mass type run records and allocate needed number of each (done here as called once only & sizes now known);
// create mass types for constructions used for delayed model surfaces
// uses: msiLast: # masses needed
{
	RC rc = RCOK;		// rc is used in E macro

// clear masses run records.  Insurance.  Masses are created in topSf2.

	// traditional code clears after run; could remove this duplication.
	MSRAT* mse;
	RLUP( MsR, mse)				// loop over any masses (cnglob.h macro)
		mse->DelSubObjects();
	CSE_E( MsR.al( msiLast, WRN) )	// delete old MsR entries, alloc to needed size now for min fragmentation
	// E macro: if returns bad, return bad to caller.
	// CAUTION: may not errCount++; be sure error propogated so cul.cuf will errCount++.
	// masses are not owned (no .ownTI 1-92). leave .ownB 0.

	return rc;				// error returns above in each E macro
}		// topMsCon
//=============================================================================

/////////////////////////////////////////////////////////////////////////////
// class MSRAT: some implementation
//    see also mspak.cpp
/////////////////////////////////////////////////////////////////////////////
RC MSRAT::ms_Make(				// fill MSRAT for SFI + CON
	SFI* sf,				// surface input data
	const CON* con,			// associated construction
	int options /*=0*/)		// options
							//   1: fill for framing path (unimplemented, 8-10)
{
	RC rc = RCOK;
	ms_isFD = SFI::sf_IsFD( sf->x.xs_modelr);
	ms_sfi = sf->ss;			// associated surface subscript
	ms_sfClass = sf->sfClass;	// associated surface class (sfcSURF vs sfcDOOR)
								//   (re ms_GetSFI())

	// create mass model of required type
	if (ms_isFD)
		ms_pMM = new MASSFD( this, sf->x.xs_modelr);
	else
		ms_pMM = new MASSMATRIX( this, sf->x.xs_modelr);

	AR_MASSLAYER arML;
	rc = con->cn_Layers( arML, options);	// extract physical layers for construction
	if (!rc)
	{	if (ms_isFD)
		{	if (sf->sf_IsWallBG())
				rc = sf->sf_BGWallSetup( arML);
			else if (sf->sf_IsFloorBG())
				rc = sf->sf_BGFloorSetup( arML);
			rc |= ms_pMM->mm_FromLayersFD( arML);
		}
		else
			rc = ms_pMM->mm_FromLayers( arML);
	}
#if defined( _DEBUG)
	if (!rc && ms_isFD)
	{	// check our work!
		// derive properties from final layers, check against source
		// skip below grade: model modifies layers, will usually mismatch
		if (!sf->sf_IsBG())
		{	double cFactor, hc;
			ms_pMM->mm_GetProperties( cFactor, hc);
			if (frDiff( double( con->conU), cFactor) > .0001
			 || frDiff( double( con->hc), hc) > .0001)
				rc |= sf->oWarn( "MASSMODEL properties mismatch");
		}
	}
#endif
	if (rc)
		sf->oer( (char *)rc);	// report error

	// fill mass record (even if not RCOK)
	strcpy( name, sf->name);	// name
#if 0 && defined( _DEBUG)
x	if (strMatch( name, "North_Door"))
x		printf( "Hit\n");
#endif
	ms_area = sf->x.xs_area;			// net area
	isSubhrly = SFI::sf_IsSubhrly( sf->x.xs_modelr);	// TRUE if to be simulated subhourly

	ms_UNom = sf->x.xs_UNom;		// U-factor: construction/layers + nominal surface conductance
									// Note: ms_UNom also set in xs_SetUNom() to ensure consistency
	ms_tc = con->hc/sf->x.uI;		// time constant, recomputed & stored here for reporting (cgresult.cpp) added 1-95

	inside.bc_Setup( ss, MSBCZONE, 0);		// 0 = doing inside
	int adjTy =					// adjacency type per surface, translate codes:
			  sf->x.sfExCnd==C_EXCNDCH_ADJZN      ?  MSBCZONE
			: sf->x.sfExCnd==C_EXCNDCH_AMBIENT    ?  MSBCAMBIENT
			: sf->x.sfExCnd==C_EXCNDCH_GROUND     ?  MSBCGROUND
			: sf->x.sfExCnd==C_EXCNDCH_SPECT      ?  MSBCSPECT
			: sf->x.sfExCnd==C_EXCNDCH_ADIABATIC  ?  MSBCADIABATIC
			:    (rc |= sf->oer( (char *)MH_S0526, sf->x.sfExCnd));			// "Bad sfExCnd %d (cnuclt.cpp:makMs())"
	outside.bc_Setup( ss, adjTy, 1);		// 1 = doing outside

	for (int si=0; si<2; si++)
	{	ZNR* zp = ms_GetZone( si);
		if (zp && !zp->zn_IsSFModelSupported( sf->x.xs_modelr))
			rc |= sf->oer( "conduction model '%s' not compatible with znModel '%s'",
					sf->getChoiTx( SFI_X+XSURF_MODELR),
					zp->getChoiTx( ZNR_I+ZNISUB_ZNMODEL));
	}

	if (!rc)
	{	if (ms_isFD)
			rc = ((MASSFD *)ms_pMM)->mf_Setup();
		else
		{	// init mass matrix used at run time
			rc = ((MASSMATRIX *)ms_pMM)->mx_Setup(
				isSubhrly ? 1.f/Top.tp_nSubSteps : 1.f,	// time step: 1 subhour (as hour fraction), or 1 hour.
				inside.bc_h,		// surface conductances from surface & msty, combined above
				outside.bc_h);			// ..
		}
	}
	if (rc)			// if failed (already msg'd)
		rc = sf->oer( (char *)MH_S0527 );	// "ms_makMs: mass model setup error (above)"
		// continue here, but perlc stopped run & rc ret will stop caller.

// if outside of mass is exposed, add XSURF to inside zone to rcv insolation
	XSRAT* xr;
	rc=cnuCompAdd( &sf->x, sf->name, inside.bc_zi, &sf->xi, &xr);	// add XSRAT to mass's inside zone, ret ptr.
	if (!rc)	// if added ok: insurance
	{	// mass wall XSURF differences from SFI.x (do not change input (esp .x.xs_ty) in case surf quick-modelled on later run)
		xr->x.xs_ty = CTMXWALL;	// XSURF type: mass ext wall
		xr->x.xs_uval=outside.bc_h;	// uval: from outside air to outer mass layer,
       							// for cgsolar.cpp re solar air/mass split. NOT used re zone .ua in cse.
								// Note that h = surface film uX + rsurf, and sun strikes between surface film and rsurf.
								// If rsurfo is 0 (no exterior carpet), all insolation goes to mass outer layer.
		xr->x.xs_msi = ss;		// mass subscript to XSURF eg for cgsolar.cpp access 2-95.
		xr->x.nsgdist = 0;   	// delete any sgdists: believed redundant 2-95.
		ms_xri = xr->ss;		// XSRAT link in mass
#if 0 && defined( DEBUGDUMP)
x		if (DbDo( dbdCONSTANTS))
x			xr->x.DbDump();
#endif
	}

	return rc;
}		// MSRAT::ms_Make
//-----------------------------------------------------------------------------
const SFI* MSRAT::ms_GetSFI() const
{	anc<SFI>* pB = ms_sfClass == sfcSURF ? &SfiB
                 : ms_sfClass == sfcDOOR ? &DriB
                 :                         NULL;
	SFI* pS;
	if (!pB)
	{	errCrit( WRN, "MSRAT '%s': bad ms_sfClass %d", name, ms_sfClass);
		pS = NULL;
	}
	else
		pS = pB->GetAt( ms_sfi);
	return pS;
}		// MSRAT::ms_GetSFI
//---------------------------------------------------------------------------
XSRAT* MSRAT::ms_GetXSRAT()
{
	return XsB.GetAt( ms_xri);
}		// MSRAT::ms_GetXSRAT
//---------------------------------------------------------------------------
const XSRAT* MSRAT::ms_GetXSRAT() const
{
	return XsB.GetAt( ms_xri);
}		// MSRAT::ms_GetXSRAT
//-----------------------------------------------------------------------------
RC CON::cn_Layers(					// extract array of physical layers for construction
	AR_MASSLAYER& arML,			// returned: array of layers
	int options /*=0*/) const	// options
								//   1: extract layers for framing path
								//      (TODO: not implemented 8-10)
// returns RCOK (or future error RCs), not msg'd here
{
	arML.clear();
	int iL = 0;		// layer idx
	LR* pLR;
	MASSLAYER tML;
	RLUP( LriB, pLR)			// loop over layers records -- all CONs
	{	if (pLR->ownTi==ss)		// if a layer of given con
		{	tML.ml_Set( iL++, pLR, options);
			arML.push_back( tML);
		}
	}
	return RCOK;
}		// CON::cn_Layers
//=============================================================================
RC FC topSg()		// SGDIST processing at RUN

// returns non-RCOK only on serious error, to terminate topCkf().
{
	RC rc=RCOK;

	// user-entered solar gain distributions loop:
	//		verify that target surface is in zone and is delayed model;
	//		determine target side of surface;
	//		put sgdists in their windows.
	//	must be between window basic sgdists loop and windows xsurfs loop
	SGI* sg;
	RLUP( SgiB, sg)			// loop SGI records
	{
		UCH* fs = (UCH *)sg + SgiB.sOff;		// point sg's field status byte array

		// for sgdist access window, surface, zone name
		SFI* gz;		// sgdist's window
		if (ckOwnRefPt( &GziB, sg, (record **)&gz, &rc) )
			continue;				// bad ref (msg issued, rc set, no run).

		// gz->ownTi was checked in topSf1()
		SFI* gzSf = SfiB.p + gz->ownTi;   		// sgdist's window's surface
		// gzSf->ownTi was checked in topSf1()
		TI gzZi = gzSf->ownTi;   		// window's zone subscript
		ZNR* zp = ZrB.p + gzZi;	 	// window's zone pointer

		// solar gains: check for too many for gz.  Duplicates sgStarPrf check (all such checks should be redone).

		if (gz->x.nsgdist >= HSMXSGDIST)   			// =8, 2-95 (cndefns.h)
		{
			gz->oer( (char *)MH_S0517, INT(HSMXSGDIST) );    	// "More than %d SGDISTS for same window"
			continue;						// next sg
		}

		// solar gain: check/access surface
		SFI* targSf;		// target surface;
		if (ckRefPt( &SfiB, sg, sg->d.sd_targTi, "", zp, (record **)&targSf, &rc) )	// (SgiB records are unowned; say "of zone 'x'" in errMsg)
			continue;								// error. msg'd, run stopped, rc set.

		// solar gain: verify specified side in zone / default to side in zone

		// (in zone requirement intended to be later (2-91) removed;
		// believe present 2-91 simulator won't handle out-of-zone target unless we make double entry
		// and adjust entry [1] to redirect zone check totals also.)
		// 2-95: SGDIST out of zone properly controlled by shades in zone
		// containing window would still be difficult in cgsolar.cpp.

		BOO exIsInZn = 		// nz if surface outside is in window's zone
			targSf->x.sfExCnd==C_EXCNDCH_ADJZN	// if ext cond zone, not specT,ambient,adiabatic.
		 && targSf->x.sfAdjZi==gzZi;   		//   and the zone is window's

		char* s = nullptr;					// set non-NULL iff error
		BOO toOutside = 0;
		if (fs[SGI_SGSIDE] & FsSET)		// if sgSide given in input
		{
			if (sg->sgSide==C_SIDECH_EXTERIOR)	// if 'exterior' specified
			{
				if (exIsInZn)			// if surf outside in zone
					toOutside = TRUE;	// surface "outside" rcvs gain
				else					// else targetting error
					s = "Outside of ";
			}
			else						// must be 'interior'
			{
				if (targSf->ownTi==gzZi)       	// if inside is in zone
					toOutside = FALSE;		// surface "inside" rcvs gain
				else
					s = "Inside of ";		// else targetting error
			}
		}
		else	// sgSide not given, default to side in zone, interior if both
		{
			if (targSf->ownTi==gzZi) 		// if surf "inside" is in zn
				toOutside = FALSE;			// inside receives solar gain
			else if (exIsInZn)				// if outside in window's zn (above)
				toOutside = TRUE;			// "outside" rcvs gain.
			else
				s = "Both sides of ";		// error, neither side in zone
		}
		if (s)			// if error
		{	sg->oer(				// message to scrn, and disable RUN.
				(char *)MH_S0518,	/* "%ssurface '%s' not in zone '%s'. \n"
										"    Can't target solar gain to surface not in window's zone." */
				s, targSf->name, zp->name );
			continue;				// next sg
		}


		// solar gain: determine targTy and targTi
		// (cgsolar.cpp later determines target ptr from .targTi & .targTy.)
		int targTy;			// SGDIST target type
		int targTi;			// SGDIST target subscript
		targTy = toOutside ? SGDTTSURFO : SGDTTSURFI; 	// say "surface" and which side
		targTi = targSf->ss;     						// which surface. SFI subscript replaced with XsB subscript
														// ... by topSg2 (subscripts not known yet)
		// may require change if target surface side can be in another zone.
		// solar gain: if target not delayed model, warn & direct to zone air

		if (!SFI::sf_IsDelayed( targSf->x.xs_modelr) && !SFI::sf_IsKiva(targSf->x.xs_modelr))
		{
			sg->oWarn( (char *)MH_S0519,	// "Target surface '%s' is not delayed model.\n"
											// "    Solar gain being directed to zone '%s' air."
				targSf->name, zp->name );
			// need do nothing to redirect the gain since target is in zone.
			// CHANGES REQUIRED when target surface side can be in another zone.
			continue;					// proceed to next sgdist. run NOT stopped.
		}

		// solar gain: default shades closed target fraction to shades open

		if (!(fs[SGI_D + SGDIST_FSC] & FsSET))		// if sd_FSC not entered by user
			VD sg->d.sd_FSC = VD sg->d.sd_FSO;		// copy sd_FSO.  V: *(void**)&: don't copy possible NANDLE as a float

		// solar gain: finally, add sgdist from window's XSURF to spec'd mass or quick surface side

		cnuSgDist( &gz->x, targTy, targTi, VD sg->d.sd_FSO, VD sg->d.sd_FSC);	// V: clglob.h: *(void**)&

		// note: explicitly distributed solar gain is subtracted from that automatically distributed to zone's
		// air and surfaces at run time, in cgsolar.cpp (can't do here as .sd_FSO/C may contain live exprs). 2-95.
	}	// RLUP( ,sg)
	return rc;
}		// topSg
//===========================================================================
RC FC topSh()		// SHADE processing at RUN
// uses window tilt (set in topSf1)
// sets window.x.iwshad (used in topSf2)

// returns non-RCOK only on serious error, to terminate topCkf().
{
	WSHADRAT *sh, *shr;
	SFI *gz;
	RC rc=RCOK;

	RLUP( ShiB, sh)			// loop shading input RAT records
	{
		//fs = (UCH *)sh + ShiB.sOff;		// SHADE fld stat byte array

		// for SHADE, access owning window and surface

		if (ckOwnRefPt( &GziB, sh, (record **)&gz, &rc) )
			continue;					// bad ref (msg issued, rc set, no run).
		// gz->ownTi was checked above in topSf1().

		// get hite and width from window
		// gz->height, width ckSet'd in topSf1.
		sh->wHeight = gz->sf_height;
		sh->wWidth = gz->sf_width;

		// check for vertical surface (assumed by shading compute fcn).

		if ( !fAboutEqual( DEG(gz->x.tilt), (float)90.f ) )	// surf tilt-->gz in topSf1()
		{
			sh->oer( (char *)MH_S0521,			// "sfTilt is %g.  Shaded window must be in vertical surface."
				DEG(gz->x.tilt) );
			continue;					// skip this shade (and msg prevented run)
		}

		// ignore SHADEs with no shading

		/* we don't do this for 0-area surfaces or windows, but easy here
		   because 1) run subscripts do not match input; 2) 0 depths must
		   be allowed as there are 3 things in one SHADE. 2-91. */

		if ( !ISNANDLE(sh->ohDepth) 	// don't test expressions (floating
		  && sh->ohDepth==0.f 		// point err wd terminate program!!)
		  && !ISNANDLE(sh->lfDepth) 	// just assume not all always 0.
		  && sh->lfDepth==0.f
		  && !ISNANDLE(sh->rfDepth)
		  && sh->rfDepth==0.f )
			continue;			// if no shading, skip this SHADE

		// check for only one (effective) shade per window

		/* >>> otta also do such a check during input (or with ONLY1)
		   to get msg at line of start of duplicate SHADE 2-91 */

		if (gz->x.iwshad)					// if window already has a SHADE
		{
			sh->oer( (char *)MH_S0522,				/* "Window '%s' is already shaded by shade '%s'. \n"
								   "    Only 1 SHADE per window allowed. */
				gz->name, WshadR.p[gz->x.iwshad].name );
			continue;						// skip (msg prevented RUN)
		}

		// make SHADE run record, put its subscript in window.x.iwshad

		CSE_E( WshadR.add( &shr, WRN) )  	// add shade run record, return if error
		*shr = *sh;						// copy record data (except internal overhead), ancrec.cpp.
		gz->x.iwshad = shr->ss;    		// WSHADRAT subscript to XSURF
	}
	return rc;
}		// topSh
//===========================================================================
RC FC topSf2()

// do surfaces or doors or windows, part 2

// makes XSURFs etc: stores values (inaccessibly)

// call AFTER topIz() as can use IzxR subscript values
// uses file-global msiLast: number of masses expected.

// returns non-RCOK only on serious error, to terminate topCkf().
// (any error msg prevents run, via error count in rmkerr.cpp.) */
{
	RC rc=RCOK;

	// loop over surfaces (walls/floors/ceilings), doors, and windows
	SFI* sf;			// door, window, or surface
	int floorBGCount = 0;	// count of below grade floors
	for (int ib=0; SFIAncs[ ib]; ib++)
	{	RLUP( *SFIAncs[ ib], sf)
		{	if (sf->sf_IsFloorBG())
			{
				floorBGCount++;			// count / skip BG floors
			}
			else
			{
				CSE_E(sf->sf_TopSf2());	// quit immediately on error
			}
		}
	}
	if (floorBGCount)
	{	// process below grade floors after all other surfaces
		//   BG floor model uses info from zone's BG walls
		RLUP( SfiB, sf)
		{	if (sf->sf_IsFloorBG())
				CSE_E( sf->sf_TopSf2());	// quit immediately on error
		}
	}

	// now have all XSRATs (runtime surfaces)
	// loop/init XSRATs
	XSRAT* xr;
	RLUP( XsB, xr)
	{
		xr->x.xs_AccumZoneValues();		// accumulate e.g. zone UA
		xr->x.xs_SetRunConstants();		// set SBC constants re convective and radiant xfer
#if defined( _DEBUG)
		xr->Validate( 0x100);			// 0x100: XSURF full check (s/b fully set up at this point)
#endif
	}

	return rc;
}	// ::topSf2
//-----------------------------------------------------------------------------
RC SFI::sf_TopSf2()
{
	RC rc = RCOK;
#if defined( _DEBUG)
	Validate();
#endif
	UCH* fs = fStat();		// record's field status byte array
	[[maybe_unused]] SFCLASS sfc = SFCLASS( sfClass);

	// access/check zone
	ZNR* zp = sf_GetZone( 0);		// point zone record
	if (!zp)
		return rc;		// skip if bad zone (previously msg'd)
	TI zi = zp->ss;		// idx of zone

	// net area now known, calc UA
	x.xs_UANom = x.xs_area * x.xs_UNom;

	// access construction if given

	BOO conSet = fs[ SFI_SFCON] & FsSET;	// non-0 if contruction given
	CON *con = nullptr;
	if (conSet)
	{	if (sfCon <= 0  ||  sfCon > ConiB.n)	// if bad
			return rc;					// skip (msg'd above)
		con = ConiB.p + sfCon;			// point construction record
	}

	// translate door/surface SFI into traditional XSURF chain for zone or "mass" or ...
	if (x.xs_modelr==C_SFMODELCH_QUICK)	// conductance model chosen
	{
		// apportion heat capacity, if any, of quick model surfaces, amoung adjacent zones.

		// added 2-95; formerly only done for intmasses (1 zone, con req'd), with entire hc being added.

		if (conSet)				// if construction given (else no heatcap, and eliminates windows)
			if (x.uI + x.uX > 0)	// if non-0 conductance (else not coupled and divide by 0) (insurance)
			{
				float hc = con->hc * sfArea;   		// heat capacity is from construction, times area
				float split = x.sfExCnd==C_EXCNDCH_ADIABATIC
							  ?  1.f   					// for adiabatic ouside put entire hc inside
							  :  x.uI/(x.uI + x.uX);		// else apportion by conductance (tentative 2-95)
				zp->i.znCAir += hc * split; 			// add portion of surface heat cap to zone "air" heat cap
				if (x.sfExCnd==C_EXCNDCH_ADJZN)			// if zone on other side ("outside") of wall
					ZrB.p[ x.sfAdjZi ].i.znCAir += hc * (1 - split);	// add rest of hc to that zone
			}
		XSRAT* xr = NULL; 		// receives ptr to XSRAT record added by cnuCompAdd
		switch (x.sfExCnd)	// simulator representation depends on ext cond
		{
		case C_EXCNDCH_AMBIENT:			// regular ol' traditional exterior surface, door, or window
		case C_EXCNDCH_SPECT:			// specified temp on other side
			CSE_E( cnuCompAdd( &x, name, zi, &xi, &xr) )		// add XSRAT record, below. return if error.
			if (x.sfExCnd==C_EXCNDCH_SPECT)				// if specT
			{
				// specified temp on other side: own xsurf chain for speed:
				// these xsurfs, only, processed every hour.
				xr->nxXsSpecT = zp->xsSpecT1;
				zp->xsSpecT1 = xr->ss;
			}
			break;

		case C_EXCNDCH_ADJZN:			// another zone on other side. Dissallowed in topSf1 for windows 2-95.
			// add conductive interzone transfer
			if (zi != x.sfAdjZi)			// omit if both sides in same zone! 2-95
			{
				IZXRAT *ize;
				CSE_E( IzxR.add( &ize, WRN) );		// add izxfer run record, return if error
				ize->iz_zi1 = zi;					// surface's zone
				ize->iz_zi2 = x.sfAdjZi;  			// other zone
				ize->iz_ua = x.xs_uval * x.xs_area;	// conductance = UA
				ize->iz_nvcntrl = C_IZNVTYCH_NONE;	// say no nat vent
				ize->iz_SetupNonAirNet();			// insurance: does nothing if no vent
			}
			// fall thru

		case C_EXCNDCH_ADIABATIC:			// no heat nor solar transfer
			// make runtime surface cuz solar gain & rad intgain targeting use inside of all surfaces, 2-95
			CSE_E( cnuCompAdd( &x, name, zi, &xi, &xr) )	// add XSRAT record, below. return if error.
			xr->x.xs_ty = CTINTWALL;		// change to "interior wall" to be sure ...
			break;							//    ... outside won't receive solar

		default:
			rc |= oer( (char *)MH_S0524, x.sfExCnd );
			// "Internal error in cncult.cpp:topSf2(): Bad exterior condition %d"
		}
	}
	else if ( SFI::sf_IsDelayed( x.xs_modelr))   	// if a massive model chosen
	{
		switch (x.sfExCnd)	// simulator representation depends on ext cond
		{
		case C_EXCNDCH_ADIABATIC:
		case C_EXCNDCH_AMBIENT:
		case C_EXCNDCH_SPECT:
		case C_EXCNDCH_ADJZN:
		case C_EXCNDCH_GROUND:
			rc |= sf_MakMs();	// make traditional MSRAT record for delayed model surf, local, below
			break;

		default:
			rc |= oer( (char *)MH_S0524, x.sfExCnd );
				// "Internal error in cncult.cpp:topSf2(): Bad exterior condition %d"
		}
	}
	else if ( SFI::sf_IsKiva(x.xs_modelr))
	{
		rc |= sf_SetupKiva();	// for floors, make KIVA records for finite difference foundation heat transfer
								// for walls, add index to sf_sharedFndWalls
	}
	else
		rc |= oer( (char *)MH_S0525, x.xs_modelr );
				// "Internal error in cncult.cpp:topSf2(): Bad surface model %d"

#if defined( _DEBUG)
	Validate();
#endif
	return rc;
}		// SFI::sf_TopSf2
//===========================================================================
RC topZn2()			// accumulate surface totals etc
{	RC rc = RCOK;
	ZNR* zp;
	RLUP( ZrB, zp)
		zp->zn_InitSurfTotals();

	// loop all surfaces exposed to zone
	SFI* sf;			// door, window, or surface
	for (int ib=0; SFIAncs[ ib]; ib++)
	{	RLUP( *SFIAncs[ ib], sf)
		{
#if defined( _DEBUG)
			sf->Validate();
#endif
			for (int si=0; si<2; si++)
			{
				zp = sf->sf_GetZone( si);
				if (zp)
				{	SBCBASE& S = sf->x.xs_SBC( si);
					zp->zn_AccumSurfTotals( S);
				}

			}
		}
	}

	// loop duct surfaces exposed to zone
	DUCTSEG* ds;
	RLUP( DsR, ds)
	{	zp = ds->ds_GetExZone();
		if (zp)
			zp->zn_AccumSurfTotals( ds->ds_sbcO);
	}

	// ?PIPE?

	RLUP( ZrB, zp)
	{	zp->zn_FinalizeSurfTotals();
		zp->zn_RadX();			// set F and Fp values for air and surfaces
	}

	return rc;

}	// ::topZn2
//---------------------------------------------------------------------------
RC topZn3()			// final zone pass
{	RC rc = RCOK;
	ZNR* zp;
	RLUP( ZrB, zp)
		zp->zn_AddIZXFERs();	// add internally-generated IZXFERs re
								//   AIRNET modeling of duct leakage and system air

	return rc;
}	// ::topZn3
//---------------------------------------------------------------------------
void ZNR::zn_InitSurfTotals()			// init (0) surface area accumulators
// values are accumulated for all surfaces adjacent to zone
//  including outside face of C_EXCNDCH_ADJZN surfaces belonging to other zones
{
	zn_sbcList.clear();
	zn_surfA = 0.;			// total surface area
	zn_surfASlr = 0.;		// solar (non window, non duct) area
	zn_ductA = 0.;			// duct surface area (included in zn_surfA)

	// zn_ductCondUA = 0. done elsewhere due to calc sequence (
    zn_surfEpsLWAvg = 0.;	// SUM( area * epsLW)/SUM( area)
	zn_ua = 0.;
	zn_uaSpecT = 0.;
	zn_UANom = 0.;			// nominal zone UA (not including zn_ductUA)
	zn_BGWallPerim = 0.;
	zn_BGWallPA4 = 0.;
	zn_BGWallPA5 = 0.;

}		// ZNR::zn_InitSurfTotals
//---------------------------------------------------------------------------
void ZNR::zn_AccumSurfTotals(
	SBCBASE& S)	// surface boundard from which to accum
// do NOT call unless boundary is adjacent to this zone
{
	double A = S.sb_AreaNet();		// net area
	zn_sbcList.push_back( &S);
	zn_surfA += A;		// total surface area
	zn_surfEpsLWAvg += A * S.sb_epsLW;

	if (S.sb_Class() == sfcDUCT)
		zn_ductA += A;		// subtotal duct area
	else if (S.sb_Class() < sfcWINDOW)
		zn_surfASlr += A;		// opaque space surface area
								//   non-window, non-duct, non-pipe

}		// ZNR::AccumSurfTotals
//---------------------------------------------------------------------------
void ZNR::zn_FinalizeSurfTotals()
{
	if (zn_surfA > 0.)
		zn_surfEpsLWAvg /= zn_surfA;

}		// ZNR::FinalizeSurfTotals
//----------------------------------------------------------------------------
void ZNR::zn_SetAirRadXArea()		// set mbrs re zone air radiant pseudo surface
// see also zn_RadX()
{
	zn_airRadXArea = zn_airRadXC1 *
		log( 1. + zn_airRadXC2*zn_relHum*exp( (tz-32.f)/30.6));
	zn_airCxF = sigmaSB4*zn_airRadXArea*zn_FAir;
#if defined( DEBUGDUMP)
	if (DbDo( dbdRADX))
		DbPrintf( "%s radXArea   tz=%0.3f  relHum=%0.4f  airArea=%0.1f  CxF=%0.6g\n",
			name, tz, zn_relHum, zn_airRadXArea, zn_airCxF);
#endif
}	// ZNR::zn_SetAirRadXArea
//-----------------------------------------------------------------------------
// FFactors() convergence failure seen in optimized release builds
//   Disabling optimization seems to fix, did not fully analyze, 2-8-12
#pragma optimize( "", off)
static int FFactors(			// derive spherical geometry factors
	int nS,				// # of surfaces
	const double areaS[],	// areas of surfaces
	double F[],				// returned: factors
	const char*& errTxt)	// returned: error message text

// F[ i] = area of spherical cap / area of base of cap
//         always >= 1 (returned as 1 when areaBase = 0)

// see Carroll, J. A., 1980a, "An MRT method of computing radiant energy exchange in rooms"
// Proceedings of the 2nd Systems Simulation and Economic Analysis Conference, San Diego, CA.

// return RCOK: success (F[] set)
//       RCBAD: trouble, errTxt points to explanatory text (static)
{
	errTxt = "";
	if (nS <= 0)
		errTxt = "no surfaces";
	else
	{	VSet( F, nS, 1.);
		double sumAF = VSum( areaS, nS);

		[[maybe_unused]] int ret = 0;
		int iL;		// loop counter
		static const int iLLimit = 100;
		double fChange, fLast;
		for (iL = 0; iL < iLLimit; iL++)
		{	fChange = 0.;
			for (int iS=0; iS<nS; iS++)
			{	fLast = F[ iS];
				F[ iS] = 1./(1. - areaS[ iS]*F[ iS] / (/*facet[ iS]* */ sumAF));
				if (F[ iS] > 100.)
				{	errTxt = "geometry cannot be mapped onto a sphere";
					break;
				}
				fChange += fabs( F[ iS] - fLast);
				sumAF += (F[iS] - fLast)*areaS[ iS];
			}
#if 0 && defined( _DEBUG)
x			// check for error in incrementally calculated sumAF
x			// seems OK, 12-18-13
x			double sumAFX = DotProd( areaS, F, nS);
x			if (fabs( sumAFX - sumAF) > .001)
x				printf( "Mismatch\n");
#endif
			if (errTxt[ 0] || fChange / nS < .0001)
				break;
		}
		if (iL >= iLLimit)
			errTxt = "did not converge";
	}

	return errTxt[ 0] ? RCBAD : RCOK;
}	// FFactors
#pragma optimize( "", on)
//-----------------------------------------------------------------------------
// #undef CZM_COMPARE		// #define in cndefns.h to use exact CZM values re result comparison
//-----------------------------------------------------------------------------
RC ZNR::zn_RadX()

// see Carroll, J. A., 1980a, "An MRT method of computing radiant energy exchange in rooms"
// Proceedings of the 2nd Systems Simulation and Economic Analysis Conference, San Diego, CA.

// returns RCOK on success, mbrs set
//         RCBAD on missing inputs (e.g. zn_surfEpsLWAvg, ); mbrs 0.

{
	RC rc = RCOK;

	zn_airRadXArea = 0.;

	int nS = zn_sbcList.size();
	if (nS == 0 || zn_surfEpsLWAvg <= 0.)
		return RCBAD;
	WVect< double> tArea( nS+1);
	WVect< double> F( nS+1);

	// zone volume / total surface area (meters)
	//   if vol or area not known, say 2.5 m (typical value for residential space)
	float voaM = zn_surfA > 0. && i.znVol > 0.
					? max( .001f, i.znVol / (zn_surfA * 3.2808f))
					: 2.5f;
	float patm = Top.tp_presAtm/PsyPBarSeaLvl;		// pressure (atmospheres)

	// temp and RH from Top. values (possibly user input)
	//   default-default = 70 F and 60% RH 9-10
	tz = Top.refTemp;
	zn_relHum = Top.refRH;
#if defined( CZM_COMPARE)
	// match hard-coded CZM.BAS assumptions
	//   facilitates results comparison 10-10
	tz = 70.f;
	zn_relHum = .5f;
	voaM = 2.5f;
#endif

	zn_airRadXC1 = 0.08 * zn_surfEpsLWAvg * zn_surfA;
	zn_airRadXC2 = 4. * voaM * patm / zn_surfEpsLWAvg;
	zn_FAir = 1.;			// initial value, updated below
	zn_SetAirRadXArea();

	int nSSpace = 0;	// # of space definition surfaces (info only)
						//  not sfcDUCT, not sfcPIPE
	int iS;
	for (iS=0; iS<nS; iS++)
	{	SBCBASE& S = *zn_sbcList[ iS];
		tArea[ iS] = S.sb_AreaNet();
		if (S.sb_Class() < sfcDUCT)
			nSSpace++;
	}

	// long wave: include air by adding additional surface to array
	const char* errTxt = NULL;
	tArea[ nS] = zn_airRadXArea;
	RC rc1 = FFactors( nS+1, tArea.data(), F.data(), errTxt);
	if (rc1 != RCOK)
	{	rc |= rc1;
		errCrit( WRN, "Zone '%s' LW FFactors failure (%s)", name, errTxt);
	}

	// add Oppenheim surface resistance
	for (iS=0; iS<nS; iS++)
	{	SBCBASE& S = *zn_sbcList[ iS];
		// F[ iS] >= 1, so the following is safe re 1/0
		//   eps = 0 -> Fp = 0
		//   eps = 1 -> Fp = F
		S.sb_Fp = S.sb_epsLW/(S.sb_epsLW/F[ iS] + 1.- S.sb_epsLW);
	}
	zn_FAir = F[ nS];
	zn_SetAirRadXArea();		// zn_FAir now known, update zn_airCxF

	// short wave: surfaces only
	int iSX = 0;
	for (iS=0; iS<nS; iS++)
	{	SBCBASE& S = *zn_sbcList[ iS];
		if (S.sb_Class() < sfcDUCT)
			tArea[ iSX++] = S.sb_AreaNet();		// include only non-ducts, non-pipes
	}

	rc1 = FFactors( iSX, tArea.data(), F.data(), errTxt);
	if (rc1 != RCOK)
	{	rc |= rc1;
		errCrit( WRN, "Zone '%s' SW FFactors failure (%s)", name, errTxt);
	}
	for (iSX=0,iS=0; iS<nS; iS++)
	{	SBCBASE& S = *zn_sbcList[ iS];
		S.sb_F = S.sb_Class() < sfcDUCT
					? F[ iSX++]
					: 0.;
	}

#if defined( DEBUGDUMP)
	if (DbDo( dbdRADX|dbdCONSTANTS))
	{	DbPrintf( "%s RadX%s\n"
			"Surf             area   epsLW     Fp   absSlr     F\n",
			name,
#if defined( CZM_COMPARE)
			"  CZM_COMPARE defined");
#else
			"");
#endif
		double airRadXeps = zn_surfA > 0. ? zn_airRadXArea/zn_surfA : 0.;
		const char* fmt="%-12s  %7.1f   %5.3f  %5.3f  %7.3f %5.3f\n";
		DbPrintf( fmt, "Air",  zn_airRadXArea, airRadXeps, zn_FAir, 0., 1.);
		for (iS=0; iS<nS; iS++)
		{	SBCBASE& S = *zn_sbcList[ iS];
			DbPrintf( fmt,
				S.sb_ParentName(), S.sb_AreaNet(), S.sb_epsLW, S.sb_Fp, S.sb_absSlr, S.sb_F);
		}
		DbPrintf( "FAir=%.3f  surfA=%.1f  vol=%.1f  voaM=%.3f\n"
			"surfEpsLWAvg=%0.3f  airRadXC1=%.2f   airRadXC2=%.4f\n\n",
			zn_FAir, zn_surfA, i.znVol, voaM, zn_surfEpsLWAvg, zn_airRadXC1, zn_airRadXC2);
	}
#endif
	return rc;
}	// ZNR::zn_RadX
//===========================================================================
RC FC topSg2()		// SGDIST processing at RUN after topSf2 calls, 2-95

// returns non-RCOK only on serious error, to terminate topCkf().
{
	SGI* sg;
	RLUP( SgiB, sg)			// loop SGI records
	{
		// for SGDISTs to quick surfaces, replace input record subscript with runtime XSRAT subscript in .targTi
		if (sg->d.sd_targTy==SGDTTSURFI || sg->d.sd_targTy==SGDTTSURFO)
			sg->d.sd_targTi = SfiB.p[ sg->d.sd_targTi].xi;   			// get XsB suscript stored by topSf2
	}
	return RCOK;
}			// topSg2

//===========================================================================
/*static*/ int SFI::sf_IsSubhrly( int m)
{  return m == sf_IsFD( m) || C_SFMODELCH_DELAYED_SUBHOUR; }
//---------------------------------------------------------------------------
/*static*/ int SFI::sf_IsDelayed( int m)
{  return sf_IsFD( m) || m == C_SFMODELCH_DELAYED || m == C_SFMODELCH_DELAYED_HOUR
	   || m == C_SFMODELCH_DELAYED_SUBHOUR; }
//---------------------------------------------------------------------------
/*static*/ int SFI::sf_IsFD( int m)
{  return m == C_SFMODELCH_FD; }
//---------------------------------------------------------------------------
/*static*/ int SFI::sf_IsKiva(int m)
{  return m == C_SFMODELCH_KIVA; }
//---------------------------------------------------------------------------
SFI::SFI( basAnc* b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
	// sf_sharedFndWalls() called *after* record()
{
	FixUp();
}	// SFI::SFI
//---------------------------------------------------------------------------
/*virtual*/ SFI::~SFI()
{
}		// SFI::~SFI
//---------------------------------------------------------------------------
/*virtual*/ void SFI::Copy( const record* pSrc, int options/*=0*/)
{
	sf_sharedFndWalls.vector::~vector<TI>();
	record::Copy( pSrc, options);
	// base class calls FixUp() and (if _DEBUG) Validate()
	new(&sf_sharedFndWalls) vector<TI>(((const SFI*)pSrc)->sf_sharedFndWalls);
}		// SFI::Copy
//----------------------------------------------------------------------------
/*virtual*/ SFI& SFI::CopyFrom(const record* src, int copyName/*= 1*/, int dupPtrs/*= 0*/)
{
	sf_sharedFndWalls.vector::~vector<TI>();
	record::CopyFrom(src, copyName, dupPtrs);
	new(&sf_sharedFndWalls) vector<TI>(((const SFI*)src)->sf_sharedFndWalls);

	return *this;
}		// SFI::CopyFrom
//---------------------------------------------------------------------------
/*virtual*/ void SFI::FixUp()		// fix links
// called from basAnc::reAl() and this c'tor, Copy()
{	x.xs_Init( this);
}		// SFI::FixUp
//---------------------------------------------------------------------------
/*virtual*/ RC SFI::Validate(
	int options/*=0*/)		// option bits
							//   0x100: XSURF full check
{	RC rc = record::Validate( options);
	if (rc == RCOK)
		rc = x.xs_Validate( options);
	return rc;
}		// SFI::Validate
//---------------------------------------------------------------------------
int SFI::sf_IsWallBG() const	// TRUE iff below grade wall
{
	return x.sfExCnd == C_EXCNDCH_GROUND
		&& sfTy == C_OSTYCH_WALL
		&& (x.xs_depthBG > 0.f		// not pseudo-slab (e.g. crawlspace edge)
			|| x.xs_modelr == C_SFMODELCH_KIVA);
}		// SFI::sf_IsWallBG
//---------------------------------------------------------------------------
int SFI::sf_IsFloorBG() const	// TRUE iff below grade floor (not slab on grade)
{	return x.sfExCnd == C_EXCNDCH_GROUND
        && sfTy == C_OSTYCH_FLR
		&& (x.xs_depthBG > 0.f	// not slab on grade
			|| x.xs_modelr == C_SFMODELCH_KIVA);
}		// SFI::sf_IsFloorBG
//---------------------------------------------------------------------------
int SFI::sf_IsBG() const	// TRUE iff below grade surface
{	return x.sfExCnd == C_EXCNDCH_GROUND
        && (x.xs_depthBG > 0.f
			|| x.xs_modelr == C_SFMODELCH_KIVA);
}		// SFI::sf_IsBG
//---------------------------------------------------------------------------
int SFI::sf_IsChild() const		// TRUE iff child surface
// DOOR and WINDOW always have parent SURF
{	return sfClass != sfcSURF;
}		// SFI::sf_IsChild
//---------------------------------------------------------------------------
SFI* SFI::sf_GetParent()		// retrieve parent surface
// returns ptr to parent
//         NULL if does not have parent
{
	SFI* sfP = sf_IsChild() ? SfiB.GetAt( ownTi) : NULL;
	return sfP;
}		// SFI::sf_GetParent
//---------------------------------------------------------------------------
const SFI* SFI::sf_GetParent() const	// retrieve parent surface
// returns ptr to parent
//         NULL if does not have parent
{
	const SFI* sfP = sf_IsChild() ? SfiB.GetAt( ownTi) : NULL;
	return sfP;
}		// SFI::sf_GetParent
//---------------------------------------------------------------------------
TI SFI::sf_GetZI(		// zone subscript adjacent to surface
	int si) const	// 0=inside, 1=outside
// returns zone subscript
//         0 if fail
{
	TI zi = 0;
	if (si)
	{	// outside
		if (x.sfExCnd == C_EXCNDCH_ADJZN)
			// sfAdjZi is valid for child surfs (copied by sf_TopSf1())
			zi = x.sfAdjZi;
	}
	else
	{	if (sf_IsChild())
		{	const SFI* sfP = sf_GetParent();
			if (sfP)
				zi = sfP->ownTi;
		}
		else
			zi = ownTi;
	}
	return zi;
}		// SFI::sf_GetZI
//---------------------------------------------------------------------------
ZNR* SFI::sf_GetZone(		// zone adjacent to surface
	int si) const	// 0=inside, 1=outside
// returns ZNR*
//         NULL if fail (no msg)
{
	TI zi = sf_GetZI( si);
	ZNR* zp = zi > 0 ? ZrB.GetAt( zi) : NULL;
	return zp;
}		// SFI::sf_GetZone
//---------------------------------------------------------------------------
RC SFI::sf_MakMs()

// make mass record for delayed-model surface

{
	RC rc = RCOK;

// add mass run record.  Subscr previously assigned in topSf1().
	MSRAT* ms;
	MsR.add( &ms, ABT, sf_msi);  		// error unexpected as previously .al'd (in topMsCon).

// access surf's construction (user input)
	CON* con;
	if (ckRefPt( &ConiB, sfCon, "sfCon", NULL, (record **)&con ) )
		return RCBAD;

	rc = ms->ms_Make( this, con, 0);

	return rc;
}		// SFI::sf_makMs

//---------------------------------------------------------------------------
RC SFI::sf_SetupKiva()

// make kiva instances

{
	RC rc = RCOK;


	if (sf_IsWallBG())	// This is done during the first surface loop...
	{
		auto flrSf = SfiB.GetAt(sfFndFloor);
		flrSf->sf_sharedFndWalls.push_back(ss); // add Wall index to list in associated floor
	}
	else // BG floor setup during second surface loop
	{
		// Get pointer to associated FOUNDATION
		FOUNDATION* pFnd;
		if (ckRefPt(&FndiB, sfFnd, "sfFnd", NULL, (record **)&pFnd))
		{
			return RCBAD;
		}

		// Add Kiva instances until all remaining exposed perimeter is accounted for
		auto remainingExpPerim = sfExpPerim;

		// Create wall combinations
		std::map<std::pair<TI, double>, KivaWallGroup> combinationMap;	// Map of construction index (TI) and wall height (double) to
																		// KivaWallGroup (relative perimeter and vector of wall indices)

		if (sf_sharedFndWalls.size() != 0)
		{
			for (auto wli : sf_sharedFndWalls)
			{
				auto wlSf = SfiB.GetAt(wli);
				auto wlHt = wlSf->x.xs_height;
				auto perimeter = wlSf->sfArea / wlHt;
				auto coni = wlSf->sfCon;

				if (combinationMap.count({ coni, wlHt }) == 0) {
					// create new combination
					std::vector<TI> walls = { wli };
					combinationMap[{ coni, wlHt }] = KivaWallGroup(perimeter, walls);
				}
				else {
					// add to existing combination
					combinationMap[{ coni, wlHt }].perimeter += perimeter;
					combinationMap[{ coni, wlHt }].wallIDs.push_back(wli);
				}
			}
		}

		// add XSURF for floor surface
		XSRAT* xr;
		rc = cnuCompAdd(&x, name, x.xs_sbcI.sb_zi, &xi, &xr);	// add XSRAT to mass's inside zone, ret ptr.
		if (!rc)	// if added ok: insurance
		{	// Kiva XSURF differences from SFI.x (do not change input (esp .x.xs_ty) in case surf quick-modelled on later run)
			xr->x.xs_ty = CTKIVA;	// XSURF type: kiva surface
			xr->x.nsgdist = 0;   	// delete any sgdists: believed redundant 2-95.
			xr->xr_kivaAggregator = new Kiva::Aggregator(Kiva::Surface::ST_SLAB_CORE);
		}
	
		bool assignKivaInstances = true;
		auto comb = combinationMap.begin();

		while (assignKivaInstances)
		{
			// Set wall combination parameters (construction, height, perimeter, surface IDs)
			TI coni;
			double height, perimeter;
			std::vector<TI> wallIDs;
			if (comb != combinationMap.end())
			{
				// Loop through wall combinations first
				coni = comb->first.first;
				height = comb->first.second;
				perimeter = comb->second.perimeter;
				wallIDs = comb->second.wallIDs;
			}
			else
			{
				// Then assign remainign exposed perimeter to a slab instance
				coni = pFnd->fd_ftWlConi; // Get construction from Foundation input
				height = 0.0;
				perimeter = remainingExpPerim;
			}

			// Create/add new kiva instance to runtime anchor
			KIVA* ki;
			TI kvi = 0;
			KvR.add(&ki, ABT, kvi);

			ki->kv_floor = ss;

			// Set weighted perimeter for this combination
			double instanceWeight = sfExpPerim > 0.0001
				? perimeter / sfExpPerim
				: 1.0; // No exposre = 1D instance exchanging heat with deep ground

			ki->kv_perimWeight = instanceWeight;

			// Copy foundation input from foundation object into KIVA runtime record
			if (!ki->kv_instance)
			{
				ki->kv_instance = new Kiva::Instance();
				if (!ki->kv_instance)
					return RCBAD; // oer?
				ki->kv_instance->foundation = std::make_shared<Kiva::Foundation>(*pFnd->fd_kivaFnd);
			}

			std::shared_ptr<Kiva::Foundation> fnd = ki->kv_instance->foundation;

			LR* pLR;
			// Set wall construction in Kiva
			CON* pCon = ConiB.GetAtSafe(coni);
			RLUPR(LriB, pLR)			// loop over layers records in reverse -- all CONs
										// Kiva defines layers in oposite dir.
										// Assumes order in anchor is consistent with order of construction.
			{
				if (pLR->ownTi == pCon->ss)		// if a layer of given con
				{
					const MAT* pMat = MatiB.GetAt(pLR->lr_mati);
					Kiva::Layer tempLayer;
					tempLayer.material = kivaMat(pMat->mt_cond,pMat->mt_dens,pMat->mt_spHt);
					tempLayer.thickness = LIPtoSI(pLR->lr_thk);

					fnd->wall.layers.push_back(tempLayer);
				}
			}

			fnd->wall.interior.emissivity = x.xs_sbcI.sb_epsLW;
			fnd->wall.exterior.emissivity = x.xs_sbcO.sb_epsLW;
			fnd->wall.interior.absorptivity = x.xs_sbcI.sb_awAbsSlr;
			fnd->wall.exterior.absorptivity = x.xs_sbcO.sb_awAbsSlr;

			// Set slab construction in Kiva
			if (ckRefPt(&ConiB, sfCon, "sfCon", NULL, (record **)&pCon))
				return RCBAD;
			RLUPR(LriB, pLR)			// loop over layers records in reverse -- all CONs
										// Kiva defines layers in oposite dir.
										// Assumes order in anchor is consistent with order of construction.
			{
				if (pLR->ownTi == pCon->ss)		// if a layer of given con
				{
					const MAT* pMat = MatiB.GetAt(pLR->lr_mati);
					Kiva::Layer tempLayer;
					tempLayer.material = kivaMat(pMat->mt_cond, pMat->mt_dens, pMat->mt_spHt);
					tempLayer.thickness = LIPtoSI(pLR->lr_thk);

					fnd->slab.layers.push_back(tempLayer);
				}
			}
			fnd->slab.interior.emissivity = x.xs_sbcI.sb_epsLW;
			fnd->slab.interior.absorptivity = x.xs_sbcI.sb_awAbsSlr;

			// Set other properties
			fnd->foundationDepth = LIPtoSI(height);
			fnd->hasPerimeterSurface = false;
			fnd->perimeterSurfaceWidth = 0.0;

			fnd->reductionStrategy = Kiva::Foundation::RS_AP;

			// Build polygon and set exposed fraction to get correct A/P
			const double sqrPerim = 4 * sqrt(sfArea);
			double l, w;
			if (Kiva::isGreaterOrEqual(sqrPerim, sfExpPerim))
			{
				l = w = LIPtoSI(sqrPerim / 4);
				fnd->exposedFraction = sfExpPerim / sqrPerim;
			}
			else
			{
				// Make a rectangle with the correct perimeter
				l = LIPtoSI((sfExpPerim + sqrt(sfExpPerim*sfExpPerim - 16*sfArea))/4.);
				w = LIPtoSI(sfArea / l);
				fnd->exposedFraction = 1.0;
			}

			fnd->polygon.outer().push_back(Kiva::Point(0., 0.));
			fnd->polygon.outer().push_back(Kiva::Point(0., l ));
			fnd->polygon.outer().push_back(Kiva::Point(w,  l ));
			fnd->polygon.outer().push_back(Kiva::Point(w,  0.));


			double halfWidth = LIPtoSI(sfArea / sfExpPerim);	// A/P ratio

			// Define X reference points
			const double x_iwall = 0.0; // by definition
			const double x_ewall = x_iwall + fnd->wall.totalWidth();
			const double x_cwall = x_iwall + (x_ewall - x_iwall) / 2.0;
			const double x_sym = x_iwall - halfWidth;  // A/P
			const double x_ff = x_ewall + fnd->farFieldWidth;

			std::map<unsigned short, double> xRefs =
			{
				{ C_FBXREFCH_WALLINT , x_iwall },
				{ C_FBXREFCH_WALLEXT , x_ewall },
				{ C_FBXREFCH_WALLC , x_cwall },
				{ C_FBXREFCH_SYMMETRY , x_sym },
				{ C_FBXREFCH_FARFIELD , x_ff }
			};

			// Define Z reference points
			const double z_twall = 0.0; // by definition
			const double z_tslab = z_twall + height;
			const double z_bslab = z_tslab + fnd->slab.totalWidth();
			const double z_bwall = z_bslab + fnd->wall.depthBelowSlab;
			const double z_grd = z_twall + fnd->wall.heightAboveGrade;
			const double z_dg = z_grd + fnd->deepGroundDepth;

			std::map<unsigned short, double> zRefs =
			{
				{ C_FBZREFCH_WALLTOP , z_twall },
				{ C_FBZREFCH_SLABTOP , z_tslab },
				{ C_FBZREFCH_SLABBOTTOM , z_bslab },
				{ C_FBZREFCH_WALLBOTTOM , z_bwall },
				{ C_FBZREFCH_GRADE , z_grd },
				{ C_FBZREFCH_DEEPGROUND , z_dg }
			};

			// Add blocks
			FNDBLOCK* pBL;
			RLUP(FbiB, pBL)			// loop over foundation block records
			{
				if (pBL->ownTi == pFnd->ss)		// if a block of given foundation
				{
					Kiva::InputBlock block;
	
					// Assign material
					const MAT* pMat = MatiB.GetAt(pBL->fb_mati);
					block.material = kivaMat(pMat->mt_cond, pMat->mt_dens, pMat->mt_spHt);

					// Place block in 2D context
					const double x1 = xRefs[pBL->fb_x1Ref] + LIPtoSI(pBL->fb_x1);
					const double x2 = xRefs[pBL->fb_x2Ref] + LIPtoSI(pBL->fb_x2);
					const double z1 = zRefs[pBL->fb_z1Ref] + LIPtoSI(pBL->fb_z1);
					const double z2 = zRefs[pBL->fb_z2Ref] + LIPtoSI(pBL->fb_z2);

					block.x = x1;
					block.width = x2 - x1;
					block.z = z1;
					block.depth = z2 - z1;
					fnd->inputBlocks.push_back(block);
				}
			}

			// Create Kiva instace
			ki->kv_Create();

			// create wall XSURFs and assign kiva aggregator
			for (auto wli : wallIDs) 
			{
				auto wlSf = SfiB.GetAt(wli);

				XSRAT* xrWl;
				rc = cnuCompAdd(&wlSf->x, wlSf->name, wlSf->x.xs_sbcI.sb_zi, &wlSf->xi, &xrWl);	// add XSRAT to mass's inside zone, ret ptr.
				ki->kv_walls.push_back(wlSf->xi);
				if (!rc)	// if added ok: insurance
				{	// Kiva XSURF differences from SFI.x (do not change input (esp .x.xs_ty) in case surf quick-modelled on later run)
					xrWl->x.xs_ty = CTKIVA;	// XSURF type: mass ext wall
					xrWl->x.nsgdist = 0;   	// delete any sgdists: believed redundant 2-95.
					xrWl->xr_kivaAggregator = new Kiva::Aggregator(Kiva::Surface::ST_WALL_INT);
					xrWl->xr_kivaAggregator->add_instance(ki->kv_instance->ground.get(), 1.0);
				}
			}

			// add instance to floor aggregator
			xr->xr_kivaAggregator->add_instance(ki->kv_instance->ground.get(), instanceWeight);

			// Increment wall combinations iterator
			if (comb != combinationMap.end()) {
				comb++;
			}

			remainingExpPerim -= perimeter;

			if (remainingExpPerim < 0.001) {
				assignKivaInstances = false;
			}

		}

	}
	
	return rc;
}		// SFI::sf_SetupKiva



//-----------------------------------------------------------------------------
static RC BGDeriveCoeffs(
	float depthBG,	// depth below grade of bottom of surface, ft
	float RNom,		// wall construction surface-to-surface resistance, ft2-F/Btuh
	int interiorInsul,	// 1 if interior surface is covered/insulated
						// 0 if not
	float a[ 6])	// returned: ground coupling coefficients, a[ 1] - a[ 5]
					//   a[ 0] unused so subscripts consistent with Niles notation
					//   a[ 1], a[ 2], a[ 3] = wall coefficients
					//   a[ 4], a[ 4] = coefficients for associated floor
// returns RCOK on success
{
	int rc = RCOK;

	VZero( a, 6);

	BOOL bFullBasement = depthBG > 4.f;
	float RWall = RNom + 0.77f;		// independent variable in Niles fits
									//  includes inside surface conductance;
									//  Niles used 0.77 instead of 0.68, we
									//  do the same so correlations are valid

	if (bFullBasement)
	{	if (interiorInsul)
		{	a[ 1] = 1.f/(-0.0156f*RWall*RWall + 2.5035f*RWall + 3.6481f);
			a[ 2] = 1.f/(-0.0248f*RWall*RWall + 3.9933f*RWall + 9.815f);
			a[ 3] = 0.f;
			a[ 4] = 0.0057f*log( RWall) + 0.0208f;
			a[ 5] = 0.001f*log( RWall) + 0.0412f;
		}
		else
		{	a[ 1] = 1.f/(2.3847f*RWall + 2.7947f);
			a[ 2] = 1.f/(-0.0203f*RWall*RWall + 1.6459f*RWall + 14.108);
			a[ 3] = 0.000030f*RWall + 0.0026f;
			a[ 4] = 0.0016f*log( RWall) + 0.0229f;
			a[ 5] = 1.f/(-0.449f*log( RWall) + 24.074f);
		}
	}
	else
	{	// partial basement
		if (interiorInsul)
		{	a[ 1] = 6.443f  * pow( RWall+5.f, -1.742f);
			a[ 2] = 0.4881f * pow( RWall+5.f, -1.188f);
			a[ 3] = 0.f;
			a[ 4] =	-0.00003f*RWall*RWall + 0.0014f*RWall + 0.0383f;
			a[ 5] =	-0.00006f*RWall + 0.0321f;
		}
		else
		{	a[ 1] = 1.f/(-0.0112f*RWall*RWall + 1.6083f*RWall + 1.414f);
			a[ 2] = 1/(8.9199*log(RWall+5)+1.4261);
			a[ 3] = 0.00008*RWall + 0.0015;
			a[ 4] = -0.000136*RWall + 0.037858;
			a[ 5] = .0000760f*RWall + .0333f;
		}
	}

	return rc;
}	// ::BGDeriveCoeffs
//-----------------------------------------------------------------------------
RC SFI::sf_BGWallSetup(		// below-grade wall setup
	AR_MASSLAYER& arML)

// call iff xs_depthBG > 0
{
	RC rc = RCOK;
	// if defaulting

	int interiorInsul;
	float RNom = arML.ml_GetPropertiesBG( interiorInsul);

	// derive ground coupling coefficients using Niles fits to Bajanac data
	//   result is in a[] (a[ 0] unused, a[ 1] = Niles a1, a[ 2] = a2 etc.
	float a[ 6];
	rc = BGDeriveCoeffs( x.xs_depthBG, RNom, interiorInsul, a);

	if (!rc)
	{	x.xs_sbcO.sb_SetCoeffsWallBG( a);

		rc = sf_BGFinalizeLayers( arML, 0.68f, RNom);

		// accumulate info for floor
		float perim = x.xs_area / x.xs_depthBG;
		ZNR* zp = sf_GetZone( 0);
		zp->zn_BGWallPerim += perim;
		zp->zn_BGWallPA4 += perim * a[ 4];
		zp->zn_BGWallPA5 += perim * a[ 5];
	}
	return rc;
}		// SFI::sf_BGWallSetup
//-----------------------------------------------------------------------------
RC SFI::sf_BGFloorSetup(		// below-grade floor setup
	AR_MASSLAYER& arML)
// call *AFTER* all other surfaces have been processed
{
	RC rc = RCOK;

	// determine average a4 and a5 coefficients
	BOOL bFullBasement = x.xs_depthBG > 4.f;
	float a4, a5;
	ZNR* zp = sf_GetZone( 0);
	if (zp->zn_BGWallPerim > 0.)
	{	a4 = zp->zn_BGWallPA4 / zp->zn_BGWallPerim;
		a5 = zp->zn_BGWallPA5 / zp->zn_BGWallPerim;
	}
	else if (bFullBasement)
	{	a4 = .0233f;		// use R-0 coeffs
		a5 = .0419f;
	}
	else
	{	a4 = .0376f;
		a5 = .0327f;
	}

	x.xs_sbcO.sb_SetCoeffsFloorBG( a4, a5);

	// adjust layers and add soil
	rc = sf_BGFinalizeLayers( arML, 0.77f);

	return rc;

}		// SFI::sf_BGFloorSetup
//-----------------------------------------------------------------------------
RC SFI::sf_BGFinalizeLayers(		// adjust layers / add soil
	AR_MASSLAYER& arML,		// construction layers
	float RFilm,			// surface film resistance implicit in
							//   ground factors
	float RNom /*=-1.f*/)
// returns: RCOK = success, arML updated
//          else failure, msg issued, arML unchanged
{
	RC rc = RCOK;
	float cGrnd = x.xs_sbcO.sb_CGrnd();
	if (cGrnd < .0000001)
		return oer( "sfExCnd is GROUND but all ground couplings are 0");
	float rGrnd = 1.f / cGrnd;
	x.xs_sbcO.sb_rGrnd = rGrnd;

	if (RNom < 0.f)
	{	int interiorInsul;	// unused here
		RNom = arML.ml_GetPropertiesBG( interiorInsul);
	}

	// determine maximum allowable construction R
	//   rGrnd = overall steady-state resistance
	//   Construction max R must be less than overall
	//   allow at lease 0.5 for sb_hxa exterior coupling
	float RSoil = 2.f;
	float RNomMax = rGrnd - RFilm - RSoil - 0.5f;

	// reduce resistance if needed by eliminating insulation
	float RNomAdj = arML.ml_AdjustResistance( RNomMax);

	if (RNomAdj > RNomMax)
	{	// could not fully adjust
		RSoil -= RNomAdj - RNomMax;
		if (RSoil < 0.f)	// if elimating soil not sufficient
			return oer( "Below grade layer adjustment failed");
		if (RSoil < 0.1f)	// if only a little soil needed
			RSoil = 0.f;	//   don't bother
	}

	// add layer of soil to outside of construction
	//   thickness is 2 ft or as adjusted smaller
	if (RSoil > 0.f)
	{	int iLSrc = arML.size();
		arML.ml_AddSoilLayer( iLSrc, RSoil);
	}

	x.xs_sbcO.sb_rConGrnd = RNomAdj + RSoil + RFilm;
	x.uC = 1.f/(RNomAdj + RSoil);

	return rc;
}		// SFI::sf_BGFinalizeLayers
//=============================================================================
PRI::PRI( basAnc* b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
{
	FixUp();
}	// PRI::PRI
//---------------------------------------------------------------------------
/*virtual*/ void PRI::Copy( const record* pSrc, int options/*=0*/)
{	record::Copy( pSrc, options);
	// base class calls FixUp() and (if _DEBUG) Validate()
}		// PRI::Copy
//-----------------------------------------------------------------------------
/*virtual*/ void PRI::FixUp()	// fix links
// called from basAnc::reAl() and this c'tor, Copy()
{	x.xs_Init( this);
}		// PRI::FixUp
//-----------------------------------------------------------------------------
RC PRI::pr_Ckf(
	[[maybe_unused]] int options)	// 1
{
	RC rc = RCOK;
	FixUp();

	return rc;
}		// PRI::pr_Ckf
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
XSRAT::XSRAT( basAnc* b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
{
	FixUp();
}	// XSRAT::XSRAT
//---------------------------------------------------------------------------
/*virtual*/ XSRAT::~XSRAT()
{
	delete xr_kivaAggregator;
	xr_kivaAggregator = NULL;
}	// XSRAT::~XSRAT
//---------------------------------------------------------------------------
/*virtual*/ void XSRAT::Copy( const record* pSrc, int options/*=0*/)
{	record::Copy( pSrc, options);
	// base class calls FixUp() and (if _DEBUG) Validate()
}		// XSRAT::Copy
//-----------------------------------------------------------------------------
/*virtual*/ void XSRAT::FixUp()		// fix links
// called from basAnc::reAl() and this c'tor, Copy()
{	x.xs_Init( this, 1);		// 1=fixup linkage only, don't change data
}		// XSRAT::FixUp
//-----------------------------------------------------------------------------
/*virtual*/ RC XSRAT::Validate(
	int options/*=0*/)		// options bits
							//  0x100: XSURF full check
{	RC rc = record::Validate( options);
	if (rc == RCOK)
	{	if (x.xs_pParent != this)
			rc |= errCrit( WRN, "XSRAT::Validate: Bad XSURF pParent");
	}
	if (rc == RCOK)
		rc = x.xs_Validate( options);
	return rc;
}		// XSRAT::Validate
//=============================================================================
XSURF::XSURF()
{	xs_Init( NULL, 0);		// owner must setup parent linkage
}	// XSURF::XSURF
//-----------------------------------------------------------------------------
XSURF::~XSURF()
{	xs_DeleteFENAW();
}		// XSURF::~XSURF
//-----------------------------------------------------------------------------
void XSURF::xs_DeleteFENAW()
{	for (int iFA=0; iFA<2; iFA++)
	{	delete xs_pFENAW[ iFA];
		xs_pFENAW[ iFA] = NULL;
	}
}		// XSURF::xs_DeleteFENAW
//-----------------------------------------------------------------------------
void XSURF::xs_Init(			// initialize
	record* pParent,		// parent record
	int options /*=0*/)		// option bits
							//   0 (default): full init (unconditionally NULLs ptrs, *caution*)
							//   1: fixup only (re link subobjects)
{	xs_pParent = pParent;
	if (options&1)
	{	// fixup, don't change data
		xs_sbcI.sb_pXS = this;
		xs_sbcO.sb_pXS = this;
		for (int iFA=0; iFA<2; iFA++)
		{	// relink FENAWs if they exist
			if (xs_pFENAW[ iFA])
				xs_pFENAW[ iFA]->fa_pXS = this;
		}
	}
	else
	{	// full init
		xs_sbcI.sb_Init( this, 0);
		xs_sbcO.sb_Init( this, 1);
		xs_DeleteFENAW();		// no ASHWAT

		// layer boundary temps: init to "unknown"
		// Due to layer merging, some tLrBs not available
		// They are never set and are reported as -99.
		VSet( xs_tLrB, XSMXTLRB, -99.f);
	}
}		// XSURF::xs_Init
//-----------------------------------------------------------------------------
XSURF& XSURF::Copy( const XSURF* pXS, [[maybe_unused]] int options /*=0*/)
{	record* pParent = xs_pParent;		// save parent ptr (set by c'tor)
	memcpy( this, pXS, sizeof( XSURF));	// bitwise copy
	xs_pParent = pParent;				// restore parent ptr
	xs_Init( xs_pParent);	// fix sub-objects
							// (deletes FENAWs, xs_SetRunConstants remakes)
	return *this;
}		// XSURF::Copy
//-----------------------------------------------------------------------------
RC XSURF::xs_Validate(
	int options/*=0*/)		// option bits
							//   0x100: full check (all linkages s/b complete
							//			and consistent)
							//   else: partial check, OK during input stages
{
	BOOL bFullCheck = (options & 0x100) != 0;

	RC rc = RCOK;
	if (xs_ty <= CTNONE || xs_ty > CTMAXVALID)
		rc |= errCrit( WRN, "XSURF '%s': bogus ty %d", xs_Name(), xs_ty);
	for (int si=0; si<2; si++)
	{	if (xs_SBC( si).sb_pXS != this)
			rc |= errCrit( WRN, "XSURF '%s': Bad SBC link", xs_Name());
	}
	if (bFullCheck)
	{	if (xs_sbcI.sb_zi <= 0 || xs_sbcI.sb_zi != xs_GetZi( 0))
			errCrit( WRN, "XSURF '%s': inside zone inconsistency", xs_Name());
		if (xs_sbcO.sb_zi != xs_GetZi( 1))
			errCrit( WRN, "XSURF '%s': outside zone inconsistency", xs_Name());
		if (!xs_IsASHWAT())
		{	if (xs_pFENAW[ 0] != NULL || xs_pFENAW[ 1] != NULL)
				rc |= errCrit( WRN, "XSURF '%s': FENAW link(s) for non-ASHWAT surface", xs_Name());
		}
		else
		{	if (xs_pFENAW[ 0] == NULL && xs_pFENAW[ 1] == NULL)
				rc |= errCrit( WRN, "XSURF '%s': FENAW link(s) missing for ASHWAT surface", xs_Name());
		}
		if (xs_ty == CTMXWALL)
		{	if (!xs_msi)
				rc |= errCrit( WRN, "XSURF '%s': No mass for CTMXWALL", xs_Name());
			else
			{	MSRAT* ms = MsR.GetAt( xs_msi);	// associated mass
				ZNR* zp = ZrB.GetAt( xs_pParent->ownTi);
				for (int si = 0;  si < 2;  si++)			// loop over sides 0 (inside) and 1 (outside)
				{	MASSBC* side = si ? &ms->outside : &ms->inside;
					if (si == 0)
					{	ZNR* zp2 = ZrB.GetAt( side->bc_zi);		// point to zone info for zone to which side is exposed
						if (zp != zp2)
							errCrit( WRN, "XSURF '%s': Mass zone mismatch", xs_Name());
					}
					// else outside?
				}
			}
		}
		else
		{	if (xs_msi)
				rc |= errCrit( WRN, "XSURF '%s': Unexpected mass %d for non-CTMXWALL", xs_Name(), xs_msi);
		}
	}
	// ASHWAT back pointer should always be good if present
	for (int iFA=0; iFA<2; iFA++)
	{	if (xs_pFENAW[ iFA] && xs_pFENAW[ iFA]->fa_pXS != this)
			rc |= errCrit( WRN, "XSURF '%s': Bad FENAW link", xs_Name());
	}
	return rc;
}		// XSURF::Validate
//-----------------------------------------------------------------------------
const char* XSURF::xs_Name() const
{	return xs_pParent ? xs_pParent->name : "?";
}		// XSURF::xs_Name
//-----------------------------------------------------------------------------
float XSURF::xs_AreaGlazed() const
{
	// TODO: complete xs_AreaGlazed
	return xs_area * xs_fMult;
}		// XSURF::xs_AreaGlazed
//-----------------------------------------------------------------------------
int XSURF::xs_Class() const
{
	const SFI* sf = dynamic_cast< SFI*>( xs_pParent);
	return sf ? sf->sfClass : sfcNUL;
}		// XSURF::xs_Class
//-----------------------------------------------------------------------------
int XSURF::xs_CanHaveExtSlr() const	// TRUE iff surface outside can receive ambient solar gain
// Note: FALSE return does not mean that outside of interzone surfaces won't receive
//       distributed gain
{	return sfAdjZi == 0
		&& sfExCnd == C_EXCNDCH_AMBIENT
        && xs_ty != CTPERIM && xs_ty != CTINTWALL;
}		// XSURF::xs_CanHaveExtSlr
//-----------------------------------------------------------------------------
RC XSURF::xs_SetUNom()			// derive/set nominal U-factor
// also sets xs_UANom, xs_rSrfNom, xs_hSrfNom, and ms_UNom
{
static float rSrfASHRAE[ 3][ 2] =
//   down      up
{	.92f,     .61f,		// ceiling
	.68f,     .68f,		// wall
	.61f,     .92f		// floor
};

	if (xs_IsASHWAT())
	{	xs_rSrfNom[ 0] = RinNFRC;
		xs_rSrfNom[ 1] = RoutNFRC;
		xs_UNom = xs_UNFRC;
		// xs_pFENAW[ 0]->DoSomething();
	}
	else
	{
		int xsty = xs_TyFromTilt();		// tilt-type: xstyCEILING, xstyWALL, xstyFLOOR

		// inside
		xs_rSrfNom[ 0] = rSrfASHRAE[ xsty][ 1];

		// outside
		if (sfExCnd == C_EXCNDCH_ADIABATIC)
			// no transfer, r and h both 0
			xs_rSrfNom[ 1] = 0.f;
		else if (sfExCnd == C_EXCNDCH_AMBIENT)
			xs_rSrfNom[ 1] = .17f;
		else if (sfExCnd == C_EXCNDCH_ADJZN)
			xs_rSrfNom[ 1] = rSrfASHRAE[ xsty][ 0];
		else if (sfExCnd == C_EXCNDCH_GROUND)
			// ground: hxa=1/Rx, Rx=res to ground temp
			xs_rSrfNom[ 1] = xs_sbcO.sb_hxa > 0.f
								? 1.f / xs_sbcO.sb_hxa
								: 0.f;
		else if (sfExCnd == C_EXCNDCH_SPECT)
			xs_rSrfNom[ 1] = uX > 0. ? 1.f/uX : 0.f;
#if defined( _DEBUG)
		else
			ASSERT( 1);		// missing case
#endif
		float rSrfTot = xs_rSrfNom[ 0] + xs_rSrfNom[ 1];
		xs_UNom = seriesU( uC, 1.f/rSrfTot);
	}

	for (int si=0; si<2; si++)
		xs_hSrfNom[ si] = xs_rSrfNom[ si] > 0.f ? 1.f/xs_rSrfNom[ si] : 0.f;

#if 1	// 10-28-2017
	// set/update derived values
	//   known case: xs_SetUNom is called a 2nd time for C_EXCNDCH_GROUND
	//               xs_UNom value changes
	xs_UANom = xs_area * xs_UNom;
	if (xs_msi)
	{	MSRAT* ms = MsR.GetAt( xs_msi);
		ms->ms_UNom = xs_UNom;
	}
#endif

	return RCOK;

}		// XSURF::xs_SetUNom

#if 0
	float tilt,
	int si,
	int bStill)
{


const float RSrfHoU =
const float RSrf45U =
const float
const float RSurfDfltI = float( 1./1.46);	// default nominal inside surf resistance,
											//   ft2-F/Btuh (approx 0.68)
const float RSurfDfltO = float( 1./6.00);	// default nominal outside surf resistance,
											//   ft2-F/Btuh (approx 0.17)


  0.61f // interior, horizontal, up

const float RSURF = 0.68f;

	if (bStill)



	else
		rSrf = 0.17f;

	return rSrf;
}
//-----------------------------------------------------------------------------
float RSurfASHRAE(		// standard ASHRAE surface resistance
	float tilt,		// tilt of *inside* face of surface wrt horiz, deg
					//   0 = ceiling, 90=wall, 180=floor
	int si,			// side of surface of interest (0=inside, 1=outside)
	int bMoving,
	int hc =
{
	if (bMoving)
		return bHeating ? 0.17f : .25f;

#endif
//-----------------------------------------------------------------------------
void XSURF::xs_AccumZoneValues()		// accumulate zone values
// Note: zone value initialized in zn_InitSurfTotals
{
	ZNR& z = ZrB[ xs_GetZi( 0)];
	if (xs_ty != CTMXWALL)  						// only non-light type
	{	if (sfExCnd==C_EXCNDCH_SPECT)				// if exposed to specified temp
			z.zn_uaSpecT += xs_area * xs_uval;		// separate hsu specT lite surf ua sum
		else if (sfExCnd==C_EXCNDCH_AMBIENT)		// else if exposed to ambient (not ADJZN nor ADIABATIC, 2-95)
			z.zn_ua += xs_area * xs_uval;			// sum ambient light surf & perimeter UA's to zone->ua.
	}
	if (sfExCnd==C_EXCNDCH_AMBIENT)
		z.zn_UANom += xs_UANom;

}	// XSURF::xs_AccumZoneValues()
//-----------------------------------------------------------------------------
void XSURF::xs_SetRunConstants()
{
	int dbPrint = 0;
#if defined( DEBUGDUMP)
	dbPrint = DbDo( dbdCONSTANTS);
	if (dbPrint)
	{	xs_sbTest();		// prints hcNat constants for range of tilts (once)
		DbPrintf( "Surface '%s': tilt=%0.f\n",
			xs_Name(), DEG( tilt));
	}
#endif

	if (!xs_IsPerim())
	{	xs_sbcI.sb_SetRunConstants(dbPrint);
		xs_sbcO.sb_SetRunConstants(dbPrint);
	}

	xs_DeleteFENAW();		// insurance
	if (xs_IsASHWAT())
	{	if (ASHWAT.xw_Setup() == RCOK)
		{	xs_pFENAW[ 0] = new FENAW( this);
			xs_pFENAW[ 0]->fa_Setup( 0);
			if (xs_HasControlledShade())
			{	xs_pFENAW[ 1] = new FENAW( this);
				xs_pFENAW[ 1]->fa_Setup( 1);
			}
		}
	}
}		// XSURF::xs_SetRunConstants
//-----------------------------------------------------------------------------
#if defined( DEBUGDUMP)
void XSURF::xs_sbTest()
{
static float tilts[] = { 0.f, 1.f, 10.f, 20.f, 40.f, 60.f, 80.f, 90.f,
                       100.f, 120.f, 140.f, 160.f, 170.f, 179.f, 180.f,
					   -999.f };
static int bTested = 0;
	if (bTested || !xs_sbcI.sb_HasHcNat() || !xs_sbcO.sb_HasHcNat())
		return;		// skip if no natural convection (won't set sb_hcConst)
	bTested++;
	float tiltSave = tilt;
	DbPrintf( "\n     ----- inside ------   ----- outside -----\n"
			    "tilt theta ta<=ts  ta>ts   theta ta<=ts  ta>ts\n");

	for (int iT=0; tilts[ iT] > -990.f; iT++)
	{	tilt = RAD( tilts[ iT]);
		xs_sbcI.sb_SetRunConstants( 0);
		xs_sbcO.sb_SetRunConstants( 0);
		DbPrintf( "%4.f  %4.f %6.3f %6.3f    %4.f %6.3f %6.3f\n",
			DEG( tilt),
			xs_sbcI.sb_atvDeg, xs_sbcI.sb_hcConst[ 0], xs_sbcI.sb_hcConst[ 1],
			xs_sbcO.sb_atvDeg, xs_sbcO.sb_hcConst[ 0], xs_sbcO.sb_hcConst[ 1]);
	}
	tilt = tiltSave;
	DbPrintf( "\n");
}
#endif	// DEBUGDUMP
//-----------------------------------------------------------------------------
int SBC::sb_HasHcNat() const	// TRUE iff natural convection occurs at this boundary
{
	return sb_zi > 0								// sees zone
		|| sb_pXS->sfExCnd == C_EXCNDCH_AMBIENT;	// or sees ambient
}		// SBC::sb_HasHcNat
//-----------------------------------------------------------------------------
void SBC::sb_SetRunConstants(		// set mbrs that do not change during simulation
	int dbPrint)		// nz: DbPrintf coefficients
// set mbrs that do not change during simulation
// sb_Fp must be set
{
	XSURF& x = *sb_pXS;
	VSet( sb_hcConst, 3, 0.);
	// local vars, used variously per model
	double wf = 0.;			// average wind factor
	double uf = 0.;			// unit conversion factor (e.g. mph -> ft/sec)
	double vf = 0.;			// wind velocity correction
	double lf = 0.;			// characteristic length factor

	// common values, not all used by all models

	// ATV = angle to vertical (aka theta in Niles memos, re convective coeff models)
	// case         tilt       atv   inside faces
	//  ceiling        0       -90      dn
	//  cath ceiling  30       -60      dn
	//  wall          90         0     (dn)
	//  tilted wall   95         5      up
	//  floor        180        90      up
	double tiltD = DEG( x.tilt);
	snapDEG( tiltD);	// snap to exactly -90, 0, 90
	sb_atvDeg = bracket( -90., tiltD - 90., 90.);
	if (sb_si && sb_atvDeg != 0.)
		sb_atvDeg = -sb_atvDeg;		// outside
	double absAtvDeg = abs( sb_atvDeg);
	sb_cosTilt = sb_si ? cos(x.tilt) : -cos(x.tilt);
	sb_cosAtv = cos( RAD( sb_atvDeg));	// 0 <= cos( theta) <= 1 always (never < 0)
	snapTrig( sb_cosAtv);
	int xsty = x.xs_TyFromTilt();		// tilt-type: xstyCEILING, xstyWALL, xstyFLOOR
	ASSERT( xstyCEILING == 0);
	ASSERT( xstyWALL == 1);
	ASSERT( xstyFLOOR == 2);

	if (sb_HasHcNat())
	{	// natural convection occurs at surface
		double hcNatConstHtFlowEnhanced, hcNatConstHtFlowReduced;
		if (sb_hcModel == C_CONVMODELCH_ASHRAE)
		{	// ASHRAE zone-side still air values used by e.g. 1199-RP
			// Table order assumed consistent with xstyCEILING etc
			static float hcNatASHRAE[ 3][ 2] =
			//   down        up
			{	.162f,     .712f,		// ceiling
				.542f,     .542f,		// wall
				.162f,     .712f		// floor
			};
			// TODO: generalize for arbitrary tilt?
			// TODO: handle non-canonical tilts (e.g. -5, 190, ...)?
			hcNatConstHtFlowReduced = hcNatASHRAE[ xsty][ 0];
			hcNatConstHtFlowEnhanced = hcNatASHRAE[ xsty][ 1];
			sb_hcConst[ 2] = Top.tp_hConvF * 0.88;
		}
		else if (sb_hcModel == C_CONVMODELCH_UNIFIED)
		{	
			// set up but not used for other
			double hcNatConstBase = 0.185 * pow( sb_cosAtv, 1./3.);
			hcNatConstHtFlowEnhanced = absAtvDeg > 60.
									? 0.00377*absAtvDeg - 0.079
									: hcNatConstBase;
			hcNatConstHtFlowReduced = max( hcNatConstBase, .08);
		}
		else 
		{
			// Used for C_CONVMODELCH_TARP and C_CONVMODELCH_DOE2 (unused for others)
			hcNatConstHtFlowEnhanced = 1.670f / (7.238f - std::abs(sb_cosTilt));  // 1.670 = 9.482 W/m^2-K^4/3
			hcNatConstHtFlowReduced = 0.262f / (1.382f + std::abs(sb_cosTilt));  // 0.262 = 1.810 W/m^2-K^4/3
		}
		int iFaceUp = sb_atvDeg > 0.;		// 1 iff surface faces upward
		// Top.tp_hConvF: elevation factor for convection
		sb_hcConst[ iFaceUp]   = Top.tp_hConvF * hcNatConstHtFlowReduced;		// const for ta > ts (when face up)
		sb_hcConst[ 1-iFaceUp] = Top.tp_hConvF * hcNatConstHtFlowEnhanced;		// const for ta <= ts (when face up)
	}

	if (sb_zi)
	{	// exposed to zone
		sb_frRad = sigmaSB4 * sb_Fp;
		sb_hCX.hc_Init( .5);		// init hc history to 0.5
									//   re maintenance of moving average
#if defined( CONV_ASHRAECOMPARE)
		sb_hCXAsh.hc_InitAsh( float( tiltD), sb_si);
#endif
	}
	else if (x.sfExCnd == C_EXCNDCH_GROUND)
	{	// Rx = "extra" resistance that results in
		//   overall steady state resistance calc'd by
		//   BG model (or per input)
		// sb_hxa = 1/Rx to couple surf exterior to ground temp
		float Rx = max( 0.f, sb_rGrnd - sb_rConGrnd);
		sb_hxa = Rx > 0.f ? 1.f/Rx : 0.;
		x.xs_SetUNom();	// re-derive U-factor using
						//   adjusted uC and final sb_hxa
						//   (doc only)
	}
	else if (x.sfExCnd == C_EXCNDCH_ADIABATIC
		  || x.sfExCnd == C_EXCNDCH_SPECT)
	{	// nothing required

	}
	else if (x.sfExCnd == C_EXCNDCH_AMBIENT)
	{	// exposed to ambient
		const ZNR& z = ZrB[ x.xs_GetZi( 0)];		// parent zone

		// estimate characteristic length(s)
		//  (not used by all models)
		float ar = 1.f;				// ratio: length / width
		double A = x.xs_area;		// surface area, ft2
		sb_widNom = sqrt( A / ar);	// shortest width of roof.
		sb_lenNom = ar * sb_widNom;
		double P = 2. * (sb_widNom + sb_lenNom);
		sb_lenCharNat = A / P;						// characteristic length for nat conv
		double L1 = 4. * sqrt( A) / P;
		double L2 = 4. * A / P;
		sb_lenEffWink = (.899 - .032 * L1) * L2;	// effective length for forced conv

		if (sb_hcModel == C_CONVMODELCH_WALTON)
		{	// Walton wind (forced) convective
			// see Niles CZM documentation
			wf = .78;					// average wind factor
			uf = sqrt( 88./60.);		// unit conversion factor mph^.5 -> ft/sec^.5
			double alpha = 0.33;		// assumed exponent = "towns and cities" (see Niles)
			vf = pow( .5, alpha/2.);	// velocity correction: need vel at mid-height
										//    = sqrt( .5^alpha)

			// characteristic length factor, nominally area/perimeter
			//   Walton based formulation on eaveZ/4
			//   safe for eaveZ < 0
			lf = 1. / sqrt( max( z.i.zn_HeightZ( 1.f)/4.f, 1.));
			sb_fcWind = 0.447 * wf * x.xs_Rf * uf * vf * lf;	// x.xs_Rf = roughness factor
		}
		else if (sb_hcModel == C_CONVMODELCH_DOE2)
		{
			sb_fcWind = Top.tp_hConvF * x.xs_Rf;
		}
		else
		{
			// wind direction factor
			//   wall-ish surfaces have reduced hc due to varying wind direction
			wf = xsty == xstyWALL ? .63 : 1.;

			uf = 5280./3600.;	// units: base formula uses ft/sec

			double vfZ =  xsty == xstyFLOOR ? z.i.zn_HeightZ( 0.f)		// floor height
						: xsty == xstyWALL  ? z.i.zn_HeightZ( 1.f)		// "top" height (eaves)
						:                     z.i.zn_HeightZ( 1.f) + 9.8f;
													// top + additional implicit in Clear et al.
													//   roof correlations
			vf = Top.tp_WindFactor( max( vfZ, .001), z.i.zn_infShld);
			// top.windF *is* applied (via Top.windSpeedSqrtSh)

			lf = sb_hcLChar > 0.f ? pow( sb_hcLChar, -.2f) : 1.f;

			// factor for hcFrc = f * V^.8
			// x.xs_Rf = roughness factor
			sb_fcWind = Top.tp_hConvF * 0.527 * wf * x.xs_Rf * lf * pow( uf * vf, 0.8);

			// factor for sb_eta
			sb_fcWind2 = .06 * sb_hcLChar * 1./(uf * uf * vf * vf);
		}

		// radiant: surface sees mix of sky and air temp
		//   ground and near-horizon sky taken as air temp
		//   cos( tilt/2) adjustment guesstimates near-horizon sky
		double cosHalfTilt = cos( x.tilt/2.);
		sb_fSky = x.vfSkyLW * cosHalfTilt;
		sb_fAir = x.vfSkyLW * (1. - cosHalfTilt) + x.vfGrndLW;
		sb_frRad = sigmaSB4 * sb_epsLW;
	}
	else
		errCrit( WRN, "Missing SBC::sb_SetRunConstants() code");

	if (dbPrint)
	{
		const char* hcModelTx = ::getChoiTxI( DTCONVMODELCH, sb_hcModel);
		if (sb_zi)
		{	DbPrintf( "  %d %s %s hcNatConst[ ta<=ts]=%0.3f hcNatConst[ ta>ts]=%0.3f",
				sb_si, ZrB[ sb_zi].name, hcModelTx, sb_hcConst[ 0], sb_hcConst[ 1]);
		}
		else
		{	const char* hcExCndTx = ::getChoiTxI( DTEXCNDCH, x.sfExCnd);
			if (x.sfExCnd != C_EXCNDCH_AMBIENT)
				hcModelTx = "";
			DbPrintf( "  %d %s %s hcNatConst[ ta<=ts]=%0.3f  hcNatConst[ ta>ts]=%0.3f  LChar=%0.1f\n"
					  "     wf=%0.3f  uf=%0.3f  rf=%0.3f  lf=%0.3f  vf=%0.3f  fcWind=%0.3f  fcWind2=%0.3f",
				sb_si, hcExCndTx, hcModelTx, sb_hcConst[ 0], sb_hcConst[ 1], sb_hcLChar,
				wf, uf, x.xs_Rf, lf, vf, sb_fcWind, sb_fcWind2);
		}
		DbPrintf( "\n");
	}
}		// SBC::sb_SetRunConstants
//-----------------------------------------------------------------------------
void SBC::sb_SetCoeffs(		// set convective and radiant coefficients
	double area,	// surface area, ft2;
	double uC)		// surface-to-surface conductance, Btuh/ft2-F
// area and uC may differ from parent values re ASHWAT window frames
// ASHWAT or not, sb_pXS must point to parent XSURF

// call at beg of step
{
// TODO: temps initialized in sb_Init(), may not be appropriate there, 10-10

// NOTE: Some values pre-set in sb_SetRunConstants for some cases.
//       Do not add general inits here w/o review
	if (sb_pXS->xs_IsPerim())
		sb_qrAbs = 0.;
	else if (sb_zi)		// if exposed to zone
	{	// boundary is adjacent to zone
		//   sb_txa, sb_txr set at end of prior step
		if (!sb_pXS->xs_IsKiva())
		{
			sb_HCZone();	// convection
			sb_hxa = sb_hcMult * (sb_hcNat + sb_hcFrc);
			sb_hxr = sb_frRad * pow3(DegFtoR(0.5*(sb_tSrf + sb_txr)));
		}
		sb_qrAbs = area > 0. ? sb_sgTarg.st_tot / area : 0.;
	}
	else if (sb_pXS->sfExCnd==C_EXCNDCH_SPECT)
	{	sb_txa = sb_pXS->sfExT;
		sb_txr = sb_txa;
		sb_hcNat = sb_pXS->uX;
		sb_hcFrc = 0.f;
		sb_hxa = sb_hcMult * sb_hcNat;
		sb_hxr = 0.f;
	}
	else if (sb_pXS->sfExCnd == C_EXCNDCH_GROUND)
	{	// ground: effective temp as combination of lagged temps
		// sb_hcNat = sb_hcFrc = 0.
		// sb_hxa set in sb_SetRunConstants
		// sb_hcMult unused
		sb_txa = (  sb_cTaDbAvgYr * Wfile.taDbAvgYr
				  + sb_cTaDbAvg31 * Wthr.d.wd_taDbAvg31
				  + sb_cTaDbAvg14 * Wthr.d.wd_taDbAvg14
				  + sb_cTaDbAvg07 * Wthr.d.wd_taDbAvg07
				  + sb_cTGrnd * Wthr.d.wd_tGrnd) * sb_rGrnd;
		sb_txr = sb_txa;	// no radiation, set sb_txr as insurance
	}
	else if (sb_pXS->sfExCnd==C_EXCNDCH_ADIABATIC || sb_pXS->xs_IsPerim())
	{	sb_hcNat = sb_hcFrc = sb_hxr = sb_hxa = 0.;
	}
	else
	{	// boundary is adjacent to ambient
		sb_txa = Top.tDbOSh;
		double hrSky = sb_frRad * sb_fSky * pow3( DegFtoR( 0.5*(sb_tSrf + Top.tSkySh)));
		double hrAir = sb_frRad * sb_fAir * pow3( DegFtoR( 0.5*(sb_tSrf + Top.tDbOSh)));
		sb_hxr = hrSky + hrAir;
		sb_txr = sb_hxr > 0.f ? (hrSky*Top.tSkySh + hrAir*Top.tDbOSh) / sb_hxr : 0.f;

		sb_HCAmbient();
		sb_hxa = sb_hcMult * (sb_hcNat + sb_hcFrc);
		sb_qrAbs = sb_absSlr * sb_sgTarg.st_tot;
	}

	sb_sg = sb_qrAbs * area;	// total solar gain (Btuh, not Btuh/ft2)


	if (sb_zi)
	{	sb_hxa = sb_hCX.hc_HC( sb_hxa);
#if defined( CONV_ASHRAECOMPARE)
		if (sb_hcModel == C_CONVMODELCH_ASHRAE && !Top.isWarmup)
		{	float hxaAsh = sb_hcMult * (sb_hcNatAsh + sb_hcFrc);
			if (fabs( hxaAsh - sb_hxa) > .01)
				printf( "Mismatch\n");
		}
#endif
	}

	sb_hxtot = sb_hxa + sb_hxr;		// frequently used sum

	if (uC > 0.)
	{	sb_uRat = uC/(sb_hxtot + uC);
		sb_fRat = sb_uRat * sb_hxtot;
	}
	else
		sb_uRat = sb_fRat = 0.;

	// adjacent environmental temp
	//   inside: set but believed unused
	//   outside: simplifies calcs
	sb_txe = sb_hxtot > 0. ? (sb_txa*sb_hxa + sb_txr*sb_hxr) / sb_hxtot : 0.f;

#if defined( DEBUGDUMP)
	if (DbDo( dbdSBC))
	{	if (sb_si == 0)
			DbPrintf( "Surface '%s'\n", sb_pXS->xs_Name());
		DbPrintf( "  %d eta=%0.3f  hcNat=%0.3f  hcFrc=%0.3f  hc=%0.3f  hr=%0.3f  qrAbs=%0.1f"
			      "  tSrf=%0.2f  txa=%0.2f  txr=%0.2f  txe=%0.2f\n",
				sb_si, sb_eta, sb_hcNat, sb_hcFrc, sb_hxa, sb_hxr, sb_qrAbs,
				sb_tSrf, sb_txa, sb_txr, sb_txe);
	}
#endif

}	// SBC::sb_SetCoeffs
//-----------------------------------------------------------------------------
#define NEWRACOR		// define to use revised Ra correlation (matches UZM changes, 12-31-2011)
//-----------------------------------------------------------------------------
static inline double RayleighNat( double tFilm, double lenChar, double absTDif)
// Rayleigh number for natural convection
{
#if defined( NEWRACOR)
	return 3.9*exp(-0.00764*tFilm) * 1000000. * pow3( lenChar) * absTDif;
#else
	return (5.68 - .956 * log( tFilm)) * 1000000. * pow3( lenChar) * absTDif;
#endif
}		// ::RayleighNat
//-----------------------------------------------------------------------------
void SBC::sb_HCAmbient()			// ambient surface convection coefficients
// sets sb_hcNat and sb_hcFrc
{
	[[maybe_unused]] BOOL bDbPrint =
#if defined( DEBUGDUMP)
		DbDo( dbdRCM);
#else
		FALSE;
#endif

	double TD = sb_txa - sb_tSrf;	// TD > 0 means heat flow into surface

	switch (sb_hcModel)
	{
	case C_CONVMODELCH_AKBARI:
		// Akbari correlation:
		sb_hcFrc = .0958f * Top.windSpeedSh + 1.1939f;    // Btuh/ft2-F for windspeed in mph.
		sb_hcNat = 0.f;
		break;

	case C_CONVMODELCH_WALTON:
		// natural convection
		sb_hcNat = 0.1898*pow( fabs( TD), .333);
		// + wind (forced) convection
		sb_hcFrc = sb_fcWind * Top.windSpeedSqrtSh;
		break;

	case C_CONVMODELCH_DOE2:
	{
		// MoWitt forced + TARP natural
		sb_hcNat = sb_hcConst[TD>0.] * pow(fabs(TD), 1. / 3.);

		// Windward/Leeward
		float angleDiff = std::fmod(std::abs(Top.windDirDegHr - DEG(sb_pXS->azm)),360.f);
		bool windward = std::abs(sb_cosTilt) > 0.98 || (angleDiff < 100 || angleDiff > 260);

		// From "Identifying and Resolving Issues in EnergyPlus and DOE-2 Window Heat Transfer Calculations" by Booten, Kruis, and Christensen (2012)
		// Using DOE-2 coefficients (correlated to local windspeed at zone hight--change if we ever use surface hight).
		static const float mowittCoeffs[2][2] =
		{   // a     b
			0.2639f, 0.89f,  // Windward 0.2639 = 2.99 Btu/hr-ft2-R-(knot)^0.89
			0.3659f, 0.617f  // Leeward 0.2804 = 3.99 Btu/hr-ft2-R-(knot)^0.617
		};

		auto parentZone = sb_pXS->xs_sbcI.sb_zi;  // Get surface parent zone
		float windSpeedModifier = 0;
		if (parentZone != 0) {
			windSpeedModifier = ZrB[parentZone].i.zn_windFLkg;
		}

		float hfGlass = mowittCoeffs[windward][0]* pow(Top.windSpeedSh*windSpeedModifier, mowittCoeffs[windward][1]);
		
		float hcGlass = std::sqrt(sb_hcNat*sb_hcNat + hfGlass*hfGlass);

		sb_hcFrc = sb_fcWind *(hcGlass - sb_hcNat);
		break;
	}
	case C_CONVMODELCH_UNIFIED:
	{
#if 0 && defined( _DEBUG)
x		if (bDbPrint && Top.iHr == 15)
x			printf( "Hit\n");
#endif
		if (Top.windSpeedSquaredSh > 0.f)
		{	sb_eta = TD != 0.
						? 1./(1.+1./log( 1. + sb_fcWind2 * fabs( TD) / Top.windSpeedSquaredSh))
						: 0.;
			sb_hcFrc = sb_fcWind * Top.windSpeedPt8Sh;
		}
		else
		{	sb_eta = 1.;
			sb_hcFrc = 0.;
		}
		sb_hcNat = sb_eta * sb_hcConst[ TD>0.] * pow( fabs( TD), 1./3.);
		break;
	}

	case C_CONVMODELCH_WINKELMANN:
	{
#if 0 && defined( _DEBUG)
x		if (bDbPrint && Top.iHr == 15)
x			printf( "Hit\n");
#endif

		float Rf = sb_pXS->xs_Rf;

		double absTDiff = fabs( TD);
		double tFilm = (sb_tSrf + sb_txa) / 2.f;
#if 0
		absTDiff = 5.;
		tFilm = 100.;
#endif
		double kAir = .013 + .000026 * tFilm;
		const double Pr = .72;

		// Rayleigh & Grashof numbers:
#if defined( _DEBUG)
		if (tFilm < .01)
			printf( "Neg tFilm");
#endif
		// double RaLnat = (5.68 - .956 * log( tFilm)) * 1000000. * pow3( sb_lenCharNat) * fabs( sb_tSrf - sb_txa);

		double RaLnat = RayleighNat( tFilm, sb_lenCharNat, absTDiff);
		double RaLeff = RaLnat * pow3( sb_lenEffWink / sb_lenCharNat);
		double GrLeff = RaLeff / Pr;
		if (GrLeff < 0.00001)
			GrLeff = 1.0;		// prevent x/0

		// Reynolds number
		double Re = (7420. - 17.4 * tFilm) * sb_lenEffWink * Top.windSpeedSh * 88. / 60.;
		if (Re < 0.00001)
			Re = 1.;		// prevent x/0
		double xc = Re > 0. ? 500000. * sb_lenEffWink / Re : 2.*sb_widNom;

		double xceff = min( xc, sb_widNom);
		double Rexceff = Re * xceff / sb_lenEffWink;  // is xceff always < sb_lenEffWink?

		// natural conv weighting factor
		double grRe2 = 1. + GrLeff / (Re*Re);
#if defined( _DEBUG)
		if (grRe2 < .01)
			printf( "Neg grRe2");
#endif
		sb_eta = 1. / (1. + 1. / log( grRe2));

		// calc combined nat and forced conv coef
		if (sb_tSrf > sb_txa)
		{	// exterior conv heat flow is up
			sb_hcNat = sb_eta * (kAir / sb_lenCharNat) * .15 * pow( RaLnat, .333);
			sb_hcFrc = (kAir / sb_lenEffWink) * Rf * .037 * pow( Re, .8) * pow( Pr, .333);
		}
		else if (xceff > sb_lenNom)
		{	sb_hcNat = sb_eta * (kAir / sb_lenCharNat) * .27 * pow( RaLnat,.25);
			sb_hcFrc = (kAir / sb_lenEffWink) * Rf * .664 * sqrt( Re) * pow( Pr,.333);
		}
		else
		{	sb_hcNat = sb_eta * (kAir / sb_lenCharNat) * .27 * pow( RaLnat,.25);
			sb_hcFrc = (kAir / sb_lenEffWink) * Rf * (   .037 * (pow( Re, .8) - pow( Rexceff, .8))
				                         + .664 * pow( Rexceff, .5)) * pow( Pr, .333);
		}
#if defined( _DEBUG)
		if (sb_hcNat < 0.)
			printf( "Neg sb_hcNat");
#endif
	  }
		break;

	case C_CONVMODELCH_INPUT:
		sb_hcNat = 1.f;
		break;

	default:
		warn( "Unsupported ambient convective model");
		sb_hcFrc = sb_hcNat = 0.f;
	}

}	// SBC::sb_HCAmbient()
//-----------------------------------------------------------------------------
float SBC::sb_CGrnd() const		// total ground conductance
// returns total conductance to ground, Btuh/ft2-F
//       = sum of several conductances to e.g. moving mean temps
{
	float cGrnd = sb_cTaDbAvgYr + sb_cTaDbAvg31
		+ sb_cTaDbAvg14 + sb_cTaDbAvg07 + sb_cTGrnd;
	return cGrnd;
}		// SBC::sb_CGrnd
//-----------------------------------------------------------------------------
void SBC::sb_SetCoeffsWallBG(		// set ground couplings for below grade wall
	float a[])		// coefficient array
					//  a[ 0] unused, a[ 1]=a1 etc
// see BGDeriveCoeffs
{
	sb_cTaDbAvg07 = 0.f;
	sb_cTaDbAvg14 = a[ 1];
	sb_cTaDbAvg31 = 0.f;
	sb_cTaDbAvgYr = a[ 3];
	sb_cTGrnd = a[ 2];
}		// SBC::sb_SetCoeffsWallBG
//-----------------------------------------------------------------------------
void SBC::sb_SetCoeffsFloorBG(		// set ground couplings for below grade floor
	float a4,		// coefficients (Niles notation)
	float a5)
// see BGDeriveCoeffs() and sf_BGFloorSetup()
{
	sb_cTaDbAvg07 = 0.f;
	sb_cTaDbAvg14 = 0.f;
	sb_cTaDbAvg31 = 0.f;
	sb_cTaDbAvgYr = a5;
	sb_cTGrnd = a4;
}		// SBC::sb_SetCoeffsFloorBG
//-----------------------------------------------------------------------------
void SBC::sb_HCZone()		// convective coefficients for surface exposed to zone
{
	ZNR& z = ZrB[ sb_zi];
	double TD = sb_txa - sb_tSrf;

	switch (sb_hcModel)
	{
	case C_CONVMODELCH_ASHRAE:
		// use "system on" value iff prior step active air HVAC
		sb_hcNat = z.zn_IsAirHVACActive() ? sb_hcConst[2] : sb_hcConst[TD>0.];
#if defined( CONV_ASHRAECOMPARE)
		sb_hcNatAsh = sb_hCXAsh.hc_HCAsh( z.zn_IsMechAirMotion(), sb_txa, sb_tSrf);
#endif
		sb_hcFrc = 0.f;
		break;

	case C_CONVMODELCH_UNIFIED:
	{
#if 0 && defined( _DEBUG)
x		if (bDbPrint && Top.iHr == 15)
x			printf( "Hit\n");
#endif
		sb_hcNat = sb_hcConst[ TD>0.] * pow( fabs( TD), 1./3.);
		sb_hcFrc = z.zn_hcFrc;		// all surfaces in zone use same value
		break;
	}

	case C_CONVMODELCH_TARP:
	{	sb_hcNat = sb_hcConst[TD>0.] * pow(fabs(TD), 1. / 3.);
		sb_hcFrc = 0.f;
		break;
	}

	case C_CONVMODELCH_MILLS:
	  {
		// get nat conv h for underside of roof construction:
		// Ref: A.F. Mills, "Heat Transfer", '92; Eq 4.85 & 4.86.
		//  applied to hot all roofs facing downward, and to cold roof facing downward if Theta < 60 deg.
		double tFilm = (sb_tSrf + sb_txa) / 2.;
		double absTDiff = fabs( TD);

		double Lchar = 10.;		// ft; characteristic length
		double theta =  DEG( acos( sb_cosAtv));
		// double Ra = (5.68 - .956 * log( tFilm)) * 1000000. * pow3( Lchar) * absTDiff;
		double Ra = RayleighNat( tFilm, Lchar, absTDiff);
		double Phi = .35;   // Prandtl number correction
		[[maybe_unused]] int mside = sb_si ? 1 : -1;
		BOOL bHtFlowUp = (sb_txa - sb_tSrf) * (sb_si ? -1. : 1.) > 0.;
#if defined( NEWRACOR)
		double kAir = .013 + .000026 * tFilm;
#else
		double kAir = 0.015;
#endif

		if (bHtFlowUp)
		{	sb_hcNat = Ra < 2.e+07
							? (kAir / Lchar) * .54 * pow( Ra, .25)		// laminar
							: (kAir / Lchar) * .14 * pow( Ra, .333);	// turbulent
			if (theta < 89.9)
			{	if (theta < 60.)
					warn( "Unsupported tilt");
				else
				{	// plate for Theta = 60 deg:
					Ra *= 0.5;	// cos( 60)
					double term = Ra > 1.e+09
							? pow( 1 + 1.6e-08 * Ra * Phi, 1./12.)	// turbulent
							: 1.;									// laminar
					double hc60 = kAir * (.68 + .67 * pow( Ra * Phi, .25) * term) / Lchar;
					sb_hcNat =  hc60 * (90 - theta) / 30.
							  + sb_hcNat * (theta - 60.) / 30.;	// interpolation
				}
			}
		}
		else
		{	// heat flow down
			if (sb_cosAtv < .01)
				sb_hcNat = 0.;
			else
			{	Ra *= sb_cosAtv;
				double term = Ra > 1.E+09
								? pow( 1 + 1.6E-08 * Ra * Phi, (1. / 12.))	// turbulent
								: 1.;			// laminar
				sb_hcNat = kAir * (.68 + .67 * pow( Ra * Phi,.25) * term) / Lchar;
			}
		}
		break;
	  }

	case C_CONVMODELCH_INPUT:
		sb_hcNat = 1.f;
		sb_hcFrc = 0.f;
		break;

	default:
		warn( "Unsupported interior convective model");
		sb_hcFrc = sb_hcNat = 0.f;
	}

#if 0
	' routine to get nat conv h for underside of roof construction:
    ' Ref: A.F. Mills, "Heat Transfer", '92; Eq 4.85 & 4.86.
    ' applied to hot all roofs facing downward, and to cold roof facing downward if Theta < 60 deg.
    LOCAL Lchar,cosTheta,Theta,Ra1,Ra,Phi,hc90,hc60 AS SINGLE
    FOR ncom = 1 TO nrf
        Lchar = 10 'ft; characteristic length
        cosTheta = pitch / SQR(1 + pitch ^ 2)' Theta = angle of roof plane from vertical, radians
        Theta = 90.0 - 57.296 * ATN(pitch)
        Tfilm = (Tsurf(u, ncom) + Tair(u)) / 2
        'Ra1 = (5.68 - .956 * LOG(Tfilm)) * 1000000! * Lchar ^ 3 * ABS(Tsurf(u, ncom) - Tair(u)) * cosTheta
        Ra1 = (5.68 - .956 * LOG(Tfilm)) * 1000000! * Lchar ^ 3 * ABS(Tsurf(u, ncom) - Tair(u)) ' kkk/ppp 04/19
        Phi = .35   ' Prandle number correction
    IF Tsurf(u, ncom) < Tair(u) THEN ' use above for theta < 60, and interpolate for 60 <theta <90:
        IF Theta > 60 THEN ' interpolate between hc at Theta = 60 and hc for Theat = 90 (horiz plate):
            ' horiz plate:
            'Ra = Ra1 / cosTheta ' kkk/ppp 04/19
            Ra = Ra1
            IF Ra < 2E+07 THEN 'use laminar case
                hc90 = (.015 / Lchar) * .54 * Ra ^ .25
            ELSE 'Ra > 2E7 so use turbulent case
                hc90 = (.015 / Lchar) * .14 * Ra ^ .333
            END IF
            ' plate for Theta = 60 deg:

            Ra = Ra * COS(60 / 57.3)
            IF Ra > 1E+09 THEN ' flow is turbulent:
                term = (1 + 1.6E-08 * Ra * Phi) ^ (1 / 12)
            ELSE 'flow is laminar:
                term = 1
            END IF
            hc60 = .015 * (.68 + .67 * ((Ra * Phi) ^ .25) * term) / Lchar   'Btu/hr-ft^2-Fio
            hc(u, ncom) = hc60 * (90 - Theta) / 30 + hc90 * (Theta - 60) / 30'interpolation
        END IF
    ELSE ' Tsurf > Tair:
        Ra = Ra1 * cosTheta ' fix as per Phil. revised 04/19 kkk/ppp
        IF Ra > 1E+09 THEN ' flow is turbulent:
            term = (1 + 1.6E-08 * Ra * Phi) ^ (1 / 12)
        ELSE 'flow is laminar:
            term = 1
        END IF
        hc(u, ncom) = .015 * (.68 + .67 * ((Ra * Phi) ^ .25) * term) / Lchar 'Btu/hr-ft^2-Fio
    END IF
    NEXT ncom

    ' routine to get natural conv coef for attic side of ceiling:
    ' Ref: A.F. Mills, "Heat Transfer", '92; Eq 4.95 & 4.96.

    FOR ncom = (nrf + 1) TO (nrf + nceil)
        IF Tsurf(u, ncom) > Tair(u) THEN
           Lchar = 10 ' ft
           Tfilm = (Tsurf(u, ncom) + Tair(u)) / 2
           Ra = (7.893 - 3.0565 * LOG(Tfilm) / LOG(10)) * 1000000! * Lchar ^ 3 * ABS(Tsurf(u, ncom) - Tair(u))
           Ra = (5.68 - .956 * LOG(Tfilm)) * 1000000! * Lchar ^ 3 * ABS(Tsurf(u, ncom) - Tair(u))

           IF Ra < 2E+07 THEN 'use laminar case
               hc(u, ncom) = (.015 / Lchar) * .54 * Ra ^ .25
           ELSE 'Ra > 2E7 so use turbulent case
               hc(u, ncom) = (.015 / Lchar) * .14 * Ra ^ .333
           END IF
        ELSE
           hc(u, ncom) = 0
        END IF
    NEXT ncom

   IF nuz = 2 THEN ' calc hc's, otherwise don't.
        ' conv coef on underside of cz floor:
        u = 2
        FOR ncom = 1 TO nflr
            IF Tsurf(u, ncom) < Tair(u) THEN
                Lchar = 10 ' ft
                Tfilm = (Tsurf(u, ncom) + Tair(u)) / 2
                Ra = (5.68 - .956 * LOG(Tfilm)) * 1000000! * Lchar ^ 3 * ABS(Tsurf(u, ncom) - Tair(u))
                    IF Ra < 2E+07 THEN 'use laminar case
                        hc(u, ncom) = (.015 / Lchar) * .54 * Ra ^ .25
                    ELSE 'Ra > 2E7 so use turbulent case
                        hc(u, ncom) = (.015 / Lchar) * .14 * Ra ^ .333
                    END IF
            ELSE
                hc(u, ncom) = 0
            END IF
        NEXT ncom

        ' conv coef for ground:
        ncom = ncomtw(u) + 1

        IF Tma(u, ncom) > Tair(u) THEN
            Lchar = 10 ' ft
            Tfilm = (Tma(u, ncom) + Tair(u)) / 2
            Ra = (5.68 - .956 * LOG(Tfilm)) * 1000000! * Lchar ^ 3 * ABS(Tma(u, ncom) - Tair(u))
            IF Ra < 2E+07 THEN 'use laminar case
                hc(u, ncom) = (.015 / Lchar) * .54 * Ra ^ .25
            ELSE 'Ra > 2E7 so use turbulent case
                hc(u, ncom) = (.015 / Lchar) * .14 * Ra ^ .333
            END IF
        ELSE
            hc(u, ncom) = 0
        END IF
    END IF
#endif

}	// SBC::sb_HCZone
//-----------------------------------------------------------------------------
void ZNR::zn_AccumBalTermsQS(		// zone energy balance terms for quick surface
	double area,			// surface area, ft2
	const SBC& sbcI,		// inside (zone) boundary conditions
	const SBC& sbcO)		// outside boundary conditions
// note: surface conductance is implicit in SBC mbrs
{
	// quick surfaces: accum to zone for convective/radiant model
	double f = sbcO.sb_fRat;		// handy copy
	double den = sbcI.sb_hxtot + f;
	double cc, cr, cx;
	if (den != 0.)
	{	cc = sbcI.sb_hxa * f / den;
		cr = sbcI.sb_hxr * f / den;
		cx = sbcI.sb_hxa * sbcI.sb_hxr / den;
	}
	else
		cc = cr = cx = 0.;

	double sumI = sbcI.sb_hxtot + sbcO.sb_fRat;
	double q = sumI != 0. ? sbcI.sb_qrAbs/sumI : 0.;
	double sumO = sbcO.sb_hxtot + sbcI.sb_fRat;
	if (sumO != 0.)
		q += sbcO.sb_qrAbs*sbcI.sb_uRat/sumO;

	zn_nAirSh += area*(sbcO.sb_txe*cc + q*sbcI.sb_hxa);
	zn_dAirSh += area*cc;
	zn_nRadSh += area*(sbcO.sb_txe*cr + q*sbcI.sb_hxr);
	zn_dRadSh += area*cr;
	zn_cxSh += area*cx;

}		// ZNR::zn_AccumBalTermsQS
//------------------------------------------------------------------------------
void SBC::sb_SetTx()
// copy adjacent temps from zone
{
	if (sb_zi)
	{	ZNR& z = ZrB[ sb_zi];
		sb_txa = z.tz;
		sb_txr = z.zn_tr;
		sb_txe = sb_hxtot > 0. ? (sb_txa*sb_hxa + sb_txr*sb_hxr) / sb_hxtot : 0.f;
	}
	// else do nothing
}		// SBC::sb_SetTx
//--------------------------------------------------------------------------
void ZNR::zn_EndSubhrQS(		// accounting re quick surface xfers
	double area,	// surface area, ft2
	double uC,		// surface-to-surface conductance, Btuh/ft2-F
	SBC& sbcI,		// inside (zone) boundary conditions
	SBC& sbcO)		// outside boundary conditions

// Note: assumes sb_SetTx() has been called

{
	// inside surf temp
	//   note: uC = surf-to-surf conductance
	double numI = sbcI.sb_QHT() + sbcO.sb_QHT()*sbcO.sb_uRat;
	double denI = sbcI.sb_hxtot + uC*(1.-sbcO.sb_uRat);
	sbcI.sb_tSrf = denI != 0. ? numI / denI : 0.;

	// outside surf temp
	double denO = sbcO.sb_hxtot + uC;
	sbcO.sb_tSrf = denO != 0. ? (sbcO.sb_QHT() + sbcI.sb_tSrf*uC) / denO : 0.;

	// conduction per unit area (+ = into zone)
	double qSrf = uC * (sbcO.sb_tSrf - sbcI.sb_tSrf);
	sbcI.sb_qSrf = -qSrf;
	sbcO.sb_qSrf =  qSrf;

	// accum conduction to adjacent zones
	//   note this is conduction rate = Btuh
	double qSrfA = area*qSrf;
	zn_qCondQS += qSrfA;
	if (sbcO.sb_zi)
		ZrB[ sbcO.sb_zi].zn_qCondQS -= qSrfA;
}		// ZNR::zn_EndSubhrQS
//-----------------------------------------------------------------------------
TI XSURF::xs_GetZi(			// get zone idx
	int si) const	// 0: get inside zone idx (always defined)
					// 1: get outside
// returns ZrB idx, 0 if not known (or ambient)
{
	TI zi = 0;
	if (si)
		zi = sfAdjZi;
	else if (xs_pParent)
		zi = xs_pParent->ownTi;
	return zi;
}		// XSURF::xs_GetZi
//-----------------------------------------------------------------------------
int XSURF::xs_IsPerim() const		// nz iff this is a PERIMETER
{
	return xs_ty == CTPERIM;
}		// XSURF::xs_IsPerim
//-----------------------------------------------------------------------------
int XSURF::xs_IsKiva() const		// nz iff surface uses Kiva ground conduction
{
	return xs_ty == CTKIVA;
}	// XSURF::xs_IsKiva
//-----------------------------------------------------------------------------
int XSURF::xs_IsASHWAT() const		// nz iff this is an ASHWAT window
{
	return xs_fenModel == C_FENMODELCH_ASHWAT;
}		// XSURF::xs_IsASHWAT
//-----------------------------------------------------------------------------
int XSURF::xs_HasControlledShade() const
{
	int bCS = scc != sco		// any model: non-equal multipliers
		   || (xs_IsASHWAT()	// ASHWAT: presence of shades
		       && xs_inShd != C_INSHDCH_NONE);
	return bCS;
}		// XSURF::xs_HasControlledShade
//-----------------------------------------------------------------------------
int XSURF::xs_CanBeSGTarget() const		// TRUE iff surface can be solar gain target
{	return xs_ty != CTWINDOW && xs_ty != CTPERIM;
}		// XSURF::xs_CanBeSGTarget
//-----------------------------------------------------------------------------
int XSURF::xs_TyFromTilt() const		// classify by tilt
{
	float tiltD = DEG( tilt);
	int xsty = tiltD > 175.f ? xstyFLOOR
		     : tiltD > 60.f  ? xstyWALL
			 :                 xstyCEILING;
	return xsty;
}	// XSURF::xs_TyFromTilt
//-----------------------------------------------------------------------------
void XSURF::DbDump() const
{	DbPrintf( "XS '%s': uval=%0.3f\n",
		 xs_Name(), xs_uval);
}	// XSURF::DbDump
//=============================================================================
void SBC::sb_Init(
	XSURF* xs,		// parent XSURF
	int si)			// which side of xs? 0=inside, 1=outside
{	sb_pXS = xs;
	sb_si = si;
	sb_zi = xs->xs_GetZi( si);
	sb_txa = 70.f;		// initialize to plausible value
	sb_txr = 70.f;
}		// SBC::sb_Init
//-----------------------------------------------------------------------------
/*virtual*/ double SBC::sb_AreaNet() const			// net area of associated surface, ft2
{	return sb_pXS ? sb_pXS->xs_area : 0.;
}	// SBC::sb_AreaNet
//-----------------------------------------------------------------------------
/*virtual*/ const char* SBC::sb_ParentName() const
{	return sb_pXS ? sb_pXS->xs_Name() : "?";
}		// SBC::sb_ParentName
//-----------------------------------------------------------------------------
/*virtual*/ int SBC::sb_Class() const
{	return sb_pXS ? sb_pXS->xs_Class() : sfcNUL;
}		// SBC::sb_Class
//-----------------------------------------------------------------------------
void SBC::sb_SGFInit()
{
	VZero( sb_sgf[ 0], socCOUNT*sgcCOUNT);
}	// SBC::sb_SGFInit
//-----------------------------------------------------------------------------
void SBC::sb_CalcAwAbsSlr()		// compute area-weighted solar abs
// sets sb_awAbsSlr and accumulates zone values
{
	if (!sb_zi		// if no zone (exterior face)
	 || !sb_pXS		// if no parent surface (insurance)
	 || !sb_pXS->xs_CanBeSGTarget())	// if not targetable surface
		return;		// do nothing

	ZNR* zp = ZrB.GetAt( sb_zi);
	sb_awAbsSlr = sb_absSlr * sb_AreaNet() / zp->zn_surfASlr;
	zp->rmAbs += sb_awAbsSlr; 			// accumulate for zone (modelled as one room)
	if (!sb_pXS->xs_IsDelayed())
		zp->rmAbsCAir += sb_awAbsSlr;	// accumulate portion for non-masses: surface is light if here
}		// SBC::sb_CalcAwAbsSlr
//-----------------------------------------------------------------------------
#if 0
x void SBC::sb_SetTQSrf(
x	double tSrf,
x	double qSrf)
x {
x	sb_tSrf = tSrf;
x	sb_qSrf = qSrf;
x #if 0
xx	sb_BalCheck();
x#endif
x}		// SBC::sb_SetTQSrf
#endif
//-----------------------------------------------------------------------------
void SBC::sb_BalCheck()		// check surface energy balance
// note: not valid check for FD masses ?
{
#if defined( _DEBUG)
	if (!Top.isWarmup)
	{	double qht = sb_QHT() - sb_hxtot*sb_tSrf;	// gains to surface (+ = into surface)
		double aDiff = fabs( qht - sb_qSrf);
		if (aDiff > .01 && aDiff > .001*(fabs( qht)+fabs( sb_qSrf)))
			warn( "Surface '%s' (%s), %s: surface energy balance error (qHT=%.3f vs qSrf=%.3f)",
				sb_ParentName(), sb_SideText(), Top.When( C_IVLCH_S), qht, sb_qSrf);
	}
#endif
}		// SBC::sb_BalCheck
//-----------------------------------------------------------------------------
const char* SBC::sb_SideText()
{
	static const char* stx[ 2] = { "inside", "outside" };
	return stx[ sb_si != 0];
}	// SBC::SideText
//-----------------------------------------------------------------------------
/*static*/ void SBC::sb_DbPrintf( const char* tag, const SBC& sbcO, const SBC& sbcI)
{
	static const char* sbFmt = " %8s tso=%.2f hco=%.3f hro=%.3f qro=%.2f   tsi=%.2f hci=%.3f hri=%.3f qri=%.2f   qSrf=%.2f\n";
	DbPrintf( sbFmt,tag,
				sbcO.sb_tSrf, sbcO.sb_hxa, sbcO.sb_hxr, sbcO.sb_qrAbs,
				sbcI.sb_tSrf, sbcI.sb_hxa, sbcI.sb_hxr, sbcI.sb_qrAbs,
				sbcI.sb_qSrf);

}		// SBC::sb_DbPrintf
//=============================================================================
LOCAL FLOAT FC seriesU( FLOAT u1, FLOAT u2)

// series value of two conductances
{
	if (ISUNSET(u1) || ISUNSET(u2) )	// protect re NAN's. exman.h macro.
	{
		perl( (char *)MH_S0528);		// "cncult:seriesU(): unexpected unset h or u"
		return 0.f;
	}
	if (u1==0.f || u2==0.f)		// FEQUALs may be needed
		return 0.f;				// avoid divide by 0
	return 1.f/(1.f/u1 + 1.f/u2);
}	// seriesU
//========================================================================
LOCAL RC FC cnuCompAdd(				// Add an XSRAT entry to zone's XSURF chain
	const XSURF* x,		// source surface
	const char* name,	// surface name
	TI zi,				// zone to which
	TI* pxi,			// returned: index
	XSRAT** pxr)		// returned: newly added XSRAT

// An XSURF represents an os, window, or perim and expresses conductive and radiative coupling to the ambient.

// returns entry pointer so caller can modify, for use where input must be preserved for later runs, eg from makMs() 3-91.
//   CAUTION: ptr volatile, not valid after another cnuCompAdd.
{
	RC rc=RCOK;

// check and access zone
	if (zi <= 0 || zi > ZrB.n)
		return err( PWRN, (char *)MH_S0529, (INT)zi);  	// "cncult3.cpp:cnuCompAdd: bad zone index %d". intrnl err msg with keypress.
	// CAUTION: err does not errCount++; be sure error return propogated back so cul.cuf can errCount++.

	ZNR* zp = ZrB.p + zi;

// allocate and fill XSRAT record
	XSRAT* xr;
	CSE_E( XsB.add( &xr, WRN) ) 		// add record to XsB (a run record anchor) / return bad if error
	strcpy( xr->name, name);		// name for runtime probing, 1-92
	xr->ownTi = zi;					// store zone
	xr->x.Copy( x);
	// apply building and zone rotation
	//   use double, make Top.tp_bldgAzm=360 have no effect
	double azmRot = double( xr->x.azm) + zp->i.znAzm + Top.tp_bldgAzm;
	xr->x.azm = float( fmod( azmRot + 3*k2Pi, k2Pi));
	slsdc( xr->x.azm, xr->x.tilt, xr->x.xs_dircos);  // dircos from tilt/azm. slpak.cpp

// chain
	xr->nxXsurf = zp->xsurf1;		// 0 the 1st time
	zp->xsurf1 = xr->ss;

#if defined( _DEBUG)
	xr->Validate();
#endif

// return stuff
	if (pxr)
		*pxr = xr;			// return pointer so caller can further modify record
	if (pxi)				// return XSURF subscript, typically into input record
		*pxi = xr->ss;		// ... so probers can get to runtime xsurf, 1-92
	return rc;
}				// cnuCompAdd
//========================================================================
LOCAL void FC NEAR cnuSgDist(

// Add a solar gain distribution to an XSURF (in an SFI entry): where gain from intercepted radiation goes.

	XSURF* x, 		// XSURF for which to fill next sgdist[] entry
	SI targTy, 		// target type (cncult.h)...       description...        targTi is subscript of record in
    				//  SGDTTMASSI/O	   massive-model surface inside/outside surface          MSRAT MsR
    				//  SGDTTQUIKI/O       quick-model surface in/out (but gain goes to zn air)  XSRAT XsB
                    //  SGDTTZNAIR/ZNTOT   zone air/total: not used here (generated in cgsolar.cpp)  ---
	TI targTi,
	NANDAT fso, NANDAT fsc )	// fractions of window's insolation, shades open and closed: FLOATs or expr nandles.

{
	SGDIST* sg = x->sgdist + x->nsgdist++;	// pt next sgdist / bump count
	sg->sd_targTy = targTy;	// store type
	sg->sd_targTi = targTi;	// subscript of target record within base/anchor's records
	VD sg->sd_FSO = fso;		// fracs of distribution (FLOAT or expr nandle: latter errors if moved as float)
	VD sg->sd_FSC = fsc;		// ... shades open and closed.  V: clglob.h: *(void**)&
}			// cnuSgDist

// end of cncult3.cpp
