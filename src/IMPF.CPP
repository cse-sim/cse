// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// impf.cpp  Imports compile and runtime code for CSE


/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>	// cse global header. ASSERT.
#include <fcntl.h>	// O_RDONLY O_BINARY

#include <ancrec.h>	// record: base class for rccn.h classes
#include <rccn.h>	// IMPF IFFNM
#include "pp.h"
#include "cutok.h"
#include "irats.h"
#include "cueval.h"
#include <msghans.h>	// MH_1901

#include <rmkerr.h>	// warn

#include <cnguts.h>	// ImpfB IffnmB

#include <impf.h>	// declarations for fcns in this file

/*
"Imports" covers:

    IMPORTFILE objects
       Specification of file from which to import data;
       causes file to be opened and records read at specified frequency.

    IMPORT and IMPORTSTR expression functions
       These retrieve values from current record of a specified import file.

Imports record types:

    IMPF records in ImpfiB and ImpfB

       Record for each import file, created when IMPORTFILE object seen.
       Contents include file buffer, record information, and fields-by-number table (.fnrt[])

    IFFNM import file field name records in IffnmB

       Record for each import file, created when first IMPORT() function seen.
       Contents include ImpfiB subscript and fields-by-name-index table (.fnmt[]).

       1) IFFNM records connect import function pseudo-code to IMPF records
          at run time, allowing forward references in input.

       2) IFFNM records remember field names in each file used in Import
          functions til file opened and field position in file determined
          from file header.

Definitions:

   Field number (fnr): 1-based field position of field in record; is .fnrt[] subscript.

   Field name index (fnmi): 1-based number assigned to field names for import file
      in order names first seen in Import functions; is fnmt[] subscript.
      Field names are translated to field numbers after file is opened at run start.

IFFNM strange status:

   The Import File Field NaMe table records in IffnmB communicate from the
   expression compiler mainly to runtime.

   They are not created via cul.c/CULT stuff, but out of cuparse.cpp

   They cannot be deleted after setup like most input records;
   they must be preserved thru run and successive runs till CLEAR.

TODO 11-2016 (when documentation written)
* Add an interval choice that supports only YEAR MONTH DAY HOUR
  (or add subhour support to import capabilities
* Use common tokenizing code?
* Improve messages
*/

///////////////////////////////////////////////////////////////////////////////
// local classes
///////////////////////////////////////////////////////////////////////////////
struct FNRT  	// fields-by-number table struct for IMPF.fnrt[] in heap
{  CHP fieldName;		// NULL or field name (in heap) per file header, for error messages
   SI fnmi;			// 0 or field name info subscript for IFFNM.p[iffnmi].fnm[]
   CHP fp;				// NULL or pointer to field's null-terminated text in current record in .buf (do not free here)
   BOO nDecoded;		// TRUE if numeric value of field in current record has been decoded
   FLOAT fnv;			// if nDecoded, this is the numeric value -- don't decode twice
};
//=============================================================================
struct FNMT		// field names table struct for IFFNM.fnmt[] in heap
{  CHP fieldName;		// field name used in Import() (no entry for fields not used by name in Import()s)
   SI fnr;			// field NUMBER established when file opened
};
//=============================================================================
class ImpFldDcdr
{
	// the import file
	IFFNM* iffnm;	// pointer to Import File Names Table record in IffnmB
	IMPF* impf;		// pointer to IMPORTFILE record in ImpfB
// the field
	int fnr;		// field number
	FNRT* fnrt;		// pointer to field number info: &impf->fnrt[fnr].  .fp .fnv .nDecoded
// re errors
	RC rc1;			// RCOK if axFile successfully completed
	RC rc2;			// RCOK if axscanFnm or -Fnr successfully completed
	char *impfName;	// "" or import file name or object name text to use in error messages
	int fileIx;		// CSE input file name index for use in error messages
	const char* srcFile;	// text for fileIx: "" or CSE source file in which import() occurred
	int line;		// line # in cse source file
	char *fieldName;	// field name text for error messages, when known (no names if no file header)
	const char** pms;	// c'tor arg: where to return TmpStr error msg pointer, so caller can embed in msg giving context.
public:
	ImpFldDcdr( int fileIx, int line, const char** pms);	// c'tor. *pms receives TmpStr error submessage pointer.
	// usage: call axFile, then axscanFnr or -Fnm, then decNum if numeric
	RC FC axFile(int iffnmi);	// access import file, set .iffnm and .impf
	RC FC axscanFnm(int fnmi);	// access and scan field by name: for field name idx, set .fnr, .fnrt, .fieldName
	RC FC axscanFnr(int _fnr);	// access and scan field by number: for field number, set .fnr, .fnrt, .fieldname
	RC FC decNum();		// decode field's numeric value. uses .fnrt, sets fnrt->fnv, ->nDecoded.
	char * sVal()
	{	return strsave( fnrt && fnrt->fp ? fnrt->fp : "");    // string value (private heap copy), "" if error
	}
	float nVal()
	{	return fnrt ? fnrt->fnv : 0.f;    // numeric value, 0 if error
	}
};			// class ImpFldDcdr
//===========================================================================
ImpFldDcdr::ImpFldDcdr( 		// constructor: initializes
	int fileIx,			// cse input file name index for file in which import() occurred, for use in error messages
	int line,			// line number in ditto
	const char** pms )	// receives message pointer on error, to let caller insert in message giving context.
{
	iffnm = NULL;
	impf = NULL;
	fnr = 0;
	fnrt = NULL;
	rc1 = rc2 = RCBAD;
	impfName = fieldName = "";
	this->fileIx = fileIx;
	this->srcFile = getFileName(fileIx);		// access text now (fast) for use in error messages. does not dmIncRef.
	this->line = line;
	this->pms = pms;			// IMPERR macro returns message subtext thru this member pointer
}		// ImpFldDcdr::ImpFldDcdr
//---------------------------------------------------------------------------
#define IMPERR(msg) ((*pms = strtprintf msg), RCBAD)	// runtime error macro, eg  return IMPERR(("Bad File %s",name));
// (())'s mandatory for vbl arg list in call.
//---------------------------------------------------------------------------
RC FC ImpFldDcdr::axFile( int iffnmi)		// access import file, set .iffnmi, .iffnm, .impfi and .impf

// errors store message pointer via .pms (set by c'tor), leave rc1 RCBAD, and return RCBAD.
{
	if (iffnmi <= 0 || iffnmi > IffnmB.n)
		return IMPERR(( (char *)MH_R1905, 	/* "%s(%d): Internal error:\n"
											   "    Import file names table record subscript %d out of range 1 to %d." */
				srcFile, line, iffnmi, IffnmB.n ));
	iffnm = IffnmB.p + iffnmi;				// set member pointer to names table record
	TI impfi = iffnm->impfi;				// get IMPORTFILE record subscript from names table record
	if (impfi <= 0 || impfi > ImpfB.n)
		return IMPERR(( (char *)MH_R1906, 		/* "%s(%d): Internal error:\n"
												   "    Import file subscript %d out of range 1 to %d." */
				srcFile, line, impfi, ImpfB.n ));
	impf = &ImpfB.p[impfi];				// point Import File record
	impfName = impf->fileName && *impf->fileName	// name for error messages: dos pathName if present,
			   ? impf->fileName : impf->name;		//  else object name, which is "" if not given
	if (!impf->isOpen)					// unless file is open and buffer allocated ok
		return IMPERR(( (char *)MH_R1907, 		/* "%s(%d): Internal error:\n"
												   "    Import file %s was not opened successfully." */
				srcFile, line, impfName ));	// unexpected: run should have been stopped
	if (impf->eof)
		return IMPERR(( (char *)MH_R1908, 		// "%s(%d): End of import file %s: data previously used up.%s"
						srcFile, line, impfName,
						Top.isWarmup
						?  msg( NULL, (char *)MH_R1909)	/* "\n    File must contain enough data for CSE warmup days (default 7)."*/
						:  ""));

	rc1 = RCOK;						// say axFile completed ok
	return RCOK;
}			// ImpFldDcdr::axFile
//---------------------------------------------------------------------------
RC FC ImpFldDcdr::axscanFnm(int fnmi)		// access field by name: set fieldName, fnr, fnrt for field name index

// preceding axFile required. scan tentatively included.
// ERRORs store message pointer via .pms (set by c'tor), leave rc2 RCBAD, and return RCBAD.
{
	if (rc1)						// if file not accessed ok, don't attempt to access field
		return IMPERR(( (char *)MH_R1910 ));		/* "Internal error: ImpfldDcdr::axscanFnm():\n"
							   "    called w/o preceding successful file access" */
	if (!iffnm->fnmt)					// prevent GP fault
		return IMPERR(( (char *)MH_R1911, 		/* "%s(%d): Internal error:\n"
												   "    no IFFNM.fnmt pointer for Import file %s" */
						srcFile, line, impf->name ));

// fetch and check field number for given field name index

	if (fnmi < 1 || fnmi > iffnm->fnmiN)			// should be 1 to max value seen during compile
		return IMPERR(( (char *)MH_R1912, 			/* "%s(%d): Internal error: in IMPORT() from file %s\n"
												       "    field name index %d out of range 1 to %d." */
						srcFile, line, impfName, fnmi, iffnm->fnmiN ));
	if (iffnm->fnmt[fnmi].fieldName)			// insurance: for NULL leave "" stored by c'tor
		fieldName = iffnm->fnmt[fnmi].fieldName;		// field name text, for error messages, eg in decNum
	int tfnr =   iffnm->fnmt[fnmi].fnr;			// 1-based field number for this field name index, to local til validated.
	if (tfnr <= 0 || tfnr > FNRMAX)			// FNRMAX: impf.h
		return IMPERR(( (char *)MH_R1913,	/* "%s(%d): Internal error:\n"
											   "    in IMPORT() from file %s field %s (name index %d),\n"
											   "    field number %d out of range 1 to %d." */
						srcFile, line,
						impfName, iffnm->fnmt[fnmi].fieldName, fnmi,
						tfnr, FNRMAX ));

// scan record thru requested field if not already done
	while (impf->nFieldsScanned < tfnr)			// until enough fields scanned in current record, if not already done
		if (!impf->scanNextField())			// scan field: delimit, dequote, do \ codes, null-terminate, etc in place
			return IMPERR(( (char *)MH_R1914,		/* "%s(%d): Too few fields in line %d of import file %s:\n"
													 "      looking for field %s (field # %d), found only %d fields." */
							srcFile, line, impf->lineNo, impfName,
							fieldName, tfnr, impf->nFieldsScanned ));

	fnr = tfnr;   			// store field #. scanNextField has alloc'd .fnrt[] this big. used eg in decNum.
	fnrt = impf->fnrt + tfnr;		// store pointer to field names info entry

	rc2 = RCOK;				// say field accessed and scanned successfully
	return RCOK;
}				// ImpFldDcdr::axscanFnm
//---------------------------------------------------------------------------
RC FC ImpFldDcdr::axscanFnr(int _fnr)	// access and scan field by number, set .fieldName, .fnr, .fnrt

// preceding axFile required. scan tentatively included.
// ERRORs store message pointer via .pms (set by c'tor), leave rc2 RCBAD, and return RCBAD.
{
	if (rc1)						// if file not accessed ok, don't attempt to access field
		return IMPERR(( (char *)MH_R1915));		/* "Internal error: ImpfldDcdr::axscanFnr():\n"
												   "    called w/o preceding successful file access" */
	if (_fnr <= 0 || _fnr > FNRMAX)
		return IMPERR(( (char *)MH_R1916, 		/* "%s(%d): Internal error: in IMPORT() from file %s, \n"
							   "    field number %d out of range 1 to %d." */
						srcFile, line, impfName, _fnr, FNRMAX ));

// scan record thru requested field if not already done

	while (impf->nFieldsScanned < _fnr)			// until enough fields scanned in current record
		if (!impf->scanNextField())			// scan field: delimit, dequote, do \ codes, null-terminate, etc in place
			return IMPERR(( (char *)MH_R1917,			/* "%s(%d): Too few fields in line %d of import file %s:\n"
								   "      looking for field %d, found only %d fields." */
							srcFile, line, impf->lineNo, impfName,
							_fnr, impf->nFieldsScanned ));

	this->fnr = _fnr;			// store field #. scanNextField has alloc'd .fnrt[] this big. used eg in decNum.
	fnrt = impf->fnrt + _fnr;		// store pointer to field names info entry
	if (fnrt->fieldName)		// for NULL (eg no header), leave "" stored by c'tor
		fieldName = fnrt->fieldName;	// retrieve field name for number -- set at open if file had header

	rc2 = RCOK;				// say field accessed and scanned successfully
	return RCOK;
}			// ImpFldDcdr::axscanFnr
//---------------------------------------------------------------------------
RC FC ImpFldDcdr::decNum()		// decode field's numeric value, set fnrt->fnv

// preceding axscanFnm or axscanFnr required.
// ERRORs store message pointer via .pms (set by c'tor) and return RCBAD.
{
	if ( rc2						// if no success code from axscanFnm or Fnr
			||  !fnrt 						// GP fault insurance: no field table entry pointer in our object
			||  !fnrt->fp )					//    .. or no scanned field ptr in the table entry
		return IMPERR(( (char *)MH_R1918));		/* "Internal error: ImpfldDcdr::decNum():\n"
							   "    called w/o preceding successful field scan" */
	if (fnrt->nDecoded)  return RCOK;			// if this field of this record already decoded, don't repeat
#if 0	// moved below so DOES DECODE each use & repeat error msg & bad return, as opposed to using 0. 2-94.
x		fnrt->nDecoded = TRUE;	// say don't need to decode this field again
x    							// (if error, later calls get 0 without repeating message)
#endif

#define SUBERR(text) { sub = text; goto subErr; }
	char *sub /*="bug"*/;				// (redundant init removed 12-94 when BCC32 4.5 warned)

// decode number
	const char* start = fnrt->fp;				// point to scanned field text
	start += strspn( start, " \t");			// pass spaces and tabs for chars consumed test
	if (strcspn(start,"0123456789") >= strlen(start))	// if no digits then syntax is bad
		SUBERR((char *)MH_R1919)				// go to end fcn and issue err msg "non-numeric value"
		//else
	{
		// { }: 'v' scope, 12-94
		char *endptr;
		double v = strtod( start, &endptr);  		// convert string to double, return end pointer. C library fcn.
		// returns +- HUGE_VAL on overflow.
		if (endptr <= start)				// if strtod used no characters then syntax is bad
			SUBERR((char *)MH_R1919)			// go issue err msg "non-numeric value"
			endptr += strspn( endptr, " \t");   		// pass spaces and tabs after value to check for end field
		if (*endptr)					// if does not point to \0 that impFld put at end field
			SUBERR((char *)MH_R1920);   			// go issue error msg "garbage after numeric value"
		if (v < -FLT_MAX || v > FLT_MAX)			// if overflow or too big to convert to float
			SUBERR((char *)MH_R1921);			// "number out of range"

		// have valid numeric value
		fnrt->fnv = float(v);				// convert to float and store value for caller & later references
		fnrt->nDecoded = TRUE;				// say don't need to decode this field again
		return RCOK;
	}

subErr:	// error using subtext "sub", eg "non-numeric value"
	const char* fldSub = fieldName && *fieldName ? strtprintf( "%s (#%d)", fieldName, fnr)	// show field name if set
				   : strtprintf( "%d", fnr);
						/* MH_R1922: "%s(%d): \n"
						 "    Import file %s, line %d, field %s:\n"
						 "      %s:\n"					// SUBERR macro arg text
						 "          \"%s\""				// field text */
	return IMPERR(( (char *)MH_R1922,				/* text just above */
					srcFile, line,
					impfName, impf->lineNo, fldSub,
					msg( NULL, sub), 					/* gets text for msg handle, 6-95 */
					start ));
}				// ImpFldDcdr::decNum
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// IMPORT() function compile support
// called from cuparse.cpp
///////////////////////////////////////////////////////////////////////////////

/* Import function story, 2-94:

Import() fcn is unique in CSE as of 2-94 in that compiling this expression
element requires compile-time setting and use of info in a record array table:
IFFNM is used to remember Import File names (permitting forward references)
and field names (resolved to numbers at file open).

(Usually, only final expression value goes in record array table
member; other accesses in expression (probes) are done at run time.)

Import() function compiling: cuparse.cpp does syntax, calling code here to
handle IFFNM records, cuparse.cpp emits pseudo-code.
*/

LOCAL RC impFcnFile( char *impfName, TI *pIffnmi, USI fileIx, int line, IVLCH *imFreq, IFFNM **ppIffnm);

//--------------------------------------------------------------------------
// Following 2 fcns make IFFNM record if new name, return its subscript.
// Name version enters field name and returns field name index.
//--------------------------------------------------------------------------
RC impFcn( 		// compile support for Import() of field by field number

	char *impfName, 		// IMPORTFILE object name (1st arg of IMPORT())
	TI *pIffnmi, 		// receives subscript of IFFNM record (added here if new) for use in pseudo-code
	int fileIx,			// file name index of CSE input file being compiled: put in IFFNM record when created ...
	int line,			// line number in srcFile ...  so errors can show location of first use.
	IVLCH *imFreq,		// receives frequency (hour-day-month-year) of import file, or safe assumption if fwd reference.
	SI /*fnr*/ )     		// requested field number

// called from cuparse.cpp. May be declared in impf.h not cncult.h.
{
// find or add IFFNM record for this IMPORTFILE object name. (IMPORTFILE record created only when IMPORTFILE seen.)
	IFFNM *iffnm;
	if (impFcnFile( impfName, pIffnmi, fileIx, line, imFreq, &iffnm) != RCOK)	// find or add IFFNM record
		return RCBAD;								// if failure retured (ABT expected)

// record max field number seen
//    * if member retained -- used?
//    if (fnr > iffnm->maxFnrSeen)
//       iffnm->maxFnrSeen = fnr;

	return RCOK;
}				// impFcn (numbered field)
//---------------------------------------------------------------------------------------------------------------------------
RC impFcn( 		// compile support for Import() of named field

	char *impfName, 		// IMPORTFILE object name (1st arg of IMPORT())
	TI *pIffnmi, 		// receives subscript of IFFNM record (added here if new) for use in pseudo-code
	int fileIx,			// file name index of input file being compiled: put in IFFNM record when created ...
	int line,			// line number in srcFile ...  so errors can show location of first use.
	IVLCH *imFreq,		// receives frequency (hour-day-month-year) of import file, or safe assumption if fwd reference.
	char *fieldName,   	// requested field name: saved here in table in IFFNM record for resolution at file open
	SI *fnmi ) 			// receives find name index for use in pseudo-code

// called from cuparse.cpp. May be declared in impf.h not cncult.h.
{
// find or add IFFNM record for this IMPORTFILE object name. (IMPORTFILE record created only when IMPORTFILE seen.)

	IFFNM *iffnm;
	if (impFcnFile( impfName, pIffnmi, fileIx, line, imFreq, &iffnm) != RCOK)	// find or add IFFNM record
		return RCBAD;								// if failure retured (ABT expected)

// find or add entry in field names table

	// Names in table will be resolved to field number at open; runtime accesses get fnr from table by fnmi.
	SI tfnmi = 0;
	BOO found = FALSE;
	if (iffnm->fnmt)		// insurance
		for (tfnmi = 1;  tfnmi <= iffnm->fnmiN;  tfnmi++)		// 1-based subscript
			if (!strcmpi( fieldName, iffnm->fnmt[tfnmi].fieldName))
			{
				found = TRUE;
				break;
			}
	if (!found)			// if must add
	{
		// assign table position === field name index
		tfnmi = ++iffnm->fnmiN;		// assign next 1-based field name index

		// allocate or enlarge table if necessary
		if ( tfnmi >= iffnm->fnmtNAl  	// if table needs to be bigger (>= like +1 for 1-based field numbers)
				||  !iffnm->fnmt )		// or table not allocated yet (insurance)
		{
#define CHUNK 32			// number of field names to allocate at a time
			SI toAl = iffnm->fnmtNAl + CHUNK;				// # slots to allocate
			if (dmral( DMPP( iffnm->fnmt), toAl * sizeof(FNMT), ABT|DMZERO))	// (re)alloc, dmpak.cpp. abort (no return) if fails.
				return RCBAD;						// if failure returned (not expected)
			iffnm->fnmtNAl = toAl;    		// ok, store new size
		}

		// fill new IFFNM.fnmt[] entry
		iffnm->fnmt[tfnmi].fieldName = strsave(fieldName);
		iffnm->fnmt[tfnmi].fnr = 0;			// believed redundant
	}
	*fnmi = tfnmi;			// return field name index from local variable
	return RCOK;
}			// impFcn (named field)
//---------------------------------------------------------------------------------------------------------------------------
LOCAL RC impFcnFile( 			// find or add IFFNM record

	char *impfName, 		// import file object name (1st arg in Import() fcn)
	TI *pIffnmi,  		// receives IffnmB subscript of IFFNM record
	USI fileIx,			// file name index of CSE input file being compiled: put in IFFNM record when created ...
	int line,			// line number in srcFile ...  so errors can show location of (first) use.
	IVLCH *imFreq,		// receives frequency (hour-day-month-year) of import file, or safe assumption if fwd reference.
	IFFNM **ppIffnm )		// receives pointer to record

// this is common part of named and numbered field impFcn() fcns.
{
// IFFNM record is used for Imports from numbered as well as named fields, for getting to IMPF at runtime so fwd ref ok.

// find else add Import file field names record for this IMPORTFILE name.

	IFFNM *iffnm;
	if (IffnmB.findRecByNmU( impfName, pIffnmi, (record **)&iffnm) != RCOK)	// find record / if not found (ancrec.cpp)
	{
		// returns RCBAD not found or RCBAD2 not unique, but latter is not expected.

		// not found, add.
		if (IffnmB.add( &iffnm, ABT) != RCOK)			// add record, abort program if out of memory
			return RCBAD;						// if failure returned (unexpected)
		*pIffnmi = iffnm->ss;					// return added record subscript

		// use IMPORTFILE given object name as record name, for later association with IMPORTFILEs.
		iffnm->SetName( impfName);

		// put source file index and line in record so (first) use can be reported in errmsg eg if no IMPF for IFFNM.
		iffnm->fileIx = fileIx;
		iffnm->line = line;
	}
	*ppIffnm = iffnm;						// return pointer to found or added record

// get frequency or return safe assumption

	IMPF* iimpf;
	if ( ImpfiB.findRecByNmU( iffnm->name, NULL, (record**)&iimpf )==RCOK	// look for IMPF record / if found
			&& iimpf->sstat[IMPF_IMFREQ] & FsVAL )					// and frequency has been specified
		*imFreq = iimpf->imFreq;						// return user-specified frequency
	else
		*imFreq = C_IVLCH_H;					// else assume the worst case: hourly

	return RCOK;
}			// impFcnFile
//---------------------------------------------------------------------------------------------------------------------------



RC topImpf()		// check/process ImportFiles at end of input

// called from cncult2.cpp:topCkf.
{
	RC rc /*=RCOK*/;			// (redundant init removed 12-94 when BCC 32 4.5 warned)

	/* IffnmB story. An IFFNM record is created when in the first Import() function
	   is seen by cuparse.cpp expression compiler code for a given Import File.
	   IFFNM purposes:

	    1) Handle forward references to IMPF's (associated here by name,
	       indirect access thru IFFNM to IMPF used during run), and

	    2) Handle field references by name (associated when file is opened at
	       start of run, indirect reference thru IFFNM.fnmt[] to IMPF.fnrt[] used
	       during run) */

// check Import File Field Name Tables and assocate with ImportFiles

	IFFNM* iffnm;
	IMPF* iimpf;
	RLUP (IffnmB, iffnm)	// loop over IFFNM records (ancrec.h macro). One anc used for expr compile support and run.
	{
		// find ImportFile else error
		if (ImpfiB.findRecByNmU( iffnm->name, &iffnm->impfi, (record**)&iimpf )!= RCOK)	// look for IMPF record, lib\ancrec.cpp
		{
			// return is RCBAD not found, RCBAD2 ambiguous, but latter not expected.
			// note: don't use oer cuz it would show IFFNM object type name "ImpFileFldNames".
			cuEr( 0, 0, 0, 1, iffnm->fileIx, iffnm->line, 0, 	// cutok.cpp
				  (char *)MH_S0574, 				// "No IMPORTFILE \"\s\" found for IMPORT(%s,...)"
				  iffnm->name, iffnm->name );
			continue;					// error message prevents run.
			// iffnm->impfi remains 0 to indicate no ImportFile for Import()(s) that created this IFFNM.
		}

		// associate importfile with iffnm. Other-way association, iffnm->impfi, was set by findRecByNmU call just above.
		iimpf->iffnmi = iffnm->ss;			// store IffnmB record subscript in ImpfiB record member
		// is iffnmi ever used?
	}

// check IMPORTFILES and make run records

	CSE_E( ImpfB.al( ImpfiB.n, WRN) )		// delete old importfile run records, alloc needed # now for min fragmentation.
	// E: return if error.  CAUTION: ratCre may not errCount++ on error.
	RLUP (ImpfiB, iimpf)		// loop over good IMPF input records (ancrec.h macro)
	{
		// be sure all required members are set. Believed redundant; extra protection against FPE bomb-out on NAN (4-byte mbrs).
		CSE_E( iimpf->CkSet( IMPF_FILENAME))			// if not set, no run, terminate checking now
		CSE_E( iimpf->CkSet( IMPF_IMFREQ))			// if not set, no run, terminate checking now

		// find file & save full path. Search same paths as for #include files. 2-95.
		ppFindFile( iimpf->fileName);
		// if not found, no message here... let old code issue message at runtime attempt to open.

		// warn if unused (no Imports)
		if (!iimpf->iffnmi)				// if no IFFNM found above for this IMPF, then it has no IMPORTs.
		{
			iimpf->oWarn( (char *)MH_S0575);		// "Unused IMPORTFILE: there are no IMPORT()s from it."
			continue;					// need no run record if unused
		}

		// access IFFNM, error if named imports but no file header. No IFFNM is ok -- normal if no Imports by field name.
		else
		{
			iffnm = IffnmB.p + iimpf->iffnmi;
			if (iimpf->hasHeader != C_NOYESCH_YES)	// if user said no header on file
				if (iffnm->fnmiN)				// if have name table entries --> IMPORT()s by name seen
				{
					iimpf->oer( (char *)MH_S0576);		// "imHeader=NO but IMPORT()s by field name (not number) used"
					continue;
				}
		}

		// ok, copy data to run record for this IMPORTFILE
		IMPF *impf;
		ImpfB.add( &impf, ABT, iimpf->ss);			// add IMPF run record, ret ptr to it.  err unexpected cuz al'd.
		*impf = *iimpf;						// copy entire record including name; incRefs any heap pointers.

		// runtime init is done in impf.cpp: allocate buffer, open file, get field names from header, etc.
	}
	return rc;
}		// topImpf
//---------------------------------------------------------------------------------------------------------------------------
RC clearImpf()	// Import stuff clear function to call at CLEAR and before initial data entry

// called from cncult2:topStarPrf, cncult2:cncultClean.
{
// clear the Import File Field NaMes table records.

	/* These differ from most CSE records in that they are used both at compile
	   time (from cuparse.cpp, not via CULT tables) and at run time,
	   and thus cannot be cleared before last run (as input records normally are),
	   nor after run if there is no clear (as run records normally are). 2-94 */

	IffnmB.free();	// destroy and free all IFFNM records

	return RCOK;
}			// clearImpf
////////////////////////////////////////////////////////////////////////////////////

//===========================================================================
//  Import File Reading Functions
//===========================================================================
RC FC impfStart()		// import files stuff done at start run

// returns non-RCOK with message issued on serious error that should prevent run.
{
	RC rc = RCOK;
	IMPF * impf;
	RLUP( ImpfB, impf)			// loop good import file run records
	{	if (!impf->iffnmi)			// if file not used (no imports seen)
			continue;				// don't open unused files. leave .isOpen 0 to prevents close.

		// allocate buffer
#define BUFSZ (1024+2)			// buffer size = absolute max record size, +2 for \r\n
		if ( dmal( DMPP( impf->buf),	// dmpak.cpp. returns RCOK (which is 0) if ok.
				   BUFSZ+1, 			// 1 extra byte for \0 always stored after data.
				   DMZERO				// zero buffer - debug aid
				   | WRN ) )			// warn & continue if can't get memory
		{	rc = RCBAD;				// if failed, say return bad. caller should prevent run else many Import() errors.
			continue;				// don't open file if no buffer
		}   						// CODE ASSUMES BUFFER OK IF .isOpen NON-0.
		impf->bufSz = BUFSZ;		// ok, store buffer size for readBuf.

		// open file. Note paths searched & full pathname saved in cncult4.cpp, 2-95.

		impf->fh = fopen( impf->fileName, "rb");
		if (impf->fh < 0)
		{	rc = err( WRN, 		 	// general error msg, errCount++. WRN: await keypress. rmkerr.cpp.
					  (char *)MH_R1901, 		// "Cannot open import file %s. No run."
					  impf->fileName );
			continue;
		}
		impf->isOpen = TRUE;		// say open & buffer allocated: enable read and close

		// scan header
		if (impf->scanHdr())		// scan file header, nop if none, get field names, non-RCOK (non-0) if error.
		{	impf->close();  		// after bad header, close now to avoid impfEnd's "too few lines" warning
			rc = RCBAD;				// return bad. expected to stop run.
			continue;
		}

		// save file postition after header to seek back to after warmup.
		LI temPos = ftell(impf->fh);		// get file position
		if (temPos < 0L)				// -1L if error
		{	rc = err( PWRN, 			// general error message (rmkerr.cpp), count error (errCount++), return RCBAD.
					  (char *)MH_R1902, 		// "Tell error (%ld) on import file %s. No run."
					  impf->fileName );
			continue;
		}
		USI unused = impf->bufN - impf->bufI2;		// number of unscanned bytes remaining in buffer
		impf->posEndHdr = temPos - (LI)(ULI)unused;	// store position at start of data
		impf->lineNoEndHdr = impf->lineNo;			// store corresponding line number for error messages
	}
	return rc;
}		// impfStart
//---------------------------------------------------------------------------
RC FC impfAfterWarmup()		// import files stuff done after CSE warmup and after each repetition of each autosizing day

// resets import files to start so they do not have to contain extra data at beginning for warmup.
// Each file must however be long enough for warmup even if run is shorter. Default warmup is 7 days, 2-94.
{
	RC rc = RCOK;
	IMPF* impf;
	RLUP( ImpfB, impf)				// loop good import file run records
	{
		if (!impf->isOpen)			// if file not open (eg not used -- no imports seen)
			continue;						// don't rewind
		int newPos = fseek( impf->fh, impf->posEndHdr, SEEK_SET);	// seek to end header / return new position or -1 if error
		if (newPos != 0)				// if got wrong position or error
			rc |= err( WRN,					// <--- apparently missing WRN added, rob 5-97
					   (char *)MH_R1903, 				// "Seek error (%ld) on import file %s, handle %d"
					   newPos, impf->fileName, impf->fh );

		impf->bufI1 = impf->bufI2 = impf->bufN = 0;	// set buffer empty -- force reread
		impf->eofRead = impf->eof = FALSE;		// ..
		impf->buf[0] = '\0';				// ..
		impf->lineNo = impf->lineNoEndHdr;		// restore file line number used in error messages
	}
	return rc;
}		// impfAfterWarmup
//---------------------------------------------------------------------------
void FC impfEnd()			// import files stuff done at end run

// redundant calls ok.
{
	IMPF* impf;
	RLUP( ImpfB, impf)				// loop import files
	{	if (impf->isOpen) 			// if this file is open
		{	// warn if not all of file used unless warmup required longer file (short run)
			int nlnfwud;				// number of lines needed for warmup days
			int wud = max( Top.wuDays, 7);		// # warmup days, not less than default of 7 (2-94)
			switch (impf->imFreq)
			{
			case C_IVLCH_H:
				nlnfwud = 24 * wud;
				break;
			case C_IVLCH_D:
				nlnfwud = wud;
				break;
			case C_IVLCH_M:
				nlnfwud = 2;
				break;	// 1 usually, 2 if crosses month end
			default:
				nlnfwud = 1;
				break;	// incl C_IVLCH_Y
			}
			if (impf->lineNo >= nlnfwud)		// if more data used than warmup could require
				if (impf->readRec())			// read next record from file / TRUE if successful
					warn( (char *)MH_R1904,		// "Import File %s has too many lines. \n"
						  impf->fileName, 		// "    Text at at/after line %d not used." */
						  impf->lineNo );		// use warn to display message without incrementing error counter

			// close file and free buffer
			impf->close();		// mbr fcn also called in destructor. clears .isOpen.
		}
		dmfree( DMPP( impf->buf));	// free buffer and NULL .buf. if redundant here, harmless.
	}
}		// impfEnd
//---------------------------------------------------------------------------
void FC impfIvl( 		// import files stuff done at start interval: get new record
	IVLCH ivl )			// interval now starting: C_IVLCH_Y, _M, _D, or _H
// D implies H, M implies H and D, etc.
{
	IMPF* impf;
	RLUP( ImpfB, impf)			// loop import files
	if ( impf->imFreq >= ivl		// if this file has frequency we are doing, or hier
			&&  impf->isOpen )		// if this file is open and buffer allocated
	{
		impf->readRec();		// get first / next record in buffer, init to scan fields.
		// no eof error here, only when a field actually accessed.
	}
}		// impfIvl
//---------------------------------------------------------------------------

//===========================================================================
//  functions for decoding import fields, called from cueval.cpp
//===========================================================================
RC impFldNmN( 	// import numeric value of named field

	int iffnmi,		// which IMPORTFILE: 1-based IffnmB subscript, from pseudo-code
	int fnmi, 		// field name index: 1-based IffnmB.fnmt[] subscript
	float *pv, 		// receives float value
	int fileIx,		// CSE input file name index for use in error messages
	int line,		// CSE input source file line number for use in error messages
	const char** pms )  	// receives transitory submessage string pointer (Tmpstr) if error occurs
{
	ImpFldDcdr fd( fileIx, line, pms);	// object for decoding field. local class. c'tor inits.
	RC rc = fd.axFile(iffnmi); 		// check/access file
	if (!rc)  rc = fd.axscanFnm(fnmi);	// if ok, check/access/scan field by name index (axFile rc internally passed)
	if (!rc)  rc = fd.decNum();		// if ok, decode field numeric value (axscanFnr rc internally passed)
	if (!rc)  *pv = fd.nVal();		// if ok, return field numeric value
	return rc;
}			// impFldNmN
//---------------------------------------------------------------------------
RC impFldNmS(	  	// import string value of named field

	int iffnmi,		// which IMPORTFILE: 1-based IffnmB subscript, from pseudo-code
	int fnmi, 		// field name index: 1-based IffnmB.fnmt[] subscript
	char **pv, 		// receives pointer to string value in heap
	int fileIx,		// CSE input file name index for use in error messages
	int line,		// CSE input source file line number for use in error messages
	const char** pms ) 	// receives transitory submessage string pointer (Tmpstr) if error occurs
{
	ImpFldDcdr fd( fileIx, line, pms);	// object for decoding field. local class. c'tor inits.
	RC rc = fd.axFile(iffnmi);		// check/access file
	if (!rc)  rc = fd.axscanFnm(fnmi);	// if ok, check/access/scan field by name index (axFile rc internally passed)
	if (!rc)  *pv = fd.sVal();		// if ok, return heap copy of field string value
	return rc;
}			// impFldNmS
//---------------------------------------------------------------------------
RC impFldNrN( 		// import numeric value of field by number
	int iffnmi,		// which IMPORTFILE: 1-based IffnmB subscript, from pseudo-code
	int fnr, 		// 1-based field number
	float *pv, 		// receives float value
	int fileIx,		// CSE input file name index for use in error messages
	int line,		// CSE input source file line number for use in error messages
	const char** pms )  	// receives transitory submessage string pointer (Tmpstr) if error occurs
{
	ImpFldDcdr fd( fileIx, line, pms);	// object for decoding field. local class. c'tor inits.
	RC rc = fd.axFile(iffnmi);		// check/access file
	if (!rc)  rc = fd.axscanFnr(fnr);	// if ok, check/access/scan field by number  (axFile rc internally passed)
	if (!rc)  rc = fd.decNum();		// if ok, decode field numeric value (axscanFnr rc internally passed)
	if (!rc)  *pv = fd.nVal();		// if ok, return field numeric value
	return rc;
}			// impFldNrN
//---------------------------------------------------------------------------
RC impFldNrS( 		// import string value of field by number

	int iffnmi,		// which IMPORTFILE: 1-based IffnmB subscript, from pseudo-code
	int fnr, 		// 1-based field number
	char **pv, 		// receives ptr to string value in heap
	int fileIx,		// CSE input file name index for use in error messages
	int line,		// CSE input source file line number for use in error messages
	const char **pms ) 	// receives transitory submessage string pointer (Tmpstr) if error occurs
{
	ImpFldDcdr fd( fileIx, line, pms);	// object for decoding field. local class. c'tor inits.
	RC rc = fd.axFile(iffnmi);		// check/access file
	if (!rc)  rc = fd.axscanFnr(fnr);	// if ok, check/access/scan field by number (axFile's rc internally communicated)
	if (!rc)  *pv = fd.sVal();		// if ok, return heap copy of field string value
	return rc;
}			// impFldNrS
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// class IFFNM: IMPORTFILE field names table
///////////////////////////////////////////////////////////////////////////////
IFFNM::~IFFNM()		// record destructor
{
// free (or decrement reference count of) derived class heap pointers in record being destroyed. dmfree:lib\dmpak.cpp.

	// input strings -- none
	//cupfree( DMPP( ..));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.

	// fields names table
	if (fnmt)					// if table allocated
		for (int fnmi = 1; fnmi < fnmtNAl; fnmi++)	// loop over table entries. Entry 0 unused.
			dmfree( DMPP( fnmt[fnmi].fieldName));		// decref/free string member

	dmfree( DMPP( fnmt));			// free the table block if allocated, and NULL pointer. dmpak.cpp.
	fnmtNAl = 0;				// say none allocated

	//record::~record() (call supplied by compiler) zeroes .gud to mark space unused.
}			// IFFNM::~IFFNM
//---------------------------------------------------------------------------------------------------------------------------
/*virtual*/ void IFFNM::Copy( const record* pSrc, int options/*=0*/)	// overrides record::Copy. declaration must be same.
// IFFNM = believed unused 2-94, but should be defined for link as is virtual fcn in base class.
{
	err( PWRN, (char *)MH_S0578);	// "Unexpected call to IFFNM::Copy". if occurs, add code to do .fnmt or verify NULL.

// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten. dmfree: lib\dmpak.cpp.
	//#define THIS ((IFFNM *)this)
	//cupfree( DMPP( THIS->..));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
	// .fnmt not handled -- must dmfree .fieldNames before dmfree'ing table.

// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED.
	record::Copy( pSrc, options);	// verfies that src and this are same record type. lib\ancrec.cpp.

// increment reference count for pointer(s) just copied. dmIncRec: lib\dmpak.cpp.
	//cupIncRef( DMPP( THIS->..));   		// dmIncRef unless NANDLE or constant inline in pseudocode, cueval.cpp.
	// .fnmt not handled -- if non-NULL must copy block and incref .fieldNames.
}				// IFFNM::Copy
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//  class IMPF
///////////////////////////////////////////////////////////////////////////////
IMPF::~IMPF()		// IMPORTFILE destructor
{
// close file if open -- might be possible during error cleanup
	close();				// member function in impf.cpp.

// free (or decrement reference count of) derived class heap pointers in record being destroyed. dmfree:lib\dmpak.cpp.

	// input strings
	cupfree( DMPP( fileName));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
	cupfree( DMPP( imTitle));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.

	// buffer
	dmfree( DMPP( buf));		// (decr ref count or) free heap block & NULL ptr, subset of cupfree, dmpak.cpp.
	bufSz = 0;				// say no buffer: insurance: beleived unnecessary but harmless at destruction

	// fields-by-number table
	if (fnrt)					// if table allocated
		for (int fnr = 1; fnr < fnrtNAl; fnr++)		// loop over table entries
		{
			dmfree( DMPP( fnrt[fnr].fieldName));		// decref/free string member
			//note member CHP fp points into buf, not to own heap block.
		}
	dmfree( DMPP( fnrt));			// free the table block if allocated, and NULL pointer. dmpak.cpp.
	fnrtNAl = 0;				// say none allocated

	//record::~record() (call supplied by compiler) zeroes .gud to mark space unused.
}			// IMPF::~IMPF
//-----------------------------------------------------------------------------
/*virtual*/ void IMPF::Copy( const record* pSrc, int options/*=0*/)	// overrides record::Copy. declaration must be same.
// IMPF = beleived used only to copy input records to run, before .fnrt[] allocated in heap, 2-94.
{
	if (fnrt || buf)	// if pointers expected to be NULL are not
		err( PWRN, (char *)MH_S0577);	// "Unexpected call to IMPF::Copy". if msg occurs, complete code re .fnrt, .buf

// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten. dmfree: lib\dmpak.cpp.
	cupfree( DMPP( fileName));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
	cupfree( DMPP( imTitle));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
	// field numbers table .fnrt[] not handled: if non-NULL, must dmfree .fieldNames. before dmfree'ing table pointer.
	// buffer .buf not handled. if non-NULL, must free.

// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED.
	record::Copy( pSrc, options);				// verfies that src and this are same record type. lib\ancrec.cpp.

// increment reference count for pointer(s) just copied. dmIncRec: lib\dmpak.cpp.
	cupIncRef( DMPP( fileName));   	// dmIncRef unless NANDLE or constant inline in pseudocode, cueval.cpp.
	cupIncRef( DMPP( imTitle));    	// dmIncRef unless NANDLE or constant inline in pseudocode, cueval.cpp.
	// field numbers table .fnrt[] not handled: if non-NULL, must copy and incref .fieldNames.
	// buffer .buf not handled. if non-NULL, must copy.
}			// IMPF::Copy
//-----------------------------------------------------------------------------
RC FC IMPF::scanHdr()	// read and decode import file header

// returns non-RCOK on serious error that should stop run, message already issued.
{
	if (hasHeader==C_NOYESCH_NO)   return RCOK;		// nop if file has no header

	BOO imperfect = FALSE;			// flag set if header format seems wrong but may be useable

// line 1: runTitle, runNumber. Not checked.
	if (!readRec())  					// read record, TRUE if 0k
		goto eof;						// if premature eof or error, cannot use file
	if (!scanNextField() || !scanNextField() || scanNextField())	// expect 2 fields & error on 3rd call
		imperfect++;						// wrong # fields: say warn & continue attempting to use file

// line 2: date & time (as 1 field -- quoted). Not checked.
	if (!readRec())
		goto eof;
	if (!scanNextField() || scanNextField())  			// expect 1 field and not another
		imperfect++;

// line 3: title, frequency. Warn if title or frequency does not match.

	if (!readRec())
		goto eof;
	if (!scanNextField()) 				// scan title field / if end record or error
		goto bad;
	if (imTitle)					// if title given in input file
		if (strcmpi( imTitle, fnrt[nFieldsScanned].fp))	// if title in import file is different
			warn( (char *)MH_R1923, 				// "Import file %s: title is %s not %s."
				  fileName, fnrt[nFieldsScanned].fp, imTitle ); 	// use warn to not ++errCount for warning. rmkerr.cpp.
	// continue scanning header: format was ok.
	if (!scanNextField()) 				// scan frequency field / if eor or error
	{
bad:
		return err(  				// gen'l error on screen, err file, ERR report. ++errCount's. rmkerr.cpp.
				   WRN,    			// WRN awaits keypress if not -b, to insure user's attention
				   (char *)MH_R1924, 		// "Import file %s: bad header format. No Run."
				   fileName );
	}
	{
		const char* fileFreq = fnrt[nFieldsScanned].fp;		// where scanNextField left field text pointer
		const char* freqTx = "bug";
		const char* ivlTx = "bug";		// added ivlTx words as used in exports, 8-5-2015
		switch (imFreq)
		{
		case C_IVLCH_Y:
			freqTx = "Annual";
			ivlTx= "Year";
			break;		// these match texts in cgresult.cpp
		case C_IVLCH_M:
			freqTx = "Monthly";
			ivlTx= "Month";
			break;
		case C_IVLCH_D:
			freqTx = "Daily";
			ivlTx= "Day";
			break;
		case C_IVLCH_H:
			freqTx = "Hourly";
			ivlTx = "Hour";
			break;
			//case C_IVLCH_SH: freqTx = "Hourly and Subhourly";  break;	add if used in future
			//case C_IVLCH_S:  freqTx = "SubHourly";   break;			..
		}
		if (strcmpi( fileFreq, freqTx)		// if frequency different in file, warn and continue execution
		 && strcmpi( fileFreq, ivlTx))
		 	// issue message, do not ++errCount (the error count)
			warn( (char *)MH_R1925, 		// "Import file %s: \n    File header says frequency is %s, not %s."
				  fileName, fileFreq,
				  strtprintf( "%s or %s", freqTx, ivlTx));
	}
	if (scanNextField())
		imperfect++;	  				// warn if a third field present

	// issue warning: do on flag to issue only once
	// and not if function terminates with (more serious) error b4 getting here
		if (imperfect)
			warn( (char *)MH_R1926, 	// "Import file %s: Incorrect header format."
				  fileName );			// issue message, rmkerr.cpp. continue execution.

// line 4: field names list: get field numbers for named fields from position of names in list
	if (!readRec())  		// read record into buffer
		goto eof;			// fail if premature eof or other error
	RC rc = RCOK;
	{	// read field names
		while (scanNextField())   		// scan field in place in record buffer. ++'s nFieldsScanned.
		{	char* nm = fnrt[nFieldsScanned].fp;	// point to scanned field text: this is name of field in this position
			fnrtAl(nFieldsScanned);    			// (re)allocate field numbers table if necessary
			fnrt[nFieldsScanned].fieldName = strsave(nm);	// save field name for use in error msgs
		}
#if 0
x only report duplicates if referenced (see below)
x duplicate checks can occur due to autosize or multi-RUN
x producing redundant warnings  11-17-18
x		// detect/warn duplicate field names
x		//   not error unless referenced (see below)
x		int ifn;
x		for (ifn=1; ifn<=nFieldsScanned; ifn++)
x		{	if (fnrt[ ifn].nDecoded < 0)
x				continue;		// previously seen duplicate name, don't re-msg
x			const char* nm = fnrt[ ifn].fieldName;
x			WStr colList;
x			for (int ifn2=ifn+1; ifn2<=nFieldsScanned; ifn2++)
x			{	if (!strcmpi( nm, fnrt[ ifn2].fieldName))
x				{	if (colList.size() == 0)
x						colList = strtprintf( "%d", ifn);		// start list with current col
x					colList += strtprintf( " %d", ifn2);		// add each add'l
x					fnrt[ ifn2].nDecoded = -1;	// is duplicate
x				}
x			}
x			if (colList.size() > 0)
x				warn( "Import file %s:\n    non-unique field name '%s' found in columns: %s",
x					  fileName, nm, colList.c_str());
x		}
#endif
		// find / bind refs from IMPORT()s
		int fnmi;  				// for field name index (fnmt[] subscript)
		IFFNM* iffnm = IffnmB.p + iffnmi;	// point to file's field names table record (or to record 0 if none)
		FNMT* fnmt = iffnm->fnmt;		// fetch pointer to field names table in heap (or a NULL from record 0)
		if (iffnmi   		// if file has field names table record -- else skip
		 && fnmt)   		// and names table record has allocated heap names table -- insurance
		{	for (fnmi = 1; fnmi <= iffnm->fnmiN; fnmi++)	// search names table for this name
			{	for (int ifn=1; ifn<=nFieldsScanned; ifn++)
				{	const char* nm = fnrt[ ifn].fieldName;
					if (!strcmpi( nm, fnmt[fnmi].fieldName))	// if this entry matches
					{	if (fnmt[ fnmi].fnr == 0)	// if name previously seen
						{	fnmt[fnmi].fnr = ifn;  		// store field number with name for use during run
							// break -- NO, check all to detect duplicate names
						}
						else if (fnmt[ fnmi].fnr != ifn)	// if different column already found
						{	// note: duplicate header scans occur if autosize or multiple RUNs
							//       so re-find of same column is OK
							rc = err( "Import file %s:"
								"\n    IMPORT()s refer to ambiguous (non-unique) field '%s'"
								"\n    found in columns %d and %d.",
								fileName, nm, fnmt[ fnmi].fnr, ifn);
						}
					}
				}
			}
		}

		// issue errors for fields used in program but not found in file
		if (iffnmi && fnmt) 		// if have field names record & it has names table (else no field names used in program)
		{	int nBads = 0;
			const char* bads[5];
			for (int i = 0; i < 5; i++)  bads[i] = "";
			for (fnmi = 1; fnmi <= iffnm->fnmiN; fnmi++)  	// search names table for names without field numbers
				if (!fnmt[fnmi].fnr)
				{	if (nBads < 5)
						bads[nBads++] = fnmt[fnmi].fieldName;	// save pointers to first 5 names not found
					else
					{	bads[5-1] = ". . .";    // at 6th name change 5th to elipses and stop
						break;
					}
					// .file and .line available, could be used.
				}
			if (nBads)
				rc = err( (char *)MH_R1927,			// err: general error (++errCount's) rmkerr.cpp. WRN: await keypress.
													/* "Import file %s:\n"
													   "    The following field name(s) were used in IMPORT()s\n"
													   "    but not found in import file %s:\n"
													   "        %s  %s  %s  %s  %s" */
							fileName, fileName,
							bads[0], bads[1], bads[2], bads[3], bads[4] );
		}
	}
	return rc;			// set to RCBAD iff err() calls above

// bad header record read common error exit
eof:
	return err( WRN, (char *)MH_R1928,    	// "Import file %s: \n"
				fileName );					// "    Premature end-of-file or error while reading header. No Run."
}					// IMPF::scanHdr
//---------------------------------------------------------------------------
BOO FC IMPF::readRec()	 // get next import file record in buffer and init to scan fields
// uses bufI2
// sets bufI1, bufI2
// inits eorScanned, nFieldsScanned, fnrt[] for scanning records
// returns FALSE if eof
{
// init (clear) stuff re scanned fields in record

	if (fnrt)				// insurance
		for (int i = 0; i < fnrtNAl; i++)		// loop fields-by-number info for file
		{
			fnrt[i].fp = NULL;			// say no field text yet for new record
			fnrt[i].nDecoded = FALSE; 	// say numeric value not decoded
			fnrt[i].fnv = 0.f;			// field numeric value 0 if accessed by accident or after decode failed
		}

// clear record stuff

	eorScanned = FALSE;
	nFieldsScanned = 0;

// advance to start next record

	ASSERT( bufI1 <= bufI2);	// bufI1 is for field scan, bufI2 points after end prior record
	lineNo++;			// advance line count at entry for correct value during record scan.
	// corresponds to \n at end record, passed below at end prior call. 1st time makes 1-based.
	for ( ; ; )			// loop to pass whitespace and blank lines between records
	{
		bufI1 = bufI2;		// init start record pointer / say readBuf may discard preceding chars
		switch (buf[bufI2])
		{
		case '\r':
			if (!buf[bufI2+1])	// if the \r is at end of buffer
				readBuf();  		// read more of file now so we can check for \n next
			if (buf[bufI2+1]=='\n')	// after \r (carriage return) check for \n (line feed / newline)
				bufI2++;		// \r\n together count as 1 line break. pass the \r now. fall thru.
		case '\n':
			lineNo++;			// count a line for \n, \r\n, or \r w/o \n. fall thru.
		case ' ':
		case '\t':
		case '\f':
			bufI2++;
			continue;		// pass any whitespace char, loop to check next char
		case '\0':
			if (readBuf()) continue;
			break;	// end of buffer: read more of file, loop unless end file
		}
		break;					// any other char (case without 'continue'): exit loop
	}

// find end of record -- be sure it is in buffer

	do						// repeat if encounter end of buffer
		bufI2 += (USI)strcspn( buf + bufI2, "\r\n\f");	// advance to record terminating character (else end buffer)
	while ( (!buf[bufI2]				// if got to end buffer
			 || buf[bufI2]=='\r' && !buf[bufI2+1])	// or to \r at end buffer (need \r\n together for correct line count)
			&&  readBuf() );				// read buffer full / if read any chars
	// (readBuf FALSE if eof or buf full, incl record longer than buf)
	if (bufI1 >= bufI2)				// if no chars read
	{
		eof = TRUE;				// say end file -- all records previously used
		ASSERT(eofRead);				// should only happen if end file already read to buffer
		ASSERT(bufI2==bufN);			// check on readBuf accounting
	}

// pass terminating char now so field scan can change it to \0 without confusing line accounting or next advance to start record

	switch (buf[bufI2])
	{
	case '\r':
		if (buf[bufI2+1]=='\n')	// \r\n together constitute one line break, pass both for correct count
			bufI2++;
		//case '\n':  lineNo++;			don't count here -- see lineNo++ near top of this function
	default:
		bufI2++; 			// pass any non-0 char
		break;

	case '\0':
		if (eofRead)			// \0 expected if entire file has been read to buffer
			break;
		//encountering \0 at end buffer when not end file means record bigger than buffer, or bug.
		// remainder of record will be used as next record, causing errors; error count will end run.
		ASSERT(!bufI1);				// start record is at start buffer or something wrong
		err( WRN, (char *)MH_R1929,			/* "Import file %s, line %d: \n"
								   "    record longer than %d characters" */
			 fileName, lineNo, BUFSZ );
		// err: general error displayer 2-94 that ++errCount's, rmkerr.cpp.
		// don't use rer: can get here b4 run (header) as well as during run.
		break;
	}
	return !eof;			// TRUE if ok
}			// IMPF::readRec
//---------------------------------------------------------------------------
BOO FC IMPF::scanNextField()		// scan next field of current import file record

// sets: .fnrt[++nFieldsScanned].fp 	pointer to null-terminated scanned text in record buffer. move text to keep!
//       .eorScanned			non-0 if end record encountered (return FALSE next time)
// returns FALSE if all fields in record previously used, no message here
{
	if (eof)  return FALSE;			// no fields at all at end file
	SI tFnr = nFieldsScanned + 1;		// field numbers are 1-based
	fnrtAl(tFnr);				// allocate fnrt[] larger if necessary, 0 new space, ABT (no return) if fails
	char *bufPtr = buf + bufI1;   		// point to current position in record buffer
	BOO ok = scanField( bufPtr,  		// scan field / update pointer / false if end record. local, below
						fnrt[tFnr].fp ); 	// receives pointer to scanned field text
	bufI1 = (USI)(bufPtr - buf);		// store file scan position as buffer offset
	if (ok)
		nFieldsScanned++;			// count fields scanned
	return ok;
}			// IMPF::scanNextField
//---------------------------------------------------------------------------
BOO IMPF::scanField(  		// find end of field, null-terminate, dequote, translate \ codes in place
	// inner fcn for scanNextField
	char *&p, 		// input text pointer, returned updated
	char *&start )	// receives start-of-field pointer (after blanks)
// returns FALSE if no more fields in record. uses/sets .eorScanned.
// second quote missing currently 2-94 issues message here and returns good using rest of line as field.

// field ends at unquoted comma or at end record.
// record ends at end line (may be passed) or end file.
// returned field is deblanked at ends, dequoted, null-terminated, \-code-decoded, etc IN PLACE in input buffer.
{
	if (eorScanned)  return FALSE;
	p += strspn( p, " \t");		// pass leading spaces and tabs
	BOO inQuote = FALSE;
	if (*p=='"')
	{
		inQuote++;
		p++;
	}
	start = p;				// set caller's start-field pointer
	char *dest = p;			// pointer for storing dequoted chars
	char *endFld = p;			// pointer after last char excluding unquoted blanks
	for ( ; ; )
	{
		char c = *p;
		if (c)  p++;
		switch (c)
		{
		case '"':
			inQuote = !inQuote;
			break;

		case ',':
			if (!inQuote)  goto breakBreak;			// comma not in quotes ends field
			*dest++ = c;
			endFld = dest; 			// in quotes, comma is a data char
			break;

		case '\r':
			if (*p=='\n')  p++;				// pass \n after \r, fall thru
		case '\n':
		case '\f':
		case 0:
			if (inQuote)
q2m:            // issue "second quote missing" error message showing file name and line of error, and continue.
				// don't use rer: don't need to show time in run, and can get here b4 run (header).
				err( WRN, 				// issue general msg, ++errCount. rmkerr.cpp. WRN: await keystroke.
					 (char *)MH_R1930, 		// "Import file %s, line %d: second quote missing"
					 (const char *)fileName, lineNo );
			eorScanned = TRUE;				// no fields in this record after this one
			goto breakBreak;					// return ok for this field, using all text to end line
			// (but if addl fields were expected, they will error.)
		case '\\':
			c = *p;
			if (c)  p++;
			switch (c)
			{
			case 0:
				if (inQuote)  goto q2m;		// issue "second quote missing" message
				eorScanned = TRUE;
				goto breakBreak;

			case 't':
				c = '\t';
				break;  	// the usual \ - escapes
			case 'r':
				c = '\r';
				break;
			case 'n':
				c = '\n';
				break;
			case 'f':
				c = '\f';
				break;
			case 'e':
				c = 0x1b;
				break;  	// Esc

			default: ;	// fall thru to store char after \, including ", \, other
			}
			// fall thru

		default:
			*dest++ = c;  					// store char. Also see ',' case.
			if (inQuote || !strchr(" \t",c))  endFld = dest; 	// update deblanked field end
			break;
		}
		if (!c)  break;
	}
breakBreak: ;		// end field
	*endFld = '\0';			// terminate after last nonblank or quoted character
	return TRUE;			// field successfully scanned
}			// IMPF::scanField
//---------------------------------------------------------------------------
BOO FC IMPF::readBuf()		// read import file buffer full

// returns FALSE if 0 bytes read, either because end file or because no chars at start file can be discarded
{
	if (eofRead)  return FALSE;					// nothing to do if EOF already read

// move remaining contents to start of buffer
	USI drop = bufI1;
	USI keep = bufN - drop;
	if (drop <=0 && keep >= bufSz)
		return FALSE;					// can't read if no space in buffer
	memmove( buf, buf + drop, keep);			// move remaining text to start buffer
	bufI1 = 0;
	bufI2 -= drop;			// adjust pointer subscripts for more
	bufN = keep;					// ..

// read buffer full
	USI toRead = bufSz - keep;
	char *whereStore = buf + keep;
	size_t nRead = fread(whereStore,sizeof(char), toRead, fh);
	if (nRead== -1)
	{
		err( WRN, (char *)MH_R1931, fileName);		// "Read error on import file %s"
		return FALSE;
	}
	if (nRead==0)			// if end of file with 0 characters left
		eofRead = TRUE;			// say at end of file
	char *end = whereStore + nRead;
	*end = '\0';			// keep buffer terminated with \0. 1 extra byte allocated for this.
	bufN = (USI)(end - buf);  		// new # characters in buffer

// change any (unexpected) \0's and ^Z's in bytes read to spaces.
	// \0 so it doesn't prematurely terminate scans. ^Z (from old editors) so we don't have to handle as terminator elsewhere.
	for ( ;  whereStore < end;  whereStore++)
		if (!*whereStore || *whereStore ==0x1a)
			*whereStore = ' ';
	return TRUE;
}				// IMPF::readBuf
//---------------------------------------------------------------------------
void FC IMPF::close()		// close import file & free buffer

// for impfEnd, and for destructor (cncult4.cpp)
// note: open is done in impfStart.
{
// close file
	if (isOpen)				// if file open
	{
		isOpen = 0;			// say not open
		FILE* tfh = fh;			// save file handle to close
		fh = NULL;				// say closed redundantly
		if (tfh >= 0)			// if open successful - insurance
			if (fclose(tfh) < 0)		// close file
				err( WRN,     			// rmkerr.cpp general error msg with errCount++.
					 (char *)MH_R1932, fileName);   	// "Close error on import file %s"
	}

// free buffer
	dmfree( DMPP( buf));
	bufSz = 0;				// say size of allocated buffer is 0: insurance
}			// IMPF::close
//---------------------------------------------------------------------------
void FC IMPF::fnrtAl(SI nNfnr)		// (re)allocate IMPF.fnrt for given # fields or larger
{
#define CHUNK 32		// number of fields to allocate at a time
	if ( nNfnr >= fnrtNAl 	// if needs to be bigger (>= like +1 for 1-based field numbers)
			||  !fnrt )		// or not allocated yet (insurance)
	{
		SI toAl = fnrtNAl + CHUNK;		// # slots to allocate
		if (!dmral( DMPP( fnrt), 		// (re) allocate, dmpak.cpp. RCOK (0) if successful.
					toAl * sizeof(FNRT),
					ABT|DMZERO ) )		// abort program (no return) if fails.
			fnrtNAl = toAl;			// [if ok], store # elements now allocated
	}
}		// IMPF::fnrtAl
//---------------------------------------------------------------------------

// end of impf.cpp
