// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* exman.h: decls for expression manager (exman.cpp) */

// include first: srd.h, for SRD. <==== NO LONGER NEEDED 1-91 edit out unnec includes and many comments ***************


/*---------- NANDLEs and NANDAT ----------*/

     /* a NANDLE is a 32-bit quantity that is not a valid floating point
	number or string pointer, used to specify the "expression number"
	for data whose value is not known at input time, and also to specify
	UNSET (no value assigned yet).

	During input, NANDLES are stored in place of string and float
	values and moved and copied freely; after input is set up, the data
	is searched for NANDLES to build the expression table that drives
	evaluating and storing expressions during the run.  (SI data is
	extended to 32 bits (TYLLI) where necessary to allow for NANDLEs.)

	The bit format of a NANDLE is: 0xFF800000 + n:						corrected from FF000000 2-92.
	  n = 0 to indicate unset;
	  n = expression number 1-16383 to indicate place to store expression.

	CAUTION: NANDLE format is system and software dependent:
	depends on IEEE floating point format (hi word FF80 is Not-A-Number);
	depends on IBM memory allocation (segment FF80 should not occur in
	  data pointers as ROM is there);
	depends on user caution to limit SI data stored in LI to 16 bits
	  to insure data not looking like a NANDLE.
	must be kept distinct from NCHOICES (cnglob.h?): 7F8x hi word. */


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
void FC exClean(CLEANCASE cs);
RC FC exPile( SI toprec, USI wanTy, USI choiDt, USI fdTy, USI _evfOk, USI useCl,
              char *ermTx, SI isType, BP b, TI i, SI fn,
	      NANDAT *pDest, USI *pGotTy, USI *pGotEvf );
RC FC uniLimCt( USI fdTy, SI ty, char *ermTx, void *p );
void FC extAdj( BP b, TI minI, TI delta);
void FC extMove( record *nuE, record *e);
void FC extDup( record *nuE, record *e);
void FC extDel( record *e);
void FC extDelFn( BP b, TI i, SI fn);
void FC extClr( void);
RC FC exClrExUses( BOO jfc);	// arg added 10-95
RC FC exWalkRecs( void);
// RC FC exReg( USI ancN, TI i, USI o); 		if wanted: restore when needed
RC addChafIf ( NANDAT *pv, USI ancN, TI i, USI o);
RC FC exEvEvf( USI evf, USI useCl);
RC FC exInfo( USI h, USI *pEvf, USI *pTy, NANDAT *pv );
const char* FC whatEx( USI h);
const char* FC whatNio( USI ratN, TI i, USI off);
RC CDEC rer( char *msg, ...);
RC CDEC rer( int erOp, char *msg, ...);
RC CDEC rWarn( char *msg, ...);
RC CDEC rWarn( int erOp, char *msg, ...);
RC CDEC rInfo( char *msg, ...);
RC rerIV( int erOp,	int isWarn, const char *fmt, va_list ap=NULL);


// end of exman.h
