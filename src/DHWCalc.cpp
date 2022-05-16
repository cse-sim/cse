// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// DHWCalc.cpp -- Domestic Hot Water model implementation
///////////////////////////////////////////////////////////////////////////////

#include "CNGLOB.H"

#include "CSE.h"
#include "ANCREC.H" 	// record: base class for rccn.h classes
#include "rccn.h"
#include "IRATS.H"
#include "LOOKUP.H"
#include "CUPARSE.H"
#include "CUEVAL.H"
#include "CVPAK.H"
#include "YACAM.H"
#include "SRD.H"

// #include <random>
#include <queue>

#include "CNGUTS.H"
#include "EXMAN.H"

#include "hpwh.hh"	// decls/defns for Ecotope heat pump water heater model


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
// public functions
///////////////////////////////////////////////////////////////////////////////
RC DHWBegIvl(		// DHW (including solar DHW) start-of-hour calcs
	IVLCH ivl)	// C_IVLCH_Y, _M, _D, _H
// sets up loads etc. for all DHWSYSs and DHWSOLARSYSs
// call at start of hour or longer interval (do not call at start of substep)
{
	RC rc = RCOK;
	DHWSYS* pWS;
	RLUPC(WsR, pWS, !pWS->ws_HasCentralDHWSYS())
	{	// loop central systems (child systems handled within ws_DoHour)
		rc |= pWS->ws_DoHour(ivl);
	}

	// solar water heating systems
	DHWSOLARSYS* pSW;
	RLUP(SwhR, pSW)
		rc |= pSW->sw_DoHour();

	return rc;
}		// DHWBegIvl
//-----------------------------------------------------------------------------
RC DHWSubhr()		// DHW (including solar DHW) subhr calcs
{

	RC rc = RCOK;

	int iTk0 = Top.iSubhr*Top.tp_nSubhrTicks;
	int iTkL = iTk0 + Top.tp_nSubhrTicks;

	DHWSYS* pWS;
	RLUP(WsR, pWS)
		rc |= pWS->ws_DoSubhrStart( iTk0);

	DHWSOLARSYS* pSW;
	RLUP( SwhR, pSW)
		rc |= pSW->sw_DoSubhrStart(iTk0);

	for (int iTk = iTk0; !rc && iTk < iTkL; iTk++)
	{
		// solar water heating systems: init
		RLUP(SwhR, pSW)
			pSW->sw_TickStart();

		// water heating systems
		//   models all children (DHWHEATER, ...)
		RLUP(WsR, pWS)
			rc |= pWS->ws_DoSubhrTick( iTk);

		// solar water heating systems
		//   draw and inlet temp are accum'd during DHWSYS calcs
		RLUP(SwhR, pSW)
			rc |= pSW->sw_TickCalc( iTk);
	}

	RLUP(WsR, pWS)
		rc |= pWS->ws_DoSubhrEnd();

	RLUP(SwhR, pSW)
		rc |= pSW->sw_DoSubhrEnd();

	return rc;

}		// DHWDoSubhr
//-----------------------------------------------------------------------------
RC DHWEndIvl(			// end-of-hour
	IVLCH ivl)	// C_IVLCH_Y, _M, _D, _H (do not call for _S)
// called at end of each hour
{
	RC rc = RCOK;

	// solar water heating systems
	DHWSOLARSYS* pSW;
	RLUP(SwhR, pSW)
		rc |= pSW->sw_EndIvl(ivl);

	DHWSYS* pWS;
	RLUP(WsR, pWS)
		rc |= pWS->ws_EndIvl(ivl);


	return rc;

}		// DHWEndIvl
//=============================================================================

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
RC DHWMTR::wmt_Init([[maybe_unused]] IVLCH ivl)
// not called for C_IVLCH_SUBHOUR
{
	memset( &curr.H.total, 0, (NDHWENDUSES+1)*sizeof( float));

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
	DHWMTR_IVL* dIvl = &curr.Y + (ivl - C_IVLCH_Y);	// point destination substruct for interval
												// ASSUMES interval members ordered like DTIVLCH choices
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
}		// DHWMTR_IVL::wmt_Accum
//-----------------------------------------------------------------------------
void DHWMTR_IVL::wmt_SetPrior() const 		// copy to prior
{
	BYTE* d =
		((BYTE*)this + (offsetof(DHWMTR, prior) - offsetof(DHWMTR, curr)));
	memcpy(d, this, sizeof(DHWMTR_IVL));
}	// DHWMETER_IVL::wmt_SetPrior
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
	float wtk_startMin;		// tick start time (minutes from hour beg)
	double wtk_whUse;		// total tick hot water draw at all water heaters, gal
	float wtk_tInletX;		// post-DWHR / post-SSF cold water temperature for this tick, F
							//   = DHWSYS.ws_tInlet if no DWHR and ws_SSF = 0
							//   inlet temp for DHWSOLARSYS if any
	int wtk_nHRDraws;		// # of DHWHEATREC draws during this tick
	float wtk_volRL;		// DHWLOOP return flow for this tick, gal
							//   iff loop returns to water heater
	float wtk_tRL;			// DHWLOOP loop return temp, F
	float wtk_qLossNoRL;	// additional non-loop losses (e.g. branch), Btu
	double wtk_volIn;		// total tick inlet vol, gal (not including wtk_volRL)
							//   = non-loop draws reduced per mixdown
							//   = primary heater draw when DHWLOOPHEATER is present
	float wtk_qDWHR;		// DWHR heat added, Btu
	float wtk_qSSF;			// ws_SSF heat added, Btu


	DHWTICK() { wtk_Init(); }
	DHWTICK(int iTk) { wtk_Init( float( iTk*Top.tp_tickDurMin)); }
	void wtk_Init( float startMin=0.f, double whUseTick=0., float tInlet=50.f)
	{	memset(this, 0, sizeof(DHWTICK));	// 0 everything
		wtk_startMin = startMin;			// set specific mbrs
		wtk_whUse = whUseTick;
		wtk_tInletX = tInlet;
	}
	void wtk_Accum( const DHWTICK& s, double mult);
	float wtk_DrawTot(float tOut, float tInlet, float tMains, float& tInletMix);
	double wtk_DrawTotM(float tOut, float& tInletMix, float tInlet = -1.f) const;
	double wtk_DrawTotMX(float tOut, float tInlet, float tMains, float& tInletMix) const;
	void wtk_ApplySSF(float SSF, float tUse);
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
//=============================================================================
float DHWTICK::wtk_DrawTot(		// tick draw
	float tOut,				// assumed heater output temp, F
	float tInletWH, 		// water heater inlet temp, F
							//   from e.g. mains, DWHR, solar
	float tMains,			// current mains temp, F
							//   from weather file or user expression
							//   needed iff mixdown occurs due to tInlet > tOut
	float& tInletMix)		// returned: mixed inlet temp, F
							//   (combined loop return and inlet)

// sets wtk_volIn = inlet flow (not including loop)

// returns total WH draw volume for tick (including any loop flow), gal
{

	wtk_volIn = wtk_whUse;	// use at WH due to fixture draw
							//  (already modified re any DWHR)

	// additional draws to represent jacket losses, T24DHW branch losses
	if (wtk_qLossNoRL > 0.f)
	{	float deltaT = max(1., tOut - wtk_tInletX);	// temp rise, F (prevent x/0)
		wtk_volIn += wtk_qLossNoRL / (waterRhoCp * deltaT);
	}

	// mix entering water down to tOut
	//  solar source can be too hot
	if (tInletWH > tOut)
	{	float fMix = DHWMixF(tOut, tInletWH, tMains);
		wtk_volIn *= fMix;
		tInletMix = tOut;
	}
	else
		tInletMix = tInletWH;
	
	// mix loop return flow
	//  loops losses cannot be met by solar
	if (wtk_volRL > 0.)
	{	float drawTot = wtk_volIn + wtk_volRL;
		tInletMix = (wtk_volIn * tInletMix + wtk_volRL * wtk_tRL) / drawTot;
		return drawTot;
	}
	else
		return wtk_volIn;

}		// DHWTICK::wtk_DrawTot
//-----------------------------------------------------------------------------
double DHWTICK::wtk_DrawTotM(		// tick draw
	float tOut,						// assumed heater output temp, F
	float& tInletMix,				// returned: mixed inlet temp
									//   (combined loop return and inletX)
	float tInlet /*=-1.f*/) const	// inlet temp, F
									//   if <0, use wtk_tInletX
									//   else from e.g. solar or primary heater
// returns draw volume for tick, gal
{
	if (tInlet < 0.f)
		tInlet = wtk_tInletX;

	float drawUse = wtk_whUse;


	if (wtk_qLossNoRL != 0.f)
	{
		float deltaT = max(1., tOut - tInlet);
		// temp rise, F max( 1, dT) to prevent x/0
		drawUse += wtk_qLossNoRL / (waterRhoCp * deltaT);
	}

	double drawTot = drawUse + wtk_volRL;
	tInletMix = drawTot <= 0.
		? tInlet
		: (drawUse * tInlet + wtk_volRL * wtk_tRL) / drawTot;
	return drawTot;

}		// DHWTICK::wtk_DrawTotM
//-----------------------------------------------------------------------------
double DHWTICK::wtk_DrawTotMX(		// tick draw
	float tOut,						// assumed heater output temp, F
	float tInletWH, 		// water heater inlet temp, F
							//   from e.g. mains, DWHR, solar
	float tMains,			// current mains temp, F
							//   from weather file or user expression
							//   needed iff mixdown occurs due to tInlet > tOut
	float& tInletMix) const				// returned: mixed inlet temp, F
									//   (combined loop return and inlet)

// sets wtk_volIn = inlet flow (not including loop)

// returns total WH draw volume for tick (including any loop flow), gal
{

	float volIn = wtk_whUse;	// use at WH due to fixture draw

	// additional draws to represent jacket losses, T24DHW branch losses
	if (wtk_qLossNoRL > 0.f)
	{
		float deltaT = max(1., tOut - tMains);	// temp rise, F (prevent x/0)
		volIn += wtk_qLossNoRL / (waterRhoCp * deltaT);
	}

	// mix entering water down to tOut
	//  solar source can be too hot
	if (tInletWH > tOut)
	{
		float fMix = DHWMixF(tOut, tInletWH, tMains);
		volIn *= fMix;
		tInletMix = tOut;
	}
	else
		tInletMix = tInletWH;

	// mix loop return flow
	//  loops losses cannot be met by solar
	if (wtk_volRL > 0.)
	{
		float drawTot = volIn + wtk_volRL;
		tInletMix = (volIn * tInletMix + wtk_volRL * wtk_tRL) / drawTot;
		return drawTot;
	}
	else
		return volIn;

}		// DHWTICK::wtk_DrawTotMX
//-----------------------------------------------------------------------------
void DHWTICK::wtk_ApplySSF(		// apply external solar savings fraction
	float SSF,	// solar savings fraction	
	float tUse)	// use temp, F (nominal system output temp)
// Adjusts inlet temp per assumed solar savings fraction.
// Independent of DHWSOLARSYS model
{
	if (wtk_whUse > 0.f)
	{	float deltaT = SSF * max(0.f, tUse - wtk_tInletX);
		wtk_qSSF = wtk_whUse * deltaT * waterRhoCp;
		wtk_tInletX += deltaT;
	}
}		// DHWTICK::wtk_ApplySSF
//=============================================================================
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
//=============================================================================

//////////////////////////////////////////////////////////////////////////////
// DHWSIZEDAY / DHWSIZER: tracks largest draw days re sizing DHWSYS
//////////////////////////////////////////////////////////////////////////////
struct DHWSIZEDAY		// retains info for candiate DHW sizing day
{
	DHWSIZEDAY() {}
	DHWSIZEDAY(const VF24& v) : wzd_loadHrs(v) {}
	DHWSIZEDAY(const DHWSIZEDAY& dsd) : wzd_loadHrs(dsd.wzd_loadHrs) {}
	void wzd_Clear() { wzd_loadHrs.Clear(); }
	void wzd_EndDay() { wzd_loadHrs.SetStats(); }
	float Sum() const { return wzd_loadHrs.Sum(); }
	VF24 wzd_loadHrs;		// 24 hr load profile, Btu

};		// struct DHWSIZEDAY
//=============================================================================
struct DHWSIZER		// data and methods to support autosizing DHWSYS components
{
	DHWSIZER(DHWSYS* pDHWSYS, size_t nSizeDays=NDHWSIZEDAYS)
		: wz_pDHWSYS(pDHWSYS), wz_capSizeF( 1.f), wz_storeSizeF( 1.f),
		  wz_iSizeDay( 6), wz_maxRunHrs( 16)
	{
		wz_nSizeDays = max(nSizeDays, 1u);
		wz_topNDays = std::make_unique<DHWSIZEDAY[]>(wz_nSizeDays);
	}
	~DHWSIZER() {}
	void wz_Clear();
	DHWSYS* wz_GetDHWSYS() const { return wz_pDHWSYS; }
	void wz_SetHr(int iH, float dhwLoad);
	void wz_DoDay();
	RC wz_DeriveSize();

private:
	DHWSYS* wz_pDHWSYS;		// parent DHWSYS
	size_t wz_nSizeDays;	// # of days tracked
	DHWSIZEDAY wz_curDay;		// current day, hourly values accum'd here
	std::unique_ptr<DHWSIZEDAY[]> wz_topNDays;	// top wz_nSizeDays DHWSIZEDAYs *not sorted*

	// priority_queue of pointers into wz_topNDays
	//  top of priority_queue = smallest of wz_topNDays
	std::priority_queue< DHWSIZEDAY *, std::vector<DHWSIZEDAY*>,
		auto(*)(const DHWSIZEDAY*, const DHWSIZEDAY*)->bool > wz_sizeDays
			{ [](const DHWSIZEDAY* l, const DHWSIZEDAY* r)->bool { return l->Sum() > r->Sum(); } };

	float wz_capSizeF;		// capacity oversize factor
	float wz_storeSizeF;	// storage volume oversize factor
	UINT wz_iSizeDay;		// 0-based ith highest day (sizes capacity)
	UINT wz_maxRunHrs;		// run duration assumed on sizing day, hr
};		// struct DHWSIZER
//-----------------------------------------------------------------------------
void DHWSIZER::wz_Clear()		// reset all data
{
	wz_curDay.wzd_Clear();
	while (!wz_sizeDays.empty())
		wz_sizeDays.pop();

}	// DHWSIZER::wz_Clear
//-----------------------------------------------------------------------------
void DHWSIZER::wz_SetHr(			// track load by hour
	int iH,		// hr of day (0-23)
	float dhwLoad)		// water heating load, Btu
{
	wz_curDay.wzd_loadHrs[iH] = dhwLoad;

}		// DHWSIZER::wz_SetHr
//-----------------------------------------------------------------------------
void DHWSIZER::wz_DoDay()		// end-of-day sizing accounting
{
	wz_curDay.wzd_EndDay();

	size_t iSz = wz_sizeDays.size();
	if (iSz < wz_nSizeDays)
	{	wz_topNDays[iSz] = wz_curDay;
		wz_sizeDays.push(&wz_topNDays[ iSz]);
	}
	else
	{	DHWSIZEDAY* pTop = wz_sizeDays.top();
		float tSize = pTop->Sum();	// smallest of tracked loads
		if (wz_curDay.Sum() > tSize)	// if cur day bigger
		{	wz_sizeDays.pop();		// discard smallest
			*pTop = wz_curDay;		// replace in wz_topNDays
			wz_sizeDays.push(pTop);	// add to priority queue
		}
	}
}		// DHWSIZER::wz_DoDay
//-----------------------------------------------------------------------------
RC DHWSIZER::wz_DeriveSize()	// calc required heating and storage volume

// Note: tracked day priority_queue is destroyed during calc
//     >> Only one call allowed <<

// returns RCOK iff success
{
	RC rc = RCOK;

	DHWSYS* pWS = wz_GetDHWSYS();

	// use priority_queue actual size = insurance re (very) short runs
	size_t nSizeDaysActual = wz_sizeDays.size();
	if (nSizeDaysActual == 0)
		return pWS->orMsg(ERR, "DHWSIZER fail");


	// fill vector with pointers to topN days ordered by daily load
	//   [ 0] = highest daily total
	//   [ 1] = next
	std::vector< DHWSIZEDAY*> topN;
	topN.resize( nSizeDaysActual);		// allocate all slots
	size_t iX = nSizeDaysActual;
	while (wz_sizeDays.size() > 0)
	{	topN[ --iX] = wz_sizeDays.top();	// fill in reverse order
		wz_sizeDays.pop();
	}

	// make array of design loads
	float heatingCapTopN[NDHWSIZEDAYS];
	VZero(heatingCapTopN, NDHWSIZEDAYS);
	for (iX=0; iX<min( nSizeDaysActual, size_t( NDHWSIZEDAYS)); iX++)
	{	float loadDay = topN[ iX]->Sum();
		float heatingCap = wz_capSizeF * loadDay / float(wz_maxRunHrs);
		heatingCapTopN[iX] = heatingCap;
	}

	// idx of sizing day.  Default 6 = annual 2% approx ((6+1)/365 = .019)
	wz_iSizeDay = min(wz_iSizeDay, nSizeDaysActual - 1);
	float heatingCapDes = heatingCapTopN[wz_iSizeDay];

	// check load profiles of each hour of each topN day
	float qRunning = 0.f;	// heat required to carry through worst event
	for (DHWSIZEDAY* pSzD : topN)
	{	for (UINT iH = 0; iH < 24; ++iH)
		{	float qSupEvent = 0.f;		// this event cummulative net HW generation
			// try increasing event duration
			for (UINT iL = 0; iL < 24; ++iL)
			{	// net heat for current hour = cap - use
				float qSupHr = heatingCapDes - pSzD->wzd_loadHrs[(iH + iL) % 24];		// + = excess capacity
				qSupEvent += qSupHr;  // total so far
				if (qSupEvent > 0.f)
					break;			// total is positive = end of event
				if (qSupEvent < qRunning)
					qRunning = qSupEvent;	// largest deficit so far (< 0)
			}
		}
	}

	// apply oversize factor
	qRunning *= wz_storeSizeF;

	// required volume (based on setpoint, not use temp)
	// 	 tank volume is derived from running volume in HPWHLINK::hw_DeriveVolFromVolRunning()
	//   (applies aquastat fraction etc.)
	float volRunning = -qRunning / (waterRhoCp * (pWS->ws_tSetpointDes - pWS->ws_tInletDes));

	pWS->ws_ApplySizingResults(heatingCapDes, heatingCapTopN, volRunning);

	return rc;

}	// DHWSIZER::wz_DeriveSize
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class DHWSYS -- represents a DHW system
//                 = hot water loads + optional equipment
///////////////////////////////////////////////////////////////////////////////

DHWSYS::~DHWSYS()
{
	// omit cupfree(DMPP(ws_dayUseName));
	//  WHY: causes access exception if ws_dayUseName is string expression
	//       when there is a runtime error (works OK on normal termination).
	//       Something to do with cleanup order?
	//       Suspect general problem with string expressions?
	ws_dayUseName = nullptr;
	delete[] ws_ticks;
	ws_ticks = nullptr;
	delete[] ws_fxList;
	ws_fxList = nullptr;
	delete ws_pSizer;
	ws_pSizer = nullptr;
}		// DHWSYS::~DHWSYS
//-------------------------------------------------------------------------------
/*virtual*/ void DHWSYS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	record::Copy( pSrc);
	cupIncRef( DMPP( ws_dayUseName));   // incr reference counts of dm strings if non-NULL
										//   nop if ISNANDLE
	// assume ws_ticks, ws_fxList, and ws_pSizer are nullptr
}		// DHWSYS::Copy
//-------------------------------------------------------------------------------
/*virtual*/ DHWSYS& DHWSYS::CopyFrom(const record* pSrc, int copyName/*=1*/, int dupPtrs/*=0*/)
{
	record::CopyFrom(pSrc, copyName, dupPtrs);
	cupIncRef(DMPP(ws_dayUseName));		// incr reference counts of dm strings if non-NULL
										//   nop if ISNANDLE
	// assume ws_ticks, ws_fxList, and ws_pSizer are nullptr
	return *this;
}		// DHWSYS::CopyFrom
//-----------------------------------------------------------------------------
RC DHWSYS::ws_CkF()		// water heating system input check / default
// called at end of each DHWSYS input
{
	RC rc = RCOK;

	if (IsSet( DHWSYS_CENTRALDHWSYSI))
	{	// if served by central DHWSYS, msg disallowed inputs
		// can't use ws_HasCentral(), ref may not be resolved yet
		rc |= disallowN( "when wsCentralDHWSYS is given",
				DHWSYS_SSF, DHWSYS_CALCMODE, DHWSYS_TSETPOINT, DHWSYS_TSETPOINTLH,
				DHWSYS_TUSE, DHWSYS_TINLET, DHWSYS_LOADSHAREDHWSYSI, 0);
	}
	else if (IsSet( DHWSYS_LOADSHAREDHWSYSI))
	{	// if DHWSYS shares load, msg disallowed inputs
		rc |= disallowN( "when wsLoadShareDHWSYS is given",
				DHWSYS_CENTRALDHWSYSI, DHWSYS_DAYUSENAME, DHWSYS_HWUSE, 0);
	}

	if (IsSet(DHWSYS_SWTI))
		rc |= disallow( "when wsDHWSOLARSYS is given", DHWSYS_SSF);

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
// used during input and at runtime (re expressions)
// returns RCOK iff simulation should continue
{
	RC rc = RCOK;

	if (Top.isWarmup)
		erOp |= IGNX;	// limit-only during warmup

	rc |= limitCheckFix(DHWSYS_SSF, 0.f, .99f, erOp);
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
DHWSYSRES* DHWSYS::ws_GetDHWSYSRES() const
{	return WsResR.GetAtSafe(ss);
}	// DHWSYS::ws_GetDHWSYSRES
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

		// use temperature = temp delivered to fixtures or loop
		//  note wsTUse > wsTSetpoint causes XBU heating
		rc |= limitCheckFix(DHWSYS_TUSE, 33.f, 210.f);

		// EcoSizer design inlet (cold) water temp
		if (!IsSet(DHWSYS_TINLETDES))
			ws_tInletDes = Wfile.tMainsMinYr;
		else
			rc |= limitCheck(DHWSYS_TINLETDES, 33., 90.);

		// EcoSizer design setpoint
		if (!IsSet(DHWSYS_TSETPOINTDES))
			ws_tSetpointDes = ws_tUse;
		rc |= limitCheck(DHWSYS_TSETPOINTDES, ws_tInletDes + 20.f, 210.f);
		// note DHWHEATER::wh_HPWHInit() can override ws_tSetpointDes for fix-setpoint HPWHs

		// EcoSizer design heatpump source temp
		if (!IsSet(DHWSYS_ASHPTSRCDES))
			ws_ashpTSrcDes = Top.heatDsTDbO;	// HPWH min operating temp may limit
												//   see HPWHLINK::hw_SetHeatingCap()

		// solar water heating
		ws_pDHWSOLARSYS = SwhR.GetAtSafe(ws_swTi);		//solar system or NULL
		if (ws_pDHWSOLARSYS)
			ws_pDHWSOLARSYS->sw_wsCount++;

		ws_SSFAnnual = 0.f;
		ws_SSFAnnualSolar = ws_SSFAnnualReq = 0.;

		if (ws_wtCount > 0)
		{	DHWTANK* pWT;
			RLUPC(WtR, pWT, pWT->ownTi == ss)
				rc |= pWT->wt_Init();
		}
	
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

		// track days with max water heating load
		//   retains profile by hour for top N days during C_WSCALCMODECH_PRERUN
		//   allows use of 2% day = 365*.02 = 7th highest day
		//   Could be done for all DHWSYSs altho not needed for those w/o DHWHEATERs
		if (!pWSCentral)
			ws_pSizer = new DHWSIZER(this, 10);		// set up to track top 10 load days

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
#if 0
0			// info message deemed unnecessary, 2-20
0			if (!rc && !ws_HasCentralDHWSYS())
0			{	if (ws_whCountUseTS == 0.f)
0					ignore(DHWSYS_TSETPOINT,
0						"-- no setpoint-aware DHWHEATERs in this DHWSYS.");
0				if (ws_wlhCountUseTS == 0.f)
0					ignore(DHWSYS_TSETPOINTLH,
0						"-- no setpoint-aware DHWLOOPHEATERs in this DHWSYS.");
0			}
#endif

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
		else if (pWS->ws_loadShareDHWSYSi > 0)
			rc |= oer( "DHWSys '%s' (given by wsLoadShareDHWSYS) also specifies wsLoadShareDHWSYS.",
					pWS->name);
		else
		{	// note ws_fxCount[ 0] is 1
			//   thus ws_loadShareCount[ 0] is # of DHWSYSs in group
			for (int iEU = 0; iEU < C_DHWEUCH_COUNT; iEU++)
			{	ws_LSRSet(iEU, pWS->ws_loadShareCount[iEU], ws_fxCount[iEU]);
				pWS->ws_loadShareCount[iEU] += ws_fxCount[iEU];
			}
			// this->ws_loadShareCount set in pass 2 (above)

			RRFldCopy( pWS, DHWSYS_DAYUSENAME);
			RRFldCopy( pWS, DHWSYS_HWUSE);
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

	// Note: Top.tp_tickDurMin == 1. is checked in tp_SetCheckTimeSteps()

	return rc;	// pass 1 return
}		// DHWSYS::ws_Init
//----------------------------------------------------------------------------
#if 0
// activate if needed
RC DHWSYS::ws_RddInit()		// late pre-run initialization
// called at beg of each design day and beg of main simulation
// redundant calls OK
{
	RC rc = RCOK;

	// child 
	DHWHEATER* pWH;
	RLUPC(WhR, pWH, pWH->ownTi == ss)	// primary heaters
		rc |= pWH->wh_RddInit();
	RLUPC(WlhR, pWH, pWH->ownTi == ss)	// loop ("swing") heaters
		rc |= pWH->wh_RddInit();

	return rc;

}	// DHWSYS::ws_RddInit
#endif
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
	IVLCH ivl,	// C_IVLCH_Y, _M, _D, _H, (_S)
	float centralMult /*=1.f*/)	// central system multiplier
								//   re recursive call
// Child DHWSYSs have all ws_wXcount = 0, so subobjects not modeled
// not called subhourly
// returns RCOK iff valid calc
{	RC rc = RCOK;

	// input elec / fuel for this DHWSYS (w/o ws_mult), Btu
	ws_inElec = 0.f;
	// ws_inFuel = 0.f;	no DHWSYS fuel use

	ws_HJL = 0.f;	// jacket losses, Btu

	ws_qDWHR = 0.f;		// DWHR (DHWHEATREC) recovered heat hour total
						//   (to WHs and to fixtures)
	ws_qDWHRWH = 0.f;	// DHWR (DHWHEATREC) recovered heat hour total to WHs

	ws_qSlr = 0.f;		// DHWSOLARSYS heat contribution, this hour

	if (ivl <= C_IVLCH_D)	// if start of day (or longer)
	{
		if (Top.isBegRun || Top.tp_isBegMainSim)
		{	// intialize run totals for all child water heaters
			//  also sets up DHWHEATER -> DHWSYSRES linkage
			DHWHEATER* pWH;
			RLUPC(WhR, pWH, pWH->ownTi == ss)
				pWH->wh_InitRunTotals();
			RLUPC(WlhR, pWH, pWH->ownTi == ss)
				pWH->wh_InitRunTotals();
		}

		if (Top.tp_isBegMainSim)
		{	// reset sizing information
			if (ws_pSizer)
				ws_pSizer->wz_Clear();

			DHWSYSRES* pWSR = ws_GetDHWSYSRES();
			rc |= pWSR->wsr_Init();

			// reset annual values after autosize / warmup
			VZero(ws_drawCount, NDHWENDUSES);

			if (ws_fxList)
				for (int iFx = 0; iFx < ws_ShowerCount(); iFx++)
					ws_fxList[iFx].fx_hitCount = 0;


			// various run totals
			ws_t24WLTot = 0.;
			VZero(ws_fxUseMixTot, NDHWENDUSESXPP);
			VZero(ws_whUseTot, NDHWENDUSESXPP);

			// init solar accounting
			ws_SSFAnnualSolar = ws_SSFAnnualReq = 0.;
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
	
	// inlet temp = source cold water
	if (!IsSet( DHWSYS_TINLET))
		ws_tInlet = Wthr.d.wd_tMains;		// default=mains

	// adjusted inlet temp: initially same as mains temp
	//   modified later re DWHR, SSF, ...
	ws_tInletX = ws_tInlet;

	// runtime checks of vals possibly set by expressions
	rc |= ws_CheckVals( ERRRT | SHOFNLN);	// checks ws_SSF, ws_tUse, ws_tSetpoint

	// distribution loss multiplier
	//   SDLM = standard distribution loss multiplier
	//          depends on unit floor area
	//   DSM = distribution system multiplier
	//         depends on distribution system configuration
	ws_DLM = 1.f + (ws_SDLM-1.f)*ws_DSM;

	// Draw duration factors
	static_assert(sizeof(ws_drawDurF) / sizeof(ws_drawDurF[0]) == NDHWENDUSES, "ws_DrawDurF array size error");
	static_assert(sizeof(ws_drawWaste) / sizeof(ws_drawWaste[0]) == NDHWENDUSES, "ws_drawWaste array size error");

	// temperature-dependent end uses
	//   losses modeled by extending draw
	float drawDurFDflt = ws_WF * ws_DLM;	// can vary hourly
	static const int dhwEUList[] =
	{ 0, C_DHWEUCH_SHOWER, C_DHWEUCH_FAUCET, C_DHWEUCH_BATH, -1 };
	for (int i = 0; dhwEUList[i] >= 0; i++)
	{	int iEU = dhwEUList[i];
		if (!IsSet(DHWSYS_DRAWDURF + iEU))
			ws_drawDurF[iEU] = drawDurFDflt;
	}
	// temperature independent end uses (_CWASHR, _DWASHR)
	//   losses do not effect draw duration by default
	//   use input value or default (= 1) (see CULT)

	// ** Hot water use **
	//   2 types of user input
	//   * ws_hwUse = use for current hour (generally an expression w/ schedule)
    //   * DHWDAYUSE = collection of times and flows
	// here we combine these to derive consistent hourly total and tick-level (1 min) bins

	// init tick bins to average of hourly (initializes for hour)
	//   ws_loadShareCount[ 0] = # of DHWSYSs in group (always >= 1)
	double hwUseX = ws_hwUse / ws_loadShareCount[ 0];	// hwUse per system
	// note ws_fxUseMixLH is set in ws_EndIvl()

	ws_TickInit( hwUseX);		// initialize ticks

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
		// Account for drain water heat recovery
		if (ws_wrCount && ws_iTk0DWHR < ws_iTkNDWHR)
			rc |= ws_DoHourDWHR();		// modify tick values re DWHR
	}
	
	// externally-determined solar savings fraction
	if (ws_SSF > 0.f)
		ws_TickApplySSF();	// apply SSF (increase tick inlet temps)

	// derived hour average inlet temp from ticks
	float whUseTot = 0.f;	// total WH use per ticks (check figure)
	ws_tInletX = ws_TickAvgTInletX( whUseTot);

	if (!ws_HasCentralDHWSYS())
	{	DHWSYS* pWSChild;
		RLUPC( WsR, pWSChild, pWSChild->ws_centralDHWSYSi == ss)
		{	rc |= pWSChild->ws_DoHour( ivl, ws_mult);
			rc |= ws_AccumCentralUse( pWSChild);
		}
	}

	// draws now known -- maintain meters, totals, etc
	float mult = ws_mult * centralMult;	// overall multiplier
	rc |= ws_DoHourDrawAccounting( mult);

	DHWTANK* pWT;
	if (ws_wtCount > 0) RLUPC(WtR, pWT, pWT->ownTi == ss)
		rc |= pWT->wt_DoHour();

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
			ws_HRBL += pWL->wl_HRBL;	// branch loss, Btu
			ws_t24WL += pWL->wl_t24WL;	// T24 model branch waste loss volume, gal (info only)
			ws_volRL += pWL->wl_volRL;	// vol returned to WH(s), gal
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
	
	// Demand response (DR) hourly setup
	ws_drStatusHPWH = HPWH::DR_ALLOW;
	if (!ws_HasCentralDHWSYS())
	{	int drSig = CHN(ws_drSignal);	// decode variable choice
		ws_drStatusHPWH = ws_drMethod == C_DHWDRMETH_SCHED
			? DHWHEATER::wh_DRMapSigToDRStatus( drSig)
			: HPWH::DR_ALLOW;
	}
	else
		ws_drStatusHPWH = HPWH::DR_ALLOW;		// no DR for child DHWSYSs (no DHWHEATERs)
	
	if (ws_wpCount > 0)		// if any child DHWPUMPs
	{	// DHWPUMPs consume electricity but have no other effect
		//  note DHWLOOPPUMPs calc'd in DHWLOOP::wl_DoHour
		DHWPUMP* pWP;
		RLUPC(WpR, pWP, pWP->ownTi == ss)
			pWP->wp_DoHour(mult);
	}
	if (ws_whCount > 0.f)
	{	DHWHEATER* pWH;
		RLUPC(WhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_DoHour();

		// loop heaters
		if (ws_wlhCount > 0) RLUPC(WlhR, pWH, pWH->ownTi == ss)
			rc |= pWH->wh_DoHour();
	}

	// DHWSYS energy use
	ws_inElec += ws_parElec * BtuperWh;	// parasitics for e.g. circulation pumping
										//   associated heat gain is ignored
										//   ws_parElec variability = hourly
	// ws_inFuel += 0.f;	// no DHWSYS-level fuel use

	// accum consumption to meters
	//   child DHWHEATERs, DHWPUMPs, etc. accum also
	//   assume no additional DHWSYS use during subhr calc
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
	ws_qDWHRWH += pWSChild->ws_qDWHRWH * mult;
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

#undef ALTDRAWCSV		// define to enable alternative draw export format
						//   (re PRERUN testing, not generally useful)

#if !defined( ALTDRAWCSV)
	// write ws_ticks draw info to CSV file
	if (ws_drawCSV == C_NOYESCH_YES && !Top.isWarmup)
		ws_WriteDrawCSV();
#endif

	// accumulate water use to DHWMTRs if defined
	//   include DHWSYS.ws_mult multiplier
	if (ws_pFXhwMtr)
		ws_pFXhwMtr->curr.H.wmt_Accum(&ws_fxUseMix, 0, mult);
	if (ws_pWHhwMtr)
		ws_pWHhwMtr->curr.H.wmt_Accum(&ws_whUse, 0, mult);

	// accumulate water use to annual totals
	//    redundant if DHWMTRs are defined
	ws_fxUseMix.wmt_AccumTo(ws_fxUseMixTot);
	ws_whUse.wmt_AccumTo(ws_whUseTot);

	// track hourly load for EcoSizer sizing
	//   done for both _PRERUN and _SIM
	if (ws_pSizer)
	{	// water heating requirement, Btu
		//   based on design temps (ignore solar, DWHR, )
		float loadDHW = ws_whUse.total * (ws_tUse - ws_tInletDes) * waterRhoCp;
		// loop heating requirement, Btu
		float loadLoop = ws_volRL * (ws_tUse - ws_tRL) * waterRhoCp;
		// other losses
		float loadLoss = ws_HJL;
		if (ws_branchModel == C_DHWBRANCHMODELCH_T24DHW)
			loadLoss += ws_HRBL;	// T24DHW: branches losses modeled as heat
									// else: branch losses included in draws
		ws_pSizer->wz_SetHr(Top.iHrST, loadDHW+loadLoop+loadLoss);
	}
	
	// track draw and load peaks for sizing
	//   = max draw in ws_drawMaxDur hrs
	//   = max load in ws_loadMaxDur hrs
	if (ws_calcMode == C_WSCALCMODECH_PRERUN)
	{
		[[maybe_unused]] float drawSum = ws_drawMaxMS.vm_Sum( ws_whUse.total, &ws_drawMax);
		float whLoad = ws_whUse.total*(ws_tUse - ws_tInletX)*waterRhoCp;
		[[maybe_unused]] float loadSum = ws_loadMaxMS.vm_Sum( whLoad, &ws_loadMax);
#if defined( ALTDRAWCSV)
		// alternative format draw export
		//   supports testing of ws_drawMaxDur and ws_loadMaxDur
		if (ws_drawCSV == C_NOYESCH_YES && !Top.isWarmup)
		{
			if (ws_pFDrawCSV == NULL)
			{	// dump file name = <cseFile>_draws_ddll.csv
				const char* nameNoWS = strDeWS(strtmp(name));
				const char* fName =
					strsave(strffix2(strtprintf("%s_%s_draws_%2.2d%2.2d", InputFilePathNoExt, nameNoWS, ws_drawMaxDur, ws_loadMaxDur), ".csv", 1));
				ws_pFDrawCSV = fopen(fName, "wt");
				if (!ws_pFDrawCSV)
				{
					ws_drawCSV = C_NOYESCH_NO;	// don't try again
					err(PERR, "Draw CSV open failure for '%s'", fName);
				}
				else
				{	// headings
					fprintf(ws_pFDrawCSV, "%s,%s\n", name, Top.runDateTime);
					fprintf(ws_pFDrawCSV, "Draw=,%d,,Load=,%d\n", ws_drawMaxDur, ws_loadMaxDur);
					fprintf(ws_pFDrawCSV, "Mon,Day,Hr,Draw,Load\n");
				}
			}
			fprintf(ws_pFDrawCSV, "%d,%d,%d,%0.1f,%0.1f\n",
				Top.tp_date.month, Top.tp_date.mday, Top.iHrST + 1, drawSum, loadSum);
		}
#endif
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
void DHWSYS::ws_TickInit(			// initialize tick data for hour
	double whUseHr)		// base hw use (at water heater(s)) for hour, gal
						//   supports non-DHWUSE draws
{	
	int nTk = Top.tp_NHrTicks();
	double whUseTick = whUseHr / nTk;
	for (int iTk=0; iTk < nTk; iTk++)
		ws_ticks[ iTk].wtk_Init( iTk*Top.tp_tickDurMin, whUseTick, ws_tInletX);

	DHWHEATREC* pWR;
	RLUPC(WrR, pWR, pWR->ownTi == ss)
		pWR->wr_InitTicks();

	ws_iTk0DWHR = 9999;		// 1st/nth tick with possible DWHR
	ws_iTkNDWHR = -1;
}		// DHWSYS::ws_TickInit
//-----------------------------------------------------------------------------
void DHWSYS::ws_TickApplySSF()		// adjust tick values per ws_SSF
// does nothing if ws_SSF = 0
{
	int nTk = Top.tp_NHrTicks();
	for (int iTk = 0; iTk < nTk; iTk++)
		ws_ticks[iTk].wtk_ApplySSF(ws_SSF, ws_tUse);

}		// DHWSYS::ws_TickApplySSF
//-----------------------------------------------------------------------------
float DHWSYS::ws_TickAvgTInletX(	// average inlet temp
	float& whUseTot) const	// returned: total use
// returns all-tick average of wtk_tInletX
{
	[[maybe_unused]] RC rc = RCOK;
	whUseTot = 0.f;
	float tUseSum = 0.f;
	float tSum = 0.f;
	int nTk = Top.tp_NHrTicks();
	for (int iTk = 0; iTk < nTk; iTk++)
	{	DHWTICK& tk = ws_ticks[iTk];
		tSum += tk.wtk_tInletX;
		if (tk.wtk_whUse > 0.f)
		{	whUseTot += tk.wtk_whUse;
			tUseSum += tk.wtk_whUse * tk.wtk_tInletX;
		}
	}
	return whUseTot > 0.f
		? tUseSum / whUseTot
		: tSum / nTk;		// use tick average if no flow

}		// DHWSYS::ws_TickAvgTInletX
//-----------------------------------------------------------------------------
RC DHWSYS::ws_DoHourDWHR()		// current hour DHWHEATREC modeling (all DHWHEATRECs)
// called iff there are DWHRs and/or events that might recover heat
{
	RC rc = RCOK;

	// hour totals (init'd at beg of ws_DoHour())
	// ws_qDWHR = 0.f;			// total heat recovered by all DHWHEATREC devices, Btu
	// ws_qDWHRWH = 0.f;		// heat recovered to water heater inlet, Btu
	// ws_whUseNoHR = 0.;		// check value: hour total hot water use w/o HR, gal
								//  init'd by caller
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

		// adjusted inlet temp
		if (qRWH > 0.f && tk.wtk_whUse > 0.)
		{	tk.wtk_tInletX += qRWH / (waterRhoCp * tk.wtk_whUse);
			tk.wtk_qDWHR += qRWH;
		}

#if defined( _DEBUG)
		// tick energy balance
		float qXNoHR = (whUseNoHR+whUseOther) * waterRhoCp * (ws_tUse - ws_tInlet);
		float qX     = tk.wtk_whUse           * waterRhoCp * (ws_tUse - tk.wtk_tInletX);
		float qXHR = qX + qR;
		if (frDiff(qXHR, qXNoHR, 1.f) > .001f)
		{	printf("\nDHWSYS '%s': ws_DoHourDWHR tick balance error (md=%d)", name, multiDraw);
			if (nReDo++ < 2)
				goto reDo;		// repeat calc (debugging aid)
		}
#endif
		ws_qDWHR += qR;		// hour total heat recovered
		ws_qDWHRWH += qRWH;	// hour total heat to WH inlet
		ws_whUseNoHR += whUseNoHR;		// hour total WH use w/o HR

	}  // end tick

#if defined( _DEBUG)
	// hour average adjusted inlet and hot water temps
	float tInletX = ws_tInlet + ws_qDWHRWH / (waterRhoCp * ws_whUse.total);

	// hour energy balance
	float qXNoHR = ws_whUseNoHR * waterRhoCp * (ws_tUse - ws_tInlet);
	float qX =     ws_whUse.total * waterRhoCp * (ws_tUse - tInletX);
	float qXHR = qX + ws_qDWHR;
	if (frDiff(qXHR, qXNoHR, 1.f) > .001f)
		printf("\nDHWSYS '%s': ws_DoHourDWHR balance error (md=%d)", name, multiDraw);
#endif

	return rc;
}		// DHWSYS::ws_DoHourDWHR
//-----------------------------------------------------------------------------
RC DHWSYS::ws_AddLossesToDraws(		// assign losses to ticks (subhr)
	DHWTICK* ticksSh)	// initial tick draw for subhr
// updates tick info re loop and other losses
// results are for DHWSYS, allocated later per DHWHEATER
{
	RC rc = RCOK;

	double scaleTick = 1. / Top.tp_NHrTicks();	// allocate per tick
	double qLossNoRL = ws_HJLsh * scaleTick;	// w/o recirc: jacket
	if (ws_branchModel == C_DHWBRANCHMODELCH_T24DHW)
		qLossNoRL += ws_HRBL * scaleTick;	// T24DHW: branches losses modeled as heat
											// else: branch losses included in draws
	double volRL = ws_volRL * scaleTick;	// DHWLOOP recirc vol/tick, gal

#if 0 && defined( _DEBUG)
	double qLossTot = (ws_HRDL + ws_HJLsh) * scaleTick;		// total: DHWLOOP + jacket
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
RC DHWSYS::ws_DoSubhrStart(		// initialize for subhour
	int iTk0)		// initial tick idx for subhr
{
	RC rc = RCOK;

	DHWSYSRES* pWSR = ws_GetDHWSYSRES();
	pWSR->S.wsr_Clear();	// subhour results
							//   tick calcs accum here

	ws_HJLsh = 0.f;		// subhr jacket losses
	DHWTANK* pWT;
	if (ws_wtCount > 0) RLUPC(WtR, pWT, pWT->ownTi == ss)
	{	rc |= pWT->wt_DoSubhr(ws_tUse);
		ws_HJLsh += pWT->wt_mult * pWT->wt_qLossSh;
	}
	ws_HJL += ws_HJLsh * Top.tp_subhrDur;	// accumulate to hour, Btu

	ws_AddLossesToDraws(ws_ticks + iTk0);

	DHWHEATER* pWH;
	RLUPC(WhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrStart();

	RLUPC(WlhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrStart();

	return rc;

}		// DHWSYS::ws_DoSubhrStart
//-----------------------------------------------------------------------------
RC DHWSYS::ws_DoSubhrTick( int iTk)
{
	RC rc = RCOK;

	DHWTICK& tk = ws_ticks[iTk];
	tk.wtk_volIn = 0.;

	DHWHEATER* pWH;

	// loop heaters if any
	if (ws_wlhCount > 0.f) RLUPC(WlhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrTick(tk, 1.f / ws_wlhCount);

	ws_tOutPrimSum = 0.;
	if (ws_whCount > 0.f) RLUPC(WhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrTick(tk, 1.f / ws_whCount);
	if (ws_tOutPrimSum != 0.)
		ws_tOutPrimLT = ws_tOutPrimSum;

	// accumulate tick info to DHWSYSRES
	DHWSYSRES* pWSR = ws_GetDHWSYSRES();
	pWSR->S.wsr_AccumTick(tk, ws_tUse);

	return rc;

}		// DHWSYS::ws_DoSubhrTick
//-----------------------------------------------------------------------------
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
		{	double minHr = iTk*Top.tp_tickDurMin + 1;
			double minDay = double(60 * iHr) + minHr;
			fprintf(ws_pFDrawCSV, "%0.2f,%0.2f,", minHr, minDay);
		}
		const DHWTICK& tk = ws_ticks[iTk];
		// write to CSV w/o trailing 0s (many draws are 0, don't write 0.0000)
		fprintf(ws_pFDrawCSV, "%s,%s,%s,%s\n",
			WStrFmtFloatDTZ( ws_tInlet, 2).c_str(),
			WStrFmtFloatDTZ( tk.wtk_tInletX,2).c_str(),
			WStrFmtFloatDTZ( ws_tUse, 2).c_str(),
			WStrFmtFloatDTZ( tk.wtk_whUse, 4).c_str());
	}
	return RCOK;
}		// DHWSYS::ws_WriteDrawCSV
//----------------------------------------------------------------------------
RC DHWSYS::ws_DoSubhrEnd()
{
	RC rc = RCOK;
	DHWHEATER* pWH;

	// water heaters
	RLUPC(WhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrEnd( false);

	// loop heaters
	RLUPC(WlhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_DoSubhrEnd( true);

	return rc;

}		// DHWSYS::ws_DoSubhrEnd
//----------------------------------------------------------------------------
RC DHWSYS::ws_EndIvl(		// end-of-hour
	int ivl)		// C_IVLCH_Y, _M, _D, _H: what interval is next
// called at end of hour
{
	RC rc = RCOK;
	if (ivl > C_IVLCH_H)		// insurance: should not be called subhourly
		return rc;

	ws_fxUseMixLH.wmt_Copy(&ws_fxUseMix);

	ws_HARL = ws_HHWO + ws_HRDL + ws_HJL;		// total recovery load

	ws_SSFAnnualSolar += ws_qSlr;				// annual total solar contribution

	DHWHEATER* pWH;
	if (ws_whCount > 0.f) RLUPC(WhR, pWH, pWH->ownTi == ss)
	{
		rc |= pWH->wh_EndIvl(ivl, ws_HARL / ws_whCount, ws_mult);
#if 0
		double diff = pWH->wh_totHARL - ws_SSFAnnualReq / ws_whCount;
		if (fabs( diff) > 1.)
			printf("\nTot mismatch");
#endif
	}
	if (ws_wlhCount > 0.f) RLUPC(WlhR, pWH, pWH->ownTi == ss)
		rc |= pWH->wh_EndIvl(ivl, 0.f, ws_mult);

	// note: DHWSYS energy/water meter accum is in ws_DoHour
	//       values do not vary subhrly

	if (ivl <= C_IVLCH_D)
	{	
		if (ws_pSizer)
			ws_pSizer->wz_DoDay();		// end-of-day sizing accounting

		if (ivl == C_IVLCH_Y)
		{
			if (ws_calcMode == C_WSCALCMODECH_PRERUN)
				rc |= ws_DoEndPreRun();

			double totHARLCk = 0.;
			if (ws_whCount > 0.f) RLUPC(WhR, pWH, pWH->ownTi == ss)
				totHARLCk = pWH->wh_totHARL;

			[[maybe_unused]] float fTotHARLCk = float(totHARLCk);

			// solar savings fraction
			if (ws_pDHWSOLARSYS)
				ws_SSFAnnual = ws_SSFAnnualReq > 0.
				   ? min(1.f, float(ws_SSFAnnualSolar / ws_SSFAnnualReq))
				   : 0.f;
		}
	}

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

	if (ws_pSizer)
		ws_pSizer->wz_DeriveSize();

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
//----------------------------------------------------------------------------
RC DHWSYS::ws_ApplySizingResults(		// store sizing results
	float heatingCap,		// required primary (compressor) capacity, Btuh
							//   = heatCapTopN[ wz_iSizeDay] typically
							//     see DHWSIZER::wz_DeriveSize()
	float* heatingCapTopN,	// top NDHWSIZEDAYS required capacity, Btuh
							//   [ 0] = highest, [1] = next etc
	float volRunning)	// required running volume, gal
						// running volume = "active" volume in tank (above aquastat)
						//    see HPWHLINK::hw_DeriveVolFromVolRunning()
// returns RCOK iff success
{
	RC rc = RCOK;

	if (!IsSet(DHWSYS_HEATINGCAPDES))
		ws_heatingCapDes = ws_fxDes * heatingCap;

	VCopy(ws_heatingCapDesTopN, NDHWSIZEDAYS, heatingCapTopN);

	if (!IsSet(DHWSYS_VOLRUNNINGDES))
		ws_volRunningDes = ws_fxDes * volRunning;	// DHWHEATER derives wh_vol
													//   if this value passed via ALTER

	// copy to input record
	DHWSYS* pWSi = WSiB.GetAtSafe(ss);
	if (pWSi && pWSi != this)
		pWSi->ws_ApplySizingResults(heatingCap, heatingCapTopN, volRunning);

	return rc;
}
//============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWSYSRES_IVL / DHWSYSRES: accumulates various DHWSYS results by interval
///////////////////////////////////////////////////////////////////////////////
RC DHWSYSRES::wsr_Init(		// init (set to 0)
	IVLCH ivl /*=-1*/)	// interval to init
						//   default = all
{
	RC rc = RCOK;
	if (ivl < C_IVLCH_Y)
	{
		for (ivl = C_IVLCH_Y; ivl <= C_IVLCH_S; ivl++)
			wsr_Init(ivl);
	}
	else if (ivl <= C_IVLCH_S)
		(&Y + (ivl - C_IVLCH_Y))->wsr_Clear();
#if defined( _DEBUG)
	else
		rc = err(PWRN, "DHWSYSRES '%s': Invalid ivl %d", name, ivl);
#endif

	return rc;
}		// DHWSYSRES::wsr_Init
//-----------------------------------------------------------------------------
#if 0
void DHWSYSRES::wsr_Accum(
	IVLCH ivl,		// destination interval: hour/day/month/year.
					//   Accumulates from subhour/hour/day/month.  Not Top.ivl!
	int firstflg)	// iff TRUE, source copied to destination
{
	DHWSYSRES_IVL* dIvl = &Y + (ivl - C_IVLCH_Y);	// point destination substruct for interval
												// ASSUMES interval members ordered like DTIVLCH choices
	DHWSYSRES_IVL* sIvl = dIvl + 1;				// source: next shorter interval

	// accumulate: copy on first call (in lieu of 0'ing dIvl).
	//   Note: wmt_Init() call in doBegIvl 0s H values
	dIvl->wsr_Accum(sIvl, firstflg != 0);
}		// DHWSYSRES::wmt_Accum
#endif
//-----------------------------------------------------------------------------
/*static*/ const size_t DHWSYSRES_IVL::wsr_NFLOAT
    = 1 + (offsetof(DHWSYSRES_IVL, qXBU) - offsetof(DHWSYSRES_IVL, qLoad)) / sizeof(float);
//-----------------------------------------------------------------------------
void DHWSYSRES_IVL::wsr_Copy(
	const DHWSYSRES_IVL* s,
	float mult/*=1.f*/)
{
	if (mult == 1.f)
		memcpy(this, s, sizeof(DHWSYSRES_IVL));
	else
		VCopy(&qLoad, wsr_NFLOAT, &s->qLoad, mult);
}	// DHWMTR_IVL::wsr_Copy
//-----------------------------------------------------------------------------
void DHWSYSRES_IVL::wsr_Accum(			// accumulate
	const DHWSYSRES_IVL* sIvl,		// source
	int firstFlg,					// true iff first accum into this (beg of ivl)
	[[maybe_unused]] int lastFlg)					// true iff last accum into this (end of ivl)
{
	float mult = 1.f;
	if (firstFlg)
		wsr_Copy(sIvl, mult);
	else
		VAccum(&qLoad, wsr_NFLOAT, &sIvl->qLoad);
#if 0
	else
		VAccum(&total, wsr_NFLOAT, &sIvl->total, mult);
#endif
}		// DHWSYSRES_IVL::wsr_Accum
//-----------------------------------------------------------------------------
void DHWSYSRES_IVL::wsr_AccumTick(		// accum tick values
	const DHWTICK& tk,		// source tick
	float tLpIn)			// loop inlet temp, F
//  accum values (generally subhr) from tick
//  WHY: some tick values are derived hourly (e.g. DHWR)
//       (not with subhr loop)
//       Here tick values are accumed to subhr
{
	qLoop += tk.wtk_volRL * waterRhoCp * (tLpIn - tk.wtk_tRL);
	qLoss += tk.wtk_qLossNoRL;
	qDWHR += tk.wtk_qDWHR;
	qSSF += tk.wtk_qSSF;

}	// DHWSYSRES_IVL::wsr_AccumTick
//=============================================================================

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
			int iWX;
			for (iWX = pWU->ss - 1; iWX > 0; iWX--)
			{	const DHWUSE* pWUX = (const DHWUSE*)pWU->b->GetAtSafe(iWX);
				if (pWUX && pWUX->gud && pWUX->ownTi == ss
				 && pWUX->wu_hwEndUse == pWU->wu_hwEndUse
				 && pWUX->wu_eventID == pWU->wu_eventID)
				{	pWU->wu_drawSeqN = pWUX->wu_drawSeqN;	// part of previous event, use same seq #
					break;
				}
			}
			if (iWX == 0)
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
		rc |= require( when, DHWUSE_TEMP);
		rc |= disallow( when, DHWUSE_HOTF);
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

	int iEU = wu_hwEndUse;
	float durX = pWS->ws_drawDurF[iEU] * wu_dur;
	if (durX == 0.f || wu_flow == 0.f || mult == 0.f)
		return rc;		// nothing to do

 	if (pWS->ws_loadShareCount[ 0] > 1)
	{	// if load is being shared by more than 1 DHWSYS
		//   allocate by eventID to rotate DHWUSEs with suitable fixtures
		//   starting DHWSYS depends on jDay
		int nFx = pWS->ws_loadShareCount[ iEU];
		int EID = wu_eventID + pWS->ws_loadShareWS0[iEU];
		int iX = EID % nFx;
		if (!pWS->ws_IsLSR( iEU, iX))
			return rc;		// not handled by this DHWSYS, do nothing
	}
	
	// derive adjusted duration, min
	//   losses are represented by extended draw
	durX += pWS->ws_DrawWaste(iEU) / wu_flow;		// warmup waste
	if (durX <= 0.f)
		return rc;		// no draw (adjustment can be <0)
	if (durX > minPerDay)
	{	// duration longer than 1 day
		// warn and limit
		rc |= orMsg( WRNRT, "adjusted draw duration %0.1f min changed to maximum allowed 1440 min (24 hr)",
			durX);
		durX = minPerDay;
	}

	double begM = wu_start * 60.;	// beg time, min of day
	roundNearest( begM, .05*Top.tp_tickDurMin);	// round to avoid tiny amounts
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
	double tickDur = Top.tp_tickDurMin;	// tick duration, min
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
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// HPWHLINK: interface to Ecotope heat pump water heater model
///////////////////////////////////////////////////////////////////////////////
HPWHLINK::~HPWHLINK()		// d'tor
{
	hw_Cleanup();
}		// HPWHLINK::~HPWHLINK
//-----------------------------------------------------------------------------
void HPWHLINK::hw_Cleanup()
// un-initialize HPWHLINK, leave OK for possible re-use
// duplicate calls OK
{
	delete hw_pHPWH;
	hw_pHPWH = NULL;
	delete[] hw_HSMap;
	hw_HSMap = NULL;
	delete hw_pNodePowerExtra_W;
	hw_pNodePowerExtra_W = NULL;
	if (hw_pFCSV)
	{	fclose( hw_pFCSV);
		hw_pFCSV = nullptr;
	}

}	// HPWHLINK::hw_Cleanup
//-----------------------------------------------------------------------------
/*static*/ void HPWHLINK::hw_HPWHMessageCallback(
	const std::string message,
	void* contextPtr)
{
	((HPWHLINK*)contextPtr)->hw_HPWHReceiveMessage(message);
}		// HPWHLINK::hw_HPWHMessageCallback
//-----------------------------------------------------------------------------
void HPWHLINK::hw_HPWHReceiveMessage(const std::string message)
{
	// forward to owner
	hw_pOwner->ReceiveRuntimeMessage( message.c_str());

}		// HPWHLINK::hw_HPWHReceiveMessage
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_Init(			// 1st initialization
	record* pOwner)		// owner object (DHWHEATER, DHWSOLARSYS, ...)
{
	RC rc = RCOK;

	hw_Cleanup();		// insurance

	hw_pOwner = pOwner;		// owner linkage

	hw_tankTempSet = 0;		// force tank temp init (insurance)
							//   (see ??)

	hw_balErrCount = 0;
	hw_balErrMax = 0.;

	hw_pHPWH = new HPWH();

	hw_pHPWH->setMessageCallback(hw_HPWHMessageCallback, this);
	hw_pHPWH->setVerbosity(HPWH::VRB_reluctant);		// messages only for fatal errors

	hw_pHPWH->setMinutesPerStep(Top.tp_tickDurMin);	// minutesPerStep

	return rc;
}		// HPWHLINK::hw_Init
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_InitGeneric(		// init HPWH as generic ASHP
	float vol,		// tank volume, gal
	float EF,		// rated energy factor
	float resUse)	// HPWH "decision point" parameter
					//   default = 7.22, not understood
// initialize EF-rated generic HPWH (no preset)
{
	RC rc = RCOK;
	if (hw_pHPWH->HPWHinit_genericHPWH(
				GAL_TO_L(max( vol, 1.f)), EF, resUse) != 0)
		rc |= RCBAD;
	return rc;
}	// HPWHLINK::hw_InitGeneric
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_InitResistance(		// set up HPWH has EF-rated resistance heater
	[[maybe_unused]] WHRESTYCH resTy,	// resistance heater type (currently unused)
	float vol,			// tank volume, gal
	float EF,			// rated EF
						//   if >0, call HPWHinit_resTank
						//   else HPWHinit_resTankGeneric
	float insulR,		// insulation resistance, ft2-F/Btuh
						//   used iff EF <= 0
	float resHtPwr,		// upper resistance heat element power, W
	float resHtPwr2)	// lower resistance heat element power, W
// returns RCOK iff success
{
	RC rc = RCOK;

	int ret = EF > 0.f
		? hw_pHPWH->HPWHinit_resTank(GAL_TO_L(max(vol, 1.f)),
			EF, resHtPwr, resHtPwr2)
		: hw_pHPWH->HPWHinit_resTankGeneric(GAL_TO_L(max(vol, 1.f)),
		   insulR / 5.678f, resHtPwr, resHtPwr2);
	   
	if (ret)
		rc |= RCBAD;

	return rc;
}		// HPWHLINK::hw_InitResistance
//-----------------------------------------------------------------------------
/*static*/ int HPWHLINK::hw_HPWHInfo(
	WHASHPTYCH ashpTy,
	int& attrs)		// returned: attribute bits
					//   hwatSMALL, hwatLARGE,
// return HPWH preset, -1 if ashpTy not found
{

// table of known HPHWs 1) maps to HPWH preset  2) holds attributes
	static const WWTABLE /* { SI key, value; } */ presetTbl[] = {
	// small storage
	{ C_WHASHPTYCH_BASICINT,         hwatSMALL | HPWH::MODELS_basicIntegrated },
	{ C_WHASHPTYCH_RESTANK,          hwatSMALL | HPWH::MODELS_restankRealistic },
	{ C_WHASHPTYCH_RESTANKNOUA,      hwatSMALL | HPWH::MODELS_restankNoUA },
	{ C_WHASHPTYCH_AOSMITHSHPT50,    hwatSMALL | HPWH::MODELS_GE2012 },	// AO Smith SHPT models: base on GE2012
	{ C_WHASHPTYCH_AOSMITHSHPT66,    hwatSMALL | HPWH::MODELS_GE2012 },	//  caller must modify UA and vol
	{ C_WHASHPTYCH_AOSMITHSHPT80,    hwatSMALL | HPWH::MODELS_GE2012 },
	{ C_WHASHPTYCH_AOSMITHPHPT60,	 hwatSMALL | HPWH::MODELS_AOSmithPHPT60 },
	{ C_WHASHPTYCH_AOSMITHPHPT80,	 hwatSMALL | HPWH::MODELS_AOSmithPHPT80 },
	{ C_WHASHPTYCH_AOSMITHHPTU50,	 hwatSMALL | HPWH::MODELS_AOSmithHPTU50 },
	{ C_WHASHPTYCH_AOSMITHHPTU66,	 hwatSMALL | HPWH::MODELS_AOSmithHPTU66 },
	{ C_WHASHPTYCH_AOSMITHHPTU80,	 hwatSMALL | HPWH::MODELS_AOSmithHPTU80 },
	{ C_WHASHPTYCH_AOSMITHHPTU80DR,  hwatSMALL | HPWH::MODELS_AOSmithHPTU80_DR },
	{ C_WHASHPTYCH_AOSMITHCAHP120,   hwatSMALL | HPWH::MODELS_AOSmithCAHP120 },
	{ C_WHASHPTYCH_SANDEN40,         hwatSMALL | HPWH::MODELS_Sanden40 },
	{ C_WHASHPTYCH_GE2012,           hwatSMALL | HPWH::MODELS_GE2012 },
	{ C_WHASHPTYCH_GE2014,           hwatSMALL | HPWH::MODELS_GE2014 },
	{ C_WHASHPTYCH_GE2014_80,        hwatSMALL | HPWH::MODELS_GE2014_80 },
	{ C_WHASHPTYCH_GE2014_80DR,      hwatSMALL | HPWH::MODELS_GE2014_80DR },
	{ C_WHASHPTYCH_GE2014STDMODE,    hwatSMALL | HPWH::MODELS_GE2014STDMode },
	{ C_WHASHPTYCH_GE2014STDMODE_80, hwatSMALL | HPWH::MODELS_GE2014STDMode_80 },

	{ C_WHASHPTYCH_BWC202065,        hwatSMALL | HPWH::MODELS_BWC2020_65 },

	{ C_WHASHPTYCH_RHEEMHB50,        hwatSMALL | HPWH::MODELS_RheemHB50 },
	{ C_WHASHPTYCH_RHEEMHBDR2250,    hwatSMALL | HPWH::MODELS_RheemHBDR2250 },
	{ C_WHASHPTYCH_RHEEMHBDR4550,    hwatSMALL | HPWH::MODELS_RheemHBDR4550 },
	{ C_WHASHPTYCH_RHEEMHBDR2265,    hwatSMALL | HPWH::MODELS_RheemHBDR2265 },
	{ C_WHASHPTYCH_RHEEMHBDR4565,    hwatSMALL | HPWH::MODELS_RheemHBDR4565 },
	{ C_WHASHPTYCH_RHEEMHBDR2280,    hwatSMALL | HPWH::MODELS_RheemHBDR2280 },
	{ C_WHASHPTYCH_RHEEMHBDR4580,    hwatSMALL | HPWH::MODELS_RheemHBDR4580 },

	{ C_WHASHPTYCH_RHEEM2020PREM40, hwatSMALL | HPWH::MODELS_Rheem2020Prem40 },
	{ C_WHASHPTYCH_RHEEM2020PREM50, hwatSMALL | HPWH::MODELS_Rheem2020Prem50 },
	{ C_WHASHPTYCH_RHEEM2020PREM65, hwatSMALL | HPWH::MODELS_Rheem2020Prem65 },
	{ C_WHASHPTYCH_RHEEM2020PREM80, hwatSMALL | HPWH::MODELS_Rheem2020Prem80 },
	{ C_WHASHPTYCH_RHEEM2020BUILD40,hwatSMALL | HPWH::MODELS_Rheem2020Build40 },
	{ C_WHASHPTYCH_RHEEM2020BUILD50,hwatSMALL | HPWH::MODELS_Rheem2020Build50 },
	{ C_WHASHPTYCH_RHEEM2020BUILD65,hwatSMALL | HPWH::MODELS_Rheem2020Build65 },
	{ C_WHASHPTYCH_RHEEMHBDRBUILD80,hwatSMALL | HPWH::MODELS_Rheem2020Build80 },

	{ C_WHASHPTYCH_STIEBEL220E,      hwatSMALL | HPWH::MODELS_Stiebel220E },
    { C_WHASHPTYCH_SANDEN40,         hwatSMALL | HPWH::MODELS_Sanden40 },
	{ C_WHASHPTYCH_SANDEN80,         hwatSMALL | HPWH::MODELS_Sanden80 },
	{ C_WHASHPTYCH_SANDEN120,        hwatSMALL | HPWH::MODELS_Sanden120 },

	{ C_WHASHPTYCH_GENERIC1,         hwatSMALL | HPWH::MODELS_Generic1 },
	{ C_WHASHPTYCH_GENERIC2,         hwatSMALL | HPWH::MODELS_Generic2 },
	{ C_WHASHPTYCH_GENERIC3,         hwatSMALL | HPWH::MODELS_Generic3 },
	{ C_WHASHPTYCH_UEF2GENERIC,      hwatSMALL | HPWH::MODELS_UEF2generic },
	{ C_WHASHPTYCH_WORSTCASEMEDIUM,  hwatSMALL | HPWH::MODELS_UEF2generic },	// alias (testing aid)
	{ C_WHASHPTYCH_AWHSTIER3GENERIC40, hwatSMALL | HPWH::MODELS_AWHSTier3Generic40 },
	{ C_WHASHPTYCH_AWHSTIER3GENERIC50, hwatSMALL | HPWH::MODELS_AWHSTier3Generic50 },
	{ C_WHASHPTYCH_AWHSTIER3GENERIC65, hwatSMALL | HPWH::MODELS_AWHSTier3Generic65 },
	{ C_WHASHPTYCH_AWHSTIER3GENERIC80, hwatSMALL | HPWH::MODELS_AWHSTier3Generic80 },

// large
	{ C_WHASHPTYCH_SANDENGS3,       hwatLARGE | HPWH::MODELS_Sanden_GS3_45HPA_US_SP },
	{ C_WHASHPTYCH_COLMACCXV5_SP,   hwatLARGE | HPWH::MODELS_ColmacCxV_5_SP },
	{ C_WHASHPTYCH_COLMACCXA10_SP,  hwatLARGE | HPWH::MODELS_ColmacCxA_10_SP },
	{ C_WHASHPTYCH_COLMACCXA15_SP,  hwatLARGE | HPWH::MODELS_ColmacCxA_15_SP },
	{ C_WHASHPTYCH_COLMACCXA20_SP,  hwatLARGE | HPWH::MODELS_ColmacCxA_20_SP },
	{ C_WHASHPTYCH_COLMACCXA25_SP,  hwatLARGE | HPWH::MODELS_ColmacCxA_25_SP },
	{ C_WHASHPTYCH_COLMACCXA30_SP,  hwatLARGE | HPWH::MODELS_ColmacCxA_30_SP },

	{ C_WHASHPTYCH_COLMACCXV5_MP,   hwatLARGE | HPWH::MODELS_ColmacCxV_5_MP },
	{ C_WHASHPTYCH_COLMACCXA10_MP,  hwatLARGE | HPWH::MODELS_ColmacCxA_10_MP },
	{ C_WHASHPTYCH_COLMACCXA15_MP,  hwatLARGE | HPWH::MODELS_ColmacCxA_15_MP },
	{ C_WHASHPTYCH_COLMACCXA20_MP,  hwatLARGE | HPWH::MODELS_ColmacCxA_20_MP },
	{ C_WHASHPTYCH_COLMACCXA25_MP,  hwatLARGE | HPWH::MODELS_ColmacCxA_25_MP },
	{ C_WHASHPTYCH_COLMACCXA30_MP,  hwatLARGE | HPWH::MODELS_ColmacCxA_30_MP },

	{ C_WHASHPTYCH_NYLEC25A_SP,     hwatLARGE | HPWH::MODELS_NyleC25A_SP },
	{ C_WHASHPTYCH_NYLEC60A_SP,     hwatLARGE | HPWH::MODELS_NyleC60A_SP  },
	{ C_WHASHPTYCH_NYLEC90A_SP,     hwatLARGE | HPWH::MODELS_NyleC90A_SP  },
	{ C_WHASHPTYCH_NYLEC125A_SP,    hwatLARGE | HPWH::MODELS_NyleC125A_SP },
	{ C_WHASHPTYCH_NYLEC185A_SP,    hwatLARGE | HPWH::MODELS_NyleC185A_SP  },
	{ C_WHASHPTYCH_NYLEC250A_SP,    hwatLARGE | HPWH::MODELS_NyleC250A_SP },

	{ C_WHASHPTYCH_NYLEC60AC_SP,     hwatLARGE | HPWH::MODELS_NyleC60A_C_SP  },
	{ C_WHASHPTYCH_NYLEC90AC_SP,     hwatLARGE | HPWH::MODELS_NyleC90A_C_SP  },
	{ C_WHASHPTYCH_NYLEC125AC_SP,    hwatLARGE | HPWH::MODELS_NyleC125A_C_SP },
	{ C_WHASHPTYCH_NYLEC185AC_SP,    hwatLARGE | HPWH::MODELS_NyleC185A_C_SP  },
	{ C_WHASHPTYCH_NYLEC250AC_SP,    hwatLARGE | HPWH::MODELS_NyleC250A_C_SP },

	{ C_WHASHPTYCH_MITSU_QAHVN136TAUHPB_SP, hwatLARGE | HPWH::MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP },

	// { C_WHASHPTYCH_NYLEC25A_MP,     hwatLARGE | HPWH::MODELS_NyleC25A_MP }, not available
	{ C_WHASHPTYCH_NYLEC60A_MP,     hwatLARGE | HPWH::MODELS_NyleC60A_MP },
	{ C_WHASHPTYCH_NYLEC90A_MP,     hwatLARGE | HPWH::MODELS_NyleC90A_MP },
	{ C_WHASHPTYCH_NYLEC125A_MP,    hwatLARGE | HPWH::MODELS_NyleC125A_MP },
	{ C_WHASHPTYCH_NYLEC185A_MP,    hwatLARGE | HPWH::MODELS_NyleC185A_MP },
	{ C_WHASHPTYCH_NYLEC250A_MP,    hwatLARGE | HPWH::MODELS_NyleC250A_MP },

	{ C_WHASHPTYCH_NYLEC60AC_MP,     hwatLARGE | HPWH::MODELS_NyleC60A_C_MP },
	{ C_WHASHPTYCH_NYLEC90AC_MP,     hwatLARGE | HPWH::MODELS_NyleC90A_C_MP },
	{ C_WHASHPTYCH_NYLEC125AC_MP,    hwatLARGE | HPWH::MODELS_NyleC125A_C_MP },
	{ C_WHASHPTYCH_NYLEC185AC_MP,    hwatLARGE | HPWH::MODELS_NyleC185A_C_MP },
	{ C_WHASHPTYCH_NYLEC250AC_MP,    hwatLARGE | HPWH::MODELS_NyleC250A_C_MP },

	{ C_WHASHPTYCH_RHEEM_HPHD60HNU_201_MP,    hwatLARGE | HPWH::MODELS_RHEEM_HPHD60HNU_201_MP },
	{ C_WHASHPTYCH_RHEEM_HPHD60VNU_201_MP,    hwatLARGE | HPWH::MODELS_RHEEM_HPHD60VNU_201_MP },
	{ C_WHASHPTYCH_RHEEM_HPHD135HNU_483_MP,   hwatLARGE | HPWH::MODELS_RHEEM_HPHD135HNU_483_MP },
	{ C_WHASHPTYCH_RHEEM_HPHD135VNU_483_MP,   hwatLARGE | HPWH::MODELS_RHEEM_HPHD135VNU_483_MP },

	{ C_WHASHPTYCH_SCALABLE_SP,    hwatLARGE | HPWH::MODELS_TamScalable_SP },
	{ C_WHASHPTYCH_SCALABLE_MP,    hwatLARGE | HPWH::MODELS_Scalable_MP },

	{ 32767,                         HPWH::MODELS(-1) }  };

	SI tableVal = presetTbl->lookup(ashpTy);
	int preset;
	if (tableVal < 0)
	{	attrs = 0;
		preset = -1;
	}
	else
	{	attrs = tableVal & hwatMASK;
		preset = tableVal & ~hwatMASK;
	}

	return preset;
}		// HPWHLINK::hw_HPWHInfo
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_InitPreset(		// set up HPWH from model type choice
	WHASHPTYCH ashpTy)		// type choice (C_WHASHPTYCH_xxx)
							//   maps to HPWH preset
// returns RCOK iff success
{
	if (!hw_pHPWH)
		return RCBAD;	// must call hw_Init() first

	RC rc = RCOK;
	
	float volX = -1.f;		// alternative volume
	float UAX = -1.f;		// alternative UA, Btuh/F

	int attrs;
	HPWH::MODELS preset = HPWH::MODELS( hw_HPWHInfo( ashpTy, attrs));	// HPWH "preset"
										// predefined type that determines most model parameters

	// alternative values for some special cases
	if (ashpTy == C_WHASHPTYCH_AOSMITHSHPT50)
	{	// preset = HPWH::MODELS_GE2012;	// AO Smith SHPT models: base on GE2012
		volX = 45.f;			// ... and change vol / UA
		UAX = 2.9f;
	}
	else if (ashpTy == C_WHASHPTYCH_AOSMITHSHPT66)
	{	// preset = HPWH::MODELS_GE2012;
		volX = 63.9f;
		UAX = 3.4f;
	}
	else if (ashpTy == C_WHASHPTYCH_AOSMITHSHPT80)
	{	// preset = HPWH::MODELS_GE2012;
		volX = 80.7f;
		UAX = 4.7f;
	}
	if (hw_pHPWH->HPWHinit_presets(preset) != 0)
		rc |= RCBAD;
	// force modify tank size (avoids tankSizeFixed error)
	if (volX > 0.f && hw_pHPWH->setTankSize(volX, HPWH::UNITS_GAL, true) != 0)
		rc |= RCBAD;
	if (UAX >= 0.f && hw_pHPWH->setUA(UAX, HPWH::UNITS_BTUperHrF) != 0)
		rc |= RCBAD;

	return rc;
}		// HPWHLINK::hw_InitPreset
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_InitTank(	// init HPWH for use as storage tank
	float vol)		// tank volume, gal
// inits HPWH tank of specified size
// note UA defaults to constant value, probably wrong
// use hw_AdjustUAIf() to set UA
// returns RCOK iff success
{
	RC rc = RCOK;

	HPWH::MODELS preset = HPWH::MODELS_StorageTank;

	if (hw_pHPWH->HPWHinit_presets(preset) != 0)
		rc |= RCBAD;
	if (hw_pHPWH->setTankSize(vol, HPWH::UNITS_GAL) != 0)
		rc |= RCBAD;
	return rc;
}		// HPWHLINK::hw_InitTank
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_AdjustUAIf(	// adjust tank UA
	float UA,			// tank UA, Btuh/F; derived from insulR if < 0
	float insulR,		// overall tank insulation resistance, ft2-F/Btuh
						//    includes surface resistance
						//    ignored if <0
	float tankCount /*=1.f*/)		// # of tanks
// NOP if UA and insulR both < 0
// uses current tank surface area (derived from volume)
// returns RCOK iff success
{
	RC rc = RCOK;
	if (insulR >= 0.f && UA < 0.f)
	{	// get total surface area
		float surfA = hw_GetTankSurfaceArea( tankCount);
		UA = surfA / max(insulR, .68f);
	}
	if (UA >= 0.f)
	{	if (hw_pHPWH->setUA(UA, HPWH::UNITS_BTUperHrF) != 0)
			rc |= RCBAD;
	}
	return rc;
}		// HPWHLINK::hw_AdjustUAIf
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_InitFinalize(		// final initialization actions
	float inHtSupply,	// supply fractional height, -1 = don't set
	float inHtLoopRet)	// loop return fractional height, -1 = don't set
{
	RC rc = RCOK;

	// tank inlet placement
	if (inHtSupply >= 0.f)
		hw_pHPWH->setInletByFraction( inHtSupply);
	if (inHtLoopRet >= 0.f)
		hw_pHPWH->setInlet2ByFraction( inHtLoopRet);

	// make map of heat sources = idxs for hw_HPWHUse[]
	// WHY: HPWH model frequently uses 3 heat sources in
	//      preset-specific order
	hw_HSCount = hw_pHPWH->getNumHeatSources();
	delete hw_HSMap;		// insurance: delete any pre-existing
	if (hw_HSCount == 0)
		hw_HSMap = NULL;
	else
	{	hw_HSMap = new int[hw_HSCount];
		if (!hw_HasCompressor())
			// no compressor, all use is primary
			VSet(hw_HSMap, hw_HSCount, 1);
		else for (int iHS = 0; iHS < hw_HSCount; iHS++)
		{
			HPWH::HEATSOURCE_TYPE hsTy = hw_pHPWH->getNthHeatSourceType(iHS);
			hw_HSMap[iHS] = hsTy != HPWH::TYPE_resistance;
			// resistance use -> hw_inElec[ 0]
			// all other (compressor) -> hw_inElec[ 1]
		}
	}

	// nominal tank heat content, kWh
	hw_tankHCNominal = KJ_TO_KWH(40. * HPWH::DENSITYWATER_kgperL * HPWH::CPWATER_kJperkgC
		* hw_pHPWH->getTankSize());

	return rc;

}	// HPWHLINK::hw_InitFinalize
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_SetHeatingCap(			// set heating capacity
	float heatingCap,		// design heating capacity, Btuh
	float ashpTSrcDes,		// source air temp, F
	float tInletDes,		// cold water inlet, F
	float tUseDes)			// hot water use temp, F
// assumes hw_pHPWH is scalable
// sets both compressor and resistance (if any) power
// returns RCOK iff success
{
	RC rc = RCOK;

	int hpwhRet = 0;
	double minT = hw_pHPWH->getMinOperatingTemp(HPWH::UNITS_F);
	if (minT == double(HPWH::HPWH_ABORT))
		++hpwhRet;
	else
	{	if (ashpTSrcDes < minT)
			ashpTSrcDes = minT;		// constrain source air temp to
									//  HPWH lockout temp

		// set compressor capacity at design conditions
		hpwhRet = hw_pHPWH->setCompressorOutputCapacity(
					heatingCap,
					ashpTSrcDes,	// design source air temp, F
					tInletDes,		// inlet temp, F
					tUseDes,		// outlet temp, F
					HPWH::UNITS_BTUperHr, HPWH::UNITS_F);

		if (!hpwhRet)
			// set capacity of all reistance elements to design cap
			//   (handles e.g. possible low-temp lockout)
			hpwhRet = hw_pHPWH->setResistanceCapacity( heatingCap, 0, HPWH::UNITS_BTUperHr);
	}
	if (hpwhRet)
		// unexpected HPWH error (inconsistent HPWH::isHPWHScalable() logic?)
		//   isHPWHScalable() checked in wh_HPWHInit()
		rc = hw_pOwner->oer("Program error (HPWHLINK::hw_SetHeatingCap): HPWH error");

	return rc;

}	// HPWHLINK::hw_SetHeatingCap
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_GetHeatingCap(			// get heating capacity
	float& heatingCap,		// returned: design heating capacity, Btuh
	float ashpTSrcDes,		// source air temp, F (used if conpressor)
	float tInletDes,		// cold water inlet, F (used if conpressor)
	float tUseDes) const	// hot water use temp, F (used if conpressor)
// returns RCOK and heatingCap iff success
//    else RCBAD (heatingCap = 0)
{
	RC rc = RCOK;
	heatingCap = 0.f;
	if (!hw_pHPWH)
		return RCBAD;		// bad setup
	double cap = 0.;
	if (hw_pHPWH->hasACompressor())
	{	double minT = hw_pHPWH->getMinOperatingTemp(HPWH::UNITS_F);
		if (minT == double(HPWH::HPWH_ABORT))
			rc = RCBAD;
		else
		{
			if (ashpTSrcDes < minT)
				ashpTSrcDes = minT;		// constrain source air temp to
										//  HPWH lockout temp

			cap = hw_pHPWH->getCompressorCapacity(
							ashpTSrcDes,	// design source air temp, F
							tInletDes,		// inlet temp, F
							tUseDes,		// outlet temp, F
							HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
			if (cap == double(HPWH::HPWH_ABORT))
				rc = RCBAD;
		}
	}
	else
	{	// resistance: return capacity of largest heating element
		//   TODO: recode to return max when HPWH is fixed
		int nRE = hw_pHPWH->getNumResistanceElements();
		for (int iRE = 0; iRE < nRE; iRE++)
		{	double capx = hw_pHPWH->getResistanceCapacity(iRE, HPWH::UNITS_BTUperHr);
			if (capx != double(HPWH::HPWH_ABORT) && capx > cap)
				cap = capx;
		}
	}
	if (!rc)
		heatingCap = float(cap);
	return rc;
}		// HPWHLINK::hw_GetHeatingCap
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_GetInfo(		// return HPWH tank values
	float& vol,		// vol, gal
	float& UA,		// UA, Btuh/F
	float& insulR,	// insulR, ft2-F/Btuh
	float tankCount /*=1.f*/) const		// # of tanks
// returns RC iff success
{
	RC rc = RCOK;
	vol = hw_pHPWH->getTankSize(HPWH::UNITS_GAL);
	double UAd;
	if (hw_pHPWH->getUA(UAd, HPWH::UNITS_BTUperHrF) != 0)
		rc |= RCBAD;
	UA = float(UAd);

	// surface area: account for multiple tanks
	float surfA = hw_GetTankSurfaceArea( tankCount, vol);

	insulR = UA > 0. ? surfA / UA : 1.e6f;

	return rc;

}		// HPWHLINK::hw_GetInfo
//-----------------------------------------------------------------------------
float HPWHLINK::hw_GetTankSurfaceArea(		// tank surface area
	float tankCount /*=-1.f*/,	// # of tanks
	float vol /*=-1*/) const	// tank volume, gal
								//   if < 0, use HPWH getTankVol
// returns total tank surface area, ft2
//   (accounting for possible multiple tanks)
{
	if (vol < 0)
		vol = hw_pHPWH->getTankSize(HPWH::UNITS_GAL);
	float volPerTank = vol / tankCount;
	double surfAPerTank = HPWH::getTankSurfaceArea(volPerTank, HPWH::UNITS_GAL, HPWH::UNITS_FT2);
	float surfA = surfAPerTank * tankCount;
	return surfA;
}		// HPWHLINK::hw_GetTankSurfaceArea
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DeriveVolFromVolRunning(		// calc required volume from running vol
	float volRunning,		// required running volume, gal
	float heatingCap,		// compressor design capacity, Btuh
	float tempRise,			// design temperature rise, F
	float& totVol) const	// returned: derived total vol, gal
// Do not call if !hasACompressor
// Does not actually set volume
// returns RCOK iff success
{
	RC rc = RCOK;

	// retrieve tank volume fractions
	//   apply insurance (crash-proof) limits
	double aquaFract;	// fraction of volume below aquastat
	double useableFract;	// fraction of volume that is useable
	if (hw_pHPWH->getSizingFractions(aquaFract, useableFract) != 0)
	{	rc |= RCBAD;
		aquaFract = .4f;		// plausible values
		useableFract = .9f;
	}
	useableFract = bracket(.6, useableFract, 1.);
	double unuseableFract = 1. - useableFract;
	aquaFract = bracket(unuseableFract + .1, aquaFract, .75);

	// total volume req'd based on aquastat position
	//    Running vol is vol above aquastat
	float totVolRun = float(volRunning / (1. - aquaFract));

	// total volume req'd based on minimum run time (avoid short cycle)
	//   Determine vol of water heated in minimum compressor cycle.
	//   Usable volume below aquastat must be >= to 
	float runHrMin = hw_pHPWH->getCompressorMinRuntime( HPWH::UNITS_HR);		// minimum compressor run time, hr
	float volCycMin = heatingCap * runHrMin / (waterRhoCp * max(tempRise, 10.f));
	float totVolCyc = volCycMin / (aquaFract - unuseableFract);

	totVol = max(totVolRun, totVolCyc);		// caller must set volume

	return rc;

}		// HPWHLINK::hw_DeriveVolFromVolRunning
//-----------------------------------------------------------------------------
void HPWHLINK::hw_InitTotals()		// run init
// start-of-run initialization totals, error counts, ...
// called at beg of warmup and run
{
	hw_fMixUse = hw_fMixRL = 1.f;
}		// HPWLINK::hw_InitTotals
//-----------------------------------------------------------------------------
bool HPWHLINK::hw_HasCompressor() const
// returns true iff HPWH has a compressor (= is a heat pump)
{
	return hw_pHPWH && hw_pHPWH->hasACompressor();
}		// HPWHLINK::hw_HasCompressor
//-----------------------------------------------------------------------------
bool HPWHLINK::hw_IsSetpointFixed() const
{
	return hw_pHPWH && hw_pHPWH->isSetpointFixed();
}		// HPWHLINK::hw_IsSetpointFixed
//-----------------------------------------------------------------------------
float HPWHLINK::hw_GetTankVol() const	// returns current tank size, gal
{
	return hw_pHPWH->getTankSize(HPWH::UNITS_GAL);
}		// HPWHLINK::hw_GetTankVol
//-----------------------------------------------------------------------------
void HPWHLINK::hw_SetNQTXNodes(int nQTXNodes)
{
	hw_nQTXNodes = nQTXNodes;

}		// HPWHLINK::hw_SetNQTXNodes
//-----------------------------------------------------------------------------
double HPWHLINK::hw_GetTankAvgTemp(		// average temp of range of tank nodes
	int iNode0 /*=0*/,			// starting node
	int nNodes /*=999*/) const	// # of nodes to include
								//   if <0, include nodes below iNode0
// returns average tank temp, F for node range
{
	int nodeCount = hw_pHPWH->getNumNodes();
	iNode0 = bracket(0, iNode0, nodeCount - 1);			// 1st node
	int iNodeN = bracket(-1, iNode0 + nNodes, nodeCount);	// 1 beyond last node
	int incr = nNodes < 0 ? -1 : 1;

	double T = 0.;
	for (int iN=iNode0; iN != iNodeN; iN+=incr)
		T += hw_pHPWH->getTankNodeTemp(iN, HPWH::UNITS_C);
	T /= max(1, abs(iNodeN - iNode0));
	return DegCtoF(T);
}		// HPWHLINK::hw_GetTankAvgTemp
//-----------------------------------------------------------------------------
double HPWHLINK::hw_GetEstimatedTOut() const
// returns estimate of tank output temp, F
//   = current top node temp (no consideration of draw etc.)
{
	int iNodeTop = hw_pHPWH->getNumNodes() - 1;
	return hw_pHPWH->getTankNodeTemp(iNodeTop, HPWH::UNITS_F);
}		// HPWHLINK::hw_GetEstimatedTOut
//-----------------------------------------------------------------------------
void HPWHLINK::hw_SetQTX(
	float qTX)		// additional heat to be added for current tick
{
	hw_qTXTick = qTX;

}		// HPWHLINK::hw_SetQTX
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DoHour(		// hourly HPWH calcs
	float& tSetpoint)	// setpoint for current hour, F
						//  returned updated to reflect HPWH
						//    restrictions if any
// Does HPWH setup etc that need not be done subhourly
// returns RCOK iff success
{
	RC rc = RCOK;

	if (Top.tp_isBegMainSim)
		hw_balErrCount = 0;

	// setpoint temp: ws_tUse has hourly variability
	//   some HPWHs (e.g. Sanden) have fixed setpoints, don't attempt
	if (!hw_pHPWH->isSetpointFixed())
	{	double tSetpointMax;
		std::string whyNot;		// HPWH explanatory text, ignored
		bool bSPP = hw_pHPWH->isNewSetpointPossible(tSetpoint, tSetpointMax, whyNot, HPWH::UNITS_F);
		// silently limit to max acceptable
		//   if HPWH has resistance, max = 212
		float tSetpointX = bSPP ? tSetpoint : tSetpointMax;		
		hw_pHPWH->setSetpoint(tSetpointX, HPWH::UNITS_F);
	}

	// retrieve resulting setpoint after HPWH restrictions
	tSetpoint = hw_pHPWH->getSetpoint(HPWH::UNITS_F);
	if (hw_tHWOut == 0.f)
		hw_tHWOut = tSetpoint;		// initial guess for HW output temp
									//   updated every substep with nz draw
	if (!hw_tankTempSet)
	{	// initialize tank temp on 1st call
		//   must be done after setting HPWH setpoint (=ws_tSetpoint)
		//   (ws_tSetpoint may be expression)
		if (hw_pHPWH->resetTankToSetpoint())
			rc |= RCBAD;
		++hw_tankTempSet;
	}

	return rc;

}	// HPWHLINK::hw_DoHour
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DoSubhrStart(	// HPWH subhour start
	float tEx,					// tank surround temperature, F
	float tASHPSrc /*=-999.f*/)	// heat pump source air temperature, F
								//   -999 = "unused" (e.g. for tank)
//
// returns RCOK iff success
{
	RC rc = RCOK;

	// subhr totals
	hw_qEnv = 0.;		// heat removed from environment, kWh
						//   + = to water heater
	hw_qLoss = 0.;		// standby losses, kWh;  + = to surround

	hw_qHW = 0.;		// total hot water heating, kWh; always >= 0
						//   includes heat to DHWLOOP;  does not include wh_HPWHxBU

	hw_qTX = 0.;		// total extra tank heat (e.g. re solar tank)

	hw_tHWOutF = 0.;	// accum re average hot water outlet temp, F
	// hw_fMixUse, hw_fMixRL: initialized in wh_InitRunTotals(); value retained hour-to-hour

	hw_nzDrawCount = 0;		// count of ticks with draw > 0


	hw_inElec[0] = hw_inElec[1] = 0.;	// energy use totals, kWh
	hw_HPWHxBU = 0.;	// add'l resistance backup this subhour, Btu
						//   water is heated to ws_tUse if HPWH does not meet load

	// setpoint and inletT: see hw_DoHour above

	// ambient and source temps
	hw_tEx = tEx;
	hw_tASHPSrc = tASHPSrc;

#define HPWH_QBAL		// define to include sub-hour energy balance check
#if defined( HPWH_QBAL)
	// tank heat content at start
	hw_tankHCStart = KJ_TO_KWH(hw_pHPWH->getTankHeatContent_kJ());
#endif

#define HPWH_DUMP		// define to include debug CSV file
#if defined( HPWH_DUMP)
	// #define HPWH_DUMPSMALL	// #define to use abbreviated version
		// use debug dump mechanism w/o headings to log file
		//   (dump goes to external CSV file)
	hw_bWriteCSV = DbDo(dbdHPWH, dbdoptNOHDGS);
#endif
	
	return rc;
}	// HPWHLINK::hw_DoSubhrStart
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DoSubhrTick(		// calcs for 1 tick, simplified call
	int iTk,		// tick within hour, 0 .. Top.nHRTicks()-1
	float draw,		// draw for tick, gal (not gpm)
	float tInlet,	// water inlet temp, F
	float* tOut,	// returned: water outlet temp, F (0 if no draw)
	float scaleWH /*=1.f*/)	// draw scale factor
{
	DHWTICK tk(iTk);
	tk.wtk_volIn = draw;
	return hw_DoSubhrTick(tk, tInlet, scaleWH, -1.f, -1.f, tOut);
}		// HPWHLINK::hw_DoSubhrTick
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DoSubhrTick(		// calcs for 1 tick
	DHWTICK& tk,			// current tick
	float tInlet,			// current inlet water temp, F
							//   includes upstream heat recovery, solar, etc.
							//   same as tMains if no upstream mods
	float scaleWH /*=1*/,	// draw scale factor
							//   re DHWSYSs with >1 DHWHEATER
							//   *not* including hw_fMixUse or hw_fMixRL;
	float tMix /*=-1.f*/,	// target mixed water temp, F
							//   if >0, mix/XBU maintains tMix
							//   else no mix
	float tMains /*=-1.f*/,	// current mains temp, F
							//   from weather file or user expression
							//   needed iff tMix is specified
	float* pTOutNoMix/*=NULL*/,	// unmixed output temp accumulated here re DHWLOOPHEATER
								// average inlet temp
	int drStatus /*=0*/)		// demand response control signal
{
	RC rc = RCOK;

#if 0 && defined( _DEBUG)
	if (Top.tp_date.month == 7
		&& Top.tp_date.mday == 27
		&& Top.iHr == 10
		&& Top.iSubhr == 3)
		hw_pHPWH->setVerbosity(HPWH::VRB_emetic);
#endif

	bool bDoMix = tMix > 0.f;

	// draw components for tick
	double drawUse;	// use draw, gal
	double drawLoss;// pseudo-draw (gal) to represent e.g. central system branch losses
	double drawRL;	// loop flow vol for tick, gal
	double tRL;		// loop return temp, F
	double drawForTick;		// total draw, gal
	if (bDoMix)
	{	double scaleX = scaleWH * hw_fMixUse;
		drawUse = tk.wtk_whUse*scaleX;
		drawLoss = tk.wtk_qLossNoRL*scaleX / (waterRhoCp * max(1., tMix - tMains));
		tk.wtk_volIn += (drawUse + drawLoss) / scaleWH;
		drawRL = tk.wtk_volRL * scaleWH * hw_fMixRL;
		drawForTick = drawUse + drawLoss + drawRL;
		tRL = tk.wtk_tRL;
	}
	else
	{	drawForTick = drawUse = tk.wtk_volIn * scaleWH;		// multipliers??
		drawLoss = drawRL = tRL = 0.f;
	}

	// extra tank heat: passed to HPWH as vector<double>* (or NULL)
	//   used to model e.g. heat addition via solar DHW heat exchanger
	std::vector< double>* pNPX = NULL;
	if (hw_qTXTick > 0.)		// ignore tank "cooling"
	{	if (hw_pNodePowerExtra_W == NULL)
			hw_pNodePowerExtra_W = new std::vector< double>;
		double qTXkWh = hw_qTXTick / BtuperkWh;
		hw_qTX += qTXkWh;		// subhour total (kWh)
		double qTXPwr	// tick power per node, W
			= qTXkWh * 1000. / (hw_nQTXNodes * Top.tp_tickDurHr);
		hw_pNodePowerExtra_W->assign(hw_nQTXNodes, qTXPwr);
		pNPX = hw_pNodePowerExtra_W;
	}
	
	int hpwhRet = hw_pHPWH->runOneStep(
		DegFtoC(tInlet),		// inlet temp, C
		GAL_TO_L(drawForTick),	// draw volume, L
		DegFtoC(hw_tEx),		// ambient T (=tank surround), C
		DegFtoC(hw_tASHPSrc),	// heat source T, C
								//   aka HPWH "external temp"
		HPWH::DRMODES( drStatus), // DRstatus: demand response signal
		GAL_TO_L(drawRL), DegFtoC(tRL),	// 2ndary draw for DHWLOOP
										//   note drawForTick includes drawRL
		pNPX);					// additional node power (re e.g. solar tanks)

	if (hpwhRet)	// 0 means success
		rc |= RCBAD;

	hw_qEnv += hw_pHPWH->getEnergyRemovedFromEnvironment();
	hw_qLoss += hw_pHPWH->getStandbyLosses();
	float HPWHxBU = 0.f;		// add'l resistance backup, this tick, Btu
	double tOut = hw_pHPWH->getOutletTemp();	// output temp, C (0 if no draw)
	if (tOut < .01)
	{	// no draw / output temp not known
		if (pTOutNoMix)
			*pTOutNoMix = 0.;
	}
	else
	{	double tOutF = DegCtoF(tOut);	// output temp, F
		if (pTOutNoMix)
			*pTOutNoMix = tOutF;
		hw_nzDrawCount++;	// this tick has draw
		if (bDoMix)
		{	// output goes to load
			if (tOutF < tMix)
			{	// load not met, add additional (unlimited) resistance heat
				hw_fMixUse = hw_fMixRL = 1.f;
				HPWHxBU = waterRhoCp * drawForTick * (tMix - tOutF);
				hw_HPWHxBU += HPWHxBU;
				tOutF = tMix;
			}
			else
			{	// mix to obtain ws_tUse
				//   set hw_fMixUse and hw_fMixRL for next tick
				DHWMix(tMix, tOutF, tMains, hw_fMixUse);
				DHWMix(tMix, tOutF, tRL, hw_fMixRL);
			}
		}
		hw_tHWOutF += tOutF;	// note tOutF may have changed (but not tOut)

		double qHWTick = KJ_TO_KWH(		// heat added to water, kWh 
			(GAL_TO_L(drawForTick)*tOut
				- GAL_TO_L(drawForTick - drawRL)*DegFtoC(tInlet)
				- GAL_TO_L(drawRL)*DegFtoC(tRL))
			* HPWH::DENSITYWATER_kgperL
			* HPWH::CPWATER_kJperkgC);
		hw_qHW += qHWTick;		// total heat added to water for substep

#if 0
		double waterRhoCpX = KWH_TO_BTU(
			KJ_TO_KWH(
				GAL_TO_L(1.) * HPWH::DENSITYWATER_kgperL * HPWH::CPWATER_kJperkgC / 1.8));

		double qX = drawForTick * (tOutF - tInlet) * waterRhoCp;
		double qHWTickBtu = KWH_TO_BTU(qHWTick) + HPWHxBU;
		double qDiff = fabs(qX - qHWTickBtu);
		if (qDiff > .001)
			printf("\nDiff");
#endif
	}

	// energy use by heat source, kWh
	// accumulate by backup resistance [ 0] vs primary (= compressor or all resistance) [ 1]
	for (int iHS = 0; iHS < hw_HSCount; iHS++)
	{
		hw_inElec[hw_HSMap[iHS]] += hw_pHPWH->getNthHeatSourceEnergyInput(iHS);
#if 0 && defined( _DEBUG)
		// debug aid
		if (hw_pHPWH->getNthHeatSourceEnergyInput(iHS) < 0.)
			printf("\nNeg input, iHS=%d", iHS);
#endif
	}
#if defined( HPWH_DUMP)
	// tick level CSV report for testing
	int dumpUx = UNSYSIP;		// unit system for CSV values
	int hpwhOptions = dumpUx == UNSYSIP ? HPWH::CSVOPT_IPUNITS : HPWH::CSVOPT_NONE;
	static const int nTCouples = 12;		// # of storage layers reported by HPWH

	if (hw_bWriteCSV)
	{
		double minHrD = tk.wtk_startMin + 1.;
		double minDay = double(60 * Top.iHr) + minHrD;
		double minYr = double((int(Top.jDayST - 1) * 24 + Top.iHrST) * 60) + minHrD;

		CSVItem CI[] =
		{ "minHr",	   minHrD,              UNNONE, 4,
		  "minDay",	   minDay,              UNNONE, 4,
		  "minYr",	   minYr,               UNNONE, 7,
		  "tDbO",      Top.tDbOSh,			UNTEMP, 5,
		  "tEnv",      hw_tEx,				UNTEMP, 5,
		  "tSrcAir",   hw_tASHPSrc > 0.f ? hw_tASHPSrc : CSVItem::ci_UNSET,
											UNTEMP, 5,
		  "fMixUse",   hw_fMixUse,		    UNNONE, 5,
		  "fMixRL",    hw_fMixRL,		    UNNONE, 5,
		  "vUse",	   drawUse,				UNLVOLUME2, 5,
		  "vLoss",     drawLoss,			UNLVOLUME2, 5,
		  "vRL",       drawRL,				UNLVOLUME2, 5,
		  "vTot",	   drawForTick,			UNLVOLUME2, 5,
		  "tMains",    tMains > 0. ? tMains : CSVItem::ci_UNSET,
											UNTEMP, 5,
		  "tDWHR",     tk.wtk_tInletX,		UNTEMP, 5,
		  "tRL",       drawRL > 0. ? tRL : CSVItem::ci_UNSET,
											UNTEMP,	5,
		  "tIn",       tInlet > 0. ? tInlet : CSVItem::ci_UNSET,
											UNTEMP,	5,
		  "tSP",	   DegCtoF(hw_pHPWH->getSetpoint()),
											UNTEMP,	5,
		  "tOut",      tOut > 0. ? DegCtoF(tOut) : CSVItem::ci_UNSET,
											UNTEMP,  5,
		  "tUse",      tMix > 0.f ? tMix : CSVItem::ci_UNSET,
											UNTEMP,  5,
		  "qTX",	   hw_qTXTick,			UNENERGY3, 5,
		  "qEnv",      KWH_TO_BTU(hw_pHPWH->getEnergyRemovedFromEnvironment()),
											UNENERGY3, 5,
		  "qLoss",     KWH_TO_BTU(hw_pHPWH->getStandbyLosses()),
											UNENERGY3, 5,
		  "XBU",       HPWHxBU,				UNENERGY3, 5,
		  NULL
		};

		CSVGen csvGen(CI);

		if (!hw_pFCSV)
		{
			// dump file name = <cseFile>_<DHWHEATER name>_hpwh.csv
			//   Overwrite pre-existing file
			//   >>> thus file contains info from only last RUN in multi-RUN sessions
			const char* nameNoWS = strDeWS(strtmp(hw_pOwner->name));
			const char* fName =
				strsave(strffix2(strtprintf("%s_%s_hpwh", InputFilePathNoExt, nameNoWS), ".csv", 1));
			hw_pFCSV = fopen(fName, "wt");
			if (!hw_pFCSV)
				err(PWRN, "HPWH report failure for '%s'", fName);
			else
			{	// headings
				fprintf(hw_pFCSV, "%s,%s,%s\n",
					hw_pOwner->GetDescription(), Top.repHdrL, Top.runDateTime);
				fprintf(hw_pFCSV, "%s%s %s %s HPWH %s\n",
					Top.tp_RepTestPfx(), ProgName, ProgVersion, ProgVariant,
					Top.tp_HPWHVersion);
#if defined( HPWH_DUMPSMALL)
				fprintf(wh_pFCSV, "minYear,draw( L)\n");
#else
				WStr s("mon,day,hr,");
				s += csvGen.cg_Hdgs(dumpUx);
				hw_pHPWH->WriteCSVHeading(hw_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
			}
		}
		if (hw_pFCSV)
		{
#if defined( HPWH_DUMPSMALL)
			fprintf(wh_pFCSV, "%0.2f,%0.3f\n", minYear, GAL_TO_L(drawForTick));
#else
			WStr s = strtprintf("%d,%d,%d,",
				Top.tp_date.month, Top.tp_date.mday, Top.iHr + 1);
			s += csvGen.cg_Values(dumpUx);
			hw_pHPWH->WriteCSVRow(hw_pFCSV, s.c_str(), nTCouples, hpwhOptions);
#endif
		}
	}
#endif	// HPWH_DUMP
	return rc;
}		// HPWHLINK::hw_DoSubhrTick
//-----------------------------------------------------------------------------
RC HPWHLINK::hw_DoSubhrEnd(		// end of subhour (accounting etc)
	float mult,			// overall multiplier (e.g. DHWSYS * DHWHEATER)
	ZNR* pZn,			// zone containing HPWH, NULL if none
	ZNR* pZnASHPSrc)	// ASHP heat source zone, NULL if none
{
	RC rc = RCOK;

	// water heater average output temp, F
	if (hw_nzDrawCount)
		hw_tHWOut = hw_tHWOutF / hw_nzDrawCount;	// note: >= ws_tUse due to unlimited XBU
													//  (unless ws_tUse is changed (e.g. via expression))
	// else leave prior value = best available (not updated when draw = 0)

	// link zone heat transfers
	if (pZn)
		pZn->zn_CoupleDHWLossSubhr(hw_qLoss * mult * BtuperkWh / Top.tp_subhrDur);

	if (pZnASHPSrc && hw_qEnv > 0.)
	{	// heat extracted from zone
		double qZn = hw_qEnv * mult * BtuperkWh / Top.tp_subhrDur;
		pZnASHPSrc->zn_qHPWH -= qZn;
		// air flow: assume 20 F deltaT
		// need approx value re zone convective coefficient derivation
		double amfZn = float(qZn / (20. * Top.tp_airSH));
		pZnASHPSrc->zn_hpwhAirX += float(amfZn / pZnASHPSrc->zn_dryAirMass);
	}

#if defined( HPWH_QBAL)
	if (!Top.isWarmup && !Top.tp_autoSizing)
	{	// form energy balance = sum of heat flows into water, all kWh
		double deltaHC = KJ_TO_KWH(hw_pHPWH->getTankHeatContent_kJ()) - hw_tankHCStart;
		double inElec = hw_inElec[0] + hw_inElec[1];
		double qBal = hw_qEnv		// HP energy extracted from surround
			- hw_qLoss		// tank loss
			+ inElec		// electricity in
			+ hw_qTX		// extra tank heat in
			- hw_qHW		// hot water energy
			- deltaHC;		// change in tank stored energy
		if (fabs(qBal) > hw_balErrMax)
			hw_balErrMax = fabs(qBal);
		double fBal = fabs(qBal) / max(hw_tankHCNominal, 1.);
		if (fBal >
#if defined( _DEBUG)
			.0025)
#else
			.004)		// higher msg threshold in release
#endif
		{	// energy balance error
			static const int HWBALERRCOUNTMAX = 10;
			hw_balErrCount++;
			if (hw_balErrCount <= HWBALERRCOUNTMAX || fBal > 0.01)
			{	hw_pOwner->orWarn("HPWH energy balance error (%1.6f kWh  f=%1.6f)%s",
					qBal, fBal,
			        hw_balErrCount == HWBALERRCOUNTMAX
						? "\n    Skipping further messages for minor energy balance errors."
						: "");
			}
		}
	}
#endif

	return rc;
}		// HPWHLINK::hw_DoSubhrEnd
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWHEATER
///////////////////////////////////////////////////////////////////////////////
DHWHEATER::~DHWHEATER()		// d'tor
{
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
	const char* whenTyHs = strtprintf("when whType=%s and whHeatSrc=%s", whTyTx, whHsTx);

	DHWSYS* pWS = wh_GetDHWSYS();
	RC rc = !pWS ? oer("DHWSYS not found")	// insurance (unexpected)
			     : RCOK;
	// rc |= pWS->ws_CheckSubObject() done in wh_Init()

	if (rc)
		return rc;	// give up

	int bIsPreRun = pWS->ws_calcMode == C_WSCALCMODECH_PRERUN;
	int whfcn = wh_SetFunction();		// sets wh_fcn

	// tank surrounding temp -- one of whTEx or whZone, not both
	//   used only re HPWH 2-16, enforce for all
	if (IsSet(DHWHEATER_TEX))
		rc |= disallow( "when 'whTEx' is specified", DHWHEATER_ZNTI);

	if (whfcn == whfcnLOOPHEATER && !wh_CanHaveLoopReturn())
		rc |= oer("DHWLOOPHEATER must have whHeatSrc=ASPX or whHeatSrc=ResistanceX.");

	if (wh_type != C_WHTYPECH_STRGSML && wh_type != C_WHTYPECH_BUILTUP)
	{	// wh_type: _STRGLRG, _INSTSML, _INSTLRG, _INSTUEF
		if (wh_heatSrc == C_WHHEATSRCCH_ASHP
			|| wh_heatSrc == C_WHHEATSRCCH_ASHPX
			|| wh_heatSrc == C_WHHEATSRCCH_ELRESX)
			rc |= ooer(DHWHEATER_HEATSRC,
				"whHeatSrc=%s is not allowed %s", whHsTx, whenTy);

		ignoreN(whenTy, DHWHEATER_LDEF, DHWHEATER_ASHPRESUSE, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2,
			DHWHEATER_TANKCOUNT, 0);

		if (wh_type == C_WHTYPECH_INSTUEF)
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
		else if (wh_type == C_WHTYPECH_STRGLRG || wh_type == C_WHTYPECH_INSTLRG)
			rc |= require(whenTy, DHWHEATER_EFF);
		else if (wh_type == C_WHTYPECH_INSTSML)
			rc |= require(whenTy, DHWHEATER_EF);
	}
	else if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
	{	// STRGSML or BUILTUP HPWH model
		// TODO: more specific checking for ASHPX
		ignoreN( whenHs, DHWHEATER_LDEF, DHWHEATER_RESHTPWR, DHWHEATER_RESHTPWR2, DHWHEATER_RESTY, 0);
		RC rc1 = requireN( whenHs, DHWHEATER_ASHPTY, 0);
		rc |= rc1;
		if (!rc1)
		{	int reqdAttr = wh_type == C_WHTYPECH_BUILTUP
				? HPWHLINK::hwatLARGE
				: HPWHLINK::hwatSMALL;
			const char* whAshpTyTx = getChoiTx(DHWHEATER_ASHPTY, 1);
			if (!wh_HPWH.hw_IsAttr(wh_ashpTy, reqdAttr))
				rc |= ooer(DHWHEATER_ASHPTY, "whASHPType=%s not supported %s", whAshpTyTx, whenTy);
			else if (wh_ashpTy == C_WHASHPTYCH_GENERIC)
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
		if (wh_type == C_WHTYPECH_BUILTUP)
		{	rc |= requireN(whenTyHs, DHWHEATER_VOL, 0);
			ignoreN(whenTyHs, DHWHEATER_EF, 0);
			wh_EF = 0.95f;		// dummy default
		}
		else
		{	rc |= requireN(whenTyHs, DHWHEATER_EF, 0);
			if (wh_EF > 0.98f)
				rc |= oer("whEF (%0.3f) must be <= 0.98 %s",
					wh_EF, whenHs);
		}
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
			ignore( strtprintf("%s and whEF=1", whenTy), DHWHEATER_LDEF);
			wh_LDEF = 1.;
		}
		else
		{	// either LDEF required or EF + prerun
			if (!bIsPreRun)
				rc |= require( strtprintf("%s and DHWSYS is not PreRun", whenTy), DHWHEATER_LDEF);
			if (IsSet(DHWHEATER_LDEF))
				ignore( strtprintf("%s and whLDEF is given", whenTy), DHWHEATER_EF);
			else if (bIsPreRun)
				rc |= require( strtprintf("%s and whLDEF is not given", whenTy), DHWHEATER_EF);
		}
	}

	if (wh_IsHPWHModel())
	{	// need sufficient info to determine tank UA
		int argCount = IsSetCount(DHWHEATER_UA, DHWHEATER_INSULR, 0);
		if (argCount == 2)
			rc |= disallow( "when 'whUA' is specified", DHWHEATER_INSULR);
		else if (argCount == 0 && wh_type == C_WHTYPECH_BUILTUP)
			rc |= oer("whUA or whInsulR is required %s", whenTy);
	}
	else
		ignoreN(whenHs, DHWHEATER_UA, DHWHEATER_INSULR, 0);

	// check heating capacity scalability
	//   wh_IsScalable() can return -1=maybe -> further checks later
	//   DHWHEATER_HEATINGCAP repeat check in wh_HPWHInit()
	//       (after HPWH linkage established)
	if (IsSet(DHWHEATER_HEATINGCAP) && wh_IsScalable() == 0)
		ignore( whenHs, DHWHEATER_HEATINGCAP);

	if (!wh_CanHaveLoopReturn())
		ignoreN(whenHs, DHWHEATER_INHTSUPPLY, DHWHEATER_INHTLOOPRET, 0);

	if (IsSet(DHWHEATER_VOLRUNNING))
	{	if (!wh_CanSetVolFromVolRunning())
			rc |= disallow(whenHs, DHWHEATER_VOLRUNNING);
		else if (IsSet(DHWHEATER_VOL))
			rc |= oer("whVol and whVolRunning cannot both be specified");
	}

	if (wh_IsStorage())
	{	// note wh_vol is required in some cases
        //   see above
        if (!IsSet( DHWHEATER_VOL))
			wh_vol = 50.f;
		rc |= limitCheck( DHWHEATER_VOL, .1, 20000.);
	}
	else if (wh_vol > 0.f)
		// tolerate specified whVol==0 for instantaneous
		rc |= disallow( whenTy, DHWHEATER_VOL);

	// if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
	// TODO: checking for Ecotope HPWH model

	wh_SetDesc();

	return rc;
}	// DHWHEATER::wh_CkF
//-----------------------------------------------------------------------------
int DHWHEATER::wh_IsScalable() const		// can heating capacity be set
// returns 1: yes
//         0: no
//        -1: maybe (re HPWH pending HPWHLINK setup)
{
	int ret = 0;
	if (wh_IsHPWHModel())
	{
		ret = wh_HPWH.hw_pHPWH
				? wh_HPWH.hw_pHPWH->isHPWHScalable()
				: -1;
	}
	// else
	//	ret = 0;

	return ret;

}		// DHWHEATER::wh_IsScalable
//-----------------------------------------------------------------------------
int DHWHEATER::wh_CanSetVolFromVolRunning() const	// can volume be derived from volRunning
// Note: further checks later
// returns 1: yes
//         0: no
//        -1: maybe (re HPWH pending HPWHLINK setup)
{
	int ret = 0;
	if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
	{	ret = -1;
		if (wh_HPWH.hw_HasCompressor()	// redundant *but* false if !hw_pHPWH
		 && !wh_HPWH.hw_pHPWH->isTankSizeFixed())
			ret = 1;
	}
	// else
	// 	Other type (including C_WHHEATSRCCH_ELRESX): volRunning not supported
	//	ret = 0;

	return ret;
}		// DHWHEATER::wh_CanSetVolFromVolRunning
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
//-----------------------------------------------------------------------------
int DHWHEATER::wh_ReportBalErrorsIf() const
// end-of-run water heater energy balance error check / report
// returns # of balance errors during run
{
	return record::ReportBalErrorsIf(wh_balErrCount, "subhours");
}		// DHWHEATER::wh_ReportBalErrorsIf
//----------------------------------------------------------------------------
RC DHWHEATER::wh_Init()		// init for run
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	wh_pFCSV = NULL;

	// one-time inits
	wh_balErrCount = 0;

	// per run totals (also called on 1st main sim day)
	wh_InitRunTotals();

	rc |= pWS->ws_CheckSubObject(this);		// check system config
											//   DHWHEATER not allows on child DHWSYS

	int whfcn = wh_GetFunction();

	// whfcnLASTHEATER: this heater feeds load (not another heater)
	if (whfcn == whfcnLOOPHEATER || pWS->ws_wlhCount == 0.f)
		wh_fcn |= whfcnLASTHEATER;

	if (wh_CanHaveLoopReturn() && pWS->ws_calcMode == C_WSCALCMODECH_SIM)
	{	// no info msgs on PRERUN -- else duplicates
		if (pWS->ws_wlCount == 0)
		{	ignore("when DHWSYS includes no DHWLOOP(s).", DHWHEATER_INHTLOOPRET);
			if (whfcn == whfcnLOOPHEATER)
				oInfo("modeled as a series heater because DHWSYS includes no DHWLOOP(s).");
		}
		else if (whfcn == whfcnPRIMARY && pWS->ws_wlhCount > 0)
			ignore( "when DHWSYS includes DHWLOOPHEATER(s).", DHWHEATER_INHTLOOPRET);
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
	wh_HPWH.hw_Cleanup();

	if (wh_IsHPWHModel())
		rc |= wh_HPWHInit();	// set up from DHWHEATER inputs

	else if (wh_type == C_WHTYPECH_INSTUEF)
		rc |= wh_InstUEFInit();		// UEF-based instantaneous water heater model 5-2017

	return rc;
}		// DHWHEATER::wh_Init
//----------------------------------------------------------------------------
void DHWHEATER::wh_InitRunTotals()
// start-of-run initialization totals, error counts, ...
// called at beg of warmup and run
{
	// linkage to DHWSYSRES subhour totals
	//   skip if DHWSYSRES not yet allocated 
	DHWSYS* pWS = wh_GetDHWSYS();
	DHWSYSRES* pWSR = pWS->ws_GetDHWSYSRES();
	wh_pResSh = pWSR ? &(pWSR->S) : NULL;

	wh_totHARL = 0.;
	wh_hrCount = 0;
	wh_totOut = 0.;
	wh_unMetHrs = 0;
	wh_stbyTicks = 0;
	wh_inElecTot = 0.;
	wh_inFuelTot = 0.;
	wh_HPWH.hw_InitTotals();

}		// DHWHEATER::wh_InitRunTotals
//----------------------------------------------------------------------------
#if 0
// activate if needed
RC DHWHEATER::wh_RddInit()		// late pre-run init
// called at beg of each design day and beg of main simulation
{
	RC rc = RCOK;
	return rc;
}	// DHWHEATER::wh_RddInit
#endif
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

	if (wh_type != C_WHTYPECH_STRGSML)
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
RC DHWHEATER::wh_DoHour()			// DHWHEATER hour calcs
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();

	int whfcn = wh_GetFunction();

	wh_hrCount++;

	wh_inElec = 0.f;
	wh_inElecBU = 0.f;
	wh_inElecXBU = 0.f;
	wh_inFuel = 0.f;
	wh_unMetSh = 0;
	wh_tInlet = 0.f;
	wh_draw = 0.f;

	float tSetpoint = pWS->ws_GetTSetpoint(whfcn);

	if (pWS->ws_tOutPrimLT == 0.f)
		pWS->ws_tOutPrimLT = tSetpoint;	// initial guess for primary heater outlet temp
										//   meaningful for HPWH only?

	if (wh_IsHPWHModel())
	{	rc |= wh_HPWH.hw_DoHour(tSetpoint);
		// check pWS->ws_tSetpointDes ?
	}

	return rc;

}	// DHWHEATER::wh_DoHour
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_EndIvl(		// end-of-hour accounting
	IVLCH ivl,		// C_IVLCH_Y etc
	float HARL,		// single heater recovery load for hour, Btu
	float wsMult)	// DHWSYS multiplier

// DHWHEATER subhour models accum to wh_inElec, wh_inElecBu, wh_inElecXBU, and wh_inFuel

// do not call for C_IVLCH_S

{
	RC rc = RCOK;

	// hour average inlet temp
	if (wh_tInlet > 0.f)
		wh_tInlet /= wh_draw;

	// accumulate load (re LDEF derivation)
	wh_totHARL += HARL;		// annual total

	// check figure
	wh_inElecTot += wh_inElec + wh_inElecBU + wh_inElecXBU;

	// accum consumption to meters (scaled by multipliers)
	float mult = wh_mult * wsMult;	// overall multiplier = system * heater

	if (wh_pMtrElec)
	{	wh_pMtrElec->H.dhw += mult * wh_inElec;
		if (wh_xBUEndUse)
		{	wh_pMtrElec->H.dhwBU += mult * wh_inElecBU;
			wh_pMtrElec->H.mtr_Accum(wh_xBUEndUse, mult*wh_inElecXBU);
		}
		else
			wh_pMtrElec->H.dhwBU += mult * (wh_inElecBU + wh_inElecXBU);
	}

	if (wh_pMtrFuel)
		wh_pMtrFuel->H.dhw += mult * wh_inFuel;

	if (ivl == C_IVLCH_Y)
	{	// definition of "unmet" depends on heater specifics
		//   generally unexpected due to XBU
		if (wh_unMetHrs > 0)
			warn("%s: Output temperature too low during %d hours of run.",
				objIdTx(), wh_unMetHrs);
	}

	return rc;

}	// DHWHEATER::wh_EndIvl
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_DoEndPreRun()
{
	RC rc = RCOK;

	DHWHEATER* pWHi = wh_GetInputDHWHEATER();
	if (!pWHi)
		return orMsg(ERR, "Bad input record linkage in wh_DoEndPreRun");

	if (0)
	{
		pWHi->wh_heatingCap = 0;
	}
	
	if (!wh_UsesDerivedLDEF())
		return rc;		// no adjustments required

	if (wh_type == C_WHTYPECH_STRGSML)
	{	if (wh_EF < 1.f)		// if not ideal efficiency (testing)
		{	// average hourly load
			float arl = wh_totHARL / wh_hrCount;
			// "Load-dependent energy factor"
			float LDEF = wh_CalcLDEF(arl);
			// update input record with derived value
			if (!pWHi->IsSet(DHWHEATER_LDEF))
			{	pWHi->wh_LDEF = LDEF;
				pWHi->fStat(DHWHEATER_LDEF) |= FsSET | FsVAL;
			}
		}
	}
	// else nothing needed

	return rc;
}		// DHWHEATER::wh_DoEndPreRun
//-----------------------------------------------------------------------------
/*virtual*/ void DHWHEATER::ReceiveRuntimeMessage(const char* msg)
{
	pInfo("%s: HPWH message (%s):\n  %s",
		objIdTx(), Top.When(C_IVLCH_S), msg);
}		// DHWHEATER::ReceiveRuntimeMessage
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_HPWHInit()		// initialize HPWH model
// returns RCOK on success
{
	RC rc = RCOK;
	DHWSYS* pWS = wh_GetDHWSYS();

	int whfcn = wh_GetFunction();
	
	wh_HPWH.hw_Init(this);

	bool bVolMaybeModifiable = false;
	if (wh_heatSrc == C_WHHEATSRCCH_ELRESX)
	{	// resistance tank (no preset)
		//  wh_EF and wh_insulR < 0 if not set
		//  wh_EF > 0 determines HPWH type of resistance tank
		//  wh_resTy is currently documentation only (9-2021)
		float insulR = IsSet(DHWHEATER_INSULR) ? wh_insulR : 12.f;
		rc |= wh_HPWH.hw_InitResistance(
			wh_resTy, wh_vol, wh_EF, insulR, wh_resHtPwr, wh_resHtPwr2);
		// bVolMaybeModifiable = true;
	}
	else if (wh_ashpTy == C_WHASHPTYCH_GENERIC)
	{	// generic HPWH (no preset)
		rc |= wh_HPWH.hw_InitGeneric(wh_vol, wh_EF, wh_ashpResUse);
	}
	else
	{	rc |= wh_HPWH.hw_InitPreset(wh_ashpTy);
		bVolMaybeModifiable = true;
		// volume set below after heatingCap is known
	}

	if (!rc && IsSet(DHWHEATER_HEATINGCAP))
	{	// check whether heating capacity can be adjusted
		if (!wh_HPWH.hw_pHPWH->isHPWHScalable() || !wh_HPWH.hw_HasCompressor())
		{	if (wh_heatSrc == C_WHHEATSRCCH_ASHPX)
				rc |= oer("whHeatingCap is not allowed when whASHPType=%s",
					getChoiTx(DHWHEATER_ASHPTY, 1));
			else
				rc |= oer("whHeatingCap is not allowed.");
		}
		else
		{	RC rc2 = wh_HPWH.hw_SetHeatingCap(
				wh_heatingCap,			// required capacity
				pWS->ws_ashpTSrcDes,	// source air
				pWS->ws_tInletDes,		// inlet water temp
				pWS->ws_tUse);			// outlet water temp
			if (rc2)
				rc |= err(PERR, "DHWHEATER::wh_HPWHInit: wh_IsScalable() inconsistency.");
		}
	}

	// retrieve capacity: may not have been set; limits may have been applied
	if (!rc)	// if success so far (else HPWH queries can fail)
		rc |= wh_HPWH.hw_GetHeatingCap(
			wh_heatingCap,			// capacity
			pWS->ws_ashpTSrcDes,	// source air
			pWS->ws_tInletDes,		// inlet water temp
			pWS->ws_tUse);			// outlet water temp

	if (!rc && bVolMaybeModifiable)
	{	const char* what = IsSet(DHWHEATER_VOL) ? "whVol"
				: IsSet(DHWHEATER_VOLRUNNING) ? "whVolRunning"
				: NULL;
		if (what)
		{	float vol = -1.f;
			if (wh_HPWH.hw_pHPWH->isTankSizeFixed())
				oInfo("%s is ignored when whASHPType=%s (tank volume is fixed at %0.0f gal)",
					what, getChoiTx(DHWHEATER_ASHPTY, 1),
					wh_HPWH.hw_pHPWH->getTankSize(HPWH::UNITS_GAL));
			else if (IsSet(DHWHEATER_VOLRUNNING))
			{	// semi-redundant check
				if (!wh_CanSetVolFromVolRunning())
					oInfo("%s is ignored (tank volume is fixed at %0.0f gal)",
						what, wh_HPWH.hw_pHPWH->getTankSize(HPWH::UNITS_GAL));
				else
				{	RC rc2 = wh_HPWH.hw_DeriveVolFromVolRunning(
						wh_volRunning,
						wh_heatingCap,
						pWS->ws_tSetpointDes - pWS->ws_tInletDes,
						vol);
					if (rc2)
						rc |= err(PERR, "DHWHEATER::wh_HPWHInit: hw_CanSetVolFromVolRunning() inconsistency.");
				}
			}
			else // IsSet( DHWHEATER_VOL)
				vol = wh_vol;

			if (vol > 0.f)
			{	wh_vol = vol;
				wh_HPWH.hw_pHPWH->setTankSize_adjustUA(vol, HPWH::UNITS_GAL);
			}
		}
	}

	// at this point, HPWH has known size and default UA
	//   (later capacity scaling does not alter size)
	//   if additional UA or insulR is provided, adjust UA
	if (!rc)
		rc |= wh_HPWH.hw_AdjustUAIf(wh_UA, wh_insulR, wh_tankCount);

	if (!rc)
	{	// tank inlet fractional heights
		float inHtSupply  = IsSet(DHWHEATER_INHTSUPPLY)  ? wh_inHtSupply  : -1.f;
		float inHtLoopRet = IsSet(DHWHEATER_INHTLOOPRET) ? wh_inHtLoopRet : -1.f;
		rc |= wh_HPWH.hw_InitFinalize(inHtSupply, inHtLoopRet);
	}

	// make probe-able values consistent with HPWH
	//   note hw_GetInfo() call uses tankCount=1: get totals from HPWH
	if (!rc)
		wh_HPWH.hw_GetInfo(wh_vol, wh_UA, wh_insulR, wh_tankCount);

	// config checks -- report only once
	if (!rc && !pWS->ws_configChecked)
	{
		if (wh_HPWH.hw_IsSetpointFixed())
		{	int fn = pWS->ws_GetTSetpointFN(whfcn);
			if (fn)
				pWS->ignore(strtprintf("-- HPWH '%s' has a fixed setpoint.", name), fn);

			// force consistent ws_tSetpointDes
			float tspFixed = wh_HPWH.hw_pHPWH->getSetpoint(HPWH::UNITS_F);
			if (!pWS->IsSet( DHWSYS_TSETPOINTDES) || tspFixed < pWS->ws_tSetpointDes)
				pWS->ws_tSetpointDes = tspFixed;
		}
		wh_DRMapValidate();	// validate drStatus mapping (ABT if bad)
	}
	return rc;
}		// DHWHEATER::wh_HPWHInit
//----------------------------------------------------------------------------
// Demand reduction (DR) stuff
struct DRMAP
{	DHWDRSIG drSig;		// CSE signal choice
	int drStatusHPWH;	// corresponding HPWH drStatus
};
static const DRMAP drMap[] =
{   C_DHWDRSIG_ON,     HPWH::DR_ALLOW,
	C_DHWDRSIG_TOO,    HPWH::DR_TOO,
	C_DHWDRSIG_TOOLOR, HPWH::DR_TOO | HPWH::DR_LOR,
	C_DHWDRSIG_TOOLOC, HPWH::DR_TOO | HPWH::DR_LOC,
	C_DHWDRSIG_TOT,    HPWH::DR_TOT,
	C_DHWDRSIG_TOTLOR, HPWH::DR_TOT | HPWH::DR_LOR,
	C_DHWDRSIG_TOTLOC, HPWH::DR_TOT | HPWH::DR_LOC,
	C_DHWDRSIG_LOC,    HPWH::DR_LOC,
	C_DHWDRSIG_LOR,    HPWH::DR_LOR,
	C_DHWDRSIG_LOCLOR, HPWH::DR_LOC | HPWH::DR_LOR,
	-1,				-1
};
//-----------------------------------------------------------------------------
/* static*/ void DHWHEATER::wh_DRMapValidate()	// one-time DRMAP checks
// Detects misconfigured code
// Aborts execution on fail
{
	//   correct table allows access by idx, avoids search
	bool bMunge = false;
	int ix;
	for (ix = 0; !bMunge && drMap[ix].drSig >= 0; ix++)
	{
		if (drMap[ix].drSig != ix + C_DHWDRSIG_ON)
			bMunge = true;
	}
	if (bMunge || ix != C_DHWDRSIG_COUNT - C_DHWDRSIG_ON)
		// table out of order or wrong # of entries
		errCrit(PABT, "DHWHEATER::wh_DRMapValidate() failure.");

}		// DHWHEATER::wh_DRMapValidate
//-----------------------------------------------------------------------------
/* static*/ int DHWHEATER::wh_DRMapSigToDRStatus(
	DHWDRSIG drSig)		// CSE DR choice value
						//   -1: validate table
// returns HPWH-compatible DRMODES value corresponding to CSE choice
{
	int ixDrSig = drSig - C_DHWDRSIG_ON;	// choice values assigned sequencially

	int drStatus = drMap[ixDrSig].drStatusHPWH;

	return drStatus;

}	// DHWHEATER::wh_DRMapSigToDRStatus
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_DoSubhrStart()
{
	RC rc = RCOK;

	// DHWSYS* pWS = wh_GetDHWSYS();

	wh_effSh = 0.f;
	wh_inElecSh = 0.f;
	wh_inElecBUSh = 0.f;
	wh_inElecXBUSh = 0.f;
	wh_inFuelSh = 0.f;
	wh_qHW = 0.f;
	wh_qXBU = 0.f;	// heat in excess of capacity
					//   provided by virtual resistance heat
					//   prevents gaming compliance via undersizing

	// ambient and source temps
	//   set temp from linked zone (else leave expression/default value)
	//   used by e.g. HPWH
	if (wh_pZn)
		wh_tEx = wh_pZn->tzls;
	if (wh_pAshpSrcZn)
		wh_ashpTSrc = wh_pAshpSrcZn->tzls;

	if (wh_IsHPWHModel())
		rc |= wh_HPWH.hw_DoSubhrStart(wh_tEx, wh_ashpTSrc);
	else if (wh_IsInstUEFModel())
	{	// carry-forward: instantaneous heaters throttle flow to maintain temp.
		//   here represented by carrying forward a limited amount of unmet load
		wh_nzDrawCount = 0;		// count of ticks with draw > 0
		wh_nTickFullLoad = 0.f;	// fractional ticks of equiv full load
		wh_nColdStarts = 0.f;	// # of cold startups
	}
	else
	{	// T24DHW efficiency-based models
		wh_effSh = wh_type == C_WHTYPECH_INSTSML ? wh_EF * 0.92f
			: wh_type == C_WHTYPECH_INSTLRG ? wh_eff * 0.92f
			: wh_type == C_WHTYPECH_STRGSML ? wh_LDEF
			: wh_type == C_WHTYPECH_STRGLRG ? wh_eff
			: -1.f;

		if (wh_effSh <= 0.f)
			rc |= err("%s, %s: Invalid water heater efficiency %0.3f",
					objIdTx(), Top.When(C_IVLCH_S), wh_effSh);
	}

	return rc;
}		// DHWHEATER::wh_DoSubhrStart
//-----------------------------------------------------------------------------
RC DHWHEATER::wh_DoSubhrTick(		// DHWHEATER energy use for 1 tick
	DHWTICK& tk,	// current tick
	float scaleWH)	// draw scale factor = 1/ws_whCount or 1/ws_wlhCount
					//   re DHWSYSs with >1 DHWHEATER
					//   *not* including any mixdown factors (e.g. hw_fMixUse or hw_fMixRL)
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	DHWSYSRES* pWSR = pWS->ws_GetDHWSYSRES();
	int whfcn = wh_GetFunction();

	// inlet (supply water) temp for this heater w/o loop returns
	//   HPWH types: loop flow handled internally
	//   non-HPWH: adjusted below for any loop returns
	float tInletWH =
		whfcn == whfcnLOOPHEATER ? pWS->ws_tOutPrimLT
	  : pWS->ws_pDHWSOLARSYS     ? pWS->ws_pDHWSOLARSYS->sw_GetAvailableTemp()
	  :                            tk.wtk_tInletX;

#if 0 && defined( _DEBUG)
	if (tInletWH > pWS->ws_tUse)
		printf("\nHot!");
#endif

	float tOutNoMix = 0.f;
	float tMix = wh_IsLastHeater() ? pWS->ws_tUse : -1.f;

	float drawForTick = 0.f;	// total draw for this tick, gal
								//   includes loop flow and loss draws

	if (wh_IsHPWHModel())
	{	// demand response (DR)
		//   use DHWSYS hourly base value
		//   turn off hour-start one-shot signals if not hour start
		//     TOO (Top off once) is sent on tick 0 only
		int drStatus;
		if (whfcn == whfcnPRIMARY)
		{	drStatus = pWS->ws_drStatusHPWH;
			if (tk.wtk_startMin > 0.f)
				drStatus &= ~(HPWH::DR_TOO);
		}
		else
			drStatus = HPWH::DR_ALLOW;

		rc |= wh_HPWH.hw_DoSubhrTick(tk, tInletWH, scaleWH, tMix, pWS->ws_tInlet,
					&tOutNoMix, drStatus);

		if (whfcn == whfcnPRIMARY)
			pWS->ws_tOutPrimSum += tOutNoMix * scaleWH * wh_mult;
		drawForTick = tk.wtk_whUse;		// ??
	}
	else 
	{	// inlet temp: combine use and any DHWLOOP return
#if 0
		float tInletMixM;
		float drawForTickM = tk.wtk_DrawTotM(pWS->ws_tUse, tInletMixM, tInletWH)*scaleWH;
		float tInletMixMX;
		double drawForTickMX = tk.wtk_DrawTotMX(pWS->ws_tUse, tInletWH, pWS->ws_tInlet, tInletMixMX)*scaleWH;
		drawForTick = drawForTickMX;
		float tInletMix = tInletMixMX;
		tInletWH = tInletMixM;
#else
		float tInletMix;
		drawForTick = tk.wtk_DrawTot(pWS->ws_tUse, tInletWH, pWS->ws_tInlet, tInletMix)*scaleWH;
		tInletWH = tInletMix;
#if 0
		if (frDiff(tInletMix, tInletMixMX) > .001)
			printf("\nDraw diff");
#endif
#endif
		if (wh_IsInstUEFModel())
			rc |= wh_InstUEFDoSubhrTick(drawForTick, tInletMix, scaleWH, tMix);

		else
		{	float deltaT = max(1.f, pWS->ws_tUse - tInletMix);

			float HARL = drawForTick * waterRhoCp * deltaT;		// load on this heater, Btu

			wh_qHW += HARL;		// output = load

			float WHEU = HARL / wh_effSh + wh_SBL * Top.tp_tickDurHr;		// current tick energy use, Btu

			// electricity / fuel consumption for this DHWHEATER (no multipliers)
			wh_inElecSh += wh_parElec * Top.tp_tickDurHr * BtuperWh;	// electric parasitic
												//  (supported for both fuel and elec WH)
			if (wh_IsElec())
				wh_inElecSh += WHEU;
			else
				wh_inFuelSh += WHEU + wh_pilotPwr * Top.tp_tickDurHr;	// pilot included in all fuel types
																		//   (but not elec WH)
		}
	}

	if (whfcn == whfcnPRIMARY)
	{
		float drawTot = tk.wtk_whUse * scaleWH * wh_mult * pWS->ws_mult;
		float dhwLoadTk1 = drawTot * waterRhoCp * (pWS->ws_tUse - pWS->ws_tInlet);
		pWSR->S.qLoad += dhwLoadTk1;
		
		float dhwLoadTk2 = drawTot * waterRhoCp * (pWS->ws_tUse - tk.wtk_tInletX);
		pWS->ws_SSFAnnualReq += dhwLoadTk2;
		
		if (pWS->ws_pDHWSOLARSYS)
		{	// accumulate solar draws for next tick
			float drawSolarSys = tk.wtk_volIn;	// draw from solar: does not include loop flow
			if (drawSolarSys > 0.f)
				rc |= pWS->ws_pDHWSOLARSYS->sw_TickAccumDraw(
					pWS,
					drawSolarSys * scaleWH * wh_mult * pWS->ws_mult,
					tk.wtk_tInletX);	// solar system inlet water temp
		}
	}

	if (drawForTick > 0.f)
	{	wh_tInlet += tInletWH * drawForTick;
		wh_draw += drawForTick;
	}

	return rc;
}		// DHWHEATER::wh_DoSubhrTick
//--------------------------------------------------------------------------------------
RC DHWHEATER::wh_DoSubhrEnd(		// end-of-subhour
	bool bIsLH)		// true iff this is a DHWLOOPHEATER
// returns RCOK iff simulation should continue
{
	RC rc = RCOK;

	DHWSYS* pWS = wh_GetDHWSYS();
	
	float mult = pWS->ws_mult * wh_mult;	// overall multiplier

	if (wh_IsHPWHModel())
	{
		wh_HPWH.hw_DoSubhrEnd(mult, wh_pZn, wh_pAshpSrcZn);

		wh_qLoss = wh_HPWH.hw_qLoss;
		wh_qEnv = wh_HPWH.hw_qEnv;
		wh_balErrCount = wh_HPWH.hw_balErrCount;
		wh_tHWOut = wh_HPWH.hw_tHWOut;
		wh_qXBU = wh_HPWH.hw_HPWHxBU;
		if (pWS->ws_tUse - wh_tHWOut > 1.f)
		{
#if 0 && defined( _DEBUG)
			if (wh_HPWH.hw_nzDrawCount != 0)
				printf("\nUnexpected unmet");
#endif
			wh_unMetSh++;	// unexpected, XBU should maintain temp
							//   will happen if ws_tUse changes (e.g. via expression)
							//   during a period of no draw
		}

		// energy output and electricity accounting (assume no fuel)
		wh_qHW = KWH_TO_BTU(wh_HPWH.hw_qHW);			// hot water heating, Btu
		wh_inElecXBUSh = wh_qXBU;						// add'l backup heating, Btu

		wh_inElecSh = wh_HPWH.hw_inElec[1] * BtuperkWh + wh_parElec * BtuperWh*Top.tp_subhrDur;
		wh_inElecBUSh = wh_HPWH.hw_inElec[0] * BtuperkWh;
	}
	else if (wh_IsInstUEFModel())
	{	double rcovFuel = wh_maxInpX * wh_nTickFullLoad;
		double startFuel = wh_cycLossFuel * wh_nColdStarts;
		double rcovElec = wh_operElec * wh_nzDrawCount * Top.tp_tickDurHr;	// assume operation for entire tick with any draw
		// double startElec = wh_cycLossElec * nTickStart;	// unused in revised model
		// standby in ticks w/o draw
		double stbyElec = wh_stbyElec * (Top.tp_nSubhrTicks - wh_nzDrawCount) * Top.tp_tickDurHr;

		// wh_qHW and wh_qXBU accum'd in wh_InstUEFDoSubhrTick()

		// energy use accounting, Btu
		wh_inElecSh += rcovElec /*+ startElec*/ + (stbyElec + wh_parElec * Top.tp_tickDurHr) * BtuperWh;
		wh_inFuelSh += rcovFuel + startFuel;
	}
	// else
	//  { efficiency model (nothing add'l required)
	//     wh_qHW accumulated in wh_DoSubhrTick()
	//     wh_qXBU = 0
	//  }

	// energy totals for hour
	wh_inElec += wh_inElecSh;
	wh_inElecBU += wh_inElecBUSh;
	wh_inElecXBU += wh_inElecXBUSh;
	wh_inFuel += wh_inFuelSh;

	// output (heat added to water, Btu) accounting
	wh_totOut += wh_qHW + wh_qXBU;	// annual total (check value)

	// DHWSYSRES accumulation
	if (wh_qHW > 0.f)
		bIsLH ? wh_pResSh->qLH : wh_pResSh->qWH += wh_qHW * wh_mult;
	if (wh_qXBU > 0.f)
		wh_pResSh->qXBU += wh_qXBU * wh_mult;
	
	return rc;
}		// DHWHEATER::wh_DoSubhrEnd
//--------------------------------------------------------------------------------------
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
	// Top.tp_tickDurMin = tick duration, min
	wh_maxFlowX = wh_ratedFlow * Top.tp_tickDurMin * 67.f;

	// maximum load carry forward, Btu
	// user input wh_loadCFwdF = multiplier for rated capacity
	//   (approximately allowed catch-up time, hr)
	wh_loadCFwdMax = wh_loadCFwdF * wh_ratedFlow * waterRhoCp * 67. * 60.;

	// fuel input: Btu/tick at flow=maxFlow and deltaT=67
	wh_maxInpX = Pf				// Btu/min at flow=flowRE and deltaT=67
		       * (wh_ratedFlow / UP.flowRE)	// scale to max flow
		       * Top.tp_tickDurMin;		// scale to actual tick duration

	// no electricity use pending model development
	wh_operElec = Pe * BtuperWh;	// electrical power during operation, Btuh
	// wh_cycLossElec = 0.f;		// electricity use per start, Btu
									//    unused in revised model 5-24-2017

	return rc;
}		// DHWHEATER::wh_InstUEFInit
//----------------------------------------------------------------------------
RC DHWHEATER::wh_InstUEFDoSubhrTick(
	double draw,	// draw for tick, gal (use + DHWLOOP)
	float tInletWH,	// current water heater inlet temp, F
					//   includes upstream heat recovery, solar, etc.
					//   also includes mixed-in DHWLOOP return, if any
	[[maybe_unused]] float scaleWH,	// draw scale factor
					//   re DHWSYSs with >1 DHWHEATER
					//   *not* including hw_fMixUse or hw_fMixRL;
	float tUse)		// assumed output temp, F

// returns RCOK iff all OK
	
{
	RC rc = RCOK;

	float deltaT = tUse - tInletWH;		// temp rise
	if (deltaT < 0.f)	// should not be <0 due to wtk_DrawTot
	{
#if defined(_DEBUG)
		printf("\nUEFInst deltaT = %02.f", deltaT);
#endif
		deltaT = 0.f;
	}
	double qPerGal = waterRhoCp * deltaT;
	if (deltaT > 0.f)
		draw += wh_loadCFwd / qPerGal;
	else
		wh_stbyTicks = 0;
	wh_loadCFwd = 0.;	// clear carry-forward, if not met, reset just below
	if (draw > 0.)
	{	wh_nzDrawCount++;
		if (deltaT > 0.f)
		{	float fLoad;
			double drawFullLoad = wh_maxFlowX / deltaT;	// max vol that can be heated in this tick
			if (draw > drawFullLoad)
			{	fLoad = 1.f;				// full capacity operation
				wh_loadCFwd = qPerGal * (draw - drawFullLoad);	// unmet load
				if (wh_loadCFwd > wh_loadCFwdMax)
				{	wh_qXBU += wh_loadCFwd - wh_loadCFwdMax;		// excess heating required to meet ws_tUse
					wh_loadCFwd = wh_loadCFwdMax;
				}
			}
			else
				fLoad = draw / drawFullLoad;
			wh_qHW += fLoad * drawFullLoad * qPerGal;
			wh_nTickFullLoad += fLoad;

			if (wh_stbyTicks)	// if no draw in prior tick
			{
				double offMins = wh_stbyTicks * Top.tp_tickDurMin;
				// exponential cooldown
				static const double TC = 50.06027;		// cooldown time constant
				double r = offMins / TC;
				wh_nColdStarts += r > 10. ? 1. : 1. - exp(-r);		// avoid underflow
				// printf( "\n%0.2f  %0.4f", offMins, nColdStarts);
				wh_stbyTicks = 0;
			}
		}
	}
	else
	{	// standby
		wh_stbyTicks++;		// count conseq ticks w/o draw
	}
	return rc;
}		// DHWHEATER::wh_InstUEFDoSubhrTick

//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWTANK -- CEC T24DHW tank model (heat loss only, no storage)
///////////////////////////////////////////////////////////////////////////////
FLOAT DHWTANK::TankSurfArea_CEC(		// calc tank surface area per CEC methods
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
	RC rc = RCOK;
	DHWSYS* pWS = wt_GetDHWSYS();
	rc |= !pWS ? oer( "DHWSYS not found")	// insurance (unexpected)
				 : pWS->ws_CheckSubObject( this);

	// tank surrounding temp -- one of wtTEx or wtZone, not both
	int nVal = IsSetCount(DHWTANK_TEX, DHWTANK_ZNTI, 0);
	if (nVal == 0)
		rc |= oer("one of 'wtTEx' and 'wtZone' must be specified.");
	else if (nVal == 2)
		rc |= disallow( "when 'wtTEx' is specified", DHWTANK_ZNTI);

	if (IsSet( DHWTANK_UA))
		rc |= disallow("when 'wtUA' is specified", DHWTANK_INSULR);
	else
	{	float tsa = TankSurfArea_CEC( wt_vol);
		wt_UA = tsa / max(0.68f, wt_insulR);
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
RC DHWTANK::wt_Init()			// init for run
{	RC rc = RCOK;
	wt_pZn = ZrB.GetAtSafe(wt_znTi);
	return rc;
}		// DHWTANK::wtInit
//-----------------------------------------------------------------------------
RC DHWTANK::wt_DoHour()			// hourly unfired DHWTANK calcs
// returns RCOK on success, wt_qLoss set
//    else results unusable
{
	RC rc = RCOK;
	wt_qLoss = 0.f;
	return rc;
}		// DHWTANK::wt_DoHour
//-----------------------------------------------------------------------------
RC DHWTANK::wt_DoSubhr(			// subhour DHWTANK calcs
	float tUse)		// system water use temp, F
					// provides default iff wt_tTank not set
{
	RC rc = RCOK;

	// resolve tank temp each hour (DHWSYS.wsTUse can vary hourly)
	float tTank = IsSet(DHWTANK_TTANK)
		? wt_tTank
		: tUse;

	if (wt_pZn)
		wt_tEx = wt_pZn->tzlh;

	// loss rate, Btuh
	wt_qLossSh = wt_UA * (tTank - wt_tEx) + wt_xLoss;

	// total loss (aka HJL in ACM App B)
	wt_qLoss += wt_qLossSh * Top.tp_subhrDur;

	if (wt_pZn)
		wt_pZn->zn_CoupleDHWLossSubhr(wt_qLossSh * wt_mult);

	return rc;
}		// DHWTANK::wt_DoSubhr
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
	[[maybe_unused]] float tdI)	// drain water inlet temp, F
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
		double v = vp/Top.tp_tickDurMin;
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
float DHWPUMP::wp_DoHour(			// hourly DHWPUMP/DHWLOOPPUMP calcs
	int mult,		// system multiplier
					//    DHWPUMP = ws_mult
					//    DHWLOOPPUMP = ws_mult*wl_mult
	float runF/*=1.*/)	// fraction of hour pump runs

// returns heat added to liquid stream, Btu
{
	[[maybe_unused]] RC rc = RCOK;
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
// PIPERUN: simple pipe run (TODO: merge with PIPESEG)
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
//============================================================================
PIPERUN::PIPERUN()		// c'tor
{
	memset(this, 0, sizeof(*this));

}	// PIPERUN::PIPERUN
//----------------------------------------------------------------------------
float PIPERUN::pr_DeriveSizeFromFlow(
	float flow,		// flow rate, gpm
	float desVel)	// design fluid velocity, fps
{
	float flowFt3PerMin = flow / galPerFt3;
	float faceArea = flowFt3PerMin / (60.f * max(0.01f, desVel));
	static const float wallThkns = 0.05;	// guesstimate wall thickness, in
											//   typical value for type L copper
	pr_size = 2.f * (12.f * sqrt(faceArea / kPi) + wallThkns);

	return pr_size;

}		// PIPERUN::pr_DeriveSizeFromFlow
//----------------------------------------------------------------------------
float PIPERUN::pr_GetOD(
	// returns outside diameter of pipe (w/o or w/ insulation), in
	int bInsul) const	// 0: get bare pipe OD
						// 1: get insulation OD
{
	float OD = pr_size + 0.125f;
	if (bInsul)
		OD += 2.f * pr_insulThk;
	return OD;
}		// PIPERUN::pr_GetOD
//----------------------------------------------------------------------------
void PIPERUN::pr_CalcGeom()		// pipe seg derived geometric values
// sets pr_exArea (ft2) and pr_vol (gal)
{
	// initialize SEGTOTS re accum to parents
	//   other members set below
	pr_totals.st_count = 1.;
	pr_totals.st_len = pr_len;

	float r = pr_GetOD(0) / 24.f;	// pipe radius, ft
	// include tube wall in vol, approximates heat cap of tubing
	pr_totals.st_vol = galPerFt3 * kPi * r * r * pr_len;

	double d = pr_GetOD(1) / 12.;
	pr_totals.st_exArea = d * kPi * pr_len;
}		// PIPERUN::pr_CalcGeom
//----------------------------------------------------------------------------
float PIPERUN::pr_CalcUA(		// derive UA
	float fUA /*=1.f*/)		// UA adjustment factor (re e.g. imperfect insul)
// returns UA, Btuh/F
{
	float diaO = pr_GetOD(0);	// bare pipe OD, in
	float Ubare = pr_exH * kPi * diaO / 12.f;

	float Uinsul;
	if (pr_insulThk < .001f)
		Uinsul = Ubare;
	else
	{
		float diaX = pr_GetOD(1);	// insulation OD, in
		double rIns = log(diaX / diaO) / (2.* pr_insulK);	// insulation resistance (per ft thk)
															// pr_insulK units = Btuh-ft/ft2-F
		double rSrf = 12. / (pr_exH * diaX);			// surface restance
		Uinsul = float(kPi / (rIns + rSrf));
#if 0 && defined( _DEBUG)
0		// test code : for diaX >> insulThk, Uround == Uflat approx
0		float Uround = Uinsul / (kPi * diaX / 12.f);
0		float Uflat = 1.f / (pr_insulThk / 12.f / pr_insulK + 1.f / pr_exH);
0		float Udiff = Uround - Uflat;
#endif
	}
	pr_totals.st_UA = pr_len * min(Ubare, fUA*Uinsul);
	return pr_totals.st_UA;
}		// PIPERUN::pr_CalcUA
//----------------------------------------------------------------------------
float PIPERUN::pr_SetBeta(
	float mCp,			// heat capacity rate, Btuh/F
	float fUA /*=1.f*/)		// UA adjustment factor
// beta = (1 - approach to surround) for pipe loss
// sets pr_beta (and returns it)
{
	pr_beta = mCp > .1f
		? exp(max(-80.f, -pr_totals.st_UA * fUA / mCp))
		: 0.f;		// very small air flow
	return pr_beta;
}		// PIPERUN::pr_SetBeta
//----------------------------------------------------------------------------
float PIPERUN::pr_CalcTOut(		// calc outlet or ending temp
	float tIn,			// inlet / beginning temp, F
	float flow)	const	// flow, gpm
// returns outlet / cooldown temp
{
	float tOut = 0.f;  // pr_exT;
	if (flow > .00001f)		// if flow
	{
		double f = exp(-double(pr_totals.st_UA / (flow)));
		tOut += float(f * (tIn - 0.f /* pr_exT*/));
	}
	return tOut;
}		// PIPERUN::ps_CalcTOut
//=============================================================================
void PBC::sb_Init(PIPESEG* pPS)
{
	sb_pPS = pPS;
}		// PBC::sb_Init
//-----------------------------------------------------------------------------
/*virtual*/ double PBC::sb_AreaNet() const		// *outside* duct area
// returns exposed (heat transfer) area
{
	return sb_pPS ? sb_pPS->ps_totals.st_exArea : 0.;
}	// PBC::sb_Area
//-----------------------------------------------------------------------------
/*virtual*/ const char* PBC::sb_ParentName() const
{
	return sb_pPS ? sb_pPS->name : "?";
}		// PBC::sb_ParentName
//-----------------------------------------------------------------------------
/*virtual*/ int PBC::sb_Class() const
{
	return sfcPIPE;
}	// PBC::sb_Class
//=============================================================================

PIPESEG::PIPESEG(basAnc *b, TI i, SI noZ)		// c'tor
	: record(b, i, noZ)
{
	ps_fRhoCpX =		// Btuh/gpm-F
		waterRhoCp	// Btu/gal-F
		* 60.f;		// min / hr
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
	ps_totals.st_vol = galPerFt3 * kPi * r * r * ps_len;
	
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
DHWSYS* DHWLOOPPUMP::wlp_GetDHWSYS() const
{
	DHWLOOP* pWL = wlp_GetDHWLOOP();
	DHWSYS* pWS = pWL->wl_GetDHWSYS();
	return pWS;
}		// DHWLOOPPUMP::wlp_GetDHWSYS
//=============================================================================


// DHWCalc.cpp end
