// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cpgbuild.h -- definitions related to cpgbuild.cpp (CSE page builder)
	         Derived from pgbuildr.h 8-28-91

	 pgbuildr functions use pgpak to build complex displays.
	 See also pgpak for details on the basic routines used. */

/*------------------------------- Option ----------------------------------*/


/*------------------ Defines for pgbuildr() ARGUMENT LIST -----------------*/

	/* pgbuildr() variable arg list: each arg after fixed args is:
	     &PBHEAD (below) + addl args per method, or
	     NULL (ignored), or
	     or a "special function" word: */

// pgbuildr SPECIAL FUNCTIONS used in vbl arg list to terminate, print, etc:
//  LI: same size as pointer
constexpr LI PBSPECOUT = 1;	// "Output" page: call fcn per prFunc arg 
constexpr LI PBSPECFR = 2;	// Free pgpak page
constexpr LI PBSPECEND = 4;	// Terminator for pgbuildr() arg list
constexpr LI PBSPECMAX = 128;	// Any value below this is taken as special

constexpr LI PBDONE = PBSPECOUT | PBSPECFR | PBSPECEND;
						// Handy combo value to end pgbuildr() arg list

/*-------------------------- Defines for PBHEAD ---------------------------*/

	/* PBHEADs (struct below) are pointed to by the pgbuildr vbl arg list
	   and contain once-only (non-array) overall control info, including
	   the "method", plus a pointer to the method-specific PB_xxx table,
	   which is an array with an entry per item to output. */

/* PBHEAD.methopt contains the fundamental identification of now to format
   data on the page.  Each method for adding data to a page is controlled
   by a specific structure and implemented by specific code in pgbuildr.c.

   PBHEAD.methopt contains 2 fields
     1.  Method:   Specifies the page formatting method used.  Low bits have
		   values 0, 1, 2, 3 ... to identify method.  High bits of
		   field (16..128) specify common setup actions: use of these
		   bits simplifies pgbuildr internally; they are NOT options.
     2.  Options:  Currently no options are defined */

#define PBMTHMASK 0x00FF	/* Mask for obtaining method + action from
				   PBHEAD.methopt */
#define PBNOACTMASK 0x000F	/* Mask to clear action bits in method --
				   used to derive subscript into sizetab[]
				   table where action bits are not wanted */
#define PBOPTMASK 0xFF00	/* Mask for obtaining options from .methopt */
			        /*   (No options currently defined) */

/* Actions (internal use; imbedded in method defines) */
#define PBM_IDX   0x0040     /* Multi-record method (all others do 1 record
				or 1 set of args).  Loops over index or RAT
				(Record Array Table, see ratpak.h) to obtain
				records (done by pgbidxmth()).  Data source
				details info in PBHEAD.val1 (below). */
#define PBM_RP    0x0080     /* Method takes a record pointer in next
				pgbuildr() arg.  A RAT entry ptr should work.*/

/*---- METHODS ----  for PBHEAD.methopt. corress PB_xxx structures below. */

/*!!! NOTE -- these defines must match data-init sizetab[] in pgbuildr() !!!*/
#define PBTEXT    0x0000     /* Text; data to be added is specified by char * pointers in a PB_TEXT structure */
#define PBTEXTL   0x000A     /* ditto + label texts left of each text; PB_TEXTL structure. */
#define PBRULE    0x0003     /* Rule: generate rows or columns of repeated characters (eg "----") to form rules */
#define PBFILLTEXT 0x0006    /* Use fill (with word wrapping) to add text specified by char * pointers in PB_TEXT
				structure (same structure as PBTEXT) */
#define PBDATA    0x0007     /* Arbitrary formatted data: driver table specifies row/column,format,datatype,units,
				and pointer to data.  PB_DATA structure. */
#define PBDATAL   0x0008		/* Ditto with label texts to left of data */
#define PBDATOFF  (0x000B | PBM_RP)	/* like PBDATA & PBDATAL except .. */
#define PBDATOFFL (0x000C | PBM_RP)	/* .. record/struct ptr given in call and member offsets only given in
					   table.  Use instead of PBRECORD/L where rcpak not linked. */
/* PBRECORDL 0x0009 defined & described above */
/* PBTEXTL   0x000A defined & described above */
/* PBDATOFF  0x000B .. */
/* PBDATOFFL 0x000C .. */

/*--- PBHEAD.val1 Defines, for methods PGTABLE and PBFILLREC ---*/

/* DATA SOURCES in PBHEAD.val1 (0x000F): */
#define PBTSOURCE_MASK 0x000F
#define PBTSOURCE_IDX	0x0001	/* source = index; pgbuildr() arg
				   n+1 is path to index of data records  */
#define PBTSOURCE_IDX2  0x0002	/* source = 2 level index; pgbuildr()
				   arg n+1 is path to index; n+2 is name
				   of subindex within index. */
#define PBTSOURCE_RAT 0x0005	/* source = RAT (Record Array Table, ratpak.h).
  				   pgbuildr arg n+1 is ptr to RATBASE. */
  /*                          6 = future rat w/breaks per another field? */

/* BREAK CONTROL in PBHEAD.val1 (0x00F0) for PBTABLE/PBFILLREC methods.
   (Breaks are skipped lines, subgroup heads, etc between groups of records).
   Use with PBTSOURCE_IDX2 only: */
#define PBTBREAK_MASK	0x00F0
#define PBTBREAK_IDX2A	0x0010	/* Output "<subindexname>" at subidx change */
#define PBTBREAK_IDX2B	0x0020	/* Output "<indexname> = <subindexname>"
				   at each subindex change. */

/* Other bits (0xFF00) in PBTABLE/PBFILLREC PBHEAD.val1 */
#define PBTDELIDX	0x0100	/* Delete (temp) index after output */

/*----------- Defines used in METHOD TABLES (PB_xxx structures) ----------*/

#define PBMETHEND 0xFFFF     /* Method table terminator, used in .pgfmt member.
				Must not be meaningful pgw fmt (see pgpak.h).
				Much code also uses "-1", 2-90. */

#define PBTBCPABS 0x4000	/* In PB_TABCOL.tbcolpos: column posn in
				   low order bits is absolute, not relative
				   to prior col (1st col always absolute).
				   (Method PBTABLE only) */

#define PBFILL 0x0100 	/* PB_TEXT and PB_DATA only: pgpak format bit done
			   here to cause fill instead of cvin2s and pgw,
			   for putting long messages in table (use CAUTION!)
			   and replacing other types with text.
			   PGINDENT usually desirable with this bit. */
	/* MUST MATCH bit left free by pgpak; also def'd in pgpak.h to check.*/

/* special .text/.pdata ptrs for PB_TEXT, _TEXTREC, _TEXTFILL, _DATA[L]: */
  #define PBARGP ((void *)-2L)	/* get ptr from pgbuildr arg list.
				   One add'l ptr must be given after PBHEAD * and rp for each of these in table. 2-90. */
  #define PBOMITP ((void *)-1L)	/* omit: display no data and no label, but do move "cursor" (by displaying ""). 2-90. */
		/* CHANGE 2-28-90: OMITP and OMITSI suppress label */
/* note: NULL pointer omits in some cases, prints "?" in other cases. */

/* special .field values for PB_RECORD, _TEXTREC, _TABCOL, _FILLREC: */
#define PBARGSI SI(-2)	/* get actual field # (or -1 to omit) from pgbuildr
			   arg list.  (With multi-record methods, only one
			   addl arg is fetched for each PBARGSI.)  Also
			   for: PB_DATA: .dt, .units, .cvfmt, .cvfmt.  2-90.
			   For multiple PBARGSI/PBARGP/PBARGDAT's, arguments
			   are in structure member order except PB_DATA data
			   type precedes data value (but data not pointer).
			   Make work for all SI args? */
#define PBOMITSI   SI(-1)	/* omit (skip): do not display value nor label;
			   do display "" to move "cursor".
			   CAUTION: explicit -1's still lurk in code. 2-90. */
		/* CHANGE 2-28-90: OMITP and OMITSI suppress label */

/*---------------------------- PBHEAD structure ---------------------------*/

/* PBHEAD.  Header table of a pgbuildr() argument group, pointed to in
   pgbuildr's variable argument list.  Contains overall (non-array) control
   info plus pointer to "method" table array of method-dependent type. */
struct PBHEAD
    {	 USI methopt;	 /* Method (low bits) and options (high bits) */
	 void *methtab;	 /* Pointer to method table (PB_xxxx) */
    /* values 1, 2, 3: uses vary by method as noted: */
	 SI val1;	/* PBTEXTREC:            colpos for text
			   PBFILLREC, PBTABLE:   data source and options;
					         see PBTxxx defns above */
	 SI val2;	/* PBTEXTREC:            colpos for data
			   PBTABLE, PBFILLREC:   Heading bottom row
			   			 (pbuildr generated heading)
			   all others:		 0 or heading bot row in
			   			 text supplied by caller
			   			 (repeat if new page).*/
	 SI val3;	/* PBFILLREC, PBTABLE:
	 			0 or # extra lines below method's output to
	 		        allow for in pagination -- so following
	 		        (pagination-dumb) method in same pgbuildr
	 		        call does not exceed page and push whole
	 		        pgpak page to next printer page, 10-30-89.*/
    };

/*------------------- METHOD TABLE (PB_xxx) structures --------------------*/

	/* PBHEAD.methtab points to an array of the PB_xxx structure
	   appropriate to the method (per PBHEAD.methopt).
	   There is one entry in the array for each item to print. */

	/* pgbuildr() code assumes all have .pgfmt and .row,
	   and .col if present, in same place. */

/* Method PBTEXT -- adds arbitrary text at arbitrary rows and columns.
     Struct also used for PBTEXTFILL,
       and matches start of others for PBFILL option.
     PBHEAD.val1, 3 unused.
     PBHEAD.val2: if non-0, # heading rows in caller's text in PAGE,
     to reprint if a new page is begun by caller via pnPgIf. PGCUR +- ok;
     rOff is added; value is passed to pnSetTh() as a service to caller.
     pgbuildr itself does not paginate under non-multi-record methods. */
struct PB_TEXT
   {  USI pgfmt;	/* pgw page format; PBFILL bit may be used
   			   to pgfill instead; or PBMETHEND to end table. */
      SI row;	        /* pgw() row, col.  All pgpak row/col positioning */
      SI col;	        /* ... features supported [except CAUTION: if pgbuildr
			   "rOff" arg is nz, don't use row PGCUR (except
			   in 1st row) but being fixed 2-90: text moved
			   down rOff xtra rows at each PGCUR.]  */
      const char* text;	// Pointer to text, or PBARGP to get a ptr from
      				//   pgbuildr arg list, or PBOMITP to display nothing
   };

/* Method PBTEXTL -- same as above plus label texts positioned left of texts */
/* NOT USED where expected to use 2-28-90; can remove if never used. */
struct PB_TEXTL			/* code assumes same as PB_TEXT + label at end */
   {  USI pgfmt; SI row; SI col; char* text;
      const char* label;	// label text: PGRJ'd to left of .text text. Include
      				//   any desired separating spaces in label text.
   };

/* PBRECORD:  Add data from fields in record to page.
      Take rp in pgbuildr arg list after PBHEAD*.  PBHEAD val1, 3 unused.
      PBHEAD.val2: 0 or # head rows in caller's text, as PBTEXT. */
struct PB_RECORD
   {  USI pgfmt;	/* Page format -- passed directly to pgw */
      SI row;	        /* pgw() row, col.  All pgpak row/col positioning */
      SI col;	        /* ... features supported [ except CAUTION: if pgbuildr
			   "rOff" arg nz, don't use row PGCUR (as PB_TEXT)] */
      SI field;         /* Field #, PBARGSI to get from pgbuildr arg list,
      			   or PBOMITSI to omit field (& PBRECORDL label). */
      SI wid;	        /* Width of area into which data will be formatted.
			    Passed to cvfddisp() */
      SI cvfmt;         /* Data conversion format passed to cvfddisp() */
   };

/* Method PBRECORDL -- same as above plus labels positioned left of fields */
/* NOT USED where expected to use 2-22-90; can remove if never used. */
struct PB_RECORDL			/* code assumes same as PB_RECORD + label at end */
   {  USI pgfmt; SI row; SI col; SI field; SI wid; SI cvfmt;
      char* label;	// label text: PGRJ'd to left of field data. Include
      				// any desired separating spaces in label text.
   };

/* Method PBRULE -- add repeated characters to form rules.
   PBHEAD val1, 3 unused.  val2: 0 or caller's # head rows, as PBTEXT. */
struct PB_RULE
   {  USI pgfmt;	/* Page format -- passed directly to pgw,
			   or PBMETHEND, indicating end of table.
			   If PGVERT, vertical rule will be generated */
      SI row;	        /* pgw() row, col.  All pgpak row/col positioning */
      SI col;	        /* ... features supported [except CAUTION: if pgbuildr
			   "rOff" arg nz, don't use row PGCUR (as PB_TEXT)] */
      SI length;        /* Length of rule to be generated. */
      char text[4];     /* Text string to serve as basis for rule.
			   Generally "-" or "=", but could be e.g. "-*".
			   Due to array dimension, 3 is max length. */
   };

/* Method PBDATA -- adds arbitrary data at arbitrary row/cols.
    Essentially table driven calls to cvin2s.  PBHEAD val1, 3 unused.
      PBHEAD.val2: 0 or # head rows in caller's text, as PBTEXT. */
struct PB_DATA		/* code assumes 1st 4 mbrs match PB_TEXT (for PBFILL)*/
   {  USI pgfmt;	/* pgw page format; PBFILL bit may be used
   			   to pgfill instead (text data only!) */
      SI row;	        /* Row, col passed to pgw.  All pgpak row */
      SI col;	        /*   and col positioning features are supported */
      void *pdata;	/* Pointer to data, or PBARGDAT to get data from
      			   pgbuildr arg list, or PBARGP to get ptr from
      			   pgbuildr arg list, or PBOMITP to skip this one
      			   (and its label if PBDATAL, 2-28-90). */
      USI dt;		/* Data type, or PGARGSI to get from arg list (if data
      			   VALUE also in arglist, it FOLLOWS dt (for size)).*/
      SI units;         /* Units for data, or PBARGSI.  Passed to cvin2s.*/
      SI wid;	        /* Width of area into which data will be formatted,
                            or PBARGSI.  Passed to cvin2s */
      SI cvfmt;         /* cvin2s data conversion format, or PBARGSI */
      /* note: PBARGSI for units, wid needed in crlout2.c 2-90;
                          for dt and cvfmt is speculative as of 2-23. */
   };

/* Method PBDATAL -- same as above plus labels positioned left of fields */
struct PB_DATAL			/* code assumes same as PB_DATA + label at end */
   {  USI pgfmt; SI row; SI col; void *pdata;
      USI dt; SI units; SI wid; SI cvfmt;
      char* label;	// label text: PGRJ'd to left of field data. Include
      				//   any desired separating spaces in label text.
   };

/* Method PBDATOFF: adds data from structure members at given rows/cols;
    also useful for records/rat entries where rcpak not linked.
    Record/structure ptr follows PBHEAD ptr in call.  PBHEAD val1, 3 unused.
      PBHEAD.val2: 0 or # head rows in caller's text, as PBTEXT. */
struct PB_DATOFF		/* code assumes 1st 3 mbrs match PB_TEXT (for PBFILL)*/
{	USI pgfmt;	/* pgw page format; PBFILL bit may be used
		   to pgfill instead (text data only!) */
	SI row;	        /* Row, col passed to pgw.  All pgpak row */
	SI col;	        /*   and col positioning features are supported */
	SI off;		/* Offset to member from rp in pgbuildr arg list,
  			   or PBARGSI to get PTR (not offset) from arg list,
  			   or PBOMITSI to skip this one (and its label) */
	USI dt;		/* Data type, or PGARGSI to get from arg list */
	SI units;         /* Units for data, or PBARGSI.  Passed to cvin2s.*/
	SI wid;	        /* Width of area into which data will be formatted,
                        or PBARGSI.  Passed to cvin2s */
	SI cvfmt;         /* cvin2s data conversion format, or PBARGSI */
};

/* Method PBDATOFFL -- same as above plus labels positioned left of fields */
struct PB_DATOFFL			/* code assumes same as PB_DATOFF + label at end */
{	USI pgfmt; SI row; SI col; SI off;
	USI dt; SI units; SI wid; SI cvfmt;
	const char* label;	// label text: PGRJ'd to left of field data. Include
      					// any desired separating spaces in label text.
};


/*------------------------- FUNCTION DECLARATIONS -------------------------*/

void CDEC pgbuildr( char **ppp, RC *prc, int rows, int cols, int rOffs, const char* title, ...);

                    // CAUTION: any FLOATs in variable arg list are passed as doubles

// end of cgbuild.h
