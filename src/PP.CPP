// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// pp.cpp: CSE user language input file preprocessor


/*--- Preprocessor Features purport to include:

 all K & R #commands and preprocessor operators:
	#include, #if, #ifdef, #ifndef, #else, #endif, #define, #undef.
 #elif: ansi/usoft feature
 #redefine: #define with no error if already defined -- Chip's idea
 defined(x): 1 if x has been #defined, else 0 -- ansi/usoft feature
 #line number "name": is EMITTED in output text re #includes, false if's, etc.
 development/test aids:
   #cex constant-expression: evaluates and displays value AT PREPROCESS TIME
   #say text:		  displays rest of line AT PREPROCESS TIME
 does NOT have: ansi token-pasting and stringizing operators, # and ##.
 where in doubt about spec details, experiments were done with
   microsoft C 6.0 preprocessor.
*/

//--------------------------------- Notes -----------------------------------

/* Can it hang on \0 in input? detect and change to space. 9-90.
   YES! Addressed in ppRead(), 8-92. */

/* $eof in a macro reference arg list probably is imperfect 9-90 */

/* should accept and pass thru #line, but presently does not */

// another bug to fix: "#define X //comment"  puts a / into the defintion of X: should be null. 7-21-92. ??
// 2-95: may have fixed in fixing other pptok.cpp bugs. retest.

/*-------------------------------- OPTIONS --------------------------------*/


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <RMKERR.H>		// errI
#include <MESSAGES.H>	// msgI
#include <MSGHANS.H>	// MH_P0001

#include <ANCREC.H>	// getFileIx getFilename
#include "XIOPAK.H"

#include <PP.H>		// decls for this file for non-pp callers
#include <VRPAK.H>  	// vrStr
#include <CNCULT.H> 	// getInpTitleText


#include <CUTOK.H>	// CUTxxx token type defines, and CUTOKMAX 8-90.
#include <CUEVAL.H>	// pseudo-code: PS___ defines; printif
#include <CUPARSEI.H>	// CS__ defines, PR___ defines, OPTBL opTbl
#include <SYTB.H>	// for SYTBH syLu


/*--- #defines SYMBOL TABLE */
struct DEFINE : public STBK		// DEFINE: an defSytb value
{
	// char *id;	// name of macro (id after #define) (in base class)
	char* text;		// text of macro
	SI nA;			// number of arguments
	char* argId[1];	// nA dummy arg ids, NULL after last one
	// actual size nA+1.  nb sizeofs in code assumes the [1].
};
static SYTBH defSytb = { 0 };	// contains DEFINEs, manipulated by sytb.cpp fcns.

/*--- INPUT SOURCE STACK: source file, #included files, macros, macro args */
enum ISTY { tyNul, tyFile, tyMacro };
struct INSTK
{
	enum ISTY ty;	// indicates file or macro/arg
	USI fileIx;		// input file name index for use with ancrec.cpp:getFileName()
	char* mName;	// macro name (or arg name )text: NULL or pointer into theDef. Used?
	FILE* file;		// NULL or FILE pointer for fopen/fread/fclose
	SI nlsNotRet;	// # \n's skipped (in mac ref), macArgs() to ppC()
	int line;		// line # in file at char isf->i (ppC()) (file only)
	SI lineSent;	// 0 if #line command needs to be sent (file only)
	SI eofRead;		// non-0 if end file has been read
	SI noMX;		// suppresses macro expansion, ppC to ppmId
	// comment flags: 0 no, 1 /-/ comment, 2 /-* comment, -1 "text"
	SI inCmt;		// comment flag for text returned by ppM (i..j-1)
	SI cmtNx;		// cmt flag for text at .j (ppM() state for next call)
	SI isId;		// identifier flag, ppM to ppC
	USI bufSz;		// buffer allocated size (BUFSZ define above)
	USI n;			// #data bytes in buffer (subscript of last+1)
	USI i;			// subscr of next data byte for ppC() input
	USI j;			// last+1 byte avail to ppC(), next byte for ppM()
	USI ech;		// subscr of 1st byte NOT yet echoed to input listing
	char *buf;		// NULL or buffer location (in heap): file buffer, theDef->text, or argV[i].
	SI echLine;			// line # in file at buf[ech]===line # (of incomplete line) at end of lisBuf
	void /*DEFINE*/ *theDef;	// NULL or macro DEFINE block, from symol table, in heap (for arg names)
	char **argV;		// NULL or ptr to nA macro arg value ptrs + NULL
};
#define INDEPTH 20				// max nested sum: in_file+#includes+macros+args. Overflow (macOpen) is messy.
static INSTK inStk[ INDEPTH] = { { tyNul } };  	// the source stack
static INSTK* is = 0; 				// &inStk[inDepth-1]: curr inStk entry
static INSTK* isf = 0;				// curr inStk FILE (not macro) entry
static SI inDepth = 0;				// inStk level, 1..INDEPTH (not -1).

/*--- PREPROCESSOR COMMAND TABLE */
struct PPCTAB
{	const char* word;	// command keyword: if, define, etc.
	SI noMX;	// non-0 to suppress macro expansion in rest of line
	int cs;		// case (PPC defines)
};

// preprocessor cmd cases for ppcTab[] data and ppcDoI switch
enum PPCS
{	PPCINCLUDE = 1, PPCIF, PPCIFDEF, PPCIFNDEF, PPCELIF, PPCELSE,
	PPCENDIF, PPCDEFINE, PPCREDEFINE, PPCUNDEF, PPCCLEAR, PPCCEX, PPCSAY,
	PPCECHOON, PPCECHOOFF
};

// preprocessor command table
static PPCTAB ppcTab[] =
{
	// --word--  -noMX-  --case--
	{ "include",  1,    PPCINCLUDE },
	{ "if",       0,    PPCIF },
	{ "ifdef",    1,    PPCIFDEF },
	{ "ifndef",   1,    PPCIFNDEF },
	{ "elif",     0,    PPCELIF },
	{ "else",     1,    PPCELSE },
	{ "endif",    1,    PPCENDIF },
	{ "define",   1,    PPCDEFINE },
	{ "redefine", 1,    PPCREDEFINE },
	{ "undef",    1,    PPCUNDEF },
	{ "clear",    1,    PPCCLEAR },
	{ "cex",      0,    PPCCEX },		// development aid: eval/display expr
	{ "say",      0,    PPCSAY },		// devel aid: display rest of line
	{ "echoon",   0,    PPCECHOON },	// enable input echo
	{ "echooff",  0,    PPCECHOOFF },	// disable input echo
	{ 0,          0,    0 }
};

/*--- CONDITIONAL COMPILATION: nested #if info */
struct IFSTK	// each member non-zero if --
{
	SI thisOn;		// on at #if this level (#if 1, or #if 0...else, etc)
	SI allOn;		// compile on considering enclosing levels too
	SI hasBeenOn;	// has been on: additional elif's go off
	SI elSeen;		// #else seen: another else or elif is error
	USI fileIx;		// input file name index ...
	int line;		// and line # of #if, for error messages
};
const int IFDEPTH = 1+20;		// 1 + # nested #if levels allowed
static IFSTK ifStk[ IFDEPTH] = 	// stack of nested #if info
{ { 1, 1, 1, 1 } };   			// top dummy: compile on when no #if
static IFSTK* iff = 0;			// &ifStk[ifDepth-1]: curr ifStk entry
static SI ifDepth = 1;			// # nested #if's at present
// initial 1 for true top dummy
// when there are too many nested #if's, ifDepth keeps counting, iff is unspecified, and ifOn is zero.
static SI ifOn = 1;	// 0 if under a false #if anywhere in nest. also used in ppC()

/*---- re Preprocessor command tokenize */

// set by ppTok(), used in preproc cmd code
static char ppToktx[512+1] = { 0 };	// text of every token, for errMsgs
static USI ppSival = 0;				// value of integer number
static char ppIdtx[512+1] = { 0 };	// text of identifier, set by ppcId()


/*------------------------- FUNCTION DECLARATIONS -------------------------*/

// re getting preprocessed text.
//   see pp.h for: ppOpen(), ppClose(), ppGet().
static RC FC ppOpI( const char *fname, char *defex);

// executing pp cmds
static RC   FC ppcDo( const char *p, int ppCase);
static void FC msgOpenIfs();
static RC   FC ppDelDefine();
static RC   FC addDefine( char *id, char *text, SI nA, char *argId[] );
//  void FC dumpDefines();   	// debug aid	// see pp.h

static RC FC ppCex( SI *pValue, char *tx);

static void FC ppctIni( const char* p, bool isClarg=false);
static SI FC ppTok();
static RC FC ppcId( int erOp);
static SI FC ppCDc();
static void FC ppUncDc();
static SI FC ppCNdc();

static RC CDEC ppErr( char *message, ...);
static RC CDEC ppWarn( char *message, ...);
static RC CDEC ppErn( char *message, ...);

// cleanup fcns called from ppClean()
static void FC ppCmdClean( CLEANCASE cs);
static void FC ppCexClean( CLEANCASE cs);
static void FC ppTokClean( CLEANCASE cs);


/*-------------------------------- DEFINES --------------------------------*/

/*--- re inStk[] file input buffering */

// re buffering and reading in ppRead() and BUFSZ define (next)
const int NREAD = 4096;	// read this many bytes at a time
const int BUFMIN = 512;	// read if < this many unprocessed chars in buf: must be >= max identifier length
const int BUFKEEP =512;	// keep this many already-processed chars in buffer:
						//   3 needed for look-back for \ \r \n in ppM.
						//   more to show file line from buf in errMsg (ppErv)
const int BUFTAIL =		// leave this many bytes after chars read
	       1 +			// for \0 at end buffer
		   1 +			// for EOF \n to separate include files
		   2;			// for */ ppM can add to end comment
// inStk file .buf size, used in ppOpI() and stored in inStk[].bufSz.
const int BUFSZ = BUFKEEP +	// # chars b4 is->i kept in buffer
		 BUFMIN +			// max # chars after is->i kept at read
		 NREAD +			// # chars read at once
		 BUFTAIL;			// space needed after chars read

//------------------- VARIABLES for INPUT LISTING DEPARTMENT -----------------

//----- re INPut listing (echo) virtual report

int VrInp = 0;	// 0 or virtual report handle (vrpak.cpp) for open INPut listing virtual report.

typedef enum LINESTATtag 	// VrInp line status type
{
	midLine=0,			// in mid-line or unknown
	begLine,			// at start of a line
	dashed			// at start of a line and preceding line is known to be ---------------.
} LINESTAT;

LOCAL LINESTAT NEAR lisLs= begLine;	// whether listing report cursor is at midline, start line, or after ----------'s line

//----- re types of input file line in listing

typedef enum LisTyTag 		// type for types of listing lines
{
	LtyNul,  			// not specified / no change
	LtyBody,   			// regular file body line -- no indicator
	LtyIf0,          		// line not compiled as enclosed in #if 0
	LtyPpc  			// preprocessor command
	// also, #included lines are detected and flagged in lisToChar, without the use of any LISTY type.
} LISTY;

LOCAL LISTY NEAR lasTy = LtyBody;	// type of lines being listed from input file: LtyNul, LtyBody, LtyIf0, or LtyPpc.

//----- re input listing (echo) buffer
constexpr int LISBUFWRITE = 512;			// transfer size out of listing buffer
constexpr int LISBUFKEEP = 128 + 128 + 768;	// bytes to keep in listing buffer, for message-placement search-back:
										//  128: allowance for one whole line
										//  128: preprocessor stays a line ahead of compiler (see cutok.cpp:cuC)
										//  256 to 768: additional text to search back over when
										// input contains long lines or long comments or
										// cul.cpp error detection lags pp.cpp preprocessing & listing,
										// plus messages already inserted and bytes listing format adds to lines
constexpr int LISBUFSZ = LISBUFKEEP + LISBUFWRITE;	// size of listing buffer (+1 for null added at alloc)
LOCAL char* lisBuf{ nullptr };			// ptr to listing buffer, in dm
LOCAL int lisBufN{ 0 };   				// # bytes in lisBuf
LOCAL int lisBufEchoOff{ 0 };			// echo off/on control: 0=echoing; >0=#echooff depth (re nesting)

/*------------------------- OTHER LOCAL VARIABLES -------------------------*/

//----- INPUT FILE PATHS
static Path ppPath;			// paths-holding, file-finding object


//----- otherwise-internal-to-fcns variables here to permit cleanup
LOCAL BOO NEAR ppGetEof = FALSE;	// TRUE if eof or fatal error has been seen in ppGet
LOCAL SI NEAR maCNext = 0;		// saved next char in macArgC
//- internal variables for function 'ppC'
LOCAL SI NEAR ppCMidline = 0;	/* 0: next char starts line (look for #);
				   1: # seen, looking for identifier, to test for #if to set .noMX;
				   2: after # and id, or after start other line */
LOCAL SI NEAR ppCppc = 0;	/* 0: current line not preprocessor command;
				   1: line IS preprocessor command (normal value);
				   2 or more: pp cmd, "overlong" msg already issued.*/
LOCAL SI NEAR dStat = 0;	// non-0 when suppressing macro expansion for id in "defined(id)" in a ppc command
LOCAL SI NEAR ppCcase = 0;	// which pp command (PPC__ define), or 0 if not looked up yet or not found.


/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/

// command line switches interface
LOCAL RC FC NEAR ppClDefine();
LOCAL RC FC NEAR ppClUndefine();	// 2-95

// getting preprocessed text
LOCAL void FC NEAR ppClI( void);
LOCAL RC   FC NEAR ppC( char *p, USI n, USI *nRet);
LOCAL RC   FC NEAR ppM( void);
LOCAL RC   FC NEAR ppmId( SI *pMFlag);
LOCAL RC   FC NEAR macArgs( DEFINE *theDef, char *** pArgV);
LOCAL RC   FC NEAR macArg( DEFINE *theDef, char *arg, SI *pC);
LOCAL void FC NEAR macArgUnc( SI c);
LOCAL RC   FC NEAR macArgC( SI *pC);
LOCAL RC   FC NEAR macArgCi( SI *pC);
LOCAL RC   FC NEAR ppmScan( void);
LOCAL RC   FC NEAR macOpen( char *id, char *text, DEFINE *theDef, char **argV, enum ISTY ty);
LOCAL RC   FC NEAR ppRead( void);

// input listing
LOCAL bool lisBufEchoOnOff(bool turnOn, const char* pSrcTxt);
LOCAL void FC NEAR lisToChar( INSTK NEAR * _is, int echTo, LISTY newTy, int flush=0);
LOCAL void FC NEAR lisBufAppend( const char *p, int n=-1);
LOCAL void FC NEAR lisBufOut( int n);
LOCAL void FC NEAR lisWrite( char *p, int n);
LOCAL int  FC NEAR lisCmp( const char* s, int i, int* pPlace, int* pnl);
LOCAL int  FC NEAR isFileLisLine( char *p);
LOCAL void FC NEAR lisBufInsert( int* pPlace, char *p, int n=-1);


/*=================== COMMAND LINE INTERFACE department ===================*/
/* interfaces to process preprocessor command line arguments:
     -Dsym		define symbol (as null text)
     -Dsym=value	define symbol
     others may be added
   note does NOT pick out input file name -- caller must pass that to ppOpen.*/

//==========================================================================
SI FC ppClargIf(

// if text is a preprocessor command line argument, execute it and return nz

	const char* s,	// one element of c's argv[] (if we are breaking up
					// command line in program, change to stop at (nonquoted?)
					// blanks and perhaps to decomment.)
	RC* prc ) 		// NULL or receives RCOK or errCode (msg issued) from executing pp argument
					//   if a pp arg.

// if NOT a pp switch, fcn returns 0 (false) and *prc is unchanged.
{
	SI retVal = 0;
	RC rcSink;

	ppctIni( s, true);		// init to scan text, and make errMsgs say "in command line arg" not "at line n".
	SI c = ppCNdc();		// get char, not decommented
	if (c=='-' || c=='/') 	// switches begin with - or / -- else not a command line arg for us, go return 0
	{
		c = ppCNdc();		// get char after - or /
		if (prc==NULL)
			prc = &rcSink;
		switch (tolower(c))
		{
		case 'd':   			// define
			*prc = ppClDefine();   	// do -D command (local, next)
			retVal++;				// say WAS a preprocessor command
			break;

		case 'u':    			// undefine, 2-95
			*prc = ppClUndefine();	// do -U command (local, below)
			retVal++;				// say was a preprocessor command
			break;

		case 'i':   			/* -Ipath;path;path... sets directory search paths for input and #include files.
								May be given more than once; paths searched in order given. */
			ppAddPath(s+2);		// function below in pp.cpp. arg: all of s after "-i".
			*prc = RCOK;   		// say no error
			retVal++;			// say a pp cmd line switch
			break;

		default:
			break;   		// not for us. fall thru to return 0.
		}
	}
	ppctIni( nullptr);		// terminate cmd line arg scan
	return retVal;			// 0 or 1
}			// ppClargIf

//==========================================================================
LOCAL RC FC NEAR ppClDefine()

// do preprocessor "define" cmd line cmd.  Call ppctIni and pass -D first.

/* uses entire input, assumed to be argv[i].
   If we are breaking up command line locally, change to stop at (nonquoted?) blanks and perhaps to decomment. */

// fcn value: RCOK if ok, else error message has been issued.
{
	char *id, *p, val[255+2+1];
	SI c;
	RC rc /*=RCOK*/;

// parse identifier
	CSE_E( ppcId(WRN) )		// get identifier to ppIdtx or issue errMsg; decomments.
	id = strsave(ppIdtx);	// save in dm for symbol table

// parse optional "=value".
	// No blanks accepted around '=': blanks SEPARATE cmd line args.
	c = ppCDc();		// next char (Decomment because ppcId decomments then ungets)
	p = val;
	if (c=='=')			// if "=" after identifier
	{
		*p++ = ' ';		// lead & trail spaces to prevent accidental token concat. Coord changes w/ doDefine().
		for ( ; ; )      	// scan rest of arg as value
		{
			c = ppCNdc();
			if (c==EOF)
				break;
			*p++ = (char)c;
		}
		*p++ = ' ';
	}
	else if (c==EOF)	// nothing after identifier is ok
		;				// or strcpy( val, "1") ? check msc.
	else				// garbage after id: bad syntax
		rc = ppErr( (char *)MH_P0025);	// issue error message "'=' expected" and define and continue
	*p = '\0';

// enter in symbol table as 0-argument macro (or error if a redefinition).
	return rc | addDefine( id, strsave(val), 0, NULL);
}		// ppClDefine

//==========================================================================
LOCAL RC FC NEAR ppClUndefine()

// do preprocessor "undefine" cmd line cmd.  Call ppctIni and pass -U first.  Added 2-95.

// fcn value: RCOK if ok, else error message has been issued.
{
	RC rc /*=RCOK*/;		// (redundant init removed 12-94 when BCC 32 4.5 warned)

// parse identifier
	CSE_E( ppcId(WRN) )		// get identifier to ppIdtx or issue errMsg, decomments.

// check for end of input
	SI c;
	do
		c = ppCDc();		// get char, decommented. Decomment may be necessary cuz ppcId decomments then ungets.
	while (isspaceW(c));	// pass whitespace, expect end
	if (c != EOF)
		return ppErr((char *)MH_P0026);		// "Unexpected stuff after -U<identifier>"

// remove definition if any
	ppDelDefine();		// uses ppIdTx. ignore return: not defined is not an error.

	return rc;
}		// ppClUndefine


/*================== GETTING PREPROCESSED TEXT department =================*/


//==========================================================================
void FC ppClean( 		// preprocessor overall init/cleanup routine

	CLEANCASE cs )	// [STARTUP,] ENTRY, DONE, CRASH [or CLOSEDOWN]. type in cnglob.h.

{
	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */

// pp.cpp getting input text variables
	if (cs > ENTRY)		// preserve paths at ENTRY call in case paths already set from command line
		ppPath.clean();		// paths object: free any memory & init (like destructor)
	else if (cs != ENTRY)	// insurance -- believe redundant even for unimplemented STARTUP call
		ppPath.init();		// initialize paths object (like constructor) without freeing any heap pointer
	if (cs > ENTRY)     	/* do files at normal or fatal error exit.
				   Tentatively do not close files at entry or startup: should have been closed at exit;
       				   at startup assume just garbage in memory (at risk of leaving unfree'd file handles) */
		if (is && inDepth > 0)	// if input file(s) apparently open
			ppClose();		// close input file(s): main and any include files, and any macro calls & their storage.
	//inDepth = 0;		initialize input file/macro nesting depth. ppOpen does this.
	//ppGetEof = FALSE;   	say input eof/error not seen this session. ppOpen now (10-93) does this.
	//isf = is = NULL; 		init ptrs to open file/macro info: beleive not needed: not used when inDepth is 0.

	ppCMidline = ppCppc = dStat = ppCcase = 0;	// ppC fcn: at start line, not doing ppc command, not doing 'defined()'

	maCNext = 0;				// no saved next char for macArgC

// pp.cpp input listing dept variables
	closeInpVr();		// if VrInp, close input vr (currently a NOP 10-93) and zero VrInp
	if (lisBuf)				// if input listing buffer is allocated
	{
		//no, abandon listing buffer contents if not written in normal course of exit, in case vrpak closed first, etc.
		// if (cs > ENTRY)		// ignore any buffer contents at entry
		//    lisBufOut(0xffff);		// output any remaining listing text at normal or fatal error exit
		dmfree( DMPP( lisBuf));  	// deallocate the buffer
		//lisBuf = NULL;			dmfree NULLs the pointer (reference arg)
	}
	//lisLs = begLine;		say input listing is at beginning of line: redundant: opInpVr does it.
	//lisBufN = 0;  		done when buffer allocated
	lasTy = LtyBody;		// say in a body line in input listing. Also done when buffer allocated -- is that soon enuf?

	ppCmdClean(cs);
	ppCexClean(cs);
	ppTokClean(cs);
}			// ppClean
//==========================================================================
void ppAddPath( const char* paths)	// add ;-separated path(s) to be searched for input and include files

// if caller wants DOS PATH to be searched, he should do:  ppAddPath(NULL); .
{
	ppPath.add(paths);
}			// ppAddPath
//==========================================================================
BOO ppFindFile( 		// find file using paths specified with ppAddPaths. Issues no message.
	const char* fname,
	char *fullPath )	// receives full path if RCOK is returned. [_MAX_PATH]
// returns TRUE iff found
{
	return ppPath.find( fname, fullPath);
}						// ppFindFile
//==========================================================================
BOO ppFindFile( 	// find file using paths specified with ppAddPaths. Issues no message.
	char* &fname)	//  file to find
					//    returned updated to full path iff found
// returns TRUE iff found
{
	char fullPath[ _MAX_PATH];
	BOO bFound = ppPath.find( fname, fullPath);
	if (bFound && strcmpi( fname, fullPath))		// if found path different (else don't save for less fragmentation)
	{	cupfree( DMPP( fname));		// if not a pointer to "text" embedded in pseudocode, dmfree name
		fname = strsave( fullPath);		// replace name with full pathname
	}
	return bFound;
}						// ppFindFile
//==========================================================================
RC FC ppOpen( const char* fname, char *defex) 	// open and init cal non-res user language main input source file

// searches current directory, and drives/directories per preceding ppAddPath calls.

{
	RC rc;

// if file already open, message and continue: insurance
	if (is && inDepth)
	{
		err( PWRN, (char *)MH_P0001);	// internal error msg "ppOpen(): cn ul input file already open" with keypress, rmkerr.cpp
		ppClose();						// conditionally close
	}

// initialization
	ppGetEof = FALSE;	// eof not yet read. Necessary for rerun.
	inDepth = 0;	// file/macro input nesting depth: none yet: insurance

// open input file using internal function
	CSE_E( ppOpI( fname, defex) )		// next.  E returns if error; inits rc.

	return rc;				// another return above (E macro)
}			// ppOpen
//==========================================================================
RC FC ppOpI( const char* fname, char *defex)		// inner pp file opener: adds an input source stack level

{
	INSTK NEAR * isi;
	RC rc;

// standardize name (upper case) and conditionally use default extension
	fname = strffix( fname, defex);		// to tmpstr

// find file using paths given to ppAddPath
	char buf[_MAX_PATH];			// receives full pathname
	if (ppFindFile( fname, buf))	// search paths (above) / if found
		fname = buf;				// replace caller's arg with full pathname found

// check for excessive nesting
	if (inDepth >= INDEPTH)
		return ppErr( (char *)MH_P0002, fname ); 	// "Cannot open '%s': already at maximum \n    include nesting depth"

// initialize next nested input file level
	isi = inStk + inDepth;		// locally point next inStk entry (inDepth not yet incremented)
	memset( isi, 0, sizeof(INSTK) );	// init many members
	isi->ty = tyFile;			// say a file (not a macro)
	isi->fileIx = getFileIx(fname);	// save name, get index (ancrec.cpp)
	isi->mName = NULL;			// insurance
	isi->line = 1;				// init file line # for errmsgs
	isi->echLine = 1;			// init line # in input echo listing

// open file
	isi->file = fopen( fname, "r");		// open to read (c library)
	if (isi->file==NULL)			// if open failed
		return ppErr( (char *)MH_P0003, fname);	// format & display msg "Cannot open file '%s'" (local), return bad

// allocate file buffer
	CSE_E( dmal( DMPP( isi->buf), BUFSZ, WRN|DMZERO) )	// allocate dm, dmpak.cpp
	isi->bufSz = BUFSZ;					// size alloc'd (define at start file)

// flush listing (input echo) buffer so text from multiple files cannot be intermixed in it
	lisBufOut(0xffff);		// note done AFTER possible open error

// ok, now set file-global ptr and bump input nest level
	inDepth++;			// input nest level
	is = isi;			// file-global inStk pointer
	isf = isi;			// ditto to file: 'is' changes for macro
	return RCOK;		// good return
	// more returns above
}			// ppOpI
//==========================================================================
void FC ppClose()		// close all open CSE user language input files if any

{
	while (inDepth > 0)		// until all #includes and source closed
		ppClI();			// close last opened file (or macro)

	if (lisBuf)				// if input listing buffer is allocated
	{
		lisBufOut(0xffff);		// output any remaining listing text
		dmfree( DMPP( lisBuf));  	// deallocate the buffer
	}
}			// ppClose
//==========================================================================
LOCAL void FC NEAR ppClI()		// inner pp input closer: closes last file/macro/arg level open

{
	INSTK NEAR * isi;

	if (inDepth > 0)		// if any possible file/macro to close
	{
		isi = is;		// local copy of pointer to entry to close
		inDepth--;		// decr global level
		is--;			// decr global ptr to macro or file level
		if (isi->ty==tyFile)
		{
			while ((--isf)->ty != tyFile  &&  isf > inStk)	// decr global ptr to current input file:
				;							// ... pass any macros, find previous file
			if (isf >= inStk)				// unless closing top file
				// above 'if' added 12-91; are there other places where isf accessed even tho value is inStk-1 ??
				isf->lineSent = 0; 			// tell ppC to send #line for file so caller cuTok will know text source
			if (isi->file)				// if file open
			{
				lisToChar( isi, isi->n, LtyNul, 1);  	/* list any remaining text in file's buffer, and flush the list
	     						   buffer to prevent mixing list lines from multiple files. */
				if (fclose(isi->file) != 0)    		// close FILE (c lib)
					ppErr( (char *)MH_P0004, 		// error message "Close error, file '%s'" if failed
						   getFileName(isi->fileIx) );
				isi->file = NULL;				// say no file open
			}
			dmfree( DMPP( isi->buf));    		// free buffer, if any
		}
		else if (isi->ty==tyMacro)
		{
			// close macro or macro argument
			if (isi->argV)   			// if have nonNULL ptr to block of arg ptrs
			{
				char **vp;
				for (vp = isi->argV;  *vp;  vp++) 	// free each value (NULL ends)
					dmfree( DMPP( *vp));		// releases memory *vp
				dmfree( DMPP( isi->argV));  	// free block of pointers
			}
			//these expected to free the blocks only if symbol table cleared during the macro expansion...
			dmfree( DMPP( isi->theDef));  	// decrement reference count to macro sym tab entry; NULL for arg. 11-95.
#if 0
x			cupfree( DMPP( isi->buf));	// did not fix crashes
#else
			dmfree( DMPP( isi->buf));		// decr ref count to mac text (theDef->text) or arg val (another argV[i]). 11-95.
#endif
			// do not dmfree .mName: not heap block.
		}
		else
			err( PWRN, (char *)MH_P0005);   	// display internal error msg "ppClI: bad is->ty", wait for key, rmkerr.cpp
	}
}		// ppClI
//==========================================================================
SI FC openInpVr()		// open input listing virtual report and set VrInp if none open; return handle.

// call if input listing report is desired, or to retreive VrInp even if already open.

/* unspooling: cncult.cpp makes up UnspoolInfo for cse.cpp to use after each run;
               crsim.cpp does end-of-session unspool */
{
	if (!VrInp)
	{
		vrOpen( &VrInp, "Input Listing", VR_FMT | VR_FINEWL);	/* open virtual report, vrpak.cpp.
	       							   name given is used in error messages.
       								   VR_FMT: use page formatting (can be turned off at unspool).
       								   VR_FINEWL: add blank line to end of report */
		lisLs = begLine;					// say the new report is at the start of a line
		// note: report title is written when first text is entered (in this file).
	}
	return VrInp;			// return handle even if already open.  caller cncult.cpp assumes this.
}			// openInpVr
//==========================================================================
void FC closeInpVr()  	// terminate and close input listing virtual report
{
	if (VrInp)			// if open
	{
		/* note: final newline is written to report in cse.cpp after calling cul(),
		   rather than here, to be near rest of listing in spool file, for unspooling speed. */

		VrInp = 0;		/* forget the handle to prevent further output (during unspooling)
       				   and cause new virtual report to be opened for next run.
       				   close call to vrpak.cpp is not necessary. */
	}
}		// closeInpVr
//==========================================================================
USI FC ppGet( char *p, USI n)			// public entry to get 1 to n characters of preprocessed text to buffer p

// fcn value 0 at EOF or fatal error, or number of characters returned.
{
	if (ppGetEof)		// if fatal error or eof previously seen this session
		return 0;		// return it again
	if (inDepth <= 0)		// if no file open, 2-91:
		return 0;		/* return like EOF; don't set rc in case file yet to be opened.
				   eg call from error handler, eg perNx().
	       			   (ppC, ppM etc don't check inDepth at entry & may be unpredictable with invalid "is".) */
	USI nRet;
	RC rc = ppC( p, n, &nRet);	// normally get text, just below. Only call.
	if (rc)						// if eof or fatal eror
		ppGetEof = TRUE;  		// set flag tested at entry, for later calls
	if (rc==RCEOF)				// if eof (first time) & no preceding errors
		msgOpenIfs();			// msg any unclosed #if's
	if (rc==RCFATAL)			// if serious error such as out of memory
		return 0;				// say so
	return nRet;				// # chars gotten, 0 if end file; other 0 returns above
}			// ppGet

/*=================== ALL FOLLOWING FUNCTIONS up to input listing section ARE LOCAL ===================*/

//==========================================================================
LOCAL RC FC NEAR ppC( char *p, USI n, USI *nRet)

// inner fcn to scan 1 to n input characters into caller's buffer at p

// expands macros (using ppM), executes preprocessor commands (using ppCDo).

/* usually 11-91 returns characters thru first newline: enough so whole preprocessed line
     is in caller's buffer, for inclusion in errmsgs;
   and not more: to facilitate embedding error messages in listing. */

/* why preprocessing ahead of compile is bad for error messages:
   1. listing is generated here from preprocessor input text.
   2. Messages with file line text: place is found when possible by search-back,
      but we do not search back over start or end of include file.
   3. Messages not including file line, or not found in search-back, are appended
      to current end of listing; preprocess-ahead moves them down inappropriately. */

/* returns: RCOK    ok
            RCEOF   end of input file (from ppRead via ppM)
            RCFATAL error that should terminate session, such as out of memory for (preprocessor) symbol table 1-8-92
            other   other error */
{
//internal variables, above at file level to permit cleanup (10-93):
// SI ppCMidline = 0;	/* 0: next char starts line (look for #);
//			   1: # seen, looking for identifier, to test for #if to set .noMX;
//			   2: after # and id, or after start other line */
// SI ppCppc = 0;	/* 0: current line not preprocessor command;
//			   1: line IS preprocessor command (normal value);
//			   2 or more: pp cmd, "overlong" msg already issued.*/
// SI dStat = 0;	// non-0 when suppressing macro expansion for id in "defined(id)" in a ppc command
// SI ppCcase = 0;	// which pp command (PPC__ define), or 0 if not looked up yet or not found.

	RC rc;

const int PPCMAX = 16383;	// max preprocessor command length

	char ppcBuf[ PPCMAX+1], *ppcP = nullptr;

	*nRet = 0;				// say no chars returned yet
	SI newlineRet = 0;			// say no \n ret'd yet. added 11-91 re reducing preprocess-ahead re embedded errmsgs.

	while (!newlineRet  &&  n > 0)	// until newline returned or caller's count satisfied
	{
		/* scan some input and expand macros; fast nop if already have scanned text.
		   Handles ends of included files; macro expansion starts and ends;
		   determines if text in quotes or comment or identifier. */

		CSE_E( ppM())  	/* sets is->j to last+1 useable char;
       			   sets is->inCmt and is->isId.  Always returns identifier separately; code here assumes this.
			   if fcn value non-RCOK (top level EOF (RCEOF) or error), E() returns it to our caller. */
		/* >>> if need found, defer non-RCOK return till after #line sent so
		   caller gets final line number if file ended in false #if. 9-90. */

		// account newlines ppM skipped over (eg in expanded mac ref arg list)
		// note msc 6.0 does by adding #line cmd, ?after next newline.
		while (is->nlsNotRet > 0)
		{
			// note expect is==isf if here
			if (isf->lineSent)			// unless about to send #line command
				// && ifOn				// if ifOn is 0, lineSent is 0
			{
				// put newline in caller's buffer for caller's line count
				if (n <= 0)			// if caller's buffer full
					return RCOK;			// return, finish next call
				*p++ = '\n';
				n--;
				(*nRet)++;	// return newline to caller
			}
			is->line++;				// our line counter
			is->nlsNotRet--;
		}

		// send #line command if necessary: tells caller file name and line
		// at start of new #include or after end of an included file

		if ( isf->lineSent==0		// if #line has not been sent
		  && ifOn != 0)			// and compilation is on (#if's)
		{
			const char* s =
				strtprintf( "\n#line %d \"%s\"\n", isf->line, getFileName( isf->fileIx) );
			int m = strlen( s);
			if (m > n)			// if not room in user's buffer to return it
				return RCOK;		// return now, #line will be redone
			// NB must be sure caller's buffer big enuf to not lock here
			isf->lineSent++;
			memcpyPass( (void **)&p, s, m);
			n -= m;
			*nRet += m;	   	// memcpyPass: memcpy, point past. strpak.cpp.
		}

		// at start line, set 'ppCppc' and 'ppCcase' if this line is a # command,
		// and turn off macro expansion for body of most # commands

		char *q;
		char *pi = is->buf + is->i;	// start of available text
		char *endi = is->buf + is->j;  	// end+1 of available text
		if (is->inCmt== -1)		// if chars ppM() returned are quoted
			ppCMidline = 2;   		// not at beg line: do not look for #.
		if (ppCMidline==0   	// if last char was \n
		 && is->inCmt==0)		// and text not in comment nor quotes
		{
			// then look for #
			q = pi;
			while (strchr(" \t", *q) && *q)	// deblank
				q++;
			if (q < endi)    	// if any nonblanks
			{
				if (*q=='#')   	// if 1st noblank is #
				{
					ppCppc = 1;   	// line is preproc cmd
					dStat = 0;		// no "defined(x)" yet in this cmd
					ppCcase = 0;	// which pp command not yet determined
					ppcP = ppcBuf;	// init buffer ptr for storing command
					pi = q;   		// pass the whitespace: deblank now
					is->noMX = 1;	// no macro expansion (ppmId) in word after # and thruout most most # commands
					ppCMidline = 1;	// say test next id for #if/#elif
				}
				else
				{
					ppCMidline = 2;	// don't look for # again til after \n
					is->noMX = 0;	// macro expansion on (redundant)
				}
			}
			/* if whitespace extended to end scanned text, leave ppCMidline=0 to look for # on next call.
			   Probable bug: this much whitespace may be returned to caller even if # found in later iteration.
			   Should only occur whitespace preceded ref to macro containing #.  Are #commands in macros supported? */
		}
		else if (ppCMidline==1)	// if # just seen
		{
			// note syntax verify & error msgs done in ppcDo().
			lisToChar( is, is->j, LtyPpc);		// echo input thru end of id after #, say is a preprocessor line
			if (is->isId==1)	// if this text is identifier (implies !inCmt)
			{
				PPCTAB * tp;

				// determine which preprocessor command, and re-enable macro
				// expansion for those commands with constant expressions next

				char csave = *endi;
				*endi = '\0';
				for (tp = ppcTab; tp->word != 0; tp++)	// search table for word
					if (strcmpi(tp->word, pi)==0)    	// compare word to this entry. Made case-insensitive 11-94.
					{	// if found
						ppCcase = tp->cs;			// case to pass to ppcDo
						is->noMX = tp->noMX;		// enable macro expansion per table
						break;
					}
				*endi = csave;
				ppCMidline = 2;			// say don't look for if or elif or # until after next \n
			}
		}

		// find end of line (include \n) or end of available input

		q = (char *)memchr( pi, '\n', (USI)(endi - pi));	// ptr TO \n, or NULL
		USI ni;
		if (q==NULL)
			ni = (USI)(endi - pi);		// # chars: all
		else
			ni = (USI)(q - pi) + 1;		// include newline in count

		/* return non-ppc text to caller.  Include comment that starts in pp cmd
		   and continues into next line -- else continuation would error. */

		if (ppCppc==0)			// if not preprocessor command
		{
			if (ifOn)			// if no #if or true #if's
			{
				/* >>> This is where to strip comments if desired:
				   if is->inCmt > 0, do not transmit text, only the \n's in it. */

				if (ni > n)						// truncate xfer len to caller's buf
					ni = n;							// .. space, leaving rest in input
				memcpyPass( (void **)&p, pi, ni);
				n -= ni;
				*nRet += ni;	// transfer to caller.  strpak.cpp fcn.
			}
			//>>> else be sure lineSent is 0 (clear at false #if)		<-- how old is this note?

			lisToChar( is, is->i + ni, ifOn ? LtyBody : LtyIf0);
										// echo this non-ppc text to input listing (if not in
          								// macro), as regular lines or if'd-out lines.
		} // nb else follows

		// buffer preprocessor command text for decoding at end line

		else						// ppCppc != 0
		{
			static SI noMXsave = 0;

			int m = (ppcBuf + sizeof( ppcBuf) - 1) - ppcP;	// remaining bufSpace
			if (m >= ni)
				m = ni;					// all will fit
			else						// m < ni: command too long
			{
				if (ppCppc++ ==1)				// exactly 1 1st time: one msg only
					ppErn( (char *)MH_P0006 );	// errMsg "Overlong preprocessor command truncated" with no text echo
				// put only m chars in ppcBuf, but remove all from input.
			}
			memcpyPass( (void **)&ppcP, pi, m);
			*ppcP = '\0';

			// logic to disable macro expansion in identifier in 'defined(x)'

			switch (dStat)	/* ug! perhaps macro expansion should have been deferred to where pp cmd is parsed.
	  			   Or maybe ppmId should replace defined(id) with 0 or 1. */
			{
			case 0:   // not scanning 'defined(id)': normal case
				if ( is->noMX==0			// else needn't bother: safer
				 && is->isId 				// for speed
				 && strcmpi( ppcP - m, "defined")==0 )	// (made case-insensitive 11-94)
					dStat = 1;				// say 'defined' seen
				break;

			case 1:   // 1st iteration after 'defined'; presumably have "(".
				noMXsave = is->noMX;		// save
				is->noMX = 1;		/* disable macro expansion: let next thing (presumably id) pass thru to
		      					   ceval even if defined */
				dStat++;			// say restore .noMX after next ppM
				break;

			case 2:   // 2nd... presumably now have the id in parens.
				is->noMX = noMXsave;		// restore
				// is there a possible problem of having changed levels (ended macro?) could noMX be different?
				/*lint -e716 */
			default:  // insurance
				dStat = 0;		// return to normal
				break;
			}  // switch (dStat)
		}   // if (ppCppc==0)...else...

		// remove transferred text from input buffer

		pi += ni;				// point after chars used
		USI i = (USI)(pi - is->buf);		// convert back to subscript for is->i
		if (i > is->n)
			ppErr( (char *)MH_P0007);		// " Internal error: ppC: read off the end of buffer "
		if (*(pi-1)!='\n')  			// if newline not transferred
			is->i = i;				// store new is->i: pass text used

		// end of line stuff

		else			// newline transferred
		{
			if (ppCppc)  		// if doing #command
			{
				// test for \ before \n in #command: not end command
				if ( ( *(ppcP-2)=='\\'				// \ \n
				|| ( *(ppcP-2)=='\r' && (*ppcP-3)=='\\' )	// \ \r \n
					 )
				// && *(ppcP-1)=='\n' TRUE if here unless truncated
				// tolerate failure if long #cmd truncated (msg issued above)
				   )
				{
					/* \ and \n in #command, per tests of msc 6.0 9-90:
					   DELETE from # command, allowing tokens to concatenate, and continue scanning # command. */
					ppcP -= (*(pi-2)=='\r') ? 3 : 2;	// from ppcBuf DELETE
					*ppcP = '\0';				// \ \r \n  or  \ \n
					is->i = i;			// pass input text
					is->line++;			// our line count
					if (ifOn)			// if compiling...
					{
						*p++ = '\n';
						n--;
						(*nRet)++;	// ret \n for caller lineCount
					}
					// ppCppc, ppCMidline, .noMX: no change: #command continues
				}
				else		// \n terminates #command
				{
					lisToChar( is, i, LtyPpc);   	// list rest of processor command including final \n
					/* pass input text TO the \n, and if #cmd ended in / * comment,
					   put comment start in buffer for caller to see after cmd done
					   so continuing cmt won't error.
					   (Original comment start was put in ppcBuf and may be gone
					   from is->buf eg in obscure  \ \n ... \n  cases).
					   (May need another method that does not alter input buf if
					   turns out this line can still be shown in an error message). */

					i--;			// leave \n in buf for after #cmd done
					if (is->inCmt==2)	// if in / * comment
					{
						if (i >= 3)   		// insurance
						{
							is->buf[--i] = '*';	// put / and * before the \n
							is->buf[--i] = '/';
							is->buf[--i] = ' ';	// spc b4 / * so can't paste
						}
						else
							ppErr( (char *)MH_P0008);		// "ppC internal err: buf space not avail for / and * "
						// if can occur, fix, eg ?? reserve 3 bytes b4 buf.
					}
					is->i = i;	// store new is->i (buf subscr of nxt char)

					// do preprocessor command

					rc = ppcDo( ppcBuf, ppCcase); 	/* decode/execute preproc cmd. Only call.
			            		   Can open new input file, changing is/isf.
	        		     		   \n left in input is then seen after eof. */
					if (rc==RCFATAL)		// if out of memory for symbol table 1-8-92, end session now.
						return rc;			// after other errors (bad syntax, etc), continue here.
					ppCppc = 0;		// now not doing # cmd
					dStat = 0;		// insurance
					ppCcase = 0;		// insurance
					is->noMX = 0;		// enable macro expansion
					/* \n or comment including \n will be rescanned:
					  1) to ++ our line # AFTER command so correct during command;
					  2) to transmit \n to caller for his line count;
					      3) so no line concat after incl file (redundant: see ppRead) */
				}
			}
			else			// not a #command line
			{
				ppCMidline = 0;	// say start of line: look for #
				is->line++;	// count file lines (shd be no \n's in macros)
				// NB don't line++ b4 ppcDo(): line # in errmsgs wd be wrong.
				is->i = i;  	// pass input text used: store new is->i
				if (ifOn)		// if this text was returned
					newlineRet++;	// say a newline has been returned: caller has whole line, as needed for echo in error message.
			}
		}    // if .. else newline transferred
	}	// while [!newlineRet  &&]  n > 0
	return RCOK;			// more returns above
}			// ppC
//==========================================================================
LOCAL RC FC NEAR ppM()		// Macro-scan 1 or more input chars from current input

/* On return, characters is->buf[is->i..j-1] may be used by caller.
   Caller must unbuffer used characters by advancing i.
   is->inCmt and is->isId are set for these chars (see INSTK definition);
     thus a comment, identifier, or "text" is always returned separately. */

/* Reads file as nec.  At end of inputs, proceeds from ref'd macros to ref'ing
   macro or file, and from included file to including file. */

// returns non-RCOK if end of source input file (RCEOF), or fatal error (messaged)
{
	RC rc;
	SI reScan;

	for ( ; ; )		// loop to scan nested macros ...
	{
		// see "continue" after ppmId() call

		// if have some macro-scanned text, nothing (more) to do
		if (is->j > is->i)	// eg if caller did not use all text from last
			return RCOK;		// .. call at this level. NB .inCmt unchanged

		// read input if buffer emptyish; at end inc file/macro, go to previous
		CSE_E( ppRead() )		/* read if < 128 chars; close/pop if nec. Only call.
				   ret err or top-level EOF (RCEOF) to caller. */

		// scan text, advancing is->j past chars with same is->inCmt and ->isId
		CSE_E( ppmScan() )		// local fcn, below, only call; ret error now.
		if (is->isId)		// if text is identifier
		{
			CSE_E( ppmId( &reScan) )	// process macro arg, macro name, $eof, etc. Only call.  Return error now.
			// CAUTION: ppmId can move is->buf[], invalidating any pointers
			if (reScan)		// if needs rescanning for nested macros
				continue;		// loop to do so
		}
		return RCOK;
	}		// for ( ; ; )
	/*NOTREACHED */
}			// ppM
//==========================================================================
LOCAL RC FC NEAR ppmId( SI *pRescan)

// check identifier for ppM: do macro args, macro refs, $eof

// at entry, is->buf[is->i..is->j-1] is identifier.
/* on return:  if macro ref or arg: set to process it (id passed,
				    inStk[] level added, flag *pRescan set);
	       $eof:   has been processed (file chopped);
	       else:   no change. */
// CAUTION: can move is->buf[] contents (at macro ref), invalidating pointers
// is recursively called whenever macro arg list contains macro reference
{
	RC rc;
	*pRescan = 0;			// tentatively tell caller no rescan needed
	char *p = is->buf + is->i;		// point start of identifier
	char *pend = is->buf + is->j; 	// loc of char after identifier
	char csave = *pend;			// save char after identifier

// test for macro argument
	if (is->theDef)				// if has DEFINE pointer, we are expanding a macro
		// (is->ty still same for macro and arg)
	{
		DEFINE* theDef1 = (DEFINE *)is->theDef;	// fetch define ptr
		if ( theDef1->nA				// if this macro takes arguments
		 && is->argV )					// if have values: insurance
		{
			*pend = '\0';					// terminate id for lu
			for (int i = 0;  i < theDef1->nA;  i++)   	// search DEFINE's arg names
			{
				if (strcmpi( p, theDef1->argId[i])==0) 	// if matches. Tentatively case-insensitive 11-94, also cuparse
				{
					// initiate substitution of macro argument
					*pend = csave;				// restore next char
					is->i = is->j;   			// pass arg name at this level
					char* thisArgV = is->argV[i];		// ith argument in reference
					CSE_E( macOpen( 				/* set up expansion (below) */
						theDef1->argId[i],		/* argument name. used? */
						thisArgV ? thisArgV : "",	/* argument value... if none, use "" (too few args insurance) */
						NULL, NULL,			/* no DEFINE, no argV for arg */
						tyMacro) )			// or different type >>>???
					dmIncRef( DMPP( thisArgV));	// after success, inc value's ref count: insurance 11-95. ppClI decRef's (dmfrees).
					*pRescan = 1;   	// tell caller to iterate to scan macro arg text for macro references
					return RCOK;
				}
			}				// falls thru if not found
			*pend = csave;		// restore
		}
	}

// test for macro reference: look id up in #define symbol table

	if (is->noMX==0)		// if macro expansion not disabled (pp cmd)
	{
		*pend = '\0';						// terminate id for lu
		DEFINE * theDef2;
		rc = syLu( &defSytb, p, 0, NULL, (void **)&theDef2);	// look up, sytb.cpp
		*pend = csave;						// restore
		if (rc==RCOK)			// if found
		{
			is->i = is->j;   		// pass macro name at this level

			// scan args if any
			char **argV;
			if (theDef2->nA==0)		// if macro has no args
				argV = NULL;		// no arg values
			else
			{
				CSE_E( macArgs( theDef2, &argV) )	// only call. just below.
				// CAUTION: macArgs can move is->buf[], invalidating any pointers
				// CAUTION: can get back here if nested macro reference
			}

			// initiate expansion
			CSE_E( macOpen( theDef2->id, theDef2->text, theDef2, argV, tyMacro) )	// set up expansion (below)

			//after success, increment ref counts of heap blocks, to make #clear during a macro safe, 11-95.
			dmIncRef( DMPP( theDef2));		// inc ref count of definition (dmpak.cpp). ppClI decRefs (with dmfree).
			dmIncRef( DMPP( theDef2->text));	// inc ref count of definition text similarly.
			*pRescan = 1;    			// tell caller to iterate to scan macro body text for nested macro references
			return RCOK;
		}
	}

	/* test for end file keyword.  If found, chop file so any remaining text
	   not preprocessed (so no spurious error messages, etc).
	   "$eof" itself is returned in main file only, not #include [or macro] */

	if ( is->j - is->i == 4 /*strlen("$eof")*/
	&& memicmp( p, "$eof", 4 /*strlen("$eof")*/ )==0)
	{
		if (inDepth > 1)			// if in include file (or macro)
		{
			pend = p;			// point START of $eof to delete it
			is->j = is->i + 1;		// only \n may be processed by caller
		}
		else				// not in include file nor macro
		{
			// callers must see main file $eof
			// pend = is->buf + is->j;  done above: point AFTER $eof
			if (ifOn==0)			// if conditional compilation is OFF
				ifOn = 1;			// force on so $eof returned
		}
		*pend++ = '\n';				// supply final \n, as ppRead does
		*pend = '\0';				// truncate text of this file in ram
		is->n = (USI)(pend - is->buf);		// # chars now in buffer
		// if file line can be shown from buffer in errMsgs, may want to rework to not alter buffer contents.
		is->eofRead = 1;  			// suppresses further reads from file
		// *pRescan = 0;				// tell ppM no reScan needed
		return RCOK;				// good return after doing $eof
	}

// else none of the above, leave text for caller to process
	// *pRescan = 0;			// tell ppM no reScan needed
	return RCOK;			// good return
	// more returns above
}				// ppmId
//==========================================================================
LOCAL RC FC NEAR macArgs(
	DEFINE* theDef,
	char *** pArgV)

// parse argument list in macro reference, for ppmId.
// call only for macros taking non-0 number of args

// CAUTION: can move is->buf[] contents, invalidating caller's pointers
// will be recursively entered if arg list contains macro reference
{
#define ARGMAX 128	// max length of argument in macro reference, used in macArgs and macArg
	RC rc;

// allocate array for pointers to argument values

	// note one extra pointer is allocated and left NULL
	CSE_E( dmal( (DMP *)pArgV, sizeof( char **) * (theDef->nA+1), WRN|DMZERO ) )

// deblank between macro name and argument list

	// note macArgC and callees decomment and communicate to ppC() about newlines in file via is->nlsNotRet.
	SI c;
	do
		CSE_E( macArgC( &c) )	// get char, rif eof/error (unexpected)
	while (isspaceW(c));		// pass whitespace

// '(' begins argument list
	if (c != '(')
	{
		ppErr( (char *)MH_P0009, theDef->id);		// "'(' expected after macro name '%s'"
		macArgUnc(c);					// unget char c
		return RCOK;		// let compilation continue with null args
	}

// scan and count comma-separated arguments until ')'

	for (SI i = 0;  ;  )
	{
		char arg[ARGMAX+1];		// space for arg and \0
		CSE_E( macArg( theDef, arg, &c) )	// scan to , or ) not in () "" or cmt
		if (i < theDef->nA)		// if not too many args
			(*pArgV)[i] = strsave(arg);	// arg to dm (actual size); ptr to arg pointers array
		i++;				// count completed args
		if (c != ',')			// comma separates args
			break;			// on other character, stop
		if (i > theDef->nA + 10)		// if more than 10 too many args !!??
			return ppErn( (char *)MH_P0010, theDef->id );   	/* "Argument list to macro '%s' far too long: \n"
					          		   "    ')' probably missing or misplaced" */
		// fatal error as probably beyond intended position of )
	}

// check termination and number of arguments

	if (c != ')')				// can this happen?
		return ppErr( (char *)MH_P0011);   	// "comma or ')' expected".  fatal as posn end arg list unknown.
	if (i > theDef->nA)
		ppErr( (char *)MH_P0012, theDef->id); 	// "Too many arguments to macro '%s'"
	else if (i < theDef->nA)
		ppErr( (char *)MH_P0013, theDef->id);  	// "Too few arguments to macro '%s'"
	// note macro reference code substitutes "" for missing arg (null argV)

	return RCOK;		// good return
	// additional returns above (including in 'CSE_E' macros)
}							// macArgs
//==========================================================================
LOCAL RC FC NEAR macArg( DEFINE *theDef, char *arg, SI *pC)

// scan one macro arg for macArgs(), return char after arg

// will be recursively entered if arg contains macro reference
{
	SI c, n, nPrn;
	RC rc;

	n = 0;		// char counter
	nPrn = 0;		// ( ) counter
	for ( ; ; )		// loop to scan to ) or , not in "'s, ()'s, or comment
	{

		// get char, decommented, return eof/error (messaged). Sets is->inCmt, to 0 if not in [comment/]quotes.
		CSE_E( macArgC( &c) )	// local fcn, next.  CAUTION can recursively call ppM, ppmId, macArgs, macArg (here)
		// process char
		if (is->inCmt==0)		// if not in quotes nor comment
		{
			switch (c)			// further process char
			{
			case '(':
				nPrn++;
				break;		// (: count () nesting level
			case ')':
				if (nPrn > 0)		// ): if nested,
					nPrn--;		// count nesting level
				else			// unnested )
					goto endArg;		// ends this argument
				break;
			case ',':
				if (nPrn <= 0)		// comma not in parens
					goto endArg;		// ends this argument
				break;
			default:
				break;
			}
		}
		// store char, up to size of receiving array
		if (n < ARGMAX)					// if not too long
			*arg++ = (char)c;
		else if (n==ARGMAX)				// only once per arg.  msg w no file line:
			ppErn( (char *)MH_P0014, theDef->id);    	// "Overlong argument to macro '%s' truncated"
		n++;
	}

endArg: ;			// come here on comma or ) not in () or "" or comment
	*arg = '\0';				// terminate arg[]
	// c is next character, ) or ,
	*pC = c;					// return next char to caller
	return RCOK;				// good return
	// other returns above (CSE_E macro)
#undef ARGMAX
}		// macArg

/*================== variables for macArgC and macArgUnc ==================*/
LOCAL SI NEAR macArgUncFlag =0;	// char-ungotten flag, macArgUnc to macArgC
LOCAL SI NEAR macArgLastC = 0;	// last char returned by macArgC

//==========================================================================
LOCAL void FC NEAR macArgUnc( SI c)

// unget char c gotten with macArgC (next)
{
	/* note: used only after non-fatal error, to improve recovery.
	   Thus, useful even if imperfect in some recursive cases. */

	/* if char matches, decrement buffer subscript to put char back in buffer,
	   so will be seen even though macArgC not used next.  Not yet clear
	   char will match in all cases due to processing in macArgC and macArgCi. */
	if ( is->i > 0
	&& *(is->buf + is->i - 1) == (char) c)		// if is last-gotten buf char
		is->i--;						// unget it
	/* Following known to NOT WORK in case where actually used: missing ( after
	   macro name.  Next char fetch not thru macArgC; leftover ungotten char
	   causes same error in every subsequent macro reference. ... 10-90 */
	else
		macArgUncFlag++;		// tell macArgC to re-return macArgLastC
}		// macArgUnc
//==========================================================================
LOCAL RC FC NEAR macArgC( SI *pC)

// get next input character while scanning macro reference

// this level eliminates \-newlines; callee reads, scans, decomments, etc
// CAUTION: can move is->buf[] contents, invalidating any pointers into it
// returns RCOK ok, other value eof or error (messaged)

// RECURSIVELY called when macro ref arg list ref's a macro with args
{
	SI c;
	RC rc;

	/* Believe a char will never remain in maCNext between macro references
	   because maCNext is char after \ and \ cannot end macro reference.

	   Believe a char will not be in maCNext at start or end valid macro ref
	   NESTED in arg list as \ cannot end identifier nor validily (but not
	   checked til cuparse) precede an identifier. */

	for ( ; ; )				// loop to get char to return: iterate to skip newlines and \-newlines
	{
		// get character

		if (macArgUncFlag)		// if a char was ungotten, reuse it
		{
			c = macArgLastC;
			macArgUncFlag = 0;
		}
		else if (maCNext)		// if have saved char from prior call, use it
		{
			c = maCNext;
			maCNext = 0;	// not checked for \: was char after \.
		}
		else			// normally
		{
			CSE_E( macArgCi( &c) )	// get char, set .inCmt / rif err/eof (msged)

			// strip \-newlines now, so work right in "texts"

			if (c=='\\')
			{
				CSE_E( macArgCi( &maCNext) )	// char after '\'
				if (maCNext=='\n')		// if \-newline
				{
					maCNext = 0;		// forget both of them
					continue;		// and get another char
				}			// else c is returned now, maCNext on next call
			}
		}

		// strip newlines: else cutok.cpp level counts file line at each use of arg.

		if (c=='\n')
		{
			if (is->inCmt== -1)			// if in "text" (set by ppmScan from macArgCi)
			{
				// message newline in quotes in arg list now, and fix it
				ppErr( (char *)MH_P0015);   	// "newline in quoted text"
				c = '"';				// close quotes so no cutok.cpp msg.  note msc 6.0 does this -- tried it.
			}
			else			// newline not in quotes
				continue;		// strip it: go get another char
		}

		macArgLastC = c;		// save char for reUse after unget
		*pC = c;			// return char to caller
		return RCOK;		// good return
		// more returns above (CSE_E macro)
	}		// for ( ; ; )
	/*NOTREACHED */
}					// macArgC
//==========================================================================
LOCAL RC FC NEAR macArgCi( SI *pC)

// get next input character while scanning macro reference, continued

// this level reads, ppmScans, decomments, counts \n's in is->nlsNotRet
// on return, is->inCmt is set for char.
// returns RCOK, RCEOF (messaged here), other (error messaged at source)
// CAUTION: can move is->buf[], invalidating any pointers into it

// RECURSIVELY called when macro ref arg list ref's a macro with args
{
	SI c;
	RC rc;

	for ( ; ; )			// loop to scan repeatedly if comment
	{
		// get some scanned text, if none (nop if is->i still < is->j).
		// NESTED RECURSIVE call to ppM, which can call ppmId, macArgs, macArgC, macArgCi (here again).
		rc = ppM();	/* Reads input; pops file/macro levels at end;
       			   expands macros; does args to curr macro if any;
       			   classifies text setting is->j, ->inCmt, ->isId.
       			   CAUTION can move text, invalidating buf ptrs. */
		if (rc)   		// if error (messaged) or eof (unMessaged)
		{
			if (rc==RCEOF) 				// here, eof is error
				ppErr( (char *)MH_P0016 );  		// "Unexpected end of file in macro reference"
			return rc;
		}
		if (is->i >= is->j)				// debug aid
			return ppErr( (char *)MH_P0017);		// "macArgCi internal error: is->i >= is->j"

		// return 1 space for each comment
		if (is->inCmt > 0)		// if text is->i..j-1 is comment
		{
			// account newlines in comment in macro reference and pass up text
			while (is->i < is->j)
			{
				if (*(is->buf + is->i)=='\n')	// if newline -- else lost
					is->nlsNotRet++;   		// ppC() applies nlsNotRet
				is->i++;				// pass char
			}
			if (is->cmtNx)		// if comment continues
				continue;			// gobble it all now
			c = ' ';		// space for comment for no token concat
		}

		// pass and return 1st remaining character of scanned text
		else
			c = *(is->buf + is->i++);

		// count newlines in macro reference (for file line # in error messages)
		if (c=='\n')		// \n's processed here are not seen by ppC,
			is->nlsNotRet++;	// so ppC adds nlsNotRet to is->line and rets that many extra \n's to caller's buffer

		*pC = c;  		// return char to caller
		return RCOK;		// error returns above
	}
	/*NOTREACHED */
}		// macArgCi
//==========================================================================
LOCAL RC FC NEAR ppmScan()

// scan 1 or more chars from current input and classify as quotes/comments or not, identifier or not, for ppM.

// On return, characters is->buf[is->i..j-1] have been scanned and is->inCmt and is->isId are set for these characters.
{
	SI c, c2 = 0;
	USI span;

#define SCANPAST(p,tx,label) \
	{ span = (USI)strcspn(p,tx); p += span; if (*p==0) goto label; p++; }

// note: file line counting is done by caller ppC().

	/* scan text to find identifiers (possible macro references) not in quotes or comments.
	   Status .inCmt, .isId is used by callers --> must return at start or end of comment or identifier
	                                               so all returned text has same "commentness" and "identifierness". */

	is->inCmt = is->cmtNx;	// init in-comment flag from last call's comment-next flag
	is->isId = 0;		// init is-identifier flag to "not identifier"
	is->cmtNx = 0;		// default to "no comment next" for next call
	char* p = is->buf + is->i;	// working pointer to text. p is used to set is->j after switch.
	switch (is->inCmt)  	// 2 */ comment, 1 // comment, 0 none, -1 ""
	{
	default:
		ppErr( (char *)MH_P0018, (INT)is->inCmt);     	// "ppmScan(): garbage .inCmt %d"
		/*lint -e616 */

	case 0:   			// base case: not in a comment, not in "text"

		// scan to char of interest: ", start-comment, start-identifier
		for ( ; ; p++)
		{
			c = *p;
			if (isalphaW(c) || strchr("$_\"",c))
				break;			// start id or "text" or \0
			if (c=='/')			// '/': possible comment
			{
				c2 = *(p+1);  		// char after /
				if (c2==0)   		// / at end buffer
				{
					if (is->eofRead==0)	// but not at eof (wd hang)
						break;			// break & do chars to / now. Next call redoes / after more text read.
				}
				else if (strchr("*/", c2))	// / or * after / starts cmt
					break;   			// c2 is used below.
				// else continue scannning.
			}
			if (c==0)			// believed redundant: strchr matches
				break;			// end of buffer.
		}
		/* if any chars passed, return them before doing item of interest,
		   because: 1) all chars ret'd must have same .inCmt and .isId;
		   	     2) must ret all chars up to macro ref b4 returning its expansion (in separate inStk[] level);
		   	     3) must ret b4 & after identifiers so caller ppC can change .noMX. */
		if (p > is->buf + is->i)		// if any non-id, non-/ chars
			break;				// return chars. cmtNx is 0.

		// quoted text: skip over it without looking for comments.

		/* Even tho caller ppC is currently looking only for # at start line (start line cannot be in ""'s),
		   we return "text"'s separately (with .inCmt = -1) for generality
		   & to insure correct handling of cases where buffer load ends between "'s. */

		if (c=='"')
		{
			p++;				// pass opening ""
			is->inCmt = -1;			// say in quoted text
			/*lint -e616 */
		case -1:  			// rentry in quoted text
			for ( ; ; )			// scan to end buffer or end "text"
			{
				SCANPAST( p, "\"\n", rebuf)	// advance p past " or newline or goto rebuf if end buf
				if (*(p-2)=='\\')		// (ppRead retains several old chars)
					continue;			// \ before " or \n: keep scanning
				break;			// "text" ends here
				// note terminating \n (when " missing) is returned with .inCmt -1.  macArgC assumes this 9-29-90.
			}
			break;  			// ret "chars" to caller. .cmtNx is 0.
		}

		// identifier: if macro reference, conditionally expand it

		if (c != '/')			// if not comment, is iden
		{
			// pass body of identifier.  Assumed all in buffer.
			while (c = *p, (isalnumW(c) || strchr("$_",c)) && c != 0)
				p++;
			// if a macro & expansion enabled, return expansion, else return the identifier
			is->j = (USI)(p - is->buf);   	// convert p back to subscript
			is->isId = 1;			// say chars is->i..j-1 are identifier
			break;				// return this text to caller
		}

		// start comment.  If here, c is '/' and c2 is next char / or *.

		p += 2;				// pass 2 chars: /, and / or *
		if (c2 != '*')			// then must be '/'
		{
			is->inCmt = 1;			// say rest-of-line comment
			/*lint -e616 */
		case 1:    			// reentry in /-/ comment (to newline)
			for ( ; ; )			// scan to \n not after \, or end buf
			{
				SCANPAST( p, "\n", rebuf)		// advance p past newline or goto rebuf if end of buf
				if (*(p-2)=='\\')			// if \ before the \n
					continue;				// do not end comment
				if (*(p-2)=='\r' && *(p-3)=='\n')	// if \ \r \n sequence
					continue;				// do not end comment
				// above relies on 3 preceding chars always being in buffer -- see ppRead().
				// relies on 3 preceding chars always in buf: see ppRead().
				// >>> be sure required ppRead() changes got done, 9-90
				break;			// no \ b4 \n: end comment
			}
			break;				// set is->j to p and ret. .cmtNx is 0
		}
		is->inCmt = 2;			// else c2 is '*'.  say comment to * /
		/*lint -e616 */
	case 2:   			// reentry in /-* comment (to */)
		for ( ; ; )			// loop to check for / after each *
		{
			SCANPAST( p, "*", rebuf2) 	// advance p past '*', or goto rebuf2 if end of buffer
			if (*p=='/')    		// if / follows *, comment ends
			{
				p++;  			// pass the / too.  .cmtNx is 0.
				break;       		// return comment, thru * / inclusive
			}
			if (*p=='\0'    		// if * is at end buffer
			&& is->eofRead==0) 		// but not at end file (would hang)
			{
				p--; 			// return chars to * now.
				goto rebuf;			// .. Next call will read more text and reconsider * in context.
			}
			// else * is not followed by /. iterate to find next *
			if (*(p-2)=='/')		// warn if / BEFORE *.
			{
				SI isave;
				isave = is->i;
				is->i = is->j = (USI)(p - is->buf); 	// advance to position caret in msg
				ppWarn( (char *)MH_P0019);		// "Comment within comment"
				is->i = isave;			// restore i so caller gets start of cmt
			}
		}  	// for ( ; ; )  scan / * comment
		break;

rebuf2:   			// end of buffer in a /-* comment
		if (is->eofRead)			// if end of file (*/ missing)
		{
			/* to always get here, ppRead() must always return once with is->eofRead set and >=1 char in buf
			   b4 popping to including file/macro.
			   Currently adding \n at end file 8-90 and space at end macro 9-90 insures this. */
			ppErn( (char *)MH_P0020);	// "Unexpected end of file or macro in comment"
			*p++ = '*';			// terminate comment so #line seen
			*p++ = '/';			// ..
			is->n += 2;			// ..
			break;				// is->cmtNx is 0.
		} // else fall thru
rebuf:      			// return at end buffer
		is->cmtNx = is->inCmt;		// on reentry, same state
		break;				// set is->j from p and return
	}   	/*lint +e616 */  // switch (inCmt)

// if here, caller may use chars up to 'p'; is->inCmt and ->isId are set.

	is->j = (USI)(p - is->buf);			// convert p back to subscript and store
	return RCOK;				// good return to caller

#undef SCANPAST
}						// ppmScan
//==========================================================================
LOCAL RC FC NEAR macOpen( 	// initiate expansion of macro or argument

	char *id, 		// macro or arg name, pointer into another heap block.
	char *text, 	// macro or arg body/value, heap block dmIncRef'd by caller, decref'd by ppClI.
	DEFINE *theDef,	// NULL or macro definition block pointer, dmIncRef'd by caller, decref'd by ppClI.
	char **argV, 	// NULL or macro arg value pointers array, free'd by ppClI.
	enum ISTY ty )	// tyMacro (or as changed)
{
	if (inDepth >= INDEPTH)
		return ppErr( (char *)MH_P0021 );	// "Too many nested macros and #includes"
	// continues, OMITTING macro or arg
	//>>> does it really CONTINUE 9-90??  <--- yes, 6-92, and it makes a mess.  otta change. ********
	INSTK NEAR * isi = is + 1;		// temp local ptr to nxt inStk[] level
	memset( isi, 0, sizeof(INSTK));	// inits many members
	isi->ty = ty;   			// type: macro or arg, not incl file
	isi->mName = id;   			// macro/arg name. is it used?
	isi->buf = text;			// macro body text is "buffer".
	isi->theDef = theDef;		// macro definition, for arg names
	isi->argV = argV;			// macro arg info or NULL
	isi->bufSz =				// need we set buffer size?
	isi->n =  					// # chars in "buffer"
		(USI)strlen( text);			//   = entire macro body is in buf
	// >>> otta store length at #define
	isi->eofRead = 1;			// tell ppM & ppRead: entire body is in buf
	isi->lineSent = 1;			// no #line command should be sent for macro
	is = isi;				// now set global pointer to inStk[inDepth-1]
	inDepth++;				// incr global include/macro nesting level
	return RCOK;			// good return after opening macro
}		// macOpen
//==========================================================================
LOCAL RC FC NEAR ppRead()		// Get some input in current level file input buffer, for ppM.

/* Gets at least enough chars to include a whole identifier,
   and enough before as well as after position to usually
   include whole file line (for error messages). */

/* Reads file if necessary.  At end of inputs, proceeds from macros to caller
   and from included file to including file */

/* returns RCEOF if end of all levels of source input (no msg issued here),
   or other non-RCOK value if error that should terminate compile. */
{
// defines at start file used here:
//   BUFKEEP  #chars already processed (before is->i) to retain
//   BUFMIN   read if < this many unprocessed chars in buf (after is->i)
//   NREAD    read this many chars if read
//   BUFTAIL  need this many bytes at end buffer, after chars read
	USI n;

	for ( ; ; )			// loop to pop levels til get text
	{
		// read if too few unprocessed chars in buffer if a file (if a macro, entire macro is in buffer at open)

		if ( is->eofRead==0 		// if eof not yet in buf (& not macro)
		&&  is->n - is->i < BUFMIN  	// if have too few chars
		&&  is->ty==tyFile)		// if file, not macro (redundant)
		{
			if (is->i > BUFKEEP)		// if too many processed chars in buf
			{
				n = is->i - BUFKEEP;			// # chars to discard
				lisToChar( is, n, LtyNul);			// be sure chars to be discarded have been listed
				memmove( is->buf, is->buf + n, is->n - n);	// move to front
				is->i -= n;
				is->j -= n;			// adjust subscripts
				is->n -= n;
				is->ech -= n;			// ..
			}
			n = min( USI(NREAD), USI(is->bufSz - is->n - BUFTAIL) );	// # to read: insrnce.
			n = (USI)fread( is->buf+is->n, 1, n, is->file);		// c lib. n = # read.
			if (n==0)						// if end file
			{
				is->eofRead++;					// once only
				/* supply newline if absent at end (include) file.
				   ppC may (8-90) also put newline after file, but file name in
				   last line errmsgs would be wrong.
						If changed, review what ppmId() does at $eof.
						Also insures a return to caller with eofRead set for this
				 file before pop -- eg for ppM "unexpected EOF in cmt" msg */
				if (is->buf[is->n-1] != '\n')
					is->buf[is->n++] = '\n';
			}
			is->n += n;						// subscr last+1 char in buf
			is->buf[is->n] = '\0';				// null-terminate buffer
			if (n)						// is it ok to do memchr(,,0) ?
				for ( ; ; )				// change any \0's to spaces to prevent hang elsewhere
				{
					char *q = (char *)memchr( is->buf + is->n - n, '\0', n);	// search for null in text just read
					if (!q)	 break;							// leave loop if none (normal case)
					/* Issue error msg. Stops run via errCount. Line # in msg is too small cuz ahead of tokenizer here.
					   ppErn as opposed to ppErr: do not show file line & caret & msg in input listing cuz a) wrong line,
					   b) wrong place to use ppErr: gets lisFind program error cuz some line # not sync'd, 8-92. */
					ppErn( (char *)MH_P0024);			/* "Replacing illegal null character ('\\0') with space\n"
								   "    in or after input file line number shown" 10-92 */
					*q = ' ';					// replace \0 with space to not terminate buffer b4 end
				}    				                // iterate to rescan to see if another null
		}

		// if any unprocessed text in buffer, done

		if (is->n > is->i)
			return RCOK;

		// buffer empty here means end include file or macro

		if (inDepth <= 1)	// if top level (top close is left to caller)
			return RCEOF;		// say end of input -- EOF for whole compile
		ppClI();			// close last opened file or macro. decrements "is", and "isf" if file.
		// loop to read file and/or close more levels if necessary
	}
	/*NOTREACHED */
}			// ppRead


/*==================== INPUT LISTING (ECHO) department ====================*/
static void lisBufAllocIf()
{
	if (!lisBuf)					// file-local variable
	{
		if (dmal(DMPP(lisBuf), LISBUFSZ + 1, WRN)) 	// allocate, dmpak.cpp / if error.  +1 for \0 used in lisBufOut
			return;					// out of memory, msg issued in dmal.
		lisBuf[0] = 0;				// terminate empty buffer with a null
		lisBufN = 0;				// buffer is empty (file-local at start file)
		lisLs = begLine;   			// say at start line (dept-local)
		lasTy = LtyBody;   			// current text is not preproc cmd nor under if'd out (dept-local)
		lisBufEchoOff = 0;			// say echo is on
	}

}		// lisBufAllocIf
//-----------------------------------------------------------------------------------------------------------
void FC lisFlushThruLine( int line)	// list and FLUSH LISTING BUFFER thru specified line number

// call when list report for a run is to be terminated.

// Avoids flushing beyond line as may belong to next run (if last run, other code flushes entire buffer at file close).

{
	// for cul.cpp:culRun

	lisThruLine(line);					// first, be sure all desired text is IN lisBuf

// find where in lisBuf given line and any following error messages end === start of next file line

	char* lis = lisBuf + lisBufN;				// point end of lisBuf. expect right after a \n.
	int tLine = isf->echLine;				// line # at end lisBuf -- # of line not yet in buffer
	while (tLine > line+1  &&  lis > lisBuf)		// if necess, search back to start of file line after specified line
	{
		if (lis > lisBuf  &&  *(lis-1)=='\n')		// back over ONE line feed / newline to preceding line
			lis--;
		while (lis > lisBuf  &&  *(lis-1) != '\n')	// find line start: position after preceding newline
			lis--;
		if (isFileLisLine(lis))				// change line number on backing over listed input file line
			tLine--;					/* ... not on backing over ----, ermsg, etc lines.
							   Thus error messages stay with line above,
							   loop stops at start of file line # "line+1",
							   so line "line" and its msgs are last things flushed. */
	}

// write all characters in lisBuf up to found position

	lisBufOut( lis - lisBuf);				// local, below

	// any remaining lisBuf text may be preprocessed-ahead commands for next run, and REMAINS IN LIST BUFFER.

}		// lisFlushThruLine
//-----------------------------------------------------------------------------------------------------------
void FC lisThruLine( int line)		// list thru specfied line number if possible

// call before outputing an error message to listing

{
	// may be called locally, in ppErv, cutok.cpp:cuErv, .

	if (inDepth <= 0)			// if no file open, do nothing
		return;				// insurance -- eg ppErv can be called during command line checking

// list thru end of requested line
	char* p;
	if (line < isf->echLine + 100)	/* if requested line # many lines ahead of listing, list no lines:
        				   Wrong file, macro's line, bug, or many many short or blank lines.
        				   Note fallthru lists no lines if req'd line # < last listed line # */
	{
		int tLine = isf->echLine;		// line # at end of echoed text
		p = isf->buf + isf->ech;		// point 1st unechoed character
		while (*p  &&  tLine <= line )	// search text to LINE AFTER given line
		{
			p += strLineLen(p);   	// advance past \n or to \0 (lib\strpak.cpp)
			if (*(p-1)=='\n')		// if passed \n
				tLine++;   		// count line
		}
		lisToChar( isf, p - isf->buf, LtyNul);	// list text UP TO start of LINE AFTER requested line # (next)
	}

// list to after a newline -- insurance against inserting message in mid-line

	if (lisLs==midLine)			// file-local variable above
	{
		p = isf->buf + isf->ech;		// point 1st unechoed character
		p += strLineLen(p);				// advance past \n or to \0 (lib\strpak.cpp fcn)
		lisToChar( isf, p - isf->buf, LtyNul);
	}
}			// lisThruLine
//----------------------------------------------------------------------------------------------------------
static void lisLinePfx(		// append leftmost 8 chars of line
	int incDpth,			// include depth
	LISTY lineTy=LtyNul)	// line type
{
	incDpth = min(incDpth, 8);
	lisBufAppend("IIIIIIIIII", incDpth);	// one I for each level of include nesting, if any
	const char* tytx =
			  lineTy == LtyIf0 ? "0       "		// get "0   " under false if,
			: lineTy == LtyPpc ? "#       "		// or "#  " if preprocessor cmd,
			:                    "        ";	// else blank
	lisBufAppend(tytx, 8 - incDpth);   	// append enuf chars to total 8 after I's (8: tab alignment)
}		// lisLinePfx
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR lisToChar( 		// list input to given character position

	INSTK NEAR * _is,	// input stack entry to list -- eg 'is' from ppC, 'isf' from lisToLine, etc.
	int echTo, 		// _is->buf subscript up to which to echo input
	LISTY newTy, 	// (line type (LtyNul, LtyBody, LtyIf0, LtyPpc), starting after last \n before buf[ech]
   					//   PRECEDING type applies to any preceding unlisted lines)
	int flush )   	// nz to clear buffer: use when starting or ending include, so all lisBuf lines are same file

{
	/* called during preprocessing, in particular whenever there is a change of line type
	   or a change of input file (flush buffer since line #'s change) */

	// call to this fcn not expected with no input file open

	if (newTy==LtyNul)			// if new type is "no type specified"
		newTy = lasTy;			// default to last type given (or initial value)

	if (_is->ty==tyFile)   // list only file input; skip macro expansions (poss future option to list?)
							//  (but for macros, do save line type (?), and do do 'flush', below).
	{

		// initialize if buffer not yet allocated.  See also lisMsg.
		lisBufAllocIf();

		// characters to list
		char* p = _is->buf + _is->ech;
		char* stopAt = _is->buf + echTo;

		// find start of line where new type applies
		//   = newline not at end of line
		char* tyChangeAt = stopAt;
		while (tyChangeAt > p && *(tyChangeAt - 1) == '\n')
			tyChangeAt--;
		while (tyChangeAt > p && *(tyChangeAt - 1) != '\n')
			tyChangeAt--;
		// list line(s) to given character position
		while (p < stopAt)
		{
			// beginning of line stuff
			// CAUTION: if # columns (8) changed, check lisLinePfx, lisMsg, lisFind, lisInsertMsg, .

			if (lisLs != midLine)
			{
				int incDpth = _is - inStk;		// known bug: counts macros that contain #includes
				if (p >= tyChangeAt)    		// if at/after last newline before buf[echTo]
					lasTy = newTy;				// change to line type specified in this call
				lisLinePfx(incDpth, lasTy);		// append leftmost 8 chars ("I#" etc)
				lisLs = midLine;
			}

			// body of line -- or as much as precedes stopping point

			int n = strLineLen(p);			// line length, including \n (or to \0), strpak.cpp
			if (p + n >= stopAt)			// if more than caller said to echo now
				n = stopAt - p;  			// reduce #
			lisBufAppend( p, n);			// put these chars in listing
			p += n;   						// point past
			_is->ech += n;    				// keep track of next character to echo
			if (*(p-1)=='\n')				// if \n is last char listed
			{
				lisLs = begLine;				// say at start line
				_is->echLine++;				// count line # in lisBuf
			}
		}
	}

	lasTy = newTy;		// insurance: if didn't pass a line start, store caller's
    					// new type for any following lines for which no type given
	if (flush)				// on option
		lisBufOut(0xffff);		// output all characters now in list buffer
}			     // lisToChar
//-----------------------------------------------------------------------------------------------------------
void FC lisMsg( 				// append message to end of listing

	char *p,		// text. not expected to end in newline.
	int dashB4, 		// non-0 if ----------- line desired above text.  Newline always supplied if needed b4 text.
	int dashAf )		// ditto below.    Newline always supplied after text.

{
	if (inDepth==0)  				// nop if no file open (eg command line error)
		return;

	int lisBufEchoOffSave = lisBufEchoOff;
	lisBufEchoOff = 0;		// enable echo so msg shows

// initialize if buffer not yet allocated.  See also lisToChar.
	lisBufAllocIf();

// conditional newline, optional dashes before message.  CAUTION: if use of -----'s changed, check lisFind.
	if (lisLs==midLine)
		lisBufAppend( "\n", 1);
	if (dashB4  &&  lisLs != dashed)
		lisBufAppend("-----------------------\n");	// 8 + 15 = 23 -'s

// message, with ???'s at start lines.  CAUTION: if ? in col 1 changed, check lisFind, lisInsertMsg

	while (*p)
	{	lisBufAppend( "???      ", 8);	// prefix each line.  Use 8 cols so tabs in message work right.
		int n = strLineLen(p);			// line length, including \n (or to \0), strpak.cpp
		lisBufAppend( p, n);
		p += n;
	}

// newline, optional dashes after message

	lisBufAppend( "\n", 1);
	lisLs = begLine;
	if (dashAf)
	{	lisBufAppend("-----------------------\n");
		lisLs = dashed;
	}
	lisBufEchoOff = lisBufEchoOffSave;		// restore echo state
}			// lisMsg
//-----------------------------------------------------------------------------------------------------------
LOCAL bool lisBufEchoOnOff(	// enable/disable input listing
	bool turnOn,	// true: enable input listing
					// false: disable input listing
	const char* pSrcText)	// initiating source text
							//   allows echo of #ECHOON
// returns true iff input echo currently enabled
{
	// worry about lisBuf nullptr?

	int incDpth = is - inStk;

	if (turnOn)
	{	// re-enable echo
		lisBufEchoOff--;
		if (lisBufEchoOff == 0)			// if final nested #ECHOON
		{	lisLinePfx(incDpth, LtyPpc);
			lisBufAppend(pSrcText);		// echo #ECHOON line (with any comments)
		}
		if (lisBufEchoOff < 0)
		{	lisBufEchoOff = 0;			// ignore redundant #ECHOONs
			// pInfolc("Ignoring redundant #ECHOON");  does not display correct file/line info, abandon
		}
	}
	else
	{	// suppress echo
		if (lisBufEchoOff == 0)
		{	lisLinePfx(incDpth, LtyPpc);
			lisBufAppend("... Suppressing input echo until matching #ECHOON\n");
		}
		lisBufEchoOff++;
	}

	return !lisBufEchoOff;

}		// lisBufEchoOnOff
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR lisBufAppend( 	// append given bytes to input listing buffer
	const char *p,		// bytes to append
	int n /*=-1*/)		// # bytes to append, or <0 to append to \0
{
	if (lisBufEchoOff > 0 || !p)
		return;		// not echoing

	if (n < 0)
		n = strlen(p);

	if (lisBuf)						// if buffer allocated (autoallocates in lisToChar)
	{
		while (n)					// until all bytes output
		{
			int m = min( LISBUFSZ - lisBufN, n);
			if (m)
				memcpy( lisBuf + lisBufN, p, m);		// transfer all or what fits
			lisBufN += m;
			p += m;
			n -= m;
			if (n)					// if there is more to append, write front of buffer
				lisBufOut(LISBUFWRITE);			// .. much of buffer is retained for lisFind
		}
		lisBuf[lisBufN] = '\0';				// keep buffer terminated.  note extra +1 in allocation size.
	}
}				// lisBufAppend
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR lisBufOut( int n)		// output bytes from list buffer. n = large number for all.

{
	// assume if lisBuf not allocated, lisBufN is 0.

	if (n > lisBufN)			// large number may be given to write entire buffer
		n = lisBufN;

	lisWrite( lisBuf, n);		// write n bytes from front of lisBuf (below)

// discard bytes from buffer (even if not written, so calling code does not lock up)

	if (lisBuf)						// if not allocated, don't move 1 byte at 0:0, 1-92.
		memmove( lisBuf, lisBuf + n, lisBufN - n + 1);	// move following bytes to be kept, + null, to front of buffer
	lisBufN -= n;
}			// lisBufOut
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR lisWrite( char *p, int n)		// write to listing (echo) output if any
{
	if (n)			// if any bytes to output
		if (VrInp)		// if input listing virtual report requested, and opened ok (in cncult.cpp).  Variable in this file.
		{
			// first time, write listing report title

			/* title for report is deferred til 1st output to 1) omit it if there is no output (keeping virtual
			   report 0 length if no output); 2) possibly get runTitle into it by deferring formtting;
			   3) also keeps report together in virtual report spool file. */

			if (vrIsEmpty(VrInp))				// if no text in virtual report yet
				vrStr( VrInp,
					   getInpTitleText() );		// format title text for this report, cncult.cpp

			// store terminating null and output n characters as null-terminated string
			char saveC = p[n];
			p[n] = 0;				// plant terminull
			vrStr( VrInp, p);      	// write null-terminated string to virtual report. vrpak.cpp.
			p[n] = saveC;
		}
}			// lisWrite
//-----------------------------------------------------------------------------------------------------------
// The following functions supports INSERTING ERROR MESSAGES in the listing after the file line they relate to.
// This is done by matching the file line in the message to the buffer contents; it it is not found
//  caller uses a different message format in which the line is repeated.
// Matching line for Cu messages will not always be found because
//   a) cutok is using preprocessor output; if any preprocessing was done, match will not be found;
//   b) cutok can run several hundred bytes behind preprocessor; matching line might be gone from buffer
//      after many error messages, flush due to #include, etc.
//-----------------------------------------------------------------------------------------------------------
int FC lisFind( 					// find matching file listing line

	int fileIx,				// input file name index
	int line, 				// line number
	const char* p, 			// text to find, may be multiple lines, null-terminated
	int* pPlace )			// receives subscript in listing buffer of 1st char of LINE AFTER
    						// matching line and any already-inserted message lines following it
// returns 0 if not found
{

	if ( inDepth <= 0  ||  !isf	// if no file open (insurnace)
	  || lisBufEchoOff > 0		// if not echoing (text probably not in buffer)
	  || !fileIx				// if 0 file index (check probably not needed)
	  || fileIx != isf->fileIx	// if wrong file name (e.g. #include has just started or ended)
	  || !lisBuf )				// if no listing buffer allocated
		return 0;			// return "not found" without searching

	lisThruLine(line);			// make sure listing is complete thru given line and ends in \n; probably redundant here

	int place = lisBufN;			// pointer to start of line under test
	int place2 = lisBufN;			// pointer to end of line under test
	int tLine = isf->line;			// line number of listing text under test

// search back for matching listing line, return if not found

	// expect 'place' is after \n at end buf; expect one preliminary iteration.
	// (if no final \n and last line matches, there will be no \n preceding returned 'place'.)

	for ( ; ; )
	{
		if (place2==0)					// if line end is beginning of buffer
			return 0;					// done search, not found, return bad

		while (place > 0  &&  lisBuf[place-1] != '\n')	// find line start: position after preceding newline
			place--;
		int nl;
		if (!lisCmp( p, place, pPlace, &nl))
		{
			if ( tLine + nl - 1 == line			// if last line compared is requested line number
			 ||  tLine + nl     == line )		// ... ('line' can lag listing by 1 in #cmd)
				return 1;
#if 1	// debug
			else
				err( PWRN, (char *)MH_P0022,  	// "pp.cpp:lisFind: matching buffer line is %d but requested line is %d\n"
					 INT(tLine+nl-1), (INT)line);
#endif
		}

		if (isFileLisLine( lisBuf + place))			// if not errmsg or -------- or #line line
			tLine--;						// move to preceding line # in file being listed
		if (place > 0  &&  lisBuf[place-1]=='\n')  place--;	// back over ONE line feed / newline to preceding line
		if (place > 0  &&  lisBuf[place-1]=='\r')  place--;	// back over carriage return before line feed
		place2 = place;						// set line end; new line start found at loop top.
	}
	/*NOTREACHED */
}			// lisFind
//-----------------------------------------------------------------------------------------------------------
LOCAL int FC NEAR lisCmp( 		// multi-line compare text to listing text

	const char* s,	// text to compare to listing
	int i,			// subscript of place in listing buffer, with 8-col prefixes and ---- and ? lines to skip over
	int* pPlace,	// if matches, receives subscr of place in listing buffer AFTER matched lines and any ? and ---- lines
	int* pnl )		// if matches, receives number of lines compared

// returns 0 if matches
{
	char *lis = lisBuf + i;		// pointer to text in listing buffer
	int nl = 0;				// number of lines compared

	for ( ; ; )
	{
		// find next listing line to test (or ptr to return): advance over '?' lines, ---- lines, and #line lines.
		for ( ;  strlen(lis) > 8  &&  !isFileLisLine(lis);  lis += strLineLen(lis) )
			;

		// find next string line to test; done if no more
		if (!memcmp( s, "#line", 5))
			s += strLineLen(s);			// pass #line command (> 1 unexpected).
		if (!*s)   				// if no more string to test, done
		{
			*pPlace = lis - lisBuf;	// return subscript of place in listing buffer
			*pnl = nl;				// return number of lines compared
			return 0;				// say matches
		}

		// compare this line
		lis += 8;				// point after prefix part of listing line
		size_t lls = strcspn( s, "\r\f\n" );	// line body length -- do not count \r, \n, etc
		if (strcspn( lis, "\r\f\n" ) != lls)
			return 1;				// different line lengths, no match
		if (lls)
		{
			if (memcmp( lis, s, lls))
				return 1;				// different text, no match
		}
		s += strLineLen(s);
		lis += strLineLen(lis);	// point next line in text and listing.  lib\strpak.cpp fcn.
		nl++;						// count lines compared
	}
}		// lisCmp
//-----------------------------------------------------------------------------------------------------------
LOCAL int FC NEAR isFileLisLine( char *p)	// 0 if listing line is error message, ----- separator, or #line
{
	return ( *p != '?'  				// is file line if not error message,
			 &&  memcmp( p, "----", 4)			// not separator,
			 &&  memcmp( p + 8, "#line", 5) );		// not #line. note +8.
}					   // isFileLisLine
//-----------------------------------------------------------------------------------------------------------
void FC lisInsertMsg( 				// insert error message in listing buffer

	int place, 		// subscript of position. assumed to be after a \n.
	char *p,    	// text to insert.  assumed NOT to end with \n.
	int dashB4, 		// non-0 if ----------- line desired above text.  Newline always supplied if needed b4 text.
	int dashAf )		// ditto below.    Newline always supplied after text.

{
	// for ppErv, cutok.cpp:cuErv to use after finding place with lisFind and reformatting message

	int n  /*=strlen(p)*/;				// (redundant init removed 12-94 when BCC 32 4.5 warned)

// optional dashes before message.  CAUTION: if use of -----'s changed, check lisFind.

	if (dashB4  &&  lisLs != dashed)
		lisBufInsert( &place, "-----------------------\n");	// 8 + 15 = 23 -'s

// message, with ???'s at start lines.  CAUTION: if ? in col 1 changed, check lisFind, lisMsg.

	while (*p)
	{
		lisBufInsert( &place, "???     ", 8);		// prefix each line.  Use 8 cols so tabs in message work right.
		n = strLineLen(p);				// line length, including \n (or to \0), lib\strpak.cpp
		lisBufInsert( &place, p, n);
		p += n;
	}

// newline, optional dashes after message.

	lisBufInsert( &place, "\n", 1);
	if (dashAf)
		lisBufInsert( &place, "-----------------------\n");
}				// lisInsertMsg
//-----------------------------------------------------------------------------------------------------------
LOCAL void FC NEAR lisBufInsert( 			// listing buffer inserter inner function

	int* pPlace, 		// subscript of place to insert, returned updated to end of inserted text
	char *p, 			// text
	int n/*=-1*/)		// length, default = use strlen

{
	int place = *pPlace;
	if (n < 0)
		n = strlen(p);

	while (n)				// may need repeated insertions for long message
	{
		if (LISBUFSZ - lisBufN < n)		// write front of buffer if not enuf empty space for (remaining) message
		{
			int m = min(place, LISBUFWRITE);	/* # bytes to write when write needed: up to LISBUFWRITE even if > n, cuz:
						     a) each write moves buf (slow) (expect multiple calls from lisInsertMsg);
						     b) if we write all text b4 'place', can write msg directly to listing. */
			lisBufOut(m);				// output m bytes from front of buffer; checks for m==0.
			place -= m;
		}

		if (place==0)				// write direct if place is (now) at very beginning of buffer
		{
			lisWrite( p, n);
			n = 0;
		}
		else					// move up tail of buffer and copy text into vacated space
		{
			int hole = min( n, LISBUFSZ - lisBufN);		// smaller of req'd space, space avail at end buffer
			if (hole)									// insurance
			{
				memmove( lisBuf + place + hole, lisBuf + place, lisBufN - place + 1);	// +1 to move \0 too
				lisBufN += hole;
				memcpy( lisBuf + place, p, hole);
				place += hole;
				p += hole;
				n -= hole;
			}
		}
	}

	*pPlace = place;			// return updated position subscript so caller can add more

}			// lisInsert
//-----------------------------------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
// preprocessor char fetch/tokenize, and error messages
/////////////////////////////////////////////////////////////////////////////////////


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

// char fetch & tokenize for pp cmds
//unused: LOCAL void FC NEAR ppUntok( void);
LOCAL SI FC NEAR ppScanto( char *set);
LOCAL void FC NEAR ppUncNdc( void);

// error messages
LOCAL RC FC NEAR ppErv( SI shoTx, SI shoCaret, SI shoFnLn, SI isWarn, char *fmt, va_list ap);

/*============ CHAR FETCH & TOKENIZE for preprocessor commands ============*/

/* also used to scan selected program startup command
   line arguments, e.g. -Dsym=value (from pp.cpp) */

/*-------- PUBLIC VARIABLES for preprocessor char fetch and tokenize -------*/


/*-------- LOCAL VARIABLES for preprocessor char fetch and tokenize -------*/

// current preprocessor command line. CAUTION: buf may be in caller's stack.
LOCAL bool ppcIsClarg{ false };			// true iff is program command line text not file text (affects errMsgs)
LOCAL const char * NEAR ppcBp = NULL; 	// ptr to start of buffer
LOCAL const char * NEAR ppcP = NULL;   	// ptr to current position in decode
LOCAL SI NEAR ppcEndRead = 0;			// nz if end buf read: do not unget

LOCAL SI NEAR ppChar = 0;		// last char ret'd by ppCDc()
LOCAL SI NEAR ppRechar = 0;		// 1 to ret same char again, ppUncDc-->ppCDc
LOCAL SI NEAR ppTokty = 0;		// token type (same as ppTok return)
//unused:  LOCAL SI NEAR ppRetok = 0;	// nz to re-ret same tok: ppUntok(),ppTok()

//===========================================================================
void FC ppTokClean( 		// ppTok init/cleanup routine

	[[maybe_unused]] CLEANCASE cs )	// ENTRY, DONE, or CRASH (type in cnglob.h)

//called from pp.cpp:ppClean.
{
	ppctIni(nullptr);	// inits ppcBp, ppcP, ppcIsClarg, ppcEndRead, ppRechar. Just below.

	//ppTokTx[0] = ppIdtx[0] = 0;	beleived not needed.
}		// ppTokClean

//===========================================================================
void FC ppctIni(	// init or terminate char fetching/tokenizing from cmd line arg or pp command

	const char* p,	// Extracted preprocessor command text or command line argument text,
   					// or NULL to say neither being scanned.
   					// NULL affects text display in error message fcns; fetch/tokenize fcns here not used for file input. */
	bool isClarg )	// non-0 if command line argument

{
	// CAUTION: command buffer p may be in caller's stack
	ppcBp = p;			// ptr to start of buffer, or NULL if none (NULL is signal to error functions)
	ppcP = p;   		// ptr to current position in decode
	ppcIsClarg = isClarg;	// nz if cmd line arg not pp cmd buffer
	ppcEndRead = 0;		// is set nz if end buf read: do not unget
	ppRechar = 0;		// 1 for ppCDc to ret same char again
#if 0	// ppUntok unused
x   ppRetok = 0;		// nz for ppTok to ret same token again
#endif
}			// ppctIni

#if 0	// ppUntok unused
x //===========================================================================
x LOCAL void FC NEAR ppUntok()
x
x // unget token: make next ppTok return it again.
x
x 	// ppIdtx, ppSival, etc must be undisturbed
x {   if (ppTokty)
x       ppRetok++;
x }			// ppUntok
#endif

//===========================================================================
SI FC ppTok()

// get next pp cmd expression (after #if) token, return type

	// sets: ppSival: value if number; ppIdtx: text if identifier.

	/* function value is token type (CUT defines in cutok.h), including:
	    CUTID:    identifier, in ppIdtx. 1-n letters, digits (not digit first)
	    CUTSI:    integer, value in ppSival.
	    		note sign not considered part of number here.
	    CUTEQL ==, CUTNE !=, CUTLE <=,
	    etc. */
{

reget:	// here to start token decode over (eg after illegal char)

	ppSival = 0;		// no numeric value yet

// pass whitespace to start token, get first character
	SI c;
	while (c = ppCDc(), isspaceW(c) != 0)	// decomment, get chars
		;
	const char* p0 = ppcP-1;   	// ptr to start token, used at end to set ppToktx[].

// peek at 2nd char of token

	SI c2 = ppCNdc();  		// peek at 2nd char, no decomment yet,
	ppUncNdc();			// leaving it in file too
	c2 = tolower(c2);		// lower case (eg for 0x)
	// c2 is 2nd char, ungotten.

// digit first: decode as integer

	SI base;
	if (isdigitW(c))
	{
		LI tem;
		// set up re 0x, 0o prefixes
		if (c=='0' && c2=='x')		// 0x<digits>: hex
		{
			base = 16;
			ppCNdc();			// gobble the x (c2 previously ungotten)
		}
		else if (c=='0' && c2=='o')	// 0o<digits>: octal
		{
			base = 8;
			ppCNdc();			// gobble the o (c2 previously ungotten)
		}
		else
			base = 10;

		// digits syntax and integer value loop
		do
		{
			tem = (LI)ppSival * (LI)base + (LI)(c-'0');	// SI value
			ppSival = (SI)(USI)tem;
			c = ppCDc();				// next token char
			if (!isalnumW(c))			// if not digit, or letter for hex
				break;				// done
			if (c >= 'A')				// make hex letters
				c = toupper(c) - 'A' + '9' + 1;	//  follow 0-9
		}
		while (c >= '0' && c < '0' + base);	// 0-7, 0-9, or 0-f
		ppUncDc();				// unget last (non-digit) char
		ppTokty = CUTSI;				// say is integer, value in ppSival
		if ( base==10 && tem > 32767L		// decimal: +- 32767 (no -32768)
				||  tem > 65535L )			// 0x or 0o: user beware of -
			ppErr( (char *)MH_P0070 );		// error message: "Number too large, truncated"

	}	// if isidigit(c)

// letter, $, or _ first: decode identifier

	else if (isalphaW(c))  	// letter begins identifier
	{
iden:			// other id 1st chars join here, from switch below
		ppUncDc();		// unget 1st char
		ppcId(WRN);		// get identifer to ppIdtx[], below.
		//  error return "impossible" here.
		ppTokty = CUTID; 	// say is an identifier
	}

// other initial characters

	else
	{
		// 2-char tokens ending in =: <=, ==, !=, etc.
		if (c2 == '=')
		{
			static const char c1s[] = { '=',    '!',   '<',   '>',  0 };
			static const SI gnuts[] = { CUTEQL, CUTNE, CUTLE, CUTGE };
			const char *p = strchr( c1s, c);		// find char in string
			if (p)					// if found
			{
				ppTokty = gnuts[(SI)(p-c1s)];		// type from parallel array of SI's
				ppCDc();					// get char to make c2 part of token
				goto ret;					// go set ppToktx and return ppTokty
			}
		}

		// 2-of-same-char tokens:  >>  <<
		if (c2 == c)
		{
			static const char cs[] = { '>',    '<',    0 };
			static const SI cuts[] = { CUTRSH, CUTLSH };
			const char *p = strchr( cs, c);		// find 1st char in string
			if (p)					// if found
			{
				ppTokty = cuts[(SI)(p-cs)];		// type from parallel array of SI's
				ppCDc();					// get char to make c2 part of token
				goto ret;					// set ppToktx, return ppTokty
			}
		}

		// end comment */ (take as single token for specific error message)
		if (c=='*' && c2=='/')
		{
			ppTokty = CUTECMT;
			ppCDc();		// get char to make c2 part of token
			goto ret;		// set ppToktx, return ppTokty
		}

		// cases for other initial characters

		switch (c)	// note c2 is next, ungotten, character
		{
		case '$':
		case '_':	// chars that start identifiers
			goto iden;		// join identifier code, above

		case EOF:		// end of file
			ppTokty = CUTEOF;
			break;

		default:  		// others are 1-char tokens
			// reject random non-whitespace, non-printing characters now
			if (c < ' ' || c > '~')
			{
				ppErr( (char *)MH_P0071, (UI)c);   	// "Ignoring illegal char 0x%x"
				goto reget;
			}
			// single:		// 1-char cases may here or fall in
			/* token type for 1-char tokens is ascii sequence with ctrls,
			   digits, letters squeezed out (for compact tables elsewhere) */
			{
				static const UCH tasc[] = { '{', '[', ':', ' ', 0 };
				static const UCH tbase[] = { 29,  23,  16,  0,  0 };
				SI i;
				for (i = 0; (UCH)c < tasc[i]; )		// find range containing char
					i++;
				ppTokty = c - tasc[i] + tbase[i];	// compute token type. MATCHES DEFINES IN cutok.h.
			}
			break;

		}	// switch(c)
	}	// if digit ... else if alfa ... else ...

ret: ;		// here to set ppToktx and return

	int n = min( USI((ppcP-ppRechar) - p0), 	// len, adj for ungotten char,
				USI(sizeof(ppToktx)-1) );  	//  truncate to buf size
	memcpy( ppToktx, p0, n);			// token text for errMsgs
	*(ppToktx + n) = '\0';			// terminate

	return ppTokty;
	// another return above
}				// ppTok

//==========================================================================
RC FC ppcId( int erOp)

// get non-macro-expanded identifier for preprocessor.

// rets identifier text in ppIdtx[], or non-RCOK if none present, issuing err msg unless era==IGN.

{
// deblank start?? >>> permits spaces after '#' >>> not needed for ppTok.
	SI c;
	do   c = ppCDc();
	while (isspaceW(c));

// first char must be letter, _, $
	if (!(isalphaW(c) || strchr( "$_", c)) )
		return erOp==IGN ? RCBAD : ppErr( (char *)MH_P0072);		// "Identifier expected"

// scan to end identifier. following chars may also be digits
	USI n = 0;
	do
	{
		if (n < sizeof(ppIdtx)-1)	// truncate if over 32 chars
			ppIdtx[n++] = (char)c;	// store for caller
		c = ppCDc();			// next char
	}
	while (isalnumW(c) || strchr("$_", c));	// stop at non-id char
	ppUncDc();					// unget the non-id char
	ppIdtx[n] = 0; 				// terminate
	return RCOK;
}			// ppcId

//==========================================================================
SI FC ppCDc()		// get preprocessor command char, decommented

// CAUTION: does not handle quotes -- caller must.
// returns whitespace for a comment.  returns EOF at end of buffer.
{
	if (ppRechar)
	{
		ppRechar = 0;
		return ppChar;
	}

	ppChar = ppCNdc(); 			// get char from file char level, below
	if (ppChar != '/')			// if not a /
		return ppChar;			// cannot begin a comment, return to caller
	int c2 = ppCNdc();   		// char after /
	switch (c2)
	{
	case '/':			//  2 /'s start rest-of-this-line comment
		ppChar = ppScanto("\r\n");		// quickly ppCNdc to \r or \n (or end)
		return ppChar;			// return the \r \n or EOF
		// note \ newline does not get here (ppC removes)
		// nb doDefine() relies on \r \n or EOF return

	case '*':			//  / and * start comment that runs to * and /
		for ( ; ; )		// loop to look at *'s in comment
		{
			ppChar = ppScanto("*");    	// quickly skip to * or end
			if (ppChar==EOF)
				return EOF;			// no msg: truncated comments expected at end extracted pp cmd lines.
			ppChar = ppCNdc();   	// char after an * in comment
			if (ppChar=='/')
				break;			// * and / end comment
			ppUncNdc();			// unget char (may be *) and loop
		}
		return ' ';			// return ' ' for comment, to be sure token such as == or identifier
							//   not allowed to have a comment in the middle

	default:  				// the / did not begin a comment
		ppUncNdc();    		// unget c2 at char level
		return ppChar;   	//	return the /
	}
	// other returns above
}			// ppCDc
//==========================================================================
void FC ppUncDc()	// unget char returned by ppCDc
{
	ppRechar = 1;		// tell ppCDc to return ppChar again
}		// ppUncDc

//==========================================================================
LOCAL SI FC NEAR ppScanto( char *set)

// pass characters at ppCNdc level not in "set", for ppCDc

// returns EOF (end buffer) or the char in set
{
	ppcP += strcspn( ppcP, set);	// quickly pass chars not in "set"
	return ppCNdc();			// return next char
}			// ppScanto
//==========================================================================
SI FC ppCNdc()

// get preprocessor command char, not decommented

// for file names, text in quotes, & internal use in ppCDc

// returns EOF at end of buffer
{
	if (ppRechar)	// if char gotten with ppCDc then ungotten
	{
		/* possible bug, as ppCDc passes comments and there is no provision
		   to back up over the comment so ppCNdc can return comment chars */
		ppWarn( (char *)MH_P0073);		/* "Internal warning: possible bug detected in pp.cpp:ppCNdc(): \n"
						   "    Character gotten with decomment, ungotten, \n"
						   "    then gotten again without decomment. " */
		ppRechar = 0;
		return ppChar;		// if recovery possible, this does it.
	}

	SI c = *ppcP++;  	// get char / advance pointer
	if (c)  		// normally
		return c;	// return char
	ppcP--;		// at end buffer, restore pointer
	ppcEndRead = 1;	// say end buf read (do not ppcP-- in ppUncNdc)
	return EOF;		// return EOF not NULL to avoid problems with strchr
}		// ppCNdc
//==========================================================================
LOCAL void FC NEAR ppUncNdc()	// unget char gotten with ppCNdc
{
	if (ppcEndRead==0)		// no incr at end, so no backup
		if (ppcP > ppcBp)	// insurance
			ppcP--;		// un-increment char fetch pointer
}		// ppUncNdc


/*======================== ERROR MESSAGE INTERFACES ========================*/

//===========================================================================
RC CDEC ppErr( char *message, ...)
// preprocessor error message - most used style
{
	va_list ap;
	va_start( ap, message);
	return ppErv( 255, 1, 1, 0, message, ap);
}					// ppErr
//===========================================================================
RC CDEC ppWarn( char *message, ...)
// preprocessor warning - does not delete execution
{
	va_list ap;
	va_start( ap, message);
	return ppErv( 255, 1, 1, 1, message, ap);
}					// ppWarn
//===========================================================================
RC CDEC ppErn( char *message, ...)
// preprocessor error message with no file line text echo
{
	va_list ap;
	va_start( ap, message);
	return ppErv( 0, 0, 1, 0, message, ap);
}					// ppErn
//===========================================================================
LOCAL RC FC NEAR ppErv(

// preprocessor error message inner function with optional text, caret, file name & line #, and with vprintf formatting

	SI shoTx, 	/* text to show:  0: none;
			   4 bit: cmd line arg (also ppcBp) if decoding one;
			   2 bit: preproc cmd line (ppcBp) if decoding one;
			   1 bit: file line (isf->buf[isf->i]).
			   bit priority: 4 if possible, else 2, else 1. */
	SI shoCaret, 	// show caret at position (only if text shown)
	SI shoFnLn, 	// show file name and line number
	SI isWarn,	// 0 for error (errCount++), 1 warning (display suppressible), 2 info.
	char *fmt, 	// message text or handle (see messages.cpp)
	va_list ap )	// vprintf-style arg list

// rets RCBAD for convenience, even if isWarn.
{
	char cmsg[750], caret[139], where[139], whole[2000], txBuf[256+3];
	USI col;

#define GLT 2	/* number of leading nonwhite (file) chars that constitute
	sufficient text, else also show preceding line.
		> 1, so ) or ; alone don't count;
		< 3, so "go" does NOT show preceding unrelated line. */


		caret[0] = where[0] = '\0';
		const char* tex = "";
		col = 9999;

		if (isf)			// insurance eg re call for bad command line
			lisThruLine(isf->line);		/* echo input to listing thru end of current input FILE line
									(expect start of line is already echoed & proper Lty set; this is to
									complete line & prevent error message inserted in mid-line). pp.cpp. */

		/* get input line text and column position.  Text can be:
		1) cmd line arg,
		2) preprocessor command,
		3) raw file line,
		4) macro being expanded, right? */

		if ( (  (shoTx & 4) &&  ppcIsClarg	  	// if cmd line arg requested
		 || (shoTx & 2) && !ppcIsClarg ) 	// if preproc cmd line req'd
		 && ppcBp != NULL )			// ... and is present
	{
		tex = ppcBp;      		// current preprocessor cmd line, if doing one
		col = (USI)(ppcP - ppcBp);	// column position in that line
		if (col > 0)			// since this pointer is usually AFTER error,
			col--;			// back up a column
	}
	else if (shoTx & 1)			// if file line requested
{
	/* impl 9-27-90, watch for addl corrections needed (seem to recall
	   some notes about things that would fail if file line echoed) */
	// This shows raw (unpreprocessed) file text or curr macro, right?? 12-90 NO NO 'isf' is file, use 'is' for macro!
	if ( inDepth > 0			// if a file open (insurance, 2-91) &
				&& isf->i <= isf->n )		// if isf->i is in range (unsigned)
		{
			// start of file line: after last newline b4 position-1
			char *p = isf->buf + isf->i;	// char that would be fetched next
			if (is->i > 0)		// since is->i usually AFTER error,
				p--;			// back up 1 char (and to prev line from beg line)
			USI glt = 0;			// got-leading-text flag/char count
			while (p > isf->buf  &&  *(p-1) != '\n')	// find line beginning
			{
				p--;
				if (!isspaceW(*p))		// if nonblank
					glt++;			// count (say got) leading text
			}
			// length of file line: to first newline
			USI n = (USI)strcspn( p, "\f\r\n");	// #chars to 1st \n (else all)
			char *q = p;  			// re (last) line, for figuring col
			// and incl up to 2 preceding lines if nec for nonblank text b4 posn
			// this logic also in cutok.cpp:cufCline().
			if (glt < GLT)   			// if don't have (enuf) leading text
			{
				for (USI nlnl = 0;  p > isf->buf;  p--)
				{
					if (*(p-1)=='\n')		// if on 1st char of a line
					{
						if ( glt >= GLT		// if have leading text
								|| nlnl > 2 )      	// or have scanned enuf lines
							break;    		// done
						nlnl++;			// # leading newlines
					}
					else if (!isspaceW(*(p-1)))	// test if nonblank
						glt++;			// # leading nonwhite chars
				}
				while (nlnl && *p=='\n')	// now delete leading blank lines
				{
					nlnl--;
					p++;		//  (well, null lines anyway)
				}
				n += (USI)(q - p);
			}
			// buffer file line
			setToMin( n, sizeof(txBuf)-3);		// truncate at buffer size
			memcpy( txBuf, p, n);			// move to a buffer so can ...
			memcpy( txBuf + n, "\n", 2);  	// ... terminate w \n & null
			tex = txBuf;				// 'tex' used in sprintf below
			// file line column for ^
			for (col = 0;  q < isf->buf + isf->i;  q++)	// scan characters left of position
			{
				if (*q=='\t')
					col |= 7;				// tab: next multiple of 8 (fall thru ++)
				col++;					// count columns
			}
			if (col > 0)				// is->i is usually AFTER err,
				col--;				// so back up a column
		}
	}

// more code after here could be shared with cutok:cuErv?

#if 0	// worked nicely
x// make up 'where': "__ at line __ of file __" text
x    if (shoFnLn)				// if requested
x       if (ppcIsClarg)					// if doing cmd line
x          sprintf( where, "%s in command line argument: ",
x			  isWarn ? "Warning" : "Error" );
x       else if (inDepth > 0 && isf)			// if a file is open
x          sprintf( where, "%s at line %d of file '%s': ",
x			  isWarn ? "Warning" : "Error",
x              	          (INT)isf->line, isf->name );
#else	// try microsoft-like format, 2-91

// make up 'where': "<file>(<line>): Error/Warning: " text

	if (shoFnLn)				// if requested
	if (ppcIsClarg)					// if doing cmd line
		sprintf( where, "Command line: %s: ",
				 isWarn ? "Warning" : "Error" );
	else if (inDepth > 0 && isf)			// if a file is open
		sprintf( where, "%s(%d): %s: ",
				 getFileName(isf->fileIx), (INT)isf->line,
				 isWarn ? "Warning" : "Error" );
#endif


// format caller's msg and args

	msgI( WRN, cmsg, sizeof( cmsg), NULL, fmt, ap);		// messages.cpp: get text if fmt is handle, vsprintf.


// make up line with caret spaced over to error column

	if ( shoCaret  		// if ^ display requested
				&&  col < sizeof(caret)-6	// if ^ position fits buffer
				&&  *tex )  		// skip if not showing input line
	{
		USI lWhere = (USI)strlen(where);
		if (shoFnLn && col >= lWhere + 3)		/* if col is right of end of "where" line,
			           append ^ thereto: save line & avoid ugly gap. */
		{
			if ( !strchr( cmsg, '\n')				// if callers message is all on one line
					&&  strlen(cmsg) + lWhere + 3 			// and it will also fit ...
					<= min(col, USI( getCpl()-8)) )  			//  left of caret with plenty of space in line
			{
				strcat( where, cmsg);				// move caller's message into "where" to be before ^
				cmsg[0] = 0;					// ..
				lWhere = (USI)strlen(where);				// ..
			}
			memset( where + lWhere, ' ', col - lWhere);		// space over
			strcpy( where + col, "^");				// append ^
			if (col < getCpl()-8 -4 -1)   				// if 1-char cmsg could go on same line (see below)
				strcat( where, "    ");				// separate it with spaces; else avoid exceeding cpl.
		}
		else if ( shoFnLn  &&  col < 10 		  	// if col small, insert ^ at left end of "where" line
				  &&  lWhere + col < getCpl() - 20)
		{
			memmove( where + col + 4, where, lWhere+1);
			memset( where, ' ', col + 4);
			where[col] = '^';
		}
		else						// basic case: ^ on separate line
		{
			//if (col > 0) unneeded insurance??
			memset( caret, ' ', col);
			strcpy( caret + col, "^\n");
		}
	}


// assemble complete text

	sprintf( whole,
			 "%s%s"		// line text (or not), and newline if needed (is needed after clarg)
			 "%s"		//     ^ (or not, or with 'where')
			 "%s%s"		// where (or not) and possible newline
			 "%s", 		// caller's message (or with where)
			 tex,    *tex && tex[strlen(tex)-1]=='\n' ? "" : "\n",
			 caret,
			 where,  *cmsg && strJoinLen( where, cmsg) > getCpl()-8  ?  "\n  "  :  "",	//-8 for cols added in input listing
			 cmsg );


// output message to error file, screen, and/or ERR report per rmkerr.cpp; increment error count.

	errI( REG, isWarn, whole);		// lib\rmkerr.cpp

// output message to input listing if in use; attempt to insert after file line.

	if (isf)						// insurance
	{
		int place;
		if ( shoTx & 1						// if file line shown
				&&  *tex						// and nonNull file line found
				&&  lisFind( isf->fileIx, isf->line, tex, &place) )	// and matching place found in listing spool buffer (pp.cpp)
		{
			// reassemble message without file line(s) text
			sprintf( whole,
					 "%s"   		//     ^ (or not, or with 'where')
					 "%s%s"		// where (or not) and possible newline
					 "%s", 		// caller's message (or with where)
					 caret,
					 where,  *cmsg && strJoinLen( where, cmsg) > getCpl()-8  ?  "\n  "  :  "",	//-8 for cols added in listing
					 cmsg );

			// insert message after found file line, with -------------- following and preceding (pp.cpp fcn)
			lisInsertMsg( place, whole, 1, 1);
		}
		else		// no file line in msg or cd not find match (too far back, b4 an include (flushes buffer), etc)

			lisMsg( whole, 1, 1);				// same message as screen to listing if in use, pp.cpp
	}
	return RCBAD;
}					// ppErv

/////////////////////////////////////////////////////////////////////////////////
// CSE preprocessor command parse/execute
/////////////////////////////////////////////////////////////////////////////////

// preprocessor commands
LOCAL RC   FC NEAR ppcDoI(int ppCase, const char* pSrcText);
LOCAL RC   FC NEAR doDefine( int redef);
LOCAL SI   FC NEAR isDefined( char *id, DEFINE **defp);
LOCAL void FC NEAR clearAllDefines();	// clear all entries in defSytb
LOCAL BOO clearOneDefine( SI tokTy, DEFINE *&theDef);	// used as arg to syClear


/*====================== PREPROCESSOR COMMANDS dept =======================*/

/*----------------- preprocessor command DECODING MACROS ------------------*/

#define DEBLC(c)  do c = ppCDc(); while (c==' ' || c=='\t')	// deblank

//==========================================================================
void FC ppCmdClean( 		// init/cleanup routine

   CLEANCASE /*cs*/ )	// [STARTUP], ENTRY, DONE, CRASH, or [CLOSEDOWN] (type in cnglob.h)
{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */

// clean symbol table for #defines
	clearAllDefines();		// clear the #defines symbol table, below.

// clean re #if's
	ifDepth = 1;		// no open #if's
	ifOn = 1;			// compiling input text -- FALSE #if not in effect
	//iff = ..			believe does not matter since data-init to 0 not &ifStk[ifDepth-1]
}		// ppCmdClean

//==========================================================================
RC FC ppcDo( const char* p, int ppCase)		// decode/execute preproc command at p

// returned error code ignored by caller unless RCFATAL
{
	// printf("ppc: '%s'\n", p);	// temp debug aid
	ppctIni( p);  		// init pp char fetch/tokenize
	RC rc = ppcDoI(ppCase, p);	// decode/execute pp cmd, only call, next.
	ppctIni( nullptr);		// say NOT doing a pp command
	return rc;
}			// ppcDo
//==========================================================================
LOCAL RC FC NEAR ppcDoI(	// decode/execute preprocessor command inner.  call ppctIni() first.
	int ppCase,	// which command: PPC__ defines at start of this file.
	const char* pSrcText)	// source text (re #ECHOON / #ECHOOFF)

// ppC() has already looked up command in table; nevertheless, command keyword is included in text received here.

// returned error code other than RCFATAL ignored by caller
{
	SI c, c1, c2, value, noCheckEnd = 0;
	char fnBuf[81], ppcWord[1+32+1], *q;
	RC rc;

// syntax verify beginning of command
	// caller ppC looked first identifier up in table,
	// but did not verify syntax nor issue error message if not found)
	if (ppCDc()!='#')			// pass #
		ppUncDc();				// unless caller didn't transmit it
	if (ppcId(IGN)) 				// scan word to ppIdtx[]
		return ppErr( (char *)MH_P0030); 	// errMsg "Invalid preprocessor command:    word required immediately after '#'"
	strcpy( ppcWord, "#");
	strcpy( ppcWord+1, ppIdtx);		// save word for errMsgs
	if (ppCase == 0)					// if ppC did not find id in table
	{	if (ifOn)
			return ppErr((char*)MH_P0031, ppcWord);	// "unrecognized preprocessor command '%s'"
		// else #if-out unrecognized #cmd, fall thru / ignored below
	}

// decode rest of command and execute it
#define CHECKEND   do  c=ppCDc(); while (isspaceW(c));  if (ppCDc()!=EOF) goto unex
	// relies on repeated ppCDc at end being ok

	switch (ppCase)
	{
		//---- preprocessor commands executed even when conditional compile is OFF

	case PPCELSE:
		if (ifDepth <= 1)				// if not under #if/ifdef
			return ppErr( (char *)MH_P0032 );		// "#else not preceded by #if"
		if (ifDepth < IFDEPTH)			// compile stays off if excess nesting
		{
			if (iff->elSeen)
				return ppErr( (char *)MH_P0033,  	// "More than one #else for same #if (#if is at line %d of file %s)"
					iff->line, getFileName(iff->fileIx) );
			iff->elSeen = 1;				// say 'else' seen for this 'if'
			value = (iff->thisOn | iff->hasBeenOn) == 0;	// goes on if off & has not been on
elifJoins: ;			// set compile on or off per 'value'
			iff->hasBeenOn |= value;		// nz makes all elif's go off
			iff->thisOn = value;		// whether on at this level
			ifOn = 		    		// set global compile-on flag for ppC()
			iff->allOn = 	    		// set whether on at this + enclosing lvls
			(iff-1)->allOn 		// if on at enclosing levels
			&& value;			// and on at this level
			if (ifOn==0)			// if compile off, say send #line
				isf->lineSent = 0;		// ppC sends next time compile is on.
		}
		break;				// checks for end #cmd line

	case PPCENDIF:
		if (ifDepth <= 1)  			// note depth 1 when no #if's
			return ppErr( (char *)MH_P0034 );	// "#endif not preceded by #if"
		// close out a #if level
		ifDepth--;  				// pop a level
		// use next outer level
		if (ifDepth < IFDEPTH)		// compile stays off if excess nesting
		{
			iff = ifStk + ifDepth - 1;
			ifOn = iff->allOn;
			// isf->lineSent is already 0 if compile has been off.
		}
		break;				// checks for end line

	case PPCIFNDEF:
		CSE_E( ppcId( WRN) )				// decode identifier to ppIdtx, msg & ret bad now if err
		value = (RCOK != syLu( &defSytb, ppIdtx, 0, NULL, NULL));	// look up id, RCOK if found, sytb.cpp
		goto ifsJoin;					// go add ifStk level, using 'value'

	case PPCIFDEF:
		CSE_E( ppcId( WRN) )				// decode identifier to ppIdtx, msg & ret bad now if err
		value = (RCOK==syLu( &defSytb, ppIdtx, 0, NULL, NULL));	// look up id, RCOK if found, sytb.cpp
ifsJoin:
		;
		ifDepth++;				// count #if nesting level
		if (ifDepth >= IFDEPTH)
		{
			ppErr( (char *)MH_P0035);		// "Too many nested #if's, skipping to #endif"
			ifOn = 0;				// program continues with compilation off till nest count not excessive
			isf->lineSent = 0;			// say send #line next time compile is on
		}
		else					// nesting not excessive
		{
			iff = ifStk + ifDepth - 1;		// next cond compile info stack level
			iff->fileIx = isf->fileIx;		// input file name index 2-94
			iff->line = isf->line;		// ... line number for errMsgs
			iff->elSeen = 0;
			iff->thisOn = iff->hasBeenOn = value;
			ifOn = iff->allOn = (value && (iff-1)->allOn);
			if (ifOn==0)			// if compile off, say send #line
				isf->lineSent = 0;		// ppC sends next time compile is on.
		}
		break;				// checks end unless noCheckEnd

	case PPCIF:
		if (ifOn==0)				// if enclosed in false #if
		{	value = 0;				// for speed, don't do expression
			noCheckEnd++;			// hence can't check end line
		}
		else
		{	CSE_E( ppCex( &value, ppcWord) )	// evaluate expr
		}
		goto ifsJoin;		// join ifdef to add ifStk level

		/*lint -e616 */
	case PPCELIF:
		if (ifDepth <= 1)			// if not under #if/ifdef
			return ppErr( (char *)MH_P0036); 	// "#elif not preceded by #if"
		if (ifDepth >= IFDEPTH)		// compile stays off if excess nesting
		{
			noCheckEnd++;  			// suppress end check: expr not parsed
			break;
		}
		if (iff->elSeen)
			return ppErr( (char *)MH_P0037,		// "#elif after #else (for #if at line %d of file %s )"
					iff->line, getFileName(iff->fileIx) );
		if ( ifOn				// if compile on, it goes off
		 || iff->hasBeenOn   		// if has been on, stays off
		 || (iff-1)->allOn==0)		// if outer if is off, does not matter
		{
			// skip parsing expr for speed
			value = 0;				// set compile off
			noCheckEnd++;			// cannot check for end of line
		}
		else				// conditionally turn compile on
		{
			CSE_E( ppCex( &value, ppcWord) )	// evaluate expr
		}
		goto elifJoins;	// join #else case. uses 'value'.

		// return ppErr( "Unimplemented preprocessor command '%s'", ppcWord);

	default:
		if (ifOn == 0)		// if conditional compilation (#ifs) is off
			noCheckEnd++;		// no check for end of command as not parsed
		else			// conditional compilation ON
		{
			switch (ppCase)
			{

				//------ preprocessor commands executed only when compile is ON -----

			case PPCINCLUDE:
				DEBLC(c1);				// char b4 filename: < or "
				if (strchr( "<\"", c1)==NULL)
					return ppErr( (char *)MH_P0038);	// "'<' or '\"' expected"
				c2 = (c1 == '<') ? '>' : c1;
				for (q = fnBuf; ; )     		// scan/copy file name
				{
					c = ppCNdc();				// get char NOT DECOMMENTED
					if (c==c2)					// if expected terminator
						break;
					if (c==EOF)					// if end of input
						return ppErr( (char *)MH_P0045, c2);	// "Closing '%c' not found"
					if (q < fnBuf + sizeof(fnBuf)-1)		// truncate at bufSize
						*q++ = (char)c;				// copy name so can terminate
				}
				*q = 0;
				CHECKEND;			// now: after open, errmsg wd have wrong file
				// execute #include
				// filename syntax check worth the bother?
				if (ppOpI( fnBuf, ".inp") )  // open incl file, pp.cpp
					return RCBAD;			// if file not found or out of memory
				break;

			case PPCUNDEF:
				CSE_E( ppcId( WRN) )	// decode identifier to ppIdtx, msg & ret bad now if err
				ppDelDefine();   	// undefine ppIdtx, local, below
				break;				// go check for garbage after name

			case PPCDEFINE:
				CSE_E( doDefine(0))		// do #define (local fcn below), ret if error, RCFATAL if out of memory
				break;				// go verify whole command used (insurance)

			case PPCREDEFINE:
				CSE_E( doDefine(1))		// do #redefine (local fcn below), ret if err, RCFATAL if out of memory
				break;			// go verify whole command used (insurance)

			case PPCCLEAR:
				clearAllDefines();	// delete ALL defines. fcn below. Added 11-1-95.
				break;

			case PPCCEX:		// eval/print const expr at compile time:
				// development aid
				CSE_E( ppCex( &value, ppcWord) )		// eval const expr
				printf( " %d ", (INT)value);   		// display value now
				break;

			case PPCSAY:		// display rest of line at compile time:
				// development aid
				do c = ppCDc();
				while (isspaceW(c));	// deblank start
				for ( ; ; )				// loop over chars of line
				{
					if (c=='\\')
					{
						c = ppCDc();
						switch (tolower(c))
						{
						case 'n':
							c = '\n';
							break;	// \n prints newline
						default:
							printf("\\");	// \other
						}
					}
					if (c==EOF)
						break;
					printf( "%c", (char)c);
					c = ppCDc();				// get char of pp cmd, decommented
					if (c=='\n')				// omit (final) newline
						break;
				}
				break;

			// #echooff / #echoon: input echo control
			//   implemented as off-level counter to support nesting
			case PPCECHOOFF:
				lisBufEchoOnOff(false, pSrcText);
				break;
			case PPCECHOON:
				lisBufEchoOnOff(true, pSrcText);
				break;

			default:
				return ppErr( (char *)MH_P0039);	// "Internal error in ppcDoI: bad 'ppCase'"
			}
		}	// inner switch (ppCase)
		/*lint +e616*/
	}		// outer switch (ppCase)

	if (noCheckEnd==0)			// certain cases that skip args set this
	{
		// check that entire line of text was used
		do
			c = ppCDc();				// get char, passing comments
		while (isspaceW(c));			// pass whitespace
		if (c != EOF)				// expect end of line
unex:     // CHECKEND macro comes here
			return ppErr((char *)MH_P0040);  	// "Unexpected stuff after preprocessor command"
	}
	return RCOK;

	// additional (error) returns above, incl in CSE_E macros
#undef CHECKEND
}						// ppcDoI
//==========================================================================
LOCAL RC FC NEAR doDefine( int redef)		// decode/execute rest of #define/#redefine command
// returns RCFATAL if out of memory for symbol table
{
const int MAXVAL = 16384;	// max #define value size.
							//   256-->1024 after complaint, 3-92.
							//   1024-->16384 5-12
	char *id;				// name being (re)defined
	SI nA;					// # arguments, 0 if none
	char *argId[100];			// temp pointers to arg names
	char theVal[MAXVAL+3];			// value (rest of line)
	SI c, n, inQuotes, afBs;
	RC rc;

// get/check id to be #defined

	CSE_E( ppcId( WRN) )			// decode identifier to ppIdtx[], return bad if error
	if (redef)				// if is #redefine cmd
		ppDelDefine();			// delete ppIdtx[] if defined, below
	// else: addDefine checks for redefinitions.
	id = strsave( ppIdtx);		// save in dm now (ppIdtx will be overwritten)

// if ( follows immediately, there are arguments
	// nb if there is space then (, the ( begins body and macro has no args.

	nA = 0;				// init # args
	if (ppCDc() != '(')			// if very next char not a '('
		// this isn't quite right if comment here changed to space
		ppUncDc();			// save char for macro body

// get list of argument names
	else
	{
		for ( ; ; )
		{
			ppcId( WRN);		// deblank, get identifier text to ppIdtx[]
			argId[nA++] = strsave(ppIdtx);	// arg name to dm
			if (nA >= 100)			// insurance
				break;			// too many args: not worth a msg?
			DEBLC(c);			// deblank, c = next char
			if (c==',')			// comma separates args
				continue;			// get next arg
			if (c==')')			// ) terminates arg list
				break;				// done args
			return ppErr( (char *)MH_P0041);	// "Comma or ')' expected"
		}
	}

// deblank before value
	DEBLC(c);				// c = next nonblank noncomment char
	ppUncDc(); 				// unget c

// get value (text): rest of line

	inQuotes = afBs = 0;
	theVal[0] = ' ';		// initial space so no unwanted token-pasting (coord any changes w pp.cpp:ppClDefine()!)
	n = 1;			// store subscript.  space uses [0].
	for ( ; ; )			// scan chars into theVal to end of line
	{
		// next character, conditionally decommented, stop at end line
		c = (inQuotes || afBs)	// in "" or after \, keep comments.
			?  ppCNdc()	// next char, not decommented
			:  ppCDc();	// ditto, decommented.
		// ?? single quotes: decomment not needed but probably ok
		if (c==EOF)		// stop if end of # command input buffer
			break;
		if (strchr("\r\n",c))	// stop at end line
			break;		// caller checks for end buffer: insurance
		// note \-newline removed b4 here (by ppC)
		afBs = (c=='\\');	// update after-backslash flag

		// start or end quoted text (re whether to decomment above)
		if (c=='"')			// a quote ...
			if (inQuotes==0)
				inQuotes++;		// always opens "text"
			else if (afBs==0)
				inQuotes = 0;		// ends "text" only if not after \

		// store char, count length.  Unstored \ is overwritten next iteration.
		if (n <= MAXVAL)				// unless too long
			theVal[n] = (char)c;			// store char
		else if (n==MAXVAL)			// msg once per define only
			ppErn( (char *)MH_P0042);		// "Overlong #define truncated"
		n++;
	}			// for ( ; ; ): value loop
	if (inQuotes)			// newline even in quotes terminates define
		ppWarn( (char *)MH_P0043);   		// "Unbalanced quotes in #define"

	/* and continue, later getting whatever error "text  causes??
	   what does c compiler do in such a case? no msg at define? */

// terminate / save value

	/* note: be sure no \r or \n can be stored in the definition,
	   as would mess up caller's line count in cutok.cpp. */
	while (n > 0  &&  isspaceW(theVal[n-1]))	// deblank end
		n--;
	theVal[n++] = ' ';			// end with space to avoid pasting
	theVal[n/*++*/] = '\0';

// enter in symbol table
	return addDefine( id, strsave(theVal), nA, argId);	// local, below, returns RCFATAL if out of memory
}		// doDefine
//==========================================================================
RC FC ppDelDefine() 	// undefine identifier in ppIdtx[]
// if undefined, returns bad (non-0), no message
{
	DEFINE *theDef;
	RC rc;

// delete symbol table entry, returning ptr to block
	CSE_E( syDel( &defSytb, 0/*tokTy*/, 1/*casi*/, 0/*nearId*/, ppIdtx, 0/*undef ok*/, (void **)&theDef) )	// sytb.cpp. Only call 11-94.

// free dynamic memory the DEFINE points to, then DEFINE block itself
	return clearOneDefine( 0, theDef);  			// just below
}					// ppDelDefine
//==========================================================================
LOCAL BOO clearOneDefine( 	// clean and free one DEFINE symbol table entry
	[[maybe_unused]] SI tokTy,		// token type, not used here
	DEFINE *&theDef )	// ref to pointer to DEFINE structure as used in defSytb entries
// returns TRUE to say symbol table entry should be deleted.
{
	if (theDef)					// insurance
	{
		dmfree( DMPP( theDef->id));
		dmfree( DMPP( theDef->text));
		for (SI i = 0; i < theDef->nA; i++)
			dmfree( DMPP( theDef->argId[i]));
		// note argId not in dm: rather, DEFINE is variable size.
		dmfree( DMPP( theDef));			// this NULL's caller's pointer via reference
	}
	return TRUE;		// TRUE tells syClear that we DID delete this entry.
}		// clearOneDefine
//==========================================================================
RC FC addDefine( 	// enter definition in preprocessor symbol table
	char *id, 		// identifier (name of macro)
	char *text, 	// value (text of macro)
	SI nA, 			// # arguments.  If 0, NULL argId ok.
	char *argId[] )	// ptr to array of argument id pointers
// all char *'s must already be in non-volatile storage: dm (or static).
// returns RCFATAL if out of memory for symbol table
{
	DEFINE *defp;
	int	i;
	RC rc=RCOK;

// check if defined
	if (isDefined( id, &defp) )
	{
		// error message unless definition identical
		// oops! even requires same argument names 10-90.
		if ( nA != defp->nA
		|| strcmpi(id,defp->id) 			// case-insensitive tentative 11-94 (also pp.cpp, cuparse.cpp, etc)
		|| strcmp(text, defp->text) )
			rc = ppErr((char *)MH_P0046, ppIdtx);   	// "Redefinition of '%s'"

		// delete dm items of rejected definition
		for (i = 0; i < nA; i++)
			dmfree( DMPP( argId[i]));
		dmfree( DMPP( id));
		dmfree( DMPP( text));
		return rc;		// RCOK if duplicate matching definition
	}

// not defined.  Make up definition block.
	CSE_E( dmal( DMPP( defp),				// alloc sym tab blk in dm
		sizeof(*defp) + nA * sizeof(defp->argId),
		WRN | DMZERO ) )
	defp->id = id;			// put definition in block
	defp->text = text;			// ...
	defp->nA = nA;			// ...
	for (i = 0; i < nA; i++)		// ... copy in argName ptrs
		defp->argId[i] = argId[i];
	defSytb.isSorted = 1;		// maintain table sorted for search speed (need only be done once)
// enter in symbol table
#ifdef LOOKING	// debug aid display, used 7-92
*    printf("before adding '%s':\n", id);
*	 dumpDefines();
*    rc = syAdd( &defSytb, 0, 0, defp, 0, 0);			// add to symbol table, sytb.cpp.  rets RCFATAL if out of memory
*    printf("after adding '%s':\n", id);
*	 dumpDefines();
*    return rc;							// more returns above, incl CSE_E macros
#elif 0
x    return syAdd( &defSytb, 0, 0, defp, 0, 0); 	// add to symbol table, sytb.cpp.  returns RCFATAL if out of memory.
x							// only add to this symbol table, 10-93.
#else//11-94 see also ppDelDefine()
	return syAdd( &defSytb, 0, TRUE, defp, 0);	/* add to symbol table, sytb.cpp.  returns RCFATAL if out of memory.
							   Made case-insensitive 10-4-94 -- tentatively making CSE case-insens
							   everywhere -- also FUNCTION defintion syAdd in cuparse.cpp.
							   Only add to this symbol table, 10-93. */
#endif
	// more returns above, incl CSE_E macros
}					// addDefine
//==========================================================================
LOCAL void FC NEAR clearAllDefines()	// clear all entries in defSytb, 11-1-95.

// callers 11-95: ppCmdClean, ppcDoI PPCCLEAR case (#clear)
{
	syClear( &defSytb, 				// clear a symbol table, a general purpose function, sytb.cpp.
			 -1,					// all entries regardless of tokTy
			 (BOO (*)(SI,STBK *&))clearOneDefine );	// fcn to call for each entry. Cast DEFINE *& arg to void *&.
	// clearOneDefine, above, frees dm pointed to by our DEFINE entry struct

	// note 11-95: definitions in use are now dmIncRef'd, so they should not be free'd until expansion is complete.

	//defSytb.isSorted = 0;				not needed: this symbol table is maintained sorted from the start.
}				// clearAllDefines
//==========================================================================
LOCAL SI FC NEAR isDefined( 	// non-0 if id is already #defined
	char *id,
	DEFINE **defp )	// NULL or receives ptr to symbol table block
{
	return
		RCOK==syLu( &defSytb, id, 0, NULL, (void **)defp);	// look it up, sytb.cpp
}		// isDefined

//==========================================================================
void FC msgOpenIfs()	// issue diagnostic messages for any unclosed #if's
{
	for (SI i = ifDepth; --i >= 1; )
		ppErn( (char *)MH_P0044,  		// "'#endif' corresponding to #if at line %d of file %s not found"
				ifStk[i].line, getFileName(ifStk[i].fileIx) );
}	// msgOpenIfs

#if 0	// development aid
t //==========================================================================
t void FC dumpDefines()	// display all defines
t{
t	for (STAE *pp = defSytb.p;  pp < defSytb.p + defSytb.n;  pp++)
t	{	DEFINE *p = (DEFINE *)pp->stbk;
t		printf( "  %d %16s", (INT)pp->iTokTy, p->id);
t		if (p->nA)
t		{	printf("(");
t	        for (SI i = 0; i < p->nA; i++)
t				printf( "%s%s", p->argId[i], (i < p->nA - 1) ?  "," : ")" );
t		}
t		printf( "%s\n", p->text);
t	}
t}		// dumpDefines
#endif


/////////////////////////////////////////////////////////////////////////////////
// CSE preprocessor constant-expression parser-evaluator
/////////////////////////////////////////////////////////////////////////////////


/* Evaluates constant expressions,
      as used in #if commands in C-style preprocessor for CSE user language.
   On arrival here, text has already been macro-expanded;
   fcn ppCex() here returns integer value for given text.
   Input text is obtained by calling ppTok() etc */

/*-------------------------------- DEFINES --------------------------------*/
//-- ceval() data types --
#define TYSI    0x01		// integer
#define TYNONE  0x200		// no value (start of (sub)expression)

/*---------------------------- LOCAL VARIABLES ----------------------------*/

//--- CURRENT TOKEN INFO.  Set mainly by ppToke().  Not changed by ppUnToke().
LOCAL SI NEAR ppTokety = 0;  	// cur token type (CUT__ def; ppToke ret val)
LOCAL SI NEAR ppPrec = 0;    	// "prec" (precedence) from opTbl[]. PR__ def.
LOCAL SI NEAR ppLsPrec = 0;	// ppPrec of PRIOR token (0 at bof).
LOCAL OPTBL * NEAR ppOpp = NULL;	// ptr to opTbl entry for token

/*--- PARSE STACK: one entry ("frame") for each subexpression being processed.
 When argument subexpressions to an operator have been completely parsed and
 evaluated, operation is performed and their frames are merged.
 ppParSp points to top entry, containing current values. */
struct PPPARSTK
{
	SI ty;			// data type: TYNONE (nothing parsed) or TYSI
	SI value;			// integer value of constant expression
};
LOCAL PPPARSTK NEAR ppParStk[20] = { { 0 } };   	// parse stack buffer
LOCAL PPPARSTK NEAR * NEAR ppParSp = ppParStk-1;  	// parse stack stack pointer

/*--- local TOKEN-UNGOTTEN FLAG (ppUnToke()/ppToke()) */
LOCAL SI NEAR ppReToke = 0;		// nz to re-return same token again

/*--- evaluator debug aid display flag (untested) */
LOCAL SI NEAR ppTrace = 0;		// nz for many printf's during ceval


/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL RC FC NEAR cex( SI *pValue, char *tx);
LOCAL RC FC NEAR ceval( SI toprec, char *tx);
LOCAL RC NEAR ppUnOp( SI toprec, PSOP opSi, char *tx);
LOCAL RC FC NEAR ppBiOp( SI toprec, PSOP opSi, char *tx);
LOCAL RC FC NEAR ppNewSf( void);
LOCAL RC FC NEAR ppPopSf( void);
LOCAL SI FC NEAR ppToke( void);
LOCAL void FC NEAR ppUnToke( void);

//===========================================================================
void FC ppCexClean( 		// init/cleanup routine

	[[maybe_unused]] CLEANCASE cs )	// ENTRY, DONE, or CRASH (type in cnglob.h)

// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
{
	//ppParSp = ppParStk-1;	done at start of cex()
	//ppReToke = 0;		done at start of ppCex() (only public entry)
}		// ppCexClean
//===========================================================================
RC FC ppCex( SI *pValue, char *tx)    	// evaluate preprocessor constant expression, to end of line

// tx: #word, etc, preceding expression: for error messages
{
	*pValue = 0;		// preset to 0 in case caller uses anyway after error return
	ppReToke = 0;		// say no ungotten token
	return cex( pValue, tx);	// parse expression and get its value, next
}		// ppCex
//===========================================================================
LOCAL RC FC NEAR cex( SI *pValue, char *tx)   	// evaluate preprocessor const expr to end line inner fcn
{
	RC rc;

	ppParSp = ppParStk-1;		// init evaluation stack
	CSE_E( ceval( PREOF, tx) )		// evaluate expression
	if (ppParSp != ppParStk + 0)	// check ppParStk level: devel aid
		return ppErr( (char *)MH_P0050, 	// "Internal error:\n    parse stack level %d not 1 after constant expression"
					  INT((ppParSp-ppParStk) + 1) );
	*pValue = ppParSp->value;		// return result
	return RCOK;
	// other returns above (CSE_E macros)
}		// cex
//==========================================================================
LOCAL RC FC NEAR ceval( SI toprec, char *tx)     	// interpret (parse/execute) constant expression inner recursive fcn

// toprec: precedence to parse to: terminator, operator, etc.

// tx:    text of operator, function, or verb, for error messages

/* interface for ceval:
   on good return: terminating token ungotten but ppPrec & ppTokety set for it.
   		   ppParSp ++'d, ppParStk frame has info on expression. */
{
#define NOVALUECHECK  if (ppLsPrec >= PROP)  return ppErr( (char *)MH_P0051, ppToktx )
	// "Syntax error: probably operator missing before '%s'"
#define EXPECT(ty,str)  if (ppToke() != (ty))  return ppErr( (char *)MH_P0052, (str) );	// "'%s' expected"
	RC rc;

	printif( ppTrace," ceval(%d) ", (INT)toprec );	// cueval.c

	ppPrec = 0;		// nothing yet: prevent "operator missing before" message if expression starts with operand
	CSE_E( ppNewSf()) 	// start new parse stack frame for this expression

// loop over tokens of sufficiently high precedence
	for ( ; ; )
	{

		// next token and info
		ppToke();   	// local fcn, calls pptok.c, sets ppPrec, ppOpp, ppTokety, ppToktx

		// done if token's precedence <= "toprec" arg
		if (ppPrec <= toprec)				// if < this ceval() call's goal
		{
			printif( ppTrace," exprDone %d ", (INT)ppPrec);
			break;					// stop b4 this token
		}

		// checks, arguments, code generation: cases for each token type
		switch (ppOpp->cs)
		{
			OPTBL *svOpp;

			/* every case must update as needed: ppParSp->ty,
							     and ppPrec = PROP if not always covered. */

			// unary integer operator.  ppPrec-1 used for toprec to do r to l.
		case CSUNN:  // "unary numeric" is integer only in preprocessor
		case CSUNI:
			CSE_E( ppUnOp( ppPrec-1, PSOP( ppOpp->v1), ppOpp->tx))
			break;

			// binary integer operators  |  &  ^  >>  <<  %
		case CSBIN:  // "binary numeric" is same here
		case CSBII:
			CSE_E( ppBiOp( ppPrec, ppOpp->v1, ppOpp->tx))
			break;

			// comparisons: binary numeric ops with integer result
		case CSCMP:
			CSE_E( ppBiOp( ppPrec, ppOpp->v1, ppOpp->tx))
			//ppParSp->ty = TYSI;  in pp, already set from arg.
			break;

			// grouping operators: (, [
		case CSGRP:
			NOVALUECHECK;
			svOpp = ppOpp;			// save ppOpp thru ppUnOp
			CSE_E( ppUnOp( PRRGR, 0, ppOpp->tx) )
			{
				char ss[2];   			// { for scope for C
				ss[0] = (char)svOpp->v2;	// char to string for msg
				ss[1] = '\0';
				EXPECT( svOpp->v1, ss);
			}
			ppPrec = PROP;		/* say have a value last: (expr) is a value even tho ')' is not:
			      			   effects interpretation of unary/binary ops and syntax checks */
			break;

			/*lint -e616  falls into case */
			// cuparse-only. specific msg?
		case CSLEO:
notInPp:
			// unexpected: improper use or token with no valid use yet
			//csu:
		case CSU:    // believed usually impossible for terminators due to low ppPrec's
			return ppErr( (char *)MH_P0053, ppToktx);  			// "Unexpected '%s'"

		default:
			return ppErr( (char *)MH_P0054,	     // "Unrecognized opTbl .cs %d for token='%s' ppPrec=%d, ppTokety=%d."
			(INT)ppOpp->cs, ppToktx, (INT)ppPrec, (INT)ppTokety );

			// additional cases by token type:
		case CSCUT:
			switch (ppTokety)
			{
			case CUTQUOT:	// quoted text, ?? text in ppToktx
			case CUTID: 	// identifier not yet in symbol table
			case CUTCOM:  	// comma
				goto notInPp;	// above

			default:
				return ppErr( (char *)MH_P0055,			// "Unrecognized ppTokety %d for token='%s' ppPrec=%d."
				(INT)ppTokety, ppToktx, (INT)ppPrec );

			case CUTSI: 	// integer constant, value in ppSIval
				NOVALUECHECK;
				ppParSp->value = ppSival;		// value to stack frame
				ppParSp->ty = TYSI;   		// have value, type integer
				// ppPrec >= PROP already true
				break;

			case CUTQM:		// C "?:" conditional expression operator
				// condition-expr precedes '?'
				if ( ppParSp < ppParStk
				|| ppParSp->ty != TYSI )				// if no preceding value
					return ppErr( (char *)MH_P0056, ppOpp->tx );	// "Preceding constant integer value expected before %s"
				// then-expr.  NB prec of ':' is prec of '?' - 1.
				CSE_E( ceval( ppPrec-1, ppOpp->tx) )	// get a value, new ppParStk frame.
				EXPECT( CUTCLN, ":")		// err if : not next
				// else-expression
				CSE_E( ceval( ppPrec, ppOpp->tx) )	// get value, another frame
				// resulting value: replace cond with then- or else-value
				(ppParSp-2)->value =
				(ppParSp-2)->value ? (ppParSp-1)->value : ppParSp->value;
				CSE_E(ppPopSf())  CSE_E(ppPopSf())		// discard the 2 frames
				//ppParSp->ty = TYSI;		// already true
				//ppPrec = PROP;			// needed if can be < PROP
				break;

			case CUTDEF:	// defined(symbol): preprocessor only
				NOVALUECHECK;
				EXPECT( CUTLPR, "(")
				// note there is specific special logic in pp.c:ppC() to suppress macro expansion in following identifier:
				EXPECT( CUTID, "identifier")	// name to ppIdtx
				EXPECT( CUTRPR, ")")
				ppParSp->value =
				(RCOK==syLu( &defSytb, ppIdtx, 0, NULL, NULL));   	// look up id, RCOK if found, sytb.c
				ppParSp->ty = TYSI;   		// have value, type integer
				ppPrec = PROP;			// restore CUTDEF precedence
				break;

				/*lint +e616 */
			}	// switch (ppTokety)
		}	// switch (case)

		if (ppParSp < ppParStk)
			return ppErr( (char *)MH_P0057);		// "Internal error: parse stack underflow"

	}       // for ( ; ; )

// check that something was parsed
	if (ppParSp->ty==TYNONE)			// indicates no operand done
		return ppErr( (char *)MH_P0058, tx);	// "Value expected after %s"

// unget terminating token and return
	ppUnToke();					// unget token, local, only call
	printif( ppTrace," exprOk ");
	return RCOK;				// added ppParStk entry returns .value
	// other returns above (including CSE_E macros)
}			// ceval
//==========================================================================
LOCAL RC NEAR ppUnOp( 	// parse arg to unary operator and execute

	SI toprec, 	// ppPrec to parse to (comma, semi, right paren, etc)
	PSOP opSi, 	// PSNUL or pseudo-code to execute
	char *tx )	// text of operator, for error messages
{
	SI v;
	RC rc;

	// why is it that NOVALUECHECK not needed here? rob 9-30-90.
	CSE_E( ppPopSf() )		// discard empty stack frame of calling ceval as next ceval will make another. right? rob 9-90.
	CSE_E( ceval( toprec, tx) )  		// get expression to appropriate prec
	// always returns ppPrec >= PROP, right?
	v = ppParSp->value;			// fetch operand to shorten cases
	/* 9-26-92, BC 3.1: observed this switch badly compiled: used same register (bx)
	   for pointer to table and case value being compared. Trying removing fastcall. */
	switch (opSi)			// do operation
	{
	case PSINOT:
		v = !v;
		break;		// logical not
	case PSIONC:
		v = (SI)~(USI)v;
		break;	// one's complement
	case PSINEG:
		v = -v;
		break;		// unary - (negate)
	case PSNUL:
		break;			// do nothing (unary +)
	default:
		ppErr( (char *)MH_P0059, (INT)opSi );	// "ppUnOp() internal error: unrecognized 'opSi' value %d"
	}
	ppParSp->value = v;				// put result back
	ppParSp->ty = TYSI;				// have integer value
	return RCOK;
	// other returns above (CSE_E macros)
}			// ppUnOp
//==========================================================================
LOCAL RC FC NEAR ppBiOp( 	// parse 2nd arg to binary operator and execute

	SI toprec, 	// ppPrec to parse to (comma, semi, right paren, etc)
	PSOP opSi, 	// PSNUL or pseudo-code to execute
	char *tx )	// text of operator, for error messages
{
	SI u, v;
	RC rc;

	if ( ppParSp < ppParStk
	|| ppParSp->ty != TYSI )			// if preceding value wrong type
		return ppErr( (char *)MH_P0060, tx );	// "Preceding constant integer value expected before %s"
	CSE_E( ceval( toprec, tx) )	  		// get following expression (v)
	u = (ppParSp-1)->value;  		// fetch operands to shorten cases
	v = ppParSp->value;			// ..
	switch (opSi)			// do operation
	{
	case PSIBAN:
		u &= v;
		break;
	case PSIBOR:
		u |= v;
		break;
	case PSIXOR:
		u ^= v;
		break;
	case PSIRSH:
		u = (SI)((USI)u >> v);
		break;
	case PSILSH:
		u <<= v;
		break;
	case PSIADD:
		u += v;
		break;
	case PSISUB:
		u -= v;
		break;
	case PSIMUL:
		u *= v;
		break;
	case PSIDIV:
		u /= v;
		break;
	case PSIMOD:
		u %= v;
		break;
	case PSILT:
		u = (u < v);
		break;
	case PSIGT:
		u = (u > v);
		break;
	case PSILE:
		u = (u <= v);
		break;
	case PSIGE:
		u = (u >= v);
		break;
	case PSIEQ:
		u = (u == v);
		break;
	case PSINE:
		u = (u != v);
		break;
	case 0:
		break;
	default:
		ppErr( (char *)MH_P0061, (INT)opSi );    	// "ppBiOp() internal error: unrecognized 'opSi' value %d"
	}
	CSE_E( ppPopSf() )		// pop 2nd ppParStk frame (discard v)
	ppParSp->value = u;		// store result in 1st frame
	return RCOK;
	// other returns above (CSE_E macros)
}			// ppBiOp
//===========================================================================
LOCAL RC FC NEAR ppNewSf()		// start new parse stack frame: call at start (sub)expression
{
// overflow check
	if ((char *)ppParSp >= (char *)ppParStk + sizeof(ppParStk) - sizeof(PPPARSTK) )
		return ppErr( (char *)MH_P0062);   			/* "Preprocessor parse stack overflow error: \n"
       								   "    preprocessor constant expression probably too complex" */
// allocate and init new frame
	ppParSp++;  			// point next frame
	ppParSp->value = static_cast<SI>(0x8000);	// poss representation of "no value"

	ppParSp->ty = TYNONE;  		// data type: no value yet
	return RCOK;
}		// ppNewS
//==========================================================================
LOCAL RC FC NEAR ppPopSf()		// discard top ppParStk frame

// Caller has already combined .value with previous frame, using appropriate operator, or otherwise finished use of frame.
{
	// could check for underflow
	ppParSp--;    			/* pop -- delete 2nd entry */
	return RCOK;
}		// ppPopSf

//==========================================================================
LOCAL SI FC NEAR ppToke()

/* local token-getter -- pptok.c:ppTok + unary/binary resolution + ... */

/* sets: ppTokety: token type: CUTxxx defines, cutok.h.
       ppPrec: precedence, comments at start file and/or cuparsei.h.
        ppOpp: pointer to entry in opTbl at start of this file.
      ppToktx: text of any token (for errMsgs)
       ppIdtx: text of identifier token, unchanged if not id */
{
	static SI precSave = 0;

	if (ppReToke)		// on token-ungotten flag, return same token again
	{
		// with this mechanism here, don't need cuUntok in cutok.c.
		ppReToke = 0;    		// clear flag
		printif( ppTrace," re ");	// debug aid print
		ppPrec = precSave;		// restore ppPrec -- because callers change it, e.g. at entry to expr()
		return ppTokety;			// all other info is still correct for token
		// >>> is it really ok to not reconsider unary-binary ambiguity? (to do so, be sure ppLsPrec is right).
	}

// get token (symbol) at pptok.c level, and set our file-global type
	ppTokety = ppTok();  			// pptok.c. only call.

// identifiers: those accepted here have own token types
	if (ppTokety==CUTID)					// if an identifier
	{
		if (strcmpi( ppIdtx, "defined")==0)		// tentatively case-insens 11-94, also pp.cpp
			ppTokety = CUTDEF;
		// else: other identifiers get error message in ceval() except when used as argument to "defined"
	}

// resolve unary/binary operator cases such as "-" by context
	ppLsPrec = ppPrec;		/* prior ppPrec to global [for unToke to restore (so following works right after unToke()),
    				   and] so caller can test context. */
	if (ppPrec >= PROP)   	// if a value precedes this token
	{
		switch (ppTokety)
		{
			// change ops that can be either unary or binary to binary (ppTok returns unary)
		case CUTPLS:
			ppTokety = CUTBPLS;
			break;
		case CUTMIN:
			ppTokety = CUTBMIN;
			break;
		default:
			;
		}
	}

// set global operator table entry pointer, fetch 'ppPrec'.
	ppOpp = opTbl + ppTokety;	// for access to .ppPrec (here), and .cs, .v1 etc by caller
	ppPrec = ppOpp->prec;   	/* get token's "ppPrec" from operator table:
    				   indicates operator precedence (operANDS have hi values >= PROP) and classifies. */
	precSave = ppPrec;		// save private copy for use on re-toke

// debug aid print
	printif( ppTrace, "\n toke=%2d ppPrec=%2d '%s' ", ppTokety, ppPrec, (char *)ppToktx );
	return ppTokety;
	// another return above
}				// ppToke()
//==========================================================================
LOCAL void FC NEAR ppUnToke()		// "unget" last gotten token

// restores prior 'ppPrec' but not prior ppTokety, ppOpp.
{
	printif( ppTrace," un ");
	ppReToke = 1;			// flag tells ppToke() to return same stuff again
}			// ppUnToke


// end of pp.cpp
