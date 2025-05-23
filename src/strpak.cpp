///////////////////////////////////////////////////////////////////////////////
// strpak.cpp -- string functions
///////////////////////////////////////////////////////////////////////////////

// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/*------------------------------- INCLUDES --------------------------------*/
#include "cnglob.h"

#include "messages.h"	// MSG_MAXLEN msgI

#include "xiopak.h"

// #include <strpak.h>	// decls for this file (#included in cnglob.h)

#if defined( _DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
// static char THIS_FILE[] = __FILE__;
#endif

// === Tmpstr ===
// General temporary string buffer.  Many uses, via strtemp().
//   Implemented as a ring buffer.  When full, buffer is re-used from start.
//   Thus strings in buffer survive "for a while".  Do not use for permanent strings.
const size_t TMPSTRSZ = 400000;		// size of Tmpstr[].
static char Tmpstr[ TMPSTRSZ+2];	// buffer.
// Each allocation is followed by prior TmpstrNx value for rev-order dealloc (strtempPop).
// +2 extra bytes at end hold flag re overwrite check (obsolete? 7-10)
static int TmpstrNx = 0;	// Next available byte in Tmpstr[].

// == CULTSTR ==
// Persistent string type that can be manipulated in the CUL realm.
//    (e.g. user input data and expressions, probes etc.)
//    Implemented as indicies into a vector of std::string.
//    Important motivation is 4-byte size (same as float and NANDLE).
//    Cannot use char * due to 8-byte size on 64 bit.

// CULSTREL: one element in vector of strings
//   = string in dynamic memory plus integer that chains free elements.
struct CULSTREL
{
	enum { uslEMPTY, uslDM, uslOTHER};

	CULSTREL() : usl_str( nullptr),  usl_status( uslEMPTY), usl_freeChainNext( 0)
	{ }

	CULSTREL(const char* str) : CULSTREL()
	{
		usl_Set( str);
	}

	CULSTREL(CULSTREL&& src) noexcept
		: usl_str{ src.usl_str }, usl_status{ src.usl_status },
		usl_freeChainNext{ src.usl_freeChainNext }
	{
		src.usl_str = nullptr;	// prevent dmfree of string in moved-out-of source
	}

	~CULSTREL()
	{
		usl_freeChainNext = 0;
		if (usl_status == uslDM)
			dmfree(DMPP(usl_str));
		else
			usl_str = nullptr;
		usl_status = uslEMPTY;
	}

	char* usl_str;				// string data
	int usl_status;				// uslEMPTY: empty (usl_str may or may not be nullptr)
								// uslDM: in use, usl_str points to heap
								// uslOTHER: in use, usl_str points elsewhere (do not dmfree)
	HCULSTR usl_freeChainNext;	// next element in chain of unused CULTRELs
	char* usl_Set(const char* str);

};	// struct CULSTREL
//-----------------------------------------------------------------------------
char* CULSTREL::usl_Set(		// set CULSTR
	const char* s)		// source string (will be copied)
{
	if (usl_status == uslOTHER)
		usl_str = nullptr;
	int sz = strlenInt(s) + 1;
	dmral(DMPP(usl_str), sz, ABT);
	usl_status = uslDM;
	return strcpy(usl_str, s);
}	// CULSTREL::usl_Set
//=============================================================================

CULSTRCONTAINER::CULSTRCONTAINER() : us_freeChainHead{ 0 }
{
	us_vectCULSTREL.push_back("");
}


/*static */  CULSTRCONTAINER CULSTR::us_csc;



//-----------------------------------------------------------------------------
CULSTR::CULSTR() : us_hCulStr(0) {}
//-----------------------------------------------------------------------------
CULSTR::CULSTR(const CULSTR& culStr) : us_hCulStr( 0)
{
	Set(culStr.CStr());
}
//-----------------------------------------------------------------------------
CULSTR::CULSTR(const char* str) : us_hCulStr( 0)
{
	Set(str);

}
//-----------------------------------------------------------------------------
char* CULSTR::CStrModifiable() const	// pointer to string
// CAUTION non-const; generally should use CStr()
{
	return IsNANDLE() ? nullptr : us_GetCULSTREL().usl_str;

}	// CULSTR::CStrModifiable()
//-----------------------------------------------------------------------------
bool CULSTR::IsNANDLE() const
{
	return ISNANDLE(us_hCulStr);
}	// CULSTR::IsNANDLE
//-----------------------------------------------------------------------------
bool CULSTR::us_HasCULSTREL() const
{
	return !IsNANDLE() && !IsNull();
}	// CULSTR::us_HasCULSTREL
//-----------------------------------------------------------------------------
void CULSTR::Set(			// set from const char*
	const char* str)	// source string
						// if nullptr, release
{
	if (!str)
	{
		Release();
		// us_hCulStr = 0 in us_Release
	}
	else
	{
		if (!us_HasCULSTREL())
		{	// this CULSTR does not have an allocated slot
			if (us_AllocMightMove())
				str = strtmp(str);		// str may be pointing into string vector
			us_Alloc();					// can move!
		}

		assert(us_hCulStr != 0);

		us_GetCULSTREL().usl_Set(str);
	}
}		// CULSTR::Set
//-----------------------------------------------------------------------------
void CULSTR::FixAfterCopy()
{
	if (!IsNANDLE() && IsSet())
	{
		const char* str = CStr();
		us_hCulStr = 0;
		Set(str);
	}
}		// CULSTR::FixAfterCopy
//-----------------------------------------------------------------------------
bool CULSTR::IsValid() const
// returns true iff CULSTR is valid
{
	// check handle: s/b NANDLE or within known range
	bool bValid = IsNANDLE()
		|| (us_hCulStr >= 0 && us_hCulStr < us_csc.us_vectCULSTREL.size());
	if (!bValid)
		err( PERR, "Invalid CULSTR %d", us_hCulStr);

#if defined( _DEBUG)
	// CULSTR 0 should always be ""
	const char* str0 = us_csc.us_vectCULSTREL[0].usl_str;
	if (!str0 || strlen(str0) > 0)
		err( PERR, "Bad str0");
#endif

	return bValid;
}		// CULSTR::IsValid
//-----------------------------------------------------------------------------
CULSTREL& CULSTR::us_GetCULSTREL() const
{
	return us_csc.us_vectCULSTREL[ IsValid() ? us_hCulStr : 0];

}	// CULSTR::us_GetCULSTREL();
//-----------------------------------------------------------------------------
void CULSTR::us_Alloc()		// allocate
{
	if (us_csc.us_freeChainHead)
	{	// use available free slot
		us_hCulStr = us_csc.us_freeChainHead;
		us_csc.us_freeChainHead = us_GetCULSTREL().usl_freeChainNext;
	}
	else
	{	// no free slot, enlarge vector
		if (us_csc.us_vectCULSTREL.size() == 0)
			us_csc.us_vectCULSTREL.emplace_back("");
		us_csc.us_vectCULSTREL.emplace_back();
		us_hCulStr = HCULSTR( us_csc.us_vectCULSTREL.size()) - 1;
	}
}		// CULSTR::us_Alloc
//-----------------------------------------------------------------------------
void CULSTR::Release()		// release string
// revert CULSTR to empty state
{
	if (us_hCulStr != 0 && !IsNANDLE())
	{	CULSTREL& el = us_GetCULSTREL();

		// string pointer: do not free (memory will be reused)
		if (el.usl_status == CULSTREL::uslOTHER)
			el.usl_str = nullptr;	// but maybe set null

		el.usl_freeChainNext = us_csc.us_freeChainHead;
		us_csc.us_freeChainHead = us_hCulStr;
	}

	us_hCulStr = 0;

}	// CULSTR::Release
//-----------------------------------------------------------------------------
bool CULSTR::us_AllocMightMove() const	// check if reallocation is possible
// returns true iff next us_Alloc might trigger us_vectCULSTR reallocation
{
	return us_csc.us_freeChainHead == 0
		&& us_csc.us_vectCULSTREL.size() == us_csc.us_vectCULSTREL.capacity();
}		// CULSTR::us_AllocMightMove
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
int strCheckPtr( DMP p)		// check DM ptr for strpak conflict
{
	return !(p >= Tmpstr && p <= Tmpstr + TMPSTRSZ);
}	// strCheckPtr
// ================================================================
char* strxtok( 		// Extract a token from a string

	char* tokbuf,		// Pointer to buffer to receive extracted token, or NULL, to use Tmpstr.
	const char* &p,		// string pointer.
    					//  *p is the starting point for scan and is updated to be the start point for next call.
    					//  Successive calls will retrieve successive tokens.
	const char* delims,	// String of delimiter characters ('\0' terminated).
	bool wsflag )		// White space flag.
					    //  true:  White space around delimiters is ignored (and trimmed from returned token).
						//         WS is also taken to be a secondary delimiter.
						//	false: WS is treated as standard character.
						//         WS chars can then be included in delims string if desired.

// This routine is similar in function to Microsoft strtok, but has more friendly specs.
//   The MS routine keeps an internal pointer to the working string, making nested calls impossible.
//   NEVER USE STRTOK!

// Returns NULL if no more tokens in string.  Otherwise, returns pointer to extracted token.
{
	int ib = -1;
	int ie = -1;
	const char* ps = p;
	int i = 0;
	int endscan = FALSE;
	char c;
	while ((c = *(ps+i)) != 0)
	{	if (isspaceW(c) && wsflag)
			endscan = (ib != -1);
		else if (strchr( delims, c) != NULL)
		{	i++;
			break;
		}
		else
		{	if (endscan)
				break;
			if (ib == -1)
				ib = i;
			ie = i;
		}
		i++;
	}  /* Scan loop */

// Set up return
	p = ps+i;
	int ltok = 0;
	if (ib==-1)
	{	if (c=='\0')
			return NULL;
	}
	else
		ltok = ie - ib + 1;

	return strncpy0( tokbuf, ps+ib, ltok+1);

}		// strxtok
//-----------------------------------------------------------------
int strTokSplit(			// split string into tokens IN PLACE
	char* str,			// string (returned modified)
	const char* delims,	// delimiter characters (e.g. " \t" or ",")
	const char* toks[],	// returned: pointers to tokens
	int dimToks)		// dimension of toks[]

// returns # of tokens identified
//        -1 if more than dimToks (toks[] filled / scan stopped)
{
	memset( toks, 0, sizeof( char*)*dimToks);	// init all ptrs to NULL
	int iTok=0;
	const char* curDelims = NULL;	// current delimiters
									//   either caller's delims or "\""
	for (char* p = str; ; )
	{	while (*p && isspaceW(*p))
			p++;			// pass spaces
		if (!*p)
			break;  		// end of string, done
		if (*p == '"')
		{	p++;
			while (*p && isspaceW(*p))
				p++;			// pass spaces
			// note: unpaired quote will return tok=<rest of string> (which may be empty)
			curDelims = "\"";		// include everything until closing quote
		}
		else
			curDelims = delims;		// not quoted, use caller's delims
		if (iTok == dimToks)
		{	iTok = -1;
			break;		// too many tokens
		}
		toks[ iTok++] = p;		// beg of token (after any WS)
		while (*p && !strchr( curDelims, *p))
			p++;
		int bEnd = *p == '\0';	// nz iff token end is string end
		*p++ = '\0';		// terminate token: change terminator to \0
		for( char* px = p-2; px >= toks[ iTok] && isspaceW( *px); px--)
			*px = '\0';		// trim trailing WS
		if (bEnd)
			break;   		// end of string, done
	}
	return iTok;
}		// strTokSplit
// ================================================================
char* memsetPass( char* &d, char c, size_t n)		// memset and point past
// returns updated d
{
	memset( d, c, n);
	d += n;
	return d;
}		// memsetPass
// ================================================================
char* memcpyPass( char* &d, const char* src, size_t n)		// memcpy and advance destination pointer past
// returns updated d
{
	memcpy( d, src, n);
	d += n;
	return d;
}		// memcpyPass

// ================================================================
bool memcpyPass(		// memcpy with overrun protection
	char*& d,	// destination buffer (returned pointing to next char)
	size_t& dSize,	// available space in d (returned reduced by n)
	const char* src,	// source
	size_t n)			// number of chars to copy
// returns true iff success
//    else false if src truncated (dSize now 0)
{
	bool bEnufSpace = n <= dSize;
	if (!bEnufSpace)
		n = dSize;
	memcpy(d, src, n);
	d += n;
	dSize -= n;
	return bEnufSpace;
}		// memcpyPass
// ====================================================================
char* strncpy0(			// copy/truncate with 0 fill
	char* d,		// destination or NULL to use Tmpstr[]
	const char* s,	// source
	size_t l)		// dim( d) (that is, including space for \0)
					//   NOTE spec change from prior version!
					// if 0, d is not altered
{
	if (!d)
		d = strtemp( static_cast<int>(l)-1);	// allocate l bytes in Tmpstr
	if (l > 0)
	{	for (size_t i=0,j=0; i<l-1; i++)
		{	d[ i] = s[ j];		// copy char or 0 fill
			if (s[ j])			// do not ref beyond end of s
				j++;
		}
		d[ l-1] = '\0';
	}
	return d;
}		// ::strncpy0
// ====================================================================
char* strt_string_view(		// copy string_view to temporary
	std::string_view sv)	// string view
{
	// strncpy0 handles sv.size() = 0
	char* sRet = strncpy0( nullptr, sv.data(), sv.size()+1);
	return sRet;
}		// :: strt_string_view
// ====================================================================
char* FC strTrim( 			// trim white space from beginning and end of a string
	char* s1,			// destination (can be the same as s2), or NULL to use Tmpstr
	const char* s2,		// string to be trimmed
	int s2len/*=999999*/)	// s2 max length (stops at shorter of s2len or \0)

// Returns s1 (pointer to destination string)
{
	if (s1 == NULL)
		s1 = strtemp( s2len < 999999 ? s2len : strlenInt( s2));	// alloc n+1 Tmpstr[] bytes. local.

	char* p{ s1 };
	char* pend{ s1 };
	char c;
	int seenNonWS = FALSE;
	for (int is2=0; (c = s2[ is2])!=0 && is2<s2len; is2++)
	{	if (!isspaceW(c))
		{	seenNonWS = TRUE;
			pend = p+1;
		}
		if (seenNonWS)
			*p++ = c;
	}
	*pend = '\0';
	return s1;
}		// strTrim
//----------------------------------------------------------------------------
char* strTrimE( char* d, const char* s)		// deblank end of string
//  if d == s, trims in place
{
	size_t l = strlen( s);
	while (l && isspaceW(*( s+l-1)) )
		l--;
	return strncpy0( d, s, l+1);
}		// strTrimE
//----------------------------------------------------------------------------
char* strTrimEX(	// deblank / clean end of string
	char* d,			// destination (NULL = tmpstr)
	const char* s,		// source
	const char* ex)		// additional chars to drop
//  if d == s, trims in place
{
	size_t l = strlen( s);
	while (l)
	{	char c = *(s+l-1);
		if (!isspaceW( c) && !strchr( ex, c))
			break;
		l--;
	}
	return strncpy0( d, s, l+1);
}		// strTrimEX
//----------------------------------------------------------------------------
char* strTrimE( char* s)		// deblank end of string IN PLACE
{
	size_t l = strlen( s);
	while (l-- && isspaceW( *( s+l)) )
		*(s+l) = '\0';
	return s;
}		// strTrimE
//----------------------------------------------------------------------------
char * strTrimB( char* d, const char* s)		// deblank beg of string
//  if d == s, trims in place
{
	return strcpy( d, strTrimB( s));
}		// strTrimB
// ====================================================================
int strLineLen( const char *s)		// line length including \n (# chars thru 1st newline or to \0)  rob 11-91

// related problem: line body length, excluding \n: use strcspn( s, "\r\f\n") inline.
{
	size_t len = strcspn( s, "\n");		// # chars TO \n or \0
	if (*(s + len)=='\n')			// if \n is present
		len++; 					// include it in count also
	return static_cast<int>(len);
}		// strLineLen
// =========================================================================
int strJoinLen( const char *s1, const char *s2)	// length of line formed if strings joined

// return length of line that would be formed where strings were concatentated, allowing for \n's in the strings,
// eg for deciding whether to insert \n between them when concatenating.
{
	const char* u = strrchr( (char *)s1, '\n');	// NULL or ptr to last newline
	size_t len =
		strlen( u ? u+1 : s1)     	// length LAST line of s1
		+  strcspn( s2, "\r\n");		// length FIRST line of s2
	return static_cast<int>(len);
}				// strJoinLen
// =======================================================================
char * FC strpad( 		// Pad a string to length n in place

	char *s,		// String to pad
	const char *pads,	// Pad string: typically " "; any length & different chars ok
	int n )			// Length to pad to

/* Returns pointer to s.  If strlen(s) >= n, no action is performed.
   Otherwise, pads is appended to s until n is reached. */
{
	int i = strlenInt(s);
	int lp = strlenInt(pads);
	int j = -1;
	while (i < n)
		*(s + i++) = *(pads + (j = ++j < lp ? j : 0) );
	*(s+i) = '\0';
	return s;
}		// strpad
// =======================================================================
char* strSpacePad( 		// Pad a string with spaces (e.g. for FORTRAN)
	char* d,				// destination
	size_t n,					// length to pad to
	const char* s /*=NULL*/)	// source
								//  if NULL, d padded in place
								//  (must be null-terminated)
// does NOT terminate with '\0'
// returns pointer to d
{
	size_t lS;
	if (s)
	{	lS = min( n, strlen( s));
		memcpy( d, s, lS);
	}
	else
		lS = min( n, strlen( d));

	memset( d+lS, ' ', n-lS);

	return d;
}		// strSpacePad
// ====================================================================
const char* FC strffix( 	// put a filename in canonical form

	const char* name, 	// input filname
	const char* ext ) 	// default extension including period

// returns uppercase filename with extension in Tmpstr[]
{
	const char* lastslsh = strrchr( name, '\\');
	char* nu = (strrchr( name, '.') <= lastslsh)
				? strtcat( name, ext, NULL)
				: strtcat( name, NULL);
	strTrim( nu, _strupr(nu));		// trim in place (in Tmpstr)
	return nu;
}		// strffix
//-------------------------------------------------------------------
const char* strffix2( 			// put a filename in canonical form (variant)
	const char* name, 	// input filname
	const char* ext, 	// default extension including period
	int options /*=0*/)	// option bits
						//   1: always add extension
// do *not* uppercase (as does strffix())
// returns filename with extension in Tmpstr[]
{
	int bAddExt = FALSE;
	if (options & 1)
		bAddExt = TRUE;
	else
	{
		bAddExt = !xfhasext(name);
	}

	const char* nu = strtcat(name, NULL);
	if (bAddExt)
		nu = strtcat( nu, strTrim( NULL, ext), NULL);
	return nu;
}		// strffix2
// ====================================================================
const char* FC strtPathCat( 		// concatenate file name onto path, adding intervening \ if needed

	const char* path, 		// drive and/or directorie(s) path
	const char* namExt)  	// file name or name.ext

// returns assembled file pathname in Tmpstr[]
{
        char full[CSE_MAX_PATH];
        xfjoinpath(path, namExt, full);
	return strtmp( full);
}						// strtPathCat
// ====================================================================
char* FC strpathparts( 	// Build string from parts of a path name (for default file names etc)

	const char* path,	// Full (valid) pathname or filename
	int partswanted,	// Bits indicating which path parts are to be included in result --
						//   STRPPDRIVE:     Drive
						//   STRPPDIR:       Directory
						//   STRPPFNAME:     File name (w/o extension)
						// STRPPEXT:       Extension (including '.')
	char* pcombo/*=NULL*/)	// buffer [CSE_MAX_PATH] for result
							//   NULL = use TmpStr

// Example: strpathparts( path, STRPPFNAME+STRPPEXT) will return name+extension found in path.

// Returns assembled string, equested parts which are not present in *path are omitted from result.
{
	char pbuf[CSE_MAX_PATH *4];
	static char wantmask[4] = { STRPPDRIVE, STRPPDIR, STRPPFNAME, STRPPEXT };

#define part(p) (pbuf+((p)*CSE_MAX_PATH))

	// Split the path into the necessary components
	xfpathroot(path, part(0));
	xfpathdir(path, part(1));
	xfpathstem(path, part(2));
	xfpathext(path, part(3));

	if (!pcombo)
		pcombo = strtemp(CSE_MAX_PATH);	// alloc n+1 Tmpstr[] bytes. local
	*pcombo = '\0';
	for (int i = 0; i < 4; i++ )
		if (partswanted & wantmask[i])
			strcat( pcombo, part(i));
	return pcombo;
}			// strpathparts
// =========================================================================
char * FC strtBsDouble( char *s)	// double any \'s in string, for displaying file paths etc thru code that interprets \'s

// returns given string, or modified string in Tmpstr
{
// count \'s, return input if none

	int nBs = 0;			// # chars to put \ b4
	char* p = s;
	while ((p = strchr( p, '\\')) != 0)	// find next \, end loop if none
	{
		nBs++;				// count \'s
		p++;				// continue search at next char
	}
	if (!nBs)				// if no \'s
		return s;			// input is ok as is. normal exit.

// copy to Tmpstr, doubling \'s
	char* t;
	p = t = strtemp( strlenInt(s) + nBs);	// alloc n+1 Tmpstr[] bytes. local.
	char c;
	while ((c = *s++) != 0)
	{
		if (c=='\\')			// before each '\'
			*p++ = '\\';			// insert a '\'
		*p++ = c;
	}
	*p = '\0';
	return t;				// another return above
}		// strtBsDouble
// =========================================================================
char * FC strBsUnDouble( char *s)	// un-double any \'s in string: for reversing effect of strtBsDouble

// returns given string with \\ -> \ IN PLACE
{
	int is = 0, ir = 0;			// source, result offsets

	while ( (*(s+ir++) = *(s+is)) != 0 )	// move char AND test for null term
	{
		if (    *(s+is) == '\\'			// if this and next are '\'
				&& *(s+is+1) == '\\')
			is++;					// extra advance
		is++;					// normal advance
	}
	return s;
}		// strBsUnDouble
//-----------------------------------------------------------------------------
const char* strOrdinal(		// convert number to string in Tmpstr with st, rd, th as appropriate
    int number)
{
	static const char *cases[] = {"%dth", "%dst", "%dnd", "%drd"};
	int caseNr;
	int mod100 = number % 100;
	if (mod100 > 10 && mod100 < 20)	// for 11, 12, 13, 111, 211...
		caseNr = 0;			// say 11th not 11st ...
	else
		caseNr = number % 10;
	if (caseNr > 3)
		caseNr = 0;
	return strtprintf( cases[caseNr], number);
}				// strOrdinal

// ====================================================================
char* FC strsave(		// save a copy of a string in heap
	const char *s )	// NULL or pointer to character string.
// Returns pointer to saved string, or NULL if argument is NULL
{
	if (!s)					// for convenience,
		return NULL;		// strsave(NULL) is NULL
	char* p;
	dmal( DMPP( p), strlen(s)+1, ABT);		// allocate heap space, dmpak.c.  failure unlikely for small blocks.
	strcpy( p, s);
	return p;
}		// strsave
//-----------------------------------------------------------------------------
char* FC strsave(		// update a copy of a string in heap
	char* &p,
	const char *s )	// NULL or pointer to character string.
// Returns pointer to saved string, or NULL if argument is NULL
{
	dmfree( DMPP( p));
	if (!s)					// for convenience,
		return NULL;		// strsave(NULL) is NULL.
	dmal( DMPP( p), strlen(s)+1, ABT);		// allocate heap space, dmpak.c.  failure unlikely for small blocks.
	strcpy( p, s);
	return p;
}		// strsave
//-----------------------------------------------------------------------------
#if 0	// out of service, fix if needed, 3-28-10
x#ifdef __cplusplus  // BC++ or any C++
x//========================================================================
xRC FC strpsave( 	// Save a copy of a string, set pointer, return error code: alternate call for strsave
x
x    char * &p,   	// pointer to be set to saved string
x    const char *s,	// string to save
x    int erOp )		// error action (IGN, WRN, ABT, etc, cnglob.h) plus: DMFREEOLD bit: free existing memory if p not NULL
x
x	// to free the string, use dmfree(p)
x
x// returns RCOK ok, else other (non-0) value (if not ABT), message already issued (if not IGN).
x{
xRC rc;
x
x    rc = dmal( DMPP( p), strlen(s)+1, erop);   	// dmpak.c
x    if (rc==RCOK)					// rc == RCOK expected from dmpal
x       strcpy( p, s);
x    return rc;
x}		// strpsave
x#else
x-// ====================================================================
x-RC FC strpsave( 	/* Save a copy of a string given pointer to where to store pointer */
x-
x-    char **pp,   	// pointer to pointer to be set to saved string
x-    const char *s,	// string to save
x-    int erOp )		/* error action (IGN, WRN, ABT, etc, cnglob.h) plus:
x-			   DMFREEOLD bit: free existing memory if pointer *p not NULL */
x-
x-	/* saved string may be accessed *pp. */
x-
x-	/* to free the string, use dmpfree(pp) */
x-
x-// returns RCOK ok, else other (non-0) value (if not ABT), message already issued (if not IGN).
x-{
x-RC rc;
x-
x-    rc = dmal( *(void **)pp, strlen(s)+1, erop);	// dmpak.c
x-    if (rc == RCOK)					// rc == RCOK expected from dmal
x-       strcpy( *pp, s);
x-    return rc;
x-}		/* strpsave */
x#endif
#endif	// 3-28-10
// ====================================================================
char * FC strtemp(		// allocate n+1 bytes in temp string buffer (Tmpstr[])
	int n )		// number of bytes needed, less 1 (1 added here eg for null)

// returns pointer to space in Tmpstr : Valid for a (very) short while

{
	n++;					// for terminal 0: saves many +1's in calls
	int priorNx = TmpstrNx;				// save for reverse ptr after the new alloc
	if (TmpstrNx + n + sizeof( int) > TMPSTRSZ)	// if full
		TmpstrNx = 0;				// wrap. else s==Tmpstr+priorNx.
	char* s = Tmpstr + TmpstrNx;	// pointer to return to caller
	TmpstrNx += n;					// pass allocated space
	*(int *)(Tmpstr+TmpstrNx) = priorNx;	// after it put old TmpstrNx value
	TmpstrNx += sizeof( int);				// point past that for next call
	return s;
}			// strtemp
// ======================================================================
char * FC strtempPop( char *anS)	// conditionally deallocate temp string buffer space

/* If given pointer is MOST RECENT strtemp-returned value,
	it is deallocated and used for next allocation.  Else nop.
	Multiple calls in exact reverse order will do multiple deallocations.

	Use (esp in loops!) to reduce chance of wrapping around Tmpstr and
	overwriting data that is e.g. by caller. */

// returns arg: valid till next strtemp -- e.g. ok for use with c lib fcn
{
	if (TmpstrNx < sizeof( int)  	// nop if startup
			|| TmpstrNx > TMPSTRSZ)		// or garbage -- insurance
		return anS;

// fetch TmpstrNx value b4 last strtemp
	int priorNx = *(int *)(Tmpstr + TmpstrNx - sizeof( int));
	if (priorNx < 0 || priorNx > TMPSTRSZ)	// nop if out of range
		return anS;				// insurance re bugs or whole Tmpstr[]'s worth of deallocs
// determine pointer most recent strtemp() returned
	char* s = Tmpstr + (priorNx > TmpstrNx		// if priorNx > current Nx
					?  0 			// then last strtemp wrapped to start
					:  priorNx ); 		// else, s was Tmpstr+priorNx.

// if deallocation request is for last allocation, deallocate
	if (s==anS)
		TmpstrNx = priorNx;		// deallocate / let it be reused NEXT.
	/* else do nothing if:
	      there has been an intervening alloc,
	      or have dealloc'd whole Tmpstr, back to overwritten stuff,
	      or other unexpected garbage. */

	return anS;			// return location of (vulnerable!) string: strUntemp thus can be nested in call using anS.
}		// strtempPop
// ======================================================================
char * FC strtmp( const char *s)		// make a copy of string in Tmpstr[] and return pointer to it

{
	return strtcat( s, NULL);
}				// strtmp
// ====================================================================
char* strtcat( const char *s, ... )	// concatenate variable number of strings in Tmpstr

// last arg must be NULL

// returns pointer to Tmpstr ... do a strsav for permanent copy !!
{
// determine total length
	int len=0;
	const char* padd = s;				// first arg
	va_list arg;
	va_start(arg, s);			// set up to get subsequent args
	while (padd)			// NULL arg terminates
	{
		len += strlenInt(padd);
		padd = va_arg( arg, char *);
	}

// allocate destination space
	char* p = strtemp(len);    		// alloc len+1 Tmpstr[] bytes. local.
	*p = '\0';				// initialize to null string

// concatentate strings into space
	padd = s;				// first arg
	va_start( arg, s);			// setup for addl args if any
	while (padd)			// NULL arg terminates
	{
		strcat( p, padd); 		// concatenate an arg
		padd = va_arg( arg, char *);	// get next arg
	}
	return p;			// return address of combined strings
}		// strtcat
// ====================================================================
char * CDEC strntcat( // concatenate variable number of strings up to a maximum length in Tmpstr
	int n,	// maximum length of string
	... )   // strings to concat, terminated by NULL

// returns pointer to Tmpstr ... do a strsav for permanent copy !!
{

	char* p = strtemp(n);		// alloc n+1 Tmpstr[] bytes. local.
	*p = '\0';			// init to null string for concatenation

	va_list arg;
	va_start( arg, n);
	char* padd;
	while ((padd = va_arg( arg, char *)) != NULL)
	{
		strncat( p, padd, n);
		if ((n -= strlenInt(padd)) <= 0)
			break;
	}
	return p;
}		// strntcat
//===========================================================================
const char* FC scWrapIf(		// concatenate strings with wrap if needed
	const char* s1,		// 1st string
	const char* s2,		// 2nd string
	const char* tweenIf,	// text to include between them if overlong
							//   line would result
	int lineLength /*=defaultCpl*/)	// line length

// concatenate strings to Tmpstr, with text "tweenIf" between them if overlong line would otherwise result
// 
// result in Tmpstr
{
	int maxll = lineLength - 3; // leave some space for adding punctuation, indent, etc.
	return strtcat( s1,
					strJoinLen( s1, s2) 		// length of line if joined
					     > maxll  ?  tweenIf  :  "",
					s2,
					NULL );
}	// scWrapIf
// ======================================================================
const char* CDEC strtprintf( 	// make like sprintf and return pointer to result in tmpstr
	MSGORHANDLE mOrH, ...)	// format string or message handle
{
	va_list ap;
	va_start( ap, mOrH);
	return strtvprintf( mOrH, ap);
}			// strtprintf
// ======================================================================
const char* FC strtvprintf( 	// make like vsprintf and return pointer to result in tmpstr.
	MSGORHANDLE mOrH,	// format string or message handle
						//   mOrH.IsNull(): return ""
	va_list ap)			// args
{
	char buf[ MSG_MAXLEN];

	msgI( WRN, buf, sizeof( buf), NULL, mOrH, ap);	// gets text if handle,
								// checks length, vprintf's to buf.  returns rc.
	return strtmp( buf);
}			// strtvprintf
// ======================================================================
WStr WStrPrintf( 	// make like sprintf and return pointer to result in XSTR
	MSGORHANDLE mOrH, ...)		// format string or message hangle
{
	va_list ap;
	va_start( ap, mOrH);
	return WStrVprintf( mOrH, ap);
}			// WStrPrintf
// ======================================================================
WStr WStrVprintf(	// make like vsprintf and return pointer to result in tmpstr.
	MSGORHANDLE mOrH,		// format string or message handle
	va_list ap /*=NULL*/)					// arg list
{
	char buf[ MSG_MAXLEN];

	msgI( WRN, buf, sizeof( buf), NULL, mOrH, ap);	// gets text if handle,
								// checks length, vprintf's to buf.  returns rc.
	WStr t = buf;
	return t;
}			// WStrVprintf
// ======================================================================
WStr WStrFmtFloatDTZ(		// format float / strip trailing 0s
	double v,				// value
	int maxPrec,			// max precision (used for initial format)
	int minPrec /*=0*/)		// min precision, if 0, '.' deleted also
{
	WStr s = WStrPrintf( "%0.*f", maxPrec, v);
	return WStrDTZ( s, minPrec);
}		// WStrFmtFloatDTZ
//------------------------------------------------------------------------
WStr WStrDTZ(		// drop trailing 0s
	const WStr& s,			// original string
	int minPrec /*=0*/)		// min precision, if 0, '.' deleted also
// returns s w/o trailing 0s
{
#if 0 && defined( _DEBUG)
	// test code, 5-2019
	static int tested = 0;
	if (!tested++)
	{	printf("\nWStrDTZ test\n");
		const char* t[] = { "0", ".0", "0.0010", "3.010200", "1230", "1205", "01.", "1.324", "" };
		for (int mp = 5; mp >= 0; mp--)
		{	printf("minPrec = %d\n", mp);
			for (int it = 0; it < sizeof(t) / sizeof(t[0]); it++)
			{	WStr tx = t[it];
				WStr txDTZ = WStrDTZ(tx, mp);
				printf("  %10s   %10s\n", t[it], txDTZ.c_str());
			}
		}
		printf("\n");
	}
#endif
	int len = static_cast<int>(s.length());
	int newLen = len;	// candidate new length
	for (int i = len; --i >= 0; )
	{	if (s[i] == '0')
		{	if (newLen == i + 1)
				newLen = i;
		}
		else if (s[i] == '.')
		{	if (newLen <= i + minPrec)
				newLen = i + minPrec + 1;
			else if (!minPrec && newLen == i + 1)
				newLen = i;		// drop decimal too
			return newLen == 0
						? "0"	// string was ".0000"
						: s.substr(0, newLen);
		}
		else if (newLen == len)
			break;		// no trailing 0s, give up
		// else keep scanning
	}
	// never hit decimal point, do nothing
	return s;
}			// WStrDTZ
// ======================================================================
int FC strlstin(			// case insensitive search for string in list

	const char *list,	// list of strings delimited by " ,/"
	const char *str )	// string to search for in list, len > 0

// returns nz if str is found in list, else 0
{
	int llist = strlenInt(list);
	int lstr = strlenInt(str);
	int off = 0;
	int match = 0;
	while (off < llist)
	{
		int lsub = static_cast<int>(strcspn( list+off, " ,/"));	// find next delim
		if (lsub
		 && lsub == lstr
		 && !_strnicmp( str, list+off, lsub) )
		{
			match = 1;
			break;
		}
		off += lsub + 1;
	}
	return match;
}		// strlstin
//===========================================================================
int strDecode(					// decode numeric value (internal fcn)
	//   note inline mapping fcns (str_opts.h)
	const char* s,		// string to be decoded
	const char *fmt,	// decode format, with "guard char", e.g. "%lg%1s"
	//   Note: %1c will NOT scan over WS, use %1s
	void *p,			// where to store converted value
	[[maybe_unused]] int erOp/*=IGN*/,			// msg ctrl: IGN/WRN/ABT
	const char* what/*=NULL*/,	// item name for msg, NULL=generic message
	const char** pMsg/*=NULL*/)	// if non-NULL, error msg returned here
//   (in addition to display per erOp)
// On error, issue mbInpErr-style msg per erOp.

// returns # of values decoded; 1=OK, EOF, 0, 2 = too little, too much data
{
	char sink[ 2];	 		// target for (unexpected) 2nd item
	int n = sscanf( s, fmt, p, sink);
	if (n != 1)		// if bad
	{	const char* msg = strtprintf( "Invalid numeric value '%s'", s);
		if (!IsBlank( what))
			msg = strtcat( msg, strtprintf( "for %s", what));
		// mbInpErr( erOp, msg);
		if (pMsg)
			*pMsg = msg;
	}
	else if (pMsg)
		*pMsg = strtmp("");
	return n;
}			// strDecode
//---------------------------------------------------------------------------
WStr& WStrUpper( WStr& s)
{	std::transform( s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}	// WStrUpper
//---------------------------------------------------------------------------
WStr& WStrLower( WStr& s)
{	std::transform( s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}	// WStrLower
//--------------------------------------------------------------------------
BOOL IsBlank(					// flexible test for blank string
	const char* s,			// string
	int len /*=999999*/)	// length of string (if 0, always returns TRUE)
// Returns: TRUE if s is NULL or all WS (up to shorter of '\0' or len)
//          FALSE otherwise
{
	if (s)				// if NULL (NULL -> blank)
	{	for( int i=0; i<len; i++)
		{	if (!isspaceW( s[ i]))
				return !s[ i];		// TRUE if s[ i] == 0 (reached end)
		}
	}
	return TRUE;		// reached end w/o non-blank
}				// IsBlank
//--------------------------------------------------------------------------
char* strFill(			// fill string IN PLACE
	char* d,		// destination (overwritten)
	const char* s,	// source string, copied repeatedly until len
	int len/*=-1*/)	// len, -1 = strlen( d)
{
	if (d)
	{
		if (len < 0)
			len = strlenInt(d);
		if (!s || !*s)
			s = "?";
		int lenS = strlenInt(s);
		for (int i=0; i < len; i++)
			d[ i] = s[ i%lenS];
		d[ len] = '\0';
	}
	return d;
}		// strFill
//---------------------------------------------------------------------------
const char* strSuffix(   // generate string suffix
	int n,				// index
	[[maybe_unused]] int options /*=0*/)	// options
						//   future: might control style
// returns (in Tmpstr) "a", "b" ..."aa", ...
{
#if 0
0	static int testMe = 1;
0	if (testMe)
0	{	testMe = 0;
0		printf("\n");
0		for (int i = 0; i < 1200; i++)
0		{	const char* t = strSuffix(i);
0			printf("%s%s", i % 26 ? " " : "\n", t);
0		}
0	}
#endif
	const char* sfx = "";
	int iX = 0;
	do
	{	int idx = n % 26 - iX;
		sfx = strtprintf("%c%s", 'a' + idx, sfx);
		n /= 26;
		iX = 1;
	} while (n > 0);

	return sfx;
}	// strSuffix
//---------------------------------------------------------------------------
char* strDeBar( 						// translate '_' to ' '
	char* d,					// destination string
	const char* s /*=NULL*/)	// source; if NULL, deBar d in place

// returns d
{
	if (!s)
		s = d;

	int i = 0;
	while (s[ i])
	{	d[ i] = s[ i] == '_' ? ' ' : s[ i];		// copy / translate
		i++;
	}
	d[ i] = '\0';			// terminate

	return d;
}						// strDeBar
//----------------------------------------------------------------------------
char* strDeQuote(		// remove quotes
	char* d,					// destination string
	const char* s /*=NULL*/)	// source; if NULL, deBar d in place
// trims WS and " from beg and end of string
{
	if (!s)
		s = d;

	// scan over leading WS and "
	int ib = 0;		// idx of 1st char wanted
	while ( s[ ib] == '\"' || isspaceW( s[ ib]))
		ib++;

	return strTrimEX( d, s+ib, "\"");

}		// strDeQuote
//----------------------------------------------------------------------------
char* strDeWS( 						// remove WS chars
	char* d,					// destination string
	const char* s /*=NULL*/)	// source; if NULL, deWS d in place
// returns d
{
	if (!s)
		s = d;
	int iD = 0;
	for( int iS=0; s[ iS]; iS++)
	{
		if (!isspaceW( s[ iS]))
			d[ iD++] = s[ iS];
	}
	d[ iD] = '\0';			// terminate
	return d;
}			// strDeWS
//-----------------------------------------------------------------------------
const char* strQuoteIfNotNull( const char *s)	// quote if not null, & supply leading space

// CAUTION: returned value in Tmpstr is transitory
{
	return (s && *s) ? strtprintf( " '%s'", s) : "";
}	// strQuoteIfNotNull
//===========================================================================

//-----------------------------------------------------------------------------
char* strTranslateEscapes(	 	// translate standard escape sequences
	char* d,					// dest and maybe source string
	const char* s /*=NULL*/)	// source, if NULL, same as d
//   translation done in place
// implements C/C++ preprocessor escape sequence definitions

// returns d
{
	if (!s)
		s = d;

	int iD = 0;
	while ( *s)
	{
		int c = *s++;
		if (c == '\\')
		{
			c = *s++;
			switch ( c)
			{
			case 'n':
				c = '\n';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'b':
				c = '\b';
				break;
			case 'r':
				c = '\r';
				break;
			case 'f':
				c = '\f';
				break;
			case 'a':
				c = '\a';
				break;
			case '0':
				c = '\0';
				break;
			case 'x':
				c = strtol( s, (char **)&s, 16);
				// hex: translate any # of hex digits
				break;
			default:
#define isodigit( c) (isdigitW( c) && c != '8' && c != '9')
				if (isodigit( c))		// octal: 1, 2, or 3 oct digits
				{
					c = c - '0';
					if (isodigit( *s))
					{
						c = c * 8 + *s++ - '0';
						if (isodigit( *s))
							c = c * 8 + *s++ - '0';
					}
				}
#undef isodigit
				// else leave it alone (handles \\, \", \', \?,
			}
			if (c > 255)	// insurance against hex/octal excesses
				c = '?';
		}
		d[ iD++] = char( c);
	}
	d[ iD] = '\0';
	return d;
}		// strTranslateEscapes
//----------------------------------------------------------------------------
char* strCatIf(		// conditional concatenation
	char* d,			// input / destination
	size_t dSize,		// dim of d (including space for \0)
	const char* brk,	// break, e.g. "-" or ", "
	const char* s,		// concat string
	int options /*=0*/)	// options
						//   1: attempt to truncate d if dSize exceeded
// adds break string iff both d and s non-null --
//   d    s	result
// ---  ---	-------
// ""	 ""	 ""
// "d"   ""  "d"
// ""    "s" "s"
// "d"   "s" "d" + brk + "s"

// returns NULL if combined result length exceeds dSize
//    else d (updated)
{
// 7-2021 prior code used strcat_s.  However, library code
//   throws on too long altho docs say it returns errno_t.
//   Modified to check length here and use strcat()
	if ( s && *s)	// if s contains a string
	{	size_t dLen = strlen(d);
		size_t len = dLen ? dLen + strlen(brk) + strlen(s) : 0;
		if (len >= dSize)
		{	size_t dLenMax = dSize - (len - dLen) - 1;
			if (dLen < dLenMax || !(options & 1))
				return NULL;	// result does not fit
			else
				d[ dLenMax] = '\0';
		}
		if (*d)		// if d not ""
			strcat(d, brk);	// add break string
		strcat( d, s);			// add s
	}
	return d;
}		// strCatIf
//-------------------------------------------------------------------------
const char* strMakeTextList(		// combine strings into text list (for msgs)
	const std::vector< const char*>& strs,		// string pointers
	const char* brkLast,		// final separator, typically "and" or "or"
	const char* brk /*=","*/,	// separator
	const char* brkPad /*=" "*/)	// separator padding

// returns combined list in Tmpstr (or static).  strsave to make permanent.
{
	int nStr = static_cast<int>(strs.size());
	if (nStr == 0)
		return "";		// no strings

	int lenSep = strlenInt(brk) + strlenInt(brkPad);
	// conservative combined length
	int lenTot = strlenInt(brkLast) + (nStr - 1) * lenSep + 2;
	for(const char* s : strs)
		lenTot += strlenInt(s);

	// big enough buffer
	char* sComb = strtemp(lenTot);
	sComb[0] = '\0';

	// build up combined string
	//  (not efficient -- repeated strcats)
	int iStr = 0;
	for (const char* s : strs)
	{
		strcat(sComb, s);	// add string
		if (++iStr == nStr)
			break;			// no more, done
		if (nStr > 2)
			strcat(sComb, brk);	// add brk (e.g. ",") if more than 2 strings
		strcat(sComb, brkPad);	// pad (e.g. " ");
		if (iStr == nStr - 1)		// if next to last
		{	strcat(sComb, brkLast);	// add e.g. "and"
			strcat(sComb, brkPad);	// + pad
		}
	}

	return sComb;
}	// strMakeTextList
//-------------------------------------------------------------------------
char* strPluralize(				// form plural of a word
	char* d,				// returned: maybe pluralized word (case generally
							//   preserved)
	const char* word,		// word to consider
	bool bPlural/*=true*/)	// true: form plural, true: leave unchanged
{
// table of exceptions: words that don't add 's' to form plural
	struct
	{
		const char* wordSingular;
		const char* wordPlural;
	} excpTbl[] =
	{
		{ "goose", "geese"},
		{ "mouse", "mice"},
		{ "is",    "are"},
		{ "has",   "have"},
		// add add'l exceptions here here
		{ NULL, NULL }
	};
	if (!word || !word[ 0])
		*d = '\0';
	else if (!bPlural)
		strcpy( d, word);
	else
	{
		bool bException = false;
		int i;
		for (i=0; excpTbl[ i].wordSingular; i++)
		{
			if (!_stricmp( word, excpTbl[ i].wordSingular))
			{
				bException = true;
				break;
			}
		}
		if (bException)
		{
			char toCases[ 3];
			toCases[ 0] = toCases[ 1] = word[ 0];
			toCases[ 2] = word[ 1];
			strCase( strcpy( d, excpTbl[ i].wordPlural), toCases);
			// convert case of plural (will fail on "GoOsE")
		}
		else
		{
			strcpy( d, word);
			int len = strlenInt( d);		// len >= 1 (check above)
			BOOL bLastUpper = isupperW( d[ len - 1] ) > 0;
			// standard rules y -> ies, else add s
			if (tolower( d[ len-1]) == 'y')
				strcpy( d+len-1, bLastUpper ? "IES" : "ies");
			else
			{
				d[ len] = "sS"[ bLastUpper];
				d[ len+1] = '\0';
			}
		}
	}

	return d;
}			// strPluralize
//---------------------------------------------------------------------------
char* strCase( char* d, const char* s, const char toCases[3])
{
	return strCase( strcpy( d, s), toCases);
}
//---------------------------------------------------------------------------
char* strCase( char* s, const char toCases[3])
// converts case of chars of s in place
// toCases[] specification:    example: "X-x"
//    [0] 1st non-space of s, [1] 1st char addl words, [2] other chars.
//    each: lower case to convert to lower case,
//          upper case to convert to upper case,
//          non-letter for no change.

// returns s for convenience.
{

// internalize toCases[] for speed
	enum { lower, nc, upper } itc[3];
	itc[0] = itc[1] = itc[2] = nc;
	int i;
	for (i = 0; i < 3; i++)
	{
		if (!toCases[i])			// if < 3 chars given
			break;					// treat rest as -
		else if (islowerW(toCases[i]))
			itc[i] = lower;
		else if (isupperW(toCases[i]))
			itc[i] = upper;
	}

// convert string
	int nsSeen = 0;			// no non-space seen
	int which = 0;			// 1st char uses cases[0]
	int len = strlenInt(s);
	for (i = 0; i < len; i++)
	{
		if (isspaceW( *( s+i)) || s[ i] == '-')
			which = nsSeen > 0;	// for s[i+1]. note init: which = 0.
		// treat hyphen as space
		else				// spaces don't need case conversion
		{
			if (itc[which]==upper)
				*((unsigned char*)s+i) = toupper(*((unsigned char*)s+i));
			// upperize 1 char, handle accented chars
			else if (itc[which]==lower)
				*((unsigned char*)s+i) = tolower(*((unsigned char*)s+i));
			// lowerize 1 char, handle accented chars
			which = 2;			// for s[i] til next space
			nsSeen++;			// next word beginning is not 1st word
		}
	}
	return s;
}		// strCase
//-------------------------------------------------------------------------
char* strRemoveCRLF(		// remove all CR and LF chars
	char* str)	// string to modify (in place)
// returns str
{
	strReplace( str, '\r', '\0');
	strReplace( str, '\n', '\0');
	return str;
}		// strRemoveCRLF
//-------------------------------------------------------------------------
int strReplace(  			// replace occurances of a string
	char* d,			// destination (NOT! overlapping with str)
	size_t dSize,		// sizeof( d)
	const char* str,		// source
	const char* sOld,		// string to be replaced
	const char* sNew,		// string to replace with
							//   treat NULL as "" (= delete sOld)
	bool bCaseSens /*=false*/)	// true: case-sensitive sOld search
								// false: case-insensitive

// Replaces all occurances of sOld with sNew.
// Next search starts after each replacement.
// 
// returns -1 if destination too small
//            partial result returned (always \0 terminated)
//   else # of replacements (>= 0)
{
	if (!sNew)
		sNew = "";

	int lenNew = strlenInt(sNew);
	int lenOld = strlenInt(sOld);

	int replaceCount = 0;

	--dSize;	// space for \0

	while (1)
	{
		// Set pointer to the next occurance of sOld
		const char* pOld = bCaseSens
			? strstr(str, sOld)
			: stristr(str, sOld);
		if (!pOld) // No more sOld found, copy the rest of the string as is
		{
			bool bOK = memcpyPass(d, dSize, str, strlen(str));
			*d = '\0';		// space always availalbe
			if (!bOK)
				break;		// insufficient d space
			return replaceCount;		// good return
		}

		// copy source string up to sOld location
		if (!memcpyPass(d, dSize, str, pOld - str))
			break;	// insufficient d space
		str = pOld+lenOld;		// advance str beyond sOld

		// copy sNew
		if (!memcpyPass(d, dSize, sNew, lenNew))
			break;	// insufficient d space
		replaceCount++;
	}
	// only insufficient space errors get here
	return -1;
}		// strReplace
//-------------------------------------------------------------------------
int strReplace(			// replace variant
	char* s,		// string (modified in place)
	char cFrom,		// char to be replaced
	char cTo,		// char to replace (if \0, drop)
	int options /*=0*/)	// options
						//   1: drop instead of substitute
						//      if next char is WS
// useful to e.g. de-comma
// returns # of replaces / drops
{
	BOOL bDropIfWS = (options & 1) != 0;
	int i = 0;
	int j = 0;
	int count = 0;
	while (1)
	{
		s[ j] = s[ i++];
		if (s[ j] == '\0')
			break;
		else if	(s[ j] == cFrom)
		{
			count++;
			if (cTo == '\0' || (bDropIfWS && isspaceW( s[ i])))
				continue;
			s[ j] = cTo;
		}
		j++;
	}
	return count;
}	// strReplace2
//----------------------------------------------------------------------------
const char* stristr(					// case-insensitive string find
	const char * str1,		// string in which to search
	const char * str2)		// string to search for
// returns pointer within str1 to 1st char of substring matching str2
//         NULL if not found
{
	if (!str1 || !str2 || !*str2)
		return str1;		// empty, immediate match

	for (const char* cp = str1; *cp; cp++)
	{
		if (toupper( *cp) == toupper( *str2))
		{
			const char* s1 = cp;
			const char* s2 = str2;
			while ( *s1 && *s2 && toupper( *s1) == toupper( *s2))
				s1++, s2++;
			if (!*s2)
				return cp;	// all of str2 matched, we have a winner
			if (!*s1)
				break;		// hit end, no match possible
		}
	}
	return NULL;
}			// stristr
//-----------------------------------------------------------------------------
BOOL strMatch(					// string match
	const char* s1,
	const char* s2)

// returns TRUE iff trim( s1) == trim( s2), case-insensitive

{
	if (!s1 || !s2)
		return FALSE;

	s1 = strTrimB( s1);		// scan off leading ws
	s2 = strTrimB( s2);

	BOOL bIdentical = TRUE;
	BOOL bSeenDifSpace = FALSE;
	int c1 = intC( s1);
	int c2 = intC( s2);
	while (1)
	{	if (c1)
			c1 = toupper( intC( s1++));
		if (c2)
			c2 = toupper( intC( s2++));
		if (c1 == c2)
		{	if (!c1)		// if at end of both strings
				return bIdentical;
			if (!isspaceW( c1) && bSeenDifSpace)
				bIdentical = FALSE;
		}
		else if ((c1 && !isspaceW( c1)) || (c2 && !isspaceW( c2)) )
			return FALSE;
		else if (isspaceW( c1) && isspaceW( c2))
			bSeenDifSpace = TRUE;		// both space but different
	}
}			// strMatch
//-----------------------------------------------------------------------------
#if CSE_COMPILER != CSE_COMPILER_MSVC
int _stricmp(	// Substitude windows _stricmp functions
	const char* char1,	// First string to be compare
	const char* char2)	// Second string to be compare
// Compares two string ignoring case sensitivity
// Eventually replace this function with POSIX standard
{
	int sum{ 0 };
	for (;; char1++, char2++) {
		sum += tolower((unsigned char)*char1) - tolower((unsigned char)*char2);
		if (sum != 0 || *char1 == '\0' || *char2 == '\0') {
			return sum;
		}
	}
} // _stricmp
//-----------------------------------------------------------------------------
int _strnicmp(	// Substitude windows _strnicmp
	const char* char1,	// First string to be compare
	const char* char2,	// Second string to be compare
	size_t count)		// Number of characters to compare
// Compares two string ignoring case sensitivity upto the count.
// Eventually replace this function with POSIX standard
{
	int sum{ 0 };
	for (size_t i = 0; i < count; i++, char1++, char2++) {
		sum += tolower((unsigned char)*char1) - tolower((unsigned char)*char2);
		if (sum != 0 || *char1 == '\0' || *char2 == '\0') {
			return sum > 0? 1:-1;
		}
	}
	return 0;
}	// _stricmp
//-----------------------------------------------------------------------------
// TODO (MP) -- this works but certainly not elegant
char* _strupr(char* stringMod) // Substitude strupr function
// Converts a string to uppercase
{
	int i=0;
	while (stringMod[i])
	{
		stringMod[i]=toupper(stringMod[i]);
		i++;
	}
	return stringMod;
}	// _strupr
//-----------------------------------------------------------------------------
// TODO (MP) -- this works but certainly not elegant
char* _strlwr(char* stringMod) // Substitude strlwr function
// Converts a string to lowercase
{
	int i=0;
	while (stringMod[i])
	{
		stringMod[i]=tolower(stringMod[i]);
		i++;
	}
	return stringMod;
}	// _strlwr
#endif
//=============================================================================



/*************************************** if-outs *****************************************/

#ifdef wanted		// no calls 11-91; restore when use found
w//========================================================================
wchar * FC strntrim( s1, s2, n)
w
w/* Copy and trim white space from a portion of a string */
w
wchar *s1;	/* Destination area to receive trimmed string (if NULL, Tmpstr is used).
w		   Does NOT handle overlap case -- s1 must NOT be same as s2 */
wchar *s2;	/* String to be trimmed.  Note that s2 is temporarily modified by this routine, so it
w		   cannot be in a memory protected area.  This generally means that it cannot be a literal. */
wSI n;		// Maximum number of characters to trim; implicitly, there is a '\0' at *(s2+n).
w
w/* Returns s1 (pointer to destination string) */
w{
w char *s2end, c;
w
w    s2end = s2 + n;
w    c = *s2end;
w    *s2end = '\0';
w    s1 = strTrim( s1, s2);
w    *s2end = c;
w    return s1;
w }		// strntrim
#endif	// wanted

#ifdef wanted		// no calls 11-91; restore if use found
w//========================================================================
wchar * FC strtrend( s1, s2)
w
w/* Trim white space from end of a string */
w
wchar *s1;	/* Destination area to receive trimmed string
w		   Can be the same as S2 */
wchar *s2;	/* String to be trimmed */
w
w/* Returns s1 (pointer to destination string) */
w{
w char *p, *pend, c;
w
w    if (s1 == NULL)
w       s1 = strtemp( strlen(s2) );	/* alloc n+1 Tmpstr[] bytes. local. */
w    pend = p = s1;
w    while ((c = *s2++) != 0)
w    {	if (!isspaceW(c))
w			pend = p;
w       *p++ = c;
w	 }
w    *(++pend) = '\0';
w    return s1;
w}		/* strtrend */
#endif

#ifdef wanted		// no calls 11-91
w//========================================================================
wchar * FC strtrunc( str, maxlen, mode)
w
w/* truncates a string to len characters */
w
wchar *str;  /* string to truncate */
wSI maxlen;  /* maximum string length (length to truncate to) */
wSI mode;    /* STRTRIGHT: truncate starting from right (keep first char)
w	       STRTLEFT:  truncate starting from left (keep last char) */
w
w/* returns Tmpstr copy of truncated string */
w{
w SI len, plen, off;
w char *p;
w
w    len = strlen(str);
w    plen = min( len, maxlen);
w    p = strtemp(plen);			/* alloc n+1 Tmpstr[] bytes. local. */
w    if (mode == STRTRIGHT || (off = len - maxlen) < 0)
w       off = 0;
w    strncpy0( p, str+off, plen+1);	/* strncpy + always add null. strpak.*/
w    return p;
w }		/* strtrunc */
#endif

#ifdef wanted		// no calls 11-91
w//=============================================================================
wchar * FC strstrch( s1, s2, s3)
w
w/* Insert a string between each character pair of a string */
w
wchar *s1;    /* Destination string.  If NULL, result built in Tmpstr.
w		This can be the same as s2; overwrite is handled. */
wchar *s2;    /* Original string */
wchar *s3;    /* Insertion string */
w
w/* Returns pointer to result string. */
w{
w SI s1len, s2len, s3len;
w char *s1p, *s2p;
w
w    s2len = strlen(s2);
w    s3len = strlen(s3);
w    s1len = s2len==0 ? 0 : (s2len-1)*s3len + s2len;
w    if (s1 == NULL)
w       s1 = strtemp( s1len);	/* alloc n (+1) Tmpstr[] bytes. local. */
w    s1p = s1 + s1len;
w    s2p = s2 + s2len;
w    *s1p-- = *s2p--;
w    if (s2len > 0)
w    {	*s1p = *s2p;
w       while (s2p > s2)
w       {  s1p -= s3len;
w          memmove( s1p, s3, s3len);
w		   *(--s1p) = *(--s2p);
w		}
w	}
w    return s1;
w}		/* strstrch */
#endif

#ifdef wanted		// no calls 11-91
w//===========================================================================
wchar * FC strpadsp( dest, source, n)
w
w/* Pad a string to length n with spaces (or truncate to n) */
w
wchar *dest;    // destination string (if NULL Tmpstr is used)
wchar *source;  // source string (can be the same as dest or NULL for all spaces)
wSI n;	       // Length to pad to
w
w/* Returns pointer to dest. Source is copied to dest and if
w   strlen(source) < n,	' ' is appended to dest until n is reached.
w
w   Note: If n < length of source, source string is truncated to n chars */
w
w{
w SI i,ls;
w
w    ls = (source == NULL) ? 0 : strlen(source);
w
w    if (dest == NULL)
w    {
w  #ifdef DONT_TRUNCATE
w		if (ls > n)
w			i = ls;
w		else
w			i = n;
w  #endif
w		i = n;
w		dest = strtemp(i);		/* alloc i+1 bytes in Tmpstr. local. */
w	}
w   i = 0;
w	if (dest != source && source != NULL)
w    {
w       while  ((*(dest+i) = *source++) != 0)
w	  if (++i >= n)
w	     break;
w}
w#ifdef DONT_TRUNCATE
w    else
w       i = ls;
w#endif
w    else
w       i = (ls > n) ? n : ls;
w
w    while (i < n)
w       *(dest+i++) = ' ';
w    *(dest+i) = '\0';
w    return dest;
w}		/* strpadsp */
#endif

#ifdef wanted		// no callers 11-91.  see C libary fcn strstr().
w//========================================================================
wchar * FC strindx( s1, s2)
w
w/* searches s1 for first occurence of s2.  if s1==NULL, the next
w   occurence in the last non NULL string is sought */
w
wchar *s1;
wchar *s2;
w
w/* returns pointer to matching string in s1.  NULL if no match */
w{
w static char *lasts1=NULL; char *tmp; SI ls1, ls2, i;
w
w    if (s1 == NULL)
w    {	if (lasts1 == NULL)
w			return NULL;
w       s1 = lasts1 + 1;
w	}
w
w    ls1 = strlen(s1);
w    ls2 = strlen(s2);
w    tmp = strtemp(ls2); 	/* alloc n+1 Tmpstr[] bytes. local. */
w    *(tmp+ls2) = 0;
w    for (i = 0; i <= ls1-ls2; i++)
w    {	memcpy( tmp, s1+i, ls2);
w       if (strcmp( tmp, s2) == 0)
w       {	lasts1 = s1+i;
w			return lasts1;
w		}
w	}
w    lasts1 = NULL;
w    return NULL;
w
}		/* strindx */
#endif

// end of strpak.cpp
