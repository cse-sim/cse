// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// ***** tryImInProbe TYSTR access needs to use owner else ratLuu, like run.


// cuprobe.cpp  portion of cuparse.cpp for compiling "probes" (references) to input and run record data


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <SRD.H>	// SFIR MNAME
#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// needed before cncult.h 2-92
#include <MSGHANS.H>	// MH_U0001

#include <CUL.H>	// CULT, FsSET, used re probes
#include <CNGUTS.H>	// Top
#include <IRATS.H>	// Topi

#include <CUTOK.H>	// token types: CUT___ defines
#include <CUEVAL.H>	// pseudo-code: PS___ defines; cupfree printif
#include <EXMAN.H>	// exInfo
#include <CUEVF.H>	// evf's and variabilities: EVF____ defines
#include <CUPARSEI.H>	// CS__ defines, PR___ defines, OPTBL opTbl.  PROP, .

#include <CUPARSEX.H>	// stuff shared by [cumain,] cuparse, cuprobe; cuTok parSp; decls fcns in this file.

#include <CUPARSE.H>	// EVFHR TYSI expTy.  cueval.h rqd 1st.

/*--------------------------- DEFINES and TYPES ---------------------------*/
struct PROBEOBJECT	// info probe() shares with callees: pass single pointer
{
	BP inB;	    		// 0 or input basAnc found with given name, 0'd if member name not found (0 is "near NULL")
	BP runB;  			// 0 or run basAnc found with given name, 0'd if member name not found
	char* what;			// name (.what) of basAnc(s) whose records being probed
	SFIR* inF, * runF;	// pointers to "fields-in-record" tables (srfd.cpp) for input and run rats
	char* mName;		// name of member being probed
	USI inFn, runFn;   	// input and run basAnc field numbers
	USI ssTy;			// data type of record subscript: TYSTR or TYINT
	SI ssIsK;			// non-0 if record subscript is constant
	void * pSsV;		// pointer to subscript value
	USI sz, dt, ty;    	// size, cu TY- type, and DT- data type, of probed field(s)
};

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC   FC NEAR findMember( PROBEOBJECT *o);
LOCAL RC   FC NEAR tryImInProbe( PROBEOBJECT *o);
LOCAL RC   FC NEAR lopNty4dt( USI dt, USI *pTy, USI *pSz, PSOP *pLop, char * * pErrSub);
LOCAL void FC NEAR disMember( SFIR *f1, SI isIn, SI isRun, SI showAll);


//==========================================================================
RC FC probe()

// parse a 'probe' (reference to a member of a record associated with a basAnc).  @ just seen.

// syntax: @ <basAnc_name> [ <id, string, or number> ] . <memberName>

{
	PROBEOBJECT o;		// contains local variables passed to callees. CAUTION: recursion possible, don't use statics.

	USI inDt=0,runDt=0;
	PSOP lop;
	char *errSub;
	BP b;
	SFIR * f;
	USI fn, minEvf;
	RC rc;

	o.inB = o.runB = 0;
	o.inFn = o.runFn = 0;		// initialize variables


// get class name (basAnc 'what') and find input and/or run basAnc

	toke();						// get token
	if (!isWord)				// accept any word even if predefined or reserved
		return perNx( (char *)MH_U0001);			// "U0001: Expected word for class name after '@'"

	for (USI ancN = 0;  basAnc::ancNext( ancN, &b );  ) 	// loop 'registered' record anchors using ancrec.cpp fcn
	{
		if (b->ba_flags & RFTYS)   		// if a "types" basAnc
			continue;       			// accept no probes; keep looking for input & run rats with same name
		if (!stricmp( (char *)b->what, cuToktx))	// will probably need to take _'s as spaces ... 12-91
		{
			if (b->ba_flags & RFINP ? o.inB : o.runB)
				return perNx( (char *)MH_U0002,
							  // "U0002: Internal error: Ambiguous class name '%s':\n"
							  // "    there are TWO %s rats with that .what.  Change one of them.",
							  (char *)b->what,  b->ba_flags & RFINP ? "input" : "run" );
			if (b->ba_flags & RFINP)
				o.inB = b;
			else
				o.runB = b;
			o.what = (char *)b->what;						// for many error messages
		}
	}
	if (!o.runB && !o.inB)
		return perNx( (char *)MH_U0003, cuToktx);   	// "U0003: Unrecognized class name '%s'"

// parse & emit record identifier in []'s: unquoted identifier, string name expression, numeric subscript expression

	b = o.inB ? o.inB : o.runB;				// single pointer to base of (one of) the basAnc(s) found
	if (tokeIf(CUTLB))					// get token / if [ next (else unget the token) (cuparse.cpp)
	{
		CSE_E( expTy( PRRGR, TYSI|TYID, "className[", 0) )	// compile integer or string expr to new stk frame.
		// TYID: like TYSTR, plus assume quotes around unreserved identifiers.
		CSE_E( konstize( &o.ssIsK, &o.pSsV, 0 ) )	// determine if constant/get value, re immediate input probes, below.
    											// evals if evaluable and un-eval'd, rets flag and pointer
		if (tokeNot(CUTRB))
			return perNx( (char *)MH_U0004,		// "U0004: Expected ']' after object %s"
					o.ssTy==TYSI ? "subscript" : "name");
	}
	else

// for single-record static basAnc such as Top, allow omission of subscript; supply constant 0

		if ( b->ba_flags & RFSTAT			// if a static-record basAnc (expect 1 entry only; inB/runB assumed consistent)
				&&  b->n < 1				// without more than 1 entry (yet)(insurance)
				&&  tokeIf(CUTPER) )			// if period next -- what would follow subscript
		{
			static SI iZero = 0;
			CSE_E( newSf())				// use separate stack frame to be like expression case
			CSE_E( emiKon( TYSI, &iZero, 0, NULL ) )	// emit code for a 0 contant
			o.pSsV = parSp->psp1 + 1; 		// where the constant 0 value is, as from curparse.cpp:isKE via cuparse:konstize.
			o.ssIsK = 1;				// say subscript is constant, as from konstize as called in [expr] case above.
			parSp->ty = TYSI;			// have integer value
			unToke();				// unget the . and fall thru
		}
		else
			return perNx( (char *)MH_U0005);		// "U0005: Expected '[' after @ and class name"

	o.ssTy = parSp->ty;  			// save type of subscript expression: TYSI or TYSTR


// get . and composite field 'name'.  'name' can be: abc, abc.def, abc[0], abc[0].def, etc.

	if (tokeNot(CUTPER))  return perNx( (char *)MH_U0006);	// "U0006: Expected '.' after ']'"	require .

	if (findMember(&o)) 			// get & look up composite member name (below) / ret if not found or other err.
		return RCBAD; 				// ... sets o.inF and/or o.runF; clears o.inB/o.runB if input does not match.

	// if here, have match in one OR BOTH tables.
	f = o.inB ? o.inF : o.runF;			// single nonNULL pointer to a fir entry
	o.mName = MNAME(f);				// point member name text for many errMsgs. srd.h macro may access special segment.


// determine DT___ and TY___ data types, and size of type

	if (o.inB)    inDt =  sFdtab[o.inF->fdTy].dtype;		// fetch recdef DT_____ data type for input record member
	if (o.runB)   runDt = sFdtab[o.runF->fdTy].dtype;		// ...  run record member
	if (o.inB  &&  o.runB  &&  inDt != runDt)			// error if inconsistent
		return perNx( (char *)MH_U0007,
					  //"U0007: Internal error: %s member '%s'\n"
					  //"    has data type (dt) %d in input rat but %d in run rat.\n"
					  //"    It cannot be probed until tables are made consistent.\n",
					  o.what, o.mName, (INT)inDt, (INT)runDt );
	o.dt = o.inB ? inDt : runDt;  				// get a single data type value

	if (lopNty4dt( o.dt, &o.ty, &o.sz, &lop, &errSub))		// get ty, size, and instruction for dt, below / if bad
		return perNx( (char *)MH_U0008,				// "U0007: %s member '%s' has %s data type (dt) %d"
					  o.what, o.mName, errSub, (INT)o.dt );


	// decide probe method to use
	// nb giving input time probes priority assumes run member
	//   with same name is not changed by program at run time.

	// if possible, use a before-setup probe method: resolves value in input record:
	// EVEOI, b4 initial setup only, or EVFFAZ, also b4 re-setup for run after autosize, 6-95

	if (o.inB  			// if have input record basAnc
	 && !(o.inF->evf &~EVEOI)	// if probed member has no rutime & no EVFFAZ variation
	 && evfOk & EVEOI )			// if end-of-input time variation ok for expr being evaluated by caller
	{
		minEvf = EVEOI;				// minimum variability, applicable here if f->evf has no variability
		b = o.inB;
		f = o.inF;
		fn = o.inFn;	// use input record: set basAnc, SFIR entry, field # for code emit below
		// note if other operands in expr vary at runtime, cuparse's evf logic will promote expression's evf appropriately.
	}
#ifdef AUTOSIZE // cnglob.h
	else if ( o.inB  			// if have input record basAnc
			  &&  !(o.inF->evf &~(EVEOI|EVFFAZ))	// if probed member has no rutime variation
			  &&  evfOk & EVFFAZ )		// if "phasely" variation ok for expr being evaluated by caller
	{
		minEvf = EVFFAZ;				// minimum variability, applicable here if f->evf has no variability
		b = o.inB;
		f = o.inF;
		fn = o.inFn;	// use input record: set basAnc, SFIR entry, field # for code emit below
		// note if other operands in expr vary at runtime, cuparse's evf logic will promote expression's evf appropriately.
	}
#endif
	else

		// else try immediate probe to input member -- only possible if member already set

	{
		rc = tryImInProbe(&o);		// below
		if (rc != RCCANNOT)  		// unless cannot use this type of probe here but no error
			return rc;			// return to caller: success, done, or error, message issued

		// else compile runtime probe, to run basAnc else input basAnc.  Caller expr will issue msg if its variability not ok.

		// Which record to probe at runtime if both avail: run, cuz probe executes faster, cuz values are stuffed
		//       (cueval must get input expr value from exTab).
		// Former logic to use whichever produced no variability error deleted 12-91, cuz I believe if both rats
		// are available and have different variabilities, there is probably an encoding error, or member that
		// changes at runtime named the same as input member (should be changed; meanwhile, probe run basAnc).

		minEvf = EVFRUN;			// minimum variabilty: applies if member itself has none
		if (o.runB)  				// if have a run basAnc, use it
		{	b = o.runB;
			f = o.runF;
			fn = o.runFn;			// set basAnc, fields-in-record tbl entry ptr, fld #
		}
		else   						// else must have input basAnc if here
		{	b = o.inB;
			f = o.inF;
			fn = o.inFn;		// use it.
		}
	}

// emit eoi or runtime probe code: PSRATRN / PSRATROS & inline args, PSRATLODx & field #.  Subscr/name expr already emitted.

	CSE_E( combSf() )				// combine stack frame & code & evf from subscr expr above with preceding code
	CSE_E( emit( o.ssTy==TYSTR
			 ? PSRATROS   	// access record by ownTi & name: string on stack, leave pointer
			 : PSRATRN ) ) 	// access record by number: SI value on stack, leave pointer on stack
	CSE_E( emit2(b->ancN) )  			// basAnc number follows inline, from basAnc
	if (o.ssTy==TYSTR)					// for lookup by name
		CSE_E( emit2( ratDefO(o.inB ? o.inB : b) ) );	// also emit inline owning input record subsc per context, or 0. cul.cpp.
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
	parSp->ty = o.ty;			// data type resulting from this probe
	parSp->evf |= 			// with evf of any preceding sub-expr and subscr/name expr above, if any, combine...
		f->evf 		// probed member's evalfreq bits fields-in-record table, and
		| minEvf;		// min evalFreq, applicable if 0 in f->evf.  expr keeps only ruling evf bit.
	prec = PROP;			// say have an operand
	return RCOK;			// many other returns above, incl in E macros.  caller ERREX's.
}			// probe
//==========================================================================
LOCAL RC FC NEAR findMember( PROBEOBJECT *o)	// parse and look up probe member name in o.inB and/or o.runB fir tables

// uses multiple input tokens as necessary.
// composite field 'name' can be: abc, abc.def, abc[0], abc[0].def, etc.

// inputs: o.inB, o.runB,

// outputs: o.inF, o.runF: BOTH nonNULL if both .inB and .runB were nonNULL and matches found in both input and run rats.
//          o.inB, o.runB set NULL when input does not match that basAnc's fir table.

// returns: non-RCOK if error, message already issued.
{
	USI l;		// length of token now being matched
	USI m;		// # chars matched by preceding tokens in multitoken member 'name'
	char c;		// after matching a token, next char to match in fir table member name

// get (first token of) member name.  Allow duplication of reserved words.

	toke();											// 1st token of name
	if (!isWord)   return perNx( (char *)MH_U0010,
		// "U0010: Expected a word for object member name, found '%s'"
		cuToktx );

// loop to match composite field name, using additional input tokens as necessary, in inB AND runB fir tables (rest of fcn).

	o->inF  = o->inB  ? o->inB->fir  : NULL;   	// search pointers into fields-in-records tables
	o->runF = o->runB ? o->runB->fir : NULL;   	// .. of the rats found above.
	m = 0;					// # chars matched by preceding tokens in multitoken member name
	for ( ; ; )				// loop over input tokens until break or error return
	{
		SFIR *f1=NULL;						// fir entry for which preceding m chars match
		l = (USI)strlen(cuToktx);					// length of the token to match now

		// search for input & run fir entries that match current token, and any preceding input tokens (m chars)

		if (o->inB)						// if input basAnc found (by caller) & name matches so far here
		{
			f1 = o->inF;						// fir entry for which preceding tokens (m chars) match
			while ( memicmp( cuToktx, MNAME(o->inF) + m, l)  	// while token does not match (continuation of) member name
			||  isalnumW(MNAME(o->inF)[m])  			// .. or matching word/number in table
			&&  isalnumW(MNAME(o->inF)[m+l]) )  		//    .. continues w/o delimiter (ie only initial substring given)
			{
				o->inF++;
				o->inFn++;   				// try next fir table entry, incr field number
				if ( !o->inF->fdTy					// if end fir table, not found
				||  m && memicmp( MNAME(f1), MNAME(o->inF), m) )	/* if preceding m chars of this entry don't match
	     							   (all entries with same beginning are together) */
				{
					o->inB = 0;
					break;			// say mbr not found in input basAnc. errMsg done after run basAnc search.
				}
			}
		}
		if (o->runB)						// if run basAnc found (by caller) & name matches so far here
		{
			f1 = o->runF;						// fir entry for which preceding tokens (m chars) match
			while ( memicmp( cuToktx, MNAME(o->runF) + m, l)	// while token does not match (continuation of) member name
			||  isalnumW(MNAME(o->runF)[m]) 			// .. or while matching word/number in table
			&& isalnumW(MNAME(o->runF)[m+l]) )		//    .. continues w/o delimiter (only initial substring given)
			{
				o->runF++;
				o->runFn++;  				// try next fir table entry; //incr field number
				if ( !o->runF->fdTy				// if end fir table, not found
				||  m && memicmp( MNAME(f1), MNAME(o->runF), m) )	/* if preceding m chars of this entry don't match
								   (all entries with same beginning are together) */
				{
					o->runB = 0;
					break;				// say member not found in run basAnc
				}
			}
		}

		// if not found, issue error message.  syntax ok if here.

		if (!o->inB && !o->runB) 					// if found in neither input nor run records basAnc
			if (!m)							// if first token of name
				return perNx( (char *)MH_U0011, o->what, cuToktx); 	// "U0011: %s member '%s' not found"
			else								// fancier error message for partial match
			{
				char* foundPart = strncpy0( NULL, MNAME( f1), m+1);				// truncate to Tmpstr, lib\strpak.cpp
				return perNx( (char *)MH_U0012,
					//"U0012: %s member '%s%s' not found: \n"
					//"    matched \"%s\" but could not match \"%s\"."
					o->what,  foundPart, cuToktx,  foundPart, cuToktx );
			}

		// match found for current token.  Done if end fir table member text; error if tables continue differently.

		m += l;								// add token length to # chars matched
		if (o->inB  &&  o->runB  &&  MNAME(o->inF)[m] != MNAME(o->runF)[m])

			/* matching input and run field names continue differently.  Error --
			   or could enhance following code to use whichever one matches input.
			   Not expected frequently -- in and run usually use same fir table or have common substruct for similar parts. */

			return perNx( (char *)MH_U0013,
			//"U0013: Internal error: inconsistent %s member naming: \n"
			//"    input member name %s vs run member name %s. \n"
			//"    member will be un-probe-able until tables corrected or \n"
			//"      match algorithm (cuprobe.cpp:findMember()) enhanced.",
			o->what, MNAME(o->inF), MNAME(o->runF) );

		c = o->inB ? MNAME(o->inF)[m] : MNAME(o->runF)[m];	// next char to match: \0, . [ ] digit alpha _
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
				return perNx( (char *)MH_U0014, 			//"U0014: Expected '%c' next in %s member specification,\n"
				c, o->what, cuToktx );		//"    found '%s'"
			break;

		default:
			if (isalphaW(c))
			{
			case '_':
				if (!isWord)  return perNx( (char *)MH_U0015, 	//"U0015: Expected word next in %s member specification,\n"
					o->what, cuToktx);	//"    found '%s'"
			}
			else if (isdigitW(c))
			{
				if (tokTy != CUTSI)
					return perNx( (char *)MH_U0016,		//"U0016: Expected number (subscript) next in %s \n"
					o->what, cuToktx );		//"    member specification, found '%s'"
				/* probably will want to add a (constant) numeric expression parse
				   then canonicalize the value into cuToktx before continuing to text match */
			}
			else
				return perNx( (char *)MH_U0017,			//"U0017: Internal error: unexpected next character '%c'"
				c, o->what);			//"    in %s fir table member name",
			break;

		}  // switch (c)
		// continuation checks for exact token match / searches for specific word or number

	}  // for ( ; ; )  composite field name match loop

	return RCOK;
}			// findMember
//==========================================================================
LOCAL RC FC NEAR tryImInProbe( PROBEOBJECT *o)

// do immediate input probe of already set member if possible, producing constant or reference to same expression as in member.

// try when evf situation will not permit eoi probe. Does backward references only -- member must be set.

/* returns: RCOK:     done, code emitted.
            RCBAD:    error, message issued.
            RCCANNOT: can't do immediate probe; caller should try another method. */
{
	record *e = NULL;
	char *name="", iBuf[10];
	TI defO;
	SI i;
	UCH fs;
	void *v, *pv;
	USI h, exEvf, exTy;
	RC trc;

	if ( !o->inB  		// cannot if did not find an input basAnc
	||  !o->ssIsK)		// cannot do if record subscript is not a constant -- don't know which record to probe.
		return RCCANNOT;
	BP b = o->inB;

// access record

	switch (o->ssTy)			// cases by type of subscript expr.  "not found" errmsg below switch if e NULL.
	{
	case TYSI:
		i = *(SI*)o->pSsV;
		if (i > 0 && i <= b->n)				// if subscript in range,
			e = &b->rec(*(SI*)o->pSsV);			// point to record by number, else leave e NULL.
		sprintf( iBuf, "[%d]", (INT)i);
		name = iBuf;	// make 'name' text for error messages
		break;

	case TYSTR:
		name = *(char**)o->pSsV;			// name for lookup and error messages
		// conditionally use owner per context of probe to resolve name ambiguity
		defO = ratDefO(b);		/* get 0 or input record subscript in b->ownB of context in which current
		   				   expr is being evaluated.  Returns 0 if not "owned record" basAnc, if its .ownB
		   				   is 0, or cur expr not embedded in stmt group for such a record. cul.cpp. */
		if (defO)
			trc = b->findRecByNmDefO( name, defO, &e, NULL);	// seek record (ancrec.cpp) by name & defO, ret ptr if found.
		else
			trc = b->findRecByNmU( name, NULL, &e);    	// seek unique record (ancpak.cpp) by name, ret ptr if found.
		/*if (!evfOk)					as below; add if found ambiguity error occurs here
									when caller fallthru to runtime probe could work */
		if (trc==RCBAD2)							// if ambiguity (not resolved by defO)
			return perNx( (char *)MH_U0020,		// "U0020: %s name '%s' is ambiguous: 2 or more records found.\n"
			b->what, name );		// "    Change to unique names."
		// fall thru not found check or return for caller to try other methods

	default: ;							// other unexpected; leaves e NULL.
	}
	if (!e)		// if (unique) record not found, can't probe its member.
	{
		if (!evfOk)	/* if constant req'd in this expr's context (not even EVEOI/EVFFAZ allowed), do our own errMsg here, now,
    			   cuz for this case, expr's msg, with its implicit start-of-run variability, is confusing.
    			   Also, no other cases will work with evfOk==0. */
			return perNx( (char *)MH_U0021,	//"U0021: %s '%s' has not been defined yet.\n"
			//"    A constant value is required %s a forward reference cannot be used.\n"
			//"    Try reordering your input.",
			o->what, name,
			ermTx 							// context per global if nonNULL
			?  strtprintf((char *)MH_U0021a, ermTx)  		// "for '%s' --\n        "
			:  "--" );
		return RCCANNOT;		/* record not found and evfOk not 0.  A non-immediate probe method may work,
       				   and expr's msg isn't so bad for other variabilities, so let caller fall thru. */
	}

// access field, check set

	fs = *((UCH *)e + b->sOff + o->inFn);   	// fetch member's field status byte
	if (!(fs & FsSET))				// if field[inFn] not set according to field status byte
	{
		if (!evfOk)			/* if constant req'd in this expr's context (not even EVEOI/EVFFAZ permitted), do our own
    					   errMsg here & now, cuz for this case, expr's msg, with its implicit start-of-run
    					   variability, is confusing, and no other probe method will be applicable. */

			return perNx( (char *) MH_U0022,	//"U0022: %s '%s' member %s has not been set yet.\n"
			//"    A constant value is required %s a forward reference cannot be used.\n"
			//"    Try reordering your input."
			o->what, name, o->mName,
			ermTx 							// context per global if nonNULL
			?  strtprintf((char *)MH_U0021a, ermTx)		// "for '%s' --\n        "
			:  "--" );
		return RCCANNOT;		/* record not found and evfOk not 0.  A non-immediate probe method may work,
       				   and expr's msg isn't so bad for other variabilities, so let caller fall thru. */
	}
	pv = (char *)e + o->inF->off;			// point to member
	v  = *(void **)pv;					// fetch member as 4-byte quantity

// if set to constant value, generate constant for same value

	if (fs & FsVAL)				// if field status says contains already-stored value (not expr nandle)
	{
		// generate code for constant value to which member already set
		dropSfs( 0, 1);  					// now discard the subscript code (drop parStk frame)
		emiKon( o->ty, o->ty==TYSTR ? v : pv, 0, NULL);  	// emit constant for probe'd field's value
		parSp->ty = o->ty;					// set type of emitted code
		prec = PROP;      					// say have an operand
		// no evf: is constant
		return RCOK;     					// ok immediate probe to previously set constant input value
	}

// test whether set to expression we can access

	if (o->sz < 4)					// if too small to hold a nandle
		return RCCANNOT;					// cannot access expression via member (seek in exTab?)
	//FsSet & ! FsVAL implies expr nandle. insurance check:
	if (! ISNANDLE(v)					// if not a nandle
	||  ISASING(v)						// or value is being determined by autosize
	||  ISUNSET(v) )					// or is plain unset (required) data (bug here)
		return RCCANNOT;    				// we can't do immediate access.
	h = EXN(v); 					// get expr's expression number
	if (exInfo( h, &exEvf, &exTy, NULL))    		// get expr's type and variability / if h bad (no msg done)(exman.cpp)
	{
		// debug aid msg; shd be ok to continue to other cases
		return perNx( (char *)MH_U0023, 			// "U0023: Internal error: %s '%s' member '%s' \n"
		o->what, name, o->mName, (UI)h );	// "    contains reference to bad expression # (0x%x)"
	}
	else if (exTy != o->ty)				// if expression type does not match member type
	{
		// here add code to resolve any resolvable differences as they become understood

		// msg mainly as debug aid -- shd be ok to continue to other cases (return RCCANNOT):
		return perNx( (char *)MH_U0024,			// "U0024: Internal error: %s '%s' member '%s', \n"
		o->what, name, o->mName, (INT)h, 	// "    containing expression (#%d):\n"
		(INT)o->ty, (INT)exTy );  		// "    member type (ty), %d and expression type, %d, do not match.",
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
}			// tryImInProbe

#if 0	// reference comments, can delete
x//==========================================================================
x /* record data access operations, 12-91. move up at reorder? */
x #define PSRATRN    103	// rat record by number: ratN inline, number on stack, leaves record address on stack
x #define PSRATRS    104	// rat record by name: ratN inline, string on stack, leaves record address on stack
x // following rat loads all add offset of fld # in next 2 bytes to record address on stack.  ARE THEY ALL NEEDED?
x #define PSRATLOD2  107	// rat load 2 bytes: fetches SI/USI.
x #define PSRATLOD4  108	// rat load 4 bytes: fetches float/LI/ULI.
x #define PSRATLODD  109	// rat load double: converts it float.
x #define PSRATLODD  110	// rat load long: converts it float.
x #define PSRATLODA  111	// rat load char array (eg ANAME): makes dm copy, leaves ptr in stack
x #define PSRATLODS  112	// rat load string: loads char * from record, duplicates.
#endif

//==========================================================================
LOCAL RC FC NEAR lopNty4dt( 	// for DT- data type, get TY- type and PSOP to load it from a record of a basAnc

	USI dt, 		// recdef world DTxxxx data type
	USI *pTy,		// NULL or receives ul TYxxxx data type: TYSI, TYFL, or TYSTR.
	USI *pSz,		// NULL or receives sizeof(ty)
	PSOP *pLop, 	/* NULL or receives pseudo-instruction to load this type from a basAnc record
			   (instructions supplied here add inline field #'s offset to address on stack, fetch to stack) */
	char * * pErrSub )	/* NULL or receives adjectivial error subtext on error return:
			   "unexpected", "unrecognized", "un-probe-able", etc */

// if no corresponding TY type, returns errSub text and returns bad.  NO ERROR MESSAGE HERE.
{
	USI ty=0, sz=0;
	PSOP lop=0;

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

	case DTFLOAT:			// float types
#if defined( DTPERCENT)
	case DTPERCENT:
#endif
#if defined( DTSGTARG)
	case DTSGTARG:
#endif
		lop = PSRATLOD4;		// record load 4 bytes: fetches float/[LI/ULI].
		ty = TYFL;
		sz = 4;
		break;

	case DTDBL:
		lop = PSRATLODD;  		// record load double: converts it float.
		ty = TYFL;
		sz = 4;
		break;

	case DTLDATETIME:			// show dateTime as number (?)
	case DTLI:
	case DTULI:
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

	case DTCHP:
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

		//unprobe-able: pointers to basic types (or add a way fetch?)
	case DTFLOATP:			// unprobable types
	case DTSGTARGP:
	case DTDBLP:				// rob 1-95

		//unprobe-able types: structures (to probe, make *substructs so they appear as their individual members)
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
	for (USI ancN = 0;  basAnc::ancNext( ancN, &b);  )	// loop registered record anchors using fcn in ancrec.cpp
		b->ba_flags &=~RFLOCAL;


// loop over basAncs. display input-run pairs together when found (rest of fcn)

	for (ancN = 0;  basAnc::ancNext( ancN, &b);  )	// for each record base-anchor
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

		for (USI ancN2 = ancN;  basAnc::ancNext( ancN2, &b2);  )   	// look for additional basAncs of same name
		{
			if (strcmpi( (char *)b->what, (char *)b2->what))
				continue;							// name different, skip it
			if (b2->ba_flags & RFINP ? inB : runB)				// same; ok if 1st input basAnc or 1st run basAnc with name
				printf( msg( NULL, (char *)MH_U0025,		//"\nInternal error: Ambiguous class name '%s':\n"
				(char *)b->what,  			//"   there are TWO %s rats with that .what. Change one of them.\n"
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
			char *subSub = (b->ba_flags & RFSTAT)==0 ?  "[1..]."	// normal (non-static) subscript basAnc: subscript runs 1 up
			:   b->n < 1           ?  ".     "	// static single-entry (Top, Topi): no subscript needed
			:   "[0..].";				// static multi-entry (not expected): subscript 0 up

			const char* nssSub = strtprintf( "@%s%s", (char *)b->what, subSub );  		// @<name>[1..].  etc

			char *ownSub = b->ownB  &&  b->ownB != (BP)&TopiR 	 			// show non-top ownership
			&&  b->ownB != (BP)&TopR
			?  strtprintf( "                  owner: %s", (char *)b->ownB->what )
			:  "";

			printf( "\n%-20s  %s   %s %s\n",  nssSub,  inB ? "I" : " ",  runB ? "R" : " ",  ownSub);
		}

		// display info on members

		inF  = inB  ? inB->fir  : NULL;   		// search pointers into fields-in-records tables
		runF = runB ? runB->fir : NULL;   		// .. of the basAnc(s) found above
		for ( ; ; )					// until break
		{
			if (inF  && !inF->fdTy)   inF = NULL;		// if end of table, set its ptr NULL
			if (runF && !runF->fdTy)  runF = NULL;	// ..
			if (!inF && !runF)  break;			// if end both tables, done

			// display a member of only table or that matches in both tables

			if (!inF || !runF || !strcmpi(MNAME(inF), MNAME(runF)) )
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
				if (!inMax  &&  !(inF+k)->fdTy)    inMax = k;		// record ends of tables when found
				if (!runMax  &&  !(runF+k)->fdTy)  runMax = k;		// ..
				if (inMax & runMax)  					// if end of both tables found
				{
					j = inMax;
					k = runMax;					// then there was no match short of ends. done.
					break;
				}
				for (j = 0; j <= k; j++)					// compare kth member of each table to 0..kth of other
				{
					if ( (!inMax || j < inMax)  &&  (!runMax || k < runMax) )
						if (!strcmpi( MNAME(inF+j), MNAME(runF+k)) )   		// compare member names
							goto breakBreak;						// found match after j input items, k run items
					if (j != k)							// reverse test wastes time if j==k
						if ( (!inMax || k < inMax)  &&  (!runMax || j < runMax) )	// compare with subscripts interchanged
							if (!strcmpi( MNAME(inF+k), MNAME(runF+j)) )
							{
								i = j;
								j = k;
								k = i;					// swap j and k
								goto breakBreak;					// found match after j input items, k run items
							}
				}
			}
breakBreak:		// show j input members, k run members

			while (j--)  if (inF->fdTy)  disMember( inF++,  1, 0, showAll);
			while (k--)  if (runF->fdTy) disMember( runF++, 0, 1, showAll);
		}
	}
}		// showProbeNames
//==========================================================================
LOCAL void FC NEAR disMember( SFIR *f1, SI isIn, SI isRun, SI showAll)	// display info on one record member, for shoProbeNames
{
	USI dt, ty;
	char *tySubTx="?", *evfSubTx;

	if (f1->ff & FFHIDE  &&  !showAll)		// if field flagged to hide (*i on field in cnrecs.def)
		return;					// don't display it

	dt = sFdtab[f1->fdTy].dtype;				// get field's DT- data type from field type's table entry

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

	evfSubTx =    !f1->evf  ?  "constant"						// variability subtext
	:  f1->evf & EVXBEGIVL ? strtprintf( "end of %s", evfTx(f1->evf,2) )
	:  evfTx( f1->evf, 0);

	printf( " %20s   %s   %s   %-15s   %s\n",
	MNAME(f1),
	isIn  ? "I" : " ",
	isRun ? "R" : " ",
	tySubTx,
	evfSubTx );
}				// disMember
//=============================================================================

// end of cuprobe.cpp
