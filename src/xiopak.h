// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// XIOPAK.H:  Definitions for extended IO package

#if !defined( _XIOPAK_H)
#define _XIOPAK_H

/*-------------------------------- DEFINES --------------------------------*/
// xfcopy() erOp option
const int XFCOPYTEXT=EROP1;	// terminate input file at ctrl-Z:
							// use when copying a text file which may later be re-opened for appending:
							// corrects WordStar's vestigial cp/m style file termination,
							// to prevent additional appended stuff from dissappearing. 11-90, rob.
// xfopen() erOp options
const int XFBUFALLOC=EROP1;	// allocate character buffer in xfOpen()

//---------------------------- COMPILE OPTIONS ------------------------------


/* xfopen() modes.  The following to some degree use the DG notation for file status:
	old:	   File must exist at open, error if it does not
	fresh:	   File is created if necessary at open or overwritten
		   if it already exists.  Any old data is lost.
	unknown:   File is created if it does not exist but preserved if
		   it does.  This is most applicable when appending. */
// Text modes
#define O_RT		"r"				// Read
#define O_WT		"w+"			// r/w (fresh)
#define O_WTOLD		"r+" 			// r/w (must exist)
#define O_WTOLDA	"a+"			// r/w (append)
#define O_WTUNKA	"a+"			// r/w append, create if nec

// Binary
#define O_RB		"rb"			// Read
#define O_WB		"wb+"			// r/w (fresh)
#define O_WBOLD		"rb+"			// r/w (old, must exist)
#define O_WBOLDA	"ab+"			// r/w append (must exist)
#define O_WBUNK		"rb+"			// r/w (create if nec.)
#define O_WBUNKA	"ab+"			// r/w append (create if nec.)

///////////////////////////////////////////////////////////////////////////////
// struct XFILE: extended IO packet set up in dm by xfopen, used with other xiopak fcns
///////////////////////////////////////////////////////////////////////////////
struct XFILE
{   FILE* han;		// File handle
    char access[20];	// Access flag used on last fopen()
    size_t nxfer;		// # bytes actually transfered on last xfread/write.
					// 0 after call w/ nbytes=0; 0 after call against file with .xflsterr != SECOK
    SEC xflsterr;	// Last error (since xlsterr()), SECOK if none
    int xferact;		// Error action (message) code: IGN, WRN, ABT
    int xfeoferr;	// EOF message code: TRUE see xferact, FALSE no msg (but SECEOF returned & still need to call xlsterr)
    const char* xfname;	// Pointer to full path of file (in dm)
					// members re char access method (xiochar.cpp)(not linked in CSE, 2-95
    int bufsz;		// allocatd size of .buf[], 0 or 512.  see xfopen.
    int bufn;		// # of chars in buf. 0'd: xiopak:xfopen, xfseek.
					//   set: xiochar:cPeekc. ref: xiopak:xfread, xftell; xiochar:cPeekNB
    int bufi;		// next char subscript. set: xiochar:cPeekc, xiopak:
    		        //    xfread.  ref: xiopak:xftell, xiochar:cPeekNB */
    int line;		// line number maintained by xiochar:cGetc(). NOT correctly maintained by xfseek.
					// ALSO .line can be high during cToken reads due to lookahead at ws and comments
    char xf_buf[1];	// char mode buffer.  Must be at end of XFILE.
    			    // xiopak:xfopen dmallocs expanded XFILE if erop contains XFBUFALLOC (e.g. from xiochar:cOpen).
					//	 NB: bfr'd chars are read BINARY, xiochar:cPeekc emulates text mode at char removal
					//   (why: allows correct xiopak:xftell on ^Z terminated files)
};	// struct XFILE

///////////////////////////////////////////////////////////////////////////////
// class Path: holds paths and find files using paths.
///////////////////////////////////////////////////////////////////////////////
class Path
{
   char* p;		// pointer to "" or one or more ;-separated drive-directory paths

public:
   Path() { init(); }
   void init() { p = NULL; }
   ~Path() { clean(); }
   void clean() { if (p) delete[] p;  p = NULL; }

   void add( const char *s);					// add path(s) to object, NULL for DOS path
   BOO find( const char *fName, char *buf); 	// find file, return full path in buf[FILENAME_MAX]
};		// class Path
//---------------------------------------------------------------------------

// public functions
void   FC xfClean();

SEC    FC xfdelete( const char *, int erOp=WRN);
SEC    FC xfrename( const char *, const char *, int erOp);
int xfWriteable( const char* fPath, const char** ppMsg=NULL);
int xfExist( const char* fPath, char* fPathChecked=NULL);
int findFile1( const char* drvDir, const char* fName, char* fPathChecked=NULL);
BOO findFile( const char* fName, const char* path, char* fPathFound);	// searches . first. NULL path searches env path.

const char * FC xgetdir( void);
SEC    FC xchdir( const char*, int erOp=WRN);
RC     FC xfGetPath( const char * *, int erOp=WRN);
XFILE* xfopen( const char* fname, const char* access, int erOp=WRN, BOO eoferr=FALSE, SEC *psec=NULL);
SEC    FC xfclose( XFILE **xfp, SEC *pCumSec=NULL);
SEC    FC xfread( XFILE *, char *, USI);
SEC    CDEC xfprintf( XFILE *, const char *, ... );
SEC    FC xfwrite( XFILE *, const char *, size_t);
SEC    FC xftell( XFILE *, LI *);
SEC    FC xfseek( XFILE *, LI, int mode);
SEC    FC xfsize( XFILE *, LI*);
SEC    FC xlsterr( XFILE *);

RC     FC xfcopy( const char *, const char *, const char*, SI);
RC     FC xfcopy2( XFILE *xfs, XFILE *xfd, int op, const char** pMsg);
uintmax_t FC xdisksp();
uintmax_t FC dskDrvFreeSpace();
SI     FC xffilcmp( const char * fileName1, const char * fileName2 );
SEC	xfclear(XFILE* xf);
void createPath(const char* path, const char* filename, char* fullPath );
void getPathRootName(const char* path, char* rootName);
void getPathRootDirectory(const char* path, char* rootDirectory);
void getPathRootStem(const char* path, char* rootStem);
void getPathExtension(const char* path, char* fileExtension);
bool hasExtension(const char* filePath);

#endif	// _XIOPAK_H

// end of xiopak.h
