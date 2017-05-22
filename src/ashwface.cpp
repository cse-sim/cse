// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// ashwface.cpp -- interface to ASHWAT
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"
#include <ancrec.h>	// record: base class for rccn.h classes
#include <rccn.h>
#include "cnguts.h"
#include "timer.h"

#include "ashwface.h"


// ASHWAT constants
// WHY: use fixed size arrays to avoid FORTRAN array descriptors
const int MSGMAXLEN = 100;
const int MSGMAXCOUNT = 20;

static CFSSWP swpBlack;			// black room (c'tor leaves all values 0)

///////////////////////////////////////////////////////////////////////////////
// CFS member functions
///////////////////////////////////////////////////////////////////////////////
const char* CFSLWP::DbFmt( const char* tag/*=""*/) const		// formatted string for e.g. DbDump()
// returns self-description (in TmpStr, no \n re embedding)
{
	return strtprintf( "%sLW EF=%0.3f EB=%0.3f T=%0.3f",
		tag, EPSLF, EPSLB, TAUL);
}		// CFSLWP::DbFmt
//=============================================================================
CFSSWP::CFSSWP()
{ }
//-----------------------------------------------------------------------------
const char* CFSSWP::DbFmt( const char* tag/*=""*/) const	// formatted string for e.g. DbDump()
// returns self-description (in TmpStr, no \n re embedding)
{
	return strtprintf(
		"%sSW TBB=%.3f/%.3f RBB=%.3f/%.3f  TBD=%.3f/%.3f RBD=%.3f/%.3f  TDD=%.3f RDD=%.3f/%.3f",
			tag,
			TAUSFBB, TAUSBBB, RHOSFBB, RHOSBBB,
			TAUSFBD, TAUSBBD, RHOSFBD, RHOSBBD,
			TAUS_DD, RHOSFDD, RHOSBDD);

}	// CFSSWP::DbFmt
//=============================================================================
const char* CFSGAP::DbFmt( const char* tag/*=""*/) const
// returns self-description (in TmpStr, no \n re embedding)
{
	return strtprintf( "%s%s ty=%d W=%0.3f Weff=%0.3f",
		tag,
		FCGET( FG.ID), GTYPE,
		TAS/25.4, TAS_EFF/25.4);		// gap width: mm to inches
}		// CFSGAP::DbFmt
//=============================================================================
CFSLAYER::CFSLAYER()
{	ASHWAT.xw_ClearCFSLayer( *this);		// clear all members
}	// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
CFSLAYER::CFSLAYER( const CFSLAYER& cl)
{	memcpy( this, &cl, sizeof( CFSLAYER));	// bitwise
}	// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
CFSLAYER::CFSLAYER(			// layer c'tor
	const char* id,			// ID (name) of layer
	int ltype,				// type (ltyGLAZE, )
							// front (outside) properties
	float tauFBB,			//    beam transmittance (or openness)
	float rhoFBB,			//    beam reflectance
	float tauBD,			//    beam-diffuse transmittance
	float epsLF /*=-1.f*/,	//    LW emittance
							// back (inside) normal properties
							//  default = same as front
	float rhoBBB /*=-1.f*/,	//    beam reflectance
	float epsLB /*=-1.f*/,	//    LW emittance
	float tauL /*=0.f*/)	//    LW transmittance
{
	if (ltype == ltyGLAZE)
		cl_InitGlaze( id, tauFBB, rhoFBB, epsLF, rhoBBB, epsLB, tauL);
	else if (ltype == ltyROLLB)
		cl_InitRollB( id, tauFBB, rhoFBB, tauBD);
	else if (ltype == ltyINSCRN)
		cl_InitInscrn( id, tauFBB, rhoFBB, tauBD);
	else if (ltype == ltyDRAPE)
		cl_InitDrape( id, tauFBB, rhoFBB, tauBD);
	else
		ASHWAT.xw_ClearCFSLayer( *this);
}		// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
CFSLAYER::CFSLAYER(			// special glazing layer c'tor (Windows 6 arg order)
	const char* id,							// ID (name) of layer
	float thk,								// thickness, in (unused)
	float tSol, float rSol1, float rSol2,	// solar properties
	float tVis, float rVis1, float rVis2,	// visible properties (unused)
	float tIR,	float eIR1,  float eIR2,	// LW properties
	float kEff)								// conductivity,
{
	thk, tVis, rVis1, rVis2, kEff;
	cl_InitGlaze( id, tSol, rSol1, eIR1, rSol2, eIR2, tIR);
}		// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
#if 0
x void CFSLAYER::cl_Clear()
x {
x 	memset( this, 0, sizeof( CFSLAYER));	// TODO
x }		// CFSLAYER::cl_Clear
#endif
//-----------------------------------------------------------------------------
RC CFSLAYER::cl_InitGlaze(			// init glazing layer
	const char* id,			// ID (name) of layer
							// front (outside) normal properties
	float tauFBB,			//    beam transmittance
	float rhoFBB,			//    beam reflectance
	float epsLF,			//    LW emittance
							// back (inside) normal properties
							//  default = same as front
	float rhoBBB /*=-1.f*/,	//    beam reflectance
	float epsLB /*=-1.f*/,	//    LW emittance
	float tauL /*=0.f*/)	//    LW transmittance

{
	ASHWAT.xw_ClearCFSLayer( *this);

	LTYPE = ltyGLAZE;
	FCSET( ID, id);

	// solar (short wave) properties
	if (rhoBBB < 0.f)
		rhoBBB = rhoFBB;

	SWP_MAT.RHOSFBB = rhoFBB;
	SWP_MAT.RHOSBBB = rhoBBB;
	SWP_MAT.TAUSFBB = tauFBB;
	SWP_MAT.TAUSBBB = tauFBB;		// front/back tau always same for glazing

	// SWP_MAT.RHOSFBD = 0.;		// BD properties = 0 (no frosted glass TODO?)
	// SWP_MAT.RHOSBBD = 0.;
	// SWP_MAT.TAUSFBD = 0.;
	// SWP_MAT.TAUSBBD = 0.;

	SWP_MAT.RHOSFDD = -1.;	// force estimation of diffuse-diffuse properties
	SWP_MAT.RHOSBDD = -1.;
	SWP_MAT.TAUS_DD = -1.;

	// long-wave (thermal) properties
	if (epsLB < 0.f)
		epsLB = epsLF;

	LWP_MAT.EPSLF = epsLF;
	LWP_MAT.EPSLB = epsLB;
	LWP_MAT.TAUL = tauL;

	// TODO: handle error message
	return ASHWAT.xw_CheckFixCFSLayer( *this, id);
}		// CFSLAYER::cl_InitGlaze
//-----------------------------------------------------------------------------
RC CFSLAYER::cl_InitRollB(			// init roller blind
	const char* id,			// ID (name) of layer
							// front (outside) normal properties
	float fOpen,			// openness (=tauFBB)
	float rhoFBD,			// front beam-diffuse reflectance
	float tauBD,			// beam-diffuse transmittance
	float rhoBBD /*=-1*/)	// back beam-diffuse reflectance

// note: total transmittance = fOpen + tauBD
{
#if 0
	ALL GxR( 6, CFSLayers( iL)%SWP_MAT%TAUSFBB, 'Openness', gxREQD, '|0|1')
	CALL GxR( 7, CFSLayers( iL)%SWP_MAT%RHOSFBD, 'RHOSFBD',  gxREQD, '|0|1')
	CALL GxR( 8, CFSLayers( iL)%SWP_MAT%RHOSBBD, 'RHOSBBD',  gxREQD, '|0|1')
	CALL GxR( 9, CFSLayers( iL)%SWP_MAT%TAUSFBD, 'TAUS_BD',  gxREQD, '|0|1')
#endif

	ASHWAT.xw_ClearCFSLayer( *this);

	LTYPE = ltyROLLB;
	FCSET( ID, id);

	// solar (short wave) properties
	if (rhoBBD < 0.f)
		rhoBBD = rhoFBD;

	SWP_MAT.TAUSFBB = fOpen;
	SWP_MAT.TAUSBBB = fOpen;
	SWP_MAT.RHOSFBD = rhoFBD;
	SWP_MAT.RHOSBBD = rhoBBD;
	SWP_MAT.TAUSFBD = tauBD;
	SWP_MAT.TAUSBBD = tauBD;

#if 0
	SWP_MAT.RHOSFDD = -1.;			// force estimation of diffuse-diffuse properties
	SWP_MAT.RHOSBDD = -1.;
	SWP_MAT.TAUS_DD = -1.;
#endif

	LWP_MAT.EPSLF = -1.;			// default all LW properties
	LWP_MAT.EPSLB = -1.;
	LWP_MAT.TAUL = -1.;

	// TODO: handle error message
	return ASHWAT.xw_CheckFixCFSLayer( *this, id);
}		// CFSLAYER::cl_InitRollB
//-----------------------------------------------------------------------------
RC CFSLAYER::cl_InitInscrn(			// init insect screen
	const char* id,			// ID (name) of layer
							// front (outside) normal properties
	float fOpen,			// openness (=tauFBB)
	float rhoFBD,			// front beam-diffuse reflectance
	float tauBD,			// beam-diffuse transmittance
	float rhoBBD /*=-1*/)	// back beam-diffuse reflectance

// note: total transmittance = fOpen + tauBD
{
#if 0
	CALL GxR( 6, CFSLayers( iL)%SWP_MAT%TAUSFBB, 'Openness', gxREQD, '|0|1')
	CALL GxR( 7, CFSLayers( iL)%SWP_MAT%RHOSFBD, 'RHOSFBD',  gxREQD, '|0|1')
	CALL GxR( 8, CFSLayers( iL)%SWP_MAT%RHOSBBD, 'RHOSBBD',  gxREQD, '|0|1')
	CALL GxR( 9, CFSLayers( iL)%SWP_MAT%TAUSFBD, 'TAUS_BD',  gxREQD, '|0|1')
	CFSLayers( iL)%S = -1.		! force derivation of wire geom from openness
	CFSLayers( iL)%W = -1.
	CFSLayers( iL)%SWP_MAT%TAUSBBB = CFSLayers( iL)%SWP_MAT%TAUSFBB
	CFSLayers( iL)%SWP_MAT%TAUSBBD = CFSLayers( iL)%SWP_MAT%TAUSFBD
#endif

	ASHWAT.xw_ClearCFSLayer( *this);

	LTYPE = ltyINSCRN;
	FCSET( ID, id);

	// solar (short wave) properties
	if (rhoBBD < 0.f)
		rhoBBD = rhoFBD;

	SWP_MAT.TAUSFBB = fOpen;
	SWP_MAT.TAUSBBB = fOpen;
	SWP_MAT.RHOSFBD = rhoFBD;
	SWP_MAT.RHOSBBD = rhoBBD;
	SWP_MAT.TAUSFBD = tauBD;
	SWP_MAT.TAUSBBD = tauBD;

#if 0
	SWP_MAT.RHOSFDD = -1.;			// force estimation of diffuse-diffuse properties
	SWP_MAT.RHOSBDD = -1.;
	SWP_MAT.TAUS_DD = -1.;
#endif

	LWP_MAT.EPSLF = -1.;			// default all LW properties
	LWP_MAT.EPSLB = -1.;
	LWP_MAT.TAUL = -1.;

	S = -1;		// force estimation of wire geometry from openness
	W = -1;

	// TODO: handle error message
	return ASHWAT.xw_CheckFixCFSLayer( *this, id);
}		// CFSLAYER::cl_InitInscrn
//-----------------------------------------------------------------------------
RC CFSLAYER::cl_InitDrape(			// init drape layer
	const char* id,			// ID (name) of layer
							// front (outside) normal properties
	float fOpen,			// openness (=tauFBB)
	float rhoFBD,			// front beam-diffuse reflectance
	float tauBD,			// beam-diffuse transmittance
	float rhoBBD /*=-1*/)	// back beam-diffuse reflectance
{

#if 0
	CALL GxR(  6, CFSLayers( iL)%SWP_MAT%TAUSFBB, 'Openness', gxREQD, '|0|1')
	CALL GxR(  7, CFSLayers( iL)%SWP_MAT%RHOSFBD, 'RHOSFBD',  gxREQD, '|0|1')
	CALL GxR(  8, CFSLayers( iL)%SWP_MAT%RHOSBBD, 'RHOSBBD',  gxREQD, '|0|1')
	CALL GxR(  9, CFSLayers( iL)%SWP_MAT%TAUSFBD, 'TAUS_BD',  gxREQD, '|0|1')
	CALL GxR( 10, CFSLayers( iL)%S,               'PleatS',  0,      '100|.01')
	CALL GxR( 11, CFSLayers( iL)%W,               'PleatW',  0,      '100|0')
	CFSLayers( iL)%SWP_MAT%RHOSFDD = -1.	! init diffuse SWP to force default derivation
	CFSLayers( iL)%SWP_MAT%RHOSBDD = -1.
	CFSLayers( iL)%SWP_MAT%TAUS_DD = -1.
	CFSLayers( iL)%SWP_MAT%TAUSBBB = CFSLayers( iL)%SWP_MAT%TAUSFBB
	CFSLayers( iL)%SWP_MAT%TAUSBBD = CFSLayers( iL)%SWP_MAT%TAUSFBD
#endif

	ASHWAT.xw_ClearCFSLayer( *this);

	LTYPE = ltyDRAPE;
	FCSET( ID, id);

	// solar (short wave) properties
	if (rhoBBD < 0.f)
		rhoBBD = rhoFBD;

	SWP_MAT.TAUSFBB = fOpen;
	SWP_MAT.TAUSBBB = fOpen;
	SWP_MAT.RHOSFBD = rhoFBD;
	SWP_MAT.RHOSBBD = rhoBBD;
	SWP_MAT.TAUSFBD = tauBD;
	SWP_MAT.TAUSBBD = tauBD;

	SWP_MAT.RHOSFDD = -1.;			// force estimation of diffuse-diffuse properties
	SWP_MAT.RHOSBDD = -1.;
	SWP_MAT.TAUS_DD = -1.;

	LWP_MAT.EPSLF = -1.;			// default all LW properties
	LWP_MAT.EPSLB = -1.;
	LWP_MAT.TAUL = -1.;

	S = 100.f;		// pleat geometry
	W = 100.f;

	// TODO: handle error message
	return ASHWAT.xw_CheckFixCFSLayer( *this, id);
}		// CFSLAYER::cl_InitDrape
//----------------------------------------------------------------------
void CFSLAYER::DbDump( const char* tag/*=""*/) const
{
	DbPrintf( "\n%s%-.10s   ty=%d\n  %s   %s\n  %s   %s",
		tag, ID, LTYPE,
		LWP_MAT.DbFmt("MAT "), SWP_MAT.DbFmt(),
		LWP_EL.DbFmt("EL  "), SWP_EL.DbFmt());
}
//======================================================================
CFSFILLGAS::CFSFILLGAS()
{	ASHWAT.xw_ClearCFSFillGas( *this);
}
//----------------------------------------------------------------------
#if 0
// no .. use ASHWAT.DLL fcns
CFSFILLGAS::CFSFILLGAS(
	const char* id, double ak, double bk, double acp, double bcp,
	double avisc, double bvisc, double mHat)
{	Init( id, ak, bk, acp, bcp, avisc, bvisc, mHat);
}		// CFSGILLGAS::CFSFILLGAS
//----------------------------------------------------------------------
void CFSFILLGAS::Init(
	const char* id, double ak, double bk, double acp, double bcp,
	double avisc, double bvisc, double mHat)
{	FCSET( ID, id);
	AK = ak; BK = bk; AVISC = avisc; BVISC = bvisc;
	ACP = acp; BCP = bcp; MHAT = mHat;
}	// CFSFILLGASS::Init
//----------------------------------------------------------------------
static CFSFILLGAS FGAir( "Air", 2.873E-3, 7.76E-5, 1002.737, 1.2324E-2, 3.723E-6, 4.94E-8,  28.97);
// Fill gases and their properties
//  Pre-defined, no input provided
//  NOTE: FGX( 1) must be Air
int iG
TYPE (CFSFILLGAS) :: FGX( 4)
DATA (FGX( iG),iG=1,SIZE( FGX)) / &
//                      AK        BK       ACP	    BCP        AVISC     BVISC     MHAT
CFSFILLGAS( "Air",     2.873E-3, 7.76E-5, 1002.737, 1.2324E-2, 3.723E-6, 4.94E-8,  28.97),  &
CFSFILLGAS( "Argon",   2.285E-3, 5.149E-5,521.9285, 0,         3.379E-6, 6.451E-8, 39.948), &
CFSFILLGAS( "Krypton", 0.9443E-3,2.826E-5,248.0907, 0.,        2.213E-6, 7.777E-8, 83.8),   &
CFSFILLGAS( "Xenon",   0.4538E-3,1.723E-5,158.3397, 0.,        1.069E-6, 7.414E-8, 131.30)  &
/
#endif
//======================================================================
CFSGAP::CFSGAP()
{	ASHWAT.xw_ClearCFSGap( *this);
}	// CFSGAP::CFSGAP
//=======================================================================
CFSTY::CFSTY()
{	cf_Clear();
}		// CFSTY::CFSTY
//-----------------------------------------------------------------------
#if 0
void CFSTY::cf_Clear()
{	ASHWAT.xw_ClearCFS( *this);
}		// CFSTY::cf_Clear
#endif
//-----------------------------------------------------------------------
RC CFSTY::cf_OffNormal(			// derive off-normal properties for layers
	float incA,			// beam incident angle, radians
	float vProfA,		// beam vertical profile angle, radians
	float hProfA,		// beam horizontal profile angle, radians
	CFSSWP* swpON)		// returned: array of layer properties
{
	for (int iL=0; iL<NL; iL++)
	{	ASHWAT.xw_OffNormalProperties( _L[ iL], incA, vProfA, hProfA, swpON[ iL]);
#if defined( ASHWAT_CPPTEST)
		_L[ iL].cl_OffNormalTest();
		// if (_L[ iL].LTYPE == ltyGLAZE || _L[ iL].LTYPE == ltyINSCRN)
		{
			CFSSWP swpONX;
			_L[ iL].cl_OffNormalProperties( incA, vProfA, hProfA, swpONX);
			if (swpONX.csw_IsNEQ( swpON[ iL],0.000001))
				printf("\nBad OffNormal");

			CFSLAYER LX( _L[ iL]);
			if (!LX.cl_Finalize())
				printf("\nBad cl_Finalize");
		}
#endif
	}
	return RCOK;
}		// CFSTY::cf_OffNormal
#if defined( ASHWAT_CPPTEST)
//------------------------------------------------------------------------
int CFSLAYER::cl_OffNormalTest()
// exhaustive-ish comparative test of off-normal property derivation
// returns error count
{
	int errCount = 0;
	switch (LTYPE)
	{	case ltyGLAZE: case ltyINSCRN: case ltyROLLB:
		{	for( int incA=0; incA <= 90; incA++)
				errCount += cl_OffNormalTest1( double( incA), 0., 0.);
			break;
		}
		case ltyDRAPE:
		{	// force "case II"
			cl_OffNormalTest1( 0., 0., DEG( atan(2.)));
			for ( int vProfA=-90; vProfA<=90; vProfA+=10)
				for (int hProfA=-90; hProfA<=90; hProfA++)
					errCount += cl_OffNormalTest1( 0., double( vProfA), double( hProfA));
			break;
		}

		default:
			break;
	}
	if (errCount > 0)
		printf("\ncl_OffNormalTest   LTYPE=%d   errCount=%d",
			LTYPE, errCount);
	return errCount;
}
//------------------------------------------------------------------------
int CFSLAYER::cl_OffNormalTest1(
	double incD,		// beam incident angle, deg
	double vProfD,		// beam vertical profile angle, deg
	double hProfD)		// beam horizontal profile angle, deg

{
	double incA = RAD( incD);
	double vProfA = RAD( vProfD);
	double hProfA = RAD( hProfD);
	int ret = 0;
	CFSSWP swpONX;
	if (!cl_OffNormalProperties( incA, vProfA, hProfA, swpONX))
	{	printf( "\ncl_OffNormalProperties ret=false");
		cl_OffNormalProperties( incA, vProfA, hProfA, swpONX);
	}
	CFSSWP swpON;
	ASHWAT.xw_OffNormalProperties( *this, incA, vProfA, hProfA, swpON);
	if (swpONX.csw_IsNEQ( swpON, 0.000001, 0, ID, "OffNorm"))
	{	printf("\nBad cl_OffNormatTest1");
		cl_OffNormalProperties( incA, vProfA, hProfA, swpONX);
		ret = 1;
	}
	return ret;
}
#endif
//------------------------------------------------------------------------
RC CFSTY::cf_SolarX(
	CFSSWP* swpON,		// layer properties w/ off-normal adjustments
	float iBm,			// incident outside beam normal, any power units
	float iDf,			// incident outside diffuse
	float iDfIn,		// incident inside diffuse
	double absL[ CFSMAXNL+1])	// returned: absorbed by layer, same units
								//   [0 .. NL-1] = outside ... inside layers
								//   [NL] = absorbed in black room
{
#if defined( ASHWAT_USECPP)
	swpON[ NL] = swpBlack;
	bool bRet = cf_Solar( swpON-1, iBm, iDf, iDfIn, absL-1);
	RC rc = bRet ? RCOK : RCBAD;
#else
	RC rc = ASHWAT.xw_Solar( NL, swpON, swpBlack, iBm, iDf, iDfIn, absL);
	VZero( absL+NL+1, CFSMAXNL-NL-1);		// clear unused values
#if defined( ASHWAT_CPPTEST)
	CFSSWP swpONX[ CFSMAXNL+2];
	VZero( (char *)swpONX, sizeof( CFSSWP)*(CFSMAXNL+2));
	for (int iL=0; iL<NL; iL++)
		swpONX[ iL+1] = swpON[ iL];
	swpONX[ NL+1] = swpBlack;
	double Source[ CFSMAXNL+1] = { 0. };
	cf_Solar( swpONX, iBm, iDf, iDfIn, Source);
	for (int iL=0; iL<=NL; iL++)
	{	if (absL[ iL] != Source[ iL+1])
			printf( "\ncf_Solar mismatch");
	}
#endif
#endif
	return rc;
}		// CFSTY::cf_SolarDLL
//-----------------------------------------------------------------------------
RC CFSTY::cf_CalcRatings(
	float& Urat,	// returned: NFRC rated winter U-factor, Btuh/ft2-F
	float& SHGCrat)	// returned: NFRC rated summer SHGC, dimless
// returns RCOK if success, else RCBAD
{
	double Ux, SHGCx;
#if defined( ASHWAT_USECPP)
	bool bRet = cf_Ratings( Ux, SHGCx);
	if (bRet)
	{	Urat = USItoIP( Ux);
		SHGCrat = SHGCx;
		return RCOK;
	}
#else
	if (ASHWAT.xw_pAWRatings)
	{	int ret = (*ASHWAT.xw_pAWRatings)( *this, Ux, SHGCx);
		if (ret != 0)
		{
#if defined(ASHWAT_CPPTEST)
			double UxX, SHGCxX;
			bool bRet = cf_Ratings( UxX, SHGCxX);
			if (!bRet || fabs(UxX-Ux) > .0001 || fabs( SHGCxX-SHGCx) > 0.0001)
				printf( "\nBad ratings");
#endif
			Urat = USItoIP( Ux);
			SHGCrat = SHGCx;
			return RCOK;
		}
	}
#endif
	Urat = 0.;		// fail
	SHGCrat = 0.;
	return RCBAD;

}		// CFSTY::cf_CalcRatings
//-----------------------------------------------------------------------------
CFSTYX::CFSTYX(			// build a CFS
	const char* id,			// unique ID (max len = CFSIDLEN)
	float _UcogNFRC,		// externally calculated NFRC cog U-factor, Btuh/ft2-F
	float _SHGCcogNFRC,		// externally calculated NFRC cog SHGC
	const char* layer1ID,	// ID of 1st layer
	...)					// add'l gap / layer info
// call = id, U, SHGC, layerID, gasID, gapT (inches), layerID, ...
{
	Clear();
	FCSET( ID, id);
	UcogNFRC = _UcogNFRC;
	SHGCcogNFRC = _SHGCcogNFRC;
	va_list ap;
	va_start( ap, _SHGCcogNFRC);
	RC rc = RCBAD;
	for (int iL=0; iL<CFSMAXNL; iL++)
	{	const char* layerID = va_arg( ap, const char *);
		const CFSLAYER* pL = ASHWAT.xw_FindLibCFSLAYER( layerID);
		if (!pL)
			break;
		_L[ iL] = *pL;
		NL = iL+1;
		const char* gasID = va_arg( ap, const char *);
		if (!gasID)
		{	rc = RCOK;
			break;
		}
		double gapT = va_arg( ap, double);	// gap (inches)
		if (ASHWAT.xw_BuildGap( _G[ iL], gasID, 25.4*gapT) != RCOK)
			break;
	}
	if (!rc)
	{	rc = ASHWAT.xw_FinalizeCFS( *this);
		if (!rc)
		{	rc = cf_CalcRatings( UcogAW, SHGCcogAW);
			// check that ratings are reasonably consistent
			//   ASHWAT results are slightly different than Windows / THERM
			if (!rc)
				if (frDiff( UcogAW, UcogNFRC) > .05
				 || frDiff( SHGCcogAW, SHGCcogNFRC) > .05)
					rc = RCBAD;
		}
	}
	if (rc)
	{	errCrit( WRN, "Library CFSTY '%s': FUBAR", id);
		Clear();
	}
}		// CFSTYX::CFSTYX
//-----------------------------------------------------------------------------
void CFSTYX::Clear()
{	UcogNFRC = 0.;
	SHGCcogNFRC = 0.;
	UcogAW = 0.;
	SHGCcogAW = 0.;
	cf_Clear();
}		// CFSTYX::Clear
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class FENAW -- XSURF substructure for ASHWAT glazings
///////////////////////////////////////////////////////////////////////////////
FENAW::FENAW( XSURF* pXS /*=NULL*/)
#if defined( ASHWAT_REV2)
	: fa_X( 25000)
#endif
{
	fa_pXS = pXS;
	fa_Init();
}		// FENAW::FENAW
//-----------------------------------------------------------------------------
void FENAW::fa_Init()
{	// fa_pXS do not alter
	fa_UNFRC = fa_SHGC = fa_frmF = fa_frmArea = fa_frmUNFRC = fa_frmUC = 0.f;
	fa_frmSbcI.sb_Init( fa_pXS, 0);
	fa_frmSbcO.sb_Init( fa_pXS, 1);
	fa_CFS.cf_Clear();
	VZero( fa_refDesc, CFSIDLEN+30);
	fa_mSolar = fa_mCond = fa_dfAbsB = fa_dfRhoB = fa_dfTauB = 0.f;
	VZero( fa_dfLAbsB, CFSMAXNL+1);
	VZero( fa_dfLAbsF, CFSMAXNL+1);
	VZero( fa_bmLAbsF[ 0], 24*(CFSMAXNL+1));
	fa_iterControl = 100;
	fa_nCall = fa_nCalc = 0;
	fa_awO.aw_Init();
#if defined( ASHWAT_REV2)
	fa_X.clear();
#else
	fa_incSlrPC = 0.;
	fa_txaiPC = fa_hxaiPC = fa_txaoPC = 0.f;
#endif
}		// FENAW::FENAW
//-----------------------------------------------------------------------------
const char* FENAW::fa_Name() const
{	return fa_pXS ? fa_pXS->xs_Name() : "?";
}		// FENAW::fa_Name
//-----------------------------------------------------------------------------
#if 0
FENAW& FENAW::Copy( const FENAW& f)
{
	XSURF* pXSSave = fa_pXS;
	memcpy( this, &f, sizeof( FENAW));	// bitwise copy OK
	fa_pXS = pXSSave;
	return *this;
}		// FENAW::Copy
#endif
//-----------------------------------------------------------------------------
RC FENAW::fa_Setup(
	int oc)		// 0=int shades open, 1=int shades closed
{

	float scm = oc ? fa_pXS->scc : fa_pXS->sco;
	float dirtShadeF = (1.f - fa_pXS->xs_dirtLoss) * scm;
	RC rc = fa_SetupBare( dirtShadeF);		// set up w/o int or ext shades

	if (!rc)
	{	if (fa_pXS->xs_exShd != C_EXSHDCH_NONE)
		{	const CFSLAYER* pL = ASHWAT.xw_FindLibCFSLAYER( "InsScrn");
			CFSGAP G;
			ASHWAT.xw_BuildGap( G, "Air", 1.f*25.4f, gtyOPENOut);
			rc = fa_InsertLayer( 0, pL, &G);
		}
	}
	if (!rc)
	{	if (fa_pXS->xs_inShd != C_INSHDCH_NONE
		 && (oc == 1 || !fa_pXS->xs_HasControlledShade()))
		{	const CFSLAYER* pL = ASHWAT.xw_FindLibCFSLAYER( "DrapeMed");
			CFSGAP G;
			ASHWAT.xw_BuildGap( G, "Air", 3.f*25.4f, gtyOPENIn);
			rc = fa_InsertLayer( fa_NL(), pL, &G);
		}
	}

	// diffuse optical properties (constant)
	if (!rc)
		rc = fa_DfSolar();

#if defined( DEBUGDUMP)
	if (DbDo( dbdASHWAT))
		DbDump();
#endif

	return rc;

}		// FENAW::fa_Setup
//-----------------------------------------------------------------------------
RC FENAW::fa_SetupBare(		// FENAW init glazing alone
	float dirtShadeF)	// SHGC dirtloss/shade multiplier
						//   1 = no losses

// caller adds insect screen and/or interior shade if required
{
	RC rc = RCOK;
	int NL;
	float fGlz = fa_pXS->xs_fMult;	// fraction of assembly area that is glazed
	float fFrm = 1.f - fGlz;
	if (fFrm < .001f)
	{	fFrm = 0.f;		// drop tiny frames
		fGlz = 1.f;
	}
	fa_frmArea = fa_pXS->xs_area * fFrm;

	if (!rc)
	{	NL = fa_pXS->xs_NGlz;
		fa_SHGC = fa_pXS->xs_SHGC;			// user's target assembly SHGC
		fa_UNFRC = fa_pXS->xs_UNFRC;

		fa_frmUNFRC = fa_UNFRC;		// assume frame has same U-factor as overall
		fa_frmUC = fa_UtoCNFRC( fa_frmUNFRC);	// remove NFRC surf conductances
												//   to get surface-to-surface

		float SHGCglz = fa_SHGC/max( fGlz, .1f);	// required glazing SHGC (assume frame SHGC = 0)

		const CFSTYX* pCFSglz = ASHWAT.xw_FindLibCFSTYX( SHGCglz, NL);
		if (!pCFSglz)
			rc = errCrit( WRN, "ASHWAT '%s': library match not found", fa_Name());
		else
			fa_CFS = *pCFSglz;
	}
	if (!rc)
	{	// description of reference glazing
		sprintf( fa_refDesc, "%s (SHGC=%.3f U=%.3f)",
			FCGET( fa_CFS.ID), fa_CFS.SHGCcogNFRC, fa_CFS.UcogNFRC);

		// adjustment factors
		//   ASHWAT models glass only
		//   factors adjust energy flows to reconcile to assembly ratings
		fa_mSolar = dirtShadeF * fa_SHGC / (fa_CFS.SHGCcogNFRC * fGlz);
		fa_mCond = (fa_UNFRC - fa_frmUNFRC*fFrm) / (fa_CFS.UcogNFRC*fGlz);

		// frame
		if (fa_frmArea > 0.f)
		{	fa_frmSbcI = fa_SBC( 0);		// same boundary conditions as XSRAT
			fa_frmSbcO = fa_SBC( 1);
			// TODO: roughness or other differences?
		}
	}
	return rc;

}		// FENAW::fa_SetupBare
//-----------------------------------------------------------------------------
RC FENAW::fa_InsertLayer(
	int iLIns,		// idx at which new layer is inserted (0 based)
					//   layers/gaps at and after (inside) this position are moved "in")
	const CFSLAYER* pL,	// layer to copy into CFS
	const CFSGAP* pG)	// gap to insert after (inside) of layer
						//   *except* if adding layer at inside, in which case gap is before layer
// returns RCOK
{
	RC rc = RCOK;
	int nL = fa_NL();
	if (nL == CFSMAXNL)
	{	rc = err( "ASHWAT '%s': cannot add shading layer (max # of layers is %d)",
			fa_Name(), CFSMAXNL);
	}
	else if (iLIns < 0 || iLIns > nL)
	{	rc = err( "ASHWAT '%s': invalid layer insert (NL=%d, iLIns=%d)",
			fa_Name(), nL, iLIns);
	}
	else
	{	if (iLIns < nL)
		{	for (int iL=nL-1; iL >= iLIns; iL--)
			{	fa_CFS._L[ iL+1] = fa_CFS._L[ iL];
				if (iL < nL-1)
					fa_CFS._G[ iL+1] = fa_CFS._G[ iL];
			}
		}
		fa_CFS._L[ iLIns] = *pL;
		fa_CFS._G[ min( iLIns, nL-1)] = *pG;
		fa_CFS.NL++;

		rc = ASHWAT.xw_FinalizeCFS( fa_CFS);
	}

	return rc;
}	// FENAW::fa_InsertLayer
//-----------------------------------------------------------------------------
void FENAW::DbDump() const
{
	int nL = fa_NL();
	DbPrintf("\nASHWAT '%s'  NL=%d  LibBase='%s'\n", fa_Name(), nL, FCGET( fa_CFS.ID));
	for (int iL=0; iL<nL; iL++)
	{	fa_CFS._L[ iL].DbDump( strtprintf("%d ", iL+1));
		if (iL < nL-1)
			DbPrintf( "  %s\n", fa_CFS._G[ iL].DbFmt());
	}
}		// FENAW::DbDump
//-----------------------------------------------------------------------------
RC FENAW::fa_BmSolar(			// ASHWAT beam solar (optical) calcs
	int iH,					// hour of day (0 - 23)
	float incA /*=0.f*/,	// angle of incidence, radians
	float vProfA /*=0.f*/,
	float hProfA /*=0.f*/)
// sets fa_bmLAbsF[ iH] to absorbed by layer per unit incident
//      fa_bmLAbsF[ iH][ NL+1] = room absorbed = transmitted
{
	RC rc = RCOK;
	VZero( fa_bmLAbsF[ iH], CFSMAXNL+1);
	if (incA < kPiOver2)
	{	CFSSWP swpON[ CFSMAXNL];
		rc = fa_CFS.cf_OffNormal( incA, vProfA, hProfA, swpON);
		if (!rc)
			rc = fa_CFS.cf_SolarX( swpON, 1., 0., 0., fa_bmLAbsF[ iH]);
		else
			VZero( fa_bmLAbsF[ iH], CFSMAXNL+1);	// insurance after fail
	}
	return rc;
}		// FENAW::fa_BmSolar
//-----------------------------------------------------------------------------
RC FENAW::fa_DfSolar()			// ASHWAT diffuse solar (optical) calcs
// sets fa_dfLAbsB and fa_dfLAbsF to absorbed by layer
//				per unit incident front and back
//      fa_bmLAbsB[ NL+1] = room absorbed = reflected from inside incident

{
	CFSSWP swpON[ CFSMAXNL];
	RC rc = fa_CFS.cf_OffNormal( 0., 0., 0., swpON);
	VZero( fa_dfLAbsF, CFSMAXNL+1);
	VZero( fa_dfLAbsB, CFSMAXNL+1);
	if (!rc)
		rc = fa_CFS.cf_SolarX( swpON, 0., 1., 0., fa_dfLAbsF);
	if (!rc)
		rc = fa_CFS.cf_SolarX( swpON, 0., 0., 1., fa_dfLAbsB);
	if (!rc)
	{	// back (room) side overall properties
		//   last "layer" is room
		fa_dfAbsB = VSum( fa_dfLAbsB, fa_NL());
		fa_dfRhoB = fa_dfLAbsB[ fa_NL()];
		fa_dfTauB = 1. - fa_dfRhoB - fa_dfAbsB;
	}
	else
	{	fa_dfRhoB = 0.f;		// insurance after fail
		fa_dfTauB = 0.f;
		VZero( fa_dfLAbsF, CFSMAXNL+1);
		VZero( fa_dfLAbsB, CFSMAXNL+1);
	}
	return rc;
}		// FENAW::fa_DfSolar
//-----------------------------------------------------------------------------
RC FENAW::fa_CalcRatings()
{
	RC rc = fa_CFS.cf_CalcRatings( fa_CFS.SHGCcogAW, fa_CFS.UcogAW);
	return rc;
}		// FENAW::fa_CalcRatings
//-----------------------------------------------------------------------------
#if 0
RC FENAW::fa_InitForRatingTargets(
	float SHGCtarg,
	float Utarg)
{
	RC rc = fa_CalcRatings();		// center of glass for current layers

}		// FENAW::fa_InitForRatingTargets
#endif
//-----------------------------------------------------------------------------
RC FENAW::fa_FrameBC()
{
	RC rc = RCOK;
	// frame is modeled as a quick surface (no mass)
	if (fa_frmArea > 0.)
	{	fa_frmSbcI.sb_SetCoeffs( fa_frmArea, fa_frmUC);
		fa_frmSbcO.sb_SetCoeffs( fa_frmArea, fa_frmUC);
	}
	return rc;
}		// FENAW::fa_FrameBC
//-----------------------------------------------------------------------------
RC FENAW::fa_FrameQS()
{
	RC rc = RCOK;
	if (fa_frmArea > 0.)
	{	ZNR& z = ZrB[ fa_frmSbcI.sb_zi];
		z.zn_AccumBalTermsQS( fa_frmArea, fa_frmSbcI, fa_frmSbcO);
		// if (fa_frmSbcO.sb_zi) ... generalize if supporting interzone fenestration
	}
	return rc;
}		// FENAW::fa_FrameQS
//-----------------------------------------------------------------------------
RC FENAW::fa_FrameEndSubhr()
{
	RC rc = RCOK;
	if (fa_frmArea > 0.)
	{	ZNR& z = ZrB[ fa_frmSbcI.sb_zi];
		fa_frmSbcI.sb_SetTx();
		fa_frmSbcO.sb_SetTx();
		z.zn_EndSubhrQS( fa_frmArea, fa_frmUC, fa_frmSbcI, fa_frmSbcO);
	}
	return rc;
}		// FENAW::fa_FrameEndSubhr
//-----------------------------------------------------------------------------
RC FENAW::fa_Subhr(				// subhr calcs for single time step
	float fActive,	// fraction of glazed area to be included in model, 0 - 1
					//   (re controlled shades)

	int bDoFrm)		// nz = do frame calcs
{
const double tol = .001;

#if !defined( ASHWAT_USECPP)
	if (!ASHWAT.xw_pAWThermal)
		return RCBAD;
#endif

	if (Top.isBegRun)
	{	fa_nCall = 0;
		fa_nCalc = 0;
	}
	fa_nCall++;		// count call

	// working copies
	int nL = fa_CFS.NL;
	SBC& sbcI = fa_SBC( 0);
	SBC& sbcO = fa_SBC( 1);
	int iH = Top.iHrST;		// hour (0-23) standard time
	ZNR& z = ZrB[ sbcI.sb_zi];
	int iL;

	int bDbPrint = 0;	// nz if doing debug printing
#if defined( DEBUGDUMP)
	bDbPrint = DbDo( dbdASHWAT);
#endif

	RC rc = RCOK;

	// solar gain
	double incSlrIP = sbcO.sb_sgTarg.st_tot + sbcI.sb_sgTarg.st_tot;
	double incSlr = IrIPtoSI( incSlrIP);	// total incident solar, W/m2 (used re SHGC calc)
	double absSlr[ CFSMAXNL+1];				// absorbed by layer, W/m2 (including layer nL = "room")
	double absSlrF[ CFSMAXNL+1];			// abs fraction by layer (test/debug aid only)

#if defined( ASHWAT_REV2)
	int bDoAW = TRUE;
#if 0 && defined( _DEBUG)
	if (fa_nCalc == 0)
		fa_PerfMap();
#endif
#else
	// do full ASHWAT iff conditions have changed "enough"
	int bDoAW = Top.isBegRun
		     // || Top.iSubhr == 1 ... idea: effect of hour changes appear at 2nd step
			 || fabs( fa_txaoPC - sbcO.sb_txa) > Top.tp_AWTrigT		// default 1 F
			 || fabs( fa_txaiPC - sbcI.sb_txa) > Top.tp_AWTrigT
			 || frDiff( fa_incSlrPC, incSlr) > Top.tp_AWTrigSlr		// default .05
			 || frDiff( fa_hxaiPC, sbcI.sb_hxa) > Top.tp_AWTrigH;	// default .1
#endif

	if (bDoAW || bDbPrint)
	{	// derive layer-by-layer absorbed solar
		iL=0;
		if (incSlr > 0.)
		{	// nL+1 cuz innermost = room pseudo-layer
			for (iL=0; iL<nL+1; iL++)
			{	absSlr[ iL] = IrIPtoSI( fa_bmLAbsF[ iH][ iL]*sbcO.sb_sgTarg.st_bm
				 			          + fa_dfLAbsF[ iL]     *sbcO.sb_sgTarg.st_df
									  + fa_dfLAbsB[ iL]     *sbcI.sb_sgTarg.st_df);
				absSlrF[ iL] = absSlr[ iL] / incSlr;
			}
		}
		for (; iL < CFSMAXNL+1; iL++)
		{	absSlr[ iL] = 0.;	// clear unused values (= all at night)
			absSlrF[ iL] = 0.;
		}
	}

	AWIN awI;
	awI.aw_Set(
		sbcO.sb_txa, sbcO.sb_hxa, sbcO.sb_txr,
		sbcI.sb_txa, sbcI.sb_hxa, sbcI.sb_txr,
		incSlr, absSlr);

#if defined( ASHWAT_REV2)
	rc |= fa_ThermalCache( awI, fa_awO);
	fa_iterControl = -1;
#else
	if (bDoAW)
	{	fa_incSlrPC = incSlr;
		fa_txaiPC = sbcI.sb_txa;
		fa_hxaiPC = sbcI.sb_hxa;
		fa_txaoPC = sbcO.sb_txa;

		rc |= fa_Thermal( awI, fa_awO);
		fa_iterControl = -1;		// don't init / don't iterate
									//   subsequent calls
	}
#endif

	// map results to CSE vars
	//  Note CSE 0=inside, ASHWAT 0=outside
	sbcI.sb_tSrf = DegKtoF( fa_awO.aw_TL[ nL-1]);		// inside
	sbcO.sb_tSrf = DegKtoF( fa_awO.aw_TL[ 0]);			// outside
	sbcO.sb_txe = sbcO.sb_txr*fa_awO.aw_FHR_OUT + sbcO.sb_txa*( 1. - fa_awO.aw_FHR_OUT);	// effective outside temp

	double Ag = fActive * fa_pXS->xs_AreaGlazed();		// note: glazed area only
	z.zn_nAirSh += Ag*(sbcO.sb_txe*fa_awO.aw_cc + sbcO.sb_sgTarg.st_tot*fa_awO.aw_FP*fa_mSolar);
	z.zn_dAirSh += Ag*fa_awO.aw_cc;
	z.zn_nRadSh += Ag*(sbcO.sb_txe*fa_awO.aw_cr + sbcO.sb_sgTarg.st_tot*fa_awO.aw_FM*fa_mSolar);
	z.zn_dRadSh += Ag*fa_awO.aw_cr;
	z.zn_cxSh   += Ag*USItoIP( fa_awO.aw_Cx);

	// zone solar gain
	//   add inward flowing fraction of absorbed
	//   transmitted already included in qSgTotSh
#if 0 && defined( _DEBUG)
	double trans = sbcO.sb_sgTarg.st_bm*fa_BmTauF( iH) + sbcO.sb_sgTarg.st_df*fa_DfTauF();
	if (fabs( Ag*fa_mSolar*trans - z.qSgTotSh) > .1)
		printf( "mismatch ");
#endif
	z.qSgTotSh  += Ag*sbcO.sb_sgTarg.st_tot*(fa_awO.aw_FP+fa_awO.aw_FM)*fa_mSolar;

#if defined( DEBUGDUMP)
	if (bDbPrint)
	{
#define AWO( f) fa_awO.aw_##f
#define LTF( iL) DegKtoF( AWO( TL[ iL]))
		DbPrintf( "\nASHWAT %s  nL=%d  tao=%0.2f tro=%0.2f teo=%0.2f   tai=%0.2f tri=%0.2f tei=%0.2f\n",
				fa_pXS->xs_Name(), nL,
				sbcO.sb_txa, sbcO.sb_txr, sbcO.sb_txe, sbcI.sb_txa, sbcI.sb_txr, sbcI.sb_txe);
		DbPrintf("   incSlr=%0.2f  trans=%0.2f (%0.3f)",
			sbcO.sb_sgTarg.st_tot, IrSItoIP( absSlr[ nL]), absSlrF[ nL]);
		VDbPrintf( "  layer abs=", absSlr, nL, "  %0.2f", 1./cfIr);
		// layer temps, F
		DbPrintf("   layer temps= ");
		for (iL=0; iL<nL; iL++)
			DbPrintf( "  %0.2f", LTF( iL));
		DbPrintf("\n   SHGC=%0.3f  ucg=%0.3f  cr=%0.3f  cc=%0.3f  FP=%0.3f  FM=%0.3f  Cx=%0.3f\n",
			AWO( SHGCrt), AWO( Urt), AWO( cr), AWO( cc), AWO( FP), AWO( FM), USItoIP( AWO( Cx)));
		SBC::sb_DbPrintf( "COG", sbcO, sbcI);
		if (fa_frmArea > 0.  && bDoFrm)
		{	const FENAW* A0= fa_pXS->xs_pFENAW[ 0];	// frame BCs always in [ 0]
			SBC::sb_DbPrintf( "Frame", A0->fa_frmSbcO, A0->fa_frmSbcI);
		}
	}
#endif

	return rc;
}		// FENAW::fa_Subhr
//-----------------------------------------------------------------------------
RC FENAW::fa_EndSubhr(			// ASHWAT end-of-subhour (accounting etc)
	float fActive)	// fraction of glazed area to be included in model, 0 - 1
					//   (re controlled shades)
{
	SBC& sbcI = fa_SBC( 0);
	SBC& sbcO = fa_SBC( 1);
	ZNR& z = ZrB[ sbcI.sb_zi];

	// glazed area
	double Ag = fActive * fa_pXS->xs_AreaGlazed();
	double cond = Ag * (  fa_awO.aw_cc*(sbcO.sb_txe - sbcI.sb_txa)
		                + fa_awO.aw_cr*(sbcO.sb_txe - sbcI.sb_txr));
	z.zn_qCondQS += cond;		// glazing conduction

#if defined( ASHWAT_STATS)
	if (!Top.isWarmup)
	{
	fax_stats[ fsxCc].sx_Obs( fax_cc, fa_cc);
	fax_stats[ fsxCr].sx_Obs( fax_cr, fa_cr);
	fax_stats[ fsxU].sx_Obs( fax_Urt, fa_Urt);
	fax_stats[ fsxFM].sx_Obs( fax_FM, fa_FM);
	fax_stats[ fsxFP].sx_Obs( fax_FP, fa_FP);
	fax_stats[ fsxCx].sx_Obs( fax_Cx, fa_Cx);
	fax_stats[ fsxFHROUT].sx_Obs( fax_FHR_OUT, fa_FHR_OUT);

	double qCndAir = fa_cc*(sbcO.sb_txe - sbcI.sb_txa);
	double qCndRad = fa_cr*(sbcO.sb_txe - sbcI.sb_txr);

	double otxeX = sbcO.sb_txr*fax_FHR_OUT + sbcO.sb_txa*( 1. - fax_FHR_OUT);	// effective outside temp

	double qCndAirX = fax_cc*(otxeX - sbcI.sb_txa);
	double qCndRadX = fax_cr*(otxeX - sbcI.sb_txr);
	double qSlrAir = sbcO.sb_sgTarg.st_tot*fa_FP*fa_mSolar;
	double qSlrRad = sbcO.sb_sgTarg.st_tot*fa_FM*fa_mSolar;
	double qSlrAirX = sbcO.sb_sgTarg.st_tot*fax_FP*fa_mSolar;
	double qSlrRadX = sbcO.sb_sgTarg.st_tot*fax_FM*fa_mSolar;

	double qTot  = qCndAir  + qSlrAir  + qCndRad  + qSlrRad;
	double qTotX = qCndAirX + qSlrAirX + qCndRadX + qSlrRadX;

	double qRad  = qCndRad + qSlrRad + fa_Cx * (sbcI.sb_txa - sbcI.sb_txr);
	double qRadX = qCndRadX + qSlrRadX + fax_Cx * (sbcI.sb_txa - sbcI.sb_txr);

	fax_stats[ fsxQTot].sx_Obs( qTotX, qTot);
	fax_stats[ fsxQRad].sx_Obs( qRadX, qRad);
	fax_stats[ fsxQAir].sx_Obs( qTotX-qRadX, qTot-qRad);
	fax_stats[ fsxQSlrRad].sx_Obs( qSlrRadX, qSlrRad);
	fax_stats[ fsxQSlrAir].sx_Obs( qSlrAirX, qSlrAir);
	fax_stats[ fsxQCndRad].sx_Obs( qCndRadX, qCndRad);
	fax_stats[ fsxQCndAir].sx_Obs( qCndAirX, qCndAir);

#if 0
x	// FR very unstable, don't understand 5-23-2011
x	double FR =  qTot != 0  ? qRad  / qTot  : 0.;
x	double FRX = qTotX != 0 ? qRadX / qTotX : 0.;
x	if (fabs( FR-FRX) > .3)
x		printf( "FR mismatch!");
x	fax_stats[ fsxFR.sx_Obs( FRX, FR);
#endif

	if (Top.tp_IsLastStep())
	{	static const char* tags[ fsxCOUNT] =
		{ "QTot", "QRad", "QAir", "QSlrRad", "QSlrAir", "QCndRad", "QCndAir",
		  "Cx", "Cr", "Cc", "U", "FM", "FP", "FHROUT"};

		DbPrintf( "\nASHWAT %s  nCall=%d   nCalc=%d   fCalc=%.3f",
				fa_pXS->xs_Name(), fa_nCall, fa_nCalc, double( fa_nCalc)/double( fa_nCall));
		for (int i=0; i<fsxCOUNT; i++)
			fax_stats[ i].DbPrintf( strtprintf( "\n%10s:", tags[ i]));
	}
	}
#endif
#if 0 && defined( _DEBUG)
	if (Top.tp_IsLastStep())
#if defined( ASHWAT_REV2)
		DbPrintf( "\nASHWAT %s  nCall=%d   nCalc=%d   fCalc=%.3f   mapLoad=%.3f",
			fa_pXS->xs_Name(), fa_nCall, fa_nCalc, double( fa_nCalc)/double( fa_nCall),
			fa_X.max_load_factor());
#else
		DbPrintf( "\nASHWAT %s  nCall=%d   nCalc=%d   fCalc=%.3f",
			fa_pXS->xs_Name(), fa_nCall, fa_nCalc, double( fa_nCalc)/double( fa_nCall));
#endif
#endif

	return RCOK;
}	// FENAW::fa_EndSubhr
//-----------------------------------------------------------------------------
/*static*/ float FENAW::fa_UtoCNFRC(
	float UNFRC)	// U-factor with nominal NFRC surface conductance
					//   Btuh/ft2-F
// returns surface-to-surface conductance, Btuh/ft2-F
//         (returns 1000 if R < surface conductances)
{
	float RGlass = 1.f/max( UNFRC, .01f) - RoutNFRC - RinNFRC;
	return 1.f / max( .001f, RGlass);
}		// FENAW::fa_UtoCNFRC
//-----------------------------------------------------------------------------
/*static*/ float FENAW::fa_CtoUNFRC(
	float Uc)		// surface-to-surface conductance, Btuh/ft2-F
{
	return 1.f / (1.f/max( Uc, .01f) + RoutNFRC + RinNFRC);
}		// FENAW::fa_CtoUNFRC
//=============================================================================
void AWIN::aw_Round()
{
	roundNearest( aw_txaO, 5.);
	roundNearest( aw_hxaO, .5);
	roundNearest( aw_txrO, 5.);
	roundNearest( aw_txaI, 2.);
	roundNearest( aw_hxaI, .1);
	roundNearest( aw_txrI, 2.);
	roundNearest( aw_incSlr, 10.);
	for (int iL=0; iL<CFSMAXNL+1; iL++)
		roundNearest (aw_absSlr[ iL], 2.);
}		// AWIN::aw_Round
//-----------------------------------------------------------------------------
size_t AWIN::aw_Hash() const
{	return (size_t( (aw_txaO+100.)/5.))
         + (size_t( aw_txaI/2.) << 6)
		 + (size_t( aw_hxaO*2.) << 12)
		 + (size_t( aw_hxaI*10.) << 16)
		 + (size_t( aw_incSlr/10.) << 22);
}		// AWIN::aw_Hash
//-----------------------------------------------------------------------------
bool AWIN::operator==( const AWIN& awI) const
{	if (aw_txaO != awI.aw_txaO
	 || aw_incSlr != awI.aw_incSlr)
		return false;
	if ( aw_incSlr > 0.)
	{	// no need to compare absorbed if no incident
		for (int iL=0; iL<CFSMAXNL+1; iL++)
			if (aw_absSlr[ iL] != awI.aw_absSlr[ iL])
				return false;
	}
	return aw_hxaO == awI.aw_hxaO
		&& aw_txrO == awI.aw_txrO
		&& aw_txaI == awI.aw_txaI
		&& aw_hxaI == awI.aw_hxaI
		&& aw_txrI == awI.aw_txrI;

}		// AWIN::operator==
//-----------------------------------------------------------------------------
WStr AWIN::aw_CSVHeading( const char* tag/*=NULL*/) const
{	const char* cols[] = { "txaO", "hxaO", "txrO", "txaI", "hxaI", "txrI",
		"incSlr", "absSlr0", "absSlr1", "absSlr2", "absSlr3", NULL };

	WStr hdg;
	for (int iH=0; cols[ iH]; iH++)
	{	if (iH)
			hdg += ',';
		if (tag)
			hdg += tag;
		hdg += cols[ iH];
	}
	return hdg;
}		// AWIN::aw_CSVHeading
//-----------------------------------------------------------------------------
WStr AWIN::aw_CSVRow() const
{	return WStrPrintf(
		"%.1f,%.2f,%.1f,%.1f,%.2f,%.1f,%.f,%.1f,%.1f, %.1f,%.1f",
			aw_txaO, aw_hxaO, aw_txrO, aw_txaI, aw_hxaI, aw_txrI,
			aw_incSlr,
			aw_absSlr[ 0], aw_absSlr[ 1], aw_absSlr[ 2], aw_absSlr[ 3]);
}	// AWIN::aw_CSVRow
//-----------------------------------------------------------------------------
WStr AWOUT::aw_CSVHeading( const char* tag/*=NULL*/) const
{	const char* cols[] = { "Urt", "Cx", "FM", "FP", "FHR_IN", "FHR_OUT",
		"TL0", "TL1", "TL2", "TL3", NULL };

	WStr hdg;
	for (int iH=0; cols[ iH]; iH++)
	{	if (iH)
			hdg += ',';
		if (tag)
			hdg += tag;
		hdg += cols[ iH];
	}
	return hdg;
}		// AWOUT::aw_CSVHeading
//-----------------------------------------------------------------------------
WStr AWOUT::aw_CSVRow() const
{
#define TL( iL) (aw_TL[ iL] > 0. ? DegKtoF( aw_TL[ iL]) : 0.)
	return WStrPrintf(
		"%.3f,%.2f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f",
		aw_Urt, aw_Cx, aw_FM, aw_FP, aw_FHR_IN, aw_FHR_OUT,
		TL( 0), TL( 1), TL(2), TL( 3));
#undef TL
}	// AWOUT::aw_CSVRow
//=============================================================================
#if defined( ASHWAT_REV2)
RC FENAW::fa_ThermalCache(		// ASHWAT thermal calcs
	const AWIN& awI,		// input values
	AWOUT& awO)			// output values
{
	RC rc = RCOK;
	AWIN awIX( awI);
	awIX.aw_Round();
	if (fa_X.size() > 0 && fa_X.count( awIX))
		awO = fa_X[ awIX];
	else
	{
#if defined( XX)
		if (fa_nCalc == 0)
		{
		}
		fa_Thermal( awI, awO);		// unrounded calc
		WStr oUnR = awO.aw_CSVRow();
#endif
		rc = fa_Thermal( awIX, awO);
		fa_X[ awIX] = awO;
#if defined( XX)


#endif
	}
	return rc;
}
#endif
//-----------------------------------------------------------------------------
RC FENAW::fa_Thermal(		// ASHWAT thermal calcs
	const AWIN& awI,		// input values
	AWOUT& awO)			// output values

{
	TMRSTART( TMR_AWCALC);		// time calcs only
								// see also TMR_AWTOT
	RC rc = RCOK;
	int nL = fa_CFS.NL;
	static const double tol = .001;
	double Q[ CFSMAXNL+2];
	double ucgSI;
	fa_nCalc++;
	double QInConv, QInRad;
#if defined( ASHWAT_USECPP)
	bool bRet = fa_CFS.cf_Thermal(
			DegFtoK( awI.aw_txaI), DegFtoK( awI.aw_txaO),
			UIPtoSI( awI.aw_hxaI), UIPtoSI( awI.aw_hxaO),
			DegFtoK( awI.aw_txrI), DegFtoK( awI.aw_txrO),
			awI.aw_incSlr, awI.aw_absSlr-1,
			tol, fa_iterControl, awO.aw_TL-1, Q,
			QInConv, QInRad,
			ucgSI, awO.aw_SHGCrt, awO.aw_Cx,
			awO.aw_FP, awO.aw_FM,	// note order: conv, rad
			awO.aw_FHR_IN, awO.aw_FHR_OUT);
	if (!bRet)
		rc = RCBAD;
#else
#if defined( ASHWAT_NEWCALL)
		int ret = (*ASHWAT.xw_pAWThermal)( fa_CFS,
			DegFtoK( awI.aw_txaI), DegFtoK( awI.aw_txaO),
			UIPtoSI( awI.aw_hxaI), UIPtoSI( awI.aw_hxaO),
			DegFtoK( awI.aw_txrI), DegFtoK( awI.aw_txrO),
			awI.aw_incSlr, awI.aw_absSlr,
			tol, fa_iterControl, awO.aw_TL, Q,
			QInConv, QInRad,
			ucgSI, awO.aw_SHGCrt, awO.aw_Cx,
			awO.aw_FP, awO.aw_FM,	// note order: rad, conv
			awO.aw_FHR_IN, awO.aw_FHR_OUT);
#else
	int ret = (*ASHWAT.xw_pAWThermal)( fa_CFS,
			DegFtoK( awI.aw_txaI), DegFtoK( awI.aw_txaO),
			UIPtoSI( awI.aw_hxaI), UIPtoSI( awI.aw_hxaO),
			DegFtoK( awI.aw_txrI), DegFtoK( awI.aw_txrO),
			awI.aw_incSlr, awI.aw_absSlr,
			tol, fa_iterControl, awO.aw_TL, Q,
			ucgSI, awO.aw_SHGCrt, awO.aw_Cx,
			awO.aw_FM, awO.aw_FP,	// note order: rad, conv
			awO.aw_FHR_IN, awO.aw_FHR_OUT);
#endif
	if (ret == 0)
		rc = RCBAD;
#if defined( ASHWAT_CPPTESTTHERMAL)
	double QX[ CFSMAXNL+2];
	double ucgSIX, SHGCrtX, CxX, FMX, FPX, FHR_INX, FHR_OUTX, QInConvX, QInRadX;
	bool bRetX = fa_CFS.cf_Thermal(
			DegFtoK( awI.aw_txaI), DegFtoK( awI.aw_txaO),
			UIPtoSI( awI.aw_hxaI), UIPtoSI( awI.aw_hxaO),
			DegFtoK( awI.aw_txrI), DegFtoK( awI.aw_txrO),
			awI.aw_incSlr, awI.aw_absSlr-1,
			tol, fa_iterControl,
			awO.aw_TLX-1, QX,
			QInConvX, QInRadX,
			ucgSIX, SHGCrtX, CxX, 
			FPX, FMX,	// note order: conv, rad
			FHR_INX, FHR_OUTX);
	double TErr = 0.;
	for (int iL=1;iL<=fa_CFS.NL; iL++)
		TErr += fabs( awO.aw_TLX[ iL-1] - awO.aw_TL[ iL-1]);
	double QErr = 0.;
	for (int iL=1;iL<=fa_CFS.NL; iL++)
		QErr += fabs( QX[ iL] - Q[ iL]);
	if (TErr > 0.00001 || QErr > 0.00001
	 || fabs( ucgSIX - ucgSI) > 0.000001
	 || fabs( SHGCrtX - awO.aw_SHGCrt) > 0.000001
	 || fabs( FHR_INX - awO.aw_FHR_IN) > 0.000001
	 || fabs( FHR_OUTX - awO.aw_FHR_OUT) > 0.000001
#if defined( ASHWAT_NEWCALL)
	 || fabs( QInConvX - QInConv) > 0.00001
	 || fabs( QInRadX - QInRad) > 0.00001
#endif
	 || fabs( FMX - awO.aw_FM) > 0.000001
	 || fabs( FPX - awO.aw_FP) > 0.000001
	 || fabs( CxX - awO.aw_Cx) > 0.000001)
	 		printf( "\ncf_Thermal mismatch, NL=%d  Err=%0.4f", fa_CFS.NL, TErr);
#endif	// ASHWAT_CPPTEST
#endif	// ASHWAT_USECPP

#if defined( _DEBUG)
	for (int iL=0; iL<nL; iL++)
	{	if (_isnan( awO.aw_TL[ iL])
		    || awO.aw_TL[ iL] < 200.
			|| awO.aw_TL[ iL] > 400.)
			warn( "Window '%s': Implausible layer %d temp (%.f degF)\n",
					fa_Name(), iL+0, DegKtoF( awO.aw_TL[ iL]));
	}
#endif
	awO.aw_Urt = fa_mCond * USItoIP( ucgSI);	// "real time" U-factor at current conditions
	awO.aw_cr = awO.aw_Urt*awO.aw_FHR_IN;	// effective conductance, tr-to-txeO
	awO.aw_cc = awO.aw_Urt - awO.aw_cr;		// effective conductance, ta-to-txeO

	TMRSTOP( TMR_AWCALC);

	return rc;

}	// FENAW::fa_Thermal
#if defined( _DEBUG)
//-----------------------------------------------------------------------------
RC FENAW::fa_PerfMap()		// dump numerous cases for testing
{
const float tO[] = { 10.f, 30.f, 50.f, 70.f, 90.f, 110.f, -1.f};
const float hxO[] = { .1f, 2.f, 4.f, -1.f };
const float tI[] = { 60.f, 70.f, 80.f, -1.f };
const float hxI[] = { 0.1f, 0.3f, 0.5f, -1.f };
const float incS[] = { 0.f, 200.f, -1.f };
const float absS[] = { 0.f, 10.f, 20.f, -1.f };

	// write to file AW_<name>.csv
	const char* nameX = fa_Name();
	if (IsBlank( nameX))
		nameX = "unk";
	FILE* f = fopen( strtprintf( "PM_%s.csv", nameX), "wt");
	if (!f)
		return RCBAD;		// can't open file

	AWIN awI;
	AWOUT awO;
	WStr hdgI = awI.aw_CSVHeading();
	WStr hdgO = awO.aw_CSVHeading();
	fprintf( f, "%s,%s\n", hdgI.c_str(), hdgO.c_str());

	int nL = fa_NL();
	double absSlr[ CFSMAXNL+1] = { 0.};
	for (int iTO=0; tO[ iTO]>0.f; iTO++)
	for (int iHO=0; hxO[ iHO]>0.f; iHO++)
	{
	  for (int iTI=0; tI[ iTI]>0.f; iTI++)
	  for (int iHI=0; hxI[ iHI]>0.f; iHI++)
	  for (int iS=0; incS[ iS]>=0.f; iS++)
	  for (int iA=0; absS[ iA]>=0.f; iA++)
	  {		if (iS==0 && iA>0)
				continue;
			if (iS>0 && iA==0)
				continue;
			VSet( absSlr, nL, double( absS[ iA]));
			awI.aw_Set( tO[ iTO], hxO[ iHO], tO[ iTO],
					tI[ iTI], hxI[ iHI], tI[ iTI],
					incS[ iS], absSlr);
			fa_Thermal( awI, awO);
			fprintf( f, "%s,%s\n",
				awI.aw_CSVRow().c_str(), awO.aw_CSVRow().c_str());
	  }
	}

	fclose( f);

	return RCOK;
}
#endif	// _DEBUG


///////////////////////////////////////////////////////////////////////////////
// class XASHWAT: ASHWAT interface
//   routes calls either to ASHWAT.DLL (FORTRAN implementation)
//   or staticly-linked ashwat.cpp
///////////////////////////////////////////////////////////////////////////////
XASHWAT ASHWAT(		// public ASHWAT object
#if !defined( ASHWAT_LINKDLL)
	"");
#elif defined( _DEBUG)
	"\\hbx\\run\\ASHWAT.DLL");
#elif 0
	"\\hbx\\run\\ASHWAT.DLL");
#else
	"ASHWAT.DLL");
#endif
//=============================================================================
XASHWAT::XASHWAT( const char* moduleName)		// c'tor
	: XMODULE( moduleName)
{
	xm_ClearPtrs();
}
//-----------------------------------------------------------------------------
XASHWAT::~XASHWAT()
{
}	// XASHWAT::~XASHWAT
//-----------------------------------------------------------------------------
/*virtual*/ void XASHWAT::xm_ClearPtrs()
{
#if defined( ASHWAT_LINKDLL)
	xw_pAWThermal = NULL;
	xw_pAWCheckFixCFSLayer = NULL;
#endif
}		// XASHWAT::xm_ClearPtrs
//-----------------------------------------------------------------------------
RC XASHWAT::xw_Setup()		// general initialization
// duplicate calls OK
{
static bool bSetup = false;
	if (bSetup)
		return RCOK;

	RC rc = RCOK;
#if defined( ASHWAT_LINKDLL)
	if (!xm_hModule)
	{	if (xm_LoadLibrary() == RCOK)
		{	xw_pAWThermal = (AWThermal *)xm_GetProcAddress( "ASHWAT_Thermal");
			xw_pAWCheckFixCFSLayer = (AWCheckFixCFSLayer *)xm_GetProcAddress( "CheckFixCFSLayer");
			xw_pAWClearCFSLayer = (AWClearCFSLayer *)xm_GetProcAddress( "ClearCFSLAYER");
			xw_pAWClearCFSGap = (AWClearCFSGap *)xm_GetProcAddress( "ClearCFSGAP");
			xw_pAWClearCFSFillGas = (AWClearCFSFillGas *)xm_GetProcAddress( "ClearCFSFillGas");
			xw_pAWClearCFS = (AWClearCFS *)xm_GetProcAddress( "ClearCFS");
			xw_pAWFinalizeCFS = (AWFinalizeCFS *)xm_GetProcAddress( "FinalizeCFS");
			xw_pAWOffNormalProperties = (AWOffNormalProperties *)xm_GetProcAddress( "ASHWAT_OffNormalProperties");
			xw_pAWSolar = (AWSolar *)xm_GetProcAddress( "ASHWAT_Solar");
			xw_pAWBuildGap = (AWBuildGap *)xm_GetProcAddress( "BuildGap");
			xw_pAWRatings = (AWRatings *)xm_GetProcAddress( "CFSRatings");
		}
	}
	RC rc = xm_RC;
#endif
#if defined( ASHWAT_USECPP)
	ASHWAT_Setup( MsgCallBackFunc, 0);
#endif
	if (!rc && xw_IsLibEmpty())
		rc = xw_BuildLib();		// build library of CFSLAYERs and CFSTYs
	bSetup = true;
	return rc;
}		// XASHWAT::xw_Setup
//-----------------------------------------------------------------------------
/*static*/ void XASHWAT::MsgCallBackFunc(
	AWMSGTY msgTy,
	const char* msg)
{

	warn( msg);

}		// XASHWAT::MsgCallBackFunc
//-----------------------------------------------------------------------------
RC XASHWAT::xw_CheckFixCFSLayer(
	CFSLAYER& L,		// layer to be checked / fixed
	const char* what)	// context for messages
{
#if defined( ASHWAT_USECPP)
	vector< string> msgs;
	bool bRet = L.cl_CheckFix( msgs, "Test");
	if (!bRet)
	{	vector< string>::const_iterator  pos;
		for (pos=msgs.begin(); pos!=msgs.end(); ++pos)
			warn( pos->c_str());	// trim beg and end -> TmpStr
	}
	return bRet ? RCOK : RCBAD;
#else
char msgs[ MSGMAXCOUNT][ MSGMAXLEN];
int nMsgs;
char fWhat[ MSGMAXLEN];

	if (!xw_pAWCheckFixCFSLayer)
		return RCBAD;

#if defined( ASHWAT_CPPTEST)
	CFSLAYER LX( L);
	if (LX.cl_IsNEQ( L, .0001, what))
		printf( "\nCFSLAYER copy c'tor mistmatch  LTYPE=%d", L.LTYPE);
	vector< string> msgsX;
	bool bRet = LX.cl_CheckFix( msgsX, "Test");
	if (!bRet)
		printf("\nBad cl_CheckFix");
#endif

	FCSET( fWhat, what);	// space pad

	int ret = (*xw_pAWCheckFixCFSLayer)( L, FCSL( msgs[ 0]), nMsgs, FCSL( fWhat));

	// display msg(s): crude error handling
	for (int iM=0; iM<nMsgs; iM++)
		warn( strTrim( NULL, FCSL( msgs[ iM])));	// trim beg and end -> TmpStr

#if defined( ASHWAT_CPPTEST)
	if (LX.cl_IsNEQ( L, .0001, what))
	{	printf( "CheckFixCFSLayer mistmatch  LTYPE=%d\n", L.LTYPE);
		LX.DbDump( "C++ ");
		L.DbDump( "DLL ");
	}
#endif

	return ret != 0 ? RCOK : RCBAD;
#endif

}		// XASHWAT::xw_CheckFixCFSLayer
//-----------------------------------------------------------------------------
RC XASHWAT::xw_ClearCFSLayer( CFSLAYER& L)
{
#if defined( ASHWAT_USECPP)
	L.cl_Clear();
#else
	if (!xw_pAWClearCFSLayer)
		return RCBAD;

	(*xw_pAWClearCFSLayer)( L);
#endif
	return RCOK;

}		// XASHWAT::xw_ClearCFSLayer
//-----------------------------------------------------------------------------
RC XASHWAT::xw_ClearCFSGap( CFSGAP& G)
{
#if defined( ASHWAT_USECPP)
	G.cg_Clear();
#else
	if (!xw_pAWClearCFSGap)
		return RCBAD;

	(*xw_pAWClearCFSGap)( G);
#endif
	return RCOK;
}		// XASHWAT::xw_ClearCFSGap
//-----------------------------------------------------------------------------
RC XASHWAT::xw_ClearCFSFillGas( CFSFILLGAS& F)
{
#if defined( ASHWAT_USECPP)
	F.cfg_Clear();
#else
	if (!xw_pAWClearCFSFillGas)
		return RCBAD;

	(*xw_pAWClearCFSFillGas)( F);
#endif
	return RCOK;

}		// XASHWAT::xw_ClearCFSFillGas
//-----------------------------------------------------------------------------
RC XASHWAT::xw_ClearCFS( CFSTY& CFS)
{
#if defined( ASHWAT_USECPP)
	CFS.cf_Clear();
#else
	if (!xw_pAWClearCFS)
		return RCBAD;

	(*xw_pAWClearCFS)( CFS);
#endif
	return RCOK;

}		// XASHWAT::xw_ClearCFS
//-----------------------------------------------------------------------------
RC XASHWAT::xw_FinalizeCFS( CFSTY& CFS)
{
#if defined( ASHWAT_USECPP)
	string msg;
	bool bRet = CFS.cf_Finalize( &msg);
	if (!bRet)
		warn( msg.c_str());
	return bRet ? RCOK : RCBAD;
#else
	char msg[ MSGMAXLEN];
	if (!xw_pAWFinalizeCFS)
		return RCBAD;

	int ret = (*xw_pAWFinalizeCFS)( CFS, FCSL( msg));
	if (ret)
		warn( strTrim( NULL, FCSL( msg)));	// trim beg + end -> TmpStr
	return ret==0 ? RCOK : RCBAD;
#endif
}		// XASHWAT::xw_FinalizeCFS
//-----------------------------------------------------------------------------
RC XASHWAT::xw_BuildGap(
	CFSGAP& G,
	const char* fgID,
	double tas,		// gap thickness, mm
	int gType)
{
const double PATM = 101325.;	// atmospheric pressure (Pa)

#if defined( ASHWAT_USECPP)
	bool bRet = G.cg_Build( fgID, tas, gType, 21., PATM);
	return bRet ? RCOK : RCBAD;
#else
	if (!xw_pAWBuildGap)
		return RCBAD;

	char FGID[ CFSIDLEN];
	FCSET( FGID, fgID);
	int ret = (*xw_pAWBuildGap)( G, FCSL( FGID), tas, gType, 21., PATM);
	return ret != 0 ? RCOK : RCBAD;
#endif
}		// XASHWAT::BuildGap
//-----------------------------------------------------------------------------
RC XASHWAT::xw_OffNormalProperties(			// ASHWAT: derive off-normal properties
	const CFSLAYER& L,	// layer for which to derive off-normal properties
						//   Used: LTYPE, SWP_EL, geometry
						//   Note: not altered (return is in LSWP_ON)
	double incA,		// solar beam angle of incidence, from normal, radians
						//   0 <= THETA <= PI/2
	double vProfA,		// solar beam vertical profile angle, +=above horizontal, radians
						//   = solar elevation angle for a vertical wall with
						//     wall-solar azimuth angle equal to zero
	double hProfA,		// solar beam horizontal profile angle, +=clockwise when viewed
						//   from above (radians)
						//   = wall-solar azimuth angle for a vertical wall
						//     Used for PD and vertical VB
	CFSSWP& LSWP_ON)	// returned: off-normal properties

{
#if defined( ASHWAT_USECPP)
	bool bRet = L.cl_OffNormalProperties( incA, vProfA, hProfA, LSWP_ON);
	return bRet ? RCOK : RCBAD;
#else
	if (!xw_pAWOffNormalProperties)
		return RCBAD;

	(*xw_pAWOffNormalProperties)( L, incA, vProfA, hProfA, LSWP_ON);
	return RCOK;
#endif
}		// XASHWAT::xw_OffNormalProperties
//-----------------------------------------------------------------------------
RC XASHWAT::xw_Solar(		// ASHWAT_Solar interface fcn
	int nL,				// # of layers
	CFSSWP* swpON,		// array of off-normal SW (solar) properties of layers
						//   [ 0]=outside; [ nL-1]=inside
	CFSSWP& swpRoom,	// SW (solar) properties of room pseudo-layer
						//   generally black or close to it
	double iBm,			// outside beam normal irradiance, power/area
	double iDf,			// outside diffuse irradiance, power/area
	double iDfIn,		// inside diffuse irradiance, power/area
	double* absL)		// returned: absorbed by layer [0 .. nL] (nL=room), power/area
						//   note: absL[ nL] = overall transmitted
{
#if defined( ASHWAT_USECPP)
	return RCBAD;		// should not be called
#else
	if (!xw_pAWSolar)
		return RCBAD;

	int ret = (*xw_pAWSolar)( nL, swpON, swpRoom, iBm, iDf, iDfIn, absL);

	return ret != 0 ? RCOK : RCBAD;
#endif
}		// XASHWAT::xw_Solar
//-----------------------------------------------------------------------------
bool XASHWAT::xw_IsLibEmpty() const
{	return xw_layerLib.size() == 0;
}		// XASHWAT::xw_IsLibEmpty
//-----------------------------------------------------------------------------
RC XASHWAT::xw_BuildLib()		// libary of built-in types
// overwrites any existing library entries
{
	RC rc = RCOK;
	xw_layerLib.clear();
	xw_CFSLib.clear();

	// properties direct paste from Ken Nittler 11-19-2010 memo
	//                         ID       D( ")  Tsol   1  Rsol 2   Tvis  1  Rvis 2   Tir   1 Emis 2   Keff
	xw_layerLib.push_back( CFSLAYER( "102",   0.120, .834, .075, .075, .899, .083, .083, .000, .840, .840, .578)); // CLEAR
	xw_layerLib.push_back( CFSLAYER( "9921",  0.118, .740, .119, .112, .842, .111, .106, .000, .164, .840, .578)); // EHSLE
	xw_layerLib.push_back( CFSLAYER( "2184F", 0.122, .694, .169, .146, .869, .065, .081, .000, .110, .840, .578)); // HSLE
	xw_layerLib.push_back( CFSLAYER( "2010",  0.087, .434, .341, .418, .798, .056, .044, .000, .840, .042, .578)); // MSLE
	xw_layerLib.push_back( CFSLAYER( "2026",  0.118, .379, .367, .467, .772, .072, .054, .000, .840, .037, .578)); // LSLE
	xw_layerLib.push_back( CFSLAYER( "2154",  0.117, .275, .429, .549, .713, .066, .044, .000, .840, .022, .578)); // ELSLE

	// exact match to prior 102 (below)
	xw_layerLib.push_back( CFSLAYER( "102X",  0.120, .830, .08,   .08, .899, .083, .083, .000, .840, .840, .578)); // CLEAR
	xw_layerLib.push_back( CFSLAYER( "InsScrn",  ltyINSCRN, .65f, .06f, .03f));

#if defined( ASHWAT_CPPTEST)
	// test layers (DLL vs C++ derived values are compared)
	CFSLAYER RB1( "RB1", ltyROLLB, 0.10f, .05f, .01f);
	CFSLAYER RB2( "RB2", ltyROLLB, 0.10f, 0.38f, 0.2f);
	CFSLAYER DM1( "DM1",  ltyDRAPE, 0.01f, 0.38f, 0.10f);
	CFSLAYER DX1( "DX1",  ltyDRAPE, 0.10f, 0.38f, 0.2f);
	CFSLAYER DX2( "DX2",  ltyDRAPE, 0.10f, 0.32f, 0.2f, .8f, 0.2f, -1.f, .05f);
	CFSLAYER DM2( "DM2",  ltyDRAPE, 0.01f, 0.38f, 0.10f);
	CFSLAYER IS2( "IS2",  ltyINSCRN, 0.55f, 0.08f, 0.06f);
#endif

// Openness = tauBB =  0.01
// Beam total transmittance = tauBT = 0.11
// Beam total reflectance     = rhoBT = 0.38
	xw_layerLib.push_back( CFSLAYER( "DrapeMed",  ltyDRAPE, 0.01f, 0.38f, 0.10f));

	xw_CFSLib.push_back( CFSTYX( "CLEAR_SINGLE", 1.042, 0.860, "102",  NULL));
	xw_CFSLib.push_back( CFSTYX( "CLEAR_AIR",    0.481, 0.763, "102",  "Air", .492, "102",   NULL));
	xw_CFSLib.push_back( CFSTYX( "EHSLE_AIR",    0.337, 0.721, "102",  "Air", .492, "9921",  NULL));
	xw_CFSLib.push_back( CFSTYX( "HSLE_AIR",     0.320, 0.686, "102",  "Air", .492, "2184F", NULL));
	xw_CFSLib.push_back( CFSTYX( "MSLE_AIR",     0.297, 0.419, "2010", "Air", .492, "102",   NULL));
	xw_CFSLib.push_back( CFSTYX( "LSLE_AIR",     0.295, 0.371, "2026", "Air", .492, "102",   NULL));
	xw_CFSLib.push_back( CFSTYX( "ELSLE_AIR",    0.289, 0.277, "2154", "Air", .492, "102",   NULL));

	return rc;

}	// XASHWAT::xw_BuildLib
//-----------------------------------------------------------------------------
const CFSLAYER* XASHWAT::xw_FindLibCFSLAYER( const char* id) const
{
	int nL = xw_layerLib.size();
	for (int iL=0; iL < nL; iL++)
	{	if (strMatch( id, FCGET( xw_layerLib[ iL].ID)))
			return &xw_layerLib[ iL];
	}
	return NULL;
}		// XASHWAT::xw_FindLibCFSLAYER
//-----------------------------------------------------------------------------
const CFSTYX* XASHWAT::xw_FindLibCFSTYX( const char* id) const
{
	int nT = xw_CFSLib.size();
	for (int iT=0; iT < nT; iT++)
	{	if (strMatch( id, FCGET( xw_CFSLib[ iT].ID)))
			return &xw_CFSLib[ iT];
	}
	return NULL;
}		// XASHWAT::xw_FindLibCFSTYX
//-----------------------------------------------------------------------------
const CFSTYX* XASHWAT::xw_FindLibCFSTYX(
	float SHGC,			// SHGC sought
	int nL) const		// # of glazing layers
{
	const CFSTYX* pClosest = NULL;
	float minDiff = 99999.f;
	int nT = xw_CFSLib.size();
	for (int iT=0; iT < nT; iT++)
	{	if (nL != xw_CFSLib[ iT].NL)
			continue;
		float diff = fabs( xw_CFSLib[ iT].SHGCcogNFRC - SHGC);
		if (diff < minDiff)
		{	minDiff = diff;
			pClosest = &xw_CFSLib[ iT];
		}
	}
	return pClosest;
}		// XASHWAT::xw_FindLibCFSTYX
//=============================================================================



// ashwface.cpp end