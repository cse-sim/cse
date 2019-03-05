// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
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

#include <random>

#include "cnguts.h"
#include "exman.h"

#include "hpwh.hh"	// decls/defns for Ecotope heat pump water heater model

const float waterRhoCp = 8.345f;		// water volumetric weight, lb/gal
										//    = volumetric heat capacity, Btu/lb-F


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
//-----------------------------------------------------------------------------
static inline float DHWMixF(
	float tMix,		// target mixed water temp
	float tHot,		// available hot water temp
	float tCold)	// available cold water temp
// returns: required hot fraction, 0 - 1 (errors ignored)
{	float fHot;
	DHWMix( tMix, tHot, tCold, fHot);
	return fHot;
}		// DHWMixF
//=============================================================================

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
// local structures
struct DHWFX		// info about a fixture
{
	DHWFX( DHWEUCH hwEndUse=0, int drainCnx=0, int coldCnx=0)
		: fx_hwEndUse(hwEndUse), fx_drainCnx(drainCnx), fx_coldCnx(coldCnx), fx_hitCount( 0)
	{}
	void fx_Set(DHWEUCH hwEndUse, int drainCnx, int coldCnx)
	{
		fx_hwEndUse = hwEndUse;
		fx_drainCnx = drainCnx;
		fx_coldCnx = coldCnx;
		fx_hitCount = 0;
	}

	DHWEUCH fx_hwEndUse;
	int fx_drainCnx;		// 0 = discarded
							// else idx of DHWHEATREC
	int fx_coldCnx;			// 0 = mains
							// 1 = DHWHEATREC
	int fx_hitCount;		// count of draws assigned to this fixture
							//   re assessment of randomization algorithms
};		// struct DHWFX
//-----------------------------------------------------------------------------
struct DWHRUSE		// info about 1 (shower) draw that could have DWHR
{
	DWHRUSE() : wdw_iFx( -1), wdw_coldCnx( 0), wdw_vol( 0.f), wdw_volHR( 0.f), wdw_temp( 0.f)
	{}
	DWHRUSE( int iFx, int coldCnx, float vol, float volHR, float temp)
		: wdw_coldCnx( coldCnx), wdw_vol( vol), wdw_volHR( volHR), wdw_temp( temp)
	{}
	~DWHRUSE() {}

	int wdw_iFx;		// DHWSYS.ws_fxList index of fixture where draw occurs
						//   assigned randomly, see DHWUSE::wu_DoHour1()
	int wdw_coldCnx;	// fixture cold water source, 0=mains  1=DHWHEATREC
	float wdw_vol;		// mixed water use at fixture for tick, gal
						//   = DHWUSE.wu_flow (gpm) * tick/draw overlap (min)
	float wdw_volHR;	// water use having recoverable heat during tick, gal
						//   re representation of warmup waste
	float wdw_temp;		// mixed use temp at fixture, F

};		// struct DHWHRUSE
//-----------------------------------------------------------------------------
struct DHWTICK	// per tick info for DHWSYS
{
	DHWTICK() { wtk_Init(); }
	double wtk_whUse;		// total tick hot water draw at all water heaters, gal
	float wtk_tInletX;		// post-DWHR cold water temperature for this tick, F
							//   = DHWSYS.ws_tInlet if no DWHR
	int wtk_nHRDraws;		// # of DHWHEATREC draws during this tick
	void wtk_Init( double whUseTick=0., float tInlet=50.f)
	{
		wtk_nHRDraws = 0;
		wtk_whUse = whUseTick; wtk_tInletX = tInlet;
	}
	void wtk_Accum( const DHWTICK& s, double mult);
};	// struct DHWTICK
//-----------------------------------------------------------------------------
void DHWTICK::wtk_Accum(		// accumulate tick info (re central parent/child)
	const DHWTICK& s,	// source
	double mult)		// multiplier
{
	double sWhUse = s.wtk_whUse * mult;		// source use
	double tWhUse = wtk_whUse + sWhUse;		// new total use
	if (tWhUse > 0.)
	{	wtk_tInletX = (wtk_whUse*wtk_tInletX + sWhUse*s.wtk_tInletX)
						/ tWhUse;
		wtk_whUse = tWhUse;
	}
	// else leave wtk_tInletX

	// wtk_dwhrDraws: not needed (DWHR results are in wtk_Use and wtk_tInletX
}		// DHWTICK::wtk_Accum
//-----------------------------------------------------------------------------
struct DHWHRTICK	// per tick info for DHWHEATREC
{
	DHWHRTICK()
	{}
	void wrtk_Init()
	{
		wrtk_draws.resize(0);
	}
	WVect< DWHRUSE> wrtk_draws;	// all draws for this DHWHEATREC for this tick

};		// struct DHWHRTICK
//-----------------------------------------------------------------------------
DHWSYS::~DHWSYS()
{
	cupfree( DMPP( ws_dayUseName));
	delete[] ws_ticks;
	ws_ticks = NULL;
	delete[] ws_fxList;
	ws_fxList = NULL;
}		// DHWSYS::~DHWSYS
//-------------------------------------------------------------------------------
/*virtual*/ void DHWSYS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	record::Copy( pSrc);
	cupIncRef( DMPP( ws_dayUseName));   // incr reference counts of dm strings if non-NULL
										//   nop if ISNANDLE
	// assume ws_ticks and ws_fxList are NULL
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
	{	float SSFsave = ws_SSF;
		if (ifBracket( 0.f, ws_SSF, 0.99f))
		{	rc |= oer( "Bad wsSSF=%0.3f: value must be in the range 0 - 0.99",
				SSFsave);
		    // calc may continue until rc checked
		}
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
int DHWSYS::ws_IsCentralDHWSYS() const
{	// note: dicey for input records
	//   ws_childSYSCount is derived in ws_Init() pass 1
#if defined( _DEBUG)
	if (b == &WSiB)
		err( PERR, "ZNRES::ws_IsCentralDHWSYS '%s': called on input record",
			name);
#endif
	return ws_childDHWSYSCount > 0;
}		// DHWSYS::ws_IsCentralDHWSYS
//-----------------------------------------------------------------------------
RC DHWSYS::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	ws_whCount = 0.f;		// insurance
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
		ws_childDHWSYSCount = 0;
		DHWSYS* pWSCentral = ws_GetCentralDHWSYS();
		if (pWSCentral)
		{	if (pWSCentral->IsSet( DHWSYS_SHOWERCOUNT))
				pWSCentral->ooer( DHWSYS_SHOWERCOUNT, "wsShowerCount not allowed on central DHWSYS");
			pWSCentral->ws_showerCount = 0;		// shower count derived from child DHWSYSs (see below)
		}
		return rc;
	}

	if (pass == 2)
	{	// final pass: set ws_loadShareCount to group value
		if (ws_loadShareDHWSYSi > 0)
		{	DHWSYS* pWS = WsR.GetAtSafe( ws_loadShareDHWSYSi);
			if (pWS)	// insurance: NULL impossible?
				ws_loadShareCount = pWS->ws_loadShareCount;
		}

		DHWHEATREC* pWR;
		ws_wrCount = 0;			// count of child DHWHEATRECs
		ws_wrFeedWHCount = 0;	// count of child DHWHEATRECs feeding DHWHEATER cold inlet
		ws_wrFxDrainCount = 0;	// count of DHWHEATREC drains
		RLUPC( WrR, pWR, pWR->ownTi == ss)
		{	rc |= pWR->wr_Init();		// init for run
			if (pWR->wr_mult > 0)
			{
				ws_wrCount += pWR->wr_mult;
				if (pWR->wr_FeedsWH())
					ws_wrFeedWHCount += pWR->wr_mult;

				ws_wrFxDrainCount += pWR->wr_nFXDrain * pWR->wr_mult;	// # of fixture drains
																			// connected to DHWHEATREC
				if (!IsSet(DHWSYS_DAYUSENAME))
					pWR->oInfo("DHWSys has no wsDayUse, heat recovery not modeled.");
			}
		}

		// check DHWHEATREC configuration
		//   do not need ws_mult, applies to both values
		if (ws_wrFxDrainCount > ws_showerCount)
			rc |= oer( "Invalid heat recovery arrangement: more DHWHEATREC drain connections (%d) than showers (%d)",
					ws_wrFxDrainCount, ws_showerCount);
		
		// set up DHWSYS fixture list
		//    associates each fixture with DHWHEATREC (or not)
		//    only C_DHWEUCH_SHOWER supported as of 2-19
		//    order does not matter: scrambled when used (see ws_AssignDHWUSEtoFX())
		delete ws_fxList;
		ws_fxList = NULL;
		if (!rc && ws_showerCount > 0)
		{
			ws_fxList = new DHWFX[ws_showerCount];

			// set up linkage to DHWHEATRECs
			//   assign for each DHWHEATREC in order
			//   fixtures in excess of DHWHEATRECs don't drain via DHWHEATREC
			int iFx = 0;
			RLUPC(WrR, pWR, pWR->ownTi == ss)
				rc |= pWR->wr_SetFXConnections(this, iFx);
			// any remaining showers are linked to "no DHWHEATREC"
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
			rc |= oer( "DHWSYS '%s' (given by wsCentralDHWSYS) is not central",
				pWSCentral->name);

		pWSCentral->ws_childDHWSYSCount += ws_mult;
		pWSCentral->ws_showerCount += ws_showerCount*ws_mult;	// total # of shower fixtures served

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

	// array of per-tick info
	delete[] ws_ticks;	// insurance (generally NULL already)
	ws_ticks = new DHWTICK[ Top.tp_NHrTicks()];

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
					? ws_whUse.total / (ws_wbCount * 60.f)
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

	ws_qDWHR = 0.f;		// DWHR (DHWHEATREC) recovered heat hour total

	if (ivl <= C_IVLCH_D)	// if start of day (or longer)
	{
		if (Top.tp_isBegMainSim)
		{	// reset annual values after autosize / warmup
			if (ws_fxList)
				for (int iFx = 0; iFx < ws_showerCount; iFx++)
					ws_fxList[iFx].fx_hitCount = 0;
		}
		
		if (IsSet( DHWSYS_DAYUSENAME))
		{	// beg of day: locate DHWDAYUSE, set ws_dayUsei
			if (WduR.findRecByNm1( ws_dayUseName, &ws_dayUsei, NULL))
				return orMsg( ERR+SHOFNLN, "DHWDAYUSE '%s' not found.", ws_dayUseName);
		}

		// re load share -- init starting DHWSYS for each end use
		// provides some randomization
		// basing on jDay yields consistent results for part-year short runs
		int seed = Top.jDay;
		for (int iEU=1; iEU<NDHWENDUSES; iEU++)
			ws_loadShareWS0[ iEU] = (seed+iEU)%ws_loadShareCount;
	}

	// inlet temp = feed water to cold side of WHs
	if (!IsSet( DHWSYS_TINLET))
		ws_tInlet = Wthr.d.wd_tMains;		// default=mains

	// runtime checks of vals possibly set by expressions
	rc |= ws_CheckVals( ERR);	// checks ws_SSF, ws_tUse, ws_tSetpoint

	// adjusted inlet temp
	// assume solar preheats feed water
	// further changes later re DHWR
	ws_tInletX = ws_AdjustTInletForSSF( ws_tInlet);

	// distribution loss multiplier
	//   SDLM = standard distribution loss multiplier
	//          depends on unit floor area
	//   DSM = distribution system multiplier
	//         depends on distribution system configuration
	ws_DLM = 1.f + (ws_SDLM-1.f)*ws_DSM;

	// construct array of use factors by end use
	//   whDrawF = water heater draw / fixture hot water draw
	//   water heater draw includes SSF adjustment
	static const int WHDRAWFDIM = sizeof( ws_whDrawDurF)/sizeof( ws_whDrawDurF[ 0]);
#if defined( _DEBUG)
	if (WHDRAWFDIM != NDHWENDUSES)
		err( PABT, "ws_whDrawDurF array size error");
	VSet( ws_whDrawDurF, WHDRAWFDIM, -1.f);
#endif

	// Duration factor
	float whDrawDurF = ws_WF * ws_DLM;
	// temperature-dependent end uses
	//   losses modeled by extending draw
	ws_whDrawDurF[ 0]
		= ws_whDrawDurF[ C_DHWEUCH_SHOWER]
		= ws_whDrawDurF[C_DHWEUCH_FAUCET]
		= ws_whDrawDurF[ C_DHWEUCH_BATH] = whDrawDurF;
	// temperature independent end uses
	//   losses do not effect draw
	float whDrawDurFTempInd = 1.f;
	ws_whDrawDurF[ C_DHWEUCH_CWASHR]
		= ws_whDrawDurF[ C_DHWEUCH_DWASHR]
		= whDrawDurFTempInd;

#if defined( _DEBUG)
	if (VMin( ws_whDrawDurF, WHDRAWFDIM) < 0.f)
		err( PABT, "ws_whDrawDurF fill error");
#endif

	// ** Hot water use **
	//   2 types of user input
	//   * ws_hwUse = use for current hour (generally an expression w/ schedule)
    //   * DHWDAYUSE = collection of times and flows
	// here we combine these to derive consistent hourly total and tick-level (1 min) bins

	// init tick bins to average of hourly (initializes for hour)
	double hwUseX = ws_hwUse / ws_loadShareCount;	// hwUse per system
	// note ws_fxUseMixLH is set in ws_EndIvl()
	ws_InitTicks( hwUseX);
	ws_fxUseMix.wmt_Clear();
	ws_whUse.wmt_Clear();

	// init water meter value
	//   ws_hwUse is enduse 0 (unknown)
	//   wdu_DoHour accums add'l DHWDAYUSE draws to these values
	ws_fxUseMix.wmt_AccumEU( 0, hwUseX);
	ws_whUse.wmt_AccumEU( 0, hwUseX);
	ws_whUseNoHR = ws_whUse.total;	// water use w/o heat recovery
									//   more added in wdu_DoHour

	DHWDAYUSE* pWDU = WduR.GetAtSafe( ws_dayUsei);	// ref'd DHWDAYUSE can vary daily
	if (pWDU)
	{	// accumulation DHWDAYUSE input to tick bins and total use
		rc |= pWDU->wdu_DoHour( this);		// accum DAYUSEs
		if (ws_wrCount && ws_iTk0DWHR < ws_iTkNDWHR)
			rc |= ws_DoHourDWHR();		// modify tick values re DWHR
	}

	if (!ws_HasCentralDHWSYS())
	{	DHWSYS* pWSChild;
		RLUPC( WsR, pWSChild, pWSChild->ws_centralDHWSYSi == ss)
		{	rc |= pWSChild->ws_DoHour( ivl, ws_mult);
			rc |= ws_AccumCentralUse( pWSChild);
		}
	}

#if 0 && defined( _DEBUG)
	// check: compare tick totals to full hour values
	float whUseSum = 0.f;
	float tInletXAvg = 0.f;
	for (int iTk=0; iTk<Top.tp_NHrTicks(); iTk++)
	{	const DHWTICK& tk = ws_ticks[ iTk];
		whUseSum += tk.wtk_whUse;
		tInletXAvg += tk.wtk_tInletX * tk.wtk_whUse;
	}
	if (whUseSum > 0.)
		tInletXAvg /= whUseSum;
	else
		tInletXAvg = ws_tInletX;
	if (frDiff( whUseSum, ws_whUse.total) > .001
	 || frDiff( tInletXAvg, ws_tInletX) > .001)
		printf( "\nDHWSYS '%s': tick average inconsistency", name);
#endif


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
	double HRLL = 0.;
	ws_HRBL = 0.f;
	ws_volRL = 0.f;
	double tVolRet = 0.;
	if (ws_wlCount > 0)		// if any loops
	{	DHWLOOP* pWL;
		RLUPC( WlR, pWL, pWL->ownTi == ss)
		{	rc |= pWL->wl_DoHour( mult);		// also calcs child DHWLOOPSEGs and DHWLOOPPUMPs
			HRLL += pWL->wl_HRLL;	// loop loss
			ws_HRBL += pWL->wl_HRBL;	// branch loss
			ws_volRL += pWL->wl_volRL;
		}
	}

	ws_HRDL = float( HRLL + ws_HRBL);

	// total recovery load
	ws_HHWO = waterRhoCp * ws_whUse.total * (ws_tUse - ws_tInletX);
#if 0 && defined( _DEBUG)
	if (ws_fxUseMix.shower > 0.f)
	{	float fHotSHNoHR;
		DHWMix( 105., ws_tUse, ws_tInlet, fHotSHNoHR);
		float useSHNoHR = ws_fxUseMix.shower * fHotSHNoHR;
		float useNoHR = ws_whUse.total - ws_whUse.shower + useSHNoHR;
		float qXNoHR = useNoHR       * waterRhoCp * (ws_tUse - ws_tInlet);
		float qX    = ws_whUse.total * waterRhoCp * (ws_tUse - ws_tInletX);
		float qXR = qXNoHR - qX;
		if (frDiff(ws_qDWHR, qXR, .01f) > .001f)
			printf( "\nDHWSYS '%s': HR heat balance error", name);
	}
#endif
	ws_HARL = ws_HHWO + ws_HRDL + ws_HJL;

	if (ws_whCount > 0.f)
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
	ws_whUseNoHR += pWSChild->ws_whUseNoHR * mult;
	ws_qDWHR += pWSChild->ws_qDWHR * mult;
	int nTk = Top.tp_NHrTicks();
	for (int iTk=0; iTk<nTk; iTk++)
		ws_ticks[ iTk].wtk_Accum( pWSChild->ws_ticks[ iTk], mult);
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
int DHWSYS::ws_AssignDHWUSEtoFX(	// assign draw to fixture re DHWHEATREC
	const DHWUSE* pWU)	// draw
// WHY: DHWSYS fixtures (ws_fxList) are associated with DHWHEATRECs
//      Here we assign a draw to a fixture for later heat recovery modeling
// returns -1 if draw does not have a fixture
//    else ws_fxList[ ] idx
{
	// determine if heat recovery possible
	//   some of these checks may be redundant due to input error checking
	if (ws_wrCount <= 0							// no DHWHEATRECs
	 || pWU->wu_hwEndUse != C_DHWEUCH_SHOWER	// unsupported end use
	 || pWU->wu_heatRecEF > 0.f					// draw uses fixed heat recovery
	 || !pWU->IsSet(DHWUSE_TEMP)				// draw does not specify a use temp
	 || pWU->IsSet( DHWUSE_HOTF)				// draw has specified hot fraction
	 || ws_showerCount <= 0)					// no showers
		return -1;			// no fixture / no DWHR via DHWHEATREC

// result must be stable for same wu_drawSeqN
//   WHY: draws can span hour boundary, should go to same fixture
//   >>> can't use Top.iHr
// pWU->ss varies with pWU->wu_drawSeqN, but pWU->ss order is not known

	unsigned int seq;
#if 0
	static int bSetup = 0;
	static int iRands[101];+		this	0x0012d71e {ws_calcMode=2 ws_centralDHWSYSi=0 ws_loadShareDHWSYSi=0 ...}	DHWSYS *

	static int counts[20] = { 0 };
	static int drawSeqCount[200] = { 0 };
	static int d2Count[41] = { 0 };
	if (!bSetup)
	{
		std::mt19937 gen(1);  // to seed mersenne twister.
		std::uniform_int_distribution<> dist(0, 100); // distribute results between 0 and 100 inclusive

		for (int i = 0; i <= 100; ++i)
		{
			// iRands[i] = dist(gen);
			iRands[i] = i;
			// counts[iRands[i] % ws_showerCount]++;
		}
		bSetup++;
	}
	drawSeqCount[pWU->wu_drawSeqN]++;
	seq = (pWU->wu_drawSeqN * 97 + Top.jDay*7) % 41;
	d2Count[seq]++;
	counts[seq%ws_showerCount]++;

	// seq = iRands[((pWU->wu_drawSeqN+1)*Top.jDay) % 101];
#elif 0
	seq = pWU->wu_drawSeqN + pWU->ss + Top.jDay;
#elif 0
	seq = (pWU->wu_drawSeqN+1) * Top.jDay;
	seq ^= seq << 13;
	seq ^= seq >> 17;
	seq ^= seq << 5;
#else
	seq = pWU->wu_drawSeqN + Top.jDay;
#endif
	static int d2Count[ 1000] = { 0 };
	d2Count[seq]++;
	int iFx = seq % ws_showerCount;

#if 0
	if (Top.jDay == 365 && Top.iHr==23 && !Top.isWarmup)
		printf("\nHit");
#endif

	return iFx;

}	// DHWSYS::ws_AssignDHWUSEtoFX
//----------------------------------------------------------------------------
void DHWSYS::ws_InitTicks(			// initialize tick data for hour
	double whUseHr)		// base hw use (at water heater(s)) for hour, gal
						//   supports non-DHWUSE draws
{	
	int nTk = Top.tp_NHrTicks();
	double whUseTick = whUseHr / nTk;
	for (int iTk=0; iTk < nTk; iTk++)
		ws_ticks[ iTk].wtk_Init( whUseTick, ws_tInletX);

	DHWHEATREC* pWR;
	RLUPC(WrR, pWR, pWR->ownTi == ss)
		pWR->wr_InitTicks();

	ws_iTk0DWHR = 9999;		// 1st/nth tick with possible DWHR
	ws_iTkNDWHR = -1;
}		// DHWSYS::ws_InitTicks
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoHourDWHR()		// current hour DHWHEATREC modeling
{
	RC rc = RCOK;

#if 0
	if (Top.jDay == 91 && Top.iHr == 20)
		printf( "\nHit");
	if (Top.iHr == 19 && iTk == 17)
		printf( "\nHit");
#endif

	// loop ticks that could include DWHR draws
	// ws_qDWHR = 0.f;			// total heat recovered by all DHWHEATREC devices, Btu
	// ws_whUseNoHR = 0.;		// check value: hour total hot water use w/o HR, gal
								//  init'd by caller
	double qRWHSum = 0.;
	// int nTk = Top.tp_NHrTicks();
	for (int iTk=ws_iTk0DWHR; iTk < ws_iTkNDWHR; iTk++)
	{	DHWTICK& tk = ws_ticks[ iTk];		// DHWSYS tick info
		if (tk.wtk_nHRDraws == 0)
			continue;		// no DHWHEATREC draws in this tick
		float vOther		// non-DHWHEATREC draws that contribute to each
							//   feedWH-DHWHEATREC potable-side vol
			= tk.wtk_whUse / max(ws_wrFeedWHCount, 1);
		float whUse = 0.f;		// hot water use, this tick / all DHWHEATREC draws
		float fxUseMix = 0.f;	// mixed water use
		float qR = 0.f;			// tick heat recovered
		float qRWH = 0.f;		// tick heat recovered to WH feed
		DHWHEATREC* pWR;
		RLUPC(WrR, pWR, pWR->ownTi == ss)
		{	DHWHRTICK& wrtk = pWR->wr_ticks[iTk];	// DHWHEATREC tick info
			if (wrtk.wrtk_draws.size() > 0)
				whUse += pWR->wr_CalcTick( this,
					wrtk,			// tick info for *pWR
					vOther,			// total non-HR hot water use, gal
					ws_whUseNoHR, fxUseMix, qR, qRWH);	// results accum'd
		}
		ws_AccumUseTick(		// accum to ws_tick, ws_whUse, and ws_fxMixUse
			C_DHWEUCH_SHOWER, iTk, fxUseMix, whUse);
		float tO = ws_tInlet;
		if (tk.wtk_whUse > 0.)
			tO += qRWH / (waterRhoCp * tk.wtk_whUse);
#if defined( _DEBUG)
		else if (qRWH > 0.f)
			printf("\nInconsistency: wtk_whUse=%0.3f  qRWHWt=%0.3f",
				tk.wtk_whUse, qRWH);
#endif
		tk.wtk_tInletX = ws_AdjustTInletForSSF(tO);
		qRWHSum += qRWH;
		ws_qDWHR += qR;		// accum to hour total heat recovered
	}  // end tick

#if 0 && defined( _DEBUG)
	if (frDiff( float( whUseTot), ws_whUse.total, .001f) > .001f)
		printf( "\nMismatch!");
#endif

	// calc hour average adjusted inlet and hot water temps
	float tInletXNoSSF = ws_tInlet;
	if (qRWHSum > 0.)
	{	tInletXNoSSF = ws_tInlet + qRWHSum / (waterRhoCp * ws_whUse.total);
		ws_tInletX = ws_AdjustTInletForSSF( tInletXNoSSF);
	}

#if defined( _DEBUG)
	float qXNoHR = ws_whUseNoHR   * waterRhoCp * (ws_tUse - ws_tInlet);
	float qX =     ws_whUse.total * waterRhoCp * (ws_tUse - tInletXNoSSF);
	if (frDiff(qX+ws_qDWHR, qXNoHR, 1.f) > .001f)
		printf( "\nDHWSYS '%s': ws_DoHourDWHR balance error", name);
#endif
	
	return rc;
}		// DHWSYS::ws_DoHourDWHR
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoSubhr()		// subhourly calcs
{
	RC rc = RCOK;
	if (ws_whCount > 0.f && ws_calcMode != C_WSCALCMODECH_PRERUN)
	{	int iTkSh = Top.iSubhr*Top.tp_nSubhrTicks;	// initial tick for this subhour
		const DHWTICK* ticksSh = ws_ticks + iTkSh;		// 1st tick info for this subhour
		double scaleWH = 1./ws_whCount;					// allocate per WH
		double scaleTick = scaleWH / Top.tp_NHrTicks();	// allocate per WH-tick
		// non-draw losses imposed on DHWHEATER(s), Btu/WH-tick
		double qLoss     = (ws_HRDL + ws_HJL) * scaleTick;	// total: DHWLOOP + jacket
		double qLossNoRL = (ws_HRBL + ws_HJL) * scaleTick;	// w/o recirc: DHWLOOP branch + jacket
		double qLossRL = qLoss - qLossNoRL;					// recirc only
		double volRL = ws_volRL * scaleTick;				// recirc vol/WH-tick, gal

		DHWHEATER* pWH;
		RLUPC( WhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_IsHPWHModel())
				rc |= pWH->wh_HPWHDoSubhr(
					ticksSh,			// draws etc. by tick
					scaleWH,		// draw scale factor (allocates draw if ws_whCount > 1)
					qLossNoRL,		// non-recirc loss, Btu/WH-tick
					qLossRL,		// recirc loop loss, Btu/WH-tick
					volRL);			// recirc loop flow, gal/WH-tick
			else if (pWH->wh_IsInstUEFModel())
				rc |= pWH->wh_InstUEFDoSubhr(
					ticksSh,
					scaleWH,
					qLoss);
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
	if (ws_whCount > 0.f && ivl == C_IVLCH_Y)
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
RC DHWDAYUSE::wdu_Init(	// one-time inits
	int pass)		// 0 or 1
// Does 2 things
// - finds beg/end ss of child DHWUSEs
//     (re faster looping in e.g. wdu_DoHour())
// - assigns draw sequence #s to child DHWUSEs
//     done for all hwEndUses
//     only 2-2019 use = random assignment of
//       C_DHWEUCH_SHOWER draws to DHWHEATRECs
// done once at run start
{
	RC rc = RCOK;


	DHWUSE* pWU;
	if (pass == 0)
	{
		wdu_wuSsBeg = 9999;		// WuR ss range
		wdu_wuSsEnd = 0;
		wdu_wuCount = 0;		// # of child DHWUSEs

		RLUPC(WuR, pWU, pWU->ownTi == ss)
		{	// DHWUSE subscript range within this DHWDAYUSE
			if (pWU->ss < wdu_wuSsBeg)
				wdu_wuSsBeg = pWU->ss;
			if (pWU->ss >= wdu_wuSsEnd)
				wdu_wuSsEnd = pWU->ss + 1;
			wdu_wuCount++;
		}
		return rc;
	}

	// pass == 1

#if 0
	// local structure re assignment of draw sequence #s
	int eventIDmax[NDHWENDUSES];
	static int drawSeqNNext[NDHWENDUSES] = { 0 };

	VSet(eventIDmax, NDHWENDUSES, -1);

	for (int iWU = wdu_wuSsBeg; iWU < wdu_wuSsEnd; iWU++)
	{
		pWU = WuR.GetAt(iWU);
		if (!pWU->gud || pWU->ownTi != ss)
			continue;
		// draw sequence numbers
		if (pWU->wu_eventID > eventIDmax[pWU->wu_hwEndUse])
		{	// as yet unseen eventID
			eventIDmax[ pWU->wu_hwEndUse] = pWU->wu_eventID;
			pWU->wu_drawSeqN = drawSeqNNext[pWU->wu_hwEndUse]++;
		}
		else
		{	// DHWUSE may be part of previously seen draw
			// search backwards for matching eventID
			int iWU;
			for (iWU = pWU->ss - 1; iWU > 0; iWU--)
			{
				const DHWUSE* pWUX = (const DHWUSE*)pWU->b->GetAtSafe(iWU);
				if (pWUX && pWUX->gud && pWUX->ownTi == ss
					&& pWUX->wu_hwEndUse == pWU->wu_hwEndUse
					&& pWUX->wu_eventID == pWU->wu_eventID)
				{
					pWU->wu_drawSeqN = pWUX->wu_drawSeqN;	// part of previous event, use same seq #
					break;
				}
			}
			if (iWU == 0)
				// unexpected (could happen for if eventID skipped)
				pWU->wu_drawSeqN = drawSeqNNext[pWU->wu_hwEndUse]++;
		}
	}
#else
	// local structure re assignment of draw sequence #s
	int eventIDmax[NDHWENDUSES];
	int drawSeqNNext[NDHWENDUSES] = { 0 };

	VSet(eventIDmax, NDHWENDUSES, -1);

	for (int iWU = wdu_wuSsBeg; iWU < wdu_wuSsEnd; iWU++)
	{	pWU = WuR.GetAt(iWU);
		if (!pWU->gud || pWU->ownTi != ss)
			continue;
		// draw sequence numbers
		if (pWU->wu_eventID > eventIDmax[pWU->wu_hwEndUse])
		{	// as yet unseen eventID
			eventIDmax[pWU->wu_hwEndUse] = pWU->wu_eventID;
			pWU->wu_drawSeqN = drawSeqNNext[pWU->wu_hwEndUse]++;
		}
		else
		{	// DHWUSE may be part of previously seen draw
			// search backwards for matching eventID
			int iWU;
			for (iWU = pWU->ss - 1; iWU > 0; iWU--)
			{	const DHWUSE* pWUX = (const DHWUSE*)pWU->b->GetAtSafe(iWU);
				if (pWUX && pWUX->gud && pWUX->ownTi == ss
				 && pWUX->wu_hwEndUse == pWU->wu_hwEndUse
				 && pWUX->wu_eventID == pWU->wu_eventID)
				{	pWU->wu_drawSeqN = pWUX->wu_drawSeqN;	// part of previous event, use same seq #
					break;
				}
			}
			if (iWU == 0)
				// unexpected (could happen for if eventID skipped)
				pWU->wu_drawSeqN = drawSeqNNext[pWU->wu_hwEndUse]++;
		}
	}
#endif
	return rc;

}		// DHWDAYUSE::wdu_Init
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
		for (int iWU = wdu_wuSsBeg; iWU < wdu_wuSsEnd; iWU++)
		{
			pWU = WuR.GetAt(iWU);
			if (pWU->gud && pWU->ownTi == ss)
				rc |= pWU->wu_DoHour(pWS, wdu_mult, Top.iHr);
		}
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
static const double minPerDay = double( 24*60);
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

	// derive adjusted duration
	//   warmup losses are represented by extending draw
	float durX = wu_dur * pWS->ws_whDrawDurF[ wu_hwEndUse];
	if (durX > minPerDay)
	{	// duration longer than 1 day
		// warn and limit
		rc |= orMsg( WRN, "adjusted draw duration %0.1f min changed to maximum allowed 1440 min (24 hr)",
			durX);
		durX = minPerDay;
	}

	double begM = wu_start * 60.;	// beg time, min of day
	roundNearest( begM, .05*Top.tp_subhrTickDur);	// round to avoid tiny amounts
													//   in adjacent bins
	double begMHot = begM + max(durX - wu_dur, 0.);	// beg of hot at fixture
													//   re warmup waste
	double endM = begM + durX;	// end time, min of day
	if (endM > minPerDay)
	{	// period wraps over midnight
		//   treat as 2 non-wrapping segments
		rc |= wu_DoHour1( pWS, mult, iH, begM, min( begMHot, minPerDay), minPerDay);
		rc |= wu_DoHour1( pWS, mult, iH, 0., max( 0., begMHot-minPerDay), endM-minPerDay);
	}
	else
		rc |= wu_DoHour1( pWS, mult, iH, begM, begMHot, endM);

	// if (any on the list)

	return rc;
}	// DHWUSE::wu_DoHour
//-----------------------------------------------------------------------------
RC DHWUSE::wu_DoHour1(		// low-level accum to tick-level bins
	DHWSYS* pWS,	// parent DHWSYS (accum info here)
	float mult,		// multiplier applied to wu_flow
	int iH,			// hour of day (0 - 23)
	double begM,	// draw beg time, min of day
	double begMHot,	// draw hot water beg time, min of day
					//	 after warmup delay, begM<=begMHot<=endM
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
	if (endX <= begX || wu_flow < 1.e-6)
		return RCOK;		// no overlap with hour or no flow

	// some use in current hour
	// compute actual flow re e.g. mixing
	double tickDur = Top.tp_subhrTickDur;	// tick duration, min
	double fxFlow = wu_flow * mult;			// total (mixed) flow at fixture, gpm
											//   (with multiplier)
	double fxVol = fxFlow * (endX - begX);	// total vol at fixture, gal
	int iTk0 = begX / tickDur;			// draw's first tick idx
	double tickBeg = iTk0*tickDur;		// start time of 1st tick
	double tickEnd;

	// handle DHWHEATREC draws separately
	int iTk;
	int iFx = pWS->ws_AssignDHWUSEtoFX(this);
	if (iFx >= 0)
	{	DHWFX& fx = pWS->ws_fxList[iFx];
		fx.fx_hitCount++;
		DHWHEATREC* pWR = WrR.GetAtSafe( fx.fx_drainCnx);
		if (pWR)
		{	// draw is for fixture draining via DHWHEATREC
			// cannot model until all simultaneous draws are known
			// save draw info for ws_DoHourDWHR
			int coldCnx = fx.fx_coldCnx;		// source of cold-side water for this draw
												//    0=mains, 1=DHWHEATREC
			pWS->ws_iTk0DWHR = min(pWS->ws_iTk0DWHR, iTk0);
			double begHotX = max(begMHot - iH * 60, 0.);
			int iTk0Hot = begHotX / tickDur;
			for (iTk = iTk0; tickBeg < endX; iTk++)
			{
				tickEnd = (iTk + 1)*tickDur;
				double endXTick = min(tickEnd, endX);
				// use in this tick (gal) = flow (gpm) * overlap duration (min)
				float fxMixTick = fxFlow * (endXTick - max(tickBeg, begX));
				float fxHotTick = iTk > iTk0Hot  ? fxMixTick
					            : iTk == iTk0Hot ? fxFlow * (endXTick - begHotX)
					            :                  0.f;
				pWR->wr_ticks[iTk].wrtk_draws.push_back(DWHRUSE(iFx, coldCnx, fxMixTick, fxHotTick, wu_temp));
				pWS->ws_ticks[iTk].wtk_nHRDraws++;
				tickBeg = tickEnd;
			}
			pWS->ws_iTkNDWHR = max(iTk, pWS->ws_iTkNDWHR);
			return rc;
		}
		// else fall through to non-DHWHEATREC case
	}
	
	float hotF;		// hot water fraction
	float hotFNoHR;	// hot water fraction w/o heat recovery
	if (!IsSet( DHWUSE_TEMP))
		hotFNoHR = hotF = wu_hotF;		// no mixing, use input value
	else
	{	// use temperature is specified
		// const DHWSYS* pWS = wu_GetDHWSYS();
		float tCold = pWS->ws_tInlet;	// cold water temp at fixture, F
										//   (*not* ws_tInletX: mix is with mains water)
		float tHot = pWS->ws_tUse;		// hot water temp at fixture, F
										//   assume system tuse
		rc |= wu_CalcHotF( tHot, tCold, hotFNoHR);

		if (wu_heatRecEF < 0.001f)
			hotF = hotFNoHR;		// no heat recovery
		else
		{	// local legacy-model heat recovery available and legal
			if (wu_heatRecEF > 0.9f)
			{	// warn and limit
				rc |= orMsg( WRN, "wuHeatRecEF=%0.2f not in valid range 0 - 0.90",
							wu_heatRecEF);
				wu_heatRecEF = 0.9f;
			}
			// assume drain temp = use temp
			float deltaT = wu_heatRecEF * (wu_temp - tCold);
			tCold += deltaT;
			rc |= wu_CalcHotF( tHot, tCold, hotF);	// hotF with heat rec
			pWS->ws_qDWHR += (1.f - hotF) * fxVol * deltaT * waterRhoCp;
		}

	}

	// hot water use assuming no heat recovery, gal
	pWS->ws_whUseNoHR += hotFNoHR * fxVol;

	// allocate to tick bins
	// accumulate total uses by end use
	for (iTk=iTk0; tickBeg < endX; iTk++)
	{	tickEnd = (iTk+1)*tickDur;
		// use in this bin (gal) = flow (gpm) * overlap duration (min)
		double fxMixTick = fxFlow * (min( tickEnd, endX) - max( tickBeg, begX));
		// note: fxMixTick rarely 0 due to tests above, not worth testing
		pWS->ws_AccumUseTick( wu_hwEndUse, iTk, fxMixTick, fxMixTick*hotF);
		tickBeg = tickEnd;
	}
	return rc;
}	// DHWUSE::wu_DoHour1
//-----------------------------------------------------------------------------
void DHWSYS::ws_AccumUseTick(		// tick-level water use accounting
	DHWEUCH hwEndUse,	// hot water end use for draw
	int iTk,			// current tick idx (within hour)
	double fxUseMix,	// fixture mixed use, gal
	double whUse)		// hot water use, gal
{
	ws_ticks[ iTk].wtk_whUse += whUse;	// tick hot use
	ws_whUse.wmt_AccumEU( hwEndUse, whUse);		// end-use accounting
	ws_fxUseMix.wmt_AccumEU( hwEndUse, fxUseMix);
}		// DHWSYS::ws_AccumUseTick
//-----------------------------------------------------------------------------
RC DHWUSE::wu_CalcHotF(
	float tHot,		// hot water temp at fixture, F
	float tCold,	// cold water temp at fixture, F
	float& hotF) const	// returned: hot water fraction (0 - 1)
// returns hot water fraction that delivers mixed water temp wu_temp
//         errors are msg'd, fHot returned 0 or 1
{
	int mixRet = DHWMix( wu_temp, tHot, tCold, hotF);
	RC rc = RCOK;
	if (mixRet)
	{	if (mixRet == -2)
			rc |= orMsg( WRN, "Cold water temp (%0.1f F) >= hot water temp (%0.1f F).  "
					"Cannot mix to wuTemp (%0.1f F).",
					tCold, tHot, wu_temp);
		else if (mixRet == -1)
			rc |= orMsg( WRN, "wuTemp (%0.1f F) < cold water temp (%0.1f F).  "
					"Cannot mix to wuTemp.", wu_temp, tCold);
		else if (mixRet == 1)
			rc |= orMsg( WRN, "wuTemp (%0.1f F) > hot water temp (%0.1f F).  "
					"Cannot mix to wuTemp.", wu_temp, tHot);
	}
	return rc;
}		// DHWUSE::wu_CalcHotF
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
	{	ignoreN( whenHs, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, 0);
		if (wh_heatSrc != C_WHHEATSRCCH_ASHPX)
			ignore( DHWHEATER_UAMULT, whenHs);
	}

	if (wh_type != C_WHTYPECH_STRGSML)
	{	if (wh_type == C_WHTYPECH_INSTUEF)
		{	if (IsSet( DHWHEATER_HEATSRC) && wh_heatSrc != C_WHHEATSRCCH_FUEL)
				rc |= ooer( DHWHEATER_HEATSRC,
					"whHeatSrc=%s is not allowed %s (use whHeatSrc=Fuel or omit)",
					whHsTx, whenTy);
			wh_heatSrc = C_WHHEATSRCCH_FUEL;
			rc |= requireN( whenTy, DHWHEATER_UEF, DHWHEATER_RATEDFLOW, DHWHEATER_ANNUALFUEL,
					DHWHEATER_ANNUALELEC, DHWHEATER_EFF, 0);
			ignoreN( whenTy, DHWHEATER_EF, 0);
			if (wh_UEF > wh_eff)
				rc |= oer( "whEff (%0.3f) must be >= whUEF (%0.3f)",
							wh_eff, wh_UEF);
			// note wh_vol check below (wh_vol=0 OK, else error)
		}
		else if (wh_heatSrc == C_WHHEATSRCCH_ASHP
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
//-----------------------------------------------------------------------------
RC DHWHEATER::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	DHWSYS* pWS = wh_GetDHWSYS();
	pWS->ws_whCount += wh_mult;		// total # of water heaters in DHWSYS
	return rc;
}	// DHWHEATER::RunDup
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
	wh_stbyTicks = 0;

	DHWSYS* pWS = wh_GetDHWSYS();

	// default meters from parent system
	if (!IsSet( DHWHEATER_ELECMTRI))
		wh_elecMtri = pWS->ws_elecMtri;
	if (!IsSet( DHWHEATER_FUELMTRI))
		wh_fuelMtri = pWS->ws_fuelMtri;

	wh_pMtrElec = MtrB.GetAtSafe( wh_elecMtri);		// elec mtr or NULL
	wh_pMtrFuel = MtrB.GetAtSafe( wh_fuelMtri);		// fuel mtr or NULL

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

// UEF-based instantaneous water heater model 5-2017
	if (wh_type == C_WHTYPECH_INSTUEF)
		rc |= wh_InstUEFInit();

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
	float wsMult)	// system multiplier
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
		else if (wh_IsInstUEFModel())
			;	// nothing required
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
	float mult = wh_mult * wsMult;	// overall multiplier = system * heater
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
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR2250 ? HPWH::MODELS_RheemHBDR2250
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR4550 ? HPWH::MODELS_RheemHBDR4550
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR2265 ? HPWH::MODELS_RheemHBDR2265
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR4565 ? HPWH::MODELS_RheemHBDR4565
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR2280 ? HPWH::MODELS_RheemHBDR2280
			: wh_ashpTy == C_WHASHPTYCH_RHEEMHBDR4580 ? HPWH::MODELS_RheemHBDR4580
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
		if (wh_UAMult != 1.f)
		{	// apply user-specified adjustment to implicit UA
			//    approximates effect of e.g. tank wrap insulation
			double UAdflt;
			int ret = wh_pHPWH->getUA( UAdflt);
			if (ret || wh_pHPWH->setUA( UAdflt*wh_UAMult) != 0)
				rc = RCBAD;
		}
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

	return rc;

}	// DHWHEATER::wh_HPWHDoHour
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoSubhr(		// HPWH subhour
	const DHWTICK* ticksSh,		// tick info for subhour (draw, cold water temp, )
	double scale,		// draw scale factor
						//   re DHWSYSs with >1 DHWHEATER
						//   *not* including wh_mixDownF;
	double qLossDraw,	// additional losses for DHWSYS, Btu/WH-tick
						//   modeled as pseudo-draw
						//   note: scale applied by caller
	double qLossRL,		// recirc loop loss, Btu/WH-tick
	double volRL)		// recirc loop flow gal/WH-tick
						//   modeled as flow with return to inlet
// calls HPWH tp_nSubhrTicks times, totals results
// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	float mult = pWS->ws_mult * wh_mult;	// overall multiplier

	// local totals
	double qEnv = 0.;		// heat removed from environment, kWh
							//   + = to water heater
	double qLoss = 0.;		// standby losses, kWh
							//   + = to surround

	double qHW = 0.;		// total hot water heating, kWh
							//   always >= 0
							//   includes loopFlow heating
							//   does not include wh_HPWHxBU

	double tHWOutF = 0.;	// accum re average hot water outlet temp, F

	wh_HPWHUse[ 0] = wh_HPWHUse[ 1] = 0.;	// energy use totals, kWh
	wh_HPWHxBU = 0.;	// add'l resistance backup this subhour, Btu
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
	// tank heat content at start
	double qHCStart = KJ_TO_KWH( wh_pHPWH->getTankHeatContent_kJ());
#endif

#define HPWH_DUMP		// define to include debug CSV file
#if defined( HPWH_DUMP)
// #define HPWH_DUMPSMALL	// #define to use abbreviated version
	int bWriteCSV = DbDo( dbdHPWH);
#endif
	// pseudo-draw (gal) to represent e.g. central system branch losses
	//  ?tInlet?
	double lossDraw = qLossDraw / (waterRhoCp * max( 1., wh_tHWOut - pWS->ws_tInlet));

	// recirc loop return temp
	//   Note: DHWLOOP losses calc'd with independent temperature assumptions (!)
	//   Derive loop return temp using heat loss ignoring DHWLOOP temps
	//     i.e. assume loop entering temp = ws_tUse
	//       tRetRL = tUse - qLossRL/(waterRhoCp * volRL)
	//     vol*temp product: tRetRL * volRL = tUse*volRL - qLossRL/waterRhoCp
	double tVolRL = volRL > 0. && qLossRL > 0.
				? pWS->ws_tUse * volRL - qLossRL / waterRhoCp
		        : 0.;		// skip tInlet mix below

	int nTickNZDraw = 0;		// count of ticks with draw > 0
	for (int iTk=0; !rc && iTk<Top.tp_nSubhrTicks; iTk++)
	{
#if 0 && defined( _DEBUG)
		if (Top.tp_date.month == 7
		  && Top.tp_date.mday == 27
		  && Top.iHr == 10
		  && Top.iSubhr == 3)
			wh_pHPWH->setVerbosity( HPWH::VRB_emetic);
#endif
		double drawForTick = ticksSh[ iTk].wtk_whUse*scale*wh_mixDownF + lossDraw;
		double tInlet = ticksSh[ iTk].wtk_tInletX;
		if (tVolRL > 0.)		// if loop flow
		{	// ?tInlet?
			tInlet = (tVolRL + drawForTick*tInlet)
						/ (volRL+drawForTick);
			drawForTick += volRL;
		}
		wh_pHPWH->setInletT( DegFtoC( tInlet));		// mixed inlet temp

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
		double tOut = wh_pHPWH->getOutletTemp();
		float HPWHxBU = 0.f;		// add'l resistance backup, this tick, Btu
		if (tOut)
		{	double tOutF = DegCtoF( tOut);	// output temp, F
			if (tOutF < pWS->ws_tUse)
			{	// load not met, add additional (unlimited) resistance heat
				wh_mixDownF = 1.f;
				HPWHxBU = waterRhoCp * drawForTick * (pWS->ws_tUse - tOutF);
				wh_HPWHxBU += HPWHxBU;
				tOutF = pWS->ws_tUse;
			}
			else
				// mix mains water (not tInletX) to obtain ws_tUse
				//   set wh_mixDownF for next tick
				DHWMix( pWS->ws_tUse, tOutF, pWS->ws_tInlet, wh_mixDownF);
			tHWOutF += tOutF;	// note tOutF may have changed (but not tOut)
			nTickNZDraw++;		// this tick has draw
			wh_stbyTicks = 0;	// reset standby duration
			qHW += KJ_TO_KWH(
				     GAL_TO_L( drawForTick)
				   * HPWH::DENSITYWATER_kgperL
				   * HPWH::CPWATER_kJperkgC
				   * (tOut - DegFtoC( tInlet)));
		}
		else
			wh_stbyTicks++;		// no draw: accum duration
								//   (info only)

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
			{
				// dump file name = <cseFile>_hpwh.csv
				const char* fName =
					strsave( strffix2( strtprintf( "%s_hpwh", InputFilePathNoExt), ".csv", 1));
				pF = fopen( fName, "wt");
				if (!pF)
					err( PWRN, "HPWH report failure for '%s'", fName);
				else
				{	// headings
					fprintf( pF, "%s,%s,%s\n",wh_desc, Top.repHdrL,Top.runDateTime);
					fprintf( pF, "%s%s %s %s HPWH %s\n",
						Top.tp_RepTestPfx(), ProgName, ProgVersion, ProgVariant,
						Top.tp_HPWHVersion);
#if defined( HPWH_DUMPSMALL)
					fprintf( pF, "minYear,draw( L)\n");
#else
					wh_pHPWH->WriteCSVHeading( pF, "month,day,hr,min,minDay,"
						            "tOut (C),tEnv (C),tSrcAir (C),tInlet (C),tSetpoint (C),"
						            "lossDraw (gal),RLDraw (gal),totDraw (gal),totDraw (L),tOut (C),XBU (Wh),");
#endif
				}
			}
			if (pF)
			{	double minHr = double( Top.iSubhr)*Top.subhrDur*60. + iTk*Top.tp_subhrTickDur + 1;
				double minDay = double( 60*Top.iHr) + minHr;
				// double minYear = double( (int(Top.jDayST-1)*24+Top.iHrST)*60) + minHr;
#if defined( HPWH_DUMPSMALL)
				fprintf( pF, "%0.2f,%0.3f\n", minYear, GAL_TO_L( drawForTick));
#else
				wh_pHPWH->WriteCSVRow( pF, strtprintf(
						"%d,%d,%d,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f, %0.3f,%0.3f,%0.3f,%0.3f,%0.2f,%0.2f,",
						Top.tp_date.month, Top.tp_date.mday, Top.iHr+1, minHr, minDay,
						DegFtoC( Top.tDbOSh),DegFtoC( wh_tEx),DegFtoC( wh_ashpTSrc),
						DegFtoC( tInlet), DegFtoC( pWS->ws_tSetpoint),
					    lossDraw, volRL, drawForTick, GAL_TO_L( drawForTick),tOut,HPWHxBU/BtuperWh));
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
	double deltaHC = KJ_TO_KWH( wh_pHPWH->getTankHeatContent_kJ()) - qHCStart;
	double qBal = qEnv								// HP energy extracted from surround
		        - qLoss								// tank loss
				+ wh_HPWHUse[ 0] + wh_HPWHUse[ 1]	// electricity in
				- qHW								// hot water energy
				- deltaHC;							// change in tank stored energy
	if (fabs( qBal)/max( qHCStart, 1.) > .002)		// added qHCStart normalization, 12-18
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
	wh_AccumElec( mult,
		wh_HPWHUse[ 1] * BtuperkWh + wh_parElec * Top.subhrDur * BtuperWh,
	    wh_HPWHUse[ 0] * BtuperkWh);
	return rc;
}		// DHWHEATER::wh_HPWHDoSubhr
//-----------------------------------------------------------------------------
void DHWHEATER::wh_AccumElec(		// electricity use accounting / meter accum
	float mult,			// overall multiplier (generally ws_mult*wh_mult)
	double inElec,		// substep primary electricity, kWh
	double inElecBU)	// substep backup electricity, kWh (not including wh_HPWHxBU)
// wh_HPWHxBU also used
{
	wh_inElec += inElec;		// hour totals
	wh_inElecBU += inElecBU + wh_HPWHxBU;
	if (wh_pMtrElec)
	{	wh_pMtrElec->H.dhw += mult * inElec;
		if (wh_xBUEndUse)
		{	wh_pMtrElec->H.dhwBU += mult*inElecBU;
			wh_pMtrElec->H.mtr_Accum( wh_xBUEndUse, mult*wh_HPWHxBU);
		}
		else
			wh_pMtrElec->H.dhwBU += mult * (inElecBU + wh_HPWHxBU);
	}
}		// DHWHEATER::wh_AccumElec
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_InstUEFInit()		// one-time setup for UEF-based instantaneous model
// returns RCOK on success
//    else other -- do not run
{
// parameters used in deriving runtime coefficients
// from Tankless WH consumption calculations for ACM.18.doc
//   J. Lutz, 28 July 2017
struct UEFPARAMS
{	float flowMax;  float flowRE;
	float A; float B; float qOutUEF; float qOutRE; float durRE; float honUEF;
};
static const UEFPARAMS UEFParams[] = {
// size	       flowMax  flowRE	      A         B      qOutUEF    qOutRE   durRE  honUEF
{ /*very small*/  0.f,	  1.f,    10.f,	    3.62150f,   5452.f,	1090.f,	 2.000f, 0.1667f },
{ /*low*/		 1.7f,	 1.7f,    22.3529f,	5.78215f,  20718.f,	8178.f,	 8.824f, 0.4275f },
{ /*medium*/	 2.8f,	 1.7f,    32.3529f,	6.30610f,  29987.f,	8178.f,	 8.824f, 0.5941f },
{ /*high*/	      4.f,	  3.f,    28.00f,   6.42050f,  45798.f, 14721.f, 9.000f, 0.6542f },
{ /*terminator*/  9999999.f }};

	RC rc = RCOK;

	// determine size bin
	int iSize;
	for (iSize=0; iSize < 4; iSize++)
	{	if (UEFParams[ iSize+1].flowMax > wh_ratedFlow)
			break;
	}
	const UEFPARAMS& UP = UEFParams[ iSize];

	// fraction of energy use that is fuel
	double F;
	if (wh_annualFuel < .000001f)
		F = 1.;		// nothing known about electrical use
	else
	{	F = 0.99;
		if (wh_annualFuel > 0.f)	// insurance
			F = 1./(1. + (wh_annualElec/wh_annualFuel)*3412./100000.);
	}

	// electrical power during operation (recovery), W
	double Pe = (wh_annualElec * 1000./365. - wh_stbyElec * (24.-UP.honUEF))
				/ UP.honUEF;
	if (Pe < 0)
		rc |= oer( "Inconsistent input for whAnnualElec (=%.2f kWh) and whStbyElec (=%.2f W).\n"
			       "    Derived operating electrical power is < 0.",
			       wh_annualElec, wh_stbyElec);

	float P = (UP.qOutUEF/wh_UEF - UP.B*UP.qOutRE/wh_eff) / (UP.A - UP.B*UP.durRE);
	float Lcyc = UP.qOutRE/wh_eff - P*UP.durRE;

	float Pf = F * P;			// fuel at flow=flowRE and deltaT=67, Btu/min
	wh_cycLossFuel = F * Lcyc;	// startup fuel, Btu/cycle

	// max flow per tick, gal-F/tick
	// Top.tp_subhrTickDur = tick duration, min
	wh_maxFlowX = wh_ratedFlow * Top.tp_subhrTickDur * 67.f;

	// maximum load carry forward, Btu
	// user input wh_loadCFwdF = multiplier for rated capacity
	//   (approximately allowed catch-up time, hr)
	wh_loadCFwdMax = wh_loadCFwdF * wh_ratedFlow * waterRhoCp * 67. * 60.;

	// fuel input: Btu/tick at flow=maxFlow and deltaT=67
	wh_maxInpX = Pf				// Btu/min at flow=flowRE and deltaT=67
		       * (wh_ratedFlow / UP.flowRE)	// scale to max flow
		       * Top.tp_subhrTickDur;		// scale to actual tick duration

	// no electricity use pending model development
	wh_operElec = Pe * BtuperWh;	// electrical power during opration, Btuh
	// wh_cycLossElec = 0.f;		// electricity use per start, Btu
									//    unused in revised model 5-24-2017

	return rc;
}		// DHWHEATER::wh_InstUEFInit
//----------------------------------------------------------------------------
RC DHWHEATER::wh_InstUEFDoSubhr(	// subhour simulation of instantaneous water heater
	const DHWTICK* ticksSh,	// subhour tick info (draw, cold water temp, ...)
	double scale,		// draw scale factor
						//   re DHWSYSs with >1 DHWHEATER
	double xLoss)		// additional losses for DHWSYS, Btu/tick
						//   re e.g. central loop losses
						//   note: scale applied by caller
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();

	// carry-forward: instantaneous heaters throttle flow to maintain temp.
	//   here represented by carrying forward a limited amount of unmet load
	wh_HPWHxBU = 0.;			// heat in excess of capacity (after carry-forward)
								//   provided by virtual resistance heat
								//   prevents gaming compliance via undersizing
	int nTickNZDraw = 0;		// count of ticks with draw > 0
	double nTickFullLoad = 0.;	// fractional ticks of equiv full load
	double nColdStarts = 0.;	// # of cold startups
	for (int iTk=0; !rc && iTk<Top.tp_nSubhrTicks; iTk++)
	{	float deltaT = max( 1., pWS->ws_tUse - ticksSh[ iTk].wtk_tInletX);
										// temp rise, F max( 1, dT) to prevent x/0
		double qPerGal = waterRhoCp * deltaT;	
		double drawForTick =
				ticksSh[ iTk].wtk_whUse*scale
			  + (xLoss + wh_loadCFwd) / qPerGal;	// xLoss and carry-forward
		wh_loadCFwd= 0.;	// clear carry-forward, if not met, reset just below
		if (drawForTick > 0.)
		{	nTickNZDraw++;
			double drawFullLoad = wh_maxFlowX / deltaT;	// max vol that can be heated in this tick
			if (drawForTick > drawFullLoad)
			{	nTickFullLoad += 1.;					// full capacity operation
				wh_loadCFwd = qPerGal * (drawForTick - drawFullLoad);	// unmet load
				if (wh_loadCFwd > wh_loadCFwdMax)
				{	wh_HPWHxBU += wh_loadCFwd - wh_loadCFwdMax;		// excess heating required to meet ws_tUse
					wh_loadCFwd = wh_loadCFwdMax;
				}
			}
			else
				nTickFullLoad += drawForTick / drawFullLoad;
			if (wh_stbyTicks)	// if no draw in prior tick
			{	double offMins = wh_stbyTicks * Top.tp_subhrTickDur;
				// exponential cooldown
				static const double TC = 50.06027;		// cooldown time constant
				double r = offMins / TC;
				nColdStarts += r > 10. ? 1. : 1.-exp(-r);		// avoid underflow
				// printf( "\n%0.2f  %0.4f", offMins, nColdStarts);
				wh_stbyTicks = 0;
			}
		}
		else
			// standby
			wh_stbyTicks++;		// count conseq ticks w/o draw
	}

	double tickDurHr = Top.tp_subhrTickDur / 60.;	// tick duration, hr
	double rcovFuel = wh_maxInpX * nTickFullLoad;
	double startFuel = wh_cycLossFuel * nColdStarts;
	double rcovElec = wh_operElec * nTickNZDraw * tickDurHr;	// assume operation for entire tick with any draw
	// double startElec = wh_cycLossElec * nTickStart;	// unused in revised model
	// standby in ticks w/o draw
	double stbyElec = wh_stbyElec * (Top.tp_nSubhrTicks-nTickNZDraw) * tickDurHr;

	// energy use accounting, Btu
	float mult = pWS->ws_mult * wh_mult;	// overall multiplier
	double inFuel = rcovFuel + startFuel;
	wh_inFuel += inFuel;
	if (wh_pMtrFuel)
		wh_pMtrFuel->H.dhw += mult * inFuel;

	wh_AccumElec( mult,
		rcovElec /*startElec*/ + (stbyElec + wh_parElec * Top.subhrDur) * BtuperWh,
		0.);	// wh_HPWHxBU is only backup

	return rc;
}			// DHWHEATER::whInstUEFDoSubhr
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
	pWS->ws_wtCount += wt_mult;		// count total # of tanks in system

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
// DHWHEATREC
///////////////////////////////////////////////////////////////////////////////
DHWHEATREC::~DHWHEATREC()		// d'tor
{
	delete[] wr_ticks;
	wr_ticks = NULL;
}	// DHWHEATREC::~DHWHEATREC
//-----------------------------------------------------------------------------
DHWSYS* DHWHEATREC::wr_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WrR ? WsR : WSiB).GetAtSafe( ownTi);
	return pWS;
}		// DHWHEATREC::wt_GetDHWSYS
//----------------------------------------------------------------------------
RC DHWHEATREC::wr_CkF()		// DHW heat rec input check / default
// called at end of each DHWHEATREC input
{
	RC rc = RCOK;
	DHWSYS* pWS = wr_GetDHWSYS();
	if (!pWS)
		rc |= oer( "DHWSYS not found");	// insurance (unexpected)
	// else
	//	pWS->ws_CheckSubObject( this);

	if (wr_hwEndUse != C_DHWEUCH_SHOWER)
		rc |= ooer( DHWHEATREC_HWENDUSE, "wrHWEndUse=%s not supported (must be Shower)",
			getChoiTx( DHWHEATREC_HWENDUSE));

	if (wr_nFXDrain <= 0)
	{	// no drain connections -- treat as not present
		wr_nFXCold = 0;
		wr_feedsWH = C_NOYESCH_NO;
	}

	if (!IsSet(DHWHEATREC_NFXCOLD))
		wr_nFXCold = wr_nFXDrain;
	else if (wr_nFXCold > wr_nFXDrain)
		rc |= oer("Invalid configuration: wrCountFXCold (%d) must be <= wrCountFXDrain (%d)",
			wr_nFXCold, wr_nFXDrain);

	if (wr_nFXDrain > 0 && !wr_FeedsWH() && !wr_FeedsFX())
		rc |= oer("Potable-side outlet not connected -- cannot model\n"
			"    (wrFeedsWH=NO and wrCountFXCold = 0)");

	return rc;
}	// DHWHEATREC::wr_CkF
//----------------------------------------------------------------------------
RC DHWHEATREC::wr_Init()		// runtime init
{	RC rc = RCOK;
	delete[] wr_ticks;
	wr_ticks = new DHWHRTICK[Top.tp_NHrTicks()];
	return rc;
}		// DHWHEATREC::wr_Init
//-----------------------------------------------------------------------------
void DHWHEATREC::wr_InitTicks()		// init for hour
{
	int nTk = Top.tp_NHrTicks();
	for (int iTk = 0; iTk < nTk; iTk++)
		wr_ticks[iTk].wrtk_Init();
}	// DHWHEATREC::wr_InitTicks
//----------------------------------------------------------------------------
#if 0
0 out of service, not maintained 2-2019
0 int DHWHEATREC::wr_IsEquiv(
0	const DHWHEATREC& wr) const
0 // returns 1 iff *this and wr are equivalent (model only once)
0 {
0 	int bEquiv = wr_nEqual == wr.wr_nEqual
0		&& wr_nUneqFX == wr.wr_nUneqFX
0		&& wr_nUneqWH == wr.wr_nUneqWH
0	    && wr_type == wr.wr_type
0		&& wr_hwEndUse == wr.wr_hwEndUse	// future proof
0		&& !ISNANDLE(wr_effRated) && !ISNANDLE(wr.wr_effRated)
0		&& wr_effRated == wr.wr_effRated
0		&& !ISNANDLE( wr_dTDrain) && !ISNANDLE( wr.wr_dTDrain)
0		&& wr_dTDrain == wr.wr_dTDrain;
0
0	return bEquiv;
0 }		// DHWHEATREC::wr_IsEquiv
#endif
//----------------------------------------------------------------------------
RC DHWHEATREC::wr_SetFXConnections(
	DHWSYS* pWS,		// parent DHWSYS
	int& iFx)
// returns RCOK iff success
{
	RC rc = RCOK;

	// TODO: wr_mult

	// first call: init all to "no DHWHEATREC"
	if (iFx == 0)
		for (int iF2=0; iF2 < pWS->ws_showerCount; iF2++)
			pWS->ws_fxList[iF2].fx_Set(
				C_DHWEUCH_SHOWER,	// end use
				0,			// drain: discard (no DHWHEATREC
				0);			// cold: mains
	
	// note wr_nFXDrain==0 implies implies "does not exist"
	for (int iCx = 0; iCx<wr_nFXDrain; iCx++)
		pWS->ws_fxList[iFx++].fx_Set(
			C_DHWEUCH_SHOWER,		// end use
			ss,						// drain = this DHWHEATREC
			iCx < wr_nFXCold);		// cold: 0 = mains
									//       1 = this DHWHEATREC
	return rc;

}	// DHWHEATREC::wr_SetFXConnections
//----------------------------------------------------------------------------
float DHWHEATREC::wr_CalcTick(		// calculate performance for 1 tick
	DHWSYS* pWS,		// parent DHWSYS
	DHWHRTICK& wrtk,	// current tick info for this DHWHEATREC
	float vOther,		// hot water draws for other fixtures, gal
						//   included in potable flow if feedsWH
	float& whUseNoHR,	// returned updated: hot water use w/o heat recovery, gal
						//   used re energy balance check
	float& fxUseMix,	// returned updated: total mixed water use, gal
	float& qR,			// returned updated: tick total recovered heat, Btu
	float& qRWH)		// returned updated: tick recovered heat added to WH inlet water, Btu

// returns hot water use for served fixtures, gal
//     (not including vOther)
{
	int nD = wrtk.wrtk_draws.size();
	if (nD == 0)
		return 0.f;		// no draws, no effect

#if 0 && defined( _DEBUG)
	// if (Top.jDay == 91 && Top.iHr == 20)
	// 	printf("  Hit\n");
	static int nDMulti = 0;
	if (nD > 1)
	{
		if (nDMulti++ == 0)
			printf("\nMultiple draws: jDay=%d  iH=%d",
				Top.jDay, Top.iHr);
	}
	else
		nDMulti = 0;
#endif

	// tick constants
	float tpI = pWS->ws_tInlet;		// mains temp
	float tHotFX = pWS->ws_tUse;	// hot water temp at fixture

	float vd = 0.f;			// total mixed use at all fixture(s), all draws, gal
							//   = drain volume
	float tdI = 0.f;		// average drain-side entering temp, F
	float vMixHR = 0.f;		// total mixed use at fixtures with cold side
							//    connection to DHWHEATREC, gal
	float vHotFX0= 0.f;		// hot water req'd for fixtures that use
							//    mains water for mixing, gal

	// re parallel potable-side DHWHEATRECs
	//   caller allocates vOther equally to all feedsWH-DHWHEATREC(s) in DHWSYS
	//    >> DHWHEATER inlet flow for other draws assumed to flow equally via parallel paths
	//   this-DHWHEATREC fixture flows are assigned to this-DHWHEATREC potable flow
	//    >> this-DHWHEATER's fixtures DHWHEATER and tempering flows do NOT
	//       contribute to other DHWHEATREC's potable flow that parallel piping
	//       might physically allow.
	// Not consistent but accounts for all flow is is believed to be conservative.

	int iD;
	for (iD = 0; iD<nD; iD++)
	{
		DWHRUSE& hru = wrtk.wrtk_draws[iD];
		vd += hru.wdw_vol;		// total drain-side vol

		// avg drain-side temp: part when hot + part during warmup
		tdI += hru.wdw_volHR*(hru.wdw_temp - wr_tdInDiff)		// hot vol (apply tdInDrain)
			 + (hru.wdw_vol - hru.wdw_volHR) * wr_tdInWarmup;	// warmup vol (no tdInDiff)

		// hot water use at fixture if mixed with mains cold
		float vHotNoHR1
			= hru.wdw_vol * DHWMixF(hru.wdw_temp, tHotFX, tpI);
		whUseNoHR += vHotNoHR1;		// accum to caller's hour total

		if (hru.wdw_coldCnx == 0)
			// fixture cold comes from mains -> hot water vol is known
			//   hot vol flows through feedsWH DHWHEATREC(s)
			vHotFX0 += vHotNoHR1;
		else
			// fixture cold comes from DHWHEATREC
			//   accum mixed vol, compute vHotFX below
			vMixHR += hru.wdw_vol;
	}
	fxUseMix += vd;		// accum to caller's total

	// avg DHWHEATREC drain-side entering temp
	//   constrained by physical limits (ignore possible cooling of potable water)
	tdI = bracket(tpI, tdI/max( vd, .0001f), tHotFX);


	float vp;		// potable-side flow, gal
	float tpO = 0.f;// potable-side outlet temp, F
	float vHotFX;	// fixture hot vol, gal

	if (wr_FeedsFX())
	{	
		// DHWHEATREC feeds fixture(s) and possibly WH
		vp = wr_FeedsWH()		// potable volume
			? vMixHR + vHotFX0 + vOther	//  feeds both
			: vMixHR / 2.f;				//  fixture only: 1st guess
		int iL;
		for (iL = 0; iL<10; iL++)
		{	// cold water temp at wdw_coldCnx fixture(s)
			//   use prior wr_eff on 1st iteration
			float tpOwas = tpO;
			tpO = wr_HX(vp, tpI, vd, tdI);

			vHotFX = 0.f;	// hot water needed
			for (iD = 0; iD < nD; iD++)
			{
				DWHRUSE& hru = wrtk.wrtk_draws[iD];
				if (hru.wdw_coldCnx)
					vHotFX += hru.wdw_vol * DHWMixF(hru.wdw_temp, tHotFX, tpO);
			}
			if (!wr_FeedsWH())
				vp = vMixHR - vHotFX;	// cold side flow

#if 0 && defined( _DEBUG)
			if (iL > 7)
				printf("\nSlow converge  iL=%d  wr_eff=%.5f  tpO=%.2f", iL, wr_eff, tpO);
#endif
			if (fabs(tpO - tpOwas) < .1f)
				break;
			wr_EffAdjusted(vp, tpI, vd, tdI);		// update efficiency
		}
	}
	else
	{	// DWHR feeds WH only -- flows known
		vHotFX = vHotFX0;
		vp = vHotFX + vOther;
		wr_EffAdjusted(vp, tpI, vd, tdI);	// derive wr_eff
		tpO = wr_HX(vp, tpI, vd, tdI);
	}

	float qR1 = vp * waterRhoCp * (tpO - tpI);	// recovered heat
	qR += qR1;
	qRWH += wr_FeedsWH() && vp > 0.f			// recovered heat to WH
		? qR1 * (vHotFX + vOther) / vp
		: 0.f;
	return vHotFX;

}		// DHWHEATREC::wr_CalcTick
//-----------------------------------------------------------------------------
float DHWHEATREC::wr_EffAdjusted(		// derive effectiveness for current conditions
	float vp,	// potable water inlet flow, gal/tick
	float tpI,	// potable water inlet temp, F
	float vd,	// drain water inlet flow, gal/tick
	float tdI)	// drain water inlet temp, F
// sets and returns wr_eff
{
	if (wr_type == C_DWHRTYCH_SETEF)
		wr_eff = wr_effRated;		// use input value (may be expression)
	else if (vp < 1.e-6f || vd < 1.e-6f)
		wr_eff = 0.f;		// no flow
	else
	{	// temperature factor
		double fT = (-3.06e-5*tpI*tpI + 4.96e-3*tpI + 0.281)/0.466;

		// volume factor
		//   assume vp is smaller flow
		//   convert vp to gpm
		double v = vp/Top.tp_subhrTickDur;
		double v2, v3;
		double v4 = v*(v3=v*(v2=v*v));
		double fV = -6.98484455e-4*v4 + 1.28561447e-2*v3
					-7.02399803e-2*v2 + 1.33657748e-2*v + 1.23339312;

		wr_eff = wr_effRated * fT * fV;

		if (vd > vp)
			wr_eff *= 1. + 0.3452 * log( min( 3.f, vd / vp));
	}
	
	return wr_eff = bracket( 0.f, wr_eff, 0.95f);
}	// DHWHEATREC::wr_effAdjusted
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
RC DHWPUMP::wp_DoHour(			// hourly DHWPUMP/DHWLOOPPUMP calcs
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
	{	if (rt == RTDHWLOOPPUMP)
			wp_pMtrElec->H.dhwMFL += wp_inElec * mult * wp_mult;
		else
			wp_pMtrElec->H.dhw += wp_inElec * mult * wp_mult;
	}

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

	if (!IsSet( DHWLOOP_ELECMTRI))
		wl_elecMtri = pWS->ws_elecMtri;

	wl_pMtrElec = MtrB.GetAtSafe( wl_elecMtri);		// elec mtr or NULL

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

	DHWSYS* pWS = wl_GetDHWSYS();

	float tIn = wl_tIn1;
	DHWLOOPSEG* pWG;
	RLUPC( WgR, pWG, pWG->ownTi == ss)
	{	// note: segment chain relies on input order
		rc |= pWG->wg_DoHour( tIn);
		wl_HRLL += pWG->ps_PL;	// flow + noflow loop losses
		wl_HRBL += pWG->wg_BL;	// branch losses
		tIn = pWG->ps_tOut;
	}

	int mult = wsMult * wl_mult;	// overall multiplier for meter accounting
	if (wl_lossMakeupPwr > 0.f && wl_HRLL > 0.f)
	{	float HRLLMakeUp = min( wl_HRLL, wl_lossMakeupPwr * BtuperWh);
		wl_HRLL -= HRLLMakeUp;
		if (wl_pMtrElec)
			wl_pMtrElec->H.dhwMFL += HRLLMakeUp * mult / wl_lossMakeupEff;
	}

	// energy and flow results: for wl_mult DHWLOOPs
	wl_HRLL  *= wl_mult;
	wl_HRBL  *= wl_mult;
	wl_volRL = wl_flow * 60.f * wl_runF * wl_mult;

	if (wl_wlpCount > 0)		// if any loop pumps
	{	DHWLOOPPUMP* pWLP;
		RLUPC( WlpR, pWLP, pWLP->ownTi == ss)
			// calc electric energy use at wl_runF
			// accum to elect mtr with multipliers
			pWLP->wp_DoHour( mult, wl_runF);
	}

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
			ps_fvf += pWS->ws_whUse.total / (pWS->ws_wlCount * 60.f);	// draw, gpm
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
	// ?tInlet?
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
	if (!IsSet( DHWLOOPPUMP_ELECMTRI))
		wp_elecMtri = pWL->wl_elecMtri;

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
