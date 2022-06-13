// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// ancrec.h: record and record anchor base class definitions for cse


// before #include, #define NEEDLIKECTOR if including file calls the anc<T> like-another constructor


//************************
//  unsuccessful OPTIONS
//************************


//---------------------------------------------------------------------------------------------------------------------------


#ifndef ANCREC_H	// endif at end file
#define ANCREC_H

#include "SRD.H"

class basAnc;
typedef basAnc* BP;	// basAnc pointer -- formerly used to localize NEARness
#define SZVFTP 4	/* size of C++ virtual function table pointer at start of each record:
 			   skip these bytes in bitwise memset/memcpy operations.
 			   Far forced 2-94 with compile option so same in 16/32 and in large/huge (srfd.cpp is huge) */

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

//***************************************************************************************************************************
//  class basAnc: base class for application record anchors.
//***************************************************************************************************************************
#define RTBRAT  0x2000	/* Old (1992) "Record Array Table" record type bit, now always on here, used in validity checks.
			   ALSO DEFINED IN SRD.H, MUST MATCH. */
//--- bits for basAnc.flags
#define RFSTAT  0x4000	// "static" record basAnc: not reallocable, min subscr 0 not 1. set ancrec.cpp, tested ancrec,cul,cuprobe.cpp.
#define RFTYS   0x1000	// is in TYPES sub-basAnc (pted to by a basAnc.tys)
#define RFNOEX  0x0800	// don't register exprs in records in this basAnc: appl (e.g. cul.cpp) to exman.cpp:exWalkRecs()
#define RFINP   0x0400	// is an input (not run) records basAnc: cul.cpp to cuparse.cpp:probe.  12-91.
#define RFPROBED 0x0200	// set if basAnc has been 'probed': do not free (input) basAnc b4 run. cuparse.cpp:probe to cul.cpp:finalClear.
#define RFLOCAL 0x0100	// flag for private local uses 1-92. used in: cuprobe.cpp:showProbeNames, .
#define RFHEAP  0x0080	// set if block is in heap, else don't dmfree. 5-92.
#define RFPERSIS 0x0040	// (run) record kept from autosize phase for main sim: don't clear its expr uses. 10-95.
//-------------------------------------------------------------------------------------------------------------------------
class basAnc    	// base class for record anchors: basAnc<recordName>
{
  public:
  // members set at construction only
    SFIR* fir;				// pointer to record type's "small fields-in-record table" (in srfd.cpp)
    USI nFlds;				// number of fields excluding base-class members (front overhead)
    const char* what;		// name of the record group (for probes, error messages)
    USI eSz;				// record size
    USI sOff;				// offset in record to "status byte" array at end
    USI ancN;				// anchor number
    RCT rt;					// record type (from rcdef.exe); now mainly for internal checks
  // members set by basAnc mbr fcns
    TI nAl;					// max+1 allocated space subscript (non-static)
    TI mn, n;				// minimum (0 or 1) and max NOT+1 used record subscript (spaces unused if .gud==0)
	inline int GetSSRange() const { return n-mn+1; }	// count INCLUDING UNUSED
	inline int GetSS0() const { return mn; }			// subscript of 0th entry
    int ba_flags;			// bits, RFxxxx defines above -- also set by user
  // members setable by application user language (cul.cpp)
    BP tyB;				// 0 or ptr to user language TYPES anchor in heap (destructor deletes)
    BP ownB;			// 0 or ptr to anchor whose objects own this anchor's objects (record.ownTi)
	const CULT* an_pCULT;	// NULL or associated CULT input table for records of this type
							//   simplifies back translation of input names
    basAnc();
    basAnc( int flags, SFIR * fir, USI nFlds, char * what, USI eSz, RCT rt, USI sOff, const CULT* pCult, int dontRegister=0 );
    void FC regis();
    virtual ~basAnc();  								// destroyed in deriv classes, to use vf
    virtual record* ptr() = 0;							// access block ptr (in drv class: typed)
	virtual void** pptr() = 0;
	virtual void setPtr( record* r) = 0;
    virtual record& rec(TI i) = 0;   // { return (record)((char *)ptr() + i*eSz); }	// access record i
	virtual record* GetAtSafe( int i) const	= 0;				// typed pointer to ith record or NULL
    virtual void* recMbr(TI i, USI off) = 0;					// point record i member by offset
    void * FC recFld(TI i, SI fn);								// point record i member by FIELD # 3-92
    RC FC al( TI n, int erOp=ABT, BP _ownB=NULL);				// allocate records block. destroys old recs.
    RC FC reAl( TI n, int erOp=ABT);							// (re)allocate records block. keeps old recs <= n.
    RC FC free();												// free block
    RC add( record **r, int erOp, TI i=0, const char* _name=NULL);		// add record to block
    RC FC del( TI i, int erOp=ABT);								// delete record i
    void statSetup( record &r, TI n=1, SI noZ=0, SI inHeap=0);  // set up with static record(s)
    static BP FC anc4n( USI an, int erOp=ABT);   				// access anchor by anchor #
    static RC FC findAnchorByNm( char *what, BP *b);
    static SI FC ancNext( USI &an, BP *_b);						// iterate anchors
    RC validate( const char* fcnName, int erOp=ABT, SI noStat=0);		// check for valid anchor
    RC findRecByNm1( const char* _name, TI *_i, record **_r);    		// find record by 1st match on name
    RC findRecByNmU( const char* _name, TI *_i, record **_r);  			// find record by unique name match
    RC findRecByNmO( const char* _name, TI ownTi, TI *_i, record **_r);	// find record by name and owner subscript
    RC findRecByNmDefO( const char* _name, TI ownTi, record **_r1, record **_r2 );	// find record by name, and owner if ambiguous
	const char* getChoiTx( int fn, int options=0, SI chan=-1, BOOL* bIsHid=NULL) const;
	void an_SetCULTLink( const CULT* pCULT) { an_pCULT = pCULT; }
	static void an_SetCULTLinks();

protected:
    virtual void conRec( TI i, SI noZ=0) = 0;			// execute constructor for record i
    virtual void desRec( TI i) = 0;				// .. destructor
    void desRecs( TI mn=0, TI n=32767); 			// destroy all or range of records (as b4 freeing block)
};				// class basAnc

// for ARRAY field, the following are used in each element's status byte
const UCH FsRQD =   1;	// value must be entered (RSBUNDEF) (reflects REQUIRE command only, not CULT flags)
const UCH FsSET =   8;	// value has been set by user, but may be an unevaluated expression, or AUTOSIZE <name> given
const UCH FsVAL =  16;	// value has been stored and is ready to check
const UCH FsERR =  32;	// may be set if error msg issued re field, may be checked to suppress redundant/multiple messages
// for ARRAY field, these bits are used in first element for entire array:
const UCH FsAS =   64;	// AUTOSIZE <name> has been given for field (not expected for array) 6-95
const UCH FsFROZ= 128;	// value cannot be changed (spec'd in type)

//***************************************************************************************************************************
//  class record: base class for application records used with anchors.
//***************************************************************************************************************************
class record		// base class for records
{ public:
// overhead members set by constructor or ancrec.cpp.  CAUTION check operator= if changed.  CAUTION code assumes order of members.
    RCT rt;				// record type (from rcdef.exe); now mainly for internal checks
    TI ss;				// record subscript
    BP b;				// pointer to record's anchor; 0 may indicate unconstructed record space.
    SI gud;				// 0: free; > 0: good record; [<0, skip/retain]. bits 0x7ffe avail to appl.
// overhead members for appl user language (ul).  CAUTION check record::CopyFrom if these members changed.
    TI ty, li;				// 0 or user language TYPE and LIKE subscripts
    int fileIx;   			// 0 or source file name index: see ancrecs:getFileName and getFileIx. 2-94.
    int line;				// 0 or 1-based file line # of object definition (for err msgs)
// base class user members
    // rcdef.exe generates table entries and defines for the following as for derived class members;
    // they are here for uniformity & access via base class ptrs.  CHANGE from old ratpak 2-92: all records have name, ownTi:
    // CAUTION check record::CopyFrom if these members changed.
    ANAME name;				// char name[]
    TI ownTi;				// 0 or subscript of owning object in anchor b->ownB

// base class functions
    record( BP _b, TI i, SI noZ=0);				// construct, as record i for anchor b, 0 mbrs unless noZ
    // override for records with specific destructor requirements such as heap pointers to delete:
    virtual ~record()  { gud = 0; }     		// base destructor: say record space is free
	virtual void DelSubOjects( int /*options*/=0) {}	// override to delete records heap objects
	virtual RC RunDup(const record* pSrc, int options=0) { Copy(pSrc, options); return RCOK;  }
	virtual void ReceiveRuntimeMessage( const char* /*msg*/) { }
	virtual const char* GetDescription(int /*options*/ = 0) { return ""; }
	virtual int ReportBalErrorsIf( int balErrCount, const char* ivlText) const;
  private:
    record() {}					// cannot construct record without basAnc and subscript
  public:
    void* field( int fn); 				// point to member in record by FIELD #
	const void* field( int fn) const;
	int DType(int fn) const;

	void RRFldCopy( const record* r, int fn);
	int RRFldCopyIf( const record* r, int fn)
	{	int ret = !IsSet( fn);
		if (ret)
			RRFldCopy( r, fn);
		return ret;
	}
	void FldCopy( int fnS, int fnD);
	int FldCopyIf( int fnS, int fnD)
	{	int ret = !IsSet( fnD);
		if (ret)
			FldCopy( fnS, fnD);
		return ret;
	}
	template< typename T> void FldSet(		// set field value and status
		int fn,		// field #
		T v)		// value
	{
		int dt = DType( fn);
		int sz = GetDttab( dt).size;
#if defined( _DEBUG)
		if (sz != sizeof( T))
			err( PWRN, "%s:%s FldSet: size mismatch",	// message, wait for keypress
				b->what, name);
#endif
		memcpy( field( fn), &v, sz);
		fStat( fn) |= FsSET | FsVAL;
	}	// record::FldSet
	float FldValFloat(int fn) const;
	inline UCH* fStat()	// access status byte array
	{ return (UCH *)this + b->sOff; }
	inline UCH& fStat(int fn)	// access specific field's status bytes, lvalue use ok
	{ return ((UCH *)this + b->sOff)[fn]; }
	UCH fStat( int fn) const	// ditto not lvalue
	{ return ((UCH *)this + b->sOff)[fn]; }
	inline int IsSet( int fn) const
	{ return (fStat( fn)&FsSET) != 0; }
	RC CkSet( int fn) const;
	int IsSetCount( int fn, ...) const;
	inline void ClrSet( int fn)
	{ fStat( fn) &= ~FsSET; }
	inline int IsVal( int fn) const
	{ return (fStat( fn)&FsVAL) != 0; }
	inline void ClrVal( int fn)
	{ fStat(fn) &= ~FsVAL; }
	int IsValAll( int fn, ...) const;
	inline int IsAusz( int fn) const
	{ return (fStat( fn)&FsAS) != 0; }
	inline int IsSetNotAusz(int fn) const
	{	return (fStat(fn) & (FsSET | FsAS)) == FsSET; 	}
    // override following for records with specific copying req'ts eg heap pointers to dup (dupPtrs does nothing here in base).
    // CAUTION dest's (this) must be init.
    virtual record& CopyFrom( const record* src, int copyName=1, int dupPtrs=0);	// copy user/ul data from another record or data
	record& Copy( const record& d) { Copy( &d); return *this; }
	record& operator=( const record& d) { Copy( &d); return *this; }
	virtual void Copy( const record* pSrc, int options=0);
	virtual int IsCountable([[maybe_unused]] int options ) const { return 1; }
	virtual void FixUp() { };		// optional fixup after reAl()
	void SetName( const char* _name) { strncpy0( name, _name, sizeof( ANAME)); }
	int IsNameMatch( const char* _name) const;
	const char* getChoiTx( int fn, int options=0, SI chan=-1, BOOL* bIsHid=NULL) const;

	// input-checking support functions
	RC ValidateFN(int fn, const char* caller) const;
    // require/disallow/ignore fields
	RC checkN(const char* when, RC(record::* checkFcn)(const char* when, int fn), va_list ap);
	RC checkN(const char* when, RC(record::* checkFcn)(const char* when, int fn), const int16_t* fnList);
    RC disallowN( const char* when, ...);
	RC disallow(int fn) { return disallow(NULL, fn); }
    RC disallow(const char* when, int fn);
	RC disallow(const char* when, const int16_t* fnList);
	template<typename... Args>
	RC disallowX(const char* when, Args... args) { return (disallow(when, args) | ...); }
    RC requireN( const char* when, ...);
	RC require(int fn) { return require(NULL, fn); }
    RC require( const char* when, int fn);
	RC require( const char* when, const int16_t* fnList);
	template<typename... Args>
	RC requireX(const char* when, Args... args) { return (require(when, args) | ...); }
    RC ignoreN( const char* when, ...);
	RC ignore(int fn) { return ignore(NULL, fn); }
    RC ignore( const char* when, int fn);
	RC ignore( const char* when, const int16_t* fnList);
	template<typename... Args>
	RC ignoreX(const char* when, Args... args) { return (ignore(when, args) | ...); }
    // specific error messages
    RC cantGiveEr( int fn, const char* when=NULL);
    RC notGivenEr( int fn, const char* when=NULL);
    RC ignoreInfo( int fn, const char* when=NULL);

    RC FC notGzEr( int fn);
	// change propogation support functions in cncult2.cpp
    void CDEC chafSelf( SI chafFn, ...);
    void CDEC chafN( BP _b, TI i, USI off, ...);
    void chafNV( BP _b, TI i, USI off, va_list ap);

	RC limitCheck( int fn, double vMin, double vMax,
		double vMinWarn=-DBL_MAX, double vMaxWarn=DBL_MAX);
	RC limitCheckFix(int fn, float vMin, float vMax, int erOp = ERR);
	RC limitCheckRatio(int fn1, int fn2, double vMin, double vMax);

	RC CDEC ooer( int fn, const char* message, ... );
	RC CDEC ooerV( int fn, const char* message, va_list ap);
	RC CDEC ooer( int fn1, int fn2, const char* message, ... );
	RC CDEC ooerV( int fn1, int fn2, const char* message, va_list ap);
	RC CDEC oer( const char* message, ... ) const;
	RC CDEC oWarn( const char* message, ... ) const;
	RC CDEC oInfo( const char* message, ... ) const;
	RC CDEC orer(const char* message, ...) const;
	RC CDEC orWarn(const char* message, ...) const;
	RC CDEC orInfo(const char* message, ...) const;

	RC orMsg( int erOp, const char* message, ...) const;

	RC oerI( int shoTx, int shoFnLn, int isWarn, const char* fmt, va_list ap) const;

	record* getOwner() const;
	const char* whatIn() const;
	const char* objIdTx( int op=0) const;
	const char* classObjTx( int op=0) const;
	const char* mbrIdTx( int fn) const;

	RC ckRefPt( BP toBase, TI mbr, const char* mbrName="",
                 record *ownRec=NULL, record **pp=NULL, RC *prc=NULL );

// self-check
	virtual RC Validate( int options=0) { options; return RCOK; }

};					// class record

//***************************************************************************************************************************
//  template class for record anchors
//***************************************************************************************************************************
//---- anchor for record type T

template <class T>  class anc : public basAnc
{ public:
    anc( char *what, SFIR *sFir, USI nFlds, RCT rt, CULT* pCULT=nullptr) 		// cpp'tor used for static instances
        : basAnc( 0, sFir, nFlds, what, sizeof(T), rt, offsetof( T, sstat), pCULT)
        { p = 0; }
    anc( const BP src, int flags, char  *_what,  			// like-another constructor,
         BP _ownB, int dontRegister=0 );    				//  code included only ifdef NEEDLIKECTOR.
    virtual ~anc();							// destroys records, and types anchor & its recs.

    T* p;									// typed pointer to record array storage block
	virtual T* GetAtSafe( int i) const		// typed pointer to ith record or NULL
	{ return i<mn || i>n ? NULL : p+i; }
#if defined( _DEBUG)
	T* GetAt( int i) const;					// typed pointer to ith record (checks i)
#else
	T* GetAt( int i) const { return p+i; }	// typed pointer to ith record (inline)
#endif
	T& operator[]( int i) { return *GetAt( i); }	// typed ref to ith record

    virtual record* ptr()		{ return p; } 		// access block ptr (in base class / generic code)
	virtual void** pptr()		{ return (void **)&p; }
	virtual void setPtr( record* r) { p = (T*)r; }
    virtual record& rec(TI i)	{ return p[i]; }		// access record i in base/generic code
    virtual void * recMbr(TI i, USI off)    { return (void *)((char *)(p + i) + off); }		// point record i member by offset
	RC add( T **r, int erOp, TI i = 0, const char* name=NULL)
	{	return basAnc::add((record **)r, erOp, i, name); }
	RC RunDup( const anc< T> &src, BP _ownB=NULL, int nxRecs=0, int erOp=ABT);
	RC AllocResultsRecs(basAnc& src);
    void statSetup( T &r, TI _n=1, SI noZ=0, SI inHeap=0) { basAnc::statSetup( r, _n, noZ, inHeap); }
	int GetCount( int options=0) const;

 protected:
    void desRecs( TI mn=0, TI n=32767); 			// ~ all records or range (as b4 freeing block)
    virtual void desRec( TI i)
	{	if (p[i].gud)
			p[i].T::~T();	// destroy record if constructed
	}

    virtual void conRec( TI i, SI noZ=0)
	{	// construct record in space i
		// (our operator= requires already constructed)
		new (p+i) T( this, i, noZ);		// use placement new
    }
};	// class anc<T>
//=============================================================================
//----- macro to loop over records of given anchor (type known) (hook to change re skipping deleted records)
#define RLUP( B, rp) for (rp=(B).p+(B).mn;  rp <= (B).p + (B).n;  rp++) if (rp->gud > 0)

//----- macro to loop over records of given anchor (type known) in reverse (hook to change re skipping deleted records)
#define RLUPR( B, rp) for (rp=(B).p+(B).n;  rp >= (B).p+(B).mn;  rp--) if (rp->gud > 0)

//----- macro to loop over records of given anchor (type known) (hook to change re skipping deleted records)
//-----  variant with condition
#define RLUPC( B, rp, C) for (rp=(B).p+(B).mn;  rp <= (B).p + (B).n;  rp++) if (rp->gud > 0 && (C))

#if 0
0 unused
0 //----- macro to loop over records of generic anchor (record type not known) (hook to change re skipping deleted records)
0 #define RLUPGEN( B, rp)  for ( rp = (record *)( (char *)(B).ptr() + (B).eSz*(B).mn );  \
0                               rp <= (record *)( (char *)(B).ptr() + (B).eSz*(B).n );  \
0                               (char *)rp += (B).eSz )    \
0                         if (((record *)rp)->gud > 0)
#endif

//----- macro to loop over records of generic anchor in member fcn (hook to change re skipping deleted records)
#define RLUPTHIS(rp)     for ( rp = (record *)( (char *)ptr() + eSz*mn );  \
                               rp <= (record *)( (char *)ptr() + eSz*n );  \
                               IncP( DMPP( rp), eSz) )    \
                         if (((record *)rp)->gud > 0)
//=============================================================================
#ifdef NEEDLIKECTOR		// define where this constructor is USED: avoids generating for classes where not used.

template <class T> anc<T>::anc( const BP src, int flags, char *_what, BP _ownB, int dontRegister/*=0*/)

	// like-another-with-records-deleted constructor.

	// Generates the same derived class as src, using generic code; assumes size of all anc<T>'s is the same.

{				// for subsidiary types anchor, cul.cpp::ratTyR, 2-92; only used for one (arbitrary) T.
    // must copy: virt fcns, fir,nFlds,eSz,sOff,rt.
    memcpy( this, src, sizeof(anc<T>) );  		// bitwise copy ALL, to incude virt fcn table ptr.
    ancN= nAl= n= 0; p= 0; tyB= 0; 			// clear what don't want: anchor #, records, ul stuff.
    mn = 1;						// mn = 1 when no records allocated even if will be static
    ba_flags = flags;  what = _what;  ownB = _ownB; 	// store members given by caller
    if (!dontRegister)
		regis();			// conditionally include in nextAnc iteration.
							// CAUTION: don't regis() tyB's or any dm anc<T>'s without
							// adding unregister logic to destructor, 10-93.
}				// anc<T>::anc
#endif	// NEEDLIKECTOR
//-------------------------------------------------------------------------------------------------------------------------
template <class T> anc<T>::~anc()			// destroy anchor: destroy its records and types.

// this template d'tor necessary cuz cannot call virtFcns in base class destructor and reach derived class fcn:
// d'tor sets object vftp to its class at entry. !!?. 2-92.
{
// destroy records and free storage block
#if 0	// unrun with RFHEAP added
x    desRecs();  				// destroy any records: calls record-derived class virtual d'tor
x    if (flags & RFHEAP)   			// do not free non-heap (static) block
x       dmfree( DMPP( ptr()));		// free memory block and set ptr() null
x    flags &= ~RFHEAP;
x    n = nAl = 0;				// say no records in use, none allocated: insurance
#else//5-31-92
    free();					// basAnc::free destroys records, dmfrees block, does n = nAl = 0 and mn = 1.
#endif

    // and, for tyB only, .what must be free'd.
	// if this anchor has user language subsidiary 'type' records anchor, destroy it and its records
    if (tyB)					// .tyB is basAnc *
    {
	   delete tyB;
	   tyB = NULL;
    }
}		// anc<T>::~anc
//-------------------------------------------------------------------------------------------------------------------------
template <class T> void anc<T>::desRecs( SI _mn, SI _n)
// ?? why isn't basAnc::desRecs sufficient, 5-31-92?
{
    if (ptr()) 						// if record memory is allocated
       for (TI i = max(mn,_mn);  i <= min(n,_n);  i++) 	// loop allocated record spaces in range given by caller
          desRec(i);					// conditionally destroy record in space with record-deriv d'tor ~T.
}			// anc<T>::desRecs
//-------------------------------------------------------------------------------------------------
template <class T> int anc<T>::GetCount( int options /*=0*/) const
{
	int count = 0;
	const T* pT;
	RLUP( *this, pT)
	{	if (!options || pT->IsCountable( options))
			count++;
	}
	return count;
}		// anc<T>::GetCount
//-----------------------------------------------------------------------------
template <class T> RC anc<T>::RunDup(		// duplicate records for run
	const anc<T> &src,		// source array (e.g. input data)
	BP _ownB /*= NULL*/,	// owner (if any)
	int nxRecs /*=0*/,		// # of extra run records to allocate
							//    (e.g. for "sum-of-all")
	int erOp /*=ABT*/)		// error action re alloc fail
							//   typically ABT, no remedy
// typical use = input -> run

// returns rc from record::RunDup
//   allows record-level run cancel
{
	// delete old records, alloc to needed size for min fragmentation
	RC rc = al(src.n+nxRecs, erOp, _ownB);

	an_pCULT = src.an_pCULT;	// assume same associated CULT

	const T* pT;
	RLUP( src, pT)
	{	T* pNew;
		add( &pNew, ABT, pT->ss); 	// add empty record
									// error not expected due to preceding al
		rc |= pNew->RunDup( pT);	// virtual, base class Copy()s
	}
	return rc;
}		// anc<T>::RunDup
//-----------------------------------------------------------------------------
template <class T> RC anc<T>::AllocResultsRecs(		// allocate/init results records
	basAnc& src)	// Source run records
// allocates N+1 results record< T>s (1..N paired with run recs; N+1 = sum)
// sets names to match src; 0s all values
// returns RCOK iff success
{
	RC rc = RCOK;
	int i = src.n + 1;
	rc = al(i, WRN); // alloc all recs at once for less fragmentation. destroys old.
	T* pR;
	if (!rc) do							
	{	rc |= add(&pR, WRN, i);	// init (to 0) results record, 1 thru i.
		const char* name;
		if (i > src.n)
			name = strtprintf("sum_of_%s", src.what);
		else
		{	name = src.rec(i).name;
			if (IsBlank(name))
				name = strtprintf("%s_%d", src.what, i);
			pR->SetName(name);	// results records have same name as their source
		}
	} while (--i > 0);

	return rc;
}		// anc< T>::AllocResultsRecs
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
template <class T> T* anc<T>::GetAt( int i) const
{
	if (i < mn || i > n)
		warn( "%s GetAt(): %d out of range", what, i);
	return p+i;
}	// anc<T>::GetAt
#endif
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
//  ancrec.cpp non-member fcn declarations
//-----------------------------------------------------------------------------
RC ArrayStatus( const UCH* _sstat0,	int count, int& nSet, int& nVal);
void FC cleanBasAncs( CLEANCASE cs); 	// destroy/free all basAnc records, and delete subsidiary "types" basAncs (.tyB)

// functions for saving file names for record.fileIx 2-94
void clearFileIxs();
int getFileIx( const char* name, int len=-1, char * * pFName4Ix=NULL);
const char* getFileName( int fileIx);
//=============================================================================


#endif	// ifndef ANREC_H at start file

// end of ancrec.h
