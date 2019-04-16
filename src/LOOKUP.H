// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// lookup.h -- declarations associated with table lookup functions in lookup.c

/*-------------------------------- TYPES ----------------------------------*/

// word key, word value table struct for lookww
struct WWTABLE		// terminate w/ last array entry of 32767, default/not found indicator
{
   SI key, value;

};

// word key, string ptr value table structure for lookws
struct WSTABLE		// terminate w/ last array entry of 32767, default/not found indicator
{
   SI key;
   char *string;

};

// subscripted table of words with limits.  for lookswl(); for choice handle and scdefHan tables.
struct SWLTAB
{
   SI ixMin;	// smallest subscript represented (at value[0])
   SI ixMax;	// largest+1 subscript represented
   SI val[1];   // actually allocated in dm with .ixMax - .ixMin elements

};
#define SZSWLTAB(n)  (sizeof(SWLTAB) + sizeof(SI)*(n-1))

// string key, word value table structure for looksw
struct SWTABLE	// terminate w/ last array entry of NULL, default/not found indicator
{
   char *key;
   SI val;

};


/* ---------------------------- FUNCTIONS -------------------------------- */
extern SI     FC lookw( SI, SI *);   			// word table searcher
extern SI     FC lookww( SI, WWTABLE *);   		// word-word table lookup
extern char * FC lookws( SI, WSTABLE *);   		// word-string table lookup
extern SI     FC lookswl( SI index, SWLTAB* table);	// subscript-word-limits table lookup
extern SI     FC looksw(char *, SWTABLE *);		// string-word table lookup

// end of lookup.h
