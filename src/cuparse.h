// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cuparse.h: decls for callers of cuparse.cpp and cuprobe.cpp


/*-------------------------------- DEFINES --------------------------------*/

// see cuevf.h for EVFs & variabilities
// see srd.h for expr() data types

/*------------------------- FUNCTION DECLARATIONS -------------------------*/

// cuparse.cpp.  Also see cuparsex.h -- fcn distribution is capricous, 10-8-90
void FC cuParseClean(CLEANCASE cs);
RC FC fov();
RC FC funcsVarsClear();
RC FC exOrk( SI toprec, USI wanTy, USI choiDt, USI evfOkPar, const char* ermTx,
	     USI *pGotTy, USI *pEvf, SI *pisKon, NANDAT* pv, PSOP **pip );
RC FC itPile( PSOP *dest, USI sizeofDest);
RC FC finPile( USI *pCodeSize);
RC FC expTy( SI toprec, USI wanTy, const char* tx, SI aN);
int toke();
void FC unToke();

RC FC cuAddItSyms( SI tokTyPar, SI casi, STBK *tbl, USI entLen, int op);

const char * FC evfTx( USI evf, SI adverb);

RC CDEC per( MSGORHANDLE msg, ...);
RC CDEC perl( MSGORHANDLE msg, ...);
RC CDEC perlc( MSGORHANDLE msg, ...);
RC CDEC pWarn( MSGORHANDLE msg, ...);
RC CDEC pWarnlc( MSGORHANDLE msg, ...);
RC CDEC pInfo( MSGORHANDLE msg, ...);
RC CDEC pInfol( MSGORHANDLE msg, ...);
RC CDEC pInfolc( MSGORHANDLE msg, ...);
RC CDEC perNx( MSGORHANDLE msg, ...);
RC CDEC pWarnNx( MSGORHANDLE msg, ...);
RC FC   perNxV( int isWarn, MSGORHANDLE msg, va_list ap);

void FC curLine( int retokPar, int* pFileIx, int* pline, int* pcol, char *s, size_t sSize);

// cuprobe.cpp
void FC showProbeNames(int showAll);
RC SetupProbeModernizeTables();

// end of cuparse.h