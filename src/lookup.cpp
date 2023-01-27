// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// lookup.cpp -- various table look up functions.

/* ----------------------------- INCLUDES -------------------------------- */
#include "cnglob.h"

#include "lookup.h"	// decls for this file; structure definitions for WWTABLE etc


//===========================================================================
int lookw( 	// word table searcher

	int key,		// value sought in table
	const int* table)	// pointer to array of ints, ended by 32767

// fcn value is 0-based position in table or -1 if not there
{
	const int* sip = table;
	while (*sip != 32767)
	{
		if (*sip==key)
			return (sip - table);	// found, return subscript
		sip++;
	}
	return -1;          // not found

}       // lookw
//===========================================================================
int lookww( 	// word-word table searcher -- general use routine

	int key,
	const WWTABLE* table )	// table is key, value pairs, ended by 32767, default

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

	int key,
	const WSTABLE* table )	// key, string pairs, ended by 32767, default

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
int lookswl( int index, const SWLTAB* table)

// retrieve member from subscripted word with limits table (lookup.h)

// returns contents of table slot, or 0 if index out of range
{
	if (index < table->ixMin || index >= table->ixMax)
		return 0;
	return table->val[ index - table->ixMin ];
}                                       // lookswl

//=========================================================================
int looksw(			// string/word table lookup

	const char* string,	// String sought
	const SWTABLE* swtab )	// Table in which to look, terminated with NULL

// Returns value in table corresponding to name.
// If not found, returns entry corresponding to NULL in table
{
	int i = -1;
	while ((swtab+(++i))->key != NULL)
	{
		if (_stricmp(string,(swtab+i)->key) == 0)
			break;
	}
	return (swtab+i)->val;

}				// looksw
//=========================================================================
int looksw_cs(			// string/word table lookup, case sensitive

	const char* string,	// String sought
	const SWTABLE* swtab)	// Table in which to look, terminated with NULL

	// Returns value in table corresponding to name.
	// If not found, returns entry corresponding to NULL in table
{
	int i = -1;
	while ((swtab + (++i))->key != NULL)
	{
		if (strcmp(string, (swtab + i)->key) == 0)
			break;
	}
	return (swtab + i)->val;

}				// looksw_cs

// end of lookup.cpp
