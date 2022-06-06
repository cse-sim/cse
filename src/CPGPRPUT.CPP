// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cpgprput.c: virtual report [printer]-only PAGE output fcn
               CSE variant derived from ..\lib\pgprput.c 8-28-91

	       separated from pgpak.c for flexible linking. */

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <VRPAK.H>	// vrStr

#include <PGPAK.H>      // public pgpak defs and decls; decl for this file;


//=========================================================================
RC FC pgVrPut( 			// Output contents of (partial) page to [printer]/virtual report

	SI vrh, 	// virtual report handle (see vrpak.c) to receive output
	SI isFmt,	// non-0 if these are page formatting lines to omit in unformatted reports or files
	char *pp,	// Pointer to page.  If NULL, no action.
	USI flags,	// Function flag bit(s): PGGPTRIM: delete white space at bottom and right of page (pgpak.h).
	SI row1,	// Row of page at which to start
	SI nrows )	// Number of rows to print.  Also stops after last row.
{
	SI r;
	SI lastr, lastcSink;
	RC rc;

	if (pp != NULL)
	{
		pgputrc( pp, flags & PGGPTRIM, 1, PP->rows, &lastr, &lastcSink);
		lastr = min( SI(row1+nrows-1), lastr);
		for (r = row1; r <= lastr; r++)
		{
			char *p = (char *)PGBUF(r,1);		// point to row in PAGE structure
			p[*LINELEN(r)] = '\0';  		// stash a null in extra byte after last used column
			rc = vrStrF( vrh, isFmt, p);		// virtual print string, vrpak.c
			if (rc != RCOK)
				return rc;
			rc = vrStrF( vrh, isFmt, "\r\n"); 	// output newline to virtual report, vrpak.c
			if (rc != RCOK)
				return rc;
			PP->flags |= PGCON;	// say (some) printed, for cpnat.c
		}  // for (row)
	}
	return RCOK;
}               // [pgprput]/pgVrPut

// end of cpgprput.c
