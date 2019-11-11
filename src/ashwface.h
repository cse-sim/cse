// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// ashwface.h -- defns and decls for ASHWAT interface
///////////////////////////////////////////////////////////////////////////////

#if !defined( _ASHWFACE_H)
#define _ASHWFACE_H

#include "ashwat.h"

// FORTRAN-to-C++ conversion aids
#define ASHWAT_USECPP	// define to use C++ implementation
						//   else ashwat.dll (FORTRAN)
#undef ASHWAT_LINKDLL	// define to set up linkage to ashwat.dll
						//   transition aid 9-2016
#define ASHWAT_NEWCALL	// define to use updated ASHWAT_THERMAL call
						//   (meaningful iff ASHWAT_LINKDLL)
#undef ASHWAT_CPPTEST	// enable DLL<->C++ comparison code
						//   usually does nothing if defined( ASHWAT_USECPP)
#undef ASHWAT_CPPTESTTHERMAL // enable DLL<->C++ comparison for cf_Thermal() only


#undef ASHWAT_REV2		// enable revised ASHWAT interface code, 5-2015
						//   revised scheme reuses cached results
						//   NOT successful (long run times)
#if defined( ASHWAT_REV2)
#include <unordered_map>
#endif

#include <vector>
#include <string>
using namespace std;


///////////////////////////////////////////////////////////////////////////////
// approx outside/inside combined coeff. under NFRC rating conditions, Btuh/ft2-F
//   based on results from ASHWAT calcs for MSLE glazing
const float RoutNFRC = .19f;
const float RinNFRC = .82f;
//=============================================================================

// CFSTYX: extended CFSTY for internal use
struct CFSTYX : public CFSTY
{
	float UcogNFRC;		// NFRC cog U (externally calulated by e.g. Windows6)
	float SHGCcogNFRC;	// NRFC cog SHGC (ditto)
	float UcogAW;		// ASHWAT cog U
	float SHGCcogAW;	// ASHWAT cog SHGC

	CFSTYX() { Clear(); }
	CFSTYX( const char* id, float _UcogNFRC, float _SHGCcogNFRC, const char* layer1ID, ...);
	void Clear();
	// wrappers for CFSTY mbrs that to facilitate C++ <-> DLL comparisons
	bool cfx_CalcRatings( float& Urat, float& SHGCrat);
	bool cfx_Solar( CFSSWP* swpON, float iBm, float iDf, float iDfIn,
		double absL[ CFSMAXNL+1]);
	bool cfx_OffNormal( float incA, float vProfA, float hProfA, CFSSWP* swpON);
};		// struct CFSTYX
//=============================================================================

#undef ASHWAT_STATS		// define to derive statistics about ASHWAT use
						// Improvement idea, not implemented 5-11
						//    set up to run 2 identical zones
						//      one ASHWAT every step
						//      one with calc skip algorithm
						//    then loads results could be compared
						//      in addition to speed figures of merit etc.
#if defined( ASHWAT_STATS)
// incomplete, 2011
class XSTATS
{
public:
public:
	XSTATS() { sx_Init(); }
	void sx_Init()
	{	sx_N=0; sx_sumAct=0.; sx_sumEst=0.; sx_sumDif=0.; sx_sumDif2=0.;}

private:
	int sx_N;
	double sx_sumAct;
	double sx_sumEst;
	double sx_sumDif;
	double sx_sumDif2;

public:
	void sx_Obs( double vAct, double vEst)
	{	sx_N++;
		sx_sumAct += vAct;
		sx_sumEst += vEst;
		double dif = vEst - vAct;
		sx_sumDif += dif;
		sx_sumDif2 += dif*dif;
	}
	double sx_FBE() const
	{	// fractional bias error
		return sx_sumAct != 0. ? sx_sumDif / sx_sumAct : 0.;
	}
	double sx_MBE() const
	{	// mean bias error
		return sx_N > 0 ? sx_sumDif / sx_N : 0.;
	}
	double sx_RMSE() const
	{	// RSME
		return sx_N > 0 ? sqrt( sx_sumDif2 / sx_N) : 0.;
	}
	double sx_CVRMSE() const
	{	// fractional RMSE (aka coefficient of variation of RMSE)
		return sx_sumAct != 0
			? sqrt( sx_N*sx_sumDif2) / sx_sumAct
			: 0.;
	}
	void DbPrintf( const char* hdg) const
	{	::DbPrintf("%s   MBE=%0.5f   RMSE=%0.5f   FBE=%0.5f   CVRMSE=%0.5f",
			hdg, sx_MBE(), sx_RMSE(), sx_FBE(), sx_CVRMSE());
	}
};
#endif // ASHWAT_STATS

///////////////////////////////////////////////////////////////////////////////
// class AWIN -- consolidated inputs for ASHWAT calculation
///////////////////////////////////////////////////////////////////////////////
class AWIN
{
friend class FENAW;
	double aw_txaO;
	double aw_hxaO;
	double aw_txrO;
	double aw_txaI;
	double aw_hxaI;
	double aw_txrI;
	double aw_incSlr;
	double aw_absSlr[ CFSMAXNL+1];

public:
	AWIN() {}
	AWIN( const AWIN& awI) { aw_Copy( awI); }
	AWIN& aw_Copy( const AWIN& awI)
	{	aw_txaO = awI.aw_txaO; aw_hxaO = awI.aw_hxaO; aw_txrO = awI.aw_txrO;
		aw_txaI = awI.aw_txaI; aw_hxaI = awI.aw_hxaI; aw_txrI = awI.aw_txrI,
		aw_incSlr = awI.aw_incSlr;
		VCopy( aw_absSlr, CFSMAXNL+1, awI.aw_absSlr);
		return *this;
	}
	void aw_Set(
		double _txaO, double _hxaO, double _txrO,
		double _txaI, double _hxaI,	double _txrI,
		double _incSlr, double _absSlr[])
	{	aw_txaO = _txaO; aw_hxaO = _hxaO; aw_txrO = _txrO;
		aw_txaI = _txaI; aw_hxaI = _hxaI; aw_txrI = _txrI,
		aw_incSlr = _incSlr;
		VCopy( aw_absSlr, CFSMAXNL+1, _absSlr);
	}
	bool operator==( const AWIN& awI) const;
	size_t aw_Hash() const;
	void aw_Round();
	WStr aw_CSVHeading( const char* tag=nullptr) const;
	WStr aw_CSVRow() const;

};		// class AWIN
//----------------------------------------------------------------------------
class AWINHASH		// access to hash function for map
{	public:
		size_t operator() (const AWIN& awI) const
		{	return awI.aw_Hash();
		}
};	// class AWINHASH

//=============================================================================
class AWOUT
{
friend class FENAW;
	double aw_TL[ CFSMAXNL];	// calculated layer temps, K
#if defined(ASHWAT_CPPTESTTHERMAL)
	double aw_TLX[ CFSMAXNL];	// alternative calculated layer temps, K
								//   re C++ conversion testing
#endif
	double aw_SHGCrt;		// SHGC (or 0 if not computed)
	double aw_Urt;			// U-factor, Btuh/ft2-F
	double aw_Cx;			// cross-coupling = implicit conductance between
							//   zone air temp and zone rad temp, W/m2-K
	double aw_FM;			//	radiant inward flowing fraction of incident solar
	double aw_FP;			//	convective inward flowing fraction of incident solar
	double aw_FHR_IN;
	double aw_FHR_OUT;
	double aw_cc;			// glazing effective conductance, airtemp-to-environment, Btuh/ft2-F
	double aw_cr;			// glazing effective conductance, radtemp-to-environment, Btuh/ft2-F

public:
	AWOUT() { aw_Init(); }

private:
	void aw_Init()
	{	memset( this, 0, sizeof( AWOUT));
	}
	WStr aw_CSVHeading( const char* tag=nullptr) const;
	WStr aw_CSVRow() const;
};		// class AWOUT

///////////////////////////////////////////////////////////////////////////////
// class FENAW -- XSURF substructure for ASHWAT glazings
//    contains info for setup and runtime ASHWAT
///////////////////////////////////////////////////////////////////////////////
class FENAW
{
public:
	XSURF* fa_pXS;				// parent pointer
	float fa_UNFRC;				// assembly target U-factor per NFRC heating conditions, Btuh/ft2-F
	float fa_SHGC;				// assembly target SHGC
	float fa_frmF;				// frame fraction
	float fa_frmArea;			// frame area, ft2
	float fa_frmUNFRC;			// frame U-factor with NFRC surface conductances, Btuh/ft2-F
	float fa_frmUC;				// frame surface-to-surface conductance, Btuh/ft2-F
	SBC fa_frmSbcI;				// frame inside (zone side) boundary conditions
	SBC fa_frmSbcO;				// frame outside boundary conditions
	CFSTYX fa_CFS;				// CFS for this fenestration: represents glazing and shades
	char fa_refDesc[ CFSIDLEN+30];	// fa_CFS.ID + " (SHGC=%.3f  U=%.3f)"
	float fa_mSolar;			// solar muliplier
	float fa_mCond;				// conduction multiplier
	float fa_dfAbsB;			// inside (back) diffuse absorptance
	float fa_dfRhoB;			// inside (back) diffuse reflectance
	float fa_dfTauB;			// inside (back) diffuse transmitance
	double fa_dfLAbsB[ CFSMAXNL+1];		// inside (back) diffuse absorptance by layer
										//    [0..NL-1] = abs CFS layers (0 = outside)
										//    [NL] = room abs (=inside reflectance)
	double fa_dfLAbsF[ CFSMAXNL+1];		// outside (front) diffuse absorptance by layer
										//    [0..NL-1] = abs CFS layers (0 = outside)
										//    [NL] = room abs (=outside diffuse transmittance)
	double fa_bmLAbsF[ 24][ CFSMAXNL+1];// outside (front) beam absorptance by hour by layer
										//    [0..NL-1] = abs CFS layers (0 = outside)
										//    [NL} = room abs (=outside beam transmittance)
	int fa_iterControl;					// iteration control, passed to ASHWAT_Thermal
										//    abs value = max # of iterations
										//    if <0, do NOT init layer temps (start with cur values)
						// saved results from last ASHWAT call
						//	re-used for multiple subhrs
#if defined( ASHWAT_REV2)
	std::unordered_map< AWIN, AWOUT, AWINHASH> fa_X;	// cache of prior results
#else
							// conditions at prior calc = last full ASHWAT calc (re change detection)
	double fa_incSlrPC;		// incident solar, W/m2
	float fa_txaiPC;		// indoor air temp, F
	float fa_hxaiPC;		// indoor convective coeff, Btuh/ft2-F
	float fa_txaoPC;		// outdoor air temp, F
#endif
	AWOUT fa_awO;

	int fa_nCall;			// # of calls to this ASHWAT (does not include skipped calls where
							//   effective area = 0 due to shade control)
	int fa_nCalc;			// # of full ASHWAT calculations


	FENAW( struct XSURF* pXS=NULL);
#if 0
	FENAW( const FENAW& f) { Copy( f); }
	FENAW& Copy( const FENAW& f);
#endif
	void fa_Init();
	const char* fa_Name() const;
	RC fa_Setup( int oc);
	RC fa_SetupBare( float dirtShadeF);
	RC fa_InsertLayer( int iLIns, const CFSLAYER* pL, const CFSGAP* pG=NULL);
    SBC& fa_SBC( int si) { return fa_pXS->xs_SBC( si); }
	int fa_NL() const { return fa_CFS.NL; }
	void DbDump() const;
	RC fa_DfSolar();
	RC fa_BmSolar( int iH, float incA=0.f, float vProfA=0.f, float hProfA=0.f);
	RC fa_CalcRatings();
	RC fa_FrameBC();
	RC fa_FrameQS();
	RC fa_FrameEndSubhr();
	RC fa_Subhr( float fActive, int bDoFrm);
	RC fa_EndSubhr( float fActive);
	RC fa_ThermalCache( const AWIN& awIn, AWOUT& awOut);
	RC fa_Thermal( const AWIN& awIn, AWOUT& awOut);
	RC fa_PerfMap();
	float fa_DfRhoB() const { return fa_dfRhoB; }
	float fa_DfTauB() const { return fa_dfTauB; }
	float fa_BmTauF( int iH) const { return fa_bmLAbsF[ iH][ fa_NL()]; }
	float fa_DfTauF() const { return fa_dfLAbsF[ fa_NL()]; }
	static float fa_UtoCNFRC( float UNFRC);
	static float fa_CtoUNFRC( float uC);
};		// class FENAW
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// class XASHWAT: interface to ASHWAT DLL (complex fenestration model)
///////////////////////////////////////////////////////////////////////////////
#define FCSL( s) s, sizeof( s)
#define FCSET( d, s) strSpacePad( FCSL( d), s)
#define FCGET( s) strTrim( NULL, FCSL( s))
#include "xmodule.h"
class XASHWAT : public XMODULE
{
friend class FENAW;
friend struct CFSTY;
#if defined( ASHWAT_LINKDLL)
#if defined( ASHWAT_NEWCALL)
// updated FORTRAN call
typedef int _stdcall AWThermal( const CFSTY& CFS, const double& TIN, const double& TOUT,
	const double& HCIN, const double& HCOUT, const double& TRMIN, const double& TRMOUT,
	const double& ISOL, const double* SOURCE, const double& tol, const int& IterControl,
	double* T, double* Q, double& QInConv, double& QInRad, double& UCG, double& SHGC,
	double& Cx, double& NConv, double& Nrad, double& FHR_IN, double& FHR_OUT);
#else
typedef int _stdcall AWThermal( const CFSTY& CFS, const double& TIN, const double& TOUT,
	const double& HCIN, const double& HCOUT, const double& TRMIN, const double& TRMOUT,
	const double& ISOL, const double* SOURCE, const double& tol, const int& IterControl,
	double* T, double* Q, double& UCG, double& SHGC, double& Cx, double& FM, double& FP,
	double& FHR_IN, double& FHR_OUT);
#endif

typedef int _stdcall AWCheckFixCFSLayer( CFSLAYER& L,
	char* msgs, int lMsg, int& nMsgs,
	const char* what, int lWhat);
typedef void _stdcall AWClearCFSLayer( CFSLAYER& L);
typedef void _stdcall AWClearCFSGap( CFSGAP& G);
typedef void _stdcall AWClearCFSFillGas( CFSFILLGAS& F);
typedef void _stdcall AWClearCFS( CFSTY& CFS);
typedef int _stdcall AWFinalizeCFS( CFSTY& CFS, char* mag, int lMsg);
typedef int _stdcall AWSolar( int& NL, CFSSWP* LSWP_ON, CFSSWP& SWP_ROOM,
	const double& IBEAM, const double& IDIFF, const double& ILIGHTS,
	double* source);
typedef void _stdcall AWOffNormalProperties( const CFSLAYER& L,
	const double& THETA, const double& OMEGA_V, const double& OMEGA_H,
	CFSSWP& LSWP_ON);
typedef int _stdcall AWBuildGap( CFSGAP& G, char* FGID, int lFGID, double& tas,
	int& GType, const double& xTMan, const double& xpMan);
typedef int _stdcall AWRatings( const CFSTY& CFS, double& Urat, double& SHGCrat);

private:
	// proc pointers
	AWThermal* xw_pAWThermal;
	AWCheckFixCFSLayer* xw_pAWCheckFixCFSLayer;
	AWClearCFSLayer* xw_pAWClearCFSLayer;
	AWClearCFSGap* xw_pAWClearCFSGap;
	AWClearCFSFillGas* xw_pAWClearCFSFillGas;
	AWClearCFS* xw_pAWClearCFS;
	AWFinalizeCFS* xw_pAWFinalizeCFS;
	AWSolar* xw_pAWSolar;
	AWOffNormalProperties* xw_pAWOffNormalProperties;
	AWBuildGap* xw_pAWBuildGap;
	AWRatings* xw_pAWRatings;
#endif

public:
	XASHWAT( const char* moduleName);
	~XASHWAT();
	virtual void xm_ClearPtrs();

	RC xw_Setup();
	static void MsgCallBackFunc( void* msgContext, AWMSGTY msgTy, const string& msg);
	bool xw_CheckFixCFSLayer( CFSLAYER& L, const char* what);
	RC xw_FinalizeCFS( CFSTY& CFS);
	RC xw_BuildGap( CFSGAP& G, const char* fgID, double tas, int gType=gtySEALED);
	RC xw_OffNormalProperties( 	const CFSLAYER& L, double incA, double vProfA,
		double hProfA, CFSSWP& LSWP_ON);
	RC xw_Solar( int nl, CFSSWP* swpON, CFSSWP& swpRoom,
		double iBm, double iDf, double iDfIn, double* source);

	WVect< CFSLAYER> xw_layerLib;		// collection of built-in CFS layers
	WVect<  struct CFSTYX> xw_CFSLib;	// ditto CFSs

	RC xw_BuildLib();
	bool xw_IsLibEmpty() const;
	const CFSLAYER* xw_FindLibCFSLAYER( const char* id) const;
	const CFSTYX* xw_FindLibCFSTYX( const char* id) const;
	const CFSTYX* xw_FindLibCFSTYX( float SHGC, int nL) const;
};		// class XASHWAT
//-----------------------------------------------------------------------------
extern XASHWAT ASHWAT;		// public XASHWAT object
//=============================================================================

#endif	// _ASHWFACE_H

// ashwface.h end