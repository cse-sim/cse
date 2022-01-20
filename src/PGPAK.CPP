// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// pgpak.cpp: PAGE management routines, screen/printer common base portion: fcns to allocate and write into PAGEs

/* PAGEs are representations of screen area or printer page area that can be written into
   with arbitrary cursor moves and overwrites, then displayed/printed after fully prepared.
   *  PAGES can hold attribute/enhancement information with text.
   *  PAGES are stored in dynamic memory (heap) (dmpak.cpp).
   *  Caller keeps pointer (type char *, PAGE struct is private). */

/* pgpak is in multiple files for flexible linking:
	lib\pgpak.cpp  	  screen/printer common base portion
	[pgpak2.cpp	  not in CSE version: screen-only functions ]
	[pgpak3.cpp	  not in CSE version: additional scrn-only fcn: pgscroll() ]
	[pgpak4.cpp	  not in CSE version: used only in support programs ]
	app\cpgprput.cpp  printer only: pgprput() */


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <PGPAK.H>	// public pgpak defs and decls,

/*-------------------------------- DEFINES --------------------------------*/

// #define PP ((PAGE *)pp)  see pgpak.h
// struct PAGE ... see pgpak.h

/*--- re \-codes that may be imbedded in text ---
      NOTE: mcdef:gnc() discards \ characters (to allow hiding characters
            from the preprocessor).  Any \-codes embedded in mcdef input
	    text need to be \\ (eg \\n, \\Z) 6-10-89 */
#define HANS ""	// 11-91 no \ codes other than \n for newline

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC     FC NEAR pgralloce( char * *, SI, SI);
LOCAL void   FC NEAR pgsetup( char *, SI, SI);
LOCAL RC     FC NEAR pgcleare( char * *, int erOp);
LOCAL void   FC NEAR pgerase( char **, SI);
LOCAL SI     FC NEAR pgGenrRc( SI, SI);
LOCAL USI       NEAR pgcByto( char **ps, USI *pnMax, char *cods, SI bsF);  // no FC: has check_stack
LOCAL USI    FC NEAR pgcStrWid( char *s, USI nMax, SI bsF);

#if 0	// no calls in CSE, 11-91. use pgalloce.
x //========================================================================
x char * FC pgalloc( rows, cols)
x
x // Initialize a page for output, abort on error
x
x SI rows;    // Number of rows in page (max value 255)
x SI cols;    // Number of columns in page (max value 255)
x
x // Returns pointer to area allocated for page
x{
x char *pp;
x
x     pgalloce( rows, cols, &pp, ABT);
x     return pp;
x}		// pgalloc
#endif

//========================================================================
RC FC pgalloce( 		// Initialize a page for output with error handling, for pgalloc

	SI rows,		// Number of rows in page (max value 255)
	SI cols,		// # of columns (max 254) (note PP->cols is 1 larger 11-91)
	char **ppp, 	// POINTER TO where to return pointer
	int erOp )		// error action: ABT, WRN, etc

// returns RCOK or RCBAD;  pointer is returned via argument ONLY if ok.
{
	char *pp;

	/* Initial allocation includes space for line lengths, enhancement pointer values, and page buffer.
	   [Space for enhancement flag values is not allocated until needed.]  See pgwe() */

	if (dmal( DMPP( pp), PGSIZE( rows, cols), erOp | DMZERO) != RCOK)	// dmpak.cpp
		return RCBAD;							// if memory full (and not ABT), return error to caller
	// appropriate uses of PP->cols changed to cols-1, or <= changed to <, when this added: many if's below.
	pgsetup( pp, rows, cols+1);		// store rows, cols, init offset to segments, etc
	// cols+1: extra byte per row for \0 at row output, eg in pgprput.cpp, rob 11-91.
	// init handled by DMZERO:
	//   PP->flags = 0;		* init bits: PGISUP off, PGCON off, *
	//   [PP->nenhance = 0;]
	if (RCOK != pgcleare( &pp, erOp) )
	{
		// error "impossible", but be safe
		dmfree( DMPP( pp));	// error, but we alloc'd block ok, so now free it before error return.
		return RCBAD;
	}
	*ppp = pp;
	return RCOK;
}		// pgalloce
//========================================================================
LOCAL RC FC NEAR pgralloce( 		// Reallocate a page with additional or fewer rows, with error handling

	char **ppp,		// Pointer to pointer to page.  Pointer is updated.
	SI rowsnew,		// New number of rows in page (max value 255)
	int erOp )		// error action: ABT, WRN, etc

// Does not eliminate enhancement flag lines which are no longer referenced.  See note at beginning of file.

/* returns RCOK if ok, or non-0 on error such as memory full (and not ABT).
   On error, original block not gone -- *ppp must still be free'd. */
{
	SI i;
	SI rowsold, cols, delta;
	char *p, *pp;
	RC rc;

// Tables for sizes and offsets of each segment of page.  Segments are: line length, line beg, and text.
#define PGRTABLESIZE 3
	SI szold[PGRTABLESIZE],  sznew[PGRTABLESIZE];
	SI offold[PGRTABLESIZE], offnew[PGRTABLESIZE];

// Working values
	pp = *ppp;
	rowsold = PP->rows;
	cols = PP->cols;
	delta = rowsnew - rowsold;

	/* Set up tables of offsets to and sizes of various segments of pb.
	   These tables allow convenient up- or down-shift of data as required.
	   This code is based on the structure of PAGE; change with care. */

	szold[0] = szold[1] = rowsold;
	szold[2] = rowsold*cols;
	sznew[0] = sznew[1] = rowsnew;
	sznew[2] = rowsnew*cols;
	offold[0] = offnew[0] = 0;
	for (i = 1; i < PGRTABLESIZE; i++)
	{
		offold[i] = offold[i-1] + szold[i-1];
		offnew[i] = offnew[i-1] + sznew[i-1];
	}

	/* Now shift and reallocate or reallocate and shift.
	   Note that first segment (i==0) of pb need not be shifted.  Its end may be overwritten, but its origin does not move. */

	if (delta < 0)
	{
		for (i = 1; i < PGRTABLESIZE; i++)		// shift lower
		{
			p = (char *)PP->pb;
			memmove( p+offnew[i], p+offold[i], sznew[i]);
		}
	}
	rc = dmral( DMPP( pp), PGSIZE( rowsnew, cols), erOp);
	if (rc)
		return rc;			// memory full: nz fcn value, *ppp unchanged.
	*ppp = pp;				// return good new location to caller
	pgsetup( pp, rowsnew, cols);	// store rows, cols, set offsets to segs of buf, etc.
	if (delta > 0)
	{
		p = (char *)PP->pb;
		for (i = PGRTABLESIZE; --i > 0;  )		// shift hier
			memmove( p+offnew[i], p+offold[i], sznew[i]);
		pgerase( ppp, rowsold+1);			// blank and init rows, local, below
	}
	return RCOK;
}		// pgralloce
//=========================================================================
LOCAL void FC NEAR pgsetup( 		// Set up various values in page structure

	char *pp,		// Pointer to already-allocated page
	SI rows,		// Number of rows in page (max value 255)
	SI cols )		// Number of columns in page (max value 254)

// [does not alter PP->nenhance: preserved for pgralloc.]
{
	PP->cols = cols;
	PP->rows = rows;
// offsets from &PP->pb[0] to ...
	PP->llenvo = -1;				// line lengths (row's last used 1-based col): -1 for 1-based subscript (row #)
	PP->lbegvo = PP->llenvo + rows;		// line beginnings: 1st used cols
	PP->pgbvo =  PP->lbegvo + rows - cols;	// page text buffer.  -cols for 1-based row (-1 above is for 1-based col here).
}			// pgsetup

//=========================================================================
void FC pgfree(				// Free page buffer
	char **ppp )	// Ptr to NULL or to pointer to page
{
	if (ppp != NULL)			// insurance -- NULL not anticipated 5-89
	{
		char* pp = *ppp;			// fetch NULL or page buffer ptr
		if (pp != NULL)			// if page buffer has been allocated
			dmfree( DMPP( pp)); 	// free page buffer.
		*ppp = NULL;
	}
}		// pgfree
//=========================================================================
LOCAL RC FC NEAR pgcleare(

// Clear a whole page (set to all blanks)

	char **ppp,		// POINTER TO Pointer to page to clear: [may be updated -- page could move.]
	[[maybe_unused]] int erOp )		// error action for memory full

// Fcn value RCOK if ok, other value if error and not ABT
{
// blank text, clear linebegs, linlens, enhp's; flag enhf rows free 4 reuse
	pgerase( ppp, 1);			// local, just below
	char* pp = *ppp;
// set current (last written) position to upper left corner
	PP->rlw = 1;
	PP->clw = 0;		// ie write 1st in 1,1
	return RCOK;
	/*lint -e715  erOp not referenced */
}			// pgcleare
/*lint +e715 */
//=========================================================================
void FC pgDelrows( char **ppp, SI row1, SI nrows)		// Delete rows from middle of page

// Moves any lower rows up, erases bottom.  Adjusts PGCUR row if in/below deleted area.

// for deleting printed part & keeping col heads of pgbuildr or crl table.
{
	char *pp;
	SI rows, r, rowcount;

	pp = *ppp;					// fetch page pointer
	rows = PP->rows;
	if (row1 > rows)
		return;					// insurance
	nrows = min( nrows, rows-row1+1);		// insurance

// move up rows below rows being deleted in all segments of PAGE (pgpaki.h)
	r = row1 + nrows;
	rowcount = PP->rows - r + 1;			// # rows being moved
	if (rowcount > 0)
	{
		memmove( LINELEN(row1), LINELEN(r), rowcount);
		memmove( LINEBEG(row1), LINEBEG(r), rowcount);
		memmove( PGBUF(row1,1), PGBUF(r,1), PP->cols*rowcount);
	}

// erase bottom: else lower lines would show twice
	pgerase( ppp, row1 + rowcount);			// next

// adjust row last written (for PGCUR row argument)
	if (PP->rlw >= r)    		// if below deleted area
		PP->rlw -= nrows;		// stay on corresponding row
	else if (PP->rlw > row1)		// if in deleted area
		PP->rlw = row1;   		// move to 1st row below deletion

	// more returns above
}				// pgDelrows
//=========================================================================
LOCAL void FC NEAR pgerase( 		// Erase all or lower part of a page

	char **ppp,  	// Pointer to pointer to page (dbl ptr so could realloc)
	SI rowbeg )		// First row to erase; 1 for whole page.  Erases to end.

/* Resets rows rowbeg to end of page to [unenhanced] ' '.
   Page is not reallocated (i.e. it remains same size, same place).
   [Does not eliminate enhancement flag lines associated with erased lines
    (ok but may waste space; note at beginning of file).] */
{
	char *pp;
	SI rowcount;

	pp = *ppp;
	rowcount = PP->rows - rowbeg + 1;
	if (rowcount > 0)					// insurance
	{
		memset( LINELEN(rowbeg), '\0',  rowcount);
		memset( LINEBEG(rowbeg), 255, rowcount);
		memset( PGBUF(rowbeg,1), ' ', PP->cols*rowcount);
	}
}			// pgerase
//=========================================================================
void FC pgw( 			// Write to a page in memory, no error return/options.

	// (interim? entry for easy conversion of old calls, 12-88)

	char **ppp,		// Pointer to pointer to page.	pointer may be updated
	USI fmt,		/* Format (pgpak.h) -- bits include:
			    pg size:   PGGROW: ok to add rows to page bottom of page
			    newline:   PGNLIF if not at left;  PGNLBEF before;	PGNLAFT after.
			    position:  PGRJ right justify;  PGCNTR center;
				*/
	SI row, SI col,	/* 1-based row and column for first (normally), last (if right-justified) or center character.
			   Each may be [PGMID for middle, PGEDGE for bottom / right, or] PGCUR+-n for current +- n. */
	char *s )		/* string to be written, or "" to just set curr row/col, or NULL to do NOTHING.
			   String is clipped at edges of page except see PGGROW. */
{
	pgwe( ppp, fmt, row, col, s, ABT);

}		// pgw
//=========================================================================
RC FC pgwe( 			// Write to a page in memory, inner routine

	char **ppp,		// Ptr to ptr to page.	ptr may be updated - page can move.
	USI fmt,		// Format (pgpak.h) -- bits include:
			        //  pg size:   PGGROW: ok to add rows to page bottom of page
			        //  newline:   PGNLIF if not at left;  PGNLBEF before;	PGNLAFT after.
			        //  position:  PGRJ right justify;  PGCNTR center;
	SI row, SI col, // 1-based row and column for first (normally), last (if right-justified) or center character.
	    		    //   Each may be PGCUR+-n for current +- n
	char *s,		// string to be written, or "" to just set curr row/col, or NULL to do NOTHING.
	        		//   String is clipped at edges of page except see PGGROW.
	int erOp )		// error action, and option bit:
	    		    //  PGOPSTAY: restore "cursor" row-col afterward,
				    //  so caller can use PGCUR-relative row or col again.
				    //  Intended for pgbuildr "labels", 2-90. */

// returns RCOK or error code e.g. for out of memory
{
	char *pp;
	SI clwSave, rlwSave;
	SI nbyNl;	// number of bytes to newline code \n, or end string
	SI wid;		// columns (bytes less codes) of text to \n or end
	SI xe;		// starting col adjustment for RJ or centering (0 for LJ)
	SI nbyHan;	// # bytes to enhancement change code, <= nbyNl
	SI nbyW;	// # bytes to write: portion of nbyHan not out of page
	SI r1, c1, c9, over;
	USI pos;
	RC rc;
	static USI k9999 = 9999;

	pp = *ppp;			// fetch page pointer (used by PP macro)
	rc = RCOK;					// no error
	rlwSave = PP->rlw;
	clwSave = PP->clw;	// for PGOPSTAY at return

// nop if NULL arg given -- for caller's convenience for optional stuff
	if (s==NULL)		// ADDED FEATURE 9-89, for use in crlout.cpp
		return rc;

// separate out options
	pos  = fmt & PGPMASK; 	// positioning: PGCNTR PGRJ

// leading newlines and initial row/col.  Note rlw, clw init'd to 1,0 in pgcleare.

	if (fmt & PGNLIF  &&  PP->clw != 0)	// newline if not at left
	{
		PP->rlw++;
		PP->clw = 0;
	}
	if (fmt & PGNLBEF)		// newline before text
	{
		PP->rlw++;
		PP->clw = 0;
	}

// loop over rows of text being output

	do	// {...} while (*s):  always once 5-5-89 so pgw("") sets position
	{

		// do any \n's on front of (remainder of) string
		while (*s=='\\'  &&  *(s+1)=='n')
		{
			PP->rlw++;
			PP->clw = 0;
			s += 2;
		}

// do text to next \n

		// starting row/col: resolve generics
		// why is this AFTER all the initial rlw/clw stuff?? looks like it makes features work only with PGCUR. rob 2-90
		// need option for same/col 0 after newline !?
		row = pgGenrRc( row, PP->rlw);			// evaluate PGCUR+-n etc
		col = pgGenrRc( col, PP->clw+1); 		//...

		// # bytes and display columns to \n
		nbyNl = pgcByto( &s, &k9999, "n", FALSE);	// # bytes to \n or end strng
		wid =  pgcStrWid( s, 9999, TRUE);	 	// subtract \codes, stop at \n

		// adjustment xe to column (or row) for RJ or centering
		switch (pos)
		{
			// case PGRRJ:  deleted 3-90 (no uses; not really supported)
		case PGRJ:
			xe = wid - 1;
			break;
		case PGCNTR:
			xe = (wid-1)/2;
			break;
		default:
			xe = 0;
		}

		{
			// leftover {
			while (1)	// loop over runs with same enhancements
			{

				// output a "run" of contiguous horiz text w constant enhancements

				// add rows to page if nec
				if (fmt & PGGROW  &&  row > PP->rows)	// if ok and needed
				{
					rc = pgralloce( ppp,row,erOp);	// expand page to include row
					if (rc)				// if error
						goto er;		// take error exit
					pp = *ppp;		// refetch page ptr in case realloc
				}

				// save row/col: now so pgw of "" sets curr posn for PGCUR
				PP->rlw = r1 = row;	// row
				c1 = col - xe;		// starting column
				PP->clw = c1 - 1;		/* col b4 start col: "last written" col if exit now.
	     				   (Note that col for PGCUR is clw+1 above.) */

				// determine run length, leave loop if 0, set end col
				if (!nbyNl)			// if no input text left, leave this level of loop
					break;
				nbyHan = pgcByto( &s, (USI*)&nbyNl, HANS, TRUE);	/* # bytes [to enh change code or] to \\:
			     					   breaks at \\ just to display only one \. */
				// columns is same as bytes: no codes in this substring.
				PP->clw += nbyHan;		// ending column (last written)

				// clip at page edges
				nbyW = nbyHan;			// init # bytes to write
				if (r1 < 1 || r1 > PP->rows)	// if not in page rows
					nbyW = -1;			// suppress write
				over = 1 - c1;			// overflow at left ( <= 0 if none)
				if (over > 0)
				{
					if (over > nbyNl)
						over = nbyNl;
					c1 = 1;
					s += over;		// adjust to 1st char that will fit
					nbyW -= over;
					nbyNl -= over;
					// apparently missing 5-5-89 (noticed while reading; untested; better yet clip left b4 nbyW = nbyHan):
					nbyHan -= over;
				}
				over = PP->clw - PP->cols + 1;	// overflow at right, or <= 0. +1 reserves byte for pgVrPut's /0.
				if (over > 0)
					nbyW -= over;			// truncate string to what fits

				// write to page
				if (nbyW > 0)			// if any of string still shows
				{
					memcpy( PGBUF(r1,c1), s, nbyW);	// copy to page text buffer
					if (*LINEBEG(r1) > (char)c1)		// row's 1st used col
						*LINEBEG(r1) = (char)c1;		// new low
					c9 = c1 + nbyW - 1;			// end col in row
					if (*LINELEN(r1) < (char)c9)
						*LINELEN(r1) = (char)c9;		// new hi

				}

				s += nbyHan;
				nbyNl -= nbyHan;		// pass text done
				col += nbyHan;				// col at which to continue
				// 12-88 with above col += nbyHan, looks better to omit the following, but still not perfect after \n:
				//xe = 0;			 * any text after any [enh change] in row is left-adjusted to col
			}	    // while (1) runs with same enhancements
		}	// [ if vert else horiz ]
	}
	while (*s);		// repeat while text present after \n after text

// optional final newline
er:			// errors come here to exit, rc set
	if (fmt & PGNLAFT)			// if newline after text
	{
		PP->rlw++;
		PP->clw = 0;		// do it, to .rlw/.clw for nxt call
	}

// on option, restore "cursor" position
	if (erOp & PGOPSTAY)		// feature added 2-90.
	{
		PP->rlw = rlwSave;
		PP->clw = clwSave;
	}
	return rc;				// RCOK or error code
	// another return above (NULL s)
}				// pgwe
//=========================================================================
LOCAL SI FC NEAR pgGenrRc( 		// Resolve generic row/column values

	SI rc,		// Input row or column value
	SI rcnext ) 	// Next row or column position to be written

// Returns actual row or column position implied by generic value
{
	if (rc > PGCURMIN)
		rc = rcnext+rc-PGCUR;
	return rc;
}		// pgGenrRc
//=========================================================================
void FC pgwrep( 			// Repeat write to a page in memory: used to draw lines, etc.

	char **ppp, 	// Pointer to pointer to page -- may move
	USI fmt,		// Format.  See pgwe() above, and pgpak.h.
	SI row, 		// row position for first char of string
	SI col, 		/* col position for first char of string.  Upper left corner is 1,1.
    			   Any portion of string which is outside page area is ignored
    			   except the page can grow at the bottom if PGGROW is included in fmt */
	char *s,		// string write: will be repeated as required to achieve length len.
	SI len ) 		// Length of string to be written (max 132, not checked).
{
	char stemp[134];

	*stemp = '\0';			// null string in long buffer
	pgw( ppp, fmt, row, col,
	strpad( stemp,s,len) );	// appends s's to stemp to length len

}		// pgwrep
#if 0	// no calls 11-91
x //=========================================================================
x RC FC pgcpfille(
x
x // Add text to a page at current position, with error handling
x
x char **ppp,  // POINTER TO Pointer to page - may be updated
x USI fmt,     // Format
x char *s,     // Text
x int erOp )   // error action
x
x // This is equivalent to pgfille(pp,fmt,PGCUR,PGCUR,s,erOp).  See pgfille().
x
x /* returns RCOK if ok, or other value if error, e.g. memory full, & not ABT.
x    NB on error return original block still exists and must be free'd;
x    On error or good return, block may have been moved and *ppp updated. */
x{
x     return pgfille( ppp, fmt, PGCUR, PGCUR, s, erOp);
x}
#endif
//=========================================================================
RC FC pgfille( 			// Copy text to a page in memory, handling word wrapping, with error recovery

	char **ppp,  	// POINTER TO Pointer to page.	Pointer may be updated.
	USI fmt,		/* Format (pgpak.h) -- bits include:
			    pg size:   PGGROW: ok to add rows to page bottom of page
			    newline:   PGNLIF if not at left;  PGNLBEF before;	PGNLAFT after.
			    position:  PGINDENT indent at each wrap to starting col
				*/
	SI row, SI col,	/* 1-based row and column for first character.	Each may be PGCUR+-n for current +- n
			   [or PGMID for middle, PGEDGE for bottom / right] */
	char *s,		// Null terminated string to be written
	int erOp )  	// WRN, ABT, etc to use if must enlarge page

/* returns RCOK if ok, other value if error, e.g. out of memory, if not ABT.
   Note that original page must still be freed after error, and indeed
     may have been reallocated and *ppp updated. */
{
	char *pp, *sw, *ew;
	SI colstart, wl, nbyHan;

	pp = *ppp;			// fetch page pointer
// leading newline options
	if (fmt & PGNLIF  &&  PP->clw != 0) 	// newline if not at left
	{
		PP->rlw++;
		PP->clw = 0;
	}
	if (fmt & PGNLBEF)				// newline before option
	{
		PP->rlw++;
		PP->clw = 0;
	}

// Resolve generic row/column values
	row = pgGenrRc( row, PP->rlw);		// resolve PGCUR +- n
	col = pgGenrRc( col, PP->clw+1);		// ...
	if (col < 1)
		col = 1; 		// Ignore caller stupidity
	if (fmt & PGINDENT)
		colstart = col;		// wrap to starting col
	else
		colstart = 1;		// wrap to left

// Loop over words

	while (*s)
	{
		// do any newline codes (\n) before this word
		while (*s=='\\' && *(s+1)=='n')
		{
			row++;			// wrap to next row,
			col = colstart;		// start column.
			s += 2;
		}

		// find start and end of word
		sw = s;				// init start word
		while (isspaceW(*sw))		// pass white space (whitespace is
			sw++;				//   output with word unless we wrap)
		ew = sw;				// init end word
		while (!(isspaceW(*ew))		// pass word body: non-whitespace,
		&& *ew					// non-null characters,
		&& !(*ew=='\\' && *(ew+1)=='n')	)	// not \n
			ew++;
		wl = (SI)(ew - s);		// word length incl preceding whitespace

		// wrap if necessary
		if (col + wl - 1 >= PP->cols)	// if word exceeds page right col ( = reserves byte for null)
		{
			if (*sw=='\0')		// if whitespace only
			{
				col = PP->cols;		// be sure next word wraps
				break;			// stop w/o wrapping: if wrapped now and newline followed, wd get unwanted blank line.
			}

			if (    col >= PP->cols	// if row is full
			|| ew-sw < PP->cols)	// or if wrapped word would fit
			{
				row++;			// wrap to next row,
				col = colstart;		// start column
				s = sw;			// skip whitespace
			}

			if (ew-sw >= PP->cols)	// if word won't fit on one row
				ew = s + PP->cols - col;	// split it -- else wd wrap forever

			wl = (SI)(ew - s);			// possibly changed word length
		}
		// conditional realloc
		if (fmt & PGGROW  &&  row > PP->rows)		// if ok and needed
		{
			if (pgralloce( &pp, row, erOp) != RCOK)	// enlarge / if fails
				return RCBAD;				// ret bad to caller
			*ppp = pp;		/* return updated location -- NOW so caller's pp has latest good value
	  			   in the event that a later realloc fails. */
		}
		// write word to page
		if (row >= 1 && row <= PP->rows) 	// if within page bounds
		{
			while (1)		// loop over runs [with same enhancements]
			{
				nbyHan = pgcByto( &s, (USI*)&wl, HANS, TRUE);	/* # bytes to [enh \ code or] end word
	     							   (or \\: breaks up to display only 1 \) */
				if (!nbyHan)				// if no bytes, done word
					break;
				memcpy( PGBUF(row,col), s ,nbyHan);	// copy text to page
				if (*LINEBEG(row) > (char)col)
					*LINEBEG(row) = (char)col;		// new 1st col used in row
				*LINELEN(row) = (char)(col + nbyHan - 1);	// row's last col

				s += nbyHan;
				wl -= nbyHan;		// pass bytes written
				col += nbyHan;				// update column for chars written
			}	    	// while () runs within word with same enhancements
		}	// if row not out of bounds
		else
			s += wl;	// pass the out of bounds text, else hangs
	}	    // while not out of words

// Update current position
	if (fmt & PGNLAFT)			// newline after
	{
		PP->rlw = row+1;
		PP->clw = 0;
	}
	else
	{
		PP->rlw = row;
		PP->clw = col - 1;
	}

	return RCOK;
}			// pgfille

//=========================================================================
LOCAL USI NEAR pgcByto( char **ps, USI *pnMax, char *cods, SI bsF)

// bytes to next \ code in given list, plus \\ special treatment if bsF

// leading \\: if bsF, advances over 1st \, considers 2nd \ part of text
// \x where x is in "cods", or non-leading \\: terminate
// other \'s considered part of text -- \ and following char display
{
	char c, *s, *s0;
	USI nMax, l, n;

	s = *ps;
	nMax = *pnMax;	// fetch args
	s0 = s;			// string start
	l = (USI)strlen(s);		// string length
	if (nMax > l)		// give large nMax to use entire strlen
		nMax = l;		// limit nMax to string length

// optionally check for leading double \: skip one \, consider 2nd part of text
	if (bsF && *s=='\\' && *(s+1)=='\\' && nMax >= 2)
	{
		(*ps)++; 		// skip 1st \: advance callers's
		(*pnMax)--;		// .. ptr and down-counter
		s0++;			// add 1 to place we count from
		nMax--;			// 1 less char to search
		s += 2;			// skip BOTH \'s in our search
	}

// loop over \'s til terminating combination or end string found
	while (1)
	{
		s = strchr( s, '\\');		// find a \ .
		if (!s)
			break;			// return nMax if no \ .
		n = (USI)(s - s0);		// # chars before the \ .
		if (n >= nMax)			// if max # chars or more b4 \ .
			break;			// return nMax
		c = * ++s;			// get char after \ .
		if (!c)				// if null after \ .
			break;			// return nMax
		if (strchr( cods, c)		// if c in chars given by caller
		|| bsF && c=='\\')		// or \\ (non-leading if here)
			return n;			// return bytes before the \ .
		++s;				// else point after \c
	}					// and loop to look for next \ .
	return nMax;		// no code found, return nMax (or strlen)
	// two other returns above
}		// pgcByto
//=========================================================================
LOCAL USI FC NEAR pgcStrWid( char *s, USI nMax, SI bsF)

// string width: strlen less enhancement \ codes, stopping at nMax or at \n

// if bsF, \\ is taken as 1 column
{
	char c, *s0;
	USI l, n, wid;

	s0 = s;			// save start of string
	l = (USI)strlen(s);		// string length
	if (nMax > l)		// give large nMax to use entire strlen
		nMax = l;		// limit nMax to string length
	wid = nMax;			// width is length if no \ codes
// loop over \ codes in string
	while (wid > 0)		// insurance endtest -- normally breaks below
	{
		s = strchr( s, '\\');		// find next \ .
		if (!s)
			break;			// if none, return width
		n = (USI)(s - s0);
		if (n >= nMax)			// if \ beyond input length
			break;			// return width so far
		c = * ++s;			// char after \ .
		if (!c)				// if null
			break;			// return width so far
		if (c=='n')			// \n means newline: stop before \n
			return n - (wid-nMax);	// chars to \n less \ codes
		if (bsF && c=='\\')		// if option and \\ .
			wid--;			// count only 1 col: 1 \ displays
		// else ... add any other cases
		s++;			// point after code to look for next \ .
	}
	return wid;			// another return above
}		// pgcStrWid
#if 0	// no calls 11-91
x//=========================================================================
xchar * FC pgpos( pp, row, col)
x
x// Returns the memory location of a row/column page position
x
xchar *pp;	  // Pointer to page
xSI row;
xSI col;
x{
x    return (char *)PGBUF( row, col);
x}			// pgpos
#endif
//=========================================================================
SI FC pgnrows( 			// Return number of rows in a page.

	char *pp,	  // Pointer to page.  If NULL, 0 is returned.
	USI flags )	  // Function flags; see pgpak.h.  Only PGGPTRIM is referred to here currently.

// Returns number of rows defined in page.  If PGGPTRIM is on, trailing blank lines are not included in count.
{
	SI rows, cols;

	if (pp != NULL)
		pgputrc( pp, flags & PGGPTRIM, 1, 10000, &rows, &cols);
	else
		rows = 0;
	return rows;
}		// pgnrows
//=========================================================================
void FC pgputrc( 			// Find last row and column for display

	char *pp,	 // Page pointer
	SI trimflg,	 // If TRUE, do not include blank areas at bottom and right of page in values returned
	SI row1,	 // First row to include
	SI nrows,	 // Number of rows to include
	SI *lrp,	 // Pointer for returning last row
	SI *lcp )	 // Pointer for returning last column

// Returns last row and column to be displayed
{
	SI l, r;
	SI lastrow;

	lastrow = min( SI(row1+nrows-SI(1)), PP->rows);
	if (trimflg)
	{
		*lrp = *lcp = 0;
		for (r = row1; r <= lastrow; r++)
			if ((l = *LINELEN(r)) > 0)	// last used col of row
			{
				*lrp = r;
				if (l > *lcp)
					*lcp = l;
			}
	}
	else
	{
		*lrp = lastrow;
		*lcp = PP->cols;
	}
}		// pgputrc
//=====================================================================
SI FC pgcrow( char *pp) 	// Return current row of page
{
	return PP->rlw;		// 1-based row last written, 1 if no write yet
}		// pgcrow
//=====================================================================
#if 0	// no calls 11-91
x SI FC pgcols( char *pp)
x
x // return full width in columns of page.  added 10-89.
x{
x     return PP->cols;		// width specified when page created
x}			// pgcols
#endif

#if 0	// debugging function, 7-17-90
x //==============================================================================
x void FC pgDbPrintf(		// page-related debugging printfs
x
x     char *s,		// text string (can contain \n etc) printfd before other debugging info
x     char *pp		// ptr to page about which info is reqd
x 	)
x
x{
x     printf( s);				// initial text string
x     printf( "pp=%p  text=|%-40s|",	// page seg:off, 1st 40 chars
x             pp,  PGBUF(1,1) );
x
x}			// pgDbPrintf
#endif	// debugging, 7-17-90

// end of pgpak.cpp
