// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// rmkerr.h -- decls for CSE remark/error reporting fcns in rmkerr.cpp

#if !defined( _RMKERR_H)
#define _RMKERR_H

#include "VECPAK.H"
/*-------------------------------- DEFINES --------------------------------*/
// see cnglob.h for MANY rmkerr-related defines
//     including IGN, WRN, ABT, NONL, DASHES, NOSCRN ...

/*--------------------------- PUBLIC VARIABLES ----------------------------*/

// virtual report handles (see vrpak.cpp) for reports generated in rmkerr.cpp
extern int VrErr;		// set/used/cleared in rmkerr.cpp and cse.cpp.
extern int VrLog;		// ..

/*------------------------- FUNCTION DECLARATIONS -------------------------*/

void FC errClean();
#ifdef WINorDLL
  #ifdef __WINDOWS_H			// defined in windows.h (Borland 3.1)
    RC screenOpen( HINSTANCE hInst, HWND hPar, const char *caption);
  #else	/* windows.h not included. Declare as types would be declared in Windows 3.1 windows.h with "STRICT"..
           so can compile without windows.h. Undefined structs after the word "struct" are ok in unused declarations. */
      RC screenOpen( struct HINSTANCE__ *  hInst, struct HWND__ * hPar, const char *caption);
  #endif
  RC FC screenClose();
#endif
int FC getScreenCol();
RC FC errFileOpen( const char* erfName);
SI FC openErrVr( void);
SI FC openLogVr( void);
void FC closeErrVr( void);
void FC closeLogVr( void);
void FC setBatchMode( BOO _batchMode);
SI FC getBatchMode();
void FC setWarnNoScrn( BOO _warnNoScrn);
SI FC getWarnNoScrn();
void FC clearErrCount();
void FC incrErrCount();
int FC errCount();

void ourAssertFail( char * condition, char * file, int line);
RC CDEC warnCrit( int erOp, const char* msg, ...);
RC CDEC errCrit( int erOp, const char* msg, ...);
RC CDEC issueMsg(int isWarn, const char* mOrH, ...);
RC CDEC info( const char* mOrH, ...);
RC CDEC warn( const char* mOrH, ...);
RC CDEC err( const char* mOrH, ...);
RC CDEC err( int erOp, const char* mOrH, ...);
RC errV( int erOp, int isWarn, const char* mOrH, va_list ap);
RC errI( int erOp, int isWarn, const char* text);
const char* GetSystemMsg( DWORD lastErr=0xffffffff);
RC logit( int op, const char* mOrH, ...);
void logitNF( const char* text, int op=0);
RC screen( int op, const char* mOrH, ...);
void screenNF(const char* text, int op = 0);
int setScreenQuiet( int sq);
BOO mbIErr( const char* fcn, const char* fmt, ...);
#ifdef WINorDLL
  BOO erBox( const char *text, unsigned int /*UINT*/ style, BOO conAb);
#endif
void FC yielder();

//===================================================================
// debug printing
//===================================================================
#if 0
0 BOOL DbReset( int dblo, int erOp);
0 void DbPrintf( DWORD oMsk, const char* fmt, ...);
0 BOOL DbObjHdg( const char* hdgFmt, BOOL bEmpty, const char* fmt=NULL, ...);
0 BOOL DbObjHdg( DWORD oMsk, const char* hdgFmt, BOOL bEmpty, const char* fmt=NULL, ...);
0 BOOL DbMbIErr( int erOp, const char* fcn, DWORD oMsk, const char* fmt, ...);
0 void DbColNumHdg( DWORD oMsk, const char* heading, const char* fmt="%6d",
0 	int nCount=24, int n0=1, int nIncr=1);
0 inline void DbColNumHdg( const char* heading, const char* fmt="%6d",
0 	int nCount=24, int n0=1, int nIncr=1)
0 {	DbColNumHdg( dbdALWAYS, heading, fmt, nCount, n0, nIncr); }
0 void DbStrings( const char* hdg, const CWString* pS, int nS);
0 BOOL DbAppending();
0 void DbSetComment(const char* cmt);
0 CWString DbGetComment();
0 void calcTimeDbPrintf( const char* name, clock_t& beg, DWORD oMsk=0);
0 struct UIFMT
0 {	const char* hd;		//	heading text
0 	int	ui;				//	unit index
0 	const char* fmt;	//	printf format string - * = use default for current unit
0 };
0 int DbDoCalcLog( CWnd* pParent);
#endif

// debug logging options
const DWORD dbdALWAYS = 1;				// print always (even if dbgMask = 0)
// 2, 4, 8 reserved
// hi 28 bits for application
const DWORD dbdANY = 0xfffffff0;		// print if any dbgMask bit enabled
const DWORD dbdSTARTRUN =  0xfffffff1;	// special run-start value (see tp_DbHdgsIf())
const DWORD dbdSTARTSTEP = 0xfffffff2;	// special timestep start value (ditto)
const DWORD dbdoptNOHDGS = 0x1;			// DbDo options: don't write headings
BOOL DbShouldPrint( DWORD oMsk);
DWORD DbSetMask( DWORD newMsk);
RC DbFileOpen( const char *_dbFName);
int DbGetVrh();
void DbPrintf( DWORD oMsk, const char* fmt, ...);
void DbPrintf( const char* fmt, ...);
void DbVprintf( const char* fmt, va_list ap=NULL);
//--------------------------------------------------------------------------
template< typename T> void VDbPrintf( 		// debug print vector
	DWORD oMsk,			// mask: print iff corres bit(s) on in dbgMsk
						//   0xffffffff = always print
	const char* heading, // heading, NULL for none
						 //   MUST include \n etc as required
						 //   w/o final \n = row label
	T* v,				// data vector
	int n,				// dim of v
	const char* fmt)	// fmt for each item in v
{
	if (DbShouldPrint( oMsk))
	{	char s[ 2000];		// assumed big enuf
		int l = 0;
		if (heading)
		{	strcpy( s, heading);
			l = strlen( s);
		}
		for (int i=0; i<n; i++)
			l += sprintf( s+l, fmt, v[ i]);
		sprintf( s+l, "\n");
		DbVprintf( s);
	}
}				// VDbPrintf
//-----------------------------------------------------------------------------
template< typename T> void VDbPrintf( 		// debug print vector
	const char* heading, T* v, int n, const char* fmt)
{	VDbPrintf( dbdALWAYS, heading, v, n, fmt);
}				// VDbPrintf
//-----------------------------------------------------------------------------
template< typename T> void VDbPrintf( 		// debug print vector (scaled)
	DWORD oMsk, const char* heading, T* v, int n, const char* fmt, double f)
{
	if (DbShouldPrint( oMsk))
	{	T vX[ 1000];
		VCopy( vX, n, v, f);
		VDbPrintf( oMsk, heading, vX, n, fmt);
	}
}			// VDbPrintf
//--------------------------------------------------------------------------
template< typename T> void VDbPrintf( 		// debug print vector (scaled)
	const char* heading, T* v, int n, const char* fmt, double f)
{	VDbPrintf( dbdALWAYS, heading, v, n, fmt, f);
}			// VDbPrintf
//--------------------------------------------------------------------------
template< typename T> void MDbPrintf( 		// debug print a matrix
	DWORD oMsk,			// mask: print iff corres bit(s) on in dbgMsk
						//   0xffffffff = always print
	const char* heading, // heading, NULL for none
						//   MUST include \n etc as required
						//   w/o final \n = row label
	T* v,				// data matrix
	int dimRow,			// dim of each row
	int nCol,			// # to print in each row
	int nRow,			// # of rows
	const char* fmt)	// fmt for each item in v
{
	if (heading)
		DbPrintf( oMsk, heading);
	// loop rows
	for (int iRow=0; iRow<nRow; iRow++)
	{	T* pRow = v + dimRow*iRow;
		VDbPrintf( oMsk, NULL, pRow, nCol, fmt);
	}
}			// MDbPrintf
// debugging aid: warn if limits invoked
template< typename T> inline T bracketWarn( T vMin, T v, T vMax)
{	T vx = bracket( vMin, v, vMax);
	if (vx != v)
		warn( "bracketWarn: hit limit\n");
	return vx;
}
//============================================================================

#endif // _RMKERR_H

// rmkerr.h end

