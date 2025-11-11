// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* pp.h: public declarations CSE user language input file preprocessor */

/*-------------------------------- OPTIONS --------------------------------*/

/*-------------- VARIABLES accessed outside of pp.cpp -------------*/
extern int VrInp;	// 0 or virtual report handle (vrpak.cpp) for open INPut listing virtual report.

/*--------------- FUNCTIONS called outside of pp.cpp files --------------*/

// command line interface for pp switches
inline bool IsCmdLineSwitch( int c) { return c == '-'; }
bool ppClargIf( const char* s, RC& rc);

// re getting preprocessed text (see pp.cpp for local fcns)
void FC ppClean( CLEANCASE cs);				// init/cleanup
void ppAddPath( const char* paths);			// add path(s) to search for input/include files
bool ppFindFile( const char *fname, char *fullPath);	// search pp paths, return full file path
#if 0
bool ppFindFile( char* &fname);	// ditto, update fname to path found
#endif
bool ppFindFile(CULSTR& fname);	// ditto, update fname to path found
RC FC ppOpen( const char* fname, const char* defex);		// open file
void FC ppClose();						// close file(s)
USI FC ppGet( char *p, USI n);			// get preprocessed text

// input listing
SI   FC openInpVr();
void FC closeInpVr();
void FC lisFlushThruLine( int line);
void FC lisThruLine( int line);
void FC lisMsg( char *p, int dashB4, int dashAf);
int  FC lisFind( int fileIx, int line, const char* p, int *pPlace);
void FC lisInsertMsg( int place, char *p, int dashB4, int dashAf);

// end of pp.h
