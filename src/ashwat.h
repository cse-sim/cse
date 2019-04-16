// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// ashwat.h -- defns and decls for ASHWAT complex fenestration models
///////////////////////////////////////////////////////////////////////////////

#if !defined( _ASHWAT_H)
#define _ASHWAT_H

#include <vector>
#include <string>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// public functions
///////////////////////////////////////////////////////////////////////////////
enum AWMSGTY { msgIGN, msgDBG, msgWRN, msgERR };
void ASHWAT_Setup(
	void* msgContext,
	void (*_pMsgCallBackFunc)( void* msgContext, AWMSGTY msgTy, const string& msg),
	int options=0);
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// until compatability with ASHWAT.DLL is abandoned,
//   structs and constants corresponding to FORTRAN TYPEs in ASHWAT CFSCalcMod.f90
//   *MUST* be maintained in parallel
///////////////////////////////////////////////////////////////////////////////
const int CFSIDLEN = 40;		// length of IDs

//=============================================================================
// Long-wave (aka LW or thermal) layer properties
//    Glazing and roller blind: standard diffuse properties
//    Blinds(VB): slat properties (beam-beam values not supported (no specular modeling)
//    Drape (PD): fabric properties @ openness = 0 (approx yarn properties)
// for all types, also used to represent equivalent flat layer properties
struct CFSLWP
{
	double EPSLF;		// thermal emittance, front (outside) side
	double EPSLB;		// thermal emittance, back (inside) side
	double TAUL;		// thermal transmittance
						//   (same value for radiation incident from front or back)
	CFSLWP();
	void clw_Clear();
	int clw_IsNEQ( const CFSLWP& lwp, double tol=0., int options=0,
		const char* w1="?", const char* w2="?") const;
	CFSLWP& clw_Reverse();
	string DbFmt( const char* tag="") const;
};	// struct CFSLWP

// Short wave (aka SW or solar) layer properties
//    Glazing: normal beam and diffuse
//    Roller blind (RB): normal beam and diffuse
//    Blinds (VB): slat material properties (specular refl/trans not modeled)
//    Drape (PD): flat (unpleated) fabric properties
// for all types, also use to represent equivalent flat layer properties
struct CFSSWP
{	double RHOSFBB;	// Solar reflectance, BEAM-BEAM, front (outside) side
					//   any angle of incidence
	double RHOSBBB;	// Solar reflectance, BEAM-BEAM, back (inside) side
					//	any angle of incidence
	double TAUSFBB;	// Solar transmittance, BEAM-BEAM, any angle of incidence
					//	radiation incident from front (outside)
	double TAUSBBB;	// Solar transmittance, BEAM-BEAM, any angle of incidence
					//	radiation incident from back (inside)
	double RHOSFBD;	// Solar reflectance, BEAM-DIFFUSE, front (outside) side
					//	BEAM-DIFFUSE, any angle of incidence
	double RHOSBBD;	// Solar reflectance, BEAM-DIFFUSE, back (inside) side
					//	any angle of incidence
	double TAUSFBD;	// Solar transmittance, BEAM-DIFFUSE, front (outside) side
					//	any angle of incidence
	double TAUSBBD;	// Solar transmittance, BEAM-DIFFUSE, any angle of incidence
	double RHOSFDD;	// Solar reflectance, DIFFUSE-DIFFUSE, front (outside) side
	double RHOSBDD;	// Solar reflectance, DIFFUSE-DIFFUSE, back (inside) side
	double TAUS_DD;	// Solar transmittance, DIFFUSE-DIFFUSE
					//	(same value for radiation incident from front or back)
	CFSSWP();
	void csw_Clear();
	void csw_ClearUnused( int lType);
	CFSSWP& csw_Reverse();
	int csw_IsNEQ( const CFSSWP& swp, double tol=0., int options=0,
		const char* w1="?", const char* w2="?") const;
	bool csw_SpecularOffNormal( double theta);
	void csw_SpecularAdjust( double RAT_1MR, double RAT_TAU);
	void csw_SpecularRATDiff( double& RAT_1MRDiff, double& RAT_TAUDiff);
	void csw_SpecularEstimateDiffuseProps();
	bool csw_FabricEstimateDiffuseProps();

	string DbFmt( const char* tag="") const;
};	// struct CFSSWP

//=============================================================================
// CFSLAYER: single glazing or shade layer
// layer types
const int ltyNONE = 0;		// unused / empty layer
const int ltyGLAZE = 1;		// glazing layer i.e, purely specular
const int ltySHADE1 = 2;	// 1st shade layer type
const int ltyDRAPE = 2;		// pleated drapes/curtains
const int ltyROLLB = 3;		// roller blind
const int ltyVBHOR = 4;		// venetian blinds - horizontal
const int ltyVBVER = 5;		// venetian blinds - vertical
const int ltyINSCRN = 6;	// insect screen
const int ltySHADEN = 6;	// last shade layer type
const int ltyROOM = 7;		// indoor space and/or make no adjustment
const int ltyGZS = 8;		// glazing with spectral data (read from aux file)
// shade control options
const int lscNONE = 0;		// no control
const int lscVBPROF = 1;	// VB slatA = ProfA (max gain)
const int lscVBNOBM = 2;	// VB slatA just exclude beam

struct CFSLAYER
{
	char ID[ CFSIDLEN];		// ID of layer  Caution: may not be null-terminated
	int LTYPE;				// layer type (see ltyXXX above)
	int iGZS;				// re spectral glazing
							//   = GZSTbl idx of LTYPE=ltyGZS (spectral glazing)
							//   else 0

							// material properties
							//  ltyGLAZE, ltyROLLB: as measured
							//  ltyGZS: derived from GSZ file data
								//  ltyVBxxx = slat properties (diffuse only)
	CFSSWP SWP_MAT;			//   short wave (solar)
	CFSLWP LWP_MAT;			//   long wave (thermal)

							// equivalent layer properties (see FinalizeCFSLAYER())
							//  = diff + direct-normal properties for pseudo flat layer
							//  ltyGLAZE, ltyGZS, ltyROLLB: same as _MAT
							//  ltyVBxxx: see VB_xxx()
	CFSSWP SWP_EL;			//   short wave (solar)
	CFSLWP LWP_EL;			//   long wave (thermal)

							// shade geometry
	double S;				// spacing
							//    VB: slat spacing, mm, >0
							//    PD: rectangular pleat spacing, mm >0
							//    IS: wire center-to-center spacing (pitch), mm, >0
							//    else unused
	double W;				// width
							//    VB: slat tip-to-tip (chord width), mm, >0
							//        if crown > 0, W < slat flattened width
							//    PD: pleat depth, mm >= 0
							//    IS: wire diameter, mm, >0, <S
							// crown
	double C;				//    VB: slat crown, mm >=0 if used
							//        crown assume upward for ltyVBHOR
							//    else unused
							// angle
	double PHI_DEG;			//    VB: slat angle, degrees (-90 <= PHI_DEG <= 90)
							//        ltyVBHOR: + = front-side slat tip below horizontal
							//        ltyVBVER: + = front-side slat tip is counter-
							//                     clockwise from normal (viewed from above)
							//    else unused
	int CNTRL;				// shade control method
							//    VB: lscNONE: PHI_DEG not changed
							//        lscVBPROF: PHI_DEG = profile angle (max gain)
							//	      lscVBNOBM: exclude beam (max visibility w/o beam)
							//			          PHI_DEG altered to just exclude beam
							//                    PHI_DEG = 20 if diffuse only
	CFSLAYER();
	CFSLAYER( const CFSLAYER& cl);
	CFSLAYER( const char* id, int ltype,
		float rhoFBB, float tauFBB, float tauBD=-1.f, float epsLF=-1.f, float rhoBBB=-1.f,
		float epsLB=-1.f, float tauL=0.f);
	CFSLAYER( const char* id, float thk, float tSol, float rSol1, float rSol2,
		float tVis, float rVis1, float rVis2, float tIR, float eIR1, float eIR2, float kEff);
	void cl_SetID( const char* _ID);
	void cl_Clear();
	void cl_ClearGeom();
	bool cl_Reverse( CFSLAYER& LR) const;
	int cl_IsNEQ( const CFSLAYER& cl, double tol=0., const char* what="?",
		int optionsMAT=1, int optionsEL=0) const;
	bool cl_Finalize();
	int cl_GZSInit();
	bool cl_IsGZS() const;
	bool cl_IsGlazeX() const;
	bool cl_IsControlledShade() const;
	bool cl_VBShadeControl(	double OMEGA_DEG);
	double cl_VBCriticalSlatAngle( double OMEGA_DEG) const;
	bool cl_DoShadeControl( double THETA, double OMEGA_V, double OMEGA_H);
	double cl_ConvectionFactor() const;
	bool cl_IsVB() const;
	bool cl_IsShadeLayer() const;
	bool cl_ISLWP( CFSLWP& LLWP) const;
	bool cl_ISSWP( CFSSWP& LSWP, double theta=-999.) const;
	bool cl_VBLWP( CFSLWP& LLWP) const;
	bool cl_VBSWP( CFSSWP& LSWP, double omega=-999.) const;
	bool cl_RBLWP( CFSLWP& LLWP) const;
	bool cl_RBSWP( CFSSWP& LSWP, double theta=-999.) const;
	bool cl_PDLWP( CFSLWP& LLWP) const;
	bool cl_PDSWP( CFSSWP& LSWP, double omegaV=-999.,double omegaH=-999.) const;
	void cl_FillDefaultsGeom( const CFSSWP& swp);
	void cl_FillDefaultsLWP( CFSLWP& lwp);
	void cl_FillDefaultsSWP( CFSSWP& swp);
	bool cl_CheckFixLWP( CFSLWP& lwp, vector< string> &msgs, const char* what);
    bool cl_CheckFixSWP( CFSSWP& swp, vector< string> &msgs, const char* what);
	bool cl_CheckFixGeom( vector< string> &msgs, const char* what);
	bool cl_CheckFix( vector< string> &msgs, const char* what);
	bool cl_CheckFix( const char* what);

	bool cl_OffNormalProperties( double THETA, double OMEGA_V, double OMEGA_H,
		CFSSWP& SWP_ON) const;
	int cl_OffNormalTest();
	int cl_OffNormalTest1( double incA, double omega_V, double omega_H);

	bool cl_InitGlaze( const char* id,
		float rhoFBB, float tauFBB, float epsLF, float rhoBBB=-1.f,
		float epsLB=-1.f, float tauL=0.f);
	bool cl_InitInscrn( const char* id,
		float fOpen, float rhoFBD, float tauBD, float rhoBBD=-1.f);
	bool cl_InitRollB( const char* id,
		float fOpen, float rhoFBD, float tauBD, float rhoBBD=-1.f);
	bool cl_InitDrape( const char* id,
		float fOpen, float rhoFBD, float tauBD, float rhoBBD=-1.f);
	void DbDump( const char* tag="") const;

};		// struct CFSLAYER

//=============================================================================
// CFSFILLGAS: fill gas
struct CFSFILLGAS
{	char ID[ CFSIDLEN];	// ID of gas (e.g. "Air")
	double AK, BK;		// conductivity coeffs: K = AK + BK * T
						//   AK (W/m-K)  BK (W/m-K2)
	double ACP, BCP;	// specific heat coeffs: CP = ACP + BCP * T
						//   ACP (J/kg-K)  BCP (J/kg-K2)
	double AVISC, BVISC;// viscosity coeffs: Visc = AVISC + BVISC * T
						//   AVISC (N-sec/m2)  BVISC (N-sec/m2-K)
	double MHAT;		// apparent molecular weight of gas

	CFSFILLGAS();
	CFSFILLGAS( const char* id, double ak, double bk, double acp, double bcp,
		double avisc, double bvisc, double mHat);
	void Init( const char* id, double ak, double bk, double acp, double bcp,
		double avisc, double bvisc, double mHat);
	void cfg_Clear();
	bool cfg_GetFillGas( const char* FGID);
	double cfg_Density( double P, double T) const;
	string DbFmt( const char* tag="") const;
};	// struct CFSFILLGAS

//=============================================================================
// CFSGAP: space between layers
// gap types
const int gtySEALED = 1;	// sealed
const int gtyOPENIn =  2;	// open to indoor air  (re Open Channel Flow (OCF))
const int gtyOPENOut = 3;	// open to outdoor air (re Open Channel Flow (OCF))

struct CFSGAP
{	int GTYPE;				// gap type (gtyXXX above)
	double TAS;				// actual surface-surface gap thickness, mm (always > 0)
							//   VB: minimum tip-surface distance (slats normal to CFS plane)
	double TAS_EFF;			// effective gap thickness, mm (always > 0)
							//   if either adjacent layer is VB
							//        adjusted re slat angle and convective behavior
							//        (see AdjustVBGap())
							//   else = TAS
	CFSFILLGAS FG;			// fill gas properties (see above)
	double RHOGAS;			// fill gas density (kg/m3)
	CFSGAP();
	// void cg_Init();
	void cg_Clear();
	bool cg_Build( const char* _FGID, double _TAS, int _GType,
		double _TMan=21., double _PMan=-1.);
	void cg_AdjustVBGap( const CFSLAYER& L);
	double cg_HConvGap( double T1, double T2);
	string DbFmt( const char* tag="") const;
};	// struct CFSGAP

//=============================================================================
// CFSTY: Complex Fenestration System
//   A collection of glazing and/or shading layers separated by gaps
// NOTE: C++ subscripts = 0 based, FORTRAN = 1-based (by default)
//       Use care!
const int CFSMAXNL = 6;	// max # of glaze or shade layers

struct CFSTY
{	char ID[ CFSIDLEN];			// ID (e.g. "Clr dbl")
	int NL;						// number of layers
	CFSLAYER _L[ CFSMAXNL];		// layer array, L[ 0] is outside layer
	CFSGAP _G[ CFSMAXNL-1];		// gap array, G[ 0] is outside-most, betw L[ 0] and L[ 1]

	CFSTY();
	void cf_SetID( const char* _ID);
	CFSLAYER& L( int iL) { return _L[ iL-1]; }
	const CFSLAYER& L( int iL) const { return _L[ iL-1]; }
	CFSGAP& G( int iG) { return _G[ iG-1]; }
	const CFSGAP& G( int iG) const { return _G[ iG-1]; }

	void cf_Clear();
	int cf_NGlz() const;
	int cf_HasShade() const;
	int cf_HasControlledShade() const;
	int cf_Info( int& nGlz, int& nShd, int& iLShd) const;
	bool cf_IsSealed() const;
	bool cf_Finalize( string* pMsg=NULL);
	int cf_Append( const CFSTY& FSX, bool bRev=false);
	double cf_EffectiveEPSLB() const;
	double cf_EffectiveEPSLF() const;
	bool cf_Thermal( double TIN, double TOUT, double HCIN, double HCOUT,
		double TRMIN, double TRMOUT, double ISOL, const double* SOURCE,
		double TOL, int IterControl, double* T, double* Q, double& QInConv,
		double& QInRad, double& UCG, double& SHGC, double& Cx, double& NConv,
		double& NRad, double& FHR_IN, double& FHR_OUT);
	bool cf_OffNormal( float incA, float vProfA, float hProfA, CFSSWP* swpON);
	bool cf_Solar( const CFSSWP* LSWP_ON, double IBEAM, double IDIFF, double ILIGHTS,
		double SOURCE[ CFSMAXNL+1]);
	// bool cf_CalcRatings( float& Urat, float& SHGCrat);
	bool cf_UFactor( double TOUT, double HCOUT,	double TIN,	double HCIN,
		double& U, double DTRMIN=0., double* TL=NULL);
	bool cf_UNFRC( double& UNFRC);
	bool cf_URated( double HT, double TO, double& HXO, double TI,
		double& HXI, double& U);
	bool cf_SHGCRated( double& SHGCrat);
	bool cf_Ratings( double& Urat, double& SHGCrat);
};	// struct CFSTY


#endif	// _ASHWAT_H

// ashwat.h end