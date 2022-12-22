// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// yacam.h  header for yacam.cpp  Yet Another Character Access Method
//          plus related CSV functions

///////////////////////////////////////////////////////////////////////////////
// class YACAM: efficient read/write of delimited text files
///////////////////////////////////////////////////////////////////////////////
class YACAM
{ public:
    char* mPathName;	// pathName. We copy to heap.
    char mWhat[20];		// descriptive phrase for error messages, eg "weather file"
    FILE* mFh;			// file handle, or -1 if not open
    int mWrAccess;		// non-0 if file open for writing
    int dirty;			// non-0 if buffer in object needs to be written to file
    long mFilSz;		// -1L or file size, for future detection of writes past end
    long mFilO;			// file offset of start of current buffer contents
    int bufN;			// number of bytes in buffer in object
    // for character/token access:
    int bufI; 			// buffer subscript of next character to return
    int mLineNo;		// line number in file
    char yc_buf[ 32768];

	YACAM();
    ~YACAM();
    void init();

    // "error actions" (erOp) in the following calls.
    //   IGN:  ignore: no message
    //   REG:  issue message and continue
    //   WRN:  issue message and await keypress

    // open exiting/create new file; close file; return RCOK if ok
    RC open( const char * pathName, const char *what="file", int erOp=WRN, int wrAcces=FALSE);
    RC close( int erOp=WRN);
	RC rewind( int erOp=WRN);
    RC clrBufIf();				// internal function to write buffer contents if dirty

    const char* pathName() { return mPathName ? mPathName : "bug"; }	// access pathName
    FILE* fh() { return mFh; }						// access file handle (-1 if not open)
    int wrAccess() { return mWrAccess; }			// return non-0 if open to write

    // random read to caller's buffer. Ret -1 if error, 0 if eof, else # bytes read (caller must check for >= count).
    int read( char *buf, int count,  long filO=-1L, int erOp=WRN );		//  YAC_EOFOK for no message at eof or short read 10-26-94

    // random access using buffer in object -- use for short, likely-to-be sequential i/o.
    char* getBytes( long filO, int count, int erOp=WRN); 	// returns NULL or pointer to data in buffer
    RC putBytes( const char* data, int count, long filO, int erOp=WRN); 	// returns RCOK if ok

    // string sequential write
    RC printf( const char *fmt, ...);		// erAct=WRN implict
    RC printf( int erOp, const char *fmt, ...);
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
             int erOp=WRN, const char** ppBuf=NULL);	//  YAC_EOFOK for no message at clean EOF
	RC tokeCSV( char* tok, unsigned tokSz,				// return next comma delimited token. decomments, dequotes.
             int erOp=WRN, const char** ppBuf=NULL);	//  YAC_EOFOK for no message at clean EOF
    RC get( int erOp, int isLeap,    			// read, decode, and store data per control string
		const char* cstr, void* p, ... );
    RC getLineCSV( int erOp, int isLeap,     	// read, decode, and store data from CSV line per control string
        const char* cstr, void* p, ... );

    /* "control string" argument characters for YACAM::get: I short integer; L long integer; F float;
	D date: month and day, no year, leap year flag from caller;
    C (quoted) string, stored *p (thus need , next in cstr if not end);
	  , start using next storage pointer in call */

    // error message functions. If called externally, erOp from last YACAM member function call is used.
	RC checkExpected( const char* found, const char* expected);
    RC errFl( const char* s, ...);	// conditional error message "Error: %s <mWhat> <mPathName>". returns RCBAD.
    RC errFlLn( const char *s, ...);  // cond'l message "Error in <mWhat> <mPathName> near line <mLineNo>: %s". rets RCBAD.

    int mErOp;	// communicates erOp from entry points to error fcns
				// note need a data mbr at end due to rcdef.exe deficiency 10-94.
#if 0 // The methods below are not used but have been modified to fit c standard file io. SHA e6181ff06f42350c8c559211c3809e25a03d89f2
    RC create(const char* pathName, const char* what = "file", int erOp = WRN);
    // random write from caller's buffer, RCOK if ok
    RC write(char* buf, int count, long filO = -1L, int erOp = WRN);
#endif
};			// class YACAM
//----------------------------------------------------------------------------
//-- option bits used with YACAM functions. EROPn defined in cnglob.h or notcne.h.
const int YAC_EOFOK = EROP6;	// no error message (RCBAD2 returned) at clean eof in toke(), get() or any eof in read().
const int YAC_NOREAD = EROP7;	// getLineCSV: don't call line()

const size_t YACAM_MAXLINELEN = 2000;		// max line length accomodated in some fcns
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CSVGen / CSVItem: helpers for generating CSV files
//                   supports column headings and some unit conversion
///////////////////////////////////////////////////////////////////////////////
struct CSVItem		// one item (column)
{
	const char* ci_hdg;		// heading text
	double ci_v;			// value
	int ci_iUn;	// units	// units
	int ci_dfw;	// sig figs (re FMTSQ)
	// int ci_iUx -- fixed unit system for this item

	static const double ci_UNSET;	// flag for unset value (writes 0)

	const char* ci_Hdg(int iUx);
	const char* ci_Value();
}; // struct CSVItem
//-----------------------------------------------------------------------------
class CSVGen
{
	CSVItem* cg_pCI;	// array of CSVItems
	int cg_iUx;			// unit system

public:
	CSVGen(CSVItem* pCI) : cg_pCI(pCI) {}
	WStr cg_Hdgs(int iUx);
	WStr cg_Values(int iUx);
};	// class CSVGen

//----------------------------------------------------------------------------

// end of yacam.h
