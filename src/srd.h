// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* srd.h: definitions for "small record (& field, etc) descriptors":
	  data and record definition info generated as C source code by rcdef.exe,
	  for compilation and linking into programs, rcdef-generated source files:

	      dttab.c: data types table
		  untab.c: units table
	      srfd.c:  field types table, fields-of-records tables, small record descriptor table.

	  comments written early in development 1-30-91; may be obsolete */

#ifndef SRD_H
#define SRD_H

//=============== DATA TYPES: dttab.cpp

//--- Data type word: DTxxx define in dtypes.h, generated by rcdef from cndtypes.def.
//    Hi bits are attributes, lo bits index (subscript) into Dttab.
#define DTBCHOICN 0x4000	// float number-or-choice data type: choice values (hi word)(nans) are 7f81, 7f82, ...
#define DTBCHOICB 0x2000	// "improved" simplified choice data type: Internal values are 1, 2, 3...
#define DTBMASK   0x1fff	// mask for lo bits that index into Dttab[] to get size & choice info.  See rcdef.c.


//--- Data type table

// data type table globals - but access using GetDttab(), below
extern ULI Dttab[];   		// dttab.cpp, generated by rcdef.exe
extern size_t Dttmax;		//   size (in ULI's) of Dttab

// structure for accessing data type table entry
//			(Dttab may be constructed by rcdef.exe as array of ULI, with subscript dt & DTBMASK,
//                      size in loword, # choices in hiword, char * choice text ptrs following.)
struct DTTAB
{   // next two correspond to LOWORD and HIWORD, respectively, of ULI Dttab[masked dt]
     SI size;        	// storage size of choice data type (2)
     SI nchoices;    	// # choices (incl hidden choices) <-- 0 (HIWORD) in non-choice type, cd use as choice type indicator.
    // next member corresponds to Dttab[dt+1...]
     char* choicbs[1];	// array of text pointers (compiled/linked Dttab) for texts of choices 1,2,3 ... .
						// Texts of hidden choices start with '*'.
};

//- function for accessing data type table entry: .size, .nchoices, .choicbs[0..].
inline DTTAB& GetDttab(USI dt) { return *(DTTAB *)(Dttab+((dt)&DTBMASK)); }

//- function for accessing text for a choice. for choicn 'chan' is CHN(value). CHN, NCNAN: cnglob.h.
//   but, to access with checks, use ::getChoiTx( dt, chan).
inline const char* GetChoiceText( USI dt, USI chan) { return GetDttab(dt).choicbs[(chan & ~NCNAN) - 1]; }

//-- expr() data types --   each type is different bit so can represent groups of types and test with 'and'.  Data type USI.
#define TYSI    0x01		// 16-bit integer (or BOOlean)
#define TYFL    0x02		// float
#define TYSTR   0x04		// string
#define TYFLSTR (TYFL|TYSTR)	// number (as float) or string, known at compile time. for CSE reportCol.colVal.
#define TYID	0x08		// id: string; quotes implied around non-reserved identifier (for record names) at outer level
#define TYCH   0x10		// choice (DTxxx must also be given): string or id, conv to 16/32 bit choicb/choicn value
#define TYNC   (TYFL|TYCH)	// number (float) or specified 32 bit choicn choice, not necess known til runtime.
//	TYDOY	day of year: def in cul.h; given to cuparse.cpp as TYSI, 2-91.
#define TYINT  0x20			// 32-integer
//              0x40-0x80	reserved for expansion of above
#define TYNUM   (TYSI|TYINT|TYFL)	// "any numeric data type" (test by and'ing)
#define TYANY	0x1f		// any value ok: 'or' of all of above
//      TYLLI   0x100		not used in cuparse: exman.cpp changes to TYSI.
#define TYNONE  0x200		// no value (start of (sub)expression)
#define TYDONE  0x400		// complete statement: no value on run stack
// CAUTION 0xf000 bits used in cul.h

//-- exman.cpp:exPile data types --
#define TYLLI  0x100		// limited long integer: 16 bit value in 32 bit storage to support nan-flagging
//TYSI   CAUTION  cannot hold nan-flags, use TYLLI (with 32 bit storage!)
//		  everywhere exman's runtime expressions are to be supported
//TYSTR TYID TYFL TYCH TYNC as above
// not used in exman.cpp: TYNUM, TYANY, TYNONE, ? TYDONE.



///////////////////////////////////////////////////////////////////////////////
// UNITS
///////////////////////////////////////////////////////////////////////////////
// units table, subscripted by unit type (units.h, also created by rcdef.exe):
extern int Nunsys;		// number of unit systems (2)
extern int Unsysext;	// current external unit system (internal units are unaffected by units selection)
extern struct UNIT Untab[];
struct UNIT			// one unit type's info in Untab[]
{   struct
    {  const char* un_symbol;	// print symbol "F" "BTU/ft2"
       double un_fact;			// conversion factor
    } unsys[ 2];         		// index is current unit system Unsysext (untab.c)
	static const char* GetSymbol( int iUn, int iUx=Unsysext) { return Untab[ iUn].unsys[ iUx].un_symbol; }
	static double GetFact( int iUn, int iUx=Unsysext) { return Untab[ iUn].unsys[ iUx].un_fact; }
};



/*========== LIMITS: no tables -- implemented in code, e.g. in lib\cvpak.c */



/*========== FIELDS and RECORDs

	Note: fields and records info is in same file, srfd.cpp, to be sure
	matching field type indeces are used: field type indeces, used only
	to access field types table from fields-in-record table, are not
	necessarily invariant amoung products: #if-ing-out field types
	anywhere in fields.def is expected.  (But a record/field description,
	if present in srfd.c, should be valid for that RT in any product). */


//--- Field types table: srfd.c:sFdtab[]

struct SFDTAB	// struct of one field's info in sFdtab[]
{   USI dtype;		// data type, DTXXXX define, Dttab[] index + bits.
    UCH lmtype;		// limit type or LMNONE, LMXXXX define
    UCH untype;		// units type or UNNONE, UNXXXX define, Untab[] index.
};
extern SFDTAB sFdtab[];			// array of SFDTAB, indexed by field type # in fields-in-record tables

//#define FDNONE 0	define if desired.	// 0 is not a valid field type #.


/*--- Small Fields-in-Records Tables (srfd.c):
	Each gives info on fields in one record type.
	Indexed by field number (RECNAME_FIELDNAME define in rcxxxx.h file).
	Accessed pointer in basAnc or (obsolescent) via RT's entry in small record descriptor, below. */
struct SFIR
{
    UCH fi_ff;		// field flags (attributes): define(s) below: FFHIDE, FFBASE. 2 bits used 6-95.
    UCH fi_fdTy;	// field type: sFdtab index. 7 bits used, almost 8, 6-95, expand using ff bits when need found.
    USI fi_evf;		// field variation (by program, see cncult.c for input variability) (EVF___ defines, cuevf.h). 9 bits 6-95.
	SI fi_nxsc;		// fn of next field requiring special (non-bitwise) code for e.g. copy
					//   -1 = not special case
					//   else this field may require special handling and links to next
					//   supports e.g. general record::Copy().  6-2023
    USI fi_off;		// member offset in rec. 14 bits needed 6-95.
    const char* fi_mName;	// record struct MEMBER name.  for arrays & nested structs, contains composite with .'s and/or [n]'s.
	
	int fi_GetDT() const { return sFdtab[fi_fdTy].dtype; }
	const char* fi_GetMName() const { return fi_mName; }
};

/*--- Field Flag bits, for SFIR.fi_ff */
inline constexpr UCH FFHIDE{ 1};	// hide field: omit field from probe info report (CSE -p)
inline constexpr UCH FFBASE{ 2};	// field of C++ base class specified via *BASECLASS in records definition file, 7-1-92.


/*----- re Record Types
        Hi bits are attributes; lo are type #
	Individual record types are defined in rctypes.h and are used in .rt, the 1st member of all record structures. */
#define RTBRAT   0x2000   /* "Record Array Table" type: has array of record structure in a DM block.  See ancrec.h, .cpp.
			     also defined in ancrec.h, MUST MATCH!. */
#define RTBHIDE  0x4000	  // hide record: omit entire record from probe info report (CSE -p)
#ifdef wanted	// define here if desired to test outside of rcdef.exe (rcdef defines internally ifndef, 12-91.)
w #define RTBSUB   0x8000   /* Substructure type definition, only for nesting in other record types
 			       or direct use in C code: Lacks record front info and stat bytes. 3-90.
			       rcdef.exe sets this bit for internal reasons, and leaves it set. */
#endif
#define RCTMASK  0x03FF 	// bit-remove mask for SRDstr.rt: yields record type
#ifndef GLOB			// defined in cnglob.h due to frequent use
  typedef USI RCT;		// record type type
#endif

#if !defined( NODTYPES)
struct VALNDT
{
	NANDAT vt_val;		// must be first, input data placed here
	USI vt_dt;			// data type; initially TYFL/TYSTR
						//   DTFLOAT/DTCULSTR at runtime

	VALNDT() : vt_val( 0), vt_dt( DTNONE)  {}
	~VALNDT()
	{
		vt_ReleaseIfString();
	}
	void vt_SetDT(USI ty);	// map TYFL/TYSTR to DTFLOAT/DTCULSTR

	bool vt_IsString() const
	{	// return true iff vt_val is CULSTR
		bool isString = vt_dt == DTCULSTR;
#if defined( DEBUG)
		if (!isString && vt_dt != DTNONE && vt_dt != DTFLOAT)
			printf("\nvt_IsString: unexpected vt_dt=%d  vt_val=%x", vt_dt, (unsigned int)(vt_val));
#endif
		return isString;
	}
	bool vt_IsNANDLE() const { return ISNANDLE(vt_val); }
	void vt_ReleaseIfString();
	void vt_FixAfterCopyIfString();
};
#endif	// NODTYPES

//=============================================================================
// struct MODERNIZEPAIR: single word modernize, maps old -> current
//    used re providing input file backwards compatibility
//    e.g. probe name moderization
struct MODERNIZEPAIR		// single word modernize: old -> current 
{
private:
	const char* mp_oldWord;		// prior word
	const char* mp_curWord;		// current (modern) equivalent

public:
	constexpr MODERNIZEPAIR(const char* oldWord, const char* curWord)
		: mp_oldWord{ oldWord }, mp_curWord{ curWord }
	{}
	bool mp_IsEnd() const
	{
		return mp_oldWord==nullptr;
	}
	bool mp_ModernizeIf(
		const char* &word) const	// word that may need modernizing
									//  e.g. from user input
									// returned updated if needed
	// returns true iff word modernized (word updated)
	//    else false
	{
		bool bMatch = _stricmp(word, mp_oldWord) == 0;
		if (bMatch)
			word = mp_curWord;
		return bMatch;
	}

};	// struct MODERNIZEPAIR

#endif 	// ifndef SRD_H at start file

// end of srd.h
