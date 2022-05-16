// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// rmkerr.cpp -- CSE screen remark and error reporting fcns
//               and debug printing fcns

#include "CNGLOB.H"

/*------------------------------- OPTIONS ---------------------------------*/
#ifndef NOVRR	// Not defined for regular use in cse.exe
				//  def'd on compiler cmd line for non-VRR

#define VRR		// define for virtual report message logging using CSE functions
				// undefine for portable linkability (or put vr world in lib)
				//   including for use in rcdef.exe. 11-91.

#endif	//NOVRR

// WIN,DLL: defined on command line for Windows and DLL versions respectively. 10-93.
// OUTPNAMS: cond'ly def'd in cnglob.h to call "saveAnOutPNam" when error file is closed. 10-93.

/*------------------------------- INCLUDES --------------------------------*/
#include "CSE.h"		// globals required for some configurations
#include <conio.h>		// getch

#include <MESSAGES.H>	// msgI
#include <ENVPAK.H>		// byebye

#include "CNCULT.H"

#if defined( VRR)
#include <VRPAK.H>
#endif

#if defined( LOGWIN)
#ifdef DLL
#include <cnedll.h>		// hInstLib
#endif
#include <logwin.h>		// LogWin class
#endif		// LOGWIN

#include <RMKERR.H>		// decls for this file

#ifndef _WIN32
#define NO_ERROR 0
#endif


/*-------------------------------- DEFINES --------------------------------*/

#define DASHLINE "---------------\n"	// separates error messages, and optionally log messages


/*---------------------------- LOCAL VARIABLES ----------------------------*/

#ifdef VRR
// virtual report handles: set and returned by openErrVr etc below; 0'd by closeErrVr etc;
// used internally by fcns that output to the respective reports;
// may also be directly accessed, eg from cse.cpp for end-session unspool. */
int VrErr = 0;		// 0 or virtual report handle for open error messages virtual report
int VrLog = 0;		// 0 or virtual report handle for open run log virtual report
#endif

LOCAL FILE * NEAR errFile = NULL;	/* File to receive error and warning messages (or NULL).
					   Opened/closed by errFileOpen; written by errI. */
#if defined( LOGWIN)
static LogWin screenLogWin;		// scrolling, sizeable window for "console" display under windows
#endif

enum LINESTAT
{	midLine=0,			// in mid-line or unknown
	begLine,			// at start of a line
	dashed			// at start of a line and preceding line is known to be ---------------.
};

LOCAL LINESTAT NEAR errFileLs= midLine;	// whether error file cursor is at midline, start line, or after ----------'s line
#ifdef VRR
LOCAL LINESTAT NEAR vrErrLs = midLine;	// .. errors virtual report ..
LOCAL LINESTAT NEAR vrLogLs = midLine;	// .. log .. .. ..
#endif
LOCAL LINESTAT NEAR screenLs = midLine;	// .. screen (insofar as known here): set in errI, log, screen, presskey, at least.
static int screenCol = 0;		// approximate screen column (# chars since \n).
// first screen msg should have no NONL option, to begin with newline to init screenLs and screenCol in dos version.

LOCAL BOO NEAR batchMode = 0;		// non-0 to not await input at any error if error file open

LOCAL BOO NEAR warnNoScrn = 0;		// non-0 to not display warnings on screen, rob 5-97

static int errorCounter = 0;		// error count, errors NOT counted if erOp==IGN or if isWarn.

static int screenQuiet = 0;			// suppress screen messages if op&QUIETIF && screenQuiet
									//    see setScreenQuiet() and screen()

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC errCritV( int erOp, int isWarn, const char* msTx, va_list ap);
LOCAL int presskey( int erAction);

// ================================================================================
void FC errClean()   	// cleanup function: DLL version should release ALL memory. rob 12-93.
{
	errFileOpen( NULL);		// close error file if open (redundant; insurance here)

	DbFileOpen( NULL);		// close debug log file if open (ditto)

	msgClean();			// clean up messages.cpp: close message file, free memory

	errorCounter = 0;		// rob 5-97
}			// errClean
#if defined( LOGWIN)
// ================================================================================
RC screenOpen( 	// open window to receive "console" output (screen() function) under Windows

	HINSTANCE hInst, HWND hPar, 	// appl instance and parent window
	const char *caption )   		// title bar text
{
	/* CAUTION 10-95: screenOpen() is now called from cse.cpp b4 longjmp byebye() error exit set up
	   --> probably should not call CSE error functions, cuz an ABT would exit to op sys without cleanup.
	   If must fix, put longjump setup outside multiple input files loop in cse.cpp. */

	screenLs = begLine;
	screenCol = 0;		// in Windows version, know at beginning of line.

// get info for positioning window
	RECT sr, pr, pncr;				// screen, parent (0-based client), parent non-client (overall) rectangles
	GetWindowRect( GetDesktopWindow(), &sr);	// screen rectangle
	HDC hdct = GetDC(hPar);
	POINT po;
	GetDCOrgEx( hdct, &po);			// get parent client area origin in screen coordinates
	SI px = (SI)po.x, py = (SI)po.y;
	ReleaseDC( hPar, hdct);
	GetClientRect( hPar, &pr);			// parent client rectangle, inside frame & menu bar, 0-based
	GetWindowRect( hPar, &pncr);		// parent overall (non-client) rectangle, in screen coordinates

	SI sw = sr.right, sh = sr.bottom;		// screen width and height. origin is 0,0.
	SI pw = pr.right, ph = pr.bottom;		// fetch parent client width and height

// window size
	SI w = 3*sw/4, h = sh/3;			// window size: 3/4 screen width, 1/3 screen hite
	if (w > 820)  w = 820;			// limit width to 82 10-pixel chars (my chars are 9-pixel 10-8-93)

// window position
	SI x, y;					// our window left and top screen coordinates
	if (!hPar || !hdct)				// insurance: if no hPar given or GetDC failed,
	{
		x = sw/2 - w/2;
		y = sh/5 - h/2;    	// use x: centered: y: 1/5 way down (above msg boxes)
	}
	else					// normally
	{
		x = px + pw/2 - w/2;			// window hor position: center over parent client,
		setToMin( x, sw - w);
		setToMax( x, 0);		// but not hanging over edges of screen.

		if (py + h <= sh)			// vert position: if window will fit below parent's menu bar
		{
			y = py + ph/2 - h/2;			// center y over parent client area, then
			setToMin( y, sh/5 - h/2);			// attempt to push up to centered 1/5 way down screen to not block msg boxes,
			setToMax( y, py);			// then push down if necessary to not block menu bar
			// above intended to yield y = sh/5 - h/2 for maximized parent window.
			if (y + h/2 > sh/2)			// if result is centered below center of screen
				setToMax( y, sh*3/4 - h/2);		// push to centered 3/4 of way down screen to be mostly below msg boxes
		}
		else 					// won't fit below parent menu
			if (h < pncr.top)   			// if window will fit above whole parent window
			{
				y = pncr.top - h;			// put window just above parent, then
				setToMin( y, sh/5 - h/2);			// push up to be above message boxes (Windows auto-centers them)
			}
			else					// won't fit either above or below parent menu bar
			{
				y = sh - h;				// put at bottom of screen
				if (px + pw/2 <= sw/2)		// if parent is centered at or left of screen center
				{
					if (px < sw - w)			// if some of parent would be exposed at left,
						setToMax( x, px + pw);		// move our window right (limited to edge screen below)
				}
				else if (px + pw > w)			// parent right of center. if some of parent would show at right,
					setToMin( x, px - w);			// move our window to left (limited to screen edge below). signed arithmetic!
			}
		setToMin( y, sh - h);
		setToMax( y, 0);		// be sure does not hang over edge of screen
		setToMin( x, sw - w);
		setToMax( x, 0);		// ..
	}

// open window
	if ( !screenLogWin.open( 		// logwin.cpp.
#ifdef DLL
				hInstLib,	// library module instance handle (LibMain arg, cnedll.cpp variable).
							// use lib's instance to prevent conflict with caller's LogWins
							// due to different copies of LogWin.cpp registering window class twice.
							// (Otherwise, would need to unregister class when last LogWin window closed.)
#else
				hInst, 		// application module instance handle (WinMain arg, cse.cpp variable)
#endif
				hPar, caption,
				x, y, w, h,     	// x y wid hite
				LW_AUTOEND	    	// autoscroll window to show end of text as added
				| LW_FORCEACTIVATE ) )	// makes sure window looks activated during sumulation
		return RCBAD;
	return RCOK;
}		// screenOpen
// ================================================================================
RC FC screenClose()	// close window for "console" output under Windows
{
	return !screenLogWin.close();
}				// screenClose
#endif // LOGWIN
// ================================================================================
int FC getScreenCol()	// return approx 0-based screen column
{
	return screenCol;
}			// getScreenCol

// ================================================================================
RC FC errFileOpen(

// close/open file to receive error messages.  (Messages are output via errI and its several interfaces, below.)

// closes any already-open error file, and deletes it if empty, else, if OUTPNAMS, calls saveAnOutPNam.

	const char* _erfName )	// pathname incl extension for error message file, or NULL to close open error message only.

// returns RCOK if all OK,
//         RCBAD if file _erfName cannot be opened
{
	static char erfName[ _MAX_PATH] = { 0 };
	RC rc=RCOK;

// close existing error message file, if any.  recursion CAUTION: called from errI.
	if (errFile != NULL)
	{
		LI len = -2L;
		if (fseek( errFile, 0L, SEEK_END)==0)	// position pointer to end, 0 if ok
			len = ftell( errFile);		// read file pointer, -1L if bad
		fclose( errFile);
		if (!IsBlank( erfName))				// if have saved name -- insurance
		{
			if (len==0L)				// if file is empty (no errors)
				unlink(erfName);			// erase file -- don't garbage up directory
#ifdef OUTPNAMS	//may be defined in cnglob.h. 10-93.
			else					// file not erased,
				saveAnOutPNam(erfName);		// save name of file for return to caller. cse.cpp, decl cnglob.h.
#endif
		}
		errFile = NULL;				// say have no open error message file
		erfName[ 0] = '\0';
	}

// open file with specified name if any given
	if (_erfName != NULL)			// NULL just closes
	{
		errFile = fopen( _erfName, "wt");   	// open empty text file for writing
		if (errFile==NULL)					// if open failed
			rc = errCrit( PWRN, "X0010: Cannot open error message file '%s'", _erfName);	// set bad return, issue msg

		strncpy0( erfName, _erfName, sizeof( erfName));	// internal copy of name
														// for deletion if file empty at close
		errFileLs = begLine;		// set status for callers: at beginning of a line
	}
	return rc;
}		// errFileOpen
#ifdef VRR
// ================================================================================
SI FC openErrVr()		// open error virtual report if not open, return handle (vrh)

/* call as soon as possible after startup and after each time closed
   (eg for unspooling) to capture as many messages as possible in Err report. */
// note: all messages go to error FILE, if open, even when error vr is closed.

// note: title for report is output first time text is added (in this file), if any is added
/* note: unspool requests for this report: cncult up UnspoolInfo for cse to use after each run;
         cse also unspools at end of session. */
{
	if (!VrErr)						// if none open, open one
	{
		vrOpen( &VrErr, "Err", VR_FMT | VR_FINEWL);  	// vrpak.cpp function; VrErr is above.
		vrErrLs = begLine;				// say new report is at start of a line
	}
	return VrErr;				// handle used by cncult.cpp even if already open
}			// openErrVr
#endif
#ifdef VRR
// ================================================================================
SI FC openLogVr()		// open log virtual report if not open, return handle (vrh)

// note: title for report is output first time text is added (in this file), if any is added
/* note: unspool requests for this report: cncult up UnspoolInfo for cse to use after each run;
         cse also unspools at end of session. */
{
	if (!VrLog)						// if none open, open one
	{
		vrOpen( &VrLog, "Log", VR_FMT | VR_FINEWL);  	// vrpak.cpp function; VrLog is above.
		vrLogLs = begLine;				// say new report is at start of a line
	}
	return VrLog;
}			// openLogVr
#endif
#ifdef VRR
// ================================================================================
void FC closeErrVr()		// close error virtual report if open

// stops output of error messages to error virtual report and finishes off report

/* must be called before unspooling as spooling during unspool is not handled
   yet unspool code may generate error messages */
{
	if (VrErr)				// if open
	{
		VrErr = 0;			// forget handle -- terminates output by other fcns in this file.
		// note caller cncult.cpp put handle in unspool info at open time.
		// close call to vrpak is not necessary.
	}
}		// closeErrVr
#endif
#ifdef VRR
// ================================================================================
void FC closeLogVr()		// close log virtual report if open

// stops output of messages to log virtual report and finishes off report

// should be called before unspooling as spooling during unspool is not handled.
{
	if (VrLog)				// if open
	{
		VrLog = 0;			// forget handle -- terminates output by other fcns in this file.
		// note caller cncult.cpp put handle in unspool info at open time.
		// close call to vrpak is not necessary.
	}
}		// closeLogVr
#endif
// ================================================================================
void FC setBatchMode( BOO _batchMode)		// batch mode on/off (-b on CSE command line)

// in batch mode messages never wait for a keypress, provided the error message file is open.  4-7-92.
{
	batchMode = _batchMode;
}				// setBatchMode
// ================================================================================
SI FC getBatchMode()		// get batch mode. 1 call in cse.cpp 10-93.
{
	return batchMode;				// non-0 if batch mode in effect
}				// getBatchMode
// ================================================================================
void FC setWarnNoScrn( BOO _warnNoScrn)		// don't put warnings on screen on/off (-n on CSE command line). Rob 5-97.
{
	warnNoScrn = _warnNoScrn;
}				// setWarnNoScrn
// ================================================================================
SI FC getWarnNoScrn()		// get no-screen-warnings. Used? Rob 5-97.
{
	return warnNoScrn;				// non-0 if don't-screen-warnings in effect
}				// getWarnNoScrn
// ================================================================================
void FC clearErrCount()		// clear error count. Rob 5-97.
{
	errorCounter = 0;
}			// clearErrCount
// ================================================================================
void FC incrErrCount()		// increment error count. Rob 5-97.
{
	errorCounter++;
}			// incrErrCount
// ================================================================================
int FC errCount()		// get error count. Rob 5-97.
{
	return errorCounter;
}			// errCount

// ================================================================================
void ourAssertFail( char * condition, char * file, int line)	// assertion failure fatal error message

/* for ASSERT macro (cnglob.h): same as compiler library "assert" debugging macro except
   exits our way so DDE is properly terminated, etc.
   (otherwise windows program dissappears from screen but does not actually exit,
   necessitating Windows restart to reRun. 8-95.) */
{
	err( PABT, "Assertion failed: %s, file %s, line %d", condition, file, line);
}

// ================================================================================
RC CDEC warnCrit(	// Report critical warning messagge: no dm use, no (possibly recursively troublesome) call to msg()

	int erOp,			// error action: IGN, (REG), WRN, ABT, PWRN, PABT, and NOPREF bit: cnglob.h.
	const char* msTx,	// printf-style msg/fmt (NOT message handle)
	... ) 				// args as required by msg

// returns RCBAD as convenience
{
	va_list ap;
	va_start( ap, msTx);				// point to arg list
	return errCritV( erOp, 1/*isWarn*/, msTx, ap);
}
// ================================================================================
RC CDEC errCrit(	// Report critical error: no dm use, no (possibly recursively troublesome) call to msg()

	int erOp,			// error action: IGN, (REG), WRN, ABT, PWRN, PABT, and NOPREF bit: cnglob.h.
	const char* msTx,	// printf-style msg/fmt (NOT message handle)
	... ) 		// args as required by msg
// avoid internal calls that could cause recursion!
// returns RCBAD as convenience
{
	va_list ap;
	va_start( ap, msTx);				// point to arg list
	return errCritV( erOp, 0/*isWarn*/, msTx, ap);
}
// ================================================================================
static const char* errMsgPrefix (	// return "Error", "Warning" etc.
	int erOp,
	int isWarn)
{
	isWarn &= 0xF;		// drop possible hi bits
	const char* pref =
		  erOp & NOPREF  ? ""					// no prefix
		: erOp & PROGERR ? "Program error: "	// "Program error" if PWRN or PABT (per cnglob.h defines).
		: isWarn == 2    ? "Info: "				// Information message
		: isWarn == 1    ? "Warning: "			// Warning message
		:                  "Error: ";			// usual case
	return pref;
}		// errMsgPrefix
// ================================================================================
LOCAL RC errCritV(		// common routine for warnCrit, errCrit
	int erOp,
	int isWarn,	// 0: error: increment error count returned by errCount(). Output to screen, file, vr.
				// 1: warning: no errCount++; don't display on screen if setWarnNoScrn(1) has been called.
				// (2: info: no errCount++.)
				// + hi bits NOSCRN, NOERRFILE, NOREPORT
	const char* msTx,
	va_list ap )
// returns RCBAD as convenience
{
	char buf[ MSG_MAXLEN+30];	// msg buffer
								//  +30 for "Program error:\n  " etc b4 message.
	strcpy( buf, errMsgPrefix( erOp, isWarn));

	char bufMsg[ MSG_MAXLEN];
	vsnprintf( bufMsg, sizeof( bufMsg)-1, msTx, ap);		// format caller's message
	if (*buf)				// except if prefix supprressed
		if (strJoinLen( buf, bufMsg) > getCpl())	// test length of joined msgs
			strcat( buf, "\n  ");		// line would be too long, so start caller's message on new line
	strcat( buf, bufMsg);		// concatenate formatted caller's msg with "Error: " & possible newline:
    						//  copy to lower subscripts in same buffer.
	return errI( erOp, isWarn, buf);	// and report it
}				// errCritV
// ================================================================================
RC CDEC issueMsg(			// general info/warn/error msg issuer
	int isWarn,			// 0: error
						// 1: warn
						// 2: info
						// + hi bits: NOSCRN, NOERRFILE, NOREPORT
	const char* mOrH,	// ptr to printf fmt message or message handle
	... )  				// args as reqd by mOrH

// difference from error message: error count not incremented; "Info: " not "Error: " prefixed.
// difference from warning message: displayed on screen even if setWarnNoScrn(1) called.

// returns caller's mOrH, or RCBAD if mOrH is char *
{
	va_list ap;
	va_start( ap, mOrH);
	return errV( REG/*erOp*/, isWarn, mOrH, ap);
}							// info
//--------------------------------------------------------------------------------
RC CDEC info(			// issue information message
	const char* mOrH,	// ptr to printf fmt message or message handle
	...)  				// args as reqd by mOrH

// difference from error message: error count not incremented; "Info: " not "Error: " prefixed.
// difference from warning message: displayed on screen even if setWarnNoScrn(1) called.

// returns caller's mOrH, or RCBAD if mOrH is char *
{
	va_list ap;
	va_start(ap, mOrH);
	return errV(REG/*erOp*/, 2/*isWarn*/, mOrH, ap);
}							// info
// ================================================================================
RC CDEC warn(				// issue warning message, rob 5-97
	const char* mOrH,	// ptr to printf fmt message or message handle
	... )  				// args as reqd by mOrH

// differences from error message:
//    error count not incremented;
//    text is prefixed with "Warning: " not "Error: ";
//	  if setWarnNoScrn(1) has been called, message not displayed on screen (still in file, vr).

// returns caller's mOrH, or RCBAD if mOrH is char *
{
	va_list ap;
	va_start( ap, mOrH);
	return errV( REG/*erOp*/, 1/*isWarn*/, mOrH, ap);
}							// warn
// ================================================================================
RC CDEC err(				// issue error message, variant without 'erOp' arg
	const char* mOrH,	// ptr to printf fmt message (e.g. "Bad value: %d") or message handle (e.g. (char *)MH_X1002)
	...)				// args as reqd by mOrH

// implicit error action "REG": output msg, return to caller. No keystroke, no termination of program.

// returns caller's mOrH, or RCBAD if mOrH is char *
{
	va_list ap;
	va_start( ap, mOrH);
	return errV( REG/*erOp*/, 0, mOrH, ap);		// use variable arg list version
}						// err
// ================================================================================
RC CDEC err(			// issue error message
	int erOp,				// error action: IGN, (REG), WRN, ABT, PWRN, PABT, NOPREF: cnglob.h, more comments in errI().
	const char* mOrH,		// ptr to printf fmt message (e.g. "Bad value: %d")
				// OR message handle (e.g. (char *)MH_X1002); message handles are
				//    defined in msghan.h and are associated with text in msgtab:msgTbl[]
	...)			// args as reqd by mOrH

// NOTE: requires MUCH stack space for message buffer(s)

// returns caller's mOrH, or RCBAD if mOrH is char *
{
	va_list ap;
	va_start( ap, mOrH);
	return errV( erOp, 0, mOrH, ap);		// use variable arg list version, next
}					// err
// ================================================================================
RC errV( 			// report error, variable arg list version

	int erOp,	// error action: IGN, (REG), WRN, ABT, PWRN, PABT, and NOPREF bit: cnglob.h.
	int isWarn,	// 0: error: increment error count returned by errCount(). Output to screen, file, vr.
				// 1: warning: no errCount++; don't display on screen if setWarnNoScrn(1) has been called.
				// 2: info: no errCount++.
				// + hi bits NOSCRN, NOERRFILE, NOREPORT
	const char* mOrH,	/* ptr to printf fmt message (e.g. "Bad value: %d")
	    				OR message handle (e.g. (char *)MH_X1002); message handles are
	    				defined in msghan.h and are associated with text in msgtab:msgTbl[] */
	va_list ap ) 	// ptr to variable arg list containing args as reqd by mOrH

/* this call is useful in other error functions that (for example)
   add formatting but pass args thru from their caller */

// NOTE: requires MUCH stack space for message buffer(s)

// returns caller's mOrH or RCBAD if mOrH is char *
{
	if ((erOp & ERAMASK)==IGN)	// if error action is "IGN" for ignore
		return RCBAD;			//    do NOTHING. exit here to save time of getting text.

	char buf[ MSG_MAXLEN+30];	// msg buffer, messages.h.
								//  +30 for "Program error:\n  " etc b4 message.
	strcpy( buf, errMsgPrefix( erOp, isWarn));

	char bufMsg[ MSG_MAXLEN];
	RC rc = msgI(			// retrieve and format message, messages.cpp
			 PWRN,			//   error reporting for (internal) errors in messages.cpp (calls err functions)
			 bufMsg,   		//   temp location in stack buffer atn which to build msg
			 sizeof( bufMsg),	// available size
			 NULL,			//   formatted length not returned
			 mOrH,			//   caller's message text or message handle
			 ap );			//   ptr to args for vsprintf() in msgI
	if (*buf)				// unless prefix "Error: " etc suppressed 2-94,
		if (strJoinLen( buf, bufMsg) > getCpl())	// test length of joined msgs
			strcat( buf, "\n  ");		// line would be too long, so start caller's message on new line
	strcat( buf, bufMsg); 	/* concatenate formatted caller's msg with "Error: " & possible newline:
    				   copy to lower subscripts in same buffer. */
	errI( erOp, isWarn, buf);	// report message
	return rc;			// return caller's mOrH or RCBAD
}		// errV

// ================================================================================
RC errI(		// report error common inner fcn for errCrit, errV
				//   never calls err or msg fcns: no recursion

	int erOp,	// error action (cnglob.h):
				//  IGN:         ignore: just return to caller.
   				//  (REG):       regular: message to screen, error file, err report if open; ret to caller. 0, omittable.
   				//  WRN or PWRN: also await a keypress, with abort option, to bring serious error to user's attention.
   				//  ABT or PABT: abort: after message and keypress, terminate program, do not return to caller.
   				//		(note PWRN PABT difference from WRN ABT done by callers errV and errCrit.)
   				// (NOPREF:      don't put "Error: " or "Program Error" in front of message -- done by callers.)
	int isWarn,	// 0: error: increment error count returned by errCount(). Output to screen, file, vr.
				// 1: warning: no errCount++; don't display on screen if setWarnNoScrn(1) has been called.
				// 2: info: no errCount++
				// + high bits NOSCRN, NOERRFILE, NOREPORT
	const char* _text )		// ptr to fully formatted message

// returns RCBAD as convenience...   or does not return on ABT or abort from keyboard.
{
	const char* text;
	if (erOp & SYSMSG)
	{
#ifdef _WIN32
		DWORD osErr = GetLastError();
		SetLastError( NO_ERROR);		// reset
		text = strtprintf( "%s\nGetLastError() = %d (%s)", _text, osErr, GetSystemMsg( osErr));
#else
		text = strtprintf("ERRNO: %i", errno);
#endif // _WIN32
	}
	else
		text = _text;

	int erAction = erOp & ERAMASK;		// isolate IGN/WRN/ABT or REG (0) from (user) option bits
	//  note this makes PWRN into WRN, PABT into ABT.

	if (erAction==IGN)			// if IGN
		return RCBAD;			//    do NOTHING ... tentatively not even count error, rob 5-97.

	if (!(isWarn & 0xF))		// unless caller said it was a "warning" or an "info"
		errorCounter++;			// count errors. rob 5-97.

	// message destination(s)...
	bool toScreen =
		(erOp & KEYP)			// force to screen if keypress requested (precaution re bad arg combinations)
		|| (!(isWarn & NOSCRN)			// if caller sez screen OK
			&& ((isWarn & 0xF) != 1		// "error" (0) or "info" (2) always goes to screen
				|| !warnNoScrn));		// "warning" (isWarn==1) goes to screen unless warnNoScrn is non-0

	bool toErrFile = !(isWarn & NOERRFILE);		// all errors and warnings go to error file if open and not suppressed
#ifdef VRR
	bool toVr = !(isWarn & NOREPORT);			//  and error virtual report if open
#endif

	// format: error messages are always followed by a ------------ line;
	// also we supply preceding ------------ line if not present (start file, after non-DASHES log msg, etc).
	// cursor and ------ status is remembered separately for each destination in local _____Ls variables (above).

	// output message

	if (toScreen)
		screenNF( text, DASHES);		// output to screen or "screen" window with ------ lines between messages

	if (toErrFile && errFile)
	{
		switch (errFileLs) 				// get to start of line after a --------- line
		{
		case midLine:
			fprintf( errFile, "\n");
		case begLine:
			fprintf( errFile, DASHLINE );
		default: ;  //case dashed:
		}
		fprintf( errFile, "%s\n" DASHLINE, text);   	// print to file
		errFileLs = dashed;
	}

#ifdef VRR
	if (toVr && VrErr		// if open (public variable in this file)
	  && erAction != ABT)	//   loop prevention re ASSERTs from VR code, 8-15
	{
		// title for report is deferred til 1st error
		//   1) to omit it if there are no errors,
		//   2) to usually get user-input runTitle text into it
		//   3) to keep report together in virtual report file
		if (vrIsEmpty(VrErr))				// first time, output title for ERR report
			vrStr( VrErr,
				   getErrTitleText() );			// format title text for this report, cncult.cpp

		switch (vrErrLs) 				// get to start of line after a --------- line
		{
		case midLine:
			vrStr( VrErr, "\n");
		case begLine:
			vrStr( VrErr, DASHLINE );
		default: ;  //case dashed:
		}
		vrStr( VrErr, text);
		vrStr( VrErr, "\n" DASHLINE);
		vrErrLs = dashed;
	}
#endif

	// conditionally get user response: keypress under DOS; use message box under Windows.
	int kpAbt = 0;
	if ( erAction & KEYP				// WRN and ABT common keypress bit (see defines in cnglob.h)
			&&  toScreen 					// insurance
			&&  !(batchMode && toErrFile && errFile) ) 	// not if "batch mode" and message went to open error file, 4-7-92
#if !defined( LOGWIN)
		kpAbt = presskey( erAction);			// get keystroke, nz if abort (below)
#else
		kpAbt = erBox( text, 				// display text again, in Windows msg box, and await response. below.
					   MB_ICONSTOP,			// show Stop icon
					   erAction != ABT);  	// if not already ABT, display 2 buttons so user can choose to abort.
	// returns TRUE if 2 buttons and abort (cancel) chosen
#endif

	// conditionally abort run

	if (kpAbt || erAction==ABT)				// if user says abort (or OUT OF MEMORY) or caller says abort
	{
		if (toScreen)
			screenNF( " *** Aborted *** ");		// no \n may leave another line showing
		if (toErrFile && errFile)
			fprintf( errFile, " *** Aborted *** \n");
#ifdef VRR
		// if (toVr && VrErr)  ...   don't bother, will not be unspooled since aborting here.
#endif
#if 0
x		ASSERT( FALSE);		// in debug build, try to get to debugger
#endif

		errFileOpen( NULL);	// close error message file if any (above)
		byebye( 2);		// exit program (exit(1) reserved for poss alt good exit)
	}

	return RCBAD;		// another return near entry, also program abort just above.
}			// errI
//-----------------------------------------------------------------------------
const char* GetSystemMsg(				// get system error text
	DWORD lastErr /*=0xffffffff*/)		// error #, from GetLastError()
//   default = do GetLastError() here
// returns message in tempstr, "" if NO_ERROR or FormatMessage failure
{
	char t[ 10000];
	t[ 0] = '\0';
	if (lastErr == 0xffffffff)
	{
#ifdef _WIN32
		lastErr = GetLastError();
		SetLastError(NO_ERROR);		// reset
#else
		lastErr = NO_ERROR;
#endif // _WIN32
	}
	if (lastErr != NO_ERROR)
	{
#ifdef _WIN32
		if (!FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM
					| FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					lastErr,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT),
					t, sizeof( t)-1, NULL))
			t[ 0] = '\0';	// FormatMessage failed
#else
		std::string tempS = strtprintf("ERRNO: %i", errno);
		strcpy(t, tempS.c_str());
#endif // _WIN32
	}
	return strTrim( NULL, t);	// copy to tmpStr
}			// GetSystemMsg

// ================================================================================
RC logit(		// put remark in log report and display on screen (NOT FC, var args)

	int op,		// option bits (rmkerr.h or cnglob.h)
				//   NONL on:  do NOT move to beg of next line b4 display.
				//        off: move to beg of next line if needed, then display rmk
				//   DASHES on: separate from preceding and following remarks with ----------- lines  rob 11-20-91
				//   NOSCRN on: do not display on screen
	const char* mOrH,	// ptr to printf fmt message (e.g. "Bad value: %d")
						// OR message handle (e.g. (char *)MH_X1002); message handles are
						//    defined in msghan.h and are associated with text in msgtab:msgTbl[]
	... )   			// args as reqd by mOrH

// usage examples:
//  logit(0, "xxx"):		  Display xxx on own line; xxx can contain additional \n for extra blank lines
//  logit(NONL, "xxx"):		  Display xxx at current cursor pos (e.g. for month name display during simulation)
//  logit(0, MH_xxxx):		  Display msgtbl.cpp message for handle MH_xxxx (handles defined in msghans.h)
//  logit(0, "Bad '%s'", what):	  Display text with substituted arguments

// returns caller's mOrH, or RCBAD if mOrH is char *.
{
	char buf[ MSG_MAXLEN];			// msg buffer
	va_list ap;
	int mLen;
	RC rc;
#ifdef VRR
	int toVr = 1;	// all log remarks currently (1-92) go to log virtual report if open
#endif

	// additional destination options might later be added...
	int toScreen = !(op & NOSCRN);		// log remarks also go to screen unless NOSCRN op bit given

	va_start( ap, mOrH);		// pt arg list

	rc = msgI(	// retrieve and format message, messages.cpp
			 PWRN,	//   msgI internal err rpt: program error, warn and continue
			 buf,	//   temp stack buffer in which to build msg
			 sizeof( buf),
			 &mLen,	//   formatted length returned
			 mOrH,	//   caller's message text or message handle
			 ap );	//   ptr to args for vsprintf() in msgI


// conditionally display remark on screen
	if (toScreen)
		screenNF( buf, op);		// output to screen or "screen" window with NONL and DASHES options

// conditionally output remark to "LOG" virtual report
#ifdef VRR
	if (toVr)					// if open (public variable in this file)
		logitNF( buf, op);
#endif

	return rc;				// return caller's mOrH or RCBAD
}			// logit
//---------------------------------------------------------------------------------
#if defined( VRR)
void logitNF(			// string to VrLog (no formating)
	const char* s,	// string
	int op/*=0*/)	// option bits
					//   NONL on:  do NOT move to beg of next line b4 display.
					//        off: move to beg of next line if needed, then display rmk
					//   DASHES on: separate from preceding and following remarks with ----------- lines  rob 11-20-91
					//   NOSCRN on: do not display on screen
{
	if (!VrLog)
		return;

	// title for report is deferred til 1st message 1) to omit it if there are no messages,
	// 2) to get user-input runTitle text into it, 3) to keep report together in virtual report file */
	if ( vrIsEmpty(VrLog))				// first time, output title for LOG report
		vrStr( VrLog, getLogTitleText() );		// format title text for this report

	if (!(op & NONL))				// NONL and DASHES together: following ---------- line only
		switch (vrLogLs) 			// get to start of line after a --------- line
		{
		case midLine:
			vrStr( VrLog,"\n");
		case begLine:
			if (op & DASHES)
				vrStr( VrLog, DASHLINE );
		default: ;	//case dashed:
		}
	int mLen = strlen( s);
	if (mLen)					// if there is a message (if not, still get newline and one ---line)
	{
		vrStr( VrLog, s);    			// output remark text
		int fiNewl = s[ mLen-1]=='\n';	// non-0 if message ends in newline
		vrLogLs = fiNewl ? begLine : midLine; 	// where cursor is now
		if (op & DASHES)				// if ---- lines desired
		{
			if (!fiNewl)
				vrStr( VrLog,"\n");    		// newline if msg did not end with one
			vrStr( VrLog, DASHLINE);		// print a -------- line after remark
			vrLogLs = dashed;				// remember status, so don't get double ----- lines
		}
	}
}		// logitNF
#endif
// ================================================================================
RC screen(		// display remark on display on screen only

	int op,		// option bits (rmkerr.h or cnglob.h)
				//   NONL on:  do NOT move to beg of next line b4 display.
				//        off: move to beg of next line if needed, then display rmk
				//   DASHES on: separate from preceding and following remarks with ----------- lines
				//   QUIETIF: suppress display if screenQuiet
	const char* mOrH,	// ptr to printf format message (e.g. "Bad value: %d")
				// OR message handle (e.g. (char *)MH_X1002); message handles are
				//    defined in msghan.h and are associated with text in msgtab:msgTbl[]
	...)				// args as reqd by mOrH

// returns caller's mOrH, or RCBAD if mOrH is char *.
{
// this function same as "logit" with logfile output code removed.

	char buf[ MSG_MAXLEN];   		// msg buffer
	va_list ap;
	int toScreen;
	RC rc;

	// destination might later be conditional on option bits ... perhaps error class bits ...
	toScreen = 1;	// currently 11-91 all remarks here go to screen

	va_start( ap, mOrH);		// pt arg list

	rc = msgI(	// retrieve and format message, messages.cpp
			 PWRN,	//   msgI internal err reporting: program error, warn and continue
			 buf,	//   temp stack buffer in which to build msg
			 sizeof ( buf),		// size available
			 NULL,	//   formatted length not returned
			 mOrH,	//   caller's message text or message handle
			 ap );	//   ptr to args for vsprintf() in msgI

	if (toScreen)
		screenNF( buf, op);		// display message on screen or "screen" window
	return rc;
}		// screen
// ================================================================================
void screenNF(		// display remark on screen only (no formatting)
	const char* text,	// pointer to fully formatted message
	int op /*=0*/)	// option bits (rmkerr.h or cnglob.h)
					//   NONL on:  do NOT move to beg of next line b4 display.
					//        off: move to beg of next line if needed, then display rmk
					//   DASHES on: separate from preceding and following remarks with ----------- lines
					//   QUIETIF: suppress display if screenQuiet
{
// macro to display console or console-equivalent message
// NOTE: use printf-style fcns with care due to possible
//    embedded '%' etc in e.g. file names
#if defined( LOGWIN)
#define DISPLAY screenLogWin.append
#elif defined( LOGCALLBACK)
#define DISPLAY LogMsgViaCallBack		// send message to DLL caller
#else
#define DISPLAY(s) printf("%s",s)
#endif

// do nothing if
	if ((op & QUIETIF) && screenQuiet)
		return;

// display remark on screen
	if (!(op & NONL))				// NONL and DASHES together: following ---------- line only
		switch (screenLs)    			// get to start of line after a --------- line
		{
			/*lint -e616 */
		case midLine:
			DISPLAY("\n");
		case begLine:
			if (op & DASHES)
				DISPLAY( DASHLINE );
			screenCol = 0;		// cuz DASHLINE ends with \n
		default: ;	//case dashed:
		}
	if (text && *text)					// if there is a message (if not, still get newline & one ---line)
	{
		DISPLAY(text); 				// display remark text
		int len = strlen(text);
		int fiNewl = text[len-1]=='\n';		// non-0 if message ends in newline
		if (op & DASHES)				// if ---- lines desired
		{
			if (!fiNewl)
				DISPLAY("\n");   			// newline unless message ended with one
			DISPLAY( DASHLINE);			// print a -------- line after remark
			screenLs = dashed;   			// remember status
			screenCol = 0;			// cuz DASHLINE ends with \n
		}
		else					// no ----\n after text
		{
			screenLs = fiNewl ? begLine : midLine;	// where cursor is now
			const char* lNewl = strrchr( text, '\n');		// point last newline in message, NULL if none
			if (lNewl)
				screenCol = int(text + len - 1 - lNewl);	// column is # chars after last newline (len is >= 1 if \n found)
			else
				screenCol += len;				// no newline in message: add msg length to column
		}
	}
}			// screenNF
//---------------------------------------------------------------------------
int setScreenQuiet( int sq)
{	int sqWas = screenQuiet;
	screenQuiet = sq != 0;
	return sqWas;
}		// ::screenQuiet
//---------------------------------------------------------------------------
BOO mbIErr( const char* fcn, const char* fmt, ... )
{
	va_list( ap);
	va_start( ap, fmt);
	const char* t = strtvprintf( fmt, ap);
	errCritV( PERR, 1 /*isWarn*/, strtprintf( "%s %s", fcn, t), NULL);
	return FALSE;
}	// mbIErr

#if defined( LOGWIN)
// ================================================================================
BOO erBox(		// display error message and wait for responce

	const char *text, 	// text to show in box, already fully formatted
	unsigned int style,	// style bits, eg MB_ICONSTOP or MB_ICONINFORMATION. not UINT so .h can be included b4 Windows.h.
	BOO conAb ) 	// TRUE:  give caller choice of continuing or aborting (two buttons).
// FALSE: one button only: OK (only single button avail in Windows MessageBox function)

// returns TRUE if conAb and CANCEL button pressed, or if OUT OF MEMORY & can't even display message
{
// determine parent window handle:
	// should be top window so clicking in wrong place cannot hide msg box with another child of same parent. works. 2-12-94.
	HWND hPar = cneHPar;		// parent window supplied by CSE caller (cnsimp.cpp public variable)
	if (screenLogWin.isOpen())		// if our "console" window is open
		hPar = screenLogWin.getHwnd();	// cneHPar is its parent, so use its handle

// display message box
	INT ret = MessageBox( 		// returns 0 if fails, or non-0 IDxxxx of button clicked. Windows fcn.
				  hPar,
				  text,
				  MBoxTitle,		// caption text "CSE Simulation Engine" or as changed (cse.cpp)
				  style					// eg MB_ICONSTOP
				  | (conAb ? MB_OKCANCEL : MB_OK) );	// (MB_OK is 0)

#if 1	// added .420 1-22-94
	// DOESN'T WORK in that an observed CSE.DLL/CNEW X0030 error repeated til stack overflow still showed not message boxes.
	if (!ret)				// if not enough memory to display message box
	{
		MessageBox( cneHPar,
					"OUT OF MEMORY",			// whatever the other error was, we are now out of memory
					MBoxTitle,
					MB_ICONHAND | MB_SYSTEMMODAL 	// request 3-line-max box that displays regardless of memory
					| MB_OK );
		return TRUE;					// tell caller errI() to abort program
	}
#endif
	return (conAb && ret==IDCANCEL);
}					// erBox
//---------------------------------------------------------------------------
#endif	// LOGWIN
// ================================================================================
void FC yielder()	// let other programs run (if under Windows)
{
#ifdef WINorDLL	// else do nothing
	// do any waiting messages for this app.
	// CALLS SELF eg to repaint if tasks switched or if covered then uncovered.

	// if messages aren't removed, yielding found unreliable.
	// Apparently PeekMessage yields only if queue empty. 2-94 Win 3.1.

	MSG msg;
	while (PeekMessage( &msg, 0/*HWND*/, 0,0/*filters*/, PM_REMOVE))	// if any msg for this app (else yields(?))
	{
		// do it:
		TranslateMessage(&msg);		// translate virtual key codes
		DispatchMessage(&msg);		// dispatch: call appropriate window proc. CALLS OWN WINDOWS with nested messages.
	}
#endif	// WINorDLL
}			// yeilder

/*----------------------------- LOCAL FUNCTIONS ---------------------------*/
LOCAL int presskey( 	// prompt user after error message display
	int erAction )	// ABT or other value (changes prompt to user)

// returns nz for program abort (per user request or erAction==ABT)
//          0 for execution continuation
{
#if defined( LOGCALLBACK)
	return 1;		// DLL w/ callbacks: no user prompt, just say "*** Aborted ***"
#elif !defined( LOGWIN)
	printf(
		erAction == ABT
		? "Press any key (program will abort) "
		: "Press 'A' to abort, any other key to continue " );
	int key = getch();							// input any char
	printf(  "\r                                               \r"); 	// erase prompt on screen
	// if screenLs was 'dashed' or 'begLine', it is now the same.
	if (screenLs==midLine)		// if was midline (unexpected)
		screenLs = begLine;		// say cursor now at beg of blank line (for errI, logit, and screen above)
	return erAction==ABT || toupper(key)== 'A';
#else
	return 1;		// don't call if LOGWIN (use erBox)
#endif
}						// presskey
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// debug printing
///////////////////////////////////////////////////////////////////////////////
#define VR_DEBUGPRINT

#if defined( VR_DEBUGPRINT) && defined( VRR)
static int dbgVrh = 0;			// virtual report handle for debug report
#else
static FILE* dbgFile = NULL;	// FILE to receive dbg printing (or NULL)
#endif
static DWORD dbgMsk = 0;		// mask (allows selective printing)
//-----------------------------------------------------------------------------
BOOL DbShouldPrint( DWORD oMsk)
{
	return (oMsk & dbdALWAYS) || (oMsk & dbgMsk);
}		// DBShouldPrint
//-----------------------------------------------------------------------------
DWORD DbSetMask( DWORD newMsk)
{
	DWORD oldMsk = dbgMsk;
	dbgMsk = newMsk;
	return oldMsk;
}		// DbSetMask
//-----------------------------------------------------------------------------
RC DbFileOpen(
	const char* _dbFName)	// pathname incl extension for error message file, or NULL to close open error message only.

// close/open debug print file, closes any already-open file, and deletes it if empty

// returns RCOK if all OK,
//         RCBAD if file _dbFName cannot be opened
{
	RC rc=RCOK;
#if defined( VR_DEBUGPRINT)
	if (_dbFName)
		unlink( _dbFName);		// erase file -- don't garbage up directory
#else
	static char dbFName[ _MAX_PATH] = { 0 };

// close existing error message file, if any.  recursion CAUTION: called from errI.
	if (dbgFile != NULL)
	{
		LI len = -2L;
		if (fseek( dbgFile, 0L, SEEK_END)==0)	// position pointer to end, 0 if ok
			len = ftell( dbgFile);		// read file pointer, -1L if bad
		fclose( dbgFile);
		if (!IsBlank( dbFName))				// if have saved name -- insurance
		{
			if (len==0L)				// if file is empty
				unlink( dbFName);		// erase file -- don't garbage up directory
#ifdef OUTPNAMS	//may be defined in cnglob.h. 10-93.
			else					// file not erased,
				saveAnOutPNam( dbFName);	// save name of file for return to caller
#endif
		}
		dbgFile = NULL;				// say have no open file
		dbFName[ 0] = '\0';
	}

// open file with specified name if any given
	if (_dbFName != NULL)			// NULL just closes
	{
		dbgFile = fopen( _dbFName, "wt");   	// open empty text file for writing
		if (dbgFile==NULL)					// if open failed
			rc = errCrit( PWRN, "X0010: Cannot open dbg file '%s'", _dbFName);	// set bad return, issue msg

		strncpy0( dbFName, _dbFName, sizeof( dbFName));		// internal copy of name
															// for deletion if file empty at close
	}
#endif
	return rc;
}		// DbFileOpen
//----------------------------------------------------------------
void DbPrintf(			// debug printf
	const char* fmt,	// printf-style args
	...)
{
	va_list ap;
	va_start( ap, fmt);
	DbVprintf( fmt, ap);
}					// DbPrintf
//----------------------------------------------------------------
void DbPrintf(			// conditional debug printf
	DWORD oMsk,			// mask: print iff corres bit(s) on in dbgMsk
	//       dbdANY: print if any app bit is on
	//		 dbdALWAYS: print always
	const char* fmt,	// printf-style args
	...)
{
	if (DbShouldPrint( oMsk))
	{
		va_list ap;
		va_start( ap, fmt);
		DbVprintf( fmt, ap);
	}
}					// DbPrintf
//----------------------------------------------------------------
void DbVprintf(					// vprintf-to-debug
	const char* fmt,		// printf-style format
	va_list ap /*=NULL*/)	// arg list (NULL = no format)
{
	if (ap)
		fmt = strtvprintf( fmt, ap);	// format

#if defined( VR_DEBUGPRINT) && defined( VRR)
	logitNF( fmt, NONL);
#else
	if (dbgFile)
		fprintf( dbgFile, fmt);
#endif
}			// DbVprintf
//=============================================================================

// rmkerr.cpp end
