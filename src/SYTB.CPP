// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// sytb.cpp: symbol table functions for cal nonres engine

// possibly combine with cutok.cpp to reduce number of files?

/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>
#include <search.h>	// qsort

#include <msghans.h>	// MH_P0080

#include <sytb.h>	// decls for this file

/*-------------------------------- DEFINES --------------------------------*/

#if 0	// 4-7-10
x ID access replaced by STAE.GetID()
x //in sytb.h: typedef struct { void *stbk; USI iTokTy; } STAE;
x
x struct DUMSTBK		// symbol table block definition for accessing id, normal version.
		x
{
	char *id;		// (actual *stbk structs are up to appl but always have id 1st)
	x
};
x struct DUMSTBKNEARID		// symbol table block definition for accessing id, version used when near-id-ptr flag is on, 3-20-91
		x
{
	char NEAR * id;
	x
};
x
x #define ID(stae) ((DUMSTBK*)(stae)->stbk->id) ???  get rid of DUMxxx
#endif

/*------------------------------- COMMENTS --------------------------------*/
// initialization of SYTBH's:  1. an all-0 SYTBH will work; call sySort for it to make it a sorted table.
// case sensitivity: (write this)

/*---------------------------- LOCAL VARIABLES ----------------------------*/
LOCAL SI NEAR sytbCass = 0;	/* callers to syCmp: non-0 to force case-sensitive compares
				   (when sorting symbol table or finding insertion place) */

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL INT CDEC /*no NEAR!*/ syCmp( const void *p, const void *q);
LOCAL RC NEAR rBsearch( STAE *key, SYTBH *st, STAE **pp);
LOCAL RC NEAR rLfind( STAE *key, SYTBH *st, STAE **pp);
LOCAL SI FC NEAR rSyCmp( const STAE *p, const STAE *q);


//===========================================================================
RC FC syLu( SYTBH *st, char *id, BOO casi, SI *pTokty/*=NULL*/, void **pStb/*=NULL*/)

// symbol table look up

// casi non-0 makes search case insensitive even for entries not flagged case-insensitive

// returns RCBAD if not found, no msg.
{
	STAE key, *p;
	STBK kstbk;
	RC rc;

// if no table or table empty, give "not found" return
	if (st==NULL || st->n==0)
		return RCBAD;
// fill STAE with stbk as search key
	kstbk.id = id;
	key.stbk = &kstbk;
	key.iTokTy = casi ? SYTBCASI : 0;
// search with binary or linear search.  Double method allows table to remain unsorted when doing more adds than lookups.
	sytbCass = 0;						// disable syCmp forced case-sens
	rc = (*( st->isSorted ? rBsearch : rLfind))( &key, st, &p);
	if (rc==RCOK)
	{
		// return info re entry found
		if (pTokty != NULL)			// if caller wants info
			*pTokty = p->iTokTy & TOKTYMASK;
		// symbol table casi bit not returned. ok?
		if (pStb != NULL)
			*pStb = p->stbk;
	}
	return rc;				// more returns above
}			// syLu
//===========================================================================
RC FC syAdd( SYTBH *st, SI tokTy, BOO casi, STBK* stbk, int op)

// add to symbol table. symbol text is in stbk->id.

/* op: DUPOK bit: no error (and no add) if same id, tokTy, casi.
		  stbk match NOT checked! (size not known here) */

// other duplicate ids: msg, tentatively added anyway, ret ok. 1-17-91.

// RCOK if successful, RCFATAL if out of memory, RCBAD if other error
{
#define SYTBADD 100	// # slots to add at once: 2 debug, 10-50 production.   2-->100 3-21-92.

	STAE key, *p;
	STBK kstbk;
	USI iTokTy, nAl;
	RC rc;

	char* id = stbk->ID();
	iTokTy = (tokTy & TOKTYMASK) | (casi ? SYTBCASI : 0);

// checks
	if (st==NULL || id==NULL || *id=='\0')
		return err( PWRN, (char *)MH_P0080);			/* display internal error msg "Sytb.cpp: bad call to syAdd"
       								   wait for key, return RCBAD, rmkerr.cpp */
	if (tokTy & ~TOKTYMASK)
		return err( PWRN, (char *)MH_P0081, (UI)tokTy);   	// 3-92. "Sytb.cpp: syAdd: tokTy 0x%x has disallowed hi bits"

// enlarge allocation if nec (does initial alloc if st->p is NULL)
	if (st->n >= st->nAl)
	{
		nAl = st->n + SYTBADD;
		if (dmral( DMPP( st->p), nAl * sizeof(STAE), WRN|DMZERO) )
			return RCFATAL;						// if out of memory
		st->nAl = nAl;
	}

// fill STAE and stbk for use as search key
	key.stbk = &kstbk;
	kstbk.id = id;
	key.iTokTy = 0;		// SYTBNEARID = 0; SYTBCASI does not matter with sytbCass on.

// determine where to put new entry and whether already defined.
	sytbCass = 1;						/* tells syCmp to be case-sens to sort table so
    								   binary search will always work */
	rc = (*( st->isSorted ? rBsearch : rLfind))( &key, st, &p);	// Use binary or linear search per whether table is now sorted.
	sytbCass = 0;						// disable syCmp forced case-sens
	if (rc==RCOK)						// if found: duplicate key
	{
		// optionally no message for duplicate key with same tokTy and casi
		if (op & DUPOK && p->iTokTy==iTokTy)
			return RCOK;						// no msg, no duplicate add
		// report duplicate entry and ? continue
		char * dupdId = stbk->ID();
		err( PWRN,						// display internal error msg, wait for key, rmkerr.cpp
			(char *)MH_P0082,		// "sytb.cpp:syAdd(): Adding symbol table entry '%s' (%d) \n    that duplicates '%s' (%d)"
			id, (INT)tokTy,
			dupdId, INT(p->iTokTy & TOKTYMASK) );
	}
	ASSERT( p >= st->p);						// 3-2-94.  8-95: ASSERT macro is in cnglob.h.
	ASSERT( p <= st->p + st->n); 					// ..
	ASSERT( p < st->p + st->nAl); 					// ..
	memmove( p + 1, p, size_t((char *)(st->p + st->n) - (char *)p) );	// make space

// fill new entry
	p->iTokTy = iTokTy;
	p->stbk = stbk;
	st->n++;				// increment entry count
	return RCOK;			// additional returns above

#undef SYTBADD
}			// syAdd
//===========================================================================
RC FC syDel( SYTBH *st, SI tokTy, BOO casi, [[maybe_unused]] BOO nearId, char *id, BOO undefBad/*=FALSE*/, void **pStb/*=NULL*/)

// delete symbol table entry

/* *pStb receives pointer to appl-dependent definition block so
   caller may deallocate it, its id, and its substructures as necessary. */

// if entry not found, issues message only if undefBad != 0, and rets RCBAD.
// if token type or casi or nearId(3-92) does not match, issues message and rets RCBAD.
{
	STAE key, *p, *last;
	STBK kstbk;
	USI iTokTy;
	RC rc;

	if (pStb)
		*pStb = NULL;			// no block for all not found returns

// if no table or table empty, give "not found" return
	if (st==NULL || st->n==0)
		goto notDefined;			// message or not, return RCBAD

// find entry to delete. code duplicates syLu().
	kstbk.id = id;
	key.stbk = &kstbk;
	key.iTokTy = casi ? SYTBCASI : 0;
	sytbCass = 0;						// disable syCmp forced case-sensitivity
	rc = (*( st->isSorted ? rBsearch : rLfind))( &key, st, &p); 	// call binary or linear search fcn
	if (rc==RCBAD)						// if not found
	{
notDefined:
		;
		if (undefBad)
			err( PWRN,					// display internal error msg, wait for key, rmkerr.cpp
			(char *)MH_P0083,			// "sytb.cpp:syDel(): symbol '%s' not found in table"
			id );
		return RCBAD;
	}

// insurance: verify token type and casi match
	iTokTy = (tokTy & ~TOKTYMASK) | (casi ? SYTBCASI : 0);
	if (p->iTokTy != iTokTy)
	{
		// always issue message for this?
		err( PWRN,				// display internal error msg, wait for key, rmkerr.cpp
		(char *)MH_P0084,			// "sytb.cpp:syDel(): bad call: token type / casi / nearId is 0x%x not 0x%x"
		(UI)p->iTokTy, (UI)iTokTy );
		return RCBAD;
	}

// return block pointer so caller may free
	if (pStb)
		*pStb = p->stbk;

// remove entry
	st->n--;				// decr count of entries in table
	last = st->p + st->n;		// point last entry.  NB n decr'd.
	if (last > p)						// insurance
		memmove( p, p + 1, (USI)((char *)last - (char *)p) );	// squeeze out
	memset( last, 0, sizeof(STAE));				// 0 last entry: insurance

	return RCOK;		// error returns above
}		// syDel
//===========================================================================
RC FC sySort( SYTBH *st)

// sort symbol table, and specify that it is to be kept sorted (sorting slows insertions and speeds lookups)

/* sort is done case-sensitive so binary search will work whether case-
   sensitive or insensitive.  See special ordering in compare fcn syCmp. */
{
	if (st)				// NOP? if NULL
	{
		if (st->p && st->n > 1)  	// else nothing to sort
		{
			sytbCass = 1;					// tells syCmp to do all compares case-sens
			qsort( st->p, st->n, sizeof(STAE), syCmp);    // sort. c library fcn.
			sytbCass = 0;					// disable syCmp forced case-sensitivity
		}
		st->isSorted = 1;
	}
	return RCOK;
}		// sySort
//===========================================================================
RC FC syClear( 		// delete all or selected entries in a symbol table, calling given function for each. Added 10-93.

	SYTBH *st,		// header of symbol table to clear
	SI tokTy /*=-1*/,	// tokTy for entries to delete, or -1 for all.

	BOO (*callBack)( SI tokTy, STBK *&pStb) /*=NULL*/ )

/* fcn to clean up one block: only caller knows what dm pointers, etc are in it.
   arguments: tokTy, REFERENCE TO pointer to block: caller's dmfree thus NULLs our pointer.
   must return TRUE if deleted, FALSE to retain in symbol table.
   If TRUE is returned, fcn must have free'd block if it should be free'd.
   If NULL given, all entries matching tokTy are deleted; their stbk blocks are NOT FREE'd. */

//  hmm... better, this fcn ptr might be in header for use by syDel also?

/* callers 11-95: ppcmd.cpp:ppCmdClean;
                  cuparse.cpp: funcsVarsClear (selective deletions at CLEAR), cuptokeClean (emptying symtab at cleanup). */
{
	if (st->p && st->nAl)		// if symbol table allocated
	{
		STAE* p = st->p;				// scanning pointer
		STAE* q = p;				// store pointer for compacting deleted out entries
		STAE* pEnd = p + st->n;			// cuz st->n may be decremented in loop
		for ( ;  p < pEnd;  p++)   		// loop over entries in symbol table
		{
			SI tt = p->iTokTy & TOKTYMASK;  	// get token type, clearing flags
			if ( ( tt==tokTy 			// if token type matches
			|| tokTy==-1 )			// or "all" specified
			&&  ( !callBack			// and no callback given
			|| (*callBack)(tt,p->stbk) ) )	// or callback says this one should be deleted
			{
				// delete this entry. callBack free'd stbk if in dm.
				st->n--;				// say one less entry.  no q++ --> entry will be overwritten
			}
			else					// retain entry
			{
				if (p != q)			// if there has been a deletion
					memcpy( q, p, sizeof(STAE));	// copy this entry to its new position
				q++;				// point next position
			}
		}
	}

// delete the symbol table if empty
	if (!st->n)
	{
		st->nAl = 0;
		dmfree( DMPP( st->p));
		//isSorted not changed: caller must clear if desired.
	}
	return RCOK;
}		// syClear

/*==================== Symbol Table INTERNAL FUNCTIONS ====================*/

//===========================================================================
LOCAL INT CDEC /*no NEAR!*/ syCmp( const void *p, const void *q)

// symbol table entry compare fcn for C library bsearch, lfind, qsort

/* always compares case-sensitive on global flag 'sytbCass' (set for sorting or finding insertion place in table),
   else compares case-insensitive if EITHER p or q flagged case-insens */
{
	SI t;
	/* So binary search will work whether done case-sensitive or insensitive,
	   use case differences only to resolve case-insensitive equalities.
	     sort in order        ABC, ABc, ABD
	     not (strcmpi order)  ABC, ABD, ABc */

	/*** FUNCTIONALITY OF syCmp AND rSyCmp MUST BE MAINTAINED THE SAME ***/

#define P ((STAE *)p)
#define Q ((STAE *)q)

	t = compareCaseStr( P->ID(), Q->ID() );		// compare case-insensitive.  ID: see start this file
	if (t != 0)					// if different case-insens
		return t;				// always use that result

	// have case-insensitive equality
	if (sytbCass==0				// if global caseSens flag off
	&& (P->iTokTy | Q->iTokTy) & SYTBCASI)	// and EITHER entry case-ins
		return t;				// use case-insens result

	return strcmp( P->ID(), Q->ID() );		// compare case-sensitive
}				    // syCmp

//===========================================================================
/* BCC32 4.0 with FC compiled code that loaded bx b4 storing pp from ebx,
   resulting in information return in wrong location. */
LOCAL RC NEAR rBsearch( 	// our binary search

	/* advantages over C library's: returns place for insertion if not found;
				        our compare function is FC NEAR: may be faster */

	STAE *key, 		// key to seek
	SYTBH *st, 		// symbol table to seek in
	STAE **pp )		// NULL or rcvs ptr to found entry or place to insert

// returns: RCOK: exact match found; RCBAD: not found, *pp is where to put it
{
	USI l, u, i;
	SI cmp;
	RC rc=RCBAD;

	l = 0;			// lower limit for key's position
	u = st->n;			// upper limit +1
	for ( ; ; )				// loop til break
	{
		i = (l + u)/2;			// test middle of range next
		// NB endtest positioned to handle st->n==0.
		if (l >= u)			// if no range left, place found
			break;			// i is where to insert, rc is RCBAD.
		cmp = rSyCmp( key, st->p + i);	// compare key to an entry. -1, 0, 1.
		if (cmp==0)			// if key == entry, found
		{
			rc = RCOK;			// say found
			break;
		}
		if (cmp < 0)			// if key < entry, position is b4 i.
			u = i;			// i is new upper limit + 1
		else				// key >, will be found after i
			l = i+1;			// new lower limit
	}
	if (pp)			// if return info ptr not nULL
		*pp = st->p + i;		// loc of found entry or place to insert
	return rc;
}		// rBsearch
//===========================================================================
// 32 bit FC removed after trouble with rBsearch -- similar call.
LOCAL RC NEAR rLfind( 		// our linear search

	/* advantage over msc's: call same as rBsearch;
				 our compare function is FC NEAR: may be faster */

	STAE *key, 		// key to seek
	SYTBH *st, 		// symbol table to seek in
	STAE **pp )		// NULL or rcvs ptr to found entry or place to insert

/* returns: RCOK: exact match found;
            RCBAD: not found, *pp is where to put it (end) */
{
	STAE *p, *end;
	RC rc=RCBAD;

	end = st->p + st->n;
	for (p = st->p; p < end; p++)
		if (rSyCmp( key, p)==0)
		{
			rc = RCOK;
			break;
		}
	if (pp)
		*pp = p;
	return rc;
}		// rLfind
//===========================================================================
LOCAL SI FC NEAR rSyCmp( const STAE *p, const STAE *q)

// NEAR FC symbol table entry compare fcn for rBsearch, rLfind

/* always compares case-sensitive on global flag sytbCass (set for sorting or finding insertion place in table),
   else compares case-insensitive if EITHER p or q flagged case-insens */
{
	SI t;
	/* So binary search will work whether done case-sensitive or insensitive,
	   use case differences only to resolve case-insensitive equalities.
	     sort in order        ABC, ABc, ABD
	     not (strcmpi order)  ABC, ABD, ABc */

	/*** FUNCTIONALITY OF syCmp AND rSyCmp MUST BE MAINTAINED THE SAME ***/

	t = compareCaseStr( p->ID(), q->ID());			// compare case-insensitive.  ID: see start of this file.
	if (t != 0)					// if different case-insens
		return t;				// always use that result

	// have case-insensitive equality
	if (sytbCass==0				// if global caseSens flag off
			&& (p->iTokTy | q->iTokTy) & SYTBCASI)	// and EITHER entry case-ins
		return t;				// use case-insens result

	return strcmp( p->ID(), q->ID());		// compare case-sensitive
}				    // syCmp

// end of sytb.cpp
