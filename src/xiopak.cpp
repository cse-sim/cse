// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// xiopak.cpp: Extended IO and file functions

/* Department of further improvements (8-24-89) --

1.  change xfopen call to be sec = xfopen( .... &xf ) and make it always
    return an XFILE (even if open fails).  Then application code could be:
    		xfopen( ..., &xf )
		xfxxx( xf, ... )
		xfxxx( xf, ... )
		xfclose( &xf, &cumSec)
		if (cumSec != SECOK)
		   did not complete correctly
*/


#include "cnglob.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace filesys = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace filesys = std::experimental::filesystem;
#else
#error "no filesystem support"
#endif

/*-------------------------------- OPTIONS --------------------------------*/
// #define TEST		 // define to include main() which includes test code for many fcns.  See #ifs below. Ancient, 2-95

#define MESSAGESC	// define to link with messages.cpp and accept message handles in xfprintf.
					// undefine to link without messages.cpp


/*------------------------------- INCLUDES --------------------------------*/

#include <sys/types.h>	// reqd b4 stat.h (defines dev_t)
#include <errno.h>	// ENOENT
#include <fcntl.h>	// O_TEXT O_RDONLY
#include <sys/stat.h>	// S_IREAD, S_IWRITE

#include "msghans.h"	// MH_xxxx defns
#include "messages.h"	// msgSec()

#include "cvpak.h"

#include "xiopak.h"	// xiopak.cpp publics

// DELETED unused code, last included in tag v0.908.0 7/7/2022
//   xfSetupPath(), xfFindOnPath(), path seach in xfopen()
//   Functionality replaced with C lib calls and caller local path searches
//   Also deleted file directry.cpp containing functions used only by deleted xf functions


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL SEC FC xioerr( XFILE *xf);


//=============================================================================
void FC xfClean()	// cleanup routine
// added for DLL version which should free every byte of allocated heap space, rob, 12-93.
{
	// currently nothing needed
}		// xfClean
//======================================================================
SEC FC xfdelete(	// delete a file

	const char *f,		// Pathname of file to delete (nop if NULL)
	int erOp/*=WRN*/)		// Error action code -- ABT, WRN, IGN, etc

// Returns SECOK or error code

{
	SEC sec = SECOK;
	if (f)
		if (std::remove(f) != 0)
		{
			sec = errno;
			err( erOp, 			// display msg per erOp, rmkerr.cpp
				 MH_I0010,		// "I0010: unable to delete '%s':\n    %s"
				 f,				//    filename
				 msgSec(sec) );		//    sec text, messages.cpp
		}
	return sec;
}		// xfdelete
//-----------------------------------------------------------------------------
SEC FC xfrename(	// Rename a file

	const char *f1,	// Old file pathname
	const char *f2,	// New file pathname
	int erOp )   	// Error action code -- ABT, WRN, IGN, etc (cnglob.h)

// Returns SECOK or error code if error (unless erOp == ABT)
{
	SEC sec = SECOK;
	if (rename(f1,f2) != 0)
	{
		sec = errno;
		err( erOp,			// display message per erOp, rmkerr.cpp
			 MH_I0021,		// "I0021: Unable to rename file '%s' to '%s':\n    %s"
			 f1, f2,		//    filenames for message
			 msgSec(sec));	//    sec text for message (messages.cpp)
	}
	return sec;
}		// xfrename
//======================================================================
const char * FC xgetdir()

/* Returns pointer to current working drive/directory pathname in Tmpstr[] */

{
	char * p = strtemp(CSE_MAX_PATH);	// alloc n+1 bytes tmp strng space, strpak.cpp
	strcpy(p, filesys::current_path().string().c_str());

	return p;		// seems to include drive.
}		// xfgetdir
//======================================================================
SEC FC xchdir(		// Change current working directory and/or drive

	const char* path, 	// new working directory. May include drive. "" is ok NOP.
	int erOp/*=WRN*/ )	// Error action -- IGN, WRN, ABT, etc

// Returns SECOK if sucessful; otherwise error number SECBADRV or ENOENT
{
	filesys::path fsysPath = filesys::path(path);
	SEC sec=SECOK;
	std::error_code ec;
	filesys::current_path(fsysPath, ec); // select directory (c library)
	if (ec) {
		sec = RCBAD;
	}
	if (sec != SECOK)
		err(erOp, "\"%s\": %s", fsysPath.string().c_str(), ec.message().c_str());
	return sec;
}		// xchdir
//======================================================================
RC FC xfGetPath(	// Check path / retrieve full pathname of a file

	/* CAUTION has been observed to return WRONG path for an opened
	   file when a separate data path utility (dpath) program is active
	   in the system.  May be correct for find-first found files. */

	const char * *fnamp,  	/* entry:  file name: can be a simple file name, or contain a full or relative path.
					   The file need not exist, but the drive and directory path must.
					   "const": actually is modified here, but restored, 10-94.
				   return: if ok, this pointer REPLACED WITH pointer to full path, in Tmpstr. */

	int erOp/*=WRN*/)	// error action for invalid dir or drive: WRN, ABT, etc

// Returns RCOK if ok, with *fnamp replaced, else RCBAD.
{
	filesys::path fname(*fnamp), parentPath, fileNamePart;	// filesystem variables
	std::error_code ec;						// Error Code for filesystem functions

	if (fname.has_parent_path()) // Check the parent path i.e drv:dir
	{
		if (filesys::exists(fname.parent_path())) {		// Check the parent path exists.
			parentPath = fname.parent_path();			// Save the parent path
		}
		else
		{
			err(erOp, "\"%s\": Invalid path", fname.string().c_str());
			return RCBAD;		// Parent path is not valid
		}
		fileNamePart = fname.filename();	// Save the filename.ext to be append at the end
	}
	else // No parent path
	{
		parentPath = filesys::current_path();	// Use the current location
		fileNamePart = fname;
	}

	// assemble, with \ between dir and filename one is lacking
	*fnamp = strtPathCat(parentPath.string().c_str(), fileNamePart.string().c_str());

	return RCOK;		// another return is in code above

}		// xfGetPath
//======================================================================
XFILE* xfopen(  	// Set up extended IO structure and open file

	const char *fname, 	// File pathname
	const char *accs,			// Access type.  Passed to fopen().  Note that the useful
						//  combinations of accesses are defined in xiopak.h
	int erOp/*=WRN*/,	// Error action code (ABT/WRN/IGN etc) for all errors both now
						// and in later calls.  To change after open, use xeract().
						// PLUS option bits:
						//   XFBUFALLOC:	if 1, allocate local buffer XFILE.xf_buf[]
	bool eoferr/*=false*/,	// If true, treat EOFs on this file as errors
	SEC *psec/*=NULL*/ ) 	// NULL or receives Error code if open fails

// #ifdef WANTXFSETUPPATH, searches paths set up with xfSetupPath if existing file requested (no O_CREAT bit in accs, 2-95)

/* if ok, returns pointer to extended file structure, with *psec = SECOK.
   if fails, returns NULL with *psec:
		 bad file name (per our check):  SECBADFN  (cnglob.h)
				     not found:  ENOENT    (errno.h)  etc.
	     if erOp not IGN, operator has seen explanatory message. */

{
static const int XFBUFFSZ = 1024;
	XFILE *xf;
	SEC sec;
	errno = SECOK;			// Make sure errno is clear

// allocate and init XFILE structure

	int tBufsz = erOp & XFBUFALLOC   	// size of buffer needed per caller
				? XFBUFFSZ		//    #define above
				: 0;   		//    no buffer needed
	dmal( DMPP( xf), 			// allocate dm (heap space) for XFILE, dmpak.cpp
		  sizeof(XFILE) + tBufsz,	// size: base XFILE + buffer if needed.  1 extra allocd due to .buf[1] in XFILE.
		  ABT | DMZERO );			// abt if dm full (unlikely -- small), 0 fill
	strcpy(xf->access, accs);    			// various fcns need to know how file opend
	xf->xferact = erOp & (ERAMASK|PROGERR);	// save for later xiopak calls for file
	xf->xfeoferr = eoferr;			// nz if eofs are to be treated as errors
	xf->bufsz = tBufsz;				// buffer size: 0 or XFBUFFSZ
	xf->line = 1;					// line #, maintained in xiochar:cGetc
	// xf->bufn = 0;	 			*   buffer empty (via DMZERO)
	// xf->bufi = 0;	 			*   next buffer char (via DMZERO)
	// xf->xfname is init below

// open file, handle fail
	if ( (xf->han = fopen( fname, xf->access)) == NULL)	// open file (C library) / if failed
	{
		xf->xfname = fname;		// used in xioerr's error messages
		sec = xioerr(xf);		// resolve sys err code & report error
		if (psec)
			*psec = sec;		// return SEC
		dmfree( DMPP( xf));
		return NULL;			// error return
	}
// success
	xf->xflsterr = SECOK;		// say no error
	if (psec)
		*psec = SECOK;			// ..

	/* get full pathname, e.g. for later reopen even if dir changed.
	   Known that can get *WRONG* path after opening name.ext w/o dir spec when a dpath (data path) program is active in system.
	   Code is being added elsewhere to get full path of all files earlier (1989? comment, ongoing 2-95).
	   No error expected if here, but do issue msg if error detected. */
	xfGetPath( &fname, WRN);		// get full path -- replaces xfname with Tmpstr pointer
	xf->xfname = strsave(fname);	// put in dm
	return xf;				// success.  Other returns above.
}		// xfopen
//======================================================================
SEC FC xfclose(		// Close file and release XFILE

	XFILE **xfp,		// extended IO packet ptr ptr. if NULL, or if points to NULL, no close. *xfp is left NULL.
	SEC *pCumSec/*=NULL*/ )	// NULL or receives cumulative SEC from all IO (including close) since last xlsterr()

/* Returns SECOK if close OK, otherwise errno.
   *pCumSec returns cumulative errno (since opened or xlsterr).
   Caller's pointer is set NULL. */

{
	SEC sec = SECOK;		// no errors yet
	if (pCumSec)
		*pCumSec = SECOK;	// no errors yet

	XFILE * xf = (xfp != NULL) ? *xfp	// fetch ptr via ptr-ptr
				 : NULL;	//   or NULL if ptr-ptr is (unexpectedly) NULL

	if (xf != nullptr)			// if we really have an XFILE
	{
		int closeRet = fclose(xf->han);	// attempt close even if prev error
		if (closeRet)			// close() returns NZ on error
			sec = xf->xflsterr
				  = xioerr(xf);		// resolve, report, rtrn SEC (below)
		if (pCumSec)			// if ptr not NULL 3-91
			*pCumSec = xf->xflsterr;	// cumulative SEC receives any error including those caused by close
		dmfree( DMPP( xf->xfname));	// free name if any (see xFindOnPath)
	}

	if (xfp)
		dmfree( DMPP( *xfp));	// free XFILE.  Note use of xfp (not &xf) so caller's pointer is NULLed.

	return sec;			// return outcome of close()
}		// xfclose
//======================================================================
SEC FC xfread(		// Read from file

	XFILE *xf,		// Pointer to extended IO packet for file
	char *buf,		// ptr to buffer into which to read (MUST be large enuf)
	USI nbytes) 	// Number of bytes to read.  NOP if 0, max = 0xFFFE

/* EOF indication: binary open: if fewer than requested bytes remain in file.
				(xf->nxfer says # actually transfered)
                   text open:	if 0 bytes remain in file. */

// EOF messaging: as error, or no msg, per eoferr arg to xfopen or xeract.

/* FORMERLY xfread returned buffered chars from xf->buf (if present) prior
            to reading.  Now, xfread ignores ->buf; if you want to change
	    from xiochar-style char access to normal access, you must close
	    & reopen (e.g. xftell, cClose, xfopen, xfseek, xfread) 3-90 */

/* CAUTION error or eof indication (even unmessaged) causes subsequent xiopak
	   calls to do nothing and return same error code till caller clears
	   .xflsterr, e.g. by calling xlsterr(xf).  Rob 10-89 */

// Returns SECOK OK, SECEOF to indicate eof, other values for other errors.
{
	if (   nbytes > 0			// if read needed
			&& xf->xflsterr == SECOK)	// and if no prior unnoticed error (clear with xlsterr(xf) )
	{

		/* Read if necessary.
		   If gets at least 1 byte (text mode) or requested bytes (binary), return ok.
		   If less, indicate EOF (or other error if read() set errno).
		   If 0 bytes are transferred, force the MSC lib variable! "errno" to SECEOF
			to cause EOF return even if eof() returns 0.
				Known case: after lseek()ing beyond the end of file,
					    read() returns 0, but eof() is not set (!). */
		/* CAUTION: caller must clear eof/error (use xlsterr(xf))
		   	    b4 ANY other xiopak call will work (except xfseek and xftell) */

		size_t nX = fread( buf, sizeof(char), nbytes/sizeof(char), xf->han)*sizeof(char);	// read

		if (  nX != nbytes)			// read() error return
		{
			if (   nX == 0		// if 0 bytes transferred
					|| feof( xf->han) )	// or at end of file
				errno = SECEOF;		// force errno to EOF (commnt abve)

			xf->xflsterr = xioerr(xf);	// issue i/o error msg or not per .xferact,.xfeoferr; SET xflsterr
			// CAUTION caller must clear error/eof -- xlsterr()
		}
		xf->nxfer = nX;			// # bytes transferred
	}
	else
		xf->nxfer = 0;			// no xfer: prev err or nbytes==0

	return xf->xflsterr;		// return SECOK, prior error code, or code just stored at xioerr
}			// xfread
//======================================================================
SEC CDEC xfprintf(		// fprintf to an XFILE (via xfwrite)
	XFILE *xf,		// file to be written to
	const char *format,	// printf-style format string, or message handle if MESSAGESC
	... )		// values as required by format

// max length of formatted string is 500 (7-3-89)

// Returns SECOK if all OK, else errno
{
	va_list ap;
	int nbytes;
	char buf[501];		// 1 for '\0'

	va_start( ap, format );			// arg list ptr for vsprintf
#ifdef MESSAGESC
	msgI( WRN, buf, sizeof( buf), &nbytes, format, ap);	// retrieve text for handle, if given, and build fmtd str in buf
#else//3-92
	nbytes = vsprintf( buf, format, ap );	// build fmtd str in buf
#endif
	return xfwrite( xf, buf, nbytes );		// NOP if nbytes == 0
}			// xfprintf
//======================================================================
SEC FC xfwrite(  	// Write to a file

	XFILE *xf,  	// Pointer to extended IO packet for file
	const char *buf,  	// ptr to buffer from which to write
	size_t nbytes) 	// Number of bytes to write. NOP if nbytes==0. Largest legal value 0xFFFE (0xFFFF rsrvd for error cond).

// Returns SECOK if all OK, else errno

/* CAUTION: if there has been an error OR EOF caller must clear .xflsterr
            (eg call xlsterr) else xiopak calls just keep returning same code*/
{
	if ( nbytes > 0			/* if write needed */
	  && xf->xflsterr == SECOK)	/* and no if error currently posted */
	{
		if ( (xf->nxfer = fwrite( buf, sizeof(char), nbytes/sizeof(char), xf->han)*sizeof(char)) != nbytes)
			xf->xflsterr = xioerr(xf);	// report err (below), SET .xflsterr
		// CAUTION caller must clear error/eof -- xlsterr()
	}
	else
		xf->nxfer = 0;		// no transfer: prior error or nbytes==0

	return xf->xflsterr;	// return prior or newly posted sec
}			// xfwrite
//======================================================================
SEC FC xftell(  	// Return current file position with proper accounting of buffered characters

	XFILE* xf,	// Pointer to extended IO packet for file
	long* fpos)	// Pointer to receive current file position

// attempts ftell() call w/o regard to prior errors so should return pos even
// on eofd file.

/* Returns: SECOK if result is good.
            else other sec w/ *fpos = -1 (DOES NOT SET xf->xflsterr) */
{
	SEC sec;
	int pos = ftell(xf->han);		/* get position at c library level */
	if (pos == -1L)
		sec = xioerr(xf);		/* report error, DON'T set xflsterr */
	else
	{
		sec = SECOK;			/* returned position is valid */
		pos -= xf->bufn - xf->bufi;	/* adjust for unread bytes in buf */
	}
	*fpos = pos;		/* return file pos or -1L */
	return sec;			/* return outcome of THIS FUNCTION ONLY */
}				// xftell
//======================================================================
SEC FC xfseek(		// Reposition file pointer

	XFILE *xf,	/* Pointer to extended IO packet for file */
	long disp,	/* Byte displacement */
	int mode )	/* Mode (these are C library constants in stdio.h) --
	      SEEK_SET:  disp is offset from beg of file
	      SEEK_CUR:  disp is offset from current position
	      SEEK_END:  disp is offset from end of file */

/* FORMER CAUTION (10-89):
            if there has been an error OR EOF INDICATION (even unmessaged)
	    caller must clear xflsterr or this and all? xiopak calls just
	    returns same code.  For example, call xlsterr(xf).  I think this
	    is a poor spec but don't wanna upset things now... (rob 10-89) */
/* FORMER CAUTION (3-90):
            Now always attempts lseek and SETS xf->xflsterr.  That is, a
	    successful lseek clears a prior posted error.  But NOTE that
	    MSC documentation for read() states that file must be closed to
	    clear eof indication (after ^Z on text file, at least).  Testing
	    did not show this, even on ^Z eof'd file.  So apparently
	    the documentation is wrong but watch your step.  (chip 3-90).
	    UPDATE: msc 6.0 documentation states that lseek clears a ^Z eof
	    but I did not test. (chip, 8-19-90) */

// Returns   SECOK iff _lseek() call was successful, sets xf->xflsterr to SECOK
//	     else other SEC (xf->xflsterr set, PRIOR xf->xflsterr lost)
{
	if (fseek( xf->han, disp, mode) != 0)
		xf->xflsterr = xioerr(xf);	/* report and post error */
	else
	{
		xf->xflsterr = SECOK;		/* CLEAR any prior error */
		xf->bufn = xf->bufi = 0;		/* force buffer reload for xiochar
   					   access method (see xiochar:cPeekc).
				   Both mbrs 0'd so xftell (just
				   above) works */
		/* xf->line maintained by xiochar:cGetc is now INCORRECT. */
	}
	return xf->xflsterr;		/* return outcome */
}			// xfseek
//======================================================================
SEC FC xfsize(		// Return the current size of a file

	XFILE* xf,			// Pointer to extended IO packet for file.  Must be open.
	uintmax_t* psize)	// Current size of file

// Returns SECOK if no errors; else other sec (DOES NOT set xf->xflsterr)
{
	std::error_code ec;
	*psize = filesys::file_size(xf->xfname, ec);	/* get cur file length, MSC */
	if (ec)
	{	// if length invalid
		err(xf->xferact, "\"%s\": %s", xf->xfname, ec.message().c_str());
		return xioerr(xf); /*    bad: report error, return SEC */
	}
	return SECOK;   		/*    good */
}				// xfsize
//======================================================================
uintmax_t FC xdisksp()		// returns diskspace on default drive
{
	return dskDrvFreeSpace();
}		// xdisksp
//=============================================================================
uintmax_t FC dskDrvFreeSpace()		// returns total disk free space in bytes on current default drive
{
	// Determine the available space on the filesystem on which current path is located
	std::error_code ec;
	uintmax_t freeSpace = filesys::space(filesys::current_path(), ec).available;

	if (ec)
		// GetLastError for more info
		return 0;					// call failed, return 0 space avail

	if ( freeSpace < UINTMAX_MAX)	// if free space fits in a uintmax
		return freeSpace;			// compute fixed point free space & return it

	return UINTMAX_MAX;				// too much for uintmax_t, return max uintmax_t value (16 Million TB)
}					// dskDrvFreeSpace

//======================================================================
SEC FC xlsterr(		// Retrieve and clear lsterr value for specified open file

	XFILE* xf)	// Pointer to extended IO packet for file

/* Returns SECOK if no errors have occurred on file xf, or errno of
	last error.	*** lsterr value is reset to SECOK. *** */

// use after error/eof indication else further calls don't work
{
	SEC i;

	i = xf->xflsterr;
	xf->xflsterr = SECOK;
	return i;
}		// xlsterr
//======================================================================
void FC xeract(		// Reset error and eof action codes for file xf
     XFILE* xf,	// Pointer to extended IO packet for file
     int erOp,		// New error action code for file: IGN, WRN, ABT, PWRN, PABT, etc (cnglobl.h)
     SI eoferr)	// If TRUE, treat EOFs as errors

{
     xf->xferact = erOp;
     xf->xfeoferr = eoferr;
}				// xeract
//======================================================================
LOCAL SEC FC xioerr(	// Resolve SEC and optionally report io error

	XFILE *xf )		// Pointer to extended IO packet for file

// caller must set errno where MSC does not (eg EOF after read() ).

// As of 3-90 DOES NOT set xf->xflsterr, so callers can determine whether error state should persist on file.

// returns SEC (usually from MSC errno)
{
	/* Determine System Error Code (SEC):
	        use system errno if not ok,
	        else use SECOTHER for cases that get here without setting errno.
	   		(SECBADFN does not come here.)
	   Former (1-3-90) code yielded SECEOF per local call to eof()
	        BUT that said EOF for non-EOF errors (eg Disk Full on write)
		SO now xfread externally forces errno to SECEOF per eof() */

	SEC sec = errno != SECOK		// check if errno non-OK
			  ? errno 		//   if errno is error use it
			  : SECOTHER;   	//   some other err w/o errno set

	errno = SECOK;	// reset errno (seems risky, but OK so far 8-89)

	// issue msg per .xferact, except not for EOF if not treating eof as err.
	// !! DO NOT call err if .xferact is IGN: caused bad self-call case in
	// CALRES, no known CSE problem but let's be safe.
	// similar code in vruOpen.
	if (   ( (xf->xferact & ERAMASK) !=IGN )  			// skip if IGN: see above
			&& ( sec != SECEOF || xf->xfeoferr ) )
		err( xf->xferact, 					// disp msg per erOp, rmkerr.cpp
			 "I0012: IO error, file '%s':\n    %s", 		// this text NOT on disk as likely to occur when opening msg file.
			 // same I0012 text also in app\vrpak2.cpp for CSE.
			 xf->xfname,						// filename
			 msgSec( sec) );					// sec text, messages.cpp, eg "no such file or directory (sec=2)

	return sec;		// return resolved SEC
}				// xioerr
//======================================================================
int xfWriteable(			// check whether file is writeable
	const char* fPath,	// path
	const char** ppMsg /*=NULL*/)	// errno message (set iff ret <= 0)
// returns 1 if writeable
//        -1 path does not exist
{
	int ret = 0;
	const char* pMsg = NULL;
	FILE* han = fopen(fPath,"ab");
	if (han)
	{
		fclose(han);
		ret = 1;
	}
	else
	{
		pMsg = msgSec(errno);
		if (errno == ENOENT)
			ret = -1;
		// else ret = 0
	}
	if (ppMsg)
		*ppMsg = pMsg;
	return ret;
}	// xfWriteable
////////////////////////////////////////////////////////////////////////
// path notes, 4-2016
//   Windows extended paths \\?\ not supported
//   Only \ separator is supported (not /)
//   consider stdlib filesystem, Boost filesystem, or POCO for more generality
////////////////////////////////////////////////////////////////////////
int xfExist(	// determine file existence
	const char* fPath,				// pathname
	char* fPathChecked /*=NULL*/)	// ptr to optional buf[ CSE_MAX_PATH]
									//  returns trimmed fPath as checked
// Returns 3 if dir
//         2 if non-empty "regular" file
//		   1 if empty "regular" file
//         0 not found or not dir or "regular" file (e.g. "C:")
//        -1 error
{
	int ret = 0;
	// trim whitespace from beg and ws + \ from end (insurance)
	// return to caller or use tmpstr
	const char* tPath = strTrimEX( fPathChecked, strTrimB( fPath), "\\");
	if (!IsBlank( tPath))
	{
		std::error_code ec;
		bool fileFound = filesys::exists(filesys::path(tPath), ec);
		if (fileFound) {
			ret = (filesys::is_directory(filesys::path(tPath))) ? 3 // Directory found 
					: (filesys::is_empty(filesys::path(tPath))) ? 1 // File empty
																: 2;// Non-empty file other (e.g. volume "C:")
		}
		else if (ec) {
			ret = -1; 	// Unknown error
		}
		else {
			ret = 0; 	// Clean not found
		}
	}

	return ret;
}		// xfExist
//===========================================================================
int fileFind1(			// check existence of a single file
	const char* drvDir,		// drive and/or dir in which to look
							//   ignored if NULL or ""
	const char* fName,		// simple file name (with or w/o ext) check
	char* fPath /*=NULL*/)	// ptr to buffer [ CSE_MAX_PATH] to receive
							//   assembled path that was checked
// returns 3 if dir
//         2 if non-empty file
//		   1 if empty file
//		   0 if not found or volume (C:)
//        -1 error (unexpected return from _stat)

{
	char tPath[CSE_MAX_PATH];
	int i = 0;
	if (drvDir && drvDir[ 0])
	{  	strTrim( tPath, drvDir);
		i = static_cast<int>(strlen(tPath));
		if (tPath[ i-1] != ':' && tPath[ i-1] != '\\')
			tPath[ i++] = '\\';		// add \ to dir if needed
	}
	strcpy( tPath+i, strTrim( NULL, fName));

	int ret = xfExist( tPath, fPath);	// check file
										// return exact name checked
	return ret;
}			// fileFind1
//-----------------------------------------------------------------------------
bool findFile( 	// non-member function to find file on given path

	const char* fName, 		// file to find. Partial path ok; no path search if contains dir path beginning with '\'.
	const char* path, 		// path to search; NULL for DOS environment PATH; current dir always searched first.
	char* buf )				// receives full pathName if found; altered even if not found. array size CSE_MAX_PATH

// returns TRUE if found (either file or directory)
{
	buf[ 0] = '\0';
	if (!fName)
		return FALSE;					// insurance: NULL pathname ptr is never found

	char fNameFound[ CSE_MAX_PATH];		// Holds a fullpath
	// lookup name as provided (no path)
	int found = fileFind1( NULL, fName, fNameFound);
	if (found == 0)
	{	// not found, maybe search on path
		if (!xfhasroot(fName) && !xfhasrootdirectory(fName))	// if fName has no drive and dir
		{	// path search iff fName is relative w/o specified drive
			if (!path)						// if NULL given
				path = getenv("PATH");		//   search environment path
			char* p = strTrim( NULL, path);	// working copy
			const char* pToks[ 100];
			int n = strTokSplit( p, ";", pToks, 100);
			if (n < 0)
				n = 100;
			for (int i=0; !found && i<n; i++)
				found = fileFind1( pToks[ i], fName, fNameFound);
		}
	}
	bool bRet = false;
	if (found > 0) {
		filesys::path bufPath = filesys::absolute(filesys::path(fNameFound)); // get full path, probably even if no such file
		bRet = filesys::exists(bufPath);
		strcpy(buf, bufPath.string().c_str());
	}	
	if (!bRet)
		buf[ 0] = '\0';		// insurance
	return bRet;
}			// findFile
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// class Path
//---------------------------------------------------------------------------
void Path::add( 		// add ;-delimited paths to Path object

	const char *s )	// path;path;path, or NULL to add DOS path paths
{
	if (!s)
		s = getenv("PATH");			// NULL means use DOS environment PATH variable
	if (!*s)
		return;					// nop if no paths to add

// determine size
	size_t lenNow = p ? strlen(p) : 0;			// 0 or length of existing path(s) string
	size_t sneed = (lenNow + strlen(s) + 2) | 32;  	// space needed: old, new, ';', '\0', | 32 for less reallocation
	size_t snow = (lenNow+1) | 32;			// now big it must be now (if p != NULL)

// allocate
	char * nup;
	if (p && sneed <= snow)				// if existing storage big enough
		nup = p;						// just use it
	else
	{
		nup = new char[sneed];				// allocate new storage
		if (!nup)
		{
			err( WRN, "Out of memory in Path::add");
			return;					// let program continue without the path
		}
		if (p)
		{
			strcpy( nup, p);		// copy old string into new storage
			delete[] p;			// free old storage
		}
	}

// update
	char *q = nup + lenNow;		// pointer to end of current paths
	if (lenNow)				// if there were already any paths
		if (*(q-1) != ';' && *s != ';')	// unless there already is a ';'
			*q++ = ';';			// supply separating ;
	strcpy( q, s);			// append new path(s)
	_strupr(q);				/* upper-case user-entered paths for uniform appearance 2-95
          				   (paths from system are upper case; most filenames get uppercased eg by strffix). */
	p = nup;				// store new pointer
}		// Path::add
//---------------------------------------------------------------------------
bool Path::find(   	// find file using paths in Path object.
	const char *fName, 	// file to find. Partial path ok; no path search if contains dir path beginning with '\'.
	char *buf )   		// receives full path if found; is altered even if not found.
 						// array size must be >= CSE_MAX_PATH (stdlib.h).
// Searches current dir first. Does not search dos PATH unless it has been added to object.

// returns TRUE iff found
{
     return ::findFile( fName, p ? p : "", buf);	// use non-member path search fcn (above).
     							// suppress dos path search on NULL for consistency with paths-added case.
}	// Path::find
//---------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////
// file-oriented functions
////////////////////////////////////////////////////////////////////////
RC FC xfcopy(		// Copy a file

	const char *fns,	/* Source file pathname for copy */
	const char *fnd,	/* Destination file pathname for copy */
	const char *accd,	/* Destination access flag -- determines whether
               destination file is overwritten, must preexist, etc.
               Typical values (xiopak.h/fcntl.h) --
                O_WB       Dest. is fresh (created or overwritten)
                O_WBUNKA   Dest. is appended to if it already exists
                           or created if it doesn't.
                O_WBOLDA   Dest. is appended to; must preexist.
               Note all access combinations are binary.  The source
               file is opened O_RB, so the dest file must be too. */
	int erOp )	// Error action:  IGN, WRN, ABT, etc (cnglob.h),  and
				//  Option bit:  XFCOPYTEXT (xiopak.h):
			 	//    Terminate input file at ctrl-Z:
				//    Use when copying a text input file which may later
				//    be re-opened for appending: corrects WordStar's
				//    vestigial cp/m style file termination, to prevent
				//    additional appended stuff from dissappearing.
				//    Do NOT use for copying binary files!

/* Returns RCOK if copy was successful,
   else nz RC; any partially written destination file is *NOT* deleted

   The RC codes returned by xfcopy() are message handles for msgs with
   args.  Best to use erOp=WRN and let msg reporting happen here */
{
	
	SI eraMasked;

	RC rc = RCOK;
	const char* erArg = NULL;
	eraMasked = erOp & ~EROMASK; 	// don't pass options to wrong fcns
	//   mask leaves only bits significant
	//   to error:errI.  cnglob.h
	SEC sec = SECOK;	// Used for error reporting below.
                        // Bad I/O action sets sec to non-SECOK value
	XFILE* xfd = NULL;
	XFILE* xfs = xfopen( fns, O_RB, IGN, FALSE, &sec); 	// IGN: we handle errs here
	MSGORHANDLE mOrH;
	if (xfs==NULL)
	{
		rc = MH_I0022;		// "I0022: file '%s' does not exist"
		mOrH = rc;
		erArg = fns;
	}
	else
	{
		xfd = xfopen( fnd, accd, IGN, FALSE, &sec);	// IGN: hndl errs here
		if (xfd==NULL)
		{
			rc = (strchr(accd, 'a') ||
					strchr(accd, 'w')) // Check for a create file mode on
				 ? MH_I0023	// "I0023: Unable to create file '%s'"
				 : MH_I0024;	// "I0024: File '%s' does not exist, cannot copy onto it"
			mOrH = rc;
			erArg = fnd;
		}
		else
		{
			uintmax_t reqspace;
			xfsize( xfs, &reqspace);	// required disk space
			uintmax_t availspace = xdisksp();		// available disk space
			if (availspace < reqspace + 2048)
			{
				rc = MH_I0025;		// "I0025: not enough disk space to copy file '%s'"
				mOrH = rc;
				erArg = fns;
			}
			else
			{
				const char* msg = nullptr;
				rc = xfcopy2(xfs, xfd, erOp, &msg);
				mOrH = msg;
			}
		}
	}
	xfclose( &xfs, NULL);	/* close files, ignore errors */
	xfclose( &xfd, NULL);
	if (rc != RCOK)
		err( eraMasked,		// disp msg, rmkerr.cpp
			 mOrH,			// MH for msg or ptr to msg text from xfcopy2
			 erArg);		//   msg arg, typically name of file w/ err;
	//     not used in all cases but harmless

	return rc;
}        // xfcopy
//==============================================================================
RC FC xfcopy2(		// copy bytes from one file to another (inner xfcopy loop)

	XFILE *xfs,		// source file, opened and positioned
	XFILE *xfd,		// destination file, opened and positioned
	int op,			// for XFCOPYTEXT bit (see xfcopy comments)
	const char** pMsg )	// NULL or ptr for return of ptr to err msg

/* copies source file into destination until source EOF (or error) */

/* fcn removed from xfcopy to allowed shared use by others for e.g.
   copying into existing file.  Immediate use did not materialize, but
   left as separate fcn since appears useful.  chip 9-11-90 */

/* returns RCOK if transfer completed successfully
      else nz RC, *pMsg pointing to UNISSUED Tmpstr error msg */
{
	char *buf;
	const char *ebuf, *fName = nullptr;
	SEC sec;

	/* allocate memory for buffer */
	size_t bufsize = 0x8000;
	dmal( DMPP( buf), bufsize, ABT);

	/* loop, reading and writing buffer loads */
	RC rc = RCOK;
	do
	{
		sec = xfread( xfs, buf, (USI)bufsize);
		if (sec != SECOK && sec != SECEOF)
		{
			rc = MH_I0026;	// "I0026: read error during copy from file '%s':\n    %s"
			fName = xfs->xfname;
		}
		else if (xfs->nxfer > 0)				/* if bytes in bfr */
		{
			size_t nw = xfs->nxfer;
			if (op & XFCOPYTEXT)	// option to terminate input file at ctrl-Z:
									// corrects WordStar's vestigial cp/m style file termination,
									// so that if file is later opened for appending, appended
									// stuff does not dissappear due to being after the ctrl-Z.
			{
				ebuf = (const char *)memchr( buf, 0x1a, nw);	// point to ctrl-Z, or NULL
				if (ebuf)					// if buffer contains ctrl-Z
					nw = (USI)(ebuf - buf);			// write only bytes b4 it.
				// and we expect this to be last buffer load.
			}
			sec = xfwrite( xfd, buf, nw );		/* write buffer */
			if (sec != SECOK)
			{
				rc = MH_I0027;		// "I0027: write error during copy to file '%s':\n    %s"
				fName = xfd->xfname;
			}
		}
	}
	while (rc==RCOK && size_t( xfs->nxfer)==bufsize);	/* last transfer is smaller than buffer */
	dmfree( DMPP( buf));			/* free buffer */

	if (rc && pMsg)			// if err and caller wants msg
		*pMsg = msg( NULL,		// retrieve msg w/ args in Tmpstr, messages.cpp
					 rc,		// MH for err
					 fName,				//   arg: name of file w/ error
					 msgSec(sec));		//   arg: sec text, messages.cpp

	return rc;			/* one other return above */
}		/* xfCopy2 */
//=============================================================================
SI FC xffilcmp(		// Compare text files

	const char *f1,		// Pathname of first file
	const char *f2 )	// Pathname of second file

/* Return values (changed 3-1-90, no effect on callers)
     -2:  Files are different due to file 2 non-existence or IO error
     -1:  Files are different due to file 1 non-existence or IO error
      0:  Files are identical
      1:  Files are different */
{
	SEC sec1, sec2;
	SI rv;
	XFILE *xf1,*xf2;
#define FCLLEN 1024	/* buffer size */
	char b1[FCLLEN],b2[FCLLEN];

	/* Open files: read text, ignore errors */
	xf1 = xfopen( f1, O_RT, IGN, FALSE, &sec1);
	if (sec1 != SECOK)
	{
		rv = -1;
		goto doneOpen0;		/* file 1 open failed */
	}
	xf2 = xfopen( f2, O_RT, IGN, FALSE, &sec2);
	if (sec2 != SECOK)
	{
		rv = -2;
		goto doneOpen1;		/* file 2 open failed */
	}

	/* Loop reading both files until a difference is found */
	while (1)
	{
		sec1 = xfread( xf1, b1, FCLLEN);		/* read file 1 */
		if (sec1 != SECOK && sec1 != SECEOF)
		{
			rv = -1;
			break;				/* file 1 IO error */
		}

		sec2 = xfread( xf2, b2, FCLLEN);		/* read file 2 */
		if (sec2 != SECOK && sec2 != SECEOF)
		{
			rv = -2;
			break;				/* file 2 IO error */
		}

		if (xf1->nxfer != xf2->nxfer)	/* if different # bytes xfer'd */
		{
			rv = 1;			/*    files differ */
			break;
		}
		/* nxfers are same */
		if (xf1->nxfer == 0)		/* if nxfers both 0 (both EOF) */
		{
			rv = 0;			/*    then files are identical */
			break;
		}
		if (memcmp( b1, b2, xf1->nxfer))	/* if buf contents differ */
		{
			rv = 1;			/*    not same, files differ */
			break;
		}
	}	/* buffer loop */

	/* Close files ignoring impossible(?) errors */
	xfclose( &xf2, &sec2);
doneOpen1:	/* come here on error opening file 2 */
	xfclose( &xf1, &sec1);
doneOpen0:	/* come here on error opening file 1 */
	return rv;

}		/* xffilcmp */
//=============================================================================
SEC xfclear(	// Cleans the file by discarting all the data in the file
	XFILE* xf)	// File to be clean
	/* Returns: SECOK on sucess or
				SECOTHER on failure
	*/
{
	xlsterr(xf);				// clear any prior xiopak error (else addl calls will nop)
	if ( (freopen(xf->xfname, "w", xf->han)) != nullptr			// Reopen with "w" to discard all the data in the file
		&& ((freopen(xf->xfname, xf->access, xf->han)) != nullptr))	// Reopen with the initial mode
	{
		return SECOK;   		/*    good */
	}
	xf->xflsterr = xioerr(xf);		// report error
	return xf->xflsterr;	/*    bad: report error */
}
//=============================================================================
void xfpathroot(	// Gets the drive letter followed by a colon (:)
	const char* path,	// Full path
	char* rootName)		// Output root name
{
	filesys::path filePath(path);
	strcpy(rootName, filePath.root_name().string().c_str());
}		/* getPathRootName */
//=============================================================================
void xfpathdir(	// Directory path including trailing slash
	const char* path,		// Full path
	char* rootDirectory)	// Output root directory
{
	filesys::path filePath(path);
	strcpy(rootDirectory, filePath.root_directory().string().c_str());
	strcat(rootDirectory, filePath.relative_path().parent_path().string().c_str());
	strcat(rootDirectory, filePath.root_directory().string().c_str());
}		/* getPathRootDirectory */
//=============================================================================
void xfpathstem(	// Base filename (no extension)
	const char* path,	// Full path
	char* rootStem)		// Output root stem
{
	filesys::path filePath(path);
	strcpy(rootStem, filePath.stem().string().c_str());
}		/* getPathRootStem */
//=============================================================================
void xfpathext(	// Filename extension (including leading period (.))
	const char* path,	// Full path
	char* fileExtension)// Output file extension
{
	filesys::path filePath(path);
	strcpy(fileExtension, filePath.extension().string().c_str());
}		/* getPathExtension */
//=============================================================================
bool xfhasroot( // Check if it contains a root
	const char* filePath) // Path
{
	return filesys::path(filePath).has_root_name();
}		/* xfhasroot */
//=============================================================================
bool xfhasrootdirectory( // Check if it contains a root directory '/'
	const char* filePath) // Path
{
	return filesys::path(filePath).has_root_directory();
}		/* xfhasdirectory */
//=============================================================================
bool xfhasext(			// Checks if the path contains an extension
	const char* filePath)	// Full path
{
	return filesys::path(filePath).has_extension();
}		/* hasExtension */
//=============================================================================
void xfjoinpath(			// Joins two directory path together
	const char* pathname1,	// directory path
	const char* pathname2,	// name of the file or another directory path
	char* fullPath)			// OUTPUT: path created
// Depending on the system the slash will either be forward or double backwards
{
	filesys::path directoryPath(pathname1);
	filesys::path name(pathname2);
	directoryPath /= name;
	strcpy(fullPath, directoryPath.string().c_str());
}  /* xfjoinpath */
//=============================================================================
bool xfisabsolutepath(// Checks whether the path is absolute
	const char* path) // Input path
// Returns true if the path, in native format, is absolute, false otherwise.
// Note: The path "/" is absolute on a POSIX OS, but is relative on Windows.
{
	bool result = filesys::path(path).is_absolute();
	return result;
}  /* xfisabsolutepath */
//=============================================================================


// end of xiopak.cpp
