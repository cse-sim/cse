// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// ENVPAK.C: Environment specific routines -- Windows dependent

/*------------------------------- INCLUDES --------------------------------*/
#include "CNGLOB.H"
// #include "cse.h"
#if (_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <signal.h> 	// signal SIGINT
#include <float.h>	// FPE_UNDERFLOW FPE_INVALID etc
#include <conio.h>
#include <sys/timeb.h>	// timeb structure

#include <LOOKUP.H>	// lookws wstable
#include <TDPAK.H>	// tdldti
#include <XIOPAK.H>	// xchdir

#include <ENVPAK.H>	// declares functions in this file

/*-------------------------------- DEFINES --------------------------------*/
// (none now)

/*--------------------------------- TYPES ---------------------------------*/
/* Types used here, defined elsewhere, include:
*  system time (as LI seconds since 0:0:0 Jan 1 1970 under MSDOS):
*    typedef long time_t	msc library's time.h type for time() return
*    typedef long LDATETIME 	our type for same thing. dtypes.def/dtypes.h
*				(dtypes.h is #included in cnglob.h) */

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

LOCAL void CDEC enkiint(INT); 	// no NEAR -- passed to another function

/*============================ TEST CODE =============================*/
/* #define TEST */
#ifdef TEST
t   unmaintained 9-12-89
t main ()
t {
t SI i;
t float x;
t LDATETIME ldt;
t IDATETIME idt;
t
t  #ifdef KITEST
t     enkiinit();
t     enkimode(KICLEAR+KIBEEP);
t     for (i = 0; i < 100; i++)
t     {	 if (enkichk())
t        {  printf("\n\nInterrupt....");
t           gcne();
t		 }
t		 printf("Hi %d\n",(INT)i);
t	  }
t	  enkimode(KILEAVEHIT);
t     if (enkichk())
t        printf("Still set\n");
t     else
t        printf("Clear\n");
t
t     hello(NULL);
t     enkimode(KICLEAR+KIBEEP);
t     printf("\nlooping...");
t     for (i = 0; i < 10000; i++)
t     {  x = 2.;
t        x = x*x*x*x*x;
t        if (enkichk())
t        {	printf("\n\nInterrupt....");
t           gcne();
t           enkimode(KICLEAR+KIBEEP);
t		 }
t	  }
t     printf("Done.");
t     enkimode(KILEAVEHIT);
t     if (enkichk())
t        printf("Still set\n");
t     else
t        printf("Clear\n");
t  #endif   /* KITEST */
t  #ifdef DTTEST
t     ldt = ensysldt();
t     ensystd(&idt);
t     printf("Time: %ld\n",ldt);
t  #endif /* DTTEST */
t }
#endif
//=============================================================================
WStr enExePath()		// full path to current executable
{
	WStr t;
	char exePath[FILENAME_MAX + 1];
#ifdef __APPLE__
		uint32_t pathSize = sizeof(exePath);
		_NSGetExecutablePath(exePath, &pathSize);
		t = exePath;
		WStrLower(t);
#elif __linux__
		ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
		if (len == -1) {
			std::cout << "ERROR: Unable to locate executable." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		else {
			exePath[len] = '\0';
			t = exePath;
			WStrLower(t);
		}
#elif _WIN32
		if (GetModuleFileName(NULL, exePath, sizeof(exePath)) > 0)
		{
			t = exePath;
			WStrLower(t);
		}
#endif
	WStrLower(t);
	return t;
}		// enExePath
//=============================================================================
WStr enExeInfo(		// retrieve build date/time, linker version, etc from exe
	WStr exePath,		// path to .exe
	int& codeSize)		// code size (bytes)
// returns info as string or "?" if error
{
	const char* msg = "?";
	time_t timeDateStamp = 0;
	codeSize = 0;
	WStr linkerVersion;

	#ifdef _WIN32
	HANDLE hFile = CreateFile( exePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		msg = "Cannot open file";
	else
	{
		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hFileMapping == 0)
			msg = "Cannot create file mapping";
		else
		{	PIMAGE_DOS_HEADER pDosHeader =
				(PIMAGE_DOS_HEADER)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
			if (pDosHeader == 0)
				msg = "MapViewOfFile() failed";
			else
			{	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
					msg = "Invalid EXE file";
				else
				{	try
					{	// use try/catch re possible bad e_lfanew, EOF, etc.
						PIMAGE_NT_HEADERS pNTHeader =
							PIMAGE_NT_HEADERS(DWORD(pDosHeader) + pDosHeader->e_lfanew);
						if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
							msg = "Not a Portable Executable (PE) EXE";
						else
						{
							timeDateStamp = pNTHeader->FileHeader.TimeDateStamp;
							codeSize = pNTHeader->OptionalHeader.SizeOfCode;
							linkerVersion = WStrPrintf( "%d.%d",
									pNTHeader->OptionalHeader.MajorLinkerVersion,
									pNTHeader->OptionalHeader.MinorLinkerVersion);
						}
					}
					catch (...)
					{	msg = "Invalid EXE file";
					}
				}
				UnmapViewOfFile( pDosHeader);
			}
			CloseHandle( hFileMapping);
		}
		CloseHandle( hFile);
	}
	#endif
	WStr exeInfo;
	if (timeDateStamp == 0)
		exeInfo = "? (enExeInfo fail)";
	else
		exeInfo = WStrPrintf( "%s   (VS %s    %d bytes)",
						tdldts( timeDateStamp, NULL),
						linkerVersion.c_str(),
						codeSize);
	return exeInfo;
}		// enExeInfo
//=====================================================================
void FC ensystd(		// Return the system date and time as IDATETIME

	IDATETIME *dt )	// Pointer to structure to receive date and time
					// (dtypes.h struct, described in tdpak.cpp)
{
// Get time in LDATETIME format from system, convert to IDATETIME
	tdldti(			// convert format: gets year/month/mday/wday. tdpak.cpp
		ensysldt(),		// get time from system as seconds from 1/1/1970
		dt );		// caller's destination
}		// ensystd
//=====================================================================
LDATETIME FC ensysldt()		// Return system date and time as LDATETIME
{
	return (LDATETIME)time(NULL);	// LI seconds since 1/1/1970 (msc library)
}			// ensysldt

/*  *************** Keyboard Interrupt Handling Routines ************ */

#define KIWAITING 0	// Waiting for a keyboard interrupt
#define KIHIT 1  	// KI received but not yet noticed by program
#define KISEEN 2 	// KI has been noticed by prog and is being processed.
					//    Don't bother user further.
					//    reverts to KIWAITING when enkimode called.
static SI NEAR kiflag = KIWAITING;	// Interrupt flag used by routines
static SI NEAR kimode = 0;  		// KIBEEP bit on to beep at ^C.

//=====================================================================
void FC enkiinit(		// Initialize for keyboard interrupt handling

	SI mode )	// Keyboard interrupt mode, passed to enkimode().
				//   Should contain a KICLEAR to properly initialize.
{
	signal( SIGINT, enkiint);	// tell C lib to call enkiint() on ^C
	enkimode(mode);		// next. save beep bit, clear kiflag.
}			// enkiinit
//=====================================================================
void FC enkimode(		// Set keyboard interrupt mode and conditionally reset interrupt flag

	SI mode )	// Keyboard interrupt mode.  defines in envpak.h:
				//  KIBEEP:      beep on ^C.  Always used 11-12-89.
				//  KISILENT(0): don't beep.  No uses.
				//  KICLEAR (0): always clear internal control flag
				//  KILEAVEHIT:  do not clear hit not yet seen by program (any real uses? dospak:pause() 11-89... */
{
	kimode = mode;		// save mode (beep bit tested in enkiint)
	if (kiflag == KISEEN 		// if hit has been seen by program
			|| ((mode & KILEAVEHIT) == 0)) 	// or not KILEAVEHIT (ie is KICLEAR)
		kiflag = KIWAITING;		// clear internal control flag
}			// enkimode
//=====================================================================
LOCAL void CDEC enkiint(		// no NEAR -- passed to another function

// Control is transferred here when user issues a keyboard interrupt

	INT /*val*/ )	// MSC runtimes pass an arg.  UNUSED ARG.
{
	signal( SIGINT, enkiint);	// reactivate at C library level
#ifdef ENVDEBUG
0    printf("\nENKIINT called...");
#endif
	if (kiflag == KIWAITING)	// change "waiting" to "hit"
		kiflag = KIHIT;		// but leave KISEEN unchanged
#if 0	// sound not supported, 4-12
x	if (kimode & KIBEEP) 	// if requested when mode set
x		enbeep(BEEPINT);		// beep. sound.cpp.
#endif
}			// enkiint
//=====================================================================
int enkichk()			// Check whether a keyboard interrupt has been received

// Returns non-0 if ^C has occurred and has not yet been cleared by calling enkimode().
{
#if 1
	return CheckAbort();
#else
	// currently not implemented under Windows 2-94.
	// to implement, watch keyboard messages in (main) window proc.

	return FALSE;		// claim no ^C typed
#endif
}				// enkichk
//=====================================================================



/*-------------------------- FUNCTION DECLARATIONS ------------------------*/
// PUBLIC functions called only from msc lib or interrupt code; no prototype
//   in envpak.h so inadvertent external calls are flagged.  8-31-91
/* INT  matherr( struct exception *);	 private math runtime error fcn;
					 actual decl in math.h */
void CDEC fpeErr( INT, INT);		// intercepts floating point errors, and integer errors under Borland
void CDEC iDiv0Err( SI);			// intercepts integer x/0 errors under MSC
void __cdecl fpeErr( INT, INT);		// intercepts floating point errors, and integer errors under Borland

/*---------------------------- LOCAL VARIABLES ----------------------------*/

// saved by hello() for byebye()
LOCAL void (* CDEC NEAR byebyeFcn)(int exitCode) = NULL;	// exit function address
LOCAL char cwdSave[FILENAME_MAX] = {0};						// current directory to restore at exit


/*----------------------------- TEST CODE ----------------------------------*/
#ifdef TEST
t
t main()			/* Test routines */
t{
t SI sec, i, j;
struct exception tx;
double x;
RC rc;
t
t     j = 0;
t     dminit();		/* initialize dynamic memory */
t     hello( NULL);
t
t     x = sqrt(-1.);
t     x = asin(5.);
t
t #if 0
t     x = 0.;
t     x = 1./x;
t #else
t     i = j;
t     i = 1/i;
t #endif
t
t     return 0;
t}			/* test main */
#endif	/* TEST */

//=====================================================================
void FC hello(		// initializes envpak, including re library code error exits and user exits.

	void CDEC (*_byebyeFcn)(int code))	// pointer to exit function for byebye (eg may do caller's longjmp).
										// argument is program or subr package return value, e.g. dos errorlevel. 0 ok, nz error.

/* This function inits re the floating point and divide by zero interrupts,
   and saves the above fcn ptrs and current drive and dir for use at any exit
   via byebye (next). Such exits include fatal errors from many lib\ fcns,
   via message fcns in lib\rmkerr.cpp. */
{
//--- save arguments
byebyeFcn = _byebyeFcn;		// save function addresses for use by byebye()

#if 0
signal( SIGFPE, 			// floating point errors, and integer errors (changes 0:0) under Borland C
		(void (*)(INT))fpeErr);	// fpeErr takes 2 args 1-31-94 but signal prototype expects 1-arg fcn.
	// also, our matherr (below) is linked in for C lib math fcn errs.
#endif

//--- integer divide by 0 error setup
	// _dos_setvect( 0, (void (interrupt *)())iDiv0Err );	// 0: integer divide by 0
	//    to ease reporting calling location (in which case aborts, no return). */

//--- save current drive/dir to restore at exit
	strcpy(cwdSave,xgetdir());	// save current dir (including drive) in static for byebye to use at exit
}		// hello
//=====================================================================
void FC byebye(		// cleanup and normal or error exit.

	int exitCode )	// DOS errorlevel or other code to return
					// to parent process or caller of subroutine package or DLL.
					// 0 means ok; use 2 or more for errors to reserve 1 for
					// possible alternate good exit (eg output file unchanged, )

// does not return to caller.
{
//--- restore current directory
	xchdir( cwdSave, WRN );		// restore drive and dir, xiopak.cpp.
								//   Ok NOP if "". cwdSave set in hello, just above.
	cwdSave[0] = '\0';				// clear saved dir eg for when subr pkg recalled.

	throw exitCode;

}		// byebye
//==========================================================================
UINT doControlFP()
{
#if defined( _DEBUG)
	int cw = _controlfp( 0, 0);
	cw &= ~(_EM_OVERFLOW | _EM_UNDERFLOW | _EM_ZERODIVIDE);
	_controlfp( cw, _MCW_EM);
#endif
	return 0;
}		// doControlFP
//==========================================================================
INT CDEC matherr(	// Handle errors detected in Microsoft/Borland math library

	struct _exception *x )	// Exception info structure provided by Microsoft; see math.h

// Calls rmkerr fcns to report error (as a warning), then returns a 1 to prevent error reporting out of library.
{
static WSTABLE /* { SI key, value; } */ table[] =
{
	{ DOMAIN,     "domain" },
	{ SING,       "argument singularity" },
	{ OVERFLOW,   "overflow" },
	{ UNDERFLOW,  "underflow" },
	{ TLOSS,      "total loss of significance" },
	{ PLOSS,      "partial loss of significance" },
	{ 32767,      "unknown exception type" }
};

	errCrit( PWRN,			// display critical msg, wait for key, abort (rmkerr.cpp)
			 "X0104: matherr exception (%s), possible argument values are "
			 "arg1=%lg arg2=%lg",
			 lookws( x->type, table),		// exception type text
			 x->arg1, x->arg2);		// POSSIBLE arg values; not reliable since intrinsic fcns do not set these members

	// limit return value to (float)-able range so no subsequent FPE overflow:
	// most math fcn calls in products cast result to float.

	if (x->retval > FHuge)		// FHuge = 1.E38, cons.cpp
		x->retval = FHuge;
	else if (x->retval < -FHuge )
		x->retval = -FHuge;

	return 1;		// 1 -> consider error corrected, continue
}		// matherr

//==========================================================================
void CDEC fpeErr(		// Handle floating point error exceptions
	[[maybe_unused]] INT sigfpe,		// SIGFPE is passed (currently not used)
	INT code )		// code indicating error

// Calls BSG error routines to report error with PABT.
// Note: initialization for this (using signal() ) is in hello() (above).
{
	static WSTABLE /* { SI key, char *s; } */ table[] =
	{
		{ FPE_ZERODIVIDE,     "divide by 0" },
		{ FPE_OVERFLOW,       "overflow" },
		{ FPE_UNDERFLOW,      "underflow" },
		{ FPE_INVALID,        "invalid (NAN or infinity)" },
		{ FPE_INEXACT,        "inexact" },
#ifndef __BORLANDC__	// in MSC, not Borland
		{ FPE_SQRTNEG,        "negative sqrt" },
		{ FPE_DENORMAL,       "denormal" },
		{ FPE_UNEMULATED,     "unemulated" },
		{ FPE_STACKOVERFLOW,  "stack overflow" },
		{ FPE_STACKUNDERFLOW, "stack underflow" },
#endif
#ifdef __BORLANDC__	// in Borland, not MSC
		{ FPE_STACKFAULT,	 "coprocessor stack overflow" },
		{ FPE_INTDIV0,	 "interger divide by 0" },
		{ FPE_INTOVFLOW,	 "interger overflow" },		// only occurs if INTO executed wiht OF set -- not from C code
#endif
		{ 32767,	         "unknown error code" }
	};

// NOTE: CR/TK version contains code which extracts the error address from
//       the stack, allowing msg include info about where error occurred.
//       Could be adapted for use here if FPE trackdown is a problem.

// issue message and abort

	errCrit( PABT,				// display critical msg, wait for key, abort (rmkerr.cpp)
			 "X0103: floating point exception %d:\n    %s",
			 (INT)code,					// show code for unknown cases 1-31-94
			 lookws( (SI)code, table));

	// no return
	/* DO NOT ATTEMPT TO RETURN and continue program: state of 8087 unknown;
	   registers probably clobbered in ways that matter. */
}		// fpeErr
//---------------------------------------------------------------------------
int CheckFP( double v, const char* vTag)		// check for valid FP value
// see CHECKFP macro (cnglob.h)
// returns 0 if OK, else 1
{	const char* t;
	if (_isnan( v))
		t = "NAN";
	else if (!_finite( v))
		t = "Inf";
	else
		return 0;
	printf( "Bad value: %s %s\n", vTag, t);
	return 1;
}		// ::CheckFP
//============================================================================

#if 0 && !defined( __BORLANDC__)	// used under microsoft C; borland generates floating point error
0 //==========================================================================
0 void CDEC iDiv0Err(		// report integer divide by zero under MSDOS (MicroSoft C)
0
0     SI fakearg )  	// to make it make a stack frame for backtrace (MSC)
0
0 // Note: initialization to activate this (using _dos_setvect) is in envpak.hello.
0
0 /* NO RETURN.
0    NOT declared as interrupt routine since that makes incorrect stack frame
0    for erhndl's backtrace: pushes all registers so saved bp is not at bp,
0    return address is not at bp+2. */
0 /*ARGSUSED */  // fakearg not referenced
0{
0    _enable();		// re-enable interrupts
0
0    /* note: does not display ideally (removing following call from display)
0 	    because did not get here with a call and FmtBkt does not decode
0 	    the divide instruction(s). */
0
0    errCrit( PABT,	// disp critical msg, wait for key, abort (rmkerr.cpp)
0             "X0102: integer divide by 0");
0
0    // no return
0    // DO NOT RETURN since register preserve and IRET would be needed
0
0}	// iDiv0Err
#endif
//=============================================================================

// end of envpak.cpp
