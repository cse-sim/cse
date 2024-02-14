// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cuparse.cpp: cal non-res engine (CSE) expression parser-compiler */

// 7-92: pp parsing bug to pursue sometime:
//	#if 0// no space after 0 fails

// 7-92: cupIncRef after PSDUP4 needed for strings (search PSDUP4).

// TYNC stuff 2-92
//   how to get select etc choice args konstized even tho can't from tconv:
//     use most restrictive wanTy so expTy does --> recode expTy to take any bit combo
//   be suspicious of type checks with & as now have multi bits in parSp->ty as well as wanTy.
//   add ISNUM and ISCHOICE user fcns?
//   add conversions as places found where lacking
//	float expected, if got TYNC, use CSE_E( cnvPrevSf( 0, PSNCH, 0))	// 'convert' to number, ie error if choice
//			  		 parSp->ty = TYFL;
//	string expected, if got TYNC, see if TYCH; if got TYCH, add convert-back-to-string if needed
//	choice expected (believe aren't any now)  use CSE_E( cnvPrevSf( 0, PSSCH, 0)); parSp->ty = TYCH;
//   search PSFLOAT for more places where float expected
//   probe review needed. choices, nchoices, TYNC-valued exprs resulting from such probe,
//		possible probe that errors if NC is not numeric (combine probe & convert for specific err msg)
//   possible probe codes to get numeric or choice values or issue specific error messages; use in cnvPrevSf.
//   fcnArgs MA type-matching switch probably unneeded -- default does it all now?
//   insurance: isKonExp should identify constants for TYNC, and return type TYCH or TYFL
//		---> konstize can change parSp->ty, or isKonExp/konstize ty return path so caller changes. review calls.
//   nb TYCH intended to accept DTCHOICN's even without TYFL.



/*--------------------------------- NOTES ---------------------------------*/

/* 1991: actual application use of cuparse.cpp is for expressions only;
         CSE statement level parsing is done in cul.cpp;
         there is no procedural code;
         so many loose ends re procedures, variables, etc have not required cleanup. */

/* 10-90 plan: leave many cuparse loose ends while working on calling context.
	       Come back and add features as required. */

/* cuparse.cpp notes, mostly from 9-90:

agenda: do:
 more system functions
 think about runtime arg binding for user expressions (user fcns)
 initial facility for finding the important evf bit (as an uncalled? fcn)

agenda: consider:
 probing data structures?

agenda: the big issues
 user functions: runtime arg binding  wasn't this done?
 accessing data structures / probe    DONE 1991
 custom reports                       DONE 1991
 nans & propogating expressions       DONE 1991

agenda: pending for later:
 search >>> (done 10-5-90)
 "built-in" --> "system" ? make consistent esp. in msgs.
 strings: if any string operators, even assignment or fcn return,
    need storage duping/management

tentative decisions:
 no general procedure, just expression fcns for now
 no assignment except to system variables or "fcns" for now
 system variables vs fcns of no args (both left and right) issue: do both
 nested sets of any type: emiDup() first then emit code that pops & stores.

believed complete:
 delete any remaining comments re prec & ty values needed for whole statement

AGENDA items decided to defer, 10-5-90:
  additional c operators: do += etc when/if do variables.
  -D on command line,
  @file on command line for infinite cmd line.
  case sensitivity: re-look-up undefined words case-insensitive
*/

#include "cnglob.h"

/*-------------------------------- OPTIONS --------------------------------*/

#undef EMIKONFIX	// define (and remove *'s) for change in emiKon() to fix
					// apparent bug in konstize() found reading code 2-94.
					// Leave unchanged till code tests out bad.

#undef LOKSV	// define to restore little-used historical code for assignment to system variables, 2-95

#undef LOKFCN	// define to restore little-used historical code for assignment to functions, 2-95

/*------------------------------- INCLUDES --------------------------------*/
// #include <cnglob.h> above

#include "srd.h"	// SFIR
#include "ancrec.h"	// record: base class for rccn.h classes
#include "rccn.h"	// TOPstr
#include "msghans.h"	// MH_S0001

#include "cvpak.h"	// cvS2Choi

#include "cnguts.h"	// Top
#include "impf.h"	// impFcn

#include "pp.h"   	// dumpDefines(): debug aid, 9-90
#include "sytb.h"	// symbol table: SYTBH; syXxx fcns
#include "cutok.h"	// token types: CUT___ defines
#include "cueval.h"	// pseudo-code: PS___ defines; cupfree printif
#include "cuevf.h"	// evf's and variabilities: EVF____ defines
#include "cuparsei.h"	// CS__ defines, PR___ defines, OPTBL opTbl

#include "cuparsex.h"	// stuff shared by [cumain,] cuparse, cuprobe; cuTok parSp ERVARS ERSAVE

#include "cuparse.h"	// defs/decls for external callers of this file; EVFHR TYSI.  cueval.h rqd 1st.

/*-------------------------------- DEFINES --------------------------------*/
// needed declarations in each fcn
#define ERVARS  PARSTK * parSpSv; PSOP* pspSv; RC rc=RCOK;
#define ERVARS1  PARSTK * parSpSv; PSOP* pspSv; RC rc;	// .. with no rc init: use when compiler warns value unused.
// do at entry to each fcn (after poss top-level init)
#define ERSAVE  parSpSv = parSp; pspSv = psp;
// common error exit code needed in each fcn
#define ERREX(f)   er: parSp = parSpSv;  psp = pspSv; \
		   printif( trace," "#f"Er ");  return rc;
// call fcn f, take error exit if returns bad
#define EE(f)    { rc=(f); if (rc) goto er;}


/*------------------------ OPERATOR TABLE DATA -------------------------*/

// This table drives two parsers:
//   preprocessor parser in ppCex.cpp, and compiler parser in cuparse.cpp.
// subscript is token type (defines in cutok.h)
// as returned by cutok.cpp:cuTok() and refined by cuparse.cpp:toke(),
// or returned by ppTok.cpp:ppTok() and refined by ppCex:ppToke().
//struct OPTBL
//{   SI prec;  	/* (left) precedence: determines order of operations */
//    SI cs;		/* case:      CSBIN   CSUNN   CSGRP          (types) */
//    SI v1;		/* value 1:  si ps   si ps   expected CUTxx  TYSI etc  */
//    SI v2;		/* value 2:  fl ps   fl ps   exp. 'token'    byteSize  */
//    char * tx;		/* for ttTx, thence error messages. */
//};	in cuparsei.h					     /* ... shortened program 180 bytes w/o lengthening DGROUP. */

OPTBL opTbl[] =
{
	// order of entries MUST MATCH CUTxxx defines in cutok.h
	/*prec  cs	v1	v2 	text
	cupars- cupars-  cueval.h
	 ei.h:   ei.h:    etc:           		   cutok.h: */
	0,		0,		0,			0,		"",			// 0 unused
	34,		CSUNI,	PSINOT,		0,		"!",		// CUTNOT   01 !
	PROP,	CSCUT,	0,			0,		"\"text\"",	// CUTQUOT  02 (") string literal. quoted text in cuToktx[].
	perr,	CSU,	0,			0,		"#",		// CUTNS    03 #
	perr,	CSU,	0,			0,		"$",		// CUTDOL   04 $
	32,		CSBII,	PSIMOD,		0,		"%",		// CUTPRC   05 %
	19,		CSBII,	PSIBAN,		0,		"&",		// CUTAMP   06 &
	36,		CSBIF,	PSFINCHES,	0,		"'",		// CUTSQ    07 ' feet-inches
	PROP,	CSGRP,	CUTRPR,		')',	"(",		// CUTLPR   08 (
	PRRGR,	CSU,	0,			0,		")",		// CUTRPR   09 )
	32,		CSBIN,	PSIMUL,		PSFMUL,	"*",		// CUTAST   10 *
	34,		CSUNN,	0,			0,		"+",		// CUTPLS   11 unary +  (code changes to CUTBPLS if binary)
	PRCOM,	CSCUT,	0,			0,		",",		// CUTCOM   12 ,
	34,		CSUNN,	PSINEG,		PSFNEG,	"-",		// CUTMIN   13 unary -  (code changes to CUTBMIN if binary)
	perr,	CSU,	0,			0,		".",		// CUTPER   14 .
	32,		CSBIN,	PSIDIV,		PSFDIV,	"/",		// CUTSLS   15 /
	12,		CSU,	0,			0,		":",		// CUTCLN   16 :
	PRSEM,	CSU,	0,			0,		";",		// CUTSEM   17 ;
	25,		CSCMP,	PSILT,		PSFLT,	"<",		// CUTLT    18 <
	PRASS,	CSU,	0,			0,		"=",		// CUTEQ    19 = assignment
	25,		CSCMP,	PSIGT,		PSFGT,	">",		// CUTGT    20 >
	13,		CSCUT,	0,			0,		"?",		// CUTQM    21 ?
	PROP,	CSCUT,	0,			0,		"@",		// CUTAT    22 @ probe
//  single chars [ .. ` have tok type ascii - '[' + 23
	PROP,	CSGRP,	CUTRB,		']',	"[",		// CUTLB    23 [ grouping
	perr,	CSU,	0,			0,		"\\",		// CUTBS    24 \ */
	PRRGR,	CSU,	0,			0,		"]",		// CUTRB    25 ]
	18,		CSBII,	PSIXOR,		0,		"^",		// CUTCF    26 ^ xor
	perr,	CSU,	0,			0,		"_",		// CUTUL    27 _
	perr,	CSU,	0,			0,		"`",		// CUTGV    28 `
//  single chars { ... ~ have tok type ascii - '{' + 29
	perr,	CSU,	0,			0,		"{",		// CUTLCB   29 {
	17,		CSBII,	PSIBOR,		0,		"|",		// CUTVB    30 |
	perr,	CSU,	0,			0,		"}",		// CUTRCB   31 }
	34,		CSUNI,	PSIONC,		0,		"~",		// CUTTIL   32 ~ one's compl
//  additional specials and multi-char tokens
	PROP,	CSCUT,	0,			0,		"identifier",	// CUTID    33 identifier not yet declared, text in cuToktx.
	PROP,	CSCUT,	0,			0,		"integer number",	// CUTSI    34 integer (syntax) numbr, value: cuIntval, cuFlval
	PROP,	CSCUT,	0,			0,		"number",	// CUTFLOAT 35 (.) floating point constant, value in cuFlval.
	perr,	CSU,	0,			0,      "",			// 36 available
	24,		CSCMP,	PSIEQ,		PSFEQ,	"==",		// CUTEQL   37 ==  equality comparison
	24,		CSCMP,	PSINE,		PSFNE,	"!=",		// CUTNE    38 !=
	25,		CSCMP,	PSIGE,		PSFGE,	">=",		// CUTGE    39 >=
	25,		CSCMP,	PSILE,		PSFLE,	"<=",		// CUTLE    40 <=
	28,		CSBII,	PSIRSH,		0,		">>",		// CUTRSH   41 >>
	28,		CSBII,	PSILSH,		0,		"<<",		// CUTLSH   42 >>
	16,		CSLEO,	PSJZP,		0,		"&&",		// CUTLAN   43 && logical and
	15,		CSLEO,	PSJNZP,		1,		"||",		// CUTLOR   44 || logical or
// spares desirable here
	PREOF,	CSU,	0,			0,		"end of file",	// CUTEOF   45 hard end-of-file
	31,		CSBIN,	PSIADD,		PSFADD,	"+",		// CUTBPLS  46 +  used if expr determines + to be binary
	31,		CSBIN,	PSISUB,		PSFSUB,	"-",		// CUTBMIN  47 -  binary ditto
	0, 		0,		0,			0,		"",			// spare    48
	0, 		0,		0,			0,		"",			// spare    49
// identifiers already in sym tbl at toke() call (also see CUTID above):
	PRVRB,	CSU,	0,			0,		"verb",		// CUTVRB   50 verb (begins statement)
	PROP,	CSCUT,	0,			0,		"system function",	// CUTSF    51 system (pre-defined) function
	PROP,	CSCUT,	0,			0,		"system variable",	// CUTSV    52 system vbl (if implemented)
	34,		CSCUT,  0,			0,		"month",			// CUTMONTH 53 month names, cuparse.cpp
	PROP,	CSCUT,	0,			0,		"user function",	// CUTUF    54 user-defined function
	PROP,	CSCUT,	0,			0,		"user variable",	// CUTUV    55 already-declared user variable
	PREOF,	CSU,	0,			0,		"$EOF",				// CUTDEOF  56 $eof reserved word
	PROP,	CSCUT,	0,			0,		"defined",			// CUTDEF   57 defined(x) (preprocessor)
	perr, 	CSU,	0,			0,		"end comment",		// CUTECMT  58 for specific errmsg
	PRDEFA, CSU,	0,			0,		"default",			// CUTDEFA  59 default (choose(), etc)
	PRDEFA, CSU,	0,			0,		"function",			// CUTWDFCN 60 word "function"
	PRDEFA, CSU,	0,			0,		"variable",			// CUTWDVAR 61 word "variable"
	PRTY,   CSU,	TYSI,		sizeof(SI),   "integer",	// CUTINT   62  type words
	PRTY,   CSU,	TYFL,		sizeof(float),  "float",	// CUTFL    63
	PRTY,   CSU,	TYSTR,		sizeof(char *), "string",	// CUTSTR   64
	PRTY,   CSU,	TYSI,		sizeof(SI), "day of year",	// CUTDOY   65  day of year as integer 2-91
	PRVRBL, CSU,	0,			0,		"like",				// CUTLIKE  66 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"usetype",			// CUTTYPE  67 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"deftype",			// CUTDEFTY 68 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"require",			// CUTRQ    69 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"freeze",			// CUTFRZ   70 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"copy", 			// CUTCOPY  71 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"alter",			// CUTALTER  72 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"delete",			// CUTDELETE 73 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"unset",			// CUTUNSET 74 word for cul.cpp
	PRVRBL, CSU,	0,			0,		"autoSize",			// CUTAUSZ 75 word for cul.cpp 6-95
};

//----------- SYMBOL TABLE initialization data and definitions ------------*/
// see also cumain.cpp

//- flag bits for SFST, SVST
#if (defined(LOCFCN) || defined(LOCSV))
* #define LOK  16384	// ok on left of '=': can be set
* #define ROK   8192	// ok on right of '=': value can be used
#else
#define LOK  0 	// untested flag, define til tables edited 2-95
#define ROK  0 	// untested flag, define til tables edited 2-95
// 16384, 8192 available.
#endif
#define SA    4096	// has side effect: does something even on right
#define VA    2048	// variable number of arguments, .nA or more
#define MA    1024	// match args: all are type a1Ty; if TYNUM or TYANY, convert to same if types vary, else error
#define VC     512	// emit code after every arg but 1st: min, max. [Cannot be used with both LOK and ROK.]
					// VC + MA: result not arg is converted if code already emitted.
#define INC    256	// system variables: increment on load, -- on store
#define F2       2	// may make FSCHOO do select(),
#define F1		 1	// makes FSCHOO do hourval(),

/*----- Verbs: moved to caller (test prog cumain.cpp; cul.cpp) 10-8-90 */

/*----- Other reserved words in language */
// each has unique token type; st block not used at runtime.*/
struct RWST : public STBK
{
	// char* id;	// text (in base class; MUST CAST ALL USES IN VARIABLE (ERROR MSG) ARG LISTS!
	SI tokTy;		// token type, used here for symtab init.
	RWST() : STBK(), tokTy( 0) {}
	RWST( const char* _id, SI _tokTy) : STBK( _id), tokTy( _tokTy) {}
};
static RWST itRws[] =
{
	/*--id--      --tokTy-- */
	RWST( "default",  CUTDEFA),
	RWST( "function", CUTWDFCN),
	RWST( "variable", CUTWDVAR),
	RWST( "integer",  CUTINT),		// types. opp->v1 is TYSI, TYFL, etc
	RWST( "float",	  CUTFL),		//	  opp->v2 is size in bytes.
	RWST( "string",	  CUTSTR),
	RWST( "doy",	  CUTDOY), 	// (day of year for future - 1-91)
	RWST( "like",	  CUTLIKE),	// cul.cpp words ..
	RWST( "usetype",  CUTTYPE), 	// ..
	RWST( "deftype",  CUTDEFTY), 	// ..
	RWST( "require",  CUTRQ),    	// ..
	RWST( "freeze",	  CUTFRZ), 	// ..
	RWST( "copy",	  CUTCOPY),	// ..
	RWST( "alter",	  CUTALTER),	// ..
	RWST( "delete",	  CUTDELETE),	// ..
	RWST( "unset",	  CUTUNSET),	// ..
	RWST( "autoSize",	CUTAUSZ), 	// .. 6-95
	RWST( "$EOF",     CUTDEOF),
	RWST( NULL, 0)
};		// itRws

/*----- System (pre-defined) Functions */

struct SFST : public STBK	// symbol table for each function
{
//	const char* id;	// fcn name text (in base class)
	USI f;		// flag bits: [LOK, ROK,] SA, MA, VA, VC (above)
	USI evf;	// evaluation frequency, 0 if no effect
	SI cs;		// FCREG, FCCHU, FCIMPORT, or other (future) parse case
	USI resTy;	// result data type; TYNUM or TYANY = same as arg
	SI nA;		//  # arguments 0-n (minimum if f & VA)
	USI a1Ty;	//  arg 1 type. TYNUM supported for a1Ty only.
	USI a2Ty;	//  arg 2 type
	USI a3Ty;	//  arg 3 and addl args type
	SI op1;		// code to emit
	SI op2;		// alternate code: FCREG: code for float arg 1 if a1Ty TYNUM, or for left side use if right also permitted.
	//                 FCCHU: default default code.
	//                 FCIMPORT: code for field by name not number.
	// [a1Ty = TYNUM and both LOK and ROK is currently inconsistent!]
	SFST() : STBK(), f( 0), evf( 0), cs( 0), resTy( 0), nA( 0), a1Ty( 0),
		a2Ty( 0), a3Ty( 0), op1( 0), op2( 0) {}
	SFST( const char* _id, USI _f, USI _evf, SI _cs, USI _resTy, SI _nA, USI _a1Ty,
		  USI _a2Ty, USI _a3Ty, SI _op1, SI _op2)
		: STBK( _id), f( _f), evf( _evf), cs( _cs), resTy( _resTy), nA( _nA), a1Ty( _a1Ty),
		  a2Ty( _a2Ty), a3Ty( _a3Ty), op1( _op1), op2( _op2) {}
};
#define FCREG     301	// fcn cs: regular
#define FCCHU     302	//   choose( index,v1,v2,...[default v] )  type fcns
#define FCIMPORT 303	//   import( <impFile>,<fld Name or #)  type fcns 2-94
static SFST itSfs[] =
{
	//    --id--   -----f-----   evf   -cs--  resTy   #args--argTy's--    ---codes---
	SFST( "fix",         ROK,     0,   FCREG, TYSI,   1, TYNUM, 0, 0,     0, PSFIX2),
	SFST( "toFloat",     ROK,     0,   FCREG, TYFL,   1, TYNUM, 0, 0,     PSFLOAT2, 0),
	SFST( "brkt",       ROK|MA,   0,   FCREG, TYNUM,  3, TYNUM, 0, 0,     PSIBRKT,PSFBRKT),
	SFST( "min",    ROK|MA|VA|VC, 0,   FCREG, TYNUM,  1, TYNUM, 0, 0,     PSIMIN, PSFMIN),
	SFST( "max",    ROK|MA|VA|VC, 0,   FCREG, TYNUM,  1, TYNUM, 0, 0,     PSIMAX, PSFMAX),
	SFST( "abs",        ROK|MA,   0,   FCREG, TYNUM,  1, TYNUM, 0, 0,     PSIABS, PSFABS), // abs value of int or float, 10-95
	SFST( "sqrt",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFSQRT, 0),
	SFST( "exp",         ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFEXP,  0),
	SFST( "pow",         ROK,     0,   FCREG, TYFL,   2, TYFL,TYFL,0,     PSFPOW,  0),	// first arg raised to 2nd arg power 10-95
	SFST( "logE",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFLOGE, 0),
	SFST( "log10",       ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFLOG10,0),
	SFST( "sin",         ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFSIN,  0),
	SFST( "cos",         ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFCOS,  0),
	SFST( "tan",         ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFTAN,  0),
	SFST( "asin",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFASIN, 0),
	SFST( "acos",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFACOS, 0),
	SFST( "atan",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFATAN, 0),
	SFST( "atan2",       ROK,     0,   FCREG, TYFL,   2, TYFL,TYFL,0,     PSFATAN2,0),
	SFST( "sind",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFSIND,  0),
	SFST( "cosd",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFCOSD,  0),
	SFST( "tand",        ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFTAND,  0),
	SFST( "asind",       ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFASIND, 0),
	SFST( "acosd",       ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFACOSD, 0),
	SFST( "atand",       ROK,     0,   FCREG, TYFL,   1, TYFL,  0, 0,     PSFATAND, 0),
	SFST( "atan2d",      ROK,     0,   FCREG, TYFL,   2, TYFL,TYFL,0,     PSFATAN2D,0),
#if 0
// string concatenation: incomplete 2-7-2024
// initial testing appears to work
// disable pending testing and generalization (N args, use "+" operator, ...)
	SFST( "concat",	     ROK,     0,   FCREG, TYSTR,  2, TYSTR,TYSTR,0,   PSCONCAT,0),
#endif
	SFST( "wFromDbWb",   ROK,  EVFRUN, FCREG, TYFL,   2, TYFL,TYFL,0,     PSDBWB2W, 0),	// humrat from tDb, wetbulb. Uses elevation. 
	SFST( "wFromDbRh",   ROK,  EVFRUN, FCREG, TYFL,   2, TYFL,TYFL,0,     PSDBRH2W, 0),	// humrat from tDb, rel hum. Uses elevation.
	SFST( "rhFromDbW",   ROK,  EVFRUN, FCREG, TYFL,   2, TYFL,TYFL,0,     PSDBW2RH, 0),	// rel hum from tdb, w
	SFST( "enthalpy",    ROK,     0,   FCREG, TYFL,   2, TYFL,TYFL,0,     PSDBW2H,  0),	// enthalpy from drybulb, humrat
	SFST( "choose",    ROK|MA|VA, 0,   FCCHU, TYANY,  1, TYANY, 0, 0,     PSDISP, PSCHUFAI),
	SFST( "choose1",   ROK|MA|VA, 0,   FCCHU, TYANY,  1, TYANY, 0, 0,     PSDISP1,PSCHUFAI), //??
	SFST( "select", ROK|MA|VA|F2, 0,   FCCHU, TYANY,  2, TYANY, 0, 0,     0,      PSSELFAI),
	SFST( "hourval", ROK|MA|VA|F1,EVFHR,FCCHU,TYANY,  1, TYANY, 0, 0,     PSDISP, PSCHUFAI),
	SFST( "effect",     ROK|SA,   0,   FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "constant",    ROK,     0,   FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "runly",       ROK,  EVFRUN, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),	// old word kept 12-91 for poss old test programs
	SFST( "initially",   ROK,  EVFRUN, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "monthly",     ROK,  EVFMON, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "monthly_hourly",ROK, EVFMH, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "daily",      ROK,    EVFDAY, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "hourly",     ROK,     EVFHR, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "subhourly",  ROK,  EVFSUBHR, FCREG, TYANY,  1, TYANY, 0, 0,     0, 0),
	SFST( "import",     ROK,  0, FCIMPORT, TYFL,   2, TYID,TYSI|TYSTR,0, PSIMPLODNNR,PSIMPLODNNM),
	SFST( "importStr",  ROK,  0, FCIMPORT, TYSTR,  2, TYID,TYSI|TYSTR,0, PSIMPLODSNR,PSIMPLODSNM),
	SFST( "contin",     ROK,       0,   FCREG, TYFL,   4, TYFL,TYFL,TYFL,  PSCONTIN, 0),	// pwr frac = contin( mpf, mlf, sp, illum)
	SFST( "stepped",    ROK,       0,   FCREG, TYFL,   3, TYSI,TYFL,TYFL,  PSSTEPPED,0),	// pwr frac = stepped( nsteps, sp, illum)
	SFST( "fileInfo",   ROK,       0,   FCREG, TYSI,   1, TYSTR,  0, 0,    PSFILEINFO, 0),  // file info
	
#ifdef LOCFCN
	*  SFST( "abcd",   LOK|ROK|SA,   0,   FCREG, TYSI,   1, TYSI,  0, 0,     0, 0),  // poss test >>fill in codes
#endif
	SFST(),  				// terminate table
};	// itSfs

/*----- System Variables */

struct SVST : public STBK
{
	// char* id;	// in base class
	// tentative members
	USI f;		// flag bits [LOK, ROK,] INC
	USI evf;	// eval frequency
	SI cs;		// case
	USI ty;		// data type
	void *p;	// location
	SVST() : STBK(), f( 0), evf( 0), cs( 0), ty( 0), p( NULL) {}
	SVST( const char* _id, USI _f, USI _evf, SI _cs, USI _ty, void* _p)
		: STBK( _id), f( _f), evf( _evf), cs( _cs), ty( _ty), p( _p) {}
};
#define SVREG 401	// system variable regular case .cs value
static SVST itSvs[] =
{
	// --id------   -flags-  ---evf---  --cs--  -ty--   ---location---
#ifdef LOKSV
* #if 1	// 2-91: intended to let program set max error count 2-91;
* 	// probably also need to alter cul.cpp to accept sytem variables on left.
*    SVST( "$maxErrors", LOK|ROK,  EVFRUN,   SVREG,  TYSI,  &maxErrors,	// below
* #endif
*    SVST( "$runtrace",  LOK|ROK,  EVFRUN,   SVREG,  TYSI,  &runtrace, 	// in cueval.cpp
#endif
	// run phase/dates/times
	SVST( "$autoSizing",ROK,     EVFFAZ,    SVREG,  TYSI,  &Top.tp_autoSizing),	// 1 autoSizing setup/do, 0 main simulation. 6-95.
	SVST( "$dayOfYear", ROK, EVFDAY|EVFMON, SVREG,  TYSI,  &Top.jDay),    		// 1-365.		(why EVFMON?? 9-95)
	SVST( "$month",     ROK,     EVFMON,    SVREG,  TYSI,  &Top.tp_date.month),	// 1-12
	SVST( "$dayOfMonth",ROK,     EVFDAY,    SVREG,  TYSI,  &Top.tp_date.mday),		// 1-31
	SVST( "$dayOfWeek", ROK|INC, EVFDAY,    SVREG,  TYSI,  &Top.tp_date.wday),		// inc'd: 1-7
	SVST( "$dsDay",     ROK,     EVFDAY,    SVREG,  TYSI,  &Top.tp_dsDay),		// 0 main sim, 1 heat autoSize, 2 cool as. 6-95.
	SVST( "$dowh",      ROK|INC, EVFDAY,    SVREG,  TYSI,  &Top.dowh), 			// inc'd. main sim: holiday 8, else day of week 1-7.
	//   autoSizing: heating 9, cooling 10. */
	SVST( "$hour",      ROK|INC, EVFHR,     SVREG,  TYSI,  &Top.iHr),			// inc'd: 1-24.  iHr also used for hourval()
	SVST( "$hourST",    ROK|INC, EVFHR,     SVREG,  TYSI,  &Top.iHrST),			// inc'd: 1-24
	SVST( "$subhour",   ROK|INC, EVFSUBHR,  SVREG,  TYSI,  &Top.iSubhr),		// inc'd: 1-N (number of subhours)
	SVST( "$isDT",      ROK,  EVFHR|EVFDAY, SVREG,  TYSI,  &Top.isDT),			// 1 if Daylight Saving time is in effect.  EVFDAY so can't make EVFMH with EVFHR, 9-95.
	//SVST( "$year",      ROK,     EVFMON,    SVREG,  TYSI,  &Top.date.year),	// < 0: generic
	//SVST( "$firstMon",  ROK,     EVFRUN,    SVREG,  TYSI,  &FirstMon),	// of run, 1-12
	//SVST( "$lastMon",   ROK,     EVFRUN,    SVREG,  TYSI,  &LastMon), 	// of run, 1-12
	// day characteristics
	SVST( "$isHoliday", ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isHoliday),	// TRUE on observed hday (Monday after true date for some)
	SVST( "$isHoliTrue",ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isHoliTrue),	// TRUE (non-0) on true date of holiday
	SVST( "$isWeHol",	ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isWeHol),	// Weekend or (observed) holiday
	SVST( "$isWeekend", ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isWeekend),	// Saturday or Sunday
	SVST( "$isWeekday", ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isWeekday),	// Mon-Fri
	SVST( "$isBegWeek", ROK,     EVFDAY,    SVREG,  TYSI,  &Top.isBegWeek),	// weekday after weekend or holiday
	SVST( "$isWorkDay",     ROK,  EVFDAY,  SVREG,  TYSI,  &Top.isWorkDay), 	// per Top.workDayMask. default: non-holiday Mon-Fri.
	SVST( "$isNonWorkDay",  ROK,  EVFDAY,  SVREG,  TYSI,  &Top.isNonWorkDay),	// default: Sat,Sun, observed Holidays.
	SVST( "$isBegWorkWeek", ROK,  EVFDAY,  SVREG,  TYSI,  &Top.isBegWorkWeek),	// workDay after nonWorkDay
	// current weather.			|EVFDAY's so cleanEvf won't combine with EVFMON to form EVFMH, 9-95.
	SVST( "$radBeam",     ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.radBeamHrAv),	// beam irrad, Btu/ft2===Btuh/ft2, hour integ (historical)
#ifdef SOLAVNEND // cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
o    //SVST( "$radBeamHr", ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.radBeamHr),	// .. hour end instantaneous power, Btuh/ft2: not impl 1-95
#endif
	SVST( "$radBeamShAv", ROK,   EVFSUBHR,  SVREG,  TYFL,  &Top.radBeamShAv),	// .. subhour average power Btuh/ft2
#ifdef SOLAVNEND // cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
o    SVST( "$radBeamSh",   ROK,   EVFSUBHR,  SVREG,  TYFL,  &Top.radBeamSh),	// .. subhour end interp instan power, Btuh/ft2
#endif
	SVST( "$radDiff",     ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.radDiffHrAv),  // diffuse irrad, Btu/ft2, hour integral (historical)
#ifdef SOLAVNEND
o    //"$radDiffHr", ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.radDiffHr,	// .. hour end instantaneous power, Btuh/ft2: not impl 1-95
#endif
	SVST( "$radDiffShAv", ROK,   EVFSUBHR,  SVREG,  TYFL,  &Top.radDiffShAv),  // .. subhour average power Btuh/ft2
#ifdef SOLAVNEND // cndefns.h (via cnglob.h) 1-18-94: only if computing & using end-ivl as well as ivl avg solar values
o    SVST( "$radDiffSh",   ROK,   EVFSUBHR,  SVREG,  TYFL,  &Top.radDiffSh,	// .. subhour end interp instan power, Btuh/ft2
#endif
	SVST( "$tDbO",        ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.tDbOHr),     	// outdoor drybulb temp, F, hour-end (historical)
	SVST( "$tDbOHrAv",    ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.tDbOHrAv),    	// .. hour average 1-95
	SVST( "$tDbOSh",      ROK,   EVFSUBHR,   SVREG, TYFL,  &Top.tDbOSh),     	// .. subhour (end) 1-95
	SVST( "$tWbO",        ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.tWbOHr),     	// outdoor wetbulb temp, hour-end (historical)
	SVST( "$tWbOHrAv",    ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.tWbOHrAv),    	// .. hour average 1-95
	SVST( "$tWbOSh",      ROK,   EVFSUBHR,   SVREG, TYFL,  &Top.tWbOSh),     	// .. subhour (end) 1-95
	SVST( "$wO",          ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.wOHr),    		// humidity ratio, hour-end (historical)
	SVST( "$wOHrAv",      ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.wOHrAv),    	// .. hour average 1-95
	SVST( "$wOSh",        ROK,   EVFSUBHR,   SVREG, TYFL,  &Top.wOSh),    		// .. subhour (end) 1-95
	SVST( "$windSpeed",   ROK, EVFHR|EVFDAY, SVREG, TYFL,  &Top.windSpeedHr), 	// wind speed, mph, end hour (?)
	SVST( "$windSpeedHrAv",ROK,EVFHR|EVFDAY, SVREG, TYFL,  &Top.windSpeedHrAv),	// .. hour average, 1-95
	SVST( "$windSpeedSh", ROK,   EVFSUBHR,   SVREG, TYFL,  &Top.windSpeedSh), 	// .. subhour (end), interpolated, 1-95
	SVST( "$windDirDeg", ROK,  EVFHR|EVFDAY, SVREG, TYFL,  &Top.windDirDegHr),	// wind direction, compass degrees, end hour (?)
#ifdef LOKSV
*   SVST( "$sink",      LOK,     0,         SVREG,  TYFL,  NULL),	/*>> supply test locs*/
*   SVST( "$source",    ROK,     0,         SVREG,  TYFL,  NULL),
*   SVST( "$both",      LOK|ROK, 0,         SVREG,  TYFL,  NULL),
#endif
	SVST(),
};		/* itSvs */

/*----- Months: implemented as unary operators, day of month expr follows */

struct MOST : public STBK
{
// char* id;   	// month name
SI nDays;   	// # days in month
SI day0;		// 1st DOY of month, 0-based.
MOST() : STBK(), nDays( 0), day0( 0) {}
	MOST( const char* _id, SI _nDays, SI _day0) : STBK( _id), nDays( _nDays), day0( _day0) {}
};
static MOST itMos[] =
{
	MOST( "jan", 31, 0),
	MOST( "feb", 28, 31),
	MOST( "mar", 31, 59),
	MOST( "apr", 30, 90),
	MOST( "may", 31, 120),
	MOST( "jun", 30, 151),
	MOST( "jul", 31, 181),
	MOST( "aug", 31, 212),
	MOST( "sep", 30, 243),
	MOST( "oct", 31, 273),
	MOST( "nov", 30, 304),
	MOST( "dec", 31, 334),
	MOST()
};

/*----- User-defined Functions */

struct UFARG : public STBK		// arguments info subStruct, for UFST and global fArg[].
{
	// char* id in base class
	USI ty;		// arg type: TYSI, TYFL, TYSTR, etc
	USI sz;		// arg size, bytes
	USI off;		// runtime stack frame offset, words: 2 (ret addr) + 2 (frame ptr) + sum( following arg word sz's)
	UFARG() : STBK(), ty( 0), sz( 0), off( 0) {}
};
struct UFST	: public STBK	// user-defined function symbol table entry
{
	// char* id in base class
	USI ty;		// return type (TYSI, TYFL, etc)
	USI sz;		// return size, bytes
	USI evf;		// evaluation frequency. Merge w evf of actual params!
	SI did;		// non-0 if has side effects
	PSOP *ip;		// code location (dm block)
	SI nA;		// number of arguments
	UFARG arg[1];	// arguments info array (actual size in dm varies)
	UFST() : STBK(), ty( 0), sz( 0), evf( 0), did( 0), ip( NULL), nA( 0) { }
};

/*----- User-defined Variables */

struct UVST : public STBK	// user-defined variable symbol table entry */
{
	// char* id in base class
	/* members to be defined */
};

/*----- Table of symbol table initialization tables */

// this data is used to put symbols in above tables into symtab at startup by addLocalSyms() (in this file)

/* external modules may also put data in symbol table by calling cuAddItSyms().
   Its args are same as a row in this table. */

/* .casi = 1 is used for reserved words to cause symtab entry to be flagged case-insensitive,
   so they will be matched case-insensitive even on possible case-sensitive searchs (for variable names) */

static struct ITSYMS
	{		SI tokTy;   SI casi;  STBK* tbl;  USI entlen; }
itSyms[] =
{
	CUTSF,      1,       itSfs,      sizeof( SFST),
	CUTSV,      1,       itSvs,      sizeof( SVST),
	CUTMONTH,   1,       itMos,	  sizeof( MOST),
	-1, /* <---get tokTy from 2nd member of block */
	1,       itRws,      sizeof( RWST),
	0,
};

/*--------- finally, the SYMBOL TABLE */

LOCAL SYTBH symtab = { NULL, 0 };
						// structure in sytb.h.  table is init'd by addLocalSyms() and cuAddItSyms(),
						// using data in itVrbs[], itSfs[], itMos[], and itSvs[],
						// and added to as user defines (future) functions and variables.
						// CAUTION: table has mixed near and far id text pointers in entries.

 /*------------------------------- COMMENTS --------------------------------*/

 /*--- prec and ty:

 "prec": single global: relates to current token (being processed)
 	value gives precedence of operators (ie prec(*) > prec(+));
 	used some places to classify tokens.
 	operands have hiest prec's; terminators/separators have lowest.
  also: lastPrec: prior token's "prec" BUT NOT AFTER unToke();
        nextPrec: next (ie ungotten) token's prec ONLY after unToke/expr/expTy.

 "ty":   member of parStk frame -- one copy per pending (sub)expression.
 	gives data type if "prec" says a value is present (prec > PROP);
 	  used to in "expr()" to determine if anything at all parsed
 */

					 /*------------------------ mostly LOCAL VARIABLES -------------------------*/
// also see: symbol table, above.
// many tentatively decl in cuparsex.h for [cumain.cpp 10-90 and now] cuprobe.cpp 12-91.

 /*--- CURRENT TOKEN INFO.  Set mainly by toke().  Not changed by unToke(). */
 SI tokTy = 0;   		// current token type (CUT__ define; cuTok ret val)
 SI prec = 0;    		// "prec" (precedence) from opTbl[]. PR__ defines.
 SI nextPrec = 0;		// "prec" of ungotten (ie next) token, ONLY valid after expTy()/expr()/unToke().
 LOCAL SI lastPrec = 0;	// "prec" of PRIOR token (0 at bof) NOT after unToke.
							//opp,ttTx,stbk,isWord NOT changed by unToke() or re-Toke(). don't alter!
 LOCAL OPTBL * opp = NULL;	// ptr to opTbl entry for token
 char * ttTx = NULL;	// saveable ptr to static token descriptive text (opp->tx) for errMsgs. cul.cpp uses.
 LOCAL void * stbk = NULL; 	// symbol table value ptr, set by toke() for already-decl identifiers, type varies...
 SI isWord = 0;     	// nz if word: undef (CUTID), user-def (CUTUF, CUTUV), or reserved (CUTVRB CUTSF etc).

 /*--- CURRENT EXPRESSION INFO, exOrk to expr and callees. */
 USI evfOk = 0xffff;	// evaluation frequencies allowed bits for current expression, ffff-->no limits.
							// EVENDIVL/EVPSTIVL stage bits here means end/post-of-interval evaluation ok.
							// also ref'd in cuprobe.cpp.
 const char* ermTx = NULL;	// NULL or word/phrase/name descriptive of entire expression, for insertion in msgs.
 LOCAL USI choiDt = 0;	// choice type (dtypes.h) for TYCH (incl TYNC): specifies ok choices & their conversions.
 // DTBCHOICB and DTBCHOICN bits determine whether 2-byte or 4-byte choice values are generated.
 // enables TYCH bit wherever TYANY wanTy is generated.
 // CAUTION: must be saved/restored if/wherever another nested choice type expr is evaluated.

 /*--- DUMMY ARGUMENTS of fcn being compiled, funcDef to dumVar */
 LOCAL SI nFa = 0;			// 0 if none or current # dummy args
 LOCAL UFARG *fArg = NULL;	// arg info array (see UFST above): points into funcDef's stack.

 /*--- PARSE STACK: one entry ("frame") for each subexpression being processed.
   When argument subexpressions to an operator have been completely parsed and type checks and conversions are done,
   their frames are merged with each other and the frame for the preceding code (and the operator's code is emitted).
   parSp points to top entry, containing current values. */
 PARSTK parStk[ 500] = { 0 };	// parse stack buffer
 PARSTK* parSp = parStk;	// parse stack stack pointer

 /*--- CODE OUTPUT variables */
 LOCAL PSOP* psp = NULL;  	// set by itPile(), used by emit() &c.
 LOCAL PSOP* psp0 = NULL;	// initial psp value, for computing length
 LOCAL PSOP* pspMax = NULL;// ptr to end of code buf less *6* bytes (6= one float or ptr + 1 PSEND) for checking in emit() &c.
 LOCAL SI psFull = 0;		// non-0 if *psp has overflowed
 constexpr int PSSZ = 4000; // max # words of pseudo-code for fcn or expr.
							//   for expr, 40 usually enuf, but 1000 has overflowed.

 /*--- re Parse Error Messages ---*/
// 1 when parsing fcn defn arg list: says a type word is NOT start new statement until a ')' has been passed:
 LOCAL SI inFlist = 0;	// set in funcDef, tested in perNx

 /*--- compiler debug aid display flags */
 int trace = 0;				// nz for many printf's during compile.
							//  must be set via temporary code or debugger
 static int kztrace = 0;	// nz for cryptic konstizing printf's in konstize(), cnvPrevSf() 10-90

 /*--- error count limit: caller may stop compile when exceeded;
                          if no compile errors, run may stop if run errors exceed it. */
 SI maxErrors = 4; 	// used in cul.cpp, exman.cpp, also $maxErrors above. init'd by cuParseClean below.
 // 4 is a good value to keep 1st msg on screen. 20 (2-91) is to get more msgs per run while testing error cases.


 /*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC   FC funcDef( OPTBL *oppTy);
LOCAL RC      expr( SI toprec, USI wanTy, const char* tx, SI aN);
LOCAL RC   FC monOp( MOST *most);
LOCAL RC   FC unOp( SI toprec, USI argTy, USI wanTy, PSOP opSi, PSOP opFl, char *tx);
LOCAL RC   FC biOp( SI toprec, USI argTy, USI wanTy, PSOP opSi, PSOP opFl, char *tx);
LOCAL RC   FC condExpr( USI wanTy);
#ifdef LOKFCN
* LOCAL RC   FC fcn( SFST *f, SI toprec, USI wanTy);
#else
LOCAL RC   FC fcn( SFST *f, USI wanTy);
#endif
LOCAL RC  FC fcnImport( SFST *f);
#if defined(LOKFCN)
* LOCAL RC   FC fcnReg( SFST *f, SI toprec, USI wanTy);
#else
LOCAL RC   FC fcnReg( SFST *f, USI wanTy);
#endif
LOCAL RC   FC fcnChoose( SFST *f, USI wanTy);
LOCAL RC   FC fcnArgs( SFST *f, SI nA0, USI wanTy, USI optn, USI *pa1Ty, SI *pnA, SI *pDefa, SI *pisKon, SI *pv);
#if defined(LOKFCN) || defined(LOCSV)
LOCAL RC   FC emiFcn( SFST *f, SI onLeft, USI a1Ty);
#else
LOCAL RC   FC emiFcn( SFST *f, USI a1Ty);
#endif
LOCAL RC   FC emiDisp( SFST *f, SI nA, SI defa);
#ifdef LOKSV
* LOCAL RC   FC sysVar( SVST *v, SI toprec, USI wanTy);
#else
LOCAL RC   FC sysVar( SVST *v, USI wanTy);
#endif
LOCAL RC   FC uFcn( UFST *stb );
LOCAL SI   FC dumVar( SI toprec, USI wanTy, RC *prc);
LOCAL RC   FC var( UVST *v, USI wanTy);
LOCAL RC   FC expSi( SI toprec, SI *pisKon, SI *pv, const char *tx, SI aN);
LOCAL RC   FC convSi( SI* pisKon, SI *pv, SI b4, const char *tx, SI aN);
LOCAL RC   FC tconv( SI n, USI *pWanTy);
LOCAL RC   FC utconvN( SI n, char *tx, SI aN);
LOCAL SI   FC isKonExp( void **ppv);
LOCAL USI  FC cleanEvf( USI evf, USI _evfOk);
LOCAL RC   FC cnvPrevSf( SI n, PSOP op1, PSOP op2);
LOCAL RC   FC movPsLastN( SI nFrames, SI nPsc);
LOCAL RC   FC movPs( PARSTK *p, SI nPsc);
LOCAL RC   FC fillJmp( SI i);
LOCAL RC   FC dropJmpIf( void);
LOCAL SI   FC isJmp( PSOP op);
LOCAL RC   FC emiLod( USI ty, void *p);
#ifdef LOKFCN
LOCAL RC   FC emiSto( SI dup1st, void *p);
#endif
LOCAL RC   FC emiDup( void);
LOCAL RC   FC emiPop( void);
LOCAL RC   FC emit4( void **p);
LOCAL RC   FC emitPtr( void** p);
#if defined( USE_PSPKONN)
LOCAL RC   FC emitStr(const char* s, int sLen);
#endif
LOCAL RC   FC emiBufFull( void);
LOCAL SI   FC tokeTest( SI tokTyPar);
LOCAL SI   FC tokeIf2( SI tokTy1, SI tokTy2);
LOCAL SI CDEC tokeIfn( SI tokTy1, ... );
LOCAL void FC cuptokeClean( CLEANCASE cs);
LOCAL RC   FC addLocalSyms( void);
LOCAL const char* FC before( const char* tx, int aN);
LOCAL const char* FC after( const char* tx, int aN);
LOCAL const char* FC asArg( const char* tx, int aN);
LOCAL const char* FC datyTx( USI ty);
LOCAL RC FC perI( int showTx, int showFnLn, int isWarn, MSGORHANDLE ms, va_list ap);


//===========================================================================
void FC cuParseClean( 		// cuParse overall init/cleanup routine

	CLEANCASE cs )	// [STARTUP,] ENTRY, DONE, or CRASH [,or CLOSEDOWN]. type in cnglob.h.

// expect to call from cul.cpp
{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */
	/* completed initial analysis, coding cleanup fcn, calling cleanup fcn in:
	    cutok.cpp 10-93, all ppxxx files, 10-93 */

	/* pseudo-code cleanup (exman?): PSKON4 can be followed by dm ptr, or not; believe unused. */


// clean toke/untoke/symbol table, below
	cuptokeClean(cs);

// local cleanup
	maxErrors = 4;		// undo any $maxErrors set.
	//parSp = parStk		is done in itPile().
	//psp  points to caller-supplied buffer, don't clean up here.
	tokTy = prec = nextPrec = lastPrec = 0;
	choiDt = 0;	// insurance. believed redundant. 12-31-93.

// clean cutok.cpp and preprocessor
	cuTokClean(cs);			// clean up cutok.cpp; also cleans up ppxxx files.
}			// cuParseClean

// thing to call when caller (cul.cpp etc) sees type word:
//===========================================================================
RC FC fov()		// compile function or variable definition

/* call after cuTok'ing type word (precedence PRTY: INTEGER, FLOAT, DOY, etc.)
   in context where function or variable definition acceptatble */

/* syntaxes:  <type> FUNCTION ... (see funcDef, next)
	      <type> VARIABLE <name>;	(future) */
{
	OPTBL *oppTy;

	oppTy = opp;	// save optbl ptr for type (for code/size)
	if (tokeTest(CUTWDFCN))		// if word "function" next
		return funcDef( oppTy);		// compile function definition
	//else if (tokTy==CUTWDVAR)  	// if "variable" next
	//   	// future variable declaration code here
	else
		return perNx( MH_S0001); 	// "Syntax error.  'FUNCTION' or 'VARIABLE' expected"
}			// fov
//===========================================================================
LOCAL RC FC funcDef( 	// compile function definition

	OPTBL *oppTy )	/* operator table entry pointer (opp) for type word: contains type code and size */

// call after seeing "<type> FUNCTION"

/* syntax, for now 12-90:
	<type> FUNCTION <name> ( <arg decls> ) = <expr> ;
   ?? future add alternative:
	<type> FUNCTION <name> ( <arg decls> );
	   <statements>
	   RETURN <expr>;
	ENDFUNCTION [<name>]; */
{
#define MAXNARG 100		// max # args for fcn (temp storage size)
	PSOP ps[PSSZ];			// holds fcn pseudo-code during compilation
	USI codeSize;

	UFST stb;  			// holds symbol table info til moved to dm
	UFARG arg[MAXNARG];	// args info while determining # args
	SI aN;				// argument Number (counter)
	USI off;			// cumulative argument offset
	UFST *p;   RC rc;

	SI nFaSave;   UFARG *fArgSave;

	/* start setting temp symbol table block in stack */
	stb.ty = oppTy->v1;					// function's type code (TYSI etc)
	stb.sz = oppTy->v2;					// return value size in bytes

	/* function name */
	if (tokeNot(CUTID))  				// if not undeclared unreserved identifier nxt
		return isWord					// non-0 if a word even if reserved
		?  perNx( MH_S0002,  		// "%s '%s' cannot be used as function name"
		tokTy==CUTUF || tokTy==CUTUV
		?  "previously defined name"
		:  "reserved word",
		cuToktx )
			:  perNx( MH_S0003 );   		// "Expected identifier (name for function)"
	// make errors here fatal when multi-line fcns accepted??
	stb.id = strsave(cuToktx);

	/* dummy argument list: types and names */
	aN = 0;					// arg counter
	if (tokeNot(CUTLPR))			// if not '(' next
	{
		if (tokTy==CUTEQ)			// if correct thing to follow arg list
			return perNx(MH_S0004);  	/* "'(' required. \n"
				                   "    If function has no arguments, empty () must nevertheless be present." */
		return perNx( MH_S0005);   	// "Syntax error: '(' expected"
	}
	inFlist = 1;			// say between ()'s of arg list: tell perNx() not to resume at type word b4 closing )
	if (tokeIf(CUTRPR)==0)			// unless ) next: empty argList. passes the ).
	{
		do						// args loop head
		{
			if (aN >= MAXNARG)
				return perNx( MH_S0006);   	// "Too many arguments"

			// type word
			toke();
			if (prec != PRTY)
				return perNx( MH_S0007 );   	// "Syntax error: expected a type such as 'FLOAT' or 'INTEGER'"
			arg[aN].ty = opp->v1;				// argument type code (TYSI etc)
			arg[aN].sz = opp->v2;				// size in bytes

			// argument identifier (dummy name)
			if (tokeNot(CUTID))				// get token / if not identifier
				return isWord
				?  perNx( MH_S0008,   		// "%s '%s' cannot be used as argument name"
				tokTy==CUTUF || tokTy==CUTUV
				?  "previously defined name"
				:  "reserved word",
				cuToktx )
					:  perNx( MH_S0009);      	// "Expected identifier (name for argument)"
			arg[aN].id = strsave(cuToktx);		// name to dm / ptr to struct
			aN++;						// count args
		}
		while (tokeTest(CUTCOM));			// args loop endtest

		if (tokTy != CUTRPR)
			return perNx( MH_S0010);   		// "Syntax error: ',' or ')' expected"
	}
	stb.nA = aN;					// store # args
	inFlist = 0;			// tell perNx not parsing arg list: type word = start of statement = recovery point

	/* runtime stack frame layout 12-90:
	  stack ptr --> return value at return
			... fcn's temp storage, expect all gone at return
	  frame ptr --> prior frame ptr (4 bytes)
			return address (4 bytes)
			last arg
			...
			first arg.  <-- ret val moved here by PSRETA */

	/* compute arg offsets in stack frame: used in compiling arg refs in fcn body (see dumVar) */
	off = 0;
	for (aN = stb.nA; --aN >= 0; )
	{
		arg[aN].off = off + ((4+4)/sizeof(SI));		// + fp and ret addr words
		off += arg[aN].sz / sizeof(SI);			// cumulate arg size in words
	}
	// 'off' is now # words to pop on return. used below.

	/* get separating = */
	if (tokeNot(CUTEQ))
		return perNx( MH_S0011 );    		// "'=' expected"

	/* init to compile code */
	CSE_E( itPile( ps, sizeof(ps) ) )		// below. Put code in stack for now.  CAUTION defaults evfOk, ermTx.

	/* compile expression of fcn's type, using fcn's dummy arguments */

	nFaSave = nFa;  fArgSave = fArg;		/* save re poss future nested fcns (to make all nested fcn args avail,
	    					   put .fArg in stb, have dumVar() search chain to NULL) */
	nFa = stb.nA;				// set global dummy arg flag/count ...
	fArg = arg;					// and info array ptr for use while compiling expr (see dumVar())

	rc = expTy( PRCOM, stb.ty, stb.id, 0);	// get expr (below)

	nFa = nFaSave ;  fArg = fArgSave;		// restore even after error!
	if (rc)					// now return if error
		return rc;

	/* side effect flag.  Known "limitation" (ie bug): set if just changes arg, even tho out of scope at return. */
	stb.did = parSp->did;			// nz if does something (eg nested assign)

	/* evaluation frequency.  Or'd with evf's of actual args at call.
	     Clear "contains dummy arg ref": not propogated into calling expr
	     as purpose is to prevent evaluation w/o call (konstize). */
	stb.evf = parSp->evf &~EVFDUMMY;

	/* terminator */
	tokeIf2( CUTSEM, CUTCOM);				/* if terminator is ';' or ?comma, pass it.
		    					   Accept but do not pass (next) verb or type (or ')' or ']' ) */
	/* emit return code */
	CSE_E( emit2( PSRETA) )  				// pop words and return absolute
	CSE_E( emit2( off) )					// # (arg) words to discard
	CSE_E( emit2( stb.sz / sizeof(SI) ) )			// # return value words to keep (move)

	/* code to dm */
	CSE_E( finPile( &codeSize) )				// terminate compilation / get size
	CSE_E( dmal( DMPP( stb.ip), codeSize, WRN) )		// alloc dm space, dmpak.cpp
	memcpy( stb.ip, ps, codeSize);   			// put pseudocode in the space

	/* allocate and fill symbol table block in dm */
	CSE_E( dmal( DMPP( p),    					/* alloc heap space, dmpak.cpp */
		sizeof(UFST) + (stb.nA - 1)*sizeof( UFARG),
		WRN) )
	memcpy( p, &stb, sizeof(UFST)-sizeof( UFARG) );	// basic info
	memcpy( p->arg, arg, stb.nA * sizeof( UFARG) );	// arg info

	/* add to symbol table */
	CSE_E( syAdd( &symtab, CUTUF, TRUE /* case insens */, p, 0) )  	// sytb.cpp. changed to case-insens 11-94.

	return RCOK;					// also many error returns above.
	// note dm string storage for id's is NOT (yet) recovered on error returns.
#undef MAXNARG
}	// funcDef
//==========================================================================
LOCAL BOO funcDel(   	// clean up and free a user function symbol table block
	// callback function for use with syClear

	SI _totTy, 		// token type (syClear passes; only CUTUF expected
	UFST** pStb )	// pointer to pointer to the block

// returns TRUE to say "do delete the symbol table entry"
{
	UFST* stb = *pStb;
	if (!stb)  return TRUE;		// if NULL pointer (unexpected) say go ahead and delete symbol table block
	if (_totTy != CUTUF)  return FALSE;	// if not a user function, this fcn should not have been called. bug.

	dmfree( DMPP( stb->id));		// free function name string
	dmfree( DMPP( stb->ip));		// free function pseudo-code block
	for (SI i=0; i < stb->nA; i++)	// for each argument
		dmfree( DMPP( stb->arg[i].id));	// free argument name string
	dmfree( DMPP( stb));		// finally, free the function block. NULLs caller's pointer via ref arg.

	return TRUE;			// say do delete the symbol table entry that contained this block
}			// funcDel
//==========================================================================
LOCAL BOO funcOrVarDel( 	// clean & free a user function or variable symbol table block, leave others
	// callback function for use with syClear

	SI _totTy, 		// token type
	STBK *&stb )		// ref to pointer to the block

// returns TRUE for deleted blocks only
{
	switch (_totTy)
	{
	case CUTUF:
		return funcDel( _totTy, (UFST **)&stb); 	// user fcn: call fcn just above
		//case CUTUV:		add code for user variable: not implemented as of 10-93.
	default:
		return FALSE;				// other type block (reserved word): do not delete
	}
}			// funcOrVarDel
//==========================================================================
RC FC funcsVarsClear()	// remove all user functions and (future) variables from symbol table.

// reserved word entries are not disturbed.  for use at CLEAR command.
{
	syClear( &symtab, -1, funcOrVarDel);   	/* call "funcOrVarDel" just above for each symbol table entry.
	    					   returns FALSE to retain entries of other types. syClear: sytb.cpp. */
	return RCOK;
}		// funcsVarsClear

// thing to call when expression needed externally:
//==========================================================================
RC FC exOrk(	// compile expression from current input file, return constant value or pseudo-code

	SI toprec,			// precedence to which to parse, or -1 to default
	USI wanTy, 			// desired type (cuparse.h) TYSI TYINT TYFL TYSTR TYFLSTR TYSTR|TYSI TYID (rets TYSTR) TYID|TYSI
		                //    TYCH (2 or 4 byte) TYNC (rets TYFL / TYCH / TYNC=runtime determined)
	USI choiDtPar,		// DTxxx data type (dtype.h) when wanTy is TYCH or TYNC, else not used.
	USI evfOkPar,  		// 0xffff or acceptable eval freq (and EVxxxIVL) bits: other evf's error
	const char *ermTxPar, 	// NULL or text describing preceding verb etc for err msgs
	USI *pGotTy,		// NULL or receives data type found (useful eg when TYFLSTR requested)
	USI *pEvf,			// receives cleaned evaluation frequency bits
	SI *pisKon,			// receives non-0 if expression is constant
	NANDAT* pvPar,		// if constant, rcvs value, <= 4 bytes, else not changed -- caller may pre-store nandle.
   						//   RECEIVES 4 BYTES even for SI or 2-byte choice value (caller truncates).
	PSOP **pip )		// rcvs ptr to code in dm (if not constant) else NULL

// also, on return, tokty reflects terminating token.
//   ; , ) ] terminators passed; verb or other ungotten.

// design intended to facilitate making parStk, psp, etc internal to this file
// while leaving EXTAB internal to calling file. */

// if not RCOK, attempts to return functional constant zero or null value.
{
#define Eer(f)  { rc = (f); if (rc!=RCOK) goto er; }	// local err handler

	RC rc = RCOK;

// init
	PSOP ps[PSSZ];					// holds pseudo-code during compilation
	Eer( itPile( ps, sizeof(ps) ) )	// init to compile, below. code to stack for now. CAUTION dflts evfOk, ermTx, choiDt.
	choiDt = choiDtPar;   		// store choice type (for acceptable words/conversion) if TYCH/TYNC
	evfOk = evfOkPar;			// acceptable evf and eval-at-end-ivl bits to global for expr()
	ermTx = ermTxPar;			// description for msgs to global for expr()
	PSOP* ip{ nullptr };		// init to no code to return (if constant, or error)
	if (toprec < 0)			// if no terminator precedence given
		toprec = PRCOM;			// terminate on  ,  ;  )  ]  }  verb  eof.

// compile expression
	USI gotTy{ wanTy };		// init vars in case expTy returns error
	USI gotEvf{ 0 };
	Eer( expTy( toprec, wanTy, ermTx, 0) )	// compile expr of type 'wanTy'. Parse to token of precedence <= 'toprec'.
				    						// Ungets terminating token.  Below.  Errors go to 'er:' (expTy issues msgs).
	gotTy = parSp->ty;		// type found

// selectively pass terminator.  Unget verb, type, other.  Leave tokTy set.
	tokeIfn( CUTCOM, CUTSEM, CUTRPR, CUTRB, 0);		// pass , ; ) ].

// determine if constant
	void* pv;
	SI isKon{ 0 };
	Eer( konstize( &isKon, &pv, 0 ) )	// evals if evaluable and un-eval'd. Rets flag and ptr (ptr to ptr for TYSTR). below.
	USI codeSize;
	Eer( finPile( &codeSize) )		// now terminate compilation / get size

// for constant, return value and no code
	NANDAT v = 0;
	if (isKon)   				// if konstize found (or made) a constant value
	{
		// fetch from konstize's storage, condition value
		if (gotTy == TYSTR)				// pv points to ptr to text
		{
			CULSTR sv = *(const char**)pv;	// convert to CULSTR (copies)
			sv.IsValid();					// msg if invalid
			v = AsNANDAT(sv);
		}
		else if (gotTy==TYSI		// SI (short int)
		  || (gotTy==TYCH && (choiDt & DTBCHOICB) && !ISNCHOICE(*(void**)pv)))	// choice, 2-byte ch req'd, didn't get 4-byte ch
																				// (redundancy: distrust choiDt global)
		{
			USI iV = *(USI*)pv;
			v = iV;	// make hi word 0 (not a nan!)
		}
		else
		{	UINT iV = *(UINT*)pv;		// fetch float, 4-byte choice, etc value
			v = iV;
		}
		//ip = NULL;					// NULL to return in pseudo-code pointer  set above
		//gotEvf = 0;					init at entry
	}

// not constant: store and return pseudo-code
	else	// isKon==0
	{
		Eer( dmal( DMPP( ip), codeSize, WRN) )   	// alloc dm space, dmpak.cpp
		memcpy( ip, ps, codeSize);			// put pseudocode in the space
		// ip returned in *pip below.
		// *pvPar left unchanged (caller may preset to nandle)
		gotEvf = cleanEvf( parSp->evf, evfOk);	// clear redundant bits,
												// convert MON & HR & !DAY to/from MH per context. below.
#if 1 // 11-25-95 making fcns work: Add here cuz removed in cleanEvf. Also changes in expr, evfTx.
		if (gotEvf & EVFDUMMY)				// 'contains dummy arg' bit should not get here (cleared in funcDef).
			perlc( "Internal error in cuparse.cpp:exOrk: Unexpected 'EVFDUMMY' flag");
#endif
	}

// on any error return constant 0 in case program continues (it doesn't; could delete this, 3-92)
	if (rc != RCOK)
	{
er:    // Eer macro comes here on non-RCOK fcn return
		isKon = 1;  	// say is constant (ret *pisKon)
		v = 0;			// return 0 for numeric types
						// for CULSTR, 0 = ""
		// ip = NULL preset above.  gotEvf=0 init.
	}

// common info return -- or not where NULL pointers given
	if (pGotTy)
		*pGotTy = gotTy;			// return actual type gotten; useful eg if TYFLSTR requested.
	if (pEvf)				// evaluation frequency bits return
		*pEvf = gotEvf;			// 0, or parSp->evf cleaned above
	if (pisKon)
		*pisKon = isKon;      		// non-0 if already-evaluated constant
	if (pip)
		*pip = ip;			// NULL or pointer to pseudo-code
	if (isKon)				// *pvPar left unchanged if non-constant
		if (pvPar)
			*pvPar = v;
	return rc;

#undef Eer
}		// exOrk

// possibly LOCAL if all entry thru exOrk, 10-90
//==========================================================================
RC FC itPile( PSOP *dest, USI sizeofDest)

// initialize to compile

// call for each separately-stored expression, statement, program, etc
{
// init to parse
	parSp = parStk - 1;		// init global parse stack pointer
	evfOk = 0xffff;		// in case entry not thru exOrk (possible?)
	ermTx = NULL;  		// ditto
	choiDt = 0;   		// ditto
	nFa = 0;			// no dummy args: not compiling a fcn

// init to emit code
	psp = dest;  			// init global pseudo-code emit ptr
	psp0 = dest;			// save initial psp value
	pspMax = (PSOP *)((char *)dest +sizeofDest -sizeof(float) -sizeof(PSOP) );
	// set emission limit for emit()
	psFull = 0;  			// buffer *psp has not overflowed yet

// init re error messages (insurance)
	inFlist = 0;	/* tell perNx NOT parsing fcn definition arg list
			   (containing type for dummy args):
			   type word = start of statement = recovery point */
	return RCOK;
}		/* itPile */

// possibly LOCAL if all entry thru exOrk, 10-90
//==========================================================================
RC FC finPile( USI *pCodeSize)

// finish compile -- terminate output code, etc
{
	RC rc;

// terminate code and DO point past terminator at end program
	rc = emit(PSEND);				// sets parSp->psp2 to psp

// return size in bytes of compiled code
	if (pCodeSize)
		*pCodeSize = (USI)((char *)psp - (char *)psp0);

	return rc;
}		/* finPile */

//==========================================================================
RC FC expTy(
	SI toprec,
	USI wanTy,		// desired type. see exOrk() above for list of externally originated types.
					// addl internally originated type combinations incl at least TYNUM and TYANY&~TYSI
	const char* tx, // NULL or text of verb / operator, for "after 'xxx'" in error messages
	SI aN )			// 0 or fcn arg number, for error messages

// parse/compile expression/statement of given type to current destination,
// 	including resultant type check and conversions

// caller must preset: parSp, psp (call iniPile() first).
// on return, entry added to parStk describes result.
{
	ERVARS1
	USI cWanTy = wanTy;

	ERSAVE
	printif( trace," expTy(%d,%d) ", toprec, wanTy );

//---- parse/compile (sub)expression ----

	if (wanTy & TYID)	// if "ID" requested be sure string accepted
		wanTy |= TYSTR;	
	EE( expr( toprec, wanTy, tx, aN))	// parse/compile to given precedence.  only call to expr 10-90.
	// EE (cuparsex.h) restores variables and returns on error.
	USI gotTy = parSp->ty;			// data type found
	switch (gotTy)			// check for valid return type
	{
	case TYNONE:
	case TYDONE:			// nothing and whole statment (not yet used in CSE) respectively
	case TYSI:
	case TYINT:
	case TYFL:
	case TYSTR:
	case TYCH: 				// single-bit valid expression return values
	case TYNC: 				// the only multi-bit valid expr return (runtime distinguished)
		break;    			// ok
		// types valid in calls only: TYID (returns TYSTR). combinations: TYFLSTR TYNUM TYANY and any combo.

	default:
		rc = perNx( MH_S0012, gotTy, datyTx(gotTy) );	// "cuparse:expTy: bad return type 0x%x (%s) from expr"
		goto er;
	}

//---- type conversions I, driven by wanTy ----

	if (wanTy & TYID)
		wanTy = (wanTy &~TYID) | TYSTR;	// TYSTR is correct return type for TYID request; fudge wanTy.

	// note original wanTy is in cWanTy.
	switch (wanTy)
	{
	case TYDONE:			// complete "statement" requested (not yet used in CSE -- "stmts" done in cul.cpp)
		if (gotTy==TYDONE)  break; 		// complete statement found, ok.
		if (gotTy != TYNONE)			// insurance; suspect not possible.
			// Expression only was found where statement expected. To disallow, issue error message here.
			EE( emiPop() )	// Emit addl code to discard any value left on run stack.
			// changes parSp->ty to TYDONE.  Error msg if no (nested) store nor side effect in expr.
			break;
	}

//---- type conversions II, driven by gotTy ----

	gotTy = parSp->ty;					// refetch data type: some cases above may change it
	if (!(gotTy & wanTy) || (gotTy &~wanTy))		// if not a desired data type yet
		switch (gotTy)
		{
		case TYNC:
			if (wanTy & TYFL)			// TYFL excludes TYCH if here
			{
				EE( cnvPrevSf( 0, PSNCN, 0))	// 'convert' to number, ie error if choice
				parSp->ty = TYFL;			// now have a float (if choice constant, EE macro took error exit)
				break;
			}
			//else if (wanTy & TYCH)  add conversion if needed: like PSNCH but reject FLOATs.
			//else if (wanTy & (TYSTR|TYSI)==(TYSTR|TYSI))		if both integer and string acceptable (probe)
			//   need to look at value if const to decide which way to go if doing both ... ug!
			//else if (wanTy & TYSI)  convert to float then fix, if fixing floats
			//else if (wanTy & (TYSTR|TYID)) if converting choices to strings, convert NC to CH thence string.
			//If above all added, probably wanna cnv TYNC to TYFL / TYCH above this switch to use existing cases.
			break;

		case TYSTR:
			if (wanTy & TYCH && choiDt)
			{
				EE( cnvPrevSf( 0, PSSCH, choiDt))	// cnv string to choice value, now if string is just a constant.
				parSp->ty = TYCH;			// type is now "choice", 2 or 4 bytes per bits in choiDt.
				break;
			}
			break;

			//case TYCH:     // if (wanTy & TYSTR), COULD convert TYCH's back to strings... wait for very clear need 2-92.
			//						then also do to TYNC's
			//break;

		case TYSI:
			if (wanTy & TYINT)	// test for TYINT before TYFL
			{
				EE(emit(PSSIINT))		// convert SI to INT.  konstize below converts constant if constant.
				parSp->ty = TYINT;
			}
			else if (wanTy & TYFL)		// includes wanTy==TYNC
			{
				EE(emit(PSFLOAT2))		// convert SI to float.  konstize below converts constant if constant.
				parSp->ty = TYFL;
			}
			break;

		case TYINT:
			if (wanTy & TYSI)	// test for TYSI before TYFL
			{
				EE(emit(PSINTSI))		// convert INT to SI.  konstize below converts constant if constant.
				parSp->ty = TYINT;
			}
			else if (wanTy & TYFL)		// includes wanTy==TYNC
			{
				EE(emit(PSFLOAT4))		// convert INT to float.  konstize below converts constant if constant.
				parSp->ty = TYFL;		// now have a float
			}
			break;

			//case TYFL:     // if (wanTy & TYSI) possibly convert to int, or perhaps only if integral.  Also do for TYNC after PSNCN
			//break;
		}

//---- final check/error message ----

	gotTy = parSp->ty;					// refetch data type: some cases above change it
	if (!(gotTy & wanTy) || (gotTy & ~wanTy))		// if still not a desired data type
	{
		char *got = "";
		if ((gotTy & (TYNC))==TYNC)			// TYNC is both TYCH and TYFL bits
			got = ", number/choice value found,"; 	// extra explanation: error might be obscure, or bug in new code
		else if (gotTy==TYCH)
			got = ", choice value found,";
		rc = perNx( MH_S0013, datyTx(cWanTy), after(tx,aN), got);	// "%s expected%s%s". use caller's wanTy for msg.
		goto er;									// (label in ERREX macro)
	}

//---- evaluate constant expressions now
	EE( konstize( NULL, NULL, 0) )	// new 10-10-90 -- suspicious.

	printif( trace," expTyOk ");
	return RCOK;
	ERREX(expTy)

}				// expTy

//==========================================================================
LOCAL RC expr(  	// parse/compile inner recursive fcn
	SI toprec,	// precedence to parse to: terminator, operator, etc.
	USI wanTy,	/* desired type, processed mainly by caller expTy; here wanTy effects error messages, and:
		   TYDONE: compile assignment as full statement
		   other:  compile assignment as nested, if allowed: leave value on run stack.
		   TYCH: convert otherwise unrecognized ID's that match choiDt choices to string constants.
		   TYFL without TYSI: causes early float of int operands 2-95. */
	const char* tx,	// text of operator, function, or verb, for error messages
	SI aN )	// 0 or fcn argument number, for error messages

// and uses: globals evfOk, ermTx, choiDt, .

/* interface for expr, expTy, etc:
   caller must preset: psp
   on bad return: psp, parSp restored. text passed TO next ';'.
   on good return: terminating token ungotten, nextPrec and tokTy set for it.
   		   parSp ++'d, parStk frame has info on expression.
   		   psp points to next unused location; PSEND there but not advanced over. */
{
	static const char * pCuToktx = cuToktx;		// for when ptr to ptr to cuToktx needed

#define EXPECT(ty,str)  if (tokeNot(ty)) {  rc = perNx( MH_S0015, (str) ); goto er; }		// "'%s' expected"
#define NOVALUECHECK    if (lastPrec >= PROP) { rc = perNx( MH_S0014, cuToktx );  goto er; }
	// "Syntax error: probably operator missing before '%s'"
	ERVARS

	ERSAVE
	printif( trace," expr(%d,%d) ", toprec, wanTy );	// cueval.cpp

	if (wanTy & TYLLI)					// debug aid
		perl( MH_S0016);   			// "Internal error in cuparse.cpp:expr: TYLLI not suppored in cuparse.cpp"

	prec = 0;		// nothing yet: prevent "operator missing before" msg if expr starts with operand
	EE( newSf()) 	// start new parse stack frame for this expr.  EE (cuparsex.h) restores vbls and rets on error.

// loop over tokens of sufficiently high precedence

	for ( ; ; )
	{

		// next token and info: sets tokTy, prec, opp, stbk,isWord,cuToktx,etc.
		toke();						// local fcn; calls cutok.cpp

		// done if token's precedence <= "toprec" arg
		if (prec <= toprec)				// if < this expr() call's goal
		{
			printif( trace," exprDone %d ", prec);
			break;					// stop b4 this token
		}

		// checks, arguments, code generation: cases for each token type

		switch (opp->cs)
		{
			OPTBL* svOpp;

			// every case must update as needed: parSp->ty, evf, did; and prec = operand if not always covered.

		case CSUNI:
			EE(unOp(prec-1, TYSI, wanTy, opp->v1, opp->v2, ttTx))  break;	// unary integer operator, e.g. '!'

		case CSUNN:
			EE(unOp(prec-1, TYNUM, wanTy, opp->v1, opp->v2, ttTx))  break;	// unary numeric ops.
			// prec-1 used for toprec to do r to l.

		case CSBII:
			EE(biOp(prec, TYSI, wanTy, opp->v1, opp->v2, ttTx))   break; 	// binary int op'rs  |  &  ^  >>  <<  %

		case CSBIF:
			EE(biOp(prec, TYFL, wanTy, opp->v1, opp->v2, ttTx))   break; 	// binary float operator:  ' (feet-inches)
			/* feet-inches notes 2-91:
			   1) implemented as binary operator, with binding power hier than unary ops so -5'6 comes out right.
			   2) wanna check inches for 0 to 12? always (cueval) or only if constant (konstize here & test)?
			   3) wanna accept " after inches? difficult context dependency for tokenizer!
			   3) later enable only if addl arg to exOrk/expTy/expr/parSp says looking for a length? */

		case CSBIN:
			EE(biOp(prec, TYNUM, wanTy, opp->v1, opp->v2, ttTx))   break;	// bi numer (float / int, result same) ops

		case CSCMP:
			EE(biOp(prec, TYNUM, wanTy, opp->v1, opp->v2, ttTx))  		// comparisons: binary numeric ops,
				parSp->ty = TYSI;  						// int result
			break;

			/* && or ||: two int args, one int result left on stack.
				  do not eval 2nd arg (branch around it) if first is conclusive; eliminate unneeded code when args are constant. */

		case CSLEO:
		{
			SI isK1, isK2, v1, v2;
			OPTBL* svOpp;
			EE(convSi(&isK1, &v1, 1, ttTx, 0))	// check/konstize value b4 & convert to int
				EE(emit(opp->v1))			// PSJZP for &&, PSJNZP for ||
				EE(emit(0xffff))   			// offset (displacment) space
				svOpp = opp;				// save thru expTy
			EE(expSi(prec, &isK2, &v2, ttTx, 0))	// value after: get int expr, konstize
				if (isK1) 				// if 1st expr was constant
				{
					/* get rid of non-significant argument:
								  *	if arg 1 is	  &&		  ||
								  *	   0		drop arg 2	drop arg 1
								  *	   nz		drop arg 1	drop arg 2 */
					EE(dropSfs( 					/* delete parStk frame(s)+code */
						(v1 != 0) ^ svOpp->v2 /*&&:0 ||:1*/,	/* 0: drop top frame (arg 2), */
						/* 1: drop top-1 frame (arg 1) */
						1))					// drop 1 frame
						EE(dropJmpIf())				// delete arg 1 trailJmp
				}
				else if (isK2)					// if 2nd expr contant
				{
					/* get rid of non-significant argument:
								  *	if arg 2 is	  &&		  ||
								  *	   0		drop arg 1	drop arg 2
								  *	   nz		drop arg 2	drop arg 1 */
					EE(dropSfs( 					/* delete parStk frame(s)+code */
						(v2 == 0) ^ svOpp->v2 /*&&:0 ||:1*/,	/* 0: drop top frame (arg 2) */
						/* 1: drop top-1 frame (arg 1) */
						1))					// drop 1 frame
						EE(dropJmpIf())				// delete arg 1 trailJmp
				}
				else	// neither arg constant, keep both
				{
					EE(fillJmp(1))   		// set offset of jmp (end PREV parStk frame) to jmp here
						EE(combSf())			// combine code after & b4
				}
			EE(emit(PSIBOO))  		// make any nz a 1
				EE(konstize(NULL, NULL, 0))	// constize to combine PSIBOO
		}
		break;

		// grouping operators: (, [
		case CSGRP:
		{
			char ss[2];
			NOVALUECHECK;
			svOpp = opp;					// save opp thru unOp
			EE(unOp(PRRGR, TYANY, wanTy, 0, 0, ttTx))
				ss[0] = (char)svOpp->v2;
			ss[1] = '\0';	// char to string for msg
			EXPECT(svOpp->v1, ss);
			prec = PROP;		/* say have a value last: (expr) is a value even tho ')' is not:
						   effects interpretation of unary/binary ops and syntax checks */
		}
		break;

		// unexpected: improper use or token with no valid use yet
		case CSU:    // believed usually impossible for terminators due to low prec's
			rc = perNx(MH_S0017, cuToktx);    			// "Unexpected '%s'"
			goto er;

		default:
			rc = perNx(MH_S0018, opp->cs, cuToktx, prec, tokTy);
			/* "Internal error in cuparse.cpp:expr: Unrecognized opTbl .cs %d"
			   "    for token='%s' prec=%d, tokTy=%d." */
			goto er;

		// additional cases by token type
		case CSCUT:
		  {	void* v{ nullptr };
			USI sz{ 0 };
			MSGORHANDLE ms; 	// choice value,size,msg as from cvS2Choi( cuToktx, choiDt, (void *)&v, &sz, &ms),
			// for CUTID (choice subcase), CUTMONTH, .
			switch (tokTy)
			{

			default:
				rc = perNx(MH_S0019, tokTy, cuToktx, prec);	// "Internal error in cuparse.cpp:expr: " ...
				goto er;							// " Unrecognized tokTy %d for token='%s' prec=%d."

			case CUTSI: 			// integer, value in cuIntval
				NOVALUECHECK;
				EE(emiKon(TYSI, &cuIntval, 0, NULL))		// local, below
					parSp->ty = TYSI;					// have value; type integer
				// parSp->evf: no change for constant
				// prec >= PROP already true
				break;

			case CUTFLOAT:			// float number, value in cuFlval
				NOVALUECHECK;
				EE(emiKon(TYFL, &cuFlval, 0, NULL))		// local, below
					parSp->ty = TYFL;  				// have value; type = float
				break;

			case CUTQUOT:			// quoted text, text in cuToktx
				NOVALUECHECK;
				EE(emiKon(TYSTR, &pCuToktx, 0, NULL))		/* 0: put text inline, not in dm: minimize
										   fragmentation; make code self-contained */
					parSp->ty = TYSTR;					// have string value
				break;

			case CUTMONTH:			// month name reserved words
				NOVALUECHECK;
				{
					MOST* most = (MOST*)stbk;   				// save thru toke/untoke for monOp call
					// take month name as month choice value under strict context conditions.  result TYCH/DTMONCH 1-12.
	#ifdef DTMONCH							// undefining month choice type deletes code
					if (wanTy==TYCH  &&  choiDt==DTMONCH  			// if looking for a month name choice
					 &&  cvS2Choi(cuToktx, choiDt, (void*)&v, &sz, &ms)==RCOK	// look up b4 toke() overwrites cuToktx / if found
					 &&  (toke(), unToke(), nextPrec < prec))			// if no operand expr for day of month follows
						goto oopsAchoice;							// jump into case CUTID.
	#endif
					// note cvS2Choi() RCBAD2 return comes here (=use of choide alias), not expected? 8-14-2012
					ms.mh_Clear();	// insurance, drop possible info msg from cvS2Choi
					// usually take month as unary operator, followed by SI day of month expr, result DOY (day of year, SI)
					EE(monOp(most));   					// just below
				}
				break;

			case CUTSF: 			// system function, set or use
				NOVALUECHECK;
	#ifdef LOKFCN
				* EE(fcn((SFST*)stbk, toprec, wanTy))		// do it
	#else
				EE(fcn((SFST*)stbk, wanTy))			// do it
	#endif
					break;

			case CUTSV: 			// system variable, set or use
				NOVALUECHECK;
	#ifdef LOKSV
				* EE(sysVar((SVST*)stbk, toprec, wanTy))
	#else
				EE(sysVar((SVST*)stbk, wanTy))
	#endif
					break;

			case CUTUF: 			// user-defined function
				NOVALUECHECK;
				EE(uFcn((UFST*)stbk))
					break;

			case CUTUV: 			// already-declared user variable
				NOVALUECHECK;
				EE(var((UVST*)stbk, wanTy))			// do it
					break;

			case CUTAT:				// @ <className>[<objectName>].<memberName>
				NOVALUECHECK;
				EE(probe());					// cuprobe.cpp
				break;

			case CUTID: 			// identifier not yet in symbol table
				NOVALUECHECK;				// added 2-92: always missing ??
				if (dumVar(toprec, wanTy, &rc))		// false if not dummy vbl in fcn, else do, set rc
				{
					if (rc)
						goto er;					// error compiling dummy vbl use
					break;						// ok dummy variable use in fcn
				}
				else if (wanTy & TYID)			// if caller wants identifier (for record name)
				{
					EE(emiKon(TYSTR, &pCuToktx, 0, NULL))	// he's got it.  convert to string constant.
						parSp->ty = TYSTR;				// have string value.
					break;
				}
				else if (wanTy & TYCH && choiDt)		// if looking for a choice value
				{
#if 0 // unrun draft rejected 2-92: if string operand intended, quote it!
x					if ()						// check if is a choice for choiDt, else issue message
x						goto er;
x					EE(emiKon(TYSTR, &pCuToktx, 0, NULL))	// convert it now to a STRING constant
x					parSp->ty = TYSTR;				// have string value
x					break;						// caller expTy, or runtime, will convert to choice value
#else
					rc = cvS2Choi(cuToktx, choiDt, (void*)&v, &sz, &ms);	// for choiDt choice text get value and size,
					// or err ms text if not found, cvpak.cpp.
					if (rc == RCBAD)
					{
						rc = perNx(MH_S0020, ms);    		// "%s\n    (OR mispelled word or omitted quotes)"
						goto er;
					}
					else if (rc == RCBAD2)
						rc = pInfolc(ms);		// info re use of deprecated choice alias, RCOK
				oopsAchoice: 	// come here with v, sz, ms set as from cvS2Choi call to take (reserved) word as choice constant
					// eg from case CUTCOM
					EE(emiKon(TYCH, (void*)&v, 0, NULL))	// emit 2 or 4-byte choice value constant, uses choiDt
						parSp->ty = TYCH;				// now have a choice value
					break;
#endif
				}
				else					// none of the above, form error message
				{
					char** xstbk;
					const char* sub;
					if (syLu(&symtab, cuToktx, 1, NULL, (void**)&xstbk)==RCOK)	// search symtab again, case-insensitive
						sub = strtprintf(MH_S0021,   			// ":\n    perhaps you intended '%s'."
						*xstbk);  	// 1st mbr of all symtab blox is char* id
					else if (wanTy==TYSTR)
						sub = ",\n    or possibly omitted quotes.";
					else if (wanTy & TYANY					// if value desired (not whole statement)
					&&  basAnc::findAnchorByNm(cuToktx, NULL)==RCOK)	// if a rat name
						sub = ",\n    or possibly omitted '@'.";
					else
						sub = "";
					rc = perNx(MH_S0022,  	// "Misspelled word or undeclared function or variable name '%s'%s"
					cuToktx, sub);
					//dumpDefines();	// ppcmd.cpp. possible #define debug aid display.
					goto er;
				}

			case CUTQM:				// C "?:" conditional expression operator: <cond> ? <then-expr> : <else-expr>
				EE(condExpr(wanTy))   		// do it, below
					break;

			case CUTCOM:  			// comma: C sequential evaluation operator
				if (parSp < parStk
				|| parSp->ty==TYNONE)
				{
					rc = perNx(MH_S0023);   	// "Value expected before ','"
					goto er;
				}
				EE(emiPop())			// discard run value to here, errmsg if has no side effect nor store
					// continue parsing with same toprec, wanTy ...
					parSp->ty = TYNONE;			// what else?
				break;

			// case CUTDEF:  preprocessor token, "cannot" get here.
			}	// switch (tokTy)
		  }	// case CUTUT scope
		}	// switch (case)

		if (parSp < parStk)
		{
			rc = perNx( MH_S0024);
			goto er;   	// "Internal error in cuparse.cpp:expr: parse stack underflow"
		}

		// issue message if evf bits unacceptable per global set by exOrk()
		// can EVFDUMMY get here? 3-91.  YES!, while compiling any subexpr within a function, 11-95.
		{
			USI evfC = cleanEvf( parSp->evf, evfOk);	// clean up evf per context for check and msg, but don't save yet
														// in case eg DAY later added to MN|HR (see cleanEvf)
			evfC &= ~EVFDUMMY;	// here disregard "contains dummy arg ref, don't konstize" flag:
								//    normally on here in fcn, cleared by caller funcDef. 11-25-95
			USI bb = evfC & ~evfOk;		// bad bits: 1 in evfC, 0 in evfOk
			if (bb)						// if any bad bits
			{	if (bb & EVXBEGIVL)
				{	rc = perNx( MH_S0025,   		/* "Value is available later in %s \n"
								   "    than is required%s.\n"
								   "    You may need to use a .prior in place of a .res" */
							evfTx( evfC, 2),					// 2: noun: eg "each hour"
							ermTx  ?  strtprintf( " for '%s'", ermTx)  :  "" );
					goto er;
				}
				const char* insert = 				// to insert between "where" and "permitted"
					evfOk==0      ?  "only non-varying values"		// avoid "at most no variation".  "only constant values" ?
				 :  bb > evfOk    									//       if bb varying than any evfOk,
							      ?  strtprintf( MH_S0026, 	// "at most %s variation"
											evfTx( evfOk, 0))		//       ... we can be more specific
				 :  evfOk==VMHLY  ?  "only monthly and hourly variation"	// (M+H combined only when with no others)
				 :                   strtprintf( MH_S0027, evfTx( bb, 0));	// "%s variation not"

				rc = perNx( MH_S0128,				// "Value varies %s\n    where %s permitted%s."	extracted 6-95
						evfTx( bb, 1),					// 1: adverb: eg "hourly"
						insert,
						ermTx  ?  strtprintf( "\n    for '%s'", ermTx)  :  "" );
				goto er;
			}	   // if bb
		}	// scope
	}       // for ( ; ; )

// check that something was parsed
	if (parSp->ty==TYNONE)						// indicates no operand done
	{
		if (wanTy==TYDONE)
			rc = perNx( MH_S0028, after( tx, aN) );   		// "Statement expected%s"
		else
			rc = perNx( MH_S0029,    				// "Value expected %s%s"
			after( tx, aN),
			(wanTy & TYANY
			&&  prec==PRVRB
			&&  basAnc::findAnchorByNm( cuToktx, NULL)==RCOK)		// a class name verb where expr expected
			?  ".  Possibly '@' omitted."  :  "" );			// is likely to be probe with @ omitted.
		goto er;									// in ERREX macro
	}

// unget terminating token and return
	unToke();					/* unget token, in this file. sets nextPrec, backs up prec;
    						   but tokTy remains that of next token. */
	printif( trace," exprOk ");

#if 1	// devel aid 10-29-90
// check that exactly one parStk frame produced
	if (parSp != parSpSv+1)
		perNx( MH_S0030, (parSp - parSpSv));	// "Internal error: cuparse.cpp:expr produced %d not 1 parStk frames"
#endif
	return RCOK;
	// added parStk entry remains, to return info to caller.

// error exit: restores parSp and psp (msg fcn passed text to ';')
	ERREX(expr)
}			// expr

// #pragma option -N.		// stack check restore

//==========================================================================
LOCAL RC FC monOp( 	// parse expr following month name & emit DOY code (like a unary operator)

	MOST *most )	// month symbol table entry: has nDays, day0.

// similar to unOp() with const args, different code emission.

/* Dates currently accepted in all numeric contexts; later add arg
   to exOrk/expTy/expr/parSp specifying goal = date, length, etc
   to enable special syntaxes w/o whole different data types? */
{
	ERVARS1

	ERSAVE
	// caller NOVALUECHECK's.
	EE( expTy( 				/* get following expression for day of month */
		prec-1, 			/* to do unary ops r to l */
		TYSI, 			/* type integer */
		"month", 0) )
	// always returns prec >= PROP, right?
	EE( emit( PSIDOY))			// emit code: dom-to-doy conversion,
	EE( emit2( most->nDays))		// # days in this month
	EE( emit2( most->day0))		// 0-based 1st day of this month
	//   adjustment for Feb 29 will be needed if year becomes non-generic.
	// result is integer, as already set by expTy( ,TYSI, ).
	EE( konstize( NULL, NULL, 0))	// eval now if possible; get run errMsgs now insofar as possible
	EE( combSf() )			// combine 2 parStk frames: combine date with any preceding code
	// prec is >= PROP from expTy
	return RCOK;
	ERREX(monOp)
}			// monOp
//==========================================================================
LOCAL RC FC unOp( 		// parse arg to unary operator, emit code.

	SI toprec,	// prec to parse to (comma, semi, right paren, etc)
	USI argTy,	// operator's arg data type, TYNANY = any, TYNUM = int or float
	USI wanTy,	// expression type being compiled, re floating ints early 2-95.
	PSOP opSi, 	// PSNUL or pseudo-code to emit EXCEPT ...
	PSOP opFl,	// pseudo-code for float if TYNUM requested
	char *tx )	// text of operator, for error messages. aN 0.

// emits pseudo-code via psp.
{
	ERVARS1
	ERSAVE
	NOVALUECHECK;
	if (argTy != TYNUM) 
		opFl = opSi;			// if (unmodified) argTy != float, all types use opSi arg.
	if ( (wanTy & TYNUM)==TYFL	// when compiling a float expression (incl TYNC, TYFLSTR),
	 &&  argTy & TYFL )			// for numeric operators (typically argTy==TYNUM, may also be TYANY),
		argTy &= ~(TYSI|TYINT); // float arguments ASAP to not truncate 2400*300 to 16 bits nor 2/3 to 0 --
       						    // too much user confusion occurred from C-like integer operations. */
	EE( expTy( toprec, argTy, tx, 0) )   		// get expression.  always returns prec >= PROP, right?
	EE( emit( (parSp->ty==TYFL)	? opFl : opSi ) )	// emit operation code for type
	EE( combSf() )					// combine 2 parStk frames
	// prec is >= PROP from expTy
	return RCOK;
	ERREX(unOp)
}			// unOp
//==========================================================================
LOCAL RC FC biOp( 		// parse 2nd arg to binary operator, emit conversions and op

	SI toprec, 	// prec to parse to (comma, semi, right paren, etc)
	USI argTy, 	// operator's arg data type, TYANY = any, TYNUM = int or float
	USI wanTy,	// expression type being compiled, re floating ints early 2-95.
	PSOP opSi, 	// PSNUL or pseudo-code to emit EXCEPT ...
	PSOP opFl,	// pseudo-code for float if TYNUM requested
	char *tx )	// text of operator, for error messages.  aN 0 here.

// emits pseudo-code via psp.
{
	ERVARS1
	ERSAVE
	if (argTy != TYNUM)
		opFl = opSi;		// if (unmodified) argTy != float, all types use opSi arg.
	if ( (wanTy & TYNUM)==TYFL			// when compiling a float expression (incl TYNC, TYFLSTR),
	  &&  argTy & TYFL )				// for numeric operators (typically argTy==TYNUM),
		argTy &= ~(TYSI|TYINT); 		// float arguments ASAP to not truncate 2400*300 to 16 bits nor 2/3 to 0 --
       									// too much user confusion occurred from C-like integer operations.
	if (parSp < parStk  ||  (parSp->ty & argTy)==0)			// if no preceding value or if preceding value wrong type
	{
		if (parSp >= parStk  &&  parSp->ty==TYSI && argTy==TYFL) 	// if have int and want float
		{
			EE( cnvPrevSf( 0, PSFLOAT2, 0))  				// float it. eg ' (ft-inches).
			parSp->ty = TYFL;						// konstize may use ty.
		}
		else if (parSp >= parStk  &&  parSp->ty==TYNC && argTy==TYFL) 	// if have number-choice and want float
		{
			EE( cnvPrevSf( 0, PSNCN, 0))  			// float it (ie error if a choice).  possible? insurance.
			parSp->ty = TYFL;						// konstize may use ty.
		}
		else
		{
			rc = perNx( tx==NULL ? MH_S0031 :    			// "Preceding %s expected"
					MH_S0032,    			// "%s expected%s"
					datyTx(argTy), before(tx,0) );
			goto er;							// (label in ERREX macro)
		}
	}
	EE( konstize( NULL, NULL, 0) )	/* if preceding expression can be evaluated now and replaced with constant, do so.
    					   Do here to constantize leading operand even when following operand is non-constant. */
	EE( expTy( toprec, argTy, tx, 0) )    				// get expression
	if (argTy==TYNUM)							// if 'numeric' requested
		EE( utconvN( 2, tx, 0) )   					// float SI if types differ
#if 0
x    EE( emit( (argTy==TYNUM && parSp->ty==TYFL) ? opFl : opSi ) )	// emit op code for type
#else//2-95
		EE( emit( (parSp->ty==TYFL)	? opFl : opSi ) )			// emit op code for type
#endif
		EE( combSf() )							// combine 2 parStk frames.  merges .evf; uses 2nd .ty
		// prec is >= PROP from expTy
		return RCOK;
	ERREX(biOp)
}			// biOp
//==========================================================================
LOCAL RC FC condExpr(		// finish parsing C conditional expression: <condition> ? <then-expr> : <else-expr>

	USI wanTy )		/* target expr type from exOrk caller: TYID and TYCH bits feed thru to control
   			   context-dependent parsing in exprs that will (or may) return to exOrk caller, 2-92.
   			   TYSI bit off causes early float of int operands, 2-95. */

// on arrival here, <condition> has been parsed and '?' seen.
{
	ERVARS1   SI isKon, v;
	ERSAVE

// re condition (expression before ?, already parsed)
	EE( convSi( &isKon, &v, 1, ttTx, 0) )		// check/konstize preceding value & convert it to int
	if (!isKon)					// if condition not constant
	{
		// after condition, branch if false to else-expr
		EE( emit(PSPJZ) )   			// emit branch-if-zero (false) op code
		EE( emit(0xffff) )   			// space to fill later with offset (displacement) to else-expr
	}
	/* else: constant-at-compile-time condition will be DELETED below --
	   but defer as long as possible to minimize possible problems on errors
	   as ERREX does not know about dropSfs()'s psp manipulations 10-90 */

// then-expr.  Any type value ok.  NB prec of ':' is prec of '?' - 1.

	USI aWanTy = (wanTy & (TYID|TYCH|TYSI|TYINT)) | (TYANY & ~(TYID|TYCH|TYSI|TYINT));	// messy type bits feed thru to then-expr and else-expr
	EE( expTy( prec-1, aWanTy, ttTx, 0)) 			// get value, new parStk frame.
	EXPECT( CUTCLN, ":")					// error if colon not next
	if (!isKon)
	{
		// after then-expr, unconditional branch around else expr
		EE( emit(PSJMP) )   			// append unconditional branch to then-expr
		EE( emit(0xffff) )   			// filled later w offset to after else-expr
	}

// now set offset of branch after condition: come here, to do else-expr
	if (!isKon)					// if condition done at runtime
		EE( fillJmp(1) )				// sets offset of branch at end PREVIOUS parStk frame to current psp value

// else-expression, conversions to make exprs same type, check
		EE( expTy( prec, aWanTy, ttTx, 0) )		// get value, another frame
		EE( tconv(2, &aWanTy) )			/* emit code to convert 2 values to same type, if possible.
		    				   May convert either stack frame; knows about unfilled jmps at end (0xffff)*/
		// type compatibility check/message is here for context-specific message format
	USI ty1 = (parSp-1)->ty;
	USI ty2 = parSp->ty;
	if (ty1 != ty2  &&  (ty1|ty2) != TYNC)	// types must be same or combined type must be TYNC -- only 2-bit type allowed
	{
		rc = perNx( MH_S0033,		// "Incompatible expressions before and after ':' -- \n    cannot combine '%s' and '%s'"
		datyTx(ty1), datyTx(ty2) );
		goto er;					// in ERREX macro
	}
	if (isKon)					// if condition constant (value in 'v')
	{
		// DELETE dead code that never needs to be executed with constant condition
		EE( dropSfs( 2, 1) )			// delete condition expression
		EE( dropSfs( 				/* delete unneeded then- or else-expression */
			v==0,			/* 1 deletes then-expr, 0 deletes else-expr */
			1 ) )
	}
	else
	{
		// complete branch after then-expr (around else-expr)
		EE( fillJmp(1) )				// set offset of branch at end prev parStk frame (then-expr) to jmp to curr psp

		// merge parStk frames: combine ?: with preceding code
		EE( combSf() )				// combine parts: then with else; makes TYNC of TYCH + TYFL.
		EE( combSf() )				// combine preceding code + condition with then + else
	}
	// prec is >= PROP from expTy, right?
	return RCOK;
	ERREX(condExpr)
}			// condExpr

//==========================================================================
#ifdef LOKFCN
* LOCAL RC FC fcn( SFST *f, SI toprec, USI wanTy)		// parse built-in function reference or assignment
#else
LOCAL RC FC fcn( SFST *f, USI wanTy)		// parse built-in function reference or assignment
#endif

// last token gotten is its name (f arg redundant).

// toprec: prec to which to parse for expression after '=' after fcn, if a "left side" use.

// wanTy: TYDONE for whole statement (leave no value in stack);
//        TYID and TYCH bits feed thru for parsing value expressions of min/max/choose/hourval/select etc
{
	ERVARS1

	ERSAVE

// process per "case" in table

	switch (f->cs)
	{
	case FCCHU:
		EE( fcnChoose( f, wanTy) )  break;	// choose(), hourval(), etc.  only call.

	case FCIMPORT:
		EE( fcnImport(f) )  break;		// import() and importStr() fcns. only call.

	default: //?
#ifdef LOKFCN
	*    case FCREG:
		EE( fcnReg( f, toprec, wanTy) )  break;	// most fcns. only call.
#else
	case FCREG:
		EE( fcnReg( f, wanTy) )  break;		// most fcns. only call.
#endif
	}

// evaluation frequency and side effect

	parSp->evf |= f->evf;	// bits saying how often fcn must be done.
    						// f->evf is 0 to eval no more often than args and/or caller require.
	parSp->did |= f->f & SA;	// nz indicates does something even if return value not stored

// set 'prec' to indicate operand last, even tho last token may have been ')'
	prec = PROP;

	return RCOK;
	ERREX(fcn)
}		// fcn

//==========================================================================
LOCAL RC FC fcnImport( SFST *f)

// parse Import() functions: access CSE import file data. Added 2-94.

/* compiled with the aid of support code in cncult4.cpp that enters records
   in CSE "Import File Field Names Table" basAnc (IFFNM records in IffnmB);
   used after compile to associate with IMPORTFILE objects,
   and at runtime to access IMPORTFILEs by Import() pseudo-ops. */
{
	// no ERVARS/ERSAVE/ERREX needed because caller does. Use CSE_E not EE macros.
	RC rc /*=RCOK*/;

// get source file name and line: these are put into IFFNM record and into pseudo-code for use in runtime error messages.
	int fileIx, line;
	curLine( 0, &fileIx, &line, NULL, NULL, 0);		// function below in cuparse.cpp

// argument list. Both arguments are constant arguments whose values are used now.

	if (tokeNot(CUTLPR))					// get '(' after fcn name
		return perNx( MH_S0034, f->id );		// "'(' missing after built-in function name '%s'". cast to far.

// first argument: name of ImportFile object.
	// cuparse seems to have no intended support for contants, but expTy then konstize should work. 2-94.
	CSE_E( expTy( PRCOM, TYID, f->id, 1) )		// get TYID (similar to TYSTR) expr for 1st argument. makes parStk frame.
	SI isKon;  						// receives TRUE if konstize detects or makes constant
	void *pv;						// receives pointer to const value (ptr to ptr for TYSTR)
	// 							(search EMIKONFIX re correcting apparent bug in ptr ptr return 2-94)
	CSE_E( konstize( &isKon, &pv, 0) )  			/* test expression just compiled: detect constant value;
    							   else evaluate now to make constant value if possible. */
	if (!isKon)						// if not contant (and could not be make constant)
		return perNx( MH_S0123,			/* "S0123: %s argument %d must be constant,\n"
							    "     but given value varies %s" */
		f->id,			// cast near ptr to far
		1,
		parSp->evf ? evfTx(parSp->evf,1) : "");	// evf bit expected, but if none, omit explanation.
	const char* impfName = *(char **)pv;		// fetch pointer to import file name text in code frame
	// parse stack frame must be retained till impfName used in impFcn call, then dropped.

	if (tokeNot(CUTCOM))				// get comma between args
		return perNx( MH_S0015, "," );		// issue error "'%s' expected"

// second argument: field name or number.
#if 0
*    CSE_E( expTy( PRCOM, TYSI|TYSTR, f->id, 2) )	// get integer or string expr for 2nd argument
#else	// 2-94 believe this will allow unquoted field names
	CSE_E( expTy( PRCOM, TYSI|TYID, f->id, 2) )	// get integer, identifier (returns TYSTR) or string expr for 2nd argument
#endif
	BOO byName = parSp->ty==TYSTR;			// TYSI is by field number. Others should not return.
	CSE_E( konstize( &isKon, &pv, 0) )			// detect or make constant value if possible
	if (!isKon)						// if not constant
		return perNx( MH_S0123,			/* "S0123: %s argument %d must be constant,\n"
							    "     but given value varies %s" */
		f->id,			// cast near ptr to far
		2,
		parSp->evf ? evfTx(parSp->evf,1) : "");	// evf bit expected, but if none, omit explanation.
	char *fieldName = NULL;  SI fnr = 0;
	if (byName)
		fieldName = *(char **)pv;			// retreive pointer to field name text
	else
	{
		fnr = *(SI *)pv;					// else must be TYSI. retrieve field number value.
		if (fnr <= 0 || fnr > FNRMAX)					// check range. FNRMAX: impf.h.
			return perNx( MH_S0124, fnr, FNRMAX);	// "Import field number %d not in range 1 to %d"
	}
	// parse stack frame must be retained till fieldName moved elsewhere, then dropped (text may be inline in code).

	if (tokeNot(CUTRPR))				// get right paren after args
		return perNx( MH_S0015, ")" );		// issue error "'%s' expected"

// error if = next: can't be used on left side

	if (tokeIf(CUTEQ))						// = next?  else unToke.
#ifdef LOKFCN
		//if (!(f->f & LOK))					// never ok with Import fcns
#endif
		return perNx( MH_S0035, f->id);  	// "Built-in function '%s' may not be assigned a value".

	// have: impfName, fieldName, fnr, fileIx, line

// use compile support function (cncult4.cpp) to get values to put in code

	TI iffnmi;		// receives IFFNM record subscript for import file name (adds record 1st time name seen)
	SI fnmi = 0;		// receives field name index for named field (adds table entry first time name seen)
	IVLCH imFreq;	// receives import frequency (hour-day-month-year) if yet given, else a safe assumption
	if (byName)
		CSE_E( impFcn( impfName, &iffnmi, fileIx, line, &imFreq, fieldName, &fnmi) )	// cncult4.cpp
		else
			CSE_E( impFcn( impfName, &iffnmi, fileIx, line, &imFreq, fnr) )		// error returns unlikely -- most errors ABT.

			CSE_E( dropSfs( 0, 2) )		// now drop top 2 stack frames: code for arguments: constants, values already used or moved.

// emit code:  op, iffnmi, fnr or fnmi, fileIx, line.  Is added to frame started by caller.

			CSE_E( emit( byName ? f->op2 : f->op1) )	// from table: op1 for field by number, op2 by name
			CSE_E( emit( iffnmi) )				// IFFNM record subscript, to identify file
			CSE_E( emit( byName ? fnmi : fnr) )		// field number, or name index (IFFNM.fnmt[] subscript)
			CSE_E( emit( (SI)fileIx ) )			// emit input file index, for run errmsgs
			CSE_E( emit( line) )				// emit line number ditto

// set result type, precedence, evaluation frequency

			parSp->ty =  f->resTy;		// result type: TYFL or TYSTR per table

	//prec is set by caller to PROP.

	switch (imFreq)			// combine eval frequency implied by import frequency with any (unexpected) preceding evf
	{
	// case C_IVLCH_S:
	default:
		parSp->evf |= EVFSUBHR;
		break;
	case C_IVLCH_H:
		parSp->evf |= EVFHR;
		break;
	case C_IVLCH_D:
		parSp->evf |= EVFDAY;
		break;
	case C_IVLCH_M:
		parSp->evf |= EVFMON;
		break;
	case C_IVLCH_Y:
		parSp->evf |= EVFRUN;
		break;
	}

	return rc;
}		// fcnImport

//==========================================================================
#ifdef LOKFCN
* LOCAL RC FC fcnReg( SFST *f, SI toprec, USI wanTy)		// parse most functions, for fcn()
#else
LOCAL RC FC fcnReg( SFST *f, USI wanTy)			// parse most functions, for fcn()
#endif
{
	USI aTy;	// arg (1) type, re code selection for SI or FL arg
	RC rc;
	// no ERVARS/ERSAVE/ERREX needed because caller does. right? 10-90.

// argument list in ( )'s

	if (tokeNot(CUTLPR))					// get '(' after fcn name
		return perNx( MH_S0034, f->id );		// "'(' missing after built-in function name '%s'". cast to far.
	CSE_E( fcnArgs( f, 0, wanTy, 0, &aTy, NULL, NULL, NULL, NULL) )	// parse args, ret bad if err

// if '=' next in input, value is being ASSIGNED TO function

#ifndef LOKFCN
	if (tokeIf(CUTEQ))
		return perNx( MH_S0035, f->id);  	// "Built-in function '%s' may not be assigned a value".
	// cast f->id to make (char far *).
#else
*    SI onLeft = tokeIf(CUTEQ);					// = next?  else unToke.  onLeft = 1 if fcn being assigned value.
*    if (onLeft)   						// if yes
*    {  if (!(f->f & LOK))					// ok for this fcn?
*          return perNx( MH_S0035, f->id);  	// "Built-in function '%s' may not be assigned a value".
*          							// cast f->id to make (char far *).
*   // get value of fcn's type to be assigned to function
*
*       char tx[50]; 					// must be in stack
*       sprintf( tx, "%s(...)=", f->id);  	// for "after ___" in errmsgs
*       CSE_E( expTy(					/* get expr. sets nextPrec. */
*                 max( toprec, PRASS-1), 		/* parse to current toprec except stop b4 , or ) */
*                 f->resTy, 				/* type: fcn's result type */
*                 tx, 0) )
*       if (wanTy != TYDONE)     			// if context is "expression desired"
*          if (nextPrec > PRCOM) 			// if not  , ) ] ; verb EOF  next
*             pWarnlc( MH_S0036);   		// "Nested assignment should be enclosed in ()'s to avoid ambiguity"
*             						// CAUTION: MH_S0036 also used in sysVar() and dumVar().
*							// warning message, keep compiling, do not suppress execution
*   // re code for fcn assignment
*
*       if (wanTy != TYDONE)				// if set is nested
*          CSE_E (emiDup())					// dup run stack top: 1 copy to store, 1 copy for nested use in expr.
*       CSE_E( combSf( ) )					// combine expr with preceding code
*       // emiFcn is called after 'else'.  Ok to do combSf 1st.
*       // parSp->ty already correct
*       parSp->did = 1;  				// say expr has had an effect: value stored.
*}
*    else			// not on left
#endif
	{

		// "right side use" -- function value used in expression

#ifdef LOKFCN	// else all have ROK, don't check. 2-95.
*       if (!(f->f & ROK))					// check if ok for this fcn
*          return perNx( MH_S0037, f->id );  	/* "Built-in function '%s' may only be used left of '=' --\n"
*					       		           "    it can be assigned a value but does not have a value" */
#else
		// former MH_S0037 probably unused.
#endif
		if ( f->resTy==TYNUM				// if table result type = 'numeric'
		|| f->resTy==TYANY )				// .. or 'any'
			parSp->ty = aTy; 				// use type of (1st) arg
		else
			parSp->ty = f->resTy;				// else use table result type
	}
	if (!(f->f & VC))					// (VC code is done in fcnArgs)
#ifdef LOKFCN
*       CSE_E( emiFcn( f, onLeft, aTy) ) 			// emit code for fcn, below
#elif defined(LOKFCN) || defined(LOCSV)
*       CSE_E( emiFcn( f, 0, aTy) ) 				// emit code for fcn, below
#else
		CSE_E( emiFcn( f, aTy) ) 				// emit code for fcn, below
#endif
		return rc;
}			// fcnReg
/* =======================================================================*/

#undef RUNT	// define to try now-improved runtime errmsg (from Konstize)
			// not local msg for index out of range / all conds false.
			// About 4 uses in next 6 pages.  2-6-91: tried, old code still better.

//==========================================================================
LOCAL RC FC fcnChoose( SFST *f, USI wanTy) 	// do choose-type fcns for fcn() (f->cs = FCCHU)

// choose(), choose1(), select(), hourval(),

// f->f & F1 specifies hourval, F2 specifies select
// code PSDISP (0-based) or PSDISP1 (1-based) is in f->op1
// [implicit: ROK but not LOK]

// wanTy: TYID and TYCH bits feed thru when parsing value expressions.
{
	USI aTy, optn;   SI nA0=0, haveIx=0, isKon=0, v, nA, defa, nAnDef, base, i;    RC rc;

	if (tokeNot(CUTLPR)) 				// pass '(' after fcn name
		return perNx( MH_S0038, f->id );	// "'(' missing after built-in function name '%s'". cast f->id to far.

	optn = 1					// options for fcnArgs call: 1: append JMPs
	| 2					//  2: accept "default" b4 last arg
	| 4  				//  4: do not combSf
	| 16;				// 16: supply default default code from f->op2

// get index

	if (f->f & F1)	// hourval(): implicit $hour index
	{
		CSE_E( newSf() )			// make parStk frame to hold code
		CSE_E( emiLod( TYSI, &Top.iHr) )   	// load hour (0-23) variable
		parSp->ty = TYSI;		// (probably unnec to set)
		parSp->evf |= EVFHR;		// hour changes hourly: don't constize
		haveIx = 1;			// have index expr in parStk
		//isKon = 0;  			// not a constant index
	}
	else if (f->f & F2)	// select(): no index argument
	{
		optn |= 8;		// tell fcnArgs to get condExpr w ea valExpr
		//haveIx = 0;		// no index frame in parStk
	}
	else		// choose(), choose1(): integer index expression
	{
		CSE_E( expSi( PRCOM, &isKon, &v, f->id, 1) )	// get int expr and konstize
#ifndef RUNT
		if (isKon)					// if index constant
			optn &= ~16;				/* don't default the default in fcnArgs: our msg below is better than
          					   PSCHUFAI done out of context by konstize below. 10-90, 2-91.*/
#endif
		haveIx = 1;				// have index expr in parStk
		nA0 = 1;					// 1 arg already parsed (for fcnArgs)
		if (tokeNot(CUTCOM))			// pass comma after arg
			return perNx( MH_S0039);   	// "',' expected"  CAUTION multiply-used message handle
	}

// get value expressions and ')'

	CSE_E( fcnArgs( 	/* parse list of exprs, of any type but all of (or conv to) same type.  ret bad if error. */
		   f, 				/* note f->f & (MA|VA) set */
		   nA0,				/* # args already done: 1 for choose */
		   wanTy,				/* messy parsing bits may feed thru to value expressions */
		   optn,				/* options set above */
		   &aTy, &nA, &defa,		/* return info. defa 0,1,2. */
		   &isKon, &v ) )			// only set for select (optn 8)
	nAnDef = nA - (defa != 0);			// compute # args less default
	// error message for left side use
	if (tokeIf(CUTEQ))    				// "=" after ")" (else unget) ?
		return perNx( MH_S0040, f->id); 	// "Built-in function '%s' may not be assigned a value"

	// messages if wrong # values for hourval
	if (f->f & F1)					// if hourval
		if (nAnDef < 24 && defa != 1)			// < 24 args w/o user default is error
			return perNx( MH_S0041);   		// "hourval() requires values for 24 hours"
		else if (nAnDef > 24)    		 	// > 24 args gets warning
			pWarnlc( MH_S0042);   		// "hourval() arguments in excess of 24 will be ignored"

	if (isKon)	// if constant index (1) or select all cond false (-1)
	{
// Delete all code but the one value expr selected by the constant index

		base = (f->op1==PSDISP1);	// 0 or 1
		v -= base;			// adjust to 0-based
		if (v < 0 || v >= nAnDef)	// if index out of range
		{
			// (includes select all false -- nA)
#ifdef RUNT	// 2-91 trial (not kept)
o #if 1	// 2-91 retain this test & msgs till convinced
o			if (!defa)					// unexpected
o				return perNx( "cuparse.cpp:fcnChoose() internal err: 'defa' off");
o #endif
o			v = nAnDef;   		// use default (nA-1)
#else
			if (defa)			// if have a default
				v = nAnDef;   		// use default (nA-1)
			else if (f->f & F2)
				return perNx( MH_S0043, f->id );  		// "All conditions in '%s' call false and no default given"
			else
				return perNx( MH_S0044,		    				// "Index out of range and no default: \n" ...
							  f->id, v+base, base, nA-1+base );	// "    argument 1 to '%s' is %d, not in range %d to %d"
			/* These errMsgs better than cueval msgs as now (10-90, 2-91) occur out of konstize:
			   cueval does not show fcn name, and shows range limits as 0 since unused exprs deleted.
			   So we suppress def def when choose() index constant (above) or all select() cond-exprs const 0 (fcnArgs). */
#endif
		}
		CSE_E( dropSfs( 0, nA-v-1) ) 	// delete value exprs after v'th
		CSE_E( dropSfs( 1, v+haveIx) )  	// del val exprs b4 v'th, and index
		CSE_E( dropJmpIf() )			// delete jmp at end remaining valExpr
		/* what is left is one choose expr, or one select val-expr (note that
		   fcnArgs deleted constant true cond-expr of select arg pair) */
	}
	else	// non-constant index
	{
		/* conditionally emit pseudocode to dispatch to appropriate value expr per
		   index value, and fill jumps and combine stack frames of fcn call */

		if (haveIx)			// select uses no dispatch
			CSE_E( emiDisp( f, nA, defa) )	// emit dispatch after index

			// fill jumps in args to after last arg (current psp value)

			for (i = nA; --i >= 1; )		// for i = nA-1..1: last has no jmp.
				CSE_E (fillJmp(i) )    		// replace 0xffff with offset to psp, rif err

				// combine nA or nA+1 stack frames into one (nA args, 1 index)

				for (i = nA+haveIx; --i >= 1; )	// nA or nA-1 times
					CSE_E( combSf() )			// combines top two stack frames
				}

// constantize and combine with preceding code

	CSE_E( konstize( NULL, NULL, 0) )	// eval now if possible. usually redundant due to isKon stuff above.
	CSE_E( combSf() )			// combine with preceding stack frame

// notes:
	//result type: same as value argument types, thus final parSp->ty correct.
	//code generation: only code is that of args plus what emiDisp emits.

	return RCOK;			// more returns above, incl in 'E' macros
}		// fcnChoose
//==========================================================================
LOCAL RC FC fcnArgs( 		// parse args for regular-case built-in function, and some special cases

	SFST *f,	// ptr to fcn table entry
	SI nA0, 	// 0 or # previously parsed special args (for errMsgs)
	USI wanTy,	/* desired type: TYID and TYCH bits may feed thru when parsing "TYANY" fcn arguments.
   		   TYFL without TYSI may float ints for TYNUM args if fcn rets arg type 2-94. */
	USI optn,	/* bits: 1: append PSJMP 0xffff to each argument but last
			 2: accept "default" before last arg
			 4: do not combine arg stack frames
			 8: get cond-expr before each value-expr (select)
			16: conditionally supply default default code from f->op2 if no "default" argument */
	USI *paTy,	// rcvs type of arg 1 (or all args if MA), for use re arg-dependent code emission and/or result type
	SI *pnA,	// rcvs # args (or pairs) parsed here. excl nA0, incl default
	SI *pDefa,		// rcvs 1 if "default" preceded last arg, 2 if dflt dflt gen'd (runtime errmsg), else 0
	SI *pisKon, SI *pv )	/* set for select (optn 8 only) a la choose index:
   			   *pisKon = nz if known which val-expr will be used;
   			   *pv = 0-based index (arg #) of that expr (nA if all false: default or error, 2-91) */
{
//ERVARS  	//<--- believe unnec here becuase caller fcn() does. 10-90
	SI aN=0, uAN=0, nSF=0, defdef=0, selC=0, isKon=-1, v=0;
	SI tisK=0, tv=0;			// isKon and v for current select() condExpr
	USI aTy=TYNONE, aWanTy=0, defa=0;
	RC rc;

	//ERSAVE	//<--- believe unnec here becuase caller fcn() does. 10-90

// check for c-syntax empty list (expr() would issue error)

	if (tokeIf(CUTRPR)==0)	// pass ')' if next / if no ')'
	{

		// argument expressions loop

		do
		{
			aN++;			// count args (counts pairs for optn 8)
			uAN++;		// user arg # for errMsg display
			nSF++;		// count un-combSf'd parStk frames (VC)

			// accept "default" before expr (as well as as separator)
			if (optn & 2  &&  !defa)	// if "default" allowed & not seen
				if (tokeIf(CUTDEFA))	// if next toke "default" (else unget)
					defa = 1;  		// say user default seen

			// optionally parse condition-expr b4 each value-expr for select()

			if (optn & 8)
			{
				if (defa)			// no cond-expr for default
				{
					tisK = 1;
					tv = 1;	// but make like const true cond
				}				// .. for purposes of isk/v return
				else
				{
					CSE_E( expSi( PRCOM, &tisK, &tv, f->id, uAN+nA0) )		// int expr
					if (tokeNot(CUTCOM))
						return perNx( MH_S0045,    			// "',' expected. '%s' arguments must be in pairs."
						f->id );
					CSE_E( emit( PSPJZ) )   	// pop and branch (around val) if 0
					CSE_E( emit( 0xffff) ) 	// filled after val-expr parsed
					uAN++;			// count an arg for errMsg display
					selC++;			// say have select cond-expr -- complete arg pair after val-expr.
				}
				// return info on constant conditions in choose-index format
				if (isKon < 0)		// if active val-expr not yet determined (ie all false constants to here)
				{
					if (tisK==0)		// if this cond-expr not constant
						isKon = 0;		// cannot know active val-expr til run
					else if (tv==0)		// if constant 0
						;			// no change, keep looking
					else			// const non-0, preceded by const 0s if here
					{
						isKon = 1;		// say DO know which val-expr will be used
						v = aN-1;		// 0-based index of that val-expr, for caller
					}
				}
			}

			// determine desired type for this arg

			if (aN==1)
				aWanTy = f->a1Ty;
			else if ((f->f & MA)==0)
				aWanTy = (aN==2 ? f->a2Ty : f->a3Ty);
			//else: aN > 1 && f & MA: do not change aWanTy here.
			if (aWanTy==0)				// if 0 in table (eg too many args)
				aWanTy = TYANY;				// make better errMsg, 10-90

			// for "any type" expr get context-dependent parsing bits re IDs, CHoices, etc from caller's wanTy 2-92

			// ASSUME TYANY expr may return, thru min/max/choose/hourval etc; consumed args have specified types.

			if (aWanTy==TYANY)						// if "any type arg"
				aWanTy = (wanTy & (TYID|TYCH)) | (TYANY & ~(TYID|TYCH));	// feed messy type bits thru..

			// for function returning arg type (min,brkt,hourly,...) and accepting numeric args, float args early 2-95.

			if (aWanTy & TYFL)				// if arg type can be float (expect TYANY or TYNUM)
				if (f->resTy==TYNUM || f->resTy==TYANY)	// if fcn rets arg type (table says returns 'numeric' or 'any type')
					if ((wanTy & TYNUM)==TYFL)		// if compiling a float expression (incl TYNC, TYFLSTR),
						aWanTy &= ~(TYSI|TYINT);	// float args ASAP to not truncate 2400*300 to 16 bits nor 2/3 to 0 --
	       											// too much user confusion occurred from C-like integer operations.

			// parse argument expression (value-expr for choose or select)

			CSE_E( expTy( PRCOM, aWanTy, f->id, uAN+nA0) )	// parse an expr of type.  Makes parStk frame.

			// check/pass separator

			toke();				// next token / set tokTy
			if ( optn & 2  &&  !defa 		// if "default" allowed
			&&  tokTy==CUTDEFA ) 		//   and present
				defa = 1;				// say so; allow no more args
			else if (tokTy==CUTCOM  &&  !defa)	// ',' not after "default"
				;					// ok
			else if (tokTy==CUTRPR) 		// ')'
				// ok  (retested at end loop)
				defdef =  				// nz for "default default"
				( (optn & (2|16))==(2|16)	// if default & def def optns
				&&  !defa  			// if no default given by user
				&&  f->op2			// if table has def def code
#ifndef RUNT	// remove to use konstize's msg not fcnChoose's
				&&  !(optn & 8 && isKon < 0)	/* not if all select condExprs
		 		      const 0: fcnChoose does better ermMsg */
#endif
				);
			else								// issue err msg
				return perNx( MH_S0046,				// "Syntax error: expected '%s'"
				uAN < f->nA           ?  ","         :
				(f->f & VA && !defa)  ?  ",' or ')"  :
				")"           );

			if ( optn & 1      		// on option append JMPs to args
			&&  ( tokTy != CUTRPR 	// no jmp after last arg, but..
			|| defdef) )		// yes jmp before default default
			{
				CSE_E( emit(PSJMP) )		// append unconditional branch
				CSE_E( emit(0xffff) )		// caller fills later w offset
			}

			// save arg 1 type to ret to caller (also used in switch below)

			if (aN==1)
				aTy = parSp->ty;

			// for select(), complete conditional jump and combine arg pair

			if (selC)				// if cond-Expr preceded this value expr
			{
				selC = 0;

				// select: if cond-expr constant true, drop it
				// BUT: we do not drop val-expr for const false cond yet: keep it here for consistent type conversion behavior.
				if (tisK && tv)
					CSE_E( dropSfs( 1, 1) )			// delete 1 parStk frame 1 frame down
					else
					{
						// select(): conditionally hold space for later val-expr conversn
						if (parSp->ty==TYSI)			// if SI, might float later
						{
							CSE_E( cnvPrevSf( 0, PSNOP, 0) )		// emit place holder (b4 jmp). PSNOPs must be emitted via cnvPrevSf 10-95.
							/* later cnvPrevSf from utconvN from MA switch will store PSFLOAT over PSNOP, not move expr,
							   & thus not invalidate offset of cond jmp which is filled and combSf'd next. */
						}
						if (parSp->ty==TYSTR)			// if string, might choice it later, likewise
							CSE_E( cnvPrevSf( 0, PSNOP, PSNOP) )	// emit place holder (b4 jmp): 2 slots, for PSSCH, choiDt.
							// select(): fill jump after cond-expr (around val-expr) and combSf
							CSE_E( fillJmp(1) )				// fill jmp 1 frame back to jmp to psp
							// another unfilled jmp remains at end valExpr.  It will jmp past all args.  Caller will fill.
							CSE_E( combSf() )				// combine condExpr and valExpr frames
							// must combSf b4 utconvN in MA switch below -- else modify utconvN, tconv to do alternate stack frames.
							// CAUTION: this frame CANNOT be konstize'd: not single value.
						}
			}

			// optionally match arg types

			if (f->f & MA)		// if matching arg types (choose/select/hourval)
			{
				switch (aWanTy)
				{
#if 0	// good b4 TYCH
x					case TYANY:		// any type, but all args match
x						//if (aN==1): only get here on 1st arg
x						switch (aTy)  		// set type for later args
x						{	case TYSI: aWanTy = TYNUM; break;	// floats may follow si
x							default: aWanTy = aTy; break;	// other: all args same
x						}
#else	// adding TYCH/TYNC 2-92
				default:		/* any aWanTy without special case (note 'any type' value varies per wanTy bits
	        			   and as tconv clears bits.  Single types (expTy handles) should fall thru ok. */

					CSE_E( tconv( nSF, &aWanTy) )		// convert nSF stack frames to compatible types if possible
					/* clears incompat aWanTy bits to make best use of expTy's conversions
					   (which are b4 konstize b4 jump added that prevents konstize) */
					// check for incompatible argument: done here for context-specific message format
					{
						USI anTy = parSp->ty;		// note assume previously-checked args remain compat with each other
						for ( SI i = nSF; --i > 0; )		// check preceding uncombined stack frames for type compat with latest
						{
							USI argType = (parSp - i)->ty;			// type of arg aN-i
							if (argType != anTy  &&  (anTy| argType) != TYNC)  	// only 2-bit type allowed is TYNC === TYCH|TYFL.
							{
								// error message for incompatible argument
								SI ann = (optn & 8) ? 2*aN + nA0 : aN + nA0;   	// argument # for errmsg of last arg parsed
								SI an1 = (optn & 8) ? ann - 2*i  : ann - i;		// arg # of other arg that failed the check
								// optn & 8: condexpr b4 each arg. done right here? if so, why copped out in TYNUM case (next)? 2-92
								char *anTx =  (nSF < aN) ? ""   			// if code already generated (VC), arg #'s unknown
								:  strtprintf(" %d and %d",an1,ann);	// arg # subtext eg " 1 and 3"
								return perNx( MH_S0047,  			// "Incompatible arguments%s to '%s':\n"..
								anTx, f->id,			// "    can't mix '%s' and '%s'"
								datyTx(argType), datyTx(anTy));
							}
						}
					}
#endif
					break;

					// BELIEVE sep TYNUM case no longer needed; when tconv works, remove it, just use default, eliminate switch! 2-92
#if 0          // so remove the TYNUM case 2-95. When has been ok for a while, edit out & remove the switch.
x
x	       case TYNUM:				// all same numeric type
x				if (parSp->ty==TYFL)
x				{	CSE_E( utconvN( nSF, f->id, (nSF < aN || optn & 8) ? -1 : aN+nA0) )
x								// float any preceding si args or (VC) intermediate results
x					aTy = TYFL;  				// arg ty to return to caller
x					aWanTy = TYFL;				// addl args must be float
x				}
x				break;
#endif
				}
			}	// if (f->f & MA)

			// optionally emit code after args after first

			if (f->f & VC)		// if variable args code option (min/max)
				if (aN >= 2)		// no code til 2nd arg done
				{
#ifdef LOCFCN
*		// VC can't work on both sides as onLeft not known (by caller) til after arg list done
*		if ( (f->f & (LOK|ROK))==(LOK|ROK) )			// "Internal error in cuparse.cpp:fcnArgs:\n" ...
*		   return perNx( MH_S0048, f->id );	// "    bad fcn table entry for '%s': VC, LOK, ROK all on"
#else
					// MH_S0048 probably no longer used 2-95.
#endif
#if defined(LOKFCN) || defined(LOCSV)
*                CSE_E( emiFcn( f, 0/*onLeft*/, aTy) ) 			// emit code for fcn
#else
					CSE_E( emiFcn( f, aTy) ) 			// emit code for fcn
#endif
					CSE_E( combSf() )				/* combine last arg with preceding code now.
							   Any addl utconvN (under MA) applies to RESULT of code just emitted. */
					CSE_E( konstize( NULL, NULL, 0) )		// eval result & repl w const if poss: e.g. get min/max of 2 constants now.
					nSF--;					// say 1 less uncombined frame
				}
		}
		while (tokTy != CUTRPR);

	}	// if have '('

// check # args

	if (uAN < f->nA)
		return perNx( MH_S0049,  		// "Too few arguments to built-in function '%s': %d not %d%s"
		f->id, uAN+nA0, f->nA+nA0,
		(f->f & VA) ? " or more" : "" );
	else if (uAN > f->nA  &&  !(f->f & VA) )
		return perNx( MH_S0050, 	  		// "Too many arguments to built-in function '%s': %d not %d"
		f->id, uAN+nA0, f->nA+nA0 );

// if no default, optionally generate one using .op2

	if (defdef)		// set above if optn & 16 && !defa ...
	{
		defa = 2;
		aN++;   	// ret as tho caller gave default except defa is 2 not 1 re hourval < 24 args errmsg
		CSE_E( newSf() )
		nSF++;
		CSE_E( emit( f->op2) )			// eg code to generate error message
		// >> expect to want to add file/line info for errmsg
		parSp->ty = (parSp-1)->ty;		// propogate type in case caller uses last arg as representative
		//parSp->evf = (parSp-1)->evf;		if need found 10-90
		//parSp->did = (parSp-1)->did;		..
		/* if defdef used in all cases, can we eliminate caller code
		   re omitted default and cueval code re 0 default offset ? 10-90.
		   But defdef NOT always used, 2-91. */
	}

// conditionally combine (remaining) stack frames  (deferred to here in case conversions needed (MA w/o VC)

	if ((optn & 4)==0)		// option defers to caller for special code generation for choose, select, etc
		while (nSF-- > 0)	// aN times, less combSf's done above: combine all args and preceding code
			CSE_E( combSf() )   	/* combine 2 parStk frames: merge already-merged args with preceding arg or
          			   (last iteration) with preceding code. Yields TYNC for TYFL + TYCH. */

// return info if nonNULL pointers given

			if (paTy)
				*paTy = aTy;		// type of 1st or all args
	if (pnA)			// number of arguments (or pairs) parsed here
		*pnA = aN;		//   including default or generated default
	if (pDefa)			// 1 if "default" before last arg,
		*pDefa = defa;   	//   2 if default default generated, else 0
	if (optn & 8)		// ret info on const cond's for select()
	{
		if (pisKon)
			*pisKon = isKon;		// 0 unknown, 1 known, -1 all cond 0: default/error
		if (pv)
			*pv = isKon >= 0	// < 0 all cond 0, 0 non-const cond,
			?  v  		// isKon > 0: 0-based idx of expr with true cond
			:  aN;		// all false: out of range to tell fcnChoose to use default, error if !defa
	}
	return RCOK;
	//ERREX(fcnArgs)
}			// fcnArgs
//==========================================================================
LOCAL RC FC emiFcn( 		// emit code for FCREG built-in function with given arg 1 type
	SFST *f,
#if defined(LOKFCN) || defined(LOCSV)
	SI onLeft,
#endif
	USI aTy )
{
#if defined(LOKFCN) || defined(LOCSV)
*// check fcn table entry for using only 1 of the 2 alt code uses
*    if (f->f & ROK)
*       if (f->f & LOK)
*          if (f->a1Ty==TYNUM)
*             return perNx( MH_S0051, f->id );  	// "Internal error in cuparse.cpp:emiFcn: bad function table entry for '%s'"
#endif

// emit appropropriate code
	return emit(		// emit code
#if defined(LOKFCN) || defined(LOCSV)
*      onLeft				// if on left of = (being assigned a value) */
*         ?  (f->f & ROK)			// if fcn also allowed on right */
*               ?  f->op2			// use alt code */
*               :  f->op1			// else use basic code */
*         :
#endif
		(f->nA > 0 && f->a1Ty==TYNUM && aTy==TYFL)
		?  f->op2    			// code used if on right and 1st arg allowed to be integer or float and is float
		:  f->op1 );			// 1st arg int and all other right cases
}				// emiFcn
//==========================================================================
LOCAL RC   FC emiDisp( 		// emit "dispatch" operation

	SFST *f,	// function table entry: code in ->op1 is used.
	SI nA, 	// number of arguments in parStk (excluding index).
	SI defa )	// non-0 if last arg is default

/* dispatch index expression precedes the nA arguments in parStk.
   Code from f->op1 (PSDISP etc) and its parameters are appended to index, using 0 default offset if !defa. */
{
	SI n, i;   PSOP *pspe;  RC rc;

// make space for dispatch and its parameters after the index expression

	n = defa ? nA-1 : nA;		// # offsets for PSDISP, excluding default
	CSE_E( movPsLastN( nA, n+3) )		// make space (n+3) after expr (parSp-nA)
	// E returns fcn value now if error (not RCOK)

// emit PSDISP or PSDISP1, n, n offsets, default offset in movPsLastN's space.

	pspe = (parSp-nA)->psp2;		// where to emit dispatch
	*pspe++ = f->op1;			// dispatch operation code is followed
	*pspe++ = n;			// by n, then n+1 offsets.
	for (i = nA; --i >= 0; )			// emit offsets to nA (n or n+1) args
	{
		*pspe = (PSOP)((parSp-i)->psp1 - pspe);	// offset from self to ith arg
		pspe++;
	}
	if (!defa)				// if default not included in nA
		*pspe++ = 0;			// supply 0 offset: runtime error
	(parSp-nA)->psp2 = pspe;		// store updated pointer

	return RCOK;
}			// emiDisp
//==========================================================================
#ifdef LOKSV
* LOCAL RC   FC sysVar( SVST *v, SI toprec, USI wanTy)
#else
LOCAL RC   FC sysVar( SVST *v, USI wanTy)
#endif

// parse built-in function reference or assignment.  No subscripts yet.

// last token gotten is its name (v arg redundant).

// [toprec: prec to which to parse for expression after '=' after sv, if a "left side" use.]

// wanTy: TYDONE for whole statement (leave no value in stack)
{
	ERVARS1
	ERSAVE
#ifndef LOKSV
	printif( trace," sysVar(%s,%d) ", v->id, wanTy);
#else
	*    printif( trace," sysVar(%s,%d,%d) ", v->id, toprec, wanTy);
#endif
	switch (v->cs)
	{
		//case SVREG:
	default:		// are all sys var the same re l/r?

		// if '=' next in input, value is being ASSIGNED TO sys var

#ifndef LOKSV
		if (tokeIf(CUTEQ))				// if '=' follows the variable name
		{
			rc = perNx( MH_S0052, v->id);	// "System variable '%s' may not be assigned a value"
			goto er;
		}
#else
*		SI onLeft = tokeIf(CUTEQ); 			// '=' next?  else unToke.  Get 1 if sys var being assigned a value.
*		if (onLeft)					// if yes
*		{	if (!(v->f & LOK))				// ok for this fcn?
*			{	rc = perNx( MH_S0052, v->id);	// "System variable '%s' may not be assigned a value"
*				goto er;
*			}
*		// get value of var's type to be assigned to function
*			EE( expTy( 				/* get expr. sets nextPrec. */
*	               max( toprec, PRASS-1),		/* parse to current toprec except stop b4 , or ) */
*	               v->ty, 				/* expr type: var's type. */
*	               v->id, 0 ) )
*			if (wanTy != TYDONE)			// if context is "expression desired"
*				if (nextPrec > PRCOM)			// if not  , ) ] ; verb EOF  next
*					perlc( MH_S0036);  		// "Nested assignment should be enclosed in ()'s to avoid ambiguity"
*             						// CAUTION: MH_S0036 also used in fcnReg() and dumVar().
*							// message but keep compiling at same place
*		// code for sys var assignment
*			if (v->f & INC)				// on incr option, emit decr
*				if (v->ty==TYSI)
*					EE( emit( PSIDEC) )
*				else if (v->ty==TYFL)
*					 EE( emit( PSFDEC) )
*		//(else bad table entry)
*				EE( emiSto( wanTy!=TYDONE, v->p) )		// uses, sets parSp->ty
*				EE( combSf( ) )				// combine with preceding code
*				// parSp->ty set by emSto
*				parSp->did = 1;				// say expr has had an effect: value stored.
*		}
*		else				// not on Left
#endif
		{
			// "right side use" -- sys var value used in expression
#ifdef LOKSV			// else all have ROK, skip check 2-95
*			if (!(v->f & ROK))					// check if ok for this fcn
*			{	rc = perNx( MH_S0053, (char *)v->id );	// "System variable '%s' may only be used left of '=' --\n" ...
*				goto er;					// "    it can be assigned a value but does not have a value"
*			}							// cast v->id to insure far ptr in variable arg list
#else
			// MH_S0053 probably unused
#endif
			EE( emiLod( v->ty, v->p) )			// emit op code to load value
			if (v->f & INC)				// on option, emit ++ (1-base for user)
				if (v->ty==TYSI)
					EE( emit( PSIINC) )
				else if (v->ty==TYFL)
					EE( emit( PSFINC) )
				//(else bad table entry)
				parSp->ty = v->ty; 			// set type
		}
		break;
	}		// switch (v->cs)

// evaluation frequency
	parSp->evf |= v->evf;	/* bits saying how often sys var must be done.
    				   v->evf is 0 to eval no more often than args and/or caller require. */
	// prec >= PROP from id or expTy, right?
	return RCOK;
	ERREX(sysVar)
}			// sysVar

//==========================================================================
LOCAL RC   FC uFcn( UFST *stb )		// compile call to user function
{
	SI aN;   USI aTy;   RC rc;

// parse argument list
	aN = 0;					// arg counter
	if (tokeNot(CUTLPR))  			// ( required
		return perNx( stb->nA==0			// special errMsg for 0 arg fcn
		?  MH_S0054	// "'(' expected: functions '%s' takes no arguments \n" ...
		// "    but nevertheless requires empty '()'"
		:  MH_S0055,   	// "'(' expected after function name '%s'"
		stb->id );
	if (tokeIf(CUTRPR)==0)			// unless ) next: empty argList. ')' gobbled.
	{
		do					// args loop head
		{
			aTy = aN >= stb->nA			// type for this arg:
			?  TYANY			// if too many args, use "any type"
			:  stb->arg[aN].ty;
			CSE_E( expTy( PRCOM, aTy, stb->id, aN+1) )	// parse expr of type, make parStk frame
			CSE_E( combSf() );  				// combine parStk frames: merge arg's code with preceding
			aN++;					// count args
		}
		while (tokeTest(CUTCOM));		// get separator / args loop endtest
		if (tokTy != CUTRPR)
			return perNx( aN >= stb->nA  ?  MH_S0056		// "')' expected"
			:  MH_S0039 );		// "',' expected"  CAUTION multiply used msg handle
	}
	if (aN != stb->nA)
		return perNx( MH_S0057,   			// "Too %s arguments to '%s': %d not %d"
		aN > stb->nA ? "many" : "few",
		stb->id,  aN,  stb->nA );

	/* code has now been emitted to stack argument values, in order.
	   PSCALA sets up stack frame whereby code in fcn (generated by dumVar) accesses values;
	   PSRETA (in fcn code) clears frame and arg values. */

// emit call code
	CSE_E( emit2( PSCALA) );			// absolute call
	CSE_E( emitPtr( (void **)&stb->ip) );		// address to call
	// fcn body removes arguments from stack, leaves return value (PSRETA)

	prec = PROP;				// operand last (necess if 0 args)
	parSp->ty = stb->ty;			// type: fcn's return type
	parSp->evf |= stb->evf;			// eval freq: args' | fcn's
	parSp->did |= stb->did;			// effect: args' | fcn's (0 usually expected)
	return rc;
}			// uFcn
//==========================================================================
LOCAL SI FC dumVar( 	/* test if current token is dummy variable name of fcn being compiled.
				   if not, return FALSE.
				   if yes, compile reference, set *prc, return TRUE */

	SI toprec,   	// used if = follows: nested assignment
	USI wanTy,		// ditto
	RC *prc )		// rcvs RCOK/error code iff IS a dummy vbl reference

/* uses globals (set non-0 by funcDef):
	nFa:  			 0 or # args to fcn defn being compiled
	fArg[] .id .ty .sz .off	 args info array */
{
	UFARG *p;   SI sto;   RC rc /*=RCOK*/;

// return 0 if not a dummy argument
	if (nFa==0 || fArg==NULL)
		return 0;				// no fcn with args being compiled
	for (p = fArg; p < fArg + nFa; p++)		// search for name
#if 0
x       if (strcmp( p->id, cuToktx)==0)
#else//11-94
		if (_stricmp( p->id, cuToktx)==0)		// tentatively made case-insensitive 11-94. Also pp.cpp, ppcmd.cpp.
#endif
			goto found;
	return 0;					// name matches no arg

// is a current dummy variable name
	/* runtime stack assumed here: each argument value is at offset
	   p->off (as set by funcDef) from stack frame pointer. */

found:			// p points to info entry for the dummy variable
	NOVALUECHECK;

	/* flag value as "containing dummy arg reference":
		prevent evaluation without a call, as in konstize().
		Cleared in funcDef, should be on only in fcn body compilation. */
	parSp->evf |= EVFDUMMY;

// if = next, variable is being set
	if (tokeIf(CUTEQ))   			// if '=' next, else unToke
	{
		// parse expression for (nested) assignment
		EE( expTy( max( toprec, PRASS-1), p->ty, p->id, 0) )	// expTy: above.  EE: goes to er if not RCOK
		if (wanTy != TYDONE)     				// if context is "expression desired"
		{
			if (nextPrec > PRCOM) 		// if not  , ) ] ; verb type EOF  next
				pWarnlc( MH_S0036 	);   	// "Nested assignment should be enclosed in ()'s to avoid ambiguity" 3-use MH!
#if 0//rc lost at next EE macro
x			rc |= emiDup();			// duplicate stack top
#else//12-14-94
			EE(emiDup());				// duplicate stack top
#endif
		}
		parSp->did = 1;				// say something done
		//evf, ty, prec set by expTy.
		EE( combSf() )				// merge expr with preceding code
		sto = 1;					// say emit store code
	}
	else			// no '='
	{
		parSp->ty = p->ty;    			// dummy variable's type
		sto = 0;					// say emit load code
		//prec = PROP set by toke().
		//no evf.  no did.
	}

// emit reference to dummy argument
	switch (p->ty)				// emit store- or load-from-stack-frame opcode
	{
	case TYSI:
		rc = emit( sto ? PSRSTO2 : PSRLOD2);
		break;

	case TYSTR:				// get a char *.  >> need to duplicate string storage?
	case TYFL:
	case TYINT:
		rc = emit( sto ? PSRSTO4 : PSRLOD4);
		break;

	default:
		rc = perNx( MH_S0058);
		goto er;  	// "Internal error: bug in cuparse.cpp:dumVar switch"
	} /*lint +e616 */
	if (rc==RCOK)
		rc = emit2( p->off);			// emit frame offset (arg to PSRLOD/STO2/4)
er:
	*prc = rc;					// return error code via pointer
	return 1;					// 1: IS a dummy vbl ref, compiled here.
	// additional returns above
}				// dumVar
//==========================================================================
LOCAL RC FC var([[maybe_unused]] UVST *f, [[maybe_unused]] USI wanTy)		// parse (poss future) variable reference or assignment

// last token gotten is its name (f arg redundant).

// wanTy: TYDONE for whole statement (leave no value in stack)

/*ARGSUSED */
{
	/*
	code to look for = afterward, check left/right side,
	compile ref or assignment, nested or not
	  (do nested by emiDup'ing before storing)
	data type of complete assignment is TYDONE if wanTy is TYDONE,
	  else variable's type.

	if value is stored, set parSp->did.

	see fcn() -- largely similar, but no arg list (subscripts: save for later)
	*/

	return perNx( MH_S0059);   			// "User variables not implemented"

	/* ... remember that DECLARE should disallow $ as 1st char user variable */
	/* ... remember to add to symbol table case-sensitive (casi arg 0) */
}		// var

//==========================================================================
LOCAL RC FC expSi( 	// get an integer expression for a condition.  accepts & fixes floats, konstizes, returns info.

	SI toprec,		// precedence to which to parse -- eg PRCOM
	SI *pisKon, 	// NULL or receives non-0 if is constant
	SI *pv,			// NULL or receives value if constant
	const char *tx,		// text of preceding operator or fcn name, for errMsgs
	SI aN )		// 0 or fcn argument number, for errMsgs

// *pisKon and *pv receive info for code optimization by elminating conditional code when condition is constant.

{
	RC rc;

// get numeric expression: SI or float  (expTy( TYSI ) errors on floats)
	CSE_E( expTy( toprec, TYNUM, tx, aN) )

// convert it to int if float (error if string), test if constant
	return convSi( pisKon, pv, 0, tx, aN);
}						// expSi
//==========================================================================
LOCAL RC FC convSi( 		// convert last expr to int, else issue error.  konstizes.

	SI* pisKon, // NULL or receives non-0 if is constant
	SI *pvPar,	// NULL or receives value if constant
	SI b4,	// for errMsgs: 1 if expr b4 <tx>; 0 expr after; moot if aN.
	const char *tx,	// text of preceding operator or fcn name, for errMsgs
	SI aN )	// 0 or fcn argument number, for errMsgs

// *pisKon and *pv receive info for code optimization by eliminating conditional code when condition is constant


{
	RC rc = RCOK;

	switch (parSp->ty)
	{
	case TYFL:
		CSE_E( emit( PSFIX2) )  	// float: convert (rif error)
		parSp->ty = TYSI;	// type is now integer
		break;		// consistency issue: some contexts (expTy) may not fix floats where int needed

	case TYSI:
		break;		// already integer, ok.

	default:
		return perNx( MH_S0060, b4 ? before(tx,aN) : after(tx,aN) );  	// "Integer value required%s"
	}

	SI isKon;
	void* pv;
	rc = konstize( &isKon, &pv, 0);	// if expression can be evaluated now and replaced with constant, do so
	if (rc==RCOK)			// if ok, return optimization info
	{
		if (pisKon)
		{
			*pisKon = isKon;  		// non-0 if value constant
			if (isKon)			// if constant (else pv unInit --> protect violation)
				if (pvPar)			// if value return pointer given
					*pvPar = *(SI *)pv;	// return constant value to caller
		}
	}
	return rc;				// error returns above
}			// convSi

//==========================================================================
LOCAL RC FC tconv( 		// generate type conversions to make last n expressions compatible 2-92
	SI n,
	USI *pWanTy )	// incompatible bits are cleared in *pWanTy for fcnArgs addl arg parsing


// DOES NOT check for incompatible args -- caller checks and messages per syntax ( ?:  or  multi-arg fcn)
// (caller fcnArgs assumes will not make previously tconv'd args 1..n-1 incompat)

// uses global choiDt.
// (does not know wanTy of enclosing expTy call nor whether operator follows)

// returns non-RCOK if error in converting, message issued.
{

// merge types into havTys; test if all same (or TYNC/TYCH/TYFL: runtime mixable)

	USI havTys = parSp->ty;
	bool bDiff = false;
	for (int i = 1; i < n; i++)				// loop n most recent parStk frames except last
	{
		USI aTy = (parSp-i)->ty;
		if (aTy != havTys && (aTy | havTys) != TYNC)  	// mixes of TYCH, TYFL, TYNC===TYFL|TYCH are not 'different'
			bDiff = true;
		havTys |= aTy;
	}
	if (!bDiff)				// if none are different (or n is 0 or 1)
		return RCOK;			// return with no processing.  SIs, STRs not changed when not mixed with others.

// float SIs if have any floats, or choices, or strings (possible choices, else error with TYSI).  For TYNUM or TYNC end result.
	USI wanTy = pWanTy ? *pWanTy : 0;
	if (havTys & (TYFL|TYCH|TYSTR))
	{
		wanTy &= ~(TYSI|TYINT);				// make expTy float any addl SI args for caller
		if (havTys & TYSI)
		{
			for (int i = 0; i < n; i++)		// loop n most recent parStk frames
			{
				PARSTK* pspe = parSp - i;
				if (pspe->ty==TYSI || pspe->ty == TYINT)		// if integer, float it
				{
					if (cnvPrevSf( i, PSFLOAT2, 0))	// insert conversion op after ith expr back (or just float it if constant)
						return RCBAD;		// error, code buffer full, etc
					// can't konstize here: eg select a select arg cond-value pair is not a value, might jmp yonder.
					pspe->ty = TYFL;
				}
			}
			havTys = (havTys & ~(TYSI|TYINT)) | TYFL;	// combined type: now have floats, no ints
		}
	}

// Convert strings to choice values if have any numbers or choices and have choiDt.  For TYCH or TYNC end result.

	if (havTys & (TYNUM|TYCH))		// if have numbers or already-conv choices, strings only compat if conv to choices.
	{
		wanTy &= ~TYSTR;				// make expTy convert addl strings to choices (or error) for caller
		if (choiDt && (havTys & TYSTR))		// if have strings and data type for conversion to choices
		{
			for (int i = 0; i < n; i++)		// loop n most recent parStk frames
			{
				PARSTK* pspe = parSp - i;
				if (pspe->ty==TYSTR)		// if string, choice it
				{
					if (cnvPrevSf( i, PSSCH, choiDt))	// insert ops after ith expr back, or just converts constant in place
						return RCBAD;
					// can't konstize here: eg select a select arg cond-value pair is not a value.
					pspe->ty = TYCH;
				}
			}
			havTys = (havTys & ~TYSTR) | TYCH;	// update combined type
		}
	}

	/* clear more incompatible bits in caller's wanTy.      For parsing addl args: to maximize conversions in expTy, where they
							        get konstize'd b4 fcnAgs adss the jmps which prevent konstize here. */
	if ((wanTy & TYNC) != TYNC)
	{
		if ((havTys & TYNUM))
			wanTy &= ~(TYSTR|TYCH);  	// strings/choices cannot be made compatible w numbers w/o TYNC
		else if (havTys & (TYSTR|TYCH))
			wanTy &= ~TYNUM;			// numbers cannot be made compat w strings/choices without TYCN

		// may clear bits already seen (ok: caller about to errMsg incompatibilities). "else" prevents clearing to 0 (insurance).
	}
	if (pWanTy)
		*pWanTy = wanTy;

	// caller must check for all types same or TYFL combined with TYCH, producing TYNC.

	return RCOK;
}		// tconv
//==========================================================================
LOCAL RC FC utconvN( 		// do "usual type conversions" to match last n NUMERIC exprs on parStk

	SI n, 		// # exprs to match
	char *tx, 		// text of operator or fcn name, for errMsgs
	SI aN )		// 0 or fcn argument number, for errMsgs (-1 for fcn, unspecified arg #)
{
	ERVARS1    PARSTK *pspe;   SI i, haveFloat = 0;

	ERSAVE
	printif( trace," utconvN ");

// check that all expressions are numeric / see if any are floats

	for (i = 0; i < n; i++)
	{
		pspe = parSp - i;
		switch (pspe->ty)
		{
		case TYFL:
			haveFloat++;
			break;
		case TYSI:
			break;
		default:
			rc = perNx( MH_S0061,		// "Numeric expression required%s". suspect can't now occur ('90..2-92)
			aN==0  ?  i==0 ? after(tx,0)
			: before(tx,0)			// operator: args b4 and after
				:  asArg(tx, aN > i ? aN-i : -1 )   );	// fcn: use arg # aN-i
			goto er;							// label in ERREX macro
		}
	}

// if expressions not all SI, convert any SI's to FLOAT

	if (haveFloat)
	{
		for (i = 0; i < n; i++)
		{
			pspe = parSp - i;
			if (pspe->ty==TYSI)
			{
				EE( cnvPrevSf( i, PSFLOAT2, 0) )	// insert conversion op after ith expr back (or just float it if constant)
				pspe->ty = TYFL;
			}
		}
	}

	// don't konstize() here -- eg creates problems with select (jmps) 10-90.

	return RCOK;

	ERREX(utconvN)
}			// utconvN

//==========================================================================
RC FC konstize(		// if possible, evaluate current (sub)expression (parSp) now and replace with a constant load

	SI* pisKon,	// NULL or receives non-0 if is constant
	void **ppv,	// NULL or, if constant, receives ptr to value (ptr to ptr for TYSTR).  Volatile: next konstize will overwrite.
	SI inDm )	// for TYSTR: 0: put text inline in code;
				// application of cases 1/2 not yet clear 10-10-90 no non-0 inDm values yet tested
				// 1: put actual given pointer inline in code CAUTION;
				// 2: copy text to dm and put that pointer in code.

// 2-91: why didn't it perNx on cueval error (place noted below)?
{
	ERVARS1
	void* p = NULL;
	char *q=NULL;
	const char* ms;
	SI isKon = 0;
	PSOP jmp;

	ERSAVE
	switch (isKonExp(&p))		// test for top parStk frame being constant (next)
	{
	default:
		//case 0:			// not constant or not a value or has side effect.
		printif( kztrace, " n0 ");
		break;			// isKon is 0.

	case 1:   		// just a constant (no reduction possible)
		isKon++;	//  NB isKonExp sets p in this case (only).
		printif( kztrace, " n1 ");
		break;

	case 2: 			// constant expression, but not last subexpr emitted
		printif( kztrace, " n2 ");
		break;			// unexpected.  ret as tho not k as don't have p.

	case 3: 			// constant expression that can be evaluated now and replaced in place with a constant

		if (psp==parSp->psp2)		// only do if last subexpr (insurance here)
			/* (else deal with length change, restore PSOP that final PSEND overwrites,
			   check space for inline string (inDm==0) AFTER eval, save/restore psp.) */
			// if need found: don't do if it has a trailing jump (or terminate, evaluate, move/restore).
		{
			printif( kztrace,  " y%d  ", parSp->ty);
			isKon++;					// say is constant

			// get value
			rc = cuEvalR( parSp->psp1, &p, &ms, NULL); 	// Evaluates, & rets ptr to value (ptr to ptr to TYSTR). cueval.cpp.
			// TYSTR CAUTIONs: 1) addl indirect lvl; 2) text may be INLINE IN CODE.
			if (rc)			// if error occurred
			{
				if (ms)			// if cuEvalR ret'd msg to complete & issue
				{
#if 1	// was in use; keep
					perlc(		// issue parse errMsg: prefix the following w input text line, file name, line #, caret.
						ms );		// sub-message formatted by cuEvalR
#else	// 2-92 idea, decided not to do yet.
*	// if wanna show what being evaluated, at least re choice conversion errors, try this:
*		   if (ermTx)						// if have description of whats being evaluated
*		   {	char *part1 = strtprintf( "%s: ", ermTx);		// put it b4 cuEvalR's msg.  "While determining %s: "?
*		      // issue parse errMsg: perlc prefix's the following w input text line, file name, line #, caret.
*		      perlc( "%s%s%s",					// assemble message, with conditional newline
*		              part1,
*			      strJoinLen(part1,ms) > getCpl() ? "\n    " : "",	// newline if line would be too long
*		              ms );							// sub-message formatted by cuEvalR
*			}
*		   else							// no description
*		      perlc(ms);					// issue cuEvalR's message, with file/line/carat.
#endif
				}
				// if ms==NULL assume cuEvalR issued message (unexpected).
				perNx(MSGORHANDLE());		// skip input to next statement.   ** believe perNx appropriate, but not here til 2-91.
				goto er;		// error exit like EE macro
			}

			// be sure string not in space to be overwritten
			if (parSp->ty==TYSTR)
			{
				q = cuStrsaveIf(*(char **)p);		// dup if in code, cueval.cpp
				p = &q;					// p points to pointer
			}
			// preserve unfilled jmp at end expression (e.g. in choose() args)
			jmp = *(parSp->psp2-2);			// possible jump; save code
			if ( *(parSp->psp2-1) != 0xffff  		// if no "unfilled" offset
			|| isJmp(jmp)==0 )  			// or not a jmp opcode
				jmp = 0;				// say no jmp to restore

			// overwrite new pseudo-code to load constant
			psp = parSp->psp1;					// deallocate present code
#ifndef EMIKONFIX	//looks like p gets one too few indirections 2-94, but leave unchanged til problem demonstrated
			EE( emiKon( parSp->ty, p, inDm, (char **)&p))	// updates p to where text put, for return
#else			//easiest to fix in emiKon; change cast here.
*             EE( emiKon( parSp->ty, p, inDm, (char ***)&p))	// updates p to where text put, for return
#endif
			if (jmp)						// if have jmp to restore
			{
				EE( emit(jmp))				// emit the jump-type opCode
				EE( emit(0xffff))			// unfilled displacement
			}
			parSp->psp2 = psp;				// set new end+1 pointer

			dmfree( DMPP( q));   			// free copied string if any
		}
		// else not last emitted. unexpected.  ret as tho non-k as don't have p.
		else printif( kztrace, " nyet ");

	}	// switch

// return info if pointers given
	if (pisKon)
		*pisKon = isKon;
	if (isKon)
		if (ppv)
			*ppv = p;

	return RCOK;		// more returns above in EE macros

	ERREX(konstize)
}		// konstize

//==========================================================================
LOCAL SI FC isKonExp(		// test if *parSp is a constant expression

	void **ppv )	// NULL or rcvs ptr to value in return 1 case ONLY (rcvs char * for TYSTR; CAUTION 1) VOLATILE 2) into code)

/* returns: 0: not constant, not a value, refs dummy arg, or has side effects
	    1: just a constant (no reduction possible)(*ppv points to value)
	    2: constant expression, but not last subexpression emitted
	    3: constant expression that can be evaluated now and replaced in place with a constant */
{

// test if has side effect -- if so, better do at run time
	if (parSp->did)			// if contains assignment, fcn w side effect, etc
		return 0;

// test if requires evaluation multiple times or later than during compilation
	if (parSp->evf != 0)		// or contains dummy ref (EVFDUMMY)
		return 0;			// some element in expr not constant

   static const char* p;	// persistent return for string pointer
							//   see return 1 case. RISKY? see issue #437

// cases by type
	PSOP kop;
	USI szkon{ 0 };

	switch (parSp->ty)
	{
		/* case TYANY:  TYNUM:  TYNONE:  TYDONE: */
	default:
		return 0;		// not expr or not sure of type

	case TYSI:
		kop = PSKON2;			// op code
		szkon = sizeof(PSOP) + sizeof(SI);	// space needed
		break;
	
	case TYINT:
		kop = PSKON4;
		szkon = sizeof(PSOP) + sizeof(INT);	// space needed
		break;

	case TYSTR:
#if defined( USE_PSPKONN)
		if (*parSp->psp1==PSPKONN)
		{
			kop = *parSp->psp1;
			szkon = (2 + ~*(parSp->psp1+1)) * sizeof(PSOP); // right?
			break;
		}	// else is load using pointer.  fall thru.
		/*lint -e616 */
#else
		kop = PSKON4;			// op code
		szkon = sizeof(PSOP) + sizeof(CULSTR);
		break;
#endif
	case TYFL:
		kop = PSKON4;			// op code
		szkon = sizeof(PSOP) + sizeof(float);
		break;

	case TYCH:
		kop = PSKON2;
		szkon = sizeof(PSOP) + sizeof(SI);
		if (*parSp->psp1 != kop)
		{
			kop = PSKON4;
			szkon = sizeof(PSOP) + sizeof(float);
		}
		break;
	}  /*lint +e616 */

// test if already just a constant
	
	USI sz = USI((char *)parSp->psp2 - (char *)parSp->psp1);
	if (sz==szkon)			// if right size incl args
	{
		if (*parSp->psp1==kop)		// if op code already constant-load
		{
			if (ppv)			// if not NULL, tell caller
#if defined( USE_PSPKONN)
				if (kop==PSPKONN)			// string inline in code
				{
					p = (const char *)(parSp->psp1 + 2);	// is after op code & length
					*ppv = &p;			// indirect like cuEvalR()
				}
				else			// other types [or string ptr in code]
#endif
					*ppv = parSp->psp1 + 1;	// point to value after op code
			return 1;			// constant, but nothing to convert
		}
	}

// test if not last sub expression parsed
	if (parSp->psp2 != psp)
		return 2;
	/* if this really occurs, make calling code in konstize() deal with length change,
	   restoring PSOP that final PSEND overwrites, and saving/restoring psp.  Then here use: */
	//test if too little space to replace in place with a constant
	// if (sz < szkon)		// too little space to overwrite
	//    if (parSp->psp2 != psp)	// and not at end of gen'd code
	//       return 0;     		// cannot convert in place
	// plus additional tests (after eval?) re PSPKONN for inDm==0.
	// ?? plus addl logic to permit changing pointed-to string to inline?

// this subexpression ok to evaluate now and replace in place with constant
	return 3;
}		// isKonExp

//==========================================================================
LOCAL USI FC maxEvf( USI evf)

// clear bits to right of hiest (ruling) evf bit(s) where these bits represent subdivisions of bit to left.

// possible returns 0 (constant) or each evf bit alone
//     EVFDUMMY always preserved; EVXBEGIVL bits preserved unless constant.
{
	USI evf1 = evf & ~EVXBEGIVL;		// search for bits other than stage modifier flags
	for (USI bit = EVFMAX;  bit;  bit >>= 1)	// from hiest bit in evf word (cuevf.h) thru bit with value 1
		if (bit & evf1)				// if this bit on
			return bit | (evf & (EVXBEGIVL | EVFDUMMY));
					// return the bit
					// preserve stage flags and "contains dummy arg ref -- don't konstize" flag

	return evf & EVFDUMMY;		// no bit found: constant. preserve EVFDUMMY
    							//  discard (unexpected) EVXBEGIVL stage bits on constant
}		// maxEvf
//==========================================================================
LOCAL USI FC cleanEvf( 		// clean up eval frequency bits
	USI evf, 			// value to clean up
	USI _evfOk )		// evf bits ok in current context, or 0 to suppress cond'l changes to MH.

// eliminates redundant ones, eg month with day;
// changes monthly plus hourly to monthly-hourly, if appropriate for context 9-95;
// conditionally changes monthly-hourly + other to monthly plus hourly

// possible returns 11-95: 0; each evf bit alone; each bit with EVENDIVL. Also EVFDUMMY returned if given.
{
#if 0	/* 11-25-95 make functions work: EVFDUMMY normally gets here.
x           Rest of this fcn appears to pass EVFDUMMY thruough; cleanEvf changed to do so; also changes in expr, exOrk. */
x    if (evf & EVFDUMMY)
x       perlc( MH_S0062);		// "Internal error in cuparse.cpp:cleanEvf: Unexpected 'EVFDUMMY' flag"
#endif

#if 0 // historical. problem: converted MON|HR to MH in context where HR was appropriate (BUG0066).
x
x// merge in and re-separate EVFMH as caller may have merged bits: so DAY|MH --> MON|DAY|HR;  MON|HR --> MH.
x    if (evf & EVFMH)    					// monthly-hourly (not daily)
x       evf = (evf & ~EVFMH) | EVFMON|EVFHR;			// combine
x    if ((evf & (EVFMON|EVFDAY|EVFHR))==(EVFMON|EVFHR))  	// if MON && HR &&!DAY
x       evf = (evf & ~(EVFMON|EVFHR)) | EVFMH;			// separate
x
#else //9-16-95 make context-dependent; also evfOk arg added, calls changed, cuevf.h changed.

// change MON|HR without DAY to MH if context accepts MH, eg for solar values used hrly on 1st day month only.
// leave unchanged in non-MH contexts so expr using MN and HR comes out HR (computed on hrly all days), 9-16-95.
	if ((evf & (EVFMON|EVFDAY|EVFHR))==(EVFMON|EVFHR))  	// if MON && HR &&!DAY
		if (_evfOk & EVFMH)					// if context accepts MH (eg a shading coeff) 9-95
			evf = (evf & ~(EVFMON|EVFHR)) | EVFMH;		// turn off month and hour, turn on monthly-hourly

// change HR to MH if context allows only the latter,
// so hourly expr used for a solar input works & is only computed on days needed (HR removed from VMHLY in cuevf.h at same time).
	if ((evf & (EVFMON|EVFDAY|EVFHR))==EVFHR)			// if HR and not daily nor monthly
		if ((_evfOk & (EVFHR|EVFMH))==EVFMH)			// if context accepts MH but not HR
			evf = (evf & ~EVFHR) | EVFMH;

// change MH|DAY to HR|DAY|MON cuz must eval hourly every day: makes result hourly where fallthru wd yield MH.
	if ((evf & (EVFDAY|EVFMH))==(EVFDAY|EVFMH))  		// if have both daily and monthly-hourly
		evf = (evf & ~EVFMH) | EVFMON|EVFHR;			// clear MH, set individual monthly and hourly bits, leaving day.

	/* note MH where not allowed is NOT now (9-95) converted to MON|HR,
	   cuz believe most actual probable values set hrly only on 1st day month. */
#endif

	/* if we have non-subdivision bits such as month and week, must separate,
	   find hiest of each (include non-confilicting bits in both), re-merge */

// find most significant bit (shortest interval); return it, any EVXBEGIVL bits, EVFDUMMY if on
//   note MH and DAY are adjacent bits and not now both on.
	return maxEvf(evf);			// just above
}			// cleanEvf
//==========================================================================
LOCAL RC FC cnvPrevSf( 	// append (conversion) operation to ith previous expression, or NOPs for later conversions.

	SI i, 		// how many expressions back, 0 ok.
	PSOP op1, 		// operation, eg PSSCH, PSFLOAT, PSNCN. Also used to emit PSNOPs to hold space for poss later conversions.
	PSOP op2 )		// 0 or 2nd thing to emit, eg choiDt.

// code is added BEFORE incomplete jump, if any, at end of parStk frame
// special cases handled for tconv/utconvN: if previous expression is integer constant, replaces it with float constant;
//					    if string constant being converted to choice, replaces with choice constant (or errmsg)
//					    if nc being checked for numeric is const choice, error now
// CALLER updates .ty to reflect new type
{
	RC rc;
	SI nnops = 0; 			// number of PSNOPs removed from end of frame: 0 indicates no pre-resolved jmp.
	PSOP op3 = 0; 			// 0 or internally generated 3rd thing to emit
	PARSTK *parSpe = parSp - i;		// stack frame to append to
	PSOP *pspe = parSpe->psp2;		// end of its code
	SI xspace = op2 ? -2 : -1;		/* space needed for # opcodes to omit, negative for space to be found or made,
					   becomes positive if space is left over. */

// test for converting NC (number-choice) constant to numeric and do now in place (accept if number, error if not, no op store)

	// not sure if there is yet any way for a numeric constant to become TYNC; good insurance 2-92.
	if (op1==PSNCN) 			// if being asked to store "convert nc to number (ie error if choice) at run time"
		if ( *(pspe-3)==PSKON4		// if frame contains 4-byte constant
		 &&  pspe-3==parSpe->psp1 ) // and nothing more
		{
			if (!ISNUM(*(void **)(pspe-2)))			// test the constant value: if any non-number (cnglob.h macro)
				if (ISNCHOICE(*(void **)(pspe-2)))			// conversion fails NOW
					return perNx( MH_S0063);		// "Expected numeric value, found choice value".  Explain to user.
				else
					return perNx( MH_S0064);   		// "Numeric value required".  (unexpected) NANDLE or bug.
			return RCOK;						// its already a number, needn't store conversion
		}
	// else: LATER 2-92 consider looking for single probe in stack frame,
	//       changing it to new probe op that checks for numeric, cuz cd do good error msg showing probed vbl name.
	// else: fall thru to store conversion

// test if code ends with (unfilled) jmp: if so, set to insert before it
	PSOP* jmpLoc = 0;	// info on unfilled jump at end of code -- insert before it
	SI sizeJmp = 0;
	if ( *(pspe-1)==0xffff  &&  isJmp(*(pspe-2)) )
	{
		pspe -= 2;					// insert before jmp
		jmpLoc = pspe;				// save ptr to jmp
		sizeJmp = 2;				// say move the 2-PSOP jmp below
	}

// test for nop place holder(s) to overwrite (emitted when necessary to resolve jmps before resolving all type conversions)

	while ( *(pspe-1)==PSNOP   		// overwrite start of nops if more present than needed
	&&  parSpe->nNops > 0			// only if known to be nop (not int const or other operand of same value) 10-95
	&&  pspe > parSpe->psp1 )		// insurance
	{
		pspe--;
		parSpe->nNops--;
		xspace++;  				// 1 more slot of avail space
		nnops++;  				// count nops removed
	}

// test for converting lone string constant to choice: replace with choice constant 1) to error now 2) smaller 3) faster

	if (op1==PSSCH  &&  op2)			// if request for string-choice conversion
	{
		PSOP* psp1 = parSpe->psp1;		// point start of frame
#if defined( USE_PSPKONN)
		if (*psp1==PSPKONN)			// else not string constant
		{
			USI slen = ~*(psp1+1);		// number of WORDS following PSPKONN and self, stored 1's complemented
			if (psp1 + 2 + slen == pspe)		// if string constant takes up whole block -- else not alone, don't convert now
			{
				ULI v;						// rcvs value, including special choicn hi bits
				USI sz;						// rcvs size, 2 or 4
				MSGORHANDLE ms;				// rcvs msg insert: "'%s' not one of x y z ..."
				if (cvS2Choi( (char *)(psp1+2), choiDt, (void *)&v, &sz, &ms)==RCBAD)	// get choice value for string, cvpak.cpp
					return perNx( MH_S0065, ms); 		// "%s\n  (or perhaps invalid mixing of string and numeric values)"
				ms.mh_Clear();	// insurance, drop possible info msg from cvS2Choi
				// code to output (on fallthru); matches emiKon.
				op1 = PSKON2;							// op code for 2-byte constant
				op2 = (USI)v;							// 2 bytes of choice value
				//xspace -= 2;							need 2 psop words for this. xspace set -2 at entry.
				if (sz==4)
				{
					op1 = PSKON4;     // differences for 4-byte value choicn
#if CSE_ARCH != 32
					printf("\nFix cnvPrevSf shift");	// TODO64 shift looks dubious on 64 bit
#endif
					op3 = (USI)(v >> 16);
					xspace--;
				}
				pspe = psp1;							// set to OVERWRITE the string constant
				xspace += slen + 2;   						// space of string constant being vacated

			}
		}
#endif
	}	// if any conditions false, fallthru emits runtime conversion of general string value to choice value

// make space to add codes if needed

	if (xspace < 0)
	{
		SI make = -xspace;						// positive # PSOP spaces to make: less confusing
		if (i  &&  parSpe->psp2 != (parSpe+1)->psp1)
			return perNx( MH_S0066);  		// "Internal error in cuparse.cpp:cnvPrevSf: expressions not consecutive"
		// don't know what to move where
		CSE_E( movPsLastN( i, make) )					/* move last i exprs (parSp-(0..i-1)) to make
       									   space for 'make' PSOPs b4 them. terminates. */
		memmove( pspe, pspe-make, (make+sizeJmp)*sizeof(PSOP) ); 	// move jmp and PSNOPs (in case extras) up to make space
		jmpLoc += make;							// maintain pointer to jmp
		parSpe->psp2 += make;						// point new end expr's code

		// xspace -= make;  or  xspace = 0;	not needed cuz xspace only has effect below if > 0, which won't occur here.
	}

// emit caller's code(s)

	*pspe++ = op1;  			// 'xspace' value already reflects stored opcodes -- see initialization above
	if (op2)  *pspe++ = op2;
	if (op3)  *pspe++ = op3;		// do not terminate at pspe: overwrites following code. movPsLastN terminated at psp, 2-91.

// if just floated an integer constant, replace with float const

	// (coding ASSUMES sizeof( SI const + PSFLOAT) = sizeof(float const)
	if ( op1==PSFLOAT2			// *pspe: just stored
	&& *(pspe-3)==PSKON2		// 2-byte const op used for int consts
	&& pspe-3==parSpe->psp1 )		// 2 codes + op only (be sure PSKON2 not tail of something else)
	{
		*(pspe-3) = PSKON4;		// change to 4 byte constant op code
		*(float *)(pspe-2)		// overwrites int const and PSFLOAT
		= (float)*(SI *)(pspe-2);	// float value: convert the int
		printif( kztrace, " yyes ");
	}

// dispose of extra space (after string to choice conversion)

	if ( xspace > 2  		// compress only if > 2 bytes left over (leave 2 nops (needed?))
	&&  !nnops  		// safe to compress if no nops overwritten (hence no pre-resolved jmp)
	&&  !i )			// save to compress if is last frame (jmp poss in other frame? may be unnec restriction 2-92)
	{
		if (pspe+xspace+sizeJmp != parSpe->psp2)
			perlc( MH_S0067, pspe+xspace, parSpe->psp2 );	// "Internal error in cuparse.cpp:cnvPrevSf:\n pspe+.. %p != psp2 %p"

		*pspe++ = PSNOP;
		*pspe++ = PSNOP;			// leave 2 nops 2-92 (insurance ?? any need for this)
		parSpe->nNops += 2;
		xspace -= 2;			// ..
		if (sizeJmp)
		{
			memmove( pspe, jmpLoc, sizeJmp * sizeof(PSOP));	// move jmp
			//pspe += sizeJmp;					no further use 12-94
		}
		CSE_E( movPsLastN( i, -xspace) ); 			// check, terminate, adjust psp, move following exprs if i != 0 gets here
		parSpe->psp2 -= xspace;				// now point new end this expr's code
	}
	else					// not sure save to compress, or <= 2 bytes left over
		while (xspace-- > 0)  			// fill extra space with nops
		{
			*pspe++ = PSNOP;
			parSpe->nNops++;
		}

	return RCOK;				// addl good ret(s) above, incl "constant is right type - conversion not stored"
	// error returns above
}				// cnvPrevSf
//==========================================================================
RC FC newSf()		// start new parse stack frame: call at start statement or (sub)expression
{
// overflow check
	if ((char *)parSp >= (char *)parStk + sizeof(parStk) - sizeof(PARSTK) )
		return perNx( MH_S0068 );  		// "Parse stack overflow error: expression probably too complex"

// terminate current frame if any
	/* .psp2 is used to append conversions and for consistency checks.
	   Currently 8-90 believe .psp2 redundant: always == next .psp1, or psp if top. */
	if (parSp >= parStk)	// if parse stack not empty
		parSp->psp2 = psp;	// set frame's "end pseudocode" pointer to current end of pseudocode.

// allocate and fill new frame
	parSp++;  			// point next frame
	parSp->ty = TYNONE;  	// data type: no value yet
	parSp->evf = 0;		// evaluation frequency: init to 'constant'.
	parSp->did = 0;		// nothing done yet
	parSp->nNops = 0;		// no trailing nops yet. 10-95.
	parSp->psp2 		// ps code end same as start (better than random)
	= parSp->psp1  		// pseudo-code start: used re insertions, conditionals, errors, & to access compiled code.
	= psp;			// global pseudo-code store pointer
	return RCOK;
}			// newSf
//==========================================================================
RC FC combSf()			// combine top two parStk frames, with second frame dominant

/* used: for args to binary op,
         to combine (no code expected) unary op frame with following arg,
         to combine fcn arg expressions, statements in program, etc. */

/* pscode expected to be consecutive.
	merges .evf's and .did's.
   .ty: combinations of TYFL, TYCH, TYNC form TYNC, else uses 2nd .ty -- caller must fix any other exceptions. */
{
	PARSTK* parSp1 = parSp-1;	/* pointer to 1st entry */

	if (parSp1->psp2 != parSp->psp1)
	{
		/* 2nd expr not right after 1st --> don't know what to move */
		return perNx( MH_S0069);  	// "Internal Error in cuparse.cpp:combSf: expressions not consecutive"
	}
	parSp1->evf |= parSp->evf;			// eval frequency: most frequent
	parSp1->did |= parSp->did;  		// merge "did something" flags
	parSp1->nNops = parSp->nNops; 		// use # nops at end 2nd frame only. 10-95.
	parSp1->ty = (parSp1->ty|parSp->ty)==TYNC 	// result data type: if combining TYNC/TYCH/TYFL,
	?  TYNC			//  use 2-bit type TYNC===TYFL|TYCH,
	:  parSp->ty;     		//  else use 2nd arg's; caller must fix other exceptions.
	parSp1->psp2 = parSp->psp2; 		// end is 2nd end. psp1 unchanged
	parSp--;    				// pop -- delete 2nd entry

	return RCOK;		// error return above
}		// combSf
//==========================================================================
RC FC dropSfs(    	// discard parse stack frame(s)

	SI k, 	// number of frames to keep on top of dropped frames
	SI n )	// number of frames to drop

// adjusts parSp and psp.
// if (k), moves parStk frames, code, adjusts .psp1's and 2's.

// for use to eliminate dead code when constant found in conditional.
{
	PARSTK * d1, * dn, * k1, * p;   USI psMove;

	if (n < 0 || k < 0)					// devel aid
		return perNx( MH_S0070, k, n);	// "Internal error: bad call to cuparse.cpp:dropSfs( %d, %d)"
	if (n==0)						// rif nothing to drop
		return RCOK;					// (but k=0 is valid)
	//kn = parSp;			// last frame being kept: just use parSp
	dn = parSp-k;  			// inclusive last parStk frame being dropped
	k1 = dn+1;  			// first parStk frame being kept (if k > 0)
	d1 = k1-n;				// first parStk frame being dropped
	psMove = (USI)(dn->psp2 - d1->psp1);  	// - how far code moves
	if (k > 0  &&  dn->psp2 != k1->psp1)
		err( PWRN, MH_S0071);		// display internal err msg "confusion (1) in dropSfs"

// move stuff being retained into space of dropped stuff
	memmove( d1->psp1, dn->psp2, 					// move code AND TERMINATOR
	(USI)((char *)(parSp->psp2 + 1) - (char *)dn->psp2) );
	for (p = k1; p <= parSp; p++)					// adj ptrs
	{
		p->psp1 -= psMove;
		p->psp2 -= psMove;
	}
	//if (k > 0)
	memmove( d1, k1, k*sizeof(PARSTK) );		// move frames

// discard top of parStk and and of code buffer
	parSp -= n;
	if (parSp->psp2 + psMove == psp)
		psp -= psMove;			// free space of dropped code
	else				// bug or non-contiguous code
		err( PWRN, MH_S0072);	// display internal err msg "confusion (2) in dropSfs"
	return RCOK;
}		// dropSfs

//==========================================================================
LOCAL RC FC movPsLastN(

// make space for insertion before last n subexpressions

	SI nFrames,	// number of subexpressions (parStk frames) to move, 0 ok, < 0 untested but intended to work 2-92.
	SI nPsc )	// size of insertion (# pseudo-codes): code moved this much, also added to psp.

// caller adjusts psp2 of expr n-1 AFTER return
{
	SI i;   RC rc;

// checks
	if (psp != parSp->psp2)
		return perNx( MH_S0073);   		// "Internal error in cuparse.cpp:movPsLast: expression not last in ps."

	for (i = 1; i < nFrames; i++)			// nFrames-1 checks
		if ((parSp-i)->psp2 != (parSp-i+1)->psp1)
			return perNx( MH_S0074);   		// "Internal error in cuparse.cpp:movPsLast: expressions not consecutive"

	while (psp + nPsc >= pspMax)			// if will overflow buffer
		CSE_E( emiBufFull() )  				// reallocate (future) or issue msg; if failed to enlarge, ret bad.
		// if ret'd ok (future), loop to see if now big enuf.

// execute
		psp += nPsc;   			// incr pseudoCode output ptr so next code output does not overwrite the moved code
	if (nPsc > 0)			// if inserting space (normal case)
	{
		*psp = PSEND;			// keep terminated; insurance here
		for (i = 0; i < nFrames; i++)	// from last expr back, ...
			CSE_E( movPs( parSp-i, nPsc) )	// move expr's code, next
		}
	else if (nPsc < 0)			// if closing a hole, do in opposite order (poss future use in cnvPrevSf, 2-92)
	{
		for (i = nFrames; --i >= 0; )	// from first expr to move forward, ...
			CSE_E( movPs( parSp-i, nPsc) )	// move expr's code, next
		}
	*psp = PSEND;			// keep terminated, even if 0 exprs moved 2-91
	return RCOK;
}				// movPsLastN
//==========================================================================
LOCAL RC FC movPs( PARSTK *p, SI nPsc)	// move pseudo-code for subexpression to permit inserting nPsc PSOP's b4 it

// caller must adjust psp!
{
	RC rc;

	/* *** check assumes expr is in same buffer *psp: *** */
	while (p->psp2 + nPsc >= pspMax)  		// check if buffer would be filled
	{
		CSE_E( emiBufFull() )  			// reallocate (future) or issue msg; if failed to enlarge, ret bad.
		// else loop to see if now large enuf
		// *** reallocator will have to adjust all ptrs in parStk ***
		// *** callers such as cnvPrevSf will have to be recoded to repoint
	}
	memmove( p->psp1 + nPsc,
	p->psp1,
	(USI)((char *)(p->psp2 + 1) - (char *)p->psp1) );		// move includes possible PSEND at *psp2.
	p->psp1 += nPsc;
	p->psp2 += nPsc;
	return RCOK;
}			// movPs

//==========================================================================
LOCAL RC FC fillJmp( SI i)			// fill jmp address

/* fill jump address at end ith PRIOR parStk frame with offset to CURRENT psp,
   ie complete jump to after current block.  i = 1 for previous block. */
{
	PSOP *where;

	where = (parSp-i)->psp2 - 1; 	// last PSOP of prior block
	if (*where != 0xffff)		// 0xffff was emitted to hold space

		return perNx( MH_S0075 );   	// "Internal error in cuparse.cpp:fillJmp: address place holder not found in Jump"

	*where = (PSOP)(psp - where);	// offset from own location to location of NEXT op emitted
	return RCOK;
}			// fillJmp
//==========================================================================
LOCAL RC FC dropJmpIf()		// delete unfilled trailing jmp in top parStk frame, if any
{
	PSOP *pspe;

	pspe = parSp->psp2;				// point end frame's code
	if ( *(pspe-1)==0xffff  &&  isJmp(*(pspe-2)) )	// if unfilled jmp
	{
		*(pspe-2) = *pspe;			// move terminator (etc)
		parSp->psp2 -= 2;			// truncate frame's code space
		if (psp==pspe)				// != unexpected
			psp -= 2;				// free global code space
		else
			err( PWRN, MH_S0076);		// display internal err msg "confusion in dropJmpIf"
	}
	return RCOK;
}		// dropJmpIf
//==========================================================================
LOCAL SI FC isJmp( PSOP op)	// test op code for being a jump
{
	return op==PSJMP || op==PSPJZ || op==PSJZP || op==PSJNZP;
}		// isJmp
//==========================================================================
RC CDEC emiKon( 			// emit code to load constant

	USI ty, 	// type TYSI TYFL TYSTR TYCH (TYCH constants also generated directly in cnvPrevSf)
	void *p, 	// ptr to value (for TYSTR, ptr to ptr to text)
	SI inDm,	// for TYSTR: 0: put text inline in code;
				// application of cases 1/2 not yet clear 10-10-90 no non-0 inDm values yet tested
				//    1: put actual given pointer inline in code CAUTION!;
				//    2: copy text to dm and put that pointer in code.
#ifndef EMIKONFIX
	char **pp )	// NULL or receives ptr to text inline in code for TYSTR only (feature for konstize)
#else //if apparent bug really found, use this.
*    char ***pp )	// NULL or receives ptr to (transient!) ptr to text inline in code for TYSTR only (feature for konstize)
*					//  and change declaration and check call in cuprobe.cpp.
#endif

// for TYCH, size 2 or 4 bytes is determined by bits in file-global choiDt.
// (if this does not work out, have caller give size in inDm arg, 2-92)
{
	ERVARS1
#ifdef EMIKONFIX
*    static char **p1;
#endif

	ERSAVE
	printif( trace," emiKon ");

	switch (ty)					// emit load op code
	{
	case TYCH:  						/* coord changes w/ cnvPrevSf: emits same code
          							   w/o calling here, when const string conv to choice.*/
		if (choiDt & DTBCHOICN)  goto fourBytes;		// determine choice size FROM FILE-GLOBAL choiDt
		if (choiDt & DTBCHOICB)  goto twoBytes;		// ..
		rc = perNx( MH_S0077, choiDt);	// "Internal Error in cuparse.cpp:emiKon:\n" ...
		goto er;		                        // "    no recognized bit in choiDt value 0x%x"

	case TYSI:
twoBytes:
		EE( emit(PSKON2) )  		// op code
		EE( emit2(*(SI*)p) )		// value inline after opcode
		break;

	case TYSTR:
#if defined( USE_PSPKONN)
		if (inDm==0)				// string inline
		{
			// pseudoCode: PSPKONN ~nwords text
			// PSPKONN: pushes pointer to text at run time
			//   ~nwords: # words in string, as a word, excl self, ONE'S COMPLEMENTED
			//   (so unlike a dm block from strsave -- see cueval.cpp:cupfree()).
			// string:  text, with null, padded to whole word.
			EE(emit(PSPKONN))
			const char* pLit = *(const char**)p;
			int litLen = strlenInt(pLit);
			EE(emit(~(((PSOP)(litLen) + 1 + 1) / sizeof(PSOP))))
#ifndef EMIKONFIX
			char* p1 = (char *)psp;		// where text will be
#else//to fix apparent konstize bug
*			p1 = (char **)&psp;		// ptr to ptr to where text will be
#endif
			EE( emitStr( pLit, litLen) )
			if (pp)				// if req'd, ret text destination ptr ptr now.
				*pp = p1;				// not sooner: might overwrite p.
			break;
		}
		else if (inDm==2)				// copy string to dm
#endif
		{
			if (inDm != 0)
				printf("\ninDm = %d", inDm);
			char* q = strsave(*(char **)p);		// and fall thru
			p = &q;
		}
		// else if inDm==1, use given p and fall thru
		[[fallthrough]];
	case TYFL:
	case TYINT:
fourBytes:
		EE( emit(PSKON4) )
		EE( emit4( (void**)p) )
		break;

	default:
		rc = perNx( MH_S0078);
		goto er;   	// "Bug in cuparse.cpp:emiKon switch"
	} /*lint -e616 */

	return RCOK;
	ERREX(emiKon)
}		// emiKon
//==========================================================================
LOCAL RC FC emiLod( USI ty, void *p)	// emit code to load datum of type ty from p
{
	ERVARS1

	ERSAVE
	printif( trace," emiLod ");

	switch (ty)				// emit load op code
	{
	case TYSI:
		EE( emit(PSLOD2) )  break;

	case TYSTR: 					// get a CULSTR.  >> need to duplicate string storage?
	case TYINT:
	case TYFL:
		EE( emit(PSLOD4) )  break;

	default:
		rc = perNx( MH_S0079);
		goto er;  	// "Bug in cuparse.cpp:emiLod switch"
	} /*lint +e616 */

	EE( emitPtr( &p) )			// address of variable follows op code

	return RCOK;
	ERREX(emiLod)
}			// emiLod
//==========================================================================
#ifdef LOKFCN
*LOCAL RC FC emiSto( SI dup1st, void *p)	// emit code to store datum on run stack top at p
*
*{
*    ERVARS1
*
*    ERSAVE
*    printif( trace," emiSto(%d,%p) ", dup1st, p);
*
*    if (dup1st)				// on flag, first dup value on stack
*       EE( emiDup() )			// ... for nested sets
*
*    switch (parSp->ty)			// emit store op code, last expr's type
*    {
*	  case TYSI:  EE( emit(PSSTO2) )  break;
*
*	  case TYSTR: 				// set a char *.  >> need to duplicate string storage?
*	  /*lint -e616 case falls in*/
*	  case TYFL:  EE( emit(PSSTO4) )  break;
*
*	  default:    rc = perNx( MH_S0080);  goto er;   	// "Bug in cuparse.cpp:emiSto switch"
*} /*lint -e616 */
*
*    EE( emit4( &p) )			// address of variable follows op code
*
*    if (dup1st==0)			// if we didn't dup it, no value remains
*       parSp->ty = TYDONE;		// .. on run stack -- have whole statement.
*
*    return RCOK;
*    ERREX(emiSto)
*}		// emiSto
#endif
//==========================================================================
LOCAL RC FC emiDup()	// emit code to dup run stack top value
{
	ERVARS1

	ERSAVE
	printif( trace," emiDup ");
	switch (parSp->ty)
	{
	case TYSI:
		EE( emit(PSDUP2) )  break;

	case TYSTR: 					// >> need to duplicate string storage?
		// YES 7-92: need to cupIncRef stack top after dup: add a new dup op. ***
		/*lint -e616 case falls in*/
	case TYFL:
		EE( emit(PSDUP4) )  break;

	default:
		rc = perNx( MH_S0081);
		goto er;   	// "Bug in cuparse.cpp:emiDup switch"
	} /*lint -e616 */
	return RCOK;
	ERREX(emiDup)
}		// emiDup
//==========================================================================
LOCAL RC FC emiPop()		// emit additional code to discard any value left on run stack.

// use to do fixup if expression only found where statement expected: must pop resultant value
{
	ERVARS1

	ERSAVE
	printif( trace," emiPop ");
	switch (parSp->ty)
	{
	case TYSI:
		EE( emit(PSPOP2) )  goto doesit;

	case TYINT:
	case TYSTR:  						// >> need to free string storage?
	case TYFL:
		EE( emit(PSPOP4) )
doesit:
		if (parSp->did==0)   			// if no store nor side effect
			pWarnlc( MH_S0082);   		// "Preceding code has no effect"
		break;

	default:
		rc = perNx( MH_S0083);
		goto er;   	// "Bug in cuparse.cpp:emiPop switch"
	} /*lint -e616 */
	parSp->ty = TYDONE; 		// now have complete statement
	return RCOK;
	ERREX(emiPop)
}		// emiPop

//==========================================================================
RC FC emit( PSOP op)			// emit a pseudo-code if non-0

// keeps code terminated (without pointing past terminator)
// maintains current stack frame .psp2

// to emit place holder PSNOPs, use cnvPrevSf -- or add parSp->nNops++ here

// returns non-0 if out of code output space
{
	if (op)				// 0 code means no emit, just terminate
		*psp++ = op;			// emit caller's code
	*psp = PSEND;			// terminate, don't point past: insurance
	parSp->psp2 = psp;  		// update current parse stack frame end-of-pseudoCode pointer
	if (psp >= pspMax)			// if buffer overfull (no data lost yet here)
		return emiBufFull();		// enlarge it (future) or issue message
	return RCOK;
}			// emit
//==========================================================================
RC FC emit2( SI i)		// emit a 2-byte quantity

// keeps code terminated (without pointing past terminator).  maintains current stack frame .psp2.
{
	*psp++ = i;   			// emit caller's datum
	*psp = PSEND;			// terminate, don't point past: insurance
	parSp->psp2 = psp;   		// update current parse stack frame end-of-pseudoCode pointer
	if (psp >= pspMax)			// if buffer overfull (no data lost yet here)
		return emiBufFull();		// enlarge it (future) or issue message
	return RCOK;
}			// emit2
//==========================================================================
LOCAL RC FC emit4( void **p)	// emit 4-byte quantity POINTED TO by p: FLOAT or INT

// keeps code terminated (without pointing past terminator)
// maintains current stack frame .psp2
{
	if (p)				// NULL means no emit, just terminate
	{	*(void **)psp = *p;
		IncP( DMPP( psp), sizeof( float));
	}
	*psp = PSEND;			// terminate, don't point past: insurance
	parSp->psp2 = psp;   		// update current parse stack frame end-of-pseudoCode pointer
	if (psp >= pspMax)			// if buffer overfull (no data lost yet here)
		return emiBufFull();		// enlarge it (future) or issue message
	return RCOK;
}			// emit4
//==========================================================================
LOCAL RC FC emitPtr(void** p)	// emit 4-byte quantity POINTED TO by p: float or pointer

// keeps code terminated (without pointing past terminator)
// maintains current stack frame .psp2
{
	if (p)				// NULL means no emit, just terminate
	{
		*(void**)psp = *p;
		IncP(DMPP(psp), sizeof(void*));
	}
	*psp = PSEND;			// terminate, don't point past: insurance
	parSp->psp2 = psp;   		// update current parse stack frame end-of-pseudoCode pointer
	if (psp >= pspMax)			// if buffer overfull (no data lost yet here)
		return emiBufFull();		// enlarge it (future) or issue message
	return RCOK;
}			// emit4
//==========================================================================
#if defined( USE_PSPKONN)	// emit string in pseudo-code stream.  pad to whole # words.
LOCAL RC FC emitStr(
	const char *s,	// string
	int sLen)		// length of string
{
	RC rc{ RCOK };

	int l = sLen + 1;  				// length, bytes, incl \0
	while ((PSOP *)((char *)psp +l +1) >= pspMax)   	// if won't fit
	{
			CSE_E( emiBufFull() )				// realloc (future) or errmsg & return bad.
			// else loop to see if now enuf space
	}
	memmove( psp, s, l);			// move string with its null
	// memmove as could overlap from emiKon() from konstize().
	IncP( DMPP( psp), l);
	while (l % sizeof(PSOP))  		// pad out to whole # words
	{
		*(char *)psp = '\\';
		IncP( DMPP( psp), 1);
		l++;
	}
	rc = emit(0); 				// terminate, update parSp->psp2, etc
	return rc;
}		// emitStr
#endif
//==========================================================================
LOCAL RC FC emiBufFull( void)		// pseudo-code buffer full handler

// might later reallocate and return RCOK (must adjust psp and parStk .psp1's and .psp2's).
{
// abort compilation.  Having only one copy of msg saves space, right? NO LONGER IF IDENTICAL -- 2-92.

	psFull++;				// set full flag, tested in statement().  flag important because ERREX may restore psp.

	return perl( MH_S0084);	// "Compiled code buffer full, ceasing compilation"

}		// emiBufFull


//===========================================================================
LOCAL SI FC tokeTest( SI tokTyPar)		// get next token, return nz if it is token of specified type.
{
	return (toke()		// get token, cuparse.cpp. sets cutok.cpp:cuToktx.
	== tokTyPar);   	// true if is specified token type
}			// tokeTest
//===========================================================================
SI FC tokeNot( SI tokTyPar)			// get next token, return nz if it is NOT token of specified type.
{
	return (toke()		// get token, cuparse.cpp. sets cutok.cpp:cuToktx.
	!= tokTyPar);   	// false if is specified token type
}			// tokeNot
//===========================================================================
SI FC tokeIf( SI tokTyPar)			// return nz if next token is token of specified type, else unget token.
{
	if (toke()			// get token, cuparse.cpp. sets cutok.cpp:cuToktx.
	== tokTyPar)   	// if is requested token type
		return 1;
	unToke();			// wrong type.  unget token (cuparse.cpp)
	return 0;
}		// tokeIf
//===========================================================================
LOCAL SI FC tokeIf2( SI tokTy1, SI tokTy2)  	// return nz if next token is token of either specified type, else unget.
{
	if ( toke()			// get token, cuparse.cpp. sets cutok.cpp:tokTy.
	== tokTy1   	// if is requested token type
	|| tokTy==tokTy2 )	// or other requested type
		return 1;		// return TRUE
	unToke();			// wrong type.  unget token (cuparse.cpp)
	return 0;
}		// tokeIf2
//===========================================================================
LOCAL SI CDEC tokeIfn( SI tokTy1, ...)	// return nz if next token is of any type in 0-terminated list, else unget token.
{
	va_list list;   SI thisArg;

	toke();			// get token, cuparse.cpp. sets cutok.cpp:tokTy.
	thisArg = tokTy1;			// 1st arg
	va_start( list, tokTy1);		// set up to fetch more args
	while (thisArg > 0)			// 0 (or -1) arg terminates list
	{
		if (tokTy==thisArg)    		// if token type matches
			return 1;			// say found
		thisArg = va_arg( list, SI);	// next arg (stdarg.h)
	}
	unToke();				// unget token (cuparse.cpp)
	return 0;				// not found
}		// tokeIfn

/*==================== VARIABLES for toke() - unToke() ====================*/
LOCAL SI reToke = 0;		// nz to ret same token again (unToke()/toke()). Also used in perI, curLine, .
LOCAL SI symtabIsIt = 0;   	// non-0 if symbol table has been initialized by adding local symbols
LOCAL SI symtabIsSorted = 0;	// non-0 if symbol table sorted (slows adds, speeds searches)

//==========================================================================
LOCAL BOO /*no FC, no */ stbkDel( SI _tokTy, STBK *&_stbk)	// syClear callback for all symtab entries
{
	switch (_tokTy)
	{
	case CUTUF:
		return funcDel( _tokTy, (UFST **)&_stbk);	// function above in cuparse.cpp cleans user functions

		//case CUTUV:	add code for user variable (unimplemented 10-93)

	default:     		// others are reserved words, blocks are not in dm, nothing to do
		return TRUE;		// say free this entry
	}
}		// stbkDel

//==========================================================================
LOCAL void FC cuptokeClean([[maybe_unused]] CLEANCASE cs)	// cleanup for toke - untoke. 10-93.
{
	reToke = 0;				// say do not re-get token. 12-93 observed that this can be set after no-run error.

// clear empty symbol table. sytb.cpp fcn.
	syClear( &symtab, -1, 		// -1: all entries
	stbkDel );	// callback just above cleans & deletes block, returns TRUE.
	symtab.isSorted = 0;		// sym tab maintained unsorted for fast adds till all reserved words added

	symtabIsIt = 0;  symtabIsSorted = 0;

}		// cuptokeClean
//==========================================================================
SI FC toke()	/* local token-getter -- cutok.cpp:cuTok + unary/binary resolution +
		                         symbol table lookup + other refinements */

/* sets: tokTy: token type: CUTxxx defines, cutok.h.
      prec: precedence, comments at start file and/or cuparsei.h.
  lastPrec: prior 'prec' value.
       opp: pointer to entry in opTbl at start of this file.
      ttTx: copy of opp->tx for errMsgs incl external use (cul.cpp)
      stbk: for previously declared identifiers only:
              block ptr from symbol table:
              ptr to VRBST, UFST, SVST, etc per type.
    isWord: non-0 if a word even if not CUTID:
    	       permits specific errMsgs for misused reserved words and
    	       use of reserved words as class and member names (probe)
	and variables cuTok() sets: cuTok.cpp:cuToktx[], cuIntval, cuFlval. */
{

	/* first time, initialize and sort symbol table. */

	if (symtabIsIt==0)
	{
		addLocalSyms(); 		// add symbols whose initialization data is in this file.  Insurance -- caller should do.
		// error return path (for out of memory) lacking here.
		symtabIsIt++;
	}
	if (symtabIsSorted==0)	// sort done here only to be sure adds by all various callers are complete 1st, 10-90.
	{
		// sort table for fast searches (symtab is initialized as an unsorted table for fast at-end adds)
		sySort( &symtab);			/* sytb.cpp */
		symtabIsSorted++;
	}

	/* on token-ungotten flag, return same token again */

	if (reToke)				// set by unToke()
	{
		// with this mechanism here, don't need cuUntok in cutok.cpp.
		reToke = 0;			// clear flag
		printif( trace," re ");		// debug aid print
	}

	/* else get new next token */

	else
	{
		/* get token (symbol) at cutok.cpp level */
		tokTy = 			// and set our file-global type
			cuTok();  	// sets cuToktx[], cuIntval, cuFlval.  Note: only call, 11-91.

		/* set isWord for all words, reserved or not, for smart error msgs. */
		if (tokTy != CUTID)
			isWord = 0;
		else				// is an identifier
		{
			isWord = 1;
			/* classify already-declared identifiers using symbol table.
				      3rd arg 0 causes token to match symtab entries different
				      in capitalization only if flagged "case insensitive" */
			syLu( &symtab, cuToktx, 0, &tokTy, &stbk /*, &nearId */);	// search sym tab (sytb.cpp)
			// if found, tokTy & stbk set from symbol table, else tokTy unchanged.
			// all uses of stbk (per tokTy returned) know whether id is lower case, 3-92.
		}
	}

	/* save prior prec for testing context (internally and also by caller) */
	lastPrec = prec;			// (note unToke() reverses this)

	/* resolve unary/binary operator cases such as "-" by context */

	if (lastPrec >= PROP)   	/* if a value precedes this token */
	{
		switch (tokTy)
		{
			/* change ops that can be either unary or binary to binary
			  (cuTok returns unary) */
		case CUTPLS:
			tokTy = CUTBPLS;
			break;
		case CUTMIN:
			tokTy = CUTBMIN;
			break;
		default:
			;
		}
	}
	else if (lastPrec < PROP)   	/* if no value precedes token */
	{
		switch (tokTy)
		{
			/* change ops that can be either unary or binary to unary.
			  Happens after unToke & caller prec change.
			  case: "fcn(...": ( has PROP but is not a value;
			        expr entry 0's prec but may toke/untoke first. 10-90. */
		case CUTBPLS:
			tokTy = CUTPLS;
			break;
		case CUTBMIN:
			tokTy = CUTMIN;
			break;
		default:
			;
		}
	}

	/* set global operator table entry pointer, fetch 'prec'. */

	opp = opTbl + tokTy;	// for access to .prec (here), and .cpps, .v1 etc by caller
	prec = opp->prec;   	/* get token's "prec" from operator table:
    				   indicates operator precedence (operANDS have hi values >= PROP) and classifies. */
	ttTx = opp->tx;		/* static descriptive text, eg "identifier" not specific name;
    				   not overwritten by later toke()s (you save ptr).
				   Compare cuToktx.  externally used (opp is private). 1-91. */

	/* debug aid print */
	printif( trace, "\n toke=%2d prec=%2d '%s' ",
			 tokTy, prec, (char *)cuToktx );
	return tokTy;
}			// toke
//==========================================================================
void FC unToke()		// "unget" last gotten token

/* sets 'nextPrec' to current prec, then restores prior 'prec'.
   Does not change tokTy, opp, stbk, isWord, cuToktx, etc.
   Tokty MUST remain set for next toke()!. */
{
	printif( trace," un ");
	nextPrec = prec;	// prec of token NEXT toke() will get: so callers can test ungotten terminator on return from expr()
	prec = lastPrec;	// restore prior "prec", for copying back to lastPrec in next toke(), w poss intervening caller modif
	reToke = 1;		// flag tells toke() to return same stuff again
}		// unToke
//==========================================================================
LOCAL RC FC addLocalSyms()		// init symbol table: local entries

/* For search speed, one symbol table contains reserved words and predefined and user-defined functions and variables.
   This function puts locally predefined items in table per initialized data --
     the init data is organized in more maintainable forms than the alphabetical everything-together runtime symbol table.
	External modules wishing to add initial symbols should call cuAddItSyms() (next). */

// returns non-RCOK if out of memory for symbol table
{
	ITSYMS *p;   RC rc;

// add entries in several data-init tables per itSyms table (in cuparse.cpp)

	for (p = itSyms;  p->tokTy != 0;  p++)					// loop over tables of data
		CSE_E( cuAddItSyms( p->tokTy, p->casi, p->tbl, p->entlen, 0) )	// add entries to symtab, just below
		return RCOK;
}		// addLocalSyms
//==========================================================================
RC FC cuAddItSyms( SI tokTyPar, SI casi, STBK* tbl, USI entLen, int op)

// add initial symbols to cuparse symbol table

/* used internally for expression items (system fcns & variables),
   externally for externally-processed words such as verbs. */

/* "tbl" points to array of struct of size entLen:
     1st (or per sytb.cpp) member is char *id (char *id if nearId): the symbol, or NULL to terminate.
     If tokTyPar==-1, 2nd (or per RWST) member is SI tokTy: token type.
     Rest of block can be anything useful to caller.
   Later, when toke() reads symbol, "stbk" is set to point to this block. */

// op: DUPOK bit (sytb.h): no error if id, tokty, casi match existing entry (regardless of block) 1-17-91

// returns non-RCOK if out of memory for symbol table
{
	STBK* q;
	RC rc;

	for (q = tbl;  q->ID() != NULL;  IncP( (void **)&q, entLen))	// loop tbl entries

		CSE_E( syAdd( &symtab,   			/* add entry to symtab, sytb.cpp */
				  tokTyPar==-1 			/* tokTyPar == -1 means varying tokTy */
				  ? ((RWST*)q)->tokTy		/* is in 2nd member of each st block */
				  : tokTyPar,     		/* else (normally) use totkTyPar */
				  casi,				/* case-insensitive flag */
				  q, 				/* sym tab block pointer */
				  op));     			/* option eg DUPOK */
	return RCOK;
}		// cuAddItSyms

//==========================================================================
LOCAL const char* FC before( const char* tx, int aN)		// subtext for error message for expression BEFORE operator

/* if tx is NULL:			return "",
   if aN != 0 (error re fcn arg):	return " as arg <aN> to '<tx>'"
   else 				return " before '<tx>'"		*/
{
	if (aN)
		return asArg( tx, aN);
	if (tx==NULL)
		return "";
	return strtprintf( " before '%s'", tx);
}					// before
//==========================================================================
LOCAL const char* FC after( const char *tx, int aN)	// subtext for error message for expression AFTER operator, similarly
{
	if (aN)
		return asArg( tx, aN);
	if (tx==NULL)
		return "";
	return strtprintf( " after '%s'", tx);
}					// after
//==========================================================================
LOCAL const char* FC asArg(	// errMsg subtext: " as argument[ n][ to 'f']"

	const char* tx,	// fcn name or NULL to omit
	int aN )	// argument number, or -1 to omit
{
	return strtprintf( " as argument%s%s",
		aN < 0  ?  ""  :  strtprintf(" %d", aN),	// -1: unspecified arg#
		tx==NULL  ?  ""  :  strtprintf(" to '%s'", tx)
					 );
}			// asArg
//==========================================================================
LOCAL const char* FC datyTx( USI ty) 	// return text for data type
{
	MH mh;
	switch (ty)
	{
	case TYNONE:
		mh = MH_S0085;
		break;		//  "no value"
	case TYDONE:
		mh = MH_S0086;
		break;		//  "complete statement"
	case TYANY:
		mh = MH_S0087;
		break;		//  "value (any type)"
	case TYNUM:
		mh = MH_S0088;
		break;		//  "numeric value"			// any numeric data type
	case TYLLI:
		mh = MH_S0089;
		break;		//  "limited-long-integer value"
	case TYINT:
	case TYSI:
		mh = MH_S0090;
		break;		//  "integer value"
	case TYFL:
		mh = MH_S0091;
		break;		//  "float value"
	case TYSTR|TYID:
	case TYID:
		mh = MH_S0092;
		break;		//  "name or string value"		// 2-92
	case TYSTR:
		mh = MH_S0093;
		break;		//  "string value"
		//TYSTR|TYSI probably unused now (probes changed to TYID 4-92):
	case TYSTR|TYSI:
		mh = MH_S0094;
		break;		//  "integer or string value"		// >>> say "numeric" if floats accepted
	case TYID|TYSI:
		mh = MH_S0095;
		break;		//  "name or string or integer value"	// >>> say "numeric" if floats accepted
	case TYFLSTR:
		mh = MH_S0096;
		break;		//  "numeric or string value"
	case TYCH:
		mh = MH_S0097;
		break;		//  "choice value"			// 2-92 could add (1st few) choices
	case TYNC:
		mh = MH_S0098;
		break;		//  "numeric or choice value"         	// ditto
		//add cases as they arise... fcnArgs may now produce addl combinations 2-92
	case 0:
		mh = MH_S0099;
		break;		//  "<unexpected 0 data type in 'datyTx' call>"
	default:
		mh = MH_S0100;
		break;		//  "<unrecog data type in 'datyTx' call>"
		//MH_S0101,MH_S0102 avail for additions 10-92.
	}
	return msg( NULL, mh);		// fetch text for handler from disk to tmpStr, lib\messages.cpp. 10-92.
}				    // datyTx
//==========================================================================
const char* FC evfTx(	 		// text for eval freq bits, to insert in message
	USI evf,
	SI adverb )		// part of speech	  example use
                    //    0   adjective	  "%s change"
    				//    1   adverb      "changes %s"
    				//    2   noun  	  "changes at end of %s"
// ignores EVXBEGIVL interval stage bits
// ignores EVFDUMMY bits
{
#if 0 // historical comment
x    // handle EVFDUMMY here if found necessary
#else // 11-25-95 making fcns work (cleanEvf, expr, exOrk changes)
	evf &= ~EVFDUMMY;		// disregard "contains fcn dummy arg, don't konstize" bit, 11-25-95.
#endif

	evf = cleanEvf( evf, 0);	// remove redundant bits, eg DAY with MON.  Cond'l changes to/from EVFMH.  local, above.
	/* if cleanEvf can return more than one bit set (eg monthly & weekly),
	   add bit-combination cases or recode to concatenate words with " and ". 11-90. */

	MH mh = MH_S0103;			// init disk text handle to bug message "(cuparse.cpp:evfTx(): bad cleanEvf value 0x%x)"
	if (adverb==2)				// if noun desired
	{
		if      (evf & EVFSUBHR)  mh = MH_S0104;    		// "each subhour"
		else if (evf & EVFHR)     mh = MH_S0105;    		// "each hour"
		else if (evf & EVFDAY)    mh = MH_S0106;    		// "each day"
		else if (evf & EVFMH)     mh = MH_S0107;    		// "each hour on 1st day of month/run".	Use not expected.
		else if (evf & EVFMON)    mh = MH_S0108;    		// "each month"
		else if (evf & EVFRUN)    mh = MH_S0109;    		// "run (of each phase, autoSize or simulate)"	May be start or end
		else if (evf & EVFFAZ)    mh = MH_S0125;			// "each phase (autoSize or simulate)"	6-95
		else if (evf & EVEOI)     mh = MH_S0110;    		// "input time"
		else if (evf==0)          mh = MH_S0111;    		// "(no change)" 	(Use unexpected)
	}
	else						// adjective (0) or adverb (1) desired
	{
		if      (evf & EVFSUBHR)  mh = MH_S0112;    		// "subhourly"
		else if (evf & EVFHR)     mh = MH_S0113;    		// "hourly"
		else if (evf & EVFDAY)    mh = MH_S0114;    		// "daily"
		else if (evf & EVFMH)     mh = MH_S0115;    		// "monthly-hourly"
		else if (evf & EVFMON)    mh = MH_S0116;    		// "monthly"
		else if (evf & EVFRUN)
			mh = adverb ? MH_S0117   	// "at start of run (of each phase, autoSize or simulate)".  Presumes EVENDIVL bit off.
			            : MH_S0118;  	// "run start time (of each phase, autoSize or simulate)"   ..
		else if (evf & EVFFAZ)
			mh = adverb ? MH_S0126   	// "at start autoSize and simulate phases" 6-95
			            : MH_S0127;  	// "autoSize and simulate phase start time" 6-95
		else if (evf & EVEOI)     mh = adverb ? MH_S0119		// "at input time"
			: MH_S0120;  	// "input time"
		else if (evf==0)          mh = adverb ? MH_S0121		// "never"          (Use unexpected)
			: MH_S0122;  	// "no"	               ..
	}
	return msg( NULL, mh, evf);		// get text from disk, 10-92.
	// evf is for %x in bad-value msg to whose handle mh was init.
}			// evfTx

//==========================================================================
RC CDEC per( MSGORHANDLE ms, ...)		// basic error message: No input line display nor file name/line # display.

// Also see following fcns: perl(),perNx(),perlc(),pWarn(),pWarnlc(),pInfol, etc.

// returns RCBAD.
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 0, 0, 0, ms, ap);	// message, ret RCBAD
}				// per
//==========================================================================
RC CDEC perl( MSGORHANDLE ms, ...)		// Error message with input file name and line #.  No line text display.

// returns RCBAD
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 0, 1, 0, ms, ap);	// message, ret RCBAD
}				// perl
//==========================================================================
RC CDEC perlc( MSGORHANDLE ms, ...)		// issue parse ERROR message with input line text, ^, file name & line #
// returns RCBAD
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 1, 1, 0, ms, ap);	// message, ret RCBAD
}				// perlc
//==========================================================================
RC CDEC pWarn( MSGORHANDLE ms, ...)		// issue plain parse WARNING message WITHOUT input line text, caret, file name, line #
// returns RCOK
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 0, 0, 1, ms, ap);	// message, ret RCOK
}				// pWarn
//==========================================================================
RC CDEC pWarnlc( MSGORHANDLE ms, ...)		// issue parse WARNING message with input line text, ^, file name & line #
// returns RCOK
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 1, 1, 1, ms, ap);	// message, ret RCOK
}				// pWarnlc
//==========================================================================
RC CDEC pInfo( MSGORHANDLE ms, ...)		// issue parse INFO message WITHOUT input line text, caret, file name, line #.
// returns RCOK
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 0, 0, 2, ms, ap);	// message, ret RCOK
}				// pInfo
//==========================================================================
RC CDEC pInfol( MSGORHANDLE ms, ...)	// issue parse INFO message with file name & line #.  no input line display.
// returns RCOK
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 0, 1, 2, ms, ap);	// message, ret RCOK
}				// pInfol

//==========================================================================
RC CDEC pInfolc( MSGORHANDLE ms, ...)	// issue parse INFO message with file name & line #, line, and caret
// returns RCOK
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return perI( 1, 1, 2, ms, ap);	// message, ret RCOK
}				// pInfolc
//==============================================================================
RC CDEC perNx( MSGORHANDLE ms, ...)

// issue parse Error message with input line text, ^, file name & line #, and/or SKIP INPUT TO NEXT STATEMENT.  Uses "inFlist".

// returns RCBAD.
{
	va_list ap;
	va_start( ap, ms);		// point variable arg list after msg arg
	return perNxV( 0, ms, ap);
}				// perNx
//==========================================================================
RC CDEC pWarnNx( MSGORHANDLE ms, ...)

// issue parse Warning message with input line text, ^, file name & line #, and/or SKIP INPUT TO NEXT STATEMENT.  Uses "inFlist".

// returns RCOK
{
	va_list ap;
	va_start( ap, ms);		// point variable arg list after msg arg
	return perNxV( 1, ms, ap);
}				// pWarnNx
//==========================================================================
RC FC perNxV(
	int isWarn,		// 0=error, 1=warning, 2=info
	MSGORHANDLE ms, va_list ap)

// ptr to arg list versn: parse err msg w input text, ^, file name & line #,
// and/or SKIP INPUT TO NEXT STATEMENT.  Uses "inFlist".

// returns RCBAD for errors, RCOK for warning/info
{
	RC rc;

	/* issue message if given */
	if (!ms.mh_IsNull())
		rc = perI( 1, 1, isWarn, ms, ap);	// message, ret RCBAD
	else					// just skipping to ';' after error msg'd elsewhere
		rc = !isWarn ? RCBAD : RCOK;		// no-msg return value

	// skip input to next statement
	if ( reToke				// advance off any token backed up to
	||  prec != PRSEM )		// if already on a ';', stay, but advance off of a verb
	{
		SI parenCount = inFlist ? 1 : 0;	// init paren counter re types -- advance out of arg list
		SI brakCount = 0;		// init bracket counter -- advance past any probe subscripts
		SI tm1 = tokTy, tm2 /*=0*/;
		for ( ; ; )				// scan to ;, eof, verb not in ()'s or []'s
		{
			tm2 = tm1;
			tm1 = tokTy;			// prior two token types
			switch(toke())				// get input token and test it
			{
			case CUTLPR:
				parenCount++;
				break;	// parens enclose fcn arg lists which can contain type words
			case CUTRPR:
				parenCount--;
				break;
			case CUTLB:
				brakCount++;
				break;	// brackets enclose probe subscripts, possibly nested probes
			case CUTRB:
				brakCount--;
				break;
			default:
				;
			}
			switch (prec)					// test precedence of token just gotton
			{
			case PREOF:
				goto breakbreak;			// end file ends scan!

			case PRVRB:
				if (tm1==CUTAT)     break;  	// verb after @ is a probe, not a verb, don't end scan
				if ( tm2==CUTRB
				&& tm1==CUTPER )  break;  	// verb after ]. is member name in a probe
				if (brakCount > 0)  break;  	// verb between [ ]'s is probably a nested probe
				goto breakbreak;				// end scan here

			case PRTY:
				if (parenCount > 0)  break;  	// type word in ()'s is probably next fcn arg, not next stmt
				goto breakbreak;
				//case PRVRBL					// like, type, etc: always take as start new statement
				//case PRSEM					// ;  always take as end of statement
			default:
				if (prec > PRSEM)  break;		// anything hier precedence is continuation same stmt
				goto breakbreak;
			}
		}
breakbreak:
		;
	}
	if (prec < PRSEM)			// stay after ;
		unToke();			// ungobble verb, type, eof. cutok.cpp.

	inFlist = 0;			// insurance: clear "parsing fcn defn arg list" flag

	printif( trace, " perNx exit ");
	return rc;
}			// perNxV
//==========================================================================
LOCAL RC FC perI( int showTx, int showFnLn, int isWarn, MSGORHANDLE ms, va_list ap)		// parse error message inner fcn

// counts errors (not warnings/infos) in rmkerr.cpp error counter.

// returns RCBAD for errors, RCOK for warnings/info.
{
// format args into msg, log/display. ++'s errCount if !isWarn.

	return cuErv( 		// returns RCBAD if error, RCOK if warning/info (a CHANGE 7-92)
		showTx,	// nz to display input line
		showTx,	// nz to put caret ^ under error
		showFnLn,	// nz to show file name and line #
		reToke,	// nz to put caret at prior token
		0, 0,  	// no given file name index / line # 2-91
		isWarn,	// 0 if error (++errCount), 1 if warning (display suppressible), 2 if "Info"
		ms, ap );	// specific msg and args
}	// perI

//===========================================================================
void FC curLine( 	// re last token: return file line text and info

	int retokPar,	// nz for previous token even if have not cuUntok'd
	int* pFileIx,	// NULL or receives "file index" for current input file
	int* pline,		// NULL or receives line # in file
	int* pcol, 		// NULL or receives column # (subscript) in line
	char *s,  		// NULL or receives line text. must be 384+ chars.
	size_t sSize )	// sizeof(*s) for overwrite protection, 9-95

// for caller issuing compile error message showing bad statement, etc.
// Adjusts for ungotten token.  Intended to respond "right" to
// newlines between tokens;  newlines within tokens not expected.
{

// merge local "token has been ungotten" flag into arguments and call cutok.cpp:cuCurLine.

	cuCurLine( reToke | retokPar, pFileIx, pline, pcol, s, sSize );

}		// curLine

// end of cuparse.cpp
