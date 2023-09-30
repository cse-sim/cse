// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// dmpak.h: declarations for heap (dynamic memory) functions for cse

	/* functions in dmpak.cpp;
	   [also former near heap functions in nheap.cpp for compilers whose libraries don't have near heap (Borland)] */

#ifndef DMPAK_H		// endif at end file
#define DMPAK_H		// (because included in ancrec.h, at least temporarily)

/*-------------------------------- DEFINES --------------------------------*/

// in cnglob.h for use in other .h files and many .cpp files:
//  typedef void *DMP;	* Dynamic memory block pointer: ptr to any type, record struct, etc, of caller's.

// options/erOp bits for indicated dmpak.cpp functions.   Note EROP1... defined in cnglob.h.
const int DMZERO = EROP1; 		// zero allocated storage: dmalloc, dmvalloc, [dmpal], etc.
const int DMFREEOLD = EROP2; 	// if pointer nonNULL, free it first. [dmpal], etc.
#if defined( _DEBUG)
#define DMHEAPCHK( s) dmCheckMemory( s);
#else
#define DMHEAPCHK( s)
#endif


/*--------------------------- PUBLIC VARIABLES ----------------------------*/

extern size_t Dmused;	// Current number of dm bytes in use
extern size_t DmMax; 	// max Dmused seen

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// dmpak.cpp
RC dmCheckMemory( const char* where=NULL);
RC dmal( DMP *pp, size_t sz, int erOp);
RC dmral( DMP *pp, size_t sz, int erOp);
RC dmfree( DMP *pp);			// checks stack, no FC
RC dmIncRef( DMP *pp, int erOp=ABT);
RC dmPrivateCopy( DMP *pp, int erOp=ABT);
int dmIsGoodPtr( DMP p, const char* s, int erOp);
void dmInitMemoryChecking();

#ifdef DEBUG2
  void FC dmeatmem( USI sz);
#endif

#endif	// #ifndef DMPAK_H at start file

// end of dmpak.h
