// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cuparsei.h  Operator table defines and decls for cuparse.c, ppCex.c
	       for cal non-res user language */


/*--------------------- OPERATOR TABLE definitions ----------------------*/

/*----- "cs" values for opTbl, cuparse:expr(), ppcex:ceval() */
#define CSUNI	101	// unary integer operator
#define CSUNN	102	// unary numeric operator.  v1,2 are int, float psc
#define CSBII	103	// binary integer operator
#define CSBIF	104	// binary float operator ( ' for ft-inches)
#define CSBIN	105	// binary numeric operator. .v1, .v2 ditto
#define CSCMP	106	// comparison (args numeric, result int)
#define CSLEO	107	// && or ||. v1 is cond jmp/pop to put between exprs
#define CSGRP	108	// grouper ( [
#define CSCUT	109	// has subcase by token type
#define CSU	110	// error if dispatched to (terminators)

/*----- "prec" (precedence) values for opTbl, cuparse:expr(), ppcex:ceval() */
	/* Data type SI.  code assumes order of values !!!. */
#define PREOF  1	// end of file or end of preprocessor cmd
#define PRVRB  2	// verbs (statement-initiating keywords)
	//verbs have unique prec so can test prec to test for any verb
#define PRVRBL 3	// other verbLike words: AUTOSIZE, LIKE, TYPE,...
#define PRTY   5	// types INTEGER,FLOAT,... . verb-like and other uses.
	// unique prec so can test for types as a group (not yet used 12-90)
#define PRSEM  6	// ;
#define PRRGR  7	// ) ]: close group, end fcn call
#define PRDEFA 8	// "default" (replaces , in certain fcn calls) and misc keywords: FUNCTION, VARIABLE, .
#define PRCOM  9	// ,
#define PRASS  10	// assignment
// ... many included literally in opTbl[] ...
//	13  ?			12  :  (?: operator)
//	15  ||  		16  &&
//	17  |			18  ^			19  &
//	24  ==  !=		25  <  <=  >  >=	28  <<  >>
//	30  + -			32  *  /  %
//	34  !  ~  unary + -  month words (jan-dec, impl as unary ops)
//	36  ' (feet-inches, 2-92.  Note binding power hier than unary - )
#define PROP  40	/* all operands are this value OR LARGER: constant, variable, fcn ref, (, [,  (expr), etc.
			   (use diff values if want to distinguish by prec) */
#define perr   99	// not understood by parser


/*------- OPERATOR/OPERAND TABLE to drive two parsers:
	    preprocessor parser in ppCex.c, and compiler parser in cuparse.c.
	  subscript is token type (defines in cutok.h)
	    as returned by cutok.c:cuTok() and refined by cuparse.c:toke(),
	    or returned by ppTok.c:ppTok() and refined by ppCex:ppToke(). */
struct OPTBL
{   SI prec;  		// (left) precedence: determines order of operations
    SI cs;		// case:      CSBIN   CSUNN   CSGRP
    SI v1;		// value 1:  si ps   si ps   expected CUTxx
    SI v2;		// value 2:  fl ps   fl ps   exp. 'token'
    char NEAR * tx;	// for ttTx, thence error messages.  NEAR added 3-20-91, shortens program w/o lengthening DGROUP.
};
extern OPTBL opTbl[];	// data is in cuparse.c.  for Borland C++ force far as threshold fails for init data.

// end of cuparsei.h
