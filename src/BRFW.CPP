// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// brfw.cpp  CSE Binary results file writer

#include <cnglob.h>

#ifdef WINorDLL
#if 0
#define STRICT
#include <windows.h>
#endif
#include "cnewin.h"		// for struct BrHans, used in brfw.h and here
#endif

#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>			// _creat

#include <ancrec.h>		// record: base class for rccn.h classes
#include <rccn.h>		// MTR, MTR_IVL_SUB
#include <msghans.h>		// MH_R1960

#include "brfw.h"		// header for this file. includes brf.h.

#include "cse.h"		// ProgName, ProgVersion


//--------------------------------------------------------------------------
// local non-member function declarations
static void NEAR memcpyCheck( void *dest, void *src, size_t size);
static BOO NEAR badSrcOrD( void *src, void *d, const char *fcnName);
const char * FC pointExt( const char *pNam);
const char * FC pointNamExt( const char *pNam);
//---------------------------------------------------------------------------
void FC ResfWriter::clearThis() 	// init most members of resfWriter object, at construction or create()
{
	basicOpened = hourlyOpened = FALSE;
	currMi = -1;
	currMon = NULL;
	nZones = nTOuts = 0;
	sumTOuts = 0.f;
	firstMonSeen = -1;				// init 1st month (determined from calls received)
	priorMonSeen = -1;				// no month just seen (used for sequentiality check)

}	// ResfWriter::clear
//---------------------------------------------------------------------------
// next and several other fastcall's commented out cuz args scrambled, this wrong, crashes: apparent compiler bugs.
// fastcall fails on member fcn with any pointer arg?
//---------------------------------------------------------------------------
RC ResfWriter::create( 	// get set to write binary results files:
	// set members, allocate & init header blocks.
	const char* pNam, 		// base pathName (exts added here IN PLACE), or NULL for memory only
	BOO discardable,		// TRUE to use discardable memory blocks if file also in use (Windows only)
	BOO basic,				// TRUE to create basic building binary results file
	BOO hourly,				// TRUE to create hourly building binary results file
	BOO _keePak,			/* TRUE to retain file form of blocks for handle return to caller at close:
					   must be TRUE if 'hans' pointer will or may be given in close() call.*/
	SI begDay, SI endDay, SI jan1DoW,
	WORD workDayMask,			/* bits for "work days" for output package graphs:
					   Sun=1,Mon=2,Tu=4,W=8,Th=16,F=32,Sat=64,Holidays=128,HtDsDay=256,CoolDsDay=512. */
	SI _nZones, SI nEgys,
	char *repHdrL, char *repHdrR,	// report top left & right headings (E10 to use for project and variant)
	char *runDateTime, 			// run date & time string, 25-27 chars expected (E10 may show in graphs). 8-94.
#ifdef TWOWV
	SI nDlwv, char *dlwvName, char *dlwv2Name	// number of weather variables being set (0,1,2) and their names
#else
	SI nDlwv, char *dlwvName     	// number of weather variables being set (0 or 1) and its name
#endif
	/*,int erOp*/ )			// note from '93: add erOp if needed when error fcns cse-ized

// returns bad if out of memory or cannot create file. Message issued.
{
// clear, save args, general init
	clear();					// zero most members, init file handles to -1, etc.
	nZones = _nZones;				// store arg (nZones stored cuz needed re hourly even if !basic)
	if (!pNam)  				// if no file,
	{
		discardable = FALSE;			// don't make the memory discardable
		_keePak = TRUE;				// do retain all data to return to caller
	}
#ifndef WINorDLL
	/* Issue messages for no file or memory handle return request under DOS 12-94.
	   (memory return not implemented under DOS cuz no handles; could add code to return POINTERS
	   (to subr caller) if need found; existing code generally handles NULL pNam. */
	if (!pNam)
		err( PWRN, (char *)MH_R1964);		// "ResfWriter::create: 'pNam' is NULL, but file must be specified under DOS."
	if (_keePak)
		err( PWRN, (char *)MH_R1965);		/* "ResfWriter::create: 'keePak' is non-0, \n"
						   "    but memory handle return NOT IMPLEMENTED under DOS." */
	_keePak = FALSE;
	discardable = FALSE;			// no discardable memory under DOS
#endif
	keePak = _keePak;				// now store: FALSE says can discard packed data immed after writing to file.
	firstMonSeen = -1;				// init 1st month (determined from calls received)
	priorMonSeen = -1;				// no month just seen (used for sequentiality check)

// init basic binary results file

	if (basic)
	{
		// init resf file info (if none, we just create memory block)
		if (pNam)  					// if file given
			fResf.init( pNam, ".brs");			// init info about it. Apply extension to given pathName.

		// init for memory block to hold entire basic bin res file
		bkResf.init( pNam ? &fResf : NULL, 		// init re memory block to hold file: associate file if given,
					 0L, 				// offset in file: 0: block is whole file
					 // macros yield size needed for entire file with # zones & egys:
					 NEXTEVEN(SizeResfRam(nZones,nEgys)), 	// unpacked data accumulation form
					 NEXTEVEN(SizeResfPak(nZones,nEgys)), 	// packed disk file form
					 discardable,				// non-0 to use discardable memory (believe moot in 32 bit version)
					 keePak, 					// non-0 to retain buffer after write for return to caller
					 (PAKFCN *)packResf );    			// function to generate disk form at write. below.

		// allocate block for entire file, lock, zero
#if 0
x     if (!bkResf.alLock())			// alloc and lock zeroed moveable memory, discardable or not.
#else//12-3-94
		if (!bkResf.al())			// alloc, zero, dirty-flag buffer for unpacked form
#endif
			return RCBAD;				// ... leaves it dirty-flagged.  BOO return!

		// init header info in file block in ram
		RESFH->init( RESFVER, bkResf.pNam, 			// includes init of 12 moav subheaders.
					 -1/*firstMon*/, -1/*nMon*/, nZones, nEgys,	// months filled by startMon call.
					 FALSE );					// not packed during data accumulation
		// header data members not set by init(). also peaks (pkHeatHf, etc) are generated during run
		RESFH->startJDay = begDay;
		RESFH->endJDay = endDay;
		RESFH->jan1DoW = jan1DoW;
#if 1//5-26-95 CSE .461 RESFVER 8
		RESFH->workDayMask = workDayMask;
#endif

		// init other once-only info in file block in ram
		Res *nz = RESFH->nonzone();
		if (nz)							// insurance -- failure not expected
		{
			//maybe these go in header, but compatibility is more easily handled in Res.
			if (repHdrL)  strncpy( nz->repHdrL, repHdrL, sizeof(nz->repHdrL));
			if (repHdrR)  strncpy( nz->repHdrR, repHdrR, sizeof(nz->repHdrR));
			if (runDateTime) strncpy( nz->runDateTime, runDateTime, sizeof(nz->runDateTime));	// 8-94
			nz->nDlwv = nDlwv;									// 9-94
#ifdef TWOWV//bfr.h
			if (dlwvName)  strncpy( nz->dlwvName[0], dlwvName, sizeof(nz->dlwvName[0]));		// 9-94
			if (dlwv2Name) strncpy( nz->dlwvName[1], dlwv2Name, sizeof(nz->dlwvName[1]));		// 9-94
#else
			if (dlwvName)  strncpy( nz->dlwvName, dlwvName, sizeof(nz->dlwvName));		// 9-94
#endif
		}

		// fResf disc file is opened only at ResfWriter::close, when entire file is written in one operation.

		basicOpened = TRUE;
	}

// init for hourly binary results file

	if (hourly)
	{
		// if pathname given set up info for hourly results file (else we just create info in a memory block)
		if (pNam)
			fHresf.init( pNam, ".bhr");

		// init memory block for hourly res file header, associated with file if given, at beginning of file.
		bkHrHdr.init( pNam ? &fHresf : NULL,  0L, sizeof(HresfHdr), sizeof(HresfHdr), discardable, keePak, (PAKFCN *)packHresfHdr);

		// allocate, zero, dirty-flag buffer for unpacked form of hourly file header
		if (!bkHrHdr.al())
			return RCBAD;

		// init hourly file header in memory. Months not known yet, are updated in startMonth.
		HRESFH->init( HRESFVER, bkHrHdr.pNam, -1 /*firstMon*/, -1 /*nMon*/, nZones, FALSE);

		// set info for monthly blocks of hourly file
		for (SI mi = 0; mi < 12; mi++)
			bkHrMon[mi].init( pNam ? &fHresf : NULL,	// same file association (or none) as header block
							  -1L, 0, 0, 			// offset & size are set during run, starting with first month of run
							  discardable, keePak, (PAKFCN *)packHresfMon );
		// months blocks are allocated and init as required, in startMonth().

		// create hourly file and write header to it. Header will be rewritten at close if changed.
		if (*fHresf.pNam)			// if have file for hourly info
			if (!fHresf.create())
				return RCBAD;
#if 0
x       bkHrHdr.write(keePak);			// must fill the space before writing 1st month, I think.
#else//12-3-94 arg moved to init()
		bkHrHdr.write(FALSE);			// must fill the space before writing 1st month, I think.
#endif

		hourlyOpened = TRUE;
	}

	return RCOK;
}			// ResfWriter::create
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::close( 	// finish writing binary results
	// file names, if any, were set by create().

	struct BrHans * hans )	// NULL for file output only (free the memory blocks), or
// pointer to struct to receive memory handles to be returned to CSE caller.
// always NULL under DOS. Hmm.. could return POINTERs under DOS extender, if need found.
// if (hans), keePak must have been TRUE in create call (checked below 12-94).

{
	RC rc = RCOK;

// create, write, close basic file if in use
	if (basicOpened)
	{
		if (*fResf.pNam  &&  !fResf.create())	// if using a file (name not ""), create the file now
			rc = RCBAD;
		else
		{
#if 0
x          if (!bkResf.put(keePak))		// write block (contains whole file) if changed if using file, unlock.
#else//12-3-94
			if (!bkResf.put())			// conditionally pack, write, free block (containing whole file).
#endif
				rc = RCBAD;			// ... if no file, just unlocks (non-discardable) block
			if (*fResf.pNam  &&  !fResf.close())	// if using a file, close it
				rc = RCBAD;
		}
	}

// flush, close hourly file if in use
	if (hourlyOpened)
	{
		for (SI mi = 0; mi < 12; mi++)		// for any months that are locked & changed in ram (expect last only)
#if 0
x          if (!bkHrMon[mi].put(keePak))		// if block dirty, if have file, write block; unlock block.
x             rc = RCBAD;
x       if (!bkHrHdr.put(keePak))		// write header (again) if changed in ram
x          rc = RCBAD;
#else//12-3-94
			if (!bkHrMon[mi].put()) 		// if block dirty, if have file, write houly block & free it.
				rc = RCBAD;
		if (!bkHrHdr.put())			// write header (again) if changed in ram, and free it.
			rc = RCBAD;
#endif
		if (*fHresf.pNam)			// if using a file, not just memory (test for pathname not "")
			if (fHresf.close())			// close the file
				rc = RCBAD;
	}

#ifdef WINorDLL	// memory handles do not exist under DOS. Could rework to return pointers if desired.
// return handles if requested; zero them here so no destructor will free the memory.  .h's are 0 if !basic or !hourly.
	if (hans)
	{
		if (!keePak)				// if !keePak, may not have used global memory with returnable handles.
			err( PWRN, 				// check added 12-3-94
				 (char *)MH_R1966);		// "ResfWriter::close: 'hans' argument given\n    but 'keePak' was FALSE at open"
		hans->hBrs = bkResf.removeHan();
		hans->hBrhHdr = bkHrHdr.removeHan();
		for (SI mi = 0; mi < 12; mi++)
			hans->hBrhMon[mi] = bkHrMon[mi].removeHan();
	}
#endif

// free any remaining packed or unpacked memory blocks whose handles were not returned.
	/* believed normally redundant:
	   needed in some error cases,
	   or if block not changed in RAM (put() might not free)(believed does not occur),
	   and for possible future cases such as DOS block pointer return. 12-94. */
	bkResf.free();
	bkHrHdr.free();
	for (SI mi = 0; mi < 12; mi++)
		bkHrMon[mi].free();

	return rc;
}			// ResfWriter::close
//---------------------------------------------------------------------------
RC FC ResfWriter::startMonth( SI mi)		// start output for a month

// updates run first and last months.
// creates and lock month's hourly data block, if writing an hourly file;
// must be called for months in run in sequence.
{
	/* formerly not handled 12-15-93: what if somebody makes a year run from jan 15 to jan 14??
	   Need to either handle getting same month again here, and handle in receiving application,
	   or disallow, with a message here and perhaps in CSE proper.
	   Minimal check added below in this fcn 12-3-94.
	   Check added in cncult2.cpp 12-4-94. */

	// check sequentiality of calls.  first-, last-, and priorMonSeen are updated at end of this fcn.
	if (basicOpened || hourlyOpened)
		if (priorMonSeen >= 0  &&  mi != priorMonSeen + 1)	// bad if not next month in order
			if (!(priorMonSeen==11 && mi==0))			// except ok to wrap from dec to jan
				err( PWRN, (char *)MH_R1960); 			// "ResfWriter: months not being generated sequentially". continue.

	if (basicOpened)		// if writing basic binary results file
	{
		if (firstMonSeen < 0)					// first time
			RESFH->firstMon = mi;					// record first month
		RESFH->nMon = RESFH->monDiff( mi, RESFH->firstMon) + 1;	// record # months, handling wrap thru new year
		bkResf.setDirty();					// say header changed in ram (believe redundant)
	}

	if (hourlyOpened)		// if writing hourly binary results file
	{
		// if another month is current, endMonth call (which calculates monthly hourly averages) has been skipped
		if (currMi >= 0)
		{
			err( PWRN, (char *)MH_R1961); 	// "ResfWriter::endMonth not called before next startMonth". continue.
			endMonth(currMi);			// do the missing call 11-94. leaves currMi -1.
		}

		// update header to include this month
		if (firstMonSeen < 0)						// first time
			HRESFH->firstMon = mi;					// record first month
		short newNMon = HRESFH->monDiff( mi, HRESFH->firstMon) + 1;	// compute new # months, allowing for wrap thru new year's
		/* 12-94: test for month already seen -- won't work for hourly file unless code added to read block in again
		   Insurance here -- checked in cncult2.cpp:brFileCk & run prevented. */
		if (newNMon <= HRESFH->nMon)			// if didn't add to # months in run
			err( PWRN, (char *)MH_R1967, mi+1);		/* "ResfWriter::startMonth: repeating month %d.\n"
							   "    Run cannot end in same month it started in \n"
							   "    when creating hourly binary results file." */
		// might result from run eg Jan 15 to Jan 14, or from bug.
		HRESFH->nMon = newNMon;				// record # months
		bkHrHdr.setDirty();   				// say header changed -- rewrite at end of run

		// init block position in file (can't do til know first month)
		BinBlock *bkMon = bkHrMon + mi;
		if (bkMon->getSize()==0)
			// this matches HresfHdr::init:
			bkMon->setOffSize( NEXTEVEN(sizeof(HresfHdr)) /* offMon0 */
							   + (long)NEXTEVEN(SizeHresfMonPak(nZones)) /* packed offPerMon*/
							   * (long)HRESFH->runMon(mi),				// packed file offset
							   (WORD)HRESFH->moSize(mi),					// ram block size
							   NEXTEVEN(SizeHresfMonPak(nZones)) /* packed offPerMon*/);	// packed block size

		// create block for month's data in unpacked form
		if (!bkMon->al())				// allocate, zero, dirty-flag block / if failed
		{
			if (currMi==-1)				// redundant insurance
				currMi = -2;				// -2 suppress hourly error msgs in setHourInfo etc, 11-94.
			return RCBAD;					// bad return to caller (probably ignored by caller)
		}

		// init monthly header. repetition (if block existed) would be ok.
		((HresfMonHdr *)bkMon->p)->init( HRESFVER, bkMon->pNam, mi, nZones, FALSE);
		bkMon->setDirty();				// say changed in memory -- must write at unlock (believe redundant)

		// make this the "current month"; it should be completed and endmonth'd (to calc mo-hrly avgs) before another accessed
		currMi = mi;
		currMon = (HresfMonHdr *)bkMon->p;

		// init to average the outdoor temp for the month: CSE doesn't.
		sumTOuts = 0.f;
		nTOuts = 0;
	}

// remember months in run

	if (firstMonSeen < 0)
		firstMonSeen = mi;
	priorMonSeen = mi;
	return RCOK;
}			// ResfWriter::startMonth
//---------------------------------------------------------------------------
RC FC ResfWriter::endMonth( SI mi)  	// finish output for a month

// completes month's hourly file block, writes if dirty, and unlocks it

// CAUTION: expect seek errors will occur if months not endMonth'd in sequential order (checked in startMonth).
{
	BOO ok = TRUE;

// should match preceding startMonth call
	if (hourlyOpened)				// ... but don't have info to check if only basicOpened.
		if (mi != currMi)
			if (mi != -2)				// suppress message if startMonth failed due to lack of memory
				err( PWRN, (char *)MH_R1962); 	// "ResfWriter::startMonth not called before endMonth". continue.

	if (basicOpened)
	{
		// average the outdoor temp for the month: CSE doesn't.
		// doing thus rather than averaging 24 m-h values makes basic value work without hourly. But is this data used?
		if (nTOuts)					// don't /0
		{
			Res *noz = RESFH->nonzone();
			if (noz)
			{
				noz->monTOut[mi] = sumTOuts/(float)nTOuts;	// sum/# = average. set in setHourInfo; init in begMonth.
				noz->ver = 1;				// change value if structure changed
				noz->isSet = TRUE;  			// does not indicate ALL monthly values set!
			}
		}
	}

	if (hourlyOpened)
	{
		// compute hourly averages for month being ended

		if (basicOpened)			// m-h avgs are in basic file
			//if (hourlyOpened)	tested above	// but are computed using hourly data
			if (currMon)		// skip if wasn't set up right by startMonth (insurance)
				if (currMi >= 0)	// more insurance 11-94
					makeMoavs();		// compute & set m-h avgs using hourly data. just below.

#if 0
x   // write changed block (if file in use) and unlock -- don't free, but if discardable, windows may free it.
x   // (if no file is in use, use of non-discardable memory is forced in ResfWriter::create().)
x
x       bkHrMon[currMi].setDirty();	// say changed in memory (believe very redundant)
x       ok = bkHrMon[currMi].put(keePak);
#else//12-3-94
		// if file in use, write and free changed month block. No provision yet implemented to alter month later.

		bkHrMon[currMi].setDirty();	// say changed in memory (believe very redundant)
		ok = bkHrMon[currMi].put();	// conditionally write & free block
#endif
		currMi = -1;			// say no current month -- don't issue "endMonth missing" error message
		currMon = NULL;
	}

	return ok;
}				// ResfWriter::endMonth
//---------------------------------------------------------------------------
RC FC ResfWriter::makeMoavs()	// compute monthly average hourly values for current month

// uses hourly data, sets moav data

// only called if doing both basic and hourly files and currMon/currMi are set ok
{
	if (currMon->lastDaySet < 0)   			// skip if no days set this month (unexpected)
		return RCOK;
	ResfMoavHdr *moav = RESFH->moav(currMi);		// this has .zone(zi) and .nonzoneRam() to point to m-h values
	if (!moav)
		return RCBAD;
	ResMoavRam * mhnoz = moav->nonzoneRam();		// destination for non-zone m-h info

	for (SI zi = 0; zi < nZones; zi++)			// loop over zones
	{
		HresMonZoneRam * z = currMon->zoneRam(zi);  	// source: struct of 31 HresZoneDayRam's for month
		ResMoavZoneRam * mhz = moav->zoneRam(zi);  	// dest: like one HresZoneDayRam, to hold month's averages
		if (z && mhz)					// if accessed both ok, not eg zi bad
		{
			HresMonRam * noz =  zi==0			// month's non-zone hourly results: tOut, dlwv, eu___
								?  currMon->nonzoneRam() 	// do non-zone stuff in iteration for 1st zone
								:  NULL;
			bkHrMon[currMi].setDirty();			// say month block changed in memory
			SI tempT[24], tOutT[24];			// to expand temps from 8 to 16 bits for summing
			register SI i;
			SI nSummed = 0;

			for (SI mdi = currMon->firstDaySet; mdi <= currMon->lastDaySet; mdi++, nSummed++)
			{
				if (!nSummed)				// copy rather than add first data, in lieu of zeroing totals
				{
					for (i = -1; ++i < 24; )  		// copy 8-bit temps to 16 bits
						tempT[i] = z->day[mdi].temp[i];
					memcpy( mhz->slr,   z->day[mdi].slr,   sizeof(mhz->slr));
					memcpy( mhz->ign,   z->day[mdi].ign,   sizeof(mhz->ign));
					memcpy( mhz->cool,  z->day[mdi].cool,  sizeof(mhz->cool));
					memcpy( mhz->heat,  z->day[mdi].heat,  sizeof(mhz->heat));
					memcpy( mhz->litDmd,z->day[mdi].litDmd,sizeof(mhz->litDmd));
					memcpy( mhz->litEu, z->day[mdi].litEu, sizeof(mhz->litEu));
				}
				else
				{
					for (i = -1; ++i < 24; )  		// add 8-bit temps into 16-bit accumulator
						tempT[i] += z->day[mdi].temp[i];
					VAccum( mhz->slr,   24, z->day[mdi].slr);
					VAccum( mhz->ign,   24, z->day[mdi].ign);
					VAccum( mhz->cool,  24, z->day[mdi].cool);
					VAccum( mhz->heat,  24, z->day[mdi].heat);
					VAccum( mhz->litDmd,24, z->day[mdi].litDmd);
					VAccum( mhz->litEu, 24, z->day[mdi].litEu);
				}
				if (noz)			// do non-zone info the first zone's loop (and only if accessed ok)
					if (mhnoz)						// insurance
						if (!nSummed)					// copy rather than add first data
						{
							for (i = -1; ++i < 24; )					// 8-bit temp values: use 16-bit working vector
								tOutT[i] = noz->day[mdi].tOut[i];
							memcpy( mhnoz->dlwv,  noz->day[mdi].dlwv,  sizeof(mhnoz->dlwv));	// copy data first time
#ifdef TWOWV//bfr.h
							memcpy( mhnoz->dlwv2, noz->day[mdi].dlwv2, sizeof(mhnoz->dlwv2));
#endif
							memcpy( mhnoz->euClg, noz->day[mdi].euClg, sizeof(mhnoz->euClg));
							memcpy( mhnoz->euHtg, noz->day[mdi].euHtg, sizeof(mhnoz->euHtg));
							memcpy( mhnoz->euFan, noz->day[mdi].euFan, sizeof(mhnoz->euFan));
							memcpy( mhnoz->euX,   noz->day[mdi].euX,   sizeof(mhnoz->euX));
						}
						else							// after 1st month, add
						{
							for (i = -1; ++i < 24; )
								tOutT[i] += noz->day[mdi].tOut[i];
							VAccum( mhnoz->dlwv,  24, noz->day[mdi].dlwv);
#ifdef TWOWV
							VAccum( mhnoz->dlwv2, 24, noz->day[mdi].dlwv2);
#endif
							VAccum( mhnoz->euClg, 24, noz->day[mdi].euClg);
							VAccum( mhnoz->euHtg, 24, noz->day[mdi].euHtg);
							VAccum( mhnoz->euFan, 24, noz->day[mdi].euFan);
							VAccum( mhnoz->euX,   24, noz->day[mdi].euX);
						}
			}
			if (nSummed)			// divide to form averages
			{
				SI *pTempI = tempT;
				float *pSlr = mhz->slr, *pIgn = mhz->ign, *pCool = mhz->cool, *pHeat = mhz->heat;
				float *pLitDmd = mhz->litDmd, *pLitEu = mhz->litEu;
				float fns = (float)nSummed;
				for (i = -1; ++i < 24; )				// pointer++ assumed to be faster than [i]
				{
					*pSlr++ /= fns;
					*pIgn++ /= fns;
					*pCool++ /= fns;
					*pHeat++ /= fns;
					*pLitDmd++ /= fns;
					*pLitEu++ /= fns;

					mhz->temp[i] = (char)(*pTempI++/nSummed);	// after dividing, put temps in their 8-bit storage
				}
				mhz->isSet = TRUE;
				mhz->ver = 1;				// set ver somewhere, once for speed. change ver value if struct changed.
				if (noz)			// do outdoor temps in first zone's loop iteration (and only if accessed ok)
				{
					if (mhnoz)		// insurance.  ResMoav * mhnoz = moav->nonzoneRam()  is above.
					{
						SI *pTOutT =  tOutT;
						float *pDlwv = mhnoz->dlwv;
#ifdef TWOWV
						float *pDlwv2 = mhnoz->dlwv2;
#endif
						float *pEuClg = mhnoz->euClg, *pEuHtg = mhnoz->euHtg, *pEuFan = mhnoz->euFan, *pEuX = mhnoz->euX;
						for (i = -1; ++i < 24; )
						{
							mhnoz->tOut[i] = (char)(*pTOutT++/nSummed);			// temps: truncate only after /nSummed
							*pDlwv++ /= fns;
#ifdef TWOWV
							*pDlwv2++ /= fns;
#endif
							*pEuClg++ /= fns;
							*pEuHtg++ /= fns;
							*pEuFan++ /= fns;
							*pEuX++ /= fns;
						}
						mhnoz->isSet = TRUE;
						mhnoz->ver = 1;			// set ver somewhere, once for speed. change ver value if struct changed.
					}
				}
			}
		}
	}

	return RCOK;
}		// ResfWriter::makeMoavs
//---------------------------------------------------------------------------
//  Routines called directly by CSE to output data
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setHourInfo( 		// store non-zone hourly info for binary results files

	SI mi, SI mdi, SI hi, 			// 0-based month, day, hour
	WORD shoy,					// subhour of year (redundant, but caller already has it)
	float tOut, float dlwv,				// weather: outdoor temp, daylighting weather variable 8-94.
#ifdef TWOWV//bfr.h 9-94
	float dlwv2,					// second daylighting weather variable 8-94
#endif
	float euClg, float euHtg, float euFan, float euX,	// all-zones, all-meters energy uses 8-94
	float cool, float heat,				// all-zones heat flows (for determining peak)
	float litDmd, float litEu )				// all-zones lighting demand and energy use (svgs computed here)
{
	RC rc = RCOK;
	if (hourlyOpened)
	{
		if (currMi != mi)
		{
			if (currMi != -2)				/* no msg here if failed to get month's memory in startMon():
							   message was issued there, don't need 31*24 more messages!  11-94 */
				err( PWRN, (char *)MH_R1963);		// "startMonth/endMonth out of sync with setHourInfo" 1st of 2 uses
		}
		else
		{
			if (currMon->firstDaySet < 0)
				currMon->firstDaySet = mdi;
			if (mdi > currMon->lastDaySet)
				currMon->lastDaySet = mdi;
			HresMonRam *noz = currMon->nonzoneRam();	// access non-zone hourly results for current month
			if (noz)
			{
				HresDayRam * nozd = noz->day + mdi;	// subscript to day
				nozd->tOut[hi] =  PACKTEMP(tOut);
				nozd->dlwv[hi] =  dlwv;
#ifdef TWOWV
				nozd->dlwv2[hi] =  dlwv2;
#endif
				nozd->euClg[hi] = euClg;
				nozd->euHtg[hi] = euHtg;
				nozd->euFan[hi] = euFan;
				nozd->euX[hi]  =  euX;
				if (hi==23)
				{
					nozd->isSet = TRUE;
					nozd->ver = 1;				// set ver somewhere, once for speed. change ver value if struct changed.
				}
			}
		}
	}
	if (basicOpened)				// put new maxima in peak members in basic binary results file
	{
		/* note 12-2-92: CSE now determines sum-of-zones hvac hf peaks itself; this code could be replaced with
		   a call done once at end of run to pass peaks CSE has determined. */
		if (fabs(cool) > fabs(RESFH->pkCoolHf.q))	// if new peak cooling heat flow for building
			RESFH->pkCoolHf.set( cool, shoy);
		if (heat > RESFH->pkHeatHf.q)			// if new peak heating heat flow for building
			RESFH->pkHeatHf.set( heat, shoy);

		if (fabs(euClg) > fabs(RESFH->pkCoolEu.q))	// if new peak cooling egy use for building
			RESFH->pkCoolEu.set( euClg, shoy);
		if (euHtg > RESFH->pkHeatEu.q)			// if new peak heating egy use for building
			RESFH->pkHeatEu.set( euHtg, shoy);

		if (litDmd > RESFH->pkLitDmd.q)			// if new peak lighting demand for building
			RESFH->pkLitDmd.set( litDmd, shoy);
		if (litDmd - litEu > RESFH->pkLitSvg.q)		// if new peak lighting savings (due to DL (daylighting)) for building
			RESFH->pkLitSvg.set( litDmd - litEu, shoy);	// compute savings; note stored positive.
	}
	sumTOuts += tOut;
	nTOuts++;			// accumulate sum of outdoor temps for averaging in endMonth
	return rc;
} 			// ResfWriter::setHourInfo
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setHourZoneInfo( 	// store hourly stuff for one zone for binary results files

	SI zi, SI mi, SI mdi, SI hi, 				// zone, month, day, hour, all 0-based
	WORD shoy,							// subhour-of-year
	float temp, float slr, float ign, float cool, float heat,	// hourly heat flows to zone (as opposed to from system)
	float litDmd, float litEu )				/* hrly lighting demand (b4 DL), and lit egy used after DL.
							   DL Savings (demand - egy use) is computed here.*/
{
	RC rc = RCOK;
	if (hourlyOpened)
	{
		if (currMi != mi)
		{
			if (currMi != -2)				/* no msg here if failed to get month's memory in startMon():
							   message was issued there, don't need 31*24 more messages!  11-94 */
				err( PWRN, (char *)MH_R1963);		// "startMonth/endMonth out of sync with setHourInfo"  2nd of 2 uses
		}
		else
		{
			HresMonZoneRam * z = currMon->zoneRam(zi);
			if (z)
			{
				HresZoneDayRam * zd = z->day + mdi;
				zd->temp[hi]  = PACKTEMP(temp);
				zd->slr[hi]  =  slr;
				zd->ign[hi]  =  ign;
				zd->cool[hi]  = cool;
				zd->heat[hi]  = heat;
				zd->litDmd[hi]= litDmd;
				zd->litEu[hi] = litEu;
				if (hi==23)
				{
					zd->isSet = TRUE;
					zd->ver = 1;			// set ver somewhere, once for speed. change ver value if struct changed.
				}
			}
		}
	}
	if (basicOpened)
	{
		ResZoneRam *z = RESFH->zoneRam(zi);
		if (z)
		{
			if (fabs(cool) > fabs(z->pkCoolHf.q))	// if new peak cooling heat flow for zone
				z->pkCoolHf.set( cool, shoy);
			if (heat > z->pkHeatHf.q)		// if new peak heating heat flow for zone
				z->pkHeatHf.set( heat, shoy);
			// peaks not set for zone heating/cooling energy use: egy not metered by zone.
			if (litDmd > z->pkLitDmd.q)			// if new peak lighting demand for zone
				z->pkLitDmd.set( litDmd, shoy);
			if (litDmd - litEu > z->pkLitSvg.q)		// if new peak lighting savings for zone
				z->pkLitSvg.set( litDmd - litEu, shoy);	// compute savings as demand - egy use. stored positive.
		}
	}
	return rc;
} 			// ResfWriter::setHourZoneInfo
//---------------------------------------------------------------------------
RC FC ResfWriter::setDayInfo(		// store non-zone daily info for binary results files

	SI mi, SI mdi,	// month 0-11 and day of month 0-30
	SI dowh )		/* day of week 0-6, 0=Sunday, or 7 if holiday. poss future use: 8,9 autoSizing heat & cool dsn days.
			   1 added here for file value. */
{
	if (basicOpened)				// if writing basic results file
	{
		Res *noz = RESFH->nonzone();		// point non-zone results in basic file
		if (noz)							// insurance
		{
			if (mi >= 0 && mi < 12 && mdi >= 0 && mdi < 31)	// if good date: insurance
				noz->dowh[mi][mdi] = (char)(dowh+1);		// store dowh. add 1 in file to reserve 0 for unset.
		}
	}
	return RCOK;
}		// ResfWriter::setDayInfo
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setMonZoneInfo( 	// store monthly info for one zone

	SI zi, SI mi, 		// 0-based zone and month indeces
	float temp, 					// average zone temp for month
	float slr, float ign, float cool, float heat,	// totals for month
	float infil, float cond,				// .. net (signed) infiltration, conduction
	float mass, float iz,				// .. net (signed) mass & interzone heat flows, added 1-94 RESFVER 5.
	float litDmd, float litEu )  			/* .. lighting demand and energy use, separately accumulated
    							      re daylighting, added 9-94 RESFVER 7. */
{
	if (basicOpened)
	{
		if (mi < 0 || mi >= 12)
			return RCBAD;
		ResZoneRam *z = RESFH->zoneRam(zi);
		if (z)
		{
			z->monTemp[mi] =  PACKTEMP(temp);
			z->monSlr[mi] =   slr;
			z->monIgn[mi] =   ign;
			z->monCool[mi] =  cool;
			z->monHeat[mi] =  heat;
			z->monInfil[mi] = infil;
			z->monCond[mi] =  cond;
			z->monMass[mi] =  mass;
			z->monIz[mi]  =   iz;
			z->monLitDmd[mi]= litDmd;
			z->monLitEu[mi] = litEu;
			// z->ver and isSet are set in setResZone.
		}
	}
	return RCOK;
}			// ResfWriter::setMonZoneInfo
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setMonMeterInfo( SI ei, SI mi, MTR *mtr)
{
	if ( basicOpened			// if basic binary results file is open (insurance)
			&&  mi >= 0 && mi < 12 )		// if valid month given
	{
		ResEgyMoRam *emo = RESFH->egyMoRam(ei);	// point monthly info for meter. checks mi range.
		if (emo)
		{
			emo->ver = 1;				// change this constant if structure changed. readers tests it. 1: original.
			emo->isSet = TRUE;			// true if at least 1 month's info stored.
			//strncpy( egy->egyName, mtr->name, sizeof(egy->egyName));	see ResEgy (runly info) for zone name
			MTR_IVL_SUB* m = &mtr->M;		// point month results subrecord in caller's MTR record
			emo->cost[mi] = m->cost;
			emo->dmdCost[mi] = m->dmdCost;
			emo->totUse[mi] = m->tot;
			for (int eu = 0; eu < NENDUSES; eu++)
				emo->endUse[eu][mi] = (&m->clg)[eu];	// clg is 1st end use and others are contiguous in MTR_IVL_SUB.
			emo->dmdQ[mi] = m->dmd;
			emo->dmdShoy[mi] = m->dmd;
			// or x emo->dmd[mi].set( m->dmd, m->dmdShoy);
		}
	}
	return RCOK;
}		// ResfWriter::setMonMeterInfo
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setResZone( 	// store once-per-run zone info for hourly results files
	// may be called at start or end of run
	SI zi, 	// 0-based zone index
	char *name,  	// zone name
	float area )	// floor area
{
	if (basicOpened)
	{
		ResZoneRam *z = RESFH->zoneRam(zi);
		if (z)
		{
			z->ver = 1;					// change this constant if structure changed. readers could test it.
			z->isSet = TRUE;					// does not indicate all monthly values set!
			strncpy( z->zoneName, name, sizeof(z->zoneName));
			z->area = area;
		}
	}
	return RCOK;
}			// ResfWriter::setResZone
//---------------------------------------------------------------------------
RC /*FC*/ ResfWriter::setMeterInfo( 	// store end-of-run meter info and results, for hourly results files

	SI ei,	// 0-based energy source (meter) index
	class MTR *mtr )   	// pointer to CSE meter record containing annual results at end of run
{
	if (basicOpened)
	{
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0       ResEgyRam *egy = RESFH->egyRam(ei);
#else
		ResEgy *egy = RESFH->egy(ei);
#endif
		if (egy)
		{
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0          egy->ver = 2;	// change this constant if structure changed. readers tests it.
0							// 1 original: no .spare, not packed. 2 packed ResEgy 8-94.
#else
			egy->ver = 1;	// change this constant if structure changed. readers tests it. 1: original.
#endif
			egy->isSet = TRUE;
			strncpy( egy->egyName, mtr->name, sizeof(egy->egyName));
			MTR_IVL_SUB* y = &mtr->Y;			// point year results subrecord
			egy->cost = y->cost;
			egy->dmdCost = y->dmdCost;
			// copy tot, 12 end uses
			memcpy( &egy->totUse, &y->tot, (NENDUSES+1)*sizeof(float) );
			egy->dmd.set( y->dmd, y->dmdShoy);
		}
	}
	return RCOK;
}			// ResfWriter::setMeterInfo
//---------------------------------------------------------------------------

//===========================================================================
//  some member functions of some results file classes -- ones used only here in brfw.cpp
//===========================================================================
//  removed fastcalls on all member fcns with a pointer arg... else compiler fails and code crashes.
//---------------------------------------------------------------------------
void /*FC*/ ResfID::init( 				// initialize .id of ResfHdr, HResfHdr, or HResfMonHdr.

	short _ver,		// RESFVER etc for .ver member
	const char *pNam )	// NULL or name.ext of this file, path ok
{
	strncpy( cneName, ProgName, sizeof(cneName));	// set writing program name from public "ProgName" (cse.c)
	strncpy( cneVer,  ProgVersion,  sizeof(cneVer));	// writing program version text from public
	ver = _ver;						// store file format version number
	if (pNam)
	{
		char buf[sizeof(fNamExt)+2];
		memset( buf, 0, sizeof(buf));
		strncpy( buf, pointNamExt(pNam), sizeof(fNamExt));	// copy name.ext only to buf
		strupr(buf);						// convert to upper case 12-4-94
		strncpy( fNamExt, buf, sizeof(fNamExt)); 		// copy to fNamExt member
	}
}		// ResfID::init
//---------------------------------------------------------------------------
void /*FC*/ ResfHdr::init( 	// initialize basic results file header

	short ver, const char *pNam,
	short _firstMon, short _nMon, 	// -1's may be passed for months, if to be determined during run
	short _nZones, short _nEgys,
	BOO pak )

// includes subobject layout members and init of 12 monthly average subheaders.
// in addition, caller must set data members: startJDay, peakHeatM, etc.
{
	id.init( ver, pNam);		// init ResfID substruct members, including ProgName and ProgVer from publics

	firstMon = _firstMon;
	nMon = _nMon;
	nZones = _nZones;
	nEgys =  _nEgys;
	isPakd = pak;

	//startJDay, endJDay, jan1DoW, workDayMask:  caller sets directly

	pkHeatHf.init();
	pkCoolHf.init();		// init peaks to 0 for accumulation during run
	pkHeatEu.init();
	pkCoolEu.init();
	pkLitDmd.init();
	pkLitSvg.init();

//lay out subobjects. see also packing function packResf() below.
	fileSize = pak ? NEXTEVEN(SizeResfPak(_nZones,_nEgys)) 	// size of entire file with # zones & egys: to use when reading
			   : NEXTEVEN(SizeResfRam(_nZones,_nEgys));
	resOff =  NEXTEVEN(sizeof(ResfHdr));
	resSize = NEXTEVEN(sizeof(Res));
	zoneOff =  resOff + resSize;
	perZone = pak ? NEXTEVEN(sizeof(ResZonePak)) : NEXTEVEN(sizeof(ResZoneRam));
	egyOff  = zoneOff + nZones * perZone;
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0    perEgy =  pak ? NEXTEVEN(sizeof(ResEgyPak)) : NEXTEVEN(sizeof(ResEgyRam));
#else
	perEgy =  NEXTEVEN(sizeof(ResEgy));
#endif
	egyMoOff = egyOff + nEgys * perEgy;
	perEgyMo = pak ? NEXTEVEN(sizeof(ResEgyMoPak)) : NEXTEVEN(sizeof(ResEgyMoRam));
	moavOff =  egyMoOff + nEgys * perEgyMo;
	perMoav = pak ? NEXTEVEN(SizeResfMoavPak(nZones)) : NEXTEVEN(SizeResfMoavRam(nZones));

// initialize all 12 subheaders for 12 possible monthly averages subsections of file, all in same memory block.
	for (SI mi = 0; mi < 12; mi++)
		moavx(mi)->init(_nZones, pak);		// init: fcn just below. moavx: access despite firstMon/nMons.
}				// ResfHdr::init
//---------------------------------------------------------------------------
void FC ResfMoavHdr::init( short _nZones, BOO pak)

// initialize header of ONE MONTH's monthly average subpart of results file
// called from ResfHdr.init().
{
	ver = 1;				// constant to change if format changed. readers could test.
	nZones = _nZones;			// number of zones in month block: same as in entire file
	isPakd = pak;
	offNonzone = NEXTEVEN(sizeof(ResfMoavHdr));
	offZone0 = offNonzone + ( pak ? NEXTEVEN(sizeof(ResMoavPak))
							  : NEXTEVEN(sizeof(ResMoavRam))    );
	offPerZone = pak ? NEXTEVEN(sizeof(ResMoavZonePak))
				 : NEXTEVEN(sizeof(ResMoavZoneRam));
}		// ResfMoavHdr::init
//---------------------------------------------------------------------------
void /*FC*/ HresfHdr::init( 		// initialize hourly results file header: use when creating file

	short ver, const char *pNam,
	short _firstMon, short _nMon, 	// can be -1 if months not known
	short _nZones,
	BOO pak )
{
	id.init( ver, pNam);			// init id substruct members
	firstMon = _firstMon;
	nMon = _nMon;	// init members
	nZones = _nZones;
	isPakd = pak;				// flag packed or not cuz size of parts different
// file layout. see also startMon above and packing function below.
	offMon0 = NEXTEVEN(sizeof(HresfHdr));	// first month is after header
	offPerMon = pak ? NEXTEVEN(SizeHresfMonPak(_nZones))  	// macro yields month block size
				: NEXTEVEN(SizeHresfMonRam(_nZones));
}		// HresfHdr::init
//---------------------------------------------------------------------------
void /*FC*/ HresfMonHdr::init( short ver, const char *pNam, short _mon, short _nZones, BOO pak)
{
	id.init( ver, pNam);		// set id substruct members: writing program name, file name.ext, etc.
	month = _mon;			// month this block represents, 0-11
	nZones = _nZones;			// number of zones in this block
	nDaysAlloc = 31;			// currently (11-93) allocating 31 days for each month
	firstDaySet = lastDaySet = -1;		// set to 0-based day numbers as data is stored
	isPakd = pak;
//lay out substructures. see also packing function below.
	offNonZone = NEXTEVEN( sizeof(HresfMonHdr));
	offZone0 =   offNonZone + (pak ? NEXTEVEN( sizeof(HresMonPak)) : NEXTEVEN( sizeof(HresMonRam)) );
	offPerZone = pak ? NEXTEVEN( sizeof(HresMonZonePak)) : NEXTEVEN( sizeof(HresMonZoneRam));
};		// HresfMonHdr::init
//---------------------------------------------------------------------------
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0 void ResEgyPak::pack( ResEgyRam *src)	// pack energy info from unpacked ram form
0{
0     if (badSrcOrD( src, this, "ResEgyPak::pack"))  return;
0
0     ver = src->ver;			// copy members before packed floats (.pf)
0     isSet = src->isSet;
0     strncpy( egyName, src->egyName, sizeof(egyName));
0     cost = src->cost;
0     dmdCost = src->dmdCost;
0     totUse = src->totUse;		// ..
0
0     endUsePak.pack( src->endUse);	// pack vector of 12 floats of similar range into small space
0
0     dmd = src->dmd;			// copy member after packed floats (.pf)
0}			// ResEgyPak::pack
#endif
//---------------------------------------------------------------------------
void ResEgyMoPak::pack( ResEgyMoRam *src)	// pack monthly energy info from ram form to disk form
{
	if (badSrcOrD( src, this, "ResEgyMoPak::pack"))  return;

	ver = src->ver;				// copy members before packed floats
	isSet = src->isSet;
	//strncpy( egyName, src->egyName, sizeof(egyName));

	cost.pack( src->cost);			// pack float[12] vectors of montly values
	dmdCost.pack( src->dmdCost);
	totUse.pack( src->totUse);
	for (int i = 0; i < NENDUSES; i++)		// each end use has a [12] vector
		endUse[i].pack( src->endUse[i]);
	dmdQ.pack( src->dmdQ);

	for (i = 0; i < 12; i++)
		dmdShoy[i] = src->dmdShoy[i];		// copy month peak subhour-of-year's (not floats)

}			// ResEgyMoPak::pack
//---------------------------------------------------------------------------

//===========================================================================
// member functions of ResfBase -- base class for ResfWriter
//===========================================================================
void ResfBase::clear()
{
	bkResf.clear();
	bkHrHdr.clear();
	for (SI i = 0; i < 12; i++)
		bkHrMon[i].clear();
}				// ResfBase::clear
//---------------------------------------------------------------------------
void ResfBase::clean()
{
	bkResf.clean();
	bkHrHdr.clean();
	for (SI i = 0; i < 12; i++)
		bkHrMon[i].clean();
}				// ResfBase::clean
//---------------------------------------------------------------------------

//===========================================================================
// non-member functions to pack results file blocks before writing to disk.
//    Used as "pakFcn" arguments to BinBlock::init.
//    Data is accumulated in non-packed form as packers determine scale
//       factors from largest value in vectors of float values for day, year, etc.
//===========================================================================
void packResf( 				// pack basic binary results file

	ResfHdr *src, ResfHdr *d )		// packs entire file block despite args typed for header only
{
	if (badSrcOrD( src, d, "packRresf"))   return;	// message NULL pointers, avoid GP fault

// header. copy then change flag and layout.
	memcpy( d, src, sizeof(ResfHdr));
	d->isPakd = TRUE;
	d->fileSize = NEXTEVEN(SizeResfPak(d->nZones,d->nEgys)); 	// size of entire file with # zones & egys: to use when reading
	d->resOff =  NEXTEVEN(sizeof(ResfHdr));
	d->resSize = NEXTEVEN(sizeof(Res));
	d->zoneOff = d->resOff + d->resSize;
	d->perZone = NEXTEVEN(sizeof(ResZonePak));
	d->egyOff  = d->zoneOff + d->nZones * d->perZone;
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0    d->perEgy =  NEXTEVEN(sizeof(ResEgyPak));
#else
	d->perEgy =  NEXTEVEN(sizeof(ResEgy));
#endif
	d->egyMoOff = d->egyOff + d->nEgys * d->perEgy;
	d->perEgyMo = NEXTEVEN(sizeof(ResEgyMoPak));
	d->moavOff = d->egyMoOff + d->nEgys * d->perEgyMo;
	d->perMoav = NEXTEVEN(SizeResfMoavPak(d->nZones));

// nonzone info
	memcpyCheck( d->nonzone(), src->nonzone(), sizeof(Res));

// zones
	for (SI zi = 0;  zi < src->nZones;  zi++)
	{
		ResZoneRam *zsrc =  src->zoneRam(zi);
		ResZonePak *zdest = d->zonePak(zi);
		if (badSrcOrD( zsrc, zdest, "packResf zones loop"))  continue;
		zdest->ver      = zsrc->ver;
		zdest->isSet    = zsrc->isSet;
		memcpy( zdest->zoneName, zsrc->zoneName, sizeof(zdest->zoneName));
		zdest->pkCoolHf   = zsrc->pkCoolHf;   				// copy structs
		zdest->pkHeatHf   = zsrc->pkHeatHf;
		//zdest->pkCoolEu   = zsrc->pkCoolEu;				eu not now recorded by zone, 9-94
		//zdest->pkHeatEu   = zsrc->pkHeatEu;
		zdest->pkLitDmd   = zsrc->pkLitDmd;
		zdest->pkLitSvg   = zsrc->pkLitSvg;
		memcpy( zdest->monTemp, zsrc->monTemp, sizeof(zdest->monTemp));
		zdest->area     = zsrc->area;
		zdest->spare    = zsrc->spare;
		zdest->monSlr.pack(zsrc->monSlr);		// pack each float vector
		zdest->monIgn.pack(zsrc->monIgn);
		zdest->monCool.pack(zsrc->monCool);
		zdest->monHeat.pack(zsrc->monHeat);
		zdest->monInfil.pack(zsrc->monInfil);
		zdest->monCond.pack(zsrc->monCond);
		zdest->monMass.pack(zsrc->monMass);		// 1-2-94, RESFVER 5
		zdest->monIz.pack(zsrc->monIz);			// 1-2-94, RESFVER 5
		zdest->monLitDmd.pack(zsrc->monLitDmd);		// 9-3-93, RESFVER 7
		zdest->monLitEu.pack(zsrc->monLitEu);		// 9-3-93, RESFVER 7
	}

// energy sources
	for (SI ei = 0;  ei < src->nEgys;  ei++)		// loop over sources (meters)
	{
#ifdef PACKRESEGY	// brf.h 8-19-94 undone
0       d->egyPak(ei)->pack( src->egyRam(ei));			// pack annual (once-per-run) info for this meter
#else
		memcpyCheck( d->egy(ei), src->egy(ei), sizeof(ResEgy));	// copy annual (once-per-run) info for this meter
#endif
		d->egyMoPak(ei)->pack( src->egyMoRam(ei));		// pack monthly info for this meter
	}

// moav
	for (SI mi = 0;  mi < 12;  mi++)				// loop months
	{
		ResfMoavHdr *moavSrc = src->moav(mi);
		ResfMoavHdr *moavd = d->moavx(mi);
		if ( moavSrc						// NULL if nothing for this month
				&&  !badSrcOrD( (void *)1L, moavd, "packResf moav loop") )	// message if bad destination
		{
			// header
			memcpy( moavd, moavSrc, sizeof(ResfMoavHdr));		// copy the header
			moavd->isPakd = TRUE;					// change header for packed: packed flag,
			moavd->offNonzone = NEXTEVEN(sizeof(ResfMoavHdr));			// subobject layout (some redundant)
			moavd->offZone0 = moavd->offNonzone + NEXTEVEN(sizeof(ResMoavPak));	// ..
			moavd->offPerZone = NEXTEVEN(sizeof(ResMoavZonePak)); 		// ..
			// non-zone
			moavd->nonzonePak()->pack( moavSrc->nonzoneRam());			// pack non-zone monthly hourly averages
			// zones
			for (zi = 0;  zi < moavSrc->nZones;  zi++)
				moavd->zonePak(zi)->pack( moavSrc->zoneRam(zi));			// pack zone month moav with mbr fcn, below
			// note 12-93 calls HresZoneDayPak::pak -- #define'd same brf.h.
		}
	}
}		// packResf
//---------------------------------------------------------------------------
void packHresfHdr( HresfHdr *src, HresfHdr *d)	// pack hourly binary results file header block
{
	if (badSrcOrD( src, d, "packHresfHdr"))   return;	// message NULL pointers, avoid GP fault

// packed form is same except for flag and offsets to month blocks
	memcpy( d, src, sizeof(HresfHdr));
	d->isPakd = TRUE;						// packed flag
	d->offMon0 = NEXTEVEN(sizeof(HresfHdr));  			// first month is after header (no change -- redundant)
	d->offPerMon = NEXTEVEN(SizeHresfMonPak(d->nZones));	// size of each month in packed file
}	// packHresfHdr
//---------------------------------------------------------------------------
void packHresfMon( 			// pack an hourly month block

	HresfMonHdr *src, HresfMonHdr *d )	// packs whole block despite arg typed as header
{
	if (badSrcOrD( src, d, "packHresfMon"))   return;	// message NULL pointers, avoid GP fault

// packed form OF HEADER PART is same except for flag and offsets to substructures
	memcpy( d, src, sizeof(HresfMonHdr));
	d->isPakd = TRUE;					// packed flag
	d->offNonZone = NEXTEVEN( sizeof(HresfMonHdr));
	d->offZone0 =   d->offNonZone + NEXTEVEN( sizeof(HresMonPak));
	d->offPerZone = NEXTEVEN( sizeof(HresMonZonePak));

// pack non-zone info: outdoor temps, selected bldg energy use info
	HresMonRam *nozMoSrc = src->nonzoneRam();
	HresMonPak *nozMoD = d->nonzonePak();
	if (!badSrcOrD( nozMoSrc, nozMoD, "packHresfMon"))	// msg if either pointer NULL else ...
	{
		nozMoD->ver = nozMoSrc->ver;			// copy month struct version
		for (int mdi = 0; mdi < 31; mdi++)		// loop days of month. or should nDaysAlloc be used (same anyway 8-94)?
			nozMoD->day[mdi].pack( &nozMoSrc->day[mdi]);	// pack one day (mbr fcn below)
	}

// pack zones
	for (SI zi = 0;  zi < src->nZones;  zi++)
	{
		HresMonZoneRam *zsrc = src->zoneRam(zi);
		HresMonZonePak *zdest = d->zonePak(zi);
		if (badSrcOrD( zsrc, zdest, "pakcHresfMon zones loop"))  continue;
		zdest->ver = zsrc->ver;
		for (SI domi = 0;  domi < src->nDaysAlloc;  domi++)
			zdest->day[domi].pack( &zsrc->day[domi]);			// pack zone day hourly results using mbr fcn, below
	}
}	// packHresfMon
//---------------------------------------------------------------------------
//  member packing functions for substructures. called in packing fcns above.
//---------------------------------------------------------------------------
void HresDayPak::pack( HresDayRam *src) 	// pack an HresDayRam into 'this' (or a ResMoavRam -- #defined same in brf.h)
{
	if (badSrcOrD( src, this, "HresDaypak::pack"))  return;

	ver = src->ver;				// copy struct version for poss future checking
	isSet = src->isSet;				// copy "data has been set" flag for day

	memcpy( tOut, src->tOut, sizeof(tOut));	// copy outdor temps (they were byte-packed when stored)

	dlwv.pack(src->dlwv);			// vector-pack the float info using member fcn
#ifdef TWOWV //bfr.h 9-94
	dlwv2.pack(src->dlwv2);
#endif
	euClg.pack(src->euClg);
	euHtg.pack(src->euHtg);
	euFan.pack(src->euFan);
	euX.pack(src->euX);
}			// HresDayPak::pack
//---------------------------------------------------------------------------
void HresZoneDayPak::pack( HresZoneDayRam *src)		// pack hourly zone results for a day
// (or month moav for zone: ResMoavZonePak/Ram #define'd same in brf.h)
{
	if (badSrcOrD( src, this, "HresZoneDayPak"))   return;	// message NULL pointers, avoid GP fault

	ver = src->ver;
	isSet = src->isSet;		// copy version and data-set flags

	memcpy( temp, src->temp, sizeof(temp));		// copy temperatures (they were byte-packed when stored)

	slr.pack( src->slr);				// scale and pack each float vector
	ign.pack( src->ign);
	cool.pack( src->cool);
	heat.pack( src->heat);
	litDmd.pack( src->litDmd);
	litEu.pack( src->litEu);
}				// HresZoneDayPak::pack (also ResMoavZonePak::pack)
//---------------------------------------------------------------------------

#if 1 /* 12-3-94 redo to not use global Windows memory except to return to caller,
cuz WIN32 global memory is not swappable and had many out of memory problems,
	and combine DOS and WIN versions and reduce support for unused options. */
//===========================================================================
// member functions of class BinBlock: memory block associated with part of a binary file. for ResfBase members.
//===========================================================================
	void BinBlock::clear()
{
	if (!this)  return;
#ifdef WINorDLL
	discardable = keePak = 0;
	hPak = 0;	// init non-DOS members
#endif
	dirty = 0;
	offset = size = sizePak = 0;
	f = NULL;
	p = NULL;
	pNam = NULL;
	pakFcn = NULL;
}					// BinBlock::clear
//---------------------------------------------------------------------------
void BinBlock::init( BinFile *_f, DWORD _offset, WORD _size, WORD _sizePak,
					 BOO _discardable /*=FALSE*/, BOO _keePak /*=FALSE*/, PAKFCN * _pakFcn /*=NULL*/ )
{
	clear();

	f = _f;
	if (f)
		pNam = f->pNam;  	// handy redundant pointer so needn't keep doing "f ? f->pNam : NULL"
	offset = _offset;
	size = _size;
	sizePak = _sizePak;
#ifdef WINorDLL
	discardable = _discardable;
	keePak = _keePak;		// arg and member added 12-3-94
#else
	if (_discardable | _keePak)
		err( PWRN, (char *)MH_R1969);	// "BinBlock::init: 'discardable' and 'keePak' arguments should be 0 under DOS"
#endif
	pakFcn = _pakFcn;
}			// BinBlock::init
//---------------------------------------------------------------------------
BOO BinBlock::al()	 		// allocate, zero, dirty-flag block
{
	// for unpacked form: never ret'd to caller, so don't use Windows global memory (which is unswappable under WIN32s), 12-94
	// thus does not use discardable memory, but do we now (12-94) free in put().
	// (to support block return without packed form, add .h mbr and use GlobalAlloc here if keePak && !pakFcn. 12-94.)
	// MH_R1970 and MH_R1971 no longer used 12-94.

	if (dmal( DMPP( p), size, DMZERO))		// allocate. dmpak.c fcn. RC return value: non-0 bad.
		return FALSE;
	dirty = TRUE;			// say changed in memory (cuz created, and caller about to store into it)
	return TRUE;
}			// BinBlock::al
//---------------------------------------------------------------------------
BOO BinBlock::put() 		// write if dirty, & free
{
	if (!p)
		return FALSE;
	if (dirty)  		// if changed in memory
		if (!write(TRUE))	// write, free block if written successfully
			return FALSE;
	return TRUE;
}		// BinBlock::put
//---------------------------------------------------------------------------
BOO BinBlock::write(			// if using a file, pack, write, optionally free block.
	// no file is error under DOS, ok under Windows if keePak.

	BOO freeIt )	// non-0 to free unpacked form after writing to file (if any) 12-94.

// packed form is generated here and free'd after writing if not to be returned to caller.
// as of 12-94 all blocks are packed; there is no provision in this code to return unpacked block to caller.
{
	BOO ok = TRUE;
	if (p && size)		// ok nop if no ram block, or if 0 length (assume means unused block)
	{
		dirty = FALSE;				// clear 'dirty' first, to not try again (thru "put") on cleanup if errors out
		if (f->isOpen())  			// if (f) and file is open
			ok = f->seek(offset);			// seek to write position
#ifdef WINorDLL
		else if (!keePak)  			// normal to get here without file if returning Windows block handles
#else
		else
#endif
			return !err( PWRN, (char *)MH_R1977); // "BinBlock: Request to write block but file not open"

		// conditionally pack and conditionally write block
		if (pakFcn)
		{
			void *pPak = NULL;			// local pack buffer ptr: ptr never returned 12-94, only Windows handle
#ifdef WINorDLL
			if (keePak)				// if returning block handles to caller, use global memory
			{
				// (else don't use global memory cuz unswappable under WIN32s, 12-94)
				if (hPak) 				// if packed block just needs locking (believe case not used 12-94)
				{
					pPak = GlobalLock(hPak);
					if (!pPak)   hPak = 0;		// if lock fails, assume block has been discarded, fall thru to realloc
				}
				if (!hPak)  			// if needs allocating (normally does)
				{
					hPak = GlobalAlloc( 		// use Windows shareable global memory so server can pass handle to client
							   discardable
							   ? (GHND|GMEM_SHARE|GMEM_DISCARDABLE) 	// discardable...
							   : (GHND|GMEM_SHARE), 			// 0'd moveable sharable memory
							   sizePak );
					// generalize wording of following error messages if this general routine used for additional purposes.
					if (!hPak)
						return !err( WRN, (char *)MH_R1972);	// "Out of memory for binary results file packing buffer". ! returns FALSE.
					pPak = GlobalLock(hPak);
					if (!pPak)  				// ! returns FALSE for RCBAD in...
						return !err( WRN, (char *)MH_R1973);	// "Lock failure in BinBlock::write re binary results file"
				}
			}
			else					// when not returning block, use local memory: significant in 32 bit version.
#endif
				if (dmal( DMPP( pPak), size, DMZERO)) 	// allocate packing buffer for local use only
					return FALSE;			// allocation failure, message already issued
			pakFcn( p, pPak);			// pack into buffer
			if (f->isOpen())			// if writing to file (not just keePak'ing) (isOpen checks for !f)
				if (ok)				// if ok to here (no seek error)
					ok = f->write( pPak, sizePak);	// write packed form
#ifdef WINorDLL
			if (keePak)				// if packing buffer is in global memory for return to caller
				GlobalUnlock(hPak);		// unlock it
			else					// if not to be returned to caller, it is in locak dmpak memory
#endif
				dmfree( DMPP( pPak));			// free packing buffer, set pPak NULL. dmpak.cpp.
		}
		else					// no pakFcn: non-packed block (no cases currently use this, 12-94)
			if (ok)				// if no seek error
				ok = f->write( p, size);		// write the unpacked form

		// if requested, free (unpacked) block after successful write to file
		if (freeIt  &&  f->isOpen()  &&  ok)
			//if (keePak && !pakFcn)		to support returning unpacked blox 12-94; also change al, removeHan, free.
			dmfree( DMPP( p));				// free unpacked block, set p NULL.  packed block free'd just above if !keePak.
	}
	return ok;

	// no longer used 12-94: MH_R1974.
}					// BinBlock::write
//---------------------------------------------------------------------------
#ifdef WINorDLL
HGLOBAL BinBlock::removeHan() 		// return (packed) handle to caller and 0 it in BinBlock to prevent free'ing
{
	HGLOBAL temp = 0;
	if (pakFcn)  		// if block has packed form
	{
		temp = hPak;  		// return its handle
		hPak = 0; 		// remove from object to prevent free() or d'tor from freeing block -- caller must free.
	}
	else			/* when needed, implement memory return for binBlocks without packed form:
				   {  temp = h;  h = 0; };  also change al, write, free.  12-94. */
		err( PWRN, (char *)MH_R1978);	// "BinBlock: handle return for unpacked blocks not implemented"
	return temp;
}			// BinBlock::removeHan
#endif
//---------------------------------------------------------------------------
void BinBlock::free() 		// free block
{
#ifdef WINorDLL
	if (hPak)
		if (GlobalFree(hPak))  			// free packed block buffer in Windows global memory.
			err( WRN, (char *)MH_R1976);   	// "GlobalFree failure (packed block) in BinBlock::free"
	hPak = 0;
#endif
	//to support returning memory for unpacked blox:  (else MH_R1975 no longer used)
	// if (keePak && !pakFcn) { if (h) if (GlobalFree(hPak)) err( WRN, (char *)MH_R1975); } } else
	dmfree( DMPP(p));			// free unpacked block buffer. dmpak.c. nop if p is already NULL; sets p NULL.
}		// BinBlock::free
//---------------------------------------------------------------------------

#else	// old versions 12-3-94. "12-3-94" if's unrun, tho mostly trial-compiled.
x //===========================================================================
x // member functions of class BinBlock: memory block associated with part of a binary file. for ResfBase members.
x //===========================================================================
x #ifdef WINorDLL	// windows version. DOS version follows.
x //---------------------------------------------------------------------------
x void BinBlock::clear()
x{
x     if (!this)  return;
x
x     discardable = dirty = 0;
x     h = 0;
x     offset = size = sizePak = 0;
x #if 0
x x    f = NULL; p = pPak = NULL; pNam = NULL; pakFcn = NULL;
x #else//12/3-94
x     f = NULL; p = NULL; pNam = NULL; pakFcn = NULL;
x #endif
x}					// BinBlock::clear
x //---------------------------------------------------------------------------
x void BinBlock::init( BinFile *_f, DWORD _offset, WORD _size, WORD _sizePak,
x                      BOO _discardable, BOO _keePak, PAKFCN * _pakFcn )
x{
x     clear();
x
x     f = _f;
x     if (f)
x        pNam = f->pNam;  	// handy redundant pointer so needn't keep doing "f ? f->pNam : NULL"
x     offset = _offset;
x     size = _size;
x     sizePak = _sizePak;
x     discardable = _discardable;
x     keePak = _keePak;		// added 12-3-94
x     pakFcn = _pakFcn;
x}			// BinBlock::init
x //---------------------------------------------------------------------------
x BOO BinBlock::alLock()	 		// allocate new block
x 					// (buffer for unpacked form. packed form alloc'd in write())
x{
x     h = GlobalAlloc( discardable ? (GHND|GMEM_SHARE|GMEM_DISCARDABLE) 	// discardable...
x     	      			 : (GHND|GMEM_SHARE), 			// 0'd moveable sharable (server passes handle to client)
x 	             size );
x     // generalize wording of following error messages if this general routine used for additional purposes.
x     if (!h)
x        return !err( WRN, (char *)MH_R1970);	// "Out of memory for binary results file buffer".  ! returns FALSE for RCBAD.
x     p = GlobalLock(h);
x     if (!p)
x        return !err( WRN, (char *)MH_R1971);	// "Lock failure in BinBlock::alLock re binary results file".  ! returns FALSE.
x     dirty = TRUE;
x     return TRUE;
x}			// BinBlock::alLock
x //---------------------------------------------------------------------------
x #if 0
x x  BOO BinBlock::put(BOO keePak)		// write if dirty & unlock
x #else//12-3-94: keePak member
x   BOO BinBlock::put()			// write if dirty & unlock
x #endif
x{
x     if (!p)
x        return FALSE;
x     if (dirty)  			// if changed, write: appl uses discardable blocks when a file is in use.
x #if 0
x x       if (!write(keePak))		// pack, and write if a file is in use
x #else//12-3-94
x        if (!write())			// pack, and write if a file is in use
x #endif
x           return FALSE;
x     return unlock();			// unlock block, set p NULL. If no file in use, memory is not discardable.
x}			// BinBlock::put
x //---------------------------------------------------------------------------
x BOO BinBlock::lock()		// try to lock existing block (unpacked form)
x{
x     if (p)
x        return TRUE;
x     if (!h)  			// when a file is in use, control comes here on the way to reading block
x        return FALSE;		// so no errmsg here (see diskFetch)
x     p = GlobalLock(h);
x     // no error message if p==NULL cuz may have been discarded and caller may read from file
x     return (p != NULL);
x}			// BinBlock::lock
x //---------------------------------------------------------------------------
x BOO BinBlock::write(		// write block if using a file, else ok nop
x #if 0	//12-3-94: made member
x x    BOO keePak  		// non-0 to keep packed form (for memory return to caller)
x #endif
x     )
x{
x
x // pack
x #if 1 //12-3-94 dismember pPak; some of following logic could also be removed.
x     void *pPak = NULL;
x #endif
x     if ( pakFcn					// if has packing fcn, else no packed from
x      &&  p					// if has non-packed data to pack
x      &&  (keePak || (f && f->isOpen()) ) )	// if keep-packed-form flag OR if we will write file just below
x     {
x        if (!pPak)						// if buffer for packed form not locked
x        {	if (hPak) 						// if packed block just needs locking
x           {	pPak = GlobalLock(hPak);
x				if (!pPak) hPak = 0;	// if lock fails, assume block has been discarded.
x			}
x           if (!hPak)  							// if needs allocating
x           {
x              hPak = GlobalAlloc( discardable ? (GHND|GMEM_SHARE|GMEM_DISCARDABLE) 	// discardable...
x                                              : (GHND|GMEM_SHARE), 			// 0'd moveable sharable memory
x                                  sizePak );   						// .. (server passes handle to client)
x              // generalize wording of following error messages if this general routine used for additional purposes.
x              if (!hPak)
x                 return !err( WRN, (char *)MH_R1972);	// "Out of memory for binary results file packing buffer". ! returns FALSE.
x              pPak = GlobalLock(hPak);
x              if (!pPak)
x                 return !err( WRN, (char *)MH_R1973);	// "Lock failure in BinBlock::write re binary results file"
x 							// ! returns FALSE for RCBAD.
x		}	}
x        pakFcn( p, pPak);			// pack into buffer
x}
x
x // write
x     BOO ok = TRUE;
x     if (p && f && f->isOpen())		// write files don't auto-open, right?
x     {  dirty = FALSE;			// clear 'dirty' first: don't try again on cleanup if errors out
x        if (size)			// nop if 0 length: presumably unused block
x        {  if (!f->seek(offset))
x              return FALSE;
x           if (pakFcn)
x              ok = f->write( pPak, sizePak);	// write packed form
x           else
x              ok = f->write( p, size); 		// no pack function. Write from non-packed buffer.
x	}	}
x
x // discard packed form (if any) now to conserve memory except on flag
x     GlobalUnlock(hPak);
x     pPak = NULL;
x     if (!keePak)
x     {  if (GlobalFree(hPak))
x           err( WRN, (char *)MH_R1974);	// "GlobalFree failure in BinBlock::write"
x        hPak = 0;
x}
x
x     return ok;
x}			// BinBlock::write
x //---------------------------------------------------------------------------
x BOO BinBlock::unlock()			// unlock without write: used internally after write. both forms.
x{
x     if (p)     if (h) { GlobalUnlock(h); p = NULL; }
x #if 0//12-3-94
x x    if (pPak)  if (hPak) { GlobalUnlock(hPak); pPak = NULL; }
x #endif
x     return TRUE;
x}			// BinBlock::unlock
x //---------------------------------------------------------------------------
x HGLOBAL BinBlock::removeHan() 		// return (packed) handle to caller and 0 it in BinBlock to prevent free'ing
x{
x     HGLOBAL temp;
x     if (pakFcn)
x	  { temp = hPak;     // return handle of packed form if block has packed from
x		hPak = 0;
x	  }
x     else
x	  {  temp = h;     h = 0;    }	// else return handle of regular form
x     return temp;
x}		// BinBlock::removeHan
x //---------------------------------------------------------------------------
x void BinBlock::free() 		// free block: both packed and unpacked forms.
x 				// note: handles of memory to be returned to caller moved & 0'd by prior call to removeHan.
x {   unlock();
x     if (h)
x        if (GlobalFree(h))
x           err( WRN, (char *)MH_R1975);  	// "GlobalFree failure in BinBlock::free"
x     h = 0;
x     if (hPak)
x        if (GlobalFree(hPak))
x           err( WRN, (char *)MH_R1976);   	// "GlobalFree failure (packed block) in BinBlock::free"
x     hPak = 0;
x}			// BinBlock::free
x //---------------------------------------------------------------------------
x #endif	// WINorDLL
x
x #ifdef DOS	// DOS subset version of BinBlock class
x //---------------------------------------------------------------------------
x void BinBlock::clear()		// DOS subset
x{
x     dirty = 0;
x     offset = size = sizePak = 0;
x #if 0
x x    f = NULL; p = pPak = NULL; pNam = NULL; pakFcn = NULL;
x #else//12-3-94
x     f = NULL; p = NULL; pNam = NULL; pakFcn = NULL;
x #endif
x}					// BinBlock::clear
x //---------------------------------------------------------------------------
x void BinBlock::init( 							// DOS subset
x     BinFile *_f, DWORD _offset, WORD _size, WORD _sizePak,
x     BOO _discardable /*=FALSE*/, BOO _keePak /*=FALSE*/, PAKFCN * _pakFcn /*=NULL*/ )
x{
x     clear();
x
x     f = _f;
x     if (f)
x        pNam = f->pNam;  	// handy redundant pointer so needn't keep doing "f ? f->pNam : NULL"
x     offset = _offset;
x     size = _size;
x     sizePak = _sizePak;
x     pakFcn = _pakFcn;
x}			// BinBlock::init
x //---------------------------------------------------------------------------
x BOO BinBlock::alLock()	 		// allocate new block, DOS version
x{
x     if (dmal( p, size, DMZERO))		// allocate. dmpak.c fcn. RC return value: non-0 bad.
x        return FALSE;
x     dirty = TRUE;
x     return TRUE;
x}			// BinBlock::alLock
x //---------------------------------------------------------------------------
x #if 0
x x BOO BinBlock::put(BOO keePak) 		// write if dirty & free, DOS version.
x #else//12-3-94: keePak member
x  BOO BinBlock::put() 			// write if dirty & free, DOS version.
x #endif
x{
x     if (!p)
x        return FALSE;
x     if (dirty)  		// if changed, write
x #if 0
x x       if (!write(keePak))
x #else//12-3-94
x        if (!write())
x #endif
x           return FALSE;
x     free();			// no handles, replace "unlock" with "free" for DOS
x     return TRUE;
x}		// BinBlock::put
x //---------------------------------------------------------------------------
x #if 0
x x BOO BinBlock::write(BOO keePak)		// write block, DOS version: error if no file or not open
x #else//12-3-94: keePak member
x  BOO BinBlock::write()			// write block, DOS version: error if no file or not open
x #endif
x{
x     BOO ok = TRUE;
x     if (p && size)		// ok nop if no ram block, or if 0 length (assume means unused block)
x     {
x #if 1 //12-3-94 dismember pPak
x        void *pPak = NULL;
x #endif
x        dirty = FALSE;		// clear 'dirty' first, to not try again (thru "put") on cleanup if errors out
x        if (!f || !f->isOpen())
x           return !err( PWRN, (char *)MH_R1977); // "BinBlock: Request to write block but file not open"
x        if (!f->seek(offset))
x           return FALSE;
x        if (pakFcn)
x        { if (dmal( pPak, size, DMZERO)) 	// allocate packing buffer
x              return FALSE;
x           pakFcn( p, pPak);			// pack into buffer
x           ok = f->write( pPak, sizePak);	// write packed form
x           dmfree(pPak);				// free packing buffer (under DOS)
x		}
x        else					// no pakFcn: non-packed block
x           ok = f->write( p, size);
x	}
x     return ok;
x }			// BinBlock::write
x //---------------------------------------------------------------------------
x void BinBlock::free() 		// free block, DOS version
x{
x     dmfree(p);		// dmpak.c. nop if p is already NULL; sets p NULL.
x #if 0//12-3-94
x x    dmfree(pPak);
x #endif
x}			// BinBlock::free
x //---------------------------------------------------------------------------
x #endif	// DOS
#endif

//===========================================================================
// member functions of class BinFile: binary file i/o class used for members of ResfBase.
//===========================================================================
void BinFile::clear()
{
	if (!this)  return;		// NOP if called as member of NULL pointer

	*pNam = 0;
	fh = -1;
	outFile = FALSE;
	maxSeek = 0L;
}		// BinFile::clear
//---------------------------------------------------------------------------
void BinFile::init( const char* pNam, const char *dotExt) 		// pNam may have extension added in place
{
	clear();
	if (pNam)
	{	strcpy( this->pNam, pNam);
		if (dotExt)
			strcpy( (char *)pointExt( this->pNam), dotExt); 	// ... overwrite any extension, else append
	}
}			// BinFile::init
//---------------------------------------------------------------------------
BOO BinFile::create()
{
	outFile = TRUE;
#if 1
	fh = ::open( pNam, 				// open file. C library function.
				 O_CREAT|O_TRUNC|O_BINARY|O_RDWR,	// create file, delete any existing contents, binary, read/write access
				 S_IREAD|S_IWRITE );			// read/write permission
#else
x    fh = _rtl_creat( pNam, FA_NORMAL);
#endif
	if (fh < 0)
		return !err( WRN, (char *)MH_R1980, pNam);	// "Create failure, file \"%s\""
	return TRUE;
}  			// BinFile::create
//---------------------------------------------------------------------------
BOO BinFile::close()
{
	if (fh < 0)
		return TRUE;			// nop if not (sucessfully) opened
	INT ret = ::close(fh);
	fh = -1;				// clear open indication even on error so doesn't loop on error in desructor
	if (ret < 0)
		return !err( WRN, (char *)MH_R1981, pNam);	// "Close failure, file \"%s\""
	return TRUE;
}  			// BinFile::close
//---------------------------------------------------------------------------
BOO BinFile::seek( DWORD offset)
{
	if (_lseek( fh, offset, SEEK_SET) < 0L)
		return !err( WRN, (char *)MH_R1982, pNam);	// "Seek failure, file \"%s\""
	if (offset > DWORD( maxSeek))
		maxSeek = offset;
	return TRUE;
}  			// BinFile::seek
//---------------------------------------------------------------------------
BOO BinFile::write( void *buf, WORD size)
{
	INT nWrit = ::write( fh, buf, size);
	if (nWrit==-1 || (WORD)nWrit < size)
		return !err( WRN, (char *)MH_R1983, pNam);  	// "Write error on file \"%s\" -- disk full?"
	return TRUE;
}  			// BinFile::write
//---------------------------------------------------------------------------

//===========================================================================
//  packed floating point vector support
//===========================================================================
static void NEAR pack( float *data, float *pf, char *v, SI n);
//---------------------------------------------------------------------------
void PACK12::pack( float *data12)
{
	::pack( data12, &scale, v, 12);
}
//---------------------------------------------------------------------------
#ifdef needed //8-18-94 no uses
0  void PACK16::pack( float *data16)
{
	::pack( data16, &scale, v, 16);
}
#endif
//---------------------------------------------------------------------------
void PACK24::pack( float *data24)
{
	::pack( data24, &scale, v, 24);
}
//---------------------------------------------------------------------------
static void NEAR pack( float *data, float *pf, char *v, SI n)
{
	if (badSrcOrD( data, v, "pack"))   return;		// message NULL pointers, avoid GP fault

// determine scale factor to map data into range -511..511 (10 bits)
	float f = 0.f;
	for (SI i = 0;  i < n;  i++)
		setToMax( f, (float)fabs(data[i]) );
	f /= 511.;
	if (pf)  *pf = f;

// compress n values
	if (v)
		for (i = 0;  i < n;  i++)
		{
			float sdat = f==0.  ?  0.f  :  data[i]/f;	// scale to range +- 511, or use 0 if all data 0
			SI idat = SI(sdat+512.5f) - 512;		// round to integer: round as a
			// positive value so independent of how neg #'s truncate.
			setToMax( idat, -512);
			setToMin( idat, 511);	// insurance
			idat &= 0x03ff;				// clear hi sign bits before merging
			SI ii = (5*i)/4;   				// 0,1,2,3,5,... offset of start of word containing the 10 bits
			SI shift = 6-2*(i%4);				// 6,4,2,0,6,... bits to left shift datum when storing
			char *p = v+ii;							// point word. manipulate as bytes to not swap.
			*p = ((WORD)*p & ~(0x03ff >> (8-shift))) | ((WORD)idat >> (8-shift));	// store hi 2 to 8 bytes
			p++;									// point next byte
			*p = (*p & ~(0x03ff << shift)) | (idat << shift);			// store lo 2 to 8 bytes
		}
}		// pack
//---------------------------------------------------------------------------

//===========================================================================
//  non-member functions
//===========================================================================
static void NEAR memcpyCheck( void *dest, void *src, size_t size)
{
	if (badSrcOrD( src, dest, "memcpyCheck"))  return;
	memcpy( dest, src, size);
}				// memcpyCheck
//---------------------------------------------------------------------------
static BOO NEAR badSrcOrD( void *src, void *d, const char *fcnName)
{
	if (!src)  err( PWRN, (char *)MH_R1988, fcnName);	// "In brfw.cpp::%s: NULL source pointer "
	if (!d)    err( PWRN, (char *)MH_R1989, fcnName);	// "In brfw.cpp::%s: NULL destination pointer"
	return !src || !d;
}			// badSrcOrD
//---------------------------------------------------------------------------
const char * FC pointExt( const char *pNam) 	// return pointer to . before extension, else to terminating null of string
{
	if (!pNam)
		return NULL;
	const char *ext1 = strrchr( pNam, '\\');
	ext1 = ext1 ? ext1+1 : pNam;		// pointer after last \ or to start
	const char *ext2 = strchr( ext1, '.');
	return ext2 ? ext2 : ext1 + strlen(ext1);	// pointer to period or to end
}					// pointExt
//---------------------------------------------------------------------------
const char * FC pointNamExt( const char *pNam)	// return pointer to primary filename in pathname
{
	if (!pNam)
		return NULL;
	const char *afColon = strchr( pNam, ':');
	afColon = afColon ? afColon+1 : pNam;	// pointer after first : or to start
	const char *afBs = strrchr( afColon, '\\');
	return afBs ? afBs + 1 : afColon;		// return pointer after last \ or after first colon
}					// pointNamExt
//---------------------------------------------------------------------------

// end of brfw.cpp
