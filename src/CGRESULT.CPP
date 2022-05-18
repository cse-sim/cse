// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cgresult.cpp -- routines for managing and printing simulator results for CSE.
                 Called from cnguts.cpp and cgresfil.cpp */

// 6-29-95: prior (5-17-95) on .cp1 b4 trial AHLOAD report changes

/* QUESTIONS 11-91
     1. More than one export in a file: some control over separation? put zone in the row? suppress repeat header?
       DONE 1-92, DOCUMENT:
     1. Reports: 1st col head is blank. 3 cols avail.  Say Mon, Day, Hr, Shr, H/S?.
     2. last subhour Md now shows in hourly report but chip (or rob?) left a "should it?" comment. */


// much reworked 11-91 for virtual report system, report/export files, flexible report requests, user defined reports.

// derived from hs/hsresult.cpp (CR, HST variant) 8-16-91

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// ZNRstr ZNRESstr ZNRES_IVL_SUB
#include <MSGHANS.H>	// MH_R0150

#include <VRPAK.H>	// vrStr vrC vrPrintf
#include <XIOPAK.H>	// xfwrite, xfcopy, xfopen ...
#include <CVPAK.H>	// cvin2s FMTLJ
#include <STRPAK.H>	// strtmp, strtprintf
#include <PSYCHRO.H>	// psyTWetBulb PSYCHROMINT

#include <EXMAN.H>	// rer
#include <CNGUTS.H>	// decls for this file

/*-------------------------------- OPTIONS --------------------------------*/

// line length limits = buffer sizes
//   >> no runtime checks !! <<
static const size_t MAXRPTLINELENSTD = 1000;	// built-in reports
static const size_t MAXRPTLINELENUDT = 30000;	// user-defined reports
												//   no limit enforced, use "impossibly" big value
												//   largest length seen in wild = 7000ish

#define QBAL	// show debugging/check info: ZEB report, AH report Qbal, .  Undefine to remove later.  6-9-92.

#define ZFILTER 0	// 2-->0  7-17-95
/* define to print as exact 0's values smaller than this many powers of 10 smaller than minimum
   value that could possibly show significance in field width.  Rob 6-92.
   Undefine to not filter small values; cvpak then prints things like "-.000" for small values. */

#undef DEFFOOT	/* define to restore deferred-footer code using VR_NEEDFOOT.  Else can delete VR_NEEDFOOT.
			Believe not needed now 1-6-92 with VR_FINEWL in use for final blank lines,
			as all footers covered by unconditional vpRxFooter calls.
			Needed if any reports with footers can have rpCond.
			May be needed if any reports with footers can end before end of year.

			MAY WANT TO RESTORE 1-20-92 as summary footer lines on many daily & monthly
			reports kept -- or just let line be omitted? (wrong-results problem with
			late call...; call vpRxportsFinish at end each day? test footer bit even
			if report row skipped? ... ) */


/*------------- REPORT/EXPORT COLUMN DEFINITION DEFINITIONS ---------------*/

/* report/export column definition used by cgresult.cpp local fcns:
   specifies row of data from interval results structure, including column heads. */
struct COLDEF
{
	const char* colhd;	// column heading text, or (1-92) -1L for next var arg list text (fmtRpColhd only)
	UCH flags;			// format and column inclusion control bits, just below
	char width;			// report column width; export max width excluding (CVS) quotes
	char dfw;			// decimal places
	USI offset; 		// offset of field value in record, or USE_NEXT_ARG for next var arg list value
	char cvflag;		// CVS, CV1, CVK, CVM, CVI, CVWB, NOCV -- below
};

/* bits values for COLDEF.flags (just above):
	1:   suppresses space left of col.
        128: left justify column
        Column control (vs rxt.flags or flags arg to fmtRpColhd etc), see BLANKBITS and SKIPBITS just below:
		2:  report-only columns
		4:  export-only columns
		8:  "time" columns
		16: "name" column (for reports, used in all- reports only)
		32: M (zone mode) column
		64: * (shutter frac) column */
#define BLANKFLAGS 0			// .flags bit for which column is left blank if bit off in rxt.flags
#define SKIPFLAGS (2|4|8|16|32|64) 	// .flags bit for which column is omitted if bit off in rxt.flags

/* display formats for COLDEF:cvflag (subscripts in dTypes[], next) */
#define CV1  0	// value is float, display w/ no scaling
#define CVK  1	// value is float, display val/1000. (eg kBtu)
#define CVM  2	// value is float, display val/1000000. (eg MBtu)
#define BTU  3	// value is float, display val/rxt->btuSf (ie divide by user-inputable rpBtuSf): scaled Btu's
#define NBTU 4	// value is float, display -val/rxt->btuSf (ie divide by -user-inputable rpBtuSf): negative scaled Btu's
#define CVI  5	// value is SI,    display w/ no scaling
#define CVS  6	// value is string ONLY FOR offsets < 0 (variable arg list). Quotes are supplied if exporting
#define CVWB 7	// print wetbulb temp per float drybulb temp per PREVIOUS TABLE ENTRY & float hum rat per this entry
#define NOCV 8	// print no value (dummy entry, eg to hold ptr to unprinted drybulb temp for following CVWB)
// corresp datatypes:   CV1      CVK      CVM      BTU,     NBTU,    CVI   CVS   CVWB     NOCV
static int dTypes[] = { DTFLOAT, DTFLOAT, DTFLOAT, DTFLOAT, DTFLOAT, DTSI, DTCH, DTFLOAT, -1 };

#define USE_ARG_HD  ((char *)-1L)  	// use for .colhd to take column head from variable arg list not table
#define USE_NEXT_ARG (USI(-1))		// use for .offset to take data arg from variable argument list not record

/*----------------- REPORT/EXPORT COLUMN DEFINITION DATA ------------------*/

// columns definition for "energy balance" report (zone results, xEB reports)
/*lint -e507  "size incompatibility" for offsetof (in oRes macros) */
/* x lint -e569  "loss of information"  storing offset in UCH */
static COLDEF ebColDef[] =				// (far cuz init data won't drive Borland data threshold, 2-92)
	/*		                     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ USE_ARG_HD,    8|2|1,  4, 0, USE_NEXT_ARG,  CVS },	// report-only "time" column: "Mon", "Day", or "Hr", etc.
	{ USE_ARG_HD, 128|16|1, 17, 0, USE_NEXT_ARG,  CVS },	// report/export "name" column. 128: left justify
	{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,  CVI },	// export-only time columns
	{ "Day",           8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,  CVS },	// ..
	{ "Tair",            0,  5, 2, oRes(tAir),    CV1 },	// average air temp.   oRes: ZNRES_IVL_SUB offset, cnguts.h
	{ "WBair",           0,  5, 2, oRes(wAir),    CVWB },	// wetbulb for average t (previous entry) and w
	{ "Cond",            0,  6, 3, oRes(qCond),   BTU },	// wall conduction gain
	{ "InfS",            0,  6, 3, oRes(qsInfil), BTU },	// infiltration sensible gain. was "Infil".
	{ "Slr",             0,  5, 3, oRes(qSlr),    BTU },	// solar gain
	{ "*",          64|2|1,  1, 0, USE_NEXT_ARG,  CVS },	// report shutter frac. conditional col head. use next addl arg.
	{ "Shutter",         4,  1, 0, USE_NEXT_ARG,  CVS },	// export shutter frac: different colhd. use addl arg.
	{ "IgnS",            0,  5, 3, oRes(qsIg),    BTU },	// sensible internal gain. was "Ign".
	{ "Mass",            0,  6, 3, oRes(qMass),   BTU },	// net transfer from mass
	{ "Izone",           0,  6, 3, oRes(qsIz),    BTU },	// interzone gain to zone
	{ "Md",           32|2,  2, 0, USE_NEXT_ARG,  CVS },	// report zone mode: condi'l col head; formatted data in addl arg.
	{ "Md",              4,  2, 0, USE_NEXT_ARG,  CVI },	// export zone mode. (addl arg)
	{ "MechS",           0,  6, 3, oRes(qsMech),  BTU },	// sensible mechanical gain (all TUs)
#ifdef QBAL //if showing unbalances
	{ "BalS",            0,  4, 3, oRes(qsBal),   BTU },	// sensible unbalance (shd be ~0; cgenbal checks.
#endif
	{ "InfL",            0,  6, 3, oRes(qlInfil), BTU },	// latent infiltration gain
	{ "IgnL",            0,  5, 3, oRes(qlIg),    BTU },	// latent internal gain (from GAIN records)
	{ "IzoneL",          0,  6, 3, oRes(qlIz),    BTU },	// latent gain from IZXFER (vent, infil, duct leaks)
	{ "AirL",            0,  6, 3, oRes(qlAir),   BTU },	// latent q of w removed from zn air (neg when w rises)
	{ "MechL",           0,  6, 3, oRes(qlMech),  BTU },	// latent mechanical gain, usually <0
#ifdef QBAL //if showing unbalances
	{ "BalL",            0,  4, 3, oRes(qlBal),   BTU },	// latent unbalance (should be near 0). was "LatBal"
#endif
	{ 0,                 0,  0, 0, 0,             CV1 }
};

// columns definition for "statistics" report (ZST report)     <--- 6-92 all the nHr____'s are never set non-0.
static COLDEF stColdef[] =
	/*		                     offset             cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)         (above) */
{
	{ USE_ARG_HD,    8|2|1,  3, 0, USE_NEXT_ARG,         CVS },	// report "time" column: "Mon", "Day", or "Hr", etc.
	{ USE_ARG_HD, 128|16|1, 17, 0, USE_NEXT_ARG,         CVS },	// report/export "name" column. left justify.
	{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,         CVI },	// export-only time columns
	{ "Day",           8|4,  2, 0, USE_NEXT_ARG,         CVI },
	{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,         CVI },
	{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,         CVS },	// ..
	{ "NV hrs",          0,  6, 0, oRes(nHrNatv),        CVI },	// # hrs with any nat vent
	{ "FV hrs",          0,  6, 0, oRes(nHrFanv),        CVI },	// # hrs with any fan vent
#if 0 //  10-11-91
x   { "FV rntm",         0,  7, 2, oRes(runTimeFanv),    CV1 },	// fanVent tot runTime, hrs
x   { "F2 rntm",         0,  7, 2, oRes(runTimeFanv2),   CV1 },	// fanVent stg2 runTime, hrs
#endif // 10-11-91
	{ "CF hrs",          0,  6, 0, oRes(nHrCeilFan),     CVI },	// # hrs with any ceil fan
#if 0 // 10-11-91
x    { "CF rntm",         0,  7, 2, oRes(runTimeCeilFan), CV1 },	// ceilFan total runTime,hrs
#endif // 10-11-91
	{ "Cl hrs",          0,  6, 0, oRes(nHrCool),        CVI },	// # hrs with any cooling
	{ "Ht hrs",          0,  6, 0, oRes(nHrHeat),        CVI },	// # hrs with any heating
	{ 0,                 0,  0, 0, 0,                    CV1 }
};

// columns definition for "meter" (MTR) report/export
#define oMtr(m)  offsetof( MTR_IVL_SUB, m)
#define D 3		// decimal digits: 1 shd be enuf for MBtu; for Btu use 3, or 4 for more resolution of small numbers.
static COLDEF mtrColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ USE_ARG_HD,    8|2|1,  3, 0, USE_NEXT_ARG,  CVS },  	// report-only "time" column: "Mon", "Day", or "Hr", etc.
	{ "Meter",    128|16|1, 17, 0, USE_NEXT_ARG,  CVS },  	// report/export "name" column. 128: left justify
	{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,  CVI },  	// export-only time columns
	{ "Day",           8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,  CVS },  	// ..
	{ "Tot",             0,  6, D, oMtr(tot),     BTU },  	// total use
	{ "Clg",             0,  6, D, oMtr(clg),     BTU },  	// space cooling use
	{ "Htg",             0,  6, D, oMtr(htg),     BTU },  	// space heating incl heat pump compressor
	{ "HPBU",            0,  6, D, oMtr(hpBU),    BTU },  	// heat pump resistance heating (backup and defrost)
	{ "Dhw",             0,  6, D, oMtr(dhw),     BTU },  	// domestic (service) hot water heating
	{ "DhwBU",           0,  6, D, oMtr(dhwBU),   BTU },  	// domestic (service) hot water heating
	{ "DhwMFL",          0,  6, D, oMtr(dhwMFL),  BTU },  	// domestic (service) hot water DHWLOOP energy
	{ "FanC",            0,  6, D, oMtr(fanC),    BTU },  	// fans - cooling and cooling ventilation
	{ "FanH",            0,  6, D, oMtr(fanH),    BTU },  	// fans - heating
	{ "FanV",            0,  6, D, oMtr(fanV),    BTU },  	// fans - IAQ ventilation
	{ "Fan",             0,  6, D, oMtr(fan),     BTU },  	// fans - other
	{ "Aux",             0,  6, D, oMtr(aux),     BTU },  	// HVAC auxiliary, not including fans
	{ "Proc",            0,  6, D, oMtr(proc),    BTU },  	// process energy
	{ "Lit",             0,  6, D, oMtr(lit),     BTU },  	// lighting
	{ "Rcp",             0,  6, D, oMtr(rcp),     BTU },  	// receptacles
	{ "Ext",             0,  6, D, oMtr(ext),     BTU },  	// exterior
	{ "Refr",            0,  6, D, oMtr(refr),    BTU },  	// refrigeration
	{ "Dish",            0,  6, D, oMtr(dish),    BTU },  	// dish washing
	{ "Dry",             0,  6, D, oMtr(dry),     BTU },  	// clothes drying
	{ "Wash",            0,  6, D, oMtr(wash),    BTU },  	// clothes washing
	{ "Cook",            0,  6, D, oMtr(cook),    BTU },  	// cooking (range/oven)
	{ "User1",           0,  6, D, oMtr(usr1),    BTU },  	// user1
	{ "User2",           0,  6, D, oMtr(usr2),    BTU },  	// user2
	{ "BT",              0,  6, D, oMtr(bt),      BTU },    // battery
	{ "PV",              0,  6, D, oMtr(pv),      BTU },  	// photovoltaics
	{ 0,                 0,  0, 0, 0,             CV1 }
};
#undef D
#undef oMtr

// columns definition for "DHW meter" (DHWMTR) report/export
#define oWMtr(m)  offsetof( DHWMTR_IVL, m)
#define D 3		// decimal digits: 1 s/b enough for water use, gal
static COLDEF wMtrColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ USE_ARG_HD,    8|2|1,  3, 0, USE_NEXT_ARG,  CVS },  	// report-only "time" column: "Mon", "Day", or "Hr", etc.
	{ "Meter",    128|16|1, 17, 0, USE_NEXT_ARG,  CVS },  	// report/export "name" column. 128: left justify
	{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,  CVI },  	// export-only time columns
	{ "Day",           8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,  CVS },  	// ..
	{ "Total",           0,  7, D, oWMtr(total),  CV1 },
	{ "Unknown",         0,  7, D, oWMtr(unknown),CV1 },
	{ "Faucet",          0,  7, D, oWMtr(faucet), CV1 },
	{ "Shower",          0,  7, D, oWMtr(shower), CV1 },
	{ "Bath",            0,  7, D, oWMtr(bath),   CV1 },
	{ "CWashr",          0,  7, D, oWMtr(cwashr), CV1 },
	{ "DWashr",          0,  7, D, oWMtr(dwashr), CV1 },
	{ 0,                 0,  0, 0, 0,             CV1 }
};
#undef D
#undef oWMtr

// columns definition for "Airflow meter" (AFMTR) report/export
#define oAFMt0(m)  offsetof( AFMTR_IVL, m)
#define oAFMt1(m)  (offsetof( AFMTR_IVL, m)+sizeof( AFMTR_IVL))
#define D 1		// decimal digits
static COLDEF afMtrColdef[] =
/*		             max     offset       cvflag  */
/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ USE_ARG_HD,    8 | 2 | 1,  3, 0, USE_NEXT_ARG,  CVS },  	// report-only "time" column: "Mon", "Day", or "Hr", etc.
	{ "Meter",    128 | 16 | 1, 17, 0, USE_NEXT_ARG,  CVS },  	// report/export "name" column. 128: left justify
	{ "Mon",           8 | 4,  2, 0, USE_NEXT_ARG,  CVI },  	// export-only time columns
	{ "Day",           8 | 4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Hr",            8 | 4,  2, 0, USE_NEXT_ARG,  CVI },
	{ "Subhr",         8 | 4,  1, 0, USE_NEXT_ARG,  CVS },  	// ..
	{ "Tot+",           0,    5, D, oAFMt0(amt_total), CV1 },
	{ "Unkn+",          0,    5, D, oAFMt0(amt_unknown),  CV1 },
	{ "InfX+",          0,    5, D, oAFMt0(amt_infEx), CV1 },
	{ "VntX+",          0,    5, D, oAFMt0(amt_vntEx), CV1 },
	{ "FanX+",          0,    5, D, oAFMt0(amt_fanEx), CV1 },
	{ "InfU+",          0,    5, D, oAFMt0(amt_infUz), CV1 },
	{ "VntU+",          0,    5, D, oAFMt0(amt_vntUz), CV1 },
	{ "FanU+",          0,    5, D, oAFMt0(amt_fanUz), CV1 },
	{ "InfC+",          0,    5, D, oAFMt0(amt_infCz), CV1 },
	{ "VntC+",          0,    5, D, oAFMt0(amt_vntCz), CV1 },
	{ "FanC+",          0,    5, D, oAFMt0(amt_fanCz), CV1 },
	{ "Duct+",          0,    5, D, oAFMt0(amt_ductLk),CV1 },
	{ "HVAC+",          0,    5, D, oAFMt0(amt_hvac),  CV1 },
	{ "Tot-",           0,    6, D, oAFMt1(amt_total), CV1 },
	{ "Unkn-",          0,    6, D, oAFMt1(amt_unknown), CV1 },
	{ "InfX-",          0,    6, D, oAFMt1(amt_infEx), CV1 },
	{ "VntX-",          0,    6, D, oAFMt1(amt_vntEx), CV1 },
	{ "FanX-",          0,    6, D, oAFMt1(amt_fanEx), CV1 },
	{ "InfU-",          0,    6, D, oAFMt1(amt_infUz), CV1 },
	{ "VntU-",          0,    6, D, oAFMt1(amt_vntUz), CV1 },
	{ "FanU-",          0,    6, D, oAFMt1(amt_fanUz), CV1 },
	{ "InfC-",          0,    6, D, oAFMt1(amt_infCz), CV1 },
	{ "VntC-",          0,    6, D, oAFMt1(amt_vntCz), CV1 },
	{ "FanC-",          0,    6, D, oAFMt1(amt_fanCz), CV1 },
	{ "Duct-",          0,    6, D, oAFMt1(amt_ductLk),CV1 },
	{ "HVAC-",          0,    6, D, oAFMt1(amt_hvac),  CV1 },
	{ 0,                0,    0, 0, 0,                 CV1 }
};
#undef D
#undef oAFMt0
#undef oAFMt1

// columns definition for "air handler" (AH) report/export
#define oAhr(m)  offsetof( AHRES_IVL_SUB, m)
#define TW 4					// temp width, originally 5
static COLDEF ahColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ USE_ARG_HD,    8|2|1,  3, 0, USE_NEXT_ARG,  CVS  },  	// report-only "time" column: "Mon", "Day", or "Hr", etc.
	{ "Ah",       128|16|1, 17, 0, USE_NEXT_ARG,  CVS  },  	// report/export "name" column. 128: left justify
	{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,  CVI  },  	// export-only time columns
	{ "Day",           8|4,  2, 0, USE_NEXT_ARG,  CVI  },
	{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,  CVI  },
	{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,  CVS  },  	// ..
	{ "Tout",            0, TW, 1, oAhr(tDbO),    CV1  },  	// outdoor drybulb temp
	{ "WBout",           0, TW, 1, oAhr(wO),      CVWB },  	// .. and wetbulb temp using this w and db per PREVIOUS TABLE ROW
	//^^^  shouldn't this be 1, all temps in columns of width 4?
	//  presume left from showing W; 2 changed to 1 7-18-95.
	{ "Tret",            0, TW, 1, oAhr(tr),      CV1  },  	// return air temp
	{ "WBret",           0, TW, 1, oAhr(wr),      CVWB },  	// .. and wetbulb temp using this w and db per PREVIOUS TABLE ROW
	{ "po",              0,  3, 2, oAhr(po),      CV1  },	// part outside air (fraction)
	{ "Tmix",            0, TW, 1, oAhr(tmix),    CV1  },  	// mixed air temp
	{ "WBmix",           0, TW, 1, oAhr(wmix),    CVWB },  	// .. and wetbulb temp using this w and db per PREVIOUS TABLE ROW
	{ "Tsup",            0, TW, 1, oAhr(ts),      CV1  },  	// supply temp
	{ "WBsup",           0, TW, 1, oAhr(ws),      CVWB },  	// .. and wetbulb temp using this w and db per PREVIOUS TABLE ROW
	{ "HrsOn",           0,  5, 2, oAhr(hrsOn),   CV1  },	// hours on
	{ "FOn",             0,  3, 2, oAhr(frFanOn), CV1  },	// fraction of time fan on
	{ "VF",              0,  5, 0, oAhr(vf),      CV1  },	// volumetric flow (cfm)
	{ "Qheat",           0,  5, 3, oAhr(qh),      BTU  },  	// heating load
	{ "-Qsen",           0,  5, 3, oAhr(qs),      NBTU },  	// cooling sensible load.  Additional col cuz negative.
	{ "-Qlat",           0,  5, 3, oAhr(ql),      NBTU },  	// cooling latent load
	{ "-Qcool",          0,  5, 3, oAhr(qc),      NBTU },  	// cooling total load
	{ "Qout",            0,  5, 3, oAhr(qO),      BTU  },  	// outdoor air q (enthalpy, tentatively rel tr/wr 6-92)
	{ "Qfan",            0,  4, 3, oAhr(qFan),    BTU  },  	// fan heat
	{ "Qloss",           0,  5, 3, oAhr(qLoss),   BTU  },  	// leaks & losses (enthalpy change)
	{ "Qload",           0,  6, 3, oAhr(qLoad),   BTU  },  	// zone load and zone, tu, plenum losses
#ifdef QBAL
	{ "Qbal",            0,  4, 3, oAhr(qBal),    BTU  },  	// balance -- should add to near 0 -- undisplay later?
#endif
	{ 0,                 0,  0, 0, 0,             CV1  }
};
#undef oAhr

// columns definition for "air handler sizing" (AHSIZE) report/export. 6-95. Annual only. Same vpRxRow arg list as ahLdColdef.
#define oAh(m)  offsetof( AH, m)
#define TW 4					// temp width, originally 5
static COLDEF ahSzColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ "Ah",       128|16|1, 17, 0, USE_NEXT_ARG,            CVS  },  	// "name" column for exports and all- reports. 128: LJ.
	{ "PkVf",            0,  5, 0, oAh(fanAs.ldPkAs),       CV1  },	// peak fan cfm
	{ "VfDs",            0,  5, 0, oAh(sfan.vfDs_AsNov),    CV1  },	// fan rated cfm, no oversize
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for Input or AutoSized fan cfm. No space to left.
	{ "PkQH",            0,  5, 3, oAh(hcAs.ldPkAs),        BTU  },	// peak heat load. Or show peak plr?
	{ "hCapt",           0,  5, 3, oAh(ahhc.captRat_AsNov), BTU  },	// heat coil rated cap'y, no oversize to match other data
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for heat col
	{ "PkMo",            0,  4, 0, oAh(qcPkMAs),            CVI  },	// cool coil load peak month 1-12, or 0 heat des day
	//{ "Dy",              0,  2, 0, oAh(qcPkDAs),            CVI  },	// peak day 1-31, unused for autoSizing
	{ "Hr",              0,  2, 0, oAh(qcPkHAs),            CVI  },	// peak hour 1-24
	{ "Tout",            0,  4, 1, oAh(qcPkTDbOAs),         CV1  },	// outdoor drybulb temp @ cool coil load peak
	{ "WBout",           0,  4, 1, oAh(qcPkWOAs),           CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "Ten",             0,  4, 1, oAh(qcPkTenAs),          CV1  },	// entering air temp @ cool coil load peak
	{ "WBen",            0,  4, 1, oAh(qcPkWenAs),          CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "Tex",             0,  4, 1, oAh(qcPkTexAs),          CV1  },	// exiting air temp @ cool coil load peak
	{ "WBex",            0,  4, 1, oAh(qcPkWexAs),          CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "-PkQs",           0,  5, 3, oAh(qcPkSAs),            NBTU },	// cool coil peak sens load
	{ "-PkQl",           0,  5, 3, oAh(qcPkLAs),            NBTU },	// cool coil peak latent load
	{ "-PkQC",           0,  5, 3, oAh(ccAs.ldPkAs),        NBTU },	// cool coil peak total load. Or show plr?
	{ "CPlr",            0,  4, 3, oAh(ccAs.plrPkAs),       CV1  },	// cool coil peak plr (not necess at time of peak load)
	{ "CCapt",           0,  5, 3, oAh(ahcc.captRat_AsNov), NBTU },	// cool coil rated total capacity, no oversize
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for cool col
	{ "CCaps",           0,  5, 3, oAh(ahcc.capsRat_AsNov), NBTU },	// cool coil rated sens capacity
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for cool col (again)
	{ 0,                 0,  0, 0, 0,                       CV1  }
};

// columns definition for "air handler load" (AHLOAD) report/export. 6-95. Annual only. Same vpRxRow arg list as ahLSzColdef.
static COLDEF ahLdColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ "Ah",       128|16|1, 17, 0, USE_NEXT_ARG,            CVS  },  	// "name" col for exports & all- reports. 128: left just.
	//{ "Mon",           8|4,  2, 0, USE_NEXT_ARG,            CVI  },  	// export-only time columns
	//{ "Day",           8|4,  2, 0, USE_NEXT_ARG,            CVI  },	// .. to use these add to arg list
	//{ "Hr",            8|4,  2, 0, USE_NEXT_ARG,            CVI  },	// .. "&rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS"
	//{ "Subhr",         8|4,  1, 0, USE_NEXT_ARG,            CVS  },  	// ..
	{ "PkVf",            0,  5, 0, oAh(fanAs.ldPk),         CV1  },	// peak fan cfm
	{ "VfDs",            0,  5, 0, oAh(sfan.vfDs),          CV1  },	// fan rated cfm
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for Input or AutoSized fan cfm. No space to left.
	{ "PkQH",            0,  5, 3, oAh(hcAs.ldPk),          BTU  },	// peak heat load. Or show peak plr?
	{ "hCapt",           0,  5, 3, oAh(ahhc.captRat),       BTU  },	// heat coil rated capacity
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for heat col
	{ "PkMo",            0,  4, 0, oAh(qcPkM),              CVI  },	// cool coil load peak month 1-12, or 0 heat des day
	{ "Dy",              0,  2, 0, oAh(qcPkD),              CVI  },	// peak day 1-31, unused for autoSizing
	{ "Hr",              0,  2, 0, oAh(qcPkH),              CVI  },	// peak hour 1-24
	{ "Tout",            0,  4, 1, oAh(qcPkTDbO),           CV1  },	// outdoor drybulb temp @ cool coil load peak
	{ "WBout",           0,  4, 1, oAh(qcPkWO),             CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "Ten",             0,  4, 1, oAh(qcPkTen),            CV1  },	// entering air temp @ cool coil load peak
	{ "WBen",            0,  4, 1, oAh(qcPkWen),            CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "Tex",             0,  4, 1, oAh(qcPkTex),            CV1  },	// exiting air temp @ cool coil load peak
	{ "WBex",            0,  4, 1, oAh(qcPkWex),            CVWB },	// .. and wetbulb using this w and preceding tbl entry t
	{ "-PkQs",           0,  5, 3, oAh(qcPkS),              NBTU },	// cool coil peak sens load
	{ "-PkQl",           0,  5, 3, oAh(qcPkL),              NBTU },	// cool coil peak latent load
	{ "-PkQC",           0,  5, 3, oAh(ccAs.ldPk),          NBTU },	// cool coil peak total load
	{ "CPlr",            0,  4, 3, oAh(ccAs.plrPk),         CV1  },	// cool coil peak plr (not necess at time of peak load)
	{ "CCapt",           0,  5, 3, oAh(ahcc.captRat),       NBTU },	// cool coil rated capacity
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for cool col
	{ "CCaps",           0,  5, 3, oAh(ahcc.capsRat),       NBTU },	// cool coil rated sens capacity
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for cool col (again)
	{ 0,                 0,  0, 0, 0,                       CV1  }
};
#undef oAh

// columns definition for "terminal sizing" (TUSIZE) report/export. 6-95. Annual only. Same vpRxRow arg list asa tuLdColdef.
#define oTu(m)  offsetof( TU, m)
static COLDEF tuSzColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ "Tu",       128|16|1, 17, 0, USE_NEXT_ARG,            CVS  },  	// "name" col for exports & all- reports. 128: left just.
	{ "Zone",       128|16, 17, 0, USE_NEXT_ARG,            CVS  },	// tu's zone. LJ. Omit here when name not shown here.
	{ "PkQLh",           0,  5, 3, oTu(hcAs.ldPkAs),        BTU  },	// peak local heat output
	{ "hcCapt",          0,  5, 3, oTu(tuhc.captRat_AsNov), BTU  },	// local heat coil capacity, less oversize for consistency
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A (Input/AutoSized) for heat coil capacity
	{ "PkQH",            0,  5, 3, oTu(qhPkAs),             BTU  },	// peak air heat output
	{ "PkVfH",           0,  5, 0, oTu(vhAs.ldPkAs),        CV1  },	// peak air heat cfm
	{ "VfMxH",           0,  5, 0, oTu(tuVfMxH_AsNov),      CV1  },	// air heat rated max cfm, no oversize in autoSize value
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for tuVfMxH
	{ "-PkQC",           0,  5, 3, oTu(qcPkAs),             NBTU },	// peak air cool output
	{ "PkVfC",           0,  5, 0, oTu(vcAs.ldPkAs),        CV1  },	// peak air cool cfm
	{ "VfMxC",           0,  5, 0, oTu(tuVfMxC_AsNov),      CV1  },	// air cool rated cfm
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for tuVfMxC
	{ 0,                 0,  0, 0, 0,                       CV1  }
};

// columns definition for "terminal load" (TULOAD) report/export. 6-95. Annual only. Same vpRxRow arg list asa tuSzColdef.
static COLDEF tuLdColdef[] =
	/*		             max     offset       cvflag  */
	/* colhd           flags  wid dfw (cnguts.h)   (above) */
{
	{ "Tu",       128|16|1, 17, 0, USE_NEXT_ARG,            CVS  },	// "name" col for exports & all- reports. 128: left just.
	{ "Zone",       128|16, 17, 0, USE_NEXT_ARG,            CVS  },	// tu's zone. LJ. Omit here when name not shown here.
	{ "PkQLh",           0,  5, 3, oTu(hcAs.ldPk),          BTU  },	// peak local heat output
	{ "hcCapt",          0,  5, 3, oTu(tuhc.captRat),       BTU  },	// local heat coil capacity
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A (Input/AutoSized) for heat coil capacity
	{ "PkQH",            0,  5, 3, oTu(qhPk),               BTU  },	// peak air heat output
	{ "PkVfH",           0,  5, 0, oTu(vhAs.ldPk),          CV1  },	// peak air heat cfm
	{ "VfMxH",           0,  5, 0, oTu(tuVfMxH),            CV1  },	// air heat rated max cfm
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for tuVfMxH
	{ "-PkQC",           0,  5, 3, oTu(qcPk),               NBTU },	// peak air cool output
	{ "PkVfC",           0,  5, 0, oTu(vcAs.ldPk),          CV1  },	// peak air cool cfm
	{ "VfMxC",           0,  5, 0, oTu(tuVfMxC),            CV1  },	// air cool rated cfm
	{ "",                1,  1, 0, USE_NEXT_ARG,            CVS  },	// I/A for tuVfMxC
	{ 0,                 0,  0, 0, 0,                       CV1  }
};
#undef oTu
/*lint +e507*/
/*lint +e569*/

/*------------------------------ Info Structure RXPORTINFO ------------------------------*/

// report/export(s) addl local info structure for vpRxPorts and callees, used in conjunction with DVRI record.

struct RXPORTINFO				// instantiated as "rxt" in vpRxports and vpRxFooter.
{
// basic inputs for report time, set in vpRxPorts.
	IVLCH fq; 		// interval's reports being done (rpFreq): C_IVLCH_Y, M, D, H, S from caller, HS generated in vpRxports.
	IVLCH fqr;		// interval's results to use: remains H or S when doing rows in HS (hourly+subhourly) reports

// re time of report
	char col1[5];			// for ZEB report and ZST report: "time" column 1: month, day, or hour
	SI xebM, xebD, xebH;
	char xebS[2];	// for ZEB and ZST export: month-day-hour-subhour

// specific to report: set by vpRxPorts inside loop
	USI flags;    	/* column control bits: 2 include report-only cols, 4 export-only cols, 8 time columns, 16 name col,
    			   32 rpt zone mode, 64 rpt shutter fraction.  see SKIPFLAGS and BLANKFLAGS defines above. */
	COLDEF *colDef;	// column info table, used re both col heads and data
};

/*-------------------------------- OTHER DATA -----------------------------*/

static char * NEAR shortIvlTexts[] = { "bg0", "Yr ", "Mon", "Day", "Hr ", "sHr", "s/h" }; 	// subscript is IVLCH value

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

LOCAL void FC   NEAR vpRxHeader( DVRI *dvrip, RXPORTINFO *rxt);
LOCAL void FC   NEAR vpRxFooter( DVRI *dvrip);
LOCAL void FC NEAR vpEbStRow( DVRI *dvrip, RXPORTINFO *rxt, TI zi);
LOCAL void FC NEAR vpMtrRow( DVRI *dvrip, RXPORTINFO *rxt, TI mtri);
LOCAL void FC NEAR vpAhRow( DVRI *dvrip, RXPORTINFO *rxt, TI mtri);
LOCAL void FC NEAR vpAhSzLdRow( DVRI *dvrip, RXPORTINFO *rxt, TI ahi);
LOCAL void FC NEAR vpTuSzLdRow( DVRI *dvrip, RXPORTINFO *rxt, TI tui);
LOCAL void CDEC NEAR vpRxRow( DVRI *dvrip, RXPORTINFO *rxt, void *zr, ...);
LOCAL void FC   NEAR vpUdtRpColHeads( DVRI *dvrip);
LOCAL void FC   NEAR vpUdtExColHeads( DVRI *dvrip);
LOCAL void FC   NEAR vpUdtRpRow( DVRI *dvrip);
LOCAL void FC   NEAR vpUdtExRow( DVRI *dvrip);
LOCAL char * CDEC NEAR fmtRpColhd( COLDEF *colDef, char *buf, USI flags, ...);
LOCAL char * CDEC NEAR fmtExColhd( COLDEF *colDef, char *buf, USI flags, ...);




//************************ DATE-DEPENDENT REPORTS/EXPORTS SUPPORT FUNCTIONS ***********************


//==========================================================================================================
void FC cgReportsDaySetup()	// Init all zns re day/hour/subhour reports this day. Uses Top.jDay.

// sets Top members .dvriY, -M, -D, -H, -S, -HS to head of lists
// (in DvriB) of vr's in category that should receive output today
{
	DVRI *dvrip;
	RLUP( DvriB, dvrip)			// initial dvrip loop
		dvrip->nextNow = 0;		// clear all today's-reports list pointers

	Top.dvriY = Top.dvriM = Top.dvriD = Top.dvriH = Top.dvriS = Top.dvriHS = 0;  	// init vr interval lists empty

// form linked lists by time interval of active "Date-dependent Virtual Report Info" records

	RLUP( DvriB, dvrip)						// loop over DVRI records (generated by cncult4.cpp)
	{
		// Test whether report active on current simulation date.
		if (dvrip->dv_IsReportActive( Top.jDay))
		{
			// put report on active linked list for its frequency
			TI *dvri = &Top.dvriY + dvrip->rpFreq - 1;	// subscript by rpFreq to point at list head Top.dvriY,M,D,H,S,or HS.
			dvrip->nextNow = *dvri;				// set active report's next-active ptr to current head of list
			*dvri = dvrip->ss;					// replace list head with report's subscript
		}
#ifdef DEFFOOT	// not needed: no footers not on full-year reports 1-6-92
o       else 							// report not active today
o			if (vrGetOptn(dvrip->vrh) & VR_NEEDFOOT) 		// if has pending footer (active yesterday, last row rpCond'd out)
o				vpRxFooter(dvrip);					// do the footer, below.  clears VR_NEEDFOOT.
o						/* note: do footer now rather than at end run to keep report's text
o						   close to contiguous in virtual report spool file. */
#endif
	}

// set flags if any hourly/subhourly reporting/exporting for this zone this day

	Top.hrxFlg = Top.dvriH | Top.dvriHS;		// if any each hour.    tested for speed in cnguts.cpp:doIvlReports.
	Top.shrxFlg = Top.dvriS | Top.dvriHS;		// if any each subhour. tested for speed in cnguts.cpp:doIvlReports.

}					// cgReportsDaySetup
//-----------------------------------------------------------------------------
bool DVRI::dv_IsReportActive(		// check if report is active
	DOY jDay) const	// simulation day of year (1 - 365/366)
// returns true iff this report should output on day jDay
{
	bool bActive{ false };
	if (rpDayEnd >= rpDayBeg)
		bActive = jDay >= rpDayBeg && jDay <= rpDayEnd;
	else
		bActive = jDay >= rpDayBeg || jDay <= rpDayEnd;

	return bActive;

}	// DVRI::dv_IsReportActive


//********************************** RESULTS PRINTING FUNCTIONS *********************************


//==========================================================================================================
void FC vpRxportsFinish()		// finish off reports and exports at end run: eg any incomplete footers
{
#ifdef DEFFOOT	// not now needed: no footers not on full-year reports 1-6-92
o    DVRI *dvrip;
o    RLUP (DvriB, dvrip)    				// loop date-dependent virtual reports
o       if (vrGetOptn(dvrip->vrh) & VR_NEEDFOOT) 	/* if has pending footer (occurs if last row rpCond'd out and
o    							   repDaySetupZn didn't do footer, eg if active til end run) */
o          vpRxFooter(dvrip);				// do the footer, below.  clears VR_NEEDFOOT.
#endif
}				// vpRxportsFinish
//==========================================================================================================
void FC vpRxports( 	// virtual print reports and exports of given frequency for current interval

	IVLCH freq,  		// interval desired: C_IVLCH_Y, M, D, H, S.
						// HS generated internally when H or S given. HS cannot be given by caller.
	BOO auszOnly/*=FALSE*/ )	// non-0 to do ONLY autosizing results reports (AHSIZE, TUSIZE, UDT).
						// used after autosize when main sim will not be performed.
						// Experiment 5-97 to fix oversight in original ausz implementation;
						// possibly autosize results reports should always be (have been) done this way
						// (always call after autosize, then exclude them when auszOnly FALSE).
{
	int reHead = 0;					// set non-0 to print report title & col heads even if not 1st time
	int doFoot = 0;					// non-0 for report end: blank line, Yr summary, etc
	RXPORTINFO rxt;					// much info to pass to/amoung callees

	memset( &rxt, 0, sizeof(rxt));			// default all info members to 0
	rxt.fqr = rxt.fq = freq;  				// store args

// init by frequency; each case breaks; stuff used by more than one report type

	switch (rxt.fq)
	{
	case C_IVLCH_S:
		sprintf(rxt.col1, "%2d%s", Top.iHr + 1, strSuffix( Top.iSubhr));
		reHead = (Top.isBegDay && Top.iSubhr == 0);		// subhrly and HS rpts get title/colHeads once day
		doFoot = (Top.isEndDay && Top.isEndHour);		// .. and termination (blank line) at end each day
		break;
	case C_IVLCH_H:
		sprintf( rxt.col1, "%2d", Top.iHr+1);
		reHead = Top.isBegDay;			// hourly rpts get title/colHeads each day
		doFoot = Top.isEndDay;			// .. and termination (blank line) at end each day
		break;
	case C_IVLCH_D:
		sprintf( rxt.col1, "%2d", Top.tp_date.mday);
		reHead = Top.isBegMonth;
		doFoot = Top.isEndMonth;
		break;
	case C_IVLCH_M:
		strcpy( rxt.col1, Top.monStr);
		doFoot = Top.isLastDay;
		break;
	case C_IVLCH_Y:
		strcpy( rxt.col1, "Yr ");
		doFoot = Top.isLastDay;
		break;
	default:
		err( PWRN, (char *)MH_R0150, (INT)rxt.fq);   	// "cgresult:vrRxports: unexpected rpFreq %d"
	}

// init that applies to frequency and more often: fall thru cases.

	switch (rxt.fq)
	{
	case C_IVLCH_S:
		rxt.xebS[0] = 'a' + (char)Top.iSubhr; 	// ZEB export subhour: leave "" if not subhourly
	case C_IVLCH_H:
		rxt.xebH = Top.iHr + 1;         	// ZEB export hour: leave 0 if daily, M, Y.
	case C_IVLCH_D:
		rxt.xebD = Top.tp_date.mday;		// ZEB export day: leave 0 if monthly or [yearly]
	case C_IVLCH_M:
		rxt.xebM = Top.tp_date.month;
	case C_IVLCH_Y:
	default:
		break;
	}

// loop over reports active for given frequency

	for ( ; ; )						// repeats to do HS if rpfreq==H or S.
	{	DVRI *dvrip /*=NULL*/;
		for (int i = (&Top.dvriY)[rxt.fq-1];  i;  i = dvrip->nextNow)	// loop over DvriB records in list for interval
		{
			dvrip = DvriB.p + i;				// point Date-dependent Virtual Report Info record
			RPTYCH rpTy = dvrip->rpTy;    		// fetch report type
			int vrh = dvrip->vrh;				// fetch output destination
			int isExport = dvrip->isExport; 	// true if export (not report)
			int isAll = dvrip->isAll;  			// true if zi or mtri or ... is TI_ALL
			rxt.flags = (rxt.flags &~(4|2)) | (isExport ? 4 : 2);	// set export (4) or report (2) flag bit; clear prior.
			rxt.flags = (rxt.flags &~(16|8))			// set name (16) & time (8) col flags r/xport
				| ( isExport ? (16|8) 			// exports show BOTH name and the 4 export time columns
					: (isAll ? 16 : 8) ); 		// all- reports show name, others rpts show time column
			rxt.flags = (rxt.flags &~(64|32))	// include shutter frac * (64) and Mode (32) columns
				| ( rxt.fq >= C_IVLCH_S	  		// .. in _HS and _S reports
				    && (dvrip->ownTi > 0 || isAll )   	// .. for a single zone or all zones
				|| isExport 					// .. and in all exports (keep format constant)
					? (64|32) : 0 );			// (but data is blanked/0'd below in non-subhr lines)

			// footer line: also on last day of day, month, or year report
			doFoot |=  (rxt.fq <= C_IVLCH_D  &&  Top.jDay==dvrip->rpDayEnd);

			// skip non-autosizing-results reports on parameter
			if (auszOnly)					// if caller said autoSize only
				if ( rpTy != C_RPTYCH_AHSIZE
				 &&  rpTy != C_RPTYCH_TUSIZE
				 &&  rpTy != C_RPTYCH_UDT)		// UDT: assume user knows what s/he is doing
					continue;

			// skip report/export this interval if rpCond is FALSE

			if (ISNANDLE(dvrip->rpCond))					// if condition UNSET (bug) or not yet evaluated
			{
				rer( (char *)MH_R0152,					// "%sCond for %s '%s' is unset or not yet evaluated"
					isExport ? "ex" : "rp",   isExport ? "export" : "report",
					dvrip->rpTitle && *dvrip->rpTitle ? dvrip->rpTitle : dvrip->name );	// title if any, else aname
				continue;									// treat as FALSE
			}
			if (!(SI)dvrip->rpCond)		// if condition false (value is SI, storage is LI to hold NAN for expr)
			{
				if (reHead)					// if header needed, set optn bit to invoke it when not skipped
					vrChangeOptn( vrh, VR_NEEDHEAD, VR_NEEDHEAD);	// ..
#ifdef DEFFOOT	// not needed: no footers not on full-year non-rpcond reports 1-6-92 (no longer true 1-20-92...)
o				if (doFoot || isAll)	 			// footer likewise
o					vrChangeOptn( vrh, VR_NEEDFOOT, VR_NEEDFOOT);	// (need a way to print last footer even if no more rows printed)
#endif
				continue;						// proceed to next report/export
			}

			// determine table / other exceptions.  Unexpected exports fall thru as corresponding report types.

			switch (rpTy)
			{
			case C_RPTYCH_ZST:
				rxt.colDef = stColdef;
				break;  	// Zone Statistics, aka conditions, report

			case C_RPTYCH_ZEB:
				rxt.colDef = ebColDef;
				break;  	// Zone Energy Balance, incl SUM and ALL

			case C_RPTYCH_MTR:
				rxt.colDef = mtrColdef;
				break;  	// Meter report or export

			case C_RPTYCH_DHWMTR:
				rxt.colDef = wMtrColdef;
				break;  	// DHW meter report or export

			case C_RPTYCH_AFMTR:
				rxt.colDef = afMtrColdef;
				break;		// Airflow meter report or export

			case C_RPTYCH_AH:
				rxt.colDef = ahColdef;
				break;  	// Air handler report or export
			case C_RPTYCH_AHSIZE:
				rxt.colDef = ahSzColdef;
				break;	// Air handler sizing report or export 6-95
			case C_RPTYCH_AHLOAD:
				rxt.colDef = ahLdColdef;
				break;	// Air handler load report or export 6-95

			case C_RPTYCH_TUSIZE:
				rxt.colDef = tuSzColdef;
				break;	// Terminal sizing report or export 6-95
			case C_RPTYCH_TULOAD:
				rxt.colDef = tuLdColdef;
				break;	// Terminal load report or export 6-95

			case C_RPTYCH_UDT:
				break;				// UDT uses no COLDEF table

			default:
				err( PWRN, (char *)MH_R0151, (INT)rpTy);  	// "cgresult.c:vpRxports: unexpected rpType %d"
			}

#ifdef DEFFOOT	/* no reports with conditional rows have footers, 1-6-92.  (no longer true 1-20-92; tentatively let
o		the skipped footer be omitted; vpRxFooter defends against mistimed calls) */
o      // if there is a pending footer (due to skipped conditional row) do it now
o
o			if (vrGetOptn(vrh) & VR_NEEDFOOT)     	// if foot request pending from skipped line
o				vpRxFooter(dvrip);	    		// virtual print report or export footer, below. clears VR_NEEDFOOT.
#endif

			// heading first time, and periodically per report type

			if ( vrIsEmpty(vrh)    			// if nothing yet output to this virtual report
				||  !isExport				// exports do not get reHeaded
				&& ( reHead 				// if a heading due now (set in init)
				  || vrGetOptn(vrh) & VR_NEEDHEAD	// or heading request pending from prior skipped line
				  || isAll ) )			// All- reports gets header (and footer) every time
					vpRxHeader( dvrip, &rxt); 		// do report/export heading, below.  clears VR_NEEDHEAD.

				// report/export row (or body for all-zones, all-meter reports)

			switch (rpTy)
			{
				case C_RPTYCH_ZST:  				// Statistics report
				case C_RPTYCH_ZEB:  				// Energy balance report
					if (isAll)  			// for all zones, loop zones here
					{
						ZNR *zp1;
						RLUP( ZrB, zp1)
						vpEbStRow( dvrip, &rxt, zp1->ss);
					}
					else					// sum of zones or one zone
						vpEbStRow( dvrip, &rxt, dvrip->ownTi);
					break;

				case C_RPTYCH_MTR:						// meter report
					if (isAll)
					{
						MTR *mtr1;
						RLUP( MtrB, mtr1)			// loop over meter records
						if (mtr1->ss < MtrB.n)			// omit last (sum) rec here (vpRxFooter shows it)
							vpMtrRow( dvrip, &rxt, mtr1->ss);	// print row of text for this meter
					}
					else
						vpMtrRow( dvrip, &rxt, dvrip->mtri);	// print specified one, or sum (TI_SUM)
					break;

				case C_RPTYCH_DHWMTR:						// DHW meter report
					if (isAll)
					{	DHWMTR* pWM;
						RLUP( WMtrR, pWM)			// loop over records
							dvrip->dv_vpDHWMtrRow( &rxt, pWM->ss);	// print row of text for this meter
					}
					else
						dvrip->dv_vpDHWMtrRow( &rxt);	// print specified one
					break;

				case C_RPTYCH_AFMTR:						// DHW meter report
					if (isAll)
					{	AFMTR* pAM;
						RLUP(AfMtrR, pAM)			// loop over records
							dvrip->dv_vpAfMtrRow(&rxt, pAM->ss);	// print row of text for this meter
					}
					else
						dvrip->dv_vpAfMtrRow(&rxt);	// print specified one
					break;

				case C_RPTYCH_AH:						// air handler report
					if (isAll)
					{
						AHRES *ahr1;
						RLUP( AhresB, ahr1)			// loop over air handler result records
						if (ahr1->ss < AhresB.n)    		// omit last (sum) rec here (vpRxFooter shows it)
							vpAhRow( dvrip, &rxt, ahr1->ss);	// print row of text for this air handler
					}
					else
						vpAhRow( dvrip, &rxt, dvrip->ahi);  	// print specified one, or sum (TI_SUM)
					break;

				case C_RPTYCH_UDT:
					if (isExport)
						vpUdtExRow(dvrip);  		// user-defined export row, below
					else
						vpUdtRpRow(dvrip);  		// user-defined report row, below
					break;

				case C_RPTYCH_AHSIZE:					// air load handler size report 6-95
				case C_RPTYCH_AHLOAD: 					// air load handler load report 6-95
					if (isAll)
					{
						AH *ah;
						RLUP( AhB, ah)				// loop over air handler records
						vpAhSzLdRow( dvrip, &rxt, ah->ss);  	// print row of text for this air handler
					}
					else
						vpAhSzLdRow( dvrip, &rxt, dvrip->ahi);  	// print specified one
					break;

				case C_RPTYCH_TUSIZE:					// terminal size report 6-95
				case C_RPTYCH_TULOAD: 					// terminal load report 6-95
					if (isAll)
					{
						TU *tu;
						RLUP( TuB, tu)				// loop over air handler records
						vpTuSzLdRow( dvrip, &rxt, tu->ss);  	// print row of text for this terminal
					}
					else
						vpTuSzLdRow( dvrip, &rxt, dvrip->tui);  	// print specified one
					break;

				default:
					break;
				} // switch (rpTy)

			// conditional report footer

			if (doFoot || isAll)  			// if foot needed per flag set in init (all- reports footed every time)
				vpRxFooter( dvrip);	  		// virtual print report or export footer, below.

		}  // for i=

	// if hourly or subhourly specified, repeat for hourly+subhourly
	//   hourly+subhourly reports are in a separate list as there is only one .nextNow in DVRI.
		if (rxt.fq==C_IVLCH_H)			// if caller said do hour hourly lines
		{	rxt.fq = C_IVLCH_HS;		// do the hourly lines of the HS reports/exports too
			reHead = 0;					// no header at hrly line of HS's: header happened at subHour 0.
		}								// and iterate the for ( ; ; ).
		else if (rxt.fq==C_IVLCH_S)		// if doing subhourly lines
		{	rxt.fq = C_IVLCH_HS;		// do subhourly lines of HS reports/exports too
			doFoot = 0;					// but do no footers for HS's: they happen at end hour, at hour call.
		}								// and iterate the for ( ; ; ).
		else
			break;					// otherwise, done

	}  // for ; ;

}		// vpRxports
//==========================================================================================================
LOCAL void FC NEAR vpRxHeader( 		// do report/export header appropropriate for type and frequency

	DVRI *dvrip, 	// date-dependent virtual report info record set up before run by cncult4.cpp
	RXPORTINFO *rxt)	// addl report/export info struct set in vpRxports

// called at first output and first hour each day if daily, first day each month if monthly, etc
{
	SI isExport = dvrip->isExport;
	SI vrh = dvrip->vrh;
	IVLCH rpFreq = dvrip->rpFreq;
	RPTYCH rpTy = dvrip->rpTy;
	SI isAll = dvrip->isAll;		// true if All- (zones or meters) report
	const char *objTx = "";			// "zone <name>", "All Zones", "Sum of Zones", etc,  or "meter <name>" etc
	const char *what = NULL;		// set to "Energy Balance", "Statistics", etc for standard report title & col heads per colDef

	char *fqTx = "<bad rpFreq> ";	// "Annual ", "Monthly ", etc for use in report title text
	char *ivlTx = "Bug";		// "Year", "Month", etc for use in All-  export head (Y,M,D,H only)
	const char *when = "";		// "" or monStr, dateStr, etc to add " for ..." to report title
	char *hd1;					// "Mon", "Day" etc short text for 1st ZEB/ZST/MTR/AH report col head.
	char *xhd1 = "";			// export col 1 head: object name
	SI hour = 0;				// 0 or hour 1-24 to show in report heading (all- reports)
	SI subHour = -1;			// -1 or subhour 0.. to show in report heading (all- reports)

	vrChangeOptn(vrh, VR_NEEDHEAD, 0); 		// clear "heading request pending" flag
	if (dvrip->rpHeader == C_RPTHDCH_NO)			// if header suppressed for this report/export by input command
		return;						// done

// get rpFreq-dependent title substrings.  Note All- reports (no date col), get one more time subdivision in title.

	hd1 = shortIvlTexts[rpFreq];	// get col 1 head text from file-level short interval texts; Yr may be changed in switch.
	switch (rpFreq)
	{
	case C_IVLCH_S:
		fqTx = "Subhourly ";
		ivlTx = "Subhour";
		when = Top.dateStr;
		break;
	case C_IVLCH_HS:
		fqTx = "Hourly and Subhourly ";
		ivlTx = "Hour + Subhour";
		when = Top.dateStr;
		break;
	case C_IVLCH_H:
		fqTx = "Hourly ";
		ivlTx = "Hour";
		when = Top.dateStr;
		break;
	case C_IVLCH_D:
		fqTx = "Daily ";
		ivlTx = "Day";
		when = Top.monStr;
		break;
	case C_IVLCH_M:
		fqTx = "Monthly ";
		ivlTx = "Month";
		break;
	case C_IVLCH_Y:
		switch (rpTy)
		{
		case C_RPTYCH_AHSIZE:
		case C_RPTYCH_AHLOAD: 	// for these non-date-dependent reports,
		case C_RPTYCH_TUSIZE:
		case C_RPTYCH_TULOAD:  	// in their expected frequency,
			fqTx = "";
			break;		// omit "Annual" from header, 6-95.
		default:
			fqTx = "Annual ";
			break;
		}
		ivlTx = "Year";   /* ?? hd1 = "";*/
		break;
	default:
		;
	}
	if (isAll)			// for all- reports, show next smaller time division in header.  (Also hd1 changed below.)
		switch (rxt->fqr)		// .fqr = rpFreq except for HS, .fqr is H or S according to row now being printed
		{
		case C_IVLCH_S:
			subHour = Top.iSubhr;		// show subhour
			/*lint -e616			case falls thru */
		case C_IVLCH_H:
			hour = Top.iHr + 1;
			break;	// show hour as well as date
		case C_IVLCH_D:
			when = Top.dateStr;
			break;
		case C_IVLCH_M:
			when = Top.monStr;
			break;
		default:;	 /*lint +e616 */
		}

	// get texts describing object(s) report/export is for

	if (dvrip->ownTi)			// if zone-based report (ZEB, ZST, )
	{
		switch (dvrip->ownTi)
		{
			ZNR *zp1;
		default:
			zp1 = ZrB.p + dvrip->ownTi;   						// one specific zone
			objTx = isExport ? zp1->name : strtprintf("zone \"%s\"", zp1->name);
			break;
		case TI_ALL:
			objTx = "All Zones";
			hd1 = "Zone";
			break;			// 1st col is object name not time
		case TI_SUM:
			objTx = "Sum of Zones";
			break;
		}
		xhd1 = "Zone";					// export 1st col always name (and 4 time cols always present)
	}
	else if (dvrip->mtri)			// if meter-based report (MTR)
	{
		switch (dvrip->mtri)
		{
			MTR *mtr1;
		default:
			mtr1 = MtrB.p + dvrip->mtri;   						// one specific meter
			objTx = isExport ? mtr1->name : strtprintf("meter \"%s\"", mtr1->name);
			break;
		case TI_ALL:
			objTx = "All Meters";
			hd1 = "Meter";
			break;			// 1st col is object name not time
		case TI_SUM:
			objTx = "Sum of Meters";
			break;
		}
		xhd1 = "Meter";					// export 1st col always name (and 4 time cols always present)
	}
	else if (dvrip->dv_dhwMtri)			// if DHWMTR-based report
	{
		switch (dvrip->dv_dhwMtri)
		{
			DHWMTR* pWM;
		default:
			pWM = WMtrR.GetAt(dvrip->dv_dhwMtri);   	// specific DHW meter
			objTx = isExport ? pWM->name : strtprintf("DHW meter \"%s\"", pWM->name);
			break;
		case TI_ALL:
			objTx = "All DHW Meters";
			hd1 = "DHW Meter";
			break;			// 1st col is object name not time
		}
		xhd1 = "DHW Meter";					// export 1st col always name (and 4 time cols always present)
	}
	else if (dvrip->dv_afMtri)
	{	switch (dvrip->dv_afMtri)
		{
			AFMTR *mtr1;
		default:
			mtr1 = AfMtrR.p + dvrip->dv_afMtri;   // one specific afmeter
			objTx = isExport ? mtr1->name : strtprintf("AFMETER \"%s\"", mtr1->name);
			break;
		case TI_ALL:
			objTx = "All AFMETERs";
			hd1 = "Air Flow Meter";
			break;			// 1st col is object name not time
		case TI_SUM:
			objTx = "Sum of AFMETERs";
			break;
		}
		xhd1 = "Air Flow Meter";
	}
	else if (dvrip->ahi)			// if airHandler-based report (AH)
	{
		switch (dvrip->ahi)
		{
			AHRES *ahr1;							// (AH and AHRES names & subscripts match)
		default:
			ahr1 = AhresB.p + dvrip->ahi;      					// one specific air handler
			objTx = isExport ? ahr1->name : strtprintf("air handler \"%s\"", ahr1->name);
			break;
		case TI_ALL:
			objTx = "All Air Handlers";
			hd1 = "Air Handler";
			break;   		// 1st col is object name not time
		case TI_SUM:
			objTx = "Sum of Air Handlers";
			break;
		}
		xhd1 = "Air Handler"; 				// export 1st col always name (and 4 time cols always present)
	}
	else if (dvrip->tui)			// if terminal-based report, 6-95
	{
		switch (dvrip->tui)
		{
			TU *tu1;
		default:
			tu1 = TuB.p + dvrip->tui;      						// one specific terminal
			objTx = isExport ? tu1->name
			: strtprintf( "terminal \"%s\" of zone \"%s\"",
			tu1->name, ZrB.p[tu1->ownTi].name );
			break;
		case TI_ALL:
			objTx = "All Terminals";
			hd1 = "Terminal";
			break;   		// 1st col is object name not time
			//case TI_SUM:   objTx = "Sum of Terminals";  break;
		}
		xhd1 = "Terminal"; 				// export 1st col always name (and 4 time cols always present)
	}

// get 'what' text for report titles / handle exceptions by report type

	switch (rpTy)
	{
	case C_RPTYCH_ZEB:
		what = "Energy Balance";
		break;  		// insert for title below

	case C_RPTYCH_ZST:
		what = "Statistics";
		break;		// insert for title

	case C_RPTYCH_MTR:
		what = "Energy Use";
		break;   		// "Meter Use"? "Metered Energy Use"?

	case C_RPTYCH_DHWMTR:
		what = "Hot Water Use";
		break;

	case C_RPTYCH_AFMTR:
		what = "Air Flow (avg CFM)";
		break;

	case C_RPTYCH_AH:
		what = "Air Handler Report";
		break;      	// insert for title

	case C_RPTYCH_AHSIZE:
		what = "Air Handler Sizing";
		break;      	// insert for title, 6-95
	case C_RPTYCH_AHLOAD:
		what = "Air Handler Load";
		break;      	// insert for title, 6-95

	case C_RPTYCH_TUSIZE:
		what = "Terminal Sizing";
		break;      	// insert for title, 6-95
	case C_RPTYCH_TULOAD:
		what = "Terminal Load";
		break;      	// insert for title, 6-95

	case C_RPTYCH_UDT:
		what = dvrip->rpTitle			// insert for title for user-defined report:
			?  dvrip->rpTitle					// use user's UDT title if given,
			:  strtcat(							// else use...
				dvrip->name[0] ? dvrip->name : "User-defined",	//    report name else "User-defined"
				isExport ? NULL : " Report",   	//    followed by " Report" if report
				NULL );
		break;

	default: ;				// caller assumed to have messaged bad type
	} // switch (rpType)

// do export header

	if (isExport)
	{
		if (dvrip->rpHeader == C_RPTHDCH_YES) {
			vrPrintf(vrh, "\"%s\",%03d\n"				// format 3 header lines at start of export
				"\"%s\"\n"
				"\"%s\",\"%s\"\n",
				Top.runTitle,    		//   user-input run title, quoted (defaulted to "")
				Top.runSerial,   		//      run serial # on same line
				Top.runDateTime,   		//   date & time string, cnguts.cpp, quoted
				what,					//   object type "Energy Balance", "User-defined", etc
				ivlTx);				//      interval "Month" etc, on same line
		}
		if (rpTy==C_RPTYCH_UDT)
			vpUdtExColHeads(dvrip);		// virtual print user-defined export col headings, below
		else					// ZEB, ZST, MTR, AH.
		{	char buf[ MAXRPTLINELENSTD];
			fmtExColhd(     			// format export column headings, below
				rxt->colDef,   		//   COLDEF struct gives col wids, hdng
				buf,   			//   output buffer
				rxt->flags,		//   options per caller
				xhd1 );			//   additional argument: text for first column head ("Zone" etc)
			vrStr( vrh, buf);			// virtual print export col headings
		}
	}

// do ZEB / ZST / MTR / AH / AH/TU SIZE/LOAD / UDT type report title, and column heads per colDef or UDT info.

	else			// is report
	{
		// format blank lines and title to virtual report

		if (rpTy==C_RPTYCH_UDT  &&  dvrip->rpTitle)	// for user-defined report with title given
			vrPrintf( vrh, "\n\n%s%s%s\n\n",			// don't show frequency or zone.
				what,					//  user-given title
				*when ? " for " : "",  when );		//  day or month name
		else						// for ZEB ZST MTR AH, and UDT with no title given
			vrPrintf( vrh, "\n\n%s%s%s%s%s%s%s%s%s\n\n",
				dvrip->rpCondGiven ? "Conditional " : "",
				fqTx,						// "Monthly " etc with trailing space, or ""
				what,						// spaceless
				*objTx ? ", " : "",  objTx,
				*when  ? ", " : "",  when,
				hour ? strtprintf(" hour %d", (INT)hour) : "",				// add separating comma?
				subHour >= 0 ? strtprintf(" subhour %c", 'a'+subHour) : "" );		// comma?

		// do report column headings per colDef and caller's flags.

		if (rpTy==C_RPTYCH_UDT)					// UDT
			vpUdtRpColHeads(dvrip);   				//   user-defined col headings, below
		else							// ZEB ZST MTR AH
		{	char buf[MAXRPTLINELENSTD];
			fmtRpColhd( rxt->colDef, buf, rxt->flags, hd1);  	//   format col heads to buf, local fcn, below
			vrStr( vrh, buf);					//   write buffer to virtual report, vrpak.h
		}
	}
	// more returns above
}				// vpRxHeader
//==========================================================================================================
LOCAL void FC NEAR vpRxFooter( 		// do report/export footer and/or blank line per report/export type and frequency

	DVRI *dvrip )			// date-dependent virtual report info record for report/export
{
#define SUMMARIZEALL	// summary line for all most non-annual ZEB/ZST reports; undefine for full-year monthly only

	IVLCH rpFreq = dvrip->rpFreq;
	RXPORTINFO rxt;				// argument struct for vpRxRow, only a few members set here
	memset( &rxt, 0, sizeof(rxt));		// default all other members to 0
	rxt.flags = 2					// column control flags: 2: report (4 for export)
		| (dvrip->isAll ? 16 : 8);	//  16 show name column, 8 show time column

#ifdef DEFFOOT	// if VR_NEEDFOOT in use
o	vrChangeOptn( rxt.vrh, VR_NEEDFOOT, 0); 	// clear report's "footer request pending" flag
#endif
	if (dvrip->rpFooter==C_NOYESCH_NO)		// if footer suppressed for this report/export by input command
		return;					// done
	if (dvrip->isExport)			// currently (12-91) no footers for any export type
		return;					// ( ... just final blank line, done by VR_FINEWL option)
	if (vrIsEmpty(dvrip->vrh))			// if nothing yet output to this virtual report
		return;					// no footer: none b4 start report, none at all for null conditional report
	// (should ZEB/ZST year summary line show anyway? if so, needs a header!)

#ifdef SUMMARIZEALL		// try more summaries 1-92
// most ZEB, ZST, MTR, and AH reports get summary line showing next longer interval

	if ( !dvrip->isAll				// not all-zones, all-meters, etc
	 &&  !dvrip->isExport)			// change insurance here
#else
o// full-year monthly ZEB and ZST reports get year results summary line
o
o	if ( !dvrip->isAll						// not all-zones, all-meters, etc
o    &&  !dvrip->isExport
o    &&  rpFreq==C_IVLCH_M
o    &&  dvrip->rpDayBeg <= Top.tp_begDay		<--- new year's bug! 2-94
o    &&  dvrip->rpDayEnd >= Top.tp_endDay )	<--- new year's bug! 2-94
#endif
	{
		switch (dvrip->rpTy)					// print summary row
		{
		case C_RPTYCH_ZEB:
		case C_RPTYCH_ZST:
		case C_RPTYCH_MTR:
		case C_RPTYCH_DHWMTR:
		case C_RPTYCH_AFMTR:
		case C_RPTYCH_AH:

			/* omit total line if report ends b4 month or year it shows is complete: partial results wd be misleading.
			       ?? also omit if report STARTED before interval it shows?  No, line is labeled "Yr" or "Mon".
			       omit total line if control does not get here when report ends (possible?) -- might show wrong data. */
		{
			//(vbl scope)
			// adjust dates to be compared for runs which cross into next year, ie end on day of year < start (2-94):
			DOY jDay = Top.jDay;
			if (jDay < Top.tp_begDay)      jDay += 365;
			DOY endDay = Top.tp_endDay;
			if (endDay < Top.tp_begDay)    endDay += 365;
			DOY rpDayEnd = dvrip->rpDayEnd;
			if (rpDayEnd < Top.tp_begDay)  rpDayEnd += 365;

			if (rpFreq==C_IVLCH_D)
			{
				if (!Top.isEndMonth  ||  jDay > rpDayEnd  /* || rpDayBeg is after start of month ??*/ )
					break;
			}
			else if (rpFreq==C_IVLCH_M)
			{
				if (rpDayEnd < endDay  /* || rpDayBeg > Top.tp_begDay ?? */ )
					break;
			}
			else if (rpFreq==C_IVLCH_Y)   			// don't do for year -- no longer interval to show
				break;
		}

		// output report row for next longer interval (or day for subhr) for same zone (or sum-of-zones)

		vrStr( dvrip->vrh, "\n");   					// blank separating line
		rxt.fq = (rpFreq < C_IVLCH_H) ? rpFreq - 1		// daily gets month, monthly gets year. local rxt.
									  : C_IVLCH_D;		// subhrly (shows whole day) or hrly gets day, 6-92
		if (dvrip->rpTy==C_RPTYCH_MTR)		// if Meter report
		{
			TI mtri = dvrip->mtri > 0 ? dvrip->mtri : MtrB.n;    		// subscript for meter or sum-of-all-meters
			MTR_IVL_SUB *mtrs = &MtrB.p[mtri].Y + (rxt.fq - 1); 		// .M is after .Y, etc
			rxt.colDef = mtrColdef;					 	// columns definition table for vpRxRow
			vpRxRow( dvrip, &rxt, mtrs,  shortIvlTexts[rxt.fq]);		// do MTR rpt row.  Uses rxt.flags, colDef.
		}

		else if (dvrip->rpTy==C_RPTYCH_DHWMTR)		// if DHW meter report
		{
			TI dhwMtri = dvrip->dv_dhwMtri;    		// subscript
			DHWMTR_IVL* pIvl = &WMtrR[ dhwMtri].curr.Y + (rxt.fq - 1); 	// .M is after .Y, etc
			rxt.colDef = wMtrColdef;					 			// columns definition table for vpRxRow
			vpRxRow( dvrip, &rxt, pIvl, shortIvlTexts[rxt.fq]);	// do DHWMTR rpt row.  Uses rxt.flags, colDef.
		}

		else if (dvrip->rpTy == C_RPTYCH_AFMTR)		// if AFMETER report
		{
			TI afMtri = dvrip->dv_afMtri > 0 ? dvrip->dv_afMtri : AfMtrR.n;   		// subscript
			AFMTR_IVL* pIvl = AfMtrR[afMtri].amt_GetAFMTR_IVL( rxt.fq);
			rxt.colDef = afMtrColdef;					 			// columns definition table for vpRxRow
			vpRxRow(dvrip, &rxt, pIvl, shortIvlTexts[rxt.fq]);	// do AFMTR rpt row.  Uses rxt.flags, colDef.
		}

		else if (dvrip->rpTy==C_RPTYCH_AH)   	// if Air handler report
		{
			TI ahi = dvrip->ahi > 0 ? dvrip->ahi : AhresB.n;  		// subscript for meter or sum-of-all-meters
			AHRES_IVL_SUB *ahrs = &AhresB.p[ahi].Y + (rxt.fq - 1);    	// .M is after .Y, etc
			rxt.colDef = ahColdef;					 	// columns definition table for vpRxRow
			vpRxRow( dvrip, &rxt, ahrs,  shortIvlTexts[rxt.fq]);		// do AH rpt row.  Uses rxt.flags, colDef.
		}
		else					// ZEB, ZST
		{
			TI resi = dvrip->ownTi > 0 ? dvrip->ownTi : ZnresB.n;		// results subscript for zone or sum-of-all-zones
			ZNRES_IVL_SUB *res = &ZnresB.p[resi].curr.Y + (rxt.fq - 1);  	// .M is after .Y, etc; no _S or _HS gets here
			rxt.colDef = dvrip->rpTy==C_RPTYCH_ZST ? stColdef : ebColDef; 	// columns definition table for vpRxRow
			if (rpFreq >= C_IVLCH_S)   					// if for _HS or _S report, include (blank) ..
				rxt.flags |= (64|32);   					// .. * and M columns for alignment
			vpRxRow( dvrip, &rxt, res,  				// do ZEB/ZST rpt row.  Uses rxt.flags, colDef.
				shortIvlTexts[rxt.fq], "", "","","","",	// time. for ebColDef, no name, no export times,
				"","","","" );							// .. blank mode and shutter columns.
		}
		break;

		default:
			;
		}
	}

// most all- reports get sum row every time

	if ( dvrip->isAll
			&&  !dvrip->isExport)
	{
		switch (dvrip->rpTy)
		{
		case C_RPTYCH_ZEB:
		case C_RPTYCH_ZST:

			if (rpFreq <= C_IVLCH_H)				// zones have no sum-of-zones results for subhour
				// (for HS cd do sum on H reports except don't know H from S here)
			{
				ZNRES_IVL_SUB *res = &ZnresB.p[ZnresB.n].curr.Y + rpFreq - 1;	// pt zn sum results (last record) for same ivl
				rxt.colDef = dvrip->rpTy==C_RPTYCH_ZST ? stColdef 		// columns definition table for vpRxRow
							 : ebColDef;
				if (rpFreq >= C_IVLCH_S)   				// insurance: H, HS, S currently 1-92 don't get here
					rxt.flags |= (64|32);   				// .. include * and M columns so columns align
				vrStr( dvrip->vrh, "\n");   				// blank separating line
				vpRxRow( dvrip, &rxt, res,  				// do ZEB/ZST rpt row.  Uses rxt.flags, colDef.
						 "", "Sum", "","","","", 			// no time, name "Sum". for ebColDef, no export times,
						 "","","","" );					// .. blank mode and shutter columns.
			}
			break;

		case C_RPTYCH_MTR:
		{
			MTR_IVL_SUB *mtrs = &MtrB.p[MtrB.n].Y + rpFreq - 1;   	// point substruct for interval in question
			rxt.colDef = mtrColdef;
			// rxt.flags set above
			vrStr( dvrip->vrh, "\n");   				// blank separating line
			vpRxRow( dvrip, &rxt, mtrs,  "", "Sum"); 		// do MTR report row, below. Uses rxt.flags, colDef.
			break;
		}
		// case C_RPTYCH_DHWMTR: no Sum
		// case C_RPTYCH_AFMTR: no Sum
		case C_RPTYCH_AH:
		{	AHRES_IVL_SUB* ahrs = &AhresB.p[AhresB.n].Y + rpFreq - 1;   	// point substruct for interval in question
			rxt.colDef = ahColdef;
			// rxt.flags set above
			vrStr( dvrip->vrh, "\n");   				// blank separating line
			vpRxRow( dvrip, &rxt, ahrs,  "", "Sum");  		// do AH report row, below. Uses rxt.flags, colDef.
			break;
		}
		default:
			;
		}
	}
}			// vpRxFooter

#ifdef WANTED
w //=================================================================
w void FC cgPrRes(   // Print an HSZNRES, for CALRES results file.
w
w     SI vrh, 		// handle of virtual report to which to "print"
w     char *heading, 	/* Heading string */
w     ZNR *zp,		// Zone recort pointer, for info used re vpRxRow options added 11-91, probably not used here.
w     HSZNRES *pzr )	/* ptr to zone results structure */
w
w /* Format printed is:
w    Line 1: <heading>
w    Line 2: energy balance values (heat flows and loads)(as for xEB reports)
w    Line 3: conditions values (hours of heating, cooling etc)(as for ZST reports) */
w
w /* NOTE ALSO existence of cgresfil.cpp, containing hst result file functions */
w
w {
w // heading
w     vrPrintf( vrh, "%s\n", heading );
w
w // EB info
w     vpRxRow(    	// format, print zone data (fcn below)
w 	      vrh, 		// print destination
w 	      ebColDef,		// "energy balance" (aka xEB reports) info columns-to-print table above
w 	//    zp,		// zone (used re options; probably unused here)
w               pzr,		// HSZNRES struct containing data
w	      1.f,		// no scaling
w 	      0,		// bits: 1 OFF: report, not export
w 	      // following args used IN ORDER at -1 .offsets in ebColDef
w 	      "", "", "" );	// blank row label, shutter fraction and mode.
w
w // ZST-like info
w     vpRxRow(
w 	      vrh, 		// print destination
w 	      stColdef,  	// "conditions" info (aka ZST report info) columns-to-print table above
w 	//    zp,		// zone (used re options; probably unused here)
w 	      pzr,		// HSZNRES struct containing data
w	      1.f,		// no scaling
w 	      0,		// bits: 1 OFF: report, not export.
w 	      // following arg(s) used at -1 .offset(s) in stColdef
w 	      "" ); 		// blank row label
w }			// cgPrRes
w --- related code for former CALRES results files ---
w HSZNRES type: gone (or renamed) (10-93)
w -- deleted from cse.cpp 10-93 (if'd out since 10-91):
w  char *resFileName = NULL;
w  XFILE *resFileXF = NULL;
w  ...
w        else if (resFileName==NULL)	// else if no results file name yet
w           resFileName = arg;   		// take this arg as results file name
w  ...
w // open results output file
w
w     if (resFileName != NULL)	/* optnl cmd line arg specs results file */
w     {  resFileXF = xfopen(		/* open file, xiopak.cpp */
w        		       resFileName,	/*   name */
w 	       O_WTUNKA,	/*   access: text, r/w append, create
w 	       			     if not found */
w 	       IGN,		/*   erOp: errors handled here */
w 	       FALSE,		/*   eofs not errors (writing) */
w 	       NULL );
w        if (resFileXF == NULL)		/* if open failed */
w           tiabort( "Unable to open results file '%s'", resFileName);
w	  }
w ...
w     after ok run...
w	  if ( resFileXF != NULL)
w 	  {  cgResWrite( resFileXF);		// write run results, hsresfil.cpp
w 	     cgMemRep( "cse cgResWrite");
w	  }
w -- deleted from cse.h 10-93. cgresfil.cpp and cgResWrite are GONE, 10-93.
w // cgresfil.cpp
w void FC cgResWrite( XFILE *xfRf);
#endif  // WANTED

//==========================================================================================================
LOCAL void FC NEAR vpEbStRow( 			// virtual print zone ZEB or ZST row for zone or sum of zones

	DVRI *dvrip,	// date-dependent virtual report info record set up before run by cncult4.cpp
	RXPORTINFO *rxt,	// much addl report info, set up in vpRxports
	TI zi )		// TI_SUM or zone subscript (caller iterates for TI_ALL)

{
	TI resi = (zi==TI_SUM ? ZnresB.n : zi);   			// results record subscr: sum of zones is in last record
	SI resSubi = rxt->fqr - 1;	// results substr offset from .Y: M, D, H, S follow .Y, no HS.
    							//   .fqr is H or S when .fq is HS for hourly+subhourly reports
	ZNRES_IVL_SUB *res = &ZnresB.p[resi].curr.Y + resSubi;   	// point results substructure for interval to be reported
	SI xMode = 0;						// for ZEB export: CSE zone mode as integer
	char *znSC = "";					// for ZEB report and export: shutter fraction, "" or "*"
	char mode[10];						// for ZEB report: CSE zone mode as text
	mode[0] = 0;

	if ( rxt->fqr==C_IVLCH_S	// show zone mode, shutter fraction nonblank only in subhour rows ...
	  && zi > 0 )				// .. not in sum-of-zones -- merging not meaningful
	{
		/* (but the columns are included for uniform format in all exports,
		   and in H rows of HS reports, Hr summary line of H, HS, or S report) */
		ZNR *zp = ZrB.p + zi;
		//if (rxt->flags & 64)				not worth bothering to test
		if (zp->i.znSC != 0.f)			// if shades shut at all in interval
			znSC = "*";					// show * in subhour if shades not fully open
		if (rxt->flags & 32)			// if zone mode is to show (else skip sprintf for speed)
		{
			xMode = zp->zn_md;					// ZEB export CSE zone mode: integer
			sprintf( mode, "%2d", zp->zn_md);	// ZEB report CSE zone mode: text
		}
	}

	vpRxRow( dvrip, rxt, res,
		rxt->col1,						// column 1 time (non-ALL reports)
		ZnresB.p[resi].name,			// name (exports, "ALL" reports)
		&rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS, 	// export time columns
		znSC, znSC,						// shades closure indicator for report, export (colHd differs)
		mode, &xMode );					// mode for report, export.  (6-95 why different?)
}				// vpEbStRow
//=================================================================
LOCAL void FC NEAR vpMtrRow( DVRI *dvrip, RXPORTINFO *rxt, TI mtri)

// virtual print MTR report/export Row for one meter or sum (caller iterates for all)
{
	MTR *mtr = MtrB.p + (mtri==TI_SUM ? MtrB.n : mtri);		// point meter record: sum is in last record.
	MTR_IVL_SUB *mtrs = (&mtr->Y) + (rxt->fq - 1);		/* point substruct for interval in question.
    								   Subhr not allowed for MTR --> fq==fqr; no extra -1 needed. */

	vpRxRow( dvrip, rxt, mtrs,  rxt->col1, mtr->name,  &rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS);
}		// vpMtrRow
//-----------------------------------------------------------------------------
void DVRI::dv_vpDHWMtrRow( RXPORTINFO *rxt, TI dhwMtri /*=-1*/)

// virtual print DHWMTR report/export Row for one meter (caller iterates for all)
{
	if (dhwMtri < 0)
		dhwMtri = dv_dhwMtri;
	DHWMTR* pWM = WMtrR.GetAt( dhwMtri);		// record
	DHWMTR_IVL* pIvl = (&pWM->curr.Y) + (rxt->fq - 1);	// interval
    												// subhr not allowed for DHWMTR --> fq==fqr; no extra -1 needed.

	vpRxRow( this, rxt, pIvl,  rxt->col1, pWM->name,  &rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS);
}		// dv_vpDHWMtrRow
//-----------------------------------------------------------------------------
void DVRI::dv_vpAfMtrRow(RXPORTINFO *rxt, TI afMtri /*=-1*/)

// virtual print AFMTR report/export Row for one meter (caller iterates for all)
{
	if (afMtri < 0)
		afMtri = dv_afMtri;
	if (afMtri == TI_SUM)
		afMtri = AfMtrR.n;					// handle "sum"
	AFMTR* pM = AfMtrR.GetAt(afMtri);		// record
	AFMTR_IVL* pIvl = pM->amt_GetAFMTR_IVL( rxt->fqr);	// interval
								// results substr offset from .Y: M, D, H, S follow .Y, no HS.
								//  .fqr is H or S when .fq is HS for hourly+subhourly reports

	vpRxRow(this, rxt, pIvl, rxt->col1, pM->name, &rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS);
}		// dv_vpDHWMtrRow
//=================================================================
LOCAL void FC NEAR vpAhRow( DVRI *dvrip, RXPORTINFO *rxt, TI ahi)

// virtual print AH report/export Row for one air handler or sum (caller iterates for all)
{
	AHRES* ahr = AhresB.p + (ahi==TI_SUM ? AhresB.n : ahi);   	// point ah results record: sum is in last record.
	SI ahrsi = rxt->fqr - 1;	// results substr offset from .Y: M, D, H, S follow .Y, no HS.
    							//  .fqr is H or S when .fq is HS for hourly+subhourly reports
	AHRES_IVL_SUB *ahrs = (&ahr->Y) + ahrsi;			// point substruct for interval in question

	vpRxRow( dvrip, rxt, ahrs,  rxt->col1, ahr->name,  &rxt->xebM, &rxt->xebD, &rxt->xebH, rxt->xebS);
}		// vpAhRow
//=================================================================
LOCAL void FC NEAR vpAhSzLdRow( DVRI *dvrip, RXPORTINFO *rxt, TI ahi)

// virtual print year AHSIZE or AHLOAD report/export row for one air handler (caller iterates for all). 6-95.
{
	AH *ah = AhB.p + ahi;   			// point ah record
	vpRxRow( dvrip, rxt, ah,
		ah->name,						// name, used in exports or all- reports. No report time column.
	// None/AutoSized/Input value indicators:
		ah->fanAs.az_active ? "a" : "i",						// fan(s) (always present)
		ah->ahhc.coilTy==C_COILTYCH_NONE ? "" : ah->hcAs.az_active ? "a" : "i",		// .. heat coil
		ah->ahcc.coilTy==C_COILTYCH_NONE ? "" : ah->ccAs.az_active ? "a" : "i",		// .. cool coil, twice for capt and caps.
		ah->ahcc.coilTy==C_COILTYCH_NONE ? "" : ah->ccAs.az_active ? "a" : "i" );	// ..
}						// vpAhSzLdRow
//=================================================================
LOCAL void FC NEAR vpTuSzLdRow( DVRI *dvrip, RXPORTINFO *rxt, TI tui)

// virtual print year TULOAD report/export Row for one terminal (caller iterates for all). 6-95.
{
	TU *tu = TuB.p + tui;   					// point terminal record
	vpRxRow( dvrip, rxt, tu,
		tu->name,						// name, used in exports or all- reports. No report time column.
		ZrB.p[tu->ownTi].name,					// terminal's zone's name
		tu->cmLh & cmStH ? tu->hcAs.az_active ? "a" : "i" : "",		// AutoSized/Input/None indicator for local heat coil
		tu->cmAr & cmStH ? tu->vhAs.az_active ? "a" : "i" : "",		// .. heat cfm (tuVfMxH)  (set output shows n, and value)
		tu->cmAr & cmStC ? tu->vcAs.az_active ? "a" : "i" : "" );	// .. cool cfm (tuVfMxC)
}						// vpTuSzLdRow

//=================================================================
LOCAL void CDEC NEAR vpRxRow(	// virtual print report or export row given COLDEF table and record

// COLDEF table specifies members to print (ZEB, ZST, MTR, etc); has option for report or export format.

	DVRI *dvrip,	// date-dependent virtual report info record set up before run by cncult4.cpp
	RXPORTINFO *rxt,	// report/export info struct set in vpRxports. members used: flags, colDef.
	void *zr,		// Values to print (ZNRES_IVL_SUB *, MTR_IVL_SUB *, etc)
	... )		// 0 or more POINTERS TO data to use, IN ORDER, when colDef.offset is -1. 3-91.

// fmtRpColhd may be used 1st to format column headings for COLDEF.
{
	va_list ap;   COLDEF *colDef;   float ftemp = 0;   USI mfw, fmt;   char temp[1000];   void *p = nullptr;

	SI isExport = dvrip->isExport;

	va_start( ap, zr);

// columns loop

	char *s = temp;
	for (colDef = rxt->colDef;  colDef->colhd;  colDef++)	// CAUTION: ->colhd may be NEAR, don't use ==NULL!
	{
		// fetch data if in arg list now, so list is independent of conditional fields, 6-95.
		if (colDef->offset > 65529U)    			// if -1...-5 stored in USI (USE_NEXT_ARG = -1U = 65535)
			p = va_arg( ap, void *);			// use next argument ptr from vbl arg list (CAUTION: limited scaling etc)
		// conditionally skip field
		if (SKIPFLAGS & colDef->flags & ~rxt->flags)	// if col has skip bit that is not on in caller's (rxt) flags
			continue;					// OMIT this column completely
		if (colDef->cvflag==NOCV)			// if nonprinting dummy (eg for db for following CVWB, when db not printed)
			continue;					// OMIT this column completely

		// column spacing or separating
		if (s != temp)				// unless first field on line. Moved to apply to reports too, 6-95.
			if (isExport)				// if exporting (spreadsheet format)
				*s++ = ',';   			// comma after prior datum
			else 					// not export
				if ( !(colDef->flags & 1) )    	// unless colDef option bit on
					*s++ = ' ';   			// skip a space b4 each col

		// determine data format
		if (isExport)				// improve export format 1-92
		{
			mfw = max( colDef->width, 13);	// width at least 13 for export (-1.23456e+123) (ignore table?)
			fmt = FMTSQ | FMTOVFE | FMTRTZ   	// format: squeeze, overflow to E format, # sigdig not #dec places given.
			+ max( colDef->dfw, 6);   	// ... # sig digits (with FMTRTZ): min 6: export full precision. (ignore table?)
		}
		else					// report
		{
			mfw = colDef->width;				// overall column width from table
			fmt = (colDef->flags & 128 ? FMTLJ : FMTRJ)	// format: left- or right-justify in field width
			+ colDef->dfw;				// ... # decimal places from column definition table
		}
		if (isExport)  if (colDef->cvflag==CVS)  *s++ = '"';	// output leading " for exported strings

#if BLANKFLAGS  // omit code if no such bits
		// conditionally blank omitted (report) column
		if (BLANKFLAGS & colDef->flags & ~rxt->flags)	// if col has blank bit that is not on in caller's (rxt) flags
		{
			cvin2sBuf( s,  						// use data formatter to blank needed width, cvpak.cpp
			isExport && colDef->cvflag != CVS ? "0" : "",	// data: for numeric export use "0", else blank
			DTCH, UNNONE, mfw, fmt );				// data type; no units; field (max) width and format
		}
		else
#endif
		{
			// point data
			if (colDef->offset <= 65529U)    		// if data NOT in arg list (if NOT -1...-5 in USI)
			{
				// (for arg list data, p set above b4 cond'l field skips)
				p = (char *)zr + colDef->offset;		// point data in record
				switch (colDef->cvflag)
				{
					//case NOCV:	done at top of loop.	nonPrinting dummy eg to hold ptr to unprinted drybulb for CVWB.
				default:
					break;				// CVI / other: p preset

				case CVK:
				case CVM:
				case CV1:
				case BTU:
				case NBTU:	// data is float, with various scalings or formattings
					ftemp = *(float *)p;  				// fetch data, so we can scale it
					p = &ftemp;  					// point to where data now is
					break;
				case CVWB:   				// wetbulb temp for drybulb per PREVIOUS ENTRY & hum rat per this entry
					ftemp = *(float *)((char *)zr + (colDef-1)->offset);	// fetch data of preceding table entry: drybulb
					ftemp =
					ftemp==0.f && (*(float *)p)==0.f	// if drybulb and hum rat (w) both exactly 0
						?  0.f 						// then no data has been stored, show 0 rather than -3.x.
						:  ftemp >= PSYCHROMINT && ftemp <= PSYCHROMAXT	// check for temp in psyTWetBulb's range 4,5-95
							?  psyTWetBulb( ftemp, *(float *)p)	/* wetbulb for drybulb & w, lib\psychro2.cpp. looks slow:
                   												   faster to accum & average t,w, convert only now. */
						:  0.f;					// out of range: fall thru and print 0
												// (would prefer 9's or *'s, but wd require addl code). */
					p = &ftemp;  					// point to where data now is
					break;
				}
			}

			// data scaling / formatting
			switch (colDef->cvflag)
			{
			case CVK:
				ftemp /= 1000.f;
				break;    		// print float/1000.
			case CVM:
				ftemp /= 1000000.;
				break;		// print float/1000000.
			case BTU:
				if (dvrip->rpBtuSf)
					ftemp /= dvrip->rpBtuSf;		// print float/rpBtuSf -- scaled Btu's (unless 0 -- insurance)
				break;
			case NBTU:
				if (dvrip->rpBtuSf)
					ftemp /= -dvrip->rpBtuSf;		// print float/rpBtuSf -- negative scaled Btu's
				break;
			default:
				;
			}

#ifdef ZFILTER	// rob 6-92
			// change values much too small to show significant digits to exact 0, to reduce appearance of "-.000"s, etc, in reports.
			//   cuz a) they're ugly and distracting b) frustrate testing for similar results by comparing report files. rob 6-92.
			//   smallest value that can possibly show significance in n columns is 5e-n, eg:  n  min print  min value  in e format
			// 										 1      1          .5       5e-1
			// 										 2      .1         .05      5e-2
			// 										 3      .01        .005     5e-2  QED.
			switch (colDef->cvflag)
			{
			case CVK:
			case CVM:
			case CV1:
			case BTU:
			case NBTU: 	// data is float, with various scalings already applied
				static float f0 = 0.f;
				USI nPow = min( USI(mfw + ZFILTER), (USI)NPTENSIZE);	// n for test: fld wid + ZFILTER cols, not > NPten[] size.
				if (fabs(*(float *)p) < 5.*NPten[nPow])			// if significance wouldn't show in n columns
					p = &f0;						// print a true zero instead of value
				break;
			}
#endif

			// data conversion
			cvin2sBuf( s, p,    			// format value to buffer, cvpak.cpp.  ptrs to buffer, data.
				dTypes[colDef->cvflag],     // data type (array at start file)
				UNNONE, mfw, fmt );			// no units output; field (max) width and format, above
		}

		// finish, point next output column
		s += strlen(s);				// same as += colDef->width for report format
		if (isExport)				// if export format
		{
			while (s > temp && *(s-1)==' ')  	//  trim trailing blanks
				s--;  				//  (for strings)
			if (colDef->cvflag==CVS)   		// exported strings get
				*s++ = '"';   			// trailing quote
		}
	}   		// column loop end

// write line

	strcpy( s, "\n");   			// nl and null on end
	vrStr( dvrip->vrh, temp);   		// write buffer
}				// vpRxRow
//==================================================================
LOCAL void FC NEAR vpUdtRpColHeads( DVRI *dvrip)		// user-defined report column heads
{
	COL *colp /*=NULL*/;
	SI i, colWid, acWid, sTween, sLeft = 0;
	char *p, *q;
	const char *text;
	JUSTCH jus;
	USI dt;

// determine column spacing: cram or space out per extra width available.  Must match in vpUdtRpRow and vpUdtRpColHeads.

	sTween = 0;							// number of extra spaces between columns: start with 0
	if (dvrip->wid + dvrip->nCol <= dvrip->rpCpl)					// note dvrip->wid includes colGaps.
	{
		sTween = 1;									// 1 if fits
		sLeft = (dvrip->rpCpl - dvrip->wid - sTween * dvrip->nCol)/(dvrip->nCol + 2);	// distribute remaining excess
		sLeft = max( 0, min( sLeft, 5));							// in range of 0 to 5 per column
		sTween += sLeft;
	}

// output row of column head texts
	char buf[MAXRPTLINELENUDT];
	char* s = buf;				// init store pointer into line buffer
	q = s + sLeft;				// where gap b4 1st col should begin: spaces stored after adj for overlong text
	for (i = dvrip->coli;  i;  )			// loop over columns of table
	{
		colp = RcolB.p + i;
		i = colp->nxColi;				// advance i now for ez lookahead
		colWid = colp->colWid;
		p = q + colp->colGap;				// nominal start of column, to adjust for overflow / justification
		q = p + colWid + sTween;				// nominal start of next column's gap, if there is another col
		//s is next avail position in buffer = min value for p
		dt = colp->colVal.ty;					// data type, DTFLOAT or DTCHP, from VALNDT struct member
		jus = colp->colJust;					// justification
		if (!jus)
			jus = dt==DTFLOAT ? C_JUSTCH_R : C_JUSTCH_L;		// default right-justified for numbers, left for strings

		// get text to display, adjust position
		text = colp->colHead ? colp->colHead		// use colHead member if nonNULL
							 : colp->name;   		// else use reportCol record name
		acWid = (SI)strlen(text);
		if (acWid > colWid)				// if overwide
			p -= min( (SI)(acWid - colWid), (SI)(p - s));	// move left into any available space between columns
		// truncate if acWid overlong?
		else if (jus==C_JUSTCH_R)			// not overwide; if right-justified
			p += colWid - acWid;				// right justify

		// now store leading blanks, text, 1 trailing blank only
		while (s < p)  *s++ = ' ';			// rest of blanks before this column
		memcpyPass( (void **)&s, text, acWid);		// store text / point past
		if (i)						// separator char, if not last column (i is colp->nxColi):
			if (s < p + colWid || RcolB.p[i].colGap)	// not if this col full and next col has 0 gap
				*s++ = ' ';   				//  one space now, rest when size next col's datum known

	}  // for (i=...  columns loop
	strcpy( s, "\n");   			// nl and null on end of line buffer
	vrStr( dvrip->vrh, buf);			// output to virtual report

// output row of ----------- underlines for col headings

	s = buf;						// reinit for next line
	memsetPass( (void **)&s, ' ', sLeft);		// spaces left of 1st column
	for (i = dvrip->coli;  i;  i = colp->nxColi)	// loop columns of table
	{
		colp = RcolB.p + i;
		memsetPass( (void **)&s, ' ', colp->colGap); 	// column's gap spaces before column text
		memsetPass( (void **)&s, '-', colp->colWid);	// ---- to column width
		if (colp->nxColi)				// if not last column
			memsetPass( (void **)&s, ' ', sTween);  	// sTween extra spaces between columns
	}
	strcpy( s, "\n");   				// nl and null on end of line buffer
	vrStr( dvrip->vrh, buf);				// output to virtual report
}				// vpUdtRpColHeads
//==================================================================
LOCAL void FC NEAR vpUdtExColHeads( DVRI *dvrip)		// user-defined export column heads
{
	COL* colp = NULL;

// output row of quoted, comma-separated column head texts
	char buf[MAXRPTLINELENUDT];
	char* s = buf;					// init store pointer into line buffer
	for (int i = dvrip->coli;  i;  i = colp->nxColi)	// loop over columns of table
	{
		colp = XcolB.p + i;
		const char* text = colp->colHead
				? colp->colHead		// use colHead member if nonNULL
				: colp->name;  		// else use reportCol record name

		// store text in quotes, separating comma
		*s++ = '"';
		memcpyPass( (void **)&s, text, (USI)strlen(text));	// store text / point past
		*s++ = '"';
		if (colp->nxColi)				// separator char, if not last column:
			*s++ = ',';		   			// export: separate items with comma

	}  // for (i=...  columns loop
	strcpy( s, "\n");   			// nl and null on end of line buffer
	vrStr( dvrip->vrh, buf);			// output to virtual report
}				// vpUdtExColHeads
//==================================================================
LOCAL void FC NEAR vpUdtRpRow( DVRI *dvrip)		// virtual print current interval row for user-defined report
{
// determine column spacing: cram or space out per extra width available.  Must match in vpUdtRpRow and vpUdtRpColHeads.

	int sTween = 0;			// number of extra spaces between columns: start with 0
	int sLeft = 0;
	if (dvrip->wid + dvrip->nCol <= dvrip->rpCpl)					// note dvrip->wid includes colGaps.
	{
		sTween = 1;									// 1 if fits
		sLeft = (dvrip->rpCpl - dvrip->wid - sTween * dvrip->nCol)/(dvrip->nCol + 2);	// distribute remaining excess
		sLeft = max( 0, min( sLeft, 5));							// in range of 0 to 5 per column
		sTween += sLeft;
	}

// output row of data
	char buf[MAXRPTLINELENUDT];	// size must exceed longest possible line
	char* s = buf;  			// set pointer for storing into line buffer
	char* q = s + sLeft;					// where gap b4 1st col begins. spaces stored after checking data len.
	for (int i = dvrip->coli;  i;  )			// loop over columns of table
	{
		COL* colp = RcolB.p + i;
		i = colp->nxColi;				// step i to next col subscript now, for convenience in lookahead
		int colWid = colp->colWid;				// column width, defaulted in cncult.cpp
		char* p = q + colp->colGap;				// nominal start of column, b4 adj for overflow / justification
		q = p + colWid + sTween;    			// nominal start next column's intercolumn gap
		// s is next avail position in buffer. 1 space has been stored after prior column.
		int spB4 = p > s ? (p - s) : 0;		// overflow space to left, reflecting actual prior data length + gap
		char* end = p + colWid;				// nominal end of this column
		char* limit = i  ?  q + RcolB.p[i].colGap - 1  	// desirable last char posn to write: start next column less one
				         :  buf + dvrip->rpCpl;			// no next col: else end of nominal line width
		int spAf = limit > end ? (limit - end) : 0;  	// overflow space to right, to next column or end line width

		// format data

		int dt = colp->colVal.ty;				// data type, DTFLOAT or DTCHP, from VALNDT struct member
		JUSTCH  jus = colp->colJust;			// justification
		if (!jus)
			jus = dt==DTFLOAT ? C_JUSTCH_R : C_JUSTCH_L;	// default right-justified for numbers, left for strings
		SI dec = colp->colDec;		// decimals, defaulted to -1 if not given
		int ptAdj = 0;				// rob's decimal point position adjustment
		char* text = NULL;
		USI cvFmt;
		float fv;
		if (ISNANDLE(colp->colVal.val))	// if UNSET (bug)(exman.h) or expression not evaluated yet, show "?".
			text = "?";					// show "?".  Issue message?  treat UNSET differently?
		/* uneval'd exprs may be able to occur at start run if rpFreq > expr's evf.
						   Only place in language where run with excess evf intentionally permitted, 12-91.
						   Note generally don't see ?'s for evf >= daily as evaluated during warmup. */
		else
			switch (dt)
			{
			case DTFLOAT:
				fv = *(float*)&colp->colVal.val;   	// fetch value, casting with no actual conversion
				if (dec >= 0)				// cvpak format word: if decimals given, use them (4 bits avail).
					cvFmt = FMTSQ | min( dec, 15);   	// use "squeeze" format (we justify below), and 'dec' digits after point.
				else					// decimals not given, specify # sig digits rather than point posn.
				{
#if 1		   // expect this code to be controversial, 11-30-91; remove if unpopular.
					// show more digits for larger numbers, so there is some tendancy for points to align.
					// assumes that numbers in application will tend to be > 1 and small ones of less interest.
					if (fv < 1.f)        dec = 3;   	// show 3 sig dig if all are after point. below .1, point moves left.
					else if (fv < 10.f)  dec = 4;   	// show 4 if 1 digit is left of point
					else if (fv < 100.f) dec = 5;   	// show 5 if 2 digits are left of point
					else					// 6 for larger numbers.  over 999.999 point drifts right.
#endif
						dec = 6;				// 6 significant digits.  Note: 7++ digits betray internal truncation.
					cvFmt = FMTSQ | FMTRTZ | dec;    	// squeeze, do 'dec' SIGNIFICANT digits, remove trailing 0's.
				}
				text = cvin2s( &fv, dt, UNNONE, colWid, cvFmt, spB4 + spAf); 	// convert, to Tmpstr, cvpak.cpp

#if 1		// this may be controversial too, 12-11-91
			// conditionally adjust default format if space permits for 3 character positions right of point position
				if ( colp->colDec < 0					// only if decimals not specified by user (dflt -1)
						&&  colp->colJust <= 0					// only if justification not specified (dflt 0)
						&&  strcspn( text, "Ee") >= strlen(text) )		// not if e format
				{
					SI dapp = (SI)(strlen(text) - strcspn(text,"."));		// chars at/after . (1 if ., +1 for each char after)
					// if no ., may here want to ++ dapp for trailing nonDigits eg k format chars
					SI wanDapp = min( 4, colWid-5);		// desired #chars at/after .: 4, but keep 5 posns left of .,
					ptAdj = max( 0, wanDapp - dapp);		// if < desired chars at/after . , set adjustment to use below
				}
#endif
				break;

			case DTCHP:
				text = (char *)colp->colVal.val;
				break;  	// string: no conversion

			default:
				text = "<bad dt>";
				break;
			}
		int acWid = strlen(text);

		// adjust to justify or overhang left or right.  strings are not truncated.

		if (acWid > colWid)				// if overwide
		{
			USI over = acWid - colWid;			// amount of excessive width
			if (jus==C_JUSTCH_L)     			// if lj, 1st hang right into space, but not into nxt col
				over -= min( over, spAf);			// reduce left-move for right-hang
			p -= min( over, spB4);	   		// move left into any available space b4 column
			// any further remainder hangs out to right, possibly into next column or beyond line width.
		}
		else if (jus==C_JUSTCH_R)			// not overwide; if right-justified,
			if (colWid > acWid + ptAdj)			//  insurance
				p += colWid - acWid - ptAdj;		// ... right justify

		// store leading blanks, text, 1 trailing blank only

		while (s < p)  *s++ = ' ';			// rest of blanks before this column
		memcpyPass( (void **)&s, text, acWid);		// store text / point past
		if (i)						// if not last column (i is colp->nxColi)
			if (s < end || RcolB.p[i].colGap)		// if this col not full or next col has non-0 gap (normally does)
				*s++ = ' ';     				// one separating space now, rest when size next col's datum known

	}  // for (i=...  columns loop
	strcpy( s, "\n");   			// nl and null on end of line buffer
	vrStr( dvrip->vrh, buf);			// output to virtual report
}				// vpUdtRpRow
//==================================================================
LOCAL void FC NEAR vpUdtExRow( DVRI *dvrip)	// virtual print current interval row for user-defined export
{

// output comma-separated row of data, strings quoted
	char buf[MAXRPTLINELENUDT];		// size must exceed longest possible line
	char* s = buf;  		// set pointer for storing into line buffer
	COL* colp;
	for (int i = dvrip->coli;  i;  i = colp->nxColi)	// loop over columns of table
	{
		colp = XcolB.p + i;

		// format data

		USI dt = colp->colVal.ty;				// data type, DTFLOAT or DTCHP, from VALNDT struct member
		char* text;
		if (ISNANDLE(colp->colVal.val))			// if UNSET (bug)(exman.h) or expression not evaluated yet, show "?".
			text = "?";					// show "?".  Issue message?  treat UNSET differently?
		/* uneval'd exprs may be able to occur at start run if rpFreq > expr's evf.
						   Only place in language where run with excess evf intentionally permitted, 12-91.
						   Note generally don't see ?'s for evf >= daily as evaluated during warmup. */
		else
			switch (dt)
			{
			case DTFLOAT:
			{	int dec = colp->colDec;
				USI cvFmt = 			// cvin2s format word (cvpak.h): bits, plus # digits in lo nibble
					(dec >= 0)			//  if decimals given, use F format with that many digits after point
					? FMTSQ | FMTOVFE 		//    use "squeezed" F format, overflow into E format (not k),
					| min(dec, 15)   	//    show 'dec' digits after point (4 bits avail).
					:				//  decimals not given, specify # sig digits rather than point posn.
					FMTSQ | FMTRTZ 	 	//    squeeze, # given is # sig not # dec dig & truncate trailing 0's,
					| FMTOVFE | 6;    	//    overfl to E format, # sig digits is 6 (if value >= .001)

				text = cvin2s(&colp->colVal.val, dt, UNNONE, colp->colWid, cvFmt); 	// convert, to Tmpstr, cvpak.cpp
				break;
			}

			case DTCHP:
				text = (char *)(colp->colVal.val);
				break;			// string: no conversion; supply quotes

			default:
				text = "\"<bad dt>\"";
				break;
			}

		// store text, quoted if string, and separating comma

		if (dt==DTCHP)
			*s++ = '"';
		memcpyPass( (void **)&s, text, (USI)strlen(text) );  	// store text / point past
		if (dt==DTCHP)
			*s++ = '"';
		if (colp->nxColi)			// if not last column
			*s++ = ',';				// separating comma

	}  // for (i=...  columns loop
	strcpy( s, "\n");   			// nl and null on end of line buffer
	vrStr( dvrip->vrh, buf);			// output buffer to virtual report
}				// vpUdtExRow
//==================================================================
LOCAL char * CDEC NEAR fmtRpColhd( 	// format report columns table heading per COLDEF table

	COLDEF *colDef,	// Pointer to table which describes heading format
	char *head,  	// ptr to (big enough) buffer in which to build head
	USI flags,    	/* col head enable bits, matched against colDef .flags for each column:
			      If any SKIPFLAGS on in colDef but off in this arg, col is OMITTED;
			      If any BLANKFLAGS on in colDef but off in this arg, col is head is BLANKED. */
	... )		// add'l char * args are used where colDef->colhd is -1, eg for Mon/Day/Hr for ZrRep 1st col per rpFreq

// Rets ptr to null-terminated heading string in head[]. Generates 2 rows: second is ----'s underlining the text.
{
	SI i, l, blankit = 0;   char *underline;   va_list ap;

	va_start( ap, flags);

// determine length, point to 2nd line position in buffer, for "-----" 's

	l = 0;							// init line length
	for (i = -1;  colDef[++i].colhd;  )				// loop over columns
		if (!(SKIPFLAGS & colDef[i].flags & ~flags)) 		// OMIT column with SKIPFLAGS bit that caller did not give
		{
			if (l)  						// no intercol space for 1st non-skipped column 6-95
				l += !(colDef[i].flags & 1);		// intercol space if not suppressed by option
			l += colDef[i].width;    				// accumulate column widths
		}
	underline = head + l + 1;					// ptr to beg of "-----" line. +1 for \n.

// column write loop

	for (i = 0;  colDef->colhd;  colDef++)		// loop columns; i is buffer subscript
	{
		if (SKIPFLAGS & colDef->flags & ~flags)  continue;     	// OMIT column with SKIPFLAGS bit that caller did not give

		if (i)							// unless 1st col on line 6-95
			if (!(colDef->flags & 1))    				// unless flag
			{
				*(head+i) = *(underline+i) = ' ';			// intercol spc
				i++;
			}

#if BLANKFLAGS  // omit code if no such bits
		blankit = (BLANKFLAGS & colDef->flags &~flags);   	// BLANK OUT column with BLANKFLAGS bit that caller did not give
#endif

		const char *colhd =  (colDef->colhd==(const char *)-1L)
				?  va_arg( ap, const char *)
			    :  colDef->colhd; 	// for pointer -1L, use next arg

		cvin2sBuf( head+i, 					// format col head text (lib\cvpak.cpp)
			blankit ? "" : colhd,
			DTCH, UNNONE, colDef->width,
			colDef->flags & 128 ? FMTLJ : FMTRJ );	// format: left justify on flag 128

		memset( underline + i, 					// store ---- in line following text
			blankit ? ' ' : '-',
			colDef->width );

		i += colDef->width;					// pass column in buffer
	}
	*(head+i) = '\n';		  		// nl at end of 1st row
	strcpy( underline+i, "\n");			// nl and null at end of 2nd row
	return head;
}			// fmtRpColhd
//==================================================================
LOCAL char * CDEC NEAR fmtExColhd( 		// format export file columns heading per COLDEF table

	COLDEF *colDef,	// Pointer to table which describes heading format
	char *head,   	// ptr to (big enough) buffer in which to build head
	USI flags,    	/* col head enable bits, matched against colDef .flags for each column:
			   SKIPFLAGS:  if any of these bits on in colDef but off in this arg, col is OMITTED
			   BLANKFLAGS: if any of these bits on in colDef but off in this arg, col is output as "" only */
	... )		// add'l char * args are used where colDef->colhd is -1L, eg for Mon/Day/Hr for ZrRep 1st col per rpFreq

// Returns pointer to null-terminated heading string in head.
{
	char *s;   va_list ap;

	va_start( ap, flags);

	for (s = head;  colDef->colhd;  colDef++) 	// column write loop.  CAUTION: don't test with==NULL: compiler supplies ds.
	{
		if (SKIPFLAGS & colDef->flags & ~flags)	// if col has SKIPFLAGS bit that caller did not give
			continue;				// OMIT this column completely
		if (s != head)				// if not first column
			*s++ = ',';				// separating comma
		*s++ = '"';				// enclose in quotes

		const char* colhd =  (colDef->colhd==(const char *)-1L)
			  ?  va_arg( ap, char *)
			  :  colDef->colhd; 	// for "pointer" -1L use next arg

#if BLANKFLAGS  // omit code if no such bits
		if (!(BLANKFLAGS & colDef->flags &~flags))	// just "" if col has BLANKFLAGS 64 bit that caller did not give
#endif
		{
			strcpy( s, colhd);			// col head to head string
			s += strlen(s);  			// point past
		}
		*s++ = '"';				// enclose in quotes
	}
	*s++ = '\n';
	*s = 0;					// terminating null
	return head;
}			// fmtExColhd

/************************************* IF-OUTs *****************************************/

#if 0	// combined with above 1-92 using new flags 2 and 4
x// columns definition for "energy balance" EXPORT (zone results, xEB exports) file
xstatic COLDEF ebExp[] =					// for [cgRepHour, -Subhr] ...
x   /*		    max      offset        cvflag  */
x   /* colhd   flags wid dfw  (cnguts.h)    (above) */
x   {   "Mon",    0,  2,  0,  255,	   CVI,	// offset > 250: use next arg pointer in variable arg list
x       "Day",    0,  2,  0,  255,	   CVI,
x       "Hr",     0,  2,  0,  255,	   CVI,
x       "Subhr",  0,  1,  0,  255,	   CVS,
x       "Tair",   0,  5,  2,  oRes(tAir),   CV1,	// average air temp
x       "Cond",   0,  6,  3,  oRes(qCond),  BTU,	// wall conduction gain
x       "Infil",  0,  6,  3,  oRes(qInfil), BTU,	// infiltration gain
x       "Slr",    0,  5,  3,  oRes(qSlr),   BTU,	// solar gain
x       "Shutter", 0, 1,  0,  USE_NEXT_ARG, CVS,	// use addl arg (shutter frac)
x       "Ign",    0,  5,  3,  oRes(qIg),    BTU,	// internal gain
x       "Mass",   0,  6,  3,  oRes(qMass),  BTU,	// net transfer from mass
x       "Izone",  0,  6,  3,  oRes(qsIz),    BTU,	// interzone gain to zone
x       "Md",     0,  2,  0,  USE_NEXT_ARG, CVI,	// mode (addl arg)
x       "Mech",   0,  6,  3,  oRes(qMech),  BTU,	// mechanical gain (all TUs)
x       NULL,     0,  0,  0,  0,            CV1
x};
#endif

#if 0	// worked; now inLine
x//=================================================================
xLOCAL void FC NEAR vpFusBody( 		// virtual print Fuel-Use report body
x
x    RXPORTINFO *rxt )	// report/export info struct set in vpRxports
x#if 0
xx    SI vrh,		// print destination
xx    COLDEF *colDef,	// column definition table
xx    IVLCH freq, 	// interval (report frequency): year, month, or day
xx    SI isExport )	// 1 if export, 0 if report
x#endif
x
x{
xFU *mtr; FU_IVL_SUB *mtrs;
x
x// lines of report body
x
x    RLUP( MtrB, fu)				// loop over fuel use records (loop over fuel types)
x    {
x       fus = (&fu->Y) + (rxt->fq - 1);		// point substruct for interval in question
x
x   // skip fuels with no useage for this inteval
x       if (fus->tot==0.f)
x          continue;
x
x   // print row of text for this fuel
x       vpFusRow( rxt, fu);				// next
x	}
x
x// FUS export needs a blank line every time (after every body).  Note usual VR_FINEWL option omitted in cncult4.cpp.
x
x    if (rxt->isExport)
x       vrStr( rxt->vrh, "\n");
x}				// vpFusBody
#endif

// cgresult.cpp end
