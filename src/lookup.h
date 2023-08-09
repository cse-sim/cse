// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// lookup.h -- declarations associated with table lookup functions in lookup.c

/* ---------------------------- FUNCTIONS -------------------------------- */
extern int lookw(int key, const int* table);  				// word table searcher
extern int lookww(int key, const struct WWTABLE* table);			// word-word table lookup
extern const char* lookws(int key, const struct WSTABLE* table);		// word-string table lookup
extern int lookswl(int index, const struct SWLTAB* table);			// subscript-word-limits table lookup
extern int looksw(const char*, const struct SWTABLE* table, bool bCaseSensitive=false);		// string-word table lookup

/*-------------------------------- TYPES ----------------------------------*/

// word key, word value table struct for lookww
struct WWTABLE		// terminate w/ last array entry of 32767, default/not found indicator
{
   int key, value;
   int lookup( int _key) const
   {
	   return lookww(_key, this);
   }

};

// word key, string ptr value table structure for lookws
struct WSTABLE		// terminate w/ last array entry of 32767, default/not found indicator
{
   int key;
   const char *string;

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
   const char* key;
   int val;

};



// end of lookup.h
