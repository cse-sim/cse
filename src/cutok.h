// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cutok.h: defines for CSE user language input file and get token


//-------------------------------- DEFINES ----------------------------------

//---- cutok.cpp:cuTok()/pp.cpp:ppTok() TOKEN TYPES
	// CAUTION: must match: algorithm used to derive from ascii in cuTok();
	//			order of initialized tables in cueval.cpp.
// single chars ! .. / have tok type ascii - ' '
#define CUTNOT	    1	// !
#define CUTQUOT	    2	// (") string literal. quoted text in cuToktx[].
#define CUTNS	    3	// #
#define CUTDOL	    4	// $
#define CUTPRC	    5	// %
#define CUTAMP	    6	// &
#define CUTSQ	    7	// '
#define CUTLPR	    8	// (
#define CUTRPR	    9	// )
#define CUTAST	   10	// *
#define CUTPLS	   11	// +
#define CUTCOM	   12	// ,
#define CUTMIN	   13	// -
#define CUTPER	   14	// .
#define CUTSLS	   15	// /
// single chars : .. @ have tok type ascii - ':' + 16
#define CUTCLN	   16	// :
#define CUTSEM	   17	// ;
#define CUTLT	   18	// <
#define CUTEQ	   19	// =
#define CUTGT	   20	// >
#define CUTQM	   21	// ?
#define CUTAT	   22	// @
// single chars [ .. ` have tok type ascii - '[' + 23
#define CUTLB	   23	// [
#define CUTBS	   24	/* \ */
#define CUTRB	   25	// ]
#define CUTCF	   26	// ^
#define CUTUL	   27	// _
#define CUTGV	   28	// `
// single chars { ... ~ have tok type ascii - '{' + 29
#define CUTLCB	   29	// {
#define CUTVB	   30	// |
#define CUTRCB	   31	// }
#define CUTTIL	   32	// ~
// additional specials and multi-char tokens
#define CUTID      33	// identifier (text in cuToktx)
#define CUTSI      34	// integer (syntax) numbr, value: cuSival, cuFlval
#define CUTFLOAT   35	// (.) floating point constant, value in cuFlval.
		// 36	available
#define CUTEQL     37	// ==  equality comparison
#define CUTNE      38	// !=
#define CUTGE      39	// >=
#define CUTLE      40	// <=
#define CUTRSH     41	// >> right shift
#define CUTLSH     42	// << left shift
#define CUTLAN     43	// && logical and
#define CUTLOR     44	// || logical or
//insert some spares here
#define CUTEOF     45	// hard end of file (see also CUTDEOF)
// used in cuparse.cpp if operator determined to be binary: if done this way:
#define CUTBPLS    46	// binary +
#define CUTBMIN    47	// binary -
// 48-49 spare
// used in cuparse.cpp:toke() for CUTID's already in symbol table
#define CUTVRB     50	// verb (word that begins a statement)
#define CUTSF      51	// system (pre-defined) function
#define CUTSV      52	// system variable (if implemented)
#define CUTMONTH   53	// month names jan-dec
#define CUTUF      54	// user-defined function
#define CUTUV      55	// user variable, already declared
// ... reserved words. used in cuparse.cpp.
#define CUTDEOF    56	// $EOF
#define CUTDEF     57	// defined(x) (preprocessor only)
#define CUTECMT    58	// */ (end comment, for specific errmsg)
#define CUTDEFA    59	// word "default": calls to fcns choose, select, etc
#define CUTWDFCN   60	// word "function": used in function declaration
#define CUTWDVAR   61	// word "variable": (future) user variable declaration
// ... ... types: initiate user declarations, cuparse.cpp and callers
#define CUTINT     62	// "integer"
#define CUTFL      63	// "float"
#define CUTSTR     64	// "string"
#define CUTDOY     65	// "doy"  poss future use, 1-91
// ... additional words in cul language (cul.cpp, via cuparse.cpp tables)
#define CUTLIKE    66	// "like"
#define CUTTYPE    67	// "usetype"
#define CUTDEFTY   68	// "deftype"
#define CUTRQ      69	// "require"
#define CUTFRZ     70	// "freeze"
#define CUTCOPY    71	// "copy"
#define CUTALTER   72	// "alter"
#define CUTDELETE  73	// "delete"
#define CUTUNSET   74	// "unset" (reset member to default value)
#define CUTAUSZ    75	// "autosize" (automatically determine certain values) 6-95

const int CUTOKMAX= 300;	// max cuTok token length, incl len quoted strings

//--------------------------- PUBLIC VARIABLES ------------------------------

//--- variables set by cuTok()
extern char cuToktx[CUTOKMAX+1];
						// token text: identifier name, quoted texts without quotes, etc
extern USI cuSival;   	// value of integer number
extern FLOAT cuFlval;	// floating value of number

//------------------------- FUNCTION DECLARATIONS ---------------------------
void cuTokClean(CLEANCASE cs);		// init/clean up 10-93
void cuUntok( void);  				// unget token
SI cuTok( void);					// get token
RC cufOpen( const char* fname, char *dflExt);		// open file
void cufClose( void);				// close file
RC CDEC cuEr( int retokPar, const char* message, ...);
RC CDEC cuEr( int shoTx, int shoCaret, int shoFnLn, int retokPar, int fileIx, int line, int isWarn, char *fmt, ... );
RC cuErv( int shoTx, int shoCaret, int shoFnLn, int re, int fileIx, int line, int isWarning, const char* msf, va_list ap=NULL);
void cuCurLine( int retokPar, int* pFileIx, int* pline, int* pcol, char *s, size_t sSize);

// end of cutok.h
