// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// mspak.h -- Defn/decls for CSE mass manipulation and modeling
///////////////////////////////////////////////////////////////////////////////

const double MINMASSVHC = .00001;		// minimum volumetric heat capacity
										// construed to be not massless, Btu/F-ft3

// mass model values for MASSMODEL::mm_flags
//   msfNOBALx out of service as of 2-1-2012, keep for possible future use
const int msfNOBALP = 1;		// balance not expected due to adjustment of
								//   pre-calculated (interior) node
const int msfNOBALS = 2;		// ditto surface node
const int msfNOBAL = 3;			// ditto either
const int msfTEMPDEPENDENT = 4;	// at least 1 layer has temperature-dependent properties
								//   (only conductivity can be temp dependent as of 3-2012)

// Below grade surface info
//   ml_AddSoilLayer adds soil layer to user's construction.
//   Properties specified here
const float BGSoilCond = 1.f;
const float BGSoilDens = 115.f;
const float BGSoilSpHt = 0.2f;

///////////////////////////////////////////////////////////////////////////////
// struct MASSLAYER: homogeneous layer representation
// class AR_MASSLAYER: array thereof
///////////////////////////////////////////////////////////////////////////////
struct MASSLAYER
{
	int ml_iLSrc;		// idx of physical source layer
						//  re sublayer scheme used for FD model
    double ml_thick; 	// thickness of layer, ft
    double ml_vhc;		// volumetric heat capacity, Btu/F-ft3
    double ml_cond;  	// apparent conductivity of material, Btuh-ft/ft2-F
						//   (as of last call to ml_SetCond(), may include temp adjustment)
	double ml_condRtd;	// rated conductivity, Btuh-ft/ft2-F (at ml_condTRat)
	double ml_condTRat;	// temp for ml_condRtd, F
	double ml_condCT;	// conductivity temp coefficient, 1/F
						//   ml_cond = ml_condRtd*(1 + ml_condCT*(T - ml_condTRad))
	float ml_condNom;	// nominal conductivity, Btuh-ft/ft2-F (at 70 F)
	float ml_RNom;		// nominal resistance (at 70 F) = ml_thick / ml_condNom

	MASSLAYER() { ml_Init(); }
	MASSLAYER( const MASSLAYER& ml) { Copy( ml); }
	MASSLAYER( int iLSrc, double thick, double vhc, double cond, float condT=70.f, double condSlope=0.)
	{	ml_Set( iLSrc, thick, vhc, cond, condT, condSlope); }
	void ml_Init()
	{	memset( (char* )this, 0, sizeof( MASSLAYER));
		ml_iLSrc = -1;
	}
	double ml_HC() const		// heat cap (Btu/F-ft2)
	{	return ml_thick * ml_vhc; }
	double ml_CondT( float T) const	// conductance at T
	{	return MAT::mt_CondT( T, ml_condRtd, ml_condCT, ml_condTRat); }
	double ml_SetCond( float T)		// set conductance at temp T
	{	return ml_cond = ml_CondT( T); }
	void ml_CondAB( double& A, double& B) const
	{	// coefficients for cond = A + B*T
		MAT::mt_CondAB( ml_condRtd, ml_condCT, ml_condTRat, A, B);
	}
	double ml_C() const		// conductance (at last SetCond( T)), Btuh/ft2-F
	{	return ml_thick > 0. ? ml_cond / ml_thick : 0.; }
	double ml_R() const		// resistance (at last SetCond( T)), ft2-F/Btuh
	{	return ml_cond > 0. ? ml_thick / ml_cond : 1000000.; }
	double ml_RRtd() const		// rated resistance
	{	return ml_condRtd > 0. ? ml_thick / ml_condRtd : 1000000.; }
	int ml_IsMassless() const { return ml_vhc < MINMASSVHC; }
	int ml_IsInsulation() const		// insulation = high resistance and low density
	{	return ml_condNom < 0.1; }
	int ml_IsCondTempDependent() const { return ml_condCT != 0.; }

	void ml_Slice( int nSL);
	void ml_ChangeThickness( double thkF);
	MASSLAYER& ml_Set( int iLSrc, double thick, double vhc, double condRtd,
		float condTRat=70.f, double condCT=0.);
	void ml_SetNom();
	MASSLAYER& ml_Set( int iLSrc, const class LR* pLR, int options=0);
	MASSLAYER& Copy( const MASSLAYER& ml)
	{	memcpy( (char*)this, (const char*)&ml, sizeof( MASSLAYER));
		return *this;
	}
	MASSLAYER& operator =( const MASSLAYER& ml) { return Copy( ml); }
	int ml_IsMergeable( const MASSLAYER& ml) const;
	MASSLAYER& ml_Merge( const MASSLAYER& ml);
	int ml_SubLayerCount( float thkFactor) const;
	double ml_PenetrationDepth( float period) const;
};	// MASSLAYER
//=============================================================================
class AR_MASSLAYER : public WVect< MASSLAYER>
{
public:
	MASSLAYER* GetLayers() { return &front(); }
	double ml_GetProperties( double& cfactor, double& hc) const;
	float ml_GetPropertiesBG( int& interiorInsul) const;
	double ml_AdjustResistance( double RMax);
	void ml_AddSoilLayer( int iLSrc, float RSoil);
};		// class AR_MASSLAYER
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class MASSMODEL: base class for mass models
///////////////////////////////////////////////////////////////////////////////
// base class for mass
class MASSMODEL
{
public:
	MSRAT* mm_pMSRAT;		// parent pointer
	int mm_model;			// mass model (C_SFMODELCH_FD, )

	double mm_tSrf[ 2];		// latest surface temp, F
							//   [ 0] = inside, [ 1] = outside
	double mm_qSrf[ 2];		// latest surface heat flux, Btuh/ft2 (+ = into surface)
							//   [ 0] = inside, [ 1] = outside
	AR_MASSLAYER mm_layers;	// array of as-modeled layers (may differ
							//	 from physical layers)
	// Solar must pass thru rsurfo/i.  Conduction thru rsurf in series with surf film
    float mm_rsurfi;		// 0 or inside additional (opaque) surface resistance representing light
    						//	inner construction layer(s) omitted from mass layers by mt_FromLayers() (eg carpet)
    float mm_rsurfo;		// 0 or outside additional (opaque) surface resistance representing light
							//	outer construction layer(s) omitted from mass layers by mt_FromLayers().
	double mm_hcTot;		// total (unadjusted) heat capacity of all layers, Btu/ft2-F
							//   note: does not reflect adjusted HC used in MASSFD
	double mm_qIE;			// current internal energy, Btu/ft2
							//   (may be based on adjusted HCs)
	double mm_qIEDelta;		// last step change in internal energy, Btu/ft2
	double mm_qBal;			// energy balance for last calc, Btu/ft2, s/b 0
							//   = (internal energy change) - (surface flux)
	int mm_flags;			// flag bits, e.g. msfTEMPDEPENDENT
#if 0
x former MASSTYPE.mushlayer not used, revive if needed, 8-10-2010
x    MASSLAYER mushlayer;	// Characteristics of homogenized layer derived by combining all mass layers in MASSTYPE.
x							//	Attempt to improve on .heaviest (above) for better reporting for multi-layer masses.
x							//   Yields same values for single layer mass.
x							//   Set by mm_FromLayers.*/
#endif

	MASSMODEL( MSRAT* pMSRAT=NULL, int model=0)
		: mm_pMSRAT( pMSRAT), mm_model( model), mm_layers(), mm_rsurfi( 0.f), mm_rsurfo( 0.f),
		  mm_hcTot( 0.), mm_flags( 0), mm_qBal( 0.), mm_qIE( 0.), mm_qIEDelta( 0.)
	{	VZero( mm_tSrf, 2); VZero( mm_qSrf, 2); }
	MASSMODEL( const MASSMODEL& mm) { Copy( mm); }
	virtual ~MASSMODEL() { }
	virtual MASSMODEL& Copy( const MASSMODEL& mmSrc, int options=0);
	virtual MASSMODEL* Clone() const = 0;
	virtual BOOL mm_IsFD() const = 0;
	int mm_IsMassless() const { return mm_hcTot == 0.; }
	MSRAT* mm_GetMSRAT() { return mm_pMSRAT; }
	const MSRAT* mm_GetMSRAT() const { return mm_pMSRAT; }
	const SFI* mm_GetSFI() const;
	const char* Name() const { return mm_pMSRAT ? mm_pMSRAT->name : "?"; }
	void mm_SetMSRAT( MSRAT* pMSRAT) { mm_pMSRAT = pMSRAT; }
	int mm_IsNoBal() const { return (mm_flags & msfNOBAL) != 0; }

	// modeled layers
	RC mm_FromLayersFD( const AR_MASSLAYER& arML, int options=0);
	RC mm_AddLayerFD( const MASSLAYER& ml, RC* prc);
	RC mm_FromLayers( const AR_MASSLAYER& arML, int options=0);
	RC mm_AddLayer(	float thick, float vhc, float cond,	RC *prc);
	int mm_NLayer() const { return mm_layers.size(); }
	MASSLAYER* mm_GetLayer( int iL) { return &mm_layers[ iL]; }
	double mm_GetProperties( double& cFactor, double& hc) const
	{	return mm_layers.ml_GetProperties( cFactor, hc); }
	virtual void mm_FillBoundaryTemps( float tLrB[], int tLrBDim) const = 0;

	// model-independent access functions
	virtual void mm_RddInit( double t)
	{	mm_InitTemps( t);
		VSet( mm_tSrf, 2, t);
		VSet( mm_qSrf, 2, 0.);
	}
	virtual int mm_NNode() const = 0;
	virtual void mm_InitTemps( double t) = 0;
	virtual double mm_InternalEnergy() const = 0;
};	// class MASSMODEL
//--------------------------------------------------------------------------------------------
// central difference (aka "matrix") mass model
class MASSMATRIX : public MASSMODEL
{
public:
	WVect< double> mx_hc;		// array of node heat capacities
	WVect< double> mx_M;		// Mass matrix M: n*(n+2) elements.  See "The Mass Matrix Story" in mspak.cpp.
	WVect< double> mx_temp;		// array of current temperatures
	WVect< double> mx_tOld;		// array of prior temperatures

	MASSMATRIX( MSRAT* pMSRAT=NULL, int model=0);		// c'tor
	MASSMATRIX( const MASSMATRIX& mx);				// copy c'tor
	~MASSMATRIX() { }		// d'tor
	void mx_SetSize( int n);

	// virtual void mm_RddInit( double t); use base
	virtual BOOL mm_IsFD() const { return 0; }
	virtual MASSMATRIX& Copy( const MASSMODEL& mmSrc, int options=0);
	virtual MASSMATRIX* Clone() const;
	virtual int mm_NNode() const { return mx_hc.size(); }
	double* mx_Temp() { return &mx_temp.front(); }				// ptr to temp vector
	const double* mx_Temp() const { return &mx_temp.front(); }	// const ditto
	double* mx_TOld() { return &mx_tOld.front(); }				// ptr to prior temp vector
	const double* mx_TOld() const { return &mx_tOld.front(); }	// const ditto
	virtual void mm_InitTemps( double t);
	virtual double mm_InternalEnergy() const;
	virtual void mm_FillBoundaryTemps( float tLrB[], int tLrBDim) const;

	RC mx_Setup( float t, float hi, float ho);
	void mx_Step(  double qsurf1, double qsurf2);
};	// class MASSMATRIX
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class MASSFD: forward difference mass model
// class MASSNODE: single node therein
//////////////////////////////////////////////////////////////////////////////
class MASSFD;
struct BCX;
class MASSNODE
{
friend class MASSFD;
friend struct BCX;
	MASSFD* mn_pMFD;		// parent MASSFD
	double mn_thick;		// layer thickness, ft
	double mn_hc;			// heat capacity (may be 0), Btu/ft2-F
	double mn_hcEff;		// effective heat capacity, Btu/ft2-F
							//   = mn_hc except for low-mass nodes
							//     (increased re time constant)
	double mn_uhA;			// half-conductance at mn_t = 0, Btuh/F
	double mn_uhB;			// half-conductance slope
							//    mn_uh = mn_uhA + mn_uhB * mn_t
	double mn_uh;			// half conductance = layer conductance * 2, Btuh/F
	double mn_ubn;			// overall conductance i -> i+1, Btuh/F
							//   last node [nL-1] not reliably updated
	double mn_f[ 3];		// update factors, see TStep()
	double mn_t;			// node temperature, F (result of last update)
	int mn_tauErrCount;		// count of time constant errors found
							//   (re excessive msg skip)

	int mn_Init( MASSFD* pMFD, const MASSLAYER& ml);
	inline double mn_UhT( float T) const { return max( .0001, mn_uhA + mn_uhB*mn_t); }
	inline void mn_UpdateUh() { mn_uh = mn_UhT( mn_t); }
	inline int mn_IsCondTempDependent() const { return mn_uhB != 0.; }
	inline const char* mn_MassName() const;
	void mn_TStep( double* told)
	{	mn_t = told[ -1]*mn_f[ 0] + told[ 0]*mn_f[ 1] + told[ 1]*mn_f[ 2]; }
	void mn_TStepSrf( const BCX& bcx, double told, double ubnAdj, double toldAdj);
	inline double mn_DtOvrTau( double C)
	{	double hcMin = C * Top.tp_subhrDur;
		mn_hcEff = max( hcMin, mn_hc);
		return hcMin / mn_hcEff;
	}
	inline double mn_InternalEnergy() const { return mn_t * mn_hcEff; }
	void mn_SetMMFlags( int bit);
	double mn_Cond() const { return mn_thick * mn_uh / 2.; }
	void DbDump( int iL) const;
};	// class MASSNODE
//---------------------------------------------------------------------------------------
// struct BCX: conditions at mass surface
enum BCXTY { bcxTARQ, bcxTNODE, bcxTSURF };
struct BCX
{	BCXTY bcxTy;	// type of boundary
					//   bcxTARQ: ta/tr/qrAbs specified (default condition)
					//   bcxTNODE: temp of surface node is specified
					//   bcxTSURF: temp of surface is specified (not implemented, 9-2010)
	double tFixed;	// specified node or surface (future) temp, F
	double ta;		// adjacent air temp, F
	double hc;		// convective coefficient, Btuh/ft2-F
	double tr;		// adjacent radiant temp, F
	double hr;		// radiant coefficient, Btuh/ft2-F (linearized)
	double qrAbs;	// total absorbed radiation at surface, Btuh/ft2
					//   (short + long, all sources)
	double cc;		// overall convective coupling from ta to 1st node, Btuh/ft2-F
	double cr;		// overall radiant coupling from tr to 1st node, Btuh/ft2-F
	double cx;		// cross coupling
	double bd;		// hc+hr+uh = common term in several calcs
	double f;		// uh/(hc+hr+uh) = common term in several calcs
	double tsx;		// term in surface temp calc, F
	double qrNd;	// heat gain due to radiation at node adjacent to boundary, Btuh/ft2
	double tNd;		// temp at node adjacent to boundary, F
	double qSrf;	// heat flux at surface, Btuh/ft2 (+ = into surface)
	void Clear()
	{	bcxTy=bcxTARQ; tFixed=0.; ta=0.; hc=0.; tr=0.; hr=0.;
		qrAbs=0.; cc=0.; cr=0.; cx=0.; bd=0.; f=0.; tsx=0.;
		qrNd=0.; tNd=0.; qSrf=0.;
	}
	void SetDerived( const MASSNODE& nd);
	inline double CTQ() const { return ta*cc + tr*cr + qrNd; }
	inline double C() const { return cc + cr; }
#if defined( _DEBUG)
	// non-inline for debugging
	double TSrf() const;
	double QSrf( double tNd);
#else
	inline double TSrf() const { return tsx + tNd*f; }
	inline double QSrf(
		double tNdL)	// prior step node temp adjacent to surface, F
						//   (forward diff heat gain is derived from prior step)
		// returns heat flow at mass surface, Btuh/ft2 + = into surface
	{	return (qSrf = cc * (ta - tNdL) + cr * (tr - tNdL) + qrNd); }
#endif
	inline double QX() const { return qrAbs + ta*hc + tr*hr - (hr+hc)*TSrf(); }
	void ZoneAccum( double A, ZNR& z) const;
	void DbDumpZA( const char* tag, double A);
	void DbDump( const char* tag);
	void FromSBC( const SBC& sbc);
};		// struct BCX
//---------------------------------------------------------------------------
class MASSFD : public MASSMODEL
{
public:
	WVect< MASSNODE> mf_nd;	// array of node characteristics
	WVect< double> mf_tOld;	// array of prior-step temperatures: (1 per layer)

	MASSFD( MSRAT* pMSRAT=NULL, int model=0);		// c'tor
	MASSFD( const MASSFD& mx);	// copy c'tor
	~MASSFD() { }		// d'tor
	void mf_SetSize( int n);
	// virtual void mm_RddInit( double t); use base
	virtual BOOL mm_IsFD() const { return 1; }
	virtual MASSFD& Copy( const MASSMODEL& mmSrc, int options=0);
	virtual MASSFD* Clone() const;
	virtual int mm_NNode() const { return mf_nd.size(); }
	virtual void mm_InitTemps( double t);
	virtual double mm_InternalEnergy() const;
	virtual double mm_InternalEnergyDelta( double* tOld) const;
	virtual void mm_FillBoundaryTemps( float tLrB[], int tLrBDim) const;
	void mf_Step( BCX bcx[ 2]);
	RC mf_Setup();
	void mf_InitNodes( int doCondUpdate=0);
	void DbDump( BCX* bcx) const;
};	// class MASSFD
//=============================================================================

inline int MSRAT::ms_NLayer() const { return ms_pMM ? ms_pMM->mm_NLayer() : 0; }
inline const char* MASSNODE::mn_MassName() const { return mn_pMFD->Name(); }
inline void MASSNODE::mn_SetMMFlags( int bit) { mn_pMFD->mm_flags |= bit; }

RC msBegIvl( int ivl);
RC msEndIvl( int ivl);
float FC msrsurf( float h, float rsurf);



// end of mspak.h
