// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cseface.h -- top level external interface to CSE
//             for any of the following:
//			dynamic link (CSE.DLL) version,
//			static link (linked-in) version (not supported, 9-12)
//
// CSE: California Simulation Engine,
//      a program to simulate building envelope and HVAC to determine energy consumption
//      for particular design under particular weather conditions.
//

// Input for one or more runs is given in input file(s) named in argument(s);
// results are returned in one or more report files and/or data export files.
//
// Subroutine returns non-0 if errors occured; details in error message file.
//
// Either entry cse() or CSEZ() may be used, depending which calling sequence
// is more convenient.
//
// See manual.

// #include windows.h before this file.


#if 1

// assume importing if not otherwise specified
//  (cse build defines as nothing or dllexport)
#if !defined( _DLLImpExp)
 #define _DLLImpExp __declspec( dllimport)
#endif

// callback return values
const int CSE_CONTINUE = 0;		// continue execution
const int CSE_ABORT = -1;		// abort and cleanup
typedef int CSEMSGCALLBACK( int level, const char* msg);


// MAIN FUNCTION DECLARATION
//
//  This is entry point called by a Windows program using CSE as a subroutine,
//  if the program can use the C main( argc, argv) argument convention and the
//  C calling convention. See also CSEZ(), below.
//
//  This declaration applies to both the static-linked version (obj files) and
//  Dynamic Link Library version (link with cse.lib, use cse.dll at runtime).
//  This function's source code is in cse.cpp -- more description there.
//
// Return value is errorlevel as would be returned to DOS: 0 = success.

_DLLImpExp int cse(
				int argc,			// arg count
				const char* argv[],	// arg pointers, [0] = .exe pathname
				CSEMSGCALLBACK* pMsgCallBack=NULL);	// callback for msgs that would
													//   be sent to screen


// EZ INTERFACE TO CSE MAIN FUNCTION
//
//  This alternate entry point may be called by programs that can more
//  conveniently use the pascal calling convention and pass switch and
//  filename arguments as a single string.
//
//  "exePath" is NULL or the path and filename of the program calling the
//  cse subroutine or DLL. Its directory is searched for additional needed
//  files. The calling program can get this with GetModuleFileName().
//
//  "cmdLine" contains the (optional) switches and the input file name(s)
//  (an input file name is required!), separated by spaces, as would be
//  typed on the DOS command line.
//
//  The rest of the arguments are as described for cse() just above.
//
//  The return value is the errorlevel; 0 indicates success.

extern "C" {
_DLLImpExp int CSEZ(
			const char* exePath,	// NULL or the path and filename of the program calling the
									//  cse subroutine or DLL. Its directory is searched for
									//  additional needed files. The calling program can get this
									//  with GetModuleFileName().
			const char* cmdLine,	// (optional) switches and the input file name(s)
									//  (an input file name is required!), separated by spaces
									//  as would be typed on the DOS command line.

			CSEMSGCALLBACK* pMsgCallBack=NULL);	// callback for msgs that would be sent to screen
}

// RETRIEVE PROGRAM VERSION INFO
//   e.g. "CSE 0.704 DLL"
// returns length of string written to buf (i.e. not counting \0)
extern "C" {
_DLLImpExp int CSEProgInfo(
	char* buf,		// where to store info text
	size_t bufSz);	// sizeof( buf)
}

#else
x CNE version for reference
x
x // STRUCTURE used for optional return of binary result memory handles
x //   for fastest access to binary results --
x //	pointer returned via argument "hans" points to this.
x // Exact content of each block is specified elsewhere and need be known
x //	only to package using the data.
x // Caller or package he uses should make sure the blocks get GlobalFree'd,
x //    for example with cneHansFree, below.
x
x struct BrHans
x {
x     HGLOBAL hBrs;  		// 0 or global memory handle for Basic Binary Results file
x    HGLOBAL hBrhHdr;  		// .. for Hourly Binary Results file Header block
x     HGLOBAL hBrhMon[12];	// .. for Hourly Binary Results file Month blocks
x };
x
x
x // MAIN FUNCTION DECLARATION
x //
x //  This is entry point called by a Windows program using CSE as a subroutine,
x //  if the program can use the C main( argc, argv) argument convention and the
x //  C calling convention. See also CNEZ(), below.
x //
x //  This declaration applies to both the static-linked version (obj files) and
x //  Dynamic Link Library version (link with small CSE.LIB, use CSE.DLL at runtime).
x //  This function's source code is in cse.cpp -- more description there.
x //  Also applies to the client-server interface function (cnecl.cpp, new 3-94).
x //
x // "argc" is argument count and "argv" is array of pointers to argument strings,
x //  as for a C language main(). argument 0 is .exe pathName. Additional
x //  arguments can be optional flags and the required input file name.
x //
x // "hInst" and "hPar" are Windows application instance and parent window handle.
x //
x // "outPNams" is NULL or a pointer to a location to receive a pointer
x // to a string of ;-separated output file pathnames.
x //
x // "hans" is NULL or points to a location to receive a pointer to a struct
x // (above) of Windows global memory handles for blocks containing binary
x // results data.
x //    Using the pointer returned via "hans" allows the fastest access to the
x // data, for example by the display package.
x //    (These data are only generatated if appropriate specifications are given
x // in the input file and/or via command line switches.)
x //
x // Return value is errorlevel as would be returned to DOS: 0 = success.
x
x extern "C" int _ExportIfDLL cse( int argc,  char* argv[],
x                              HINSTANCE hInst,  HWND hPar,
x                               char** outPNams,
x                               struct BrHans** hans );
x
x // EZ INTERFACE TO CsE MAIN FUNCTION, 10-95
x //
x //  This alternate entry point may be called by programs that can more
x //  conveniently use the pascal calling convention and pass switch and
x //  filename arguments as a single string.
x //
x //  cse subroutine or DLL. Its directory is searched for additional needed
x //  files. The calling program can get this with GetModuleFileName().
x //
x //  "cmdLine" contains the (optional) switches and the input file name(s)
x //  (an input file name is required!), separated by spaces, as would be
x //  typed on the DOS command line.
x //
x //  The rest of the arguments are as described for cse() just above.
x //
x //  The return value is the errorlevel; 0 indicates success.
x
x extern "C" int _ExportIfDLL pascal CNEZ( const char* exePath, const char* cmdLine,
x                                 HINSTANCE hInst,  HWND hPar,
x                                char** outPNams,
x                                 struct BrHans** hans );
x
x
x // BrHans BINARY RESULTS MEMORY-FREEING FUNCTION
x //
x // Those who use cse's Binary Results memory handle returning feature
x // (non-NULL "hans" argument and -z or -m flags) may use this function
x // to free the memory when they are done with it.
x //
x // For example, call before each cse run unless you wish to keep the memory
x // from the prior run and have copied the handles to your own storage.
x // Also call at program exit.
x //
x // Argument is a pointer returned by cse via "hans" argument,
x // or pointer to copy you have made of storage pointed to thereby.
x // Does nothing if argument is NULL, nor for 0 handles in given struct.
x // Replaces free'd handles with 0s. Redundant calls harmless.
x
x extern "C" void _ExportIfDLL cneHansFree( struct BrHans far * hans);
x
x
x // CLEANUP FUNCTION
x //
x// Call at application exit to:
x //
x // * free memory to which a pointer was returned via cse arguments outPNams
x //   and/or hans;
x // * do any other cse cleanup added in the future.
x //
x // Does NOT free the memory whose handles are IN returned Brhans struct
x // -- use cneHansClean (just above) if you want cse to to that.
x
x extern "C" void _ExportIfDLL cneCleanup();
#endif

// end of cseface.h
