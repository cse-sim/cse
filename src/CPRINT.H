// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* CPRINT.H: printer defines used externally, for cprint.c
             CSE version derived from prpak.h 8-28-91 */


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
/* cprint.c */
 RC FC prOpen( int erOp, char *pathname);
 RC FC prClose( void);
 RC FC prSetPage( SI);
 SI FC prGetPage( void);
 SI FC prGetLine( void);
 RC FC prNewl( void);
 RC FC prC( char);
 RC FC prStrN( char *, SI);
#if 0	// changing last call (cpgprput.c) to prStrN 11-7-91
x RC FC prStr( char *);
#endif
#if 0	// eliminate -- expand any uses with prC( 0x0c or 0x0d); rob 11-91
x RC FC prFf( void);
x RC FC prCr( void);
#endif
#if 0	// no ext uses rob 11-91
x RC FC sendN( char *, SI);
#endif

// end of cprint.h
