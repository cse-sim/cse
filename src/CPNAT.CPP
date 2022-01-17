// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cpnat.c  paginator fcns for use with pgpak/cpgbuild.c
            CSE variant derived for ..\lib\pnat.c 8-28-91,
            reworked for virtual reports 11-91. */


/* notes on use with CSE virtual reports 11-91:

1.  When desired to use for multiple simultaneous reports, put the variables
    in this file into vrpak's VRI table to produce a copy for each report.
    (11-91 only used in CSE from cgdebug.c.)

2.  Unformatted reports:
    In calls to vrpak.c, pnat.c flags as "formatting text":
	form feeds, table heads repeated on new page, [page headers], [and page footers].
    Thus report should print showing no breaks if report or output file is unformatted.
    pnat.c goes thru its pagination process regardless; one reason for this is that
	    if pagination were suppressed here, might get huge PAGEs and run
	    out of memory (study code if need to know -- might not be true). */

#include <CNGLOB.H>

/*-------------------------------- OPTIONS --------------------------------*/
/* #define HEADERS	 * define to restore page header code.
                           Previously used (in CALRES, 1990).
			   (in CSE, Headers and Footers are added in vrUnspool, 11-91) */
/* #define FOOTERS	 * define for (untested 2-90) code for page footers
			   (and remove f's in col 1).  Incomplete
			   re space-down and pagination (?? comments). */

/*------------------------------- INCLUDES --------------------------------*/
#include <VRPAK.H>	// vrStrF

#include <PGPAK.H>	// declarations for this file

/*------------------------------- COMMENTS --------------------------------*/
/* usage: expected call sequence (written b4 vr's):
*    start of report
*       init [printer or] virtual report (elsewhere)
*       print first page header if any
*       pnSetTxRows() if data-init pnLpp value not to be used
*       make up footer and continuation page header pgpak PAGEs if any
*       pnSetHeader and pnSetFooter if any
*    each table or section of report (much done by pgbuildr calls from appl)
*       pnAlloc()		initialize and alloc page
*       pnTitle()		do title if any
*       pnSetTh()		set # table head rows, if any
*       put text in page with pgpak fcns and/or pgbuildr
*       if conditional pagination desired:
*         pnPgIf( , badness, 0)	at possible page breaks.
*         after pnPgIf(), all row specifications must be PGCUR-relative as
*           printed rows may be deleted from PAGE.
*       if caller decides where to break page:
*         pnPgIf( , 1, 0)  (or mak pnNewPrPage public?)
*       pnPrPg()			to print (any remainder if pnPgIf used)
*       pnFree()			release page storage
*    end of report
*       print footer on last page. page # already current in it.
*             hmm... otta be a fcn here to space down and do it.
*       close [printer or] virtual report */

/*---------------------------- INITIALIZED DATA ---------------------------*/
/* # Text lines on a printer page: we form feed after this many lines, or fewer.
   Settable via fcn pnSetTxRows.  Used in getLlop.
   (cncult:getBodyLpp not called from here for modularity) */
LOCAL SI NEAR pnLpp=54;	/* default 54 here for hdrs/ftrs added 11-91.
			   Note max incl hdrs/ftrs is 60 for generic printers:
			   If 62, HP LaserJet puts 2 lines on next page. */

/*---------------------------- LOCAL VARIABLES ----------------------------*/

/*---- Variables used til changed by application, typically for whole report:*/

// current virtual report, set by pnSetVrH:
LOCAL SI NEAR pnVrh = 0;

/* CAUTION: pnat can only be used for one virtual report at a time until
   following variables are replicated per vr (eg put in vrpak's vr[] table, etc).
   In that case, does vrh want to become an arg to each call?
   (11-12-91: cpnat/cpgbuild only used one place -- from cdebug -- so ok) */

#ifdef HEADERS
h  /* for page header and footer: pointer (or NULL) to PGPAK "PAGE" with text,
h     and page number position information: row (or 0), column, and pn format.
h     Format is: PGLJ (0) to left-align "Page n" at col,
h                or PGRJ to right-align: col is rightmost col of "Page n". */
h  LOCAL char ** NEAR hdPpp = NULL;
h  LOCAL SI NEAR hdPnRow = 0, hdPnCol = 0, hdPnFmt = 0;
#endif
#ifdef FOOTERS
f  LOCAL char ** NEAR ftPpp = NULL;
f  LOCAL SI NEAR ftPnRow = 0, ftPnCol = 0, ftPnFmt = 0;
#endif

/*---- Variables used for one table or report section; auto reset at
       pnFree or pnAlloc; handled from within pgbuildr or by appl: */

/* 0 or number of "table head" rows: printed at first print of pgpak PAGE and
   whenever it is continued to new print page; retained in PAGE but not
   printed when more is printed on same print page. */
LOCAL SI NEAR nThRows = 0;	/* set: pnSetTh;  used: pnPgIf, pnPrRows */

/* where, in the "table head rows", to add "continued",
   if table is continued on next print page. */
LOCAL SI NEAR thConRow = 0;	/* 0 to suppress.  Set by pnTitle, ... */
LOCAL SI NEAR thConCol = 0;	/* ... used by pnPrRows. */

/*---- Variable used for one print page */
LOCAL SI NEAR pnBadness = 0;	/* see pnPgIf(). "badness" of point to which
				   have already printed this table on this
				   print page: whenever a less "bad" page
				   break comes along, print to it.
				   Cleared in pnAlloc/pnFree and pnPrPg. */


/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL RC FC NEAR pnPrRows( char **ppp, USI flags,
						   SI delrows, SI row1, SI nrows, SI x);
LOCAL RC FC NEAR pnNewPrPage( void);
LOCAL SI FC NEAR getLlop( void);


//===========================================================================
void FC pnSetVrh( SI vrh)	// set virtual report handle for subsequent output
{
	pnVrh = vrh;		// store argument in local variable for use in other fcns in this file
}		// pnSetVrh

//===========================================================================
void FC pnSetTxRows( SI lpp)	// set number of lines of TEXT on each print page
{
	pnLpp = lpp;	// to file-global for use in getLlop
}

#ifdef HEADERS
h //===========================================================================
h void FC pnSetHeader(
h
h /* specify continuation page header.
h
h 	Is used till changed.  Page number is optionally inserted here.
h 	Caller does first page header (may be different) */
h
h char **ppp, 	/* pointer to pgpak PAGE containing header text, or NULL.
h 		   Is not PGGPTRIMed -- include only desired whitespace. */
h SI fmt, 	/* page # format: PGLJ: left-adjust "Page n", start at col;
h 		                  PGRJ: right-adjust, last digit in col. */
h SI row, SI col)	/* where to put page number in header (row 0 suppresses) */
h
h{
h     /* store in file-globals for pnPrPg */
h     hdPpp = ppp;	hdPnRow = row;	hdPnCol = col;	hdPnFmt = fmt;
h	/* pnSetHeader */
#endif
#ifdef FOOTERS
f//===========================================================================
fVOID FC pnSetFooter(
f
f/* specify page footer.
f
f	Is used till changed.  Page number is optionally inserted here.
f	Caller does last page footer (but pn is up to date from here). */
f	char **ppp, 	/* pointer to pgpak PAGE containing footer text, or NULL>
f		   Is not PGGPTRIMed -- include only desired whitespace.  */
f	SI fmt, 	/* page # format: PGLJ: left-adjust "Page n", start at col;
f		                  PGRJ: right-adjust, last digit in col. */
f	SI row, SI col ) /* where to put page number in footer (row 0 suppresses) */
f    /* store in file-globals for pnPrPg */
f {
f    ftPpp = ppp; 	ftPnRow = row;	ftPnCol = col; 	ftPnFmt = fmt;
f
f    ?? must determine number of rows footer requires (incl any margin)
f    and store in a file-global for getLlop().
f
f	}		/* pnSetFooter */
#endif	/* FOOTERS */

//===========================================================================
RC FC pnAlloc( char **ppp, SI rows, SI cols, int erOp)

/* allocate a pgpak PAGE for use thru cpnat.c */
/* clears per-table stuff: "continued" posn, # head rows */

/* returns non-RCOK if allocation error, e.g. out of memory */
{
	pnFree( ppp);		/* free any prior PAGE, 0 per-table stuff */
	return pgalloce( rows, cols, ppp, erOp);
	/* allocate/init pgpak "PAGE" structure
		   to hold text to print. pgpak.c. */
}		/* pnAlloc */

//===========================================================================
void FC pnFree( char **ppp)

/* free a pgpak PAGE, primarily for when cpnat.c is being used */

/* clears per-table stuff: "continued" posn, # head rows */
{
	/* free the page (if any) */
	pgfree(ppp);
	/* clear stuff that normally changes for each PAGE (table, report sect, etc) */
	nThRows = 0;		/* no table head rows to reprint on new page*/
	thConRow = thConCol = 0;	/* no place to add "continued" for new page */
	pnBadness = 0;		/* see pnPgIf.  Say have not yet printed to a
    				   bad page break on this print page in this
    				   table. */
}		/* pnFree */

//===========================================================================
void FC pnTitle(

	/* set/display title for current pgpak PAGE */

	/* title text is pgw'd and position of end is remembered for
	   adding "continued" if table header is repeated on new print page.*/

	char **ppp,	/* (used to put s in page)
		   CAUTION info retained for only 1 page at a time */
	SI row,		/* 0 or title row */
	SI col, 	/* 0 or title text left col if text given,
		   or exact left col for "continued" if s is NULL */
	char *s ) 	/* title text, NULL for none */
{
	thConRow = row;
	thConCol = col;		/* strlen(s)+1 conditionally added below */
	if (s != NULL)		/* if text given */
		if (row > 0 && col > 0)		/* if have row & col */
		{
			pgw( ppp, PGLJ, row, col, s);  	/* put title text in page */
			thConCol += (SI)strlen(s) + 1;    	/* put "continued"
	       					   1 column beyond title. */
		}
}			/* pnTitle */

//===========================================================================
void FC pnSetTh(

	/* set number of table header rows for current pgpak PAGE */

	SI thRows )	/* # head rows, 0 for none: rows of PAGE retained
		   for reprint if continued on new print page. */
{
	nThRows = thRows;		/* to file-global for pnPgIf, pnPrRows */
}			/* pnSetTh */

//===========================================================================
RC FC pnPgIf(

	/* conditional page (conditional print-to-here) */

	/* starts new line if "crsr" not at start of a line */
	char **ppp,
	SI badness,	/* "badness" of current (PGCUR) row as start of new page:
		   1 = best .. 30 or so = worst.  Is also # lines in which
		   to print to here now, i.e. to commit to stuff above being
		   on this page (if it fits), and thus page here if no
		   further equally good break occurs on this page. */
	SI x )		/* extra lines below end of PAGE to allow for in pagination:
		   allows "protecting" following rows */

/* returns non-COK if error eg print stopped by alt-break (already messaged).
   CAUTION: do not test for specific code -- may be or'd together. */
{
	char *pp;
	SI llop, row, nrows;
	RC rc = RCOK;

	/* PRINT TO HERE IF:
	   1. Current PAGE exceeds printer page: pnPrPg will start new page using a
	      prior break or start of page.  Then this break is saved for possible
	      later use on the new page.
	   2. More than "badness" space on this printer page: will print on current
	      page, and will later break here (or farther down).  Printing to break
	      is how we save potential break point.
	   3. This break not "worse" than last used ("badness" not larger): this
	      is how we get the best break available, and get the farthest down
	      of equal breaks.

	   DO NOT PRINT NOW IF: current PAGE will fit on curr printer page and space
	   left is less than "badness": do not want to break here this near end page
	   -- push all text to next page (at a future call) unless all fits to end
	   of report or less "bad" break.  Except if have already printed some of
	   PAGE on this print page, and "badness" not larger, DO print to get to
	   better break or equal break farther down on page. */

	/* start a new line if not at start of a line */
	pgw( ppp, PGNLIF, PGCUR, PGCUR, "");

	/* some init */
	pp = *ppp;
	llop = getLlop() - x;	/* lines left on printer page, below,
    				   less extra lines caller said to leave */
	row = pgcrow(pp)-1;  	/* row # (PGCUR) on PAGE, pgpak.c,
    				   less 1 for last row ABOVE cursor */

	/* # rows to print. pnPrRows handles header rows; body rows .. */
	nrows = row - nThRows;	/* .. already printed have been deleted. */

	/* the big decision */
	if ( nrows > llop		/* if text won't fit on this print page */
	|| llop > badness		/* if space on page > clr's min to print in */
	|| badness <= pnBadness )	/* if this break no worse than one already
      				   printed to in this table on this page */
	{
		/* print to here: commit to put at least this must text on this page */
		rc |= pnPrRows( ppp, 0, TRUE, nThRows+1, nrows, x);
		/* print & delete rows. Handles table header
		   rows, new page 1st if necessary. below. */
		pnBadness = badness;	/* "badness" of break to which have printed.
	       			   Cleared in pnFree/pnAlloc and pnPrPg. */
	}
	/* else do nothing: page will break at point of a prior or later call. */
	return rc;
}		/* pnPgIf */

//===========================================================================
RC FC pnPrPg( char **ppp, SI x)

/* print pgpak PAGE (finish if some already printed) */

/* x: extra rows to leave below PAGE on page -- go to new page if necess. */

/* returns non-RCOK if error eg print stopped by alt-break (already messaged).
   CAUTION: do not test for specific code -- may be or'd together. */
{
	/* request printing of entire page with bottom whitespace trimmed.
	   pnPrRows finds last line,
	            does not duplicate table head rows if already printed.
	   Any non-tbl-head rows already printed have been deleted (as of 2-90). */

	return pnPrRows( ppp, PGGPTRIM, FALSE, 1, 9999, x);

}		/* prPrPg */

/* 7-4-90, 6.0: found compiled wrong with FC and -Osewzr: fetched BP+6,
   which I claim would be x, then tested in for > 0 as tho had nrows.
   Caused hang and/or qemm exceptions.  OK if cv-compiled.
   FC args: ppp stk, flags AX, delrows DX, row1 BX, nrows stk, x stk.
   removing FC FIXED it for the moment ...
Later 7-4-90, 6.0: under -Oswr, without FC, apparently compiled wrong again:
   to test for nrows > 0, tested BP+E (row1) not BP+10 (nrows).
   Restore FC, try pragma.  */
#pragma optimize("",off)	/* 6.0  7-4-90  compiled wrong */
/*   not reverified with final -O options */
//===========================================================================
LOCAL RC FC NEAR pnPrRows(

	/* print some or all of a PAGE */

	/* starts new page if nec;
		handles table header portion of page and "continued" in title
	 if previously specified with fcns pnSetTh, pnSetTitle above.
		If more than a pageful of rows specified (not expected),
	 pages first and every pnLpp rows. */
	char **ppp,
	USI flags, 	/* PGGPTRIM to not print bottom white space (& right) (pgpak.h) */
	SI delrows,	/* TRUE to delete printed rows (except table head) */
	SI row1, 	/* start row, 1-based (table head rows excluded here) */
	SI nrows,	/* number of rows of PAGE to print */
	SI x )		/* additional rows that caller says must fit same print page */

/* returns non-RCOK if error eg print stopped by alt-break (already messaged).
   CAUTION: do not test for specific code -- may be or'd together. */
{
	char *pp;
	SI contin, pth, r, lneed, ltp;
	RC rc = RCOK;

	pp = *ppp;
	while (nrows > 0)		/* repeat if > pageful (unexpected) */
	{
		contin = PP->flags & PGCON;	/* nz if some of PAGE alredy printed */
		pth = !contin;			/* print tbl header if 1st print */

		/* determine number of rows needed if not on new print page */
		if (row1 <= nThRows)		/* auto exclude head rows .. */
		{
			nrows -= nThRows - row1 + 1;	/* .. needed when called from pnPrPg*/
			row1 = nThRows + 1;		/* .. also fixes row 0 to 1 */
		}
		r = pgnrows( pp, flags);	/* # rows on page, does PGGPTRIM. pgpak.c */
		if (r < row1)
			return rc;				/* nothing to do */
		nrows = min( nrows, r - row1 + 1);  	/* reduce nrows to actual */
		lneed = nrows + x + (pth ? nThRows : 0);	/* # rows needed to print */

		/* start new printer page if necessary */
		if (lneed > getLlop())	/* Lines left on page fcn is below */
		{
			rc |= pnNewPrPage();	/* footer, form feed, page hdr. next */
			pth++;   		/* do print any table heading each new page */
		}

		/* print or reprint table heading if appropriate */
		if ( pth		/* if 1st print this PAGE, or new printer page*/
		&& nThRows )	/* if have a table header */
		{
			if (contin				/* if not 1st print */
			&&  thConRow > 0  &&  thConCol > 0 )	/* and have position info */
			{
				SI tem1 = PP->rlw, tem2 = PP->clw;
				pgw( &pp, PGLJ, thConRow, thConCol, "continued ");	/* pgpak.c */
				PP->rlw = tem1;
				PP->clw = tem2;		/* restore PAGE cursor position */
			}

			/* 'contin' non-0 in following call indicates page formatting text:
			      table head repeated at table continuation on a new page.
			   vrPak.c (from pgVrPut) will omit this text in unformatted report or file.
			   Note that initial printing of table head is NOT format-flagged,
			      as it should print even in un-page-formatted reports. rob 11-91.
			   Form feeds, from pnNewPrPage, are also format-flagged. */

			rc |= pgVrPut( pnVrh, contin, pp, 0, 1, nThRows);	// virtual print rows, app\cpgprput.c
		}

		/* print and delete rows */
		ltp = min( nrows, getLlop() );	/* not more than pageful on page */
		rc |= pgVrPut( pnVrh, 0, pp, flags, row1, ltp);	/* virtual print rows, app\cpgprput.c.
							   sets PP->flags & PGCON. */
		if (delrows)			/* delete for pnPgIf, not for pnPrPg.*/
			pgDelrows( &pp, row1, ltp);	/* delete printed rows. pgpak.c.
    					   adjusts PGCUR. */
		else				/* if not deleted */
			row1 += ltp;			/* start row for next iteration */
		nrows -= ltp;			/* reduce row count, usually to 0 */
	}	/* while nrows > 0 */
	*ppp = pp;				/* in case something moved PAGE */
	return rc;
}			/* pnPrRows */
#pragma optimize("",on)			/* restore */

//===========================================================================
LOCAL RC FC NEAR pnNewPrPage()	/* start new (continuation) printer page */

/* returns non-RCOK if error eg alt-break (already messaged). */
{
	RC rc = RCOK;

#ifdef FOOTERS
f/* footer. page # was updated at prior call. */
f    rc = pgVrPut( pnVrh, 1, ftPpp, 0, 1, 9999);	// virtual print page footer (app\cpgprput.c)
#endif
	/* advance printer to top of next page */
	vrStrF( pnVrh, 1, "\f");	// output form feed to virtual report (vrpak.c).
	// 1 indicates "page formatting text" for vrpak to omit if unformatted output.

#ifdef HEADERS
h /* update page numbers if any in header and footer */
h     if (hdPnRow)			/* if hdr has a page # for us to set*/
h     {	pgw( hdPpp, 			/* write into PAGE, pgpak.c */
h             hdPnFmt, hdPnRow, hdPnCol,
h             strtprintf("Page %d",			// printf and return ptr (strpak.c)
h                        (INT)vrGetPage(pnVrh) ) );	// get virtual page # (vrpak.c)
h	  }
#endif
#ifdef FOOTERS
f    ?? logic missing here to space down to proper footer position
f    if (ftPnRow)
f    {	rc |= pgw( ftPpp, ftPnFmt, ftPnRow, ftPnCol,
f                  strtprintf( "%d",
f                               (INT)vrGetPage(pnVrh) );
f	 }
f    /* footer is now ready for foot page just started,
f       including last page where caller prints it */
#endif
#ifdef HEADERS
h /* header */
h     if (hdPpp)					/* if there is a header */
h        rc |= pgVrPut( pnVrh, 1, *hdPpp, 0, 1, 9999);	// "print" all rows to virtual report, app\cpgprput.c
#endif

	/* file-global variable for conditional pagination logic */
	pnBadness = 0;	/* see pnPgIf().  Say have not yet printed to a bad
			   page break on this print page in this table.
			   Note crl pagination on which this is modelled
			   did not clear this on new print page!?. 2-90. */
	return rc;
}			/* pnNewPrPage */

//===========================================================================
LOCAL SI FC NEAR getLlop( void)	/* get number of lines left on printer page */
{
#ifdef FOOTERS
f   ?? need to subtract number of rows that must be reserved for footer
#endif
	return pnLpp 			/* lines per page, top of this file */
	- vrGetLine(pnVrh);		// line # on virtual page, vrpak.c
}			/* getLlop */

// end of cpnat.c
