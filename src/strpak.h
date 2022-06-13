// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// strpak.h -- extended string functions

#if !defined( _STRPAK_H)
#define _STRPAK_H

// definitions for strpathparts()
const int STRPPDRIVE = 1;
const int STRPPDIR   = 2;
const int STRPPFNAME = 4;
const int STRPPEXT   = 8;

///////////////////////////////////////////////////////////////////////////
// public fcns
///////////////////////////////////////////////////////////////////////////
// char type wrappers re VS2008 and later
//  WHY: VS2008 char type fcns assert if !(0 <= c <= 0xff).
//       The following causes assert
//       const char* s;
//       int c = *s;		// c is < 0 if *s is 128 - 255
//       if (isspace( c))	// can assert
//  isxxxxxW are provided with explicit arg types
#include <ctype.h>
#define charTypeW( fcn) \
inline int fcn##W( int c) { return fcn( c); } \
inline int fcn##W( char c) { return fcn( (unsigned char)c); } \
inline int fcn##W( unsigned char c) { return fcn( (unsigned char)c); }
// charTypeErr is used to redefine the original functions so that when they are used,
// the compiler will throw an error because "Should_use_##fcn##W" is not defined.
// This indicates to the user that they should use the "W" suffixed version of the function
// instead for proper casting.
#define charTypeErr( fcn) Should_use_##fcn##W
charTypeW( isspace)
#define isspace charTypeErr( isspace)
charTypeW( iscntrl)
#define iscntrl charTypeErr( iscntrl)
charTypeW( isalpha)
#define isalpha charTypeErr( isalpha)
charTypeW( isdigit)
#define isdigit charTypeErr( isdigit)
charTypeW( isalnum)
#define isalnum charTypeErr( isalnum)
charTypeW( ispunct)
#define ispunct charTypeErr( ispunct)
charTypeW( isprint)
#define isprint charTypeErr( isprint)
charTypeW( isupper)
#define isupper charTypeErr( isupper)
charTypeW( islower)
#define islower charTypeErr( islower)
inline int intC( const char* p) { return int( (unsigned char)*p); }


/*------------------------- FUNCTION DECLARATIONS -------------------------*/
int strCheckPtr( DMP p);
char * strxtok( char *tokbuf, const char* &p, const char* delims, int wsflag);
int strTokSplit( char* str, const char* delims, const char* toks[], int dimToks);
void * FC memsetPass( void * * pdest, char c, USI n);
void * FC memcpyPass( void * * pdest, const void * src, USI n);
char * FC strncpy0( char *d, const char *s, size_t l);
inline char* strTrimB( char* s)
{ while (isspaceW( *s)) s++; return s; }
inline const char* strTrimB( const char* s)
{	while (isspaceW( *s)) s++; return s; }
char* strTrimB( char* d, const char* s);
char* strTrimE( char* d, const char* s);
char* strTrimEX( char* d, const char* s, const char* ex);
char* strTrimE( char* s);
char* strTrim( char* s1, const char* s2, int s2Len=999999);
inline char* strTrim( char* s) { return strTrim( s, s); }	// trim in place
int strLineLen( const char *s);
int strJoinLen( const char *s1, const char *s2);
char * FC strpad( char *s, const char *pads, SI n);
char* strSpacePad( char* d, size_t n, const char* s=NULL);
char * FC strffix( const char *name, const char *ext);
char* strffix2( const char* name, const char* ext, int options=0);
char * FC strtPathCat( const char *path, const char *namExt);
char * FC strpathparts( const char *path, int partswanted, char* pcombo=NULL);
char * FC strtBsDouble( char *s);
char * FC strBsUnDouble( char *s);
char * FC strsave( const char *s);
char * strsave( char* &p, const char *s);
char * FC strtemp( size_t n);
char * FC strtempPop( char *anS);
char * FC strtmp( const char *s);
char * CDEC strtcat( const char *s, ... );
char * CDEC strntcat( SI n, ...);
const char* scWrapIf( const char* s1, const char* s2, const char* tween);
const char* strtprintf( const char *format, ...);
const char* strtvprintf( const char *format, va_list ap=NULL);
SI     FC strlstin( const char *list, const char *str);
const char* strOrdinal( int number);

#if 0	// out of service
x const int STRTRIGHT = 0;  // truncate right end of string  (keep first char)
x const int STRTLEFT = 1;	  // truncate left end of string   (keep last char)
x char * FC strntrim( char *s1, char *s2, SI n);
x char * FC strtrend( char *s1, char *s2);
x char * FC strtrunc( char *str, SI maxlen, SI mode);
x char * FC strstrch( char *s1, char *s2, char *s3);
x char * FC strpadsp( char *dest, char *source, SI n);
x char * FC strindx( char *s1, char *s2);
x char tolowerx( char *p );
#endif

// WStr variants
WStr& WStrUpper( WStr& s);
WStr& WStrLower( WStr& s);
WStr WStrPrintf( const char* mOrH, ...);
WStr WStrVprintf( const char* mOrH, va_list ap=NULL);
WStr WStrFmtFloatDTZ( double v, int maxPrec, int minPrec=0);
WStr WStrDTZ( const WStr& s, int minPrec=0);

int strDecode( const char* s, const char* fmt, void *p, int erOp=IGN,
	const char* what=NULL, const char** pMsg=NULL);
inline int strDecode( const char* s, double &v, int erOp=IGN,
	const char* what=NULL, const char** pMsg=NULL)
{ 	return strDecode( s, "%lg%1s", &v, erOp, what, pMsg); }
inline int strDecode( const char* s, float &v, int erOp=IGN,
	const char* what=NULL, const char** pMsg=NULL)
{ 	return strDecode( s, "%g%1s", &v, erOp, what, pMsg); }
inline int strDecode( const char* s, int &v, int erOp=IGN,
	const char* what=NULL, const char** pMsg=NULL)
{ 	return strDecode( s, "%d%1s", &v, erOp, what, pMsg); }
inline int strDecode( const char* s, DWORD &v, int erOp=IGN,
	const char* what=NULL, const char** pMsg=NULL)
{ 	return strDecode( s, "%u%1s", &v, erOp, what, pMsg); }
inline BOOL IsNumber( const char* s)
{	double v; return strDecode( s, v) == 1; }
inline BOOL IsInteger( const char* s)
{	int v; return strDecode( s, v) == 1; }
BOOL IsBlank( const char* s, int len=999999);
char* strFill( char* d, const char* s, int len=-1);
const char* strSuffix(int n, int options = 0);
char* strTranslateEscapes( char* d, const char* s=NULL);
char* strCatIf( char* d, size_t dSize, const char* brk, const char* s, int options=0);
char* strDeBar( char* d, const char* s=NULL);
char* strDeQuote( char* d, const char* s=NULL);
char* strDeWS( char* d, const char* s=NULL);
char* strCase( char* s, const char toCases[3]);
char* strCase( char* d, const char* s, const char toCases[3]);
char* strPluralize( char* d, const char* word, bool bPlural=true);
char* strRemoveCRLF(char* str);
int strReplace2( char* s, char cFrom, char sTo, int options=0);
int strReplace( char* str, const char* sOld, const char* sNew,
	BOOL bCaseSens=FALSE);
char* stristr( const char* str1, const char* str2);
BOOL strMatch( const char* s1, const char* s2);
#ifndef _WIN32
inline int _stricmp(const char* char1, const char* char2);
inline int _strnicmp(const char* char1, const char* char2, size_t count);
char* _strupr(char* stringMod);
char* _strlwr(char* stringMod);
int memicmp(const void* str1, const void* str2, unsigned int count);
#endif


#endif	// _STRPAK_H

// end of strpak.h
