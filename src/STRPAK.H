// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
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
//  isxxxxxW (Wrightsoft variants) are provided with explicit arg types
#include <ctype.h>
#define charTypeW( fcn) \
inline int fcn##W( int c) { return fcn( c); } \
inline int fcn##W( char c) { return fcn( (unsigned char)c); } \
inline int fcn##W( unsigned char c) { return fcn( (unsigned char)c); }
// cause error in VS2008 (won't compile in VS6 due to Dundas grid STL troubles, 5-31-06)
#if _MSC_VER >= 1400
#define charTypeErr( fcn) Should_use_##fcn##W
#else
#define charTypeErr( fcn) fcn
#endif
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
char * strxtok( char *tokbuf, const char** p, const char* delims, int wsflag);
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
char * FC strtemp( SI n);
char * FC strtempPop( char *anS);
char * FC strtmp( const char *s);
char * CDEC strtcat( const char *s, ... );
char * CDEC strntcat( SI n, ...);
const char* scWrapIf( const char* s1, const char* s2, const char* tween);
const char* strtprintf( const char *format, ...);
const char* strtvprintf( const char *format, va_list ap=NULL);
SI     FC strlstin( const char *list, const char *str);

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
WStr WStrPrintf( const char* format, ...);
WStr WStrVprintf( const char* format, va_list ap=NULL);

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
char* strTranslateEscapes( char* d, const char* s=NULL);
char* strCatIf( char* d, size_t dSize, const char* brk, const char* s);
char* strDeBar( char* d, const char* s=NULL);
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


#if 0
/////////////////////////////////////////////////////////////////////////////
// class CWString -- Wrightsoft extended CString
/////////////////////////////////////////////////////////////////////////////
class CWString : public CString
{
public:
// constructors (same as CString)
	CWString()							 : CString() {}
	CWString( const CString& stringSrc)	 : CString(stringSrc) {}
	CWString( TCHAR ch, int nRepeat = 1) : CString( ch, nRepeat) {}
	CWString( LPCSTR lpsz)			 	 : CString( lpsz) {}
	CWString( LPCWSTR lpsz)				 : CString( lpsz) {}
	CWString( LPCTSTR lpch, int nLength) : CString( lpch, nLength) {}
	CWString( const unsigned char* psz)	 : CString( psz) {}

    ~CWString() {}

	CWString Set( const char* s, int len=INT_MAX);
	CWString Fill( const char* s, int len=-1);
	CWString Format( LPCTSTR lpszFormat, ...);
	CWString Format( UINT nFormatID, ...);
	CWString FormatV( LPCTSTR lpszFormat, va_list arglist);
	CWString FmtIfV( LPCTSTR fmtNonBlank, LPCTSTR fmtBlank, va_list argList);
	CWString FmtIf( LPCTSTR fmtNonBlank, LPCTSTR fmtBlank, ...);
	CWString FmtForWhat( const char* what, const char* fmt, ...);
	CWString FmtFloatDTZ( double v, int maxPrec, int minPrec=0);
	CWString DropTrailing0s( int minPrec=0);
	CWString CSV( const char* s);
	CWString CSV( double v, int maxPres, int minPrec=0);
	CWString CSV( float v, int maxPres, int minPrec=0);
	CWString CSV( int v);
	CWString PathParts( int parts) const;
	int CWString::SplitPfx( CWString& sPfx, CWString& sMain, const char* delim="|") const;
	BOOL Piece( CWString &sRet, int iB, const char* delim="|", int iE=-1,
		int* pNPRet=NULL) const;
	inline CWString Piece(	int iB, const char* delim="|", int iE= -1) const
	{	CWString t;	Piece( t, iB, delim, iE); return t; }
	int PieceCount(	const char* delim="|", int *pNNonBlank=NULL) const;
	static int PieceFindHelper( const char* s, const char* sFind,
		const char* delim="|", int brk=-1, CWString* pFound=NULL);
	inline int PieceFind( const char* s, const char* delim="|", int brk=-1,
		CWString* pFound=NULL) const
	{	return PieceFindHelper( s, *this, delim, brk, pFound); }
	inline int PieceFind2( const char* sFind, const char* delim="|", int brk=-1,
		CWString* pFound=NULL) const
	{	return PieceFindHelper( *this, sFind, delim, brk, pFound); }
	CWString PieceDelimPad( int nPReq, int nPHave=-1, const char* delim="|");
	CWString PieceSet( const char* s, int iB, const char* delim="|", int iE=-1);
	CWString PieceDel( int iB, const char* delim="|", int iE=-1);
	CWString PieceAdd( const char* s, int iB=-3, const char* delim="|");
	BOOL PieceAddUniq( const char* s, int iB=-3, const char* delim="|");
	CWString PieceStringFromNullSeparated( const char* s, int len=-1,
		const char* delim="|");
	CWString PieceFormatList( int* pCount=NULL, const char* last="and",
		const char* delim="|") const;
	CWString PieceTrim( const char* delim="|") const;
	CWString& Trim();
	CWString& CatIf( const char* s, const char* brk=" ");
	BOOL IsBlank() const {  return ::IsBlank( *this); }
	CWString& MakeLower() { CString::MakeLower(); return *this; }
	CWString& MakeUpper() { CString::MakeUpper(); return *this; }
	CWString DoubleDelim( const char* delim);
	CWString TranslateEscapes();
	CWString DeBar();
	CWString DeWS();
	CWString Case( const char toCases[ 3]);
	int ReplaceNoCase( const char* sOld, const char* sNew);
	int FindNoCase( const char* str) const;
	CWString& Replace2( char cFrom, char cTo, int options=0);
	CWString& RemoveCRLF();
};		// class CWString

// handy global temporary CWString fcns
inline CWString tTrim( const char* s)	// note: s=NULL OK (returns "")
{	return CWString( s).Trim(); }
inline CWString tCopy( const char* s)	// note: s=NULL OK (returns "")
{	return CWString ( s); }
CWString tCase( const char* s, const char toCases[ 3]);
CWString tFormatV( LPCTSTR lpszFormat, va_list ap);
CWString tFormat( LPCTSTR lpszFormat, ...);
CWString tFmtIf( LPCTSTR fmtNonBlank, LPCTSTR fmtBlank, ...);
inline CWString tFmtFloatDTZ( double v, int maxPrec, int minPrec=0)
{	return CWString().FmtFloatDTZ( v, maxPrec, minPrec); }
inline CWString tMsgPfx( const char* msgPfx, const char* delim=": ")
{	return tFmtIf( "%s%s", "", msgPfx, delim); }

// tPluralize: form plural of word
CWString tPluralize( const char* word, bool bPlural=true);
// tPluralize variants count as int and float
inline CWString tPluralize( const char* word, int count)
{	return tPluralize( word, count != 1); }
inline CWString tPluralize( const char* word, float count)
{	return tPluralize( word, fabs( count-1.f) < 0.001); }
// tPluralize variants: return "1 zone" or "2 zones"
inline CWString tFmtPlural( int count, const char* word,
		const char* fmt="%d %s")
{	return tFormat( fmt, count, tPluralize( word, count)); }
inline CWString tFmtPlural( float count, const char* word,
	const char* fmt="%0.2f %s")
{	return tFormat( fmt, count, tPluralize( word, count)); }
#endif

#endif	// _STRPAK_H

// end of strpak.h
