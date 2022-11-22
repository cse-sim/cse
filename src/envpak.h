// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// envpak.h: defs and decls for envpak.cpp

/*-------------------------------- DEFINES --------------------------------*/

// Constants for enkimode() argument
#define KISILENT 0	    // Do not beep when interrupt is detected
#define KIBEEP 1	    // Issue a beep when interrupt is detected
#define KICLEAR 0	    // Unconditionally clear interrupt flag
#define KILEAVEHIT 2	    // Clear interrupt flag unless an unprocessed ^C is indicated.

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
WStr enExePath();			// path to current .exe
WStr enExeInfo( WStr exePath, int& codeSize);	// timestamp (from header) of .exe
void FC ensystd( IDATETIME*);	// Return system date and time
LDATETIME FC ensysldt();	// Return system date/time as LDATETIME
void FC enkiinit(SI);		// Initialize keyboard interrupt handler
void FC enkimode(SI);		// Set keyboard interrupt mode
int enkichk();				// Check if keyboard interrupt recvd

UINT doControlFP();			// (maybe) unmask FP exceptions

void FC hello();					// entry fcn
void FC byebye( int exitCode);  	// exit fcn

void FC cknull();						// check for memory clobbers
/* also public but called ONLY from msc lib or interrupt
 * SI   matherr( struct exception *);	* private math runtime error fcn *
 * void fpeErr( SI, SI);		* intercepts floating point errors *
 * void iDiv0Err( SI);			* intercepts int x/0 errors *
*/

// end of envpak.h
