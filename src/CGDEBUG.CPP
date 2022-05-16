// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cgdebug.cpp -- debug printing functions for CSE

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// includes dtypes.h: MASSBC

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>

#include <CVPAK.H>	// FMTLJ
#include <VRPAK.H>	// vrNewl
#include <PGPAK.H>	// PGLJ PGCUR
#include <CPGBUILD.H>	// PBHEAD PB_DATOFFL pgbuildr
#include <CNCULT.H>	// getBodyLpp
#include <CNGUTS.H>	// declarations for this file; Top
#include "ashwface.h"
#include "mspak.h"

/*-------------------------------- DEFINES --------------------------------*/

// member offset macros, to cnglob.h or pgbuildr.h when debugged ???
// signed version of stddef.h:offsetof
//   (for less lintCasting, because PBDATOFF/L.off is SI, because
//   special values are negative, because PBDATA.field is SI).
#define off(s,m)	SI(int(&(((s *)0L)->m)))
//  array member version of above: & removed for no "& ignored" warning
#define ofa(s,m)	SI(int(/*&*/(((s *)0L)->m)))

/*lint -e507	suppress "Size incompatibility" in every use of off(s,m) */

// pgbuildr table shorteners:
#define L PGLJ
#define C PGCUR
#define DTF DTFLOAT
#define UNN UNNONE
#define FL  FMTLJ
#define FS  FMTSQ

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL void  FC NEAR cgCmpDump( SI vrh, TI zi, TI adjZi, SI ty, PBHEAD *sdtab);
LOCAL void  FC NEAR cgMassDump( MSRAT *mse);
// LOCAL void  FC NEAR opsDumpI( char **ppp, char *tag, float *pOps);

//==================================================================
void FC cgzndump(		// "Print" description of zone from ZNR record ("ZDD" report)

	SI vrh,		// handle of "virtual report" (see vrpak.cpp) to which to append info
	TI zi )		// subscript of ZNR record (entry) to dump

// outputs to a virtual report opened here, using vrpak.cpp functions, via cpnat/cpgbuild.cpp.

// return value: virtual report handle
{
// ZNR entry (cnrecs.def/rccn.h) pgbuildr table
static PB_DATOFFL hzt[] = {
// pgfmt r   c    memberOffset          dt   units wid cvfmt label
{ L, C+3, 20, ofa( ZNR, name),       DTANAME, UNN,16, FS,   "ZONE DATA for zone " },
{ L, C, C+20, PBARGSI,				 DTCH,    UNN, 7, FS,   "Model: " },
// omit .xsurf1 .. .shtf
{ L, C+1, 6,  off( ZNR, i.znArea),      DTF,  UNN, 7, FL+2, "CFA: " },
{ L, C, C+7,  off( ZNR, i.znVol),       DTF,  UNN, 7, FL+1, "Vol: " },
{ L, C, C+9,  off( ZNR, i.zn_floorZ),   DTF,  UNN, 7, FL+2, "FloorZ: " },
{ L, C, C+10, off( ZNR, i.zn_ceilingHt),DTF,  UNN, 5, FL+2, "CeilingHt: " },
{ L, C, C+9,  off( ZNR, i.zn_eaveZ),    DTF,  UNN, 5, FL+2, "EaveZ: " },
{ L, C, C+8,  off( ZNR, i.znCAir),      DTF,  UNN, 5, FL+2, "CAir: " },
// omit 4 Opsi's, .qheatmax
// omit .qcoolmax
// infiltration info
{ L, C+1, 11, off( ZNR, i.infAC),     DTF,   UNN, 7, FL+3, "Infil AC: " },
{ L, C, C+7,  off( ZNR, i.infELA),    DTF, UNAREA2,7, FL+3, "ELA: " },
/* omit .inf.shielding */
{ L, C, C+10, off( ZNR, zn_stackc),   DTF,   UNN, 7, FL+3, "Stackc: " },
{ L, C, C+9,  off( ZNR, zn_windc),    DTF,   UNN, 7, FL+3, "Windc: " },
// omit .inf.xac .xstackc .xwindc
// [omit .natv..fanv..ceilFanFlg: separate tables]
{ L, C+1, 8,   off( ZNR, zn_ua),           DTDBL, UNN, 7, FL+3, "UA:    " },	// ambient UA, excludes specT,adjZn's,izxfers,...
{ L, C,   C+10,off( ZNR, zn_UANom),		  DTDBL, UNN, 7, FL+3, "UANom:  " },
{ L, C+1, 1,  PBOMITSI,		   0,     0,   0, 0,    NULL }, 	// newline
{ PBMETHEND }
};
static PBHEAD hzh = { PBDATOFFL, hzt, 0, 0, 0 };

#ifdef OLDNV
o/* ZNR (records.def/rchs.h) nat vent members pgbuildr table */
ostatic PB_DATOFFL nvt[] = {
o/* pgfmt r c    mbrOffset              dt   units wid cvfmt label */
o #if 0	// unimpl members eliminated rob 1-30-91 due to choica/choicb complications (unimpl VENTCON 'control' exists in tk too)
ox    L, C+1, 11, off( HSNATV, control), DTF,  UNN,  7, FL+3, "NV  ctrl: ",
ox    L, C, C+8,  off( HSNATV, topen),   DTF,  UNN,  7, FL+3, "Topn: ",
ox    L, C, C+8,  off( HSNATV, trange),  DTF,  UNN,  7, FL+3, "Trng: ",
ox    L, C, C+7,  off( HSNATV, ahigh),   DTF,  UNN,  7, FL+3, "VHi: ",
o #endif
o    L, C+1, 10, off( ZNR, i.nvAHi),    DTF,  UNN,  7, FL+3, "NV  VHi: ",
o    L, C, C+7,  off( ZNR, i.nvALo),    DTF,  UNN,  7, FL+3, "VLo: ",
o    L, C+1, 9,  off( ZNR, i.nvHD),     DTF,  UNN,  7, FL+3, "Hd: ",
o    L, C, C+9,  off( ZNR, i.nvAzmI),   DTF,  UNN,  7, FL+3, "AzI: ",
o    L, C, C+8,  off( ZNR,i.nvStEM),    DTF,  UNN,  7, FL+3, "Ems: ",
o    L, C, C+9,  off( ZNR,i.nvDdEM),    DTF,  UNN,  7, FL+3, "Emdd: ",
o    L, C, C+9,  off( ZNR,i.nvDiEM),    DTF,  UNN,  7, FL+3, "Emdi: ",
o    L, C+1, 10, off( ZNR, i.nvTD),     DTF,  UNN,  7, FL+3, "Td:  ",
o    L, C, C+9,  off( ZNR, dieff),      DTF,  UNN,  7, FL+3, "Efdi: ",
o    /* omit .fwind .fstack .chour */
o    L, C+1, 1,  PBOMITSI,	      0,    0,    0, 0, NULL, /* newline */
o    PBMETHEND
};
ostatic PBHEAD nvh = { PBDATOFFL, nvt, 0, 0, 0 };
#endif
#ifdef OLDFV // 1-92 cndefns.h
o/* ZNR (records.def/rchs.h) fan vent members pgbuildr table */
o static PB_DATOFFL fvt[] = {
o/* pgfmt r c    mbrOffset              dt units wid cvfmt label */
o    L, C+1, 11, off( ZNR, i.fvCfm ),   DTF, UNN, 7, FL+3, "FV   cfm: ",
o    L, C, C+6,  off( ZNR, i.fvKW  ),   DTF, UNKW,7, FL+3, "kW: ",
o    L, C, C+9,  off( ZNR, i.fvFz  ),   DTF, UNN, 7, FL+3, "fZone: ",
o #if 1 /* indirect evap cooling, 4-23-91 */
o    L, C, C+11, off( ZNR, i.fvDECeff), DTF, UNN, 7, FL+3, "DECeff: ",
o    L, C, C+11, off( ZNR, i.fvIECeff), DTF, UNN, 7, FL+3, "IECeff: ",
o #else
ox    L, C, C+11, off( ZNR,i.fvEvapEff), DTF, UNN, 7, FL+3, "evapEff: ",
ox    L, C, C+11, off( ZNR, i.fDirect ), DTF, UNN, 7, FL+3, "fDirect: ",
o #endif /* 4-23-91 */
o    L, C+1, 10, off( ZNR, i.fvTD  ),   DTF, UNN, 7, FL+3, "     Td: ",
o    L, C, C+9,  off( ZNR,i.fvWbMax),   DTF, UNN, 7, FL+3, "wbMax: ",
o    L, C, C+9,  off( ZNR, tRise     ), DTF, UNN, 7, FL+3, "tRise: ",
o    L, C, C+10, off( ZNR, havmax    ), DTF, UNN, 7, FL+3, "havMax: ",
o    /* omit .tsup */
o    L, C+1, 1,  PBOMITSI,	       0,   0,   0, 0, NULL, /* newline */
o    PBMETHEND };
ostatic PBHEAD fvh = { PBDATOFFL, fvt, 0, 0, 0 };
#endif
#ifdef OLDCF
o/* ZNR (records.def/rchs.h) ceiling fan members pgbuildr table */
o static PB_DATOFFL cft[] = {
o/* pgfmt r c    mbrOffset              dt   units wid cvfmt label */
o    L, C+1, 10, off( ZNR, i.cfKW     ), DTF, UNKW,7, FL+3, "CF   kW: ",
o    L, C, C+10, off( ZNR, i.cfFz     ), DTF, UNN, 7, FL+3, "fZone: ",
o    L, C, C+7,  off( ZNR, qZone      ), DTF, UNN, 7, FL+3, "qZone: ",
o    L, C+1, 1,  PBOMITSI,	        0,   0,   0, 0, NULL, /* newline */
o    PBMETHEND };
o static PBHEAD cfh = { PBDATOFFL, cft, 0, 0, 0 };
#endif

// XSRAT (rchs.def) pgbuildr table (as surface)
static PB_DATOFFL cmpt1[] = {
// pgfmt r   c    mbrOffset           dt   units   wid cvfmt label
// omit .ty, .next
{ L, C,   9,  ofa( XSRAT, name),    DTANAME, UNN,   16, FL,    "Surface " },
{ L, C, C+6,  off( XSRAT, x.xs_area ), DTF, UNN,     7, FL+1,    "Area: " },
{ L, C, C+5,  off( XSRAT, x.azm  ), DTF,    UNANGLE, 4, FL,   "Az: "  },
{ L, C, C+3,  off( XSRAT, x.tilt ), DTF,    UNANGLE, 4, FL,   "T: "   },
{ L, C, C+6,  off( XSRAT, x.xs_UNom ),DTF,  UNN,     7, FL+3, "UNom: "   },
{ L, C, C+6,  off( XSRAT, x.xs_UANom ),DTF, UNN,     7, FL+2, "UA: " },
{ L, C, C+7,  off( XSRAT, x.xs_sbcI.sb_absSlr),
									DTF,    UNN,     6, FL+2, "InAbs: " },
{ L, C, C+6,  off( XSRAT, x.xs_sbcO.sb_absSlr),
									DTF,    UNN,     6, FL+2, "ExAbs: " },
// caller supplies exterior condition string argument with type plus temperature, zone name, etc.
{ L, C, C+8,  PBARGSI,              DTCH,   UNN,    30, FL,  "ExCnd: " },
{ PBMETHEND }
};
static PBHEAD cmph1 = { PBDATOFFL, cmpt1, 0, 0, 0 };

// XSRAT pgbuildr table (as interior surface) 2-95.
//	Differences from above: "IntSurf"; XZn not XT.
static PB_DATOFFL cmpt1i[] = {
// pgfmt r   c    mbrOffset           dt   units   wid cvfmt label
// omit .ty, .next
{ L, C,   9,  ofa( XSRAT, name),       DTANAME, UNN, 16, FL,   "IntSurf " },	// "IntSurf": difference from above
{ L, C, C+6,  off( XSRAT, x.xs_area ), DTF,    UNN,  7, FL+2, "Area: " },
{ L, C, C+5,  off( XSRAT, x.azm  ),    DTF, UNANGLE, 4, FL,   "Az: "  },
{ L, C, C+3,  off( XSRAT, x.tilt ),    DTF, UNANGLE, 4, FL,   "T: "   },
{ L, C, C+6,  off( XSRAT, x.xs_UNom ), DTF, UNN,     7, FL+4, "UNom: "   },
{ L, C, C+9,  off( XSRAT, x.xs_sbcI.sb_absSlr),
									   DTF, UNN,     6, FL+3, "InAbs: " },
{ L, C, C+7,  off( XSRAT, x.xs_sbcO.sb_absSlr),
									   DTF, UNN,     6, FL+3, "ExAbs: " },
// caller supplies exterior condition string argument with type plus temperature, zone name, etc.
{ L, C, C+8,  PBARGSI,                 DTCH, UNN,    30, FL,   "ExCnd: " },
{ PBMETHEND }
};
static PBHEAD cmph1i = { PBDATOFFL, cmpt1i, 0, 0, 0 };

// XSRAT (rchs.def) pgbuildr table (as other zone side of interior surface) 2-95
//	Difference from above: ownTi not sfAdjZi.
static PB_DATOFFL cmpt1io[] = {
// pgfmt r   c    mbrOffset           dt   units   wid cvfmt label
// omit .ty, .next
{ L, C,   9,  ofa( XSRAT, name),   DTANAME, UNN,    16, FL,   "IntSurf " },
{ L, C, C+6,  off( XSRAT, x.xs_area ), DTF, UNN,     7, FL+2, "Area: " },
{ L, C, C+5,  off( XSRAT, x.azm  ), DTF,    UNANGLE, 4, FL,   "Az: "  },
{ L, C, C+3,  off( XSRAT, x.tilt ), DTF,    UNANGLE, 4, FL,   "T: "   },
// U, InAbs, ExAbs: shown only in owning zone.
// caller supplies exterior condition string argument with owning zone name.
{ L, C, C+31, PBARGSI,             DTCH, UNN,    16, FL,   "'outside' of surface in zone " },
{ PBMETHEND }
};
static PBHEAD cmph1io = { PBDATOFFL, cmpt1io, 0, 0, 0 };

// XSRAT (rchs.def) pgbuildr table (as window)
static PB_DATOFFL cmpt2[] = {
// pgfmt r   c    mbrOffset           dt   units   wid cvfmt label
// omit .ty, .next
{ L, C,  8,   ofa( XSRAT, name   ),DTANAME, UNN, 16, FL,   "Window " },
{ L, C, C+7,  off( XSRAT, x.xs_area ), DTF, UNN,  7, FL+2, "Area: " },
{ L, C, C+5,  off( XSRAT, x.azm  ), DTF, UNANGLE, 4, FL,   "Az: " },
{ L, C, C+3,  off( XSRAT, x.tilt ), DTF, UNANGLE, 4, FL,   "T: " },
{ L, C, C+6,  off( XSRAT, x.xs_UNom ),DTF, UNN,   7, FL+3, "UNom: " },
{ L, C, C+6,  off( XSRAT, x.xs_UANom ),DTF, UNN,  7, FL+1, "UA: " },
{ L, C, C+7,  off( XSRAT, x.sco  ), DTF, UNN,     6, FL+4, "SMSO: " },
{ L, C, C+7,  off( XSRAT, x.scc  ), DTF, UNN,     6, FL+4, "SMSC: " },
// caller supplies exterior condition string argument with type plus temperature, zone name, etc.
{ L, C, C+8,  PBARGSI,             DTCH, UNN,    30, FL,   "ExCnd: " },
{ PBMETHEND }
};
static PBHEAD cmph2 = { PBDATOFFL, cmpt2, 0, 0, 0 };

// XSRAT pgbuildr table (as perim)
static PB_DATOFFL cmpt3[] = {
// pgfmt r   c    mbrOffset            dt    units   wid cvfmt label
		// omit .ty, .next
{ L, C,   7,  ofa( XSRAT, name),       DTANAME, UNN, 16, FL,   "Perim " },
{ L, C, C+8,  off( XSRAT, x.xs_area ), DTF,    UNN,  7, FL+2, "Length: " },	// aligned with surface Area
{ L, C, C+19, off( XSRAT, x.xs_UNom ), DTF, UNN,     7, FL+3, "F2: " },	    // aligned with surface U
{ PBMETHEND }
};
static PBHEAD cmph3 = { PBDATOFFL, cmpt3, 0, 0, 0 };


// IZXRAT (rchs.h/records.def) (interzone transfer) pgbuildr table
static PB_DATOFFL izxct[] = {
// pgfmt r  c     mbrOffset               dt  units wid cvfmt label
		// newline between / blank line above first
{ L, C,   8,  ofa( XSRAT, name),        DTANAME, UNN,16, FL,   "IzXfer " },	// izxfer name, if any, constant width.
// zone names: caller puts pointers in arg list. squeezed format.
{ L, C, C+6,  PBARGSI,                  DTCH, UNN,16, FS,   "Zn1: " },	// c +1 to align. wid 12-->16 2-95.
{ L, C, C+7,  PBARGSI,                  DTCH, UNN,16, FS,   "Zn2: " },	// wid 12-->16 2-95
{ L, C, C+11, off( IZXRAT, iz_ua),      DTF,  UNN, 7, FL+3, "UAconst: " },
{ L, C, C+8,  PBARGSI,                 DTCH, UNN,16,  FS,   "NVctrl: " },
{ L, C+1, 10, off( IZXRAT, iz_a1),      DTF,  UNN, 7, FL+3, "Vlo: " },
{ L, C, C+8,  off( IZXRAT, iz_a2),      DTF,  UNN, 7, FL+3, "Vhi:  " },
{ L, C, C+6,  off( IZXRAT, iz_hz),      DTF,  UNN, 7, FL+3, "Hz: " },
{ L, C, C+6,  off( IZXRAT, iz_cd),      DTF,  UNN, 7, FL+3, "Cd: " },
{ L, C, C+7,  off( IZXRAT, iz_nvcoeff), DTF,  UNN, 7, FL+3, "NVc: " },
{ L, C, C+7,  off( IZXRAT, iz_cpr),     DTF,  UNN, 7, FL+3, "Cpr: " },
// omit .z1first, .z2first
{ L, C+1, 1,  PBOMITSI,	        0,   0,   0, 0, NULL },		 // newline
{ PBMETHEND }};
static PBHEAD izxch = { PBDATOFFL, izxct, 0, 0, 0 };

	MSRAT *mse;
	IZXRAT *ize;
	SI did;
	char *pp=NULL;
	RC rc=RCOK;

	ZNR* zp = ZrB.p + zi;			// point to ZNR entry

	pnSetVrh(vrh);			// set virtual report handle for cpgbuild/cpnat output. cpnat.cpp.
	pnSetTxRows( getBodyLpp());		/* get # report body lines per page and pass it to pnat.cpp, 11-91
					   cncult.cpp fcn computes bodyLpp from user repLpp, repTM, repBM, hdr, ftr sizes */

// Basic info, nat vent, fan vent, ceil fan, possible page break??.
#if 0	// if OLDNV,FV,CF etc
o    pgbuildr( &pp, &rc, 20, 78, 0, NULL, &hzh, zp, PBSPECEND );
#else
	pgbuildr( &pp, &rc, 20, 90, 0, NULL, &hzh, zp,
		zp->getChoiTx( ZNR_I+ZNISUB_ZNMODEL, 1), PBDONE );
#endif
#ifdef OLDNV //cndefns.h 1-92
o    pgbuildr( &pp, &rc,  0,  0, 0, NULL, &nvh, zp, PBSPECEND );
#endif
#ifdef OLDFV
o    pgbuildr( &pp, &rc,  0,  0, 0, NULL, &fvh, zp, PBSPECEND );
#endif
#ifdef OLDCF
o    pgbuildr( &pp, &rc,  0,  0, 0, NULL, &cfh, zp, PBDONE );
#endif
	// possible page break here

// XSURFs: Walls, windows, perimeter, mass exterior walls
// CAUTION special args for some tables may be hard-coded into cgCmpDump by 'ty'.
	cgCmpDump( vrh, zi, 0, CTEXTWALL, &cmph1);
	cgCmpDump( vrh, zi, 0, CTMXWALL,  &cmph1);
	cgCmpDump( vrh, zi, 0, CTINTWALL, &cmph1i); 	// !! 2-95
	cgCmpDump( vrh, 0, zi, CTINTWALL, &cmph1io);	// duplicate display of walls with "outside" in this zone
	cgCmpDump( vrh, zi, 0, CTWINDOW,  &cmph2);
	cgCmpDump( vrh, zi, 0, CTPERIM,   &cmph3);

// Mass: dump those masses whose inside is adjacent to the current zone.
	did = 0;
	RLUP( MsR, mse)
	{	if (IsMSBCZone( mse->inside.bc_ty) && mse->inside.bc_zi==zi)	// if mass inside is adjacent to zone zi
		{
			cgMassDump( mse);
			did++;
		}
	}
	if (did)				// if printed any
		vrStr(vrh, "\r\n" );		// blank line after all 12-89

// Interzone transfers; scan all, print those whose zone 1 is equal to current zone.  Thus each IZXfer prints once.
	did = 0;
	pgbuildr( &pp, &rc, 20, 120, 0, NULL, PBSPECEND );		// 78-->120 2-95
	// pre-alloc PAGE.  Unlikely to be more than a full page of them, so print all on 1 PAGE to page 1st not between.
	RLUP( IzxR, ize)				// for izxfer 1 to .n (cnglob.h macro)
	{
		if (ize->iz_zi1 == zi)
		{
			const char* name2 =
					ize->iz_zi2 > 0      ? ZrB[ ize->iz_zi2].name
				  : ize->iz_IsHERV()     ? "(HERV)"
				  : ize->iz_IsExterior() ? "(ambient)"
				  :                        "--";
			pgbuildr( &pp, &rc, 0, 0, 0, NULL, &izxch, ize,
					  ZrB[ ize->iz_zi1].name,  			// PBARGSI 1
					  name2,							// 2
					  ize->getChoiTx( IZXRAT_NVCNTRL, 1),// 3
					  PBSPECEND );
			did++;
		}
	}
	if (did)						// if any izxfers
		pgbuildr( &pp, &rc, 0, 0, 0, NULL, PBDONE);	// print PAGE
	pgfree( &pp);					// free page, NULL pp, pgpak.cpp
}				// cgzndump

//==================================================================
LOCAL void FC NEAR cgCmpDump(	// Dump a zone's XSURFs of specified type
	SI vrh,		// virtual report to which to output
	TI zi,		// zone subscript, 0=any
	TI adjZi,	// adjacent zone subscript (for interior walls), 0=any
	SI ty, 		// XSURF ty (CTxxx defines in cnguts.h) to dump
	PBHEAD* pbh )	// Pointer to PBHEAD pgbuildr driver table.
					// CAUTION: special args for some tables may be hard-coded in this fcn per 'ty'.

{
// WSHADRAT (records.def/rchs.h) pgbuildr table
static PB_DATOFFL wshadt[] = {
// pgfmt r   c    memberOffset              dt  units wid cvfmt label
		// newline: is continuation on same PAGE
#if 0
x		{ L, C+1, 17, off( WSHADRAT, wWidth  ), DTF, UNN, 7, FL+3, "Shading  wW: " },
#else//name 2-95
{ L, C+1, 10, ofa( WSHADRAT, name    ), DTANAME,UNN,16,FL, "Shading "},
{ L, C, C+4,  off( WSHADRAT, wWidth  ), DTF, UNN, 7, FL+3, "wW: "    },
#endif
{ L, C, C+6,  off( WSHADRAT, wHeight ), DTF, UNN, 7, FL+3, "wH: "    },
{ L, C+1, 10, off( WSHADRAT, ohDepth ), DTF, UNN, 7, FL+3, "ohD: "   },
{ L, C, C+8,  off( WSHADRAT, ohDistUp), DTF, UNN, 7, FL+3, "ohWd: "  },
{ L, C, C+8,  off( WSHADRAT, ohExL   ), DTF, UNN, 7, FL+3, "ohLx: "  },
{ L, C, C+8,  off( WSHADRAT, ohExR   ), DTF, UNN, 7, FL+3, "ohRx: "  },
{ L, C, C+10, off( WSHADRAT, ohFlap  ), DTF, UNN, 7, FL+3, "ohFlap: "},
{ L, C+1, 10, off( WSHADRAT, lfDepth ), DTF, UNN, 7, FL+3, "flD: "   },
{ L, C, C+8,  off( WSHADRAT, lfTopUp ), DTF, UNN, 7, FL+3, "flTx: "  },
{ L, C, C+8,  off( WSHADRAT, lfDistL ), DTF, UNN, 7, FL+3, "flWd: "  },
{ L, C, C+8,  off( WSHADRAT, lfBotUp ), DTF, UNN, 7, FL+3, "flBx: "  },
{ L, C+1, 10, off( WSHADRAT, rfDepth ), DTF, UNN, 7, FL+3, "frD: "   },
{ L, C, C+8,  off( WSHADRAT, rfTopUp ), DTF, UNN, 7, FL+3, "frTx: "  },
{ L, C, C+8,  off( WSHADRAT, rfDistR ), DTF, UNN, 7, FL+3, "frWd: "  },
{ L, C, C+8,  off( WSHADRAT, rfBotUp ), DTF, UNN, 7, FL+3, "frBx: "  },
{ PBMETHEND }
};
static PBHEAD wshadh = { PBDATOFFL, wshadt, 0, 0, 0 };

// Solar gain distrib pgbuildr table (substruct SGDIST of XSURF, rchs.def).
//	   table to produce old (3-90) output from new XSURFs */
static PB_DATOFFL sgdt[] = {
// pgfmt r   c    memberOff                 dt     units wid cvfmt label
// newline: is continuation on same PAGE
// caller supplies THREE string args describing target
{ L, C+1, 16, PBARGSI,                  DTANAME, UNN, 20, FS,   "SgDist to " },	// "mass"/"surface"/"zone"
{ L, C, C+1,  PBARGSI,                  DTCH,    UNN, 20, FS,   " " },		// name
{ L, C, C+1,  PBARGSI,                  DTCH,    UNN, 20, FS,   " " },		// "inside"/"outside"/"CAir"/"total"
{ L, C, C+6+3, off( SGDIST, sd_FSO),  DTF,  UNN,  7, FL+3, " FSO: " },
{ L, C, C+6,   off( SGDIST, sd_FSC),  DTF,  UNN,  7, FL+3, " FSC: " },
{ PBMETHEND }
};
static PBHEAD sgdh = { PBDATOFFL, sgdt, 0, 0, 0 };

// ASHWAT window details
static PB_DATOFFL ashwatT[] = {
//   r    c    memberOff               dt       units wid cvfmt label
{ L, C+1, 16,  off( FENAW, fa_SHGC),   DTF,     UNN,  8, FL+3, "ASHWAT SHGC: " },
{ L, C,  C+6,  off( FENAW, fa_refDesc),DTANAME, UNN, 35, FS,   " ref: " },
{ L, C,  C+15, off( FENAW, fa_frmArea),DTF,     UNN,  8, FL+2, " frmArea: " },
{ L, C,  C+10, off( FENAW, fa_mSolar), DTF,     UNN,  8, FL+3, " mSolar: " },
{ L, C,  C+6,  off( FENAW, fa_mCond),  DTF,     UNN,  8, FL+3, " mCond: " },
{ PBMETHEND }
};
static PBHEAD ashwatH = { PBDATOFFL, ashwatT, 0, 0, 0 };

	char *pp=NULL;
	RC rc=RCOK;
	SI did = 0;

	XSRAT* xs;
	RLUP (XsB, xs)				// loop over all XSURFs (2-95 have no chain yet by outside zone)
	{
		if (zi)			  				// if owning (inside) zone specified
			if (xs->ownTi != zi)				// if wrong inside zone
				continue;						// skip
		if (adjZi) 						// if adjacent (outside) zone specified
			if ( xs->x.sfExCnd != C_EXCNDCH_ADJZN 		// if no zone outside (believe redundant to check this)
	 		 ||  xs->x.sfAdjZi != adjZi )				//   or wrong adjacent zone
				continue;						// skip
		if (xs->x.xs_ty == ty)			// if type matches
		{
			// special arg(s) required by certain specific tables, 2-95
			char * argp = (char *)PBSPECEND;
			if (ty==CTINTWALL && adjZi)			// if doing other sides of interior walls
				argp = ZrB.p[xs->ownTi].name;				// show owning zone name, where full info is
			else if ( ty==CTEXTWALL || ty==CTINTWALL	// for surface...
					|| ty==CTMXWALL || ty==CTWINDOW )	// or window,  show exterior condition, with pertinent subfields
			{
				char excstr[40];
				argp = excstr;
				strcpy( excstr, ::getChoiTxI( DTEXCNDCH, xs->x.sfExCnd));	// text "Ambient", "Adiabatic", "SpecifiedT", or "AdjZn"
				char *p = excstr + strlen(excstr);
				switch (xs->x.sfExCnd)					// add subfields for exCnd's that need them
				{
				case C_EXCNDCH_SPECT: 					// add specified temperature
					*p++ = ' ';
					cvin2sBuf( p, &xs->x.sfExT, DTFLOAT, UNTEMP, 7, FMTSQ);  	// checks for expr NANs. lib\cvpak.cpp.
					break;

				case C_EXCNDCH_ADJZN: 					// add adjacent zone name
					*p++ = ' ';
					strcpy( p, ZrB.p[xs->x.sfAdjZi].name);
					break;
				}
			}
			// display record and subsidiary records
			pgbuildr( &pp, &rc, 30, 127, 0, NULL, pbh, xs, 		// alloc PAGE, dump.
					  argp, PBSPECEND );
			if (xs->x.xs_IsASHWAT())
				pgbuildr( &pp, &rc, 0, 0, 0, NULL,
					  &ashwatH, xs->x.xs_pFENAW[ 0], PBSPECEND );
			if (xs->x.iwshad)						// If there's shading ...
				pgbuildr( &pp, &rc, 0, 0, 0, NULL, 			// dump it
						  &wshadh, &WshadR.p[xs->x.iwshad], PBSPECEND );
			for (int i = -1; ++i < xs->x.nsgdist; )				// dump its solar gain dists
			{
				char *s1 = nullptr, *s2, *s3 = nullptr;
				BP rb = nullptr;
				switch (xs->x.sgdist[i].sd_targTy)
				{
				case SGDTTZNAIR:
				case SGDTTZNTOT:
					s1 = "zone";
					rb = (BP)&ZrB;
					break;

				case SGDTTSURFI:
				case SGDTTSURFO:
					s1 = XsB[ xs->x.sgdist[i].sd_targTi].x.xs_msi > 0 ? "mass" : "surface";
				    rb = (BP)&XsB;
					break;

				}
				s2 = rb->rec(xs->x.sgdist[i].sd_targTi).name;
				switch (xs->x.sgdist[i].sd_targTy)
				{
				case SGDTTZNAIR:
					s3 = "CAir";
					break;   		// not expected here 2-95
				case SGDTTZNTOT:
					s3 = "total";
					break;  		// should not get here 2-95
				case SGDTTSURFI:
					s3 = "inside";
					break;
				case SGDTTSURFO:
					s3 = "outside";
					break;
				}
				pgbuildr( &pp, &rc, 0, 0, 0, NULL,
						  &sgdh, &xs->x.sgdist[i], s1, s2, s3, PBSPECEND );
			}
			pgbuildr( &pp, &rc, 0, 0, 0, NULL, PBDONE);			// print PAGE
			did++;
		}
	}
	if (did)						// if printed any
		vrStr(vrh, "\r\n" );				// blank line after all of type
}				// cgCmpDump
//==================================================================
LOCAL void FC NEAR cgMassDump( MSRAT* mse)	// Dump a mass
{
// MSRAT entry pgbuild table
static PB_DATOFFL masst[] = {
// pgfmt r c    mbrOffset                  dt      units wid cvfmt label
{ L, C, 6,   ofa( MSRAT, name),    DTANAME, UNN, 16, FL,   "Mass " },		// wid 12-->16, FS-->FL 2-95.
{ L, C, C+9, off( MSRAT, ms_area), DTF,     UNN,  7, FL+3, "Area: " },
{ L, C, C+9, off( MSRAT, ms_UNom), DTF,     UNN, 10, FL+4, "UNom: " },
{ L, C, C+6, off( MSRAT, ms_tc ),  DTF,     UNN,  5, FL+2, "tc: " },
{ L, C,C+11, off( MSRAT,isSubhrly), DTSI,   UNN,  1, FL,   "subhrly: " },
{ PBMETHEND }};
static PBHEAD massh = { PBDATOFFL, masst, 0, 0, 0 };

// MASSBC (boundary condition) pgbuild table
static PB_DATOFFL bct[] = {
// pgfmt r  c    mbrOffset            dt   units wid cvfmt label
// caller supplies 1 string args
{ L, C+1, 2,  PBTEXT,             DTCH,    UNN, 100, FL,  "" },
{ PBMETHEND }};
static PBHEAD bch = { PBDATOFFL, bct, 0, 0, 0 };

// MASSLAYER (dtypes.def) pgbuild table
static PB_DATOFFL lyrt[] =
{	// pgfmt r  c    mbrOffset              dt   units wid cvfmt label
{ L, C+1, 15, off( MASSLAYER, ml_thick),   DTDBL,   UNN, 7, FL+3, "Layer  thk: " },
{ L, C, C+6,  off( MASSLAYER, ml_vhc),     DTDBL,   UNN, 7, FL+3, "vhc: " },
{ L, C, C+7,  off( MASSLAYER, ml_condNom), DTFLOAT, UNN, 7, FL+3, "cond: " },
{ L, C, C+7,  off( MASSLAYER, ml_RNom),    DTFLOAT, UNN, 7, FL+3, "R: " },
{ PBMETHEND }};
static PBHEAD lyrh = { PBDATOFFL, lyrt, 0, 0, 0 };

	char *pp = NULL;
	RC rc = RCOK;
	[[maybe_unused]] const XSURF& xs = mse->ms_GetXSRAT()->x;

// Allocate page (autoexpands) and format basic mass info
	pgbuildr( &pp, &rc, 10, 78, 0, NULL, &massh, mse, PBSPECEND );

// Inside BC
	const char* bcx = mse->ms_SurfBCDesc( 0);
	pgbuildr( &pp, &rc, 0, 0, 0, NULL, &bch, bcx, PBSPECEND );

// Layers
	MASSMODEL* pMM = mse->ms_pMM;
	for (int iL = 0; iL < pMM->mm_NLayer(); iL++)
		pgbuildr( &pp, &rc, 0, 0, 0, NULL, &lyrh, pMM->mm_GetLayer( iL), PBSPECEND );

// Outside BC and print PAGE
	bcx = mse->ms_SurfBCDesc( 1);

	pgbuildr( &pp, &rc, 0, 0, 0, NULL, &bch, bcx, PBDONE );
}       	// cgMassDump
//--------------------------------------------------------------------------------
const char* MSRAT::ms_SurfBCDesc(		// generate description of boundary condix
	int si) const	// 0=inside, 1=outside
// returns descriptive string (in TmpStr)
{
	const XSURF& xs = ms_GetXSRAT()->x;
	[[maybe_unused]] const XSURF* pXS = &xs;
	const MASSBC& bc = si ? outside : inside;
	const char* bcx = "?";
	switch( bc.bc_ty)
	{
	case MSBCADIABATIC:
		bcx = "Adiabatic";
		break;
	case MSBCAMBIENT:
		bcx = "Ambient";
		break;
	case MSBCGROUND:
		bcx = "Ground";
		break;
	case MSBCZONE:
		bcx = strtprintf("Zone '%s'", ZrB.p[ bc.bc_zi].name);
		break;
	case MSBCSPECT:
		// use cvin2s cuz it checks for expr NANs:
		bcx = strtprintf( "SpecifiedT %s", cvin2s( &bc.bc_exTa, DTFLOAT, UNTEMP, 7, FMTSQ) );
		break;
	}

	const char* hStr = ms_isFD
		? strtprintf( "RSrfNom: %.2f", xs.xs_rSrfNom[ si])
		: strtprintf( "rsurf=%.2f  h=%.3f", bc.bc_rsurf, bc.bc_h);

	static const char* inOut[] = {"Inside", "Outside" };

	const char* desc = strtprintf( "%s: %s   %s", inOut[ si], bcx, hStr);

	return desc;
}		// MSRAT::ms_SurfBCDesc

#ifdef wanted		// no uses 12-1-91
w //=================================================================
w LOCAL void FC NEAR opsDumpI( ppp, tag, pOps)
w
w /* generate a formatted printout of a 24 hour Operation Schedule */
w
w char **ppp;	/* pgpak PAGE pointer pointer */
w char *tag;	/* identifying tag, 1st 7 chars printed */
w float *pOps;	/* pointer to vector of 24 hourly values
w 		   (temperatures or internal gains) */
w
w /* Display format has leading \n only and is 79 chars wide:
w 	TAG    xx.xx xx.xx xx.xx .... (values 0 - 11)
w 	       xx.xx xx.xx xx.xx .... (values 12 - 23) */
w
{
w register SI i, bx;
SI j;
w char b[200];		/* buffers formatted line before writing */
w
w     for (j = 0; j < 2; j++)		/* loop over 12-hour rows */
w     {	 bx = sprintf( b, "%-7s", tag );	/* start line / init offset */
w        tag = "";			/* no tag on 2nd line */
w        for (i = -1; ++i < 12; )			/* loop over 12 hours */
w        {	*(b + bx++) = ' ';	/* inter-column space */
w           cvin2sBuf(		/* format value, cvpak.cpp */
w               b+bx,
w 	      (char *)(pOps+j*12+i),	/*   ptr to value */
w 	      DTFLOAT,		/*   data type */
w 	      UNNONE, 	 	/*   no units */
w 	      5,  		/*   total field width */
w 	      FMTRJ+4 );	/*   right justify, use up to 4 dec digits.
w 	      			     (note used for internal gains,
w 		 		     with values like .047) */
w           bx += 5;   		/* advance buffer offset */
w		 }
w        b[bx] = '\0';				/* terminate buffer */
w        pgw( ppp, PGLJ|PGGROW, PGCUR+1, 1, b);	/* write to pgpak PAGE */
w	}
w}	/* opsDumpI */
#endif

/*lint +e507	restore for next file */

// end of cgdebug.cpp
