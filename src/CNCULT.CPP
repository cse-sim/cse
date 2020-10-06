// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncult.cpp: cul user interface tables and functions for cnr.exe: part 1: tables and most fcns called via tables

//unresolved notes
// colJust does nothing for exportCol. take it out. 8-14-92.

// line 650: ooWarn wanted.  Is it now implemented?  7-10-92.

// TYREF's are supposed to be VEOI/VFAZLY, right? A lot were 0, changed to VEOI 10-9-92. cul.cpp doesn't check evf for TYREF.


/* there are 3 include files for cncult,2,3,4,5,6.cpp (2-92), cuz various inclusions are required first, etc:
cncult.h: use classes, globally used functions
irats.h: declarations of input record arrays (rats)
cnculti.h: cncult internal functions shared only amoung cncult,2,3,4,5,6.cpp */

/* ideas re file line numbers ...  (~1991)
   1. save in xSp frame when opened
   2. show in "ender missing" message
   3. show in "bad mass name" message later.
*/

/*-------------------------------- OPTIONS ---------------------------------*/

//BRUCEDFL  search for this re Bruce's default changes 10-28-92. Should I get Taylor to review these?
//	also in cncult6 cncult3 at least

/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>	// USI SI LI
#include <unordered_set>

#include <ancrec.h>	// record: base class for rccn.h classes
#include <rccn.h>	// TOPRATstr ZNRstr SFIstr SDIstr
#include <msghans.h>	// MH_S0400

#include <cnguts.h>	// Top CTWINDOW
#include <vrpak.h>	// isUsedVrFile 7-13-92

#include <cuparse.h>	// TYFL TYSTR evfTx
#include <cuevf.h>	// evf's & variabilities: VMHLY VSUBHRLY
#include <cul.h>	// CULT RATE KDAT TYIREF TYREF RQD
#include <cueval.h>	// cupfree cupIncRef

// declaration for this file:
#include <cncult.h>	// use classes, globally used functions
#include <irats.h>	// declarations of input record arrays (rats)
#include <cnculti.h>	// cncult internal functions shared only amoung cncult,2,3,4,5.cpp


/*---------------------------- INPUT VARIABLES ----------------------------*/

// declarations in irats.h.
// order of declaration may control order of -p info display

/* "Top" input record base/anchor (basAnc): contains top-level / one-instance-only input stuff,
   set via table cnTopCult[], as cul.cpp user input functions can store only in RATs.
   May be accessed as static Topi.member, or as TopiR.p->member. */
makAncTOPRAT( TopiR);   		// "Top" info input record's basAnc. rccn.h macro, ancrec.h template class, ancrec.cpp constructor.
TOPRAT Topi( (anc<TOPRAT> *)&TopiR, 0);  	// "Top" info input RAT's one static entry (see ratStat call in cnPreInit)
// pending: getting constructor to do ratStat().  then cnPreInit would be empty. 2-92.

// inverse objects
makAncINVERSE(IvB);

// zones, transfers, gains, meters
makAncZNI(ZiB);				// zones input info records basAnc
makAncIZXRAT( IzxiB);		// interzone transfer input records basAnc
makAncDUCTSEG( DsiB);		// duct segment input records basAnc
makAncGAIN(GniB);			// (zone) gains input records basAnc
makAncMTR(MtriB);			// Meters input records basAnc
makAncAFMTR(AfMtriB);		// Airflow meters input records basAnc

// simple HVAC and DHW (for CSE T24 residential compliance)
makAncRSYS( RSiB);			// residential HVAC system
makAncDHWMTR( WMtriB);		// DHW meters
makAncDHWSYS( WSiB);		// DHW system
makAncDHWHEATER( WHiB);		// DHW heater
makAncDHWTANK( WTiB);		// DHW tank
makAncDHWHEATREC( WRiB);	// DHW heat recovery
makAncDHWPUMP( WPiB);		// DHW pump
makAncDHWLOOP( WLiB);		// DHW loop
makAncDHWLOOPSEG( WGiB);	// DHW loop segment
makAncDHWLOOPBRANCH( WBiB);	// DHW loop branch
makAncDHWLOOPPUMP( WLPiB);	// DHW loop pump
makAncDHWHEATER2(WLHiB,"DHWLoopHeater");		// DHW loop heater (aka "swing tank")
makAncDHWDAYUSE( WDUiB);	// DHW daily use
makAncDHWUSE( WUiB);		// DHW use (single draw)

makAncDHWSOLARSYS(SWHiB);	// DHW Solar System
makAncDHWSOLARCOLLECTOR(SCiB);	// DHW Solar Collector

makAncPVARRAY(PViB);		// PV array
makAncBATTERY(BTiB);		// Battery

makAncSHADEX( SXiB);		// SHADEX external shading

// air handlers and terminals and plant
makAncTU(TuiB);				// terminals input
makAncAH(AhiB);				// air handlers input
makAncHEATPLANT(HpiB);		// HEATPLANTs input info
makAncBOILER(BlriB);		// BOILERs input info
makAncCOOLPLANT(CpiB);		// COOLPLANTs input info
makAncCHILLER(ChiB);		// CHILLERs input info
makAncTOWERPLANT(TpiB);		// TOWERPLANTs input info
// surfaces etc
makAncPRI(PriB);  			// perimeters input records basAnc
//surfaces, doors, windows: same struct: required by topSf1() and -2.
makAncSFI(SfiB);  			// surfaces input records basAnc
makAncSFI2(DriB,"door");   	// doors input records basAnc
makAncSFI2(GziB,"window"); 	// windows input records basAnc
makAncWSHADRAT(ShiB);		// shades (fins, ovh's) input records basAnc
makAncSGI(SgiB); 			// solar gain distributions input records basAnc
// construction
makAncGT(GtiB);  			// glazeTypes input info 12-94
makAncFOUNDATION(FndiB);  			// foundations input info records basAnc
makAncFNDBLOCK(FbiB);  			// foundation blocks input info records basAnc
makAncCON(ConiB);  			// constructions input info records basAnc
makAncLR(LriB);    			// layers input info records basAnc
makAncMAT(MatiB);			// materials input info records basAnc
// reports etc
makAncRFI(RfiB);  				// REPORTFILE input info
makAncRFI2(XfiB,"exportFile"); // EXPORTFILE input info
makAncRI(RiB);    				// REPORT input info
makAncRI2(XiB,"export");		// EXPORT input info
makAncCOL(RcoliB);				// REPORTCOL input info for UDT reports
makAncCOL2(XcoliB,"exportCol"); // EXPORTCOL input info for UDT exports
makAncIMPF(ImpfiB);				// IMPORTFILE input info, 2-94

makAncHDAY(HdayiB);				// HDAY input info
makAncDESCOND(DCiB);			// DESCOND input info

//=======================================================================
void FC iRatsFree()	// free record storage for all input basAncs.

// added for DLL version that must always free all its storage.
// calling this is safer than walking the tables for error exits
// believed redundant but harmless for normal exits.
{
	ZiB.free();
	IzxiB.free();
	AfMtriB.free();
	GniB.free();
	MtriB.free();
	RSiB.free();
	WMtriB.free();
	WSiB.free();
	WHiB.free();
	WLHiB.free();
	WTiB.free();
	WRiB.free();
	WPiB.free();
	WLiB.free();
	WGiB.free();
	WBiB.free();
	WLPiB.free();
	SWHiB.free();
	SCiB.free();
	PViB.free();
	BTiB.free();
	SXiB.free();
	TuiB.free();
	AhiB.free();
	HpiB.free();
	BlriB.free();
	CpiB.free();
	ChiB.free();
	TpiB.free();
	PriB.free();
	SfiB.free();
	DriB.free();
	GziB.free();
	ShiB.free();
	SgiB.free();
	GtiB.free();
	FndiB.free();
	FbiB.free();
	ConiB.free();
	LriB.free();
	MatiB.free();
	RfiB.free();
	XfiB.free();
	RiB.free();
	XiB.free();
	RcoliB.free();
	XcoliB.free();
	ImpfiB.free();
	HdayiB.free();
	DCiB.free();
}			// iRatsFree
//=======================================================================
void FC cnPreInit()		// preliminary cncult.cpp initialization [needed before showProbeNames]

// 'showProbeNames' displays member names of all registered rats for CSE -p.
// It is executed before most initialization and can be executed without doing a run.
{
	basAnc::an_SetCULTLinks();	// link record anchors to associated input language CULTs

// clear/Init the "Top input RAT" which holds once-only input parameters, set mainly via cnTopCult[]
	TopiR.statSetup( Topi, 1);	// init cncult:TopiR as basAnc with 1 static record cncult:Topi.
								// and zero record Topi and init its front members.
								// Topi can then be accessed as static structure,
								// or via gen'l routines that work thru TopiR.

// say WFDATA records have no members set by via expressions
// prevents exWalkRecs() from msging deliberately UNSET values
// (TDV values are left UNSET if no TDV file is provided and
//  and are detected if used in expressions.)
	WthrR.ba_flags |= RFNOEX;
	WthrNxHrR.ba_flags |= RFNOEX;

// note rats are now 'registered' in cse.cpp so input and run rats can be intermixed.

}			// cnPreInit



/*============== CUL Tables (Initialized Data) and Functions ==============*/


#define N	NULL		// to shorten following data
#define v	(void *)	// ..
#define nc(nck) NCHOICE(nck)	// cnglob.h macro makes full 32-bit NCHOICE value from hi-word dtypes.h constant

//==================== TABLES FOR COMMANDS IN TOP TABLE =====================

//---------------- MATERIAL (for Top table; used in layers) -----------------

static CULT matT[] = //------------------------------------- MATERIAL Cmd table
{
//    id             cs    fn               f    uc evf     ty     b  dfls    p2  ckf
//    ----------     ----- ---------------  ---- -- ------  -----  -  ------- -- ----
CULT( "matThk",      DAT,  MAT_THK,    		0,   0, VEOI,   TYFL,  0, -1.f,   N,  N),
CULT( "matCond",     DAT,  MAT_COND,   		RQD, 0, VEOI,   TYFL,  0,  0.f,   N,  N),
CULT( "matCondT",    DAT,  MAT_CONDTRAT,	0,   0, VEOI,   TYFL,  0, 70.f,   N,  N),
CULT( "matCondCT",   DAT,  MAT_CONDCT,		0,   0, VEOI,   TYFL,  0,  0.f,   N,  N),
CULT( "matSpHt",     DAT,  MAT_SPHT,   		0,   0, VEOI,   TYFL,  0,  0.f,   N,  N),
CULT( "matDens",     DAT,  MAT_DENS,   		0,   0, VEOI,   TYFL,  0,  0.f,   N,  N),
CULT( "matRNom",     DAT,  MAT_RNOM,   		0,   0, VEOI,   TYFL,  0, -1.f,   N,  N),
CULT( "endMaterial", ENDER, 0,              0,   0, 0,      0,     0,  0.f,   N,  N),
CULT()
};		// matT

//------------------------ LAYER (for CONSTRUCTION) -------------------------

//------------------------------------------------------------ LAYER check fcn
LOCAL RC FC lrStarCkf( CULT *c, /* LR* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// LAYER check function.  only arg 'p' used.

// called at end layer, to get error messages near source.

// not called at run, but topLr appears to contain same check 12-91.
{
	LR* P = (LR *)p;
	RC rc = RCOK;
	UCH *fs = (UCH *)p + LriB.sOff;		// field status byte array

// error if one only of framing fraction and framing material given

	if (!defTyping)					// omit msg when defining a type: missing info can be given in type use.
		if (fs[ LR_FRMMATI ] & FsSET)   			// if framing material given
		{
			if ( !(fs[ LR_FRMFRAC ] & FsSET) )			// if frame fraction not given
				rc |= P->oer( 					// issue object error message, cul.cpp
						   (char *)MH_S0400 );			// handle of msg "lrFrmMat given but lrFrmFrac omitted" (msghans.h)
		}
		else		// lrFrmMat not given
		{
			if (fs[ LR_FRMFRAC ] & FsVAL)   		// if frame fraction given and has been stored
				if (P->lr_frmFrac > 0.f)				// no message if 0 value
					rc |= P->oer( (char *)MH_S0401, P->lr_frmFrac );	// "lrFrmFrac=%g given but lrFrmMat omitted"
		}

	return rc;
}		// lrStarCkf

static CULT lrT[] = //--------------------- LAYER subCmd table for CONSTRUCTION
{
	// id         cs    fn            	 f              uc evf   ty       b       dfls     p2   ckf
	//----------  ----  -----------------  -------------  -- ----  -------  ------  -------  ---- ---------
	CULT( "*",	    STAR, 0,		       0,             0, 0,    0,       0,      0.f,     N,   lrStarCkf),
	CULT( "lrCon",  DAT,  LR_OWNTI,        NO_INP|RDFLIN, 0, 0,    TYIREF,  &ConiB, 0.f,     N,   N),
	CULT( "lrThk",  DAT,  LR_THK, 	       0,             0, VEOI, TYFL,    0,      0.f,     N,   N),
	CULT( "lrMat",  DAT,  LR_MATI,         RQD,           0, VEOI, TYREF,   &MatiB, 0.f,     N,   N),
	CULT( "lrFrmMat", DAT,LR_FRMMATI,      0,             0, VEOI, TYREF,   &MatiB, 0.f,     N,   N),
	CULT( "lrFrmFrac",DAT,LR_FRMFRAC,	   0,             0, VEOI, TYFL,    0,      0.f,     N,   N),
	CULT( "endLayer", ENDER, 0,            0,             0, 0,    0,       0,      0.f,     N,   N),	// missing til 12-9-93!
	CULT()
};		// lrT

//------------- CONSTRUCTION (for Top table; ref'd in surface,) -------------

//----------------------------------------------------- CONSTRUCTION check fcn
LOCAL RC FC conStarCkf( CULT *c, /*CON* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// called at end construction object, to get messages near source of error.

// topCon2, called at RUN, contains same check 12-91.

{
	CON* P = (CON *)p;
	RC rc = RCOK;
	UCH *fs = (UCH *)p + ConiB.sOff;		// field status byte array

// determine if any layers entered for this construction
	int n = 0;
	LR* lr;
	RLUP( LriB, lr)			// loop all layers RAT entries
	if (lr->ownTi==P->ss)	// note references accept no exprs ---> needn't check FsVAL for lr->coni 12-91
	{	n++;
		break;
	}

// error if both/neither .conU, layers given
	if (fs[ CON_CONU ] & FsSET)				// if conU given
	{
		if (n)						// if 1 or more layers given
			// skip if defTyping? COULD delete layers later...  unlikely.
			rc |= P->oer( (char *)MH_S0402);		// "Cannot give both conU and layers". Issue object error message, cul.cpp
	}
	else					// conU not given
		if (n==0)				// if no layers given
			if (!defTyping)		// not error during type define: conU/layers can be given when type used.
				rc |= P->oer( (char *)MH_S0403);	// "Neither conU nor any layers given"
	return rc;

#undef P
}		// conStarCkf

static CULT conT[] = //--------------------------------- CONSTRUCTION Cmd table
{
	// id     cs    fn             f   uc evf   ty     b      dfls     p2   ckf
	//------  ----- -------------  --  -- ----  -----  ----   ------   ---- ----
	CULT( "*",	    STAR, 0,	         0,  0, 0,    0,     0,     0.f,     N,   conStarCkf),
	CULT( "conU",    DAT,  CON_CONU,      0,  0, VEOI, TYFL,  0,     0.f,     N,   N),
	CULT( "layer",   RATE, 0,             0,  0, 0,    0,     &LriB, 0.f,     lrT, N),
	CULT( "endConstruction", ENDER, 0,    0,  0, 0,    0,     0,     0.f,     N,   N),
	CULT()
};		// conT

//------------- FNDBLOCK (for Foundation table) -------------

//------------------------------------------------------------ FNDBLOCK check fcn
RC FC fbStarCkf(CULT *c, /* GT* */ void *p, void *p2, void *p3) /*ARGSUSED*/
{
	RC rc = RCOK;


	return rc;
#undef P
}		// fcStarCkf

static CULT fbT[] = //---------------------------------- FNDBLOCK Cmd Table
{
	// id            cs    fn         f     uc evf   ty     b        dfls    p2 ckf
	//-------------  ----- ---------  ----  -- ----- ------ -------  ------  -- ----
	CULT("*",	     STAR, 0,         0,    0, 0,    0,     0,       N,      0.f,  N, fbStarCkf),
	CULT("fbFnd",    DAT,  FNDBLOCK_OWNTI, NO_INP | RDFLIN, 0, 0, TYIREF,&FndiB,  0.f,  N,   N),
	CULT("fbMat",    DAT,  FNDBLOCK_MATI,  RQD,  0, VEOI, TYREF, &MatiB,  N,      0.f,  N, N),
	CULT("fbX1Ref",  DAT,  FNDBLOCK_X1REF, 0,    0, VEOI, TYCH,  0,       v C_FBXREFCH_WALLINT,0.f,    N,   N),
	CULT("fbZ1Ref",  DAT,  FNDBLOCK_Z1REF, 0,    0, VEOI, TYCH,  0,       v C_FBZREFCH_WALLTOP,0.f,    N,   N),
	CULT("fbX1",     DAT,  FNDBLOCK_X1,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("fbZ1",     DAT,  FNDBLOCK_Z1,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("fbX2Ref",  DAT,  FNDBLOCK_X2REF, 0,    0, VEOI, TYCH,  0,       v C_FBXREFCH_WALLINT,0.f,    N,   N),
	CULT("fbZ2Ref",  DAT,  FNDBLOCK_Z2REF, 0,    0, VEOI, TYCH,  0,       v C_FBZREFCH_WALLTOP,0.f,    N,   N),
	CULT("fbX2",     DAT,  FNDBLOCK_X2,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("fbZ2",     DAT,  FNDBLOCK_Z2,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("endFndBlock",ENDER,0, 0,    0, VEOI, 0,     0,       N,       0.f,  N, N),
	CULT()
};		// fbT

//------------- FOUNDATION (for Top table; ref'd in surface,) -------------

//------------------------------------------------------------ FOUNDATION check fcn
RC FC fdStarCkf(CULT *c, /* GT* */ void *p, void *p2, void *p3) /*ARGSUSED*/
{
	RC rc = RCOK;


	return rc;
#undef P
}		// fdStarCkf

static CULT fdT[] = //---------------------------------- FOUNDATION Cmd Table
{
	// id            cs    fn         f     uc evf   ty     b        dfls    p2 ckf
	//-------------  ----- ---------  ----  -- ----- ------ -------  ------  -- ----
	CULT("*",	           STAR,  0,         0,    0, 0,    0,     0,       N,      0.f,  N, fdStarCkf),
	CULT("fdWlHtAbvGrd",   DAT,   FOUNDATION_WLHTABVGRD,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("fdWlDpBlwSlb",   DAT,   FOUNDATION_WLDPBLWSLB,    0,    0, VEOI, TYFL,  0,       0.f,     N,   N),
	CULT("fdFtCon",        DAT,   FOUNDATION_FTWLCONI,  0,  0, VEOI, TYREF, &ConiB,  N,      0.f,  N, N),
	CULT("fndblock",     RATE, 0,                 0,            0, 0,      0,      &FbiB, 0.f,        fbT,   N),
	CULT("endFoundation",  ENDER, 0,     0,    0, 0, 0,     0,       N, 0.f,  N,   N),
	CULT()
};		// fdT
		
//------------- GLAZETYPE (for Top table; ref'd in window,) -------------

//------------------------------------------------------------ GLAZETYPE check fcn
RC FC gtStarCkf( CULT *c, /* GT* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// called at end layer, to get error messages near source.
// recalled at run (believe unnecessary except when using a defined type, 12-94).

{
	GT* P = (GT *)p;
	RC rc = RCOK;
	UCH *fs = (UCH *)p + GtiB.sOff;			// field status byte array

// check for poly coeffs given: require [1..3]. [0] unused, [4] optional, [5] is for 0 terminator req'd by array input stuff.

	if (!defTyping)					// omit msg when defining a type: missing info can be given in type use.
	{	if (P->gt_IsASHWAT())
		{	if (!P->IsSet( GT_GTFMULT))
				P->gtFMult = 0.85f;		// default frame = 15%
		}
		else
		{	if (!P->IsSet( GT_GTDMSHGC))
				rc |= P->oer( "'gtDMSHGC' missing (required for SHGC model)");
			if (!P->IsSet( GT_GTDMRBSOL))
				rc |= P->oer( "'gtDMRBSol' missing (required for SHGC model)");
			if ( !(fs[GT_GTPYSHGC + PY4_K + 1] & FsSET))				// if [1] not given -- thus totally omitted
				rc |= P->oer( "'gtPySHGC' missing");
			else if ( !(fs[GT_GTPYSHGC + PY4_K + 2] & FsSET) || !(fs[GT_GTPYSHGC + PY4_K + 3] & FsSET) )	// if too few coeffs given
				rc |= P->oer( "'gtPySHGC': 3 or 4 comma-separated values required.");
		}
	}

// check that poly coeffs sum to 1.0, so poly 1.0 @ right angle
//  so normal SHGC expressed in gtSHGC only. 12-17-94.
	if (!rc && !P->gt_IsASHWAT())						// if no error yet (such as too few coeff given)
	{	if (fs[GT_GTPYSHGC + PY4_K + 1] & FsSET)		// if given (ok to omit when defTyping)
		{
			float t = P->gtPySHGC.k[1] + P->gtPySHGC.k[2] + P->gtPySHGC.k[3] + P->gtPySHGC.k[4];
			if (fabs(t - 1.0) > .001)
				rc |= P->oer( "Inconsistent gtPySHGC coeffiecients: sum is %g not 1.0.", t);
		}
	}

	return rc;
#undef P
}		// gtStarCkf

static CULT gtT[] = //---------------------------------- GLAZETYPE Cmd Table
{
// id      cs    fn             f      uc evf     ty    b   dfls    p2 ckf
//-------  ----- -------------  -----  -- ------  ----  -   ------  -- ----
CULT( "*",	     STAR, 0,	          0,     0, 0,      0,    0,  N,0.f,  N, gtStarCkf),
CULT( "gtSHGC",   DAT,  GT_GTSHGC,     RQD,   0, VEOI,   TYFL, 0,  N,0.f,  N, N), 	// non-shade part of SHGC: normal xmvty
CULT( "gtSMSO",   DAT,  GT_GTSMSO,     0,     0, VMHLY,  TYFL, 0,  N,1.f,  N, N), 	// shades-open multiplier. default 1.0. 6-95
CULT( "gtSMSC",   DAT,  GT_GTSMSC,     0,     0, VMHLY,  TYFL, 0,  N,0.f,  N, N),	// closed... defaults in code.
CULT( "gtFMult",  DAT,  GT_GTFMULT,    0,     0, 0,      TYFL, 0,  N,1.f,  N, N),
CULT( "gtPySHGC", DAT,  GT_GTPYSHGC+PY4_K+1,		 				// fn+1 to skip k[0]: constant part not input
									   ARRAY, 0, VEOI,   TYFL, 0,  N,0.f, v 5,N),	// [1..3] RQD in code. VEOI 6-95.
CULT( "gtDMSHGC", DAT,  GT_GTDMSHGC,   0,     0, VEOI,   TYFL, 0,  N,0.f,  N, N),
CULT( "gtDMRBSol",DAT,  GT_GTDMRBSOL,  0,     0, VEOI,   TYFL, 0,  N,0.f,  N, N),
CULT( "gtU",      DAT,  GT_GTU,        0,     0, VEOI,   TYFL, 0,  N,0.f,  N, N),
CULT( "gtUNFRC",  DAT,  GT_GTUNFRC,    0,     0, VEOI,   TYFL, 0,  N,0.f,  N, N),
CULT( "gtModel",  DAT,  GT_GTFENMODEL, 0,     0, VEOI,   TYCH, 0,  v C_FENMODELCH_SHGC,0.f,    N,   N),
CULT( "gtNGlz",	  DAT,	GT_GTNGLZ,	   0,	  0, VEOI,   TYSI, 0,  v 2, 0.f,    N, N),
CULT( "gtExShd",  DAT,  GT_GTEXSHD,    0,     0, VEOI,   TYCH, 0,  v C_EXSHDCH_NONE,0.f,    N,   N),
CULT( "gtInShd",  DAT,  GT_GTINSHD,    0,     0, VEOI,   TYCH, 0,  v C_INSHDCH_NONE,0.f,    N,   N),
CULT( "gtDirtLoss",DAT, GT_GTDIRTLOSS, 0,     0, VEOI,   TYFL, 0,  N,0.f,  N, N),
CULT( "endGlazeType",ENDER, 0,         0,     0, VEOI,      0, 0,  N,0.f,  N, N),
CULT()
};		// gtT

//================= Subcommands for it, then ZONE command ===================


/*-=-=-=-=- subcommands for WINDOW, then WINDOW, for surface, for zone -=-=-=-=-

    Any number of windows allowed.

    Input to SFI (an XSURF is chained to ZNR record at RUN).

    If shading (overhang or fin) specified, input to WSHADRAT and store
       reference (subscript) in SFI.  One WSHADRAT entry holds overhang,
       left fin, and/or right fin, and also window height and width, which
       are not used if no shading. */

//-------------------- SGDIST, for WINDOW, for SURFACE ---------------------

//---------------------------------------------- SGDIST pre-input fcn: limit #
// note: pre-input not Itf used so ownTi already set in RAT for errmsg. Otherwise, worked fine as Itf.
LOCAL RC sgStarPrf( CULT *c, /*SGDIST* */ void *p, /*SFI* */ void *p2, void *p3) /*ARGSUSED*/
{
	/* check for too many sgdists for same window during input, for clear msg
	   at offending input file line.  Sgdist's are rechecked and stored at RUN. */

	// count sgdists for same window.  Current one will be included.
	SGI* sg;
	int n=0;
	RLUP( SgiB, sg)			// loop SGI records
		if (sg->ownTi==((SFI *)p2)->ss)	// if this sgdist for same window (FsVAL check not needed for TYIREF)
			n++;

	// error message if too many
	if (n > HSMXSGDIST)						// HSMXSGDIST (cnguts.h) = 8, 2-95.
		return ((record*)p)->oer( (char *)MH_S0404, (INT)HSMXSGDIST);	// "More than %d sgdist's for same window"
	return RCOK;
}		// sgStarPrf

static CULT sgT[] = //------------------------- SGDIST subCmd table for WINDOW
{
	// id           cs    fn                      f              uc evf     ty      b      dfls     p2   ckf
	//----------    ----  ----------------------  -------------  -- ------  ------  -----  -------  ---- ----
	CULT( "*",           STAR, 0,                      PRFP,          0, 0,      0,      0,     0.f,     v sgStarPrf,N),
	CULT( "sgWindow",    DAT,  SGI_OWNTI,              NO_INP|RDFLIN, 0, 0,      TYIREF, &GziB, 0.f,     N,   N),
	CULT( "sgSurf",      DAT,  SGI_D+SGDIST_TARGTI,RQD,               0, VEOI,   TYREF,  &SfiB, 0.f,     N,   N),
	CULT( "sgSide",      DAT,  SGI_SGSIDE,             0,             0, VEOI,   TYCH,   0,     0.f,     N,   N),
	/* .targTy is set to SGDTTMASSI/SGDTTQUIKI or SGDTTMASSO/SGDTTQUIKO per sgSide
	   (default, for now 2-91: whichever side of mass is in window's zone) */
	CULT( "sgFSO",       DAT,  SGI_D + SGDIST_FSO, RQD,               0, VMHLY,  TYFL,   0,     0.f,     N,   N),
	CULT( "sgFSC",       DAT,  SGI_D + SGDIST_FSC, 0,                 0, VMHLY,  TYFL,   0,     0.f,     N,   N),
	CULT( "endSgdist",   ENDER,0,                      0,             0, 0,      0,      0,     0.f,     N,   N),
	CULT()
};	// sgT

//-------------------- SHADE, for WINDOW, for SURFACE. ---------------------

/* shading processing includes
    here (ckf? resurrect ONLY1?) disallow > 1 shading per window
    at RUN, after surfaces, before window XSURFs made
       access window, get h w (beware unsets), check tilt vertical
       check only 1 shade per window
       make run record, set window.x.iwshad.
*/

static CULT shT[] = //------------------------------------- SHADE command table
{
	// id         cs   fn                   f              uc evf    ty     b      dfls      p2   ckf
	//----------  ---- ------------------   -------------  -- -----  -----  -----  -------   ---- ----
	CULT( "shWindow",    DAT, WSHADRAT_OWNTI,      NO_INP|RDFLIN, 0, 0,     TYIREF,&GziB, 0.f,      N,   N),
// overhang
	CULT( "ohDepth",     DAT, WSHADRAT_OHDEPTH,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "ohDistUp",    DAT, WSHADRAT_OHDISTUP,   0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "ohExL",       DAT, WSHADRAT_OHEXL,      0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "ohExR",       DAT, WSHADRAT_OHEXR,      0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "ohFlap",      DAT, WSHADRAT_OHFLAP,     0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
// left fin
	CULT( "lfDepth",     DAT, WSHADRAT_LFDEPTH,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "lfTopUp",     DAT, WSHADRAT_LFTOPUP,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "lfDistL",     DAT, WSHADRAT_LFDISTL,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "lfBotUp",     DAT, WSHADRAT_LFBOTUP,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
// right fin
	CULT( "rfDepth",     DAT, WSHADRAT_RFDEPTH,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "rfTopUp",     DAT, WSHADRAT_RFTOPUP,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "rfDistR",     DAT, WSHADRAT_RFDISTR,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
	CULT( "rfBotUp",     DAT, WSHADRAT_RFBOTUP,    0,             0, VMHLY, TYFL,  0,     0.f,      N,   N),
//
	CULT( "endShade",   ENDER,0,                   0,             0, 0,     0,     0,     0.f,      N,   N),
	CULT()
};	// shT


//----------------- WINDOW itself, for SURFACE, for ZONE. ------------------

/* window processing (1992?) includes:
     at RUN, after surfaces, before shades:
	 beware UNSETs in wnSCSO hite wid wnU
	 calculate area (could be in gzCkf).
         net out area in RUN surface.
         get azm/tilt from surface.
         default H's from surface.
	 calc derived H's -- like surface
         check wnSCSC <= SCSO or
	 default wnSCSC from wnSCSO

         then do shades: they use window tilt, set window x.iwshad

         after shades (for iwshad), make XSURFs
*/
//-----------------------------------------------------------
RC SFI::sf_CkfWINDOW(
	int options)		// option bits
						//   1: caller is sf_TopSf1 (as opposed to CULT)
{
	options;
	RC rc = RCOK;
	sfClass = sfcWINDOW;
	FixUp();
#if defined( _DEBUG)
	Validate();
#endif
	x.xs_modelr = C_SFMODELCH_QUICK;
	return rc;
}	// SFI::sf_ChkWINDOW
//------------------------------------------------------------------------
RC FC wnStarCkf( CULT *c, /*SFI* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// called at end of window object entry.
// ALSO called from topSf1() at RUN, to issue msgs otherwise lost
// if window is part of DEFTYPE ZONE and not ALTERed at TYPE use
//   (suppress msgs here during deftype if correctible at TYPE use).
{
	int options = c == NULL;	// sf_topSf1 call has no CULT
	return ((SFI*)p)->sf_CkfWINDOW( options);
}		// wnStarCkf

static CULT wnT[] = //----------------------------------- WINDOW command table
{
// id       cs    fn                f              uc evf     ty      b      dfls          p2   ckf
//--------- ----- ---------------   -------------- -- ------  ------  -----  ------------  ---- ----
CULT( "*",         STAR, 0,           0,           0, 0,      0,      0,     0.f,          N,   wnStarCkf),
CULT( "gzXtype",   DAT,  SFX( TY),    NO_INP,      0, 0,      TYSI,   0,     CTWINDOW,     N,   N),
CULT( "gzSurf",    DAT,  SFI_OWNTI,   NO_INP|RDFLIN,0,0,      TYIREF, &SfiB, 0.f,          N,   N),
// members
//area, if given, overrides, so old test files can be used 2-91.
CULT( "wnArea",    DAT,  SFI_SFAREA,  0,           0, VEOI,   TYFL,   0,     0.f,          N,   N),
CULT( "wnWidth",   DAT,  SFI_WIDTH,   RQD,         0, VEOI,   TYFL,   0,     0.f,          N,   N),
CULT( "wnHeight",  DAT,  SFI_HEIGHT,  RQD,         0, VEOI,   TYFL,   0,     0.f,          N,   N),
CULT( "wnMult",    DAT,  SFI_MULT,    0,           0, VEOI,   TYFL,   0,     1.f,        N,   N),
CULT( "wnGt",      DAT,  SFX( GTI),   0,           0, VEOI,   TYREF,  &GtiB, 0.f,        N,   N),
CULT( "wnSHGC",    DAT,  SFX( SHGC),  0,           0, VEOI,   TYFL,   0,     0.f,        N,   N),
CULT( "wnFMult",   DAT,  SFX( FMULT), 0,           0, VEOI,   TYFL,   0,     1.0f,       N,   N),
CULT( "wnUNFRC",   DAT,  SFX( UNFRC), 0,           0, VEOI,   TYFL,   0,     0.f,        N,   N),
CULT( "wnModel",   DAT,  SFX( FENMODEL), 0,        0, 0,      TYCH,   0,     C_FENMODELCH_SHGC,    N,   N),
CULT( "wnNGlz",    DAT,	 SFX( NGLZ),  0,	       0, VEOI,   TYSI,   0,     2,          N, N),
CULT( "wnExShd",   DAT,  SFX( EXSHD), 0,           0, VEOI,   TYCH,   0,     C_EXSHDCH_NONE,    N,   N),
CULT( "wnInShd",   DAT,  SFX( INSHD), 0,           0, VEOI,   TYCH,   0,     C_INSHDCH_NONE,    N,   N),
CULT( "wnDirtLoss",DAT,  SFX( DIRTLOSS), 0,        0, VEOI,   TYFL,   0,     0.f,       N,   N),
CULT( "wnU",       DAT,  SFI_SFU,     0,           0, VEOI,   TYFL,   0,     0.f,       N,   N),
CULT( "wnInH",     DAT,  SFX( UI),    0,           0, VEOI,   TYFL,   0,     10000.f,   N,   N),//BRUCEDFL was dflt'd in code
CULT( "wnExH",     DAT,  SFX( UX),    0,           0, VEOI,   TYFL,   0,     0.f,       N,   N),
CULT( "wnExEpsLW", DAT,  SFXO( EPSLW),0,           0, VEOI,   TYFL,   0,     .84f,      N,   N),
CULT( "wnInEpsLW", DAT,  SFXI( EPSLW),0,           0, VEOI,   TYFL,   0,     .84f,      N,   N),
CULT( "wnExHcModel",DAT, SFXO( HCMODEL),0,         0, 0,      TYCH,   0,     C_CONVMODELCH_UNIFIED,   N,   N),
CULT( "wnExHcLChar",DAT, SFXO( HCLCHAR),0,         0, VEOI,   TYFL,   0,     10.f,                   N,   N),
CULT( "wnExHcMult", DAT, SFXO( HCMULT), 0,         0, VSUBHRLY,TYFL,  0,      1.f,                   N,   N),
CULT( "wnInHcModel",DAT, SFXI( HCMODEL),0,         0, 0,      TYCH,   0,     C_CONVMODELCH_UNIFIED,   N,   N),
CULT( "wnInHcMult", DAT, SFXI( HCMULT), 0,         0, VSUBHRLY,TYFL,  0,      1.f,                   N,   N),
CULT( "wnSMSO",    DAT,  SFX( SCO),    0,          0, VMHLY,  TYFL,   0,     1.f,          N,   N),	// dfls to gtSMSO in code
CULT( "wnSMSC",    DAT,  SFX( SCC),    0,          0, VMHLY,  TYFL,   0,     0.f,          N,   N),	// dfls in code
CULT( "wnGrndRefl",DAT,  SFX( GRNDREFL), 0,        0, VMHLY,  TYFL,   0,     0.f,          N,   N),	// dfl'd by code. 5-95.
CULT( "wnVfSkyDf", DAT,  SFX( VFSKYDF),  0,        0, VMHLY,  TYFL,   0,     0.f,          N,   N),	// dfl'd by code. 5-95.
CULT( "wnVfGrndDf",DAT,  SFX( VFGRNDDF), 0,        0, VMHLY,  TYFL,   0,     0.f,          N,   N),	// dfl'd by code. 5-95.
	// and: area: calculated. azm/tilt: from surface.  H's default from surface.  wnU/FMult default from gt 12-94.
	// subobjects
CULT( "sgdist",    RATE, 0,                 0,            0, 0,      0,      &SgiB, 0.f,        sgT,   N),
CULT( "shade",     RATE, 0,                 0,            0, 0,      0,      &ShiB, 0.f,        shT,   N),
CULT( "endWindow", ENDER,0,                 0,            0, 0,      0,      0,     0.f,          N,   N),
CULT()
};	// wnT

//---------------------- DOOR, for surface, for zone ------------------------
RC SFI::sf_CkfDOOR(
	int options)		// option bits
						//   1: caller is sf_TopSf1 (as opposed to CULT)
{
	options;
	sfClass = sfcDOOR;		// assign class
	FixUp();
#if defined( _DEBUG)
	Validate(0x100);
#endif

	RC rc = RCOK;

// sf_Ckf: require construction or u value, not both

	BOO consSet = IsSet( SFI_SFCON);
	BOO uSet = IsSet( SFI_SFU);
	if (!consSet && !uSet && !defTyping)
		rc |= oer( (char *)MH_S0405);  		// "Neither drCon nor drU given".  oer: cul.cpp.
	else if (consSet && uSet)
		rc |= oer( (char *)MH_S0406); 		// "Both drCon and drU given"
	// getting uval from construction: defer to topCkf.

	x.xs_modelr = C_SFMODELCH_QUICK;

	return rc;
}		// SFI::sf_CkfDOOR
//----------------------------------------------------------------------------=
RC FC drStarCkf( CULT *c, /*SFI* */ void* p, void *p2, void *p3)

// called at end of door object entry.
// ALSO called from sf_topSf1() at RUN, to issue msgs otherwise lost
// if door is part of DEFTYPE ZONE and not ALTERed at TYPE use
//  (suppress msgs here during deftype if correctible at TYPE use).
{
	int options = c == NULL;	// sf_topSf1 call has no CULT
	return ((SFI *)p)->sf_CkfDOOR( options);
}		// drStarCkf
//----------------------------------------------------------------------------
static CULT drT[] =
{
// id       cs    fn                    f              uc evf     ty     b       dfls                 p2   ckf
//--------  ----- -------------------   -------------  -- ------  -----  ------  -----------------    ---- ----
CULT( "*",         STAR, 0,             0,             0, 0,      0,     0,      0.f,                 N,   drStarCkf),
CULT( "drXtype",   DAT,  SFX( TY),      NO_INP,        0, 0,      TYSI,  0,      CTEXTWALL,           N,   N),
CULT( "drSurf",    DAT,  SFI_OWNTI,     NO_INP|RDFLIN, 0, 0,      TYIREF,&SfiB,  0.f,                 N,   N),
//
CULT( "drArea",    DAT,  SFI_SFAREA,    RQD,           0, VEOI,   TYFL,  0,      0.f,                 N,   N),
CULT( "drModel",   DAT,  SFX( MODEL),   0,             0, VEOI,   TYCH,  0,      C_SFMODELCH_AUTO,    N,   N),
CULT( "drCon",     DAT,  SFI_SFCON,     0,             0, VEOI,   TYREF, &ConiB,  0.f,                N,   N),
CULT( "drU",       DAT,  SFI_SFU,       0,             0, VEOI,   TYFL,  0,      0.f,                 N,   N),
CULT( "drLThkF",   DAT,	 SFX( LTHKF),   0,			   0, VEOI,   TYFL,  0,	    .5f,				  N,   N),
CULT( "drInH",     DAT,  SFX( UI),      0,             0, VEOI,   TYFL,  0,      0.f,                 N,   N),
CULT( "drExH",     DAT,  SFX( UX),      0,             0, VEOI,   TYFL,  0,      0.f,                 N,   N),
CULT( "drExAbs",   DAT,  SFXO( ABSSLR), 0,             0, VMHLY,  TYFL,  0,      .5f,                 N,   N),
CULT( "drExRf",    DAT,  SFX( RF),      0,             0, VEOI,   TYFL,  0,       1.f,                N,   N),
CULT( "drInAbs",   DAT,  SFXI( ABSSLR), 0,             0, VMHLY,  TYFL,  0,      .5f,                 N,   N),//dfltd in code
CULT( "drExEpsLW", DAT,  SFXO( EPSLW),  0,             0, VEOI,   TYFL,  0,      .9f,                 N,   N),
CULT( "drInEpsLW", DAT,  SFXI( EPSLW),	0,             0, VEOI,   TYFL,  0,      .9f,                 N,   N),
CULT( "drExHcModel",DAT, SFXO( HCMODEL),0,             0, 0,      TYCH,  0,      C_CONVMODELCH_UNIFIED,N,   N),
CULT( "drExHcLChar",DAT, SFXO( HCLCHAR),0,             0, VEOI,   TYFL,  0,     10.f,                 N,   N),
CULT( "drExHcMult", DAT, SFXO( HCMULT),	0,             0, VSUBHRLY,TYFL, 0,      1.f,                 N,   N),
CULT( "drInHcModel",DAT, SFXI( HCMODEL),0,             0, 0,      TYCH,  0,      C_CONVMODELCH_UNIFIED,N,   N),
CULT( "drInHcMult", DAT, SFXI( HCMULT),	0,             0, VSUBHRLY,TYFL, 0,      1.f,                 N,   N),

// and: azm/tilt: from surface.  H's default from surface.
CULT( "endDoor",  ENDER, 0,             0,             0, 0,      0,     0,      0.f,                 N,   N),
CULT()
};	// drT

//----------------------- SURFACE command (for zone) ------------------------
//     Any number of surfaces allowed.
//     Input now to SFI, later chain XSURFs to ZNR record.
//-----------------------------------------------------------
RC SFI::sf_CkfSURF(		// surface checker
	int options)		// option bits
						//   1: caller is sf_TopSf1 (as opposed to CULT)
{
	BOOL bRunCheck = (options&1) != 0;
	sfClass = sfcSURF;
	if (options & 1)
	{
		FixUp();
#if defined( _DEBUG)
		Validate(0x100);
#endif
	}

	RC rc = RCOK;
	BOO xcSet = IsSet( SFX( SFEXCND)); // nz if sfExCnd given by user (a choice type: no exprs

// sf_CkfSURF: sfTilt checks and default
	float t;   					// receives tilt if defaulted or constant given
	if (IsVal( SFX( TILT)))		// if sfTilt value stored (not if expr not eval'd yet)
	{	t = x.tilt;					// fetch sfTilt
		switch (sfTy)				// check sfTilt
		{
		case C_OSTYCH_WALL:
			// note ABOUT0 (cnglob.h) is const tolerance for float compares.
			// compare in degrees for smaller (but enuf) effective tolerance.
			if (DEG(t) <= 60.f-ABOUT0  ||  DEG(t) >= 180.f+ABOUT0)
				rc |= ooer( SFX( TILT), (char *)MH_S0408, DEG(t) );	// "Wall sfTilt = %g: not 60 to 180 degrees"
			break;
		case C_OSTYCH_FLR:
			// note fAboutEqual (cnglob.h) compares for fabs(difference) < ABOUT0
			if (!fAboutEqual(DEG(t),180.f)) 				// no msg if 180
				rc |= ooer( SFX( TILT), (char *)MH_S0409 );	// "sfTilt (other than 180 degrees) may not be specified for a floor"
			break;
		case C_OSTYCH_CEIL:
			if (t < 0.f  ||  DEG(t) > 60.f+ABOUT0)
				rc |= ooer( SFX( TILT), (char *)MH_S0410, DEG(t) );	// "Ceiling sfTilt = %g: not 0 to 60 degrees"
			break;
			//[case C_OSTYCH_IM1:   case C_OSTYCH_IM2:]
		default:
			break;		// [tilt optional & unrestricted]
		}
	}
	else if (!(IsSet( SFX( TILT))))			// if sfTilt not entered (neither expr nor value stored)
	{
		switch (sfTy)						// default sfTilt, into t
		{
		case C_OSTYCH_FLR:
			t = RAD(180.f);
			break;
		case C_OSTYCH_CEIL:
			t = RAD(0.f);
			break;
			//case C_OSTYCH_WALL: //[case C_OSTYCH_IM1: case C_OSTYCH_IM2:]
		default:
			t = RAD(90.f);
			break;
		}
		x.tilt = t;					// store defaulted sfTilt
		fStat( SFX( TILT)) |= FsVAL;	// say there is now a value in .x.tilt
										//  (also in t, used below)
	}
	// else: expr entered for tilt not eval'd yet; t is unset.

// sf_CkfSURF: disallow or require azimuth
	int xc = x.sfExCnd;			// exterior conditions handy copy (choice = no exprs)
	int azmSet = IsSet( SFX( AZM));
	switch (sfTy)				// check azm per surface type
	{
	case C_OSTYCH_FLR:
		if (azmSet)
			rc |= ooer( SFX( AZM), (char *)MH_S0411); 	// "sfAzm may not be given for floors [and intmasses]"
		break;

	default:
		// C_OSTYCH_CEIL / WALL: azm needed if above grade and not horizontal
		if (!azmSet && !defTyping)	// no missing data messages at type define
		{	int azmNeeded =
			   xc != C_EXCNDCH_GROUND
				&& IsVal( SFX( TILT))		// tilt has a value (in t)
				&& ( !fAboutEqual(DEG(t),0.f) && !fAboutEqual(DEG(t),180.f) );   	// if tilted surface
			if (azmNeeded)
				rc |= ooer( SFX( AZM), (char *)MH_S0412);		// "No sfAzm given for tilted surface"
		}
		break;
	}

// sf_CkfSURF: default sfInH, sfInAbs, and adjacent-zone sfExAbs
	float defAbs = .6f;		// default interior absorptivity value. preset to interior wall value.
	switch (sfTy)
	{
	case C_OSTYCH_FLR:
		defAbs = .8f;
		goto dflInH;			// go default sfInH, 10-28-92
	case C_OSTYCH_CEIL:
		defAbs = .2f;
dflInH:
		if (!(IsSet(SFX( UI))))		// default sfInH for floor or ceiling
			x.uI = 1.32f;			// to 1.32 not value in CULT table. 10-28-92 BRUCEDFL.
		break;
		//case C_OSTYCH_WALL: //[case C_OSTYCH_IM1: case C_OSTYCH_IM2:]   wall abs preset; CULT sfInH default correct for walls.
	default:
		break;
	}
	if (!(IsSet(SFXI( ABSSLR))))		// if sfInAbs not given
		x.xs_AbsSlr( 0) = defAbs;		// default sfInAbs per sfType per above switch

	if (!(IsSet(SFXO( ABSSLR))))		// if sfExAbs not given
		if (xc==C_EXCNDCH_ADJZN)						// if zone "outside" wall (else RQD or CULT table has default)
			x.xs_AbsSlr( 1) = defAbs;				// default sfExAbs per sfType per above switch for interior wall

	if (!IsSet( SFXO( HCMODEL)) && xc == C_EXCNDCH_ADJZN)
		x.xs_sbcO.sb_hcModel = C_CONVMODELCH_UNIFIED;	// provisional sb_hcModel
														//   may be changed for child surfaces
														//   to same as parent

// sf_CkfSURF: check surface model for consistency with other parameters
	BOO consSet = IsSet( SFI_SFCON );
	if (SFI::sf_IsDelayed( x.xs_model))
	{	if (!consSet && !defTyping)				// 	(MH_S0513 also used in cncult3)
			ooer( SFI_SFCON, (char *)MH_S0513,	// "Can't use delayed (massive) sfModel=%s without giving sfCon"
				getChoiTx( SFX( MODEL)));
	}
	// else
	//	other/redundant checks in sf_TopSf1

// sf_CkfSURF: require construction or u value, not both
	BOO uSet = IsSet( SFI_SFU);
	if (!consSet && !uSet && !defTyping)
		rc |= oer( (char *)MH_S0417);  	// "Neither sfCon nor sfU given"
	else if (consSet && uSet)
		rc |= oer( (char *)MH_S0418);   // "Both sfCon and sfU given"
										// getting uval from construction: defer to topCkf.

	// sfExAbs needed unless adiabatic; tentatively don't bother disallowing 2-95.
	// former MH_S0420 probably now unused 2-95.

// sf_CkfSURF: require sfExT if ext cond is SpecT else disallow
	BOO xtSet = IsSet( SFX( SFEXT));  		// nz if sfExT entered
	if (xc==C_EXCNDCH_SPECT)
	{	if (!xtSet && !defTyping)
			ooer( SFX( SFEXT), (char *)MH_S0422);  	// "sfExCnd is SpecifiedT but no sfExT given"
	}
	else if (xtSet)
		oWarn( (char *)MH_S0423);		// "sfExT not needed when sfExCnd not 'SpecifiedT'".
										//   Do an ooWarn: warn once?

// sf_CkfSURF: Set xs_modelr and kiva related checks
	if (IsSet(SFI_SFFND) || IsSet(SFI_SFFNDFLOOR))
	{
		if (IsSet(SFI_SFFND) && sfTy != C_OSTYCH_FLR)
		{
			oer("Only floor surfaces are allowed to reference 'sfFnd'. <> Is not a floor."); // TODO add formatted string
		}

		if (IsSet(SFI_SFFNDFLOOR) && sfTy != C_OSTYCH_WALL)
		{
			oer("Only wall surfaces are allowed to reference 'sfFndFloor'. <> Is not a wall."); // TODO add formatted string
		}

		if (IsSet(SFI_SFFND) && !IsSet(SFI_SFEXPPERIM))
		{
			oer("'sfExpPerim' must be set for foundaiton floors (i.e., when 'sfFnd' is set)."); // TODO add formatted string
		}

		// TODO: Checks for exposed perimeter...
		// TODO: Check depthBG/height
		x.xs_modelr = C_SFMODELCH_KIVA;
		if (x.xs_modelr != x.xs_model && x.xs_model != C_SFMODELCH_AUTO)
		{
			oWarn("Surface reset to sfModel=Kiva"); // TODO add formatted string
		}
		// TODO Warn/ERR if not C_EXCNDCH_GROUND
	}
	else
	{
		x.xs_modelr = C_SFMODELCH_QUICK;	// tentatively set resolved model to "quick"
	}

// sf_CkfSURF: if GROUND, use separate processing (below)
	if (xc == C_EXCNDCH_GROUND)
		rc |= sf_CkfSURFGround( options);	// below grade or on grade floor, below grade wall
	else if (!bRunCheck)
	{	// not ground: msg any ground-related inputs as ignored
		//   skip at RUN, avoid dup messages
		ignoreN( "when sfExCnd not GROUND", SFXO( CTADBAVGYR), SFXO( CTADBAVG31),
			SFXO( CTADBAVG14), SFXO( CTADBAVG07), SFXO( CTGRND), SFXO( RCONGRND),
			SFX( DEPTHBG), SFX( HEIGHT), SFI_SFFND, SFI_SFEXPPERIM, SFI_SFFNDFLOOR, 0);
	}

// sf_CkfSURF: require sfAdjZn if ext cond is Zone else disallow
	BOO ajzSet = IsSet( SFX( SFADJZI) );    	// nz if adjacent zone entered
	if (xcSet)						// if ext cond set: insurance
	{
		if (xc==C_EXCNDCH_ADJZN)
		{
			if (!ajzSet && !defTyping)
				ooer( SFX( SFADJZI), (char *)MH_S0425);  	// "sfExCnd is Zone but no sfAdjZn given"
		}
		else if (ajzSet)
			oWarn( (char *)MH_S0426);			// "sfAdjZn not needed when sfExCnd not 'Zone'". ooWarn?
		// shouldn't we use more warnings?
	}
#if defined( _DEBUG)
	Validate();
#endif

	return rc;
}		// SFI::sf_CkfSURF
//-----------------------------------------------------------------------------
RC SFI::sf_CkfSURFGround(
	int options)		// option bits
						//   1: caller is sf_TopSf1 (as opposed to CULT)
{
	BOOL bRun = (options & 1) != 0;
	RC rc = RCOK;

	const char* whenGROUND = "when sfExCnd=GROUND";

	// sfExH unused for GROUND
	if (!bRun)
		ignoreN(whenGROUND, SFX(UX), 0);	// notify user
	x.uX = 0.f;		// overwrite default (1.5)

	if (sf_IsKiva(x.xs_modelr)) {
		ignoreN("when sfExCnd=GROUND and sfFnd or sfFndFloor are set.", SFX(DEPTHBG), 
			SFXO(CTADBAVGYR), SFXO(CTADBAVG31),	SFXO(CTADBAVG14), SFXO(CTADBAVG07), 
			SFXO(CTGRND), SFXO(RCONGRND), 0);
		return rc;
	}
	else
	{
		//oInfo("sfExCnd=GROUND without defining sfFnd or sfFndFloor for this surface.\n"
		//	"    Using simple ground model instead of two-dimensional finite difference.");
		ignoreN("when sfExCnd=GROUND and neither sfFnd or sfFndFloor is set.",
			SFX(HEIGHT), SFI_SFFND, SFI_SFEXPPERIM, SFI_SFFNDFLOOR, 0);
	}

	if (!sf_IsFD( x.xs_model))
		rc |= oer( "sfModel=%s not supported %s -- must be sfModel=FD",
				getChoiTx( SFX( MODEL)), whenGROUND);

	// check for unresolved expressions
	int allVals = IsValAll( SFX( DEPTHBG), SFXO( CTADBAVGYR), SFXO( CTADBAVG31),
					SFXO( CTADBAVG14), SFXO( CTADBAVG07), SFXO( CTGRND), SFXO( RCONGRND), 0);
	if (!allVals || x.xs_sbcO.sb_groundModel != 0)
		return rc;	// can't check or already checked or not relevant

	float cGrnd = x.xs_sbcO.sb_CGrnd();	// total conductance to ground, Btuh/ft2-F
	float rGrnd = 0.f;
	if (cGrnd < .0000001)
	{	// no coefficients provided
		//   will use correlations to set coefficients
		if (x.xs_depthBG == 0.f)	// if no depth given
			rc |= oer( "sfExCnd is GROUND but all ground couplings are 0");
		else
			rGrnd = -1.f;
		x.xs_sbcO.sb_groundModel = C_GROUNDMODELCH_D2COR;	// model now known
	}
	else
	{	if (x.xs_sbcO.sb_groundModel == 0 && x.xs_depthBG > 0.f)
			ignoreInfo( SFX( DEPTHBG), "when sfExCnd is GROUND and ground couplings are specificed");
		rGrnd = 1.f/cGrnd;
		if (x.xs_sbcO.sb_rConGrnd > rGrnd-.0001)
			rc |= oer( "sfExRConGrnd (%.2f) must be less than overall"
					   " ground coupling resistance (%0.2f)",
					   x.xs_sbcO.sb_rConGrnd, rGrnd);
		x.xs_sbcO.sb_groundModel = C_GROUNDMODELCH_D2INP;	// model now known
	}
	x.xs_sbcO.sb_rGrnd = rGrnd;
	return rc;
}		// SFI::sf_CkfSURFGround
//-----------------------------------------------------------------------------
RC sfStarCkf( CULT *c, /*SFI* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// called at end of surface object entry.
//	ALSO called from sf_topSf1() at RUN, to issue msgs otherwise lost
//  if surface is part of DEFTYPE ZONE and not ALTERed at TYPE use
//  (suppress msgs here during deftype if correctible at TYPE use).

// ONLY argument 'p' is used.
{
	int options = c == NULL;	// sf_topSf1 call has no CULT
	return ((SFI* )p)->sf_CkfSURF( options);
}		// sfStarCkf
//-------------------------------------------------------------------------------
// SURFACE command table
static CULT sfT[] =
{
// id             cs    fn                   f              uc evf     ty      b        dfls                   p2   ckf
//-------         ----  -------------------  -------------  -- ------  ------  -------  ---------------------  ---- ----
CULT( "*",        STAR, 0,                   0,             0, 0,      0,      0,       0.f,                   N,   sfStarCkf),
CULT( "sfXtype",  DAT,  SFX( TY),            NO_INP,        0, 0,      TYSI,   0,       CTEXTWALL,             N,   N),
CULT( "sfZone",   DAT,  SFI_OWNTI,           NO_INP|RDFLIN, 0, 0,      TYIREF, &ZiB,    0.f,                   N,   N),
CULT( "sfType",   DAT,  SFI_SFTY,            RQD,           0, 0,      TYCH,   0,       0.f,                   N,   N),//sfStarCkf uses -> not VEOI
CULT( "sfArea",   DAT,  SFI_SFAREA,          RQD,           0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfTilt",   DAT,  SFX( TILT),          0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfAzm",    DAT,  SFX( AZM),           0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfModel",  DAT,  SFX( MODEL),         0,             0, 0,      TYCH,   0,       C_SFMODELCH_AUTO,      N,   N),//sfStarCkf uses
CULT( "sfDepthBG",DAT,  SFX( DEPTHBG),		 0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfHeight", DAT,  SFX( HEIGHT),		 0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfCon",    DAT,  SFI_SFCON,           0,             0, VEOI,   TYREF, &ConiB,   0.f,                   N,   N),
CULT( "sfU",      DAT,  SFI_SFU,             0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfLThkF",  DAT,	SFX( LTHKF),		 0,				0, VEOI,   TYFL,   0,		.5f,				   N,   N),
CULT( "sfInH",    DAT,  SFX( UI),            0,             0, VEOI,   TYFL,   0,       1.5f,                  N,   N),//sfStarCkf re-defaults for some sfTy's BRUCEDFL
CULT( "sfExH",    DAT,  SFX( UX),            0,             0, VEOI,   TYFL,   0,       1.5f,                  N,   N),//re-defaults to dflExH for some exConds
CULT( "sfExCnd",  DAT,  SFX( SFEXCND),       0,             0, 0,      TYCH,   0,       C_EXCNDCH_AMBIENT,     N,   N),//sfStarCkf uses
CULT( "sfExT",    DAT,  SFX( SFEXT),         0,             0, VSUBHRLY,TYFL,  0,       0.f,                   N,   N),
CULT( "sfExT",    DAT,  SFX( SFEXT),         0,             0, VHRLY,  TYFL,   0,       0.f,                   N,   N),
CULT( "sfExAbs",  DAT,  SFXO( ABSSLR),		 0,             0, VMHLY,  TYFL,   0,       .5f,                   N,   N),
CULT( "sfExRf",   DAT,  SFX( RF),            0,             0, VEOI,   TYFL,   0,       1.f,                   N,   N),
CULT( "sfInAbs",  DAT,  SFXI( ABSSLR),		 0,             0, VMHLY,  TYFL,   0,       .5f,                   N,   N),//dfltd in code
CULT( "sfExEpsLW",DAT,  SFXO( EPSLW),		 0,             0, VEOI,   TYFL,   0,       .9f,                   N,   N),
CULT( "sfInEpsLW",DAT,  SFXI( EPSLW),		 0,             0, VEOI,   TYFL,   0,       .9f,                   N,   N),
CULT( "sfExHcModel",DAT,SFXO( HCMODEL),	     0,             0, 0,      TYCH,   0,      C_CONVMODELCH_UNIFIED,  N,   N),
CULT( "sfExHcLChar",DAT,SFXO( HCLCHAR),	     0,             0, VEOI,   TYFL,   0,       10.f,                  N,   N),
CULT( "sfExHcMult", DAT,SFXO( HCMULT),	     0,             0, VSUBHRLY,TYFL,  0,       1.f,                   N,   N),
CULT( "sfFnd",    DAT,  SFI_SFFND,           0,             0, VEOI,   TYREF, &FndiB,   0.f,                   N,   N),
CULT( "sfExpPerim", DAT,SFI_SFEXPPERIM,      0,             0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfFndFloor", DAT,SFI_SFFNDFLOOR,      0,             0, VEOI,   TYREF, &SfiB,   0.f,                    N,   N),
CULT( "sfExCTaDbAvgYr",DAT, SFXO( CTADBAVGYR),0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfExCTaDbAvg31",DAT, SFXO( CTADBAVG31),0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfExCTaDbAvg14",DAT, SFXO( CTADBAVG14),0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfExCTaDbAvg07",DAT, SFXO( CTADBAVG07),0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfExCTGrnd",    DAT, SFXO( CTGRND),    0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfExRConGrnd",  DAT, SFXO( RCONGRND),  0,            0, VEOI,   TYFL,   0,       0.f,                   N,   N),
CULT( "sfInHcModel",DAT,SFXI( HCMODEL),	     0,             0, 0,      TYCH,   0,      C_CONVMODELCH_UNIFIED,  N,   N),
CULT( "sfInHcMult", DAT,SFXI( HCMULT),	     0,             0, VSUBHRLY,TYFL,  0,       1.f,                   N,   N),
CULT( "sfAdjZn",  DAT,  SFX( SFADJZI),       0,             0, VEOI,   TYREF,  &ZiB,    0.f,                   N,   N),
CULT( "sfGrndRefl",DAT, SFX( GRNDREFL),      0,             0, VMHLY,  TYFL,   0,       0.f,                  N,   N), // dfl'd by code. 5-95.
// subobjects
CULT( "window",    RATE, 0,                  0,             0, 0,      0,      &GziB,   0.f,                 wnT,   N),
CULT( "door",      RATE, 0,                  0,             0, 0,      0,      &DriB,   0.f,                 drT,   N),
CULT( "endSurface",ENDER,0,                  0,             0, 0,      0,      0,       0.f,                   N,   N),
CULT()
};	// sfT


/*------------------------ PERIM command (for zone) ------------------------
  Any number allowed.
  Input now to PRI; later XSURFs are chained to ZNR record. */
//-----------------------------------------------------------------------------
RC prStarCkf(CULT *c, /*SFI* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// called at end of surface object entry.
//	ALSO called from sf_topSf1() at RUN, to issue msgs otherwise lost
//  if surface is part of DEFTYPE ZONE and not ALTERed at TYPE use
//  (suppress msgs here during deftype if correctible at TYPE use).

// ONLY argument 'p' is used.
{
	int options = c == NULL;	// sf_topSf1 call has no CULT
	return ((PRI*)p)->pr_Ckf(options);
}		// sfStarCkf
//-----------------------------------------------------------------------------
static CULT perT[] =
{
// id       cs    fn                   f              uc evf     ty      b      dfls                 p2  ckf
//--------  ----  -------------------  -------------  -- ------  -----  ------  -------------------- --- ----
CULT( "*",         STAR, 0,                   0,             0, 0,      0,      0,     0.f,                 N,   prStarCkf),
CULT( "*",         STAR, 0,                   0,             0, 0,      0,      0,     0.f,                 N,  N),
CULT( "prZone",    DAT,  PRI_OWNTI,           NO_INP|RDFLIN, 0, 0,      TYREF,  &ZiB,  0.f,                 N,  N), //TYIREF-->TYREF 10-9-92
CULT( "prXtype",   DAT,  PRI_X+XSURF_TY,      NO_INP,        0, 0,      TYSI,   0,    v CTPERIM,0.f,        N,  N), //CTPERIM: cnguts.h.
CULT( "prXExCnd",  DAT,  PRI_X+XSURF_SFEXCND, NO_INP,        0, 0,      TYCH,   0, v C_EXCNDCH_AMBIENT,0.f, N,  N), //added 2-95
CULT( "prLen",     DAT,  PRI_PRLEN,           RQD,           0, VEOI,   TYFL,   0,     0.f,                 N,  N),
CULT( "prF2",      DAT,  PRI_PRF2,            RQD,           0, VEOI,   TYFL,   0,     0.f,                 N,  N),
CULT( "endPerimeter",ENDER,0,                 0,             0, 0,      0,      0,     0.f,                 N,  N),
CULT()
};	// perT


/*------------------------ TERMINAL command (for zone) ----------------------*/

//-------------------------------------------- terminal pre-input fcn: limit #
// note: pre-input not Itf used so ownTi (=zi) already set in RAT for errmsg.
LOCAL RC tuPrf( CULT *c, TU *p, ZNI *p2, void *p3) /*ARGSUSED*/
{
	TU *tu;
	int n=0;

// check for too many terminals for zone.
// NOTE zone terminal tables are in ZNR (not ZNI) -- setup deferred until topTu() below.

	// count terminals for same zone.  Current one will be included.
	RLUP( TuiB, tu)			// loop TU records
		if (tu->ownTi==p2->ss)	// if terminal belongs to cur zone (reference type, need not check FsVAL 12-91)
			n++;				//    count it

	// error if too many
	if (n > MAX_ZONETUS)						// =3, cndefns.h
		return p->oer( (char *)MH_S0427, (INT)MAX_ZONETUS, p2->name );  	// "More than %d terminals for zone '%s'"

	return RCOK;
}		// tuPrf

static CULT tuT[] = //--------------------------------------- TERMINAL subCmd table for ZONE
{
#define TUHC(m)  TU_TUHC + HEATCOIL_##m		// CAUTION to documentors: most DEFAULTS are supplied by code.
#define TFAN(m)  TU_TFAN + FAN_##m
// id         cs    fn                   f              uc evf     ty      b       dfls               p2    ckf
//----------  ----  -------------------  -------------- -- ------  -----   -----   --------------  -------- ---
CULT( "*",           STAR, 0,                   PRFP,          0, 0,      0,      0,      N,        0.f,  v tuPrf, N),
CULT( "tuZone",      DAT,  TU_OWNTI,            NO_INP|RDFLIN, 0, 0,      TYIREF, &ZiB,   N,        0.f,      N,   N),
CULT( "tuTLh",       DAT,  TU_TUTLH,            0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuQMnLh",     DAT,  TU_TUQMNLH,          0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuQMxLh",     DAT,  TU_TUQMXLH,          0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
#if 0 //removing 1-4-95. later edit out stuff in cncult5.cpp and cnrecs.def.
x "tuQDsLh",     DAT,  TU_TUQDSLH,          0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N), // obsolete,warns,9-92.
#endif
CULT( "tuPriLh",     DAT,  TU_TUPRILH,          0,             0, VFAZLY, TYSI,   0,  v 100,        0.f,      N,   N),
CULT( "tuLhNeedsFlow",DAT, TU_TULHNEEDSFLOW,    0,             0, VFAZLY, TYCH,   0,  v C_NOYESCH_NO,  0.f,   N,   N),
CULT( "tuhcType",    DAT,  TUHC(COILTY),        0,             0, VFAZLY, TYCH,   0,  v C_COILTYCH_NONE, 0.f, N,   N),
CULT( "tuhcCaptRat", DAT,  TUHC(CAPTRAT),       AS_OK,         0, VFAZLY, TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuhcHeatplant",DAT, TUHC(HPI),           0,             0, VEOI,   TYREF,  &HpiB,  N,        0.f,      N,   N),
CULT( "tuhcMtr",     DAT,  TUHC(MTRI),          0,             0, VEOI,   TYREF,  &MtriB, N,        0.f,      N,   N),
CULT( "tuTH",        DAT,  TU_TUTH,             0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuTC",        DAT,  TU_TUTC,             0,             0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuVfMn",      DAT,  TU_TUVFMN,           AS_OK,         0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuAh",        DAT,  TU_AI,               0,             0, VEOI,   TYREF,  &AhiB,  N,        0.f,      N,   N),
CULT( "tuVfMxH",     DAT,  TU_TUVFMXH,          AS_OK,         0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuVfMxC",     DAT,  TU_TUVFMXC,          AS_OK,         0, VHRLY,  TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuVfDs",      DAT,  TU_TUVFDS,           0,             0, VFAZLY, TYFL,   0,      N,        0.f,      N,   N),
CULT( "tuVfMxHC",    DAT,  TU_TUVFMXHC,         0,             0, VFAZLY, TYCH,  0, v C_DIFFSAMECH_DIFF, 0.f,  N,  N), // 6-95
CULT( "tuOversize",  DAT,  TU_TUOVERSIZE,       0,             0, VFAZLY, TYFL,   0,      N,        .1f,      N,   N), // 6-95
CULT( "tuPriH",      DAT,  TU_TUPRIH,           0,             0, VFAZLY, TYSI,   0,     v 1,       0.f,      N,   N),
CULT( "tuPriC",      DAT,  TU_TUPRIC,           0,             0, VFAZLY, TYSI,   0,     v 1,       0.f,      N,   N),
CULT( "tuSRLeak",    DAT,  TU_TUSRLEAK,         0,             0, VFAZLY, TYFL,   0,      N,         .05f,    N,   N),
CULT( "tuSRLoss",    DAT,  TU_TUSRLOSS,         0,             0, VFAZLY, TYFL,   0,      N,         .1f,     N,   N), // unimpl 4-95
CULT( "tfanSched",   DAT,  TU_TFANSCH,          0,             0, VHRLY,  TYCH,   0, nc(C_TFANSCHVC_OFF), 0.f, N,   N),
CULT( "tfanOffLeak", DAT,  TU_TFANOFFLEAK,      0,             0, VFAZLY, TYFL,   0,      N,         .1f,     N,   N),
CULT( "tfanType",    DAT,  TFAN(FANTY),         0,             0, VFAZLY, TYCH,   0,  v C_FANTYCH_NONE, 0.f,  N,   N),
CULT( "tfanVfDs",    DAT,  TFAN(VFDS),          0,             0, VFAZLY, TYFL,   0,      N,        0.f,      N,   N),
CULT( "tfanPress",   DAT,  TFAN(PRESS),         0,             0, VFAZLY, TYFL,   0,      N,         .3f,     N,   N),
CULT( "tfanEff",     DAT,  TFAN(EFF),           0,             0, VFAZLY, TYFL,   0,      N,         .08f,    N,   N),
CULT( "tfanMtr",     DAT,  TFAN(MTRI),          0,             0, VEOI,   TYREF,  &MtriB, N,        0.f,      N,   N),
CULT( "tfanVfMxF",   DAT,  TFAN(VFMXF),         NO_INP,        0, 0,      TYFL,   0,      N,        1.0f,     N,   N),
CULT( "tfanMotPos",  DAT,  TFAN(MOTPOS),        NO_INP,        0, 0,      TYCH,   0,  v C_MOTPOSCH_INFLO, 0,  N,   N),
CULT( "tfanCurvePy", DAT,  TFAN(CURVEPY+PYCUBIC_K), ARRAY,     0, VFAZLY, TYFL,   0,      N,        0.f,     v 6,  N), //incl x0
CULT( "tfanMotEff",  DAT,  TFAN(MOTEFF),        NO_INP,        0, 0,      TYFL,   0,      N,        1.f,      N,   N),
CULT( "endTerminal", ENDER,0,                   0,             0, 0,      0,      0,      N,        0.f,      N,   N),
CULT()
};	// tuT


//===================================== METER command ============================================

static CULT mtrT[] = //------------------ METER cmd table, used from cnTopCult
{
	// id       cs     fn               f        uc evf    ty     b       dfls                p2   ckf
	//--------  -----  ---------------  -------  -- -----  -----  ------  ----------------    ---- ----
	//"*",        STAR,      0,           0,       0, 0,     0,     0,      N,   0.f,           N,   N),
CULT( "mtrRate",      DAT,   MTR_RATE,    0,       0, VHRLY, TYFL,  0,      N,   0.f,           N,   N),
CULT( "mtrDemandRate",DAT,   MTR_DMDRATE, 0,       0, VHRLY, TYFL,  0,      N,   0.f,           N,   N),
CULT( "endMeter",     ENDER, 0,           0,       0, 0,     0,     0,      N,   0.f,           N,   N),
CULT()
};	// mtrT


//===================================== GAIN command for ZONE ============================================
LOCAL RC FC gnStarCkf( CULT* c, /*GAIN* */ void *p, void *p2, void *p3)
// called at end of GAIN input, to get messages near source of error.
{
	return ((GAIN*)p)->gn_CkF( 0);
}		// gnStarCkf
//--------------------------------------------------------------------------------------------------------
static CULT gnT[] = //----------- GAIN cmd table, used from cnTopCult and znT
{
// id       cs     fn               f        uc evf     ty     b       dfls                p2   ckf
//--------  -----  ---------------  -------  -- ------  -----  ------  ----------------    ---- ----
CULT( "*",         STAR,  0,               0,       0, 0,      0,     0,      0,    N, gnStarCkf),
CULT( "gnZone",    DAT,   GAIN_OWNTI,      DFLIN,   0, VEOI,   TYREF, &ZiB,   N,   0.f,           N,   N),
CULT( "gnPower",   DAT,   GAIN_GNPOWER,    RQD,     0, VHRLY,  TYFL,  0,      N,   0.f,           N,   N),
CULT( "gnMeter",   DAT,   GAIN_MTRI,       0,       0, VEOI,   TYREF, &MtriB, N,   0,             N,   N),
CULT( "gnEndUse",  DAT,   GAIN_GNENDUSE,   0,       0, VFAZLY, TYCH,  0,      N,   0.f,           N,   N), // rqd if fuel != none
CULT( "gnFrLat",   DAT,   GAIN_GNFRLAT,    0,       0, VHRLY,  TYFL,  0,      N,   0.f,           N,   N),
CULT( "gnFrRad",   DAT,   GAIN_GNFRRAD,    0,       0, VHRLY,  TYFL,  0,      N,   0.f,           N,   N), // added 11-95
CULT( "gnFrZn",    DAT,   GAIN_GNFRZN,     0,       0, VHRLY,  TYFL,  0,      N,   1.f,           N,   N), // these three must
CULT( "gnFrPl",    DAT,   GAIN_GNFRPL,     0,       0, VHRLY,  TYFL,  0,      N,   0.f,           N,   N), // sum to
CULT( "gnFrRtn",   DAT,   GAIN_GNFRRTN,    0,       0, VHRLY,  TYFL,  0,      N,   0.f,           N,   N), // 1 or less at runtime
CULT( "gnDlFrPow", DAT,   GAIN_GNDLFRPOW,  0,       0, VHRLY,  TYFL,  0,      N,   1.f,           N,   N),
CULT( "gnCtrlDHWSYS",DAT, GAIN_DHWSYSI,    0,       0, VEOI,   TYREF, &WSiB,  N,   0,             N,   N),
CULT( "gnCtrlDHWEndUse",DAT,GAIN_DHWENDUSE,0,       0, VEOI,   TYCH,  0,      v C_DHWEUXCH_TOTAL, 0.f, N,   N),
CULT( "endGain",   ENDER, 0,               0,       0, 0,      0,     0,      N,   0.f,           N,   N),
CULT()
};	// gnT


//===================================== REPORTCOL command ============================================

static CULT rpColT[] = //----------- REPORTCOL cmd table, used from cnTopCult and rpT.
{
// id         cs     fn             f        uc evf       ty      b       dfls         p2   ckf
//----------  -----  -------------  -------  -- --------  -----   ------  ----------   --  ----
CULT( "*",           STAR,  0,             0,       0, 0,        0,      0,      N,   0.f,    N,   N),
CULT( "colReport",   DAT,   COL_OWNTI,     RDFLIN,  0, VEOI,     TYREF,  &RiB,   N,   0.f,    N,   N),
CULT( "colHead",     DAT,   COL_COLHEAD,   0,       0, VEOI,     TYSTR,  0,      N,   0.f,    N,   N),
CULT( "colGap",      DAT,   COL_COLGAP,    0,       0, VEOI,     TYSI,   0,     v 1,  0.f,    N,   N),
CULT( "colWid",      DAT,   COL_COLWID,    0,       0, VEOI,     TYSI,   0,      N,   0.f,    N,   N),
CULT( "colDec",      DAT,   COL_COLDEC,    0,       0, VEOI,     TYSI,   0,     v -1, 0.f,    N,   N),
CULT( "colJust",     DAT,   COL_COLJUST,   0,       0, VEOI,     TYCH,   0,      N,   0.f,    N,   N),
CULT( "colVal",      DAT,   COL_COLVAL,    RQD,     0, VSUBHRLY|EVPSTIVL,			// ok to evaluate at post interval
TYFLSTR,			// float or string, struct w/ dt.
0,      N,   0.f,    N,   N),
CULT( "endReportcol",ENDER, 0,             0,       0, 0,        0,      0,      N,   0.f,    N,   N),
CULT()
};	// rpColT


//===================================== EXPORTCOL command ============================================

static CULT exColT[] = //----------- EXPORTCOL cmd table, used from cnTopCult and exT.
{
// id         cs     fn             f        uc evf       ty      b       dfls         p2   ckf
//----------  -----  -------------  -------  -- --------  -----   ------  ----------   ---- ----
CULT( "*",           STAR,  0,             0,       0, 0,        0,      0,      N,  0.f,     N,   N),
CULT( "colExport",   DAT,   COL_OWNTI,     RDFLIN,  0, VEOI,     TYREF,  &XiB,   N,  0.f,     N,   N),
CULT( "colHead",     DAT,   COL_COLHEAD,   0,       0, VEOI,     TYSTR,  0,      N,  0.f,     N,   N),
CULT( "colWid",      DAT,   COL_COLWID,    0,       0, VEOI,     TYSI,   0,      N,  0.f,     N,   N),
CULT( "colDec",      DAT,   COL_COLDEC,    0,       0, VEOI,     TYSI,   0,     v -1, 0.f,    N,   N),
CULT( "colJust",     DAT,   COL_COLJUST,   0,       0, VEOI,     TYCH,   0,      N,  0.f,     N,   N),
CULT( "colVal",      DAT,   COL_COLVAL,    RQD,     0, VSUBHRLY|EVPSTIVL,			// ok to evaluate at post interval
														         TYFLSTR,			// float or string, struct w/ dt.
0,      N,  0.f,     N,   N),
CULT( "endExportcol",ENDER, 0,             0,       0, 0,     0,         0,      N,  0.f,     N,   N),
CULT()
};	// exColT


//===================================== REPORT command ============================================

// re/exports and re/exportfiles persist through autosize and main sim phases;
// they are not reinitialized and thus cannot have any VFAZLY members. 6-95.

static RC FC rpStarCkf( CULT *c, /*RI* */ void *p, void *p2, void *p3) /*ARGSUSED*/
// check function automatically called at end of REPORT and EXPORT object entry (per tables * lines).
// also called by code in TopCkf.
// ONLY argument 'p' is used.
{
	return ((RI *)p)->ri_CkF();
}		// rpStarCkf
//-------------------------------------------------------------------------------------------------
static CULT rpT[] = //----------- REPORT cmd table, used from cnTopCult and rpfT
{
// id         cs    fn             f                uc evf       ty     b        dfls          p2  ckf
//----------  ----- -------------  ---------------  -- -----     -----  -----   ----------    ---- ----
CULT( "*",           STAR, 0,             0,               0, 0,        0,     0,       N,   0.f,      N, rpStarCkf),
CULT( "rpReportfile",DAT,  RI_OWNTI,      DFLIN,           0, VEOI,     TYIREF,&RfiB,   N,   0.f,      N, N), // also dflt'd in cncult4. TYREF->TYIREF 10-9-92
CULT( "rpZone",      DAT,  RI_ZI,         SUM_OK|ALL_OK,   0, VEOI,     TYREF, &ZiB,    N,   0.f,      N, N), // rqd/disallowed per rpTy
CULT( "rpMeter",     DAT,  RI_MTRI,       SUM_OK|ALL_OK,   0, VEOI,     TYREF, &MtriB,  N,   0.f,      N, N),
CULT( "rpAh",        DAT,  RI_AHI,        SUM_OK|ALL_OK,   0, VEOI,     TYREF, &AhiB,   N,   0.f,      N, N),
CULT( "rpDHWMeter",  DAT,  RI_DHWMTRI,    ALL_OK,          0, VEOI,     TYREF, &WMtriB, N,   0.f,      N, N),
CULT( "rpAFMeter",   DAT,  RI_AFMTRI,     SUM_OK|ALL_OK,   0, VEOI,     TYREF, &AfMtriB,N,   0.f,      N, N),
CULT( "rpTu",        DAT,  RI_TUI,        ALL_OK,          0, VEOI,     TYREF, &TuiB,   N,   0.f,      N, N), // 6-95
CULT( "rpType",      DAT,  RI_RPTY,       RQD,             0, 0,        TYCH,  0,       v 0, 0.f,      N, N), // used in rpStarCkf: no VEOI
CULT( "rpFreq",      DAT,  RI_RPFREQ,     0,               0, 0,        TYCH,  0,       v 0, 0.f,      N, N), // used in rpStarCkf: no VEOI
CULT( "rpDayBeg",    DAT,  RI_RPDAYBEG,   0,               0, VEOI,     TYDOY, 0,       v 0, 0.f,      N, N),
CULT( "rpDayEnd",    DAT,  RI_RPDAYEND,   0,               0, VEOI,     TYDOY, 0,       v 0, 0.f,      N, N),
CULT( "rpBtuSf",     DAT,  RI_RPBTUSF,    0,               0, VEOI,     TYFL,  0,       v 0, 1e6,      N, N), // show mBtu. Also in cncult4:addRep().
CULT( "rpCond",      DAT,  RI_RPCOND,     0,               0, VSUBHRLY|EVPSTIVL,		 	// ok if evaluated at end interval
																		TYLLI, 				// SI condition, dflt TRUE, in LI for NAN
																		       0,       v 1L,0.f,      N, N),
CULT( "rpTitle",     DAT,  RI_RPTITLE,    0,               0, VEOI,     TYSTR, 0,       v 0, 0.f,      N, N),
CULT( "rpCpl",       DAT,  RI_RPCPL,      0,               0, VEOI,     TYSI,  0,       v 0, 0.f,      N, N),
CULT( "rpHeader",	 DAT,  RI_RPHEADER,   0,               0, VEOI,     TYCH,  0, v C_RPTHDCH_YES,0.f, N, N),
CULT( "rpFooter",	 DAT,  RI_RPFOOTER,   0,               0, VEOI,     TYCH,  0, v C_NOYESCH_YES,0.f, N, N),
CULT( "reportCol",   RATE, 0,             0,               0, 0,        0,    &RcoliB,  N,   0.f, rpColT, N),
CULT( "isExport",    DAT,  RI_ISEXPORT,   NO_INP,          0, 0,        TYSI,  0,       v 0, 0.f,      N, N), // set 0 for reports
CULT( "endReport",  ENDER, 0,             0,               0, 0,        0,     0,       N,   0.f,      N, N),
CULT()
};	// rpT

#if 0 // 1-20-92 make owned by reportfiles anyway
x		/* REPORTs "ownership" inconsistency from most of input language:
x		     Consistency would have reports owned by reportFiles & defaulting to use file in which embedded.
x		       But specs are: rpReportfile defaults to automatically-supplied, normally invisible "Primary".
x		       Rob judges an additional default when explicit report files present to be too complex for users.
x		     So there is no rpT entry in rpfT; since there is one in znT, cul.cpp's normal action causes
x		       reports to be owned by zones; this shows to user as error msgs about "Report ___ of Zone ___".
x		     Exports/exportFiles likewise.  rob 11-29-91. */
#endif


//===================================== EXPORT command ============================================

static CULT exT[] = //----------- EXPORT cmd table, used from cnTopCult and exfT
{
// id         cs    fn             f                uc evf       ty      b       dfls         p2   ckf
//----------  ----- -------------  ---------------  -- -----     -----  -----    ----------   ---- ----
CULT( "*",           STAR, 0,             0,               0, 0,        0,      0,      N,   0.f,    N,   rpStarCkf),	// Ckf fcn shared with REPORT *****
CULT( "exExportfile",DAT,  RI_OWNTI,      DFLIN,           0, VEOI,     TYIREF, &XfiB,  N,   0.f,    N,   N), // also dflt'd in cncult4. TYREF->TYIREF 10-9-92
CULT( "exZone",      DAT,  RI_ZI,     DFLIN|SUM_OK|ALL_OK, 0, VEOI,     TYREF,  &ZiB,   N,   0.f,    N,   N), // rqd/disallowed per rpTy
CULT( "exMeter",     DAT,  RI_MTRI,       SUM_OK|ALL_OK,   0, VEOI,     TYREF,  &MtriB, N,   0.f,    N,   N),
CULT( "exAh",        DAT,  RI_AHI,        SUM_OK|ALL_OK,   0, VEOI,     TYREF,  &AhiB,  N,   0.f,    N,   N),
CULT( "exDHWMeter",  DAT,  RI_DHWMTRI,    ALL_OK,          0, VEOI,     TYREF,  &WMtriB, N,  0.f,    N, N),
CULT( "exAFMeter",   DAT,  RI_AFMTRI,     ALL_OK,          0, VEOI,     TYREF,  &AfMtriB,N,  0.f,    N, N),
CULT( "exTu",        DAT,  RI_TUI,        ALL_OK,          0, VEOI,     TYREF,  &TuiB,  N,   0.f,    N,   N), // 6-95
CULT( "exType",      DAT,  RI_RPTY,       RQD,             0, 0,        TYCH,   0,      v 0, 0.f,    N,   N), // used in rpStarCkf: no VEOI
CULT( "exFreq",      DAT,  RI_RPFREQ,     0,               0, 0,        TYCH,   0,      v 0, 0.f,    N,   N), // used in rpStarCkf: no VEOI
CULT( "exDayBeg",    DAT,  RI_RPDAYBEG,   0,               0, VEOI,     TYDOY,  0,      v 0, 0.f,    N,   N),
CULT( "exDayEnd",    DAT,  RI_RPDAYEND,   0,               0, VEOI,     TYDOY,  0,      v 0, 0.f,    N,   N),
CULT( "exBtuSf",     DAT,  RI_RPBTUSF,    0,               0, VEOI,     TYFL,   0,      v 0, 1e6,    N,   N), // show mBtu. Also in cncult4:addRep().
CULT( "exCond",      DAT,  RI_RPCOND,     0,               0, VSUBHRLY|EVPSTIVL,		 	// ok if evaluated post interval
	                                                                    TYLLI,  		 	// SI condition, dflt TRUE, in LI for NAN
	                                                                            0,      v 1L,0.f,    N,   N),
CULT( "exTitle",     DAT,  RI_RPTITLE,    0,               0, VEOI,     TYSTR,  0,      v 0, 0.f,    N,   N),
CULT( "exHeader",	 DAT,  RI_RPHEADER,   0,               0, VEOI,     TYCH,   0, v C_RPTHDCH_YES,0.f, N, N),
CULT( "exFooter",	 DAT,  RI_RPFOOTER,   0,               0, VEOI,     TYCH,   0, v C_NOYESCH_YES,0.f, N, N),
CULT( "exportCol",   RATE, 0,             0,               0, 0,        0,      &XcoliB, N,   0.f, exColT, N),
CULT( "isExport",    DAT,  RI_ISEXPORT,   NO_INP,          0, 0,        TYSI,   0,      v 1, 0.f,    N,   N), // set 1 for export
CULT( "endExport",  ENDER, 0,             0,               0, 0,        0,      0,      N,   0.f,    N,   N),
CULT()
};	// exT


//====================================== REPORTFILE command =======================================

// initial records for following implicit commands are entered from topPrfRep.
//        REPORTFILE Primary rfFileName = <inputfile>.rep

// re/exports and re/exportfiles persist through autosize and main sim phases;
// they are not reinitialized and thus cannot have any VFAZLY members.

//-----------------------------------------------------------------------------
RC FC rfStarCkf( CULT *c, /*RFI* */ void *p, void *p2, void *p3) /*ARGSUSED*/	//-------------------------------
// check function automatically called at end of REPORTFILE object entry, also called from TopCkf.
// ONLY argument 'p' is used.
{
	return ((RFI *)p)->rf_CkF( 0);	// 0 = is report
}		// rfStarCkf

static CULT rpfT[] = //-------------- REPORTFILE cmd table, used from cnTopCult
{
	// id           cs     fn             f        uc evf     ty      b    dfls                           p2   ckf
	//------------  -----  -------------  -------  -- -----   -----   -    --------------------------     ---- ----
	CULT( "*",             STAR,  0,             0,       0, 0,      0,      0,   N,    0.f,                     N,   rfStarCkf),
	CULT( "rfFileName",    DAT,   RFI_FILENAME,  RQD,     0, VEOI,   TYSTR,  0,   N,    0.f,                     N,   N), // set to inputfile.rep for primary
	CULT( "rfFileStat",    DAT,   RFI_FILESTAT,  0,       0, 0,      TYCH,   0,   v C_FILESTATCH_OVERWRITE,0.f,  N,   N), // used in rfStarCkf: no VEOI
	CULT( "rfPageFmt",     DAT,   RFI_PAGEFMT,   0,       0, VEOI,   TYCH,   0,   v C_NOYESCH_YES,0.f,           N,   N),
	CULT( "endReportFile", ENDER, 0,             0,       0, 0,      0,      0,   N,    0.f,                     N,   N),
	CULT( "report",        RATE,  0,             0,       0, 0,      0,     &RiB, N,    0.f,                    rpT,  N),
	CULT()
};	// rpfT


//====================================== EXPORTFILE command =======================================

// initial record for implicit command EXPORTFILE Primary rfFileName = <inputfile>.csv is entered from topPrfRep.

RC FC xfStarCkf( CULT *c, /*RFI* */ void *p, void *p2, void *p3) /*ARGSUSED*/  //----------------------------------

// check function automatically called at end of EXPORTFILE object entry, also called from TopCkf.
// ONLY argument 'p' is used.
{
	return ((RFI *)p)->rf_CkF( 1);	// 0 = is export
}		// xfStarCkf

static CULT exfT[] = //-------------- EXPORTFILE cmd table, used from cnTopCult
{
	// id          cs     fn             f        uc evf     ty      b    dfls                          p2   ckf
	//-----------  -----  -------------  -------  -- ------  -----   -    --------------------------    ---- ----
	CULT( "*",            STAR,  0,             0,       0, 0,      0,      0,   N,    0.f,                    N,   xfStarCkf),
	CULT( "xfFileName",   DAT,   RFI_FILENAME,  RQD,     0, VEOI,   TYSTR,  0,   N,    0.f,                    N,   N), // set to inputfile.csv for primary
	CULT( "xfFileStat",   DAT,   RFI_FILESTAT,  0,       0, 0,      TYCH,   0,  v C_FILESTATCH_OVERWRITE, 0.f, N,   N), // used in xfStarCkf: no VEOI
	CULT( "endExportFile",ENDER, 0,             0,       0, 0,      0,      0,   N,    0.f,                    N,   N),
	CULT( "export",       RATE,  0,             0,       0, 0,      0,     &XiB, N,    0.f,                   exT,  N),
	CULT()
};	// exfT

//=================================== IMPORTFILE command ==========================================

/* related stuff: Import File data is accessed in expressions with the Import() function.
   The code that compiles Import() (in and called from cuparse.cpp) generates IffnmB
   records that access ImpfB records at run time. IffnmB has no CULT table. */

// import files are opened for each of autosize, run phases 6-95; name may be changed.

static CULT impfT[] = //-------------- IMPORTFILE cmd table, used from cnTopCult
{
	// id           cs     fn             f        uc evf     ty      b    dfls                           p2   ckf
	//------------  -----  -------------  -------  -- ------  -----   -    --------------------------     ---- ----
	//"*",          STAR,  0,             0,       0, 0,      0,      0,   N,    0.f,                     N,   impfStarCkf),
	CULT( "imFileName",    DAT,   IMPF_FILENAME, RQD,     0, VFAZLY, TYSTR,  0,   N,    0.f,                     N,   N),
	CULT( "imTitle",       DAT,   IMPF_IMTITLE,  0,       0, VFAZLY, TYSTR,  0,   N,    0.f,                     N,   N),
	CULT( "imFreq",        DAT,   IMPF_IMFREQ,   RQD,     0, VEOI,   TYCH,   0,   N,    0.f,                     N,   N),
	CULT( "imHeader",      DAT,   IMPF_HASHEADER,0,       0, VFAZLY, TYCH,   0,   v C_NOYESCH_YES, 0.f,          N,   N),
	CULT( "imBinary",      DAT,   IMPF_IMBINARY, 0,       0, VEOI,   TYCH,   0,   v C_NOYESCH_NO,  0.f,          N,   N), //poss future use
	CULT( "endImportFile", ENDER, 0,             0,       0, 0,      0,      0,   N,    0.f,                     N,   N),
	CULT()
};	// impfT



//--------------- ZONE command itself: enters a ZONE RAT Entry -------------

LOCAL RC FC infShldCkf( CULT *c, /*SI* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// check sheilding when entered, not called if expr.  check is repeated in topZn.
{
	if (*(SI *)p < 1 || *(SI *)p > 5)
		return ((ZNI *)p2)->ooer( ZNI_I + ZNISUB_INFSHLD, (char *)MH_S0443, (INT)*(SI *)p);  	// "infShld = %d: not in range 1 to 5"
	return RCOK;
}

LOCAL RC FC infStoriesCkf( CULT *c, /*SI* */ void *p, void *p2, void *p3) /*ARGSUSED*/

// check stories at entry, not called if expr.  check is repeated in topZn.
{
	if (*(SI *)p < 1 || *(SI *)p > 3)
		return ((ZNI *)p2)->ooer( ZNI_I + ZNISUB_INFSTORIES, (char *)MH_S0444, (INT)*(SI *)p);   	// "infStories = %d: not in range 1 to 3"
	return RCOK;
}

static CULT znT[] = //-------------------------- ZONE cmd RAT Entry table, used from cnTopCult
{
#define ZI(m)   (ZNI_I + ZNISUB_##m)
#define XFAN(m) (ZNI_I + ZNISUB_XFAN + FAN_##m)
// id                cs     fn               f         uc evf     ty     b       dfls              p2   ckf
//----------         -----  ---------------  --------  -- ------  -----  ------  -------------     ---- ----
// general
	CULT( "znModel",     DAT,   ZI(ZNMODEL),     0,        0, VEOI,   TYCH,  0,  C_ZNMODELCH_CNE, N,   N),
// note: znArea/znVol required, see runtime check
	CULT( "znArea",      DAT,   ZI(ZNAREA),      0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znVol",       DAT,   ZI(ZNVOL),       0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znFloorZ",    DAT,   ZI( FLOORZ),     0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znCeilingHt", DAT,   ZI( CEILINGHT),  0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znCAir",      DAT,   ZI(ZNCAIR),      0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znHIRatio",	 DAT,	ZI( HIRATIO),	 0,		   0, VEOI,   TYFL,  0,      1.f,	   N,	N),
	CULT( "znAzm",       DAT,   ZI(ZNAZM),       0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znSC",        DAT,   ZI(ZNSC),        0,        0, VHRLY,  TYFL,  0,      0.f,      N,   N),

	CULT( "znTH",        DAT,   ZI(ZNTH),        0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znTD",        DAT,   ZI(ZNTD),        0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znTC",        DAT,   ZI(ZNTC),        0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znQMxH",      DAT,   ZI(ZNQMXH),      0,        0, VHRLY,  TYFL,  0,      0.f,      N,   N),
	CULT( "znQMxHRated", DAT,   ZI(ZNQMXHRATED), AS_OK,    0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znQMxC",      DAT,   ZI(ZNQMXC),      0,        0, VHRLY,  TYFL,  0,      0.f,      N,   N),
	CULT( "znQMxCRated", DAT,   ZI(ZNQMXCRATED), 0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "znRsys",	     DAT,   ZI( RSI),        0,        0, VEOI,   TYREF, &RSiB,  N,        N,   N),

// convection
	CULT( "znHcFrcF",    DAT,   ZI( HCFRCF),     0,        0, VHRLY,   TYFL, 0,      0.2f,     N,   N),
	CULT( "znHcAirX",    DAT,   ZI( HCAIRX),     0,        0, VSUBHRLY,TYFL, 0,      1.0f,     N,   N),

// comfort
#ifdef COMFORT_MODEL
	CULT( "znComfClo",   DAT,   ZI(ZNCOMFCLO),   0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znComfMet",   DAT,   ZI(ZNCOMFMET),   0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znComfAirV",  DAT,   ZI(ZNCOMFAIRV),  0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
	CULT( "znComfRh",    DAT,   ZI(ZNCOMFRH),    0,        0, VSUBHRLY,TYFL, 0,      0.f,      N,   N),
#endif

// infiltration, airnet, exterior convection
	CULT( "infAC",       DAT,   ZI(INFAC),       0,        0, VHRLY,  TYFL,  0,      .5f,      N,   N),
	CULT( "infELA",      DAT,   ZI(INFELA),      0,        0, VHRLY,  TYFL,  0,      0.f,      N,   N),
	CULT( "infShld",     DAT,   ZI(INFSHLD),     0,        0, VEOI,   TYSI,  0,      3,      N,   infShldCkf),
	CULT( "infStories",  DAT,   ZI(INFSTORIES),  0,        0, VEOI,   TYSI,  0,      1,      N,   infStoriesCkf),
	CULT( "znEaveZ",	 DAT,	ZI( EAVEZ),		 0,		   0, VEOI,   TYFL,  0,      8.f,              N,   N),
	CULT( "znWindFLkg",  DAT,   ZI( WINDFLKG),   0,        0, VSUBHRLY, TYFL,  0,    1.f,           N,   N),
	CULT( "znAFMtr",     DAT,   ZI( AFMTRI),     0,        0, VEOI,   TYREF, &AfMtriB, 0.f,              N,   N),

#if 0
x idea, not implemented, 2-2012
x	CULT( "znWindFCnv",  DAT,   ZI( WINDFCNV),   0,        0, VSUBHRLY, TYFL,  0,    1.f,           N,   N),
#endif
// zone exhaust fan
	CULT( "xfanFOn",     DAT,  ZI(XFANFON),      0,        0, VHRLY,  TYFL,  0,      1.f,      N,   N),
	CULT( "xfanVfDs",    DAT,  XFAN(VFDS),       0,        0, VEOI,   TYFL,  0,      0.f,      N,   N), // 0 means no fan
	CULT( "xfanPress",   DAT,  XFAN(PRESS),      0,        0, VEOI,   TYFL,  0,       .3f,     N,   N), //*****default??????
	CULT( "xfanEff",     DAT,  XFAN(EFF),        0,        0, VEOI,   TYFL,  0,       .08f,    N,   N),
	CULT( "xfanShaftBhp",DAT,  XFAN(SHAFTPWR),   0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "xfanElecPwr", DAT,  XFAN(ELECPWR),    0,        0, VEOI,   TYFL,  0,      0.f,      N,   N),
	CULT( "xfanMtr",     DAT,  XFAN(MTRI),       0,        0, VEOI,   TYREF, &MtriB, 0.f,      N,   N),
	CULT( "xfanType",    DAT,  XFAN(FANTY),      NO_INP,   0, 0,      TYCH,  0,   C_FANTYCH_NONE, N,   N),
	CULT( "xfanVfMxF",   DAT,  XFAN(VFMXF),      NO_INP,   0, 0,      TYFL,  0,      1.f,     N,   N),
	CULT( "xfanMotPos",  DAT,  XFAN(MOTPOS),     NO_INP,   0, 0,      TYCH,  0,   C_MOTPOSCH_INFLO, N,   N),
	CULT( "xfanCurvePy", DAT,XFAN(CURVEPY+PYCUBIC_K),
	                                       NO_INP|ARRAY,   0, VEOI,   TYFL,  0,     0.f,     v 6,  N), // use default: runs at full flow only. cult entry probably needed for id for FAN error msgs, 6-92
	CULT( "xfanMotEff",  DAT,  XFAN(MOTEFF),     NO_INP,   0, 0,      TYFL,  0,     1.f,      N,   N),
// zone subobjects
	CULT( "surface",     RATE,  0,               0,        0, 0,      0,     &SfiB,  N,     sfT,   N),
	CULT( "perimeter",   RATE,  0,               0,        0, 0,      0,     &PriB,  N,    perT,   N),
	CULT( "terminal",    RATE,  0,               0,        0, 0,      0,     &TuiB,  N,     tuT,   N),
	CULT( "gain",        RATE,  0,               0,        0, 0,      0,     &GniB,  N,     gnT,   N),
	CULT( "endZone",     ENDER, 0,               0,        0, 0,      0,     0,      N,       N,   N),
	CULT()
};	// znT
#undef ZI
#undef XFAN

//=================== IZXFER command (interzone transfer) ===================
RC izStarCkf( CULT* c, void *p, void* p2, void* p3)

// called at end of IZXFER object entry.
//	ALSO called from topSf1() at RUN, to issue msgs otherwise lost
//  if surface is part of DEFTYPE ZONE and not ALTERed at TYPE use
//  (suppress msgs here during deftype if correctible at TYPE use).

// ONLY argument 'p' is used.
{
	return ((IZXRAT* )p)->iz_CkfIZXFER();
}		// sfStarCkf
//---------------------------------------------------------------------------
#define ZFAN(m) (IZXRAT_FAN + FAN_##m)
static CULT izxT[] = //------ IZXFER cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn            f   uc evf      ty     b      dfls                 p2   ckf
//----------         -----  ------------  --- -- ------   -----  -----  -------------        ---- ----
CULT( "*",           STAR,  0,            0,   0, 0,      0,     0,      0.f,                N,   izStarCkf),
CULT( "izZn1",       DAT,   IZXRAT_ZI1,   RQD, 0, VEOI,   TYREF, &ZiB,   0.f,                N,   N),
CULT( "izZn2",       DAT,   IZXRAT_ZI2,   0,   0, VEOI,   TYREF, &ZiB,   0.f,                N,   N),
CULT( "izHConst",    DAT,   IZXRAT_UA,	  0,   0, VHRLY,  TYFL,  0,      0.f,                N,   N),
CULT( "izNVType",    DAT,   IZXRAT_NVCNTRL,0,  0, VEOI,   TYCH,  0,   C_IZNVTYCH_NONE,       N,   N),
CULT( "izAFCat",     DAT,   IZXRAT_AFCATI, 0,  0, VEOI,   TYCH,  0,      0,                  N,   N),
CULT( "izALo",       DAT,   IZXRAT_A1,    0,   0, VHRLY,  TYFL,  0,      0.f,                N,   N),
CULT( "izAHi",       DAT,   IZXRAT_A2,    0,   0, VHRLY,  TYFL,  0,      0.f,                N,   N),
CULT( "izL1",        DAT,   IZXRAT_L1,    0,   0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izL2",        DAT,   IZXRAT_L2,    0,   0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izStairAngle",DAT,	IZXRAT_STAIRANGLE,
										  0,   0, VEOI,	  TYFL,  0,      34.f,			     N,   N),
CULT( "izHD",        DAT,   IZXRAT_HZ,    0,   0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izNVEff",     DAT,   IZXRAT_CD,    0,   0, VEOI,   TYFL,  0,      .8f,                N,   N),
CULT( "izCpr",		 DAT,	IZXRAT_CPR,	  0,   0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izExp",		 DAT,	IZXRAT_EXP,	  0,   0, VEOI,   TYFL,  0,      .5f,                N,   N),
CULT( "izVfMin",	 DAT,   IZXRAT_VFMIN, 0,   0, VSUBHRLY,TYFL, 0,      0.f,                N,   N),
CULT( "izVfMax",	 DAT,   IZXRAT_VFMAX, 0,   0, VSUBHRLY,TYFL, 0,      0.f,                N,   N),
CULT( "izASEF",		 DAT,   IZXRAT_ASEF,  0,   0, VSUBHRLY,TYFL, 0,	     0.f,				 N,   N),
CULT( "izLEF",		 DAT,   IZXRAT_LEF,   0,   0, VSUBHRLY,TYFL, 0,		 0.f,				 N,   N),
CULT( "izSRE",		 DAT,   IZXRAT_SRE,   0,   0, VSUBHRLY,TYFL, 0,		 0.f,				 N,   N),
CULT( "izASRE",		 DAT,   IZXRAT_ASRE,  0,   0, VSUBHRLY,TYFL, 0,		 0.f,				 N,   N),
CULT( "izRVFanHeatF",DAT,   IZXRAT_RVFANHEATF,0,0,VSUBHRLY,TYFL, 0,		 0.f,				 N,   N),

CULT( "izVfExhRat",	 DAT,   IZXRAT_VFEXHRAT, 0,0, VSUBHRLY,TYFL, 0,      1.f,                N,   N),
CULT( "izEATR",		 DAT,   IZXRAT_EATR,  0,   0, VSUBHRLY,TYFL, 0,      0.f,                N,   N),

// following iff fan
CULT( "izFanVfDs",   DAT,   ZFAN(VFDS),   0,   0, VEOI,   TYFL,  0,      0.f,                N,   N), // 0 means no fan
CULT( "izFanPress",  DAT,   ZFAN(PRESS),  0,   0, VEOI,   TYFL,  0,      .3f,                N,   N), //*****default??????
CULT( "izFanEff",    DAT,   ZFAN(EFF),    0,   0, VEOI,   TYFL,  0,      .08f,               N,   N),
CULT( "izFanShaftBhp",DAT,  ZFAN(SHAFTPWR),0,  0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izFanElecPwr",DAT,   ZFAN(ELECPWR),0,   0, VEOI,   TYFL,  0,      0.f,                N,   N),
CULT( "izFanMtr",    DAT,   ZFAN(MTRI),   0,   0, VEOI,   TYREF, &MtriB, 0.f,                N,   N),
CULT( "izFanEndUse", DAT,	ZFAN(ENDUSE), 0,   0, VEOI,   TYCH,  0,      C_ENDUSECH_FAN,	 N,   N),
CULT( "izFanType",   DAT,   ZFAN(FANTY),  NO_INP, 0, 0,   TYCH,  0,      C_FANTYCH_NONE,     N,   N),
CULT( "izFanVfMxF",  DAT,   ZFAN(VFMXF),  NO_INP, 0, 0,   TYFL,  0,      1.0f,               N,   N),
CULT( "izFanMotPos", DAT,   ZFAN(MOTPOS), NO_INP, 0, 0,   TYCH,  0,      C_MOTPOSCH_INFLO,   N,   N),
CULT( "izFanCurvePy",DAT,   ZFAN(CURVEPY+PYCUBIC_K),
                                          ARRAY,  0, VEOI,TYFL,  0,      0.f,               v 6,  N), // use default: runs at full flow only. cult entry probably needed for id for FAN error msgs
CULT( "izFanMotEff", DAT,   ZFAN(MOTEFF), NO_INP, 0, 0,   TYFL,  0,      1.f,                N,   N),
CULT( "endIzxfer",   ENDER, 0,            0,   0, 0,      0,     0,      0.f,                N,   N),
CULT()
};	// izxT
#undef ZFAN


//=================== DUCTSEG command ( duct segment) ===================
RC DUCTSEG::ds_Ckf()
{
	RC rc=RCOK;

	if (IsSet( DUCTSEG_EXAREA))
	{	// flat configuration: area is fixed
		rc |= disallow( DUCTSEG_INAREA, "when dsExArea is given");
		ignoreN( "when dsExArea is given", DUCTSEG_DIAM, DUCTSEG_LEN,
			DUCTSEG_BRANCHCOUNT, DUCTSEG_BRANCHLEN, DUCTSEG_BRANCHCFA, DUCTSEG_AIRVELDS, 0);
	}
	else
	{	// round: many interdependent defaults
		// # of branches: user-input branch count takes precedence
		if (IsSet( DUCTSEG_BRANCHCOUNT))
			ignore( DUCTSEG_BRANCHCFA, "when dsBranchCount is given");
		if (IsSet( DUCTSEG_BRANCHLEN))
			rc |= disallow( DUCTSEG_LEN, "when dsBranchLen is given");

		int dimCount = IsSet( DUCTSEG_DIAM) + IsSet( DUCTSEG_LEN) + IsSet( DUCTSEG_INAREA);
		if (dimCount == 3)
			rc |= oer( "at most 2 of dsDiameter, dsLength, and dsInArea are allowed");
	}

	const char* when = strtprintf( "when %s=%s",
		mbrIdTx( DUCTSEG_EXCND), getChoiTx( DUCTSEG_EXCND));

	switch (ds_exCnd)
	{
	case C_EXCNDCH_ADJZN:
		rc |= requireN( when, DSBC( ZI), 0 );
		rc |= disallowN( when, DSBC( TXE), 0 );
		break;

	case C_EXCNDCH_AMBIENT:
		rc |= disallowN( when, DSBC( ZI), DSBC( TXA), 0 );
		break;

	case C_EXCNDCH_ADIABATIC:
		rc |= disallowN( when, DSBC( ZI), DSBC( TXA), 0 );
		break;

	case C_EXCNDCH_SPECT:
		rc |= disallowN( when, DSBC( ZI), 0);
		rc |= requireN( when, DSBC( TXA), 0 );
		break;

	default:
		rc = ooer( DUCTSEG_EXCND, "dsExCnd=%s not supported", getChoiTx( DUCTSEG_EXCND));
		break;
	}

	return rc;
}	// DUCTSEG::ds_Ckf
//-----------------------------------------------------------------------------
static RC FC dsStarCkf( CULT *c, /*DUCTSEG* */ void *p, void *p2, void *p3)
{	return ((DUCTSEG*)p)->ds_Ckf();
}
//-----------------------------------------------------------------------------
static CULT dsT[] = //------ DUCTSEG cmd RAT Entry table, used from RSys
{
// id         cs     fn               f        uc evf        ty     b      dfls                 p2   ckf
//----------  -----  ---------------  -------  -- ------     -----  -----  -------------        ---- ----
CULT( "*",           STAR, 0,             0,   0, 0,         0,     0,      0.f,                N,   &dsStarCkf),
CULT( "dsRSys",      DAT,  DUCTSEG_OWNTI, NO_INP|RDFLIN,0,0, TYREF, &RSiB,  0,                  N,   N),
CULT( "dsTy",		 DAT,  DUCTSEG_TY,    RQD, 0, VEOI,      TYCH,  0,      -1,                 N,   N),
CULT( "dsBranchLen", DAT,  DUCTSEG_BRANCHLEN,0,0, VEOI, 	 TYFL,  0,     -1.f,				N,   N),
CULT( "dsBranchCount",DAT, DUCTSEG_BRANCHCOUNT,0,0,VEOI,     TYSI,  0,     -1,                  N,   N),
CULT( "dsBranchCFA", DAT,  DUCTSEG_BRANCHCFA,0,0, VEOI,      TYFL,  0,     -1,                  N,   N),
CULT( "dsAirVelDs",  DAT,  DUCTSEG_AIRVELDS, 0,0, VEOI, 	 TYFL,  0,     -1.f,				N,   N),
CULT( "dsExArea",    DAT,  DUCTSEG_EXAREA,0,   0, VEOI,      TYFL,  0,      0.f,                N,   N),
CULT( "dsInArea",    DAT,  DUCTSEG_INAREA,0,   0, VEOI,      TYFL,  0,      0.f,                N,   N),
CULT( "dsDiameter",	 DAT,  DUCTSEG_DIAM,  0,   0, VEOI,		 TYFL,  0,      0.f,				N,   N),
CULT( "dsLength",	 DAT,  DUCTSEG_LEN,   0,   0, VEOI, 	 TYFL,  0,      0.f,				N,   N),
CULT( "dsExCnd",     DAT,  DUCTSEG_EXCND, 0,   0, 0,         TYCH,  0,      C_EXCNDCH_ADJZN,    N,   N),
CULT( "dsAdjZn",     DAT,  DSBC( ZI),     0,   0, VEOI,      TYREF, &ZiB,   0.f,                N,   N),
CULT( "dsExT",       DAT,  DSBC( TXA),    0,   0, VSUBHRLY,  TYFL,  0,      70.f,               N,   N),
CULT( "dsEpsLW",     DAT,  DSBC( EPSLW),  0,   0, VEOI,      TYFL,  0,      .9f,                N,   N),
CULT( "dsInsulR",	 DAT,  DUCTSEG_INSULR,0,   0, VEOI,	     TYFL,  0,      0.f,				N,   N),
CULT( "dsInsulMat",  DAT,  DUCTSEG_INSULMATI,0,0, VEOI,      TYREF, &MatiB,   0,                N,   N),
CULT( "dsLeakF",	 DAT,  DUCTSEG_LEAKF, 0,   0, VEOI,		 TYFL,  0,      0.f,				N,   N),
CULT( "dsExH",       DAT,  DSBC( HXA),    0,   0, VSUBHRLY,  TYFL,  0,     .54f,                N,   N),
CULT( "endDuctSeg",  ENDER, 0,            0,   0, 0,         0,     0,      0.f,                N,   N),
CULT()
};	// dsT

//=================== RSYS command ( residential system) ===================
LOCAL RC FC rsStarCkf( CULT* c, /*RSYS* */ void *p, void *p2, void *p3)
// called at end of RSYS input, to get messages near source of error.
{
	return ((RSYS *)p)->rs_CkF();
}		// rsStarCkf
//==========================================================================
#define RSFAN( m) (RSYS_FAN + FAN_##m)
static CULT rsysT[] = //------ RSYS cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,               0,       0, 0,      0,     0,      0.f,    N,   rsStarCkf),
CULT( "rsType",		 DAT,   RSYS_TYPE,		 0,       0, VEOI,   TYCH,  0,      C_RSYSTYCH_ACFURN, N, N),
CULT( "rsDesc",		 DAT,   RSYS_DESC,       0,       0, VEOI,   TYSTR, 0,		0,		N,	 N),
CULT( "rsPerfMap",	 DAT,	RSYS_PERFMAP,	 0,       0, VEOI,   TYCH,  0,      C_NOYESCH_NO, N, N),
CULT( "rsFanTy",	 DAT,	RSFAN( FANTY),   0,       0, VEOI,   TYCH,  0,      C_FANTYCH_BLOWTHRU, N, N ),
CULT( "rsFanMotTy",  DAT,   RSFAN( MOTTY),   0,       0, VEOI,   TYCH,  0,      C_MOTTYCH_PSC,  N, N ),
CULT( "rsModeCtrl",  DAT,   RSYS_MODECTRL,   0,       0, VHRLY,  TYCH,  0,    C_RSYSMODECTRLCH_AUTO, N, N),
CULT( "rsElecMtr",   DAT,   RSYS_ELECMTRI,   0,       0, VEOI,	 TYREF, &MtriB, N,      N,   N),
CULT( "rsFuelMtr",   DAT,   RSYS_FUELMTRI,   0,       0, VEOI,	 TYREF, &MtriB, N,      N,   N),

CULT( "rsTdDesH",	 DAT,   RSYS_TDDESH,     0,       0, VEOI,   TYFL,  0,    50.f,     N,   N),
CULT( "rsTdDesC",	 DAT,   RSYS_TDDESC,     0,       0, VEOI,   TYFL,  0,    -25.f,    N,   N),
CULT( "rsFxCapH",	 DAT,   RSYS_FXCAPHTARG, 0,       0, VEOI,   TYFL,  0,    1.4f,     N,   N),
CULT( "rsFxCapAuxH", DAT,   RSYS_FXCAPAUXHTARG, 0,    0, VEOI,   TYFL,  0,    1.0f,     N,   N),
CULT( "rsFxCapC",    DAT,   RSYS_FXCAPCTARG, 0,       0, VEOI,   TYFL,  0,    1.2f,     N,   N),

CULT( "rsAFUE",      DAT,   RSYS_AFUE,       0,       0, VEOI,   TYFL,  0,     .90f,    N,   N),
CULT( "rsCapH",      DAT,   RSYS_CAPH,       AS_OK,   0, VFAZLY|EVENDIVL,TYFL,0,0.f,    N,   N),

CULT( "rsHSPF",		 DAT,   RSYS_HSPF,       0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCap47",	 DAT,   RSYS_CAP47,      AS_OK,   0, VFAZLY|EVENDIVL,TYFL,0,0.f,    N,   N),
CULT( "rsCOP47",	 DAT,   RSYS_COP47,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCap35",	 DAT,   RSYS_CAP35,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCOP35",	 DAT,   RSYS_COP35,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCap17",	 DAT,   RSYS_CAP17,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCOP17",	 DAT,   RSYS_COP17,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCdH",       DAT,   RSYS_CDH,        0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCapAuxH",	 DAT,   RSYS_CAPAUXH,    AS_OK,   0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCOPAuxH",	 DAT,   RSYS_COPAUXH,    0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "rsASHPLockOutT", DAT, RSYS_ASHPLOCKOUTT,  0,   0, VHRLY,  TYFL,  0,   -999.f,    N,   N),
CULT( "rsDefrostModel", DAT, RSYS_DEFROSTMODEL,	 0,   0, VEOI,   TYCH,  0,    C_RSYSDEFROSTMODELCH_REVCYCLEAUX, N, N),

CULT( "rsFanPwrH",   DAT,   RSYS_FANPWRH,    0,       0, VEOI,   TYFL,  0,    .365f,    N,   N),

CULT( "rsSEER",		 DAT,   RSYS_SEER,       0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsEER",		 DAT,   RSYS_EER95,      0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsCapC",		 DAT,   RSYS_CAP95,	     AS_OK,   0, VFAZLY|EVENDIVL, TYFL, 0, 0.f, N,   N),
CULT( "rsFChg",      DAT,   RSYS_FCHG,       0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "rsFSize",     DAT,   RSYS_FSIZE,      0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "rsCdC",       DAT,   RSYS_CDC,        0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsVFPerTon",  DAT,   RSYS_VFPERTON,   0,       0, VEOI,   TYFL,  0,    350.f,    N,   N),
CULT( "rsFanPwrC",   DAT,   RSYS_FANPWRC,    0,       0, VEOI,   TYFL,  0,    .365f,    N,   N),

CULT( "rsParElec",	 DAT,   RSYS_PARELEC,    0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "rsParFuel",	 DAT,   RSYS_PARFUEL,    0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),

CULT( "rsFEffH",     DAT,   RSYS_FEFFH,      0,       0, VSUBHRLY,TYFL, 0,      1.f,    N,   N),
CULT( "rsFEffC",     DAT,   RSYS_FEFFC,      0,       0, VSUBHRLY,TYFL, 0,      1.f,    N,   N),

CULT( "rsRhIn",      DAT,   RSYS_RHINTEST,   0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "rsTdbOut",	 DAT,	RSYS_TDBOUT,	 0,		  0, VHRLY,  TYFL,  0,      0.f,    N,   N),

CULT( "rsDSEH",		 DAT,   RSYS_DSEH,       0,       0, VHRLY,  TYFL,  0,     -1.f,    N,   N),
CULT( "rsDSEC",		 DAT,   RSYS_DSEC,       0,       0, VHRLY,  TYFL,  0,     -1.f,    N,   N),

CULT( "rsCapNomH",   DAT,   RSYS_CAPNOMH,    0,       0, VDAILY, TYFL,  0,     0.f,    N,   N),
CULT( "rsCapNomC",	 DAT,   RSYS_CAPNOMC,    0,       0, VDAILY, TYFL,  0,     0.f,    N,   N),

CULT( "rsOAVType",	 DAT,   RSYS_OAVTYPE,    0,       0, VEOI,   TYCH,  0,    C_RSYSOAVTYCH_NONE, N, N),
CULT( "rsOAVReliefZn",DAT,  RSYS_OAVRELIEFZI,0,       0, VEOI,   TYREF,  &ZiB, 0, N, N),
CULT( "rsOAVTDbInlet",DAT,	RSYS_OAVTDBINLET,0,		  0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "rsOAVTdiff",  DAT,	RSYS_OAVTDIFF,   0,		  0, VHRLY,  TYFL,  0,      5.f,    N,   N),
CULT( "rsOAVVfDs",   DAT,	RSYS_OAVAVFDS,   0,		  0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "rsOAVVfMinF", DAT,	RSYS_OAVAVFMINF, 0,		  0, VEOI,   TYFL,  0,      0.2f,   N,   N),
CULT( "rsOAVFanPwr", DAT,	RSYS_OAVFANPWR,  0,		  0, VEOI,   TYFL,  0,      .5f,    N,   N),

CULT( "ductSeg",     RATE,  0,               0,       0, 0,      0,     &DsiB,  N,      dsT, N),
CULT( "endRSYS",     ENDER, 0,               0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// rsysT
#undef AS_OKIF

//============================= DHWMETER command =============================
LOCAL RC wmtStarCkf( CULT* c, /*DHWMTR* */ void *p, void *p2, void *p3)
// called at end of DHWMETER input, to get messages near source of error.
{
	return ((DHWMTR*)p)->wmt_CkF();
}		// wmtStarCkf
//=============================================================================
static CULT dhwMeterT[] = //------ DHWMETER cmd RAT Entry table
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wmtStarCkf),
CULT( "endDHWMETER", ENDER, 0,                 0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwMeterT

//============================= AFMETER command =============================
LOCAL RC amtStarCkf(CULT* c, /*AFMTR* */ void *p, void *p2, void *p3)
// called at end of AFMETER input, to get messages near source of error.
{
	return ((AFMTR*)p)->amt_CkF();
}		// amtStarCkf
//=============================================================================
static CULT afMeterT[] = //------ AFMETER cmd RAT Entry table
{
	// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
	//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
	CULT("*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   amtStarCkf),
	CULT("endAFMETER",  ENDER, 0,                 0,       0, 0,      0,     0,      0.f,    N,   N),
	CULT()
};	// afMeterT

//============================= DHWUSE command =============================
LOCAL RC FC wuStarCkf( CULT* c, /*DHWUSE* */ void *p, void *p2, void *p3)
// called at end of DHWUSE input, to get messages near source of error.
{
	return ((DHWUSE*)p)->wu_CkF();
}		// wuStarCkf
//=============================================================================
static CULT dhwUseT[] = //------ DHWUSE cmd RAT Entry table, used from dhwDayUseT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wuStarCkf),
CULT( "wuDHWDAYUSE", DAT,   DHWUSE_OWNTI,      NO_INP|RDFLIN,0,0,  TYREF, &WDUiB,  0,     N,   N),
CULT( "wuHWEndUse",	 DAT,	DHWUSE_HWENDUSE,   0,       0, VEOI,   TYCH,  0,      0,      N,   N),
CULT( "wuStart",	 DAT,	DHWUSE_START,      RQD,     0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "wuFlow",      DAT,   DHWUSE_FLOW,       0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "wuDuration",  DAT,   DHWUSE_DUR,        0,       0, VHRLY,  TYFL,  0,      1.f,    N,   N),
CULT( "wuTemp",      DAT,   DHWUSE_TEMP,       0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "wuHotF",      DAT,   DHWUSE_HOTF,       0,       0, VHRLY,  TYFL,  0,      1.f,    N,   N),
CULT( "wuHeatRecEF", DAT,   DHWUSE_HEATRECEF,  0,       0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "wuEventID",   DAT,   DHWUSE_EVENTID,    0,       0, VEOI,   TYSI,  0,      0,      N,   N),
CULT( "endDHWUSE",   ENDER, 0,                 0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwUseT


//============================= DHWDAYUSE command =============================
LOCAL RC wduStarCkf( CULT* c, /*DHWDAYUSE* */ void *p, void *p2, void *p3)
// called at end of DHWDAYUSE input, to get messages near source of error.
{
	return ((DHWDAYUSE*)p)->wdu_CkF();
}		// wduStarCkf
//=============================================================================
static CULT dhwDayUseT[] = //------ DHWDAYUSE cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wduStarCkf),
CULT( "wduMult",	 DAT,	DHWDAYUSE_MULT,    0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "dhwuse",      RATE,  0,                 0,       0, 0,      0,     &WUiB,  N,      dhwUseT, N),
CULT( "endDHWDAYUSE",ENDER, 0,                 0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwDayUseT

//============================= DHWHEATER command =============================
LOCAL RC FC whStarCkf( CULT* c, /*DHWHEATER* */ void *p, void *p2, void *p3)
// called at end of DHWHEATER input, to get messages near source of error.
{
	return ((DHWHEATER*)p)->wh_CkF();
}		// whStarCkf
//=============================================================================
static CULT dhwHeaterT[] = //------ DHWHEATER cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   whStarCkf),
CULT( "whDHWSYS",    DAT,   DHWHEATER_OWNTI,   NO_INP|RDFLIN,0,0, TYREF, &WSiB,  0,     N,   N),
CULT( "whMult",		 DAT,	DHWHEATER_MULT,	   0,     0, VEOI,   TYFL,  0,      1.f,    N, N),
CULT( "whType",	     DAT,   DHWHEATER_TYPE,    0,     0, VEOI,   TYCH,  0,      C_WHTYPECH_STRGSML, N, N),
CULT( "whHeatSrc",	 DAT,   DHWHEATER_HEATSRC, 0,     0, VEOI,   TYCH,  0,      C_WHHEATSRCCH_FUEL, N, N),
CULT( "whZone",      DAT,   DHWHEATER_ZNTI,    0,     0, VEOI,   TYREF, &ZiB,   0,                  N, N),
CULT( "whTEx",       DAT,   DHWHEATER_TEX,     0,     0, VSUBHRLY,TYFL, 0,      70.f,               N, N),
CULT( "whASHPType",	 DAT,   DHWHEATER_ASHPTY,  0,     0, VEOI,   TYCH,  0,      -1,                 N, N),
CULT( "whASHPSrcZn", DAT,   DHWHEATER_ASHPSRCZNTI,0,  0, VEOI,   TYREF, &ZiB,   0,                  N, N),
CULT( "whASHPSrcT",  DAT,   DHWHEATER_ASHPTSRC,0,     0, VSUBHRLY,TYFL, 0,      70.f,               N, N),
CULT( "whASHPResUse",DAT,   DHWHEATER_ASHPRESUSE,0,   0, VEOI,   TYFL,  0,      7.22f,              N, N),
CULT( "whtankCount", DAT,   DHWHEATER_TANKCOUNT,0,    0, VEOI,   TYFL,  0,      1.f,    N, N),
CULT( "whVol",		 DAT,   DHWHEATER_VOL,      0,    0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "whUA",		 DAT,   DHWHEATER_UA,       0,    0, VEOI,   TYFL,  0,     -1.f,    N, N),
CULT( "whInsulR",    DAT,   DHWHEATER_INSULR,  0,     0, VEOI,   TYFL,  0,     -1.f,    N, N),
CULT( "whInHtSupply", DAT,  DHWHEATER_INHTSUPPLY, 0,  0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "whInHtLoopRet",DAT,  DHWHEATER_INHTLOOPRET,0,  0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "whEF",		 DAT,   DHWHEATER_EF,      0,     0, VEOI,   TYFL,  0,      0.82f,  N, N),
CULT( "whUEF",		 DAT,   DHWHEATER_UEF,     0,     0, VEOI,   TYFL,  0,        0.f,  N, N),
CULT( "whStbyElec",  DAT,   DHWHEATER_STBYELEC,  0,   0, VEOI,   TYFL,  0,        4.f,  N, N),
CULT( "whRatedFlow", DAT,   DHWHEATER_RATEDFLOW, 0,   0, VEOI,   TYFL,  0,        0.f,  N, N),
CULT( "whLoadCFwdF", DAT,   DHWHEATER_LOADCFWDF, 0,   0, VEOI,   TYFL,  0,        1.f,  N, N),
CULT( "whAnnualFuel",DAT,   DHWHEATER_ANNUALFUEL,0,   0, VEOI,   TYFL,  0,        0.f,  N, N),
CULT( "whAnnualElec",DAT,   DHWHEATER_ANNUALELEC,0,   0, VEOI,   TYFL,  0,        0.f,  N, N),
CULT( "whResHtPwr",	 DAT,   DHWHEATER_RESHTPWR,0,     0, VEOI,   TYFL,  0,     4500.f,  N, N),
CULT( "whResHtPwr2", DAT,   DHWHEATER_RESHTPWR2,0,    0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "whLDEF",		 DAT,   DHWHEATER_LDEF,    0,     0, VEOI,   TYFL,  0,      1.0f,   N, N),
CULT( "whEff",		 DAT,   DHWHEATER_EFF,     0,     0, VEOI,   TYFL,  0,      1.0f,   N, N),
CULT( "whSBL",		 DAT,   DHWHEATER_SBL,     0,     0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "whPilotPwr",  DAT,   DHWHEATER_PILOTPWR,0,     0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "whParElec",   DAT,   DHWHEATER_PARELEC, 0,     0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "whElecMtr",   DAT,   DHWHEATER_ELECMTRI,0,     0, VEOI,	 TYREF, &MtriB, N,      N, N),
CULT( "whFuelMtr",   DAT,   DHWHEATER_FUELMTRI,0,     0, VEOI,	 TYREF, &MtriB, N,      N,   N),
CULT( "whxBUEndUse", DAT,	DHWHEATER_XBUENDUSE,0,    0, VEOI,   TYCH,  0,      0,	    N,   N),
CULT( "endDHWHEATER",ENDER, 0,                 0,     0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwHeaterT

//============================= DHWHEATREC command =============================
LOCAL RC FC wrStarCkf( CULT* c, /*DHWTANK* */ void *p, void *p2, void *p3)
// called at end of DHWHEATREC input, to get messages near source of error.
{
	return ((DHWHEATREC*)p)->wr_CkF();
}		// wtStarCkf
//=============================================================================
static CULT dhwHeatRecT[] = //------ DHWHEATREC cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wrStarCkf),
CULT( "wrDHWSYS",    DAT,   DHWHEATREC_OWNTI,  NO_INP|RDFLIN,0,0,  TYREF, &WSiB,  0,      N,   N),
CULT( "wrMult",		 DAT,	DHWHEATREC_MULT,   0,        0, VEOI,  TYSI,  0,      1,      N,   N),
CULT( "wrType",	     DAT,	DHWHEATREC_TYPE,   0,        0, VEOI,  TYCH,  0,      C_DWHRTYCH_VERT, N,   N),
CULT( "wrCSARatedEF",DAT,	DHWHEATREC_EFFRATED,RQD,     0, VHRLY, TYFL,  0,      .5f,    N,   N),
CULT( "wrCountFXDrain",DAT, DHWHEATREC_NFXDRAIN,0,       0, VEOI,  TYSI,  0,      1,      N,   N),
CULT( "wrCountFXCold",DAT,  DHWHEATREC_NFXCOLD, 0,       0, VEOI,  TYSI,  0,      0,      N,   N),
CULT( "wrFeedsWH",   DAT,	DHWHEATREC_FEEDSWH, 0,       0, VEOI,  TYCH,  0,      C_NOYESCH_NO,  N,   N),
CULT( "wrTDInDiff",  DAT,   DHWHEATREC_TDINDIFF,0,       0, VHRLY, TYFL,  0,     4.6f,    N,   N),
CULT( "wrTDInWarmup",DAT,   DHWHEATREC_TDINWARMUP, 0,    0, VHRLY, TYFL,  0,     65.f,    N,   N),
CULT( "wrHWEndUse",	 DAT,	DHWHEATREC_HWENDUSE,0,       0, VEOI,  TYCH,  0,      C_DHWEUCH_SHOWER, N,   N),
CULT( "endDHWHEATREC",  ENDER, 0,              0,        0, 0,     0,     0,      0,      N,   N),
CULT()
};	// dhwHeatRecT

//============================= DHWTANK command =============================
LOCAL RC FC wtStarCkf( CULT* c, /*DHWTANK* */ void *p, void *p2, void *p3)
// called at end of DHWTANK input, to get messages near source of error.
{
	return ((DHWTANK*)p)->wt_CkF();
}		// wtStarCkf
//=============================================================================
static CULT dhwTankT[] = //------ DHWTANK cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wtStarCkf),
CULT( "wtDHWSYS",    DAT,   DHWTANK_OWNTI,     NO_INP|RDFLIN,0,0,  TYREF, &WSiB,  0,      N,   N),
CULT( "wtMult",		 DAT,	DHWTANK_MULT,	   0,     0, VEOI,     TYSI,  0,      1,      N,   N),
CULT( "wtVol",		 DAT,	DHWTANK_VOL,	   0,     0, VEOI,     TYFL,  0,      0.f,    N,   N),
CULT( "wtUA",		 DAT,	DHWTANK_UA,	       0,     0, VEOI,     TYFL,  0,     -1.f,    N,   N),
CULT( "wtInsulR",    DAT,	DHWTANK_INSULR,    0,     0, VEOI,     TYFL,  0,     12.f,    N,   N),
CULT( "wtZone",      DAT,   DHWTANK_ZNTI,      0,     0, VEOI,     TYREF, &ZiB,   0,      N, N),
CULT( "wtTEx",		 DAT,	DHWTANK_TEX,	   0,     0, VHRLY,    TYFL,  0,      0.f,    N,   N),
CULT( "wtTTank",     DAT,	DHWTANK_TTANK,     0,     0, VHRLY,    TYFL,  0,      0.f,    N,   N),
CULT( "wtXLoss",     DAT,	DHWTANK_XLOSS,     0,     0, VHRLY,    TYFL,  0,      0.f,    N,   N),
CULT( "endDHWTANK",  ENDER, 0,                 0,     0, 0,        0,     0,      0.f,    N,   N),
CULT()
};	// dhwTankT

//============================= DHWPUMP command =============================
LOCAL RC FC wpStarCkf( CULT* c, /*DHWPUMP* */ void *p, void *p2, void *p3)
// called at end of DHWPUMP input, to get messages near source of error.
{
	return ((DHWPUMP*)p)->wp_CkF();
}		// wpStarCkf
//=============================================================================
static CULT dhwPumpT[] = //------ DHWPUMP cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   whStarCkf),
CULT( "wpDHWSYS",    DAT,   DHWPUMP_OWNTI,   NO_INP|RDFLIN,0,0, TYREF,    &WSiB,  0,     N,   N),
CULT( "wpMult",		 DAT,	DHWPUMP_MULT,	   0,     0, VEOI,   TYSI,    0,      1,      N, N),
CULT( "wpElecMtr",   DAT,   DHWPUMP_ELECMTRI,  0,     0, VEOI,	 TYREF,   &MtriB, N,      N, N),
CULT( "wpPwr",		 DAT,	DHWPUMP_PWR,	   0,     0, VHRLY,  TYFL,    0,      0.f,    N, N),
CULT( "endDHWPUMP",  ENDER, 0,                 0,     0, 0,      0,       0,      0.f,    N,   N),
CULT()
};	// dhwPumpT

//============================= DHWLOOPBRANCH command =============================
LOCAL RC FC wbStarCkf( CULT* c, /*DHWLOOPBRANCH* */ void *p, void *p2, void *p3)
// called at end of DHWLOOPBRANCH input, to get messages near source of error.
{
	return ((DHWLOOPBRANCH*)p)->wb_CkF();
}		// wbStarCkf
//=============================================================================
static CULT dhwLoopBranchT[] = //------ DHWLOOPBRANCH cmd RAT Entry table, used from dhwLoopSegT
{
// id                cs     fn                    f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------     -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                    0,       0, 0,      0,     0,      0.f,    N,   wbStarCkf),
CULT( "wbDHWLOOPSEG",DAT,   DHWLOOPBRANCH_OWNTI,  NO_INP|RDFLIN,0,0,  TYREF, &WGiB,  0,      N,   N),
CULT( "wbMult",		 DAT,	DHWLOOPBRANCH_MULT,	  0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "wbLength",	 DAT,	DHWLOOPBRANCH_LEN,	  0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "wbSize",		 DAT,	DHWLOOPBRANCH_SIZE,   0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "wbInsulK",	 DAT,	DHWLOOPBRANCH_INSULK, 0,       0, VEOI,   TYFL,  0,    0.02167f, N,   N),
CULT( "wbInsulThk",	 DAT,	DHWLOOPBRANCH_INSULTHK,0,      0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "wbFUA",	     DAT,	DHWLOOPBRANCH_FUA,     0,      0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "wbExH",		 DAT,	DHWLOOPBRANCH_EXH,     0,      0, VHRLY,  TYFL,  0,     1.5f,    N,   N),
CULT( "wbExCnd",     DAT,   DHWLOOPBRANCH_EXCND,   0,      0, VEOI,   TYCH,  0,      C_EXCNDCH_SPECT,    N,   N),
CULT( "wbAdjZn",     DAT,   WBBC(ZI),              0,      0, VEOI,   TYREF, &ZiB,   0,      N,   N),
CULT( "wbExTX",      DAT,   WBBC(TXA),             0,      0, VSUBHRLY,TYFL, 0,     70.f,    N,   N),
CULT( "wbExT",		 DAT,	DHWLOOPBRANCH_EXT,     0,      0, VHRLY,  TYFL,  0,     70.f,    N,   N),
CULT( "wbFlow",		 DAT,	DHWLOOPBRANCH_FLOW,    0,      0, VHRLY,  TYFL,  0,      2.f,    N,   N),
CULT( "wbFWaste",	 DAT,	DHWLOOPBRANCH_FWASTE,  0,      0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "endDHWLOOPBRANCH", ENDER, 0,                0,      0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwLoopBranchT

//============================= DHWLOOPSEG command =============================
LOCAL RC FC wgStarCkf( CULT* c, /*DHWLOOPSEG* */ void *p, void *p2, void *p3)
// called at end of DHWLOOPSEG input, to get messages near source of error.
{
	return ((DHWLOOPSEG*)p)->wg_CkF();
}		// wgStarCkf
//=============================================================================
static CULT dhwLoopSegT[] = //------ DHWLOOPSEG cmd RAT Entry table, used from dhwLoopT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wgStarCkf),
CULT( "wgDHWLOOP",   DAT,   DHWLOOPSEG_OWNTI,  NO_INP|RDFLIN,0,0,  TYREF, &WLiB,  0,      N,   N),
CULT( "wgTy",        DAT,   DHWLOOPSEG_TY,	   RQD,     0, VEOI,   TYCH,  0,      -1,     N,   N),
CULT( "wgLength",	 DAT,	DHWLOOPSEG_LEN,	   0,       0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "wgSize",		 DAT,	DHWLOOPSEG_SIZE,   RQD,     0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "wgInsulK",	 DAT,	DHWLOOPSEG_INSULK, 0,       0, VEOI,   TYFL,  0,    0.02167f, N,   N),
CULT( "wgInsulThk",	 DAT,	DHWLOOPSEG_INSULTHK,0,      0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "wgExH",		 DAT,	DHWLOOPSEG_EXH,     0,      0, VHRLY,  TYFL,  0,     1.5f,    N,   N),
CULT( "wgExT",		 DAT,	DHWLOOPSEG_EXT,     0,      0, VHRLY,  TYFL,  0,     70.f,    N,   N),
CULT( "wgFNoDraw",	 DAT,	DHWLOOPSEG_FNODRAW, 0,      0, VHRLY,  TYFL,  0,     1.f,     N,   N),
CULT( "dhwloopBranch", RATE,0,                  0,      0, 0,      0,     &WBiB,  N,      dhwLoopBranchT, N),
CULT( "endDHWLOOPSEG", ENDER, 0,                0,      0, 0,      0,     0,     0.f,     N,   N),
CULT()
};	// dhwLoopSegT

//============================= DHWLOOPPUMP command =============================
LOCAL RC FC wlpStarCkf( CULT* c, /*DHWLOOPPUMP */ void *p, void *p2, void *p3)
// called at end of DHWLOOPSEG input, to get messages near source of error.
{
	return ((DHWLOOPPUMP*)p)->wlp_CkF();
}		// wgStarCkf
//=============================================================================
static CULT dhwLoopPumpT[] = //------ DHWLOOPPUMP cmd RAT Entry table, used from dhwLoopT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wlpStarCkf),
CULT( "wlpDHWLOOP",  DAT,   DHWLOOPPUMP_OWNTI, NO_INP|RDFLIN,0,0,  TYREF, &WLiB,  0,      N,   N),
CULT( "wlpMult",	 DAT,	DHWLOOPPUMP_MULT,	  0,     0, VEOI,  TYSI,    0,      1,      N, N),
CULT( "wlpElecMtr",  DAT,   DHWLOOPPUMP_ELECMTRI, 0,     0, VEOI,  TYREF,   &MtriB, N,      N, N),
CULT( "wlpPwr",		 DAT,	DHWLOOPPUMP_PWR,	  0,     0, VHRLY, TYFL,    0,      0.f,    N, N),
CULT( "wlpLiqHeatF",  DAT,	DHWPUMP_LIQHEATF,     0,     0, VHRLY, TYFL,    0,      1.f,    N, N),
CULT( "endDHWLOOPPUMP", ENDER, 0,                 0,     0, 0,     0,       0,   0.f,    N,   N),
CULT()
};	// dhwLoopPumpT

//============================= DHWLOOP command =============================
LOCAL RC FC wlStarCkf( CULT* c, /*DHWLOOP* */ void *p, void *p2, void *p3)
// called at end of DHWLOOPSEG input, to get messages near source of error.
{
	return ((DHWLOOP*)p)->wl_CkF();
}		// wlStarCkf
//=============================================================================
static CULT dhwLoopT[] = //------ DHWLOOP cmd RAT Entry table, used from dhwSysT
{
// id                cs     fn                 f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  -----------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,                 0,       0, 0,      0,     0,      0.f,    N,   wlStarCkf),
CULT( "wlDHWSYS",    DAT,   DHWLOOP_OWNTI,     NO_INP|RDFLIN,0,0,  TYREF, &WSiB,  0,      N,   N),
CULT( "wlMult",		 DAT,	DHWPUMP_MULT,	   0,       0, VEOI,   TYSI,  0,      1,      N,   N),
CULT( "wlFlow",	     DAT,	DHWLOOP_FLOW,      0,       0, VHRLY,  TYFL,  0,      6.f,    N,   N),
CULT( "wlTIn1",	     DAT,	DHWLOOP_TIN1,      0,       0, VHRLY,  TYFL,  0,    130.f,    N,   N),
CULT( "wlRunF",	     DAT,	DHWLOOP_RUNF,      0,       0, VHRLY,  TYFL,  0,      1.f,    N,   N),
CULT( "wlFUA",	     DAT,	DHWLOOP_FUA,       0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "wlLossMakeupPwr",DAT,DHWLOOP_LOSSMAKEUPPWR, 0,   0, VHRLY,  TYFL,  0,      0.f,    N,   N),
CULT( "wlLossMakeupEff",DAT,DHWLOOP_LOSSMAKEUPEFF, 0,   0, VHRLY,  TYFL,  0,      1.f,    N,   N),
CULT( "wlElecMtr",   DAT,   DHWLOOP_ELECMTRI,  0,       0, VEOI,   TYREF,   &MtriB, N,      N, N),
CULT( "dhwloopSeg",  RATE,  0,                 0,       0, 0,      0,     &WGiB,  N,      dhwLoopSegT, N),
CULT( "dhwloopPump", RATE,  0,                 0,       0, 0,      0,     &WLPiB, N,      dhwLoopPumpT, N),
CULT( "endDHWLOOP",  ENDER, 0,                 0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwLoopT

//============================== DHWSYS command ===============================
LOCAL RC FC wsStarCkf( CULT* c, /*DHWSYS* */ void *p, void *p2, void *p3)
// called at end of DHWSYS input, to get messages near source of error.
{
	return ((DHWSYS*)p)->ws_CkF();
}		// wsStarCkf
//=============================================================================
#define FXC( eu) (DHWSYS_FXCOUNT+eu)
#define FDX( eu) (DHWSYS_DRAWWASTE+eu)
#define FDF( eu) (DHWSYS_DRAWDURF+eu)
#define DWF( eu) (DHWSYS_DAYWASTEDRAWF+eu)

static CULT dhwSysT[] = //------ DHWSYS cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,               0,       0, 0,      0,     0,      0.f,    N, wsStarCkf),
CULT( "wsCentralDHWSYS",DAT,DHWSYS_CENTRALDHWSYSI,0,  0, VEOI,   TYREF, &WSiB,  N,      N, N),
CULT( "wsLoadShareDHWSYS",DAT,DHWSYS_LOADSHAREDHWSYSI,0, 0,VEOI, TYREF, &WSiB,  N,      N, N),
CULT( "wsCalcMode",  DAT,   DHWSYS_CALCMODE, 0,       0, VEOI,   TYCH,  0,      C_WSCALCMODECH_SIM, N, N),
CULT( "wsMult",		 DAT,   DHWSYS_MULT,	 0,       0, VEOI,   TYFL,  0,      1.f,	N, N),
CULT( "wsTInlet",	 DAT,   DHWSYS_TINLET,	 0,       0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "wsUse",		 DAT,   DHWSYS_HWUSE,	 0,       0, VHRLY,  TYFL,  0,      0.f,	N, N),
CULT( "wsTUse",		 DAT,   DHWSYS_TUSE,	 0,       0, VHRLY,  TYFL,  0,    120.f,	N, N),
CULT( "wsTSetpoint", DAT,   DHWSYS_TSETPOINT,0,       0, VHRLY,  TYFL,  0,    120.f,	N, N),
CULT( "wsTSetpointLH",DAT,  DHWSYS_TSETPOINTLH,0,     0, VHRLY,  TYFL,  0,    120.f,	N, N),
CULT( "wsDayUse",    DAT,   DHWSYS_DAYUSENAME,0,      0, VDAILY, TYSTR, 0,      N,      N, N),
CULT( "wsWHhwMtr",   DAT,   DHWSYS_WHHWMTRI, 0,       0, VEOI,	 TYREF, &WMtriB,N,      N, N),
CULT( "wsFXhwMtr",   DAT,   DHWSYS_FXHWMTRI, 0,       0, VEOI,	 TYREF, &WMtriB,N,      N, N),
CULT( "wsParElec",	 DAT,   DHWSYS_PARELEC,  0,       0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "wsSolarSys",  DAT,   DHWSYS_SWTI,     0,       0, VEOI,	 TYREF, &SWHiB,	N,      N, N),
CULT( "wsSDLM",      DAT,   DHWSYS_SDLM,     0,       0, VEOI,   TYFL,  0,      1.f,    N, N),
CULT( "wsDSM",       DAT,   DHWSYS_DSM,      0,       0, VEOI,   TYFL,  0,      1.f,    N, N),
CULT( "wsSSF",       DAT,   DHWSYS_SSF,      0,       0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "wsWF",        DAT,   DHWSYS_WF,       0,       0, VHRLY,  TYFL,  0,      1.f,    N, N),

CULT( "wsFaucetCount",DAT,  FXC(C_DHWEUCH_FAUCET), 0, 0, VEOI,   TYSI,  0,      1,      N, N),
CULT( "wsShowerCount",DAT,  FXC(C_DHWEUCH_SHOWER), 0, 0, VEOI,   TYSI,  0,      1,      N, N),
CULT( "wsBathCount",  DAT,  FXC(C_DHWEUCH_BATH),   0, 0, VEOI,   TYSI,  0,      1,      N, N),
CULT( "wsCWashrCount",DAT,  FXC(C_DHWEUCH_CWASHR), 0, 0, VEOI,   TYSI,  0,      1,      N, N),
CULT( "wsDWashrCount",DAT,  FXC(C_DHWEUCH_DWASHR), 0, 0, VEOI,   TYSI,  0,      1,      N, N),

CULT( "wsBranchModel",DAT,  DHWSYS_BRANCHMODEL,    0, 0, VEOI,   TYCH,  0, C_DHWBRANCHMODELCH_T24DHW, N, N),

CULT("wsUnkDrawDurF",    DAT, FDF( 0),                0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsFaucetDrawDurF", DAT, FDF(C_DHWEUCH_FAUCET), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsShowerDrawDurF", DAT, FDF(C_DHWEUCH_SHOWER), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsBathDrawDurF",   DAT, FDF(C_DHWEUCH_BATH),   0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsCWashrDrawDurF", DAT, FDF(C_DHWEUCH_CWASHR), 0, 0, VHRLY,  TYFL,  0,      1.f,    N, N),
CULT("wsDWashrDurF",     DAT, FDF(C_DHWEUCH_DWASHR), 0, 0, VHRLY,  TYFL,  0,      1.f,    N, N),

CULT("wsUnkDrawWaste",    DAT,  FDX(0),                0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsFaucetDrawWaste", DAT,  FDX(C_DHWEUCH_FAUCET), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsShowerDrawWaste", DAT,  FDX(C_DHWEUCH_SHOWER), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsBathDrawWaste",   DAT,  FDX(C_DHWEUCH_BATH),   0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsCWashrDrawWaste" ,DAT,  FDX(C_DHWEUCH_CWASHR), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT("wsDWashrDrawWaste", DAT,  FDX(C_DHWEUCH_DWASHR), 0, 0, VHRLY,  TYFL,  0,      0.f,    N, N),

CULT("wsUnkDayWasteF",   DAT,   DWF( 0),               0, 0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT("wsFaucetDayWasteF",DAT,   DWF(C_DHWEUCH_FAUCET), 0, 0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT("wsShowerDayWasteF",DAT,   DWF(C_DHWEUCH_SHOWER), 0, 0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT("wsBathDayWasteF",  DAT,   DWF(C_DHWEUCH_BATH),   0, 0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT("wsCWashrDayWasteF",DAT,   DWF(C_DHWEUCH_CWASHR), 0, 0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT("wsDWashrDayWasteF",DAT,   DWF(C_DHWEUCH_DWASHR), 0, 0, VEOI,  TYFL,  0,      0.f,    N, N),

CULT( "wsDayWasteVol",    DAT, DHWSYS_DAYWASTEVOL,   0, 0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "wsDayWasteBranchVolF",DAT,DHWSYS_DAYWASTEBRANCHVOLF,
												   0, 0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT("wsDrawMaxDur", DAT,   DHWSYS_DRAWMAXDUR,     0, 0, VEOI,  TYSI,  0,      4,    N, N),
CULT("wsLoadMaxDur", DAT,   DHWSYS_LOADMAXDUR,     0, 0, VEOI,  TYSI,  0,     12,    N, N),

CULT( "wsElecMtr",   DAT,   DHWSYS_ELECMTRI, 0,       0, VEOI,	 TYREF, &MtriB, N,      N, N),
CULT( "wsFuelMtr",   DAT,   DHWSYS_FUELMTRI, 0,       0, VEOI,	 TYREF, &MtriB, N,      N, N),
CULT( "wsWriteDrawCSV",DAT, DHWSYS_DRAWCSV,  0,       0, VEOI,   TYCH,  0,      C_NOYESCH_NO, N, N),
CULT( "dhwheater",   RATE,  0,               0,       0, 0,      0,     &WHiB,  N,      dhwHeaterT, N),
CULT( "dhwloopheater",RATE, 0,               0,       0, 0,      0,     &WLHiB, N,      dhwHeaterT, N),
CULT( "dhwtank",     RATE,  0,               0,       0, 0,      0,     &WTiB,  N,      dhwTankT, N),
CULT( "dhwpump",     RATE,  0,               0,       0, 0,      0,     &WPiB,  N,      dhwPumpT, N),
CULT( "dhwloop",     RATE,  0,               0,       0, 0,      0,     &WLiB,  N,      dhwLoopT, N),
CULT( "dhwheatrec",  RATE,  0,               0,       0, 0,      0,     &WRiB,  N,      dhwHeatRecT, N),

CULT( "endDHWSYS",   ENDER, 0,               0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// dhwSysT
#undef FXC
#undef FDX
#undef FDF
#undef DWF

//============================== DHWSOLARCOLLECTOR command ===============================
LOCAL RC FC scStarCkf(CULT* c, /*DHWSOLARCOLLECTOR* */ void *p, void *p2, void *p3)
// called at end of DHWSOLARSYS input, to get messages near source of error.
{
	return ((DHWSOLARCOLLECTOR*)p)->sc_CkF();
}		// scStarCkf
//=============================================================================

static CULT dhwSolColT[] = //------ DHWSOLARCOLLECTOR cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,               0,       0, 0,      0,      0,      0.f,    N, scStarCkf),
CULT( "scDHWSOLARSYS", DAT, DHWSOLARCOLLECTOR_OWNTI,   NO_INP|RDFLIN,0,0,TYREF, &SWHiB,  0,     N,   N),
CULT( "scArea",      DAT,   DHWSOLARCOLLECTOR_AREA,   RQD,      0, VEOI, TYFL,  0,      0.f,    N, N),
CULT( "scMult",      DAT,   DHWSOLARCOLLECTOR_MULT,    0,       0, VEOI, TYFL,  0,      1.f,    N, N),
CULT( "scTilt",      DAT,   DHWSOLARCOLLECTOR_TILT,   RQD,      0, VEOI, TYFL,  0,      0.f,    N, N),
CULT( "scAzm",       DAT,   DHWSOLARCOLLECTOR_AZM,    RQD,      0, VEOI, TYFL,  0,      0.f,    N, N),
CULT( "scFRUL",      DAT,   DHWSOLARCOLLECTOR_TESTFRUL, 0,     0, VEOI,  TYFL,  0,     -0.727f, N, N),
CULT( "scFRTA",      DAT,   DHWSOLARCOLLECTOR_TESTFRTA, 0,     0, VEOI,  TYFL,  0,      0.758f, N, N),
CULT( "scTestMassFlow", DAT,DHWSOLARCOLLECTOR_TESTMASSFLOW, 0, 0, VEOI,  TYFL,  0,      14.79f, N, N),
CULT( "scKta60",     DAT,   DHWSOLARCOLLECTOR_KTA60,   0,      0, VEOI,  TYFL,  0,      0.72f,  N, N),
CULT( "scOprMassFlow",DAT,  DHWSOLARCOLLECTOR_OPRMASSFLOW, 0,  0, VEOI,  TYFL,  0,       0.f,   N, N),
CULT( "scPipingLength",	 DAT, SCPIPE( LEN),	           0,      0, VEOI,  TYFL,  0,      0.f,    N,   N),
CULT( "scPipingInsulK",	 DAT, SCPIPE( INSULK),         0,      0, VEOI,  TYFL,  0,    0.02167f, N,   N),
CULT( "scPipingInsulThk",DAT, SCPIPE( INSULTHK),       0,      0, VEOI,  TYFL,  0,      1.f,    N,   N),
CULT( "scPipingExH",	 DAT, SCPIPE( EXH),            0,      0, VEOI,  TYFL,  0,     1.5f,    N,   N),
CULT( "scPipingExT", DAT,   DHWSOLARCOLLECTOR_PIPINGTEX, 0,    0, VHRLY, TYFL, 0,      70.f,   N,   N),
CULT( "scPumpPwr",   DAT,   DHWSOLARCOLLECTOR_PUMPPWR, 0,      0, VEOI,  TYFL,  0,      0.f,    N, N),
CULT( "scPumpLiqHeatF",DAT, DHWSOLARCOLLECTOR_PUMPLIQHEATF, 0, 0, VEOI,  TYFL,  0,      1.f,    N, N),
CULT( "scPumpOnDeltaT",DAT, DHWSOLARCOLLECTOR_PUMPONDELTAT, 0, 0, VEOI,  TYFL,  0,      5.f,    N, N),
CULT( "scPumpOffDeltaT",DAT,DHWSOLARCOLLECTOR_PUMPOFFDELTAT, 0,0, VEOI,  TYFL,  0,      1.f,     N, N),
CULT( "endDHWSOLARCOLLECTOR",   ENDER, 0,               0,     0, 0,      0,    0,      0.f,  N,   N),
CULT()
};	// dhwSolColT

//============================== DHWSOLARSYS command ===============================
LOCAL RC FC swStarCkf(CULT* c, /*DHWSOLARSYS* */ void *p, void *p2, void *p3)
// called at end of DHWSOLARSYS input, to get messages near source of error.
{
	return ((DHWSOLARSYS*)p)->sw_CkF();
}		// swStarCkf
//=============================================================================

static CULT dhwSolSysT[] = //------ DHWSOLARSYS cmd RAT Entry table, used from cnTopCult
{
// id                cs        fn                         f    uc evf     ty     b       dfls    p2   ckf
//-----------------  -----     ---------------            ---- -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,     0,                          0,  0, 0,      0,     0,      0.f,    N, swStarCkf),
CULT( "swElecMtr",   DAT,      DHWSOLARSYS_ELECMTRI,       0,  0, VEOI,   TYREF, &MtriB, N,      N, N),
CULT( "swEndUse",    DAT,      DHWSOLARSYS_ENDUSE,         0,  0, VEOI,   TYCH,  0,      C_ENDUSECH_DHW, N, N),
CULT( "swParElec",   DAT,      DHWSOLARSYS_PARELEC,        0,  0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "swSCFluidSpHt",DAT,     DHWSOLARSYS_SCFLUIDSPHT,    0,  0, VEOI,   TYFL,  0,     0.9f,    N, N),  // default = 35% (vol)
CULT( "swSCFluidDens",DAT,     DHWSOLARSYS_SCFLUIDDENS,    0,  0, VEOI,   TYFL,  0,     64.f,    N, N),  //   ethylene glycol
CULT( "swTankHXEff", DAT,      DHWSOLARSYS_TANKHXEFF,      0,  0, VEOI,   TYFL,  0,      0.7f,   N, N),
CULT( "swTankTHxLimit", DAT,   DHWSOLARSYS_TANKTHXLIMIT,   0,  0, VEOI,   TYFL,  0,     180.f,   N, N),
CULT( "swTankVol",   DAT,      DHWSOLARSYS_TANKVOL,        0,  0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "swTankUA",    DAT,      DHWSOLARSYS_TANKUA,         0,  0, VEOI,   TYFL,  0,     -1.f,    N, N),
CULT( "swTankInsulR", DAT,     DHWSOLARSYS_TANKINSULR,     0,  0, VEOI,   TYFL,  0,     12.f,    N, N),
CULT( "swTankZone",   DAT,     DHWSOLARSYS_TANKZNTI,       0,  0, VEOI,   TYREF, &ZiB,   N,      N, N),
CULT( "swTankTEx",   DAT,      DHWSOLARSYS_TANKTEX,        0,  0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "dhwsolarcollector",RATE,0,                          0,  0, 0,         0,  &SCiB,  N,     dhwSolColT, N),
CULT( "endDHWSOLARSYS", ENDER, 0,                          0,  0, 0,         0,  0,      0.f,    N,   N),
CULT()
};	// dhwSolSysT


//============================== PVARRAY command ===============================
LOCAL RC FC pvStarCkf( CULT* c, /*PVARRAY* */ void *p, void *p2, void *p3)
// called at end of PVARRAY input, to get messages near source of error.
{
	return ((PVARRAY*)p)->pv_CkF();
}		// pvStarCkf
//=============================================================================
#define PVG( m) (PVARRAY_G+SURFGEOM_##m)
static CULT pvArrT[] = //------ PVARRAY cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,               0,       0, 0,      0,     0,      0.f,    N, pvStarCkf),
CULT( "pvElecMtr",   DAT,   PVARRAY_ELECMTRI,  0,     0, VEOI,   TYREF, &MtriB, N,      N, N),
CULT( "pvEndUse",    DAT,   PVARRAY_ENDUSE,  0,       0, VEOI,   TYCH,  0,      C_ENDUSECH_PV, N, N),
CULT( "pvDCSysSize", DAT,   PVARRAY_DCCAP,   RQD,     0, VEOI,   TYFL,  0,      0.f,    N, N),
CULT( "pvModuleType", DAT,  PVARRAY_MODULETYPE, 0,    0, VEOI,   TYCH,  0,      C_PVMODCH_STD, N, N),
CULT( "pvCoverRefrInd", DAT, PVARRAY_COVREFRIND, 0,   0, VEOI,   TYFL,  0,      1.3f,   N, N),
CULT( "pvTempCoeff", DAT,   PVARRAY_TEMPCOEFF, 0,     0, VEOI,   TYFL,  0,      -0.00206f,  N, N),
CULT( "pvArrayType", DAT,   PVARRAY_ARRAYTYPE, 0,     0, VEOI,   TYCH,  0,      C_PVARRCH_FXDOR, N, N),
CULT( "pvTilt",      DAT,   PVARRAY_TILT,    0,       0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "pvAzm",       DAT,   PVARRAY_AZM,     0,       0, VHRLY,  TYFL,  0,      0.f,    N, N),
CULT( "pvGrndRefl",  DAT,   PVARRAY_GRNDREFL, 0,      0, VHRLY,  TYFL,  0,      0.2f,   N, N),
CULT( "pvGCR",       DAT,   PVARRAY_GCR,     0,       0, VEOI,   TYFL,  0,      0.4f,   N, N),
CULT( "pvDCtoACRatio", DAT, PVARRAY_DCACRAT, 0,       0, VEOI,   TYFL,  0,      1.2f,   N, N),
CULT( "pvInverterEff", DAT, PVARRAY_INVEFF,  0,       0, VEOI,   TYFL,  0,      0.96f,  N, N),
CULT( "pvSIF",       DAT,   PVARRAY_SIF,     0,       0, VEOI,   TYFL,  0,      1.2f,   N, N),
CULT( "pvSysLosses", DAT,   PVARRAY_SYSLOSS, 0,       0, VHRLY,  TYFL,  0,      0.14f,  N, N),
CULT( "pvMounting",  DAT,   PVG( MOUNTING),  0,       0, VEOI,   TYCH,  0,      C_MOUNTCH_BLDG, N, N),
CULT( "pvVertices",  DAT,   PVG( VRTINP),    ARRAY,   0, VEOI,   TYFL,  0,      0.f,    v DIM_POLYGONXYZ,  N),
CULT( "endPVARRAY",  ENDER, 0,               0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// pvArrT

//============================== BATTERY command ===============================
LOCAL RC FC btStarCkf(CULT* c, /*BATTERY* */ void *p, void *p2, void *p3)
// called at end of BATTERY input, to get messages near source of error.
{
	return ((BATTERY*)p)->bt_CkF();
}		// btStarCkf
//=============================================================================
static CULT btT[] = //------ BATTERY cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",            STAR,  0,               0,       0, 0,      0,     0,      0.f,    N,   btStarCkf),
CULT( "btMeter",      DAT,   BATTERY_METER,   0,       0, VEOI,   TYREF, &MtriB, N,      N,   N),
CULT( "btEndUse",     DAT,   BATTERY_ENDUSE,  0,       0, VEOI,   TYCH,  0,      C_ENDUSECH_BT, N, N),
CULT( "btChgEff",     DAT,   BATTERY_CHGEFF,  0,       0, VHRLY,  TYFL, 0,      0.975f, N,   N),
CULT( "btDschgEff",   DAT,   BATTERY_DSCHGEFF,0,       0, VHRLY,  TYFL, 0,      0.975f, N,   N),
CULT( "btMaxCap",     DAT,   BATTERY_MAXCAP,  0,       0, VEOI,   TYFL,  0,      6.f,    N,   N),
CULT( "btInitSOE",    DAT,   BATTERY_INITSOE, 0,       0, VEOI,   TYFL,  0,      1.f,    N,   N),
CULT( "btInitCycles", DAT,   BATTERY_INITCYCLES,0,     0, VEOI,   TYFL,  0,      0.f,    N,   N),
CULT( "btMaxChgPwr",  DAT,   BATTERY_MAXCHGPWR,0,      0, VHRLY,  TYFL, 0,      4.f,    N,   N),
CULT( "btMaxDschgPwr",DAT,   BATTERY_MAXDSCHGPWR,0,    0, VHRLY,  TYFL, 0,      4.f,    N,   N),
CULT( "btChgReq",     DAT,   BATTERY_CHGREQ,  0,       0, VHRLY|EVENDIVL,  TYFL, 0,      0.f,    N,   N),
CULT( "btUseUsrChg",  DAT,   BATTERY_USEUSRCHG,0,      0, VEOI,   TYCH, 0,      C_NOYESCH_NO, N, N),
CULT( "btControlAlg", DAT,   BATTERY_CONTROLALG,0,     0, VHRLY,  TYCH,  0,  nc( C_BATCTRLALGVC_DEFAULT), 0.f, N, N),
CULT( "endBATTERY",   ENDER, 0,               0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// btT

//============================== SHADEX command ===============================
LOCAL RC FC shStarCkf(CULT* c, /*SHADE* */ void *p, void *p2, void *p3)
// called at end of SHADE input, to get messages near source of error.
{
	return ((SHADEX*)p)->sx_CkF();
}		// shStarCkf
//=============================================================================
#define SXG( m) (SHADEX_G+SURFGEOM_##m)
static CULT shadexT[] = //------ SHADEX cmd RAT Entry table, used from cnTopCult
{
// id                cs     fn               f        uc evf     ty     b       dfls    p2   ckf
//-----------------  -----  ---------------  -------  -- ------  -----  ------  ------  ---- ----
CULT( "*",           STAR,  0,               0,       0, 0,      0,     0,      0.f,    N,   shStarCkf),
CULT( "sxMounting",  DAT,   SXG( MOUNTING),  0,       0, VEOI,   TYCH,  0,      C_MOUNTCH_SITE, N, N),
CULT( "sxVertices",  DAT,   SXG( VRTINP),    ARRAY,   0, VEOI,   TYFL,  0,      0.f,    v DIM_POLYGONXYZ,  N),
CULT( "endSHADEX",   ENDER, 0,               0,       0, 0,      0,     0,      0.f,    N,   N),
CULT()
};	// shadeT

//====================== AIRHANDLER command (Air Handler) ===========================
static CULT ahT[] = 
{
#define SFAN(m)  (AH_SFAN + FAN_##m)
#define RFAN(m)  (AH_RFAN + FAN_##m)
#define CCH(m)   (AH_CCH  + CCH_##m)			// crankcase heater subrecord member
#define AHHC(m)  (AH_AHHC + AHHEATCOIL_##m)
#define AHCC(m)  (AH_AHCC + COOLCOIL_##m)
//#define AHCCC(m) (AH_AHCC + COOLCOIL_##m)		// same, edit to AHCC when shaken down, later 7-92.

	// id              cs    fn                     f        uc evf     ty      b     dfls                   p2   ckf
	//---------------  ----  ---------------------  -------  -- ------  -----  -----  ---------------------  ---- ----
	CULT( "*",                STAR, 0,                     0,       0, 0,      0,      0,    N,             0.f,     N,   N),
	CULT( "ahSched",          DAT,  AH_AHSCH,              0,       0, VHRLY,  TYCH,   0,  nc(C_AHSCHVC_ON),0.f,     N,   N),
	CULT( "fanOversize",      DAT,  AH_FANOVERSIZE,        0,       0, VFAZLY, TYFL,   0,    N,             .1f,     N,   N), // 6-95
	CULT( "sfanType",         DAT,  SFAN(FANTY),           0,       0, VFAZLY, TYCH,   0,  v C_FANTYCH_DRAWTHRU,0,   N,   N),
	CULT( "sfanVfDs",         DAT,  SFAN(VFDS),         RQD|AS_OK,  0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "sfanVfMxF",        DAT,  SFAN(VFMXF),           0,       0, VFAZLY, TYFL,   0,    N,             1.3f,    N,   N),
	CULT( "sfanPress",        DAT,  SFAN(PRESS),           0,       0, VFAZLY, TYFL,   0,    N,             3.f,     N,   N),
	CULT( "sfanEff",          DAT,  SFAN(EFF),             0,       0, VFAZLY, TYFL,   0,    N,              .65f,   N,   N),
	CULT( "sfanShaftBhp",     DAT,  SFAN(SHAFTPWR),        0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "sfanElecPwr",      DAT,  SFAN(ELECPWR),         0,       0, VEOI,   TYFL,  0,     N,      0.f,      N,   N),
	CULT( "sfanMotEff",       DAT,  SFAN(MOTEFF),          0,       0, VFAZLY, TYFL,   0,    N,              .9f,    N,   N),
	CULT( "sfanMotPos",       DAT,  SFAN(MOTPOS),          0,       0, VFAZLY, TYCH,   0,  v C_MOTPOSCH_INFLO, 0,    N,   N),
	CULT( "sfanCurvePy",      DAT,  SFAN(CURVEPY+PYCUBIC_K),ARRAY,  0, VFAZLY, TYFL,   0,    N,               0.f,  v 6,  N), //incl x0
	CULT( "sfanMtr",          DAT,  SFAN(MTRI),            0,       0, VEOI,   TYREF, &MtriB,N,               0.f,   N,   N),
	CULT( "rfanType",         DAT,  RFAN(FANTY),           0,       0, VFAZLY, TYCH,   0,  v C_FANTYCH_NONE,  0.f,   N,   N),
	CULT( "rfanVfDs",         DAT,  RFAN(VFDS),            AS_OK,   0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "rfanVfMxF",        DAT,  RFAN(VFMXF),           0,       0, VFAZLY, TYFL,   0,    N,             1.3f,    N,   N),
	CULT( "rfanPress",        DAT,  RFAN(PRESS),           0,       0, VFAZLY, TYFL,   0,    N,              .75f,   N,   N),
	CULT( "rfanEff",          DAT,  RFAN(EFF),             0,       0, VFAZLY, TYFL,   0,    N,              .65f,   N,   N),
	CULT( "rfanShaftBhp",     DAT,  RFAN(SHAFTPWR),        0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "rfanElecPwr",      DAT,  RFAN(ELECPWR),         0,       0, VEOI,   TYFL,   0,    N,      0.f,      N,   N),
	CULT( "rfanMotEff",       DAT,  RFAN(MOTEFF),          0,       0, VFAZLY, TYFL,   0,    N,              .9f,    N,   N),
	CULT( "rfanMotPos",       DAT,  RFAN(MOTPOS),          0,       0, VFAZLY, TYCH,   0,  v C_MOTPOSCH_INFLO,  0,   N,   N),
	CULT( "rfanCurvePy",      DAT,  RFAN(CURVEPY+PYCUBIC_K),ARRAY,  0, VFAZLY, TYFL,   0,    N,               0.f,  v 6,  N), //incl x0
	CULT( "rfanMtr",          DAT,  RFAN(MTRI),            0,       0, VEOI,   TYREF, &MtriB,N,               0.f,   N,   N),
	CULT( "cchCM",            DAT,  CCH(CCHCM),            0,       0, VFAZLY, TYCH,   0,  v C_CCHCM_NONE,      0,   N,   N), //code dflts
	CULT( "cchPMx",           DAT,  CCH(PMX),              0,       0, VFAZLY, TYFL,   0,    N,                .4f,  N,   N),
	CULT( "cchPMn",           DAT,  CCH(PMN),              0,       0, VFAZLY, TYFL,   0,    N,                .04f, N,   N),
	CULT( "cchTMx",           DAT,  CCH(TMX),              0,       0, VFAZLY, TYFL,   0,    N,               0.f,   N,   N),
	CULT( "cchTMn",           DAT,  CCH(TMN),              0,       0, VFAZLY, TYFL,   0,    N,             150.f,   N,   N),
	CULT( "cchDT",            DAT,  CCH(DT),               0,       0, VFAZLY, TYFL,   0,    N,              20.f,   N,   N),
	CULT( "cchTOn",           DAT,  CCH(TON),              0,       0, VFAZLY, TYFL,   0,    N,              72.f,   N,   N),
	CULT( "cchTOff",          DAT,  CCH(TOFF),             0,       0, VFAZLY, TYFL,   0,    N,              72.f,   N,   N), //code dflts
	CULT( "cchMtr",           DAT,  CCH(MTRI),             0,       0, VEOI,   TYREF, &MtriB,N,            0.f,     N,   N),
	CULT( "coilOversize",     DAT,  AH_COILOVERSIZE,       0,       0, VFAZLY, TYFL,   0,    N,             .1f,     N,   N), //6-95
	CULT( "ahhcType",         DAT,  AHHC(COILTY),          0,       0, VFAZLY, TYCH,   0,  v C_COILTYCH_NONE,   0,   N,   N),
	CULT( "ahhcSched",        DAT,  AHHC(SCHED),           0,       0, VHRLY,  TYCH,   0, nc(C_OFFAVAILVC_AVAIL),0,  N,   N),
	CULT( "ahhcCaptRat",      DAT,  AHHC(CAPTRAT),         AS_OK,   0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N), //hrly vbl for ahhc only
	CULT( "ahhcHeatplant",    DAT,  AHHC(HPI),             0,       0, VEOI,   TYREF, &HpiB, N,             0.f,     N,   N),
	CULT( "ahhcEffR",         DAT,  AHHC(EFFRAT),          0,       0, VFAZLY, TYFL,   0,    N,             1.f,     N,   N),
	CULT( "ahhcEirR",         DAT,  AHHC(EIRRAT),          0,       0, VHRLY,  TYFL,   0,    N,             1.f,     N,   N), //hrly vbl for ahhc
	CULT( "ahhcPyEi",         DAT,  AHHC(PYEI+PYCUBIC_K),  ARRAY,   0, VFAZLY, TYFL,   0,    N,             0.f,    v 5,  N),
	CULT( "ahhcStackEffect",  DAT,  AHHC(STACKEFFECT),     0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahhcMtr",          DAT,  AHHC(MTRI),            0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahhcAuxOn",        DAT,  AHHC(AUXON),           0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahhcAuxOnMtr",     DAT,  AHHC(AUXONMTRI),       0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahhcAuxOff",       DAT,  AHHC(AUXOFF),          0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahhcAuxOffMtr",    DAT,  AHHC(AUXOFFMTRI),      0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahhcAuxOnAtall",   DAT,  AHHC(AUXONATALL),      0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahhcAuxOnAtallMtr",DAT,  AHHC(AUXONATALLMTRI),  0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahhcAuxFullOff",   DAT,  AHHC(AUXFULLOFF),      0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahhcAuxFullOffMtr",DAT,  AHHC(AUXFULLOFFMTRI),  0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahpCap17",         DAT,  AHHC(CAP17),           0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //RQD by code if AHP
	CULT( "ahpCap47",         DAT,  AHHC(CAP47),           0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //RQD by code if AHP
	CULT( "ahpCap35",         DAT,  AHHC(CAP35),           0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //code dflts
	CULT( "ahpFd35Df",        DAT,  AHHC(FD35DF),          0,       0, VFAZLY, TYFL,   0,    N,              .85f,   N,   N),
	CULT( "ahpCapIa",         DAT,  AHHC(CAPIA),           0,       0, VFAZLY, TYFL,   0,    N,              .004f,  N,   N),
	CULT( "ahpSupRh",         DAT,  AHHC(SUPRH),           0,       0, VFAZLY, TYFL,   0,    N,            10.f,     N,   N),
	CULT( "ahpTFrMn",         DAT,  AHHC(TFRMN),           0,       0, VFAZLY, TYFL,   0,    N,            17.f,     N,   N),
	CULT( "ahpTFrMx",         DAT,  AHHC(TFRMX),           0,       0, VFAZLY, TYFL,   0,    N,            47.f,     N,   N),
	CULT( "ahpTFrPk",         DAT,  AHHC(TFRPK),           0,       0, VFAZLY, TYFL,   0,    N,            42.f,     N,   N),
	CULT( "ahpDfrFMn",        DAT,  AHHC(DFRFMN),          0,       0, VFAZLY, TYFL,   0,    N,              .0222f, N,   N),
	CULT( "ahpDfrFMx",        DAT,  AHHC(DFRFMX),          0,       0, VFAZLY, TYFL,   0,    N,              .0889f, N,   N),
	CULT( "ahpDfrCap",        DAT,  AHHC(DFRCAP),          0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //code dflts
	CULT( "ahpDfrRh",         DAT,  AHHC(DFRRH),           0,       0, VFAZLY, TYFL,   0,    N,             5.f,     N,   N),
	CULT( "ahpTOff",          DAT,  AHHC(TOFF),            0,       0, VFAZLY, TYFL,   0,    N,             5.f,     N,   N),
	CULT( "ahpTOn",           DAT,  AHHC(TON),             0,       0, VFAZLY, TYFL,   0,    N,            12.f,     N,   N),
	CULT( "ahpIn17",          DAT,  AHHC(IN17),            0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //RQD by code for AHP
	CULT( "ahpIn47",          DAT,  AHHC(IN47),            0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //RQD by code for AHP
	CULT( "ahpInIa",          DAT,  AHHC(INIA),            0,       0, VFAZLY, TYFL,   0,    N,              .004f,  N,   N),
	CULT( "ahpCd",            DAT,  AHHC(CD),              0,       0, VFAZLY, TYFL,   0,    N,              .25f,   N,   N),
	CULT( "ahccSHR",          DAT,  AH_AHCCSHR,            0,       0, VFAZLY, TYFL,   0,    N,              .77f,   N,   N), // 6-95
	CULT( "ahccType",         DAT,  AHCC(COILTY),          0,       0, VFAZLY, TYCH,   0,  v C_COILTYCH_NONE,  0.f,  N,   N),
	CULT( "ahccSched",        DAT,  AHCC(SCHED),           0,       0, VHRLY,  TYCH,   0, nc(C_OFFAVAILVC_AVAIL),0,  N,   N),
	CULT( "ahccCaptRat",      DAT,  AHCC(CAPTRAT),         AS_OK,   0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahccCapsRat",      DAT,  AHCC(CAPSRAT),         0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahccK1",           DAT,  AHCC(K1),              0,       0, VFAZLY, TYFL,   0,    N,             -.4f,    N,   N),
	CULT( "ahccMinTEvap",     DAT,  AHCC(MINTEVAP),        0,       0, VFAZLY, TYFL,   0,    N,            35.f,     N,   N), // 40-->35 8-95
	CULT( "ahccDsTDbCnd",     DAT,  AHCC(DSTDBCND),        0,       0, VFAZLY, TYFL,   0,    N,            95.f,     N,   N),
	CULT( "ahccDsTDbEn",      DAT,  AHCC(DSTDBEN),         0,       0, VFAZLY, TYFL,   0,    N,            80.f,     N,   N),
	CULT( "ahccDsTWbEn",      DAT,  AHCC(DSTWBEN),         0,       0, VFAZLY, TYFL,   0,    N,            67.f,     N,   N),
	CULT( "ahccVfR",          DAT,  AHCC(VFR),             0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N), //code dflts
	CULT( "ahccVfRperTon",    DAT,  AHCC(VFRPERTON),       0,       0, VFAZLY, TYFL,   0,    N,           400.f,     N,   N), //8-28-95
#if 0
	x "ahccEirR",         DAT,  AHCC(EIRRAT),          0,       0, VFAZLY, TYFL,   0,    N,             1.f,     N,   N), //code dflts
#else// more vbl for something Bruce is doing 11-95
	CULT( "ahccEirR",         DAT,  AHCC(EIRRAT),          0,       0, VHRLY,  TYFL,   0,    N,             1.f,     N,   N), //code dflts
	// note ahccEirR VHRLY cuz easy & convenient to Bruce 11-95; VINITLY would actually be enuf for him if need to restruct.
#endif
	CULT( "ahccMinUnldPlr",   DAT,  AHCC(MINUNLDPLR),      0,       0, VFAZLY, TYFL,   0,    N,             1.f,   v 6,   N),
	CULT( "ahccMinFsldPlr",   DAT,  AHCC(MINFSLDPLR),      0,       0, VFAZLY, TYFL,   0,    N,             1.f,   v 6,   N),
//note py's are defaulted in code in cncult5.cpp.
	CULT( "pydxCaptT",        DAT,  AHCC(PYDXCAPTT+PYBIQUAD_K),ARRAY,0,VFAZLY, TYFL,   0,    N,             0.f,   v 7,   N),
	CULT( "pydxCaptF",        DAT,  AHCC(PYDXCAPTF+PYCUBIC_K),ARRAY,0, VFAZLY, TYFL,   0,    N,             0.f,   v 5,   N),
	CULT( "pydxCaptFLim",     DAT,  AHCC(PYDXCAPTFLIM),    0,       0, VFAZLY, TYFL,   0,    N,             1.05f,   N,   N), // 8-28-95
#if 0//may be restored re V-H-9
	x"pydxCapsT",        DAT,  AHCC(PYDXCAPST+PYBIQUAD_K),ARRAY,0,VFAZLY, TYFL,   0,    N,             0.f,   v 7,   N),
	x"pydxCapsF",        DAT,  AHCC(PYDXCAPSF+PYCUBIC_K),ARRAY,0, VFAZLY, TYFL,   0,    N,             0.f,   v 5,   N),
#endif
	CULT( "pydxEirT",         DAT,  AHCC(PYDXEIRT+PYBIQUAD_K),ARRAY,0, VFAZLY, TYFL,   0,    N,             0.f,   v 7,   N),
	CULT( "pydxEirUl",        DAT,  AHCC(PYDXEIRUL+PYCUBIC_K),ARRAY,0, VFAZLY, TYFL,   0,    N,             0.f,   v 5,   N),
	CULT( "ahccCoolplant",    DAT,  AHCC(CPI),             0,       0, VEOI,   TYREF, &CpiB, N,             0.f,     N,   N),
	CULT( "ahccGpmDs",        DAT,  AHCC(GPMDS),           0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahccNtuoDs",       DAT,  AHCC(NTUODS),          0,       0, VFAZLY, TYFL,   0,    N,             2.f,     N,   N),
	CULT( "ahccNtuiDs",       DAT,  AHCC(NTUIDS),          0,       0, VFAZLY, TYFL,   0,    N,             2.f,     N,   N),
	CULT( "ahccBypass",       DAT,  AH_AHCCBYPASS,         0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahFanCycles",      DAT,  AH_AHFANCYCLES,        0,       0, VHRLY,  TYCH,   0,  nc(C_NOYESVC_NO), 0.f,    N,   N),
	CULT( "ahccMtr",          DAT,  AHCC(MTRI),            0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahccAuxOn",        DAT,  AHCC(AUXON),           0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahccAuxOnMtr",     DAT,  AHCC(AUXONMTRI),       0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahccAuxOff",       DAT,  AHCC(AUXOFF),          0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahccAuxOffMtr",    DAT,  AHCC(AUXOFFMTRI),      0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahccAuxOnAtall",   DAT,  AHCC(AUXONATALL),      0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahccAuxOnAtallMtr",DAT,  AHCC(AUXONATALLMTRI),  0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahccAuxFullOff",   DAT,  AHCC(AUXFULLOFF),      0,       0, VHRLY,  TYFL,   0,     N,            0.f,     N,   N),
	CULT( "ahccAuxFullOffMtr",DAT,  AHCC(AUXFULLOFFMTRI),  0,       0, VEOI,   TYREF, &MtriB, N,            0.f,     N,   N),
	CULT( "ahTsSp",           DAT,  AH_AHTSSP,             0,       0, VHRLY,  TYNC,   0,    N,             0.f,     N,   N),
// next 2 RQD at runtime if ahTsSp==RA, or (5-95) if ahTsSp==ZN and  ahFanCyles==YES.
	CULT( "ahTsMn",           DAT,  AH_AHTSMN,             0,       0, VHRLY,  TYFL,   0,    N,            40.f,     N,   N), //dfl 0-->40 4-95
	CULT( "ahTsMx",           DAT,  AH_AHTSMX,             0,       0, VHRLY,  TYFL,   0,    N,           250.f,     N,   N), //dfl 999-->250 4-95
	CULT( "ahTsRaMn",         DAT,  AH_AHTSRAMN,           0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahTsRaMx",         DAT,  AH_AHTSRAMX,           0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N),
	CULT( "ahTsDsH",          DAT,  AH_AHTSDSH,            0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N), //dfl in runtime hrly code to ahTsMx. 6-95.
	CULT( "ahTsDsC",          DAT,  AH_AHTSDSC,            0,       0, VHRLY,  TYFL,   0,    N,             0.f,     N,   N), //dfl in runtime hrly code to ahTsMn. 6-95.
	CULT( "ahCtu",            DAT,  AH_AHCTU,              0,       0, VEOI,   TYREF, &TuB,  N,             0.f,     N,   N),
#if 0 //in use til 9-92
	x "ahWzCzns",         DAT,  AH_AHWZCZNS,           ARRAY,   0, VEOI,   TYREF, &ZiB, v TI_ALL,       0.f,   v 16,  N),
	x "ahCzCzns",         DAT,  AH_AHCZCZNS,           ARRAY,   0, VEOI,   TYREF, &ZiB, v TI_ALL,       0.f,   v 16,  N),
#else //looks like needs ALL_OK to enable ALL and ALL_BUT per manual  (not tested). 9-23-92.
	CULT( "ahWzCzns",         DAT,  AH_AHWZCZNS,     ARRAY|ALL_OK,  0, VEOI,   TYREF, &ZiB, v TI_ALL,       0.f,   v 16,  N),
	CULT( "ahCzCzns",         DAT,  AH_AHCZCZNS,     ARRAY|ALL_OK,  0, VEOI,   TYREF, &ZiB, v TI_ALL,       0.f,   v 16,  N),
#endif
	CULT( "oaMnCtrl",         DAT,  AH_OAMNCM,             0,       0, VFAZLY, TYCH,   0,   v C_OAMNCH_VOL, 0.f,     N,   N),
	CULT( "oaMnFrac",         DAT,  AH_OAMNFRAC,           0,       0, VHRLY,  TYFL,   0,    N,             1.f,     N,   N),
	CULT( "oaVfDsMn",         DAT,  AH_OAVFDSMN,           0,       0, VFAZLY, TYFL,   0,    N,             0.f,     N,   N),
	CULT( "oaEcoType",        DAT,  AH_OAECOTY,            0,       0, VFAZLY, TYCH,   0,  v C_ECOTYCH_NONE,0.f,     N,   N),
	CULT( "oaLimT",           DAT,  AH_OALIMT,             0,       0, VHRLY,  TYNC,   0,  nc(C_RANC_RA),   0.f,     N,   N),
	CULT( "oaLimE",           DAT,  AH_OALIME,             0,       0, VHRLY,  TYNC,   0,    N,           999.f,     N,   N),
	CULT( "oaOaLeak",         DAT,  AH_OAOALEAK,           0,       0, VFAZLY, TYFL,   0,    N,              .05f,   N,   N),
	CULT( "oaRaLeak",         DAT,  AH_OARALEAK,           0,       0, VFAZLY, TYFL,   0,    N,              .05f,   N,   N),
	CULT( "ahSOLeak",         DAT,  AH_AHSOLEAK,           0,       0, VFAZLY, TYFL,   0,    N,              .01f,   N,   N),
	CULT( "ahROLeak",         DAT,  AH_AHROLEAK,           0,       0, VFAZLY, TYFL,   0,    N,              .01f,   N,   N),
	CULT( "ahSOLoss",         DAT,  AH_AHSOLOSS,           0,       0, VFAZLY, TYFL,   0,    N,              .02f,   N,   N),
	CULT( "ahROLoss",         DAT,  AH_AHROLOSS,           0,       0, VFAZLY, TYFL,   0,    N,              .02f,   N,   N),
	CULT( "endAirhandler",    ENDER,0,                     0,       0, 0,      0,      0,    N,             0.f,     N,   N),
	CULT()
};	// ahT


//============================ BOILER command (subclass of HEATPLANT) ===========================

static CULT blrT[] =
{
#define BLRP(m) (BOILER_BLRP+PUMP_##m)
	// id              cs    fn                     f        uc evf     ty      b      dfls                  p2   ckf
	//---------------  ----  ---------------------  -------  -- ------  -----  -----  ---------------------  ---- ----
	CULT( "blrHeatplant",     DAT,  BOILER_OWNTI,    NO_INP|RDFLIN, 0, 0,      TYREF, &HpiB,  N,           0.f,     N,   N),
//boiler user inputs
	CULT( "blrCap",           DAT,  BOILER_BLRCAP,         RQD,     0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrEffR",          DAT,  BOILER_BLREFFR,        0,       0, VFAZLY, TYFL,   0,     N,           .80f,    N,   N),
	CULT( "blrEirR",          DAT,  BOILER_BLREIRR,        0,       0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrPyEi",          DAT,BOILER_BLRPYEI+PYCUBIC_K, ARRAY,  0, VFAZLY, TYFL,   0,     N,           0.f,    v 5,  N),
	CULT( "blrMtr",           DAT,  BOILER_MTRI,           0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "blrpGpm",          DAT,  BLRP(GPM),             0,       0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N), //code dflts. BRUCEDFL, was RQD.
	CULT( "blrpHdLoss",       DAT,  BLRP(HDLOSS),          0,       0, VFAZLY, TYFL,   0,     N,         114.45f,   N,   N), //BRUCEDFL, was 35.f/*??*/
	CULT( "blrpMotEff",       DAT,  BLRP(MOTEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .88f,    N,   N),
	CULT( "blrpHydEff",       DAT,  BLRP(HYDEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .70f,    N,   N),
	CULT( "blrpOvrunF",       DAT,  BLRP(OVRUNF),          NO_INP,  0, 0,      TYFL,   0,     N,           1.3f,    N,   N), //poss fut input
	CULT( "blrpMtr",          DAT,  BLRP(MTRI),            0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
//boiler auxiliaries user inputs, similar to those for a COIL.
	CULT( "blrAuxOn",        DAT,  BOILER_AUXON,           0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrAuxOnMtr",     DAT,  BOILER_AUXONMTRI,       0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "blrAuxOff",       DAT,  BOILER_AUXOFF,          0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrAuxOffMtr",    DAT,  BOILER_AUXOFFMTRI,      0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "blrAuxOnAtall",   DAT,  BOILER_AUXONATALL,      0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrAuxOnAtallMtr",DAT,  BOILER_AUXONATALLMTRI,  0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "blrAuxFullOff",   DAT,  BOILER_AUXFULLOFF,      0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "blrAuxFullOffMtr",DAT,  BOILER_AUXFULLOFFMTRI,  0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "endBoiler",       ENDER, 0,                     0,       0, 0,      0,      0,     N,           0.f,     N,   N),
	CULT()
};	// blrT


//============================ HEATPLANT command (subclass of TOP) ===========================

static CULT hpT[] =
{
	// id              cs    fn                     f        uc evf      ty      b      dfls                  p2   ckf
	//---------------  ----  ---------------------  -------  -- -------  -----  -----  ---------------------  ---- ----
//heatplant user inputs
	CULT( "hpSched",          DAT, HEATPLANT_HPSCHED,      0,        0, VHRLY,  TYCH,   0, nc(C_OFFAVAILONVC_AVAIL),0.f, N, N),
	CULT( "hpPipeLossF",      DAT, HEATPLANT_HPPIPELOSSF,  0,        0, VFAZLY, TYFL,   0,     N,          .01f,     N,   N),
	CULT( "hpStage1",         DAT, HEATPLANT_HPSTAGE1, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage2",         DAT, HEATPLANT_HPSTAGE2, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage3",         DAT, HEATPLANT_HPSTAGE3, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage4",         DAT, HEATPLANT_HPSTAGE4, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage5",         DAT, HEATPLANT_HPSTAGE5, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage6",         DAT, HEATPLANT_HPSTAGE6, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
	CULT( "hpStage7",         DAT, HEATPLANT_HPSTAGE7, ARRAY|ALL_OK, 0, VEOI,   TYREF, &BlriB, N,          0.f,     v 8,  N),
//heatplant subobject
	CULT( "boiler",          RATE, 0,                      NM_RQD,   0, 0,      0,     &BlriB, N,          0.f,     blrT, N),
	CULT( "endHeatplant",   ENDER, 0,                      0,        0, 0,      0,      0,     N,          0.f,      N,   N),
	CULT()
};	// hpT


//============================ CHILLER command (subclass of COOLPLANT) ===========================

static CULT chT[] =
{
#define CHPP(m) (CHILLER_CHPP+PUMP_##m)
#define CHCP(m) (CHILLER_CHCP+PUMP_##m)
	// id              cs    fn                     f        uc evf     ty      b      dfls                  p2   ckf
	//---------------  ----  ---------------------  -------  -- ------  -----  -----  ---------------------  ---- ----
	CULT( "chCoolplant",      DAT,  CHILLER_OWNTI,    NO_INP|RDFLIN, 0, 0,     TYREF, &CpiB,  N,           0.f,     N,   N),
//chiller user inputs
	CULT( "chCapDs",          DAT,  CHILLER_CHCAPDS,       RQD,     0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chTsDs",           DAT,  CHILLER_CHTSDS,        0,       0, VFAZLY, TYFL,   0,     N,          44.f,     N,   N),
	CULT( "chTcndDs",         DAT,  CHILLER_CHTCNDDS,      0,       0, VFAZLY, TYFL,   0,     N,          85.f,     N,   N),
	CULT( "chPyCapT",      DAT, CHILLER_CHPYCAPT+PYBIQUAD_K, ARRAY, 0, VFAZLY, TYFL,   0,     N,           0.f,    v 7,  N), //code dflts
	CULT( "chCop",            DAT,  CHILLER_CHCOP,         0,       0, VFAZLY, TYFL,   0,     N,           4.2f,    N,   N),
	CULT( "chEirDs",          DAT,  CHILLER_CHEIRDS,       0,       0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chPyEirT",      DAT, CHILLER_CHPYEIRT+PYBIQUAD_K, ARRAY, 0, VFAZLY, TYFL,   0,     N,           0.f,    v 7,  N), //code dflts
	CULT( "chPyEirUl",     DAT, CHILLER_CHPYEIRUL+PYCUBIC_K, ARRAY, 0, VFAZLY, TYFL,   0,     N,           0.f,    v 5,  N), //code dflts
	CULT( "chMinUnldPlr",     DAT,  CHILLER_CHMINUNLDPLR,  0,       0, VFAZLY, TYFL,   0,     N,           .1f,     N,   N),
	CULT( "chMinFsldPlr",     DAT,  CHILLER_CHMINFSLDPLR,  0,       0, VFAZLY, TYFL,   0,     N,           .1f,     N,   N),
	CULT( "chMotEff",         DAT,  CHILLER_CHMOTEFF,      0,       0, VFAZLY, TYFL,   0,     N,           1.f,     N,   N),
	CULT( "chMtr",            DAT,  CHILLER_MTRI,          0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
//primary pump inputs
	CULT( "chppGpm",          DAT,  CHPP(GPM),             0,       0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N), // code defaults
	CULT( "chppHdLoss",       DAT,  CHPP(HDLOSS),          0,       0, VFAZLY, TYFL,   0,     N,          57.22f,   N,   N), //BRUCEDFL. was 65.f/*??*/
	CULT( "chppMotEff",       DAT,  CHPP(MOTEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .88f,    N,   N),
	CULT( "chppHydEff",       DAT,  CHPP(HYDEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .70f,    N,   N),
	CULT( "chppOvrunF",       DAT,  CHPP(OVRUNF),          0,       0, VFAZLY, TYFL,   0,     N,           1.3f,    N,   N), //this one inputtable
	CULT( "chppMtr",          DAT,  CHPP(MTRI),            0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
//condenser pump inputs
	CULT( "chcpGpm",          DAT,  CHCP(GPM),             0,       0, VFAZLY, TYFL,   0,     N,           0.f,     N,   N), // code defaults
	CULT( "chcpHdLoss",       DAT,  CHCP(HDLOSS),          0,       0, VFAZLY, TYFL,   0,     N,          45.78f,   N,   N), //BRUCEDFL. was 45.f/*??*/
	CULT( "chcpMotEff",       DAT,  CHCP(MOTEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .88f,    N,   N),
	CULT( "chcpHydEff",       DAT,  CHCP(HYDEFF),          0,       0, VFAZLY, TYFL,   0,     N,           .70f,    N,   N),
	CULT( "chcpOvrunF",       DAT,  CHCP(OVRUNF),          NO_INP,  0, 0,      TYFL,   0,     N,           1.3f,    N,   N), //poss fut input
	CULT( "chcpMtr",          DAT,  CHCP(MTRI),            0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
//chiller auxiliaries user inputs, similar to those for a COIL or a BOILER.
	CULT( "chAuxOn",          DAT,  CHILLER_AUXON,         0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chAuxOnMtr",       DAT,  CHILLER_AUXONMTRI,     0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "chAuxOff",         DAT,  CHILLER_AUXOFF,        0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chAuxOffMtr",      DAT,  CHILLER_AUXOFFMTRI,    0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "chAuxOnAtall",     DAT,  CHILLER_AUXONATALL,    0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chAuxOnAtallMtr",  DAT,  CHILLER_AUXONATALLMTRI,0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "chAuxFullOff",     DAT,  CHILLER_AUXFULLOFF,    0,       0, VHRLY,  TYFL,   0,     N,           0.f,     N,   N),
	CULT( "chAuxFullOffMtr",  DAT,  CHILLER_AUXFULLOFFMTRI,0,       0, VEOI,   TYREF, &MtriB, N,           0.f,     N,   N),
	CULT( "endChiller",       ENDER, 0,                    0,       0, 0,      0,      0,     N,           0.f,     N,   N),
	CULT()
};	// chT


//============================ COOLPLANT command (subclass of TOP) ===========================

static CULT cpT[] =
{
	// id              cs    fn                     f        uc evf      ty      b      dfls                  p2   ckf
	//---------------  ----  ---------------------  -------  -- -------  -----  -----  ---------------------  ---- ----
//coolplant user inputs
	CULT( "cpSched",          DAT, COOLPLANT_CPSCHED,      0,        0, VHRLY,  TYCH,   0, nc(C_OFFAVAILONVC_AVAIL),0.f, N, N),
	CULT( "cpTsSp",           DAT, COOLPLANT_CPTSSP,       0,        0, VHRLY,  TYFL,   0,     N,        44.f,       N,   N),
	CULT( "cpPipeLossF",      DAT, COOLPLANT_CPPIPELOSSF,  0,        0, VFAZLY, TYFL,   0,     N,          .01f,     N,   N),
	CULT( "cpTowerplant",     DAT, COOLPLANT_CPTOWI,       RQD,      0, VEOI,   TYREF, &TpiB,  N,          0.f,      N,   N),
	CULT( "cpStage1",         DAT, COOLPLANT_CPSTAGE1, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage2",         DAT, COOLPLANT_CPSTAGE2, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage3",         DAT, COOLPLANT_CPSTAGE3, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage4",         DAT, COOLPLANT_CPSTAGE4, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage5",         DAT, COOLPLANT_CPSTAGE5, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage6",         DAT, COOLPLANT_CPSTAGE6, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
	CULT( "cpStage7",         DAT, COOLPLANT_CPSTAGE7, ARRAY|ALL_OK, 0, VEOI,   TYREF, &ChiB,  N,          0.f,     v 8,  N),
//coolplant subobject
	CULT( "chiller",         RATE, 0,                      NM_RQD,   0, 0,      0,     &ChiB,  N,          0.f,     chT,  N),
	CULT( "endCoolplant",   ENDER, 0,                      0,        0, 0,      0,      0,     N,          0.f,      N,   N),
	CULT()
};	// cpT


//============================ TOWERPLANT command (subclass of TOP) ===========================

static CULT tpT[] =
{
#define TP(m) TOWERPLANT_##m
	// id              cs    fn                     f        uc evf     ty      b      dfls                  p2   ckf
	//---------------  ----  ---------------------  -------  -- ------  -----  -----  ---------------------  ---- ----
//towerplant user inputs
	CULT( "ctN",              DAT,  TP(CTN),               0,       0, VFAZLY, TYSI,  0,     v 1,       0.f,        N,   N),
	CULT( "tpStg",            DAT,  TP(TPSTG),             0,       0, VFAZLY, TYCH,  0, v C_TPSTGCH_TOGETHER, 0.f, N,   N),
	CULT( "tpTsSp",           DAT,  TP(TPTSSP),            0,       0, VHRLY,  TYFL,  0,      N,       85.f,        N,   N),
	CULT( "tpMtr",            DAT,  TP(TPMTR),             0,       0, VEOI,   TYREF, &MtriB, N,        0.f,        N,   N),
	CULT( "ctType",           DAT,  TP(CTTY),              0,       0, VFAZLY, TYCH,  0, v C_CTTYCH_ONESPEED, 0.f,  N,   N),
	CULT( "ctLoSpd",          DAT,  TP(CTLOSPD),           0,       0, VFAZLY, TYFL,  0,      N,        .5f,        N,   N),
	CULT( "ctShaftBhp",       DAT,  TP(CTSHAFTPWR),        0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code defaults (BRUCEDFL, was RQD)
	CULT( "ctMotEff",         DAT,  TP(CTMOTEFF),          0,       0, VFAZLY, TYFL,  0,      N,        .88f,       N,   N),
	CULT( "ctFcOne",          DAT,  TP(CTFCONE)+PYLINEAR_K,ARRAY,   0, VFAZLY, TYFL,  0,      N,        0.f,       v 3,  N),
	CULT( "ctFcLo",           DAT,  TP(CTFCLO)+PYLINEAR_K, ARRAY,   0, VFAZLY, TYFL,  0,      N,        0.f,       v 3,  N),
	CULT( "ctFcHi",           DAT,  TP(CTFCHI)+PYLINEAR_K, ARRAY,   0, VFAZLY, TYFL,  0,      N,        0.f,       v 3,  N),
	CULT( "ctFcVar",          DAT,  TP(CTFCVAR)+PYCUBIC_K, ARRAY,   0, VFAZLY, TYFL,  0,      N,        0.f,       v 5,  N),
// .. design conditions
	CULT( "ctCapDs",          DAT,  TP(CTCAPDS),           0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code dflts
	CULT( "ctVfDs",           DAT,  TP(CTVFDS),            0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code defaults (BRUCEDFL, was RQD)
	CULT( "ctGpmDs",          DAT,  TP(CTGPMDS),           0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code dflts
	CULT( "ctTDbODs",         DAT,  TP(CTTDBODS),          0,       0, VFAZLY, TYFL,  0,      N,       93.5f,       N,   N), //BRUCEDFL, was RQD
	CULT( "ctTWbODs",         DAT,  TP(CTTWBODS),          0,       0, VFAZLY, TYFL,  0,      N,       78.f,        N,   N), //BRUCEDFL, was RQD
	CULT( "ctTwoDs",          DAT,  TP(CTTWODS),           0,       0, VFAZLY, TYFL,  0,      N,       85.f,        N,   N),
// .. "off-design" conditions 			o
	CULT( "ctCapOd",          DAT,  TP(CTCAPOD),           0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code dflts
	CULT( "ctVfOd",           DAT,  TP(CTVFOD),            0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code defaults (BRUCEDFL, was conditionally RQD)
	CULT( "ctGpmOd",          DAT,  TP(CTGPMOD),           0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N), //code dflts
	CULT( "ctTDbOOd",         DAT,  TP(CTTDBOOD),          0,       0, VFAZLY, TYFL,  0,      N,       93.5f,       N,   N), //BRUCEDFL was conditionally RQD
	CULT( "ctTWbOOd",         DAT,  TP(CTTWBOOD),          0,       0, VFAZLY, TYFL,  0,      N,       78.f,        N,   N), //BRUCEDFL was conditionally RQD
	CULT( "ctTwoOd",          DAT,  TP(CTTWOOD),           0,       0, VFAZLY, TYFL,  0,      N,       85.f,        N,   N),
//
	CULT( "ctK",              DAT,  TP(CTK),               0,       0, VFAZLY, TYFL,  0,      N,        .4f,        N,   N), //or code dflts
	CULT( "ctStkFlFr",        DAT,  TP(CTSTKFLFR),         0,       0, VFAZLY, TYFL,  0,      N,       .18f,        N,   N),
	CULT( "ctBldn",           DAT,  TP(CTBLDN),            0,       0, VFAZLY, TYFL,  0,      N,       .01f,        N,   N),
	CULT( "ctDrft",           DAT,  TP(CTDRFT),            0,       0, VFAZLY, TYFL,  0,      N,        0.f,        N,   N),
	CULT( "ctTWm",            DAT,  TP(CTTWM),             0,       0, VFAZLY, TYFL,  0,      N,       60.f,        N,   N),
	CULT( "endTowerplant",  ENDER,  0,                     0,       0, 0,      0,     0,      N,        0.f,        N,   N),
	CULT()
};	// tpT


//================== HDAY command (note holidays are defaulted in cncult4.cpp) =================

static CULT hdayT[] =
{
// id               cs     fn               f        uc evf     ty      b     dfls        p2   ckf
//----------        -----  ---------------  -------  -- ------  ------  ----  ----------  ---- ----
CULT( "hdDateTrue",	DAT,   HDAY_HDDATETRUE,	0,       0, VEOI,   TYDOY,  0,    0.f,        N,   N),
CULT( "hdDateObs",	DAT,   HDAY_HDDATEOBS,	0,       0, VEOI,   TYDOY,  0,    0.f,        N,   N),
CULT( "hdOnMonday",	DAT,   HDAY_HDONMONDAY,	0,       0, VEOI,   TYCH,   0,    0.f,        N,   N), // defaulted YES in code
CULT( "hdCase",	    DAT,   HDAY_HDCASE,	    0,       0, VEOI,   TYCH,   0,    0.f,        N,   N),
CULT( "hdDow",	    DAT,   HDAY_HDDOW,	    0,       0, VEOI,   TYCH,   0,    0.f,        N,   N),
CULT( "hdMon",	    DAT,   HDAY_HDMON,	    0,       0, VEOI,   TYCH,   0,    0.f,        N,   N),
CULT( "endHoliday",  ENDER, 0,              0,       0, 0,      0,      0,    0.f,        N,   N),
CULT()
};	// hdayT

//================== DESCOND command =================

LOCAL RC FC dcStarCkf(CULT* c, /*DESCOND* */ void *p, void *p2, void *p3)
// called at end of DESCOND input, to get messages near source of error.
{
	return ((DESCOND*)p)->dc_CkF();
}		// dsStarCkf
//-----------------------------------------------------------------------------------------------
static CULT dcT[] =
{
// id               cs    fn               f        uc evf     ty      b     dfls        p2   ckf
//----------        ----- ---------------  -------  -- ------  ------  ----  ----------  ---- ----
CULT( "*",          STAR,  0,               0,       0, 0,      0,     0,      0.f,    N,   dcStarCkf),

CULT( "dcDay",	    DAT,  DESCOND_DOY,	    RQD,      0, VEOI,   TYDOY,  0,    200,    N,   N),

CULT( "dcDB",       DAT,  DESCOND_DB,       RQD,      0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcMCDBR",    DAT,  DESCOND_MCDBR,    RQD,      0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcMCWB",     DAT,  DESCOND_MCWB,     RQD,      0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcMCWBR",	DAT,  DESCOND_MCWBR,    RQD,      0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcWindSpeed",DAT,  DESCOND_WNDSPD,   RQD,      0, VEOI,   TYFL,   0,    0.f,       N,   N),
// Taub/Taud and Ebn/Ebh interact, see dc_TopDC
CULT( "dcTauB",     DAT,  DESCOND_TAUB,      0,       0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcTauD",     DAT,  DESCOND_TAUD,      0,       0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcEbnSlrNoon",DAT, DESCOND_EBNSLRNOON,0,       0, VEOI,   TYFL,   0,    0.f,       N,   N),
CULT( "dcEdhSlrNoon",DAT, DESCOND_EDHSLRNOON,0,       0, VEOI,   TYFL,   0,    0.f,       N,   N),


CULT( "endDesCond", ENDER, 0,           0,        0, 0,      0,      0,    0.f,       N,   N),
CULT()
};	// dcT

//================== INVERSE ===================================================
LOCAL RC FC ivStarCkf(CULT* c, /*DESCOND* */ void *p, void *p2, void *p3)
// called at end of DESCOND input, to get messages near source of error.
{
	return ((INVERSE*)p)->iv_CkF();
}		// dsStarCkf
//-----------------------------------------------------------------------------------------------
static CULT inverseT[] =
{
// id           cs    fn               f        uc evf     ty      b     dfls        p2   ckf
//----------    ----- ---------------  -------  -- ------  ------  ----  ----------  ---- ----
CULT("*",       STAR, 0,               0,       0, 0,      0,      0,    0.f,        N,   ivStarCkf),
CULT("ivFREQ",	DAT,  INVERSE_FREQ,	   RQD,     0, VEOI,   TYCH,   0,    -1,         N,   N),
CULT("ivX0",    DAT,  INVERSE_X0,      0,       0, VEOI,   TYFL,   0,    0.f,        N,   N),
CULT("ivY0",    DAT,  INVERSE_Y0,      0,       0, VEOI,   TYFL,   0,    0.f,        N,   N),
CULT("ivYTarg", DAT,  INVERSE_YTARG,   RQD,     0, VEOI,   TYFL,   0,    0.f,        N,   N),
CULT("ivX",     DAT,  INVERSE_X,       RQD,     0, VSUBHRLY | EVPSTIVL,
                                                           TYFL,   0,    0.f,        N,   N),
CULT("ivY",     DAT,  INVERSE_Y,       RQD,     0, VSUBHRLY | EVPSTIVL,
                                                           TYFL,   0,    0.f,        N,   N),
CULT("endInverse",ENDER, 0,            0,       0, 0,      0,      0,    0.f,        N,   N),
CULT()
};		// inverseT

//================== TOP LEVEL COMMANDS, used to access all other tables. ==================

CULT cnTopCult[] = 		// Top level table, points to all other tables, used in call to cul.cpp:cul(), declared in cncult.h.
{
	// id         cs     fn                 f           uc evf     ty     b      dfls                   p2   ckf
	//----------  -----  -----------------  -------     -- ------  -----  -----  ------------------     ---- ----
	CULT( "*",           STAR,  0,                 PRFP,       0, 0,      0,     0,    v topStarItf,0.f,     v topStarPrf,  //also called by CLEAR entry below; calls topStarPrf2.
	topStarPrf2), //main ckf ptr is used for re-entry pre-input fcn.
// TOP overall control
#ifdef AUTOSIZE
// ?? word to do main simulation??
	CULT( "doAutoSize",  DAT,   TOPRAT_CHAUTOSIZE, 0,          0, VEOI,   TYCH,  0,    C_NOYESCH_NO,      N,   N), // code defaults
	CULT( "doMainSim",   DAT,   TOPRAT_CHSIMULATE, 0,          0, VEOI,   TYCH,  0,    C_NOYESCH_YES,     N,   N),
#endif
	CULT( "begDay",      DAT,   TOPRAT_BEGDAY,     0,          0, VEOI,   TYDOY, 0,     1,                N,   N),
	CULT( "endDay",      DAT,   TOPRAT_ENDDAY,     0,          0, VEOI,   TYDOY, 0,     365,              N,   N),
	CULT( "jan1DoW",     DAT,   TOPRAT_JAN1DOW,    0,          0, VEOI,   TYCH,  0,     C_DOWCH_THU,      N,   N),
	CULT( "wuDays",      DAT,   TOPRAT_WUDAYS,     0,          0, VEOI,   TYSI,  0,     7,                N,   N),
	CULT( "nSubSteps",   DAT,   TOPRAT_NSUBSTEPS,  0,          0, VEOI,   TYSI,  0,     4,                N,   N),

// TOP location
	CULT( "wfName",      DAT,   TOPRAT_WFNAME,     RQD,        0, VFAZLY, TYSTR, 0,      N,    0.f,            N,   N),
	CULT( "TDVfName",    DAT,   TOPRAT_TDVFNAME,   0,          0, VFAZLY, TYSTR, 0,      N,    0.f,            N,   N),
	CULT( "elevation",   DAT,   TOPRAT_ELEVATION,  0,          0, VFAZLY, TYFL,  0,      N,    0.f,            N,   N), // defaulted from wthr file by code 1-95
	CULT( "refTemp",     DAT,   TOPRAT_REFTEMP,    0,          0, VFAZLY, TYFL,  0,      N,   60.f,            N,   N),
	CULT( "refRH",       DAT,   TOPRAT_REFRH,      0,          0, VFAZLY, TYFL,  0,      N,    .6f,            N,   N),
	CULT( "grndRefl",    DAT,   TOPRAT_GRNDREFL,   0,          0, VMHLY,  TYFL,  0,      N,    .2f,            N,   N),
	CULT( "grndEmit",    DAT,   TOPRAT_GRNDEMIT,   0,          0, VEOI,   TYFL,  0,      N,    .8f,            N,   N),
	CULT( "grndRf",      DAT,   TOPRAT_GRNDRF,     0,          0, VEOI,   TYFL,  0,      N,    .1f,            N,   N),
	CULT( "soilDiff",    DAT,   TOPRAT_SOILDIFF,   0,          0, VEOI,   TYFL,  0,      N,    .025f,          N,   N),
	CULT( "soilCond",    DAT,   TOPRAT_SOILCOND,   0,          0, VEOI,   TYFL,  0,      N,   1.0f,            N,   N),
	CULT( "soilSpHt",    DAT,   TOPRAT_SOILSPHT,   0,          0, VEOI,   TYFL,  0,      N,    .1f,            N,   N),
	CULT( "soilDens",    DAT,   TOPRAT_SOILDENS,   0,          0, VEOI,   TYFL,  0,      N,   115.0f,          N,   N),
	CULT( "farFieldWidth", DAT, TOPRAT_FARFIELDWIDTH, 0,       0, VEOI,   TYFL,  0,      N,   130.0f,          N,   N),
	CULT( "deepGrndCnd", DAT,   TOPRAT_DEEPGRNDCND, 0,         0, VEOI,   TYCH,  0,     C_DGCH_ZEROFLUX,       N,   N),
	CULT( "deepGrndDepth", DAT, TOPRAT_DEEPGRNDDEPTH, 0,       0, VEOI,   TYFL,  0,      N,   130.0f,          N,   N),
	CULT( "deepGrndT",   DAT,   TOPRAT_DEEPGRNDT,  0,          0, VHRLY,  TYFL,  0,      N,   50.0f,           N,   N), // defaulted from wthr file by code

// TOP check tolerances and precisions
	CULT( "tol",         DAT,   TOPRAT_TOL,        0,          0, VEOI,   TYFL,  0,      N,    .001f,          N,   N),
	CULT( "humTolF",     DAT,   TOPRAT_HUMTOLF,    0,          0, VEOI,   TYFL,  0,      N,    .0001f,         N,   N),
// TOP energy balance checking tolerances
	// 2-94 .00002
	// 1-95 try even smaller with double precision masses: .00001
	// 1-11 reverted to .0001 re minor unbals in CSE
	CULT( "ebTolMon",    DAT,   TOPRAT_EBTOLMON,   0,          0, VEOI,   TYFL,  0,      .0001f,         N,   N),
	CULT( "ebTolDay",    DAT,   TOPRAT_EBTOLDAY,   0,          0, VEOI,   TYFL,  0,      .0001f,         N,   N),
	CULT( "ebTolHour",   DAT,   TOPRAT_EBTOLHOUR,  0,          0, VEOI,   TYFL,  0,      .0001f,         N,   N),
	CULT( "ebTolSubhr",  DAT,   TOPRAT_EBTOLSUBHR, 0,          0, VEOI,   TYFL,  0,      .0001f,         N,   N),
// TOP Kiva calc settings
	CULT( "grndMinDim",  DAT,   TOPRAT_GRNDMINDIM, 0,          0, VEOI,   TYFL,  0,       .066f,         N,   N),
	CULT( "grndMaxGrthCoeff", DAT, TOPRAT_GRNDMAXGRTHCOEFF, 0, 0, VEOI,   TYFL,  0,        1.5f,         N,   N),
	CULT( "grndTimeStep", DAT,  TOPRAT_GRNDTIMESTEP, 0,        0, VEOI,   TYCH,  0,     C_TSCH_H,        N,   N),
// TOP ASHWAT calc trigger thresholds (1-2015)
	CULT( "AWTrigT",	 DAT,   TOPRAT_AWTRIGT,    0,          0, VEOI,   TYFL,  0,      1.f,            N,   N),
	CULT( "AWTrigSlr",   DAT,   TOPRAT_AWTRIGSLR,  0,          0, VEOI,   TYFL,  0,      .05f,           N,   N),
	CULT( "AWTrigH",	 DAT,   TOPRAT_AWTRIGH,    0,          0, VEOI,   TYFL,  0,      .1f,            N,   N),
// TOP AirNet convergence tolerances (1-2015)
	CULT( "ANTolAbs",	 DAT,   TOPRAT_ANTOLABS,   0,          0, VEOI,   TYFL,  0,      0.00125f,       N,   N),
	CULT( "ANTolRel",    DAT,   TOPRAT_ANTOLREL,   0,          0, VEOI,   TYFL,  0,      0.0001f,        N,   N),
// TOP other
	CULT( "bldgAzm",     DAT,   TOPRAT_BLDGAZM,    0,          0, VEOI,   TYFL,  0,      0.f,            N,   N),
	CULT( "skymodel",    DAT,   TOPRAT_SKYMODEL,   0,          0, VEOI,   TYCH,  0,   C_SKYMODCH_ANISO,  N,   N),
	CULT( "skymodelLW",  DAT,   TOPRAT_SKYMODELLW, 0,          0, VEOI,   TYCH,  0,   C_SKYMODLWCH_DEFAULT, N, N),
	CULT( "exShadeModel",DAT,   TOPRAT_EXSHADEMODEL, 0,        0, VEOI,   TYCH,  0,   C_EXSHMODELCH_PENUMBRA, N, N),

	CULT( "humMeth",     DAT,   TOPRAT_HUMMETH,    0,          0, VEOI,   TYCH,  0,   C_HUMTHCH_ROB,     N,   N),
	CULT( "dflExH",      DAT,   TOPRAT_DFLEXH,     0,          0, VEOI,   TYFL,  0,      2.64f,          N,   N),
	CULT( "workDayMask", DAT,   TOPRAT_WORKDAYMASK, 0,         0, VEOI,   TYSI,  0,  (2+4+8+16+32+256+512),    N,   N), //5-95. dflt mon..fri+ht&cool dsnDays
	CULT( "DT",          DAT,   TOPRAT_DT,         0,          0, VEOI,   TYCH,  0,   C_NOYESCH_YES,           N,   N),
	CULT( "DTbegDay",    DAT,   TOPRAT_DTBEGDAY,   0,          0, VEOI,   TYDOY, 0,     0,                     N,   N), //code dflts
	CULT( "DTendDay",    DAT,   TOPRAT_DTENDDAY,   0,          0, VEOI,   TYDOY, 0,     0,                     N,   N), //code dflts
	CULT( "terrainClass",DAT,	TOPRAT_TERRAINCLASS, 0,        0, VEOI,	  TYSI,  0,     4,	   		           N,   N),
	CULT( "windSpeedMin",DAT,   TOPRAT_WINDSPEEDMIN,0,         0, VEOI,   TYFL,  0,      .5f,            N,   N),
	CULT( "windF",       DAT,   TOPRAT_WINDF,      0,          0, VEOI,   TYFL,  0,      1.f,            N,   N),
	CULT( "radBeamF",    DAT,   TOPRAT_RADBEAMF,   0,          0, VEOI,   TYFL,  0,      1.f,            N,   N),
	CULT( "radDiffF",    DAT,   TOPRAT_RADDIFFF,   0,          0, VEOI,   TYFL,  0,      1.f,            N,   N),
	CULT( "hConvMod",    DAT,   TOPRAT_HCONVMOD,   0,          0, VEOI,   TYCH,  0,   C_NOYESCH_YES,     N,   N),
	CULT( "verbose",     DAT,   TOPRAT_VERBOSE,    0,          0, VEOI,   TYSI,  0,      1,			     N,   N),
	CULT( "dbgPrintMask",DAT,	TOPRAT_DBGPRINTMASK,0,		   0, VHRLY,  TYLLI, 0,      0,              N,   N),
	CULT( "dbgPrintMaskC",DAT,	TOPRAT_DBGPRINTMASKC,0,		   0, VEOI,   TYLLI, 0,      0,              N,   N),
	CULT( "ventAvail",   DAT,   TOPRAT_VENTAVAIL,   0,         0, VHRLY,  TYCH,  0,  nc( C_VENTAVAILVC_WHOLEBLDG), 0.f, N,   N),

#ifdef AUTOSIZE
// TOP autosizing
	CULT( "auszTol",     DAT,   TOPRAT_AUSZTOL,    0,          0, VEOI,   TYFL,  0,      .005f,           N,   N), // .03->.01, 7-95
																												   // .01->.005, 2-14
	CULT( "heatDsTDbO",  DAT,   TOPRAT_HEATDSTDBO, 0,          0, VHRLY,  TYFL,  0,      0.f,            N,   N), // code dflts
	CULT( "heatDsTWbO",  DAT,   TOPRAT_HEATDSTWBO, 0,          0, VHRLY,  TYFL,  0,      0.f,            N,   N), // code dflts at runtime
	CULT( "coolDsMo",    DAT,   TOPRAT_COOLDSMO,   ARRAY,      0, VEOI,   TYSI,  0,      0.f,           v 13, N), // code dflts
	CULT( "coolDsDay",   DAT,   TOPRAT_COOLDSDAY,  ARRAY,      0, VEOI,   TYDOY, 0,      0.f,           v 13, N), // code dflts
	CULT( "coolDsCond",  DAT,   TOPRAT_COOLDSCOND, ARRAY,      0, VEOI,   TYREF, &DCiB,  0.f,           v 13, N),

#endif
// TOP reporting and exporting
	CULT( "runSerial",   DAT,   TOPRAT_RUNSERIAL,  0,          0, VEOI,   TYSI,  0,      0,            N,   N), // dflt'd by topStarPrf2.
	CULT( "runTitle",    DAT,   TOPRAT_RUNTITLE,   0,          0, VEOI,   TYSTR, 0,      0,            N,   N),
#ifdef BINRES	//cnglob.h.  re binary results files, rob 11-93.
	CULT( "BinResFile",     DAT, TOPRAT_BRS,       0,          0, VEOI,   TYCH,  0,  C_NOYESCH_NO,       N,   N),
	CULT( "BinResFileHourly",DAT,TOPRAT_BRHRLY,    0,          0, VEOI,   TYCH,  0,  C_NOYESCH_NO,       N,   N),
	CULT( "BinResFileName", DAT, TOPRAT_BRFILENAME,0,          0, VEOI,   TYSTR, 0,      0,            N,   N),
#endif
// TOP report page formatting
	CULT( "repHdrL",     DAT,   TOPRAT_REPHDRL,    0,          0, VEOI,   TYSTR, 0,      0,            N,   N),
	CULT( "repHdrR",     DAT,   TOPRAT_REPHDRR,    0,          0, VEOI,   TYSTR, 0,      0,            N,   N),
	CULT( "repCpl",      DAT,   TOPRAT_REPCPL,     0,          0, VEOI,   TYSI,  0,      78,            N,   N), //VEOI for TYSI's??
	CULT( "repLpp",      DAT,   TOPRAT_REPLPP,     0,          0, VEOI,   TYSI,  0,      66,            N,   N),
	CULT( "repTopM",     DAT,   TOPRAT_REPTOPM,    0,          0, VEOI,   TYSI,  0,      3,            N,   N),
	CULT( "repBotM",     DAT,   TOPRAT_REPBOTM,    0,          0, VEOI,   TYSI,  0,      3,            N,   N),
	CULT( "repTestPfx",  DAT,   TOPRAT_REPTESTPFX, 0,          0, VEOI,   TYSTR, 0,      N,            N,   N),
	CULT( "ck5aa5",      DAT,   TOPRAT_CK5AA5,     NO_INP,     0, 0,      TYSI,  0,     0x5aa5,            N,   N), // for verifying initialization
// TOP subclasses
	CULT( "material",    RATE,  0,                 NM_RQD,     0, 0,      0,    &MatiB,  N,    0.f,          matT,  N),
	CULT( "construction",RATE,  0,                 NM_RQD,     0, 0,      0,    &ConiB,  N,    0.f,          conT,  N),
	CULT( "foundation",  RATE,  0,                 NM_RQD,     0, 0,      0,    &FndiB,  N,    0.f,           fdT,  N),
	CULT( "glazeType",   RATE,  0,                 NM_RQD,     0, 0,      0,    &GtiB,   N,    0.f,           gtT,  N), // added 12-94
	CULT( "zone",        RATE,  0,                 NM_RQD/*|RQD*/, 0, 0,  0,    &ZiB,    N,    0.f,           znT,  N),
	CULT( "izXfer",      RATE,  0,                 0,          0, 0,      0,    &IzxiB,  N,    0.f,          izxT,  N),
	CULT("afmeter",      RATE,  0,                 NM_RQD,     0, 0,      0,    &AfMtriB,N,    0.f,      afMeterT,  N),
	CULT( "rsys",        RATE,  0,                 NM_RQD,     0, 0,      0,    &RSiB,   N,    0.f,         rsysT,  N),
	CULT( "dhwdayuse",   RATE,  0,                 NM_RQD,     0, 0,      0,    &WDUiB,  N,    0.f,    dhwDayUseT,  N),
	CULT( "dhwmeter",    RATE,  0,                 NM_RQD,     0, 0,      0,    &WMtriB, N,    0.f,     dhwMeterT,  N),
	CULT( "dhwsys",      RATE,  0,                 0,          0, 0,      0,    &WSiB,   N,    0.f,       dhwSysT,  N),
	CULT( "dhwsolarsys", RATE,  0,                 NM_RQD,     0, 0,      0,    &SWHiB,  N,    0.f,    dhwSolSysT,  N),
	CULT( "pvarray",     RATE,  0,                 0,          0, 0,      0,    &PViB,   N,    0.f,        pvArrT,  N),
	CULT( "battery",     RATE,  0,                 0,          0, 0,      0,    &BTiB,   N,    0.f,           btT,  N),
	CULT( "shadex",      RATE,  0,                 0,          0, 0,      0,    &SXiB,   N,    0.f,       shadexT,  N),
	CULT( "airHandler",  RATE,  0,                 NM_RQD,     0, 0,      0,    &AhiB,   N,    0.f,           ahT,  N),
	CULT( "meter",       RATE,  0,                 NM_RQD,     0, 0,      0,    &MtriB,  N,    0.f,          mtrT,  N),
	CULT( "gain",        RATE,  0,                 NOTOWNED,   0, 0,      0,    &GniB,   N,    0.f,           gnT,  N),
	CULT( "reportCol",   RATE,  0,                 NOTOWNED,   0, 0,      0,    &RcoliB, N,    0.f,         rpColT, N),
	CULT( "exportCol",   RATE,  0,                 NOTOWNED,   0, 0,      0,    &XcoliB, N,    0.f,         exColT, N),
	CULT( "report",      RATE,  0,                 NOTOWNED,   0, 0,      0,    &RiB,    N,    0.f,           rpT,  N),
	CULT( "export",      RATE,  0,                 NOTOWNED,   0, 0,      0,    &XiB,    N,    0.f,           exT,  N),
	CULT( "reportfile",  RATE,  0,                 0,          0, 0,      0,    &RfiB,   N,    0.f,          rpfT,  N),
	CULT( "exportfile",  RATE,  0,                 0,          0, 0,      0,    &XfiB,   N,    0.f,          exfT,  N),
	CULT( "importfile",  RATE,  0,                 NM_RQD,     0, 0,      0,	&ImpfiB, N,    0.f,         impfT,  N),
	CULT( "heatPlant",   RATE,  0,                 NM_RQD,     0, 0,      0,    &HpiB,   N,    0.f,           hpT,  N),
	CULT( "coolPlant",   RATE,  0,                 NM_RQD,     0, 0,      0,    &CpiB,   N,    0.f,           cpT,  N),
	CULT( "towerPlant",  RATE,  0,                 NM_RQD,     0, 0,      0,    &TpiB,   N,    0.f,           tpT,  N),
	CULT( "holiday",     RATE,  0,                 NM_RQD,     0, 0,      0,    &HdayiB, N,    0.f,         hdayT,  N),
	CULT( "descond",     RATE,  0,                 NM_RQD,     0, 0,      0,    &DCiB,   N,    0.f,           dcT,  N),
	CULT( "inverse",     RATE,  0,                 NM_RQD,     0, 0,      0,    &IvB,    N,    0.f,      inverseT,  N),

// TOP commands
	CULT( "run",         RUN,   0,                 0,          0, 0,      0,     0,      N,    0.f,            N,   topCkf),
	CULT( "clear",       CLEAR, 0,                 PRFP,       0, 0,      0,     0,   v topClearItf,0.f,   v topStarPrf, N), // note topStarPrf reuse
	CULT()
};	// cnTopCult

/*lint +e507 */
//============================================================================================
/*static*/ void basAnc::an_SetCULTLinks()
{	RiB.an_SetCULTLink( rpT);
	XiB.an_SetCULTLink( exT);
}		// SetCULTLinks
//============================================================================================
class CULTDOC
{
public:
	CULTDOC( int options) : cu_options( options) {}
	int cu_Doc();

private:
	int cu_options;
	std::unordered_set< std::string> cu_doneList;
	int cu_Doc1( const CULT* pCULT, const char* name, const char* parentName=NULL);
};		// class CULTDOC
//----------------------------------------------------------------------------
int CULTDOC::cu_Doc()
{
	return cu_Doc1( cnTopCult, "Top");

}		// CULTDOC::cu_Doc
//----------------------------------------------------------------------------
int CULTDOC::cu_Doc1(	// document CULT table and children
	const CULT* pCULT,
	const char* name,
	const char* parentName /*=NULL*/)
// returns 0 if info displayed
//         1 if skipped (already seen)
{
	int ret = 0;
	std::string nameS( name);

	std::unordered_set<std::string>::const_iterator doneIt = cu_doneList.find( nameS);
	if (doneIt != cu_doneList.end() )
		ret = 1;
	else
	{	printf( "\n\n%s", name);
		if (parentName)
			printf("    Parent: %s", parentName);
		printf("\n");
		const CULT* pCX = pCULT;
		while (pCX->id)
		{   if (pCX->cs != RATE)
				pCX->cu_ShowDoc( cu_options);
			pCX++;
		}
		pCX = pCULT;
		while (pCX->id)
		{   if (pCX->cs == RATE)
				cu_Doc1( (const CULT*)pCX->p2, pCX->id, name);
			pCX++;
		}
		cu_doneList.insert( nameS);
	}
    return ret;
}       // CULTDOC::cu_Doc1
//---------------------------------------------------------------------------
int CULT::cu_ShowDoc(       // document this CULT
    int options /*=0*/) const
// returns 0 if info displayed
//         1 if no display (id=="*" or NO_INP)
{ 
    options;
	int ret = 0;
	if (strcmp( id, "*") == 0
	 || (f & NO_INP))
		ret = 1;
	else
		printf( "   %s\n", id);

    return ret;
    
}   // CULT::cu_ShowDoc
//----------------------------------------------------------------------------
int culShowDoc(			// public function: display all input
    int options/*=0*/)
{
	CULTDOC cultDoc( options);		// local class
	int ret = cultDoc.cu_Doc();
    return ret;
}       // culDoc1
//=============================================================================

/************************************************ rest of file is if-outs ***************************************************/

#if 0 //ifdef OLDAT			// undefined in cnglob.h, 5-92
o /*------------------------ TERMINAL command (for zone) ----------------------*/
o
o //-------------------------------------------- terminal pre-input fcn: limit #
o // note: pre-input not Itf used so ownTi (=zi) already set in RAT for errmsg.
o LOCAL RC tuxPrf( CULT *c, TUX *p, ZNI *p2, void *p3) /*ARGSUSED*/
o{
o TUX *tu;   SI n=0;
o
o // check for too many terminals for zone.
o // NOTE zone terminal tables are in ZNR (not ZNI) -- setup deferred until topTu() below.
o
o   // count terminals for same zone.  Current one will be included.
o     RLUP( TuxiB, tu)			// loop TUX records
o        if (tu->ownTi==p2->ss)		// if terminal belongs to cur zone (reference type, need not check FsVAL 12-91)
o           n++;				//    count it
o
o   // error if too many
o     if (n > MAX_ZONETUS)	// =3, cndefns.h
o        return oer( p, "More than %d terminals for zone '%s'", (INT)MAX_ZONETUS, p2->name );
o
o     return RCOK;
o}		// tuxPrf
o
o static CULT tuxT[] = //------------------------- TERMINALX subCmd table for ZONE
o   // id       cs    fn                   f              uc evf    ty      b       dfls         p2    ckf
{
o   //--------  ----  -------------------  -------------- -- -----  -----   -----   ---------  ------- ---
o  "*",         STAR, 0,                   PRFP,          0, 0,     0,      0,      N,  0.f,   v tuxPrf, N),
o
o  "tuxZone",    DAT,  TUX_OWNTI,            NO_INP|RDFLIN, 0, 0,     TYIREF, &ZiB,   N,  0.f,      N,   N),
o  "tuxTLH",     DAT,  TUX_TUXTLH,            0,             0, VHRLY, TYFL,   0,      N, 68.f,      N,   N),
o  "tuxMxLH",    DAT,  TUX_TUXMXLH,           0,             0, VHRLY, TYFL,   0,      N,  0.f,      N,   N),
o  "tuxMnLH",    DAT,  TUX_TUXMNLH,           0,             0, VHRLY, TYFL,   0,      N,  0.f,      N,   N),
o #ifdef OLDAT			// undefined in cnglob.h, 5-92
o o "tuxAhu",     DAT,  TUX_TUXAHI,            0,             0, VEOI,  TYREF,  &AhuxiB,N,  0,        N,   N),
o #endif
o  "tuxTH",      DAT,  TUX_TUXTH,             0,             0, VHRLY, TYFL,   0,      N, 68.f,      N,   N),
o  "tuxTC",      DAT,  TUX_TUXTC,             0,             0, VHRLY, TYFL,   0,      N, 78.f,      N,   N),
o  "tuxCMn",     DAT,  TUX_TUXCMN,            0,             0, VHRLY, TYFL,   0,      N,  0.f,      N,   N),
o  "tuxCMxH",    DAT,  TUX_TUXCMXH,           0,             0, VHRLY, TYFL,   0,      N,  0.f,      N,   N),
o  "tuxCMxC",    DAT,  TUX_TUXCMXC,           0,             0, VHRLY, TYFL,   0,      N,  0.f,      N,   N),
o
o  "endTerminalx",ENDER,0,                  0,             0, 0,     0,      0,      N,  0.f,      N,   N),
oCULT()
o
};	// tuxT
#endif

// znT deletions:
#ifdef OLDNV // 1-92 cndefns.h
o//natVent
o "nvAHi",       DAT,   ZI(NVAHI),       0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "nvALo",       DAT,   ZI(NVALO),       0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "nvHD",        DAT,   ZI(NVHD),        0,        0, VEOI,  TYFL,  0,      N,      2.f,      N,   N),
o "nvAzmI",      DAT,   ZI(NVAZMI),      0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "nvStEM",      DAT,   ZI(NVSTEM),      0,        0, VEOI,  TYFL,  0,      N,      1.f,      N,   N),
o "nvDdEM",      DAT,   ZI(NVDDEM),      0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "nvDiEM",      DAT,   ZI(NVDIEM),      0,        0, VEOI,  TYFL,  0,      N,      .5f,      N,   N),
o "nvTD",        DAT,   ZI(NVTD),        0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
#endif
#ifdef OLDFV
o//fanvent
o "fvCfm",       DAT,   ZI(FVCFM),       0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "fvKW",        DAT,   ZI(FVKW),        0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "fvFz",        DAT,   ZI(FVFZ),        0,        0, VEOI,  TYFL,  0,      N,      1.f,      N,   N),
o "fvDECeff",    DAT,   ZI(FVDECEFF),    0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "fvIEDeff",    DAT,   ZI(FVIECEFF),    0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "fvTD",        DAT,   ZI(FVTD),        0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "fvWbMax",     DAT,   ZI(FVWBMAX),     0,        0, VEOI,  TYFL,  0,      N,      999.f,    N,   N),
#endif
#ifdef OLDCF
o//ceiling fan
o "cfKW",        DAT,   ZI(CFKW),        0,        0, VEOI,  TYFL,  0,      N,      0.f,      N,   N),
o "cfFz",        DAT,   ZI(CFFZ),        0,        0, VEOI,  TYFL,  0,      N,      1.f,      N,   N),
#endif
#ifdef OLDNV // old comments, moved to facilitate edit
o//natVent.  want erMsg (at end zone) for only 1 of ahigh, alow > 0.
o /* 1-91: chip to verify traditional dH default: something like 2 for 1 story, 8 for 2 stories. 2-91: his memo shows 0. */
o // 12-90: otta require azmI iff ddEM is non-0. later.
#endif
#if 0	//ifdef OLDAT			// undefined in cnglob.h, 5-92
o  "terminalx",   RATE,  0,               0,        0, 0,     0,    &TuxiB,  N,      0.f,   tuxT,   N),
#endif

// end of cncult.cpp
