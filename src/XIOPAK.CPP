// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// XIOPAK.CPP: Extended IO and file functions

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

// #define OPENTEST	 // define for debugging xiopak:xfopen and xiopak2:xFindOnPath. old 2-95.

#define MESSAGESC	// define to link with messages.cpp and accept message handles in xfprintf.
					// undefine to link without messages.cpp

//#undef WANTXFSETUPPATH	define in xiopak.h to restore xfFindOnPath, xfSetupPath code, 3-95

//#define USEFOP	// defined in xiopak.h to use fop.cpp not local path code, 2-95.

/*------------------------------- INCLUDES --------------------------------*/

#include <direct.h>	// getcwd, chdir, _chdrive
#include <sys\types.h>	// reqd b4 stat.h (defines dev_t)
#include <errno.h>	// ENOENT
#include <fcntl.h>	// O_TEXT O_RDONLY
#include <sys\stat.h>	// S_IREAD, S_IWRITE

#include <msghans.h>	// MH_xxxx defns
#include <messages.h>	// msgSec()

#include "tdpak.h"
#include "cvpak.h"

#include "xiopak.h"	// xiopak.cpp publics

#undef WANTXFSETUPPATH  // define if want xfSetupPath() and xfFindOnPath() and path seach in xfopen():
						// Replacing this stuff with various C lib calls and caller local path searches


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL SEC FC NEAR xioerr( XFILE *xf);


/* ************************** CLEANUP ROUTINE **************************** */

//=============================================================================
void FC xfClean()	// cleanup routine
// added for DLL version which should free every byte of allocated heap space, rob, 12-93.
{
#ifdef WANTXFSETUPPATH
0 #ifdef USEFOP
0     xPath.clean();			// free paths in xiopak's path-holding object
0 #else
0 0// free heap array of heap paths if allocated
0 0    if (Xpath)					// NULL or points to NULL-terminated block of pointers
0 0    {  for (XPATH * * xp = Xpath;  *xp;  xp++)	// loop over pointers in block
0 0          dmfree( DMPP( *xp));    		// free block for one pointer, set pointer NULL
0 0       dmfree( DMPP( Xpath));			// lastly, free the block of pointers
0 0		}
0 #endif
#endif
}		// xfClean
//======================================================================
SEC FC xfdelete(	// delete a file

	const char *f,		// Pathname of file to delete (nop if NULL)
	int erOp/*=WRN*/)		// Error action code -- ABT, WRN, IGN, etc

// Returns SECOK or error code

{
	SEC sec = SECOK;
	if (f)
		if (unlink(f) != 0)
		{
			sec = errno;
			err( erOp, 			// display msg per erOp, rmkerr.cpp
				 (char *)MH_I0010,	// "I0010: unable to delete '%s':\n    %s"
				 f,			//    filename
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
			 (char *)(LI)MH_I0021,	// "I0021: Unable to rename file '%s' to '%s':\n    %s"
			 f1, f2,			//    filenames for message
			 msgSec(sec));		//    sec text for message (messages.cpp)
	}
	return sec;
}		// xfrename
//======================================================================
const char * FC xgetdir()

/* Returns pointer to current working drive/directory pathname in Tmpstr[] */

{
	char * p = strtemp( FILENAME_MAX );	// alloc n+1 bytes tmp strng space, strpak.cpp
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
	BOO eoferr/*=FALSE*/,	// If TRUE, treat EOFs on this file as errors
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

#if (defined (USEFOP) | !defined(WANTXFSETUPPATH))	// to use fop.cpp path stuff or no path stuff, 2,3-95. xiopak.h consts.
	/* Eliminates syntax scan of filename and MUCH! code.
	   May make path-using rules more liberal;
	   makes consistent with other fop.cpp-using code in CSE. */
#ifdef WANTXFSETUPPATH // worked, 2-95
0// if existing file expected, find it fisrt on path(s) set up with xfSetupPath
0   BOO gotPath = FALSE;
0   if (!(accs & O_CREAT))		// if existing file requested (note pre-USEFOP code responded to errno==EMNOENT re paths)
0   {  char buf[_MAX_PATH];		// receives full path
0      if (xPath.find( fname, buf))	// find file, return full path / if succeeded
0      {  fname = buf;			// continue, using the full pathname. CAUTION: must put in dm (not stack) b4 return.
0         gotPath = TRUE;
0	}	}		// if not found, fall thru with user-given name, let open() fail
#endif

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
#ifdef WANTXFSETUPPATH
	if (!gotPath)			// if we didn't get full path above
#endif
		xfGetPath( &fname, WRN);		// get full path -- replaces xfname with Tmpstr pointer
	xf->xfname = strsave(fname);	// put in dm
	return xf;				// success.  Other returns above.
#else
0/* file name syntaxCheck, init path to open, set local globals for xfNextPath*/
0    if (xfStartPaths( fname,		/* name from caller */
0		      &xf->xfname,	/* receives name with .\ for curr dir*/
0		      erOp ) != RCOK)	/* issue messages per erOp */
0    {  if (psec)
0          *psec = SECBADFN;		/* our code for bad file name */
0       goto er;		/* bad name return: go free xf and return NULL */
0	}
0
0/* loop to attempt open in current dir, then with each path, til found */
0
0   while (1)	/* path loop.  First time uses null path (curr drive/dir). */
0	{
0   /* attempt open / leave loop if succeed */
0       if ( (xf->han = open( xf->xfname, accs, S_IREAD|S_IWRITE)) != -1)
0			break;
0
0 #ifdef OPENTEST
0 0      printf( "open failed: %s\n", xf->xfname);
0 #endif
0
0   /* if not found (other errors will not be cured by adding path),
0      if we have another compatible path, try the open again */
0 #ifdef PCMS
0       if (errno==ENOENT)
0/*#else optionally redo appropriately for other systems -- adds speed. */
0 #endif
0	  if (xfNextPath( &xf->xfname)) 	/* updates xf->fname */
0	     continue;
0
0   /* fail */
0       xf->xfname = fname;	// replace proper name
0       sec = xioerr(xf);	// resolve sys err code & report error
0       if (psec)
0          *psec = sec;		// return SEC
0    er:			/* here to release XFILE and return NULL */
0       dmfree( DMPP( xf));
0       return NULL;		/* error return */
0	}
0
0// success
0 #ifdef OPENTEST
0 0   printf("Found: %s \n", xf->xfname);
0 #endif
0    xf->xflsterr = SECOK;		// say no error
0    if (psec)
0       *psec = SECOK;			// ..
0
0/* get full pathname, e.g. for later reopen even if dir changed.
0   Known that can get *WRONG* path after opening name.ext w/o dir spec
0   when a dpath (data path) program is active in system.
0   Code is being elsewhere to get full path of all files earlier.
0   No error expected if here, but do issue msg if error detected. */
0
0    xfGetPath( &xf->xfname, WRN);	// get full path -- replaces xfname
0    xf->xfname = strsave( xf->xfname);	// put in dm
0 #ifdef OPENTEST
0 0   printf("Opened File: %s   Handle: %d\n\n", xf->xfname, (INT)xf->han);
0 #endif
0    return xf;		// success.  Other returns above.
#endif	// USEFOP/WANTXFSETUPPATH...else...
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
	USI nX;

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

		nX = fread( buf, sizeof(char), nbytes/sizeof(char), xf->han)*sizeof(char);	// read

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
	USI nbytes) 	// Number of bytes to write. NOP if nbytes==0. Largest legal value 0xFFFE (0xFFFF rsrvd for error cond).

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

	XFILE *xf,	// Pointer to extended IO packet for file
	LI *fpos)	// Pointer to LI to receive current file position

/* attempts ftell() call w/o regard to prior errors so should return pos even
   on eofd file. */

/* Returns: SECOK if result is good.
            else other sec w/ *fpos = -1 (DOES NOT SET xf->xflsterr) */
{
	SEC sec;
	LI pos = ftell(xf->han);		/* get position at c library level */
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
	LI disp,	/* Byte displacement */
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

	XFILE *xf,			// Pointer to extended IO packet for file.  Must be open.
	uintmax_t *psize)	// Current size of file

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
#if 0			// Replace by xfClear 2/22
0SEC FC xfchsize(		// Change the size of a file
0
0	XFILE *xf,	// Pointer to extended IO packet for file
0	LI size)	// New size of file
0
0// Returns SECOK if no errors; else other sec (SETS xf->xflsterr)
0{
0	if (xfflush(xf) == SECOK) {  // Flush buffers; if no prior error and flush successful
0		std::error_code ec;
0		filesys::resize_file(filesys::path(xf->xfname), size, ec);
0		if (ec)		// set length, if good
0			xf->xflsterr = xioerr(xf);		//  failed, report err
0	}
0	return xf->xflsterr;
0}				// xfchsize
#endif
//======================================================================
#if 0			// Replace by xfClear 2/22
0SEC FC xfflush(		// Flush all buffers associated with a file
0
0	XFILE *xf ) 	// Pointer to extended IO packet for file
0
0// Returns SECOK if all OK, else other sec (SETS xf->xflsterr)
0{
0	if (xf->xflsterr==SECOK)			// if no prior error
0	{
0		// Check if the stream was open for writting
0		if (strchr(xf->access,'+')
0			|| strchr(xf->access, 'w')
0			|| strchr(xf->access, 'a')) {
0			
0			// flush buffers
0			if (fflush(xf->han) == 0)
0				return SECOK;			// success
0		}
0		xf->xflsterr = xioerr(xf);		// report error
0	}
0	return xf->xflsterr;	// return (possibly prior) sec
0}			// xfflush
#endif
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

	XFILE *xf)	// Pointer to extended IO packet for file

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
     XFILE *xf,	// Pointer to extended IO packet for file
     int erOp,		// New error action code for file: IGN, WRN, ABT, PWRN, PABT, etc (cnglobl.h)
     SI eoferr)	// If TRUE, treat EOFs as errors

{
     xf->xferact = erOp;
     xf->xfeoferr = eoferr;
}				// xeract
//======================================================================
LOCAL SEC FC NEAR xioerr(	// Resolve SEC and optionally report io error

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
	char* fPathChecked /*=NULL*/)	// ptr to optional buf[ _MAX_PATH]
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
	{	struct _stat stat;
		int statRet = _stat( tPath, &stat);
		if (statRet == 0)		// if file found
			ret = (stat.st_mode & _S_IFDIR)  ? 3
			    : (stat.st_mode & _S_IFREG)  ? 1 + int( stat.st_size > 0)
				:                              0;	// other (e.g. volume "C:")
		else if (statRet==-1 && errno == ENOENT)
			ret = 0;	// clean not found
		else
			ret = -1;	// unknown error
	}
	return ret;
}		// xfExist
//===========================================================================
int fileFind1(			// check existence of a single file
	const char* drvDir,		// drive and/or dir in which to look
							//   ignored if NULL or ""
	const char* fName,		// simple file name (with or w/o ext) check
	char* fPath /*=NULL*/)	// ptr to buffer [ _MAX_PATH] to receive
							//   assembled path that was checked
// returns 3 if dir
//         2 if non-empty file
//		   1 if empty file
//		   0 if not found or volume (C:)
//        -1 error (unexpected return from _stat)

{
	char tPath[ _MAX_PATH];
	int i = 0;
	if (drvDir && drvDir[ 0])
	{  	strTrim( tPath, drvDir);
		i = strlen( tPath);
		if (tPath[ i-1] != ':' && tPath[ i-1] != '\\')
			tPath[ i++] = '\\';		// add \ to dir if needed
	}
	strcpy( tPath+i, strTrim( NULL, fName));

	int ret = xfExist( tPath, fPath);	// check file
										// return exact name checked
	return ret;
}			// fileFind1
//-----------------------------------------------------------------------------
BOO findFile( 	// non-member function to find file on given path

	const char* fName, 		// file to find. Partial path ok; no path search if contains dir path beginning with '\'.
	const char* path, 		// path to search; NULL for DOS environment PATH; current dir always searched first.
	char* buf )				// receives full pathName if found; altered even if not found. array size _MAX_PATH

// returns TRUE if found (either file or directory)
{
	buf[ 0] = '\0';
	if (!fName)
		return FALSE;					// insurance: NULL pathname ptr is never found
#if 1
	char fNameFound[ _MAX_PATH];
	// lookup name as provided (no path)
	int found = fileFind1( NULL, fName, fNameFound);
	if (found == 0)
	{	// not found, maybe search on path
		char fRoot[ _MAX_PATH];		// drive (C:) or (\\server\)
		char fDir[ _MAX_PATH];		// directory
		_splitpath( fName, fRoot, fDir, NULL, NULL);		// extract drive and dir
		if (IsBlank( fRoot) && strTrimB( fDir)[0] != '\\')	// if fName has no drive and dir
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
	BOO bRet = FALSE;
	if (found > 0)
		bRet = _fullpath( buf, fNameFound, _MAX_PATH) != NULL;	// get full path, probably even if no such file
	if (!bRet)
		buf[ 0] = '\0';		// insurance
	return bRet;
#else
	char fDrv[ 10]; char fDir[_MAX_DIR];
	_splitpath( fName, fDrv, fDir, NULL, NULL);		// extract directory part of path
	if (IsBlank( fDir) || fDir[0]=='\\')			// if given fname's directory begins with '\'
	{
		// look for fName as given and search no paths.
		*buf = 0;					// (for consistency with _searchstr on fail returns)
		struct _finddata_t f;					// findfirst return info that we don't use
		if (_findfirst( fName, &f) < 0)		// C library: find file
			return FALSE;					//   fail now if file not found
		if (_fullpath( buf, fName, _MAX_PATH)==NULL)	// C library: get full path, probably even if no such file
			return FALSE;					//   (fail if bad drive or path too long -- unexpected)
		return TRUE;						// file found, full path is in buf
	}

// search path for file

	if (!path)						// if NULL given
		path = getenv("PATH");			// search DOS path
	char* p = strtmp( path);		// working copy
	const char* pToks[ 100];
	int n = strTokSplit( p, ";", pToks, 100);
	if (n < 0)
		n = 100;
	for (int i=0; i<n; i++)
	{	int ffRet = fileFind1( pToks[ i], fName);
		if (ffRet == 1)
			break;
	}


	return (*buf != 0);					// puts full path in buf, or "" if not found
#endif
}			// findFile

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
	strupr(q);				/* upper-case user-entered paths for uniform appearance 2-95
          				   (paths from system are upper case; most filenames get uppercased eg by strffix). */
	p = nup;				// store new pointer
}		// Path::add
//---------------------------------------------------------------------------
BOO Path::find(   	// find file using paths in Path object.
	const char *fName, 	// file to find. Partial path ok; no path search if contains dir path beginning with '\'.
	char *buf )   		// receives full path if found; is altered even if not found.
 						// array size must be >= _MAX_PATH (stdlib.h).
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
	XFILE *xfs, *xfd;
	SEC sec;
	uintmax_t reqspace, availspace;
	RC rc;
	SI eraMasked;
	const char *erArg;
	const char* mOrH;

	//rc = RCOK;			all paths set rc and BCC32 warns unused, 12-94
	erArg = NULL;
	eraMasked = erOp & ~EROMASK; 	// don't pass options to wrong fcns
	//   mask leaves only bits significant
	//   to error:errI.  cnglob.h
	sec = SECOK;          /* Used for error reporting below.
                         Bad I/O action sets sec to non-SECOK value */
	xfd = NULL;
	xfs = xfopen( fns, O_RB, IGN, FALSE, &sec); 	// IGN: we handle errs here
	if (xfs==NULL)
	{
		rc = MH_I0022;		// "I0022: file '%s' does not exist"
		mOrH = (char *)(LI)rc;
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
			mOrH = (char *)(LI)rc;
			erArg = fnd;
		}
		else
		{
			xfsize( xfs, &reqspace);	// required disk space
			availspace = xdisksp();		// available disk space
			if (availspace < reqspace + 2048)
			{
				rc = MH_I0025;		// "I0025: not enough disk space to copy file '%s'"
				mOrH = (char *)(LI)rc;
				erArg = fns;
			}
			else
				rc = xfcopy2( xfs, xfd, erOp, &mOrH);
		}
	}
	xfclose( &xfs, NULL);	/* close files, ignore errors */
	xfclose( &xfd, NULL);
	if (rc != RCOK)
		err( eraMasked,		// disp msg, rmkerr.cpp
			 mOrH,		// MH for msg or ptr to msg text from xfcopy2
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
	RC rc;
	char *buf;
	const char *ebuf, *fName = nullptr;
	USI nw;
	SEC sec;

	/* allocate memory for buffer */
	size_t bufsize = 0x8000;
	dmal( DMPP( buf), bufsize, ABT);

	/* loop, reading and writing buffer loads */
	rc = RCOK;
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
			nw = xfs->nxfer;
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
					 (char *)(LI)rc,	// MH for err
					 fName,			//   arg: name of file w/ error
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

// all if-outs below
#ifdef TEST
t/* ********************** UNMAINTAINED TEST CODE ************************** */
t #include <timer.h>
t main (argc,argv)
t
t SI argc;
t char *argv[];
t{
t SI i,iclose,n,pa[1000];
t LI pos, pos1, pos2;
t SEC sec, secSink, sec1, sec2;
t XFILE *x,*xx,*xf[50];
t char buf[100], c, cx;
t RC rc;
t
t /* general initialization */
t     dminit();	/* set up dynamic mem, dmpak.cpp */
t
t /* #define TESTCOPY */
t #ifdef TESTCOPY
t  #ifdef TESTDIR
t   #if 0	/* xpushdir/xpopdir eliminated 9-14-89 */
t X    xpushdir();
t   #endif
t      printf("\nDirectory is %s",xgetdir());
t      while (1)
t      {	printf("\nDirectory is %s",xgetdir());
t 	printf("\nNew dir: ");
t 	if (!scanf("%s",buf))
t 	   break;
t   #if 0	/* 9-14-89 */
t X	xpushdir();
t   #endif
t 	xchdir(buf,WRN);
t 	printf("\nDirectory is %s",xgetdir());
t   #if 0	/* 9-14-89 */
t X	xpopdir();
t   #endif
t	}
t   #if 0	/* 9-14-89 */
t X    xpopdir();
t   #endif
t  #endif	/* TESTDIR */
t
t     while (1)
t     {	printf("\nFile spec: ");
t        if (!scanf("%s",buf))
t           break;
t		 printf("\nPathname is '%s'",xgetpath(buf));    /* update to xfGetPath*/
t		}
t
t     tmrInit("Copy",0);
t     tmrInit("Comp",1);
t     printf("\nCopy test");
t     tmrStart(0);
t     rc = xfcopy(argv[1],argv[2],WRN);
t     tmrstop(0);
t     printf("\nrc = %d",(INT)rc);
t     printf("\nCompare test");
t     tmrStart(1);
t     i = xffilcmp(argv[1],argv[2]);
t     tmrstop(1);
t     printf("\ni = %d",(INT)i);
t     tmrdisp(stdout,0);
t     tmrdisp(stdout,1);
t #endif	/* TESTCOPY */
t
t #define TESTREAD
t #ifdef TESTREAD
t   #if 0
t     x = xfopen( "test", O_RT, WRN, FALSE, &sec);
t     n = 0;
t     while (1)
t     {		sec1 = xfread(x, buf, 1);
t        sec2 = xftell(x, &pos);
t        cx = isprint(buf[0])
t               ? buf[0]
t 	      : '.';
t        printf("sec1=%d %d (%c) sec2=%d pos=%ld\n",
t        	 (INT)sec1, (INT)buf[0], (INT)cx, (INT)sec2, pos );
t        if (sec1 != SECOK)
t           break;
t	}
t   #endif
t     cOpen( "test", &x);
t     n = 10;
t     for (i = 0; i < n; i++)
t     {
t        sec1 = xftell(x, &pos1);
t        pa[i] = (SI)pos1;
t        c = cGetc(x);	/* get char */
t        cx = isprint(c)
t               ? c
t 	      : '.';
t        sec2 = xftell(x, &pos2);
t        printf("%d pBef=%ld (sec=%d) %d (%c) pAft=%ld (sec=%d)\n",
t        	(INT)i, pos1, (INT)sec1, (INT)c, (INT)cx, pos2, (INT)sec2 );
t		}
t     for (i = 0; i < n; i++)
t     {  sec = xfseek(x, (LI)pa[i], SEEK_SET );
t        c = cGetc(x);	/* get char */
t        cx = isprint(c)
t               ? c
t 	      : '.';
t        printf("%d %d (%c) %d\n", (INT)i, (INT)c, (INT)cx, (INT)sec );
t	  }
t #endif	/* TESTREAD */
t
t #ifdef OPENTEST
t      for (i = 0; i < 50; i++)
t      {
t 			printf( "Open %d\n", (INT)i);
t 			sprintf(buf,"ootest%2.2d", (INT)i);
t 			xf[i] = xfopen(buf,O_WT,WRN,TRUE,&sec);
t
t 			iclose = i/2;
t 			if (i%3 == 1)
t 			{
t 				printf( "Closing %d  handle =%d\n", iclose, (INT)xf[iclose]->han);
t 				xfclose( &xf[iclose], &secSink);
t			}
t		}
t #endif	/* Open test */
t
t      return 1;		/* keep compiler happy */
t}
#endif		/* TEST */

#if defined( WANTED)
0* void   FC xeract( XFILE *, SI, SI);
0// File info structure used in xiopak and dospak
0// FILEINFO flattr attribute bits
0#define FILEAT_READONLY 0x01	// read only
0#define FILEAT_HIDDEN	0x02	// hidden file
0#define FILEAT_SYSTEM	0x04	// system file
0#define FILEAT_VOLLAB	0x08	// volume label
0#define FILEAT_DIR	0x10		// directory
0#define FILEAT_ARCH	0x20		// archive bit
0struct FILEINFO
0{   char flattr;			// file attribute
0    IDATETIME fltlm;		// date/time last modified
0    LI flsize;				// file size in bytes
0    char flname[ _MAX_PATH]; // file name (blanks removed, trailing '\0')
0};
#endif

#ifdef WANTXFSETUPPATH
0 #define USEFOP  	/* define to use lib\fop.cpp functions in place of xiopak's historical path stuff.
0			   Maintains much of external interface while making path manipulation consistent w/ other CSE path stuff.
0			   Reduces checking, liberalizes path concatenation, deletes oodles of code. 2-95. */
0 #ifndef USEFOP
0  const int XFCURRLAST=EROP2;	/* search for existing file in current directory last rather than first:
0 				   use for files that are part of program, such as msg file, to use copy in .exe's dir
0 				   in preference to copy in current (typically input file) dir. Rob 1-18-94. */
0 #endif
0 // xfSetupPath eract1 option
0 const int XFADDPATH=EROP3;   	// adding path(s) -- do not clear paths first. 2-95.
0 #ifndef USEFOP	// else believe xfScanFileName uncalled (and is big!)
0 // Structure for xfScanFileName information return
0  #ifndef GLOB				// to cnglob.h for other .h files
0   typedef struct xfninfoStr XFNINFO;
0  #endif
0 struct xfninfoStr
0 {
0  /* following apply to caller name string + caller default extension.
0     The "full path return" always returns drive and directorie(s),
0     but flags and lengths are NOT updated to reflect this. */
0    SI isDev;		// nz if reserved device name
0    SI hasDrive; 	// 0 no drive given by caller, else NUMBER OF CHARS in drive
0    SI hasDir;		// nz if has directory (followed by \)
0    SI hasName;		// nz if has name (not followed by \)
0    SI hasDot;		// dot seen after last \ (incl defualt ext given in call)
0    SI len;		// total length
0    SI pathLen;		// length of directory path portion
0    SI errCrsr;		// offset to position of error, if any
0    SI spare1, spare2;
0 };
0 #ifndef USEFOP	// def'd above, 2-95
0 // structure for paths Xpath points to
0 typedef struct
0 {   SI hasDrive, hasDir;
0     char path[1]; 	// null-terminated drive/directory path
0 }
0 XPATH;
0 #ifdef USEFOP
0  extern class Path xPath;	// xiopak Path object. declaration in fop.h.
0 #else
0 0 // path array pointer, used to find files by xfopen and xFindOnPath, set by xfSetupPath
0 0 extern XPATH ** NEAR Xpath;	// NULL or pointer to NULL-terminated array of pointers to XPATHs
0 #endif
0 #ifndef USEFOP	// else believe uncalled (and is big!)
0 RC     FC xfScanFileName( const char * * fnamp, const char *defext, SI optns, XFNINFO *ni, int erOp=WRN);
0 RC     FC xfFindOnPath( const char * *, SI);
0 void   FC xfSetupPath( const char *, RC *, SI, SI, RC *, SI);
0 #ifdef USEFOP//xiopak.h
0   Path xPath;			// path-holding, file-finding object. Used in xiopak, xiopak2.
0 #else//local path code
0 0  XPATH * * NEAR Xpath = NULL;	/* path array pointer: NULL or pointer to NULL-terminated array of ptrs to XPATHs.
0 0				   Used by xfopen & xfFindOnPath, set by xfSetupPath. Some uses are in xiopak2.cpp */
0 #endif
0 #ifndef USEFOP//xiopak.h
0 0 LOCAL const char * FC NEAR mPorF( SI);
0 #endif
0
0#ifndef USEFOP
0 #if 0
0 x LOCAL void NEAR addPaths( const char *, SI*, RC*, SI);
0 x LOCAL RC NEAR addaPath( const char *, SI*, SI);
0 #else//2-95
0  LOCAL void NEAR addPaths( const char *, RC*, SI);
0  LOCAL RC NEAR addaPath( const char *, SI);
0 #endif
0#endif
* LOCAL SI CDEC xfsrtcmp(FILEINFO *, FILEINFO *); 	// no NEAR/FC: ptr is passed
* LOCAL SI FC NEAR xfficnt( FILEINFO *files);
0#ifndef USEFOP
0 #ifdef PCMS	// if IBM PC. needs much rework for other systems.
0
0 //=============================================================================
0 RC FC xfScanFileName(	// file name or path scanner / analyzer / checker.
0
0     const char * * fnamp,	// input text, optionally REPLACED on return
0
0     const char *defext,	/* "" or default extension INCLUDING PERIOD.
0 			   Use optn 16 for return of name w/ ext defaulted. */
0
0     SI optns,		/* 1: off if looking for valid complete pathname,
0 			      on  if looking for dir path (to add name to)
0 					(4 should be off if 2 on)
0 			   2: off disallow DOS reserved device names
0 			      on  accept devices CON, CON:, COM, LPT1, etc
0 			   4: on  allow wild cards * and ? in name and type
0 			   8: on  verify existence of path.
0 				  SLOW - may hit disk if dirs specified.
0 			  16: on  RETURN updated name by REPLACING CALLER's POINTER.
0 				  Alone, returns default extension.
0 				  With 8, returns full path.
0 			  32: on  put rtrn string (optn 16) in DM (caller must free).
0 				  else is in Tmpstr. */
0
0     XFNINFO *ni,	/* on good return, receives stuff including the
0 			   following (this stuff doesn't reflect the drive and
0 			   dir that the "return full path" option can add):
0 	     .isDev:	nz if dos reserved device name
0 	     .hasDrive: 0 or NUMBER OF CHARACTERS in drive (letter-colon)
0 			as given by caller (option 8|16 always returns drive)
0 	     .hasDir:	nz if has a directory specification as given
0 	     .hasName:	nz if ends with name, name., or name.ext
0 			  (not followed by \ and with optns & 1 off)
0 	     .hasDot:	nz if name followed by . (after default extension
0 			  added if supplied)
0 	     .errCrsr:	approx string offset of error, if any (bad return)*/
0
0     int erOp/*=WRN*/)	/* error/message cntrl: IGN, WRN, ABT, PWRN, PABT (cnglob.h/rmkerr.cpp)*/
0
0 /* returns RCOK if appears to be good;
0    other value if obviously bad (invalid char, too long, a.b.c, etc). */
0{
0 SI i;
0 SI hasDrive;	/* 0 or NUMBER OF CHARACTERS in drive field */
0 SI fldLen;	/* length of current field (name or extension) */
0 SI nameLen;	/* length of name if in extension (dotsSeen nz), else 0 */
0 SI dotsSeen;	/* .'s seen since : or \, excluding .\ and ..\ */
0 SI inName;	/* now scanning name; may turn out to be dir or file name */
0 SI len, pathLen;
0 char c;
0 const char *fname, *p;
0 char fw[14], *fwp, wild[2];
0
0     ni->isDev = ni->hasDrive = ni->hasDir = ni->hasName = ni->hasDot = 0;
0     hasDrive = dotsSeen = inName = fldLen = nameLen = 0;
0     wild[0] =					/* no wild card */
0       wild[1] = '\0';
0     fname = *fnamp;
0     p = fname;
0
0 /* look for drive and device names */
0
0     for (fldLen = 0, fwp = fw;		/* extract first word */
0 	 c = toupper(*p),
0 	   (isupper(c) || isdigitW(c))	/* accept letters and digits */
0 	   && fldLen < (SI)sizeof(fw)-1; /* stop at buf size. falls thru ok.*/
0 	 * fwp++ =c, p++, fldLen++)
0 	;				/* empty for */
0     * fwp = '\0';                       /* terminate null */
0
0     if (fldLen > 0)	/* if any initial letters or digits */
0     {
0        ni->errCrsr = fldLen-1;	/* approx position of error, if errors now */
0
0        if (/* (fldLen==3 || fldLen==4)&& add 4 speed? */
0 	   strlstin( "CON AUX COM1 COM2 PRN LPT1 LPT2 LPT3 NUL", fw) )
0 					     /* if dos reserved device name */
0        {
0 	  if (!(optns & 2))
0 	     return err( erOp, (char *)MH_I0001, fname, fw, mPorF(optns));
0 			// "I0001: '%s' is a bad %s because '%s' is a
0 			//  DOS reserved device name"
0 	  if (c==':')           /* pass optional colon after device name */
0 	     c = *p++;
0 	  if (c)		/* cannot be anything but colon after name */
0 	     return err( erOp, (char *)MH_I0002, c, fw, fname);
0 			// "I0002: Invalid char '%c' after device
0 			//  name '%s' in '%s'",
0 	  ni->isDev++;
0 	  ni->len = (SI)(p-fname);
0 	  ni->pathLen = 0;
0 	  /* uppercase for caller if optns & 16? */
0 	  return RCOK;
0	}
0        else if (c==':' && fldLen==1)    /* 1 char and : can only be drive */
0        {
0 	  c = fw[0];		/* get the drive character */
0 	  if (dskDrvCheck( c, IGN) != RCOK)	/* check if good drive */
0 	     return err( erOp, (char *)MH_I0003,
0 	                 c, fname, mPorF(optns), fname);
0 				// "I0003: Bad drive letter '%c' in %s '%s'",
0 	  ni->hasDrive = hasDrive = 2;	/* NUMBER OF CHARACTERS in drive spec*/
0 	  //p++;			point past colon -- no further use of 'p' and BCC32 warns unused, 12-94
0 	  fldLen = 0;			/* next char starts new field */
0
}
0        else	/* other start with 1 or more letters already scanned */
0        {
0 	  inName++;		/* is name (may later be changed to dir) */
0 	  /* fldLen is set. */
0	}}
0
0 /* uppercase and add default extension if none -- AFTER doing device names */
0
0     char * fnamex = strffix( fname, defext);	// canonicalize to TmpStr. no longer const 10-94.
0     char * px = fnamex + hasDrive + fldLen;	// point past chars already scanned
0
0 /* loop to scan (directory) names separated by \'s */
0
0     while (c = *px, c != 0)
0     {
0        px++;
0        i = (SI)(px-fnamex);
0        ni->errCrsr = i-1;	/* probable position of error, if errors now */
0        switch (c)
0        {
0 	case ':':  	/* drive is processed above.  case is just to get
0 			   syntax error message instead of bad char msg. */
0 	 badSyntax:
0 		   return err( erOp, (char *)MH_I0004,
0 		               fnamex, mPorF(optns), c);
0 				// "I0004: '%s': bad %s syntax (at '%c').",
0
0 	case '\\': if (i >= 2 && *(px-2)=='\\')
0 		     goto badSyntax;		/* \\ */
0 		   if (dotsSeen > 1)
0 		     goto badSyntax;	/* max 1 more . in each dir name.ext */
0 	  isdir:
0 		   if (wild[0]) 	/* if wild card in directory portion */
0 		     goto badwild;	/* error return */
0 		   ni->hasDir++;	/* any \ indicates directory present */
0 		   inName = 0;		/* not a name */
0 		   fldLen = 0;		/* start new field */
0 		   nameLen = 0;		/* no name (b4 .) yet this field */
0 		   dotsSeen = 0;	/* no . yet this field */
0 		   break;
0
0 	case '.': if (dotsSeen)	/* (. and .. dirs done by lookahead) */
0 		     goto badSyntax;	/* a.b.c or ..., etc */
0 		   if (fldLen==0)	/* after \ : or start */
0 		   {
0 		     if (*px=='.')		/* look ahead for .. etc */
0 		       px++;
0 		     if (*px=='\0' || *px++ =='\\')
0 		       goto isdir;		/* . or .. then \ or end */
0 		     goto badSyntax;	/* else . or \. or :. is error */
0			}
0 		   /* is first . since start, \, or :, and fldLen > 0 */
0 		   dotsSeen++;
0 		   nameLen = fldLen;	/* length of portion before . */
0 		   fldLen = 0;		/* start new field */
0 		   break;
0
0 	default:	if (c <= ' '  ||  c > '~' 		/* char range check */
0 		    || strchr( "\"/[]|<>+=:;,", c) != NULL)	/* exceptions*/
0 		     return err( erOp, (char *)MH_I0005,
0 				c, mPorF(optns), fnamex);
0 				// "I0005: bad character '%c' in %s '%s'"
0 		   if (c=='*' || c=='?')
0 		   {
0 		     wild[0] = c;
0 		     if (!(optns & 4))
0 	  badwild:
0 		       return err( erOp, (char *)MH_I0006,
0 				     mPorF(optns), fnamex, wild);
0 			// "I0006: bad %s '%s': illegal use of wild card '%s'"
0		}
0 		   fldLen++;		/* count chars in each name or ext */
0 		   inName++;		/* assume name (not dir) for now */
0 		   break;
0
0	} }	/* while, switch */
0
0     ni->len = len = (SI)(px - fnamex);	/* total length */
0     ni->hasDot = dotsSeen;		/* nz if . in final name[.[ext]] */
0
0 /* option determines whether final name is dir name or file name */
0     if (inName)
0       if (optns & 1)
0         ni->hasDir++;
0       else
0         ni->hasName++;
0
0 /* length of dirctory path portion: subtract non-directory portions */
0     ni->pathLen =
0       pathLen = len - hasDrive		/* # chars in drive */
0 	- ( (optns&1) ? 0			/* name.ext are dir if path */
0 		  : nameLen + fldLen + dotsSeen);
0
0 /* if filename (not path) requested, it must have a file name */
0     if (!(optns & 1))
0     { if (!inName)		/* eg ends in : or \ or .., or is null */
0         return err( erOp, (char *)MH_I0007, fnamex);
0 	// "I0007: '%s' is supposed to be a file name, but there is no name part",
0 }
0 #ifndef EIGHTneedNotWorkWithONE
0     else
0 /* if path requested, add \ if necessary to connect right to file name */
0     { if (len > 0)
0       { if (strchr( ":\\", fnamex[len-1])==NULL)	/* if \ needed */
0 	{ fnamex = strtcat( fnamex, "\\", NULL);	/* add one */
0 	  len++;				/* update length for \. */
0 	  pathLen++;			/* update path length for check */
0 } } }
0 #endif
0
0 /* check length (protect against caller's array overflow) */
0     if (len > 2 + 64 + 8 + 1 + 3)
0        return err( erOp, (char *)MH_I0008, mPorF(optns), fnamex);
0 			// "I0008: %s '%s' is too long",
0     if (pathLen > 64+1)			/* 1 for always-included trailing \ */
0        return err( erOp, (char *)MH_I0009, fnamex);
0 		// "I0009: directory path portion of '%s' is too long"
0
0 /* do optional path check */
0     if (optns & 8)
0     {
0 #ifndef EIGHTneedNotWorkWithONE	/* another of these above */
0       if (optns & 1)				/* if want path, must add a file name*/
0          fnamex = strtcat( fnamex, "x", NULL);		/* add a name for xfGetPath */
0       RC rc = xfGetPath( (const char * *)&fnamex, erOp);	/* get full path: replaces fnamex */
0       if (rc != RCOK)					/* .. xfGetPath issues msgs per erOp*/
0          return rc;				/* bad path return */
0       len = strlen(fnamex);			/* length is now this */
0       if (optns & 1)
0          fnamex[ --len] = '\0';			/* remove added 1-char filename */
0 #else
0       RC rc = xfGetPath( (const char *)&fnamex, erOp);	/* get full path: replaces fnamex */
0       if (rc != RCOK)					/* .. xfGetPath issues msgs per erOp*/
0          return rc;				/* bad path return */
0 #endif
0 }
0
0 /* optionally return name with default ext, or path with \, or full pathName.*/
0     if (optns&(32))
0       fnamex = strsave(fnamex);	// return in DM on option (used with 16)
0     if (optns&16)			// on option
0     { *fnamp = fnamex;		// ret name w/ext, or path, to caller
0  #if 0	// to return lengths for optn 16: too much code til need shown
0 x	// and requires #ifndef EIGHTneedNotWorkWithONE stuff above
0 x      if (optns & 8)			// full path
0 x      hasDrive = 2;			// ALWAYS has drive (2 chars)
0 x      ni->pathlen += len - ni->len			// path len change is len change
0 x                     + ni->hasDrive - hasDrive;	// .. less drive len change
0 x      ni->len = len;					// store len (updated if full path)
0 x      ni->hasDrive = hasDrive;
0  #endif
0		}
0
0     return RCOK;				// good return.  many other returns in code above
0 }			// xfScanFileName
0 //================================
0 LOCAL const char * FC NEAR mPorF(	/* get "path" or "filename" per arg lo bit */
0
0     SI optns )		/* .. internal fcn for xfScanFileName errmsgs. */
0
0 {
0     return optns & 1 ? "path" : "filename";
0 }						/* mPorF */
0 #endif	// #ifdef PCMS 270 lines up
0#endif	// #ifdef USEFOP 272 lines up, 2-95
0
0#ifndef USEFOP // xiopak.h
0/********* xfopen internal functions for paths search ********/
0
0// variables shared by xfStartPaths and xfNextPath
0LOCAL XFNINFO NEAR xfNinfo;		// info about fname, from xfScanFileName
0LOCAL char NEAR xfDrv[3];		// contains drive and colon, or null string
0LOCAL const char * NEAR xffname;	// points start of caller's pathName. Rob 1-18-94.
0LOCAL const char * NEAR xfAftDrv;	// points portion of caller's fname after drive
0LOCAL SI NEAR xfPathI;  		// trial paths (Xpath) index
0LOCAL BOO xfTryCurr;			// TRUE for deferred search of current/specified dir under XFCURRLAST option
0
0//======================================================================
0RC xfStartPaths(	// syntax check file name, set up for path search, generate 1st path
0
0  const char * fname, 	// file name as given by xfopen or xfFindOnPath caller
0
0  const char * * pxfname, 	// receives pointer to first path to try: ".\" is inserted if no directory is specified
0
0  int erOp/*=WRN*/)	/* IGN, WRN, ABT, etc as usual, plus bit(s):
0			   XFCURRLAST:	1 to search for file in current directory last rather than first. Rob 1-18-94. */
0
0/* checks name, then sets shared variables and returns.
0   caller attempts open first with name as given by caller,
0   then calls xfNextPath to prepend successive trial paths
0   until NULL returned or open succeeds. */
0
0// returns RCOK unless error detected in file name.
0{
0// check and analyze file name.  global xfNinfo is used by xfNextPath.
0   RC rc = xfScanFileName( &fname, "", 0, &xfNinfo, erOp & (ERAMASK|PROGERR));
0   if (rc != RCOK)
0   {
0 #ifdef OPENTEST
0 0     printf( "Bad syntax: %s \n", fname);
0 #endif
0		return rc;			/* if clearly bad */
0	}
0
0// separate drive from rest of fname, so can easily insert path between
0    SI i;
0    for (xfAftDrv = fname, i = 0;
0		i < xfNinfo.hasDrive;		// 0 or NUMBER OF CHARS in drive spec
0		i++)
0			xfDrv[i] = *xfAftDrv++;	/* copy drive / advance ptr to rest */
0    xfDrv[i] = 0;
0    xffname = fname;			// also save ptr to entire fname, Rob 1-18-94
0
0// init to 1st path for xfNextPath
0    xfPathI = 0;			// init path index
0    xfTryCurr = FALSE;			// tentatively say curr dir already searched
0
0// 1st path to try if searching current dir last, 1-18-94
0    if ( (erOp & XFCURRLAST)		// if (xfopen) caller said search current dir last
0     &&  xfNextPath(pxfname) )   	// if found another path searchable with this xfname
0       xfTryCurr = TRUE;		// use it, tell xfNextPath to try current dir last
0    else
0
0   /* 1st path to try normally or if no other paths:
0      if in current directory, insert ".\" to make it explicit:
0      else dpath, if active in system, will find it in another directory, then
0      our code will replace current directory with full path and not be able to find it later. */
0
0       if (xfNinfo.hasDir)		/* if there is a dir in given path */
0          *pxfname = fname;		/* use it unchanged */
0       else
0          *pxfname = strtcat( xfDrv, ".\\", xfAftDrv, NULL);
0
0    return RCOK;
0}		// xfStartPaths
0//======================================================================
0SI FC xfNextPath(		// next trial path selector for path search
0
0  const char * * pxfname)	// where to return new (tmpstr) pathname for next open()
0
0	// and uses variables incl xfNinfo, xfDrv, *xfAftDrv, xfPathI
0
0/* returns TRUE if another pathname has been created for trial open() call,
0   FALSE if no more compatible paths to try. */
0{
0 #if 0
0 x XPATH *xp; char *defDir;
0 x
0 x   if (Xpath == NULL)
0 x      return FALSE;		/* have no paths */
0 x
0 x   while (1)		/* search for next combinable one */
0 x   {
0 x      xp = *(Xpath+xfPathI++); /* fetch next pointer to XPATH struct */
0 x      if (xp == NULL)		/* null indicates end of *Xpath */
0 x         return FALSE; 	/* no more paths */
0 #else //rearranged to support XFCURRLAST, 1-18-94
0    XPATH *xp = NULL;
0    char *defDir;
0
0    while (1)		/* search for next combinable path */
0    {
0       if (Xpath)		// if have any paths
0          xp = *(Xpath+xfPathI++); /* fetch next pointer to XPATH struct */
0       if (xp == NULL)		/* null indicates end of *Xpath or no paths (init NULL) */
0       {
0          if (xfTryCurr)		// flag set in xfStartPaths if XFCURRLAST specified, 1-18-94
0          {	xfTryCurr = FALSE;		// clear flag for next call
0
0             // return directory specified by caller (not expected to get here) or current dir
0             if (xfNinfo.hasDir)		// if there is a dir in given path
0                *pxfname = (char *)xffname;	// use it unchanged ((char *) undoes "const")
0             else
0                *pxfname = strtcat( xfDrv, ".\\", xfAftDrv, NULL);	/* supply .\ for no dir so unambiguous
0                							   eg re "dpath" tsr problems */
0             return TRUE;
0		}
0
0		return FALSE; 	/* no more paths to try */
0	}
0 #endif
0
0   /* when no directory string is present use ".\" for explicit curr dir:
0      prevents programs such as dpath finding it elsewhere, which will fail
0      later in this program after it retrieves full path and tries to use it.*/
0
0       defDir = (xfNinfo.hasDir || xp->hasDir)	/* if dir in path or name */
0			? ""                    /* null string */
0			: ".\\";                /* else explicit curr dir */
0
0   /* determine if path compatible with name */
0
0       if ( !xfNinfo.hasDir || !xp->hasDir )	/* if <= 1 dir spec */
0       {
0	  if ( !xfNinfo.hasDrive || !xp->hasDrive)	/* at most 1 drive */
0	  {
0	    /* for speed, should add code here to skip path if it only
0	       specifies current default drive and/or directory */
0
0	     *pxfname = strtcat(	/* make up pathname to try */
0			  xfDrv,	/* caller's drive, if any */
0			  /* drive is either in arg above or below or neither*/
0			  xp->path,	/* insert path after drive */
0			  defDir,	/* ".\" if no other directory */
0			  xfAftDrv,	/* rest of caller's fname after drive*/
0			  /* directory is in one of the last 3 args above */
0			  NULL );
0	     return TRUE;		/* now retry open() call */
0	}
0	  else if (((xfDrv[0]^xp->path[0])&~0x20)==0)	/* drives same,
0							ignoring case*/
0	  {
0	     if (xfNinfo.hasDir || xp->hasDir)	/* if > 0 dir specs */
0	     {
0		*pxfname = strtcat(	/* make up pathname, .. */
0			   xp->path,	/* with only 1 copy of drive */
0			   xfAftDrv,
0			   NULL);
0		return TRUE;
0		}
0	     /* else: same drive, no directory, would be repetition */
0	}
0	  /* else: 2 different drives, do not attempt to combine */
0}
0       /* else: both have directory specifications, do not combine
0	   (or should we put path in front of a relative dir spec?) */
0       /* note that we do combine directory spec from path or fname with
0		non-default drive from fname or path.  ok? */
0 #ifdef OPENTEST
0 0      printf( "reject path: %s\n", xp->path);
0 #endif
0	}	    // while (1): compatible path search loop
0    /*NOTREACHED */
0}			// xfNextPath
0#endif // #ifndef USEFOP
0 //=========================================================================
0 RC FC xfFindOnPath(	// find existing (input) file using path, and return full pathName.
0
0    // Searches path set up with xfSetupPath. Wild cards not allowed.
0
0         /* can be used when immediate open not desired,
0            and (we hope) gets correct full path even when dpath in system
0            (whereas xfopen can put wrong path in xf->fname). */
0
0    const char * *fnamp,	/* entry: desired file name, drive/directory optional.
0                    	   return: if found, REPLACED with ptr to full path, in dm.  CALLER must dmfree. */
0    int erOp )		// the usual error message control
0
0 // returns RCOK if found (*fnamp replaced); other value if not found or drive or directory not valid.
0{
0 #ifdef USEFOP	//xiopak.h 2-95
0     char buf[_MAX_PATH];		// receives full path if file found
0     if (!xPath.find( *fnamp, buf))	// search path(s) given in calls to xfSetupPath / if not found
0        return err( erOp,		// display err msg per erOp, rmkerr.cpp, return RCBAD
0                    (char *)MH_I0022,	// "I0022: file '%s' does not exist",
0 		   *fnamp );
0     *fnamp = strsave(buf);		// REPLACE caller's filename ptr with ptr to full path in dm. caller must dmfree.
0     return RCOK;
0 #else
0 0
0 0    const char *aPath; FILEINFO fi;
0 0  #ifdef OPENTEST
0 0  0 char *p;
0 0  #endif
0 0
0 0/* file name syntax check, set up first path to try (use .\ if curr dir),
0 0   and init internal globals used by xfNextPath */
0 0    RC rc = xfStartPaths( *fnamp, &aPath, erOp);  // messages per erOp
0 0    if (rc != RCOK)
0 0       return rc;			// name bad.
0 0
0 0// look to try name as given (current or given drive:dir\), then paths
0 0    while (1)
0 0    {  	/* note: prior version that used open was rejected becuase it found
0 0           wrong paths with dpath on (before .\ put into xfStartPaths). */
0 0       if (dpfindfl( &fi, aPath))	// use dos "find 1st", directry.cpp
0 0          break;			// if found
0 0
0 0  #ifdef OPENTEST
0 0  0     printf("find first failed: %s\n", aPath);
0 0  #endif
0 0
0 0   // if we have another compatible path, try again
0 0       if (xfNextPath( &aPath) )        // set aPath with next path if any
0 0          continue;
0 0
0 0  // Out of paths. Issue message, to be consistent with name syntax errors.
0 0       return err( erOp,		// disp err msg per erOp, rmkerr.cpp
0 0                   (char *)MH_I0022,	// "I0022: file '%s' does not exist",
0 0		   *fnamp);
0 0
0 0	}
0 0  #ifdef OPENTEST
0 0  0  p = aPath;				/* save for msg */
0 0  #endif
0 0    rc = xfGetPath( &aPath, WRN);    /* get full path ptr in aPath */
0 0    if (rc != RCOK)                     /* error NOT expected here */
0 0       return rc;                       /* but check anyway: insurance */
0 0    *fnamp = strsave( aPath);           /* ok. copy Tmpstr to dm. */
0 0  #ifdef OPENTEST
0 0  0  printf("found  %s  as  %s, \n  full path  %s.\n", *fnamp, p, aPath);
0 0  #endif
0 0    return rc;                  /* success. other returns in code above */
0 0
0 #endif // USEFOP...ELSE...
0}       			// xfFindOnPath
0 //=========================================================================
0 void FC xfSetupPath( 		// set up or add to paths for xfopen and xfFindOnPath to use
0
0     const char *s,  	// NULL or PATH string (one or more paths separated by ;'s)
0     RC* perf1,    	/* NULL or receives RCOK if no path errors in s; unspecified!
0 			   other value if syntax error or directory not found */
0     int erOp1,    	/* for s: IGN to issue no messages; WRN to issue warning
0 			   (continues after keypress); ABT to terminate program.
0 			   And option bit: XFADDPATH (xiopak.h) to not clear paths at entry, 2-95. */
0     SI pathFlag, 	// 1 to use paths from environment PATH variable
0     RC* perf2,   	// likewise for paths from environment PATH (or NULL)
0     int erOp2 ) 	// ..
0
0 #ifndef USEFOP
0 /* if both s and pathFlag are given, s paths are in front of PATH paths.
0    any prior paths in Xpath are lost unless XFADDPATH: dmfree is used if path not NULL.
0    (to save old Xpath value, copy elsewhere and set Xpath NULL first.) */
0
0 /* Arguments allow caller to record errors then recall later (after messages
0    set up) to issue messages; they also allow different level of error
0    reporting for s (command line) paths than system PATH. */
0 #else
0   #pragma argsused	// perf1,2, eract1,2 not be used (NULL / IGN in existing calls, 2-95)
0 #endif
0{
0 #ifdef USEFOP // xiopak.h 2-95
0 // clear paths (for old code; prob unnec)
0     if (!(eract1 & XFADDPATH))		// unless suppressed by option (added 2-95)
0        xPath.clean();			// delete any paths already in object
0
0 // add given paths
0     if (s)
0        xPath.add(s);
0     if (pathFlag)
0        xPath.add(NULL);
0 #else
0 0// clear paths
0 0    if (!(eract1 & XFADDPATH))	// unless suppressed by option (added 2-95)
0 0    {  XPATH** p = Xpath;
0 0       if (p != NULL)      	    // free existing path storage if any
0 0       {	while (*p != NULL)		// *Xpath is NULL-term array of ptrs
0 0          {	dmfree( DMPP( *p));	// free one path's dm
0 0				p++;
0 0			 }
0 0          dmfree( DMPP(Xpath));   	// free the pointer array dm
0 0          Xpath = NULL;            	// tell addPaths to allocate and init
0 0		}	}
0 0
0 0// add given paths
0 0    RC rc1=RCOK, rc2=RCOK;		// init to no errors
0 0 #if 0
0 0 x   SI xpal;        			// for number of allocated entries in Xpath
0 0 x   addPaths( s, &xpal, &rc1, eract1);	// add path(s) in s if not NULL
0 0 #else//2-95 delete xpal & recode addapath to make XFADDPATH work. more unif'd.
0 0    addPaths( s, &rc1, eract1);		// add path(s) in s if not NULL
0 0 #endif
0 0
0 0// add DOS PATH environment variable paths
0 0    if (pathFlag)
0 0       addPaths( getenv("PATH"), &rc2, eract2);
0 0
0 0    if (perf1)  *perf1 = rc1;		// return error codes if non-NULL pointers given
0 0    if (perf2)  *perf2 = rc2;
0 #endif // ifdef USEFOP ... else ...
0}				// xfSetupPath
0
0 #ifndef USEFOP
0 0
0 0/********* internal functions for xfSetupPath *********/
0 0
0 0//============================
0 0LOCAL void NEAR addPaths( 		/* break s up into ;paths; and add to Xpath */
0 0
0 0    const char *s, 	// NULL or pointer to string of ;-delimited paths
0 0 #if 0//2-95
0 0 x   SI *pxpal,  	/* storage for # allocated Xpath entries; is init and/or updated here */
0 0 #endif
0 0    RC *perf,		// returned nz if error(s) in path even if IGNRed.
0 0    int erOp )		// IGN, WRN, ABT, etc
0 0
0 0{
0 0   char ws[_MAX_PATH];	// Buffer for single path segment
0 0
0 0   /* *perf = RCOK;             * caller inits to no errors */
0 0   if (s != NULL)
0 0   {
0 0 /* Loop over ";" delimited segments of path, extract them and
0 0    pass them one by one to addaPath(), which validates them and adds
0 0    them to Xpath.  addaPath returns RCOK (which is 0) if good,
0 0    or'ing all returns together makes the overall return RC */
0 0
0 0    while (*s)		/* strxtok returns s pointing to terminating '\0'
0 0			   when no more tokens */
0 0       *perf |= addaPath( strxtok( ws,      /* seg. buffer */
0 0                                   s,      /* Working pointer */
0 0                                   ";",     /* Delimiter */
0 0                                   TRUE),   /* Trim WS from segs. */
0 0                          erOp);
0 0		}
0 0}       /* addPaths */
0 0
0 0//============================
0 0LOCAL RC NEAR addaPath( 		/* add path to Xpath */
0 0
0 0    const char *s, 	// pointer to a string to add as a path
0 0 #if 0//2-95 recoded to not need
0 0 x   SI *pxpal,		/* points to # allocated (not necessarily used) Xpath entries*/
0 0 #endif
0 0    int erOp )		// IGN, WRN, ABT, etc
0 0
0 0/* function value TRUE if ok (path added, or null or duplicate);
0 0   FALSE if not added due to bad syntax (check may be incomplete) */
0 0
0 0{
0 0 #define XPSZINC 4       /* Xpath allocation size increment.  Enlarge to maybe 16 after debugging */
0 0 SI xpi;
0 0 XFNINFO ninfo;
0 0 XPATH *xp;
0 0 RC rc;
0 0
0 0 #if 0	// 2-95: created if needed by dmral call below
0 0 x/* create Xpath pointer buffer if nec */
0 0 x   if (Xpath==NULL)
0 0 x   {
0 0 x      dmal( (void *)Xpath, XPSZINC * sizeof(XPATH**), ABT);
0 0 x      *Xpath = NULL;					/* terminate: mark empty */
0 0 x      *pxpal = XPSZINC;				/* size allocated */
0 0 x}
0 0 #endif
0 0
0 0// sanitize the path
0 0    char *sx = strTrim( NULL, s);	// deblank both ends, to TmpStr 10-94
0 0    strupr(sx);         		// upper case it, in place
0 0    if (!*sx)            		// ignore null paths, but do not indicate error
0 0       return RCOK;			// (xfopen automatically includes current dir)
0 0    /* reject if syntactically bad? */
0 0
0 0/* scan path (1) to determine if contains drive, directory; syntax check.
0 0   Make sure path exists on disk (8) (too slow?).
0 0   Do not have xfScanFileName return FULL path (16) -- keep user's. */
0 0    rc = xfScanFileName( (const char * *)&sx, "", 1|8, &ninfo, erOp);
0 0    if (rc != RCOK)
0 0       return rc;
0 0
0 0// if does not end in colon or \, append \ for combining with file name. to TmpStr.
0 0    sx = strtPathCat( sx, "");
0 0
0 0// check if duplicate / find # used entries
0 0    xpi = 0;
0 0    if (Xpath)
0 0       while (1)
0 0       {  xp = *(Xpath+xpi);
0 0          if (xp==NULL)
0 0             break;			// xpi is now next subscript to use */
0 0          xpi++;
0 0          if (!strcmp( sx, xp->path)) 	// do not store duplicates
0 0             return rc;			// but return good
0 0		  }
0 0
0 0// make up XPATH structure in dm block
0 0    dmal( (void *)xp, sizeof( XPATH) + strlen(sx), ABT);
0 0    xp->hasDrive = ninfo.hasDrive;
0 0    xp->hasDir = ninfo.hasDir;
0 0    strcpy( xp->path, sx);
0 0
0 0// create/enlarge Xpath if nec, then add the new entry
0 0
0 0 #if 1//2-95
0 0    if ( !Xpath					// if not yet allocated
0 0     ||  (xpi + 1)/XPSZINC > xpi/XPSZINC )	// or if next slot (and its NULL) goes into next increment of size
0 0       dmral( DMPP( Xpath),			// (re)allocate, dmpak.cpp
0 0              (xpi+1+XPSZINC)*sizeof(XPATH**), 	// eg 5,8,12,16.. for XPSZINC=4, xpi=3.
0 0              DMZERO|ABT );
0 0 #else
0 0 x   if (xpi+1 >= *pxpal)        		/* if NULL now in last slot */
0 0 x   {
0 0 x      dmral( DMPP( Xpath), (*pxpal+XPSZINC)*sizeof(XPATH**), ABT );
0 0 x      *pxpal += XPSZINC;
0 0 x	}
0 0 #endif
0 0    Xpath[xpi++] = xp;
0 0    Xpath[xpi] = NULL;			// NULL always termiates (redundant with DMZERO)
0 0    return rc;				// good.  other returns within function.
0 0 }			// addaPath
0 #endif // ifndef USEFOP
#endif // #ifdef WANTXFSETUPPATH
#ifdef WANTED
* //======================================================================
* SI FC xfsort(		// sorts a FILEINFO block
*
*    FILEINFO *files )	// FILEINFO block to sort
*
* // returns number of files in block
*{
*     SI n = xfficnt(files);
*     qsort( (void *)files, (size_t)n, sizeof(FILEINFO),
*            (INT (CDEC *)(const void *, const void *))xfsrtcmp);
*     return n;
*}		// xfsort
* //======================================================================
* LOCAL SI CDEC xfsrtcmp(		// sort compare for xfsort().  no NEAR/FC! ptr is passed
*
*     FILEINFO *f1,	// pointer to FILEINFO block 1
*     FILEINFO *f2 )	// pointer to FILEINFO block 2
*
*{
*     return strcmp( f1->flname, f2->flname);
*}
* //======================================================================
* LOCAL SI FC NEAR xfficnt(	// count entries in a FILEINFO chain
*
*     FILEINFO *files )	// FILEINFO list to count
*
* // returns number of files in FILEINFO
*{
*     SI n = 0;
*     while ((files+n)->flname[0])
*        n++;
*     return n;
*}                       // xfficnt
* //======================================================================
* FILEINFO * FC xffind(	// Searches for ALL files matching template
*
*     const char * templt )	// pathname, wildcards * and ? ok in name and type
*
* /* returns a pointer to a dm array of FILEINFO structs, containing filename,
*    attribute, size, and time/date last written (see xiopak.h) of each file
*    matching path.  The end of the array is indicated by a NULL filename. */
*
* // caller must dmfree() ret'd pointer when finished.
*
* // returns NULL if no files match or could not get memory for table.
*{
*     return dpgetdir(templt);	// build array of FILEINFOs, directry.cpp
*}
#endif

// end of xiopak.cpp
