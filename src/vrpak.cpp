// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// vrpak.cpp  virtual reports

// generates an indefinite number of report files using spooling method
// limits # file handles in use simultaneously

//--------------------- working notes, delete
/* could do

    use collections for e.g. VROUTINFO vrhs[ ]

    establish a max text length, check here at write, be sure min buffer accomodates (put both sizes in vrpaki.h)

    spool during unspool (eg error messages) -- will now screw up unspool if buffer moved -- 11-91.
       a) check for (have an unspooling flag in spl)
       b) partial handing: append to spool file if end in ram and enuf buffer space
       c) further handling: write to end file on disk if not in ram
       d) meanwhile, cse.cpp 0's VrErr and other critical vrh's during unspool calls.
*/

/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"	// CSE global defines, incl our ASSERT macro 8-95.
#include <fcntl.h> 	// O_CREAT O_BINARY
#include <errno.h>	// EMFILE

#include "ancrec.h"	// record: base class for rccn.h classes
#include "rccn.h"	// TOPRATstr
#include "msghans.h"	// MH_R1200

#include "xiopak.h"	// xfopen, xfwrite,
#include "messages.h"	// msgI, MSG_MAXLEN, err

#include "cnguts.h"	// Top
#include "cncult.h"	// getBodyLpp
#include "vrpak.h"	// external decls for this file


//---------------------------- TYPES AND DATA -------------------------------

// type that starts each entry in virtual report spool file
enum SpoolTy
{
	vrBody = 243,		// body text. handle, text, \0 follows.
	vrFormat			// page formatting text, ditto.  omit if outputing to unformatted file.
};
//---------------------------------------------------------------------------
// info re one virtual report: struct for dm array spl.vr[]
struct VRI
{
	char spooling, spooled;	// status booleans
	const char* vrName;	// name text for error messages.   used ???  use it eg on bad handle msg
						//   also possible unspool-time selection key and/or unspool file name??
	int optn;			// option bit(s): VR_FMT (vrpak.h) for page formatting on
						// note pageN/row/col operate per text received even for unformatted reports
						//   (but have no relation to eventual output).
	int pageN; 			// during spool; pageN is # pages at end (+1 if final form feed?).
	int col;
	int row;			// line #: on current page during spooling; on last page when spooling done.
	int row1;			// # rows on first page if pageN > 1
	ULI o1, o2;			// start and end offsets in spool file of vr's data, both -1L if no data in vr
};
//---------------------------------------------------------------------------
// once-only info on virtual reports and spool file: static, shared amoung vr files & fcns.
struct SPL
{
// re spool file
	BOO isInit;			// nz if initialized (data-init to 0)
	BOO isOpen;			// nz if spool file is open (possibly redundant)
	const char* splFName;	// spool file name  (is separate copy needed??)
	XFILE *splxf;   	// spool file
	ULI spO;			// spool file offset: curr ptr while writing, eof offset while reading.
	ULI spWo;			// offset to which written - any bytes after spWo not on disk yet.
	BOO vrRc;			// nz if previous error that must terminate vr scheme, such as i/o error on spool file.
    					// Nops further calls.  DO NOT SET on local errors that admit of continuation. */
// re virtual reports in spool file
	int sp_nVrh;		// max used handle (vr subscript+1).  handle 0 not used.
	int sp_vrNal;		// allocated size of vr
	VRI* sp_vr;			// pointer to active VRI table in dm
// re unspooling
	VROUTINFO*			// unused report file (vrUnspool) argument info:
		voInfo;			// ... points to NULL or VROUTINFO (vrpak.h) for next output file
	// each variable-length, 0-terminated VROUTINFO is followed by another or NULL.
	ULI runO1, runO2;	// min and max+1 spool file offsets of vr's in current run
	int uN;			// number of uns[] entries in use
	int nFo;     	// number of open output files
	int maxFo;    	// nFo after which dos open error occurred: assume out of handles, do not exceed again.
// re spool data in buffer
	char* p1; 		// start of data in buffer.  code assumes always same as buf1.
	char* p;		// read ptr / during write, points to start of item (for dropping)
	char* p2;		// end of data+1 / write ptr
	ULI bufO1, bufO2;	// file offset of p1, p2
// re spool file buffer (in dm)
	static const ULI BUFSZ;		// nominal size
	ULI bufSz;		// working size (BUFSZ-1)
	char* buf1;		// start
	char* buf2;		// end+1
};	// struct SPL
const ULI SPL::BUFSZ = 0xf000;	// spool file buffer size
								//   tested OK at 0x2800, 8-15
static SPL spl = { 0 };		// info about spool file, etc; points to VRI vr[] in dm.

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC vrPut( const char* s, int n, int incl);
LOCAL RC vrCkHan( int vrh, VRI **pvr);
LOCAL RC vrErr( const char* fcn, const char* mOrH, ...);

static void vrpak2Clean();
static RC vrErrIV( const char* file, const char* fcn, const char* mOrH, va_list ap=NULL);

static RC vrBufAl();
static RC vrBufLoad();
static RC vrBufMore();
static RC dropFront( ULI drop);


//---------------------------------------------------------------------------------------------------------------------------
void vrpakClean( 		// vrpak overall init/cleanup routine

	[[maybe_unused]] CLEANCASE cs )	// [STARTUP,] DONE, or CRASH (type in cnglob.h)

{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
// function should: initialize any variables for which data-initialization was previously assumed,
//                  free all heap memory and NULL the pointers to it,
//                  close any files and free other resources.

	vrTerminate();		// clean spl: close and delete spool file if open, free dm:
    					// spl.vr[].vrName, spl.vr, spl.splFName, spl.splxf, spl.buf1
	memset( &spl, 0, sizeof( spl));	// zero spl: rerun insurance.

	vrpak2Clean();			// clean local variables

}				// vrpakClean
//---------------------------------------------------------------------------------------------------------------------------
//  control functions re outputting to virtual reports
//---------------------------------------------------------------------------------------------------------------------------
RC vrInit( const char* splFName)	// initialize virtual report scheme
// optional, use when desire to set file pathName
{
// checks; init info structure "spl"

	if (!spl.isInit)  				// flag data-init 0
		memset( &spl, 0, sizeof(spl));		// zero all members.  probably redundant; insurance.
	if (spl.isOpen)				// vrErr: program err msg fcn, below, interfaces to "err("
		vrErr( "vrInit", (char *)MH_R1201 );	//    msg handle for "Ignoring redundant or late call"
	spl.splFName = strsave( splFName);
	spl.isInit++;				// say spl is initialized, don't 0 it again.

// open spool file.  This receives all virtual report output, unsorted.  Note: buffer alloc'd in vrPut.

	spl.splxf = xfopen( spl.splFName, O_WB, WRN, TRUE/*eof is error*/, NULL);
	if (!spl.splxf)			// if open failed
		spl.vrRc = RCBAD;		// disable further calls
	else
		spl.isOpen++;			// say spool file is open
	spl.spO = spl.spWo = 0L;		// insurance
	return spl.vrRc;			// RCOK (0) if opened ok
}			// vrInit
//---------------------------------------------------------------------------------------------------------------------------
RC vrClear()		// delete all virtual reports in spool in preparation for re-use
{
	if (!spl.isInit)					// nop if spool has not been intialized
		return RCOK;					// (may occur b4 1st run)

// clear spool file
	if (spl.isOpen)					// if open (probably always true here)
	{
		if (xfclear(spl.splxf) != SECOK) {
			return RCBAD;
		}
	}
	spl.spO = spl.spWo = 0;					// set file end offset and written offset to 0
	spl.bufO2 = spl.bufO1 = 0;				// forget any data in buffer
	spl.p2 = spl.p1 = spl.buf1;				// .. insurance
	// should we deallocate buffer??

// deassign all handles
	if (spl.sp_vr)
		for (int i = 0;  ++i <= spl.sp_nVrh; )
			dmfree( DMPP( spl.sp_vr[i].vrName));   		// free dm name strings for vrh's to be free'd 2-14-94
	spl.sp_nVrh = 0;
	// spl.sp_vr is left allocated.

// clear any prior error
	spl.vrRc = RCOK;
	return RCOK;
}			// vrClear
//---------------------------------------------------------------------------------------------------------------------------
void vrTerminate()		// clean up after use of vr stuff: "destructor"  DOES NOT UNSPOOL
{
	if (spl.isInit)			// else nothing to do
	{

		// close and delete the temporary spool file
		if (spl.isOpen)
			xfclose( &spl.splxf, NULL);       	// close file, xiopak.cpp
		spl.isOpen = 0;
		if (spl.splFName)			// insurance
			xfdelete( spl.splFName, WRN );   	// delete file, xiopak.cpp
		dmfree( DMPP( spl.splFName));
		dmfree( DMPP( spl.buf1));   		// free spool file data buffer if any

		// free VRI table
		if (spl.sp_vr)
			for (int i = 0;  ++i <= spl.sp_nVrh; )
				dmfree( DMPP( spl.sp_vr[i].vrName));		// free dm name strings
		dmfree( DMPP( spl.sp_vr));

		// say virtual report system not initialized
		spl.isInit = 0;				// insure full re-init or errMsg on any following vr call
	}
	// void return, for conversion to C++ destructor (else return vrRc | close and delete errors)
}		// vrTerminate
//---------------------------------------------------------------------------------------------------------------------------
RC vrOpen( 		// open virtual report and return handle

	int* pVrh,  		// receives handle to use in subsequent calls
	const char* vrName, // name for poss use in err msgs, or NULL to use itoa(vrh)
	int optn )			// option bit(s) (vrpak.h):
						//   VR_FMT: on for formatting at spool time (formatting will be removed at
						//       unspool time if output file is unformatted)
						//   more -- see vrpak.h
{
	*pVrh = 0;

// initialize if first call
	if (!(spl.isInit && spl.isOpen))
	{	const char* splFName = strffix2( InputFilePathNoExt, ".spl", 1);
		RC rc;		// for CSE_E()
		CSE_E( vrInit( splFName));
	}

// fail now if prior serious error
	if (spl.vrRc)				// non-RCOK if error that should terminate virtual reports has occurred
		return spl.vrRc;

// (re)alloc vr table if not large enuf.  Note slot 0 is not used.
	if (spl.sp_nVrh + 1 >= spl.sp_vrNal)			// +1 for increment later before use (0 is not used)
	{
		int nuNal = spl.sp_vrNal + 16;	// add 16 vr's at a time
										// 2 tested ok. increase to 32??? depending on use.
		if (dmral( DMPP( spl.sp_vr), nuNal * sizeof(VRI), DMZERO | WRN))  	// initial allocation or reallocation. dmpak.cpp.
			return RCBAD;
		spl.sp_vrNal = nuNal;
	}

// assign handle and fill & initialize table entry
	int vrh = ++spl.sp_nVrh;			// 0 is not used ---> ++ first.
	char nmBuf[20];
	if (!vrName)						// if no name given, use text of handle number
		vrName = itoa( vrh, nmBuf, 10);		// moved to dm below
	VRI* vr = spl.sp_vr + vrh;
	memset( vr, 0, sizeof(VRI) );	// 0 table entry (needed if entry previously used)
	vr->vrName = strsave(vrName);	// make our own dm copy, free'd by vrTerminate
	vr->optn = optn;			// store options: VR_FMT,
	vr->pageN = 1;				// 1-based for external display
	//vr->row1 = vr->row = vr->col = 0;  	// 0'd by memset above
	vr->spooling++;				// say virtual report is open to receive text into spool
	vr->o1 = vr->o2 = ULI(-1);	// say no data in this vr yet

	*pVrh = vrh;				// return handle

	return spl.vrRc;			// return bad if any error since vrInit or vrClear
}		// vrOpen
//---------------------------------------------------------------------------------------------------------------------------
RC vrClose( int vrh)			// close a virtual report -- terminate output to it

// does NOT zero caller's handle as unspool is expected after close

// currently 11-91 optional: using detects erroneous writes after call
{
	VRI *vr;
	RC rc;
// check
	CSE_E( vrCkHan( vrh, &vr));		// ret bad now if if bad handle or prior error (vrRc)

// say not open
	vr->spooling = 0;		// say not ready to receive text
	vr->spooled = 1; 		// say has received text
	return spl.vrRc;
}			// vrClose
//---------------------------------------------------------------------------------------------------------------------------
int vrGetOptn( int vrh)		// retrieve option bits for a vr

// for hypothetical use re transmission in vr optns of REPORT etc command options to report generating code.
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))
		return 0;			// bad argument returns 0 options after error message
	return vr->optn;
}			// vrGetOptn
//---------------------------------------------------------------------------------------------------------------------------
RC vrChangeOptn( int vrh, int mask, int optn)	// masked option bits change for open virtual report

// expected use: to add VR_FMT option after opening but b4 writing data.
// use with CAUTION
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))
		return RCBAD;
	vr->optn = (vr->optn & (~mask)) | (optn & mask);
	return RCOK;
}			// vrChangeOptn
//---------------------------------------------------------------------------------------------------------------------------
RC vrIsFormatted( int vrh)		// test if formatting is on for virtual report

// note: if on, formatting can still be off for output file
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))			// if bad handle or prior error (vrRc)
		return 0;				// return as not formatted
	return ((vr->optn & VR_FMT) != 0);		// TRUE if formatting option (vrpak.h) on
}					// vrIsFormatted
//---------------------------------------------------------------------------------------------------------------------------
int vrIsEmpty( int vrh)		// test if any output yet in virtual report

// used e.g. to write heading first time
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))			// if bad handle or prior error (vrRc)
		return 0;				// return as though non-empty
	return (vr->o2 <= vr->o1);			// TRUE if end <= beginning
}				// vrIsEmpty
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// output to virtual report functions
///////////////////////////////////////////////////////////////////////////////
RC vrSetPage( int vrh, int pageN)	// set page

// if not used, page increases from 1 for each virtual report

// returns RCOK
{
	VRI *vr;
	RC rc;
	CSE_E( vrCkHan( vrh, &vr));		// ret bad now if if bad handle or prior error (vrRc)
	vr->pageN = pageN;
	return RCOK;
}		// vrSetPage
//===========================================================================
int vrGetPage( int vrh)		// get page number

// returns 1-based page number, or -1 if bad handle or other error
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))
		return -1;
	return vr->pageN;
}			// vrGetPage
//===========================================================================
int vrGetLine( int vrh)		// get 0-based line number on page

// returns 0-based line number on page for virtual report, or -1 if bad handle or other error

// First printable line is 0.  Much calling code is coded on this assumption
{
	VRI *vr;
	if (vrCkHan( vrh, &vr))
		return -1;
	return vr->row;
}			// vrGetLine */
//===========================================================================
RC CDEC vrPrintf( int vrh, const char* s, ...)		// printf to a virtual report (like vrPrintfF without 2nd arg)

// CAUTIONS: uses much stk space; no overlong msg check.
{
	va_list ap;
	va_start( ap, s);
	return vrVPrintfF( vrh, 0, s, ap);
}				     // vrPrintf
//===========================================================================
RC CDEC vrPrintfF( 			// printf to virtual report with format flag

	int vrh, 	// desitination virtual report handle
	int isFmt,	// non-0 if this text is page formatting information -- omit if report is unformatted
	const char* s,	// message ('format' for printf)
	... )	// printf-style args as required by mOrH

// CAUTION: does NOT check for overlong message.  Limit MSG_MAXLEN chars formatted.
// returns RCOK or error code
{
	va_list ap;
	va_start( ap, s);				// point to first of the "..." arguments
	return vrVPrintfF( vrh, isFmt, s, ap);	// use variable arg list version (next)
}					// vrPrintf
//===========================================================================
RC vrVPrintfF( 			// vprintf to virtual report

	int vrh,    		// desitination virtual report handle
	int isFmt,		// non-0 if page formatting text
	const char* s,	// message ('format' for printf) or handle (see messages.h/cpp)
	va_list ap)		// variable arg lst of printf-style args as required by mOrH

// CAUTION: does NOT check for overlong message.  Limit 1000 chars formatted.
// returns RCOK or error code
{
	char buf[ MSG_MAXLEN];
	msgI( WRN, buf, sizeof( buf), NULL, s, ap);		// fetch text for message handle
	return vrStrF( vrh, isFmt, buf);		// output it with non-formatting vr string outputter (next)
}			     	    // vrVPrintfF
//===========================================================================
RC vrStr( int vrh, const char* s)	// output string to virtual report (vrStrF without 'isFmt' argument)
{
	return vrStrF( vrh, 0, s);
}					// vrStr
//===========================================================================
RC vrStrF( 		// output string to virtual report with format info option; inner function.

	int vrh,			// virtual report handle (from vrOpen)
	int isFmt,		// non-0 if page formatting text: omit (now) if unformatted report or (at unspool) if unformatted file
	const char* s )	// string. \r \n \f processed here, even if text omitted in unformatted report.

// returns RCOK or error code
{
	RC rc;

	int bodyLpp = getBodyLpp();	// get best-yet-avail report body lines per page: if Top.bodyLpp not
    							//  yet set (eg if spooling an input-time error message), may return
    							//  Topi.bodyLpp or a default value. cncult.cpp
	VRI* vr;
	CSE_E( vrCkHan( vrh, &vr));			// checks/set vr table pointer; return bad now if bad vrh or prior error

	int incl = !isFmt || (vr->optn & VR_FMT);	// include text if not formatting text or report is page-formatted

	/* format text for unformatted report is not put in spool, but is
	   scanned to maintain row counter, so callers such as cpnat.cpp (which
	   calls vrGetLine) can proceed oblivious to formatting option.
	   (Not sure this really matters; study code if need to know.) */

	if (!*s)		   			// if string length 0
		return RCOK;				// don't bother to write it
	if (incl)					// not if text not going into buffer
		if (int( vr->o1) < 0)			// first time (preset to -1L)
			vr->o1 = spl.spO;			// save spool offset of start of vr's data

// write start-of-text stuff into spool
	char oBuf[ sizeof(int)+1];
	spl.p = spl.p2;				// keep spl.p pointing to start of an item: if buffer full, write to p.
	oBuf[0] = isFmt ? static_cast<char>(vrFormat)	// start-format-text code in a char
					: static_cast<char>(vrBody);		// start-body-text code in a char.  oBuf contents also used below.
	*(int *)(oBuf+1) = vrh;			// handle in an int
	vrPut( oBuf, sizeof(oBuf), incl); 	// output characters -- or not if !incl
	// vrPut nops if spl.vrRc is set, so we don't check each return.
// loop till out of text

	while (*s)
	{
		int n = strcspn( s, "\r\n\f");	// n = # chars that require no special processing
		if (n)
		{	vrPut( s, n, incl);   	// output text to spool file
			vr->col += n;			// account for motion of print head
			s += n;			// point past characters output
		}
		else				// have a control
		{
			switch (*s++)		// CAUTION: cases must match "text" in strcspn above
			{
			case '\r':			// cr: test for cr and lf together

				if (*s != '\n')    		/* if cr without lf */
				{
					vrPut( "\r", 1, incl);   	// send character
					vr->col = 0;			// say carriage is at left
					break;
				}
				s++;				// treat crlf as newline
				/* fall thru */

			case '\n':			// newline (or line feed, not preceded by cr in same text)

				/* note: non-header-footer version, if page full, would check first for full page and
				   write only \f or \r\f, not \r\n\f, to permit using all lines with Lpp=paper length while
				   not omitting \f's (in case paper actually longer or needed eg on PostScript printer). */

				vrPut( "\r\n", 2, incl);    	// send crlf to spool
				vr->col = 0;
				vr->row++;		// update posn
				if ( vr->row < bodyLpp		// if page not full (can still print a line of text)
				||  !(vr->optn & VR_FMT) )	// or report not page-formatted
					break;				// done

				/* If formatted page overfilled at crlf, output form feed to invoke footer/header at unspool time.
				   Must be flagged as "formatting data" so will be deleted if file not page-formatted. */

				vrPut( "", 1, incl);			// write \0: terminate current string.
				vrStrF( vrh, 1, "\f");			// call self to write the form feed as "formatting text"
				vrPut( oBuf, sizeof( oBuf), incl);	// write start-of-string stuff again (oBuf still set).
				break;

			case '\f':		// form feed
				if (vr->col != 0)   		// if not at left
					vrPut( "\r\n", 2, incl);	// supply crlf (insurance)
				vrPut( "\f", 1, incl);  		// output form feed
				if (vr->pageN <= 1)		// if first page
					vr->row1 = vr->row;		// save # rows on 1st page for poss use re ^L between vr's when unspooling
				vr->row = 0;
				vr->col = 0;
				vr->pageN++;
				break;

			default: ;		// satisfy lint
			}		      // switch
		}		    // if - else
	}		 	// while *s

// write \0 to terminate text in spool
	vrPut( "", 1, incl);			// write a \0

// keep track of vr's ending file offset in spool, so can optimize unspool
	if (incl)
		vr->o2 = spl.spO;

	return spl.vrRc;			// return bad if error during call
}			// vrStrF
//===========================================================================
LOCAL RC vrPut( 	// transmit n bytes to virtual report -- writes to vr spool file

	const char* s,	// location of bytes to output
	int n, 			// # chars. nulls that gets here is transmitted.
	int incl )		// 0 to not output this text -- for convenience

/* returns RCOK if successful
      else nz if error or output is disabled due to prior error */
{
	if (incl && !spl.vrRc)				// if text to be included and no serious error yet
	{
		// allocate buffer first time here

		// do we want to leave some memory avail, say by alloc'ing 32K, allocing buffer, then freeing the 32K?
		// if so, might wanna have logic to attempt to expand buffer at start of unspool and start addl run
		if (!spl.buf1)				// if no buffer allocated,
			if (vrBufAl())			// allocate buffer, init p, p1, p2, buf1,2.  vrpak3.cpp.
				return spl.vrRc;			// failed.  spl.vrRc is set.

		// if eof not in buffer, flush buffer contents
		// This might occur when more spooling follows unspooling.
		// Or could try clever interface to vrpak:loadBuf to conditionally read the gap.
		if (spl.bufO2 != spl.spO)		// if end of buffer is not write point
		{
			ASSERT( spl.spO==spl.spWo);		//    then end file must have been written to disk
			dropFront( ULI( spl.p2 - spl.p1));  	// write or discard entire buffer contents, update p,p1,p2, bufO1,2. vrpak3.
			spl.bufO1 = spl.bufO2 = spl.spO;	// set offsets of empty buffer to those of file eof
			ASSERT( n < spl.buf2 - spl.p2);
		}

		// write buffer if necessary to make space.  p points to item start===end of bytes to drop.

		else if (n > spl.buf2 - spl.p2)		// includes ptrs NULL or equal: not alloc'd: insurance
		{	if (spl.p  &&  spl.p > spl.p1)		// insurance
				dropFront( spl.p - spl.p1);		// write/discard bytes from front of buffer
			if (!spl.p							// if p not set -- buf was not alloc'd at start item -- nb 2+++ shd fit
			 || n > spl.buf2 - spl.p2 )			// or still too little free space in buffer
				return spl.vrRc = vrErr( "vrPut", (char *)MH_R1202);	// "Too little buffer space and can't write: deadlock"
		}

		// move bytes to buffer
		if (n)						// insurance
			memcpy( spl.p2, s, n);
		spl.p2 += n;				// pointer to end of buffer
		ASSERT( spl.spO==spl.bufO2);
		spl.bufO2 =					// file offset of end of buffer
			spl.spO += n;   		// file offset of end of file
		// spl.spWo is updated at write -- by bufWrite from dropFront.
	}
	return spl.vrRc;					// 2+ error returns above
}			// vrPut
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vrCkHan( int vrh, VRI **pvr)	// internal fcn to check handle, set pointer to vr table
{
// general checks
	if (!spl.isInit)
		return vrErr( "vrCkHan", (char *)MH_R1203);	// "Virtual reports not initialized"
	if (spl.vrRc)					// if there has been a previous serious error (already reported)
		return spl.vrRc;					// silent bad return: nop any further calls
	if (!spl.isOpen)					// unexpected after preceding checks (?)
		return vrErr( "vrCkHan", (char *)MH_R1204);  	// "Virtual reports spool file not open"

// check handle
	if (vrh <= 0  ||  vrh > spl.sp_nVrh)
	{
		return vrErr( "vrCkHan", (char *)MH_R1205, vrh); 	// "Out of range virtual report handle %d"
	}

// point to table entry for handle
	VRI* vr = spl.sp_vr + vrh;

// check vr table entry
	// always desired? if not caller must do:
	if (!vr->spooling)
		return vrErr( "vrCkHan", (char *)MH_R1206, vrh);   	// "Virtual report %d not open to receive text"

	*pvr = vr;
	return spl.vrRc;				// RCOK expected here
}			// vrCkHan


//---------------------------------------------------------------------------------------------------------------------------
//  local error message fcns
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vrErr( const char* fcn, const char* mOrH, ...)	// internal error fcn for vrpak.cpp only
{
	va_list ap;
	va_start( ap, mOrH);
	return vrErrIV( "vrpak.cpp", fcn, mOrH, ap);			// next
}						// vrErr
//---------------------------------------------------------------------------------------------------------------------------
static RC vrErrIV( const char* file, const char* fcn, const char* mOrH, va_list ap/*=NULL*/)
{
	char buf[MSG_MAXLEN];

// format caller's submessage
	// following is like  vsprintf( buf, msg, ap)  plus retrieves text for codes (mOrH), checks length, ...
	RC rc = msgI(		// retrieve and format message, messages.cpp
			 PWRN,		//   error action for problem in msgI
			 buf,		//   temp stack buffer in which to build msg
			 sizeof( buf),	// available buffer space
			 NULL,		//   formatted length not returned
			 mOrH,		//   caller's message text or message handle
			 ap );		//   ptr to args for vsprintf() in msgI

// issue message with caller's msg embedded.  Uses another MSG_MAXLEN bytes of stack.
	err( PWRN,				// PWRN: is program (internal) error; get keypress.
		 (char *)MH_R1200, 		// "[Internal error ]re virtual reports, file '%s', function '%s':\n%s"
		 file, fcn, buf);		//  err(PWRN) now supplies "Internal error: ", 1-92
	return rc;
}		// vrErrIV
//---------------------------------------------------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions to unspool virtual reports to report output files
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEVAIDS		// define for debug aid messages in little-tested paths; undefine later. 11-91.


//------------------------------ LOCAL DATA ---------------------------------

LOCAL int vrBodyLpp = 54;	// lines per report body page, from cncult:getBodyLpp()

//---- UNS and uns[]: info on unspool active destination files

const int UNSSZ=16;	// max # report files to write at once / dimension of uns[].
					// 1, 3 tested.  Production value 16 to 32.
struct UNS
{
	const char* fName;	// file name
	XFILE* xf;		// open file info for xiopak.cpp calls
	int optn;		// option bit(s): VR_FMT (vrpak.h) on if page formatting on for this file
	int* vrhs;		// ptr to 0-terminated list of handles (in a caller's arg or arg list)
	int vrh;		// current/next handle
	ULI o1, o2;		// start and end+1 file offsets of vr's data in spool file, copied from spl.vr[vrh]
	int fStat;		// out file status: -2: ready for next out file (start or prior closed);
					//  -1: set up, ready to open;  0: ok, unspooling vr's;
					//  > 0: ready to close and set to -2: 1: completed ok, 2: error (suppress final page formatting).
	int hStat;		// vr 'vrh' status: 0: start of vr not yet found; 1: unspooling; 2: done, can go to next vrh.
	int did;		// nz if did a vr this pass (hStat/vrh may be set for next vr)
	int atTopPage;	// non-0 if header for current page has not been output
	int pageN;		// 1-based page number for page-formatted output files
	int row;		// 0-based line # on page
	int col;		// 0-based col # in line
	int ufi;		// subscript in uf[] (next) for accessing persistent pageN/row/col
};
LOCAL UNS uns[UNSSZ];   // info on report files to which virtual reports are being unspooled
LOCAL UNS* unsMax;		// ptr to last+1 used uns[] entry: loop limit


/*---- USEDFILE and uf[]: info on used unspool destination (report/export) files,

			retained thru clear, to 1) append if file used in later run. 2) continue page # sequence. */
struct USEDFILE
{
	const char* fName;	// pointer to file name (with path if entered by user), in dm (heap)
	int nUses;			// number of times unspooled to: incremented when opened; 0 if erased but not opened.
	int pageN;			// 1-based page number
	int row;			// 0-based line # and col in page body, excludes hdr/topM,
	int col;			// .. but if 0, assume page's hdr not printed yet.
};

LOCAL int nUf = 0;			// number of uf[] entries in use
LOCAL int nUfAl = 0;		// number of uf[] entries allocated
LOCAL USEDFILE * uf = NULL;	// ptr to dm array of info on used files: .fName (to not overwrite); .pageN/.row/.col.


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL int runRange( void);
LOCAL int testRange( UNS* u, int cs);
LOCAL int vruNuHan( UNS* u);
LOCAL int vruNuf( UNS* u);
LOCAL int vruOpen( UNS* u);
LOCAL RC  unsRun( UNS* onlyU);
LOCAL int vrEvent( UNS* onlyU, ULI *pNxEv);
LOCAL RC  endVrCase( UNS* u);
LOCAL int getWholeText( UNS* onlyU);
LOCAL RC  vruOut( UNS* u, char* s);
LOCAL RC  vruHeadIf( UNS* u);
LOCAL RC  vruFoot( UNS* u);
LOCAL RC  vruWrite( UNS* u, const char* s, int n=-1);
LOCAL RC  vruFlush( UNS* u);
LOCAL int ufLook(const char* fName);
LOCAL int addUsedFile( const char* fName);

//---------------------------------------------------------------------------------------------------------------------------
void vrpak2Clean()		// clean local variables, called from vrpak.cpp, 10-93.
{
	vrBodyLpp = 54;				// lines per page: restore data-init value

	for (UNS* un = uns;  un < unsMax;  un++)	// loop uns[]: array of unspool info
	{
		dmfree( DMPP( un->fName));  			// free file name string
		xfclose(&un->xf);  				// close file if open. believe NULL if not open; xfclose leaves NULL.
	}
	memset( uns, 0, sizeof(uns));			// zero out all the uns[] info

	if (uf)							// if allocated
		for (USEDFILE *aUf = uf;  aUf < uf + nUf;  aUf++)	// loop over used file info in dm block
			dmfree( DMPP( aUf->fName));				// free dm filename string if any
	nUf = 0;							// say no uf entries in use
	nUfAl = 0;							// say no uf info allocated
	dmfree( DMPP( uf));						// free uf block, set ptr NULL. dmpak.cpp.
}			// vrpak2Clean
//---------------------------------------------------------------------------------------------------------------------------
void vrInfoClean(  		// clean up contents of a VROUTINFO if needed, e.g. at fatal error exit. 10-93.

	VROUTINFO *vo)		// pointer to variable number of VROUTINFO's, followed by a NULL (even when static!).
// each VROUTINFO is itself variable length, terminated by 0 vrh.

// limitation: if other code has free'd filenames in middle of block, block may will seem to be terminated prematurely.

{
	// called internally in vrUnspool (next); 2 calls in cnguts.cpp:cgClean. 10-93.

	if (!vo)
		return;				// nop if NULL pointer passed
	do						// assume at least one entry (?)
	{
		if (vo->fName)  				// if non-NULL,
			dmfree( DMPP( vo->fName));		// free filename and set ptr NULL. dmpak.cpp.
		while (vo->vrhs[0])			// loop over 0-terminated variable array at end of structure
		{
			vo->vrhs[0] = 0; 			// clear virtual report handle (?)
			IncP( DMPP( vo), sizeof( int));	// increment struct ptr by sizeof(vrh) for each non-0 vrh
		}					// ... so when get to 0 vrh, vo+1 points at next entry
		vo++;					// now point to next entry, starting with filename ptr, else to a NULL.
	}
	while (vo->fName);				// if no filename ptr, assume end of block
}			// vrInfoClean
//---------------------------------------------------------------------------------------------------------------------------
RC vrUnspool( VROUTINFO* voInfo)

// unspool 1 or more virtual reports to each of 1 or more report output files.

/* voInfo format: block containing one or more VROUTINFO structs (vrpak.h) with NULL after last one.
                  Note that each ends with variable length 0-terminated list of virtual report handles;
                  thus whole block ends with 0, NULL.

   VROUTINFO.optn bits (vrpak.h) for each output file:

	VR_OVERWRITE:  overwrite any existing file; else append.
    VR_FMT:        on if page formatting is on for this file.
		       if on, for complete formatting, each vrh must also have been written with formatting on.
		       if off, any page formatting in spool file is omitted from report. */

/* CAUTION: dmfree's filename strings in voInfo after closing each file.
	    must be private strsave'd copy; don't attempt to reUse voInfo. */
{
	vrBodyLpp = getBodyLpp();	// get best-yet-avail report body lines per page to file-global:
    							//  if Top.bodyLpp not yet set (eg if spooling an input-time err Msg),
    							//  may return Topi.bodyLpp or a default value.
	if (spl.vrRc)				// fail now if prior serious error, incl during writing vr's to spool file
		return spl.vrRc;		// error that should terminate virtual reports has occurred
	if ( voInfo==NULL				// if NULL info pointer
	 ||  voInfo->fName==NULL )			// or no reports requested (empty info block)
		return RCOK;				// exit now
	if (!spl.isInit || spl.sp_nVrh==0)
#ifdef DEVAIDS				// else don't bother the user -- possible consequence of other errors.
		return vrErr( "vrUspool", (char *)MH_R1210);	// "Unspool called but nothing has been spooled"
#else
		return RCBAD;				// silent bad?
#endif
	if (!spl.buf1)   				// if no buffer now allocated (unexpected -- used during spooling)
		if (vrBufAl())				// allocate spool read buffer. inits spl members bufSz, buf1, buf2, p, p1, p2.
			return spl.vrRc;			// out of memory
	spl.voInfo = voInfo;			// store arg ptr for access by vruNuf. Is incremented as used.
	VROUTINFO* origVoInfo = voInfo;		// save copy for cleanup at end of this fcn 10-93
	spl.uN = spl.nFo = spl.maxFo = 0;   	// init spl members
	memset( uns, 0, sizeof( uns));		// zero unspool output files info array

// open output files and init uns[] entries till full, out of args, or out of file handles
	UNS *u;
	for (spl.uN = 0; spl.uN < UNSSZ; spl.uN++) 	// ... spl.uN becomes # uns[] entries in use
	{
		u = uns + spl.uN;
		u->fStat = -2;			// say ready for a new file
		if (vruNuf(u))			// init *u and open next file in arg list & set up its 1st vr vrh / 0 ok
			break;				// stop without uN++ if out of files (1) or file handles (2)
	}
	unsMax = uns + spl.uN;
#ifdef DEVAIDS		// devel aid because could really occur if user deleted
					// default reports but not Primary report file, 11-91,
					// or other errors made all vr's empty 11-20-91. */
	if (spl.uN==0)
		vrErr( "vrUnspool", (char *)MH_R1211);	// "After initial setup, 'spl.uN' is 0". Bug, or call with empty files only
#endif

// do passes (runs) over spooled data until all requested reports completely written

	do
	{
		if (runRange())		// choose vr(s) to do at once, set spl.runO1,2 (used by vrBufLoad), 0 ok.  below.
			break;		// 1 can't find any (bug, add devel aid msg?)  2 vrUnspool complete.
		if (vrBufLoad())		// read data, or start of it, into buffer if not there.  RCOK ok.  vrpak3.cpp
			break;		// serious error eg i/o error on spool file (vrRc set).
		if (unsRun(0))		// scan data in buffer, writing vr data to output files per info in uns[].
			// at end of a vr's data, advances that uns[] to its next vr, and does one too that if possible.
			break;		// serious error eg i/o error on spool file (vrRc set).

		// devel aid checks
		int did;
		for (did = 0, u = uns;  u < unsMax;  u++)
		{
			did |= u->did;		// at least 1 should be non-0
			u->did = 0;			// clear for next pass
			if (!u->fStat)		// if file being written (0), not complete, handle status...
				if (u->hStat)		// shd be 0 (looking for start) not 1 (unspooling) or 2 (ended & failed to set up next)
					vrErr( "vrUnspool",
						   (char *)MH_R1212, 		// "At end pass, uns[%d] hStat is %d not 0"
						   u - uns, u->hStat );
		}
		if (!did)
		{
			vrErr( "vrUnspool", (char *)MH_R1213);	// "At end of pass, all uns[].did's are 0"
			break;					// don't loop forever after error 12-5-94
		}
	}
	while (!spl.vrRc);		// repeat til break (after runRange call) or serious error

// final checks

	if (!spl.vrRc)						// omit if error
	{
		if (spl.voInfo->fName != NULL)
			vrErr( "vrUnspool", (char *)MH_R1214);		// "Unspool did not use all its arguments"

		for (u = uns;  u < unsMax;  u++)
		{
			if (u->fStat != -2)		// shd be -2 (ready for next, but aren't any),
	          						// not -1: ready to open, 0 open, nor 1 done, ready to close).
				vrErr( "vrUnspool",
					   (char *)MH_R1215,   		// "At end of Unspool, uns[%d] fStat is %d not -2"
					   u - uns, u->fStat );
			if (u->hStat != 2)				// shd be 2, ready for next (but aren't any more)
				vrErr( "vrUnspool",
					   (char *)MH_R1216, 			// "At end of Unspool, uns[%d] hStat is %d not 2"
					   u - uns, u->hStat );
		}
	}

	// terminate

	vrInfoClean( origVoInfo);		/* now delete dm filenames in input info, 10-93.
						   Deferred til here cuz vrInfoClean cannot distinguish end of info from
						   NULL file ptr before end, eg cleanup call upon fatal error exit. */

	return spl.vrRc;			// non-0 if any serious error such as i/o error on spool file
}				// vrUnspool
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int runRange()

// endtest / set spl.runO1,2 to spool file offset limits of one or more overlapping vr's in uns.

/* chooses range to optimize unspooling: make maximum use of data in buffer;
   separate non-overlapping vr's so can reorganize in the "gap" considering new vrh's that have replaced completed ones. */


// returns 0 ok, 1 no files sucessfully open in uns[] (no handles or bug), 2 no more vr's to process (vrUnspool complete)
{
	UNS* u;
	int found1 = 0;		// nz if vr(s) found
	int cs;				// case/criterion: increase till matching vr(s) found:
	       				//  1) completely in buffer, 2) start in buffer, 3) overlaps buffer, 4+) any vr

	cs = (spl.p1 < spl.p2 + 512) ? 0 : 4;		// start at case 4 if little or no data in buffer
	spl.runO1 = 0x7fffffff;						// init large to search for smallest start

// First, search for strictest criterion for which vr exists; if tie, use smallest start offset

	for (  ;  cs <= 4;  cs++)				// loop criteria till vr found
	{
		for (u = uns;  u < unsMax;  u++)			// loop uns[]: find best entry for this cs
		{
			if (u->fStat || u->hStat)			// consider only entries with out file & input vr handle ready
				continue;
			if (testRange( u, cs))			// if u meets criteria (is in buffer, etc)
			{
				if (u->o1 < spl.runO1)			// if first or starts sooner
				{
					found1 = 1;				// say have a vr at this cs level
					spl.runO1 = u->o1;
					spl.runO2 = u->o2;	// save its spool file offset range
				}
			}
		}
		if (found1)  break;
	}

// Second, expand range to include any adjacent or overlapping vr's that meet same criterion

	/* believe that if "catch-up" thing in unsRun works, criteria here could be relaxed to include any vr's
	   that start between spl.runO1 and spl.runO2 -- might save time scanning text without increasing disk i/o
	   Hmm... didn't I code unsRun / unsEndtest such that will continue into any trailing overlapping runs anyway? */

	if (found1)						// if found any
	{
		int hit;
		do						// repeat while find more
		{
			hit = 0;
			for (u = uns;  u < unsMax;  u++)   		// loop uns[]
			{
				if (u->fStat || u->hStat)   		// consider only entries with out file & input vr handle ready
					continue;
				if (u->o2 >= spl.runO1  &&  u->o1 <= spl.runO2	// use only u's with adjacent or overlapping range
						&& testRange( u, cs) )				// ... that meet same criterion re buffer
				{
					// needn't check for o1 less as first loop chose smallest o1 for cs.
					if (u->o2 > spl.runO2)			// if ends later
					{
						spl.runO2 = u->o2;			// extend pass to include it
						hit++;				// say extended range: need another iteration here
					}
				}
			}
		}
		while (hit);
	}

// Finally, check if doable runs found

	if (found1==0)					// if found no vr's ready to go
		if (spl.voInfo->fName==NULL)			// if there is no more input, must be done
		{
			// poss addl check: scan uns for all fStat's == -2
			return 2;
		}
		else
			// presumably there are some fStat's == -1 (ready to open)
			// fStat == 0 (open) or > 0 (done) means caller didn't do vruNuHan / vruNuf.
			return 1;				// could not open any files, or something is wrong

	return 0;					// normal good return, o1 and o2 set.
}		// runRange
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int testRange(
	UNS* u, 	// test if u's spool file data --
	int cs )	// 1: is completely in buffer; 2: starts in buf, 3: overlaps data in buf, 4/other: (returns true) */
{
	switch (cs)
	{
	case 1:
		if (u->o1 >= spl.bufO1  &&  u->o2 <= spl.bufO2)  return 1;
		break;
	case 2:
		if (u->o1 >= spl.bufO1  &&  u->o1 <= spl.bufO2)  return 1;
		break;
	case 3:
		if (u->o2 >= spl.bufO1  &&  u->o1 <= spl.bufO1)  return 1;
		break;
	//case 4:
	default:
		return 1;
	}
	return 0;
}			// testRange
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int vruNuHan( UNS* u)		// get next input vr handle in uns entry

// call after getting a new file and whenever a handle completes.
// returns 0 done (hStat 0), 1 no more (good) handles for this output report (fStat set to 1).

{
// if file ready for a new handle, open next good handle

	if ( u->fStat==0 			// if output file open -- else not using handles (insurance)
	 &&  u->hStat==2 )			// if ready for a new handle (insurance)
		for ( ; ; )				// bad handle retry loop
		{
			if (*u->vrhs==0)		// if no more vr handles to be unspooled to this report
			{
				u->fStat = 1;		// say output file complete, ready to be closed
				return 1;			// say no more handles for this file
			}
			int vrh = *u->vrhs++;				// fetch next handle, point past it
			VRI* vr = spl.sp_vr + vrh;

			// check handle
			if (vrh <= 0 || vrh > spl.sp_nVrh)		// check for valid handle
				vrErr( "vruNuHan", (char *)MH_R1217,	// "Out of range vr handle %d for output file %s in vrUnspool call"
					   vrh, u->fName );	// ... and iterate to try next handle
			else if (vr->o2 <= vr->o1)			// check that vr contains data
				;						// continue;	no msg: empty vr's can normally occur.
			else				// good handle
			{
				// conditional form feed between vr's
				int row1 = (vr->pageN > 1) ? vr->row1 : vr->row;	// # lines on 1st page of vr being opened
				if ( u->optn & VR_FMT				// if output file is formatted
						&&  !u->atTopPage					// if prior report does not end with page feed
						&&  row1 + u->row >= vrBodyLpp )			// if combined page would be too long
					vruOut( u, "\f");				// output form feed to file. error sets vrRc.

				// open the handle
				u->vrh = vrh;					// store handle
				u->hStat = 0;					// say looking for start of vr data for this vrh
				u->o1 = vr->o1;					// fetch vr's spool file data offset limits
				u->o2 = vr->o2;					// ... for convenience in selecting run range
				u->optn = (u->optn &~VR_FINEWL) | (vr->optn & VR_FINEWL);	/* get vr's "add final newline" option bit
	     								   for use in vrEvent at end of report */

#ifdef DEVAIDS					// since output will occur & problem be visible, don't bother user
				// debug aid check. output report regardless. remove if can properly occur.
				if ( u->optn & VR_FMT
						&&  !(vr->optn & VR_FMT) )
					vrErr( "vruNuHan",
						   (char *)MH_R1218,			// "Unformatted virtual report, handle %d, name '%s' \n"
						   vrh, vr->vrName, u->fName );	// "in formatted output file '%s'.  Will continue."
#endif
				return 0;				// expected good return with next handle
			}
		}
	return 0;					// good return for (unexpected) unnecessary call
}		// vruNuHan
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int vruNuf( UNS* u)		// open next caller's report file in uns entry, at start or when prior done

// call during setup and whenever a file completes.

// in initial setup: caller increments uN only if successfully opened -- ie backs off of u for which out of file handles

// returns 0 done ok; 1 no more files; 2 out of dos file handles.
{

// close completed report, set up and open next valid report output file

	for ( ; ; )					// file open retry loop -- eg if invalid file name or no good handles
	{
		VROUTINFO * voInfoWas = spl.voInfo;		// save to put back if out of file handles

		// close completed file if any.  Includes iterating after opening file then finding no good handles for it.

		if (u->fStat > 0)  			// if u contains completed file ready to close
		{
			// conditional final form feed
			if ( u->optn & VR_FMT			// if a page-formatted output file
					&&  u->row > 0 )		// if not at top page (set in vrh close code in vrEvent)
				vruOut( u, "\f");			// write form feed. writes if fStat is 1, nop if 2. sets vrRc on error.
			// put page position into uf[] for use if report appended to this file in a later run of session
			USEDFILE *f = uf + u->ufi;			// point uf[u->ufi] (at top of this file)
			f->pageN = u->pageN;				// store position info for next opening
			f->row   = u->row;
			f->col   = u->col;
			// conditionally update "primary report file" info, re final end-of-run appending (after report files input records cleared).
			if (!_stricmp( u->fName, PriRep.f.fName))	// if primary report file file for which cncult.cpp saved info
			{
				// PriRep: cnguts.cpp/h.
				PriRep.f.optn &= ~VR_OVERWRITE;		// append, don't overwrite!
			}
			// close file
			vruFlush(u);				// write any text in its XFILE.buf[]fer (unless fStat is 2), below.
			xfclose( &u->xf, NULL);		// xiopak.cpp. leaves u->xf NULL.
			spl.nFo--;				// reduce our count of open output files
			u->fStat = -2;			// say u is ready to be set up for next output file
			// free file name string now -- gotta clean up somewhere and don't have C++ destructor.
			dmfree( DMPP( u->fName));			// dmpak.cpp: free block *arg and set pointer NULL.
		}

		// stop here if max # files open (per prior open failure)

		if (spl.maxFo && spl.nFo >= spl.maxFo)
			return 2;				// max # files already open

		// set up u for next report output file from vrUnspool's arg list

		if (u->fStat==-2)			// if u ready for a new file
		{
			int nVrh;
			for ( ; ; )				// repeat if 0 vr's in file
			{
				u->fName= spl.voInfo->fName;		// fetch next filename ptr from arg list
				if (u->fName==NULL)  			// NULL for filename ptr indicates end of caller's arg list
					return 1;					// no more input
				dmIncRef( DMPP(u->fName));		// ++ reference count of copied name cuz will be free'd both places,
             									// in unpredictable order in error cleanup cases. 10-93.
				u->optn = spl.voInfo->optn;				// fetch option bits word: VR_FMT, VR_OVERWRITE,
				u->vrhs = spl.voInfo->vrhs;				// set ptr to handles list
				// add or access entry in used files table
				u->ufi = addUsedFile(u->fName);				// add and init entry, or incr nUses if found, ret subscr
				USEDFILE *f = uf + u->ufi;					// point to entry
				u->pageN = f->pageN;					// fetch page number, 1 if new entry.
				u->row = f->row;  						// fetch row and col, presumably 0.
				u->col = f->col;  						// ..
				u->atTopPage = (u->row | u->col)==0 ? 1 : 0;		// assume current page head not output if at row 0, col 0.
				if (f->nUses)						// if file previously written to this session
					u->optn &= ~VR_OVERWRITE;				// force append (redundant insurance): cncult shd handle.
				// count vrh's / point past VROUTINFO
				for (nVrh = 0; spl.voInfo->vrhs[nVrh]; nVrh++)	// count non-0 handles.  don't count the 0 at end,
					;											// ... as sizeof(VROUTINFO) includes .vrh[1].
				IncP( DMPP( spl.voInfo), sizeof(VROUTINFO) + nVrh * sizeof(int));	// point past VROUTINFO just used:
				// pass empty vrh's to determine if have any output for file
				for ( ;  nVrh > 0;  nVrh--, u->vrhs++)	// advance over empty vr eg ERR or LOG report
				{
					VRI *vr = spl.sp_vr + *u->vrhs;
					if (vr->o2 > vr->o1)			// if this vr contains data
						break;				// do not advance (more)
				}
				// skip files to receive no virtual reports here, so 0-length files are not created. But do conditionally erase file.
				if (nVrh==0)				// if file is to receive no reports
				{
					if (u->optn & VR_OVERWRITE)		// if file is to be overwritten (only on 1st run using file in session)
						xfdelete( u->fName, IGN);		// erase any version from a prior session.
					dmfree( DMPP( u->fName));		// free the fileName in dm
					continue;				// done with this file, set up the next one.
				}
				break;				// have a file, leave erase&skip-empty-files loop
			}
			u->fStat = -1;   		// say ready to open file
			u->vrh = 0;				// insurance
		}

		// open file

		if (u->fStat==-1)			// insurance: protects against unexpected call with 0.
		{
			int fail = vruOpen(u); 	// do xfopen, special no-msg return if out of file handles, below
			if (fail)				// 0 ok, 2 too many files, 1 other
			{
				u->fStat = -2;			// say u is ready for next file
				if (fail==2) 			// if out of file handles
				{
					spl.voInfo = voInfoWas; 	// put back arg to use when next handle avail even if in a different u
#ifdef DEVAIDS	// debug aid, remove. Not an error, just likely to have bugs at first.
					logit( DASHES, (char *)MH_R1223,	// "Debugging Information: Number of simultaneous report/export output \n"
						   spl.nFo );					// "  files limited to %d by # available DOS file handles.  Expect bugs."
#endif
					if (spl.nFo >= 2)		// if opened only 0 or 1 files don't set limit: so few we need to push
						spl.maxFo = spl.nFo;  	// save as limit on # dos files open so won't keep banging on it
					return 2;			// say can't open as out of handles
				}
				// else: other create error, such as bad file name, message issued by vruOpen.
				continue;			// try NEXT file. note fStat is -2 and spl.voInfo is NOT backed up.
			}
			u->fStat = 0;			// say file opened and in use ok
			u->hStat = 2;			// say ready for next handle
			uf[u->ufi].nUses++;		// say file has been used
			spl.nFo++;			// count files open

#ifdef OUTPNAMS //cnglob.h. set for WIN, DLL versions (or as changed). 10-8-93.
			if (uf[u->ufi].nUses <= 1)  	// if first use of this file
				saveAnOutPNam(u->fName);	// save its name for return to calling program. cse.cpp, decl cnglob.h.
#endif
		}

		// set up first (good) virtual report handle for this file

		if (vruNuHan(u))		// nop if hStat not 2. checks handles, advances to a good one, nz if can't find one
			continue;		// failed: bad/no handles.  Iterate to close this file, open next file.
		break;			// completed ok
	}
	return 0;			// normal good exit
}		// vruNuf
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int vruOpen( UNS* u)		// open report output file for virtual report unspooling

// callers must issue error message for pre-existing files if desired.

// does xfopen to append or overwrite per u->optn, suppressing message if out of DOS file handles.
// does not set vrRc: after error, caller skips file and attempts to proceed.

// returns ok: 0, *xf set.
//         too many files open: 2, no message.
//         other error: 1, xiopak-style message issued.
{
	SEC sec;		// receives "system error code" if open error.  NB errno itself is cleared b4 xfopen return.

	u->xf = xfopen( u->fName,
					u->optn & VR_OVERWRITE 				// access per VROUTINFO optn bit
						? "wb"
						: "ab",
					IGN 						// no msg at open (changed below for later calls)
					| XFBUFALLOC,				// alloc buffer -- XFILE.buf[],.busz,.bufn. see vruWrite.
					FALSE, 					// eof is not an error on this file
					&sec );					// receives code from 'errno'

// handle error

	if (!u->xf)				// if failed
	{
		if (sec==EMFILE)			// if error was "too many files open" (errno.h)
			return 2;			// give unique return, w/o message, for this special application

		// issue message for other errors, as though PWRN (WRN would be better) not IGN had been given in xfopen call.
		// mimics code in xioerr() as called from xfopen().
		vrErr( "vruOpen", 			// display internal error msg, below, calls "err" in rmkerr.cpp
			   "I0012: IO error, file '%s':\n    %s",	// Note use of xiopak msg handle.
			   u->fName,   			// filename
			   msgSec(sec) );		// text for sec, eg "no such file or directory (sec=2)", messages.cpp
		return 1;
	}

// success. Change xfile error action so any later errors get normal messages
	u->xf->xferact = WRN;

	return 0;			// normal ok return
}			// vruOpen
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC unsRun(		// guts of pass over spool file.

	UNS* onlyU )			// normally 0, or ptr to only uns entry to process in new vr nested catch-up call

// returns non-RCOK if unspooling should end, e.g. on spool file i/o error.
// (errors re single handles or out files do not cause bad return; item is skipped and run continues after message.)

{
	// called from --- and from endVrCase from self.

	UNS* u0, * u, * un;
	SpoolTy ty;
	int vrh, txLen = 0;
	ULI nxEv;

	// onlyU must disable anything that could alter buffer or pointers, including reading more text.

#if 0	// ifdef DEVAIDS	// debug aid, remove. 11-27-91: seen several times without problem in 1-2 month runs.
x    if (onlyU)
x        logit( 	// msg to screen & log report, rmkerr.cpp  (may not appear in log report since issued during unspool)
x               DASHES, "Debugging Information: using virtual report catch-up run \n"
x                       "  for vr %d, '%s', to file '%s'.  Expect bugs.",
x                       (INT)onlyU->vrh, spl.vr[onlyU->vrh].vrName, onlyU->fName );
#endif

	if (onlyU)					// set limits for inner loop below:
		u0 = onlyU, un = onlyU + 1;		//   if special call to do only one u, set so 'for' loops once only
	else
		u0 = uns, un = unsMax;			//   set to loop over all uns[]

	nxEv = 0L;			// file offset of next "event" (vr start or end): 0 causes initial vrEvent call to set up.
	for ( ; spl.vrRc==RCOK; )
	{
		// only when file offset "nxEv" is reached, process vr starts and ends and do endtest and determine new nxEv.

		if (spl.bufO1 + (spl.p - spl.p1)  >=  nxEv)	// spool file offset at buffer ptr : next event offset
			if (vrEvent( onlyU, &nxEv))			// process "event" and determine next one
				break;							// end of run or serious error

		// if too little text in ram, get more
		if ( (spl.p2 - spl.p) < 1 + sizeof(int))	// if don't have type and handle.  Text read only when needed.
		{
			if (onlyU)				// normal termination of nested call
				break;
			if (spl.bufO2 < spl.spO)		// unless end file known already in buffer (inf loop possible)
				if (vrBufMore())			// read more into buffer -- vrpak3.cpp.  may delete data up to spl.p.
					break;				// on spool i/o error, terminate run. vrRc set.
			if (spl.p2 - spl.p <= 0  &&  spl.bufO2 < spl.spO)
			{
				spl.vrRc = vrErr( "unsRun", (char *)MH_R1219);		// "Unexpected empty unspool buffer"
				break;
			}
		}

		// read spool item

		ty = (SpoolTy)(ULI)*(UCH *)spl.p++;		// type of next item in buffer; avoid sign extension
		vrh = *(int *)spl.p;
		spl.p += sizeof(int);	// vr handle follows all ty's other than eof
		if (ty==vrBody || ty==vrFormat)			// if text item (body or format)
			txLen = getWholeText(onlyU);			// determine length, get all in buffer, leave p at start of text

		// process item for each output file wanting this virtual report (can be more than one)

		for (u = u0;  u < un;  u++)			// loop over uns[], or just do onlyU
		{
			if (u->fStat || u->vrh != vrh)		// if not open report or not vr being put into this report
				continue;					// skip the uns entry
			switch (ty)
			{
			case vrFormat:		// page formatting text for virtual report (vrh)
				if (!(u->optn & VR_FMT))	// if output file has page formatting off
					break;			// skip this text
				// fall thru

			case vrBody:		// body text for virtual report (vrh), txLen chars at spl.p.
				if (u->hStat != 1)		// if not unspooling vr, ignore
					break;			// (ok -- occurs when changed vrh when p is past start)
				vruOut( u, spl.p);		// output null-terminated text to output report file u->xf.
				break;				// ... sets spl.vrRc if error.

			default: ;			// for lint.  unrecognized spool item type errmsg is below.

			}  // switch (ty)
		}  // for (u=  uns[] loop

		// pass entry in buffer / error message if unrecognized type

		switch (ty)
		{
		case vrBody:
		case vrFormat:
			spl.p += txLen+1;
			break;	  	// pass the text and the terminating null

		default:
			vrErr( "unsRun",
				   (char *)MH_R1220,				// "Invalid type %d in spool file at offset %ld"
				   ty,  spl.bufO1 + (spl.p-spl.p1) );
			while (spl.p < spl.p2  &&  *spl.p < static_cast<char>(vrBody))	// skip to a probable type byte (or to end buf)
				spl.p++;
			break;				// continue here.  but wanna cause bad return or say run is bad?
		}

	}  // for (;;) read loop

	return spl.vrRc;	// normal return.  Set non-RCOK on error that should terminate unspooling,
    					// for example read error on spool file in vrpak3:bufRead.
}			// unsRun
//----------------------------------------------------------------------------------------------------------------------
LOCAL int vrEvent( 		// process unsRun "event" if any and determine file offset of next event

	UNS* onlyU, 		// non-0 if caller unsRun is doing nested one-vr look-back catch-up call
	ULI *pNxEv )    		/* receives spool file offset of next "event" requiring attention here:
    				   next offset at which a vr opens or closes. */

// returns non-0 to terminate run (no open vr's, or serious error)
{
	UNS* u;
	ULI bufOp = spl.bufO1 + (ULI)(spl.p - spl.p1);
	ULI nxEv;

// open & close vr's / count open vrs

	int nou = 0;
	for (u = uns;  u < unsMax;  u++)
	{
		if (onlyU && u != onlyU)
			continue;					// do not consider u's not being processed now
		if (u->fStat)
			continue;					// skip non-open files
		if (u->hStat==0  &&  bufOp==u->o1)		// if not yet opened and at its start, open vr
		{
			u->hStat = 1;
			nou++;					// count open vr's
		}
		else if (u->hStat==1)				// if vr is open
		{
			if (bufOp >= u->o2)    			// if at its end, close it
			{
				// optional blank line at end of report
				if (u->optn & VR_FINEWL)			// if option bit on (set at vrOpen or with vrChangeOptn)
					if (!u->atTopPage)			// if not already at top (formatted) page
						vruOut( u, "\n");			// write crlf, paginate, below.
				// close vrh
				u->hStat = 2;				// say u is ready for next vrh
				u->did++;					// say did a run, this u, this pass.
				// open new vrh else new file
				if (vruNuHan(u))				// set up input next handle for output file / 0 done.  local above.
					vruNuf(u);				// no more vrh's to go into file, so set up next file if any. above.
				//hStat may now be 0 (looking for start); "did" remains nz.
			}
			else 						// vr is open and not fully unspooled
				nou++;					// count open vr's
		}
	}

// run endtest

	if ( nou==0						// if no open vr's
			//&&  bufOp >= spl.runO2  		// and intended range of text scanned -- add if necessary
			||  bufOp >= spl.spO				// or have processed entire spool file
			||  spl.vrRc )					// or serious error
		return 1; 					// say end this run

// conditionally do "catch-up" for any ready-to-open vr's whose data starts b4 current position but after start of spool buffer

	if (!onlyU)						// don't do if already on a "catch-up" call (caller will)
		for (u = uns;  u < unsMax;  u++)
			if (endVrCase(u))				// next
				break;					// serious error, vrRc set

// determine next "event": next opening or closing of vr

	nxEv = spl.spO;					// eof is last possible event
	for (u = uns;  u < unsMax;  u++)
	{
		if (onlyU && u != onlyU)
			continue;					// do not consider u's not being processed now
		if (u->fStat==0)					// if output file open
		{
			if (u->hStat==0  &&  u->o1 > bufOp)		// if vr not open and opens after here
			{
				if (u->o1 < nxEv)				// if file offset smaller
					nxEv = u->o1;				// this event is next event
			}
			else if (u->hStat==1  &&  u->o2 > bufOp)	// if vr is open its o2 is an event to consider
			{
				if (u->o2 < nxEv)
					nxEv = u->o2;
			}
		}
	}
	*pNxEv = nxEv;

	return spl.vrRc;
}				// vrEvent
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC endVrCase( UNS* u)	// called from unsRun after end of vr u advanced to next vrh/file if any

/* if the new vr starts in buf but b4 current posn, do nested "catch-up" call to unsRun so vr can be included
   in current run, possibly eliminating need to re-read its data from disk later. */
{
	char didSave, nesDid;
	char*  pSave, * p2Save;

	do
	{
		if ( u->fStat		  			// if file not open
				||  u->hStat					// if handle not ready to open
				||  u->o1 < spl.bufO1   			// if this vr starts before start of data in spool buffer
				||  u->o1 >= spl.bufO1 + (spl.p - spl.p1)	// or after current position
				||  spl.vrRc )					// or serious error has occurred
			break;					// then catch-up call not applicable, return to caller

		didSave = u->did;
		pSave = spl.p;
		p2Save = spl.p2; 				// save unspool buffer pointers
		spl.p2 = spl.p;
		spl.p = spl.p1 + (ULI)(u->o1 - spl.bufO1);  	// set to process from start vr to curr posn only
		u->did = 0;
		unsRun(u);					// re-scan text to curr pos doing this one u only
		// nb 'onlyU' disables any buffer read or move.
		spl.p2 = p2Save;
		spl.p = pSave;			// restore buffer ptrs to continue this pass
		nesDid = u->did;					// nz if found start of vr for u->vrh (expected)
		u->did = didSave;				// restore
	}
	while (nesDid				// repeat if vr started (expected)
			&& u->hStat==0);			// and completed before spl.p: see if next vr can also use a catch-up call
	/* (hStat 0 indicates handle ready to start; with 'did' nz this
	   implies that handle was finished and next one set up.)
	   (hStat==0 is redundant check as tested at loop top) */
	return spl.vrRc;
}			// endVrCase
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int getWholeText( UNS* onlyU )		// read entire text into buffer, return its length.  Alters spl.p.

// called when spl.p is after type/vrh, before text itself.

// returns length of text
{
	int txLen;
	for ( ; ; )				// loop in case one read doesn't get it all: read length depends on buf spc, o2, etc.
	{
		txLen = strlen(spl.p);
		if (spl.p + txLen < spl.p2)				// if all in ram ( < to include null IN buffer)
			break;
		if (spl.bufO2 >= spl.spO)				// if there is no more text to read
		{
			vrErr( "getWholeText", (char *)MH_R1221);	// "text runs off end spool file"
			txLen = (spl.p2 - spl.p) - 1;				// termination problem.  truncate & continue.
			break;
		}
		if ( ULI(spl.p2 - spl.p) > spl.bufSz - 1024)		// min buf size - 1024 would be better -- uniformity
		{
			vrErr( "getWholeText", (char *)MH_R1222);		// "text too long for spool file buffer"
			txLen = (spl.p2 - spl.p) - 1;	// truncate
			break;							// expect "bad type" errors during recovery
		}
		if (onlyU)				// if catch-up call for new run in onlyU, can't read: might change p
			break;				// and should not occur anyway.  internal error message?
		spl.p -= 1 + sizeof(int);    	// adjust p to beginning of entry so data left in buffer is reusable
		if (vrBufMore())				// append more text to end buffer; may delete text up to p.
			break;				// read error (other than eof) on spool file
		spl.p += 1 + sizeof(int);   	// restore p to beginning of text, after type/handle.
	}
	return txLen;
}			// getWholeText
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vruOut( UNS* u, char* s) 	// output text to report file with optional page formatting

// if a page-formatted file, supplies header at top each page and footer at bottom each page (form feed / enuf lf's).

// error handling: caller may ignore non-RCOK return if he checks spl.vrRc and u->fStat's regularly.  See vruFlush.
{
	if (spl.vrRc)			// if there has been a serious error (eg full disk)
		return spl.vrRc;			// return now.
	// Error in one file proceeds here -- vruWrite nops to that u, works for other u's.
	while (*s)
	{
		int n = strcspn( s, "\r\n\f");		// n = # chars that require no special processing
		if (n)
		{
			vruHeadIf(u);				// if atTopPage, output page header if formatted file, & clear atTopPage
			vruWrite( u, s, n);			/* write n chars to report output file.  No error check needed here --
						   error sets u to nop further writes, and sets spl.vrRc if disk full.*/
			u->col += n;				// account for motion of print head
			s += n;				// point past characters output
		}
		else					// have a control of interest to us
		{
			switch (*s++)		// CAUTION: cases must match "text" in strcspn above
			{
			case '\r':			// cr: test for cr and lf together

				if (*s != '\n')    		// if cr without lf
				{
					vruHeadIf(u);			// if atTopPage, output page header if formatted file, & clear atTopPage
					vruWrite( u, "\r", 1);   	// output carriage return only
					u->col = 0;			// say carriage is at left
					break;
				}
				s++;				// treat crlf as newline: check for end page
				// fall thru for crlf in file

			case '\n':			// newline (or line feed; only gets here if no preceding \r in same string)

				vruHeadIf(u);			// conditional page header
				vruWrite( u, "\r\n", 2);		// output crlf to output file
				u->col = 0;
				u->row++;		// update posn
				if ( u->row < vrBodyLpp		// if page not full (if can still print a row)
						||  !(u->optn & VR_FMT) )    	// or if file not page-formatted
					break;				// done
				// fall thru to end full page in page-formatted file

			case '\f':			// form feed

				if (u->atTopPage)			// ignore redundant form feed -- possible due to multiple paginators.
					break;				/* e.g. when we page due to newline count, next char in file
						   should be \f from vrpak1 paging at same newline count.
						   Also may be 1st char in new vr after vruNuHan put \f between vr's. */
				vruHeadIf(u);			// conditional page header
				if (u->col != 0)     		// if not at left ("impossible" here: vrpak1 crlf's b4 \f)
				{
					vruWrite( u, "\r\n", 2);  	// return carriage first
					u->row++;  //u->col = 0;	col not used
				}
				vruFoot(u);   			// if page-formatted file, space down and do footer.  uses u->row.
				vruWrite( u, "\f", 1);  	 	// write form feed
				u->row = 0;
				u->col = 0;		// say at top of
				u->pageN++;			// .. next page
				u->atTopPage++;			// .. say header for that page needed, when 1st char found on page
				break;				// .. (don't do header here in case \f is last char in file).

			default: ;		// satisfy lint
			}		      /* switch *s */
		}		    /* if - else */
	}		 	/* while *s */

	return spl.vrRc;				// set (in vruWrite) if disk is full
	// another return above
}				// vruOut
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vruHeadIf( UNS* u)
{
	if (u->atTopPage)
		if (u->optn & VR_FMT)
			vruWrite( u, getHeaderText(u->pageN));
								// get top margin and header text (cgrpt.cpp) and output it.
								// don't count lines: already subtracted from bodyLpp.
	u->atTopPage = 0;
	return spl.vrRc;
}			// vruHeadIf
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vruFoot( UNS* u)
{
	if (u->optn & VR_FMT)
	{
		while (u->row <= vrBodyLpp)		// space down to row  bodyLpp + 1,  ie write at least 1 blank line.
		{
			vruWrite( u, "\r\n", 2);
			u->row++;
		}
		vruWrite( u, getFooterText(u->pageN));
							// get footer text (cgrpt.cpp) and output it.
							// don't count lines: already subtracted from bodyLpp.
	}
	return spl.vrRc;
}			// vruFoot
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vruWrite(		// inner fcn to write text to report file for vruOut
	UNS* u, const char* s, int n/*=-1*/)
// error handling: see vruFlush; RCOK if ok.
{

	if (n < 0)		// <0 length means use strlen
		n = strlen(s);

	while 			// loop to fill and write buffer, normally exits at "break" below.
	( spl.vrRc==RCOK    	// while no serious error
			&&  !(u->fStat & ~1) )	/* and fstat is 0 (open) or 1 (closing, no error).
				   Note if fStat is 2, we do no (further) output and return RCOK, so no further
				   errors after error even tho further calls can occur for final page footer. */
	{

		// put text in buffer.
		// buffer is in XFILE, alloc'd per XFBUFALLOC option in xfopen in vruNuf above.  xf.bufn is buffer index.

		XFILE *xf = u->xf;
		int spaceInBuf = xf->bufsz - xf->bufn;
		int moveNow = min( n, spaceInBuf);
		if (moveNow)
			memcpy( xf->xf_buf + xf->bufn, s, moveNow);
		n -= moveNow;
		s += moveNow;
		xf->bufn += moveNow;
		if (n==0)					// endtest
			break;					// normal good exit from loop

		// more text than will fit buffer.  Write buffer and iterate.

		vruFlush(u);				// write buffer contents (next).  changes u->fStat and/or spl.vrRc on error.
		// 'while' above terminates loop if error.
	}
	return spl.vrRc;				// error return; can return RCOK if error on this file only
}			// vruWrite
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC vruFlush( UNS* u)	// write output buffer for report output file

// call when full (from vruWrite) and at close (from vruNuf).

/* errors: one-file error (bad DOS handle): flags file as complete (sets u->fStat to 2), making further calls nop;
		calling code will advance that u to next file in due course.
	   serious error (disk full): also sets spl.vrRc, terminating unSpool. */
{
	SEC sec;
	XFILE *xf;

	if (spl.vrRc != RCOK	// if there has been a serious error
			|| u->fStat & ~1 )		// or fstat not 0 (open) or 1 (closing, no error)
		return spl.vrRc;		// no write.
	// in particular, prior error on this file (fStat==2) nop's and returns RCOK.

// write buffer.  buffer is in XFILE, alloc'd per XFBUFALLOC option in xfopen in vruNuf above.  xf.bufn is buffer index.

	xf = u->xf;
	sec = xfwrite( xf, xf->xf_buf, xf->bufn);		// xiopak.cpp
	if (sec==SECOK)					// if ok
	{
		xf->bufn = 0;					// say buffer empty
		return RCOK;					// normal good return
	}

// write error.  message has been issued in xiopak.cpp.  terminate output to this file.
//   sec is EBADF (bad handle - internal error) or ENOSPC (disk full, also terminate unspool run).

	u->fStat = 2;    		// say this file complete, u is ready to close & advance to next input file.
	// (2 not 1 says complete, with error --> nop here when called re final page formatting).
	if (sec==ENOSPC)   			// if disk full
		spl.vrRc = RCBAD;		// shut down unspool run
	return RCBAD;			// error return
}			// vruFlush
//---------------------------------------------------------------------------------------------------------------------------
//  Remember-used-files-even-thru-clear dept, 7-13-92
//---------------------------------------------------------------------------------------------------------------------------
BOO isUsedVrFile( const char* fName)	// test if report/export filename has been used this session: public interface

// compares given text -- does not detect different paths to same directory.
// file is considered "used" only if actually written to or erased (per where nUses is ++'d in vrpak)
{
	int i = ufLook( fName);				// use inner fcn (next)
	return i>=0 && uf[ i].nUses > 0;	// TRUE iff in table and opened
}					// isUsedVrFile
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int ufLook(const char* fName)  	// get -1 or 0-based file name subscript in used files info, uf[].
{
	if (uf)						// insurance
		for (int i = 0; i < nUf; i++)
			if (!_stricmp( fName, uf[i].fName))    	// if name same as one of array of used names (saved thru CLEARs)
				return i;					// return subscript 0.. of entry
	return -1;						// not present
}		// ufLook
//---------------------------------------------------------------------------------------------------------------------------
LOCAL int addUsedFile(		// add report/export file name to list tested by isUsedVrFile

	const char* fName)	// file name text, assumed in dm

// returns subscript of entry in uf[].  Caller accesses that directly re pageN/row/col.

// list built here is saved thru CLEAR to prevent erasing earlier runs report/export files, and to continue page numbering.
{
	int i = ufLook(fName);			// search table, -1 if not yet there
	if (i >= 0)  return i;			// if present in table, return subscript to caller

// add new name to table
	static const int NAL=16;   				// # entries to allocate at a time
	if (nUf >= nUfAl)								// if need to create/enlarge list
	{
		if (dmral( DMPP( uf), (nUfAl + NAL)*sizeof(USEDFILE), DMZERO|ABT)) 	// (re)allocate table, dmpak.cpp.
			return 0;								// insurance: error rtn unexpected
		nUfAl += NAL;								// remember size allocated
	}
	USEDFILE *f = uf + nUf;		// point to next uf[] entry
	dmIncRef( DMPP( fName));	// increment reference count in dm block (or possibly dup ptr). dmpak.cpp.
	f->fName = fName;			// store name in entry
	f->pageN = 1;			// init page number to 1.  .row/.col are 0-based and memory is already 0.
	//nUses left 0 til vrh's found for file (++'d in vruNuf)
	return nUf++;			// return subscript of entry / ++ for next one
}		// addUsedFile
//---------------------------------------------------------------------------------------------------------------------------


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL int bufCase();
LOCAL RC bufLoad( int cs);
LOCAL RC bufWrite( const char* p, ULI n);
LOCAL RC bufRead( ULI off, char* buf, ULI n, ULI *pnRead );

//-----------------------------------------------------------------------------------------------------------------------------
RC vrBufAl()			// allocate vr spool file buffer
// to free, use dmfree(spl.buf1).
// sets spl.bufSz, buf1, buf2, p, p1, p2.  spl is in vrpak1.cpp.
{
	dmal( DMPP( spl.buf1), spl.BUFSZ, DMZERO | WRN);
	if (!spl.buf1)
		spl.vrRc = RCBAD;				// if out of memory (msg'd in dmvalloc)
	else
	{	spl.bufSz = spl.BUFSZ-1;		// reserve a byte at end for \0
		spl.buf2 = spl.buf1 + spl.bufSz;
		spl.p = spl.p1 = spl.p2 = spl.buf1;		// set pointers to indicate empty buffer
		spl.bufO1 = spl.bufO2 = 0;			// redundantly say no text in buffer
	}
	return spl.vrRc;
}			// vrBufAl
//-----------------------------------------------------------------------------------------------------------------------------
RC vrBufLoad()		// load text into unspool buffer, call at start of run which will use bytes spl.runO1 to runO2.

/* keeps some or all of text already in buffer when possible.
   gets start of required text in buffer.
   caller must call vrBufMore if additional text is needed (total may be too much for buffer) */

// uses: spl.runO1, 2, bufSz.
// uses/updates: spl.bufO1, bufO2, bufWo, p, p1, p2.

// if return is not RCOK, can't read spool file, caller should end run.  spl.vrRc is set.
{
	if (!spl.vrRc)		// nop if already a fatal-to-unspool-call error
		bufLoad(  		// discards and/or reads text appropriately for case. local, below.
			bufCase() );  	// determines case (new read, overlap, etc) per spl.bufO1,2, o1,2.
	return spl.vrRc;		// set on spool i/o error by bufRead (from bufLoad)
}			// vrBufLoad
//-----------------------------------------------------------------------------------------------------------------------------
RC vrBufMore()		// append additional data from spool file to unspool buffer

// call when spl.p is between items.  can move data already in buffer.
// updates spl.p1, p, p2, bufO1, bufO2, bufWo.

// if return is not RCOK, can't read spool file, caller should end run.  spl.vrRc is set.
{
	if (spl.vrRc)			// if already a fatal-to-unspool call error
		return spl.vrRc;			// just return bad

// determine # bytes to read
	int n = 8192;						// standard buffer read
	if (n > int( spl.spO - spl.bufO2))
		n = spl.spO - spl.bufO2;			// reduce to # bytes remaining in file
	if ( spl.runO2 > spl.bufO2
	 &&  spl.runO2 < spl.bufO2 + n)			// if end run req't is less than n bytes ahead
		n = spl.runO2 - spl.bufO2;  		// read only enuf to get to it: might save buf data next run needs
	int nAv = spl.buf2 - spl.p2;  			// bytes available in buffer
	if (n > nAv)							// if too few bytes in buffer
		if (nAv >= 2048)					// ... but at least 2048 bytes
			n = nAv;    					// limit read to what will fit buffer -- defer copy-back.
		else						// space is < n and < 2048
			dropFront( ULI( spl.p - spl.p1));   // discard data at start of buf to make room for read.
	  											//   Discard all text up to p to be at an item boundary.
	if (n > spl.buf2 - spl.p2)  			// again check space in buffer
		n = spl.buf2 - spl.p2;  			// must be small buffer, reduce read to fit

// read
	ULI nRead;
	if (bufRead( spl.bufO2, spl.p2, n, &nRead))
	{	// on i/o error reading spool, set buffer emtpy.  bufRead has set vrRc.  caller should terminate run.
		spl.p = spl.p1 = spl.p2 = spl.buf1;
		spl.bufO2 = spl.bufO1;
	}
	else
	{	spl.p2 += nRead;
		spl.bufO2 += nRead;
	}
	return spl.vrRc;
}			// vrBufMore
//-----------------------------------------------------------------------------------------------------------------------------
LOCAL int bufCase()  		// buffer load case-determiner

// uses: spl.runO1, runO2, bufO1, bufO2, p1, p2, bufSz; can update .bufWo.

/* returns:
    0: forget anything in buffer and read anew
    1: read in front of current contents, dropping some of end if necessary
    2: start of text already in ram and all will fit: just set p per o1.
    3: start of text already in ram but end will not fit: drop text in buffer before o1.
    4: read desired text after text in buffer, including gap if any */
{
	int bufN = spl.p2 - spl.p1;		// bytes in buffer, <= 0 if none
	if (bufN < 512)	  				// if no text or < 512 bytes in buffer, forget it
		return 0;
	if (spl.runO1 < spl.bufO1)				// if desired text starts before text in buffer
	{
		if (spl.bufO1 - spl.runO1 + 4096 > spl.bufSz)  	// more than buf len (less 4k) b4 text in buf
			return 0;									// forget buf contents
		if (LI(spl.bufO1 - spl.runO2) > min(LI(bufN),4096))	// if no overlap & if gap > # bytes in buf or 4K
			return 0;						// forget buf contents
		// overlaps, or gap is small so read its text too in order to preserve rest of buffer contents
		return 1;					// say read text from o1 to bufO1 in front of present text
	}
	//else o1 >= bufO1: desired text starts after text in buffer
	if (spl.runO1 < spl.bufO2)				// if overlaps
	{
		if (spl.runO2 - spl.bufO1 < spl.bufSz)   	// if all of buffer text and desired text will fit buffer
			return 2;					// no changes, just set p per o1
		return 3;					// drop buffer contents before o1 (known to be an item start)
	}
	// desired text starts after text in buffer
	if (spl.runO2 - spl.bufO1 > spl.bufSz)    		// if desired & present text won't all fit in buffer
		return 0;					// forget buffer contents
	if (spl.runO1 - spl.bufO2 > ULI( min( bufN, 4096)))	// if gap is not small
		return 0;					// forget buffer contents (rathar than reading text in gap)
	return 4;						// read the gap (bufO2..o1) and some of desired text
}		// bufCase
//-----------------------------------------------------------------------------------------------------------------------------
LOCAL RC bufLoad( int cs)		// vr buffer loader inner function

/* argument cs:
    0: forget anything in buffer and read anew
    1: read in front of current contents, dropping some of end if necessary
    2: start of text already in ram and all will fit: just set p per o1.
    3: start of text already in ram but end will not fit: drop text in buffer before o1.
    4: read desired text after text in buffer, including gap if any */

// uses: spl.runO1, runO2, bufSz.
// uses/updates: spl.bufO1, bufO2, p, p1, p2.

// if return is not RCOK, can't read spool, caller terminate run (spl.vrRc is set).
{
	// assumption: buf1 = p1 always.
	int N;
	ULI n, nRead;
	switch (cs)
	{
case0:
	case 0: 				// discard buffer contents, read anew
		// discard data in buffer
		dropFront( spl.p2 - spl.p1);			// drop all. writes if necessary.
		// read
		n = min( spl.runO2 - spl.runO1, spl.bufSz);	// n = run length, up to buffer size
		if (bufRead( spl.runO1, spl.buf1, n, &nRead))		// read, below
			break;						// read error other than eof; buf set empty below.
		spl.p = spl.p1 = spl.buf1;
		spl.p2 = spl.p1 + nRead;
		*spl.p2 = 0;				// null after text (buffer has extra byte)
		spl.bufO1 = spl.runO1;
		break;

	case 1: 				// read in front of current buffer contents, dropping some at end if necessary
		N = spl.bufO1 - spl.runO1;		// how far to move buffer contents up == # bytes to read
		if (N & 0xffff0000L)			// insurance
			goto case0;
		n = N;
		// determine if must discard data at end of buffer
		if (spl.p2 > spl.buf2 - n)
		{
			// write text about to be discarded if unwritten
			if (spl.spWo < spl.bufO2)
			{
				int w = spl.bufO2 - spl.spWo;	// # unwritten bytes
				bufWrite( spl.p2 - w, w);		// write at spWo, set vrRc if error. below.
				ASSERT( spl.spWo == spl.bufO2);		// fatal error (no return) if arg false
			}
			// discard
			spl.p2 = spl.buf2 - n;			// discard text that won't fit at end buffer
			// spl.bufO2 is computed after switch
		}
		// move up
		memmove( spl.p1 + n, spl.p1, spl.p2 - spl.p1);
		spl.p1 += n;
		spl.p2 += n;
		// read
		if (bufRead( spl.runO1, spl.buf1, n, &nRead))		// read spool file at offset to address, below
			break;						// read error other than eof
		if (nRead != n)			// if something is screwy (unexpected since know not at eof)
			goto case0;
		spl.p = spl.p1 = spl.buf1;
		spl.bufO1 = spl.runO1;
		break;

	case 2:				// 2: text already in ram and all will fit: just set p per o1.
		spl.p = spl.p1 + (spl.runO1 - spl.bufO1);
		break;

	case 3:				// 3: text already in ram but not all will fit: drop text before o1.
		N = spl.runO1 - spl.bufO1;		// # bytes to drop at front of buffer so rest of text can be kept
		if (N & 0xffff0000L)			// insurance
			goto case0;
		dropFront( N);			// next.  updates spl members, sets vrRc if error.
		spl.p = spl.p1;			// desired text now starts at start of buffer
		break;

	case 4:				// 4: read text after, including gap if any
		N = spl.runO2 - spl.bufO2;		// # bytes to read: gap + desired text (caller verified fit).
		if (N & 0xffff0000L)			// insurance
			goto case0;
		if (bufRead( spl.bufO2, spl.p2, N, &nRead))
			break;
		spl.p = spl.p2 + (spl.runO1 - spl.bufO2);
		spl.p2 += nRead;
		break;		// .bufO2 updated after switch

	default: ;	// errmsg ??
	}

	if (spl.vrRc)			// if error in bufRead (or before bufLoad called)
	{
		// i/o error reading spool. set buffer emtpy. caller should terminate run.
		spl.p = spl.p1 = spl.p2 = spl.buf1;
		spl.bufO2 = spl.bufO1;
	}
	else
		spl.bufO2 = spl.bufO1 + (spl.p2 - spl.p1);	// set end-of-buffer file offset, for use on later calls

	return spl.vrRc;					// set by bufRead/bufWrite on error
}			// bufLoad
//-----------------------------------------------------------------------------------------------------------------------------
RC dropFront( ULI drop)		// discard bytes from front of unspool buffer, set p to front of buffer
{
// write any unwritten bytes in data to be dropped
	if (spl.spWo < spl.bufO1 + drop)				// if some of bytes to be dropped not on disk yet
	{
		ASSERT( spl.spWo >= spl.bufO1);
		bufWrite( spl.p1 + (spl.spWo - spl.bufO1),		// next. sets vrRc on error.  location,
				  (spl.bufO1 + drop) - spl.spWo);		// ... # bytes.
		ASSERT( spl.spWo == spl.bufO1 + drop );
	}

// move data to be kept to front of buffer
	if (spl.p1 + drop < spl.p2)					// if any text will be left (can be called to flush)
		memmove( spl.p1, spl.p1 + drop, (spl.p2 - spl.p1) - drop);
	spl.p2 -= drop;
	spl.p = (spl.p >= spl.p1 + drop) ? spl.p - drop : spl.p1;	// max( spl.p - drop, spl.p1)  rearranged to not underflow
	spl.bufO1 += drop;
	ASSERT( spl.bufO2 == spl.bufO1 + (spl.p2 - spl.p1));	// bufO2 unchanged
	return spl.vrRc;
}			// dropFront
//-----------------------------------------------------------------------------------------------------------------------------
LOCAL RC bufWrite(		// write n bytes from p to file offset spl.spWo, update spWo
	const char* p,
	ULI n)
{
	if (spl.vrRc)			// if already a fatal-to-unspool call error
		return spl.vrRc;			// just return bad
// seek
	if (xfseek( spl.splxf, spl.spWo, SEEK_SET) != SECOK)
		return (spl.vrRc = RCBAD);
// write
	SEC sec = xfwrite( spl.splxf, p, n);
	if (sec != SECOK)			// if error
		spl.vrRc = RCBAD;		// say error: shut down vrUnspool.  callers assume this set.
	else
		spl.spWo += n;   		// update file offset of first unwritten byte.  Insure no sign extension.
	return spl.vrRc;
}			// bufWrite
//-----------------------------------------------------------------------------------------------------------------------------
LOCAL RC bufRead( ULI off, char* buf, ULI n, ULI* pnRead )		// read into unspool buffer

// on error, sets spl.vrRc and returns RCBAD.
{
	SEC sec;

// check not reading unwritten data
	ASSERT( off + n <= spl.spWo );	// fatal error if argument false
// seek
	if (xfseek( spl.splxf, off, SEEK_SET) != SECOK)
	{
		*pnRead = 0;
		return (spl.vrRc = RCBAD);
	}
// read
	sec = xfread( spl.splxf, buf, n);	// read, lib\xiopak.cpp
	*pnRead = spl.splxf->nxfer;		// return # bytes read
	if (sec != SECOK)			// if error or eof (not an error to us)
	{
		xlsterr( spl.splxf);		// clear error/eof indication -- else further calls may nop
		spl.vrRc = RCBAD;		// say error; shut down vrUnspool.  callers assume set on nonRCOK return.
		// should not get eof: caller should not read off end of file (spl.spO).
		// note eof will yield error message per xfopen parameter (vrpak.cpp).
	}
	return spl.vrRc;
}			// bufRead
//---------------------------------------------------------------------------------------------------------------------------


// vrpak.cpp end


