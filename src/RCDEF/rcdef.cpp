// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// RCDEF -- Record definer program.  Builds structure definitions,
//   include files, etc. required for record manipulation.

#include "cnglob.h"     // CSE global defines. ASSERT.

/*
 *  Much cleanup remains!
 *
 *  As of 9-18-91  M A N Y   O B S O L E T E   C O M M E N T S !!!
 *
 *	with working comments (??, ???) while i was figuring it out, rob 9-88
 *
 *	newly added 12-89, comments incompletely updated:
 *		standardized record definition syntax, *oride eliminated
 *
 *	newly added 9-88, comments incompletely updated:
 *		data type handles in input
 *		choice handles and Chantab
 *		record type handles in input
 * /

   Rcdef can be used to generate .H files only (make arg 7 NUL) (common run)
   [or recdef database only (make arg 6 NUL) (application-specific run)]
   or both.

   INPUT AND OUTPUT file name are specified on the command line as follows:

   1:  Datatypes definition input file (eg "dtypes.def")

   2:  Units definition file ("units.def")

   3:  Limits definition file ("dtlims.def")

   4:  Fields definitions file ("fields.def")

   5:  Records definition file ("records.def").
	  See comments in this file about * directives therein.

   6:  Path to include (.H) file output directory ("..\h") (. for current dir),
	  or NUL to not output .H files.  (Output H files are listed below.)

   7:  CSE: must be NUL (9-19-91); formerly --
	  Output recdef database name ("recdef.tkf" or "recdef.crs", etc) or NUL
	  to not output recdef database.  This file is read by routines in
	  cpdbinit.cpp to set up field and record tables in the application program.

   8:  In CSE must be NUL (9-9-91); formerly ...
	  Help index (input) file name used for linking help screens to schedules
	  ("..\help\helptk.idx").  NUL is allowed, to not check help handles,
	  if argument 7 is NUL.

   9:  In CSE must be NUL (9-9-91); formerly ...
	  Help dummy output file, or NUL for none.  This file receives an empty
	  help page for every item which should have help but does not, as an
	  aid to writing missing help screens.

   10:  NUL or generated .cpp source files path (..\cse) for dttab.cpp, untab.cpp,
	  dtlims.cpp, srd.cpp for new regime (1-91) compiled & linked-in tables for
	  use in programs (such as CSE) that do not use recdef files.

   11:  Optional option switches, grouped as single arg (eg "DXY"):
		D:  Debug: display token stream and other
			info as execution procedes.
		S:  SCDEF dump: dump schedule definition structures
			(SCDEFs) to file "scdef.dmp" as they are generated.

   Command files, such as cndef.bat, are generally used to invoke rcdef.


   OUTPUT FILES include [recdef database specified by arg 7] plus .H files as
   follows, in the directory specified by arg 6:

   dtypes.h   Contains datatypes, record types, limits, choices, units
 #included in all source files of each application via include in cnglob.h

   rcxxxx.h   Record structures and record field defines.  File names are
			  specified by *file directives in record definition file (arg 5). */


/*   "*" Directives   in records.def

   The record definition file contains a series of tokens which control
   construction of record descriptors.  Some of these tokens begin with "*"
   and are designated DIRECTIVES; they invoke options and special features.

   (See also comments in rec def file, and in code herebelow, re file format).

   Between-record * directives:

 *FILE <name.h>	Names .h file to receive record declarations til
						next *file.

   Record level * directives:

		Note: these are customarily given right after the record name, but
		program may actually accept them in same places as field level *words.

 *RAT		This is a "record array table" record type, used in arrays
				in dm blocks rather than singly in db blocks.
				An additional xxxBASE data type is also defined for each RAT.
				See ratpak.h/ratpak.cpp for more RAT info 2-90, or ancrec.h/.cpp 2-92.
				NOW THE NORMAL CASE, 2-92, even tho ratpak is replaced with ancrec.

 *SUBSTRUCT	This isn't really a record, but just a structure definition
				for use in C code and as a substructure *nest-ed in other
				records.  Saves space by omitting status bytes and front-end
				members put in normal records and RATs; makes pointer to
				first field member same as pointer to the aggregate.

 *BASECLASS <name>	Specifies a previously defined record type as C++ base class, 7-1-92.
						If omitted, base class is "record" for non-substructs.
						Only works with SUBSTRUCT as of 7-92, right?

   Field (of record) level * directives:

 *array <size>		Makes array.
						example:  *array 2 FLOAT pstat
						generates:  FLOAT pstat[2];

 *nest <recTyName> <memName>
				Nests a PREVIOUSLY DEFINED record type as a substructure in
				current record, as though all of its fields are individually
				in the record.  (For field #'s add nested member #defines to
				nestee member define).  Accessible in C code by member
				(p->name.name) or as aggregate.  For array, precede w/ *array
 *array <size>.  See record level *substruct directive. 3-90.
				  Example:   *nest FOO fum
				  Generates: FOO fum;		(where structure for FOO
												has already been output)
						and: #define FOO_FUM n

				WHY use nest record types?  Because if you define a desired
				nested structure as a data type and thus a single field,
				its members aren't accessible as fields e.g. for use with
				pgbuildr, cvfddisp, rcfManip, etc.  Or, if you used *struct,
				the structure isn't named for use elsewhere or in multiple
				record types.

 *struct <memName> <arSz> {		Makes array of (unnamed) structure
	... (member fields)			within record.  Nestable.  Can also
	} (or *END)				use arrays within the struct.  Unused,
										unmodernized, undebugged 1988-90.
				  Example:   *struct unsys 2 {
							 NAME
							 UNFACT
 |FOO fum
							 }
				  generates: struct {
								  KEY name;
								  FLOAT unfact;
								  FOO fum;
							   } unsys[2];

 */

/*------------------------------- INCLUDES --------------------------------*/
// #include <cnglob.h>	// above
#include <srd.h>

#include <lookup.h>
#include <xiopak.h>     // xffilcomp
#include <cuevf.h>      // EVFHR EVFMH
#include <cvpak.h>

// replacements for MSVC HIWORD / LOWORD
//  move to cnglob.h ?

typedef uint16_t UI16;
typedef uint32_t UI32;

inline UI32 GetLo16Bits(UI32 l) { return static_cast<UI16>(l); }
inline UI32 GetHi16Bits(UI32 l) { return l >> 16; }
inline UI32 SetLo16Bits(UI32 l, UI32 l2) { return (l & 0xFFFF0000) | (l2 & 0xFFFF); }
inline UI32 SetHi16Bits(UI32 l, UI32 l2) { return (l & 0xFFFF) | (l2 << 16); }
inline UI32 SetHiLo16Bits(UI32 hi, UI32 lo) { return ((hi << 16) + GetLo16Bits(lo)); }


/*-------------------------------- DEFINES --------------------------------*/
//  Note additional #definitions intermingled with declarations below

#ifndef RTBSUB  /* may be externally defined, eg in srd.h 12-91.
Bit of possible general interest currently only used in rcdef.cpp 12-91. */

#define RTBSUB   0x8000   /* Record type bit indicating Substructure type definition, only for nesting in other
record types or direct use in C code: Lacks record front info and stat bytes. 3-90.
rcdef.exe sets this bit for internal reasons, and leaves it set. */
#endif

#define REQUIRED_ARGS 10        // # required command line args not including command itself.

/* #define BASECLASS	define to include code for for C++ *baseclass, 6-92; should be undefined if not C++.
						   coded out as #defined, 4-19-10
						   Initial use is for derived *substruct classes; is in use & works well 9-92.
						   Unresolved re derived *RAT's:
							1. Allocation of front members at proper offsets not verified.
							2. Space waste by duplicate sstat[] not prevented.  Does it make errors? */

const int MAXRCS=130;		// Record names.  120 -> 130, 3-24
							// Also max rec handle (prob unnec??).
const int MAXFIELDS=200;	// Field type names.  Max 255 for UCH in srd.h,cul.cpp,exman 6-95.
							//   600-->200 1-92. ->130 5-95.   160->200 11-19
const int MAXDT=130;		// Data types.        160-->60 1-92. ->90 3-92. ->100 10-10 ->120 5-13 -> 130 4-18
const int MAXUN = 80;		// Units.              90-->60 1-92. ->80 3-92. ->60 5-95. ->80 7-13
const int MAXUNSYS=2;		// Unit systems. 5-->2 5-95.

const int MAXLM = 12;		// Limits. 25->12 5-95.
const int MAXFDREC=1050;	// Max fields in a record. Separated from MAXFIELDS, 4-92.

const int MAXDTH=600;		// max+1 data type handle. 800-->200 1-92 ->400 3-92. ->432(0x1b0) 2-94. ->352 (0x160) 5-95.
							//   352->400, 1-16; 400->500, 4-16; 500->600, 9-20
const int MAXDTC=105;		// maximum number of choices for choice data type.
//#define MAXARRAY  20		// largest number of record array structure members * NOT checked, but should be.
const int MAXNAMEL = 40;	// Max length of name, etc ("s" token)
const int MAXQSTRL = 512;   // Max length for quoted string ("q" token).  assumed >= MAXNAMEL for array allocations.
		// 80->200 3-90 for huge struct data types eg MASSBC. ->128 5-95 when out of memory.
const int MAXTOKS = 6;		// Maximum no. of tokens per action (size of arrays; max length format arg to gtoks). 10->6 10-92.
const int AVFDS = 70;		// Max average # fields / record, for Rcdtab alloc.  Reduce if MAXRCS*RCDSIZE(AVFDS) approaches 64K.
							// 25->40 3-92 after overflow with only 39 of 50 records. 40->50 10-92. ->40 5-95. ->50 7-95. */

// gtoks return values (fcn value and in gtokRet)
const int GTOK = 0;         // things seem ok
const int GTEOF = 1;		// unexpected EOF
const int GTEND = 2;        // first token was *END
const int GTERR = 3;		// conversion error or *END in pos other than first.  Error message has been issued.
int gtokRet = 0;                         // last gtoks call fcn return value
char Sval[MAXTOKS][MAXQSTRL];   // String values of tokens read (s, d, f formats), and deQuoted value of q tokens.
int Dval[MAXTOKS];               // Decimal Integer values of tokens (d)
float Fval[MAXTOKS];            // Float values of tokens (f)

// Predefined record types
#define RTNONE 0000     // may be put in dtypes.h from here

///////////////////////////////////////////////////////////////////////////////
// Stubs to minimize dependencies
///////////////////////////////////////////////////////////////////////////////
void CDEC byebye(int code)           // function to return from program
// called from rmkerr.cpp at least
// stub here to avoid including envpak.cpp
{
	throw code;		// return to main
}               // byebye
//-----------------------------------------------------------------------------
int getChoiTxTyX(const char* chtx)		// categorize choice text
// only cvpak.cpp function required; dup here to reduce dependencies
// TODO: find way to avoid duplication
{
	int tyX = *chtx == '*' ? chtyHIDDEN		// hidden
		: *chtx == '!' ? chtyALIAS		// alias
		: *chtx == '~' ? chtyALIASDEP	// deprecated alias
		: chtyNORMAL;
	return tyX;
}
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// LUTAB = simple symbol table support
///////////////////////////////////////////////////////////////////////////////
const int LUFAIL = -1;
class LUTAB
{
	int lu_nent;  	// # entries currently in table
	int lu_mxent;  	// Max # entries, excluding sentinel slot
	const char** lu_nmp;	// ptr to array of entry names
	unsigned char* lu_stat;  // optional char array of status bytes

public:
	LUTAB(const char** nmp, int N, unsigned char* stat = nullptr) : lu_nent(0), lu_mxent(N), lu_nmp(nmp), lu_stat(stat)
	{
	}
	int lu_GetCount() const { return lu_nent; }
	unsigned char lu_GetStat(int i) const { return lu_stat ? lu_stat[i] : 0; }
	const char* lu_GetName(int i) const { return lu_nmp[i]; }
	void lu_Setup(const char* sym, char* canonicl)
	{
		int c;
		while ((c = *(unsigned char*)sym++) != 0)
			*canonicl++ = toupper(c);
		*canonicl = '\0';
	}
	int lu_Find(const char* sym)
	{
		char canonicl[1000];
		lu_Setup(sym, canonicl);
		return lu_Search(canonicl);
	}
	int lu_Add(char* sym)
	{
		if (lu_nent >= lu_mxent)
			return LUFAIL;
		lu_Setup(sym, sym);
		int idx = lu_nent++;
		lu_nmp[idx] = sym;
		if ((lu_stat) != nullptr)
			lu_stat[idx] = 0;
		return idx;
	}
	int lu_Search(char* canonicl)
	{
		for (int i=0; i<lu_nent; i++)
		{	if (!strcmp(lu_nmp[ i], canonicl))
			{	if (lu_stat != nullptr)
					lu_stat[i] = 1;
				return i;
			}
		}
		return LUFAIL;
	}
};	// class LUTAB
///////////////////////////////////////////////////////////////////////////////

/*------------ General variables ------------*/

const char* ProgVrsnId = "RCDEF";	// program version identifying string used in errorlog file header info, erpak2.cpp.
									// Defined in config.cpp in "real" products.
FILE * Fpm;             /* current input stream, read by gtoks.  Note several files are opened
								   during command line checking; Fpm is set to each in sequence as they are used. */
FILE *Fout;             /* current output .hx file stream -- used locally several places. */
char* incdir=NULL;      // include file output path (from cmd line)
char* cFilesDir=NULL;   // cpp output file path (from cmd line)
bool Debug = false;		// iff true, token stream is displayed (from gtoks() during execution.  Set by D on command line.
int Errcount = 0;		// Number of errors so far

/*------- re Command Line --------*/

bool argNotNUL[ REQUIRED_ARGS+1]; /* non-0 if argv[i] not NULL;
								   0's for disabled fcns via following defines: */
#define HFILESOUT argNotNUL[6]          // true if outputting H files (arg 6 not NUL)
#define RCDBOUT   argNotNUL[7]          // if outputting recdef database
#define HELPCONV  argNotNUL[8]          // if checking or converting help references
#define HELPDUMMY argNotNUL[9]          // if outputting dummies for undef help
#define CFILESOUT  argNotNUL[10]        // if writing table .cpp source files



/*------------- Data type global variables -------------*/

ULI Dttab[MAXDTH + MAXDTC + 1];
// table of data type info, subscripted by data type. While filling, next subscript is dttabsz
//   Simple types: (int, float, ): contains 0
//   "choice" types (a hi bit flag in type define), also receives # choices and choice texts.
//        Snake offset is used here for choice text, stored in sizeof(char *) 2-94 to match appl's struct
//        re checking that data types don't collide.
//   array: # of elements
int dttabsz = 0;         // [next] slot in Dttab = next data type; is size of table (in ULI's) at end.
// non-sparse data type arrays: subscripts are dtnmi[datatype].
const char* dtnames[MAXDT];                  // data type names, set w luadd(dtlut..)
static LUTAB dtlut(dtnames, MAXDT);
const char * dtdecl[MAXDT];                           // Ptrs to decls for data type
const char * dtcdecl[MAXDT][MAXDTC];          // Ptrs to decls for choice data type. not since large & data seg full.
const char * dtxnm[MAXDT];                    // NULLs or ptrs to external data type names
int dtsize[MAXDT];               // Size of data type in bytes
int dttype[MAXDT];				 // Data type: bits plus Dttab subscript
int ndtypes = 0;                 // number of data types
static std::vector<int> dtnmi( MAXDT);  // data types -> info (data types are sparse)
										// dtnmi[ datatype] = idx into dtdecl[ ], dtxnm[ ] etc


/*------------- Unit variables -------------*/
int Nunsys;							// # unit systems. may not work if not 2
UNIT Untab[MAXUN*sizeof(UNIT)];     // units table: print names, factors.  Decl must be same as appl's (srd.h).
// int Unsysext = UNSYSIP;				// unit system currently in effect (so cvpak.cpp links)

const char* unsysnm[ MAXUNSYS];		// unit system names, w/o leading UNSYS-
int nuntypes = 0;                   // current number of unit types
const char* unnames[MAXUN];				// unit type names: set via unlut
const char* unSymTx[MAXUN][MAXUNSYS];	// units symbol text
const char* unFacTx[MAXUN][MAXUNSYS];   // unit factors for untab.cpp, AS TEXT to avoid conversion errors.
LUTAB unlut(unnames, MAXUN);

/*------------- Limit variables -------------*/

// note there is no limits table.
static const char * lmnames[MAXLM];                   // limit type names
static int lmtype[MAXLM];
LUTAB lmlut(lmnames, MAXLM);
int nlmtypes = 0;                                        // current number of limit types

/*------------- Field descriptor variables -------------*/

struct FIELDDESC
{
	int dtype; 
	int lmtype;
	int untype;
};
static FIELDDESC Fdtab[MAXFIELDS] = { { 0} };      // field descriptors table

// Field name stuff
static const char * rcfdnms[MAXFIELDS];		// Table of field name pointers
static unsigned char rdfas[MAXFIELDS];		// set 1 by lufind if used
LUTAB fdlut(rcfdnms, MAXFIELDS, rdfas);
int nfdtypes = 0;                                                        // Current number of field names

// MORE VARIABLES incl most record variables are below, just above record()

/*============================= LOCAL ROUTINES ============================*/
LOCAL void   dtypes( FILE* file_dtypesh);
LOCAL void   wdtypes( FILE *f);
LOCAL void   wChoices( FILE* f);
LOCAL void   wDttab( void);
LOCAL void   units( FILE* file_unitsh);
LOCAL void   wUnits( FILE* f);
LOCAL void   wUntab( void);
LOCAL void   limits( FILE* file_limitsh);
LOCAL void   wLimits( FILE* f);
LOCAL void   fields( void);
LOCAL RC     recs( char *argv[], FILE* fil_dtypesh);
LOCAL void   base_fds( void);
LOCAL void   base_class_fds( const char* baseClass, int& bRctype);
LOCAL void   rec_fds( void);
struct RCDstr;
LOCAL void   wrStr( FILE *rdf, char *name, int arSz, struct RCD *rcdpin, int evf, int ff);
LOCAL void   nest( FILE *rcf, char *recTyNm, int arSz, struct RCD *rcdpin, int evf, int ff, bool noname);
LOCAL void   wRcTy( FILE* f);
LOCAL void   wRcTd( FILE *f);
LOCAL void   wSrfd1( FILE *f);
LOCAL void   wSrfd2( FILE *f);
LOCAL void   wSrfd3( FILE *f);
#if 0
0 unused
0 LOCAL void   wSrfd4( FILE *f);
#endif
LOCAL void   sumry( void);
LOCAL FILE * rcfopen( const char *s, char **argv, int argi);
LOCAL int    gtoks( const char * );
LOCAL void   rcderr( const char *s, ...);
LOCAL int update( const char* old, const char* nu);
LOCAL void   newsec( const char *);
LOCAL const char* enquote( const char *s);

// Common string buffer
//   Used to preserve many strings.
//   Faster than malloc when dealloc not needed. See stash and stashSval functions.
const size_t STBUFSIZE = 100000;
char Stbuf[STBUFSIZE] = { 0 };	// the buffer
char* Stbp = Stbuf;			// points to beg of last stored string
//======================================================================
LOCAL char* stash(const char* s)

// copy given string to next space in our string buffer, return location
// return value is not const -- stashed values can be modified in place

{
	char* newStbp = Stbp + strlen(Stbp) + 1;     // point past last string to next Stbuf byte
	if ((newStbp - Stbuf) > STBUFSIZE - 500)
	{
		rcderr("Stbuf not large enough -- increase STBUFSIZE.");
		byebye(2);
	}
	Stbp = newStbp;
	return strcpy(Stbp, s);    // copy string there, ret ptr
}                       // stash
//--------------------------------------------------------------------
inline char* stashSval( int i)

// copy token in Sval[i] to next space in our string buffer, return location
// return value is not const -- stashed values can be modified in place
{
	return stash(Sval[i]);
}                       // stashSval
//======================================================================


//////////////////////////////////////////////////////////////////////////////
// RCDEF MAIN ROUTINE
//////////////////////////////////////////////////////////////////////////////

int CDEC main( int argc, char * argv[] )
{
	char fdtyphname[80];        /* data types file name */
	int exitCode = 0;

	// hello();

	// Startup announcement
	printf("\nR C D E F ");

	/* ************* Command Line, Arguments, Open Files *************** */

	// Check number of arguments
	if (argc <= REQUIRED_ARGS || argc > REQUIRED_ARGS+2)
	{
		printf("\nExactly %d or %d args are required\n",
			   REQUIRED_ARGS, REQUIRED_ARGS+1 );
		exit(2);	// do nothing if args not all present
					//	(Note reserving errorlevel 1 for possible
					//	future alternate good exit, 12-89)
	}

	try
	{

	// Test all args for NUL: inits macro "flags" HFILESOUT, HELPCONV, etc
	for (int i = 0; i <= REQUIRED_ARGS; i++)
		argNotNUL[i] = _stricmp( argv[i], "NUL") != 0;

	/* Get and check input file names from command line */
	FILE* file_dtypes  = rcfopen( "data types", argv, 1);
	FILE* file_units   = rcfopen( "units",      argv, 2);
	FILE* file_limits  = rcfopen( "limits",     argv, 3);
	FILE* file_fields  = rcfopen( "fields",     argv, 4);
	FILE* file_records = rcfopen( "records",    argv, 5);

	/* check include directory argument if specified */
	if (HFILESOUT)              /* if argv[6] not NUL */
	{
		incdir = argv[6];
		if (xfExist(incdir) != 3)
		{
			if (xfExist(incdir) == 0) {
				printf("\n'%s': Include file output directory path not found.\n", incdir);
				Errcount++;
			}
			else if (xfExist(incdir) == 1 || xfExist(incdir) == 2) {
				printf("\n'%s': Include file output directory path exists, but is not a directory.\n", incdir);
				Errcount++;
			}
			else {
				printf("\n'%s': Unknown error in finding Include file output directory path.\n", incdir);
				Errcount++;
			}
		}
	}

	/* check cpp file directory argument if specified */
	if (CFILESOUT)              /* if argv[10] not NUL */
	{
		cFilesDir = argv[10];
		if (xfExist(cFilesDir) != 3)
		{
			if (xfExist(cFilesDir) == 0) {
				printf("\n'%s': C++ file output directory path not found.\n", cFilesDir);
				Errcount++;
			}
			else if (xfExist(cFilesDir) == 1 || xfExist(cFilesDir) == 2) {
				printf("\n'%s': C++ file output directory path exists, but is not a directory.\n", cFilesDir);
				Errcount++;
			}
			else {
				printf("\n'%s': Unknown error in finding C++ file output directory path.\n", cFilesDir);
				Errcount++;
			}
		}
	}

	/* Option flags */

	if (argc == REQUIRED_ARGS + 2)      /* if another arg, it's options */
	{
		int i = REQUIRED_ARGS+1;
		for (char* s = argv[i]; *s != 0; s++)  /* decode options */
			switch (*s)
			{
			case 'd':
			case 'D':
				Debug = true;
				break;

			default:
				printf("\n\nUnknown option %c\n",*s);
				Errcount++;
			}
	}

	/* consistency checks */

	if (RCDBOUT || HELPCONV || HELPDUMMY)
	{
		printf("\nWarning: Arg 7 (recdef db), Arg 8 (help idx), and Arg 9\n"
			   "(help dummies file) should all be NUL.\n"
			   "Non-NULs have been ignored.\n");
		RCDBOUT = HELPCONV = HELPDUMMY = 0;      // insurance, shd not be refd
		// nb no Errcount++: program is to continue.
	}
	if (!HFILESOUT && !RCDBOUT &&!HELPDUMMY && !CFILESOUT)
		printf("\nWarning: no output specified (args 6, 7, 9, and 10 all NUL).\n"
			   "Proceeding, performing partial checks of input files.\n");

	/* Done command line -- check errors; exit if there are any */
	if (Errcount > 0)
	{
		printf( "\nAborting due to command line error(s)\n");
		exit( 2);	// do nothing if any args missing.
					// (Reserving errorlevel 1 for possible future alternate good exit, 12-89)
	}

	/* ************* Data types ************** */

	Fpm = file_dtypes;                  // Set input file for gtoks().  fopen() is in command line checking at beg of main
	FILE* fdtyph = NULL;
	if (HFILESOUT)                      // not if not outputting .h files
	{
		xfjoinpath(incdir, "dtypes.hx", fdtyphname);
		fdtyph = fopen( fdtyphname,"w"); // open in main becuase left open til end for record structure typedefs
	}
	dtypes( fdtyph);                            // local fcn, after main. sets many globals.

	/* ************ Unit definitions ************* */

	Fpm = file_units;           // Set input file for token-reader gtoks().  fopen()'d in cmd line checking at beg of main.
	units( fdtyph);                     // local fcn, sets globals, calls wUnits().
	fclose(Fpm);                // done with units def file

	/* ************ Get limit definitions ************* */

	Fpm = file_limits;          // Input file for token-reader gtoks().  fopen'd in cmd line checking at beg of main.
	limits( fdtyph);            // local fcn, sets globals, calls wlimits() ... write to dtypes.h
	fclose(Fpm);                // done with dtlims.def input

	/* ********* FIELD DESCRIPTORS ********** */

	Fpm = file_fields;          // Input file for token-reader gtoks().  fopen'd in cmd line checking at beg of main.
	fields();                   // local fcn, below. sets globals.
								// Fdtab remains for access while doing records.
	fclose(Fpm);                // done with fields.def input file

	/* ******************* RECORDS *********************/

	Fpm = file_records;         // Set input file for token-reader gtoks().  fopen'd in cmd line checking at beg of main.
	if (recs( argv, fdtyph) )   // local fcn, below, uses/sets globals, writes rcxxx.h files, adds record typedefs to dtypes.h.
		goto leave;             // error exit
	fclose( Fpm);               // done with records definitions input file

// Now close dtypes.h file, see if changed.
	if (HFILESOUT)              // if outputting .h files
	{
		fclose( fdtyph);         // opened above b4 dtypes() called
		printf("\n");
		char dtypesHPath[CSE_MAX_PATH];
		xfjoinpath(incdir, "dtypes.h", dtypesHPath);
		update( dtypesHPath, fdtyphname);        // compare, replace file if different.
	}

	/* ************* SUMMARY ******************* */

// All done
leave:
#if CSE_COMPILER == CSE_COMPILER_MSVC
	if (!_CrtCheckMemory())
		rcderr("Memory corruption!  Output validity dubious.");
#endif
// display and file summary
	sumry();                    // local fcn below. uses many globals.

// return to operating system
	byebye( Errcount + (Errcount!=0) ); // exit with errorlevel 0 if ok, 2 or more if errors
										// (exit(1) being reserved for poss future alternate good exit, 12-89
	}
	catch (int _exitCode)
	{
		exitCode = _exitCode;
	}
	return exitCode;
}           // main

///////////////////////////////////////////////////////////////////////////////
static int determine_size(		// size of a data type
	const char* decl)	// declaration text from cndtypes.def
// returns size (bytes) of decl if known else 0
{
static SWTABLE declSize[] =
{	{ "short",			sizeof(short) },
	{ "int",			sizeof(int) },
	{ "unsigned short",	sizeof(unsigned short) },
	{ "unsigned",		sizeof(unsigned) },
	{ "long",			sizeof(unsigned long) },
	{ "unsigned long",	sizeof(unsigned long) },
	{ "float",			sizeof(float) },
	{ "double",			sizeof(double) },
	{ "char",			sizeof(char) },
	{ "unsigned char",	sizeof(unsigned char) },
	{ "time_t",			sizeof(time_t) },
	{ NULL,				0 }
};
	int sz = 0;
	if (strchr(decl, '*'))
		sz = sizeof(char*);		// all ptrs have same size
	else
	{	// for previously defined equivalent
		int idx = dtlut.lu_Find( decl);
		if (idx >= 0)
			sz = dtsize[idx];
	}
	if (!sz)
		// look in table of known types (case sensitive)
		sz = looksw(decl, declSize, true);
	if (!sz)
	{   // try array: crude parse of type [ dim ]
		char declCopy[1000];	// copy to modifiable buffer
		strncpy0(declCopy, decl, sizeof(declCopy));
		const int arrayTokDim = 10;
		const char* toks[arrayTokDim];
		int nTok = strTokSplit(declCopy, "[]", toks, arrayTokDim);
		if (nTok == 2)
		{	int szTy = determine_size(toks[0]);
			if (szTy > 0)
			{	int dim;
				if (strDecode(toks[1], dim) == 1)
					sz = szTy * dim;
			}
		}
	}
#if 0	// do not report missing info
	if (!sz)
		printf("\nSize not found for '%s'", decl);
#endif

	return sz;

}	// determine_size
//======================================================================
LOCAL void dtypes(                      // do data types
	FILE* file_dtypesh)         // where to write dtypes.h[x]
{
	

	// global Fpm assumed preset to file_dtypes
	// global file fdtyph assumed open if HFILESOUT

	newsec("DATA TYPES");


// Init memory table that holds our sequential array subscripts for the sparse data types, for retreival later in rcdef.

	/* input format example:

			 Han  TypeName  Extern    Decl       Size  Max
			 ---  --------  --------  ---------- ----- ------
			 000    SI      XSI       "int"       2     none
			 0x013  PLANE   --	  "struct {float dircos[3],k;}"
												 16    none    ... */

	/* output into internal LI Dttab[] 2-94:
		  non-choice type:
			  LI size				0 HIWORD could indicate not a choice type
		  choice type (constructed struct CHOICB):
			  LI SetHiLo16Bits( nChoices, size)	 size in lo16, nChoices in hi16
			  char * choiceTexts[nChoices];  */ 
											
// 3-1-94 could now more cleanly store everything into Dttab thru srd.h:GetDttab(dt) -- do at next rework.

// loop over info in data types definition file
// manually assigned handles eliminated 2-25-2011
	const int STK0 = 0;
	const int STK1 = 1;
	dttabsz = 1;    // next free word in Dttab
					//  (reserve 0 for automatically generated DTNONE)
	while (gtoks("ss")==GTOK)       // 2 tokens:  typeName, Extern,   or:  *choicb/n, typeName.
									// Sets gtokRet to ret val, used after loop.
	{
		if (ndtypes >= MAXDT)                            // prevent overflow of internal arrays
		{
			rcderr("too many data types");
			continue;                                     // recovery imperfect
		}
		if (dttabsz >= MAXDTH)                             // protect re Dttab overwrite
		{
			rcderr( "handle 0x%x > max 0x%x.", dttabsz, MAXDTH );
			continue;                                     // recovery imperfect
		}

		const char* cp = nullptr;
		int choicb = 0;			// not (yet) a choice data type
		int choicn = 0; 
		if (*Sval[STK0] == '*')                             // is it "*choicb"?
		{
			if (_stricmp( Sval[STK0] + 1, "choicb")==0)
				choicb = 1;
			else if (_stricmp( Sval[STK0] + 1, "choicn")==0)
				choicn = 1;
			else
			{
				rcderr( "unrecognized data type *-word '%s':\n    expect '*choicb' or '*choicn'", Sval[STK0] );
				continue;                                  // recovery imperfect
			}
			strcpy( Sval[STK0], Sval[STK1]);                    // move next token (name) back
		}
		else                             // no *-word, not a choice type
			cp = (Sval[STK1][1] == '-')                  // if "--" given for external name
				 ? (char *)NULL                          // tell wdtypes to make no ext decl
				 : stashSval( STK1);                     // save extern type name in next Stbuf slot, set cp to it.  Used below.
		if (dtlut.lu_Find( Sval[ STK0]) != LUFAIL)       // check for duplicate
		{
			rcderr("Duplicate data type '%s' omitted.", Sval[STK1]);
			continue;
		}

		// common processing of choice, var, non-var data types

		int val = dtlut.lu_Add( stashSval( STK0) );		// same typeName in nxt Stbuf space, enter ptr to saved name in tbl.
														// Sets dtnames[]. Subscript val is also used for other dt arrays
		// dtMap.emplace(STK0, 0);

		if (val==LUFAIL)
			rcderr("Too many data types.");

		// subscript "val" to dtnmi for retrieval later by data type
		if (size_t(dttabsz) >= dtnmi.size())
			dtnmi.resize(dttabsz+10);
		dtnmi[ dttabsz] = val;


		if (choicb || choicn)
		{

			// process a choice type.  choicb/n format: han *choicb/n <name> {  name "text"   name "text"  ...  }

			dtxnm[val] = NULL;                                   // no external name
			dtdecl[val] = choicn ? const_cast<char*>("float") : const_cast<char*>("SI");               // ... int-->SI 2-94 to keep 16 bits
			dtsize[val] = choicn ? sizeof(float) : sizeof(SI);
			dttype[val] = dttabsz                                // type: Dttab index, plus
				  | (choicn ? DTBCHOICN : DTBCHOICB);    //  appropriate choice bit
			if (gtoks("s"))                                      // gobble the {
				rcderr("choice data type { error.");

			// loop over choices list

			int nchoices = 0;
			while (!gtoks("p"))                          // peek at next char / while ok
			{
				if (Sval[0][0] =='}')                     // if next char }, not next handle #
				{
					gtoks("s");
					break;                    // gobble final } and stop
				}
				if (gtoks("sq"))                          // read name, text
					rcderr( "Choicb problem.");

				if (getChoiTxTyX( Sval[ 0]) != 0)
					rcderr( "Disallowed prefix on choicb/n name '%s' --\n    choib.h will be bad.", Sval[0] );
				dtcdecl[ val][ nchoices]= stash(Sval[ 0]);                 // save choicb/n name

				// choice TEXT may specify aliases ("MAIN|ALIAS1|ALIAS2")
				// Items may begin with prefix
				//    * = hidden (output only, not recognized on input) (meaningful only on MAIN text)
				//    ! = alias (redundant)
				//    ~ = deprecated alias (same as ! but give info msg on input)
				//    else "normal"
				// NOTE: only MAIN yields #define C_XXX_xxx

				const char* chStr = stashSval( 1);         // choicb/n text

				// choicb/n index: indeces 1,2,3 used.
				int chan = nchoices + 1;                      // 1, 2, 3, ... here for error msg; regenerated in wChoices().

				int tyX = getChoiTxTyX( Sval[ 1]);
				if (tyX >= chtyALIAS)
						rcderr( "choicb/n '%s' -- main choice cannot be alias", Sval[ 0]);

				if (nchoices >= MAXDTC)
					rcderr( "Discarding choices in excess of %d.", MAXDTC);
				else
				{
					// check/fill choice's slot in Dttab entry
					int i = dttabsz + 1 + nchoices;   // choice text's Dttab subscript, after LI size/#choices, previous choices
					ASSERT( sizeof(char *)==sizeof(LI));  // our assert macro, cnglob.h, 8-95.
					ULI* pli = Dttab + i;
					if (*pli)
						rcderr(
							"Choice handle 0x%x for dtype %s apparently conflicts:\n"
							"    slot 0x%x already non-0.  Dttab will be bad.",
								chan, dtnames[val], i );
					*pli = ULI(chStr);	// choicb / n text snake offset to Dttab :
					nchoices++;                   // count choices for this data type
				}
			} // while (!gtoks("p"))  choices loop

			// set Dttab[masked dt] for choice type
			Dttab[dttabsz++] = SetHiLo16Bits( nchoices,                // Hi16 is # choices
											choicn ? sizeof(float) : sizeof(SI)); // Lo16 is size (2 or 4)
			dttabsz += nchoices;                                     // point past choices, already stored in loop above.
		}                // if choicb/n ...
		else             // not a choice type
		{

			// get rest of non-choice data type input and process

			if (gtoks("q"))								// get decl
				rcderr("Bad datatype definition");

			dtxnm[val] = cp;                             // NULL or external type text, saved above.
			dtdecl[val] = stashSval(0);                  // save decl text, set array
			dtsize[val] = determine_size(dtdecl[val]);	 // size to array
			*(Dttab + dttabsz) = dtsize[val];            // size to Dttab
			dttype[val] = dttabsz++;                     // type: Dttab index

		}        // if choice ... else ...

		// end of data types loop

		ndtypes++;                                       // count data types
	}  // data types token while loop

	if (gtokRet != GTEND)                               // if 1st token not *END (gtoks at loop top)
		rcderr("Data types definitions do not end properly");

// Write data type definitions to dtypes.hx, dttab.cpp

	if (HFILESOUT)                      // if outputting .h files
	{
		wdtypes( file_dtypesh);			// writes data types info. uses globals ndtypes, dtnames[], dtdecl[], dtxnm[],
										//    dttype[], dtcdecl[][].
		wChoices( file_dtypesh);        // write choice data types choice defines, below.
	}
	if (CFILESOUT)                      // if outputting .cpp files
	{
		printf("\n    ");
		wDttab();                        // write Dttab source code to compile and link into programs
	}

// Done with master definitions of data types

	fclose(Fpm);
	printf("   %d data types. ", ndtypes);

}               // dtypes
//======================================================================
LOCAL void wdtypes( FILE *f)

// write data types info to already-open dtypes.h file

// uses globals: dtnames[], dtdecl[], dtxnm[], dttype[], dtcdecl[][]
{

// introductory info

	fprintf( f,
			 "/* dtypes.h: data types include file automatically generated by rcdef.exe. */\n"
			 "\n"
			 "   /* this file is #included in cnglob.h. */\n"
			 "\n"
			 "   /* do not edit this file -- it is written over any time rcdef is run --\n"
			 "      instead change the input, dtypes.def (also records.def), \n"
			 "      and re-run rcdef. */\n"
			 "\n"
			 "   /* text of these automatically generated comments written 10-88 \n"
			 "      -- likely to be obsolete! */\n" ) ;

// data types typedefs

	fprintf( f,
			 "\n\n\n/* Typedefs for data types \n"
			 "\n"
			 "   For record fields, and general use.\n"
			 "   Type names are as specified in CNDTYPES.DEF.  \n"
			 "   \n"
			 "   For *choicb data types, the declaration is for a short int to hold a \n"
			 "   \"choicb index\" to represent the current selection\n"
			 "   (see C_xxx defines below in this file).\n"
			 "\n"
			 "   For *choicn data types, the declaration is for a float to hold a \n"
			 "   numerical value or a NAN representing \"unset\" or the current selection.\n"
			 "   (see C_xxx defines below, and cnglob.h NCHOICE and CHN macros).\n"
			 "\n"
			 "   (Structures for records (as specified in records.def) are not here, \n"
			 "   but in several RCxxxx.H files whose names are given in records.def). */\n"
			 "\n" );

	for (int i = 0; i < ndtypes; i++)               // dt 0 unused 12-91
	{
		const char *tail;
		/* parse declaration: put part starting with [ or ( not in {}'s after name,
		 so   typedef  char ANAME[16]  does not come out  typedef char[16] ANAME
		 and  typedef struct { float dircos[3]; } PLANE  still works */

		for (tail = dtdecl[i]; ; )
		{
			tail += strcspn( tail, "[({" );        /* # chars to [ ( { or end */
			if (*tail != '{')                     /* if not a {, */
				break;                            /* done: insert name here */
			for (int nstLvl = 1; nstLvl > 0 && *tail; )  /* {: scan to matching } */
			{
				tail += 1+strcspn( tail+1, "{}");  /* find { } or end */
				if (*tail=='{')                   /* each { means must scan */
					nstLvl++;                     /* to another } before */
				else                              /* .. again looking for [ ( */
					nstLvl--;
			}
		}
		fprintf( f, "typedef %.*s %s%s;\n",
				 static_cast<int>(tail-dtdecl[i]), dtdecl[i],  dtnames[i],  tail );
	}

// external defines

	fprintf( f,"\n\n\n"
			 "/* Data type extern declarations     names are  X<typeName> or as in dtypes.def\n"
			 "\n"
			 "   These define a single word for an external declaration for the type,\n"
			 "   for non-choice types. */\n\n" );

	for (int i = 0; i < ndtypes; i++)
		if (dtxnm[i] != NULL)
			fprintf( f, "#define %s extern %s\n", dtxnm[i], dtnames[i]);

// data types

	fprintf( f, "\n\n\n/* Data type defines    names are  DT<typeName>\n"
			 "\n"
			 "   Values stored in the field information in a record descriptor (also for \n"
			 "   general use) consisting of Dttab offset plus bits as follows:\n"
			 "\n");
	fprintf( f,
			 "   DTBCHOICB (0x2000)  Multiple-choice, \"improved and simplified\" CN style\n"
			 "                       (see srd.h): takes one of several values given\n"
			 "                       in dtypes.def; values are stored as ints 1, 2, 3 ... ;\n"
			 "                       Symbols for values are defined below.\n"
			 "\n");
	fprintf( f,
			 "   DTBCHOICN (0x4000)  Number OR Multiple-choice value stored in a float.\n"
			 "                       Choice values are stored in the float as NANs: \n"
			 "                       hi word is 0x7f81, 0x7f82, 0x7f83 ....\n"
			 "                       Symbols for values are defined below.\n"
			 "\n");
	fprintf( f,
			 "   LOWORD(Dttab[ dtype & DTBMASK]) contains, for all data types, \n"
			 "   the SIZE in bytes of the data type.\n"
			 "\n"
			 "   For a choice types, the number of choices and the\n"
			 "   text pointers of each choice follow per rcpak.h struct CHOICB.\n"
			 "   \n"
			 "   The position in Dttab comes from the \"type handle\" specified in dtypes.h;\n"
			 "   changing these invalidates compiled code. */\n"
			 "\n" );

	fprintf( f, "#define DTNONE 0x0000         // supplied by rcdef\n");
	fprintf( f, "#define DTUNDEF 0xc000        // supplied by rcdef\n");
	fprintf( f, "#define DTNA 0xc001           // supplied by rcdef\n");
	for (int i = 0; i < ndtypes; i++)
		fprintf( f, "#define DT%s %#04x\n", dtnames[i], dttype[i]);

}               // wdtypes
//----------------------------------------------------------------------
static const char* getChoiceText(		// retrieve choice text
	int dt,			// choice DT
	int chan)		// choice idx (1 based)
{
	int dtm = dt & DTBMASK;
	auto pli = Dttab + dtm;
	const char* chtx = (const char*)(*(pli + chan));
	return chtx;
}		// getChoiceText
//======================================================================
LOCAL void wChoices(            // write choices info to dtypes.h file.
	FILE* f)                    // where to write

// call only if HFILESOUT.
{
//----------- Choice data types choice defines
	fprintf( f,
			 "\n\n/* Choice data type CHOICE INDEX defines\n"
			 "\n"
			 "   These are values stored in fields and variables of choice\n"
			 "   (choicb, see srd.h) data types; the C_ defines are used in code to set\n"
			 "   and test choice data.\n"
			 "\n" );
	fprintf( f,
			 "   The value is the sequence number of the choice in dtypes.def.  If changed,\n"
			 "   grep and recompile all pertinent code.\n"
			 "\n"
			 "   The value is used in accessing the choice's text pointer at\n"
			 "	Dttab + (dt & DTBMASK) + masked value\n"
			 "\n"
			 "   The names in these defines are  C_<typeName>_<choiceName> */\n\n" );

	for (int i = 0; i < ndtypes; i++)
		if (dttype[i] & DTBCHOICB)
		{
			int n = GetHi16Bits( Dttab[dttype[i]&DTBMASK]);      // number of choices for data type from hi 16 bits of Dttab[dt]
			int iCh = 1;
			for (int j = 0; j < n; j++)
			{	[[maybe_unused]] const char* chtx = getChoiceText( dttype[ i], j+1);
				fprintf( f, "#define C_%s %u\n",
					strtcat( dtnames[i],"_",  dtcdecl[i][j], NULL),
					iCh++ );               // <-- choice handle or index. choicb difference
			}
		}

	fprintf( f,
			 "\n\n/* Number-Choice data type CHOICE VALUES\n"   // added 2-92
			 "\n"
			 "   These are used with \"choicn\" data types that can hold a (float) number\n"
			 "   or a choice.  They differ from choicb indeces in that hi bits have\n"
			 "   been added so as to form a NAN when stored in the hi word of a float.\n"
			 "   See cnglob.h for a macro (CHN(n)) to fetch the appropriate part of\n"
			 "   a float and cast it for comparison to these constants. \n"
			 "\n"
			 "   The names in these defines are  C_<typeName>_<choiceName> */\n\n" );

	for (int i = 0; i < ndtypes; i++)
		if (dttype[i] & DTBCHOICN)
		{	int n = GetHi16Bits( Dttab[dttype[i]&DTBMASK]);      // number of choices for data type from hi word of Dttab[dt]
			for (int j = 0; j < n; j++)
			{	// const char* chtx = getChoiceText( dttype[ i], j+1);
				fprintf( f, "#define C_%s 0x%x\n",
					 strtcat( dtnames[i], "_", dtcdecl[i][j], NULL),
					 j + NCNAN + 1);                   // <-- choicn difference.  NCNAN: 7f80, cnglob.h.
			}
		}

	fprintf( f, "\n// end of choice definitions\n");
}       // wChoices
//======================================================================
LOCAL void wDttab()     // write C++ source data types table dttab.cpp
{
// open working file dttab.cx
	char buf[CSE_MAX_PATH];
	xfjoinpath(cFilesDir, "dttab.cx", buf);		// buf also used to close
	FILE* f = fopen( buf, "w");
	if (f==NULL)
	{
		rcderr( "error opening '%s'", buf );
		return;
	}

// write front of file stuff
	fprintf( f,
			 "/* dttab.cpp: data types table source file automatically generated by rcdef. */\n"
			 "\n"
			 "   /* do not edit this file -- it is written over any time rcdef is run --\n"
			 "      instead change the input, dtypes.def, and re-run rcdef.exe. */\n"
			 "\n"
			 "\n"
			 "#include \"cnglob.h\"\n"
			 "#include \"srd.h\"	// declares variables defined here\n"
			 "//#include <dtypes.h>	typedef's types sizeof'd here -- already included via cnglob.h\n"
			 "\n"
			 "#define ML(low, high) ((ULI)(((USI)(low)) | (((ULI)((USI)(high))) << 16)))	//MAKELONG\n"
		   );

// write head of data types table
	fprintf( f,
			 "\n/* Data types table */\n"
			 "\n"
			 "ULI Dttab[] =\n"
			 "{ /* size   #choices if choice type\n"
			 "                choice texts if choice type   // type (Dttab subscript + bits) & symbol (dtypes.h) */\n");

// write content of data types table
//   sizeof(SI),					// DTSI
//   ML(sizeof(SI),2), (ULI)"yes",
//		       (ULI)"no",            // DTNOYES
	int w = 0;                                              // counts Dttab array members written
	for (int i = 0; i < ndtypes; i++)                       // loop data types
	{
		int dtm = dttype[i] & DTBMASK;                       // mask type for Dttab subscript

		// fill table for gap in user-assigned data type handles
		while (dtm >= w + 16)
		{
			fprintf( f, "    0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,\n");
			w += 16;
		}
		if (dtm > w)
		{
			fprintf( f, "    ");
			while (dtm > w)
			{
				fprintf( f, "0,");
				w++;
			}
			fprintf( f, "\n");
		}
		if (dtm < w)                                     // insurance: dtypes() prevents this
		{
			printf( "Internal error: data type out of order\n");
			continue;
		}

		// write info for type
		auto pli = Dttab + dtm;							// point Dttab entry for type
		if (!(dttype[i] & (DTBCHOICB|DTBCHOICN)))		// if not a choice
		{
			// non-choice type: size (in ULI) is entire entry.
			fprintf( f, "    sizeof(%s), ", dtnames[i]);         // write size: let compiler evaluate sizeof(type)
			fprintf( f, "\t\t\t");                       // space over
			pli++;                                       // point past non-choice Dttab entry
			w++;                                         // one LI (size) written
		}
		else                                             // is a choice type
		{
			// choice type: start entry with ML(sizeof(decl),#choices)
			int n = GetHi16Bits(*pli);
			pli++;                                       // point past size/# choices
			fprintf( f, "    ML( sizeof(%s), %d), ",
					 dtnames[i],                         // write declaration into sizeof, let compiler determine 16/32 bit size
					 n );                                // # choices comes from and goes to HIWORD of LI
			// choices
			for (int j = 0; j < n; j++)                      // loop choices
			{
				const char* chtx = getChoiceText( dttype[ i], j+1);
				fprintf( f, "\n\t\t(ULI)%-13s\t",			// write a choice text
						 strtprintf( "\"%s\",", chtx) );
			}
			w += 1 + n;                                  // # Dttab LI's written / next avail subscript
		}
		fprintf( f, "\t// 0x%04x DT%s\n", dttype[i], dtnames[i]);        // value & name comment, and newline
	}   // for i=

// write foot of data types table
	fprintf( f,
			 "};	// Dttab[]\n");

// write size of data types table (to provide same variables as recdef vsrn)
	fprintf( f,
			 "\n\n// size of data types table (in ULI's)\n"
			 "\nsize_t Dttmax = %d;\n", dttabsz );

// terminate file, close, update if different
	fprintf( f, "\n/* end of dttab.cpp */\n");
	fclose(f);
	char temp[CSE_MAX_PATH];
	xfjoinpath(cFilesDir, "dttab.cpp", temp);
	update( temp, buf);                         // compare file, replace if different

}               // wDttab()
//======================================================================
LOCAL void units(       // do units types, for rcdef main()
	FILE* file_unitsh)                  // where to write .h info
{

	newsec("UNITS");

	/* get target machine from def file */

	if (gtoks("sd") != GTOK)    // read "UNSYS" and decimal # unit systems
		rcderr("TARGET trouble.");
	Nunsys = Dval[1];
	if (Nunsys != 2)	// Not clear rcdef can handle Nunsys != 2, although input format implies that it can.
						// This error added to force checking if issue arises.
		rcderr( "Nunsys != 2 -- check RCDEF code and results !!");
	if (Nunsys > MAXUNSYS)                                              // check added 5-95 at MAXUNSYS reduction to 2
		rcderr( "Nunsys > MAXUNSYS -- expect RCDEF to crash !!");

	/* read unit systems stuff.  Used in writing units.h. */

	for (int i = 0; i < Nunsys; i++)
	{
		gtoks("s");                      /* read unit system name */
		unsysnm[i] = stash(_strupr(Sval[0]) );
	}

	/* unit types info format + examples.  2 lines/entry, for 2 unit systems.

	   Unit		Print
	   Type Name	Names		Factors		Description (comment)
	   ---------    ----------	-----------	----------------------
	   UNNONE		""		1.00		* No units *
					""		1.00

	   UNTEMP		"F"		1.00		* Temperature *
					"C"		0.55555556				*/

	/* read unit types info */

	while (gtoks("s")==GTOK)            // read Unit Type Name text
	{
		if (nuntypes >= MAXUN)          /* prevent overwriting table */
		{
			rcderr("Too many units.");
			continue;                    /* does not recover right */
		}
		if (unlut.lu_Find( Sval[0] ) != LUFAIL) // check if type in table
			rcderr("Duplicate unit type.");
		else
		{
			/* process unit typeName */

			int val = unlut.lu_Add( stashSval(0) ); /* save typeName text in Stbuf,
										   enter ptr in table, set unnames[val].
										   Subscript val is used as type #. */
			if (val == LUFAIL)
			{
				rcderr("Too many unit types.");
				continue;
			}

			/* unit systems info: print name, factor */

			for (int i = 0; i < Nunsys; i++)
			{
				gtoks("qf");                                    // read print name, factor
				unSymTx[val][i] = stashSval( 0);
				unFacTx[val][i] = stashSval( 1);              // save factor TEXT for untab.cpp
																// to avoid multiple conversion errors, 1-91 */
			}
			nuntypes++; /* count unit types read, for later output */
		}
	}
	if (gtokRet != GTEND)               // if last gtok() (at top of loop) did not read *END
		rcderr( "Units definitions do not end properly");

	/* make units.h and untab.cpp */
	if (HFILESOUT)      // not if not outputting .h files
		wUnits( file_unitsh);   // write units info header file uses Nunsys, unsysnm[], nuntypes, unnames[], .
	if (CFILESOUT)      // not if not outputting .cpp files
		wUntab();       // write units info source file to compile and link into programs, 1-91
	printf(" %d units.", nuntypes);
}                                       /* units */
//======================================================================
LOCAL void wUnits(              // write units info to units.h if different
	FILE* f)            // where to write

// uses Nunsys, unsysnm[], nuntypes, unnames[], incdir.
{
	/* head of file comments */
	fprintf( f,
			 "\n\n/* units definitions automatically generated by rcdef.exe. */\n");

	/* unit systems info */
	fprintf(f,"\n\n/* Unit systems */\n");
	for (int i = 0; i < Nunsys; i++)
		fprintf( f, "#define UNSYS%s %d\n", unsysnm[i], i);

	/* unit types info */
	fprintf( f, "\n\n"
			 "/* Unit types					  (comments written 10-4-88)\n"
			 "\n"
			 "   These are for .untype for fields (srd.h) as well as general use.\n"
			 "   They are subscripts into Untab (untab.cpp); also generated by rcdef.\n"
			 "   Untab is of type UNIT (srd.h) and contains a text and a conversion \n"
			 "   factor for each of the 2 unit systems. */\n\n" );

	for (int i = 0; i < nuntypes; i++)
		fprintf(f,"#define %s %d\n", unnames[i], i);
	fprintf( f, "\n// end of units definitions\n");
}                       /* wUnits */
//======================================================================
LOCAL void wUntab()                     // write untab.cpp
{
// open working file untab.cx
	char buf[CSE_MAX_PATH];
	xfjoinpath(cFilesDir, "untab.cx", buf);		// buf also used to close
	FILE* f = fopen( buf, "w");
	if (f==NULL)
	{
		rcderr( "error opening '%s'", buf );
		return;
	}

// head of file comments
	fprintf( f,
			 "/* untab.cpp: units table source file automatically generated by rcdef.exe. */\n"
			 "\n"
			 "   /* do not edit this file -- it is written over any time rcdef is run --\n"
			 "      instead change the input, units.def, and re-run rcdef. */" );

// write head of file stuff
	fprintf( f, "\n\n\n#include \"cnglob.h\"");
	fprintf( f, "\n#include \"srd.h\"	// declares variable(s) defined here");

// write unit systems info
	fprintf( f, "\n\n\n/* Unit systems */\n"
			 "\n"
			 "int Nunsys = %d;\t\t	// number of unit systems (2)\n"
			 "const char* UnsysNames[] = { ", Nunsys );
	for (int i = 0; i < Nunsys; i++)
		fprintf( f, "\"%s\", ", unsysnm[i] );
	fprintf( f, "NULL };\t// unit system names\n" );
	fprintf( f, "int Unsysext = 0;\t\t	// unit system currently in effect: used in cvpak.cpp\n");

// write unit types info
// (not latest):
//  UNIT Untab[] =
//	{ // -----unsys 1-----		-------unsys 2--------
//	// printName	factor      printName	factor
//	   "a/bc",	9.9999		"xxx"		1.234	/* UNXXXX */
//	   ...
//	};

	fprintf( f,
			 "\n\n/* Unit types table */\n\n"
			 "UNIT Untab[] =\n"
			 "{ //                -------unsys 1--------   \t            -------unsys 2--------\n"
			 "  //                printName       factor   \t            printName       factor\n"
			 "  //                ---------       ------   \t            ---------       ------\n" );

	for (int i = 0; i < nuntypes; i++)
	{
		fprintf( f, "  { {");
		for (int j = 0; j < Nunsys; j++)
			fprintf( f, j < Nunsys-1 ? " {%20s,%12s },\t" : "{%20s,%12s }",
					 enquote( unSymTx[i][j]),			// symbol text
					 unFacTx[i][j] );                                            // factor text
		fprintf( f, " } }, \t// %s\n", unnames[i]);
	}
	fprintf( f, "};	// Untab[]\n");

	/* terminate file, close, update if different */
	fprintf( f, "\n/* end of untab.cpp */\n");
	fclose(f);
	char temp[CSE_MAX_PATH];
	xfjoinpath(cFilesDir, "untab.cpp", temp);
	update( temp, buf);                 // compare, replace if different
}                       // wUntab
//======================================================================
LOCAL void limits( FILE* file_limitsh)  // do limit types
{
	newsec("LIMITS");

	/* read limits info from def file */

	nlmtypes = 0;
	while (gtoks("s")==GTOK)                    // read typeName.  Sets gtokRet same as return value.
	{
		if (lmlut.lu_Find( Sval[0]) != LUFAIL)
			rcderr("Duplicate limit type.");
		else
		{
			// enter in table.  Sets lmnames[val].
			int val = lmlut.lu_Add( stashSval( 0) );
			if (val==LUFAIL)
				rcderr("Too many limit types.");
			else if (val < MAXLM)
				lmtype[val] = val;                  // save type in array parallel to lmnames
		}
		nlmtypes++;
	}

	if (gtokRet != GTEND)                       // unless *END gtok'd last
		rcderr("Limits definitions do not end properly");

	/* Write limit definitions to .h file */

	if (HFILESOUT)              // not if not outputting .H files this run
		wLimits( file_limitsh);         // write .h, uses nlmtypes, lmnames[], lmtype[], incdir.

	printf(" %d limits. ", nlmtypes);

// Note: there is no limits table, thus no limits table C++ source file.
}                                       // limits
//======================================================================
LOCAL void wLimits( FILE* f)            // write to .h file

// uses nlmtypes,lmnames[],lmtype[],incdir.
{
	fprintf( f,
			 "\n\n// Limit definitions\n"
			 "//   These specify the type of limit checking, e.g. for cvpak.cpp functions.\n"
			 "//   There is no limits table.\n\n" );

	for (int i = 0; i < nlmtypes; i++)
		fprintf( f, "#define %s %d\n", lmnames[i], lmtype[i]);
	fprintf( f, "\n// end of limits\n");
}               // wLimits
//======================================================================
LOCAL void fields()     // do fields
{
	newsec("FIELD DESCRIPTORS");

// Fdtab[] is all 0 on entry

// make dummy 0th table entry to reserve field type 0 for "FDNONE", 2-1-91
	static char noneStr[]= { "none" };
	int val = fdlut.lu_Add( noneStr );
	nfdtypes++;

	/* loop to read field info from .def file */

	while (gtoks("ssss") == GTOK)               // (sets gtokRet same as ret val)
		/* read TypeName Datatype Limits Units
				[0]      [1]      [2]    [3] */
	{
		if (nfdtypes >= MAXFIELDS)
		{
			rcderr("Too many fields.");
			continue;
		}
		if (strlen(Sval[0]) >= MAXNAMEL)
			rcderr("Field name too long.");
		if (fdlut.lu_Find( Sval[0]) != LUFAIL)
		{
			rcderr("Duplicate field name attempted.");
			continue;
		}
		val = fdlut.lu_Add( stashSval(0) );		// typeName to Stbuf, ptr to lookup table.  Sets rcfdnms[val].
												// Subscript val is used on parallel arrays and as field type.
		if (val==LUFAIL)
		{
			rcderr("Too many field names.");
			continue;
		}
		if (dtlut.lu_Find( Sval[1]) == LUFAIL)
		{
			printf("\nWarning: cannot find data type %s",(char *)Sval[1]);
			rcderr("");
		}
		if (lmlut.lu_Find( Sval[2]) == LUFAIL)
		{
			printf("\nWarning: cannot find limit type %s",(char *)Sval[2]);
			rcderr("");
		}
		if (unlut.lu_Find( Sval[3]) == LUFAIL)
		{
			printf("\nWarning: cannot find unit type %s", (char*)Sval[3]);
			rcderr("");
		}
		Fdtab[ val].dtype = dttype[ dtlut.lu_Find( Sval[1]) ];
		Fdtab[ val].lmtype = lmtype[ lmlut.lu_Find( Sval[2]) ];
		Fdtab[ val].untype = unlut.lu_Find( Sval[3]);
		nfdtypes++;
	}
	if (gtokRet != GTEND)                               // if gtoks didn't read *END
		rcderr("Fields definitions do not end properly");

	printf(" %d field descriptors. ", nfdtypes);
}                                                       // fields


/*------------- Record Descriptor variables (and schedules) --------------*/

// Field descriptor substructure of record descriptor
struct RCFDD
{
	int rcfdnm;		// index to FIELDDESC in Fdtab
	int evf;		// field variation bits (eval frequency for expr using filed)
	int ff;			// field flags (attributes), defines in srd.h: FFHIDE, FFBASE.
};

#undef MBRNAMESX
#if defined( MBRNAMESX)
// idea: consolidated retention of member names
//   not complete, 4-2-2012
// structure to retain member names
struct MBRNM
{	char mn_nm[ MAXNAMEL+1];		// member name as input
	char mn_nmNoPfx[ MAXNAMEL+1];	// member name w/o prefix
	int mn_fnr;				// member field number
	MBRNM();
	void mn_Set( const char* nm, int fnr);
	MBRNM& mn_Copy( const MBRNM& s, int fnr=-1);
};
#endif

// Record descriptor, RCD.  For Rcdtab (below)
struct RCD
{
	int rcdtype;		// record type being described by this descriptor, w bits: RTxxxx values as defined in dtypes.h.
	int rcdrsize;		// size of record fields (excludes base class; only for odd/even)
	int rcdnfds;		// number of fields in record or RAT entry
	const char* rWhat;	// record description; same text is also in RATBASE.what.
	int rd_nstrel;		// number of structure elements (things with field # defines)
	char** rd_strelNm;  // name of each structure element: dm array of char ptrs
	char** rd_strelNmNoPfx;  // ditto w/o prefix
	int* rd_strelFnr;       // field number (rcdfdd subscript) for each structure element: dm array
#if defined( MBRNAMESX)
	MBRNM rd_mbrNames[ MAXFDREC];	// name etc of each structure element (things with field # defines)
#endif
	RCFDD rcdfdd[1];     // Field descriptors
	RCD()
	{ }
};
#define RCDSIZE(n) (sizeof (RCD) + (n-1)*(sizeof(RCFDD)))       // size of record descriptor for n fields

const size_t RCDTABBYTES =		// Rcdtab size (bytes)
						MAXRCS * sizeof(RCD*)      // pointers to max # records
                      + MAXRCS * RCDSIZE(AVFDS)     // + max # records with average fields each
                      + RCDSIZE(MAXFDREC);         // + one record with max # fields
const size_t RCDTABSZ = RCDTABBYTES / sizeof(RCD*);
static RCD* Rcdtab[ RCDTABSZ];	// array in which descriptors of all record types are built.
								//   Contains ptrs[] by rctype, then RCD's (descriptors). Ptrs are alloc'd RCD*,
								//   so program can make absolute, but contain offset from start block only.
								//   Set/used in recs()

static const char* rcnms[MAXRCS];		// rec names (typeNames) (part of rclut).  set: recs. used: rec_fds wRcTd wRcTy.
LUTAB rclut(rcnms, MAXRCS);
static int rctypes[ MAXRCS];           // rec types, w bits, by rec sequence number (current one in 'rctype')
const char* recIncFile[ MAXRCS];	// Include file name for each record type
int nrcnms = 0;					// # record names (max rcseq).  (Note value ret by luadd actually used for rcseq,
								//   to be sure subscrs of various arrays match.)  Set: recs(). used: wRcTy wRcTd sumry
// current record
RCD* rcdesc;			// loc of current/next record descriptor in Rcdtab. set: recs. used: rec_flds. */
int nextRcType = 1;		// re automatic generation of record type (new 7-10)
//   rctype 0 reserved for RTNONE
int rctype = -1;	// type of rcdesc being built, autoRcType + bits (= rcseq + bits)
int rcseq = -1;		// Sequence # of record under construction -- used as subscr of arrays; max is nrcnms.
					// set: recs. used: rec_flds, wrStr. */
int Fdoff =0;       // Field offset: moves fwd as fields defined.  set/used: recs, rec_flds, wrStr.
//  in C++/anchors version, excludes base class, only used for odd/even length checks.
int Nfields =0;			// # of fields in current record. set/used: recs, rec_flds, wrStr.
int MaxNfields=0;        // largest # of fields in a record (reported for poss use re sizing arrays in program). recs to sumry.
const char* rcNam=NULL;		// curr rec name -- copy of rcnms[rcseq]. Used in error messages, etc.

enum { RD_BASECLASS=32700, RD_EXCON, RD_EXDES, RD_OVRCOPY, RD_PREFIX, RD_UNKNOWN };
static SWTABLE rdirtab[] =      /* table of record level * directives:
										   Content is record type bit or other value tested in switch in records loop */
{
	{ "rat",       RTBRAT     },        // Record Array Table record type
	{ "substruct", RTBSUB     },		// substructure-only "record" definition
	{ "hideall",   RTBHIDE    },		// omit entire record from probe names help (CSE -p)
	{ "baseclass", RD_BASECLASS   },	// *baseclass <name>: C++ base class for this record type
	{ "excon",     RD_EXCON      },		// external constructor: declare only (else inline code may be supplied)
	{ "exdes",     RD_EXDES      },		// external destructor: declare only in class definition
	{ "ovrcopy",   RD_OVRCOPY     },	// overridden Copy()
	{ "prefix",	   RD_PREFIX	  },	// *prefix xx  drop xx from field # definitions and SFIR.mnames (but not from C declaration)
	{ NULL,        RD_UNKNOWN     }		// 32767 returned if not in table
};                      // rdirtab[]

enum { FD_DECLARE = 32720, FD_NONAME, FD_NEST, FD_STRUCT, FD_ARRAY, FD_UNKNOWN };
static SWTABLE fdirtab[] =      /* table of field level * directives.  Content word is:
										0x8000 + SFIR.ff bit, or
										SFIR.evf bit, or
										value for which switch in fields loop has a case. */
{
	// field flags, for SFIR.ff
	{ "hide",   0x8000 | FFHIDE },  // hide field: omit field from probe names help (CSE -p)

// variations: how often program changes field, for SFIR.evf
	// input rat members set during input checking should be encoded at least *r to prevent input time probes.
	// User input members may be encoded *z or with no *variation (see cncult tables for acceptable input expr variability).
	// run rat members with no encoded variation are treated as *r.
	{ "z",      0        },             // indicates no variation (rather than member not encoded yet): doct'n aid. optional.
	{ "i",      EVEOI    },             /* *i available after input, b4 setup (rat reference subscripts, resolved b4 topCkf, 1991;
												members accepting end-of-input variable expr (VEOI), mainly doc aid) */
	{ "f",      EVFFAZ   },             // *f may change after input (b4 setup) and again b4 main run setup after autosize. 6-95.
	{ "r",      EVFRUN   },             // *r runstart=runly=initially: at start run (in/after setup) ('constant' in a run rat)
	{ "y",      EVFRUN   },             // *y same; use with *e to indicate once at END of run (annual results)
	{ "m",      EVFMON   },             // *m
	{ "d",      EVFDAY   },             // *d
	{ "mh",     EVFMH    },             // *mh
	{ "h",      EVFHR    },             // *h
	{ "s",      EVFSUBHR },             // *s
	{ "e",      EVENDIVL },             // *e: value available at end of calcs for interval, not start (results)
	//  use with the above.
	{ "p",      EVPSTIVL },				// *p: value available after post-processing for interval
//  use with above

// specials: numbers match switch cases...
	{ "declare",  FD_DECLARE  },		// *declare "whatever"   spit text thru into record class definition immediately
	{ "noname",   FD_NONAME  },			// with *nest, omit aggregate name from SFIR.mnames (but not from C declaration)
	{ "nest",     FD_NEST },			// nest <name>: imbed structure of another rec type
	{ "struct",   FD_STRUCT  },			// struct name n { ...
	{ "array",    FD_ARRAY  },			// array n
	{ NULL,       FD_UNKNOWN }			// no match
};                      // fdirtab[]

// include file for record info output
const char* rchFileNm = NULL;	// Name of current record output include file, for recIncFile[] and for use in file text.
								// Notes: name with x is also in dbuff[] for closing/renaming.
								// FILE is "frc" in main(), set only if HFILESOUT.
// addl record vbls used by recs() and new callees rec_swl, rec_fds,
FILE * frc = NULL;      // current rcxxx output file if HFILESOUT
int nstrel;                      // subscr of struct members (elts) for fld
int strelFnr[MAXFDREC];          // field numbers for structure elts, this rec.  struct elts include those of base class.
char* strelNm[MAXFDREC];        // mbr names of struct elts in rec, subscr nstrel (not Nfields), for field #defines and err msgs.
char* strelNmNoPfx[MAXFDREC];	// ditto w/o prefix
// above 3 are saved in RCDstr members of same name at record completion re BASECLASS.
#if defined( MBRNAMESX)
MBRNM mbrNames[ MAXFDREC];		// mbr names of struct elts in rec, subscr nstrel (not Nfields), for field #defines and err msgs.
#endif

WMapIntStr rcPrefix;	// prefix for current record
// Full and user versions of qualified field name text, incl [i] & .nestMbr, of every member of this record,
// for small fields-in-record table (wSrfd3())
WMapIntStr fldFullNm2[ MAXRCS];
WMapIntStr fldNm2[ MAXRCS];	// no-prefix variant user name with some qualifying names omitted per *nohide, for probing.

//======================================================================
// modify element name
const int enfxUPPER = 1;		// convert to upper case
const int enfxNOPREFIX = 2;		// remove prefix if present
LOCAL WStr ElNmFix(
	const char* elNm,		// element name
	int options)
{
	WStr t( elNm);

	if ((options&enfxNOPREFIX) && !rcPrefix[ rcseq].empty())
	{
		int lPfx = static_cast<int>(rcPrefix[rcseq].length());
		if (t.substr(0, lPfx) == rcPrefix[ rcseq].c_str())
			t = t.substr( lPfx);
	}

	if (options & enfxUPPER)
		WStrUpper( t);

	return t;
}	// ElNmFix
//-----------------------------------------------------------------------
static char* fixName( char* p)		// fix name *in place*
{
	// trim off trailing ';' (common typo) 4-12
	int len = static_cast<int>(strlen(p));
	for (int i=len-1; i > 0; i--)
	{	if (p[ i] == ';')
			p[ i] = ' ';
		if (isspaceW( p[ i]))
			p[ i] = '\0';
		else
			break;
	}
	return p;
}	// fixName
//-----------------------------------------------------------------------
static void strelSave(		// save element name
	const char* nm,		// name
	int fnr)				// field number
{
	char* p = stash( nm);	// save member name
	fixName( p);
	strelNm[ nstrel] = p;
	strelNmNoPfx[ nstrel] = stash( ElNmFix( p, enfxNOPREFIX).c_str() );
	strelFnr[ nstrel] = fnr;                     // save field number for this member
}		// strelSave
//-----------------------------------------------------------------------
static void fldNm2Save(
	const char* nm,
	int nf)
{
char tNm[ MAXNAMEL+1];
	strcpy( tNm, nm);
	fixName( tNm);

	fldNm2[ rcseq][nf] = ElNmFix( tNm, enfxNOPREFIX);	// no-prefix variant
	fldFullNm2[ rcseq][nf] = tNm;
}		// fullNmSave
//----------------------------------------------------------------------
#if defined( MBRNAMESX)
MBRNM::MBRNM()
: mn_fnr( 0)
{	memset( mn_nm, 0, MAXNAMEL+1);
	memset( mn_nmNoPfx, 0, MAXNAMEL+1);
}		// MBRNM::MBRNM
//----------------------------------------------------------------------
void MBRNM::mn_Set(			// set member name info
	const char* nm, int fnr)
{
	strncpy0( mn_nm, nm, MAXNAMEL+1);
	strncpy0( mn_nmNoPfx, ElNmFix( nm, enfxNOPREFIX), MAXNAMEL+1);
	mn_fnr=fnr;
}	// MBRNM::mn_Set
//----------------------------------------------------------------------
MBRNM& MBRNM::mn_Copy(
	const MBRNM& s,		// source MBRNM
	int fnr/*=-1*/)		// if >= 0, alternative field number
{	memcpy( mn_nm, s.mn_nm, MAXNAMEL+1);
	memcpy( mn_nmNoPfx, s.mn_nmNoPfx, MAXNAMEL+1);
	mn_fnr= fnr >= 0 ? fnr : s.mn_fnr;
	return *this;
}	// MBRNM::mn_Copy
#endif
//======================================================================
LOCAL RC recs(                  // do records
	[[maybe_unused]] char *argv[],
	FILE* file_dtypeh)                  // open file dtypes.hx

// returns RCOK ok; RCBAD for some error exit cases
{

	// predefined/reserved record type number

	nrcnms = 0;         // init number of record names (total/max rcseq to output to dtypes.h; and for sumry)
	static char noneStr[] = { "NONE" };
	if (rclut.lu_Add( noneStr) != RTNONE)     // RTNONE must be 0th entry (historical)
		rcderr("Warning: RTNONE not 0");
	rctypes[0] = RTNONE;                        // type value for h file
	nrcnms++;                                   // count types for writing rec types


	/* ************* Records defined in def file ************ */

	/* nrcnms init above: RTNONE and any dummies already added to it */
	MaxNfields = 0;                     // max # fields in a record

	// open and start "small record & field descriptor" output .cpp file
	char fsrfdName[CSE_MAX_PATH]; // pathname.cx for small frd output file
	FILE* fSrfd = NULL; // small frd output FILE if CFILESOUT, else NULL
	if (CFILESOUT)                                      // if outputting tables to compile & link
	{
		xfjoinpath(cFilesDir, "srfd.cx", fsrfdName);	// also used below
		fSrfd = fopen( fsrfdName, "wt");
		if (fSrfd==NULL)
			printf( "\n\nCannot open srfd output file '%s'\n", fsrfdName );
		else                             // opened ok
			wSrfd1( fSrfd);              // write part b4 per-record portion, below
	}

	// initialize data base block for record descriptors
#if 1
	const char* rcdend = (char*)Rcdtab + sizeof( Rcdtab);		// end of allocated Rcdtab space
	rcdesc =                                    // init pointer for RCD creation
		(RCD*)((char*)Rcdtab + MAXRCS);  // into Rcdtab, after space for pointers
#else
	size_t uliTem =               // size for Rcdtab
		MAXRCS*sizeof(RCD**)      // pointers to max # records
		+ MAXRCS*RCDSIZE(AVFDS)        // + max # records with average fields each
		+ RCDSIZE(MAXFDREC);           // + one record with max # fields
	// dmal( DMPP( Rcdtab), uliTem, DMZERO|ABT);      // alloc record descriptor table in heap
	char* rcdend = (char *)Rcdtab + uliTem;		// end of allocated Rcdtab space
	rcdesc =                                    // init pointer for RCD creation
		(RCD*)((char **)Rcdtab + MAXRCS);  // into Rcdtab, after space for pointers
#endif

	/*--- set up to do records ---*/

	newsec("RECORDS");
	frc = NULL;                         // no rcxxxx.h output file open yet (FILE)
	rchFileNm = NULL;                   // .. (name)
	char dbuff[80] = { 0 };					// filename .hx
	 
	/*--- decode records info in def file ---*/

	/* top of records loop.  Process a between-records *word or a RECORD. */

	gtoks("s");                         // read token, set Sval and gtokRet
	while (gtokRet == GTOK)               // until *END or (unexpected) eof or error.  Sval[0] shd be "*file" or "RECORD".
	{
		bool excon = false;           // true iff record has explicit external constructor ..
		bool exdes = false;           // .. destructor ..  (declare only in class defn output here; omit default code)
		bool ovrcopy = false;         // .. if has overridden Copy()
		const char* baseClass = "record";	// current record's base class, default "record" if not specified by *BASECLASS
		bool baseGiven = false;				// true iff base class specified with *BASECLASS
		int i;

		if (nrcnms >= MAXRCS)                   // prevent table overflow
		{
			rcderr("Too many record types.");    // fallthru gobbles *END / eof w msg for each token.
		nexTokRec:;                            // come here after *word or error */
			if (gtoks("s"))                      // get token to replace that used
				rcderr("Error in records.def.");
			continue;                            // repeat record loop. gtokRet set.
		}
		if ((char*)rcdesc + RCDSIZE(MAXFDREC) >= rcdend)       // room for max rec recDescriptor?
		{
			rcderr("Record descriptor storage full.");           // if this occurs, recompile with larger MAXFDREC and/or AVFDS
			goto nexTokRec;                                      // get token and reiterate record loop
		}

		/* process *file <name> statement before next record if present */

		if (_stricmp(Sval[0], "*file") == 0)              // if *file
		{	
			if (frc)                            // if a file open (HFILESOUT and not start)
			{
				char temp[80];
				fprintf( frc, "\n\n/* %s end */\n", rchFileNm);
				fclose(frc);
				strcpy( temp, dbuff);
				temp[strlen(dbuff)-1] = '\0';             // Remove the last 'x' for file name
				update( temp, dbuff);
			}
			if (gtoks("s"))                             // read file name
				rcderr("Bad name after *file.");
			rchFileNm = stashSval(0);                   // store name for rec type definition and for have-file check below
			char rchFileNmX[CSE_MAX_FILENAME];				// rchFileNm variable with a x at the end
			sprintf(rchFileNmX,"%sx", rchFileNm);	// Add x
			xfjoinpath(incdir, rchFileNmX, dbuff);
			printf( "\n %s ...   ", dbuff);
			if (CFILESOUT)                              // if outputting tables to compile & link
				wSrfd2( fSrfd);                         // write "#include <rcxxx.h>" for new rchFileNm
			if (HFILESOUT)                              // if argv[6] not NUL
			{
				frc = fopen( dbuff, "w");
				if (frc == NULL)
				{
					rcderr( "Error opening *FILE '%s'", dbuff );
					return RCBAD;
				}
				fprintf( frc, "/* %s -- generated by RCDEF */\n", rchFileNm);
			}
			goto nexTokRec;             // get token and reiterate rec / *word loop
		}

		/* else input should be "RECORD" */

		if (_stricmp(Sval[0],"RECORD") != 0)
		{
			rcderr( "Passing '%s': ignoring to 'RECORD' or '*file'.", Sval[0] );
			goto nexTokRec;              // get token and reiterate rec / *word loop
		}

		/* be sure file specified before first record */

		if (!rchFileNm)                         // if no *file <name> yet processed
		{
			rcderr("No *file <name> before first record.");
			break;                              // intended to leave records loop
		}

		// automatically assigned record handle (type number)
		//   replaced hand-assigned scheme 7-10
		if (nextRcType >= MAXRCS)             // prevent exceeding tables
		{
			rcderr( "Record handle 0x%x is too big (max 0x%x).",
					nextRcType, MAXRCS-1 );
			goto nexTokRec;                     // get token and reiterate record loop
		}
		rcseq = nextRcType++;
		rctype = rcseq;					// bits are added to this to form record type
		rcPrefix[rcseq] = "";	// no prefix by default

		// record typeName
		if (gtoks("s"))                         // get next token: typeName (set gtokRet)
		{
			rcderr( "Record name trouble.");
			goto nexTokRec;                        // get token and reiterate record loop
		}
		if (strlen(Sval[0]) >= MAXNAMEL)
		{
			rcderr( "Record name too long.");
			goto nexTokRec;                        // get token and reiterate record loop
		}
		if (rclut.lu_Find(Sval[0]) != LUFAIL)
		{
			rcderr("Duplicate record name attempted.");
			goto nexTokRec;                        // get token and reiterate record loop
		}

		/* put record name in table used for dupl check just above, get
		   sequence # (our array subscript).  Stores name in rcnms[rcseq]. */
		if (LUFAIL == rclut.lu_Add( stashSval(0) ))		// save typeName, put in table, get subscript for parallel tbls.
		{
			rcderr("Too many record types.");
			goto nexTokRec;                                // get token and reiterate record loop
		}
		rcNam = rcnms[ rcseq];                  // get name ptr from where luadd put it, for use in many error messages

#if 0
x		// trap record name for debugging
x		static const char* trapRec = "AIRNET";
x		if (!_stricmp( rcNam, trapRec))
x		{    printf( "\nRecord trap!");}
#endif

		// record sequence number returned by luadd is used internally to index parallel arrays,
		// making it possible to loop over all record definitions

		// now we have record sequence number (array subscript) */
		for (i = 0; i < rcseq; i++)                             // dupl type check
			if ( ( (rctypes[i] ^ rctype) & RCTMASK) ==0)
				rcderr( "Duplicate record type 0x%x.", rctype );

		recIncFile[ rcseq] = rchFileNm;         // store rc file name for output later in comment in typdef in rctpes.h

		// record description/title text (".what" name, used in probes and error messages)
		if (gtoks("q") != GTOK)
		{
			rcderr("Record description text problem.");
			rcdesc->rWhat = "?";
		}
		else    // description ok
		{
			rcdesc->rWhat = stashSval(0);
		}

		/* process record level * directives */

		while (gtoks("s")==GTOK         // while next token (sets gtokRet) is ok
				&& *Sval[0]=='*')        // ... and starts with '*'
		{
			int val = looksw( Sval[0]+1, rdirtab);    // look up word after * in table, rets special value

			if (val==RD_UNKNOWN)                        // if not found
				break;								// leave token for fld loop.
			else switch ( val)                      // special value from rdirtab
			{
				default:                   // in table but not in switch
					rcderr("Bug re record level * directive: '%s'.", Sval[0]);
					break;                         // nb fallthru gets token

				case RD_BASECLASS:        // *baseclass <name>: C++ base class for this record type
					if (baseGiven)
						rcderr("record type '%s': only one *BASECLASS per record", rcNam);
					if (gtoks("s"))
						rcderr("Class name missing after '*BASECLASS' after '%s'", rcNam);
					baseClass = stash(Sval[0]);
					baseGiven = true;
					break;
				case RD_EXCON:
					excon = true;	// class has explicit external constructor: declare only in class defn
					break;
				case RD_EXDES:
					exdes = true;	// .. destructor
					break;
				case RD_OVRCOPY:
					ovrcopy = true;	// class has overridden Copy()
					break;
				case RD_PREFIX:		// *prefix xx -- specify member name prefix
					if (gtoks( "s"))
						rcderr( "Error getting text after '*prefix'");
					else
						rcPrefix[rcseq] = Sval[ 0];
					break;
				case RTBSUB:               // "*substruct": non-record only for *nest-ing and/or structure use in C code
				case RTBRAT:       // "*rat": a Record Array Table rec type (all (but *sub's) now are, 1991)
				case RTBHIDE:      // "*hideall": omit entire record from probe names help (CSE -p report)
					rctype |= val; // set bit in record type word
					break;
			}                // switch (val)
		}       // record level * directives loop
		// have a token

		/* check for invalid record type bit combinations */

		if ((rctype & (RTBRAT|RTBSUB)) == (RTBRAT|RTBSUB))
			rcderr( "*RAT and *SUBSTRUCT not valid together");

		// init to do fields

		nstrel = 0;     // counts structure elements in rec incl base class, 1 only for a nested array, struct, or record.
						// Each gets a RECORD_field #define.
		Nfields = 0;    // counts fields in rec, incl all elts of arrays and all nested structure/rec members.
						// Each gets field info entry in RCD->rcdfdd[].
		Fdoff = 0;      // init field offset (excluding base class): used to determine if record length odd/even

		/* write start of structure to current rcxxx.h file */

		if (HFILESOUT)                                  // if writing the h files
		{
			fprintf( frc, "\n\n/*--- %s ---*/\n\n", rcNam);
			if (rctype & RTBSUB)                                // for *SUBSTRUCT
			{
				fprintf( frc, "// %s record structure\n", rcNam);
				if (baseGiven)
					fprintf( frc, "struct %s : public %s\n", rcNam, baseClass);
				else
					fprintf( frc, "struct %s\n", rcNam);
				fprintf( frc, " { \n");                               // NO front non-fld members
				if (excon)                                                              // *excon
					fprintf( frc, "    %s();\n",  rcNam );
				if (exdes)
					fprintf( frc, "    virtual ~%s();\n",   rcNam );
			}
			else if (rctype & RTBRAT)                                   // for record array table
			{
				// class name, base class, open {
				fprintf( frc, "// %s record class\n"
						 "class %s : public %s \n"
						 "{ public:\n",               rcNam, rcNam, baseClass);
				// constructor and destructor
				//   //%s() : record() {}						<-- add default constructor if need found
				//   %s( basAnc *_b, TI i, SI noZ=0)  :  record( _b, i, noZ)  {}	<-- not anc<rcNam>: uses template b4 class rcNam defined.
				//   virtual ~%s() {}						(compiler supplies call to record::~record)
				if (excon)                                                              // *excon
					fprintf( frc, "    %s( basAnc *_b, TI i, SI noZ=0);\n",  rcNam );    // just declare external constructor
				else
					fprintf( frc, "    %s( basAnc *_b, TI i, SI noZ=0)  :  %s( _b, i, noZ)  {} \n",  rcNam, baseClass );
				if (exdes)
					fprintf( frc, "    virtual ~%s(); \n",   rcNam );                   // *exdes: declare external destructor
				else
					fprintf( frc, "    virtual ~%s() {} \n",  rcNam );

				// standard type-specific members: Copy() and operator=()
				fprintf( frc, "    %s& Copy( const %s& _d) { Copy( static_cast< const record*>(&_d)); return *this; }\n", rcNam, rcNam);
#if 0
0 experiment re elimination of duplicate Copy / CopyFrom
0 retain pending finalization of Copy / CopyFrom merge 6-2023
0				fprintf(frc, "    %s& operator=( const %s& _d) = delete;\n", rcNam, rcNam);
#else
				fprintf( frc, "    %s& operator=( const %s& _d) { Copy( static_cast< const record*>(&_d)); return *this; }\n", rcNam, rcNam);
#endif

				// virtual Copy(): override or not
				fprintf( frc, "    virtual void Copy( const record* pSrc, int options=0)%s;\n",
						 ovrcopy ? "" : " { record::Copy( pSrc, options); }");
				fprintf( frc, "\n");


			}
			else                                        // former 'normal' record */
				rcderr( "Record neither *RAT nor *SUBSTRUCT: no longer supported");
		}

		// get base class fields info for this record
		int bRctype = 0;                         // receives rctype of base class if *BASECLASS
		if (baseGiven)                          // if *BASECLASS given
		{
			base_class_fds( baseClass, bRctype);         // get table entries for the explicit base class
			rctype |= bRctype & RTBRAT;                  // *RAT propogates up, must be given at lowest base level
			if ( rctype & RTBRAT                         // (lowest) base of *RAT record must be *RAT, for default base members
					&&  !(bRctype & RTBRAT) )
				rcderr("record type %s: *RAT record cannot have a non-*RAT BASECLASS", rcNam);
		}
		else if (rctype & RTBRAT)	// if no *BASECLASS and not a *SUBSTRUCT (substructs have no implicit base class)
			base_fds();				// supply table entries for those fields of default base class that are visible to user


		// get fields for this record.  Have next token.

		rec_fds();              // local fcn, below.  uses: many globals above main()
		// on return, do NOT have token

		// store record size

		if (rctype & RTBSUB)                    // subtruct-use-only rec definition
			rcdesc->rcdrsize = Fdoff;                                   // "record" size is just the fields: used when nested.
		else                                    // not *substruct
			rcdesc->rcdrsize = Fdoff + Nfields + (Nfields+Fdoff)%2;     /* record size (less base class), forced even.
																		   believe not used, 2-92. */
		// complete record structure declaration -- status bytes, closing }

		if (HFILESOUT)                  // if writing the h files
		{
			if (!(rctype & RTBSUB))             // substruct recs have no sstat[]!
				fprintf( frc, "    unsigned char sstat[%d];		// fld status bytes (base class excluded)\n",
						 max( Nfields+(Nfields+Fdoff)%2, 1) );     // sstat[] size: # fields + 1 if struct len odd
			fprintf( frc, "};		// %s\n\n", rcNam);
		}

		if (gtokRet != GTEND)                           // if last token gtok'd not *END
			rcderr("Record definition error.");
		else                                            // record def input ended correctly
		{

			// set rest of record descriptor

			rcdesc->rcdnfds = Nfields;          // # fields incl array & struct mbrs
			rcdesc->rcdtype = rctype;           // record type # with bits

			rctypes[rcseq] = rctype;            // now also save type + bits for dupl checking, wRcTd, *nest, .

			gtoks("s");                         // get next token, set gtokRet for loop top

			// do stuff for this record in small record & field descriptor file
			// CAUTION: do before rcdesc incremented.
			if (fSrfd)                          // if CFILESOUT and opened ok
				wSrfd3( fSrfd);                 /* write flds-in-record tbl for this rt, below.
												   Uses globals: rcdesc, rcNam, fldNm[], rcfdnms[], nstrel, stelFnr, */

			if (HFILESOUT)                              // if writing the h files
			{

				// field number defines to current rc include file

				fprintf( frc, "// %s field numbers (subscripts for %s.sstat[] / sfir%s[])\n",
						 rcNam, rcNam, rcNam );
				WStr elNm;
				for (i = 0; i < nstrel; i++)                                    // for each struct element
				{
#if defined( MBRNAMESX)
					elNm = mbrNames[ i].mn_nmNoPfx;
					elNm.MakeUpper();
					fprintf( frc, "#define %s_%s %d\n",
							 rcNam, elNm, mbrNames[i].mn_fnr );
#elif 1
					elNm = strelNmNoPfx[ i];
					WStrUpper( elNm);
					fprintf( frc, "#define %s_%s %d\n",
							 rcNam, elNm.c_str(), strelFnr[i] );
#else
					elNm = ElNmFix( strelNm[ i], enfxUPPER|enfxNOPREFIX);
					fprintf( frc, "#define %s_%s %d\n",
							 rcNam, elNm, strelFnr[i] );
#endif
				}

				// # fields define (# sstat[] bytes, less poss odd-size fill byte)

				fprintf( frc, "\n#define %s_NFIELDS %d  \t\t\t\t\t// excludes rec start ovh; is # sstat[] bytes.\n\n",
						 rcNam, Nfields );

				// macros for instantiating anchor for this record type
				if (rctype & RTBRAT)
				{
#if 0
					// write record type define (used in macro below) so dtypes.h needn't be included first
					fprintf( frc, "#define RT%s\t%#04x\t\t\t\t\t// record type value\n\n", rcNam, rctype );
#endif

					// write extern for directly-linked small fields-in-record table (used in macro below)
					fprintf( frc, "extern SFIR sfir%s[];\t\t\t\t// %s small fields-in-record table (srfd.cpp)\n\n",
							 rcNam, rcNam );

					// write, for record name %s:
					//    #define  makAnc%s(name)        anc<%s> name( "<what>", sfir%s, %s_NFIELDS, RT%s)
					//    #define  makAnc%s2(name,what)  anc<%s> name( what, sfir%s, %s_NFIELDS, RT%s)      // 'what' override
					fprintf( frc, "#define  makAnc%s(name,pCult)        anc<%s> name( \"%s\", sfir%s, %s_NFIELDS, RT%s, pCult) \n", rcNam, rcNam, rcdesc->rWhat, rcNam, rcNam, rcNam );
					fprintf( frc, "#define  makAnc%s2(name,pCult,what)  anc<%s> name( what, sfir%s, %s_NFIELDS, RT%s, pCult)\t// 'what' override\n", rcNam, rcNam, rcNam, rcNam, rcNam );
				}
			}

			// dealloc or save full field name texts 3-5-91

			// save structure element info in case record is used as a base class 7-92
			rcdesc->rd_nstrel = nstrel;
#if !defined( MBRNAMESX)
#if 1
			rcdesc->rd_strelFnr = new int[nstrel];
			rcdesc->rd_strelNm = new char*[nstrel];
			rcdesc->rd_strelNmNoPfx = new char*[nstrel];
#else
			dmal( DMPP( rcdesc->rd_strelFnr), nstrel*sizeof(int), ABT);
			dmal( DMPP( rcdesc->rd_strelNm), nstrel*sizeof(char *), ABT);
			dmal( DMPP( rcdesc->rd_strelNmNoPfx), nstrel*sizeof(char *), ABT);
#endif
			memcpy(rcdesc->rd_strelFnr, strelFnr, nstrel * sizeof(int));
			memcpy(rcdesc->rd_strelNm, strelNm, nstrel * sizeof(char*));
			memcpy(rcdesc->rd_strelNmNoPfx, strelNmNoPfx, nstrel * sizeof(char*));
#else
			for (i=0; i<nstrel; i++)
				rcdesc->rd_mbrNames[ i].mn_Copy( mbrNames[ i]);
#endif

			// set relative pointer to descriptor block in table, point next space
			*(Rcdtab + (rcseq)) =
				(RCD*)((char *)rcdesc - (char *)Rcdtab);
			/* pointers are at front of block containing all descriptors (RCD's).  Cast the offset in block
			   into a pointer type: use of pointer type leaves room so program can make absolute. */
#if 1
			rcdesc = (RCD*)((char*)rcdesc + RCDSIZE(Nfields));	// where next RCD will go, or end Rcdtab if last one
#else
			IncP( DMPP( rcdesc), RCDSIZE(Nfields));     // where next RCD will go, or end Rcdtab if last one
#endif

			nrcnms++;                                   // count record names.  Note RTNONE was added to this above.

			if (Nfields > MaxNfields)                   // determine track of largest
				MaxNfields = Nfields;                   // .. # fields in a record
		}
	}   // Record loop (top ~500 lines up)

	if (gtokRet != GTEND)                               // if last token gtok'd not *END
		rcderr("Record definitions do not end properly");

// Close final record include file.
	// Note there is another close above for all but last close. Control structure should be reworked to eliminate this.

	if (frc)                                            // implies HFILESOUT
	{
		char temp[80];
		fprintf( frc, "\n\n// %s end\n", rchFileNm);
		fclose(frc);
		strcpy( temp, dbuff);                             // copy of name
		temp[ strlen(dbuff)-1] = '\0';                    // truncate final x
		update( temp, dbuff);                             // compare, update if different
	}

// finish and close small frd output .cpp file

	if (fSrfd)                                          // if doing it and opened ok
	{
		// wSrfd4( fSrfd);    write part after per-record stuff, below  DROPPED 6-2023
		fprintf( fSrfd, "\n\n/* end of srfd.cpp */\n" );
		fclose(fSrfd);
		printf("    \n");
		char srfdCPPPath[CSE_MAX_PATH];
		xfjoinpath(cFilesDir, "srfd.cpp", srfdCPPPath);
		update(srfdCPPPath, fsrfdName);       // compare new include file to old, replace if different
	}

// create file containing record type definitions (written to dtypes.h 4-5-10)

	if (HFILESOUT)              // if out'ing .H files
		wRcTy( file_dtypeh);                    // conditionally write file.  Uses nrcnms, rcnms[], rctypes[], recIncFile[], incdir.

	printf("  %d records. ", nrcnms);

	/* Write record typedefs to dtypes.h file.  caller main closes. */
	if (HFILESOUT)                      // if outputting .h files
		wRcTd( file_dtypeh);    // write record structure typedefs. uses globals nrcnms, rctypes[], rcnms[], recIncFile[]
	return RCOK;                        // 2+ RCBAD returns above
}                       // recs

//======================================================================
LOCAL void base_fds()

// supply table entries for user fields in (default) base class, 2-17-92

/* Certain standard user fields are defined in base class, for access in generic code;
   This function causes them to be included in most rcdef outputs (srfd.cpp, rc___.h except not class members)
   for access in the same manners as fields defined in records.def file. */

// Note that this is not ALL fields in base class -- internal-use overhead remains concealed.

// This probably does not make record nestable, as fields don't add up to record size.

// do not call for *substructs.

// uses lotsa globals.
{
	// base fields info.  MUST BE MAINTAINED TO MATCH BASE CLASS (ancrec.h:record, or as changed).
	static struct BASEFIELDS
	{
		const char* name;
		const char* fdTyNam;
		int evf;
		int ff;
	} baseFields[] =
	{
		//  name      fdTyNam    efv      ff
		//  --------  ---------  -------  ------
		  { "name",   "CULSTR",  0,       0      },       // record name, constant, visible.
		  { "ownTi",  "TI",      EVEOI,   FFHIDE },       // owning record index, *i, *hide.
		  { 0,        0,         0,       0      }
	};


	for (BASEFIELDS* bf = baseFields;  bf->name;  bf++)
	{
		// this code mimics rec_fds for scalar members, without * directives, and without class definition member output.

		// field name

#if !defined( MBRNAMESX)
		strelSave( bf->name, Nfields);
		fldNm2Save( bf->name, Nfields);
#else
		mbrNames[ nstrel].mn_Set( bf->name, Nfields);
#endif


		// field type

		int fieldtype = fdlut.lu_Find( bf->fdTyNam);     // look in fields table for field typeName
		if (fieldtype==LUFAIL)
		{
			rcderr( "Unknown field type name %s in BASEFIELDS table ", bf->fdTyNam );
			continue;
		}
		int dtIdx = dtnmi[ DTBMASK & (Fdtab+fieldtype)->dtype ];  // fetch idx to data type arrays for fld data type

		// put field info in record descriptor

		rcdesc->rcdfdd[Nfields].rcfdnm = fieldtype;             // field type (index in Fdtab)
		rcdesc->rcdfdd[Nfields].evf = bf->evf;                  // initial field flag (attribute) bits
		rcdesc->rcdfdd[Nfields].ff = bf->ff;                    // initial variation (eval freq) bits

		// increment

		Fdoff += abs( dtsize[dtIdx]);                         // update data offset in rec. why abs ??
		Nfields++;                                              // each field or each array element gets an rcdfdd entry in RCD
		nstrel++;                                               // next structure element (subscript) / is count after loop
	}
}               // base_fds
//======================================================================
LOCAL void base_class_fds( const char *baseClass, int &bRctype )

// enter info in record descriptor for fields of base class explicitly specified with *BASECLASS

// and uses: base field names from rcFldNms[][], .
// alters globals: *rcdesc, Nfields, Fdoff, fldNm[], nstrel, strelFnr[], strelNm[], .
{
// look up given base class name, get type, descriptor, # fields

	int bRcseq = rclut.lu_Find( baseClass);
	if (bRcseq==LUFAIL)
	{
		rcderr( "Undefined *BASECLASS record type %s in record %s",
				baseClass, rclut.lu_GetName( rcseq) );
		return;
	}
	bRctype = rctypes[bRcseq];
	RCD* bRcdesc = (RCD *)( (char *)Rcdtab + (ULI)Rcdtab[ bRctype&RCTMASK] );     // use offset stored in pointer array
	int bNfields = bRcdesc->rcdnfds;

// copy field info in record descriptor, update globals Nfields, Fdoff.

	RCFDD* dest = rcdesc->rcdfdd + Nfields;      // point destination RCD field info
	RCFDD* source = bRcdesc->rcdfdd;             // source field 0 info ptr
	for (int j = 0; j < bNfields; j++)                   // loop over base rec flds
	{
		// copy one field's info (substruct rcfdd of RCD)
		dest->rcfdnm =   source->rcfdnm;                // field type: Fdtab index
		dest->evf =      source->evf;                   // field variation: base record's
		dest->ff =       source->ff | FFBASE;           // field flags (attributes): merge base record's + base-class field flag
		fldFullNm2[ rcseq][Nfields] = fldFullNm2[ bRcseq][j];
		fldNm2[ rcseq][Nfields] = fldNm2[ bRcseq][j];
		// next field
		source++;
		dest++;                              // next fields in rec descriptors
		Nfields++;                                      // count fields in derived record
	}

// copy structure element info to globals used for current record (for field # defines)
	for (int j = 0;  j < bRcdesc->rd_nstrel;  j++)
	{
#if !defined( MBRNAMESX)
		strelFnr[nstrel] = bRcdesc->rd_strelFnr[j];
		strelNm[nstrel] = bRcdesc->rd_strelNm[j];
		// dmIncRef( DMPP( strelNm[nstrel]), ABT);
		strelNmNoPfx[nstrel] = bRcdesc->rd_strelNmNoPfx[j];
		// dmIncRef( DMPP( strelNmNoPfx[nstrel]), ABT);
#else
		mbrNames[ nstrel].mn_Copy( bRcdesc->rd_mbrNames[ j]);
#endif
		nstrel++;
	}

	/* field offset: pass whole base record: includes any stat bytes at end and front-end overhead non-field members at start
	   (all must be allocated as we declare substructure to C by type) */
	Fdoff += bRcdesc->rcdrsize;

	// another return above
}               // base_class_fds
//======================================================================
LOCAL void rec_fds()

// get user input fields for current record description

// at entry, next token has been gotten.  on return, token NOT gotten.

// uses: many of the globals above main(), and rctype, nstrel, strelFnr, strelNm,fldNm,rcFldNms, frc, .
{
	// when changing table entries made here, check base_fds just above re corress changes. 2-92.

	// fields loop begins here
	for ( ;                     // first time, have token from above
			gtokRet==GTOK;        // Leave loop if error or *END
			gtoks("s") )          // After 1st time, get token, set gtokRet
		// non-*END token is *directive, else field type.
	{
		// init for new field.  NB repeated in *directive cases that complete field: struct, nest. */
		int evf = 0;           // init field variation (evaluation frequency) bits
		int ff = 0;            // init field flag bits
		int array = 0;         // not an array (if nz, is # elements)
		bool noname = false;   // true to suppress nested structure name in SFIR.mname

		// check for too many fields/structure elements in record 4-11-92

		if (nstrel >= MAXFDREC || Nfields >= MAXFDREC)                                          // need nstrel be checked??
			rcderr( "Too many fields in record, recompile rcdef with larger MAXFDREC");         // no recovery yet coded

		// Process field level "*" directives

		while (*Sval[0] == '*')                 // if *word
		{
			int wasDeclare = 0;		// set nz iff handling *declare
			int val = looksw( Sval[0]+1, fdirtab);  // look up word after '*'
			switch (val)                        // returns bit or spec value
			{
			default:                    // most cases: bit is in table
				if (val & 0x8000)               // this bit says
					ff |= val & ~0x8000;        //   rest of val is bit(s) for "field flags" (eg *hide for FFHIDE)
				else
					evf |= val;                 // else val is bit(s) for "field variation"
				break;

			case FD_UNKNOWN:                 // not in *words table
				rcderr("Unknown or misplaced * directive: '%s'.", Sval[0]);
				break;                          // nb fallthru gets token

			case FD_ARRAY:                 // array n.  array size follows
				if (gtoks("d"))                 // get array size
					rcderr("Array size error");
				array = Dval[0];                // array flag / # elements
				break;

			case FD_STRUCT:                 // struct  memName  arraySize { ...
				/* unused (1988, 89, 90), not known if still works.  Any preceding * directives ignored.
				   Fully processed by this case & wrStr(), called here. */
				if (gtoks("sds"))                               //  memNam  arSz  {  (3rd token ignored)
					rcderr("Struct name error");
#if !defined( MBRNAMESX)
				strelSave( Sval[ 0], Nfields);
#else
				mbrNames[ nstrel].mn_Set( Sval[ 0], Nfields);
#endif
				wrStr( frc, Sval[0], Dval[1], rcdesc, evf, ff);
				/* read structure, conditionally write to output file, fill field info in record descriptor,
				   calls self for nested structures. Uses/updates globals Nfields, Fdoff, gtokRet,
				   NEEDS UPDATING to set fldNm[] -- 3-91 -- and fldFullNm[], 2-92. */
				++nstrel;                                       // next structure member
				// Note: a single field #define will be output by recs() for the entire structure (only one strel).
				// struct fields now completely processed.
				goto nextFld;           // continue outer loop to get token and start new field

			case FD_NEST:                 // nest <recTyName> <memName>
				// Any preceding * directives but *array and evf/ff ignored. Fully processed by this case & nest(), called here.
				if (gtoks("ss"))                                // rec type, member name
					rcderr("*nest error");
#if !defined( MBRNAMESX)
				strelSave( Sval[ 1], Nfields);
#else
				mbrNames[ nstrel].mn_Set( Sval[ 1], Nfields);
#endif

				nest( frc, Sval[0], array, rcdesc, evf, ff, noname );
				/* access prev'ly defined record type to be nested, copy its rec descriptor
				   info to ours, merge fld addrs, conditionally write mbr decl to out file,
				   Uses/updates globals Nfields, Fdoff, mbrNames, fldNm, rcFldNms, */
				++nstrel;                               // next structure element
				/* Note: a single field #define will be output by recs() for the entire nested record (only one strel).
				   Calling code may access fields within nested record by ADDING field #'s of nested record to it. */
				// nested record now completely processed.
				goto nextFld;           // continue outer loop to get token and start new field

			case FD_NONAME:                 // noname: with *nest, omit aggregate name from SFIR.mnames (but kept in C declaration)
				noname = true;
				break;

			case FD_DECLARE:                 // *declare "text": spit text thru immediately into record class definition.
				// intended uses include declarations of record-specific C++ member functions. 3-4-92.
				wasDeclare++;
				if (gtoks("q"))
					rcderr("Error getting \"text\" after '*declare'");
				else
					fprintf( frc, "    %s\n", Sval[0]);                 // format the text with indent and newline
				break;

			}           // switch (val)

			// get next token: next * directive else processed after *word loop
			// BUG: get error here if *declare is last thing in record, 10-94.
			int gtRet = gtoks("s");
			if (gtRet != GTOK)
			{	const char* msg = wasDeclare
					? "Error getting token after *declare (note *declare CANNOT be last in RECORD)"
					: "Error getting token after field * directive";
				rcderr(msg);
			}

		}   // while (*Sval[0] == '*') "*" directive
		// on fallthru have a token

		/* tokens after * directives are field type and field member name */

		char fdTyNam[100];			// current field type (assumed big enuf)
		strncpy0(fdTyNam, Sval[0], sizeof( fdTyNam));	// save type name

		if (gtoks("s"))                         // next token is member name
			rcderr("Error getting field member name");
		if (*Sval[0] == '*')
			rcderr("Expected field member name, found * directive");
#if !defined( MBRNAMESX)
		strelSave( Sval[ 0], Nfields);
#else
		mbrNames[ nstrel].mn_Set( Sval[ 0], Nfields);
#endif

		// now next token NOT gotten

		/* save full name with *array subscripts for sfir table, 3-91 */
		// also done in nest().
		if (CFILESOUT)                          // else not needed, leave NULL
		{
			if (array)
			{
				fixName(Sval[0]);		// drop trailing ';' if any
				for (int i = 0; i < array; i++)		// for each element of array
				{
					const char* tx = strtprintf("%s[%d]", Sval[0], i);	// generate text "name[i]"
					fldNm2Save(tx, Nfields + i);
				}
			}
			else                                                  // non-array
				fldNm2Save(Sval[0], Nfields);
		}

		/* get type (table index) for field's typeName */
		{
			int fieldtype = fdlut.lu_Find( fdTyNam);     // look in fields table for field typeName
			if (fieldtype == LUFAIL)
			{
				rcderr( "Unknown field type name %s in RECORD %s.", fdTyNam, rclut.lu_GetName( rcseq) );
				break;
			}

			int dtIdx = dtnmi[ DTBMASK & (Fdtab+fieldtype)->dtype ];          // fetch idx to data type arrays for fld data type

			/* put field info in record descriptor, alloc data space, count flds */

			int j = 0;
			do                      // once or for each array element
			{
				rcdesc->rcdfdd[Nfields].rcfdnm = fieldtype;         // field type (index in Fdtab)
				rcdesc->rcdfdd[Nfields].evf = evf;                  // initial field flag (attribute) bits
				rcdesc->rcdfdd[Nfields].ff = ff;                    // initial variation (eval freq) bits
				Fdoff += abs( dtsize[ dtIdx]);                     // update data offset in rec. why abs ??
				Nfields++;                                  // each field or each array element gets an rcdfdd entry in RCD
			}
			while (++j < array);

			/* write member declaration to rcxxxx.h file.  This code also in wrStr() and nest(). */

			if (HFILESOUT)                                  // if writing the h files
			{
				fprintf( frc, "    %s %s",
						dtnames[ dtIdx],                  // dtype name (declaration)
#if !defined( MBRNAMESX)
						strelNm[ nstrel] );                 // member name
#else
						mbrNames[ nstrel].mn_nm );			// member name
#endif

				if (array)                                   // if array
					fprintf( frc, "[%d]", array);       // [size]
				fprintf( frc, ";\n");
			}

			// warn if odd length: compiler might word-align next (rcdef does not)
			if ( (dtsize[dtIdx]                           // data type size
					* (array ? array : 1))                    // times array size or 1
					& 1 )                                      // if not whole words
				rcderr( "Odd size field %s used in record %s.  Compiler alignment complications probable.",
#if !defined( MBRNAMESX)
						strelNm[nstrel],
#else
						mbrNames[ nstrel].mn_nm,
#endif
						rcNam );

			nstrel++;               // next structure element (subscript) / is count after loop
		}
nextFld: ;                      /* *directives that complete processing field come here from inner loop
								   to get token and start new field if not *END.  *struct, *nest. */
	}    // end of field loop
}                                       // rec_fds
//====================================================================
LOCAL void wrStr(               // Do *struct field

	// Reads struct defn from records.def input file, writes decl to rcxxxx.h output file, puts fields info in rec descriptor.
	// Does not generate field # define. (recs() generates a single define for the entire structure).

	FILE *rcf,          // File to which to write declarations, or NULL for none
	char *name,         // member name of structure -- written after }
	int arSz,           // array size of structure, for [%d] at end (any member within structure can also be array)
	RCD *rcdpin,        // record descriptor being filled: field info added here
	int evf,            // field variation 1-92
	int ff )            // field flags (attributes) 1-92

// alters globals: Nfields, Fdoff,
// unused since '87 or b4; read/commented 1990; treat w suspicion.
// NEEDS UPDATING to set fldNm[] -- 3-91
{

	int fdoffbeg = Fdoff;
	int fieldbeg = Nfields;

	/* begin structure declaration: indent, write "struct {" */
	static int level = 0;
	level++;
	if (rcf)                                    // not if not HFILESOUT
	{
		for (int i = 0; i < level; i++)
			fprintf( rcf, "    ");               // indent
		fprintf( rcf, "struct {\n");
	}

	/* *directives / members loop */

	while (!gtoks("s"))                         // next token / while not *END/error
	{
		int array = 0;
		if (*Sval[0] == '}')
			break;                                       // stop on } (or *END?)
		if (*Sval[0] == '*')
		{
			if (!_stricmp( Sval[0]+1, "array"))                    // *array name n
			{
				if (gtoks("sd"))
					rcderr("Array name error");
				array = Dval[1];
			}
			else if (!_stricmp( Sval[0]+1, "struct"))              // nested *struct
			{
				if (gtoks("sds"))
					rcderr("Struct name error");
				if (rcf)
					wrStr( rcf, Sval[0], Dval[1], rcdpin, evf, ff); // call self
			}
			else
				rcderr("Unknown directive");
		}
		else                                     // no *
			array = 0;                           // not array

		/* member. unmodernized syntax:
					either	 name           data type AND member name
					or	 |typeName memberName */
		bool generic = false;
		if (*Sval[0] == '|')
		{
			strcpy( Sval[0], Sval[0]+1);
			generic = true;              // separate member name follows
		}

		/* look up field type */
		int fdTy = fdlut.lu_Find( Sval[0]);        // get subscript in Fdtab
		if (fdTy==LUFAIL)
		{
			rcderr( "Unknown field name %s in RECORD %s.",
					Sval[0], rclut.lu_GetName( rcseq) );
			break;
		}
		int dtIdx = dtnmi[DTBMASK&(Fdtab+fdTy)->dtype];    // get subscript into our data type tables

		// get member name
		char lcsnm[MAXNAMEL];            // Field name, lower case
		if (generic)
		{
			if (gtoks("s"))                       // separate token
				rcderr("Generic field error");
			_strlwr( strcpy( lcsnm, Sval[0]) );
		}
		else                                     // copy/lower same token
			_strlwr( strcpy( lcsnm, rcfdnms[fdTy]) );

		// output member declaration
		if (rcf)                                 // if outputting h files
		{
			for (int j = 0; j < level; j++)           // indent
				fprintf( rcf, "    ");
			fprintf( rcf,"    %s %s", dtnames[ dtIdx], lcsnm);
			if (array)
				fprintf( rcf, "[%d]", array);
			fprintf( rcf, ";\n");
		}

		/* make field info entry in record descriptor */
		for (int i = 0; i < (array ? array : 1); i++)        // member array size times
		{
			rcdpin->rcdfdd[Nfields].rcfdnm = fdTy;       // Fdtab subscript
			// note b4 12-91 .rdfa (predecessor of .evf/.ff) was set ZERO here.
			rcdpin->rcdfdd[Nfields].evf = evf;           // fld variation per caller
			rcdpin->rcdfdd[Nfields].ff = ff;             // fld flags (attributes) per caller
			Fdoff += abs(dtsize[dtIdx]);               // advance record offset
			Nfields++;                                   // count fields in record
		}
	} // end of members loop

	/* Structure complete.  If array of the structure being put in record,
	   repeat all the fields info for a total of 'arSz' repetitions. */

	int nflds = Nfields - fieldbeg;                 // # fields in structure
	int fldsize = Fdoff - fdoffbeg;                 // # bytes
	// odd length check desirable here
	for (int i = 1; i < arSz; i++)                  // start at 1: done once above
	{
		for (int j = 0; j < nflds; j++)              // loop fields within struct
		{
			int field = fieldbeg + i*nflds + j;       // output rcdfdd subscript
			int source = fieldbeg + j;                // input rcdfdd subscript
			// 1-92: bits for addl array elts were still being set 0, not per caller as 1st elt (above), assumed not intended.
			rcdpin->rcdfdd[field].evf = evf;                              // fld variation per caller
			rcdpin->rcdfdd[field].ff = ff;                                // fld flags (attributes) per caller
			rcdpin->rcdfdd[field].rcfdnm = rcdpin->rcdfdd[source].rcfdnm;
			Nfields++;
		}
		Fdoff += fldsize;                // for each array element, add total size of fields in struct to field offset
	}

	/* terminate declaration in the rcxxx.h file: "} name[size];" */
	if (rcf)
	{
		for (int i = 0; i < level; i++)
			fprintf( rcf, "    ");
		fprintf( rcf,"} %s[%d];\n", name, arSz);
	}
}                       // wrStr
/*=============================== COMMENTS ================================*/

/*           NESTING one record type in another

   Syntax:   *nest RECTYPE memName
 *array n  may precede the above to make an array of structures.
   RECTYPE must be previously defined.
   Suggest RECTYPE be in same rcxxx.h file.
   Characteristics
   Whole struct can be accessed as aggregate in C code using memName.
   Individual members can be accessed in C code in form p->name.name.
   Record descriptor field info is generated for each field of nested
	 structure as tho they were fields in nestee record, but only
	 a single field # define is generated for nestee record.
   Individual members can be accessed with rcpak fcns by field #
	 by ADDING field # defines of nested type to the single
	 nestee record define (RECORD_memName).
   Limitations
   Nested front and status bytes are allocated and wasted.
   Nested struct cannot be easily accessed as an aggregate by field #.
   Only one field # define generated for nestee record -- accesses 1st
	 nested member; use as base to add nested type's defines.
 */
//======================================================================
LOCAL void nest(                // Do *nest: imbed a previously defined record type's structure in record being defined

	// verifies record type, writes member declaration, copies its field info to current record descriptor
	// Does not generate field # define(s) (recs() generates ONE)

	FILE *rcf,          // File to which to write declarations, or NULL for none
	char *recTyNm,      // name of record type to nest in current record type
	int arSz,			// 0 or array size for array of the entire record
	RCD *rcdpin,        // record descriptor being filled -- field info added here
	int evf,            // field variation to MERGE 12-91
	int ff,             // field flags (attributes) to MERGE 1-92
	bool noname )         // non-0 to omit the structure name from SFIR .mnames (if not *array)

// and uses: nestee member name from mbrNames[nstrel], nested field names from rcFldNms[][]
// alters globals: Nfields, Fdoff, rctype, fldNm[],
{
	/* look up given record type name, get type, descriptor, # fields */

	int nRcseq = rclut.lu_Find( recTyNm);
	if (nRcseq==LUFAIL)
	{
		rcderr( "Undefined *nest record type %s in record %s",
				recTyNm, rclut.lu_GetName( rcseq) );
		return;
	}
	RCT nRctype = rctypes[ nRcseq];
	RCD* nRcdesc = (RCD *)( (char *)Rcdtab + (ULI)Rcdtab[nRctype & RCTMASK] );

	// use offset stored in pointer array
	int nNfields = nRcdesc->rcdnfds;

	// copy field info in record descriptor, update globals Nfields, Fdoff

	RCFDD* dest = rcdpin->rcdfdd + Nfields;             // point destination RCD field info
	int i = 0;                                          // arSz counter
	do                                                  // do it all arSz times.
	{
		RCFDD* source = nRcdesc->rcdfdd;                 // source field 0 info ptr
		const char* name1 =                              // mbr name of *nested rec, to add nested .mbr nms to.
			arSz
#if !defined( MBRNAMESX)
				? strtprintf( "%s[%d]", strelNm[nstrel], i)
				: strelNm[nstrel];
#else
				? strtprintf( "%s[%d]", mbrNames[nstrel].mn_nm, i)
				: mbrNames[ nstrel].mn_nm;
#endif

		for (int j = 0; j < nNfields; j++)                   // loop over nested rec flds
		{
			// copy one field's info (substruct rcfdd of RCD)
			dest->rcfdnm =   source->rcfdnm;             // field type: Fdtab index
			dest->evf =      source->evf | evf;          // field variation: merge nested record's and caller's
			dest->ff =       source->ff  | ff;           // field flags (attributes): merge nested record's and caller's
			// make field name text for sfir file (wSrfd3)
			if (CFILESOUT && !fldNm2[ nRcseq].empty())	// omit if no cpp file output (save ram) or ptr NULL (unexpected)
			{
				fldFullNm2[ rcseq][Nfields] =                      // Full name for C++ offsetof, etc
					strtprintf( "%s.%s", // qualified name: combine with '.':
										 name1, //   curr rec mbr name[i] (name of structure member)
										 fldFullNm2[ nRcseq][j].c_str() );		//   nested fld full name

				fldNm2[ rcseq][Nfields] =              // User name for messages, probing
						(noname && !arSz)                // on *noname request, if not array (subscript needed), don't qualify,
						?  fldNm2[ nRcseq][j]            //   just use nested member name.
						:  strtprintf( "%s.%s",          // else qualified name: combine with '.':
									   ElNmFix( name1, enfxNOPREFIX).c_str(),	//   curr rec mbr name[i] (name of structure member)
									   fldNm2[ nRcseq][j].c_str() );			//   nested fld user name
			}
			// next field
			source++;
			dest++;                           // next fields in rec descriptors
			Nfields++;                        // count fields in nestee record
		}
		/* nestee field offset: pass whole structure: includes any stat bytes
		  at end and front-end overhead non-field members at start
		  (all must be allocated as we declare substructure to C by type) */
		Fdoff += nRcdesc->rcdrsize;
	}
	while (++i < arSz);                         // repeat if nesting an array of record type

	/* write member declaration to rcxxxx.h file */

	if (HFILESOUT)                              // if writing the h files
	{
		fprintf( rcf, "    %s %s",
				 rcnms[nRcseq],                  // name of nested record is type
#if !defined( MBRNAMESX)
				strelNm[nstrel] );
#else
				mbrNames[ nstrel].mn_nm );		// structure mbr name
#endif

		if (arSz)                                // if array
			fprintf( rcf, "[%d]", arSz);    // [size]
		fprintf( rcf, ";\n");
	}
	// another return above
}                               // nest
//======================================================================
LOCAL void wRcTy(                       // conditionally write file with rec type defines
	FILE* f)            // where to write
{

	/* initial comments */
	fprintf( f,
			 "\n\n/* RECORD TYPE DEFINES\n"
			 "\n"
			 "   COMMENTS OBSOLETE FOR CSE: rcdef revisions required.\n"
			 "\n"
			 "   The record type allows access to the record descriptor in Rcdtab.\n"
			 "   The rctype is stored in the first word of every record and is extensively \n"
			 "   used by routines in rcpak.cpp and via macros defined in rcpak.h.\n"
			 "   \n"
			 "   Each name consists of \"RT\" and the <typeName> given in records.def.\n"
			 "\n"
			 "   Each record type value consists of an Rcdtab offset plus hi bits (rcpak.h):\n"
			 "      (no bits left in CN, 21-1-91)\n"
			 "\n");
	fprintf( f,
			 "   The low order bits (mask RCTMASK) are an offset into the pointer table \n"
			 "   at the start of Rcdtab (cpdbinit.cpp); the accessed pointer points to an \n"
			 "   RCD (rcpak.h) containing record and field information.\n"
			 "\n"
			 "   The offset is determined by the record handle specified in \n"
			 "   records.def; these should not be changed as existing saved files \n"
			 "   (as well as compiled code) would be invalidated. */\n\n" );

	/* record type defines */
	for (int i = 0; i < nrcnms; i++)
		fprintf( f,
				 "#define RT%s\t%#04x\n",
				 rcnms[i],                      // RTname
				 rctypes[i]);                   // type # with bits

	/* finish file */
	fprintf( f, "\n// end of record type definitions\n");
}       // wRcTy
//======================================================================
LOCAL void wRcTd( FILE *f)              // write record and anchor class declarations to dtypes.h file
// write [record structure typedefs] and [RAT BASE typedefs] to dtypes.h file

// uses globals nrcnms, rctypes[], rcnms[], recIncFile[]

{

	fprintf( f, "\n\n\n/* Record class and substruct typedefs\n"
			 "\n"
			 "   These are here to put the typedefs in all compilations (dtypes.h is \n"
			 "   included in cnglob.h) so function prototypes and external variable \n"
			 "   declarations using the types will always compile.\n"
			 "\n"
			 "   (Note: this section of dtypes.h is generated from information in \n"
			 "   records.def, not dtypes.def.) */\n\n" );

	for (int i = 1; i < nrcnms; i++)                        // Start at 1: skip RTNONE
	{
		fprintf( f, "%s %s;\n",
				 (rctypes[i] & RTBSUB) ? "struct" : "class",
				 rcnms[ i]);

		if (rctypes[i] & RTBRAT)
			if (rctypes[i] & RTBSUB)                     // for substruct (for record, use anc<recordName> directly.
				fprintf( f, "struct %sBASE;\n", rcnms[i]);
	}
	fprintf( f, "\n// end of dtypes.h\n");

}               // wRcTd
//======================================================================
LOCAL void wSrfd1( FILE *f)

// write "Small record and field descriptor" C++ file for compilation and linking into applications

// PART 1 of 4 parts: field types table, and stuff above portion repeated for each record type.

// for file format see comments in srd.h, or in srfd.cpp as output.
{

// Write front-of-file comments and includes

	fprintf( f,
			 "// srfd.cpp\n"
			 "\n"
			 "/* This is a Record and Field Descriptor Tables source file generated generated by rcdef.exe.\n"
			 "   This file is compiled and linked into CSE.*/\n" );
	fprintf( f, "\n"
			 "/* DO NOT EDIT: This file is overwritten when rcdef is run.\n"
			 "   To change, change rcdef.exe input as desired and re-run rcdef.exe via the appropropriate batch file. */\n\n"
			 "\n"
			 "#include \"cnglob.h\"	// includes <dtypes.h> for DTxxxx symbols\n"
			 "#include \"srd.h\"	// defines structures and declares variables.  Plus comments.  Manually generated file.\n"
			 "#include \"ancrec.h\"	// defines base class for record classes.  Manually generated file.\n"
			 "// also rccn.h is included below.\n"
			 "\n"
			 "#undef o		// in case an .h file defines o\n"// 1-95
		   );


// Write field types table

	fprintf( f,
			 "\n\n/*========= small FIELD TYPES table */\n"
			 "\n"
			 "struct SFDTAB sFdtab[] =	// indexed by SFIR.fdTy's below\n"
			 "{ /*\n"
			 "           data type      limits          units \n"
			 "             dtypes.h    dtypes.h       dtypes.h       <-- symbol defined in file\n"
			 "             Dttab[]          --        Untab[]       <-- is index into\n"
			 "             dttab.cpp        --        untab.cpp     <-- which is in source file */\n" );
	for (int i = 0; i < nfdtypes; i++)
	{
		// write data type 0 as "0", not "DTSI" or whatever is first, for clearer entry 0 (FDNONE)
		const char* dtTx = (Fdtab[i].dtype==0  ?  "0"
				:  strtcat( "DT", dtnames[ dtnmi[ DTBMASK&(Fdtab[i].dtype) ] ],	NULL ) );
		/* write line: DTXXXX,   LMXXXX,   UNXXXX,      // number  NAME */
		fprintf( f, "    {%15s, %10s, %13s },\t//%3d  %s\n",
				 dtTx,
				 lmnames[ Fdtab[i].lmtype ],
				 unnames[ Fdtab[i].untype ],
				 i,  rcfdnms[i] );
	}
	fprintf( f,
			 "    {              0,          0,             0 } \t// table terminator (needed?)\n"
			 "};		// sFdtab\n");


// Write stuff above "fir" tables for record types

	fprintf(f,
			 "\n\n/*========== small FIELDS-IN-RECORDS tables for each record type */\n"
			 "\n"
			 "// COLUMNS ARE\n"
			 "//    ff       field flags\n"
			 "//    fdTy     field type (sFdtab idx)\n"
		     "//    evf      variability (evaluation interval)\n"
		     "//    nxsc     fn of next field requiring special case copy\n"
		     "//    off      member offset\n"
		     "//    mName    member name\n"
		     "//    data type (as comment)\n\n");

// wSrfd3, called for each record type, continues the file
}               // wSrfd1
//======================================================================
LOCAL void wSrfd2( FILE *f)

// write "Small record and field descriptor" c++ source file for compilation and linking into applications

// PART 2 of 4 parts: portion repeated at each *file statement

// globals used include: rchFileNm: current rc___.h output include file name

// for file format see comments in srd.h, or in srfd.cpp as output.
{
	fprintf( f, "\n#include <%s>	// defines classes and structures whose tables follow\n\n", rchFileNm );
}                               // wSrfd2
//======================================================================
LOCAL void wSrfd3( FILE *f)

// write "Small record and field descriptor" C++ source file for compilation and linking into applications

// PART 3 of 4 parts: portion repeated for each record type: fields-in-record table for current record.

/* globals used include: rcNam: current record name
						rcdesc: current record descriptor pointer
					   fldNm[]: qualified field names
					 rcfdnms[]: field type names
						nstrel: number of struct elements in record
					strelFnr[]: 1st field # of each struct elt */

// for file format see comments in srd.h, or in srfd.cpp as output.
{
	// identify fields that cannot be copied bit-wise
	// chain them together for use in e.g. record::Copy
	std::vector< int> nxsc(rcdesc->rcdnfds, -1);
	int iNxscPrior = 0;
	for (int j = 0; j < rcdesc->rcdnfds; j++)
	{
		int fdi = rcdesc->rcdfdd[j].rcfdnm;              // field type
		if (strcmp( rcfdnms[fdi], "CHP")==0)
		{	nxsc[iNxscPrior] = j;
			iNxscPrior = j;
		}
	}

	fprintf( f, "struct SFIR sfir%s[] =\t// fields info for RT%s\n"
			 "{\n #define o(m) offsetof(%s,m)\n"
			 "//     ff  fdTy   evf  nxsc                 off               mName\n",
			 rcNam,  rcNam, rcNam );
	for (int j = 0; j < rcdesc->rcdnfds; j++)                    // fields loop
	{
		int fdi = rcdesc->rcdfdd[j].rcfdnm;                       // get field type
		fprintf( f, "    {%4d, %4d, %4d, %4d,%20s,%19s },\t// %s\n",
				 rcdesc->rcdfdd[j].ff,                   // field flags (attributes)
				 fdi,                                    // field type
				 rcdesc->rcdfdd[j].evf,                  // field variation
				 nxsc[ j],								 // next special copy field
				 strtprintf("o(%s)", fldFullNm2[rcseq][j].c_str()),	// full member name, in macro call to make compiler supply offset
				 enquote( fldNm2[ rcseq][j].c_str()),		// user member name in quotes (for probes, error messages)
				 rcfdnms[fdi] );							// name of field type, in comment
#define WARNAT (30-1)								// extra -1 got lots of messages
		if (strlen(fldNm2[ rcseq][j].c_str()) > WARNAT)		// report overlong ones
			rcderr( "Warning: member name over %d long: %s", WARNAT, fldNm2[ rcseq][j].c_str());
	}
	fprintf( f, "    {   0,    0,    0,    0,                  0,                  0 }\t// terminator\n"
			 " #undef o\n"
			 "};	// sfir%s\n\n", rcNam );
}                                                       // wSrfd3
//======================================================================
#if 0
0 DROPPED 6-2023 (unused)
0 LOCAL void wSrfd4( FILE *f)
0
0 // write "Small record and field descriptor" C++ file for compilation and linking into applications
0
0 // PART 4 of 4 parts: stuff after portion repeated for each record type; small record descriptor table.
0 {
0
0 // Write small record descriptor table
0
0	fprintf( f, "\n\n/*========== small RECORD DESCRIPTOR table */\n"
0			 "\n"
0			 "	// find desired entry by searching for .rt \n"
0			 "\n"
0			 "SRD sRd[] =\n"
0			 "{ //        recTy,    #fds,  fields-in-record pointer\n"
0			 "  //        .rt      .nFlds     .fir\n" );
0	for (int i = 1; i < nrcnms; i++)        // loop records types
0		// note start at 1 to skip RTNONE
0	{
0		RCD* rd = (RCD *)( (char *)Rcdtab + (ULI)Rcdtab[ rctypes[i] & RCTMASK]);
0		// use offset stored in pointer array
0		// write line "    RTXXXX,  nFields,  sfirXXXX,"
0		fprintf( f, "    {%15s, %3d,   sfir%s },\n",
0				 strtcat( "RT", rcnms[i], NULL),
0				 rd->rcdnfds,  rcnms[i] );
0	}
0	fprintf( f, "    {              0,   0,   0 }\t// terminate table for searching\n"
0			 "};		// sRd[]\n");
0
0	// caller recs() finishes file and closes.
0 }               // wSrfd4
#endif

//======================================================================
LOCAL void sumry()              // write rcdef summary to screen and file
{
	newsec("SUMMARY");
	for (int i = 0; i < 2; i++)             // once to file, once to screen
	{
		FILE *stream;
		if (i)
			stream = stdout;
		else
			stream = fopen("rcdef.sum","w");
		fprintf( stream, "   %d errors\n", Errcount);
		fprintf( stream, " Stbuf use: %zd of %zd\n",          Stbp-Stbuf, STBUFSIZE);
		fprintf( stream, " dtypes: %d of %d (%zd bytes)  ",   ndtypes,  MAXDT, dttabsz*sizeof(Dttab[ 0]) );
		fprintf( stream, " units: %d of %d \n",               nuntypes, MAXUN );
		fprintf( stream, " limits: %d of max %d          ",   nlmtypes, MAXLM );
		fprintf( stream, " fields: %d of %d\n",               nfdtypes, MAXFIELDS );
		fprintf( stream, " records: %d of %d             ",   nrcnms,   MAXRCS );
		fprintf( stream, " most flds in a record = %d of %d\n",   MaxNfields, MAXFDREC );
		if (Errcount)
			fprintf( stream, "Run did *NOT* complete correctly ************.\n");
		if (i==0)
		{	// list unused fields to rcdef.sum
			int n = 0;
			for (int j = 0; j < fdlut.lu_GetCount(); j++)        // loop over lupak table
				if (!fdlut.lu_GetStat( j))                    // if defined (lu_Add'ed) but not used (not lufound)
				{
					if (!n++)                                   // 1st time
						fprintf( stream, "\n\nUnused fields ->");
					fprintf( stream, "\n   %s", fdlut.lu_GetName( j));
				}
		}
	}
}                       // sumry
//======================================================================
LOCAL FILE * rcfopen(           // Open an existing input file from the command line

	const char *s,		// File identifying comment for error messages
	char **argv,		// Command line argument pointer array
	int argi )			// Index of command line arg to be processed

/* Returns FILE * for stream if open was successful.
   Otherwise, returns NULL, errors have been reported, and Errcount incremented */
{
	FILE *f = fopen( argv[argi], "r");
	if (f==NULL)
	{
		printf( "\nCannot open %s definition file '%s'\n",
				s, argv[argi]);
		Errcount++;
	}
	return f;
}               // rcfopen
//======================================================================
LOCAL int gtoks(                 // Retrieve tokens from input stream "Fpm" according to tokf

	const char *tokf )        /* Format string, max length MAXTOKS,
						   1 char for each desired token as follows:
						   s   string
						   d   integer value; any token beginning with 0x or 0X is decoded as hex
						   f   floating value
						   q   quoted string
						   p   peek-ahead: returns next non-blank char without removing it from input stream */

/* decoded tokens are placed in global arrays indexed by token #:
				Sval[]	string value (ALL types)
				Dval[]	int value (d).
				Fval[]	float value (f) */
/* Returns (fcn value AND global gtokRet):
		GTOK:       things seem ok
		GTEOF:      unexpected EOF
		GTEND:      first token was *END
		GTERR:      conversion error or *END in pos other than first,
					Error message has been issued. */
{
	// loop over format chars

	char f;             // format of current token
	int ntok = -1;		// token number
	while ( (f = *tokf++) != 0)       // get next fmt char / done if null
	{
		if (ntok++ >= MAXTOKS)
			rcderr("Too many tokens FUBAR !!!");

		// first decomment, deblank, and scan token into token[]
		char token[400];            // token from input file
		if (f=='q' || f=='p')
		{

			/* deblank/decomment for q or p */
			int c = 0;
			while (1)
			{
				c = fgetc(Fpm);                // next input char
				if (c == EOF)                  // watch for unexpected eof
				{
					goto ueof;
				}
				if (c=='/')                       // look for start comment
				{
					int nextc = fgetc(Fpm);
					if (nextc != '*')                       // if no * after /, unget and
						ungetc( nextc, Fpm);                // fall thru to garbage msg
					else
					{
						do                                   // pass the comment
						{
							while ((c=fgetc(Fpm)) != '*')     // ignore til *
								if (c == EOF)
								{
									goto ueof;
								}
						}
						while (fgetc(Fpm) != '/');           // ignore til / follows an *
						continue;                            // resume " scan loop
					}
				}
				if (!strchr(" \t\r\n\f", c))      // if not whitespace
				{
					if (f=='p')                           // for p, done deblank at any nonblank
						break;                            // c is 1st nonblank char
					if (c=='"')                           // for q, done deblank at open quote
						break;
					rcderr( "Ignoring garbage %c where quoted text expected.", c);
				}
			}                // while (1): deblanking for p, q
			if (f=='p')
			{
				/* set token[] for type p */

				token[0] = (char)c;                      // nonblank on which deblank ended
				token[1] = 0;
				ungetc( c, Fpm);                         // put the char back in input stream
			}
			else if (f=='q')
			{
				/* scan type q token */
				// opening quote already input (c)
				int i;
				for (i = 0; i >= 0; i++)                 // scan to close quote
				{
					if ((token[i] = (char)fgetc(Fpm))=='"') // get char / if ""
					{
						if (i <= 0  ||  token[i-1] != '\\') // if no \ b4 it
							break;                        // end token
						token[--i] = '"';                 // overwrite \ with "
					}
					else if (token[i]=='\n')             // newline: error
					{
						token[i+1] = '\0';
						rcderr("Warning: Newline in quote field: '%s'.", token);
						return (gtokRet = GTERR);
					}
				}
				token[i] = 0;
				if (i >= MAXQSTRL)
					rcderr( "Error: Quoted string longer than %d: %s", MAXQSTRL, token);
			}    // if q
		}
		else     // fmt not p or q
		{

			/* deblank/decomment/get token for all other formats */

			/* read tokens till not in comment */
			int commentflag = 0;	// 1 if currently scanning comment
			int oldcommentflag = 0;
			do
			{
				int nchar = fscanf(Fpm,"%s",token);
				if (nchar < 0)
				{
					goto ueof;
				}
#if 1        // work-around for preprocessor inserting a NULL in file
				// .. (just before comment terminator) 6-13-89
				while (nchar > 1
						&& *token=='\0'                   // leading null !!?? in token
						&& int( strlen(token+1)) <= nchar)  // insurance: only if rest is terminated within nchar
					// ^^^ should be < but did not work
				{
					strcpy( token, token+1);
					nchar--;
					printf( "\n  gtoks removing leading null from token !? \n");
				}
#endif
				static const char* comdelims[2] = { "/*", "*/" }; // Comment delimiters
				oldcommentflag = commentflag;
				commentflag =
					!(commentflag ^ (strcmp( token, comdelims[commentflag]) !=0) );
			}
			while (oldcommentflag || commentflag);

			/* check for *END eof indicator (formerly $END 6-92, but Borland CPP won't pass $ thru) */
			if (!strcmp(token,"*END"))
			{
				if (ntok == 0)
					return (gtokRet = GTEND);
				rcderr("Unexpected *END.");
				return (gtokRet = GTERR);
			}

			/* put token as a string in Sval[].  d,f,x further decoded below. */
			if (strlen(token) >= MAXNAMEL)
				rcderr("Error: token longer than MAXNAMEL: '%s'.", token);
		}                // if q else

		strcpy( Sval[ntok], token);                      // put token in appropriate Sval[]

		if (Debug)
		{
			if (ntok==0)
				printf("\n");
			else
				printf("\t");
			printf("%s",token);
		}

		/* decode token[] per format type */

		switch (f)
		{
		case 'd':
			{	const char* fmt = nullptr;
				const char* t2 = nullptr;
				if (*token == '0' && toupper(*(token + 1)) == 'X')
				{
					fmt = "%hx";                                      // use %x if token looks hex-ish
					t2 = token + 2;                                     // point after 0x
				}
				else
				{
					fmt = "%hd";
					t2 = token;
				}
				if (sscanf(t2, fmt, &Dval[ntok]) != 1)
					rcderr("Bad integer value: '%s'.", token);
			}
			break;

		case 'f':
			if (sscanf(token,"%f",(float*)&Fval[ntok]) != 1)
				rcderr("Bad real value: '%s'.", token);
			break;

		case 'p':                        // already decode separately above
		case 'q':                        // already decode separately above
		case 's':
			break;               // already complete: Sval is set

		default:
			rcderr("Format FUBAR !!!");
			break;
		}
	} // while (format char)
	return (gtokRet = GTOK);            // multiple error returns above
ueof:
	rcderr("Unexpected EOF: missing '*END'?");
	return (gtokRet = GTEOF);
}                       // gtoks
//======================================================================
LOCAL void rcderr( const char *s, ...)                // Print an error message with optional arguments and count error

// s is Error message, with optional %'s as for printf; any necessary arguments follow.
{
	FILE *stream;
	static FILE *errfile=NULL;
	va_list ap;

	va_start( ap, s);                   // point at args, if any
	const char* ss = strtvprintf( s, ap);           // format (printf-like) into Tmpstr
	for (int i = 0; i < 2; i++)
	{
		if (i==0)
			stream = stdout;
		else
		{
			if (errfile == NULL)
				errfile = fopen("rcdef.err","w");
			stream = errfile;
		}
		fprintf( stream, "\n    %s\n", ss);
		fprintf( stream, "Error occurred near: %s\n", Stbp );
	}
	Errcount++;

	/* #define ABORTONERROR */
#ifdef ABORTONERROR
	exit(2);                            // (exit(1) reserved for poss alt good exit, 12-89)
#endif
}               // rcderr
//======================================================================
LOCAL void newsec(const char *s)              // Output heading (s) for new section of run
{
	printf("\n%-16s  ", s);             // no trailing \n.  -20s-->-16s 1-31-94.
}                               // newsec
//====================================================================
LOCAL int update(              // replace old version of file with new if different, else just delete new.

	const char* old,	// Old file name.  On return, a file with this name remains.
	const char* nu )    // New file name.  Files must be closed.

// Purpose is to avoid giving a new date/time to output files which have not actually changed.
// returns 0 iff no errors else nz
{
	int ret = 0;
	if (xffilcmp(old, nu) != 0)
	{
		printf(" Updating %s. ", old);
		/*ret =*/ std::remove(old);	// ignore return
									// (common for file to not exist)
		if (!ret)
			ret = std::rename(nu, old);
	}
	else
	{
		printf(" %s unchanged. ", old);
		ret = std::remove(nu);
	}

	if (ret)
		// report error, prevent success at exit
		rcderr("Update fail: old=%s   new=%s", old, nu);

	return ret;
}                   // update
//======================================================================
LOCAL const char* enquote( const char *s)  // quote string (to Tmpstr)
{
	return strtprintf( "\"%s\"", s);    // result is transitory!
}               // enquote
//======================================================================


// rcdef.cpp end
