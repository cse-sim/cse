// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* pgpak.h -- Declarations for output page management routines:

	      files pgpak.cpp, cpgprput, cpnat.cpp.

	      See also related code in cpgbuild.c/h: pgbuildr functions
	      use pgpak to assist in generating complex displays */

	/* PAGEs are representations of screen area or printer page area
	   that can be written into with arbitrary cursor moves and
	   overwrites, then displayed/printed after fully prepared.
	   PAGES are stored in dynamic memory (dmpak.c).
	   Caller keeps pointer (type char *, PAGE struct is private).
	   PAGES can hold attribute/enhancement information with text. */

	// CSE variant 9-13-91: includes some txpak.h defns so that file is not needed.


// PAGE structure.   NB pgpak:pgralloc() makes assumptions about order of "segments" in buffer; change with care.
struct PAGE
{
    SI rows, cols;	// Number of rows and columns in page
    SI rlw, clw; 	// 1-based position of last char put in page, maintained by pgwe(), can be outside page boundaries.
    USI flags;		// flags (defs just below): PGISUP, PGCON, .
  // offsets relative to pb[0] ("Virtual Offsets") for 3 (formerly 5) "segments" of buffer
    // next 2 segments are 1 byte per row, offset is for 1-based subscript
    SI llenvo;		// bytes: "line lengths" (max used 1-based column (1 more byte avail for \0 in pgVrPut, 11-91))
    SI lbegvo;		// bytes: line begs (min used 1-based col in row)
    // next segment is full rows, offset for subscript = 1-based row*col
    SI pgbvo;		// offset to text buffer - rows*(cols+1) bytes.  +1 for \0 added by rob, 11-91
    unsigned char pb[1];	// Beg of page buffer -- where -vo's point.
};

//--- defines for PAGE.flags
#define PGISUP 1	// non-0 if page is (probably) now showing on screen
#define PGCON  2	// print continutation flag: set by cpgprput for cpnat

//--- Useful macros
#define PP ((PAGE *)pp)						// for accessing PAGE pp when pp is declared as char *
 #define PGSIZE(r,c) ( sizeof(PAGE) + (r)*((c)+3+1) - 1 )	// allocation size for # rows and cols; +1 for null 11-91
								// c here is 1 less than stored .cols.
#define LINELEN(r) ( PP->pb + PP->llenvo + (r) )		// points to line length for row
#define LINEBEG(r) ( PP->pb + PP->lbegvo + (r) )		// .. beginning
#define PGBUF(r,c) ( PP->pb + PP->pgbvo + (r)*PP->cols + (c) )	// points into text buffer for r,c


/*---------------------- DEFINES FOR pgw() FORMAT -------------------------*/

     /* Used with pgpak:pgw, pgwe, pgwrep, pgcpfille, pgfille ("fmt" arg);
	and passed to pgpak via many other fcns such as pgbuildr
	via "pgfmt" argument or structure member. */

     /* Use *ONE* orientation/pos. constant plus any options desired. */

     /* NOTE: pgbuildr uses the pgw() format 0xFFFF as a terminator.
	If 0xFFFF becomes a useful pgw() format, changes will be required
	in pgbuildr.c, pgbuildr.h, and calling code with data-init tables. */


/*--- pgw fmt: Orientation/positioning constants: use ONE */
#define PGLJ        0x0000    // Left justify -- default
#define PGRJ        0x0040    // Right justify (bit is PGINDENT to pgfille) (no cnr uses 11-91)
#define PGCNTR      0x0080    // Center (not for pgfille) (no cnr uses 11-91)
#define PGPMASK     0x00C0    // Mask for positioning field

/*--- pgw fmt: "Newline" control.  These are functional in all cases, but are
   useful primarily in the pgfille() style writes.  See also PGCUR generic
   value for row/col specification. */
#define PGNLBEF     0x0200	// do NL before writing (no cnr uses 11-91)
#define PGNLIF      0x0400	// do NL b4 writing iff not at left margin.
#define PGNLAFT     0x0800	// do NL after writing (no cnr uses 11-91)
#define PGINDENT    PGRJ	/* Makes pgfille indent to starting column
				   instead of left edge at each wrap.
				   Same as PGRJ to save bits (no cnr uses 11-91). */

/*--- pgw fmt: Page re-allocation */
#define PGGROW      0x8000	/* Add rows to page upon writing below
				   bottom (without this option, text is
				   discarded).  Does not add columns. */

/*--- pgw fmt: Bit used by pgbuildr in pgpak format word.
   Duplicate define here is to cause error if bits not maintained to match
   in pgbuildr.h and pgpak.h. */
#define PBFILL 0x0100 	/* Causes pgfille not pgw in certain pgbuildr methods;
			   see pgbuildr.h. rob 2-90. */


/*------------- GENERIC row/col values for pgw, pgwe, pgfille -------------*/

#define PGCUR    11000   /* Generic row/col pos -- current position.
			    PGCUR+n means advance n, PGCUR-n == go up/back n */
#define PGCURMIN 10000   /* Anything above this value is a PGCUR.
			    (also used in tests in tested in pgbuildr.c) */


/*------------------- OPTION bit for pgwe "erOp" arg ----------------------*/

const int PGOPSTAY = EROP7;	// restore "cursor" row-col after writing: so caller
							// may use same PGCUR-relative row or col again. 2-90.


/*----------------------- [pgprput]/ pgvrput FUNCTION FLAG[S] --------------------------*/

	// uses: "flags" arg of cpgprput:pgvrput; pgvrput callers in cpnat.c.

#define PGGPTRIM   0x0001	// Do not display or print white space at
							// right edge and bottom of page


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
//   pgpak.cpp
extern RC     FC pgalloce( SI, SI, char * *, int erOp);
extern void   FC pgfree( char * *);
extern void   FC pgDelrows( char **ppp, SI row, SI nrows);
extern void   FC pgw( char **, USI, SI, SI, const char *);
extern RC     FC pgwe( char **, USI, SI, SI, const char *, int erOp);
extern void   FC pgwrep( char **, USI, SI, SI, const char *, SI);
extern RC     FC pgfille( char * *, USI, SI, SI, const char *, int erOp);
extern SI     FC pgnrows( char *,USI);
extern SI     FC pgcrow( char *);
extern void   FC pgputrc( char *, SI, SI, SI, SI *, SI *);
#if 0	// no calls 11-91
x extern char * FC pgalloc( SI, SI);
x extern RC     FC pgcpfille( char * *, USI, char *, SI);
x extern char * FC pgpos( char *, SI, SI);
x extern SI     FC pgcols( char *);
#endif
#if 0	// debugging fcn, 7-17-90
d  extern void FC pgDbPrintf( char *s, char *pp);
#endif

#if 0	// unused in cnr (?) verifying. rob 11-91
x    //   pgpak2
x  extern void   FC pgboxmes( char *);
x  extern void   FC pggrput( char *, USI, SI, SI);
x  extern void   FC pggrpput( char *, USI, SI, SI, SI, SI);
x  extern void   FC pggrrstr( char *);
x
x  //   pgpak3
x  extern SI     FC pgscroll( char *, USI, SI, SI, SI *, SI, SI, SI *,
x  			   void(*)(SI,SI), SI, char *, char *);
x  //   pgpak4 (used in support programs only)
x  extern SI     FC pglnlen( char *, SI);
x  extern char * FC pgpcopy( char *,SI,SI,SI,SI);
#endif

//   cpgprput.cpp
extern RC     FC pgVrPut( SI vrh, SI isFmt, char *pp, USI flags, SI row1, SI nrows);

//   cpnat.cpp
void FC pnSetVrh( SI vrh);
void FC pnSetTxRows( SI lpp);
void FC pnSetHeader( char **ppp, SI fmt, SI row, SI col);
RC   FC pnAlloc( char **ppp, SI rows, SI cols, int erOp);
void FC pnFree( char **ppp);
void FC pnTitle( char **ppp, SI row, SI col, char *s);
void FC pnSetTh( SI thRows);
RC   FC pnPgIf( char **ppp, SI badness, SI x);
RC   FC pnPrPg( char **ppp, SI x);

// end of pgpak.h
