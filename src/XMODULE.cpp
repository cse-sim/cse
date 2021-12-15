// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// xmodule.cpp -- interface to external DLL/EXE functions
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"
#if defined(SUPPORT_DLLS)
#include "envpak.h"
#include "ancrec.h"
#include "rccn.h"
#include "cnguts.h"
#include "mspak.h"

#include "xmodule.h"


///////////////////////////////////////////////////////////////////////////////
// class XMODULE: base class for external module (DLL / EXE) interfaces
///////////////////////////////////////////////////////////////////////////////
XMODULE::XMODULE( const char* moduleName)
	: xm_moduleName( moduleName), xm_hModule( NULL), xm_RC( RCOK)
{
}	// XMODULE::XMODULE
//-----------------------------------------------------------------------------
XMODULE::~XMODULE()
{
	xm_Shutdown();		// may have already been called, dup OK
}		// XMODULE::~XMODULE
//-----------------------------------------------------------------------------
RC XMODULE::xm_SetModule(const char* moduleName)
{
	xm_moduleName = moduleName;
	xm_hModule = NULL;
	xm_RC = RCOK;
	return xm_RC;
}	// XMODULE::xm_SetModuleName
//-----------------------------------------------------------------------------
RC XMODULE::xm_LoadLibrary()
{
	if (!xm_hModule)
	{	xm_hModule = LoadLibrary( xm_moduleName);
		doControlFP();
		if (!xm_hModule)
			xm_RC = errCrit( WRN|SYSMSG, "X0070: LoadLibrary() fail '%s'", xm_moduleName);
		else if (GetModuleFileName( xm_hModule, xm_modulePath, sizeof( xm_modulePath)) <= 0)
			xm_RC = RCBAD;
		if (xm_RC != RCOK)
			memset( xm_modulePath, 0, sizeof( xm_modulePath));
	}
	return xm_RC;
}		// XMODULE::xm_LoadLibrary
//-----------------------------------------------------------------------------
RC XMODULE::xm_Shutdown()		// finish up with DLL
// duplicate calls OK
// returns RCOK iff success or NOP
{
	RC rc = RCOK;
	if (xm_hModule != NULL)
	{	if (!FreeLibrary( xm_hModule))
			errCrit( WRN|SYSMSG, "X0071: FreeLibrary() fail '%s'", xm_moduleName);
		rc = RCBAD;
	}
	xm_hModule = NULL;
	xm_ClearPtrs();		// virtual: clear now-defunct function pointers in derived class
	return rc;
}		// XMODULE::xm_Shutdown
//-----------------------------------------------------------------------------
FARPROC XMODULE::xm_GetProcAddress(
	const char* procName)
{
	FARPROC pProc( NULL);
	if (!xm_hModule)
		xm_RC = RCBAD;
	else
	{	pProc = ::GetProcAddress( xm_hModule, procName);
		if (!pProc)
		{	errCrit( WRN|SYSMSG, "X0072: GetProcAddress() fail '%s:%s'",
				xm_moduleName, procName);
			xm_RC = RCBAD;
		}
	}
	return pProc;
}		// XMODULE::xm_GetProcAddress
//=============================================================================

#if 0
0 ///////////////////////////////////////////////////////////////////////////////
0 // class XUZM: interface to UZM (unconditioned zone model)
0 ///////////////////////////////////////////////////////////////////////////////
0 XUZM UZM( "UZM.DLL");		// public UZM object
0 //=============================================================================
0 XUZM::XUZM( const char* moduleName)		// c'tor
0 	: XMODULE( moduleName), xu_NANCount( 0), xu_NANMsgCount( 0)
0 {
0 	xm_ClearPtrs();
0 }
0 //-----------------------------------------------------------------------------
0 XUZM::~XUZM()
0 {
0 }	// XUZM::~XUZM
0 //-----------------------------------------------------------------------------
0 /*virtual*/ void XUZM::xm_ClearPtrs()
0 {
0 	xu_pUZMInit = NULL;
0 	xu_pUZMSubhr = NULL;
0 }		// XUZM::xm_ClearPtrs
0 //-----------------------------------------------------------------------------
0 RC XUZM::xu_Setup()
0 {
0 	if (xm_LoadLibrary() == RCOK)
0 	{	xu_pUZMInit		= (UZMINIT*)    xm_GetProcAddress( "UZMINIT");
0 		xu_pUZMSubhr    = (UZMSUBHR*)   xm_GetProcAddress( "UZMSUBHR");
0 #if 0
0 		xu_pUZMDay      = (UZMDAY*)     xm_GetProcAddress( "UZMDAY");
0 		xu_pUZMDayRun   = (UZMDAYRUN*)  xm_GetProcAddress( "UZMDAYRUN");
0 		xu_pUZMMonth	= (UZMMONTH*)   xm_GetProcAddress( "UZMMONTH");
0 		xu_pUZMMonthRun = (UZMMONTHRUN*)xm_GetProcAddress( "UZMMONTHRUN");
0 		xu_pUZMFinal	= (UZMFINAL*)   xm_GetProcAddress( "UZMFINAL");
0 		xu_pUZMMain		= (UZMMAIN*)    xm_GetProcAddress( "UZMMAIN");
0 #endif
0 	}
0 	return xm_RC;
0 }		// XUZM::xu_Setup
0 //-----------------------------------------------------------------------------
0 RC XUZM::xu_Init(
0 	const char* inputFile,	// UZM input file path
0 	int debug,				// nz to request debug output
0 	int level,				// diagnostic level (? details)
0 	float qcCap,			// required cooling capacity (Btuh, <0)
0 							//   used by UZM to estimate duct flow
0 	float& attArea,			// returned: attic area per UZM, ft2
0 	float& attVol)			// returned: attic vol per UZM, ft3
0 {
0 	RC rc = RCOK;
0 	int pbDebug = debug ? -1 : 0;		// PowerBASIC true is all bits on
0 	float qhCap = 0.f;					// currently unused, 9-2010
0 	float patm = Top.presAtm / 29.921;
0 	float aX, vX;
0 	int ret = (*xu_pUZMInit)( inputFile, pbDebug, level, Top.tp_nSubSteps, patm,
0 			qhCap, qcCap, aX, vX);
0 	if (!ret)
0 	{	rc = errCrit( WRN, "X0080: UZM Init fail");
0 		attArea = attVol = 0.f;
0 	}
0 	else
0 	{	attArea = aX;
0 		attVol = vX;
0 		xu_NANCount = xu_NANMsgCount = 0;
0 	}
0 	return rc;
0 }		// XUZM::xu_Init
0 //-----------------------------------------------------------------------------
0 RC XUZM::xu_Shutdown()		// finish use of UZM DLL
0 {
0 	return xm_Shutdown();
0
0 }		// XUZM::xu_Shutdown
0 //-----------------------------------------------------------------------------
0 RC XUZM::xu_Subhr(
0 	ZNR* pUZ,	// unconditioned zone (the UZM zone)
0 	ZNR* pCZ,	// associated conditioned zone
0 	MSRAT* ucSrfs[ ZNR::ucsrfCOUNT])	// common surfaces betw CZ and UZ
0 										// some/all may be NULL
0 {
0 	if (!xu_pUZMSubhr)
0 		return RCBAD;
0
0 // issues
0 // * daylight savings
0 // * sign convention on all heat flows
0
0 	// debug: 1=print subhour info, 2=print hour info
0 	//   debug file name specified in xxx.uzm input file
0 	DbDo( dbdUZMS|dbdUZMH);
0 	LI dbgPrintMask = Top.tp_SetDbMask();
0 	int ndebug = ((dbgPrintMask & dbdUZMS) != 0)
0 		     + 2*((dbgPrintMask & dbdUZMH) != 0);
0
0 	float vRad = Wthr.d.wd_glrad;	// TODO
0 	float qSky = QRad( Top.tSkySh, Top.tDbOSh);
0
0 #if 1	// change airnet linkage to flow + T re stability, 3-5-2011
0 	float qAirNet = 0.f;
0 	float fVent = pCZ->zn_fVent;		// vent fraction (from CZM)
0 	float anMCp  = (1.f - fVent)*pUZ->zn_anMCp[ 0]  + fVent*pUZ->zn_anMCp[ 1];
0 	float anMCpT = (1.f - fVent)*pUZ->zn_anMCpT[ 0] + fVent*pUZ->zn_anMCpT[ 1];
0 #elif 0	// bug fix, 3-4-2011
0 x	float anMCp = 0.;
0 x	float anMCpT = 0.;
0 x	float fVent = pCZ->zn_fVent;		// vent fraction (from CZM)
0 x	float qAirNet = pUZ->qIzSh			// total IZ heat including qAirNet[ 0]
0 x			+ fVent * (pUZ->qAirNet[ 1] - pUZ->qAirNet[ 0]);	// adjust per vent
0 #else
0 x	float qAirNet = pUZ->qIzSh;
0 #endif
0
0 	float tCZ = 70.f;
0 	float qldCZ = 0.f;
0 	if (pCZ)
0 	{	tCZ = pCZ->tz;
0 		qldCZ = pCZ->zn_qsHvac;
0 	}
0 	int mode = 0;			// 0=off, 1=heating, 2=cooling
0 	float qCapHVAC;				// available capacity *at system*
0 	if (fabs( qldCZ) < 1.)		// if no load
0 	{	mode = 0;		// system is off
0 		qCapHVAC = 0.f;
0 		qldCZ = 0.f;	// say load exactly 0
0 	}
0 	else if (qldCZ > 0.f)
0 	{	mode = 1;	// system is heating
0 		qCapHVAC = max( pCZ->i.znQMxH, 0.f);	// drop < 0
0 	}
0 	else
0 	{	mode = 2;	// system is cooling
0 		qCapHVAC = min( pCZ->i.znQMxC, 0.f);	// drop > 0
0 	}
0 	if (mode > 0 && fabs( qCapHVAC) < 1.f)
0 	{	mode = 0;
0 		qldCZ = 0.f;
0 		qCapHVAC = 0.f;
0 		if (pCZ)
0 			warn( "Zone '%s', %s: UZM qld>0 but qCapHVAC=0",
0 				pCZ->name, Top.When( C_IVLCH_S));
0 	}
0
0 	float qs[ ZNR::ucsrfCOUNT], ts[ ZNR::ucsrfCOUNT];
0
0 	MSRAT* pMS;
0 	for (int iS=0; iS < ZNR::ucsrfCOUNT; iS++)
0 	{	pMS = ucSrfs[ iS];
0 		qs[ iS] = pMS ? pMS->ms_area*pMS->ms_pMM->mm_qSrf[ 0] : 0.f;
0 	}
0
0 	float tAttic;
0 	float qNeed;
0 	float sdl;		// supply duct leakage, lbm/sec
0 	float rdl;		// return duct leakage, lbm/sec
0
0 #if defined( DEBUGDUMP)
0 	if (ndebug)
0 		DbPrintf( dbdALWAYS, "UZM call: tCZ=%.1f  qld=%.f  mode=%d  qCap=%.f   anQ=%.0f  anMCP=%.0f  anMCPT=%.0f  qCbj=%.1f  qCtj=%.1f  qKbj=%.1f  qKtj=%.1f\n",
0 			tCZ, qldCZ, mode, qCapHVAC, qAirNet, anMCp, anMCpT,
0 			qs[ ZNR::ucsrfCBJ], qs[ ZNR::ucsrfCTJ], qs[ ZNR::ucsrfKBJ], qs[ ZNR::ucsrfKTJ]);
0 #endif
0 	int ret = (*xu_pUZMSubhr)(
0 		Top.iSubhr+1, Top.iHr+1, Top.tp_date.mday, Top.tp_date.month, ndebug,
0 		Top.tDbOSh, qSky, Top.windSpeedSh,
0 		Wthr.d.wd_glrad, vRad,
0 		qAirNet, anMCp, anMCpT, mode, tCZ, qldCZ, qCapHVAC,
0 		qs[ ZNR::ucsrfCBJ], qs[ ZNR::ucsrfCTJ], qs[ ZNR::ucsrfKBJ], qs[ ZNR::ucsrfKTJ],		// input=mass heat flows
0 		ts[ ZNR::ucsrfCBJ], ts[ ZNR::ucsrfCTJ], ts[ ZNR::ucsrfKBJ], ts[ ZNR::ucsrfKTJ],		// returned=mass temps
0 		tAttic, qNeed, sdl, rdl);		// returned: zone temp, required HVAC, supply/return duct leakage
0
0 	// NaN UZM returns have been seen
0 	//   known cause: qldCZ > 0 and qCapHVAC == 0
0 	//   that case prevented above, but test for NaN as insurance
0
0 #if 1	// warning msgs for returned NANs, 8-9-2011
0 	xu_NANCount = 0;
0 	pUZ->tz = xu_CheckNAN( tAttic, pUZ->tz);	// leave unchanged if NAN tAttic
0 #if defined( _DEBUG)
0 	if (tAttic < -100.f || tAttic > 300.f)
0 		warn( "%s\nUZM '%s': Implausible air temp %.f\n",
0 			Top.When( C_IVLCH_S), pUZ->name, tAttic);
0 #endif
0 	pUZ->zn_qsHvac = xu_CheckNAN( qNeed);		// 0 if NAN qNeed
0 	pUZ->mdotSDL = xu_CheckNAN( sdl);
0 	pUZ->mdotRDL = xu_CheckNAN( rdl);
0 	for (int iS=0; iS < ZNR::ucsrfCOUNT; iS++)
0 	{	// set 1st node temp
0 		if (ucSrfs[ iS])
0 			ucSrfs[ iS]->inside.bc_exTa = xu_CheckNAN( ts[ iS], 70.f);
0 	}
0 	if (xu_NANCount)		// if any NANs
0 	{	if (xu_NANMsgCount <= 10)
0 		{	CWString skipMsg;
0 			if (xu_NANMsgCount == 10)
0 				skipMsg = "\n(skipping further NAN warnings for this UZM)";
0 			warn( "%s\nUZM '%s': UZM returned %d NAN values%s",
0 				Top.When( C_IVLCH_S), pUZ->name, xu_NANCount, skipMsg);
0 		}
0 		xu_NANMsgCount++;
0 	}
0 	if (xu_NANMsgCount && Top.tp_IsLastStep())
0 		// report total # of errors
0 		warn( "UZM '%s': NAN errors detected in %d subhour(s)",
0 			pUZ->name, xu_NANMsgCount);
0 #else
0 x#if defined( _DEBUG)
0 x	if (tAttic < -100.f || tAttic > 300.f)
0 x		warn( "UZM '%s': Implausible air temp %.f\n", pUZ->name, tAttic);
0 x#endif
0 x	if (!_isnan( tAttic))
0 x		pUZ->tz = tAttic;		// else leave unchanged
0 x	pUZ->zn_qsHvac = _isnan( qNeed) ? 0.f : qNeed;
0 x#if 1	// bug fix, 8-8-2011
0 x	pUZ->mdotSDL = _isnan( sdl) ? 0.f : sdl;
0 x	pUZ->mdotRDL = _isnan( rdl) ? 0.f : rdl;
0 x#else
0 x	pUZ->mdotSDL = sdl;
0 x	pUZ->mdotRDL = rdl;
0 x#endif
0 x	for (int iS=0; iS < ZNR::ucsrfCOUNT; iS++)
0 x	{	// set 1st node temp
0 x		if (ucSrfs[ iS])
0 x		{
0 x#if defined( _DEBUG)
0 x			if (_isnan( ts[ iS]))
0 x				ts[ iS] = 70.f;
0 x#endif
0 x			ucSrfs[ iS]->inside.bc_exTa = ts[ iS];
0 x		}
0 x	}
0 #endif
0
0 	// update duct efficiency iff there is a load
0 	//   note zn_ductEffs *must be* > 0
0 	if (fabs( qldCZ) > 1.f)
0 	{	if (qNeed > 0.)
0 			pUZ->zn_ductEffH = qldCZ / qNeed;
0 		else if (qNeed < 0.)
0 			pUZ->zn_ductEffC = qldCZ / qNeed;
0 		// else qNeed==0, carry eff forward
0 	}
0
0 #if defined( DEBUGDUMP)
0 	if (ndebug)
0 		DbPrintf( dbdALWAYS, "     ret: tAttic=%.2f qNeed=%.f dctEffH=%.3f dctEffC=%.3f sdl=%.4f rdl=%.4f tCbj=%.2f tCtj=%.2f tKbj=%.2f tKtj=%.2f\n",
0 			tAttic, qNeed, pUZ->zn_ductEffH, pUZ->zn_ductEffC,
0 			sdl, rdl, ts[ ZNR::ucsrfCBJ], ts[ ZNR::ucsrfCTJ], ts[ ZNR::ucsrfKBJ], ts[ ZNR::ucsrfKTJ]);
0 #endif
0
0 	return RCOK;
0
0 }	// XUZM::xu_Subhr
0 //=============================================================================
#endif


#if 0
0 ///////////////////////////////////////////////////////////////////////////////
0 // class XCZM: interface to CZM (unconditioned zone model)
0 ///////////////////////////////////////////////////////////////////////////////
0 XCZM CZM( "CZM.DLL");		// public CZM object
0 //=============================================================================
0 XCZM::XCZM( const char* moduleName)		// c'tor
0 	: XMODULE( moduleName)
0 {
0 	xm_ClearPtrs();
0 }
0 //-----------------------------------------------------------------------------
0 XCZM::~XCZM()
0 {
0 }	// XCZM::~XCZM
0 //-----------------------------------------------------------------------------
0 /*virtual*/ void XCZM::xm_ClearPtrs()
0 {	xc_pCZInit = NULL;
0 	xc_pCZZB = NULL;
0 	xc_pCZFinish = NULL;
0 }		// XCZM::xm_ClearPtrs
0 //-----------------------------------------------------------------------------
0 RC XCZM::xc_Setup()
0 {
0 	if (xm_LoadLibrary() == RCOK)
0 	{	xc_pCZInit = (CZInit *)xm_GetProcAddress( "CZ_INIT");
0 		xc_pCZZB = (CZZB *)xm_GetProcAddress( "ZONE_BALANCE");
0 		xc_pCZFinish = (CZFinish *)xm_GetProcAddress( "CZ_FINISH");
0 	}
0 	return xm_RC;
0 }		// XCZM::xc_Setup
0 //-----------------------------------------------------------------------------
0 RC XCZM::xc_Init()
0 {	if (!xc_pCZInit)
0 		return RCBAD;
0 	return (*xc_pCZInit)( 0);
0 }		// XCZM::xc_Init
0 //-----------------------------------------------------------------------------
0 RC XCZM::xc_Subhr(
0 	ZNR* pCZ,	// conditioned zone (this zone)
0 	ZNR* pUZ)	// associated unconditioned zone
0 				//   NULL if none
0 {
0 	if (!xc_pCZZB)
0 		return RCBAD;
0
0 	int nhour = (Top.jDay-1)*24 + Top.iHr;
0 	int stepno = Top.iSubhr+1;
0 	DbDo( dbdCZM);
0 	int ndebug = 2*((Top.dbgPrintMask & dbdCZM) != 0);	// 0 or 2
0 	double frConvH = 1.;	// heating convective fraction
0 	double Nair = pCZ->zn_nAirSh;
0 	double Dair = pCZ->zn_dAirSh;
0 	double Nrad = pCZ->zn_nRadSh;
0 	double Drad = pCZ->zn_dRadSh;
0 	double CX = pCZ->zn_cxSh;
0
0 	double TH = pCZ->i.znTH;
0 	double TC = pCZ->i.znTC;
0 	double TD = pCZ->i.znTD;
0 #if 1		// duct efficiency, 3-11
0 	double QhCap =  max( 0., pCZ->i.znQMxH) * (pUZ ? pUZ->zn_ductEffH : 1.);
0 	double QcCap =  min( 0., pCZ->i.znQMxC) * (pUZ ? pUZ->zn_ductEffC : 1.);
0 #else
0 x	double QhCap =   1000000.;
0 x	double QcCap =  -1000000.;
0 #endif
0 	double QvInf = pCZ->qAirNet[ 0];
0 	double QvCap = min( pCZ->qAirNet[ 1], pCZ->qAirNet[ 0]);	// never allow vent to heat
0 	double Qv;
0 	double Tr;
0 	double Ta;
0 	static double Qhc = 0.;		// retain for next step
0
0 #if defined( DEBUGDUMP)
0 	if (ndebug)
0 		DbPrintf( dbdALWAYS,
0 			"%s CZM call: TH=%.f  TD=%.f  TC=%.f  qvInf=%.f  qvCap=%.f  qhCap=%.f  qcCap=%.f\n"
0 			"          Nair=%.2f  Dair=%.2f  Nrad=%.2f  Drad=%.2f  CX=%.2f\n",
0 			pCZ->name, TH, TD, TC, QvInf, QvCap, QhCap, QcCap, Nair, Dair, Nrad, Drad, CX);
0 #endif
0
0 	(*xc_pCZZB)(
0 		nhour, stepno, ndebug, frConvH, Dair, Drad, CX, Nair, Nrad,
0 		TH, TC, TD, QhCap, QcCap, QvInf, QvCap,
0 		Qv, Tr, Ta,		// out: returned
0 		Qhc);			// in/out
0
0 	pCZ->tz = Ta;
0 	pCZ->tr = Tr;
0 	pCZ->zn_qsHvac = Qhc;
0 	pCZ->zn_fVent = QvCap != QvInf ? (QvInf-Qv)/(QvInf - QvCap) : 0.;
0 	pCZ->zn_qIzSh = Qv;
0
0 #if defined( DEBUGDUMP)
0 	if (ndebug)
0 		DbPrintf( dbdALWAYS, "     ret: ta=%.2f  tr=%.2f  qv=%.f  fvent=%.3f  qhc=%.f\n",
0 			Ta, Tr, Qv, pCZ->zn_fVent, Qhc);
0 #endif
0
0 #if 0
0 x	ZNRES_IVL_SUB* zrs = &ZnresB.p[ pCZ->ss].curr.S;
0 x	if (Qhc < 0.)
0 x		zrs->qcMech += Qhc;
0 x	else
0 x		zrs->qhMech += Qhc;
0 #endif
0
0
0 	return RCOK;
0 #if 0
0  SUB ZONE_BALANCE (  _
0     BYVAL nhour AS LONG, _      ' hour of year, 0 - 8759 (debug print documentation only)
0     BYVAL stepno AS LONG, _     ' substep with hour 1 ...
0     BYVAL ndebugX AS LONG, _    ' debug print control (0 or 2)
0     BYVAL frConvH AS DOUBLE, _  ' heating fraction convective
0     BYVAL Dair AS DOUBLE, _     ' denominator terms
0     BYVAL Drad AS DOUBLE, _
0     BYVAL CX AS DOUBLE, _       ' conv / radiant node coupling
0     BYVAL Nair AS DOUBLE, _     ' numerator terms
0     BYVAL Nrad AS DOUBLE, _
0     BYVAL TH AS DOUBLE, _       ' heating setpoint, F
0     BYVAL TC AS DOUBLE, _       ' cooling setpoint, F
0     BYVAL TD AS DOUBLE, _       ' desired zone air temp, F
0     BYVAL QhCap AS DOUBLE, _    ' available heating capacity, Btuh (>= 0)
0     BYVAL QcCap AS DOUBLE, _    ' available cooling capacity, Btuh (<= 0)
0     BYVAL QvInf AS DOUBLE, _    ' heat transfer to zone air with infil venting only, Btuh, + = into zone
0     BYVAL QvCap AS DOUBLE, _    ' max possible heat transfer to zone with infil + vent, Btuh, + = into zone
0     BYREF Qv AS DOUBLE, _       ' returned: vent transfer
0     BYREF Tr AS DOUBLE, _       ' returned: zone radiant node temp, F
0     BYREF Ta AS DOUBLE, _       ' returned: zone air temp, F
0     BYREF Qhc AS DOUBLE      ) EXPORT   ' returned: heating/cooling requirements, Btuh, + = heating
0 #endif
0
0
0 }	// XCZM::xc_Subhr
0 //-----------------------------------------------------------------------------
0 RC XCZM::xc_Shutdown()		// finish use of CZM DLL
0 {
0 	RC rc = RCOK;
0 	if (xm_hModule != NULL)
0 	{	if (xc_pCZFinish)
0 			(*xc_pCZFinish)();
0 		rc = xm_Shutdown();
0 	}
0 	return rc;
0 }		// XCZM::xc_Shutdown
0 //=============================================================================
#endif // #if 0
#endif // SUPPORT_DLLS
// xmodule.cpp end