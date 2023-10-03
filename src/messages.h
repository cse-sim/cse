// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// messages.h -- definitions and declarations for messages.cpp (CSE message string retriever),
//               msgtab.cpp (CSE message text), and msgw.cpp (CSE message file creator).


/*-------------------------------- DEFINES --------------------------------*/

const int MSG_MAXLEN = 2000;	// PROBABLE safe dimension for msg buffers, for use as as dim in messages.cpp, rmkerr.cpp, etc.
				//   CALLERS BEWARE: Formatted msg len is not known until vsprintf() is done so hard
				//   enforcement not possible.
				//   messages:msgCheck verifies that tabulated msgs are shorter than MSG_MAXLEN/2.

/*--------------------------------- TYPES ---------------------------------*/

//---- message handle ----
//MH is typedef'd in cnglob.h.

//---- entry in msgTbl[] ----
struct MSGTBL
{   MH msgHan;			// message handle: int 0 - 16384 as defined in msghans.h
    const char* msg;	// pointer to message text associated with msgHan
};

/*--------------------------- PUBLIC VARIABLES ----------------------------*/
// msgtab.cpp public variables declared and refd ONLY in messages.cpp and msgw.cpp
//  MSGTBL msgTbl[]  or  MSGTBL msgTbl*;		message table, or offset table for texts on disk
//  SI     msgTblCount;	number of entries in msgTbl


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// messages.cpp
void FC msgClean();
RC msgInit( int erOp);
const char* CDEC msg( char *mBuf, const char *mOrH, ...);			// ALSO DECL IN cnglob.h
RC msgI( int erOp, char* mBuf, size_t mBufSz, int* pMLen, const char* mOrH, va_list ap=NULL);
const char* FC msgSec( SEC sec);
bool FC msgIsHan( const char* mOrH);
MH msgGetHan(const char* mOrH);

// messages.h end
