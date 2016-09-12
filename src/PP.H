// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* pp.h: public declarations CSE user language input file preprocessor */

/*-------------------------------- OPTIONS --------------------------------*/

#define ECHO	// define for input listing code; undefine to remove most.
				// Implemented only with VRR as of 11-91.

#ifdef ECHO	// else VRR makes no difference
   #define VRR	// define for input echo (listing) to go to "virtual report" using external fcns, and variable "VrInp".  11-22-91.
				// undefine for portable linkability.
				// tested in pp.cpp.
				// suggested future resolution: echo input thru a callback function
				//   whose address is provided by application to a new entry point to this file.
#endif

/*-------------- VARIABLES accessed outside of pp.cpp -------------*/

#ifdef ECHO
 #ifdef VRR
  extern int VrInp;	// 0 or virtual report handle (vrpak.cpp) for open INPut listing virtual report.
 #endif
#endif

/*--------------- FUNCTIONS called outside of pp.cpp files --------------*/

// pp.cpp: command line interface for pp switches
SI FC ppClargIf( const char* s, RC *prc /*,era?*/ );

// pp.cpp...: re getting preprocessed text (see pp.cpp for local fcns)
void FC ppClean( CLEANCASE cs);				// init/cleanup
void ppAddPath( const char* paths);			// add path(s) to search for input/include files. 2-21-95.
BOO ppFindFile( const char *fname, char *fullPath);	// search pp paths, return full file path. 2-21-95.
RC FC ppOpen( const char* fname, char *defex);		// open file
void FC ppClose();						// close file(s)
USI FC ppGet( char *p, USI n);			// get preprocessed text

// pp.cpp...: input listing
SI   FC openInpVr();
void FC closeInpVr();
void FC lisFlushThruLine( SI line);
void FC lisThruLine( SI line);
void FC lisMsg( char *p, SI dashB4, SI dashAf);
SI   FC lisFind( USI  fileIx, SI line, const char* p, USI *pPlace);
void FC lisInsertMsg( USI place, char *p, SI dashB4, SI dashAf);

void FC dumpDefines();		// debug aid

// end of pp.h
