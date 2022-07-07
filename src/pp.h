// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* pp.h: public declarations CSE user language input file preprocessor */

/*-------------------------------- OPTIONS --------------------------------*/

/*-------------- VARIABLES accessed outside of pp.cpp -------------*/
extern int VrInp;	// 0 or virtual report handle (vrpak.cpp) for open INPut listing virtual report.

/*--------------- FUNCTIONS called outside of pp.cpp files --------------*/

// pp.cpp: command line interface for pp switches
SI FC ppClargIf( const char* s, RC *prc /*,era?*/ );

// pp.cpp...: re getting preprocessed text (see pp.cpp for local fcns)
void FC ppClean( CLEANCASE cs);				// init/cleanup
void ppAddPath( const char* paths);			// add path(s) to search for input/include files. 2-21-95.
BOO ppFindFile( const char *fname, char *fullPath);	// search pp paths, return full file path. 2-21-95.
BOO ppFindFile( char* &fname);	// ditto, update fname to path found
RC FC ppOpen( const char* fname, char *defex);		// open file
void FC ppClose();						// close file(s)
USI FC ppGet( char *p, USI n);			// get preprocessed text

// pp.cpp...: input listing
SI   FC openInpVr();
void FC closeInpVr();
void FC lisFlushThruLine( int line);
void FC lisThruLine( int line);
void FC lisMsg( char *p, int dashB4, int dashAf);
int  FC lisFind( int fileIx, int line, const char* p, int *pPlace);
void FC lisInsertMsg( int place, char *p, int dashB4, int dashAf);

void FC dumpDefines();		// debug aid

// end of pp.h
