// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// vrpak.h: header for virtual reports (vrpak,2,3.c)

	/* vrpak generates an indefinite number of report files using
	   spooling method to limit # file handles in use simultaneously */

#ifndef VRPAK_H
#define VRPAK_H

//------------------------ DEFINES used by callers --------------------------

//----- "optn" bits
//
// for virtual reports (optn arguments, VRI.optn) and output files (vrUnspool optn, UNS.optn).
// VR_FMT and VR_OVERWRITE are used within vrpak; others may be passed thru for user's info.
// EROPn bits are defined in cnglob.h.
//
const int VR_FMT=EROP6;	// PAGE FORMATTING ON: enables page breaks, headers, footers, continuation table headers.
			// VR_FMT is associated with individual reports (during spooling) and also with output files
			//   (during unspooling).  If spec'd for unspooling (in vrUnspool/Ind output file optns) but not
			//   when the report was spooled (vrOpen optns arg), page headers and footers will be output but
			//   formatting from spooling calling code, such as smart page breaks and repetition of table column
			//   heads on contin pages, will not occur.  If specified at spool time and not at unspool,
			//   spool time formatting is stored then removed at unspool, wasting a little time.
const int VR_OVERWRITE=EROP5;	// in vrUnspool/Ind output file optn member: overwrite any existing output file; if off, append.
const int VR_ISEXPORT =EROP4;	// may be given to indicate an export, not a report, for possible future testing by callers.
const int VR_NEEDHEAD =EROP3;	// report needs heading & deferred due to rpCond (internal to cgresult.c:vpRxports)
const int VR_NEEDFOOT =EROP2;	// report needs footing & deferred due to rpCond (internal to cgresult.c:vpRxports)
const int VR_FINEWL   =EROP1;	// append blank line during unspool if report contains any text, 1-6-92.

//-------------------------------- TYPES ------------------------------------

//----- VROUTINFO: information about an output file and virtual reports to put into it.
//                 Arg to vrUnspool is a NULL-terminated CONCATENATION of these (not an array: they are variable size).
//
//		   CAUTION: do not attempt to use VROUTINFO twice as vrUnspool frees the filename string.
//
// template for forming variable argument block to vrUnspool
struct VROUTINFO
{   const char* fName;	// pointer to file name. must be private (strsave'd) copy in dm: dmfree'd after use in vrUnspool.
    int optn;		// option bits (above) including VR_OVERWRITE and VR_FMT (VR_FMT should also be given in vrOpens)
    int vrhs[1];	// 0-terminated variable length array of virtual report handles, to unspool to this file in order.
};		// followed by another VROUTINFO or NULL; thus whole arg ends with 0, NULL.
//
// statically-usable vrUnspool argument block to unspool up to 5 vrs to one output file
struct VROUTINFO5
{   VROUTINFO f;		// above
    int moreVrhs[ 5];	// 4 more vrh's and a 0 to terminate vrhs[].
    char* aNULL;		// NULL to say no more VROUTINFOs follow
};


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
void FC vrpakClean(CLEANCASE cs);
RC FC vrInit( const char *splFName);
RC FC vrClear();
void FC vrTerminate();
RC FC vrOpen( int* pVrh, const char* vrName, int optn);
RC FC vrClose( int vrh);
int FC vrGetOptn( int vrh);
RC FC vrChangeOptn( int vrh, int mask, int optn);
RC FC vrIsFormatted( int vrh);
int FC vrIsEmpty( int vrh);
RC FC vrSetPage( int vrh, int pageN);
int FC vrGetPage( int vrh);
int FC vrGetLine( int vrh);
RC CDEC vrPrintf( int vrh, const char *s, ...);
RC CDEC vrPrintfF( int vrh, int isFmt, const char *s, ...);
RC FC vrVPrintfF( int vrh, int isFmt, const char *s, va_list ap );
RC FC vrStr( int vrh, const char *s);
RC FC vrStrF( int vrh, int isFmt, const char *s);

void FC vrInfoClean( VROUTINFO *voInfo);
RC FC vrUnspool( VROUTINFO *voInfo);
BOO FC isUsedVrFile( const char* fName);

#endif

// end of vrpak.h
