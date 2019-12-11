// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
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
#include "cvpak.h"
#include "srd.h"

// #include <random>

#include "cnguts.h"
#include "exman.h"

#include "hpwh.hh"	// decls/defns for Ecotope heat pump water heater model

const float waterRhoCp = 8.345f;		// water volumetric weight, lb/gal
										//    = volumetric heat capacity, Btu/gal-F


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
		: wdw_iFx( iFx), wdw_coldCnx( coldCnx), wdw_vol( vol), wdw_volHR( volHR), wdw_temp( temp)
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
	float wtk_startMin;		// tick start time (minutes from hour beg)
	double wtk_whUse;		// total tick hot water draw at all water heaters, gal
	float wtk_tInletX;		// post-DWHR cold water temperature for this tick, F
							//   = DHWSYS.ws_tInlet if no DWHR
	int wtk_nHRDraws;		// # of DHWHEATREC draws during this tick
	float wtk_volRL;		// DHWLOOP return flow for this tick, gal
							//   iff loop returns to water heater
	float wtk_tRL;			// DHWLOOP loop return temp, F
	float wtk_qLossNoRL;	// additional non-loop losses (e.g. branch), Btu
	double wtk_volIn;		// total tick inlet vol, gal (not including wtk_volRL)
							//   = non-loop draws reduced per mixdown
							//   = primary heater draw when DHWLOOPHEATER is present

	void wtk_Init( float startMin=0.f, double whUseTick=0., float tInlet=50.f)
	{	wtk_startMin = startMin;
		wtk_nHRDraws = 0;
		wtk_whUse = whUseTick; wtk_tInletX = tInlet;
		wtk_volRL = wtk_tRL = wtk_qLossNoRL = wtk_volIn = 0.f;
	}
	void wtk_Accum( const DHWTICK& s, double mult);
	double wtk_DrawTot(float tOut, float& tInletMix) const;
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

	// TODO: other members?

}		// DHWTICK::wtk_Accum
//-----------------------------------------------------------------------------
double DHWTICK::wtk_DrawTot(		// tick draw (for InstUEF)
	float tOut,				// assumed heater output temp
	float& tInletMix) const	// returned: mixed inlet temp
							//   (combined loop return and inletX)
// returns draw volume for tick, gal
{
	float drawUse = wtk_whUse;
	if (wtk_qLossNoRL != 0.f)
	{	float deltaT = max(1., tOut - wtk_tInletX);
				// temp rise, F max( 1, dT) to prevent x/0
		drawUse += wtk_qLossNoRL / (waterRhoCp * deltaT);
	}

	double drawTot = drawUse + wtk_volRL;
	tInletMix = drawTot <= 0.
		? wtk_tInletX
		: (drawUse * wtk_tInletX + wtk_volRL * wtk_tRL) / drawTot;
	return drawTot;

}		// DHWTICK::wtk_DrawTot
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
				DHWSYS_SSF, DHWSYS_CALCMODE, DHWSYS_TSETPOINT, DHWSYS_TSETPOINTLH,
				DHWSYS_TUSE, DHWSYS_TINLET, DHWSYS_LOADSHAREDHWSYSI, 0);
	}
	else if (IsSet( DHWSYS_LOADSHAREDHWSYSI))
	{	// if DHWSYS shares load, msg disallowed inputs
		rc = disallowN( "when wsLoadShareDHWSYS is given",
				DHWSYS_CENTRALDHWSYSI, DHWSYS_DAYUSENAME, DHWSYS_HWUSE, 0);
	}

	// ws_tSetpoint defaults to tUse, handled during simulation
	//  due to interaction with fixed setpoints in some HPWH models

	rc |= ws_CheckVals( ERR);

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

	if (Top.isWarmup)
		erOp |= IGNX;	// limit-only during warmup

	rc |= limitCheckFix(DHWSYS_SSF, 0.f, .99f, erOp);
	rc |= limitCheckFix(DHWSYS_TUSE, 33.f, 211.f, erOp);
	rc |= limitCheckFix(DHWSYS_TSETPOINT, 33.f, 210.f, erOp);
	rc |= limitCheckFix(DHWSYS_TSETPOINTLH, 33.f, 210.f, erOp);

	return rc;

}	// DHWSYS::ws_CheckVals
//-----------------------------------------------------------------------------
float DHWSYS::ws_GetTSetpoint(		// resolve setpoint
	int whfcn) const	// water heating function
						//   whfcnPRIMARY, whfcnLOOPHEATER
// values can change hourly
{
	// cascading defaults
	//    tSetpoint defaults to tUse
	//    tSetpointLH defaults to tSetpoint
	float tSetpoint = ws_tUse;
	if (IsSet(DHWSYS_TSETPOINT))
		tSetpoint = ws_tSetpoint;
	if (whfcn == DHWHEATER::whfcnLOOPHEATER
	 && IsSet(DHWSYS_TSETPOINTLH))
		tSetpoint = ws_tSetpointLH;
	return tSetpoint;
}		// DHWSYS::ws_GetTSetpoint
//-----------------------------------------------------------------------------
int DHWSYS::ws_GetTSetpointFN(		// resolve setpoint source field
	int whfcn) const	// water heating function
						//   whfcnPRIMARY, whfcnLOOPHEATER
{
	// cascading defaults
	//    tSetpoint defaults to tUse
	//    tSetpointLH defaults to tSetpoint
	int fn = 0;
	if (IsSet(DHWSYS_TSETPOINT))
		fn = DHWSYS_TSETPOINT;
	if (whfcn == DHWHEATER::whfcnLOOPHEATER
	 && IsSet(DHWSYS_TSETPOINTLH))
		fn = DHWSYS_TSETPOINTLH;
	return fn;
}		// DHWSYS::ws_GetTSetpointFN
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
		if (IsSet(DHWSYS_CENTRALDHWSYSI))
		{	const DHWSYS* pWSX = ws_GetCentralDHWSYS();
			rc |= r->oer("Not allowed here, this DHWSYS is served by %s",
				pWSX ? strtprintf("central DHWSYS '%s'", pWSX->name)
				     : "a central DHWSYS.");
		}
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
	return ws_childDHWSYSCount > 0.f;
}		// DHWSYS::ws_IsCentralDHWSYS
//-----------------------------------------------------------------------------
RC DHWSYS::RunDup(		// copy input to run record; check and initialize
	const record* pSrc,		// input record
	int options/*=0*/)
{
	RC rc = record::RunDup( pSrc, options);
	ws_whCount = 0.f;		// insurance
	ws_wlhCount = 0.f;		// insurance
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
		//   modified in later passes iff ws_loadShareDHWSYSi
		ws_fxCount[0] = 1;
		VCopy(ws_loadShareCount, C_DHWEUCH_COUNT, ws_fxCount);
		for (int iEU = 0; iEU < C_DHWEUCH_COUNT; iEU++)
			ws_LSRSet(iEU, 0, ws_fxCount[iEU]);

		ws_childDHWSYSCount = 0;
		DHWSYS* pWSCentral = ws_GetCentralDHWSYS();
		if (pWSCentral)
		{	for (int iEU = 0; iEU < C_DHWEUCH_COUNT; iEU++)
			{	int iFn = DHWSYS_FXCOUNT + iEU;
				if (pWSCentral->IsSet(iFn))
					rc |= pWSCentral->ooer(iFn, "%s not allowed on central DHWSYS",
						pWSCentral->mbrIdTx(iFn));
				pWSCentral->ws_fxCount[iEU] = 0;		// counts derived from child DHWSYSs (see below)
			}
			if (IsSet(DHWSYS_DAYUSENAME))
				pWSCentral->ws_childDHWDAYUSEFlag++;
		}

		// moving sums re sizing values
		//   retain recent draw / load values during C_WSCALCMODECH_PRERUN
		//   else left 0
		ws_drawMaxMS.vm_Init(ws_drawMaxDur);
		ws_loadMaxMS.vm_Init(ws_loadMaxDur);

		return rc;
	}

	if (pass == 2)
	{	// final pass: all mbrs of loadShare group get identical ws_loadShareCount[]
		if (ws_loadShareDHWSYSi > 0)
		{	DHWSYS* pWS = WsR.GetAtSafe( ws_loadShareDHWSYSi);
			if (pWS)	// insurance: NULL impossible?
			{	// ensure that at least one end use target is included in group.
				//   No effect if no draws of that end use are given
				//   But all draws will be handled
				for (int iEU = 0; iEU < C_DHWEUCH_COUNT; iEU++)
				{	if (pWS->ws_loadShareCount[iEU] < 1)
					{	if (iEU > 0)
							pWS->oInfo("1 %s has been added for load sharing purposes -- none found in input.",
								getChoiTxI(DTDHWEUCH, iEU));
						pWS->ws_loadShareCount[iEU] = 1;
						pWS->ws_LSRSet(iEU, 0, 1);
					}
				}
				// all members of group have identical counts
				VCopy(ws_loadShareCount, C_DHWEUCH_COUNT, pWS->ws_loadShareCount);
			}
		}

		// check that water heating is possible
		if (ws_whCount == 0.f)		// if no water heaters
		{	if (!ws_HasCentralDHWSYS())		// if central or stand-alone
				oInfo("no DHWHEATER(s), water heating energy use not modeled.");
			if (ws_wlhCount > 0.f)
				rc |= oer( "no DHWHEATER(s), DHWLOOPHEATER(s) not allowed.");
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
					pWR->oInfo("no wsDayUse, DHWHEATREC heat recovery not modeled.");
			}
		}

		// check DHWHEATREC configuration
		//   do not need ws_mult, applies to both values
		if (ws_wrFxDrainCount > ws_ShowerCount())
			rc |= oer( "Invalid heat recovery arrangement: more DHWHEATREC drain connections (%d) than showers (%d)",
					ws_wrFxDrainCount, ws_ShowerCount());
		
		// set up DHWSYS fixture list
		//    associates each fixture with DHWHEATREC (or not)
		//    only C_DHWEUCH_SHOWER supported as of 2-19
		//    order does not matter: scrambled when used (see ws_AssignDHWUSEtoFX())
		delete[] ws_fxList;
		ws_fxList = NULL;
		if (!rc && ws_ShowerCount() > 0)
		{
			ws_fxList = new DHWFX[ws_ShowerCount()];

			// set up linkage to DHWHEATRECs
			//   assign for each DHWHEATREC in order
			//   fixtures in excess of DHWHEATRECs don't drain via DHWHEATREC
			int iFx = 0;
			RLUPC(WrR, pWR, pWR->ownTi == ss)
				rc |= pWR->wr_SetFXConnections(this, iFx);
			// any remaining showers are linked to "no DHWHEATREC"
		}

		// additional config checks
		if (!ws_configChecked)
		{	// don't bother with ignore msgs if not RCOK (no run)
			if (!rc && !ws_HasCentralDHWSYS())
			{	if (ws_whCountUseTS == 0.f)
					ignore(DHWSYS_TSETPOINT,
						"-- no setpoint-aware DHWHEATERs in this DHWSYS.");
				if (ws_wlhCountUseTS == 0.f)
					ignore(DHWSYS_TSETPOINTLH,
						"-- no setpoint-aware DHWLOOPHEATERs in this DHWSYS.");
			}

			// set checked flag in both run and input DHWSYSs
			// WHY: suppresses duplicate info msgs when >1 RUN
			// side-effect: does not re-check after ALTER
			ws_configChecked++;
			DHWSYS* pWSi = WSiB.GetAtSafe(ss);
			if (pWSi)
				pWSi->ws_configChecked++;	// set in input record
											// carries to subsequent WsR copies
		}

		return rc;
	}	// pass == 2

	// pass 1
	if (ws_HasCentralDHWSYS())
	{	// check that central DHWSYS tree is only one deep
		//   additional levels could be allowed if needed
		//   see ws_AccumCentralUse() etc.
		DHWSYS* pWSCentral = ws_GetCentralDHWSYS();
		if (pWSCentral->ws_HasCentralDHWSYS())
			rc |= oer( "DHWSYS '%s' (given by wsCentralDHWSYS) is not central",
				pWSCentral->name);

		pWSCentral->ws_childDHWSYSCount += ws_mult;
		VAccum( pWSCentral->ws_fxCount, C_DHWEUCH_COUNT, ws_fxCount, ws_mult);	// total # of fixtures served

		// set or default some child values from parent
		//   wsCalcMode, wsTSetpoint: not allowed
		//   wsSSF: not allowed, uses parent
		//   wsTInlet, wsTUse: allowed, default to parent
		//   wsSDLM, wsDSM, wsWF: allowed, default to parent

		RRFldCopy( pWSCentral, DHWSYS_SSF);
		RRFldCopyIf( pWSCentral, DHWSYS_SDLM);
		RRFldCopyIf( pWSCentral, DHWSYS_WF);
		RRFldCopyIf( pWSCentral, DHWSYS_DSM);
		RRFldCopyIf( pWSCentral, DHWSYS_TUSE);
		RRFldCopyIf( pWSCentral, DHWSYS_TINLET);
		RRFldCopyIf( pWSCentral, DHWSYS_TSETPOINT);
		RRFldCopyIf( pWSCentral, DHWSYS_TSETPOINTLH);

		RRFldCopyIf( pWSCentral, DHWSYS_DAYWASTESCALE);
		for (int iEU=0; iEU<NDHWENDUSES; iEU++)
			RRFldCopyIf( pWSCentral, DHWSYS_DAYWASTEDRAWF+iEU);
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
			{	// ws_loadShareIdx = pWS->ws_loadShareCount[ 0];
				// pWS->ws_loadShareCount[ 0]++;
				for (int iEU = 0; iEU < C_DHWEUCH_COUNT; iEU++)
				{	ws_LSRSet(iEU, pWS->ws_loadShareCount[iEU], ws_fxCount[iEU]);
					pWS->ws_loadShareCount[iEU] += ws_fxCount[iEU];
				}
				// this->ws_loadShareCount set in pass 2 (above)

				RRFldCopy( pWS, DHWSYS_DAYUSENAME);
				RRFldCopy( pWS, DHWSYS_HWUSE);
			}
		}
	}

	// array of per-tick info
	delete[] ws_ticks;	// insurance (generally NULL already)
	ws_ticks = new DHWTICK[ Top.tp_NHrTicks()];

	// ws_whCount / ws_wshCount set in DHWHEATER::RunDup
	float noLHCount = 0.f;	// count of primary heaters that cannot
							//   support DHWLOOPHEATER
	DHWHEATER* pWH;
	RLUPC(WhR, pWH, pWH->ownTi == ss)	// primary heaters
	{	rc |= pWH->wh_Init();
		if (!pWH->wh_CanHaveDHWLOOPHEATER())
			noLHCount += pWH->wh_mult;
	}
	RLUPC(WlhR, pWH, pWH->ownTi == ss)	// loop ("swing") heaters
	{	if (noLHCount > 0.f)
			rc |= pWH->oer("Unsupported configuration --\n"
				   "      primary DHWHEATER(s) not whHeatSrc=ASHPX or RESISTANCEX");
		rc |= pWH->wh_Init();
	}

	// all-child totals
	ws_loopSegTotals.st_Init();		// DHWLOOPSEGs
	ws_branchTotals.st_Init();		// DHWLOOPBRANCHs

	DHWLOOP* pWL;
	RLUPC( WlR, pWL, pWL->ownTi == ss)
	{	rc |= pWL->wl_Init();
		ws_loopSegTotals.st_Accum(pWL->wl_segTotals, pWL->wl_mult);
		ws_branchTotals.st_Accum(pWL->wl_branchTotals, pWL->wl_mult);
	}

	// total target warmup water waste, gal/day
	ws_dayWaste = ws_dayWasteVol + ws_dayWasteBranchVolF * ws_branchTotals.st_vol;

	if (!ws_HasDHWDAYUSEDraws())
	{	// no DHWDAYUSE (on this or any child): info msgs re draw-related input
		const char* when = "-- there are no wsDayUse draws.";
		ignoreN(when, DHWSYS_DAYWASTEVOL, DHWSYS_DAYWASTEBRANCHVOLF, 0);
		for (int iEU=1; iEU<NDHWENDUSES; iEU++)
			ignoreN(when, DHWSYS_DRAWWASTE + iEU, DHWSYS_DAYWASTEDRAWF + iEU, 0);
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

	return rc;	// pass 1 return
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
	float brVF = ws_branchTotals.st_count > 0.f
					? ws_whUse.total / (ws_branchTotals.st_count * 60.f)
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
	static double wlVolTot = 0.;

	if (ivl <= C_IVLCH_D)	// if start of day (or longer)
	{
		if (Top.tp_isBegMainSim)
		{	// reset annual values after autosize / warmup
			VZero(ws_drawCount, NDHWENDUSES);

			if (ws_fxList)
				for (int iFx = 0; iFx < ws_ShowerCount(); iFx++)
					ws_fxList[iFx].fx_hitCount = 0;
			// intialize run totals for all child water heaters
			DHWHEATER* pWH;
			RLUPC(WhR, pWH, pWH->ownTi == ss)
				pWH->wh_InitTotals();

			// various run totals
			ws_t24WLTot = 0.;
			VZero(ws_fxUseMixTot, NDHWENDUSESXPP);
			VZero(ws_whUseTot, NDHWENDUSESXPP);

		}
		
		if (IsSet( DHWSYS_DAYUSENAME))
		{	// beg of day: locate DHWDAYUSE, set ws_dayUsei
			if (WduR.findRecByNm1( ws_dayUseName, &ws_dayUsei, NULL))
				return orMsg( ERRRT+SHOFNLN, "DHWDAYUSE '%s' not found.", ws_dayUseName);
		}

		// re load share -- init starting DHWSYS for each end use
		// provides some randomization
		// basing on jDay yields consistent results for part-year short runs
		int seed = Top.jDay;
		for (int iEU=1; iEU<NDHWENDUSES; iEU++)
			ws_loadShareWS0[ iEU] = (seed+iEU)%ws_loadShareCount[ 0];
	}
	
	// inlet temp = feed water to cold side of WHs
	if (!IsSet( DHWSYS_TINLET))
		ws_tInlet = Wthr.d.wd_tMains;		// default=mains

	// runtime checks of vals possibly set by expressions
	rc |= ws_CheckVals( ERRRT | SHOFNLN);	// checks ws_SSF, ws_tUse, ws_tSetpoint

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
	static const int WHDRAWFDIM = sizeof( ws_drawDurF)/sizeof( ws_drawDurF[ 0]);
#if defined( _DEBUG)
	if (WHDRAWFDIM != NDHWENDUSES)
		err( PABT, "ws_drawDurF array size error");
	VSet( ws_drawDurF, WHDRAWFDIM, -1.f);
	if (sizeof(ws_drawWaste)/sizeof( ws_drawWaste[ 0]) != NDHWENDUSES)
		err(PABT, "ws_drawWaste array size error");
#endif

	// Duration factor
	float drawDurF = ws_WF * ws_DLM;
	// temperature-dependent end uses
	//   losses modeled by extending draw
	ws_drawDurF[ 0]
		= ws_drawDurF[ C_DHWEUCH_SHOWER]
		= ws_drawDurF[C_DHWEUCH_FAUCET]
		= ws_drawDurF[ C_DHWEUCH_BATH] = drawDurF;
	// temperature independent end uses
	//   losses do not effect draw
	float drawDurFTempInd = 1.f;
	ws_drawDurF[ C_DHWEUCH_CWASHR]
		= ws_drawDurF[ C_DHWEUCH_DWASHR]
		= drawDurFTempInd;

#if defined( _DEBUG)
	if (VMin( ws_drawDurF, WHDRAWFDIM) < 0.f)
		err( PABT, "ws_drawDurF fill error");
#endif

	// ** Hot water use **
	//   2 types of user input
	//   * ws_hwUse = use for current hour (generally an expression w/ schedule)
    //   * DHWDAYUSE = collection of times and flows
	// here we combine these to derive consistent hourly total and tick-level (1 min) bins

	// init tick bins to average of hourly (initializes for hour)
	double hwUseX = ws_hwUse / ws_loadShareCount[ 0];	// hwUse per system
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

	// draws now known -- maintain meters, totals, etc
	float mult = ws_mult * centralMult;	// overall multiplier
	rc |= ws_DoHourDrawAccounting( mult);

#if 0
	// write draw info to CSV file
	if (ws_drawCSV == C_NOYESCH_YES && !Top.isWarmup)
		ws_WriteDrawCSV();

	// accumulate water use to DHWMTRs if defined
	//   include DHWSYS.ws_mult multiplier
	float mult = ws_mult*centralMult;	// overall multiplier
	if (ws_pFXhwMtr)
		ws_pFXhwMtr->H.wmt_Accum( &ws_fxUseMix, 0, mult);
	if (ws_pWHhwMtr)
		ws_pWHhwMtr->H.wmt_Accum( &ws_whUse, 0, mult);

	// accumulate water use to annual totals
	//    redundant if DHWMTRs are defined
	ws_fxUseMix.wmt_AccumTo( ws_fxUseMixTot);
	ws_whUse.wmt_AccumTo( ws_whUseTot);
#endif

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
	ws_t24WL = 0.f;
	ws_volRL = 0.f;
	double tVolRet = 0.;
	if (ws_wlCount > 0)		// if any loops
	{	DHWLOOP* pWL;
		RLUPC( WlR, pWL, pWL->ownTi == ss)
		{	rc |= pWL->wl_DoHour( mult);		// also calcs child DHWLOOPSEGs and DHWLOOPPUMPs
			HRLL += pWL->wl_HRLLnet;	// loop loss
			ws_HRBL += pWL->wl_HRBL;	// branch loss
			ws_t24WL += pWL->wl_t24WL;	// branch waste loss volume
			ws_volRL += pWL->wl_volRL;
			tVolRet += pWL->wl_volRL * pWL->wl_tRL;
		}
		ws_tRL = tVolRet / ws_volRL;
	}
	ws_tRL = ws_volRL > 0.f ? tVolRet / ws_volRL : 0.f;

	ws_t24WLTot += ws_t24WL;

	// distribution losses
	ws_HRDL = float(HRLL);
	if (ws_branchModel == C_DHWBRANCHMODELCH_T24DHW)
		ws_HRDL += ws_HRBL;		// conditionally include branch losses

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
	{
		DHWHEATER* pWH;
		RLUPC(WhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_DoHour(ws_HARL / ws_whCount, mult);

		// loop heaters
		if (ws_wlhCount > 0) RLUPC(WlhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_IsHPWHModel())
				pWH->wh_DoHour(0.f, mult);
		}
	}

	if (ws_wpCount > 0)		// if any child pumps
	{	DHWPUMP* pWP;
		RLUPC( WpR, pWP, pWP->ownTi == ss)
			pWP->wp_DoHour( mult);
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
//-----------------------------------------------------------------------------
RC DHWSYS::ws_DoHourDrawAccounting(		// water use accounting
	float mult)		// overall multiplier
					//    ws_mult * centralMult
// call *after* basic draw info is known for hour
//   ws_fxUseMix
//   ws_whUse
//   ws_ticks
// tick-level loop modeling done later

// returns RCOK iff no runtime error
{
	RC rc = RCOK;

	// write ws_ticks draw info to CSV file
	if (ws_drawCSV == C_NOYESCH_YES && !Top.isWarmup)
		ws_WriteDrawCSV();

	// accumulate water use to DHWMTRs if defined
	//   include DHWSYS.ws_mult multiplier
	if (ws_pFXhwMtr)
		ws_pFXhwMtr->H.wmt_Accum(&ws_fxUseMix, 0, mult);
	if (ws_pWHhwMtr)
		ws_pWHhwMtr->H.wmt_Accum(&ws_whUse, 0, mult);

	// accumulate water use to annual totals
	//    redundant if DHWMTRs are defined
	ws_fxUseMix.wmt_AccumTo(ws_fxUseMixTot);
	ws_whUse.wmt_AccumTo(ws_whUseTot);
	
	// track peaks for sizing
	//   = max draw in ws_drawMaxDur
	//   = max load in ws_loadMaxDur
	if (ws_calcMode == C_WSCALCMODECH_PRERUN)
	{	ws_drawMaxMS.vm_Sum( ws_whUse.total, &ws_drawMax);
		float whLoad = ws_whUse.total*(ws_tUse - ws_tInletX)*waterRhoCp;
		ws_loadMaxMS.vm_Sum( whLoad, &ws_loadMax);
	}

	return rc;
}		// DHWSYS::ws_DoHourDrawAccounting
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
	 || ws_ShowerCount() <= 0)					// no showers
		return -1;			// no fixture / no DWHR via DHWHEATREC

// result must be stable for same wu_drawSeqN
//   WHY: draws can span hour boundary, should go to same fixture
//   >>> can't use Top.iHr
// pWU->ss varies with pWU->wu_drawSeqN, but pWU->ss order is not known

	unsigned int seq;
#if 0
	static int bSetup = 0;
	static int iRands[101];

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
			// counts[iRands[i] % ws_ShowerCount()]++;
		}
		bSetup++;
	}
	drawSeqCount[pWU->wu_drawSeqN]++;
	seq = (pWU->wu_drawSeqN * 97 + Top.jDay*7) % 41;
	d2Count[seq]++;
	counts[seq%ws_ShowerCount()]++;

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
	int iFx = seq % ws_ShowerCount();

#if 0
0	if (Top.jDay == 365 && Top.iHr==23 && !Top.isWarmup)
0		printf("\nHit");
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
		ws_ticks[ iTk].wtk_Init( iTk*Top.tp_subhrTickDur, whUseTick, ws_tInletX);

	DHWHEATREC* pWR;
	RLUPC(WrR, pWR, pWR->ownTi == ss)
		pWR->wr_InitTicks();

	ws_iTk0DWHR = 9999;		// 1st/nth tick with possible DWHR
	ws_iTkNDWHR = -1;
}		// DHWSYS::ws_InitTicks
//-----------------------------------------------------------------------------
RC DHWSYS::ws_DoHourDWHR()		// current hour DHWHEATREC modeling (all DHWHEATRECs)
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
	double qRWHSum = 0.;		// hour total recovered heat to WH inlet, Btu
	int multiDraw = 0;
	// int nTk = Top.tp_NHrTicks();
	for (int iTk=ws_iTk0DWHR; iTk < ws_iTkNDWHR; iTk++)
	{	DHWTICK& tk = ws_ticks[ iTk];		// DHWSYS tick info
		if (tk.wtk_nHRDraws == 0)
			continue;		// no DHWHEATREC draws in this tick
		if (tk.wtk_nHRDraws > 1)
			multiDraw++;
		float whUseOther = tk.wtk_whUse;
		float vHotOther		// non-DHWHEATREC draws that contribute to each
							//   feedWH-DHWHEATREC potable-side vol
			= whUseOther / max(ws_wrFeedWHCount, 1);
#if defined( _DEBUG)
		int nReDo = 0;		// debugging aid, see below
	reDo:
#endif
		float whUse = 0.f;		// hot water use, this tick / all DHWHEATREC draws
		float fxUseMix = 0.f;	// mixed water use
		float qR = 0.f;			// tick heat recovered
		float qRWH = 0.f;		// tick heat recovered to WH feed
		float whUseNoHR = 0.f;	// tick hot water use w/o HR
		DHWHEATREC* pWR;
		RLUPC(WrR, pWR, pWR->ownTi == ss)
		{	DHWHRTICK& wrtk = pWR->wr_ticks[iTk];	// DHWHEATREC tick info
			if (wrtk.wrtk_draws.size() > 0)
				whUse += pWR->wr_CalcTick( this,
					wrtk,			// tick info for *pWR
					vHotOther,		// total non-HR hot water use, gal
					whUseNoHR, fxUseMix, qR, qRWH);	// results accum'd
		}
#if defined( _DEBUG)
		if (!nReDo)
#endif
			ws_AccumUseTick(		// accum to ws_tick, ws_whUse, and ws_fxMixUse
				C_DHWEUCH_SHOWER, iTk, fxUseMix, whUse);

		// water heater inlet temp
		float tO = ws_tInlet;
		if (tk.wtk_whUse > 0.)
			tO += qRWH / (waterRhoCp * tk.wtk_whUse);
		tk.wtk_tInletX = ws_AdjustTInletForSSF(tO);

#if defined( _DEBUG)
		// tick energy balance
		float qXNoHR = (whUseNoHR+whUseOther) * waterRhoCp * (ws_tUse - ws_tInlet);
		float qX     = tk.wtk_whUse           * waterRhoCp * (ws_tUse - tO);
		float qXHR = qX + qR;
		if (frDiff(qXHR, qXNoHR, 1.f) > .001f)
		{
			printf("\nDHWSYS '%s': ws_DoHourDWHR tick balance error (md=%d)", name, multiDraw);
			if (nReDo++ < 2)
				goto reDo;		// repeat calc (debugging aid)
		}
#endif
		ws_qDWHR += qR;		// hour total heat recovered
		qRWHSum += qRWH;	// hour total heat to WH inlet
		ws_whUseNoHR += whUseNoHR;		// hour total WH use w/o HR

	}  // end tick

	// hour average adjusted inlet and hot water temps
	float tInletXNoSSF;
	if (qRWHSum > 0.)
	{
		tInletXNoSSF = ws_tInlet + qRWHSum / (waterRhoCp * ws_whUse.total);
		ws_tInletX = ws_AdjustTInletForSSF(tInletXNoSSF);
	}
	else
		tInletXNoSSF = ws_tInlet;

#if defined( _DEBUG)
	// hour energy balance
	float qXNoHR = ws_whUseNoHR * waterRhoCp * (ws_tUse - ws_tInlet);
	float qX =     ws_whUse.total * waterRhoCp * (ws_tUse - tInletXNoSSF);
	float qXHR = qX + ws_qDWHR;
	if (frDiff(qXHR, qXNoHR, 1.f) > .001f)
		printf("\nDHWSYS '%s': ws_DoHourDWHR balance error (md=%d)", name, multiDraw);
#endif

	return rc;
}		// DHWSYS::ws_DoHourDWHR
//-----------------------------------------------------------------------------
RC DHWSYS::ws_AddLossesToDraws(		// assign losses to ticks (subhr)
	DHWTICK* ticksSh)	// draws by tick
// updates tick info re loop and other losses
// results are for DHWSYS, allocated later per DHWHEATER
{
	RC rc = RCOK;

	double scaleTick = 1. / Top.tp_NHrTicks();	// allocate per tick
	double qLossNoRL = ws_HJL * scaleTick;	// w/o recirc: jacket
	if (ws_branchModel == C_DHWBRANCHMODELCH_T24DHW)
		qLossNoRL += ws_HRBL * scaleTick;	// T24DHW: branches losses modeled as heat
											// else: branch losses included in draws
	double volRL = ws_volRL * scaleTick;	// DHWLOOP recirc vol/tick, gal

#if 0 && defined( _DEBUG)
	double qLossTot = (ws_HRDL + ws_HJL) * scaleTick;		// total: DHWLOOP + jacket
	double qLossRL = qLossTot - qLossNoRL;					// recirc only
	// compared to ws_tRL??
#endif

	// loop return conditions
	for (int iTk = 0; iTk < Top.tp_nSubhrTicks; iTk++)
	{	DHWTICK& tk = ticksSh[iTk];
		tk.wtk_volRL = volRL;
		tk.wtk_tRL = ws_tRL;
		tk.wtk_qLossNoRL = qLossNoRL;
	}

	return rc;

}	// DHWSYS::ws_AddLossesToDraws
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoSubhr()		// subhourly calcs
{
	RC rc = RCOK;
#if 1
	if (ws_whCount > 0.f && ws_calcMode != C_WSCALCMODECH_PRERUN)
	{
		int iTkSh = Top.iSubhr*Top.tp_nSubhrTicks;	// initial tick for this subhour
		DHWTICK* ticksSh = ws_ticks + iTkSh;		// 1st tick info for this subhour
		rc |= ws_AddLossesToDraws(ticksSh);

		double scaleWH = 1. / ws_whCount;		// allocate per WH

		DHWHEATER* pWH;

		if (ws_wlhCount == 0.f)		// if no loop heaters
		{
			RLUPC(WhR, pWH, pWH->ownTi == ss)
			{
				if (pWH->wh_IsHPWHModel())
					rc |= pWH->wh_HPWHDoSubhr(ticksSh, scaleWH);
				else if (pWH->wh_IsInstUEFModel())
					rc |= pWH->wh_InstUEFDoSubhr(ticksSh, scaleWH);
				else
					continue;
				// else missing case
				if (Top.isEndHour)
					pWH->wh_unMetHrs += pWH->wh_unMetSh > 0;
			}
			return rc;
		}
		

		// DHWSYS includes DHWLOOPHEATER(s) -- interleave primary/loop heater tick calcs
		//   all HPWH (enforced by input check)
		RLUPC(WhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_HPWHDoSubhrStart();
		RLUPC(WlhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_HPWHDoSubhrStart();

		for (int iTk = 0; !rc && iTk < Top.tp_nSubhrTicks; iTk++)
		{
			DHWTICK& tk = ticksSh[iTk];		// handy ref to current tick
			tk.wtk_volIn = 0.;
			RLUPC(WlhR, pWH, pWH->ownTi == ss)
				rc |= pWH->wh_HPWHDoSubhrTick(tk, 1./ws_wlhCount);

			ws_tOutPrimSum = 0.;
			RLUPC(WhR, pWH, pWH->ownTi == ss)
				rc |= pWH->wh_HPWHDoSubhrTick(tk, scaleWH);
			if (ws_tOutPrimSum != 0.)
				ws_tOutPrimLT = ws_tOutPrimSum;
		}

		RLUPC(WhR, pWH, pWH->ownTi == ss)
		{	rc |= pWH->wh_HPWHDoSubhrEnd();
			if (Top.isEndHour)
				pWH->wh_unMetHrs += pWH->wh_unMetSh > 0;
		}
		RLUPC(WlhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_HPWHDoSubhrEnd();
	}
#else
	if (ws_whCount > 0.f && ws_calcMode != C_WSCALCMODECH_PRERUN)
	{	int iTkSh = Top.iSubhr*Top.tp_nSubhrTicks;	// initial tick for this subhour
		DHWTICK* ticksSh = ws_ticks + iTkSh;		// 1st tick info for this subhour
		rc |= ws_AddLossesToDraws(ticksSh);

		double scaleWH = 1. / ws_whCount;		// allocate per WH

		DHWHEATER* pWH;
		RLUPC(WhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_IsHPWHModel())
				rc |= pWH->wh_HPWHDoSubhr(
					ticksSh,		// draws etc. by tick
					scaleWH);		// draw scale factor (allocates draw if ws_whCount > 1)
			else if (pWH->wh_IsInstUEFModel())
				rc |= pWH->wh_InstUEFDoSubhr(
					ticksSh,
					scaleWH);
			// else missing case
			if (Top.isEndHour)
				pWH->wh_unMetHrs += pWH->wh_unMetSh > 0;
		}
		// loop heaters
		if (ws_wlhCount > 0.f) RLUPC(WlhR, pWH, pWH->ownTi == ss)
		{	if (pWH->wh_IsHPWHModel())
				rc |= pWH->wh_HPWHDoSubhr(
					ticksSh,		// draws etc. by tick
					1. / ws_wlhCount); // draw scale factor (allocates draw if ws_wlhCount > 1)
			// else missing case

		}
	}
#endif
	return rc;
}	// DHWSYS::ws_DoSubhr
//----------------------------------------------------------------------------
RC DHWSYS::ws_WriteDrawCSV()// write this hour draw info to CSV
{
	if (!ws_pFDrawCSV)
	{
		// dump file name = <cseFile>_draws.csv
		const char* nameNoWS = strDeWS(strtmp(name));
		const char* fName =
			strsave(strffix2(strtprintf("%s_%s_draws", InputFilePathNoExt,nameNoWS), ".csv", 1));
		ws_pFDrawCSV = fopen(fName, "wt");
		if (!ws_pFDrawCSV)
		{	ws_drawCSV = C_NOYESCH_NO;	// don't try again
			return err(PERR, "Draw CSV open failure for '%s'", fName);
		}
		else
		{	// headings
			fprintf(ws_pFDrawCSV, "%s,%s\n", name, Top.runDateTime);
			fprintf(ws_pFDrawCSV, "%s %s\n", ProgName, ProgVersion);
			fprintf(ws_pFDrawCSV, "Mon,Day,DOW,Hr,MinHr,MinDay,tIn (F),tInHR (F),tHot (F),vHot (gal)\n");
		}
	}

	// loop ticks (typical tick duration = 1 min)
	int nTk = Top.tp_NHrTicks();
	for (int iTk = 0; iTk < nTk; iTk++)
	{	// standard time?
		int iHr = Top.iHrST;		// report in standard time
		fprintf(ws_pFDrawCSV, "%d,%d,%d,%d,",
			Top.tp_date.month, Top.tp_date.mday, Top.dowh+1, iHr+1);
		if (nTk == 60)
			fprintf(ws_pFDrawCSV, "%d,%d,", iTk+1, 60*iHr+iTk+1);
		else
		{	double minHr = iTk*Top.tp_subhrTickDur + 1;
			double minDay = double(60 * iHr) + minHr;
			fprintf(ws_pFDrawCSV, "%0.2f,%0.2f,", minHr, minDay);
		}
		const DHWTICK& tk = ws_ticks[iTk];
		// write to CSV w/o trailing 0s (many draws are 0, don't write 0.0000)
		fprintf(ws_pFDrawCSV, "%s,%s,%s,%s\n",
			WStrFmtFloatDTZ( tk.wtk_tInletX,2).c_str(),
			WStrFmtFloatDTZ( ws_tInlet, 2).c_str(),
			WStrFmtFloatDTZ( ws_tUse, 2).c_str(),
			WStrFmtFloatDTZ( tk.wtk_whUse, 4).c_str());
	}
	return RCOK;
}		// DHWSYS::ws_WriteDrawCSV
//----------------------------------------------------------------------------
RC DHWSYS::ws_EndIvl( int ivl)		// end-of-interval
{
	RC rc = RCOK;
	if (ivl == C_IVLCH_Y)
	{	if (ws_calcMode == C_WSCALCMODECH_PRERUN)
			rc |= ws_DoEndPreRun();
		if (ws_whCount > 0.f)
		{
			DHWHEATER* pWH;
			RLUPC(WhR, pWH, pWH->ownTi == ss)
			{
				if (pWH->wh_unMetHrs > 0)
					warn("%s: Output temperature below use temperature during %d hours of run.",
						pWH->objIdTx(), pWH->wh_unMetHrs);
			}
		}
	}
	if (ivl == C_IVLCH_H)
		ws_fxUseMixLH.wmt_Copy( &ws_fxUseMix);

	return rc;

}		// DHWSYS::ws_EndIvl
//-----------------------------------------------------------------------------
RC DHWSYS::ws_DoEndPreRun()		// finalize PRERUN results
// call after last hour of PRERUN
{
	RC rc = RCOK;

	DHWHEATER* pWH;
	RLUPC(WhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoEndPreRun();

	DHWSYS* pWSi = WSiB.GetAtSafe(ss);	// source input record
	if (!pWSi)
		return orMsg(ERR, "Bad input record linkage in ws_DoEndPreRun");


	if (!ws_HasCentralDHWSYS())		// if central or stand-alone
	{
		// accum any child drawCounts to central		
		DHWSYS* pWSChild;
		RLUPC(WsR, pWSChild, pWSChild->ws_centralDHWSYSi == ss)
			VAccum(ws_drawCount, NDHWENDUSES, pWSChild->ws_drawCount);

		// draws per day by end use
		VCopy(ws_drawsPerDay, NDHWENDUSES, ws_drawCount, 1. / Top.nDays);

		double wasteUnscaledTot = 0.;
		for (int iEU = 1; iEU < NDHWENDUSES; iEU++)
		{
			if (ws_dayWasteDrawF[iEU] > 0.f)
			{	// average daily waste = average # of draws * waste per draw
				double wasteUnscaled = ws_drawsPerDay[iEU] * ws_dayWasteDrawF[iEU];
				if (ws_fxUseMixTot[ iEU+1] > 0.)
					// adjust to waste at fixture
					wasteUnscaled *= ws_whUseTot[iEU + 1] / ws_fxUseMixTot[iEU + 1];
				wasteUnscaledTot += wasteUnscaled;
			}
		}
		// draw scaling factor: cause draw waste to (approx) equal target waste per day
		pWSi->ws_dayWasteScale = ws_dayWasteScale
			= wasteUnscaledTot > 0. ? float( ws_dayWaste / wasteUnscaledTot) : 0.f;

		// copy sizing info to input record
		//   available for use in DHWHEATER sizing expressions at start of C_WSCALCMODECH_SIM
		pWSi->ws_drawMax = ws_drawMax;
		pWSi->ws_loadMax = ws_loadMax;

	}

	// reset input record ws_calcMode
	pWSi->ws_calcMode = C_WSCALCMODECH_SIM;

	return rc;
}	// DHWSYS::ws_DoEndPreRun
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
//     only use (2/19) = random assignment of
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

 	if (pWS->ws_loadShareCount[ 0] > 1)
	{	// if load is being shared by more than 1 DHWSYS
		//   allocate by eventID to rotate DHWUSEs with suitable fixtures
		//   starting DHWSYS depends on jDay
		int iEU = wu_hwEndUse;
		int nFx = pWS->ws_loadShareCount[ iEU];
		int EID = wu_eventID + pWS->ws_loadShareWS0[iEU];
		int iX = EID % nFx;
		if (!pWS->ws_IsLSR( iEU, iX))
			return rc;		// not handled by this DHWSYS, do nothing
	}


	// derive adjusted duration, min
	//   losses are represented by extended draw
	float durX = wu_dur * pWS->ws_drawDurF[wu_hwEndUse]
			  + pWS->ws_DrawWaste(wu_hwEndUse) / wu_flow;		// warmup waste
	if (durX > minPerDay)
	{	// duration longer than 1 day
		// warn and limit
		rc |= orMsg( WRNRT, "adjusted draw duration %0.1f min changed to maximum allowed 1440 min (24 hr)",
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

	// >> some use in current hour <<

	// count draw
	//   Note: counts are (slightly) approx
	//     1. draws that span midnight are counted twice (these are rare in typical input)
	//     2. draws that have same eventID (e.g. DWASHR) are counted individually
	if (wu_hwEndUse > 0)
		pWS->ws_drawCount[wu_hwEndUse]++;
	pWS->ws_drawCount[0]++;

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
				rc |= orMsg( WRNRT, "wuHeatRecEF=%0.2f not in valid range 0 - 0.90",
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
void DHWSYS::ws_AccumUseTick(		// tick-level water use DHWMTR accounting
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
			rc |= orMsg( WRNRT, "Cold water temp (%0.1f F) >= hot water temp (%0.1f F).  "
					"Cannot mix to wuTemp (%0.1f F).",
					tCold, tHot, wu_temp);
		else if (mixRet == -1)
			rc |= orMsg( WRNRT, "wuTemp (%0.1f F) < cold water temp (%0.1f F).  "
					"Cannot mix to wuTemp.", wu_temp, tCold);
		else if (mixRet == 1)
			rc |= orMsg( WRNRT, "wuTemp (%0.1f F) > hot water temp (%0.1f F).  "
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
	RC rc = !pWS ? oer("DHWSYS not found")	// insurance (unexpected)
			     : RCOK;
	// rc |= pWS->ws_CheckSubObject() done in wh_Init()

	if (rc)
		return rc;	// give up

	int bIsPreRun = pWS->ws_calcMode == C_WSCALCMODECH_PRERUN;
	int whfcn = wh_SetFunction();		// sets wh_fcn

	// surrounding zone or temp: zone illegal if temp given
	//   used only re HPWH 2-16, enforce for all
	if (IsSet( DHWHEATER_TEX))
		rc |= disallowN( "when 'whTEx' is specified", DHWHEATER_ZNTI, 0);

	if (wh_heatSrc != C_WHHEATSRCCH_ELRESX)
	{	ignoreN( whenHs, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, 0);
		if (wh_heatSrc != C_WHHEATSRCCH_ASHPX)
			ignore( DHWHEATER_UAMULT, whenHs);
	}

	if (whfcn == whfcnLOOPHEATER && !wh_CanHaveLoopReturn())
		rc |= oer("DHWLOOPHEATER must be whType=SmallStorage with whHeatSrc=ASPX or whHeatSrc=ResistanceX.");


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
			VD wh_ashpTSrc = VD wh_tEx;		// default ashpTSrc to tEx
											//   VD handles NANDLES
		}
	}
	else if (wh_heatSrc == C_WHHEATSRCCH_ELRESX)
	{	// small storage electric resistance (HPWH model)
		ignoreN( whenHs, DHWHEATER_LDEF, DHWHEATER_ASHPTY,
			DHWHEATER_ASHPTSRC, DHWHEATER_ASHPSRCZNTI, DHWHEATER_ASHPRESUSE, 0);
		rc |= requireN( whenHs, DHWHEATER_EF, DHWHEATER_VOL, 0);
 		if (wh_EF > 0.98f)
			rc |= oer("whEF (%0.3f) must be <= 0.98 %s",
				wh_EF, whenHs);
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

	if (!wh_CanHaveLoopReturn())
		ignoreN(whenHs, DHWHEATER_INHTSUPPLY, DHWHEATER_INHTLOOPRET, 0);

	if (wh_IsStorage())
	{	// note wh_vol is required in some cases
        //   see above
        if (!IsSet( DHWHEATER_VOL))
			wh_vol = 50.f;
		rc |= limitCheck( DHWHEATER_VOL, .1, 20000.);
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
	int whfcn = wh_GetFunction();
	int usesTSetpoint = wh_UsesTSetpoint();
	if (whfcn == whfcnPRIMARY)
	{	pWS->ws_whCount += wh_mult;
		pWS->ws_whCountUseTS += usesTSetpoint * wh_mult;
	}
	else
	{	pWS->ws_wlhCount += wh_mult;
		pWS->ws_wlhCountUseTS += usesTSetpoint * wh_mult;
	}
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

	wh_pFCSV = NULL;

	// one-time inits
	wh_balErrCount = 0;

	// per run totals (also called on 1st main sim day)
	wh_InitTotals();

	DHWSYS* pWS = wh_GetDHWSYS();

	rc |= pWS->ws_CheckSubObject(this);		// check system config
											//   DHWHEATER not allows on child DHWSYS

	int whfcn = wh_GetFunction();

	// whfcnLASTHEATER: this heater feeds load (not another heater)
	if (whfcn == whfcnLOOPHEATER || pWS->ws_wlhCount == 0.f)
		wh_fcn |= whfcnLASTHEATER;

	if (wh_CanHaveLoopReturn() && pWS->ws_calcMode == C_WSCALCMODECH_SIM)
	{	// no info msgs on PRERUN -- else duplicates
		if (pWS->ws_wlCount == 0)
		{	ignore(DHWHEATER_INHTLOOPRET, "when DHWSYS includes no DHWLOOP(s).");
			if (whfcn == whfcnLOOPHEATER)
				oInfo("modeled as a series heater because DHWSYS includes no DHWLOOP(s).");
		}
		else if (whfcn == whfcnPRIMARY && pWS->ws_wlhCount > 0)
			ignore(DHWHEATER_INHTLOOPRET, "when DHWSYS includes DHWLOOPHEATER(s).");
	}

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
	delete wh_pHPWH; wh_pHPWH = NULL;	// free pre-existing HPWH data (insurance)
	delete[] wh_HPWHHSMap; wh_HPWHHSMap = NULL;

	if (wh_IsHPWHModel())
		rc |= wh_HPWHInit();	// set up from DHWHEATER inputs

	else if (wh_type == C_WHTYPECH_INSTUEF)
		rc |= wh_InstUEFInit();		// UEF-based instantaneous water heater model 5-2017

	return rc;
}		// DHWHEATER::wh_Init
//----------------------------------------------------------------------------
void DHWHEATER::wh_InitTotals()
// start-of-run initialization totals, error counts, ...
// called at beg of warmup and run
{
	wh_totHARL = 0.;
	wh_hrCount = 0;
	wh_totOut = 0.;
	wh_unMetHrs = 0;
	wh_stbyTicks = 0;
	wh_fMixUse = wh_fMixRL = 1.f;

}		// DHWHEATER::wh_InitTotals
//----------------------------------------------------------------------------
DHWSYS* DHWHEATER::wh_GetDHWSYS() const
{
	DHWSYS* pWS = (b == &WhR || b == &WlhR ? WsR : WSiB).GetAtSafe(ownTi);
	return pWS;
}		// DHWHEATER::wh_GetDHWSYS
//----------------------------------------------------------------------------
int DHWHEATER::wh_SetFunction()		// determine function
// returns whfcnXXXX
{
	wh_fcn =
		  b == &WhR || b == &WHiB ? whfcnPRIMARY
		: b != NULL               ? whfcnLOOPHEATER
		:                           whfcnUNKNOWN;
	// note: whfcnLASTHEATER bit added in wh_Init()
	return wh_fcn;
}		// DHWHEATER::wh_SetFunction
//----------------------------------------------------------------------------
DHWHEATER* DHWHEATER::wh_GetInputDHWHEATER() const
// returns ptr to source input record
{
	anc< DHWHEATER>*bpi = b == &WhR ? &WHiB : b == &WlhR ? &WLHiB : NULL;
	DHWHEATER* pWH = bpi ? bpi->GetAtSafe(ss) : NULL;
	return pWH;
}		// DHWHEATER::wh_GetInputDHWHEATER
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
	wh_totHARL += HARL;		// annual total
	wh_hrCount++;

	wh_inElec = 0.f;
	wh_inElecBU = 0.f;
	wh_inFuel = 0.f;

	if (wh_IsSubhrModel())
	{	// this DHWHEATER uses subhour model
		//   cannot simply loop subhour model at this point
		//   Must include calcs at subhour level re e.g. zone coupling
		//   See ws_DoSubhr()
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
	{	if (wh_EF < 1.f)		// if not ideal efficiency (testing)
		{	// average hourly load
			float arl = wh_totHARL / wh_hrCount;
			// "Load-dependent energy factor"
			float LDEF = wh_CalcLDEF(arl);
			// update input record with derived value
			DHWHEATER* pWHi = wh_GetInputDHWHEATER();
			if (pWHi && !pWHi->IsSet(DHWHEATER_LDEF))
			{	pWHi->wh_LDEF = LDEF;
				pWHi->fStat(DHWHEATER_LDEF) |= FsSET | FsVAL;
			}
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
	pInfo( "DHWHEATER '%s' HPWH message (%s):\n  %s",
		name, Top.When( C_IVLCH_S), message.c_str());
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

	int whfcn = wh_GetFunction();

	wh_pHPWH = new( HPWH);

	wh_pHPWH->setMessageCallback( wh_HPWHMessageCallback, this);
	wh_pHPWH->setVerbosity( HPWH::VRB_reluctant);		// messages only for fatal errors

	int hpwhRet = 0;
	float UAX = -1.f;		// alternative UA
	float volX = IsSet(DHWHEATER_VOL) ? wh_vol : -1.f;		// alternative volume
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
		if (volX < 0.f)
			volX = 45.f;			// ... and change vol / UA
		UAX = 2.9f;
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_AOSMITHSHPT66)
	{	preset = HPWH::MODELS_GE2012;
		if (volX < 0.f)
			volX = 63.9f;
		UAX = 3.4f;
	}
	else if ( wh_ashpTy == C_WHASHPTYCH_AOSMITHSHPT80)
	{	preset = HPWH::MODELS_GE2012;
		if (volX < 0.f)
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
		wh_vol = wh_pHPWH->getTankSize(HPWH::UNITS_GAL);	// force force probe-able value

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

		// config checks -- report only once
		if (!pWS->ws_configChecked)
		{	if (wh_pHPWH->isSetpointFixed())
			{	int fn = pWS->ws_GetTSetpointFN(whfcn);
				if (fn)
					pWS->ignore(fn,
						strtprintf("-- HPWH '%s' has a fixed setpoint.", name));
			}
		}

		// inlet fractional heights
		//  set iff user input given -- HPWH may have non-0 defaults
		if (IsSet( DHWHEATER_INHTSUPPLY))
			wh_pHPWH->setInletByFraction(wh_inHtSupply);
		if (IsSet( DHWHEATER_INHTLOOPRET))
			wh_pHPWH->setInlet2ByFraction(wh_inHtLoopRet);
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

		// nominal tank heat content, kJ
		wh_tankHCNominal = KJ_TO_KWH(40. * HPWH::DENSITYWATER_kgperL * HPWH::CPWATER_kJperkgC
			* wh_pHPWH->getTankSize());
	}

	return rc;
}		// DHWHEATER::wh_HPWHInit
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoHour()		// hourly HPWH calcs
// any HPWH setup etc that need not be done subhourly
// returns RCOK iff success
{
	RC rc = RCOK;

	int whfcn = wh_GetFunction();

	DHWSYS* pWS = wh_GetDHWSYS();

	wh_unMetSh = 0;		// count of this hrs subhrs
						//   having wh_tHOut < wh_tUse

	// setpoint temp: ws_tUse has hourly variability
	//   some HPWHs (e.g. Sanden) have fixed setpoints, don't attempt
	if (!wh_pHPWH->isSetpointFixed())
		wh_pHPWH->setSetpoint( DegFtoC( pWS->ws_GetTSetpoint( whfcn)));

	// retrieve resulting setpoint after HPWH restrictions
	double tHPWHSetpoint = DegCtoF( wh_pHPWH->getSetpoint());
	if (wh_tHWOut == 0.f)
		wh_tHWOut = tHPWHSetpoint;		// initial guess for HW output temp
										//   updated every substep with nz draw
	if (pWS->ws_tOutPrimLT == 0.f)
		pWS->ws_tOutPrimLT = tHPWHSetpoint;		// initial guess for primary heater outlet temp

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
struct CSVItem
{
	const char* ci_hdg;
	double ci_v;
	int ci_iUn;	// units
	int ci_dfw;	// sig figs (re FMTSQ)
	// int ci_iUx -- fixed unit system for this item

	static const double ci_UNSET;	// flag for unset value (writes 0)

	const char* ci_Hdg( int iUx);
	const char* ci_Value();
};
//-----------------------------------------------------------------------------
/*static*/ const double CSVItem::ci_UNSET = -99999999.;
//-----------------------------------------------------------------------------
const char* CSVItem::ci_Hdg(int iUx)
{
	return strtprintf( ci_iUn != UNNONE ? "%s(%s)" : "%s",
		ci_hdg, UNIT::GetSymbol(ci_iUn, iUx));
}		// CSVItem::ci_Hdg
//-----------------------------------------------------------------------------
const char* CSVItem::ci_Value()
{
	return ci_v != ci_UNSET
		? cvin2s(&ci_v, DTDBL, ci_iUn, 20, FMTSQ | FMTOVFE | FMTRTZ + ci_dfw)
		: "0";
}		// CSVItem::ci_Value
//-----------------------------------------------------------------------------
class CSVGen
{
	CSVItem* cg_pCI;	// array of CSVItems
	int cg_iUx;			// unit system

public:
	CSVGen(CSVItem* pCI) : cg_pCI( pCI) {}
	WStr cg_Hdgs( int iUx);
	WStr cg_Values(int iUx);
};
//-----------------------------------------------------------------------------
WStr CSVGen::cg_Hdgs(int iUx)
{
	WStr s;
	for (int i = 0; cg_pCI[i].ci_hdg; i++)
	{
		s += cg_pCI[i].ci_Hdg(iUx);
		s += ",";
	}
	return s;
}
//-----------------------------------------------------------------------------
WStr CSVGen::cg_Values(int iUx)
{
	int UnsysextSave = Unsysext;
	Unsysext = iUx;
	WStr s;
	for (int i = 0; cg_pCI[i].ci_hdg; i++)
	{
		s += cg_pCI[i].ci_Value();
		s += ",";
	}
	Unsysext = UnsysextSave;
	return s;
}
//-----------------------------------------------------------------------------
#if 0
RC DHWHEATER::wh_HPWHDoSubhr(		// HPWH subhour
	DHWTICK* ticksSh,	// tick info for subhour (draw, cold water temp, )
						//   returned with output temps updated
	double scaleWH)		// draw scale factor
						//   re DHWSYSs with >1 DHWHEATER
						//   *not* including wh_fMixUse or wh_fMixRL;
// calls HPWH tp_nSubhrTicks times, totals results
// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	float mult = pWS->ws_mult * wh_mult;	// overall multiplier
	int whfcn = wh_GetFunction();
	bool bLastWH	// true iff this is last heater (output goes to loop and draws)
		= whfcn == whfcnLOOPHEATER || pWS->ws_wlhCount == 0.f;

	// local totals
	wh_qEnv = 0.;		// heat removed from environment, kWh
						//   + = to water heater
	wh_qLoss = 0.;		// standby losses, kWh;  + = to surround

	wh_qHW = 0.;		// total hot water heating, kWh; always >= 0
						//   includes heat to DHWLOOP;  does not include wh_HPWHxBU

	double tHWOutF = 0.;	// accum re average hot water outlet temp, F
	// wh_fMixUse, wh_fMixRL: initialized in wh_InitTotals(); value retained hour-to-hour

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
	// use debug dump mechanism w/o headings to log file
	//   (dump goes to external CSV file)
	int bWriteCSV = DbDo( dbdHPWH, dbdoptNOHDGS);
#endif
	

#if 0
	// recirc loop return temp
	//   Note: DHWLOOP losses calc'd with independent temperature assumptions (!)
	//   Derive loop return temp using heat loss ignoring DHWLOOP temps
	//     i.e. assume loop entering temp = ws_tUse
	//       tRetRL = tUse - qLossRL/(waterRhoCp * volRL)
	//     vol*temp product: tRetRL * volRL = tUse*volRL - qLossRL/waterRhoCp
	double tVolRL = 0.;
	float tRetRL = 0.f;
	if (volRL > 0.)
	{
		tRetRL = pWS->ws_tUse - qLossRL / (waterRhoCp*volRL);
		tVolRL = volRL * tRetRL;
	}
#endif

	int nzDrawCount = 0;		// count of ticks with draw > 0
	for (int iTk=0; !rc && iTk<Top.tp_nSubhrTicks; iTk++)
	{	
		DHWTICK& tk = ticksSh[iTk];		// handy ref to current tick

#if 1
		int minHr = Top.iSubhr*Top.tp_nSubhrTicks + iTk;
		rc |= wh_HPWHDoTick(minHr, tk, scaleWH, nzDrawCount, tHWOutF, bWriteCSV);
#else
#if 0 && defined( _DEBUG)
		if (Top.tp_date.month == 7
		  && Top.tp_date.mday == 27
		  && Top.iHr == 10
		  && Top.iSubhr == 3)
			wh_pHPWH->setVerbosity( HPWH::VRB_emetic);
#endif
		double scaleX = scaleWH * wh_fMixUse;
		double drawUse = tk.wtk_whUse*scaleX;		// use draw same for primary and loop heater

		// inlet temp, F
		double tInlet = whfcn == whfcnLOOPHEATER
			? tk.wtk_TOutPrimAvg()	// loopheater: average outlet temp of primary heaters
			: tk.wtk_tInletX;		// primary: inlet temp( mains + DWHR)
		
		// pseudo-draw (gal) to represent e.g. central system branch losses
		//  ?tInlet?
		double drawLoss = tk.wtk_qLossNoRL*scaleX / (waterRhoCp * max(1., pWS->ws_tUse - pWS->ws_tInlet));
		
		// loop flow
		double drawRL;	// volume
		double tRL;		// loop return temp
		if (bLastWH)
		{	drawRL = tk.wtk_volRL * scaleWH * wh_fMixRL;
			tRL = tk.wtk_tRL;
		}
		else
			drawRL = tRL = 0.f;

		double drawForTick = drawUse + drawLoss + drawRL;


#define MIX 0
#if MIX == 1
		// mixed all to inlet 1
		tInlet = (drawRL*tRL + (drawForTick - drawRL)* tInlet) / drawForTick;
		drawRL = 0.;
		tRL = 0.;
#elif MIX==2
		// mixed all to inlet 2
		tRL = (drawRL*tRL + (drawForTick - drawRL)* tInlet) / drawForTick;
		drawRL = drawForTick;
		tInlet = 0.;
#endif

		wh_pHPWH->setInletT( DegFtoC( tInlet));		// mixed inlet temp

		int hpwhRet = wh_pHPWH->runOneStep(
			GAL_TO_L(drawForTick),	// draw volume, L
			DegFtoC(wh_tEx),		// ambient T (=tank surround), C
			DegFtoC(wh_ashpTSrc),	// heat source T, C
									//   aka HPWH "external temp"
			HPWH::DR_ALLOW,			// DRstatus: no demand response modeled
			GAL_TO_L( drawRL), DegFtoC(tRL));

		if (hpwhRet)	// 0 means success
			rc |= RCBAD;

		wh_qEnv += wh_pHPWH->getEnergyRemovedFromEnvironment();
		wh_qLoss += wh_pHPWH->getStandbyLosses();
		float HPWHxBU = 0.f;		// add'l resistance backup, this tick, Btu
		double tOut = wh_pHPWH->getOutletTemp();	// output temp, C (0 if no draw)
		if (tOut < .01)
			wh_stbyTicks++;		// no draw: accum duration
								//   (info only)
		else
		{	nzDrawCount++;		// this tick has draw
			wh_stbyTicks = 0;	// reset standby duration
			double tOutF = DegCtoF( tOut);	// output temp, F
			tk.wtk_TOutPrimAccum( tOutF, scaleWH);
			if (bLastWH)
			{	// output goes to load
				if (tOutF < pWS->ws_tUse)
				{	// load not met, add additional (unlimited) resistance heat
					wh_fMixUse = wh_fMixRL = 1.f;
					HPWHxBU = waterRhoCp * drawForTick * (pWS->ws_tUse - tOutF);
					wh_HPWHxBU += HPWHxBU;
					tOutF = pWS->ws_tUse;
				}
				else
				{	// mix to obtain ws_tUse
					//   set wh_fMixUse and wh_fMixRL for next tick
					DHWMix(pWS->ws_tUse, tOutF, tk.wtk_tInletX, wh_fMixUse);
					DHWMix(pWS->ws_tUse, tOutF, tRL, wh_fMixRL);
				}
			}
			tHWOutF += tOutF;	// note tOutF may have changed (but not tOut)

			wh_qHW += KJ_TO_KWH(		// heat added to water, kWh 
				(GAL_TO_L(drawForTick)*tOut
					- GAL_TO_L(drawForTick-drawRL)*DegFtoC(tInlet)
					- GAL_TO_L(drawRL)*DegFtoC(tRL))
				* HPWH::DENSITYWATER_kgperL
				* HPWH::CPWATER_kJperkgC);
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
		int dumpUx = UNSYSIP;		// unit system for CSV values
		int hpwhOptions = dumpUx == UNSYSIP ? HPWH::CSVOPT_IPUNITS : HPWH::CSVOPT_NONE;
		static const int nTCouples = 12;		// # of storage layers reported by HPWH
		
		if (bWriteCSV)
		{
			double minHr = double(Top.iSubhr)*Top.subhrDur*60. + iTk * Top.tp_subhrTickDur + 1;
			double minDay = double(60 * Top.iHr) + minHr;
			double minYr = double( (int(Top.jDayST-1)*24+Top.iHrST)*60) + minHr;

			CSVItem CI[] =
			{ "minHr",	   minHr,               UNNONE, 4,	
			  "minDay",	   minDay,              UNNONE, 4,
			  "minYr",	   minYr,               UNNONE, 4,
			  "tDbO",      Top.tDbOSh,			UNTEMP, 5,
			  "tEnv",      wh_tEx,				UNTEMP, 5,
			  "tSrcAir",   wh_ashpTSrc,			UNTEMP, 5,
			  "fMixUse",   wh_fMixUse,		    UNNONE, 5,
			  "fMixRL",    wh_fMixRL,		    UNNONE, 5,
			  "vUse",	   drawUse,				UNLVOLUME2, 5,
			  "vLoss",     drawLoss,			UNLVOLUME2, 5,
			  "vRL",       drawRL,				UNLVOLUME2, 5,
			  "vTot",	   drawForTick,			UNLVOLUME2, 5,
			  "tMains",    pWS->ws_tInlet,		UNTEMP, 5,
			  "tDWHR",     tk.wtk_tInletX,		UNTEMP, 5,
			  "tRL",       drawRL > 0. ? tRL : CSVItem::ci_UNSET,
												UNTEMP,	5,
			  "tIn",       tInlet > 0. ? tInlet : CSVItem::ci_UNSET,
												UNTEMP,	5,
			  "tSP",	   DegCtoF( wh_pHPWH->getSetpoint()),
												UNTEMP,	5,
			  "tOut",      tOut > 0. ? DegCtoF(tOut) : CSVItem::ci_UNSET,
												UNTEMP,  5,
			  "XBU",       HPWHxBU,				UNENERGY3,	5,
			  "tUse",      pWS->ws_tUse,		UNTEMP,  5,
			  "qLoss",     KWH_TO_BTU( wh_pHPWH->getStandbyLosses()), UNENERGY3, 5,
			  NULL
			};

 			CSVGen csvGen(CI);

			if (!wh_pFCSV)
			{
				// dump file name = <cseFile>_<DHWHEATER name>_hpwh.csv
				const char* nameNoWS = strDeWS(strtmp(name));
				const char* fName =
					strsave(strffix2(strtprintf("%s_%s_hpwh", InputFilePathNoExt, nameNoWS), ".csv", 1));
				wh_pFCSV = fopen( fName, "wt");
				if (!wh_pFCSV)
					err( PWRN, "HPWH report failure for '%s'", fName);
				else
				{	// headings
					fprintf( wh_pFCSV, "%s,%s,%s\n",wh_desc, Top.repHdrL,Top.runDateTime);
					fprintf( wh_pFCSV, "%s%s %s %s HPWH %s\n",
						Top.tp_RepTestPfx(), ProgName, ProgVersion, ProgVariant,
						Top.tp_HPWHVersion);
#if defined( HPWH_DUMPSMALL)
					fprintf( wh_pFCSV, "minYear,draw( L)\n");
#else
					WStr s("mon,day,hr,");
					s += csvGen.cg_Hdgs(dumpUx);
					wh_pHPWH->WriteCSVHeading(wh_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
				}
			}
			if (wh_pFCSV)
			{	
#if defined( HPWH_DUMPSMALL)
				fprintf( wh_pFCSV, "%0.2f,%0.3f\n", minYear, GAL_TO_L( drawForTick));
#else
				WStr s = strtprintf("%d,%d,%d,",
					Top.tp_date.month, Top.tp_date.mday, Top.iHr+1);
				s += csvGen.cg_Values(dumpUx);
				wh_pHPWH->WriteCSVRow(wh_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
			}
		}
#endif	// HPWH_DUMP
#endif
	}		// tick (1 min) loop

	// water heater average output temp, F
	if (nzDrawCount)
	{	wh_tHWOut = tHWOutF / nzDrawCount;	// note: >= ws_tUse due to unlimited add'l backup
		if (pWS->ws_tUse - wh_tHWOut > 1.f)
			wh_unMetSh++;			// unexpected
	}
	// else leave prior value = best available

	// link zone heat transfers
	if (wh_pZn)
	{	double qLPwr = wh_qLoss * mult * BtuperkWh / Top.subhrDur;	// power to zone, Btuh
		wh_pZn->zn_qDHWLoss += qLPwr;
		// TODO: HPWH 50/50 conc/rad split is approx at best
		wh_pZn->zn_qDHWLossAir += qLPwr * 0.5;
		wh_pZn->zn_qDHWLossRad += qLPwr * 0.5;
	}

	if (wh_pAshpSrcZn && wh_qEnv > 0.)
	{	// heat extracted from zone
		double qZn = wh_qEnv * mult * BtuperkWh / Top.subhrDur;
		wh_pAshpSrcZn->zn_qHPWH -= qZn;
		// air flow: assume 20 F deltaT
		// need approx value re zone convective coefficient derivation
		double amfZn = float( qZn / (20. * Top.tp_airSH));
		wh_pAshpSrcZn->zn_hpwhAirX += float( amfZn / wh_pAshpSrcZn->zn_dryAirMass);
	}

#if defined( HPWH_QBAL)
	if (!Top.isWarmup && !Top.tp_autoSizing)
	{	// form energy balance = sum of heat flows into water, all kWh
		double deltaHC = KJ_TO_KWH( wh_pHPWH->getTankHeatContent_kJ()) - qHCStart;
		double elecIn = wh_HPWHUse[0] + wh_HPWHUse[1];
		double qBal = wh_qEnv		// HP energy extracted from surround
					- wh_qLoss		// tank loss
					+ elecIn		// electricity in
					- wh_qHW		// hot water energy
					- deltaHC;		// change in tank stored energy
		double fBal = fabs(qBal) / max( wh_tankHCNominal, 1.);
		if (fBal >
#if defined( _DEBUG)
				.002)
#else
				.004)		// higher msg threshold in release
#endif
		{	// energy balance error
			static const int WHBALERRCOUNTMAX = 10;
 			wh_balErrCount++;
			if (wh_balErrCount <= WHBALERRCOUNTMAX || fBal > 0.01)
			{	warn("DHWHEATER '%s': HPWH energy balance error for %s (%1.6f kWh  f=%1.6f)",
					name,
					Top.When(C_IVLCH_S),	// date, hr, subhr
					qBal, fBal);			// unbalance calc'd just above
				if (wh_balErrCount == WHBALERRCOUNTMAX)
					warn("DHWHEATER '%s': Skipping further messages for minor energy balance errors.",
						name);
			}
		}
	}
#endif

	// output accounting = heat delivered to water
	wh_totOut += KWH_TO_BTU( wh_qHW) + wh_HPWHxBU;

	// energy use accounting, Btu (electricity only, assume no fuel)
	wh_AccumElec( mult,
		wh_HPWHUse[ 1] * BtuperkWh + wh_parElec * Top.subhrDur * BtuperWh,
	    wh_HPWHUse[ 0] * BtuperkWh);
	return rc;
}		// DHWHEATER::wh_HPWHDoSubhr
//-----------------------------------------------------------------------------
#endif

RC DHWHEATER::wh_HPWHDoSubhr(		// HPWH subhour
	DHWTICK* ticksSh,	// tick info for subhour (draw, cold water temp, )
						//   returned with output temps updated
	double scaleWH)		// draw scale factor
						//   re DHWSYSs with >1 DHWHEATER
						//   *not* including wh_fMixUse or wh_fMixRL;
// calls HPWH tp_nSubhrTicks times, totals results
// returns RCOK iff success
{
	RC rc = wh_HPWHDoSubhrStart();

	for (int iTk = 0; !rc && iTk < Top.tp_nSubhrTicks; iTk++)
	{
		DHWTICK& tk = ticksSh[iTk];		// handy ref to current tick

		rc |= wh_HPWHDoSubhrTick( tk, scaleWH);

	}		// tick (1 min) loop

	rc |= wh_HPWHDoSubhrEnd();

	return rc;
}


// ---------------------------------------------------------------------------- -
RC DHWHEATER::wh_HPWHDoSubhrStart()		// HPWH subhour start
//
// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();

	// local totals
	wh_qEnv = 0.;		// heat removed from environment, kWh
						//   + = to water heater
	wh_qLoss = 0.;		// standby losses, kWh;  + = to surround

	wh_qHW = 0.;		// total hot water heating, kWh; always >= 0
						//   includes heat to DHWLOOP;  does not include wh_HPWHxBU

	wh_tHWOutF = 0.;	// accum re average hot water outlet temp, F
	// wh_fMixUse, wh_fMixRL: initialized in wh_InitTotals(); value retained hour-to-hour

	wh_HPWHUse[0] = wh_HPWHUse[1] = 0.;	// energy use totals, kWh
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
	wh_tankHCStart = KJ_TO_KWH(wh_pHPWH->getTankHeatContent_kJ());
#endif

#define HPWH_DUMP		// define to include debug CSV file
#if defined( HPWH_DUMP)
	// #define HPWH_DUMPSMALL	// #define to use abbreviated version
		// use debug dump mechanism w/o headings to log file
		//   (dump goes to external CSV file)
	wh_bWriteCSV = DbDo(dbdHPWH, dbdoptNOHDGS);
#endif

	wh_nzDrawCount = 0;		// count of ticks with draw > 0

	return rc;
}	// DHWHEATER::wh_HPWHDoSubhrStart
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoSubhrTick(
	DHWTICK& tk,	// current tick
	double scaleWH) // draw scale factor
					//   re DHWSYSs with >1 DHWHEATER
					//   *not* including wh_fMixUse or wh_fMixRL;
{
	RC rc = RCOK;
#if 0 && defined( _DEBUG)
	if (Top.tp_date.month == 7
		&& Top.tp_date.mday == 27
		&& Top.iHr == 10
		&& Top.iSubhr == 3)
		wh_pHPWH->setVerbosity(HPWH::VRB_emetic);
#endif
	DHWSYS* pWS = wh_GetDHWSYS();

	// inlet temp, F
	double tInlet = wh_GetFunction() == whfcnLOOPHEATER
		? pWS->ws_tOutPrimLT	// loopheater: average outlet temp of primary heaters
		: tk.wtk_tInletX;		// primary: inlet temp( mains + DWHR)

	// draw components
	double scaleX = scaleWH * wh_fMixUse;
	double drawUse;	// use draw
	double drawLoss;// pseudo-draw (gal) to represent e.g. central system branch losses
	double drawRL;	// loop flow vol for tick
	double tRL;		// loop return temp
	if (wh_IsLastHeater())
	{	drawUse = tk.wtk_whUse*scaleX;
		drawLoss = tk.wtk_qLossNoRL*scaleX / (waterRhoCp * max(1., pWS->ws_tUse - pWS->ws_tInlet));
		tk.wtk_volIn += (drawUse + drawLoss) / scaleWH;
		drawRL = tk.wtk_volRL * scaleWH * wh_fMixRL;
		tRL = tk.wtk_tRL;
	}
	else
	{
		drawUse = tk.wtk_volIn * scaleWH;		// multipliers??
		drawLoss = drawRL = tRL = 0.f;
	}

	double drawForTick = drawUse + drawLoss + drawRL;


#define MIX 0
#if MIX == 1
	// mixed all to inlet 1
	tInlet = (drawRL*tRL + (drawForTick - drawRL)* tInlet) / drawForTick;
	drawRL = 0.;
	tRL = 0.;
#elif MIX==2
	// mixed all to inlet 2
	tRL = (drawRL*tRL + (drawForTick - drawRL)* tInlet) / drawForTick;
	drawRL = drawForTick;
	tInlet = 0.;
#endif

	wh_pHPWH->setInletT(DegFtoC(tInlet));		// mixed inlet temp

	int hpwhRet = wh_pHPWH->runOneStep(
		GAL_TO_L(drawForTick),	// draw volume, L
		DegFtoC(wh_tEx),		// ambient T (=tank surround), C
		DegFtoC(wh_ashpTSrc),	// heat source T, C
								//   aka HPWH "external temp"
		HPWH::DR_ALLOW,			// DRstatus: no demand response modeled
		GAL_TO_L(drawRL), DegFtoC(tRL));

	if (hpwhRet)	// 0 means success
		rc |= RCBAD;

	wh_qEnv += wh_pHPWH->getEnergyRemovedFromEnvironment();
	wh_qLoss += wh_pHPWH->getStandbyLosses();
	float HPWHxBU = 0.f;		// add'l resistance backup, this tick, Btu
	double tOut = wh_pHPWH->getOutletTemp();	// output temp, C (0 if no draw)
	if (tOut < .01)
		wh_stbyTicks++;		// no draw: accum duration
							//   (info only)
	else
	{	wh_nzDrawCount++;	// this tick has draw
		wh_stbyTicks = 0;	// reset standby duration
		double tOutF = DegCtoF(tOut);	// output temp, F
		if (wh_GetFunction() == whfcnPRIMARY)
			pWS->ws_tOutPrimSum += tOutF * wh_mult * scaleWH;
		if (wh_IsLastHeater())
		{	// output goes to load
			if (tOutF < pWS->ws_tUse)
			{	// load not met, add additional (unlimited) resistance heat
				wh_fMixUse = wh_fMixRL = 1.f;
				HPWHxBU = waterRhoCp * drawForTick * (pWS->ws_tUse - tOutF);
				wh_HPWHxBU += HPWHxBU;
				tOutF = pWS->ws_tUse;
			}
			else
			{	// mix to obtain ws_tUse
				//   set wh_fMixUse and wh_fMixRL for next tick
				DHWMix(pWS->ws_tUse, tOutF, tk.wtk_tInletX, wh_fMixUse);
				DHWMix(pWS->ws_tUse, tOutF, tRL, wh_fMixRL);
			}
		}
		wh_tHWOutF += tOutF;	// note tOutF may have changed (but not tOut)

		wh_qHW += KJ_TO_KWH(		// heat added to water, kWh 
			(GAL_TO_L(drawForTick)*tOut
				- GAL_TO_L(drawForTick - drawRL)*DegFtoC(tInlet)
				- GAL_TO_L(drawRL)*DegFtoC(tRL))
			* HPWH::DENSITYWATER_kgperL
			* HPWH::CPWATER_kJperkgC);
	}

	// energy use by heat source, kWh
	// accumulate by backup resistance [ 0] vs primary (= compressor or all resistance) [ 1]
	for (int iHS = 0; iHS < wh_HPWHHSCount; iHS++)
	{
		wh_HPWHUse[wh_HPWHHSMap[iHS]] += wh_pHPWH->getNthHeatSourceEnergyInput(iHS);
#if 0 && defined( _DEBUG)
		// debug aid
		if (wh_pHPWH->getNthHeatSourceEnergyInput(iHS) < 0.)
			printf("\nNeg input, iHS=%d", iHS);
#endif
	}
#if defined( HPWH_DUMP)
	// tick level CSV report for testing
	int dumpUx = UNSYSIP;		// unit system for CSV values
	int hpwhOptions = dumpUx == UNSYSIP ? HPWH::CSVOPT_IPUNITS : HPWH::CSVOPT_NONE;
	static const int nTCouples = 12;		// # of storage layers reported by HPWH

	if (wh_bWriteCSV)
	{	double minHrD = tk.wtk_startMin + 1.;
		double minDay = double(60 * Top.iHr) + minHrD;
		double minYr = double((int(Top.jDayST - 1) * 24 + Top.iHrST) * 60) + minHrD;

		CSVItem CI[] =
		{ "minHr",	   minHrD,              UNNONE, 4,
		  "minDay",	   minDay,              UNNONE, 4,
		  "minYr",	   minYr,               UNNONE, 4,
		  "tDbO",      Top.tDbOSh,			UNTEMP, 5,
		  "tEnv",      wh_tEx,				UNTEMP, 5,
		  "tSrcAir",   wh_ashpTSrc,			UNTEMP, 5,
		  "fMixUse",   wh_fMixUse,		    UNNONE, 5,
		  "fMixRL",    wh_fMixRL,		    UNNONE, 5,
		  "vUse",	   drawUse,				UNLVOLUME2, 5,
		  "vLoss",     drawLoss,			UNLVOLUME2, 5,
		  "vRL",       drawRL,				UNLVOLUME2, 5,
		  "vTot",	   drawForTick,			UNLVOLUME2, 5,
		  "tMains",    pWS->ws_tInlet,		UNTEMP, 5,
		  "tDWHR",     tk.wtk_tInletX,		UNTEMP, 5,
		  "tRL",       drawRL > 0. ? tRL : CSVItem::ci_UNSET,
											UNTEMP,	5,
		  "tIn",       tInlet > 0. ? tInlet : CSVItem::ci_UNSET,
											UNTEMP,	5,
		  "tSP",	   DegCtoF(wh_pHPWH->getSetpoint()),
											UNTEMP,	5,
		  "tOut",      tOut > 0. ? DegCtoF(tOut) : CSVItem::ci_UNSET,
											UNTEMP,  5,
		  "XBU",       HPWHxBU,				UNENERGY3,	5,
		  "tUse",      pWS->ws_tUse,		UNTEMP,  5,
		  "qLoss",     KWH_TO_BTU(wh_pHPWH->getStandbyLosses()), UNENERGY3, 5,
		  NULL
		};

		CSVGen csvGen(CI);

		if (!wh_pFCSV)
		{
			// dump file name = <cseFile>_<DHWHEATER name>_hpwh.csv
			const char* nameNoWS = strDeWS(strtmp(name));
			const char* fName =
				strsave(strffix2(strtprintf("%s_%s_hpwh", InputFilePathNoExt, nameNoWS), ".csv", 1));
			wh_pFCSV = fopen(fName, "wt");
			if (!wh_pFCSV)
				err(PWRN, "HPWH report failure for '%s'", fName);
			else
			{	// headings
				fprintf(wh_pFCSV, "%s,%s,%s\n", wh_desc, Top.repHdrL, Top.runDateTime);
				fprintf(wh_pFCSV, "%s%s %s %s HPWH %s\n",
					Top.tp_RepTestPfx(), ProgName, ProgVersion, ProgVariant,
					Top.tp_HPWHVersion);
#if defined( HPWH_DUMPSMALL)
				fprintf(wh_pFCSV, "minYear,draw( L)\n");
#else
				WStr s("mon,day,hr,");
				s += csvGen.cg_Hdgs(dumpUx);
				wh_pHPWH->WriteCSVHeading(wh_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
			}
		}
		if (wh_pFCSV)
		{
#if defined( HPWH_DUMPSMALL)
			fprintf(wh_pFCSV, "%0.2f,%0.3f\n", minYear, GAL_TO_L(drawForTick));
#else
			WStr s = strtprintf("%d,%d,%d,",
				Top.tp_date.month, Top.tp_date.mday, Top.iHr + 1);
			s += csvGen.cg_Values(dumpUx);
			wh_pHPWH->WriteCSVRow(wh_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
		}
	}
#endif	// HPWH_DUMP
	return rc;
}		// DHWHEATER::wh_HPWHDoSubhrTick

//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHDoSubhrEnd()
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	float mult = pWS->ws_mult * wh_mult;	// overall multiplier

	// water heater average output temp, F
	if (wh_nzDrawCount)
	{	wh_tHWOut = wh_tHWOutF / wh_nzDrawCount;	// note: >= ws_tUse due to unlimited add'l backup
		if (pWS->ws_tUse - wh_tHWOut > 1.f)
			wh_unMetSh++;			// unexpected
	}
	// else leave prior value = best available

	// link zone heat transfers
	if (wh_pZn)
	{
		double qLPwr = wh_qLoss * mult * BtuperkWh / Top.subhrDur;	// power to zone, Btuh
		wh_pZn->zn_qDHWLoss += qLPwr;
		// TODO: HPWH 50/50 conc/rad split is approx at best
		wh_pZn->zn_qDHWLossAir += qLPwr * 0.5;
		wh_pZn->zn_qDHWLossRad += qLPwr * 0.5;
	}

	if (wh_pAshpSrcZn && wh_qEnv > 0.)
	{	// heat extracted from zone
		double qZn = wh_qEnv * mult * BtuperkWh / Top.subhrDur;
		wh_pAshpSrcZn->zn_qHPWH -= qZn;
		// air flow: assume 20 F deltaT
		// need approx value re zone convective coefficient derivation
		double amfZn = float(qZn / (20. * Top.tp_airSH));
		wh_pAshpSrcZn->zn_hpwhAirX += float(amfZn / wh_pAshpSrcZn->zn_dryAirMass);
	}

#if defined( HPWH_QBAL)
	if (!Top.isWarmup && !Top.tp_autoSizing)
	{	// form energy balance = sum of heat flows into water, all kWh
		double deltaHC = KJ_TO_KWH(wh_pHPWH->getTankHeatContent_kJ()) - wh_tankHCStart;
		double elecIn = wh_HPWHUse[0] + wh_HPWHUse[1];
		double qBal = wh_qEnv		// HP energy extracted from surround
			- wh_qLoss		// tank loss
			+ elecIn		// electricity in
			- wh_qHW		// hot water energy
			- deltaHC;		// change in tank stored energy
		double fBal = fabs(qBal) / max(wh_tankHCNominal, 1.);
		if (fBal >
#if defined( _DEBUG)
			.002)
#else
			.004)		// higher msg threshold in release
#endif
		{	// energy balance error
			static const int WHBALERRCOUNTMAX = 10;
			wh_balErrCount++;
			if (wh_balErrCount <= WHBALERRCOUNTMAX || fBal > 0.01)
			{
				warn("DHWHEATER '%s': HPWH energy balance error for %s (%1.6f kWh  f=%1.6f)",
					name,
					Top.When(C_IVLCH_S),	// date, hr, subhr
					qBal, fBal);			// unbalance calc'd just above
				if (wh_balErrCount == WHBALERRCOUNTMAX)
					warn("DHWHEATER '%s': Skipping further messages for minor energy balance errors.",
						name);
			}
		}
	}
#endif

	// output accounting = heat delivered to water
	wh_totOut += KWH_TO_BTU(wh_qHW) + wh_HPWHxBU;

	// energy use accounting, Btu (electricity only, assume no fuel)
	wh_AccumElec(mult,
		wh_HPWHUse[1] * BtuperkWh + wh_parElec * Top.subhrDur * BtuperWh,
		wh_HPWHUse[0] * BtuperkWh);
	return rc;
}		// DHWHEATER::wh_HPWHDoSubhrEnd



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
	double scaleWH)			// draw scale factor
							//   re DHWSYSs with >1 DHWHEATER
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();

	// carry-forward: instantaneous heaters throttle flow to maintain temp.
	//   here represented by carrying forward a limited amount of unmet load
	wh_HPWHxBU = 0.;			// heat in excess of capacity (after carry-forward)
								//   provided by virtual resistance heat
								//   prevents gaming compliance via undersizing
	int nzDrawCount = 0;		// count of ticks with draw > 0
	double nTickFullLoad = 0.;	// fractional ticks of equiv full load
	double nColdStarts = 0.;	// # of cold startups
	for (int iTk=0; !rc && iTk<Top.tp_nSubhrTicks; iTk++)
	{	const DHWTICK& tk = ticksSh[iTk];
		float tInletMix;
		double drawForTick = tk.wtk_DrawTot(pWS->ws_tUse, tInletMix)*scaleWH;

		float deltaT = max( 1., pWS->ws_tUse - tInletMix);
										// temp rise, F max( 1, dT) to prevent x/0
		double qPerGal = waterRhoCp * deltaT;
		drawForTick += wh_loadCFwd / qPerGal;
		wh_loadCFwd= 0.;	// clear carry-forward, if not met, reset just below
		if (drawForTick > 0.)
		{	nzDrawCount++;
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
	double rcovElec = wh_operElec * nzDrawCount * tickDurHr;	// assume operation for entire tick with any draw
	// double startElec = wh_cycLossElec * nTickStart;	// unused in revised model
	// standby in ticks w/o draw
	double stbyElec = wh_stbyElec * (Top.tp_nSubhrTicks-nzDrawCount) * tickDurHr;

	// output accounting = heat delivered to water
	double qHW = nTickFullLoad * wh_maxFlowX / 67.;
	wh_totOut += qHW + wh_HPWHxBU;

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
// DHWTANK -- CEC T24DHW tank model (heat loss only, no storage)
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
#if 0 && defined( _DEBUG)
	static int testDone = 0;
	if (!testDone)
	{	wr_EffAdjustedTest();
		testDone++;
	}
#endif
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
		for (int iF2=0; iF2 < pWS->ws_ShowerCount(); iF2++)
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
	float vHotOther,	// hot water draws for other fixtures, gal
						//   included in potable flow if feedsWH
	float& whUseNoHR,	// returned updated: hot water use w/o heat recovery, gal
						//   used re energy balance check
	float& fxUseMix,	// returned updated: total mixed water use, gal
	float& qR,			// returned updated: tick total recovered heat, Btu
	float& qRWH)		// returned updated: tick recovered heat added to WH inlet water, Btu

// returns hot water use for served fixtures, gal
//     (not including vHotOther)
{
	int nD = wrtk.wrtk_draws.size();
	if (nD == 0)
		return 0.f;		// no draws, no effect

	// tick constants
	float tpI = pWS->ws_tInlet;		// mains temp
	float tHotFX = pWS->ws_tUse;	// hot water temp at fixture

	float vd = 0.f;			// total mixed use at all fixture(s), all draws, gal
							//   = drain volume
	float tdI = 0.f;		// average drain-side entering temp, F
	float vMixFXHR = 0.f;	// total mixed use at fixtures with cold side
							//    connection to DHWHEATREC, gal
	float vHotFX0= 0.f;		// hot water req'd for fixtures that use
							//    mains water for mixing, gal

	// re parallel potable-side DHWHEATRECs
	//   caller allocates vHotOther equally to all feedsWH-DHWHEATREC(s) in DHWSYS
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
			vMixFXHR += hru.wdw_vol;
	}
	fxUseMix += vd;		// accum to caller's total

	// avg DHWHEATREC drain-side entering temp
	//   constrained by physical limits (ignore possible cooling of potable water)
	tdI = bracket(tpI, tdI/max( vd, .0001f), tHotFX);


	float vp = 0.f;		// potable-side flow, gal
	float tpO = 0.f;	// potable-side outlet temp, F
	float vHotFX = 0.f;	// fixture hot vol, gal

	if (vMixFXHR > 0.f)		// if any current draw feeds a fixture
	{	
		// DHWHEATREC feeds fixture(s) and possibly WH
		vp = wr_FeedsWH()		// potable volume
			? vMixFXHR + vHotFX0 + vHotOther	//  feeds both
			: vMixFXHR / 2.f;				//  fixture only: 1st guess
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
				vp = vMixFXHR - vHotFX;

#if 0 && defined( _DEBUG)
			if (iL > 7)
				printf("\nSlow converge  iL=%d  wr_eff=%.5f  tpO=%.2f", iL, wr_eff, tpO);
#endif
			if (fabs(tpO - tpOwas) < .1f)
				break;
			wr_EffAdjusted(vp, tpI, vd, tdI);		// update efficiency
		}
		vHotFX += vHotFX0;		// total fixture hot water
	}
	else
	{	// no current fixture draw uses tempered cold-side water
		//   all flows known
		vHotFX = vHotFX0;		// hot water needed
		if (wr_FeedsWH())
		{	// potable side feeds water heater
			//    recovered heat boosts tInlet
			vp = vHotFX + vHotOther;
			wr_EffAdjusted(vp, tpI, vd, tdI);	// derive wr_eff
			tpO = wr_HX(vp, tpI, vd, tdI);
		}
		else
		{	// does not feed WH
			//   no heat recovered
			vp = 0.f;		// no potable-side flow
			tpO = tpI;		// outlet temp = inlet temp (insurance)
		}
	}

	float qR1 = vp * waterRhoCp * (tpO - tpI);	// recovered heat
	qR += qR1;
	if (wr_FeedsWH() && vp > 0.f)
		qRWH += qR1 * (vHotFX + vHotOther) / vp;			// recovered heat to WH
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
		float tpIX = min(tpI, 81.f);	// limit tpI to 81 F
										//   fT slope is <0 above that
		double fT = (-3.06e-5*tpIX*tpIX + 4.96e-3*tpIX + 0.281)/0.466;

		// volume factor
		//   assume vp is smaller flow
		//   convert vp to gpm
		double v = vp/Top.tp_subhrTickDur;
		double v2, v3;
		double v4 = v*(v3=v*(v2=v*v));
		double fV = -6.98484455e-4*v4 + 1.28561447e-2*v3
					-7.02399803e-2*v2 + 1.33657748e-2*v + 1.23339312;

		wr_eff = wr_effRated * fT * fV;

		if (vd != vp)
		{	double fD = 1. + 0.3452 * log(vd / vp);
			wr_eff *= fD;
		}
	}
	
	return wr_eff = bracket( 0.f, wr_eff, 0.95f);
}	// DHWHEATREC::wr_effAdjusted
//-----------------------------------------------------------------------------
#if 0
x CASE report unequal flow regression
x   unused, not fully tested
x static double effAdjustedX(
x	float vp,	// potable water inlet flow, gal/tick
x	float tpI,	// potable water inlet temp, F
x	float vd,	// drain water inlet flow, gal/tick
x	float tdI)	// drain water inlet temp, F
x {
x	if (vp < 1.e-6f || vd < 1.e-6f)
x		return 0.;
x
x	// temperature factor
x	double fT = (-3.06e-5*tpI*tpI + 4.96e-3*tpI + 0.281) / 0.466;
x
x	double vp2 = vp * vp;
x	double vp3 = vp * vp2;
x	double vd2 = vd * vd;
x	double vd3 = vd * vd2;
x
x	double fV =
x		1.50240798
x - 1.54775844    * vp
x + 7.71878873e-1 * vp2
x - 1.21373939e-1 * vp3
x + 1.01401103    *       vd
x - 1.98860634e-1 * vp  * vd
x - 1.29877294e-1 * vp2 * vd
x + 3.56076077e-2 * vp3 * vd
x - 2.21066737e-1 *       vd2
x + 9.21985918e-2 * vp  * vd2
x + 1.06956686e-3 * vp2 * vd2
x - 3.51465197e-3 * vp3 * vd2
x + 1.38862333e-2 *       vd3
x - 6.82229011e-3 * vp  * vd3
x + 5.86661087e-4 * vp2 * vd3
x + 1.09840430e-4 * vp3 * vd3;
x
x	return bracket(0., fT * fV, 0.95);
x
x}
#endif
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
void DHWHEATREC::wr_EffAdjustedTest()
{
static float vdX[] = { 0.f, 0.1f, 0.3f, 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 4.f, 7.f, 10.f, 13.f, 16.f, 20.f, -1.f };
static float tdIX[] = { 100.f, -1.f };
static float vpX[] = { 0.f, 0.1f, 0.3f, 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 4.f, 7.f, 10.f, 13.f, 16.f, 20.f, -1.f };
static float tpIX[] = { 40.f, 50.f, 60.f, 70.f, -1.f };

	float effRatedSave = wr_effRated;
	wr_effRated = 0.46f;

	FILE* pF = fopen("DWHREff.csv", "wt");
	if (!pF)
		return;
	// headings
	fprintf(pF, "DWHR efficiency,%s\n", Top.runDateTime);
	fprintf(pF, "effRated,%0.2f\n", wr_effRated);

	fprintf(pF, "tdI, tpI, vd, vp, ef\n");

	for (int iTdI = 0; tdIX[iTdI] >= 0.f; iTdI++)
	for (int iTpI = 0; tpIX[ iTpI] >= 0.f; iTpI++)
	for (int iVd = 0; vdX[iVd] >= 0.f; iVd++)
	for (int iVp = 0; vpX[iVp] >= 0.f; iVp++)
	{
		float ef = wr_EffAdjusted(vpX[iVp], tpIX[iTpI], vdX[iVd], tdIX[iTdI]);
		// float fX = effAdjustedX(vpX[iVp], tpIX[iTpI], vdX[iVd], tdIX[iTdI]);
		fprintf(pF, "%.1f,%.1f,%.2f,%.2f,%.3f\n",
			tdIX[iTdI], tpIX[iTpI], vdX[iVd], vpX[iVp], ef);

	}
	wr_effRated = effRatedSave;

}	// DHWHEATREC::wr_EffAdjustedTest
#endif
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
}		// DHWPUMP::wp_GetDHWSYS
//-----------------------------------------------------------------------------
float DHWPUMP::wp_DoHour(			// hourly DHWPUMP/DHWLOOPPUMP calcs
	int mult,		// system multiplier
					//    DHWPUMP = ws_mult
					//    DHWLOOPPUMP = ws_mult*wl_mult
	float runF/*=1.*/)	// fraction of hour pump runs

// returns heat added to liquid stream, Btu
{
	RC rc = RCOK;
	wp_inElec = BtuperWh * runF * wp_pwr;	// electrical input, Btuh
											//   per pump (no wp_mult)

	if (wp_pMtrElec)
	{	if (rt == RTDHWLOOPPUMP)
			wp_pMtrElec->H.dhwMFL += wp_inElec * mult * wp_mult;
		else
			wp_pMtrElec->H.dhw += wp_inElec * mult * wp_mult;
	}

	return wp_inElec * wp_liqHeatF * wp_mult;	// heat added to liquid stream
												//  all pumps -- includes wp_mult

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
RC DHWLOOP::wl_Init()		// DHWLOOP init for run
{
	RC rc = RCOK;

	// compute totals
	wl_segTotals.st_Init();
	wl_branchTotals.st_Init();
	DHWLOOPSEG* pWG;
	RLUPC( WgR, pWG, pWG->ownTi == ss)
	{	rc |= pWG->wg_Init();
		wl_segTotals.st_Accum(pWG->ps_totals);
		DHWLOOPBRANCH* pWB;
		RLUPC(WbR, pWB, pWB->ownTi == pWG->ss)
			wl_branchTotals.st_Accum(pWB->ps_totals, pWB->wb_mult);
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
	wl_t24WL = 0.f;

	DHWSYS* pWS = wl_GetDHWSYS();

	float tIn1 = IsSet( DHWLOOP_TIN1) ? wl_tIn1 : pWS->ws_tUse;
	float tIn = tIn1;
	DHWLOOPSEG* pWG;
	RLUPC( WgR, pWG, pWG->ownTi == ss)
	{	// note: segment chain relies on input order
		rc |= pWG->wg_DoHour( tIn);
		wl_HRLL += pWG->wg_LL;	// flow + noflow loop losses
		wl_HRBL += pWG->wg_BL;	// branch losses
		wl_t24WL += pWG->wg_t24WL;	// branch waste loss vol
		tIn = pWG->ps_tOut;
	}

	int mult = wsMult * wl_mult;	// overall multiplier for meter accounting

	wl_qLiqLP = 0.f;	// heat gain to liquid stream, Btu
	if (wl_wlpCount > 0 && wl_flow*wl_runF > 0.f)	// if any loop pumps and any flow
	{	DHWLOOPPUMP* pWLP;
		RLUPC(WlpR, pWLP, pWLP->ownTi == ss)
			// calc electric energy use at wl_runF
			// accum to elect mtr with multipliers
			wl_qLiqLP += pWLP->wp_DoHour(mult, wl_runF);
	}
	
	wl_HRLLnet = wl_HRLL - wl_qLiqLP;	// cancel loop losses with pump power
										//  NOTE: wl_HRLLnet < 0 is possible

	float fMakeUp = 0.f;	// fraction of loss handled by loop heater
	if (wl_HRLLnet > 0.f)
	{	// meter hookup
		if (wl_lossMakeupPwr > 0.f)
		{	float HRLLMakeUp = min(wl_HRLLnet, wl_lossMakeupPwr * BtuperWh);
			fMakeUp = HRLLMakeUp / wl_HRLLnet;
			wl_HRLLnet -= HRLLMakeUp;
			if (wl_pMtrElec)
				wl_pMtrElec->H.dhwMFL += HRLLMakeUp * mult / wl_lossMakeupEff;
		}
	}

	// return water conditions
	float volHr = wl_flow * wl_runF * 60.;	// total flow for hour, gal
	wl_tRL = volHr > 0.f		// return temp
		? tIn1 - wl_HRLLnet / (volHr*waterRhoCp)	// not wl_tIn1! (see above)
		: 0.f;

	// energy and flow results: for wl_mult DHWLOOPs
	wl_HRLL  *= wl_mult;
	wl_HRLLnet *= wl_mult;
	wl_HRBL  *= wl_mult;
	wl_volRL = (1.f - fMakeUp) * volHr * wl_mult;	// flow returning to primary DHWHEATER(s)
													//   makeup heat fraction skips primary
	return rc;
}		// DHWLOOP::wl_DoHour
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// SEGTOTS: segment aggregated info
// PBC: pipe boundary condition
// PIPESEG: base class for DHWLOOPSEG and DHWLOOPBRANCH
//          implements common methods
///////////////////////////////////////////////////////////////////////////////
SEGTOTS::SEGTOTS()
{
	st_Init();
}
//-----------------------------------------------------------------------------
void SEGTOTS::st_Init()
{	st_count = st_len = st_vol = st_exArea = st_UA = 0.;
}
//-----------------------------------------------------------------------------
void SEGTOTS::st_Accum(
	const SEGTOTS& src,
	double count /*=1.*/)
{
	st_count += src.st_count*count;
	st_len += src.st_len*count;
	st_vol += src.st_vol*count;
	st_exArea += src.st_exArea*count;
	st_UA += src.st_UA*count;
}		// SEGTOTS::st_Accum
//=============================================================================
void PBC::sb_Init(PIPESEG* pPS)
{
	sb_pPS = pPS;
}		// DBC::sb_Init
//-----------------------------------------------------------------------------
#if 0
/*virtual*/ double DBC::sb_AreaNet() const		// *outside* duct area
// returns exposed (heat transfer) area
{
	return sb_pDS ? sb_pDS->ds_exArea : 0.;
}	// DBC::sb_Area
//-----------------------------------------------------------------------------
/*virtual*/ const char* DBC::sb_ParentName() const
{
	return sb_pDS ? sb_pDS->name : "?";
}		// SBC::sb_ParentName
//-----------------------------------------------------------------------------
#endif
/*virtual*/ int PBC::sb_Class() const
{
	return sfcPIPE;
}	// PBC::sb_Class
//=============================================================================

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
void PIPESEG::ps_CalcGeom()		// pipe seg derived geometric values
// sets ps_exArea (ft2) and ps_vol (gal)
{
	// initialize SEGTOTS re accum to parents
	//   other members set below
	ps_totals.st_count = 1.;
	ps_totals.st_len = ps_len;

	float r = ps_GetOD( 0) / 24.f;	// pipe radius, ft
	// include tube wall in vol, approximates heat cap of tubing
	ps_totals.st_vol = 7.48 * kPi * r * r * ps_len;
	
	double d = ps_GetOD(1) / 12.;
	ps_totals.st_exArea = d * kPi * ps_len;
}		// PIPESEG::ps_CalcGeom
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
	ps_totals.st_UA = ps_len * min( Ubare, fUA*Uinsul);
	return ps_totals.st_UA;
}		// PIPESEG::ps_CalcUA
//----------------------------------------------------------------------------
float PIPESEG::ps_CalcTOut(		// calc outlet or ending temp
	float tIn,			// inlet / beginning temp, F
	float flow)	const	// flow, gpm
// returns outlet / cooldown temp
{
	float tOut = ps_exT;
	if (flow > .00001f)		// if flow
	{	double f = exp( - double( ps_totals.st_UA / (flow * ps_fRhoCpX)));
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

	DHWLOOPSEG* pWGin = wg_GetInputDHWLOOPSEG();
	bool badOrder = false;
	if (wg_ty == C_DHWLSEGTYCH_SUP)
		badOrder = pWGin && pWGin->wg_ty != C_DHWLSEGTYCH_SUP;
	else if (wg_ty == C_DHWLSEGTYCH_RET)
		badOrder = pWGin == nullptr;
	if (badOrder)
		rc = ooer(DHWLOOPSEG_TY, "Bad segment order: wgTy=RETURN DHWLOOPSEGs\n"
			"    must follow wgTy=SUPPLY DHWLOOPSEGs");

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
DHWLOOPSEG* DHWLOOPSEG::wg_GetInputDHWLOOPSEG() const
// relies on input order (revise if network generalized)
// return ss of DHWLOOPSEG that feeds this
//        -1 if none (head segment)
{
	// search backwards for prior loop with same owner
	const anc< DHWLOOPSEG>* B = static_cast< const anc< DHWLOOPSEG>*> ( b);
	if (B) for (int ix = ss-1; ix >= B->mn; ix--)
	{	DHWLOOPSEG* pWG = B->GetAtSafe(ix);
		if (pWG && pWG->gud > 0 && pWG->ownTi == ownTi)
			return pWG;
	}
	return NULL;
}		// DHWLOOPSEG::wg_GetInputDHWLOOPSEG
//----------------------------------------------------------------------------
RC DHWLOOPSEG::wg_Init()		// init for run
{
	RC rc = RCOK;
	ps_CalcGeom();
	wg_CalcUA();

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
	{	ps_fvf = pWL->wl_flow;
		if (wg_ty == C_DHWLSEGTYCH_SUP)			// if supply segment
		{	float drawFlow = pWS->ws_whUse.total / (pWS->ws_wlCount * 60.f);	// draw, gpm
			if (drawFlow > 0.f)
				ps_fvf += drawFlow;
		}
	}
	else
		ps_fvf = 0.f;

	// outlet temp
	ps_tIn = tIn;
	ps_tOut = ps_CalcTOut( tIn, ps_fvf);

	// heat loss (flow)
	if (fNoFlow < 1.f)
		ps_PLWF = ps_fvf * (1.f - fNoFlow) * ps_fRhoCpX * (ps_tIn - ps_tOut);
	else
		ps_PLWF = 0.f;

	// heat loss (noflow)
	if (fNoFlow > 0.f)
	{	float tStart = (ps_tIn + ps_tOut) / 2.f;
		float volX = ps_totals.st_vol / 60.f;
		float tEnd = ps_CalcTOut( tStart, volX/fNoFlow);
		ps_PLCD = volX * ps_fRhoCpX * (tStart - tEnd);
	}
	else
		ps_PLCD = 0.f;

	// totals
	ps_PL = ps_PLWF + ps_PLCD;		// total DHWLOOPSEG loss
	wg_LL = ps_PL;					// losses seen in loop return

	// branch losses
	wg_BL = 0.f;
	wg_t24WL = 0.f;
	if (wg_wbCount > 0.f)
	{	DHWLOOPBRANCH* pWB;
		RLUPC( WbR, pWB, pWB->ownTi == ss)
		{	rc |= pWB->wb_DoHour( ps_tIn);		// TODO: inlet temp?
 			wg_BL += pWB->wb_mult * (pWB->wb_HBUL + pWB->wb_HBWL);
			wg_t24WL += pWB->wb_mult * pWB->wb_t24WL;
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
	ps_CalcGeom();
	wb_CalcUA();
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
	return ps_CalcUA( wb_fUA);		// note: loop wl_fUA not applied to branches
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
	// DHWLOOP* pWL = wb_GetDHWLOOP();

	// loss while water in use
	//   outlet temp found assuming use flow rate
	//   energy lost found using average flow rate
	ps_tOut = ps_CalcTOut( ps_tIn, wb_flow);
	ps_fvf = pWS->ws_BranchFlow();		// average flow rate
	wb_HBUL = ps_fvf * ps_fRhoCpX * (ps_tIn - ps_tOut);

	// waste loss
	// ?tInlet?
	wb_t24WL = wb_fWaste * ps_totals.st_vol;
	wb_HBWL = wb_t24WL * (ps_fRhoCpX/60.f) * (ps_tIn - pWS->ws_tInlet);

	// note: wb_mult applied by caller

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
