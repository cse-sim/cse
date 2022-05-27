// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// XMODULE.h -- interfaces to external DLL/EXE functions
///////////////////////////////////////////////////////////////////////////////

#if !defined( _XMODULE_H)
#define _XMODULE_H

///////////////////////////////////////////////////////////////////////////////
// class XMODULE: base class
///////////////////////////////////////////////////////////////////////////////
class XMODULE
{
protected:
	const char* xm_moduleName;	// name of module from caller ("ashwat.dll")
								//   points to caller's string (don't pass temp string)
	HMODULE xm_hModule;			// module handle from LoadLibrary()
								//    NULL if not found (yet)
	char xm_modulePath[FILENAME_MAX];		// path to module file actually loaded
	int xm_RC;					// cumulative outcome

	XMODULE( const char* moduleName);
	XMODULE(){};
	virtual ~XMODULE();
	virtual void xm_ClearPtrs() { };
	RC xm_LoadLibrary();
	RC xm_SetModule(const char* moduleName);
	RC xm_Shutdown();
	int xm_IsLoaded() const { return xm_hModule != NULL; }
	FARPROC xm_GetProcAddress( const char* procName);
};	// class XMODULE
//=============================================================================

#if 0
0 ///////////////////////////////////////////////////////////////////////////////
0 // class XUZM: interface to UZM.DLL (unconditioned zone model)
0 ///////////////////////////////////////////////////////////////////////////////
0 class XUZM : public XMODULE
0 {
0
0 typedef	int _stdcall UZMINIT(
0	const char* inputFile, int debug, int level, int nSubSteps,
0	float patm, float qhCap, float qcCap,
0	float& attArea, float& attVol);
0 typedef int _stdcall UZMSUBHR(
0	int sh, int h, int d, int m, int dbPrint,
0	float tAmb, float qSky, float vWind, float qSolh, float qSolv,
0	float qAirNet, float anMCp, float anMCpT,
0	int mode, float tCZ, float qldCZ, float qCapHVAC,
0	float  qCbj, float  qCtj, float  qKbj, float  qKtj,
0	float& tCbj, float& tCtj, float& tKbj, float& tKtj,
0	float& tAttic, float& qNeed, float& sdl, float& rdl);
0 private:
0	// proc pointers
0	UZMINIT* xu_pUZMInit;
0	UZMSUBHR* xu_pUZMSubhr;
0	int xu_NANCount;		// subHr count of NANs returned
0	int xu_NANMsgCount;		// # of NAN messaged issued (re verosity limitation)
0	inline float xu_CheckNAN( float& v, float vN=0.f)
0	{	if (_isnan( v))
0		{	xu_NANCount++;
0			v = vN;
0		}
0		return v;
0	}
0
0 public:
0	XUZM( const char* moduleName);
0	~XUZM();
0	virtual void xm_ClearPtrs();
0	RC xu_Setup();
0	RC xu_Shutdown();
0
0	RC xu_Init( const char* inputFile, int debug, int level,
0		float qcCap, float& attArea, float& attVol);
0	RC xu_Subhr( class ZNR* pUZ, ZNR* pCZ, MSRAT* ucSrfs[]);
0 };		// class XUZM
0 //-----------------------------------------------------------------------------
0 extern XUZM UZM;		// public XUZM object
0 //=============================================================================
#endif

#if 0
0 ///////////////////////////////////////////////////////////////////////////////
0 // class XCZM: interface to CZM.DLL (conditioned zone model)
0 ///////////////////////////////////////////////////////////////////////////////
0 class XCZM : public XMODULE
0 {
0 typedef int _stdcall CZInit( int debugFileHandle);
0 typedef void _stdcall CZZB( int nhour, int stepno, int ndebug,
0 	double frConvH, double Dair, double Drad, double CX, double Nair, double Nrad,
0 	double TH, double TC, double TD, double QhCap, double QcCap, double QvInf, double QvCap,
0 	double& Qv, double& Tr, double& Ta, double& Qhc);
0 typedef int _stdcall CZFinish();
0
0 private:
0 	// proc pointers
0 	CZInit* xc_pCZInit;
0 	CZZB* xc_pCZZB;
0 	CZFinish* xc_pCZFinish;
0
0 public:
0 	XCZM( const char* moduleName);
0 	~XCZM();
0 	virtual void xm_ClearPtrs();
0 	RC xc_Setup();
0 	RC xc_Shutdown();
0
0 	RC xc_Init();
0 	RC xc_Subhr( ZNR* pCZ, ZNR* pUZ);
0 	// RC xc_Finish();	// finish now done in xc_Shutdown(); add separate call if needed
0
0 };		// class XCZM
0 //-----------------------------------------------------------------------------
0 extern XCZM CZM;		// public XCZM object
#endif
//=============================================================================
#endif	// _XMODULE_H

// xmodule.h end