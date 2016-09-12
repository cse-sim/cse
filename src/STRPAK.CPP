// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* strpak.c: String functions above and beyond standard C library
	     Except as noted, these routines have specs identical to Microsoft library routines */

/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>

#include <messages.h>	// MAX_MSGLEN msgI

// #include <strpak.h>	// decls for this file (#included in cnglob.h)

#if defined( _DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*------------------------------- VARIABLES -------------------------------*/
// General temporary string buffer.  Many uses, via strtemp().
static int TmpstrNx = 0;	// Next available byte in Tmpstr[].
/*--------------------------- PUBLIC VARIABLES ----------------------------*/
// temporary string buffer and access macro
const int TMPSTRSZ = 200000;    		// size of Temp str buffer Tmpstr[].
static char Tmpstr[ TMPSTRSZ+2];	// buffer.  When full, buffer is re-used from start.
// Each allocation is followed by prior TmpstrNx value for rev-order dealloc (strtempPop).
// TMPSTRSZ: strpak.h.  +2 extra bytes at end hold flag re overwrite check (obsolete? 7-10)
// see envpak.cpp

/*=============================== TEST CODE ================================*/
/* #define TEST */
#ifdef TEST
t
t main()
t{
t SI i, off;
t static char s[100]="This is a test";
t char *snake, *p, *ptok;
t
t
t #ifdef PATHPARTSTEST
t     p = "C:\\bob\\DOG.X";
t     printf( "Starting with: %s\n", p);
t     for (i = 0; i < 16; i++)
t        printf( "  %x  [%s]\n", (UI)i, strpathparts( p, i));
t #endif
t
t #ifdef OTHERTESTS
t     i = 0;
t loop:
t     printf( "'%s' '%s'\n", strncpy0( NULL, s+i, 3), strntrim( NULL, s+i, 3) );
t     goto loop;
t #endif
t
t #define XTOKTEST
t #ifdef XTOKTEST
t     p = "  Token  / ,  test ing,,1      ";
t     printf("\n[%s]\n",p);
t     while (ptok = strxtok( NULL, &p, " ,/", TRUE))
t        printf( "[%s]  %p\n", ptok, p);
t #endif /* XTOKTEST */
t}
#endif /* TEST */

//-----------------------------------------------------------------------------
int strCheckPtr( DMP p)		// check DM ptr for strpak conflict
{
	return !(p >= Tmpstr && p <= Tmpstr + TMPSTRSZ);
}	// strCheckPtr
// ================================================================
char* strxtok( 		// Extract a token from a string

	char *tokbuf,		// Pointer to buffer to receive extracted token, or NULL, to use Tmpstr.
	const char** p,		// Pointer to string pointer.
    					//  *p is the starting point for scan and is updated to be the start point for next call.
    					//  Successive calls will retrieve successive tokens.
	const char* delims,	// String of delimiter characters ('\0' terminated).
	int wsflag )		// White space flag.
					    //  TRUE:  White space around delimiters is ignored (and trimmed from returned token).
						//         WS is also taken to be a secondary delimiter.
						//	FALSE: WS is treated as standard character.
						//         WS chars can then be included in delims string if desired.

// This routine is similar in function to Microsoft strtok, but has more friendly specs.
//   The MS routine keeps an internal pointer to the working string, making nested calls impossible.
//   NEVER USE STRTOK!

// Returns NULL if no more tokens in string.  Otherwise, returns pointer to extracted token.
{
	int i;
	char c;
	int ib, ie, endscan, ltok;
	const char *ps;

	ib = ie = -1;
	ps = *p;
	i = 0;
	endscan = FALSE;
	while ((c = *(ps+i)) != 0)
	{
		if (isspaceW(c) && wsflag)
			endscan = (ib != -1);
		else if (strchr( delims, c) != NULL)
		{
			i++;
			break;
		}
		else
		{
			if (endscan)
				break;
			if (ib == -1)
				ib = i;
			ie = i;
		}
		i++;
	}  /* Scan loop */

// Set up return
	*p = ps+i;
	if (ib==-1)
	{
		if (c=='\0')
			return NULL;
		else
			ltok = 0;
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
void * FC memsetPass( void * * pdest, char c, USI n)		// memset and point past.  Rob 1-92.
{
	memset( *pdest, c, n);
	IncP( pdest, n);
	return *pdest;
}		// memsetPass
// ================================================================
void * FC memcpyPass( void * * pdest, const void * src, USI n)		// memcpy and advance destination pointer past, 1-92.
{
	memcpy( *pdest, src, n);
	IncP( pdest, n);
	return *pdest;
}		// memcpyPass
// ====================================================================
char* strncpy0(			// copy/truncate with 0 fill
	char* d,		// destination or NULL to use Tmpstr[]
	const char* s,	// source
	size_t l)		// dim( d) NOTE spec change from prior version!
					// if 0, d is not altered
{
	if (!d)
		d = strtemp( l-1);		// allocate l bytes in Tmpstr
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
#if 1
char* FC strTrim( 			// trim white space from beginning and end of a string
	char* s1,			// destination (can be the same as s2), or NULL to use Tmpstr
	const char* s2,		// string to be trimmed
	int s2len/*=999999*/)	// s2 max length (stops at shorter of s2len or \0)

// Returns s1 (pointer to destination string)
{
	if (s1 == NULL)
		s1 = strtemp( s2len < 999999 ? s2len : strlen( s2) );	// alloc n+1 Tmpstr[] bytes. local.

	char *p, *pend, c;
	p = pend = s1;
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
	int l = strlen( s);
	while (l && isspaceW(*( s+l-1)) )
		l--;
	return strncpy0( d, s, l+1);
}		// strTrimE
//----------------------------------------------------------------------------
char* strTrimEX(	// deblank / clean end of string
	char* d,			// destination (NULL = tmpstr)
	const char* s,		// source
	const char* ex)		// addition chars to drop
//  if d == s, trims in place
{
	int l = strlen( s);
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
	int l = strlen( s);
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
#else
char* FC strtrim( 		// Trim white space from beginning and end of a string

	char *s1,		 // destination (can be the same as S2), or NULL to use Tmpstr
	const char *s2 ) // string to be trimmed

// Returns s1 (pointer to destination string)
{
	char *p, *pend, c;

	if (s1 == NULL)
		s1 = strtemp( strlen(s2) );	// alloc n+1 Tmpstr[] bytes. local.
	p = pend = s1;
	int flag = FALSE;
	while ((c = *s2++) != 0)
	{
		if (!isspaceW(c))
		{
			flag = TRUE;
			pend = p+1;
		}
		if (flag)
			*p++ = c;
	}
	*pend = '\0';
	return s1;
}		// strtrim
#endif
// ====================================================================
int strLineLen( const char *s)		// line length including \n (# chars thru 1st newline or to \0)  rob 11-91

// related problem: line body length, excluding \n: use strcspn( s, "\r\f\n") inline.
{
	int len = strcspn( s, "\n");		// # chars TO \n or \0
	if (*(s + len)=='\n')			// if \n is present
		len++; 					// include it in count also
	return len;
}		// strLineLen
// =========================================================================
int strJoinLen( const char *s1, const char *s2)	// length of line formed if strings joined

/* return length of line that would be formed where strings were concatentated, allowing for \n's in the strings,
   eg for deciding whether to insert \n between them when concatenating. */
{
	const char* u = strrchr( (char *)s1, '\n');	// NULL or ptr to last newline
	return
		strlen( u ? u+1 : s1)     	// length LAST line of s1
		+  strcspn( s2, "\r\n");		// length FIRST line of s2
}				// strJoinLen
// =======================================================================
char * FC strpad( 		// Pad a string to length n in place

	char *s,		// String to pad
	const char *pads,	// Pad string: typically " "; any length & different chars ok
	SI n )			// Length to pad to

/* Returns pointer to s.  If strlen(s) >= n, no action is performed.
   Otherwise, pads is appended to s until n is reached. */
{
	register SI i, j;
	SI lp;

	i = strlen(s);
	lp = strlen(pads);
	j = -1;
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
char * FC strffix( 			// put a filename in canonical form

	const char *name, 	// input filname
	const char *ext ) 	// default extension including period

// returns uppercase filename with extension in Tmpstr[]
{
	char *nu, *lastslsh;

	lastslsh = strrchr( (char *)name, '\\');
	if (strrchr( name, '.') <= lastslsh)
		nu = strtcat( name, ext, NULL);
	else
		nu = strtcat( name, NULL);
	strTrim( nu, strupr(nu));
	return nu;
}		// strffix
//-------------------------------------------------------------------
char* strffix2( 			// put a filename in canonical form (variant)
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
	{	char curExt[ _MAX_EXT];
		_splitpath( name, NULL, NULL, NULL, curExt);
		bAddExt = IsBlank( curExt);
	}

	char* nu = strTrim( NULL, name);	// trim to TmpStr
	if (bAddExt)
		nu = strtcat( nu, strTrim( NULL, ext), NULL);
	return nu;
}		// strffix2
// ====================================================================
char * FC strtPathCat( 		// concatenate file name onto path, adding intervening \ if needed

	const char* path, 		// drive and/or directorie(s) path
	const char* namExt)  	// file name or name.ext

// returns assembled file pathname in Tmpstr[]
{
	char *addMe;

	int len = strlen(path);
	char pathLast = path[len-1];
	if (len > 0	&&  pathLast != ':' && pathLast != '\\')
		addMe = "\\";
	else 		/* path ending in \ or :, or null path, needs no \ */
		addMe = "";
	return strtcat( path, addMe, namExt, NULL);
}						// strtPathCat
// ====================================================================
char* FC strpathparts( 	// Build string from parts of a path name (for default file names etc)

	const char* path,	// Full (valid) pathname or filename
	int partswanted,	// Bits indicating which path parts are to be included in result --
						//   STRPPDRIVE:     Drive
						//   STRPPDIR:       Directory
						//   STRPPFNAME:     File name (w/o extension)
						// STRPPEXT:       Extension (including '.')
	char* pcombo/*=NULL*/)	// buffer [_MAX_PATH] for result
							//   NULL = use TmpStr

// Example: strpathparts( path, STRPPFNAME+STRPPEXT) will return name+extension found in path.

// Returns assembled string, equested parts which are not present in *path are omitted from result.
{
	char pbuf[_MAX_PATH*4];
	static char wantmask[4] = { STRPPDRIVE, STRPPDIR, STRPPFNAME, STRPPEXT };

#define part(p) (pbuf+((p)*_MAX_PATH))

	_splitpath( path, part(0), part(1), part(2), part(3));	/* msc lib */

	if (!pcombo)
		pcombo = strtemp( _MAX_PATH);	// alloc n+1 Tmpstr[] bytes. local
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
	char *p, *t, c;

// count \'s, return input if none

	SI nBs = 0;			// # chars to put \ b4
	p = s;
	while ((p = strchr( p, '\\')) != 0)	// find next \, end loop if none
	{
		nBs++;				// count \'s
		p++;				// continue search at next char
	}
	if (!nBs)				// if no \'s
		return s;			// input is ok as is. normal exit.

// copy to Tmpstr, doubling \'s

	p = t = strtemp( strlen(s) + nBs);	// alloc n+1 Tmpstr[] bytes. local.
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
	register SI is = 0, ir = 0;			// source, result offsets

	/*lint -e720 boolean test of assignment */
	while ( (*(s+ir++) = *(s+is)) != 0 )	// move char AND test for null term
		/*lint +e720 */
	{
		if (    *(s+is) == '\\'			// if this and next are '\'
				&& *(s+is+1) == '\\')
			is++;					// extra advance
		is++;					// normal advance
	}
	return s;
}		// strBsUnDouble

// ====================================================================
char* FC strsave(		// save a copy of a string in heap
	const char *s )	// NULL or pointer to character string.
// Returns pointer to saved string, or NULL if argument is NULL
{
	char *p;

	if (!s)					// for convenience,
		return NULL;				// strsave(NULL) is NULL.  rob 11-91.
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

	SI n )	// number of bytes needed, less 1 (1 added here eg for null, 9-89)

// returns pointer to space in Tmpstr : Valid for a (very) short while

{
	n++;					// for null: saves many +1's in calls
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
		goto ret;

// fetch TmpstrNx value b4 last strtemp
	int priorNx = *(int *)(Tmpstr + TmpstrNx - sizeof( int));
	if (priorNx < 0 || priorNx > TMPSTRSZ)	// nop if out of range
		goto ret;				// insurance re bugs or whole Tmpstr[]'s worth of deallocs
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
ret:
	return anS;			// return location of (vulnerable!) string: strUntemp thus can be nested in call using anS.
}		// strtempPop
// ======================================================================
char * FC strtmp( const char *s)		// make a copy of string in Tmpstr[] and return pointer to it

{
	return strtcat( s, NULL);
}				// strtmp
// ====================================================================
char * CDEC strtcat( const char *s, ... )	// concatenate variable number of strings in Tmpstr

// last arg must be NULL

// returns pointer to Tmpstr ... do a strsav for permanent copy !!
{
	SI len=0;
	char *p;
	va_list arg;
	const char *padd;

// determine total length
	padd = s;				// first arg
	va_start(arg, s);			// set up to get subsequent args
	while (padd)			// NULL arg terminates
	{
		len += strlen(padd);
		padd = va_arg( arg, char *);
	}

// allocate destination space
	p = strtemp(len);    		// alloc len+1 Tmpstr[] bytes. local.
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
char * CDEC strntcat( SI n, ... )	// concatenate variable number of strings up to a maximum length in Tmpstr

/* SI n;  	* maximum length of string */
/* ...  	* strings to concat, terminated by NULL */

// returns pointer to Tmpstr ... do a strsav for permanent copy !! */
{
	char *p;
	va_list arg;
	char *padd;

	p = strtemp(n);		// alloc n+1 Tmpstr[] bytes. local.
	*p = '\0';			// init to null string for concatenation

	va_start( arg, n);
	while ((padd = va_arg( arg, char *)) != NULL)
	{
		strncat( p, padd, n);
		if ((n -= strlen(padd)) <= 0)
			break;
	}
	return p;
}		// strntcat
//===========================================================================
const char* FC scWrapIf( const char* s1, const char* s2, const char* tweenIf)

// concatenate strings to Tmpstr, with text "tweenIf" between them if overlong line would otherwise result
{
	USI maxll = getCpl() - 3;
	// -3: leave some space for adding punctuation, indent, etc.
	return strtcat( s1,
					strJoinLen( s1, s2) 		// length of line if joined
					> maxll  ?  tweenIf  :  "",
					s2,
					NULL );
}	// scWrapIf
// ======================================================================
const char* CDEC strtprintf( 	// make like sprintf and return pointer to result in tmpstr
	const char* mOrH, ...)	// format string or message handle
{
	va_list ap;
	va_start( ap, mOrH);
	return strtvprintf( mOrH, ap);
}			// strtprintf
// ======================================================================
const char* FC strtvprintf( 	// make like vsprintf and return pointer to result in tmpstr.
	const char * mOrH, va_list ap)	// format string or message handle
{
	char buf[ MSG_MAXLEN];

	msgI( WRN, buf, sizeof( buf), NULL, mOrH, ap);	// gets text if handle,
								// checks length, vprintf's to buf.  returns rc.
	return strtmp( buf);
}			// strtvprintf
// ======================================================================
WStr WStrPrintf( 	// make like sprintf and return pointer to result in XSTR
	const char* mOrH, ...)		// format string or message hangle
{
	va_list ap;
	va_start( ap, mOrH);
	return WStrVprintf( mOrH, ap);
}			// WStrPrintf
// ======================================================================
WStr WStrVprintf(	// make like vsprintf and return pointer to result in tmpstr.
	const char* mOrH,		// format string or message handle
	va_list ap /*=NULL*/)					// arg list
{
	char buf[ MSG_MAXLEN];

	msgI( WRN, buf, sizeof( buf), NULL, mOrH, ap);	// gets text if handle,
								// checks length, vprintf's to buf.  returns rc.
	WStr t = buf;
	return t;
}			// WStrVprintf
// ======================================================================
SI FC strlstin(			// case insensitive search for string in list

	const char *list,	// list of strings delimited by " ,/"
	const char *str )	// string to search for in list, len > 0

// recoded w/o test to eliminate call to template matcher strtnmtch
//  if wildcard matching needed, TK/CR version must be resurrected, 9-29-91

// returns nz if str is found in list, else 0
{
	int llist = strlen( list);
	int lstr  = strlen( str);
	int off = 0;
	int match = 0;
	while (off < llist)
	{
		int lsub = strcspn( list+off, " ,/");	// find next delim
		if (lsub
		 && lsub == lstr
		 && !strnicmp( str, list+off, lsub) )
		{
			match = 1;
			break;
		}
		off += lsub + 1;
	}
	return match;
}		// strlstin


#if 0
///////////////////////////////////////////////////////////////////////////
// class CWString
///////////////////////////////////////////////////////////////////////////
CWString CWString::Set(			// set CWString from char[]
	const char* s,			// char array
	int len /*=INT_MAX*/)	// max # of chars to copy
//   if INT_MAX, use length (added 3-29-05)
{
	if (!s || len <=0)
		Empty();
	else
	{
		if (len == INT_MAX)
			len = strlen( s);
		strncpy0( GetBuffer( len), s, len+1);
		ReleaseBuffer( len);
	}
	return *this;
}		// CWString::Set
//------------------------------------------------------------------------
CWString CWString::Fill(			// fill CWString
	const char* s,		// source string, repeatedly copied until len
	int len /*=-1*/)	// required length, -1 = use current
{
	if (len < 0)
		len = GetLength();
	strFill( GetBuffer( len), s, len);
	ReleaseBuffer( len);
	return *this;
}			// CWString::Fill
//------------------------------------------------------------------------
CWString CWString::FormatV(			// expose useful CString protected mbr
	//   and return self
	LPCTSTR lpszFormat,
	va_list arglist)			// args (if NULL, no format)
// returns self

{
	if (!lpszFormat || !arglist)
		*this = lpszFormat;		// no FormatV, NULL OK (yields empty)
	else
		CString::FormatV( lpszFormat, arglist);
	return *this;
}			// CWString::FormatV
//------------------------------------------------------------------------
CWString CWString::Format( LPCTSTR lpszFormat, ...)
{
	va_list argList;
	va_start( argList, lpszFormat);
	return FormatV( lpszFormat, argList);
}			// CWString::Format
//------------------------------------------------------------------------
CWString CWString::Format( UINT nFormatID, ...)
{
	CString strFormat;
	VERIFY( strFormat.LoadString( nFormatID) != 0);
	va_list argList;
	va_start( argList, nFormatID);
	return FormatV( strFormat, argList);
}			// CWString::Format
//------------------------------------------------------------------------
CWString CWString::FmtIfV(				// conditional Format
	LPCTSTR fmtNonBlank,		// format to use if arg1 non-blank
	LPCTSTR fmtBlank,			// format to use if arg1 is blank
	va_list argList)			// args for format, 1st assumed char *
{
	va_list aL2 = argList;
	const char* arg1 = va_arg( aL2, const char *);
	return FormatV( !::IsBlank( arg1) ? fmtNonBlank : fmtBlank, argList);
}			// CWString::FmtIfV
//------------------------------------------------------------------------
CWString CWString::FmtIf(				// conditional Format
	LPCTSTR fmtNonBlank,		// format to use if arg1 non-blank
	LPCTSTR fmtBlank,			// format to use if arg1 is blank
	...)						// args for format, 1st assumed char *
{
	va_list argList;
	va_start( argList, fmtBlank);
	FmtIfV( fmtNonBlank, fmtBlank, argList);
	return *this;
}			// CWString::FmtIf
//---------------------------------------------------------------------------
CWString CWString::FmtForWhat(		// format msg with optional "for what"
	const char* what,		// if non-blank, "for <what>" appended to msg
	const char* fmt, ...)
{
	va_list ap;
	va_start( ap, fmt);
	FormatV( fmt, ap);
	CatIf( what, " for ");
	return *this;
}		// CWString::FmtForWhat
//------------------------------------------------------------------------
CWString CWString::FmtFloatDTZ(		// format float / strip trailing 0s
	double v,				// value
	int maxPrec,			// max precision (used for initial format)
	int minPrec /*=0*/)		// min precision, if 0, '.' deleted also
{
	Format( "%0.*f", maxPrec, v);
	return DropTrailing0s( minPrec);
}		// CWString::FmtFloatDTZ
//------------------------------------------------------------------------
CWString CWString::DropTrailing0s(		// drop trailing 0s (in place)
	int minPrec /*=0*/)		// min precision, if 0, '.' deleted also
// returns *this with trailing 0s removed
{
	int len = GetLength();
	int newLen = len;	// candidate new length
	for (int i = len; --i >= 0; )
	{
		int c = GetAt( i);
		if (c == '0')
		{
			if (newLen == i+1)
				newLen = i;
		}
		else if (c == '.')
		{
			if (newLen <= i + minPrec)
				newLen = i + minPrec + 1;
			else if (!minPrec && newLen == i+1)
				newLen = i;		// drop decimal too
			if (newLen == 0)
				*this = "0";	// string was ".0000"
			else
				Delete( newLen, len-newLen);
			break;
		}
		else if (newLen == len)
			break;		// no trailing 0s, give up
		// else keep scanning
	}
	// never hit decimal point, do nothing
	return *this;
}			// CWString::DropTrailing0s
//------------------------------------------------------------------------
CWString CWString::CSV( const char* s)		// append string to CSV line
{
	if (!IsEmpty())
		*this += ',';
	*this += s;
	return *this;
}		// CWString::CSV
//------------------------------------------------------------------------
CWString CWString::CSV(						// append double to CSV line
	double v, int maxPrec, int minPrec/*=0*/)
{
	return CSV( tFmtFloatDTZ( v, maxPrec, minPrec));
}		// CWString::CSV
//------------------------------------------------------------------------
CWString CWString::CSV(						// append float to CSV line
	float v, int maxPrec, int minPrec/*=0*/)
{
	return CSV( tFmtFloatDTZ( v, maxPrec, minPrec));
}		// CWString::CSV
//------------------------------------------------------------------------
CWString CWString::CSV( int v)				// append int to CSV line
// append double to CSV line
{
	return CSV( tFormat( "%d", v));
}		// CWString::CSV
//------------------------------------------------------------------------
CWString CWString::PathParts(			// return portions of full path

	int parts) const		// bits = parts desired
//    8 = drive	or UNC share
//    4 = dir
//    2 = file (aka "title")
//    1 = ext

// NOTE: combinations 9, 10, 11 (share w/o dir) not meaningful for UNC

// returns reassembled portions
//		e.g. if x = "c:\path\file.ext", x.PathParts( 6) = "\path\file"
{

	char drive[ _MAX_DRIVE];
	char dir[ _MAX_DIR];
	char file[ _MAX_FNAME];
	char ext[ _MAX_EXT];

	CWString t( *this);
	t.Trim();
	int len = t.GetLength();
	int partsSav;
	CWString uncDrive;
	if (len >= 2 && t.Left( 2) == "\\\\")		// if UNC
	{
		int bs1 = t.Find( '\\', 2);
		int bs2 = bs1 > 0 ? t.Find( '\\', bs1+1) : -1;
		int split = bs2 < 0 ? len : bs2;
		uncDrive = t.Left( split);
		t = t.Right( len - split);
		partsSav = parts;
		parts &= 7;			// drop drive
	}
	else
		partsSav = 0;

	_splitpath( t, drive, dir, file, ext);

	char assembly[ _MAX_PATH];
	_makepath( assembly,
			   (parts & 8) ? drive : NULL,
			   (parts & 4) ? dir   : NULL,
			   (parts & 2) ? file  : NULL,
			   (parts & 1) ? ext   : NULL);
	return (partsSav & 8)
		   ? uncDrive + assembly
		   : assembly;
}					// CWString::PathParts
//-------------------------------------------------------------------------
int CWString::SplitPfx(			// split delimited prefix
	CWString& sPfx,				// returned: prefix section
	CWString& sMain,			// returned: remaining section
	const char* delim /*="|"*/) const		// delimiter
// handy equivalent to 1 or 2 Piece calls

// returns -1: bad call (no delim etc)
//          0: no prefix found (sMain = this)
//          1: prefix found (sPfx = prefix, sMain = remainder (after delim)
{
	int ret = -1;
	if (delim && *delim)		// if meaningful delimiter
	{
		int iDelim = Find( delim);
		if (iDelim >= 0)
		{
			sPfx = Left( iDelim);
			sMain = Mid( iDelim+strlen( delim));
			ret = 1;		// prefix found
		}
		else
		{
			sPfx.Empty();
			sMain = *this;
			ret = 0;		// no prefix
		}
	}
	else
	{
		sPfx.Empty();
		sMain.Empty();
	}
	return ret;
}		// CWString::SplitPfx
//-------------------------------------------------------------------------
BOOL CWString::Piece(			// return a delimited piece of a string

	CWString& sRet,				// returned string pieces
	int iB,						// idx of 1st piece (0-based)
	const char* delim /*="|"*/,	// delimiter: generally single char but
	//   can be anything
	int iE /*=-1*/,				// idx of last piece, -1 = same as 1st
	//   (rarely used)
	int* pNPRet /*=NULL*/) const	// optional return: # of pieces in sRet
//   (avoids caller PieceCount() in some cases)

// Example: given "aaaa.bbbb.cccc.d.e",
//    Piece( t, 0, "."), t = "aaaa"
//    Piece( t, 3, "."), = "d"
//    Piece( t, 99, "."), t = "" (returns FALSE)
//    Piece( t, 2, ".", 3), t = "cccc.d"

// returns FALSE if piece iB is beyond end of string, sRet = ""
//    else TRUE and sRet = string piece(s)
{
// out-of-bounds requests return sensible things
	if (iB < 0)
		iB = 0;
	if (iE < iB)
		iE = iB;

	int i1 = 0;			// 0-based char pos of beg of 1st piece
	int n = 0;			// length of chunk to return

	const char* pStr = *this;	// ptr to this string
	int lStr = GetLength();
	BOOL bFound = FALSE;
	int nPRet = 0;
	if (delim && *delim && pStr && lStr)		// if meaningful
	{
		const char* p1 = pStr;
		const char* pNew;
		int lDelim = strlen( delim);

		bFound = TRUE;
		int i;
		for (i=0; i < iB; i++)	  	// scan p1 for beg of 1st piece
		{
			pNew = strstr( p1, delim);
			if (pNew)
				p1 = pNew + lDelim;
			else
			{
				bFound = FALSE;			// 1st piece is beyond end of str
				break;
			}
		}

		if (bFound)			// if found piece iB
		{
			const char* p2 = p1;
			for ( ; i <= iE; i++)
			{
				pNew = strstr( p2, delim);
				nPRet++;
				if (pNew)
				{
					p2 = pNew + lDelim;
				}
				else
				{
					p2 = pStr + lStr + lDelim;
					break;
				}
			}
			i1 = p1 - pStr;
			n  = p2 - p1 - lDelim;
		}
	}
	sRet = Mid(i1, n);
	if (pNPRet)
		*pNPRet = nPRet;
	return bFound;
}					// CWString::Piece
//---------------------------------------------------------------------------
int CWString::PieceCount(			// count pieces in piece string
	const char* delim /*="|"*/,		// delimiter: generally single char but
	//   can be anything
	int* pNNonBlank /*=NULL*/) const // if non-NULL, return # of non-blank
//   pieces here

// returns # of pieces and optionally # of non-blank pieces
{
	int n = 0;
	int nNonBlank = 0;
	CWString pc;
	while ( Piece( pc, n, delim))
	{
		if (!pc.IsBlank())
			nNonBlank++;
		n++;
	}
	if (pNNonBlank)
		*pNNonBlank = nNonBlank;
	return n;
}		// CWString::PieceCount
//---------------------------------------------------------------------------
/*static*/ int CWString::PieceFindHelper(  	// PieceFind() internal fcn
	const char* s,	   			// delimited list of strings to search
	//   e.g. "str1|str2|str3"
	const char* sFind,			// string sought
	const char* delim/*="|"*/,	// delimiter: generally single char but
	//   can be anything
	int brk/*=-1*/,				// break char: alters match rules
	//   -2: beg with 1: match length of s
	//   -1: exact piece match required
	//    0: beg with 2: match length of piece
	//   >0: prefix: match part before brk
	CWString* pFound/*=NULL*/)	// if non-NULL, entire found piece
//   is returned here

// WHY: this function supports PieceFind() (= find string=this in list)
//      and PieceFind2() (= find string in list=this)
// performs case-insensitive compare of this against each piece of s
//    PieceFind( "XX|ZZZ|FT", "FT") returns 2
//    PieceFind( "XX-3|FT-2|FTX", "FT", "|", '-') returns 1
//    s.PieceFind( "A|C|E|F", "FT", "|", 0) returns 3
// returns 0-based idx of found piece
//        -1 if not found
{
	int lFind = sFind ? strlen( sFind) : 0;
	int lCmp = lFind;		// length to compare
	if (!delim || !*delim || !s || !*s || !sFind || !lFind)	// if meaningful
		return -1;

	int lS = strlen( s);
	int lDelim = strlen( delim);
	const char* p1 = s;
	for (int iP=0;  ; iP++)
	{
		const char* pNew = strstr( p1, delim);		// ptr to next delim
		if (!pNew)
			pNew = s + lS;					// at end: pt to '\0'
		int lP = pNew - p1;					// length of piece
		if (!brk)
			lCmp = min( lFind, lP);
		if (lP >= lCmp						// if piece big enuf to match
				&& !_strnicmp( p1, sFind, lCmp))	// and it matches
		{
			if (lP == lCmp					// if identical
					|| (*(p1+lFind) == brk)		// or next char is brk
					|| brk == -2)					// or partial match
			{
				if (pFound)
				{
					memcpy( pFound->GetBuffer( lP), p1, lP);
					pFound->ReleaseBuffer( lP);
				}
				return iP;
			}
		}
		if (!*pNew)						// if no more
			return -1;					//   not found
		p1 = pNew + lDelim;			// move past delim and try next one
	}
}				// CWString::PieceFindHelper
//---------------------------------------------------------------------------
CWString CWString::PieceDelimPad(		// helper: add delimiters if needed
	int nPReq,						// required # of pieces
	int nPHave /*=-1*/,				// current # of pieces (-1 = not known)
	const char* delim /*="|"*/)		// delimiter: generally single char but
//   can be anything
// returns modified string
{
	if (nPHave < 0)
		nPHave = PieceCount( delim);
	int nPAdd = nPReq - nPHave;	// # to be added
	for (int iPAdd=0; iPAdd < nPAdd; iPAdd++)
		*this += delim;
	return *this;
}		// CWString::PieceDelimPad
//---------------------------------------------------------------------------------
CWString CWString::PieceSet(  			// set (overwrite or add) piece in this string
	const char* s,					// string to add (NULL treated as "")
	int iB,							// piece to set
	//   if beyond end, append with add'l delims
	//   if <0, append w/o add'l delims
	const char* delim /*="|"*/,		// delimiter: generally single char but
	//   can be anything
	int iE /*=-1*/)					// idx of last piece, -1 = same as 1st
//   (rarely used)

// returns modified string
{
	if (!s)
		s = "";
	BOOL bPad = TRUE;
	if (iB < 0)
	{
		bPad = FALSE;
		iB = 999999;
	}
	if (iE < 0)
		iE = iB;
	CWString beg;
	int nPBeg = 0;		// # of pieces in beg
	if (iB > 0)
		Piece( beg, 0, delim, iB-1, &nPBeg);
	CWString end;
	if (!Piece( end, iE+1, delim, 999999))
	{
		// beyond end
		if (bPad)
			beg.PieceDelimPad( iB, nPBeg, delim);
	}
	*this = beg;
	if (!IsEmpty())
		*this += delim;
	*this += s;
	if (!end.IsEmpty())
		*this += delim + end;
	return *this;
}			// CWString::PieceSet
//---------------------------------------------------------------------------------
CWString CWString::PieceDel(  			// delete piece(s)
	int iB,							// idx of 1st piece to delete (0-based)
	//   if beyond end, NOP
	const char* delim /*="|"*/,		// delimiter: generally single char but
	//   can be anything
	int iE /*=-1*/)					// idx of last piece, -1 = same as 1st
//   (rarely used)
// returns modified string
{
	if (iE < 0)
		iE = iB;
	CWString beg;
	if (iB > 0)
		beg = Piece( 0, delim, iB-1);
	CWString end( Piece( iE+1, delim, 999999));
	*this = beg;
	if (!end.IsEmpty())
		*this += delim + end;
	return *this;
}			// CWString::PieceDel
//---------------------------------------------------------------------------
CWString CWString::PieceAdd(  			// add/insert a piece to this string
	const char* s,				// string to add (NULL treated as "")
	int iB/*=-3*/,				// where to add
	//   -3: append w/o delimiter pad
	//   -2: descending order (s inserted bef
	//       1st piece <= s)
	//   -1: ascending order (s inserted bef
	//		 1st piece >= s)
	//  >=0: at piece iB (0 based); if iB beyond end, s is
	//       appended with add'l delimiters
	const char* delim /*="|"*/)		// delimiter: generally single char but
//   can be anything
// returns modified string
{
	if (!s)
		s = "";
	BOOL bPad = TRUE;		// pad with delimiters
	if (iB == -3)
	{
		bPad = FALSE;	// append: suppress pad
		iB = 999999;
	}
	else if (iB < 0)
	{
		BOOL bAscend = iB == -1;
		CWString p;
		for( iB=0; Piece( p, iB, delim); iB++)
		{
			if (bAscend)
			{
				if (s <= p)
					break;
			}
			else if (s >= p)
				break;
		}
	}
	CWString beg;
	int nPBeg = 0;		// # of pieces in beg
	if (iB > 0)
		Piece( beg, 0, delim, iB-1, &nPBeg);
#if 1	// recode: handle s="" and add DelimPad, 9-28-06
	CWString end;
	if (!Piece( end, iB, delim, 999999))
	{
		// beyond end
		if (bPad)
			beg.PieceDelimPad( iB, nPBeg, delim);
	}
	*this = beg;
	if (!IsEmpty())
		*this += delim;
	*this += s;
	if (!end.IsEmpty())
		*this += delim + end;
	return *this;
#else
x	CWString end = Piece( iB, delim, 9999);
x
x	*this = beg;
x	CatIf( s, delim);
x	return CatIf( end, delim);
#endif
}			// CWString::PieceAdd
//---------------------------------------------------------------------------------
BOOL CWString::PieceAddUniq(  			// add/insert piece iff not already there
	const char* s,					// string to add
	int iB/*=-3*/,					// where to add
	//   -3: append w/o add'l delimiters
	//   -2: descending order (s inserted bef
	//       1st piece <= s)
	//   -1: ascending order (s inserted bef
	//		 1st piece >= s)
	//  >=0: at piece iB (0 based); if iB beyond end, s is
	//       appended with add'l delimiters
	const char* delim /*="|"*/)		// delimiter: generally single char but
//   can be anything
//returns TRUE if added, otherwise FALSE (already exist)
{
	if (PieceFind2( s, delim) >=0 )
		return FALSE;
	PieceAdd( s, iB, delim);
	return TRUE;
}			// CWString::PieceAddUniq
//---------------------------------------------------------------------------
CWString CWString::PieceStringFromNullSeparated(
	const char* s,		// \0 separated string list
	//   (as returned by e.g. GetPrivateProfileSectionNames())
	int len/*=-1*/,			// length of s, if <0, s is terminated
	//	by \0\0
	const char* delim /*="|"*/)		// delimiter: generally single char
//   but can be any non-blank string
{
	Empty();
	const char* pS = s;
	for (int iB=0; (len < 0 && *s) || pS < s+len; iB++)
	{
		PieceAdd( pS, iB, delim);
		pS += strlen( pS)+1;
	}
	return *this;
}		// CWString::PieceStringFromNullSeparated
//---------------------------------------------------------------------------
CWString CWString::PieceFormatList(			// make "x, y, and z" list
	int* pCount /*=NULL*/,				// if non-NULL, returns count of
	//   entries in list
	const char* last /*="and"*/,		// last separator ("and", "or", )
	const char* delim /*="|"*/)	const	// delimiter: generally single char
//   but can be any non-blank string
// returns list suitable for display (may be empty)
{
	CWString list;
	CWString cur;
	CWString prev;		// prior non-blank piece
	int iCount = 0;		// # on list so far
	for (int i=0; ; i++)
	{
		if (!Piece( cur, i, delim))			// if no more
		{
			if (!prev.IsEmpty())
			{
				if (iCount == 1)
					list += tFormat( " %s ", last);
				else if (iCount > 1)
					list += tFormat(", %s ", last);
				list += prev;
				iCount++;
			}
			break;
		}
		cur.Trim();
		if (cur.IsEmpty())
			continue;
		if (!prev.IsEmpty())
		{
			if (iCount)
				list += ", ";
			list += prev;
			iCount++;
		}
		prev = cur;
	}
	if (pCount)
		*pCount = iCount;
	return list;
}			// CWString::PieceFormatList
//---------------------------------------------------------------------------
CWString CWString::PieceTrim(				// trim all pieces
	const char* delim /*="|"*/)	const	// delimiter: generally single char
//   but can be any non-blank string
// returns string with leading / trailing WS trimmed from each piece
{
	CWString t,sP;
	for (int i=0; Piece( sP, i, delim); i++)
		t.PieceAdd( sP.Trim(), i, delim);
	return t;
}		// CWString::PieceTrim
//---------------------------------------------------------------------------
CWString& CWString::Trim()			// remove leading and trailing WS
{
	TrimRight();
	TrimLeft();
	return *this;
}			// CWString::Trim
//----------------------------------------------------------------------------
CWString& CWString::CatIf( const char* s, const char *brk /*=" "*/)
// conditional concatenate
// adds break string iff both d and s non-null --
//    this   s	  result
//    ----  ---  -------
//    ""	""    ""
//    "d"   ""    "d"
//    ""    "s"   "s"
//    "d"   "s"   "d" + brk + "s"
// returns *this
{
	if ( s && *s)	// if s contains a string
	{
		if (IsEmpty())		// if we're empty
			*this = s;		//   just use s
		else
			*this += CWString( brk) + s;  	// concat brk + s
	}
	return *this;
}		// CWString::CatIf
//-----------------------------------------------------------------------------
CWString CWString::TranslateEscapes() 		// translate std escape sequences
{
	strTranslateEscapes( GetBuffer( 1));	// translate in place
	ReleaseBuffer();			// reset length, release buffer
	return *this;

}		// CWString::TranslateEscapes
//-------------------------------------------------------------------------
CWString CWString::DeBar() 				// change '_' to ' ' in place
{
	strDeBar( GetBuffer( 1));
	ReleaseBuffer();			// reset length, release buffer
	return *this;
}		// CWString::DeBar
//-------------------------------------------------------------------------
CWString CWString::DeWS()			// remove all WS characters
{
	strDeWS( GetBuffer( 1));
	ReleaseBuffer();			// reset length, release buffer
	return *this;
}		// CWString::DeWS
//-------------------------------------------------------------------------
CWString& CWString::RemoveCRLF()
{
	strRemoveCRLF( GetBuffer( 1));
	ReleaseBuffer();			// reset length, release buffer
	return *this;
}		// CWString::RemoveCRLF
//-------------------------------------------------------------------------
CWString& CWString::Replace2(
	char cFrom,
	char cTo,
	int options /*=0*/)
{
	strReplace2( GetBuffer( 1), cFrom, cTo, options);
	ReleaseBuffer();
	return *this;
}		// CWString::Replace2
//-------------------------------------------------------------------------
CWString CWString::Case( const char toCases[ 3])		// alter case
{
	strCase( GetBuffer( 1), toCases);
	ReleaseBuffer();
	return *this;
}		// CWString::Case
//-------------------------------------------------------------------------
int CWString::ReplaceNoCase(  				// case-insensitive replace
	const char* sOld,		// string to be replaced
	const char* sNew)		// string to replace with
// Replaces all instances of sOld with sNew; sOld match is case-
//	insensitive.  Subsequent search starts after each replacement.
//	Analogous to Replace(), tho slower.
// returns # of replacements
{
	if (!sNew)
		sNew = "";
	int lenNew = strlen( sNew);
	int lenOld = strlen( sOld);

	int count = 0;
	int iStart = 0;
	while (1)
	{
		char* pOld = stristr( (const char*)(*this)+iStart, sOld);
		if (!pOld)
			break;
		int iOld = pOld - (const char*)(*this);
		pOld = GetBuffer( GetLength() + lenNew - lenOld) + iOld;
		if (lenNew != lenOld)
		{
			int nAfter = GetLength() - iOld - lenOld;
			if (nAfter)
				memmove( pOld+lenNew, pOld+lenOld, nAfter+1);
		}
		memcpy( pOld, sNew, lenNew);
		iStart += lenNew;
		ReleaseBuffer();
		count++;
	}
	return count;

}		// CWString::ReplaceNoCase
//-------------------------------------------------------------------------
int CWString::FindNoCase( const char* str) const
{
	CWString fw(str), ts(*this);
	fw.MakeLower();
	ts.MakeLower();
	return ts.Find(fw);
}	// CWString::FindNoCase
//-------------------------------------------------------------------------
#if 0
CWString CWString::WordWrap(			// crude word wrap
	int lLineMax /*=80*/,			// max line final line length (including pfx len)
	//   min length = 20+lPfx
	const char* pfx /*=NULL*/,		// prefix added to each line (re indent etc)
	int options /*=0*/) const		// option bits
//   1: no prefix on 1st line
//   2: drop blank lines
// returns *this broken into multiple chunks with embedded \n's
{
	int lThis = GetLength();
	BOOL bPfx1 = (options & 1) == 0;
	BOOL bDropBlankLines = (options & 2) != 0;
	if (!pfx)
		pfx = "";
	int lPfx = strlen( pfx);
	lLineMax = max( 20, lLineMax-lPfx);

	CWString tOut;		// output string
	int nLOut = 0;		// # of lines in tOut
	CWString tLine;		// current line
	int lToWS = 999;	// length to last non-WS in line
	//   init to "whole line"
	for (int i=0; i<=lThis; i++)
	{
		int lLine = tLine.GetLength();	// current line length
		char c;
		if (!lLine && i == lThis)
			break;		// no more chars and cur line empty
		else
			c = i < lThis ? GetAt( i) : '\n';		// fake '\n' at end
		BOOL bWS = isspaceW( c);
		if ( bWS)
		{
			if (!lLine)
			{
				if (c != '\n' || bDropBlankLines)
					continue;	// drop leading WS
			}
			else if (!isspaceW( tLine[ lLine-1]))
				lToWS = lLine;
		}
		if (lLine >= lLineMax || c == '\n')
		{
			// wrap line
			if (nLOut > 0)
				tOut += '\n';	// if not 1st line
			if (nLOut > 0 || bPfx1)
				tOut += pfx;
			tOut += tLine.Left( lToWS);
			nLOut++;
			tLine = tLine.Right( lLine - lToWS);
			tLine.TrimLeft();
			lToWS = 999;
			if (bWS && !tLine.GetLength())
				continue;	// don't add WS at beg
		}
		tLine += c;
	}
	return tOut;
}		// CWString::WordWrap
#endif
//=========================================================================

///////////////////////////////////////////////////////////////////////////
// handy temporary CWString public fcns
//   inline CWString tCopy( const char* s);		// in str_opts.h
//-------------------------------------------------------------------------
CWString tFormatV( LPCTSTR lpszFormat, va_list ap)
{
	CWString t;
	t.FormatV( lpszFormat, ap);
	return t;
}		// tFormatV
//-------------------------------------------------------------------------
CWString tFormat( LPCTSTR lpszFormat, ...)		// format to temporary
{
	va_list argList;
	return tFormatV( lpszFormat, va_start( argList, lpszFormat));
}				// ::tFormat
//-------------------------------------------------------------------------
CWString tFmtIf(					// conditional format to temporary
	LPCTSTR fmtNonBlank,		// format to use if arg1 not blank
	LPCTSTR fmtBlank,			// format to use if arg1 is blank
	...)						// args ... 1st arg assumed to be char*
{
	va_list argList;
	va_start( argList, fmtBlank);
	CWString t;
	return t.FmtIfV( fmtNonBlank, fmtBlank, argList);
}				// ::tFmtIf
//-------------------------------------------------------------------------
CWString tPluralize( const char* word, bool bPlural/*=true*/)
{
	CWString t;
	strPluralize( t.GetBuffer( 50), word, bPlural);
	t.ReleaseBuffer();
	return t;
}				// ::tPluralize
//-------------------------------------------------------------------------
CWString tCase( const char* s, const char toCases[ 3])
{
	CWString t( s);
	return t.Case( toCases);
}				// ::tCase
//-------------------------------------------------------------------------
CWString CWString::DoubleDelim(		// double delimiter in place
	const char* delim)		// delimiter (len>1 OK)

// Convert all occurences of "c" to "cc". For example
// string "kdd'kd'kddd'" will become "kdd''kd''kddd''"
// This is useful for building SQL statements, because
// all ' should be converted to '' there.

// returns converted string
{
	CWString delim2( delim);
	delim2 += delim;
	Replace( delim, delim2);
	return *this;
}		// CWString::DoubleDelim
#endif

#if 0
//----------------------------------------------------------------------------
char * strTrim( char* d, const char* s)		// deblank beg and end of string
//  if d == s, trims in place
{
	strTrim( d, s, 999999);		// don't limit d, ignore return
	return d;
}		// strTrim
//----------------------------------------------------------------------------
BOOL strTrim(			// deblank beg and end of string
	char* d,			// dest string, if d==s, deblanks in place
	const char* s,		// source string
	int l)			// sizeof( d); truncates at l-1
// returns TRUE if all non-WS chars preserved
//        FALSE if 1 or more non-WS chars truncated
{
	s = strTrimB( s);	// scan off leading ws

	int iWS = l;		// idx of beg of last WS run
	//    =l when not in run
	for (int i=0; *s && i<l-1; i++)
	{
		d[ i] = *s;				// store char
		if (isspaceW( *s))
		{
			if (iWS == l)
				iWS = i;		// start new WS run
		}
		else
			iWS = l;			// we're not in a WS run
		s++;
	}
	d[ min( iWS, i)] = '\0';	// terminate at beg of WS or end

	s = strTrimB( s);			// scan for dropped non-WS
	return *s == '\0';
}			// strTrim
//----------------------------------------------------------------------------
char* strTrim2(			// deblank beg and end of string in place
	char* s)			// string
// returns TRUE if all non-WS chars preserved
//        FALSE if 1 or more non-WS chars truncated
{
	char* sEnd = NULL;
	char* sBeg = strTrimB( s);
	for (s=sBeg; *s; s++)
	{	if (isspaceW( *s))
		{	if (!sEnd)
				sEnd = s;
		}
		else
			sEnd = NULL;
	}
	if (sEnd)
		*sEnd = '\0';

	return sBeg;
}			// strTrim2
#endif
//===========================================================================
int strDecode(					// decode numeric value (internal fcn)
	//   note inline mapping fcns (str_opts.h)
	const char* s,		// string to be decoded
	const char *fmt,	// decode format, with "guard char", e.g. "%lg%1s"
	//   Note: %1c will NOT scan over WS, use %1s
	void *p,			// where to store converted value
	int erOp/*=IGN*/,			// msg ctrl: IGN/WRN/ABT
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
			len = strlen( d);
		if (!s || !*s)
			s = "?";
		int lenS = strlen( s);
		for (int i=0; i < len; i++)
			d[ i] = s[ i%lenS];
		d[ len] = '\0';
	}
	return d;
}		// strFill
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
	{
		d[ i] = s[ i] == '_' ? ' ' : s[ i];		// copy / translate
		i++;
	}
	d[ i] = '\0';			// terminate

	return d;
}						// strDeBar
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
char* strCatIf( char* d, size_t dSize, const char *brk, const char* s)
// conditional concatenate
// adds break string iff both d and s non-null --
//   d    s	result
// ---  ---	-------
// ""	 ""	 ""
// "d"   ""  "d"
// ""    "s" "s"
// "d"   "s" "d" + brk + "s"

// returns d
{
	if ( s && *s)	// if s contains a string
	{	if (*d)		// if d not ""
			strcat_s( d, dSize, brk);	// add break string
		strcat_s( d, dSize, s);			// add s
	}
	return d;
}		// strCatIf
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
		"goose", "geese",
		"mouse", "mice",
		"is",    "are",
		"has",   "have",
		// add add'l exceptions here here
		NULL
	};
	if (!word || !word[ 0])
		*d = '\0';
	else if (!bPlural)
		strcpy( d, word);
	else
	{
		bool bException = false;
		for (int i=0; excpTbl[ i].wordSingular; i++)
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
			int len = strlen( d);		// len >= 1 (check above)
			BOOL bLastUpper = IsCharUpper( d[ len-1]) > 0;
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
	for (int i = 0; i < 3; i++)
	{
		if (!toCases[i])			// if < 3 chars given
			break;					// treat rest as -
		else if (IsCharLower(toCases[i]))
			itc[i] = lower;
		else if (IsCharUpper(toCases[i]))
			itc[i] = upper;
	}

// convert string
	int nsSeen = 0;			// no non-space seen
	int which = 0;			// 1st char uses cases[0]
	int len = strlen(s);
	for (i = 0; i < len; i++)
	{
		if (isspaceW( *( s+i)) || s[ i] == '-')
			which = nsSeen > 0;	// for s[i+1]. note init: which = 0.
		// treat hyphen as space
		else				// spaces don't need case conversion
		{
			if (itc[which]==upper)
				CharUpperBuff( s+i, 1);
			// upperize 1 char, handle accented chars
			else if (itc[which]==lower)
				CharLowerBuff( s+i, 1);
			// lowerize 1 char, handle accented chars
			which = 2;			// for s[i] til next space
			nsSeen++;			// next word beginning is not 1st word
		}
	}
	return s;
}		// strCase
//-------------------------------------------------------------------------
char* strRemoveCRLF( char* str)
{
	strReplace( str, "\r", "");
	strReplace( str, "\n", "");
	return str;
}		// strRemoveCRLF
//-------------------------------------------------------------------------
int strReplace(  				// case-insensitive replace
	char* str,				// string in which replacements are to be made
	//   IN PLACE, caller must supply enuf space!
	const char* sOld,		// string to be replaced
	const char* sNew,		// string to replace with
	BOOL bCaseSens /*=FALSE*/)	// TRUE: case-sensitive sOld search

// Replaces all instances of sOld with sNew; sOld match is case-
//	(in)sensitive per options.  Next search starts after each replacement.
// returns # of replacements
{
	if (!sNew)
		sNew = "";
	int lenNew = strlen( sNew);
	int lenOld = strlen( sOld);
	int len = strlen( str);

	int count = 0;
	int iStart = 0;
	while (1)
	{
		const char* pOld = bCaseSens
						   ? strstr(  (const char*)(str+iStart), sOld)
						   : stristr( (const char*)(str+iStart), sOld);
		if (!pOld)
			break;
		if (lenNew != lenOld)
		{
			int nAfter = len - (pOld-str) - lenOld;
			if (nAfter)
				memmove( (void *)(pOld+lenNew), pOld+lenOld, nAfter+1);
			len += lenNew - lenOld;
		}
		memcpy( (void *)pOld, sNew, lenNew);
		iStart += lenNew;
		count++;
	}
	return count;
}		// strReplace
//-----------------------------------------------------------------------------
int strReplace2(			// replace variant
	char* s,		// string (modified in place)
	char cFrom,		// char to be replaced
	char cTo,		// char to replace (if \0, drop)
	int options /*=0*/)		// options
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
char* stristr(					// case-insensitive string find
	const char * str1,		// string in which to search
	const char * str2)		// string to search for
// returns pointer within str1 of 1st char of substring matching str2
//         NULL if not found
{
	if (!str1 || !str2 || !*str2)
		return (char *)str1;		// empty, immediate match

	for (char* cp = (char *)str1; *cp; cp++)
	{
		if (toupper( *cp) == toupper( *str2))
		{
			char* s1 = cp;
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
