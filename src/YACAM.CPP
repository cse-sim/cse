// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// yacam.cpp  Yet Another Character Access Method

// This file contains member functions for YACAM objects, which
//   do random and sequential file i/o, and also read ascii files by
//   token and decode tokens of several types

// Also CSVItem / CSVGen helpers for CSV file export

#include "CNGLOB.H"	// CSE global definitions, data types, etc, etc.

#include <io.h>			// _rtl_creat open close read
#include <fcntl.h>		// O_BINARY O_RDONLY
#include <sys/stat.h>	// S_IREAD, S_IWRITE

#include "MSGHANS.H"	// MH_xxxx defns: disk message text handles
#include "MESSAGES.H"	// msgI: gets disk texts

#include "SRD.H"
#include "CVPAK.H"

#include "DATFCNS.H"

#include "YACAM.H"		// header for this file

//===========================================================================
//  YACAM constructor & destructor
//---------------------------------------------------------------------------
YACAM::YACAM()
{
	init();
}				// YACAM::YACAM
//---------------------------------------------------------------------------
void YACAM::init()	// initialize -- content of c'tor, also separately callable
{
	mPathName = NULL;
	mWhat[0] = 0;
	mFh = -1;
	mWrAccess = dirty = FALSE;
	mFilSz = mFilO = -1L;
	bufN = bufI = mLineNo = 0;
}				// YACAM::init
//---------------------------------------------------------------------------
YACAM::~YACAM()
{
	close();
}			// YACAM::~YACAM
//---------------------------------------------------------------------------
// "error action" (erOp) arguments for all of the following functions:
//   IGN:  ignore: no message
//   ERR:  issue message and continue
//   WRN:  issue message and await keypress
//   There are more. see defines in notcne.h or cnglob.h.
//===========================================================================
//  YACAM open and close functions
//---------------------------------------------------------------------------
RC YACAM::open( 	// open file for reading, return RCOK if ok
	const char* pathName, 		// name to open. no directory search here. fcn fails without message if NULL.
	const char* what /*="file"*/,	// descriptive insert for error messages
	int erOp /*=WRN*/, 			// error action: IGN no message, WRN msg & keypress, etc, above.
	int wrAccess /*=FALSE*/ )  	// non-0 to allow write access as well as read
{
	mErOp = erOp;				// communicate error action to errFl

// nop if NULL pathName given
	if (!pathName)
		return RCBAD;

// copy pathName to heap
	mPathName = new char[ strlen(pathName)+1];
	if (!mPathName)
		return RCBAD;		// memory full. msg?
	strcpy( mPathName, pathName);

// copy 'what' to object
	strncpy( mWhat, what, sizeof(mWhat)-1);
	mWhat[ sizeof(mWhat)-1] = '\0';

// indicate empty buffer, init line count, etc
	bufN = bufI = 0;
	dirty = FALSE;
	mWrAccess = wrAccess;
	mLineNo = 1;
	mFilO = -1L;
	mFilSz = -1L;			// file size "unkown": only used when writing 10-94

// open file, conditionally report error
	mFh = ::open( mPathName, 			 		// C library function
				  (wrAccess ? O_RDWR : O_RDONLY) | O_BINARY,
				  S_IREAD|S_IWRITE );
	if (mFh < 0) 						// returns -1 if failed
		return errFl((const char *)MH_I0101);		// issue message with "Cannot open" (or not) per mErOp, return RCBAD

	return RCOK;				// successful
}			// YACAM::open
//---------------------------------------------------------------------------
RC YACAM::create( 	// create file to be written, return RCOK if ok
	const char * pathName, 		// name to create. fcn fails without message if NULL.
	const char * what /*="file"*/,	// descriptive insert for error messages
	int erOp /*=WRN*/ )		// error action: IGN no message, WRN msg & keypress, etc, above.
{
	mErOp = erOp;				// communicate error action to errFl

// nop if NULL pathName given
	if (!pathName)  return RCBAD;

// copy pathName to heap
	mPathName = new char[strlen(pathName)+1];
	if (!mPathName)  return RCBAD;		// memory full. msg?
	strcpy( mPathName, pathName);

// copy 'what' to object
	strncpy( mWhat, what, sizeof(mWhat)-1);
	mWhat[sizeof(mWhat)-1] = '\0';

// indicate empty buffer etc
	mFilO = -1L;  		// file offset of buffer contents: -1 indicates no buffer contents
	bufN = bufI = 0;
	dirty = FALSE;
	//mLineNo = 1;		line # used only with token-reading method

// open file, conditionally report error
	mFh = ::open( mPathName, 				// open file. C library function.
				  O_CREAT|O_TRUNC|O_BINARY|O_RDWR,	// create file, delete any existing contents, binary, read/write access
				  S_IREAD|S_IWRITE );			// read/write permission
	if (mFh < 0) 					// returns -1 if failed
		return errFl((const char *)MH_I0102);		// issue message with "Cannot create" (or not) per mErOp, return RCBAD

// now have 0-length file with write access
	mFilSz = 0L;					// file size now 0: we just deleted any contents
	mWrAccess = TRUE;				// file is open with write access (as well as read access)

	return RCOK;			// successful
}			// YACAM::create
//---------------------------------------------------------------------------
RC YACAM::close( int erOp /*=WRN*/)	// close file, nop if not opened, RCOK if ok

// returns RCBAD only if file was open and an error occurred
{
	mErOp = erOp;			// communicate error action to errFl
	RC rc = RCOK;				// init return code to "ok"

// if file has been opened, write buffer and close
	if (mFh >= 0)				// if file has been opened
	{
		rc = clrBufIf();				// if open for write, if buffer dirty, write buffer contents

		if (::close(mFh) < 0)			// close file -- C library function
			rc = errFl((const char *)MH_I0103);  	// conditional message containing "Close error on" / return RCBAD
	}

// free pathname storage in heap
	delete[] mPathName;		// nop if NULL

// clear variables
	dirty = mWrAccess = FALSE;
	mFh = -1;
	mFilO = mFilSz = -1L;
	mPathName = NULL;
	return rc;
}			// YACAM::close
//---------------------------------------------------------------------------
RC YACAM::rewind([[maybe_unused]] int erOp /*=WRN*/)
// repos file to beginning
{
	RC rc = clrBufIf();
	if (mFh >= 0)
	{	if (_lseek( mFh, 0, SEEK_SET) == -1L)
			rc = errFl("Seek error on");
	}
	mFilSz = mFilO = -1L;
	bufN = bufI = mLineNo = 0;
	return rc;
}		// YACAM::rewind
//---------------------------------------------------------------------------
RC YACAM::clrBufIf()		// write buffer contents if 'dirty'.
// uses mErOp as set by caller.
{
	RC rc = RCOK;			// init return code to 'no error'
	if ( mFh >= 0			// if file is open
			&&  mWrAccess			// if file is open for writing
			&&  dirty				// if buffer contains unwritten data
			&&  bufN )				// if buffer contains any data
	{
		if ( mFilO >= 0					// if have file offset for buffer contents
				?  _lseek( mFh, mFilO, SEEK_SET)==-1L	//  seek to file postition
				:  _lseek( mFh, 0, SEEK_END)==-1L )		//  else seek to end file
			rc = errFl((const char *)MH_I0104);			// seek failed, conditionally issue error msg containing "Seek error on"

		int nw = ::write( mFh, yc_buf, bufN); 	// write buffer contents, return -1 or # bytes written. C library function.
		if ( nw == -1						// if -1 for error
				||  nw < bufN )   			// if too few bytes written -- probable full disk
			rc = errFl((const char *)MH_I0105);	// conditionally issue message. "Write error on".

		dirty = FALSE;				// say buffer now "clean" (if write failed, data is lost)
	}
	return rc;
}		// YACAM::clrBufIf
//===========================================================================
//  YACAM random i/o functions.
//  Don't intermix char and other i/o here: a given open file should be accessed with
//	read/write/getBytes/putBytes  OR  peekC/getC/toke/get.
//---------------------------------------------------------------------------
int YACAM::read( 		// read to caller's buffer

	char *buf,  int count, 	// buffer address and desired number of bytes
	long filO /*-1L*/,    	// file offset or -1L to use current position
	int erOp /*=WRN*/ )		// error action: IGNore, WaRN, etc -- comments above
// and option bit(s): YAC_EOFOK: no message for reading end of file.

// random read to caller's buffer. Returns -1 if error (msg issued per erOp), 0 if eof, else # bytes read.
// Caller must check # bytes read; no message issued here for eof or short read if erOp & YAC_EOFOK.

// is used as internal function by getBtyes and peekC; may be externally called if peekC/getC/toke/get not being used.
{
	mErOp = erOp;				// communicate error action to errFl

// seek if requested
	if (filO >= 0L)
		if (_lseek( mFh, filO, SEEK_SET)==-1L)
		{
			errFl((const char *)MH_I0104);	// conditional msg. "Seek error on" 2nd use.
			return -1;
		}

// read
	int cr = ::read( mFh, buf, count);		// read bytes. C library function.
	if (cr==-1)					// returns byte count, 0 if eof, -1 if error
	{
		errFl((const char *)MH_I0106);		// "Read error on"
		return -1;
	}
	if (cr < count)		// if got fewer bytes than requested
		if (!(erOp & YAC_EOFOK))			// if caller wants end file message
			errFl((const char *)MH_I0107);	// issue message for 0 or partial count. "Unexpected end of file in".
	// caller must check for enuf bytes read.
	return cr;			// return # bytes read, 0 if already at EOF.
}				// YACAM::read
//---------------------------------------------------------------------------
RC YACAM::write( 		// write from caller's buffer

	char *buf,  int count, 	// buffer address and desired number of bytes
	long filO /*-1L*/,    	// file offset or -1L to use current position
	int erOp /*=WRN*/ )		// error action: IGNore, WaRN, etc -- comments above

// random write from caller's buffer. Returns RCOK if ok.
{
	mErOp = erOp;				// communicate error action to errFl
	if (!mWrAccess)
		return errFl((const char *)MH_I0108);		// cond'l msg containing "Writing to file not opened for writing:"

// seek if requested
	if (filO >= 0L)
		if (_lseek( mFh, filO, SEEK_SET)==-1L)
			return errFl((const char *)MH_I0104);	// cond'l msg containing "Seek error on" (3rd use)

// write
	int cw = ::write( mFh, buf, count);		// write bytes. C library function.
	if ( cw==-1					// if error
			||  cw < count )	// or too few bytes written (probable full disk)
		return errFl((const char *)MH_I0105);		// cnd'l msg containing "Write error on" (2nd use)

// update file size for possible future write-beyond-eof checking
	if (filO >= 0L)				// if we know where we wrote these bytes
		setToMax( mFilSz, filO + count);		// file size is at least write offset + count

	return RCOK;				// ok, all bytes written
}			// YACAM::write
//---------------------------------------------------------------------------
char* YACAM::getBytes(     // random access using buffer in object -- use for short, likely-to-be sequential reads.

	long filO, 		// file offset of desired bytes
	int count,  	// number of bytes
	int erOp /*=WRN*/ )

/* returns pointer to bytes in buffer within object, or NULL if error (message issued per erOp).
   Does no i/o if bytes are already there; always reads entire buffer length if reads at all. */
{
	mErOp = erOp;			// communicate error action to errFl/errFlLn

// write buffer if dirty (not expected 10-94, so not yet optimized re requested data already in buffer)
	clrBufIf();				// above

// start or all of data may be in buffer. use it.

	int nKeep = 0; 		// for number of bytes from start of current buffer contents to keep
	if (mFilO >= 0L && bufN > 0)	// if there is data in buffer
	{
		if (filO >= mFilO  &&  filO+count <= mFilO+bufN)		// if all of the data is in buffer
			return yc_buf + (filO - mFilO);   	// just return its address within buffer
		if (filO >= mFilO  &&  filO < mFilO+bufN)		// if start of data is in buffer
			nKeep = bufN - (filO - mFilO);  	// compute how many bytes to keep
		//else nKeep = 0;
	}
	if (nKeep > 0)
	{
		memmove( yc_buf, yc_buf+bufN-nKeep, nKeep);		// move bytes to keep to start buffer so can read after them
		//mFilO += nKeep; bufN -= nKeep; bufI ??		reset on fallthru
	}

// read enough bytes to fill buffer

	int cr = read( yc_buf + nKeep, sizeof(yc_buf) - nKeep,	// seek and read bytes (mbr fcn). buffer, count.
							  filO + nKeep, 			// file offset
							  erOp | YAC_EOFOK );		/* suppress eof/short read msg cuz want no msg if enuf
                              					   bytes for caller's request but < bufferful. 10-26-94. */
	if (cr==-1)						// if error
		return NULL;				// message already issued
	// 0 or more bytes read; read() issued no msg even if eof.
	mFilO = filO;				// file offset where buffer contents start
	bufN = nKeep + cr;				// # bytes in buffer
	if (bufN < count)				// if don't have enough bytes (file ends b4 or in middle of caller's request)
	{
		errFl((const char *)MH_I0107);    	// cnd'l msg containing "Unexpected end of file in" (2nd use)
		return NULL;
	}
	return yc_buf;					// ok, desired bytes start at front of buffer. Another good return above.
}			// YACAM::getBytes
//---------------------------------------------------------------------------
RC YACAM::putBytes(     // random access using buffer in object -- use for short, likely-to-be sequential writes.

	const char* data,	// pointer to data to write
	int count,  		// number of bytes
	long filO, 			// file offset at which to put bytes (or -1L for "at eof" -- believe unused 10-94)
	int erOp /*=WRN*/ )

// puts data in internal buffer, writes only when necessary or at close (thus errors may be deferred).
{
	mErOp = erOp;			// communicate error action to errFl/errFlLn
	if (!mWrAccess)
		return errFl((const char *)MH_I0108);  	// cnd'l msg with "Writing to file not opened for writing:" (2nd use)

	int i = 0;		// buffer offset at which to put new data

// if data position is within or contiguous after current buffer, add this data to buffer if it fits, else clear buffer

	if ( bufN > 0 					// if any data in buffer
			&&  mFilO == -1L 					// if buffer is to be written at eof
			&&  filO == -1L					// and new data is also for eof
			&&  bufN + count < sizeof(yc_buf) )	// and new data will fit in buffer
	{
		i = bufN;					// put new data after present buffer contents; no write now.
	}
	else if ( bufN > 0					// if any data in buffer
			  &&  mFilO >= 0L 					// if buffer data goes at specified buffer position
			  &&  filO >= mFilO					// if new data does not start before buffer position (thus filO >= 0L)
			  &&  filO <= mFilO + bufN				// if new data does not start after end of buffer contents
			  &&  filO + count <= long( mFilO + sizeof(yc_buf)) )		// if new data would not extend past buffer capacity
	{
		i = (filO - mFilO);		// offset within buffer for the new data
	}
	else						// cannot use current buffer contents
	{
		if (clrBufIf() != RCOK)			// conditionally write buffer contents if "dirty"
			return RCBAD;				// if seek error, write error, etc. (msg issued per mErOp.)
		mFilO = filO;				// say buffer contents start at specified file offset or -1L
		bufN = bufI = 0;				// say no data in buffer
	}

// put data in buffer

	memcpy( yc_buf + i, data, count);		// copy data into and/or after end of buffer contents
	setToMax( bufN, (USI)(i + count));		// new size of buffer contents
	dirty = TRUE;				// buffer needs to be written
	//mFilO was set above if changed
	return RCOK;				// done -- successful
}			// YACAM::putBytes
//===========================================================================
//  string (sequential) write
//---------------------------------------------------------------------------
RC YACAM::printf( 	// "printf" to file with implicit "WRN" error action

	const char *fmt, 		// string to output, max 512 chars after insertion of arguments
	... )			// printf-like arguments as required by "fmt"
{
	va_list ap;
	va_start( ap, fmt);
	return vprintf( WRN, fmt, ap);
}
//---------------------------------------------------------------------------
RC YACAM::printf( 	// "printf" to file

	int erOp, 		// error action: WaRN, ABorT, IGNore, etc as commented above.
	const char *fmt, 		// string to output, max 512 chars after insertion of arguments
	... )			// printf-like arguments as required by "fmt"
{
	va_list ap;
	va_start( ap, fmt);
	return vprintf( erOp, fmt, ap);
}
//---------------------------------------------------------------------------
RC YACAM::vprintf(		// "vprintf" to file

	int erOp, 		// error action: WaRN, ABorT, IGNore, etc as commented above.
	const char *fmt, 		// string to output, max 512 chars after insertion of arguments
	va_list ap )		// pointer to vprintf-like argument list
{
	char buf[514];
	vsprintf( buf, fmt, ap);				// format arguments (if any) into given format string
	return putBytes( buf, strlen(buf), -1L, erOp);	// write (above), return result
}
//===========================================================================
//  YACAM character sequential read functions: peekC, getC, toke, get.
//	don't intermix use of these functions with read/write/getBytes/putBytes above.
//---------------------------------------------------------------------------
char YACAM::peekC( int erOp /*=WRN*/)			// return next character or EOF and leave in buffer

// DO NOT INTERMIX with random i/o here without adding code to manage mFilO and/or bufI.
{
	mErOp = erOp;				// communicate error action to errFl

	//clrBufIf ... nah, cuz intermixed random & token i/o won't now work nohow, 10-94.

// if buffer empty, read it full
	if (bufI >= bufN)				// if no (more) chars in buffer
	{
		bufN = bufI = 0;				// no chars in buffer; next char to use will be yc_buf[0].

		int cr = read( yc_buf, sizeof(yc_buf), -1L, erOp|YAC_EOFOK); 	// read without seek, member fcn above.

		if (cr==-1)				// if error
			return EOF;			// read() issued a message
		if (cr==0)				// if nothing read (eof)
			return EOF;			// indicate end file. No message.
		bufN = cr;				// ok, # chars in buffer is # read.
	}
// return next char in buffer, do not point past it
	char c = yc_buf[bufI];
	return c==0x1a ? EOF : c;	// return EOF for ctrl-z: ctrl-z is stored by some archaic editors to indicate eof
}				// YACAM::peekC
//---------------------------------------------------------------------------
char YACAM::getC( int erOp /*=WRN*/)			// return next character or EOF

// DO NOT INTERMIX with random i/o here without adding code to manage mFilO and/or bufI.
{
	mErOp = erOp;			// communicate error action to errFl, eg if caller calls errFl or errFlLn
	char c = peekC(erOp);	// read if buffer empty, get next char and leave in buffer.
	switch (c)
	{
	case EOF:
		return EOF;		// on EOF bufI is not ++'d so ^Z returns EOF's forever
	case '\n':
		mLineNo++;		// line feed/newline: count lines in file, for error messages
		// fall thru
	default:
		bufI++;		// increment buffer subscript: pass this character
		return c;
	}
}		// YACAM::getC
//---------------------------------------------------------------------------
RC YACAM::toke( 	// return next whitespace/comma delimited token.  decomments, dequotes, deblanks.
	char* tok,			// returned: token
	unsigned tokSz,		// *tok size. must be big enuf to hold \0.
	int erOp /*=WRN*/,	// error message action, comments above.
	const char** ppBuf /*=NULL*/)	// alternative source (else getC())
									// returned updated if used
									// >> not tested, 3-2017
// and option bit(s): YAC_EOFOK: issue no error message at clean EOF.

// Uses getC function (above).
// DO NOT INTERMIX with random i/o here without adding code to manage mFilO and/or bufI.

// Features: scans over c-style /*...*/ comments and includes ws in "ed or 'ed tokens.
//   Old examples: {  126.54          } yields {126.54}
//                 { /* comment */124 }        {124}
//	           { 12/*comment*/4  }        {12},{4}  (2 calls)
//	           { "  124"          }        {124}
//	           { "12""4"          }        {124}
//	           { "12" "4"         }        {12},{4}  (2 calls)
//	           { "'1 24'"         }        {'1 24'}
//	           { "'"124'"'        }        {'124"}
//
//   commas added 10-94 with intent of making spreadsheet files readable:
//	 comma in token (not quoted nor commented) terminates token.
//      comma before token returns null token.
//      spaces, tabs, 1 comma gobbled after token.
//
//   C++ //comments added 10-94: ignore chars thru end line, terminate token
//
//   C /*..*/ comments changed 10-94 to terminate token like whitespace (passing comma)
//
// from xiochar.cpp:cToken, 10-94, before comma and comment changes.

// returns RCOK ok, tok set
//         RCEOF at clean eof before token
//         RCBAD unexpected eof or buffer overrun (msg issued per erOp).

{
//---Action/state table: specifies action & new state per cur state & char type

// actions (hi bits in asTab entries)
static const unsigned char scn=0x00;	// save current char and go to next
static const unsigned char rsc=0x10;	// return saved char, look at current again
static const unsigned char rcc=0x20;	// return current char and go to next
static const unsigned char pcx=0x30;	// pass spaces, tabs, 1 comma and xit -- added 10-94.
static const unsigned char xit=0x40;	// add terminating '\0' to token, return
static const unsigned char efo=0x50;	// "clean" eof (before token) -- added 10-94
static const unsigned char efr=0x60;	// premature EOF, return error

// new state (lo bits in asTab entries)
static const unsigned char LWS=0x00;	// leading ws scan
static const unsigned char LCB=0x01;	// leading comment begin ('/' seen, check for '*' or '/')
static const unsigned char LCS=0x02;	// leading C comment scan (discard chars until '*')
static const unsigned char LCE=0x03;	// leading C comment end ('*' seen, check for '/')
static const unsigned char LPS=0x04;  	// leading C++ comment scan (// seen, discard chars thru end line)
static const unsigned char TOK=0x05;	// token body (return chars)
static const unsigned char TCB=0x06;	// token comment beg ('/' in token, check for '*' or '/')
static const unsigned char TCS=0x07;	// token C comment scan (discard chars until '*')
static const unsigned char TCE=0x08;	// token C comment end ('*' seen, check for '/')
static const unsigned char TPS=0x09;  	// token C++ comment scan (// seen, discard chars thru end line)
static const unsigned char DQT=0x0a;	// " enclosed token (return chars until ")
static const unsigned char SQT=0x0b;	// ' enclosed token (return chars until ')

static const unsigned char asTab[12][9] = 	// [state][ct]
{
/*          10-94   other
     ct-->  \r \n   white sp     /       *        "        '        ,      other     EOF
state	    -------- -------- -------- -------- -------- -------- -------- -------- -------- */
/* LWS */ { scn+LWS, scn+LWS, scn+LCB, rcc+TOK, scn+DQT, scn+SQT, xit,     rcc+TOK, efo     },
/* LCB */ { rsc+TOK, rsc+TOK, scn+LPS, scn+LCS, rsc+TOK, rsc+TOK, xit,     rsc+TOK, rsc+TOK },	// / rsc+TOK-->scn+LPS for //comments 10-94
/* LCS */ { scn+LCS, scn+LCS, scn+LCS, scn+LCE, scn+LCS, scn+LCS, scn+LCS, scn+LCS, efr     },
/* LCE */ { scn+LCS, scn+LCS, scn+LWS, scn+LCS, scn+LCS, scn+LCS, scn+LCS, scn+LCS, efr     },
/* LPS */ { scn+LWS, scn+LPS, scn+LPS, scn+LPS, scn+LPS, scn+LPS, scn+LPS, scn+LPS, efr     },	// 10-94 C++ //comments

/* TOK */ { pcx    , pcx    , scn+TCB, rcc+TOK, scn+DQT, scn+SQT, xit    , rcc+TOK, xit     },
/* TCB */ { rsc+TOK, rsc+TOK, scn+TPS, scn+TCS, rsc+TOK, rsc+TOK, xit    , rsc+TOK, rsc+TOK },	// / rsc+TOK-->scn+TPS for //comments 10-94
/* TCS */ { scn+TCS, scn+TCS, scn+TCS, scn+TCE, scn+TCS, scn+TCS, scn+TCS, scn+TCS, efr     },
/* TCE */ { scn+TCS, scn+TCS, pcx,     scn+TCS, scn+TCS, scn+TCS, scn+TCS, scn+TCS, efr     },	// / scn+TOK-->pcx so comment ends token 10-94
/* TPS */ { xit,     scn+TPS, scn+TPS, scn+TPS, scn+TPS, scn+TPS, scn+TPS, scn+TPS, efr     },	// 10-94 C++ //comments. xit: pass no , after.

/* DQT */ { rcc+DQT, rcc+DQT, rcc+DQT, rcc+DQT, scn+TOK, rcc+DQT, rcc+DQT, rcc+DQT, efr     },
/* SQT */ { rcc+SQT, rcc+SQT, rcc+SQT, rcc+SQT, rcc+SQT, scn+TOK, rcc+SQT, rcc+SQT, efr     }
};

	int overrun = 0;	// 0 ok; non-0 = token buffer overrun (scan continues, msg at return)
	int st = LWS;		// init state
	unsigned toki = 0;	// no returned chars yet
	mErOp = erOp;		// communicate error action to errFl
	const char* srcTy = ppBuf ? "line" : "file";

	while (1)			// loop until complete token or EOF
	{
		// get and classify character
		char c;
		if (ppBuf)
		{	c= **ppBuf;
			(*ppBuf)++;
			if (c == '\0')
				c=EOF;
		}
		else
			c = getC(erOp); 			// get next character
		int ct;  			// character type
		if (c=='\r' || c=='\n') ct=0;		// tentatively end C++ //comment at \r so lineNo not ++'d til next token 10-94
		else if (isspaceW(c))   ct=1;		// determine char type. see also space-tabs distinction in pcx case.
		else if (c=='/')        ct=2;
		else if (c=='*')        ct=3;
		else if (c=='\"')       ct=4;
		else if (c=='\'')       ct=5;
		else if (c==',')        ct=6;		// inserted 10-94
		else if (c==EOF)        ct=8;
		else                    ct=7;

		// perform action for char type in state
		char savc = NULL;
		int reDo;
		do
		{
			reDo = 0;				// normally don't redo action swtch
			switch( asTab[st][ct] & 0xF0 )	// action (saTab hi bits)
			{
			case scn:
				savc = c;		// save char and go on
				break;

			case rsc:
				*(tok + toki++) = savc;	// return prev char
				reDo++;			// and act on curr char again in new state
				goto chkfull;

			case rcc:
				*(tok + toki++) = c;	// return current char
chkfull:
				if ( toki >= tokSz )	// if no room for terminull
				{	toki--;			//    discard character
					overrun++;		//    and set error return
				}
				break;			// continue in all cases

			case efo:
				*(tok + toki) = '\0';		// ok EOF, before start of token. store terminull.
				if (!(erOp & YAC_EOFOK))	// option bit suppresses this error message only
					errFlLn((const char *)MH_I0109,srcTy);	// cond'l msg per erOp (via mErOp), with "Unexpected end of line/file."
				return RCEOF;							// unique return code for clean eof

			case efr:
				*(tok + toki) = '\0';	// premature EOF. store terminull.
				errFlLn((const char *)MH_I0109,srcTy);	// cond'l error message with "Unexpected end of line/file."
				return RCBAD;						// error return

			case pcx:
				for ( ; ; )		// pass spaces, tabs, comma and exit. 10-94.
				{
					c = peekC(erOp);		// look at character without removing from file
					if (c==' ' || c=='\t')	// if space or tab
					{	getC(erOp);		// gobble character
						continue; 		// keep looking
					}
					if (c==',')  		// if comma
						getC(erOp);		// gobble it and fall thru to exit
					break;			// comma (gobbled) or any other char (not gobbled), leave loop & exit.
				}
				// fall thru

			case xit:
				*(tok + toki) = '\0';	// exit. store terminull.
				if (!overrun)				// if ok
					return RCOK;			// good return
				return errFlLn((char *)MH_I0110);	// conditional error message with "Token too long.", return RCBAD
			default:
				;
			}
			st = asTab[st][ct] & 0x0F;	// new state (asTab lo bits)
		}
		while (reDo);			// action loop
	}					// character loop
	/*NOTREACHED */
}		// YACAM::toke
//---------------------------------------------------------------------------
RC YACAM::tokeCSV( 	// return next comma delimited token.  decomments, dequotes, deblanks.
    char* tok, 	unsigned tokSz,	// token buffer and its size. must be big enuf to hold \0.
    int erOp /*=WRN*/, 	// error message action, comments above.
						// and option bit(s): YAK_EOFOK: issue no error message at clean EOF.
	const char** ppBuf /*=NULL*/)		// alternative text source (else getC())
										//  *ppBuf returned updated if used

// DO NOT INTERMIX with random i/o here without adding code to manage mFilO and/or bufI.

// returns RCOK iff success,
//	   RCEOF at clean eof before token
//     RCBAD unexpected eof or buffer overrun (msg issued per erOp).
{
//---Action/state table: specifies action & new state per cur state & char type

// actions (hi bits in asTab entries)
static const unsigned char scn=0x00;	// save current char and go to next
static const unsigned char rsc=0x10;	// return saved char, look at current again
static const unsigned char rcc=0x20;	// return current char and go to next
static const unsigned char rcw=0x30;	// return current char (retain WS) and go to next
static const unsigned char xit=0x40;	// add terminating '\0' to token, return
static const unsigned char efo=0x50;	// "clean" eof (before token)
static const unsigned char efr=0x60;	// premature EOF, return error

// state (lo bits in asTab entries)
//   non-quoted tokens returned trimmed
//   WS retained in quoted
//   "" returns " within quoted
//   quoted multi-line tokens may not be handled correctly
static const unsigned char LWS=0x00;	// leading ws scan
static const unsigned char TOK=0x01;	// token body (return chars)
static const unsigned char DQS=0x02;	// leading " during LWS
static const unsigned char DQT=0x03;	// " enclosed token (return chars until ")
static const unsigned char DQX=0x04;	// " found within DQT, check for double quote

// see comments below re currently unnec char types
#if 1
static const unsigned char asTab[ 5][9] = 	// [state][ct]
{/*                  other
      ct--> \n       white sp     /       *        "        '        ,      other     EOF
 state	    -------- -------- -------- -------- -------- -------- -------- -------- -------- */
/* LWS */ { scn+LWS, scn+LWS, rcc+TOK, rcc+TOK, scn+DQS, rcc+TOK, xit,     rcc+TOK, efo     },
/* TOK */ { xit    , rcc+TOK, rcc+TOK, rcc+TOK, scn+DQS, rcc+TOK, xit,	   rcc+TOK, xit     },
/* DQS */ { rcw+DQT, rcw+DQT, rcw+DQT, rcw+DQT, rcc+TOK, rcw+DQT, rcw+DQT, rcw+DQT, efr     },
/* DQT */ { rcw+DQT, rcw+DQT, rcw+DQT, rcw+DQT, scn+DQX, rcw+DQT, rcw+DQT, rcw+DQT, efr     },
/* DQX */ { xit,     rcc+TOK, rcc+TOK, rcc+TOK, rcc+DQT, rcc+TOK, xit,     rcc+TOK, xit     },
};
#else
static const unsigned char asTab[ 5][9] = 	// [state][ct]
{/*                  other
      ct--> \n       white sp     /       *        "        '        ,      other     EOF
 state	    -------- -------- -------- -------- -------- -------- -------- -------- -------- */
/* LWS */ { scn+LWS, scn+LWS, rcc+TOK, rcc+TOK, scn+DQS, rcc+TOK, xit,     rcc+TOK, efo     },
/* TOK */ { xit    , rcc+TOK, rcc+TOK, rcc+TOK, scn+DQS, rcc+TOK, xit,	   rcc+TOK, efo     },
/* DQS */ { rcw+DQT, rcw+DQT, rcw+DQT, rcw+DQT, rcc+TOK, rcw+DQT, rcw+DQT, rcw+DQT, efr     },
/* DQT */ { rcw+DQT, rcw+DQT, rcw+DQT, rcw+DQT, scn+DQX, rcw+DQT, rcw+DQT, rcw+DQT, efr     },
/* DQX */ { xit,     rcc+TOK, rcc+TOK, rcc+TOK, rcc+DQT, rcc+TOK, xit,     rcc+TOK, efo     },
};
#endif

	int overrun = 0;	// 0 ok; non-0 = token buffer overrun (scan continues, msg at return)
	int st = LWS;		// init state
	unsigned toki = 0;		// no returned chars yet
	unsigned tokiEnd = 0;	// last+1 char to be returned (re trailing WS trim)
	mErOp = erOp;			// communicate error action to errFl
	const char* srcTy = ppBuf ? "line" : "file";

	while (1)			// loop until complete token or EOF
	{  // get and classify character
	   // Note extra char types inherited from toke()
	   //   retained for possible future use, 6-21-05
		char c;
		if (ppBuf)
		{	c= **ppBuf;
			if (c == '\0')
				c=EOF;		// treat end of string as end of file
							//  do not advance beyond end
			else
				(*ppBuf)++;
		}
		else
			c = getC(erOp); 			// get next character
		int ct = c == '\n'            ? 0
			   : isspaceW( c)         ? 1
			   : c == '/'             ? 2
			   : c == '*'             ? 3
			   : c == '\"'            ? 4
			   : c == '\''            ? 5
			   : c == ','             ? 6
			   : c == EOF             ? 8
			   :                        7;

   // perform action for char type in state
		char savc = 0;
		int reDo = 0;
		do
		{	reDo = 0;			// normally don't redo action switch
			char storc = 0;		// character to be stored to tok[] if nz
			BOOL bKeepWS = FALSE;	// TRUE iff token WS should be retained
			int action = asTab[ st][ ct] & 0xF0;
			switch( action)
			{	case scn:	savc = c;		// save char and go on
							break;

				case rsc:	storc = savc;	// return saved char, redo c in new state
							reDo++;
							break;

				case rcc:	storc = c;		// return current char
							break;

				case rcw:	storc = c;
							bKeepWS = TRUE;
							break;

				case efo:  	tok[ tokiEnd] = '\0';	// ok EOF, before start of token. store terminull.
							if (!(erOp & YAC_EOFOK))	// option bit suppresses this error message only
								errFlLn((const char *)MH_I0109, srcTy);	// cond'l msg per erOp (via mErOp), with "Unexpected end of file."
							return RCEOF;								// unique return code for clean eof

				case efr:   tok[ tokiEnd] = '\0';	// premature EOF. store terminull.
							errFlLn( (const char *)MH_I0109, srcTy);	// cond'l error message with "Unexpected end of file."
							return RCBAD;				// error return

				case xit:	tok[ tokiEnd] = '\0';	// exit. store terminull.
							if (overrun)			// if overrun
            					return errFlLn( (const char *)MH_I0110);	// Token too long
							return RCOK;		// good return
				default: ;
			}

			if (storc)
			{	if (toki < tokSz-1)		// if space for char + \0
				{	tok[ toki++] = storc;
					if (bKeepWS || !isspaceW( tok[ toki-1]))
						tokiEnd = toki;		// remember last pos of token
				}
				else
					overrun++;		// out of space, discard char and continue
			}

			st = asTab[ st][ ct] & 0x0F;	// new state (asTab lo bits)

		} while (reDo);			// action loop
    }					// character loop
    /*NOTREACHED */
}		// YACAM::tokeCSV
//---------------------------------------------------------------------------
int YACAM::line( 	// return current line
    char* line,	int lineSz,	// token buffer and its size. must be big enuf to hold \0.
    int erOp /*=WRN*/)  	// error message action + options
// returns -1 if clean EOF before 1st char
//         else >= 0 = line length
{
	char c = NULL;
	int iC;
	for (iC=0; iC < lineSz; iC++)
	{	c = getC( erOp);
		if (c == '\n' || c == EOF)
		{	line[ iC] = '\0';
			if (iC > 0 && line[ iC-1] == '\r')
				line[ --iC] = '\0';
			break;
		}
		line[ iC] = c;
	}
	return iC==0 && c==EOF ? -1 : iC;
}			// YACAM::line
//---------------------------------------------------------------------------
int YACAM::scanLine(			// scan file for line match
	const char* s,			// string sought
	int erOp /*=WRN*/,		// msg control + option bits
							//   EROP1: compare only beg strlen( strTrim( s))
	int maxLines /*=-1*/)	// if > 0, max # of lines to scan
// Note: compare is case insensitive and ignores leading/trailing ws
// returns -1 iff file is at EOF
//         -2 iff maxLines exceeded
//    else # of lines read
{
	int bBeg = (erOp & EROP1) != 0;
	char tS[ 1000];
	strTrim( tS, s);		// trimmed s
	int tSLen = strlen( tS);
	int ret = 0;
	int iL;
	for (iL=1; ret==0; iL++)
	{	if (maxLines > 0 && iL > maxLines)
			ret = -2;
		else
		{	char ln[ YACAM_MAXLINELEN];
			int len = line( ln, YACAM_MAXLINELEN, erOp);
			if (len < 0)
				ret = -1;
			else if (_strnicmp( s, ln, bBeg ? tSLen : len) == 0)
				ret = iL;
		}
	}
	return ret;
}		// YACAM::scanLine
//---------------------------------------------------------------------------
// "control string" chars for YACAM::getLineCSV. Each letter uses a pointer from variable arg list
//	I L F:	short integer, long integer, float.
//      C:	read quoted string to char[] array. Array size follows pointer in var arg list.
//	    D:  date, month and day, no year, leap year flag from caller
//      X:  skip (data discarded, no pointer advance)
//   1-99:	repeat count for following char, using same pointer: for arrays or adjacent members of same type.
//----------------------------------------------------------------------------
#pragma optimize( "", off)		// re bad va_arg code, 9-2010
RC YACAM::getLineCSV( 	// read, decode, and store data per control string
	int erOp, 		// error message control: IGN, WRN, etc per comments above;
					// and option bit(s)
					//   YAC_EOFOK: no error message if clean eof before first token
					//   YAC_NOREAD: don't call line(), 1st var arg points to line to be parsed
	int isLeap,		// leap year flag for Julian date conversion
	const char* cstr, 	// Control string indicating data types of tokens in file (string of I L F C D, above)
	void *p, ... )	// pointer to storage location for each letter in control string.
					//   Ptrs for C's must be followed by size_t array size (incl \0).
    				//	 list terminated with 32-bit NULL (use 0L). */

// returns RCOK if ok
//         RCBAD2 for clean eof before token (no msg if b4 1st token if YAC_EOFOK)
//         RCBAD other error
{
// initialize
	RC rc = RCOK;
	mErOp = erOp;				// communicate error action to errFl/errFlLn
	va_list pp;
	va_start( pp, cstr);		// set up to get variable args, starting after cstr

	char lnBuf[ YACAM_MAXLINELEN];
	const char* pLn;		// strxtok pointer
	if (erOp & YAC_NOREAD)
	{	pLn = va_arg( pp, char *);	// 1st arg points to pre-read line
		if (!pLn)
			return RCBAD;
	}
	else
	{	int len = line( lnBuf, sizeof( lnBuf), erOp);
		if (len < 0)
			return RCBAD2;			// EOF
		pLn = lnBuf;
	}

// Loop over characters in control string
	while (*cstr)
	{	// get control char
		char cc = *cstr++;
		if (isspaceW(cc))  				// deblank
			continue;

		// decode optional count before control letter
		int count;
		for (count = 0;  isdigitW(cc);  cc = *cstr++)	// decode count. if none, 0 treated as 1 by code below.
			count = count * 10 + (cc - '0');
		while (cc && isspaceW(cc))			// deblank again after count
			cc = *cstr++;
		if (!cc)
			break;
		cc = toupper(cc);

		// access variable arg list if not skipping
		size_t size = 0;
		count = 0;
		if (cc != 'X')
		{ 	p = va_arg( pp, void *);	// get result-storing pointer from var arg list
			if (cc=='C')				// for C, pointer is followed by character array size
				size = va_arg( pp, size_t);		// fetch destination size from var arg list
		}

		do	// convert 'count' tokens to type cc, or at least one
		{
			// get token
			char tok[ 256];				// token buffer
#if 1
			// experiment, 3-16-2017
			rc = tokeCSV( tok, sizeof( tok), erOp, &pLn);
			if (rc != RCOK && rc != RCEOF)
				return RCBAD;
#else
			if (!strxtok( tok, pLn, ",", FALSE))
				return RCBAD;		// out of tokens
#endif
			erOp &= ~YAC_EOFOK;				// after first token clear no-message-on-eof option bit
			strTrim( tok);

			// convert token per cc
			switch (cc)
			{
			  default:
				return errFlLn( "Internal error: invalid character '%c' in control string in call to YACAM::getLineCSV().", cc);

			  case 'X':
				continue;

				//case 'S': *(char **)p = strsave(tok);  size=sizeof(char **);  break;	// string to dm
				//case 'T': *(char **)p = strtmp(tok);   size=sizeof(char **);  break;	// string to Tmpstr

			  case 'C': 				     	// string to *p
				if (strlen( tok) >= size)	// if too long, issue message, truncate, continue
				{	errFlLn( "Overlong string will be truncated to %d characters (from %d):\n    '%s'.",
							 (int)size-1, (int)strlen(tok), tok );
					tok[ size-1] = '\0';		// truncate;
				}
				strcpy( (char *)p, tok);		// cannot overrun
				break;

			  case 'D':
			  {
				short mon, day; 			// month-day date
				if (dStr2MonDay( tok, &mon, &day) != RCOK)		// decode date, datfcns.cpp
					rc = errFlLn("Invalid date \"%s\".", tok);
				else
					*(int *)p = dMonDay2Jday( mon, day, isLeap);   	// convert month & day to Julian date & return it
				break;
			  }

			  case 'I':
			  case 'L':
			  case 'F':			// numbers
			  {	// Note: experimental re-code using strDecode, annual EPW run 5% slower; 10-1-14
#if 1	// 10-1-14, omit check
				char *pNext;
#else
x				this check rejects ".0", better to just let strtod catch errors
x				// initial syntax check
x				char *pNext = tok;				// pointer to input text. Note already deblanked by token read.
x				if (strchr( "+-", *pNext))
x					pNext++;					// if sign present, pass it
x				if (!isdigitW(*pNext))			// if no digit, it is not a valid number
x					rc= errFlLn( (char *)MH_I0114, tok);			// cnd'l msg. "Missing or invalid number \"%s\".".
x				else							// has digit
#endif
				{	// decode
					if (cc=='F')				// F: 32-bit floating point
					{	*(float *)p = (float)strtod( tok, &pNext);	// decode as double, return next char ptr. C library.
						size = sizeof(float);
					}
					else					// I or L: 16 or 32 bit integer
					{	LI v = strtol( tok, &pNext, 0);		// convert to long. radix 0 --> respond to 0, 0x.
						if (cc=='I')
						{	if (v < -32768L || v > 32767L)
								rc = errFlLn( (char *)MH_I0115, tok);	// cnd'l msg including "Integer too large: \"%s\"."
							*(short *)p = (short)v;
							size = sizeof(short);
						}
						else						// 'L'
						{	*(long *)p = v;
							size = sizeof(long);
						}
					}
					// final syntax check: check for all input used
					if (*pNext)
						rc = errFlLn( (char *)MH_I0116, 		// "Unrecognized character(s) after %snumber \"%s\"."
									  cc=='F' ? "" : "integer ",
									  tok );
				}
				break;
			  }
			}
			p = (char *)p + size; 		// point past data in output
		}
		while (--count > 0);		// repeat til done "count" times

	}  // while (*cstr)

// check for NULL at end arg list: insures that control string and arg list match.
	void* pn = va_arg( pp, void *);
	if (pn)
		rc = errFlLn("Internal error: Invalid call to YACAM::getLineCSV():"
			         "\n    NULL not found at end arg list.");

 	return rc;
}		// YACAM::getLineCSV
//---------------------
#pragma optimize( "", on)
//-----------------------------------------------------------------------------
RC YACAM::checkExpected(		// check match to expected item
	const char* found,
	const char* expected)
{	
	RC rc = RCOK;
	if (!strMatch( found, expected))
		rc = errFlLn( "Incorrect item '%s', expected '%s'",
				found, expected);
	return rc;
}		// YACAM::checkExpected
//-----------------------------------------------------------------------------
RC YACAM::errFl( const char *s, ...)		// error message "%s <mWhat> <mPathName>". returns RCBAD.
{
	va_list ap;
	va_start( ap, s);
	char buf[300];
	msgI( WRN, buf, sizeof( buf), NULL, s, ap);	// retrieve error text if s is handle, format args into it to form text to insert in our msg
	return err( mErOp, 			// conditionally issue message (may prefix "Error: "), rmkerr.cpp or errdmy.cpp.
				"%s %s %s",
				buf,  mWhat,  mPathName ? mPathName : "bug") ;
}
//----------------------------------------------------------------------------
RC YACAM::errFlLn( const char *s, ...)	// error message "%s in <mWhat> <mPathName> near line <mLineMo>". returns RCBAD.
{
	va_list ap;
	va_start( ap, s);
	char buf[300];
	msgI( WRN, buf, sizeof( buf), NULL, s, ap);	// retrieve disk text if s is handle
												// format args into it to form text to insert in our msg
	return err( 						// conditionally issue message, rmkerr.cpp or errdmy.cpp.
			   mErOp,					// ERR, WRN, ...
			   (char *)MH_I0118,		// "%s '%s' (near line %d):\n  %s"
			   mWhat,  mPathName ? mPathName : "bug",  mLineNo,  buf );
}
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CSVGen / CSVItem: helpers for generating CSV files
//                   supports column headings and some unit conversion
///////////////////////////////////////////////////////////////////////////////
/*static*/ const double CSVItem::ci_UNSET = -99999999.;
//-----------------------------------------------------------------------------
const char* CSVItem::ci_Hdg(int iUx)
{
	return strtprintf(ci_iUn != UNNONE ? "%s(%s)" : "%s",
		ci_hdg, UNIT::GetSymbol(ci_iUn, iUx));
}		// CSVItem::ci_Hdg
//-----------------------------------------------------------------------------
const char* CSVItem::ci_Value()
{
	return ci_v != ci_UNSET
		? cvin2s(&ci_v, DTDBL, ci_iUn, 20, FMTSQ | FMTOVFE | FMTRTZ + ci_dfw)
		: "0";
}		// CSVItem::ci_Value
//-----------------------------------------------------------------------------
WStr CSVGen::cg_Hdgs(int iUx)
{
	WStr s;
	for (int i = 0; cg_pCI[i].ci_hdg; i++)
	{
		s += cg_pCI[i].ci_Hdg(iUx);
		s += ",";
	}
	return s;
}	// CSVGen::cg_Hdgs
//-----------------------------------------------------------------------------
WStr CSVGen::cg_Values(int iUx)
{
	int UnsysextSave = Unsysext;
	Unsysext = iUx;
	WStr s;
	for (int i = 0; cg_pCI[i].ci_hdg; i++)
	{
		s += cg_pCI[i].ci_Value();
		s += ",";
	}
	Unsysext = UnsysextSave;
	return s;
}	// CSVGen::cg_Values
//=============================================================================

// end of yacam.cpp
