// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// rundata.cpp  runtime (as opposed to input time) data objects for CSE


//-------------------------------- INCLUDES ---------------------------------
#include <cnglob.h>

#include <ancrec.h>	// record: base class for rccn.h classes
#include <rccn.h>	// WSHADRATstr ZNRstr

#include <vrpak.h>	// VROUTINFO5

#include <cnguts.h>	// decls for this file

/*------------------ HOURLY SIMULATOR GLOBAL VARIABLES -------------------*/


// ---- Input Globals: set externally in cse.cpp

const char* cmdLineArgs = NULL;			// command line arguments (argv[ 1 ..]) (in dm)
										//   suitable for display (wrapped if long)
const char* exePath = NULL;				// path(s) to .exe file or dll;exe in dm
const char* InputFileName = NULL;		// input file name as entered by user: no added path nor defaulted ext.
										//   = pts into cne3
const char* InputFilePath = NULL;		// input file full path. ptr into cse.cpp:cne3() stack.
const char* InputFilePathNoExt = NULL;	// input file full pathName with any .ext removed, in dm.
const char* InputDirPath = NULL;		// drive/dir path to input file, in dm.
VROUTINFO5 PriRep = { { 0 } };	// information about primary output file, for appending final end-session info in cse.cpp
								// (after report file input records have been deleted).
								// out file members set from cncult (at input) and vrpak (at close)
								// has room for 5 vrh's (set where used, in cse.cpp).

// run serial number in lieu of future status file, 7-92.
SI cnRunSerial = 0;		// incremented in cgInit; copied to Topi.runSerial in cncult2\TopStarPrf2.

// virtual report unspooling specifications for this run
VROUTINFO* UnspoolInfo = NULL;	// dm block of info re unspooling virtual reports into actual report files.
								// set up in cncult.cpp; passed to vrUnspool then dmfree'd in cse.cpp;
								// vrpak.h struct.  Note: vrUnspool dmfree's .fNames in info as it closes files.

///////////////////////////////////////////////////////////////////////////////
// utility functions
///////////////////////////////////////////////////////////////////////////////


// end of rundata.cpp
