// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// RCDEF -- Record definer program.  Builds structure definitions,
//   include files, etc. required for record manipulation.

#include <CNGLOB.H>     // CSE global defines. ASSERT.

/*
 *  Much cleanup remains!
 *
 *  As of 9-18-91  M A N Y   O B S O L E T E   C O M M E N T S !!!
 *
 *	   note 5-89: if out of memory problems recurr:
 *	   try coding out the few remaining strsave's in records loop.
 *	   Actual dm use (Dmused) is only 82K for takeoff)
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
						example:  *array 2 SI pstat
						generates:  SI pstat[2];

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
#include <SRD.H>

#include <XIOPAK.H>     // xffilcomp
#include <ENVPAK.H>     // hello byebye
#include <LOOKUP.H>     // SWTABLE structure, looksw
#include <ANCREC.H>     // RFNAMED
#include <CUEVF.H>      // EVFHR EVFMH
#include "CVPAK.H"

/*-------------------------------- DEFINES --------------------------------*/
//  Note additional #definitions intermingled with declarations below

#ifndef RTBSUB  /* may be externally defined, eg in srd.h 12-91.
Bit of possible general interest currently only used in rcdef.cpp 12-91. */

#define RTBSUB   0x8000   /* Record type bit indicating Substructure type definition, only for nesting in other
record types or direct use in C code: Lacks record front info and stat bytes. 3-90.
rcdef.exe sets this bit for internal reasons, and leaves it set. */
#endif

#define REQUIRED_ARGS 10        // # required command line args not including command itself.
// 9-->10 for srd.cpp, rob 10-90.

/* #define BASECLASS	define to include code for for C++ *baseclass, 6-92; should be undefined if not C++.
						   coded out as #defined, 4-19-10
						   Initial use is for derived *substruct classes; is in use & works well 9-92.
						   Unresolved re derived *RAT's:
							1. Allocation of front members at proper offsets not verified.
							2. Space waste by duplicate sstat[] not prevented.  Does it make errors? */

const int MAXRCS=120;		// Record names.  80->100, 10-13
							// Also max rec handle (prob unnec??).
const int MAXFIELDS=200;	// Field type names.  Max 255 for UCH in srd.h,cul.cpp,exman 6-95.
							//   600-->200 1-92. ->130 5-95.   160->200 11-19
const int MAXDT=130;		// Data types.        160-->60 1-92. ->90 3-92. ->100 10-10 ->120 5-13 -> 130 4-18
const int MAXUN = 80;		// Units.              90-->60 1-92. ->80 3-92. ->60 5-95. ->80 7-13
const int MAXUNSYS=2;		// Unit systems. 5-->2 5-95.

const int MAXLM=12;			// Limits. 25->12 5-95.
const int MAXFDREC=600;		// Max fields in a record. Separated from MAXFIELDS, 4-92.


const int MAXDTH=600;	// max+1 data type handle. 800-->200 1-92 ->400 3-92. ->432(0x1b0) 2-94. ->352 (0x160) 5-95.
						//   352->400, 1-16; 400->500, 4-16; 500->600, 9-20
const int MAXDTC=90;	// maximum number of choices for choice data type.
//#define MAXARRAY  20	// largest number of record array structure members * NOT checked, but should be.
#define MAXNAMEL  40    // Max length of name, etc ("s" token)
#define MAXQSTRL 512    /* Max length for quoted string ("q" token).  assumed >= MAXNAMEL for array allocations.
		80->200 3-90 for huge struct data types eg MASSBC. ->128 5-95 when out of memory. */
const int MAXTOKS = 6;  // Maximum no. of tokens per action (size of arrays; max length format arg to gtoks). 10->6 10-92.
#define AVFDS   60      // Max average # fields / record, for Rcdtab alloc.  Reduce if MAXRCS*RCDSIZE(AVFDS) approaches 64K.
				// 25->40 3-92 after overflow with only 39 of 50 records. 40->50 10-92. ->40 5-95. ->50 7-95. */

// gtoks return values (fcn value and in gtokRet)
#define GTOK  0         // things seem ok
#define GTEOF 1         // unexpected EOF
#define GTEND 2         // first token was *END
#define GTERR 3         // conversion error or *END in pos other than first.  Error message has been issued.

// Predefined record types
#define RTNONE 0000     // may be put in dtypes.h from here, though as of 12-91 has no external uses.


/////////////////////////////////////////////////////////////////////////////////
// lupak: semi-obsolete lookup table manipulation
//    only uses are in rcdef.cpp, code included below 7-10
/////////////////////////////////////////////////////////////////////////////////
/*-------------------------------- DEFINES --------------------------------*/
#define LUHASHF 251	/* Hash divisor -- should be prime and less
					   than 256 given current data structure */
#define LUFAIL -1	/* Returned when a lookup fails */

/*--------------------------------- TYPES ---------------------------------*/
/* Lookup table structure */

#define LUTAB(n)  { \
	  SI lunent;  	/* # entries currently in table */ \
	  SI lumxent;  	/* Max # entries, excluding sentinel slot */ \
	  char **lunmp;	/* ptr to array of entry names */ \
	  char *stat;  	/* optional char array of status bytes (note belo) */ \
	  char luhash[n+1]; } /* Hash array */

struct LUTAB1
{   SI lunent;		/* Number of entries currently in table */
	SI lumxent;		/* Max # on entries allowed, excluding sentinel slot */
	char **lunmp;	/* Pointer to array of entry names */
	char *stat;		/* optional char array of status bytes */
	char luhash[1+1];	/* Hash array */
};
		/* stat: status array bytes, if present, are set 0 when
		   entry is added, and 1 when accessed (found by lusearch.) */

#define LUTABSZ(n) (sizeof(LUTAB1) + n)

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
/* note: must cast arg 1 to (LUTAB1 *) to avoid warnings.  5-31-89 */
static SI FC lufind( LUTAB1 * lutable, char * sym);
static SI FC luadd( LUTAB1 * lutable, char * sym);
//===========================================================================

/* ============================= VARIABLES ============================= */
/* more below, b4 record() */


/*------------ General variables ------------*/

char * ProgVrsnId = "RCDEF";    /* program version identifying string used in errorlog file header info, erpak2.cpp.
										   Defined in config.cpp in "real" products. */
FILE * Fpm;             /* current input stream, read by gtoks.  Note several files are opened
								   during command line checking; Fpm is set to each in sequence as they are used. */
FILE *Fout;             /* current output .hx file stream -- used locally several places. */
char* incdir=NULL;      // include file output path (from cmd line)
char* cFilesDir=NULL;   // cpp output file path (from cmd line)
SI Debug = FALSE;               // If TRUE, token stream is displayed (from gtoks() during execution.  Set by D on command line.
SI Errcount = 0;                // Number of errors so far

/*------- re Command Line --------*/

SI argNotNUL[ REQUIRED_ARGS+1]; /* non-0 if argv[i] not NULL;
								   0's for disabled fcns via following defines: */
#define HFILESOUT argNotNUL[6]          // true if outputting H files (arg 6 not NUL)
#define RCDBOUT   argNotNUL[7]          // if outputting recdef database
#define HELPCONV  argNotNUL[8]          // if checking or converting help references
#define HELPDUMMY argNotNUL[9]          // if outputting dummies for undef help
#define CFILESOUT  argNotNUL[10]        // if writing table .cpp source files

/*------- Filled by multiple-token-reader gtoks() --------*/

SI gtokRet = 0;                         // last gtoks call fcn return value
char Sval[MAXTOKS][MAXQSTRL];   // String values of tokens read (s, d, f formats), and deQuoted value of q tokens.
SI Dval[MAXTOKS];               // Decimal Integer values of tokens (d)
float Fval[MAXTOKS];            // Float values of tokens (f)

/*------- Common string buffer --------*/

// Used to preserve many strings.  presumed faster than malloc when dealloc not needed. See stash and stashSval functions.
#define STBUFSIZE 20000U                // 65534 or 65532 gets lint warning 538. 64000-->20000 5-95.
char Stbuf[STBUFSIZE] = { 0 };          // the buffer. not -- its big!
char * Stbp = Stbuf;            // points to last stored string

/*------------ Text snake ------------*/

/* a "snake" is a bunch of null-terminated strings, one after the other, with a header.
   Texts can be retrieved by SI offset.  Routines to manipulate snakes are in xxstrpak.cpp. */

static char* Cp4snake = NULL; // main recdef text snake, built in rcdef, now used only for internal storage in rcdef.exe 12-91.

/*------------- Data type global variables -------------*/

LI Dttab[MAXDTH+MAXDTC+1];
/* table of data type info, subscripted by data type.  Receives, for most types, SI length.
   for "choice" types (a hi bit flag in type define), also receives # choices and choice texts.
   Snake offset is used here for choice text, stored in sizeof(char *) 2-94 to match appl's struct
   re checking that data types don't collide.
   Used to write dttab.ppc for compilation and linking into application.
   While filling, next subscript is dttabsz.  May be used at runtime eg by lib\cvpak.cpp, 12-89 */
int dttabsz = 0;         // [next] slot in Dttab = next data type; is size of table (in SI's) at end.
// non-sparse data type arrays: subscripts are dtnmi[datatype].
char * dtnames[MAXDT];                  // data type names, set w luadd(dtlut..)
static struct LUTAB (MAXDT) dtlut = { 0, MAXDT, dtnames, NULL };
char * dtdecl[MAXDT];                           // Ptrs to decls for data type
char * dtcdecl[MAXDT][MAXDTC];          // Ptrs to decls for choice data type. not since large & data seg full.
char * dtxnm[MAXDT];                    // NULLs or ptrs to external data type names
char * dtmax[MAXDT];                    // max value of datatype, as string
SI dtsize[MAXDT];               // Size of data type in bytes
SI dttype[MAXDT];                       // Data type: bits plus Dttab subscript
SI ndtypes = 0;                 // number of data types
SI dtindex;             // temp for subscript in above arrays, retreived from dtnmi, used during records processing.
SI * dtnmi;     // memory table for data type indeces (subscrs of above arrays) by dt, as (choice) data types are sparse.


/*------------- Unit variables -------------*/
int Nunsys;							// # unit systems. may not work if not 2
UNIT Untab[MAXUN*sizeof(UNIT)];     // units table: print names, factors.  Decl must be same as appl's (srd.h).
int Unsysext = UNSYSIP;				// unit system currently in effect (so cvpak.cpp links)

const char* unsysnm[ MAXUNSYS];		// unit system names, w/o leading UNSYS-
int nuntypes = 0;                   // current number of unit types
char* unnames[MAXUN];				// unit type names: set via LUTAB unlut
const char* unSymTx[MAXUN][MAXUNSYS];	// units symbol text
const char* unFacTx[MAXUN][MAXUNSYS];   // unit factors for untab.cpp, AS TEXT to avoid conversion errors.
struct LUTAB (MAXUN) unlut = { 0, MAXUN, unnames, NULL };

/*------------- Limit variables -------------*/

// note there is no limits table.
static char * lmnames[MAXLM];                   // limit type names
static SI lmtype[MAXLM];
static struct LUTAB (MAXLM) lmlut = { 0, MAXLM, lmnames, NULL };
SI nlmtypes = 0;                                        // current number of limit types

/*------------- Field descriptor variables -------------*/

struct FIELDDESC
{
	SI dtype;           // make unsigned char? NO -- hi bits needed
	SI lmtype;          // make unsigned char?
	SI untype;          // make unsigned char?
};
FIELDDESC * Fdtab;      // field descriptors table

// Field name stuff
static char * rcfdnms[MAXFIELDS];                                       // Table of field name pointers
static char rdfas[MAXFIELDS];                                   // set 1 by lufind if used
static struct LUTAB (MAXFIELDS) fdlut= {0,MAXFIELDS,rcfdnms,rdfas};      // lupak fields table
SI nfdtypes = 0;                                                        // Current number of field names
USI fieldtype;

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
LOCAL void   base_class_fds( char *baseClass, SI &bRctype);
LOCAL void   rec_fds( void);
struct RCDstr;
LOCAL void   wrStr( FILE *rdf, char *name, SI arSz, struct RCD *rcdpin, USI evf, USI ff);
LOCAL void   nest( FILE *rcf, char *recTyNm, SI arSz, struct RCD *rcdpin, USI evf, USI ff, SI noname);
LOCAL void   wRcTy( FILE* f);
LOCAL void   wRcTd( FILE *f);
LOCAL void   wSrfd1( FILE *f);
LOCAL void   wSrfd2( FILE *f);
LOCAL void   wSrfd3( FILE *f);
LOCAL void   wSrfd4( FILE *f);
LOCAL void   sumry( void);
LOCAL FILE * rcfopen( char *, char **, SI);
LOCAL SI     gtoks( char * );
LOCAL void   rcderr( const char *s, ...);
LOCAL void   update( const char *, const char *);
LOCAL void   newsec( char *);
LOCAL char * stashSval( SI i);
LOCAL char * stash( char *s);
LOCAL const char* enquote( const char *s);
void CDEC ourByebye( int code);  // pointer passed -- no NEAR!

///////////////////////////////////////////////////////////////////////////////
// mtpak -- semi-obsolete memory table scheme
//    only uses are in rcdef.cpp, code included below 7-10
///////////////////////////////////////////////////////////////////////////////
struct MTU
{
	USI oldval;
	USI newval;
};
#define MTUCUR(pmtu) (pmtu)->oldval	/* next slot to use in MTU table:
Slot 0 .oldval contains # used slots; 1st user slot is slot 1.
Used 6-89 in points.c, ceilmu.c, dbxpak.c. */
LOCAL RC     FC mtpinit( char **pmtab, SI sltsize, SI nslots);
LOCAL char * FC mtinit( SI, SI);
LOCAL char * FC mtadd( char **, SI, char *);
LOCAL void   FC mtfree( char **);
LOCAL MTU *  FC mtuadd( MTU *, USI, USI);
LOCAL USI    FC mtulook( MTU *, USI);
LOCAL SI     FC mtulook2( MTU *, USI, USI *);
LOCAL USI    FC mturep( MTU *, USI, USI);
//===============================================================================



//////////////////////////////////////////////////////////////////////////////
// snake: a block of null-terminated strings, typically in dm,
// accessed using snake address and offset of particular string.
//////////////////////////////////////////////////////////////////////////////
struct SNAKE
{
	USI size;       /* allocated size of snake */
	USI tail;       /* position of end of snake: where to add */
	char text[1];   /* null terminated strings */
};
#define SNTAILINIT 4    /* initial offset of text */
static SI FC strsnfnd( char *snp, char *str);
//----------------------------------------------------------------------------
static SI FC strsndma(          /* add string to snake (if not there) in dm, return offset */

	char **snpp,        /* pointer to pointer to snake: may move. If points to NULL, snake will be initialized. */
	char *str )         /* string to add */

/* returns non-0 offset of added string */
{
#define SNAQUAN 32      /* power of 2 in which to quantize snake size */
	char *snp;
	USI snakesize, newsize, necessize, lstr;
	SI tail;            /* SI for return lint compatibility */

	snp = *snpp;                /* fetch snake location or NULL */

	/* Init if new, search for given string if old. */

	if (snp == NULL)            /* if does not exist yet */
	{
		snakesize = 0;
		tail = SNTAILINIT;       /* 4 -- bytes for .size and .tail at front. */
	}
	else                        /* snake exists */
	{
		tail = strsnfnd( snp, str);              /* see if str already there */
		if (tail)                                /* if offset nz */
			return tail;                         /* found, return */
		snakesize = ((SNAKE *)snp)->size;
		tail      = ((SNAKE *)snp)->tail;        /* where to store string */
	}

	/* Not already there.  Determine size needed */
	lstr = (SI)strlen(str) + 1;
	newsize = tail + lstr;

	/* create or enlarge snake if necessary */
	if (newsize > snakesize)
	{
		necessize = newsize;
		newsize = (newsize + SNAQUAN) & ~(SNAQUAN-1);   /* attempt to get multiple of 32 */
		if ( dmral( DMPP( snp), newsize,                /* create or enlarge, zero, copy */
					DMZERO|IGN) )                       /* no message if error */
		{
			newsize = necessize;                         /* try for needed space only */
			dmral( DMPP( snp), newsize, DMZERO|ABT);     /* on error, abort */
		}
		*snpp = snp;                    /* poss new location to caller */
		((SNAKE *)snp)->tail = tail;    /* for new case */
		((SNAKE *)snp)->size = newsize;
	}

	/* add string to snake */
	strcpy( snp+tail, str);             /* copy string to end of snake */
	((SNAKE *)snp)->tail += lstr;       /* point past */
	return tail;                /* return offset at which string added */
	/* another return above */
}               /* strsndma */
//------------------------------------------------------------------------------
static char * FC strsninit(             /* initializes a snake to a given size */

	USI size )          /* initial size (in bytes) for snake */

/* returns pointer to allocated snake */
{
	SNAKE *snake;

	dmal( DMPP( snake), size, DMZERO|ABT);
	snake->size = size;
	snake->tail = SNTAILINIT;
	return (char *)snake;
}               /* strsninit */
//-----------------------------------------------------------------------------
static SI FC strsnfnd(          /* find string in snake */
	char *snp,
	char *str )

/* returns offset in snake to str if successful, 0 if not found */
{
	SI ls, off;

	if (snp == NULL)
		return 0;
	ls = ((SNAKE *)snp)->tail;
	off = SNTAILINIT;
	while (off < ls)
	{
		if (!strcmp( str, snp+off))
			return off;
		off += (SI)strlen(snp+off) + 1;
	}
	return 0;
}               /* strsnfnd */
#ifdef STRDEBUG
//-----------------------------------------------------------------------------
static void FC strsndump(               /* dumps snake info */
	SNAKE *snp )
{
	SI ls, off, i;

	if (snp != NULL)
	{
		printf( "\nSnake size: %d   tail: %d", (INT)snp->size, (INT)snp->tail);
		ls = snp->tail;
		off = SNTAILINIT;
		i = 1;
		while (1)
		{
			printf( "\n\t%d. offset: %d   %s", INT(i++), (INT)off, (char *)snp + off);
			off += strlen((char *)snp+off) + 1;
			if (off >= ls)
				break;
		}
	}
}               /* strsndump */
#endif          /* STRDEBUG */

//////////////////////////////////////////////////////////////////////////////
// RCDEF MAIN ROUTINE
//////////////////////////////////////////////////////////////////////////////

int CDEC main( SI argc, char * argv[] )
{
	FILE *file_dtypes, *file_units, *file_limits, *file_fields, *file_records;
	/* .def files, opened at start */
	char fdtyphname[80];        /* data types file name */
	SI i;                       /* Working integer */
	char *s;
	int exitCode = 0;

	hello( ourByebye);	// initialize envpak

	// Startup announcement
	printf("\nR C D E F ");

	/* ************* Command Line, Arguments, Open Files *************** */

	// Check number of arguments
	if (argc <= REQUIRED_ARGS || argc > REQUIRED_ARGS+2)
	{
		printf("\nExactly %d or %d args are required\n",
			   (INT)REQUIRED_ARGS, INT(REQUIRED_ARGS+1) );
		exit(2);	// do nothing if args not all present
					//	(Note reserving errorlevel 1 for possible
					//	future alternate good exit, 12-89)
	}

	try
	{

	/* Test all args for NUL: inits macro "flags" HFILESOUT, HELPCONV, etc */
	for (i = 0; i <= REQUIRED_ARGS; i++)
		argNotNUL[i] = compareCaseStr(argv[i], "NUL");

	/* Get and check input file names from command line */
	file_dtypes  = rcfopen( "data types", argv, 1);
	file_units   = rcfopen( "units",      argv, 2);
	file_limits  = rcfopen( "limits",     argv, 3);
	file_fields  = rcfopen( "fields",     argv, 4);
	file_records = rcfopen( "records",    argv, 5);

	/* check include directory argument if specified */
	if (HFILESOUT)              /* if argv[6] not NUL */
	{
		incdir = argv[6];
		if (!filesys::exists(filesys::path(incdir)))
		{
			printf("\n'%s': Include file output directory path not found.\n", incdir);
			Errcount++;
		}
	}

	/* check cpp file directory argument if specified */
	if (CFILESOUT)              /* if argv[10] not NUL */
	{
		cFilesDir = argv[10];
		if (!filesys::exists(filesys::path(cFilesDir)))
		{
			printf("\n'%s': C++ file output directory path not found.\n", incdir);
			Errcount++;
		}
	}

	/* Option flags */

	if (argc == REQUIRED_ARGS + 2)      /* if another arg, it's options */
	{
		i = REQUIRED_ARGS+1;
		for (s = argv[i]; *s != 0; s++)  /* decode options */
			switch (*s)
			{
			case 'd':
			case 'D':
				Debug = TRUE;
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

	/* ************* Other init *************** */

	/* Initialize Cp4snake */
	Cp4snake = strsninit(15000);

	/* ************* Data types ************** */

	Fpm = file_dtypes;                  // Set input file for gtoks().  fopen() is in command line checking at beg of main
	FILE* fdtyph = NULL;
	if (HFILESOUT)                      // not if not outputting .h files
	{
		sprintf( fdtyphname, "%s/dtypes.hx",incdir);
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
		update( strtprintf( "%s/dtypes.h", incdir), fdtyphname);        // compare, replace file if different.
	}

	/* ************* SUMMARY ******************* */

// trim snake: old code, retain re sumry?
	dmral( DMPP( Cp4snake),                                             // shrink to size needed. 5-89 rob.
		   ((SNAKE *)Cp4snake)->size = ((SNAKE *)Cp4snake)->tail,
		   ABT );

// All done
leave:
// display and file summary
	sumry();                    // local fcn below. uses many globals.

// return to operating system
	byebye( Errcount + (Errcount!=0) ); // exit with errorlevel 0 if ok, 2 or more if errors
										// (exit(1) being reserved for poss future alternate good exit, 12-89).
										// 10-93: calls ourByebye (below) as set up by hello() call above. */
	}
	catch (int _exitCode)
	{
		exitCode = _exitCode;
	}
	return exitCode;
}           // main
//------------------------------------------------------------------------------------------
int getCpl( class TOPRAT** pTp /*=NULL*/)    // get chars/line (stub fcn, allows linking w/o full CSE runtime)
{
	pTp;
	return 78;
}
//======================================================================
LOCAL void dtypes(                      // do data types
	FILE* file_dtypesh)         // where to write dtypes.h[x]
{
	SI val, choff, choicb, choicn, nchoices, i;
	USI chan;
	char *cp;

	// global Fpm assumed preset to file_dtypes
	// global file fdtyph assumed open if HFILESOUT

	newsec("DATA TYPES");


// Init memory table that holds our sequential array subscripts for the sparse data types, for retreival later in rcdef.

	dtnmi = (SI *)mtinit( sizeof(SI), MAXDT);     // autoenlarges as necessary

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
			  LI MAKELONG( size, nChoices)		size in LOWORD, nChoices in HIWORD
			  char * choiceTexts[nChoices];  <-- Internally, this contains historical SI snake offsets to texts,
												 but sizeof(LI) is used to keep internal table size matching that
												 of application table whose source is written to dttab.cpp. */

	// 2-94 working on replacing size from cndtypes.def with sizeof(decl), so 32 bit size can differ from 16,
	// when succeeds can ignore size in def file or delete.

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
		choicb = choicn = 0;                             // not (yet) a choice data type
		if (*Sval[STK0] == '*')                             // is it "*choicb"?
		{
			if ( compareCaseStr(Sval[STK0] + 1, "choicb") == 0)
				choicb = 1;
			else if (compareCaseStr(Sval[STK0] + 1, "choicn") == 0)
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
		if (lufind( (LUTAB1*)&dtlut,                     // cast for lint
					Sval[STK0] ) != LUFAIL)                 // check for duplicate
		{
			rcderr("Duplicate data type '%s' omitted.", Sval[STK1]);
			continue;
		}

		// common processing of choice, var, non-var data types

		val = luadd( (LUTAB1*)&dtlut,            // cast to constant len for linting
					 stashSval( STK0) );		/* same typeName in nxt Stbuf space, enter ptr to saved name in tbl.
												   Sets dtnames[]. Subscript val is also used for other dt arrays */
		if (val==LUFAIL)
			rcderr("Too many data types.");
		mtadd( (char **)&dtnmi, dttabsz, (char *)&val);
		// subscript "val" to dtnmi memory table for retreival later by data type

		if (choicb || choicn)
		{

			// process a choice type.  choicb/n format: han *choicb/n <name> {  name "text"   name "text"  ...  }

			dtxnm[val] = NULL;                                   // no external name
			dtdecl[val] = choicn ? const_cast<char*>("float") : const_cast<char*>("SI"); // ... int-->SI 2-94 to keep 16 bits
			dtsize[val] = choicn ? sizeof(float) : sizeof(SI);
			dtmax[val]  = NULL;                                  // no max
			// 2-94 working on replacing size from cndtypes.def with sizeof(decl), when succeeds can probably ignore size in def file.
			dttype[val] = dttabsz                                // type: Dttab index, plus
						  | (choicn ? DTBCHOICN : DTBCHOICB);    //  appropriate choice bit
			if (gtoks("s"))                                      // gobble the {
				rcderr("choice data type { error.");

			// loop over choices list

			nchoices = 0;
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
				// NOTE: main and all aliases have same #define C_XXX_xxx

				choff = strsndma( &Cp4snake, Sval[ 1]);                    // choicb/n text to snake, get offset

				// choicb/n index: indeces 1,2,3 used.
				chan = nchoices + 1;                      // 1, 2, 3, ... here for error msg; regenerated in wChoices().

				int tyX = getChoiTxTyX( Sval[ 1]);
				if (tyX >= chtyALIAS)
						rcderr( "choicb/n '%s' -- main choice cannot be alias", Sval[ 0]);

				if (nchoices >= MAXDTC)
					rcderr( "Discarding choices in excess of %d.", MAXDTC);
				else
				{
					// check/fill choice's slot in Dttab entry
					i = dttabsz + 1 + nchoices;   // choice text's Dttab subscript, after LI size/#choices, previous choices
					ASSERT( sizeof(char *)==sizeof(LI));  // our assert macro, cnglob.h, 8-95.
					LI * pli = Dttab + i;
					if (*pli)
						rcderr(
							"Choice handle 0x%x for dtype %s apparently conflicts:\n"
							"    slot 0x%x already non-0.  Dttab will be bad.",
								chan, dtnames[val], i );
					*pli = choff;                 /* choicb/n text snake offset to Dttab:
												   used to retreive text to write Dttab.cpp source code. */
					nchoices++;                   // count choices for this data type
				}
			} // while (!gtoks("p"))  choices loop

			// set Dttab[masked dt] for choice type
			Dttab[dttabsz++] = (LI)(   ((ULI)(USI)nchoices << 16)                // HIWORD is # choices
									   | (choicn ? sizeof(float) : sizeof(SI)) ); // LOWORD is size (2 or 4)
			dttabsz += nchoices;                                     // point past choices, already stored in loop above.
		}                // if choicb/n ...
		else             // not a choice type
		{

			// get rest of non-choice data type input and process

			// 2-94 working on replacing size from cndtypes.def with sizeof(decl)
			// when succeeds can probably ignore size in def file.

			if (gtoks("qds"))                            // get decl-size-decl
				rcderr("Bad datatype definition");

			dtxnm[val] = cp;                             // NULL or external type text, saved above.
			dtdecl[val] = stashSval(0);                  // save decl text, set array
			if (compareCaseStr(Sval[2], "none"))
				dtmax[val] = stashSval(2);               // save max, set array
			else
				dtmax[val] = NULL;                       // no max given
			dtsize[val] = Dval[1];                       // size to array
			*(Dttab + dttabsz) = dtsize[val];            // size to Dttab
			dttype[val] = dttabsz++;                     // type: Dttab index

		}        // if choice ... else ...

		// end of data types loop

		ndtypes++;                                       // count data types
	}  // data types token while loop

	if (gtokRet != GTEND)                               // if 1st token not *END (gtoks at loop top)
		rcderr("Data types definitions do not end properly");

// Write data type definitions to dtypes.hx, choicb.hx, dttab.cpp

	if (HFILESOUT)                      // if outputting .h files
	{
		wdtypes( file_dtypesh);			// writes data types info. uses globals ndtypes, dtnames[], dtdecl[], dtxnm[],
										//    dttype[], dtmax[], dtcdecl[][].
		wChoices( file_dtypesh);        // write choice data types choice defines, below.
	}
	if (CFILESOUT)                      // if outputting .cpp files
	{
		printf("\n    ");
		wDttab();                        // write Dttab source code to compile and link into programs
	}

// Done with master definitions of data types

	fclose(Fpm);
	printf("   %d data types. ", (INT)ndtypes);

}               // dtypes
//======================================================================
LOCAL void wdtypes( FILE *f)

// write data types info to already-open dtypes.h file

// uses globals: dtnames[], dtdecl[], dtxnm[], dttype[], dtmax[], dtcdecl[][]
{
	SI i;

// introductory info

	fprintf( f,
			 "/* DTYPES.H: data types include file automatically generated by rcdef.exe. */\n"
			 "\n"
			 "   /* this file is #included in cnglob.h. */\n"
			 "\n"
			 "   /* do not edit this file -- it is written over any time rcdef is run --\n"
			 "      instead change the input, dtypes.def (also records.def), \n"
			 "      and re-run rcdef. */\n"
			 "\n"
			 "   /* see also CHOICB.H, for choice data type stuff */\n"
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
			 "   (see C_xxx defines in CHOICB.H).\n"
			 "\n"
			 "   For *choicn data types, the declaration is for a float to hold a \n"
			 "   numerical value or a NAN representing \"unset\" or the current selection.\n"
			 "   (see C_xxx defines in CHOICB.H, and cnblog.h NCHOICE and CHN macros).\n"
			 "\n"
			 "   (Structures for records (as specified in records.def) are not here, \n"
			 "   but in several RCxxxx.H files whose names are given in records.def). */\n"
			 "\n" );

	for (i = 0; i < ndtypes; i++)               // dt 0 unused 12-91
	{
		char *tail;
		SI nstLvl;
		/* parse declaration: put part starting with [ or ( not in {}'s after name,
		 so   typedef  char ANAME[16]  does not come out  typedef char[16] ANAME
		 and  typedef struct { float dircos[3]; } PLANE  still works */

		for (tail = dtdecl[i]; ; )
		{
			tail += strcspn( tail, "[({" );        /* # chars to [ ( { or end */
			if (*tail != '{')                     /* if not a {, */
				break;                            /* done: insert name here */
			for (nstLvl = 1; nstLvl > 0 && *tail; )  /* {: scan to matching } */
			{
				tail += 1+strcspn( tail+1, "{}");  /* find { } or end */
				if (*tail=='{')                   /* each { means must scan */
					nstLvl++;                     /* to another } before */
				else                              /* .. again looking for [ ( */
					nstLvl--;
			}
		}
		fprintf( f, "typedef %.*s %s%s;\n",
				 (SI)(tail-dtdecl[i]), dtdecl[i],  dtnames[i],  tail );
	}

// external defines

	fprintf( f,"\n\n\n"
			 "/* Data type extern declarations     names are  X<typeName> or as in dtypes.def\n"
			 "\n"
			 "   These define a single word for an external declaration for the type,\n"
			 "   for non-choice types. */\n\n" );

	for (i = 0; i < ndtypes; i++)
		if (dtxnm[i] != NULL)
			fprintf( f, "#define %s extern %s\n", dtxnm[i], dtnames[i]);

// data types

	fprintf( f, "\n\n\n/* Data type defines    names are  DT<typeName>\n"
			 "\n"
			 "   Values stored in the field information in a record descriptor (also for \n"
			 "   general use) consisting of Dttab offset plus bits (rcpak.h) as follows:\n"
			 "\n");
	fprintf( f,
			 "   DTBCHOICB (0x2000)  Multiple-choice, \"improved and simplified\" CN style\n"
			 "                       (see srd.h): takes one of several values given\n"
			 "                       in dtypes.def; values are stored as ints 1, 2, 3 ... ;\n"
			 "                       Symbols for values are defined in CHOICB.H.  1-91.\n"
			 "\n");
	fprintf( f,
			 "   DTBCHOICN (0x4000)  Number OR Multiple-choice value stored in a float.\n"
			 "                       Choice values are stored in the float as NANs: \n"
			 "                       hi word is 0x7f81, 0x7f82, 0x7f83 ....\n"
			 "                       Symbols for values are defined in CHOICB.H.  2-92.\n"
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

	fprintf( f, "#define DTNONE 0x0000         // supplied by rcdef (rob 12-1-91)\n");
	fprintf( f, "#define DTUNDEF 0xc000        // supplied by rcdef\n");
	fprintf( f, "#define DTNA 0xc001           // supplied by rcdef\n");
	for (i = 0; i < ndtypes; i++)
		fprintf( f, "#define DT%s %#04x\n", dtnames[i], dttype[i]);

// maxima

	fprintf( f,
			 "\n\n\n/* Data type MAX defines         names are  DT<typeName>MAX\n"
			 "\n"
			 "   Maximum absolute value for some types, for general use, optionally \n"
			 "   specifyable in dtypes.def. */\n\n" );

	for (i = 0; i < ndtypes; i++)
		if (dtmax[i] != NULL)
			fprintf( f, "#define DT%sMAX %s\n", dtnames[i], dtmax[i]);

}               // wdtypes
//----------------------------------------------------------------------
static const char* getChoiTxFromSnake(		// retrieve choice text
	int dt,			// choice DT
	int chan)		// choice idx (1 based)
{
	int dtm = dt & DTBMASK;
	LI * pli = Dttab + dtm;
	int offset = *(pli + chan);
	const char* chtx = Cp4snake + offset;
	return chtx;
}		// getChoiTxFromSnake
//======================================================================
LOCAL void wChoices(            // write choices info to dtypes.h file.
	FILE* f)                    // where to write

// call only if HFILESOUT.
{
	SI i, n, j;

//----------- CHOICB.H: choice data types choice defines
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

	for (i = 0; i < ndtypes; i++)
		if (dttype[i] & DTBCHOICB)
		{
			n = SI( ULI(Dttab[dttype[i]&DTBMASK]) >> 16 );      // number of choices for data type from hi word of Dttab[dt]
			int iCh = 1;
			for (j = 0; j < n; j++)
			{	const char* chtx = getChoiTxFromSnake( dttype[ i], j+1);
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

	for (i = 0; i < ndtypes; i++)
		if (dttype[i] & DTBCHOICN)
		{	n = SI( ULI(Dttab[dttype[i]&DTBMASK]) >> 16 );      // number of choices for data type from hi word of Dttab[dt]
			for (j = 0; j < n; j++)
			{	const char* chtx = getChoiTxFromSnake( dttype[ i], j+1);
				fprintf( f, "#define C_%s 0x%x\n",
					 strtcat( dtnames[i], "_", dtcdecl[i][j], NULL),
					 UI(j + NCNAN + 1) );                   // <-- choicn difference.  NCNAN: 7f80, cnglob.h.
			}
		}

	fprintf( f, "\n// end of choice definitions\n");
}       // wChoices
//======================================================================
LOCAL void wDttab()     // write C++ source data types table dttab.cpp
{
	int i, j, n;
	USI w, dtm;
	char buf[80], temp[80];
	FILE *f;

// open working file dttab.cx
	sprintf( buf, "%s\\dttab.cx", cFilesDir);                   // buf also used to close
	f = fopen( buf, "w");
	if (f==NULL)
	{
		rcderr( "error opening '%s'", buf );
		return;
	}

// write front of file stuff
	fprintf( f,
			 "/* DTTAB.CPP: data types table source file automatically generated by rcdef. */\n"
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
			 "LI Dttab[] =\n"
			 "{ /* size   #choices if choice type\n"
			 "                choice texts if choice type   // type (Dttab subscript + bits) & symbol (dtypes.h) */\n");

// write content of data types table
//   sizeof(SI),					// DTSI
//   ML(sizeof(SI),2), (LI)"yes",
//		       (LI)"no",            // DTNOYES
	w = 0;                                              // counts Dttab array members written
	for (i = 0; i < ndtypes; i++)                       // loop data types
	{
		dtm = dttype[i] & DTBMASK;                       // mask type for Dttab subscript

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
		LI* pli = Dttab + dtm;							// point Dttab entry for type
		if (!(dttype[i] & (DTBCHOICB|DTBCHOICN)))		// if not a choice
		{
			// non-choice type: size (in LI) is entire entry.
			fprintf( f, "    sizeof(%s), ", dtnames[i]);         // write size: let compiler evaluate sizeof(type)
			fprintf( f, "\t\t\t");                       // space over
			pli++;                                       // point past non-choice Dttab entry
			w++;                                         // one LI (size) written
		}
		else                                             // is a choice type
		{
			// choice type: start entry with ML(sizeof(decl),#choices)
			n = SI( (ULI(*pli) >> 16) );                 // fetch number of choices from hi word of Dttab[masked dt]
			pli++;                                       // point past size/# choices
			fprintf( f, "    ML( sizeof(%s), %d), ",
					 dtnames[i],                         // write declaration into sizeof, let compiler determine 16/32 bit size
					 n );                                // # choices comes from and goes to HIWORD of LI
			// choices
			for (j = 0; j < n; j++)                      // loop choices
			{
				const char* chtx = getChoiTxFromSnake( dttype[ i], j+1);
#if 0
x				int offset = *pli;
x				const char* chtxx = Cp4snake + (SI)*pli++;	// get choicb text
x				if (chtx != chtxx)
x					printf( "Vom\n");
#endif
				fprintf( f, "\n\t\t(LI)%-13s\t",			// write a choice text
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
			 "\n\n// size of data types table (in LI's)\n"
			 "\nUSI Dttmax = %d;\n", (INT)dttabsz );

// terminate file, close, update if different
	fprintf( f, "\n/* end of dttab.cpp */\n");
	fclose(f);
	sprintf( temp, "%s\\dttab.cpp", cFilesDir);
	update( temp, buf);                         // compare file, replace if different

}               // wDttab()
//======================================================================
LOCAL void units(       // do units types, for rcdef main()
	FILE* file_unitsh)                  // where to write .h info
{
	SI val, i;

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

	for (i = 0; i < Nunsys; i++)
	{
		gtoks("s");                      /* read unit system name */
		unsysnm[i] = stash( stringUpperCase(Sval[0]) );
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
		if (lufind( (LUTAB1*)&unlut,    /* cast (LUTAB1*) is just for lint */
					Sval[0] ) != LUFAIL) /* check if type in table */
			rcderr("Duplicate unit type.");
		else
		{
			/* process unit typeName */

			val = luadd( (LUTAB1*)&unlut,
						 stashSval(0) ); /* save typeName text in Stbuf,
										   enter ptr in table, set unnames[val].
										   Subscript val is used as type #. */
			if (val == LUFAIL)
			{
				rcderr("Too many unit types.");
				continue;
			}

			/* unit systems info: print name, factor */

			for (i = 0; i < Nunsys; i++)
			{
				gtoks("qf");                                    // read print name, factor
				unSymTx[val][i] = stash( Sval[0]);
				unFacTx[val][i] = stash( Sval[1]);              // save factor TEXT for untab.cpp
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
	printf(" %d units.", (INT)nuntypes);
}                                       /* units */
//======================================================================
LOCAL void wUnits(              // write units info to units.h if different
	FILE* f)            // where to write

// uses Nunsys, unsysnm[], nuntypes, unnames[], incdir.
{
	SI i;

	/* head of file comments */
	fprintf( f,
			 "\n\n/* units definitions automatically generated by rcdef.exe. */\n");

	/* unit systems info */
	fprintf(f,"\n\n/* Unit systems */\n");
	for (i = 0; i < Nunsys; i++)
		fprintf( f, "#define UNSYS%s %d\n", unsysnm[i], (INT)i);

	/* unit types info */
	fprintf( f, "\n\n"
			 "/* Unit types					  (comments written 10-4-88)\n"
			 "\n"
			 "   These are for .untype for fields (srd.h) as well as general use.\n"
			 "   They are subscripts into Untab (untab.cpp); also generated by rcdef.\n"
			 "   Untab is of type UNIT (srd.h) and contains a text and a conversion \n"
			 "   factor for each of the 2 unit systems. */\n\n" );

	for (i = 0; i < nuntypes; i++)
		fprintf(f,"#define %s %d\n", unnames[i], (INT)i);
	fprintf( f, "\n// end of units definitions\n");
}                       /* wUnits */
//======================================================================
LOCAL void wUntab()                     // write untab.cpp
{
	SI i, j;
	char buf[80], temp[80];
	FILE *f;

// open working file untab.cx
	sprintf( buf, "%s\\untab.cx", cFilesDir);   // buf also used to close
	f = fopen( buf, "w");
	if (f==NULL)
	{
		rcderr( "error opening '%s'", buf );
		return;
	}

// head of file comments
	fprintf( f,
			 "/* UNTAB.CPP: units table source file automatically generated by rcdef.exe. */\n"
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
			 "const char* UnsysNames[] = { ", (INT)Nunsys );
	for (i = 0; i < Nunsys; i++)
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

	for (i = 0; i < nuntypes; i++)
	{
		fprintf( f, "  { {");
		for (j = 0; j < Nunsys; j++)
			fprintf( f, j < Nunsys-1 ? " {%20s,%12s },\t" : "{%20s,%12s }",
					 enquote( unSymTx[i][j]),			// symbol text
					 unFacTx[i][j] );                                            // factor text
		fprintf( f, " } }, \t// %s\n", unnames[i]);
	}
	fprintf( f, "};	// Untab[]\n");

	/* terminate file, close, update if different */
	fprintf( f, "\n/* end of untab.cpp */\n");
	fclose(f);
	sprintf( temp, "%s\\untab.cpp", cFilesDir);
	update( temp, buf);                 // compare, replace if different
}                       // wUntab
//======================================================================
LOCAL void limits( FILE* file_limitsh)  // do limit types
{
	SI val;

	newsec("LIMITS");

	/* read limits info from def file */

	nlmtypes = 0;
	while (gtoks("s")==GTOK)                    // read typeName.  Sets gtokRet same as return value.
	{
		if (lufind( (LUTAB1*)&lmlut, Sval[0]) != LUFAIL)
			rcderr("Duplicate limit type.");
		else
		{
			/* enter in table.  Sets lmnames[val]. */
			val = luadd( (LUTAB1*)&lmlut, stash( Sval[0]) );
			if (val==LUFAIL)
				rcderr("Too many limit types.");
			lmtype[val] = val;                  // save type in array parallel to lmnames
		}
		nlmtypes++;
	}

	if (gtokRet != GTEND)                       // unless *END gtok'd last
		rcderr("Limits definitions do not end properly");

	/* Write limit definitions to .h file */

	if (HFILESOUT)              // not if not outputting .H files this run
		wLimits( file_limitsh);         // write .h, uses nlmtypes, lmnames[], lmtype[], incdir.

	printf(" %d limits. ", (INT)nlmtypes);

// Note: there is no limits table, thus no limits table C++ source file.
}                                       // limits
//======================================================================
LOCAL void wLimits( FILE* f)            // write to .h file

// uses nlmtypes,lmnames[],lmtype[],incdir.
{
	SI i;

	fprintf( f,
			 "\n\n// Limit definitions\n"
			 "//   These specify the type of limit checking, e.g. for cvpak.cpp functions.\n"
			 "//   There is no limits table.\n\n" );

	for (i = 0; i < nlmtypes; i++)
		fprintf( f, "#define %s %d\n", lmnames[i], (INT)lmtype[i]);
	fprintf( f, "\n// end of limits\n");
}               // wLimits
//======================================================================
LOCAL void fields()     // do fields
{
	SI val;

	newsec("FIELD DESCRIPTORS");

// Allocate for field table
	dmal( DMPP( Fdtab), (USI)MAXFIELDS*sizeof(FIELDDESC), DMZERO|ABT);

// make dummy 0th table entry to reserve field type 0 for "FDNONE", 2-1-91
	static char noneStr[]= { "none" };
	val = luadd( (LUTAB1*)&fdlut, noneStr );
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
		if (lufind( (LUTAB1*)&fdlut, Sval[0]) != LUFAIL)
		{
			rcderr("Duplicate field name attempted.");
			continue;
		}
		val = luadd( (LUTAB1*)&fdlut, stashSval(0) );   /* typeName to Stbuf, ptr to lookup table.  Sets rcfdnms[val].
														   Subscript val is used on parallel arrays and as field type. */
		if (val==LUFAIL)
		{
			rcderr("Too many field names.");
			continue;
		}
		if (lufind( (LUTAB1*)&dtlut, Sval[1]) == LUFAIL)
		{
			printf("\nWarning: cannot find data type %s",(char *)Sval[1]);
			rcderr("");
		}
		if (lufind( (LUTAB1*)&lmlut, Sval[2]) == LUFAIL)
		{
			printf("\nWarning: cannot find limit type %s",(char *)Sval[2]);
			rcderr("");
		}
		if (lufind( (LUTAB1*)&unlut,    /* look in table. cast is for lint. */
					Sval[3]) == LUFAIL)
		{
			printf("\nWarning: cannot find unit type %s", (char*)Sval[3]);
			rcderr("");
		}
		(Fdtab+val)->dtype = dttype[ lufind( (LUTAB1*)&dtlut, Sval[1]) ];
		(Fdtab+val)->lmtype = lmtype[ lufind( (LUTAB1*)&lmlut, Sval[2]) ];
		(Fdtab+val)->untype = lufind( (LUTAB1*)&unlut, Sval[3]);
		nfdtypes++;
	}
	if (gtokRet != GTEND)                               // if gtoks didn't read *END
		rcderr("Fields definitions do not end properly");

// CSE: Fdtab remains in DM for access while doing records

	printf(" %d field descriptors. ", (INT)nfdtypes);
}                                                       // fields

/*================ MORE VARIABLES global to following fcns only ===========*/

/*------------- Record Descriptor variables (and schedules) --------------*/

/* Field descriptor substructure of record descriptor */
struct rcfdd
{
	SI rcfdnm;          // Index to FIELDDESC in Fdtab
	USI evf;            // field variation bits (eval frequency for expr using filed)
	USI ff;             // field flags (attributes), defines in srd.h: FFHIDE, FFBASE.
};

#undef MBRNAMESX
#if defined( MBRNAMESX)
// idea: consolidated retention of member names
//   not complete, 4-2-2012
// structure to retain member names
struct MBRNM
{	char mn_nm[ MAXNAMEL+1];			// member name as input
	char mn_nmNoPfx[ MAXNAMEL+1];	// member name w/o prefix
	int mn_fnr;				// member field number
	MBRNM();
	void mn_Set( const char* nm, int fnr);
	MBRNM& mn_Copy( const MBRNM& s, int fnr=-1);
};
#endif

// Record descriptor, RCD.  For **Rcdtab (below); now an internal rcdef variable 12-91
struct RCD
{
	SI rcdtype;         // Record type being described by this descriptor, w bits: RTxxxx values as defined in dtypes.h.
	SI rcdrsize;        // Size of record fields (excludes base class; only for odd/even)
	SI rcdnfds;         // Number of fields in record or RAT entry
	SI rNameSnx;        // 0 or record name (description) snake index (into Cp4snake).  Same text is also in RATBASE.what.
	SI rd_nstrel;       // number of structure elements (things with field # defines)
	char** rd_strelNm;  // name of each structure element: dm array of char ptrs
	char** rd_strelNmNoPfx;  // ditto w/o prefix
	SI* rd_strelFnr;       // field number (rcdfdd subscript) for each structure element: dm array
#if defined( MBRNAMESX)
	MBRNM rd_mbrNames[ MAXFDREC];	// name etc of each structure element (things with field # defines)
#endif
	struct rcfdd rcdfdd[1];     // Field descriptors
	RCD()
	{ }
};
#define RCDSIZE(n) (sizeof (RCD) + (n-1)*(sizeof (struct rcfdd)))       // size of record descriptor for n fields

RCD** Rcdtab;           // ptr to dm block in which descriptors of all record types are built.
						//   Contains ptrs[] by rctype, then RCD's (descriptors). Ptrs are alloc'd RCD*,
						//   so program can make absolute, but contain cast USI offset from start block only.
						//   Set/used in recs(); public for poss lib linking.
static char* rcnms[MAXRCS];			// rec names (typeNames) (part of rclut).  set: recs. used: rec_fds wRcTd wRcTy.
static struct LUTAB (MAXRCS)		// lookup table for record names. Note luadd to this sets rcnms[].
  rclut = { 0, MAXRCS, rcnms, NULL };             // ... set: recs(). used: rec_fds wrStr
int rctypes[ MAXRCS];           // rec types, w bits, by rec sequence number (current one in 'rctype')
char* recIncFile[ MAXRCS];		// Include file name for each record type
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
char* rcNam=NULL;		// curr rec name -- copy of rcnms[rcseq]. Used in error messages, etc.

static SWTABLE rdirtab[] =      /* table of record level * directives:
										   Content is record type bit or other value tested in switch in records loop */
{
	{ "rat",       RTBRAT     },        // Record Array Table record type
	{ "substruct", (SI)RTBSUB },        // substructure-only "record" definition
	{ "hideall",   RTBHIDE    },        // omit entire record from probe names help (CSE -p)
	{ "baseclass", 32764      },        // *baseclass <name>: C++ base class for this record type
	{ "excon",     32765      },        // external constructor: declare only (else inline code may be supplied)
	{ "exdes",     32766      },        // external destructor: declare only in class definition
	{ "ovrcopy",   32763      },        // overridden Copy()
	{ "prefix",	   32761	  },		// *prefix xx  drop xx from field # definitions and SFIR.mnames (but not from C declaration)
	{ NULL,        32767      }         // 32767 returned if not in table
};                      // rdirtab[]

static SWTABLE fdirtab[] =      /* table of field level * directives.  Content word is:
										0x8000 + SFIR.ff bit, or
										SFIR.evf bit, or
										value for which switch in fields loop has a case. */
{
// field flags, for SFIR.ff
	{ "hide",   (SI)0x8000 | FFHIDE },  // hide field: omit field from probe names help (CSE -p)

// variations: how often program changes field, for SFIR.evf
	// input rat members set during input checking should be encoded at least *r to prevent input time probes.
	// User input members may be encoded *z or with no *variation (see cncult tables for acceptable input expr variability).
	// run rat members with no encoded variation are treated as *r.
	{ "z",      0        },             // indicates no variation (rather than member not encoded yet): doct'n aid. optional.
	{ "i",      EVEOI    },             /* *i available after input, b4 setup (rat reference subscripts, resolved b4 topCkf, 1991;
											  members accepting end-of-input variable expr (VEOI), mainly doc aid) */
#ifdef AUTOSIZE //cnglob.h
	{ "f",      EVFFAZ   },             // *f may change after input (b4 setup) and again b4 main run setup after autosize. 6-95.
#endif
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
	{ "declare",  32762  },             // *declare "whatever"   spit text thru into record class definition immediately
	{ "noname",   32763  },             // with *nest, omit aggregate name from SFIR.mnames (but not from C declaration)
	{ "nest",     32764  },             // nest <name>: imbed structure of another rec type
	{ "struct",   32765  },             // struct name n { ...
	{ "array",    32766  },             // array n
	{ NULL,       32767  }      // 32767 returned if not in table
};                      // fdirtab[]

// include file for record info output
char* rchFileNm = NULL;        /* Name of current record output include file, for recIncFile[] and for use in file text.
								   Notes: name with x is also in dbuff[] for closing/renaming.
									FILE is "frc" in main(), set only if HFILESOUT. */
// addl record vbls used by recs() and new callees rec_swl, rec_fds,
FILE * frc = NULL;      // current rcxxx output file if HFILESOUT
SI nstrel;                      // subscr of struct members (elts) for fld
SI strelFnr[MAXFDREC];          // field numbers for structure elts, this rec.  struct elts include those of base class.
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
	{	int lPfx = (int)rcPrefix[ rcseq].length();
		if (t.substr(0, lPfx) == rcPrefix[ rcseq].c_str())
			t = t.substr( lPfx);
	}

	if (options & enfxUPPER)
		WStrUpper( t);

	return t;
}	// ElNmFix
//-----------------------------------------------------------------------
#define STRELSAVE		// #define to use strelSave() (drops inadvertent ';'s) 4-12
#if defined( STRELSAVE)
static char* fixName( char* p)		// fix name *in place*
{
	// trim off trailing ';' (common typo) 4-12
	int len = strlen( p);
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
	SI fnr)				// field number
{
	char* p = strsave( nm);	// save member name (dmfree'd: strsave better than stash).
	fixName( p);
	strelNm[ nstrel] = p;
	strelNmNoPfx[ nstrel] = strsave( ElNmFix( p, enfxNOPREFIX).c_str() );
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
#endif
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
	char *argv[],
	FILE* file_dtypeh)                  // open file dtypes.hx

// returns RCOK ok; RCBAD for some error exit cases
{
	char dbuff[80];     // filename .hx */
	FILE *fSrfd = NULL; // small frd output FILE if CFILESOUT, else NULL
	char fsrfdName[80]; // pathname.cx for small frd output file
	/*--- some local record descriptor variables ---*/
	char * rcdend;      // end of Rcdtab allocation


	nrcnms = 0;         // init number of record names (total/max rcseq to output to dtypes.h; and for sumry)

	/* predefined/reserved record type number */

	static char noneStr[] = { "NONE" };
	if (luadd( (LUTAB1*)&rclut, noneStr) != RTNONE)     // RTNONE must be 0th entry (historical)
		rcderr("Warning: RTNONE not 0");
	rctypes[0] = RTNONE;                        // type value for h file
	nrcnms++;                                   // count types for writing rec types


	/* ************* Records defined in def file ************ */

	/*--- additional init ---*/

	/* nrcnms init above: RTNONE and any dummies already added to it */
	MaxNfields = 0;                     // max # fields in a record
	/* many more variables init in their declarations. */

	/* open and start "small record & field descriptor" output .cpp file */
	if (CFILESOUT)                                      // if outputting tables to compile & link
	{
		sprintf( fsrfdName, "%s\\srfd.cx", cFilesDir);   // also used below
		fSrfd = fopen( fsrfdName, "wt");
		if (fSrfd==NULL)
			printf( "\n\nCannot open srfd output file '%s'\n", fsrfdName );
		else                             // opened ok
			wSrfd1( fSrfd);              // write part b4 per-record portion, below
	}

	/* initialize data base block for record descriptors */
	ULI uliTem =                                // size for Rcdtab (reduced later), compute long to check for exceeding 64K 10-92
		(ULI)MAXRCS*sizeof(RCD**)      // pointers to max # records
		+ MAXRCS*RCDSIZE(AVFDS)        // + max # records with average fields each
		+ RCDSIZE(MAXFDREC);           // + one record with max # fields
	dmal( DMPP( Rcdtab), uliTem, DMZERO|ABT);      // alloc record descriptor table in heap
	rcdend = (char *)Rcdtab + uliTem;              // end of allocated Rcdtab space
	rcdesc =                                    // init pointer for RCD creation
		(RCD*)((char **)Rcdtab + MAXRCS);  // into Rcdtab, after space for pointers

	/*--- set up to do records ---*/

	newsec("RECORDS");
	frc = NULL;                         // no rcxxxx.h output file open yet (FILE)
	rchFileNm = NULL;                   // .. (name)
	dbuff[0] = '\0';                    // working file name: init for Lint

	/*--- decode records info in def file ---*/

	/* top of records loop.  Process a between-records *word or a RECORD. */

	gtoks("s");                         // read token, set Sval and gtokRet
	while (gtokRet==GTOK)               // until *END or (unexpected) eof or error.  Sval[0] shd be "*file" or "RECORD".
	{
		SI excon = 0;           // non-0 if record has explicit external constructor ..
		SI exdes = 0;           // .. destructor ..  (declare only in class defn output here; omit default code)
		SI ovrcopy = 0;         // .. if has overridden Copy()
		char *baseClass = "record";             // current record's base class, default "record" if not specified by *BASECLASS
		BOO baseGiven = FALSE;                  // true if base class specified with *BASECLASS
		int i;

		if (nrcnms >= MAXRCS)                   // prevent table overflow
		{
			rcderr("Too many record types.");    // fallthru gobbles *END / eof w msg for each token.
nexTokRec: ;                            // come here after *word or error */
			if (gtoks("s"))                      // get token to replace that used
				rcderr("Error in records.def.");
			continue;                            // repeat record loop. gtokRet set.
		}
		if ((char *)rcdesc + RCDSIZE(MAXFDREC) >= rcdend)       // room for max rec recDescriptor?
		{
			rcderr("Record descriptor storage full.");           // if this occurs, recompile with larger MAXFDREC and/or AVFDS
			goto nexTokRec;                                      // get token and reiterate record loop
		}

		/* process *file <name> statement before next record if present */

		if (compareCaseStr(Sval[0],"*file") == 0)              // if *file
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
			sprintf( dbuff, "%s/%sx", incdir, rchFileNm);
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

		if (compareCaseStr(Sval[0],"RECORD") != 0)
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
					(UI)nextRcType, (UI)MAXRCS-1 );
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
		if (lufind( (LUTAB1*)&rclut, Sval[0]) != LUFAIL)
		{
			rcderr("Duplicate record name attempted.");
			goto nexTokRec;                        // get token and reiterate record loop
		}

		/* put record name in table used for dupl check just above, get
		   sequence # (our array subscript).  Stores name in rcnms[rcseq]. */
		if (LUFAIL == luadd( (LUTAB1*)&rclut,	// cast for lint
							stashSval(0) ))		// save typeName, put in table, get subscript for parallel tbls.
		{
			rcderr("Too many record types.");
			goto nexTokRec;                                // get token and reiterate record loop
		}
		rcNam = rcnms[ rcseq];                  // get name ptr from where luadd put it, for use in many error messages

#if 0
x		// trap record name for debugging
x		static const char* trapRec = "AIRNET";
x		if (!compareCaseStr( rcNam, trapRec))
x		{    printf( "\nRecord trap!");}
#endif

		// record sequence number returned by luadd is used internally to index parallel arrays,
		// making it possible to loop over all record definitions

		// now we have record sequence number (array subscript) */
		for (i = 0; i < rcseq; i++)                             // dupl type check
			if ( ( (rctypes[i] ^ rctype) & RCTMASK) ==0)
				rcderr( "Duplicate record type 0x%x.", (UI)rctype );

		recIncFile[ rcseq] = rchFileNm;         // store rc file name for output later in comment in typdef in rctpes.h

		/* record description/title text (".what" name, used in probes and error messages) */
		if (gtoks("q") != GTOK)
		{
			rcderr("Record description text problem.");
			rcdesc->rNameSnx = 0;
		}
		else    // description ok.  Put in snake, offset to recdesc
		{
			rcdesc->rNameSnx =             // put offset in RCD
				strsndma( &Cp4snake,	// add to snake, ret offset
						  Sval[0]);		// text to add
		}

		/* process record level * directives */

		while (gtoks("s")==GTOK         // while next token (sets gtokRet) is ok
				&& *Sval[0]=='*')        // ... and starts with '*'
		{
			USI val = (USI)looksw( Sval[0]+1, rdirtab);    // look up word after * in table, rets special value
			if (val==32767)                        // if not found
				break;								// leave token for fld loop.
			else switch ( val)                      // special value from rdirtab
			{
				default:                   // in table but not in switch
					rcderr("Bug re record level * directive: '%s'.", Sval[0]);
					break;                         // nb fallthru gets token

				case 32764:        // *baseclass <name>: C++ base class for this record type
					if (baseGiven)
						rcderr("record type '%s': only one *BASECLASS per record", rcNam);
					if (gtoks("s"))
						rcderr("Class name missing after '*BASECLASS' after '%s'", rcNam);
					baseClass = stash(Sval[0]);
					baseGiven = TRUE;
					break;
				case 32765:
					excon++;                       // class has explicit external constructor: declare only in class defn
					break;
				case 32766:
					exdes++;                       // .. destructor
					break;
				case 32763:
					ovrcopy++;                     // class has overridden Copy()
					break;
				case 32761:		// *prefix xx -- specify member name prefix
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
				//   %s( basAnc *b, TI i, SI noZ=0)  :  record( b, i, noZ)  {}	<-- not anc<rcNam>: uses template b4 class rcNam defined.
				//   virtual ~%s() {}						(compiler supplies call to record::~record)
				if (excon)                                                              // *excon
					fprintf( frc, "    %s( basAnc *b, TI i, SI noZ=0);\n",  rcNam );    // just declare external constructor
				else
					fprintf( frc, "    %s( basAnc *b, TI i, SI noZ=0)  :  %s( b, i, noZ)  {} \n",  rcNam, baseClass );
				if (exdes)
					fprintf( frc, "    virtual ~%s(); \n",   rcNam );                   // *exdes: declare external destructor
				else
					fprintf( frc, "    virtual ~%s() {} \n",  rcNam );

				// standard type-specific members: Copy() and operator=()
				fprintf( frc, "    %s& Copy( const %s& d) { Copy( static_cast< const record*>(&d)); return *this; }\n", rcNam, rcNam);
				fprintf( frc, "    %s& operator=( const %s& d) { Copy( static_cast< const record*>(&d)); return *this; }\n", rcNam, rcNam);

				// virtual Copy(): override or not
				fprintf( frc, "    virtual void Copy( const record* pSrc, int options=0)%s;\n",
						 ovrcopy ? "" : " { record::Copy( pSrc, options); }");
				fprintf( frc, "\n");


			}
			else                                        // former 'normal' record */
				rcderr( "Record neither *RAT nor *SUBSTRUCT: no longer supported");
		}

		// get base class fields info for this record
		SI bRctype = 0;                         // receives rctype of base class if *BASECLASS
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
						 INT(max( Nfields+(Nfields+Fdoff)%2, 1)) );     // sstat[] size: # fields + 1 if struct len odd
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
						 rcNam, (INT)Nfields );

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
					fprintf( frc, "#define  makAnc%s(name)        anc<%s> name( \"%s\", sfir%s, %s_NFIELDS, RT%s) \n", rcNam, rcNam, Cp4snake + rcdesc->rNameSnx, rcNam, rcNam, rcNam );
					fprintf( frc, "#define  makAnc%s2(name,what)  anc<%s> name( what, sfir%s, %s_NFIELDS, RT%s)\t// 'what' override\n", rcNam, rcNam, rcNam, rcNam, rcNam );
				}
			}

			// dealloc or save full field name texts 3-5-91

			// save structure element info in case record is used as a base class 7-92
			rcdesc->rd_nstrel = nstrel;
#if !defined( MBRNAMESX)
			dmal( DMPP( rcdesc->rd_strelFnr), nstrel*sizeof(SI), ABT);
			memcpy( rcdesc->rd_strelFnr, strelFnr, nstrel*sizeof(SI));
			dmal( DMPP( rcdesc->rd_strelNm), nstrel*sizeof(char *), ABT);
			memcpy( rcdesc->rd_strelNm, strelNm, nstrel*sizeof(char *));
			dmal( DMPP( rcdesc->rd_strelNmNoPfx), nstrel*sizeof(char *), ABT);
			memcpy( rcdesc->rd_strelNmNoPfx, strelNmNoPfx, nstrel*sizeof(char *));
#else
			for (i=0; i<nstrel; i++)
				rcdesc->rd_mbrNames[ i].mn_Copy( mbrNames[ i]);
#endif

			// set relative pointer to descriptor block in table, point next space
			*(Rcdtab + (rcseq)) =
				(RCD*)(ULI)(USI)
				((char *)rcdesc - (char *)Rcdtab);
			/* pointers are at front of block containing all descriptors (RCD's).  Cast the offset in block
			   into a pointer type: use of pointer type leaves room so program can make absolute. */
			/*lint -e63  no msg for left cast */
			IncP( DMPP( rcdesc), RCDSIZE(Nfields));     // where next RCD will go, or end Rcdtab if last one
			/*lint +e63 */


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
		wSrfd4( fSrfd);                                  // write part after per-record stuff, below
		fprintf( fSrfd, "\n\n/* end of srfd.cpp */\n" );
		fclose(fSrfd);
		printf("    \n");
		update( strtprintf( "%s\\srfd.cpp",cFilesDir), fsrfdName);       // compare new include file to old, replace if different
	}

// create file containing record type definitions (written to dtypes.h 4-5-10)

	if (HFILESOUT)              // if out'ing .H files
		wRcTy( file_dtypeh);                    // conditionally write file.  Uses nrcnms, rcnms[], rctypes[], recIncFile[], incdir.

	printf("  %d records. ", (INT)nrcnms);

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
		char *name;
		char *fdTyNam;
		USI evf;
		USI ff;
	} baseFields[] =
	{
		//  name      fdTyNam    efv      ff
		//  --------  ---------  -------  ------
		{ "name",   "ANAME",   0,       0      },       // record name, constant, visible.
		{ "ownTi",  "TI",      EVEOI,   FFHIDE },       // owning record index, *i, *hide.
		{ 0,        0,         0,       0      }
	};


	for (BASEFIELDS* bf = baseFields;  bf->name;  bf++)
	{
		// this code mimics rec_fds for scalar members, without * directives, and without class definition member output.

		// field name

#if !defined( MBRNAMESX)
#if defined( STRELSAVE)
		strelSave( bf->name, Nfields);
		fldNm2Save( bf->name, Nfields);
#else
		strelNm[nstrel] = strsave(bf->name);            // save member name (dmfree'd: strsave better than stash).
		strelNmNoPfx[nstrel] = strsave( ElNmFix( bf->name, enfxNOPREFIX) );
		strelFnr[nstrel] = Nfields;                     // save field number for this member
		fldNm2[ rcseq][Nfields] = ElNmFix( bf->name, enfxNOPREFIX);	// no-prefix variant
		fldFullNm2[ rcseq][Nfields] = bf->name;

#endif
#else
		mbrNames[ nstrel].mn_Set( bf->name, Nfields);
#endif


		// field type

		USI fieldtype = (USI)lufind( (LUTAB1*)&fdlut, bf->fdTyNam);     // look in fields table for field typeName
		if (fieldtype==(USI)LUFAIL)
		{
			rcderr( "Unknown field type name %s in BASEFIELDS table ", bf->fdTyNam );
			continue;
		}
		dtindex = dtnmi[ DTBMASK & (Fdtab+fieldtype)->dtype ];  // fetch idx to data type arrays for fld data type

		// put field info in record descriptor

		rcdesc->rcdfdd[Nfields].rcfdnm = fieldtype;             // field type (index in Fdtab)
		rcdesc->rcdfdd[Nfields].evf = bf->evf;                  // initial field flag (attribute) bits
		rcdesc->rcdfdd[Nfields].ff = bf->ff;                    // initial variation (eval freq) bits

		// increment

		Fdoff += abs( dtsize[dtindex]);                         // update data offset in rec. why abs ??
		Nfields++;                                              // each field or each array element gets an rcdfdd entry in RCD
		nstrel++;                                               // next structure element (subscript) / is count after loop
	}
}               // base_fds
//======================================================================
LOCAL void base_class_fds( char *baseClass, SI &bRctype )

// enter info in record descriptor for fields of base class explicitly specified with *BASECLASS

// and uses: base field names from rcFldNms[][], .
// alters globals: *rcdesc, Nfields, Fdoff, fldNm[], nstrel, strelFnr[], strelNm[], .
{
// look up given base class name, get type, descriptor, # fields

	int bRcseq = lufind( (LUTAB1*)&rclut, baseClass);
	if (bRcseq==LUFAIL)
	{
		rcderr( "Undefined *BASECLASS record type %s in record %s",
				baseClass, rclut.lunmp[rcseq] );
		return;
	}
	bRctype = rctypes[bRcseq];
	RCD* bRcdesc = (RCD *)( (char *)Rcdtab + (USI)(ULI)Rcdtab[ bRctype&RCTMASK] );     // use USI offset stored in pointer array
	int bNfields = bRcdesc->rcdnfds;

// copy field info in record descriptor, update globals Nfields, Fdoff.

	struct rcfdd *dest = rcdesc->rcdfdd + Nfields;      // point destination RCD field info
	struct rcfdd *source = bRcdesc->rcdfdd;             // source field 0 info ptr
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
		dmIncRef( DMPP( strelNm[nstrel]), ABT);
		strelNmNoPfx[nstrel] = bRcdesc->rd_strelNmNoPfx[j];
		dmIncRef( DMPP( strelNmNoPfx[nstrel]), ABT);
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
		SI j;
		char* fdTyNam;          // typeName of current field in record

		/* init for new field.  NB repeated in *directive cases that complete field: struct, nest. */
		USI evf = 0;            // init field variation (evaluation frequency) bits
		USI ff = 0;             // init field flag bits
		SI array = 0;           // not an array (if nz, is # elements)
		SI noname = 0;          // non-0 to suppress nested structure name in SFIR.mname

		// check for too many fields/structure elements in record 4-11-92

		if (nstrel >= MAXFDREC || Nfields >= MAXFDREC)                                          // need nstrel be checked??
			rcderr( "Too many fields in record, recompile rcdef with larger MAXFDREC");         // no recovery yet coded

		// Process field level "*" directives

		while (*Sval[0] == '*')                 // if *word
		{
			int wasDeclare = 0;		// set nz iff handling *declare
			SI val;
			val = looksw( Sval[0]+1, fdirtab);  // look up word after '*'
			switch (val)                        // returns bit or spec value
			{
			default:                    // most cases: bit is in table
				if (val & 0x8000)               // this bit says
					ff |= val & ~0x8000;        //   rest of val is bit(s) for "field flags" (eg *hide for FFHIDE)
				else
					evf |= val;                 // else val is bit(s) for "field variation"
				break;

			case 32767:                 // not in *words table
				rcderr("Unknown or misplaced * directive: '%s'.", Sval[0]);
				break;                          // nb fallthru gets token

			case 32766:                 // array n.  array size follows
				if (gtoks("d"))                 // get array size
					rcderr("Array size error");
				array = Dval[0];                // array flag / # elements
				break;

			case 32765:                 // struct  memName  arraySize { ...
				/* unused (1988, 89, 90), not known if still works.  Any preceding * directives ignored.
				   Fully processed by this case & wrStr(), called here. */
				if (gtoks("sds"))                               //  memNam  arSz  {  (3rd token ignored)
					rcderr("Struct name error");
#if !defined( MBRNAMESX)
#if defined( STRELSAVE)
				strelSave( Sval[ 0], Nfields);
#else
				strelFnr[nstrel] = Nfields;
				strelNm[nstrel] = strsave( Sval[0]);            // save mbr name (dmfree'd: strsave better than stash).
				strelNmNoPfx[nstrel] = strsave( ElNmFix( Sval[0], enfxNOPREFIX));
#endif
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

			case 32764:                 // nest <recTyName> <memName>
				// Any preceding * directives but *array and evf/ff ignored. Fully processed by this case & nest(), called here.
				if (gtoks("ss"))                                // rec type, member name
					rcderr("*nest error");
#if !defined( MBRNAMESX)
#if defined( STRELSAVE)
				strelSave( Sval[ 1], Nfields);
#else
				strelFnr[nstrel] = Nfields;                     // for field # define
				strelNm[nstrel] = strsave( Sval[1]);            // save mbr name (dmfree'd later). nest() uses.
																// fldNm[]/fldFullNm[] is done by nest().
				strelNmNoPfx[nstrel] = strsave( ElNmFix( Sval[1], enfxNOPREFIX));            // save mbr name (dmfree'd later). nest() uses.
#endif
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

			case 32763:                 // noname: with *nest, omit aggregate name from SFIR.mnames (but kept in C declaration)
				noname++;
				break;

			case 32762:                 // *declare "text": spit text thru immediately into record class definition.
				// intended uses include declarations of record-specific C++ member functions. 3-4-92.
				wasDeclare++;
				if (gtoks("q"))
					rcderr("Error getting \"text\" after '*declare'");
				else
					fprintf( frc, "    %s\n", Sval[0]);                 // format the text with indent and newline
				break;

			}           // switch (val)

			/* get next token: next * directive else processed after *word loop */
			// BUG: get error here if *declare is last thing in record, 10-94.
			if (gtoks("s"))
			{	const char* msg = wasDeclare
					? "Error getting token after *declare (note *declare CANNOT be last in RECORD)"
					: "Error getting token after field * directive";
				rcderr(msg);
			}

		}   // while (*Sval[0] == '*') "*" directive
		// on fallthru have a token

		/* tokens after * directives are field type and field member name */

		fdTyNam = strsave( Sval[0]);            // save type name  <---- strsave appears now superflous, 2-92. stash? <<<<<
		if (gtoks("s"))                         // next token is member name
			rcderr("Error getting field member name");
		if (*Sval[0] == '*')
			rcderr("Expected field member name, found * directive");
#if !defined( MBRNAMESX)
#if defined( STRELSAVE)
		strelSave( Sval[ 0], Nfields);
#else
		strelNm[nstrel] = strsave( Sval[0]);    // save member name (dmfree'd: strsave better than stash).
		strelNmNoPfx[nstrel] = strsave( ElNmFix( Sval[0], enfxNOPREFIX));
#endif
#else
		mbrNames[ nstrel].mn_Set( Sval[ 0], Nfields);
#endif

		// now next token NOT gotten

		/* save full name with *array subscripts for sfir table, 3-91 */
		// also done in nest().
		if (CFILESOUT)                          // else not needed, leave NULL
		{
			int i;
#if defined( STRELSAVE)
			if (array)
			{	fixName( Sval[ 0]);		// drop trailing ';' if any
				for (i = 0; i < array; i++)							// for each element of array
				{
					const char* tx = strtprintf( "%s[%d]", Sval[0], i );	// generate text "name[i]"
					fldNm2Save( tx, Nfields+i);
				}
			}
			else                                                  // non-array
				fldNm2Save( Sval[ 0], Nfields);
#else
			if (array)
			{	for (i = 0; i < array; i++)							// for each element of array
				{
					const char* tx = strtprintf( "%s[%d]", Sval[0], i );	// generate text "name[i]"
					fldFullNm2[ rcseq][Nfields+i] = tx;
					fldNm2[ rcseq][Nfields+i] = ElNmFix( tx, enfxNOPREFIX);
				}
			}
			else                                                  // non-array
			{
				fldFullNm2[ rcseq][Nfields] = Sval[ 0];
				fldNm2[ rcseq][Nfields] = ElNmFix( Sval[ 0], enfxNOPREFIX);
			}
#endif
		}                            // use strsave not stash: will be dmfree'd.

		/* get type (table index) for field's typeName */

		fieldtype = (USI)lufind( (LUTAB1*)&fdlut, fdTyNam);     // look in fields table for field typeName
		if (fieldtype == (USI)LUFAIL)
		{
			rcderr( "Unknown field type name %s in RECORD %s.", fdTyNam, rclut.lunmp[rcseq] );
			break;
		}


#if !defined( STRELSAVE)
		/* save field number in record */
		strelFnr[nstrel] = Nfields;                     // field number for this member
#endif

		dtindex = dtnmi[ DTBMASK & (Fdtab+fieldtype)->dtype ];          // fetch idx to data type arrays for fld data type

		/* put field info in record descriptor, alloc data space, count flds */

		j = 0;
		do                      // once or for each array element
		{
			rcdesc->rcdfdd[Nfields].rcfdnm = fieldtype;         // field type (index in Fdtab)
			rcdesc->rcdfdd[Nfields].evf = evf;                  // initial field flag (attribute) bits
			rcdesc->rcdfdd[Nfields].ff = ff;                    // initial variation (eval freq) bits
			Fdoff += abs( dtsize[dtindex]);                     // update data offset in rec. why abs ??
			Nfields++;                                  // each field or each array element gets an rcdfdd entry in RCD
		}
		while (++j < array);

		/* write member declaration to rcxxxx.h file.  This code also in wrStr() and nest(). */

		if (HFILESOUT)                                  // if writing the h files
		{
			fprintf( frc, "    %s %s",
					 dtnames[ dtindex],                  // dtype name (declaration)
#if !defined( MBRNAMESX)
					 strelNm[ nstrel] );                 // member name
#else
					 mbrNames[ nstrel].mn_nm );			// member name
#endif

			if (array)                                   // if array
				fprintf( frc, "[%d]", (INT)array);       // [size]
			fprintf( frc, ";\n");
		}

		// warn if odd length: compiler might word-align next (rcdef does not)
		if ( (dtsize[dtindex]                           // data type size
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
	SI arSz,            // array size of structure, for [%d] at end (any member within structure can also be array)
	RCD *rcdpin,        // record descriptor being filled: field info added here
	USI evf,            // field variation 1-92
	USI ff )            // field flags (attributes) 1-92

// alters globals: Nfields, Fdoff,
// unused since '87 or b4; read/commented 1990; treat w suspicion.
// NEEDS UPDATING to set fldNm[] -- 3-91
{
	static SI level=0;
	SI i, j, fdTy;
	SI field, nflds, fldsize, fdoffbeg, fieldbeg, source;
	SI array, generic;
	char lcsnm[MAXNAMEL];                       // Field name, lower case

	fdoffbeg = Fdoff;
	fieldbeg = Nfields;

	/* begin structure declaration: indent, write "struct {" */
	level++;
	if (rcf)                                    // not if not HFILESOUT
	{
		for (i = 0; i < level; i++)
			fprintf( rcf, "    ");               // indent
		fprintf( rcf, "struct {\n");
	}

	/* *directives / members loop */

	while (!gtoks("s"))                         // next token / while not *END/error
	{
		if (*Sval[0] == '}')
			break;                                       // stop on } (or *END?)
		if (*Sval[0] == '*')
		{
			if (!compareCaseStr( Sval[0]+1, "array"))                    // *array name n
			{
				if (gtoks("sd"))
					rcderr("Array name error");
				array = Dval[1];
			}
			else if (!compareCaseStr( Sval[0]+1, "struct"))              // nested *struct
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
		generic = FALSE;
		if (*Sval[0] == '|')
		{
			strcpy( Sval[0], Sval[0]+1);
			generic = TRUE;                               // separate member name follows
		}

		/* look up field type */
		fdTy = lufind( (LUTAB1*)&fdlut, Sval[0]);        // get subscript in Fdtab
		if (fdTy==LUFAIL)
		{
			rcderr( "Unknown field name %s in RECORD %s.",
					Sval[0], rclut.lunmp[rcseq] );
			break;
		}
		dtindex = dtnmi[DTBMASK&(Fdtab+fdTy)->dtype];    // get subscript into our data type tables

		/* get member name */
		if (generic)
		{
			if (gtoks("s"))                       // separate token
				rcderr("Generic field error");
			stringLowerCase( strcpy( lcsnm, Sval[0]) );
		}
		else {                                    // copy/lower same token
			stringLowerCase( strcpy(lcsnm, rcfdnms[fdTy]) );
		}
		/* output member declaration */
		if (rcf)                                 // if outputting h files
		{
			for (j = 0; j < level; j++)           // indent
				fprintf( rcf, "    ");
			fprintf( rcf,"    %s %s", dtnames[dtindex], lcsnm);
			if (array)
				fprintf( rcf, "[%d]", (INT)array);
			fprintf( rcf, ";\n");
		}

		/* make field info entry in record descriptor */
		for (i = 0; i < (array ? array : 1); i++)        // member array size times
		{
			rcdpin->rcdfdd[Nfields].rcfdnm = fdTy;       // Fdtab subscript
			// note b4 12-91 .rdfa (predecessor of .evf/.ff) was set ZERO here.
			rcdpin->rcdfdd[Nfields].evf = evf;           // fld variation per caller
			rcdpin->rcdfdd[Nfields].ff = ff;             // fld flags (attributes) per caller
			Fdoff += abs(dtsize[dtindex]);               // advance record offset
			Nfields++;                                   // count fields in record
		}
	} // end of members loop

	/* Structure complete.  If array of the structure being put in record,
	   repeat all the fields info for a total of 'arSz' repetitions. */

	nflds = Nfields - fieldbeg;                 // # fields in structure
	fldsize = Fdoff - fdoffbeg;                 // # bytes
	// odd length check desirable here
	for (i = 1; i < arSz; i++)                  // start at 1: done once above
	{
		for (j = 0; j < nflds; j++)              // loop fields within struct
		{
			field = fieldbeg + i*nflds + j;       // output rcdfdd subscript
			source = fieldbeg + j;                // input rcdfdd subscript
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
		for (i = 0; i < level; i++)
			fprintf( rcf, "    ");
		fprintf( rcf,"} %s[%d];\n", name, (INT)arSz);
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
	SI arSz,            // 0 or array size for array of the entire record
	RCD *rcdpin,        // record descriptor being filled -- field info added here
	USI evf,            // field variation to MERGE 12-91
	USI ff,             // field flags (attributes) to MERGE 1-92
	SI noname )         // non-0 to omit the structure name from SFIR .mnames (if not *array)

// and uses: nestee member name from mbrNames[nstrel], nested field names from rcFldNms[][]
// alters globals: Nfields, Fdoff, rctype, fldNm[],
{
	SI nRcseq, nNfields;
	RCT nRctype;
	RCD* nRcdesc;     // re nested rec type
	struct rcfdd *source, *dest;                // ptrs to RCD field info
	SI i, j;

	/* look up given record type name, get type, descriptor, # fields */

	nRcseq = lufind( (LUTAB1*)&rclut, recTyNm);
	if (nRcseq==LUFAIL)
	{
		rcderr( "Undefined *nest record type %s in record %s",
				recTyNm, rclut.lunmp[rcseq] );
		return;
	}
	nRctype = rctypes[ nRcseq];
	nRcdesc = (RCD *)( (char *)Rcdtab + (USI)(ULI)Rcdtab[nRctype & RCTMASK] );
	// use USI offset stored in pointer array
	nNfields = nRcdesc->rcdnfds;

	// copy field info in record descriptor, update globals Nfields, Fdoff

	dest = rcdpin->rcdfdd + Nfields;                    // point destination RCD field info
	i = 0;                                              // arSz counter
	do                                                  // do it all arSz times.
	{
		source = nRcdesc->rcdfdd;                        // source field 0 info ptr
		const char* name1 =                              // mbr name of *nested rec, to add nested .mbr nms to.
			arSz
#if !defined( MBRNAMESX)
				? strtprintf( "%s[%d]", strelNm[nstrel], (INT)i)
				: strelNm[nstrel];
#else
				? strtprintf( "%s[%d]", mbrNames[nstrel].mn_nm, i)
				: mbrNames[ nstrel].mn_nm;
#endif

		for (j = 0; j < nNfields; j++)                   // loop over nested rec flds
		{
			// copy one field's info (substruct rcfdd of RCD)
			dest->rcfdnm =   source->rcfdnm;             // field type: Fdtab index
			dest->evf =      source->evf | evf;          // field variation: merge nested record's and caller's
			dest->ff =       source->ff  | ff;           // field flags (attributes): merge nested record's and caller's
			// make field name text for sfir file (wSrfd3)
			if (CFILESOUT && !fldNm2[ nRcseq].empty())	// omit if no cpp file output (save ram) or ptr NULL (unexpected)
			{
				fldFullNm2[ rcseq][Nfields] =                      // Full name for C++ offsetof, etc (strsave not stash: dmfree'd):
					strtprintf( "%s.%s", // qualified name: combine with '.':
										 name1, //   curr rec mbr name[i] (name of structure member)
										 fldFullNm2[ nRcseq][j].c_str() );		//   nested fld full name

				fldNm2[ rcseq][Nfields] =              // User name for messages, probing
						(noname & !arSz)                // on *noname request, if not array (subscript needed), don't qualify,
						?  fldNm2[ nRcseq][j]                     //   just use nested member name. strsave needed for private ptr?
						:  strtprintf( "%s.%s",               // else qualified name: combine with '.':
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
			fprintf( rcf, "[%d]", (INT)arSz);    // [size]
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
	SI i;

	fprintf( f, "\n\n\n/* Record class and substruct typedefs\n"
			 "\n"
			 "   These are here to put the typedefs in all compilations (dtypes.h is \n"
			 "   included in cnglob.h) so function prototypes and external variable \n"
			 "   declarations using the types will always compile.\n"
			 "\n"
			 "   (Note: this section of dtypes.h is generated from information in \n"
			 "   records.def, not dtypes.def.) */\n\n" );

	for (i = 1; i < nrcnms; i++)                        // Start at 1: skip RTNONE
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

// write "Small record and field descriptor" C source file for compilation and linking into applications

// PART 1 of 4 parts: field types table, and stuff above portion repeated for each record type.

// for file format see comments in srd.h, or in srfd.cpp as output.
{
	SI i;
	char *dtTx;

// Write front-of-file comments and includes

	fprintf( f,
			 "// srfd.cpp\n"
			 "\n"
			 "/* This is a Record and Field Descriptor Tables source file generated generated by rcdef.exe.\n"
			 "   This file is compiled and linked into an application, such as CN. */\n" );
	fprintf( f, "\n"
			 "/* DO NOT EDIT: This file is overwritten when rcdef is run.\n"
			 "   To change, change rcdef.exe input as desired and re-run rcdef.exe via the appropropriate batch file. */\n\n"
			 "\n"
			 "#include \"cnglob.h\"	// includes <dtypes.h> for DTxxxx symbols\n"
			 "#include \"srd.h\"	// defines structures and declares variables.  Plus comments.  Manually generated file.\n"
			 "#include <ancrec.h>	// defines base class for record classes in rc____.h files.  Manually generated file.\n"
			 "// also <rc___.h> file(s) are included below.\n"
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
	for (i = 0; i < nfdtypes; i++)
	{
		// write data type 0 as "0", not "DTSI" or whatever is first, for clearer entry 0 (FDNONE)
		dtTx = (Fdtab[i].dtype==0  ? const_cast < char*>("0")
				:  strtcat( "DT", dtnames[ dtnmi[ DTBMASK&(Fdtab[i].dtype) ] ],
							NULL ) );
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

	fprintf( f,
			 "\n\n/*========== small FIELDS-IN-RECORDS tables for each record type */\n"
			 "\n"
			 " /* COLUMNS ARE\n"
			 "     .fdTy   .evf   .ff               .off              .mName \n"
			 "   (sFdtab (vari-  (fld \n"
			 "    index) ation) flgs)    (member offset)       (member name)         field type name\n"
			 "   ------- ------ -----  -----------------  ------------------         --------------- */\n"
			 "\n"
			 "/*lint -e619	suppress msg for putting \"text\" in pointer */\n\n");

// wSrfd3, called for each record type, continues the file
}               // wSrfd1
//======================================================================
LOCAL void wSrfd2( FILE *f)

// write "Small record and field descriptor" C source file for compilation and linking into applications

// PART 2 of 4 parts: portion repeated at each *file statement

// globals used include: rchFileNm: current rc___.h output include file name

// for file format see comments in srd.h, or in srfd.cpp as output.
{
	fprintf( f, "\n#include <%s>	// defines classes and structures whose tables follow\n\n", rchFileNm );
}                               // wSrfd2
//======================================================================
LOCAL void wSrfd3( FILE *f)

// write "Small record and field descriptor" C source file for compilation and linking into applications

// PART 3 of 4 parts: portion repeated for each record type: fields-in-record table for current record.

/* globals used include: rcNam: current record name
						rcdesc: current record descriptor pointer
					   fldNm[]: qualified field names
					 rcfdnms[]: field type names
						nstrel: number of struct elements in record
					strelFnr[]: 1st field # of each struct elt */

// for file format see comments in srd.h, or in srfd.cpp as output.
{
	fprintf( f, "struct SFIR sfir%s[] =\t// fields info for RT%s\n"
			 "{\n #define o(m) offsetof(%s,m)\n"
			 //    {dddd, dddd, dddd,ssssssssssssssssssss,sssssssssssssssssss },
			 " //    .ff .fdTy  .evf                 .off             .mName\n",
			 rcNam,  rcNam, rcNam );
	for (SI j = 0; j < rcdesc->rcdnfds; j++)                    // fields loop
	{
		SI fdi = rcdesc->rcdfdd[j].rcfdnm;                       // get field type
		fprintf( f, "    {%4d, %4d, %4d,%20s,%19s },\t// %s\n",
				 rcdesc->rcdfdd[j].ff,                   // field flags (attributes)
				 fdi,                                    // field type
				 rcdesc->rcdfdd[j].evf,                  // field variation
				 strtprintf("o(%s)", fldFullNm2[rcseq][j].c_str()),	// full member name, in macro call to make compiler supply offset
				 enquote( fldNm2[ rcseq][j].c_str()),		// user member name in quotes (for probes, error messages)
				 rcfdnms[fdi] );							// name of field type, in comment
#define WARNAT (30-1)								// extra -1 got lots of messages
		if (strlen(fldNm2[ rcseq][j].c_str()) > WARNAT)		// report overlong ones
			rcderr( "Warning: member name over %d long: %s", WARNAT, fldNm2[ rcseq][j]);
	}
	fprintf( f, "    {   0,    0,    0,                   0,                  0 }\t// terminator\n"
			 " #undef o\n"
			 "};	// sfir%s\n\n", rcNam );
}                                                       // wSrfd3
//======================================================================
LOCAL void wSrfd4( FILE *f)

// write "Small record and field descriptor" C source file for compilation and linking into applications

// PART 4 of 4 parts: stuff after portion repeated for each record type; small record descriptor table.
{
	SI i;
	RCD *rd;

// Write small record descriptor table

	fprintf( f, "\n\n/*========== small RECORD DESCRIPTOR table */\n"
			 "\n"
			 "	// find desired entry by searching for .rt \n"
			 "\n"
			 "SRD sRd[] =\n"
			 "{ //        recTy,    #fds,  fields-in-record pointer\n"
			 "  //        .rt      .nFlds     .fir\n" );
	for (i = 1; i < nrcnms; i++)        // loop records types
		// note start at 1 to skip RTNONE
	{
		rd = (RCD *)( (char *)Rcdtab + (USI)(ULI)Rcdtab[ rctypes[i] & RCTMASK]);
		// use USI offset stored in pointer array
		// write line "    RTXXXX,  nFields,  sfirXXXX,"
		fprintf( f, "    {%15s, %3d,   sfir%s },\n",
				 strtcat( "RT", rcnms[i], NULL),
				 rd->rcdnfds,  rcnms[i] );
	}
	fprintf( f, "    {              0,   0,   0 }\t// terminate table for searching\n"
			 "};		// sRd[]\n");

	// caller recs() finishes file and closes.
}               // wSrfd4

//======================================================================
LOCAL void sumry()              // write rcdef summary to screen and file
{
	SI i, j;

	newsec("SUMMARY");
	for (i = 0; i < 2; i++)             // once to file, once to screen
	{
		FILE *stream;
		if (i)
			stream = stdout;
		else
			stream = fopen("rcdef.sum","w");
		fprintf( stream, "   %d errors\n", (INT)Errcount);
		if ((USI)(Stbp-Stbuf) > STBUFSIZE/2)                            // suppress if little used
			/*lint -e559 no message for arg STBUFSIZE > 32K */
			fprintf( stream, " Stbuf use: %u of %u   \n", Stbp-Stbuf, STBUFSIZE);
		/*lint +e559 */
		fprintf( stream, " dtypes: %d of %d (%d bytes)  ",    (INT)ndtypes,  (INT)MAXDT, INT(dttabsz*sizeof(SI)) );
		fprintf( stream, " units: %d of %d \n",               (INT)nuntypes, (INT)MAXUN );
		fprintf( stream, " limits: %d of max %d          ",   (INT)nlmtypes, (INT)MAXLM );
		fprintf( stream, " fields: %d of %d \n",              (INT)nfdtypes, (INT)MAXFIELDS );
		fprintf( stream, " records: %d of %d             ",   (INT)nrcnms,   (INT)MAXRCS );
		fprintf( stream, " most flds in a record = %d of %d \n",   (INT)MaxNfields, (INT)MAXFDREC );
		if (Errcount)
			fprintf( stream, "Run did *NOT* complete correctly ************.\n");
		if (i==0)
		{
			SI n = 0;
			if (fdlut.stat != NULL)                      // if status array set up  (added 6-1-89 rob: was not set)
			{
				for (j = 0; j < fdlut.lunent; j++)        // loop over lupak table
					if (!fdlut.stat[j])                    // if defined (luadd'ed) but not used (not lufound)
					{
						if (!n++)                                   // 1st time
							fprintf( stream, "\n\nUnused fields ->");
						fprintf( stream, "\n   %s", *(fdlut.lunmp+j));
					}
			}
		}
	}
}                       // sumry
//======================================================================
LOCAL FILE * rcfopen(           // Open an existing input file from the command line

	char *s,            // File identifying comment for error messages
	char **argv,        // Command line argument pointer array
	SI argi )           // Index of command line arg to be processed

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
LOCAL SI gtoks(                 // Retrieve tokens from input stream "Fpm" according to tokf

	char *tokf )        /* Format string, max length MAXTOKS,
						   1 char for each desired token as follows:
						   s   string
						   d   integer value; any token beginning with 0x or 0X is decoded as hex
						   f   floating value
						   q   quoted string
						   p   peek-ahead: returns next non-blank char without removing it from input stream */

/* decoded tokens are placed in global arrays indexed by token #:
				Sval[]	string value (ALL types)
				Dval[]	SI value (d).
				Fval[]	float value (f) */
/* Returns (fcn value AND global gtokRet):
		GTOK:       things seem ok
		GTEOF:      unexpected EOF
		GTEND:      first token was *END
		GTERR:      conversion error or *END in pos other than first,
					Error message has been issued. */
{
	SI ntok;            // token number
	char f;             // format of current token
	SI c, nextc;
	SI i;
	char *fmt,*t2;
	char token[400];                            // Token from input file
	SI commentflag, oldcommentflag;             // 1 if currently scanning a comment
	static char *comdelims[2] = { "/*", "*/" }; // Comment delimiters

	if ((USI)(Stbp-Stbuf) > STBUFSIZE - 500)
		rcderr("Stbuf not large enough !");
	ntok = -1;

	/* loop over format chars */

	while ( (f = *tokf++) != 0)                 // get next fmt char / done if null
	{
		if (ntok++ >= MAXTOKS)
			rcderr("Too many tokens FUBAR !!!");

		/* first decomment, deblank, and scan token into token[] */

		if (f=='q' || f=='p')
		{

			/* deblank/decomment for q or p */

			while (1)
			{
				c = fgetc(Fpm);                   // next input char
				if (c == EOF)                     // watch for unexpected eof
				{
ueof:
					rcderr("Unexpected EOF: missing '*END'?");
					return (gtokRet = GTEOF);
				}
				if (c=='/')                       // look for start comment
				{
					nextc = fgetc(Fpm);
					if (nextc != '*')                       // if no * after /, unget and
						ungetc( nextc, Fpm);                // fall thru to garbage msg
					else
					{
						do                                   // pass the comment
						{
							while ((c=fgetc(Fpm)) != '*')     // ignore til *
								if (c == EOF)
									goto ueof;
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
					rcderr( "Error: Quoted string longer than %d: %s", (INT)MAXQSTRL, token);
			}    // if q
		}
		else     // fmt not p or q
		{

			/* deblank/decomment/get token for all other formats */

			/* read tokens till not in comment */
			commentflag = 0;
			do
			{
				SI nchar;
				nchar = fscanf(Fpm,"%s",token);
				if (nchar < 0)
					goto ueof;
#if 1        // work-around for preprocessor inserting a NULL in file
				// .. (just before comment terminator) 6-13-89
				while (nchar > 1
						&& *token=='\0'                   // leading null !!?? in token
						&& strlen(token+1) <=(USI)nchar)  // insurance: only if rest is terminated within nchar
					// ^^^ should be < but did not work
				{
					strcpy( token, token+1);
					nchar--;
					printf( "\n  gtoks removing leading null from token !? \n");
				}
#endif
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
			if (*token=='0' && toupper(*(token+1))=='X')
			{
				fmt = "%hx";                                      // use %x if token looks hex-ish
				t2 = token+2;                                     // point after 0x
			}
			else
			{
				fmt = "%hd";
				t2 = token;
			}
			if (sscanf(t2,fmt,&Dval[ntok]) != 1)
				rcderr("Bad integer value: '%s'.", token);
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
}                       // gtoks
//======================================================================
LOCAL void rcderr( const char *s, ...)                // Print an error message with optional arguments and count error

// s is Error message, with optional %'s as for printf; any necessary arguments follow.
{
	SI i;
	FILE *stream;
	static FILE *errfile=NULL;
	va_list ap;

	va_start( ap, s);                   // point at args, if any
	const char* ss = strtvprintf( s, ap);           // format (printf-like) into Tmpstr
	for (i = 0; i < 2; i++)
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
}               // recderr
//======================================================================
LOCAL void newsec(char *s)              // Output heading (s) for new section of run
{
	printf("\n%-16s  ", s);             // no trailing \n.  -20s-->-16s 1-31-94.
}                               // newsec
//====================================================================
LOCAL void update(              // replace old version of file with new if different, else just delete new.

	const char *old,	// Old file name.  On return, a file with this name remains.
	const char *nu )    // New file name.  Files must be closed.

// Purpose is to avoid giving a new date/time to output files which have not actually changed.
{
	if (xffilcmp( old, nu) != 0)
	{
		printf( " Updating %s. ", old);          // (trailing newline removed 2-90)
		xfdelete( old, IGN);
		xfrename( nu, old, ABT);
	}
	else
	{
		printf( " %s unchanged. ", old);
		xfdelete( nu, ABT);
	}
}                   // update
//======================================================================
LOCAL char * stashSval( SI i)

// copy token in Sval[i] to next space in our string buffer, return location
{
	return stash( Sval[i]);
}                       // stashSval
//======================================================================
LOCAL char * stash( char *s)

// copy given string to next space in our string buffer, return location
{
	Stbp += strlen(Stbp)+1;     // point past last string to next Stbuf byte
	return strcpy( Stbp, s);    // copy string there, ret ptr
}                       // stash
//======================================================================
LOCAL const char* enquote( const char *s)  // quote string (to Tmpstr)
{
	return strtprintf( "\"%s\"", s);    // result is transitory!
}               // enquote
//======================================================================
void CDEC ourByebye( int code)           // function to return from program

// Used with envpak.cpp hello() and byebye().

/* In rcdef, this fcn's serves only to bypass envpak.cpp messages
   that envpak may issue if no hello() call with byebyeFcn ptr is executed b4 exit via byebye().
   Fatal errors exit with byebye; there are many error calls with
   erOp = ABT in lib code called from rcdef.exe. 10-93. */
{
	exit( code);		// return to DOS. C library function.
}               // ourByebye
//---------------------------------------------------------------------
int CheckAbort()
// in CSE, CheckAbort is used re caller interrupt of DLL simulation
// here provide stub for RCDEF link
{	return 0;
}		// CheckAbort
/*====================================================================*/

// mtpak.c -- memory table management pak

/* Possible improvements dept. 9-18-88

   1.	mtuadd / mtulook world is weak since

	a.  Works only with 16 bit objects

	b.  Original spec of mtulook precluded use of 0 as an associated
		value.  Chip added mtulook2 9-18-88 to get around this, but
		a general change not done because there are *MANY* calls to
		mtinit / mtuadd / mtulook etc.  Careful re-spec and a
		thorough pass is required.

	c.  mtulook treats pmtu==NULL as if the entry sought was not
		found, should probably be a program error abort.  Again,
		careful check of all calls required.

	d.  function overlaps with Rob's lookww routine in util.c

	2.	The whole pak overlaps to some degree with stpak (stack
	routines).  mtpak is simply an indexed addressing of memory
	tables rather than FIFO.  Should be unified ??.
	  Rob's COMMENT 6-89: mtpak table pointer is to after header,
	stpak ptr is to start of header.  If all stpak calls were reworked
	to be like mtpak (change stpak as intermediate step?),
	then should be easy to unify: add sp member to header and mtpush,
	mtpop calls.  use the sp member for MTUCUR.  rob 6-89 */

/*-------------------------------- DEFINES --------------------------------*/

#define MTGROWTH 5	// how many slots to add when enlarging a table

/* defined in mtpak.h, used 6-89 in points.c, ceilmu.c, dbxpak.c:
 *  #define MTUCUR(pmtu) (pmtu)->oldval	 * next slot to use in MTU table:
					   Slot 0 .oldval contains # used
					   slots; 1st user slot is slot 1. */

/*------------------------------- TYPEDEFS --------------------------------*/

struct MTHEADER			// memory table header
{
	SI slotsize;
	SI alslots;
};

/*--------------------------- PUBLIC VARIABLES ----------------------------*/
SI NEAR Mteract = ABT;	// mtpak error action code.  Changed and restored 6-89 by: pdbpak, dospak.

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC /*no FC: checks stack*/ NEAR mtralloc( char **, SI, SI);


//=========================================================================
RC FC mtpinit( char **pmtab, SI sltsize, SI nslots)

// (re)initialize a memory table, pointer interface: rob 6-89.

// at entry *pmtab must be NULL or point to old table.
// see mtinit (next)

// returns RCOK if ok.  Errors abort unless caller changes Mteract.
{
	RC rc;

	rc = mtralloc( pmtab, sltsize, nslots);  	// (re)alloc to size. local.
	// on error, *pmtab not changed: old table or NULL remain.
	if (rc==RCOK)
		memset( (char *)*pmtab, 0, sltsize * nslots);	// zero whole table
	return rc;
}		// mtpinit
//=========================================================================
char * FC mtinit( 			// initialize a memory table

	SI sltsize,  	// length of each slot in bytes
	SI nslots )  	// number of slots to allocate

/* caller can access data in resulting table at  ptr + slot * sltsize.
	mtadd( &ptr, slot, data ) can be used to point to or store data,
	and will enlarge the table if necessary. */

/* sometime recode uses to mtpinit, above, 6-89?
   if so BE SURE ptr is NULL or valid mt -- init stack variables! */

/* returns char * ptr to data area of initialized, zeroed table.
   CAUTION: ptr is not to start of dm block; free only with mtfree()!
   Returns NULL if out of memory and Mteract is not ABT. */
{
	MTHEADER *p;

	if (dmal( DMPP( p), sltsize*nslots + sizeof(MTHEADER), Mteract | DMZERO)==RCOK)	// zero it
	{
		p->slotsize = sltsize;
		p->alslots = nslots;
		return (char *)(p+1);		// user's pointer is AFTER the header
	}
	else
		return NULL;			// out of memory and not ABT
}		       // mtinit
//=========================================================================
char * FC mtadd( 		// add an entry to a memory table; return pointer to entry.

	char **pmtab,	// pointer to pointer to memory table:  CAUTION: table may move; ptr updated if so.
	SI slot,		// slot to put data in.  Table is reallocated if necess, and added memory is 0'd.
	char *data )	/* NULL or pointer to new data to store in slot.
			   (User may store data using returned pointer,
			   or *(pmtab + slot * slotsize) if table already big enuf.)*/

/* returns pointer to data location in memory table (a CHANGE 6-89 rob;
   no existing code uses return value but new crb update code may...).
   If out of memory and Mteract not ABT, rets NULL w/o changing *pmtab. */
{
	SI sltsize;
	MTHEADER *phead;
	char *datloc;

	phead = (MTHEADER *)*pmtab - 1;	// header is before data
	sltsize = phead->slotsize;		// fetch slot size
	if (slot >= phead->alslots)		// if necessary, enlarge table.
		if (mtralloc( pmtab, 		// realloc / if ok. local.
		sltsize,
		slot + MTGROWTH) )	// extra space. only use of MTGROWTH.
			return NULL;			// error: ret NULL, *pmtab unchanged
	datloc = *pmtab			// fetch after mtralloc: can move it!
	+ slot*sltsize;      	// point data location in table
	if (data != NULL)    		// if data given for us to store
		memcpy( datloc, data, sltsize);	// copy data into table slot
	return datloc;			// ret data location
}		// mtadd
//=========================================================================
void FC mtfree(			// frees a memory table from dm and sets caller's pointer NULL

	char **mtabp )	// pointer to NULL or to pointer to memory table
{
	char *p;

	if (mtabp)			// insurance -- NULL here not expected
	{
		p = *mtabp;
		if (p)			// if caller's pointer not NULL
		{
			p -= sizeof(MTHEADER);	// point dm block: b4 mt header
			dmfree( DMPP( p));		// free the dm block
			*mtabp = NULL;		// NULL caller's pointer: tell caller's logic (or next mtfree) no table allocated.
		}
	}
}			    // mtfree
//=========================================================================
MTU * FC mtuadd(		// adds an element to an mtu (memory table unsigned

	MTU * pmtu, 	// pointer to mtu (if NULL it will be allocated
	USI old,		// old USI (or key -- rob
	USI nu )		// new USI (or value

/*  an mtu provides a simple look up capability for USI values.  Example:

		ptmu = NULL;
		mtuadd(pmtu,oldbn,newbn);
		.
		.
		.
		bn = mtulook(pmtu,oldbn);  //this will return newbn */

// returns updated ptmu (it may be reallocated and move)
{
	MTU mtu;

	if (pmtu == NULL)
	{
		pmtu = (MTU *)mtinit( 2*sizeof(USI), 20);
		if (pmtu != NULL)
			MTUCUR(pmtu) = 1;	    /* The count of objects in the table is stored in slot 0 (not header);
					   actual data starts in slot 1 */
	}
	if (pmtu != NULL)
	{
		mtu.oldval = old;
		mtu.newval = nu;
		mtadd( (char **)&pmtu, MTUCUR(pmtu)++, (char *)(&mtu));		// (mtadd is NULL if out of memory and not ABT -- 6-89)
	}
	return pmtu;
}			    // mtuadd
//=========================================================================
USI FC mtulook( 	// Look up USI value associated with old in a MTU; see also mtulook2

	MTU *pmtu,		// pointer to mtu
	USI old )		// old USI

/* This is the original mtulook spec.  mtulook2 added 9-18-88 by Chip becuase
   mtulook does not allow a 0 value for "nu".  See comments at beg. file */

// Returns new USI associated with old, 0 if not found
{
	USI nu;

	if (mtulook2( pmtu, old, &nu))
		return nu;
	else
		return 0;
}		// mtulook
//=========================================================================
SI FC mtulook2( 		// Looks up USI value associated with another in an MTU

	MTU *pmtu,		// Pointer to MTU.  If NULL, FALSE is returned
	USI old,		// USI sought
	USI *pnew )		// Pointer for returning associatied value

// see also mtulook() above

/* Returns:  TRUE if match found, *pnew contains matched value
		 FALSE if no match (*pnew undefined) */
{
	USI i, n;

	if (pmtu != NULL)
	{
		n = MTUCUR(pmtu);
		for (i = 0; ++i < n; )		// Start search at slot 1 since slot 0 holds count used by MTUCUR()
			if ((pmtu+i)->oldval == old)
			{
				*pnew = (pmtu+i)->newval;
				return TRUE;
			}
	}
	return FALSE;
}			// mtulook2
//=========================================================================
USI FC mturep( 			// Looks up USI value associated with old in a MTU and replaces prev new with current new

	MTU *pmtu,		// pointer to mtu
	USI old,		// old USI - exist in the table
	USI nu )		// new USI to replace value of previous new USI in the table

// returns previous new USI associated with old, 0 if not found and not replaced
{
	USI i;
	USI prevnew;

	for (i = 1; i < MTUCUR(pmtu); i++)
	{
		if ((pmtu+i)->oldval == old)
		{
			prevnew =  (pmtu+i)->newval;
			(pmtu+i)->newval = nu;
			return prevnew;
		}
	}
	return 0;
}			    // mturep
//=========================================================================
// no FC because generates compile warning with stack_check on
LOCAL RC NEAR mtralloc( 		// (re)allocate a memory table

	char **pmtab,	// pointer to NULL or memory table pointer.  Updated if successful.  Added memory is 0'd.
	SI sltsize,  	// # bytes per "slot"
	SI nslots )  	// slot which table must grow to accept, plus extra

// returns RCOK if succeeded; errors abort unless caller changes Mteract
{
	MTHEADER *p;
	RC rc;

	p = (MTHEADER*)*pmtab;	// fetch caller's table pointer
	if (p)			// unless NULL (no table alloc)
		p--;			// back over header to start dm block
	rc = dmral( DMPP( p),				// (re)alloc block, dmpak.c
	nslots*sltsize + sizeof(MTHEADER),	// new size in bytes
	Mteract 				// ABT or as changed externally
	| DMZERO);				// 0 new memory 9-88
	if (rc==RCOK)
	{
		p->slotsize = sltsize;   	// (for new alloc case)
		p->alslots = nslots;		// new size in slots to header
		*pmtab = (char *)(p + 1);	// return user table ptr: after hdr
	}
	return rc;
}			// mtralloc
//=============================================================================

/////////////////////////////////////////////////////////////////////////////////
// lupak: semi-obsolete lookup table manipulation
//    only uses are in rcdef.cpp, code included below 7-10
/////////////////////////////////////////////////////////////////////////////////
/* Routines for managing simple static lookup tables such as
   those used for packet names and slot names.

   These routines are used in rcdef.c, hpcreate.c, mcdef.c.
   They are not used in takeoff itself.

Lookup tables are basically arrays of pointers to character strings.
(The strings are not stored in the table and must be manipulated
externally in char arrays or in dynamic memory.)  The symbols are
cast into canonical form (currently all upper case) prior to storage.
Lookups compare canonical forms, so lower case values match uppercase
equivalents.  A simple hash scheme is used to speed lookup: each entry
has a hash value associated with it and no string compare is performed
unless the hash values match.

Abbreviations: Currently, no abbreviations are provided for.  They
could be simply added by modifying the LUTAB structure in lupak.h
to include either the abbreviation for each entry (or a pointer to
the abbreviation), and adding a 2nd hash byte for the abbrev. */


/*-------------------------------- DEFINES --------------------------------*/
#define MAXSYML 100	// Maximum lookup length -- used for declaring temporary strings

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL SI FC NEAR lusetup( char * sym, char * canonicl);
LOCAL SI FC NEAR lusearch( LUTAB1 * lutab, char * canonicl, SI hash);

//======================================================================
SI FC lufind( 			// Find a symbol in a lookup table

	LUTAB1 *lutable,	// Lookup table (actually struct LUTAB(n)).
	char *sym )		// Symbol sought

// Returns: Table position associated with sym or LUFAIL
{
	SI hash;
	char canonicl[MAXSYML];

	hash = lusetup( sym, canonicl);
	return lusearch( lutable, canonicl, hash);

}		// lufind
//======================================================================
SI FC luadd( 			// Canonicalize symbol in place and add to lookup table */

	LUTAB1 *lutable,	// Lookup table
	char *sym )		/* Symbol to add.
			   Must be in static storage: luadd does not copy it.
			   Is overwritten with canonical form of itself. */

// Returns slot number assigned to symbol, or LUFAIL if no space avail.
{
	SI hash, p;

	if (lutable->lunent == lutable->lumxent)
		return (LUFAIL);
	hash = lusetup(sym,sym);
	p = lutable->lunent++;
	*((lutable->lunmp)+p) = sym;
	// next line - set status only if stat array is set up
	if ((lutable->stat) != NULL)
		*((lutable->stat)+p) = 0;
	lutable->luhash[p] = (char) hash;
	return (p);
}		// luadd
//======================================================================
LOCAL SI FC NEAR lusetup( 		// Set up for hash values and canonical symbol for lookup table action

	char *sym,		// Symbol in original form */
	char *canonicl )	// Canonical form symbol will be returned here.  Must be large enough to accomodate symbol.

// Returns: Hash value associated with sym.
{
	SI hsum;
	UCH c;

	hsum = 0;
	while ( (c = *(UCH *)sym++) != 0)
		hsum += (*canonicl++ = (char)toupper(c));
	*canonicl = '\0';
	return (hsum % LUHASHF);

}		// lusetup
//======================================================================
LOCAL SI FC NEAR lusearch( 				// Search a lookup table for a symbol.

	LUTAB1 *lutable,  	// Lookup table
	char *canonicl,  	// Symbol sought in canonicl form (see lusetup)
	SI hash )         	// Hash value for symbol (see lusetup)

// Lusetup must be called prior to calling lusearch (see lufind to do both).

// Returns: Value associated with symbol or LUFAIL if not found
{
	char *p, *pbeg, *pend;
	SI i;

	pbeg = &(lutable->luhash[0]);
	pend = pbeg + lutable->lunent;
	*pend = (char)hash;  		// Sentinel for end of loop
	for (p=pbeg; ; p++)
	{
		if (*(UCH *)p != (UCH)hash)
			continue;
		if (p == pend)
			return (LUFAIL);
		i = (SI)(p - pbeg);
		if (!strcmp( (char *)*(lutable->lunmp+i), (char *)canonicl) )
		{
			// next line - set status only if stat array is set up
			if ((lutable->stat) != NULL)
				*(lutable->stat+i) = 1;
			return (i);
		}
	}
}		// lusearch
//=============================================================================


// rcdef.cpp end
