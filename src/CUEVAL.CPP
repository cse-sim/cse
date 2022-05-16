// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cueval.cpp: cal non-res expression evaluator pseudo-code interpreter


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// CSE global definitions. ASSERT.

/*-------------------------------- OPTIONS --------------------------------*/
#define PROBE	// define for features to support CSE record data probes. Undefine for portable linkability.
#define IMPORT	// define for features to support CSE Import() function; Undefine for portable linkability. 2-94.

#ifdef PROBE
#include <SRD.H>		// SFIR
#include <ANCREC.H>		// record: base class for rccn.h classes
#include <rccn.h>		// Top.isEndOf
#endif
#include <MSGHANS.H>	// MH_R0201

#include <MESSAGES.H>	// msgIsHan
#include <CVPAK.H>		// cvS2Choi

#include <PSYCHRO.H>	// functions in lib\psychro1/2.cpp. psyEnthalpy psyHumRat1.
#ifdef IMPORT
#include <IMPF.H>		// impFldNrN
#endif

#include <CUEVAL.H>		// PS defines, RCUNSET; decls for fcns in this file
#ifdef PROBE
#include <CUL.H>		// FsSET FsVAL
#include <CUPARSE.H>	// evfTx     must follow cueval.h, for PSOP
#include <CNGUTS.H>		// Top
#include <CUEVF.H>
#include <EXMAN.H>		// whatEx whatNio exInfo
#endif
#include "XIOPAK.H"


/*-------------------------------- DEFINES --------------------------------*/
// see cueval.h

/*------------------------------- VARIABLES -------------------------------*/

//-- eval stack.  Builds to LOWER memory addresses.  Entry sizes vary.
LOCAL char evStk[ 2000];			// stack buffer. may reallocate later
LOCAL void* evStkTop = evStk + 8;	// stack top for overflow check, room for 2 extra float pushes
LOCAL void* evStkBase = evStk + sizeof(evStk) - 4;
// initial sp initial / underflow ck value, room for extra float pop
union _MTP
{
	void* p;
	void** pp;
	record** ppr;
	SI* pi;
	SI** ppi;
	USI* pu;
	float* pf;
	LI* pl;
	LI** ppl;
	PSOP* pOp;
	char* pc;
	char** ppc;
};
static _MTP _evSp = { NULL };		// multi-type stack pointers
#define evSp _evSp.p		// stack pointer, to last pushed item
#define SPP _evSp.pp		// .. as pointer TO a POINTER
#define SPPR _evSp.ppr		// .. as pointer TO a POINTER to a 'record' (ancrec.h)
#define SPI _evSp.pi		// .. as pointer to SI
#define SPU _evSp.pu		// .. as pointer to USI
#define SPF _evSp.pf		// .. as pointer to float
#define SPL _evSp.pl		// .. as pointer to 4 generic bytes
#define SPC _evSp.pc		// .. as pointer to char
#define SPCC _evSp.ppc		// .. as pointer to pointer to char

// stack frame pointer: evSp at call, above args and ret address
static void* evFp = NULL;  	// stack frame ptr

// pointer to psuedo-code being interpreted
static _MTP _evIp = { NULL };
#define evIp _evIp.p
#define IPP _evIp.pp
#define IPI _evIp.pi		// .. as ptr to SI
#define IPU _evIp.pu		// .. as ptr to USI
#define IPPI _evIp.ppi
#define IPL _evIp.pl
#define IPPL _evIp.ppl
#define IPOP _evIp.pOp		// .. as ptr to pseudoCode OpCode

// display debugging info flag
SI NEAR runtrace = 0;		// settable as sys var $runtrace (cuparse.cpp)

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC FC NEAR cuEval( void *ip, const char** pmsg, USI *pBadH);
LOCAL RC FC NEAR cuEvalI( const char** pmsg, USI *pBadH);
#ifdef PROBE					// features that depend especially on being linked in CSE environmemt
LOCAL RC FC NEAR cuRmGet( void **pv, const char** pms, USI *pBadH);
LOCAL RC FC NEAR cuRm2Get( SI *pi, const char** pms, USI *pBadH);
LOCAL const char * FC NEAR ivlTx( IVLCH ivl);
#endif

/*lint -e124 "Pointer to void not allowed" when comparing void ptrs */
/*lint -e52 "Expected an lvalue" on every "((SI*)evSp)--" etc */
/*lint -e63 "Expected an lvalue" on every "(char *)evSp += ..." */

#if 1		// idea, used in test program, may not be pertinent
//===========================================================================
RC FC cuEvalTop( void *ip)		// evaluate psuedocode at ip, check for empty stack when done

// for use where code is supposed to store all results

// issues message and returns -1. if error.
{
	RC rc;

	rc = cuEval(ip, NULL, NULL);  	// evaluate psc at ip.  NULL: issue any error message in cuEval.
	if (evSp != evStkBase)		// verify stack empty
		return err( PWRN,				// display internal error msg, await key, ret RCBAD, rmkerr.cpp
		(char *)MH_R0201,			// "cuEvalTop: %d words left on eval stack"
		INT( (SI *)evStkBase - SPI) );
	return rc;
	// additional return(s) above
}			// cuEvalTop
#endif

#ifdef wanted		// idea, may not be pertinent
w //===========================================================================
w FLOAT FC cuEvalF( void *ip)
w /* evaluate psuedocode at ip, return float result */
w
w /* issues message and returns -1. if error */
w{
w     if (cuEval( ip, NULL, NULL))	/* evaluate psc at ip, leave result in stack (next) */
w        return -1.f;
w     if (evSp != (char *)evStkBase + sizeof(FLOAT))
w     {	err( PWRN, "cuEvalF: result not exactly 1 float");
w        return -1.f;
w}
w     return *SPF++;
w     /* 2 additional returns above */
w}			/* cuEvalF */
#endif

//===========================================================================
RC FC cuEvalR( 		// evaluate pseudocode & return ptr to value

	void *ip, 	// ptr to code to evaluate

	void **ppv,	/* receives ptr to value (ptr to ptr to TYSTR).
		   Caller must know value type/size.
		   Value must be moved before next call to any eval fcn.
		   For TYSTR, value may be ptr to text INLINE IN CODE. */
	const char **pmsg,	// NULL or receives ptr to un-issued Tmpstr error msg.
    					// CAUTION: msg is in transitory temp string storage: use or strsave promptly.
	USI *pBadH )	// NULL or receives 0 or expr # of uneval'd expr when RCUNSET is returned.

// purpose of this level is to keep eval stack local to cueval.cpp.

/* returns RCOK if all ok.
   On error, returns non-RCOK value.  If pmsg is NULL, message has been issued,
	     else *pmsg receives [NULL or] ptr to un-issued msg: allows caller
	     to add text explaining location of error to message.
   specific error return codes include: RCUNSET,  (see cuEval comments, next). */
{
	RC rc;

	rc = cuEval( ip, pmsg, pBadH);	// evaluate to *evStk
	if (ppv != NULL)				// return pointer to value in eval stack
		*ppv = evSp;				//  for TYSTR, stack contains ptr.
	if (rc==RCOK)			// if ok, check size of value: devel aid
	{
		// remove if other sizes become possible
		SI sz = (SI)((char *)evStkBase - (char *)evSp);
		if (sz != 2  &&  sz != 4)
		{
			const char* ms = strtprintf( (char *)MH_R0202, (INT)sz); 	// "cuEvalR internal error: \n    value %d bytes: neither 2 nor 4"
			if (pmsg)
				*pmsg = ms;  	// return TRANSITORY message pointer
			else
				err( PWRN, ms);	// issue internal error msg, wait keypress, rmkerr.cpp
			return RCBAD;
		}
	}
	return rc;
}		// cuEvalR
// make public if need found:
//===========================================================================
LOCAL RC FC NEAR cuEval(	// evaluate, leave any result in stack

	void *ip,	// pseudocode to evaluate

	const char** pmsg,	// NULL or receives ptr to un-issued Tmpstr error msg
	USI *pBadH )		// NULL or receives 0 (for UNSET, or SI expr)
						//  or unevaluated expr # if RCUNSET returned

/* returns RCOK if all ok.
   On error, returns non-RCOK value.  If pmsg is NULL, message has been issued,
	     else *pmsg receives [NULL or] ptr to un-issued msg:
	     allows caller to add text explaining location of error to message.
   specific error codes include:
         RCUNSET, unset value or un-evaluated expression accessed (PSRATLODxx), 0 or handle returned *pBadH. */
{
// init pseudo-code eval stack.  variables in this file.
	evSp = evStkBase;				// stack pointer
	evFp = NULL;				// frame pointer: no current call
// init pseudo-code instruction pointer to caller's
	evIp = ip;
// evaluate.  Issues message if bad pseudo-code.
	return cuEvalI( pmsg, pBadH);		// uses evIp, evSp, etc. next. only call 2-91.
}			    	    // cuEval
//===========================================================================
LOCAL RC FC NEAR cuEvalI(

// expression eval pseudo-code interpreter inner loop

	const char **pmsg,	// NULL or receives ptr to un-issued Tmpstr error msg.
    					// CAUTION: msg is in transitory temp string storage: use or strsave promptly.
	USI *pBadH )	// NULL or receives 0 (for UNSET, or SI expr) or unevaluated expr # if RCUNSET returned
// and uses: evIp, evSp,

/* returns RCOK if all ok.
   On error, returns non-RCOK value.  If pmsg is NULL, message has been issued,
	     else *pmsg receives [NULL or] ptr to un-issued msg: allows caller
	     to add text explaining location of error to message.
   specific error codes include:
             RCUNSET, unset value or un-evaluated expression accessed (PSRATLODxx), 0 or handle returned *pBadH. */
{
	PSOP op;   			// pseudo op code being interpreted
	const char *ms = NULL;	// init to no error
	RC rc = RCOK;			// if error (ms nz), RCBAD will be supplied at exit unless other nz value set.
	SI idx=0, lo=0, hi=0;		// errMsg info, PSDISP_ to PSCHUFAI.

	// code assumes LI's, floats, char * are same size
	static_assert(sizeof(LI) == sizeof(float));
	static_assert(sizeof(LI) == sizeof(char*));
	static_assert( sizeof(SI)==sizeof(PSOP));		// assumed in (SI *) cast used in PSPKONN case

	for ( ; ; )
	{
		// eval stack over/underflow checks. NB builds down, top is < base.
		if (evSp < evStkTop)
		{
			ms = strtprintf( (char *)MH_R0204, evIp);	// "cuEvalI internal error: \n    eval stack overflow at ps ip=%p"
			break;
		}
		if (evSp > evStkBase)
		{
			ms = strtprintf( (char *)MH_R0205, evIp);	// "cuEvalI internal error: \n    eval stack underflow at ps ip=%p"
			break;
		}

		// fetch next pseudo-code
		op = *IPOP++;
		printif( runtrace, "  ip=0x%x sp=%d op=%d  ",
			(UI)evIp, (INT)evSp, (INT)op);

		//--- execute pseudo-code ---
		switch (op)
		{
			SI siTem;
			void *v;
			SI n, i, defO;
			RC trc;
			USI h;
			void *tIp, *tSp, *p;
			BP b;
			record *e;
#ifdef IMPORT
			float fv;
			int fileIx;
			int line;
#endif

			//--- constants inline in code.  Move 4 bytes as long not float to avoid 8087 exceptions.
		case PSKON2:
			*--SPI = *IPI++;
			break; 	// 2 bytes
		case PSKON4:
			*--SPL = *IPL++;
			break; 	// 4 bytes
		case PSPKONN: // PSPKONN nwords literal: load ptr to inline const
			*--SPP = (char *)(IPOP + 1);		// push ptr to literal
			IPOP += (~*(SI *)IPOP) + 1;		// evIp: pass ~nwords & lit.
			break;					// ... (SI *) so 32-bit version sign-extends b4 ~.

			//--- load constants or variables from memory locations
		case PSLOD2:
			*--SPI = *(*IPPI++);
			break;	// 2 bytes
		case PSLOD4:
			*--SPL = *(*IPPL++);
			break;	// 4 bytes

			//--- load 2, 4 bytes from stack frame (fcn args 12-90).  signed offset in words is in next word.
		case PSRLOD2:
			*--SPI = *((SI *)evFp + *IPI++);
			goto evFck;
		case PSRLOD4:
			*--SPL = *(LI*)((SI *)evFp + *IPI++);
		evFck:
			if (evFp == NULL)				// late no-frame check
			{
				ms = strtprintf((char *)MH_R0206, evIp);	/* "cuEvalI internal error: \n"
											   "    arg access thru NULL frame ptr at ip=%p" */
				goto breakbreak;
			}
			break;

			//--- pop and store variables to memory locations
		case PSSTO2:
			*(*IPPI++) = *SPI++;
			break;	// 2 bytes
		case PSSTO4:
			*(*IPPL++) = *SPL++;
			break;	// 4 bytes

			//--- store variables to memory locations without removing from stack
		case PSPUT2:
			*(*IPPI++) = *SPI;
			break;	// 2 bytes
		case PSPUT4:
			*(*IPPL++) = *SPL;
			break;	// 4 bytes

			//--- store 2, 4 bytes into stack frame (fcn return or changing arg).  Signed offset in words is in next word. 12-90.
		case PSRSTO2:
			*((SI *)evFp + *IPI++) = *SPI++;
			goto evFck;
		case PSRSTO4:
			*(LI *)((SI *)evFp + *IPI++) = *SPL++;
			goto evFck;

			//--- duplicate stack top
		case PSDUP2:
			--SPI;
			*SPI = *(SPI + 1);
			break; 	// 2 bytes
		case PSDUP4:
			--SPL;
			*SPL = *(SPL + 1);
			break;  	// 4 bytes. CAUTION string ptrs now need cupIncRef, 7-92

			//--- conversions
		case PSFIX:
			siTem = (SI)*SPF;
			SPC += sizeof(float) - sizeof(SI);
			*SPI = siTem;
			break;

		case PSFLOAT:
			siTem = *SPI;
			SPC += sizeof(SI) - sizeof(float);
			*SPF = (float)siTem;
			break;

		case PSIBOO:
			*SPI = (*SPI != 0);
			break;	// make any non-0 a 1

		case PSSCH:			// string (stack) to choice (stack) per data type (inline) 2-92
			if (cvS2Choi((char*)*SPP++, *IPU++, (void *)&v, &h, &ms) == RCBAD)  	// cvpak.cpp
				goto breakbreak;							// if error (ms set)
			ms = NULL;		// insurance, drop possible info msg
			if (h == 4)  *--SPP = v;				// if a 4-byte choice type (choicn), return 4 bytes
			else       *--SPI = *(SI*)&v;		// assume 2 bytes: choicb
			break;

		case PSNCN:			// number-choice to number 'conversion'
			if (ISNUM(v = *SPP))  break;			// if stack top is a number (cnglob.h), ok, done
			//note that if 2-byte choice gets here (shouldn't) it is taken as a number.
			if (ISNCHOICE(v))
				ms = strtprintf((char *)MH_R0207, (INT)CHN(v) & ~NCNAN);	// "Choice (%d) value used where number needed"
			else if (ISNANDLE(v))
				if (ISUNSET(v))
					ms = (char *)MH_R0208;			// "Unexpected UNSET value where number neeeded"
#ifdef AUTOSIZE		// found "ifdef AUTOSIZING" ?? 4-16-10
				else if (ISASING(v))
					ms = (char *)MH_R0231;			// "Unexpected being-autosized value where number needed"
#endif
				else
					ms = strtprintf((char *)MH_R0209, (INT)EXN(v));	// "Unexpected reference to expression #%d"
			else
				ms = (char *)MH_R0210;			// "Unexpected mystery nan" (text retreived below)
			goto breakbreak;

		case PSFINCHES:		// combine float inches with float feet
			// do we wanna check for 0 <= inches < 12?
			*SPF /= 12.0f;		// divide inches by 12
			goto psfAdd;	 	// go add to feet.

		case PSIDOY:  			// day of month to day of year.
			// Month known at compile time. Stack top: day of month.
			// inline: # days in month, 1st day of month less 1.
			if (*SPI < 1 || *SPI > *(SI*)evIp)					// check DOM
			{
				ms = strtprintf((char *)MH_R0211, (INT)*SPI, (INT)*(SI*)evIp);	// "Day of month %d is out of range 1 to %d"
				goto breakbreak;
			}
			IPI++;		// point past # days in month
			*SPI += *IPI++;	// DOM + 0-based 1st day of month = DOY
			break;

			//--- integer arithmetic and tests
		case PSIABS:
			if (*SPI >= 0)  break;			// abs value 10-95
			// else fall thru
		case PSINEG:
			*SPI = -*SPI;
			break;			// negate
		case PSIADD:
			*(SPI + 1) += *SPI;
			goto sipop;		// add
		case PSISUB:
			*(SPI + 1) -= *SPI;
			goto sipop;		// sub
		case PSIMUL:
			*(SPI + 1) *= *SPI;
			goto sipop;		// mul
		case PSIDIV:
			if (*SPI == 0)
			{
				ms = (char *)MH_R0212;     // "Division by 0"
				goto breakbreak;
			}
			*(SPI + 1) /= *SPI;
			goto sipop;		// div
		case PSIMOD:
			*(SPI + 1) %= *SPI;
			goto sipop;		// mod
		case PSIDEC:
			(*SPI)--;
			break;		// --
		case PSIINC:
			(*SPI)++;
			break;		// ++
		case PSINOT:
			*SPI = !*SPI;
			break;		// not (!)
		case PSIONC:
			*SPU = ~*SPU;
			break;		// 1's compl
		case PSIEQ:
			siTem = (*(SPI + 1) == *SPI);
			goto si21;	// ==
		case PSINE:
			siTem = (*(SPI + 1) != *SPI);
			goto si21;	// !=
		case PSILT:
			siTem = (*(SPI + 1) < *SPI);
			goto si21;	// <
		case PSILE:
			siTem = (*(SPI + 1) <= *SPI);
			goto si21;	// <=
		case PSIGE:
			siTem = (*(SPI + 1) >= *SPI);
			goto si21;	// >=
		case PSIGT:
			siTem = (*(SPI + 1) > *SPI);  		// >
		si21:
			SPI++;
			*SPI = siTem;
			break;		// binary int result
		case PSILSH:
			*(SPU + 1) <<= *SPU;
			goto sipop;	// << lsh
		case PSIRSH:
			*(SPU + 1) >>= *SPU;
			goto sipop;	// >> rsh
		case PSIBOR:
			*(SPI + 1) |= *SPI;
			goto sipop;	// bit or
		case PSIXOR:
			*(SPI + 1) ^= *SPI;
			goto sipop;	// bit xor
		case PSIBAN:
			*(SPI + 1) &= *SPI;
			goto sipop;	// bit and
		case PSIBRKT:
			if (*(SPI + 1) > *SPI)	// bracket (lo, val, hi)
				*(SPI + 1) = *SPI;	//  value > hi: use hi
			SPI++;			//  discard
			// fall thru to use max(lo, val)
		case PSIMAX:
			if (*(SPI + 1) > *SPI)	// max( a, b)
				goto sipop;
			goto siyank;
		case PSIMIN:
			if (*(SPI + 1) < *SPI)	// min( a, b)
				goto sipop;
			//goto siyank;
		siyank:
			*(SPI + 1) = *SPI; 	// discard next to top value
		case PSPOP2:
		sipop:
			SPI++;
			break;   	// pop 1 int

			//--- float arithmetic and tests
		case PSFABS:
			if (*SPF >= 0)     break;   		// abs value 10-95
			// else fall thru
		case PSFNEG:
			*SPF = -*SPF;
			break;		// negate
		psfAdd:;				// PSFINCHES joins
		case PSFADD:
			*(SPF + 1) += *SPF;
			goto flpop;		// add
		case PSFSUB:
			*(SPF + 1) -= *SPF;
			goto flpop;		// sub
		case PSFMUL:
			*(SPF + 1) *= *SPF;
			goto flpop;		// mul
		case PSFDIV:
			if (*SPF == 0.)
			{
				ms = (char *)MH_R0213;     // "Division by 0."
				goto breakbreak;
			}
			*(SPF + 1) /= *SPF;
			goto flpop;		// div
		case PSFDEC:
			(*SPF)--;
			break;		// --
		case PSFINC:
			(*SPF)++;
			break;		// ++
		case PSFBRKT:
			if (*(SPF + 1) > *SPF)		// bracket (lo, val, hi)
				*(SPF + 1) = *SPF;		//  value > hi: use hi
			SPF++;				//  discard
			// fall thru to use max(lo, val)
		case PSFMAX:
			if (*(SPF + 1) > *SPF)		// max( a, b)
				goto flpop;
			goto flyank;
		case PSFMIN:
			if (*(SPF + 1) < *SPF)		// min( a, b)
				goto flpop;
			//goto flyank;
		flyank:
			*(SPF + 1) = *SPF; 		// discard next to top value
		case PSPOP4:
		flpop:
			SPF++;
			break;				// pop 1 float
		case PSFEQ:
			siTem = (*(SPF + 1) == *SPF);
			goto fl2i1; 	// ==
		case PSFNE:
			siTem = (*(SPF + 1) != *SPF);
			goto fl2i1; 	// !=
		case PSFLT:
			siTem = (*(SPF + 1) < *SPF);
			goto fl2i1; 	// <
		case PSFLE:
			siTem = (*(SPF + 1) <= *SPF);
			goto fl2i1; 	// <=
		case PSFGE:
			siTem = (*(SPF + 1) >= *SPF);
			goto fl2i1; 	// >=
		case PSFGT:
			siTem = (*(SPF + 1) > *SPF);			// >
		fl2i1:
			SPC += 2 * sizeof(float) - sizeof(SI);	// here to pop 2 floats, push SI in siTem
			*SPI = siTem;
			break;

			//--- float math function
		case PSFSQRT:
			if (*SPF < 0.f)
			{	ms = (char *)MH_R0214;     // "SQRT of a negative number"
				goto breakbreak;
			}
			*SPF = sqrt(*SPF);
			break;			// square root, to support ahhcStackEffect, 7-4-92.
		case PSFEXP:
			if (*SPF < -87.f)	// exp( -87) = 1.656e-38 approx (slightly > FLT_MIN)
			{	ms = "exp( x) underflow, x < -87";	// NEWMS
				goto breakbreak;
			}
			if (*SPF > 88.5f)	// exp( 88.5) = 2.723e38 approx (slightly < FLT_MAX)
			{	ms = "exp( x) overflow, x > 88.5";	// NEWMS
				goto breakbreak;
			}
			*SPF = exp(*SPF);
			break;			// exp: e^x
		case PSFPOW:
			if (*SPF==0.f && *(SPF+1)==0.f) 		// pow(x,y): x^y: raise x to the y power
			{	ms = "zero to the zeroth power";	// NEWMS
				goto breakbreak;
			}
			if (*(SPF+1) < 0.f  &&  float(int(*SPF)) != *SPF)
			{	ms = "negative number to non-integral power";     // NEWMS
				goto breakbreak;
			}
			*(SPF+1) = pow( *(SPF+1), *SPF);
			goto flpop;
		case PSFLOGE:
			if (*SPF < 0.)
			{
				ms = "log of a negative number";     // NEWMS 2 uses
				goto breakbreak;
			}
			*SPF = log(*SPF);
			break;			// logE: natural logarithm
		case PSFLOG10:
			if (*SPF < 0.)
			{
				ms = "log of a negative number";     // NEWMS 2 uses
				goto breakbreak;
			}
			*SPF = log10(*SPF);
			break;			// log10: base 10 logarithm
		case PSFSIN:
			*SPF = sin(*SPF);
			break;			// sin
		case PSFCOS:
			*SPF = cos(*SPF);
			break;			// cos
		case PSFTAN:
			*SPF = tan(*SPF);
			break;			// tan
		case PSFASIN:
			if (fabs(*SPF) > 1.)
			{	ms = "asin of number not -1 - +1";     // NEWMS
				goto breakbreak;
			}
			*SPF = asin( *SPF);
			break;			// asin
		case PSFACOS:
			if (fabs(*SPF) > 1.)
			{	ms = "acos of number not -1 - +1";     // NEWMS
				goto breakbreak;
			}
			*SPF = acos( *SPF);
			break;			// acos
		case PSFATAN:
			*SPF = atan( *SPF);
			break;			// atan
		case PSFATAN2:
			*(SPF+1) = atan2( *(SPF+1), *SPF);
			goto flpop;		// atan2

		case PSFSIND:
			*SPF = sin( RAD( *SPF));
			break;			// sin (degrees)
		case PSFCOSD:
			*SPF = cos( RAD( *SPF));
			break;			// cos (degrees)
		case PSFTAND:
			*SPF = tan( RAD( *SPF));
			break;			// tan (degrees)
		case PSFASIND:
			if (fabs(*SPF) > 1.)
			{	ms = "asind of number not -1 - +1";     // NEWMS
				goto breakbreak;
			}
			*SPF = DEG( asin( *SPF));
			break;			// asin (degrees)
		case PSFACOSD:
			if (fabs(*SPF) > 1.)
			{	ms = "acosd of number not -1 - +1";     // NEWMS
				goto breakbreak;
			}
			*SPF = DEG( acos( *SPF));
			break;			// acos (degrees)
		case PSFATAND:
			*SPF = DEG( atan( *SPF));
			break;			// atan (degrees)
		case PSFATAN2D:
			*(SPF+1) = DEG( atan2( *(SPF+1), *SPF));
			goto flpop;		// atan2 (degrees)

		//--- float psychrometric functions, 10-95
		case PSDBWB2W:
			*(SPF+1) = psyHumRat1( *(SPF+1), *SPF);		// humrat (w) from drybulb & wetbulb temps
			goto flpop;
		case PSDBRH2W:
			*(SPF+1) = psyHumRat2( *(SPF+1), *SPF);		// humrat (w) from drybulb temp & relative humidity
			goto flpop;
		case PSDBW2RH:
			*(SPF + 1) = psyRelHum4(*(SPF + 1), *SPF);	// rel hum (0 - 1) from drybulb temp & humrat
			goto flpop;
		case PSDBW2H:
			*(SPF+1) = psyEnthalpy( *(SPF+1), *SPF);	// enthalpy (h) from drybulb temp & humrat
			goto flpop;

		case PSNOP:
			break;				// no-operation (place holder)

			/* (future) string operations notes:
			   String operations will store their results in dm, eg with strsave.
			   They should free their arguments with cupfree (below) which will
			   dmfree values in dm without disturbing pointers to strings
			   inline in code, as from PSPKONN above. 10-90. */

			//--- flow of control.  signed jump displacements, in words, from self, in next 2 bytes.

		case PSJZP:
			if (*SPI==0)	// branch if 0 ELSE pop (for &&)
				goto jmp;
			goto noJmpPop;

		case PSJNZP:
			if (*SPI)	// branch if nz ELSE pop (for ||)
				goto jmp;
noJmpPop:
			SPI++;			// pop int off of run stack
			IPOP++;
			break;		// pass unused branch offset

		case PSPJZ:
			if (*SPI++)	// pop si, branch if zero
			{
				IPOP++;		// pass branch offset
				break;		// do following psuedoCode
			}
			/*lint -e616  control falls into case*/
		case PSJMP:			// unconditional branch
jmp:
			if (*IPOP==0xffff)	// if unfilled jmp (during compile, eg constizing)
				goto breakbreak;	// treat as terminator
			IPOP += *IPOP;
			break;

		case PSDISP1:		// 1-based dispatch
			(*SPI)--;			// decr index to make 0-based
			/*lint -e616 */
		case PSDISP:		// dispatch.  stack: 0-based index.  code: n, n offsets, default offset or 0.
			siTem = *SPI++;			// get dispatch index from stk
			n = *IPOP++;			// get # choices from code
			if (siTem >= 0 && siTem < n)	// if in range
			{
				IPOP += siTem;		// point offset to use
				goto jmp;			// go jump using offset
			}
			IPOP += n;			// point default offset
			// info for errmsg just below or in PSCHUFAI
			i = (*(IPOP-n-2)==PSDISP1);	// 1 if 1-based
			idx = siTem+i;
			lo = i;
			hi = n-1+i;
			// if PSCHUFAI always supplied, eliminate test for 0:
			if (*IPOP)			// if offset non-0, use it ...
				goto jmp;			// may go to a PSCHUFAI. idx,lo,hi set.
			/*lint -e616 else fall in */

			/* fcn call stack frame 12-90:
			evSp --> return value		 at PSRETA -- must be moved
				 ... 			 stuff here unexpected at
				 ...			    return but handle it
			  	evFp --> saved evFp (frame ptr)  2 words
			  		 ret addr		 2 words
			  		 arg values	pushed by caller, discarded by PSRETA
			*/
			// control flow: absolute pseudo-code call/return for user fcns 12-90
			// related ops: PSRLOD2,4,PSRSTO2,4 above for args/retval.
		case PSCALA:			// absolute call to addr in next 4 bytes
			tIp = *IPP++;  // addr from next 4 bytes
			*--SPP = evIp;		// push return address
			evIp = tIp;		// new ip (eval address)
			*--SPP = evFp;		// push frame ptr
			evFp = evSp;		// new frame pointer
			break;

		case PSRETA:		// return from pseudo-code called with PSCALA
			//  following in code: # arg words to discard,
			//   # return value words to move & keep.
			tSp = evFp;			// temp SP to frame
			evFp = *((void **)tSp);  	// pop frame ptr
			IncP( DMPP( tSp), sizeof( void *));
			tIp =  *((void **)tSp);  	// pop ip to temp
			IncP( DMPP( tSp), sizeof( void *));
			IncP( DMPP( tSp), sizeof( SI) * *IPI++);	// discard args
			// move return value (push at tSp).  May overlap:
			for (siTem = *IPI; --siTem >= 0; )
			{	IncP( DMPP( tSp), -SI( sizeof( SI)));
				*(SI *)tSp = *(SPI + siTem);
			}
			evSp = tSp;			// store new stack ptr
			evIp = tIp;			// new ip (eval ptr)
			break;				// ret val now on stack top

			// failure ops: issue err Msgs.  expect to add file/line info args.
		case PSCHUFAI:		// issue fatal "index out of range" error.
			// uses idx, lo, hi, set in PSDISP_ case.
			// default default for PSDISP.
			ms = strtprintf( (char *)MH_R0215, 		// "In choose() or similar fcn, \n"
			(INT)idx, (INT)lo, (INT)hi );	//   "    dispatch index %d is not in range %d to %d \n"
			//   "    and no default given."
			goto breakbreak;

		case PSSELFAI: 	// issue "no true condition" message: default default for select()
			ms = (char *)MH_R0216;			/* "in select() or similar function, \n"
					        		   "    all conditions false and no default given." */
			goto breakbreak;

			//--- prints (development aids)
		case PSIPRN:
			printf( "% d", *SPI++);
			break;
		case PSFPRN:
			printf( "% f", *SPF++);
			break;
		case PSSPRN:
			printf( "%s", *SPCC++);
			break;

#ifdef PROBE	// probe-related features that depend especially on being linked in CSE environmemt

			//--- record data access operations, 12-91. move up at reorder?
		case PSRATRN:			// point record by number: ancN inline, rec number on stack, leaves record address on stack
			b = basAnc::anc4n( *IPU++, PABT);   	// get inline anchor number, point basAnc (ancpak.cpp)
			i = *SPI++;					// get record number from stack
			if (i < b->mn || i > b->n)
			{
				ms = strtprintf( (char *)MH_R0217, 			// "%s subscript %d out of range %d to %d"
						(char *)b->what, (INT)i, (INT)b->mn, (INT)b->n);
				goto breakbreak;
			}
			*--SPP = &b->rec(i);			// push pointer to record i
			break;

		case PSRATROS:
		  {	 // rat record by default owner and name: ratN inline, default owner subscript inline,
			 // string on stack, leaves record address on stack
			b = basAnc::anc4n( *IPU++, PABT);	// get inline anchor number, point basAnc (ancrec.cpp)
			defO = *IPI++;			// get 0 or default owner subscript for ambiquity resolution
			p = *SPP;				// get ptr to name string from stack. no pop: will overwrite.
			const char* pName = strTrim( (char *)p);	// trim in place (added 7-3-2013)
			if (defO				// if defO given (owning input record subscript)
			 && b->ownB )    		// if owning-basAnc pointer set in probed basAnc (should be set in run rat
			        			   //    only if subscripts in owning run rat match input subscripts)
				trc = b->findRecByNmDefO( pName, defO, SPPR, NULL);	// seek rcd by name & defO, repl ptr in stk.
			else
				trc = b->findRecByNmU( pName, NULL, SPPR);	// seek unique record by name, replace ptr in stk.
			if (trc)
			{
				ms = strtprintf(
						trc==RCBAD2
							? (char *)MH_R0218	// "%s name '%s' is ambiguous: 2 or more records found.\n"
												// "    Change to unique names."
							: (char *)MH_R0219, // "%s record '%s' not found."
						(char *)b->what, pName );
				goto breakbreak;
			}
			cupfree( DMPP( p));			// free dm for consumed string, if not ptr into pseudoCode
		  }
		  break;

			//--- record data loads: load datum of indicated type to 2 or 4 bytes, making private dm copy of strings.
			// these take rec ptr in stack (from PSRATRN/S), field number inline.
			// macro gets ptr from stk, adds member offset for inline field number
#define POINT  e = (record*)*SPP++;  v = (char *)e + e->b->fir[ *IPU++ ].off
#ifdef wanted				// not wanted 12-91: there are no UCH or CH values in records.
w	 case PSRATLOD1U: POINT; *--SPU = (USI)*(UCH*)v; break;	// 1 unsigned byte
w	 case PSRATLOD1S: POINT; *--SPI = (SI)*(CH*)v; break; 	// 1 byte, extend sign
#endif
		case PSRATLOD2:
			if ((rc = cuRm2Get(&i,&ms,pBadH)) != RCOK) 	// 2 bytes: SI [or USI].  1st check/fetch 2 bytes
				goto breakbreak;				// if unset/uneval'd
			*--SPI = i;					// ok, push value fetched
			break;
			/*lint -e70 unable to convert */
		case PSRATLODD:
			POINT;
			*--SPF = (float)*(double *)v;		// double: convert to float.
			break;  /*lint +e70 */				//   expr not allowed; add UNSET ck if need found.
		case PSRATLODL:
			POINT;
			*--SPF = (float)*(LI *)v;			// long: convert to float.
			break;  /*lint +e70 */				//   add UNSET ck if need found.
		case PSRATLODA:
			POINT;
			*--SPP = strsave((char *)v);
			break;  	// char[], eg ANAME: put in dm. NAN not expected.
		case PSRATLODS:
			if ((rc = cuRmGet(&p,&ms,pBadH)) != RCOK)		// char *.  1st fetch/check/fix 4 bytes.
				goto breakbreak;				//   if unset data or uneval'd expr, ms set.
			if (p)
				cupIncRef( DMPP( p));				//   ++ ref count of ptr if not into code (below)
			else
				p = strsave("");				//   supply dm "" for NULL pointer
			*--SPP = p;					//   stack pointer
			break;
		case PSRATLOD4:
			if ((rc = cuRmGet(&p,&ms,pBadH)) != RCOK)	// 4 bytes: float/LI/ULI.  1st fetch/check/fix 4 bytes.
				goto breakbreak;			//   if unset data or uneval'd expr, ms set.
			*--SPP = p;				//   push value fetched
			break;

			//--- expression data loads: used when (immediate input) locn already containing expr is probed. 12-91.
		case PSEXPLOD4:     			// 4-byte load (LI, float)
			h = *IPU++;			// fetch inline expression # (handle)
			if (exInfo( h, NULL, NULL, &p))	 	// get expression value / if bad expr #
				goto badExprH;			// issue "bad expr #" msg (h)
			if (ISNANDLE(p))				// if VALUE of expr is UNSET (other NAN not expected in expr tbl)
				/* DON'T evaluate it now from here without a way to reorder later evaluations,
				   because stale values would be used on iterations after the first (unless constant). **** */
				goto unsExprH; 			// issue errmsg for unset expr (h), below
			*--SPP = p;				// ok, stack value
			break;
		case PSEXPLODS:			// string (incRef it)
			h = *IPU++;			// fetch inline expression # (handle)
			if (exInfo( h, NULL, NULL, &p))	 	// get expression value / if bad expr #
			{
badExprH:
				ms = strtprintf( (char *)MH_R0220, (UI)h);	// "cuEvalI internal error: bad expression number 0x%x"
				goto breakbreak;
			}
			if (ISNANDLE(p))				// if VALUE of expr is UNSET (other NAN not expected in expr tbl)
			{
unsExprH:
				ms = strtprintf( (char *)MH_R0230,	// "%s has not been evaluated yet." 11-92
					whatEx(h) );    				// describes expr origin, exman.cpp
				rc = RCUNSET;    					// specific "uneval'd expr" error code, callers test.
				if (pBadH)							// if info return ptr given
					*pBadH = h;						// return uneval'd expr # (0 if UNSET)
				goto breakbreak;
			}
			*--SPP = p ? p : strsave("");			//   stack pointer, supplying dm "" for NULL ptr
			dmIncRef( DMPP( p));					//   ++ ref count of copied ptr, nop if NULL
			break;
#endif	// PROBE

#ifdef IMPORT	// Import-file related features that depend on being linked in CSE environment

			//--- fetch data from fields of current record of import files. Records are read periodically elsewhere. 1-94.
		case PSIMPLODNNR: 		// load number from field specified by field number (column number)
			n = *IPI++;			// fetch inline file index (ImpfiB subscript)
			i = *IPI++;			// fetch inline 1-based field number
			fileIx = *IPU++;  	// fetch inline CSE input fileName index for error msgs
			line = *IPI++;		// fetch inline line number
			rc = impFldNrN( n, i, &fv, fileIx, line, &ms); 	// import number by field #, app\impf.cpp
			if (rc != RCOK)
				goto breakbreak;		// on error, ms is set to sub-message ptr.
			*--SPF = fv;				// store returned float
			break;
		case PSIMPLODSNR: 		// load string from field specified by field number (column number)
			n = *IPI++;			// fetch inline file index (ImpfiB subscript)
			i = *IPI++;			// fetch inline 1-based field number
			fileIx = *IPU++;  		// fetch inline CSE input fileName index for error msgs
			line = *IPI++;					// fetch inline line number
			rc = impFldNrS( n, i, (char **)&p, fileIx, line, &ms);	// import string by field #, app\impf.cpp
			if (rc != RCOK)
				goto breakbreak;			// on error, ms is set to sub-message ptr.
			*--SPP = p;				// store returned heap pointer
			break;
		case PSIMPLODNNM:		// load number from named field
			n = *IPI++;			// fetch inline file index (ImpfiB subscript)
			i = *IPI++;			// fetch inline 1-based field name index (a table subscript)
			fileIx = *IPU++;  		// fetch inline CSE input fileName index for error msgs
			line = *IPI++;				// fetch inline line number
			rc = impFldNmN( n, i, &fv, fileIx, line, &ms);	// import number from named field, app\impf.cpp
			if (rc != RCOK)
				goto breakbreak;				// on error, ms is set to sub-message ptr.
			*--SPF = fv;				// store returned float
			break;
		case PSIMPLODSNM:		// load string from named field
			n = *IPI++;			// fetch inline file index (ImpfiB subscript)
			i = *IPI++;			// fetch inline 1-based field name index (a table subscript)
			fileIx = *IPU++;  		// fetch inline CSE input fileName index for error msgs
			line = *IPI++;					// fetch inline line number
			rc = impFldNmS( n, i, (char **)&p, fileIx, line, &ms);	// import string from named field, app\impf.cpp
			if (rc != RCOK)
				goto breakbreak;			// on error, ms is set to sub-message ptr.
			*--SPP = p;				// store returned heap pointer
			break;
#endif	// IMPORT

		case PSCONTIN:			// continuous lighting controller with minimum power @ minimum light.
								// does not go off. returns float fraction power requred on stack.
		{
			float il = *SPF++;	// arguments on stack, in reverse order, are float illuminance,
			float sp = *SPF++;	//  float setpoint (same units as illuminance),
			float mlf = *SPF++;	//  float minimum lighting fraction (never goes lower),
			float mpf = *SPF;	//  float minumim power fraction to use at min lighting fraction. NB not popped.
			float fp = mpf;		// result: init to minimum power.
			if (sp > 0.f)		// 0 setpoint requires no lighting, return mpf
			{
				if (il <= 0.f)			// no illumination
					fp = 1.f;			// requires full power
				else
				{
					float fl = (sp - il)/sp;	// fraction of lighting required
					if (fl > mlf)			// if > minimum lighting (mpf pre-stored)
						fp += /* mpf + */	// compute fraction power needed in addition to mpf
						(fl - mlf) 			// linear from (mlf,mpf) to (1,1)
						* (1.f - mpf)/(1.f - mlf);	// <-- this part normally constant, could pre-compute
				}
			}
			*SPF = fp; 		// store result (fraction lighting power), overwriting last (unpopped) arg
		}
		break;

		case PSSTEPPED:		// stepped lighting controller: n equal steps. result: float fraction lighting power.
		{
			float il = *SPF++;	// arguments on stack, in reverse order, are float illuminance,
			float sp = *SPF++;  //  float setpoint (same units as illuminance),
			SI ns = *SPI++;		//  integer number of steps.
			float fp = 0.f;		// for result: fraction power
			if (sp > 0.f)  				// if setpoint <= 0 illuminance, no lighting required
			{
				if (il <= 0)        fp = 1.0f;		// no illum: full power
				else if (il >= sp)  fp = 0.f;		// full illum: no power
				else    fp = ceil( ns*(sp - il)/sp) / ns;	// next step >= fraction of lighting power needed
			}
			*--SPF = fp;		// push result: fraction of lighting power required
		}
		break;

		case PSFILEINFO:
		{	// 0=not found, 1=empty file, 2=non-empty file, 3=dir, -1=err
			SI fiRet = xfExist( (const char*)*SPP++);
			*--SPI = fiRet;
		}
		break;

		default:
			ms = strtprintf( (char *)MH_R0221,	// "cuEvalI internal error:\n    Bad pseudo opcode 0x%x at psip=%p"
				(UI)op, IPOP-1 );
		case PSEND: 			// normal terminator
			goto breakbreak;
		}	// switch (op)
	}	// for ( ; ; )
breakbreak:
	;

	if (pmsg)			// if msg return requested
	{
		if (msgIsHan(ms))	// if ms is an unretrieved message HANDLE (excludes NULL) (lib\messages.cpp)
			ms = strtprintf(ms);	// retrieve message text and put in Tmpstr, to get char * ptr to return to caller
		*pmsg = ms;		// return NULL or ptr to Tmpstr msg text. TRANSITORY!
	}
	else			// pmsg==NULL: issue any msg here
		if (ms)			// if there is a messsage
			err( WRN, ms);	// display error msg, wait for key, rmkerr.cpp
	if (ms && !rc)		// if no specific error return code already set
		rc = RCBAD;		// supply generic error code
	return rc;			// RCOK if ok
}		// cuEvalI

#ifdef PROBE	// features that depend especially on being linked in CSE environmemt
//============================================================================
// is it time to put cuEvalI ms and pBadH in file-globals?
LOCAL RC FC NEAR cuRmGet( void **pv, const char** pms, USI *pBadH)

// access 4-byte record member, for cuEvalI, with unset check and expr fix.

// pops record pointer from eval stack (evSp); fetches inline field number from instruction stream (evIp).

// Expression reference (expected if input record probed) is replaced with current 4-byte value from expression table if set.

// if ok, returns RCOK with *pv set to the non-NAN 4-byte value from the record or the expression table.
// returns non-RCOK on error, with *pms set to unissued error message text, in TmpStr.
//         return value is RCUNSET if un-evaluated expression is referenced, with its expr # or 0 in *pBadH.
{
	record *e = (record *)*SPP++;		// get record ptr from eval stack.  e->b is its basAnc.
	SFIR *fir = e->b->fir + *IPU++;	// get inline field ptr, point to fields-in-record entry for field
	//USI off = fir->off;			how to get member offset from fields-in-record table
	//USI evf = fir.evf;			how to get field variation (evaluation frequency) bits

// check for mistimed probe eg to monthly results at end of day (when monthly results contain incomplete data)

	if (fir->evf & EVXBEGIVL)	// if probed field avail only at end ivl (can probe start-ivl flds anytime)
	{	if (!Top.isEndOf)    // if not now end of an interval (cnguts.cpp)
		{
			*pms = strtprintf( (char *)MH_R0222,	// "Internal Error: mistimed probe: %s varies at end of %s\n"
													// "    but access occurred %s"
						whatNio( e->b->ancN, e->ss, fir->off),
						evfTx( fir->evf, 2),
						Top.isBegOf
							? strtprintf( (char *)MH_R0223,		// "at BEGINNING of %s"
								ivlTx(Top.isBegOf) )  			// ivlTx: below
							: strtprintf( (char *)MH_R0224) );	// use strtprintf to retreive msg to Tmpstr
																//"neither at beginning nor end of an interval????"
			return RCBAD;
		}

		IVLCH minIvl =
			fir->evf >= EVFSUBHR ? C_IVLCH_S	// get ivl corresponding to leftmost evf bit
		  : fir->evf >= EVFHR    ? C_IVLCH_H	//   (shortest interval at which ok to probe this field)
		  : fir->evf >= EVFDAY   ? C_IVLCH_D
	 	  : fir->evf >= EVFMON   ? C_IVLCH_M
		  :                        C_IVLCH_Y;	// EVFRUN, EVFFAZ, EVFEOI

		if (Top.isEndOf > minIvl)		// if end of too short an interval (C_IVLCH_ increases for shorter times)
		{
			*pms = strtprintf( (char *)MH_R0225,	/* "Mistimed probe: %s\n"
														"    varies at end of %s but accessed at end of %s.\n"
														"    Possibly you combined it in an expression with a faster-varying datum." */
						whatNio( e->b->ancN, e->ss, fir->off),
						evfTx( fir->evf, 2),
						ivlTx(Top.isEndOf) );
			return RCBAD;
		}
	}

// fetch and check 4-byte quantity

	void* v = *(void **)((char *)e + fir->off); 	// fetch 4-byte value from record at offset
	if (ISNANDLE(v))
	{
		if (ISUNSET(v))
		{
			*pms = strtprintf( (char *)MH_R0226,		// "Internal error: Unset data for %s"
			whatNio( e->b->ancN, e->ss, fir->off) );   	// describe probed mbr. exman.cpp
			return RCBAD;
		}
#if defined( AUTOSIZE) // found "ifdef AUTOSIZING" ?? 4-16-10
		else if (ISASING(v))
		{
			*pms = strtprintf( (char *)MH_R0232,		// "%s probed while being autosized"
			whatNio( e->b->ancN, e->ss, fir->off) );   	// describe probed mbr. exman.cpp
			return RCBAD;
		}
#endif
		else					// other nandles are expression handles
		{
			USI h = EXN(v);						// extract expression number, exman.h macro.
			if (exInfo( h, NULL, NULL, &v))				// get value / if not valid expr #
			{
				*pms = strtprintf( (char *)MH_R0227,			// "Internal error: bad expression number %d found in %s"
				(INT)h, whatNio( e->b->ancN, e->ss, fir->off) );
				return RCBAD;
			}
			if (ISNANDLE(v))					// if expr table contains nan, expr is not eval'd yet.
			{
				/* DON'T evaluate it now from here without a way to reorder later evaluations,
						because stale values would be used on iterations after the first (unless constant). **** */

				*pms = strtprintf( (char *)MH_R0228,		// "%s has not been evaluated yet."  Also used below.
				whatEx(h) );  			/* whatEx: describes expression origin per exTab, exman.cpp.
                              					   whatNio produces similar text in cases tried but
                              					   suspect whatEx may be better in obscure cases. */
				if (pBadH)						// if caller gave info return pointer
					*pBadH = h;					// return expression number of uneval'd expr (0 if UNSET)
				return RCUNSET;					// specific "uneval'd expr" error code, callers may test.
			}
		}
	}
	*pv = v;
	return RCOK;
}			// cuRmGet
#endif

#ifdef PROBE	// features that depend especially on being linked in CSE environmemt
//============================================================================
// is it time to put cuEvalI ms and pBadH in file-globals?
LOCAL RC FC NEAR cuRm2Get( SI *pi, const char** pms, USI *pBadH)

// access 2-byte record member, for cuEvalI, with partial unset check

// pops record pointer from eval stack (evSp); fetches inline field number from instruction stream (evIp).

// if ok, returns RCOK with *pv set to the non-NAN 4-byte value from the record or the expression table.
// returns non-RCOK on error, with *pms set to unissued error message text, in TmpStr.
//         return value is RCUNSET if un-evaluated expression is referenced, with 0 in *pBadH.
{
	record *e = (record *)*SPP++;		// get record ptr from eval stack.  e->b is its basAnc.
	USI fn  = *IPU++;  		// get inline field number from instruction stream
	SFIR *fir = e->b->fir + fn;			// get pointer to fields-in-record table for this field
	//USI off = fir->off;			how to get member offset from fields-in-record table
	//USI evf = fir.evf;  			how to get field variation (evaluation frequency) bits

// check 2-byte field -- too small to hold NANDLE, use field status

	UCH fs = *((UCH *)e + e->b->sOff + fn);
	if ((fs & (FsSET | FsVAL))==FsSET)  	/* if "set", but not "value stored", assume is input field with uneval'd
    						   expression (if neither flag on, assume is run field -- bits not set.) */
	{
		*pms = strtprintf( (char *)MH_R0228,				// "%s has not been evaluated yet."  Also used above.
					whatNio( e->b->ancN, e->ss, fir->off) );	// describe probed mbr. exman.cpp
		if (pBadH)
			*pBadH = 0;				// don't know handle of uneval'd expr for this variable (could search exTab)?
		return RCUNSET;				// say unset data -- try rearranging exprs
	}

// fetch 2-byte quantity
	*pi = *(SI *)((char *)e + fir->off); 		// fetch 2-byte value from record at offset
	return RCOK;
}			// cuRm2Get
#endif

#ifdef PROBE	// features that depend especially on being linked in CSE environmemt
//============================================================================
LOCAL const char* FC NEAR ivlTx( IVLCH ivl)	// text for interval.  general use function?
{
	switch (ivl)
	{
	case C_IVLCH_Y:
		return "run";
	case C_IVLCH_M:
		return "month";
	case C_IVLCH_D:
		return "day";
	case C_IVLCH_H:
		return "hour";
	case C_IVLCH_S:
		return "subhour";
	default:
		return strtprintf( (char *)MH_R0229, (INT)ivl);	// "Bad IVLCH value %d"
	}
}		// ivlTx
#endif

//============================================================================
static inline bool IsDM( DMP p)		// return nz iff string block "looks like" it is on heap
// Constants in pseudo-code (see PSPKONN) are preceded by
// 16 bit ONES COMPLEMENT of length in words, recognized here by TWO
// hi order 1 bits (change to 1 bit to support strings > 16K).

// Strings in dm blocks are preceded by the 32 bit (positive) block size
// in bytes, which will have 0's in hi bits of lo 16 bits for reasonable string lengths
//  (see dmpak.cpp; IMPLEMENTATION DEPENDENT).
{
	return (*((USI*)(p)-1) & 0xc000) != 0xc000;
}		// IsDM
//----------------------------------------------------------------------------
RC FC cupfree( 		// free a dm string without disturbing a NANDLE or string constant in code
	DMP* pp ) 		// reference to ptr to string; ptr is NULL'd if not NANDLE
// returns RCOK iff success
{
	DMP p = *pp;
	if (p != NULL			// nop if NULL ptr: no such string in use (not alloc'd)
	 && !ISNANDLE(p))		// nop if unset or expression handle
	{
#if defined( BUG_COLTYPECOPY)
		if (strMatch( (char *)p, "Houx"))
			printf( "COLTYPE cupfree\n");
#endif
		if (IsDM( p))
			dmfree( pp);// free memory and NULL ptr
		else
			*pp = NULL;	// probably a PSPKONN constant in code; don't free
						// but do NULL pointer in all cases
	}
	return RCOK;
}			// cupfree
//============================================================================
RC FC cupIncRef( DMP* pp, int erOp/*=ABT*/)	// if dm pointer, duplicate block or ++ref count after pointer copied;
// do not disturb if NANDLE or pointer to constant in pseudo-code.
// nop if p is NULL.
// note: to decrement reference count, use cupfree.
// returns RCOK iff success
{
	DMP p = *pp;
	if (p != NULL			// nop if NULL ptr: no such string in use (not alloc'd)
	 && !ISNANDLE(p))		// nop if unset or expression handle (cnglob.h macro for exman.cpp feature)
	{
#if defined( BUG_COLTYPECOPY)
		if (strMatch( (char *)p, "Houx"))
			printf( "COLTYPE cupIncRef\n");
#endif
		if (IsDM( p))
			dmIncRef( pp, erOp);      			// increment reference count or duplicate block, as implemented. dmpak.cpp.
		// else: probably a PSPKONN constant in code; don't dmIncRef
	}
	return RCOK;					// like dmIncRef()
}			// cupIncRef
//============================================================================
char * FC cuStrsaveIf( char *s)		// save a copy of string in dm if string is now inline in pseudo-code

// use when desired to retain string result of cuEval() but delete the pseudo-code that produced it
{
	if (IsDM( s))
		return s;					// not in PSPKONN, ok as is
	else
		return strsave(s);			// make a copy and return its location
}			// cuStrsaveIf

//============================================================================
int CDEC printif( int flag, const char* fmt, ... )	// printf if flag nz
{
	if (flag)
	{	va_list ap;
		va_start( ap, fmt);
		return vprintf( fmt, ap);
	}
	return 0;
}			// printif


// end of cueval.cpp
