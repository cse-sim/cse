// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// msgtbl.cpp -- CSE message text table: for linking in CSE program if resident text in use

// See messages.cpp for STORY and related code

// This module defines and initializes:
//   msgTbl[]		table associating message handles with messages
//   msgTblCount	number of entries in msgTbl[]
// These variables are publicly visible but are declared and refd ONLY in messages.cpp and msgw.cpp

// !! NOTE: msgTbl[] is MH-order sorted in messages:msgInit(), SO --
//    1.  You can add messages anywhere in the table at any time without
//        concern about order (but msg han values must be unique)
//    2.  Runtime order differs from initialization order.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// sub-includes cndefns.h

#include <MSGHANS.H>	// MHxxx definitions
#include <MESSAGES.H>	// MSGTBL,

//===========================================================================
MSGTBL far msgTbl[] =		//(far rob 1-92 for BC)
{
	{ MHOK,       "" },		// must be "" ? (don't change w/o research)

// --- Control and usage messages ---
// cse.cpp --
	{
		MH_C0001, "C0001: usage: %s <switches> <inputfile> \n"
		"\n"
		"  switches available include:\n"
		"     -ipath;path;...            input & include file drives/directories\n"
		"     -Dname  or  -Dname=value   define preprocessor symbol\n"
		"     -Uname                     undefine preprocessor symbol\n"
		"     -b     batch: don't await keypress at error\n"
		"     -n     no warning messages on screen (only in error file)\n"
#ifdef BINRES//cndefns.h
		"     -r     generate basic binary results file\n"
		"     -h     generate hourly binary results file. Use with -r.\n"
#endif
		"     -p     display member names for use in input file 'probe' expressions\n"
	    "     -q     display all member names, including unprobable\n"
	    "     -c     display input names\n"
	    "     -tnn   set test options to nn\n"
	    "     -x\"s\"  set report test prefix to s\n"
	},
{ MH_C0002, "C0002: Too many non-switch arguments on command line: %s ..." },
{ MH_C0003, "C0003: No input file name given on command line" },
{ MH_C0004, "C0004: Command line error(s) prevent execution" },
// more usage errors, cse.cpp, added 9-26-92
{ MH_C0005, "C1005: Switch letter required after '%c'" },
{ MH_C0006, "C1006: Unrecognized switch '%c%c'" },
{ MH_C0007, "C1007: Unexpected cul() preliminary initialization error" },
{ MH_C0008, "C0008: \"%s\":\n    It is clearer to always place all switches before file name." },	// 2-95 .454
// additions 6-95 .462
{ MH_C0009, "C0009: Can't use switch -%c unless calling program\n"
            "    gives \"hans\" argument for returning memory handles" },
{ MH_C0010, "C0010: Switch -%c not available in DOS version" },
{ MH_C0011, "C0011: \"%s\": \n    input file name cannot contain wild cards '?' and '*'." },
#if 0
x  // timing info, cse.cpp, 6-95, .462.
x  { MH_T0001, "\n\n%s %s %s run(s) done: %s" },
x  { MH_T0002, "\n\nInput file name:  %s" },
x  { MH_T0003, "\nReport file name: %s" },
#endif	// 3-24-10

// cnguts.cpp --
{ MH_C0100, "C0100: *** Interrupted ***" },
{ MH_C0101, "C1101: For GAIN '%s': More than 100 percent of gnPower distributed:\n"	// 100%% --> 100 percent 10-23-96
	"    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n"
	"        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g" },

// --- Internal errors ---
// commented messages are coded in line to avoid possible recursion trouble
// messages.cpp --
// "X0001: msgInit(): duplicate message handle '%d' found in msgTbl[]",
// "X0002: msgCheck(): Message '%1.15s ...' is too long; truncting to max "
//         "allowed length of %d",
// "X0005: No message for message handle '%d'"

// msgerr.cpp --
// "X0010: Cannot open error message file '%s'",

// dmpak.cpp critical errors.  Exact wording may be wrong, check code.
// "X0020: dmalloc(): out of memory on request for %ud bytes",
// "X0021: dmralloc(): out of memory on request for %u bytes (old size=%u)",
// "X0025: dmCkPrange(): DM error in %s at %p -- invalid pointer",
// "X0026: dmcheck1(): DM error in %s at %p -- "
//         "odd MS size (%d)",
// "X0027: dmcheck1(): DM error in %s at %p -- "
//         "MS size (%d) doesn't match copy (%d)",
// "X0028: dmcheck1(): DM error in %s at %p -- "
//         "MS size (%u) is less than data size (%u)",
// "X0029: dmcheck1(): DM error in %s at %p -- "
//         "bad size (%u) at end of block -- expected %u"

// ratpak.cpp --   OBSOLETE -- REVIEW
{ MH_X0040, "X0040: %s(): argument not valid RATBASE" },
{ MH_X0041, "X0041: %s(): Illegal use of static RAT" },
{ MH_X0042, "X0042: %s(): given RAT has not been created" },
{ MH_X0043, "X0043: %s(): bad subscript %d: less than 1" },
{ MH_X0044, "X0044: %s(): subscript %d in use (.gud = 0x%x)" },
{ MH_X0045, "X0045: %s(): SRD not found for rt 0x%x" },
{ MH_X0046, "X0046: %s(): bad or unassigned rat number %d" },
{ MH_X0047, "X0047: %s(): bad RAT entry" },
{ MH_X0048, "X0048: %s(): called for non-%s RATBASE" },

// ancrec.cpp, added 5-14-92
{ MH_X0050, "X0050: record::CopyFrom: unconstructed destination (b is 0)" },
{ MH_X0051, "X0051: record::operator=(): unconstructed destination (b is 0)" },
{ MH_X0052, "X0052: record::operator=(): records not same type" },
{ MH_X0053, "X0053: anc4n: bad or unassigned record anchor number %d" },
{ MH_X0054, "X0054: %s() called for NULL object pointer 'this'" },
{ MH_X0055, "X0055: %s() argument not a valid anchor" },
{ MH_X0056, "X0056: %s(): illegal use of static-storage anchor" },
{ MH_X0057, "X0057: basAnc constructor: non-near anc at %p\n" },

// xmodule.cpp --
	// "X0070: LoadLibrary fail
	// "X0071: FreeLibrary fail
	// "X0072: GetProcAddress fail

// envpak.cpp --
	// "X0100: cknull(): %p is %#lx not %#lx: memory clobber detected"
	// "X0101: cknull(): Tmpstr overwrite"
	// "X0102: integer divide by 0"
	// "X0103: floating point exception: %s"
	// "X0104: matherr exception (%s), possible argument values are "
	//         "arg1=%lg arg2=%lg",

// --- Input language errors 3-92 ---

// pp.cpp 5-14-92 },
	{ MH_P0001, "P0001: ppOpen(): cn ul input file already open" },
	{ MH_P0002, "P0002: Cannot open '%s': already at maximum \n    include nesting depth" },
	{ MH_P0003, "P0003: Cannot open file '%s'" },
	{ MH_P0004, "P0004: Close error, file '%s'" },
	{ MH_P0005, "P0005: ppClI: bad is->ty" },
	{ MH_P0006, "P0006: Overlong preprocessor command truncated" },
	{ MH_P0007, "P0007: Internal error: ppC: read off the end of buffer " },
	{ MH_P0008, "P0008: ppC intrnl err: buf space not avail for / and * " },
	{ MH_P0009, "P0009: '(' expected after macro name '%s'" },
	{ MH_P0010, "P0010: Argument list to macro '%s' far too long: \n    ')' probably missing or misplaced" },
	{ MH_P0011, "P0011: comma or ')' expected" },
	{ MH_P0012, "P0012: Too many arguments to macro '%s'" },
	{ MH_P0013, "P0013: Too few arguments to macro '%s'" },
	{ MH_P0014, "P0014: Overlong argument to macro '%s' truncated" },
	{ MH_P0015, "P0015: newline in quoted text" },
	{ MH_P0016, "P0016: Unexpected end of file in macro reference" },
	{ MH_P0017, "P0017: macArgCi internal error: is->i >= is->j" },
	{ MH_P0018, "P0018: ppmScan(): garbage .inCmt %d" },
	{ MH_P0019, "P0019: Comment within comment" },
	{ MH_P0020, "P0020: Unexpected end of file or macro in comment" },
	{ MH_P0021, "P0021: Too many nested macros and #includes" },
	{ MH_P0022, "P0022: pp.cpp:lisFind: matching buffer line is %d but requested line is %d" },
	{ MH_P0024, "P0024: Replacing illegal null character ('\\0') with space\n    in or after input file line number shown."},	//10-92
	//pp.cpp additions 6-95
	{ MH_P0025, "P0025: '=' expected" },
	{ MH_P0026, "P0026: Unexpected stuff after -U<identifier>" },

	{ MH_P0030, "P0030: Invalid preprocessor command:    word required immediately after '#'" },
	{ MH_P0031, "P0031: Unrecognized preprocessor command '%s'" },
	{ MH_P0032, "P0032: #else not preceded by #if" },
	{ MH_P0033, "P0033: More than one #else for same #if (#if is at line %d of file %s)" },
	{ MH_P0034, "P0034: #endif not preceded by #if" },
	{ MH_P0035, "P0035: Too many nested #if's, skipping to #endif" },
	{ MH_P0036, "P0036: #elif not preceded by #if" },
	{ MH_P0037, "P0037: #elif after #else (for #if at line %d of file %s )" },
	{ MH_P0038, "P0038: '<' or '\"' expected" },
	{ MH_P0039, "P0039: Internal error in ppcDoI: bad 'ppCase'" },
	{ MH_P0040, "P0040: Unexpected stuff after preprocessor command" },
	{ MH_P0041, "P0041: Comma or ')' expected" },
	{ MH_P0042, "P0042: Overlong #define truncated" },
	{ MH_P0043, "P0043: Unbalanced quotes in #define" },
	{ MH_P0044, "P0044: '#endif' corresponding to #if at line %d of file %s not found" },
	{ MH_P0045, "P5045: Closing '%c' not found" },
	{ MH_P0046, "P5046: Redefinition of '%s'" },

// ppcex.cpp  5-14-92 },
	{ MH_P0050, "P0050: ppcex.cpp:cex() Internal error:\n    parse stack level %d not 1 after constant expression" },
	{ MH_P0051, "P0051: Syntax error: probably operator missing before '%s'" },
	{ MH_P0052, "P0052: '%s' expected" },
	{ MH_P0053, "P0053: Unexpected '%s'" },
	{ MH_P0054, "P0054: Unrecognized opTbl .cs %d for token='%s' ppPrec=%d, ppTokety=%d." },
	{ MH_P0055, "P0055: Unrecognized ppTokety %d for token='%s' ppPrec=%d." },
	{ MH_P0056, "P0056: Preceding constant integer value expected before %s" },
	{ MH_P0057, "P0057: Internal error: parse stack underflow" },
	{ MH_P0058, "P0058: Value expected after %s" },
	{ MH_P0059, "P0059: ppcex.cpp:ppUnOp() intrnl error: unrecognized 'opSi' value %d" },
	{ MH_P0060, "P0060: Preceding constant integer value expected before %s" },
	{ MH_P0061, "P0061: ppcex.cpp:ppBiOp() internal error: unrecognized 'opSi' value %d" },
	{ MH_P0062, "P0062: Preprocessor parse stack overflow error: \n    preprocessor constant expression probably too complex" },

// pptok.cpp 5-92
	{ MH_P0070, "P0070: Number too large, truncated" },
	{ MH_P0071, "P0071: Ignoring illegal char 0x%x" },
	{ MH_P0072, "P0072: Identifier expected" },
	{
		MH_P0073, "P0073: Internal warning: possible bug detected in pp.cpp:ppCNdc(): \n"
		"    Character gotten with decomment, ungotten, \n"
		"    then gotten again without decomment. "
	},
// sytb.cpp 5-92
	{ MH_P0080, "P0080: sytb.cpp: bad call to syAdd()" },
	{ MH_P0081, "P0081: sytb.cpp:syAdd(): tokTy 0x%x has disallowed hi bits" },
	{ MH_P0082, "P0082: sytb.cpp:syAdd(): Adding symbol table entry '%s' (%d) \n    that duplicates '%s' (%d)" },
	{ MH_P0083, "P0083: sytb.cpp:syDel(): symbol '%s' not found in table" },
	{ MH_P0084, "P0084: sytb.cpp:syDel(): bad call: token type / casi / nearId is 0x%x not 0x%x" },

// exman.cpp input time expression errors. 5-92
	{ MH_E0090, "E0090: exman.cpp:exPile: Internal error: bad table entry: \n    bad evfOk 0x%x for 16-bit quantity" },
	{ MH_E0091, "E0091: exman.cpp:exPile: Internal error: \n    called with TYCH wanTy 0x%x but non-choice choiDt 0x%x" },
	{ MH_E0092, "E0092: exman.cpp:exPile: Internal error: \n    called with TYNC wanTy 0x%x but 2-byte choiDt 0x%x" },
	{ MH_E0093, "E0093: exman.cpp:exPile: Internal error: \n    4-byte choice value returned by exOrk for 2-byte type" },
	{ MH_E0094, "E0094: exman.cpp:expile: fdTy > 255, change UCH to USI" },
	{ MH_E0095, "E0095: Bad %s: %s: %s" },
	{ MH_E0096, "E0096: exman.cpp:exWalkRecs: os[] overflow" },
	{ MH_E0097, "E0097: exman.cpp:exWalkRecs: \n    Unset data for %s" },
	{ MH_E0098, "E0098: exman.cpp:exReg: \n    Unset data for %s" },
// exman.cpp run time expression errors. 5-92
	{ MH_E0100, "E0100: exman.cpp:exEvEvf: exTab=NULL but exN=%d" },
	{ MH_E0101, "E0101: Unset data or unevaluated expression error(s) have occurred.\n    Terminating run." },
	{ MH_E0102, "E0102: More than %d errors.  Terminating run." },
	{ MH_E0103, "E0103: exman.cpp:exEv: expr %d has NULL ip" },
	{ MH_E0104, "While determining %s: " },			// code removed 10-93: is submessage
	{ MH_E0105, "E0105: Bad value, %s, for %s: %s" },
	{ MH_E0106, "E0106: exman.cpp:pRecRef: record subscript out of range" },
	{ MH_E0107, "E0107: exman.cpp:txVal: unexpected 'ty' 0x%x" },
	{ MH_E0108, "<bad rat #: %d>" },				// 10-92. code removed 10-93: is insert.
	{ MH_E0109, "member at offset %d" },				// 10-92. "
	{ MH_E0110, "'%s' (subscript [%d])" },   			// 10-92. "

// mostly syntax errors -- cuparse.cpp 5-92
	{ MH_S0001, "S0001: Syntax error.  'FUNCTION' or 'VARIABLE' expected" },
	{ MH_S0002, "S0002: %s '%s' cannot be used as function name" },
	{ MH_S0003, "S0003: Expected identifier (name for function)" },
	{ MH_S0004, "S0004: '(' required. \n    If function has no arguments, empty () must nevertheless be present." },
	{ MH_S0005, "S0005: Syntax error: '(' expected" },
	{ MH_S0006, "S0006: Too many arguments" },
	{ MH_S0007, "S0007: Syntax error: expected a type such as 'FLOAT' or 'INTEGER'" },
	{ MH_S0008, "S0008: %s '%s' cannot be used as argument name" },
	{ MH_S0009, "S0009: Expected identifier (name for argument)" },
	{ MH_S0010, "S0010: Syntax error: ',' or ')' expected" },
	{ MH_S0011, "S0011: '=' expected" },
	{ MH_S0012, "S0012: cuparse:expTy: bad return type 0x%x (%s) from expr" },
	{ MH_S0013, "S0013: %s expected%s%s" },
	{ MH_S0014, "S0014: Syntax error: probably operator missing before '%s'" },
	{ MH_S0015, "S0015: '%s' expected" },
	{ MH_S0016, "S0016: Internal error in cuparse.cpp:expr: TYLLI not suppored in cuparse.cpp" },
	{ MH_S0017, "S0017: Unexpected '%s'" },
	{ MH_S0018, "S0018: Internal error in cuparse.cpp:expr: Unrecognized opTbl .cs %d\n    for token='%s' prec=%d, tokTy=%d." },
	{ MH_S0019, "S0019: Internal error in cuparse.cpp:expr: \n    Unrecognized tokTy %d for token='%s' prec=%d." },
	{ MH_S0020, "S0020: %s\n    (OR mispelled word or omitted quotes)" },
	{ MH_S0021, "S0021: :\n    perhaps you intended '%s'." },
	{ MH_S0022, "S0022: Misspelled word or undeclared function or variable name '%s'%s" },
	{ MH_S0023, "S0023: Value expected before ','" },
	{ MH_S0024, "S0024: Internal error in cuparse.cpp:expr: parse stack underflow" },
	{ MH_S0025, "S0025: Value is not available until later in %s\n    than is required%s.\n    You may need to use a .prior in place of a .res" },
	{ MH_S0026, "at most %s variation" },
	{ MH_S0027, "%s variation not" },
	{ MH_S0028, "S0028: Statement expected%s" },
	{ MH_S0029, "S0029: Value expected %s%s" },
	{ MH_S0030, "S0030: Internal error: cuparse.cpp:expr produced %d not 1 parStk frames" },
	{ MH_S0031, "S0031: Preceding %s expected" },
	{ MH_S0032, "S0032: %s expected%s" },
	{ MH_S0033, "S0033: Incompatible expressions before and after ':' -- \n    cannot combine '%s' and '%s'" },
	{ MH_S0034, "S0034: '(' missing after built-in function name '%s'" },
	{ MH_S0035, "S0035: Built-in function '%s' may not be assigned a value" },
	{ MH_S0036, "S0036: Nested assignment should be enclosed in ()'s to avoid ambiguity" /* CAUTION 3 uses */ },
	{ MH_S0037, "S0037: Built-in function '%s' may only be used left of '=' --\n    it can be assigned a value but does not have a value" },
	{ MH_S0038, "S0038: '(' missing after built-in function name '%s'" },
	{ MH_S0039, "S0039: ',' expected"  /* CAUTION multiple uses */ },
	{ MH_S0040, "S0040: Built-in function '%s' may not be assigned a value" },
	{ MH_S0041, "S0041: hourval() requires values for 24 hours" },
	{ MH_S0042, "S0042: hourval() arguments in excess of 24 will be ignored" },
	{ MH_S0043, "S0043: All conditions in '%s' call false and no default given" },
	{ MH_S0044, "S0044: Index out of range and no default: \n    argument 1 to '%s' is %d, not in range %d to %d" },
	{ MH_S0045, "S0045: ',' expected. '%s' arguments must be in pairs." },
	{ MH_S0046, "S0046: Syntax error: expected '%s'" },
	{ MH_S0047, "S0047: Incompatible arguments%s to '%s':\n    can't mix '%s' and '%s'" },
	{ MH_S0048, "S0048: Internal error in cuparse.cpp:fcnArgs:\n    bad fcn table entry for '%s': VC, LOK, ROK all on" },
	{ MH_S0049, "S0049: Too few arguments to built-in function '%s': %d not %d%s" },
	{ MH_S0050, "S0050: Too many arguments to built-in function '%s': %d not %d" },
	{ MH_S0051, "S0051: Internal error in cuparse.cpp:emiFcn: bad function table entry for '%s'" },
	{ MH_S0052, "S0052: System variable '%s' may not be assigned a value" },
	{ MH_S0053, "S0053: System variable '%s' may only be used left of '=' --\n    it can be assigned a value but does not have a value" },
	{ MH_S0054, "S0054: '(' expected: functions '%s' takes no arguments\n    but nevertheless requires empty '()'" },
	{ MH_S0055, "S0055: '(' expected after function name '%s'" },
	{ MH_S0056, "S0056: ')' expected" },
	{ MH_S0057, "S0057: Too %s arguments to '%s': %d not %d" },
	{ MH_S0058, "S0058: Internal error: bug in cuparse.cpp:dumVar switch" },
	{ MH_S0059, "S0059: User variables not implemented" },
	{ MH_S0060, "S0060: Integer value required%s" },
	{ MH_S0061, "S0061: Numeric expression required%s" },
	{ MH_S0062, "S0062: Internal error in cuparse.cpp:cleanEvf: Unexpected 'EVFDUMMY' flag" },
	{ MH_S0063, "S0063: Expected numeric value, found choice value" },
	{ MH_S0064, "S0064: Numeric value required" },
	{ MH_S0065, "S0065: %s\n    (or perhaps invalid mixing of string and numeric values)" },
	{ MH_S0066, "S0066: Internal error in cuparse.cpp:cnvPrevSf: expressions not consecutive" },
	{ MH_S0067, "S0067: Internal error in cuparse.cpp:cnvPrevSf:\n    pspe+.. %p != psp2 %p" },
	{ MH_S0068, "S0068: Parse stack overflow error: expression probably too complex" },
	{ MH_S0069, "S0069: Internal Error in cuparse.cpp:combSf: expressions not consecutive" },
	{ MH_S0070, "S0070: Internal error: bad call to cuparse.cpp:dropSfs( %d, %d)" },
	{ MH_S0071, "S0071: confusion (1) in dropSfs" },
	{ MH_S0072, "S0072: confusion (2) in dropSfs" },
	{ MH_S0073, "S0073: Internal error in cuparse.cpp:movPsLast: expression not last in ps." },
	{ MH_S0074, "S0074: Internal error in cuparse.cpp:movPsLast: expressions not consecutive" },
	{ MH_S0075, "S0075: Internal error in cuparse.cpp:fillJmp: address place holder not found in Jump" },
	{ MH_S0076, "S0076: confusion in dropJmpIf" },
	{ MH_S0077, "S0077: Internal Error in cuparse.cpp:emiKon:\n    no recognized bit in choiDt value 0x%x" },
	{ MH_S0078, "S0078: Bug in cuparse.cpp:emiKon switch" },
	{ MH_S0079, "S0079: Bug in cuparse.cpp:emiLod switch" },
	{ MH_S0080, "S0080: Bug in cuparse.cpp:emiSto switch" },
	{ MH_S0081, "S0081: Bug in cuparse.cpp:emiDup switch" },
	{ MH_S0082, "S0082: Preceding code has no effect" },
	{ MH_S0083, "S0083: Bug in cuparse.cpp:emiPop switch" },
	{ MH_S0084, "S0084: Compiled code buffer full, ceasing compilation" },
	// 10-92..      S0084-S0086:  reserve for short texts that could go on disk in functions: before, after, asArg.
	{ MH_S0085, "no value" },			// codes removed from 0085-0100 10-93 cuz they are inserts.
	{ MH_S0086, "complete statement" },
	{ MH_S0087, "value (any type)" },
	{ MH_S0088, "numeric value" },
	{ MH_S0089, "limited-long-integer value" },
	{ MH_S0090, "integer value" },
	{ MH_S0091, "float value" },
	{ MH_S0092, "name or string value" },
	{ MH_S0093, "string value" },
	{ MH_S0094, "integer or string value" },
	{ MH_S0095, "name or string or integer value" },
	{ MH_S0096, "numeric or string value" },
	{ MH_S0097, "choice value" },
	{ MH_S0098, "numeric or choice value" },
	{ MH_S0099, "<unexpected 0 data type in 'datyTx' call>" },
	{ MH_S0100, "<unrecog data type in 'datyTx' call>" },
	//S0101,S0102 available
	{ MH_S0103, "(cuparse.cpp:evfTx(): bad cleanEvf value 0x%x)" },
	{ MH_S0104, "each subhour" },			// codes removed S0103-S0122: inserted in other messages. 10-93.
	{ MH_S0105, "each hour" },
	{ MH_S0106, "each day" },
	{ MH_S0107, "each hour on 1st day of month/run" },
	{ MH_S0108, "each month" },
#if 0
	x  { MH_S0109, "run" },
#else//11-95 attempting to clarify wording
	{ MH_S0109, "run (of each phase, autoSize or simulate)" },
#endif
	{ MH_S0110, "input time" },
	{ MH_S0111, "(no change)" },
	{ MH_S0112, "subhourly" },
	{ MH_S0113, "hourly" },
	{ MH_S0114, "daily" },
	{ MH_S0115, "monthly-hourly" },
	{ MH_S0116, "monthly" },
#if 0
	x  { MH_S0117, "at run start" },
	x  { MH_S0118, "run start time" },
#else//attempting to clarify wording, 11-95
	{ MH_S0117, "at start of run (of each phase, autoSize or simulate)" },
	{ MH_S0118, "run start time (of each phase, autoSize or simulate)" },
#endif
	{ MH_S0119, "at input time" },
	{ MH_S0120, "input time" },
	{ MH_S0121, "never" },
	{ MH_S0122, "no" },
	// Import() 2-94
	{ MH_S0123, "S0123: %s argument %d must be constant,\n    but given value varies %s" },	// 2 uses re Import().
	{ MH_S0124, "S0124: Import field number %d not in range 1 to %d" },
	//added 6-95: TERMINOLOGY??
	{ MH_S0125, "each phase (autosize or simulate)" },
	{ MH_S0126, "at start autosize and simulate phases" },
	{ MH_S0127, "autosize and simulate phase start time" },
	{ MH_S0128, "S0128: Value varies %s\n    where %s permitted%s." },	// extracted 6-95

// mostly token syntax errors -- cutok.cpp 7-92
	{ MH_S0150, "MH_S0150: Number too big, truncated" },
	{ MH_S0151, "MH_S0151: Ignoring illegal char 0x%x" },
	{ MH_S0152, "MH_S0152: Overlong quoted text truncated: \"%s...\"" },
	{ MH_S0153, "MH_S0153: Overlong token truncated: '%s...'" },
	{ MH_S0154, "MH_S0154: Unexpected end of file in quoted text" },
	{ MH_S0155, "MH_S0155: Newline in quoted text" },

// input language errors -- cul.cpp -- 6-92
	{ MH_S0200, "S0200: cul.cpp:clearRat internal error: bad tables: RATE entry without CULT ptr" },
	{ MH_S0201, "S0201: cul.cpp:rateDuper(): move or delete request without CULT:\n    can't adjust ownTi in owned RAT entries" },
	{
		MH_S0202, "S0202: cul.cpp:ratCultO: internal error: bad tables: \n"
		"    '%s' rat at %p is 'owned' by '%s' rat at %p and also '%s' rat at %p"
	},
	{ MH_S0203, "S0203: cul.cpp:drefRes() internal error: bad %s RAT index %d" },
	{ MH_S0204, "S0204: cul(): NULL RAT entry arg (e)" },
	{ MH_S0205, "S0205: cul(): bad RAT entry arg (e)" },
	{ MH_S0206, "S0206: cul.cpp:cul(): Unexpected culComp return value %d" },
	{ MH_S0207, "S0207: More than %d errors.  Terminating session." },
	{ MH_S0208, "S0208: 'RUN' apparently missing" },
	{ MH_S0209, "S0209: No commands in file" },
	{ MH_S0210, "S0210: cul.cpp:cul() internal error: no statements in file since 'RUN',\n    yet cul() recalled" },
	{ MH_S0211, "S0211: '$EOF' not found at end file" },
	{ MH_S0212, "S0212: Syntax error.  Expecting a word: class name, member name, or verb." },
	{ MH_S0213, "S0213: culDo() Internal error: unrecognized 'cs' %d" },
	{ MH_S0214, "S0214: No run due to error(s) above" },
	{ MH_S0215, "S0215: Context stack overflow: too much nesting in command tables. \n    Increase size of xStk[] in cul.cpp." },
	{ MH_S0216, "S0216: Nothing to end (use $EOF at top level to terminate file)" },
	{ MH_S0217, "S0217: %s name mismatch: '%s' vs '%s'" },
	{ MH_S0218, "S0218: Cannot DELETE '%s's" },
	{ MH_S0219, "S0219: Can't define a '%s' within a DEFTYPE" },
	{ MH_S0220, "S0220: cul.cpp:culRATE Internal error: bad table:\n    RATE entry contains fcn not cult ptr" },
	{ MH_S0221, "S0221: %s type name required" },
	{ MH_S0222, "S0222: %s name required" },
	{ MH_S0223, "S0223: cannot define TYPEs for '%s'" },
	{ MH_S0224, "S0224: cannot ALTER '%s's" },
	{ MH_S0225, "S0225: '%s' not permitted with '%s'"	/* USED TWICE */ },
	{ MH_S0226, "S0226: Can't use LIKE or COPY with USETYPE in same %s" },
	{ MH_S0227, "S0227: duplicate %s name '%s' in " },
	{ MH_S0228, "S0228: duplicate %s%s name '%s'" },
	{ MH_S0229, "S0229: %s type '%s' not found" },
	{
		MH_S0230, "S0230: cul.cpp:clearRat Internal error: Too much nesting in tables: \n"
		"    increase size of context stack xStk[] in cul.cpp."
	},
	{ MH_S0231, "S0231: '=' expected" },
	{ MH_S0232, "S0232: Invalid duplicate '%s =' or 'AUTOSIZE %s' command" },
	{ MH_S0233, "S0233: '%s' cannot be specified here -- previously frozen" },
	{ MH_S0234, "S0234: cul.cpp:culDAT: Bad .ty %d in CULT for '%s' at %p" },
	{ MH_S0235, "S0235: Value '%s' for '%s' not one of %s" },
	{ MH_S0236, "S0236: Statement says %s '%s', but it is \n    in statement group for %s." },
	{ MH_S0237, "S0237: Can't give more than %d values for '%s'" },
	{ MH_S0238, "S0238: Internal error: cul.cpp:datPt(): \n    bad CULT table entry: flags incompatible with ARRAY flag" },
	{ MH_S0239, "S0239: Internal error: cul.cpp:datPt(): \n    bad CULT table entry: ARRAY flag, but 0 size in .p2" },
	{ MH_S0240, "S0240: Internal error: cul.cpp:datPt(): \n    bad CULT table entry: ARRAY size 0x%lx in p2 is too big" },
	{ MH_S0241, "S0241: cul.cpp:datPt(): Unrecognized CULT.ty %d in entry '%s' at %p" },
	{ MH_S0242, "S0242: cul.cpp:datPt(): Bad ty (0x%x) / dtype (0x%x) combination in entry '%s' at %p" },
	{
		MH_S0243, "S0243: Internal error: cul.cpp:datPt(): data size inconsistency: \n"
		"    size of cul type 0x%x is %d, but size of field data type 0x%x is %d"
	},
	{ MH_S0244, "S0244: Ambiguity in tables: '%s' is used in '%s' and also '%s'" },
	{ MH_S0245, "S0245: cul.cpp:finalClear() called with xSp %p not top (%p)" },
	{ MH_S0246, "S0246: cul.cpp:xStkPt(): bad xStk->cs value %d" },
	{ MH_S0247, "S0247: Illegal character 0x%x in name" },
	{ MH_S0248, "S0248: Zero-length name not allowed" },
	{ MH_S0249, "S0249: Expected ';' (or class name, member name, or verb)" },
	{ MH_S0250, "S0250: 'SUM' cannot be used here" },
	{ MH_S0251, "S0251: 'ALL' cannot be used here" },
	{ MH_S0252, "S0252: 'ALL_BUT' cannot be used here" },
	//MH_S0253
	{ MH_S0254, "S0254: Misspelled word?  Expected class name, member name, command, or ';'." },
	{ MH_S0255, "S0255: Expected ';' (or class name, member name, or command)" },
	{ MH_S0256, "S0256: Expected name of class to %s" },
	{ MH_S0257, "S0257: Internal error in cul.cpp:bFind() call: bad cs %d" },
	{ MH_S0258, "S0258: Can't define types for '%s'" },
	{ MH_S0259, "S0259: '%s' is not %s-able (here)"	/* USED TWICE */ },
	{ MH_S0260, "S0260: '%s' cannot be given here" },
	{ MH_S0261, "S0261: Expected name of member to %s" },
	{ MH_S0262, "S0262: Confused at '%s' after error(s) above.  Terminating session." },
	{ MH_S0263, "S0263: No %s given in %s" },
	{ MH_S0264, "S0264: No %s in %s" },
	{ MH_S0265, "S0265: cul.cpp:ratPutTy(): 'e' argument not valid RAT entry ptr" },
	{ MH_S0266, "S0266: cul.cpp:ratPutTy(): RAT entry to be made a TYPE must be last" },
	{ MH_S0267, "S0267: Out of near heap memory (cul:ratTyr)" },
	{ MH_S0268, "S0268: cul.cpp:ratTyR(): argument not valid RATBASE ptr" },
	{ MH_S0269, "S0269: out of near heap memory (cul:ratTyr)" },
	{ MH_S0270, "S0270: cul.cpp:nxRat: Too much nesting in tables:\n    Increase size of context stack xStk[] in cul.cpp" },
	{ MH_S0272, "S0272: *** oerI(); probable non-RAT record arg ***" },
	{ MH_S0273, "S0273: *** objIdTx(); probable non-RAT record ptr ***" },
	{ MH_S0274, "S0274: *** classObjTx(); probable non-RAT record arg ***" },
	{ MH_S0275, "S0275: cul.cpp:culMbrId called with xSp %p not top (%p)" },
	{ MH_S0276, "S0276: [%s or %s: table ambiguity:\n    recode this error message to not use cul.cpp:culMbrId]" },
	{ MH_S0277, "S0277: [%s not found by cul.cpp:culMbrId]" },
	//10-92...
	{ MH_S0278, "S0278: %s '%s' not found" },							// 2 uses in cul.cpp:ratLuDefO.
	{ MH_S0279, "S0279: ambiguous name: \n    %s '%s' exists in %s\n    and also in %s" },
	// 6-95
	{ MH_S0280,  "MH_S0280: '%s' cannot be AUTOSIZEd" },
	{ MH_S0281,  "MH_S0281: Invalid duplicate 'AUTOSIZE %s' or '%s =' command" },

// input statement errors & setup time errors, cncult.cpp  7-92.
	{ MH_S0400, "S0400: lrFrmMat given but lrFrmFrac omitted" },
	{ MH_S0401, "S0401: lrFrmFrac=%g given but lrFrmMat omitted" },
	{ MH_S0402, "S0402: Cannot give both conU and layers" },
	{ MH_S0403, "S0403: Neither conU nor any layers given" },
	{ MH_S0404, "S0404: More than %d sgdist's for same window" },
	{ MH_S0405, "S0405: Neither drCon nor drU given" },
	{ MH_S0406, "S0406: Both drCon and drU given" },
	{ MH_S0408, "S0408: Wall sfTilt = %g: not 60 to 180 degrees" },
	{ MH_S0409, "S0409: sfTilt (other than 180 degrees) may not be specified for a floor" },
	{ MH_S0410, "S0410: Ceiling sfTilt = %g: not 0 to 60 degrees" },
	{ MH_S0411, "S0411: sfAzm may not be given for floors" },
	{ MH_S0412, "S0412: No sfAzm given for tilted surface" },
	{ MH_S0414, "S0414: Cannot use delayed (massive) sfModel without giving sfCon" },
	{ MH_S0417, "S0417: Neither sfCon nor sfU given" },
	{ MH_S0418, "S0418: Both sfCon and sfU given" },
	{ MH_S0420, "S0420: sfExAbs may not be given if sfExCnd is not Ambient or SpecifiedT" },
	{ MH_S0422, "S0422: sfExCnd is SpecifiedT but no sfExT given" },
	{ MH_S0423, "S0423: sfExT not needed when sfExCnd not 'SpecifiedT'" },
	{ MH_S0425, "S0425: sfExCnd is Zone but no sfAdjZn given" },
	{ MH_S0426, "S0426: sfAdjZn not needed when sfExCnd not 'Zone'" },
	{ MH_S0427, "S0427: More than %d terminals for zone '%s'" },
	{ MH_S0428, "S0428: %sFreq may not be given with %sType=%s" },
	{ MH_S0429, "S0429: %sDayBeg may not be given with %sType=%s" },
	{ MH_S0430, "S0430: %sDayEnd may not be given with %sType=%s" },
	{ MH_S0431, "S0431: No %sFreq given: required with %sType=%s" },
	{ MH_S0432, "S0432: %sDayBeg may not be given with %sFreq=%s" },
	{ MH_S0433, "S0433: %sDayEnd may not be given with %sFreq=%s" },
	{ MH_S0434, "S0434: %sFreq=%s not allowed with %sty=%s" },
	{ MH_S0435, "S0435: %sFreq=%s not allowed in Sum-of-%s %s" },
	{ MH_S0436, "S0436: No %sDayBeg given: required with %sFreq=%s" },
	{ MH_S0437, "S0437: Internal error in RI::ri_CkF: bad rpFreq %d" },
	{ MH_S0438, "S0438: Internal error in RI::ri_CkF: bad rpTy %d" },
	{ MH_S0439, "S0439: %sCpl cannot be given unless %sTy is UDT" },
	{ MH_S0440, "S0440: %sTitle cannot be given unless %sTy is UDT" },
	{ MH_S0441, "S0441: Duplicate %s '%s'\n    (already used in ReportFile '%s')" },
	{ MH_S0442, "S0442: Duplicate %s '%s'\n    (already used in ExportFile '%s')" },
	{ MH_S0443, "S0443: infShld = %d: not in range 1 to 5" },
	{ MH_S0444, "S0444: infStories = %d: not in range 1 to 3" },
	{ MH_S0445, "S0445: '%s' is obsolete. Please remove it from your input." },

// input statement setup time errors, cncult2.cpp  7-92
	// 10-93 deleted msg numbers from some subclasues starting with "when"
	{ MH_S0470, "S0470: Daylight saving time should not start and end on same day" },
	{ MH_S0471, "S0471: infShld = %d: not in range 1 to 5" },
	{ MH_S0472, "S0472: infStories = %d: not in range 1 to 3" },
	{ MH_S0473, "S0473: Internal error: bad izNVType %s (0x%x)" },
	{ MH_S0474, "when izNVType is NONE or omitted" },
	{ MH_S0475, "S0475: \n    (no ventilation is modelled with no height difference)" },
	{ MH_S0476, "when izNVType is TWOWAY" },
	{ MH_S0477, "S0477: lrMat not given" },
	{ MH_S0478, "S0478: Ignoring unexpected mt_rNom=%g of framing material '%s'" },
	{ MH_S0479, "S0479: Non-0 lrFrmFrac %g given but lrFrmMat omitted." },
	{ MH_S0480, "S0480: lrFrmMat given, but lrFrmFrac omitted" },
	{ MH_S0481, "S0481: Material thickness %g does not match lrThk %g" },
	{ MH_S0482, "S0482: Framing material thickness %g does not match lrThk %g" },
	{ MH_S0483, "S0483: Framing material thickness %g does not match \n    material thickness %g" },
	{ MH_S0484, "S0484: No thickness given in layer, nor in its material%s" },
	{ MH_S0485, "S0485: Only 1 framed layer allowed per construction" },
	{ MH_S0486, "S0486: More than 1 framed layer in construction" },
	{ MH_S0487, "S0487: Both conU and layers given" },
	{ MH_S0488, "S0488: Neither conU nor layers given" },
	{ MH_S0489, "S0489: No gnEndUse given" },
	{ MH_S0490, "S0490: No gnMeter given (required when gnEndUse is given)" },
	{
		MH_S0491, "S0491: More than 100%% of gnPower distributed:\n"
		"    Total of fractions gnFrZn, gnFrPl, and gnFrRtn exceeds 1.0:\n"
		"        gnFrZn (%g) + gnFrPl (%g) + gnFrRtn (%g) = %g%s"
	},
	{ MH_S0493, "S0493: Required member '%s' has not been set,\n    and apparently no message about it appeared above" },
	{ MH_S0494, "S0494: bad %s index %d in%s %s '%s' [%d]%s" },
	{ MH_S0495, "S0495: %s: Internal error: field # %d: unterminated arg list?" },
	{ MH_S0496, "S0496: Can't give '%s' %s" },
	{ MH_S0497, "S0497: '%s' missing: required %s" },
	{ MH_S0498, "S0498: '%s' missing" },
	{ MH_S0499, "S0499: '%s' must be > 0"  },
	{ MH_S0500, "S0500: refTemp (%g) seems to be over boiling for elevation (%g)" },		// rob 5-95
	// 6-95 additions
	{
		MH_S0501, "S0501: Weather file does not include all run dates:\n"
		"    run is %s to %s,\n"
		"    but '%s' dates are %s to %s.\n"
	},
	{
		MH_S0502, "S0502: Run over a month long cannot end in starting month\n"
		"    when hourly binary results are requested."
	},
	{
		MH_S0503, "S0503: Extension given in binary results file name \"%s\" \n"
		"    will not be used -- extensions \".brs\" and \".bhr\" are always used."
	},
	{
		MH_S0504, "S0504: You have given a binary results file name with\n"
		"        \"BinResFileName = %s\",\n"
		"    but no binary results file will be written as you have not given\n"
		"    any of the following commands:\n"
		"        BinResFile = YES;  BinResFileHourly = YES;\n"
		"    nor either of the following command line switches:\n"
		"        -r   -h\n"
	},
	{
		MH_S0505, "S0505: You have given a binary results file name with\n"
		"        \"BinResFileName = %s\",\n"
		"    but no binary results file will be written as you have also specified\n"
		"    memory-only binary results with the -m DLL command line switch."
	},
	{
		MH_S0506, "S0506: gtSMSC (%g) > gtSMSO (%g):\n"
		"    Shades-Closed Solar heat gain coeff Multiplier\n"
		"      must be <= shades-Open value."
	},
	{
		MH_S0507, "S0507: No gnEndUse given when gnDlFrPow given.\n"
		"    gnEndUse=\"Lit\" is usual with gnDlFrPow."
	},
	{
		MH_S0508, "S0508: gnEndUse other than \"Lit\" given when gnDlFrPow given.\n"
		"    gnEndUse=\"Lit\" is usual with gnDlFrPow."
	},

// input statement setup time errors... cncult3.cpp  7-92
	{ MH_S0511, "S0511: Area of %s (%g) is greater than \n    area of %s's surface%s (%g)" },
	{ MH_S0512, "S0512: Area of %s (%g) is greater than \n"
		"    remaining area of %s's surface%s \n"
		"    (%g after subtraction of previous door areas)"
	},
	{ MH_S0513, "S0513: Can't use delayed (massive) sfModel=%s without giving sfCon" }, 	// cncult.cpp, cncult3.cpp 1-95
	{ MH_S0514, "S0514: Delayed (massive) sfModel=%s specified\n"
		"    but surface's construction, '%s', has no layers"
	},
	//unused 1-10-95
	//{ MH_S0515, "S0515: Time constant %g too short for delayed (massive) sfModel.\n"
	//            "    Using sfModel=Quick.\n"
	//            "    (Time constant = constr heat cap (%g) / sfInH (%g) \n"
	//            "    must be >= %g to use 'sfModel=Delayed')" },
	{ MH_S0516, "S0516: wnSCSC (%g) > wnSCSO (%g):\n    shades-closed shading coeff must be <= shades-open shading coeff." },
	{ MH_S0517, "S0517: More than %d SGDISTS for same window" },
	{ MH_S0518, "S0518: %ssurface '%s' not in zone '%s'. \n    Can't target solar gain to surface not in window's zone." },
	{ MH_S0519, "S0519: Target surface '%s' is not delayed model.\n    Solar gain being directed to zone '%s' air." },
	{ MH_S0520, "S0520: Internal error in cncult.cpp:topSg: Delayed model target surface\n    '%s' has no .msi assigned." },
	{ MH_S0521, "S0521: sfTilt is %g.  Shaded window must be in vertical surface." },
	{ MH_S0522, "S0522: Window '%s' is already shaded by shade '%s'. \n    Only 1 SHADE per window allowed." },
	{ MH_S0524, "S0524: Internal error in cncult.cpp:topSf2(): Bad exterior condition %d" },
	{ MH_S0525, "S0525: Internal error in cncult.cpp:topSf2(): Bad surface model %d" },
	{ MH_S0526, "S0526: Bad sfExCnd %d (ms_Make())" },
	{ MH_S0527, "S0527: ms_Make(): model setup error (above)" },
	{ MH_S0528, "S0528: cncult:seriesU(): unexpected unset h or u" },
	{ MH_S0529, "S0529: cncult3.cpp:cnuCompAdd: bad zone index %d" },
	//cncult3 additions 6-95 .462
	{ MH_S0530, "S0530: Window in interior wall not allowed" },
	{ MH_S0531, "S0531: No U-value given: neither wnU nor glazeType '%s' gtU given" },
	{
		MH_S0532, "S0532: wnSMSC (%g) > wnSMSO (%g):\n"
		"    shades-closed SHGC multiplier must be <= \n"
		"    shades-open SHGC multiplier."
	},
	{
		MH_S0533, "S0533: Time constant %g probably too short for delayed (mass) sfModel.\n"
		"    Use of delayed sfModel is NOT recommened when time constant is < %g.\n"
		"    Results may be strange. \n"
		"    (Time constant = constr heat cap (%g) / sfInH (%g) )."
	},
	{
		MH_S0534, "S0534: Time constant %g probably too short for delayed (mass) sfModel.\n"
		"    Use of sfModel=Delayed_Hour \n"
		"        is NOT recommened when time constant is < %g.\n"
		"    Results may be strange. \n"
		"    (Time constant = constr heat cap (%g) / sfInH (%g) )."
	},

// input statement setup time errors... cncult4.cpp  10-92
	{ MH_S0540, "S0540: repCpl %d is not between 78 and 132" },
	{ MH_S0541, "S0541: repLpp %d is less than 50" },
	{ MH_S0542, "S0542: repTopM %d is not between 0 and 12" },
	{ MH_S0543, "S0543: repBotM %d is not between 0 and 12" },
	{ MH_S0544, "S0544: File %s exists" },
	{ MH_S0545, "S0545: %sCol associated with %s whose %sType is not UDT" },
	{ MH_S0546, "S0546: End-of-interval varying value is reported more often than it is set:\n"
		"    %sFreq of %s '%s' is '%s', but colVal for colHead '%s'\n"
		"    varies (is given a value) only at end of %s."
	},
	{ MH_S0547, "S0547: Bad data type (colVal.dt) %d" },
	{ MH_S0548, "S0548: %sCond may not be given with %sType=%s" },
	{ MH_S0549, "S0549: No %sMeter given: required with %sType=%s" },
	{ MH_S0550, "S0550: No %sAh given: required with %sType=%s" },
	{ MH_S0551, "S0551: No %sZone given: required with %sType=%s" },
	{ MH_S0552, "S0552: %sZone may not be given with %sType=%s" },
	{ MH_S0553, "S0553: %sMeter may not be given with %sType=%s" },
	{ MH_S0554, "S0554: %sAh may not be given with %sType=%s" },
	{ MH_S0555, "S0555: cncult:topRp: Internal error: Bad %sType %d" },
	{ MH_S0556, "S0556: No exExportfile given" },
	{ MH_S0557, "S0557: No rpReportfile given" },
	{ MH_S0558, "S0558: rpZone=='%s' cannot be used with %sType '%s'" },
	{ MH_S0559, "S0559: rpMeter=='%s' cannot be used with %sType '%s'" },
	{ MH_S0560, "S0560: rpAh=='%s' cannot be used with %sType '%s'" },
	{ MH_S0561, "S0561: %sTitle may only be given when %sType=UDT" },
	{ MH_S0562, "S0562: %sport has %sCols but %sType is not UDT" },
	{ MH_S0563, "S0563: no %sCols given for user-defined %s" },
	{
		MH_S0564, "S0564: %s width %d greater than line width %d.\n"
		"    Override this message if desired with %sCpl = %d"
	},
	{ MH_S0565, "S0565: cncult:topRp: unexpected %sTy %d" },
	{
		MH_S0566, "S0566: Can't intermix use of hdCase, hdDow, and hdMon\n"
		"    with hdDateTrue, hdDateObs, and hdOnMonday for same holiday"
	},
	{ MH_S0567, "S0567: No hdDateTrue given" },
	{ MH_S0568, "S0568: hdDateTrue not a valid day of year" },
	{ MH_S0569, "S0569: hdDateObs not a valid day of year" },
	{ MH_S0570, "S0570: Can't give both hdDateObs and hdOnMonday" },
	{ MH_S0571, "S0571: If any of hdCase, hdDow, hdMon are given, all three must be given" },
	{ MH_S0572, "S0572: hdDow not a valid month" },
	{ MH_S0573, "S0573: Either hdDateTrue or hdCase+hdDow+hdMon must be given" },
//imports 2-94
	{ MH_S0574, "S0574: No IMPORTFILE \"%s\" found for IMPORT(%s,...)" },
	{ MH_S0575, "S0575: Unused IMPORTFILE: no IMPORT()s from it found." },
	{ MH_S0576, "S0576: imHeader=NO but IMPORT()s by field name (not number) used" },
	{ MH_S0577, "S0577: Unexpected call to IMPF::operator=" },
	{ MH_S0578, "S0578: Unexpected call to IFFNM::operator=" },

// input statement setup time errors... cncult5.cpp  10-93
//   message numbers omitted in msgs inserted in others.
	{
		MH_S0600, "when terminal has no local heat\n"
		"    (when neither tuTLh nor tuQMnLh given)"
	},
	{ MH_S0601, "S0601: tuQMxLh = %g will be ineffective \n    because tuhcCaptRat is less: %g" },
	{
		MH_S0602, "S0602: 'tuQDsLh' is obsolete and should be removed from your input file.\n"
		"    (Note: if heatPlant overloads, available power will be apportioned\n"
		"    amoung HW coils in proportion to tuhcCaptRat/ahhcCaptRat values.)\n"
	},
	{
		MH_S0603, "when local heat is not thermostat controlled\n"
		"    (when tuTLh is not given)"
	},
	{
		MH_S0604, "when terminal has thermostat-controlled local heat\n"
		"    (when tuTLh is given)"
	},
	{ MH_S0605, "when tuTH not given" },
	{ MH_S0606, "when tuTC not given" },
	{
		MH_S0607, "when terminal has no air heat nor cool\n"
		"    (when none of tuTH, tuTC, or tuVfMn is given)"
	},
	{
		MH_S0608, "when terminal has air heat or cool\n"
		"    (when any of tuTH, tuTC, or tuVfMn are given)"
	},
	{ MH_S0609, "when zone plenumRet is 0" },
	{
		MH_S0610, "when terminal has no capabilities: \n"
		"    no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given"
	},
	{ MH_S0611, "when tfanType = NONE" },
	{ MH_S0612, "when tfanType not NONE" },
	{
		MH_S0613, "S0613: Ignoring terminal with no capabilities: \n"
		"    no setpoint (tuTLh, tuTH, tuTC) nor minimum (tuQMnLh, tuVfMn) given"
	},
	{ MH_S0614, "S0614: Air handler has no terminals!" },
	{
		MH_S0615, "S0615: oaVfDsMn > sfanVfDs:\n"
		"    min outside air (%g) greater than supply fan capacity (%g)"
	},
	{
		MH_S0616, "S0616: oaVfDsMn required: default (%g) of .15 cfm per sf of served\n"
		"    area (%g) would be greater than supply fan capacity (%g)"
	},
	{ MH_S0617, "when no economizer (oaEcoTy NONE or omitted)" },
	{ MH_S0618, "S0618: oaOaLeak + oaRaLeak = %g: leaking %d% of mixed air!" },
	{
		MH_S0619, "S0619: Control Terminal for air handler %s:\n"
		"    Must give heating setpoint tuTH and/or cooling setpoint tuTC"
	},
	{
		MH_S0620, "when air handler has no econimizer nor coil\n"
		"    (did you forget to specify your coil?)"
	},
	{ MH_S0621, "unless air handler has no economizer nor coil" },
	{ MH_S0622, "when ahTsSp is ZN or ZN2 and more than one terminal served" },
	{ MH_S0623, "when ahTsSp is RA" },
	{ MH_S0624, "S0624: ahFanCycles=YES not allowed when ahTsSp is ZN2" },
	{ MH_S0625, "S0625: More than one terminal not allowed on air handler when fan cycles" },
	{ MH_S0626, "S0626: No control terminal: required when fan cycles." },
	{
		MH_S0627, "S0627: When fan cycles,\n"
		"    \"constant volume\" outside air works differently than you may expect:\n"
		"    outside air will be supplied only when fan is on per zone thermostat.\n"
		"   To disable outside air use the command \"oaVfDsMn = 0;\"."
	},
	{
		MH_S0628, "S0628: Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
		"    but terminal '%s' max cooling air flow (tuVfMxC) is %g.\n"
		"    Usually, these should be equal when fan cycles.\n"
		"    The more limiting value will rule."
	},
	{
		MH_S0629, "S0629: Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
		"    but terminal '%s' max heating air flow (tuVfMxH) is %g.\n"
		"    Usually, these should be equal when fan cycles.\n"
		"    The more limiting value will rule."
	},
	{
		MH_S0630, "S0630: Control terminal '%s':\n"
		"    tuVfMn=%g: must be zero or omitted when fan cycles."
	},
	{ MH_S0631, "when cchCM is NONE" },
	{
		MH_S0632, "S0632: Crankcase heater specifed, but neither heat coil nor cool coil \n"
		"    is a type for which crankcase heater is appropriate"
	},
	{ MH_S0633, "S0633: cchPMx (%g) must be > cchPMn (%g)" },
	{ MH_S0634, "S0634: cchTMx (%g) must be <= cchTMn (%g)" },
	{ MH_S0635, "when cchCM is not PTC nor PTC_CLO" },
	{ MH_S0636, "S0636: cchTOff (%g) must be > cchTOn (%g)" },
	{ MH_S0637, "when cchCM is not TSTAT" },
	{ MH_S0638, "S0638: Internal error: Bad cchCM %d" },
	{ MH_S0639, "S0639: Internal error: cncult5.cpp:ckCoil: bad coil app 0x%x" },
	{ MH_S0640, "S0640: '%s' given, but no '%s' to charge to it given" },
	{ MH_S0641, "when ahccType is NONE or omitted" },
	{ MH_S0642, "S0642: Bad coil type '%s' for %s" },
	{ MH_S0643, "S0643: Sensible capacity '%s' larger than Total capacity '%s'" },
	{ MH_S0644, "when ahccType is CHW" },
	{ MH_S0645, "when ahccType is CHW" },
	{ MH_S0646, "when ahccType is not CHW" },
	{
		MH_S0647, "S0647: %scoiling coil rating air flow (%g) differs \n"
		"    from supply fan design flow (%g) by more than 25%%"
	},	// 20 --> 25%  .485 5-97
/*
	{
		MH_S0???, "S0???: Supply fan design air flow per ton of rated cooling \n"
		"    capcaity (%g) is outside of the expected range (280 - 450) \n"
		"    %scooling coil rated air flow per ton is (%g)\n"
	},
*/
	{ MH_S0648, "when ahccType is not DX" },
	{ MH_S0649, "when ahccType is not DX nor CHW" },
	{ MH_S0650, "S0650: Bad coil type '%s' for %s" },
	{ MH_S0651, "S0651: Heating coil rated capacity 'captRat' %g is not > 0" },
	{ MH_S0652, "S0652: Cannot give both %s and %s" },
	{ MH_S0653, "S0653: %s, %s must be constant 1.0 or omitted" },
	{ MH_S0654, "S0654: %s, %s must be 1.0 or omitted" },
	{ MH_S0655, "S0655: %s must be >= 1.0" },
	{
		MH_S0656, "S0656: %s,\n"
		"    Either %s or %s must be given"
	},
	{ MH_S0657, "S0657: %s must be <= 1.0" },
	{ MH_S0658, "S0658: %s must be > 0." },
	{ MH_S0659, "when ahhcType is NONE or omitted" },
	{ MH_S0660, "when ahhcType is not AHP" },
	{ MH_S0661, "when ahhcType is AHP" },
	{ MH_S0662, "S0662: ahpCap17 (%g) must be < ahhcCaptRat (%g)" },
	{ MH_S0663, "S0663: ahpDfrFMx (%g) must be > ahpDfrFMn (%g)" },
	{ MH_S0664, "S0664: ahpTOff (%g) must be <= ahpTOn (%g)" },
	{ MH_S0665, "S0665: ahpCd (%g) must be < 1" },
	{ MH_S0666, "S0666: ahpTFrMn (%g) must be < ahpTFrMx (%g)" },
	{ MH_S0667, "S0667: ahpTFrMn (%g) must be < ahpTFrPk (%g)" },
	{ MH_S0668, "S0668: ahpTFrPk (%g) must be < ahpTFrMx (%g)" },
	{ MH_S0669, "S0669: ahpTFrMn (%g) must be < 35." },
	{ MH_S0670, "S0670: ahpTFrMx (%g) must be > 35." },
	{
		MH_S0672, "S0672: ahpCap35 (%g) cannot be > proportional value (%g) between \n"
		"    ahpCap17 (%g) and ahhcCaptRat (%g) -- \n"
		"    that is, frost/defrost effects cannot increase capacity."
	},
	{
		MH_S0673, "S0673: cdm (%g) not in range 0 <= cdm < 1\n"
		"    (cdm is ahpCd modified internally to remove crankcase heater effects)"
	},
	{
		MH_S0674, "S0674: cdm (%g) not <= ahpCd (%g): does this mean bad input?\n"
		"    (cdm is ahpCd modified internally to remove crankcase heater effects)"
	},
	{ MH_S0675, "when ahhcType is AHP" },
	{ MH_S0676, "S0676: Internal error: cncult5.cpp:ckFan: bad fan app 0x%x" },
	{ MH_S0677, "S0677: [internal error: no supply fan]" },
	{ MH_S0678, "when rfanType is NONE or omitted" },
	{ MH_S0679, "when no exhaust fan (xfanVfDs 0 or omitted)" },
	{ MH_S0680, "when tfanType is NONE or omitted" },
	{ MH_S0681, "S0681: bad fan type '%s' for %s" },
	{ MH_S0682, "S0682: '%s' must be given" },
	{ MH_S0683, "S0683: '%s' cannot be less than 1.0" },
	{ MH_S0684, "S0684: '%s' cannot be less than 0" },
	{ MH_S0685, "S0685: Cannot give both '%s' and '%s'" },
	{
		MH_S0686, "S0686: '%s' is less than air-moving output power \n"
		"      ( = '%s' times '%s' times units conversion factor)"
	},
	{
		MH_S0687, "S0687: Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
		"    for %s = %g.\n"
		"    Coefficients will be normalized and execution will continue." /* USED 3 PLACES! */
	},
	{
		MH_S0688, "S0688: Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
		"    for %s = %g"  /* USED 3 PLACES! */
	},
	{
		MH_S0689, "S0689: Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
		"    for %s=%g and %s=%g.\n"
		"    Coefficients will be normalized and execution will continue."
	},
	{
		MH_S0690, "S0690: Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
		"    for %s=%g and %s=%g\n"
	},
	{ MH_S0691, "\n    when terminal has thermostat-controlled air heat (when tuTH is given)" },
	{ MH_S0692, "\n    when terminal has thermostat-controlled air cooling (when tuTC is given)" },

// input staetement setup time errors... cncult6.cpp  10-93
	{ MH_S0700, "S0700: heatPlant has no BOILERs" },
	{ MH_S0701, "S0701: HeatPlant has no loads: no HW coils, no HPLOOP HX's." },
	{ MH_S0702, "S0702: %s: boiler '%s' is not in this heatPlant" },
	{ MH_S0703, "S0703: %s: BOILER '%s' cannot be used twice in same stage" },
	{ MH_S0704, "S0704: Internal error: %s not terminated with 0" },
	{
		MH_S0705, "S0705: stage %s is not more powerful than hpStage%d, \n"
		"    and thus will never be used"
	},
	{ MH_S0706, "S0706: No non-empty hpStages specified" },
	{ MH_S0707, "S0707: Not used in any hpStage of heatPlant" },
	{ MH_S0708, "S0708: blrEirR must be >= 1.0" },
	{ MH_S0709, "S0709: Can't give both blrEirR and blrEffR" },
	{ MH_S0710, "S0710: '%s' given, but no '%s' to charge to it given" },
	{ MH_S0711, "S0711: coolPlant has no CHILLERs" },
	{ MH_S0712, "S0712: CoolPlant has no loads: no CHW coils." },
	{ MH_S0713, "S0713: %s: chiller '%s' is not in this coolPlant" },
	{ MH_S0714, "S0714: %s: CHILLER '%s' cannot be used twice in same stage" },
	{ MH_S0715, "S0715: Internal error: %s not terminated with 0" },
	{ MH_S0716, "S0716: Internal error: bad # chillers (%d) found in stage %s" },
	{ MH_S0717, "S0717: Internal error: \n    nonNegative total capacity %g of chillers in stage %s" },
	{
		MH_S0718, "S0718: stage %s is not more powerful (under design conditions) than \n"
		"    cpStage%d, and thus may never be used"
	},
	{ MH_S0719, "S0719: No non-empty cpStages specified" },
	{ MH_S0720, "S0720: Not used in any cpStage of coolPlant" },
	{
		MH_S0721, "S0721: Total design flow of connected coils, %g lb/hr,\n"
		"    is greater than most powerful stage primary pumping capacity \n"
		"    (with overrun), %g lb/hr."
	},
	{ MH_S0722, "S0722: chMinFsldPlr (%g) must be <= chMinUnldPlr (%g)" },
	{ MH_S0723, "S0723: Primary pump heat (%g) exceeds chCapDs (%g)" },
	{ MH_S0724, "S0724: Primary pump heat (%g) exceeds 1/4 of chCapDs (%g)." },
	{ MH_S0725, "S0725: Condenser pump heat (%g) exceeds 1/4 of chCapDs (%g)." },
	{ MH_S0726, "when chEirDs is given" },
	{ MH_S0727, "S0727: '%s' given, but no '%s' to charge to it given" },
	{ MH_S0728, "S0728: TowerPlant has no loads: no coolPlants, no hpLoop HX's." },
	{ MH_S0729, "S0729: Total pump gpm of connected loads (%g) must be > 0" },
	{ MH_S0730, "when ctTy is not TWOSPEED" },
	{
		MH_S0731, "S0731: ctVfOd (%g) is same as ctVfDs.\n"
		"    These values must differ %s."
	},
	{
		MH_S0732, "S0732: ctVfOd and ctVfDs are both defaulted to %g.\n"
		"    These values must differ %s."
	},
	{
		MH_S0733, "S0733: ctGpmOd (%g) is same as ctGpmDs.\n"
		"    These values must differ %s."
	},
	{
		MH_S0734, "S0734: ctGpmOd and ctGpmDs are both defaulted to %g.\n"
		"    These values must differ %s."
	},
	{ MH_S0735, "S0735: ctK must be less than 1.0" },
	{
		MH_S0736, "S0736: Inconsistent low and hi speed fan curve polynomials:\n"
		"    ctFcLo(ctLoSpd)=%g, but ctFcHi(ctLoSpd)=%g."
	},
	{ MH_S0737, "when ctType is not ONESPEED" },
	{ MH_S0738, "when ctType is not TWOSPEED" },
	{ MH_S0739, "when ctType is not VARIABLE" },
	{
		MH_S0740, "S0740: maOd/mwOd must not equal maDs/mwDs.\n"
		"    maOd=%g  mwOd=%g  maDs=%g  mwDs=%g\n"
		"    maOd/mwOd = maDs/mwDs = %g"
	},
	{ MH_S0741, "S0741: ctK (%g) did not come out between 0 and 1 exclusive" },
	{
		MH_S0742, "S0742: %s outdoor wetbulb temperature (ctTWbODs=%g) must be\n"
		"    less than %s leaving water temperature (ctTwoDs=%g)"
	},
	{ MH_S0743, "S0743: Internal error in setupNtuaADs: haiDs (%g) not < hswoDs (%g)" },
	{
		MH_S0744, "S0744: %s conditions produce impossible air heating:\n"
		"    enthalpy of leaving air (%g) not less than enthalpy of\n"
		"    saturated air (%g) at leaving water temp (ctTWbODs=%g).\n"
		"    Try more air flow (ctVfDs=%g)."
	},
	{ MH_S0745, "S0745: Internal error: ntuADs (%g) not > 0" },
	{
		MH_S0746, "S0746: %s outdoor wetbulb temperature (ctTWbOOd=%g) must be\n"
		"    less than %s leaving water temperature (ctTwoOd=%g)"
	},
	{ MH_S0747, "S0747: Internal error in setupNtuaAOd: haiOd (%g) not < hswoOd (%g)" },
	{ MH_S0748, "S0748: Internal error: ntuAOd (%g) not > 0" },
	{ MH_S0749, "S0749: %s must be >= 1.0" },

// cuprobe.cpp -- probe()
	{ MH_U0001, "U0001: Expected word for class name after '@'" },
	{
		MH_U0002, "U0002: Internal error: Ambiguous class name '%s':\n"
		"    there are TWO %s rats with that .what.  Change one of them."
	},
	{ MH_U0003, "U0003: Unrecognized class name '%s'" },
	{ MH_U0004, "U0004: Expected ']' after object %s" },
	{ MH_U0005, "U0005: Expected '[' after @ and class name" },
	{ MH_U0006, "U0006: Expected '.' after ']'" },
	{
		MH_U0007, "U0007: Internal error: %s member '%s'\n"
		"    has data type (dt) %d in input rat but %d in run rat.\n"
		"    It cannot be probed until tables are made consistent.\n"
	},
	{ MH_U0008, "U0008: %s member '%s' has %s data type (dt) %d" },
	// cuprobe.cpp:findMember
	{ MH_U0010, "U0010: Expected a word for object member name, found '%s'" },
	{ MH_U0011, "U0011: %s member '%s' not found" },
	{ MH_U0012, "U0012: %s member '%s%s' not found: \n    matched \"%s\" but could not match \"%s\"." },
	{
		MH_U0013, "U0013: Internal error: inconsistent %s member naming: \n"
		"    input member name %s vs run member name %s. \n"
		"    member will be un-probe-able until tables corrected or \n"
		"      match algorithm (cuprobe.cpp:findMember()) enhanced."
	},
	{ MH_U0014, "U0014: Expected '%c' next in %s member specification,\n    found '%s'" },
	{ MH_U0015, "U0015: Expected word next in %s member specification,\n    found '%s'" },
	{ MH_U0016, "U0016: Expected number (subscript) next in %s \n    member specification, found '%s'" },
	{ MH_U0017, "U0017: Internal error: unexpected next character '%c'\n    in %s fir table member name" },
	//cuprobe.cpp:tryImInProbe
	{ MH_U0020, "U0020: %s name '%s' is ambiguous: 2 or more records found.\n    Change to unique names." },
	{
		MH_U0021, "U0021: %s '%s' has not been defined yet.\n"
		"    A constant value is required %s a forward reference cannot be used.\n"
		"    Try reordering your input."
	},
	{ MH_U0021a,"for '%s' --\n        " },					// insert for preceding and next -- keep together
	{
		MH_U0022, "U0022: %s '%s' member %s has not been set yet.\n"
		"    A constant value is required %s a forward reference cannot be used.\n"
		"    Try reordering your input."
	},
	{ MH_U0023, "U0023: Internal error: %s '%s' member '%s' \n    contains reference to bad expression # (0x%x)" },
	{
		MH_U0024, "U0024: Internal error: %s '%s' member '%s', \n"
		"    containing expression (#%d):\n"
		"    member type (ty), %d and expression type, %d, do not match."
	},
	{
		MH_U0025, "\nU0026: Internal error: Ambiguous class name '%s':\n"
		"   there are TWO %s rats with that .what. Change one of them.\n"
	},		// 10-92

// --- Value conversion messages ---
// tdpak.cpp --
	{ MH_V0001, "V0001: undecipherable day-month string" },
	{ MH_V0002, "V0002: illegal day of month" },
	{ MH_V0003, "V0003: month name must be 3 letter abbreviation or full name" },
// cvpak.cpp --
	{ MH_V0010, "V0010: invalid number" },
	{ MH_V0011, "V0011: value must be between -32767 and 32767" },
	{ MH_V0012, "V0012: value must be between -2147483639 and 2147483639" },
	{ MH_V0013, "V0013: value must be between 10e-38 and 10e38" },
	{ MH_V0014, "V0014: value must be < 32767" },
	{ MH_V0015, "V0015: value must be < 65535" },
	{ MH_V0016, "V0016: too many digits.  Use E format for very large or very small values" },
	{ MH_V0021, "V0021: value must be less than 0" },
	{ MH_V0022, "V0022: value must be 0 or negative" },
	{ MH_V0023, "V0023: value must be 0 or greater" },
	{ MH_V0024, "V0024: value must be greater than 0" },
	{ MH_V0025, "V0025: value must be non-0" },
	{ MH_V0026, "V0026: value must be negative" },
	{ MH_V0027, "V0027: value must be 1 or greater" },
	{ MH_V0028, "V0028: value must be greater than 1" },
	{ MH_V0031, "V0031: value must be in the range 0 <= v <= 1" },
	{ MH_V0032, "V0032: value must be in the range 0 < v <= 1" },
	{ MH_V0033, "V0033: value must in the range 0 <= v <= 100" },
	{ MH_V0034, "V0034: value must be in range 1 (Jan 1) to 365 (Dec 31)" },
	{ MH_V0035, "V0035: name must be 1 to 63 characters" },
	{ MH_V0036, "V0036: cvpak:getChoiTxI(): choice %d out of range for dt 0x%x" },
	{ MH_V0037, "V0037: cvpak:getChoiTxI(): given data type 0x%x not a choice type" },
	{ MH_V0038, "V0038: cvpak:cvS2Choi(): given data type 0x%x not a choice type" },
	{ MH_V0039, "V0039: '%s' is not one of" },

// --- Run time / calculation errors ---
// psychro1.cpp psychro2.cpp
	{ MH_R0001, "R0001: %s(): psychrometric value (%g) out of range" },
	{ MH_R0002, "R0002: %s(): convergence failure" },
	{ MH_R0003, "R0003: %s(): temperature apparently over boiling for elevation" },

// Weather files and Simulator Test Driver Files (wfpak.cpp)
	{ MH_R0100, "R0100: %s file '%s': open failed" },
	{ MH_R0101, "R0101: %s file '%s': bad format or header" },
	{ MH_R0103, "R0103: Attempt to read data for day not in weather file '%s'" },
	{ MH_R0104, "R0104: Weather file read error in file '%s'" },
	{ MH_R1101, "R1101: Corrupted header in ET1 weather file \"%s\"." },
	{ MH_R1102, "R1102: Missing or invalid number at offset %d in %s %s" },
	{ MH_R1103, "R1103: Bad decoding table for %s in wfpak.cpp" },
	{ MH_R1104, "R1104: Unrecognized characters after number at offset %d in %s %s" },
	//wfpak additions 6-95
	{ MH_R0107, "R0107: Design Days not supported in BSGS (aka TMP or PC) weather files." },
	{
		MH_R0108, "R0108: Program error in use of function wfPackedhrRead\n\n"
		"    Invalid Month (%d) in \"jDay\" argument: not in range 1 to 12."
	},
	{ MH_R0109, "R0109: Weather file \"%s\" contains no design days." },
	{
		MH_R0110, "R0110: Weather file contains no design day for month %d\n\n"
		"    (Weather file \"%s\" contains design day data only for months %d to %d)."
	},
	{ MH_R0112, "R0112: Unreadable Date." },
	{ MH_R0113, "R0113: Incorrect date (%d Julian not %d Julian)." },
	{ MH_R0114, "R0114: Unreadable hour." },
	{ MH_R0115, "R0115: Incorrect hour (%d not %d)." },
	{ MH_R0116, "R0116: Hourly data for hour %d is unreadable." },
	{ MH_R0117, "R0117: Hourly zone data for hour %d is unreadable." },

// mspak.cpp --
	{ MH_R0120, "R0120: Mass '%s': no layers" },
	{ MH_R0121, "R0121: Mass '%s': mass matrix too large or singular, cannot invert" },
	{ MH_R0122, "R0122: no layers in mass" },
	{ MH_R0123, "R0123: mass layer with 0 thickness encountered" },
	{ MH_R0124, "R0124: mass layer with 0 conductivity encountered" },
	{
		MH_R0125, "R0125: number of mass layers exceeds program limit.  Reduce "
		"thickness and/or number of layers in construction"
	},

// cgresult.cpp. some are runtime internal errors, classification is getting too tedious 9-92.
	{ MH_R0150, "R0150: cgresult:vrRxports: unexpected rpFreq %d" },
	{ MH_R0151, "R0151: cgresult.cpp:vpRxports: unexpected rpType %d" },
	{ MH_R0152, "R0152: %sCond for %s '%s' is unset or not yet evaluated" },

// cgsolar.cpp 9-92
	{ MH_R0160, "R0160: Internal error in cgsolar.cpp:makHrSgt:\n    Surface '%s' sgdist[%d]: sgd_FSC (%g) != sgd_FSO (%g)." },
	{ MH_R0161, "R0161: Window '%s': wnSCSC (%g) > wnSCSO (%g):\n    shades-closed shading coeff must be <= shades-open shading coeff." },
	{ MH_R0162, "R0162: %g percent of %s solar gain for window '%s'\n    of zone '%s' distributed: more than 100 percent.  Check SGDISTs." /* 2 uses */ },
	// in above, "%%" --> "percent" 2 places after % problems in other messages, 10-23-96.
	//cgsolar.cpp additions 6-95
	{
		MH_R0163, "R0163: Window '%s': wnSMSC (%g) > wnSMSO (%g):\n"
		"    SHGC Multiplier for Shades Closed must be <= same for Shades Open"
	},
	{
		MH_R0164, "R0164: cgsolar.cpp:SgThruWin::doit(): zone \"%s\", window \"%s\":\n"
		"    undistributed gain fraction is %g (should be 0)."
	},
	{
		MH_R0165, "R0165: cgsolar.cpp:SgThruWin::toZoneCav:\n"
		"    control zone (\"%s\") differs from target zone (\"%s\")."
	},
	{ MH_R0166, "R0166: cgsolar.cpp:sgrAdd(): called for SGDTT %d" },

// cueval.cpp: runtime errors in expression evaluator --  7-92
	{ MH_R0201, "R0201: cuEvalTop: %d words left on eval stack" },
	{ MH_R0202, "R0202: cuEvalR internal error: \n    value %d bytes: neither 2 nor 4" },
	// { MH_R0203, "R0203: " },	// out of service
	{ MH_R0204, "R0204: cuEvalI internal error: \n    eval stack overflow at ps ip=%p" },
	{ MH_R0205, "R0205: cuEvalI internal error: \n    eval stack underflow at ps ip=%p" },
	{ MH_R0206, "R0206: cuEvalI internal error: \n    arg access thru NULL frame ptr at ip=%p" },
	{ MH_R0207, "R0207: Choice (%d) value used where number needed" },
	{ MH_R0208, "R0208: Unexpected UNSET value where number neeeded" },
	{ MH_R0209, "R0209: Unexpected reference to expression #%d" },
	{ MH_R0210, "R0210: Unexpected mystery nan" },
	{ MH_R0211, "R0211: Day of month %d is out of range 1 to %d" },
	{ MH_R0212, "R0212: Division by 0" },
	{ MH_R0213, "R0213: Division by 0." },
	{ MH_R0214, "R0214: SQRT of a negative number" },
	{ MH_R0215, "R0215: In choose() or similar fcn, \n    dispatch index %d is not in range %d to %d \n    and no default given." },
	{ MH_R0216, "R0216: In select() or similar function, \n    all conditions false and no default given." },
	{ MH_R0217, "R0217: %s subscript %d out of range %d to %d" },
	{ MH_R0218, "R0218: %s name '%s' is ambiguous: 2 or more records found.\n    Change to unique names." },
	{ MH_R0219, "R0219: %s record '%s' not found." },
	{ MH_R0220, "R0220: cuEvalI internal error: bad expression number 0x%x" },
	{ MH_R0221, "R0221: cuEvalI internal error:\n    Bad pseudo opcode 0x%x at psip=%p" },
	{ MH_R0222, "R0222: Internal Error: mistimed probe: %s varies at end of %s\n    but access occurred %s" },
	{ MH_R0223, "R0223: at BEGINNING of %s" },
	{ MH_R0224, "R0224: neither at beginning nor end of an interval????" },
	{ MH_R0225, "R0225: Mistimed probe: %s\n    varies at end of %s but accessed at end of %s.\n    Possibly you combined it in an expression with a faster-varying datum." },
	{ MH_R0226, "R0226: Internal error: Unset data for %s" },
	{ MH_R0227, "R0227: Internal error: bad expression number %d found in %s" },
	{ MH_R0228, "R0228: %s has not been evaluated yet." },			// used twice in cueval.cpp
	{ MH_R0229, "R0229: Bad IVLCH value %d" },
	{ MH_R0230, "R0230: %s has not been evaluated yet." },			// 10-92
	{ MH_R0231, "R0231: Unexpected being-autosized value where number needed" },	// 6-95
	{ MH_R0232, "R0232: %s probed while being autosized" },			// 6-95

// mostly run errors, virtual reports
//vrpak.cpp 9-92
	{ MH_R1200, "R1200: re virtual reports, file '%s', function '%s':\n%s" },
	{ MH_R1201, "R1201: Ignoring redundant or late call" },
	{ MH_R1202, "R1202: Too little buffer space and can't write: deadlock" },
	{ MH_R1203, "R1203: Virtual reports not initialized" },
	{ MH_R1204, "R1204: Virtual reports spool file not open" },
	{ MH_R1205, "R1205: Out of range virtual report handle %d" },
	{ MH_R1206, "R1206: Virtual report %d not open to receive text" },
	{ MH_R1210, "R1210: Unspool called but nothing has been spooled" },
	{ MH_R1211, "R1211: After initial setup, 'spl.uN' is 0" },
	{ MH_R1212, "R1212: At end pass, uns[%d] hStat is %d not 0" },
	{ MH_R1213, "R1213: At end of pass, all uns[].did's are 0" },
	{ MH_R1214, "R1214: Unspool did not use all its arguments" },
	{ MH_R1215, "R1215: At end of Unspool, uns[%d] fStat is %d not -2" },
	{ MH_R1216, "R1216: At end of Unspool, uns[%d] hStat is %d not 2" },
	{ MH_R1217, "R1217: Out of range vr handle %d for output file %s in vrUnspool call" },
	{ MH_R1218, "R1218: Unformatted virtual report, handle %d, name '%s' \nin formatted output file '%s'.  Will continue." },
	{ MH_R1219, "R1229: Unexpected empty unspool buffer" },
	{ MH_R1220, "R1220: Invalid type %d in spool file at offset %ld" },
	{ MH_R1221, "R1221: text runs off end spool file" },
	{ MH_R1222, "R1222: text too long for spool file buffer" },
	{
		MH_R1223, "R1223: Debugging Information: Number of simultaneous report/export output \n"
		"  files limited to %d by # available DOS file handles.  Expect bugs."
	},
//vrpak3.cpp: no texts.

//cse.cpp (1000+), cnguts.cpp (1100+): near start of file

// runtime errors: cnztu.cpp 10-92
	{ MH_R1250, "R1250: airHandler - Terminals convergence failure: %d iterations" },
	{ MH_R1251, "R1251: More than %d errors.  Terminating run." },	// also cnguts.cpp, rob 5-97
	{
		MH_R1252, "R1252: heatPlant %s is scheduled OFF, \n"
		"    but terminal %s's local heat is NOT scheduled off: \n"
		"        tuQMnLh = %g   tuQMxLh = %g"
	},
	{
		MH_R1253, "R1253: Local heat that requires air flow is on without flow\n"
		"    for terminal '%s' of zone '%s'.\n"
		"    Probable internal error, \n"
		"    but perhaps you can compensate by scheduling local heat off."
	},
	{ MH_R1254, "R1254: Internal error: 'b' is 0, cannot determine float temp for zone '%s'" },
	{ MH_R1255, "R1255: Internal error: ztuMode is increasing tFuzz to %g (mode %d)" },
	{ MH_R1256, "R1256: Inconsistency in ztuMode: qsHvac (%g) != q2 (%g)" },
	{ MH_R1257, "R1257: Equal setpoints (%g) with equal priorities (%d) in zone '%s' -- \n""    %s and %s" },
	{ MH_R1258, "R1258: terminal '%s'" },				// two uses
	{ MH_R1259, "R1259: Cooling setpoint temp (%g) is less than heating setpoint (%g)\n""    for terminal '%s' of zone '%s'" },
	{ MH_R1260, "R1260: Bad Top.humMeth %d in cnztu.cpp:ZNR::ztuAbs" },
// runtime errors: cnah 10-92
	{ MH_R1270, "R1270: airHandler '%s': ahTsMn (%g) > ahTsMx (%g)" },
	{ MH_R1271, "R1271: airHandler '%s': ahFanCycles=YES not allowed when ahTsSp is ZN2" },
	{ MH_R1272, "R1272: airHandler '%s': more than one terminal\n""    not allowed when fan cycles" },
	{ MH_R1273, "R1273: airHandler '%s': fan cycles, but no control terminal:\n""    ahCtu = ... apparently missing" },
	{
		MH_R1274, "R1274: Control terminal '%s' of airHandler '%s':\n"
		"    terminal minumum flow (tuVfMn=%g) must be 0 when fan cycles"
	},
	{ MH_R1275, "R1275: airHandler '%s': frFanOn (%g) > 1.0" },
#if 0	// unused 11-1-92, changed here 1-7-94
	x  {
		MH_R1276, "R1276: airHandler '%s': convergence failure.\n"
		x              "   tr=%g  wr=%g  cr=%g  frFanOn=%g\n"
		x              "   po=%g  ts=%g  ws=%g  fanF=%g  ahMode=%d\n"
		x              "   trNx=%g  wrNx=%g  crNx=%g"
	},
#else
	// MH_R1276 free
#endif
	{ MH_R1277, "R1277: airHandler '%s': bad oaLimT 0x%lx" },
	{ MH_R1278, "R1278: airHandler '%s': bad oaLimE 0x%lx" },
	{ MH_R1279, "R1279: Internal error: Airhandler '%s': unrecognized ts sp control method 0x%lx" },
	{ MH_R1280, "R1280: airHandler '%s': ahTsSp is RA but no ahTsRaMn has been given" },
	{ MH_R1281, "R1281: airHandler '%s': ahTsSp is RA but no ahTsRaMx has been given" },
	{ MH_R1282, "R1282: airHandler '%s': ahTsSp is RA but no ahTsMn has been given" },
	{ MH_R1283, "R1283: airHandler '%s': ahTsSp is RA but no ahTsMx has been given" },
	{ MH_R1284, "R1284: ahTsSp for airHandler '%s' is ZN or ZN2 but no ahCtu has been given" },
	{ MH_R1285, "R1285: Unexpected use 0x%x for terminal [%d] for control zone [%d] for ah [%d]" },
	{ MH_R1286, "R1286: airHandler '%s': ahTsSp is '%s'\n""    but control terminal '%s' has no %s" },
	{ MH_R1287, "R1287: airHandler '%s': ahTsSp is '%s'\n""    but no control zone with terminal with %s found" },
	{ MH_R1288, "R1288: airHandler %s: antRatTs: internal error: not float mode as expected" },
	{ MH_R1289, "R1289: airHandler %s: Internal error in antRatTs: \n""    terminal %s is not terminal of zone's active zhx" },
	{ MH_R1290, "R1290: airHandler %s: Internal error in antRatTs: unexpected tu->useAr 0x%x" },
	{ MH_R1291, "R1291: Terminal '%s': tuVfMn is %g, not 0, when ahFanCyles = YES" },
	{ MH_R1292, "R1292: AH::setFrFanOn: airHandler '%s': unexpected 'uUseAr' 0x%x" },
	{ MH_R1293, "R1293: AH::setFrFanOn: airHandler '%s': \n""    cMxnx is 0 but crNx is non-0: %g" },
	{
		MH_R1294, "R1294: airHandler '%s' not yet converged. Top.iter=%d\n"
		"   tr=%g  wr=%g  cr=%g  frFanOn=%g\n"
		"   po=%g  ts=%g  ws=%g  fanF=%g  ahMode=%d\n"
		"   trNx=%g  wrNx=%g  crNx=%g\n"
		"   DISREGARD THIS DEBUGGING MESSAGE UNLESS FOLLOWED BY\n"
		"       \"Air handler - Terminals convergence failure\" error message."
	},
	{ MH_R1295, "R1295: airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)\n    but no ahTsMn has been given." },	// 5-95
	{ MH_R1296, "R1296: airHandler '%s': ahTsSp is ZN (with ahFanCycles=YES)\n    but no ahTsMx has been given." },	// 5-95
// mostly runtime errors: cncoil.cpp 10-92
	{ MH_R1310, "R1310: AirHandler %s's heat coil is scheduled on, \n""    but heatPlant %s is scheduled off." },
	{ MH_R1311, "R1311: AirHandler %s's cool coil is scheduled on, \n""    but coolPlant %s is scheduled off." },
	// (1312-1317 are cncoil.cpp setup time errors)
	{ MH_R1312, "R1312: DX coil rated entering conditions are supersaturated." },
	{
		MH_R1313, "R1313: DX coil capacity specifications yield supersaturated air at coil exit:\n"
		"    full-load exit enthalpy (%g) greater than saturated air enthalpy\n"
		"    (%g) at full-load exit temp (%g) at rated conditions."
	},
	{
		MH_R1314, "R1314: Program Error: DX coil effective temp (te) at rated conditions\n"
		"    seems to be greater than entering air temp"
	},
	{
		MH_R1315, "R1315: DX coil effective temperature (intersection of coil process line with\n"
		"    saturation line on psychro chart) at rated conditions is less than 0F.\n"
		"    Possibly your ahccCaptRat is too large relative to ahccCapsRat."
	},
	{
		MH_R1316, "R1316: airHandler '%s': \n"
		"    DX coil setup effective point search convergence failure\n"
		"        errTe=%g"
	},
	{ MH_R1317, "R1317: Program Error: DX coil effectiveness %g not in range 0 to 1" },
	// cncoil.cpp runtime errors...
	{
		MH_R1320, "R1320: airHandler '%s': Air entering DX cooling coil is supersaturated:\n"
		"    ten = %g  wen = %g.  Coil model won't work."
	},
	{ MH_R1321, "R1321: airHandler '%s': DX coil effectiveness %g not in range 0 to 1" },
	{
		MH_R1322, "R1322: airHandler '%s' DX coil inconsistency:\n"
		"    he is %g but h(te=%g,we=%g) is %g.  wena=%g. cs1=%d"
	},
	{
		MH_R1323, "R1323: airHandler '%s':\n"
		"    DX coil full-load exit state convergence failure.\n"
		"      entry conditions: ten=%g  wen=%g  plrVf=%g\n"
		"      unfinished results: te=%g  we=%g  last we=%g\n"
		"          wena=%g  last wena=%g  next wena=%g"
	},
	{
		MH_R1324, "R1324: airHandler '%s' DX cool coil inconsistency: \n"
		"    slopef = %g but slopee = %g.\n"
		"           wen=%g wena=%g we=%g wexf=%g;\n"
		"           ten=%g texf=%g;  cs=%d,%d"
	},
	{
		MH_R1325, "R1325: airHandler '%s':\n"
		"    DX coil exit humidity ratio convergence failure.\n"
		"      inputs:  ten=%g  wen=%g  tex=%g\n"
		"      unfinished result: wex=%g  last wex=%g\n"
		"          teCOn=%g    weSatCOn=%g\n"
		"          plr=%g  frCOn=%g  last frCOn=%g"
	},
	{ MH_R1326, "R1326: airHandler %s: Inconsistency #1: total capacity (%g) not negative" },
	{ MH_R1327, "R1327: airHandler %s: Inconsistency #1a: sensible capacity (%g) not negative" },
	{
		MH_R1328, "R1328: airHandler %s: Inconsistency #2: sensible capacity (%g)\n"
		"    larger than total capacity (%g)"
	},
	{ MH_R1329, "R1329: airHandler %s: Inconsistency #3: wex (%g) > wen (%g)" },
	{ MH_R1330, "R1330: airHandler %s: Inconsistency #4: tex (%g) > ten (%g)" },
	{
		MH_R1331, "R1331: airHandler %s: Inconsistency #5: teCOn (%g) < te (%g)\n"
		"           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
		"           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
	},
	{
		MH_R1332, "R1332: airHandler %s: Inconsistency #6: weSatCOn (%g) < we (%g)\n"
		"           wen=%g wena=%g we=%g wexf=%g;  ten=%g texf=%g\n"
		"           ql=%g qs=%g q=%g;   plr=%g plrSens=%g;   case=%d,%d"
	},
	{ MH_R1333, "R1333: airHandler %s: Inconsistency #7: wex (%g) < weSatCOn (%g)" },
	{ MH_R1334, "R1334: airHandler %s: Inconsistency #8: wex (%g) < we (%g)" },
	{ MH_R1335, "R1335: airHandler %s: Inconsistency #9: wex (%g) < wexf (%g)" },
	{
		MH_R1336, "R1336: airHandler '%s':\n"
		"    heat coil capacity 'ahhcCaptRat' (%g) not > 0"
	},
	{
		MH_R1337, "R1337: airHandler '%s':\n"
		"    heat coil full-load energy input ratio 'ahhcEirR' (%g) not > 1.0"
	},
	{ MH_R1338, "R1338: cncoil.cpp:AH::coilsEndSubhr: Bad heat coil type 0x%x" },
	{ MH_R1339, "R1339: cncoil.cpp:AH::coilsEndSubhr: Bad cool coil type 0x%x" },
	{ MH_R1340, "R1340: airHandler '%s': frozen CHW cooling coil:\n    leaving water temperature computed as %g F or less" }, //12-3-92
	//cncoil additions 6-95
	{
		MH_R1341, "R1341: airHandler %s: Internal error in doAhpCoil: \n"
		"    arg to sqrt (%g) for quad formula hlf not >= 0.\n"
		"        cdm %g   pDfrhCon %g   capCon %g   qWant %g\n"
		"        A %g   B %g   C %g\n"
	},
	{
		MH_R1342, "R1342: airHandler %s: Internal error in doAhpCoil: \n"
		"    hlf (%g) not in range 0 < hlf <= qWant/capCon (%g)"
	},
	{
		MH_R1343, "R1343: airHandler %s: Internal error in doAhpCoil: \n"
		"    plf (%g) not > 0"
	},
	{
		MH_R1344, "R1344: airHandler %s: Internal error in doAhpCoil: \n"
		"    frCprOn (%g) > frFanOn (%g)"
	},
	{
		MH_R1345, "R1345: airHandler %s: Internal error in doAhpCoil: \n"
		"    q = %g but qWant is %g -- should be the same"
	},
// cnhp.cpp runtime errors 10-92
	{ MH_R1350, "R1350: heatPlant %s overload failure: q (%g) > stgCap[stgMxQ] (%g)" },
	{ MH_R1351, "R1351: heatPlant %s: bad stage number %d: not in range 0..%d" },

// cncp, cntp.cpp can go here...

// impf.cpp: import files (runtime) errors, 6-95
	{ MH_R1901, "R1901: Cannot open import file %s. No run." },
	{ MH_R1902, "R1902: Tell error (%ld) on import file %s. No run." },
	{ MH_R1903, "R1903: Seek error (%ld) on import file %s, handle %d" },
	{ MH_R1904, "R1904: Import File %s has too many lines. \n    Text at at/after line %d not used." },
	{ MH_R1905, "R1905: %s(%d): Internal error:\n    Import file names table record subscript %d out of range 1 to %d." },
	{ MH_R1906, "R1906: %s(%d): Internal error:\n    Import file subscript %d out of range 1 to %d." },
	{ MH_R1907, "R1907: %s(%d): Internal error:\n    Import file %s was not opened successfully." },
	{ MH_R1908, "R1908: %s(%d): End of import file %s: data previously used up.%s" },
	{ MH_R1909, "\n    File must contain enough data for CSE warmup days (default 7)." },
	{ MH_R1910, "R1910: Internal error: ImpfldDcdr::axscanFnm():\n    called w/o preceding successful file access" },
	{ MH_R1911, "R1911: %s(%d): Internal error:\n    no IFFNM.fnmt pointer for Import file %s" },
	{ MH_R1912, "R1912: %s(%d): Internal error: in IMPORT() from file %s\n    field name index %d out of range 1 to %d." },
	{
		MH_R1913, "R1913: %s(%d): Internal error:\n"
		"    in IMPORT() from file %s field %s (name index %d),\n"
		"    field number %d out of range 1 to %d."
	},
	{
		MH_R1914, "R1914: %s(%d): Too few fields in line %d of import file %s:\n"
		"      looking for field %s (field # %d), found only %d fields."
	},
	{
		MH_R1915, "R1915: Internal error: ImpfldDcdr::axscanFnr():\n"
		"    called w/o preceding successful file access."
	},
	{
		MH_R1916, "R1916: %s(%d): Internal error: in IMPORT() from file %s, \n"
		"    field number %d out of range 1 to %d."
	},
	{
		MH_R1917, "R1917: %s(%d): Too few fields in line %d of import file %s:\n"
		"      looking for field %d, found only %d fields."
	},
	{
		MH_R1918, "R1918: Internal error: ImpfldDcdr::decNum():\n"
		"    called w/o preceding successful field scan"
	},
	{ MH_R1919, "non-numeric value" },
	{ MH_R1920, "garbage after numeric value" },
	{ MH_R1921, "number out of range" },
	{ MH_R1922, "R1922: %s(%d): \n    Import file %s, line %d, field %s:\n      %s:\n          \"%s\"" },
	{ MH_R1923, "R1923: Import file %s:\n    File title (\"%s\") does not match IMPORTFILE imTitle (\"%s\")." },
	{ MH_R1924, "R1924: Import file %s: bad header format. No Run." },
	{ MH_R1925, "R1925: Import file %s:\n    File frequency (%s) does not match IMPORTFILE imFreq.  Expected %s." },
	{ MH_R1926, "R1926: Import file %s: Incorrect header format." },
	{ MH_R1927, "R1927: Import file %s:\n"
				"    The following field name(s) were used in IMPORT()s\n"
				"    but not found in import file %s:\n"
				"        %s  %s  %s  %s  %s" },
	{ MH_R1928, "R1928: Import file %s:\n    Premature end-of-file or error while reading header. No Run." },
	{ MH_R1929, "R1929: Import file %s, line %d:\n    record longer than %d characters" },
	{ MH_R1930, "R1930: Import file %s, line %d: second quote missing" },
	{ MH_R1931, "R1931: Read error on import file %s" },
	{ MH_R1932, "R1932: Close error on import file %s" },

// brfw.cpp: binary results file writing (runtime) errors, 11-94.
//brfw.cpp:ResfWriter messages
	{ MH_R1960, "R1960: ResfWriter: months not being generated sequentially" },
	{ MH_R1961, "R1961: ResfWriter::endMonth not called before next startMonth" },
	{ MH_R1962, "R1962: ResfWriter::startMonth not called before endMonth" },
	{ MH_R1963, "R1963: startMonth/endMonth out of sync with setHourInfo" },	// 2 uses
//brfw 6-95 additions
	{ MH_R1964, "R1964: ResfWriter::create: 'pNam' is NULL, \n    but file must be specified under DOS." },
	{
		MH_R1965, "R1965: ResfWriter::create: 'keePak' is non-0, \n"
		"    but memory handle return NOT IMPLEMENTED under DOS."
	},
	{ MH_R1966, "R1966: ResfWriter::close: 'hans' argument given\n    but 'keePak' was FALSE at open" },
	{
		MH_R1967, "R1967: ResfWriter::startMonth: repeating month %d.\n"
		"    Run cannot end in same month it started in \n"
		"    when creating hourly binary results file."
	},
//brfw.cpp:BinBlock messages
	{ MH_R1969, "R1969: BinBlock::init: 'discardable' and 'keePak' arguments should be 0 under DOS" }, 	// 6-95
	{ MH_R1970, "R1970: Out of memory for binary results file buffer" },
	{ MH_R1971, "R1971: Lock failure in BinBlock::alLock re binary results file" },
	{ MH_R1972, "R1972: Out of memory for binary results file packing buffer" },
	{ MH_R1973, "R1973: Lock failure in BinBlock::write re binary results file" },
	{ MH_R1974, "R1974: GlobalFree failure in BinBlock::write" },
	{ MH_R1975, "R1975: GlobalFree failure in BinBlock::free" },
	{ MH_R1976, "R1976: GlobalFree failure (packed block) in BinBlock::free" },
	{ MH_R1977, "R1977: BinBlock: Request to write block but file not open" },
	{ MH_R1978, "R1978: BinBlock: handle return for unpacked blocks not implemented" },	// 6-95
//brfw.cpp:BinFile messages
	{ MH_R1980, "R1980: Create failure, file \"%s\"" },
	{ MH_R1981, "R1981: Close failure, file \"%s\"" },
	{ MH_R1982, "R1982: Seek failure, file \"%s\"" },
	{ MH_R1983, "R1983: Write error on file \"%s\" -- disk full?" },
//1984-1987 free
//brfw.cpp non-mbr fcn msgs
	{ MH_R1988, "R1988: In brfw.cpp::%s: NULL source pointer " },
	{ MH_R1989, "R1989: In brfw.cpp::%s: NULL destination pointer" },

// --- IO errors ---
// xiopak.cpp --
	{ MH_I0001, "I0001: '%s' is a bad %s because '%s' is a DOS reserved device name" },
	{ MH_I0002, "I0002: invalid char '%c' after device name '%s' in '%s'" },
	{ MH_I0003, "I0003: bad drive letter '%c' in %s '%s'" },
	{ MH_I0004, "I0004: '%s': bad %s syntax (at '%c')" },
	{ MH_I0005, "I0005: bad character '%c' in %s '%s'" },
	{ MH_I0006, "I0006: bad %s '%s': illegal use of wild card '%s'" },
	{ MH_I0007, "I0007: '%s' is supposed to be a file name, but there is no name part" },
	{ MH_I0008, "I0008: %s '%s' is too long" },
	{ MH_I0009, "I0009: directory path portion of '%s' is too long" },
	{ MH_I0010, "I0010: unable to delete '%s':\n    %s" },
	{ MH_I0011, "I0011: unable to change working directory to '%s':\n    %s" },
	/*{ MH_I0012, "I0012: IO error, file '%s':\n    %s" },  in ram as occurs while opening/accessing error file 3-92 */

// xiopak2.cpp/xiopak3.cpp --
	{ MH_I0021, "I0021: unable to rename file '%s' to '%s':\n    %s" },
	{ MH_I0022, "I0022: file '%s' does not exist" },
	{ MH_I0023, "I0023: unable to create file '%s'" },
	{ MH_I0024, "I0024: file '%s' does not exist, cannot copy onto it" },
	{ MH_I0025, "I0025: not enough disk space to copy file '%s'" },
	{ MH_I0026, "I0026: read error during copy from file '%s':\n    %s" },
	{ MH_I0027, "I0027: write error during copy to file '%s':\n    %s" },

// dskdrive.cpp --
	{ MH_I0031, "I0031: '%c': not a valid drive letter" },

	//used with errFl
	{ MH_I0101, "I0101: Cannot open" },
	{ MH_I0102, "I0102: Cannot create" },
	{ MH_I0103, "I0103: Close error on" },
	{ MH_I0104, "I0104: Seek error on" },
	{ MH_I0105, "I0105: Write error on" },
	{ MH_I0106, "I0106: Read error on" },
	{ MH_I0107, "I0107: Unexpected end of file in" },
	{ MH_I0108, "I0108: Writing to file not opened for writing:" },
	//used with errFlLn
	{ MH_I0109, "I0109: Unexpected end of %s." },	// "line" or "file"
	{ MH_I0110, "I0110: Token too long." },
	{ MH_I0111, "I0111: Internal error: invalid character '%c' in control string in call to YACAM::get()." },
	{ MH_I0112, "I0112: Overlong string will be truncated to %d characters (from %d):\n    \"%s\"." },
	{ MH_I0113, "I0113: Invalid date \"%s\"." },
	{ MH_I0114, "I0114: Missing or invalid number \"%s\"." },
	{ MH_I0115, "I0115: Integer too large: \"%s\"." },
	{ MH_I0116, "I0116: Unrecognized character(s) after %snumber \"%s\"." },
	{ MH_I0117, "I0117: Internal error: Invalid call to YACAM::get(): \n    NULL not found at end arg list.\n" },
	//errFlLn meta-message, no msg number in text:
	{ MH_I0118, "%s '%s' (near line %d):\n  %s" },

// nb: no terminator, use msgTblCount
};

SI msgTblCount = sizeof(msgTbl)/sizeof(MSGTBL);		// number of msgTbl entries

// msgtbl.cpp end
