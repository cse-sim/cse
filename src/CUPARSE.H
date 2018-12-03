// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cuparse.h: decls for callers of cuparse.cpp and cuprobe.cpp


/*-------------------------------- DEFINES --------------------------------*/

// see cuevf.h for EVFs & variabilities

//-- expr() data types --   each type is different bit so can represent groups of types and test with 'and'.  Data type USI.
#define TYSI    0x01		// 16-bit integer (or BOOlean)
#define TYFL    0x02		// float
#define TYNUM   (TYSI|TYFL)	// "any numeric data type" (test by and'ing)
#define TYSTR   0x04		// string
#define TYFLSTR (TYFL|TYSTR)	// number (as float) or string, known at compile time. for CSE reportCol.colVal.
#if 1	//new or moved, 2-92
 #define TYID	0x08		// id: string; quotes implied around non-reserved identifier (for record names) at outer level
 #define TYCH   0x10		// choice (DTxxx must also be given): string or id, conv to 16/32 bit choicb/choicn value
 #define TYNC   (TYFL|TYCH)	// number (float) or specified 32 bit choicn choice, not necess known til runtime.
#endif
//	TYDOY	day of year: def in cul.h; given to cuparse.cpp as TYSI, 2-91.
//              0x20-0x80	reserved for expansion of above
#define TYANY	0x1f		// any value ok: 'or' of all of above
//      TYLLI   0x100		not used in cuparse: exman.cpp changes to TYSI.
#define TYNONE  0x200		// no value (start of (sub)expression)
#define TYDONE  0x400		// complete statement: no value on run stack
// CAUTION 0xf000 bits used in cul.h

//-- exman.cpp:exPile data types --
#define TYLLI  0x100		// limited long integer: 16 bit value in 32 bit storage to support nan-flagging
//TYSI   CAUTION  cannot hold nan-flags, use TYLLI (with LI storage!)
//		  everywhere exman's runtime expressions are to be supported
//TYSTR TYID TYFL TYCH TYNC as above
// not used in exman.cpp: TYNUM, TYANY, TYNONE, ? TYDONE.


/*------------------------- FUNCTION DECLARATIONS -------------------------*/

// cuparse.cpp.  Also see cuparsex.h -- fcn distribution is capricous, 10-8-90
void FC cuParseClean(CLEANCASE cs);
RC FC fov();
RC FC funcsVarsClear();
RC FC exOrk( SI toprec, USI wanTy, USI choiDt, USI evfOkPar, char *ermTx,
	     USI *pGotTy, USI *pEvf, SI *pisKon, void *pv, PSOP **pip );
RC FC itPile( PSOP *dest, USI sizeofDest);
RC FC finPile( USI *pCodeSize);
RC FC expTy( SI toprec, USI wanTy, char *tx, SI aN);
SI FC toke();
void FC unToke();

RC FC cuAddItSyms( SI tokTyPar, SI casi, STBK *tbl, USI entLen, int op);

const char * FC evfTx( USI evf, SI adverb);

RC CDEC per( const char* msg, ...);
RC CDEC perl( const char* msg, ...);
RC CDEC perlc( const char* msg, ...);
RC CDEC pWarn( const char* msg, ...);
RC CDEC pWarnlc( const char* msg, ...);
RC CDEC pInfo( const char* msg, ...);
RC CDEC pInfol( const char* msg, ...);
RC CDEC pInfolc( const char* msg, ...);
RC CDEC perNx( const char* msg, ...);
RC CDEC pWarnNx( const char* msg, ...);
RC FC   perNxV( int isWarn, const char* msg, va_list ap);

void FC curLine( int retokPar, int* pFileIx, int* pline, int* pcol, char *s, size_t sSize);

// cuprobe.cpp
void FC showProbeNames(int showAll);

// end of cuparse.h