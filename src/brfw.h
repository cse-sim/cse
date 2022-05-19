// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// brfw.h	header for CSE Binary Results Files Writer

#ifndef _BRF_H
  #include "BRF.H"		// Binary results files structures
#endif

//===========================================================================
//  Binary file I/O subclass for ResfBase:
//    version for writing files only
//===========================================================================
const char * FC pointExt( const char *pNam);
const char * FC pointNamExt( const char *pNam);
//---------------------------------------------------------------------------
class BinFile
{
    BYTE outFile;		// TRUE if output file (pre-created; read files can auto-open)
    BYTE filler;
    INT fh;			// file handle, -1 if file not open or open failed. 16 or 32 bits as compiled.
    long maxSeek;		// max seek pointer value seen = length of output file
public:
    char pNam[_MAX_PATH]; 	// pathName of file, or "" if file not in use

    void clear();
    void inline clean() { if (this) { if (fh >= 0) close(); clear(); } }	// cleanup: close file

    BinFile() { clear(); }
    ~BinFile() { clean(); }

    BOO isOpen() { return (this && fh >= 0); }
    long maxSought() { return maxSeek; }

    // member functions in brfw.cpp
    void init( const char* _pNam, const char* dotExt);	// pNam may be modified in place
    BOO create();
    BOO close();
    BOO seek( DWORD offset);
    BOO write( void *buf, WORD size);
};		// class BinFile
//---------------------------------------------------------------------------

typedef void PAKFCN(void *src, void *dest); 	// PAKFCN: type for fcn to pack a BinBlock

//===========================================================================
// subclass for Memory block associated with part of packed binary file,
//   version for writing only, for Brfw:ResfBase.
//===========================================================================
class BinBlock
{
    //HGLOBAL h;	no handle for non-packed form: always in dmpak block, 12-94.
 #ifdef WINorDLL
    BYTE discardable;	// non-0 to use discardable memory for this block if returned to caller
    BYTE keePak;	/* non-0 to use Windows global memory for write buffers to permit memory handle return to caller
    			   incl to different task, as server to client -- else local memory may be used. 12-3-94. */
    HGLOBAL hPak;	// handle for packed form to optionally return to caller
    // former pPak mbr deleted since don't anticipate wanting to return ptr (not han) to caller, 12-3-94.
 #endif
    BYTE dirty;		// non-0 if block is changed in ram
    BYTE filler;
    WORD  size;		// size of non-packed block
    WORD  sizePak;	// size of packed block, in memory and in file. Packed form not used if pakFcn is NULL.
    BinFile *f; 	// pointer to object for associated file, NULL if not associated with file
    DWORD offset;	// position in file where block has been / will be stored
    PAKFCN *pakFcn;	// NULL or pointer to fcn to pack ram form into packed form
public:
    const char *pNam;  	// pointer to pathName (in f): duplicate for convenience
    void *p;		// pointer to block in memory

    void clear();
    void clean() { if (f->isOpen()) put(); free(); clear(); }

    BinBlock() { clear(); }
    ~BinBlock() { clean(); }

    void setDirty() { dirty++; }
    WORD getSize() { return size; }
    inline void setOffSize( DWORD _offset, WORD _size, WORD _sizePak) { offset = _offset; size = _size; sizePak = _sizePak; }

    // member functions in brfw.cpp. DOS versions.
    void init( BinFile *_f, DWORD _offset, WORD _size, WORD _sizePak,
               BOO discardable /*= FALSE*/, BOO keePak /*= FALSE*/, PAKFCN *pakFcn /*= NULL*/ );
    BOO al(); 	 			// allocate local block, not returned to caller. name was alLock, 12-3-94.
    BOO put();				// write if dirty & free
    BOO write(BOO freeIt);		// write block if using a file, else ok nop. 'freeIt' arg added 12-3-94.
#ifdef WINorDLL
    HGLOBAL removeHan(); 		// return (packed) handle to caller and 0 it here
#endif
    void free();			// free block (for memory return to caller, call removeHan first (WINorDLL only)
};			// class BinBlock
//---------------------------------------------------------------------------

//===========================================================================
//  base structure used while reading or writing binary results file(s):
//	holds handles, pointers, etc for information in memory and/or files
//  base class for ResfWriter, similar to base class for ResFileReader
//===========================================================================
struct ResfBase
{
public:

// disk files: class BinFile {  char pNam[ _MAX_PATH]; INT fh; now lots more };
    BinFile fResf;		// information on building binary results (other than hourly) file, if any
    BinFile fHresf;		// information on building binary hourly results file, if any

// memory blocks: class BinBlock {  [HGLOBAL h;]  void *p;  BinBlock *f;  DWORD offset;  WORD size; BOO dirty; more };
    BinBlock bkResf;			// memory block for entire bldg bin res file
    BinBlock bkHrHdr;			// memory block for header only of bldg bin hourly res file
    BinBlock bkHrMon[12];  		// memory blocks for each month's portion of hourly results file
    #define RESFH   ((ResfHdr *)bkResf.p)
    #define HRESFH ((HresfHdr *)bkHrHdr.p)

    // member functions in brfw.cpp
    void clear();		// clear members. no explicit c'tor / d'tor needed here cuz all members have them.
    void clean();		// clean members
};			// ResfBase
//---------------------------------------------------------------------------

//===========================================================================
//  class to write binary results files from CSE
//===========================================================================
class ResfWriter : public ResfBase
{
    BOO basicOpened, hourlyOpened;	// TRUE if basic, hourly files respectively successfully initialized

    BOO keePak;		// non-0 to retain packed forms for return to caller at close

    SI nZones; 		// # zones specified in open call

    SI firstMonSeen; 	// -1 or first month seen, 0-11
    SI priorMonSeen; 	// -1 or last previous month 0-11 seen, for sequentiality checking

    SI currMi;   		/* month number 0-11 for currMon, or -1 (no month) or -2 (no month cuz out of memory
				   in startMon: suppresses hourly error messages 11-94). */
    HresfMonHdr *currMon;	/* points to block for entire month's hourly info, but typed as tho points to header only.
	    			   Member fcns of header are used to access other info in block. */

    float sumTOuts;		// sum and number of outdoor temps this month, so we can compute average,
    SI nTOuts;   		// ... cuz cse does not average outdoor temps

public:
    void FC clearThis();					// inits most members
    inline void clear()  { ResfBase::clear();  clearThis(); }
    ResfWriter()  { clear(); }
    ~ResfWriter() {}		// and member destructors close files and free blocks

    // member functions in brfw.cpp.  fastcall's commented out where didn't work... scrambled args and this's... crashes.
    RC create( const char* pNam, BOO discardable, BOO basic, BOO hourly, BOO keePak,
                      SI begDay, SI endDay, SI jan1DoW,
  #if 1//5-26-95 CSE .461 RESFVER 8
                      WORD workDayMask,
  #endif
                      SI nZones, SI nEgys,
		      char *repHdrL, char *repHdrR
                      , char *runDateTime
                      , SI nDlwv, char *dlwvName1
  #ifdef TWOWV
                      , char *dlwvName2
  #endif
                     );
    RC /*FC*/ close( struct BrHans *hans);

    // call at start month
    RC FC startMonth( SI mi);

    // call each day
    RC FC setDayInfo( SI mi, SI mdi, SI dowh);

    // call at end hour
    RC /*FC*/ setHourInfo( SI mi, SI mdi, SI hi, WORD shoy,
    			   float tOut, float dlwv, 				// weather
    #ifdef TWOWV//9-94
                           float dlwv2,
    #endif
                           float euClg, float euHtg, float euFan, float euX, 	// whole-bldg energy use
                           float cool, float heat,				// whole-bldg heat flows
                           float litDmd, float litEu );				// whole-bldg lighting eu
    RC /*FC*/ setHourZoneInfo( SI zi, SI mi, SI mdi, SI hi, WORD shoy,
                               float temp, float slr, float ign, float cool, float heat,
                               float litDmd, float litEu );
    // call at end month
    RC /*FC*/ setMonZoneInfo( SI zi, SI mi,
                              float temp, float slr, float ign, float cool, float heat,
                              float infil, float cond, float mass, float iz,
                              float litDmd, float litEu );
    RC /*FC*/ setMonMeterInfo( SI ei, SI mi, MTR *mtr);
    RC FC endMonth( SI mi);

    // call at end run
    RC /*FC*/ setResZone( SI zi, char *name, float area);	// (or could call this one at start)
    RC /*FC*/ setMeterInfo( SI ei, class MTR *mtr);

private:
    RC FC makeMoavs();
};			// class ResfWriter
//---------------------------------------------------------------------------

// end of brfw.h
