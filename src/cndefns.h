// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cndefns.h -- shared #defines for CSE code and rcdef/cnrecs.def

// #included in cnrecs.def, cndtypes.def, .
// may be #included in cnglob.h cuz symbols defined here come out in dtypes.h, 1-94

// NOTE!! These #defines cannot contain expressions (rcdef limitation)

#ifndef _CNEDEFNS_H		// skip duplicate includes 1-94
#define _CNEDEFNS_H		// say included

//------------------------------------------------ OPTIONS --------------------------------------------------
#undef BINRES	// define for code to output binary results files, 11-93.
				// Implemented for NREL, but generally useful (if documented for others!).

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
#undef AIRNET_COMPARE		// #define to use methods identical to those in
							//   AirNet.bas (re result comparison)
#undef CONV_ASHRAECOMPARE

#undef WTHR_T24DLL			// #define to support T24WTHR.DLL source for hourly compliance
							//   weather data

// implement zone exhaust fan, 8-10
#define ZONE_XFAN	// define to enable zone exhaust fan implementation, 8-10
					//   FAN object historically in ZNISUB.xfan but previously not simulated


#undef BUG_COLTYPECOPY	// define to include object trap code re memcpy bug
						//   associated with copying COL objects
						//   File coltypebug.cse crashes.
						//   Fixed (maybe) by overriding COL::CopyFrom().
						//   Further research needed: why both Copy() and CopyFrom()?
						//      (both use memcpy(), dangerous when object contains heap ptrs)
						//   2-24-2012

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

#define ASHP_COPREG 6		// select COP17/47 regression model
							//   1 = original (2012?)
							//   2 = revised 6-3-2013
							//   3 = COP/HSPF fit 6-4-2013
							//   4 = kwPerTon fit 6-4-2013
							//   5 = force COP17=1.8 at HSPF=6.8 6-4-2013
							//   6 = derive COP17 and COP35 so HSPF matches input

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

#endif	// ifndef _CNEDEFNS_H

// cndefns.h end
