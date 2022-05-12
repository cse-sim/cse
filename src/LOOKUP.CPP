// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// lookup.c -- various table look up functions.

/* ----------------------------- INCLUDES -------------------------------- */
#include <cnglob.h>

#include <lookup.h>	// decls for this file; structure definitions for WWTABLE etc


//===========================================================================
SI FC lookw( 	// word table searcher

	SI key,		// value sought in table
	SI table[] )	// pointer to array of ints, ended by 32767

// fcn value is 0-based position in table or -1 if not there
{
	SI *sip;

	sip = table;
	while (*sip != 32767)
	{
		if (*sip==key)
			return (SI)(sip - table);	// found, return subscript
		sip++;
	}
	return -1;          // not found

}       // lookw
//===========================================================================
SI FC lookww( 	// word-word table searcher -- general use routine

	SI key,
	const WWTABLE *table )	// table is key, value pairs, ended by 32767, default

// fcn value is table value for key, or default (last entry) if not found
{
	while (1)
	{
		if (table->key==key || table->key==32767)
			return table->value;
		table++;
	}
}               // lookww
//===========================================================================
const char* FC lookws( 	// word-string table searcher

	SI key,
	const WSTABLE *table )	// key, string pairs, ended by 32767, default

// fcn value is table value for key, or default (last entry) if not found
{
	while (1)
	{
		if (table->key==key || table->key==32767)
			return table->string;
		table++;
	}
}               // lookws

//===========================================================================
SI FC lookswl( SI index, const SWLTAB* table)

// retrieve member from subscripted word with limits table (lookup.h)

/* related code: rcdef.c generates tables of this form
		 and contains a copy of this function. */

// returns contents of table slot, or 0 if index out of range
{
	if (index < table->ixMin || index >= table->ixMax)
		return 0;
	return table->val[ index - table->ixMin ];
}                                       // lookswl

//=========================================================================
SI FC looksw(			// string/word table lookup

	const char *string,	// String sought
	const SWTABLE *swtab )	// Table in which to look, terminated with NULL

/* Returns value in table corresponding to name.  If not found,
   returns entry corresponding to NULL in table */
{
	SI i = -1;
	while ((swtab+(++i))->key != NULL)
	{
		if (compareCaseStr(string,(swtab+i)->key) == 0)
			break;
	}
	return (swtab+i)->val;

}				// looksw

// end of lookup.c
