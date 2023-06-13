// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cul.cpp -- Cal nonres User Language -- table driven input compiler */

// 7-19-92 some rat-related comments probably remain to be updated
// 9-92 cul.bk1 is prior version, before removal of many if 0's and backtracks.

/* document 2-91:
	UNSET member;		makes as tho omitted
				(restores internal default value even when
				not inputtable due to limits)
*/

/* 2-2-91+ could-do's list
    redo dref table using field # not offset?
    redo exman.cpp:extab WHERE using field # not offset?
    FDNONE in srd.h or cnglob.h?
    field constants or way to access fdTy eg for ANAME for ganame()?
*/

/* hanging 12-25-90
   expressions in Top -- handle when variability permitted there.
   1. must be registered where copied to
      b. copy with rcfmanip etc or similar new fcn, adding register option */

/* notes/problems in initial use 11-90
1. file names must be quoted and have double \'s.
   See if can change "text" to delete fewer \'s;
   Add a data type and/or a token type for unquoted file name? */

/* doing 1-91, be sure doct'n updated:
    removing , as alternative to ; between statements DONE
    removing := as alternative to =  DONE
*/

/*-------------------------------- CAUTIONS -------------------------------*/

/* CULT.id is, or may be, char *:
   1) cast (char *) when used in variable arg list
   2) don't compare to NULL (compiler supplies ds): use 0 or !. */

#include "cnglob.h"	// USI SI LI

/*-------------------------------- OPTIONS --------------------------------*/

// more local options below

/*------------------------------- INCLUDES --------------------------------*/

#include "srd.h"		// SFIR; sFdtab; GetDttab
#define NEEDLIKECTOR	// say this .cpp file needs anc<T> like-another constructor: prevents generation for classes where not used.
#include "ancrec.h"	// record: base class for rccn.h classes; anc<T> template definition.
#undef NEEDLIKECTOR
#include "rccn.h"		// SFI_  [VALNDT_]
#include "msghans.h"	// MH_S0201 MH_S0202

#include "cvpak.h"	// cvExtoIn
#include "rmkerr.h"	// errCount clearErrCount incrErrCout

#include "sytb.h"	// DUPOK
#include "pp.h"
#include "cutok.h"	// CUTVRB CUTID CUTSEM
#include "cuevf.h"	// evf's and variabilities: EVEOI
#include "cuparsex.h"	// tokTy ttTx maxErrors
#include "cuparsei.h"	// PREOF PRVRB PRSEM added 1-20-91
#include "cuparse.h"	// TYSTR TYSI TYLLI perNx
#include "exman.h"	// exEvEvf extDel extAdj
#include "cul.h"	// CULT RATE KDAT TYIREF TYREF RQD; decls for cul.cpp

/*----------------------------- conteXt stack -------------------------------

  An entry is added to this stack by each RATE CULT entry;
  one is popped by the ENDER-word in the current entry's table. */

struct XSTK
{
	CULT *cult;	// pointer to start of current table
	CULT *c;   	// ptr to entry in table for curr cmd (needed on pop)
	SI cs;		// case: 3 filling non-type (basAnc record (entry)), 4 defining a type: filling record for DEFTYPE.
	UCH fdTy;	// target field type, for access to dtype, units, limit type.
	UCH filla;	// alignment filler / available / future flags?
	BP b;		// ptr to basAnc (formerly rat) base (else 0: still possible 2-91??)
	TI i;		// basAnc record subscript if b != 0
	record *e;	// record (basAnc entry) base location: .b->p + .i
	void *p0;	// current datum location, of [0] if an array:. .e + member offset.
	USI j;		// 0-based array subscript, set 0 by datPt, incr'd by caller
	USI arSz;	// # elements in array of DAT, usually 1
	void *p;	// array element locn: set to p0 by datPt, incr'd (p0 + sz*j) by caller.
	USI sz;		// current datum size, from ty
	UCH *fs0;	// field status bytes base: ptr to sstat[0] in record.
	UCH *fs;	// curr fld (ARRAY elt [0]) stat byte locn: ptr into record sstat[].
	// CAUTION: for most Fs bits must use  fs[j].
};	// struct XSTK

/*-------------- Deferred-resolution basAnc record REFerences ----------------

  This table holds info until RUN, so referencee may be defined after reference,
  and reference is validated at each of multiple RUNs. */

struct DREF
{
	BP b;	 	// ptr to base of basAnc record making reference (cannot use ancN as type anchors are unregistered)
	TI i;		// subscript ref'ing record in ditto
	SI fn;		// referencing field number in ditto
	BP toB;		// ptr to base of basAnc record being referenced
	const char* toName;  	// name of basAnc record (entry) being referenced
	TI defO;		// default owner of referee (subscr in toB->ownB), eg zone TI for surface, to resolve ambiguities.
	int fileIx;		// for err msgs: input file name index of referencing stmt
	int line;		// .. line number ..
};
// extern USI nDref;	// number of dref entries in use		when needed
// extern DREF * dref;	// info on deferred references, array in heap

/*----------------------- re user-fcn caller cuf() ------------------------*/

// for overview of user function calling see comments in cul.h.

// type for cuf arg 1: which fcn to call
enum WHICHFCN
{	whichFcnNul,
	ITF,   	// initialization-time fcn (can set default value)(in .p2 OR in .dfpi of non-data cult entries)
	PRF,  	// pre-input fcn: called when cmd id seen, b4 input decoding.
	CKF   	// check/finish fcn (or main STAR reentry PRF)
};

// type for ucs (cuf arg 2). culPhase is set to CHECK_ except for RGLR.
enum CUFCASE 				// CULT for fcn,    p     p2 (arg 3)
{	cfcnul,			  	//  fcn arg 1	 (arg 2)   NULL or:
	RGLR,   // regular: DAT,KDAT:                xSp->c    xSp->p    xSp->e
	NONDAT, // non-data: NODAT,ENDER,RUN,CLEAR:  xSp->c    xSp->e  (xSp-1)->e
	STARS,  // star entry for RATE,MAIN:       xSp->cult   xSp->e  (xSp-1)->e
	CLRS    // calling (nesting) entry RATE:  (xSp-1)->c   xSp->e  (xSp-1)->e
};			      // p3 (fcn arg 3) is (xSp- one more )->e.


/*-------------------------------- OPTIONS --------------------------------*/

/* define to restore code to default RATEs using data at
   *CULT.dfpi and size CULT.ty, including via STAR entry, 1-11-91. */
#undef DEFAG   		// saves little. put back?

// define for CULT float default in same member as int/ptr default: merge .dfpi and .dff to form .dfl.
#undef DF1		// believe works here; cannot find way init member with float value, so reverted to old 1-92.


/*-------------------------------- DEFINES --------------------------------*/

#define CULTP2  p2		// CULT cult ptr mbr is now .p2 (formerly .cult). This define preserves backtrack 1-92.

#define IFLAGS  f		// former CULT .iFlags bits now in member .f, 1-92

#define EUCL    uc		// expr use class bits, formerly in .spare1 1-92, then in lo byte of f2, now in .uc 1-92

#ifdef DF1			// if CULT float and int/ptr members are combined in .dfl
o  #define DFF  dfl
o  #define DFPI dfl
#else
#define DFF  dff
#define DFPI dfpi
#endif

// return codes
//  RCOK (0), RCFATAL (-1), and RCBAD (-2) defined in cnglob.h
#define RCSKIP2END  16385  	/* skip-to-matching-ENDer return code from culRATE etc to culDo, culComp.
NB return this BEFORE making xStk entry for command that fails. */
#define RC_SUM      16386	// "sum" was input in place of name (and enabled). for xpr, ganame,
#define RC_ALL      16387	// "all" was input in place of name (and enabled). for xpr, ganame,
#define RC_ALLBUT   16388	// "all_but" was input in place of name (and enabled). for xpr, ganame,

//  restore when used:
//#define Z(f) { rv=(f); if (rv) return rv; }	// call f; if non-0, return it


// call fcn; return RCFATAL errors, only, to caller now
#define F(f) { if ((f)==RCFATAL) return RCFATAL; }

// call fcn; return RCFATAL to caller now; skip to END on other errors
#define FS(f) { rc = (f); if (rc !=RCOK) return rc==RCFATAL ? rc : RCSKIP2END; }

// ditto, popping context stack a level on RCSKIP2END return
#define FSP(f) { rc = (f); if (rc != RCOK) { if (rc==RCFATAL) return rc; \
                  			     xSp--;  return RCSKIP2END; } }
/* note FSP neglects to clear 'defTyping' if xSp->cs==4.
	Believe that skip2end() cleans defTyping up ok. 1-91. */

/*--------------------------- PUBLIC VARIABLES ----------------------------*/

// for testing by user .itf, .prf, .ckf fcns as well as internal use:
SI defTyping = 0;	/* non-0 if defining a "type"
		   			      (a type is a RATE while setting members)
			               set: culDEFTY. cleared: culENDER.
			               remains on thru nested culDATs, etc;
			               also xSp->cs is 4 not 3 at DEFTYPE level. */


/*---------------------------- LOCAL VARIABLES ----------------------------*/

// CONTEXT STACK: each RATE CULT entry adds an entry; each (usually implicit) END pops an entry.
LOCAL XSTK xStk[10+1];  					// context stack. last entry not used.
// ^^^ test msgs then increase size to ? 20.  msg seen ok @3, 4.  6-->10 3-92.
LOCAL XSTK* xSp = xStk;  				// context stack pointer

// Deferred-resolution basAnc REFerences: table holds info til RUN, to permit forward references, and to validate refs at each RUN.
LOCAL USI drefNal = 0;  		// allocated size of table, 0 = not allocated
LOCAL USI nDref = 0;  			// number of dref entries in use (consecutive from 0)
LOCAL DREF * dref = NULL;		// pointer to dref (deferred references info) table in heap

// Phase of session: INPUT_PHASE, CHECK_, RUN_ .  Controls filename/line shown in oerI() error messages.
LOCAL CULPHASE culPhase = INPUT_PHASE;		// typedef: cul.h.   make public if needed, in eg cncult.cpp.

LOCAL BOO * pAuszF = NULL;		// NULL pointer to cul caller's autosize flag, set in culAUTOSZ.

LOCAL SI culNRuns = 0;		// counts valid RUN commands re culRun errmsg
LOCAL SI culNClears = 0;		// counts valid Clear commands re errmsgs

//--- variables that would be statics inside fcns except we need to clean them.
LOCAL BOO addVrbsDone = 0; 	// non-0 when CULT verbs have been added to symbol table (don't repeat, for speed). 10-93.
LOCAL SI addVrbsLvl = 0;		// addVrbs nesting level

// options for nxRat()
const int nxropIGNORE_BEEN_HERE = 1;	// ignore BEEN_HERE flags, allows handling multiple sub-RATEs with same CULT table
										//   example: DHWHEATER and DHWLOOPHEATER both owned by DHWSYS

/*---------------------------- INITIALIZED DATA ---------------------------*/

// always-defined verbs.  Only .id and .cs used.
CULT stdVrbs[] =
{
	// id     cs    fn       f     uc evf  ty      rb   dfls      p2     ckf
	//------  ----  -------  ----  -- ---  ------  --   ------    -----  -----
#ifdef DF1
o{ "end",     END,  0,       0,    0, 0,   0,      0,   0,        NULL,  NULL },
#else
CULT( "end",  END,  0,       0,    0, 0,   0,      0,   0, 0.f,   NULL,  NULL ),
#endif
CULT()
};	// stdVrbs

/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL SI FC culComp( void);
LOCAL SI FC culDo( SI cs);
LOCAL SI FC culRun( void);
LOCAL SI FC fret( RC rc);
LOCAL RC FC culEndTo( XSTK* x);
LOCAL RC FC culENDER( SI cs);
LOCAL RC FC culDELETE( void);
LOCAL RC FC culRATE( SI defTy, SI alter);
LOCAL RC FC nuCult( SI clr, SI like);
LOCAL RC FC clearRat( CULT *c);
LOCAL RC FC xCult( void);
LOCAL RC FC culCLEAR( void);
LOCAL RC FC culRqFrz( UCH flag);
LOCAL RC FC culAUTOSZ( void);
LOCAL RC FC culRESET( void);
LOCAL RC FC culNODAT( void);
LOCAL RC FC culKDAT( void);
LOCAL RC culDAT( void);
LOCAL RC FC datPt( void);
LOCAL RC FC ckiCults( void);
LOCAL void FC finalClear( void);
LOCAL void FC xStkPt( void);
LOCAL RC ganame( USI f, char *what, char **pp);
LOCAL RC xpr( SI ty, USI fdTy, USI _evfOk, USI useCl, USI f, char *what, NANDAT *p, USI *pGotTy, USI *pGotEvf );
LOCAL void FC termnIf( void);
LOCAL RC CDEC perNxE( char *ms, ...);
LOCAL SI FC skip2end( SI lastCs, SI *pcs);
LOCAL RC bFind( SI *pcs, XSTK** px, CULT **pc, int erOp );
LOCAL SI cultFind( CULT *c, CULT **pc);
LOCAL SI deftyFind( CULT *c, CULT **pc);
LOCAL RC FC vFind( SI cs);
LOCAL SI csFind( CULT *c, SI cs, CULT **pc);
LOCAL SI FC tkIf( SI tokTyPar);
LOCAL SI FC tkIf2( SI tokTy1, SI tokTy2);
// LOCAL SI CDEC tkIfn( SI tokTy1, ... );  calls gone 1-20-91
LOCAL SI FC nxPrec( void);
//note: syntax error on 'f': wrong place to put arg name
//	LOCAL RC FC cif( RC (*)( void *, CULT *) f, void *, CULT *);
//note: syntax error on ',': name for one arg but not for all
//	LOCAL RC FC cif( RC (* f)( void *, CULT *), void *, CULT *);
//no names ok: LOCAL RC FC cif( RC (*)( void *, CULT *), void *, CULT *);
//all names ok:LOCAL RC FC cif( RC (*f)( void *p, CULT *c), void *p, CULT *c);
//ok, but use typedef: LOCAL RC FC cif( RC (*f)( void *, CULT *), void *p, CULT *c);
//ok, but now its obsolete: LOCAL RC FC cif( UFCNPTR f, void *p, CULT *c);
LOCAL RC FC cuf( WHICHFCN fcn, CUFCASE ucs, SI skie);
LOCAL RC FC msgMissReq( void);
LOCAL RC ratPutTy( record *e, CULT *c);
LOCAL RC rateDuper( record *newE, record *oldE, SI move, SI cn, TI own, CULT *c );
LOCAL void FC adjOwTi( BP ownB, CULT *c, TI minI, TI delta);
LOCAL RC FC ratTyR( BP b);
LOCAL SI nxOwRat( BP ownB, CULT *c, CULT **pc, BP *prTyIf, BP *pr);
LOCAL SI nxOwRec( TI ownTi, BP b, TI *pi, record **pe);
LOCAL SI FC xnxDatMbr( SI bads, void **pp);
LOCAL bool xnxC( CULT* &c, int options=0);
LOCAL SI FC nxRat( BP * pr, int options=0);
LOCAL SI FC nxRec( SI bads, void **pe);
LOCAL SI FC nxDatMbr( void **pp);
LOCAL bool nxC( CULT* &pp);
LOCAL RC ratLuCtx( BP b, char *name, char *what, record **pE);
LOCAL RC ratLuDefO( BP b, const char* name, TI defO, char *what, record **pE, const char** pMsg);
//public 1-92: LOCAL TI FC ratDefO( BP b);
LOCAL void FC ratCultO( void);

LOCAL void FC drefAdd( const char* toName);
LOCAL void FC drefAddI( BP b, TI i, SI fn, BP toB, const char* toName, TI defO);
LOCAL DREF* drefNextFree();
LOCAL void drefMove( record *nuE, record *e);
LOCAL void drefDup( record *nuE, record *e);
LOCAL void FC drefDel( record *e);
LOCAL void FC drefDelFn( BP b, TI i, SI fn);
LOCAL void FC drefAdj( BP b, TI minI, TI delta);
LOCAL void FC drefRes( void);
LOCAL void FC drefClr( const char* callTag);

LOCAL RC FC addVrbs( CULT *cult);
LOCAL void FC clrBeenHeres( CULT *c);

// main entry is cul(), next, below.
//===========================================================================
void FC culClean( 		// cul overall init/cleanup routine

	CLEANCASE cs )	// [STARTUP,] ENTRY, DONE, CRASH, or [CLOSEDOWN]. type in cnglob.h.

// see also: cncult2.cpp:cncultClean (cleans cncultn.cpp files)
{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.

	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */

	/* cleanup analysis/coding/calling done: cuparse,cutok,ppxxx.cpp. 10-93
	                                         exman,cncult,2,3,4,5,6   12-93
	                                         cueval,sytb - nothing found 12-93
	                                         beleive all  files now done. */

	/* notes re this file (cul.cpp) cleanup:
	   Neither of following repeated here cuz may be delicate & require much initialization:
	     Input basAnc records and types are cleared by nuCult(nz,0) in first call to cul() (next).
	     finalClear clears all input strings, and records in unprobed basAncs.
	   But ALL basAncs are cleared and .tyB's deleted by ancrec:clearBasAncs from cse.cpp.
	   Should review C++ destructors for all record types to make sure all strings free'd.
	   Search for strsave, dmal: none found. static: all ok or handled. dmral: all handled.
	   dmfree's: there are several dmfree's of temp results or prior values in this file which
	     might get bypassed upon longjmp on error in called fcn; leave them for now 10-11-93.
	   types sub-basAncs: now deleted in finalClear.
	  PENDING 10-11-93:
	    be sure all record d'tors free all dm strings.
	    pseudo-code: any dm pointers? PSKON4(string) used? clean in exman? PSKON4(string) not used, 12-93.
	*/

// cul.cpp cleanup

	addVrbsDone = FALSE;	// cuz code elsewhere will clear the symbol table
	addVrbsLvl = 0;		// not in a nested addVrbs call

	drefClr( "culClean");	// clear "deferred reference table". local.
	if (dref)				// if allocated
	{
		dmfree( DMPP( dref));	// free dref table storage
		drefNal = nDref = 0;
	}

// clean modules used by this module

	cuParseClean(cs);		// clean cuParse, which calls cuToke, which calls pp, which calls ppcmd,ppcex,pptok.

	exClean(cs);		// clean exman.cpp  Rob 12-3-93

}		// culClean

//===========================================================================
SI FC cul( 	// public entry to compile user language commands from open input file, using given table.

	SI cs,	// case: 0: opt'l prelim init, flag input basAncs, set input basAnc ownership, etc, eg re probe names rpt (CSE -p).
            //   1: first call (except poss cs 0 call), initialize, use cult.
  			//   2: recall (after execution after return 2), continue from current internal state & input file position.
	  		//   3: just close file (use after error elsewhere after 2 return).  Returns 1.
	  		//   4: clear input data, free input basAncs and DM alloc'd here:
	  		//      call for max DM for last run, or re finding unfree'd DM. Uses no other args. Returns 1.
	const char* fName,	// file name (case 1)
	char* defex,	// default extension (must include leading period)

	CULT *cult,  	// top-level table, used if cs==1
	record *e,   	// ptr to basAnc record (entry) to rcv top level data
	// (alternative to e: implement optional use of info in STAR entry (permitting statSetup here?)
	BOO *_pAuszF /*=NULL*/)	// if cs==1, NULL or ptr to flag set non-0 if input contains any AUTOSIZE commands. 6-95.
							// eg &Topi.bAutoSizeCmd in CSE.

/* caller must: open err file (rmkerr.cpp:errFileOpen()), if desired */

/* returns: 0: returned by cs 0 call only.
            1: terminate session (fatal error or too many errors)
            2: run, then re-call.  file still open and contains more commands;
               existing input may be reused if no CLEAR.
			3: run (execute), then terminate session. */
{
#define P ptr()
	SI rv;

// initialize for entry or reentry
#if 1	// bug fix, 3-2-2016
	if (cs != 4)
#endif
		culPhase = INPUT_PHASE;		// say inputting (show curr file/line in msgs)

#if 0
x // debug aid
x	trace = 1;
#endif

// initialize upon initial entry
	if (cs <= 1)
	{
		xSp = xStk;			// init context stack pointer
		culNRuns = 0;			// init RUN cmd counter
		culNClears = 0;			// init CLEAR cmd counter

		if (!addVrbsDone)		// don't repeat, for speed, 10-93
		{
			/* enter statement-starting keywords in global symbol table as "verbs".
			   Assists classification of words (ids) seen; improves compile error recovery when statement-terminating ';' missing. */
			if (addVrbs( cult) )   		// local fcn.  Does nested tables.
				return 1;   			// if error, consider it fatal
			// and the always-defined verbs such as "end"
			if (addVrbs( stdVrbs) )		// stdVrbs: data-init at start file
				return 1;
			addVrbsDone = TRUE;
		}

		// check arguments
		if (e==NULL)
		{
			err( PWRN, (char *)MH_S0204);     // "cul(): NULL RAT entry arg (e)"
			return 1;
		}
		if (e->b->validate("cul.cpp:cul",WRN))  return 1;

		// autosize flag pointer and flag
		pAuszF = _pAuszF;		// store argument pointer or NULL given by caller
		if (pAuszF)  *pAuszF = FALSE;	// say no AUTOSIZE commands yet seen

		// init top-level context stack frame
		memset( xSp, 0, sizeof(XSTK) );	// zero it: inits many members
		xSp->cult = cult;		// table specified by caller
		xSp->cs = 3;			// say xStk entry for basAnc Entry
		xSp->b = e->b;  			// basAnc location
		xSp->i = e->ss;  		// retrieve subscript of record
		xSp->e = e;			// record pointer
		xSp->fs0 = (UCH *)e + e->b->sOff;	// field status bytes (sstat[]) base

		// determine basAnc (formerly rat) "ownership" from table structure, set b->ownB's.
		/* ("ownership" used in resolving ambiguous references,
		   as eg surfaces with same name are allowed if in different zones) */
		ratCultO();			// local, below, uses xSp->b and ->cult.

		// check / initialize cult tables and check size of xStk
		if (ckiCults())			// uses xSp->cult and xSp->b
			return 1;			// consider returned error fatal

		// preliminary initialization (re -p) is complete here
		if (cs==0)			// if just setting up so can do probe names report (CSE -p) 1-92
			return 0;

		// init/clear/default top table and its members
		if (nuCult( 1, 0) != RCOK)	// uses xSp.  below.  calls STAR-entry initialization (ITF) fcn, if any.
			return 1;			// if error, consider it fatal
		//if (xCult() ) return 1;	nothing for it to do: no context yet.

#if 1	// reverse call order, 11-28-2012

		// call main table STAR-entry pre-input function, if any
		// do *before* file open so pre-input init is complete
		//   even if open fails
		if (cuf( PRF, STARS, 99) !=RCOK)	// call * entry (if any) fcn (if any)
			return 1;			// make any error from .prf fcn fatal

		// open input file,
		// may also init token-getter cutok.cpp and/or preprocessor pp.cpp
		// error file no longer opened here -- closes too soon 3-91.
		if (cufOpen( fName, defex) != RCOK)	// open file etc (cutok.cpp)
			return 1;				// consider error fatal

#else
0		// open input file,
0		// may also init token-getter cutok.cpp and/or preprocessor pp.cpp
0		// error file no longer opened here -- closes too soon 3-91.
0		if (cufOpen( fName, defex) != RCOK)	// open file etc (cutok.cpp)
0			return 1;				// consider error fatal
0
0		// call main table STAR-entry pre-input function, if any
0		if (cuf( PRF, STARS, 99) !=RCOK)	// call * entry (if any) fcn (if any)
0			return 1;			// make any error from .prf fcn fatal
#endif

	} // if cs==1

// reinitialize on reentry: call re-entry pre-input fcn, using main STAR check-function pointer, 7-12-92
	if (cs==2)				// if recall, to continue data entry eg for another a run.
		if (cuf( CKF, STARS, 99) !=RCOK)	// call * entry (if any) fcn (if any).  Note special use of "check-finish" function.
			return 1;			// make any error from .prf fcn fatal

// do clear data case
	if (cs==4)
	{
		// xSp = xStk	set xSp as finalClear requires if it gets too complicated to do at all other exits.
		finalClear();		// local, below.
		return 1;
	}

// compile
	if (cs != 3)		// unless just closing file. cases 0 and 4 have returned above.
		rv = culComp();		// compile, for cases 1 and 2.  returns 1,2,3,4. just below.
	else
		rv = 1;

	if (rv < 1 || rv > 3)
		err( PWRN, (char *)MH_S0206, (INT)rv); 	// display internal error msg
												// "cul.cpp:cul(): Unexpected culComp return value %d"
	if (rv != 2)		// unless recall expected
		cufClose();		// close file(s) (cutok.cpp)

	xSp = xStk;			/* clear xStk, as expected by finalClear() upon cul() recall with cs=4:
	    			   fixes xSp after many culComp() error returns. */
	if (rv != 1)		// 2 or 3
		culPhase = RUN_PHASE;	// say execution phase: may affect errMsgs.
	return rv;
}			// cul

//===========================================================================
LOCAL SI FC culComp()	/* compile cul commands from open input file
				   per current status (xSp)
				   until end file, run command, or error */
/* returns: 1: terminate session (fatal error or too many errors)
            2: execute (run), then re-call cul().
	    3: execute (run), then end session */
{
	CULT *c;
	XSTK* x;
	SI lastCs, cs=0, rv, nCmd=0, skef=0;
	RC rc;

	for ( ; ; )		// do commands until one returns
	{
		DMHEAPCHK( "culComp")
		lastCs = cs;	// skip2end needs to know if failure was in DEFTY
		cs = 0;		// this iteration's case not identified

		// if too many (non-fatal) compile errors, generate fatal error
		// errCount(): gets error count ++'d by calls to perNx, oer, err, etc, etc. rmkerr.cpp.
		// maxErrors: cuparse.cpp. Data init, accessible as $maxErrors.
		if (errCount() > maxErrors)
		{
			pInfol( (char *)MH_S0207, (INT)maxErrors );	// "More than %d errors.  Terminating session."
			// issue "Info" message with line number, cuparse.cpp.
			// xSp = xStk is done by caller
			return 1;
		}

		// if top level ended, end session (happens on eof not right after RUN)
		if (xSp < xStk)				// if culDo ended top level
		{
			xSp = xStk;				// restore for poss recall, eg cul(4) 1-12-91
			if (nCmd)				// if stmts seen in this culComp
				perl( (char *)MH_S0208);		// "'RUN' apparently missing"
			else					// nCmd==0
				if (culNRuns==0)			// if no RUN seen in session
					perl( (char *)MH_S0209);	// "no commands in file" (does not get here if top table has any REQDs)
				else
					perl( (char *)MH_S0210);	/* "cul.cpp:cul() internal error: no statements in file since 'RUN',\n"
						   "    yet cul() recalled" */
			return 1;				// end session, no (additional) RUN
		}

		// on flag, skip input to END at xSp->c->CULTP2 level

		/* flag indicates problem starting a group of stmts (typically RATE)
		   that prevents processing stmts in appropriate context: continuing
		   without skip would use wrong cult, making many spurious errors.
			  To unimplement, change to fatal error exit. */

		/* NB no xStk entry has been made for stmt containing error,
		   but xSp->c->CULTP2 must have been set. */

		if (skef)			// set on return 4 from a callee
		{
			skef = 0;
			if (skip2end( lastCs, &cs))	// local, below, returns 0 or 1.
				return 1;			// return fatal error to caller.
			/* skip2end may set 'cs', in which case we use that cs, skipping
			   toke() and switch.  (Occurs re group-starting 1st words DEFTY
			   and ALTER: otherwise 2-token backup would be necessary.) */
		}

		// get input token, test for end file, else expect verb (keyword)

		if (cs==0)			// skip switch(toke()) if skip2end() set cs
			switch (toke())	// get token (cuparse.cpp), returns tokTy.
       						//   sets: tokTy:     token type (cutok.h defs)
       						//   cuToktx[]: token text.
			{
			case CUTSEM:
				continue;	// ignore extra ;'s

			case CUTEOF:			// physical end of file
				if (errCount()==0)     		// suppress this msg if other errs, (pp errors can stop file reading)
					perl( (char *)MH_S0211);	// "'$EOF' not found at end file"
				/*lint -e516  fall thru */
			case CUTDEOF:			// '$eof': expected top-level terminator
				// assume enuf "end"'s to close any unclosed stmt groups
				rc = culEndTo( xStk);		// IEND out of nested tables
				// ignore minor error here: RCBAD/other
				if (rc==RCFATAL || rc==RCSKIP2END)	// (RCSKIP2END unexpected)
					return 1;			// nothing to skip at eof. fatal error return.
				cs = DEND;				// say is $eof, not a CULT command.
				goto found; 			/* join others. culENDER does RQD checks.
						   Exits below when top lvl ended (if nested, loops thru here til xStk empty). */

			case CUTDOY:    		// day-of-year type (impl as integer, 2-91)
			case CUTINT:			// type words begin user function or (future)
			case CUTFL:			// .. user variable definition.  They are
			case CUTSTR:			/* .. syntactically verb-like but processed by different code
         				   & entered in expr compiler's symbol table rather than appl data base. */
				// add check here to disallow in group
				fov();			// compile user function or vbl, cuparse.cpp.
				/* returns RC, disregard here. Msgs done, reflected in errCount. (to make error fatal, return 1 here.)
						Defined fcn or vbl available for use in subsequent exprs. */
				continue;

			case CUTDEFTY:  		// "deftype" define type for a basAnc
				cs = DEFTY;
				break;  		// say is "deftype" -- not a CULT id

				/* if changes are made re group-starting preliminary words DEFTY
				   and ALTER, make corresponding changes in skip2end(). */

			case CUTALTER:  		// "alter" reopen a RATE for modification
				cs = ALTER;
				break;  		// say is "alter" -- not a CULT id

			case CUTDELETE:  		// "delete" reopen a RATE for modification
				cs = CDELETE;		// say is "delete" -- not a CULT id
				goto found;			// join others

			case CUTRQ:    			// "require": set "required" flag for member
				cs = RQ;			// say is "required" -- not a CULT id
				goto found;			// join others

			case CUTFRZ:    		// "freeze": set "frozen" flag for member
				cs = FRZ;				// say is "freeze" -- not a CULT id
				goto found;			// join others

			case CUTUNSET:    		// "unset": reset member to default value
				cs = RESET;			// say is "unset" -- not a CULT id
				goto found;			// join others

			case CUTAUSZ:			// "autosize": determine value automatically 6-95 (a certain few tu and ah members only)
				cs = AUTOSZ;   			// say is "autosize" -- not a CULT id
				goto found;			// join others

			case CUTID:			// non-verb word gets own errmsg
				perlc( scWrapIf( "Unrecognized or misspelled word '%s':", 
					             " Class name, member name, or command expected.",
					             "\n   ", getCpl()),
					cuToktx );
				// If followed by =, is probably a member, skip stmt & continue.
				//  If no =, might be misspelled group-starting stmt,
				//  continuing w same table wd make many spurious errors, so make fatal.
				if (tkIf(CUTEQ))
				{
					perNx(NULL);  			// pass input to verb or after ;
					continue;			// resume compile
				}
				return 1;				// terminate session

			default:			// unrecognized token type: error message
				perNx( (char *)MH_S0212);		// display msg, pass ';'. cuparse.cpp.
				// "Syntax error.  Expecting a word: class name, member name, or verb.". wording??
				continue;				// continue compiling

			case CUTVRB: ;			// verb (keyword)
				// cs = 0;			preset before switch

			}	/*lint +e516*/	// switch (toke())
		// verbs fall out of switch with cs = 0;
		// 1st words DEFTY & ALTER break w cs set, class name NOT gotten.

		/* get class name verb after DEFTY/ALTER; look up verb */

		rc = bFind( &cs, &x, &c, WRN);	// local, below, issues messages.
		// updates cs if 0; sets x, c.
		if (rc)				// if bFind error
		{
			if (rc==RCFATAL)	// if fatal error (irrecoverable confusion)
				return 1;  	//   terminate session now
			continue; 		// continue compile, starting new statement
		}

		/* Assume needed number of implicit ENDs per 'x' returned by bFind:
		   End statement groups (object descriptions) to get to context where found,
		   ie to xStk level whose .cult containted the verb. */

		rc = culEndTo(x);	// does culEnder() til xSp <= x.
		// minor errors: RCBAD/other: continue compile
		if (rc==RCFATAL)		// if fatal error
			return 1;		// end session
		if (rc==RCSKIP2END)	// if "skip to end group" (unexpected here)
		{
			skef++;
			continue;	// set flag, go to top loop to skip
		}

		// now store table entry pointer in context stack frame
		xSp->c = c;			// c returned by bFind

found:	// recognized verb.  cs set, and xSp->c for cases in table.

		// compile one command

		if ( cs != DEND  &&  cs != END  		// count executable stmts:
		 &&  cs != ENDER &&  cs != DEFTY )	// non-end ...
			if (!defTyping)			// non-type-defining...
				nCmd++;				// stmts this call, re errMsgs

		rv = culDo(cs); 		// compile this cmd. returns 0 ok, 1 fatal,
		if (rv)					// ... 2,3 RUN, 4 skip input to matching END
		{
			if (rv==4)			// on request
				skef++;			//   say skip to END (at top loop)
			else
				return rv;		// return 1 (fatal), or 2 or 3 (run) now
		}
	}	// for ( ; ; ) commands loop
	// several returns above, including Z macros
}				// culComp
//===========================================================================
LOCAL SI FC culDo( 	// do one  c u l  command

	SI cs )	// case, as for CULT.cs.

// also uses: rest of cult entry (accessed via xSp->c) for  RATE DAT END ENDER... but not for DEND,IEND,DEFTY,RQ,FRZ,AUTOSZ.

/* returns: 0: ok. done if xSp < xStk (eof without RUN),
			   else keep compiling (call again)
            1: terminate session (fatal error or too many errors)
            2: run (execute), then re-call cul.
            3: run and terminate session: nothing more in file.
            4: skip to END at xSp->c->CULTP2 level and continue compile. */
{
	DMHEAPCHK( "culDo")

	RC rc;

	switch (cs)
	{
	default:
		perNx( (char *)MH_S0213, (INT)cs);	// "culDo() Internal error: unrecognized 'cs' %d"
		return 1;				// fatal error

	case RUN:					// run (execute) command
		return culRun();			// returns 1, 2, or 3.  4 unexpected.

	case DEND:				// $end or physical end of file
	case IEND:				// implicit end at verb in table above
	case END:					// generic END [name]
	case ENDER:  				// table word to terminate RATE
		rc = culENDER(cs);
		break;

	case DEFTY:			// make "type" basAnc Entry, find & start table
		// do like regular basAnc record, then ratPutTy in culENDER()).
		defTyping = 1;			// set global "defining a type" flag
		// trace = 1;
		rc = culRATE( 1, 0);  		// below. uses xSp->c.
		// defTyping stays set till corress level "culENDER" call; if error b4 xStk level added, skip2end() clears it.
		break;

	case ALTER:
		rc = culRATE( 0, 1);
		break;  	// reopen a basAnc record for modification.  fcn below. uses xSp->c.

	case CDELETE:
		rc = culDELETE();
		break;		// erase a basAnc record

	case RATE:
		// trace = 1;
		rc = culRATE( 0, 0);
		break;		// make basAnc (formerly RAT) Entry and start new table to fill

	case FRZ:
		rc = culRqFrz( FsFROZ);
		break;	// freeze member (name follows)

	case RQ:
		rc = culRqFrz( FsRQD);
		break;	// require member (name follows)

	case RESET:
		rc = culRESET();
		break;		// unset (redeflt) mbr (name follows)

	case AUTOSZ:
		rc = culAUTOSZ();
		break;		// autosize mbr (name follows)

	case NODAT:
		rc = culNODAT();
		break;		// non-data command: just call fcns

	case KDAT:
		rc = culKDAT();
		break;		// const-data command: no expr; call fcns

	case CLEAR:
		rc = culCLEAR();
		break;		// clear all data -- start over

	case DAT:
		rc = culDAT();
		break;		// data item: name = expr
	}	// switch (cs)

// process RC return code: 4 for RCSKIP2END, ignore non-fatal, etc.
	return fret(rc);			// and checks for context stack undeflow.
	// other returns above
}				// culDo
//===========================================================================
LOCAL SI FC culRun()	// do RUN command in top-level table

// user function called: ckf of RUN entry. (no longer calls STAR CKF: that ptr now used for reentry PRF 7-92.)

/* returns: 1: fatal error -- terminate session
				(accumulated non-fatal errors cause
				fatal error here to suppress execution).
			2: run (execute), then re-call cul().
            3: run and terminate session: nothing more in file.
            (4: skip to END at xSp->c->CULTP2 level, then continue compile) */
{

	DMHEAPCHK( "culRun")

// parse rest of command
	termnIf();				// pass ';' if present

// assume enough "end"'s to close all open stmt groups (but top level)
	SI rv = fret( culEndTo( xStk) );	// IEND out of nested tables
	if (rv)				// 0 ok, 1 fatal error, 2,3,4 unexpected
		return rv;			// return error now

// checks
	msgMissReq();	// error msgs for any missing req'd info per table of current stack frame (presumed top level)

// say check-phase
	// changes some errmsgs to show object's file/line, not current (of RUN)
	// probably mostly redundant in that cuf() sets CHECK_PHASE for run ckf.
	culPhase = CHECK_PHASE;

// resolve references (check-finish fcns assume already done)
	drefRes();		// resolve deferred-resolution references to basAnc records.  errCount++'s if issues errMsgs.  local.

#if 1 // 6-95: revert, cuz don't wanna do FAZ exprs till caller has determined $autosize.
// evaluate expressions whose evaluation time is 'end-of-input': replace 'probes' to input members with actual values
	exEvEvf( EVEOI, 0xffff);		// evaluate end-input exprs, all use classes. errCount++'s on errors. exman.cpp.
	/* note re CSE EVFFAZ expressions: can't do from here cuz $autosizing, used in such expressions, not known yet.
	   CSE RUN check/finish function topCkf first determines $autosizing, then evaluates phasely exprs b4 rest of processing. */
#elif defined(AUTOSIZE)//6-95
x// evaluate EVEOI and EVFFAZ expressions now, replacing probes or phase-variable expressions in input records with actual values.
x    exEvEvf( 				// evaluate exprs. errCount++'s on errors. exman.cpp.
x             EVEOI			// exprs whose value avail after input, eg probes of input mbrs eg those set by drefRes.
x             | EVFFAZ, 			// exprs whose value avail after input & which may change after autosizing - $autoSizing.
x             0xffff );			// all use classes
#else
x // evaluate expressions whose evaluation time is 'end-of-input': replace 'probes' to input members with actual values
x     exEvEvf( EVEOI, 0xffff);		// evaluate end-input exprs, all use classes. errCount++'s on errors. exman.cpp.
#endif

// call RUN command's check-finish function if any
	/* fcn may do final checks (so cult-style error messages still accessible)
	   and fill runtime data structures from input data structures (eg cncult.cpp:TopCkf).
	   References assumed resolved; end-of-input expressions assumed evaluated. */
	if (errCount()==0)					// if no errors to here
		cuf( CKF, NONDAT, 99);				// call xSp->c->ckf w args

// explanatory message if any errors
	if (errCount())  			// if any errors (just above or anytime during compilation) (pptok.cpp variable)
		pInfol( (char *)MH_S0214 );	// "No run due to error(s) above"

// be sure input listing report is complete to here, and not farther -- in report file, it is divided by run.
	// intentionally done before eof check below.
	// other listing stuff: pp.cpp; pptok.cpp & cutok.cpp re error messages.
	int line;
	curLine(				// get input line #, etc (etc not used here), cuparse.cpp.
		0,  			//  use start current token, or previous if have untok'd
		NULL,			//  would rcv input file index
		&line,			//  rcvs line number
		NULL,			//  would receive column
		NULL, 0 );			//  char[] buffer would rcv line text
	lisFlushThruLine(line);		// list to line and flush list buffer, only to line.  11-91. pp.cpp.
	// if this is last run, rest of listing buffer will be flushed when file closed.

// abort if any errors
	if (errCount())
		return 1;			// treat earlier errors as fatal at run: suppress execution

// execute RUN

	culNRuns++;			// count RUNs re errmsg in culComp

	// determine if nothing more in file; if so, return 3 not 2 (and cul closes)

	if (tkIf2( CUTEOF, CUTDEOF))	// get token / if any of / else unToke
		return 3;					// is $eof or eof
	return 2;			// 2 tells caller to run then call cul() again
	// note cul() sets culPhase to RUN_.
}		// culRun
//===========================================================================
LOCAL SI FC fret( RC rc)	// fix/fudge return code

// Converts RC return codes of internal fcns to SI return codes for cul().

/* rc:   RCOK:  succesful,  xSp < xStk to end session (eof w/o run)
     RCSKIP2END:  skip input to end of statement group defining object
     		(at xSp->c->CULTP2 level), continue compile: returns 4.
      RCFATAL:  fatal error, already msg'd, terminate session.  returns 1.
        other:  non-fatal error, message issued, rest of input statement
                    has been skipped, continue compilation.  returns 0. */
/* returns: 0:  ok
	    1:  fatal error, terminate session now
	 (2,3:  don't get here)
	    4:  skip to END, continue compile. */
{
// return any fatal error now.
	if (rc==RCFATAL)
		return 1;		// translate return code scheme
// disregard non-fatal errors here: continue compile (but errCount is set) */

	/* check for context stack overlow.
	   Note last entry is wasted, so can test for overflow here only.
	   Note UNDERflow is normal here at end of file. */

	if (xSp >= xStk + sizeof(xStk)/sizeof(XSTK) - 1)
	{
		perl( (char *)MH_S0215 );		/* "Context stack overflow: too much nesting in command tables. \n"
				        	   "    Increase size of xStk[] in cul.cpp." */
		return 1;				// fatal error
	}

// on code from aggregate, skip to matching ENDER
	if (rc==RCSKIP2END)
		return 4;	// tells culComp to skip2end()

	return 0;		// if here, return ok.  other returns above.
}		// fret
//===========================================================================
LOCAL RC FC culEndTo( XSTK* x)		// do "implicit ends" to given xStk level.

// note: where SI return required, use fret(culEndTo(x));
{
	RC rc=RCOK;

	while (xSp > x)
	{
		rc = culENDER(IEND);		// do 1 implicit end (xSp--'s)
		if (rc==RCFATAL)			// if fatal error,
			return rc;			// ret now, caller will end session.
		// rc=RCSKIP2END not expected here.
		// ignore minor errors (other RC's) here (?)
	}
	return rc;
}		// culEndTo
//===========================================================================
LOCAL RC FC culENDER( 	// do  c u l  end cases

	SI cs )	/* case:  ENDER: word coded as ENDER in current table
		    	  END:   generic "end" from stdVrbs table
		    	  IEND:  implicit end: verb matches hier table
		    	  DEND:  $eof or end of file seen. */

/* ends one table level (terminates one statement group), popping xSp.
   Is recalled for multi-level ends (DEND, IEND) */

/* also uses: xSp->cult
		  xSp->c if not IEND nor DEND. */

/* returns: RCOK:    succesful.  xSp < xStk if top level ended (DEND).
            RCFATAL: fatal error, message already issued, terminate session
            other:   non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{
	SI explt;
	RC rc;

	// note: call with xSp==xStk expected only with cs==DEND (at eof w/o run).

	explt = (cs==END || cs==ENDER);	/* true if explicit: have input stmt,
					   can process input text */

// scan rest of explicit END or ENDER statements; call check-fcn

	if (explt)			// scan no input at implicit end or eof
	{
		CULT *c = xSp->c;	// [c->cs != cs if non-table command (DEND, IEND)]

		// error message for top level explicit END
		if (xSp <= xStk)
			return perNx( (char *)MH_S0216);	// "Nothing to end (use $EOF at top level to terminate file)"

		// look for and check optional basAnc record name after command

		if ( (xSp->cs==3			// if ending a basAnc record
		  || xSp->cs==4)			// .. or DEFTYPE basAnc record
		  && nxPrec() > PRSEM )   	// if next token NOT terminator (eof, verb, verb-like, type, ';') (ungets the token)
		{
			char *p;
			if (ganame( 0, NULL, &p)==RCOK)   				// get id, "text", or const string expr (uses xSp->c->id)
			{
				if (!((record*)xSp->e)->IsNameMatch( p))	// if name wrong
					pWarnlc( (char *)MH_S0217, 					// warning message "%s name mismatch: '%s' vs '%s'"
					(char *)c->id, p, ((record*)(xSp->e))->name );
				dmfree( DMPP( p));
			} // else: continue on error. ganame assumed to have perNx'd.
		}

		// optional ; statement terminator
		termnIf();			// local fcn, below

		// call check-function (if any) of table command.  Temp sets CHECK_PHASE.
		F( cuf( CKF, NONDAT, 0) )	// call xSp->c->ckf w args, handle ret
		/* F macro rets RCFATAL errors now & continues here on others
				  (but all non-RCOK returns errCount++'d in cuf: tested at RUN.)
		   NB E'ing would make mess by skipping then continuing. */

	}		// if not DEND nor IEND

// call check-function (if any) of table's STAR entry (if any)
	F( cuf( CKF, STARS, 0) )	// call user fcn / handle return, below.

// Issue errMsgs for any unEntered required data items in basAnc record
	msgMissReq();		// below. uses xSp->cult. messages ++ errCount.

// call check-finish fcn of CALLING table entry if this RATE had one
	if (defTyping==0)			// DEFTYPE has no cult entry
		if (xSp > xStk)			// unless top level end (no caller)
			F( cuf( CKF, CLRS, 0) )	// call (xSp-1)->c->ckf w args, handle return.

// if defining a type, complete the definition and put it away
			if (xSp->cs==4)					// if end of DEFTYPE <ratName> <name> ...
			{
				/* convert regular basAnc record and any owned subRat records to "type(s)"
				   (massage flags) and move it to special sub-basAnc at basAnc.tyB. */
				CSE_E( ratPutTy( (record *)xSp->e, xSp->cult) )	// local, below.  returns RCOK/RCBAD/RCFATAL.
				defTyping = 0;					// clear "defining a type" flag
			}

// pop a context frame
	xSp--;		// underflows xStk if ended top level (expected at eof or $EOF w/o run). caller then ends session.
	return RCOK;	// good return. other returns above.
}		// culENDER

// culSTR, formerly 1-11-91 here, is in cul.ifo and cul.4, 1-23-91.

//===========================================================================
LOCAL RC FC culDELETE()	// do "delete" for culDo.  Uses xSp->c.

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
	    [RCSKIP2END: error, skip to END command for this object
	              as following input won't match table.]
            other:  non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{

// parse rest of statement
	RC rc;
	CSE_E( vFind( RATE) )		// get RATE verb for class name from input stream, look it up, msg if not found, do implicit ENDs
	CULT *c = xSp->c;
	BP b = (BP)c->b;

	// get Identifier, Quoted txt, or constant string eXpr for name, & get terminator.
	char* name;
	CSE_E( ganame( 0,		// disallow "all" and "sum"
			NULL,    	// use xSp->c->id in error messages
			&name ) ) 	// rcvs dm string ptr.  gets terminator. local.

// execute: find existing record to delete
	record *e;
	rc = ratLuCtx( b, name, c->id, &e);   	// look up basAnc record by name, resolve ambiguity per curr context. local.
	dmfree( DMPP( name));			// free name string in dm (heap): avoid memory leak. dmpak.cpp.
	if (rc)
		return RCBAD;    			// error return if not found or ambiguous, msg already done.

// execute...: delete record
	rc = rateDuper( NULL, e, -1, 0, -1, (CULT *)c->CULTP2 );		// local.  returns RCOK/RCBAD?/RCFATAL.
	// Squeezes record out of basAnc's block, changing subscripts of hier records.
	// Adjusts ownTi's of records in other basAncs owned by the moved records;
	// other refs are resolved at handled at RUN.
	return rc;
}			// culDELETE
//===========================================================================
LOCAL RC FC culRATE(	// do RATE cult entry
	SI defTy,	// non-0 if defining a type (set in call from culDEFTY; also global "defTyping" is alrady set)
	SI alter )	// non-0 if altering existing record -- else create new

/* returns: RCOK: succesful
         RCFATAL: fatal error, message already issued, terminate session
	  RCSKIP2END: error, skip to END command for this object as following input won't match table.
          [other:  non-fatal error, message issued, rest of input statement has been skipped, continue compilation.] */

{
	DMHEAPCHK( "culRATE entry")
	record *e;
	int fileIx, line;
	char *name=NULL, *likeName=NULL, *typeName=NULL;
	SI xprD=0, copy=0, lity=0;
	RC rc;

	CULT *c = xSp->c;
	BP b = (BP)c->b;   			// base of basAnc in which to make Entry

// initial checks
	if (!defTy)				// if not defining a type (this call)
	{
		if (!alter)			// alter may be bad; if good name, already set.
			c->IFLAGS |= BEEN_SET;		// say this command seen
	}
	if (c->IFLAGS & (ITFP|PRFP))
		return perNxE( (char *)MH_S0220);	// "cul.cpp:culRATE Internal error: bad table: RATE entry contains fcn not cult ptr"
	// more appropropriate err fcn?

// get file name index / line # here to put in basAnc record header
	curLine(			// get line, line, etc for errmsg, cuparse.cpp.
		0,  		// start current (not previous) token
		&fileIx,		// rcvs index of input file name
		&line,		// rcvs line number
		NULL,		// would receive column
		NULL, 0 );		// char[] buffer would rcv line text

	if (nxPrec() <= PRSEM)		// if terminator next (eof, verb-like, verb, type, ';') (ungets it)
	{
		if (defTy)				// deftype rq's name
		{
			toke();						// to position ^ in errmsg
			return perNxE( (char *)MH_S0221, (char *)c->id);	// err msg "%s type name required", skip group (?)
		}
		if (c->f & NM_RQD)					// if name required
		{
			toke();						// position errMsg ^
			return perNxE( (char *)MH_S0222, (char *)c->id);	// err msg "%s name required", skip group (?)
		}
	}
	else					// terminator not next
	{
		// get Identifier, Quoted txt, or constant string eXpression for name. local fcn.
		if ( ganame( 0,		// disallow "all" and "sum"
			NULL,		// use xSp->c->id in error messages
			&name )		// rcvs string ptr. ganame gets terminator.
			    != RCOK )
			return RCSKIP2END;		// on error in name, skip stmt blk (ganame already perNx'd).
		// duplicate name check is below
		xprD++;			// say terminator already gotten (by xpr from ganame)
		// note: permits ; between name and LIKE/USETYPE: not desired 1-91.
	}

// get optional "LIKE <name>" or "COPY <name>"
	if (!alter && tkIf2( CUTLIKE, CUTCOPY))
	{
		if (tokTy==CUTCOPY)
			copy++;			// with likeName, indicates copy

		if (c->f & NO_LITY)						// note 1-12-91 NO_LITY unused, but keep as might want it.
			return perNxE( (char *)MH_S0225, cuToktx, (char *)c->id);	// "'%s' not permitted with '%s'" ALSO USED JUST BELOW

		// get Identifier, Quoted text, or constant string eXpr for record name to like/copy
		if ( ganame( 0,    		// disallow "all" and "sum"
					 ttTx,		// "like" or "copy" txt for msgs, set by toke()
					 &likeName )		// rcvs string ptr. xpr (from ganame) gets terminator.
				!= RCOK )   		// (also uses xSp->c->id)
			return RCSKIP2END;		// on error in likeName, skip stmt blk.
		// don't look up name til new record added: basAnc may move
		xprD++;				// say terminator already gotten
	}

// get optional "USETYPE <name>"
	if (!alter && tkIf(CUTTYPE))
	{
		if (c->f & NO_LITY)						// note 1-12-91 NO_LITY unused, but keep as might want it.
			return perNxE( (char *)MH_S0225, cuToktx, (char *)c->id);	// "'%s' not permitted with '%s'" SECOND USE

		// >>> error checks for not USETYPEable can go here

		// get Identifier, Quoted text, or constant string eXpression for type name
		if ( ganame( 0,   		// disallow "all" and "sum"
					 ttTx,		// "usetype" text for errmsgs, set by toke()
					 &typeName )		// rcvs string ptr.  gets terminator.
				!= RCOK )   		// (also uses xSp->c->id)
			return RCSKIP2END;		// on error in typeName, skip stmt blk.
		// name is looked up later.
		xprD++;				// say terminator already gotten

		if (likeName || tkIf2(CUTLIKE,CUTCOPY) )			// if LIKE b4 or after
			return perNxE( (char *)MH_S0226, (char *)c->id );  	// "Can't use LIKE or COPY with USETYPE in same %s"
	}

// get optional terminator
	if (!xprD)			// not if xpr just called: it does terminator
		termnIf();		// get terminating ';' else unget. below.

	if (alter)
	{
// find existing record to alter
		if (ratLuCtx( b, name, c->id, &e) != RCOK)
			// look up basAnc record by name, resolving ambiguities per curr context
			return RCSKIP2END;			// not found or ambiguous, msg already done, say skip input to end of object
	}
	else	// if alter ...
	{
// check for duplicate name before adding record.
		if (name)				// if name given !!?? not checked til 1-92 !
		{
			// "owned" basAnc record names need only be unique for same owner:
			if ( !defTy			// type names must be globally unique
			&& xSp > xStk )		// insurance: if command is nested
			{
				if (b->findRecByNmO( name,   			// find basAnc record by name/owner, ancrec.cpp
				xSp->i,  			//  owner subscript. always here??
				NULL, NULL)==RCOK )  		//   if found
					// if owner ti can be set != xSp->i, then check again at END.<<<<
					perlc( 			// errMsg, show input line and ^. cuparse.cpp.
						scWrapIf( 							// strtcat w conditional \n
							strtprintf( (char *)MH_S0227, (char *)c->id, name ),	// "duplicate %s name '%s' in "
							xSp->b->rec(xSp->i).objIdTx( 1 ),	// owner class & obj name, etc
							"\n    "), getCpl()); 						// \n between if wd be longer
				// and continue here (?) (perlc prevents RUN)
			}
			else 						// not ownable or not in ownable context
				if ( (defTy  ?  b->tyB->findRecByNm1( name, NULL, NULL)  	// search basAnc's types basAnc if defining a type
				:  b  ?  b->findRecByNm1( name, NULL, NULL)  	// else the basAnc itself
				:  RCBAD )==RCOK )				// NULL b ??: unsure while converting code, sometime if possible
					// if any record with same name found found
					perlc( (char *)MH_S0228, (char *)c->id,  defTy ? " type" : "",  name );	// "duplicate %s%s name '%s'"
			// and continue here (?) (perlc prevents RUN)
			// if necessary to avoid (future) ambiguity during defTy, also search regular table (b).
#if 0/* added 11-94,
x       and undone cuz error msg already occurs in exman.cpp over 15 chars and truncation to 14 chars below fixed. */
x      // warn & continue if name overlong. This message added and search comparisons limited to ANAME-1 characters 11-94.
x          if (strlen(name) > sizeof(ANAME)-1)
x             pWarnlc( (char *)MH_S0280,					// "%s%s name \"%s\" is too long (%d characters). \n"
x                      (char *)c->id,   defTy ? " type" : "", 		// "    Only %d characters will be used."
x                      name,  strlen(name),  sizeof(ANAME)-1 );
#endif
		}

// add record to specified basAnc

		if (b->add( &e, WRN) != RCOK)			// add 0'd record to basAnc, ancrec.cpp.  Puts subscript in .ss.
			// if defTy, record is MOVED to tyB at END.
			// note basAnc::add's errmsg currently 1-92 does not go into input listing.
			return RCFATAL;				// out of memory, terminate session.
		/* note: there is no default data for RATEs in calling cult entry.
		   New record is all 0's (inits FsSET, FsVAL, FsERR, FsFROZ, etc to 0).
		   Default data can be supplied via STAR entry: see nuCult.  Still??7-92 */
		if (name)						// if name given
			e->SetName( name);			// copy name into record
		// .fileIx & .line are set below, after LIKE/COPY/USETYPE

		// do LIKE/COPY if given: look up record in this basAnc, copy data after name
		if (likeName)
		{
			record *likeE;
			if (ratLuCtx( b, likeName, c->id, &likeE)==RCOK)	// below.
				/* search for record in b by name / if found.  Resolves dupl name ambiguities in
				   owned-record basAncs by context (xStk): eg use surface in embedding zone if any.*/
				// else: msg issued, RUN stopped, continue here with raw record
			{
				lity = 1;					// say good COPY/LIKE in effect
				e->li = likeE->ss;				// nz means LIKE record. Avail for user fcns to test or access record.
				CSE_E( rateDuper( e, likeE, 0, 0, -1, (CULT *)(copy ? c->CULTP2 : NULL) ) )
				/* copy basAnc record contents, its deferred ref table entries (drefs),
				   and, if COPY, subrat records w drefs. local. can ret RCFATAL. */
			}
			dmfree( DMPP( likeName));
		}	// if likeName

		// do USETYPE if given: look up record in types subrat, copy data after name
		if (typeName)
		{
			record *typeE;
			if (b->tyB->findRecByNm1( typeName, NULL, /*VV*/ &typeE) != RCOK)
				perlc( (char *)MH_S0229, (char *)c->id, typeName);  		// "%s type '%s' not found"
			// and continue here with raw record (perlc prevents RUN)
			else					// found
			{
				lity = 2;				// say good USETYPE in effect. 2 distinguishes from COPY/LIKE - poss future use.
				e->ty = typeE->ss;		// nz means USETYPE'd entry. Avail for user fcns to test or access type
				CSE_E( rateDuper( e, typeE, 0, 0, -1, (CULT *)c->CULTP2) )	// copy contents, dref's, subRat records
			}
#if 1 // memory leak, 8-24-2010
			dmfree( DMPP( typeName));
#endif
			DMHEAPCHK( "USETYPE dup")
		}	// if typeName
	}	// if alter ... else ...

	dmfree( DMPP( name));				// free name string storage

// Add context stack level / invoke nested table
	xSp++;				// overflow checked by caller
	memset( xSp, 0, sizeof(XSTK) );	// 0 all members of new xStk entry
	xSp->cult = (CULT *)c->CULTP2;   	// new table per table
	xSp->cs = defTy  ?  4		// say entry for DEFTYPE RATE
			  :  3;		// say entry for RATE
	xSp->b = b;  			// basAnc
	xSp->i = e->ss;			// retrieve subscript of record
	//xSp->e = e;   			xStkPt sets	// record pointer
	//xSp->fs0 = (UCH *)e + b->sOff;	xStkPt sets	// point field status bytes in record
	xStkPt();				// compute ptrs in xStk in case a basAnc moved & to init new xSp->e, ->fs0. local, below.

	if (alter)
	{
		CULT *cc;
		SI fn;
// reInit for ALTER
		for (cc = xSp->cult;  cc->id;  cc++)	// clear CULT "entry has been used" flags: permits reentry
			cc->IFLAGS &= ~BEEN_SET;		// of item, without requiring it (FsSET, FsVAL, FsAS left on).
		for (fn = 0;  fn < b->nFlds;  fn++)	// clear field status "error msg issued for field" bits:
			xSp->fs0[fn] &= ~FsERR;		// insure full check & reporting of errors after alter. 12-91.
	}
	else
	{
// not ALTER: finish record init, init cult, init new record, call fcns

		// init more members of new record (here so LIKE etc won't overwrite)
		e->fileIx = fileIx;  		// file name index and line # for errmsgs
		e->line = line;  		// .. (obtained by curLine call above)

		// call calling-cult-entry .itf (in its .DFPI, since .p2 used for cult).  CAUTION: must not overwrite name etc.
		if (!lity)			// no .itf's called for type/copy/like 1-2-91.
			FS( cuf( ITF, CLRS, 99))	// if nonNULL, call (xSp-1)->c->itf w args

		// init flags, do defaults, call member init fcns for new table
		FSP( nuCult( 0, lity) ) 		// below.  Uses/alters *xSp.  FSP: on non-fatal err, xSp--, ret RCSKIP2END.

		// and always do basAnc context-dependent defaults (DFLIN/RDFLIN)
		FSP( xCult() )			// similar local fcn below

		// call optional functions pointed to by tables

		// .itf fcns: calling cult entry: above after ratAdd call. STAR entry and members: nuCult call just above.

		// call pre-entry fcn of nested table's STAR entry, if any
		FSP( cuf( PRF, STARS, 99) )	// FSP: on non-fatal err, xSp--,
		//   ret RCSKIP2END.

		// .ckf functions: called after data entry by culDAT and culENDER.
	}
	DMHEAPCHK( "culRATE exit")
	return RCOK;			// other rets above incl FSP macros
}		// culRATE

// are new nxRat etc fcns pertinent here? 1-22-91 <<<<<<
// nuCult b4 many deletions is in cul.ifo, cul.4 1-23-91.
//===========================================================================
LOCAL RC FC nuCult(

	/* initialize to use new xSp->cult table: STAR, flags, non-array defaults and member init fcns.
						  see also xCult: context dependent portions */

	SI clr,	/* non-0: clear: remove all records from ref'd basAncs: used to init at cul() entry, and from culCLEAR.
		   0:     do not clear: used normally, in case basAnc ref'd elsewhere and data already entered. */

	SI lity )	/* non-0 if RATE already init, as with LIKE or USETYPE: don't default data; don't call member itf fcns 1-2-91
    		   (will probably need likeItf fcns or addl info to fcns) */
// (1-91: culRATE passes lity = 1 for COPY/LIKE, 2 for USETYPE, in case we need to distinguish)

/* inits, for DAT cult entries:  default values.
			         IFLAGS bits: BEEN_SET: 0 */
// calls: initialization fcns for STAR, DAT.
// uses: xSp->cult, ->e,

// RATEs are NOT init'd, tho they are cleared if clr.

// CAUTION: also alters xSp->c, ->p, ->sz

/* returns: RCOK:    succesful -- continue
            RCFATAL: fatal error, message already issued, terminate session
            [other:  non-fatal error, message issued, rest of input stmt skipped, return to culDo and continue compilation.] */
{
	RC rc;

	CULT *c = xSp->cult;

	/* STAR entry: overall default data and/or initialization function */
	// itf fcn call occurs AFTER default data and .itf from calling entry, but b4 member defaults and .itfs

	if ( c->cs==STAR	// if have STAR entry (always 1st if present)
	&& !lity )	// 1-2-91 tentatively NO fcns, no init for LIKE/USETYPE
	{
		F( cuf( ITF, STARS, 0) )	// call xSp->cult->itf w args, handle return
		/* F macro rets RCFATAL errors now & continues here on others
				  (but all non-RCOK rets errCount++'d in cuf -- tested at RUN.)
		   NB E'ing would make mess by skipping then continuing. */
	}

	/* loop over members of table */

	for ( /*c=xSp->cult*/ ;  c->id;  c++)
	{
		xSp->c = c;			// input to cuf(), datPt()

		// init flags

		// runtime flag bits in cult entry
		c->IFLAGS &= ~BEEN_SET;		// clear flag saying entry has been used

		// flag bits in field status byte
		if ( lity			// if not like or usetype, ratAdd inits them 0.
		&& c->cs==DAT )		// if datum -- else has no fs byte
		{
			UCH *fs = xSp->fs0 + c->fn;	// point field's data status byte
			/* like/type fs flag init not fully clear yet, but at least clear "error msg issued",
			   and "value has been entered" under type/uselike if type said "required" */
			int j = 0;			// element subscript in array field
			do
			{
				fs[j] &= ~FsERR;		// say no msg issued for field: insure full checking. 12-91.
				if (fs[0] & FsRQD)		// only set here via type (or like).  NB FsRQD[0] applies to all ARRAY elts.
					fs[j] &= ~FsSET;    	// only set here via like or type.  can leave FsVAL on, as not used re rqd check, right?
			}
			while (c->f & ARRAY  &&  ++j < int( c->p2));		// if array field, loop over elements
		}

		// call init fcn for NODAT entry, if any.  Uses xSp->c.
		if (!lity)
			if (c->cs==NODAT)
				F( cuf( ITF, NONDAT, 0) )  	// call xSp->c->itf(xSp->c, xSp->e)

				// call init fcn for KDAT entry, if any.  Uses xSp->c.
				if (!lity)
					if (c->cs==KDAT)			// ARRAY flag not supported for KDAT (nor checked)
					{
						rc = datPt();
						if (rc==RCFATAL)
							return rc;
						if (rc==RCOK)
							F( cuf( ITF, RGLR, 0) )		// call xSp->c->itf(xSp->c, xSp->p)
						}

		// conditionally remove all records and free dm data from referenced basAncs

		if (clr)					// only on "clear" calls
			if (c->cs==RATE)			// if this entry creates a basAnc record
			{
				clearRat(c);	/* frees dm data (TYSTR) and removes records from basAnc and basAncs c->CULTP2 references.
	     			   Uses c->b, c->CULTP2.  only call.  below. */
				/* Note caller culCLEAR called clear .itf fcn b4 calling nuCult.
				   that fcn must have done all exceptional cleanup. */
				/* Code incomplete 1-13-91: (unexpected) nested static basAncs are not initialized (default/UNSET/.itf etc).
				   Should clearRat() call back to nuCult() to do that (place commented)? */
			}

		// default and initialize members

		if (c->cs==DAT) 				// if datum
		{
			rc = datPt(); 				// point to datum: uses xSp->c, xSp->e.  sets xSp->p, sz, arSz.
			if (rc==RCFATAL)
				return rc;   				// return fatal error to caller
			else 							// non-fatal errors skip member (in 'for' endtest) & continue
				for ( ; xSp->j < xSp->arSz  &&  rc==RCOK;  		// loop over array elements (usually 1 only) if/while ok
				xSp->j++, IncP( DMPP( xSp->p), xSp->sz) )
				{
					USI sz = xSp->sz;
					void *p = xSp->p;		// fetch member (element) size and location

					if (lity)
					{

						// duplicate dm data in like/usetype copy

						if ( c->cs==DAT 						// if data (appears redundant 11-91)
						 && (c->ty==TYSTR 						// if data is a string
						     || (c->ty==TYFLSTR && ((VALNDT *)p)->ty==TYSTR )) )  	// or a float-or-string set to string 11-91
						{
							//  nb assumes offsetof(VALNDT,val)==0
							if ( *(char **)p != NULL		// if ptr not NULL
							 && !ISNANDLE(*(void **)p) )	// and not UNSET or expr handle
								dmIncRef( (void **)p);		// increment reference count of pointed-to block, dmpak.cpp
						}
					}
					else  			// not under LIKE or USETYPE
					{

						// default member.  similar code in culRESET().

						if ( c->ty==TYSTR					// if a string datum
						||  c->ty==TYFLSTR && ((VALNDT *)p)->ty==TYSTR )  	// or a float-or-string set to string 11-91
							//  nb assumes offsetof(VALNDT,val)==0
						{
#if 0 // in use til 3-95!
x							dmfree( DMPP( *p));      		// free any prior string value: free ram, NULL ptr, dmpak.cpp
#else // fix crash if never set b4 CLEAR, 3-95
							if (ISNANDLE(*(void **)p) )		// if UNSET or an expression reference (eg CLEAR b4 ever set)
								*(void **)p = NULL;		// just remove value, don't crash in dmfree, 3-95
							else					// normally is heap pointer or NULL (dmfree checks for NULL)
								dmfree( (void **)p);    		// free any prior string value: free ram, NULL ptr, dmpak.cpp
#endif
						}
						if (sz==4  &&  c->f & RQD) 		// insurance: if a 4-byte required item
							*(NANDAT*)p = UNSET; 		// default to "not set"
#ifdef DF1
						else if (c->ty==TYFL || c->ty==TYNC)	// floats: units.
#else
						else if ( c->ty==TYFL			// float: sep cult mbr; units.
						||  c->ty==TYNC && !c->DFPI ) 	// number/choice is init as a number from .dff if .dfpi is 0.
#endif
						{
							if (!ISNUM(c->DFF))				// keep nonNumeric value out of coprocessor!
								*(NANDAT *)p = *(NANDAT *)&c->DFF;	// (if !DF1, non-num default otta be in DFPI; insurance).
							else
								*(float *)p =
									(float)cvExtoIn(
										*(float *)&c->DFF,			// scale value in field's external units to internal, cvpak.cpp
										sFdtab[ xSp->b->fir[c->fn].fi_fdTy ].untype );			// field's units
									// note cvpak units scaling applies only to
									// NC's, DTFLOAT, [DTDBL], [DTPERCENT], and [DTSSE].
						}
						else if (sz <= 4)			// if <= 4 bytes long, default data is IN .DFPI
							memcpy( p, &c->DFPI, sz);		// copy default value to member
						else if (c->DFPI)			// over 4 bytes, if .DFPI is non-NULL
							memcpy( p, c->DFPI, sz);		// .DFPI is POINTER TO default data. copy data into member.
						else					// NULL pointer in .DFPI: memcpy would get GP fault
							memset( p, 0, sz);			// zero member larger than 4 bytes if data pointer NULL, rob 6-3-92.

						// LIMITS: are not checked for defaults, so that special null values etc may be used even if out of range.

						// DFLIN/RDFLIN: default to ref to basAnc Entry in which nested: must be deflt'd at each context change -- see xCult.
						if (c->f & RDFLIN  &&  (c->ty==TYIREF || c->ty==TYREF) )
							c->f |= RQD; 			// here, for RDFLIN, just say "input required" in case at top level

						// call datum's initialization fcn if any.  Not under like/usetype.

						F( cuf(ITF,RGLR,0) )			// call with args, handle return.  Continue here on non-fatal error.
					}      // if lity ... else ...
				}     // for (j=  array loop / datPt OK
		}    // if DAT
	}   // for (c=
	return RCOK;		// other returns above incl in E, F macros
}		// nuCult
//===========================================================================
LOCAL RC FC clearRat( CULT *c)

// clear basAnc per cult entry, and basAncs it references, and their types

/* returns: RCOK:    succesful -- continue
            RCFATAL: fatal error, message already issued, terminate session
            [other:  non-fatal error, message issued, rest of input statement
                     skipped, return to culDo and continue compilation.] */
{
	BP b = (BP)c->b;
	CULT *cc;

	if (c->IFLAGS & (ITFP|PRFP) || !c->p2)
	{
		per( (char *)MH_S0200);			// "cul.cpp:clearRat internal error: bad tables: RATE entry without CULT ptr"
		return RCFATAL;
	}

// make nested context stack frame for basAnc b and CULT c->CULTP2

	// check for full xStk.  Keeps an extra space to match test in culDo.
	if (xSp >= xStk + sizeof(xStk)/sizeof(XSTK) - 1 - 1)
	{
		perl( (char *)MH_S0230);		/* "cul.cpp:clearRat Internal error: Too much nesting in tables: \n"
						   "    Increase size of context stack xStk[] in cul.cpp." */
		return RCFATAL;				// fatal error
	}
	xSp++;				// next context stack level
	memset( xSp, 0, sizeof(XSTK) );	// 0 all members of new xStk entry
	xSp->cult = (CULT *)c->CULTP2;    	// nested cmd table
	xSp->cs = 3;			// say entry for RATE
	xSp->b = b;				// base of the basAnc
	// xSp->->i, ->e, and ->fs0 set in record loop below

// call self for any nested basAncs, to free their data & clear them.

	//if (xSp->b->n > 0)		**?? do even if no records in this basAnc ??
	for (cc = xSp->cult; cc->id; cc++)		// loop table records
		if (cc->cs==RATE)
			clearRat(cc);

// loop over records in this basAnc to free data

	for (xSp->i = xSp->b->mn;  xSp->i <= xSp->b->n;  xSp->i++)	// loop basAnc records, even .gud <= 0 ones
	{
		xStkPt();				// compute xSp->e and xSp->fs0 for xSp->i. local, below.

		/* free known dm data in members of record.  Appl should use CLEAR's .itf fcn (called b4 here) for other dmfree's and
							        cleanup: non-CULT members, dm pointers not DAT & TYSTR, derived info, etc.
							        Also, C++ destructors are now called (by b->free() below. */
		for (cc = (CULT *)c->CULTP2;  cc->id;  cc++)		// loop table records
			if (cc->cs==DAT)					// if table entry is for data
			{
				void *p = (char *)xSp->e + xSp->b->fir[ cc->fn ].fi_off;	// datPt() subset: record base + member offset
				if ( cc->ty==TYSTR						// if data is string,
				||  cc->ty==TYFLSTR && ((VALNDT*)p)->ty==TYSTR )		//  or float-or-string now set to string,
				{
					int arSz = cc->f & ARRAY ? int( cc->p2) : 1;			// datPt() subset... # array elements: usually 1
					int j;
					for (j = 0;  j < arSz;  j++, IncP( &p, sizeof(char *)) )	// loop array elements
						if (ISNANDLE(*(void **)p))					// if UNSET, ASING, expr handle, etc (cnglob.h)
							*(void **)p = NULL;				// just remove value, don't crash in dmfree, 11-95
						else								// else is heap ptr or NULL (dmfree cks 4 NULL)
							dmfree( (void **)p);		// free the string, set ptr NULL. dmpak.cpp.
				}
			}									// CAUTION: assumes VALNDT.val is 1st member.
	}

// now free records of this basAnc
	if (!(b->ba_flags & RFSTAT))				// unless "static-record" basAnc like TopR
	{
		// >>>> (future) code needed to remove names from central symtab?
		b->free();					// destroy/delete all of basAnc's records, ancrec.cpp
		if (b->tyB)					// if basAnc has types sub-basAnc
			b->tyB->free();				// clear its types too
	}
	else						// nested static basAnc (unexpected 1-91)
		b->statSetup( *b->ptr(), 0, 0);			// zero and reinit it, ancrec.cpp
	/* (unexpected 1-91) nested static basAnc needs nuCult'ing
	   to default members.  Do here while have xStk frame? <<<<<< */

// pop context stack: drop entry made above
	xSp--;

	return RCOK;
}		// clearRat

//===========================================================================
LOCAL RC FC xCult()

/* initialize to use new xSp->cult table: context dependent portion:
	RDFLIN/DFLIN defaults.
      see also nuCult(), above. */

/* returns: RCOK:    succesful -- continue
            RCFATAL: fatal error, message already issued, terminate session
            [other:  non-fatal error, message issued, rest of input statement
                     skipped, return to culDo and continue compilation.] */
{
	CULT *c;
	XSTK* x;
	RC rc;

	/* loop over members of table */

	for (c = xSp->cult;  c->id;  c++)
	{
		xSp->c = c;			// input to datPt, self.

		if (c->cs==DAT &&  c->f & (RDFLIN|DFLIN)  &&  (c->ty==TYIREF || c->ty==TYREF) )
		{
			rc = datPt(); 				// pt to data member: set xSp->p, sz, arSz.
			if (rc==RCFATAL)  return rc;			// return fatal error to caller
			else 							// non-fatal errors skip member (in 'for' endtest) & continue
				for ( ; xSp->j < xSp->arSz  &&  rc==RCOK; 			// loop over array elements (usually 1 only) if/while ok
				xSp->j++, IncP( DMPP( xSp->p), xSp->sz))
				{

					// DFLIN/RDFLIN: default to reference to basAnc Entry in which nested
					// similar code in culRESET()

					/* Say required and re-default, in case not nested in correct RATE.
					     NB really must store default now to have it if input error:
					     a multiply-used table may have been xCult'd from in a RATE,
					     storing nz value, since nuCult'd. */

					*(TI*)xSp->p = (TI)(ULI)c->DFPI; 	// set element to tabulated default value
					if (xSp->j==0)   			// elements after [0] are not defaulted to embedding basAnc record,
						// and there is only one RQD flag.
					{
						if (defTyping==0)		// nothing is required in a TYPE
							if (c->f & RDFLIN)		// if required if not settable per context 11-17-91
								c->f |= RQD;    		// say must be input

						// search embedding context stack levels
						for (x = xStk; x < xSp; x++)	// loop embedding lvls
						{
							if ( x->b==c->b )		// if RATE of desired type
								// c->b now same as xSp->r->ownB here ... see ratCultO().
							{
								*(TI *)xSp->p = x->i;	// default to ref to it
								c->f &= ~RQD;		// input not required
								// no break: loop runs up & must complete to get nearest
							}
						}
					}
				}	// for (j=  array loop / if rc==RCOK
		}    // if DAT && ...
	}		// for (c=

	return RCOK;		// other returns above incl in E, F macros
}		// xCult
//===========================================================================
LOCAL RC FC culRqFrz( 	// do "Require" or "Freeze" non-table command

	UCH flag ) 		// sstat[] flag bit to set: FsFROZ or FsRQD

// uses ttTx for errMsgs: verb "freeze" etc must be last gotten token.

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            other:  non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{
	RC rc;

// parse rest of statment
	CSE_E( vFind( DAT) )	/* get DAT verb from input stream, look it up,
    			   msg if not found, do implicit ENDs, set xSp->c. */
	termnIf();		// get optional terminating ;

// point at the member and its flags byte.  local.
	CSE_E( datPt() )

// execute command
	*xSp->fs |= flag;		// say member required or frozen (FsFROZ/FsRQD used only on 1st elt of ARRAYs)
	return rc;
}		// culRqFrz
//===========================================================================
LOCAL RC FC culAUTOSZ() 	// do "AutoSize" non-table command

// uses ttTx for errMsgs: verb "autoSize" etc must be last gotten token.

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            other:  non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{
	RC rc;

// parse rest of statment, check for autosizable member
	CSE_E( vFind( DAT) )					/* get DAT verb from input stream, look it up, msg if not found,
							   do implicit ENDs, set xSp->c. */
	if (!(xSp->c->f & AS_OK))				// error if member not flagged as autosizable in CULT table
		return perNx( (char *)MH_S0280, cuToktx);	// "'%s' cannot be AUTOSIZEd"

	termnIf();						// get optional terminating ;

// error if previously set
	if (xSp->c->IFLAGS & BEEN_SET)
		return perNx( (char *)MH_S0281,    			// "Invalid duplicate 'AUTOSIZE %s' or '%s =' command"
			(char *)xSp->c->id, (char *)xSp->c->id );

// point at the member and its flags byte.  local.
	CSE_E( datPt() )

// execute command

	// say an AUTOSIZE command seen if caller gave optional pointer to flag
	if (pAuszF)  *pAuszF = TRUE;

	// say AUTOSIZE x seen, thus value given for x
	*xSp->fs |= FsAS | FsSET;		// say AutoSize has been given for member; say a value has been given for member
	xSp->c->IFLAGS |= BEEN_SET;      	// say this command seen in cult
	// ?? former lasts thru type/like; only latter triggers 'invalid duplicate' message.

	// store NAN in x. May help detect invalid probes (cuprobe.cpp)
	if (xSp->sz==4)			// insurance -- only TYFL's expected
		*(NANDAT *)xSp->p = ASING;
	return rc;
}		// culAUTOSZ
//===========================================================================
LOCAL RC FC culRESET() 	// "unset" a member -- re-default it

// allows reversion to special "not-entered" values that cannot be
// entered explicitly as outside of field's limits

// uses ttTx for errMsgs: verb "unset" etc must be last gotten token.

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            other:  non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{
	CULT *c;
	void *p;
	XSTK* x;
	RC rc;

// parse rest of statment
	CSE_E( vFind( DAT) )	/* get DAT verb from input stream, look it up,
    			   msg if not found, do implicit ENDs, set xSp->c. */
	termnIf();		// get optional terminating ;

// point at the member and its flags byte.  local.
	CSE_E( datPt() )
	for ( ; xSp->j < xSp->arSz;  xSp->j++, IncP( DMPP( xSp->p), xSp->sz))	// loop ARRAY elements (usually 1)
	{

		// 0: was dmfree'ing prior TYSTR omitted? 3-92.

		// execute command: 1. default member, like nuCult().

		c = xSp->c;
		p = xSp->p;
		if ( c->ty==TYSTR   				// if a string
		||  c->ty==TYFLSTR && ((VALNDT*)p)->ty==TYSTR )	//  or a float-or-string now set to string,
		{
			if (ISNANDLE(*reinterpret_cast<NANDAT *>(p)) )	// if UNSET or an expression reference (eg RESET b4 ever set?)
				*reinterpret_cast<NANDAT *>(p) = NULL;		// just remove value, don't crash in dmfree, 3-95
			else						// normally is heap pointer or NULL (dmfree checks for NULL)
				dmfree( (void**)p);     			// free any prior string value: free ram, set ptr NULL, dmpak.cpp
		}
		if (xSp->sz==4  &&  c->f & RQD) 			// insurance: if a 4-byte required item
			*reinterpret_cast<NANDAT *>(p) = UNSET;  			// default to "not set"
#ifdef DF1
		else if (c->ty==TYFL || c->ty==TYNC)		// floats: units.
#else
		else if ( c->ty==TYFL				// float: sep cult mbr; units.
		||  c->ty==TYNC && !c->DFPI ) 		// number/choice is defaulted as a number from .dff if .dfpi is 0.
#endif
		{
			if (!ISNUM(c->DFF))				// keep nonNumeric value out of coprocessor!
				*(NANDAT *)p = *(NANDAT *)&c->DFF; 	// (if !DF1, non-num default otta be in DFPI; insurance).
			else
				*(float *)p = (float)cvExtoIn(  			// scale value in field's external units to internal, lib\cvpak.cpp
					*(float *)&c->DFF,     				// fetch float default as float
					sFdtab[ xSp->b->fir[c->fn].fi_fdTy ].untype ); 	// fld's units
		}
		else if (xSp->sz <= 4)			// if <= 4 bytes long, default data is IN .DFPI
			memcpy( p, &c->DFPI, xSp->sz);  	// copy data default data to member
		else if (c->DFPI)			// over 4 bytes (unexpected here), if .DFPI is non-NULL
			memcpy( p, c->DFPI, xSp->sz);		// .DFPI is POINTER TO data. copy data into member.
		else					// NULL pointer in .DFPI: memcpy would get GP fault
			memset( p, 0, xSp->sz);   		// zero member (larger than 4 bytes) if data pointer NULL, rob 6-3-92

		// 2. DFLIN/RDFLIN: deflt to ref to basAnc Entry in which (now !?) nested, like xCult().

		if (xSp->j==0)						// for ARRAY, do element [0] only
			if (c->f & (RDFLIN|DFLIN)  &&  (c->ty==TYIREF || c->ty==TYREF) )
			{
				if (defTyping==0)  			// require nothing in a TYPE
					if (c->f & RDFLIN)			// DFLIN (no 'R') does NOT imply RQD
						c->f |= RQD;	        		// in case not suitably nested
				// field was set to default value above.
				// search embedding context stack levels
				for (x = xStk; x < xSp; x++)		// loop embedding lvls
				{
					if (x->b==c->b )			// if RATE of desired type
					{
						*(TI *)xSp->p = x->i;		// default to ref to it
						c->f &= ~RQD;			// input not required
						// no break: loop runs up & must complete to get nearest
					}
				}
			}

		// 3. flags: as tho not yet input

		c->IFLAGS &= ~BEEN_SET;  		// cult entry: clear has-been-input
		xSp->fs[xSp->j] &= 			// record field status:
			~(FsSET|FsVAL|FsERR|FsAS);		//   clear has-been-entered, value-is-stored, error-msg-issued, autosize

		// 4.  delete entries for field in deferred reference table and expression table (added 12-91: apparent omission)

		if (c->ty==TYIREF || c->ty==TYREF)
			drefDelFn( xSp->b, xSp->i, xSp->c->fn + xSp->j);    	// from dref table. local, below. array elts are successive fn's.
		extDelFn( xSp->b, xSp->i, xSp->c->fn);			// from expression table.  exman.cpp


		// do we wanna do this? 2-91.
		//// 5. call datum's initialization fcn if any.
		//    F( cuf(ITF,RGLR,0) )   		// call with args, handle return. continue here on non-fatal error.

	}	// for ( ; j ... ARRAY element loop

	return rc;				// error returns above, incl CSE_E macros
}		// culRESET
//===========================================================================
LOCAL RC FC culNODAT()	// do cul no-data case per xSp

// this table entry creates a command word that just calls functions

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            [other:  non-fatal error, message issued, rest of input statement has been skipped, continue compilation.] */
{
	// .itf function, if any, is called by nuCult.

// "pre-input" function.  This fcn may scan arguments, eg to skip obsolete command arguments 1-93.
	F( cuf( PRF, NONDAT, 0) )      	// call fcn if any with args, ret fatal error to caller now

// get terminating ; else unget
	termnIf();

// "finish/check" function
	F( cuf( CKF, NONDAT, 0) )		// call w args, handle return. local.

	return RCOK;
}			// culNODAT
//===========================================================================
LOCAL RC FC culKDAT()	// do cul constant-data case per xSp

// stores constant values (from table "default" members) and calls functions.

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            [other:  non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation.] */
{
	CULT *c;
	RC rc;

	c = xSp->c;
	termnIf();				// get terminating ; else unget

	// .itf function, if any, is called by nuCult.

// point to data storage: sets xSp->p, ->sz, ->fs, etc. local.
	CSE_E( datPt() )
	// ARRAY flag assumed not used, but not checked.

// call "pre-input" function
	F( cuf( PRF, RGLR, 0) )		// call fcn if any with args, ret fatal error to caller now

// set datum from "default" table info
	if ( c->ty==TYSTR    					// free any prior string value
	||  c->ty==TYFLSTR && ((VALNDT*)xSp->p)->ty==TYSTR )	//  or float-or-string now set to string,
#if 1 // 11-1-95: another one like nuCult 3-95 or clearRat 11-95!
		if (ISNANDLE(*(void **)xSp->p)) 			// if UNSET, ASING, expr handle, etc (cnglob.h)
			*(void **)xSp->p = NULL;  				// just remove value, don't crash in dmfree, 11-95
		else							// else is heap ptr or NULL (dmfree cks for NULL)
#endif
			dmfree( (void **)xSp->p);  				// free ram, NULL ptr, dmpak.cpp
#ifdef DF1
	else if (c->ty==TYFL || c->ty==TYNC)			// floats: units.
#else
	else if ( c->ty==TYFL					// float: sep cult mbr; units.
	||  c->ty==TYNC && !c->DFPI ) 			// number/choice: use .dff if .dfpi is 0.
#endif
	{
		if (!ISNUM(c->DFF))					// keep nonNumeric value out of coprocessor!
			*(NANDAT *)xSp->p = *(NANDAT *)&c->DFF;   		// (if !DF1, insurance: non-num default otta be in DFPI)
		else
			*(float *)xSp->p = (float)cvExtoIn(			// scale value in field's external units to internal, lib\cvpak.cpp
				*(float*)&c->DFF, 				// float default in cult
				sFdtab[ xSp->fdTy ].untype ); 		// field's units
		// note TYNC's and DTFLOAT are the only cul types to which cvpak applies units, 2-91.
	}
	else if (xSp->sz <= 4)				// if <= 4 bytes long, data is IN .DFPI
		// if ((c->ty==TYIREF || c->ty==TYREF) && c->f & (RDFLIN|DFLIN))
		//   if desired, do default to embedding basAnc ti here.
		// else
		memcpy( xSp->p, &c->DFPI, xSp->sz);		// copy data to member
	else if (c->DFPI)					// over 4 bytes, if .DFPI is non-NULL
		memcpy( xSp->p, c->DFPI, xSp->sz);		// .DFPI is POINTER TO data. copy data into member.
	else						// NULL pointer in .DFPI: memcpy would get GP fault
		memset( xSp->p, 0, xSp->sz);			// zero member larger than 4 bytes if data pointer NULL, rob 6-3-92.

	// LIMITs check desirable here 2-92?

// say this member has been set
	*(xSp->fs) |= FsSET|FsVAL;		// say 'has been entered' and 'value stored' in fld status byte
	c->IFLAGS |= BEEN_SET;      	// say this command seen in cult

// call "finish/check" function
	F( cuf( CKF, RGLR, 0) )  	// call w args, handle return. local.

	return RCOK;
}		// culKDAT
//===========================================================================
LOCAL RC FC culCLEAR()     	// do cul CLEAR command

/* re-defaults and re-inits all data for current cult and below;
   removes all records from basAncs and from referenced basAncs.

   .itf/.prf/.ckf functions must be used to repeat other required init.

   Expected use is at top level.  Below top level, should work but:
     will clear ref'd basAncs and arrays even if also ref'd via unCleared cults;
     will not zero RATE b4 dflting mbrs.
     does not clear expression table entries --
       ok if caller extClr's before registering exprs at each RUN
     does not clear autosizing flag *pAuszF (6-95) --
       ok if caller searches records for FsAS flags to determine if any AUTOSIZEs given*/

/* calls to optional functions:
     CLEAR.itf			<--- nb BEFORE data cleared
     per cults, as at startup, while data is being re-defaulted:
        STAR.itf's, KDAT.itf's, data itf's.
     CLEAR.prf
     CLEAR.ckf
   not re-called: STAR.prf: but could put same fcn in CLEAR.prf or .ckf.
	    (xxxx note STAR.prf IS recalled at cul() re-call NO undone 12-90). */

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            other:   non-fatal error, message issued, continue compilation. */
{
	if (xSp==xStk)		// if CLEAR given in top level cult,
	{
		clearErrCount();	// forget any preceding errors: set error count to 0. rmkerr.cpp.
		culNClears++;		// count top level CLEAR commands: changes errmsg in msgMissReq.
		// clear AUTOSIZE's given flag
		if (pAuszF) 		// if non-null ptr to flag given in cs==1 cul call (local global)
			*pAuszF = FALSE;	// set the caller's flag FALSE.
		// clear registered expressions table
		extClr();		// exman.cpp
		// clear deferred basAnc references table 1-12-91
		drefClr( "culCLEAR");		// local
		// delete user functions and (future) variables. New code 10-93, watch for bugs
		funcsVarsClear();  	/* delete user-input-defined symbol table entries,
       				   leave reserved words and entries from CULTs. cuparse.cpp. */
		/* note 11-1-95: does not clear #defines... and can't easily cuz text is preprocessed ahead,
		   may have already done #defines to next non-pp token. I'm gonna look at adding #clear. */
		// >>>> and need to clear everything we (future) put in central symbol table.
	}

	termnIf();				// get terminating ; else unget

	/* call .itf fcn.  Call this one fcn so user code can access uncleared data,
	                   to clear/default/reinit anything not handled via cult */
	F( cuf( ITF, NONDAT, 0) )	// call fcn, w args, if any; ret fatal err now

	/* NB this-level basAnc record (eg TopR) is not zeroed, just nuCult'd, so .itf
	   fcn must handle members without cult entries, non-TYSTR dm data, etc.*/

	/* re-init and re-default all data for this cult.
	   Removes all records from ref'd basAncs and from array members.
	   re-calls itf's of STAR, KDAT, and data, as at startup. */

	// does not clear TYPEs. should it ? 1-91. <--- Now DOES appear to clear types (see clearRat). 10-93.

	F( nuCult( 2, 0) ); 	// uses/alters *xSp.  Fatal error rets now.

// call the other two optional functions if present.
	F( cuf( PRF, NONDAT, 0) )	// call user fcn if any with appropriate args,
	F( cuf( CKF, NONDAT, 0) )	// ... return any fatal error to caller now

	return RCOK;		// other returns above including F macros
}		// culCLEAR
//===========================================================================
/* see if fixes bad compile in which fsj init'd in ebx, stored in ebp-0c,
   but then used at "setFsVAL:" in ebx without reloading register. 3-4-92. DID NOT FIX IT. */
LOCAL RC culDAT()	// do cul DAT case per xSp

/* returns: RCOK:    succesful
            RCFATAL: fatal error, message already issued, terminate session
            other:   non-fatal error, message issued, rest of input statement
                     has been skipped, continue compilation. */
{
// get '=' next
	if (tkIf(CUTEQ)==0)   			// if not = next
		return perNx( (char *)MH_S0231);  	// "'=' expected"

// initial checks
	CULT *c = xSp->c;				// pointer to tbl entry for cmd to compile
	if (c->IFLAGS & BEEN_SET)
		return perNx( (char *)MH_S0232, 			// "Invalid duplicate '%s =' or 'AUTOSIZE %s' command"
			(char *)c->id, (char *)c->id);

	/* NOTE THAT current token is now neither beginning nor end of statement,
	   so perNx (eg in datPt) won't skip next statement, yet
	   unToke-perNx (eg in cuf) won't stick in same place. */

// if unset, set basAnc front .fileIx/.line
	// so non-culRATE-created objects eg Top will have file/line of 1st mbr=.
	if ( ((record*)xSp->e)->fileIx == 0 )
		curLine(	0,				// get fileIx, line, etc for errmsg, cuparse.cpp.
			&((record*)xSp->e)->fileIx,	// rcvs index of file name: see ancrec:getFileName
			&((record*)xSp->e)->line,	// rcvs line number
			NULL, NULL, 0 );

// point to storage (of first element if array): set xSp->p & ->sz / return code if error
	RC rc;
	CSE_E( datPt() )				// uses xSp->c. zero's xSp->j. local, below.
	// return now on error.  datPt skipped to next statement.

#if 0	// BCC32 4.0 compiled bad code for fsj: used from ebx at "setFsVal" after register altered.
x	// replaced "*fsj" with "xSp->fs[xSp->j]" in rest of this fcn, and deleted "fsj++" where xSp->j++'d, as noted.
x	// didn't fix it; fixed when compiled all with -OD, -rd, and NO fastcalls. 3-4-94.
x    UCH *fsj = xSp->fs;				// fetch field status byte ptr.  Incremented for successive elements.
#endif

	// error if FROZEN.  for ARRAY, 1st element FsFROZ bit used for all elements.
	if (xSp->fs[xSp->j] & FsFROZ)
		return perNx( (char *)MH_S0233, (char *)c->id );  	// "'%s' cannot be specified here -- previously frozen"

	USI f = c->f;				// fetch flags ALL_OK etc: may be altered after 1st array element

	for ( ; ; )				// loop array elements if any and comma after input (else breaks below)
	{

		// call pre-input fcn if this datum has one
		CSE_E( cuf( PRF, RGLR, 1) ) 		/* if nonNULL, call xSp->c->prf with args.
    					   return any err to our caller now (cuf skipped input to nxt stmt). */

		// get and store value according to type

		USI useCl = (c->EUCL) ? (c->EUCL) : 1;  	// expr use class (1991) from .uc or as changed; default 0 to 1.
		// (all uc's are default, 1995; uc must be non-0; 1 is former cncult.h:UGEN).
		switch (c->ty)
		{
			XSTK* x;
			record *e;
			SI v;
			USI gotEvf, l;
			char ch, *s, *p;

		default:
			return perNx( (char *)MH_S0234, (INT)c->ty, (char *)c->id, c );	//"cul.cpp:culDAT: Bad .ty %d in CULT for '%s' at %p"

		case TYREF:				// deferred-resolution reference to basAnc record, or "all"/"sum" per flag bits
			// input is name of record; drefRes() stores record subscript at RUN

			// get Identifier, Quoted text, or constant string eXpression. local fcn.
			rc = ganame( f,		// allow "all", "sum", etc per CULT flags; ALL_OK cleared after 1st ARRAY elt.
					NULL,		// use xSp->c->id in errMsgs
					&p );		// receives dm string ptr.  ganame gets terminator.
			if (rc==RCOK)		// if ok record name, not special word or error
			{
				drefAdd(p);        	//   add dref (Deferred REFerence) to basAnc record named *p; uses xSp. local.
				gotEvf = EVEOI;		//   value will be stored at end of input, do not set FsSET now.
			}				//   (drefAdd uses p without IncRef; dref stuff later dmfrees it).
			else
				gotEvf = 0;		// no variablity for RC_ALL etc
			goto haveFieldRc;		// go do other rc's, field status

		case TYFLSTR:		// string or float: get value/NANDLE and type to VALNDT structure. 11-91 for reportCol.

#define VDP ((VALNDT*)xSp->p)
			rc = xpr( c->ty, xSp->fdTy, c->evf, useCl, 0, NULL, (NANDAT*)&VDP->val, &VDP->ty, &gotEvf);	// below
			goto haveFieldRc;									// set Fs per rc, break/return

		case TYDOY: 	// day of year.  See datPt(), xpr(). fall thru.  Uses SI expr (incl month names: see cuparse.cpp).

		case TYID:
		case TYCH:
		case TYNC:
		case TYLLI:
		case TYFL:
		case TYSTR:
		case TYSI:

			/* compile expression: stores value if constant & known now, else saves code & stores "NANDLE" (see exman.h).
			   Does errMsgs, terminator.  Also uses: defTyping, xSp->b, xSp->i, xSp->c->fn. */

			rc = xpr( c->ty, xSp->fdTy, c->evf, useCl, f, NULL, (NANDAT*)xSp->p, NULL, &gotEvf);	// below
haveFieldRc:		// preceding similar cases joins here; rc, gotEvf set.
			if (rc != RCOK)
				switch (rc)		// if exception
				{
					SI specVal;
					// special values, entered with enabling c->f bits on: store code (cnglob.h) in place of value.
				case RC_SUM:
					specVal = TI_SUM;
					goto special;		// "sum" (eg rpZone)
				case RC_ALL:
					specVal = TI_ALL;
					goto special;		// "all" (eg rpZone, or list of records)
				case RC_ALLBUT:
					specVal = TI_ALLBUT;	// "all_but" in list of records
					// goto special .. fall-thru;		
special:
					if (xSp->sz==2)
						*(SI *)xSp->p = specVal;	// negative value is not a possible record subscript
					else				// assume a 4-byte field
						*(LI *)xSp->p = (LI)specVal;    // sign extend to yield NAN for negative codes. 3-92.
					// delete any existing table entry for reference from this field, in case ALTER and TYREF
					drefDelFn( xSp->b, xSp->i, c->fn+xSp->j);	// array elts are successive field #'s
					break;					// special rc may be used again at ARRAY loop end

					// other return is error, ganame/xpr issued message and skipped input to next statement
				default:
					xSp->fs[xSp->j] |= FsERR;
					return rc;   	// suppress addl msgs for field
				} //switch(rc)
			if (gotEvf==0)			// if expression parsed was a constant, it is already evaluated and stored.
setFsVAL:
				xSp->fs[xSp->j] |= FsVAL;	// say 'value stored, can be accessed for checking' in fld status byte
			else
				xSp->fs[xSp->j] &= ~FsVAL;	// expr entered: clear flag: insurance eg re leftover flag in TYPE or LIKE 3-92
			break;				// FsSET is set after break

			// be sure nothing permits any kind of expr for [TYCH or] TYQCH as wouldn't be decoded when later eval'd.

		case TYQCH:	// historical "quick choice" data type (decoding string in .b)

			// get Identifier, Quoted text, or string eXpression. local fcn.
			CSE_E( xpr( TYID,			// get string, assume quotes around non-reserved undefined words
				xSp->fdTy, 			// field type (or use 0 here?? 2-91)
				0, 0,      			// no evfOk, no useCL: require constant.
				0,	        		// for flags for "all" and "sum" and numbers
				NULL,				// use xSp->c->id in errMsgs
				(NANDAT *)&p,		// receives string ptr. xpr gets terminator.
				NULL, NULL ) )     	// don't want type and evf: know would be TYSTR and 0.

			// decode using string of form "value0/value1/value2 ..."
			l = (USI)strlen(p);
			v = 0;				// value (count of /'s)
			for (s = (char *)c->b; ; )
			{
				ch = *s;					// char of decode string
				if (ch==0)    					// if end string, not found
				{
					xSp->fs[xSp->j] |= FsERR; 			// say ermsg'd for field: suppress "no ___ given". 12-91.
					return perNx( (char *)MH_S0235, 		// "Value '%s' for '%s' not one of %s"
					p, (char *)c->id, c->b );	// errMsg, skip to nxt stmt, cuparse.cpp
				}
				if (isspaceW(ch))
				{
					s++;
					continue;			// skip whitespace
				}
				if (ch=='/')            		// / separator means "next value"
				{
					s++;
					v++;
					continue;		// extra /'s skip values
				}					// eg leading / makes 1-based
				if (_strnicmp( s, p, l)==0)      	// if matches
				{
					ch = *(s+l);   			// decode string char after
					if ( ch=='\0' || ch=='/'      	// if end, separator, or space
					|| isspaceW(ch) )		//  then whole thing matched
					{
						*(SI *)xSp->p = v;		// store value
						break;    			// done
					}
				}
				while (*s && *s != '/')      		// advance to next /
					s++;
			}
			dmfree( DMPP( p));      	// free input string storage
			goto setFsVAL;      		// go set "value stored" bit in field status byte, then break. 12-14-91

		case TYIREF:		// reference to already-defined basAnc record
			// input is name of record; record subscript is stored.

			/* TYIREF not TYREF typically used in ownTi's of basAncs that may own
			   records in others basAncs, so set during input, in case used during
			   input re ambiguity resolution.  Not positive needed.  2-91. */
			// Probably no uses left that are not RDFLIN/NOINP. later 2-91.

			// get Identifier, Quoted text, or const string eXpression. local fcn.
			CSE_E( ganame( 0,	// disallow "all" and "sum" **** implement here like TYREF if needed, 1-92
				NULL,   	// use xSp->c->id in errMsgs
				&p ) )  	// receives dm string ptr.  gets terminator.

			// search specified basAnc for record with that name.  If > 1 found, use the one whose owning RATE is open per xStk.
			if (ratLuCtx( (BP)c->b, p, c->id, &e) != RCOK)	// local.  if not found...
			{
				xSp->fs[xSp->j] |= FsERR;		// say msg issued for field (eg suppress "no ___ given" if RQD)
				return perNx(NULL);				// skip input to nxt stmt (ratLuCtx issued message).
			}
			*(SI *)xSp->p = e->ss;      		// found.  Store subscript.

			// also treat as deferred reference so verified still good at RUN -- in case record deleted later
			drefAdd(p);				// add dref to basAnc entry named *p; uses xSp. local.
			//   (drefAdd uses p without IncRef; dref stuff later dmfrees it).
			// issue warning if in statement block for another entry of same basAnc
			for (x = xStk;  x < xSp;  x++)
				if ( (x->cs==3||x->cs==4)  &&  x->b==c->b  &&  x->e != e)
					pWarnlc( (char *)MH_S0236, 					// "Statement says %s '%s', but it is \n"
					(char *)c->id, e->name, x->e->classObjTx() );	// "    in statement group for %s."
			goto setFsVAL;						// go say 'value stored' in fld status bytes

		}  // switch (ty)

		// say a value has been entered for this member (possibly uneval'd expression)
		c->IFLAGS |= BEEN_SET;		// say this command seen in cult
		xSp->fs[xSp->j] |= FsSET;	// say 'value entered' in fld status byte
		// only latter triggers 'invalid duplicate' message; ?? former lasts thru type/like 1-91.
		//FsVAL set above if value already evaluated & stored, ready to check.
		xSp->fs[xSp->j] &= ~FsAS;	// say not being autosized 6-95

		// call check-function if this datum has one and value stored.   Call MOVED after flag sets by rob, 12-14-91.
		if (xSp->fs[xSp->j] & FsVAL)	/* don't call if set to expr.  Thus fcn needn't check for nandle
	       				   (and exprs now 12-91 possible in SI's with no indicator stored).
       					   All data should be checked again at run from run's ckf.*/
		{
			rc = cuf( CKF, RGLR, 0);	// call fcn if any w args, handle return.
			if (rc==RCFATAL)
				return RCFATAL;    	// only fatal error codes are returned to caller
			else if (rc)  		// other error return
				xSp->fs[xSp->j] |= FsERR;	// say field has had error message, to suppress some duplicate messages 12-91
		}

		// array element list endtest/increment
		if (!(c->f & ARRAY))				// insurance only, cuz comma allowed only if ARRAY,
			break;					//   and xSp->p, j currently not used by caller 6-92.
		xSp->j++;					// next array element, for next value or termination: incr subscript
		IncP( DMPP( xSp->p), xSp->sz);			//   point next element
#if 0	// BCC32 4.0 compiled bad code for fsj; recoded to xSp->fs[xSp->j] as noted above.
x		fsj++;   					//   next field status byte (for most Fs flags; some use 1st only)
#endif

#if 1// 9-25-92 attempt to allow ALL_BUT a,b,c without comma after ALL_BUT even tho a is stored as separate array element.
		// fails now cuz xpr calls exPile b4 looking for ALL_BUT, and next precedence not low --> "operator missing" error message.
		// next effort would be to look ahead for ALL_BUT,ALL,etc (as IDs or "texts") in xpr before calling exPile.

		if (f & ALL_OK && rc==RC_ALLBUT)			// if "ALL_BUT", attempt to allow following list without comma
		{
			if (prec < PRCOM)				//  array done if verb, ; ) ] etc next
				break;					//  comma (passed by xpr) or other (not passed): continue array
		}
		else if (tokTy != CUTCOM)			// if this element was anything but "ALL_BUT"
			break;		      			//  array done if no comma next (comma allowed in xpr() only if ARRAY)
#else//works, requiring undersired comma after ALL_BUT: ALL_BUT,a,b,c
x		if (tokTy != CUTCOM)      			// done if no comma after input (comma allowed in xpr() only if ARRAY)
x			break;
#endif
		f &= ~ALL_OK;					// "all" and "all_but" (also sum??) only valid at start of list
		if (xSp->j >= xSp->arSz-1)						// check array for full; leave room for terminator
			return perNx( (char *)MH_S0237, INT(xSp->arSz-1), (char *)c->id);  	// "Can't give more than %d values for '%s'"
	}

// terminate array input
	if ( c->f & ARRAY
	&&  xSp->j < xSp->arSz-1 )				// insurance
	{
		memset( xSp->p, 0, xSp->sz*(xSp->arSz - xSp->j - 1));	// fill all elements after last entered with 0's
		xSp->fs[xSp->j] &= ~(FsSET|FsVAL|FsERR);			// clear field status for terminating element (insurance)
	}
	return RCOK;				// ok completion
}		// culDAT

// datPt before many deletions is in cul.ifo, 1-23-91.
//===========================================================================
LOCAL RC FC datPt()		// point to DAT and KDAT data storage per xSp->c, e, fs0

// sets: xSp->p0, ->p, ->sz, ->arSz, ->fs, ->fdTy.

/* returns: RCOK:     succesful -- continue
            [RCFATAL:  fatal error, message already issued, terminate session]
            other:    expected only if [arg is 1 or] CULT is bad:
                      non-fatal error incl bad tables, msg issued, .p NULL, rest of stmt has been skipped.  Continue compile. */
{
	CULT *c;
	SI ty;
	USI sz, dt;

	c = xSp->c;				// fetch current CULT entry ptr
	xSp->p = NULL;			// values to return if error
	xSp->fs = NULL;			// ..
	xSp->sz = 0;  			// ..
	xSp->arSz = 1;			// array size is 1 unless validly specified otherwise
	xSp->j = 0;				// array subscript is 0 til CALLER changes it
	xStkPt();				// be sure all (basAnc-record-related) ptrs current

// fetch field type -- used to specify data type and limit type to exman.cpp
	xSp->fdTy = xSp->b->fir[ c->fn ].fi_fdTy;		// from tbl whose source code is made by rcdef.exe

// get array size if specified and check for inconsistent flag combinations
	if (c->f & ARRAY)
	{
		ULI arSz = (ULI)c->p2;				// fetch array size from multi-use member. keep 32 bits til checked.
		if (c->f & (ITFP|PRFP))				// this imply other .p2 uses
			return perNx( (char *)MH_S0238);		/* "Internal error: cul.cpp:datPt(): "
							   "bad CULT table entry: flags incompatible with ARRAY flag" */
		if (arSz <= 0)
			return perNx( (char *)MH_S0239 );		/* "Internal error: cul.cpp:datPt(): "
				                           "bad CULT table entry: ARRAY flag, but 0 size in .p2" */
		if (arSz > 128)					// arbitrary limit to catch pointers; increase as needed
			return perNx( (char *)MH_S0240, arSz);	/* "Internal error: cul.cpp:datPt(): "
							   "bad CULT table entry: ARRAY size 0x%lx in p2 is too big" */
		xSp->arSz = (USI)arSz;				// ok, truncate and store
	}

	/* reconcile cult .ty and field's dtype, and determine data size.
	   In various cases either ty or dtype may be able to rule the input conversion method for field... 2-91. */

	ty = c->ty;							// CULT table entry type
	dt = sFdtab[ xSp->fdTy ].dtype;		// field's data type
	sz = 2;					// preset sz to most common value
	if (dt & DTBCHOICB) 			// modern (1991) choice field data types
	{
		if ( ty != TYCH  			// cult type must by TYCH
		&& ty != TYQCH )			// or TYQCH (cult decoding overrides)
			goto badTynDt;
		// sz = 2;	already set		// size is 2 bytes
	}
	else if (dt & DTBCHOICN) 		// choice/number field data types (1992)
	{
		if (ty != TYCH  &&  ty != TYNC) 	// cult type must by TYCH (choice only, runtime vbl) or TYNC (choice or float, vbl)
			goto badTynDt;
		sz = 4;				// size is 4 bytes (float; choice stored as NAN in hi word)
	}
	else 			// dt bits ok.  do by ty.
	{
		USI dtMustBe;
		switch (ty)	// break with sz and dtMustBe set
		{
		case TYQCH:
			dtMustBe = DTSI;
			break;  		// or any CHOICB, handled above
		case TYIREF:
		case TYREF:
			dtMustBe = DTTI;
			break;  		// sz = 2 preset
		case TYDOY:
			dtMustBe = DTDOY;
			break;
		case TYSI:
			if (dt==DTBOO)      dtMustBe = dt;		// accept DTBOO or DTSI for TYSI, 3-92
			else if (dt==DTUSI) dtMustBe = dt;		// or DTUSI, 5-95, for Top.workDayMask.
			else                dtMustBe = DTSI;
			break;

		case TYLLI:
			dtMustBe = DTLI;
			sz = 4;
			break;

		case TYFL:
#if 0
x // incomplete attempt to accomodate DBL, 7-13
x			if (dt == DTDBL)
x			{	dtMustBe = dt;
x				sz = sizeof( DBL);
x			}
x			else
x			{	dtMustBe = DTFLOAT;
x				sz = 4;
x			}
#else
			dtMustBe = DTFLOAT;
			sz = 4;
#endif
			break;

		case TYID:
		case TYSTR:
			dtMustBe = DTCHP;   				// others dt's too?
			sz = 4;
			break;

		case TYFLSTR:
			dtMustBe = DTVALNDT;
			sz = sizeof(VALNDT);
			break;	// string or float 11-91

		case TYNC:
		case TYCH:
			goto badTynDt;				// valid TY's for which valid uses don't get to this switch

		default:
			return perNx( (char *)MH_S0241, 	// "cul.cpp:datPt(): Unrecognized CULT.ty %d in entry '%s' at %p"
			(INT)ty, (char *)c->id, c );
		}
		if (dt != dtMustBe)								// if dt not consistent with ty
badTynDt:
			return perNx( (char *)MH_S0242, (UI)ty, (UI)dt, (char *)c->id, c );
		// "cul.cpp:datPt(): Bad ty (0x%x) / dtype (0x%x) combination in entry '%s' at %p"
	}
	xSp->sz = sz;			// ok, store size.

// size consistency check 2-92

	if (GetDttab(dt).size != sz)			// get field's data type's size using srd.h fcn 3-1-94
		return perNx( (char *)MH_S0243,			/* "Internal error: cul.cpp:datPt(): data size inconsistency:
       							    size of cul type 0x%x is %d, but size of field data type 0x%x is %d" */
					  (UI)ty, (INT)sz, (UI)dt, (UI)GetDttab(dt).size );

// data and field status locations

	xSp->p = 							// current element location same, til caller increments it
		xSp->p0 = (char *)xSp->e + xSp->b->fir[c->fn].fi_off;	// element [0] location: record base, + mbr offset, from FIR table
	xSp->fs = xSp->fs0 + c->fn; 			// field status byte basic locn is base + field number
	// for most Fs bits, caller must subscript fs by array elt #.
	return RCOK;
}			// datPt

//===========================================================================
LOCAL RC FC ckiCults()	// check / initialize CULTs 1-21-91

/* Displays msgs for nested entry names that cd make implicit ENDs ambiguous.  Sets RFNOEX and RFINP flags in each basAnc.
   ...
*/

// uses xSp->cult, xSp->b; adds xStk levels.

// call at top level for complete check.
{
	// if we can make xStk full in xnxC fatal, can drop other checks? 1-91 <<<<<<

	for (CULT* c = NULL; xnxC(c, nxropIGNORE_BEEN_HERE); ) 		// loop over all members of all cults
	{
		// flag input basAncs as input basAncs and to suppress expression evaluation.
		// because: 1) speed; 2) keep NANDLEs in input records in case used in a later run.
		// (Run basAncs are separate from input basAncs & have no CULT entries.)
		// (need only be in basAncs loop, not entries loop)
		xSp->b->ba_flags |= RFNOEX  	// flag causes exman.cpp:exWalkRecs to not look for expr references in this basAnc.
		                  | RFINP;		// flag tells cuparse.cpp:probe that this is an input basAnc.

		// look for ambiguities re implicit ENDs: id of a group also used in any table nested within the group
		for (XSTK* x = xStk; x < xSp; x++)			// loop over nestING cults
			for (CULT* cc = x->cult; cc->id; cc++)		// entries of a nestING cult
				if ( cc != x->c				// skip entry that got here!
				&& cc->cs != STAR			// skip * entries: all same!
				&& _stricmp( cc->id, c->id)==0		// msg if same id except ...
				&& !(cc->cs==RATE && c->cs==RATE		// ok if RATEs w same cults
				&& cc->CULTP2==c->CULTP2) ) 	// ...eg print in zone & top
					err( PWRN,				// display internal error msg
					(char *)MH_S0244,  			// "Ambiguity in tables: '%s' is used in '%s' and also '%s'"
					(char *)c->id,  (char *)(xSp-1)->c->id,
					x > xStk  ?  (char *)(x-1)->c->id  :  "top" );
		// continue at ambiguity: allow use anyway.

		// clear flags of entry c here ? (1990)

		// other checks & init? (1990)
	}
	return RCOK;			// this was missing!  added 2-92.
}		// ckiCults

//===========================================================================
LOCAL void FC finalClear()		// clear input data, for cul(4).

// clear all to free up dm for final RUN, and at exit to help find unfree'd DM.

// does NOT clear probed basAncs (RFPROBED flag on) as needed during run.  Add option (cul(5)?) to do that at program exit?

// does NOT clear expression table: to do so call exman.cpp:extClr(), but not before run complete and all calls to exEvEvf done.

// does not free types basAncs (small amount of near dm not recovered)
{
	void *p;
	BP b;

	if (xSp != xStk)			// nxRat etc work as desired here only at top level.
		err( PWRN,				// display internal error msg
		(char *)MH_S0245,   		// "cul.cpp:finalClear() called with xSp %p not top (%p)"
		(void *)xSp, (void *)xStk );	// casts are to make far.

// free all DM data: TYSTR strings, add any other general DM data types
	// believe duplicates C++ destructors harmlessly 7-92; maybe remove when destructors fully implemented.
	for (p = NULL;  xnxDatMbr( 1, &p); )    	/* loop over all data in all basAncs, incl TYPEs, incl bad/deleted recs (1).
    						   uses/adds to xStk. below. only use 3-92. */
	{
		if (!(xSp->b->ba_flags & RFPROBED) )	// if its basAnc has not been 'probed' (would be accesed during run) 12-91
			if ( xSp->c->ty==TYSTR					// if a string datum
			||  xSp->c->ty==TYFLSTR && ((VALNDT *)p)->ty==TYSTR )	// or string-or-float now set to string
				// 11-95: historically this hasn't crashed on nans -- already cleared elsewhere? adding protection anyway...
				if (ISNANDLE(*(void **)p)) 				// if UNSET, ASING, expr handle, etc (cnglob.h)
					*(void **)p = NULL;					// just remove value, don't crash in dmfree, 11-95
				else							// else is heap ptr or NULL
					dmfree( (void **)p);	// free it if its ptr at *p non-NULL. dmpak.cpp
	}									// note assumption that VALNDT.val is 1st member.

// basAnc loop 1: free all record blocks of input basAncs
	for (b = 0; nxRat(&b); )		// loop all basAncs ref'd in CULTs, incl nesting & TYPEs sub-basAncs
	{
		if ( b != xStk->b 		// do not free top (static) basAnc: insurance
		 &&  !(b->ba_flags & RFPROBED) )	// do not free input basAncs which are 'probed' by compiled code (set in cuparse.cpp) 12-91
		{
#if defined( BUG_COLTYPECOPY)
			if (strMatch( b->what, "ReportCol type"))
				printf( "ReportCol type free\n");
#endif
			b->free();			// eliminate all records / free dm storage
		}
	}

#if 0	// 2-8-91: NO as converting to NEAR ptrs. <--- basAnc::delete handles the NEAR ptrs, 10-93.
// and NO without checking on need to un-register. <--- not registered 10-93.
// if need to restore, put a dedicated basAncFree in ratpak.cpp.
x/* basAnc loop 2: free subsidiary basAncs for types (small -- need dubious).
x	As of 1-22-91, nxRat is organized such that if this done
x	in above loop, would never get to ratFree-ing the TYPEs subRat. */
x    for (b = NULL; nxRat(&b); )	// loop all basAncs again
x       dmfree( DMPP( b->tyB));	// free TYPEs sub-basAnc if any. dmpak.cpp.
x
#else // 10-93 attempt to do it cuz of cleanup and near heap clearing on reentry.

// basAnc loop 2: free subsidiary basAncs for types. These basAncs are in (near) heap and alloc'd only when needed.

	/* notes: 1. not done in loop 1 as loop body would not be executed for the tyR (as nxRat now organized 10-93).
	             a. but if we proved basAnc delete destroyed records, would not need to execute loop body.
	          2. only safe cuz tyR's aren't registered (basAnc d'tors have no unregister 10-93)
	          3. small, reusable, need here dubious, but for cleanup 10-93 need to work out how to free ALL near heap storage.*/

	for (b = NULL; nxRat(&b); ) 	// loop all basAncs again
		// types aren't probed, right?
		if (b->tyB)			// if basAnc has types sub-basAnc, clear its types too.
		{
			/*b->tyB->free();		   destroy type records: redundant cuz: 1) done in loop 1 above;
						   2) c'tor called by delete supposed to do it, too.*/
			delete (basAnc *)b->tyB; 	// destroy and free the types sub-basAnc in near heap. cast makes ptr far.
			b->tyB = 0;			// 16-bit-NULL the pointer to deleted object
		}
#endif

// clear deferred basAnc ref's table.  Table itself not in dm (otta be?) 1-21-91.
	drefClr( "finalClear");			// below

	// expression table is NOT cleared here as needed during run!

}		// finalClear();

// xStkPt before STR deletions is in cul.4, cul.ifo 1-23-91.
//===========================================================================
LOCAL void FC xStkPt()	// repoint basAnc-record-based items in context stack

// use to init ptr in new frame, or when realloc might have moved a basAnc

// does NOT recompute p, fs: never used after nested item, right?

/* returns: RCOK:     succesful
            [RCFATAL:  fatal error, message already issued, end session.]*/
{
	XSTK* x;

	for (x = xStk;  x <= xSp;  x++)
	{
		switch (x->cs)
		{
		default:
			err( PWRN,				// display internal error msg
			(char *)MH_S0246, (INT)x->cs );	// "cul.cpp:xStkPt(): bad xStk->cs value %d"
			break;

		case 4:	// basAnc record for DEFTYPE
		case 3:	// basAnc record: point basAnc record per subscript
			x->e = &x->b->rec(x->i);
			x->fs0 = (UCH *)x->e  +  x->b->sOff;
			break;
		}
	}
}		// xStkPt

//===========================================================================
LOCAL RC ganame(

	// get identifier, quoted text, or const string expr, or optional special word, for an object name, and terminator

	USI f,		// flag bits: SUM_OK: accept "sum" (returns RC_SUM); ALL_OK, [ELECTRIC_OK,] etc likewise (see xpr).
	char *what,		// text for errmsgs (eg ttTx) or NULL for xSp->c->id
	char **pp )		// receives NANDLE or ptr to string in dm
/* also uses: if 'what' is NULL: xSp->c->id (re compile errMsgs) */

/* returns: RCOK:   normal success, tokTy = CUTCOM if comma followed it and f & ARRAY on.
            RC_SUM, RC_ALL, RC_ALLBUT, [RC_ELECTRIC]: returned if special word gotten and enabling flags on.
            other:  error, message issued, perNx done. */
{
	/* link to a fir in srfd.cpp to get field type of an ANAME, partly to
	   excercise new fields limit code 2-92 -- later just check length here?*/

	char *p;
	RC rc = xpr( TYID,				// TYID: string, implied quotes on unreserved undeclared words
		sfirSFI[ SFI_NAME].fi_fdTy,	// get field type for an ANAME field so exman.cpp will check string length
    								// (we have no defines for fld types: not invariant: data\fields.def
									// is rearrangable & product dependent) 2-91. Declared in rccn.h.
		0, 0,						// 0 evfOk, useCl: require constant value
		f,							// pass flags for "all" and "sum", etc 1-92
		what, 						// pass arg thru
		(NANDAT *)&p,				// receives pointer to constant string value
		NULL, NULL ); 				// don't want type and evf

	if (rc==RCOK)					// if ok and not special, check characters in name, 4-92.
	{
		// allow high graphics characters 129-255.  Should we bother to disallow 127-128?  Embedded spaces ok.
		// remove leading and trailing blanks from name
		while (p[0]  &&  strchr( " \t", p[0]))  strcpy( p, p+1);			// copy back over leading spaces and tabs
		USI len = (USI)strlen(p);
		while (len > 0  &&  strchr( " \t", p[len-1]) )   p[--len] = '\0';	// truncate trailing spaces and tabs
		// in names, disallow control characters 1-31: escape, form feed, newline, etc
		for (SI i = 0;  i < len;  i++)
			if ( p[i] < ' ')							// disallow control chars
				if (p[i] > 0)							// allow 129-255 even if compiler extends sign
					return perNx( (char *)MH_S0247, (UI)p[i]);			// "Illegal character 0x%x in name"
		// if caret not useful, consider adding to errMsg, before the word "name": "what ? what : xSp->c->id".
		// require non-0 name length
		if (len==0)
			return perNx( (char *)MH_S0248); 				// "Zero-length name not allowed"
		// if caret not clear enuf also show what name is of.
		// return pointer
		*pp = p;
	}
	return rc;				// RCOK, RC_SUM etc per f, error code.  Also error returns above.
}			// ganame
//===========================================================================
LOCAL RC xpr(   	// our local expression compiler interface / checker

	SI ty, 		// desired cuparse.cpp type: TYLLI TYFL TYSTR TYSI TYFLSTR or TYCH TYNC (fdTy req'd)
	// or TYID (returns TYSTR).  Also accepts non-cuparse type: TYDOY.
	USI fdTy,		// 0 (FDNONE?) or rec def fld type: dtype (for choices), units (for floats) and limit type are used.
	USI _evfOK, 	// permissible eval frequency bits, 0 for constant.
	USI useCl,		// "use class" for selective expr evaluation 3-91
	USI f,			// flag bits:
					// ARRAY: enables comma as a terminator (CUTCOM returned in tokTy)
    				// these options should be used with TYID and evf=0:
					//    SUM_OK (cul.h): accept "sum" (returns RC_SUM).
					//    ALL_OK (cul.h): accept "all" (returns RC_ALL)
					//    ARRAY and ALL_OK: also accept "all_but" (returns RC_ALLBUT)
                    //    [ELECTRIC_OK (cul.h): accept "all" (returns RC_ELECTRIC)]
	char *what,		// token text for errMsgs or NULL to use xSp->c->id.
	NANDAT *pp,		// receives result (dm ptr for TYSTR), or "NANDLE" (see exman.h) if a run-time expr.
	USI *pGotTy,  	// NULL or rcvs cuparse type found, eg TYFL or TYSTR for ty=TYFLSTR, TYSI for TYDOY. 11-91.
	USI *pGotEvf )	// NULL or rcvs variability gotten -- 0 constant (already stored), EVEOI, EVFHRLY, etc.

/* also uses:  if 'what' is NULL: xSp->c->id (re compile errMsgs)
   if _evfOK is non-0 and expr non-constant:   defTyping, xSp->b, xSp->i, xSp->c->fn (run errMsgs,exman.cpp).*/

/* returns: RCOK:   normal success, tokTy = CUTCOM if comma followed it and f & ARRAY on.
            RC_SUM, RC_ALL, RC_ALLBUT, [RC_ELECTRIC]: returned if special word gotten and enabling flags on.
            other:  error, message issued, perNx done. */
{
	SI tem;
	USI gotTy;
	RC rc;


	if (what==NULL)		// if user gave no text, use id of current
		what = xSp->c->id;	// ... CULT in msgs of form "... after <what>"

// check for no expression nor ; at all (exPile wd gobble next statement)
	tem = nxPrec();			// prec of next token (unToke's)
	if (tem >= PRVRB && tem <= PRTY)	// if verb, verb-like, or type next
	{
		toke();				// to position ^
		perlc( scWrapIf( "value expected after '%s',",
				" but '%s' is the initial keyword of another statement%s",
				"\n    ", getCpl()),
				what, cuToktx,
				basAnc::findAnchorByNm( cuToktx, NULL) ? "" : "\n    (Or did you omit the @ necessary to use a member's value?)" );
		unToke();
		return RCBAD;				// the verb/type is left in the input.
	}

// get cuparse type for certain cul types
	if (ty==TYDOY)		// date: use integer expr.
		ty = TYSI;		// cuparse accepts month-day in all SI exprs; later add 'goal-is-date' arg to enable that?

// compile expression

#if 1	// 9-11-2017
	if (_evfOK & EVPSTIVL)		// if post-interval is acceptable
		_evfOK |= EVENDIVL;		//   then end-interval OK too
#endif

	rc = exPile(		// exman.cpp. only call 11-91.
		-1,				// say parse to ; or CUTVRB, etc
		ty,				// desired cuparse type
		fdTy ? sFdtab[fdTy].dtype : 0,	// 0 or choice type (DT___ define, dtypes.h) for TYCH, TYNC
		fdTy,				// 0 or field type
		_evfOK,			// permissable evaluation frequency
		useCl,			// use Class for non-constants
		what, 			// id to insert in error messages
		defTyping,		// destination info, for runtime errMsgs ...
		xSp->b,			// ... not used if _evfOK 0 or expr constant ...
		xSp->i, 		// ... don't matter for non-field iqx() calls.
		xSp->c->fn + xSp->j,	// ...
		pp,				// receives result or NANDLE
		&gotTy,  		// rcvs type of result
		pGotEvf );		// NULL or rcvs evf of result
	if (rc!=RCOK)		// if error (exPile has perNx'd)
		return rc;		// return it now

	/* check terminator: cuparse.cpp:exOrk (from exman.cpp:exPile) has accepted and passed:  ; , ) ] "default"
	                                                          or accepted and ungotten:  verb, verb-like word, eof
	   here reject those that are passed that we do not like. */

	if ( tokTy==CUTCOM && !(f & ARRAY)						// comma ok if 'array' flag, 3-92
	 ||  tokTy==CUTRPR  ||  tokTy==CUTRB  ||  tokTy==CUTDEFA )
		perNx( (char *)MH_S0249 );				// issue msg, scan to eof, verb, or after ';'.
	// "expected ';' (or class name, member name, or verb)"

// check returned value for reserved words 'sum' and 'all' 1-92.  rc=RCOK here.
	if (gotTy==TYSTR)				// if string found
	{
		char *p = *(char **)pp;			// fetch result (pointer for TYSTR)
		if (!ISNANDLE(p))			// if constant, not expression, found (don't allow exprs and sum/all in same call!)
			if (!_stricmp( p, "sum"))
				if (f & SUM_OK)  rc = RC_SUM;
				else rc = perNx( (char *)MH_S0250);   	// "'SUM' cannot be used here"
			else if (!_stricmp( p, "all"))
				if (f & ALL_OK)  rc = RC_ALL;
				else rc = perNx( (char *)MH_S0251);   	// "'ALL' cannot be used here"
			else if (!_stricmp( p, "all_but"))
				if (f & ALL_OK && f & ARRAY)  rc = RC_ALLBUT;
				else rc = perNx( (char *)MH_S0252);	// "'ALL_BUT' cannot be used here"
		if (rc)					// if sum or all (whether error or ok)
			dmfree( DMPP( *pp));			// discard the string
	}

	if (pGotTy)  *pGotTy = gotTy;		// return type if caller wants it

	return rc;
}			// xpr
//===========================================================================
LOCAL void FC termnIf()

/* get (pass) optional non-expression statement terminator.
   Issues error message only if next thing not  ;  ,  verb, or eof. */
{
// accept and pass semicolon.  cuparse:cutok.cpp.  CUT___: cutok.h.
	if (toke()==CUTSEM)  	// get token (set & ret tokTy), test for ;
		return;			// ok. ; or , gobbled.

// accept low-precedence things and leave in input
	if (prec < PRSEM)		// if eof, verb, verb-like word, type
	{
		unToke();  		// un-gobble (cuparse.cpp): leave token in input
		return;			// ok
	}

	/* other next token (possible expression constituent, misspelled verb, etc):
	   issue message and skip past next ; or to verb or eof */
	perNx(				// msg and pass input to ; or b4 verb or eof (cuparse.cpp)
		tokTy==CUTID			// unidentified word gets different msg text
		?  (char *)MH_S0254		// "Misspelled word?  Expected class name, member name, command, or ';'."
		:  (char *)MH_S0255 );		// "expected ';' (or class name, member name, or command)"
	// 2 additional returns above
}		// termnIf
//===========================================================================
LOCAL RC bFind(

	/* ye mind-boggling-Find: do lookup & context find for verb:
	   any plain class or member name,
	   or RATE verb (gotten here) after DEFTY/ALTER */

	/* This portion of statement parser is shared between normal scan & skip2end()
	   since do not have facility to back up 2 tokens to exit skip2end after
	   determining # ENDs needed for class name after DEFTY/ALTER. */

	SI *pcs,		/* *pcs at entry: 0: CUTVRB just gotten, must look up.
						DEFTY, ALTER: just gotten (ttTx used here),
						get following CUTVRB & look up.
	 					on return: updated from c->cs for plain verb */
	XSTK** px,     	/* rcvs xSp or pointer to less-nested xStk frame for
	    			   context in which found. caller culEndTo(x)'s.*/
	CULT **pc,			// rcvs pointer to CULT entry found
	int erOp )			// WRN normally; IGN to suppress msgs (for skip2end())
// and uses: ttTx, defTyping, xStk[].cult & .cs,

/* returns: RCOK:  *pcs is set if was 0;
		   *px is xSp level to IEND to;
   		   *pc is CULT to put in xSp->c after IENDing to *px.
	 RCFATAL:  confused, terminate compile.
     RCBAD/other:  continue compile, starting a new statement. */
{
	CULT *c;
	SI xdt;
	SI cs = *pcs;  		// fetch case; returned updated if now 0
	XSTK* x = xSp;   	// init to no context stack pops
	char *whatDoTx = ttTx;	// text for last token: "defty", "alter", "verb", etc
	const char* ms=NULL; 	// for error message text.

// get next token for class name of object to defty/alter
	if (cs)						// reg verb (cs==0) already gotten
		if (tkIf(CUTVRB)==0)					// all class & member names are "verbs"
		{
			ms = strtprintf( (char *)MH_S0256, whatDoTx);		// "Expected name of class to %s"
			goto badToWhat;
		}

// find cult entry and context level (# ends to assume) for tokTx per cs

	switch (cs)		// cases goto found with x, c, cs set to return.
	{
	default:
		perlc( (char *)MH_S0257, (INT)cs );   	// "Internal error in cul.cpp:bFind() call: bad cs %d"
		return RCFATAL;


	case DEFTY:			// DEFTY <class> <name> ...

		/* search for a RATE entry for the class name (verb) (cuTokTx).
			 If nested in a DEFTY, assume enuf ENDs to terminate it.
			 Then look in current table and its nested tables; if not found,
			 try nestING tables and their nestED tables; assume enuf ENDs if found.
			 NB can DEFTY ABOVE the level where the class would be valid.
			 CAUTION: SLOW if many nested tables!! */

		xdt = defTyping;			// defTyping flag for x level
		for (  ;  x >= xStk;  x--)		// loop curr & nestING tables, outward
			if (xdt)				// if nested in a DEFTYPE, no search,
			{
				if (x->cs==4)			// if this is DEFTYPE xStk entry,
					xdt = 0;			// start search at next tbl out.
			}
			else				// not in tables invoked in DEFTYPE
				if (deftyFind( x->cult, &c))		// search given & nestED tbls (recursive; below)
					goto found;				// x, c set.
		ms = strtprintf( (char *)MH_S0258, cuToktx);   	// "Can't define types for '%s'"
		goto badToWhat;


	case ALTER:		// ALTER <class> <name> ...

		/* search for a RATE entry for class (cuTokTx).  Look in current table,
		   then try nestING tables; assume enuf implicit "end"s if found. */

		for (  ;  x >= xStk;  x--)			// loop curr & callING tables, outward
			if (csFind( x->cult, RATE, &c))			// search table / if found
				goto found;						// x, c set.
		ms = strtprintf( (char *)MH_S0259, cuToktx, whatDoTx);   	// "'%s' is not %s-able (here)" ALSO USED in vFind below

badToWhat:  			// Error re class name after DEFTY/ALTER, 'ms' set
		if (erOp != IGN)			// suppress messages for skip2end()
			perlc(ms);				// message with file line and caret; stop RUN.

		/* Make error fatal for group-starters DEFTY and ALTER as have not found the CULT for interpreting statements in group.
		   Continuing compile at next stmt would yield spurious msgs due to wrong CULT; skip2end can't be done without CULT. */

		return RCFATAL;			// say terminate session


	case 0:		// plain verb. may be class name (RATE) or not

		// search for verb (keyword) ...
		if ( cultFind( xSp->cult, &c)		// in CURRENT table (uses cuToktx)
		|| cultFind( stdVrbs, &c) )		// else in always-defined words table
			goto cfound;			// found: set cs from c, goto found
		// search calling tables, if found assume enough "end"s.
		for (  ;  --x >= xStk; )		// loop nesting tables, outward
			if (cultFind( x->cult, &c))	// seek cuToktx / if found
			{
cfound:
				cs = c->cs;			// set cs from cult for plain verb
				goto found;			// x, c, cs set.
			}
		// unrecognized plain verb
		if (erOp != IGN)
			perlc( (char *)MH_S0260, cuToktx );  	// "'%s' cannot be given here"
		if (tkIf(CUTEQ))			// if = next: setting a member
			return perNx(NULL);		// skip past ; and continue compile
		else  				// no = next, may be context change
			return RCFATAL;			/* confused, so make fatal: don't know if starts stmt block
             					   (requiring skip2end -- and don't have CULT for skipping anyway). */

	}	// switch.  cases goto found with x, c, cs set.

// good exit
found:					// x, c, cs set.
	*pcs = cs;
	*px = x;
	*pc = c;
	return RCOK;				// many error returns above
} 			// bFind
//===========================================================================
// after problem, turned out to be elsewhere.
LOCAL SI cultFind( 	// seach CULT table for word cuToktx

	CULT *c, 		// table to search
	CULT **pc )		// receives ptr to entry if found

// function value 0 if not found (*pc unchanged)
{
	for ( ;  c->id;  c++)			// loop table entries til NULL .id
		if (_stricmp( cuToktx, c->id)==0 	// if id matches text of curr token
		&& (c->f & NO_INP)==0 )	 	// and not a no-input entry
		{
			if (pc)				// unless NULL return ptr ptr given
				*pc = c;				// return pointer to matching entry
			return 1;				// say found
		}
	return 0;					// end of table: say not found.
}		// cultFind
//===========================================================================
LOCAL SI deftyFind( 	// special CULT search for DEFTYPE

	CULT *cult,    	// table to search
	CULT **pc )		// receives ptr to entry if found

/* searches for word cuToktx in a RATE entry of given table
   or ANY table referenced by it, recursively */

// function value 0 if not found (*pc unchanged)
{
	CULT *c;

// search given table for RATE entries, set *pc and return 1 if found
	if (csFind( cult, RATE, pc))	// local, below
		return 1;

// not found yet.  Call self for each table ref'd by this table.
	for (c = cult;  c->id;  c++)  			// loop table entries til NULL .id
		if (c->cs==RATE) 				// if entry type has subtable
			if (c->CULTP2 && !(c->IFLAGS & (ITFP|PRFP)))	// paranoia
				if (deftyFind( (CULT *)c->CULTP2, pc) )   	// call self for subtable
					return 1;				// if found (*pc set), ret ok

// end of table, give "not found" return.
	return 0;
}		// deftyFind
//===========================================================================
LOCAL RC FC vFind( SI cs)

/* do word following freeze, require, delete, unset, etc:
	get verb token from input,
	find in entry of type cs (RATE, DAT, etc) in curr or hier cult,
	IEND as nec,
	set xSp->c. */

// uses ttTx for errMsgs: word "freeze" etc must be last gotten token.

// returns RCOK, RCFATAL, RCother (msgs issued).
{
	XSTK* x;
	CULT *c;
	char *id = ttTx;		// save text of preceding verb for errMsgs

// next token is name of member or object to require/freeze/[alter/]delete
	if (tkIf(CUTVRB)==0)		// all member names are "verbs"
		return perNx( (char *)MH_S0261, id);   	// "Expected name of member to %s"

// search for a DAT entry for member (cuTokTx).
// Look in current table, then, if not found,
// try nestING tables, and do enuf implicit "end"s if found.

	for (x = xSp;  x >= xStk;  x--)	// loop curr & callING tables, outward
		if (csFind( x->cult, cs, &c))	// search table / if found
		{
			F( culEndTo(x) );		// IEND to where found; ret fatal err.
			xSp->c = c;
			return RCOK;
		}

// not found
	return perNx( (char *)MH_S0259, cuToktx, id);	// "'%s' is not %s-able (here)" SECOND USE -- also used in bFind
}		// vFind
//===========================================================================
LOCAL SI csFind(	// special CULT search for Freeze/Require

	CULT *c, 		// table to search
	SI cs,		// accept only entries with this .cs
	CULT **pc )		// receives ptr to entry if found

// searches for word cuToktx in inputtable DAT entry of given table

// function value 0 if not found (*pc unchanged)
{
	for ( ;  c->id;  c++)		// loop table entries til NULL .id
		if ( SI( c->cs)==cs   		// only look at given type tbl entries
		&& _stricmp( cuToktx, c->id)==0	// if id matches text of curr token
		&& (c->f & NO_INP)==0 ) 	// and not a no-input entry
		{
			if (pc)			// unless NULL return ptr ptr given
				*pc = c;			// return pointer to matching entry
			return 1;			// say found
		}
	return 0;				// end of table: say not found.
}		// csFind

//===========================================================================
LOCAL RC CDEC perNxE( char *ms, ...)

/* parse error message and SKIP STATEMENT BLOCK:

   issues message (if ms not NULL) with input line text, caret ^ under token,
   file name, line #, and caller's ms with printf args, like cuparse:perNx,

   and returns RCSKIP2END to tell certain callers to skip to matching 'end'.*/
{
	va_list ap;

	va_start( ap, ms);		// point variable arg list after ms arg
	perNxV( 0, ms, ap);		// do error msg, skip statement. cuparse.cpp.
	return RCSKIP2END;		/* ret special value.  eg culRun tests for
    				   this value and calls skip2end() (next) */
}		// perNxE

// new 1-19-91. old skip2end is in cul.4 and cul.ifo.
//===========================================================================
LOCAL SI FC skip2end(

	// skip input to end of group at xSp->c->CULTP2 level

	/* Used to skip input after problem in RATE opening stmt --
	   when can't open the object group but can determine CULT to use.
	   Scans to ENDER or implicit end (verb found in an embedding table)
	   that matches current statement, taking nesting into account. */

	SI lastCs,	/* DEFTY (we hope) iff error occurred in a DEFTY stmt:
    		   lets us skip properly, and clear defTyping. */
	SI *pcs )	/* rcvs DEFTY or ALTER if group ended due to a DEFTY or ALTER
		   from an embeddING context; else unchanged.  (If set, caller
    		   culComp then skips toke() and cs-determining switch.) */
// and uses: xSp->c->CULTP2.

// assumes xSp->c->CULTP2 set for failed stmt but xSp++ NOT done.

/* returns 0:  to continue compile (found probable 1st statement after group);
               *pcs may have been set (see comment above).
           1:  if fatal error (confused or too many nesting levels). */
{
	SI cs, csB4, rv=1;
	CULT *c;
	XSTK* x;

	XSTK* xSpSave = xSp;			// save "real" xStk level

	/* make xStk frame for entry level.  Partial xStk entries are used here for
	   nesting here so bFind can search both skipping and embedding contexts. */
	xSp++;					// add an xStk level.
	xSp->cult = (CULT *)(xSp-1)->c->CULTP2;	// tbl of failed stmt, for bFind.
	if (lastCs==DEFTY)				// if error was in DEFTY stmt
	{
		xSp->cs = 4;				// special DEFTY xStk type: bFind may test.
		defTyping = 1;   			// global DEFTY flag: bFind() may test.
		/* At entry, may have had defTyping=1 but no xStk frame yet. Making
		   xSp->cs=4 frame here causes defTyping to be cleared on return. */
	}
	else
		xSp->cs = 3;  		// xStk entry for a RATE group, incl ALTER.

// scan until end of group or of file

	for ( ; ; )
	{
		// get token, identify verbs and words usable b4 group-starting verbs
		cs = 0;
		switch (toke())
		{
		default:
			continue;	// other than 1st word, verb, eof: continue

		case CUTEOF:
		case CUTDEOF:
			goto untokeRetOk;	// eof: stop

			/* preliminary words for group start: set cs: will cause bFind
			   to get following class name to look up to determine context
			   and thus whether preceding group is implicitly ended.
			   Coordinate changes here with culComp/culDo code changes. */
		case CUTDEFTY:
			cs = DEFTY;
			break;
		case CUTALTER:
			cs = ALTER;
			break;

		case CUTVRB:
			break;	// fall out, cs still 0.
		} // switch (toke())

		/* look verb up with same logic as culComp: searches embedding contexts
		   (xStk entries) appropriately; gets verb after DEFTY or ALTER. */
		csB4 = cs;
		switch (bFind( &cs, &x, &c, IGN))	// bFind: local, above
		{
		default:
			continue; 	// RCBAD/etc: minor err, continue scan

		case RCSKIP2END: 		// (unexpected)
		case RCFATAL:			// irrecoverable error
#ifdef DEBUG2	//cnglob.h. Message mainly helps debug skip2end; otherwise appropriate to exit with no addl msg.
			perlc( (char *)MH_S0262, cuToktx );   	// "Confused at '%s' after error(s) above.""  Terminating session."
#endif
			goto retBad;		// fatal error return

		case RCOK:
			break;		// found, cs, c, x set, fall thru
		}

		// if found in a "real" context, return to resume compile
		if (x <= xSpSave)	/* if in cult of xStk entry not made here,
	       			   implies ENDs to all xStk entries we made.*/
		{
			if (csB4)		// if DEFTY or ALTER preceded verb
				*pcs = cs;		// tell call so, as can't back up 2 tokens
			// caller will re-Toke and re bFind().
untokeRetOk:		// here to unget token & return good
			unToke();		/* unget 1 token.  In DEFTY or ALTER stmts,
          			   this is token after, gotten by bFind. */
retOk:		// here to give good return
			rv = 0;		// indicate success
retBad:		// here for fatal error return (rv preset 1)
			/* end all nested groups started here (if x < xSp, caller will
			   IEND the rest of the nested groups after re-bFind-ing). */
			while (xSp > xSpSave)		// until xStk at our entry level
			{
				if (xSp->cs==4)		// if this level was a DEFTYPE
					defTyping = 0;		// clear flag: no longer defining type
				xSp--;			// pop xStk a level
			}
			return rv;		// 0 or 1
		}

		// found in a group being skipped
		while (xSp > x)		// end groups nested in group verb found in
		{
			if (xSp->cs==4)
				defTyping = 0;
			xSp--;
		}

		// process per case returned by bFind
		switch (cs)
		{
		default:
			continue;		// most verbs: just keep scanning

		case CLEAR:		// clear: assume top lvl --> ends all groups
			goto untokeRetOk;     	// stop to do it & resume compile

		case END:
		case ENDER:  	// stmt group enders
			perNx(NULL);  		// skip rest of cmd, ';'
			// >>>> check perNx re DEFTY and other new tokens.
			// pop an xStk level, return or continue scan
			if (xSp->cs==4)		// if xStk level was for DEFTY
				defTyping = 0;	// no longer defining a type.
			xSp--;			// pop context stack
			if (xSp <= xSpSave)	/* if popped all internal lvls,
	                 		   this END matches stmt that failed
	                 		   and caused skip2end(), so return.*/
				goto retOk;		// go give good return
			continue;

		case ALTER:
		case DEFTY:
		case RATE:				// stmt group starters
			if (xSp >= xStk + sizeof(xStk)/sizeof(XSTK) - 1)
				goto retBad;		// stack full, just make silent fatal
			// make nested partial xStk entry, then continue scan
			xSp++;				// push
			xSp->cult = (CULT *)c->CULTP2;	// table per table entry bFind found: bFind uses xStk[].cult's.
			if (cs==DEFTY)			// if skipping definition of a type
			{
				xSp->cs = 4;			// special xStk type: bFind may test.
				defTyping = 1;		// global flag: bFind() may test.
			}
			else
				xSp->cs = 3;  		// xStk entry for a RATE group
			continue;

		} // switch (cs)
		/*NOTREACHED */
	}
	/*NOTREACHED */
}		// skip2end

//===========================================================================
LOCAL SI FC tkIf( SI tokTyPar)

// return nz if next token is token of specified type, else unget token.
{
	if (toke()			// get token, cuparse.cpp. sets cutok.cpp:cuToktx.
	== tokTyPar)   	// if is requested token type
		return 1;
	unToke();			// wrong type.  unget token (cuparse.cpp)
	return 0;
}		// tkIf
//===========================================================================
LOCAL SI FC tkIf2( SI tokTy1, SI tokTy2)

// return nz if next token is token of either specified type, else unget token.
{
	if ( toke()			// get token, cuparse.cpp. sets cutok.cpp:tokTy.
	== tokTy1   	// if is requested token type
	|| tokTy==tokTy2 )	// or other requested type
		return 1;		// return TRUE
	unToke();			// wrong type.  unget token (cuparse.cpp)
	return 0;
}		// tkIf2

//===========================================================================
LOCAL SI FC nxPrec()	// return precedence of NEXT token (unget it)
{
	SI tem;

	toke();		// get token, cutok.cpp, sets global 'prec'
	tem = prec;		// save precedence to return (unToke may change it)
	unToke();		// now unget the token
	return tem;
}		// nxPrec
//===========================================================================
LOCAL RC FC cuf(   	// call CULT user fcn if any, handle err ret

	WHICHFCN fcn,	// which fcn: ITF PRF CKF (enum at start of this file)

	CUFCASE ucs,  	/* case --		                 CULT for fcn,    p     p2 (arg 3)  p3
			   -ucs-   --- application ---             fcn arg 1   (arg 2)   NULL or:   ..
			   RGLR:   regular: DAT, KDAT:               xSp->c     xSp->p  xSp->e
			   NONDAT: non-data: NODAT,ENDER,RUN,CLEAR:  xSp->c     xSp->e  (xSp-1)->e
			   STARS:  star entry for RATE,MAIN:        xSpp->cult  xSp->e  (xSp-1)->e
			   CLRS:   calling (nesting) entry RATE:    (xSp-1)->c  xSp->e  (xSp-1)->e */

	SI skie )		/* 1: skip rest of input stmt on error
    			   0: do not skip: use if fully parsed or if skipping no processing on (non-fatal) error.
    			  99: documentary: does not matter for this call. */

// if fcn returns non-RCOK, errCount++'s, and if skie, skips input to next stmt.
// returns: fcn's return value, RCOK if no fcn.
{
	RC rc;
	CULT *c;
	UFCNPTR f=NULL;
	void *p, *p2, *p3;

	CULPHASE phaseSave = culPhase;	// save to restore at exit
	SI errCountSave = errCount();	// save to see if fcn increments

	XSTK* x = ucs==CLRS ? xSp-1 : xSp;	// point appropriate xStk frame

	if (ucs==STARS)			// if star entry fcn requested
	{
		c = x->cult;			// use first entry of current CULT[]
		if (c->cs != STAR)		// if table has no * entry,
			return RCOK;			// no * fcns, nothing to do, ret ok
	}
	else
		c = x->c;			// other cases: current CULT entry ptr

	switch (fcn)					// fetch fcn ptr from CULT, if entry has such a fcn ptr
	{

	case ITF:
		if (c->IFLAGS & ITFP)  f = (UFCNPTR)c->p2;  		// itf and prf (and cult) share ptr .p2 per bits in .f
		else if (ucs != RGLR)  f = (UFCNPTR)c->DFPI;  	// also itf may be in .DFPI for non-data entries
		break;

	case PRF:
		if (c->IFLAGS & PRFP)  f = (UFCNPTR)c->p2;
		break;

	case CKF:
		f = c->ckf;
		break;

	default:
		;
	}
	if (f==NULL)			// if fcn ptr is NULL or pertinent f bit is off
		return RCOK;			// nothing to do, return ok

	if (ucs==RGLR)				// reglar case: data
	{
		p = xSp->p;				// arg 2 is item location
		p2 = xSp->e;				// arg 3 is RATE loc, or NULL
		p3 = xSp > xStk ? (xSp-1)->e : NULL;	// arg 4: calling tbl RATE loc or NULL.  May be irrelevant.
	}
	else		// other cases: whole RATE table entry, no member
	{
		p = xSp->e;				// arg 2 is RATE loc
		p2 = xSp > xStk ? (xSp-1)->e : NULL;	// arg 3: calling table RATE loc, or NULL.
		p3 = xSp > xStk + 1 ? (xSp-2)->e : NULL;	// arg 4: caller's caller's ditto.
		/* p2, p3 are successive RATE ptrs out if STRs are nested;
		   else often irrelevant even if nz.*/
		culPhase = CHECK_PHASE;			/* except for RGLR (1-member fcns), temporarily change to "check phase"
       						   so errMsgs show object's file/line (from basAnc record)
       						   not current file/line (of RUN,END,etc). 2-91 */
	}

	rc = (*f)( c, p, p2, p3);		// call fcn with arguments

	if (rc != RCOK)			// if user fcn returned error
	{
		// printf( " (cuf: fcn returned %d) \n", (INT)rc);  	// debug aid
		if (skie)			// if input skip on error requested
			perNx(NULL);			// skip rest of this input statement
		if (errCount() <= errCountSave)	// if fcn ++'d error count, don't repeat
			incrErrCount();		// incr global err count (in rmkerr.cpp): tested by RUN to stop execution
	}
	culPhase = phaseSave;		// restore phase of session
 	return rc;				// callers test for RCFATAL.  more returns above.
}		// cuf

//===========================================================================
LOCAL RC FC msgMissReq()

// Issue error messages for MISSING REQUIRED INFORMATION for current xSp->cult

// returns RCOK if none; also each message ++'s errCount.
{
	CULT *c;
	const char* ms;
	UCH fs, *fsp = nullptr;
	RC rc=RCOK;

// no checks if defining a type: omitting anything is ok
	if (defTyping)			// flag in this file set by culDEFTY
		return RCOK;

// check each table entry (only checks element [0] of arrays -- intentional)

	for (c = xSp->cult;  c->id;  c++)		// loop table
	{
		if (c->cs==DAT)
		{
			fsp = xSp->fs0 + c->fn;		// point to data field status (sstat[]) byte in record
			fs = *fsp;					// get its contents
		}
		else
			fs = 0;						// dummy field status for RATE etc

		// test if required but not given, and not error'd yet.  Only first element of ARRAYs is checked.

		if ( ( (c->f & RQD) 			// cult bit from user table: all entries
		       | (fs & FsRQD) )			// fld status: data fields only; poss future use from TYPEs 1-91.
		 && !( (c->IFLAGS & BEEN_SET)	// runtime cult bit: all entries
		       | (fs & FsSET) )			// fld status: data only, ??lasts thru type/like, includes AUTOSIZE
		 && !(fs & FsERR) )				// no further msg e.g. if error occurred decoding choice for this field
		{
			if (c->cs==DAT)			// if a data field -- else has no fs byte
				*fsp |= FsERR;			// say errMsg'd this field: may suppress addl msgs eg in user check code. 12-91.

			// msg subtext saying where: "input file", "zone 'foo'", "ebalck", etc
			const char* watIn;
			if (xSp <= xStk)			// if top level
				if (culNClears)			// if has been a "clear" cmd
					watIn = "input file (after 'clear')";
				else
					watIn = "input file";
			else 					// test for named basAnc record
				watIn = xSp->e->classObjTx();	// 'zone "Z1"', 'sgdist', etc, below.

			// message text for member assignment or for RATE group
			ms = c->cs==DAT  ?  (char *)MH_S0263		// "No %s given in %s"
							 :  (char *)MH_S0264;   	// "No %s in %s"

			// issue message with name of item and what it is missing from.
			rc |= perlc( ms, (char *)c->id, watIn );	/* [no caret:] item missing somewhere in preceding statement group.
          						   Line is that of ENDER, or eof if top level. */
			// add perl( feature to not repeat preamble of multiple error messages for same file line?
		}
	}
	return rc;
}		// msgMissReq

//===========================================================================
LOCAL RC ratPutTy( record *e, CULT *c)

// convert a basAnc record (entry) to a TYPE in same basAnc.  Deletes the basAnc record.

// returns RCOK, RCBAD, RCFATAL.
{
	BP b;
	record *tye;
	UCH *fs;
	SI f;
	RC rc;

	b = e->b;		// access basAnc from record

#ifdef DEBUG
// check argument
	if (b->validate("cul:ratPutTy",WRN))  return RCBAD;
#endif

// require record to be LAST in basAnc -- else have no way to delete  ( <<<< believe no longer true 1-92, fix if need found)
	if (e->ss != b->n)
		return err( PWRN, (char *)MH_S0266);	// "cul.cpp:ratPutTy(): basAnc record to be made a TYPE must be last"

// if this basAnc does not already have secondary basAnc for types, make one
	CSE_E( ratTyR(b) )				// below.  return if error.

// move record from main basAnc to its types basAnc

	if (b->tyB->add( &tye, WRN) )   			// add record to types basAnc
		return RCFATAL;					// if out of memory, terminate session 1-8-92
	CSE_E( rateDuper( tye, e, 1, 1, -1, c) )		// move record including name, dref's, owned subRat entries. Above.

// adjust sstat[] flags to convert record into a TYPE.   (caller pre-do this flags loop if ratPutTy moved to ancrec.cpp)

	for (f = 0;  f < b->nFlds;  f++) 	 	// loop field numbers
	{
		fs = (UCH *)tye + b->tyB->sOff + f;	// field's status byte (.sstat[f]).  Fs___ defines: cul.h.
		*fs &= ~FsERR;				// clear "error message issued" flag: insure full checking later.
		if (*fs & FsRQD) 			// if field is flagged "required" (for ARRAY, used on [0] only)
			//*fs &= ~(FsSET);			// clear its "has been entered" flag.  OK to leave FsVAL on?  I doubt it.
			*fs &= ~(FsSET|FsVAL);		// clear its "has been entered" flag, and "value stored" 3-92
		// ARRAY: would be better to clear FsSET/FsVAL for all array elts-->drive this loop by CULT.
	}

	return rc;
}		// ratPutTy
//===========================================================================
LOCAL RC rateDuper(

// copy, move, or delete basAnc record contents & dref's & owned sub-basAnc entries

	record* newE, 	// new record (ratAdded by caller); .b and .i used.
					//   Not used if deleting (move < 0) -- pass NULL.
	record* oldE, 	// old record; .b, .i, & contents used; deleted if move.
	SI move,		// 0: copy, 1: delete source after copying (ratPutTy)
					// -1: just delete source, and its subRATEs if c.
	SI cn,			// non-0 to copy name here -- else left null/unchanged
	TI own,			// -1 for no change, or new owner TI value
	CULT *c )		// table to direct duplication of "owned" subrat entries, or NULL to not do so.
// newE and oldE may be in main basAnc or types sub-basAnc.
// RECURSIVE if c != NULL.

// returns RCOK/RCFATAL 1-92
{
	DMHEAPCHK( "cul rateDuper() entry")
	TI srcOti;
	RC rc;

	BP ownB = oldE->b;		// owning basAnc
	srcOti = oldE->ss;  	// owner subscript of any subSRate's to copy

// copy contents.  Caller has ratAdded to main or types basAnc as desired.
	if (move >= 0)		// not if just deleting oldE
	{
		newE->CopyFrom( oldE, cn);	// copy contents, and name if cn
		if (own >= 0)				// if new owner value (or 0) given
			newE->ownTi = own;		//  store it: overwrite what CopyFrom copied.
		DMHEAPCHK( "cul rateDuper() CopyFrom")
	}

// do reference table and expression table entries
	if (move < 0)		// if deleting
	{
		drefDel( oldE);		// delete references to oldE, local, only call.
		extDel( oldE);		// delete expressions in oldE, exman.cpp 12-91
	}
	else if (move > 0)		// if moving not copying (for ratPutTy)
	{
		drefMove( newE, oldE);	// change any dref's for oldE to newE
		extMove( newE, oldE);	// change any expressions in oldE to belong to newE
	}
	else			// copying
	{
		drefDup( newE, oldE);	// duplicate deferred reference table entries
		extDup( newE, oldE);	// duplicate any expression table entries
	}
	DMHEAPCHK( "cul rateDuper() dref")

// if CULT given, copy/move/delete owned subrat entries and their dref's

	if (c != NULL)
	{
		TI i;
		CULT *cc;
		BP b, fromB, toB;
		record* fromE = NULL;
		record* toE = NULL;

		// loop over basAncs owned by oldE's basAnc: search CULT entries
		for (cc = c-1; nxOwRat( ownB, c, &cc, &fromB, &b); )		// loop basAncs owned by basAnc ownB
		{
			// Do this basAnc's records (entries) owned by record oldE, recursively
			for (i = 0; nxOwRec( srcOti, fromB, &i, &fromE); )
			{
#if defined( _DEBUG)
				fromE->Validate();
#endif
				// loop fromB entries with ownTi==oldE's ti
				if (move >= 0)		// not if delete
				{
					if (newE->b->ba_flags & RFTYS)	// if dest is a TYPE
					{
						CSE_E( ratTyR(b) )		// add it if necessary (returns RCOK/RCFATAL), and
						toB = b->tyB;		//  use dest's types subRatBas
					}
					else
						toB = b;
					if (toB->add( &toE, WRN) )	// add record. CAN MOVE fromE.
						return RCFATAL;			// if out of memory, end session 1-8-92
					fromE = &fromB->rec(i);		// repoint fromE
#if defined( _DEBUG)
					fromE->Validate();
#endif
				}
				CSE_E( rateDuper(     		// CALL SELF: copy/move/delete record,
					   toE,				// ... dref's, and subcults */
					   fromE,			// repoint fromE
					   move, 1,   		// do copy name in subs
					   newE ? newE->ss : -1,	// owner subscr for subRat record copies made / illeg mem read protection */
					   (CULT *)cc->CULTP2 ) )  	// subtbl drives subcult copy
				if (move)			// if move or delete, adj subscr for
					i--;			// ...deletion: basAnc has been squeezed.
			}  // for (fromE=
		}    // for (cc=
		DMHEAPCHK( "cul rateDuper() subrats")
	}	// if (c != NULL)

// delete source if move or delete

	/* do AFTER doing owned subRAT entries: squeeze here would
	   make ownTi's non-unique if those owned by oldE not gone. */

	if (move)				// if 1 or -1
	{
		ownB->del(srcOti);		// squeeze out basAnc record, lib\ancrec.cpp
		drefAdj( ownB, srcOti+1, -1);	// adjust dref's for sqz'd out subscr. only call.
		extAdj( ownB, srcOti+1, -1);	// adjust expression table entries for sqz'd out subscr, exman.cpp, 12-91.
		if (c)					// if cult given, so can find ownees
			adjOwTi( ownB, c, srcOti+1, -1);	// fix ownTi in basAnc records owned by entries whose subscr was changed in squeeze
		else
		{
			err( PWRN, (char *)MH_S0201);		// display internal error msg
			/* "cul.cpp:rateDuper(): move or delete request without CULT:\n"
								           "    can't adjust ownTi in owned RAT entries" */
			return RCFATAL;			// 1-8-91
		}
		DMHEAPCHK( "cul rateDuper() move")
	}
	DMHEAPCHK( "cul rateDuper() exit")
	return RCOK;
}				// rateDuper
//===========================================================================
LOCAL void FC adjOwTi(

// adjust owner subscript in basAncs owned by given basAnc (after delete(/insert))

	BP ownB, 	// basAnc whose ownees to adjust
	CULT *c, 		// table for finding possible ownees of ownB
	TI minI, 		// change .ownTi's >= this
	TI delta )		// .. by this much (signed)

// see also: drefAdj()
{
	BP b;
	for (CULT *cc = c-1; nxOwRat( ownB, c, &cc, &b, 0); )	// loop over basAncs owned by ownB
	{
		for (SI i = 1;  i <= b->n;  i++)		// loop records in basAnc b
		{
			record *e = &b->rec(i);	// point to record i (ancpak.cpp)
			if (e->ownTi < minI)		// if owner out of range of interest
				continue;			// next i
			e->ownTi += delta;		// adjust this owner ti
		}
	}
}		// adjOwTi
//===========================================================================
LOCAL RC FC ratTyR( BP b)		// if basAnc does not have secondary basAnc for types, add one to it.

// returns RCOK/RCFATAL
{
#ifdef DEBUG
	if (b->validate("ratTyR",WRN))	// check argument
		return RCFATAL;
#endif

	if (b->tyB==0)			// if this anc does not already have secondary anc for types, make one
	{
		RC rc;

		// generate constructor arguments: flags, what, ownB

		int flags = RFTYS | RFNOEX;  				// say is "types" anchor, disable expression expansion

		char *_what = strsave( strtcat( (char *)b->what, " type", NULL) );	// form record type name
		if (!_what)								// if failed (strsave issued msg, right?)
			return RCFATAL;						// out of memory: end session.
		BP _ownB = 0;
		if (b->ownB)			// if basAnc is owned, then tyB's owner will be owners tyB
		{
			CSE_E( ratTyR(b->ownB) )    	// CALL SELF now to be sure owner already has tyB: if added later, ptr wd not get here
			_ownB = b->ownB->tyB;
		}

		// make anc<T> of main anc's <T> (ie same derived class): use special constructor (of an arbitrary T)
		// that COPIES given anc INCLUDING VIRTUAL FCNS.  Copies rt, eSz, sOff, etc; drops records, tyB.
		// CAUTION: must not register w/o adding unregister to d'tor.
		b->tyB = (BP)new anc<ZNR>( b, flags, _what, _ownB, 1 );	// construct copy w/ no records; do not register.  ancrec.h.
		if (!b->tyB)
		{
			//  (known inconsistency: out of memory messages from
			// memfullerr: ;			//  ancrec, dmpak, etc do not appear in input listing.)
			cuEr( 0, (char *)MH_S0267);		// display message and echo in input listing 1-8-92.
											// "Out of near heap memory (cul:ratTyr)"
			return RCFATAL;					// out of memory: end session.
		}
	}
	return RCOK;
}			// ratTyR

//===========================================================================
LOCAL SI nxOwRat( 	// next basAnc owned by b for CULT

	BP ownB, 	// owning basAnc
	CULT *c,	// base of table to search
	CULT **pc, 	// rcvs ptr to CULT entry for *pr; init NULL
	BP *prTyIf,	// NULL or rcvs ptr, for types or not, per ownB flags.
	BP *pr ) 	// NULL or rcvs non-types basAnc ptr.

// fcn value 0 if no (more) owned basAncs entries in CULT
// typical call: for (cc = NULL; nxOwRat( b, c, &cc, &rr, NULL); )
{
	CULT *cc, *ccc;
	BP bb, bbTyIf;

// loop over RATE entries in table c
	cc =  *pc  ?  *pc + 1  :  c;	// next or first cult entry
	for ( ;  cc->id;  cc++)		// loop til end table (or found)
	{
		if (cc->cs != RATE)		// if not a basAnc-record entering CULT cmd
			continue;			// proceed to next cc
		// test whether this one is owned by ownB and should be returned
		bb = (BP)cc->b;				// basAnc to which this table entry adds records
		for (ccc = c;  ccc < cc; ccc++)   	// loop prev entries
			if (ccc->cs==RATE && ccc->b==bb)	// if a duplicate
				goto conCon;   			// proceed to next cc
		// if ownB is a types basAnc, access types basAnc of bb
		if (ownB->ba_flags & RFTYS)		// if owner is TYPEs basAnc
		{
			bbTyIf = bb->tyB;     	// get ownee TYPEs basAnc
			if (bbTyIf==0)			// if no TYPEs in this basAnc
				continue;			// nothing to return; next cc
		}
		else				// owner not TYPEs basAnc
			bbTyIf = bb;			// use main basAnc
		if (bbTyIf->ownB != ownB)	// insurance: if wrong owner
conCon:
			continue;			// proceed to next cc
		// found, return info
		if (prTyIf)  *prTyIf = bbTyIf;	// if caller wants it, owned basAnc for caller
		if (pr)	*pr = bb;		// if caller wants it, corresponding non-Types basAnc
		*pc = cc;			// cult for ownable-record basAnc, for caller and next call here.
		return 1; 			// say found an owned basAnc.
	} // for ; ;
	return 0;				// none or no more
}		// nxOwRat
//===========================================================================
LOCAL SI nxOwRec( 	// next owned basAnc record (entry) in owned basAnc b

	TI ownTi, 		// desired owner subscript, normally should be non-0
	BP b,	   	// caller should have verified b owned by desired basAnc
	TI *pi,		// rcvs subscript. init 0 for 1st call
	record **pe )	// NULL or rcvs record ptr

// fcn value 0 if no (more) owned entries in basAnc
// typical call: for (i = 0; nxOwRec( ownTi, b, &i, &e); )
{
// search from next record.  Caller inits *pi to 0 for first call.
	for (TI i = *pi + 1;  i <= b->n;  i++) 		// till found one or end basAnc's records
	{
		record *e = &b->rec(i);				// point to record (entry) i (ancrec.cpp)
		if (e->ownTi != ownTi)
			continue;			// wrong owner, try next i
		// found
		*pi = i;			// return subscript: we use it on next call
		if (pe)			// if caller wants it,
			*pe = e;		// return pointer
		return 1;  		// say found
	}
	return 0;			// no (more) entries in basAnc
}		// nxOwRec

/* There are probably more uses for:  xnxDatMbr xnxC nxRec nxRat nxDatMbr nxC
   but limited as  rateDuper nxOwRat nxOwRec  (at present) make no xSp
   entries as they nest; also not determined
   if top use of  xSp->b, ->cult  (not args)  ok for them.  1-23-91. */

//===========================================================================
LOCAL SI FC xnxDatMbr(

	/* extended first/next data member of world of current xSp level:
	   loops basAncs, basAnc records, members in each record */

	SI bads,	// 0 to include only basAnc records with .gud > 0
	void **pp )	// init *pp NULL on 1st call; rcvs data ptr (same as xSp->p)

// uses: xSp->b, xSp->cult (current/top level)

/* sets: xSp->c, ->e, ->fs0, ->p, ->p0, ->arSz, ->sz, ->fs;
   nested levels: xSp->b, xSp->cult */

// call at top xStk level to loop over data in all basAncs.
// forms xStk entries per table nesting; xSp restored only at completion.
// DO NOT NEST CALLS (nxRat, xnxDatMbr have internal static data)
// walks tree bottom-up -- does most nested basAncs first.
// includes TYPEs (detect with  if (xSp->b->ba_flags & RFTYS) ... )
// typical use:  for (p = NULL; xnxDatMbr( 0, &p); )
{
	static BP b; 		// self-cleaning: *pp NULL on outer call.
	static void * e;		// ditto.

	if (*pp != NULL)			// unless 1st call
		goto reenter;			// resume in middle of loops

	for (b = 0;  nxRat(&b); )   	// loop over all basAncs incl TYPEs
		for (e = NULL;  nxRec( bads, &e); )	// loop over records in a basAnc
			for (*pp = NULL;  nxDatMbr(pp); )	// loop over members of rec
			{
				return 1;				// say have a datum, p and xStk set
reenter:
				;
			}
	return 0;				// done.  another return above.
}		// xnxDatMbr
//===========================================================================
LOCAL bool xnxC(
	CULT* &c,			// init c NULL on 1st call; rcvs cult ptr (same as xSp->c)
	int options /*=0*/)	// options (passed to nxRat)

// extended first/next cult entry of world of current xSp level:
// loops cults, following nesting of RATE records, depth first */

// uses: xSp->b, xSp->cult (current/top level)
// sets: xSp->c's, and, for nested levels, xSp->b, xSp->cult

// call at top xStk level to loop over all cults.
// forms xStk entries per table nesting; xSp restored only at completion.
// DO NOT NEST CALLS (nxRat, xnxC have internal static data)
// walks tree bottom-up -- does most nested cults first.
// typical use:  for (c = NULL; xnxC(c); )

// returns true if 
{
	static BP b;		// static basAnc * b
	// (self-cleaning: *pc NULL on outer call)

	if (c != NULL)				// unless 1st call
		goto reenter;				// resume in middle of loops

	for (b = 0;  nxRat(&b, options); )		// loop over all CULTs/basAncs
	{
		if (b->ba_flags & RFTYS)     		// skip any types sub-basAncs
			continue; 					// .. wanna do each CULT once.
		for (c = NULL;  nxC( c); )  	// loop over members of cult
		{
			return true;				// say have a cult, *pc and xStk set
reenter:
			;
		}
	}
	return false;				// done.  another return above.
}		// xnxC
//===========================================================================
LOCAL SI FC nxRat( 	// first/next basAnc in current+nested tables

	BP *pr,		// basAnc * *.  init *pr NULL on 1st call; rcvs basAnc ptr (same as xSp->b).
	int options /*=0*/)		// option bits
							//   nxropIGNORE_BEEN_HERE: ignore BEEN_HERE flags

// also uses: xSp->b, xSp->cult (current/top level)
// sets: xSp->b, xSp->cult (nested levels) (adds to xStk for nested tbls)
// uses internally (nesting levels) xSp->c

// call at top xStk level to loop over all basAncs.
// DO NOT NEST CALLS (nxRat has internal static data)
// walks tree bottom-up (does subrats first).  Includes TYPEs basAncs.
// typical use: for (b=NULL; nxRat(&b); ) { [if (b->ba_flags & RFTYS) continue;]
{
	static XSTK* xSpSave;	// (self-cleaning cuz *pr NULL on outer call, 10-93)

	if (*pr==0) 			// if first call
	{
		xSpSave = xSp; 			// save entry xSp level for endtest
		xSp->c = xSp->cult - 1;		// init entry pointer in current CULT
		clrBeenHeres(xSp->cult);		// clear IFLAGS & BEEN_HERE throughout tables
	}
	else			// reentry. Just did a basAnc, so now pop a level to continue.
	{
		if (xSp->b->tyB)			// but 1st, if basAnc done had a TYPEs sub-basAnc,
		{
			xSp->b = xSp->b->tyB;  	// do that now. (limitation: this
			*pr = xSp->b;			// .. sequence does not permit deleting
			return 1;				// .. the sub-basAncs in same pass)
		}
		if (xSp <= xSpSave)			// if at entry xStk level
			return 0;				// table tree walk is complete, say done
		--xSp;   			// else continue at embedding level
	}
// loop to find next basAnc to return to caller
	for ( ; ; )
	{
		++xSp->c;
		// first, do any nested basAncs
		if ( xSp->c->cs==RATE 					// if CULT tbl entry is for basAnc record
				&& (!(((CULT *)xSp->c->CULTP2)->IFLAGS & BEEN_HERE) 	// if not done yet
			       || (options&nxropIGNORE_BEEN_HERE)) )
		{
#if 0 && defined( _DEBUG)
			if ((((CULT *)xSp->c->CULTP2)->IFLAGS & BEEN_HERE) 	// if done before
				&& (options & nxropIGNORE_BEEN_HERE))			// but caller sez ignore
			{
				CULT* pC = xSp->c;
				CULT* pCX = (CULT *)pC->CULTP2;
				printf("\nWould have skipped");
			}
#endif
			((CULT *)xSp->c->CULTP2)->IFLAGS |= BEEN_HERE;   	// say done.  CAUTION: ratCultO may clear this bit
	  															// where rescan desired.  Coordinate changes.
			if (xSp >= xStk + sizeof(xStk)/sizeof(XSTK) - 1 - 1)
				perl((char *)MH_S0270);	//"cul.cpp:nxRat: Too much nesting in tables: Increase size of context stack xStk[] in cul.cpp"
			else				// room for another xStk entry
			{
				++xSp;					// push xStk
				xSp->b = (BP)(xSp-1)->c->b;     		// the basAnc
				xSp->cult = (CULT *)(xSp-1)->c->CULTP2;  	// the CULT table
				xSp->c = xSp->cult-1;			// init CULT entry ptr
				continue;		// go to top loop to check this tbl for RATEs.
             					// When there are no more levels with basAncs, control will get to break in next 'if'.
			}
		}
		// then, do this basAnc
		if (!xSp->c->id)  	// if terminating entry of CULT
			break;		// ret this basAnc to caller now: xSp->b, ->cult. Caller may alter xSp->c -- next call pops xStk.
	}	// for ; ;
// found basAnc, return stuff
	*pr = xSp->b;		// return basAnc location / say initialized
	return 1;			// say have a basAnc.  2+ other returns above.
}		// nxRat
//===========================================================================
LOCAL SI FC nxRec( 	// first/next record in current basAnc (xSp->b)

	SI bads,	// 0 to include only records with .gud > 0
	void **pe )	// init *pe NULL on 1st call; rcvs record ptr (same as xSp->e)

// also uses: xSp->b
// sets: xSp->e, ->fs0

// typical use:  for (e = NULL; nxRec( 0, &e); )
{
	if (*pe==NULL)  			// if first call
		xSp->i = xSp->b->mn;			// first record subscript of basAnc: 0 if static, else 1
	else
		xSp->i++;				// next record subscript
	for ( ; ; xSp->i++)			// find good record, if !babs
	{
		if (xSp->i > xSp->b->n)   		// if no more entries
			return 0;				// say done
		xStkPt();				// set xSp->e, ->fs0 (and others)
		if ( ((record *)xSp->e)->gud > 0 	// if good record
		|| bads )				// or accepting bad/deleted entries
			break;				// found record to return
	}
	*pe = xSp->e;			// return pointer / initialized indicator
	return 1;				// say have one
}		// nxRec
//===========================================================================
LOCAL SI FC nxDatMbr( 	// first/next data member of current record

	void **pp )	// init *pp NULL on 1st call; rcvs data ptr (same as xSp->p)

// iteration includes ARRAY members, but caller must subscript fs for those Fs flags used on each element.

// also uses: xSp->cult, xSp->e, xSp->fs0
// sets: xSp->c, ->p, ->arSz, ->j, ->sz, ->fs

// typical use:  for (p = NULL; nxDatMbr(&p); )
{
	if (*pp==NULL)  			// if first call
		xSp->c = xSp->cult - 1;  	// init working ptr into table
	else if (xSp->c->f & ARRAY)		// not first call, if doing array member, return next element of array
	{
		IncP( DMPP( xSp->p), xSp->sz);	//  next element
		if (++xSp->j < xSp->arSz)	//  next subscript / if not end of array
		{
			*pp = xSp->p;			//   return pointer / initialized indicator
			return 1;			//   say found one
		}
	}
	do 				// until a data member found
	{
		++xSp->c;  			// next table entry
		if (!xSp->c->id)  		// if end table (** may be near, don't use ==NULL)
			return 0; 			// say done
	}
	while (xSp->cs != DAT  &&  xSp->cs !=KDAT);
	if (datPt() != RCOK)	// sets xSp->p, sz, fs from xSp->c, e, fs0.
		return 0;		// on error (eg bad CULT; msg issued; xSp may be NULL) act as tho no more members 2-91.
	*pp = xSp->p;		// return pointer / initialized indicator
	return 1;			// say found one.  Other return(s) above.
}		// nxDatMbr
//===========================================================================
LOCAL bool nxC( 	// first/next member of current cult

	CULT* &c )	// init c = NULL on 1st call; rcvs cult ptr (same as xSp->c)

// also uses: xSp->cult
// sets: xSp->c
// DOES NOT datPt

// typical use:  for (c = NULL; nxC(c); )

// returns true iff c set pointing to next CULT

{
	if (c==NULL)  			// if first call
		xSp->c = xSp->cult - 1; // init working ptr into table
	++xSp->c;					// next table entry
	if (!xSp->c->id)     		// if end table
		return false; 			// say done
	c = xSp->c;					// return pointer / initialized indicator
	return true;				// say found one.  Another return above.
}		// nxC

//===========================================================================
LOCAL RC ratLuCtx(

// look up basAnc record by name, resolving ambiguities using current context.

	BP b,
	char *name,
	char *what, 	// name (eg c->id) of thing being sought, for errMsgs, or NULL to use b->what
	record **pE )	// NULL or receives ptr to record if found

/* if b is an ownable-record basAnc, uses default owner per current context
     when possible to determine which of entries with same name to use.
	Ie, when looking for a surface, take one for a zone with a current
 xStk record even if other zones have surfaces with same name */

/* returns: RCOK:  found, *pep set.
	    RCBAD: not found or ambiguous.
	    	   Error message has been issued and RUN prevented.  Caller
	    	   must perNx(NULL) if desired to skip rest of input stmt. */

{
	return ratLuDefO( b, name,
		ratDefO(b),	// determine dlt owner if any per xStk
		what, pE,
		NULL );		// tell ratLuDefO to issue messages
}	// ratLuCtx
//===========================================================================
LOCAL RC ratLuDefO(	// look up basAnc record by name, resolving ambiguities using default owner if nz.

	BP b,
	const char* name,
	TI defO,    	/* 0 or dfl owner subscript for ambiguity resolution.  Must be 0 if b is not owned-record basAnc.
			   May be obtained with ratDefO (below). */
	char *what, 	// name (eg c->id) of thing being sought, for errMsgs, or NULL to use b->what
	record **pE,	// NULL or receives ptr to record if found
	const char** pMsg)	// NULL or receives ptr to unissued Tmpstr error msg

/* If more than one record with name is present, if defO non-0, returns one with owner defO if any.
   If defO is 0 or no records with that owner are found, returns bad if there are multiple records for name. */

/* returns: RCOK:  found, *pep set.
	    RCBAD: not found or ambiguous.
		   if "msg" arg given, rcvs ptr to unissued message,
		   else err msg has been issued and RUN prevented.  Caller
	    	   must perNx(NULL) if desired to skip rest of input stmt. */
{
	RC rc;
	record *e, *e2;
	const char* ms = NULL;

	if (what==NULL)
		what = (char *)b->what;
	if (defO==0)						// if no default owner given
	{
		rc = b->findRecByNm1( name, NULL, /*VV*/ &e);		// find first record with given name, ancrec.cpp
		if (rc)
			ms = strtprintf( (char *)MH_S0278, what, name);	// format ms "%s '%s' not found" to Tmpstr.  Also used below.
	}
	else
	{
		rc = b->findRecByNmDefO( name, defO, /*VV*/ &e, /*VV*/ &e2);	// find record by name & dfl owner, ancrec.cpp
		if (rc)								// (RCBAD not found, RCBAD2 ambiguous 1-92)
		{
			if (e)							// if bad and entrie(s) returned, is ambiguous match
				ms = strtprintf( (char *)MH_S0279,				// "ambiguous name: \n"
					what, name, e->whatIn(),			// "    %s '%s' exists in %s\n"
					e2->whatIn() );				// "    and also in %s" */
			else
				ms = strtprintf( (char *)MH_S0278, what, name);		// "%s '%s' not found" also used above
		}
	}
	if (ms)		// if have unissued error message (rc also set)
	{
		if (pMsg)	// if caller wants it
			*pMsg = ms;	// give it to him
		else
			perlc(ms);	// else give it to user now. cuparse.cpp.
		// issue message showing file name and line #; disables RUN.
	}
	if (rc==RCOK)	// if no error
		if (pE)		// if return pointer given
			*pE = e;  	// return record location
	return rc;
}		// ratLuDefO
//===========================================================================
TI FC ratDefO( BP b)

// find default owning record for records in basAnc of current context

/* returns subscript of a record in basAnc whose records own records in basAnc b,
   or 0 if not an owned-record basAnc, b->ownB not set yet (may occur if empty),
           or possible owning record not found in current context stack. */
{
	if (b->ownB)   			// if owner is set (non-0) in basAnc b
		for (XSTK* x = xSp; x >= xStk; x--)
		{
			if (x->b==b->ownB)
				if (x->i)
					return x->i;
		}
	return 0;		// not found, not owned, or owner not set
}		// ratDefO
//===========================================================================
LOCAL void FC ratCultO( void)

// determine rat "ownership" from table structure, set b->ownB's

// call at top xStk level (to do all basAncs).
// uses xSp->b and xSp->cult; and uses more xStk entries internally.
{
	for (BP b = 0;  nxRat(&b, nxropIGNORE_BEEN_HERE);  )
										// loop basAncs by looping tables, adding xStk entries per table
    									// nesting (ie ownership).  Uses xSp->cult, xSp->b.  above.
										//   ignore BEEN_HERE so multiple sub-RATEs of same type are handled
										//     eg DHWHEATER and DHWLOOPHEATER from DHWSYS
	{
		//if (b->isOwnable())       if an ownable-record basAnc: all are, 7-92.
		if (xSp > xStk )  			// if nested (not top: no owner)
			if ((xSp-1)->c->f & NOTOWNED)  			// if embedding table entry flagged as "not indicating ownership" 11-91
				xSp->cult->IFLAGS &= ~BEEN_HERE;	// clear nxRat's flag so this cult will be returned again when
             										//  accessed by another entry -- may indicate its owner 12-10-91
			else
			{
				BP ownB = (xSp-1)->b;			// owner here is embedding basAnc
				if (b->ownB != 0   				// if owner already set
				 && b->ownB != ownB )  			// to another owner
					per( (char *)MH_S0202,			// error message, cuparse.cpp
					//"cul.cpp:ratCultO: internal error: bad tables: \n"
					//"    '%s' rat at %p is 'owned' by '%s' rat at %p and also '%s' rat at %p",
					b->what, b, b->ownB->what, b->ownB, ownB->what, ownB );
				else
					b->ownB = ownB;		// set owner
			}
	}
}		// ratCultO

//===========================================================================
LOCAL void FC drefAdd( 		// add dref using xSp frame
	const char* toName )	// name of entry referred to. dm pointer used here, no IncRef here.

/* reference is to a basAnc record named toName in rat xSp->c->b.
   reference is in current datum (per datPt'd xSp entry)
	at xSp->p (offset computed here).
   table entry made here causes reference to be resolved (subscript of
      ref'd entry stored) when drefRes is called (at RUN).
   deferring resolution allows definition after reference. */
{
	drefAddI( xSp->b, xSp->i, 			// refering b, ti
		xSp->c->fn + xSp->j,			// refering field: successive array elements are successive fields.
		(BP)xSp->c->b, toName, 			// referee b, name,
		ratDefO( (BP)xSp->c->b) );    	// owning subscript in ->ownB, if found in xStk, else 0.  Above.
}				// drefAdd
//===========================================================================
LOCAL void FC drefAddI( 	// add deferred reference table entry -- general inner routine
	BP b,		// rat containing reference
	TI i,		//  entry subscript ditto
	SI fn,		//  field number ditto
	BP toB,		// rat being referred to
	const char* toName,	// name of entry being referred to.
						//  Dm ptr used here: caller is done with pointer or does own IncRef.
	TI defO )			// 0 or default owner per context
{
	DREF* drfp;

	// search table for existing entry to overwrite:
	// to eliminate superceded old entry (eg from type):
	//   save table space, prevent spurious errMsg if not found at RUN.
	for (int d = 0;  d < nDref;  d++)
	{
		drfp = dref + d;
		if (drfp->fn==fn && drfp->i==i && drfp->b==b)
			goto found;
	}
// no table entry found, add one at end
	drfp = drefNextFree();

found:
// fill entry
	drfp->b = b;			// where reference is: rat,
	drfp->i = i;			//     entry subscr,
	drfp->fn = fn;			//     field number.
	drfp->toB = toB;			// what is being referenced: rat,
	drfp->toName = toName;		//			entry name.
	drfp->defO = defO;			//	default owner from context
// .. get file/line info for error messages using cutok.cpp fcn
	curLine(				// get line, line, etc for errmsg, cuparse.cpp.
		0,  			// start current (not previous) token
		&drfp->fileIx,		// rcvs index to input file name
		&drfp->line,		// rcvs line number
		NULL,			// would receive column
		NULL, 0 );   		// char[] buffer would rcv line text
}			// drefAddI
//-----------------------------------------------------------------------------
LOCAL DREF* drefNextFree()		// ptr to next free DREF (enlarge table if needed)
{
	if (nDref >= drefNal)						// if need to allocate or enlarge table
	{	int nuNal = drefNal + 200;
		if (dmral( DMPP( dref), nuNal * sizeof( DREF), DMZERO|ABT))		// allocate or reallocate heap block, lib\dmpak.cpp
			return NULL;							// (return unexpected with ABT)
		drefNal = nuNal;							// now store new allocated size
	}
	DREF* drfp = dref + nDref;		// point to entry to use
	nDref++;						// bump count of entries. drfp already set.
	return drfp;
}		// drefNextFree
//===========================================================================
LOCAL void drefMove( 	// change dref's to refer to a new entry

	record *nuE,		// new basAnc record
	record *e )		// record being copied (to be deleted)
{
	BP b = e->b;		// rat and
	TI i = e->ss; 		//  entry subscript whose dref's to move
	BP nuB = nuE->b;  	// new rat and
	TI nuI = nuE->ss;	//  subscr to put in the entries
	DREF *drfp;		// deferred reference table search pointer
	SI d;			// .. subscript

	for (d = nDref; --d >= 0; )			// loop deferred ref's table
	{
		drfp = dref + d;
		if (drfp->b==b && drfp->i==i)  		// if a ref to given basAnc entry
		{
			drfp->b = nuB;  			// store new referer rat
			drfp->i = nuI;  			// .. subscript
		}
	}
}		// drefMove
//===========================================================================
LOCAL void drefDup( 	// duplicate dref's at basAnc record dup

	record *nuE,		// new basAnc record
	record *e )		// record being copied
{
	DMHEAPCHK( "drefDup entry")
	BP b = e->b;		// rat and
	TI i = e->ss; 		//  entry subscript whose dref's to dup
	BP nuB = nuE->b;  	// new rat and
	TI nuI = nuE->ss;	//  subscr to put in the duplicate entries
	SI d;			// deferred reference table search subscript

	for (d = nDref; --d >= 0; )  			// loop deferred ref's table
	{
		DREF* drfp = dref + d;  			// point to entry d
		if (drfp->b==b && drfp->i==i)		// if a ref to given basAnc entry
		{
			DREF* nud = drefNextFree();		// next available empty slot
			drfp = dref + d;  				// repoint to entry d (drefNextFree reallocates to expand)
			memcpy( nud, drfp, sizeof(DREF) );		// copy contents
			nud->b = nuB; 				// new referer
			nud->i = nuI; 				// ..
			dmIncRef( DMPP( nud->toName));   		// bump reference count of dm blocks, dmpak.cpp, 4-92
		}
	}
	DMHEAPCHK( "drefDup exit")
}		// drefDup
//===========================================================================
LOCAL void FC drefDel( record *e)		// delete dref's referring to given basAnc record
{
	DMHEAPCHK( "drefDel entry")
	BP b = e->b;		// rat and
	TI i = e->ss; 		//  entry subscript whose dref's to delete
	DREF *drfp; 		// deferred reference table search pointer
	SI d;			// deferred reference table search subscript

	for (d = nDref; --d >= 0; )			// loop deferred ref's table
	{
		drfp = dref + d;
		if (drfp->b==b && drfp->i==i)					// if a ref to given basAnc entry
		{
			dmfree( DMPP( drfp->toName));    			// free dm pointer in entry to be overwritten
			memmove( drfp, drfp+1, 						// squeeze out of table
				(USI)((char *)(dref + nDref) - (char *)(drfp + 1)) );
			nDref--;								// have one less dref
			dref[nDref].toName = NULL;				// insurance: clear dm pointer just moved: avoid duplicates
		}
	}
	DMHEAPCHK( "drefDel exit")
}		// drefDel
//===========================================================================
LOCAL void FC drefDelFn( BP b, TI i, SI fn)	// delete dref's referring to given field in given basAnc record
{
	DMHEAPCHK( "drefDelFn entry")
	DREF *drfp; 		// deferred reference table search pointer
	SI d;				// deferred reference table search subscript

	for (d = nDref; --d >= 0; )				// loop deferred ref's table
	{
		drfp = dref + d;					// point to entry d
		if (drfp->b==b && drfp->i==i && drfp->fn==fn)				// if a ref to given basAnc record & field
		{
			dmfree( DMPP( drfp->toName));    					// free dm pointer in entry to be overwritten
			memmove( drfp, drfp+1, 						// squeeze out of table
				(USI)((char *)(dref + nDref) - (char *)(drfp + 1)) );
			nDref--;								// have one less dref
			dref[nDref].toName = NULL;				// insurance: clear dm pointer just moved: avoid duplicates
		}
	}
	DMHEAPCHK( "drefDelFn exit")
}		// drefDelFn
//===========================================================================
LOCAL void FC drefAdj(

// adjust dref table at rat subscript deletion (or insertion)

	BP b,	   	// references IN this rat,
	TI minI, 		// in entries with this subscript or larger
	TI delta )		// have subscr in dref[] adj by this much (signed)

// see also: adjOwTi().
{
	DREF *drfp;  		// deferred reference table search pointer
	SI d;			// deferred reference table search subscript

	for (d = nDref; --d >= 0; )			// loop deferred ref's table
	{
		drfp = dref + d;				// point to entry d
		// reference in (from) given rat
		if ( drfp->b==b  			// if a ref in given basAnc's records
		&& drfp->i >= minI )   		// if ref is in entry at/after change point
			drfp->i += delta;     		// adjust for deletion (or insertion) in basAnc's record block
		// reference TO rat OWNED BY given rat
		if ( drfp->toB->ownB==b
		&& drfp->defO >= minI )
			drfp->defO += delta;
		// refs TO given rat ... are held by name in table!
	}
}			// drefAdj
//===========================================================================
LOCAL void FC drefRes()

// resolve deferred rat references (TYREF's): look up names, store subscripts
{
	for (int d = 0;  d < nDref;  d++)	// loop over deferred reference table.  Loop forward in case
	{								// ... there are (unexpected) unremoved superceded entries.
		DREF* drfp = dref + d;  			
		BP b = drfp->b;
		if (b->ba_flags & RFTYS)	// don't fill ref's in TYPE rat entries
			continue;				// .. (yet the dref entries ARE needed, to propogate at USETYPE)
		record* e = b->GetAtSafe( drfp->i);		// record location
		if (!e)
		{	per( (char *)MH_S0203, b->what, (INT)drfp->i );	// "cul.cpp:drefRes() internal error: bad %s RAT index %d"
			continue;
		}
		TI* p = (TI *)((char *)e			// field location: where to store reference subscript: record location,
				+ b->fir[drfp->fn].fi_off);	//  plus field offset
		UCH* fs = (UCH *)e + b->sOff;		// field status byte location for field
		const char* ms=NULL;
		if (ratLuDefO( drfp->toB, drfp->toName, drfp->defO,
				NULL,
				&e, &ms ) != RCOK )	// local.  Note e reuse.
									// look up name in basAnc's records
									//   resolving eg same surf name in 2 zones by dflt owner.
		{
			*p = 0;				// not found. store 0.
			*fs |= FsERR;    	// say errMsg has been issued re this field: suppress some addl msgs, 12-91.
			if (ms)				// if ratLuDefO returned unissued msg
				cuErv( 0, 0, 0, 0, drfp->fileIx, drfp->line, 0, ms, NULL);
			// issue msg.  Show file/line of ref, not curr file/line (of RUN). cutok.cpp.
		}
		else				// entry found
		{
			*p =  e->ss;			// store entry's subscript
			*fs |= FsVAL;			// say 'value has been stored' in field's field status byte
		}
	}
}		// drefRes
//===========================================================================
LOCAL void FC drefClr(
	[[maybe_unused]] const char* callTag)		// identification of call (debug aid)

// clear table of deferred rat references (TYREF's)
{
#if defined( _DEBUG)
	DMHEAPCHK( strtprintf("drefClr entry %s", callTag))
#endif

	while (nDref > 0)
	{
		DREF* drfp = dref + --nDref;		// point last entry / delete it by --count
		dmfree( DMPP( drfp->toName));    	// clean deleted entry: free dm ptr (from iqx)
	}
#if defined( _DEBUG)
	DMHEAPCHK( strtprintf("drefClr exit %s", callTag))
#endif
}		// drefClr

//===========================================================================
LOCAL RC FC addVrbs( CULT *cult)

// Add all ->id's in cult and nested cults to global symbol table as "verbs"

/* This is done to:
   *  Improve compile error recovery when terminating ';'
      is missing, as "verbs" have a low precedence and will not be
      passed by perNx() when looking for start next statement.
   *  Generally allow classification of words for other (future 11-90) uses.*/

/* Notes:
   1.  Causes tokTy to be CUTVRB (not CUTID) after read by toke().
   2.  When toke() reads one of these words, "stbk" is set to point to its
       CULT entry per symbol table (unspecified one if duplicate symbols).
       We do not now 11-90 use this as we accept only id's in current table
       and allow id's in one table to duplicate those of another. */
{
	CULT *c;
	RC rc=RCOK;

	// CAUTION: cannot recode with nxRat as also used for stdVrbs[].

	if (addVrbsLvl++ == 0)	// at top level
		clrBeenHeres(cult);	// clear BEEN_HERE flags incl in all subCults

	if ((cult->IFLAGS & BEEN_HERE)==0)	// do not repeat a table: would make duplicate symbol table entries
	{
		cult->IFLAGS |= BEEN_HERE;	// say tbl done: set flag in 1st entry

		/* add all keywords in this table to global symbol table.
		   Pointer to the cult entry is used as symbol table block pointer. */
		rc |= cuAddItSyms(			// cuparse.cpp, calls sytb.cpp
			CUTVRB,		// token type (comes back in tokTy)
			1,		// case-independent
			cult->cs==STAR 	// if has * entry (must be 1st)
			?  cult + 1	// don't enter *: start at 2nd entry.
			:  cult,	// table: array of struct, char * id is 1st member, NULL ends
			sizeof(CULT),	// entry size
			DUPOK);		/* say no msg if same verb used more than once.
		      			       NB cult entries may differ; only 1st one is entered.*/

		// do all nested tables recursively
		if (rc==RCOK)				// cut losses at (poss future) error
			for (c = cult; c->id; c++)   		// loop over entries in this table
				if (c->cs==RATE)				// if uses subsidiary table
					rc |= addVrbs( (CULT *)c->CULTP2);  	// do that table too
	}
	addVrbsLvl--;
	return rc;
}		// addVrbs
//===========================================================================
LOCAL void FC clrBeenHeres( CULT *c)

// clear BEEN_HERE flags in table and its subTables
{
	// CAUTION: do not recode with xnxC() as used therein.
	for ( ; c->id; c++)
	{
		c->IFLAGS &= ~BEEN_HERE;
		if (c->cs==RATE)
			clrBeenHeres( (CULT *)c->CULTP2);
	}
}		// clrBeenHeres


// ==================== cul.cpp error message interfaces =====================
//  primarily for use from check-functions and run code, eg in cncult.cpp.
//  Also error functions in cuparse.cpp and cutok.cpp are called directly. */
//-----------------------------------------------------------------------------
RC CDEC record::ooer( int fn, const char* message, ... )	// addl args like printf
{	va_list ap; va_start( ap, message);
	return ooerV( fn, message, ap);
}	// record::ooer
//-----------------------------------------------------------------------------
RC CDEC record::ooerV( int fn, const char* message, va_list ap)

// error message ONCE pertaining to a member (fn) of an object (rat record r):
// sets field's FsERR status bit and issues message (see oer, next), or nop's if bit already set. 12-91.

// returns RCBAD
{
	RC rc;
	[[maybe_unused]] UCH* fs = NULL;

	if (fn && (fStat( fn) & FsERR))		// always message if 0 field # given (?)
		return RCBAD;					// no more messages for field
	rc = oerI( 1, 1, 0, message, ap);	// do msg using vbl arg list flavor of oer
	if (rc && fn)				// if an error (RCOK unexpected) and fn given
		fStat( fn) |= FsERR;	// note error
	return rc;					// RCBAD
}	// record::ooerV
//-----------------------------------------------------------------------------
RC CDEC record::ooer( int fn1, int fn2, const char* message, ... )
{	va_list ap; va_start( ap, message);
	return ooerV( fn1, fn2, message, ap);
}	// record::ooer
//-----------------------------------------------------------------------------
RC CDEC record::ooerV( int fn1, int fn2, const char* message, va_list ap)

// error message ONCE pertaining to TWO members (fn1, fn2) of an object (rat record r):
// sets fields' FsERR status bits and issues message (see oer, next), or nop's if either bit already set. 3-92.
// note if one bit set, no message and other DOES NOT GET SET here.

// returns RCBAD
{
	[[maybe_unused]] UCH* fs1 = NULL;
	[[maybe_unused]] UCH* fs2 = NULL;
	if (fn1 && (fStat( fn1) & FsERR))	// always message if both field #'s 0 (?)
		return RCBAD;			// if field flagged as already having an error, do not issue another
	if (fn2 && (fStat( fn2) & FsERR))	// always message if both field #'s 0 (?)
		return RCBAD;			// if field flagged as already having an error, do not issue another
	RC rc = oerI( 1, 1, 0, message, ap);		// do msg using vbl arg list flavor of oer
	if (rc) 						// if an error (in case RCOK -- unexpected)
	{	if (fn1)
			fStat( fn1) |= FsERR;	// if field # 1 given, say have issued an error message about field
		if (fn2)
			fStat( fn2) |= FsERR;	// if field # 2 given, say have issued an error message about field
	}
	return rc;						// RCBAD
}			// record::ooerV
//-----------------------------------------------------------------------------
RC CDEC record::oer( const char* message, ... ) const	// addl args like printf
// error message pertaining to an object
// puts eg 'surface "fum" of zone "north"' in front of caller's message;
// shows input file line text, caret, file name, line in INPUT phase;
// object's file & line (from rat record) in CHECK or RUN phases. */

// returns RCBAD
{
	va_list ap;
	va_start( ap, message);
	return oerI( 1, 1, 0, message, ap);
}						// oer
//-----------------------------------------------------------------------------
RC CDEC record::oWarn( const char* message, ... ) const
// warning message pertaining to an object (rat record r)
// returns RCOK
{
	va_list ap;
	va_start( ap, message);
	return oerI( 1, 1, 1, message, ap);
}						// oWarn
//-----------------------------------------------------------------------------
RC CDEC record::oInfo( const char *message, ... ) const
// info message pertaining to an object (rat record r)
// returns RCOK
{
	va_list ap;
	va_start( ap, message);
	return oerI( 1, 1, 2, message, ap);
}						// oInfo
//-----------------------------------------------------------------------------
RC CDEC record::orer(const char* message, ...) const	// addl args like printf
// runtime error message pertaining to an object
// puts eg 'surface "fum" of zone "north"' in front of caller's message;
// shows date / time of simulation
// returns RCBAD
{
	va_list ap;
	va_start(ap, message);
	return oerI(1, 0, 0 | RTMSG, message, ap);
}						// orer
//-----------------------------------------------------------------------------
RC CDEC record::orWarn(const char* message, ...) const
// runtime warning message pertaining to an object (rat record r)
// returns RCOK
{
	va_list ap;
	va_start(ap, message);
	return oerI(1, 0, 1 | RTMSG, message, ap);
}						// orWarn
//-----------------------------------------------------------------------------
RC CDEC record::orInfo(const char *message, ...) const
// info message pertaining to an object (rat record r)
// returns RCOK
{
	va_list ap;
	va_start(ap, message);
	return oerI(1, 0, 2 | RTMSG, message, ap);
}						// orInfo
//-----------------------------------------------------------------------------
RC record::orMsg(			// flexible object-specific msg
	int erOp,		// 
	const char *message, ... ) const

// puts eg 'surface "fum" of zone "north"' in front of caller's message;
// optionally shows input file line text, caret, file name, line in INPUT phase;
// object's file & line (from rat record) in CHECK or RUN phases.

// returns RCOK iff erOp&IGNX
//    else RCBAD
{
	if (erOp & IGNX)
		return RCOK;	// "extra ignore", do nothing

	va_list ap;
	va_start( ap, message);
	int shoTx = 1;
	int shoFnLn = (erOp & SHOFNLN) != 0;
	int erAct = erOp & ERAMASK;
	int isWarn = erAct == WRN ? 1
		       : erAct == INF ? 2
			   :                0;

	return oerI( shoTx, shoFnLn, isWarn | (erOp & RTMSG), message, ap);
}	// record::orMsg
//-----------------------------------------------------------------------------
RC record::oerI(    		// object error message, inner function
	int shoTx,			// 1 to show input file line and caret if input phase
	int shoFnLn,		// 1 to show input file name and line number if input phase
	int isWarn,			// 1 for Warning, 2 for Info, 0 for Error
						//   + RTMSG = report as runtime error (via rerIV)
						//             shows date/time in simulation
	const char* fmt,	// body of error message (a la printf format)(CSE message handle ok)
	va_list ap) const	// ptr to argument ptrs if any (as for vprintf)

// puts object identification before message;
// shows current file/line in input phase,
// object's definition file/line in check or run phases */

// returns RCBAD if error (for convenience), RCOK if warning/info
{
	int bIsRuntime = (isWarn & RTMSG) != 0;
	isWarn &= 0x7;

	// verify basAnc record arg
	if (b->rt != rt)
		err( PWRN, (char *)MH_S0272);		// display internal error msg
	// "*** cul.cpp:oerI(); probable non-RAT record arg ***"

// determine file info to show
	USI _fileIx=0, _line=0;
	const char* where = "";
	if (culPhase==INPUT_PHASE)
	{
		// show input file text, ^, name, line # or not per arguments
	}
	else	// CHECK_ (or perhaps RUN_) phase
	{
		if (bIsRuntime && shoFnLn)
			where = strtprintf( " defined at %s(%d)", getFileName(fileIx), line);
		shoTx = shoFnLn = 0;	// disable showing curr input file stuff
		_fileIx = fileIx;		// show file name and line # where
		_line = line;			// ... object defintion began, if nz
	}

// prepend object identification to caller's msg
	const char* mx = strtvprintf( fmt, ap);	// format caller's msg
	const char* ms = scWrapIf(				// concat w conditional newline
					   strtprintf( "%s%s: ", 	// string identifying object, eg "wall "a" of zone "main""
							   objIdTx(),
							   where),
					   mx, "\n    ", getCpl());   	// insert .. if line longer than line length

	RC rc;
	if (bIsRuntime)
	{	rerIV(
			REG,
			isWarn, 	// 0: error (increment errCount); 1: warning (display suppressible); 2: info.
			ms);		// message text
		rc = isWarn ? RCOK : RCBAD;
	}
	else
		// use cutok.cpp fcn to finish formatting and issue message
		rc =  			// returns RCOK if isWarn, else RCBAD 7-92
			cuErv( shoTx, 	// nz to display input line
				   shoTx, 	// nz to show ^ under error position
				shoFnLn, 	// nz to show input file name and line #
				0, 		// nz to use previous not curr token re above
				_fileIx, _line,	// NULL or fileIx/line to show if shoTx/shFnLn 0
				isWarn, 	// 0 Error, 1 Warning, 2 Info
				ms);	// message text, args already inserted.

	return rc;
}		// record::oerI
//-----------------------------------------------------------------------------
const char* record::mbrIdTx( int fn) const	// record field id from cult table
{
	return culMbrId( b, fn);
}		// record::mbrIdTx
//===========================================================================
const char* FC culMbrId( BP b, unsigned int fn)	// return record field name from cult table for use in error message

{
#if 0 //2-95 check only on failure, below, so can be used for members of the nested cult for nested-object error messages.
x    if (xSp != xStk)			// xnxC, nxRat etc work as desired here only at top level.
x       err( PWRN,				// display internal error msg
x            (char *)MH_S0275,    		// "cul.cpp:culMbrId called with xSp %p not top (%p)"
x            (void *)xSp, (void *)xStk );	// casts are to make far.
#endif


	// use basAnc's associated CULT, 3-24-2016
	if (b->an_pCULT)		// if basAnc has an associated CULT
	{	for (const CULT* c=b->an_pCULT; c->id; c++)
		{	if (c->cs==DAT && c->fn==fn)
				return c->id;
		}
	}

	const char* tx = NULL;
	for (CULT *c = NULL;  xnxC(c);  )	// loop CULT entries, using xStk[0], making addl xStk entries
		if (
#if 1	// another try, 4-9-2013
			(xSp->b->rt==b->rt			// match rt not b so run basAncs, types basAncs work
			 || xSp->b == b)			// also match b re ambiguous fn (e.g. among surface, door, window)
#elif 0	// experiment re ambiguous fn (e.g. among surface, door, window), 3-9-2012
x			xSp->b == b					// seems to work for types basAncs?
#else
x			xSp->b->rt==b->rt			// (match rt not b so run basAncs, types basAncs work)
#endif
		 &&  c->cs==DAT
		 &&  c->fn==fn)			// if data entry for desired field of basAnc of desired type
			if (!tx)  						// if first match
				tx = c->id;					// remember it
			else if (_stricmp( tx, c->id))  		// additional match: ignore if same name
				return strtprintf( (char *)MH_S0276, tx, (char *)c->id);
	// "[%s or %s: table ambiguity: recode this error message to not use cul.cpp:culMbrId]"

	if (tx)						// if name found
		return tx;				// return it. Is unique if here.

	/* Issue message if not top level and not found: xnxC,nxRat only search all tables when called from top xSp level.
	   No message if found (a change 2-95), so can be used without complaint re error messages for nested
	   objects provided the field sought is in the accessible nested portion of the CULT table structure. */

	if (xSp != xStk)			// xnxC, nxRat etc work as desired here only at top level.
		err( PWRN,				// display internal error msg
			(char *)MH_S0275,    			// "cul.cpp:culMbrId called with xSp %p not top (%p)"
			(void *)xSp, (void *)xStk );	// casts are to make far.

	return strtprintf( (char *)MH_S0277, 	// not found. "[%s not found by cul.cpp:culMbrId]".
			MNAME( b->fir + fn) ); 			//  punt, using name of member of record structure.
}	// culMbrId
//===========================================================================
const char* FC quifnn( char *s)	// quote if not null, & supply leading space

// CAUTION: returned value in Tmpstr is transitory
{
	return (s && *s)  ?  strtprintf( " '%s'", s)  :  "";
}								// quifnn
//===========================================================================

// end of cul.cpp

