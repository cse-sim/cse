// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// exman.cpp: CSE expression manager

// 6-95: search NUMS for new texts to extract
// note 2-94: rer() error action made to work again. remove WRN etc from calls where not desired.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// ULI SI

#include <MSGHANS.H>	// MH_V0035, MH_E0090

#include <MESSAGES.H>	// msgI
#include <SRD.H>	// sRd[] Untab Dttab sFdtab; BC3 needs b4 rccn.h for SFIRstr; GetDttab.
#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// Top, for  .iHr .isWarmup .dateStr  for error msgs

#include <CVPAK.H>	// cvExtoIn cvLmCk
#include <RMKERR.H>	// errI

#include <CNGUTS.H>	// Top
#include <CUEVAL.H>	// cuEvalR, RCUNSET
#include <CUL.H>	// FsVAL
#include <CNCULT.H>
#include <CUEVF.H>	// evf's and variabilities: EVF____ defines and EVENDIVL
#include <CUPARSE.H>	// exOrk TYSTR TYSI TYLLI. cueval.h rqd 1st.
#include <CUPARSEX.H>	// maxErrors. only, 3-91.
#include <EXMAN.H>	// public decls for this file


/*-------------------------------- OPTIONS --------------------------------*/

#define NOINCREF	// define to not increment reference count (cupIncRef) of expression string values here, rob 5-97.
/* story: believe all string values cupIncRef'd at origin in cueval.cpp: PSRATLODS, PSEXPLODS. where else?
   Yet there are cupIncRef's here, indicated as a bug fix 5-25-97 (no dates remain 5-97 at cueval cupIncRefs.).
   history: 5-97 problem occurred with excess ref count for @Top.dateStr in UDT report; added NOINCREF;
   didn't fix (problem was bypassing decref when !isChanged)
   but leave defined (looks right) til counter-reason found. */

#undef STATREF	// define to restore possibility of expressions at static locations as well as in basAnc records. 12-4-91.
// undefined enables use of compact RECREF structure.
// much editing needed to restore; not all new fcn versions have ifndef's.


/*-------------------------------- COMMENTS -------------------------------*/

/* a NANDLE is a 32-bit quantity that is not a valid floating point
	number or string pointer, used to specify the "expression number"
	for data whose value is not known at input time, and also to specify
	UNSET (no value assigned yet).

	During input, NANDLES are stored in place of string and float
	values and moved and copied freely; after input is set up, the data
	is searched for NANDLES to build the expression table that drives
	evaluating and storing expressions during the run.  (SI data is
	extended to 32 bits (TYLLI) where necessary to allow for NANDLEs.)

	The bit format of a NANDLE is: 0xFF000000 + n:
 n = 0 to indicate unset;
 n = expression number 1-16383 to indicate place to store expression.

	CAUTION: NANDLE format is system and software dependent:
	depends on IEEE floating point format (hi word FF00 is Not-A-Number);
	depends on IBM memory allocation (segment FF00 will not occur in
 data pointers as ROM is there);
	depends on user caution to limit SI data stored in LI to 16 bits
 to insure data not looking like a NANDLE. */


/*--------------------------- DEFINES and TYPES ---------------------------*/

// see cnglob.h for: NANDLE, NANDAT, UNSET, EXN, ISUNSET, ISNANDLE

#if 0 && defined( _DEBUG)
#define EXTDUMP			// define to enable debug printfs
#define EXTDUMPIF( t) extDump( t)
#else
#define EXTDUMPIF( t)
#endif

struct RECREF
{
	unsigned ancN;		// anchor # (idx of record's basAnc)
	unsigned i;			// record subscript
	unsigned o;			// offset within record (TODO: why not fn?)
#if defined( EXTDUMP)
	void dump() const;
#endif
};


/*---------- WHERE: identifies a static place or basAnc record member ----------*/

/* used in Expression Table, to identify places to store expression
   value and increment change flags each time value changes. */

#ifdef STATREF
0 typedef struct
0{
0	enum WPCS { whcsNUL, whcsSTAT, whcsRAT } cs;	// case: static or RAT
0	union
0	{	NANDAT *p;	// for whcsSTAT: static location
0		struct		// for whcsRAT
0		{	USI ancN;	// basAnc number of registered rat
0 			// >>>> replace ancN with pointer 2-91??: baseAncs now NEAR.
0           TI i;  	// RAT entry subscript
0 			USI o;	// offset of subject member
0		} r;
0	} w;
0} WHERE;
#else
typedef RECREF WHERE;	// use ancN - i - o struct above; combine into one type?
#endif


/*---------- Expression Table ----------*/

struct EXTAB	// expression table (*exTab[i]) struct
{
	SI nx;			// 0 or subscript of next entry in evaluation order (chain), -1 if deleted entry.
	PSOP *ip;    	// ptr to pseudo-code dm block.  PSOP: cueval.h/cnglob.h.  (was void * b4 bcpp, 2-91)
	USI evf;		// evaluation frequency (EVF___ defines, cuevf.h)
	USI useCl;		// caller's use class bits for selective eval (EVBEGIVL
	USI ty;			// data type TYLLI/TYFL/TYSTR/[TYSI/]TYCH/TYNC, cuparse.h.
	UCH fdTy;		// (0 or) target field type (sFdtab[] subscript) 2-91
	UCH flags;		// alignment filler / avail / future flags etc
	// original destination of expr, for runtime error msgs (see whatEx()):
	SI srcIsType;	//  non-0 if a 'type' (see cul.cpp)
					//    suspect can now 12-91 use b->ba_flags & RFTYS; must check cul.cpp carefully.
	BP srcB;		//  baseAnc where expression originally input
	TI srcI;		//  record subscript where input
	SI srcFn;		//  field number where input
	USI evfOk;		//  permitted evaluation frequency for this expression -- for possible future uses 12-91
	NANDAT v;		// most recent value, or UNSET.  [hi 0 for SI data].
	USI whValN;		// number of locations to fill with value
	USI whValNal; 	// allocated size of whVal[]
	WHERE *whVal;	// NULL or ptr to dm array of info about places to put value each time it changes
	USI whChafN;  	// number of increment-on-change locations
	USI whChafNal;	// number of ditto allocated (size of whChaf[])
	WHERE *whChaf; 	// NULL or ptr to dm array of info about locations to ++ when value changes (Change flags)

	void extEfree();
#if defined( EXTDUMP)
	void extDump1() const;
#endif
};	// struct EXTAB
LOCAL EXTAB* exTab = NULL;	// NULL or dm ptr to expr table (array of EXTAB)
LOCAL USI exNal = 0;	// allocated size of exTab[]
LOCAL USI exN = 0;		// used ditto = next avail expr # less 1
// (0 not used for expr: expr # 0 means unset; entry 0 .nx is head of eval order list)
LOCAL SI NEAR exTail = 0;	// subscript of last exTab entry in eval order; 0 for empty table

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
// (expect to make some public as required)
LOCAL RC       FC NEAR uniLim( USI fdTy, USI ty, void *p);
LOCAL RC       FC NEAR extEntry( BP b, TI i, USI fn, USI *ph);
LOCAL RC       FC NEAR extAdd( USI *ph);
LOCAL RC       FC NEAR addStore( USI h, WHERE w);
LOCAL RC       FC NEAR exEvUp( USI h, SI isEoi, SI silentUnset, USI *pBadH);
LOCAL NANDAT * FC NEAR pRecRef( RECREF rr);
LOCAL const char*   FC NEAR txVal( SI ty, void *p);
LOCAL const char*   FC NEAR whatRr( RECREF rr);

/*================================ COMMENTS ===============================*/

/* exman.cpp DATA TYPES at user level, for use in calls to exPile:

TYSTR	string:  char * in user storage, private dm copy ??
TYFL	float.
TYFLSTR	float or string 11-91
TYLLI	limited long integer: LI user storage but user interface and exman
	support 16 bit values only.  Generally accessed with (SI) cast (but
	you could store 32 bit data after exWalkRecs done).
TYSI	Integer in 16 bit storage: user interface supports only
	constant values and end-of-input variation, as cannot store nan's in them.
*/


//=============================================================================
// debugging fcns
#if defined( EXTDUMP)
static void extDump(const char* tag/*=""*/)
{
	printf("\nExTab %s   exNal=%d", tag, exNal);
	if (exTab) for (int iEX = 0; iEX <= exN; iEX++)
	{
		const EXTAB* pEX = exTab + iEX;
		pEX->extDump1();
	}
	printf("\n");

}		// extDump
//-----------------------------------------------------------------------------
void EXTAB::extDump1() const		// debug aid: printf EXTAB info
{
	int iEx = this - exTab;
	printf("\n%d: %s[%d] fn=%2d  nx=%2d ip=%p evf=%d", iEx,
			srcB ? srcB->what : "?", srcI, srcFn, nx, ip, evf);
	for (int iW = 0; iW < whValN; iW++)
		(whVal + iW)->dump();

}		// EXTAB::extDump
//-----------------------------------------------------------------------------
void RECREF::dump() const
{
	BP b = basAnc::anc4n(ancN, WRN);
	printf("  %s[%d] %d", b->what, i, o);
}		// RECREF::dump()
#endif
//-----------------------------------------------------------------------------
static RC extValidate(
	const char* tag)	// caller info included in msg
// returns RCOK if no exTab checks failed
//    else RCBAD / program will not run
{
	RC rc = RCOK;
	if (exTab)
	{	int iEX;
		EXTAB* pEX;
		// 0 flags in all EXTABs and count deleted entries
		int nDeleted{ 0 };
		for (iEX = 0; iEX <= exN; iEX++)
		{	exTab[iEX].flags = 0;
			if (exTab[iEX].nx < 0)
				nDeleted++;
		}

		// verify that each EXTAB appears only once in nx chain
		int nSeen{ 0 };
		int nLoop{ 0 };
		pEX = exTab;
		// follow nx chain until terminator (0) or deleted (<0)=error
		while (nSeen < exN && pEX->nx > 0)
		{	++nSeen;
			pEX = exTab + pEX->nx;
			if (pEX->flags++ != 0)
			{	nLoop++;
				break;
			}
		}
		const char* msg{ nullptr };
		if (nLoop)
			msg = "processing order contains loop(s)";
		else if (nSeen + nDeleted != exN)
			msg = strtprintf("length=%d (expected %d)", nSeen+nDeleted, exN);
		if (msg)
			rc = err(PWRN, "exman.cpp:extValidate (called from %s): Invalid expression table -- %s", tag, msg);
	}

	return rc;
}	// extValidate
//===========================================================================
void FC exClean(	// exman.cpp cleanup routine

	[[maybe_unused]] CLEANCASE cs )	// ENTRY, DONE, or CRASH. type in cnglob.h.

// added for DLL version that should free every bit of memory. Rob 12-3-93.
{
	extClr();				// clear expression table (below). may leave allocated.
	exNal = 0;				// say no expression table entries allocated
	dmfree( DMPP( exTab));		// deallocate expression table block, if any.
}				// exClean

//===========================================================================
RC FC exPile(		// compile an expression from current input

	SI toprec,		// -1 for default or precedence to which to evaluate
	USI wanTy,		// desired cul data type: TYLLI,TYFL,TYSTR,TYSI,TYCH(2 or 4 byte),TYCN,TYFLSTR,TYID (returns TYSTR).
	USI choiDt,		// dtypes.h choice data type (DTxxx) when wanTy is TYCH or TYCN.  2-92
	USI fdTy,		// 0 (FDNONE?) or target field type (sFdtab[] subscr), for units factor and limits check.
	USI _evfOk,		// ffff or acceptable eval freq bits for context, including eval-at-end-interval bit, EVENDIVL.
	USI useCl,		// caller's "use class" bit(s) for selective expr eval, now (1995) always 1.
					//   changed here to UENDIVL if expr has EVENDIVL on.
	char *_ermTx,	// NULL or text saying what expr is arg to,
					// for (compile) errMsgs in form "after <ermTx>".
	SI isType, BP b,  TI i,  SI fn,	// type flag, baseAnc, rec subscr (for name - fetched now), and fld #
									// of destination, for RUNTIME ERROR MESSAGES.
									// DON'T MATTER for non-field calls: not used if evfOk 0 or expr constant. */
									// >>> OR should we save ermTx, to show cult not fir member names?
									// review what callers pass for ermTx. 2-91 <<<<
	NANDAT *pDest, 	// rcvs constant value (LI, float, char *, or SI), or nandle for non-constant expr
    				// (which is saved for access by the nandle) (not allowed for TYSI).
	USI *pGotTy,	// NULL or receives actual data type gotten (useful with wanTy = TYFLSTR).
	USI *pGotEvf )	// NULL or receives actual eval frequency (variation) of expression compiled

/* NOTE that if nandle returned, caller must later register its location
   (after moving & copying as desired) with exReg (or via exWalkRecs)
   in order for location(s) to be set to expr each time it changes. */

/* on RCOK return, tokTy reflects terminator and , ; ) ] terminators passed.
   on error return, msg issued and rest of input statement passed (we hope).*/
{
	USI gotTy, gotEvf, h;
	SI isKon;
	NANDAT v=0;
	PSOP *ip;
	EXTAB *ex;
	RC rc;

// arg checks

	if (_evfOk & ~(EVEOI|EVFFAZ))	// if variability other than const, end-of-input, or end-input + end-autosize spec'd

		if (wanTy==TYSI || wanTy==TYCH && choiDt & DTBCHOICB)	// 16-bit ints and choicb choices cannot hold NANDLEs
		{
			perlc( (char *)MH_E0090, (UI)_evfOk );	// devel aid. perlc: issue parse errMsg w line # & caret, cuparse.cpp
          											// "exman.cpp:exPile: Internal error: bad table entry: \n"
          											// "    bad evfOk 0x%x for 16-bit quantity"
			_evfOk &= (EVEOI|EVFFAZ);			// fix & continue
		}
	if (wanTy & TYCH  &&  !(choiDt & (DTBCHOICB|DTBCHOICN)))
		return perlc( (char *)MH_E0091, (UI)wanTy, (UI)choiDt);	/* "exman.cpp:exPile: Internal error: \n"
       								   "    called with TYCH wanTy 0x%x but non-choice choiDt 0x%x" */
	if ((wanTy & (TYCH|TYFL))==(TYCH|TYFL))
		if (!(choiDt & DTBCHOICN))
			return perlc( (char *)MH_E0092, (UI)wanTy, (UI)choiDt);	/* "exman.cpp:exPile: Internal error: \n"
								   "    called with TYNC wanTy 0x%x but 2-byte choiDt 0x%x" */
	/* compile expression from current input file to pseudocode OR constant value */

	CSE_E( 				/* CSE_E: if not RCOK, return error code rc now. */
		exOrk(   			/* cuparse.cpp */
			toprec, 			/* terminating precedence, exOrk defaults. */
			wanTy==TYLLI ? TYSI : wanTy,	/* for limited-long-integer get 16-bit value (4 byte size used to hold nandle) */
			choiDt,
			_evfOk,
			_ermTx,
			&gotTy,				/* rcvs actual data type found 11-91 */
			&gotEvf,   			/* rcvs evaluation frequency bits */
			&isKon, 			/* receives is-constant flag */
			&v,	 			/* iff const, rcvs constant value (4 bytes even for SI; dm ptr for TYSTR) */
			&ip ) )   			// rcvs NULL if constant, or ptr to pseudo-code in dm
	// leaves tokTy set; comma ; ) ] terminators gobbled.

	/* check/return constant now */
	if (isKon)
	{
		CSE_E( uniLimCt( fdTy, gotTy, _ermTx, &v) )	// check limits & apply units, with errMsg suitable for compile time. below.

		if ( gotTy==TYSI				// return the constant value in destination.  SI has 16 bit storage only
		||  gotTy==TYCH && choiDt & DTBCHOICB )	// choice types with this bit on are 16 bits only
		{
			*(SI *)pDest = (SI)(LI)v;		// return lo 16 bits of value
			if (ISNCHOICE(v))			// redundantly check for not a 4-byte choice value (cnglob.h macro)
				perlc( (char *)MH_E0093);		/* "exman.cpp:exPile: Internal error:\n"
             					   "    4-byte choice value returned by exOrk for 2-byte type".  devel aid */
		}
		else
			*pDest = v;				// all other types return 32 bits

		extDelFn( b, i, fn);  			// delete any pre-existing expression table entry for this member
	}

	/* if not an already-evaluated constant, enter in expr table, return nandle in caller's destination. */

	else
	{
		EXTDUMPIF("exPile start");
		CSE_E( extEntry( b, i, fn, &h) )  		// find existing entry to reuse, or add new expr table entry, return h / rif error

		// if got an "evaluate-at-end/post-interval" expression, indicate in use class for eval at correct time
		if (gotEvf & (EVENDIVL|EVPSTIVL))
			useCl = gotEvf & (EVENDIVL|EVPSTIVL);	// overwrites any other distinction caller may have had.  cncult.h.
		// start/end interval is really independent of evf and ucl; separate if found necessary.
		// fill expression table entry
		ex = exTab + h;				// point expression table entry
		ex->ip = ip;				// set pseudo-code ptr for later eval
		ex->evf = gotEvf;
		ex->useCl = useCl;
		ex->ty = (wanTy==TYLLI && gotTy==TYSI) ? TYLLI : gotTy;
		if ((fdTy & 0xff00) != 0)
			err( PWRN, (char *)MH_E0094); 	/* display internal error msg "exman.cpp:expile: fdTy > 255, change UCH to USI",
          					   wait for key. rmkerr.cpp. Note fdTy still UCH in srd.h, 6-95. */
		ex->fdTy = (UCH)fdTy;
		// next 4 members identify bad thing in runtime errMsg (eg limits), also used to store value at end-of-input eval:
		ex->srcIsType = isType;
		ex->srcB = b;
		ex->srcI = i;						// identify source record by subscript 12-91
		ex->srcFn = fn;
		ex->evfOk = _evfOk;					// save expr's acceptable variablity re poss fixups, 12-91.
		ex->v = UNSET;						// init value to "not set"

		// maintain evaluation order list (later rearranged to fix dependencies)
		if (ex->nx <= 0)	// if not on list
		{	// add to end
			exTab[exTail].nx = h;
			ex->nx = 0;
			exTail = h;  	// put on end of evaluation order list.
			// exTail is initially 0 so exTab[0].nx is head of list.
		}
		// else already on list (new expr for previously seen field)

		// Put nandle for expr # in 32-bit destination (16-bit destinations must be EVEOI|EVFFAZ only; can't hold nandle)
		if ( wanTy != TYSI					// integers are 16 bits only (insurance -- checked at entry)
		  &&  !(wanTy==TYCH && (choiDt & DTBCHOICB)) ) 		// choice types with this bit on are 16 bits only
			*pDest = NANDLE(h);
		/* nandle identifies expression # to which to later set locations containing it.  After end of input,
		   program moves data as desired, then exWalkRecs finds nandles in run rats & registers their locations for ExEvEvf.
		   Don't register pDest here: nandles in input locations remain for later runs (except EVEOI|EVFFAZ evaluation). */
		EXTDUMPIF( "exPile new end");
	}

#if 0 && defined( _DEBUG)
	extValidate( "exPile");
#endif

	if (pGotTy)
		*pGotTy = gotTy;		// return actual type gotten
	if (pGotEvf)
		*pGotEvf = gotEvf;	// return actual evf gotten
	return rc;			// addl return(s) above, including CSE_E macros
}		// exPile

//===========================================================================
RC FC uniLimCt(

// check limits & apply units, with errMsg suitable for compile time

	USI fdTy,		// target field type for data type, units, and limits. 0 (FDNONE?) for no scaling or check
	SI ty, 		// cul data type (TYFL,TYSTR,TYSI,etc): used in displaying data in msg
	char *_ermTx,	// text describing what expr is, for errmsgs.  Should not let _ermTx get here NULL.
	void *p )		// pointer to data (ptr to ptr for TYSTR)
{
	RC rc = uniLim( fdTy, ty, p);  	// do it, below.  Issues no messages.
	if (!rc)				// if error
		return RCOK;
	// else error

	/* uniLim rets lib\cvpak.cpp code for no-arg msg for limit error.
	   Its text says what the limits are but does not show the bad value nor identify it
	   (and cannot readily change and still share the code with TK/CR).
	   Generate a msg showing value, identification, AND cvpak's explanation. */

	return perNx(		/* format msg like printf, and add input text, ^ under error, file name, line #.
				   Gobbles input to start next statement.  returns RCBAD.  cuparse.cpp */
		(char *)MH_E0095,		// "Bad %s: %s: %s"
		_ermTx ? _ermTx : "",	// in case _ermTx is NULL (unexpected/fix call if found)
		txVal( ty, p),    		// text for the bad value (below)
		msg( NULL,				// text for MH (in rc) to Tmpstr (messages.cpp)  SHD BE OK TO pass han to perNx now 3-92.
			(char *)(LI)rc));	//   MH in low byte of char *
}				// uniLimCt
//===========================================================================
LOCAL RC FC NEAR uniLim(
	USI fdTy, 	// 0 or field type (sFdtab index, as from b->fir[fn].fdTy
	USI ty,		// cul data type; TYNC significant here
	void *p )	// pointer to value (to ptr for strings)

// does NOT display error message
{
	if (fdTy==0)			// if no field type (does this happen? 2-91)
		return RCOK;			// ok nop.  CAUTION: fdTy 0 has dt 0 which wd not necessarily fall thru ok  2-91.

	USI dt = sFdtab[fdTy].dtype;	// fetch data type from small fields table (table in srfd.cpp, generated by rcdef.exe)
	int units = sFdtab[fdTy].untype;	// fetch units ditto

	/* note units before limits is cvpak's historical order.
	   Believe are no flds with non-0 limits & non-1.0 units factor anyway, 2-91.*/

	/* divide by units factor: scale to internal unit system if different */

	if (units != UNNONE)				// for runtime speed
		if (dt==DTFLOAT || ty==TYNC)			// only done for FLOATs and doubles and number/choices
		{
			if (ISNUM(*(void**)p))						// and not if (number/choice) value is a nan
				*(float *)p = float( cvExtoIn( *(float *)p, units ));
		}
		else if (dt==DTDBL)						// doubles
			*(double *)p = cvExtoIn( *(double *)p, units); 	// cvpak.cpp

	/* check string length for selected string types.  Historically, cvpak did this by data type (not limit type)
							   in cvs2in's dtype switch, which isn't used if here */
	if (dt==DTANAME)
		if (strlen( *(char **)p) > DTANAMEMAX)
			return MH_V0035;		// "V0035: name must be 1 to 63 characters"
	// return MH code for consisency with cvpak errors

	/* check that value is within limits (numerical types) */

	return cvLmCk( dt, sFdtab[fdTy].lmtype, p); 		// cvpak.cpp.
	// on error, issues no message, returns mh of no-arg text explaining the limits exceeded.
}		// uniLim

//===========================================================================
LOCAL RC FC NEAR extEntry( BP b, TI i, USI fn, USI *ph)

// find existing exTab entry (new expr for old field) else allocate a new one

// non-RCOK if error (out of memory)
{
	RC rc = RCBAD;
	for (USI h = 1;  h <= exN;  h++)				// loop expression table entries
	{
		EXTAB *ex = exTab + h;
		if (b==ex->srcB  &&  i==ex->srcI  &&  fn==ex->srcFn)	// if is sought entry
		{	// new expression for previously seen field
			ex->extEfree();		// free stuff this entry points to: will be overwritten
								// note: nx is >0 (entry is in eval chain)
			*ph = h;
			rc = RCOK;
			break;
		}
	}
	if (rc != RCOK)
		rc = extAdd(ph);	// not found, allocate a new one
	EXTDUMPIF("extEntry");
	return rc;
}			// extEntry
//===========================================================================
LOCAL RC FC NEAR extAdd( USI *ph)

// allocate exTab entry and return expression number
{
#define EXTAB_NADD 16		// make 50?

// use next entry at end if there is one - fastest
	if (exNal > exN+1+1)		// +1 for 1-based exN, +1 for ++ next
	{
		*ph = ++exN;
		return RCOK;
	}

// search for a deleted entry
	for (USI h = 0;  ++h <= exN; )
	{
		if (exTab[h].nx== -1)
		{
			*ph = h;
			return RCOK;			// caller is expected to make .nx >= 0
		}
	}

// expression table is full, or not yet allocated.  (re)alloc it and assign next expresion number.
	RC rc;
	USI nuNal = exNal + EXTAB_NADD;
	CSE_E( dmral( DMPP( exTab),
				nuNal*sizeof(EXTAB),
				WRN|DMZERO ) )		// zero added space; return bad if out of memory
	exNal = nuNal;
	*ph = ++exN;			// 0 is not used (h==0 means unset; exTab[0].nx is head of eval order list)
	return RCOK;

#undef EXTAB_NADD
}			// extAdd
//===========================================================================
void FC extDelFn( BP b, TI i, SI fn)	// delete expression table entry if any for given field
{
	USI h;
	if (exTab)						// if expr table allocated 1-92
	{
		for (USI hm1 = 0;  (h = exTab[hm1].nx) != 0;  )	// loop over exprs in evaluation list order. note hm1=h at end loop.
		{
			if (exTab[h].srcFn==fn)    			// test 1 condition asap for speed: fcn is called every expr compiled.
			{
				EXTAB *ex = exTab + h;
				if (ex->srcB==b && ex->srcI==i /*&& ex->srcFn==fn*/)	// if is entry to delete
				{
					exTab[hm1].nx = ex->nx;   			// unlink
					ex->nx = -1;    					// flag free
					if (exTail==h)  					// if it was the end of the list
						exTail = hm1;					// now the one before it is end
					continue;						// bypass hm1 = h.
				}
			}
			hm1 = h; 			// loop increment: advance in list.  Note don't get here after unlinking entry to delete
		}
	}
}		// extDelFn
//===========================================================================
void FC extDel( record *e)	// delete all expression table entries for given record
{
	USI h;
	if (exTab)						// if expr table allocated 1-92
	{
		for (USI hm1 = 0;  (h = exTab[hm1].nx) != 0;  )	// loop over exprs in evaluation list order. note hm1=h at end loop.
		{
			EXTAB *ex = exTab + h;
			if (ex->srcB==e->b  &&  ex->srcI==e->ss) 	// if is an entry to delete
			{
				exTab[hm1].nx = ex->nx;   			// unlink
				ex->nx = -1;    			// flag free
				if (exTail==h)				// if it was the end of the eval order list
					exTail = hm1;			// now the one before it is end
			}
			else
				hm1 = h;			// loop increment: advance in list.  Note don't get here unlinking entry to delete
		}
	}
}		// extDel
//===========================================================================
void FC extAdj( BP b, TI minI, TI delta)	// add delta to all b subscripts >= minI in expression table
{
	for (EXTAB *ex = exTab + exN;  ex > exTab;  ex--)
		if (ex->srcB==b  &&  ex->srcI >= minI)
			ex->srcI += delta;
}				// extAdj
//===========================================================================
void FC extMove( record *nuE, record *e)	// change expression table entries for given record to given new record
{
	for (EXTAB *ex = exTab + exN;  ex > exTab;  ex--)
	{
		if ( ex->srcB==e->b  &&  ex->srcI==e->ss  	// if entry is for given record
		&&  ex->nx >= 0 )				// if entry not deleted
		{
			ex->srcB = nuE->b;
			ex->srcI = nuE->ss;
			ex->srcIsType = (nuE->b->ba_flags & RFTYS) != 0;
		}
	}
}		// extMove
//===========================================================================
void FC extDup( record *nuE, record *e) 	// duplicate expression table entries for given record to given new record
{
	USI exNwas = exN;
	for (USI h = 1;  h <= exNwas;  h++)				// loop expressions already in table
	{
		EXTAB *ex = exTab + h;
		if ( ex->srcB==e->b  &&  ex->srcI==e->ss    		// if expr in given record
		&&  ex->nx >= 0)					// if not deleted exTab entry
		{
			USI off = ex->srcB->fir[ex->srcFn].off;
			if ( ex->ty==TYSI 					// if integer (too small for nandle)
			 || *(void **)((char *)e + off)==NANDLE(h))		// or field has correct nandle -- insurance
			{
				USI nuH;
				if (extAdd(&nuH)==RCOK)				// add exTab entry / if ok
				{
					BP nuB = nuE->b;
					EXTAB *nuEx = exTab + nuH;
					ex = exTab + h;					// repoint: may have moved

					// initialize duplicate exTab entry
					memcpy( nuEx, ex, sizeof(EXTAB) );	// start by copying old
					dmIncRef( DMPP( nuEx->ip));			// increment reference count to (same) pseudo-code block, 4-14-92
					nuEx->srcB = nuB;					// update for new source
					nuEx->srcI = nuE->ss;
					nuEx->srcIsType = ((nuB->ba_flags & RFTYS) != 0);	// determine whether new record is type
					nuEx->v = UNSET;					// value: not evaluated yet
					dmfree( DMPP( nuEx->whVal));    	// new expression has no uses yet,
					dmfree( DMPP( nuEx->whChaf));    	// and no registered increment-on-change locations
					nuEx->whValN = nuEx->whValNal = nuEx->whChafN = nuEx->whChafNal = 0;	// ..   this line missing til 5-92

					// append to evaluation order list
					exTab[exTail].nx = nuH;
					nuEx->nx = 0;
					exTail = nuH;

					// put its expression handle in its record member
					if (nuEx->ty != TYSI)
						*(void**)((char *)nuE + off) = NANDLE(nuH);
				}
			}
		}
	}
	EXTDUMPIF( "extDup");
}		// extDup
//===========================================================================
void FC extClr()	// clear expression table
{
	if (exTab)
	{
		for (EXTAB *ex = exTab;  ex++ < exTab + exN;  )	// loop registered expressions
			ex->extEfree();			// free stuff this entry points to
		memset( exTab, '\0', exN * sizeof(EXTAB) );	// zero table: insurance.  includes exTab[0].nx: eval order list head
		// leave block allocated for next run (but exClean above frees it).
	}
	exN = 0;					// say no expressions in table
	exTail = 0;					// ..
}		// extClr
//===========================================================================
void EXTAB::extEfree()		// free heap stuff used by this expression
{
	// string expression
	if (ty == TYSTR)
	{	// free pointers set by this expression in records
		//   else record pointers left dangling
		//   can e.g. mem fault in record destructor
		//   new 3-3-2016
		for (int i = 0; i < whValN; i++)
		{	NANDAT* pVal = pRecRef( whVal[i]);	// get pointer to basAnc record member, or NULL
			if (pVal)				// in case pRat errored
				cupfree( (void **)pVal);	// dec ref count or free block of old ptr if not inline in code nor UNSET, cueval.cpp
		}
		// free expression's string value
		cupfree( DMPP( v)); 	// nop if string inline in pseudocode
	}

	dmfree( DMPP( ip));
	dmfree( DMPP( whVal));
	dmfree( DMPP( whChaf));
	whValN = whValNal = whChafN = whChafNal = 0;
}		// EXTAB::extEfree
//-----------------------------------------------------------------------------

//===========================================================================
RC FC exClrExUses(	// re-init old expr table entries for next run

	// clears registered uses & change flags in expr table; sets prior value to UNSET; keeps the expressions themselves.
	// call between runs, before new input decoding (cuz input stuff can can add change flags), and between phases

	BOO jfc )	/* TRUE if call is just a phase change, ie between autoSize and main simulation phases:
		   retain expr uses in records that are not regenerated, per RFPERSIS flag. 10-95. */
{
	for (EXTAB *ex = exTab;  ex++ < exTab + exN;  )	// loop registered expressions
	{
		if (!jfc)		// if doing full clear, set like newly-init expr tbl entry (but retain alloc'd storage).
		{
			if (ex->ty==TYSTR)		// if string value
				cupfree( DMPP( ex->v)); 		// if in dm (not inline in code, not UNSET), decr ref count or free, NULL ptr. cueval.cpp.
			ex->v = UNSET;    		// set prior value to "unset", to be sure stored/chaf'd when first evaluated
			ex->whValN = 0;   		// say no uses of expression value (but leave whVal allocated)
			ex->whChafN = 0;   		// say no change flags registered
		}
		else			// phase change: conditionally keep use and change flag registrations and value. 10-95.
		{
			// conditionally (?) discard value.
			//    runtime exprs (EVFRUN..EVFSUBHR): must unset here to be sure stored 1st time in new run records.
			//    EVEOI,EVFFAZ: expect usually resolved in input records; might be accessed later by expr with multiple probes.
			//      EVFFAZ: expect re-eval b4 use due to expr tab ordering; unset to be sure stored; remove if case found.
			if (ex->evf != EVEOI)		// EVEOI won't be eval'd/stored again, but might be PSEXPLOD4/S'd, so tentatively keep.
				ex->v = UNSET;    		// set prior value to "unset", to be sure stored/chaf'd when first evaluated

			// keep expr uses in recs that are NOT free'd tween phases (per basAnc RFPERSIS flag, set eg from cncult2.cpp).
			WHERE *w;
			BP b;
			USI i, j = 0;
			for (i = 0;  i < ex->whValN;  i++)  		// loop over use registrations for this expr
			{
				w = ex->whVal+i;
				b = basAnc::anc4n( w->ancN, WRN);   	// point basAnc of record of expr use
				if (b  &&  b->ba_flags & RFPERSIS)		// if basAnc has flag set, keep this expr use entry
				{
					if (j < i)				// if any uses have been deleted,
					{
						ex->whVal[j] = *w;			// copy this one down to keep compact
						memset( w, 0, sizeof(*w));		// zero vacated space: insurance
					}
					j++;					// count uses retained / next copy-back subscript
				}
				else					// drop this expr use entry: no j++
					memset( w, 0, sizeof(*w));		// zero vacated space: insurance
			}
			ex->whValN = j;				// store number of uses retained. excess space not dealloc'd.

			// keep change flag registrations in recs that are NOT free'd tween phases (tho suspect ok to free all, 10-95)
			j = 0;
			b = nullptr;
			for (i = 0;  i < ex->whChafN;  i++)  		// loop over change flag registrations for this expr
			{
				w = ex->whChaf+i;
				b = basAnc::anc4n( w->ancN, WRN);	// point basAnc of record of change flag
				if (b && b->ba_flags & RFPERSIS)		// if basAnc has flag set, keep this entry
				{
					if (j < i)				// if any have been deleted,
					{
						ex->whChaf[j] = *w;			// copy this one down to keep compact
						memset( w, 0, sizeof(*w));		// zero vacated space: insurance
					}
					j++;					// count those retained / next copy-back subscript
				}
				else					// drop this one: no j++
					memset( w, 0, sizeof(*w));		// zero vacated space: insurance
			}
			ex->whChafN = j;				// store number retained. excess space not dealloc'd.
		}
	}
	return RCOK;
}		    // exClrExUses

//===========================================================================
RC FC exWalkRecs()

// "Walk" the records of all basAncs and 1) issue messages for unset data 2) register any run-time expression uses found.

/* Looks dumbly at: all 4-byte members, plus all VALNDT type members, and tests for NANDLES:
	a) the value UNSET, possibly stored many places before input;
	b) expr handle values returned when expr value not known at input.
	   Data type of destination now containing NANDLE must match that
           originally used to input expression -- no checking here. */

// returns non-RCOK if error (eg out of memory)
{
	USI os[450];    		// 200->300 after overflow chip 10-4-91; 450 insurance (rob 1,7-92) (most fields in rec 340, 7-92)
	RC rc;

// loop over all record anchors as "registered" in basAnc::al and ::statSetup
	BP b;
	for (USI ancN = 0;  basAnc::ancNext( ancN, &b);  ) 		// loop basAncs (lib\ancrec.cpp)
	{

		// skip flagged rats
		if (b->ba_flags & (RFTYS    	// in "types" and ...
		             | RFNOEX) )		// in records of basAncs flagged by application, eg input basAncs, ...
			continue;       			// register no expressions.

		// select locations of interest and make table.

		// this is separted to facilitate elaboration of SmallRd[] or of conditions for looking at a member.
		// Currently 10-90, take all 4-byte members (floats, LI's, pointers).

		USI oi = 0;					// init offset table os[] index
		for (SI f = 0;  f < b->nFlds; f++)		// loop fields in record
		{
			USI dt = sFdtab[ b->fir[f].fdTy ].dtype;	// get data type from field info
			if ( GetDttab(dt).size==4 		// if any 4-byte field (FLOAT, CHP, ... ) (GetDttab: srd.h)
			  || dt==DTVALNDT )				/* or value+data type substruct, which begins with
           						   a 4-byte value field (for reportCol 11-91) */
			{
				if (oi >= sizeof(os)/sizeof(os[0]))
				{
					err( PWRN,				// disp internal err msg, await key, rmkerr.cpp; was printf 10-91.
					  (char *)MH_E0096);  		// "exman.cpp:exWalkRecs: os[] overflow"
					break;
				}
				os[oi++] = b->fir[f].off;			// save field's offset
			}
		}

		// loop over records for this anchor

		WHERE w;
		w.ancN = ancN;					// anchor number number to addStore arg
		for (TI i = b->mn;  i <= b->n;  i++)		// loop subscripts
		{
			char *e = (char *)&b->rec(i);  	// point record
			if (((record *)e)->gud <= 0)	// if deleted or skip-flagged
				continue;					// ... record, skip it 2-91

			// look for NANDLEs at offsets tabulated above

			w.i = i;					// record number to addStore arg
			for (USI oj = 0;  oj < oi;  oj++)   		// loop tabulated offsets
			{
				USI o = os[oj];				// an offset from table
				NANDAT v = *(NANDAT*)(e + o);	// fetch what's there
				if (ISNANDLE(v))				// if 1 of our special things
				{
					// or use exReg() here.
					w.o = o;				// offset to addStore arg
					if (ISUNSET(v))
					{
						err( PWRN, (char *)MH_E0097, whatRr(w));	// "exman.cpp:exWalkRecs: unset data for %s"
						// do something to stop run?
					}
					else if (ISASING(v))	// if value is to be determined by AUTOSIZE 6-95
						;					// do nothing with it now
					else					// else must be expr
#ifdef STATREF
0						addStore( EXN(v), ancN, i, o );	// register expr use
#else
						CSE_E( addStore( EXN(v), w) )		// register expr use at w
#endif
				}
			}
		}    // for (i =   entries loop
	}	// for (ancN =   basAncs loop
	EXTDUMPIF("exWalkRecs end");			// optional debug dump
	return extValidate("exWalkRecs");		// final checking of exTab structure
}			// exWalkRecs


#if 0	// if needed.  mainly exWalkRecs does this.  restore when desired.  compiled ok 7-92.
o // STATREF version at end file
o //===========================================================================
o RC FC exReg( 							// formerly exRegRat, 7-92
o
o // conditionally "register" expr use in basAnc record member & check for unset data
o
o 	// nop if member contains data rather than NANDLE or UNSET.
o
o    USI ancN,	// rat number (if don't have it, use ratReg(&baseAnc)
o    TI i,	// rat entry subscript 1..
o    USI o )	// rat member offset
o
o // returns RCOK if ok
o{
o	WHERE w; 	NANDAT v;
o
o     w.ancN = ancN; 	w.i = i; 	w.o = o;		// combine into single thing
o     v = *pRecRef(w);				// fetch contents of member
o     if (ISUNSET(v))
o        return err( PWRN, (char *)MH_E0098, whatRr(w) ); 	// "exman.cpp:exReg: Unset data for %s"
o     else if ISNANDLE(v)
o        return addStore( EXN(v), w);     		// register place to put value of expr EXN(v).
o     return RCOK;
o}	// exReg
#endif

// STATREF version at end file
//===========================================================================
LOCAL RC FC NEAR addStore( 		// register use of expression h in basAnc record

	USI h, 	// expression number (EXN(nandle)) */
	WHERE w )	// rat reference: rat number .ancN, record subscript .i, offset .o
{
#define WHVAL_NADD 4		// make 10?

	// could here check ancN and i for being too large for their bit fields

	EXTAB *ex = exTab + h;				// point expression table entry
	if (ex->whValNal <= ex->whValN + 1)			// test if necessary to allocate (more) where's for expression
	{
		USI nuNal = ex->whValNal + WHVAL_NADD;
		RC rc;
		CSE_E( dmral( DMPP( ex->whVal), nuNal * sizeof(WHERE), WRN) )  	// (re)allocate heap block, dmpak.cpp
		ex->whValNal = nuNal;
	}
	ex->whVal[ex->whValN++] = w;			// store ancN-i-o in next available store-where slot
	return RCOK;
}				    // addStore

//STATREF version at end file
//===========================================================================
RC addChafIf (		// conditionally register change flag in basAnc record for expr.   (formerly addChafRatIf, 7-92)

	NANDAT *pv, 		// if this contains a "nandle" on a live run-time expression, change flag will be registered; else not
	// where to ++ when expression changes:
	USI ancN,		// rat number (basAnc.ancN) of change flag
	TI i,		// rat entry subscript 1..
	USI o )		// rat member offset of SI change flag

// [note: if have expression number rather than variable contents, use addChafIf( &...NANDLE(h), ancN, i, o);]
{
#define WHCHAF_NADD 4		// make 10?

	NANDAT v = *pv;
	if (!ISNANDLE(v))				// if not an expr nandle nor an unset nandle
		return RCOK;
	if (ISUNSET(v))				// unset's don't get registered!
		return RCOK;  			// msg option?
	if (ISASING(v))				// if value is to be determined by AUTOSIZE 6-95
		return RCOK;			// do nothing with it now
	EXTAB *ex = exTab + EXN(v);			// point expression table entry
	if (ex->whChafNal <= ex->whChafN + 1)	// test if needs (more) change-flag-where's allocated
	{
		USI nuNal = ex->whChafNal + WHCHAF_NADD;
		RC rc;
		CSE_E( dmral( DMPP( ex->whChaf), nuNal * sizeof(WHERE), WRN) )	// (re)allocate heap (dm) block, dmpak.cpp
		ex->whChafNal = nuNal;
	}
	WHERE *w = &ex->whChaf[ ex->whChafN++];	// point next available change-flag-where for expression
	w->ancN = ancN;				// store info to allow locating rat member
	w->i  = i;					// ... even if rat is moved to new location
	w->o  = o;					// ... (as can happen at reallocation)
	return RCOK;
}		// addChafIf
//===========================================================================
RC FC exEvEvf( 			// evaluate expressions and do their updates

	USI evf,		// evaluation frequency bit mask
	USI useCl )  	// update class bit mask

/* does ALL exprs with 1 or more matching evf bit
                   AND 1 or more matching useClass bit.
   Attempts to reorder evaluation (at startup) to resolve interdependencies
      (method involves multiple evaluation attempts; expressions assumed to have no side effects).
   Displays messages and ++'s error count for any errors not resolved.
   Returns non-RCOK only when max error count exceeded; caller should then terminate run. */
{
	USI hm1, h, hh, uH;
	EXTAB *ex, *uex;
	SI isEoi, reordering, nBads, nLeft, nuerr=0, exerr=0;
	RC rc1, rc=RCOK;

	if (!exN)						// if there are no expressions
		return RCOK;				// nothing to do, return now

	if (exN)						// if there are any registered expressions
		if (exTab==NULL)			// debug aid check -- could delete later
			return err( PWRN, (char *)MH_E0100, (INT)exN);	// "exman.cpp:exEvEvf: exTab=NULL but exN=%d"


	/* test for "end of input" etc evaluation call */
	/* "end of input"/"before each phase setup" evaluation is done b4 input checking and exWalkRecs.
	 	Value of successfully-eval'd exprs is stored in expr source member, so expr
	 	   eval'd now can be used in contexts not otherwise accepting exprs.
	 	   Expr value, not nandle, thus propogates from input to run rats.
	 	(In contrast, in regular (runtime) evaluation, exprs have already been propogated
	 	   from input to run rats as nandles, and found by exWalkRecs; expr value is NOT
	 	   stored in source member as nandle must still be there for next run.)
	 	Also, end-input messaging may be different below. */
	isEoi = evf & ( EVEOI		// expressions to evaluate between input and check/setup
				  | EVFFAZ );	// expressions to evaluate ditto and also after autosize, before re check/setup for main run


	/* evaluate expressions whose eval frequency and use class match arguments.  Rearrange evaluation order if necessary. */

	/* Rearrange expressions to resolve interdependencies such as forward reference to another expression when possible,
	   by moving exprs to end of list without error message on unset/uneval'd data errors.
	   When stuck, issue error messages ('loop' in dependencies, or dependency on something not eval'd this call).
	   Rearranging occurs at start only, when NANDLES are in variables; use of list repeats same order later. */

	reordering = 1;				// say rearranging eval order: suppress unset/uneval'd error messages
	nBads = 0;					// no errors since last success

	for (hm1 = 0;  (h = exTab[hm1].nx) != 0;  )	// loop over exprs in evaluation list order.  exTab[0] holds list head only.
	    										// In the loop expr h can be moved to end of list;
	    										// "hm1 = h" at end is executed only if expr h is not moved.
	{
		ex = exTab + h;
		if (ex->evf & evf				// if has 1 or more given evf bit(s)
		 && ex->useCl & useCl)			// & 1 or more given use class bit(s)
		{
			rc1 = exEvUp( h, isEoi, reordering, &uH); 	// do it, local, next.  'reordering' nz suppresses msg on RCUNSET.

			if (rc1==RCOK)				// if ok
				nBads = 0;					// say no errors since success
			else if (rc1==RCUNSET && reordering)  	// if unset/uneval'd data error while reordering
			{
				uex = exTab + uH;				// point exTab entry for uneval'd expr, if h is valid
				if ( isEoi						// if end-of-input etc eval time (b4 exWalkRecs)
				 &&  uH > 0  &&  uH <= exN  &&  exTab[uH].nx >= 0	// if access to valid uneval'd expr (not UNSET nor bug)
				 &&  !(uex->evf & evf  &&  uex->useCl & useCl)	// if accessed expr will not be evaluated now
				 &&  !(uex->evf & ~ex->evfOk) 			// if accessed expr's evaluation time ok for current expression
				 &&  uex->useCl==ex->useCl 				// if accessed expr's use class same
														//   don't replace expr's evaluated at different times in ivl
				 &&  ex->ty != TYSI )					// can't eval SI's after end input as can't hold nandle
              											// to identify where to store in run rats.
              											// (insurance: evf check should cover).
				{
					/* End-of-input time expr contained probe to a member that now contains expr to be evaluatated later.
					   Change current expression to be evaluated at that time. */

					ex->evf = uex->evf;

					// believe this is neither a good nor a bad re reordering endtest.
					// note from here execute hm1 = h at foot of loop.
				}
				else
				{

					/* reordering endtest: if there have been as many consecutive errors as exprs remain in eval order list,
					   then further reordering cannot help: all exprs in rest of list are bad due to mutual dependency
					   or dependency on exprs not to be eval'd this call or unset data due to bug. */

					nBads++;   					// count errors since RCOK
					nLeft = 0;
					for (hh = hm1;  (hh = exTab[hh].nx) != 0;  )	// count entries remaining in eval order list
						nLeft++;
					if (nBads >= nLeft)		// if as many consecutive errors as list entries
						reordering = 0;  		/* after this expr, stop attempting to fix by reordering, but complete pass
						   to issue error messages, and in case exprs are good (beleived not possible).
						   Do reorder this expr, so its err msg comes last: probable original order. */

					// move this expr to end of evaluation order list, so it will be retried later to see if its inputs have been set

					exTab[hm1].nx = exTab[h].nx;   				// unlink
					exTab[exTail].nx = h;
					exTab[h].nx = 0;
					exTail = h; 	// link to end
					continue;   						// proceed to new next expr exTab[hm1].nx using same hm1
				}
			}
			else
			{

				// error is not unset/uneval'd data, or we have ceased reordering expressions

				rc |= rc1;			// merged error code -- 0 (RCOK) if no errors
				if (rc1==RCUNSET)		// if unset data / unevaluated expr error (not corrected by reordering if here)
					nuerr++;		// say such has occurred: stops run.
				else			// other error (rc not RCOK if here)
					exerr++;		// 6-95
			}
		}
		hm1 = h;				// loop increment: advance in list.  Note don't get here after moving expr to end.

	} // for (hm1= ... expression table loop


// conditionally terminate run after errors

	if (rc && !isEoi)			// (end-input etc caller tests error count, does own messages)
	{
		if (nuerr)				// if nan's encountered in data, end run: avoid FPE crash!
			// (but 12-5-91 when it continued it didn't crash.  why?)
			return rer( (char *)MH_E0101);	// "Unset data or unevaluated expression error(s) have occurred.\n"
          									// "    Terminating run." */
		if (errCount() > maxErrors)		// if too many total errors, msg & ret RCBAD.
			// errCount: gets error count ++'d by calls to rer, err, etc. rmkerr.cpp.
			// maxErrors: cuparse.cpp. Data init, accessible as $maxErrors.
			return rInfo( (char *)MH_E0102, (INT)maxErrors );	/* runtime "Information" message, exman.cpp
          							   "More than %d errors.  Terminating run." */
#if 1 // 6-95. case: probed record name not found when setting tuQMxLh.
		// 6-95 stop run on ANY expr evaluation error cuz unstored result might have left a NAN in target --> crash.
		if (exerr)				// if any exEvUp errors occurred that were not fixed by reordering exprs
			return rInfo( "Error (above) while evaluating expression. Terminating run.");	// NUMS
#endif
	}
	return RCOK;		   	// say continue run (to non-EVEOI|EVFFAZ callers).
	// another return above
}				// exEvEvf

//===========================================================================
LOCAL RC FC NEAR exEvUp( 	// evaluate expression.  If ok and changed, store and increment change flags per exTab.

	USI h, 		// which expression (exTab subscript)
	SI isEoi,	// non-0 during before-setup evaluation: after input, also after autosize if main run is to be done:
				//    store expression value in its source.
	SI silentUnset,	// non-0 to suppress error message on unset/uneval'd data (returns RCUNSET)
	USI *pBadH )	// NULL or, when RCUNSET is returned, receives expr # of referenced uneval'd expr,
      				//   or 0 if data location containted UNSET

/* if error, returns non-RCOK, nothing stored.
   if error is unset value or un-evaluated expression encountered (by PSRATLODx or PSEXPLODx),
      returns RCUNSET, with no message if 'silentUnset' is non-0. */
{
	RC rc;
	const char* ms;
	NANDAT v = nullptr, *pv;
	SI isChanged;

// get new value: evaluate expression's pseudo-code

	EXTAB *ex = exTab + h;
	if (ex->ip==NULL)
		return err( PWRN, (char *)MH_E0103, (INT)h );   	// "exman.cpp:exEv: expr %d has NULL ip"
	rc = cuEvalR( ex->ip, (void**)&pv, &ms, pBadH);	// evaluate, return ptr.
													// returns RCOK/RCBAD/RCUNSET ...
	if (rc)						// if error (not RCOK)
	{
		if (rc==RCUNSET && silentUnset)			// if unset/un-eval'd and caller said no message
			return rc;					// return the specific error code now.  cuEvalR set *pBadH.
		if (ms)							// if ms ret'd for us to complete and issue
		{
			const char* part1 = strtprintf( (char *)MH_E0104, 	// "While determining %s: ". pre-do start of msg to get its length
									whatEx(h) );				// what expr originally set (local)
			rer(				// runtime errMsg: prefix the following with simulation date & time
								//    (as opposed to perNx's input file line).  This file.
				"%s%s%s",
				part1,			// 1st part formatted just above
				strJoinLen( part1, ms) > (USI)getCpl() ? "\n    " : "", 	// break line if too long
				ms );			// insert ms formatted by cuEvalR
		}
		// if ms==NULL, assume cueval.cpp issued the message (unexpected).
		return rc;			// error return to caller
	}

// units-scale it, check limits, issue run-time error message. 2-91.

	rc = uniLim( ex->fdTy, ex->ty, pv);		// do units/limits, issues no messages
	if (rc)					// on error, code of explanatory text is returned
		// message giving cvpak's explanation and showing value AND identifying what expr was for
		return rer(   	// issue runtime errMsg: shows simulation data & time
       					// rather than input file line.  this file.
					(char *)MH_E0105,     	// "Bad value, %s, for %s: %s"
					txVal( ex->ty, pv), 	// convert value to text (local)
					whatEx(h),			// what this expr orginally set (local)
					msg( NULL,			// text for MH to Tmpstr, messages.cpp SHD BE OK TO PASS han to rer now, 3-92.
					(char *)(LI)rc) ); 	//   cast rc to char *

// test for change, condition value by type, store new value in exTab. ex->v is old value, *pv is new value.

	switch (ex->ty)
	{
	case TYSTR:  // pv contains ptr to ptr to the string
		isChanged = (ex->v==UNSET  ||  strcmp( (char *)ex->v, *(char **)pv));
		if (isChanged)
		{
			/* believe no need to copy string: code is stable during run so ok to store pointers into it. 10-90.
						 (to copy, use v = cuStrsaveIf(*(char **)pv); */
			cupfree( DMPP( ex->v));   	// decref/free old str value if in dm (nop if inline in code or UNSET). cueval.cpp.
			v = *(char **)pv;   	// fetch pointer to new string value, used here and below
			ex->v = v;		// store new value for TYSTR
#ifndef NOINCREF // 5-97 this now appears to be a duplicate cupIncRef w/o a double cupFree. Try deleting.
* #if 1 //5-25-95 fix bug with probe of Top.runDateTime in export
*                       cupIncRef(ex->v);  	/* increment ref count of block of copied pointer if not inline in code nor NANDLE,
*						   cueval.cpp, calls dmpak.cpp:dmIncRef (or dupls block if ref count not impl). */
* #endif
#endif
		} 	// else v not set!!
		else				// new value same as old
			cupfree( (void **)pv );	// decref/free the new, else ref count can grow at each evaluation
		break;

	case TYSI: 	  // (10-90: insurance: SI's can't get here as exPile accepts only constant values for user SI type)
		// but if can happen, 0 pad the SI for exTab.v.
	case TYLLI:    					// SI data in LI storage
		v = (NANDAT)*(USI*)pv;     		// 0 pad SI to LI
		goto chtst;

	default:
		v = *pv;					// fetch float -- as NANDAT in case a nan (eg for NCHOICE).
#if 0 && defined( _DEBUG)
		float fv = *(float*)pv;		// debug aid: inspectable value
#endif
chtst:
		isChanged = (ex->v==UNSET  ||  v != ex->v);
		if (isChanged)				// if changed
			ex->v = v;				// store new value (other than TYSTR) in exTab
		break;
	}

// if changed, store value at uses and increment change flags.  v is new value if changed.

	if (isChanged)
	{
		USI i;
		NANDAT *pVal;


		// if end-of-input etc evaluation (b4 exWalkRecs), store new value in expression's source, replacing NANDLE, 12-91.

		if (isEoi)				// includes before re-setup for main run after autosizing, 6-95.
		{
			BP b = ex->srcB;
			record* e = b->GetAtSafe( ex->srcI);
			if (e)			// insurance check.  else errmsg?
			{
				pv = (NANDAT *)((char *)e + b->fir[ex->srcFn].off);	// point to member in record by field number
				if (ex->ty==TYSI)
					*(SI *)pv = (SI)(LI)v;				// store only 16 bits into SI
				else
				{
					// TYFL, TYLLI, TYSTR (if here, no prior string value to free)
					*pv = v;
#if 1	// another try, 7-10
					if (ex->ty==TYSTR)
						cupIncRef( DMPP( v));   	// incr ref count of block of copied ptr if not inline in code nor NANDLE
#endif
				}
#ifndef NOINCREF // 5-97 this may now also be a duplicate cupIncRef w/o a double cupFree. Try deleting.
* #if 1 //5-25-95 looks line another missing cupIncRef, tho no bug yet reported.
*             if (ex->ty==TYSTR)
*                cupIncRef(*pv);   			/* incr ref count of block of copied ptr if not inline in code nor NANDLE,
*	               					   cueval.cpp, calls dmpak.cpp:dmIncRef (or dupls block if refco unimpl).*/
* #endif
#endif
				*((UCH *)e + b->sOff + ex->srcFn) |= FsVAL;	/* field status bit: say value now stored in this member:
             							   is no longer nandle; caller can test/use its value. */
			}
		}

		// store new value v at all registered places

		if (ex->whVal)				// insurance -- whValN should be 0 if NULL
			for (i = 0; i < ex->whValN; i++)
			{
				pVal = pRecRef(ex->whVal[i]);	// get pointer to basAnc record member, or NULL
				if (pVal)				// in case pRat errored
				{
					if (ex->ty==TYSI)		// "can't" get here 10-90
						*(SI *)pVal = (SI)(LI)v;	// SI's have only 16 bits

					else if (ex->ty==TYSTR)
					{
#if 0
x pursuing string expression memory trash, 3-2016
x attempt here is to copy string rather than share it
x did not fix problems and caused incorrect formatting of day of year in UDT reports
x						if (ISNANDLE( *pVal))
x							*pVal = NULL;
x						strsave( *(char **)(pVal), (const char *)v);
#else
						cupfree( (void **)pVal);	// dec ref count or free block of old ptr if not inline in code nor UNSET, cueval.cpp
						*pVal = v;					// store ptr to string.  May be inline in code, or in dm.
						cupIncRef( (void **)pVal);	// increment ref count of block of copied pointer if not inline in code nor NANDLE,
#endif
						//    cueval.cpp, calls dmpak.cpp:dmIncRef (or dupls block if ref count not impl).
					}
					else				// TYLLI and TYFL
						*pVal = v;		// can just be stored
				}
			}     // whVal loop

		// increment all registered change flags

		if (ex->whChaf)				// insurance
			for (i = 0; i < ex->whChafN; i++)
			{
				SI *pChaf = (SI *)pRecRef(ex->whChaf[i]);	// get ptr to rat member, or NULL
				if (pChaf)					// in case pRat errors
					(*pChaf)++;				// increment change flag

			}   // whChaf loop
	}	// if (isChanged)
	return rc;			// additonal return(s) above (including CSE_E macros)
}		// exEvUp

//===========================================================================
RC FC exInfo(		 	// return info on expression #

	USI h,		// expression # (or EXN(nandle))
	// NULLs or receive --
	USI *pEvf, 		// evaluation frequency bits (cuevf.h)
	USI *pTy, 		// type TYLLI TYFL TYSTR [TYSI] (cuparse.h)
	NANDAT *pv )	// current value (ptr if TYSTR) or UNSET.  string NOT dmIncRef'd!

// returns non-RCOK if h is not a valid expression number, NO MESSAGE ISSUED.
{
	EXTAB *ex = exTab + h;
	if (h==0 || h > exN || ex->nx < 0)
		return RCBAD;			// not valid expression number
	if (pEvf)
		*pEvf = ex->evf;
	if (pTy)
		*pTy = ex->ty;
	if (pv)
		if (ex->ty==TYSI)
			*(SI *)pv = (SI)(LI)ex->v;
		else
			*pv = ex->v;			// caller cueval.cpp cupIncRef's pointer if string, 7-92.
	return RCOK;
}			// exInfo

//===========================================================================
LOCAL NANDAT * FC NEAR pRecRef( RECREF rr)

// return NULL or pointer to rat member per RECREF
{
	BP b = basAnc::anc4n( rr.ancN, WRN);	// access basAnc by #, lib\ancrec.cpp
	if (b==0)				// if failed
		return NULL;			// (anc4n issued message)
	if (b->ptr()==NULL)			// if rat not ratCre'd (insurance)
		return NULL;			// (no msg)
	if (rr.i > (USI)b->n)
	{
		err( PWRN, (char *)MH_E0106);	// "exman.cpp:pRecRef: record subscript out of range"
		return NULL;
	}
	return (NANDAT *)b->recMbr( rr.i, rr.o);			// return pointer to record member
}						// pRecRef


//********************************** ERROR MESSAGES and support ************************************

//===========================================================================
LOCAL const char* FC NEAR txVal(

// return text in Tmpstr for value, eg for error messages

	SI ty, 	// cuparse data type
	void *p )	// pointer to data (to ptr to data if a string)

//>> is there already one of these in cul.cpp or somewhere? 2-91
{
	switch (ty)
	{
		// case TYDOY:  type is TYSI here, 2-91
	case TYLLI:
	case TYSI:
		return strtprintf( "%d", (INT)*(SI *)p);

	case TYFL:
		return strtprintf( "%g", *(float *)p);

	case TYSTR:
		return strtprintf( "'%s'", *(char **)p);

	default:
		err( PWRN,					// display intrnl err msg, await key, rmkerr.cpp. was printf 10-91
		(char *)MH_E0107, (UI)ty );  		// "exman.cpp:txVal: unexpected 'ty' 0x%x"
		return "?";
	}
}		// txVal
//===========================================================================
const char* FC whatEx( USI h)

// return text saying what expression table expression is for, for errMsgs.
// identify by what user originally set, not what is being set now.
{
	EXTAB *ex = exTab + h;
	BP b = ex->srcB;
	if (b)					// if baseAnc specified in exTab: insurance
		return strtprintf( "%s of %s%s %s",
		MNAME(b->fir + ex->srcFn),   		// field member name in record (srd.h macro may access special segment)
		(char *)b->what,				// rat name: ZONE etc
		ex->srcIsType ? " type" : "",   		// "type" if pertinent
		b->ptr()								// note 1
		?  strtprintf( "'%s'", b->rec(ex->srcI).name) 		// record name
		:  strtprintf( "[%d]", (INT)ex->srcI) );			// else subscript
	else				// no b (currently not expected 2-91)
		return "";

	/* note 1: 3-23-92: observed garbage name display due to b pointing to INPUT ancBas for which p was 0 (deleted at start run).
	           Added b->ptr() / b->p to force to subscript notation in this case.
	           If this is a usual problem, need better solution: way to get run rat ptr, or name in table, or ?? */
}			// whatEx
//===========================================================================
LOCAL const char* FC NEAR whatRr( RECREF rr) 	// error message insert describing given rat reference
{
	return whatNio( rr.ancN, rr.i, rr.o);
}							// whatRr
//===========================================================================
// if needed: LOCAL char * FC NEAR whatBio( BP b, TI i, USI off) { return whatNio( b->ancN, i, off); }
// proposed name 7-92: whatRecNio.
//===========================================================================
const char* FC whatNio( USI ancN, TI i, USI off)		// error message insert describing given rat record member
{

// basAnc
	BP b = basAnc::anc4n( ancN, WRN);  				// point base for given anchor number, ancrec.cpp
	if (!b)
		return strtprintf( (char *)MH_E0108, (INT)ancN);		// "<bad rat #: %d>"

// field name
	const char* mName = nullptr;
	for (SFIR *fir = b->fir;  fir && fir->fdTy;  fir++)		// find member name in rat's fields-in-record table
		if (fir->off==off)
			mName = MNAME(fir);					// srd.h macro points to name text, possibly in special segment
	if (!mName)
		mName = strtprintf( (char *)MH_E0109, (INT)off);		// if member not found, show offset in msg "member at offset %d"

// record name
	const char* rName;
	if ( b->ptr()						// if anchor's record block allocated: insurance, and
	 &&  b->rec(i).name[0] )  					// and if name is non-""
		rName = strtprintf( (char *)MH_E0110, b->rec(i).name, (INT)i);	// show name, + subscr in parens. "'%s' (subscript [%d])"
	else
		rName = strtprintf( "[%d]", (INT)i);  			// [unnamed or] null name or not alloc'd, show subscipt only

// assemble <memberName> of <ratName>[ type] '<recordName> [(n)]' etc

	return strtprintf( "%s of %s%s %s",
				mName,
				(char *)b->what,
				b->ba_flags & RFTYS ? " type" : "",
				rName );
}	// whatNio
//===========================================================================
RC CDEC rer( char *ms, ...)

// issue runtime error msg with simulation hr & date, disk msg retrieval, and printf formatting.

// **** WRN used here made effective again here 2-94, change call to rer( 0, ms, ...) where keystroke wait not desired.
//      OR change value used here to REG (0) ?

// increments error count, returns RCBAD for convenience.
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return rerIV( WRN, 0, ms, ap);	// use inner fcn. WRN: not input error; report & continue; 0: error, not warning or info.
}				// rer
//===========================================================================
RC CDEC rer( int erOp /*eg PWRN*/, char *ms, ...)

// issue runtime error msg with simulation hr & date, disk msg retrieval, printf formatting, and given error action.

// **** WRN/PWRN made effective again here 2-94, remove where effects (keystroke wait etc) not desired.

// increments error count, returns RCBAD for convenience.
{
	va_list ap;
	va_start( ap, ms);				// point variable arg list
	return rerIV( erOp, 0, ms, ap);		// use inner fcn. 0: error, not warning or info.
}				// rer
//===========================================================================
RC CDEC rWarn( char *ms, ...)

// issue runtime warning msg with simulation hr & date

// returns RCBAD for convenience.
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return rerIV( REG, 1, ms, ap);	// use inner fcn. REG: regular message: report & continue; 1: warning, not error or info.
}				// rWarn
//===========================================================================
RC CDEC rWarn( int erOp /*eg PWRN*/, char *ms, ...)

// issue runtime warning msg with simulation hr & date, disk msg retrieval, and printf formatting, and given error action.

// **** WRN/PWRN made effective again here 2-94, remove where effects (keystroke wait etc) not desired.

// returns RCBAD for convenience.
{
	va_list ap;
	va_start( ap, ms);				// point variable arg list
	return rerIV( erOp, 1, ms, ap);		// use inner fcn. 1: warning, not error or info.
}				// rWarn
//===========================================================================
RC CDEC rInfo( char *ms, ...)

// issue runtime "Info" msg with simulation hr & date, disk msg retrieval, and printf formatting.

// returns RCBAD for convenience.  Used, for example, to report termination due to too many errors (not fatal in themselves).
{
	va_list ap;
	va_start( ap, ms);			// point variable arg list
	return rerIV( REG, 2, ms, ap);	// use inner fcn. REG: regular, report & continue; 2: "Info" msg, not error nor warning.
}				// rInfo
//===========================================================================
RC rerIV( 	// inner fcn to issue runtime error message; msg handle ok for fmt; takes ptr to variable arg list.

	// **** WRN/PWRN made effective again here 2-94, remove where effects (keystroke wait etc) not desired.

	int erOp,			// error action: WRN or PWRN
	int isWarn, 			// 0: error (increment errCount); 1: warning (display suppressible); 2: info.
	const char* fmt,		// message text
	va_list ap /*=NULL*/)	// message argument list

// increments error count if isWarn==0.  returns RCBAD for convenience.
{

// format time & date preliminary string
	// say "Program Error" like err() 2-94 cuz comments show use of PWRN intended & many existing calls use it.
	char *isWhat = isWarn==1 ? "Warning" : isWarn==2 ? "Info" : erOp & PROGERR ? "Program Error" : "Error";

	char when[120];
	if (!Top.dateStr)				// if NULL, still input time (eg end-of-input eval call). otta formalize this ???
		sprintf( when, "%s during %sinput setup",  			// setup
			isWhat,  Top.tp_autoSizing ? "autoSizing " : "");
	else if (!Top.tp_autoSizing)							// main sim
		sprintf( when, "%s at hour/subhour %d/%d on %s of simulation%s",
			isWhat,  Top.iHr+1, Top.iSubhr,  Top.dateStr,  Top.isWarmup ? " warmup" : "" );

	else									// autoSizing
	{	int nIt = Top.dsDayNIt;
		sprintf( when, "%s at hour/subhour %d/%d on autoSizing %s%s",
			isWhat,
			Top.iHr+1, Top.iSubhr,
			Top.tp_AuszDoing(),		// "heating design day", "Jul cooling design day", etc
			nIt ? strtprintf( " iteration %d", nIt) : "" );	// show nIt if non-0
			// show isWarmup? tbd 6-95.
	}

// format caller's msg and args
	char cmsg[800];
	msgI( erOp, cmsg, sizeof( cmsg), NULL, fmt, ap);		// retrieve text for handle if given, format like vsprintf. messages.cpp.
	// era here controls reporting of errors in msgI only.
// assemble complete text
	char whole[920];
	_snprintf( whole, sizeof( whole)-1, "%s:\n  %s", when, cmsg );

// output message to err file and/or screen, increment error count.
	return errI( 			// central message issuer, rmkerr.cpp. returns RCBAD for convenience.
			   erOp, 			/* <--- passed 2-94 restoring effect of WRN, PWRN, PABT in many calls.
                   			   watch for cases where effect undesired and change call,
                   			   including default WRN from rer(). */
			   isWarn, 		// non-0 if warning: don't increment error counter; optionally suppress on screen.
			   whole );		// formatted msg text
}	// rerIV


//********************************  rest of file is IF-OUTS and such  ****************************************

#ifdef STATREF	// non-STATREF version above, unIf'd.
0//===========================================================================
0RC FC exRegRat(
0
0// conditionally "register" expr use in RAT member & check for unset data
0
0	// nop if member contains data rather than NANDLE or UNSET.
0
0   USI ancN,	// rat number (if don't have it, use ratReg(&RATBASE)
0   TI i,	// rat entry subscript 1..
0   USI o,	// rat member offset
0   char *name )	// NULL or name text for unset error message.  If NULL, message is an "internal error".
0
0{
0NANDAT v;
0
0    v = *pRat( ancN, i, 0);				// fetch contents of member
0    if (ISUNSET(v))
0    {
0       if (name)
0          return err( PWRN, "'%s' is unset (in rat %d, entry %d, member offset %d)", name, ancN, i, o );
0       else
0          return err( PWRN, "exRegRat: unset data in rat %d, entry %d, member offset %d", ancN, i, o );
0}
0    else if ISNANDLE(v)
0       addStore( EXN(v), ancN, i, o);   	// register place to put value of expr EXN(v).
0    return RCOK;
0}	// exRegRat
#endif
#ifdef STATREF	// probably useful fcn
0//===========================================================================
0RC FC exRegStat(
0
0	/* conditionally "register" expression use in static location;
0	   also checks for unset data */
0
0   NANDAT *pv,  	/* ptr to location to replace (later) with expression
0   			   value if now contains NANDLE (nop if not NANDLE) */
0   char *name )		/* NULL or name text for unset error message.
0   			   If NULL, message is an "internal error" */
0
0   // use this ONLY for data that won't move; see also exRegRat.
0{
0NANDAT v; WHERE *w;
0
0    v = *pv;
0    if (ISUNSET(v))
0    {	if (name)
0			return err( PWRN, "'%s' (location %p) is unset", name, pv);
0		else
0			return err( PWRN, "exRegStat: unset value at %p", pv);
0	 }
0    else if ISNANDLE(v)		// if a not-a-number representing expression
0    {
0   /* register *pv as place to put value of expr EXN(v) */
0       w = addWhStore(EXN(v));		// add a value-where to exTab entry
0       w->cs = whcsSTAT;		// say is an entry for a fixed loc'n
0       w->w.p = (NANDAT *)pv;		// say where it is
0	 }
0    return RCOK;
0}		// exRegStat
#endif
#ifdef STATREF
0//===========================================================================
0void FC addChafStatIf( 	// conditionally register static change flag for expr
0
0   NANDAT *pv, 	/* if this contains a "nandle" on a live run-time expression,
0   		   change flag will be registered; else (data, value known
0   		   at input time) no registration. */
0   		/* CAUTION: must pass ptr not contents to get bit pattern
0   		   unaltered; else c will attempt conversions on nans. */
0
0   SI *p )	/* fixed location of SI change flag to be ++'d each time
0   		   expression of v changes -- if changeable */
0
0   // [note: if have expression number rather than variable contents, use addChafStatIf( &  NANDLE(h), p);]
0{
0WHERE *w; NANDAT v = *pv;
0
0    if (!ISNANDLE(v))		// if not an expr nandle nor an unset nandle
0       return;
0    if (ISUNSET(v))		// unset's don't get registered!
0       return;			// msg option?
0    w = addWhChaf( EXN(v));	// add a change-flag-where to exTab entry
0    w->cs = whcsSTAT;		// say is an entry for a fixed location
0    w->w.p = (NANDAT *)p;	// say where it is
0}		// addChafStatIf
#endif
#ifdef STATREF	// pre-RATREF version
0//===========================================================================
0LOCAL NANDAT * FC NEAR pRat( USI ancN, TI i, USI o)
0
0/* return NULL or pointer to rat member per number, entry, member offset */
0{
0RBP b;
0
0    b = rat4n( ratn, WRN);	// access ratBase by #, ratpak.c
0    if (b==0)			// if failed
0       return NULL;		// (rat4n issued message)
0    if (b->p==NULL)		// if rat not ratCre'd (insurance)
0       return NULL;		// (no msg)
0    if (i > b->n)
0    {	err( PWRN, "pRat: RAT entry subscript out of range");
0       return NULL;
0	 }
0    return (NANDAT *)( (char *)b->p 			// access table
0                                   + b->eSz * i  	// access entry
0					       + o );	// access member
0}		/* pRat */
#endif
#ifdef STATREF	// old version 12-4-91
0//===========================================================================
0LOCAL void FC NEAR addStore(
0
0	// register use of expression h in RAT
0
0   USI h, 	// expression number (EXN(nandle)) */
0
0   USI ratn,	// rat number (if don't have it, use ratReg(&RATBASE)
0   TI i,	// rat entry subscript 1..
0   USI o )	// rat member offset
0
0{
0WHERE *w;
0    w = addWhStore(h);	// add a value-where to exTab entry
0    w->cs = whcsRAT;	// say is an entry for a RAT member
0    w->w.r.ratn = ratn;	// store info to allow locating rat member
0    w->w.r.i  = i;	// ... even if rat is moved to new location
0    w->w.r.o  = o;	// ...
0}		// addStore
#endif
#ifdef STATREF
0//===========================================================================
0LOCAL WHERE * FC NEAR addWhStore( USI h)	// inner add value use to h
0
0{
0#define WHVAL_NADD 3		// make 10?
0EXTAB *ex; USI nuNal;
0
0    ex = exTab + h;
0    if (ex->whValNal <= ex->whValN + 1)
0    {	nuNal = ex->whValNal + WHVAL_NADD;
0       dmpral( &ex->whVal, nuNal * sizeof(WHERE), ABT);
0       ex->whValNal = nuNal;
0	 }
0    return &ex->whVal[ ex->whValN++];
0}					// addWhStore
#endif
#ifdef STATREF
0//===========================================================================
0void FC addChafRatIf(	// conditionally register change flag in RAT for expr
0
0   NANDAT *pv, 	/* if this contains a "nandle" on a live run-time expression,
0   		   change flag will be registered; else not */
0   // where to ++ when expression changes:
0    USI ratn,		// rat number (RATBASE.ratn)
0    TI i,		// rat entry subscript 1..
0    USI o )		// rat member offset
0
0   // [note: if have expression number rather than variable contents, use addChafRatIf( &...NANDLE(h), ratn, i, o);]
0
0{
0WHERE *w; NANDAT v = *pv;
0
0    if (!ISNANDLE(v))		// if not an expr nandle nor an unset nandle
0       return;
0    if (ISUNSET(v))		// unset's don't get registered!
0       return;			// msg option?
0    w = addWhChaf( EXN(v));	// add a change-flag-where to exTab entry
0    w->cs = whcsRAT;		// say is an entry for a RAT member
0    w->w.r.ratn = ratn;		// store info to allow locating rat member
0    w->w.r.i  = i;		// ... even if rat is moved to new location
0    w->w.r.o  = o;		// ... (as can happen at reallocation)
0}			// addChafRatIf
#endif
#ifdef STATREF
0//===========================================================================
0LOCAL WHERE * FC NEAR addWhChaf( USI h)	// inner add change flag to h
0
0{
0#define WHCHAF_NADD 4		// make 10?
0EXTAB *ex; USI nuNal;
0
0    ex = exTab + h;
0    if (ex->whChafNal <= ex->whChafN + 1)
0    {  nuNal = ex->whChafNal + WHCHAF_NADD;
0       dmpral( &ex->whChaf, nuNal * sizeof(WHERE), ABT);
0       ex->whChafNal = nuNal;
0	 }
0    return &ex->whChaf[ ex->whChafN++];
0}					// addWhChaf
#endif

#if 0	// poss useful code fragment
w
w //--------------------------------------------------------------------------
w /* to free expression number (h) */
w{
w  EXTAB* px = exTab + h;
w    dmfree( DMPP( px->ip));
w    if (px->ty==TYSTR)		// free string ??? are private copies used?
w       cupfree(px->v);
w    dmfree(px->whVal);
w    dmfree(px->chafp);
w    memset( px, 0, sizeof(EXTAB) );	// zero entry in case reused
w    if (h==exN)
w       exN--;
w    // else expression number is not recovered
w}
#endif	// 0

// end of exman.cpp
