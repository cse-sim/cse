// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cndefns.h -- shared #defines for CSE code and rcdef/cnrecs.def

// #included in cnglob.h to control configuration of CSE build
// #included in cnrecs.def, cndtypes.def, etc. to control

// NOTE!! Use care when #defines contain expressions.
//   Expressions cannot be evaluated by RCDEF.

#ifndef _CNDEFNS_H		// skip duplicate includes
#define _CNDEFNS_H		// say included
// convert #defined value to string literal
//   #define NAME BOB
//   MAKE_LITERAL( NAME) -> "BOB"
#define MAKE_LITERAL2(s) #s
#define MAKE_LITERAL(s) MAKE_LITERAL2(s)

// Target operating system (https://sourceforge.net/p/predef/wiki/OperatingSystems/)
#define CSE_OS_WINDOWS 1
#define CSE_OS_MACOS 2
#define CSE_OS_LINUX 4

#if defined(_WIN32) // _WIN32 Defined for both windows 32-bit and windows 64-bit environments 1
#define CSE_OS CSE_OS_WINDOWS
#elif defined( __APPLE__)
#define CSE_OS CSE_OS_MACOS
#elif defined( __linux)	// May need to be more specific later (e.g., __GNU__)
#define CSE_OS CSE_OS_LINUX
#else
#error "Unknown CSE_OS"
#endif

// Current compiler (https://sourceforge.net/p/predef/wiki/Compilers/)
#define CSE_COMPILER_MSVC 1
#define CSE_COMPILER_GCC  2
#define CSE_COMPILER_CLANG 4
#define CSE_COMPILER_APPLECLANG 8

#if defined( _MSC_VER)
#define CSE_COMPILER CSE_COMPILER_MSVC
#elif defined(__clang__)
#if defined(__apple_build_version__)
#define CSE_COMPILER CSE_COMPILER_APPLECLANG // (unsure if we'll need this distinction)
#else
#define CSE_COMPILER CSE_COMPILER_CLANG
#endif
#elif defined( __GNUC__)
#define CSE_COMPILER CSE_COMPILER_GCC
#else
#error "Unknown CSE_COMPILER"
#endif

// Target architecture
//    CSE_ARCH=32 or 64 from command line
#if !defined( CSE_ARCH)
#if CSE_COMPILER == CSE_COMPILER_MSVC
#define CSE_ARCH 32
#else
#define CSE_ARCH 64
#endif
#endif
#if CSE_ARCH != 32 && CSE_ARCH != 64
#error "Invalid CSE_ARCH -- must be 32 or 64"
#endif

#if 0
// testing aid: echo config values
#pragma message("CSE_ARCH = " MAKE_LITERAL(CSE_ARCH))
#pragma message("CSE_OS = " MAKE_LITERAL(CSE_OS))
#pragma message("CSE_COMPILER = " MAKE_LITERAL(CSE_COMPILER))
#if defined( NDEBUG)
#pragma message("NDEBUG defined")
#else
#pragma message("NDEBUG not defined")
#endif
#endif

/*----------------------------- compiling for -----------------------------*/
// #defines assumed on compiler command line
// WIN		build Windows appl, screen output to window (not maintained 9-12)
// DLL		build Windows DLL, screen output to window (not maintained 9-12)
// CSE_DLL	build "silent" CSE DLL, screen output returned via callback
// else CSE_CONSOLE  build console app, screen output to cmd window

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

//--- Debugging conditionals
// NDEBUG	controlling define from command line, indicates release build
//          define to REMOVE ASSERT macros (below) (and assert macros, assert.h)
// _DEBUG	define to include debugging/checking code
//          MSVC debug flag, used widely in CSE source
//          Set per NDEBUG
// DEBUG 	define to include extra checks & messages -- desirable to leave in during (early only?) user testing
// DEBUG2	define to include devel aids that are expensive or will be mainly deliberately used by tester.

#if defined( NDEBUG)	// if release, def'd in build
  // #define NDEBUG		// omit ASSERTs (and asserts) in release version
#undef _DEBUG			// omit debug code and checks
#define DEBUG			// leave 1st level extra checks & messages in
#undef  DEBUG2		// from release version remove devel aids that are more expensive or for explicit use only
// #define DEBUG2		// TEMPORARILY define while looking for why BUG0089 happens only in release versn
#else			// else debugging version
  // #undef NDEBUG			// include ASSERTs
  #if !defined( _DEBUG)
    #define _DEBUG			// avoid duplicate definition on MSVC
  #endif
  #define DEBUG
  #define DEBUG2
#endif


//------------------------------------------------ OPTIONS --------------------------------------------------
// #define SHINTERP	// define for subhour-interpolated weather data. Used in many files. 1-95
					// coded out #defined 4-1-10

#undef SOLAVNEND
			/* undef to use subhour-average interpolated solar or hour-average solar values at all uses.
			   Define to make use of subhour-end interpolated values for zones, average values for masses.
			   Code for defined incomplete: surface sgdist to zone total must be reworked to use
			   avg value for solar SGDIST'd to mass, end value for remainder going to zone, determined
			   m-h (eg in cgsolar.cpp) cuz SGDIST sgd_FSC/O are runtime-variable.
			   (Present code fails (ebal errs) when SGDISTS present, eg CKENV\S1HWN.)
			   Could code out undefined after works and Bruce/Phil confirm (memo NILES108.TXT).
			   Used in cnrecs.def, cuparse.cpp, cgwthr,cgsolar,cnloads.cpp. 1-18-95. */

//--- option(s) re supporting NREL Design Tool, 1993-94

/*--- history of removal of former seasons stuff which used different (possibly smaller) sgdist f's in "summer".
 * #undef SEASONS  91? 92?: define for vestigial do-nothing seasons and csType, smrBeg, smrEnd, and smrTemp commands.
 *		            files: cnrecs, cnguts, cncult,2,3 cuparse, cgsolar, cgwthr at least.
 *		   7-92:    Leaving SEASONS DEFINED, redid SGDISTS and cgsolar for shades open-closed not seasons,
 *			    making seasons do nothing but $isSummer.
 *		   1-93:    Undefined SEASONS; put temporary warnings in cncult.cpp re seasons commands.
 *		   2..3-94: All SEASONS code deleted. */

//--- following provide grep-able backtrack to deleted obsolete data and code 1-92 (some previously disabled)
#undef OLDNV	// cr/hst nat vent KEEP THE CODE -- Chip says record needed
#undef OLDFV	// cr/hst fan vent code
#undef OLDCF	// cr/hst ceiling fan code

// enhanced CSE zone models, 7-10
#undef CZM_COMPARE			// define to use methods identical to those in
							//   CZM.BAS (re result comparison) 10-10
#undef CONV_ASHRAECOMPARE

#define AIRNET_EIGEN		// define to use Eigen for AirNet solving
							//   else gaussjb

#undef WTHR_T24DLL			// #define to support T24WTHR.DLL source for hourly compliance
							//   weather data

// implement zone exhaust fan, 8-10
#define ZONE_XFAN	// define to enable zone exhaust fan implementation, 8-10
					//   FAN object historically in ZNISUB.xfan but previously not simulated

#undef RSYS_FIXEDOAT	// define to cause fixed RSYS supply air temp / humrat
						//   (development aid)

#define USE_STDLIB		// re conversion to non-MFC containers

#undef AUSZ_REPORTS		// define to enable hourly reports during autosizing
						//   Note: crude at best, use for testing only, 12-31-2012

//------------------------------------------------ DEFINES --------------------------------------------------
#define MAXSUBSTEPS 120		// max # of substeps per hour
							// limit may be less if specific items present
							//    if DHWSYS is present, limit is 60
							// increase with care, trouble has been seen for higher values
							//    e.g. 240 causes wfpak WDSUBHRFACTORS::wdsf_Setup() assert
							//    5-2021

#define HSMXSGDIST 8	// Max # of solar gain targets from one XSURF.
						// Need 1 per solar gain distribution. 4->5 1990, ->8 2-95.
#define XSMXTLRB 10		// Max # of layer boundary temps maintained in XSURF
						//   see XSURF.xs_tLrB[]


// terminal and mode definitions; used in cnrecs.def TU, ZNR,
//#define MAX_TUKS     3	// max capabilities per terminal: local heat, air heat, air cool.  No uses; define if needed.
#define MAX_ZONETUS    3	// max terminal units allowed to serve a zone: enforced during input
#define MAX_ZONEMODES 21	// max mdSeq[] modes pos with max TUs each with max capabilities:
							//   need 2 per TU capability (1 at set point, 1 floating above)
							//      + 2 per additional mode (e.g.nat vent) + 1 (low end floating)
							//   = 2 * MAX_TUKS * MAX_ZONETUS + 3

#define IVLDATADIM 4		// dimension of interval data arrays
							//    4 = Y, M, D, H
							// see IvlData() template function, cnglobl.h

#define RSYSMODES 3			// # of RSYS operating modes (not including rsmOFF)
							//   must be known in cnrecs.def
							//   must be consistent the rsmXXX enum

#define NDHWENDUSESPP 6		// # of DHW end uses (for use in cnrecs.def)
							//   must be same as C_DHWEUCH_COUNT
							//   see also cnglob.h const int NDHWENDUSES
#define NDHWENDUSESXPP 8	// # of extended DHW end uses
							//   must be same as C_DHWEUXCH_COUNT
#define NDHWSIZEDAYS 10		// # of DHW sizing design days for EcoSizer method

#define DETAILED_TIMING		// define to enable fine-grain timing

#undef COMFORT_MODEL		// define to include comfort model
							//   calculation of PPD and PMV

							// initial geometry implementation, 2-2017
							//   fixed size polygons as input convenience
							//   using ARRAY mechanism
							//   Note limit is due to ARRAY input only.
#define MAX_POLYGONVERTICES 12	// maximum number of input polygon vertices
#define DIM_POLYGONXYZ 37		// input arrays dimension
								//   = MAX_POLYLPVERTICIES*3 + 1

#define CRTERMAH			// define to enable support of TERMINAL / AIRHANDLER model
							//    in convective / radiant zones
#undef SUPPRESS_ENBAL_CHECKS	// define to suppress energy balance checks
								//   (development aid)

#define DIM_SUBMETERLIST 51		// dimension of submeter lists in MTR, LOADMTR,
								//   max # submeters inputable = DIM_SUBMETERLIST-1

#define DIM_DHWTANKTINIT 13		// dimension of DHWHEATER wh_tankTInit array
								//   (initial tank layer temperatures)
								// 12 input values required
#define DIM_HCFRCCOEFFS 4		// dimension of sb_hcFrcCoeffs[]
								//   3 input values + 1 terminator
#undef METER_DBL			// define to use double for MTR_IVL end-use values
							// (else float).  6-23 experiment, infinitesimal impact.
							// code out when confirmed useless.

#endif	// ifndef _CNDEFNS_H

// cndefns.h end
