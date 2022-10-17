// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cse.h: declarations for cse.cpp (CSE subroutine main function)
//          for use within CSE subroutine.


/*--------------------------- PUBLIC VARIABLES ----------------------------*/
extern const char ProgName[];   	// program name text, eg "CSE"
extern const char ProgVersion[];	// static version number text for CSE program, e.g. "0.15",
extern const char ProgVariant[];	// static variant text
extern int TestOptions;				// test option bits, set via -t command line argument
#ifdef WINorDLL
 //extern const char ConsoleTitle[];	make public when need found: caption for "console" window displayed during run.
  extern const char MBoxTitle[];	// error and other message box caption text, cse.cpp, used in rmkerr.cpp.

  #ifdef __WINDOWS_H			// defined in windows.h (Borland 3.1, 4)
    extern HINSTANCE cneHInstApp;	// appl instance handle: needed eg for registering window classes in rmkerr.cpp
    extern HWND cneHPar;		// appl window handle to use as parent for (error msg) windows opened eg in rmkerr.cpp
  #else	/* windows.h not included. Declare as types would be declared in Windows 3.1 windows.h with "STRICT"..
           so can compile without windows.h. Undefined structs after the word "struct" are ok in unused declarations. */
    //32: #define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
    //16: #define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef const struct name##__ NEAR* name
      extern struct HINSTANCE__ *  cneHInstApp;		// appl instance handle: needed to register window classes
      extern struct HWND__ *       cneHPar;   		// appl window handle for parent for windows eg errMsgBoxes
  #endif
#if defined( _DLL)
  extern HINSTANCE hInstLib;	// library's module instance handle
#endif
#endif
#ifdef BINRES //cnglob.h
  extern struct BrHans* hans;	// pointer to struct (cnewin.h) for returning memory handles of binary results data.
   								// NULL if not returning memory handles.
#endif

/*--------------------------- PUBLIC FUNCTIONS ----------------------------*/
// public for use within CSE subroutine.
#ifdef OUTPNAMS //cnglob.h
  #ifndef GLOB	//decl in cnglob.h so don't need cse.h where this is called. 10-93.
     void FC saveAnOutPNam( const char * pNam);
  #endif
#endif

// re sending DLL screen msgs to calling EXE
// defined but not used per LOGCALLBACK
void LogMsgViaCallBack( const char* msg, int level=0);
// int CheckAbort() -- see cnglob.h

// end of cse.h
