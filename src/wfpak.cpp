// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// wfpak.cpp -- weather file functions, part 1

/*--------------------------------- INCLUDES ------------------------------- */
#include "CNGLOB.H"	// CSE global definitions -- included first in every module

#include "MSGHANS.H"	// MH_xxxx defns
#include "SRD.H"	// SFIRstr: BC3 needs b4 rccn.h, 2-92
#include "ANCREC.H"	// record: base class for rccn.h classes
#include "EXMAN.H"
#include "rccn.h"	// TOPRAT WFILE WFDATA
#include "PSYCHRO.H"
#include "solar.h"

#include "YACAM.H"	// class YACAM
#include "TDPAK.H"

#if defined( WTHR_T24DLL)
#include "xmodule.h"		// interface to T24WTHR.DLL
#endif

#include "WFPAK.H"

// file constants
const int WFMAXLNLEN = 2000;		// maximum line length supported
									//   for text file formats
const int WFMAXCOLS = 100;			// max # of columns supported


/*--------------------------- LOCAL FUNCTIONS -----------------------------*/
static float CalcGroundTemp( int jD, float taDbAvYr, float taDbRngYr,
	float depth, float soilDiff, int isLeapYr=0);
static float CalcTMainsCEC( int jD, float taDbAvYr, float taDbRngYr,
	float taDbAvg31, int isLeapYr=0);
struct WFHTAB;
LOCAL RC decodeFld( USI dt, SI count, void *srcRec, USI srcOff, USI srcLen,
	void *dest, USI destLen, int erOp, const char *what, const char *which );
static inline int IsMissing( float v)
{
#if 0 && defined( _DEBUG)
x	int bMissing = v < -900.f;
x	if (bMissing)
x		printf( "Missing!\n");
x	return bMissing;
#else
	return v < -900.f;
#endif
}	// IsMissing
//==========================================================================


/*======================= UNMAINTAINED TEST CODE ==========================*/
/* #define TEST */
#ifdef TEST
t //----------------------------------------------------------------------------
t main()
t{
t char *filename="fresno.ca";
t WFILE *wp=NULL;
t WFDATA wd;
t RC rc;
t
t     wp = wfOpen( filename, wp, &rc);
t
t     wfRead(wp,1,0,&wd);
t     wfRead(wp,1,1,&wd);
t     wfRead(wp,3,12,&wd);
t     wfClose(wp);
t
t #if 0
t /* Additional test code of uncertain value.  Moved here from wftest.cpp 3-89 */
t main ()
t {
t struct wthrf *wthrfp, *ptr;
t char fName[20], buffer[200];
t SI jday, ihr, error, q=0, j;
t
t     dminit();
t     for ( ; ; )
t     {	printf("\nfilename : ");
t        gets(fName);
t        if (q)
t           scanf("\n");
t        ptr=NULL;
t        wthrfp = wfOpen( fName, ptr);
t        if (wthrfp->wferror)
t           traceback(wthrfp->wferror);
t        printf("dynflg   = %d\n", (INT)wthrfp->dynflg);
t        printf("nhdszwf  = %d\n", (INT)wthrfp->nhdszwf);
t        printf("wflat    = %f\n", wthrfp->wflat);
t        printf("wflong   = %f\n", wthrfp->wflong);
t        printf("itzhwf   = %d\n", (INT)wthrfp->itzhwf);
t        printf("solflgwf = %d\n", (INT)wthrfp->solflgwf);
t        printf("jhroffwr = %d\n", (INT)wthrfp->jhroffwr);
t        printf("wferror  = %d\n", (INT)wthrfp->wferror);
t        for ( ; ; )
t        {	printf("\nenter jday, ihr : ");
t           j = sscanf(gets(buffer),"%hd%h", &jday, &ihr);
t           if (jday==0)
t              break;
t           error=wfRead(wthrfp,jday,ihr);
t           if (wthrfp->wferror)
t              traceback(wthrfp->wferror);
t           printf("db       = %f\n",wthrfp->db);
t           printf("wb       = %f\n",wthrfp->wb);
t           printf("dp       = %f\n",wthrfp->dp);
t           printf("tsky     = %f\n",wthrfp->tsky);
t           printf("wndSpd   = %f\n",wthrfp->wndSpd);
t           printf("wndDir   = %f\n",wthrfp->wndDir);
t           printf("ccov     = %f\n",wthrfp->ccov);
t           printf("barpres  = %f\n",wthrfp->barpres);
t           printf("bmrad    = %f\n",wthrfp->bmrad);
t           printf("dfrad    = %f\n",wthrfp->dfrad);
t           printf("wferror  = %d\n",(INT)wthrfp->wferror);
t	}
t        error = wfClose(wthrfp);
t        if (wthrfp->wferror)
t           traceback(wthrfp->wferror);
t	}
t;
t #endif      /* additional test code */
t
t}		/* main */
#endif	/* TEST */

#if defined( WTHR_T24DLL)
///////////////////////////////////////////////////////////////////////////////
// class XT24WTHR: interface to T24WTHR.DLL
//    California Title 24 hourly weather data
///////////////////////////////////////////////////////////////////////////////
// T24WTHR.DLL data item indexes
// per Doug Herr documentation, 2-6-2012
const int t24WthrDB = 1;	// dry bulb, F
const int t24WthrWB = 2;	// wet bulb, F
const int t24WthrDP = 3;	// dew point, F
const int t24WthrTG = 4;    // ground temperature, F
const int t24WthrTS = 5;    // Tsky, F
const int t24WthrBR = 6;    // direct normal radiation, Btu/sf
const int t24WthrDR = 7;    // diffuse radiation, Btu/sf
const int t24WthrWS = 8;    // wind speed, miles/hr
const int t24WthrPA = 9;    // atmospheric pressure, mb
const int t24WthrTL7 = 10;  // 7-day average temperature with lag, F
const int t24WthrTL14 = 11; // 14-day average temperature with lag, F
const int t24WthrTL31 = 12; // 31-day average temperature with lag, F
//-----------------------------------------------------------------------------
class XT24WTHR : public XMODULE
{
    typedef int T24InitWthr( int CZ, char* VerMsg, char* ErrMsg);
    typedef int T24InitWthrResearch( char* FILENAME, char* VerMsg, char* ErrMsg);
    typedef float T24HrWthr( int Month, int Day, int Hour, int DataField, char* ErrMsg);
    typedef int T24WthrLocation( char* LocData, char* VerMsg, char* ErrMsg);

private:
	// proc pointers
	T24InitWthr* xw_pT24InitWthr;
	T24InitWthrResearch* xw_pT24InitWthrResearch;
	T24HrWthr* xw_pT24HrWthr;
	T24WthrLocation* xw_pT24WthrLocation;
	char xw_errMsg[ 256];		// error message buffer used by several functions
	CString xw_verMsg;			// DLL version message (returned by T24InitWthr)
								//  e.g. "T24WTHR.CLL ver 2013.W0.07 Douglas Herr/CEC"

public:
	XT24WTHR( const char* moduleName);
	~XT24WTHR();
	virtual void xm_ClearPtrs();
	RC xw_Setup();
	RC xw_Shutdown();
	RC xw_InitWthr( int CZ, int erOp=WRN);
	CString xw_GetVerMsg() const { return xw_verMsg; }
	CString xw_GetErrMsg() const { return xw_errMsg; }
	RC xw_GetHr( int erOp, int iMon, int iDom, int iHr, WFDATA* pwd);
};		// class XT24WTHR
//=============================================================================
static XT24WTHR T24Wthr( "T24WTHR.DLL");		// XT24WTHR object
//=============================================================================
XT24WTHR::XT24WTHR( const char* moduleName)		// c'tor
	: XMODULE( moduleName)
{
	xm_ClearPtrs();
	memset( xw_errMsg, 0, sizeof( xw_errMsg));
	xw_verMsg.Empty();
}
//-----------------------------------------------------------------------------
XT24WTHR::~XT24WTHR()
{
}	// XT24WTHR::~XT24WTHR
//-----------------------------------------------------------------------------
/*virtual*/ void XT24WTHR::xm_ClearPtrs()
{	xw_pT24InitWthr = NULL;
	xw_pT24InitWthrResearch = NULL;
	xw_pT24HrWthr = NULL;
	xw_pT24WthrLocation = NULL;
}		// XT24WTHR::xm_ClearPtrs
//-----------------------------------------------------------------------------
RC XT24WTHR::xw_Setup()
{
	if (xm_LoadLibrary() == RCOK)
	{
		xw_pT24InitWthr = (T24InitWthr*)xm_GetProcAddress( "T24InitWthr");
		xw_pT24InitWthrResearch = (T24InitWthrResearch*)xm_GetProcAddress( "T24InitWthrResearch");
		xw_pT24HrWthr = (T24HrWthr*)xm_GetProcAddress( "T24HrWthr");
#if 0
		xw_pT24WthrLocation = (T24WthrLocation*)xm_GetProcAddress( "T24WthrLocation");
#endif
	}
	return xm_RC;
}		// XT24WTHR::xw_Setup
//-----------------------------------------------------------------------------
RC XT24WTHR::xw_InitWthr(
	int CZ,		// climate zone, 1-16
	int erOp /*=WRN*/)	// msg control
// return RCOK on success
//   else RCBAD (msg'd per erOp, message in xw_errMsg)
{	if (!xw_pT24InitWthr)
		return RCBAD;
	char verMsg[ 256];
	int ret = (*xw_pT24InitWthr)( CZ, verMsg, xw_errMsg);
	int rc;
	if (ret)
	{	xw_verMsg.Empty();
		rc = err( erOp, "T24WTHR.DLL error: %s", xw_errMsg);
	}
	else
	{	xw_verMsg = verMsg;
		rc = RCOK;
	}
	return rc;
}		// XT24WTHR::xw_InitWthr
//-----------------------------------------------------------------------------
RC XT24WTHR::xw_Shutdown()		// finish use of T24WTHR DLL
{
	RC rc = RCOK;
	if (xm_hModule != NULL)
	{	// no DLL shutdown call
		rc = xm_Shutdown();
	}
	return rc;
}		// XT24WTHR::xw_Shutdown
//-----------------------------------------------------------------------------
RC XT24WTHR::xw_GetHr(
	int erOp,		// msg control: WRN, IGN, etc.
					// option bit WF_DSNDAY: causes error message here.
	int iMon,		// month (1-12)
	int iDom,		// day of month (1-31)
	int iHr,		// hour (1-24)
	WFDATA* pwd )	// weather data structure to receive hourly data.
{
	if (!xw_pT24HrWthr)
		return RCBAD;

#if 0
    wd_DNI = wd_bmrad = 0.f;
    wd_DHI = wd_dfrad = 0.f;
    wd_db	 = 0.f;
    wd_wb = 0.f;
    wd_wndDir = 0.f;
    wd_wndSpd = 0.f;
	wd_glrad = 0.f;
	wd_cldCvr = 0.f;
	wd_tSky = 0.f;
	wd_tGrnd = 0.f;
	wd_tMains = 0.f;
	wd_taDp = 0.f;
	wd_taDbPvPk = 0.f;
	wd_taDbAvg = 0.f;
	wd_taDbAvg01 = 0.f;
	wd_taDbAvg07 = 0.f;
	wd_taDbAvg14 = 0.f;
	wd_taDbAvg31 = 0.f;
#endif

struct WFDATAMAP
{	int iFld;			// T24WTHR.DLL field index
	float WFDATA::*pVM;	// where to store value (member pointer)
};
static WFDATAMAP WFDATAMap[] =
{	t24WthrDB,   &WFDATA::wd_db,
	t24WthrWB,   &WFDATA::wd_wb,
	t24WthrDP,   &WFDATA::wd_taDp,
	t24WthrTG,   &WFDATA::wd_tGrnd,
	t24WthrTS,   &WFDATA::wd_tSky,
	t24WthrBR,   &WFDATA::wd_bmrad,
	t24WthrDR,   &WFDATA::wd_dfrad,
	t24WthrWS,   &WFDATA::wd_wndSpd,
	// t24WthrPA,   &WFDATA::wd_?
	t24WthrTL7,  &WFDATA::wd_taDbAvg07,
	t24WthrTL14, &WFDATA::wd_taDbAvg14,
	t24WthrTL31, &WFDATA::wd_taDbAvg31,
				// wd_glrad
				// wd_cldCvr
				// wd_taDbPvPk
				// wd_wndDir
   -1
};

	RC rc = RCOK;
	for (int i=0; ; i++)
	{	WFDATAMAP& wfdm = WFDATAMap[ i];
		if (wfdm.iFld < 0)
			break;
		float v = (*xw_pT24HrWthr)( iMon, iDom, iHr, wfdm.iFld, xw_errMsg);
		if (v > 99990.f)	// if error
			rc |= err( erOp, "Error reading %d", wfdm.iFld);		// TODO
		else
			pwd->*wfdm.pVM = v;
	}
	return rc;

}	// XT24WTHR::xw_GetHr
#endif	// WTHR_T24DLL
//=============================================================================
void WDHR::wd_Init(	// initialize all members
	int options/*=0*/)		// options
							//   0: init to 0
							//   1: init to UNSET
{
	if (!options)
		// bitwise OK (no ptrs)
		memset( (char *)this, 0, sizeof( WDHR));
	else
	{	static const NANDAT unset( UNSET);
		float unsetf;
		VD unsetf = VD unset;
		float* pF0 = &wd_sunupf;
		float* pFN = &wd_tdvElecAvg01;
		int szFloat = pFN - pF0 + 1;
		// attempt to detect layout change (+2 is due to packing)
		ASSERT( szFloat*sizeof( float)+sizeof( wd_tdvElecHrRank)+2==sizeof( WDHR));
		VSet( pF0, szFloat, unsetf);
		wd_sunupf = 0.f;	// unsetf may confuse pre-existing logic
		VZero( wd_tdvElecHrRank, 25);
	}
}			// WDHR::wd_Init
//------------------------------------------------------------------------------
WDHR& WDHR::Copy(
	const WDHR& wdhr,	// source
	int options /*=0*/)	// option bits
						//  do not overwrite solar position data
{	
	if (options&1)
		memcpy( (char *)&wd_db, (const char*)&wdhr.wd_db,
			sizeof( WDHR) - offsetof( WDHR, wd_db));
	else
		memcpy( (char *)this, (const char*)&wdhr, sizeof( WDHR));
	return* this;
}	// WDHR::Copy
//----------------------------------------------------------------------------
RC WDHR::wd_Unpack(		// single-hour unpack
	int iHr,				// hour of day (0-23)
	USI* phour,			// binary data as read from file (4 16 bit words)
	int wFileFormat /*=ET1*/)	// packed file format
// returns RCOK
{

	wd_Init( 1);	// set all to UNSET

	int i = (wFileFormat==BSGSdemo);	// normally fetch words 0,1,2, but to unscramble demo format, fetch 1,2,0
	int w = phour[i++];				// word 0, or 1 if scrambled
	int t = ((USI)w >> 6)-100;		// drybulb+100 is hi 10 bits
	wd_db = (float)t;
	wd_wb = (float)(t-(w & 0x3F));		// lo 6 bits is drybulb - wetbulb

	w = phour[i++];				// word 1, or 2 if scrambled
	wd_bmrad = wd_DNI = (float)((USI)w >> 4);
	wd_wndDir  = (float)(w & 0xF) * 22.5f;

	i = i % 3;					// for demo format change i from 3 to 0; normally moot
	w = phour[i /*++*/ ]; 			// word 2, or 0 if scrambled
	wd_dfrad  = wd_DHI = (float)((USI)w >> 7);
	wd_wndSpd = (float)(w & 0x7F);

	// additional values for ET1 weather file
	if (wFileFormat==ET1)
	{	w = phour[3];
		wd_glrad = (float)((USI)w >> 4);
		wd_cldCvr = (float)(w & 0xF);
		wd_EstimateMissingET1( iHr);
	}
	// else if ...

	return RCOK;
}		// WDHR::wd_UnPack
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// class WDDAY: daily statistics (derived from pre-read of weather file)
// class WDYEAR: full year of weather data
//////////////////////////////////////////////////////////////////////////////
class WDDAY
{
friend class WDYEAR;
friend WDHR;
friend WFILE;
	float wdd_taDbPvPk;		// previous day max dry bulb air temp, F
	float wdd_taDbPk;		// current day max dry bulb air temp, F
	float wdd_taDbAvg;		// this day mean dry bulb air temp, F
							//   used re calc of wdd_taDbAvgXX, wd_taDbAvg01,
	float wdd_tGrnd;		// ground temp, F.  Calculated with Kusuda model
							//   depth=10 ft, soilDiff = per user input
	float wdd_tMains;		// cold water mains temp, F.  calc'd per
							//   CEC 2013 methods, see CalcTMainsCEC()

							// mean dry bulb temp over recent days, F
							// *not* including current day
	float wdd_taDbAvg01;	//    1 day
	float wdd_taDbAvg07;	//    7 day
	float wdd_taDbAvg14;	//   14 day
	float wdd_taDbAvg31;	//   31 day

							// TDV elect statistics
	float wdd_tdvElecPk;	//   peak for cur day
	int wdd_tdvElecPkRank;	//   rank within year of wdd_tdvElecPk (1-366)
	float wdd_tdvElecAvg;	//   avg for cur day
	float wdd_tdvElecPvPk;	//   peak for prior day
	float wdd_tdvElecAvg01;	//   avg for print day
	UCH wdd_tdvElecHrSTRank[ 24];	// TDV hr ranking (*standard time*)
									//   [ 0] = hour (1-24) with highest TDVElec
									//   [ 1] = hour (1-24) with 2nd highest TDVElec
									//   [ 2 .. 23] etc								
public:
	void wdd_Init();
	int wdd_Compare( const WDHR* wdHr) const;
	void wdd_Set24( WDHR* wdHr, int options=0) const;
	static int wdd_TdvPkCompare(const void* p1, const void* p2)
	{	const WDDAY* pD1 = *(WDDAY **)p1;
		const WDDAY* pD2 = *(WDDAY **)p2;
		return -1*LTEQGT( pD1->wdd_tdvElecPk, pD2->wdd_tdvElecPk);
	}
};	// class WDDAY
//-----------------------------------------------------------------------------
struct WDSUBHRFACTORS		// constant factors for weather data interpolation
{
	float f;	// fraction of hour passed at start this subhour
	float g;	// fraction of hour passed at end this subhour
	float h;	// fraction of hour remaining after this subhour
	float slrF[3];	// [ 0] = fraction of previous hour avg value to use in forming subhr avg value
					// [ 1] = .. current ..
					// [ 2] = .. next ..

	void wdsf_Setup(int iSh, int nSh);
}; // struct WDSUBHRFACTORS
//----------------------------------------------------------------------------
struct WDSLRDAY;	// fwd defns
class WDSUBHR;
//----------------------------------------------------------------------------
struct WDSLRHR		// working solar data for 1 hour
{
				// Values set by WDYEAR::wdy_PrepareWDSLRDAY() 
	WDSLRDAY* whs_pSD;		// parent
	const WDHR* whs_pWD;	// source WDHR (NULL if none (e.g. heating design day))
	int whs_iH;				// hour of day (0 - 23)
	int whs_hrTy;			// 0: no sun; 1: sunrise; 2: full hour; 3: sunset
	float whs_hrSunupBeg;	// fractional hour beg of sun up period
	float whs_hrSunupEnd;	// fractional hour end of sun up period

	float whs_hrRad[scCOUNT];	// hour total irradiation, Btu/ft2
								//   from weather file or design day model

				// for SLRINTERPMETH_TRNSYS
	float whs_gEnd[scCOUNT];	// instantaneous irradiance at end of sunup
								//   period for hour, Btuh/ft2

	WDSLRHR& whs_GetHr( int iH);

	void whs_ClearData()
	{	// do not clear whs_iH and linkage ptrs
		whs_hrTy = 0;
		whs_hrSunupBeg = whs_hrSunupEnd = 0.f;
		VZero(whs_hrRad, 2);
		VZero(whs_gEnd, 2);
	}
	void whs_Init(WDSLRDAY* pSD, int iH, const WDHR* pWD);

	RC whs_GenSubhrRad1CSE(WDSUBHR wdsh[], const WDSUBHRFACTORS* wShF);
	RC whs_GenSubhrRad1TRNSYS(WDSUBHR wdsh[]);

};	// struct WDSLRHR
//----------------------------------------------------------------------------
struct WDSLRDAY		// working data re daily solar calcs
{

	WDSLRHR wds_rad[24];	// working data by hour
	int wds_nSubhr;			// # of subhrs per hour (copy of wdy_nSubhr)

	WDSLRDAY() = delete;
	WDSLRDAY(const WDYEAR* pWDY, int dayTy, int jDay);

	void wds_InitHour(int iH, const WDHR* pWD)
	{	wds_rad[iH].whs_Init(this, iH, pWD);
	}
	
};	// struct WDSLRDAY
//----------------------------------------------------------------------------
class WDSUBHR		// derived subhourly weather data
{
public:
	void wdsh_Clear()
	{	VZero( wdsh_rad, SLRCMP::scCOUNT);	}
	float& operator[](SLRCMP iC) { return wdsh_rad[iC]; }
	float wdsh_rad[ SLRCMP::scCOUNT];	// average DHI, DNI for subhour, Btuh/ft2
};		// class WDSUBHR
//----------------------------------------------------------------------------
struct VHR		// sort helper for wdy_Stats
{	double v;		// value
	int iHr;		// hour when value occurs (0 - 23)
	static int Compare( const void* pV1, const void* pV2)
	{	return LTEQGT( ((const VHR *)pV2)->v, ((const VHR *)pV1)->v); }
};	// struct VHR
//----------------------------------------------------------------------------
class WDYEAR		// caches year of weather data re calc of daily stats etc
{
friend class WFILE;
friend struct WDSLRDAY;
	int wdy_isLeap;				// nz: 366 day year
	WDDAY wdy_day[ 366];		// daily statistics ([ 0] = jan-1)
	WDHR wdy_hr[ 24*366];		// hourly data (read from weather file)
								//   all standard time
								//   DST adjustment applied in wdy_TransferHr
	WDHR wdy_hrDsDay[24];		// current design-day data by standard time hour
								//   generated or filled from file data
	int wdy_nSubhr;				// # of subhrs per hour
								//   local copy of Top.tp_nSubSteps
	WDSUBHRFACTORS* wdy_wShF;	// array [nSubhr] of constants re subhr radiation upsampling
								//   constant for run (depends on nSubhr only)
	int wdy_shJday;				// day of year now in wdy_shDay (1-365/366)
								//   < 0 if not set up
	int wdy_shDayTy;			// type of day now in wdy_shDay
								//   gshrFILEDAY, gshrDSDAY, gshrDSDAY0
								//   < 0 if not set up
	int wdy_shErrCount;			// count of solar subhour errors
	WDSUBHR* wdy_shDay;			// subhour solar radiation data for wdy_shJday
	double wdy_taDbAvg[ 13];	// avg daily temp by month, [ 0] = year
	double wdy_tMainsAvg[ 13];	// avg cold water temp by month, [ 0] = year
	float wdy_tMainsMin;		// annual minimum cold water temp, F

public:
	WDYEAR();
	~WDYEAR();
	void wdy_Init( int options=0);
	int wdy_YrDays() const { return wdy_isLeap ? 366 : 365; }
	int wdy_DayIdx(int jDay) const	// day idx with wrap (1-based day-of-year (<1 OK))
	{	// jDay=-1: Dec-30; jDay=0: Dec-31; jDay=1: Jan-1; jDay=2: Jan-2
		int yrDays = wdy_YrDays();
		return (jDay - 1 + yrDays) % yrDays;
	}
	int wdy_HrIdx(int jDay, int iHr) const	// hr idx with wrap (jDay 1 based; iHr 0 based)
	{	int yrHrs = wdy_YrDays() * 24;
		return (24 * (jDay - 1) + iHr + yrHrs) % yrHrs;
	}
	WDHR& wdy_Hr( int jDay, int iHr) { return wdy_hr[ wdy_HrIdx(jDay, iHr)]; }
	WDHR& wdy_HrNoWrap(int jDay, int iHr) { return wdy_hr[ 24*(jDay-1)+iHr]; }
	const WDHR& wdy_Hr( int jDay, int iHr) const { return wdy_hr[ wdy_HrIdx(jDay, iHr)]; }
	WDDAY& wdy_Day( int jDay)		// 1-based day-of-year (<1 OK)
	{	return wdy_day[ wdy_DayIdx( jDay)];
	}
	WDSUBHR& wdy_Subhr(int iHrST, int iSh=0)
	{	return wdy_shDay[iHrST * wdy_nSubhr + iSh];
	}
	RC wdy_Fill( WFILE* pWF, int erOp=WRN);
	RC wdy_TransferHr( int jDay, int iHr, WDHR* pwd, int erOp);
	void wdy_Stats( int jDay, double& taDbMean, double& taDbMax,
		double& tdvElecMean, double& tdvElecMax, UCH tdvElecHrRank[ 24]) const;
	float wdy_TaDbAvg( int jDay1, int jDay2);
#if 0
	RC wdy_PrepareWDSLRDAY( WDSLRDAY& wdsd, int dayTy, int jDay);
#endif
	RC wdy_GenSubhrRadSetup( int nSh);
	RC wdy_GenSubhrRad(SLRINTERPMETH slrInterpMeth, int dayTy, int jDay);
	RC wdy_GenSubhrRad1(WDSUBHR wdsh[], int dayTy, int jDay, int iHr);
	RC wdy_GenSubhrRad1(WDSUBHR wdsh[], WDSLRDAY& wdsd, int iHr);
	RC wdy_GenSubhrRad1TRNSYS(WDSUBHR wdsh[], WDSLRDAY& wdsd, int iHr);
	RC wdy_VerifySubhrRad( const WDSLRDAY& wdsd);
};		// class WDYEAR
//-----------------------------------------------------------------------------
WDYEAR::WDYEAR()			// c'tor
	: wdy_wShF(NULL), wdy_shDay(NULL)
{
	wdy_Init();
}		// WDYEAR::WDYEAR
//-----------------------------------------------------------------------------
WDYEAR::~WDYEAR()			// d'tor
{
	delete[] wdy_wShF;
	delete[] wdy_shDay;
}		// WDYEAR::~WDYEAR
//-----------------------------------------------------------------------------
void WDYEAR::wdy_Init(
	[[maybe_unused]] int options /*=0*/)
{
	wdy_isLeap = 0;		// TODO: handle leap year
	VZero(wdy_taDbAvg, 13);
	VZero(wdy_tMainsAvg, 13);
	for (int iD = 0; iD < 366; iD++)
		wdy_day[iD].wdd_Init();	// note: access via [], not wdy_Day()
									//   avoid wrapping etc so all init'ed()
	wdy_shJday = wdy_shDayTy = -1;	// no subhr day set up
	// don't init wdy_hr[]: very big and will be filled by weather file read

	wdy_shErrCount = 0;

}		// WDYEAR::wdy_Init
//-----------------------------------------------------------------------------
RC WFILE::wf_FillWDYEAR(	// read and unpack weather data for entire file
	int erOp /*=WRN*/)
// Note: no DST awareness / adjustment
// returns RCOK on success
{
	RC rc = RCOK;
	if (!wf_pWDY)
		wf_pWDY = new WDYEAR;
	rc = wf_pWDY->wdy_Fill( this, erOp);

	if (IsMissing( taDbAvgYr))
		taDbAvgYr = wf_pWDY->wdy_taDbAvg[ 0];
	tMainsAvgYr = wf_pWDY->wdy_tMainsAvg[ 0];
	tMainsMinYr = wf_pWDY->wdy_tMainsMin;

	// fill hour values from daily
	int options = 0;		// wdd_Set24: fill always
	int jD;
	for (jD=1; jD <= wf_pWDY->wdy_YrDays(); jD++)
		wf_pWDY->wdy_Day( jD).wdd_Set24( &wf_pWDY->wdy_Hr( jD, 0), options);
#if 0 && defined( _DEBUG)
	if (wFileFormat == CSW)
	{	// CSW data should match calculated
		if (fabs( wf_pWDY->wdy_taDbAvg[ 0] - taDbAvgYr) > .01)
			printf( "Mismatch AvgYr\n");
		int errCount = 0;
		for (jD=1; jD<=yrDays(); jD++)
		{	if (wf_pWDY->wdy_Day( jD).wdd_Compare( &wf_pWDY->wdy_Hr( jD, 0)))
			{	printf( "Mismatch jDay=%d\n", jD);
				if (++errCount > 5)
					break;	// the point has been made, quit
			}
		}
	}
#endif

	// init for subhr interpolation
	rc |= wf_pWDY->wdy_GenSubhrRadSetup( Top.tp_nSubSteps);

	return rc;
}	// WFILE::wf_FillWDYEAR
//------------------------------------------------------------------------------
RC WDYEAR::wdy_Fill(	// read weather data for entire file; compute averages etc.
	WFILE* pWF,
	int erOp /*=WRN*/)
// returns RCOK on success
//    else data is unusable
{
	RC rc = RCOK;
	int iMon;
	for (iMon=0; iMon<13; iMon++)
	{	wdy_taDbAvg[ iMon] = 0.;
		wdy_tMainsAvg[ iMon] = 0.;
	}
	wdy_tMainsMin = 999.f;

	int yrDays = wdy_YrDays();			// TODO: handle leap year
	static const int monLens[ 13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
	int iDay;
	int jDay = 0;
	for (iMon=1; iMon<=12; iMon++)
	{	int monLen = monLens[ iMon]  + (wdy_isLeap && iMon==2);
		for (iDay=0; iDay<monLen; iDay++)
		{	jDay++;
			slday(jDay, sltmLST);
			for (int iHr=0; iHr<24; iHr++)
			{	WDHR* pWd = &wdy_Hr(jDay, iHr);
				rc |= pWF->wf_Read( pWd, jDay, iHr, erOp|WF_FORCEREAD);
				if (rc)
					break;
				pWd->wd_Adjust( iHr);	// apply adjusments per user input
										//   aniso, solar factors, wind factor
			}
			if (rc)
				break;
			double taDbMean, taDbMax, tdvElecMean, tdvElecMax;
			WDDAY& wdd = wdy_Day( jDay);
			wdy_Stats( jDay, taDbMean, taDbMax, tdvElecMean, tdvElecMax, 
				wdd.wdd_tdvElecHrSTRank);

			// current day values
			// next day's previous day values = current day
			WDDAY& wdd1 = wdy_Day( jDay+1);
			wdd.wdd_taDbPk = wdd1.wdd_taDbPvPk = taDbMax;
			wdd.wdd_taDbAvg = wdd1.wdd_taDbAvg01 = taDbMean;
			wdd.wdd_tdvElecPk = wdd1.wdd_tdvElecPvPk = tdvElecMax;
			wdd.wdd_tdvElecAvg = wdd1.wdd_tdvElecAvg01 = tdvElecMean;
			wdy_taDbAvg[ iMon] += taDbMean;		// re monthly average
		}
		wdy_taDbAvg[ 0] += wdy_taDbAvg[ iMon];		// re year avg
		wdy_taDbAvg[ iMon] /= monLen;
	}
	if (rc)
		return rc;

	if (!wdy_isLeap)
	{	// 365 day year: set unused day to UNSET (insurance)
		//   do NOT use wdy_Hr() cuz it wraps
		for (int iHr=0; iHr<24; iHr++)
			wdy_HrNoWrap(366, iHr).wd_Init( 1);
	}

	wdy_taDbAvg[ 0] /= yrDays;	// mean of daily means
	float taDbRngYr				// range based on monthly means
			= VMax( wdy_taDbAvg+1, 12) - VMin( wdy_taDbAvg+1, 12);
	jDay = 0;
	WDDAY* wddRank[366];	// pointers to days re rank sort
	wddRank[365] = NULL;	// insurance, unused unless leap year
	for (iMon=1; iMon<=12; iMon++)
	{	int monLen = monLens[ iMon]  + (wdy_isLeap && iMon==2);
		for (iDay=0; iDay<monLen; iDay++)
		{	jDay++;
			WDDAY& wdd = wdy_Day( jDay);
			wddRank[ jDay-1] = &wdd;
			// lagged average dry-bulb temps: values do not include current day
			// wdd.wdd_taDbAvg01 = wdy_TaDbAvg( jDay-1, jDay-1) -- see above
			wdd.wdd_taDbAvg07 = wdy_TaDbAvg( jDay-7, jDay-1);
			wdd.wdd_taDbAvg14 = wdy_TaDbAvg( jDay-14, jDay-1);
			wdd.wdd_taDbAvg31 = wdy_TaDbAvg( jDay-31, jDay-1);
			wdd.wdd_tGrnd = CalcGroundTemp(
								jDay,				// day of year
								wdy_taDbAvg[ 0],	// air temp annual mean, F
								taDbRngYr,			// air temp annual range
													//   based on monthly means
								10.f,				// depth
								Top.tp_soilDiff);	// soil diffusivity (user input)
			wdd.wdd_tMains = CalcTMainsCEC(
								jDay,				// day of year
								wdy_taDbAvg[ 0],	// air temp annual mean, F
								taDbRngYr,			// air temp annual range
													//   based on monthly means
								wdd.wdd_taDbAvg31);	// lagged 31-day average dry-bulb temp
			wdy_tMainsAvg[ iMon] += wdd.wdd_tMains;
			if (wdd.wdd_tMains < wdy_tMainsMin)
				wdy_tMainsMin = wdd.wdd_tMains;
		}
		wdy_tMainsAvg[ 0] += wdy_tMainsAvg[ iMon];
		wdy_tMainsAvg[ iMon] /= monLen;
	}
	wdy_tMainsAvg[ 0] /= yrDays;

	// sort wddRank pointers by decreasing wdd_tdvElecPk
	qsort( wddRank, yrDays, sizeof(const WDDAY*), WDDAY::wdd_TdvPkCompare);

	// set wdy_day ranks via now-rank-ordered pointers
	for (int i = 1; i <= yrDays; i++)
		wddRank[ i-1]->wdd_tdvElecPkRank = i;

	return rc;
}	// WDYEAR::wdy_Fill
//-----------------------------------------------------------------------------
void WDYEAR::wdy_Stats(			// statistics for day
	int jDay,				// day of year (1-365/366)
	double& taDbMean,		// returned: 24hr mean dry-bulb, F
	double& taDbMax,		// returned: max dry-bulb, F
	double& tdvElecMean,		// returned: mean TDV elec
	double& tdvElecMax,			// returned: max TDV elec
	UCH tdvElecHrSTRank[ 24]) const	// returned: hour ranking of tdvElec
									//   [ 0] = 1-based hour of peak TDV
									//   [ 1] = ditto, next highest
// returns min, mean, and max dry-bulb for day
{
	VHR tdvElecHr[ 24];		// day's tdvElec for hour-rank sort
	taDbMax = tdvElecMax = -9999.;
	taDbMean = tdvElecMean = 0.;
	int iHr;
	for (iHr=0; iHr<24; iHr++)
	{	const WDHR& wd = wdy_Hr( jDay, iHr);
		if (wd.wd_db > taDbMax)
			taDbMax = wd.wd_db;
		taDbMean += wd.wd_db;
		if (ISUNSET( wd.wd_tdvElec))
			tdvElecMax = tdvElecMean = wd.wd_tdvElec;		// first unset yields unset max and mean
		else if (!ISUNSET( tdvElecMax))	// if tdvElec OK so far
		{	if (wd.wd_tdvElec > tdvElecMax)
				tdvElecMax = wd.wd_tdvElec;
			tdvElecMean += wd.wd_tdvElec;
			tdvElecHr[ iHr].v = wd.wd_tdvElec;	// tdvElec for rank-hour sort
			tdvElecHr[ iHr].iHr = iHr;			// corresponding hour
		}
	}
	taDbMean /= 24.;
	if (!ISUNSET( tdvElecMean))
	{	tdvElecMean /= 24.;
		// sort the days tdvElec values is descending order
		qsort( tdvElecHr, 24, sizeof( VHR), VHR::Compare);
		for (iHr=0; iHr<24; iHr++)
			tdvElecHrSTRank[ iHr] = tdvElecHr[ iHr].iHr+1;
	}
	else
		VZero( tdvElecHrSTRank, 24);		// tdvElec values not complete
}		// WDYEAR::wdy_Stats
//-----------------------------------------------------------------------------
float WDYEAR::wdy_TaDbAvg(			// multi-day average dry bulb
	int jDay1,		// 1st day (possibly < 0)
	int jDay2)		// last day (>= jDay1)
{
	double taDbSum = 0;
	for (int jDay=jDay1; jDay<=jDay2; jDay++)
		taDbSum += wdy_Day( jDay).wdd_taDbAvg;
	return float( taDbSum / (jDay2 - jDay1 + 1));
}		// WDYEAR::wdy_TaDbAvg
//-----------------------------------------------------------------------------
RC WDYEAR::wdy_TransferHr(		// transfer cached data to application
	int jDay,		// day sought (1-365/366)
					//   caller handles leap year issues (see wf_Read() WF_USELEAP)
	int iHr,		// standard time hour (0 - 23)
	WDHR* pwd,		// returned: WDHR data for application use
	int erOp)		// options
					//  WF_DSTFIX: get previous-day values from jDay+1 if iHr==23
					//      WHY: During daylight savings, prior hour data is used, resulting
					//			 in prior day for iHrClock=0
					//      Ignored if WF_DSNDAY|WF_FORCEDREAD
					//  WF_SAVESLRGEOM: do NOT overwrite wd_slAzm, wd_slAlt, and wd_sunupf
					//      Ignored if WF_DSNDAY|WF_FORCEDREAD
{
	pwd->Copy( wdy_Hr( jDay, iHr), (erOp&WF_SAVESLRGEOM) != 0);
	if (erOp&WF_DSTFIX)
	{	if (iHr == 23)
		{ 	// hr 23: clock time is beg of 1st hour of next day
			//        use selected daily values from next day
			WDDAY& wdd = wdy_Day( jDay+1);
			pwd->wd_taDbPvPk    = wdd.wdd_taDbPvPk;
			pwd->wd_tdvElecPvPk = wdd.wdd_tdvElecPvPk;
		}
		pwd->wd_ShiftTdvElecHrRankForDST();		// shift TDV rank hrs to DST
												//   to be consistent with $hour
												//   in expressions
	}
	return RCOK;
}		// WDYEAR::wdy_TransferHr
//-----------------------------------------------------------------------------
RC WDYEAR::wdy_GenSubhrRadSetup(		// one-time init for solar upsample
	int nSh)	// # of subhours per hour
				//   *must* = Top.nSubsteps
// returns RCOK iff success
{
	RC rc = RCOK;

	wdy_nSubhr = nSh;
	delete[] wdy_wShF;
	wdy_wShF = new WDSUBHRFACTORS[nSh];
	delete[] wdy_shDay;
	wdy_shDay = new WDSUBHR[ 24*nSh];
	wdy_shJday = wdy_shDayTy = -1;

	for (int iSh = 0; iSh < nSh; iSh++)
		wdy_wShF[iSh].wdsf_Setup(iSh, nSh);

	return rc;

}		// WDYEAR::wdy_GenSubhrRadSetup
//-----------------------------------------------------------------------------
void WDSUBHRFACTORS::wdsf_Setup(		// constant factors re upsampling interpolation
	int iSh,		// subhr, 0 - nSh-1
	int nSh)		// number of subhrs per hour (*must* = Top.tp_nSubSteps)
// factors derived here vary by subhr only
//   (independent of day, hr, ...)
{
	float subhrDur = 1.f / nSh;			// subhour duration, hr
	f = float(iSh) / nSh;		// fraction of hour passed at start this subhour
	g = float(iSh + 1) / nSh;	// fraction of hour passed at end this subhour
	h = 1.f - g;				// fraction of hour remaining after this subhour

	// setup for interpolating subhour average value for data with average (integrated) values in file:
	//  Determine a = fraction previous hour, b = fraction current hour, c = fraction next hour  even if subhr overlaps middle hour.
	// The simple cases are:
	//   subhr in 1st half hour: use part prev, part curr hour:   a = .5 - m;  b = .5 + m;   c = 0;
	//   subhr in 2nd half hour: use part curr, part next hour:   a = 0;       b = 1.5 - m;  c = m - .5;
	// where  m = (f + g)/2. Use weighted sum when subhr overlaps middle of hour. */

	float f1 = min(f, .5f), g1 = min(g, .5f);		// start and end of portion of subhr in 1st half of hour
	float f2 = max(f, .5f), g2 = max(g, .5f);		// .. in 2nd half of hour
	float fr1 = (g1 - f1) / subhrDur, fr2 = (g2 - f2) / subhrDur; 	// fractions of subhr in 1st and 2nd halves of hour
	float m1 = .5f*(f1 + g1), m2 = .5f*(f2 + g2);	// middle of portions of subhr in 1st and 2nd halves of hour

	slrF[0] = (.5f - m1)*fr1;						// fraction of previous hour avg value to use in forming subhr avg value
	slrF[1] = (.5f + m1)*fr1 + (1.5f - m2)*fr2;		// .. current ..
	slrF[2] = (m2 - .5f)*fr2;						// .. next ..

	// asserts when nSh = 240; nSh = 120 OK.  MAXSUBSTEPS=120 defined in cndefns.h
	ASSERT(fabs(slrF[0] + slrF[1] + slrF[2] - 1.f) < 1.e-5f);			// a + b + c should be 1.0 within limits of float arithmetic

}	// WDSUBHRFACTORS::wdsf_Setup
//-----------------------------------------------------------------------------
RC WFILE::wf_GenSubhrRad(		// generate subhr solar values for entire day
	SLRINTERPMETH slrInterpMeth,	// subhr interpolation method
	int dayTy,		// day type: gshrFILEDAY, gshrDSDAY, gshrDSDAY0
	int jDay)		// day of year, 1-365/366
// returns RCOK iff subhour data successfully generated
{
	RC rc = wf_pWDY ? wf_pWDY->wdy_GenSubhrRad(slrInterpMeth, dayTy, jDay) : RCBAD;
	return rc;
}		// WFILE::wf_GenSubhrSolar
//-----------------------------------------------------------------------------
RC WDYEAR::wdy_GenSubhrRad(		// generate subhr radiation values for day
	SLRINTERPMETH slrInterpMeth,	// subhr interpolation method
	int dayTy,	// gshrFILEDAY, gshrDSDAY, gshrDSDAY0
	int jDay)	// day of year (1-365/366)
// NOPs if jDay already set up
// returns RCOK iff success
{
#if 0 && defined( _DEBUG)
	if (Top.jDay == 121)
		printf("\nHit %s", Top.dateStr);
#endif
	RC rc = RCOK;
	if (wdy_shJday != jDay || wdy_shDayTy != dayTy)
	{	WDSLRDAY wdsd( this, dayTy, jDay);
		for (int iHr = 0; iHr < 24; iHr++)
		{	WDSUBHR* pWDSH = &wdy_Subhr(iHr, 0);
			WDSLRHR& wdsh = wdsd.wds_rad[iHr];
			if (slrInterpMeth == C_SLRINTERPMETH_TRNSYS)
				rc |= wdsh.whs_GenSubhrRad1TRNSYS( pWDSH);
			else
				rc |= wdsh.whs_GenSubhrRad1CSE(pWDSH, wdy_wShF);
		}
		wdy_shJday = jDay;
		wdy_shDayTy = dayTy;
#if defined( _DEBUG)
		if (!rc)
			rc = wdy_VerifySubhrRad( wdsd);
#endif
	}
	return rc;
}		// WDYEAR::wdy_GenSubhrRad
//-----------------------------------------------------------------------------
RC WDYEAR::wdy_VerifySubhrRad(
	const WDSLRDAY& wdsd)
{
	static int SHERRCOUNTMAX = 20;
	RC rc = RCOK;
	for (int iHr = 0; iHr < 24; iHr++)
	{
		float totSh[scCOUNT] = { 0.f };
		for (int iSh = 0; iSh < wdy_nSubhr; iSh++)
		{
			WDSUBHR& wdsh = wdy_Subhr(iHr, iSh);
			totSh[ scDIFF] += wdsh[ scDIFF];
			totSh[ scBEAM] += wdsh[ scBEAM];
		}

		if (wdsd.wds_rad[ iHr].whs_hrTy > 0)
		{
			for (SLRCMP iC : { scDIFF, scBEAM })
			{
				float totHrC = wdsd.wds_rad[iHr].whs_hrRad[iC];
				float totShC = totSh[iC] / wdy_nSubhr;
				float fDiff = frDiff(totHrC, totShC);
				if (fDiff > .001f)
				{	++wdy_shErrCount;
					if (wdy_shErrCount <= SHERRCOUNTMAX || fDiff > 0.05f)
						rWarn("%s solar subhr mismatch (totHr=%0.2f  totSh=%0.2f)%s",
							iC ? "Beam" : "Diffuse", totHrC, totShC,
							wdy_shErrCount == SHERRCOUNTMAX
								? "\n    Skipping further messages for minor errors." : "");
				}
			}
		}
	}

	return rc;
}		// WDYEAR::wdy_VerifySubhrRad
//-----------------------------------------------------------------------------
WDSLRDAY::WDSLRDAY(		// c'tor: prepare day's solar data for subhr processing
	const WDYEAR* pWDY,	// source WDYEAR
	int dayTy,		// generation options
					//   gshrDSDAY0: set rad values to 0 (for heating design day)
					//   gshrDSDAY: use design day hourly data
					//   gshrFILEDAY: use hourly data from weather file
	int jDay)		// day of year (1-365/366)
{

	wds_nSubhr = pWDY->wdy_nSubhr;

	// fill hour average data
	for (int iH = 0; iH < 24; iH++)
	{
		const WDHR* pWDH =
			dayTy == WFILE::gshrDSDAY ? &pWDY->wdy_hrDsDay[iH]
			: dayTy == WFILE::gshrDSDAY0 ? NULL
			: &pWDY->wdy_Hr(jDay, iH);
		wds_InitHour(iH, pWDH);
	}
}		// WDSLRDAY::WDSLRDAY
//-----------------------------------------------------------------------------
void WDSLRHR::whs_Init(		// init working data for hour
	WDSLRDAY* pSD,	// parent
	int iH,			// hour (0 - 23)
	const WDHR* pWD)	// source weather data
						//   if NULL, init to 0 (re heating design days)
{
	whs_iH = iH;
	whs_pSD = pSD;
	whs_pWD = pWD;

	whs_ClearData();

	if (pWD)
	{
		whs_hrTy = pWD->wd_SunupBegEnd(iH, whs_hrSunupBeg, whs_hrSunupEnd);

		whs_hrRad[scDIFF] = pWD->wd_dfrad;
		whs_hrRad[scBEAM] = pWD->wd_bmrad;
	}

}		// WDSLRHR::whs_Init
//-----------------------------------------------------------------------------
WDSLRHR& WDSLRHR::whs_GetHr( int iH)	// retrieve WDSLRHR for hour of current day
// handles wrap (iH may be <0 or >23)
{
	return whs_pSD->wds_rad[(iH+24) % 24];
}	// WDSLRHR::whs_GetHr
//-----------------------------------------------------------------------------
RC WDSLRHR::whs_GenSubhrRad1TRNSYS(		// subhr interpolation, TRNSYS algorithm
	WDSUBHR wdsh[])		// returned: avg solar subhr values for hour
{
	RC rc = RCOK;

	int nSubhr = whs_pSD->wds_nSubhr;

	// set results to 0
	int iSh;
	for (iSh = 0; iSh < nSubhr; iSh++)
		wdsh[iSh].wdsh_Clear();
	
	if (!whs_hrTy)
		return rc;		// sun not up

	float g[ MAXSUBSTEPS+1];	// calculated instantaneous irrad at subhour boundaries, Btuh/ft2
						//   required size = nSubhr+1, use big enuf, don't check
						//   vZero()d below

	float subhrDur = 1.f / float(nSubhr);

	float sunupDur = whs_hrSunupEnd - whs_hrSunupBeg;
	int iShBeg = int((whs_hrSunupBeg - whs_iH) / subhrDur);
	float fShBeg = float(iShBeg) / nSubhr;
	int iShEnd = iShBeg + int(ceil( sunupDur / subhrDur));		// next sh after end
	float fShEnd = float(iShEnd) / nSubhr;
	int nShSunup = iShEnd - iShBeg;
	float sunupShDur = nShSunup * subhrDur;	// duration of sunup subhrs
#if defined( _DEBUG)
	if (iShBeg < 0 || iShEnd > nSubhr+1 || iShBeg >= iShEnd || nShSunup > nSubhr)
		printf("\nSubhr range fubar");
#endif

// debug aid: track case being handled for msgs
	const char* tag[scCOUNT] = { "   ", "   "};
#if defined( _DEBUG)
#define TAG( s) tag[ iC] = s
#else
// do nothing in release
#define TAG( s)
#endif


	for (SLRCMP iC : { scDIFF, scBEAM })
	{	// instantaneous irrad at beg, mid, end of sunup interval
		float gBeg = whs_GetHr(whs_iH - 1).whs_gEnd[ iC];
		float gMid = 0.f;
		float gEnd = 0.f;
		VZero(g, nSubhr + 1);		// instantaneous irrad at subhr boundaries
		g[iShBeg] = gBeg;			// initial point from end of last hr

		float hrRad = whs_hrRad[iC];			// hr irradiation, Btu/ft2
		float irrad2 = 2.f * hrRad / sunupShDur;	// 2x irradiance while sun up, Btuh/ft2

		if (nShSunup == 1)
		{	// sun up for 1 substep: force total match
			g[ whs_hrTy == 1 ? iShEnd : iShBeg] = irrad2;
			TAG("1up");
		}
		else
		{	// sun up for during 2 or more subhrs
#if 1
			if (whs_hrTy < 3)
			{
				float hrRadNx = whs_GetHr(whs_iH + 1).whs_hrRad[iC];
				gEnd = 0.5f * (hrRad + hrRadNx);
#if defined( _DEBUG)
				if (whs_hrTy == 1)
					TAG(" su");
#endif
			}
			else
			{	// else sunset gEnd = 0 per above
				TAG(" sd");
			}
#else
			float hrRadNx = whsNext.whs_hrRad[iC]
			gEnd = 0.5f * (hrRad + hrRadNx);

			if (whs_hrTy == 1)
			{	// sunrise hour
				if (true || hrRadNx >= hrRad)
				{
					// gEnd = 2.f * hrRad / sunupDur;
					gEnd = 0.5f * (hrRad + hrRadNx);
					TAG("su+");
				}
				else
				{
					gEnd = (0.25f * hrRad + 0.75f * hrRadNx) / sunupDur;
					TAG("su-");
				}
			}
			else if (whs_hrTy == 2)
				gEnd = 0.5f * (hrRad + hrRadNx);
			// else whs_hrTy == 3 and gEnd == 0. per whs_Clear() above
#endif

			whs_gEnd[iC] = gEnd;		// save ending irrad for beg of next hour

			if (hrRad < .01f)
			{	// no irrad
				// wdsh[..][iC] = 0 (due to whs_Clear() above)
				// whs_gEnd[iC] is set (for next hr)
				TAG("R=0");
				continue;		// nothing further needed
			}
			g[iShEnd] = gEnd;

			int iShMid = nShSunup / 2;		// integer midpoint of sunup period
			float fMidSunup = float(iShMid) / float(nShSunup);
			gMid = irrad2 - fMidSunup * gBeg - (1.f - fMidSunup) * gEnd;

			if (gMid < 0.f)
			{
				float gx = ((2.f * hrRad / subhrDur) - gBeg - gEnd)
					/ (2.f * (nShSunup - 1));
				if (gx < 0.f)
				{
					gx = 0.f;
					TAG("fm0");
					// adjust end points to force match to total
					float r = (2.f * hrRad / subhrDur) / (gBeg + gEnd);
					g[iShBeg] *= r;
					g[iShEnd] *= r;
				}
				else
					TAG("fmx");
				for (iSh=iShBeg+1; iSh < iShEnd; iSh++)
					g[iSh] = gx;

			}
			else
			{	// interpolate to find instantaneous gains on subhr boundaries
				float fShMid = float(iShMid + iShBeg) / float(nSubhr);
				for (iSh = iShBeg + 1; iSh < iShEnd; iSh++)
				{
					float f;
					float fSh = float(iSh) / nSubhr;
					if (fSh < fShMid)
					{	f = (fSh - fShBeg) / (fShMid - fShBeg);
						g[iSh] = f * gMid + (1.f - f) * gBeg;
					}
					else
					{	f = (fSh - fShMid) / (fShEnd - fShMid);
						g[iSh] = f * gEnd + (1.f - f) * gMid;
					}
				}

#if 0
// experiment: average (smooths peaks)
				float gx[100];
				VCopy(gx, nSubhr + 1, g);
				for (iSh = iShBeg + 2; iSh < iShEnd - 1; iSh++)
					g[iSh] = 0.25f * (gx[iSh - 2] + gx[iSh - 1] + gx[iSh + 1] + gx[iSh + 2]);
#endif
			}
		}

		// result: derived subhr avg values from instantaneous
		for (iSh = iShBeg; iSh < iShEnd; iSh++)
		{	float radAvg = 0.5f * (g[iSh] + g[iSh + 1]);
			wdsh[iSh][iC] = radAvg;
		}

		
	}

#if defined( _DEBUG)
	// check: subhr sum should equal hour
	for (SLRCMP iC : { scDIFF, scBEAM })
	{
		float radShSum = 0.f;
		for (iSh = 0; iSh < nSubhr; iSh++)
			radShSum += wdsh[iSh][iC];

		float totSh = radShSum / nSubhr;
		float totHr = whs_hrRad[iC];
		float fDiff = frDiff(totSh, totHr);
		if (fDiff > 0.001f)
			printf("\nMismatch  hrTy=%d   iC=%d  tag=%s, nShU=%d  totSh=%0.2f  totHr=%0.2f", whs_hrTy, iC, tag[iC], nShSunup, totSh, totHr);
	}
#endif


	return rc;

}	// WDSLRHR::whs_GenSubhrRad1TRNSYS
//-----------------------------------------------------------------------------
RC WDSLRHR::whs_GenSubhrRad1CSE(		// generate solar subhr values for 1 hour, CSE algorithm
	WDSUBHR wdSh[],		// returned: subhour average irradiation values
	const WDSUBHRFACTORS* wShF)  // array[nSubhr] of constants re subhr radiation upsampling

// returns RCOK iff success
{
	RC rc = RCOK;

	int nSubhr = whs_pSD->wds_nSubhr;

	// hourly data for prev hour, this hr, next hr
	float radBeam[3];	//   beam
	float radDiff[3];	// diffuse
	for (int id = 0; id < 3; id++)
	{
		const WDSLRHR& whx = whs_GetHr(whs_iH + id - 1);
		radBeam[id] = whx.whs_hrRad[scBEAM];
		radDiff[id] = whx.whs_hrRad[scDIFF];
	}

	if (radBeam[1] < .1f && radDiff[1] < .1f)
	{
		for (int iSh = 0; iSh < nSubhr; iSh++)
			wdSh[iSh].wdsh_Clear();
	}
	else
	{	// Compute adjustment factors (constant over hour)
		//  make subhour values add up to given hour value.
		// See Rob's NILES106.TXT, 1-95.
		double beamAdj = radBeam[1] == 0.f ? 0.f : 							// prevents /0
			8 * radBeam[1] / (radBeam[0] + 6 * radBeam[1] + radBeam[2]);

		double diffAdj = radDiff[1] == 0.f ? 0.f :
			8 * radDiff[1] / (radDiff[0] + 6 * radDiff[1] + radDiff[2]);

		for (int iSh = 0; iSh < nSubhr; iSh++)
		{	// note: see also TOPRAT::tp_WthrBegSubhr()
			//  interpolation of data with instantenous end-hour values (e.g. temperatures)
			//  is done there

			// Interpolate subhour average power values for data with average values in file.
			//   Multiply by subhrDur for energy.
			const float* slrF = wShF[iSh].slrF;
			wdSh[iSh][scBEAM] = VIProd(radBeam, 3, slrF) * beamAdj;
			wdSh[iSh][scDIFF] = VIProd(radDiff, 3, slrF) * diffAdj;
		}
	}

	return rc;
}		// WDSLRHR::whs_GenSubhrRad1CSE
//-----------------------------------------------------------------------------
RC WFILE::wf_GetSubhrRad(		// retrieve subhr solar values
	[[maybe_unused]] int jDay,	// day of year, 1 - 365/366
	int iHrST,	// standard time hour of day, 0 - 23
	int iSh,	// substep, 0 - tp_nSubSteps-1
	float& radBeamAv,		// returned: avg DNI for subhr, Btuh/ft2
	float& radDiffAv)		// returned: avg DHI for subhr, Btuh/ft2
{
	RC rc = RCOK;
	const WDSUBHR& wdsh = wf_pWDY->wdy_Subhr(iHrST, iSh);
	radBeamAv = wdsh.wdsh_rad[scBEAM];
	radDiffAv = wdsh.wdsh_rad[scDIFF];
	return rc;
}	// WFILE::wf_GetSubhrRad
//=============================================================================
void WDDAY::wdd_Init()
{	
	wdd_taDbPk = 0.f;
	wdd_taDbAvg = 0.f;
	
	wdd_taDbPvPk = 0.f;
	wdd_taDbAvg01 = 0.f;

	wdd_taDbAvg07 = 0.f;
	wdd_taDbAvg14 = 0.f;
	wdd_taDbAvg31 = 0.f;
	
	wdd_tdvElecPk = 0.f;
	wdd_tdvElecPkRank = 0;
	wdd_tdvElecAvg = 0.f;
	wdd_tdvElecPvPk = 0.f;
	wdd_tdvElecAvg01 = 0.f;
	VZero( wdd_tdvElecHrSTRank, 24);

	wdd_tGrnd = 0.f;
	wdd_tMains = 0.f;

}	// WDDAY::wdd_Init
//-----------------------------------------------------------------------------
int WDDAY::wdd_Compare(		// check that hour values match daily
	const WDHR* wdHr) const
{
	int diffCount = 0;
	for (int iHr=0; iHr<24; iHr++)
	{	const WDHR& wd = wdHr[ iHr];
		if ( fabs( wd.wd_taDbPvPk - wdd_taDbPvPk) > .01)
			diffCount++;
		if ( fabs( wd.wd_taDbAvg - wdd_taDbAvg) > .01)
			diffCount++;
		if ( fabs( wd.wd_taDbAvg07 - wdd_taDbAvg07) > .01)
			diffCount++;
		if ( fabs( wd.wd_taDbAvg14 - wdd_taDbAvg14) > .01)
			diffCount++;
		if ( fabs( wd.wd_taDbAvg31 - wdd_taDbAvg31) > .01)
			diffCount++;
		if ( fabs( wd.wd_tGrnd - wdd_tGrnd) > .01)
			diffCount++;
	}
	return diffCount;
}		// WDDAY::wdd_Compare
//-----------------------------------------------------------------------------
void WDDAY::wdd_Set24(		// set hourly values from daily
	WDHR* wdHr,					// 1st hour data for day
	int options /*=0*/)	const	// option bits
								//   1: set iff destination is "missing"
// WHY: values that change daily are copied to all hours of day.
//      Facilitates probing
{
	if (!options)
	{	for (int iHr=0; iHr<24; iHr++)
			wdHr[ iHr].wd_SetDayValues( *this);
	}
	else
	{	for (int iHr=0; iHr<24; iHr++)
			wdHr[ iHr].wd_SetDayValuesIfMissing( *this);
	}
}		// WDDAY::wdd_Set24
//-----------------------------------------------------------------------------
void WDHR::wd_SetDayValues(		// set daily members for this hour
	const WDDAY& wdd)			// source
// note: no DST awareness / adjustment
{
	wd_taDbPk      = wdd.wdd_taDbPk;
	wd_taDbAvg     = wdd.wdd_taDbAvg;
	wd_taDbPvPk    = wdd.wdd_taDbPvPk;
	wd_taDbAvg01   = wdd.wdd_taDbAvg01;
	wd_taDbAvg07   = wdd.wdd_taDbAvg07;
	wd_taDbAvg14   = wdd.wdd_taDbAvg14;
	wd_taDbAvg31   = wdd.wdd_taDbAvg31;
	wd_tGrnd       = wdd.wdd_tGrnd;
	wd_tMains      = wdd.wdd_tMains;
	wd_tdvElecPk   = wdd.wdd_tdvElecPk;
	wd_tdvElecPkRank = wdd.wdd_tdvElecPkRank;
	wd_tdvElecAvg  = wdd.wdd_tdvElecAvg;
	wd_tdvElecPvPk = wdd.wdd_tdvElecPvPk;
	wd_tdvElecAvg01= wdd.wdd_tdvElecAvg01;
	wd_SetTdvElecHrRank( wdd);
}	// WDHR::wd_SetDayValues
//-----------------------------------------------------------------------------
void WDHR::wd_SetDayValuesIfMissing(		// set missing daily members
	const WDDAY& wdd)			// source
// note: no DST awareness / adjustment
{
#define SETIF( d, s) if (IsMissing( d)) d = s;
	SETIF( wd_taDbPk,		wdd.wdd_taDbPk)
	SETIF( wd_taDbAvg,		wdd.wdd_taDbAvg)
	SETIF( wd_taDbPvPk,		wdd.wdd_taDbPvPk)
	SETIF( wd_taDbAvg01,	wdd.wdd_taDbAvg01)
	SETIF( wd_taDbAvg07,	wdd.wdd_taDbAvg07)
	SETIF( wd_taDbAvg14,	wdd.wdd_taDbAvg14)
	SETIF( wd_taDbAvg31,	wdd.wdd_taDbAvg31)
	SETIF( wd_tGrnd,		wdd.wdd_tGrnd)
	SETIF( wd_tMains,		wdd.wdd_tMains)
	SETIF( wd_tdvElecPk,	wdd.wdd_tdvElecPk)
	SETIF( wd_tdvElecAvg,	wdd.wdd_tdvElecAvg)
	SETIF( wd_tdvElecPvPk,	wdd.wdd_tdvElecPvPk)
	SETIF( wd_tdvElecAvg01,	wdd.wdd_tdvElecAvg01)
	wd_SetTdvElecHrRank( wdd);
#undef SETIF
}	// WDHR::wd_SetDayValuesIfMissing
//----------------------------------------------------------------------------
void WDHR::wd_SetTdvElecHrRank( const WDDAY& wdd)
{	wd_tdvElecHrRank[ 0] = 0;	// unused
	VCopy( wd_tdvElecHrRank+1, 24, wdd.wdd_tdvElecHrSTRank);
}	/// WDHR::wd_SetTdvElecHrRank
//----------------------------------------------------------------------------
void WDHR::wd_ShiftTdvElecHrRankForDST()		// handle DST
{
	for (int iHr=1; iHr<=24; iHr++)
	{	if (++wd_tdvElecHrRank[ iHr] == 25)
			wd_tdvElecHrRank[ iHr] = 1;		// wrap
	}
}		// WDHR::wd_ShiftTdvElecHrRankForDST
//----------------------------------------------------------------------------
bool WDHR::wd_HasTdvData() const	// true iff TDV data is present
// California hourly "Time-Dependent Valuation" data are optionally read,
//   see Top.tp_TDVfName
// returns true iff this hour has populated TDV values
{
	return !ISUNSET( wd_tdvElec);

}	// WDHR::wd_HasTdvData
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// class WFDATA: one hour's data in internal format (as used by simulation)
//////////////////////////////////////////////////////////////////////////////
void WFDATA::wd_Init()	// WFDATA initialization function to call before each run

// note also CSE record may be 0'd by base c'tor and/or statSetup, but not necessarily
//   done between runs after bad run in multi-run versions incl DLL version of CSE.
//   DO NOT memset to 0 here: would destroy record base class header members.
{
	d.wd_Init();
}			// WFDATA::wd_Init()
//-----------------------------------------------------------------------------
RC WDHR::wd_CSWReadHr(			// read 1 hour's data from CSW weather file
	WFILE* pWF,			// source file (open, header read, )
	int jHr,			// hour-of-year sought, 1-8760 (or 8784 TODO)
	int erOp /*=WRN*/)	// err msg control
{

#if 0
	"Month"
	"Day"
	"Hour"
	"TDV Elec"
	"TDV NatGas"
	"TDV Propane"
	"Dry Bulb"
	"Wet Bulb"
	"Dew Point"
	"31-day Avg lag DB"
	"14-day Avg lag DB"
	"7-day Avg lag DB"
	"T Ground"
	"previous day's peak DB"
	"T sky"
	"Wind direction"
	"Wind speed"
	"Global Horizontal Radiation"
	"Direct Normal Radiation"
	"Diffuse Horiz Radiation"
	"Total Sky Cover"
#endif
	int m, d, h;
	RC rc = RCOK;
	while (1)
	{	wd_Init( 1);	// set all to UNSET
		rc = pWF->yac->getLineCSV( erOp, pWF->isLeap, "LLLXXXFFFXXXXXFFFFFFF",
			&m, &d, &h,			// month / day / hour (all 1-based)
			// skip, skip, skip,	// "TDV Elec", "TDV NatGas", "TDV Propane"
			&wd_db, &wd_wb, &wd_taDp,	// "Dry Bulb", "Wet Bulb", "Dew Point"
			// skip &wd_taDbAvg31,		// "31-day Avg lag DB"
			// skip &wd_taDbAvg14,		// "14-day Avg lag DB"
			// skip &wd_taDbAvg07,		// "7-day Avg lag DB"
			// skip &wd_tGrnd,				// "T Ground"
			// skip &wd_taDbPvPk,	// "previous day's peak DB"
			&wd_tSky,				// "T sky"
			&wd_wndDir, &wd_wndSpd,	// "Wind direction", "Wind speed"
			&wd_glrad,				// "Global Horizontal Radiation",
			&wd_DNI, &wd_DHI,		// "Direct Normal Radiation", "Diffuse Horiz Radiation"
			&wd_cldCvr,				// "Total Sky Cover"
			NULL);
		wd_bmrad = wd_DNI;
		wd_dfrad = wd_DHI;

 		if (rc != RCBAD2)
		{	if (rc)
				return pWF->yac->errFlLn("Unreadable data");		// issue msg per last get's erOp with file & line, ret RCBAD

			int jDayFile = tddDoyMonBeg( m) + d - 1;
			int jHrFile = (jDayFile-1)*24 + h;		// hour of year
			if (jHrFile == jHr)
				break;
			else if (jHrFile < jHr)
				continue;
		}
		pWF->wf_CSWPosHr1( erOp);
	}

	if (!rc && IsMissing( wd_tSky))
		wd_tSky = wd_CalcSkyTemp( C_SKYMODLWCH_BERDAHLMARTIN, h-1);

	return rc;
}		// WDHR::wd_CSWReadHr
//-----------------------------------------------------------------------------
RC WDHR::wd_TDVReadHr(			// read 1 hour's data from CSW weather file
	WFILE* pWF,			// source file (open, header read, )
	int jHr,			// hour-of-year sought, 1-8760 (or 8784 TODO)
	int erOp /*=WRN*/)	// err msg control
{

	RC rc = RCOK;
	while (1)
	{	rc = pWF->yacTDV->getLineCSV( erOp, pWF->isLeap, "FF",
		    &wd_tdvElec, &wd_tdvFuel, NULL);

 		if (rc != RCBAD2)
		{	if (rc)
				return pWF->yacTDV->errFlLn("Unreadable data");		// issue msg per last get's erOp with file & line, ret RCBAD

			pWF->wf_TDVFileJHr++;
			if (pWF->wf_TDVFileJHr == jHr)
				break;
			else if (pWF->wf_TDVFileJHr < jHr)
				continue;
		}
		pWF->wf_TDVPosHr1( erOp);
	}

	return rc;
}		// WDHR::wd_TDVReadHr
//-----------------------------------------------------------------------------
float CalcGroundTemp(			// ground temperature model
	int jD,				// day of year (1-365/366)
	float taDbAvYr,		// annual average temperature, F
	float taDbRngYr,	// annual temperature range, F
						//   = warmest month taDbAvg - coldest month taDbAvg
	float depth,		// calculation depth, ft
	float soilDiff,		// soil diffusivity, ft2/hr
	int isLeapYr /*=0*/)	// nz: base calc on 366 days, else 365
// uses kusuda and Achenbach (1965) method
// returns ground temp for day of year at specified depth, F
{
	int yrDays = isLeapYr ? 366 : 365;

	double beta = depth * sqrt( kPi / (max( soilDiff, .001) * yrDays*24.));

	double cosB = cos( beta);
	double sinB = sin( beta);
	double expMB = exp( -beta);

	double gm = sqrt( (expMB*expMB - 2.*expMB*cosB + 1.) / (2.*beta*beta));
	double phi = atan2( 1.-expMB*(cosB+sinB), 1.-expMB*(cosB-sinB));
	double ang = k2Pi * (jD-1)/yrDays	// time of year, 0 - 2Pi
				 - 0.6		// phase angle for ground surf temp (Kusuda)
				 - phi;		// phase angle for depth averaged ground temp
	double tGrnd = taDbAvYr - gm * taDbRngYr * cos( ang) / 2.;
	return float( tGrnd);
}		// CalcGroundTemp
//-----------------------------------------------------------------------------
float CalcTMainsCEC(
	int jD,				// day of year (1-365/366)
	float taDbAvYr,		// annual average temperature, F
	float taDbRngYr,	// annual temperature range, F
						//   = warmest month taDbAvg - coldest month taDbAvg
	float taDbAv31,		// average dry bulb, last 31 days
	int isLeapYr /*=0*/)	// nz: base calc on 366 days, else 365
// Method = 2013 Res ACM manual, App E, Eq RE-9
{
	// ground temp at 10 ft
	//   soil diffusivity: ACM App E says 0.0435, but T24DHW.DLL uses
	//      weather file value calculated with diff=0.025
	//   Note also that ACM App E calc is hourly, here we do daily.
	//      CSW weather file values are also daily.
	float soilDiff =
		0.025f;		// always 0.025 pending rule finalization

#if 0 && defined( _DEBUG)
0	static int jDTrap = -99;
0	if (jD == jDTrap)
0		printf( "\nHit!");
#endif

	float tG = CalcGroundTemp( jD, taDbAvYr, taDbRngYr, 10.f, soilDiff, isLeapYr);

	float tMains = 0.65f*tG + 0.35f*taDbAv31;

	return tMains;

}		// CalcTMainsCEC
//-----------------------------------------------------------------------------
float CalcSkyTemp(		// sky temp model
	int skyModelLW,	// model choice
					//   C_SKYMODLWCH_BERDAHLMARTIN
					//	 C_SKYMODLWCH_BLAST
					//	 C_SKYMODLWCH_DRYBULB
	int iHr,		// hour of day (0 - 23, 0=midnight - 1 AM)
	float taDb,		// dry bulb temp, F
	float taDp,		// dew point temp, F
	float cldCvr,	// total cloud cover, tenths (0.f - 10.f)
	float presAtm	// atmospheric pressure, in Hg
	/*float ceilHt*/)		// TODO


// TODO
//   deal with total vs. opaque sky cover
//   handle ceiling height?

// returns sky temp (F) for *end* of hour (iHr=0 returns temp for 1 AM)

{
	// constants used only for Berdhahl-Martin
	//   set up for all
	static float hrConst[ 24] = { 0.f };
	static int isSetup = 0;
	if (!isSetup)
	{	// set constants only once
		for (int iHx=0; iHx<24; iHx++)
			hrConst[ iHx] = float( 0.013f * cos( kPi * (iHx+1)/12.));
		isSetup++;
	}

	float tSky;
	if (skyModelLW == C_SKYMODLWCH_DRYBULB)
		tSky = taDb;
	else
	{	float eSky;
		if (skyModelLW == C_SKYMODLWCH_BLAST)
			// BLAST "annual" model
			// as documented in ASHRAE Loads Toolkit
			// bracket() = insurance, impossible? for taDp to be out of range
			eSky = bracket( 0.f, 0.00344f*DegFtoK( taDp) - 0.137f, 1.f);
		else if (skyModelLW == C_SKYMODLWCH_BERDAHLMARTIN)
		{	// Berdahl-Martin as adapted by Palmiter and Niles.
			// See CSE algorithm documentation

			// clear sky emittance
			// note: constant 29.53f = (in Hg / bar)
			float taDpC = DegFtoC( taDp);
			float e0 = .711 + (0.56f/100.f)*taDpC + (.73f/10000.f)*taDpC*taDpC
						+ hrConst[ iHr] + .12f * (presAtm/29.53f - 1.f);

			eSky = e0 + 0.784f*(1.f - e0)*cldCvr/10.f;  // equation 76 from 2013 ACM
		}
		else
		{	err( ABT, "CalcSkyTemp: Invalid skyModelLW=%d", skyModelLW);
			return -999.f;	// insurance: err() does not return
		}

		tSky = DegRtoF( pow( eSky, .25f) * DegFtoR( taDb));
	}

	return tSky;
}		// ::CalcSkyTemp
//-----------------------------------------------------------------------------
RC WDHR::wd_EstimateMissingET1(		// estimate values from ET1 data
	int iHr)		// hour of day (0 - 23)
{
	RC rc = RCOK;

	float w = psyHumRatX( wd_db, wd_wb);
	wd_taDp = psyTDewPoint( w);

	wd_tSky = wd_CalcSkyTemp( C_SKYMODLWCH_BERDAHLMARTIN, iHr);

	return rc;

}		// WDHR::wd_EstimateMissingET1
//-----------------------------------------------------------------------------
float WDHR::wd_CalcSkyTemp(
	int skyModelLW,	// C_SKYMODLWCH_BERDAHLMARTIN, _BLAST, _DRYBULB,
	int iHr)		// hour of day (0 - 23)
{	return ::CalcSkyTemp( skyModelLW,
				iHr, wd_db, wd_taDp, wd_cldCvr, PsyPBar);
}		// WDHR::wd_CalcSkyTemp
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class WFILE: general interface to weather file
//              supports several formats
///////////////////////////////////////////////////////////////////////////////
WFILE::WFILE( basAnc *b, TI i, SI noZ /*=0*/)	// c'tor to help insure init is called, 10-94
	:  record( b, i, noZ),			// "*excon" is used in cnrecs.def.
	   yac( NULL), yacTDV( NULL), wf_pWDY( NULL)
{
	wf_Init();			// say no file open, etc. next.
}		// WFILE::WFILE
//----------------------------------------------------------------------------
WFILE::WFILE()			// default c'tor (insurance)
	: record( NULL, 0, 1), yac( NULL), yacTDV( NULL), wf_pWDY( NULL)
{	wf_Init();
}		// WFILE::WFILE
//----------------------------------------------------------------------------
WFILE::~WFILE()
{	delete yac;
	yac = NULL;
	delete yacTDV;
	yacTDV = NULL;
	delete wf_pWDY;
	wf_pWDY = NULL;
}		// WFILE::~WFILE
//----------------------------------------------------------------------------
/*virtual*/ void WFILE::Copy( const record* pSrc, int options/*=0*/)
{
	delete yac;			// free destination yac
	delete yacTDV;

	record::Copy( pSrc, options);	// verifies class (rt) same, copies whole derived class record. ancrec.cpp.

	yac = new YACAM();		// overwrite yac pointer, if any
	yacTDV = new YACAM();

}		// WFILE::Copy
//----------------------------------------------------------------------------
void WFILE::wf_Init()	// WFILE initialization function to call before each run

// note also CSE record may be 0'd by base c'tor and/or statSetup, but not necessarily
// done between runs after bad run in multi-run versions incl DLL version of CSE.
// DO NOT memset to 0 here: would destroy record base class header members.
{
	wFileFormat = UNK;
    VZero( loc, sizeof( loc));
    VZero( lid, sizeof( lid));
    yr = -1;
    jd1 = -1;
    jdl = -1;
    lat	= 0.f;
	lon = 0.f;
	tz = 0.f;
	elev = 0.f;
	taDbAvgYr = -999.f;
    solartime = FALSE;
	VZero( loc2, sizeof( loc2));
    isLeap = 0;
    firstDdm = 0;
    lastDdm	 = 0;
    winMOE	 = 0;
    win99TDb = 0;
    win97TDb = 0;
    sum1TDb	 = 0;
    sum1TWb	 = 0;
    sum2TDb	 = 0;
    sum2TWb  = 0;
    sum5TDb	 = 0;
    sum5TWb	 = 0;
    range	 = 0;
    sumMonHi = 0;
    hdrBytes = 0;
    hourBytes = 0;
	csvCols = 0;

	if (yac)
	{	yac->close();				// insurance
		yac->init();				// say no file open
	}
	if (yacTDV)
	{	yacTDV->close();
		yacTDV->init();
	}
	// else leave NULL (created in wf_Open() below)

}			// WFILE::wf_Init()
//----------------------------------------------------------------------------
RC WFILE::wf_Open(	// Open existing weather file and initialize WFILE structure

	const char* wfName,		// file path
	const char* TDVfName,	// optional TDV file path, NULL = no TDV file
	int erOp /*=ERR*/,		// error handling: ERR, WRN, IGN etc
							//   default ERR: report error, return rc != RCOK
	int wrAccess/*=FALSE*/,	// write access desired. Used only in utilities (not in CSE).
	char* hdr /*=NULL*/,	// non-null to return raw header of packed files. For wthr utility programs.
							// WFHDRSZ bytes returned even if file's hdr smaller.
	/* the following three arguments allow for return of monthly values (in header) not needed in CSE.
	   Each may be NULL or a pointer to float[12].
	   Values are returned only for file types containing them (ie ET1). */
	float* clrnss /*=NULL*/,	// receives 12 monthly clearness multipliers, eg 1.05.
	float* turbid /*=NULL*/,	// receives 12 monthly average atmospheric turbidity values
	float* atmois /*=NULL*/ )	// receives 12 monthly average atmospheric moisure values

// returns RCOK iff success
//		else other value, message issued per erOp

// sets isLeap if leap year file (future ET1 files 10-94).
{

#ifdef WANTED // not needed here in CSE 3-95... caller cncult2.cpp now does path search.
x // search for file in directories on path and others given by caller
x     if (xfFindOnPath( &wfName, erOp) != RCOK)		// check name, find file, substitute strsave'd full path / if ok
x        // BUG: double message: xfFindOnPath already issued msg I0022 or other message, 3-95.
x        return err( erOp, (char *)MH_I0022, wfName);	// "I0022: file '%s' does not exist"
x 							/* error may also have been bad drive, dir not found, etc;
x 							   xfFindOnPath issued msg per erOp.*/
#endif

// create YACAM if haven't yet done so
// creation deferred until this actual use due to multiple c'tor calls
	if (!yac)
		yac = new YACAM();		// calls YACAM::init()

// Try to open file in various formats, 1st success determines type
	RC rc = RCBAD;			// init to "cannot open file" to issue message on fallthru if NULL ptr passed

	rc = wf_T24DLLOpen( wfName, erOp);	// try as T24DLL
	if (rc == RCBAD2)
		rc = wf_PackedOpen( wfName, erOp, wrAccess, 	// attempt open as BSGS or ET or other future packed types
						hdr, clrnss, turbid, atmois );
	if (rc == RCBAD2)						// if wrong file type (but file could be opened)
		rc = wf_CSWOpen( wfName, erOp);		// retry as CSW
	if (rc == RCBAD2)
		rc = wf_EPWOpen( wfName, erOp);		// retry as EPW

	if (rc)				// if error
	{	err( erOp,			// display msg per erOp, rmkerr.cpp
			 rc==RCBAD2
			   ? (char *)MH_R0101	//  "R0101: %s file '%s': bad format or header"
			   : (char *)MH_R0100,	//  "R0100: %s file '%s': open failed"
			 "Weather",  wfName);
	}

	if (TDVfName)
	{	if (!yacTDV)
			yacTDV = new YACAM();
		RC rcTDV = wf_TDVOpen( TDVfName, erOp);
		if (rcTDV)
		{	err( erOp,			// display msg per erOp, rmkerr.cpp
			 rcTDV==RCBAD2
			   ? (char *)MH_R0101	//  "R0101: %s file '%s': bad format or header"
			   : (char *)MH_R0100,	//  "R0100: %s file '%s': open failed"
			 "TDV",  TDVfName);
			rc |= rcTDV;
		}
	}

	if (rc)
		wf_Close();

#ifdef WANTED // restore if xfFindOnPath restored above
x     dmfree( DMPP( wfName));		// free the heap storage now -- name is in yac->pathName().
#endif
	return rc;			// return value RCOK if ok
}		// WFILE::wf_Open
//----------------------------------------------------------------------------
RC WFILE::wf_Close( 		// Close weather file if open
	char* hdr /*=NULL*/ )	// NULL or pointer to header to rewrite if file open with write access. For wthr utilities.
// duplicate calls OK (= nop)
// returns RCOK iff success
{
	RC rc = RCOK;
	if (yac)
	{	if ( hdr				// if header given
		 &&  yac->wrAccess() )	// and file open for write access (ie only in util programs)
								// (FALSE if file not open at all)
			rc = yac->write( hdr, hdrBytes, 0L, WRN); 	// rewrite header at start file from caller's buffer

		rc |= yac->close(WRN);					// close file. nop if not open. yacam.cpp.
	}
	if (yacTDV)
		rc |= yacTDV->close( WRN);
#if defined( WTHR_T24DLL)
	rc |= T24Wthr.xw_Shutdown();
#endif
	return rc;
}				// WFILE::wf_Close
//-----------------------------------------------------------------------------
const char* WFILE::wf_FilePath() const		// return current weather file if known
{	return strtmp( yac ? yac->mPathName : "?");
}		// WFILE::wf_FilePath
//============================================================================
//  Opening packed weather files
//----------------------------------------------------------------------------
/*--- header decoding table structure
      For each field to decode from header, gives field types, offset, size,
         and where to put in WFILE struct.
      NOTE that header offsets are also used in wfpak3:
         wfBuildHdr there must be changed there if header is changed. */
struct WFHTAB
{	UCH dt;		// data type
	UCH count;		// array size, usually 1. Currently always 1, could remove, 10-94.
	USI hdrOffset;	// offset to data item in header
	UCH hdrLen;		// length of data item in header
	USI wfileOffset;	// offset to data item in WFILE struct
	UCH wfileLen;	// length of item in WFILE struct -- added 10-94 to facilitate multiple header formats
};
#define WFO(m)   offsetof(WFILE,m)		// WFILE member offset macro for use for WFHTAB.wfileOffeeset
#define WFS(m)   (sizeof( ((WFILE *)0L)->m ))	// WFILE member size macro for use for WFHTAB.wfileLen
#define WFOS(m)  WFO(m), WFS(m)			// macro for both offset and size of WFILE member
//----------------------------------------------------------------------------
RC WFILE::wf_PackedOpen(	// open BSGS or BSGSdemo or ET or ... packed weather file and read its header

	const char* wfName,	// pathName to open
	int erOp, 			// error WRN, IGN, etc action for specific diagnostic messages issued here, 10-94
	int wrAccess,		// non-0 for write as well as read access (for wthr utility programs)
	char* hdr,			// NULL or receives header (WFHDRSZ bytes) (for wthr utility programs)
	float* clrnss,		// NULL or receives monthly info from header if file has it (ET1)
	float* turbid, float* atmois )	// ditto, ditto

// returns: RCOK:   successful, *pWf initialized from file header.
//          RCBAD:  could not open file (no message)
//          RCBAD2: file opened but not of a type known here (no message)
//                  or has bad header (possible specific message issued here).
//          No message issued for file not found or wrong type, so caller may silently try a different open function.
//          Caller issues "not found" or "bad header" error message on bad return if no other fcn works. */

// sets isLeap if leap year file (future ET1 files)
{
// open file
	if (!wfName || yac->open( wfName, "weather file", IGN, wrAccess) != RCOK)
		return RCBAD;							// if failed

// read header
	char hdrBuf[WFHDRSZ];	// buffer for header -- largest header size
	if (!hdr)			// if caller didn't request header return
		hdr = hdrBuf;		// use internal buffer
	unsigned short nRead = yac->read( hdr, sizeof(hdrBuf), -1L, IGN);	// read bytes, no seek, no messages. yacam.cpp.
	if (nRead==sizeof(hdrBuf))					// if ok (not too few bytes = premature eof, -1 = error)
	{
		// (if failed, error returned at end of function)

		// set some file-type-independent WFILE members
		solartime = FALSE;		// all BSGS files are in local time

		// decode header according to type indicator at start (formats doc'd in wfpak.h)

		if (!memcmp( hdr, "TMP", 3))		// File type is "TMP" for BSGS or BSGSdemo weather file
		{
			if (hdr[13]=='P')				// Packer ID must be "P", else not BSGS wthr file (nor any other type)
				if (wf_BsgsDecodeHdr( hdr, erOp)==RCOK)	// decode header / if ok
					return RCOK;
			// on error, fall thru to close file
		}
		else if (!memcmp( hdr, "ET1", 3))
		{
			// insure validity of header and program by checking 2nd formatCode near end of header
			if (memcmp( ((ET1Hdr *)hdr)->formatCode2, "ET1!", 4))
				return err( erOp, (char *)MH_R1101, wfName);	// "Corrupted header in ET1 weather file \"%s\"."

			if (wf_EtDecodeHdr( hdr, erOp, clrnss, turbid, atmois)==RCOK)	// decode hdr (below) / if ok
				return RCOK;
			// on error, fall thru to close file
		}
		else if (!memcmp( hdr, "ET2", 3))
		{
			err( erOp, "ET2 wf code incomplete");
			// on error, fall thru to close file
		}
	}  // if header read ok

// arrive here if header can't be read or decoded
	yac->close(erOp);	// close if open, nop if not open
	return RCBAD2;		// RCBAD2: bad header or not a file type known here. Caller issues message.
}			// WFILE::wf_PackedOpen
//----------------------------------------------------------------------------
RC WFILE::wf_BsgsDecodeHdr( char* hdr, int erOp)

// RCOK if ok; on error (msg issued per erOp) caller must close file
{
//--- header decoding table for BSGS and BSGSdemo (aka PC or TMP) weather files
static WFHTAB wfhTab_BSGS[] =
//               ---header---  ---WFILE----
//  dataType [#] offset & len  offset & len
{
{ DTCH,   1,    53, 30,    WFOS(loc),	},	// location
{ DTCH,   1,    83, 11,    WFOS(lid),   },	// location id
{ DTSI,   1,    94,  2,    WFOS(yr),    },	// year, 2 digits
{ DTFLOAT,1,    96,  6,    WFOS(lat),   },	// latitude
{ DTFLOAT,1,   102,  7,    WFOS(lon),   },	// longitude
{ DTFLOAT,1,   109,  6,    WFOS(tz),    },	// timezone
{ DTFLOAT,1,   115,  5,    WFOS(elev),  },	// elevation
{ DTSI,   1,   120,  3,    WFOS(jd1),   },	// 1st DOY
{ DTSI,   1,   123,  3,    WFOS(jdl),   },	// last DOY
{ DTNONE, 0,     0,  0,    0, 0         }
};

// set WFILE members
	wFileFormat = (UCH)hdr[14]==255
					? BSGSdemo 	// ff in byte 14 indicates demo format file
					: BSGS;	// else is regular pc format if here. Tell wfread how to read file.
	hdrBytes = WFHDRSZ_BSGS;  			// offset for wfRead file pos
	hourBytes = WFBYTEHR_BSGS;			// offset per hour for wfRead

// decode fields per table above
	RC rc = wf_DecodeHdrFields( hdr, wfhTab_BSGS, erOp);
	if (rc==RCOK)
		if (yr >= 0 && yr <= 99)	// translate 2-digit year that is not -1 to 4 digits to be like ET file
			yr += 1900;
	return rc;
}		// WIFLE::wf_BsgsDecodeHdr
//----------------------------------------------------------------------------
RC WFILE::wf_EtDecodeHdr( char* hdr, int erOp, float *clrnss, float *turbid, float *atmois)

// sets isLeap if leap year file (future)
// RCOK if all Ok
{
	//--- header decoding table for ET1 weather files
#define E1O(m)   offsetof(ET1Hdr,m)
#define E1S(m)   sizeof(((ET1Hdr *)0)->m)
#define E1OS(m)  E1O(m), E1S(m)
static WFHTAB far wfhTab_ET1[] =
//                -----header------   ---WFILE----
//  dataType [#]   offset & length    offset & len
{
{ DTCH,   1,  E1OS(location1),     WFOS(loc),		   },	// location 1
{ DTCH,   1,  E1OS(location2),     WFOS(loc2),         },	// location 2
{ DTCH,   1,  E1OS(locationID),    WFOS(lid),          },	// location id
{ DTSI,   1,  E1OS(wthrYear),      WFOS(yr),           },	// year, 4 digits
{ DTFLOAT,1,  E1OS(latitude),      WFOS(lat),          },	// latitude
{ DTFLOAT,1,  E1OS(longitude),     WFOS(lon),          },	// longitude
{ DTFLOAT,1,  E1OS(timeZone),      WFOS(tz),           },	// timezone
{ DTFLOAT,1,  E1OS(elevation),     WFOS(elev),         },	// elevation
{ DTSI,   1,  E1OS(firstDay),      WFOS(jd1),          },	// 1st DOY
{ DTSI,   1,  E1OS(lastDay),       WFOS(jdl),          },	// last DOY
//{ DTCH, 1,  E1OS(isLeap),        WFOS(isLeap),       },	// 'Y' if leap year (future use): translated to 0 or 1 by caller
{ DTSI,   1,  E1OS(firstDdm),      WFOS(firstDdm),     },	// 1st month 1-12 with design day in file
{ DTSI,   1,  E1OS(lastDdm),       WFOS(lastDdm),      },	// last month 1-12 with design day in file
{ DTSI,   1,  E1OS(winMOE),        WFOS(winMOE),       },	// winter median of extremes (deg F)
{ DTSI,   1,  E1OS(win99TDb),      WFOS(win99TDb),     },	// winter 99% design temp (deg F)
{ DTSI,   1,  E1OS(win97TDb),      WFOS(win97TDb),     },	// winter 97.5% design temp (deg F)
{ DTSI,   1,  E1OS(sum1TDb),       WFOS(sum1TDb),      },	// summer 1% design temp (deg F)
{ DTSI,   1,  E1OS(sum1TWb),       WFOS(sum1TWb),      },	// summer 1% design coincident WB (deg F)
{ DTSI,   1,  E1OS(sum2TDb),       WFOS(sum2TDb),      },	// summer 2.5% design temp (deg F)
{ DTSI,   1,  E1OS(sum2TWb),       WFOS(sum2TWb),      },	// summer 2.5% design coincident WB (deg F)
{ DTSI,   1,  E1OS(sum5TDb),       WFOS(sum5TDb),      },	// summer 5% design temp (deg F)
{ DTSI,   1,  E1OS(sum5TWb),       WFOS(sum5TWb),      },	// summer 5% design coincident WB (deg F)
{ DTSI,   1,  E1OS(range),         WFOS(range),        },	// mean daily range (deg F)
{ DTSI,   1,  E1OS(sumMonHi),      WFOS(sumMonHi),     },	// month of hottest design day, 1-12
{ DTNONE, 0,  0,  0,               0, 0                }
};

// set WFILE members
	wFileFormat = ET1;
	hdrBytes = WFHDRSZ_ET1;	// offset for wfRead file pos for first day / header rewrite size
	hourBytes = WFBYTEHR_ET1;	// offset per hour for wfRead

// decode fields per table above to WFILE members
	RC rc = wf_DecodeHdrFields( hdr, wfhTab_ET1, erOp);
	taDbAvgYr = -999.f;		// annual average temp not in ET1 header
							//   say 'missing' -- will be derived in wf_FillWDYEAR()

// decode leap year flag, 'Y' for leap year, to Boolean
	isLeap = (strchr( "YyTt1", ((ET1Hdr *)hdr)->isLeap[0]) != NULL);	// take Y, y, T, t or 1 as TRUE

// optionally decode and return additional values if return ptr args are non-NULL
	const char* wfName = yac->pathName();				// point to pathName
	rc |= decodeFld( DTFLOAT, 12, hdr, E1OS(clearness[0]), clrnss, sizeof(float), erOp, "header of weather file", wfName);
	rc |= decodeFld( DTFLOAT, 12, hdr, E1OS(turbidity[0]), turbid, sizeof(float), erOp, "header of weather file", wfName);
	rc |= decodeFld( DTFLOAT, 12, hdr, E1OS(atmois[0]),    atmois, sizeof(float), erOp, "header of weather file", wfName);

	return rc;
}			// WFILE::wf_EtDecodeHdr
//----------------------------------------------------------------------------
RC WFILE::wf_DecodeHdrFields( 	// decode header fields to WFILE members per table
	char *hdr,
	struct WFHTAB * wfht0,
	int erOp )

// returns RCOK if no errors. Messages issued per erOp on errors; Completes all values regardless of errors.
{
// decode header fields into WFILE according to WFHTAB

	RC rc = RCOK;
	for (WFHTAB * wfht = wfht0;  wfht->dt != DTNONE;  wfht++)   	// loop over table entries === fields to decode
		rc |= decodeFld( wfht->dt, 							// data type
			(SI)(USI)wfht->count,							// count. do not sign-extend if UCH in struct.
			hdr,  wfht->hdrOffset,  (USI)wfht->hdrLen,		// source record, offset, len. don't sign-extend.
			(char *)this + wfht->wfileOffset,  (USI)wfht->wfileLen,	// destination and len
			erOp, 											// error action and filename for errmsgs
			"header of weather file", yac->pathName() );  	// what and which for error messages
	return rc;
}		// WFILE::wf_DecodeHdrFields
//----------------------------------------------------------------------------
LOCAL RC FC NEAR decodeFld( 		// decode one by-column-number field to internal -- potential general-use function

	USI dt, 				// DTSI: short int, DTFLOAT: float, DTCH: char[]
	SI count,	 			// number of data (ptrs incremented by srcLen and destLen irrespective of sizeof(dt)).
	void *srcRec, USI srcOff, 	// source record pointer and offset (separate only cuz of how err msgs are done)
	USI srcLen, 			// source length
	void *dest, USI destLen, 	// destination pointer and length. NOP if dest is NULL.
	int erOp, 				// error message control: WRN, IGN, etc
	const char* what,		// error message insert, eg "header of weather file"
	const char* which )		// error message insert, eg file pathname

// DTCH copies smaller of srcLen, destLen -1 bytes and always null-fills and null-terminates dest.
// DTSI and DTFLOAT copy 1 short and 1 float respectively.
// for count > 1, srcLen and destLen are added to src and dest and operation is repeated.
//  for DTCH, this copies  char[count][srcLen]  to  char [count][destLen].

// returns RCOK if ok.
{
	if (!dest)				// NULL destination pointer means skip this one (optional info-return arg)
		return RCOK;
	RC rc=RCOK;				// RCOK is 0; other return codes or'd in.
	do
	{
		char* pBeg = (char *)srcRec + srcOff; 			// ptr to source data
		char* pEnd = pBeg + srcLen;     			// ptr to 1st char after source data
		while (pEnd > pBeg && (!pEnd[-1] || isspaceW(pEnd[-1]))) 	// deblank end
			pEnd--;
		char cSave = *pEnd; 				// save char after data
		*pEnd = '\0';					//   and replace it with terminull -- given fields are not terminated.
		while (isspaceW(*pBeg))	  			// deblank start
			pBeg++;

		if (dt==DTCH)					// if a character field
		{
			strncpy( (char *)dest, pBeg, destLen-1);	// copy to dest with \0 at end
			((char *)dest)[destLen-1] = '\0';		// ..
		}
		else	// DTSI or DTFLOAT -- numeric
		{
			// initial syntax check
			char *pNext = pBeg;				// pointer to input text. Note already deblanked above
			if (strchr( "+-", *pNext))   pNext++;		// if sign present, pass it
			if (!isdigitW(*pNext))				// if no digit, it is not a valid number
				rc |= err( erOp, 					// conditionally issue message. prefixes "Error:\n  ".
					(char *)MH_R1102,			// "Missing or invalid number at offset %d in %s %s"
					(int)srcOff, what, which );
			else								// has digit, finish decoding
			{
				if (dt==DTSI)				// 16-bit integer
				{
					int tI = atoi(pBeg);			// get value. C library function.
					*(SI *)dest = (SI)tI;			// convert if necess (if 32-bit int), store
					while (isdigitW(*pNext))  pNext++;   	// pass digits for final syntax check below
				}
				else if (dt==DTFLOAT)			// 32-bit floating point value
				{
					double tD = strtod( pBeg, &pNext);	// decode to double, return next char ptr. C library function.
					*(float *)dest = (float)tD;		// convert to float and store.
				}
				else
				{
					rc |= err( PWRN, 						// bad table. unconditional error.
					(char *)MH_R1103, what );				// "Bad decoding table for %s in wfpak.cpp"
					pNext = "";							// suppress error message just below
				}
			}
			if (*pNext)						// if number did not use all of input
				rc |= err( erOp,
				(char *)MH_R1104,			// "Unrecognized characters after number at offset %d in %s %s"
				"Unrecognized characters after number at offset %d in %s %s",
				(int)srcOff, what, which );
		}
		*pEnd = cSave;			// restore char after data
		srcOff += srcLen;
		*(char * *)&dest += destLen;
	}
	while (--count > 0);
	return rc;
}				// decodeFld
//-----------------------------------------------------------------------------
WDHR& WFILE::wf_GetDsDayWDHR(int iHr)	// access to design day hour data
{
	return wf_pWDY->wdy_hrDsDay[iHr];
}	// WFILE::wf_GetDsDayWDHR
//-----------------------------------------------------------------------------
RC WFILE::wf_GetDsDayHr(		// retrieve hour data from current design day
	WDHR* pwd,		// returned: WDHR data for application use
	int iHr,		// standard time hour (0 - 23)
	int erOp)		// options
					//  WF_DSTFIX: get previous-day values from jDay+1 if iHr==23
					//      WHY: During daylight savings, prior hour data is used, resulting
					//			 in prior day for iHrClock=0
					//      Ignored if WF_DSNDAY|WF_FORCEDREAD
					//  WF_SAVESLRGEOM: do NOT overwrite wd_slAzm, wd_slAlt, and wd_sunupf
					//      Ignored if WF_DSNDAY|WF_FORCEDREAD
{
	pwd->Copy(wf_GetDsDayWDHR( iHr), (erOp&WF_SAVESLRGEOM) != 0);

	// DST_FIX?
	
	return RCOK;
}		// WFILE::wf_GetHrDsDay
//-----------------------------------------------------------------------------
RC WFILE::wf_Read(	// read and unpack weather data for an hour
	WDHR* pwd,		// weather data structure to receive hourly data.
	int jDay, 		// julian date 1-365 (or 366 if leap year), or month 1-12 with WF_DSNDAY option
	int iHr,		// standard time hour for which data is required (0 - 23)
	int erOp /*=WRN*/ )	// error reporting control: WRN, IGN, etc (notcne.h/cnglob.h); and
	    				//  option bit(s)
						//  WF_DSNDAY: read hour from design day for month 1-12 in "jDay" argument.
						//  WF_FORCEREAD: do not use cached data (read from file)
						//  WF_USELEAP: use Feb 29 data if in file, else skip over it.
						//		Added for poss future use. CSE does not do leap years, 10-94.
						//		Caller may test isLeap to detect leap year file.
						//  WF_DSTFIX: adjust *pwd daily values for daylight savings
						//      NOTE: alters WDHR *daily* values only, not hourly
						//            does not alter hour read
						//      Ignored if WF_DSNDAY|WF_FORCEDREAD
						//  WF_SAVESLRGEOM: do NOT overwrite wd_slAzm, wd_slAlt, and wd_sunupf
						//      Ignored if WF_DSNDAY|WF_FORCEDREAD
						//  WF_DBGTRACE: printf trace msg for debugging
// returns RCOK if OK else other RC, error msg issued per erOp.
{
	RC rc = RCOK;

	// handle Feb 29
	if ( isLeap 				// if wf has Feb 29 (for possible future use in ET1 files 10-94)
 	 &&  !(erOp & WF_USELEAP) )	// if caller is NOT simulating Feb 29
		if (jDay > 31+28)	// if date is AFTER Feb 29
			jDay++;			// increment date to use in accessing file: day after Feb 28 is Mar 1 not Feb 29.

	// use cached data if available unless caller says otherwise
	//   NOTE: cached data assumes full annual weather file
	//         TODO: support partial weather years
	if (wf_pWDY && !(erOp & (WF_FORCEREAD|WF_DSNDAY)))
		rc |= wf_pWDY->wdy_TransferHr( jDay, iHr, pwd, erOp);
	else
	{	// handle alternative formats
		// pwd->wd_Init( 1);		// set all to UNSET -- done in each xxxRead
		if (wf_IsPacked())
		{	// format is packed: BSGS, BSGSdemo, ET1, or ... .
			USI* phour = (USI*)wf_PackedHrRead(jDay, iHr, erOp);
			rc |= phour ? pwd->wd_Unpack(iHr, phour, wFileFormat) : RCBAD;
		}
		else
			rc |=
			  wFileFormat == CSW ? wf_CSWRead(pwd, jDay, iHr, erOp)
			: wFileFormat == EPW ? wf_EPWRead(pwd, jDay, iHr, erOp)
			: wFileFormat == T24DLL ? wf_T24DLLRead(pwd, jDay, iHr, erOp)
			: RCBAD;

		// read additional TDV data if requested and available
		rc |= wf_TDVReadIf(pwd, jDay, iHr, erOp);
	}
#if 0 && defined( _DEBUG)
	if (erOp & WF_DBGTRACE)
		printf("\nWFR %2d %2d %6d db=%0.2f",
			jDay, iHr, erOp, pwd->wd_db);
#endif

#if defined( DOSOLARONREAD)
	pwd->wd_SetSolarValues(jDay, iHr);
#endif

	return rc;		// other returns above
}		// WFILE::wf_Read
//---------------------------------------------------------------------------
USI* WFILE::wf_PackedHrRead( 	// access an hour's packed data
	int jDay, 		// Julian date 1-365 (or 366 if leap year), or month 1-12 with WF_DSNDAY option
	int iHr, 		// hour 0-23
	int erOp /*=WRN*/ )	// msg control: WRN, IGN, etc + options
						//  WF_DSNDAY: read hour from design day for month 1-12 in "jDay" argument.
						//  WF_DSTFIX: no effect

// returns pointer to hour's data, or NULL if error
{
// determine file offset of desired hour's data
	LI hrOffset = wf_PackedHrOffset( jDay, iHr, erOp);
	if (!hrOffset)
		return NULL;

// read bytes, return pointer into file buffer
	return (USI *)yac->getBytes( 	// get data into internal buf if not there, return pointer, NULL if error
			   hrOffset,			// hour's file offset
			   hourBytes, 			// need 1 hour's bytes
			   erOp );
}	// WFILE::wf_PackedHrRead
//---------------------------------------------------------------------------
LI WFILE::wf_PackedHrOffset( 	// determine file offset of an hour's packed data
	int jDay, 		// Julian date 1-365 (or 366 if leap year), or month 1-12 with WF_DSNDAY option
	int iHr, 		// hour 0-23
	int erOp /*=WRN*/ )	// msg control: WRN, IGN, etc (notcne.h/cnglob.h)
// option bit(s) (wfpak.h): WF_DSNDAY: read hour from design day for month 1-12 in "jDay" argument.

// returns offset of hour's data, or 0L if error
{
	LI hrOffset;
	if (erOp & WF_DSNDAY)	// if design day rather than regular simulation data
	{
		if (wFileFormat==BSGS || wFileFormat==BSGSdemo)
		{
			err( erOp, (char *)MH_R0107);  		// "Design Days not supported in BSGS (aka TMP or PC) weather files"
			return 0L;
		}
		// assume ET1 or later format
		if (jDay < 1 || jDay > 12)
		{
			err( erOp, (char *)MH_R0108, (int) jDay);	/* "Program error in use of function wfPackedhrRead\n\n"
                     					   "    Invalid Month (%d) in \"jDay\" argument: not in range 1 to 12." */
			return 0L;
		}
		if (firstDdm > lastDdm || lastDdm < 1)
		{
			err( erOp, (char *)MH_R0109, yac->pathName() );			// "Weather file \"%s\" contains no design days"
			return 0L;
		}
		if (jDay < firstDdm || jDay > lastDdm)
		{
			err( erOp, (char *)MH_R0110, jDay,	// "Weather file contains no design day for month %d\n\n"
				 yac->pathName(), 	// "    (Weather file \"%s\" contains design day data only for months %d to %d)"
				 (int)firstDdm,
				 (int)lastDdm );
			return 0L;
		}
		// design days are AFTER the regular days
		hrOffset = hdrBytes 					// pass header
				   + hourBytes*( 24L *( jdl - jd1 + 1	// pass all regular hourly data
											 + jDay - firstDdm )	// pass preceding months' design days
									  + iHr ); 				// pass preceding hours this day (iHr 0-based)
	}
	else									// not design day request -- normally
		hrOffset = hdrBytes + ((jDay - jd1)*24L + iHr)*hourBytes;	// offset of regular hourly data
	return hrOffset;
}			// wfPackedOffset
//=============================================================================
RC WFILE::wf_CSWHdrVal( const char* key, const char* val, float& v, RC& rcAll, int erOp/*=WRN*/)
{	RC rc = RCOK;
	const char* msg;
	if ( strDecode( val, v, IGN, key, &msg) != 1)
		rc = err( erOp, "Bad header info in weather file '%s': %s", wf_FilePath(), msg);
	if (rc)
		rcAll |= rc;
	return rc;
}		// WFILE::wf_CSWHdrVal
//------------------------------------------------------------------------------
RC WFILE::wf_CSWHdrVal( const char* key, const char* val, int& v, RC& rcAll, int erOp/*=WRN*/)
{	RC rc = RCOK;
	const char* msg;
	if ( strDecode( val, v, IGN, key, &msg) != 1)
		rc = err( erOp, "Bad header info in weather file '%s': %s", wf_FilePath(), msg);
	if (rc)
		rcAll |= rc;
	return rc;
}		// WFILE::wf_CSWHdrVal
//------------------------------------------------------------------------------
RC WFILE::wf_CSWOpen(	// open California CSW weather file
	const char* wfName,	// pathName of file to open
	int erOp)
// no wrAccess argument: can't rewrite records in exiting file; create files with text editor or spreadsheet.

// returns: RCOK:   success, *this initialized from file header.
//	        RCBAD:  could not open file (no message)
//          RCBAD2: file opened but not of a type known here (no message)
//                    or has bad header (possible specific message issued here).
//          No message issued for file not found or wrong type, so caller may silently try a different open function.
//          Caller issues "not found" or "bad header" error message on bad return if no other fcn works.
{
// set WFILE members, including unused ones as insurance
	wf_Init();

// open file
	if (yac->open( wfName, "weather file", IGN) != RCOK)	// attempt open, no message. yacam.cpp.
		return RCBAD;						// return if failed. caller issues message.

	RC rc = RCOK;

	// look for file version
	//   +EROP1 = match only beg of line (ignores trailing ,,,)
	if (yac->scanLine( "File format version", IGN+EROP1, 50) < 0)
		return RCBAD2;

	// look for beginning of header (50 lines max)
	//   +EROP1 = match only beg of line (ignores trailing ,,,)
	if (yac->scanLine( "Header elements", IGN+EROP1, 50) < 0)
		rc = err( erOp,	"file '%s' is not a valid CSW weather file", wfName);

	WStr city, state, country;	// temporaries
	int nHL = 0;		// # of header lines read
	if (!rc)
	{	char ln[ WFMAXLNLEN];		// big enuf
		RC rc1;
		int bSeenHourlyData = FALSE;		// set 1 when "Hourly Data" encountered
		for (nHL=0; ; nHL++)
		{	// read header line-by-line
			int len = yac->line( ln, sizeof( ln), IGN);
			if (len < 0)
			{	rc = err( erOp,	"bad 'Header Elements' info in file CSW file '%s'", wfName);
				break;
			}
			// header values are <key>,<value>
			//   some lines have gratitous trailing commas
			//   order not necessarily defined
			//   This code tolerates any order and optional comma
			//   Does NOT detect missing or duplicate items.
			const char* lnToks[ WFMAXCOLS];
			int nTok = strTokSplit( ln, ",", lnToks, WFMAXCOLS);
			const char* key = nTok > 0 ? lnToks[ 0] : "";
			const char* val = nTok > 1 ? lnToks[ 1] : "";
			if (IsBlank( key))
				continue;		// blank line: skip
			if (strMatch( key, "Hourly Data"))		// beg of next section
			{	bSeenHourlyData = TRUE;				// set to decode one more line
				continue;
			}
			if (bSeenHourlyData)
			{	if (strMatch( key, "Columns"))
					wf_CSWHdrVal( key, val, csvCols, rc);
				else
					rc = err( erOp,	"bad 'Columns' format in file CSW file '%s'", wfName);
				break;
			}

			// decode heading values by name
			if (strMatch( key, "City"))
				city = val;
			else if (strMatch( key, "State"))
				state = val;
			else if (strMatch( key, "Country"))
				country = val;
			else if (strMatch( key, "Time zone"))
			{	rc1 = wf_CSWHdrVal( key, val, tz, rc);
				tz = -tz;		// non-standard internal sign convention
			}
			else if (strMatch( key, "Latitude"))
				rc1 = wf_CSWHdrVal( key, val, lat, rc);
			else if (strMatch( key, "Longitude"))
			{	rc1 = wf_CSWHdrVal( key, val, lon, rc);
				lon = -lon;		// non-standard internal sign convention
			}
			else if (strMatch( key, "Elevation"))
				rc1 = wf_CSWHdrVal( key, val, elev, rc);
			else if (strMatch( key, "365-day Avg DB"))
				rc1 = wf_CSWHdrVal( key, val, taDbAvgYr, rc);
		}
	}
	if (!rc)
		rc = wf_CSWPosHr1( erOp);

	if (!rc)
	{	wFileFormat = CSW;		// accept that the format is CSW
		// transfer location info (could be refined/improved)
		strncpy0( loc, city.c_str(), sizeof( loc));
		strncpy0( loc2, state.c_str(), sizeof( loc2));
		strCatIf( loc2, sizeof( loc2), ", ", country.c_str());
	}
	else
	{	yac->close( erOp);		// close file if open, nop if not open
		return RCBAD2;			// RCBAD2: bad header or not a file type known here. Caller issues message.
	}
	return RCOK;
}			// WFILE::wf_CSWOpen
//----------------------------------------------------------------------------
RC WFILE::wf_CSWPosHr1( int erOp)			// position file at beg of 1st hour
{
	RC rc = RCOK;
	yac->rewind();		// pos file to beginning

	// scan over header (EROP1 = compare line begs only, ignore trailing commas)
	if (yac->scanLine( "Hourly Data", IGN+EROP1, 50) < 0)
		rc = RCBAD;
	char ln[ WFMAXLNLEN];
	for (int iL=0; !rc && iL<5; iL++)
	{	int len = yac->line( ln, sizeof( ln), IGN);
		if (len < 0)
			rc = RCBAD;
	}
	if (rc)
		rc = err( erOp,	"format trouble in CSW weather file '%s'", wf_FilePath());
	return rc;
}		// WFILE::wf_CSWPosHr1
//----------------------------------------------------------------------------
RC WFILE::wf_CSWRead(	// read and unpack data from CSW
	WDHR* pwd,		// weather data structure to receive hourly data.
	int jDay,		// julian day for which data is expected (1 - 365)
	int iHr,		// hour for which data is expected (0 - 23)
	int erOp)		// msg control: WRN, IGN, etc.
					// option bit WF_DSNDAY: causes error message here.

// returns RCOK if OK, else other RC; msg issued per erOp
{

// check options
	if (erOp & WF_DSNDAY)
		return err( erOp, "Design Days not supported in CSW weather files");

	int jHrSought = (jDay-1)*24 + iHr + 1;		// 1-based hour of year
	RC rc = pwd->wd_CSWReadHr( this, jHrSought, erOp);

	return rc;
}			// WFILE::wf_CSWRead
//------------------------------------------------------------------------------
RC WFILE::wf_TDVOpen(	// open California Time of Day Valuation (TDV) file
	const char* TDVfName,	// pathName of file to open
	int erOp)

// returns: RCOK:   successful, *this members set
//	        RCBAD:  could not open file (no message)
//          RCBAD2: but format bad (no message)
{
	// wf_Init() assumed called

	RC rc = RCOK;

// open file
	if (yacTDV->open( TDVfName, "TDV file", IGN) != RCOK)	// attempt open, no message
		rc = RCBAD;										// return if failed. caller issues message.
	else
	{	rc = wf_TDVReadHdr( erOp);
		if (rc)
		{	yacTDV->close( erOp);	// close file if open, nop if not open
			delete yacTDV;
			yacTDV=NULL;
			rc = RCBAD2;			// RCBAD2: bad file format
		}
	}
	return rc;
}			// WFILE::wf_TDVOpen
//----------------------------------------------------------------------------
#define _C( s) s,sizeof( s)		// helper re getLineCSV calls
//----------------------------------------------------------------------------
void WFILE::wf_TDVInitHdrInfo()
{	memset(  wf_TDVFileTimeStamp, 0, sizeof( wf_TDVFileTimeStamp));
	memset(  wf_TDVFileTitle, 0, sizeof( wf_TDVFileTitle));
}	// WFILE::wf_TDVInitHdrInfo
//----------------------------------------------------------------------------
RC WFILE::wf_TDVReadHdr( int erOp)		// read / decode TDV file header
{
	RC rc = RCOK;
	yacTDV->rewind();
	wf_TDVFileJHr = 0;
	wf_TDVInitHdrInfo();

// example TDV header
// same structure as import file
//	"TDV Data (TDV/Btu)","001"
//	"Wed 14-Dec-16  12:30:29 pm"
//	"BEMCmpMgr 2019.0.0 RV (758), CZ12, Fuel NatGas","Hour"
//	"tdvElec","tdvFuel"

	char T1[ 200], T2[ 200];

	// initial tag
	rc = yacTDV->getLineCSV( erOp, 0, "CC", _C( T1), _C( T2), NULL);
	rc |= yacTDV->checkExpected( T1, "TDV Data (TDV/Btu)");
	if (!rc)
		// time stamp
		rc = yacTDV->getLineCSV( erOp, 0, "C", _C( wf_TDVFileTimeStamp), NULL);
	if (!rc)
	{	// title
		rc = yacTDV->getLineCSV( erOp, 0, "CC", _C( wf_TDVFileTitle), _C( T2), NULL);
		rc |= yacTDV->checkExpected( T2, "Hour");
	}
	if (!rc)
		// column heads (ignored)
		rc = yacTDV->getLineCSV( erOp, 0, "CC", _C(T1), _C( T2), NULL);

	if (rc)
		wf_TDVInitHdrInfo();	// error: clear any partial data
	
	return rc;

}		// WFILE::wf_TDVReadHdr
//----------------------------------------------------------------------------
RC WFILE::wf_TDVPosHr1( int erOp)			// position file at beg of 1st hour
{
	RC rc = RCOK;
	yacTDV->rewind();		// pos file to beginning
	wf_TDVFileJHr = 0;

	// read/discard head (4 lines)
	char ln[ WFMAXLNLEN];
	for (int iL=0; !rc && iL<4; iL++)
	{	int len = yacTDV->line( ln, sizeof( ln), IGN);
		if (len < 0)
			rc = RCBAD;
	}
	if (rc)
		rc = err( erOp,	"format trouble in TDV file '%s'", wf_FilePath());
	return rc;
}		// WFILE::wf_TDVPosHr1
//-----------------------------------------------------------------------------
RC WFILE::wf_TDVReadIf(	// read TDV data
	WDHR* pwd,		// weather data structure to receive hourly data.
	int jDay,		// julian day for which data is expected (1 - 365)
	int iHr,		// Hour for which data is expected (0 - 23)
	int erOp)		// msg control: WRN, IGN, etc.
					// option bit WF_DSNDAY: ignored here

// returns RCOK if OK or NOP
//		else other RC; msg issued per erOp
{
	RC rc = RCOK;
	if (yacTDV && !(erOp & WF_DSNDAY))
	{	int jHrSought = (jDay-1)*24 + iHr + 1;		// 1-based hour of year
		rc |= pwd->wd_TDVReadHr( this, jHrSought, erOp);
	}
	// else NOP (caller initializes wd_TDVxxx to UNSET)

	return rc;
}			// WFILE::wf_TDVReadIf
//-----------------------------------------------------------------------------
RC WFILE::wf_T24DLLOpen(		// set up access to T24WTHR.DLL hourly data
	const char* wfName,	// file "name" -- if not in form "$CZxx", return RCBAD2
	[[maybe_unused]] int erOp)			// msg control

// returns: RCOK:   successful, *pWf initialized from file header.
//          RCBAD:  could not open file (no message)
//          RCBAD2: type not known here (no message)
//                    or has bad header (possible specific message issued here).
//          No message issued for file not found or wrong type, so caller may silently try a different open function.
//          Caller issues "not found" or "bad header" error message on bad return if no other fcn works. */

{
	RC rc = RCBAD;
	if (!wfName)
		rc = RCBAD;		// no name, always bad
#if defined( WTHR_T24DLL)
	CString czCode( wfName);
	czCode.TrimLeft().TrimRight();
	if (czCode.GetLength() < 4 || czCode.Left( 3).CompareNoCase( "$CZ") )
		rc = RCBAD2;		// cannot be valid $CZxx, say wrong file type
	else
	{	int CZ;
		if (strDecode( czCode.Mid( 3), CZ) != 1)
			rc = err( erOp,	"Invalid climate zone code '%s'", czCode);
		else
			rc = T24Wthr.xw_Setup();
		if (rc == RCOK)
		{	// note: T24WTHR.DLL checks 1 <= CZ <= 16
			rc = T24Wthr.xw_InitWthr( CZ, IGN);
			if (rc != RCOK)
				err( erOp, "Failed to find weather data for '%s':\n%s",
					czCode, T24Wthr.xw_GetErrMsg());
		}
		if (rc == RCOK)
			// header info?
			wFileFormat = T24DLL;		// success
	}
#else
	else
		rc = RCBAD2;		// type unknown here
#endif
	return rc;
}	// WFILE::wf_T24DLLOpen
//--------------------------------------------------------------------------
RC WFILE::wf_T24DLLRead(	// read and unpack data from CSW
	[[maybe_unused]] WDHR* pwd,	// weather data structure to receive hourly data.
	[[maybe_unused]] int jDay,		// Julian day for which data is expected (1 - 365)
	[[maybe_unused]] int iHr,			// Hour for which data is expected (0 - 23)
	[[maybe_unused]] int erOp)		// msg control: WRN, IGN, etc.
					// option bit WF_DSNDAY: causes error message here.
{
#if defined( WTHR_T24DLL)
	IDATE iDate;
	SI yr = -1;	// assume T24WTHR.DLL years are always 365 days
	tddyi( jDay, yr, &iDate);

	return T24Wthr.xw_GetHr( erOp, iDate.month, iDate.mday, iHr+1, pwd);
#else
	return RCBAD;
#endif
}	// WFILE::wf_T24DLLRead
//------------------------------------------------------------------------------
RC WFILE::wf_EPWOpen(	// open EnergyPlus weather file
	const char* wfName,	// pathName of file to open
	int erOp)
// no wrAccess argument: can't rewrite records in exiting file; create files with text editor or spreadsheet.

// returns: RCOK:   successful, *pWf initialized from file header.
//          RCBAD:  could not open file (no message)
//          RCBAD2: file opened but not of a type known here (no message)
//                  or has bad header (possible specific message issued here).
//          No message issued for file not found or wrong type, so caller may silently try a different open function.
//          Caller issues "not found" or "bad header" error message on bad return if no other fcn works. */
{
// set WFILE members, including unused ones as insurance
	wf_Init();

// open file
	if (yac->open( wfName, "weather file", IGN) != RCOK)	// attempt open, no message. yacam.cpp.
		return RCBAD;

	RC rc = RCOK;

	// location (1st line)
	char lineTag[ 200], xCity[ 100], xState[ 100], xCountry[ 100], xSource[ 100], xWMO[ 100];
	rc = yac->getLineCSV( erOp, isLeap, "CCCCCCFFFF",
				_C( lineTag), _C( xCity), _C( xState), _C( xCountry),
				_C( xSource), _C( xWMO),
				&lat, &lon, &tz, &elev, NULL);
	elev = LSItoIP( elev);		// m -> ft

#if 0
	if (!rc)
		lid = srcWBAN;
#endif


	if (!rc)
		rc = wf_EPWPosHr1( erOp);

	if (!rc)
	{	wFileFormat = EPW;		// accept that the format is CSW
		// transfer location info (could be refined/improved)
		strncpy0( loc, xCity, sizeof( loc));
		strncpy0( loc2, xState, sizeof( loc2));
		strCatIf( loc2, sizeof( loc2), ", ", xCountry);
	}
	else
	{	yac->close( erOp);		// close file if open, nop if not open
		return RCBAD2;			// RCBAD2: bad header or not a file type known here. Caller issues message.
	}
	return rc;


}		// WFILE::wf_EPWOpen
//----------------------------------------------------------------------------
RC WFILE::wf_EPWPosHr1( int erOp)			// position EPW file at beg of 1st hour
{
	RC rc = RCOK;

	yac->rewind();		// pos file to beginning

	int nDataPeriods = 0;
	int nRecordsPerHour = 0;
	char xTagFile[ 100], xDP1Desc[ 500], xDP1DOW[ 100], xDP1BegDate[ 100], xDP1EndDate[ 100];
	const char* xTagSeek = "DATA PERIODS";
	char ln[ YACAM_MAXLINELEN];
	for (int iL=0; !rc && iL<20; iL++)
	{	int len = yac->line( ln, sizeof( ln), IGN);
		if (len < 0)
			rc = RCBAD;
		if (_strnicmp( xTagSeek, ln, strlen( xTagSeek)) == 0)
		{	rc = yac->getLineCSV( erOp | YAC_NOREAD, isLeap, "CLLCCCC",
					ln,		// re YAC_NOREAD: line already here
					_C( xTagFile), &nDataPeriods, &nRecordsPerHour,
					_C( xDP1Desc), _C( xDP1DOW), _C( xDP1BegDate), _C( xDP1EndDate),
					NULL);
			break;
		}
	}

	if (rc)
		rc = err( erOp,	"format trouble in EPW weather file '%s'", wf_FilePath());
	else if ( nDataPeriods != 1 || nRecordsPerHour != 1)
		rc = err( erOp, "Bad EPW '%s'", wf_FilePath());
	return rc;
}		// WFILE::wf_EPWPosHr1
//----------------------------------------------------------------------------
RC WFILE::wf_EPWRead(	// read and unpack data from EPW
	WDHR* pwd,		// weather data structure to receive hourly data.
	int jDay,		// Julian day for which data is expected (1 - 365)
	int iHr,		// Hour for which data is expected (0 - 23)
	int erOp)		// msg control: WRN, IGN, etc.
					// option bit WF_DSNDAY: causes error message here.

	// returns RCOK if OK, else other RC; msg issued per erOp
{

// check options
	if (erOp & WF_DSNDAY)
		return err( erOp, "Design Days not supported in EPW weather files");

	int jHrSought = (jDay-1)*24 + iHr + 1;		// 1-based hour of year
	RC rc = pwd->wd_EPWReadHr( this, jHrSought, erOp);

	return rc;
}			// WFILE::wf_EPWRead

#if 0

// also opens input file at appropriate place
// sets members: destIsLeap, doyBeg, doyEnd, 1st & last des day months, elev, wf, .
/* returns 0:   ok;
           -4: failed but do rest of commands in cmd file;
           other:  failed & terminate run (cmd file error or cmd file for cmd not completely read) */
{
	CWString sBuf;			//	general purpose temporary buffer

	sWf = SWF_EPW;						// say source format is WYEC2 (member)

	BOOL bRet = EPWfile.Open(srcPNam);
	if (!bRet)
		return -2;

	int rc;

	fOrg = "EPW";
	InitWF();
	rc = EPWfile.GetValCR(5, 0, srcWBAN, 17);	//	WMO
	if (!rc)
		lid = srcWBAN;
	//				col row
	if (!rc)
		rc = EPWfile.GetValCR(1, 0, loc, 20);
	if (!rc)
		rc = EPWfile.GetValCR(3, 0, loc2, 20);
	if (!rc & strMatch(loc2, "USA"))	//	state only if not USA
		rc = EPWfile.GetValCR(2, 0, loc2, 20);
	if (!rc)
		rc = EPWfile.GetValCR(8, 0, &tz);
	if (!rc)
		rc = EPWfile.GetValCR(6, 0, &lat);
	if (!rc)
		rc =EPWfile.GetValCR(7, 0, &lon);
	if (!rc)
		rc = EPWfile.GetValCR(9, 0, &elev);
	if (!rc)
	{	elev = (float)Units.UnitConvert(elev, unLenM, unLenFt);
		elev = elev;
		psyElevation(elev);			// set atmospheric pressure for psychro fcns. psychro1.cpp
	}

	//	assemble target filename if not in cmd file (ends with backslash)
	if (!destPNam.Right(1).Compare("\\"))
	{
//		CWString tmpPNam = loc2;
		CWString tmpPNam = srcPNam;
		int	start = srcPNam.ReverseFind('\\') + 1;
		int len = srcPNam.ReverseFind('.') - start;
		CWString rootName = tmpPNam.Mid(start, len);
//		rootName.Replace(" ", "_");
//		rootName.Replace(".", "_");
		tmpPNam = rootName + ".et1";
		if (destPNam.GetLength() + tmpPNam.GetLength() < 82)
			destPNam += tmpPNam;
		else
		{	printf("Destination path longer than 81 ([%s] and [%s])\n", destPNam, tmpPNam);
			return -2;
		}
	}

	if (!rc)
		rc = EPWfile.GetValCR(1, 4, sBuf, 3);
	if (!rc)
		isLeap = destIsLeap = (sBuf.CompareNoCase("YES") == 0);
	sourceDate.Empty();
	yr = -1;	//	fabricated year
	if (!rc)
		rc = EPWfile.GetValCR(5, 7, sBuf, 5);	// month (1-12) of first monthly design day in file
	if (!rc)
	{	firstDdm = atoi(sBuf.Piece(0,"/"));
		jd1 = doyBeg = Calendar.GetDoy(atoi(sBuf.Piece(0,"/")), atoi(sBuf.Piece(1,"/")));
	}
	if (!rc)
		rc = EPWfile.GetValCR(6, 7, sBuf, 5);	// month (1-12) of last monthly design day in file, 0 if none
	if (!rc)
	{	lastDdm = atoi(sBuf.Piece(0,"/"));
		jdl = doyEnd = Calendar.GetDoy(atoi(sBuf.Piece(0,"/")), atoi(sBuf.Piece(1,"/")));
	}
	winMOE	= -99;	// winter median of extremes (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+ 6, 1, &win99TDb	);	// winter 99% design temp (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+ 7, 1, &win97TDb	);	// winter 97.5% design temp (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+10, 1, &sum1TDb	);	// summer 1% design temp (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+11, 1, &sum1TWb	);	// summer 1% design coincident WB (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+12, 1, &sum2TDb	);	// summer 2.5% design temp (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+13, 1, &sum2TWb	);	// summer 2.5% design coincident WB (deg F)
	sum5TDb	= -99;							// summer 5% design temp (deg F)
	sum5TWb	= -99;							// summer 5% design coincident WB (deg F)
	if (!rc)
		rc = EPWfile.iGetFTempCR(6+23, 1, &range	);	// mean daily temperature range (deg F)
	sumMonHi = 0;								// month of hottest design day, 1-12
	for (int i = 0;  i < 12;  i++)			// format three optional vectors of 12 mothly floats
	{
		clearness[i] = 0.f;	// clearness numbers, for design day solar
		turbidity[i] = 0.f;	// avg atmospheric turbidity, for daylight calcs
		atmois[i] = 0;		// avg atmospheric moisture, for daylight calcs
	}
	if (!rc)
		rc = EPWfile.GetValCR(1, 5, sBuf, 128);
	if (!rc)
		strcpy(EPWcomment, sBuf);
	if (!rc)
		rc = EPWfile.GetValCR(1, 6, sBuf, 128);			// comment (128 chars)
	if (!rc)
	{	if (strlen(EPWcomment) + sBuf.GetLength() < 127)
			strcat(EPWcomment, sBuf);
		else
			strcat(EPWcomment, sBuf.Right(127-strlen(EPWcomment)));
	}

    return rc;



	// look for beginning of header (50 lines max)
	//   +EROP1 = match only beg of line (ignores trailing ,,,)
	if (yac->scanLine( "Header elements", IGN+EROP1, 50) < 0)
		rc = err( erOp,	"file '%s' is not a valid CSW weather file", wfName);

	CWString city, state, country;	// temporaries
	int nHL = 0;		// # of header lines read
	if (!rc)
	{	CWString ln, key, val;
		RC rc1;
		int bSeenHourlyData = FALSE;		// set 1 when "Hourly Data" encountered
		for (nHL=0; ; nHL++)
		{	// read header line-by-line
			int len = yac->line( ln, IGN);
			if (len < 0)
			{	rc = err( erOp,	"bad 'Header Elements' info in file CSW file '%s'", wfName);
				break;
			}
			// header values are <key>,<value>
			//   some lines have gratitous trailing commas
			//   order not necessarily defined
			//   This code tolerates any order and optional comma
			//   Does NOT detect missing or duplicate items.
			key = ln.Piece( 0, ",");
			val = ln.Piece( 1, ",");
			if (IsBlank( key))
				continue;		// blank line: skip
			if (strMatch( key, "Hourly Data"))		// beg of next section
			{	bSeenHourlyData = TRUE;				// set to decode one more line
				continue;
			}
			if (bSeenHourlyData)
			{	if (strMatch( key, "Columns"))
					wf_CSWHdrVal( key, val, csvCols, rc);
				else
					rc = err( erOp,	"bad 'Columns' format in file CSW file '%s'", wfName);
				break;
			}

			// decode heading values by name
			if (strMatch( key, "City"))
				city = val;
			else if (strMatch( key, "State"))
				state = val;
			else if (strMatch( key, "Country"))
				country = val;
			else if (strMatch( key, "Time zone"))
			{	rc1 = wf_CSWHdrVal( key, val, tz, rc);
				tz = -tz;		// non-standard internal sign convention
			}
			else if (strMatch( key, "Latitude"))
				rc1 = wf_CSWHdrVal( key, val, lat, rc);
			else if (strMatch( key, "Longitude"))
			{	rc1 = wf_CSWHdrVal( key, val, lon, rc);
				lon = -lon;		// non-standard internal sign convention
			}
			else if (strMatch( key, "Elevation"))
				rc1 = wf_CSWHdrVal( key, val, elev, rc);
			else if (strMatch( key, "365-day Avg DB"))
				rc1 = wf_CSWHdrVal( key, val, taDbAvgYr, rc);
		}
	}

//----------------------------------------------------------------------------
int WFPACKER::createEt1FromEPW()		// read EPW file, create ET1wthr file, write header


}				// WFPACKER::createEt1FromEPW


//---------------------------------------------------------------------------
int WFPACKER::readHrEPW( 	// read hour's data from EPW source file to .wd & calc derived fields

    int doy, int iHr ) 			// day of year, 1-365 or 366; hour 0-23.

// uses member .iBuf to buffer a day's data.
// sets .wd members, .month, .

/* returns 0:   ok;
           -4  missing data (certain fields only) -- msg issued, continue packing;
           -2   error, message issued, terminate packing. */
{
    int inMDay = 0, inH = 0, inDoy = 0;
	const int hdrLen = 8;
	int curRow = hdrLen+DoyHrToIdx(doy, iHr);

	// read day's data, mostly to mbr WFDATA wd. ASCII input file not different for BSGS as opposed to ET1.
	int rc;
	rc = EPWfile.GetValCR(1, curRow, &month);
	if (!rc)
		rc = EPWfile.GetValCR(2, curRow, &inMDay);
	//	update srcIsLeap if Feb 29
	if (!rc && month == 2 && inMDay == 29)
		srcIsLeap = TRUE;
	if (!rc)
		inDoy = dMonDay2Jday( month, inMDay, srcIsLeap);	// convert input month 1-12 and day 1-31 to Julian day of year 1-365

	if (!rc)
		rc = EPWfile.GetValCR(3, curRow, &inH);

	// error if wrong date
	if (!rc && (inDoy != doy || inH != iHr+1))  	// if unexpected day or hour
		return err( WRN,
					"Bad mon, day, or hr in input record at doy/hr %d/%d.",
					doy, iHr+1 );

	WFDATA* pWd = &wda[curRow-hdrLen];

	if (!rc)
		rc = EPWfile.GetValCR(13, curRow, &pWd->glrad);
	if (!rc)
		pWd->glrad = (float)Units.UnitConvert(pWd->glrad, unWhm2, unBtuFt2);

	if (!rc)
		rc = EPWfile.GetValCR(14, curRow, &pWd->bmrad);
#if 0	//	for debugging
x	if (month == 1 && iHr == 9)
x		printf("Wh/m2 = %f\t", pWd->bmrad);
#endif
	if (!rc)
		pWd->bmrad = (float)Units.UnitConvert(pWd->bmrad, unWhm2, unBtuFt2);
#if 0
x	if (month == 1 && iHr == 9)
x		printf("Btu/ft2 = %f\n", pWd->bmrad);
#endif

	if (!rc)
		rc = EPWfile.GetValCR(15, curRow, &pWd->dfrad);
	if (!rc)
		pWd->dfrad = (float)Units.UnitConvert(pWd->dfrad, unWhm2, unBtuFt2);

	if (!rc)
		rc = EPWfile.fGetFTempCR(6, curRow, &pWd->db);
	if (!rc)
		rc = EPWfile.fGetFTempCR(7, curRow, &dewpoint);
	if (!rc)
		rc = EPWfile.GetValCR(20, curRow, &pWd->wndDir);		//	deg from North(360)
	if (!rc)
		rc = EPWfile.GetValCR(21, curRow, &pWd->wndSpd);		//	m/s
	if (!rc)
		pWd->wndSpd = (float)Units.UnitConvert(pWd->wndSpd, unVelMS, unVelMiHr);	//	convert to mph
	if (!rc)
		rc = EPWfile.GetValCR(22, curRow, &pWd->cldCvr);		//	10ths of sky

	// Calculate derived field(s)
    if (!rc)
	{	if (dewpoint < 0.f)
			pWd->wb = psyTWetBulb( pWd->db, psyHumRat3(dewpoint));		// compute wet bulb
		pWd->wb = psyTWetBulb( pWd->db, psyHumRat3(dewpoint));		// compute wet bulb
	}
    return rc;
}	//	WFPACKER::readHrEPW
#endif
//-----------------------------------------------------------------------------
RC WDHR::wd_EPWReadHr(			// read 1 hour's data from EPW weather file
	WFILE* pWF,			// source file (open, header read, )
	int jHr,			// hour-of-year sought, 1-8760 (or 8784 TODO)
	int erOp /*=WRN*/)	// err msg control
{
	int yr, m, d, h, minute;
	char sSink[ 100];
	// temporaries for SI values
	float db, dp, rh, prs, irhoriz, glrad, bmrad, dfrad, wndDir, wndSpd, tsc, osc;
	RC rc = RCOK;
	while (1)
	{	char wfLine[ WFMAXLNLEN];
		pWF->yac->line( wfLine, sizeof( wfLine));
		rc = pWF->yac->getLineCSV( erOp|YAC_NOREAD, pWF->isLeap,
			"LLLLLCFFFFXXFFFFXXXXFFFF",
			wfLine,
			&yr, &m, &d, &h, &minute,	// year, month / day / hour / minute (all 1-based)
			_C( sSink),				// data sources and uncertainty flags
			&db,					// dry bulb temp, degC
			&dp,					// dew point temp, degC
			&rh,					// relative humidity, %
			&prs,					// atmospheric station pressure, Pa
			&irhoriz,				// sky radiation, Wh/m2
			&glrad,					// global horizontal irradiation, Wh/m2
			&bmrad,					// direct normal irradiation, Wh/m2
			&dfrad,					// diffuse horizonal irradiation, Wh/m2
			&wndDir,				// wind direction, deg
			&wndSpd,				// wind spd, m/s
			&tsc,					// total sky cover, tenths
			&osc,					// opaque sky cover, tenths
			NULL);
 		if (rc != RCBAD2)
		{	if (rc)
				return pWF->yac->errFlLn("Unreadable data");		// issue msg per last get's erOp with file & line, ret RCBAD

			int jDayFile = tddDoyMonBeg( m) + d - 1;
			int jHrFile = (jDayFile-1)*24 + h;		// hour of year
			if (jHrFile == jHr)
				break;
			else if (jHrFile < jHr)
				continue;
		}
		pWF->wf_EPWPosHr1( erOp);		// wrap
		// loop stopper?
	}


	if (!rc)
	{	wd_Init( 1);		// set all to UNSET
		// missing?
		wd_db = DegCtoF( db);
		wd_taDp = DegCtoF( dp);
		float w = psyHumRat3( wd_taDp);
		wd_wb = psyTWetBulb( wd_db, w);
#if 0	// test code
x		float rhx = psyRelHum( wd_db, wd_taDp);
x		if (fabs( rh/100.f - rhx) > .01)
x			printf( "mismatch\n");
#endif

		wd_glrad = IrSItoIP( glrad);
		wd_bmrad = wd_DNI = IrSItoIP( bmrad);
		wd_dfrad = wd_DHI = IrSItoIP( dfrad);

		wd_wndDir = wndDir;
		wd_wndSpd = VSItoIP( wndSpd);

		wd_cldCvr = tsc;

		wd_tSky = wd_CalcSkyTemp( C_SKYMODLWCH_BERDAHLMARTIN, h-1);

#if 0
// sky temp experiment
		float tSkyIR = DegRtoF(pow( IrSItoIP( irhoriz)/sigmaSB, 0.25));
		static FILE* pF = NULL;		// file
		if (!pF)
		{
			const char* fName = "tSky.csv";
			pF = fopen(fName, "wt");
			fprintf(pF, "yr,mon,day,hr,osc,tsc,tSky_CSE,tSky_IR\n");
		}
		fprintf(pF, "%d,%d,%d,%d,%0.1f,%0.1f,%0.1f,%0.1f\n", yr, m, d, h, osc, tsc, wd_tSky, tSkyIR);
#endif
	}
	return rc;
}		// WDHR::wd_EPWReadHr
#undef _C
//-----------------------------------------------------------------------------
RC WFILE::wf_FixJday(DOY& jDay, int begDay)
{
	if (jd1 > 0				// if weather file dates known
		&& jd1 + 364 != jdl		// if not a full year weather file (365-day file has all dates!)
		&&  jd1 - 1 != jdl)		// .. (eg Jul 1..Jun 30 is full year)
	{
		if (begDay < jd1)		// eg if file starts in Nov and run starts in Feb
			jDay += 365;			// add 365 to warmup start for comparability
		setToMax(jDay, jd1 - 1);		// move warmup start ahead to wthr file start if nec (-1 cuz ++ below)
	}
	jDay = (jDay + 365) % 365;		// adjust to 0-364 if negative or >= 365 (++'d b4 use)

	return RCOK;
}

#if 0

struct WFCSVMAP
{	int wm_iCol;		// "column" number (0 based)
	int wm_dt;			// data type
	int wm_units;		// source units
	int wm_offset;
	void* wm_dataP;

	void* GetDataP()
	{	return wm_offset >= 0
			? ((void *)((char*)this + wm_offset))
			: wm_dataP;
	}
};


int WDHR::wd_CSVParseHr(
	WFCSVMAP& colMap,
   	char* line,			// source line (will be modified herein, \0s added)
	int jHr,			// hour-of-year sought, 1-8760 (or 8784 TODO)
	int erOp /*=WRN*/)	// err msg control
{
char line[ WFMAXLNLEN];
const char* pCols[ WFMAXCOLS];

	int nCol = strTokSplit( line, ",", pCols, WFMAXCOLS);

	for (int iFn=0; colMap[ iFn].?; iFn++)
	{	W???
		int dt = colMap[ iFn].dt;
		int iCol = colMap[ iFn].iCol;
		if (iCol >= nCol)
			error

		void* pData = colMap[ iFn].offset >= 0

		if (dt == DTINT)
			ndc = strDecode( pCols[ iCol], *(int *)GetDataP(), ?)
		else if (dt == DTFLOAT)
		{	double v;
			ndc = strDecode( pCols[ iCol], v, ?)
			if (units)
				convert
			*(float *)GetDataP()



		}
		if (ndc != 1)





	}


#endif
//---------------------------------------------------------------------------



// end of wfpak.cpp
