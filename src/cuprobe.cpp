// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// ***** PROBEOBJECT::po_TryImInProbe TYSTR access needs to use owner else ratLuu, like run.


// cuprobe.cpp  portion of cuparse.cpp for compiling "probes" (references) to input and run record data


/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"

#include "srd.h"	// SFIR
#include "ancrec.h"	// record: base class for rccn.h classes
#include "rccn.h"	// needed before cncult.h 2-92
#include "msghans.h"	// MH_U0001

#include "cul.h"	// CULT, FsSET, used re probes
#include "cnguts.h"	// Top
#include "irats.h"	// Topi

#include "cutok.h"	// token types: CUT___ defines
#include "cueval.h"	// pseudo-code: PS___ defines; cupfree printif
#include "exman.h"	// exInfo
#include "cuevf.h"	// evf's and variabilities: EVF____ defines
#include "cuparsei.h"	// CS__ defines, PR___ defines, OPTBL opTbl.  PROP, .

#include "cuparsex.h"	// stuff shared by [cumain,] cuparse, cuprobe; cuTok parSp; decls fcns in this file.

#include "cuparse.h"	// EVFHR TYSI expTy.  cueval.h rqd 1st.

/*--------------------------- DEFINES and TYPES ---------------------------*/
struct PROBEOBJECT	// info probe() shares with callees: pass single pointer
{
	BP po_inB;	   		// 0 or input basAnc found with given name, 0'd if member name not found (0 is "near NULL")
	BP po_runB;  		// 0 or run basAnc found with given name, 0'd if member name not found
	const char* po_what; // name (.what) of basAnc(s) whose records being probed
	const SFIR* po_inF;		// pointer to "fields-in-record" tables (srfd.cpp) for input rat
	const SFIR* po_runF;		// pointer to "fields-in-record" tables (srfd.cpp) for run rat
	const char* po_mName;	// name of member being probed
	USI po_inFn;   		// input basAnc field number
	USI po_runFn;   	// run basAnc field number
	USI po_ssTy;		// data type of record subscript: TYSTR or TYINT
	SI po_ssIsK;		// non-0 if record subscript is constant
	void* po_pSsV;		// pointer to subscript value
	USI po_sz, po_dt, po_ty;   // size, cu TY- type, and DT- data type, of probed field(s)

	PROBEOBJECT() { memset(this, 0, sizeof(PROBEOBJECT)); }
	RC po_DoProbe();
	RC po_FindMember();
	RC po_TryImInProbe();
};

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC   FC lopNty4dt( USI dt, USI *pTy, USI *pSz, PSOP *pLop, const char** pErrSub);
LOCAL void FC disMember( SFIR *f1, SI isIn, SI isRun, SI showAll);

//-----------------------------------------------------------------------------
RC SetupProbeModernizeTables()
{
	RC rc = RCOK;

	// probe rename pair tables: { old name, current name }
	constexpr static MODERNIZEPAIR RSYS_PMTable[]{ {"fanPwrC", "fanSFPC"}, {"fChg", "fChgC"}, {nullptr, nullptr} };
	RSiB.ba_SetProbeModernizeTable(RSYS_PMTable);
	RsR.ba_SetProbeModernizeTable(RSYS_PMTable);

	constexpr static MODERNIZEPAIR WFDATA_PMTable[]{ {"taDbPk", "taDbMax"}, {"taDbPvPk", "taDbPvMax"}, {nullptr, nullptr} };
	WthrR.ba_SetProbeModernizeTable(WFDATA_PMTable);
	WthrNxHrR.ba_SetProbeModernizeTable(WFDATA_PMTable);

	return rc;
}	// ::SetupProbeModernizeTables
//-----------------------------------------------------------------------------
const char* basAnc::ba_ModernizeProbeName(
	const char* probeName) const
{
	if (ba_probeModernizeTable)
	{
		const char* probeNameWas{ probeName };
		for (const MODERNIZEPAIR* pMP = ba_probeModernizeTable; !pMP->mp_IsEnd(); pMP++)
		{
			if (pMP->mp_ModernizeIf( probeName))
			{
				info("Updating %s name '%s' to '%s'",
					what, probeNameWas, probeName);
				break;
			}
		}
	}
	return probeName;
}		// basAnc::ba_ModernizeProbeName
//==========================================================================
RC FC probe()

// parse a 'probe' (reference to a member of a record associated with a basAnc).  @ just seen.

// syntax: @ <basAnc_name> [ <id, string, or number> ] . <memberName>

{
	// get class name (basAnc 'what') and find input and/or run basAnc

	toke();						// get token
	if (!isWord)				// accept any word even if predefined or reserved
		return perNx(MH_U0001);			// "U0001: Expected word for class name after '@'"

	PROBEOBJECT o;		// contains local variables passed to callees.
						//  CAUTION: recursion possible, don't use statics.
	return o.po_DoProbe();
}	// ::probe
//-----------------------------------------------------------------------------
RC PROBEOBJECT::po_DoProbe()
{

	BP b = nullptr;
	for (size_t ancN = 0;  basAnc::ancNext( ancN, &b );  ) 	// loop 'registered' record anchors using ancrec.cpp fcn
	{
		if (b->ba_flags & RFTYS)   		// if a "types" basAnc
			continue;       			// accept no probes; keep looking for input & run rats with same name
		if (!_stricmp( b->what, cuToktx))	// will probably need to take _'s as spaces ... 12-91
		{
			if (b->ba_flags & RFINP ? po_inB : po_runB)
				return perNx( MH_U0002,
							  // "U0002: Internal error: Ambiguous class name '%s':\n"
							  // "    there are TWO %s rats with that .what.  Change one of them.",
							  b->what,  b->ba_flags & RFINP ? "input" : "run" );
			if (b->ba_flags & RFINP)
				po_inB = b;
			else
				po_runB = b;
			po_what = b->what;				// for many error messages
		}
	}
	if (!po_runB && !po_inB)
		return perNx( MH_U0003, cuToktx);   	// "U0003: Unrecognized class name '%s'"

// parse & emit record identifier in []'s: unquoted identifier, string name expression, numeric subscript expression

	b = po_inB ? po_inB : po_runB;				// single pointer to base of (one of) the basAnc(s) found
	RC rc = RCOK;	// used in CSE_E
	if (tokeIf(CUTLB))					// get token / if [ next (else unget the token) (cuparse.cpp)
	{
		CSE_E( expTy( PRRGR, TYSI|TYID, "className[", 0) )	// compile integer or string expr to new stk frame.
		// TYID: like TYSTR, plus assume quotes around unreserved identifiers.
		CSE_E( konstize( &po_ssIsK, &po_pSsV, 0 ) )	// determine if constant/get value, re immediate input probes, below.
    											// evals if evaluable and un-eval'd, rets flag and pointer
		if (tokeNot(CUTRB))
			return perNx( MH_U0004,		// "U0004: Expected ']' after object %s"
					po_ssTy==TYSI ? "subscript" : "name");
	}
	else

// for single-record static basAnc such as Top, allow omission of subscript; supply constant 0

		if ( b->ba_flags & RFSTAT			// if a static-record basAnc (expect 1 entry only; po_inB/po_runB assumed consistent)
				&&  b->n < 1				// without more than 1 entry (yet)(insurance)
				&&  tokeIf(CUTPER) )			// if period next -- what would follow subscript
		{
			static SI iZero = 0;
			CSE_E( newSf())				// use separate stack frame to be like expression case
			CSE_E( emiKon( TYSI, &iZero, 0, NULL ) )	// emit code for a 0 contant
			po_pSsV = parSp->psp1 + 1; 		// where the constant 0 value is, as from curparse.cpp:isKonExp via cuparse:konstize.
			po_ssIsK = 1;				// say subscript is constant, as from konstize as called in [expr] case above.
			parSp->ty = TYSI;			// have integer value
			unToke();				// unget the . and fall thru
		}
		else
			return perNx( MH_U0005);		// "U0005: Expected '[' after @ and class name"

	po_ssTy = parSp->ty;  			// save type of subscript expression: TYSI or TYSTR


// get . and composite field 'name'.  'name' can be: abc, abc.def, abc[0], abc[0].def, etc.

	if (tokeNot(CUTPER))  return perNx( MH_U0006);	// "U0006: Expected '.' after ']'"	require .

	if (po_FindMember()) 			// get & look up composite member name (below) / ret if not found or other err.
		return RCBAD; 				// ... sets po_inF and/or po_runF; clears po_inB/po_runB if input does not match.

	// if here, have match in one OR BOTH tables.
	const SFIR* f = po_inB ? po_inF : po_runF;			// single nonNULL pointer to a fir entry
	po_mName = f->fi_GetMName();				// point member name text for many errMsgs


// determine DT___ and TY___ data types, and size of type
	USI inDt = 0, runDt = 0;
	if (po_inB)    inDt =  sFdtab[po_inF->fi_fdTy].dtype;		// fetch recdef DT_____ data type for input record member
	if (po_runB)   runDt = sFdtab[po_runF->fi_fdTy].dtype;	// ...  run record member
	if (po_inB  &&  po_runB  &&  inDt != runDt)			// error if inconsistent
		return perNx( MH_U0007,
					  //"U0007: Internal error: %s member '%s'\n"
					  //"    has data type (dt) %d in input rat but %d in run rat.\n"
					  //"    It cannot be probed until tables are made consistent.\n",
					  po_what, po_mName, inDt, runDt );
	po_dt = po_inB ? inDt : runDt;  				// get a single data type value

	PSOP lop;
	const char* errSub;
	if (lopNty4dt( po_dt, &po_ty, &po_sz, &lop, &errSub))		// get ty, size, and instruction for dt, below / if bad
		return perNx( MH_U0008,				// "U0007: %s member '%s' has %s data type (dt) %d"
					  po_what, po_mName, errSub, po_dt );

	// decide probe method to use
	// nb giving input time probes priority assumes run member
	//   with same name is not changed by program at run time.

	// if possible, use a before-setup probe method: resolves value in input record:
	// EVEOI, b4 initial setup only, or EVFFAZ, also b4 re-setup for run after autosize, 6-95

	USI minEvf = 0;
	USI fn = 0;
	if (po_inB  			// if have input record basAnc
	 && !(po_inF->fi_evf &~EVEOI)	// if probed member has no rutime & no EVFFAZ variation
	 && evfOk & EVEOI )				// if end-of-input time variation ok for expr being evaluated by caller
	{
		minEvf = EVEOI;				// minimum variability, applicable here if f->fi_evf has no variability
		b = po_inB;
		f = po_inF;
		fn = po_inFn;	// use input record: set basAnc, SFIR entry, field # for code emit below
		// note if other operands in expr vary at runtime, cuparse's evf logic will promote expression's evf appropriately.
	}
	else if ( po_inB  			// if have input record basAnc
			  &&  !(po_inF->fi_evf &~(EVEOI|EVFFAZ))	// if probed member has no rutime variation
			  &&  evfOk & EVFFAZ )		// if "phasely" variation ok for expr being evaluated by caller
	{
		minEvf = EVFFAZ;				// minimum variability, applicable here if f->fi_evf has no variability
		b = po_inB;
		f = po_inF;
		fn = po_inFn;	// use input record: set basAnc, SFIR entry, field # for code emit below
		// note if other operands in expr vary at runtime, cuparse's evf logic will promote expression's evf appropriately.
	}
	else

		// else try immediate probe to input member -- only possible if member already set

	{
		rc = po_TryImInProbe();	// below
		if (rc != RCCANNOT)  	// unless cannot use this type of probe here but no error
			return rc;			// return to caller: success, done, or error, message issued

		// else compile runtime probe, to run basAnc else input basAnc.  Caller expr will issue msg if its variability not ok.

		// Which record to probe at runtime if both avail: run, cuz probe executes faster, cuz values are stuffed
		//       (cueval must get input expr value from exTab).
		// Former logic to use whichever produced no variability error deleted 12-91, cuz I believe if both rats
		// are available and have different variabilities, there is probably an encoding error, or member that
		// changes at runtime named the same as input member (should be changed; meanwhile, probe run basAnc).

		minEvf = EVFRUN;			// minimum variabilty: applies if member itself has none
		if (po_runB)  				// if have a run basAnc, use it
		{	b = po_runB;
			f = po_runF;
			fn = po_runFn;			// set basAnc, fields-in-record tbl entry ptr, fld #
		}
		else   						// else must have input basAnc if here
		{	b = po_inB;
			f = po_inF;
			fn = po_inFn;		// use it.
		}
	}

// emit eoi or runtime probe code: PSRATRN / PSRATROS & inline args, PSRATLODx & field #.  Subscr/name expr already emitted.

	CSE_E( combSf() )				// combine stack frame & code & evf from subscr expr above with preceding code
	CSE_E( emit( po_ssTy==TYSTR
			 ? PSRATROS   	// access record by ownTi & name: string on stack, leave pointer
			 : PSRATRN ) ) 	// access record by number: SI value on stack, leave pointer on stack
	CSE_E( emit2(b->ancN) )  			// basAnc number follows inline, from basAnc
	if (po_ssTy==TYSTR)					// for lookup by name
		CSE_E( emit2( ratDefO(po_inB ? po_inB : b) ) );	// also emit inline owning input record subsc per context, or 0. cul.cpp.
	/* ratDefO returns subscript of input record in b->ownB of context
	   in which current expr is being evaluated, if any, for resolving ambiguous names.
	   Returns 0 if not ownable-record basAnc (none remain 7-92?) or its ownB unset or not embedded
	     in statement group of owning type (including if b is a run record basAnc).
	   RUNTIME ignores defO if actual probed basAnc is not ownable-record or its .ownB not set:
	     run setup (cncult:topCkf) sets run .ownB's only where run subscripts corres to input subscrs. */
	CSE_E( emit(lop) )					// load per data type: takes record addr on stack, field number inline
	CSE_E( emit2(fn) )					// field number follows inline

// finish

	b->ba_flags |= RFPROBED;	// say have compiled a probe into a record of basAnc: do not free its (input records) block b4 run.
	// ... not needed for EVEOI/EVFFAZ probe, but set where if another operand changes evf to runtime?
	parSp->ty = po_ty;			// data type resulting from this probe
	parSp->evf |= 			// with evf of any preceding sub-expr and subscr/name expr above, if any, combine...
		f->fi_evf 		// probed member's evalfreq bits fields-in-record table, and
		| minEvf;		// min evalFreq, applicable if 0 in f->fi_evf.  expr keeps only ruling evf bit.
	prec = PROP;			// say have an operand
	return RCOK;			// many other returns above, incl in E macros.  caller ERREX's.
}			// PROBEOBJECT::po_DoProbe
//==========================================================================

//-----------------------------------------------------------------------------
/*static*/ void po_SearchSFIR(
	BP& pB,
	const SFIR* &pFi,
	const SFIR* &pF1,
	USI& Fn,
	int &m,
	int &l)

{
	const char* mNameSought = pB->ba_ModernizeProbeName(cuToktx);
	l = strlenInt(mNameSought);				// length of the token to match now

	pF1 = pFi;						// fir entry for which preceding tokens (m chars) match
	while (_strnicmp( mNameSought, pFi->fi_GetMName() + m, l)  	// while token does not match (continuation of) member name
			||  isalnumW(pFi->fi_GetMName()[m])  			// .. or matching word/number in table
			&&  isalnumW(pFi->fi_GetMName()[m+l]) )  		//    .. continues w/o delimiter (ie only initial substring given)
	{
		pFi++;
		Fn++;   				// try next fir table entry, incr field number
		if ( !pFi->fi_fdTy					// if end fir table, not found
			||  m && _strnicmp( pF1->fi_GetMName(), pFi->fi_GetMName(), m) )	/* if preceding m chars of this entry don't match
	     							   (all entries with same beginning are together) */
		{
			pB = nullptr;
			break;			// say mbr not found in input basAnc. errMsg done after run basAnc search.
		}
	}
}
RC PROBEOBJECT::po_FindMember()	// parse and look up probe member name in po_inB and/or po_runB fir tables

// uses multiple input tokens as necessary.
// composite field 'name' can be: abc, abc.def, abc[0], abc[0].def, etc.

// inputs: po_inB, po_runB,

// outputs: po_inF, po_runF: BOTH nonNULL if both po_inB and po_runB were nonNULL and matches found in both input and run rats.
//          po_inB, po_runB set NULL when input does not match that basAnc's fir table.

// returns: non-RCOK if error, message already issued.
{
// get (first token of) member name.  Allow duplication of reserved words.

	toke();											// 1st token of name
	if (!isWord)   return perNx( MH_U0010,
		// "U0010: Expected a word for object member name, found '%s'"
		cuToktx );
 

// loop to match composite field name, using additional input tokens as necessary, in po_inB AND po_runB fir tables (rest of fcn).

	po_inF  = po_inB  ? po_inB->fir  : NULL;   	// search pointers into fields-in-records tables
	po_runF = po_runB ? po_runB->fir : NULL;   	// .. of the rats found above.
	int m = 0;				// # chars matched by preceding tokens in multitoken member name
	for ( ; ; )				// loop over input tokens until break or error return
	{
		const SFIR *f1 = nullptr;		// fir entry for which preceding m chars match
		int l = 0;

#if 1
		if (po_inB)
			po_SearchSFIR(po_inB, po_inF, f1, po_inFn, m, l);

		if (po_runB)
			po_SearchSFIR(po_runB, po_runF, f1, po_runFn, m, l);

#else
		// search for input & run fir entries that match current token, and any preceding input tokens (m chars)

		if (po_inB)						// if input basAnc found (by caller) & name matches so far here
		{
			const char* mNameSought = po_inB->ApplyAlias(cuToktx);
			l = strlenInt(mNameSought);				// length of the token to match now

			f1 = po_inF;						// fir entry for which preceding tokens (m chars) match
			while (_strnicmp( mNameSought, po_inF->fi_GetMName() + m, l)  	// while token does not match (continuation of) member name
			||  isalnumW(po_inF->fi_GetMName()[m])  			// .. or matching word/number in table
			&&  isalnumW(po_inF->fi_GetMName()[m+l]) )  		//    .. continues w/o delimiter (ie only initial substring given)
			{
				po_inF++;
				po_inFn++;   				// try next fir table entry, incr field number
				if ( !po_inF->fi_fdTy					// if end fir table, not found
				||  m && _strnicmp( f1->fi_GetMName(), po_inF->fi_GetMName(), m) )	/* if preceding m chars of this entry don't match
	     							   (all entries with same beginning are together) */
				{
					po_inB = 0;
					break;			// say mbr not found in input basAnc. errMsg done after run basAnc search.
				}
			}
		}
		if (po_runB)						// if run basAnc found (by caller) & name matches so far here
		{
			const char* mNameSought = po_runB->ApplyAlias(cuToktx);
			l = strlenInt(mNameSought);				// length of the token to match now

			f1 = po_runF;						// fir entry for which preceding tokens (m chars) match
			while (_strnicmp( mNameSought, po_runF->fi_GetMName() + m, l)	// while token does not match (continuation of) member name
			||  isalnumW( po_runF->fi_GetMName()[m]) 		// .. or while matching word/number in table
			&& isalnumW(po_runF->fi_GetMName()[m+l]) )		//    .. continues w/o delimiter (only initial substring given)
			{
				po_runF++;
				po_runFn++;  				// try next fir table entry; //incr field number
				if ( !po_runF->fi_fdTy				// if end fir table, not found
				||  m && _strnicmp( f1->fi_GetMName(), po_runF->fi_GetMName(), m) )	// if preceding m chars of this entry don't match
																					// (all entries with same beginning are together)
				{
					po_runB = 0;
					break;				// say member not found in run basAnc
				}
			}
		}
#endif

		// if not found, issue error message.  syntax ok if here.

		if (!po_inB && !po_runB) 					// if found in neither input nor run records basAnc
			if (!m)							// if first token of name
				return perNx( MH_U0011, po_what, cuToktx); 	// "U0011: %s member '%s' not found"
			else
			{	// fancier error message for partial match
				const char* foundPart = strncpy0( NULL, f1->fi_GetMName(), m+1);		// truncate to Tmpstr
				return perNx( MH_U0012,
					//"U0012: %s member '%s%s' not found: \n"
					//"    matched \"%s\" but could not match \"%s\"."
					po_what,  foundPart, cuToktx,  foundPart, cuToktx );
			}

		// match found for current token.  Done if end fir table member text; error if tables continue differently.

		m += l;								// add token length to # chars matched
		if (po_inB && po_runB && po_inF->fi_GetMName()[m] != po_runF->fi_GetMName()[m])

			/* matching input and run field names continue differently.  Error --
			   or could enhance following code to use whichever one matches input.
			   Not expected frequently -- in and run usually use same fir table or have common substruct for similar parts. */

			return perNx( MH_U0013,
			//"U0013: Internal error: inconsistent %s member naming: \n"
			//"    input member name %s vs run member name %s. \n"
			//"    member will be un-probe-able until tables corrected or \n"
			//"      match algorithm (PROBEOBJECT::po_FindMember()) enhanced.",
				po_what, po_inF->fi_GetMName(), po_runF->fi_GetMName() );

		const char c = po_inB ? po_inF->fi_GetMName()[m] : po_runF->fi_GetMName()[m];	// next char to match: \0, . [ ] digit alpha _
		if (c=='\0')						// if end of member name in fir table
			break;						// done! complete matching entry found.  leave "for ( ; ; )".

		// is more text to match in fir member name, get more input and iterate

		toke();		// get next input token (cuparse.cpp): expect . [ ] integer identifier (duplication of reserved words ok).

		/* next thing in fir table text may be  .  [  ]  c identifier  or  number.
			  Following fir table entries with same 1st m chars will have same thing next.
			  Error now if token type wrong; proceed to search for specific match (word or number) if type ok. */
		switch (c)			// cases by next char in fir table member name
		{
		case '.':
		case '[':
		case ']':
			if (c != cuToktx[0])
				return perNx( MH_U0014, 			//"U0014: Expected '%c' next in %s member specification,\n"
				c, po_what, cuToktx );		//"    found '%s'"
			break;

		default:
			if (isalphaW(c))
			{
			case '_':
				if (!isWord)  return perNx( MH_U0015, 	//"U0015: Expected word next in %s member specification,\n"
					po_what, cuToktx);	//"    found '%s'"
			}
			else if (isdigitW(c))
			{
				if (tokTy != CUTSI)
					return perNx( MH_U0016,		//"U0016: Expected number (subscript) next in %s \n"
					po_what, cuToktx );		//"    member specification, found '%s'"
				/* probably will want to add a (constant) numeric expression parse
				   then canonicalize the value into cuToktx before continuing to text match */
			}
			else
				return perNx( MH_U0017,			//"U0017: Internal error: unexpected next character '%c'"
				c, po_what);			//"    in %s fir table member name",
			break;

		}  // switch (c)
		// continuation checks for exact token match / searches for specific word or number

	}  // for ( ; ; )  composite field name match loop

	return RCOK;
}			// PROBEOBJECT::po_FindMember
//-----------------------------------------------------------------------------
RC PROBEOBJECT::po_TryImInProbe()

// do immediate input probe of already set member if possible, producing constant or reference to same expression as in member.

// try when evf situation will not permit eoi probe. Does backward references only -- member must be set.

/* returns: RCOK:     done, code emitted.
            RCBAD:    error, message issued.
            RCCANNOT: can't do immediate probe; caller should try another method. */
{
	

	if ( !po_inB  		// cannot if did not find an input basAnc
	||  !po_ssIsK)		// cannot do if record subscript is not a constant -- don't know which record to probe.
		return RCCANNOT;

	BP b = po_inB;
	record *e = NULL;
	const char* name = "";
	char iBuf[10];

// access record

	switch (po_ssTy)			// cases by type of subscript expr.  "not found" errmsg below switch if e NULL.
	{
	case TYSI:
	{
		SI i = *(SI*)po_pSsV;
		if (i > 0 && i <= b->n)				// if subscript in range,
			e = &b->rec(*(SI*)po_pSsV);			// point to record by number, else leave e NULL.
		snprintf(iBuf, sizeof(iBuf), "[%d]", i);
		name = iBuf;	// make 'name' text for error messages
		break;
	}

	case TYSTR:
	{
		name = *(char**)po_pSsV;			// name for lookup and error messages
		// conditionally use owner per context of probe to resolve name ambiguity
		TI defO = ratDefO(b);		/* get 0 or input record subscript in b->ownB of context in which current
						   expr is being evaluated.  Returns 0 if not "owned record" basAnc, if its .ownB
						   is 0, or cur expr not embedded in stmt group for such a record. cul.cpp. */
		RC trc = defO
			? b->findRecByNmDefO(name, defO, &e, NULL)	// seek record (ancrec.cpp) by name & defO, ret ptr if found.
			: b->findRecByNmU(name, NULL, &e);    	// seek unique record (ancpak.cpp) by name, ret ptr if found.
		/*if (!evfOk)					as below; add if found ambiguity error occurs here
									when caller fallthru to runtime probe could work */
		if (trc==RCBAD2)							// if ambiguity (not resolved by defO)
			return perNx(MH_U0020,		// "U0020: %s name '%s' is ambiguous: 2 or more records found.\n"
			  b->what, name);		// "    Change to unique names."
		break;
	}
		// fall thru not found check or return for caller to try other methods

	default:							// other unexpected; leaves e NULL.
		break;
	}
	if (!e)		// if (unique) record not found, can't probe its member.
	{
		if (!evfOk)	/* if constant req'd in this expr's context (not even EVEOI/EVFFAZ allowed), do our own errMsg here, now,
    			   cuz for this case, expr's msg, with its implicit start-of-run variability, is confusing.
    			   Also, no other cases will work with evfOk==0. */
			return perNx( MH_U0021,	//"U0021: %s '%s' has not been defined yet.\n"
			//"    A constant value is required %s a forward reference cannot be used.\n"
			//"    Try reordering your input.",
			po_what, name,
			ermTx 							// context per global if nonNULL
			?  strtprintf(MH_U0021a, ermTx)  		// "for '%s' --\n        "
			:  "--" );
		return RCCANNOT;		/* record not found and evfOk not 0.  A non-immediate probe method may work,
       				   and expr's msg isn't so bad for other variabilities, so let caller fall thru. */
	}

// access field, check set

	UCH fs = e->fStat()[ po_inFn];   	// fetch member's field status byte
	if (!(fs & FsSET))				// if field[inFn] not set according to field status byte
	{
		if (!evfOk)			/* if constant req'd in this expr's context (not even EVEOI/EVFFAZ permitted), do our own
    					   errMsg here & now, cuz for this case, expr's msg, with its implicit start-of-run
    					   variability, is confusing, and no other probe method will be applicable. */

			return perNx( MH_U0022,	//"U0022: %s '%s' member %s has not been set yet.\n"
			//"    A constant value is required %s a forward reference cannot be used.\n"
			//"    Try reordering your input."
			po_what, name, po_mName,
			ermTx 							// context per global if nonNULL
			?  strtprintf(MH_U0021a, ermTx)		// "for '%s' --\n        "
			:  "--" );
		return RCCANNOT;		/* record not found and evfOk not 0.  A non-immediate probe method may work,
       				   and expr's msg isn't so bad for other variabilities, so let caller fall thru. */
	}
	void* pv = (char *)e + po_inF->fi_off;		// point to member
	void* v  = *(void **)pv;					// fetch member as 4-byte quantity

// if set to constant value, generate constant for same value

	if (fs & FsVAL)				// if field status says contains already-stored value (not expr nandle)
	{
		// generate code for constant value to which member already set
		dropSfs( 0, 1);  					// now discard the subscript code (drop parStk frame)
		emiKon( po_ty, po_ty==TYSTR ? v : pv, 0, NULL);  	// emit constant for probe'd field's value
		parSp->ty = po_ty;					// set type of emitted code
		prec = PROP;      					// say have an operand
		// no evf: is constant
		return RCOK;     					// ok immediate probe to previously set constant input value
	}

// test whether set to expression we can access

	if (po_sz < 4)					// if too small to hold a nandle
		return RCCANNOT;					// cannot access expression via member (seek in exTab?)
	//FsSet & ! FsVAL implies expr nandle. insurance check:
	if (! ISNANDLE(v)					// if not a nandle
	||  ISASING(v)						// or value is being determined by autosize
	||  ISUNSET(v) )					// or is plain unset (required) data (bug here)
		return RCCANNOT;    				// we can't do immediate access.
	USI h = EXN(v); 					// get expr's expression number
	USI exEvf, exTy;
	if (exInfo( h, &exEvf, &exTy, NULL))    		// get expr's type and variability / if h bad (no msg done)(exman.cpp)
	{
		// debug aid msg; shd be ok to continue to other cases
		return perNx( MH_U0023, 	// "U0023: Internal error: %s '%s' member '%s' \n"
			po_what, name, po_mName, h );	// "    contains reference to bad expression # (0x%x)"
	}
	else if (exTy != po_ty)				// if expression type does not match member type
	{
		// here add code to resolve any resolvable differences as they become understood

		// msg mainly as debug aid -- shd be ok to continue to other cases (return RCCANNOT):
		return perNx( MH_U0024,			// "U0024: Internal error: %s '%s' member '%s', \n"
		po_what, name, po_mName, h, 	// "    containing expression (#%d):\n"
		po_ty, exTy );  		// "    member type (ty), %d and expression type, %d, do not match.",
	}

// generate code to reference same expression as member is already set to

	dropSfs( 0, 1);					// discard code for record subscipt
	emit(exTy==TYSTR ? PSEXPLODS : PSEXPLOD4);  	// emit op code to load expression value from expression table
	emit2(h);   					// followed by inline expression number (handle)
	prec = PROP;					// say have an operand
	parSp->ty = exTy;   				// its type
	parSp->evf |= exEvf;				// use only vblty of the expr.  shd be <= that of probed member.
	return RCOK;					// ok immediate probe to previously set expression input value
	// another good return and several error returns above.
}			// PROBEOBJECT::po_TryImInProbe
//==========================================================================
LOCAL RC FC lopNty4dt( 	// for DT- data type, get TY- type and PSOP to load it from a record of a basAnc

	USI dt, 		// recdef world DTxxxx data type
	USI* pTy,		// NULL or receives ul TYxxxx data type: TYSI, TYFL, or TYSTR.
	USI* pSz,		// NULL or receives sizeof(ty)
	PSOP* pLop,		// NULL or receives pseudo - instruction to load this type from a basAnc record
					//   (instructions supplied here add inline field #'s offset to address on stack, fetch to stack)
	const char** pErrSub )	/* NULL or receives adjectivial error subtext on error return:
			   "unexpected", "unrecognized", "un-probe-able", etc */

// if no corresponding TY type, returns errSub text and returns bad.  NO ERROR MESSAGE HERE.
{
	USI ty = 0, sz = 0;
	PSOP lop = 0;

	char* errorSub = NULL;			// say no error
	switch (dt)				// for dt, get load pseudo-op (lop) or error message insert text (errorSub)
	{
	case DTSI:			// 2-byte types that are or can be treated as integers
	case DTBOO:
	case DTUSI:
	case DTDOW:
	case DTMONTH:
	case DTDOY:
	case DTTI:
	case DTWFILEFORMAT:		// 10-94. an enum type.
	case DTNOYESCH:			// choices become integers at least for now
	case DTSKYMODCH:
	case DTDOWCH:
	case DTOSTYCH:
	case DTZNMODELCH:
	case DTIZNVTYCH:
	case DTEXCNDCH:
	case DTSFMODELCH:
	case DTSIDECH:
	case DTFILESTATCH:
	case DTIVLCH:
	case DTRPTYCH:
	case DTJUSTCH:
	case DTENDUSECH:
	case DTDHWEUCH:
	case DTDHWEUXCH:
		lop = PSRATLOD2;  		// basAnc record load 2 bytes: fetches SI/USI.
		ty = TYSI;
		sz = 2;
		break;

	case DTINT:
		lop = PSRATLOD4;  		// basAnc record load 2 bytes: fetches SI/USI.
		ty = TYINT;
		sz = 4;
		break;

	case DTFLOAT:			// float types
#if defined( DTPERCENT)
	case DTPERCENT:
#endif
#if defined( DTSGTARG)
	case DTSGTARG:
#endif
		lop = PSRATLOD4;		// record load 4 bytes: fetches float
		ty = TYFL;
		sz = 4;
		break;

	case DTDBL:
		lop = PSRATLODD;  		// record load double: converts it float.
		ty = TYFL;
		sz = 4;
		break;

	case DTLDATETIME:			// show dateTime as number (?)
		lop = PSRATLODL;  		// record load long: converts it float, potentially loosing some precision.
		ty = TYFL;
		sz = 4;
		break;

	case DTANAME:
	case DTWFLOC:			// char arrays used in WFILE, 1-94
	case DTWFLID:			// ..
	case DTWFLOC2:			// 10-94
		lop = PSRATLODA;  		// record load char array (eg ANAME): makes dm copy, leaves ptr in stack
		ty = TYSTR;
		sz = 4;
		break;

	case DTCULSTR:
		lop = PSRATLODS;  		// record load string: loads char * from record, duplicates.
		ty = TYSTR;
		sz = 4;
		break;

#ifdef wanted			// restore if CH field used in record and wanna be able to probe it
w      case DTCH:
w          lop = PSRATLOD1S;		// record load 1 signed byte: fetchs char, converts to 2 bytes, extending sign.
w          ty = TYSI;
w			sz = 2;
w          break;
#else
		// DTCH un-probe-able: only used in field type CH, which is not used in any records 12-91

	case DTCH:
#endif

#ifdef wanted			// restore if UCH used in a field type used in a record and want it probeable
w      case DTUCH:
w          lop = PSRATLOD1U;  		// record load 1 unsigned byte: fetchs UCH, converts to 2 bytes w/o sign extend
w          ty = TYSI;         		// produces ul "integer" type
w			sz = 2;
w          break;
#else
	// DTUCH un-probe-able: no field with this DT 12-91, so not used in records, so don't support.
	case DTUCH:
#endif

	// DTLI / DTULI un-probable.  32 or 64 bit.  No fields with this DT 10-23.  Could add load / convert to int?
	case DTLI:
	case DTULI:

	// unprobe-able: pointers to basic types (or add a way fetch?)
	case DTSGTARGP:

	// unprobe-able types: structures (to probe, make *substructs so they appear as their individual members)
	case DTIDATETIME:
	case DTIDATE:
	case DTITIME:
#ifdef DTVALNDT		// tentatively made *substruct
	case DTVALNDT:
#endif

	//unprobe-able types: pointers to unprobe-able types
#ifdef DTVOIDP
	case DTVOIDP:
#endif
	case DTMASSLAYERP:
	case DTMASSMODELP:
	case DTYACAMP:
	case DTXFILEP:
		errorSub = "un-probe-able";
		break;

	case DTNONE:			// should not occur in fir tables
	case DTUNDEF:
	case DTNA:
		errorSub = "unexpected";
		break;

	default:
		errorSub = "unrecognized";
		break;

	} // switch (dt)

	if (errorSub)				// if error
	{
		if (pErrSub)  *pErrSub = errorSub;	// return subtext for caller to insert in message
		return RCBAD;
	}

	if (pTy)   *pTy = ty;		// return results
	if (pSz)   *pSz = sz;
	if (pLop)  *pLop = lop;
	return RCOK;
}			// lopNty4dt

//==========================================================================
void FC showProbeNames(int showAll)

// display record and member names and info, for -p command line switch
{
	BP b, b2, inB, runB;
	SFIR *inF, *runF;
	SI i, j, k, inMax, runMax;


// in all basAncs, clear flag we will use to indicate already displayed
	for (size_t ancN = 0;  basAnc::ancNext( ancN, &b);  )	// loop registered record anchors using fcn in ancrec.cpp
		b->ba_flags &=~RFLOCAL;


// loop over basAncs. display input-run pairs together when found (rest of fcn)

	for (size_t ancN = 0;  basAnc::ancNext( ancN, &b);  )	// for each record base-anchor
	{
		if (b->ba_flags & (RFLOCAL		// skip if already displayed (via input-run pairing)
				|RFTYS) )    			// skip if a types basAnc (not expected at startup; names duplicate)
			continue;
		if (b->rt & RTBHIDE  &&  !showAll)		// if record type has "hide" bit (*i on record in cnrecs.def),
			continue;					//  do not display it
		b->ba_flags |= RFLOCAL;				// say this one displayed
		inB = runB = 0;
		if (b->ba_flags & RFINP)  inB = b;
		else  runB = b; 	// set input or run basAnc ptr (whichever it is)

		// find run records basAnc to match input basAnc or vica versa; error message if 2 in or 2 run rats with same name

		for (size_t ancN2 = ancN;  basAnc::ancNext( ancN2, &b2);  )   	// look for additional basAncs of same name
		{
			if (_stricmp( b->what, b2->what))
				continue;							// name different, skip it
			if (b2->ba_flags & RFINP ? inB : runB)				// same; ok if 1st input basAnc or 1st run basAnc with name
				printf( msg( NULL, MH_U0025,		//"\nInternal error: Ambiguous class name '%s':\n"
				b->what,  							//"   there are TWO %s rats with that .what. Change one of them.\n"
				b->ba_flags & RFINP ? "input" : "run" ) );	// msg() gets disk text (and formats) -- printf does not.
			else
			{
				b2->ba_flags |= RFLOCAL;	// say this one displayed too
				if (b2->ba_flags & RFINP)  inB = b2;
				else  runB = b2; 		// set input or run records basAnc ptr (whichever it is)
			}
		}

		// display header for this basAnc's information

		{
			// 7-92 min subscr is now a basAnc member, recode this when needed.
			const char *subSub = (b->ba_flags & RFSTAT)==0 ?  "[1..]."	// normal (non-static) subscript basAnc: subscript runs 1 up
			:   b->n < 1           ?  ".     "	// static single-entry (Top, Topi): no subscript needed
			:   "[0..].";				// static multi-entry (not expected): subscript 0 up

			const char* nssSub = strtprintf( "@%s%s", b->what, subSub );  		// @<name>[1..].  etc

			const char *ownSub = b->ownB  &&  b->ownB != (BP)&TopiR 	 			// show non-top ownership
			&&  b->ownB != (BP)&TopR
			?  strtprintf( "                  owner: %s", b->ownB->what )
			:  "";

			printf( "\n%-20s  %s   %s %s\n",  nssSub,  inB ? "I" : " ",  runB ? "R" : " ",  ownSub);
		}

		// display info on members

		inF  = inB  ? inB->fir  : NULL;   		// search pointers into fields-in-records tables
		runF = runB ? runB->fir : NULL;   		// .. of the basAnc(s) found above
		for ( ; ; )					// until break
		{
			if (inF  && !inF->fi_fdTy)   inF = NULL;		// if end of table, set its ptr NULL
			if (runF && !runF->fi_fdTy)  runF = NULL;	// ..
			if (!inF && !runF)  break;			// if end both tables, done

			// display a member of only table or that matches in both tables

			if (!inF || !runF || !_stricmp(inF->fi_GetMName(), runF->fi_GetMName()) )
			{
				disMember( inF ? inF : runF, inF != NULL, runF != NULL, showAll);
				if (inF)  inF++;
				if (runF) runF++;
				continue;					// do endtest above
			}

			// members remain in both tables and their names differ; find match and display intervening members

			inMax = runMax = 0;						// end not yet found in either table
			for (k = 1; ; k++)						// try looking ahead 1,2,... til end or match
			{
				if (!inMax  &&  !(inF+k)->fi_fdTy)    inMax = k;		// record ends of tables when found
				if (!runMax  &&  !(runF+k)->fi_fdTy)  runMax = k;		// ..
				if (inMax & runMax)  					// if end of both tables found
				{
					j = inMax;
					k = runMax;					// then there was no match short of ends. done.
					break;
				}
				for (j = 0; j <= k; j++)					// compare kth member of each table to 0..kth of other
				{
					if ( (!inMax || j < inMax)  &&  (!runMax || k < runMax) )
						if (!_stricmp( inF[j].fi_GetMName(), runF[k].fi_GetMName()) )   	// compare member names
							goto breakBreak;						// found match after j input items, k run items
					if (j != k)							// reverse test wastes time if j==k
						if ( (!inMax || k < inMax)  &&  (!runMax || j < runMax) )	// compare with subscripts interchanged
							if (!_stricmp( inF[k].fi_GetMName(), runF[j].fi_GetMName()) )
							{
								i = j;
								j = k;
								k = i;					// swap j and k
								goto breakBreak;		// found match after j input items, k run items
							}
				}
			}
breakBreak:		// show j input members, k run members

			while (j--)  if (inF->fi_fdTy)  disMember( inF++,  1, 0, showAll);
			while (k--)  if (runF->fi_fdTy) disMember( runF++, 0, 1, showAll);
		}
	}
}		// showProbeNames
//==========================================================================
LOCAL void FC disMember( SFIR *f1, SI isIn, SI isRun, SI showAll)	// display info on one record member, for shoProbeNames
{
	if (f1->fi_ff & FFHIDE  &&  !showAll)		// if field flagged to hide (*i on field in cnrecs.def)
		return;					// don't display it

	USI dt = sFdtab[f1->fi_fdTy].dtype;		// get field's DT- data type from field type's table entry

	USI ty;
	const char* tySubTx = "?";
	if (lopNty4dt( dt, &ty, NULL, NULL, &tySubTx)==RCOK)  	// get TY- type or error subText for field's DT- type. above.
		switch (ty)						// if ok get text for DT type (lopNty4dt set it if not RCOK)
		{
		case TYSI:
			tySubTx = "integer number";
			break;
		case TYFL:
			tySubTx = "number";
			break;
		case TYSTR:
			tySubTx = "string";
			break;
		default:
			if (!showAll)  return;
			break;	// normally hide structures, pointers, errors, etc.
		}

	const char* evfSubTx =    !f1->fi_evf  ?  "constant"						// variability subtext
		:  f1->fi_evf & EVXBEGIVL ? strtprintf( "end of %s", evfTx(f1->fi_evf,2) )
		:  evfTx( f1->fi_evf, 0);

	printf( " %20s   %s   %s   %-15s   %s\n",
		f1->fi_GetMName(),
		isIn  ? "I" : " ",
		isRun ? "R" : " ",
		tySubTx,
		evfSubTx );
}				// disMember
//=============================================================================

// end of cuprobe.cpp
