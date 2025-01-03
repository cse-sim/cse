// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cuparsex.h: parsing-related stuff shared between cuparse.cpp and cuprobe.cpp


/* 12-91: Extending this file to stuff shared with files split off of cuparse.cpp: initially, cuprobe.cpp.

   10-90: Was not clear whether this stuff will later become local to
   cuparse again -- if all external calls at expTy level or above
   (verb() in cumain now uses much, may be deleted.

   If does not revert to local, look at redefining interfaces to reduce # externally used symbols. */



/*-------------------------------- DEFINES --------------------------------*/

/*----------------- cuparse.cpp's "mostly LOCAL" VARIABLES ----------------*/

/*--- CURRENT TOKEN INFO.  Set mainly by toke().  Not changed by unToke(). */
extern SI tokTy;   		// current token type (CUT__ define; cuTok ret val)
extern SI prec;    		// "prec" (precedence) from opTbl[]. PR__ defines.
extern SI nextPrec;		// "prec" of ungotten (ie next) token, only valid after expTy()/expr()/unToke().
//extern SI lastPrec;		// "prec" of PRIOR token (0 at bof).
//extern OPTBL * opp;		// ptr to opTbl entry for token
extern const char * ttTx;	// saveable ptr to static token descriptive text (opp->tx) for errMsgs.  cul.cpp uses.
// extern void * stbk; 		// symbol table value ptr, set by toke() for already-decl identifiers, type varies...
extern bool isWord;  		// true iff word: reserved, defined, or CUTID.

/*--- CURRENT EXPRESSION INFO, exOrk to expr and callees, incl cuprobe.cpp. */
extern USI evfOk;  	// evaluation frequencies allowed bits for current expression,
							// ffff-->no limits.  EVENDIVL/EVPSTIVL stage bits here mean non start-of-interval evaluation ok.
							// also ref'd in cuprobe.cpp.
extern const char* ermTx;	// NULL or word/phrase/name descriptive of entire expression, for insertion in msgs.

/*--- PARSE STACK: one entry ("frame") for each subexpression being processed.
   When argument subexpressions to an operator have been completely parsed and type checks and conversions are done,
   their frames are merged with each other and the frame for the preceding code (and the operator's code is emitted).
   parSp points to top entry, containing current values. */
struct PARSTK
{   USI ty;			// data type info (TY defines, cuparse.h)
    USI evf;		// evaluation frequency bits (EVF defines, cuevf.h)
    char did;		// nz if does something: value stored or side effect
    UCH nNops;		// 0 or # nop place holders at end stack frame, to be sure not confused with operands. see cnvPrevSf().
    PSOP *psp1, *psp2;	// pointers (psp values) to start & end of pseudo-code
};
extern PARSTK parStk[];	// parse stack buffer
extern struct PARSTK* parSp;	// parse stack stack pointer

/*--- CODE OUTPUT variables */
//extern PSOP* psp;		// set by itPile(), used by emit() &cpp.
//extern PSOP* psp0;	// initial psp value, for computing length
//extern PSOP* pspMax;	// ptr to end of code buffer less *6* bytes
							// (6 = one float or ptr, 1 PSEND)
							// for checking in emit() &cpp.
//extern SI psFull;  	// non-0 if *psp has overflowed

/*--- compiler debug aid display flags */
extern int trace;	// nz for many printf's during compile.
					//  (must be set via temporary code or debugger)

/*--- error count limit.  data init; also $maxErrors 2-91. */
extern SI maxErrors;	// used in cul.cpp

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// some cuparse.cpp fcns used in [cumain.cpp or] cuprobe.cpp
RC FC konstize( SI* isKon, void **ppv, SI inDm);
RC FC newSf();
RC FC combSf();
RC FC dropSfs( SI k, SI n);
RC CDEC emiKon( USI ty, void *p, SI inDm, char **pp);
RC FC emit( PSOP op);
RC FC emit2( SI i);
SI FC tokeNot( SI tokTyPar);
SI FC tokeIf( SI tokTyPar);

// cuprobe.cpp
RC FC probe();

// end of cuparsex.h
