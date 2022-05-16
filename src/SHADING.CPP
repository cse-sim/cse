///////////////////////////////////////////////////////////////////////////////
// shading.cpp -- routines re determination of shaded/sunlit fractions of
//                surfaces for given sun position
///////////////////////////////////////////////////////////////////////////////

// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.


/*------------------------------- INCLUDES --------------------------------*/
#include "CNGLOB.H"

#include "ANCREC.H"	// record: base class for rccn.h classes
#include "rccn.h"	// WSHADRAT, SHADEX,
#include "CUPARSE.H"
#include "solar.h"
#include "CNGUTS.H"
#include "EXMAN.H"

#include "geometry.h"

#include <penumbra/penumbra.h>



float WSHADRAT::SunlitFract( 		// Calculate sunlit fraction of rectangle shaded
									//   with fins and/or overhang
   float gamma,				// azimuth of sun less azimuth of window:
							//   angle 0 if sun normal to window,
							//   positive cw looking down (radians)
   float cosz) const		// cosine of solar zenith angle

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
	// Members of  WSHADRAT and associated working variables
	// member   variable    description
	//  .wWidth     W      window width
	//  .wHeight    H      window height
	//  .ohDepth           depth of overhang
	//  .ohDistUp   U      distance from top of window to overhang
	//  .ohExL      E*     distance overhang extends beyond left edge of window
	//  .ohExRb            distance overhang extends beyond right edge of window
	//  .ohFlap            depth of vertical projection at the end of overhang
	//  .lfDepth    FD*    depth of left fin
	//  .lfTopUp    FU*    distance left fin extends above top of window
	//  .lfDistL    FO*    distance from left edge of window to left fin
	//  .lfBotUp    FBU*   distance left fin stops short above bottom of window
	//  .rfDepth           depth of right fin
	//  .rfTopUp           distance right fin extends above top of window
	//  .rfDistR           distance from right edge of window to right fin
	//  .rfBotUp           distance right fin stops short above bottom of window
	//            * set to left or right fin/overhang end per sun position.
	// comment comments
	//     "near" = toward sun (horizontally), "far" or beyond = away from sun.
	//     "ray" = extended slanted shadow line from top of fin or near (sunward)
	//             end of overhang.
	//     _1 variables are for hor overhang,
	//     _2  are for vertical flap at end of overhang,
	//     _3  are for fin.

// triangle area for specific altitude and global hor and ver components.
// Divide by 0 protection: most alt's are 0 if ver is 0.
#define TRIA(alt) ( (alt)==0.f ? 0.f : (alt)*(alt)*0.5f*hor/ver )

	// init
	if (cosz >= 1.f)			/* sun at zenith: no sun enters win.*/
		return 0.f;			/* (rob 1-90 prevents div by 0) */
	float cosg = (float)cos(gamma);
	if (cosg <= 0.f)		// if sun behind wall
		return 0.f;			//   window fully in shadow
	float sing = (float)sin(gamma);		/* ... solar azimuth rel to window */

	int K = 0;			/* 1 later during fin-flap overlap */
	float ver = float(cosz / sqrt(1. - cosz * cosz) / cosg);	/* tan(zenith angle) / cos(gamma) */
	float hor = (float)fabs(sing) / cosg;	/* abs(tan(gamma)): unit fin shade */
	float AREAO = 0.f;  		/* hor overhang shadow area */
	float AREAV = 0.f;  		/* ver overhang flap shadow area */
	float ARintF = 0.f;  		/* fin shadow area */
	float AREA1 = 0.f;  		/* working copies of above */
	float H = wHeight;	// hite, wid of window (later of flap shadow)
	float W = wWidth;
	float WAREA = H * W;	// whole window area */
	float W2 = 0.f;		/* init width & hite of oh flap shadow */
	float H2 = 0.f;
/* horiz overhang setup */
	float E = gamma < 0.f ? ohExR : ohExL;	// pertinent (near sun) oh extension
	float U = ohDistUp; 			// distance from top win to overhang
	float HU = H + U;				/* from overhang to bottom window */
	float WE = W + E;  			/* from near end oh to far side wind*/
	float Y1 = ohDepth * ver;		/* how far down oh shadow comes */
	float X1 = ohDepth * hor;	    /* hor component oh shad slantedge
						   = hor displ corners oh shadow */
						   /* NOTE: X1, Y1 are components of actual length (to corner) of shadow
									   slanted edge, based on oh depth.
									NY, FY relate to slanted ray intercepts with window edges,
									   irrespective of whether beyond actual corner. */
	int L = Y1 > U && Y1 < HU&& X1 < WE;	/* true if near shad bot corn
							(hence hor edge) is in win */
	if (ohDepth <= 0.f)		/* if no overhang */
		goto fin37;			/* go do fins.  flap is ignored. */

 /*----- horizontal overhang shadow area "AREAO" -----*/
	float NY, FY;	// how far down slant ray is at near, far win edge intercepts
	if (Y1 > U			/* if shadow comes below top window */
	 && (hor == 0.f ||		/* /0 protection */
		 (FY = WE * ver / hor) > U))	/* and edge passes below top far win corn */
			/* (set FY to how far down slant ray hits far side) */
	{
		if (Y1 > HU)		/* chop shadow at bot win to simplify */
			Y1 = HU;		/* (adjusting X1 to match doesn't matter
						 in any below-win cases) */
						 /* area: start with window rectangle above shadow bottom */
		AREAO = W * (Y1 - U);	/* as tho shadow corner sunward of window */

	/* subtract unshaded portions of rectangle if shad corner is in win */
		if (hor != 0.f)		/* else done and don't divide by 0 */
		{
			NY = E * ver / hor;   	/* "near Y": distance down from oh to where
					   slant ray hits near (to sun) win edge */
					   /* if corner not sunward of window, subtract unshaded triangle area */
			if (NY < Y1)  		/* if slant hits nr side abov sh bot*/
			{
				AREAO -= TRIA(Y1 - NY); 	/* triangle sunward of edge */
		  /* add back possible corners thereof hanging out above & beyond win*/
				if (NY < U)
					/* slantedge passes thru top, not near side, of window */
					AREAO += TRIA(U - NY);	/* add back trian ABOVE win */
				FY = WE * ver / hor;  	/* "far Y": distance down to where
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
	W2 = WE - max(E, X1);
	/* flapShad hite: from: lower of hor oh shad bot, window top.
				  to: hier of hor oh shad bot + flap ht, bottom window. */
	H2 = min(Y1 + ohFlap, HU) - max(Y1, U);
	/* if hite or wid <= 0, no shadow, AREAV stays 0, H2 and W2 don't matter. */
	if (H2 > 0.f && W2 > 0.f)
	{
		AREAV = H2 * W2;			/* area shaded by flap */
		if (AREAO + AREAV >= WAREA)	/* if now fully shaded */
			goto exit68;			/* go exit */
	}

	/*----- FINS: select sunward fin and set up -----*/
fin37:			/* come here if no overhang */
	if (gamma == 0.f)  		/* if sun straight on, no fin shadows, exit */
		goto exit68;		/* (and prevent division by hor: is 0) */

	float FD;	// fin depth: how far out fin projects
	float FU;	// "fin up": fin top extension above top win
	float FO;	// "fin over": how far sunward from window to fin
	float FBU;	// "fin bottom up": how far above bot win fin ends
	if (gamma > 0.f)	    /* if sun coming from left */
	{
		FD = lfDepth;		/* fin depth: how far out fin projects */
		FU = lfTopUp; 		/* "fin up": fin top extension above top win*/
		FO = lfDistL; 		/* "fin over": how far sunward from window */
		FBU = lfBotUp; 		/* "fin bottom up": how far above bot win */
	}
	else  		    /* else right fin shades toward window */
	{
		FD = rfDepth; 	/* fin depth */
		FU = rfTopUp; 		/* top extension */
		FO = rfDistR; 		/* hor distance from window */
		FBU = rfBotUp; 		/* bottom distance above windowsill */
	}
	if (FD <= 0.f)		/* if fin depth 0 */
		goto exit68;		/* no fin, go exit */

	float X3 = FD * hor;	/* how far over (antisunward) shadow comes */

/*---- FIN... Adjustments to not double-count overlap
			 of fin's shadow with horizontal overhang's shadow ----*/
	if (AREAO > 0.f)			/* if have overhang */
	{
		float FUO = U + (FO - E) * ver / hor;	/* how far up fin would extend for fin
							and oh shad slantEdges to just meet
							for this sun posn.  Can be < 0. */
		if (FU > FUO)			/* if fin shadow overlaps oh shadow */
		{
			if (L == 0			/* if oh shad has no hor edge in win*/
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
				AREA1 = W * (Y1 - U) - AREAO;
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
	NY = FO * ver / hor;	/* "near Y":  y component slant ray distance from
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
		FY = (W + FO) * ver / hor;	/* "far Y":  y component slant ray
						distance from top fin to far side win */
						/* HFU = H + FU;	* top fin to bot window distance *  not used */
					/* subtract any lit rectangle and triang above shad.  nb FU can be < 0. */
		if (FY > FU)		/* if some of top of win lit */
		{
			float FYbelowWIN;
			if (NY <= FU)			/* if less than full width lit */
				AREA1 -= TRIA(FY - FU);	    /* subtract top far triangle */
			else				/* full wid (both top corners) lit */
				AREA1 -= W * (NY - FU)	    /* subtract lit top rectangle */
				+ TRIA(FY - NY);	    /* and lit far triangle below it*/
			FYbelowWIN = FY - (H + FU);
			if (FYbelowWIN > 0.f)		/* if far lit triang goes below win */
				AREA1 += TRIA(FYbelowWIN);	    /* add back area belo win: wanna
									  subtract a trapezoid only */
		}
		/* subtract any lit rectangle and triangle below fin shadow */
		if (NY < FBU)		/* if some of bottom of win lit */
			if (FY >= FBU)		/* if less than full width lit */
				AREA1 -= TRIA(FBU - NY);	    /* subtract bot near triangle */
			else				/* full wid (both bot corners) lit */
				AREA1 -= W * (FBU - FY)	    /* subtact lit bottom rectangle */
				+ TRIA(FY - NY);	    /* and lit near triang above it */
/* add in entire area as tho shaded (LAST for best precision: largest #) */
		AREA1 += H * W;
	}

	/*---- FIN... done this shadow; sum and proceed from window to flap ----*/

	if (K)			/* if just computed shadow on flap shadow */
		AREA1 = -AREA1;		/* negate fin shadow on flap shadow */
	ARintF += AREA1;		/* combine shadow areas */
	if (K == 0 && AREAV > 0.f)	/* if just did window and have flap shadow */
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
		HbelowFlapShad = HU - Y1 - ohFlap;
		if (HbelowFlapShad > 0.f)	/* if bot flap shadow above bot win */
		{
			FBU -= HbelowFlapShad;	/* adjust fin-bottom-up to keep fin
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
	float fsunlit = 1.f - (AREAO + AREAV + ARintF) / WAREA;
	// return 0 if result so small that is probably just roundoff errors;
	//  in particular, return 0 for small negative results.
	return fsunlit > 1.e-7f ? fsunlit : 0.f;

}	// WSHADRAT::SunlitFract

///////////////////////////////////////////////////////////////////////////////
// class SURFGEOMDET -- geometric details for SURFGEOM
//                      local / hides geometry.h classes
///////////////////////////////////////////////////////////////////////////////
#define PUMBRA( m) (Top.tp_pPumbra->m)		// access to Penumbra GPU shading
//-----------------------------------------------------------------------------
class SURFGEOMDET
{
friend SURFGEOM;
	SURFGEOMDET( SURFGEOM* pSG) : gxd_pSG( pSG)
	{}
	SURFGEOMDET( SURFGEOM* pSG, const SURFGEOMDET& sgd);
	bool gxd_IsEmpty() const	// true iff no valid polygon
	{	return gxd_uNorm.IsZero(); }

	SURFGEOM* gxd_pSG;			// parent
	CPolygon3D gxd_polygon;		// polygon (as input, no rotation etc.)
	CPV3D gxd_uNorm;			// unit normal of gxd_polygon (not transforms)
	CPV3D gxd_uNormT;			// unit normal with any transforms (e.g. bldgAzm rotation)

	SURFGEOM& SG()
	{	return *gxd_pSG; }
	record& PREC()
	{	return *SG().gx_pParent; }
	RC gxd_FillAndCheck( int fnVI, float vrt[], int nVrt);
	int gxd_GetAzmTilt( float& azm, float& tilt) const
	{	double dAzm,dTilt;
		int ret = gxd_uNorm.AzmTilt( dAzm, dTilt);
		azm = float( dAzm);
		tilt = float( dTilt);
		return ret;
	}
	int gxd_SetupShading( const CT3D* MT=NULL);
};		// class SURFGEOMDET
//-----------------------------------------------------------------------------
SURFGEOMDET::SURFGEOMDET(		// special copy c'tor
	SURFGEOM* pSG,			// parent SURFGEOM
	const SURFGEOMDET& sgd)	// source data
	: gxd_pSG( pSG), gxd_polygon( sgd.gxd_polygon), gxd_uNorm( sgd.gxd_uNorm),
	  gxd_uNormT( sgd.gxd_uNormT)
{
}	// SURFGEOMDET::SURFGEOMDET
//-----------------------------------------------------------------------------
RC SURFGEOMDET::gxd_FillAndCheck(
	int fnVI,			// source record vertices field number
	float vrt[],		// vertices (x, y, z)
	int nVrt)			// # of vertices (= dim vrt[]/3)
{
	RC rc = RCOK;
	gxd_polygon.DeleteAll();
	gxd_polygon.Add( vrt, nVrt);
	record& PR = PREC();		// parent record (SHADEX, PVARRAY, )

	SG().gx_area = gxd_polygon.UnitNormal( gxd_uNorm);
	if (SG().gx_area < 0.)
		rc = PR.ooer( fnVI, "Bad polygon, cannot determine normal");
	else
	{	double dist = gxd_polygon.CheckFix( 1);
		if (dist > .01)
			rc = PR.ooer( fnVI, "Bad vertices");
	}
	return rc;
}		// SURFGEOMDET::gxd_FillAndCheck
//-----------------------------------------------------------------------------
int SURFGEOMDET::gxd_SetupShading(	// convert polygon to Penumbra format
	const CT3D* MT /*=NULL*/) 	// point transform matrix if any
								//   e.g. re bldgAzm rotation
// minimal error checking -- *this assumed good
// returns Penumbra surface idx
//         -1 if not successful
{	
	// unit normal
	gxd_uNormT = MT ? MT->TX( gxd_uNorm) : gxd_uNorm;

	// polygon
	int pnIdx = -1;
	int nV=gxd_polygon.GetSize();
	if (nV >= 3)
	{	Pumbra::Polygon plg;
		for (int iV=0; iV<nV; iV++)
		{	CPV3D p3 = gxd_polygon.p3_Vrt( iV);
			if (MT)
				p3 = MT->TX( p3);	// apply transform if caller provides
			// Penumbra coords are float
			//   winding order same as CSE
			for (int iD=0; iD<3; iD++)	// loop x, y, z
				plg.push_back( float( p3[ iD]));
		}
		Pumbra::Surface pnSurf( plg);
		pnIdx = PUMBRA( addSurface( pnSurf));
	}
	return pnIdx;
}		// SURFGEOMDET::gxd_SetupShading
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// struct SURFGEOM: nestable surface geometry
//                  used with *nest in e.g. SHADEX, PVARRAY
///////////////////////////////////////////////////////////////////////////////
SURFGEOM::SURFGEOM()
{
	gx_sgDet = new SURFGEOMDET( this);
	gx_pnIdx = -1;
}	// SURFGEOM::SURFGEOM
//-----------------------------------------------------------------------------
SURFGEOM::~SURFGEOM()
{
	delete gx_sgDet;
	gx_sgDet = NULL;
}	// SURFGEOM::~SURFGEOM
//-----------------------------------------------------------------------------
void SURFGEOM::gx_SetParent(			// parent linkage etc
	record* pParent,		// parent record: SHADEX, PVARRAY, etc
	[[maybe_unused]] int options /*=0*/)		// options TBD
{
	gx_pParent = pParent;
}		// SURFGEOM::gx_SetParent
//-----------------------------------------------------------------------------
RC SURFGEOM::gx_Init()		// start-of-run init
{	RC rc = RCOK;
	gx_fBeamErrCount = 0;	// fBeam > 1ish error count
	return rc;
}		// SURFGEOM::gx_Init
//-----------------------------------------------------------------------------
bool SURFGEOM::gx_IsEmpty() const	// true iff no polygon
{
	return !gx_sgDet || gx_sgDet->gxd_IsEmpty();
}		// SURFGEOM::gx_IsEmpty
//-----------------------------------------------------------------------------
RC SURFGEOM::gx_Validate(
	record* pParent,	// actual parent
	const char* what,	// parent type ("PVARRAY")
	int options /*=0*/)	// option bits
						//   1: check SURFGEOMDET linkage
{
	RC rc = RCOK;
	if (gx_pParent != pParent)
		rc |= errCrit( WRN, "%s::Validate: Bad SURFGEOM linkage", what);
	else if (options&1 && gx_sgDet->gxd_pSG != this)
		rc |= errCrit( WRN, "%s::Validate: Bad SURFGEOMDET linkage", what);
	return rc;
}		// SURFGEOM::gx_Validate
//-----------------------------------------------------------------------------
void SURFGEOM::gx_CopySubObjects()		// copy heap subobjects
// call AFTER bitwise parent Copy()
//   subojects are duplicated
{	if (gx_sgDet)
		gx_sgDet = new SURFGEOMDET( this, *gx_sgDet);
}	// SURFGEOM::gx_CopySubObjects
//-----------------------------------------------------------------------------
RC SURFGEOM::gx_CheckAndMakePolygon(
	[[maybe_unused]] int phase,
	int fnG)		// SURFGEOM field number within parent
// returns RCOK iff success

{
	// gx_polygon->DeleteAll();  clean out on first call?

	int fnVI = fnG + SURFGEOM_VRTINP;
	int nSet, nVal;
	RC rc = ArrayStatus( &gx_pParent->fStat( fnVI), DIM_POLYGONXYZ, nSet, nVal);

	if (rc)
		gx_pParent->ooer( fnVI, "Internal error: invalid ARRAY");
	else if (nSet == 0)
		return RCOK;

	else if (nSet < 9 || nSet%3 != 0)
		rc = gx_pParent->ooer( fnVI, "Invalid number of XYZ coordinates (%d)."
			   "  Must be a multiple of 3 and >= 9.", nSet);

	if (!rc)
	{	
		gx_sgDet->gxd_FillAndCheck( fnVI, gx_vrtInp, nSet/3);

		// validate polyon
		// compare to tilt / azm?
	}
	
	return rc;

}	// SURFGEOM::gx_CheckAndMakePolygon
//----------------------------------------------------------------------------
int SURFGEOM::gx_GetAzmTilt( float& azmR, float& tiltR) const
// azm / tilt (radians)
{	return gx_sgDet->gxd_GetAzmTilt( azmR, tiltR);
}	// SURFGEOM::gx_GetAzmTilt
//-----------------------------------------------------------------------------
RC SURFGEOM::gx_SetupShading(		// shading calcs setup
	int& nS,				// shading surface count
							//    incremented iff success
	const CT3D* MT,			// point transformation matrix
							//   (e.g. re bldgAzm rotation)
							//   NULL -> no transformation
	int options /*=0*/)		// options TBD
// add this surface to the Penumbra model
// returns RCOK: success or no geometry specified
//    else RCBAD: bad geometry or addSurface fail
{
	options;
	gx_pnIdx = -1;		// no associated Penumbra surface
	RC rc = RCOK;
	if (gx_sgDet && !gx_IsEmpty())
	{	if (gx_mounting == C_MOUNTCH_SITE)
			MT = NULL;		// no rotation for site-mounted objects
		gx_pnIdx = gx_sgDet->gxd_SetupShading( MT);
		if (gx_pnIdx >= 0)
			nS++;
		else
			rc = RCBAD;
	}
	return rc;
}		// SURFGEOM::gx_SetupShading
//-----------------------------------------------------------------------------
void SURFGEOM::gx_ClearShading()		// clear shading results (e.g. when sun is down)
{	gx_fBeam = 0.f;
}	// SURFGEOM::gx_ClearShading
//-----------------------------------------------------------------------------
int SURFGEOM::gx_CalcBeamShading(		// beam shading for current hour
	float& cosi,	// returned: cos( beam incA)
	float& fBeam)	// returned: fraction of surface sunlit

// slday() must have been called

// returns 1: sun up relative to surface (cosi, fBeam set)
//            fBeam = 1 if Penumbra shading not possible or disabled
//         0: sun down relative to surface
//            (cosi=fBeam=0)
//        -1: calc failure (cosi=fBeam=0)
{
	float dcSrf[ 3];		// surface direction cosines
	gx_sgDet->gxd_uNormT.Get( dcSrf);
	float azm, cosz;
	int sunupSrf = slsurfhr( dcSrf, Top.iHrST, &cosi, &azm, &cosz);

	if (!sunupSrf)
	{	fBeam = 0.f;		// sun is down or behind surface
		// cosi = 0.f;		// set in slsurfhr

	}
	else if (Top.tp_exShadeModel != C_EXSHMODELCH_PENUMBRA
		  || gx_pnIdx < 0)
		fBeam = 1.f;		// Penumbra model disabled
							// or no associated Penumbra surface
							// say no shade
	else
	{	// use Penumbra model
		float alt = kPiOver2 - acos( cosz);
		float dcSun[ 3];
		slsdc( azm, kPiOver2-alt, dcSun);
		cosi = DotProd3( dcSrf, dcSun);		// replace cosi with value
											//   consistent with alt and azm
		if (PUMBRA(setSunPosition)( azm, alt) != Pumbra::PN_SUCCESS)
		{	cosi = fBeam = 0.f;
			sunupSrf = -1;	// unexpected
		}
		else
		{	float PSSF = PUMBRA( calculatePSSF)( gx_pnIdx);
			fBeam = PSSF / (cosi*gx_area);
			// PUMBRA(renderScene)(gx_pnIdx);

			if (fBeam > 1.f)
			{	if (fBeam > 1.05f && cosi > .01f)
				{	// report some errors > 5% and all errors > 10%
					//  ignore if low sun angles
					static const int MAXFBEAMMSGS = 20;
					if ( ++gx_fBeamErrCount <= MAXFBEAMMSGS || fBeam > 5.f)
					{	const char* noMore = gx_fBeamErrCount == MAXFBEAMMSGS
									? "\n  Omitting further messages for small errors of this type."
									: "";
						rWarn( "%s: calculated sunlit fraction (=%0.4f) s/b <= 1.%s",
							gx_pParent->classObjTx( 1), fBeam, noMore);
						// PUMBRA(renderScene)(gx_pnIdx);
					}
				}
				fBeam = 1.;		// limit to 1
			}
			// round to nearest thousandth to minimize pixel precision 
			// errors for unshaded surfaces and differences among GPUs
			roundNearest(fBeam, 0.001f);
		}
	}
	gx_fBeam = fBeam;
	return sunupSrf;
}		// SURFGEOM::gx_CalcBeamShading
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class SHADEX: geometric shade object
///////////////////////////////////////////////////////////////////////////////
SHADEX::SHADEX( basAnc *b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
{
	FixUp();
}	// SHADEX::SHADEX
//----------------------------------------------------------------------------
/*virtual*/ void SHADEX::FixUp()
{
	sx_g.gx_SetParent( this);
}		// SHADEX::FixUp
//-----------------------------------------------------------------------------
/*virtual*/ RC SHADEX::Validate(
	int options/*=0*/)		// options bits
							//  1: check child SURFGEOMDET also
{
	RC rc = record::Validate( options);
	if (rc == RCOK)
		rc = sx_g.gx_Validate( this, "SHADEX", options);
	return rc;
}		// SHADEX::Validate
//----------------------------------------------------------------------------
RC SHADEX::sx_Init()		// init at run start
{
	RC rc = sx_g.gx_Init();

	return rc;
}		// SHADEX::sx_Init
//----------------------------------------------------------------------------
void SHADEX::Copy( const record* pSrc, int options/*=0*/)
{	// bitwise copy of record
	record::Copy( pSrc, options);	// calls FixUp()
	// copy SURFGEOM heap subobjects
	sx_g.gx_CopySubObjects();
}	// SHADEX::Copy
//----------------------------------------------------------------------------
RC SHADEX::sx_CkF()
{
	RC rc = RCOK;

	rc = sx_g.gx_CheckAndMakePolygon( 0, SHADEX_G);

	return rc;
}		// SHADEX::sx_CkF
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// TOPRAT interface to shading model
///////////////////////////////////////////////////////////////////////////////
static void PenumbraMessageHandler(
	const int messageType,
	const std::string message,
	[[maybe_unused]] void* contextPtr)
{
	// TOPRAT* pTop = (TOPRAT *)contextPtr;

	int erOp;
	const char* what;
	if (messageType == Pumbra::MSG_ERR)
	{	erOp = ABT;		// message / keypress / abort
		what = "error";
	}
	else if (messageType == Pumbra::MSG_WARN)
	{	erOp = ERR;		// message / keypress / continue
		what = "warning";
	}
	else
	{	erOp = INF;		// message / continue
		what = "info";
	}
	err(
		erOp | NOPREF,			// do not prepend "Error:"
		"Shading %s: %s", what, message.c_str());

}		// penumbraMessageHandler
//-----------------------------------------------------------------------------
RC TOPRAT::tp_PumbraInit()
{	RC rc = RCOK;
	if (!tp_pPumbra)
	{	const int size = 4096;	// context size (=# of x and y pixels of gpu buffer)
								//   500->4096 1-2019 after inaccurate cases found
		tp_pPumbra = new Pumbra::Penumbra( PenumbraMessageHandler, this, size);
		if (!tp_pPumbra)
			rc = RCBAD;		// not expected
	}
	else
		tp_PumbraClearIf();

	return rc;
}	// TOPRAT::tp_PumbraInit
//-----------------------------------------------------------------------------
void TOPRAT::tp_PumbraDestroy()
{	delete tp_pPumbra;
	tp_pPumbra = NULL;
}		// TOPRAT::tp_PumbraDestroy
//-----------------------------------------------------------------------------
int TOPRAT::tp_PumbraClearIf()		// clear Penumbra data if any
// returns 0 if Penumbra not instantiated, nothing done
//         1 if data successfully cleared
{	if (!tp_pPumbra)
		return 0;		// nothing to do

	tp_pPumbra->clearModel();
	return 1;
}		// TOPRAT::tp_PumbraClearIf
//-----------------------------------------------------------------------------
int TOPRAT::tp_PumbraAvailability() const
{
	return tp_pPumbra != NULL;
}		// TOPRAT::tp_PumbraAvailability
//-----------------------------------------------------------------------------
RC TOPRAT::tp_PumbraSetModel()
{
	RC rc = tp_pPumbra->setModel() == Pumbra::PN_SUCCESS
				? RCOK
				: RCBAD;
	// message?
	return rc;
}		// TOPRAT::tp_PumbraSetModel
//-----------------------------------------------------------------------------
int TOPRAT::tp_ExshCount()
// returns nz if current project has ANY shading surface
{
	int pvCount = 0;
	PVARRAY* pPV;
	RLUP( PvR, pPV)
	{	if (pPV->pv_HasPenumbraShading())
			pvCount++;
	}

	int sxCount = SxR.GetCount();

	tp_exshNShade = pvCount + sxCount;	// # of shade surfaces
	tp_exshNRec = pvCount;				// # of receiving surfaces

	return pvCount > 1 || (pvCount > 0 && sxCount > 0);
}	// TOPRAT::tp_ExshCount
//-----------------------------------------------------------------------------
RC TOPRAT::tp_ExshRunInit()		// start-of-run init for Penumbra shading
// sets up Penumbra model from all shading or receiving objects
{
	RC rc = RCOK;

	int exshCalcNeeded = tp_ExshCount();	// set tp_exshNShade and tp_exshNRec
											//   return nz iff shading calc needed

	if (exshCalcNeeded && Top.tp_exShadeModel == C_EXSHMODELCH_NONE)
	{	info( "Advanced shading input in PVARRAY and/or SHADEX has been ignored\n"
			  "      due to Top exShadeModel='none'");
		exshCalcNeeded = 0;
	}

	if (!exshCalcNeeded)
	{	tp_PumbraClearIf();		// clear Penumbra data if any
		return RCOK;
	}

	// shading is available and required
	tp_PumbraInit();

	// building rotation
	//   construct rotation matrix re C_SHADETY_BLDG
	CT3D MRot( 1);		// rotation matrix
	MRot.Rotate( tp_bldgAzm, -2);	// rotate about z axis
									//  -2 = right-handed coord system

	// create Penumbra surface model
	PVARRAY* pPV;
	int NRec = 0;		// # of receiving surfaces (unused)
						//  = PVARRAYs with vertices
	RLUP( PvR, pPV)
		rc |= pPV->pv_SetupShading( NRec, &MRot);

	SHADEX* pSX;
	int NShade = 0;		// # of shade surfaces (unused)
	RLUP( SxR, pSX)
		rc |= pSX->sx_SetupShading( NShade, &MRot);

	if (!rc)
		rc = tp_PumbraSetModel();

	return rc;
}		// TOPRAT::tp_ExshRunInit
//-----------------------------------------------------------------------------
RC TOPRAT::tp_ExshBegHour()		// once-per-hour exterior shading setup
{
	RC rc = RCOK;

	// all setup done per-surface

	return rc;
}		// TOPRAT::tp_ExshBegHour
//=============================================================================

// shading.cpp end
