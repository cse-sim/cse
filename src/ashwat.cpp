// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==============================================================================
// ashwat.cpp -- Solar and thermal models for complex fenestration systems
//==============================================================================

//==============================================================================
// based on ASHRAE 1311-RP research project conducted by
//     John L. Wright and Nathan Kotey
//     University of Waterloo, Mechanical Engineering
//     Advanced Glazing System Laboratory
//	   and
//     Chip Barnaby, Wrightsoft corporation
//
// 1311-RP FORTRAN code re-implemented in C++ by Chip Barnaby, Sept 2016
//==============================================================================

#include "CNGLOB.H"

#include "ashwat.h"

// TODO
// * Unified air properties
// * Cache constant long wave derived values (beg of cf_Thermal)
// * CFSShadeMod.f90: finish arg rationalization / commenting
//   Is arg order consistent?
//   Are there still redundant args passed to sub-models (TAUFDD, TAUBDD)? 12-30-08
// * Profile angle: non-vert surfaces, horiz profile (see ProfAng(),solarmod.f90) 12-30-08
// * Shade control: input, correct handling
//   Note: shade control means diffuse properties can change by hour
// * If runtime "check temporary" option selected, several runtime msgs
//   are produced for ASHWAT_THERMAL CFSAbs() arg (at least).  Investigate. 12-29-08
// * Some fixed-size arrays remain.
// * Add thickness to CFSLayer glazing, improve off-normal/diffuse calcs?
// * Correctly handle BD for specular or disable
// * ReverseCFSLayer ... fix all types.
// * OCF: 1) why not allow OCF for positions other than innermost gap?
//        2) why not allow gap connected to outside air? 1-12-07
// * Review definition of slat crown.  Does model handle concave up?
//   What is proper definition for vertical blinds?  1-05-08
// pre-2007 TODOs, need review
// * handle blind overlap cases (VB_SOL46() at least)
// * review HBX, add new calls (GenerateEL etc)
// * use more vector operations
// * use direction cosines for solar pos and surface orientation?
// * check in VB_xxx for S>0, W>0?
// * straighten out U / D convention for VB slat properties

// DONE
// * Eliminate unnec HCOCF etc. args from CFSFenProp
// * Remove leftover HCOCFIn etc, ASHWAT_THERMAL
// * use of crown for VB curve?  Use of curvature R/W as curvature ratio?
//   Better might be W/R (0 - 2) or W/2R (0 - 1)?
// * slat properties: which are used? Then fix ClearCFSLayerUnused()
// * rename _REV subroutines
// * compare TestOffNormal diff and LW results to Nathan's output
// * integrate and test VB subroutines
// * parameterize dimensions in new fcns
// * comments etc in VB subroutines

// defines
#define FORTRAN_TRANSITION		// define to include code that aids
								//   FORTRAN->C++ transition / testing
#undef PD_PRINTCASE				// define to enable pleated drape debug
								//   printing
#undef isspace					// code here should use library isspace
								//   instead of locally #defined variants (as found in e.g. CSE)


// Constants (all static to avoid conflict with possible public variants)
static const double SMALL_ERROR = 0.000001;
static const double TKAdd = 273.15;		// degK = degC + TKAdd
static const double PI = 3.14159265358979;
static const double PIOVER2 = PI/2.;
static const double DTOR = PI / 180.;
static const double RTOD = 180. / PI;

// constant values from http://physics.nist.gov/cuu/Constants/index.html, 1-16-06
static const double GACC = 9.80665;		// acceleration of gravity (m/sec2)
static const double SIG = 5.6704E-8;	// Stephan-Boltzman constant (W/m2-K4)
								//   http://physics.nist.gov/cuu/Constants/index.html
static const double R_UNIV=8.314472;	// Universal gas constant (J/mol-K)
static const double PATM = 101325.;		// Atmospheric pressure (Pa)
static const double HOC_ASHRAE = 16.9;	// nominal ASHRAE exterior convective
										//   coefficient, W/m2-K
										//   7.5 mph cooling conditions

///////////////////////////////////////////////////////////////////////////////
static int awOptions = 0;		// ASHWAT global options (none defined, 9-2016)
static void* awMsgContext = NULL;
static void (*pMsgCallBackFunc)( void* msgContext, AWMSGTY msgTy, const string& msg) = NULL;
//-----------------------------------------------------------------------------
void ASHWAT_Setup(		// setup message reporting linkage
	void* _msgContext,		// caller's context passed to MsgCallBackFunc
	void (*_pMsgCallBackFunc)( void* msgContext, AWMSGTY msgTy, const string& msg),
	int options /*=0*/)		// options TBD
// duplicate calls harmless
{
	awMsgContext = _msgContext;
	pMsgCallBackFunc = _pMsgCallBackFunc;
	awOptions = options;
}		// ASHWAT_Setup
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// utility functions
///////////////////////////////////////////////////////////////////////////////
template< typename T> inline static void vswap( T& v1, T& v2) { T vt=v1; v1=v2; v2=vt; }
template< typename T> inline static T Square(T v) { return v*v; }
template< typename T> inline static T Cube(T v) { return v*v*v; }
template< typename T> inline static T SqrtSumSq( T a, T b) { return sqrt( a*a+b*b); }
template< typename T> inline static T SqrtDifSq( T a, T b) { return sqrt( a*a-b*b); }
template< typename T> inline static T RadTX(T t1, T t2)	// helper re radiant coefficient
{	return SIG*(t1*t1 + t2*t2)*(t1+t2); }
template< typename T> inline static T RadPwr(T t, T eps)	// radiant emitted power
{	T t2=t*t; return SIG*eps*t2*t2; }

// macro for combining vNEQ results
//   debug: accum errors, all compares called
//   non-debug: OR errors -> early out on first fail
#if defined( _DEBUG)
#define _X_ +
#else
#define _X_ ||
#endif
//-----------------------------------------------------------------------------
template< typename T> static int vNEQ(	// compare, 0 iff "equal"
	T v1, T v2,		// values
	T tol=0,		// fractional diff
					//   if 0, require exact equality
	T eps=1.e-10)	// absolute diff
// returns 0 iff v1 "equals" v2
{	if (tol==0)
		return v1!=v2;		// 0 iff exactly equal
	T dif = abs( v1-v2);
	T fDif = 2*dif / (abs( v1)+abs(v2));
	return dif > eps && fDif > tol;
}		// vNEQ
///////////////////////////////////////////////////////////////////////////////
// non-heap arrays w/ optional arbitrary subscript range
// supports FORTRAN subscript notation
// ** No runtime subscript checking **
const int AMAXDIM = 20;		// maximum dimension
							// must be > CFSMAXNL + 2
//-----------------------------------------------------------------------------
template < int _i0> class A1D	// non-heap 1D array
{	double D[ AMAXDIM];		// data
	double* pD0;			// virtual origin of data
	int dN;					// active dimension
public:
	A1D( int _dN=AMAXDIM) : dN( _dN) { pD0 = D - _i0; }
	inline void Zero() { memset( D, 0, AMAXDIM*sizeof( double)); }
	inline double& operator()( int i) { return *(pD0 + i); }
	inline double operator()( int i) const { return *(pD0 + i); }
	double operator=( double v) { for (int i=0;i<AMAXDIM;i++) D[ i]=v; return v; }
	inline int Dim() const { return dN; }
};	// class A1D
//-----------------------------------------------------------------------------
// non-heap 2-D array with templated max size
template< int rAl, int cAl> class A2D
{	double* pR[ rAl];		// row pointers
	double D[ rAl*cAl];		// data
	int rN;					// active rows
	int cN;					// active cols
	int r0;					// row initial subscript
	int c0;					// col initial subscript

public:
	A2D( int _rN, int _cN, int _r0=0, int _c0=0)
		: rN( _rN), cN( _cN), r0( _r0), c0( _c0)
	{	Setup(); }
	A2D( const A2D< rAl,cAl>& S)
	{	Copy( S); }
	void Setup()
	{	for (int i=0; i<rN; i++)
			pR[ i] = D + i*cAl;
	}
	void Resize( int _rN, int _cN, int _r0=0, int _c0=0)
	{	rN=_rN; cN=_cN; r0=_r0; c0=_c0; }
	double& M( int r, int c) { return *(PRow( r)+c); }
	double M( int r, int c) const { return *(PRow( r)+c); }
	double& operator()(int r, int c) { return M( r, c); }
	void Zero() { memset( D, 0, rAl*cAl*sizeof( double)); }
	void Copy( const A2D< rAl,cAl>& S)
	{	memcpy( this, &S, sizeof( S)); }
	double operator=( double v) { for (int i=0;i<rAl*cAl;i++) D[ i]=v; return v; }
	void operator=( const A2D< rAl,cAl>& S) { Copy( S); }
	int NRows() const { return rN; }
	int NCols() const { return cN; }
	double* PRow( int r) const { return pR[ r-r0]-c0; }
	double SumRow( int r, int _c1=INT_MAX, int _cN=INT_MAX)
	{	if (_c1==INT_MAX) _c1 = c0;
		if (_cN==INT_MAX) _cN = cN;
		double sum=0.;
		const double* p = PRow( r);
		for (int c=_c1; c<=_cN; c++)
			sum += *(p+c);
		return sum;
	}
	void SwapRows( int r1, int r2)
	{	if (r1 != r2)
			vswap( pR[ r1-r0], pR[ r2-r0]);
	}
int Solve(			// matrix solver
	A1D< 1>& XSOL)	// returned: solution vector, min req dimension: XSOL( rN)
// Solves matrix by the elimination method supplemented by a
//     search for the largest pivotal element at each stage
{
	int ret = 0;
	int N = NRows();
	int NM1=N-1;
	int NP1=N+1;
	int NP2=N+2;

	for (int I=1; I<=N; I++)
		M( I, NP2)=SumRow( I, 1, NP1);

	for (int L=1; L<=NM1; L++)
	{	double CMAX = M( L, L);
		int LP = L+1;
		int NOS = L;
		for (int I=LP; I<=N; I++)
		{	if (abs( M( I,L)) > abs( CMAX))
			{	CMAX = M( I, L);
				NOS = I;
			}
		}
		SwapRows( L, NOS);
		for (int I=LP; I<=N; I++)
		{	double Y = M( I, L) / M( L,L);
			for (int J=L; J<=NP2; J++)
				M( I, J) -= Y * M( L, J);
		}
	}

	//  back-substitute
	XSOL( N) = M( N, NP1) / M( N, N);
	for (int I=1; I<=NM1; I++)
	{	int NI = N - I;
		double tD = 0.;
		for (int J=1; J<=I; J++)
		{	int NJ = N+1-J;
			tD += M( NI, NJ)*XSOL( NJ);
		}
		XSOL( NI) = (M( NI, NP1) - tD) / M( NI, NI);
	}
	return ret;

}  // A2D::Solve
};	// class A2D
//-----------------------------------------------------------------------------
static string awStrFromBF(		// convert blank-filled to string
	const char* s,
	size_t sDim)
{
	int iS = sDim-1;
	while (iS>=0 && isspace( unsigned(*(s+iS))))
		iS--;
	std::string t( s, iS+1);
	return t;
}	// awStrFromBF
//-----------------------------------------------------------------------------
static const char* awStrToBF(		// c-string to space-padded string
	char* d,		// destination
	size_t dDim,	// size of destination
	const char* s)	// source (null terminatinated)
{
	while (*s && isspace( unsigned( *s)))
		s++;
	size_t iD = 0;
	while (*s && iD<dDim)
		*(d+iD++) = *s++;
	while (iD<dDim)
		*(d+iD++) = ' ';
	return d;
}	// awStrToBF
//-----------------------------------------------------------------------------
#if 0
// unused, save for possible future use
static const char* awStrToZF(		// safe c-string copy with truncate
	char* d,		// destination
	size_t dDim,	// size of destination
	const char* s)	// source (null terminatinated)
{
	while (*s && isspace( unsigned( *s)))
		s++;
	size_t iD = 0;
	while (*s && iD<dDim-1)
		*(d+iD++) = *s++;
	while (iD<dDim)
		*(d+iD++) = '\0';
	return d;
}	// awStrToZF
#endif
//-----------------------------------------------------------------------------
#if defined( FORTRAN_TRANSITION)
#define FCSET( d, s) awStrToBF( d, sizeof( d), s)
#define FCGET( s) awStrFromBF( s, sizeof( s)).c_str()
#else
#define FCSET( d, s) awStrToZF( d, sizeof( d))
#define FCGET( s) s
#endif		// FORTRAN_TRANSITION
//-----------------------------------------------------------------------------
static string stringFmtV( const char* fmt, va_list ap=NULL)
{
	static const int maxLen = 2000;
	char buf[ maxLen];
	if (ap)
	{	int fRet = vsprintf_s( buf, maxLen, fmt, ap);
		fmt = fRet >= 0 ? buf : "?? stringFmtV vsprintf_s failure.";
	}
	return fmt;
}		// ::stringFmtV
//-----------------------------------------------------------------------------
static string stringFmt( const char* fmt, ...)
{	va_list ap; va_start( ap, fmt);
	return stringFmtV( fmt, ap);
}		// ::stringFmt
//-----------------------------------------------------------------------------
static bool MessageV(
	AWMSGTY msgTy,		// message type
	const char* fmt,
	va_list ap=NULL)
{
	string msg = stringFmtV( fmt, ap);
	if (pMsgCallBackFunc)
		(*pMsgCallBackFunc)( awMsgContext, msgTy, msg);	// transmit message to caller
	else
	{	printf( msg.c_str());		// no message callback defined, use printf
		printf("\n");
	}
	return msgTy >= msgWRN;	// TODO
}	// ::MessageV
//------------------------------------------------------------------------------
static bool Message(
	AWMSGTY msgTy,
	const char* fmt,
	...)
{
	va_list ap;
	va_start( ap, fmt);
	return MessageV( msgTy, fmt, ap);
}	// ::Message
//------------------------------------------------------------------------------
static bool AddMsgV(		// add message to array of messages
	vector< string> &msgs,		// array of messages
	const char* what,			// context for message
	const char* fmt,			// message (vsprintf'd if !ap)
	va_list ap=NULL)			// optional args for fmt
// add message to array
// returns false (handy in some contexts)
{
	string msgText = stringFmtV( fmt, ap);
	string msg = string( what) + ": " + msgText;
	msgs.push_back( msg);
	return false;
}  // AddMsgV
//------------------------------------------------------------------------------
static bool AddMsg(			// add message to array of messages
	vector< string> &msgs,		// array of messages
	const char* what,			// context for message
	const char* fmt,
	...)
{
	va_list ap;
	va_start( ap, fmt);
	return AddMsgV( msgs, what, fmt, ap);
}	// AddMsg
//------------------------------------------------------------------------------
bool AppendMsg(		// append message with break;
	string comboMsg,		// combined string
	const string msg,		// message to append
	const string brk)		// break (e.g. "; ")
// append messgage
// returns false (handy in some contexts)
{
	if (msg.length() > 0)
	{   if (comboMsg.length() > 0)
			comboMsg += brk + msg;
		else
			comboMsg = msg;
	}
	return false;
}  // AppendMsg
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
static bool CompMsg(		// message re compare error (double)
	const char* w1,		// context 1
	const char* w2,		// context 2
	const char* item,	// specific item
	double v1,			// val 1
	double v2,			// val 2
	bool doHdg)			// generate object heading iff true
// returns false
{
	if (doHdg)
		Message( msgDBG, "\n%s %s: compare mismatch:", w1, w2);
	Message( msgDBG, "   %s  %0.5f   %0.5f", item, v1, v2);
	return false;
}	// CompMsg
//-----------------------------------------------------------------------------
static bool CompMsg(		// message re compare error (int)
	const char* w1,		// context 1
	const char* w2,		// context 2
	const char* item,	// specific item
	int v1,			// val 1
	int v2,			// val 2
	bool doHdg)			// generate object heading iff true
// returns false
{
	if (doHdg)
		Message( msgDBG, "\n%s %s: compare mismatch:", w1, w2);
	Message( msgDBG, "   %s  %d   %d", item, v1, v2);
	return false;
}	// CompMsg
//-----------------------------------------------------------------------------
template <typename T> static int vNEQMsg(			// compare, issue message if fail
	T v1, T v2,				// values
	T tol,					// fractional diff
							//   if 0, require exact equality
	const char* w1,			// context 1
	const char* w2,			// context 2
	const char* item,		// specific item (e.g. mbr name)
	int& errCount)			// error count (icr'd)
// returns 0 iff v1 "equals" v2
{
	int ret = vNEQ( v1, v2, tol);
	if (ret)
		CompMsg( w1, w2, item, v1, v2, errCount++==0);
	return ret;
}	// vNEQMsg
#endif
//=============================================================================
static double P01(		//  constrain property to range 0 - 1
	double P,			//  property
	[[maybe_unused]] const char* what)	//  identifier for err msg
{
#if defined( _DEBUG)
	if (P < -.05 || P > 1.05)
		Message( msgWRN, "(%s%s out of range (%0.3f)",
			what,
			P<-.2 || P>1.2 ? " very" : "",
			P);
#endif
	return max( 0., min( 1., P));
} // P01
//------------------------------------------------------------------------------
static bool CheckFixPropRange(		// check property value
	double &p, const char* pName,	// property
	vector< string> &msgs,	// msg array
	const char* what,		// context for error msg
	double pMin=0.,			// allowed min
	double pMax=1.)			// allowed max

//  error <= msgTol: fix, return true
//  error > msgTol: msg, fix, return false
{
	bool bRet = true;
	if (p < pMin || p > pMax)
	{	const double msgTol = .0001;
		if (p < (pMin-msgTol) || p > (pMax+msgTol))
			bRet = AddMsg(msgs, what, "%s must be in range %0.2f - %0.2f",
						pName, pMin, pMax);
		p = max( pMin, min( pMax, p));
	}
	return bRet;
}  // CheckFixPropRange
//------------------------------------------------------------------------------
static bool CheckFixPropPair(
	int opt,				// what to check: 1: p1==p2, 2: p1+p2 <= 1
	double& p1, const char* p1Name,	// property 1 (returned possibly updated)
	double& p2, const char* p2Name,	// property 2 (returned possibly updated)
	vector< string> &msgs,			// msg array (new msg(s) added)
	const char* what)				// context for error msgs
// check/fix property pair per opt
// returns false iff error and fix
{
	bool L1 = CheckFixPropRange( p1, p1Name, msgs, what);
	bool L2 = CheckFixPropRange( p2, p2Name, msgs, what);
	bool bRet = L1 && L2;
	if (opt == 1)
	{	// check equal
		double dif = abs( p1 - p2);
		if (dif > .001)
			bRet = AddMsg( msgs, what, "%s and %s must be equal", p1Name, p2Name);
		p1 = p2;
	}
	else if (opt == 2)
	{	// check p1 + p2 <= 1
		if (p1 + p2 > 1.)
		{	if (p1 + p2 > 1.0001)
				bRet = AddMsg( msgs, what, "%s + %s must be <= 1.", p1Name, p2Name);
			p1 = 1.-p2;		// apply fix to p1 (0 <= p2 <= 1, so fixed p1 is valid)
		}
	}
	return bRet;
}  // CheckFixPropPair
//------------------------------------------------------------------------------
static void FixProp2Pair(	// fix paired values to match corrected sum
	double &pa,			// value 1
	double &pb,			// value 2
	double pSumFix)		// corrected value for pa+pb
// returns pa and pb modified such that pa+pb = pSumFix
{
	double pSum = pa + pb;
	if (pSum != 0.)
	{	pa *= pSumFix / pSum;
		pb *= pSumFix / pSum;
	}
	else
	{	pa = pSumFix / 2.;
		pb = pSumFix / 2.;
	}
}    // FixProp2Pair
//------------------------------------------------------------------------------
static bool CheckFixProp2Pair(	// check/fix linked property pairs per opt
	int opt,  // what to check: 1: p1==p2, 2: p1+p2 <= 1
	double &p1a, double &p1b, const char* p1Name,
	double &p2a, double &p2b, const char* p2Name,
	vector< string> &msgs,	// message array
	const char* what)		// context for error msgs
// returns false iff value error / fix
{
	double p1x = p1a + p1b;
	double p2x = p2a + p2b;
	bool bRet = CheckFixPropPair( opt, p1x, p1Name, p2x, p2Name, msgs, what);
	if (!bRet)
	{	FixProp2Pair( p1a, p1b, p1x);
		FixProp2Pair( p2a, p2b, p2x);
	}
	return bRet;
}  // CheckFixProp2Pair
//-----------------------------------------------------------------------------
// HEMINT integrand function parameters
//   used by xx_F() functions

const int hipDFLT = 0;		// return default value
const int hipRHO = 1;		// return reflectance
const int hipTAU = 2;		// return transmittance

struct HEMINTP		// HEMINT parameters
{	double RHO_BT0;		// normal beam-total reflectance
	double TAU_BT0;		// normal beam-total transmittance
	double TAU_BB0;		// normal beam-baem transmittance
	HEMINTP( double rho_bt0, double tau_bt0, double tau_bb0)
		: RHO_BT0( rho_bt0), TAU_BT0( tau_bt0), TAU_BB0( tau_bb0) {}
};	// struct HEMINTP
//------------------------------------------------------------------------------
static double HEMINT(	// hemisphere integration
	double (*F)( double theta, const HEMINTP& P, int opt),	// property integrand function
	const HEMINTP& F_P,		// parameters passed to F()
	int F_opt=hipDFLT)		// options passed to F()
// Romberg integration of property function over hemispehere
{
static const int KMAX = 8;		//  max steps
static const int NPANMAX = int( pow( 2, KMAX));
static const double TOL = .0005;		//  convergence tolerance
double T[ KMAX][ KMAX];
// double X, DX, SUM, DIFF
// INTEGER nPan, I, K, L, iPX

	double X1 = 0.;			//  integration limits
	double X2 = PIOVER2;
	int nPan=1;
	double SUM = 0.;
	int K;
	for (K=0; K < KMAX; K++)
	{	double DX = (X2-X1) / nPan;
		int iPX = NPANMAX / nPan;
		for (int I = 0; I <= nPan; I++)
		{
			if (K == 0 || (I*iPX)%(iPX*2) != 0)
			{	//  evaluate ingegrand function for new X values
				//    2 * sin( x) * cos( x) covers hemisphere with single intregral
				double X = X1 + I*DX;
				double FX  = 2. * sin( X) * cos( X) * (*F)( X, F_P, F_opt);
				if (K == 0)
					FX /= 2.;
				SUM += FX;
			}
		}
		T[ K][ 0]= DX * SUM;

		//  trapezoid result - i.e., first column Romberg entry
		//  Now complete the row
		if (K > 0)
		{	int p4 = 4;
			for (int L=1; L<=K; L++)
			{	T[ K][ L] = (p4*T[ K][ L-1] - T[ K-1][ L-1]) / (p4-1.);
				p4 *= 4;
			}
			//  check for convergence
			//     do 8 panels minimum, else can miss F() features
			if ( nPan >= 8)
			{	double DIFF = abs( T[ K][ K] - T[ K-1][ K-1]);
				if (DIFF < TOL)
					break;
			}
		}
		nPan *= 2;
	}
	if (K == KMAX)
	{	Message( msgERR, "HEMINT: convergence failure");
		K = KMAX-1;
	}
	return P01( T[ K][ K], "HEMINT");

 } // HEMINT
//------------------------------------------------------------------------------
#if defined( _DEBUG)
double TEST_F(	//  test function
	double x,			//  ind var
	int /*OPT*/,		//  options (unused)
	double P[])	//  parameters
{
	return x*x*P[ 2] + x*P[ 1] + P[ 0];
} // TEST_F
#endif

//------------------------------------------------------------------------------
static double HIC_ASHRAE(		// inside surface convective coeff
	double HT,		// glazing height, m
	double TG,		// glazing inside surf temp, C or K
	double TI)		// inside air temp, C or K
// returns hic, W/m2-K, per footnote on Table 2, p. 31.6 (Fenestration) HOF 2005
{
	double hic = 1.46 * pow( abs( TG-TI) / max( HT, .001), 0.25);
	return hic;
}  // HIC_ASHRAE
//-------------------------------------------------------------------------------
static void AirProps(			// some air propeties
	double Ts,		// shade temp, K
	double Tg,		// glass temp, K
	double b,		// gap space, m
	double& k,		// returned: conductivity, W/mK
	double& RAbsG)	// returned: ?
{
	double Tavg=(Ts+Tg)/2;		// T for properties calculations

	// properties of AIR
	double rho=PATM/(287.097*Tavg);     // density (kg/m3) <- temperature in (K)
	double beta=1./Tavg;              // thermal expansion coef(/K)
	double dvisc = (18.05 + ((Tavg-290.)/10.0)  * (18.53-18.05)) * 1.0e-6;
					   //  dynamic viscosity (kg/m.sec) or (N.sec/m2)
	double Cp= 1044.66-0.31597*Tavg+0.000707908*Square( Tavg)  -0.00000027034*Cube( Tavg);
					   //  specific heat at constant pressure (J/kg.K)
	k= 0.02538+((Tavg-290.)/10.)*(0.02614-0.02538); // conductivity (W/m.K)

	RAbsG = (9.81*beta*Cube( b)*abs(Ts-Tg)*Square( rho)*Cp)/(dvisc*k);
}		// AirProps
//------------------------------------------------------------------------------
static double SLtoGL(	// shade layer to glass layer heat transfer coefficient
	double breal,	// distance from shade to glass (m) where air flow takes place
	double Ts,		// shade temperature (K)
	double Tg,		// glass temperature (K)
	int scheme)		// model selection

// fill gas is always air, orientation is always vertical
// hsg should be infinite at b=0, zero at b=large

// returns hsg, the heat transfer coefficient, shade-to-glass, W/m2K
{

	double hsg = 0.;   //  default - large b spacing
	double b = max( breal, 0.00001);	// avoid division by zero

	if (scheme == 1)     //  simple conduction layer, Nu=1
	{   double Tavg=(Ts+Tg)/2;		// T for properties calculations
		double k= 0.02538+((Tavg-290.)/10.)*(0.02614-0.02538); // conductivity (W/m.K)
		hsg=k/b;
	}
	else if (scheme == 2)     // similar to Nu=1 but small penalty at
	{                         // larger Ra    (Collins)
		double RAbsG, k;
		AirProps( Ts, Tg, b, k, RAbsG);
		double Nubsg = 1+0.2*(1-exp(-0.005*RAbsG));
		hsg=Nubsg*k/b;
	}
	return hsg;
}    // SLtoGL
//-----------------------------------------------------------------------------
static double SLtoAMB(	// shade layer to ambient (room air) convective heat transfer coefficient
	double b,		// distance from shade to glass (m) where air flow takes place
	double L,		// window height, m (usually taken as 1 m)
	double Ts,		// shade temperature (K)
	double Tamb,	// room temperature (K)
	double hc_in,	// indoor (room) convective coefficient, W/m2K
	int scheme)		// model selection

// fill gas is always air, orientation is always vertical
// hsa should be h-flatplate at b=0 and 2*h-flatplate at b=large
// note that hsa is the same at slat angle = 0, 90, -90 degrees but increase
//        by 20% at slat angle =45 degrees to mimic air pumping between slats
// therefore, specify slat angle=0 or 90 or -90 is shade is other than
//        a venetian blind
// returns shade-to-ambient convective coeff, W/m2K
{
	double hsa;		// result

	switch (scheme)
	{
	case 1: case 2:
		{	double RAbsA, k;
			AirProps( Ts, Tamb, b, k, RAbsA);
			double hfp =
				scheme == 1 ? HIC_ASHRAE( L, Ts, Tamb)	// h - flat plate, influence by
														// window height and temperature
														// difference.  Note:  hfp goes to
														// zero as delta-T goes to 0
				: hc_in;		// scheme 2
			hsa = hfp * (1. + exp(-6000./RAbsA));
			// hsa goes to 2*hfp at large b and hfp at small b and small (20%)
			// penalty is applied if slat angle is not zero or +/- 90 degrees
			// Note:  influence of distance is lost if delta-T goes to zero
			// Note:  as delta-T -> zero, Rabga->0, SLtoAmb -> hfp, not 2hfp,
			//     for any spacing, even large b.  This is a problem
		}
		break;
	case 3:
		// use callers value for h - flat plate
		//   adjust for distance from window glass
		hsa = hc_in * (2.0 - exp(-4.6 * b /0.1));
			// Note:  using this approach, L and temperatures no longer have
			//                               influence on result
			//  hsa = 2*hc_in when glass/shade spacing, b, is large
			//  hsa = hc_in when glass/shade spacing, b, is zero
			//  The exponential decay is 99% complete at b=4 inches = 0.1 m
			//                                               ln(0.01) = -4.6
			//  This coefficient could be fine tuned in future versions, perhaps
			//  as a function of boundary layer thickness for specific values
			//  of glass and shade temperatures
		break;

	default:
		hsa = 2. * hc_in;	//    DEFAULT - convection from both sides
							//    of shading layer - large spacing, b
	}
	return hsa;
}  // SLtoAMB
//-----------------------------------------------------------------------------
static double GLtoAMB(	// glass to ambient (room air)  heat transfer coefficient
	double b,	// distance from shade to glass (m) where air flow takes place
	double L,	// window height (m), usually taken as 1 meter
	double Tg,	// glass temperature (K)
	double Tamb,	// room temperature (K)
	double hc_in,	// indoor (room) convective coefficient, W/m2K
	int scheme)		// flag to select model
					// scheme=2 has problems, scheme=3 recommended
// fill gas is always air, orientation is always vertical
// hga should be zero at b=0, h-flatplate at b=large
// returns hga = glass-to-ambient heat transfer coefficient, W/m2K
{

	double hga;
	switch (scheme)
	{
	case 1:		// Collins
	case 2:
		{	double RAbsA, k;
			AirProps( Tg, Tamb, b, k, RAbsA);
			double hfp =
				scheme == 1 ? HIC_ASHRAE( L, Tg, Tamb)	// h - flat plate, influence by
													// window height and temperature
													// difference.  Note:  hfp goes to
													// zero as delta-T goes to 0
				:	hc_in;		// scheme 2

			hga = hfp*exp(-50./RAbsA);
				// Note:  as delta-T -> zero, Rabga->0, hgamb -> zero too
				//        for any spacing, even large b.  This is a problem
			break;
		}
	case 3:
		// use callers hc_in as h - flat plate - from calling routine
		hga = hc_in * (1. - exp(-4.6 * b/0.1));
		// Note:  using this approach, L and temperatures no longer have
		//                               influence on result
		//  hga = hc_in when glass/shade spacing, b, is large
		//  hga = zero  when glass/shade spacing, b, is zero
		//  The exponential decay is 99% complete at b=4 inches = 0.1 m
		//                                               ln(0.01) = -4.6
		//  This coefficient could be fine tuned in future versions, perhaps
		//  as a function of boundary layer thickness for specific values
		//  of glass and shade temperatures
		break;
	default:
		hga = hc_in;	// default, suitable for large b
	}
	return hga;
}    // GLtoAMB
//-----------------------------------------------------------------------------
bool CFSTY::cf_Thermal(		// layer temps / heat fluxes
	double TIN,			// indoor air temperature, K
	double TOUT,		// outdoor air temperature, K
	double HCIN,		// indoor convective coefficient, W/m2K
	double HCOUT,		// outdoor convective  coefficient, W/m2K
	double TRMIN,		// indoor mean radiant temp, K
	double TRMOUT,		// outdoor mean radiant temp, K
	double ISOL,		// total incident solar, W/m2 (values used for SOURCE derivation)
						//   = outside direct + outside diffuse + inside diffuse
	const double* SOURCE,	// absorbed solar by layer,  W/m2 (from cf_Solar)
							//  [ 0]: outside / unused
							//  [ 1..NL]: layer by layer
							//  [ NL+1]: room
	double TOL,				// convergence tolerance, usually
							//   0.001 (good) or 0.0001 (tight)
	int IterControl,		// interation control
							//   abs( IterControl) = max # of iterations
							//   if < 0, do NOT initialize T
	double* T,				// returned: layer temperatures, 1=outside-most layer, K
							//   [ 0]: outside / unused
							//   [ 1..NL]: layers
							//   [ NL+1]: room / unused
	double* Q,			    // returned: heat flux at ith gap (betw layers i and i+1), W/m2
							//   + = heat flow indoor to outdoor
	double& QInConv,		// returned: total radiant heat gain to room, W/m2
	double& QInRad,			// returned: total convective heat gain to room, W/m2
	double& Ucg,			// returned: center-glass U-factor, W/m2-K
	double& SHGCcg,			// returned: center-glass SHGC (Solar Heat Gain Coefficient)
	double& Cx,				// returned: cross coupling term, W/m2
	double& NConv,			// returned: convective inward-flowing fraction, dimless
							//   = fraction of ISOL that is added to room via convection
							//     (0 if ISOL is 0)
	double& NRad,			// returned: rediant inward-flowing fraction, dimless
							//   = fraction of ISOL that is added to room via LW radiation
							//     (note: does not include transmitted SW)
							//     (0 if ISOL is 0)
	double& FHR_IN, 		// hre/(hre+hce) fraction radiant h, outdoor or indoor, used for TAE
	double& FHR_OUT)

// calculates the glazing temperatures of the
// various elements of a window/shade array while solving an energy
// balance which accounts for absorbed solar radiation, indoor-
// outdoor temperature difference, any combination of hemispherical
// IR optical properties of the various glazings/shading layers.
// Mean radiant temperatures can differ from air temperature on
// both the indoor and outdoor sides.
// It is also possible to allow air-flow between the two layers
// adjacent to the indoor side and/or the two layers adjacent the
// outdoor side.
// U-factor and SHGC calculations are also included (optional)

// returns true if calculation succeeds
//         false on error

{
	int I,J;		// commonly-used layer idxs

	// fixed parameters
	const int hin_scheme = 3;	// specifies scheme for calculating convection coefficients
								//  glass-to-air and shade-to-air
								//  if open channel air flow is allowed
								//  see the corresponding subroutines for detail
								//  = 1 gives dependence of height, spacing, delta-T
								//  = 2 gives dependence of spacing, delta-T but
								//    returns unrealistic values for large spacing
								//  = 3 glass-shade spacing dependence only on HCIN
								//  = negative, applies HCIN without adjusting for
								//    temperature, height, spacing, slat angle
								//  Recommended -> hin_scheme=3 for use with HBX,
								//              simplicity, right trends wrt spacing
	const double HFS = 1.;	// height of fenestration system, assumed 1 m

	// caller options
	int MAX_ITERATION = max( abs( IterControl), 1);		// max # of iterations
	bool bDoLTempInit = IterControl > 0;	// true iff layer temps should be initialized
											//   else retain current values

	// set true to print debugging info
#if defined( TEST_PRINT)
	bool DoPrint = true;
#else
	[[maybe_unused]] bool DoPrint = false;
#endif

	if (NL < 1)
		return false;				// no layers -- fail
	bool bRet = MAX_ITERATION < 2;	// cannot check for convergence so assume OK


// organize longwave radiant properties - book-keeping
//  these variables help simplify the code because it is useful to increase
//  the scope of the arrays to include indoor and outdoor nodes - more general
//  Later could add RHOF_ROOM to the parameter list

	A1D< 0> RHOF( NL+2);	// longwave reflectance, front
	A1D< 0> RHOB( NL+2);	// longwave reflectance, back
	A1D< 0> EPSF( NL+2);	// longwave emisivity,   front
	A1D< 0> EPSB( NL+2);	// longwave emisivity,   back
	A1D< 0> TAU( NL+2);		// longwave transmittance

	const double TAU_ROOM  = 0.;	// must always be zero
	const double RHOF_ROOM = 0.;    // almost always zero
	const double EPSF_ROOM = 1.- TAU_ROOM - RHOF_ROOM; // almost always unity
	RHOF(NL+1) = RHOF_ROOM;
	EPSF(NL+1) = EPSF_ROOM;
	RHOB(NL+1) = EPSB(NL+1) = 0.;	// "back" of room, unused
	TAU (NL+1) = TAU_ROOM;

	for( I=1; I<=NL; I++)
	{	EPSF( I) = L( I).LWP_EL.EPSLF;
		EPSB( I) = L( I).LWP_EL.EPSLB;
		TAU ( I) = L( I).LWP_EL.TAUL;
		RHOF( I) = 1. - L( I).LWP_EL.EPSLF - L( I).LWP_EL.TAUL;
		RHOB( I) = 1. - L( I).LWP_EL.EPSLB - L( I).LWP_EL.TAUL;
	}

	const double TAU_OUT =  0.0;                     // must always be zero
	const double RHOB_OUT = 0.0;                     // DON'T CHANGE
	const double EPSB_OUT = 1. - TAU_OUT - RHOB_OUT; // should always be unity
	TAU (0)  = TAU_OUT;
	EPSB(0)  = EPSB_OUT;
	RHOB(0)  = RHOB_OUT;
	RHOF(0)  = EPSF(0) = 0.;	// "front" of outdoors, unused


//	Message( msgXXX, "RHOF " 8F8.5)') RHOF(0:NL+1)
//	Message( msgXXX, "RHOB " 8F8.5)') RHOB(0:NL+1)
//	Message( msgXXX, "EPSF " 8F8.5)') EPSF(0:NL+1)
//	Message( msgXXX, "EPSB " 8F8.5)') EPSB(0:NL+1)
//	Message( msgXXX, "TAU " 8F8.5)') TAU(0:NL+1)


	// set up the matrix
	int ADIM = 2*NL + 2;
	A2D< 3*CFSMAXNL+2, 3*CFSMAXNL+4> A( ADIM, ADIM+2, 1, 1);
	A = 0.;		// right hand column will be all zeros

	// first Row
	A(1,1) = 1;
	A(1,2) = -RHOB(0);

	// middle
	for (I=1; I<=NL; I++)
	{	A(2*I,2*I-1) = -RHOF( I);
		A(2*I,2*I) = 1;
		A(2*I,2*I+2) = -TAU( I);

		A(2*I+1,2*I-1) = -TAU( I);
		A(2*I+1,2*I+1) = 1;
		A(2*I+1,2*I+2) = -RHOB( I);
	}

	// Last Row
	A(2*NL+2, 2*NL+1) = -RHOF(NL+1);
	A(2*NL+2, 2*NL+2) = 1;

	// save a copy of A matrix
	A2D< 3*CFSMAXNL+2, 3*CFSMAXNL+4> ACopy( A);

	// unit emission from outside
	A( 1, ADIM+1) = 1;

	A1D< 1> XSOL;
	A.Solve( XSOL);

	// radiosities due to emission from other surfaces, constant WRT temperature
	//   J_xx( i, j): i = emitting surface, j = radiant surface
	// find the shape factors
	A2D< CFSMAXNL+2, CFSMAXNL+2> J_BB( NL+2, NL+2, 0, 0);
					// radiosity of a back due to unit emission of a back;
	A2D< CFSMAXNL+2, CFSMAXNL+2> J_BF( NL+2, NL+2, 0, 0);
					// radiosity of a front due to unit emission of a back;
	A2D< CFSMAXNL+2, CFSMAXNL+2> J_FF( NL+2, NL+2, 0, 0);
					// radiosity of a front due to unit emission of a front;
	A2D< CFSMAXNL+2, CFSMAXNL+2> J_FB( NL+2, NL+2, 0, 0);

	J_BB = 0.;		// unnecessary?
	J_BF = 0.;
	J_FF = 0.;
	J_FB = 0.;

	// from back, 0
	J_BB(0,0) = XSOL(1);

	for (J=1; J<=NL; J++)
	{	J_BF(0,J) = XSOL(2*J);
		J_BB(0,J) = XSOL(2*J+1);
	}
	J_BF(0,NL+1) = XSOL(2*NL+2);

	// from each layer
	for (I=1; I<=NL; I++)
	{	// unit emission from front of layer I
		A = ACopy;
		A( I*2, ADIM+1) = 1;
		A.Solve( XSOL);
		J_FB(I,0) = XSOL(1);
		for (J=1; J<=NL; J++)
		{	J_FF(I,J) = XSOL(2*J);
			J_FB(I,J) = XSOL(2*J+1);
		}
		J_FF(I, NL+1) = XSOL(2*NL+2);

		// unit emission from back of layer I
		A = ACopy;
		A(I*2+1, ADIM+1) = 1;
		A.Solve( XSOL);

		J_BB(I,0) = XSOL(1);
		for (J=1; J<=NL; J++)
		{	J_BF(I,J) = XSOL(2*J);
			J_BB(I,J) = XSOL(2*J+1);
		}
		J_BF(I, NL+1) = XSOL(2*NL+2);
	}

	// unit emissions from inside
	A = ACopy;
	A(NL*2+2, ADIM+1) = 1;
	A.Solve( XSOL);

	// from front, NL+1
	J_FB(NL+1, 0) = XSOL(1);
	for (J=1; J<=NL; J++)
	{	J_FF(NL+1,J) = XSOL(2*J);
		J_FB(NL+1,J) = XSOL(2*J+1);
	}
	J_FF(NL+1, NL+1) = XSOL(2*NL+2);

#if defined( _DEBUG)
	// check reciprocity
	int SUMERR_GI = 0;
	for (I=1; I<=NL+1; I++)
		for (J=0; J<=NL; J++)
			if (vNEQ( J_FF(I, J+1), J_BB(J, I-1), .0001))
				SUMERR_GI++;
	if (SUMERR_GI)
		Message( msgERR, "Radiosity trouble");
#endif

	// iterate
	 // first guess layer temperatures
	if (bDoLTempInit)
		for (I=1; I<=NL; I++)
			T[ I] = TOUT + (double( I))/(double(NL+1)) * (TIN-TOUT);
	// else use passed values

	A2D< CFSMAXNL+2, CFSMAXNL+2> HC2D(NL+2, NL+2, 0, 0);	// convective heat transfer coefficients between layers i and j
	A2D< CFSMAXNL+2, CFSMAXNL+2> HR2D(NL+2, NL+2, 0, 0);	// radiant heat transfer coefficients between layers i and j

	int iTry;
	for (iTry=1; iTry<=MAX_ITERATION; iTry++)
	{	int IB, IE;
		//convection coefficients
		HC2D = 0.;
		// deal with OCF in first or last gap
		if (NL >= 2)  // else no gaps, so no worries
		{
			// GLtoAMB call
			if (G(NL-1).GTYPE == gtyOPENIn)
			{	double TOC_EFF = G(NL-1).TAS_EFF/1000.; //effective thickness of OCF gap

				// between last two layers
				HC2D(NL-1, NL) = SLtoGL( TOC_EFF, T[ NL], T[ NL-1], 1);
				HC2D(NL, NL-1) = HC2D(NL-1, NL);

				// between last layer and room
				double ConvF = L( NL).cl_ConvectionFactor();
				HC2D(NL, NL+1) = ConvF*SLtoAMB(TOC_EFF, HFS, T[ NL], TIN, HCIN, hin_scheme);
				HC2D(NL+1, NL) = HC2D(NL, NL+1);

				// between second last layer and room
				HC2D(NL-1, NL+1) = GLtoAMB(TOC_EFF, HFS, T[ NL-1], TIN, HCIN, hin_scheme);
				HC2D(NL+1, NL-1) = HC2D(NL-1, NL+1);

				// skip the open gap when calculating convection coefficients
				IE = NL-2;
			}
			else
			{	IE = NL-1;
				// inner layer to the room
				HC2D(NL, NL+1) = HCIN;
				HC2D(NL+1, NL) = HC2D(NL, NL+1);
			}

			if (G(1).GTYPE == gtyOPENOut)
			{	// between first two layers
				HC2D(1, 2) = 0.;
				HC2D(2, 1) = HC2D(1, 2);
				// between first layer and outside
				HC2D(0, 1) = 2. * HCOUT;
				HC2D(1, 0) = HC2D(0, 1);
				// between second layer and outside
				HC2D(0, 2) = HCOUT;
				HC2D(2, 0) = HC2D(0, 2);
				// skip the open gap when calculating convection coefficients
				IB = 2;
			}
			else
			{	IB = 1;
				// outer layer to outside
				HC2D(0,1) = HCOUT;
				HC2D(1,0) = HC2D(0,1);
			}

			for (int i=IB; i<=IE; i++)
			{	HC2D(i, i+1) = G( i).cg_HConvGap( T[  i], T[ i+1]);
				HC2D(i+1, i) = G( i).cg_HConvGap( T[ i+1], T[  i]);
			}
		}
		else if (NL == 1)
		{	// only one layer, calculate the coefficients
			HC2D(0,1) = HCOUT;
			HC2D(1,0) = HC2D(0,1);
			HC2D(1,2) = HCIN;
			HC2D(2,1) = HC2D(1,2);
		}

		// radiative coefficients
		HR2D = 0.;
		for (I=1; I<=NL; I++)
		{	// to outside
			HR2D(0,I) = EPSB(0) * (EPSF( I)*J_FF(I,1 ) + EPSB( I)*J_BF(I,1))
						* RadTX( T[  I], TRMOUT);
			HR2D(I,0) = HR2D(0,I);

			// between layers
			for ( J=I+1; J<=NL; J++)
			{	HR2D(I,J) = (   EPSF( J)*(EPSF( I)*J_FB(I,J-1) + EPSB( I)*J_BB(I,J-1)) +
							   + EPSB( J)*(EPSF( I)*J_FF(I,J+1) + EPSB( I)*J_BF(I,J+1)) )
							* RadTX( T[  I], T[ J]);
				HR2D(J,I)= HR2D(I,J);
			}
			// if this gets used we'll know what's wrong, safety check
			HR2D(I, I) = -1000.;

			// to inside
			HR2D(I, NL+1) = EPSF(NL+1)*(EPSF( I)*J_FB(I,NL)+EPSB( I)*J_BB(I, NL))
							* RadTX( T[  I], TRMIN);
			HR2D(NL+1, I) = HR2D(I,NL+1);
		}

		// set up matrix
		ADIM = NL;
		A.Resize( ADIM, ADIM+2, 1, 1);
		A = 0.;
		for (I=1; I<=NL; I++)
		{	A( I, ADIM+1) = HC2D(0,I)*TOUT + HR2D(0,I)*TRMOUT
							+ HC2D( I, NL+1)*TIN + HR2D(I,NL+1)*TRMIN
							+ SOURCE[ I];
			for (J=0; J<=NL+1; J++)
			{	if (J != I)
				{	A( I, I) += HC2D(I, J) + HR2D( I, J);
					if (J != 0 && J != NL+1)
						A(I, J) = - (HC2D(I,J) + HR2D(I,J));
				}
			}


		}
		A.Solve( XSOL);

		// solution = new later temps
		double TNew[ CFSMAXNL+2];
		for (I=1; I<=NL; I++)
			TNew[ I] = XSOL( I);

		// Message( msgXXX, '(/" TNEW( I) = ", 7(F10.3))') (TNEW[ j], J=1, NL)

		// check for convergence
		double MAXERR = 0.;
		for (I=1; I<=NL; I++)
			MAXERR = max( MAXERR, abs( (TNew[ I] - T[  I])/T[  I]));

		for (I=1; I<=NL; I++)
			T[  I] = TNew[ I];

		if (MAXERR <= TOL)
		{	bRet = true;
			break;
		}
	}  // iTry Iteration loop

	// calculate radiosities of each layer
	ADIM = 2*NL + 2;
	A = ACopy; // right hand column will be all zeros

	// right hand side of A is now emission from each layer
	// back of outside
	A( 1, ADIM+1) = RadPwr( TRMOUT, EPSB(0));
		// for each layer
	for (I=1; I<=NL; I++)
	{	A(2*I, ADIM+1) = RadPwr( T[  I], EPSF( I));		// front
		A(2*I+1, ADIM+1) = RadPwr( T[  I], EPSB( I));	// back
	}
	// for inside
	A(2*NL+2, ADIM+1) = RadPwr( TRMIN, EPSF(NL+1));

	A.Solve( XSOL);

	A1D< 0> JF( NL+2);	// front (outside facing) radiosity of surfaces, W/m2
						//   JF( NL+1) = room radiosity
	A1D< 0> JB( NL+2);	// back (inside facing) radiosity, W/m2
						//   JB( 0) = outside environment radiosity

	JF(0) = 0.;
	JB(0) = XSOL( 1);
	for (I=1; I<=NL; I++)
	{	JF( I) = XSOL(2*I);		// front
		JB( I) = XSOL(2*I+1);	// back
	}
	JF(NL+1) = XSOL(2*NL+2);
	JB(NL+1) = 0.;

	// calculate indices of merit
	// set up matrix
	ADIM = NL;
	A.Resize( ADIM, ADIM+2, 1, 1);
	A = 0.;

	for (I=1; I<=NL; I++)
	{	for (J=0; J<=NL+1; J++)
		{	if (J != I)
			{	A(I,I) += HC2D(I,J) + HR2D(I,J);
				if (J != 0 && J != NL+1)
					A(I,J) = - (HC2D(I,J) + HR2D(I,J));
			}
		}
	}

	ACopy = A; // Acopy has matrix for indices of merit now, rather than for radiosities

	// dummy variables for boundary conditions in calculating indices of merit
	double TOUTdv = 1.;
	double TRMOUTdv = 1.;
	double TINdv = 0.;
	double TRMINdv = 0.;
	A1D< 0> SOURCEdv;
	SOURCEdv = 0.;

	// Calculate Ucg
	for (I=1; I<=NL; I++)
		A(I, ADIM+1) = HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv
					  + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
					  + SOURCEdv( I);
	A.Solve( XSOL);

	// calculate heat flow
	//   Note: XSOL( I) = temp of layer I
	double Q_INdv = HC2D(0, NL+1)*(TOUTdv - TINdv) + HR2D(0, NL+1) * (TRMOUTdv - TRMINdv);
	for (I=1; I<=NL; I++)
		Q_INdv += HC2D(I,NL+1)*(XSOL( I)-TINdv) + HR2D(I,NL+1)*(XSOL( I)-TRMINdv);

	Ucg = Q_INdv;
	[[maybe_unused]] double Rvalue = 5.678/Ucg;		// Resistance in I-P units, ft2-F/Btuh

	// find SHGCcg
	SHGCcg = 0.;
	if (ISOL > 0.01)  // for now only calculate if insolation is non-zero
	{	A = ACopy;

		TOUTdv = 0.;
		TRMOUTdv = 0.;
		TINdv = 0.;
		TRMINdv = 0.;

		for (I=1; I<=NL; I++)
			A(I, ADIM+1) = HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv
						  + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
						  + SOURCE[ I];
		A.Solve( XSOL);

		// calculate heat flow
		Q_INdv = HC2D(0, NL+1)*(TOUTdv - TINdv) + HR2D(0, NL+1) * (TRMOUTdv - TRMINdv)
				+ SOURCE[ NL+1];
		for (I=1; I<=NL; I++)
			Q_INdv += HC2D(I,NL+1)*(XSOL( I)-TINdv) + HR2D(I,NL+1)*(XSOL( I)-TRMINdv);

		SHGCcg = Q_INdv/ISOL;

	} // calculate SHGCcg

	// calculate FHR_IN
	A = ACopy;

	TOUTdv =	0.;
	TRMOUTdv =	0.;
	TINdv =		1.;
	TRMINdv =	0.;
	SOURCEdv =	0.;

	for (I=1; I<=NL; I++)
		A(I, ADIM+1) =  HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv +
					  + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
					  + SOURCEdv( I);

	A.Solve( XSOL);

	// calculate heat flow
	Q_INdv = HC2D(0, NL+1)*(TOUTdv - TINdv) + HR2D(0, NL+1) * (TRMOUTdv - TRMINdv)
			 + SOURCEdv( NL+1);
	for (I=1; I<=NL; I++)
		Q_INdv += HC2D(I,NL+1)*(XSOL( I)-TINdv) + HR2D(I,NL+1)*(XSOL( I)-TRMINdv);

	FHR_IN = 1.0 + (Q_INdv/Ucg);
	[[maybe_unused]] double TAE_IN = FHR_IN*TRMIN + (1-FHR_IN)*TIN;

	// calculate FHR_OUT
	A = ACopy;

	TOUTdv =	1.;
	TRMOUTdv =	0.;
	TINdv =		0.;
	TRMINdv =	0.;
	SOURCEdv =	0.;

	for (I=1; I<=NL; I++)
		A(I, ADIM+1) = HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv
					 + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
					 + SOURCEdv( I);

	A.Solve( XSOL);

	// calculate heat flow
	Q_INdv = HC2D(0, NL+1)*(TOUTdv - TINdv) + HR2D(0, NL+1) * (TRMOUTdv - TRMINdv)
		   + SOURCEdv(NL+1);
	for (I=1; I<=NL; I++)
		Q_INdv += HC2D(I,NL+1)*(XSOL( I)-TINdv) + HR2D(I,NL+1)*(XSOL( I)-TRMINdv);

	FHR_OUT = 1.0 - (Q_INdv/Ucg);
	[[maybe_unused]] double TAE_OUT = FHR_OUT*TRMOUT + (1-FHR_OUT)*TOUT;

	// compute inward-flowing fractions
	// can only be done if ISOL > 0
	NRad = 0.;
	NConv = 0.;
	if (ISOL >= 0.001)
	{   TOUTdv =	0.;
		TRMOUTdv =	0.;
		TINdv =		0.;
		TRMINdv =	0.;

		//  M=radiant and P=convective fractions for each layer to inside
		double M[ CFSMAXNL+1] = { 0. };
		double P[ CFSMAXNL+1] = { 0. };
		for (J=1; J<=NL; J++)
		{   SOURCEdv =	0.;
			SOURCEdv( J) = 1.; // only one layer has any heat energy

			A = ACopy;
			for (I=1; I<=NL; I++)
				A(I, ADIM+1) = HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv
							 + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
							 + SOURCEdv( I);
			A.Solve( XSOL);

			// calculate radiant and convective heat flows
			for (I=1; I<=NL; I++)
			{	M[ J] += HR2D(I,NL+1)*(XSOL( I)-TRMINdv);
				P[ J] += HC2D(I,NL+1)*(XSOL( I)-TINdv);
			}
		}

		// find A for each layer, use to get NRad & NConv fraction for that layer
		for (I=1; I<=NL; I++)
		{	SOURCEdv( I) = SOURCE[ I]/ISOL;
			NRad  += M[ I]*SOURCEdv( I);
			NConv += P[ I]*SOURCEdv( I);
		}

#if defined( _DEBUG)
		// check values, should be equal to SHGC
		if (SHGCcg > 0. && ISOL > .01)
		{	Q_INdv = SOURCE[ NL+1];
			for (I=1; I<=NL; I++)
				Q_INdv += SOURCE[ I]*(M[ I] + P[ I]);
			Q_INdv /= ISOL;
			double SUMERR = Q_INdv - SHGCcg;
			if (abs( SUMERR) > .0001)
				Message( msgERR, "Check for M & P, should be 0: %0.6f", SUMERR);
		}
#endif
	}

	// Calculate Q for each gap

	// use actual conditions, no dummy variables to initiate
	for (I=0; I<=NL; I++)
	{
		// calculate convective heat transfer outward from layer I+1
		double Q_OUTdv = 0;
		if (I != 0)
		{	if (I != NL)
			{	Q_OUTdv = HC2D(I+1,0)*(T[ I+1]-TOUT);
				for (J=1; J<I; J++)
					Q_OUTdv += HC2D(I+1,J)*(T[ I+1]-T[ J]);
			}
			else
			{	Q_OUTdv = HC2D(NL+1,0)*(TIN-TOUT);
				for (J=1; J<I; J++)
					Q_OUTdv += HC2D(NL+1,J)*(TIN-T[ J]);
			}
			// else no earlier layers to have heat transfer to
		}

		// calculate convective heat transfer inward from layer I (includes to layer I+1)
		Q_INdv = 0.;
		if (I == 0)
		{	for (J=1; J<=NL; J++)
				Q_INdv += HC2D(0,J)*(TOUT-T[ J]);
			Q_INdv += HC2D(0,NL+1)*(TOUT - TIN);
		}
		else
		{	for (J=I+1; J<=NL; J++)
				Q_INdv += HC2D(I,J)*(T[ I] - T[ J]);
			Q_INdv += HC2D(I,NL+1)*(T[ I] - TIN);
		}

		Q[ I] = JF(I+1) - JB( I) + Q_OUTdv - Q_INdv;
//		Message( msgXXX, "Convective heat flow in, out = " 2F10.5)') Q_INdv, Q_OUTdv

	}

#if 0 && defined( _DEBUG)
	// 9-2016: not convinced this test is formulated correctly
	//    disable until understood
	// check Q values -- perform energy balance at each gap
	// depending on TOL, this check does not match perfectly
	//   with typical TOL=.0001, .05 absTol seems OK 5-17-2011
	for (I=1; I<=NL; I++)
	{	// double SUMERR = Q[ I] - Q[ I-1] + SOURCE[ I];
		if (vNEQ( Q[ I]+SOURCE[ I], Q[ I-1], .05, .2))
		{	Message( msgERR, "non-0 net energy=%0.5f", Q[ I] - Q[ I-1] + SOURCE[ I]);
		}
	}
#endif

	// Calculate Cx
	// experiment 1 (only experiment if not debugging)
	TOUTdv =	1.;
	TRMOUTdv =	1.;
	TINdv =		0.;
	TRMINdv =	1.;
	SOURCEdv =	0.;

	A = ACopy;

	for (I=1; I<=NL; I++)
		A(I, ADIM+1) = HC2D(0, I)*TOUTdv + HR2D(0, I)*TRMOUTdv
					 + HC2D(I, NL+1)*TINdv + HR2D(I, NL+1)*TRMINdv
					 + SOURCEdv( I);
	A.Solve( XSOL);

	// calculate Q_conv and Q_rad
	double Q_CONVdv = HC2D(0,NL+1)*(TOUTdv-TINdv);
	double Q_RADdv  = HR2D(0,NL+1)*(TRMOUTdv-TRMINdv);

	for (I=1; I<=NL; I++)
	{	Q_CONVdv += HC2D(I, NL+1)*(XSOL( I)-TINdv);
		Q_RADdv  += HR2D(I, NL+1)*(XSOL( I)-TRMINdv);
	}

	Cx = -Q_RADdv;

	// calculate total convective and radiative gain of inside of room
	// total gain to room
	QInConv = HC2D(0,NL+1)*(TOUT-TIN);
	QInRad  = HR2D(0,NL+1)*(TRMOUT-TRMIN);
	for (I=1; I<=NL; I++)
	{	QInConv += HC2D( I, NL+1)*(T[ I]-TIN);
		QInRad  += HR2D( I, NL+1)*(T[ I]-TRMIN);
	}

#if defined( _DEBUG)
	// check: compare total gain to alternative calc
	//  match is generally extremely good, 5-15-2011
	double QInConvX = (1.-FHR_IN)*Ucg*(TAE_OUT - TIN) + Cx*( TRMIN - TIN) + NConv*ISOL;
	double QInRadX = FHR_IN*Ucg*(TAE_OUT - TRMIN) + Cx*(TIN-TRMIN) + NRad*ISOL;

	if (vNEQ( QInConv, QInConvX, .0002))
	{	Message( msgERR, "QConv mismatch");
		vNEQ( QInConv, QInConvX, .0002);
	}
	if (vNEQ( QInRad, QInRadX, .0002))
	{	Message( msgERR, "QRad mismatch");
		vNEQ( QInRad, QInRadX, .0002);
	}
#endif

	return bRet;

}  // CFSTY::cf_Thermal
//----------------------------------------------------------------------------
static double FRA(	// Rayleigh number
	double TM,				// mean gas temp, K
	double T,				// gas layer thickness, m
							//   (as adjusted e.g. re VB models)
	double DT,				// temp difference across layer, K
	double AK, double BK,	// gas conductance coeffs, K = AK + BK*TM
	double ACP, double BCP,	// gas specific heat coeffs, CP = ACP + BCP*TM
	double AVISC, double BVISC,	// gas viscosity coeffs, VISC = AVISC + BVISC*TM
	double RHOGAS)			// gas density, kg/m3
// returns Rayleigh number
{
	double Z=1.;
	double K = AK + BK*TM;
	double CP = ACP + BCP*TM;
	double VISC=AVISC + BVISC*TM;
	double RA = GACC*RHOGAS*RHOGAS*DT*T*T*T*CP/(VISC*K*TM*Z*Z);
//	PR = VISC * CP / K		// check Prandtl number, s/b 0.70 - 0.71 per JLW
	return RA;
}  // FRA
//**********************************************************************
static double FNU(	// Nusselt number (function of Rayleigh number)
	double RA)	// Rayleigh number
// correlation from Wright (1996)
// returns Nusselt number
{
	double FNU = 0.;
	double ARA = abs( RA);
	if (ARA <= 10000.)
		FNU=1. + 1.75967E-10 * pow( ARA, 2.2984755);
	else if (ARA <= 50000.)
		FNU=0.028154 * pow( ARA, 0.413993);
	else
		FNU=0.0673838 * pow( ARA, 1./3.);
	return FNU;
}  // FNU
//--------------------------------------------------------------------------
#if 0
// unused, save for possible future use
static double HRadPar(
	double T1, double T2,	// bounding surface temps, K
	double E1, double E2)	// bounding surface emissivities
// for parallel, opaque plates configuration
// and automatically reverts to small-object-in-large-enclosure
// if one of the emissivities is set to unity  i.e., set E1=1
// and surface 2 is the small object with hr based on area A2
// If one emissivity is zero then hr=0, division by zero is
// avoided even if T1=T2
// returns radiative coefficient, hr, W/m2-K
{
	double hr = 0.;
	if (E1 > 0.001 && E2 > 0.001)
	{	double D = (1./E1) + (1./E2)-1.0;
		hr = (SIG/D) * (T1+T2) * (T1*T1 + T2*T2);
	}
	return hr;
}  // HRadPar
#endif
//==============================================================================
double CFSLAYER::cl_ConvectionFactor() const	// layer convection enhancement
// modifies convection rate per shade config
{
	double confF = 1.;
	if (LTYPE == ltyVBHOR)
	{	// horiz VB: enhanced convection at +/- 45 due to "pumping"
		double slatADeg = min( 90., abs( PHI_DEG));
		confF = 1. + .2 * sin( DTOR * 2.*slatADeg);
	}
	return confF;
}  // CFSLAYER::cl_ConvectionFactor
//------------------------------------------------------------------------------
static double HRLin(	// linearized radiant coefficient
	double Ts,		// surface temp, C
	double Es,		// surface emissivity (0 - 1)
	double Tx)		// effective black body temp of surround, C
// returns linearized radiant coefficient, W/m2K
{
	double HR = Es * RadTX( Ts+TKAdd, Tx+TKAdd);
	return HR;
}	// HRLin
//------------------------------------------------------------------------------
static double HRLinK(	// linearized radiant coefficient (abs temp variant)
	double Ts,		// surface temp, K
	double Es,		// surface emissivity (0 - 1)
	double Tx)		// effective black body temp of surround, K
// returns linearized radiant coefficient, W/m2K
{
	double HR = Es * RadTX( Ts, Tx);
	return HR;
}	// HRLinK
//-----------------------------------------------------------------------------
static void HtoHCHR(		// hc and hr from hCombined
	double H,		// combined radiant + convective coefficient, W/m2
	double Ts,		// surface temp, C
	double Es,		// surface emissivity (0 - 1)
	double Tx,		// effective black body temp of surround, C
	double& HC,	// returned: convective coefficient, W/m2
	double& HR)	// returned: linearized radiant coefficent, W/m2
{
	HR = HRLin( Ts, Es, Tx);
	HC = H - HR;		// ignore possible negative value
}		// HtoHCHR
//==============================================================================
bool CFSTY::cf_UFactor(	// U-factor for CFS
	double TOUT,		// outdoor temperature, C (air and MRT)
	double HCOUT,		// outdoor convective coefficient, W/m2-K
	double TIN,			// indoor air temperature, C
	double HCIN,		// indoor convective coefficient, W/m2-K
	double& U,			// returned: U factor, W/m2-K
						//   for conditions specified (no incident solar)
	double DTRMIN /*=0*/,	// inside MRT offset, K; dflt = 0
							//   TRMIN = TIN + DTRMIN
	double* TL /*=NULL*/)	// optionally returned: layer temps, K
// returns true iff calculation succeeded, false if error (0 delta-T, )
{
double SHGC, Cx, NRad, NConv, FHR_IN, FHR_OUT, QInConv, QInRad;
double Q[ CFSMAXNL+2], TX[ CFSMAXNL+2];

	if (abs( TOUT - TIN) < 0.01)
	{	U = -1.;
		return false;
	}

	double TOABS = TOUT + TKAdd;
	double TRMOUT = TOABS;
	double TIABS = TIN + TKAdd;
	double TRMIN = TIABS + DTRMIN;

	double TOL=1.E-4;		// .001 -> .0001 9-10-08
	double ISOL = 0.;		// no solar
	double SOURCE[ CFSMAXNL+2] = { 0. };
	double* T = TL ? TL : TX;

	bool bRet = cf_Thermal( TIABS, TOABS, HCIN, HCOUT, TRMIN, TRMOUT,
		ISOL, SOURCE, TOL, 100, T, Q, QInConv, QInRad, U, SHGC,
		Cx, NRad, NConv, FHR_IN, FHR_OUT);

	return bRet;
}  // CFSTY::cf_UFactor
//------------------------------------------------------------------------------
bool CFSTY::cf_UNFRC(		// NFRC rated *HEATING* U-factor
	double& UNFRC)	// returned: U-factor derived with NFRC assumptions
					//   W/m2-K
// returns true if U-factor successfully derived
{
#if 1
	// improved match to NFRC
	double HXO = -26.;	// fixed outside HCO (=4 + 4*5.5 per NFRC 100)
	double HXI = 0.;	// to be calculted from height
	double HT = 1.5;	// nominal height, m (small effect only)
#else
x   // 2009 assumptions
x	double HXO = +29.  // fixed outside conv+rad
x	double HXI = 0.	// to be calculted from height
x	double HT = 1.2	// nominal height, m (small effect only)
#endif
	bool bRet = cf_URated( HT, -18., HXO, 21., HXI, UNFRC);
	return bRet;
}  // CFSTY::cf_UNFRC
//------------------------------------------------------------------------------
bool CFSTY::cf_URated(		// U-factor
	double HT,			// CFS height, m
						//    L<=0 implies unspecified
						//         HXI is input/fixed and must have a valid value
						//    L>0 implies use ASHRAE convective coeff relation,
						//         HXI is output
	double TO,			// outdoor air/radiant temperature, C
	double& HXO,		// outdoor surf coeff, W/m2-K
						//    HXO > 0 = combined conv+rad
						//    HXO < 0 = conv only
						//              returned updated to combined conv+rad
	double TI,			// indoor air/radiant temperature, C
	double& HXI,		// indoor combined conv+rad surf coeff, W/m2-K
	double& U)			// returned: U-factor, W/m2-K
// returns true iff U-factor successfully derived
{

double HRO, HCO = 0, HRI, HCI, TGO, TGI;
double TL[ CFSMAXNL+2] = { 0. };	// layer temps, K

	bool bRet = false;
	double DT = TI - TO;		// note DT == 0 detected in CFSUFactor()
	double EO = L( 1).LWP_EL.EPSLF;		// emissivities
	double EI = L( NL).LWP_EL.EPSLB;
	U = 5. / NL;		// initial guess
	if (HT > 0.)
		HXI = 7.;		// initial guess
	bool bHCOVar;
	if (HXO > 0)
		bHCOVar = true;	// HCO derived = HXO - HRO
	else
	{	HCO = -HXO;
		bHCOVar = false;	// HCO fixed
	}

	// iterate: find surface temps, update coeffs, converge to U
	for (int I=1; I<11; I++)
	{
		TGO = TO + U * DT / HXO;		// update glazing surface temps
		TGI = TI - U * DT / HXI;
#if 0 && defined( _DEBUG)
		// if FS is "sealed", surface conductance should match overall U*DT
		//   sealed = LW opaque and sealed gaps
		if (I>2 && IsSealed( FS))
		{	double TGODif = TGO - (TL( 1)-TkAdd);			// compare to ASHWAT temp (insurance)
			double TGIDif = TGI - (TL( NL)-TkAdd);
			if (abs( TGODif) > .01 || abs( TGIDif) > .01)
				Message( msgXXX, "CFSURated temp mismatch");
			}
		}
#endif
		if (bHCOVar)
			HtoHCHR( HXO, TGO, EO, TO, HCO, HRO);
		else
		{	HRO = HRLin( TGO, EO, TO);
			HXO = HCO + HRO;
		}
		HtoHCHR( HXI, TGI, EI, TI, HCI, HRI);
		if (HCO < .001 || HCI < .001)
			break;
		if (HT > 0.)
		{	HCI = HIC_ASHRAE( HT, TGI, TI);
			HXI = HCI + HRI;
		}
		double UOld = U;
		if (!cf_UFactor( TO, HCO, TI, HCI, U, 0., TL))
			break;
		if (I > 1 && !vNEQ( U, UOld, .0002))
		{   bRet = true;
			break;
		}
	}
	if (!bRet)
		U = -1.;	// insurance: return bad value if calc failed
	return bRet;
}  // CFSTY::cf_URated
//------------------------------------------------------------------------------
bool CFSTY::cf_SHGCRated(		// rated SHGC
	double& SHGCrat)	// NFRC rated (cooling) SHGC, dimless
// returns TRUE on success, FALSE if error
{

	// static CFSSWP swpBlack;			// black room (c'tor leaves all values 0)

	// normal properties (insurance)
	CFSSWP SWP_ON[ CFSMAXNL+1];
	memset (SWP_ON, 0, sizeof( SWP_ON));
	bool bRet = true;
	for (int iL=1; bRet && iL<=NL; iL++)
		bRet = L( iL).cl_OffNormalProperties( 0., 0., 0., SWP_ON[ iL]);

	if (bRet)
	{	const double IBEAM = 783.;	// NFRC 200
		double SOURCE[ CFSMAXNL+2] = { 0.};
		bRet = cf_Solar( SWP_ON, IBEAM, 0., 0., SOURCE);

		int itr=0;
		if (bRet)
		{	const double EI = L( NL).LWP_EL.EPSLB;
			const double TIABS = 24. + TKAdd;
			const double TRMIN = TIABS;
			const double TOABS = 32. + TKAdd;
			const double TRMOUT = TOABS;
			const double HXIRate = 7.7;		// inside conv + radiant (hc + hr) coeff per NFRC, W/m2-K
			double HCIN = 3.;				// initial guess inside hc
			const double HCOUT = 15.;		// ISO 15099: hc = 4 + 4*Vs
											//   V rating = 2.75 m/s

			for (itr=0; itr<10; itr++)
			{	double Cx, NRad, NConv, QInConv, QInRad, USink, FHR_IN, FHR_OUT;
				double Q[ CFSMAXNL+2], T[ CFSMAXNL+2];
				bRet = cf_Thermal( TIABS, TOABS, HCIN, HCOUT, TRMIN, TRMOUT,
					IBEAM, SOURCE, .001, 100, T, Q, QInConv, QInRad, USink, SHGCrat,
					Cx, NRad, NConv, FHR_IN, FHR_OUT);

				if (!bRet)
					break;
				double HXI = HRLinK( T[ NL], EI, TIABS) + HCIN;
				double HXErr = HXIRate - HXI;
				if (abs( HXErr) < .001)
					break;
				HCIN = HCIN + HXErr;
			}
			if (itr >= 10)
				bRet = false;  // did not converge
		}
	}
	if (!bRet)
		SHGCrat = 0.;
	return bRet;
}  //  CFSTY::cf_SHGCRated
//------------------------------------------------------------------------------
bool CFSTY::cf_Ratings(		// standard ratings
	double& Urat,		// returned: NFRC rated (heating) U-factor, W/m2-K
	double& SHGCrat)	// returned: NFRC rated (cooling) SHGC, dimless
// returns TRUE on success, FALSE if error
{
	bool UOK = cf_UNFRC( Urat);

	bool SHGCOK = cf_SHGCRated( SHGCrat);

	return UOK && SHGCOK;

}  // CFSTY::cf_Ratings
//==============================================================================
static void NetRad(		// beam fluxes between layers
	int NL,					// # of layers
	const CFSSWP* LSWP_ON,	// layer short wave properties
							//  [ 0]: outdoors (unused)
							//  [ 1 .. NL]: CFS equivalent layers
							//  [ NL+1]: room
	double ISOL,		// incident flux (W/m2)
	A1D< 0>& QPLUS,		// returned: outward solar flux (see Edwards paper)
	A1D< 0>& QMINUS)	// returned: inward solar flux (see Edwards paper)

//  calculates solar radiant fluxes (w/m2) between layers,  largely Edwards
//  TED, RED, QPLUS, QMINUS correspond to variables found in Edwards
//  but with reversed (layer 1=outside .. nl=inside) layer indexing
//  (equations have been changed accordingly)
//  gap i is between layer i and i+1

{
	if (NL < 1)
		return;

	// reflectance and transmittance
	A1D< 1> RED;
	A1D< 1> TED;
	RED(NL+1) = LSWP_ON[ NL+1].RHOSFBB;		// room reflectance
	TED(NL+1) = 0.;
	int iL;
	for (iL=NL; iL>=1; iL--)
	{	const CFSSWP& LP = LSWP_ON[ iL];
		TED(iL) = LP.TAUSFBB / max( .00001, 1.-LP.RHOSBBB*RED(iL+1));
		RED(iL) = LP.RHOSFBB + TED( iL)*LP.TAUSBBB*RED(iL+1);
	}

	// outward and inward solar fluxes, QPLUS and QMINUS, respectively
	QMINUS(0)=ISOL;
	QPLUS(0)=QMINUS(0)*RED(1);
	for (iL=1; iL<=NL; iL++)
	{	QMINUS( iL) = QMINUS(iL-1)*TED( iL);
		QPLUS( iL)  = QMINUS(iL)*RED(iL+1);
	}
}    // NetRad
//-----------------------------------------------------------------------------
static void TDMA_R(	// TDMA reverse solver east/west then west/east sweep
	A1D< 1> X,		// returned
	const A1D< 1>& AP,
	const A1D< 1>& AE,
	const A1D< 1>& AW,
	const A1D< 1>& BP,
	int N)
{
	int J;
	A1D< 1> ALPHA, BETA;
	ALPHA(N)=AW(N)/AP(N);
	BETA(N) =BP(N)/AP(N);
	for (J=N-1;J>=1;J--)
	{	ALPHA(J)= AW(J) / ( AP(J)-ALPHA(J+1)*AE(J));
		BETA(J) = (AE(J)*BETA(J+1) + BP(J)) / ( AP(J)-ALPHA(J+1)*AE(J) );
	}
	X(1) = BETA(1);
	for (J=2; J<=N; J++)
		X(J) = ALPHA(J)*X(J-1) + BETA(J);
}	// TDMA_R
//-----------------------------------------------------------------------------
static void TDMA(	// 1-D TDMA solver, nodes i=1..N
	A1D< 1> X,		// returned
	const A1D< 1>& AP,
	const A1D< 1>& AE,
	const A1D< 1>& AW,
	const A1D< 1>& BP,
	int N)
{
	int J;
	A1D< 1> ALPHA, BETA;
	ALPHA(1) = AE(1)/AP(1);
	BETA(1) = BP(1)/AP(1);
	for (J=2; J<=N; J++)
	{	double D = AP(J)-ALPHA(J-1)*AW(J);
		if ( abs( D) < .0001)
			ALPHA( J) = BETA( J) = 0.;
		else
		{	ALPHA(J)= AE(J) / D;
			BETA(J) = (AW(J)*BETA(J-1) + BP(J)) / D;
		}
	}

	X( N) = BETA(N);
	for (J=N-1; J>=1; J--)
		X( J) = ALPHA(J)*X(J+1) + BETA(J);
}		// TDMA
//-----------------------------------------------------------------------------
static void AutoTDMA(	//  1-D TDMA solver management routine
	A1D< 1> X,		// returned
	A1D< 1>& AP,
	const A1D< 1>& AE,
	const A1D< 1>& AW,
	const A1D< 1>& BP,
	int N)
{
// Calls TDMA for forward (i.e., west-to-east and back) calculation
// or TDMA_R for reverse (i.e., east-to-west and back) calculation
//    TDMA   won't tolerate RHOSFxx(1)=0   (i.e., AP(1)=0)
//  but TDMA_R won't tolerate RHOSBxx(N-1)=0 (i.e., AP(n)=0)
//  where n-1 refers to the outdoor layer (glazing or shading layer)

// This if-statement will catch the situation where RHOSFxx(1)=0.
// i.e., AP(1)=0.

	if (AP(1) < AP(N))
		TDMA_R( X, AP, AE, AW, BP, N);
	else
	{	// This "fix" (on the next line) is only used as a last resort
		// The if-statement will catch the very unusual situation where both
		// RHOSBxx(N-1)=0. and RHOSFxx(1)=0.
		if (AP(1) < 0.0001)
			AP(1)=0.0001;
		TDMA( X, AP, AE, AW, BP, N);
	}
}	// AutoTDMA
//-----------------------------------------------------------------------------
bool CFSTY::cf_Solar(		// layer absorption
	const CFSSWP* LSWP_ON,	// layer short wave properties
							//  [ 0]: outdoors (unused)
							//  [ 1 .. NL]: CFS equivalent layers
							//  [ NL+1]: room (generally black)
	double IBEAM,				// incident beam insolation (W/m2 aperture)
	double IDIFF,				// incident diffuse insolation (W/m2 aperture)
	double ILIGHTS,				// incident diffuse insolation (W/m2 aperture)
											//	 on inside surface (e.g., from lights)
	double* SOURCE)  		// returned: layer-by-layer flux of absorbed solar radiation, w/m2
							//    solar radiation (W/m2)
							// absL[ NL+1] is the flux of solar radiation
							//  absorbed in conditioned space (W/m2 aperture area)
//  Solar, specular/diffuse, multi-layer solution model
//    using combination net radiation method and TDMA solver

// RETURNS false on failure (invalid # of layers, )
//         true on success (results valid)
{

int iL;

	if (NL < 1)
		return false;
	bool bRet = true;

	// step one: the beam-beam analysis to find BPLUS and BMINUS
	A1D< 0> BPLUS, BMINUS;	// beam solar fluxes flowing in outward and inward directions
							//   correspond to Edwards QPLUS and QMINUS (except note
							//   reverse layer numbering)
	NetRad( NL, LSWP_ON, IBEAM, BPLUS, BMINUS);

#if defined( TEST_PRINT)
	for (iL=0; i<=NL; i++)
		Message( msgDBG,"%d   B+ = %0.3f   B- = %0.3f", iL, BPLUS(iL), BMINUS(iL));
#endif


	// step two: calculate the diffuse-caused-by-beam sources CPLUS and CMINUS
	A1D< 0> CPLUS, CMINUS;	// diffuse solar fluxes caused by BPLUS and BMINUS;
							//   appear as sources in diffuse calculation
	CPLUS( NL) = LSWP_ON[ NL+1].RHOSFBD * BMINUS( NL);
	for (iL=NL; iL>=1; iL--)	// march through layers, indoor to outdoor
	{	CPLUS( iL-1) = LSWP_ON[ iL].RHOSFBD*BMINUS( iL-1)
					 + LSWP_ON[ iL].TAUSBBD*BPLUS( iL);
		CMINUS( iL) =  LSWP_ON[ iL].RHOSBBD*BPLUS( iL)
					 + LSWP_ON[ iL].TAUSFBD*BMINUS( iL-1);
	}
	CMINUS(0) = 0.;

#if defined( TEST_PRINT)
	  for (iL=0; i<=NL; i++)
		Message( msgDBG,"%d   C+ = %0.3f   C- = %0.3f", iL, CPLUS(iL), CMINUS(iL));
#endif

	//  step three: diffuse fluxes, DPLUS and DMINUs,
	//  caused by diffuse incident, IDIFF on the outdoor side
	//  and by ILIGHTS on the indoor side, and by
	//  diffuse source (from beam) fluxes, CPLUS and CMINUS
	A1D< 0> DPLUS, DMINUS;	// diffuse solar fluxes flowing in outward and inward
							//   directions (W/m2)
	int N_TDMA = 2*NL;
	A1D< 1> AP, AE, AW, BP;
	for (iL=1; iL<=NL; iL++)
	{	int LINE = 2*iL-1;
		AP( LINE) = LSWP_ON[ iL].RHOSBDD;
		AE( LINE) = 1.0;
		if (LINE != 1)      // default
		{	AW( LINE) = -1.0 * LSWP_ON[ iL].TAUS_DD;
			BP( LINE) = -1.0 * CMINUS( iL);
		}
		else      //  special case at west-most node
		{	AW( 1) = 0.0;
			BP( 1) = -1.0*LSWP_ON[1].TAUS_DD*IDIFF - CMINUS(1);
		}

		LINE = 2*iL;
		AW( LINE) = 1.0;
		if (LINE != N_TDMA)      // default
		{   AP( LINE) = LSWP_ON[ iL+1].RHOSFDD;
			AE( LINE) = -1.0 * LSWP_ON[ iL+1].TAUS_DD;
			BP( LINE) = -1.0 * CPLUS( iL);
		}
		else          //  special case at east-most node
		{   AP( LINE) = LSWP_ON[ NL+1].RHOSFDD;
			BP( N_TDMA) = -1.0 * (CPLUS(NL)+ILIGHTS);
			AE( N_TDMA) = 0.;
		}
	}

	A1D< 1> X;
	AutoTDMA( X, AP, AE, AW, BP , N_TDMA);

	// unpack tdma solution vector
	for (iL=1; iL<=NL; iL++)
	{   DPLUS( iL)  = X( 2*iL-1);
		DMINUS( iL) = X( 2*iL);
	}

	// finish diffuse calculations
	DMINUS(0) = IDIFF;
	DPLUS(0)  = LSWP_ON[ 1].RHOSFDD*DMINUS(0)
			  + LSWP_ON[ 1].TAUS_DD*DPLUS(1)
			  + CPLUS(0);

#if defined( TEST_PRINT)
	for (iL=0; i<=NL; i++)
		Message( msgDBG,"%d   D+ = %0.3f   D- = %0.3f", iL, DPLUS(iL), DMINUS(iL));
#endif

	//  step four: absorbed solar radiation at each layer/node
	SOURCE[ NL+1] = BMINUS(NL)-BPLUS(NL)  // solar flux
				  + DMINUS(NL)-DPLUS(NL)  // transmitted to
				  + ILIGHTS;              // room

//  NOTE:  In calculating SOURCE(room) there is a trick included in the
//         previous line:  ILIGHTS is added because it is included
//         in DPLUS(NL) but ILIGHTS should not be included in this
//         type of calculation of SOURCE(i).  No similar adjustment
//         is needed for any of the other values of SOURCE(i)
//         As an alternative get the same result using:
//     SOURCE(NL+1) = BMINUS(NL)*(1.0 - SWP_ROOM.RHOSFBB - SWP_ROOM.RHOSFBD)
//                  + DMINUS(NL)*(1.0 - SWP_ROOM.RHOSFDD)
//         Take your pick
	for (iL=1; iL<=NL; iL++)
		SOURCE[ iL] = BPLUS( iL) - BMINUS( iL) - BPLUS( iL-1) + BMINUS( iL-1)
					+ DPLUS( iL) - DMINUS( iL) - DPLUS( iL-1) + DMINUS( iL-1);

//  CHECKSUM - ALL INCOMING SOLAR FLUX MUST GO SOMEWHERE, SHOULD EQUAL ZERO
	double CHKSUM = IBEAM + IDIFF + ILIGHTS - BPLUS(0) - DPLUS(0);
	for (iL=1;iL<=NL+1; iL++)
		CHKSUM -= SOURCE[ iL];
	if (vNEQ( CHKSUM, 0., .00001))
		Message( msgERR, "cf_Solar checksum = %0.5f (should be 0)", CHKSUM);

	return bRet;

}  // CFSTY::cf_Solar
//=============================================================================
bool CFSLAYER::cl_OffNormalProperties(		// off-normal properties

	double THETA,		// solar beam angle of incidence, from normal, radians
						//	0 <= THETA <= PI/2
	double OMEGA_V,		// solar beam vertical profile angle, +=above horizontal, radians
						//   = solar elevation angle for a vertical wall with
						//     wall-solar azimuth angle equal to zero
	double OMEGA_H,		// solar beam horizontal profile angle, +=clockwise when viewed
						//   from above (radians)
						//   = wall-solar azimuth angle for a vertical wall
						//     Used for PD and vertical VB
	CFSSWP& SWP_ON)	const	// returned: off-normal properties

//  Converts layer direct-normal, total solar, beam-beam and beam diffuse
//  properties to the corresponding off-normal properties
//  Uses: LTYPE, SWP_EL, geometry

// returns true iff success
{
	SWP_ON = SWP_EL;		// init to normal properties
							//  calls below modify in place

	bool bRet = cl_IsGlazeX()    ? SWP_ON.csw_SpecularOffNormal( THETA)
			: LTYPE == ltyVBHOR  ? cl_VBSWP( SWP_ON, OMEGA_V)
			: LTYPE == ltyVBVER  ? cl_VBSWP( SWP_ON, OMEGA_H)
			: LTYPE == ltyDRAPE  ? cl_PDSWP( SWP_ON, OMEGA_V, OMEGA_H)
			: LTYPE == ltyROLLB  ? cl_RBSWP( SWP_ON, THETA)
			: LTYPE == ltyINSCRN ? cl_ISSWP( SWP_ON, THETA)
			: /* LTYPE == ltyNONE || LTYPE == ltyROOM)*/  true;
	return bRet;
}    // CFSLAYER::cl_OffNormalProperties


//******************************************************************************
// common models (shared by more than one shade type)
//******************************************************************************
void OPENNESS_LW(	// long wave properties for shade types characterized by openness
					//   insect screen, roller blind, drape fabric
	double OPENNESS,	//  shade openness (=tausbb at normal incidence)
	double EPSLW0,		//  apparent LW emittance of shade at 0 openness
						//    (= wire or thread emittance)
						//    typical (default) values
						//       dark insect screen = .93
						//       metalic insect screen = .32
						//       roller blinds = .91
						//       drape fabric = .87
	double TAULW0,		//  apparent LW transmittance of shade at 0 openness
						//    typical (default) values
						//       dark insect screen = .02
						//       metalic insect screen = .19
						//       roller blinds = .05
						//       drape fabric = .05
	double& EPSLW,		//  returned: effective LW emittance of shade
	double& TAULW)		//  returned: effective LW transmittance of shade
{
	EPSLW = EPSLW0*(1.-OPENNESS);
	TAULW = TAULW0*(1.-OPENNESS) + OPENNESS;

}  // OPENNESS_LW

//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// RB (roller blind) models
///////////////////////////////////////////////////////////////////////////////
void RB_BEAM(	// roller blind off-normal properties

	double THETA,		//  angle of incidence, radians (0 - PI/2)
	double RHO_BT0,		//  normal incidence beam-total front reflectance
	double TAU_BT0,		//  normal incidence beam-total transmittance
						//    TAU_BT0 = TAU_BB0 + TAU_BD0
	double TAU_BB0,		//  normal incidence beam-beam transmittance
						//    (openness)
	double& RHO_BD,		//  returned: beam-diffuse front reflectance
	double& TAU_BB,		//  returned: beam-beam transmittance
	double& TAU_BD)		//  returned: beam-diffuse transmittance
// uses semi-emprical relations
{
	double TAU_BT = 0.;		// beam-total transmittance
	THETA = min( 89.99*DTOR, THETA);		// limit to < 90 deg
	if (TAU_BB0 > .9999)
		TAU_BB = TAU_BT = 1.;		// transparent (TAU_BD 0ed below)
	else
	{	//  apparent blind material transmittance at normal incidence
		double TAUM0 = min( 1., (TAU_BT0 - TAU_BB0) / (1.0-TAU_BB0));
		double TAUBT_EXPO = TAUM0 <= 0.33
							? .133 * pow( TAUM0 + .003, -.467)
							: .33 * (1. - TAUM0);
		//  beam total
		TAU_BT = TAU_BT0 * pow( cos( THETA), TAUBT_EXPO);	//  always 0 - 1
		//  cutoff angle, radians (angle beyond which total transmittance goes to zero)
		double THETA_CUTOFF = DTOR*(90. - 25. * cos( TAU_BB0 * PIOVER2));
		if (THETA >= THETA_CUTOFF)
			TAU_BB = 0.;
		else
		{	double TAUBB_EXPO = .6 * pow( cos( TAU_BB0 * PIOVER2),.3);
			TAU_BB = TAU_BB0 * pow( cos( PIOVER2*THETA/THETA_CUTOFF), TAUBB_EXPO);
			//  BB correlation can produce results slightly larger than BT
			//  enforce consistency
			TAU_BB = min( TAU_BT, TAU_BB);
		}
	}

	RHO_BD = RHO_BT0;
	TAU_BD = P01( TAU_BT-TAU_BB, "RB_BEAM TauBD");

}  // RB_BEAM
//-----------------------------------------------------------------------------
static double RB_F(	//  roller blind integrand
	double THETA,		//  incidence angle, radians
	const HEMINTP& P,	//  parameters
	[[maybe_unused]] int opt=0)			// unused options
{
	double RHO_BD, TAU_BB, TAU_BD;
	RB_BEAM( THETA, P.RHO_BT0, P.TAU_BT0, P.TAU_BB0,
		RHO_BD, TAU_BB, TAU_BD);
	return TAU_BB+TAU_BD;
} // RB_F
//-----------------------------------------------------------------------------
void RB_DIFF(		// roller blind diffuse-diffuse solar optical properties

	double RHO_BT0,	//  normal incidence beam-total reflectance
	double TAU_BT0,	//  normal incidence beam-total transmittance
					//    TAUFF_BT0 = TAUFF_BB0 + TAUFF_BD0
	double TAU_BB0,	//  normal incidence beam-beam transmittance
	double& RHO_DD,	//  returned: diffuse-diffuse reflectance
	double& TAU_DD)	//  returned: diffuse-diffuse transmittance

// integrates the corresponding properties over the hemisphere
{
	RHO_DD = RHO_BT0;
	HEMINTP P( RHO_BT0, TAU_BT0, TAU_BB0);
	TAU_DD = HEMINT( RB_F, P);
	if (RHO_DD + TAU_DD > 1.)
	{
#if defined( _DEBUG)
		if (RHO_DD + TAU_DD > 1.10)
			Message( msgWRN, "RB_DIFF inconsistent properties: TAU=%0.4  RHO=%0.4f", RHO_DD, TAU_DD);
#endif
		TAU_DD = 1. - RHO_DD;
	}
}  // RB_DIFF
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_RBLWP(	// roller blind LW properties
	CFSLWP& LLWP) const		// returned: equivalent layer long wave properties
{
	if (LTYPE != ltyROLLB)
		return false;

	bool bRet = true;
	double OPENNESS = SWP_MAT.TAUSFBB;

	OPENNESS_LW( OPENNESS, LWP_MAT.EPSLF, LWP_MAT.TAUL,
		LLWP.EPSLF, LLWP.TAUL);

	double TAULX;
	OPENNESS_LW( OPENNESS, LWP_MAT.EPSLB, LWP_MAT.TAUL,
		LLWP.EPSLB, TAULX);

#if defined( _DEBUG)
	if (abs( LLWP.TAUL - TAULX) > 0.001)
		Message( msgERR, "Layer '%s': RB LW TAU mismatch", FCGET( ID));
#endif
	return bRet;
}  // CFSLAYER::cl_RBLWP
//------------------------------------------------------------------------------------------
bool CFSLAYER::cl_RBSWP(		// roller blind short wave properties
	CFSSWP& LSWP,				// returned: equivalent layer properties set
								//   sets ONLY RHOSFDD, RHOSBDD, TAUS_DD
	double THETA/*=-999.*/) const	// incident angle, 0 <= theta <= PI/2
									//  if -999, derive diffuse properties
{
	if (LTYPE != ltyROLLB)
		return false;

	bool bRet = true;

	bool bDiffuse = THETA < -998.;

	// normal beam-total properties of fabric
	double RHOFF_BT0 = SWP_MAT.RHOSFBB + SWP_MAT.RHOSFBD;	// front rho
	double RHOBF_BT0 = SWP_MAT.RHOSBBB + SWP_MAT.RHOSBBD;	// back rho

	double TAUFF_BT0 = SWP_MAT.TAUSFBB + SWP_MAT.TAUSFBD;	// front tau
	double TAUBF_BT0 = SWP_MAT.TAUSBBB + SWP_MAT.TAUSBBD;	// back tau

	if (bDiffuse)
	{	// front
		RB_DIFF( RHOFF_BT0, TAUFF_BT0, SWP_MAT.TAUSFBB,
				LSWP.RHOSFDD, LSWP.TAUS_DD);
		// back
		double TAUX;
		RB_DIFF( RHOBF_BT0, TAUBF_BT0, SWP_MAT.TAUSBBB,
				LSWP.RHOSBDD, TAUX);
#if defined( _DEBUG)
		if (abs( LSWP.TAUS_DD - TAUX) > 0.001)
			Message( msgERR, "Layer '%s': roller blind SW DD TAU mismatch  F=%0.4f   B=%0.4f",
				FCGET( ID), LSWP.TAUS_DD, TAUX);
#endif
	}
	else
	{	RB_BEAM( THETA, RHOFF_BT0, TAUFF_BT0, SWP_MAT.TAUSFBB,
			LSWP.RHOSFBD, LSWP.TAUSFBB, LSWP.TAUSFBD);

		RB_BEAM( THETA, RHOBF_BT0, TAUBF_BT0, SWP_MAT.TAUSBBB,
			LSWP.RHOSBBD, LSWP.TAUSBBB, LSWP.TAUSBBD);
	}
	return bRet;
}  // CFSLAYER::cl_RBSWP
//==============================================================================================

////////////////////////////////////////////////////////////////////////////////////////////////
// Insect screen (IS) models
////////////////////////////////////////////////////////////////////////////////////////////////
static double IS_OPENNESS(		//  openness from wire geometry
	double D,			//  wire diameter
	double S)			//  wire spacing (note D-S = distance between wires)
// returns openness (0 - 1)
{
	double openness = S > 0.
			? Square( max( S-D, 0.) / S)
			: 0.;
	return openness;
} // IS_OPENNESS
//------------------------------------------------------------------------------
static double IS_DSRATIO(		//  wire geometry from openness
	double OPENNESS)	//  openness
// returns diameter / spacing ratio
{
	double dsRatio = OPENNESS > 0.
			? 1. - min( sqrt( OPENNESS), 1.)
			: 0.;
#if defined( _DEBUG)
	if (abs( IS_OPENNESS( dsRatio, 1.)-OPENNESS) > .00001)
		Message( msgERR, "IS_DSDATIO openness mismatch");
#endif
	return dsRatio;
} // IS_DSRATIO
//-----------------------------------------------------------------------------
static void IS_BEAM(	// insect screen BB and BD properties
	double THETA,		//  incidence angle, radians (0 - PI/2)
	double RHO_BT0,		//  beam-total reflectance
	double TAU_BT0,		//  beam-total transmittance at normal incidence
						//    TAU_BTO = TAU_BB0 + TAU_BD0
	double TAU_BB0,		//  beam-beam transmittance at normal incidence
	double& RHO_BD,		//  returned: beam-diffuse reflectance
	double& TAU_BB,		//  returned: beam-beam transmittance
	double& TAU_BD)		//  returned: beam-diffuse transmittance
// Calculates insect screen off-normal solar optical properties
//   using semi-empirical relations.
{
	THETA = min( 89.99*DTOR, THETA);
	double COSTHETA = cos( THETA);

	double RHO_W = RHO_BT0/max(.00001, 1.-TAU_BB0);
	double B = -.45 * log( max( RHO_W, .01));

	double RHO_BT90 = RHO_BT0 + (1. - RHO_BT0)*(.35 * RHO_W);

	RHO_BD = P01( RHO_BT0 + (RHO_BT90 - RHO_BT0) * (1.-pow( COSTHETA, B)), "IS_BEAM RhoBD");

	double TAU_BT = 0.;
	if (TAU_BT0 < .00001)
		TAU_BB = 0.;
	else
	{	double THETA_CUTOFF = acos( IS_DSRATIO( TAU_BB0));
		if (THETA >= THETA_CUTOFF)
			TAU_BB = 0.;
		else
		{	B = -.45*log( max( TAU_BB0, .01)) + .1;
			TAU_BB = P01( TAU_BB0 * pow( cos( PIOVER2*THETA/THETA_CUTOFF), B), "IS_BEAM TauBB");
		}

		double expB = -.65 * log( max( TAU_BT0, .01)) + .1;
		TAU_BT = P01( TAU_BT0 * pow( COSTHETA, expB), "IS_BEAM TauBT");
	}

	TAU_BD = P01( TAU_BT-TAU_BB, "IS_BEAM TauBD");

}  // IS_BEAM
//-----------------------------------------------------------------------------
static double IS_F(		//  insect screen integrand
	double THETA,		//  incidence angle, radians
	const HEMINTP& P,	// parameters
	int opt)			// options (what to calc)
{
	double RHO_BD, TAU_BB, TAU_BD;
	IS_BEAM( THETA, P.RHO_BT0, P.TAU_BT0, P.TAU_BB0,
			RHO_BD, TAU_BB, TAU_BD);

	double ret = opt == hipRHO ? RHO_BD
			   : opt == hipTAU ? TAU_BB+TAU_BD
			   :				 -1.;
	return ret;
} // IS_F
//-------------------------------------------------------------------------------
static void IS_DIFF(		// insect screen diffuse properties
	double RHO_BT0,	//  normal incidence beam-total reflectance
	double TAU_BT0,	//  normal incidence beam-total transmittance
					//    TAU_BT0 = TAU_BB0 + TAU_BD0
	double TAU_BB0,	//  normal incidence beam-beam transmittance
	double& RHO_DD,	//  returned: diffuse-diffuse reflectance
	double& TAU_DD)	//  returned: diffuse-diffuse transmittance
//  Calculates insect screen diffuse-diffuse solar optical properties by integrating
//  the corresponding properties over the hemisphere
{
	HEMINTP P( RHO_BT0, TAU_BT0, TAU_BB0);

	RHO_DD = HEMINT( IS_F, P, hipRHO);
	TAU_DD = HEMINT( IS_F, P, hipTAU);

	if (RHO_DD + TAU_DD > 1.)
	{	if (RHO_DD + TAU_DD > 1.10)
			Message( msgERR, "IS_DIFF inconsistent properties: TAU=%0.4f  RHO=%0.4f",
				RHO_DD, TAU_DD);
		TAU_DD = 1. - RHO_DD;
	}

}  // IS_DIFF
//=============================================================================
bool CFSLAYER::cl_ISLWP(		// insect screen LW properties
	CFSLWP& LLWP) const		// returned: equivalent layer long wave properties
{
	if (LTYPE != ltyINSCRN)
		return false;

	bool bRet = true;
	double OPENNESS = SWP_MAT.TAUSFBB;

	OPENNESS_LW( OPENNESS, LWP_MAT.EPSLF, LWP_MAT.TAUL, LLWP.EPSLF, LLWP.TAUL);

	double TAULX;
	OPENNESS_LW( OPENNESS, LWP_MAT.EPSLB, LWP_MAT.TAUL, LLWP.EPSLB, TAULX);

#if defined( _DEBUG)
	if (abs( LLWP.TAUL - TAULX) > 0.001)
		Message( msgWRN, "Layer '%s': IS LW TAU mismatch", FCGET( ID));
#endif
	return bRet;
}  // CFSLAYER::cl_ISLWP
//===============================================================================
bool CFSLAYER::cl_ISSWP(		// insect screen short wave properties
	CFSSWP& LSWP,	// returned: equivalent layer short wave properties
	double THETA/*=-999.*/)	const
							// incident angle, 0 <= theta <= PI/2
							//  if <0, derive diffuse properties
{
	if (LTYPE != ltyINSCRN)
		return false;

	bool bRet = true;

	bool bDiffuse = THETA < 0.;

	// normal beam-total properties
	double RHOFF_BT0 = SWP_MAT.RHOSFBB + SWP_MAT.RHOSFBD;	// front rho
	double RHOBF_BT0 = SWP_MAT.RHOSBBB + SWP_MAT.RHOSBBD;	// back rho

	double TAUFF_BT0 = SWP_MAT.TAUSFBB + SWP_MAT.TAUSFBD;	// front tau
	double TAUBF_BT0 = SWP_MAT.TAUSBBB + SWP_MAT.TAUSBBD;	// back tau

	if (bDiffuse)
	{   // front
		IS_DIFF( RHOFF_BT0, TAUFF_BT0, SWP_MAT.TAUSFBB,
				LSWP.RHOSFDD, LSWP.TAUS_DD);
		// back
		double TAUX;
		IS_DIFF( RHOBF_BT0, TAUBF_BT0, SWP_MAT.TAUSBBB,
				LSWP.RHOSBDD, TAUX);
	  #if defined( _DEBUG)
		if (abs( LSWP.TAUS_DD - TAUX) > 0.001)
			Message( msgWRN, "Layer '%s': Insect screen SW DD TAU mismatch", FCGET( ID));
	  #endif
	}
	else
	{	// front
		IS_BEAM( THETA, RHOFF_BT0, TAUFF_BT0, SWP_MAT.TAUSFBB,
			LSWP.RHOSFBD, LSWP.TAUSFBB, LSWP.TAUSFBD);

		// back -- call with reverse material properies
		IS_BEAM( THETA, RHOBF_BT0, TAUBF_BT0, SWP_MAT.TAUSBBB,
			LSWP.RHOSBBD, LSWP.TAUSBBB, LSWP.TAUSBBD);
	}
	return bRet;
}  // CFSLAYER::cl_ISSWP
//==============================================================================================

////////////////////////////////////////////////////////////////////////////////////////////////
// Pleated drape (PD) models and associated FM (fabric) methods
////////////////////////////////////////////////////////////////////////////////////////////////
static void FM_BEAM(
	double THETA,	//  incidence angle, radians (0 - PI/2)
	double RHO_BT0,	//  fabric beam-total reflectance
	double TAU_BT0,	//  fabric beam-total transmittance at normal incidence
									//    TAU_BTO = TAU_BB0 + TAU_BD0
	double TAU_BB0,	//  fabric beam-beam transmittance at normal incidence
									//    = openness
	double& RHO_BD,	//  returned: fabric beam-diffuse reflectance
	double& TAU_BB,	//  returned: fabric beam-beam transmittance
	double& TAU_BD)	//  returned: fabric beam-diffuse transmittance
//  Calculates the solar optical properties of a fabric for beam radiation incident on the forward facing surface
//  using optical properties at normal incidence and semi-empirical relations.
//  If you want the solar optical properties for the backward facing surface, call the subroutine again and supply it
//     with the corresponding backward facing surface optical properties at normal incidence.
{
	THETA = abs( max( -89.99*DTOR, min( 89.99*DTOR, THETA)));
					//  limit -89.99 - +89.99
					//  by symmetry, optical properties same at +/- theta
	double COSTHETA = cos( THETA);

	double RHO_Y = RHO_BT0/max( .00001, 1.0-TAU_BB0);
	double R = 0.7 * pow( RHO_Y, .7);
	double RHO_BT90 = RHO_BT0 + (1. - RHO_BT0)*R;
	double B = 0.6;
	RHO_BD = P01( RHO_BT0 + (RHO_BT90-RHO_BT0) * (1.0-pow(COSTHETA,B)), "FM_BEAM RhoBD");

	if (TAU_BT0 < .00001)
		TAU_BB = TAU_BD = 0.;
	else
	{	B = max( -.5 * log( max( TAU_BB0, .01)), .35);
		TAU_BB = TAU_BB0 * pow( COSTHETA, B);
		B = max( -.5 * log( max( TAU_BT0, .01)), .35);
		double TAU_BT = TAU_BT0 * pow( COSTHETA, B);
		TAU_BD = P01( TAU_BT - TAU_BB, "FM_BEAM TauBD");
	}
}  // FM_BEAM
//-------------------------------------------------------------------------------
static double FM_F(			//  fabric property integrand
	double THETA,		//  incidence angle, radians
	const HEMINTP& P,	// HEMINT parameters
	int opt)			// options: what property to return
{
	double RHO_BD, TAU_BB, TAU_BD;
	FM_BEAM( THETA, P.RHO_BT0, P.TAU_BT0, P.TAU_BB0,
			RHO_BD, TAU_BB, TAU_BD);

	double ret = opt == hipRHO ? RHO_BD
			   : opt == hipTAU ? TAU_BB+TAU_BD
			   :                 -1;
	return ret;
} // FM_F
//------------------------------------------------------------------------------
static void FM_DIFF(		// fabric diffuse properties
	double RHO_BT0,	//  fabric beam-total reflectance at normal incidence
	double TAU_BT0,	//  fabric beam-total transmittance at normal incidence
									//    (TAU_BT0 = TAU_BB0 + TAU_BD0)
	double TAU_BB0,	//  forward facing fabric beam-beam transmittance at normal incidence
	double& RHO_DD,	//  returned: fabric diffuse-diffuse reflectance
	double& TAU_DD)	//  returned: fabric diffuse-diffuse transmittance
//  Calculates fabric diffuse-diffuse solar optical properties by integrating
//    the corresponding beam properties over the hemisphere
{
	HEMINTP P( RHO_BT0, TAU_BT0, TAU_BB0);
	RHO_DD = HEMINT( FM_F, P, hipRHO);
	TAU_DD = HEMINT( FM_F, P, hipTAU);

	if (RHO_DD + TAU_DD > 1.)
	{	if (RHO_DD + TAU_DD > 1.10)
			Message( msgERR, "FM_DIFF inconsistent properties: TAU=%0.4f   RHO=%0.4f", RHO_DD, TAU_DD);
		TAU_DD = 1. - RHO_DD;
	}
}  // FM_DIFF
//-----------------------------------------------------------------------------
bool CFSSWP::csw_FabricEstimateDiffuseProps()	// estimate diffuse properties of drape fabric
// derives and sets RHOSFDD, RHOSBDD, TAUS_DD
{
	double RHOFF_BT0 = RHOSFBB + RHOSFBD;	// front rho
	double RHOBF_BT0 = RHOSBBB + RHOSBBD;	// back rho

	double TAUFF_BT0 = TAUSFBB + TAUSFBD;	// front tau
	double TAUBF_BT0 = TAUSBBB + TAUSBBD;	// back tau

	FM_DIFF( RHOFF_BT0, TAUFF_BT0, TAUSFBB, RHOSFDD, TAUS_DD);

	double TAUX;
	FM_DIFF( RHOBF_BT0, TAUBF_BT0, TAUSBBB, RHOSBDD, TAUX);

#if 0 && defined( _DEBUG)
	DbPrintf( "\nFEDP  RF0=%0.4f TF0=%0.4f TFBB=%0.4f  RB0=%0.4f TB0=%0.4f  TBBB=%0.4f\n  %s",
		RHOFF_BT0, TAUFF_BT0, TAUSFBB, RHOBF_BT0, TAUBF_BT0, TAUSBBB,
		DbFmt( "  "));
	if (abs( TAUS_DD - TAUX) > 0.001)
			Message( msgWRN, "Fabric SW DD TAU mismatch");
#endif
	return true;
}  // CFSSWP::csw_FabricEstimateDiffuseProps
//-----------------------------------------------------------------------------
static void PD_DIFF(		// Eight surface flat-fabric model with rectangular enclosure
	double S,			//  pleat spacing (> 0)
	double W,			//  pleat depth (>=0, same units as S)
	double RHOFF_DD,	//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,	//  fabric back diffuse-diffuse reflectance
	double TAUF_DD,		//  fabric diffuse-diffuse transmittance
	double& RHOFDD,		//  returned: drape diffuse-diffuse reflectance
	double& TAUFDD)		//  returned: drape diffuse-diffuse transmittance
//  Calculates the effective diffuse properties of a drapery layer.
//    Used for both LW and solar diffuse.
//  Returns the transmittance and the front-side reflectance of the drapery layer.
//  For back-side reflectance, call with reversed front and back properties
{
	if (W/S < SMALL_ERROR)
	{	//  flat drape (no pleats)
		RHOFDD = RHOFF_DD;
		TAUFDD = TAUF_DD;
		return;
	}

// SOLVE FOR DIAGONAL STRINGS AND SHAPE FACTORS
	double AK, CG;						//  length of diagonal strings of the rectangular enclosure
	AK = CG = sqrt (S*S + W*W);

	double F12, F14, F32, F21, F31, F34, F24, F41, F42;		//  shape factors
	double F57, F56, F58, F67, F65, F68, F75, F76, F78, F85, F87, F86;
	F12 = (S+W-AK)/(2.0*S);
	F14 = (S+W-CG)/(2.0*S);
	F32 = F14;
	F31 = (AK+CG-2.0*W)/(2.0*S);
	F34 = F12;
	F21 = (S+W-AK)/(2.0*W);
	F24 = (AK+CG-2.0*S)/(2.0*W);
	F41 = (S+W-CG)/(2.0*W);
	F42 = F24;
	F57 = F31;
	F56 = F12;
	F58 = F14;
	F75 = F31;
	F76 = F32;
	F78 = F34;
	F67 = F41;
	F65 = F21;
	F68 = F24;
	F85 = F41;
	F87 = F21;
	F86 = F42;

	// radiosity matrix
	A2D< 6, 8> A( 6, 8, 1, 1);
	A.Zero();

	A(1,1) = 1.0;
	A(1,2) = -RHOBF_DD*F12;
	A(1,3) = -RHOBF_DD*F14;
	A(1,4) = 0.0;
	A(1,5) = 0.0;
	A(1,6) = 0.0;
	A(1,7) = TAUF_DD;
	A(2,1) = -RHOBF_DD*F21;
	A(2,2) = 1.0;
	A(2,3) = -RHOBF_DD*F24;
	A(2,4) = -TAUF_DD*F87;
	A(2,5) = -TAUF_DD*F86;
	A(2,6) = 0.0;
	A(2,7) = TAUF_DD*F85;
	A(3,1) = -RHOBF_DD*F41;
	A(3,2) = -RHOBF_DD*F42;
	A(3,3) = 1.0;
	A(3,4) = -TAUF_DD*F67;
	A(3,5) = 0.0;
	A(3,6) = -TAUF_DD*F68;
	A(3,7) = TAUF_DD*F65;
	A(4,1) = 0.0;
	A(4,2) = 0.0;
	A(4,3) = 0.0;
	A(4,4) = 1.0;
	A(4,5) = -RHOFF_DD*F76;
	A(4,6) = -RHOFF_DD*F78;
	A(4,7) = RHOFF_DD*F75;
	A(5,1) = -TAUF_DD*F41;
	A(5,2) = -TAUF_DD*F42;
	A(5,3) = 0.0;
	A(5,4) = -RHOFF_DD*F67;
	A(5,5) = 1.0;
	A(5,6) = -RHOFF_DD*F68;
	A(5,7) = RHOFF_DD*F65;
	A(6,1) = -TAUF_DD*F21;
	A(6,2) = 0.0;
	A(6,3) = -TAUF_DD*F24;
	A(6,4) = -RHOFF_DD*F87;
	A(6,5) = -RHOFF_DD*F86;
	A(6,6) = 1.0;
	A(6,7) = RHOFF_DD*F85;

	A1D< 1> XSOL( 6);
	XSOL.Zero();

	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL( 1);
	double J2 = XSOL( 2);
	double J4 = XSOL( 3);
	double J7 = XSOL( 4);
	double J6 = XSOL( 5);
	double J8 = XSOL( 6);

	// irradiance, surface i
	double G1 = F12*J2+F14*J4;
	double G3 = F32*J2+F31*J1+F34*J4;
	double G5 = F57*J7+F56*J6+F58*J8;
	double G7 = F75+F76*J6+F78*J8;

	TAUFDD = P01( (G3+TAUF_DD*G7)/2., "PD_DIFF TauDD");
	RHOFDD = P01( (RHOFF_DD+TAUF_DD*G1+G5)/2.0, "PD_DIFF RhoDD");

}  // PD_DIFF
//-----------------------------------------------------------------------------
static void PD_LW(		// Pleated drape layer long wave effective properties
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	double OPENNESS_FABRIC,	//  fabric openness, 0-1 (=tausbb at normal incidence)
	double EPSLWF0_FABRIC,	//  fabric LW front emittance at 0 openness
							//     typical (default) = 0.92
	double EPSLWB0_FABRIC,	//  fabric LW back emittance at 0 openness
							//     typical (default) = 0.92
	double TAULW0_FABRIC,	//  fabric LW transmittance at 0 openness
							//     nearly always 0
	double& EPSLWF_PD,		//  returned: drape front effective LW emittance
	double& TAULW_PD)		//  returned: drape effective LW transmittance
//  Calculates the effective longwave properties of a drapery layer
//  Returns the front-side emittance and transmittance of the drapery layer.

//  If you want the back-side reflectance call the subroutine a second time with the same
//  input data - but interchange forward-facing and backward-facing properties
{
	double TAULW_FABRIC, EPSLWF_FABRIC, EPSLWB_FABRIC, TAULX, RHOLWF_PD;
	OPENNESS_LW( OPENNESS_FABRIC, EPSLWF0_FABRIC, TAULW0_FABRIC, EPSLWF_FABRIC, TAULW_FABRIC);
	OPENNESS_LW( OPENNESS_FABRIC, EPSLWB0_FABRIC, TAULW0_FABRIC, EPSLWB_FABRIC, TAULX);

	double RHOLWF_FABRIC = P01( 1. - EPSLWF_FABRIC - TAULW_FABRIC, "PD_LW RhoLWF");
	double RHOLWB_FABRIC = P01( 1. - EPSLWB_FABRIC - TAULW_FABRIC, "PD_LW RhoLWB");

	PD_DIFF( S, W, RHOLWF_FABRIC, RHOLWB_FABRIC, TAULW_FABRIC, RHOLWF_PD, TAULW_PD);

	EPSLWF_PD = P01( 1. - TAULW_PD - RHOLWF_PD, "PD_LW EpsLWF");

}  // PD_LW
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_I(		// pleated drape 14 surface flat-fabric model
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	[[maybe_unused]] double OMEGA_H,			//  horizontal profile angle, radians
	double DE,				//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	double RHOFF_BT_PERP, double TAUFF_BB_PERP, double TAUFF_BD_PERP,
	double RHOBF_BT_PERP, double TAUBF_BB_PERP, double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{
#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case I  omega=%0.2f", OMEGA_H/DTOR);
#endif

	// lengths of surfaces and diagonal strings
	double AB = DE;
	double GN = DE;
	double NP = DE;
	double GP = 2.0*DE;
	double NK = W-DE;
	double PK = W-2.0*DE;
	double BC = NK;
	double AN = sqrt(S*S+DE*DE);
	double AP = sqrt(S*S+GP*GP);
	double AK = sqrt(W*W+S*S);
	double BG = AN;
	double BP = AN;
	double CG = AK;
	double BK = sqrt(S*S+BC*BC);
	double CP = sqrt(S*S+PK*PK);
	double CN = sqrt(S*S+NK*NK);

	// beam and diffuse source terms due to incident beam radiation
	double TAUBF_BT_PERP = TAUBF_BD_PERP + TAUBF_BB_PERP;
	double Z1_BB = TAUFF_BB_PARL;
	double Z1_BD = TAUFF_BD_PARL;
	double Z2_BD = Z1_BB*RHOBF_BT_PERP*S/GN;
	double Z7_BB = TAUFF_BB_PERP*S/DE;
	double Z7_BD = TAUFF_BD_PERP*S/DE;
	double Z3_BD = Z7_BB*RHOBF_BT_PERP;
	double Z9_BD = RHOFF_BT_PERP*S/DE;
	double Z13_BD = Z7_BB*TAUBF_BT_PERP;
	double Z14_BD = Z1_BB*TAUBF_BT_PERP*S/GN;

	// shape factors
	double F12, F13, F14, F16, F17, F21, F25, F26, F27, F31, F35, F36, F37;
	double F41, F45, F46, F47, F51, F52, F53, F54, F56, F57, F61;
	double F62, F63, F64, F71, F72, F73, F74, F89, F810, F811, F812, F813;
	double F814, F911, F912, F913, F914, F1011, F1012, F1013, F1014;
	double F119, F1110, F1112, F1113, F1114, F129, F1210, F1211, F139, F1310, F1311, F149, F1410, F1411;
	F12 = (S+GN-AN)/(2.0*S);
	F13 = (AN+GP-(GN+AP))/(2.0*S);
	F14 = (AP+W-(GP+AK))/(2.0*S);
	F16 = (W+BG-(AB+CG))/(2.0*S);
	F17 = (S+AB-BG)/(2.0*S);
	F21 = (S+GN-AN)/(2.0*GN);
	F25 = (W+CN-(CG+NK))/(2.0*GN);
	F26 = (CG+S-(BG+CN))/(2.0*GN);
	F27 = (AN+BG-2.0*S)/(2.0*GN);
	F31 = (AN+GP-(GN+AP))/(2.0*NP);
	F35 = (NK+CP-(CN+PK))/(2.0*NP);
	F36 = (CN+BP-(S+CP))/(2.0*NP);
	F37 = (S+AP-(AN+BP))/(2.0*NP);
	F41 = (W+AP-(GP+AK))/(2.0*PK);
	F45 = (S+PK-CP)/(2.0*PK);
	F46 = (CP+BK-(S+BP))/(2.0*PK);
	F47 = (BP+AK-(AP+BK))/(2.0*PK);
	F51 = (AK+CG-2.0*W)/(2.0*S);
	F52 = (W+CN-(CG+NK))/(2.0*S);
	F53 = (NK+CP-(CN+PK))/(2.0*S);
	F54 = (S+PK-CP)/(2.0*S);
	F56 = (S+BC-BK)/(2.0*S);
	F57 = (W+BK-(BC+AK))/(2.0*S);
	F61 = (W+BG-(AB+CG))/(2.0*BC);
	F62 = (S+CG-(BG+CN))/(2.0*BC);
	F63 = (CN+BP-(S+CP))/(2.0*BC);
	F64 = (BK+CP-(S+BP))/(2.0*BC);
	F71 = F21;
	F72 = F27;
	F73 = F37;
	F74 = (BP+AK-(BK+AP))/(2.0*AB);
	F89 = F12;
	F810 = F16;
	F811 = F51;
	F812 = F14;
	F813 = F13;
	F814 = F12;
	F911 = F25;
	F912 = F74;
	F913 = F73;
	F914 = F27;
	F1011 = (BC+S-BK)/(2.0*BC);
	F1012 = F64;
	F1013 = F63;
	F1014 = F62;
	F119 = F57;
	F1110 = F56;
	F1112 = F54;
	F1113 = F53;
	F1114 = F52;
	F129 = F47;
	F1210 = F46;
	F1211 = F45;
	F139 = F37;
	F1310 = F36;
	F1311 = F35;
	F149 = F27;
	F1410 = F26;
	F1411 = F25;

	// radiosity matrix
	A2D< 12, 14> A( 12, 14, 1, 1);
	A.Zero();		//  INITIALIZE RADIOSITY MATRIX COEFFICIENTS
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F13;
	A(1,4)	= -RHOBF_DD*F14;
	A(1,5)	= -RHOBF_DD*F16;
	A(1,6)	= -RHOBF_DD*F17;
	A(1,7)	= 0.0;
	A(1,8)	= 0.0;
	A(1,9)	= 0.0;
	A(1,10)	= 0.0;
	A(1,11)	= 0.0;
	A(1,12)	= 0.0;
	A(1,13)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= 0.0;
	A(2,4)	= 0.0;
	A(2,5)	= -RHOBF_DD*F26;
	A(2,6)	= -RHOBF_DD*F27;
	A(2,7)	= -TAUFF_DD*F149;
	A(2,8)	= -TAUFF_DD*F1410;
	A(2,9)	= -TAUFF_DD*F1411;
	A(2,10)	= 0.0;
	A(2,11)	= 0.0;
	A(2,12)	= 0.0;
	A(2,13)	= Z2_BD;
	A(3,1)	= -RHOBF_DD*F31;
	A(3,2)	= 0.0;
	A(3,3)	= 1.0;
	A(3,4)	= 0.0;
	A(3,5)	= -RHOBF_DD*F36;
	A(3,6)	= -RHOBF_DD*F37;
	A(3,7)	= -TAUFF_DD*F139;
	A(3,8)	= -TAUFF_DD*F1310;
	A(3,9)	= -TAUFF_DD*F1311;
	A(3,10)	= 0.0;
	A(3,11)	= 0.0;
	A(3,12)	= 0.0;
	A(3,13)	= Z3_BD;
	A(4,1)	= -RHOBF_DD*F41;
	A(4,2)	= 0.0;
	A(4,3)	= 0.0;
	A(4,4)	= 1.0;
	A(4,5)	= -RHOBF_DD*F46;
	A(4,6)	= -RHOBF_DD*F47;
	A(4,7)	= -TAUFF_DD*F129;
	A(4,8)	= -TAUFF_DD*F1210;
	A(4,9)	= -TAUFF_DD*F1211;
	A(4,10)	= 0.0;
	A(4,11)	= 0.0;
	A(4,12)	= 0.0;
	A(4,13)	= 0.0;
	A(5,1)	= -RHOBF_DD*F61;
	A(5,2)	= -RHOBF_DD*F62;
	A(5,3)	= -RHOBF_DD*F63;
	A(5,4)	= -RHOBF_DD*F64;
	A(5,5)	= 1.0;
	A(5,6)	= 0.0;
	A(5,7)	= 0.0;
	A(5,8)	= 0.0;
	A(5,9)	= -TAUFF_DD*F1011;
	A(5,10)	= -TAUFF_DD*F1012;
	A(5,11)	= -TAUFF_DD*F1013;
	A(5,12)	= -TAUFF_DD*F1014;
	A(5,13)	= 0.0;
	A(6,1)	= -RHOBF_DD*F71;
	A(6,2)	= -RHOBF_DD*F72;
	A(6,3)	= -RHOBF_DD*F73;
	A(6,4)	= -RHOBF_DD*F74;
	A(6,5)	= 0.0;
	A(6,6)	= 1.0;
	A(6,7)	= 0.0;
	A(6,8)	= 0.0;
	A(6,9)	= -TAUFF_DD*F911;
	A(6,10)	= -TAUFF_DD*F912;
	A(6,11)	= -TAUFF_DD*F913;
	A(6,12)	= -TAUFF_DD*F914;
	A(6,13)	= Z7_BD;
	A(7,1)	= -TAUBF_DD*F71;
	A(7,2)	= -TAUBF_DD*F72;
	A(7,3)	= -TAUBF_DD*F73;
	A(7,4)	= -TAUBF_DD*F74;
	A(7,5)	= 0.0;
	A(7,6)	= 0.0;
	A(7,7)	= 1.0;
	A(7,8)	= 0.0;
	A(7,9)	= -RHOFF_DD*F911;
	A(7,10)	= -RHOFF_DD*F912;
	A(7,11)	= -RHOFF_DD*F913;
	A(7,12)	= -RHOFF_DD*F914;
	A(7,13)	= Z9_BD;
	A(8,1)	= -TAUBF_DD*F61;
	A(8,2)	= -TAUBF_DD*F62;
	A(8,3)	= -TAUBF_DD*F63;
	A(8,4)	= -TAUBF_DD*F64;
	A(8,5)	= 0.0;
	A(8,6)	= 0.0;
	A(8,7)	= 0.0;
	A(8,8)	= 1.0;
	A(8,9)	= -RHOFF_DD*F1011;
	A(8,10)	= -RHOFF_DD*F1012;
	A(8,11)	= -RHOFF_DD*F1013;
	A(8,12)	= -RHOFF_DD*F1014;
	A(8,13)	= 0.0;
	A(9,1)	= 0.0;
	A(9,2)	= 0.0;
	A(9,3)	= 0.0;
	A(9,4)	= 0.0;
	A(9,5)	= 0.0;
	A(9,6)	= 0.0;
	A(9,7)	= -RHOFF_DD*F119;
	A(9,8)	= -RHOFF_DD*F1110;
	A(9,9)	= 1.0;
	A(9,10)	= -RHOFF_DD*F1112;
	A(9,11)	= -RHOFF_DD*F1113;
	A(9,12)	= -RHOFF_DD*F1114;
	A(9,13)	= 0.0;
	A(10,1)	= -TAUBF_DD*F41;
	A(10,2)	= 0.0;
	A(10,3)	= 0.0;
	A(10,4)	= 0.0;
	A(10,5)	= -TAUBF_DD*F46;
	A(10,6)	= -TAUBF_DD*F47;
	A(10,7)	= -RHOFF_DD*F129;
	A(10,8)	= -RHOFF_DD*F1210;
	A(10,9)	= -RHOFF_DD*F1211;
	A(10,10) = 1.0;
	A(10,11)	= 0.0;
	A(10,12)	= 0.0;
	A(10,13)	= 0.0;
	A(11,1)	= -TAUBF_DD*F31;
	A(11,2)	= 0.0;
	A(11,3)	= 0.0;
	A(11,4)	= 0.0;
	A(11,5)	= -TAUBF_DD*F36;
	A(11,6)	= -TAUBF_DD*F37;
	A(11,7)	= -RHOFF_DD*F139;
	A(11,8)	= -RHOFF_DD*F1310;
	A(11,9)	= -RHOFF_DD*F1311;
	A(11,10)	= 0.0;
	A(11,11)	= 1.0;
	A(11,12)	= 0.0;
	A(11,13)	= Z13_BD;
	A(12,1)	= -TAUBF_DD*F21;
	A(12,2)	= 0.0;
	A(12,3)	= 0.0;
	A(12,4)	= 0.0;
	A(12,5)	= -TAUBF_DD*F26;
	A(12,6)	= -TAUBF_DD*F27;
	A(12,7)	= -RHOFF_DD*F149;
	A(12,8)	= -RHOFF_DD*F1410;
	A(12,9)	= -RHOFF_DD*F1411;
	A(12,10)	= 0.0;
	A(12,11)	= 0.0;
	A(12,12)	= 1.0;
	A(12,13)	= Z14_BD;

	A1D< 1> XSOL( 14);
	XSOL.Zero();

	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J3 = XSOL(3);
	double J4 = XSOL(4);
	double J6 = XSOL(5);
	double J7 = XSOL(6);
	double J9 = XSOL(7);
	double J10 = XSOL(8);
	double J11 = XSOL(9);
	double J12 = XSOL(10);
	double J13 = XSOL(11);
	double J14 = XSOL(12);

	// irradiance, surface i
	double G1 = F12*J2+F13*J3+F14*J4+F16*J6+F17*J7;
	double G5 = F56*J6+F57*J7+F51*J1+F52*J2+F53*J3+F54*J4;
	double G8 = F89*J9+F810*J10+F811*J11+F812*J12+F813*J13+F814*J14;
	double G11 = F1112*J12+F1113*J13+F1114*J14+F119*J9+F1110*J10;

	TAU_BB = 0.0;
	TAU_BD = (G5+TAUFF_DD*G11)/2.0;
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G8)/2.0;

}  // PD_BEAM_CASE_I
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_II(		// pleated drape 14 surface flat-fabric model, case II
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	[[maybe_unused]] double OMEGA_H,	//  horizontal profile angle, radians
	double DE,				//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	double RHOFF_BT_PERP, double TAUFF_BB_PERP, double TAUFF_BD_PERP,
	double RHOBF_BT_PERP, double TAUBF_BB_PERP, double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{
#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case II  omega=%0.2f", OMEGA_H/DTOR);
#endif

	// lengths of surfaces and diagonal strings
	double AB = DE;
	double GN = DE;
	double NK = W-DE;
	double BC = NK;
	double AN = sqrt(S*S+DE*DE);
	double AK = sqrt(W*W+S*S);
	double BG = AN;
	double CG = AK;
	double BK = sqrt(S*S+BC*BC);
	double CN = sqrt(S*S+NK*NK);

	//  diffuse (BD) and beam (BB) source terms due to incident beam radiation
	double TAUBF_BT_PERP = TAUBF_BD_PERP + TAUBF_BB_PERP;
	double Z1_BB = TAUFF_BB_PARL;
	double Z1_BD = TAUFF_BD_PARL;
	double Z2_BD = Z1_BB*RHOBF_BT_PERP*S/GN;
	double Z6_BB = TAUFF_BB_PERP*S/DE;
	double Z6_BD = TAUFF_BD_PERP*S/DE;
	double Z3_BD = Z6_BB*RHOBF_BT_PERP;
	double Z8_BD = RHOFF_BT_PERP*S/DE;
	double Z11_BD = Z6_BB*TAUBF_BT_PERP;
	double Z12_BD = Z1_BB*TAUBF_BT_PERP*S/GN;

	// shape factors
	double F12, F13, F15, F16, F21, F25, F26, F31, F35, F36, F41, F42, F43, F45, F46;
	double F51, F52, F53, F54, F61, F62, F63;
	double F78, F79, F710, F711, F712, F810, F811, F812, F910, F911;
	double F912, F108, F109, F1011, F1012, F118, F119, F1110, F128, F129, F1210;
	F12 = (S+GN-AN)/(2.0*S);
	F13 = (W+AN-(GN+AK))/(2.0*S);
	F15 = (W+BG-(AB+CG))/(2.0*S);
	F16 = (S+AB-BG)/(2.0*S);
	F21 = (S+GN-AN)/(2.0*GN);
	F25 = (S+CG-(BG+CN))/(2.0*GN);
	F26 = (AN+BG-2.0*S)/(2.0*GN);
	F31 = (W+AN-(GN+AK))/(2.0*NK);
	F35 = (BK+CN-2.0*S)/(2.0*NK);
	F36 = (S+AK-(AN+BK))/(2.0*NK);
	F41 = (AK+CG-2.0*W)/(2.0*S);
	F42 = (W+CN-(CG+NK))/(2.0*S);
	F43 = (S+NK-CN)/(2.0*S);
	F45 = (S+BC-BK)/(2.0*S);
	F46 = (W+BK-(AK+BC))/(2.0*S);
	F51 = (W+BG-(AB+CG))/(2.0*BC);
	F52 = (S+CG-(BG+CN))/(2.0*BC);
	F53 = (BK+CN-2.0*S)/(2.0*BC);
	F54 = (S+BC-BK)/(2.0*BC);
	F61 = (S+AB-BG)/(2.0*AB);
	F62 = (AN+BG-2.0*S)/(2.0*AB);
	F63 = (S+AK-(AN+BK))/(2.0*AB);
	F78 = F12;
	F79 = F13;
	F710 = (AK+CG-2.0*W)/(2.0*S);
	F711 = F15;
	F712 = F16;
	F810 = (W+CN-(CG+NK))/(2.0*S);
	F811 = F25;
	F812 = F26;
	F910 = (S+NK-CN)/(2.0*NK);
	F911 = F35;
	F912 = F36;
	F108 = F42;
	F109 = F43;
	F1011 = F45;
	F1012 = F46;
	F118 = F52;
	F119 = F53;
	F1110 = (S+BC-BK)/(2.0*NK);
	F128 = F62;
	F129 = F63;
	F1210 = (W+BK-(AK+BC))/(2.0*GN);


//-----------------------------------------------------------------------------------

	// radiosity matrix
	A2D< 10, 12> A( 10, 12, 1, 1);
	A.Zero();
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F13;
	A(1,4)	= -RHOBF_DD*F15;
	A(1,5)	= -RHOBF_DD*F16;
	A(1,6)	= 0.0;
	A(1,7)	= 0.0;
	A(1,8)	= 0.0;
	A(1,9)	= 0.0;
	A(1,10)	= 0.0;
	A(1,11)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= 0.0;
	A(2,4)	= -RHOBF_DD*F25;
	A(2,5)	= -RHOBF_DD*F26;
	A(2,6)	= -TAUFF_DD*F128;
	A(2,7)	= -TAUFF_DD*F129;
	A(2,8)	= -TAUFF_DD*F1210;
	A(2,9)	= 0.0;
	A(2,10)	= 0.0;
	A(2,11)	= Z2_BD;
	A(3,1)	= -RHOBF_DD*F31;
	A(3,2)	= 0.0;
	A(3,3)	= 1.0;
	A(3,4)	= -RHOBF_DD*F35;
	A(3,5)	= -RHOBF_DD*F36;
	A(3,6)	= -TAUFF_DD*F118;
	A(3,7)	= -TAUFF_DD*F119;
	A(3,8)	= -TAUFF_DD*F1110;
	A(3,9)	= 0.0;
	A(3,10)	= 0.0;
	A(3,11)	= Z3_BD;
	A(4,1)	= -RHOBF_DD*F51;
	A(4,2)	= -RHOBF_DD*F52;
	A(4,3)	= -RHOBF_DD*F53;
	A(4,4)	= 1.0;
	A(4,5)	= 0.0;
	A(4,6)	= 0.0;
	A(4,7)	= 0.0;
	A(4,8)	= -TAUFF_DD*F910;
	A(4,9)	= -TAUFF_DD*F911;
	A(4,10)	= -TAUFF_DD*F912;
	A(4,11)	= 0.0;
	A(5,1)	= -RHOBF_DD*F61;
	A(5,2)	= -RHOBF_DD*F62;
	A(5,3)	= -RHOBF_DD*F63;
	A(5,4)	= 0.0;
	A(5,5)	= 1.0;
	A(5,6)	= 0.0;
	A(5,7)	= 0.0;
	A(5,8)	= -TAUFF_DD*F810;
	A(5,9)	= -TAUFF_DD*F811;
	A(5,10)	= -TAUFF_DD*F812;
	A(5,11)	= Z6_BD;
	A(6,1)	= -TAUBF_DD*F61;
	A(6,2)	= -TAUBF_DD*F62;
	A(6,3)	= -TAUBF_DD*F63;
	A(6,4)	= 0.0;
	A(6,5)	= 0.0;
	A(6,6)	= 1.0;
	A(6,7)	= 0.0;
	A(6,8)	= -RHOFF_DD*F810;
	A(6,9)	= -RHOFF_DD*F811;
	A(6,10)	= -RHOFF_DD*F812;
	A(6,11)	= Z8_BD;
	A(7,1)	= -TAUBF_DD*F51;
	A(7,2)	= -TAUBF_DD*F52;
	A(7,3)	= -TAUBF_DD*F53;
	A(7,4)	= 0.0;
	A(7,5)	= 0.0;
	A(7,6)	= 0.0;
	A(7,7)	= 1.0;
	A(7,8)	= -RHOFF_DD*F910;
	A(7,9)	= -RHOFF_DD*F911;
	A(7,10)	= -RHOFF_DD*F912;
	A(7,11)	= 0.0;
	A(8,1)	= 0.0;
	A(8,2)	= 0.0;
	A(8,3)	= 0.0;
	A(8,4)	= 0.0;
	A(8,5)	= 0.0;
	A(8,6)	= -RHOFF_DD*F108;
	A(8,7)	= -RHOFF_DD*F109;
	A(8,8)	= 1.0;
	A(8,9)	= -RHOFF_DD*F1011;
	A(8,10)	= -RHOFF_DD*F1012;
	A(8,11)	= 0.0;
	A(9,1)	= -TAUBF_DD*F31;
	A(9,2)	= 0.0;
	A(9,3)	= 0.0;
	A(9,4)	= -TAUBF_DD*F35;
	A(9,5)	= -TAUBF_DD*F36;
	A(9,6)	= -RHOFF_DD*F118;
	A(9,7)	= -RHOFF_DD*F119;
	A(9,8)	= -RHOFF_DD*F1110;
	A(9,9)	= 1.0;
	A(9,10)	= 0.0;
	A(9,11)	= Z11_BD;
	A(10,1)	= -TAUBF_DD*F21;
	A(10,2)	= 0.0;
	A(10,3)	= 0.0;
	A(10,4)	= -TAUBF_DD*F25;
	A(10,5)	= -TAUBF_DD*F26;
	A(10,6)	= -RHOFF_DD*F128;
	A(10,7)	= -RHOFF_DD*F129;
	A(10,8)	= -RHOFF_DD*F1210;
	A(10,9)	= 0.0;
	A(10,10)	= 1.0;
	A(10,11)	= Z12_BD;

	A1D< 1> XSOL( 10);
	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J3 = XSOL(3);
	double J5 = XSOL(4);
	double J6 = XSOL(5);
	double J8 = XSOL(6);
	double J9 = XSOL(7);
	double J10 = XSOL(8);
	double J11 = XSOL(9);
	double J12 = XSOL(10);

	// irradiance, surface i
	double G1 = F12*J2+F13*J3+F15*J5+F16*J6;
	double G4 = F41*J1+F42*J2+F43*J3+F45*J5+F46*J6;
	double G7 = F78*J8+F79*J9+F710*J10+F711*J11+F712*J12;
	double G10 = F108*J8+F109*J9+F1011*J11+F1012*J12;

	TAU_BB = 0.0;
	TAU_BD = (G4+TAUFF_DD*G10)/2.0;
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G7)/2.0;

}  // PD_BEAM_CASE_II
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_III(		// pleated drape 14 surface flat-fabric model, case III
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	double OMEGA_H,			//  horizontal profile angle, radians
	double DE,				//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	double RHOFF_BT_PERP, double TAUFF_BB_PERP, double TAUFF_BD_PERP,
	double RHOBF_BT_PERP, double TAUBF_BB_PERP, double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{

#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case III  omega=%0.2f", OMEGA_H/DTOR);
#endif

	// lengths of surfaces and diagonal strings
	double AB = DE;
	double GN = DE;
	double NK = W-DE;
	double BC = NK;
	double AN = sqrt(S*S+DE*DE);
	double AK = sqrt(W*W+S*S);
	double BG = AN;
	double CG = AK;
	double BK = sqrt(S*S+BC*BC);
	double CN = sqrt(S*S+NK*NK);

	// diffuse and beam source terms
	double TAUBF_BT_PERP = TAUBF_BD_PERP + TAUBF_BB_PERP;
	double Z1_BB = TAUFF_BB_PARL;
	double Z1_BD = TAUFF_BD_PARL;
	double Z2_BD = Z1_BB*RHOBF_BT_PERP*S/GN;
	double Z6_BB = TAUFF_BB_PERP*S/DE;
	double Z6_BD = TAUFF_BD_PERP*S/DE;
	double Z3_BD = Z6_BB*RHOBF_BT_PERP;
	double Z8_BD = RHOFF_BT_PERP*S/DE;
	double Z11_BD = Z6_BB*TAUBF_BT_PERP;
	double Z12_BD = Z1_BB*TAUBF_BT_PERP*S/GN;

	// shape factors
	double F12, F13, F15, F16, F21, F25, F26, F31, F35, F36, F41, F42, F43, F45, F46;
	double F51, F52, F53, F54, F61, F62, F63;
	double F78, F79, F710, F711, F712, F810, F811, F812, F910, F911, F912;
	double F108, F109, F1011, F1012, F118, F119, F1110, F128, F129, F1210;

	F12 = (S+GN-AN)/(2.0*S);
	F13 = (W+AN-(GN+AK))/(2.0*S);
	F15 = (W+BG-(AB+CG))/(2.0*S);
	F16 = (S+AB-BG)/(2.0*S);
	F21 = (S+GN-AN)/(2.0*GN);
	F25 = (S+CG-(BG+CN))/(2.0*GN);
	F26 = (AN+BG-2.0*S)/(2.0*GN);
	F31 = (W+AN-(GN+AK))/(2.0*NK);
	F35 = (BK+CN-2.0*S)/(2.0*NK);
	F36 = (S+AK-(AN+BK))/(2.0*NK);
	F41 = (AK+CG-2.0*W)/(2.0*S);
	F42 = (W+CN-(CG+NK))/(2.0*S);
	F43 = (S+NK-CN)/(2.0*S);
	F45 = (S+BC-BK)/(2.0*S);
	F46 = (W+BK-(AK+BC))/(2.0*S);
	F51 = (W+BG-(AB+CG))/(2.0*BC);
	F52 = (S+CG-(BG+CN))/(2.0*BC);
	F53 = (BK+CN-2.0*S)/(2.0*BC);
	F54 = (S+BC-BK)/(2.0*BC);
	F61 = (S+AB-BG)/(2.0*AB);
	F62 = (AN+BG-2.0*S)/(2.0*AB);
	F63 = (S+AK-(AN+BK))/(2.0*AB);
	F78 = F12;
	F79 = F13;
	F710 = (AK+CG-2.0*W)/(2.0*S);
	F711 = F15;
	F712 = F16;
	F810 = (W+CN-(CG+NK))/(2.0*S);
	F811 = F25;
	F812 = F26;
	F910 = (S+NK-CN)/(2.0*NK);
	F911 = F35;
	F912 = F36;
	F108 = F42;
	F109 = F43;
	F1011 = F45;
	F1012 = F46;
	F118 = F52;
	F119 = F53;
	F1110 = (S+BC-BK)/(2.0*NK);
	F128 = F62;
	F129 = F63;
	F1210 = (W+BK-(AK+BC))/(2.0*GN);

	// radiosity matrix
	A2D< 10, 12> A( 10, 12, 1, 1);
	A.Zero();
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F13;
	A(1,4)	= -RHOBF_DD*F15;
	A(1,5)	= -RHOBF_DD*F16;
	A(1,6)	= 0.0;
	A(1,7)	= 0.0;
	A(1,8)	= 0.0;
	A(1,9)	= 0.0;
	A(1,10)	= 0.0;
	A(1,11)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= 0.0;
	A(2,4)	= -RHOBF_DD*F25;
	A(2,5)	= -RHOBF_DD*F26;
	A(2,6)	= -TAUFF_DD*F128;
	A(2,7)	= -TAUFF_DD*F129;
	A(2,8)	= -TAUFF_DD*F1210;
	A(2,9)	= 0.0;
	A(2,10)	= 0.0;
	A(2,11)	= Z2_BD;
	A(3,1)	= -RHOBF_DD*F31;
	A(3,2)	= 0.0;
	A(3,3)	= 1.0;
	A(3,4)	= -RHOBF_DD*F35;
	A(3,5)	= -RHOBF_DD*F36;
	A(3,6)	= -TAUFF_DD*F118;
	A(3,7)	= -TAUFF_DD*F119;
	A(3,8)	= -TAUFF_DD*F1110;
	A(3,9)	= 0.0;
	A(3,10)	= 0.0;
	A(3,11)	= Z3_BD;
	A(4,1)	= -RHOBF_DD*F51;
	A(4,2)	= -RHOBF_DD*F52;
	A(4,3)	= -RHOBF_DD*F53;
	A(4,4)	= 1.0;
	A(4,5)	= 0.0;
	A(4,6)	= 0.0;
	A(4,7)	= 0.0;
	A(4,8)	= -TAUFF_DD*F910;
	A(4,9)	= -TAUFF_DD*F911;
	A(4,10)	= -TAUFF_DD*F912;
	A(4,11)	= 0.0;
	A(5,1)	= -RHOBF_DD*F61;
	A(5,2)	= -RHOBF_DD*F62;
	A(5,3)	= -RHOBF_DD*F63;
	A(5,4)	= 0.0;
	A(5,5)	= 1.0;
	A(5,6)	= 0.0;
	A(5,7)	= 0.0;
	A(5,8)	= -TAUFF_DD*F810;
	A(5,9)	= -TAUFF_DD*F811;
	A(5,10)	= -TAUFF_DD*F812;
	A(5,11)	= Z6_BD;
	A(6,1)	= -TAUBF_DD*F61;
	A(6,2)	= -TAUBF_DD*F62;
	A(6,3)	= -TAUBF_DD*F63;
	A(6,4)	= 0.0;
	A(6,5)	= 0.0;
	A(6,6)	= 1.0;
	A(6,7)	= 0.0;
	A(6,8)	= -RHOFF_DD*F810;
	A(6,9)	= -RHOFF_DD*F811;
	A(6,10)	= -RHOFF_DD*F812;
	A(6,11)	= Z8_BD;
	A(7,1)	= -TAUBF_DD*F51;
	A(7,2)	= -TAUBF_DD*F52;
	A(7,3)	= -TAUBF_DD*F53;
	A(7,4)	= 0.0;
	A(7,5)	= 0.0;
	A(7,6)	= 0.0;
	A(7,7)	= 1.0;
	A(7,8)	= -RHOFF_DD*F910;
	A(7,9)	= -RHOFF_DD*F911;
	A(7,10)	= -RHOFF_DD*F912;
	A(7,11)	= 0.0;
	A(8,1)	= 0.0;
	A(8,2)	= 0.0;
	A(8,3)	= 0.0;
	A(8,4)	= 0.0;
	A(8,5)	= 0.0;
	A(8,6)	= -RHOFF_DD*F108;
	A(8,7)	= -RHOFF_DD*F109;
	A(8,8)	= 1.0;
	A(8,9)	= -RHOFF_DD*F1011;
	A(8,10)	= -RHOFF_DD*F1012;
	A(8,11)	= 0.0;
	A(9,1)	= -TAUBF_DD*F31;
	A(9,2)	= 0.0;
	A(9,3)	= 0.0;
	A(9,4)	= -TAUBF_DD*F35;
	A(9,5)	= -TAUBF_DD*F36;
	A(9,6)	= -RHOFF_DD*F118;
	A(9,7)	= -RHOFF_DD*F119;
	A(9,8)	= -RHOFF_DD*F1110;
	A(9,9)	= 1.0;
	A(9,10)	= 0.0;
	A(9,11)	= Z11_BD;
	A(10,1)	= -TAUBF_DD*F21;
	A(10,2)	= 0.0;
	A(10,3)	= 0.0;
	A(10,4)	= -TAUBF_DD*F25;
	A(10,5)	= -TAUBF_DD*F26;
	A(10,6)	= -RHOFF_DD*F128;
	A(10,7)	= -RHOFF_DD*F129;
	A(10,8)	= -RHOFF_DD*F1210;
	A(10,9)	= 0.0;
	A(10,10)	= 1.0;
	A(10,11)	= Z12_BD;

	A1D< 1> XSOL( 10);
	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J3 = XSOL(3);
	double J5 = XSOL(4);
	double J6 = XSOL(5);
	double J8 = XSOL(6);
	double J9 = XSOL(7);
	double J10 = XSOL(8);
	double J11 = XSOL(9);
	double J12 = XSOL(10);

	// irradiance, surface i
	double G1 = F12*J2+F13*J3+F15*J5+F16*J6;
	double G4 = F41*J1+F42*J2+F43*J3+F45*J5+F46*J6;
	double G7 = F78*J8+F79*J9+F710*J10+F711*J11+F712*J12;
	double G10 = F108*J8+F109*J9+F1011*J11+F1012*J12;

	TAU_BB = (TAUFF_BB_PERP*(AB-NK)*abs(sin(OMEGA_H)))/(2.0*S*abs(cos(OMEGA_H)));
	TAU_BD = (G4+TAUFF_DD*G10)/2.0;
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G7)/2.0;

}  // PD_BEAM_CASE_III
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_IV(		// pleated drape 14 surface flat-fabric model, case IV
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	[[maybe_unused]] double OMEGA_H,	//  horizontal profile angle, radians
	[[maybe_unused]] double DE,			//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	double RHOFF_BT_PERP, double TAUFF_BB_PERP, double TAUFF_BD_PERP,
	double RHOBF_BT_PERP, double TAUBF_BB_PERP, double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{
#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case Iv  omega=%0.2f", OMEGA_H/DTOR);
#endif
	// length of diagonal strings
	double AK = sqrt(W*W+S*S);
	double CG = AK;

	// diffuse and beam source terms
	double TAUBF_BT_PERP = TAUBF_BD_PERP + TAUBF_BB_PERP;
	double Z1_BB = TAUFF_BB_PARL;
	double Z1_BD = TAUFF_BD_PARL;
	double Z2_BD = Z1_BB*RHOBF_BT_PERP*S/W;
	double Z4_BD = TAUFF_BD_PERP*S/W;
	double Z6_BD = RHOFF_BT_PERP*S/W;
	double Z8_BD = Z1_BB*TAUBF_BT_PERP*S/W;

	// shape factors
	double F12, F14, F21, F24, F31, F32, F34, F41, F42, F56, F57, F58;
	double F67, F68, F76, F78, F86, F87;
	F12 = (S+W-AK)/(2.0*S);
	F14 = (S+W-CG)/(2.0*S);
	F21 = (S+W-AK)/(2.0*W);
	F24 = (AK+CG-2.0*S)/(2.0*W);
	F31 = (AK+CG-2.0*W)/(2.0*S);
	F32 = F12;
	F34 = F12;
	F41 = F21;
	F42 = F24;
	F56 = F12;
	F57 = F31;
	F58 = F14;
	F67 = F41;
	F68 = F24;
	F76 = F32;
	F78 = F34;
	F86 = F42;
	F87 = F21;

	// radiosity matrix
	A2D< 6, 8> A( 6, 8, 1, 1);
	A.Zero();
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F14;
	A(1,4)	= 0.0;
	A(1,5)	= 0.0;
	A(1,6)	= 0.0;
	A(1,7)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= -RHOBF_DD*F24;
	A(2,4)	= -TAUFF_DD*F86;
	A(2,5)	= -TAUFF_DD*F87;
	A(2,6)	= 0.0;
	A(2,7)	= Z2_BD;
	A(3,1)	= -RHOBF_DD*F41;
	A(3,2)	= -RHOBF_DD*F42;
	A(3,3)	= 1.0;
	A(3,4)	= 0.0;
	A(3,5)	= -TAUFF_DD*F67;
	A(3,6)	= -TAUFF_DD*F68;
	A(3,7)	= Z4_BD;
	A(4,1)	= -TAUBF_DD*F41;
	A(4,2)	= -TAUBF_DD*F42;
	A(4,3)	= 0.0;
	A(4,4)	= 1.0;
	A(4,5)	= -RHOFF_DD*F67;
	A(4,6)	= -RHOFF_DD*F68;
	A(4,7)	= Z6_BD;
	A(5,1)	= 0.0;
	A(5,2)	= 0.0;
	A(5,3)	= 0.0;
	A(5,4)	= -RHOFF_DD*F76;
	A(5,5)	= 1.0;
	A(5,6)	= -RHOFF_DD*F78;
	A(5,7)	= 0.0;
	A(6,1)	= -TAUBF_DD*F21;
	A(6,2)	= 0.0;
	A(6,3)	= -TAUBF_DD*F24;
	A(6,4)	= -RHOFF_DD*F86;
	A(6,5)	= -RHOFF_DD*F87;
	A(6,6)	= 1.0;
	A(6,7)	= Z8_BD;

	A1D< 1> XSOL( 6);
	XSOL.Zero();

	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J4 = XSOL(3);
	double J6 = XSOL(4);
	double J7 = XSOL(5);
	double J8 = XSOL(6);

	// irradiance, surface i
	double G1 = F12*J2+F14*J4;
	double G3 = F31*J1+F32*J2+F34*J4;
	double G5 = F56*J6+F57*J7+F58*J8;
	double G7 = F76*J6+F78*J8;

	TAU_BB = TAUFF_BB_PERP/2;
	TAU_BD = (G3+TAUFF_DD*G7)/2.0;
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G5)/2.0;

}  // PD_BEAM_CASE_IV
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_V(		// pleated drape 14 surface flat-fabric model, case V
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	double OMEGA_H,			//  horizontal profile angle, radians
	double DE,				//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	double RHOFF_BT_PERP, double TAUFF_BB_PERP, double TAUFF_BD_PERP,
	double RHOBF_BT_PERP, double TAUBF_BB_PERP, double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{
#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case V  omega=%0.2f", OMEGA_H/DTOR);
#endif

	// lengths of surfaces and diagonal strings
	double AK = sqrt(W*W+S*S);
	double CG = AK;
	double MK = (W*abs(sin(OMEGA_H)))/abs(cos(OMEGA_H));
	double DK = AK;
	double MF = S-MK;
	double DM = sqrt(W*W+MF*MF);
	double GM = sqrt(W*W+MK*MK);
	double GF = AK;

	// beam and diffuse source terms
	double TAUBF_BT_PERP = TAUBF_BD_PERP + TAUBF_BB_PERP;
	double Z1_BB = TAUFF_BB_PARL;
	double Z1_BD = TAUFF_BD_PARL;
	double Z2_BD = Z1_BB*RHOBF_BT_PERP*S/DE;
	double Z4_BD = TAUFF_BD_PERP*S/DE;
	double Z6_BD = RHOFF_BT_PERP*S/DE;
	double Z7_BD = RHOFF_BT_PARL;
	double Z9_BD = Z1_BB*TAUBF_BT_PERP*S/DE;

	// shape factors
	double F12, F14, F21, F24, F31, F32, F34, F41, F42, F56, F57, F58, F59;
	double F67, F68, F69, F76, F79, F86, F89, F96, F97, F98;
	F12 = (S+W-AK)/(2.0*S);
	F14 = (S+W-CG)/(2.0*S);
	F21 = (S+W-AK)/(2.0*W);
	F24 = (AK+CG-2.0*S)/(2.0*W);
	F31 = (AK+CG-2.0*W)/(2.0*S);
	F32 = F14;
	F34 = F12;
	F41 = F21;
	F42 = F24;
	F56 = F12;
	F57 = (DM+GF-(GM+W))/(2.0*S);
	F58 = (DK+GM-(DM+W))/(2.0*S);
	F59 = F14;
	F67 = (W+MF-DM)/(2.0*W);
	F68 = (DM+S-(DK+MF))/(2.0*W);
	F69 = F24;
	F76 = (W+MF-DM)/(2.0*MF);
	F79 = (GM+S-(GF+MK))/(2.0*MF);
	F86 = (DM+S-(DK+MF))/(2.0*MK);
	F89 = (W+MK-GM)/(2.0*MK);
	F96 = F42;
	F97 = (GM+S-(GF+MK))/(2.0*W);
	F98 = (W+MK-GM)/(2.0*W);

	// radiosity matrix
	A2D< 7, 9> A(7, 9, 1, 1);
	A.Zero();
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F14;
	A(1,4)	= 0.0;
	A(1,5)	= 0.0;
	A(1,6)	= 0.0;
	A(1,7)	= 0.0;
	A(1,8)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= -RHOBF_DD*F24;
	A(2,4)	= -TAUFF_DD*F96;
	A(2,5)	= -TAUFF_DD*F97;
	A(2,6)	= -TAUFF_DD*F98;
	A(2,7)	= 0.0;
	A(2,8)	= Z2_BD;
	A(3,1)	= -RHOBF_DD*F41;
	A(3,2)	= -RHOBF_DD*F42;
	A(3,3)	= 1.0;
	A(3,4)	= 0.0;
	A(3,5)	= -TAUFF_DD*F67;
	A(3,6)	= -TAUFF_DD*F68;
	A(3,7)	= -TAUFF_DD*F69;
	A(3,8)	= Z4_BD;
	A(4,1)	= -TAUBF_DD*F41;
	A(4,2)	= -TAUBF_DD*F42;
	A(4,3)	= 0.0;
	A(4,4)	= 1.0;
	A(4,5)	= -RHOFF_DD*F67;
	A(4,6)	= -RHOFF_DD*F68;
	A(4,7)	= -RHOFF_DD*F69;
	A(4,8)	= Z6_BD;
	A(5,1)	= 0.0;
	A(5,2)	= 0.0;
	A(5,3)	= 0.0;
	A(5,4)	= -RHOFF_DD*F76;
	A(5,5)	= 1.0;
	A(5,6)	= 0.0;
	A(5,7)	= -RHOFF_DD*F79;
	A(5,8)	= Z7_BD;
	A(6,1)	= 0.0;
	A(6,2)	= 0.0;
	A(6,3)	= 0.0;
	A(6,4)	= -RHOFF_DD*F86;
	A(6,5)	= 0.0;
	A(6,6)	= 1.0;
	A(6,7)	= -RHOFF_DD*F89;
	A(6,8)	= 0.0;
	A(7,1)	= -TAUBF_DD*F21;
	A(7,2)	= 0.0;
	A(7,3)	= -TAUBF_DD*F24;
	A(7,4)	= -RHOFF_DD*F96;
	A(7,5)	= -RHOFF_DD*F97;
	A(7,6)	= -RHOFF_DD*F98;
	A(7,7)	= 1.0;
	A(7,8)	= Z9_BD;

	A1D< 1> XSOL( 7);
	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J4 = XSOL(3);
	double J6 = XSOL(4);
	double J7 = XSOL(5);
	double J8 = XSOL(6);
	double J9 = XSOL(7);

	// irradiance, surface i
	double G1 = F12*J2+F14*J4;
	double G3 = F31*J1+F32*J2+F34*J4;
	double G5 = F56*J6+F57*J7+F58*J8+F59*J9;
	double G7 = F76*J6+F79*J9;
	double G8 = F86*J6+F89*J9;

	TAU_BB = (2.0*(DE-W)*abs(sin(OMEGA_H))*TAUFF_BB_PARL
		   + (S*abs(cos(OMEGA_H))-(DE-W)*abs(sin(OMEGA_H)))*TAUFF_BB_PERP)/(2.*S*abs(cos(OMEGA_H)));
	TAU_BD = (S*G3+TAUFF_DD*(MK*G8+MF*G7)+MF*TAUFF_BD_PARL)/(2.*S);
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G5)/2.;

}  // PD_BEAM_CASE_V
//-----------------------------------------------------------------------------
static void PD_BEAM_CASE_VI(		// pleated drape 14 surface flat-fabric model, case VI
	double S,				//  pleat spacing (> 0)
	double W,				//  pleat depth (>=0, same units as S)
	[[maybe_unused]] double OMEGA_H,	//  horizontal profile angle, radians
	[[maybe_unused]] double DE,			//  width of illumination on pleat bottom (same units as S)
							//  fabric properties at current (off-normal) incidence
							//    _PARL = surface parallel to window (pleat top/bot)
							//    _PERP = surface perpendicular to window (pleat side)
	double RHOFF_BT_PARL, double TAUFF_BB_PARL, double TAUFF_BD_PARL,
	[[maybe_unused]] double RHOBF_BT_PARL, [[maybe_unused]] double TAUBF_BB_PARL, [[maybe_unused]] double TAUBF_BD_PARL,
	[[maybe_unused]] double RHOFF_BT_PERP, [[maybe_unused]] double TAUFF_BB_PERP, [[maybe_unused]] double TAUFF_BD_PERP,
	[[maybe_unused]] double RHOBF_BT_PERP, [[maybe_unused]] double TAUBF_BB_PERP, [[maybe_unused]] double TAUBF_BD_PERP,
	double RHOFF_DD,		//  fabric front diffuse-diffuse reflectance
	double RHOBF_DD,		//  fabric back diffuse-diffuse reflectance
	double TAUFF_DD,		//  fabric front diffuse-diffuse transmittance
	double TAUBF_DD,		//  fabric back diffuse-diffuse transmittance
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BD,			//  returned: drape front beam-diffuse transmittance
	double& TAU_BB)			//  returned: drape front beam-beam transmittance
{
#if defined( PD_PRINTCASE)
	Message( msgDBG, "PD case VI  omega=%0.2f", OMEGA_H/DTOR);
#endif

	// lengths of diagonal strings
	double AK = SqrtSumSq( W, S);
	double CG = AK;

	// diffuse source terms
	double Z1_BD = TAUFF_BD_PARL;
	double Z7_BD = RHOFF_BT_PARL;

	// shape factors
	double F12, F14, F21, F24, F31, F32, F34, F41, F42;
	double F56, F57, F58, F67, F68, F76, F78, F86, F87;
	F12 = (S+W-AK)/(2.0*S);
	F14 = (S+W-CG)/(2.0*S);
	F21 = (S+W-AK)/(2.0*W);
	F24 = (AK+CG-2.0*S)/(2.0*W);
	F31 = (AK+CG-2.0*W)/(2.0*S);
	F32 = F12;
	F34 = F14;
	F41 = F21;
	F42 = F24;
	F56 = F12;
	F57 = F31;
	F58 = F14;
	F67 = F41;
	F68 = F24;
	F76 = F14;
	F78 = F14;
	F86 = F42;
	F87 = F21;

	// radiosity matrix
	A2D< 6, 8> A( 6, 8, 1, 1);
	A.Zero();
	A(1,1)	= 1.0;
	A(1,2)	= -RHOBF_DD*F12;
	A(1,3)	= -RHOBF_DD*F14;
	A(1,4)	= 0.0;
	A(1,5)	= 0.0;
	A(1,6)	= 0.0;
	A(1,7)	= Z1_BD;
	A(2,1)	= -RHOBF_DD*F21;
	A(2,2)	= 1.0;
	A(2,3)	= -RHOBF_DD*F24;
	A(2,4)	= -TAUFF_DD*F86;
	A(2,5)	= -TAUFF_DD*F87;
	A(2,6)	= 0.0;
	A(2,7)	= 0.0;
	A(3,1)	= -RHOBF_DD*F41;
	A(3,2)	= -RHOBF_DD*F42;
	A(3,3)	= 1.0;
	A(3,4)	= 0.0;
	A(3,5)	= -TAUFF_DD*F67;
	A(3,6)	= -TAUFF_DD*F68;
	A(3,7)	= 0.0;
	A(4,1)	= -TAUBF_DD*F41;
	A(4,2)	= -TAUBF_DD*F42;
	A(4,3)	= 0.0;
	A(4,4)	= 1.0;
	A(4,5)	= -RHOFF_DD*F67;
	A(4,6)	= -RHOFF_DD*F68;
	A(4,7)	= 0.0;
	A(5,1)	= 0.0;
	A(5,2)	= 0.0;
	A(5,3)	= 0.0;
	A(5,4)	= -RHOFF_DD*F76;
	A(5,5)	= 1.0;
	A(5,6)	= -RHOFF_DD*F78;
	A(5,7)	= Z7_BD;
	A(6,1)	= -TAUBF_DD*F21;
	A(6,2)	= 0.0;
	A(6,3)	= -TAUBF_DD*F24;
	A(6,4)	= -RHOFF_DD*F86;
	A(6,5)	= -RHOFF_DD*F87;
	A(6,6)	= 1.0;
	A(6,7)	= 0.0;

	A1D< 1> XSOL( 6);
	XSOL.Zero();
	A.Solve( XSOL);

	// radiosity, surface i
	double J1 = XSOL(1);
	double J2 = XSOL(2);
	double J4 = XSOL(3);
	double J6 = XSOL(4);
	double J7 = XSOL(5);
	double J8 = XSOL(6);

	// irradiane, surface i
	double G1 = F12*J2+F14*J4;
	double G3 = F31*J1+F32*J2+F34*J4;
	double G5 = F56*J6+F57*J7+F58*J8;
	double G7 = F76*J6+F78*J8;

	TAU_BB = TAUFF_BB_PARL;
	TAU_BD = (G3+TAUFF_DD*G7+TAUFF_BD_PARL)/2.0;
	RHO_BD = (RHOFF_BT_PARL+TAUBF_DD*G1+G5)/2.0;

}  // PD_BEAM_CASE_VI
//-----------------------------------------------------------------------------
static int PD_BEAM(	// beam properties pleated drape flat-fabric model with rectangular enclosure
	double S,				// pleat spacing (> 0)
	double W,				// pleat depth (>=0, same units as S)
	double OHM_V_RAD,		// vertical profile angle, radians
							//    +=above horiz)
	double OHM_H_RAD,		// horizontal profile angle, radians
							//    +=clockwise when viewed from above)
	double RHOFF_BT0, double TAUFF_BB0,	// front (outside) fabric properties
		double TAUFF_BD0, double RHOFF_DD, double TAUFF_DD,
	double RHOBF_BT0, double TAUBF_BB0, // back (inside) fabric properties
		double TAUBF_BD0, double RHOBF_DD, double TAUBF_DD,
	double& RHO_BD,			//  returned: drape front beam-diffuse reflectance
	double& TAU_BB,			//  returned: drape beam-beam transmittance
	double& TAU_BD)			//  returned: drape beam-diffuse transmittance
// calculates the effective front-side solar optical properties of a drapery layer.
// back-side properties can be calculated by calling with interchanged fabric properties
// returns 0 for flat shade (no pleats)
//         1-6 per geometric case
{
double RHOFF_BT_PARL, TAUFF_BB_PARL, TAUFF_BD_PARL;
double RHOBF_BT_PARL, TAUBF_BB_PARL, TAUBF_BD_PARL;
double RHOFF_BT_PERP, TAUFF_BB_PERP, TAUFF_BD_PERP;
double RHOBF_BT_PERP, TAUBF_BB_PERP, TAUBF_BD_PERP;

	//  limit profile angles to +/- PI/2
	//  by symmetry, properties same for +/- profile angle
	double OMEGA_V = abs( max( -89.5*DTOR, min( 89.5*DTOR, OHM_V_RAD)));
	double OMEGA_H = abs( max( -89.5*DTOR, min( 89.5*DTOR, OHM_H_RAD)));

	// incidence angles on pleat front/back (_PARL) and sides (_PERP)
	//   parallel / perpendicular is relative to window plane
	double THETA_PARL = acos(abs(cos(atan(tan(OMEGA_V)*cos(OMEGA_H)))*cos(OMEGA_H)));
	double THETA_PERP = acos(abs(cos(atan(tan(OMEGA_V)*sin(OMEGA_H)))*sin(OMEGA_H)));

	//  off-normal fabric properties, front surface
	double TAUFF_BT0 = TAUFF_BB0 + TAUFF_BD0;
	FM_BEAM( THETA_PARL, RHOFF_BT0, TAUFF_BT0, TAUFF_BB0, RHOFF_BT_PARL, TAUFF_BB_PARL, TAUFF_BD_PARL);
	if (W/S < SMALL_ERROR)
	{	//  flat drape (no pleats) -- return fabric properties
		RHO_BD = RHOFF_BT_PARL;
		TAU_BD = TAUFF_BD_PARL;
		TAU_BB = TAUFF_BB_PARL;
		return 0;
	}

	FM_BEAM( THETA_PERP, RHOFF_BT0, TAUFF_BT0, TAUFF_BB0, RHOFF_BT_PERP, TAUFF_BB_PERP, TAUFF_BD_PERP);

	//  Off-normal fabric properties, back surface
	double TAUBF_BT0 = TAUBF_BB0 + TAUBF_BD0;
	FM_BEAM( THETA_PARL, RHOBF_BT0, TAUBF_BT0, TAUBF_BB0, RHOBF_BT_PARL, TAUBF_BB_PARL, TAUBF_BD_PARL);
	FM_BEAM( THETA_PERP, RHOBF_BT0, TAUBF_BT0, TAUBF_BB0, RHOBF_BT_PERP, TAUBF_BB_PERP, TAUBF_BD_PERP);

	//  DE = length of directly illuminated surface on side of pleat that
	//    is open on front (same units as S and W)
	double DE = S * abs(cos(OMEGA_H) / max( .000001, sin(OMEGA_H)) );
	//  EF = length of pleat side shaded surface (W-DE) (same units as S and W)
	double EF = W-DE;

	int geoCase = 0;

	//  select geometric case
	if ( DE < W - SMALL_ERROR)
	{	//  illuminated length less than pleat depth
		if (DE < EF - SMALL_ERROR)
			geoCase = 1;
		else if (DE <= EF + SMALL_ERROR)
			geoCase = 2; //  illum and shade equal
		else
			geoCase = 3; //  illum > shade
	}
	else if (DE <= W + SMALL_ERROR)
		geoCase = 4;	//  illum length same as pleat depth
	else if (DE < 9000.*S)
		geoCase = 5;	//  some direct illum on pleat back
	else
		geoCase = 6;	//  beam parallel to pleat sides (no direct illum on pleat back)

	typedef void cdecl PDCASE( double, double, double, double,
			   double, double, double, double, double, double,
			   double, double, double, double, double, double,
			   double, double, double, double,
			   double&, double&, double&);

	static PDCASE* PDCaseTbl[ 7] = { NULL, PD_BEAM_CASE_I, PD_BEAM_CASE_II, PD_BEAM_CASE_III,
	PD_BEAM_CASE_IV, PD_BEAM_CASE_V, PD_BEAM_CASE_VI };
	(*PDCaseTbl[ geoCase])( S, W, OMEGA_H, DE,
				RHOFF_BT_PARL, TAUFF_BB_PARL, TAUFF_BD_PARL,
				RHOBF_BT_PARL, TAUBF_BB_PARL, TAUBF_BD_PARL,
				RHOFF_BT_PERP, TAUFF_BB_PERP, TAUFF_BD_PERP,
				RHOBF_BT_PERP, TAUBF_BB_PERP, TAUBF_BD_PERP,
				RHOBF_DD, RHOFF_DD, TAUFF_DD, TAUBF_DD,
				RHO_BD, TAU_BD, TAU_BB);
	return geoCase;
}  // PD_BEAM
//-------------------------------------------------------------------------------
bool CFSLAYER::cl_PDLWP(	// Drape LW properties from fabric / geometry
	CFSLWP& LLWP) const		// returned: equivalent layer long wave properties
{
	if (LTYPE != ltyDRAPE)
		return false;

	bool bRet = true;

	double OPENNESS_FABRIC = SWP_MAT.TAUSFBB;

	PD_LW( S, W, OPENNESS_FABRIC, LWP_MAT.EPSLF, LWP_MAT.EPSLB, LWP_MAT.TAUL,
		LLWP.EPSLF, LLWP.TAUL);

	double TAULX;
	PD_LW( S, W, OPENNESS_FABRIC, LWP_MAT.EPSLB, LWP_MAT.EPSLF, LWP_MAT.TAUL,
		LLWP.EPSLB, TAULX);

#if defined( _DEBUG)
	if (abs( LLWP.TAUL - TAULX) > 0.001)
		Message( msgERR, "Layer '%s': PD LW TAU mismatch", FCGET( ID));
#endif
	return bRet;
}  // CFSLAYER::cl_PDLWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_PDSWP(	// Drape SW diffuse properties from fabric / geometry
	CFSSWP& LSWP,			// returned: equivalent layer properties
	double OHM_V_RAD /*=-999.*/,	// solar profile angle, radians
									//   positive above horizon, -PI/2 - PI/2
	double OHM_H_RAD /*=-999.*/) const	// horizonal profile angle, radians
										//   = wall-solar azimuth angle
{
#if defined( _DEBUG)
static int caseCount[ 7] = { 0};
#endif
	if (LTYPE != ltyDRAPE)
		return false;

	bool bRet = true;

	bool bDiffuse = OHM_V_RAD < -998. && OHM_H_RAD < -998.;

	if (bDiffuse)
	{	PD_DIFF( S, W, SWP_MAT.RHOSFDD, SWP_MAT.RHOSBDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSFDD, LSWP.TAUS_DD);
		double TAUX;
		PD_DIFF( S, W, SWP_MAT.RHOSBDD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSBDD, TAUX);
	  #if 0 && defined( _DEBUG)
		DbDump( "cl_PDSWP diff  ");
		if (abs( LSWP.TAUS_DD - TAUX) > 0.001)
			Message( msgERR, "Layer '%s': Drape SW DD TAU mismatch", FCGET( ID));
	  #endif
	}
	else
	{	// normal beam-total properties of fabric
		double RHOFF_BT0 = SWP_MAT.RHOSFBB + SWP_MAT.RHOSFBD;	// front rho
		double RHOBF_BT0 = SWP_MAT.RHOSBBB + SWP_MAT.RHOSBBD;	// back rho

		// drape front properties
		[[maybe_unused]] int geoCaseF = PD_BEAM( S, W, OHM_V_RAD, OHM_H_RAD,
			RHOFF_BT0, SWP_MAT.TAUSFBB, SWP_MAT.TAUSFBD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
			RHOBF_BT0, SWP_MAT.TAUSBBB, SWP_MAT.TAUSBBD, SWP_MAT.RHOSBDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSFBD, LSWP.TAUSFBB, LSWP.TAUSFBD);

		// drape back properties: call with reversed fabric properies
		[[maybe_unused]] int geoCaseB = PD_BEAM( S, W, OHM_V_RAD, OHM_H_RAD,
			RHOBF_BT0, SWP_MAT.TAUSBBB, SWP_MAT.TAUSBBD, SWP_MAT.RHOSBDD, SWP_MAT.TAUS_DD,
			RHOFF_BT0, SWP_MAT.TAUSFBB, SWP_MAT.TAUSFBD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSBBD, LSWP.TAUSBBB, LSWP.TAUSBBD);
#if 0 && defined( _DEBUG)
		DbDump( "cl_PDSWP beam  ");
		caseCount[ geoCaseF]++;
		caseCount[ geoCaseB]++;
#endif
	}
	return bRet;
}  // CFSLAYER::cl_PDSWP
//-----------------------------------------------------------------------------
void CFSLAYER::DbDump( const char* tag/*=""*/) const
{
	Message( msgDBG, "\n%s%-.10s   ty=%d\n  %s   %s\n  %s   %s",
		tag, ID, LTYPE,
		LWP_MAT.DbFmt("MAT ").c_str(), SWP_MAT.DbFmt().c_str(),
		LWP_EL.DbFmt("EL  ").c_str(), SWP_EL.DbFmt().c_str());
}	// CFSLAYER::DbDump
//==============================================================================

///////////////////////////////////////////////////////////////////////////////
// VB (venetian blind) models
///////////////////////////////////////////////////////////////////////////////
static void VB_DIFF(		// four surface flat-slat model with slat transmittance

	double S,			// slat spacing (any length units; same units as W)
						// must be > 0
	double W,			// slat tip-to-tip width (any length units; same units as S)
						//    must be > 0
	double PHI,			// slat angle, radians (-PI/2 <= PHI <= PI/2)
						//    ltyVBHOR: + = front-side slat tip below horizontal
						//    ltyVBVER: + = front-side slat tip is counter-
						//                  clockwise from normal (viewed from above)
	double RHODFS_SLAT,	// reflectance of downward-facing slat surfaces (concave?)
	double RHOUFS_SLAT,	// reflectance of upward-facing slat surfaces (convex?)
	double TAU_SLAT,	// diffuse transmitance of slats
	double& RHOFVB,		// returned: front side effective diffuse reflectance of venetian blind
	double& TAUVB)		// returned: effective diffuse transmittance of venetian blind

//  Calculates the venetian blind layer effective diffuse properties.
//  Returns the transmittance and front-side reflectance.
//  To obtain the back-side reflectance call a second time with the same
//     input data - except negate the slat angle, PHI
{
	//  lengths of the diagonal strings used in the four-surface model
	double CD = sqrt (Square(W*cos(PHI)) + Square(S + W*sin(PHI)));
	double AF = sqrt (Square(W*cos(PHI)) + Square(S - W*sin(PHI)));

	double F13 = (W+S-CD)/(2.0*S);       // shape factor front opening to top slat
	double F14 = (W+S-AF)/(2.0*S);       // shape factor front opening to bottom slat
	double FSS = 1.0 - (S/W)*(F13+F14);  // slat-to-slat shape factor
	double F31 = (S/W)*F13;              // shape factor - top to front
	double F41 = (S/W)*F14;              // shape factor - bottom to front
	double F12 = 1.0 - F13 - F14;        // front opening to back opening shape factor

	double DEN = 1.0 - (TAU_SLAT*FSS);   // denominator - used four times
	double C3 = (RHODFS_SLAT*F31 + TAU_SLAT*F41)/DEN;
	double B3 = (RHODFS_SLAT*FSS)/DEN;
	double C4 = (RHOUFS_SLAT*F41 + TAU_SLAT*F31)/DEN;
	double B4 = (RHOUFS_SLAT*FSS)/DEN;

	double K3 = (C3 + (B3*C4))/(1.0 - (B3*B4));
	double K4 = (C4 + (B4*C3))/(1.0 - (B3*B4));

	TAUVB = P01( F12 + (F14*K3) + (F13*K4), "VB_DIFF Tau");	//  transmittance of VB (equal front/back)
	RHOFVB = P01( (F13*K3) + (F14*K4), "VB_DIFF RhoF");		//  diffuse reflectance of VB front-side
}  // VB_DIFF
//------------------------------------------------------------------------------
static double VB_SLAT_RADIUS_RATIO(		//  curved slat radius ratio (W / R)

	double W,		//  slat tip-to-tip (chord) width (any length units; same units as C)
					//    must be > 0
	double C)		//  slat crown height (any units, same units as W)
					//    must be >= 0
{
	double rat = 0.;
	if (C > 0. && W > 0.)
	{	double CX = min( C, W/2.001);
		rat = 2 * W * CX / (CX*CX + W*W/4);
	}
	return rat;
} // VB_SLAT_RADIUS_RATIO


//-----------------------------------------------------------------------------
static void VB_SOL4(	// four surface flat-slat model with slat transmittance
	double S,		//  slat spacing (any length units; same units as W)
					// 	must be > 0
	double W,		//  slat tip-to-tip width (any length units; same units as S)
					//    must be > 0
	double OMEGA,	//  incident beam profile angle (radians)
					//    ltyVBHOR: +=above horizontal
					//    ltyVBVER: +=clockwise when viewed from above
	double DE,		//  distance from front tip of any slat to shadow (caused by the adjacent slat) on
					//     the plane of the same slat de may be greater than the slat width, w
	double PHI,		//  slat angle, radians (-PI/2 <= PHI <= PI/2)
					//    ltyVBHOR: + = front-side slat tip below horizontal
					//    ltyVBVER: + = front-side slat tip is counter-
					//                  clockwise from normal (viewed from above)
	double RHODFS_SLAT,	//  solar reflectance downward-facing slat surfaces (concave?)
	double RHOUFS_SLAT,	//  solar reflectance upward-facing slat surfaces (convex?)
	double TAU_SLAT,	//  solar transmittance of slat
						//     Note: all solar slat properties - incident-to-diffuse
	double& RHO_BD,		//  returned: solar beam-to-diffuse reflectance the venetian blind (front side)
	double& TAU_BD)		//  returned: solar beam-to-diffuse transmittance of the venetian blind (front side)

// calculates the effective solar properties of a venetian blind layer
// returns transmittance(s) and front-side reflectance of the venetian blind layer.
// if you want the back-side reflectance call with the same
// input data - except negate the slat angle, PHI_DEG
{
	//  lengths of diagonal strings used in the four-surface model
	double AF = SqrtSumSq( W*cos(PHI), S - W*sin( PHI));
	double CD = SqrtSumSq( W*cos(PHI), S + W*sin( PHI));

	double Z3, Z4;	//  diffuse source terms from surfaces 3 and 4 due to incident beam radiation
	if ((PHI + OMEGA) >= 0.0)
	{	//  sun shines on top of slat
		Z3 = TAU_SLAT*S / DE;
		Z4 = RHOUFS_SLAT*S / DE;
	}
	else
	{	//  sun shines on bottom of slat
		Z3 = RHODFS_SLAT*S / DE;
		Z4 = TAU_SLAT*S / DE;
	}
	
	if (abs(PHI - PIOVER2) < SMALL_ERROR)
	{	// venetian blind is closed
		if (W < S)
		{	// gaps between slats when closed
			RHO_BD = (W / S)*RHOUFS_SLAT;
			TAU_BD = (W / S)*TAU_SLAT;
		}
		else
		{	// no gaps between slats when closed
			RHO_BD = RHOUFS_SLAT;
			TAU_BD = TAU_SLAT;
		}
	}
	else
	{	// blind is open
		double F13, F14, F23, F24, F34, F43;	//  Shape factors
		F13 = (S+W-CD)/(2.0*S);
		F14 = (S+W-AF)/(2.0*S);
		F23 = (S+W-AF)/(2.0*S);
		F24 = (S+W-CD)/(2.0*S);
		F34 = (CD+AF-2.0*S)/(2.0*W);
		F43 = (CD+AF-2.0*S)/(2.0*W);

		double C3 = 1.0 / (1.0 - TAU_SLAT*F43);
		double B3 = (RHODFS_SLAT*F34) / (1.0 - TAU_SLAT*F43);
		double C4 = 1.0 / (1.0 - TAU_SLAT*F34);
		double B4 = (RHOUFS_SLAT*F43) / (1.0 - TAU_SLAT*F34);
		double J3 = (C3*Z3 + B3*C4*Z4) / (1.0 - B3*B4);
		double J4 = (C4*Z4 + B4*C3*Z3) / (1.0 - B3*B4);

		RHO_BD = F13*J3 + F14*J4;
		TAU_BD = F23*J3 + F24*J4;
	}
}  // VB_SOL4
//-----------------------------------------------------------------------------
void VB_SOL6(	// 6 surface flat-slat model with slat transmittance
	double S,		//  slat spacing (any length units; same units as W)
					// 	must be > 0
	double W,		//  slat tip-to-tip width (any length units; same units as S)
					//    must be > 0
	double OMEGA,	//  incident beam profile angle (radians)
					//    ltyVBHOR: +=above horizontal
					//    ltyVBVER: +=clockwise when viewed from above
	double DE,		//  distance from front tip of any slat to shadow (caused by the adjacent slat) on
					//     the plane of the same slat DE may be greater than the slat width, w
	double PHI,		//  slat angle, radians (-PI/2 <= PHI <= PI/2)
						//    ltyVBHOR: + = front-side slat tip below horizontal
						//    ltyVBVER: + = front-side slat tip is counter-
						//                  clockwise from normal (viewed from above)
	double RHODFS_SLAT,	//  solar reflectance downward-facing slat surfaces (concave)
	double RHOUFS_SLAT,	//  solar reflectance upward-facing slat surfaces (convex)
	double TAU_SLAT,	//  solar transmittance of slat
						//     Note: all solar slat properties - incident-to-diffuse
	double& RHO_BD,		//  returned: solar beam-to-diffuse reflectance the venetian blind (front side)
	double& TAU_BD)		//  returned: solar beam-to-diffuse transmittance of the venetian blind (front side)

// calculates the effective solar properties of a venetian blind layer
//    returns the transmittance(s) and front-side reflectance
//    if you want the back-side reflectance call with the same input data
//	     except negate the slat angle, PHI_DEG
//
{
	double Z3, Z4;			//  diffuse source terms from surfaces 3 and 4 due to incident beam radiation
	if ((PHI + OMEGA) >= 0.)
	{	// sun shines on top of slat
		Z3 = TAU_SLAT*S / DE;
		Z4 = RHOUFS_SLAT*S / DE;
	}
	else
	{	// sun shines on bottom of slat
		Z3 = RHODFS_SLAT*S / DE;
		Z4 = TAU_SLAT*S / DE;
	}

//  CHECK TO SEE if VENETIAN BLIND IS CLOSED
	if (abs(PHI - PIOVER2) < SMALL_ERROR)
	{	// blind is closed
		if (W < S)
		{	// gaps between slats when blind is closed
			RHO_BD = (W / S)*RHOUFS_SLAT;
			TAU_BD = (W / S)*TAU_SLAT;
		}
		else
		{	// no gaps when closed
			RHO_BD = RHOUFS_SLAT;
			TAU_BD = TAU_SLAT;
		}
	}
	else
	{	// blind is open
		//  lengths of slat segments and diagonal strings used in the six-surface model
		double AB = DE;
		double AF = SqrtSumSq(W*cos(PHI), S - W*sin(PHI));
		double BC = W - AB;
		double EF = BC;
		double BD = SqrtSumSq(DE*cos(PHI), S + DE*sin(PHI));
		double BF = SqrtSumSq(EF*cos(PHI), S - EF*sin(PHI));
		double CD = SqrtSumSq(W*cos(PHI), S + W*sin(PHI));
		double CE = SqrtSumSq(EF*cos(PHI), S + EF*sin(PHI));
		double AE = SqrtSumSq(DE*cos(PHI), S - DE*sin(PHI));

		double F13, F14, F23, F24, F34, F36, F15, F16;	//  shape factors
		double F43, F45, F54, F56, F63, F65, F25, F26;
		F13 = (S + AB - BD) / (2.0*S);
		F14 = (S + DE - AE) / (2.0*S);
		F15 = (W + BD - (AB + CD)) / (2.0*S);
		F16 = (W + AE - (AF + DE)) / (2.0*S);
		F23 = (W + BF - (BC + AF)) / (2.0*S);
		F24 = (W + CE - (CD + EF)) / (2.0*S);
		F25 = (S + BC - BF) / (2.0*S);
		F26 = (S + EF - CE) / (2.0*S);
		F34 = (AE + BD - 2.0*S) / (2.0*AB);
		F36 = (AF + S - (AE + BF)) / (2.0*AB);
		F43 = (AE + BD - 2.0*S) / (2.0*DE);
		F45 = (CD + S - (BD + CE)) / (2.0*DE);
		F54 = (CD + S - (BD + CE)) / (2.0*BC);
		F56 = (CE + BF - 2.0*S) / (2.0*BC);
		F63 = (AF + S - (AE + BF)) / (2.0*EF);
		F65 = (BF + CE - 2.0*S) / (2.0*EF);

		//  POPULATE THE COEFFICIENTS OF THE RADIOSITY MATRIX
		A2D< 4, 6> A(4, 6, 1, 1);
		A(1, 1) = 1.0 - TAU_SLAT*F43;
		A(1, 2) = -RHODFS_SLAT*F34;
		A(1, 3) = -TAU_SLAT*F45;
		A(1, 4) = -RHODFS_SLAT*F36;
		A(1, 5) = Z3;
		A(2, 1) = -RHOUFS_SLAT*F43;
		A(2, 2) = 1.0 - TAU_SLAT*F34;
		A(2, 3) = -RHOUFS_SLAT*F45;
		A(2, 4) = -TAU_SLAT*F36;
		A(2, 5) = Z4;
		A(3, 1) = -TAU_SLAT*F63;
		A(3, 2) = -RHODFS_SLAT*F54;
		A(3, 3) = 1.0 - TAU_SLAT*F65;
		A(3, 4) = -RHODFS_SLAT*F56;
		A(3, 5) = 0.0;
		A(4, 1) = -RHOUFS_SLAT*F63;
		A(4, 2) = -TAU_SLAT*F54;
		A(4, 3) = -RHOUFS_SLAT*F65;
		A(4, 4) = 1.0 - TAU_SLAT*F56;
		A(4, 5) = 0.0;

		A1D< 1> XSOL(4);
		A.Solve(XSOL);
		double J3, J4, J5, J6;	//  radiosity, surface i
		J3 = XSOL(1);
		J4 = XSOL(2);
		J5 = XSOL(3);
		J6 = XSOL(4);

		RHO_BD = F13*J3 + F14*J4 + F15*J5 + F16*J6;
		TAU_BD = F23*J3 + F24*J4 + F25*J5 + F26*J6;
	}
}  // VB_SOL6
//-----------------------------------------------------------------------------
static void VB_SOL46_CURVE(	// four and six surface curve-slat model with slat transmittance
	double S,			//  slat spacing (any length units; same units as W)
						// 	must be > 0
	double W,			//  slat tip-to-tip (chord) width (any length units; same units as S)
						//    must be > 0
	double SL_WR,		//  slat curvature radius ratio (= W/R)
						//    0 = flat
	double PHI,			//  slat angle, radians (-PI/2 <= PHI <= PI/2)
						//    ltyVBHOR: + = front-side slat tip below horizontal
						//    ltyVBVER: + = front-side slat tip is counter-
						//                  clockwise from normal (viewed from above)
	double OMEGA,		//  incident beam profile angle (radians)
						//    ltyVBHOR: +=above horizontal
						//    ltyVBVER: +=clockwise when viewed from above
						//    Note: All solar slat properties are incident-to-diffuse
						//          Specular effects not covered by model
	double RHODFS_SLAT,	//  SW (solar) reflectance downward-facing slat surfaces (concave?)
	double RHOUFS_SLAT,	//  SW (solar) reflectance upward-facing slat surfaces (convex?)
	double TAU_SLAT,	//  SW (solar) transmittance of slats
	double& RHO_BD,		//  returned: effective SW (solar) beam-to-diffuse reflectance front side
	double& TAU_BB,		//  returned: effective SW (solar) beam-to-beam transmittance front side
	double& TAU_BD)		//  returned: effective SW (solar) beam-to-diffuse transmittance front side

// calculates the effective solar properties of a venetian blind layer
//  returns the transmittance(s) and front-side
//  reflectance of the venetian blind layer.
//  if you want the back-side reflectance call with the same
//     input data - except negate the slat angle, phi_deg
{

#undef CURVEDSLAT_CORR	// define to enable curved slat correction model

double DE = 0.;		//  distance from front tip of any slat to shadow (caused by the adjacent slat) on
					//     the plane of the same slat; DE may be greater than the slat width, W


	//  limit slat angle to +/- 90 deg
	PHI = max( -DTOR*90., min( DTOR*90., PHI));
	//  limit profile angle to +/- 89.5 deg
	OMEGA = max( -DTOR*89.5, min( DTOR*89.5, OMEGA));

	[[maybe_unused]] double SL_RAD = W / max( SL_WR, .0000001);
	[[maybe_unused]] double SL_THETA = 2. * asin( 0.5*SL_WR);

#if defined( CURVEDSLAT_CORR)
double  Slope,T_CORR_D,T_CORR_F,RHO_TEMP,TAU_TEMP;
double  XA,XB,XC,XD,XE,XF,YA,YB,YC,YD,YE,YF;

	//   determine bounds for curvature correction and apply correction to beam-beam transmittance
	if (abs(PHI+OMEGA) < SL_THETA/2.)
	{	// calculate beam transmission
		XA=SL_RAD*sin(-SL_THETA/2.0);	// Glass-side end coordinate
		YA=SL_RAD*cos(-SL_THETA/2.0);
		XB=-XA;						// Indoor-side end coordinate
		YB=YA;
		YC=SL_RAD*cos( PHI+OMEGA);	// Tangent to slat in irradiance direction
		XC=SqrtDifSq( SL_RAD, YC);
		Slope=-XC/YC;
		if (abs(Slope) < SMALL_ERROR)
		{	XD=0.0;
			YD=YA;
			XE=0.;
			YE=YD;
		}
		else
		{	if ((PHI+OMEGA)<0.)
			{	XC=-XC;
				Slope=-Slope;
				XD=(YB-Slope*XB)/(-1.0/Slope-Slope);
				XF=(YA-Slope*XA)/(-1.0/Slope-Slope);
				XE=XA+2.0*abs(XA-XF);
			}
			else
			{	XD=(YA-Slope*XA)/(-1.0/Slope-Slope);
				XF=(YB-Slope*XB)/(-1.0/Slope-Slope);
				XE=XB-2.0*abs(XB-XF);
			}
			YD=-XD/Slope;
			YE=-XE/Slope;
			YF=-XF/Slope;
		}

		T_CORR_D=SqrtSumSq( XC-XD, YC-YD);	// Slat thickness perpendicular to light direction
		T_CORR_F=SqrtSumSq( XC-XF, YC-YF);

		TAU_BB=1.0-T_CORR_D/(S*cos(OMEGA));
	}
	else
	{	// do not apply curvature correction to beam-beam transmittance
		DE = abs( OMEGA + PHI) < 0.0001
				? S * 1000000.
				: S * abs(cos(OMEGA) / sin( OMEGA + PHI);
		// check to see if there is direct beam transmission
		TAU_BB = (DE/W) > (1.0 - SMALL_ERROR))
					? max( 0., (DE-W)/DE)
					: 0.0;
	}

	// check to see if curvature correction includes double blockage
	// (tau_bb < 0.0 and set tau_bb = 0.0)
	if (TAU_BB < 0.0)
	{	//  yes, there is double blockage
		TAU_BB = 0.;

		// do not apply curvature correction to rho_bd, tau_bd if tau_bb < 0.0
		DE = abs( OMEGA + PHI) < 0.0001
				? S * 1000000.
				: S * abs(cos(OMEGA) / sin( OMEGA + PHI);
		if ((DE/W) > (1.0 - SMALL_ERROR))
			VB_SOL4( S, W, OMEGA, DE, PHI,
				RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT,	RHO_BD, TAU_BD);

		else
			VB_SOL6( S, W, OMEGA, DE, PHI,
				 RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT, RHO_BD, TAU_BD);
	}
	else
	{	//  there is no double blockage
		if (abs(PHI+OMEGA)<(SL_THETA/2.0))
		{	//  apply curvature correction
			XA=SL_RAD*sin(-SL_THETA/2.0);	// Glass-side end coordinate
			YA=SL_RAD*cos(-SL_THETA/2.0);
			XB=-XA;						// Indoor-side end coordinate
			YB=YA;
			YC=SL_RAD*cos(PHI+OMEGA);	// Tangent to slat in irradiance direction
			XC=SqrtDifSq( SL_RAD, YC);
			Slope=-XC/YC;
			if (abs(Slope) < SMALL_ERROR)
			{	XD=0.;
				YD=YA;
				XE=0.;
				YE=YD;
			}
			else
			{	if ((PHI+OMEGA)<0.0)
				{	XC=-XC;
					Slope=-Slope;
					XD=(YB-Slope*XB)/(-1.0/Slope-Slope);
					XF=(YA-Slope*XA)/(-1.0/Slope-Slope);
					XE=XA+2.0*abs(XA-XF);
				}
				else
				{	XD=(YA-Slope*XA)/(-1.0/Slope-Slope)
					XF=(YB-Slope*XB)/(-1.0/Slope-Slope)
					XE=XB-2.0*abs(XB-XF)
				}
				YD=-XD/Slope;
				YE=-XE/Slope;
				YF=-XF/Slope;
			}
			T_CORR_D=SqrtSumSq( XC-XD, YC-YD);	// Slat thickness perpendicular to light direction
			T_CORR_F=SqrtSumSq( XC-XF, YC-YF);

			if ((PHI+OMEGA)>= 0.)		// Slat is lit from above
			{	DE=XC-XA;
				VB_SOL6(S, W, OMEGA, DE, PHI,
								 RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT,
							 RHO_BD, TAU_BD)
				RHO_BD=RHO_BD*T_CORR_D/(S*cos(OMEGA))
				TAU_BD=TAU_BD*T_CORR_D/(S*cos(OMEGA))
			}
			else							// Slat is lit from below
			{	DE=XC-XA
				VB_SOL6(S, W, OMEGA, DE, PHI,
						RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT, RHO_BD, TAU_BD);
				RHO_TEMP=RHO_BD*T_CORR_F/(S*cos(OMEGA))
				TAU_TEMP=TAU_BD*T_CORR_F/(S*cos(OMEGA))
				DE=abs(XB-XF)
				VB_SOL6(S, W, OMEGA, DE, PHI,
								 RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT,  &
							 RHO_BD, TAU_BD)
				RHO_BD=RHO_BD*(T_CORR_D-T_CORR_F)/(S*cos(OMEGA))+RHO_TEMP
				TAU_BD=TAU_BD*(T_CORR_D-T_CORR_F)/(S*cos(OMEGA))+TAU_TEMP
			}
		}
	}
	else //  NO, DO NOT APPLY CURVATURE CORRECTION
	{	if (abs( OMEGA + PHI) < 0.0001) THEN
			DE = S*1000000.
		else
			DE = S * abs(cos(OMEGA) / sin(OMEGA + PHI))
		if ((DE/W) > (1.0 - SMALL_ERROR))
			VB_SOL4( S, W, OMEGA, DE, PHI,
				RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT,	RHO_BD, TAU_BD);
		else
			VB_SOL6( S, W, OMEGA, DE, PHI,
				RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT, RHO_BD, TAU_BD);
	}

#else	// CURVEDSLAT_CORR undefined
	RHO_BD = 0.;
	TAU_BB = 0.;
	TAU_BD = 0.;
	if (abs(PHI + OMEGA) < SMALL_ERROR)
		// beam aligned with slats
		TAU_BB = 1.;
	else                             //  BEAM NOT ALIGNED WITH SLATS
	{	DE = S * abs(cos(OMEGA) / sin(OMEGA + PHI));
		//  check to see if there is direct beam transmission
		if ((DE/W) > (1.0 - SMALL_ERROR))
		{	// direct beam transmission
			TAU_BB = max( (DE-W)/DE, 0.);
			VB_SOL4( S, W, OMEGA, DE, PHI,
				RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT, RHO_BD, TAU_BD);
		}
		else
			VB_SOL6( S, W, OMEGA, DE, PHI,
				RHODFS_SLAT, RHOUFS_SLAT, TAU_SLAT, RHO_BD, TAU_BD);
	}
#endif	// CURVEDSLAT_CORRECTION
}  // VB_SOL46_CURVE
//=============================================================================
bool CFSLAYER::cl_VBLWP(
	CFSLWP& LLWP) const	// LW properties from slat properties / geometry
{
	if (!cl_IsVB())
		return false;
	bool bRet = true;

	// slat reflectances
	double RHODFS_SLAT = 1. - LWP_MAT.EPSLB - LWP_MAT.TAUL;	// downward surface
	double RHOUFS_SLAT = 1. - LWP_MAT.EPSLF - LWP_MAT.TAUL;	// upward surface

	// TODO: are there cases where 2 calls not needed (RHODFS_SLAT == RHOUFS_SLAT??)
	double RHOLF;
	VB_DIFF( S, W, DTOR*PHI_DEG, RHODFS_SLAT, RHOUFS_SLAT, LWP_MAT.TAUL,
		RHOLF, LLWP.TAUL);
	LLWP.EPSLF = 1. - RHOLF - LLWP.TAUL;

	double RHOLB,TAULX;
	VB_DIFF( S, W, -DTOR*PHI_DEG, RHODFS_SLAT, RHOUFS_SLAT, LWP_MAT.TAUL,
		RHOLB, TAULX);
	LLWP.EPSLB = 1. - RHOLB - LLWP.TAUL;

#if defined( _DEBUG)
	if (abs( LLWP.TAUL - TAULX) > 0.001)
		Message( msgERR, "Layer '%s': VB LW TAU mismatch", FCGET( ID));
#endif
	return bRet;
}  // CFSLAYER::cl_VB_LWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_VBSWP(	// VB off-normal short wave (solar) properties
	CFSSWP& LSWP,				// returned: equivalent off-normal properties
								//   sets: RHOSFBD, TAUSFBB, TAUSFBD
	double OMEGA /*=-999.*/) const	// incident profile angle (radians)
									//   see comments elsewhere re sign convention
									//   if -999, do diffuse calc
{
	if (!cl_IsVB())
		return false;
	bool bRet = true;

	bool bDiffuse = OMEGA < -998.;

	if (bDiffuse)
	{	VB_DIFF( S, W, DTOR*PHI_DEG,
			SWP_MAT.RHOSBDD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSFDD, LSWP.TAUS_DD);

		double TAUX;
		VB_DIFF( S, W, -DTOR*PHI_DEG,
			SWP_MAT.RHOSBDD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
			LSWP.RHOSBDD, TAUX);

	#if defined( _DEBUG)
		if (abs( LSWP.TAUS_DD - TAUX) > 0.001)
			Message( msgERR, "Layer '%s': VB SW DD TAU mismatch", FCGET( ID));
	#endif
	}
	else
	{	double SL_WR = VB_SLAT_RADIUS_RATIO( W, C);

		// modify angle-dependent values for actual profile angle
		VB_SOL46_CURVE( S, W, SL_WR, DTOR*PHI_DEG, OMEGA,
				SWP_MAT.RHOSBDD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
				LSWP.RHOSFBD, LSWP.TAUSFBB, LSWP.TAUSFBD);

		VB_SOL46_CURVE( S, W, SL_WR, -DTOR*PHI_DEG, OMEGA,
				SWP_MAT.RHOSBDD, SWP_MAT.RHOSFDD, SWP_MAT.TAUS_DD,
				LSWP.RHOSBBD, LSWP.TAUSBBB, LSWP.TAUSBBD);
	}
	return bRet;
}  // CFSLAYER::cl_VBSWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_VBShadeControl(
	double OMEGA_DEG)	// incident profile angle (degrees)
						//   see comments elsewhere re sign convention
						//   < 0 = diffuse
// returns true iff PHI_DEG altered by more than .01 deg
//     else false (PHI_DEG not changed)
{
	double SLATA = PHI_DEG;

	if (CNTRL == lscVBPROF)
		// slatA = profA (max gain)
		SLATA = OMEGA_DEG < 0
					? -30.			// assume 30 deg for diffuse
					: -OMEGA_DEG;
	else if (CNTRL == lscVBNOBM)
		// slatA set to just exclude beam
		SLATA = cl_VBCriticalSlatAngle(
					OMEGA_DEG < 0 ? 30. : OMEGA_DEG);	// assume 30 deg for diffuse

	bool bRet = abs( SLATA - PHI_DEG) > .01;
	if (bRet)
		PHI_DEG = SLATA;
	return bRet;
}  // CFSLAYER::cl_VBShadeControl
//------------------------------------------------------------------------------
double CFSLAYER::cl_VBCriticalSlatAngle(	// slat angle that just excludes beam
	double OMEGA_DEG) const		// incident profile angle (degrees)
								//   see comments elsewhere re sign convention
// returns slat angle that just excludes beam, deg
{
	// TODO handle vert blind cases etc
	double RAT = S * cos( OMEGA_DEG*DTOR) / W;
	// limit upward slat angle to horiz = max visibility
	double critAng = max( 0., RTOD * asin( RAT) - OMEGA_DEG);
	return critAng;
}  // CFSLAYER::cl_VBCriticalSlatAngle
//------------------------------------------------------------------------------
bool CFSLAYER::cl_DoShadeControl(		// apply shade control
	double THETA,		// solar beam angle of incidence, from normal, (radians)
						// 0 <= THETA <= PI/2
	double OMEGA_V,		// solar beam vertical profile angle, +=above horizontal (radians)
						//   = solar elevation angle for a vertical wall with
						//     wall-solar azimuth angle equal to zero
	double OMEGA_H)		// solar beam horizontal profile angle, +=clockwise when viewed
						//   from above (radians)
						//   = wall-solar azimuth angle for a vertical wall
						//     Used for PD and vertical VB
// returns true iff L is modified
{
	bool bRet = false;		// default: no shade controls implemented

	// must be consistent with IsControlledShade()
	if (cl_IsVB() && CNTRL != lscNONE)
	{	double OMEGA_DEG =		// controlling profile angle
			  THETA < 0. || THETA >= PI/2. ? -1.	// diffuse only
			: LTYPE == ltyVBHOR			   ? RTOD * OMEGA_V		// horiz VB
			:								 RTOD * OMEGA_H;	// vert VB
		if (cl_VBShadeControl( OMEGA_DEG))
		{	cl_Finalize();
			bRet = true;
		}
	}
	return bRet;
}  // CFSLAYER::cl_DoShadeControl
//==============================================================================
CFSTY::CFSTY()
{	cf_Clear();
}		// CFSTY::CFSTY
//-----------------------------------------------------------------------
void CFSTY::cf_Clear()			// clear CFS
{
	memset( ID, 0, sizeof( ID));
	NL = 0;
	for (int iL=1; iL<=CFSMAXNL; iL++)
		L( iL).cl_Clear();
	for (int iG=1; iG<CFSMAXNL; iG++)
		G( iG).cg_Clear();
}    // CFSTY::cf_Clear
//-----------------------------------------------------------------------------
void CFSTY::cf_SetID( const char* _ID)
{	FCSET( ID, _ID);
}		// CFSTY::cf_SetID
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CFSLAYER
///////////////////////////////////////////////////////////////////////////////
CFSLAYER::CFSLAYER()
{	cl_Clear();		// clear all members
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
		cl_Clear();
}		// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
CFSLAYER::CFSLAYER(			// special glazing layer c'tor (Windows 6 arg order)
	const char* id,							// ID (name) of layer
	float thk,								// thickness, in (unused)
	float tSol, float rSol1, float rSol2,	// solar properties
	float tVis, float rVis1, float rVis2,	// visible properties (unused)
	float tIR,	float eIR1,  float eIR2,	// LW properties
	float kEff)								// conductivity
{
	thk, tVis, rVis1, rVis2, kEff;
	cl_InitGlaze( id, tSol, rSol1, eIR1, rSol2, eIR2, tIR);
}		// CFSLAYER::CFSLAYER
//-----------------------------------------------------------------------------
void CFSLAYER::cl_SetID( const char* _ID)
{	FCSET( ID, _ID);
}		// CFSLAYER::cl_SetID
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_InitGlaze(			// init glazing layer
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
	cl_Clear();

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
	return cl_CheckFix( id);
}		// CFSLAYER::cl_InitGlaze
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_InitRollB(			// init roller blind
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
#endif

	cl_Clear();

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

	return cl_CheckFix( id);
}		// CFSLAYER::cl_InitRollB
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_InitInscrn(			// init insect screen
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

	cl_Clear();

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

	return cl_CheckFix( id);
}		// CFSLAYER::cl_InitInscrn
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_InitDrape(			// init drape layer
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

	cl_Clear();

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

	return cl_CheckFix( id);
}		// CFSLAYER::cl_InitDrape
//------------------------------------------------------------------------------
void CFSLAYER::cl_Clear()		// clear CFS layer
{
	memset( ID, 0, sizeof( ID));
	LTYPE = ltyNONE;
	iGZS = 0;
	SWP_MAT.csw_Clear();
	LWP_MAT.clw_Clear();
	SWP_EL.csw_Clear();
	LWP_EL.clw_Clear();
	cl_ClearGeom();
}    // CFSLAYER::cl_Clear
//------------------------------------------------------------------------------
void CFSLAYER::cl_ClearGeom()		// clear CFS layer geometry values
{
	S = 0.;
	W = 0.;
	C = 0.;
	PHI_DEG = 0.;
	CNTRL = lscNONE;
}    // CFSLAYER::cl_ClearGeom
//-------------------------------------------------------------------------------
int CFSLAYER::cl_IsNEQ(		// compare CFSLAYERs
	const CFSLAYER& cl,			// compare cl to *this
	double tol /*=0.*/,			// relative tolerance
								//   0=require exact equality
	const char* what /*="?"*/,	// context for messages
	int optionsMAT /*=1*/,		// option bits for xxx_MAT comparison
								//   default: don't compare input
	int optionsEL /*=0*/) const	// option bits for xxx_EL comparison
								//   default: compare all
// returns 0 iff *this == cl
//    else minimum difference count
{
#if defined( _DEBUG)
#define XCI( m) vNEQMsg( m, cl.m, 0, what, "", #m, errCount)
#define XCD( m) vNEQMsg( m, cl.m, tol, what, "", #m, errCount)
#else
#define XCI( m) vNEQ( m, cl.m, 0)
#define XCD( m) vNEQ( m, cl.m, tol)
#endif
	[[maybe_unused]] int errCount = 0;
	int ret = XCI( LTYPE) _X_ XCI( iGZS) _X_ XCI( CNTRL)
		  _X_ XCD( S) _X_ XCD( W) _X_ XCD( C) _X_ XCD( PHI_DEG)
		  _X_ SWP_MAT.csw_IsNEQ( cl.SWP_MAT, tol, optionsMAT, what, "SWP.MAT")
		  _X_ LWP_MAT.clw_IsNEQ( cl.LWP_MAT, tol, optionsMAT, what, "LWP.MAT")
		  _X_ SWP_EL.csw_IsNEQ( cl.SWP_EL, tol, optionsEL, what, "SWP.EL")
		  _X_ LWP_EL.clw_IsNEQ( cl.LWP_EL, tol, optionsEL, what, "LWP.EL");
	return ret;
}		// CFSLAYER::cl_IsEqual
//------------------------------------------------------------------------------
bool CFSLAYER::cl_Reverse(			// reverse a layer
	CFSLAYER& LR) const		// returned: reversed layer
// returns true iff success
{
	string sID = FCGET( ID);
	sID += "_Rev";
	LR.cl_SetID( sID.c_str());
	
	// copy invariant mbrs
	LR.LTYPE = LTYPE;
	LR.S = S;
	LR.W = W;
	LR.C = C;
	LR.PHI_DEG = PHI_DEG != 0. ? -PHI_DEG : 0.;	// flip slat angle (avoid -0)
	LR.CNTRL = CNTRL;
	if (cl_IsVB())
	{	// blinds: slat properties don't change (up still up)
		LR.SWP_MAT = SWP_MAT;
		LR.LWP_MAT = LWP_MAT;
	}
    // else drapes?
    else
	{   // other types: F <-> B
		(LR.SWP_MAT = SWP_MAT).csw_Reverse();
		(LR.LWP_MAT = LWP_MAT).clw_Reverse();
		(LR.SWP_EL = SWP_EL).csw_Reverse();
		(LR.LWP_EL = LWP_EL).clw_Reverse();
    }

	return LR.cl_Finalize();	// insurance

}  // CFSLAYER::cl_Reverse
//------------------------------------------------------------------------------
bool CFSLAYER::cl_Finalize()	// finish layer construction
//   input: lType, LWP_MAT, SWP_MAT
//          geometry (per lType)
//   output: LWP_EL, SWP_EL
// returns true iff CFSLAYER is valid
{
	bool LOK = false;
	bool DOK = false;
	bool BOK = false;

	SWP_MAT.csw_ClearUnused( LTYPE);

	SWP_EL.csw_Clear();		// pre-clear equivalent layer (overkill)
	LWP_EL.clw_Clear();

	if (cl_IsVB())
	{	LOK = cl_VBLWP( LWP_EL);
		DOK = cl_VBSWP( SWP_EL);		// SW diffuse
		BOK = cl_VBSWP( SWP_EL, 0.);	// SW properties w/ profile ang = 0
	}
	else
	{	PHI_DEG = 0.;		// phi, C, CNTRL are VB only
		C = 0.;
		CNTRL = lscNONE;
		if (LTYPE == ltyDRAPE)
		{	LOK = cl_PDLWP( LWP_EL);
			DOK = cl_PDSWP( SWP_EL);			// SW diffuse
			BOK = cl_PDSWP( SWP_EL, 0., 0.);	// SW properties w/ profile angs = 0
		}
		else if (LTYPE == ltyINSCRN)
		{	LOK = cl_ISLWP( LWP_EL);		// LW
			DOK = cl_ISSWP(	SWP_EL);		// SW diffuse
			BOK = cl_ISSWP( SWP_EL, 0.);	// SW beam w/ theta = 0
		}
		else
		{	S = 0.;		// geometry mbrs unused
			W = 0.;
			if (LTYPE == ltyROLLB)
			{	LOK = cl_RBLWP( LWP_EL);		// LW
				DOK = cl_RBSWP( SWP_EL);		// SW diffuse
				BOK = cl_RBSWP( SWP_EL, 0.);	// SW beam w/ theta = 0
			}
			else if (cl_IsGZS())
			{   // spectral glazing
				BOK = cl_GZSInit() == 0;	// set layer xxx_MAT from GZS file data
				SWP_EL = SWP_MAT;
				LWP_EL = LWP_MAT;
				LOK = DOK = true;
			}
			else
			{	// glazing
				SWP_EL = SWP_MAT;
				LWP_EL = LWP_MAT;
				LOK = DOK = BOK = true;
			}
		}
	}
	return LOK && DOK && BOK;
}	// CFSLAYER::cl_Finalalize
//-----------------------------------------------------------------------------
int CFSLAYER::cl_GZSInit()		// one-time init for GZS layer
// returns 0 on success
{
#if 1
	return -1;		// TODO
#else
	if (iGZS <= 0)
		return -1;
	LWP_MAT = GZSTbl[ iGZS].LWP;
	SWP_MAT = GZSTbl[ iGZS].SWPBB;	// init to broadband
	return 0;
#endif
}  // CFSLAYER::cl_GZSInit
//------------------------------------------------------------------------------
bool CFSLAYER::cl_IsGZS() const		// true iff layer has GZS data from external file
{
	return LTYPE == ltyGZS;
}  // CFSLAYER::cl_IsGZS
//------------------------------------------------------------------------------
bool CFSLAYER::cl_IsGlazeX() const		// true iff layer is glazing (including GZS)
{
	return LTYPE == ltyGLAZE || cl_IsGZS();
}  // CFSLAYER::cl_IsGlazeX
//------------------------------------------------------------------------------
bool CFSLAYER::cl_IsControlledShade() const	// true iff layer properties changeable
{
	// See DoShadeControl
	return cl_IsVB() && CNTRL != lscNONE;
}  // CFSLAYER::cl_IsControlledShade
//------------------------------------------------------------------------------
bool CFSLAYER::cl_IsVB() const		// true iff layer is VB (slatted blind)
{
	return LTYPE == ltyVBHOR || LTYPE == ltyVBVER;
}  // CFSLAYER::cl_IsVB
//------------------------------------------------------------------------------
bool CFSLAYER::cl_IsShadeLayer() const	// true iff layer shade (not glaze or special)
{
	return LTYPE >= ltySHADE1 && LTYPE <= ltySHADEN;
}  // CFSLAYER::cl_IsShadeLayer
//==============================================================================

////////////////////////////////////////////////////////////////////////////////
// CFSSWP -- short wave properties
////////////////////////////////////////////////////////////////////////////////
CFSSWP::CFSSWP()
{	csw_Clear();
}		// CFSSWP::CFSSWP
//------------------------------------------------------------------------------
void CFSSWP::csw_Clear()		// clear short-wave properties
{
	TAUSFBB = TAUSBBB = RHOSFBB = RHOSBBB =
	TAUSFBD = TAUSBBD = RHOSFBD = RHOSBBD =
	TAUS_DD = RHOSFDD = RHOSBDD = 0.;
}    // CFSSWP::csw_Clear
//------------------------------------------------------------------------------
void CFSSWP::csw_ClearUnused(	// clear unused short-wave properties
	int lType)		// layer type
{
	if (lType == ltyVBVER || lType == ltyVBHOR)
	{   TAUSFBB = TAUSBBB = RHOSFBB = RHOSBBB =
		TAUSFBD = TAUSBBD = RHOSFBD = RHOSBBD = 0.;
	}
	else if (lType == ltyDRAPE)
	{	// RHOSFDD = RHOSBDD = TAUS_DD = 0.;
	}
	// else: add other types as needed
}    // CFSSWP::csw_ClearUnused
//------------------------------------------------------------------------------
int CFSSWP::csw_IsNEQ(	// compare short wave properties
	const CFSSWP& swp,			// compare swp to *this
	double tol /*=0.*/,			// absolute tolerance (0 sez require exact equality)
	int options /*=0*/,			// options:
								//   0: compare all
								//   1: compare only input
	[[maybe_unused]] const char* w1/*="?"*/,		// context for error messages
	[[maybe_unused]] const char* w2/*=?*/) const	// context for error messages
// returns 0 iff *this "equals" swp
//    else minimum # of errors found
{
#if defined( _DEBUG)
// debug version: all mbrs compared
int errCount=0;
#define XC( m) vNEQMsg( m, swp.m, tol, w1, w2, #m, errCount)
#else
// non-debug: use early out
#define XC( m) vNEQ( m, swp.m, tol)
#endif
	int ret = XC( TAUSFBB) _X_ XC( TAUSBBB) _X_ XC( RHOSFBB) _X_ XC( RHOSBBB)
		  _X_ XC( TAUSFBD) _X_ XC( TAUSBBD) _X_ XC( RHOSFBD) _X_ XC( RHOSBBD);

	if (!(options&1))
	{	// if compare inputs
#if !defined( _DEBUG)
		if (!ret)
#endif
			ret += XC( TAUS_DD) _X_ XC( RHOSFDD) _X_ XC( RHOSBDD);
	}

	return ret;
#undef XC
}  // CFSSWP::csw_IsEqual
//------------------------------------------------------------------------------
CFSSWP& CFSSWP::csw_Reverse()		// reverse short-wave properties
// back / front properties exhanged in place
{
	// beam-beam
	vswap( TAUSFBB, TAUSBBB);
	vswap( RHOSFBB, RHOSBBB);
	// beam-diffuse
	vswap( TAUSFBD, TAUSBBD);
	vswap( RHOSFBD, RHOSBBD);
	// diffuse-diffuse
	// TAUS_DD unchanged
	vswap( RHOSFDD, RHOSBDD);

	return *this;

}  // CFSSWP::csw_Reverse
//--------------------------------------------------------------------------
static int Specular_OffNormal(		// specular glazing off-normal property ratios

	double THETA,		// solar beam angle of incidence from normal, radians
						//	0 <= THETA <= PI/2
	double& RAT_1MR,	// returned: ratio of off-normal to normal solar (1-reflectance)
						//   NOTE: rhoAdj = 1-(1-rho)*RAT_1MR
	double& RAT_TAU)	// returned: ratio of off-normal to normal solar transmittance

// returns 2 iff RAT_TAU < 1 or RAT_1MR < 1 (and thus csw_SpecularAdjust s/b called)
//         1 iff RAT_TAU==1 && RAT_1MR==1 (no need to call csw_SpecularAdjust)
//         0 if error (none currently defined)

{
	int ret = 2;
	THETA = abs( THETA);
	if (THETA > PI/2. - DTOR)
	{	// theta > 89 deg
		RAT_TAU = 0.;
		RAT_1MR = 0.;
	}
	else if (THETA >= DTOR)		// if theta >= 1 deg
	{
		// N2 = reference refractive index for generating general off-normal curves
		static const double N2=1.526;

		// KL = extinction coefficient*thickness product, also used as a
		//	reference value to generate off-normal curves
		static const double KL =55.*0.006;

		static const double TAU_A0 = exp(-KL);
		static const double RPERP0=Square((N2-1.)/(N2+1.));
		static const double TAU0 = TAU_A0*Square(1.-RPERP0) / (1.-Square( RPERP0*TAU_A0));
		static const double RHO0 = RPERP0*(1.+TAU_A0*TAU0);

		double THETA2 = asin( sin(THETA)/N2);
		double TAU_A = exp( -KL/cos(THETA2));
		// RPERP, RPARL = interface reflectance with respect to perpendicular
		//                and parallel polarization components of solar rad'n
		double RPERP = Square( sin( THETA2-THETA)/ sin( THETA2+THETA));
		double RPARL = Square( tan( THETA2-THETA)/ tan( THETA2+THETA));
		double TAUPERP = TAU_A * Square(1.-RPERP) / (1. - Square( RPERP*TAU_A));
		double TAUPARL = TAU_A * Square(1.-RPARL) / (1. - Square( RPARL*TAU_A));
		double RHOPERP = RPERP*(1.+(TAU_A*TAUPERP));
		double RHOPARL = RPARL*(1.+(TAU_A*TAUPARL));
		double TAU_ON=(TAUPERP+TAUPARL)/2.;
		double RHO_ON=(RHOPERP+RHOPARL)/2.;
		RAT_TAU = TAU_ON/TAU0;
		RAT_1MR = (1.-RHO_ON)/(1.-RHO0);
	}
	else
	{	RAT_TAU = RAT_1MR = 1.;
		ret = 1;
	}
	return ret;

}  // ::Specular_OffNormal
//------------------------------------------------------------------------------
bool CFSSWP::csw_SpecularOffNormal(		// specular glazing off-normal short wave properties
	double theta)				// incident angle, radians
// properties adjusted in place
// returns true iff success
{
	double RAT_1MR, RAT_TAU;			// adjustment factors, see Specular_OffNormal()
	int ret = ::Specular_OffNormal( theta, RAT_1MR, RAT_TAU);
	if (ret == 2)
		csw_SpecularAdjust( RAT_1MR, RAT_TAU);
	return ret > 0;
}    // CFSSWP::csw_Specular
//------------------------------------------------------------------------------
void CFSSWP::csw_SpecularAdjust(		// adjust properties
	double RAT_1MR,		// adjustment factors, see Specular_OffNormal()
	double RAT_TAU)
// properties adjusted in place
{
	TAUSFBB *= RAT_TAU;
	TAUSBBB *= RAT_TAU;
	RHOSFBB = 1.-RAT_1MR * (1.-RHOSFBB);
	RHOSBBB = 1.-RAT_1MR * (1.-RHOSBBB);
}    // CFSSWP::csw_SpecularAdjust
//----------------------------------------------------------------------------
static double Specular_F(		// integrand fcn for specular properties
	double THETA,		// incidence angle, radians
	[[maybe_unused]] const HEMINTP& P,	// parameters
	int opt)			// options: what proterty to return
{
	double RAT_TAU, RAT_1MR;
	::Specular_OffNormal( THETA, RAT_1MR, RAT_TAU);

	double ret = opt == hipRHO ? RAT_1MR
			   : opt == hipTAU ? RAT_TAU
			   :                 -1.;
	return ret;
}  // Specular_F
//--------------------------------------------------------------------
void CFSSWP::csw_SpecularRATDiff(
	double& RAT_1MRDiff,		// returned: (1-rho) ratio
	double& RAT_TAUDiff)		// returned: tau ratio
// returns property ratios re estimating diffuse properties
{
static double X1MRDiff = -1.;
static double XTAUDiff = -1.;

	if (XTAUDiff < 0.)
	{	// calculate and save on first call
		HEMINTP P( 0., 0., 0.);
		X1MRDiff = HEMINT( Specular_F, P, hipRHO);
		XTAUDiff = HEMINT( Specular_F, P, hipTAU);
	}
	RAT_TAUDiff = XTAUDiff;
	RAT_1MRDiff = X1MRDiff;
}    // CFSSWP::csw_SpecularRATDiff
//--------------------------------------------------------------------
void CFSSWP::csw_SpecularEstimateDiffuseProps()		// diffuse properties
// derives diffuse propeties from beam properties
{
	double RAT_TAU, RAT_1MR;
	csw_SpecularRATDiff( RAT_1MR, RAT_TAU);
	TAUS_DD =    RAT_TAU * TAUSFBB;
	RHOSFDD = 1.-RAT_1MR * (1.-RHOSFBB);
	RHOSBDD = 1.-RAT_1MR * (1.-RHOSBBB);
}    // CFSSWP::csw_SpecularEstimateDiffuseProps

//-----------------------------------------------------------------------------
string CFSSWP::DbFmt( const char* tag/*=""*/) const	// formatted string for e.g. DbDump()
// returns self-description w/o \n
{
	return stringFmt(
		"%sSW TBB=%.3f/%.3f RBB=%.3f/%.3f  TBD=%.3f/%.3f RBD=%.3f/%.3f  TDD=%.3f RDD=%.3f/%.3f",
			tag,
			TAUSFBB, TAUSBBB, RHOSFBB, RHOSBBB,
			TAUSFBD, TAUSBBD, RHOSFBD, RHOSBBD,
			TAUS_DD, RHOSFDD, RHOSBDD);

}	// CFSSWP::DbFmt
//===============================================================================

/////////////////////////////////////////////////////////////////////////////////
// CFSLWP
/////////////////////////////////////////////////////////////////////////////////
CFSLWP::CFSLWP()		// c'tor
{	clw_Clear();
}	// CFSLWP::CFSLWP
//------------------------------------------------------------------------------
void CFSLWP::clw_Clear()		// clear long-wave properties
{
	EPSLF = 0.;
	EPSLB = 0.;
	TAUL = 0.;
}    // CFSLWP::clw_Clear
//------------------------------------------------------------------------------
int CFSLWP::clw_IsNEQ(	// compare long wave properties
	const CFSLWP& lwp,			// compare lwp to *this
	double tol /*=0.*/,			// relative tolerance
								//   0=require exact equality
	int options/*=0*/,			// options (unused)
	[[maybe_unused]] const char* w1/*="?"*/,		// context for msgs
	[[maybe_unused]] const char* w2/*="?"*/) const
{
#if defined( _DEBUG)
// debug version: all mbrs compared
int errCount=0;
#define XC( m) vNEQMsg( m, lwp.m, tol, w1, w2, #m, errCount)
#else
// non-debug: use early out
#define XC( m) vNEQ( m, lwp.m, tol)
#endif
	options;
	int ret = XC( EPSLF) _X_ XC( EPSLB) _X_ XC( TAUL);
	return ret;
#undef XC
}  // CFSLWP::clw_IsEqual
//------------------------------------------------------------------------------
CFSLWP& CFSLWP::clw_Reverse()	// reverse long-wave properties in place
// exchanges F/B properties
{
	vswap( EPSLF, EPSLB);
	// TAUL unchanged
	return *this;
}  // CFSLWP::clw_Reverse
//------------------------------------------------------------------------------
string CFSLWP::DbFmt(		// formatted string for debugging
	const char* tag/*=""*/) const
// returns self-description
{
	return stringFmt( "%sLW EF=%0.3f EB=%0.3f T=%0.3f",
		tag, EPSLF, EPSLB, TAUL);
}		// CFSLWP::DbFmt
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CFSGAP
///////////////////////////////////////////////////////////////////////////////
CFSGAP::CFSGAP()
{	cg_Clear();
}	// CFSGAP::CFSGAP
//------------------------------------------------------------------------------
void CFSGAP::cg_Clear()			// clear gap
{
	GTYPE = 0;
	TAS = 0.;
	TAS_EFF = 0.;
	FG.cfg_Clear();
	RHOGAS = 0.;
}    // CFSGAP::cg_Clear
//------------------------------------------------------------------------------
bool CFSGAP::cg_Build(
	const char* _FGID,	// ID of gap fill gas
	double _TAS,		// gap thickness, mm
	int _GType,			// gap type (gtyOPENin, gtyOPENout or gtySEALED)
	double _TMan /*=21.*/,	// temp at time of manufacturer, C
	double _PMan /*=-1*/)	// pressure at time of manufacture, Pa
// returns true iff FGID found
//    else false, fill gas set to air
{
	TAS = TAS_EFF = _TAS;	// actual and effective gap thickness
							//   effective my be modified, see AdjustVBGap()
	GTYPE = _GType;
	if (_PMan < 0.)
		_PMan = PATM;
	bool bRet = FG.cfg_GetFillGas( _FGID);	// returns Air if _FGID not found
	RHOGAS = FG.cfg_Density( _PMan, _TMan+TKAdd);
	return bRet;
}  // CFGGAP::cg_BuildGap
//------------------------------------------------------------------------------
void CFSGAP::cg_AdjustVBGap(		// adjust gap adjacent to slatted blind
	const CFSLAYER& L)				// adjacent layer
// returns *this updated
{
	if (!L.cl_IsVB())
		return; // insurance

	// treat VB layer as if it has 70% of actual thickness
	// See Wright, J.L., Huang, N.Y.T., Collins, M.R.
	//     "Thermal Resistance of a Window with an Enclosed Venetian Blind: A Simplified Model"
	//     ASHRAE Transactions (2007?)
	double VBTHICK = L.W * cos( DTOR * L.PHI_DEG);	// VB layer thickness at slat angle
	TAS_EFF = TAS + (L.W - 0.7*VBTHICK)/2.;
}    // CFSGAP::cg_AdjustVBGap
//---------------------------------------------------------------------
double CFSGAP::cg_HConvGap(		// convective coeff for gap
	double T1,		// bounding surface temps, K
	double T2)
// returns convective coefficient, W/m2-K
{
	double T = TAS_EFF / 1000.;	// gap thickness, m
	double TM = (T1 + T2) / 2.;
	double DT = T1 - T2;
	double RA=FRA( TM, T, DT, FG.AK, FG.BK,
			FG.ACP, FG.BCP, FG.AVISC, FG.BVISC, RHOGAS);
	double NU=FNU( RA);
	//	if (NU > 1.) ALPHA=DOT(S-PI/6.)/(PI/3.)*0.5+0.5
	double KGAS = FG.AK + FG.BK*TM;
	double hc = NU*KGAS/T;
	return hc;
}  // CFSGAP::cg_HConvGap
//-----------------------------------------------------------------------------
string CFSGAP::DbFmt( const char* tag/*=""*/) const
// returns self-description w/o \n
{
	return stringFmt( "%s%s ty=%d W=%0.3f Weff=%0.3f",
		tag,
		FCGET( FG.ID), GTYPE,
		TAS/25.4, TAS_EFF/25.4);		// gap width: mm to inches
}		// CFSGAP::DbFmt
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CFSFILLGAS
///////////////////////////////////////////////////////////////////////////////
CFSFILLGAS::CFSFILLGAS()
{	cfg_Clear();
}	// CFSGILLGAS::CFSFILLGAS
//-----------------------------------------------------------------------------
CFSFILLGAS::CFSFILLGAS(
	const char* id, double ak, double bk, double acp, double bcp,
	double avisc, double bvisc, double mHat)
{	Init( id, ak, bk, acp, bcp, avisc, bvisc, mHat);
}		// CFSGILLGAS::CFSFILLGAS
//-----------------------------------------------------------------------------
void CFSFILLGAS::Init(
	const char* id, double ak, double bk, double acp, double bcp,
	double avisc, double bvisc, double mHat)
{	strcpy( ID, id);		// TODO
	AK = ak; BK = bk; AVISC = avisc; BVISC = bvisc;
	ACP = acp; BCP = bcp; MHAT = mHat;
}	// CFSFILLGASS::Init
//-----------------------------------------------------------------------------
void CFSFILLGAS::cfg_Clear()		// clear fill gas properties
{
	memset( ID, 0, sizeof( ID));
	AK = 0.;
	BK = 0.;
	ACP = 0.;
	BCP = 0.;
	AVISC = 0.;
	BVISC = 0.;
	MHAT = 0.;
}    // CFSFILLGAS::cfg_Clear
//-----------------------------------------------------------------------------
bool CFSFILLGAS::cfg_GetFillGas(		// init from ID
	const char* FGID)		// fill gas ID sought ("Argon")
// returns true iff FGID found, *this filled with properties
//    else false, *this set to Air properties
{
// Fill gases and their properties
//  Pre-defined, no input provided
//  NOTE: FGX[ 0] must be Air
static CFSFILLGAS FGX[] =
{ //                      AK        BK       ACP	    BCP        AVISC     BVISC     MHAT
  CFSFILLGAS( "Air",     2.873E-3, 7.76E-5, 1002.737, 1.2324E-2, 3.723E-6, 4.94E-8,  28.97),
  CFSFILLGAS( "Argon",   2.285E-3, 5.149E-5,521.9285, 0,         3.379E-6, 6.451E-8, 39.948),
  CFSFILLGAS( "Krypton", 0.9443E-3,2.826E-5,248.0907, 0.,        2.213E-6, 7.777E-8, 83.8),
  CFSFILLGAS( "Xenon",   0.4538E-3,1.723E-5,158.3397, 0.,        1.069E-6, 7.414E-8, 131.30),
  CFSFILLGAS()
};
	bool bFound = false;
	for (int iFG=0; !bFound && FGX[ iFG].ID[ 0]; iFG++)
	{	if (stricmp( FGID, FGX[ iFG].ID)==0)
		{	*this = FGX[ iFG];
			bFound = true;
		}
	}
	if (!bFound)
		*this = FGX[ 0];		// air
	return bFound;
}  // CFGFILLGAS::cfg_GetFillGas
//------------------------------------------------------------------------------
double CFSFILLGAS::cfg_Density(		// calculate gas density
	double P,			// pressure, Pa
	double T) const		// temperature, K
// returns gas density at P and T, kg/m3
{
	return P*MHAT/(R_UNIV*max( T, 1.)*1000.);
}  // CFSFILLGAS::cfg_Density
//------------------------------------------------------------------------------
int CFSTY::cf_NGlz() const		// count glazing layers
{
	int nGlz = 0;
	for (int iL=1; iL<NL; iL++)
		nGlz += L( iL).cl_IsGlazeX();
	return nGlz;
}  // CFSTY::cf_NGlz
//------------------------------------------------------------------------------
int CFSTY::cf_HasShade() const	// check for shade layer
// returns idx of outermost shade
//   0 if no shade
{
	int nGlz, nShd, iLShd;
	cf_Info( nGlz, nShd, iLShd);
	return iLShd;
}  // CFSTY::cf_HasShade
//------------------------------------------------------------------------------
int CFSTY::cf_Info(			// info about this CFSTY
	int& nGlz,			// returned: # of glazing layers
	int& nShd,			// returned: # of shade (non-glazing) layers
	int& iLShd) const	// returned: layer idx of 1st (outermost) shade
						//    0=no shade, 1=outside, NL=inside
// returns total # of layers
{
	nGlz = nShd = iLShd = 0;
	for (int iL=1; iL<=NL; iL++)
	{	if ( L( iL).cl_IsGlazeX())
			nGlz++;
		else
		{   nShd++;
			if (iLShd == 0)
				iLShd = iL;
		}
	}
	return NL;
}  // CFSTY::cf_Info
//------------------------------------------------------------------------------
int CFSTY::cf_HasControlledShade() const	// check for controlled shade layer
// returns 1-based idx of outermost controlled shade layer
//         0 if none
{
	for (int iL=1; iL<=NL; iL++)
		if (L( iL).cl_IsControlledShade())
			return iL;
	return 0;
}  // CFSTY::cf_HasControlledShade
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_CheckFix(		// verify CFSLAYER validity
	vector< string> &msgs,	// message array (new messages appended)
	const char* what)		// context for message(s)
// set bad items to valid defaults if possible
// returns true iff all data OK
//    else false (msgs populated)
{
	// set missing values to defaults
	cl_FillDefaultsGeom( SWP_MAT);
	cl_FillDefaultsLWP( LWP_MAT);
	cl_FillDefaultsSWP( SWP_MAT);

	msgs.clear();
	bool LWPOK = cl_CheckFixLWP( LWP_MAT, msgs, what);
	bool SWPOK = cl_CheckFixSWP( SWP_MAT, msgs, what);
	bool GOK = cl_CheckFixGeom( msgs, what);
	bool FLOK = cl_Finalize();
	return LWPOK && SWPOK && GOK && FLOK;
}  // CFSLAYER::cl_CheckFix
//-----------------------------------------------------------------------------
bool CFSLAYER::cl_CheckFix(		// verify CFSLAYER validity w/ message reporting)
	const char* what)		// context for message(s)
// set bad items to valid defaults if possible
// returns true iff all data OK
//    else false (msgs reported)
{
	vector< string> msgs;
	bool bRet = cl_CheckFix( msgs, what);
	if (!bRet)
	{	vector< string>::const_iterator  pos;
		for (pos=msgs.begin(); pos!=msgs.end(); ++pos)
			Message( msgWRN, pos->c_str());
	}
	return bRet;
}	// CFSLAYER::cl_CheckFix
//------------------------------------------------------------------------------
void CFSLAYER::cl_FillDefaultsGeom(
	const CFSSWP& swp)	// properties (may be within *this)
{
	if (LTYPE == ltyINSCRN)
	{	if (S <= 0. && swp.TAUSFBB >= 0.)
		{	// set wire spacing and diameter from openness
			S = 1.;
			W = IS_DSRATIO( swp.TAUSFBB);
		}
	}
}    // CFSLAYER::cl_FillDefaultsGeom
//------------------------------------------------------------------------------
void CFSLAYER::cl_FillDefaultsLWP(	// default long wave properties
	CFSLWP& lwp)	// properties to fill (may be within *this)
{
	if (LTYPE == ltyGLAZE)
	{   if (lwp.EPSLF < 0.) lwp.EPSLF = .84;
		if (lwp.EPSLB < 0.) lwp.EPSLB = .84;
		if (lwp.TAUL < 0.) lwp.TAUL = 0.;	// glass: no LW trans
	}
	else if (LTYPE == ltyVBHOR || LTYPE == ltyVBVER)
	{	if (lwp.EPSLF < 0.) lwp.EPSLF = .90;
		if (lwp.EPSLB < 0.) lwp.EPSLB = .90;
		if (lwp.TAUL < 0.) lwp.TAUL = 0.;	// slat: no LW trans
	}
	else if (LTYPE == ltyDRAPE)
	{	// default based on Kotey data
		if (lwp.EPSLF < 0.) lwp.EPSLF = .87;
		if (lwp.EPSLB < 0.) lwp.EPSLB = .87;
		if (lwp.TAUL < 0.) lwp.TAUL = 0.05;	// apparent T
	}
	else if (LTYPE == ltyROLLB)
	{	// default based on Kotey data
		if (lwp.EPSLF < 0.) lwp.EPSLF = .91;
		if (lwp.EPSLB < 0.) lwp.EPSLB = .91;
		if (lwp.TAUL < 0.) lwp.TAUL = 0.05;	// apparent T
	}
	else if (LTYPE == ltyINSCRN)
	{	// default based on Kotey data
		if (lwp.EPSLF < 0.) lwp.EPSLF = .93;
		if (lwp.EPSLB < 0.) lwp.EPSLB = .93;
		if (lwp.TAUL < 0.) lwp.TAUL = 0.02;	// apparent T
	}
	else if (LTYPE == ltyNONE || LTYPE == ltyROOM)
		;  // none or room: do nothing
	else
		Message( msgERR, "CFSLAYER::cl_FillDefaultsLWP: bad LTYPE=%d", LTYPE);

}    // CFSLAYER::cl_FillDefaultsLWP
//------------------------------------------------------------------------------
void CFSLAYER::cl_FillDefaultsSWP(		// fill defaulted properties
	CFSSWP& swp)		// properties to fill (may be within *this)
{
	// default back taus to front (often equal)
	if (swp.TAUSBBB < 0.)
		swp.TAUSBBB = swp.TAUSFBB;
	if( swp.TAUSBBD < 0.)
		swp.TAUSBBD = swp.TAUSFBD;

	if (LTYPE == ltyGLAZE)
	{	// estimate diffuse properties if any < 0
		if (swp.RHOSBDD<0. || swp.RHOSFDD<0. || swp.TAUS_DD<0.)
			swp.csw_SpecularEstimateDiffuseProps();
	}
	else if (LTYPE == ltyVBHOR || LTYPE == ltyVBVER)
		;	// nothing to do
	else if (LTYPE == ltyDRAPE)
	{	// estimate diffuse properties if any < 0
		if (swp.RHOSBDD<0. || swp.RHOSFDD<0. || swp.TAUS_DD<0.)
			swp.csw_FabricEstimateDiffuseProps();
	}
	else if (LTYPE == ltyROLLB)
	{	// estimate diffuse properties if any < 0
		if (swp.RHOSBDD<0. || swp.RHOSFDD<0. || swp.TAUS_DD<0.)
			cl_RBSWP( swp);	// TODO RB
	}
	else if (LTYPE == ltyINSCRN)
	{	if (swp.TAUSFBB < 0.)
		{	swp.TAUSFBB = IS_OPENNESS( S, W);
			if (swp.TAUSBBB < 0.)
				swp.TAUSBBB = swp.TAUSFBB;
		}
		if (swp.RHOSBDD<0. || swp.RHOSFDD<0. || swp.TAUS_DD<0.)
			cl_ISSWP( swp);	// TODO IS
	}
	else if (LTYPE == ltyNONE || LTYPE == ltyROOM)
		;	// none or room: do nothing
	else
		Message( msgERR, "CFSLAYER::cl_FillDefaultsSWP: missing case");
}    // CFSLAYER::cl_FillDefaultsSWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_CheckFixLWP(	// check layer long wave properties
	CFSLWP& lwp,			// properties (may be within *this)
	vector< string> &msgs,	// msg array
	const char* what)		// context for messages
// returns true iff LWP OK
//    else false, LWP.xxx changed to legal but NOT necessarily consistent values
//                Layer should not be used
{
	bool L1 = CheckFixPropPair( 2, lwp.EPSLF, "EPSLF", lwp.TAUL, "TAUL", msgs, what);
	bool L2 = CheckFixPropPair( 2, lwp.EPSLB, "EPSLB", lwp.TAUL, "TAUL", msgs, what);

	return L1 && L2;

}  // CFSLAYER::cl_CheckFixLWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_CheckFixSWP(	// check short wave properties
	CFSSWP& swp,			// properties (may be within *this)
	vector< string> &msgs,	// msg array
	const char* what)		// context for messages

// checks input layer properties
// NOTE: equivalent layer properties may fail here, do not call for SWL_EL
// returns TRUE iff SWP OK
//    else FALSE, swp.xxx changed to legal but NOT necessarily consistent values
//                Layer should not be used
{
	size_t msgsSizeStart = msgs.size();
	bool bRet = true;
	if (LTYPE==ltyGLAZE)
	{	// no frosted / figured glass support -> all BD values must be 0
		//  TODO: verify 10-26-08
		if (swp.RHOSFBD != 0. || swp.RHOSBBD != 0.
		 || swp.TAUSFBD != 0. || swp.TAUSBBD != 0.)
		{	bRet = AddMsgV( msgs, what, "RHOSFBD, RHOSBBD, TAUSFBD, and TAUSBBD must be 0");
			swp.RHOSFBD=0.;
			swp.RHOSBBD=0.;
			swp.TAUSFBD=0.;
			swp.TAUSBBD=0.;
		}
	}
	else if (cl_IsVB())
	{	// VB: all beam properties unused (assume xxxbBD = xxxxDD)
		if (swp.RHOSFBB != 0. || swp.RHOSBBB != 0.
		 || swp.TAUSFBB != 0. || swp.TAUSBBB != 0.
		 || swp.RHOSFBD != 0. || swp.RHOSBBD != 0.
		 || swp.TAUSFBD != 0. || swp.TAUSBBD != 0.)
		{	bRet = AddMsgV( msgs, what, "all beam properties must be 0");
			swp.RHOSFBB=0.;
			swp.RHOSBBB=0.;
			swp.TAUSFBB=0.;
			swp.TAUSBBB=0.;
			swp.RHOSFBD=0.;
			swp.RHOSBBD=0.;
			swp.TAUSFBD=0.;
			swp.TAUSBBD=0.;
		}
	}
	else if (LTYPE==ltyDRAPE || LTYPE == ltyROLLB || LTYPE == ltyINSCRN)
	{	// no specular reflectance
		if (  swp.RHOSFBB != 0. || swp.RHOSBBB != 0.)
		{	bRet = AddMsgV( msgs, what, "RHOSFBB and RHOSBBB must be 0");
			swp.RHOSFBB=0.;
			swp.RHOSBBB=0.;
		}
	}

	if (LTYPE==ltyROLLB || LTYPE==ltyINSCRN)
	{	// diffuse properties internally derived
		if ( swp.RHOSFDD != 0. || swp.RHOSBDD != 0. || swp.TAUS_DD != 0.)
		{	bRet = AddMsgV( msgs, what, "RHOSFDD, RHOSBDD, and TAUS_DD must be 0");
			swp.RHOSFDD=0.;
			swp.RHOSBDD=0.;
			swp.TAUS_DD=0.;
		}
	}

	if (LTYPE==ltyROLLB)
	{	CheckFixPropRange( swp.TAUSFBB, "TAUSFBB (openness)", msgs, what, 0., .2);
		swp.TAUSBBB = swp.TAUSFBB;
	}

	// beam-beam front/back taus must be equal for all types
	// (beam-diffuse may differ)
	CheckFixPropPair( 1, swp.TAUSFBB, "TAUSFBB", swp.TAUSBBB, "TAUSBBB", msgs, what);

	//   rho+tau <= 1
	CheckFixPropPair( 2, swp.RHOSFBB, "RHOSFBB", swp.TAUSFBB, "TAUSFBB", msgs, what);
	CheckFixPropPair( 2, swp.RHOSBBB, "RHOSBBB", swp.TAUSBBB, "TAUSBBB", msgs, what);
	CheckFixPropPair( 2, swp.RHOSFBD, "RHOSFBD", swp.TAUSFBD, "TAUSFBD", msgs, what);
	CheckFixPropPair( 2, swp.RHOSBBD, "RHOSBBD", swp.TAUSBBD, "TAUSBBD", msgs, what);
	CheckFixPropPair( 2, swp.RHOSFDD, "RHOSFDD", swp.TAUS_DD, "TAUS_DD", msgs, what);
	CheckFixPropPair( 2, swp.RHOSBBD, "RHOSBDD", swp.TAUS_DD, "TAUS_DD", msgs, what);
	// tau beam-beam + tau beam-diff <= 1
	CheckFixPropPair( 2, swp.TAUSFBB, "TAUSFBB", swp.TAUSFBD, "TAUSFBD", msgs, what);
	CheckFixPropPair( 2, swp.TAUSBBB, "TAUSBBB", swp.TAUSBBD, "TAUSBBD", msgs, what);
	// rho beam-beam + rho beam-diff <= 1
	CheckFixPropPair( 2, swp.RHOSFBB, "RHOSFBB", swp.RHOSFBD, "RHOSFBD", msgs, what);
	CheckFixPropPair( 2, swp.RHOSBBB, "RHOSBBB", swp.RHOSBBD, "RHOSBBD", msgs, what);
	// tau BT + rho BT <= 1
	CheckFixProp2Pair( 2, swp.RHOSFBB, swp.RHOSFBD, "RHOSFBT",
							   swp.TAUSFBB, swp.TAUSFBD, "TAUSFBT", msgs, what);
	CheckFixProp2Pair( 2, swp.RHOSBBB, swp.RHOSBBD, "RHOSBBT",
							   swp.TAUSBBB, swp.TAUSBBD, "TAUSBBT", msgs, what);

	int msgsAdded = msgs.size() > msgsSizeStart;
	return bRet && msgsAdded == 0;

}  // CFSLAYER::cl_CheckFixSWP
//------------------------------------------------------------------------------
bool CFSLAYER::cl_CheckFixGeom(		// check and apply limits to geometric values
	vector< string> &msgs,		// msg array (any new messages appended)
	const char* what)			// context for messages
// returns true iff no corrections
//         false otherwise, msg(s) added
{
	bool bRet = true;
	if (cl_IsVB())
	{	if (S < .001)
		{	bRet = AddMsgV( msgs, what, "slat spacing must be > 0");
			S = 1.;
		}
		if (W < .001 || W > 2.*S)
		{	bRet = AddMsgV( msgs, what, "slat width range is 0 < W <= 2*S");
			W = S;
		}
		if (C < 0. || C >= W/2.)
		{	bRet = AddMsgV( msgs, what, "crown range is 0 <= C < W/2");
			C = 0.;
		}
		if (PHI_DEG < -90. || PHI_DEG > 90.)
		{	bRet = AddMsgV( msgs, what, "slat angle range is -90 <= phi <= 90");
			PHI_DEG = 0.;
		}
	}
	else if (LTYPE == ltyINSCRN)
	{	if (S < .00001)
		{	bRet = AddMsgV( msgs, what, "wire spacing must be > 0");
			S = 1.;
		}
		if (W < .00001 || W > S)
		{	bRet = AddMsgV( msgs, what, "wire diameter range 0 < W <= S");
			W = S / 5.;
		}
	}
	else if (LTYPE == ltyDRAPE)
	{	if (S < .001)
		{	bRet = AddMsgV( msgs, what, "pleat spacing must be > 0");
			S = 1.;
		}
		if (W < 0)
		{	bRet = AddMsgV( msgs, what, "pleat depth must be >= 0");
			W = S;
		}
	}
	// else set geometric values to 0?

	return bRet;
}  // CFSLAYER::cl_CheckFixGeom
//------------------------------------------------------------------------------
bool CFSTY::cf_Finalize(			// complete CFS after BuildCFS or data input
	string* pMsg/*=NULL*/)		// optionally returned: error msg(s)
// returns true iff OK
//         false if error (msg explains)
{
	string msg;
	bool LVBPREV = false;		// true iff previous layer is VB
	for (int iL=1; iL<=NL; iL++)
	{
		if (!L( iL).cl_IsVB())
			LVBPREV = false;
		else if (LVBPREV)
			AppendMsg( msg, "adjacent VB layers", "; ");
		else
		{   LVBPREV = true;
			if (iL > 1)
				G( iL-1).cg_AdjustVBGap( L( iL));
			if (iL < NL)
				G( iL).cg_AdjustVBGap( L( iL));
		}
		if (iL < NL)
		{	int gType = G( iL).GTYPE;
			if (gType == gtyOPENOut && iL != 1)
				AppendMsg( msg, "OpenOut gap not outermost", "; ");
			if (gType == gtyOPENIn && iL != NL-1)
				AppendMsg( msg, "OpenIn gap not innermost", "; ");
		}
	}
	if (pMsg)
		*pMsg = msg;

	return msg.length() == 0;

}  // CFSTY::cf_Finalize
//------------------------------------------------------------------------------
int CFSTY::cf_Append(		// append CFS
	const CFSTY& FS,		// source CFS to append to *this
	bool bRev /*=false*/)	// true: reverse FS
// returns 0 on success
//         -1 if too many total layers (*this unchanged)
//          1 if reverse trouble
{
	if (NL + FS.NL > CFSMAXNL)
		return -1;

	int ret = 0;
	for (int iLX=1; iLX <= FS.NL; iLX++)
	{	int iGX = -1;
		CFSLAYER LX;
		if (bRev)
		{	int iLXR = FS.NL-iLX+1;
			if (!FS.L( iLXR).cl_Reverse( LX))
			{	ret = 1;
				if (iLXR > 1)
					iGX = iLXR - 1;
			}
			else
			{	LX = FS.L( iLX);
				if (iLX < FS.NL)
					iGX = iLX;
			}
			int iL = NL++;
			L( iL) = LX;
			if (iGX > 0)
				G( iL) = FS.G( iGX);
		}
	}
	return ret;
}  // CFSTY::cf_Append
//------------------------------------------------------------------------------
bool CFSTY::cf_IsSealed() const
// returns true iff both faces are LW opaque and have sealed gaps
// WHY (example): surface temp / U-factor consistency checks are possible if sealed
{
	bool bRet = false;
	if (NL > 0 && L( 1).LWP_EL.TAUL < .001)
	{	if (NL == 1)
			bRet = true;
		else if (G( 1).GTYPE == gtySEALED && L( NL).LWP_EL.TAUL < .001)
			bRet = NL==2 || G( NL-1).GTYPE == gtySEALED;
	}
	return bRet;
}  // CFSTY::cf_IsSealed
//------------------------------------------------------------------------------
double CFSTY::cf_EffectiveEPSLF() const	// effective outside LW eps
// handles partially transparent layers
{
	double E = 0.;
	double TX = 1.;
	for (int iL=1; iL<=NL+1; iL++)
	{	if (iL == NL+1)
			E += .9 * TX;
		else
		{	E += L( iL).LWP_EL.EPSLF * TX;
			if (L( iL).LWP_EL.TAUL < .001)
				break;	// opaque layer, stop
			TX *= L( iL).LWP_EL.TAUL;
		}
	}
	return E;
}  // CFSTY::cf_EffectiveEPSLF
//------------------------------------------------------------------------------
double CFSTY::cf_EffectiveEPSLB() const		// effective inside (room side) LW eps
// handles partially transparent layers
{
	double E = 0.;
	double TX = 1.;
	for (int iL=NL; iL>=0; iL--)
	{	if (iL == 0)
			E += .9 * TX;
		else
		{	E += L( iL).LWP_EL.EPSLB * TX;
			if (L( iL).LWP_EL.TAUL < .001)
				break;		// opaque layer, stop
			TX *= L( iL).LWP_EL.TAUL;
		}
	}
	return E;
}  // CFSTY::cf_EffectiveEPSLB

// ashwat.cpp end

