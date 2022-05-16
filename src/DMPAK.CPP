// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// dmpak.cpp: Dynamic memory routines for CSE

#include "CNGLOB.H"	// global defns

#if 0	// debug aid, 7-10
x #include "ancrec.h"
x #include "rccn.h"
x #include "cnguts.h"
x #include "irats.h"
#endif


const USI dmBlkOvhd = sizeof( USI) + sizeof( size_t);  // total overhead
//  p-(6 bytes) = 16 bit reference count
//  p-(4 bytes) = size_t size as malloc'd (w/o any compiler overhead)
static inline void* dmAppP( void* q) { return (char *)q + dmBlkOvhd; }
static inline void* dmMallocP( void* p) { return (char *)p - dmBlkOvhd; }
static inline USI& dmRefCount( void* p) { return *(USI *)dmMallocP( p); }
static inline ULI& dmSize( void* p) { return *(((ULI*)p)-1); }

// Statistics available to tune memory mgmt
//  TODO: not maintained in MSVC
ULI Dmused = 0L;	// Number of bytes currently allocated in dm (including overhead, at least under MSC)
ULI DmMax = 0L;		// largest Dmused seen
static USI DmCount = 0;	// number of blocks currently in use

//=============================================================================
static DMP mgMalloc(	// malloc with statistics
   size_t sz, 	// number of bytes desired in block
   int op )		// DMZERO bit: zero the allocated storage
// returns pointer to block, or NULL
{
	DMP p=NULL;
	size_t blkSz = sz + dmBlkOvhd;
    DMP q = malloc( blkSz); 	// adjust size for overhead
    if (q != NULL)			// if succeeded
    {	p = dmAppP( q);	// app pointer (after overhead)
		dmSize( p) = blkSz;
		dmRefCount( p) = 1;
		DmCount++;			// statistics: # blocks in use
		Dmused += blkSz; 	// .. # bytes in use
		if (Dmused > DmMax)		// largest Dmused seen
			DmMax = Dmused;
		if (op & DMZERO)			// on option
			memset( p, 0, sz);		// zero the new storage
    }
    return p;
}		// mgMalloc
//-----------------------------------------------------------------------------
static DMP dmralloc(	// Reallocate heap space using realloc()

   DMP pOld,	// app pointer to previously allocated dm block
				//  if NULL block will be allocated here)
   size_t nuSz,	// new app size of block
   int erOp )	// error actions (if cannot get storage): IGN, WRN, ABT
				//  and Option bit: DMZERO (dmpak.h): zero added memory
// Returns dmralloc pointing to reallocated space (or NULL)
{
    DMP pNu=NULL;
	size_t oldsz = 0;
    if (pOld==NULL)				// if no existing block
       pNu = mgMalloc( nuSz, erOp);
    else
    {	DMP qOld = dmMallocP( pOld);		// make internal (malloc) ptr
		oldsz = dmSize( pOld);				// total size of old blk
		size_t oldsz2 = oldsz - dmBlkOvhd;	// size of old blk w/o overhead
		DMP qNu = realloc( qOld, nuSz + dmBlkOvhd);
		if (qNu)
		{	pNu = dmAppP( qNu);
			dmSize( pNu) = nuSz + dmBlkOvhd;
			Dmused += dmSize( pNu) - oldsz;	// statistics
			if (Dmused > DmMax)
				DmMax = Dmused;
			if (erOp & DMZERO)      			// zero added memory option
			{	// note oldsz==0 does not get here
				if (nuSz > oldsz2)		// if enlarged
					memset( (char *)pNu + oldsz2, 0, nuSz-oldsz2);
			}
		}
    }

    if (!pNu)											// if failed
       errCrit( erOp, "X0021: dmralloc(): Trouble!\n"
                      "    bytes requested=%u  Old size=%u",
					  nuSz, oldsz);
    return pNu;
}		// dmralloc
//-----------------------------------------------------------------------------
int dmIsGoodPtr(  	// identify bad heap pointer
   DMP p,			// app pointer
   const char* s,	// function name to insert in error messages
   int erOp ) 		// IGN/REG/WRN/ABT/PERR/PWRN/PABT (cnglob.h)
// returns 0 iff p is definitely not a valid heap pointer
{
    if (!p)
       return 0;						// NULL pointers are nops, no error message
#if 0	// debugging trap, 7-10
x	RC rc = 0;
x	if (p == Top.tp_wfName || p == Topi.tp_wfName)
x		rc = RCOK;
#endif

    // most specific checks first for best error messages.
    // messages need message numbers; update comments in msgtbl.cpp.

    if (!strCheckPtr( p))
    {	errCrit( erOp, "dmpak.cpp:%s: pointer %p points into temporary string storage Tmpstr[]", s, p);
		return 0;
    }
#if defined( _DEBUG)
#if 1
	if (!_CrtIsMemoryBlock( dmMallocP( p), dmSize( p), NULL, NULL, NULL))
#else
	if (!_CrtIsValidHeapPointer( dmMallocP( p)))
#endif
    {	errCrit( erOp, "dmpak.cpp:%s: _CrtIsMemoryBlock( %p) returned 0", s, p);
		return 0;
    }
#endif
    return 1;
}			// dmIsGoodPtr
//------------------------------------------------------------------------------------------------
RC dmCheckMemory(
	const char* doing /*=NULL*/)
{
#if defined( CSE_MFC)
	if (AfxCheckMemory())
#else
	if (_CrtCheckMemory())
#endif
		return RCOK;

	if (!doing)
		doing = "?";
	return errCrit( ABT, "dmCheckMemory() failure at %s", doing);
}		// dmCheckMemory
//================================================================================================
RC dmal(		// allocate dynamic memory
	DMP* pp,		// mem pointer pointer
	size_t sz, 		// size in bytes
	int erOp )		// error (IGN, WRN, ABT) action plus options:
					//  DMZERO bit: zero the new memory
					//  DMFREEOLD: free existing memory if pointer not NULL
// returns RCOK ok, else other (non-0) value (if not ABT), msg'd (if not IGN).
{
    if (erOp & DMFREEOLD)
		dmfree( pp);				// on option only, free *pp if not NULL; AUTOINITs.
    DMP p = mgMalloc( sz, erOp);	// AUTOINIT, get space, do statistics if ok, DMCHECK stuff.
		      						// uses only DMZERO option bit in erOp.
    if (!p)
       return errCrit( erOp, "X0030: dmal(): OUT OF MEMORY\n"
                             "    bytes requested=%u  DmUsed=%ld  DmMax=%ld", sz, Dmused, DmMax );	// rmkerr.cpp. returns RCBAD.
	*pp = p;
    return RCOK;
}				// dmal
//-----------------------------------------------------------------------------
RC dmral(		// reallocate dynamic memory with unique pointer
	DMP* pp,		// reference to pointer to block to reallocate
					//		(if contains NULL, new block allocated)
	size_t sz, 		// new size in bytes
	int erOp )		// error action

// returns RCOK if ok
{
    DMP p1 = dmralloc( *pp, sz, erOp);	// old reallocator.  AUTOINITs.
    if (!p1)
		return RCBAD;		// if failed tell caller
    *pp = p1;				// ok, now alter caller's pointer
    return RCOK;
}			// dmral
//-----------------------------------------------------------------------------
RC dmfree( DMP* pp)		// free dynamic memory via reference, NULL caller's pointer
// nop if p already NULL; NULLs p.
{
    RC rc=RCOK;
    if (dmIsGoodPtr( *pp, "dmfree", PERR))	// no free if NULL or obviously bad
	{	dmRefCount( *pp)--;
		if (dmRefCount( *pp) <= 0)
		{	if (dmRefCount( *pp) < 0)
				rc= errCrit( PWRN, "dmfree(): X0032: attempt to free block with 0 ref count at %p, length 0x%x",
                      *pp, dmSize( *pp) );
			Dmused -= dmSize( *pp);
			DmCount--;
			free( dmMallocP( *pp));
		}
	}
    *pp = NULL;
    return rc;
}		// dmfree
//-----------------------------------------------------------------------------
RC dmIncRef( DMP* pp, int erOp/*=ABT*/)	// duplicate block or ++ref count (as implemented) after pointer copied
	// nop if p is NULL.
	// note: to decrement reference count, use dmfree.
{

    if (dmIsGoodPtr( *pp, "dmIncRef", erOp))
		dmRefCount( *pp)++;
    return RCOK;
}			// dmIncRef
///////////////////////////////////////////////////////////////////////////////

// end of dmpak.cpp
