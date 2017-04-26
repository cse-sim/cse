// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cueval.h: public defines for users of cueval.c:
//  CSE expression evaluator pseudo-code interpreter;
//  also used in CSE preprocessor constant expression interpreter (ppcex.c)

///////////////////////////////////////////////////////////////////////////////
// pseudo-code operation codes
///////////////////////////////////////////////////////////////////////////////

/* These pseudo-codes are executed TWO PLACES:
	1) in the preprocessor constant expression interpreter (ppcex.c),
	   they are executed during parsing, e.g. in ppcex:biOp();
	2) In the user language proper, they are compiled by cuparse.c
	   into code streams which are interpreted by cueval.c.
	In BOTH CASES the codes are taken from entries in the operator table opTbl[] in cuparse.c. */

#if 0
#ifndef GLOB		// defined in cnglob.h for use in cuparse.h and multiple .c files, 1-92.
  typedef USI PSOP;	// type for pseudo-code opcodes
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// pseudo-code operation codes
///////////////////////////////////////////////////////////////////////////////
enum PSOPE {
PSNUL,		// means "output no code" in cuparse.c
PSEND,		// done - ends pseudo-code string

// load constants from following pseudo-code bytes
PSKON2,		// 2 bytes: eg SI
PSKON4,		// 4 bytes: float, or possibly a pointer
PSPKONN,	// load pointer to inline literal such as "text".
			//  followed by ~#words (excl self) and the literal.

// load values from memory locations, address in next 4 pseudo-code bytes
PSLOD2,		// 2 bytes
PSLOD4,		// 4 bytes

// load from stack frame (for fcn arguments)
PSRLOD2,	// load 2 bytes, offset rel frame ptr in next word
PSRLOD4,	// load (push) 4 bytes, FP offset follows

// pop values and store, address in next 4 bytes */
PSSTO2,		// 2 bytes
PSSTO4,		// 4 bytes

// store values and keep in stack. address in next 4 bytes.  USED?
PSPUT2,	 	// 2 bytes
PSPUT4,	 	// 4 bytes

// store into stack frame (fcn returning value or altering an argument)
PSRSTO2,	// store 2 bytes, offset rel frame ptr in next word
PSRSTO4,	// store (push) 4 bytes, FP offset follows

// duplicate value on stack top
PSDUP2,		// dup 2 bytes
PSDUP4,		// dup 4 bytes. CAUTION string ptrs now need cupIncRef

// adjust stack pointer
PSPOP2,		// discard 2 bytes (SI) (expr as statement)
PSPOP4,		// discard 4 bytes (float or string pointer) (ditto)

// conversions
PSFIX,		// float to SI
PSFLOAT,	// SI to float
PSIBOO,		// SI to "boolean": make any non-0 a 1
PSSCH,		// string to choicb or choicn. char * on stack, DTxxx (Dttab[]/dtypes.h) data type follows.
PSNCN,		// number-choice to number, ie error if contains choice
PSFINCHES,	// combine feet & inches: divide by 12.0, add.
PSIDOY,		// day of year 1-365: day of month 1-31 on stack; # days in month and 1st DOY of month 0-364 follow.

// integer arithmetic and tests
PSIABS,	// integer absolute value
PSINEG,	// negate (unary -)
PSIADD,	// add
PSISUB,	// subtract
PSIMUL,	// multiply
PSIDIV,	// divide
PSIMOD,	// %
PSIDEC,	// --
PSIINC,	// ++
PSINOT,	// not (!)
PSIONC,	// one's complement (bitwise)
PSILSH,	// << left shift
PSIRSH,	// >> right shift
PSIEQ,	// ==
PSINE,	// !=
PSILT,	// <
PSILE,	// <=
PSIGE,	// >=
PSIGT,	// >
PSIBOR,	// bitwise or: |
PSIXOR,	// bitwise xor: ^
PSIBAN,	// bitwise and
PSIBRKT,// bracket  lo, val, hi
PSIMIN,	// smaller of two args
PSIMAX,	// larger of two args

// float arith and tests
PSFABS,	// float absolute value
PSFNEG,	// negate (unary -)
PSFADD,	// add
PSFSUB,	// subtract
PSFMUL,	// multiply
PSFDIV,	// divide
PSFDEC,	// --
PSFINC,	// ++

PSFEQ,	// ==
PSFNE,	// !=
PSFLT,	// <
PSFLE,	// <=
PSFGE,	// >=
PSFGT,	// >
PSFBRKT,// bracket  lo, val, hi
PSFMIN,	// smaller of two args
PSFMAX,	// larger of two args

// float math functions
PSFSQRT,	// square root
PSFEXP,		// exp: e^x
PSFPOW,		// power: x^y
PSFLOGE,	// logE: natural logarithm
PSFLOG10,	// log10: base10 logarithm
PSFSIN,		// sin (radians)
PSFCOS,		// cos
PSFTAN,		// tan
PSFASIN,	// asin
PSFACOS,	// acos
PSFATAN,	// atan
PSFATAN2,	// atan2
PSFSIND,	// sin (degrees)
PSFCOSD,	// cos
PSFTAND,	// tan
PSFASIND,	// asin
PSFACOSD,	// acos
PSFATAND,	// atan
PSFATAN2D,	// atan2

// float psychrometric functions
PSDBWB2W,	// Humrat (w) from drybulb & wetbulb temps
PSDBRH2W,	// Humrat (w) from drybulb temp & relative humidity
PSDBW2H,	// Enthalpy (h) from drybulb temp & humrat

PSNOP,	// no-operation (place holder)

/* (future) string operations note:
     string ops will have to put string results in dm; they should then
     cueval.c:cupfree() their args to dmfree them if in dm but not if
     inline in pseudo-code as from PSPKONN.,-90. */

/* flow of control.
     signed jump displacements, in words, from own loc, in next 2 bytes */
/*  compile if ...      then ...           else ... ;      */
/*  to         ...  PSPJZ l1 ...  PSJMP l2  l1: ...  l2: ; */
PSJMP,		// unconditional branch
PSPJZ,		// pop si off stack, branch if zero
PSJZP,		// branch if si on stack top 0 ELSE pop it
PSJNZP,		// branch if si on stack top non-0 ELSE pop it
PSDISP,		// pop si off stack and DISPATCH.  followed by n, n jump offsets,
			// default offset (used if value not in range 0..n-1).  for choose(), hourval(), etc.
PSDISP1,	// 1-based version of above

/* control flow: absolute pseudo-code call/return for user fcns 12-90 */
PSCALA,		// absolute call to addr in next 4 bytes
PSRETA,		// return from pseudo-code called with PSCALA.
			// following words: # arg words to pop,
			//   # return value words to move.
// failure operations: issue err Msgs.  expect to add file/line info args
PSCHUFAI,	// default default for PSDISP: "index out of range"
PSSELFAI,	// default default for select(): "no true condition"

// prints: devel aids
PSIPRN,	// print integer
PSFPRN,	// print float
PSSPRN,	// print string

// rat (record array table) data access operations, move up at reorder?
PSRATRN,	// rat record by number: ratN inline, number (subscript) on stack, leaves record address on stack
//PSRATRS,	xx rat record by name: ratN inline, string on stack, leaves record address on stack
PSRATROS,	// rat record by default owner and name: ratN inline, 0 or default owner subscript inline,
			//  string on stack, leaves record address on stack.
// following rat loads all add offset of field number in next 2 bytes to record address on stack.
#ifdef wanted				// not wanted 12-91: there are no UCH or CH values in records.
w  PSRATLOD1U,	// rat load 1 unsigned byte: fetchs UCH, converts to 2 bytes w/o sign extend
w  PSRATLOD1S,	// rat load 1 signed byte: fetchs char, converts to 2 bytes, extending sign.
#endif
PSRATLOD2,	// rat load 2 bytes: fetches SI/USI.
PSRATLOD4,	// rat load 4 bytes: fetches float/LI/ULI.
PSRATLODD,	// rat load double: converts it float.
PSRATLODL,	// rat load long: converts it float.
PSRATLODA,	// rat load char array (eg ANAME): makes dm copy, leaves ptr in stack
PSRATLODS,	// rat load string: loads char * from record, duplicates.

// expression data access: used when an expr references (probes) an input data location already containing an expression.
PSEXPLOD4,	// load 4 byte (li,float) expr value.  2-byte expression number follows inline.
PSEXPLODS,	// load string expr value: make duplicate heap (dm) copy.  ditto inline.

// import file field loads Followed by --    then source file index, line #.
PSIMPLODNNR,	// load number from field by field #. file index (ImpfiB subscr), field # (column #).
PSIMPLODSNR,	// load string from field by field #. file index (ImpfiB subscr), field name index.
PSIMPLODNNM,	// load number from named field.      file index (ImpfiB subscr), field # (column #).
PSIMPLODSNM,	// load string from named field.      file index (ImpfiB subscr), field name index.

// daylighting controls simulation functions
PSCONTIN,	// simulate "continuous" light control. 4 float args, 1 float result.
PSSTEPPED	// simulate "stepped" light control. 1 SI and 2 float args, 1 float result.

/* to do (8-90) (turns out not to be important in actual use,,1)
* PSCALL <<<<<<<<< need PS addressing mechanism
*   for user-defined functions
* load and store variables. by variable # or assign abs locations?
* many specials for built-in fcns?
*/
};	// enum EPSOP


/*----------------------------- OTHER DEFINES -----------------------------*/


/*--------------------------- PUBLIC VARIABLES ----------------------------*/
XSI NEAR runtrace;	// non-0 to display debugging info during execution

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// cueval.c
RC FC cuEvalTop( void *ip);
RC FC cuEvalR( void *ip, void **ppv, const char **pmsg, USI *pBadH);
RC FC cupfree( DMP *p);						// was RC FC cupfree( void **pp); 7-92
RC FC cupIncRef( DMP *p, int erOp=ABT);
char * FC cuStrsaveIf( char *s);
int CDEC printif( int flag, const char* fmt, ... );

// end of cueval.h
