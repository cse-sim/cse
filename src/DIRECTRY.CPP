// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// directry.cpp -- directory-related functions

// formerly part of dospak.cpp; fcns here called ONLY from xiopak2 (3-90)


//------------------------------- INCLUDES ----------------------------------
#include <CNGLOB.H>

#include <XIOPAK.H> 	// fileinfoStr, #includes fcntl.h
#include <TDPAK.H> 	// tddiw

#include <directry.h>	// decls for this file


//----------------------- LOCAL FUNCTION DECLARATIONS -----------------------

LOCAL void    FC NEAR dpflstuf( const WIN32_FIND_DATAA *, FILEINFO *);

//=============================================================================
FILEINFO * FC dpgetdir(  // search (current) dir for all files matching path

	const char *path )   	// template for search (can include * and ?)

/* returns a pointer to a dm array of FILEINFO structs, containing filename,
   attribute, size, and time/date last written (see xiopak.h) of each file
   matching path.  The end of the array is indicated by a NULL filename. */

// caller must dmfree() ret'd pointer when finished

// returns NULL if no files match or could not get memory for table.
{
	FILEINFO *pbuf = NULL;			// NULL to return if dmal fails or 0 files found
	SI nf = dpflcnt(path);			// count files matching path
	if (nf)
	{
		if (dmal( DMPP( pbuf), 			// alloc dm (heap space), dmpak.cpp / if ok
		(nf+1) * sizeof( FILEINFO),	// include space for term
		WRN | DMZERO )==RCOK)		// don't abort, set to 0
		{
			nf = 0;
			while (dpfindfl( pbuf+nf, nf ? (const char *)NULL : path))
				nf++;
			(pbuf+nf)->flname[0] = 0;	// insurance terminator
		}				// else: allocation failed, pbuf is NULL
	}			// else: 0 files, pbuf is NULL
	return pbuf;
}			// dpgetdir

//=============================================================================
SI FC dpflcnt(	// count the number of files matching path

	const char *path )  	// template for search (can include * and ?)

// returns number of files matching path
{
	SI nfiles = 0;
	while (dpfindfl( NULL, nfiles ? (const char *)NULL : path))
		nfiles++;
	return nfiles;
}			// dpflcnt

//-----------------------------------------------------------------------------
// reference info from BC4 winnt.h 2-94. ok to delete.
//typedef struct _WIN32_FIND_DATAA {
//    DWORD dwFileAttributes;
//    FILETIME ftCreationTime;
//    FILETIME ftLastAccessTime;
//    FILETIME ftLastWriteTime;
//    DWORD nFileSizeHigh;
//    DWORD nFileSizeLow;
//    DWORD dwReserved0;
//    DWORD dwReserved1;
//    CHAR   cFileName[ MAX_PATH ];
//    CHAR   cAlternateFileName[ 14 ];
//} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
//=============================================================================
SI FC dpfindfl(	// Find first or next matching file in directory search

	FILEINFO *finfo,	/* NULL or Pointer to FILEINFO structure to receive
			   info about matching file */
	const char *path ) 	/* Template for search (* and ? are OK)
			   If not NULL, this is an initial call and the first
			   file matching path is located.  If NULL, path is
			   ignored and the next file matching the path of the
			   previous FIRST call is located. */

/* Returns TRUE if a matching file is found, FALSE if no match is found.
   If match found, *finfo is set to info about file (unless finfo NULL). */
{
	// Windows assumed. intdosx not available 2-94.
	// probably never called in CSE --> 32 bit code UNTESTED.

	static WIN32_FIND_DATAA dtabuf = { 0 };
	static HANDLE hFindFile = 0;
	if (path)			// path given means first call, find first file
	{
		/* inregs.x.cx = 2 + 4 + 16;
		   meaning of cx in code below not determined 2-94.
		   If it is attributes to include or exclude,
		   would need to select on return per dtabuf.dwFileAttributes bits (winnt.h):
		FILE_ATTRIBUTE_HIDDEN           0x00000002
		FILE_ATTRIBUTE_SYSTEM           0x00000004
		FILE_ATTRIBUTE_DIRECTORY        0x00000010 */

		hFindFile = FindFirstFile( path, &dtabuf);
		if (hFindFile==INVALID_HANDLE_VALUE)
			// use GetLastError for more info
			return FALSE;
	}
	else			// not first call, find next file
	{
		if (!FindNextFile( hFindFile, &dtabuf))
			// use GetLastError for more info
			return FALSE;
	}
	// found a(nother) file if here
	if (finfo != NULL)			// no info return if no buffer
		dpflstuf( &dtabuf, finfo);	// WIN32_FIND_DATAA --> FILEINFO, below
	return TRUE;
}		// dpfindfl

//=============================================================================
LOCAL void FC NEAR dpflstuf(	/* Convert DOS DTA buffer or WIN32_FIND_DATAA
				   from a directory search to accessible structure */

	const WIN32_FIND_DATAA * pdta,	// winnt.h struct returned by FindFirstFile / FindNextFile, as in dpfindfl
	FILEINFO *fibuff )			// structure (xiopak.h) to receive file information
{
	struct DOSDATE
	{
		unsigned day:5;
		unsigned mon:4;
		unsigned year:7;				// since 1980
	};

	struct DOSTIME
	{
		unsigned sec:5;				// second/2
		unsigned minute:6;
		unsigned hour:5;
	};

// Windows assumed. intdosx not available 2-94.
// for compatibility, convert to historical FILEINFO structure.
// probably never called in CSE --> 32 bit code UNTESTED.

//file attributes. win32 bits match xiopak.h defines (except no win32 define 8 (vol label)).
	fibuff->flattr = char(pdta->dwFileAttributes);

//date & time last modified
	DOSDATE dlm;
	DOSTIME tlm;
	FileTimeToDosDateTime( &pdta->ftLastWriteTime, (LPWORD)&dlm, (LPWORD)&tlm);
	fibuff->fltlm.hour = tlm.hour;
	fibuff->fltlm.min  = tlm.minute;
	fibuff->fltlm.sec  = tlm.sec*2;
	fibuff->fltlm.year	= dlm.year + 80;		// make 1900-based
	fibuff->fltlm.month = dlm.mon;
	fibuff->fltlm.mday	= dlm.day;
	tddiw( (IDATE *)&fibuff->fltlm);			// determines day of week, sets .wday. tdpak.cpp.

//file size
	fibuff->flsize = pdta->nFileSizeLow;		// hi DWORD of file size is lost.

//file name
	strcpy( fibuff->flname, pdta->cAlternateFileName);		// use short (8.3) form

}						// dpflstuf

// end of directry.cpp

