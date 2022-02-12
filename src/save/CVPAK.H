// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cvpak.h -- conversion routines include file


/*-------------------------------- DEFINES --------------------------------*/

/*------- Format field assignments; used by output conversion routines.
	  Format is currently a 16 bit field.  Some assumptions about these
	  values are hard coded in cvpak.  Change with care. ---------*/

#define FMTDFWMASK 0x000F	/* Mask for "dfw" for floats in low order bits:
				   # of places after point, or max sig digits under FMTRTZ for values >= .001.
				   dfw will be reduced if necessary to fit value in field width,
				   and may be increased to show that many SIGNIFICANT digits for values < .1. */
#define FMTDFW(fmt) ((fmt) & FMTDFWMASK)

#if 0	// p plus value feature group coded out as FMTPVPLUS 11-91 rob.
p        // define FMTPVMASK to restore code in cvpak.cpp, 11-91
p #define FMTPVMASK 0x0030     // Positive value display
p #define FMTPVNULL 0x0000        // Pos prefix = ""
p #define FMTPVPLUS 0x0010        // Pos prefix = "+"
p #define FMTPVSPACE 0x0020       // Pos prefix = " "
p #define FMTPVSHIFT 4            // Right shift count to get small integer
p /* FMTPVxxx FEATURES, UNUSED 9-89:
p    * The above positive value options are NOT USED (grep 10-88, 9-89, rob).
p    * Eliminating fully would remove LOTS of cvpak.cpp code and tables.
p    * However features should remain available or restorable to keep library
p      general for future applications (Chip, 9-89).
p    * Removing with conditional compilation and maintaining would be difficult
p      as code and tables many places is affected.  So, 9-89, kept.
p    * 11-91 CSE: removed despite the previously-perceived difficulty */
#endif

#define FMTJMASK 0x00C0      // Justification
#define FMTLJ 0x0000              // Left justified
#define FMTRJ 0x0040              // Right justified
#define FMTLZ 0x0080              // Rt just w/ leading 0's. NOT USED 10-88
#define FMTSQ 0x00C0              // Squeeze: trim to shortest poss length.  Much used in print stuff, text files.*/
#define FMTJSHIFT 6               // Right shift count to get small integer

#define FMTOVFMASK 0x0100    // Float overflow format -- K or E
#define FMTOVFK    0x0000            // K format
#define FMTOVFE    0x0100            // E format

#define FMTUNITS   0x0200     // Add units to string -- space and units tag
#define FMTPU      0x2000     // Add units in parens, no spaces (for crb res loads; can remove if format changed. 2-90)
		// 0x0400  unused in CSE (formerly FMTNODATA)
#define FMTRTZ     0x0800     /* Trim trailing 0's from FLOAT values.  Makes DFW be total sig digits, not dig after point.
				 Used in wcpak.cpp and text data files.

				 Now (b4 10-88!) kludged w/ g format; Better soon.
				 ??? 10-88: what is wrong with g format method???
				 Internal fcn cvttz() added for inches may be pertinent. rob.
				 >> 9-89 A Problem with G FORMAT is that it goes into e format too easily
				 (when there wd be a trailing 0 left of pt) -- wont' load.

				 ALSO 10-88 trims inches of length values when used with FMTSQ. */

#if 0	// define to restore code in cvpak.cpp, 11-91
x #define FMTNOQUINCH 0x1000      // suppress " on inches of feet-inches (for text data files) rob 10-88
x // FTMNOQUINCH FMTRTZ and FMTSQ all on --> omit 0 inch values with feet.
#endif
// 0x2000 used above for FMTPU

/*--------------------------- PUBLIC VARIABLES ----------------------------*/

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
// cvpak.cpp functions
char* FC cvin2sBuf( char *buf, void *data, USI dt, SI units, USI mfw, USI fmt);
char* FC cvin2s( const void *data, USI dt, SI units, USI mfw, USI fmt, USI xfw=0);
RC FC cvLmCk( SI dt, SI limit, void *p);
double FC cvExtoIn( double f, int units);
double FC cvIntoEx( double f, int units);
double FC cvstdangle( double ang );
enum CHOITXTY { chtyNORMAL, chtyHIDDEN, chtyALIAS, chtyALIASDEP };
int getChoiTxTyX( const char* chtx);
const char* getChoiTxI( USI dt, SI chan, int* pTyX=NULL, int options=0);
RC FC cvS2Choi( const char *s, USI dt, void *pv, USI *pSz, const char** pms);

// cvatoxxx.cpp functions
RC FC cvatol( char *s, LI *pn, SI hexoct); 		// convert string to long integer
RC FC cvatof( char *str, double *vp, SI percent );
RC FC cvatof2( char *s1, char *s2, double *vp, SI percent );

// end of cvpak.h
