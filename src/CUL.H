// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cul.h: for callers of CSE User Language, cul.c */

#include "SYTB.H"		// base class

/*--------------------------- TYPES and DEFINES ---------------------------*/

//---- Phase of session: for culPhase: controls format of some oerI() error msgs ---
enum CULPHASE
{   INPUT_PHASE,	// input: show current input file name & line #
    CHECK_PHASE,	// checking and/or setting up to run: at end object, RUN command, etc: show object's file/line.
    RUN_PHASE		// doing simulation. show ditto. No difference from CHECK_PHASE 6-95.
};

#if 0
/*---- Field (member) Status Byte Bits --------------------------------------

   These are in  unsigned char sstat[]  array generated at end of each record
   structure (in rc___.h) by rcdef.exe and pointed to by XSTK.fs0 and .fs.

   There is 1 sstat[] byte per field, ie per member known in record definition.
   Nested substructs specified in records.def with *substruct and arrays
   specified with *array have one byte per member.  (Aggregates specfied as
   one field (via dtypes.def and fields.def) have only a single byte and will
   not work right if members are treated individually in CULTs.)

   These bits are in the same bytes as the TK/CR rcpak.h RSB____ bits used by
   rcpak.c fcns; while it is not expected that those fcns will be used
   on RATs processed here, I am trying to minimize conflict in definitions.

   Uses 2-91: 1. internally to cul.c;
	      2. tested by user functions pointed to by CULTs (cncult.c,)
	         to determine if field has been entered,
	         re field-interdependent error messages and defaults.
	      3. may be tested by cuprobe.:probe() re input time probes. */

// for ARRAY field, the following are used in each element's status byte
#defined FsSET     8	// value has been set by user, but may be an unevaluated expression, or AUTOSIZE <name> given
#define FsVAL     16	// value has been stored and is ready to check
#define FsERR     32	// may be set if error msg issued re field, may be checked to suppress redundant/multiple messages
// for ARRAY field, these bits are used in first element for entire array:
#ifdef AUTOSIZE
 #define FsAS      64	// AUTOSIZE <name> has been given for field (not expected for array) 6-95
#endif
#define FsFROZ  0x80	// value cannot be changed (spec'd in type)
#define FsRQD      1	// value must be entered (RSBUNDEF) (reflects REQUIRE command only, not CULT flags)
#endif


/*================= CULT: Cal[non]res User Language Table =================*/

/* UFCNPTR: type for ptrs to optional user fcns (see "CULT user fcn calls" comments below):
					     init/default fcn (itf) ptr, in CULT.p2 or non-data CULT.dfpi,
                                             pre-input fcn (prf) ptr, in .p2,
                                             check/finish fcn (ckf) ptr, in .ckf. */
typedef RC FC (*UFCNPTR)( CULT *c, void *p, void *p2, void *p3);		// (FC 7-20-92 at BC 3.1 conversion)

	/* args:  1: loc of CULT entry pointing to fcn (eg for flags).
		  2: item loc: datum if DAT/KDAT entry, ancRec record (entry) loc (else NULL) for RATE,ENDER,NODAT,RUN,CLEAR.
		  3: NULL or next thing out: rat record for datum, record in embedding cult for ancRec record.
		  4: NULL or next record out from there.
       ret value: RCOK: ok.
     		  if non-RCOK value returned, function must issue error msg.
      		      RCFATAL: abort now.
      		      RCBAD/other: skip item, continue compile, disable RUN.
       public variables of possible interest to fcn:
                  defTyping: non-0 if defining a type */
// CULT structure -----------------------------------------------------------

struct CULT	: public STBK // for initialized data to drive user interface
{
	// char* id;	// input word: begins statement; user item name (in base class)
	unsigned cs:4;	// entry type: DAT, RATE, STAR, ENDER etc below.  Max value in table is 15.
	unsigned fn:12;	// field number (DAT).
	USI f;			// input flag bits: RQD, NO_INP, ITFRP, PRFP, ARRAY, etc below; and internal flag bits.
	unsigned uc:4;	// expr use class bits (cncult.h)(always now (1-92) 0=default, use bits to expand evf as needed).
	unsigned evf:12;// DAT: variability: highest (leftmost bit) eval freq ok for expr.  cuevf.h. 9 bits in use 6-95.
	SI ty;			// DAT,KDAT: cul data type, below, TYSI,TYQCH,TYREF,&c.
	void* b;		// ptr to basAnc for RATE or if .ty is TYREF/TYIREF; decoding string if ty is TYQCH.
	void* dfpi;		// DAT,KDAT: integer or pointer default value; "itf" fcn ptr for non-data entries.
	float dff;		// DAT/KDAT: float default, used if TYFL, and if TYNC and .dfpi is 0.
   					//    (b4 C++, Couldn't find a way to init float in void *).
	void* p2;		// variable-use pointer:
					//   if .f & ITFP, is UFCNPTR to init/default fcn, called asap:
					//		when embedding record is added (& again if stmt/group repeated for same memory, if allowed).
					//		(itf ptr may be in .dfpi for non-data entries where p2 is otherwise used)
					//      (former prf call for RATE no longer available (use nested star entry) 1-92)
					//	 if .f & PRFP, is UFCNPTR to pre-input fcn, called upon seeing cmd word, b4 any input value decoded.
					//   if .f & ARRAY, is cast USI size of array in record, incl 0-terminator slot 3-92.
					//   else is CULT * ptr to nested table for RATE.
	UFCNPTR ckf;	// finish/check fcn for all entry types, called after data entry if any,
					//   EXCEPT in main (top) table, is reentry pre-input fcn

	CULT() : STBK(), cs( 0), fn( 0), f( 0), uc( 0), evf( 0),
		  ty( 0), b( 0), dfpi( 0), dff( 0), p2( 0), ckf( 0) { }

	// general form: both void* and float defaults
	CULT( const char* _id, unsigned _cs, unsigned _fn, USI _f, unsigned _uc, unsigned _evf,
		SI _ty, void* _b, void* _dfpi, float _dff, void* _p2, UFCNPTR _ckf)
		: STBK( _id), cs( _cs), fn( _fn), f( _f), uc( _uc), evf( _evf),
		  ty( _ty), b( _b), dfpi( _dfpi), dff( _dff), p2( _p2), ckf( _ckf) { }

	// variant: float default only
	CULT( const char* _id, unsigned _cs, unsigned _fn, USI _f, unsigned _uc, unsigned _evf,
		SI _ty, void* _b, float _dff, void* _p2, UFCNPTR _ckf)
		: STBK( _id), cs( _cs), fn( _fn), f( _f), uc( _uc), evf( _evf),
		  ty( _ty), b( _b), dfpi( NULL), dff( _dff), p2( _p2), ckf( _ckf) { }

	// variant: choice default only
	CULT( const char* _id, unsigned _cs, unsigned _fn, USI _f, unsigned _uc, unsigned _evf,
		SI _ty, void* _b, int _dfpi, void* _p2, UFCNPTR _ckf)
		: STBK( _id), cs( _cs), fn( _fn), f( _f), uc( _uc), evf( _evf),
		  ty( _ty), b( _b), dfpi( (void*)_dfpi), dff( 0.f), p2( _p2), ckf( _ckf) { }

    int cu_ShowDoc( int options=0) const;

};	// struct CULT

/* CULT user function calls ------

		ITF = init/default fcn, ptr in CULT.p2 or non-data CULT.dfpi.
		PRF = pre-input fcn, ptr in .p2,
		CKF = check/finish fcn, ptr in .ckf.
		All user fcns are optional.
		For an aggregate, ITF/PRF/CKF functions can be several places.
		For simplicity, use only one of the places -- STAR generally suggested 11-14-90.

I.  Top level
    A.  Initialization at startup (cul(cs=1)): nested (in main) objects deleted (code incomplete for nested statics), then:
        1. main (top) STAR ITF is called (from nuCult)
        2. main table members (DAT) are defaulted and each DAT/KDAT/NOTDAT mbr's ITF is called (nuCult)
        3. main STAR PRF is called
    B.  Initialization at reentry for more input (cul(cs=2): none of the above, but
        1. re-entry pre-input fcn: main STAR CKF ptr is called here.
    C.  CLEAR command (used only in main but code general)
        1. ITF of CLEAR command table entry
        2. init as at startup (but not STAR PRF):
           a. delete nested objects,
           b. ITF of STAR entry of table containing CLEAR (main expected)
           c. default mbrs, member ITF's
        3. PRF of CLEAR command (in CSE, table uses topStarPrf here also)
        4. CKF of CLEAR command
    D.  RUN command (code not restricted to top)
        a. CKF of RUN command
        z. (CKF of STAR entry no longer called here: now used in main as re-entry PRF, 7-92).
    E.  Completion without RUN: implicit END(s), no more.

II. Nested object (RATE command seen; nested table opened)
    A.  Initialization.  Record is allocated and 0'd (or copied into), then, if not ALTER/LIKE/USETYPE:
        1. ITF of calling RATE table entry is called (recommend avoiding, use STAR ITF when possible)
        2. ITF of STAR table entry is called
        3. table's DAT members are defaulted and each DAT/NODAT/KDAT mbr's ITF is called (if any)
        4. PRF of STAR table entry is called (note NO calling prf call)
    B.  Completion
        1. CKF of ENDER table entry (not if generic or implicit END (usual case))
        2. CKF of STAR table entry called
        3. error messages issued missing RQD members
        5. CKF of calling RATE table entry (confusing: avoid if STAR CKF will suffice)

III. Member Input
    0. DAT/KDAT/NODAT ITF's called at startup (static) or at allocation (at RATE in nesting object).
    1. DAT:   PRF; accept input, store value; CKF.   (regular data input)
    2. KDAT:  PRF; set datum; CKF.
    3. NODAT: PRF; scan past terminating ';'; CKF.
    4. RESET: no calls; probably should ITF (noted in code).
*/

/* CULT notes: ------

1.  Default data is stored before itf's called.
2.  When a CULT is opened, all its non-RATE members are defaulted and itf'd.
    Records are defaulted (to all 0) and itf'd when allocated.
3.  ckf's are called at end of member or record entry; not called for un-entered items.
4.  RQD flags are checked at end of entering a record. */

// CULT.cs entry types ------------------------------------------------------

 #define STAR	   3	/* optional dummy FIRST entry for itf, prf, ckf fcns for ENTIRE TABLE:
 			   so these fcns can be in tbl, not calling tbl entry, and can be used at top level.*/
 #define RATE	   5	// add object: add record to spec'd basAnc & open spec'd new table for its members (formerly "RAT Entry").
 #define DAT	   6	// cmd sets single record member datum to expr
 #define KDAT	   7	// cmd sets datum to constant in .dfpi/.dff
 #define NODAT	   8	// none of the above -- cmd just calls .prf, .ckf.
 #define CLEAR	   9	// CLEAR command: defaults all data, removes entries from RATs & arrays per cult.
 						//   Use its fcns to re-do other init as required.
 #define RUN	  12	// start-execution command (returns 2 from cul.c)
 #define ENDER	  13	// .id is word that ends input for table & pops
 #define END      15	// generic ENDER "end", accepted even if not in table (used in cul.c table "stdVrbs")
		// max table value: 4 bits
 // next 9 are internal to cul.c, not used in tables but must not conflict:
  #define IEND     51	// implicit "end": verb from a calling table seen
  //#define DELETE   52	conflicts with winnt.h 2-94
  #define CDELETE  52	// "delete": erase a record (an object)
  #define ALTER    53	// "alter": reopening record (object) modification
  #define DEFTY    54	// "deftype": defining an object "type" (prototype record)
  #define RQ       55	// "require" <name>
  #define FRZ      56	// "freeze" <name>
  #define RESET    57	// "unset" <name>
 #ifdef AUTOSIZE//cnglob.h
   #define AUTOSZ   63	// "autoSize" <name> 6-95
 #endif
  #define DEND     64	// "$end" or eof, detected by toke(), not in table

// CULT.f flag bits ---------------------------------------------------------

#define RDFLIN	    1	// for TYIREF/TYREF: default to context in which nested, if proper type RATE, else implies RQD.
#define DFLIN       2	// default TYIREF/TYREF per context, but do NOT imply required. eg for report zones.
#define NO_INP      4	// may not be input: just gets init via .dfpi/.dff, .itf.
#define RQD			8	// input required
#define NM_RQD     16	// object name required (else name optional: for LIKEing only) (in RATE table entries).
#define NOTOWNED   32	// This RATE entry does NOT imply basAnc "ownership" (prevents ambig owning; see cul:ratCultO)
#define NO_LITY    64	// do not allow LIKE or USETYPE with this RATE cmd.  No uses 1-91, but keep in case needed.
#define ARRAY     128	// array of DATs, size in p2, input at once as comma-separated list, 3-92.  may control code in cul.c.
#define SUM_OK    256	// TYREF: ok to enter "sum" (TI_SUM (cnglob.h) stored).
#define ALL_OK    512	// TYREF: ok to enter "all" (TI_ALL stored), also "all_but" (TI_ALLBUT) if ARRAY.
#ifdef AUTOSIZE	// cnglob.h
 #define AS_OK    1024	// AUTOSIZE name ok for this member. 6-95.
#endif
	// none avail except NO_LITY and 0x8000. 1-93.
//.p2 use:
#define ITFP       0x0800	// .p2 pts init/default fcn, called after rec added & init (itf ptr may also be in non-data .dfpi)
#define PRFP       0x1000	// .p2 pts to pre-input fcn, called when id word seen, b4 any input decoding (unavail in RATE).
	// else .p2 is CULT ptr (used in RATE entries).
// .f flag bits used internally to cul.c only:
#define BEEN_HERE  0x2000	/* temp flag for gen'l use to avoid duplicate processing of tables pointed to more than 1 place.
				   Used in 1st entry by addVrbs, ; cleared by clrBeenHeres(). */
#define BEEN_SET   0x4000	/* cleared when table opened (nuCult from RATE); set when entry used (RATE,DAT).
				   (vs FsSET: cleared at rec create & kept at like/usetype/alter unless FsRQD;
				              both must be off for errMsg). */


// CULT.ty data types -------------------------------------------------------

//TYSI, TYLLI, TYFL, TYSTR, TYCH, TYNC, TYID: see cuparse.h.
//    --cul type--      --Dttab type(s)--    --comments--
//	TYSI		   DTSI		TYSI cannot accept exprs.
//	TYLLI		   DTLI   	? add SI limits?
//	TYFL		   DTFLOAT
//	TYSTR		   DTCHP	Dm char * is stored
#define TYREF   0x2000	// TI	Reference to record in basAnc .b.  Input by name (like TYID); looked up and subscr stored at RUN.
#define TYIREF	0x3000	// TI.  Immediate-resolution reference to already-defined entry in basAnc .b.  Input like TYID.
						// TYIREF not TYREF used in ownTi's of rats that may own entries in others rats,
						// so set during input, in case used during input re ambiguity resolution
						// (not positive needed) 2-91. Typically used with RDFLIN. */
#define TYQCH	0x6000	// SI or any CHOICB.  "Quick choice", decoded per /-delimited keyword string in cult.b.
                        // historical; still needed 2-91? */
#define TYDOY	0x7000	// DOY	Day of year julian date, input as SI expr (month day syntax accepted).

/* bit assignment CAUTION: only 0xf000 avail here; others used in cuparse.h.
   (and change types to USI to use hi bit to avoid lint warnings) */

/*--------------------------- PUBLIC VARIABLES ----------------------------*/

// intended for testing by user .itf, .prf, .ckf fcns:
extern SI NEAR defTyping;	// non-0 if defining a "type"
							//  (a type is a RATE while setting members)

extern SI NEAR firstCulCall;	// non-0 if first cul call, 0 on reentry (eg after a RUN).
								// added 11-91 and NOT YET USED; delete later.

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// cul.cpp
void FC culClean(CLEANCASE cs);
SI FC cul( SI cs, const char* fName, char *defex, CULT *cult, record *e, BOO *pAuszF=NULL);
TI FC ratDefO( BP b);
const char* FC culMbrId( BP b, unsigned int fn);
const char* FC quifnn( char *s);

// cncult2.cpp
RC   FC ckRefPt( BP toBase, record * fromRec, TI mbr, const char* mbrName="",
                 record *ownRec=NULL, record **pp=NULL, RC *prc=NULL );

// end of cul.h