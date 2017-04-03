// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// DHWCalc.cpp -- Domestic Hot Water model implementation
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"

#include "cse.h"
#include "ancrec.h" 	// record: base class for rccn.h classes
#include "rccn.h"
#include "irats.h"
#include "cuparse.h"
#include "cueval.h"

#include "cnguts.h"
#include "exman.h"

#include "hpwh/hpwh.hh"	// decls/defns for Ecotope heat pump water heater model


#if 0 && defined( _DEBUG)
#include "yacam.h"
static int WriteDHWUSEif( FILE* f, double wuStart, int wuFlow, int wuDur)
{	if (wuFlow <= 0)
		return 0;	// nothing
	fprintf( f, "   DHWUSE wuStart=%0.3f wuFlow=%d", wuStart, wuFlow);
	if (wuDur > 1)
		fprintf( f, " wuDuration=%d", wuDur);
	fprintf( f, "\n");
	return 1;
}	// WriteDHWUSEif
//---------------------------------------------------------------------
static int ConvertEcotopeSchedules(
	const char* pathIn,
	const char* pathOut)
{
const int NCOL = 11;
const int SZ = 30;
char s[ NCOL][ SZ];				// raw strings read from file
int colRows[ NCOL] = { 0 };		// # of rows by column
char colNames[ NCOL][ SZ] = { 0 }; // names of columns
int d[ 20000][ NCOL] = { 0 };
const char* suffix[ 8] = { "A", "B", "C", "D", "E", "F", "G", "H"};
	RC rc = RCOK;
	YACAM yak;
	rc |= yak.open( pathIn, "Ecotope csv file");
	int iCol;
	for (int iLn=0; !rc; iLn++)
	{	memset( s, 0, NCOL*SZ);
		rc = yak.getLineCSV( WRN|YAC_EOFOK, 0, strtprintf( "%dC", NCOL),
				s, SZ, 0);
		if (rc == RCBAD2)
		{	rc = RCOK;		// clean EOF
			break;
		}
		if (!rc)
		{	if (iLn == 0)
			{	// row counts for each column
				for (iCol=1;iCol<NCOL; iCol++)
					if (strDecode( s[ iCol], colRows[ iCol], WRN) != 1)
						rc = RCBAD;
			}
			else if (iLn == 1)
			{	// names of each column
				for (iCol=0; iCol<NCOL; iCol++)
					strcpy( colNames[ iCol], s[ iCol]);
			}
			else
			{	// data by minute
				int iRow = iLn-2;
				for (iCol=0; iCol<NCOL; iCol++)
				{	if (strlen( s[ iCol]) == 0)
						d[ iRow][ iCol] = -1;
					else if (strDecode( s[ iCol], d[ iRow][ iCol], WRN) != 1)
						rc = RCBAD;
				}
			}
		}
	}
	if (rc)
		return rc;

	// part 2: write out in CSE format
	//  merge adjacent rows with same flow
	FILE* f = fopen( pathOut, "w");
	if (!f)
		return RCBAD;
	for (iCol=1; iCol<NCOL; iCol++)
	{	int iDay = -1;
		int weekSched = colRows[ iCol] > 1440;
		int drawPrior = -1;
		int drawDur = -1;
		int iRowPrior = -1;
		double drawStart = 0.;
		for (int iRow=0; iRow<colRows[ iCol]; iRow++)
		{	int row1Day = iRow%1440 == 0;
			int rowNDay = iRow%1440 == 1439;
			if (row1Day)
			{	iDay++;
				const char* name = strtprintf( "%s%s", colNames[ iCol],
						weekSched ? suffix[ iDay] : "");
				fprintf( f, "DHWDAYUSE %s\n", name);
				drawPrior = 0;
				drawDur = 0;
				iRowPrior = -1;
			}
			int draw = d[ iRow][ iCol];		// draw in this minute, gal
			int iMin = d[ iRow][ 0] - 1 - iDay*1440;	// minute-of-day at *beg* of minute
			double hr = double( iMin)/60.;
			int continuingDraw = iRow == iRowPrior+1 && draw == drawPrior;
			if (continuingDraw)
			{	drawDur += 1;
				iRowPrior = iRow;
			}
			if (!continuingDraw)
			{	WriteDHWUSEif( f, drawStart, drawPrior, drawDur);
				drawPrior = draw;
				drawStart = hr;
				iRowPrior = iRow;
				drawDur = 1;
			}
			if (rowNDay)
			{	WriteDHWUSEif( f, drawStart, drawPrior, drawDur);
				fprintf( f, "ENDDHWDAYUSE\n");
			}
		}
	}
	fclose( f);
	return rc;
}
#endif
///////////////////////////////////////////////////////////////////////////////
// utility functions
///////////////////////////////////////////////////////////////////////////////
static int DHWMix(
	float tMix,		// target mixed water temp
	float tHot,		// available hot water temp
	float tCold,	// available cold water temp
	float& fHot)	// returned: required hot fraction
					//   0 - 1
// returns 0 success (fHot = fraction hot required)
//        -2 tHot <= tCold (fHot = 1)
//        -1 tMix < tCold (fHot = 0)
//        +1 tMix > tHot (fHot = 1)
{
	int ret = 0;
	if (tHot <= tCold)
	{	fHot = 1.f;
		ret = -2;
	}
	else
	{	fHot = (tMix - tCold) / (tHot - tCold);
		if (fHot < 0.f)
		{	fHot = 0.f;
			ret = -1;
		}
		else if (fHot > 1.f)
		{	fHot = 1.f;
			ret = 1;
		}
	}
	return ret;
}		// DHWMix

///////////////////////////////////////////////////////////////////////////////
// DHWMTR: accumulates hot water use (gal) by end use
///////////////////////////////////////////////////////////////////////////////
RC DHWMTR::wmt_CkF()
{	return RCOK;
}
//-----------------------------------------------------------------------------
RC DHWMTR::wmt_Init( IVLCH ivl)
// not called for C_IVLCH_SUBHOUR
{
	memset( &H.total, 0, (NDHWENDUSES+1)*sizeof( float));

	return RCOK;

}		// DHWMTR::wmt_Init
//-----------------------------------------------------------------------------
void DHWMTR_IVL::wmt_Copy(
	const DHWMTR_IVL* s,
	float mult/*=1.f*/)
{	if (mult==1.f)
		memcpy( this, s, sizeof( DHWMTR_IVL));
	else
		VCopy( &total, NDHWENDUSES+1, &s->total, mult);
}	// DHWMTR_IVL::wmt_Copy
//-----------------------------------------------------------------------------
void DHWMTR::wmt_Accum(
	IVLCH ivl,		// destination interval: day/month/year.  Accumulates from hour/day/month.  Not Top.ivl!
	int firstflg)	// iff TRUE, destination will be initialized before values are accumulated into it
{
	DHWMTR_IVL* dIvl = &Y + (ivl - C_IVLCH_Y);	// point destination substruct for interval
												// ASSUMES MTR interval members ordered like DTIVLCH choices
	DHWMTR_IVL* sIvl = dIvl + 1;				// source: next shorter interval

	// if hour-to-day call, compute total use
	if (ivl==C_IVLCH_D)
		sIvl->wmt_Finalize();  	// sum members

	// accumulate: copy on first call (in lieu of 0'ing dIvl).
	//   Note: wmt_Init() call in doBegIvl 0s H values
	dIvl->wmt_Accum( sIvl, firstflg != 0);
}		// DHWMTR::wmt_Accum
//-----------------------------------------------------------------------------
void DHWMTR_IVL::wmt_Accum(			// accumulate
	const DHWMTR_IVL* sIvl,		// source
	int options/*=0*/,			// options
								//   1: copy rather than add (re first call)
	float mult/*=1.f*/)			// multiplier
{
	if (options&1)
		wmt_Copy( sIvl, mult);
	else if (mult==1.f)
		VAccum( &total, NDHWENDUSES+1, &sIvl->total);
	else
		VAccum( &total, NDHWENDUSES+1, &sIvl->total, mult);
}		// DHWMTR_IVL
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// DHWSYS
///////////////////////////////////////////////////////////////////////////////
DHWSYS::~DHWSYS()
{
	cupfree( DMPP( ws_dayUseName));
	delete[] ws_whUseTick;
	ws_whUseTick = NULL;
}		// DHWSYS::~DHWSYS
//-------------------------------------------------------------------------------
/*virtual*/ void DHWSYS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	record::Copy( pSrc);
	cupIncRef( DMPP( ws_dayUseName));   // incr reference counts of dm strings if non-NULL
										//   nop if ISNANDLE
	// assume ws_whUseTick and ws_subhrHwUse are NULL
}		// DHWSYS::Copy
//-----------------------------------------------------------------------------
RC DHWSYS::ws_CkF()		// water heating system input check / default
// called at end of each DHWSYS input
{
	RC rc = RCOK;

	if (IsSet( DHWSYS_CENTRALDHWSYSI))
	{	// if served by central DHWSYS, msg disallowed inputs
		// can't use ws_HasCentral(), ref may not be resolved yet
		rc = disallowN( "when wsCentralDHWSYS is given",
				DHWSYS_SSF, DHWSYS_CALCMODE, DHWSYS_TSETPOINT,
				DHWSYS_LOADSHAREDHWSYSI, 0);
	}
	else if (IsSet( DHWSYS_LOADSHAREDHWSYSI))
	{	// if DHWSYS shares load, msg disallowed inputs
		rc = disallowN( "when wsLoadShareDHWSYS is given",
				DHWSYS_CENTRALDHWSYSI, DHWSYS_DAYUSENAME, DHWSYS_HWUSE, 0);
	}

	// if unset, ws_tSetpoint=ws_tUse (handles expressions)
	FldCopyIf( DHWSYS_TUSE, DHWSYS_TSETPOINT);

	rc |= ws_CheckVals( ERR|SHOFNLN);

#if 0 && defined( _DEBUG)
0   temporary data conversion code
0	ConvertEcotopeSchedules( "drawschedule.csv", "dhwdayuse.txt");
#endif

	return rc;
}	// DHWSYS::ws_CkF
//-----------------------------------------------------------------------------
RC DHWSYS::ws_CheckVals(		// check value ranges
	int erOp)
{
	RC rc = RCOK;

	if (!ISNANDLE( ws_SSF))
	{	if (ws_SSF < 0.f || ws_SSF > 0.99f)
			rc |= oer( "Bad wsSSF=%0.3f: value must be in the range 0 - 0.99",
				ws_SSF);
		ws_SSF = bracket( 0.f, ws_SSF, 0.99f);  // limit: calc may continue until rc checked
	}

	if (!ISNANDLE( ws_tUse))
	{	if (ws_tUse <= 32.f || ws_tUse >= 212.f)
		{	if (!Top.isWarmup)
				rc |= orMsg( erOp, "unreasonable wsTUse = %0.1f F", ws_tUse);
			ws_tUse = bracket( 32.1f, ws_tUse, 211.9f);
		}
	}
	if (!ISNANDLE( ws_tSetpoint))
	{	if (ws_tSetpoint <= 32.f || ws_tSetpoint >= 212.f)
		{	if (!Top.isWarmup)
				rc |= orMsg( erOp, "unreasonable wsTSetpoint = %0.1f F", ws_tUse);
			ws_tSetpoint = bracket( 32.1f, ws_tSetpoint, 211.9f);
		}
#if 0	// bug fix, 1-14-16
x No: do not change ws_tUse (extra backup provided if needed)
x		else if (!rc && !ISNANDLE( ws_tUse))
x		{	if (ws_tUse > ws_tSetpoint)
x			{	if (!Top.isWarmup)
x					rc |= orMsg( erOp, "wstUse (%0.1f F) > wsTSetpoint (%0.1f F)",
x						ws_tUse, ws_tSetpoint);
x				ws_tUse = ws_tSetpoint;
x			}
x		}
#endif
	}
	return rc;

}	// DHWSYS::ws_CheckVals
//-----------------------------------------------------------------------------
RC DHWSYS::ws_CheckSubObject(		// check that sub-object is acceptable
	record* r)		// subobject record (DHWHEATER, DHWPUMP, ...)
// WHY: child DHWSYSs have only HW use, not full set of components
// returns RCOK if r is a legal subobject
//         else not (do not run)
{
	RC rc = RCOK;
	if (!IsSet( DHWSYS_LOADSHAREDHWSYSI))
	{	// don't report error if both wsLoadShareDHWSYS and wsCentralDHWSYS are given.
		//   checked/errored in ws_CkF()
		//   msg here is confusing
		if (IsSet( DHWSYS_CENTRALDHWSYSI))
			rc = r->oer( "Not allowed here, this DHWSYS is served by a central DHWSYS.");
	}
	return rc;
}	// DHWSYS::ws_CheckSubObject
//-----------------------------------------------------------------------------
DHWSYS* DHWSYS::ws_GetCentralDHWSYS() const
{	// note: dicey for input records
	//   ws_centralDHWSYS may not be resolved
	DHWSYS* pWS = (DHWSYS *)b->GetAtSafe( ws_centralDHWSYSi);
	return pWS;
}		// DHWSYS::ws_GetCentralDHWSYS
//-----------------------------------------------------------------------------
RC DHWSYS::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	ws_whCount = 0;		// insurance
	return rc;
}	// DHWSYS::RunDup
//----------------------------------------------------------------------------
void DHWSYS::ws_SetMTRPtrs()		// set runtime pointers to meters
// WHY: simplifies runtime code
{
	ws_pMtrElec = MtrB.GetAtSafe( ws_elecMtri);		// elec mtr or NULL
	ws_pMtrFuel = MtrB.GetAtSafe( ws_fuelMtri);		// fuel mtr or NULL
	ws_pFXhwMtr = WMtrR.GetAtSafe( ws_FXhwMtri);	// fixture HW meter or NULL
	ws_pWHhwMtr = WMtrR.GetAtSafe( ws_WHhwMtri);	// water heater HW meter or NULL

	// central DHWSYS: prevent double counting
	//   if child and central say same meter, only central should accum
	//   if different, both should accum (child=subtotal, central=total)
	DHWSYS* pWSCentral = ws_GetCentralDHWSYS();		// parent or NULL
	if (pWSCentral)
	{
		#define CKDBLCOUNT( m) if (m == pWSCentral->m) m = NULL;
		CKDBLCOUNT( ws_pMtrElec)
		CKDBLCOUNT( ws_pMtrFuel)
		CKDBLCOUNT( ws_pFXhwMtr)
		CKDBLCOUNT( ws_pWHhwMtr)
		#undef CKDBLCOUNT
	}
}		// DHWSYS::ws_SetMTRPtrs
//----------------------------------------------------------------------------
RC DHWSYS::ws_Init(		// init for run (including children)
	int pass)		// pass (0, 1, 2)
// called *last* from topDHW
{
	RC rc = RCOK;

	if (pass == 0)
	{	// pass 0: init things that have no inter-DHWSYS effect
		ws_SetMTRPtrs();

		// load sharing base state: assume no sharing
		ws_loadShareCount = 1;
		ws_loadShareIdx = 0;
		return rc;
	}

	if (pass == 2)
	{	// final pass: set ws_loadShareCount to group value
		if (ws_loadShareDHWSYSi > 0)
		{	DHWSYS* pWS = WsR.GetAtSafe( ws_loadShareDHWSYSi);
			if (pWS)	// insurance: NULL impossible?
				ws_loadShareCount = pWS->ws_loadShareCount;
		}
		return rc;
	}

	// pass 1
	if (ws_HasCentralDHWSYS())
	{	// check that central DHWSYS tree is only one deep
		//   additional levels could be allowed if needed
		//   see ws_AccumCentralUse() etc
		DHWSYS* pWSCentral = ws_GetCentralDHWSYS();
		if (pWSCentral->ws_HasCentralDHWSYS())
			rc |= oer( "DHWSys '%s' (given by wsCentralDHWSYS) is not central",
				pWSCentral->name);

		// set or default some child values from parent
		//   wsCalcMode, wsTSetpoint: not allowed
		//   wsSSF: not allowed, uses parent
		//   wsTInlet, wsTUse: allowed, default to parent
		//   wsSDLM, wsDSM, wsWF: allowed, default to parent

		RRFldCopy( pWSCentral, DHWSYS_SSF);
		RRFldCopyIf( pWSCentral, DHWSYS_SDLM);
		RRFldCopyIf( pWSCentral, DHWSYS_WF);
		RRFldCopyIf( pWSCentral, DHWSYS_DSM);
		RRFldCopyIf( pWSCentral, DHWSYS_TINLET);
		RRFldCopyIf( pWSCentral, DHWSYS_TUSE);
		RRFldCopyIf( pWSCentral, DHWSYS_TSETPOINT);
	}

	else if (ws_loadShareDHWSYSi > 0)
	{	DHWSYS* pWS = WsR.GetAtSafe( ws_loadShareDHWSYSi);
		if (!pWS)
			rc |= rer( "wsLoadShareDHWSYS not found.");		// impossible?
		else
		{	if (pWS->ws_loadShareDHWSYSi > 0)
				rc |= oer( "DHWSys '%s' (given by wsLoadShareDHWSYS) also specifies wsLoadShareDHWSYS.",
					pWS->name);
			else
			{	ws_loadShareIdx = pWS->ws_loadShareCount;
				pWS->ws_loadShareCount++;
				// this->ws_loadShareCount set in pass 2 (above)
				RRFldCopy( pWS, DHWSYS_DAYUSENAME);
				RRFldCopy( pWS, DHWSYS_HWUSE);
			}
		}
	}

	ws_whUseTick = new double[ Top.tp_NHrTicks()];
	VZero( ws_whUseTick, Top.tp_NHrTicks());

	// ws_whCount set in DHWHEATER::RunDup
	DHWHEATER* pWH;
	RLUPC( WhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_Init();

	ws_wbCount = 0.f;	// branch count can be non-integer
	DHWLOOP* pWL;
	RLUPC( WlR, pWL, pWL->ownTi == ss)
	{	rc |= pWL->wl_Init();
		ws_wbCount += pWL->wl_mult * pWL->wl_wbCount;	// total branches served by this system
	}

	ws_simMeth = ws_DetermineSimMeth();
	if (ws_simMeth == wssimSUBHR)
	{	if (!Top.tp_subhrWSCount++)
		{	// check ticks on first DHWSYS needing subhr sim
			// Initially require 1 min ticks
			// TODO: generalize to allow other durations
			if (Top.tp_subhrTickDur != 1.)
				rc |= oer( "integral minute substep duration required");
		}
	}

	return rc;
}		// DHWSYS::ws_Init
//----------------------------------------------------------------------------
int DHWSYS::ws_DetermineSimMeth() const
{
	int simMeth = wssimHR;
	DHWHEATER* pWH;
	RLUPC( WhR, pWH, pWH->ownTi == ss)
	{	if (pWH->wh_IsSubhrModel())
		{	simMeth = wssimSUBHR;
			break;
		}
	}
	return simMeth;
}	// DHWSYS::ws_DetermineSimMeth
//----------------------------------------------------------------------------
float DHWSYS::ws_BranchFlow() const		// average branch flow rate
// returns nominal branch flow, gpm
{
	float brVF = ws_wbCount > 0.f
					? ws_fxUseHot / (ws_wbCount * 60.f)
					: 0.f;

	return brVF;
}		// DHWSYS::ws_BranchFlow
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoHour(		// hourly calcs
	int ivl,	// C_IVLCH_Y, _M, _D, _H, (_S)
	float centralMult /*=1.f*/)	// central system multiplier
								//   re recursive call
// Child DHWSYSs have all ws_wXcount = 0, so subobjects not modeled
// not called subhourly
// returns RCOK iff valid calc
{	RC rc = RCOK;

	// input elec / fuel for this DHWSYS (w/o ws_mult), Btu
	ws_inElec = 0.f;
	// ws_inFuel = 0.f;	no DHWSYS fuel use

	if (ivl <= C_IVLCH_D)	// if start of day (or longer)
	{	if (IsSet( DHWSYS_DAYUSENAME))
		{	// beg of day: locate DHWDAYUSE, set ws_dayUsei
			if (WduR.findRecByNm1( ws_dayUseName, &ws_dayUsei, NULL))
				return orMsg( ERR+SHOFNLN, "DHWDAYUSE '%s' not found.", ws_dayUseName);
		}
		// else ws_dayUsei = 0

		// re load share -- init starting DHWSYS for each end use
		// provides some randomization
		// basing on jDay yields consistent results for part-year short runs
		int seed = Top.jDay;
		for (int iEU=1; iEU<NDHWENDUSES; iEU++)
			ws_loadShareWS0[ iEU] = (seed+iEU)%ws_loadShareCount;
	}

	// mains temp
	if (!IsSet( DHWSYS_TINLET))
		ws_tInlet = Wthr.d.wd_tMains;

	// runtime checks of vals possibly set by expressions
	rc |= ws_CheckVals( ERR);	// checks ws_SSF, ws_tUse, ws_tSetpoint

	// distribution loss multiplier
	//   SDLM = standard distribution loss multiplier
	//          depends on unit floor area
	//   DSM = distribution system multiplier
	//         depends on distribution system configuration
	ws_DLM = 1.f + (ws_SDLM-1.f)*ws_DSM;

	// construct array of use factors by end use
	//   whDrawF = water heater draw / fixture hot water draw
	//   water heater draw includes SSF adjustment
	static const int WHDRAWFDIM = sizeof( ws_whDrawF)/sizeof( ws_whDrawF[ 0]);
#if defined( _DEBUG)
	if (WHDRAWFDIM != NDHWENDUSES)
		err( PABT, "ws_whDrawF array size error");
	VSet( ws_whDrawF, WHDRAWFDIM, -1.f);
#endif
	// temperature-dependent end uses
	float whDrawF = ws_WF * max( 0.f, ws_DLM - ws_SSF);
	ws_whDrawF[ 0]
		= ws_whDrawF[ C_DHWEUCH_SHOWER]
		= ws_whDrawF[ C_DHWEUCH_BATH] = whDrawF;
	// temperature independent end uses
	float whDrawFTempInd = 1.f - ws_SSF;
	ws_whDrawF[ C_DHWEUCH_CWASHR]
		= ws_whDrawF[ C_DHWEUCH_DWASHR]
		= ws_whDrawF[ C_DHWEUCH_FAUCET]
		= whDrawFTempInd;
#if defined( _DEBUG)
	if (VMin( ws_whDrawF, WHDRAWFDIM) < 0.f)
		err( PABT, "ws_whDrawF fill error");
#endif

	// ** Hot water use **
	//   2 types of user input
	//   * ws_hwUse = use for current hour (generally an expression w/ schedule)
    //   * DHWDAYUSE = collection of times and flows
	// here we combine these to derive consistent hourly total and tick-level (1 min) bins

	// init tick bins to average of hourly
	double hwUseX = ws_hwUse / ws_loadShareCount;	// hwUse per system
	VSet( ws_whUseTick, Top.tp_NHrTicks(), hwUseX / Top.tp_NHrTicks());
	// note ws_fxUseMixLH is set in ws_EndIvl()
	ws_fxUseMix.wmt_Clear();
	ws_whUse.wmt_Clear();

	// init water meter value
	//   ws_hwUse is enduse 0 (unknown)
	//   wdu_DoHour accums add'l DHWDAYUSE draws to these values
	ws_fxUseMix.wmt_AccumEU( 0, hwUseX);
	ws_fxUseHot = hwUseX;
	ws_whUse.wmt_AccumEU( 0, hwUseX*ws_whDrawF[ 0]);

	DHWDAYUSE* pWDU = WduR.GetAtSafe( ws_dayUsei);	// ref'd DHWDAYUSE can vary daily
	if (pWDU)
	{	// accumulation DHWDAYUSE input to tick bins and total use
		rc |= pWDU->wdu_DoHour( this);		// accum DAYUSEs, returns nUse = # of ticks with use
	}

	if (!ws_HasCentralDHWSYS())
	{	DHWSYS* pWSChild;
		RLUPC( WsR, pWSChild, pWSChild->ws_centralDHWSYSi == ss)
		{	rc |= pWSChild->ws_DoHour( ivl, ws_mult);
			rc |= ws_AccumCentralUse( pWSChild);
		}
	}

	ws_fxUseMix.wmt_Finalize();
	ws_whUse.wmt_Finalize();

	// accumulate to DHWMTRs if defined
	//   include DHWSYS.ws_mult multiplier
	float mult = ws_mult*centralMult;	// overall multiplier
	if (ws_pFXhwMtr)
		ws_pFXhwMtr->H.wmt_Accum( &ws_fxUseMix, 0, mult);
	if (ws_pWHhwMtr)
		ws_pWHhwMtr->H.wmt_Accum( &ws_whUse, 0, mult);

	// jacket loss
	ws_HJL = 0.f;
	if (ws_wtCount > 0)		// if any child tanks
	{	DHWTANK* pWT;
		RLUPC( WtR, pWT, pWT->ownTi == ss)
		{	rc |= pWT->wt_DoHour( ws_tUse);
			ws_HJL += pWT->wt_mult * pWT->wt_qLoss;
		}
	}

	// multi-unit distribution losses
	ws_HRDL = 0.f;
	if (ws_wlCount > 0)		// if any loops
	{	DHWLOOP* pWL;
		RLUPC( WlR, pWL, pWL->ownTi == ss)
		{	rc |= pWL->wl_DoHour( mult);		// also calcs child DHWLOOPSEGs and DHWLOOPPUMPs
			ws_HRDL += pWL->wl_HRLL + pWL->wl_HRBL;	// loop loss + branch loss
		}
	}

	// total recovery load
#if 1
	ws_HHWO = 8.345f * ws_whUse.total * (ws_tUse - ws_tInlet);
	ws_HARL = ws_HHWO + ws_HRDL + ws_HJL;
#else
	ws_HSEU = 8.345f * ws_fxUseHot * (ws_tUse - ws_tInlet);
	ws_HARL = ws_HSEU * ws_whDrawF[ 0] + ws_HRDL + ws_HJL;
#endif

	if (ws_whCount > 0)
	{	DHWHEATER* pWH;
		RLUPC( WhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_DoHour( ws_HARL / ws_whCount, mult);

		if (ws_calcMode==C_WSCALCMODECH_PRERUN && Top.tp_IsLastHour())
		{	RLUPC( WhR, pWH, pWH->ownTi == ss)
				rc |= pWH->wh_DoEndPreRun();
			DHWSYS* pWSi = WSiB.GetAtSafe( ss);
			if (pWSi)
				pWSi->ws_calcMode = C_WSCALCMODECH_SIM;
		}
	}

	if (ws_wpCount > 0)		// if any child pumps
	{	DHWPUMP* pWP;
		RLUPC( WpR, pWP, pWP->ownTi == ss)
			rc |= pWP->wp_DoHour( mult);
	}

	// DHWSYS energy use
	ws_inElec += ws_parElec * BtuperWh;	// parasitics for e.g. circulation pumping
										//   associated heat gain is ignored
										//   ws_parElec variability = hourly
	// ws_inFuel += 0.f;	// no DHWSYS-level fuel use

	// accum consumption to meters
	//   note: each DHWHEATER and DHWPUMP accums also
	if (ws_pMtrElec)
		ws_pMtrElec->H.dhw += mult * ws_inElec;
	if (ws_pMtrFuel)
		ws_pMtrFuel->H.dhw += mult * ws_inFuel;

	return rc;
}	// DHWSYS::ws_DoHour
//----------------------------------------------------------------------------
RC DHWSYS::ws_AccumCentralUse(		// accumulate central DHWSYS water use values
	const DHWSYS* pWSChild)	// child DHWSYS
{
	RC rc = RCOK;

	// apply child multiplier only
	// parent (central) multiplier applied by caller
	double mult = pWSChild->ws_mult;

	// == water use ==
	// ws_hwUse: do not accum (input)
	VAccum( ws_whUseTick, Top.tp_NHrTicks(), pWSChild->ws_whUseTick, mult);
	ws_fxUseHot += pWSChild->ws_fxUseHot * mult;
	ws_fxUseMix.wmt_Accum( &pWSChild->ws_fxUseMix, 0, mult);
	// ws_fxUseMixLH: do not accum, set for all DHWSYSs in ws_EndIvl
	ws_whUse.wmt_Accum( &pWSChild->ws_whUse, 0, mult);
	// water meters: do nothing here
	//   caller accums from ws_fxUseMix and ws_whUse

	// == energy ==
	ws_inElec += pWSChild->ws_inElec * mult;
	// ws_inFuel += pWSChild->ws_inFuel * mult;	// no DHWSYS fuel use

	return rc;
}		// DHWSYS::ws_AccumCentralUse
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoSubhr()		// subhourly calcs
{
	RC rc = RCOK;
	if (ws_whCount > 0 && ws_calcMode != C_WSCALCMODECH_PRERUN)
	{	DHWHEATER* pWH;
		double scale = 1./ws_whCount;
		// additional losses, Btu/tick for 1 DHWHEATER
		double xLoss = scale*(ws_HRDL + ws_HJL) / Top.tp_NHrTicks();
		RLUPC( WhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_IsHPWHModel())
				rc |= pWH->wh_HPWHDoSubhr(
					ws_whUseTick + Top.iSubhr*Top.tp_nSubhrTicks,	// HW use bins for this subhr
					scale,			// draw scale factor (allocates draw if ws_whCount > 1)
					xLoss);			// additional losses, Btu/tick
			// else missing case
			if (Top.isEndHour)
				pWH->wh_unMetHrs += pWH->wh_unMetSh > 0;
		}
	}
	return rc;
}	// DHWSYS::ws_DoSubhr
//----------------------------------------------------------------------------
void DHWSYS::ws_EndIvl( int ivl)		// end-of-interval
{
	if (ws_whCount > 0 && ivl == C_IVLCH_Y)
	{	DHWHEATER* pWH;
		RLUPC( WhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_unMetHrs > 0)
				warn( "%s: Output temperature below use temperature during %d hours of run.",
					pWH->objIdTx(), pWH->wh_unMetHrs);
		}
	}
	if (ivl == C_IVLCH_H)
		ws_fxUseMixLH.wmt_Copy( &ws_fxUseMix);

}		// DHWSYS::ws_EndIvl
//============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWDAYUSE -- detailed daily water use
//            = collection of DHWUSE
///////////////////////////////////////////////////////////////////////////////
RC DHWDAYUSE::wdu_CkF()		// input check / default
// called at end of each DHWDAYUSE
{
	RC rc = RCOK;

	return rc;
}	// DHWDAYUSE::wdu_CkF
//----------------------------------------------------------------------------
RC DHWDAYUSE::wdu_DoHour(		// accumulate tick-level HW use
	DHWSYS* pWS) const	// DHWSYS being calc'd (accum here)

// DHWUSE info is accumulated to tick bins and other DHWSYS totals
// DHWSYS values are NOT initialized here.

// return RCOK on success
{
	RC rc = RCOK;

	if (wdu_mult > 0.f)
	{	DHWUSE* pWU;
		RLUPC( WuR, pWU, pWU->ownTi == ss)
			rc |= pWU->wu_DoHour( pWS, wdu_mult, Top.iHr);
	}
	return rc;

}	// DHWDAYUSE::wdu_DoHour
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWUSE -- single hot water draw
///////////////////////////////////////////////////////////////////////////////
RC DHWUSE::wu_CkF()		// input check / default
// called at end of each DHWUSE input
{
	RC rc = RCOK;

	// check ranges
	//   note runtime checks elsewhere re hourly variability
	if (IsVal( DHWUSE_DUR))
		rc |= limitCheck( DHWUSE_DUR, 0., double( 60*24));
	if (IsSet( DHWUSE_HEATRECEF))
	{	const char* when = "when wuHeatRecEF is specified";
		rc |= require( DHWUSE_TEMP, when);
		rc |= disallow( DHWUSE_HOTF, when);
		if (IsVal( DHWUSE_HEATRECEF))
			rc |= limitCheck( DHWUSE_HEATRECEF, 0., 0.9);
	}
	else if (IsSet( DHWUSE_HOTF))
		rc |= disallowN( "when wuHotF is specified", DHWUSE_TEMP, DHWUSE_HEATRECEF, 0);

	return rc;
}	// DHWUSE::wu_CkF
//----------------------------------------------------------------------------
RC DHWUSE::wu_DoHour(		// accumulate 1 DHWUSE to tick-level bins
	DHWSYS* pWS,	// parent DHWSYS (accumulate herein)
	float mult,		// multiplier applied to each wu_flow
	int iH)			// hour of day (0 - 23)

	// returns RCOK on success
//         else error (stop run)
{
static const double minPerDay = 24.*60.;
	RC rc = RCOK;
	if (wu_dur == 0. || wu_flow*mult == 0.)
		return rc;		// nothing to do

	if (pWS->ws_loadShareCount > 1)
	{	// if load is being shared by more than 1 DHWSYS
		//   allocate by eventID to rotate DHWUSEs
		//   starting DHWSYS depends on jDay
		int EID = wu_eventID + pWS->ws_loadShareWS0[ wu_hwEndUse];
		int iWS = EID % pWS->ws_loadShareCount;		// target DHWSYS
		if (iWS != pWS->ws_loadShareIdx)
			return rc;		// not current DHWSYS, do nothing
	}

	if (wu_dur > minPerDay)
	{	// duration longer than 1 day
		// warn and limit
		rc |= orMsg( WRN, "wuDuration=%0.1f min changed to maximum allowed 1440 min (24 hr)",
			wu_dur);
		wu_dur = minPerDay;
	}

	double begM = wu_start * 60.;	// beg time, min of day
	roundNearest( begM, .05*Top.tp_subhrTickDur);	// round to avoid tiny amounts
													//   in adjacent bins
	double endM = begM + wu_dur;	// end time, min of day
	if (endM > minPerDay)
	{	// period wraps over midnight
		//   treat as 2 non-wrapping segments
		rc |= wu_DoHour1( pWS, mult, iH, begM, minPerDay);
		rc |= wu_DoHour1( pWS, mult, iH, 0., endM-minPerDay);
	}
	else
		rc |= wu_DoHour1( pWS, mult, iH, begM, endM);

	return rc;
}	// DHWUSE::wu_DoHour
//-----------------------------------------------------------------------------
RC DHWUSE::wu_DoHour1(		// low-level accum to tick-level bins
	DHWSYS* pWS,	// parent DHWSYS (accum info here)
	float mult,		// multiplier applied to wu_flow
	int iH,			// hour of day (0 - 23)
	double begM,	// draw beg time, min of day
	double endM)	// draw end time, min of day
					//   caller ensures endM > begM
// returns RCOK if any use in this hour
//         else error (stop run)
{
	RC rc = RCOK;

	// shift 0 point of time to current hour
	// determine overlap in this hour
	double begX = max( begM-iH*60, 0.);
	double endX = min( endM-iH*60, 60.);
	if (endX <= begX)
		return RCOK;		// no overlap with hour

	// some use in hour
	// compute actual flow re e.g. mixing
	double binStep = Top.tp_subhrTickDur;	// tick duration, min
	double fxFlow = wu_flow * mult;			// total (mixed) flow at fixture, gpm
											//   (with multiplier)
	if (IsSet( DHWUSE_TEMP))
	{	// use temperature is specified
		// const DHWSYS* pWS = wu_GetDHWSYS();
		float tCold = pWS->ws_tInlet;	// cold water temp at fixture, F
										//   assume same as mains temp
		float tHot = pWS->ws_tUse;		// hot water temp at fixture, F
										//   assume system tuse
		if (wu_heatRecEF > 0.f)
		{	// if heat recovery available
			if (wu_heatRecEF > 0.9f)
			{	// warn and limit
				rc |= orMsg( WRN, "wuHeatRecEF=%0.2f not in valid range 0 - 0.90",
						wu_heatRecEF);
				wu_heatRecEF = 0.9f;
			}
			// assume drain temp = use temp
			tCold = wu_heatRecEF*wu_temp + (1.f - wu_heatRecEF)*tCold;
		}
		// determine wu_hotF (= hot water mix fraction)
		int mixRet = DHWMix( wu_temp, tHot, tCold, wu_hotF);
		if (mixRet)
		{	if (mixRet == -2)
				rc |= orMsg( WRN, "Cold water temp (%0.1f F) >= hot water temp (%0.1f F).  "
					   "Cannot mix to wuTemp (%0.1f F).",
						tCold, tHot, wu_temp);
			else if (mixRet == -1)
				rc |= orMsg( WRN, "wuTemp (%0.1f F) < cold water temp (%0.1f F).  "
					   "Cannot mix to wuTemp.", wu_temp, tCold);
			else if (mixRet == 1)
				// impossible due to test above?
				rc |= orMsg( WRN, "wuTemp (%0.1f F) > hot water temp (%0.1f F).  "
					   "Cannot mix to wuTemp.", wu_temp, tHot);
		}
	}

	// allocate to tick bins
	// accumulate total uses by end use
	int iB0 = begX / binStep;	// first bin idx
	double begBin = iB0*binStep;		// start time of 1st bin
	for (int iB=iB0; begBin < endX; iB++)
	{	double endBin = (iB+1)*binStep;
		// use in this bin (gal) = flow (gpm) * overlap duration (min)
		double fxMixTick = fxFlow * (min( endBin, endX) - max( begBin, begX));
		// note: fxMixTick rarely 0 due to tests above, not worth testing
		double fxHotTick = fxMixTick * wu_hotF;
		double whUseTick = fxHotTick * pWS->ws_whDrawF[ wu_hwEndUse];
		pWS->ws_whUseTick[ iB] += whUseTick;	// tick total at WH
		pWS->ws_fxUseHot += fxHotTick;		// hour total hot at fixture
		pWS->ws_whUse.wmt_AccumEU( wu_hwEndUse, whUseTick);
		pWS->ws_fxUseMix.wmt_AccumEU( wu_hwEndUse, fxMixTick);
		begBin = endBin;
	}
	return rc;
}	// DHWUSE::wu_DoHour1
//----------------------------------------------------------------------------
DHWDAYUSE* DHWUSE::wu_GetDHWDAYUSE() const
{
	DHWDAYUSE* pWDU = (b == &WuR ? WduR : WDUiB).GetAtSafe( ownTi);
	return pWDU;
}		// DHWUSE::wu_GetDHWDAYUSE
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWHEATER
///////////////////////////////////////////////////////////////////////////////
DHWHEATER::~DHWHEATER()		// d'tor
{
	delete wh_pHPWH;
	wh_pHPWH = NULL;
	delete[] wh_HPWHHSMap;
	wh_HPWHHSMap = NULL;
}		// DHWHEATER::DHWHEATER()
//-----------------------------------------------------------------------------
/*static*/ WStr DHWHEATER::wh_GetHPWHVersion()	// return HPWH version string
{	return HPWH::getVersion();
}		// DHWHEATER::wh_GetHPWHVersion
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_CkF()		// water heater input check / default
// called at end of each DHWHEATER input
{
	const char* whTyTx = getChoiTx( DHWHEATER_TYPE, 1);
	const char* whenTy = strtprintf( "when whType=%s", whTyTx);
	const char* whHsTx = getChoiTx( DHWHEATER_HEATSRC, 1);
	const char* whenHs = strtprintf( "when whHeatSrc=%s", whHsTx);

	DHWSYS* pWS = wh_GetDHWSYS();
	RC rc = !pWS ? oer( "DHWSYS not found")	// insurance (unexpected)
				 : pWS->ws_CheckSubObject( this);
	if (rc)
		return rc;	// give up

	int bIsPreRun = pWS->ws_calcMode == C_WSCALCMODECH_PRERUN;

	// surrounding zone or temp: zone illegal if temp given
	//   used only re HPWH 2-16, enforce for all
	if (IsSet( DHWHEATER_TEX))
		rc |= disallowN( "when 'whTEx' is specified", DHWHEATER_ZNTI, 0);

	if (wh_heatSrc != C_WHHEATSRCCH_ELRESX)
		ignoreN( whenHs, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, 0);

	if (wh_type != C_WHTYPECH_STRGSML)
	{
		if (wh_heatSrc == C_WHHEATSRCCH_ASHP
	     || wh_heatSrc == C_WHHEATSRCCH_ASHPX)
			rc |= ooer( DHWHEATER_HEATSRC,
					"whHeatSrc=%s is not allowed %s", whHsTx, whenTy);
		if (wh_type == C_WHTYPECH_STRGLRG || wh_type == C_WHTYPECH_INSTLRG)
			rc |= require( DHWHEATER_EFF, whenTy);
		else if (wh_type == C_WHTYPECH_INSTSML)
			rc |= require( DHWHEATER_EF, whenTy);
		ignoreN( whenTy, DHWHEATER_LDEF, DHWHEATER_ASHPRESUSE, 0);
	}
	else if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
	{	// small storage HPWH model
		// TODO: more specific checking for ASHPX
		ignoreN( whenHs, DHWHEATER_LDEF, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, 0);
		RC rc1 = requireN( whenHs, DHWHEATER_ASHPTY, 0);
		rc |= rc1;
		if (!rc1)
		{	if (wh_ashpTy == C_WHASHPTYCH_GENERIC)
				rc |= requireN( "when whASHPType=Generic", DHWHEATER_EF, DHWHEATER_VOL, 0);
			else
				ignoreN( whenHs, DHWHEATER_EF, DHWHEATER_ASHPRESUSE, 0);
		}

		if (IsSet( DHWHEATER_ASHPTSRC))
			rc |= disallowN( "when 'whASHPSrcT' is specified", DHWHEATER_ASHPSRCZNTI, 0);
		else
		{	// default ASHP src from heater location zone
			//   (heat source is typically heater location zone)
			// wh_ashpSrcZnTi = wh_znTi done in wh_Init() (after deferred ref resolution)
			V wh_ashpTSrc = V wh_tEx;		// default ashpTSrc to tEx
											//   V handles NANDLES
		}
	}
	else if (wh_heatSrc == C_WHHEATSRCCH_ELRESX)
	{	// small storage electric resistance (HPWH model)
		ignoreN( whenHs, DHWHEATER_LDEF, DHWHEATER_ASHPTY,
			DHWHEATER_ASHPTSRC, DHWHEATER_ASHPSRCZNTI, DHWHEATER_ASHPRESUSE, 0);
		rc |= requireN( whenHs, DHWHEATER_EF, DHWHEATER_VOL, 0);
		if (!IsSet( DHWHEATER_RESHTPWR2))
			wh_resHtPwr2 = wh_resHtPwr;		// lower element power defaults from upper
	}
	else
	{	// T24DHW.DLL model
		//   ASHPX inputs ignored
		ignoreN( whenHs, DHWHEATER_ASHPTY, DHWHEATER_ASHPTSRC, DHWHEATER_ASHPSRCZNTI,
			DHWHEATER_ASHPRESUSE, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, 0);

		if (wh_EF == 1.f)
		{	// special case: "ideal" behavior (no losses)
			ignore( DHWHEATER_LDEF, strtprintf("%s and whEF=1", whenTy));
			wh_LDEF = 1.;
		}
		else
		{	// either LDEF required or EF + prerun
			if (!bIsPreRun)
				rc |= require(DHWHEATER_LDEF, strtprintf("%s and DHWSYS is not PreRun", whenTy));
			if (IsSet(DHWHEATER_LDEF))
				ignore( DHWHEATER_EF, strtprintf("%s and whLDEF is given", whenTy));
			else if (bIsPreRun)
				rc |= require(DHWHEATER_EF, strtprintf("%s and whLDEF is not given", whenTy));
		}
	}

	if (wh_IsStorage())
	{	// note wh_vol is required in some cases
        //   see above
        if (!IsSet( DHWHEATER_VOL))
			wh_vol = 50.f;
		rc |= limitCheck( DHWHEATER_VOL, .1, 10000., 2., 500.);
	}
	else if (wh_vol > 0.f)
		// tolerate specified whVol==0 for instantaneous
		rc |= disallow( DHWHEATER_VOL, whenTy);

	if (wh_heatSrc == C_WHHEATSRCCH_ASHP)
		rc |= require( DHWHEATER_HPAF, whenHs);
	else
	{	ignore( DHWHEATER_HPAF, whenHs);
		wh_HPAF = 1.f;		// make sure HPAF=1 if not ASHP
							//   factor is always active, see wh_DoHour
		// if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
		// TODO: checking for Ecotope HPWH model
	}

	wh_SetDesc();
	return rc;
}	// DHWHEATER::wh_CkF
//----------------------------------------------------------------------------
RC DHWHEATER::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	DHWSYS* pWS = wh_GetDHWSYS();
	pWS->ws_whCount += wh_mult;		// total # of water heaters in DHWSYS
	return rc;
}		// DHWHEATER::RunDup
//----------------------------------------------------------------------------
void DHWHEATER::wh_SetDesc()		// build probable description
// WHY: choice members cannot be probed
{
	const char* whTyTx = getChoiTx( DHWHEATER_TYPE, 1);
	const char* whSrcTx = getChoiTx( DHWHEATER_HEATSRC, 1);
	const char* hpTy = "";
	if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
		hpTy = strtprintf( " %s",getChoiTx( DHWHEATER_ASHPTY, 1));

	const char* t = strtprintf( "%s %s%s", whSrcTx, whTyTx, hpTy);
	strncpy0( wh_desc, t, sizeof( wh_desc));
}		// DHWHEATER::wh_SetDesc
//----------------------------------------------------------------------------
RC DHWHEATER::wh_Init()		// init for run
{
	RC rc = RCOK;

	wh_totHARL = 0.;
	wh_hrCount = 0;
	wh_totOut = 0.;
	wh_unMetHrs = 0;
	wh_balErrCount = 0;

	DHWSYS* pWS = wh_GetDHWSYS();

	// default meters from parent system
	if (!IsSet( DHWHEATER_ELECMTRI))
		wh_elecMtri = pWS->ws_elecMtri;
	if (!IsSet( DHWHEATER_FUELMTRI))
		wh_fuelMtri = pWS->ws_fuelMtri;

	wh_pMtrElec = MtrB.GetAtSafe( wh_elecMtri);		// elec mtr or NULL
	wh_pMtrFuel = MtrB.GetAtSafe( wh_fuelMtri);		// fuel mtr or NULL
#if 0
0	idea
0	wh_pMtrPrim = wh_IsElec() ? wh_pMtrElec : wh_pMtrFuel;	// heat mtr or NULL
#endif

// zone linkage pointers
// currently used only by HPWH model, 2-16
	wh_pZn = ZrB.GetAtSafe( wh_znTi);
	if (!IsSet( DHWHEATER_ASHPTSRC) && !IsSet( DHWHEATER_ASHPSRCZNTI))
		wh_ashpSrcZnTi = wh_znTi;	// default ASHP source zone = DHWHEATER location zone
	wh_pAshpSrcZn = ZrB.GetAtSafe( wh_ashpSrcZnTi);

// set up Ecotope heat pump water heater model
// free (impossible?) pre-existing HPWH data
	delete wh_pHPWH; wh_pHPWH = NULL;
	delete[] wh_HPWHHSMap; wh_HPWHHSMap = NULL;

	if (wh_IsHPWHModel())
		rc |= wh_HPWHInit();	// set up from DHWHEATER inputs

	return rc;
}		// DHWHEATER::wh_Init
//----------------------------------------------------------------------------
DHWSYS* DHWHEATER::wh_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WhR ? WsR : WSiB).GetAtSafe( ownTi);
	return pWS;
}		// DHWHEATER::wh_GetDHWSYS
//----------------------------------------------------------------------------
int DHWHEATER::wh_UsesDerivedLDEF() const
// returns nz iff wh_LDEF needs to be derived via PreRun
{
	int ret = 0;
	if (wh_type == C_WHTYPECH_STRGSML)
	{	if (!wh_IsSubhrModel()
	      && (wh_heatSrc != C_WHHEATSRCCH_ELRES || wh_EF != 1.f))
			ret = 1;		// ELRES + EF=1 means ideal heater
	}
	else if (wh_type == C_WHTYPECH_STRGLRG)
		// T24DHW.DLL erroneously uses LDEF for large
		ret = Top.tp_dhwModel == C_DHWMODELCH_T24DHW;
	return ret;
}		// DHWHEATER::wh_UsesDerivedLDEF
//----------------------------------------------------------------------------
float DHWHEATER::wh_CalcLDEF(		// calculate load dependent energy factor
	float arl,			// hourly recovery load, Btu
						//   generally annual average
	int options /*=0*/)	// option bits
						//   1: apply "e" factor (not used when annual avg ?)
// implements Eqn 35 in ACM
// returns LDEF = energy factor modified for annual load
//           -1.f iff error (inapplicable wh_type, )
{

// Table RE-4 ACM App E
//   values agree with T24DHW.DLL code, 9-1-2015
static const float LDtab[][6] =
{     //       a         b          c          d        e    max
	{  -0.098311f, 0.240182f, 1.356491f, -0.872446f, 0.946f, .90f },	// storage gas
	{  -0.91263f, 0.94278f,  4.31687f,   -3.42732f,  0.976f, .99f},		// storage elec
	{   0.44189f,-0.28361f,	-0.71673f,	  1.13480f,	 0.947f,  4.f }		// heat pump
};

	if (wh_type != C_WHTYPECH_STRGSML && Top.tp_dhwModel != C_DHWMODELCH_T24DHW)
		return -1.f;		// LDEF applicable only for small storage
							//  don't enforce for T24DHW mode due to
							//    erroneous use of LDEF for Large Storage

	int iLD = wh_heatSrc == C_WHHEATSRCCH_ELRES ? 1
		    : wh_heatSrc == C_WHHEATSRCCH_ASHP  ? 2
		    :                                     0;
	const float* LD = LDtab[ iLD];
	float LDEF = log( max( arl, .01f)*24.f/1000.f)*(LD[ 0]*wh_EF+LD[ 1])
					+ LD[ 2]*wh_EF+LD[ 3];
	if (options & 1)
		LDEF *= LD[ 4];

	// apply limits
	//   regression can produce unrealistic results
	LDEF = bracket( .1f, LDEF, LD[ 5]);

	return LDEF;

}	// DHWHEATER::wh_CalcLDEF
//----------------------------------------------------------------------------
RC DHWHEATER::wh_DoHour(			// DHWHEATER hour calcs
	float HARL,		// hourly adjusted recovery load, Btu
	int wsMult)		// system multiplier
{
	RC rc = RCOK;

	// accumulate load (re LDEF derivation)
	wh_totHARL += HARL;
	wh_hrCount++;

	wh_inElec = 0.f;
	wh_inElecBU = 0.f;
	wh_inFuel = 0.f;
	wh_mixDownF = 1.f;

	if (wh_IsSubhrModel())
	{	// this DHWHEATER uses subhour model
		//   cannot simply loop subhour model at this point
		//   Must include calcs at subhour level re e.g. zone coupling
		//   See wh_DoSubhr()
		if (wh_IsHPWHModel())
			wh_HPWHDoHour();		// hourly portions of HPWH calc
		else
			rc = err( PABT, "Missing DHWHEATER subhourly model");
		return rc;
	}

	// hourly model
	float WHEU = 0.f;		// current hour energy use, Btu
	wh_totOut += HARL;		// output = load

	switch (wh_type)
	{
	case C_WHTYPECH_INSTSML:
		WHEU = HARL / (wh_EF * 0.92f);
		break;

	case C_WHTYPECH_INSTLRG:
		WHEU = HARL / (wh_eff * 0.92f);		// pilot included below
		break;

	case C_WHTYPECH_STRGSML:
		// if (wh_heatSrc == C_WHHEATSRCCH_ASHPX) -> wh_IsSubhrModel() logic error
		WHEU = HARL * wh_HPAF / wh_LDEF;
		break;

	case C_WHTYPECH_STRGLRG:
		WHEU = Top.tp_dhwModel == C_DHWMODELCH_T24DHW
		    && wh_heatSrc == C_WHHEATSRCCH_ELRES
				? HARL / wh_LDEF	// match T24DWH.DLL erroneous use of small
									//   storage model for electric large storage
				: HARL / wh_eff + wh_SBL;
		break;


	default:
		break;		// all results 0 per init above
	}

	// electricity / fuel consumption for this DHWHEATER (no multipliers)
	wh_inElec = wh_parElec * BtuperWh;	// electric parasitic
										//  (supported for both fuel and elec WH)
	if (wh_IsElec())
		wh_inElec += WHEU;
	else
		wh_inFuel = WHEU + wh_pilotPwr;	// pilot included in all fuel types
											//   (but not elec WH)

	// accum consumption to meters (scaled by multipliers)
	int mult = wh_mult * wsMult;	// overall multiplier = system * heater
	if (wh_pMtrElec)
		wh_pMtrElec->H.dhw += mult * wh_inElec;
	if (wh_pMtrFuel)
		wh_pMtrFuel->H.dhw += mult * wh_inFuel;

	return rc;

}	// DHWHEATER::wh_DoHour
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_DoEndPreRun()
{
	RC rc = RCOK;

	if (!wh_UsesDerivedLDEF())
		return rc;		// no adjustments required

	if (wh_type == C_WHTYPECH_STRGSML)
	{	// average hourly load
		float arl = wh_totHARL / wh_hrCount;
		// "Load-dependent energy factor"
		float LDEF = wh_CalcLDEF( arl);
		// update input record with derived value
		DHWHEATER* pWHi = WHiB.GetAtSafe( ss);
		if (pWHi && !pWHi->IsSet( DHWHEATER_LDEF))
		{	pWHi->wh_LDEF = LDEF;
			pWHi->fStat( DHWHEATER_LDEF) |= FsSET | FsVAL;
		}
	}
	else if (wh_type == C_WHTYPECH_STRGLRG && Top.tp_dhwModel == C_DHWMODELCH_T24DHW)
	{	// T24DHW.DLL erroneously uses small storage model for large storage
		//   (for electric resistance only)
		//   derive LDEF from Eff
		float arl = wh_totHARL / wh_hrCount;
		wh_EF = wh_eff;
		float LDEF = wh_CalcLDEF( arl);
		DHWHEATER* pWHi = WHiB.GetAtSafe( ss);
		if (pWHi && !pWHi->IsSet( DHWHEATER_LDEF))
		{	pWHi->wh_LDEF = LDEF;
			pWHi->fStat( DHWHEATER_LDEF) |= FsSET | FsVAL;
		}
	}
	// else nothing needed

	return rc;
}		// DHWHEATER::wh_DoEndPreRun
//-----------------------------------------------------------------------------
/*static*/ void DHWHEATER::wh_HPWHMessageCallback(
	const std::string message,
	void* contextPtr)
{	((DHWHEATER *)contextPtr)->wh_HPWHReceiveMessage( message);
}		// DHWHEATER::wh_HPWHMessageCallback
//-----------------------------------------------------------------------------
void DHWHEATER::wh_HPWHReceiveMessage( const std::string message)
{
	pInfo( "DHWHEATER '%s' HPWH message (%s): %s", name, Top.When( C_IVLCH_S), message.c_str());
}		// DHWHEATER::wh_HPWHReceiveMessage
//-----------------------------------------------------------------------------
int DHWHEATER::wh_HPWHHasCompressor() const
// returns -1: type does not use HPWH
//         0: no compressor (resistance only)
//         1: has compressor (heat pump)
{
	if (!wh_IsHPWHModel())
		return -1;

	int noComp = wh_heatSrc == C_WHHEATSRCCH_ELRESX
			  || wh_ashpTy == C_WHASHPTYCH_RESTANK
			  || wh_ashpTy == C_WHASHPTYCH_RESTANKNOUA;

	return !noComp;

}		// DHWHEATER::wh_HPWHHasCompressor
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHInit()		// initialize HPWH model
// returns RCOK on success
{
	RC rc = RCOK;
	DHWSYS* pWS = wh_GetDHWSYS();

	wh_HPWHTankTempSet = 0;		// force tank temp init (insurance)
								//   (see DHWHEATER::wh_HPWHDoHour)

	wh_pHPWH = new( HPWH);

	wh_pHPWH->setMessageCallback( wh_HPWHMessageCallback, this);
	wh_pHPWH->setVerbosity( HPWH::VRB_reluctant);		// messages only for fatal errors

	int hpwhRet = 0;
	float UAX = -1.f;		// alternative UA
	float volX = -1.f;		// alternative volume
	HPWH::MODELS preset = HPWH::MODELS( -1);	// HPWH "preset" = predefined type that determines most model parameters
	if (wh_heatSrc == C_WHHEATSRCCH_ELRESX)
	{	// resistance tank (no preset)
		hpwhRet = wh_pHPWH->HPWHinit_resTank( GAL_TO_L( max( wh_vol, 1.f)), wh_EF,
						wh_resHtPwr, wh_resHtPwr2);
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_GENERIC)
	{	// generic HPWH (no preset)
		hpwhRet = wh_pHPWH->HPWHinit_genericHPWH(
					GAL_TO_L( max( wh_vol, 1.f)), wh_EF, wh_ashpResUse);
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_AOSMITHSHPT50)
	{	preset = HPWH::MODELS_GE2012;	// AO Smith SHPT models: base on GE2012
		volX = 45.f;			// ... and change vol / UA
		UAX = 2.9f;
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_AOSMITHSHPT66)
	{	preset = HPWH::MODELS_GE2012;
		volX = 63.9f;
		UAX = 3.4f;
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_AOSMITHSHPT80)
	{	preset = HPWH::MODELS_GE2012;
		volX = 80.7f;
		UAX = 4.7f;
	}
	else
	{	// heat pump
		preset =
			  wh_ashpTy == C_WHASHPTYCH_BASICINT      ? HPWH::MODELS_basicIntegrated
			: wh_ashpTy == C_WHASHPTYCH_RESTANK       ? HPWH::MODELS_restankRealistic
			: wh_ashpTy == C_WHASHPTYCH_RESTANKNOUA   ? HPWH::MODELS_restankNoUA
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHPHPT60 ? HPWH::MODELS_AOSmithPHPT60
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHPHPT80 ? HPWH::MODELS_AOSmithPHPT80
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHHPTU50 ? HPWH::MODELS_AOSmithHPTU50
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHHPTU66 ? HPWH::MODELS_AOSmithHPTU66
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHHPTU80 ? HPWH::MODELS_AOSmithHPTU80
			: wh_ashpTy == C_WHASHPTYCH_AOSMITHHPTU80DR ? HPWH::MODELS_AOSmithHPTU80_DR
			: wh_ashpTy == C_WHASHPTYCH_SANDEN40	  ? HPWH::MODELS_Sanden40
			: wh_ashpTy == C_WHASHPTYCH_SANDEN80      ? HPWH::MODELS_Sanden80
			: wh_ashpTy == C_WHASHPTYCH_GE2012		  ? HPWH::MODELS_GE2012
			: wh_ashpTy == C_WHASHPTYCH_GE2014		  ? HPWH::MODELS_GE2014
			: wh_ashpTy == C_WHASHPTYCH_GE2014_80	  ? HPWH::MODELS_GE2014_80
			: wh_ashpTy == C_WHASHPTYCH_GE2014_80DR   ? HPWH::MODELS_GE2014_80DR
			: wh_ashpTy == C_WHASHPTYCH_GE2014STDMODE ? HPWH::MODELS_GE2014STDMode
			: wh_ashpTy == C_WHASHPTYCH_GE2014STDMODE_80 ? HPWH::MODELS_GE2014STDMode_80
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHB50     ? HPWH::MODELS_RheemHB50
			: wh_ashpTy == C_WHASHPTYCH_STIEBEL220E   ? HPWH::MODELS_Stiebel220E
			: wh_ashpTy == C_WHASHPTYCH_GENERIC1	  ? HPWH::MODELS_Generic1
			: wh_ashpTy == C_WHASHPTYCH_GENERIC2	  ? HPWH::MODELS_Generic2
			: wh_ashpTy == C_WHASHPTYCH_GENERIC3	  ? HPWH::MODELS_Generic3
			: wh_ashpTy == C_WHASHPTYCH_UEF2GENERIC   ? HPWH::MODELS_UEF2generic
			: wh_ashpTy == C_WHASHPTYCH_WORSTCASEMEDIUM	 ? HPWH::MODELS_UEF2generic	// alias (testing aid)
			:										       HPWH::MODELS( -1);	// HPWHInit_presets will reject
	}
	if (preset > 0)
		hpwhRet = wh_pHPWH->HPWHinit_presets( preset);

	if (hpwhRet)	// 0 means success
		rc |= RCBAD;
	else
	{	// set additional parameters if needed
		if (volX > 0.f && wh_pHPWH->setTankSize( volX, HPWH::UNITS_GAL) != 0)
			rc = RCBAD;
		if (UAX > 0.f && wh_pHPWH->setUA( UAX, HPWH::UNITS_BTUperHrF) != 0)
			rc = RCBAD;
	}

	if (!rc)
	{	// make map of heat sources = idxs for wh_HPWHUse[]
		// WHY: HPWH model frequently uses 3 heat sources in
		//      preset-specific order
		wh_HPWHHSCount = wh_pHPWH->getNumHeatSources();
		wh_HPWHHSMap = new int[ wh_HPWHHSCount];
		if (wh_HPWHHasCompressor() == 0)
			// no compressor, all use is primary
			VSet( wh_HPWHHSMap, wh_HPWHHSCount, 1);
		else for (int iHS=0; iHS<wh_HPWHHSCount; iHS++)
		{	HPWH::HEATSOURCE_TYPE hsTy = wh_pHPWH->getNthHeatSourceType( iHS);
			wh_HPWHHSMap[ iHS] = hsTy != HPWH::TYPE_resistance;
									// resistance use -> wh_HPWHUse[ 0]
									// all other (compressor) -> wh_HPWHUse[ 1]
		}
		wh_pHPWH->setMinutesPerStep(Top.tp_subhrTickDur);	// minutesPerStep
	}

	return rc;
}		// DHWHEATER::wh_HPWHInit
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoHour()		// hourly HPWH calcs
// any HPWH setup etc that need not be done subhourly
// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();

	wh_unMetSh = 0;		// count of this hrs subhrs
						//   having wh_tHOut < wh_tUse

	// setpoint temp: ws_tUse has hourly variability
	//   some HPWHs (e.g. Sanden) have fixed setpoints, don't attempt
	if (!wh_pHPWH->isSetpointFixed())
	{
#if 0 && defined _DEBUG
		static float tSetpointPrior = 0.f;
		if (pWS->ws_tSetpoint != tSetpointPrior)
		{	printf( "\nSetpoint change!");
			tSetpointPrior = pWS->ws_tSetpoint;
		}
#endif
		wh_pHPWH->setSetpoint( DegFtoC( pWS->ws_tSetpoint));
	}

	// retrieve resulting setpoint after HPWH restrictions
	double tHPWHSetpoint = DegCtoF( wh_pHPWH->getSetpoint());
	if (wh_tHWOut == 0.f)
		wh_tHWOut = tHPWHSetpoint;		// initial guess for HW output temp
										//   updated every substep with nz draw

	if (!wh_HPWHTankTempSet)
	{	// initialize tank temp on 1st call
		//   must be done after setting HPWH setpoint (=ws_tSetpoint)
		//   (ws_tSetpoint may be expression)
		if (wh_pHPWH->resetTankToSetpoint())
			rc |= RCBAD;
		wh_HPWHTankTempSet++;
	}

	wh_pHPWH->setInletT( DegFtoC( pWS->ws_tInlet));

	return rc;

}	// DHWHEATER::wh_HPWHDoHour
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoSubhr(		// HPWH subhour
	double* draw,		// per-tick table of draw amounts, gal
	double scale,		// draw scale factor
						//   re DHWSYSs with >1 DHWHEATER
						//   *not* including wh_mixDownF;
	double xLoss)		// additional losses for DHWSYS, Btu/tick
						//   re e.g. central loop losses
						//   note: scale applied by caller
// calls HPWH tp_nSubhrTicks times, totals results
// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	int mult = pWS->ws_mult * wh_mult;	// overall multiplier

	// local totals
	double qEnv = 0.;		// heat removed from environment, kWh
							//   + = to water heater
	double qLoss = 0.;		// standby losses, kWh
							//   + = to surround

	double qHW = 0.;		// useful hot water heating, kWh
							//   always >= 0
							//   does not include wh_HPWHxBU

	double tHWOutF = 0.;	// accum re average hot water outlet temp, F

	wh_HPWHUse[ 0] = wh_HPWHUse[ 1] = 0.;	// energy use totals, kWh
	wh_HPWHxBU = 0.;	// add'l resistance backup, Btu
						//   water is heated to ws_tUse if HPWH does not meet load

	// setpoint and inletT: see wh_HPHWDoHour above

	// ambient and source temps
	//   set temp from linked zone (else leave expression/default value)
	if (wh_pZn)
		wh_tEx = wh_pZn->tzls;
	if (wh_pAshpSrcZn)
		wh_ashpTSrc = wh_pAshpSrcZn->tzls;

#define HPWH_QBAL		// define to include sub-hour energy balance check
#if defined( HPWH_QBAL)
	double qHCStart = wh_pHPWH->getTankHeatContent_kJ();
#endif

#define HPWH_DUMP		// define to include debug CSV file
#if defined( HPWH_DUMP)
// #define HPWH_DUMPSMALL	// #define to use abbreviated version
	int bWriteCSV = DbDo( dbdHPWH);
#endif

	// pseudo-draw (gal) to represent e.g. central system distribution losses
#if 0
x now impossible due to unlimited backup
x	//   prevent "death spiral" if wh_tHWOut goes below 105 by ignoring some loss
x	//   losses are typically calculated using 70 F surround temp and
x	//   arbitrary water temp (e.g. 130 F)
x	if (wh_tHWOut < 105.f)
x		xLoss *= max( 0., (wh_tHWOut - 70.)/( 105. - 70.));
#endif
	double drawLoss = xLoss / (8.345*max(1., wh_tHWOut - pWS->ws_tInlet));

	int nTickNZDraw = 0;		// count of ticks with draw > 0
	for (int iT=0; !rc && iT<Top.tp_nSubhrTicks; iT++)
	{	
#if 0 && defined( _DEBUG)
		if (Top.tp_date.month == 7
		  && Top.tp_date.mday == 27
		  && Top.iHr == 10
		  && Top.iSubhr == 3)
			wh_pHPWH->setVerbosity( HPWH::VRB_emetic);
#endif
		
		double drawForTick = draw[ iT]*scale*wh_mixDownF + drawLoss;
		int hpwhRet = wh_pHPWH->runOneStep(
						GAL_TO_L( drawForTick),	// draw volume, L
						DegFtoC( wh_tEx),		// ambient T (=tank surround), C
						DegFtoC( wh_ashpTSrc),	// heat source T, C
												//   aka HPWH "external temp"
						HPWH::DR_ALLOW);		// DRstatus: no demand response modeled
		if (hpwhRet)	// 0 means success
			rc |= RCBAD;

		qEnv += wh_pHPWH->getEnergyRemovedFromEnvironment();
		qLoss += wh_pHPWH->getStandbyLosses();
		double tO = wh_pHPWH->getOutletTemp();
		if (tO)
		{	double tOF = DegCtoF( tO);
			if (tOF < pWS->ws_tUse)
			{	// load not met, add additional (unlimited) resistance heat
				wh_mixDownF = 1.f;
				wh_HPWHxBU += 8.345 * drawForTick * (pWS->ws_tUse - tOF);
				tOF = pWS->ws_tUse;
			}
			else
				DHWMix( pWS->ws_tUse, tOF, pWS->ws_tInlet, wh_mixDownF);
			tHWOutF += tOF;		// note tOF may have changed (but not tO)
			nTickNZDraw++;
			qHW += KJ_TO_KWH(
				     GAL_TO_L( drawForTick)
				   * HPWH::DENSITYWATER_kgperL
				   * HPWH::CPWATER_kJperkgC
				   * (tO - DegFtoC( pWS->ws_tInlet)));
		}

		// energy use by heat source, kWh
		// accumulate by backup resistance [ 0] vs primary (= compressor or all resistance) [ 1]
		for (int iHS=0; iHS < wh_HPWHHSCount; iHS++)
		{	wh_HPWHUse[ wh_HPWHHSMap[ iHS]] += wh_pHPWH->getNthHeatSourceEnergyInput( iHS);
#if 0 && defined( _DEBUG)
			// debug aid
			if (wh_pHPWH->getNthHeatSourceEnergyInput( iHS) < 0.)
				printf( "\nNeg input, iHS=%d", iHS);
#endif
		}
#if defined( HPWH_DUMP)
		// tick level CSV report for testing
		static FILE* pF = NULL;		// file
		if (bWriteCSV)
		{	if (!pF)
			{	const char* fName = strtprintf( "HPWH_%s.csv", name);
				pF = fopen( fName, "wt");
				if (!pF)
					err( PWRN, "HPWH report failure for '%s'", fName);
				else
				{	// headings
					fprintf( pF, "%s\n",wh_desc, Top.runDateTime);
					fprintf( pF, "%s%s %s %s HPWH %s   %s\n",
						Top.tp_RepTestPfx(), ProgName, ProgVersion, ProgVariant,
						Top.tp_HPWHVersion, Top.runDateTime);
#if defined( HPWH_DUMPSMALL)
					fprintf( pF, "minYear,draw( L)\n");
#else
					wh_pHPWH->WriteCSVHeading( pF, "month,day,hr,min,minDay,"
									"tEnv (C),tSrcAir (C),"
						            "tInlet (C),tSetpoint (C),draw (gal),draw (L),");
#endif
				}
			}
			if (pF)
			{	double minHr = double( Top.iSubhr)*Top.subhrDur*60. + iT*Top.tp_subhrTickDur + 1;
				double minDay = double( 60*Top.iHr) + minHr;
				// double minYear = double( (int(Top.jDayST-1)*24+Top.iHrST)*60) + minHr;
#if defined( HPWH_DUMPSMALL)
				fprintf( pF, "%0.2f,%0.3f\n", minYear, GAL_TO_L( drawForTick));
#else
				wh_pHPWH->WriteCSVRow( pF, strtprintf(
						"%d,%d,%d,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.3f,%0.3f,",
						Top.tp_date.month, Top.tp_date.mday, Top.iHr+1, minHr, minDay,
						DegFtoC( wh_tEx), DegFtoC( wh_ashpTSrc),
						DegFtoC( pWS->ws_tInlet), DegFtoC( pWS->ws_tSetpoint),
						drawForTick, GAL_TO_L( drawForTick)));
#endif
			}
		}
#endif	// HPWH_DUMP
	}		// tick (1 min) loop

	// water heater average output temp, F
	if (nTickNZDraw)
	{	wh_tHWOut = tHWOutF / nTickNZDraw;	// note: >= ws_tUse due to unlimited add'l backup
		if (pWS->ws_tUse - wh_tHWOut > 1.f)
			wh_unMetSh++;			// unexpected
	}
	// else leave prior value = best available

	// link zone heat transfers
	if (wh_pZn)
	{	double qLPwr = qLoss * mult * BtuperkWh / Top.subhrDur;	// power to zone, Btuh
		wh_pZn->zn_qDHWLoss += qLPwr;
		// TODO: HPWH 50/50 conc/rad split is approx at best
		wh_pZn->zn_qDHWLossAir += qLPwr * 0.5;
		wh_pZn->zn_qDHWLossRad += qLPwr * 0.5;
	}

	if (wh_pAshpSrcZn && qEnv > 0.)
	{	// heat extracted from zone
		double qZn = qEnv * mult * BtuperkWh / Top.subhrDur;
		wh_pAshpSrcZn->zn_qHPWH -= qZn;
		// air flow: assume 20 F deltaT
		// need approx value re zone convective coefficient derivation
		double amfZn = float( qZn / (20. * Top.tp_airSH));
		wh_pAshpSrcZn->zn_hpwhAirX += float( amfZn / wh_pAshpSrcZn->zn_dryAirMass);
	}

#if defined( HPWH_QBAL)
	// form energy balance = sum of heat flows into water, all kWh
	double deltaHC = KJ_TO_KWH( wh_pHPWH->getTankHeatContent_kJ() - qHCStart);
	double qBal = qEnv								// HP energy extracted from surround
		        - qLoss								// tank loss
				+ wh_HPWHUse[ 0] + wh_HPWHUse[ 1]	// electricity in
				- qHW								// hot water energy
				- deltaHC;							// change in tank stored energy
	if (fabs( qBal) > .001)		// .0002 -> .001 3-30-2017
	{	// energy balance error
		static const int WHBALERRCOUNTMAX = 10;
		wh_balErrCount++;
		if (wh_balErrCount < WHBALERRCOUNTMAX)
			warn( "DHWHEATER '%s': HPWH energy balance error for %s (%1.6f kWh)",
				name,
				Top.When( C_IVLCH_S),	// date, hr, subhr
				qBal);   				// unbalance calc'd just above
		else if (wh_balErrCount == WHBALERRCOUNTMAX)
				warn( "DHWHEATER '%s': Skipping further energy balance warning messages.",
					name);
	}
#endif
	
	// output accounting = heat delivered to water
	wh_totOut += KWH_TO_BTU( qHW) + wh_HPWHxBU;

	// energy use accounting, Btu (electricity only, assume no fuel)
	double inElec   = wh_HPWHUse[ 1] * BtuperkWh + wh_parElec * Top.subhrDur * BtuperWh;
	double inElecBU = wh_HPWHUse[ 0] * BtuperkWh + wh_HPWHxBU;
	wh_inElec += inElec;		// hour total
	wh_inElecBU += inElecBU;
	if (wh_pMtrElec)
	{	wh_pMtrElec->H.dhw   += mult * inElec;
		wh_pMtrElec->H.dhwBU += mult * inElecBU;
	}

	return rc;
}		// DHWHEATER::wh_HPWHDoSubhr
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWTANK
///////////////////////////////////////////////////////////////////////////////
static FLOAT TankSurfArea_CEC(		// calc tank surface area per CEC methods
	float vol)	// tank volume, gal
// source: RACM 2016 App B, eqns 42; Table B8

// returns tank surface area, ft2
{
	double t1 = 1.254 * pow( vol, 0.33) + 0.531;
	float tsa = float( t1 * t1);

	return tsa;
}		// ::TankSurfArea_CEC
//--------------------------------------------------------------------
RC DHWTANK::wt_CkF()		// DHWTANK input check / default
// called at end of each DHWTANK input
{
	DHWSYS* pWS = wt_GetDHWSYS();
	RC rc = !pWS ? oer( "DHWSYS not found")	// insurance (unexpected)
				 : pWS->ws_CheckSubObject( this);

	if (!rc)
	{	if (!IsSet( DHWTANK_UA))
		{	float tsa = TankSurfArea_CEC( wt_vol);
			if (wt_insulR < 0.01f)
			{	rc |= ooer( DHWTANK_INSULR, "wtInsulR must be >= 0.01 so valid wtUA can be calculated");
				wt_insulR = 0.01;
			}
			wt_UA = tsa / wt_insulR;
		}
	}
	return rc;
}	// DHWTANK::wt_CkF
//----------------------------------------------------------------------------
RC DHWTANK::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup(pSrc, options);
	DHWSYS* pWS = wt_GetDHWSYS();
	pWS->ws_wtCount += wt_mult;		// count total # of water heaters

	return rc;
}		// DHWTANK::RunDup
//----------------------------------------------------------------------------
DHWSYS* DHWTANK::wt_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WtR ? WsR : WSiB).GetAtSafe( ownTi);
	return pWS;
}		// DHWTANK::wt_GetDHWSYS
//-----------------------------------------------------------------------------
RC DHWTANK::wt_DoHour(			// hourly unfired DHWTANK calcs
	float tUse)		// system water use temp, F
					// provides default iff wt_tTank not set
// returns RCOK on success, wt_qLoss set
//    else results unusable
{
	RC rc = RCOK;
	// resolve tank temp each hour (DHWSYS.wsTUse can vary hourly)
	float tTank = IsSet( DHWTANK_TTANK)
					? wt_tTank
					: tUse;
	// total loss (aka HJL in ACM App B)
	wt_qLoss = wt_UA * (tTank - wt_tEx) + wt_xLoss;
	return rc;
}		// DHWTANK::wt_DoHour
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// DHWPUMP
///////////////////////////////////////////////////////////////////////////////
RC DHWPUMP::wp_CkF()		// DHW pump input check / default
// called at end of each DHWPUMP input
{
	DHWSYS* pWS = wp_GetDHWSYS();
	RC rc = !pWS ? oer( "DHWSYS not found")	// insurance (unexpected)
				 : pWS->ws_CheckSubObject( this);
	return rc;
}	// DHWTANK::wp_CkF
//----------------------------------------------------------------------------
RC DHWPUMP::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup(pSrc, options);

	DHWSYS* pWS = wp_GetDHWSYS();
	pWS->ws_wpCount += wp_mult;		// count total # of pumps

	// default meter from parent system
	if (!IsSet( DHWPUMP_ELECMTRI))
		wp_elecMtri = pWS->ws_elecMtri;

	wp_pMtrElec = MtrB.GetAtSafe( wp_elecMtri);		// elec mtr or NULL

	return rc;
}		// DHWPUMP::RunDup
//----------------------------------------------------------------------------
DHWSYS* DHWPUMP::wp_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WpR ? WsR : WSiB).GetAtSafe( ownTi);
	return pWS;
}		// DHWPUMP::wh_GetDHWSYS
//-----------------------------------------------------------------------------
RC DHWPUMP::wp_DoHour(			// hourly DHWPUMP calcs
	int mult,		// system multiplier
					//    DHWPUMP = ws_mult
					//    DHWLOOPPUMP = ws_mult*wl_mult
	float runF/*=1.*/)	// fraction of hour pump runs
// returns RCOK on success
//    else results unusable
{
	RC rc = RCOK;
	wp_inElec = BtuperWh * runF * wp_pwr;		// electrical input, Btuh

	if (wp_pMtrElec)
		wp_pMtrElec->H.dhw += wp_inElec * mult * wp_mult;

	return rc;
}		// DHWPUMP::wp_DoHour
//============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWLOOP
///////////////////////////////////////////////////////////////////////////////
RC DHWLOOP::wl_CkF()		// DHW loop input check / default
// called at end of each DHWPUMP input
{
	DHWSYS* pWS = wl_GetDHWSYS();
	RC rc = !pWS ? oer( "DHWSYS not found")	// insurance (unexpected)
				 : pWS->ws_CheckSubObject( this);
	return rc;
}	// DHWLOOP::wl_CkF
//----------------------------------------------------------------------------
RC DHWLOOP::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup( pSrc, options);

	DHWSYS* pWS = wl_GetDHWSYS();
	pWS->ws_wlCount += wl_mult;

	return rc;
}		// DHWLOOP::RunDup
//----------------------------------------------------------------------------
RC DHWLOOP::wl_Init()		// init for run
{
	RC rc = RCOK;
	wl_wbCount = 0.f;		// branch count (may be non-integer)
	DHWLOOPSEG* pWG;
	RLUPC( WgR, pWG, pWG->ownTi == ss)
	{	rc |= pWG->wg_Init();
		wl_wbCount += pWG->wg_wbCount;
	}
	return rc;
}		// DHWLOOP::wl_Init
//----------------------------------------------------------------------------
DHWSYS* DHWLOOP::wl_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WlR ? WsR : WSiB).GetAt( ownTi);
	return pWS;
}		// DHWLOOP::wl_GetDHWSYS
//-----------------------------------------------------------------------------
RC DHWLOOP::wl_DoHour(		// hourly DHWLOOP calcs
	int wsMult)		// system multiplier
// returns RCOK on success
//    else results unusable
{
	RC rc = RCOK;

	wl_HRLL = 0.f;
	wl_HRBL = 0.f;

	float tIn = wl_tIn1;
	DHWLOOPSEG* pWG;
	RLUPC( WgR, pWG, pWG->ownTi == ss)
	{	// note: segment chain relies on input order
		rc |= pWG->wg_DoHour( tIn);
		wl_HRLL += pWG->ps_PL;
		wl_HRBL += pWG->wg_BL;
		tIn = pWG->ps_tOut;
	}

	if (wl_wlpCount > 0)		// if any loop pumps
	{	DHWLOOPPUMP* pWLP;
		RLUPC( WlpR, pWLP, pWLP->ownTi == ss)
			// calc electric energy use at wl_runF
			// accum to elect mtr with multipliers
			pWLP->wp_DoHour( wl_mult*wsMult, wl_runF);
	}

	wl_HRLL *= wl_mult;
	wl_HRBL *= wl_mult;

	return rc;
}		// DHWLOOP::wl_DoHour
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// PIPESEG: base class for DHWLOOPSEG and DHWLOOPBRANCH
//          implements common methods
///////////////////////////////////////////////////////////////////////////////
PIPESEG::PIPESEG( basAnc *b, TI i, SI noZ)		// c'tor
	: record( b, i, noZ)
{	ps_fRhoCpX =
		8.33f	// lb/gal
		* 60.f	// min / hr
		* 1.f;	// Btu/lb-F
}	// PIPESEG::PIPESEG
//----------------------------------------------------------------------------
float PIPESEG::ps_GetOD(
// returns outside diameter of pipe (w/o or w/ insulation), in
	int bInsul) const	// 0: get bare pipe OD
						// 1: get insulation OD
{	float OD = ps_size + 0.125f;
	if (bInsul)
		OD += 2.f * ps_insulThk;
	return OD;
}		// PIPESEG::ps_GetOD
//----------------------------------------------------------------------------
float PIPESEG::ps_CalcVol()		// pipe seg volume
// sets and returns ps_vol, gal
{
	float r = ps_GetOD( 0) / 24.f;	// pipe radius, ft
	// include tube wall in vol, approximates heat cap of tubing
	ps_vol = 7.48f * kPi * r * r * ps_len;
	return ps_vol;
}		// PIPESEG::ps_CalcVol
//----------------------------------------------------------------------------
float PIPESEG::ps_CalcUA(
	float fUA /*=1.f*/)
{
	float diaO = ps_GetOD( 0);	// bare pipe OD, in
	float Ubare = ps_exH * kPi * diaO / 12.f;

	float Uinsul;
	if (ps_insulThk < .001f)
		Uinsul = Ubare;
	else
	{	float diaX = ps_GetOD( 1);	// insulation OD, in
		double rIns = log( diaX/diaO) / (2.* ps_insulK);	// insulation resistance
															// ps_insulK units = Btuh-ft/ft2-F
		double rSrf = 12./(ps_exH * diaX);			// surface restance
		Uinsul = float( kPi / (rIns + rSrf));
#if 0 && defined( _DEBUG)
0 test code: for diaX >> insulThk, Uround == Uflat approx
0		float Uround = Uinsul / (Pi * diaX/12.f);
0		float Uflat = 1.f/(ps_insulThk/12.f/ps_insulK + 1.f/ps_exH);
#endif
	}
	ps_UA = ps_len * min( Ubare, fUA*Uinsul);
	return ps_UA;
}		// PIPESEG::ps_CalcUA
//----------------------------------------------------------------------------
float PIPESEG::ps_CalcTOut(		// calc outlet or ending temp
	float tIn,			// inlet / beginning temp, F
	float flow)	const	// flow, gpm
// returns outlet / cooldown temp
{
	float tOut = ps_exT;
	if (flow > .00001f)		// if flow
	{	double f = exp( - double( ps_UA / (flow * ps_fRhoCpX)));
		tOut += float( f * (tIn - ps_exT));
	}
	return tOut;
}		// PIPESEG::ps_CalcTOut
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWLOOPSEG
///////////////////////////////////////////////////////////////////////////////
RC DHWLOOPSEG::wg_CkF()		// DHW loop segment input check / default
// called at end of each DHWLOOPSEG input
{
	RC rc = RCOK;
	return rc;
}	// DHWLOOPSEG::wg_CkF
//----------------------------------------------------------------------------
RC DHWLOOPSEG::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup( pSrc, options);

	return rc;
}		// DHWLOOPSEG::RunDup
//----------------------------------------------------------------------------
RC DHWLOOPSEG::wg_Init()		// init for run
{
	RC rc = RCOK;
	wg_CalcUA();
	ps_CalcVol();

	wg_wbCount = 0.f;
	DHWLOOPBRANCH* pWB;
	RLUPC( WbR, pWB, pWB->ownTi == ss)
	{	rc |= pWB->wb_Init();
		wg_wbCount += pWB->wb_mult;
	}

	return rc;
}		// DHWLOOPSEG::wl_Init
//----------------------------------------------------------------------------
DHWLOOP* DHWLOOPSEG::wg_GetDHWLOOP() const
{
	DHWLOOP* pWL = (b == &WgR ? WlR : WLiB).GetAtSafe( ownTi);
	return pWL;
}		// DHWLOOPSEG::wg_GetDHWLOOP
//----------------------------------------------------------------------------
DHWSYS* DHWLOOPSEG::wg_GetDHWSYS() const
{
	DHWLOOP* pWL = wg_GetDHWLOOP();
	DHWSYS* pWS = pWL ? pWL->wl_GetDHWSYS() : NULL;
	return pWS;
}		// DHWLOOPSEG::wg_GetDHWSYS
//----------------------------------------------------------------------------
float DHWLOOPSEG::wg_CalcUA()
{
	DHWLOOP* pWL = wg_GetDHWLOOP();
	return ps_CalcUA( pWL->wl_fUA);
}		// DHWLOOPSEG::wg_CalcUA
//-----------------------------------------------------------------------------
RC DHWLOOPSEG::wg_DoHour(			// hourly DHWLOOPSEG calcs
	float tIn)		// segment inlet temp, F
// returns RCOK on success
//    else results unusable
{
	RC rc = RCOK;

	DHWSYS* pWS = wg_GetDHWSYS();
	DHWLOOP* pWL = wg_GetDHWLOOP();

	// flow rate, gpm
	float fNoFlow = (1.f - pWL->wl_runF) * wg_fNoDraw;
	if (fNoFlow < 1.f)
	{	ps_fvf = pWL->wl_flow;					// recirc, gpm
		if (wg_ty == C_DHWLSEGTYCH_SUP)			// if supply segment
			ps_fvf += pWS->ws_fxUseHot / (pWS->ws_wlCount * 60.f);	// draw, gpm
	}
	else
		ps_fvf = 0.f;

	// outlet temp
	ps_tIn = tIn;
	ps_tOut = ps_CalcTOut( tIn, ps_fvf);

	// heat loss
	if (fNoFlow < 1.f)
		ps_PLWF = ps_fvf * (1.f - fNoFlow) * ps_fRhoCpX * (ps_tIn - ps_tOut);
	else
		ps_PLWF = 0.f;

	if (fNoFlow > 0.f)
	{	float tStart = (ps_tIn + ps_tOut) / 2.f;
		float volX = ps_vol / 60.f;
		float tEnd = ps_CalcTOut( tStart, volX/fNoFlow);
		ps_PLCD = volX * ps_fRhoCpX * (tStart - tEnd);
	}
	else
		ps_PLCD = 0.f;

	ps_PL = ps_PLWF + ps_PLCD;

	// branch losses
	wg_BL = 0.f;
	if (wg_wbCount > 0.f)
	{	DHWLOOPBRANCH* pWB;
		RLUPC( WbR, pWB, pWB->ownTi == ss)
		{	rc |= pWB->wb_DoHour( ps_tIn);		// TODO: inlet temp?
			wg_BL += pWB->wb_mult * (pWB->wb_HBUL + pWB->wb_HBWL);
		}
	}

	return rc;
}		// DHWLOOPSEG::wg_DoHour
//============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWLOOPBRANCH
///////////////////////////////////////////////////////////////////////////////
RC DHWLOOPBRANCH::wb_CkF()		// DHW loop branch input check / default
// called at end of each DHWPUMP input
{
	RC rc = RCOK;
	return rc;
}	// DHWLOOPBRANCH::wb_CkF
//----------------------------------------------------------------------------
RC DHWLOOPBRANCH::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup(pSrc, options);

    const DHWLOOPSEG* pWG = wb_GetDHWLOOPSEG();
    if (pWG->wg_ty != C_DHWLSEGTYCH_SUP)
        rc |= oer( "Parent DHWLOOPSEG wgTy must be SUPPLY");

	return rc;
}		// DHWLOOPBRANCH::RunDup
//----------------------------------------------------------------------------
RC DHWLOOPBRANCH::wb_Init()		// init for run
{	RC rc = RCOK;
	wb_CalcUA();
	ps_CalcVol();
	return rc;
}		// DHWLOOPBRANCH::wb_Init
//----------------------------------------------------------------------------
DHWLOOPSEG* DHWLOOPBRANCH::wb_GetDHWLOOPSEG() const
{
	DHWLOOPSEG* pWG = (b == &WbR ? WgR : WGiB).GetAtSafe( ownTi);
	return pWG;
}		// DHWLOOPBRANCH::wb_GetDHWSYS
//----------------------------------------------------------------------------
DHWLOOP* DHWLOOPBRANCH::wb_GetDHWLOOP() const
{
	DHWLOOPSEG* pWG = wb_GetDHWLOOPSEG();
	DHWLOOP* pWL = pWG ? pWG->wg_GetDHWLOOP() : NULL;
	return pWL;
}		// DHWLOOPBRANCH::wb_GetDHWLOOP
//----------------------------------------------------------------------------
DHWSYS* DHWLOOPBRANCH::wb_GetDHWSYS() const
{
	DHWLOOPSEG* pWG = wb_GetDHWLOOPSEG();
	DHWSYS* pWS = pWG ? pWG->wg_GetDHWSYS() : NULL;
	return pWS;
}		// DHWLOOPBRANCH::wb_GetDHWSYS
//----------------------------------------------------------------------------
float DHWLOOPBRANCH::wb_CalcUA()
{
	return ps_CalcUA();		// loop wl_fUA not applied to branches
}		// DHWLOOPBRANCH::wb_CalcUA
//-----------------------------------------------------------------------------
RC DHWLOOPBRANCH::wb_DoHour(			// hourly DHWLOOPBRANCH calcs
	float tIn)		// branch fluid inlet temp, F
// returns RCOK on success (wb_HBUL and wb_HBWL set)
//    else results unusable
{
	RC rc = RCOK;
	ps_tIn = tIn;
	DHWSYS* pWS = wb_GetDHWSYS();
	DHWLOOP* pWL = wb_GetDHWLOOP();

	// loss while water in use
	//   outlet temp found assuming use flow rate
	//   energy lost found using average flow rate
	ps_tOut = ps_CalcTOut( ps_tIn, wb_flow);
	ps_fvf = pWS->ws_BranchFlow();		// average flow rate
	wb_HBUL = ps_fvf * ps_fRhoCpX * (ps_tIn - ps_tOut);

	// waste loss
	wb_HBWL = wb_fWaste * ps_vol * (ps_fRhoCpX/60.f) * (ps_tIn - pWS->ws_tInlet);

	// note wb_mult applied by caller

	return rc;
}		// DHWLOOPBRANCH::wb_DoHour
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWLOOPPUMP
//   derived from DHWPUMP, minor behavior diffs only
///////////////////////////////////////////////////////////////////////////////
RC DHWLOOPPUMP::wlp_CkF()		// DHW loop pump input check / default
// called at end of each DHWPUMP input
{
	RC rc = RCOK;
	return rc;
}	// DHWLOOPPUMP::wlp_CkF
//----------------------------------------------------------------------------
RC DHWLOOPPUMP::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,
	int options /*=0*/)
{
	RC rc = record::RunDup(pSrc, options);

	DHWLOOP* pWL = wlp_GetDHWLOOP();
	pWL->wl_wlpCount += wp_mult;		// count total # of pumps on loop

	// default meter from parent system
	DHWSYS* pWS = wlp_GetDHWSYS();
	if (!IsSet( DHWLOOPPUMP_ELECMTRI))
		wp_elecMtri = pWS->ws_elecMtri;

	wp_pMtrElec = MtrB.GetAtSafe( wp_elecMtri);		// elec mtr or NULL

	return rc;
}		// DHWLOOPPUMP::RunDup
//----------------------------------------------------------------------------
DHWLOOP* DHWLOOPPUMP::wlp_GetDHWLOOP() const
{
	DHWLOOP* pWL = (b == &WlpR ? WlR : WLiB).GetAtSafe( ownTi);
	return pWL;
}		// DHWLOOPPUMP::wlp_GetDHWSYS
//----------------------------------------------------------------------------
DHWSYS* DHWLOOPPUMP::wlp_GetDHWSYS() const
{
	DHWLOOP* pWL = wlp_GetDHWLOOP();
	DHWSYS* pWS = pWL->wl_GetDHWSYS();
	return pWS;
}		// DHWLOOPPUMP::wlp_GetDHWSYS
//=============================================================================


// DHWCalc.cpp end