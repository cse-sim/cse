// Copyright (c) 1997-2021 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// CSE.cpp : Main program module
//           Defines entry points
///////////////////////////////////////////////////////////////////////////////

// CSE: California Simulation Engine
//      a program to simulate a building and its HVAC systems to determine
//		energy consumption under particular weather conditions.
//
// Detailed input for one or more runs is given in input file named in an argument;
// results are returned in one or more report files and/or data export files.
//
// Returns non-0 if errors occurred; details in error message file.
//
// See manual.

// TODO list, some old / unverified
// * DONE. Idea: add top-level "vent available" flag.  Do only 1 airnet if vent not available
// * Problem: can get AirNet convergence errors of no hole in zone.  May need way to
//         disable AirNet -- if disabled heat due to air but ignore the mass?
// * does "adiabatic" make sense as a choice for ducts exterior conditions? 5-12
// * review psychro.cpp.  Not clear "enhancement" is correct.
//   Need better behavior at extremes?  5-12
// * DONE Make RSYS ducts optional 5-12
// * error message if RSYS capacity has wrong sign?
// * rework culMbrId() -- need pointer from basAnc to CULT?  see comments
//   about non-unique fn re e.g. SURFACE / DOOR / WINDOW. 3-12
// * complete IZXFER UA coupling scheme (including ONEWAY) 3-12
// * are we OK on default surface roughness? 3-12
// * rework floor height / eave height.  Need Z0 for building? 3-12
// * Add airnet ACH to znRes?  Outdoor only.  Note znres.airX already exixts.  3-12
// * Add air flow meter for AirNet.  Mass or energy? Separate + and -. 4-12
// * Enforce zone order in HORIZ airnet. 4-12
// * Rework znres mbrs to be double?  ditto MTR? 4-12
// * Rework keyboard interrupt stuff (enkimode etc), good to be able to cancel a run, 4-12
// * Add ability to set hr = surface radiant coefficient (for testing)
// * Use UNIFIED convection model on duct exteriors (including airchange-based forced) 4-12
// * DONE eliminate ZNR.qAirNet?  Now using zn_anMCp etc.  Interacts with smoothing, 3-12
// * unified record::Copy and record::CopyFrom().  Why do both exist? 3-12
// * relate Sherman-Grimsrud infiltration to general wind / terrain etc 3-12
// * review child surface inheriting of parent sfExHcModel and sfInHcModel 3-12
// * find all refs to CNE, change to CSE. 4-12
// * search NUMS for texts to extract 6-95
// * review all records for destructors that delete dm strings including those
//   cult tables point to 6-93
// * exman.cpp: call exClr; do full review. 6-93
// * cnguts:cgClear must also clean other cg modules 6-93
// * minor BUG observed 7-26-92 by rob: error in command line -D (eg = changed
//   to space via batch file) neither appears in error file nor stops run
//    --> bad define makes mystery errors.

/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"

#include <process.h>	// exit

#include <ancrec.h>		// record: base class for rccn.h classes
#include <rccn.h>		// TOPRATstr

#include <vrpak.h>		// vrInit vrTerminate
#include <xiopak.h>		// xfClean
#include <envpak.h>     // ensystd KICLEAR KIBEEP

#include <pp.h>			// ppClargIf ppAddPath
#include <exman.h>		// exWalkRecs
#include <cul.h>		// cul
#include <cncult.h>		// cnPreInit freeHdrFtr
#include <irats.h>		// Topi
#include <msghans.h>	// MH_xxxx message handle defns. MH_C0100.
#include <messages.h>	// message retrieval fcns, msgInit
#include <rmkerr.h>		// error reporting fcns, errFileOpen, log, screen

#include "timer.h"      // tmrInit
#include <tdpak.h>      // tddtis
#include <cuparse.h>	// showProbeNames
#include <cnguts.h>

#include "csevrsn.h"	// version #

#include "cse.h"		// decls for this file

#include <penumbra/penumbra.h>	// penumbraInit penumbraTerminate for GPU calculations


// configuration defined on compiler command line (or inferred, see cnglob.h)
#include "cseface.h"

#if defined(WINorDLL)
#include <cnewin.h>	// public declarations for Windows caller of CSE subroutine. Declares cne() for Windows (linked or DLL).
#ifdef DLL
// #include <cnedll.h>	// dll-only stuff: hInstLib. 1-18-94.
HINSTANCE hInstLib = 0;	// library's module instance handle.
#endif
#endif


/*------------------------------ PUBLIC DATA -------------------------------*/
#ifdef WINorDLL
static const char ConsoleTitle[] = "CSE Simulation Engine";	// caption for "console" window displayed during run, used below.

const char MBoxTitle[] = "CSE Simulation Engine";		// caption for error and other message boxes (rmkerr.cpp).
#endif

const char ProgName[] = "CSE";

const char ProgVersion[] = CSEVRSN_TEXT;	// program version text "x.xxx" (csevrsn.h)

const char ProgVariant[] = 	// text showing platform
	#if defined( WIN)
		"for Win32";
	#elif defined( DLL)
		"Win32 DLL";
	#elif defined( CSE_DLL)
		"DLL";
	#elif defined( CSE_CONSOLE)
		"for Win32 console";
	#endif

int TestOptions = 0;	// test option bits, set via -t command line argument
						//   1: hide directory paths in error messages (show file name only)
						//      allows location-independent reference report files, 1-2016

const char* cmdLineArgs = NULL;			// command line arguments (argv[ 1 ..]) (in dm)
										//   suitable for display (wrapped if long)
const char* exePath = NULL;				// path(s) to .exe file or dll;exe in dm
const char* InputFileName = NULL;		// input file name as entered by user: no added path nor defaulted ext.
										//   = pts into cne3
const char* InputFilePath = NULL;		// input file full path. ptr into cse.cpp:cne3() stack.
const char* InputFilePathNoExt = NULL;	// input file full pathName with any .ext removed, in dm.
const char* InputDirPath = NULL;		// drive/dir path to input file, in dm.
VROUTINFO5 PriRep = { { 0 } };	// information about primary output file, for appending final end-session info in cse.cpp
								// (after report file input records have been deleted).
								// out file members set from cncult (at input) and vrpak (at close)
								// has room for 5 vrh's (set where used, in cse.cpp).

// run serial number in lieu of future status file, 7-92.
SI cnRunSerial = 0;		// incremented in cgInit; copied to Topi.runSerial in cncult2\TopStarPrf2.

// virtual report unspooling specifications for this run
VROUTINFO* UnspoolInfo = NULL;	// dm block of info re unspooling virtual reports into actual report files.
								// set up in cncult.cpp; passed to vrUnspool then dmfree'd in cse.cpp;
								// vrpak.h struct.  Note: vrUnspool dmfree's .fNames in info as it closes files.


#ifdef WINorDLL
HINSTANCE cneHInstApp = 0;	// application instance handle: needed eg for registering window classes eg in rmkerr.cpp.
							// Is server's hInst for client-server, but caller's hInst for DLL (see hInstLib)
HWND cneHPar = 0;  			// application window handle to use as parent for windows such as error message boxes
#endif

#ifdef OUTPNAMS				// defined in cnglob.h if WINorDLL to return file names to caller
static char * outPNams = NULL;	/* points to output pathnames used (error,report,export) with ;'s between them,
	 	 			   for return to caller via outPNams argument. */
#endif

#ifdef BINRES
struct BrHans* hans = NULL;	/* NULL, or dm pointer to structure (cnewin.h) for return of binary
  					   results memory block handles if pointer for its return given in call.
  					   NULL if not returning handles; always NULL under DOS.
  					   NULL value disables features that generate non-filed memory results.*/
#endif

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

#ifdef WINorDLL
LOCAL void ourGlobalFree( HGLOBAL *pHan, char *desc);
#endif
LOCAL INT cse1( INT argc, const char* argv[]);
LOCAL INT cse2( INT argc, const char* argv[]);
LOCAL INT cse3( INT argc, const char* argv[]);
LOCAL void zStaticVrhs( void);
LOCAL void cnClean( CLEANCASE cs);

#if defined( CSE_MFC)

///////////////////////////////////////////////////////////////////////////////
// class CSEAPP: application class for CSE
///////////////////////////////////////////////////////////////////////////////
class CSEAPP : public CWinApp
{
public:
	CSEAPP();
	virtual ~CSEAPP();
	BOOL InitApp();
};		// class CSEAPP
//=============================================================================

#if defined( CSE_CONSOLE)
///////////////////////////////////////////////////////////////////////////////
// main function
///////////////////////////////////////////////////////////////////////////////
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[])
{
	int ret = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		printf( "Fatal Error: MFC initialization failed");
		ret = 1;
	}
	else
	{
		ret = cse( argc, (const char **)argv);	// use inner main function, cse.cpp.
	}
	return ret;
}	// _tmain
#endif	// CSE_CONSOLE

///////////////////////////////////////////////////////////////////////////////
// class CSEAPP: application object
///////////////////////////////////////////////////////////////////////////////
static CSEAPP CSEApp;		// the one and only app
//-----------------------------------------------------------------------------
CSEAPP& App()		// public access to application
{	return CSEApp;
}
//-----------------------------------------------------------------------------
CSEAPP::CSEAPP()		// c'tor
	: CWinApp()
{
}	// CSEAPP::CSEAPP
//-----------------------------------------------------------------------------
/*virtual*/ CSEAPP::~CSEAPP()
{
}	// CSEAPP::~CSEAPP
//----------------------------------------------------------------------------
BOOL CSEAPP::InitApp()			// application initialization
// returns TRUE iff application is runable
{
	return TRUE;
}		// CSEAPP::InitApp
//---------------------------------------------------------------------------
#if 0
x early experimental code (2010)
x /*virtual*/ BOOL CSEAPP::ParseParam(			// parse command line parameters
x	const char* pszParam,		// parameter or flag
x	BOOL bFlag,					// TRUE if parameter (!) (else flag)
x	BOOL bLast)					// TRUE iff last param/flag on command line
x// returns TRUE if param processed
x{
x
x// -wf<path> = weather file path
x	if (bFlag && !_strnicmp( pszParam, "wf", 2))
x	{
x		za_wFilePath = tTrim( pszParam+2);
x		return TRUE;
x	}
x
x	return CWAppBase::ParseParam( pszParam, bFlag, bLast);
x}		// CSEAPP::ParseParam
x//--------------------------------------------------------------------------
xint CSEAPP::Run()			// run MZM
x// returns retCode (for return to OS by main())
x//         0 = success
x//        >0 = errors TBD
x{
x	int ret = 0;
x	if (!InitApp())
x		return 1;		// could not initialize
x
x	MZSIM S;
x	S.SetWthrFile( App().za_wFilePath);
x
x	printf( "MZApp::Run\n\n");
x
x	return ret;
x}		// CSEAPP::Run
#endif
//=============================================================================

#else
// not CSE_MFC
//===========================================================================
int main( int argc, char *argv[])		// CSE.EXE main program

// argc is argument count, including arg[0].
// argv[] contains pointers to null-terminated command line argument strings, as parsed by compiler-supplied startup code.
// argv[0] is program .exe file name, with full path. Path is used by cse to find other required files.

// return value is errorlevel to return: 0 ok, 255 ^C 1-95, other non-0 other error.
{
	int ret = cse( argc, (const char **)argv);
	return ret;

}	// main

#endif	// CSE_MFC

///////////////////////////////////////////////////////////////////////////////
//  External entry points to CSE package follow:
//     cseCleanup(), cseHansFree(), CNEZ(), and cse() itself.
//=====================================================================================================================

#if defined( WINorDLL)	// currently 2-94 not used under DOS -- no outPNams nor hans.
_DLLImpExp void cseCleanup()		// cse cleanup function to call at application exit

// does:
//	 1. frees outPNams memory block, a pointer to which cse() returns to caller
//	    (better Windows-only solution might be to return Windows handle not ptr and let caller free).
//	 2. should it do general cleanup as extra insurance? not till need shown,
//	    cuz all error exits purport to do it, and also this fcn currently not used under DOS.
{
#ifdef OUTPNAMS //cnglob.h

// Free any previous run concatenated output file names memory block.
// (This block must be left allocated at cne() return because cse returns a ptr to it via argument _outPNams.)
// Does not free handles IN the block -- caller must do himself or use cneHansFree (next).

	dmfree( DMPP( outPNams)); 	// free block if ptr not NULL, and set pointer to NULL.
	// fcn in lib\dmpak.cpp; reference argument.
#endif

#ifdef BINRES //cnglob.h

// Free any previous run binary results memory handles block.
// (This block must be left allocated at cse() return because cse returns a ptr to it via argument _hans.)

	dmfree( DMPP( hans));		// free block if ptr not NULL, set ptr NULL. lib\dmpak.cpp.

#endif

// ?? could also do cnClean(DONE) if cnClean(ENTRY) has been done (per new flag).

}			// cneCleanup
#endif	// WINorDLL
//=====================================================================================================================
#if defined( WINorDLL)	// currently 3-94 not used under DOS -- no outPNams nor hans.
_DLLImpExp void cneHansFree(	// free memory whose handles previously returned to
					 			// caller using optional "hans" argument to cse()

	struct BrHans *hans )	// pointer to Binary Results memory Handles structure (or NULL)
// -- pointer returned by cse via "hans" argument,
//    or pointer to copy you have made of storage pointed to by ptr returned *hans by cse.
{
	if (hans)		// do nothing if NULL pointer given
	{
		// call local function (next) for each member. does GlobalFree + checks & messages & replaces handle with 0.

		ourGlobalFree( &hans->hBrs, "Brhans.hBrs");
		ourGlobalFree( &hans->hBrhHdr, "Brhans.hBrhHdr");
		for (INT i = 0; i < 12; i++)
			ourGlobalFree( &hans->hBrhMon[i], "Brhans.hBrhMon[i]");
	}
}			// cneHansFree
//---------------------------------------------------------------------------------------------------------------------
LOCAL void ourGlobalFree( 	// Windows GlobalFree plus checks & messages
	// internal function for cneHansFree (just above)

	HGLOBAL *pHan, 	/* pointer to Windows global memory handle (or 0).
    			   Handle replaced with 0 if successfully free'd. */
	char *desc )	// descriptive text for insertion in error messages
{
	if (pHan)			// if pointer not NULL (prevents GP fault on bad call)
	{
		if (*pHan)		// if pointed-to handle is non-0 (0 indicates already free'd or never used)
		{
			char buf[200];
			if (GlobalFlags(*pHan) & GMEM_LOCKCOUNT)   	// test for 0 lock count, to facilitate explicit error message
			{
				sprintf( buf, "Error in cneHansFree():\n\n"		// lock count non-0. format message.
						 "Global handle 0x%x (%s) is locked",
						 (unsigned)*pHan, desc );
				MessageBox( 0, buf, MBoxTitle, MB_ICONSTOP | MB_OK );	// display message. awaits click or keypress.
			}
			else 						// not locked
				if (GlobalFree(*pHan))				// free the memory, return value 0 if ok
				{
					sprintf( buf, "Error in cneHansFree():\n\n"		// GlobalFree error (unexpected). format message.
							 "GlobalFree() failed, handle 0x%x (%s)",
							 (unsigned)*pHan, desc );
					MessageBox( 0, buf, MBoxTitle, MB_ICONSTOP | MB_OK );	// display message.
				}
				else						// GlobalFree succeeded
					*pHan = 0;					// zero the handle so won't use it or free it again
		}
	}
}		// cneGlobalFree
#endif	// WINorDLL

//////////////////////////////////////////////////////////////////////////////
// screen message callback scheme
static CSEMSGCALLBACK* pMsgCallBack = NULL;	// pointer to msg handling function (from caller)
static int isCallBackAbort = 0;				// set nz if callback has ever returned CSE_ABORT
//----------------------------------------------------------------------------
void LogMsgViaCallBack( const char* msg, int level /*=0*/)
{
	if (pMsgCallBack)
	{	int ret = (*pMsgCallBack)( level, msg);
		if (ret == CSE_ABORT)
			isCallBackAbort = 1;
	}
}		// LogMsgViaCallBack
//-----------------------------------------------------------------------------
int CheckAbort()
{	return isCallBackAbort;
}		// CheckAbort
///////////////////////////////////////////////////////////////////////////////

//=====================================================================================================================
//  The primary entry point for C callers, cse()
//=====================================================================================================================
_DLLImpExp int cse( 		// CSE main function, called by console main(), Windows WinMain, or application using DLL.
	int argc, 			// argument count (size of argv), including arg[0], which is (client) .exe file pathname.
	const char* argv[],	// array of pointers to argument strings, each a file name or flag.
						// multiple file names cause multiple calls from cne1 to cse2 level
	[[maybe_unused]] CSEMSGCALLBACK* _pMsgCallBack /*=NULL*/
#if defined( WINorDLL)
	, HINSTANCE _hInst		// calling application instance handle (but server's if client-server; see hInstLib for DLL's)
	, HWND _hPar    		// calling application Window handle -- parent of windows displayed here
	, char far * far * _outPNams	// NULL or points to location to receive pointer to list of pathnames of
									// output files written during CSE execution, in form path;path;path;...\0.
									// Includes report file(s), export file(s), and error/warning file, if any
	, struct BrHans far * far * _hans	// NULL or points to location to receive pointer to structure (cnewin.h)
  									// of Windows global memory handles for binary results data, for fastest access.
  									// This data is generated only if appropriate specs are in input file
  									// or on the command line (and only if BINRES -- cnglob.h)
  									// Caller must GlobalFree any handles returned, or use cneHansFree (above).
#endif
)

/* argv is command line broken into substrings at spaces.
     Command line is thus parsed by compiler startup code for DOS version.
   argv[0] must be pathName from which .exe file was started (used to find other required files).
     Under windows, caller may get this with GetModuleFileName().
   At least one arg after argv[0] must be present: input file name.
     Additional arguments may contain option flags. */

// return value is errorlevel to return to DOS or to caller of subroutine package or library: 0 ok, non-0 error, 255 ^C 1-95.
// specific error information has been given on screen and, usually, in error message file.

// this function level does the grossest environment checks, then calls cne1 (below).
{

//---- major checks that should be done before calling library code

#if defined( WINorDLL)	// added 9-21-94
	//-- check parent window handle argument

	if (!IsWindow(_hPar))
	{
		char buf[100];
		sprintf( buf, "Error in call to cse():\n\n"
				 "Parent window handle 0x%x is invalid.",
				 (int)_hPar );
		MessageBox( 0, buf, MBoxTitle, MB_ICONSTOP | MB_OK );
		return 2;
	}
#elif defined( CSE_DLL)
#if !defined( LOGCALLBACK)
#error LOGCALLBACK required for CSE_DLL build
#endif
	pMsgCallBack = _pMsgCallBack;
#endif	// CSE_DLL

	isCallBackAbort = 0;	// clear abort flag (1 if prior run aborted)

	//-- nested entry protection: DLL only has 1 copy of variables even if 2 copies of app loaded
	static int nest = 0;
	if (nest)
	{
#if defined( WINorDLL)
		// can't use any rmkerr.cpp functions cuz they would use the other caller's _hPar.
		MessageBox( _hPar,
					"Simultaneous CSE use not supported.\n\n"
					"If you are running two CSE applications, "
					"wait for the other to finish simulating.",
					MBoxTitle,
					MB_ICONSTOP | MB_OK );
#else // console
		printf( "\nSimultaneous CSE use not supported.\n"
				"\nIf you have two copies of application loaded,"
				"\n     let other one finish simulating first\n" );
#endif
		return 2;
	}
	nest++;				// ok, can use CSE. Say in use.

//---- ok, now it is safe to store arguments in globals

#ifdef WINorDLL //cnglob.h
	cneHInstApp = _hInst;  	// application instance handle: needed eg for registering window classes eg in rmkerr.cpp.
	cneHPar =  _hPar;		// application window handle to use as parent for windows opened eg in rmkerr.cpp.
#endif

#ifdef OUTPNAMS //cnglob.h
	// _outPNams is used at return.
	// outPNams is free'd now cuz must remain at return so caller can use its contents.
	// *** what if this is a call by a different program? should we return Windows handle not ptr and let caller free?
	//     (above is problem for DLL only -- static link is private, client interface to server copies)
	dmfree( DMPP( outPNams)); 	// init pointer to concatentated pathnames for return via _outPNams: free and set to NULL
#endif

#ifdef BINRES //cnglob.h
#ifdef WINorDLL //Windows memory handles cannot be saved or returned under DOS!
	// any (previously returned) hans is free'd now cuz must remain at return so caller can use its contents.
	// *** what if this is a call by a different program? should we return Windows handle not ptr and let caller free?
	//     (above is problem for DLL only -- static link is private, client interface to server copies)
	// *** handles IN THE BLOCK are not free'd here -- caller must do himself or use cneHansFree (above).
	dmfree( DMPP( hans));		// free any old (previously returned) hans
	if (_hans)						// if return of handles structure requested
		dmal( DMPP( hans), sizeof(BrHans), DMZERO);	// allocate structure now; msg & continue if no memory
	// non-NULL hans serves as flag enabling results output to memory without also outputting to a file.
#endif
#endif

//---- open "screen" display window
#if defined( LOGWIN)
	screenOpen( cneHInstApp, 				// open window to receive "console" output under Windows. rmkerr.cpp.
				cneHPar,
				ConsoleTitle );  			// title bar text, at head of this file: "CSE Simulation Engine".
#endif

//---- call next level

	INT errlvl = cse1( argc, argv);	// below. returns 0 ok, 255 ^C 1-95, other non-0 other error.

//---- return info

#ifdef OUTPNAMS		// cnglob.h; only defined if WINorDLL.
	if (_outPNams)  			// if pointer argument given
		* _outPNams = outPNams;		// return location of block containing concatenated output pathnames
	// not free'd --> memory leak, 2-14-94.  Add entry to free?
	else				// no pointer given for return
		dmfree( DMPP( outPNams)); 	// free now, set pointer NULL
#endif
#ifdef BINRES
#ifdef WINorDLL	// no such return under DOS (tho bin res FILES can be generated)
	if (_hans)			// if pointer argument given
		* _hans = hans;		// return location of block of binary results memory handles
#endif
#endif

//---- close "screen" display window
#if defined( LOGWIN)
	// do we want this? 10-93 Maybe only if .err file has been created??
	if (!getBatchMode())  				// rmkerr.cpp
		erBox( "Run(s) done. Click to continue.", 0, 0);	// rmkerr.cpp
	screenClose();					// rmkerr.cpp.
#endif

//---- finally, close/clean up error message stuff

	// done here to be after cnClean(), after byebye() (could issue message?),
	// and after (any future possible) msgs in above exit code

	errClean();			// close disk text file if open, free buffers, close error output file
						//   if still open (not expected). rmkerr.cpp. rob 12-3-93.
	nest--;
	return errlvl;
}				// cse
//=====================================================================================================================
//  CSEZ(): more accessible CSE entry point for non-C callers: parses command line into argv[].
//=====================================================================================================================
extern "C" {
_DLLImpExp int CSEZ( 			// interface to CSE main function, callable by WinMain or application using DLL or subroutine

	const char* _exePath,   // calling program pathName, searched for other files (get with GetModuleFileName)
	const char* _cmdLine,   	// string of fileName and flag arguments as would be given on command line
	[[maybe_unused]] CSEMSGCALLBACK* _pMsgCallBack /*=NULL*/
#if defined( WINorDLL)
	, HINSTANCE _hInst		// additional arguments as for cse() -- see descriptions below
	, HWND _hPar
	, char** _outPNams
	, struct BrHans** _hans
#endif
)
// return value is errorlevel: 0 ok, non-0 error, 255 ^C.
{
	const int MAXARGS=65;
	const char* argv[ MAXARGS];	// believe can't overflow

// arg[0] is exepath
	argv[ 0] = _exePath
				   ?  _exePath		// arg 0 is pathName of CALLING PROGRAM
				   :  "";			// change NULL to "": insurance

// parse arg[1]... out of cmdLine
	char cmdLine[ 2048];		// working copy (strTokSplit modifies in place)
	strncpy0( cmdLine, _cmdLine, sizeof( cmdLine));
	int argc = 1 + strTokSplit( cmdLine, " \t", argv+1, MAXARGS-1);
	if (argc < 0)
		return 1;		// munged command line (unexpected)

// call "real" main
	return cse( 					// call the CSE level
			   argc, argv				// # arguments and array of string pointers to arguments
#if defined( CSE_DLL)
			   , _pMsgCallBack
#elif defined( WINorDLL)
			   , _hInst, _hPar, _outPNams, _hans	// pass thru additional Windows/dll args
#endif
		   );
}			// CSEZ
}			// extern "C"
//-----------------------------------------------------------------------------
extern "C" {
_DLLImpExp int CSEProgInfo( 			// returns
	char* buf,   // where to write info
	size_t bufSz)
// return # of characters written to buf
{
   return _snprintf_s( buf, bufSz, _TRUNCATE,
			"%s %s %s", ProgName, ProgVersion, ProgVariant);
}		// CSEInfo
}		// extern "C"

//------------------------------------------------------------------------
LOCAL INT cse1( INT argc, const char* argv[])

// This function level rearranges arguments when multiple input files are given,
// calling cse2 (below) for each input file with cumulative flag (switch) arguments

// ALL flags PRECEDING file name are passed to cse3 with file name;
// for last file name, following switches are also passed (cse3 may then issue warning). */

// Args as for cse().

// return value is 'or' of return values from the multiple calls to cse2().

{
	INT errlvl = 0;
	const char* argvx[ 64];	// argv[] for cse2: one filename, all flags to present filename, args after if last file.
	argvx[0] = argv[0];		// exe file name always passes thru
	int argci = 1;			// input arg counter
	int argcx = 1;			// output arg counter
	do				// call cse2 at least once: let cse2 issue message or usage info if no file given
	{
		for ( ; argci < argc; )		// copy args til find a filename or end of args
		{
			const char* a = argv[argci++];		// get one arg pointer
			if (IsBlank( a))
				continue;		// ignore blank or NULL (unexpected) arguments
			argvx[argcx++] = a;			// use this arg in this run
			if (*a != '-'  &&  *a != '/')		// if it was not a flag (not - nor / first), assume it is an input filename
			{
				BOO moreFilesFollow = FALSE;
				for (int argcj = argci;  argcj < argc; )	// scan remaining args to see if more filenames follow
				{
					const char* b = argv[argcj++];		// get one additional arg pointer
					if (IsBlank( b))
						continue;			// ignore blank or NULL (unexpected) arguments
					if (*b != '-'  &&  *b != '/')	// if not a flag
						moreFilesFollow++;			// assume it is a file name
				}
				if (moreFilesFollow) 	// if additional file args follow
					break;				// terminate arg list for run with file name
				// else continue, thus including any trailing flags with last file
			}
		}
		argvx[argcx] = 0L;			// terminate arg list for this run. no ++ : overwrites later.

		int thisErrLvl = cse2( argcx, argvx);	// call next level to do run.
												// Returns 0 ok, 255 ^C, other non-0 (eg 2) for other error. */

		errlvl |= thisErrLvl;			// merge error levels for return to caller; 0 ok.
       									// since files presumed independent, errors other than ^C continue to next file

		if (thisErrLvl==255)			// if interrupted from keyboard (^C pressed)
			break;						// stop, do not continue to other files. 1-95.

		argcx--;						// drop the last arg used this run:
										// overwrite used filename with next arg if not end of args.
	}
	while (argci < argc);		// repeat til all input args used

	Pumbra::penumbraTerminate();	// Clean up GPU calculation memory
	return errlvl;			// 0 ok, nz error, 255 if ^C, 1-95.
}			// cse1run
//------------------------------------------------------------------------
LOCAL INT cse2( INT argc, const char* argv[])

// This function level calls cse3 (below) and handles exceptions.
// called by cse1(), above

// Args and return as for cse(); only one input file name accepted in argument list (argv).
{
	BOO cleanDone = FALSE;

	// set up envpak: initializes re standard exit fcn "byebye" which is used for
	// fatal error exits in msg routines
	// Also inits arithmetec error handling and 0:0 and DS:0 monitoring.

	hello();
	int errlvl = 0;
	try
	{	// call next level
		errlvl = cse3( argc, argv);	// CSE inner fcn, below.
	}
	catch (int exitCode)
	{
		errlvl = exitCode;
	}
	cnClean( DONE);			// clean up after successful or non-ABT error completion.

	// local fcn, calls clean fcns elsewhere.
	cleanDone = TRUE;		// say cnClean completed

	if (!cleanDone)		// unless normal return and cleanDone(DONE) completed just above
		cnClean( CRASH);		// do ABT-exit cleanup. fcn below.
	//cleanDone = FALSE;   	// redundant since flag is internal

	return errlvl;			// 0 ok, nz error, 255 if ^C 1-95.
}			// cse2
//=====================================================================================================================
//--- command line switch variables, cse3 to tp_SetOptions, 12-94
#ifdef BINRES	// cnglob.h
// flags for binary results files command line switches, used in tp_TopOptions after input for each run decoded.
// Can't set Top while decoding cmd line cuz Top is defaulted and filled after command line decoded,
// and switches apply to multiple runs even if CLEAR in input file.
LOCAL BOO brs = FALSE;		// TRUE for basic binary results file (non-hourly info)
LOCAL BOO brHrly = FALSE;	// TRUE for hourly binary results file
LOCAL BOO brMem = FALSE;   	// TRUE to return binary results (per brs, brHrly) in memory blocks not files
LOCAL BOO brDiscardable = FALSE; 	// TRUE to return bin res in discardable memory blocks as well as files
#endif
static char* _repTestPfx = NULL;		// from -x: testing prefix for some report data (hides e.g. rundatetime)
//---------------------------------------------------------------------------------------------------------------------
LOCAL INT cse3( INT argc, const char* argv[])

// CSE primary function: inits, decodes input, does runs & reports, terminates.

// Args and return value as for cse(), with ONE input file name argument only.
// cneHInstApp and cneHPar globals already set.

{

// Sign on
	screen( 	 				// remark to screen, rmkerr.cpp.
		0,						//    0: no options --> start with newline.
		"%s %s %s\n", ProgName, ProgVersion, ProgVariant );	//   message and printf-style args

// Initialize the program

	INT errorlevel = 0;	// value to return to caller. 0 says all ok, 255 indicates ^C'd, other nz is error.
						//  Caller may return it as errorlevel.

	clearErrCount();	// clear the error count incremented by err, etc, etc, calls. rmkerr.cpp. rob 5-97.
						// Note clearErrCount may also be called from cul.h at CLEAR command.

	// get exe or dll path -- its directory will be searched for msg, weather, etc files
	exePath = strsave( strpathparts( argv[0], STRPPDRIVE|STRPPDIR));	/* from .exe pathName, extract path.
									   strsave Tmpstr value for ppAddPaths call way below.
    									   (strpak.cpp fcns, rundata.cpp vbl.) */
#ifdef WINorDLL
	// get module path: may different from calling .exe for DLL (1-18-94)
	// or for Windows version in use as DDE server (even though couldn't prove need 8-22-95).
	char modPathBuf[ _MAX_PATH];
#if 0 && defined( DLL)		// DLL TODO
	GetModuleFileName( hInstLib, 					// DLL hInst (cnedll.cpp, set by LibMain)
					   modPathBuf, sizeof(modPathBuf) );		// .. for DLL, cneHInstApp is caller's not DLL's hInst.
#else
	GetModuleFileName( cneHInstApp,					// DDE server or subr caller's hInst (cse.cpp global)
					   modPathBuf, sizeof(modPathBuf) );
#endif
	char * tems = strpathparts( modPathBuf, STRPPDRIVE|STRPPDIR);	// extract path from pathName to Tmpstr[] (strpak.cpp)
	if (strcmpi( tems, exePath))					// if different from exe path
	{
		tems = strtcat( tems, ";", exePath, NULL);			// assemble path. caller's .exe path after .dll/.exe path.
		dmfree( DMPP( exePath));  						// free prior .exePath
		exePath = strsave(tems);						// replace with new exePath
	}
#endif

	// init re error message texts
	msgInit(PWRN);			/* check message table, messages.cpp. (autoinits if called sooner (as by constructor);
    					   call now to be sure of getting all warnings at program start as devel aid). */

	enkiinit(KICLEAR+KIBEEP);		/* init keyboard interrupt stuff, envpak.cpp.  (if omitted, enkichk()
    					   I think (and printf anyway) aborts on ^C -- so dummy enkichk.) */

	cnClean(ENTRY);		// insurance-initialize everything in case left dirty on prior call to cse package, 10-93.
						// local fcn, below, may call fcns in other packages.
						// corresponding exit call is done by caller to allow multiple returns here.

	doControlFP();

	setbuf(stdout, NULL);	// disable buffering on stdout, 6/12/2014


	/*----- Command line -----*/

// check # command line args; conditionally display help and exit
	if ( argc < 2			// if too few args: min = progName + inputFile
			|| strchr( argv[1],'?') )		// or user wants help -- CSE ? or CSE -?
	{
		screen( 0, 						// display msg (rmkerr.cpp)
				(char *)MH_C0001, 				// msg text explains command line, msgtbl.cpp.
				strlwr( strpathparts( argv[0], STRPPFNAME)) );	// lower case exe file name in Tmpstr, to insert at %s, 2-95

		byebye( errorlevel);		// envpak.cpp: clean up and and return to subr pkg caller,
		// as set up by envpak:hello call in cse2() above.
		return errorlevel;		// insurance: return not expected
	}

// display command line -- eg to confirm flags with multiple input files

	screen( NONL, "Command line:");			// note signon ended with \n
	int i;
	const char* tArgs = "";		// assemble args in tmpstr
	for (i = 1;  i < argc;  i++)
		tArgs = scWrapIf(
					tArgs,
					strtcat(" ", argv[ i], NULL),	// not strtprintf! args may contain '%'
					"\n             ");
	cmdLineArgs = strsave( tArgs);	// copy to dm
	screenNF( cmdLineArgs, NONL);	// display w/o further formatting

// decode command line

	int probeNamesFlag = 0;	// non-0 to display probeable record/member names (-p or -q on command line)
	int allProbeNames = 0;	// non-0 to display ALL probe names (-q on command line)
	BOO warnNoScrn = 0;		// non-0 to suppress display of warnings on screen (-n)
	int culDocFlag = 0;		// non-0 to display entire input tree (-c)

#ifdef BINRES
	// init flags above for binary results files command line switches. Used after each run input decoded -- see tp_SetOptions.
	brs = FALSE;			// TRUE for basic binary results file (non-hourly info)
	brHrly = FALSE;			// TRUE for hourly binary results file
	brMem = FALSE;  		// TRUE to return binary results (per brs, brHrly) in memory blocks not files
	brDiscardable = FALSE; 	// TRUE to return bin res in discardable memory blocks as well as files
#endif
	InputFileName = NULL;	// input file name: none yet. public variable.
	char inputFileNameBuf[ _MAX_PATH];		// input file name put here
											//   InputFileName pts here when file name is known
	RC rc = RCOK;			// return code from many fcns. init to "ok".
	for (i = 1; i < argc; i++) 	// loop over cmd line args
	{
		char c, c0;
		const char* arg = argv[i];
		RC trc;
		if (ppClargIf( arg, &trc) )	// test if a preprocessor cmd line arg such as a define or include path.
       								// If so, do it, set trc, return true.  pp.cpp.
       								// uses -d, -D, -i, -I as of 2-95
			rc |= trc;			// merge cmd line arg error codes
		else if ((c0 = *arg) != 0  &&  strchr("-/", *arg))	// test for switch
		{
			// warning for switch after file name, cuz confusing: apply to preceding file only if last file 2-95. Use switch anyway.
			if (InputFileName)
				warn( (char *)MH_C0008,  // "C0008: \"%s\":\n    it is clearer to always place all switches before file name."
					  arg );

			switch (c = tolower(*(arg + 1)))		// switch switch
			{
			case 'b':    					// -b: batch: do not stop for errors once err file open
				setBatchMode(TRUE);
				break;

			case 'q':
				allProbeNames++;			// -q: display ALL member names
				/*lint -e616  case falls in */
			case 'p':
				probeNamesFlag++;  			// -p: display user probeable member names
				break; /*lint+e 616 */

			case 'c':				// -c: list all input names (walks cul tree)
				culDocFlag++;
				break;

			case 'n':				// -n: no warnings on screen
				warnNoScrn++;
				break;

#ifdef BINRES	// cnglob.h. rob 11-93
			case 'r':
				brs++;
				break;				// -r: generate basic binary results file (non-hrly info)

			case 'h':
				brHrly++;
				break;				// -h: generate hourly binary results file

#ifdef WINorDLL
			case 'm':
				if (!hans)			// -m: output bin res's to memory only, return handles
					goto noHans;	// don't do it if not returning memory handles
				else
					brMem++;		// copied to Top.brMem for each run
				break;

			case 'z':
				if (!hans)			// -z: output bin res's to memory as well as file
noHans:
					rc |= err( (char *)MH_C0009, c);	/* "Can't use switch -%c unless calling program\n"
								   "    gives \"hans\" argument for returning memory handles" */
				else
					brDiscardable++;
				break;
#else
			case 'm':
			case 'z':
				rc |= err( (char *)MH_C0010, c); 	// "Switch -%c not available in DOS version"
				break;
#endif
#endif
			case 'x':
				_repTestPfx = strsave( arg + 2);
				break;

			case 't':
				// TestOptions: testing-related bits
				if (strDecode( arg+2, TestOptions) != 1)
					rc |= err( "Invalid -t option '%s' (s/b integer)", arg+2);
				break;

			case '\0':
				rc |= err(  				// issue error msg, continue for now
						  (char *)MH_C0005, c0);  	//    "Switch letter required after '%c'"
				break;

			default:
				rc |= err(  				// msg, continue for now. rmkerr.cpp
						  (char *)MH_C0006, c0, c);	//    "Unrecognized switch '%c%c'"
			}
		}
		else if (InputFileName==NULL)	// else if no input file name yet
			InputFileName = strcpy( inputFileNameBuf, arg);   	// take this arg as input file name
		else						// dunno what to do with it. Should now be impossible -- see cse1() level 2-95.
			err( ABT,				// issue msg, await keypress, abort program; does not return. rmkerr.cpp.
				 (char *)MH_C0002,  	//    "Too many non-switch arguments on command line: %s ..."
				 arg );
	}

	if ( !InputFileName					// if no filename given
	 &&  !probeNamesFlag && !culDocFlag)	// nor anything else to do requested
		err( ABT,			// issue msg, keypress, abort; rmkerr.cpp
			 (char *)MH_C0003);  	//    "No input file name given on command line"

	if (rc)				// if error(s) in cmd line args, exit now.  ppClargIf or code above issued specific msgs.
		err( ABT,			// issue msg, keypress, abort; rmkerr.cpp
			 (char *)MH_C0004);		//    "Command line error(s) prevent execution"


	/*----- do special switches, exit if no input file to drive runs -----*/

// display probeable member names if requested (-p or -q on command line); do necessary init first.

	cnPreInit();				// preliminary input data init needed by showProbeNames (cncult.cpp)
	cgPreInit();				// preliminary simulator data init needed by showProbeNames (cnguts.cpp)
	if (cul( 0, NULL, "", cnTopCult, &Topi ))	// prelim cul.cpp:cul init: set things needed by showProbeNames
		err( PABT, 			 				// if error, fatal program error message (rmkerr.cpp). no return.
			 (char *)MH_C0007 );			//    "Unexpected cul() preliminary initialization error"

	if (probeNamesFlag)
		showProbeNames( allProbeNames);  		// do it, cuprobe.cpp

	if (culDocFlag)
		culShowDoc();

// exit if no input file (gets to here only if -p or other no-input-file switch given)

	if (!InputFileName)
	{
		byebye( errorlevel);		// envpak.cpp: clean up and and return to subr pkg caller,
		// as set up by envpak:hello call in cse2() above (formerly ret'd directly to DOS).
		return errorlevel;		// insurance: return not expected
	}


	/*----- Initialize input and outputs -----*/

// input file syntax check

	/* if syntax bad, exit now to avoid additional confusing errors creating error file and report file, 2-95.
	   But continue for ok-syntax non-existent file (in valid direcotry -- checked below),
	     so .err and .rep files are created & receive error message.
	   Note: for most thorough check, restore xiopak.cpp:xfScanFileName(). */

	if (strcspn( InputFileName, "?*") < strlen(InputFileName))		// if an * or ? in pathname
		err( ABT, (char *)MH_C0011, InputFileName );	// "\"%s\": \n    input file name cannot contain wild cards '?' and '*'."

// default input extension -- before path search

	char infPathBuf[ _MAX_PATH];				// receives full path if found

	// file find strategy
	//    long file names may contain "extra" extensions (e.g. x.v2.cse)
	//    search order
	//        cmd line = "cx.v2.cse"
	//           cx.v2.cse.cse, x.vw.cse.inp, x.v2.cse (bingo)
	//		  cmd line = "cx.v2"
	//           cx.v2.cse (bingo)
	//    check w/o makes it OK to have a file "cx.cse" and a folder "cx".
	//    double extensions deemed rare, so checking cx.cse.cse is not risky
	const char* dfltExtList[] = { ".cse", ".inp", "", NULL };
	const char* testExt = NULL;		// extension being checked (used below)
	int iExt;
	for (iExt=0; dfltExtList[ iExt]; iExt++)
	{	testExt = dfltExtList[ iExt];
		const char* tinf = IsBlank( testExt)
			? InputFileName
			: strffix2( InputFileName, testExt, 1);		// unconditionally add extension

		// find the input file now so full actual path available for use for output files

		// path search order is same as for #include files; we don't yet ppAddPath all paths and use ppFindFile
		//   cuz want to add input file path next after -i paths (added above)

		if (!ppFindFile( tinf, infPathBuf))				// search . and paths given on cmd line with -i (pp.cpp) / if not found
			if (!findFile( tinf, NULL, infPathBuf))		// search (. again and) DOS PATH / if not found
				if (!findFile( tinf, exePath, infPathBuf))	// search exe and dll file directories / if not found
				{
					// if a non-existent directory given, exit now to avoid errors creating .err file and .rep file
					//   abort even if iExt==0, no point looking further if path bad
					xfGetPath( &tinf, ABT);			// get full path (in Tmpstr, replaces tinf), checking dirs,
													//   else issue message and abort. xiopak.cpp.
					strcpy( infPathBuf, tinf);		// use name given, with path found by xfGetPath
             										//   (path in errMsg helps user see if he had wrong curr dir)
					continue;		// try next extension or exit loop with infPathBuf set to tinf
				}
		break;		// here means a findFile succeeded
	}
	// global InputFileName remains pointing to name as entered by user, for use eg in report footers.
	InputFilePath = infPathBuf;			// set global ptr to full input pathname in stack bfr
	InputDirPath = strsave( strpathparts( infPathBuf, STRPPDRIVE|STRPPDIR ) );	// save input drive/dir path

	// if file found has has recognized extension, drop it
	//   (could probably be done in above loop, but safer here)
	int bKeepExt = TRUE;
	for (iExt=0; bKeepExt && dfltExtList[ iExt]; iExt++)
	{	const char* tExt = strpathparts( infPathBuf, STRPPEXT);
		if (stricmp( tExt, dfltExtList[ iExt])==0)
			bKeepExt = FALSE;	// recognized extension, drop it
	}
	InputFilePathNoExt =			// save input file full pathname with no extension
			strsave( bKeepExt
				? infPathBuf
				: strpathparts( infPathBuf, ~STRPPEXT ));

	// Initialize error message file.  Messages are written directly to file, not thru virtual report, to capture early & late
	//   msgs in session, and messages during unspooling, eg from vrpak.
	//   Omits command line error messages issued above.
	//   Note: *after* InputFilePathNoExt is known
	errFileOpen( ErrFilePath());		// open error message file, rmkerr.cpp

#if defined( DEBUGDUMP)
	DbFileOpen( strffix2( InputFilePathNoExt, ".dbg", 1));	// debug print file (see DbPrintf() etc.)
#else
	DbFileOpen( NULL);									// delete any pre-existing dbg file (insurance)
#endif

	setWarnNoScrn( warnNoScrn);				// suppress on-screen warnings if cmd line -n given. rmkerr.cpp. rob 5-97.

// set up search paths for input and #include files, 2-95. Also used for weather file, 3-95.
	//ppAddPath done from ppClargIf call above for -i paths	   first search user-specified paths
	ppAddPath( InputDirPath);				// then input file path
	ppAddPath( NULL);						// then DOS environment PATH paths
	ppAddPath( exePath);					// finally, look in .exe/.dll directorie(s)

#if 0	// introduce, when/if desired to control spool file (user path command so can go on ram disk?
*	// hmm.. executed here each run; need only to do once --> move to cnguts.cpp?
*
* /* init "virtual report" spooling system:  Receives monthly/hourly results report/export text &c
* 					   for later sorting/unspooling to report/export output files. */
*    if (vrInit( spoolFilePathName))		// set vr spool file path and init vr's, vrpak.cpp
*       ; // failure - probably should abort session.
#endif

	Top.tp_SetOptions();		// set some Top members (exePath etc)
								//   else not set when input error suppresses run

	/* preset primary report file info, to rcv most errmsgs (unspooled below) if an input error before cncult.cpp sets it.
									   (if no early input error, cncult.cpp redoes with
									   primary report file name & options given by user) */
	PriRep.f.fName = strsave( strffix2( InputFilePathNoExt, ".rep", 1) );	// PriRep: cnguts.cpp. filename: .REP; private dm copy.
	PriRep.f.optn = VR_OVERWRITE | VR_FMT;			// overwrite for now (user wd miss new msgs at end if appended)
	// page formatting on, the default. user may turn off.
	// rest of PriRep is data-init 0.

	// open Virtual report for error messages (ERR report).  While open, spools err msgs for report file.
	// Omits msgs when not open, as during unspooling.
	// Note ALL error messages go to screen, and to error file except b4 opened / after closed.
	openErrVr();				// fcn in rmkerr.cpp; sets public VrErr.
	// Note: unspool request(s) for ERR report are generated in cncult.cpp unless
	//   user input suppresses default.  But opened unconditionally as ERR report
	//   can rcv text b4 cncult acts on user input. */

// Set up timers
	tmrInit( "Input", TMR_INPUT);
	tmrInit( "AutoSizing", TMR_AUSZ);
	tmrInit( "Simulation", TMR_RUN);
	tmrInit( "Reports", TMR_REP);
	tmrInit( "Total", TMR_TOTAL);
	tmrStart( TMR_TOTAL);			// start the "total" timer
#if defined( DETAILED_TIMING)
	tmrInit( "AirNet", TMR_AIRNET);
	tmrInit( "AWTot", TMR_AWTOT);
	tmrInit( "AWCalc", TMR_AWCALC);
	tmrInit( "Cond", TMR_COND);
	tmrInit( "BC", TMR_BC);
	tmrInit( "Zone", TMR_ZONE);
#endif

	/*----- loop over runs specified in input file -----*/

	int rv;				// cul() ret val: 1 error, 2 go & recall, 3 go & done.
	int recall = 1;		// 2 when re-calling cul()
	do					// repeated for additional runs in input file
	{
		// init the houly simulator
		cgInit();						// b4-run-input hourly sim init, cnguts.cpp: sets cnRunSerial.
		screen( QUIETIF, "  %s run %03d:", InputFilePath, cnRunSerial );	// no NONL: starts new sceen line
		if (getScreenCol() > 60)					// start new line after very long pathname
			screen( QUIETIF, "   "); 						// newline (no NONL) and indent (+1 space in each text)
		screen( NONL|QUIETIF, " Input");						// continue screen line

		//--- decode the input file and set up run records: call cul.cpp

		tmrStart(TMR_INPUT);

		// open "virtual report" to receive input listing (INP report)
		openInpVr();			/* fcn in pp.cpp; sets VrInp.  Note: opened uncond'ly: is written b4
			  	           open-if-req'd mechanism (cncult.cpp) works.  However, cncult.cpp does generate
					   unspool request(s) only if report req'd (or default not overwridden) */
		// expression reinit
		rc |= exClrExUses(FALSE);	// clear uses, chaf's, value of any exprs left from any prior run. exman.cpp. 5-31-92.

		// use cul() to compile from the input file.
		/* compiles input file to input basAncs; at RUN command, calls cncult2:topCkf which checks input,
		   determines initial $autoSizing, evaluates "phasely" expressions, and sets up run records from input.
		   calls tp_SetOptions (below) to apply cmd line flags to Top. */
		rv = 				// 1 error, done, 2 go, then recall, 3 go, then done.
			cul(
				recall, 		// 1 1st call (open the file), 2 recall
				InputFilePath,	// input full path, as found above, with defaulted extension
				"",     		// input default extension: done before path search above
				cnTopCult,		// CUL Table for CSE, in cncult.cpp.
				&Topi,   		// (static) RAT entry to rcv top data
				&Topi.bAutoSizeCmd );	// flag to set if input contains AUTOSIZE commands
		recall = 2;			// set for next call, if any
		if (rv==1)			// if cul detected input error
			rc = RCBAD;			// set our error code. sets errorlevel to 2 below.

#if 0
0 input records are modified in e.g. RSYS autosize
0 keep input records to avoid pointer checking
0 also, memory savings no longer of concern.  5-20-2022
0		// conditionally delete input records now to make max memory available
0		if (rv != 2)					// not if not end input file: addl statements may ADD to present input.
0			if ( !(Top.chAutoSize==C_NOYESCH_YES     // not if both autosizing and main-simulating requested,
0			 &&  Top.chSimulate==C_NOYESCH_YES ) )	 //   cuz input needed to re-setup for main sim run
0				cul( 4, NULL, NULL, NULL, NULL);      	// cul.cpp. Does not clear 'probed' basAncs. Duplicate call ok.
#endif

		// find and register expression uses in basAnc records
		if (rc==RCOK)			// if no error yet
			rc = exWalkRecs();		// search for exprs and register, msg UNSETS in all basAnc records. exman.cpp.
		tmrStop( TMR_INPUT);


		//--- do the run
		if (Top.chAutoSize==C_NOYESCH_YES  &&  rc==RCOK)	// if autosizing phase requested by input (as validated) and no error
		{
			// autosizing phase

			tmrStart (TMR_AUSZ);
			rc = cgAusz();			// do autosizing, cnausz.cpp. only call. returns MH_C0100 for ^C.
			if (errCount())  		// if any error messages issued
				rc = RCBAD;			// set our error flag: insurance
			tmrStop( TMR_AUSZ);
			if (Top.chSimulate==C_NOYESCH_NO  &&  rc==RCOK)	// if main simulation phase NOT requested, and no error nor ^C
			{
				// if no main simulation requested, virtual print autosizing reports.
				// afterthought 5-97.
				// Maybe ausz rpTy's should always be (have been) done here & omitted from main sim vpRxports call.
				// Experimental code, watch for bugs/differences from reports with main sim.

				Top.jDay = Top.tp_endDay;	// set date to a day in run, for cgresult:cgReportsDaySetup.
				cgReportsDaySetup();		// make lists Top.dvriY etc of active reports for vpRxports. Uses Top.jDay.
				vpRxports( C_IVLCH_Y,		// virtual print annual reports/exports. Uses Top.dvriY. cgresult.cpp
						   TRUE );				// say do only autosize-results-related reports (new param added 5-97)
			}
			if (Top.chSimulate==C_NOYESCH_YES  &&  rc==RCOK)	// if main simulation phase also requested, and no error nor ^C
			{
				// between phases when both done, repeat input check/setup to reinit and handle $autoSizing dependencies.
				if (Top.verbose > 0)		// if autoSize displayed any detailed screen messages
					screen( 0, "   ");		// start new screen line new line (no NONL) and indent (+1 space in each text)
				runFazDataFree();   		// delete "phasely" run basAnc records. cnguts.cpp. 6-95.
				screen( NONL|QUIETIF, " Re-setup");	// (?)
				tmrStart(TMR_INPUT);
				rc = exClrExUses(TRUE);		/* clear uses & chaf's of expressions except those uses & chafs
											   in basAncs not re-set-up by topReCkf: DvriB, RcolB, XcolB 10-95. exman.cpp. */
				if (rc==RCOK)
					rc = topReCkf();   		/* re-check input data and re-setup run basAnc records, cncult2.cpp.
											   First updates Top.tp_autoSizing and re-evaluates "phasely" expressions.
											   Similar to original topCkf call from cul.cpp:culRun(). */

				if (errCount())  			// if any error messages issued
					rc = RCBAD;			// set our error flag. believe redundant.
				if (rc)  							// if any errors
					pInfo( "No main simulation due to error(s) above.");	// NUMS

				if (rv != 2)				// not if cul() said above that there are more stmts in input file
					cul( 4, NULL, NULL, NULL, NULL);	// delete input data now to make the most memory available. cul.cpp.
				if (rc==RCOK)   			// if ok, find and register expression uses in re-set-up run basAnc records
					rc = exWalkRecs();   	// search for exprs and register, msg UNSETS in all basAnc records. exman.cpp.
				tmrStop( TMR_INPUT);
			}
		}

		if (Top.chSimulate==C_NOYESCH_YES  &&  rc==RCOK)	// if main simulation phase requested by input and no error nor ^C
		{
			// main simulation phase
			tmrStart( TMR_RUN);
			rc = Top.tp_MainSim();			// main simulation run, cnguts.cpp. only call.
											// returns MH_C0100 for ^C.
			tmrStop(TMR_RUN);
		}
		if (rc==MH_C0100)			// if keyboard interrupt (^C pressed)
			errorlevel = 255;		// return special errorlevel
		else if (rc != RCOK) 		// if other error during input or run
			errorlevel = 2;			// for return to caller, thence possibly to OS. NZ ends loop.
		// Note saving errorlevel 1 for alternate good returns.

		// reports: generate ZDD reports (zone data dump - input data verification) for any zones requesting them
		ZNR* zp;
		RLUP( ZrB, zp)				// loop over good zones (ancrec.h macro)
		{
			int vrh = zp->i.vrZdd;
			if (vrh)   				// if zone requests a zdd report
			{
				cgzndump( vrh, zp->ss);		// do it (cgdebug.cpp)
				if (vrGetLine(vrh))   		// unless now on line 0 of page (vrpak.cpp)
					vrStrF( vrh, 1, "\f");		// end page (if page-formatted report file): each zone on own page. vrpak.cpp.
			}
		}

		// reports: Log report (contains only this + maybe debug logging via DbPrintf etc)
		const char* pfx = Top.tp_RepTestPfx();
		logit( NOSCRN, "%s%s %s %s\n", 				// remark to log file only, rmkerr.cpp.
			   pfx, ProgName, ProgVersion, ProgVariant);	// ProgName, -Vers, -Variant: cse.cpp.

		// close down hourly simulator
		runFazDataFree();   	// delete "phasely" run basAnc records. cnguts.cpp. 6-95.
		cgDone();				// hourly simulator cleanup after all phases done, cnguts.cpp

		// reports: "Unspool" virtual reports from this run into report/export output files
		screen( NONL|QUIETIF, " Reports\n");	// progress indicator. follows last month name on screen (if no errMsgs).
												//  Is final message, so end with newline.
		if (UnspoolInfo)	// if UnspoolInfo got set up (in cncult.cpp).
       						// If not (early input error), ermsgs will be unspooled below via PriRep info.
		{
			tmrStart(TMR_REP);
			zStaticVrhs();	/* close err and log vr's, and
							clear static vr handles to prevent spool during unspool, below.
							note: errMsgs during unspool go to .ERR file but not to ERR rpt
							note: vrh's in RATs: cgDone just above cleared most basAncs. */
			vrUnspool(UnspoolInfo);    		// do unspool, using pointer to block set up in cncult.cpp.
			dmfree( DMPP( UnspoolInfo));		// free VROUTINFO dm block built in cncult.cpp
			// note: vrUnspool() has dmfree'd the filenames within this block.
			vrClear();				// clear virtual report system: free all handles, empty spool file. vrpak.cpp.
			// note: if there is no vrInit call, vrpak self-initializes.
			openErrVr();				// promptly open new Virtual report for further error messages.  sets VrErr.

			/* note: any error messages (possible) or log msgs (unexpected) during unspool do not get into report files.
			   To handle: make vrpak handle spool during unspool (write to end spool file if not in ram),
			              open new VrErr before Unspool, if not empty, do addl unSpool before vrClear here.
			              judged not worth the code & complexity for now.
			   expected error msgs are internal errors, and i/o errors (inf lup danger if logged).  11-20-91. */
			tmrStop(TMR_REP);
		}

	}
	while ( rv==2			// run loop ends if cul() said done (rv==1,3)
			&& errorlevel==0 );	// or if input error, or if simulator err (including ^C)


	/*----- Runs complete.  Finish & Close down. -----*/

	DMHEAPCHK( "cse3 finish" )

	/* Free all input data (cul has its own RATs, and much DM).  Do now b4 exit to help find unfree'd dm -- else does not matter.
								     Also may be called above, before final run, in non-error cases. */
	cul( 4, NULL, NULL, NULL, NULL);	// cul.cpp
	extClr();				// clear expression tbl (to free DM), exman.cpp. cannot be done before RUN!

// Format overall timing info and other final notes to virtual report. Includes date, time, program name, version.

	tmrStop(TMR_TOTAL);
	int vrTimes = 0;
	if (vrOpen( &vrTimes, "Timing info for log", VR_FMT)==RCOK)		// open virtual report / if ok. vrpak.cpp.
	{
		const char* pfx = Top.tp_RepTestPfx();	// optional test prefix (allows hiding e.g. date/time from text compare)
		IDATETIME idt;
		ensystd( &idt);								// ret system date & time, envpak.cpp
		vrPrintf( vrTimes, "\n\n%s%s %s %s run(s) done: %s",
			pfx, ProgName, ProgVersion, ProgVariant, tddtis( &idt, NULL) );
		vrPrintf (vrTimes, "\n\n%sExecutable:   %s\n%s              %s  (HPWH %s)",
			pfx, Top.tp_exePath, pfx, Top.tp_exeInfo, Top.tp_HPWHVersion);
		vrPrintf( vrTimes, "\n%sCommand line:%s", pfx, Top.tp_cmdLineArgs);
		vrPrintf( vrTimes, "\n%sInput file:   %s",
			pfx, InputFilePath ? InputFilePath : "NULL");
		vrPrintf( vrTimes, "\n%sReport file:  %s",
			pfx, PriRep.f.fName ? PriRep.f.fName : "NULL");
		vrPrintf( vrTimes, "\n\n%sTiming info --\n\n", pfx);
		for (i = 0;  i <= TMR_TOTAL;  i++)
			vrTmrDisp( vrTimes, i, pfx);		// timer.cpp
	}

// Output timing info & any remaining log/err msgs to report file -- in particular, input error that prevented unspool above.

	if (PriRep.f.fName )						// insurance
	{
		int* vrhs;
		/* File name, overwrite optn, etc in PriRep (VROUTINFO5 in cnguts.cpp) have been preset and updated
		   by code above, in cncult.cpp, and in vrpak.cpp. Add vrh's to unspool: nz ones only, 0's terminate. */
		vrhs = PriRep.f.vrhs;
		if (VrErr)    *vrhs++ = VrErr;	// any remaining error messages, especially early fatal input errors.
		if (VrLog)    *vrhs++ = VrLog;	// any further log messages (unexpected)
		if (VrInp)    *vrhs++ = VrInp;	// any input listing not already unspooled
		if (vrTimes)  *vrhs++ = vrTimes;	// timing info spooled just above
		// 2 more slots available
		*vrhs++ = 0;			// terminate variable vrh[] with a 0 in case (future) previously used
		*(char **)vrhs = NULL;	// terminate PriRep -- say not another VROUTINFO following.
		zStaticVrhs();			// close/clear vr handles to insure against spool during unspool
		vrUnspool( &PriRep.f);   	// unspool virtual reports to primary report file. append or overwrite. vrpak2.cpp.
	}

// close down the "virtual report" system -- delete its memory tables and spool file
	zStaticVrhs();				// clear vr handles to insure against spool after terminated
	vrTerminate();				// vrpak.cpp. 11-91.

// free some more dynamic memory (dm) -- helps find unfree'd dm memory-wasting bugs
	freeHdrFtr();	// free externally-inaccessible dm blocks in cncult.cpp.
	//   Do not call before report generation complete.
// close error message file
	setWarnNoScrn(FALSE);			// restore warning display on screen, if it was disabled with -n. 5-97.
	errFileOpen( NULL);				// erases it if empty, else (WINorDLL) calls saveAnOutPNam in this file.
	//    note rmkerr:errClean is called last, from cse().
	DbFileOpen( NULL);				// ditto debug log file

// exit
#if 0	// sound not currently supported, 4-12
x	if ( rv==3 					// if no cncult input error
x			&& rc==RCOK )		// if no error here
x		enbeep( BEEPDONE);		// good completion beep
#endif
#if 0 // tired of looking at this 1-95... Sometime restore and chase down why Dmused is reported as 20K+ at exit.
*  #ifdef DEBUG
*    screen( 0, "  Dmused at exit: %ld  DmMax: %ld\n", Dmused, DmMax);	// report final Dmused so dmCreep may be noticed
*  #endif
#endif
#if 0			// how to do conditional final newline if necessary, 1-95, unrun.
*    screen( 0, "");				// final newline if necessary. rmkerr.cpp.
#endif
	dmfree( DMPP( _repTestPfx));
	InputFilePath = NULL;		// insurance: clear global that points into local stack storage 2-95
	return errorlevel;			// on return, caller should byebye() for the cleanup it does.
	// other exits above (no input file case; usage help request case)
}			// cse3
//-----------------------------------------------------------------------------------------------------------
void TOPRAT::tp_SetOptions()	// apply command line options etc. to Top record

// adds command line options into top record Topi b4 checks.
{
// set Top members per command line switches, as saved above by cse3 in static flags
#ifdef BINRES	// cnglob.h. rob 11-93
	if (brs)
		tp_brs = C_NOYESCH_YES;    	// basic results file no-yes choice (also input-file-inputtable)
	if (brHrly)
		tp_brHrly = C_NOYESCH_YES;	// hourly results file ..
	tp_brMem = brMem;				// memory not file br output Boolean
	tp_brDiscardable = brDiscardable;		// discardable memory + file br output
#endif
	if (_repTestPfx)		// if command line prefix specified
	{
		// overwrite prefix from input (if any)
		strsave( tp_repTestPfx, _repTestPfx);
	}
	strsave( tp_progVersion, ::ProgVersion);	// program version text (for probes)

	WStr tExePath = enExePath();
	strsave( tp_exePath, tExePath.c_str());

	strsave( tp_HPWHVersion, DHWHEATER::wh_GetHPWHVersion().c_str());

	strsave( tp_exeInfo, enExeInfo( tExePath, tp_exeCodeSize).c_str());

	strsave( tp_cmdLineArgs, cmdLineArgs);

	setScreenQuiet( verbose == -1);		// per user input, set rmkerr.cpp flag
										//   suppresses non-error screen messages
										//   e.g. progress ("Input", "Warmup", ...)
										//   re multi-run linkage to CBECC-Res
}	// TOPRAT::tp_SetOptions
//-----------------------------------------------------------------------------
const char* TOPRAT::tp_RepTestPfx() const
{	return tp_repTestPfx ? tp_repTestPfx : "";
}		// TOPRAT::tp_RepTestPfx
//-----------------------------------------------------------------------------
RC TOPRAT::tp_CheckOutputFilePath(		// check output file name
	const char* filePath,
	const char** pMsg /*=NULL*/)	// optionally returned: explanatory msg
									//  (may be TmpStr, treat as temporary)
// returns RCOK if filePath is legit for user-specified output file (REPORTFILE, EXPORTFILE, )
//    else RCBAD
{
	const char* msg = NULL;
	if (!strcmpi( filePath, InputFilePath))
		msg = "Cannot overwrite current input file";
	else if (!strcmpi( filePath, ErrFilePath()))
		msg = "Cannot overwrite current error file";
	if (msg)
	{	if (pMsg)
			*pMsg = msg;
		return RCBAD;
	}
	return RCOK;
}		// tp_CheckOutputFileName
//===========================================================================================================
LOCAL void zStaticVrhs()	// zero static vrh's

// zero virtual report handles, especially VrErr, to deactivate vr output code while unspooling or after vrpak terminated.

// also terminates certain reports if open: error, log, input.

// this is the official and only closing for error and log reports, 11-25-91.

// caller must open a VrErr again as soon as possible to capture as many error msgs as possible for ERR report
{
	closeLogVr();					// terminate log virtual report if open, and 0 VrLog, rmkerr.cpp
	closeErrVr();					// terminate err virtual report if open, and 0 VrErr, rmkerr.cpp
	closeInpVr();					// zero VrInp, pp.cpp.

	Top.vrSum = 0;					// extra insurance -- Top is not cleared.

	// vrh's in records of basAnc's are not 0'd; most basAncs were cleared after run.

}	// zStaticVrhs
//------------------------------------------------------------------------
#ifdef OUTPNAMS		// defined in cnglob.h to return file names to caller
// possible strpak, etc fcn, now used only in saveAnOutPNam (next), 10-8-93:
char* strDmAppend( char *&p, const char *s);
char* strDmAppend( 	// append string s to string p with automatic (re) allocation
	char *&p, 			// base string, ok if NULL on first call
	const char *s )  	// concatenate this to end of p
{
	if (!s || !*s)  return p;				// ok NOP if nothing to append

	// have no way to determine block size so reallocate at every little change. ug!. 10-8-93.

	size_t sz = (p ? strlen(p) : 0) + strlen(s) + 1;	// space needed, + 1 for \0.
	DMP pWas = p;
#if 1	// fix? 8-24-2012
	if (dmral( DMPP( p), sz, WRN)==RCOK) 	// (re)allocate / if ok. dmpak.cpp.
#else
x	if (dmral( (DMP)p, sz, WRN)==RCOK) 		// (re)allocate / if ok. dmpak.cpp.
#endif
	{
		if (!pWas)  *p = 0;			// if new block, make it a null string
		strcat( p, s);				// concatenate the new string. C library.
	}
	return p;					// for possible convenience
}		// strDmAppend
//------------------------------------------------------------------------
void saveAnOutPNam( const char* pNam)	// save an output pathName for return to caller
{
	strDmAppend( 				// append copy of second string to first with automatic heap (re)allocation
		outPNams,			// pathname accumulation string: cse.cpp file-global variable.
		outPNams && *outPNams 		// if not first string
		?  strtcat( ";", pNam, NULL)	// pre-combine separator with new string to minimize heap reallocations
		:  pNam );
}				// saveAnOutPNam
#endif

//===========================================================================================================
// Global initialization/cleanup cleanup dept -- added for rerunnable subr 10-93
//------------------------------------------------------------------------
LOCAL void cnClean( 		// CSE overall init/cleanup routine

	CLEANCASE cs )	// STARTUP (future), ENTRY, DONE, CRASH, or CLOSEDOWN (future). type in cnglob.h.
{
// supports repeatedly-callable, DLLable subroutine.
// function should: initialize any variables for which data-initialization was previously assumed,
//	                free all heap memory and NULL the pointers to it,
//	                close any files and free other resources.
//	                 Note that a DLL should be clean and useable by another caller even if a crash occurred in it,
//	                 yet DLL's resources are not free'd when non-last caller terminates

	// under construction 10-93...

	cncultClean(cs);		// cncult2.cpp. frees input rats; cleans cncult,-2,3,4,5,6.cpp.

	culClean(cs);			// cul.cpp. also cleans cuparse, cutok, ppxxx, exman.

	cgClean(cs);			// cnguts.cpp. also (when completed) will clean other cg files

	vrpakClean(cs);			// clean vrpak files.

	// need to free all big DMP's!

	// free all records (in basAncs)(both input and run)
	// and any "types" sub-basAncs.  includes clearFileIxs().
	cleanBasAncs(cs);		// ancrec.cpp

	if (cs != ENTRY)			// cuz paths are already set up
		xfClean();				// xiopak.cpp and associated files

	// note: rmkerr:erClean is called from cse() -- last call before returning to cse subr caller.

}			// cnClean
//=============================================================================

// end cse.cpp

