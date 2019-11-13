// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// sytb.h: defines and decls for symbol table functions in sytb.cpp

#if !defined( _SYTB_H)
#define _SYTB_H

/*------------------------ SYMBOL TABLE STRUCTURES ------------------------*/
struct STBK			// symbol table block base class
					// (actual *stbk structs are up to appl but always have id 1st)
{	char* id;
	STBK() : id( NULL) {}
	STBK( char* _id) : id( _id) {}
	STBK( const char* _id) : id( const_cast<char *>(_id)) {}
	char* ID() const { return id; }
};
struct STAE		// sym table array struct: SYTBH.p points to array of these
{
    // name of symbol (lookup key) is not in STAE itself, but is STAE.stbk.id.
    STBK* stbk;	// appl's pointer to info dependent on type: block of whatever size and type appl needs.
				//    FIRST member of stbk is always "char *id;" */
    USI iTokTy;	// SYTBCASI bit: on if case-insensitive symbol.
    			//   lo bits (TOKTYMASK): appl's token type etc for symbol
    			//		CUTxxx define) */
	char* ID() const { return stbk->ID(); }
};	// struct STAE
const USI SYTBCASI = 0x8000;	// bit in iTokTy indicating case-insensitive entry
const USI SYTBRES  = 0x4000;	// bit reserved for next feature to be added
const USI TOKTYMASK = (~(SYTBCASI|SYTBRES));

struct SYTBH	// symbol table header (base) structure
{   STAE *p;		// pointer to array of STAE entries
    USI n;			// # entries in use
    USI nAl;		// # entries allocated, if in dm
    SI isSorted;	// nz if is in ascii order: can binary search
    SI spare;
};

/* initialization of SYTBH's:
   1. an all-0 SYTBH will work; call sySort for it to make it a sorted table.
   2. can data-initialize desired initial table contents; syAdd will
      duplicate in dm to permit modification. */

/*-------------------------------- DEFINES --------------------------------*/
// syAdd call "op" bits
const int DUPOK=EROP2;	// no error (and no add) if same id, tokTy, casi. stbk match NOT checked! (size not known)

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
RC FC syLu(  SYTBH *sytbh, char *id, BOO casi, SI *pTokty=NULL, void **pStb=NULL);
RC FC syAdd( SYTBH *sytbh, SI tokTy, BOO casi, STBK* stbk, int op);
RC FC syDel( SYTBH *sytbh, SI tokTy, BOO casi, BOO nearId, char *id, BOO uBad=FALSE, void **pStb=NULL);
RC FC sySort( SYTBH *sytbh);
RC FC syClear( SYTBH *st, SI tokTy=-1, BOO (*callBack)(SI tokTy, STBK *&pStbk) =NULL);

#endif	// _SYTB_H

// end of sytb.h
