// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// yacam.h  header for yacam.cpp  Yet Another Character Access Method

/* This file access code uses no other code but rmkerr.cpp or errdmy.cpp
   and C library; It is intended to be portable, eg for weather file utilities
   and LBL weather file reader; It provides an adequate subset of xiopak/
   xiochar/xiogetvl functionality without linking the rest of the BSG world. */

//============================================================================
//  YACAM class declaration
//----------------------------------------------------------------------------
class YACAM
{ public:
    char* mPathName;	// pathName. We copy to heap.
    char mWhat[20];		// descriptive phrase for error messages, eg "weather file"
    int mFh;			// file handle, or -1 if not open
    int mWrAccess;		// non-0 if file open for writing
    int dirty;			// non-0 if buffer in object needs to be written to file
    long mFilSz;		// -1L or file size, for future detection of writes past end
    long mFilO;			// file offset of start of current buffer contents
    int bufN;			// number of bytes in buffer in object
    // for character/token access:
    int bufI; 			// buffer subscript of next character to return
    int mLineNo;		// line number in file
    char buf[ 32768];

	YACAM();
    ~YACAM();
    void init();

    // "error actions" (erOp) in the following calls.
    //   IGN:  ignore: no message
    //   REG:  issue message and continue
    //   WRN:  issue message and await keypress

    // open exiting/create new file; close file; return RCOK if ok
    RC open( const char * pathName, const char *what="file", int erOp=WRN, int wrAcces=FALSE);
    RC create( const char * pathName, const char *what="file", int erOp=WRN);
    RC close( int erOp=WRN);
	RC rewind( int erOp=WRN);
    RC clrBufIf();				// internal function to write buffer contents if dirty

    const char* pathName() { return mPathName ? mPathName : "bug"; }	// access pathName
    int fh() { return mFh; }						// access file handle (-1 if not open)
    int wrAccess() { return mWrAccess; }			// return non-0 if open to write

    // random read to caller's buffer. Ret -1 if error, 0 if eof, else # bytes read (caller must check for >= count).
    int read( char *buf, int count,  long filO=-1L, int erOp=WRN );		//  YAC_EOFOK for no message at eof or short read 10-26-94
    // random write from caller's buffer, RCOK if ok
    RC write( char *buf, int count, long filO=-1L, int erOp=WRN );

    // random access using buffer in object -- use for short, likely-to-be sequential i/o.
    char* getBytes( long filO, int count, int erOp=WRN); 	// returns NULL or pointer to data in buffer
    RC putBytes( const char* data, int count, long filO, int erOp=WRN); 	// returns RCOK if ok

    // string sequential write
    RC cdecl printf( const char *fmt, ...);		// erAct=WRN implict
    RC cdecl printf( int erOp, const char *fmt, ...);
    RC vprintf( int erOp, const char *fmt, va_list ap);

    // character/token read. Don't intermix with random access functions above.
    int lineNo() { return mLineNo; }	// get file line number, only valid if no random access has been used.
    char peekC( int erOp=WRN);			// return next character or EOF and leave in buffer
    char getC( int erOp=WRN);			// return next character or EOF
#if 0
0	int line( CWString& sLine, int erOp=WRN);
#endif
	int line( char* line, int lineSz, int erOp=WRN);
	int scanLine( const char* s, int erOp=WRN, int maxLines=-1);

    RC toke( char* tok, unsigned tokSz,		// return next whitespace/comma delimited token. decomments, dequotes.
             int erOp=WRN, const char* ln=NULL);	//  YAC_EOFOK for no message at clean EOF
    RC cdecl get( int erOp, int isLeap,    			// read, decode, and store data per control string
		const char* cstr, void* p, ... );
    RC cdecl getLineCSV( int erOp, int isLeap,     	// read, decode, and store data from CSV line per control string
        const char* cstr, void* p, ... );

    /* "control string" argument characters for YACAM::get: I short integer; L long integer; F float;
	D date: month and day, no year, leap year flag from caller;
        C (quoted) string, stored *p (thus need , next in cstr if not end);
	, start using next storage pointer in call */

    // error message functions. If called externally, erAct from last YACAM member function call is used.
    RC cdecl errFl( const char* s, ...);	// conditional error message "Error: %s <mWhat> <mPathName>". returns RCBAD.
    RC cdecl errFlLn( const char *s, ...);  // cond'l message "Error in <mWhat> <mPathName> near line <mLineNo>: %s". rets RCBAD.

    int mErOp;	// communicates erOp from entry points to error fcns
				// note need a data mbr at end due to rcdef.exe deficiency 10-94.
};			// YACAM

//----------------------------------------------------------------------------
//-- option bits used with YACAM functions. EROPn defined in cnglob.h or notcne.h.
const int YAC_EOFOK = EROP6;	// no error message (RCBAD2 returned) at clean eof in toke(), get() or any eof in read().
const int YAC_NOREAD = EROP7;	// getLineCSV: don't call line()

const size_t YACAM_MAXLINELEN = 2000;		// max line length accomodated in some fcns

				// may also be used with wthr\wufcns.cpp fcns.
//----------------------------------------------------------------------------

// end of yacam.h
