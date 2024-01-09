// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// messages.cpp -- CSE message string retriever

//   see also msgtbl.cpp: defines/inits msgTbl[] and msgTblCount[]


/* STORY:  These functions retrieve text strings from msgTbl[] and format
   printf-style arguments into them.  Text can be passed as a string literal
   or identified with a 16-bit "message handle" (MH_xxxx constants #defined
   in msghans.h).  Examples:
      msg( NULL, "Bad value: %d", x)	returns ptr to Tmpstr formatted string
      msg( myBuf, "Bad value: %d", x)   returns ptr to fmtd msg in myBuf
      msg( myBuf, MH_U0003, x)  retrieves string having handle
      					MH_U0003 (as defined in msgtbl.cpp),
					fmts into myBuf, returns ptr
   See also rmkerr.cpp for remark display and error handling functions
   which access these functions.

   On compile option, text can be in disk file; file is opened and text
   retrieved here.  Program can call msgInit to specify file name and search paths,
   or file will auto-open here using compiled-in default filename (searches DOS path only).
   File will be found anywhere on path provided by caller or on DOS path. */

/*-------------------------------- OPTIONS --------------------------------*/


/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"

#include "lookup.h"	// lookws
#include "rmkerr.h"	// errCrit

#include "messages.h"	// decls for this file


/*------------------------- SEMI-PUBLIC VARIABLES --------------------------*/
// defined in msgtbl.cpp, public but declared and refd ONLY in this file (and msgw.cpp)
extern MSGTBL msgTbl[];		// message table
extern SI msgTblCount;		// number of messages in msgTbl[]

/*-------------------------------- VARIABLES -------------------------------*/
LOCAL SI msgIsinit = 0;			// non-0 after message world initialized this session
/*----------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL void msgSort( void);
LOCAL const char* msgFind( MH mh);
LOCAL int msgCompare( const void *m1, const void *m2); // for qsort
LOCAL RC msgCheck( int erOp,	const char *pMsg);

//==============================================================================
const char* MSGORHANDLE::mh_GetMsg(
	const char* nullMsg /*=""*/) const	// returned iff mh_
// return text for MSGORHANADLE
// if IsNull(), returns "" (= MHOK / RCOK)
{
	const char* msg = 
		mh_IsNull()     ? nullMsg
		: mh_IsHandle() ? msgFind( mh_GetHandle())
		:                 mh_msgOrHandle;
	return msg;
}		// MSGORHANDLE::mh_GetMsg
//==============================================================================
void msgClean()   	// cleanup function: DLL version must release ALL memory. rob 12-93.

{
	if (!msgIsinit)		// if has not been initialized
		return;			// do nothing
	msgIsinit = 0;		/* say not initialized --> if error cleaning up, don't try again:
    				   would be likely to create loop preventing program from exiting. */
}		// msgClean

//==============================================================================
RC msgInit(   	// message world (auto) initialization

	int erOp )	// controls reporting of (non-fatal) errors

// returns RCOK:  completely successful initialization
//         RCBAD: non-fatal msgTbl[] errors found & msg'd per erOp, execution can continue
//    aborts on fatal error (not possible 8-10)

// and msg world users: msgI,  (in case error in constructor).
{
	RC rc = RCOK;						// init return value to "no error" (0).

	if (!msgIsinit)
	{

		// sort and check messages
		msgSort();					// sort msgTbl, local fcn, below
		for (int i=-1; ++i < msgTblCount; )		// scan/check msgTbl
		{
			if (i  &&  msgTbl[i-1].msgHan == msgTbl[i].msgHan)
				rc = errCrit( erOp, "X0001: msgInit(): duplicate message handle '%d' found in msgTbl[]", (INT)msgTbl[i].msgHan);
			rc |= msgCheck( erOp, msgTbl[i].msg); 	// check msg length, below; merge errors
		}
		msgIsinit++;					// say message world intialization completed
	}
	return rc;
}			// msgInit
//==============================================================================
const char* msg(		// retrieve message text, format args

	char *mBuf,		// ptr to buffer into which to format, NULL for TMPSTR
	//   buffer dim = [MSG_MAXLEN] is PROBABLY large enough
	MSGORHANDLE mOrH,	// ptr to printf fmt message (e.g. "Bad value: %d")
						// OR message handle (e.g. MH_X1002); message
						//    handles are defined in msghan.h and are associated with
						//    text in msgtab:msgTbl[]
	...)		// values as required by message

// CALLERS BEWARE: only protection against memory overwrite is large value of MSG_MAXLEN.
//   Unformatted msgs are checked for length MSG_MAXLEN/2, but there is no check on length of formatted msg !!

// returns ptr to formatted msg; if mBuf was NULL, ptr is to TRANSIENT string
{
	va_list ap;
	va_start( ap, mOrH);

	char stkBuf[ MSG_MAXLEN];
	char* p;
	size_t bufSz;
	if ( mBuf)
	{	p = mBuf;
		bufSz = MSG_MAXLEN;
	}
	else
	{	p = stkBuf;
		bufSz = sizeof( stkBuf);
	}

	int mLen;
	msgI(	// retrieve and format message, discard returned rc (local)
		PWRN,		// error reporting: program error, warn and continue
		p,			// buffer for msg
		bufSz,		// size of buffer
		&mLen,		// formatted length returned
		mOrH,		// caller's message text or message handle
		ap );		// ptr to args for vsprintf() in msgI

	if (!mBuf)					// if caller wants Tmpstr
		p = strncpy0( NULL, stkBuf, mLen+1);	// copy string to Tmpstr

	return p;		// return pt to caller's mBuf or to Tmpstr
}		// msg
//==============================================================================
RC msgI(			// retrieve message text, format args: inner function

	int erOp,			// bits controlling error reporting
	char* mBuf,			// ptr to buffer to receive message; MUST be large enuf to hold message
						//   (messages.h MSG_MAXLEN is probably large enough)
	size_t mBufSz,		// sizeof( *mBuf)
	int* pMLen,			// NULL or ptr for return of message strlen
	MSGORHANDLE mOrH,	// msg text OR message handle
						//   mOrH.IsNull(): return ""
	va_list ap/*=NULL*/) // printf-style values are reqd by msg text
						//   NULL: no formatting (vsprintf not called)

// resolves mOrH to full message text (if required) and formats message

// returns RCOK (mBuf="")    if mOrH==NULL
//         RCFATAL (mBuf="") if message too long
//         RCBAD (mBuf set)  if mOrH pts to text
//         caller's mOrH (mBuf set) if handle
{
	msgInit( PWRN);   		// first time called, init msg world: in case error occurs in constructor.

	*mBuf = '\0';			// insurance: return "" for exceptions
	if (pMLen)
		*pMLen = 0;
	if (mOrH.mh_IsNull())		// insurance
		return RCOK;		//   NULL mOrH: NOP

	const char* pMsg = mOrH.mh_GetMsg();
	RC rc = mOrH.mh_GetRC();

	// here with: pMsg pointing to useable message text
	//            mh suitable for return as RC

	size_t mLen{ 0 };
	bool tooLong;
	if (ap)
	{	mLen = vsnprintf (mBuf, mBufSz - 1, pMsg, ap);	// format final msg in mBuf
		tooLong = mLen < 0;
		if (tooLong)
			mBuf[ mBufSz-1] = '\0';
	}
	else
	{	// no formatting
		strncpy0( mBuf, pMsg, mBufSz);
		mLen = strlen( mBuf);
		tooLong = mLen >= mBufSz-1;
	}
	if (tooLong)
	{	// buffer not big enough
		errCrit( erOp,
			"X0002: msgI(): Message '%1.15s ...' is too long\n"
			"    (max length is %d)",
			  mBuf, mBufSz);
		rc = RCFATAL;
	}

	if (pMLen)				// if pointer given (rob 11-91)
		*pMLen = static_cast<int>(mLen);			// return message length

	return rc;
}			// msgI

//===========================================================================
const char* msgSec( 	// get sub-msg text for system error code

	SEC sec )	// "System" Error Code: c library codes 0 - sys_nerr, or our SECOK, SECEOF, SECOTHER (defined in cnglob.h).

/* SEC values: SECOK: -1				(cnglob.h)
	       our other values: -2, -3, -4, ...	(SECEOF, SECBADFN, etc, cnglob.h, texts HERE in this fcn)
	       system-defined errors: 0, 1, 2, ... 	(C library symbols are ENOENT etc (errno.h);
			       				   symbols rarely used in this program. rob 2-92) */

// these messages are typically imbedded in messages giving context, for example, see xiopak.cpp.

// return value is pointer to transient msg (in Tmpstr)
{
	static WSTABLE secText[] =
    {	{ SECOK,		"no error" },
        { SECEOF,		"end of file" },
        { SECBADFN,		"invalid file name" },
        { SECBADRV,		"invalid drive letter" },
        { SECOTHER,		"unknown error, errno not set" },
        { SECBADPATH,		"No such file or directory"},
        { 32767,		"unhandled system error" }			// terminator
	};

	const char *pText =				// pt to text corresponding to SEC
		// sys_nerr and sys_errlist are deprecated
		// https://github.com/jgbit/vuda/issues/11
#if 0
		sec >= 0 && sec < sys_nerr 	// for errors in system range
	      ? sys_errlist[sec]  		//    get msc library msg text
	      :
#endif
		lookws(sec, secText);   //    else get text from our table
	return strtprintf(			// format msg in Tmpstr, strpak.cpp
			   "%s (SEC=%d)",			//   eg "end of file (SEC=-2)" or "no such file or directory (SEC=2)"
			   *pText 					//   if non-"" corresponding text
				  ? pText 				//      use it
			      : "undefined SEC",	//      otherwise, admit ignorance
			   (INT)sec );		//   sec value
}	// msgSec

//==============================================================================
LOCAL void msgSort()	// sort msgTbl[] by message handle
{
	qsort(			// quick-sort, msc lib
		msgTbl,			//   base table, defnd in msgtab.cpp
		msgTblCount,  		//   number of entries in msgTbl
		sizeof(MSGTBL),		//   msgTbl element size
		msgCompare ); 		//   comparison function
}			// msgSort

//==============================================================================
LOCAL const char* msgFind(	// retrieve text for a message handle

	MH mh )		// message handle of message being sought

// returns ptr to MASTER COPY of message (dup b4 modifying).
// message should be copied before next call here as disk text is volatile.
{

// find table entry for handle

	const MSGTBL* p = (const MSGTBL *)bsearch(		// binary search, C library
		&mh,  		//   key
		msgTbl,		//   base table, defnd in msgTbl.cpp
		msgTblCount,  	//   number of entries in msgTbl
		sizeof(MSGTBL),	//   msgTbl element size
		msgCompare );  	//   comparison function

	if (!p)									// if match not found
	{
		errCrit( WRN, "X0005: No message for message handle '%d'", mh); 	//   report error per erOp,
		return "Bad message handle";						//   return a message (3-92)
	}
	else					// message handle found
		return p->msg;				// msgTbl contains pointer to text in ram

}				// msgFind

//==============================================================================
LOCAL int msgCompare(	// compare messages (for qsort and bsearch)

	const void *m1,		// ptrs to msgTbl[] entries to be compared
	const void *m2 )		// .. (actual type  const MSGTBL *  but C++ requires void * to match library fcn decls)

// compares MSGTBL mh values
// returns <0 (mh1<mh2), 0 (mh1==mh2), or >0 (mh1>mh2)
{
	return ((const MSGTBL *)m1)->msgHan < ((const MSGTBL *)m2)->msgHan
	? -1							// mh1 < mh2
	: ((const MSGTBL *)m1)->msgHan > ((const MSGTBL *)m2)->msgHan;		// else 0 if ==, 1 if >

}			// msgCompare
//==============================================================================
LOCAL RC msgCheck(			// check an unformatted message

	int erOp,		// error reporting control
	const char *pMsg ) 	// message to be checked

// returns RCOK:    msg passes all tests
//         RCBAD:   msg failed test(s)
//         RCFATAL: msg is unuseable
{
	RC rc=RCOK;
	// could scan msg and look at all %X to get a better guess at fmtd length. FOR NOW, just check its length.
	USI chkLen = MSG_MAXLEN/2;			// MSG_MAXLEN = 2000, messages.h.  Allow half this before formatting.
	USI len = (USI)strlen(pMsg);
	if (len > chkLen)
	{
		rc = errCrit( len > 3*MSG_MAXLEN/4 ? ABT : erOp, 			// if 3/4 buffer size, abort -- else crash likely
					  "X0002: msgCheck(): Message '%1.15s ...' is too long \n"
					  "    (max length is %d)",
					  pMsg, (INT)chkLen);
		//pMsg[chkLen] = '\0';	// truncate: to do this we'd need to remove lots of "const"s including on strpak fcns, 3-92.
	}
	return rc;
}			// msgCheck

// end of messages.cpp
