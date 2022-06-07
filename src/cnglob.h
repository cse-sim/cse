// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// cnglob.h: Global definitions for CSE: include first in all files.
///////////////////////////////////////////////////////////////////////////////

// rework for Microsoft Visual Studio 3-17-10

// #defines assumed on compiler command line
// WIN		build Windows appl, screen output to window (not maintained 9-12)
// DLL		build Windows DLL, screen output to window (not maintained 9-12)
// CSE_DLL	build "silent" CSE DLL, screen output returned via callback
// else CSE_CONSOLE  build console app, screen output to cmd window

//--- Options in cndefns.h (eg for use in cnrecs.def), now #included below in this file ---
//
//undef or
//#define BINRES	define for code to output binary results files, 11-93.
//
//#define SHINTERP	define for subhour-interpolated weather data, 1-95.
//#undef SOLAVNEND	undef for hour/subhour-average interpolated solar
//

/*----------------------------- compiling for -----------------------------*/

// configuration (from compiler command line)
#if defined(WIN) || defined(DLL)	// if compiling for Windows .exe application (has own copy of all obj's)
									// or compiling for Windows .dll library (has own separate copy of obj's)
  #define WINorDLL		// define combined symbol for convenience.
  #if defined( DLL)
    #define _DLLImpExp __declspec( dllexport)
  #endif
#elif defined( CSE_DLL)
  #define _DLLImpExp __declspec( dllexport)
  #define LOGCALLBACK		// send screen messages to caller via callback
#else						// otherwise
  #define CSE_CONSOLE		// say compiling for console operation (under Windows)
#endif

#if !defined( _DLLImpExp)
  #define _DLLImpExp
#endif

//--- release versus debugging version. Also obj dir, compile optns, link optns, exe name may be changed by make file.
//DEBUG 	define to include extra checks & messages -- desirable to leave in during (early only?) user testing
//DEBUG2	define to include devel aids that are expensive or will be mainly deliberately used by tester.
//NDEBUG	define to REMOVE ASSERT macros (below) (and assert macros, assert.h)
#if defined( _WIN32)
#if !defined( _DEBUG)	// def'd in build
  #define NDEBUG		// omit ASSERTs (and asserts) in release version
  #define DEBUG			// leave 1st level extra checks & messages in
  #undef  DEBUG2		// from release version remove devel aids that are more expensive or for explicit use only
  // #define DEBUG2		// TEMPORARILY define while looking for why BUG0089 happens only in release versn
#else			// else debugging version
  #undef NDEBUG			// include ASSERTs
  #define DEBUG
  #define DEBUG2
#endif
#endif

#if 0 && _MSC_VER >= 1500
// VS2008
 #define WINVER 0x0501					// support Windows XP and later ?C9?
 #define _WIN32_WINDOWS 0x0501			// ditto ?C9?
 #define _WIN32_IE 0x0600				// require IE 6 or later
#endif

#pragma warning( disable: 4793)		// do not warn on 'vararg' causes native code generation ?C9?
#pragma conform( forScope, off)		// for loop idx in local scope (not for scope) ?C9?
#define _CRT_SECURE_NO_DEPRECATE		// do not warn on "insecure" CRT functions (strcpy, ) ?C9?
#pragma warning( disable: 4996)			// do not warn on ISO deprecated functions (stricmp, ) ?C9?
#pragma warning( disable: 4244 4305)	// do not warn on double->float conversion
#pragma warning( disable: 4065)			// do not warn if only 'default' in switch


/*------------------------- Enhanced declarations --------------------------*/

// following can't be in dtypes.h (without added conditionals) cuz of 16 or 32 bit size
typedef int INT;			// signed int 16 or 32 bits as compiled, eg for library fcn args or printf %d.
typedef unsigned int UI;	// unsigned 16 or 32 bit int, eg for %x in printf arg list.
typedef long long LLI;		// signed 64 bit integer
typedef unsigned long long ULLI;
//typedef int BOOL;	if needed: // 16 or 32 bit Boolean, matches windows.h.

/*---------------------- Windows definitions -------------------------------*/
#ifdef _WIN32
#include <windows.h>
#else
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned int BOOL;
typedef unsigned short WORD;
typedef char TCHAR;
typedef unsigned char BYTE;
typedef void* PVOID;
typedef PVOID HANDLE;
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

#define LOCAL   static	// for file-local functions: a clearer word than "static"
#define STATIC  static	// for local data; static un-doable for (former) debugging
#define REFDATA static	// For readonly, pak-specific data which could be overlayed
#define CDEC	// use compiler defaults for linkage, 3-16-10
#define FC 		// eliminating all 32-bit fastcalls fixes rampant bad bcc32 4.0 compiles 3-3-94.
				// tried -Od and -rd compile options at same time, found not necessary.
				// gave up on trying to change individual FC's only b4 getting cul.cpp to complete imput compilation 3-94.

// forward-ref types
struct XFILE;					// Extended IO packet set up in dm by xfopen, used as arg to othr xiopak/xiochar calls
struct FILEINFO;				// file info struct, dospak to xiopak
struct XFNINFO;					// for xfScanFileName info return
struct VROUTINFO;
struct VROUTINFO5;
struct STBK;
struct CULT;

#include "CNDEFNS.H"	// configuration definitions
						//   contains preprocessor #xxx only: used in cnrecs.def

// universal #includes
#undef CSE_MFC		// define to build MFC-based application
#undef LOGWIN		// define to display screen messages to window (re WINorDLL)
					//   (not maintained, 9-12)

#if defined( CSE_MFC)

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxtempl.h>		// template container classes
#if 0
0 activate if needed
0 #include <afxext.h>       // MFC extensions
0 #include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
0 #ifndef _AFX_NO_AFXCMN_SUPPORT
0 #include <afxcmn.h>			// MFC support for Windows Common Controls
0 #endif // _AFX_NO_AFXCMN_SUPPORT
#endif
//   #include <stdio.h>		// included in afx
//   #include <stdarg.h>
//   #include <stdlib.h>
//   #include <time.h>
//   #include <limits.h>
//   #include <string.h>

#else

#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#endif	// CSE_MFC

#include <math.h>
#include <float.h>

#if defined( USE_STDLIB)
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#define WVect std::vector
typedef std::map< int, std::string> WMapIntStr;
typedef std::string WStr;
#endif

#include <dtypes.h>		// portable data types.  File auto generated by rcdef
						//   NOTE: < > required re rcdef build

/*---------------------- global macros and defines -------------------------*/

/*------------------------------- math -------------------------------------*/
// error handling
extern UINT doControlFP();
#define FPCHECK __asm { fwait }	// trap pending FP exception (if any)
								//   re searching for FP exceptions

// min and max (converted to template fcns, 10-12)
#undef min
#undef max
template< typename T, typename TX> inline T min( T a, TX b)
{	return a < b ? a : b; }
template< typename T, typename TX> inline T max( T a, TX b)
{	return a > b ? a : b; }
inline double min( double a, double b, double c)   { return min( min(a,b), c); }
inline double max( double a, double b, double c)   { return max( max(a,b), c); }
#if 0
inline double max( double a, double b, double c, double d)   { return max( max(a,b), max(c,d) ); }
inline double max( double a, double b, double c, double d, double e)   { return max( max(a,b,c), max(d,e) ); }
inline double max( double a, double b, double c, double d, double e, double f)   { return max( max(a,b,c), max(d,e,f) ); }
#endif

// floating point compare
const double ABOUT0 = 0.001;
template< typename T> inline int fAboutEqual(T a, T b) { return fabs(a-b) < ABOUT0; }
template< typename T> inline T frDiff( T a, T b, T tolAbs=T(.00001))
{	T d=abs(a-b);
	return d<=max( tolAbs, T( 1e-30)) ? T( 0.) : T(2.)*d/(abs( a)+abs( b));
}
#define ABSCHANGE(is,was) (fabs((is)-(was)))				// absolute change of float
#define RELCHANGE(is,was) (fabs( ((is)-(was)) / (fabs(is)+.1f) ))	// relative change of float as fraction, w /0 protection

// setToXxx: formerly non-template floor( a, b) and ceil( a, b), 10-12
// setToMax: a = max( a, b)
template< typename T, typename TX> inline T	setToMax( T &a, TX b)
{	if (b > a) a = T( b); return a; }
// setToMin: a = min( a, b)
template< typename T, typename TX> inline T setToMin( T &a, TX b)
{	if (b < a) a = T( b); return a; }
// setToMinOrMax: if doMin setToMin( a, b) else setToMax( a, b)
template< typename T, typename TX> inline T setToMinOrMax( T &a, TX b, int doMin)
{	if ((b > a)^doMin) a = T( b); return a; }

// return -1, 0, or 1 per value of v
template< typename T> T xsign( T v) { return v==0 ? 0 : v>0 ? 1 : -1; }
// return -1 if <, 0 if == , or 1 if >
template< typename T> int LTEQGT( T v1, T v2) { return v1>v2 ? 1 : v1<v2 ? -1 : 0; }

// bracket: limit value betw vMin and vMax
template< typename T> inline T bracket( T vMin, T v, T vMax)
{	return v < vMin ? vMin : v > vMax ? vMax : v; }
// ifBracket: limit value, return 1 iff changed
template< typename T> inline int ifBracket( T vMin, T& v, T vMax)
{	if (v < vMin) {	v = vMin; return 1; }
	if (v > vMax) {	v = vMax; return 1; }
	return 0;
}
// debugging aid: warn if limits invoked
template< typename T> inline T bracketWarn( T vMin, T v, T vMax)
{	T vx = bracket( vMin, v, vMax);
	if (vx != v)
		warn( "bracketWarn: hit limit\n");
	return vx;
}
template< typename T> inline void tswap( T v1, T v2)
{	T vx = v1; v1 = v2; v2 = vx; }

// snap to -1, 0, or 1
template< typename T> inline T snapTrig( T& v, double tol=1e-10)
{	return snap( v, T( 0), tol) || snap( v, T( 1), tol) || snap( v, T( -1), tol); }
// snap to specified value
template< typename T> int snap( T& v, T vSnap, double tol=1e-10)
{	if (fabs(v-vSnap)<tol) { v=vSnap; return 1; } return 0; }
template< typename T> int snapRAD( T& v, double tol=.1)
{	return snap( v, T( 0), tol)
        || v < T( 0) ? snap( v, T( -kPiOver2), tol) || snap( v, T( -kPi), tol)
                     : snap( v, T(  kPiOver2), tol) || snap( v, T(  kPi), tol);
}
template< typename T> int snapDEG( T& v, double tol=.1)
{	return snap( v, T( 0), tol)
        || v < T( 0) ? snap( v, T( -90), tol) || snap( v, T( -180), tol)
		             : snap( v, T(  90), tol) || snap( v, T(  180), tol);
}
// round v to nearest rv (e.g. roundNearest( 14., 5.) -> 15.
template< typename T> inline void roundNearest( T &v, T rv)
{	v = round( v/rv)*rv;  }
// round double to integer (no overflow protection!)
inline int iRound( double a) { return int( a > 0. ? a+.5 : a-.5 ); }
// return v if not nan, else alternative value
template< typename T> inline T ifNotNaN( T v, T vForNaN=0)
{ return isnan( v) ? vForNaN : v; }
//-----------------------------------------------------------------------------

// access to interval data
//   returns ref to array mbr for C_IVLCH_H/D/M/Y
template< typename T> T& IvlData( T* ivlData, int ivl)
{
#if defined( _DEBUG)
	return ivlData[ bracketWarn( 0, ivl-C_IVLCH_Y, IVLDATADIM-1) ];
#else
	return ivlData[ ivl-C_IVLCH_Y];
#endif
}		// IvlData
#if 0	// idea
0 template< typename T> void SetIvlData( T* ivlData, T v, int ivl1, ivl2)
0 {	VSet( ivlData+ivl1-C_IVLCH_Y, ivl2-ivl1+1, v);
0 }
#endif

/*---------------------------- dept of NANs: unset, nandles, nchoices ---------------------------*/

// a NANDLE is a 32-bit quantity that is not a valid IEEE float number nor an expected string pointer value,
// used to indicate unset data and to specify the "expression number" of runtime expression entered for the variable
//    or whose value is to be stored in the variable later.  More discussion in exman.h, exman.cpp.
// A NANDLE contains 0xFF80 in the hi word and 0 for unset or ffff for autosizing or expression number 1-16383 in the low word.
// CAUTION: highly system dependent.  CAUTION: keep distinct from NCHOICEs (below)

// type to hold a NANDLE or a datum (ptr if string)
typedef void * NANDAT;		// CAUTION: for fcn args use ptr (NANDAT *) to be sure C does not alter arg bit pattern.
							// CAUTION: NANDAT * can pt to only 16 bits (TYSI); check type b4 storing.
#define ISUNSET(nandat)  ((ULI)*(void**)&(nandat)==0xff800000L)    		// test variable for unset data
#define ISASING(nandat)  ((ULI)*(void**)&(nandat)==0xff80ffffL)    		// test variable for "to be autosized" 6-95
#define ISNANDLE(nandat) (((ULI)*(void**)&(nandat) & 0xffff0000L)==0xff800000L)	// test for ref to non-constant expr (or unset)
#define ISNANDLEP(pNandat) (((ULI)*(void**)(pNandat) & 0xffff0000L)==0xff800000L)	// test for ptr to ref to non-constant expr (or unset)
#define EXN(nandle)  ((USI)(ULI)*(void**)&(nandle))				// extract expression # from nandle
#define UNSET ((NANDAT)NANDLE(0))					// "unset" value for float/ptr/LI.  cast as desired.
#define ASING ((NANDAT)NANDLE(0xffff))				// may be stored in values to be determined by autosizing 6-95
#define NANDLE(h) ((NANDAT)(0xff800000L + h))		// "expr n" ref for float/ptr/LI (or SI in 4 bytes). h = 1..16383.

//  NCHOICEs are values which can represent one of several choices and which can
//       be stored in a float and distinguished from all numeric values. 2-92.
//  NCHOICEs are used in CHOICN data types, definable thru rcdef.exe per input file cndtypes.def.
//  NCHOICES are stored as IEEE NAN's (not-a-number's) in the form 0x7f80 plus the choice number 1-7f in the hi word.
//  NCHOICES must be kept distinct from UNSET and NANDLES (hi word ff8x) (above/exman.h).
//    CAUTION: to test a float's bit pattern, cast to a pointer then to ULI.
//    Casting directly to ULI causes its numeric value to be converted, altering the bit pattern

#define NCNAN 0x7f80			// bits that make nchoice a nan; is combined with choice index 1-7f to form stored value

// macro to access a float that may contain a NAN: don't let compiler treat as floats til numeric content verified.
#define VD *(void **)&			// usage:  VD x = VD y;  if (VD x == VD y) ..  where x and y are floats such as CHOICN's.

// macro to test if n has an NCHOICE value:
#define ISNCHOICE(n)  (((ULI)VD(n) & 0xff800000L)==0x7f800000L)

// macro to test if float is a number (not UNSET, NANDLE, NCHOICE or other NAN)
#define ISNUM(n)  (((ULI)VD(n) & 0x7f800000L) != 0x7f800000L)

// macro to fetch/store into variable n's hi word. Use w 16-bit flag/choice # dtypes.h C_DTYPE_XXXX constants gen'd by rcdef.
//   usage:  float y;  CHN(y) = C_ABCNC_X;   if (CHN(y)==C_ABCNC_X) ...
// #define CHN(n) ((USI)((ULI)(*(void **)&(n)) >> 16))				more portable fetch-only alternative
#define CHN(n) (*((USI *)&(n)+1))			// access hi word, lvalue use ok. 80x86 DEPENDENT. PCMS<--grep target.

// macro to generate 32-bit value from 16-bit choicb.h constants, for use where full value needed, as in initialized data
//   usage:  float y = NCHOICE(C_ABCNC_X);
#define NCHOICE(nck)  ((void *)((ULI)(nck) << 16))		// put in hi word.  nck already includes 0x7f80.

/*----------------------------- constants ----------------------------------*/
#define TRUE 1
#define FALSE 0

/*---------------------- return codes / error codes ------------------------*/

// RC Return Codes: integers returned by fcns to indicate call outcome
// 0 indicates all OK so RC can be cumulatively or'd
typedef SI RC;
const RC RCOK=0;		// fcn succeeded.  code assumes 0 value.
const RC RCFATAL = -1;	// fcn encountered fatal error; current process should be abandoned.
						//   note (RCFATAL | x) == RCFATAL, code may assume
const RC RCBAD = -2;	// fcn failed -- general
const RC RCBAD2 = -4;	// alternate failure return, for use where needed; note RCBAD2|RCBAD == RCBAD.
// values 1 - 16384	reserved, overlap with MH_xxxx definitions (see msghans.h and ...)

// values > 16384	reserved for local definition and use, e.g. RCUNSET in cueval.h
const RC RCUNSET = 16385;	// returned by e.g. cuEvalxx if unset data accessed
							//  (eg by probe of a RAT member)
							// -- caller may want to treat this error differently.
const RC RCCANNOT = 16386;	// return code for "cannot do it, try another way" (see tryImInProbe())
const RC RCNOP = 16390;		// fcn did nothing (due to e.g. specific input values)
const RC RCEOF = 16391;		// indicates end of input when returned by ppRead, ppM, ppC, etc
#define CSE_E(f)  { rc = (f); if (rc) return rc; }	// if fcn f returns error, return it to caller.  Note RCOK is 0.
#define CSE_EF(f) { if (f) return RCBAD; }			// "fast" variant: return RCBAD for any non-RCOK

// SEC additional system error codes -- in addition to c library errno values.
//   must not conflict with errno values.  See rmkerr:secMsg and xiopak.cpp.
typedef SI SEC; 		// system error code data type (matches type of msc c library "errno")
const SEC SECOK = -1;	// system error code for no error
const SEC SECEOF = -2;	// system error code for EOF (none def by MSC).
const SEC SECOTHER = -3;	// system error code indicating that an error
							//   was signalled but errno was not set
const SEC SECBADFN = -4;	// bad file name, detected in our code (not library).
							//	 msc at least has no such msg.  added by rob 1-88. */
const SEC SECBADRV = -5;	// bad drv letter (xiopak:chdir, likely other uses)


/*------------- Error Actions and Options ("erOp" arguments) --------------*/

/*--- error reporting and handling options, for rmkerr and its many callers. */
 		// error action: controls error:err response to error
const int IGN    = 0x0001;	// ignore: no message, silently continue execution
const int INF    = 0x0004;	// info
const int ERR    = 0x0000;	// error: msg to screen, error file, and err report if open, continue execution.
							//   0, often omitted.
const int REG = ERR;
const int WRN    = 0x0002;	// issue msg as ERR, await keypress, then continue unless A pressed
const int ABT    = 0x0003;	// issue msg, get keypress, abort program.
const int KEYP   = 0x0002;	// keypress bit: on in WRN/ABT, off in IGN/INF/ERR
const int ERAMASK= 0x0007;	// mask to clear all option/app bits for testing for IGN/WRN/ABT. general use.

const int PROGERR= 0x0008;	// program error bit: on in PWRN/PABT (removing makes WRN/ABT), off in others.
const int PERR   = ERR|PROGERR;	// program error (different msg wording) ERR
const int PWRN   = WRN|PROGERR;	// program error WRN
const int PABT   = ABT|PROGERR;	// program error ABT
const int NOPREF = 0x0010;	// do not add "Error: "/"Program Error: " prefix to message text. 2-94.
const int SYSMSG = 0x0020;	// Do GetLastError(), add system error msg
const int SHOFNLN= 0x0040;	// show input file name and line # (orMsg)
const int RTMSG  = 0x0080;		// runtime format (include day/hour info in msg) (orMsg)
const int IGNX   = 0x0100;		// "extra ignore" -- no msg, do not return RCBAD (orMsg)
const int ERRRT = ERR | RTMSG;
const int WRNRT = WRN | RTMSG;

// options for rmkErr:screen() and :logit() remark display
const int NONL   = 0x1000;	// do NOT force remark to the beginning of its own line (default: start each message on new line)
const int DASHES = 0x2000;	// separate this remark from preceding & following message with ------------
const int NOSCRN = 0x4000;	// logit: do not also display on screen
const int QUIETIF= 0x8000;	// screen: do not display if zn screenQuiet (rmkErr.cpp)

// options for errI() isWarn
const int NOERRFILE = 0x1000;	// do NOT write message to error file (override default behavior)
const int NOREPORT = 0x2000;	// do NOT write message to report file (override default behavior)
// const int NOSCRN = 0x4000;	// do NOT write message to screen (override default behavior)


// application option bits in same argument word as error action.
// Options for specific functions are defined in other .h files in terms of
// the following                ---------- users include ------
const int EROP1 =  0x010000;	//  vrpak.h  dmpak.h  xiopak.h    [ratpak.h]
const int EROP2 =  0x020000;	//  vrpak.h  dmpak.h  [xiopak.h]  sytb.cpp
const int EROP3 =  0x040000;	//  vrpak.h  wfpak.h  [xiopak.h]
const int EROP4 =  0x080000;	//  vrpak.h  wfpak.h
const int EROP5 =  0x100000;	//  vrpak.h  wfpak.h
const int EROP6 =  0x200000;	//  vrpak.h  yacam.h
const int EROP7 =  0x400000;	//  pgpak.h  [ratpak]
const int EROMASK= 0xff0000;	// mask for application option bits


// ------------------------- Debug aid ASSERT macro -------------------------
//
// Issues message and terminates program if argument is false.
// Like compiler library "assert" macro (assert.h), but exits our way so DDE is properly terminated, etc
// (otherwise windows program dissappears from screen but does not actually exit promptly,
// necessitating Windows restart to reRun program.) 8-95.

#if !defined( CSE_MFC)
#ifdef NDEBUG				// (un)def above
   #define ASSERT(p)   ((void)0)	// if ommitting assertion checks
#else
   #define ASSERT(p)   ((p) ? (void)0 : ourAssertFail( #p, __FILE__, __LINE__))	// function in rmkerr.cpp
#endif
#endif
// compile-time assert (ugly but finds errors)
#define STATIC_ASSERT( condition, name ) \
	typedef char assert_failed_ ## name[ (condition) ? 1 : -1 ];

// check floating point value
//   calls function that does _isnan, _finite, etc
#if defined( _DEBUG)
extern int CheckFP( double v, const char* tag);
#define CHECKFP( v) CheckFP( v, #v);
#else
// compile to nothing in release
#define CHECKFP( v)
#endif

#if 0 && !defined( _DEBUG)
// Intrinsics
//   DEBUG build slightly slower (2%?) with no intrinsics and possibly easier to debug
// intrinsics
// Tested debug (non optimized) compiles, VC 4.1; byte counts as follows:
// fcn      non-intrinsic    intrinsic
// fabs        16               5
// abs          9               5
// labs         9               5
// pow         24              11
// fmod, acos, asin, cosh, sinh, tanh
//		(did not test, but expect pow-like advantage)
// memset      15              19 (does not win)
// strlen      12              17 (does not win)
#pragma intrinsic( fabs, abs, labs, pow, fmod, acos, asin, cosh, sinh, tanh)
// release build does not use intrinsics w/o #pragma
// changes debug build to alternative call
#pragma intrinsic( sin, cos, tan, atan, atan2, exp, log, log10, sqrt)
/* intrinsics which don't win in size (7-3-89 MSC 5.0):
      memcpy (24 -> 43), memcmp (24 -> 55),
      strlen (18 -> 24), strcpy (20 -> 55), strcat (20 -> 64),
      min, max (error: "not available as intrinsic") */
#endif	// _DEBUG


// COMMONLY USED TYPES and defines

#define GLOB	// ifndef GLOB may be used to prevent duplicate compilation

extern TOPRAT Top;	// top-level record universally accessible
class record;
typedef USI RCT;	// record type type.  Needed for RATBASE defn below, for ratpak.h, and rccn.h.
struct SFIR;		// small fields-in-record info table (for basAnc.fir, SRD.fir). struct in srd.h.
typedef SI MH;		// message handle, used in calls to err() and msg(), identifier for msgs in msgtab:msgTbl[]
typedef void* DMP;	// Dynamic memory block pointer: ptr to any type, record struct, etc, of caller's
#define DMPP( p) ((DMP *)DMP( &(p)))
inline void IncP( void **pp, int b) { *pp = (void *)((char*)(*pp) + b); }
typedef USI PSOP;	// type for pseudo-code opcodes -- used in sevaral .cpp files and in cuparse.h
struct RXPORTINFO;
namespace Pumbra { class Penumbra; }
namespace Kiva { class Instance; class Aggregator; class Foundation; }
namespace Btwxt { class RegularGridInterpolator; }



#ifdef WINorDLL
  // control of scattered code for returning file names used via a cse() argument.
  #define OUTPNAMS	// expect to test this in cse.cpp, rmkerr.cpp, vrpak.cpp, etc. 10-6-93
#endif

// cse.h.     keep after OUTPNAMS define!
#ifdef OUTPNAMS
   void FC saveAnOutPNam( const char* pNam);	// accepts output file name to return to caller
#endif

// for init/cleanup procedure args 10-93
enum CLEANCASE		// caution code assumes STARTUP < ENTRY < others.
{   /*STARTUP=1,*/	// possible future: initializing, at DLL entry
    ENTRY=2,		// initializing at user call to subr package. May follow a previous "fatal" error exit.
    DONE,			// cleanup from normal completion path, after success or error
    CRASH			// cleanup after longjmp out after "fatal" error (that terminated program prior to 10-93)
 #ifdef DLL
    /*, CLOSEDOWN */		// possible future: terminating library, from WEP procedure
 #endif
};

// stored in place of record subscript for special words entered, where permitted.  Code assumes negative.
//   cul.cpp, cucnultx.cpp, cgresult.cpp, etc 1-92
#define TI_SUM    -3	// "sum" entered: use sum of all (zones, etc), not a specific one.
#define TI_ALL    -4	// "all" entered: do report, warmest zone control, etc for all records (zones, meters, etc).
#define TI_ALLBUT  -5	// "all_but" entered (at start of list of TI's).

/*------------------------- COMMONLY USED FUNCTIONS ------------------------*/

// here to reduce need to include files.

// cncult.cpp (or stub in e.g. rcdef.cpp)
int getCpl( TOPRAT** pTp=NULL);	// get chars/line
									// default if Top not yet init & input value unavailable

// re DLL interrupt (in cse.cpp, stub in rcdef.cpp)
int CheckAbort();

// messages.cpp
const char * CDEC msg( char *mBuf, const char *mOrH, ...);

// commonly used pow variants
inline float  pow2(  float v) { return v*v; }
inline double pow2( double v) { return v*v; }
inline float  pow3(  float v) { return v*v*v; }
inline double pow3( double v) { return v*v*v; }
inline float  pow4(  float v) { return v*v*v*v; }
inline double pow4( double v) { return v*v*v*v; }
inline float pow033( float v)
{
#if defined( FASTPOW)
	?
#else
	return powf( v, .333f);
#endif
}

// useful constants
const double kPi=3.14159265358979323846;
const double k2Pi=2.*kPi;
const double k4Pi=4.*kPi;
const double kPiOver2=kPi/2.;
const double g0Std     = 32.2;			// standard acceleration of gravity (g0 or "little g"), ft/sec2
const double sigmaSB   = 0.1714e-8;		// stefan-boltzman constant, Btuh/ft2-R4
const double sigmaSB4  = sigmaSB*4.;	// 4 * ditto
const double sigmaSBSI = 5.669e-8;		// stefan-boltzman constant, W/m2-K4
const double tAbs0F						// 0 F in Rankine
#if defined( CZM_COMPARE)
	= 460.;
#else
	= 459.67;
#endif
inline float  DegFtoR( float f)  { return float( f+tAbs0F); }
inline double DegFtoR( double f)  { return f+tAbs0F; }
inline float  DegRtoF( float r)  { return float( r-tAbs0F); }
inline double DegRtoF( double r)  { return r-tAbs0F; }
inline float  DegFtoC( float f)  { return float( (f-32.)/1.8); }
inline double DegFtoC( double f)  { return (f-32.)/1.8; }
inline float  DegCtoF( float c)  { return float( c*1.8+32.); }
inline double DegCtoF( double c)  { return c*1.8+32.; }
inline float  DegCtoK(float c)  { return float(c + 273.15); }
inline double DegCtoK(double c)  { return c + 273.15; }
inline float  DegFtoK(float f)  { return float((f + tAbs0F) / 1.8); }
inline double DegFtoK( double f)  { return (f+tAbs0F)/1.8; }
inline float  DegKtoF( float k)  { return float( k*1.8-tAbs0F); }
inline double DegKtoF( double k)  { return k*1.8-tAbs0F; }


inline double QRad( double t1, double t2)		// radiant power betw 2 black surfaces
{	return sigmaSB*(pow4( DegFtoR( t1))-pow4(DegFtoR( t2))); }

// Radians <--> degrees
template <typename T> inline T DEG( T r) {	return T( r*180./kPi); }
template <typename T> inline T RAD( T d) {	return T( d*kPi/180.); }

// Btuh/ft2-F <--> W/m2-K
const double cfU = 5.678263;		// W/m2-K per Btuh/ft2-F
inline float UIPtoSI( float u)   { return float( u*cfU); }
inline double UIPtoSI( double u) { return u*cfU; }
inline float USItoIP( float u)	 { return float( u/cfU); }
inline double USItoIP( double u) { return u/cfU; }

// Irradiance Btuh/ft2 <--> W/m2
const double cfIr = 3.154591;
inline float IrIPtoSI( float ir)   { return float( ir*cfIr); }
inline double IrIPtoSI( double ir) { return ir*cfIr; }
inline float IrSItoIP( float ir)	 { return float( ir/cfIr); }
inline double IrSItoIP( double ir) { return ir/cfIr; }

// length ft <--> m
const double cfL = 1./3.28084;	// m/ft
inline float LIPtoSI( float l)   { return float( l*cfL); }
inline double LIPtoSI( double l) { return l*cfL; }
inline float LSItoIP( float l)	 { return float( l/cfL); }
inline double LSItoIP( double l) { return l/cfL; }

// area ft^2 <--> m^2
const double cfA = cfL*cfL; // m2/ft2
inline float AIPtoSI(float a) { return float(a * cfA); }
inline double AIPtoSI(double a) { return a * cfA; }
inline float ASItoIP(float a) { return float(a / cfA); }
inline double ASItoIP(double a) { return a / cfA; }

// Conductivity Btuh-ft/ft2-F <--> W/m-K
const double cfK = cfU * cfL; // W/m-K per Btuh-ft/ft2-F
inline float KIPtoSI(float k) { return float(k * cfK); }
inline double KIPtoSI(double k) { return k * cfK; }
inline float KSItoIP(float k) { return float(k / cfK); }
inline double KSItoIP(double k) { return k / cfK; }

// Specific Heat Btu/lb-F <--> J/kg-K
const double cfSH = 4186.8; // J/kg-K per Btu/lb-F (exact)
inline float SHIPtoSI(float sh) { return float(sh * cfSH); }
inline double SHIPtoSI(double sh) { return sh * cfSH; }
inline float SHSItoIP(float sh) { return float(sh / cfSH); }
inline double SHSItoIP(double sh) { return sh / cfSH; }

// Density lb/ft3 <--> kg/m3
const double cfD = 16.0185; // kg/m3 per lb/ft3
inline float DIPtoSI(float d) { return float(d * cfD); }
inline double DIPtoSI(double d) { return d * cfD; }
inline float DSItoIP(float d) { return float(d / cfD); }
inline double DSItoIP(double d) { return d / cfD; }

// mass flow rate lb/hr <--> kg/s
const double cfMFR = 0.000126; //
inline float MFRIPtoSI(float mfr) { return float(mfr * cfMFR); }
inline float MFRSItoIP(float mfr) { return mfr * cfMFR; }
inline double MFRIPtoSI(double mfr) { return float(mfr / cfMFR); }
inline double MFRSItoIP(double mfr) { return mfr / cfMFR; }


// velocity mph <-> m/s
const double cfV = 1./2.23694;		// m/s / mph
template< typename T> inline T VIPtoSI(T mph) { return T( mph*cfV); }
template< typename T> inline T VSItoIP(T ms) { return T( ms/cfV); }

// air mass flow <--> air volume flow
//   amf in lbm/hr
//   cfm in ft3/min std air
inline double AMFtoAVF( double amf) { return amf / (60.*.075); }
inline float  AMFtoAVF( float amf)  { return amf / (60.f*.075f); }
inline double AVFtoAMF( double avf) { return avf * 60.*.075; }
inline float  AVFtoAMF( float avf)  { return avf * 60.f*.075f; }
// amf in lbm/s
inline float AMFtoAVF2( float amf) { return amf * 60.f / .075f;  }


// Powers of 10
const int PTENSIZE = 21;			// # of pwrs of 10 in Pten array
const double Pten[PTENSIZE] = {1.,10.,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,1e20};
const int NPTENSIZE = 21;			// # of negative pwrs of 10 in NPten array
const double NPten[NPTENSIZE] = {1.,.1,.01,1e-3,1e-4,1e-5,1e-6,1e-7,1e-8,1e-9,1e-10,1e-11,1e-12,1e-13,1e-14,1e-15,1e-16,1e-17,1e-18,1e-19,1e-20 };

const float FHuge = float(1.E38);	// Handy large (not strictly largest) value used for x/0,
									// search initializations, etc.  Single defn for portability and code clarity.

// Physical constants
#if 1
const float BtuperWh    =  3.41214f;	// Btu per Wh
const float BtuperkWh   =  3412.14f;	// Btu per kWh
const float waterRhoCp = 8.29018282f;	// water volumetric weight, lb/gal
										//    = volumetric heat capacity, Btu/gal-F
										// 30 C (85 F) value used, appropriate for DHW calcs
#else
const float BtuperWh = 3.413f;	// Btu per Wh
const float BtuperkWh = 3413.f;	// Btu per kWh
const float waterRhoCp = 8.345f;
#endif
const float galPerFt3 = 7.48052f;		// gal per cubic foot

#if 0	// obsolete
x const float SrcMultElect = 3.f;	// Source multiplier for electrical energy (California CEC value)
x const float BtuSrcperWh = 10.239f;	/* Btu (source energy) per Wh.  Includes California electric
x					   to source conversion factor of 3 (3 * 3.413 = 10.239);
x					   used to convert electrical consumption to source energy */
#endif

// re solar gain distribution (must be known for rccn.h)
enum { socOPEN, socCLSD, socCOUNT };			// internal shade modes (open, closed)
enum {			// irrad components
	sgcBMXBM,		// beam per unit exterior beam
	sgcDFXDF,		// diffuse per exterior diffuse
	sgcDFXBM,		// diffuse per exterior beam
	sgcCOUNT
};

// surface classes e.g. for argument to topSf1(), topSf2()
//  NOTE: code depends on value order, change with care
enum SFCLASS { sfcNUL, sfcSURF, sfcDOOR, sfcWINDOW, sfcDUCT, sfcPIPE};

// zone air flow indices
#if 0	// unused, 2-20-2013
x //   note: zafCOUNT must equal #define ZAFCOUNT in cndefns.h (re use in cnrecs.def)
x enum ZAFTY { zafANINF, zafANVENT, zafINFIL, zafDUCTLKI, zafSYSAIRI,
x	zafDUCTLKO, zafSYSAIRO, zafDUCTLKILS, zafCOUNT };
x const int zanDUCTLKI   = 0x00000001;
x const int zanDUCTLKO   = 0x00000002;
x const int zanDUCTLKUB  = 0x00000004;
x const int zanSYSAIR    = 0x00000008;
x const int zanSYSAIRUB  = 0x00000010;
#endif


// End Uses
//  NENDUSES = number of end use members in MTR_IVL_SUB, for mtrsAccum and mtrAccum1.
//	Defined in terms of last end use choice: last end use member in MTR_IVL_SUB.
const int NENDUSES = C_ENDUSECH_PV;	// must be same as C_ENDUSECH_PV
									// (and # choices in enum endUses if used)
const int NDHWENDUSES = C_DHWEUCH_COUNT;	// # of DHW end uses
static_assert(NDHWENDUSES == NDHWENDUSESPP, "Inconsistent DHW EU constants");
static_assert(NDHWENDUSESXPP == C_DHWEUXCH_COUNT, "Inconsistent DHW UEX constants");

// more nested includes
#include "DMPAK.H"
#include "STRPAK.H"
#include "RMKERR.H"
#include "VECPAK.H"
#include "PSYCHRO.H"

#if 1 || defined( _DEBUG)
#define DEBUGDUMP	// define to include DbPrintf() etc code
#endif
BOOL DbDo( DWORD oMsk, int options=0);

// debugging option bits
//   re control of conditional DbPrintf()s
// const DWORD dbdALWAYS = 1;
const DWORD dbdCONSTANTS = 8;	// various constants
const DWORD dbdAIRNET = 16;
const DWORD dbdMASS = 32;
const DWORD dbdZM = 64;			// main zone model debug print control
const DWORD dbdDUCT = 128;		// duct model
const DWORD dbdHPWH = 256;		// heat pump water heater (writes to external CSV)
const DWORD dbdRADX = 512;		// radiant exchange and SGDIST
const DWORD dbdRCM = 1024;		// radiant/convective model details
const DWORD dbdIZ  = 2048;		// selected interzone info
const DWORD dbdSGDIST = 4096;	// solar gain distrib
const DWORD dbdASHWAT = 8192;	// ASHWAT call/return
// const DWORD dbdCOMFORT = 16384;	// Comfort model values (minimal as of 1-11)
const DWORD dbdSBC = 16384;		// surface conditions
// NOTE: max value = 32768 due to USI limit for expression values
//       workaround: create additional user-settable values
//       shift / combine these values in tp_SetDbMask()
//		 1-2012


// end of cnglob.h
