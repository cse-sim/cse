// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cutok.cpp: CSE user language get token stuff

#include <CNGLOB.H>

/*-------------------------------- OPTIONS --------------------------------*/

/*------------------------------- INCLUDES --------------------------------*/
#include <MSGHANS.H>	// MH_S0150

#include <ANCREC.H>	// getFileIx
#include <CVPAK.H>	// cvatof
#include <RMKERR.H>	// errI
#include <MESSAGES.H>	// MSG_MAXLEN msgI

#include <PP.H>		// ppOpen ppClose ppGet
#include <CUTOK.H>	// decls for this file; CUTSI CUTFLOAT cufClose


/*-------------------------------- DEFINES --------------------------------*/

/*--------------------------- PUBLIC VARIABLES ----------------------------*/
// see "token level" and "error message interfaces" below

/*---------------------------- LOCAL VARIABLES ----------------------------*/
// also see "token level" and "pp interface" below

// following is set by "preprocessor interface" & used in "token input level"
LOCAL USI uliFline = 0;		// input file line # (for error messages)

/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
// also see "token level" below
// following in "preprocessor interface" and called by "token input level"
LOCAL void cuppiClean( CLEANCASE cs);
LOCAL void cuUnc();
LOCAL SI   cuC();
LOCAL SI   cuScanto( char *set);
LOCAL void svFname( char *name, USI len);
LOCAL void cufMark1();
LOCAL void cufMark2();
LOCAL void cufCline( int flag, int* pcol, char *s, size_t sSize );


/************************** TOKEN INPUT LEVEL *****************************/


/*----------------- PUBLIC VARIABLES for token input level ----------------*/

// variables set by cuTok()
char cuToktx[CUTOKMAX+1] = { 0 };	// token text: identifier name, quoted texts without quotes, etc
static SI cuToklen = 0;			// # chars in token; chars truncated if > CUTOKMAX.
USI cuSival = 0;			// value of integer number
FLOAT cuFlval = 0.F;		// floating value of number.  note numbers exclude signs at this level

/*----------------- LOCAL VARIABLES FOR token input level -----------------*/
LOCAL SI cuTokty = 0;	// token type (same as cuTok return) make public if need found
LOCAL SI cuRetok = 0;	// nz to re-ret same tok: cuUntok(),cuTok()
LOCAL USI cutFline = 0;	// file line at start token (for errmsgs)
LOCAL USI pvCutFline = 0;	// " previous token, use if cuRetok nz.
LOCAL SI cutChar = 0;  	// last char returned by cuTc()
LOCAL SI cutRechar = 0;	// nz to return cutChar again: cuUntc(),cuTc()

/*---------- LOCAL FUNCTION DECLARATIONS for token input level ------------*/
LOCAL void cuQuoTx();
LOCAL SI   cuTc();
LOCAL SI   cuTcNdc();
LOCAL SI   cuTcDc();
LOCAL void cuUntc();
LOCAL void cuNottc();

//===========================================================================
void cuTokClean( 		// cuTok init/cleanup routine

	CLEANCASE cs )	// [STARTUP], ENTRY, DONE, CRASH, or [CLOSEDOWN]. type in cnglob.h.

// called from cuparse.cpp
{
// added 10-93 while converting from main program to repeatedly-callable, DLLable subroutine.
	/* function should: initialize any variables for which data-initialization was previously assumed,
	                    free all heap memory and NULL the pointers to it,
	                    close any files and free other resources. */
	// 10-10-93: this file reviewed for cleanup, fcns coded and called.
	// 10-10-93: all ppxxx.cpp files reviewed for cleanup, and cleanup fcns coded and called.

// clean cutok.cpp "token input level"
	cuRetok = 0;		// say don't re-ret same tok: cuUntok(),cuTok()
	cutRechar = 0;		// say don't return cutChar again: cuUntc(),cuTc()
	//cuTokty = 0;		// beleived unneeded
	cutFline = pvCutFline = 0;	// zero file lines at start token, previous token (beleived redundant unless bug elsewhere)

// clean cutok.cpp "preprocessor interface level"
	cuppiClean(cs);		// below

// clean preprocessor
	ppClean(cs);		// clean up pp.cpp and associated files, pp.cpp.
}		// cuTokClean
//===========================================================================
void cuUntok()		// unget token: make next cuTok return it again.

// cuToktx etc must be undisturbed
// NO CALLS 11-25-91 -- cuparse has its own unToke mechanism.
{
	if (cuTokty)
		cuRetok++;
}			// cuUntok

//===========================================================================
SI cuTok()		// get next cu token, return type

// sets: cuToktx, cuToklen, cuSival, cuFlval

// note: open/close file with cufOpen/cufClose, below

/* function value is token type (CUT defines in cutok.h), including:
    CUTID:    identifier, in cuToktx. 1-n letters, digits (not digit first)
    CUTSI:    integer,  value in cuSival (cuFlval also set).
    CUTFLOAT: floating number (too big, or has . or e etc), cuFlval set.
	note signs are not considered part of numbers here.
    CUTQUOT:  quoted text, contents of quoytes in cuToktx.
    CUTEQL ==, CUTNE !=, CUTLE <=,
    etc. */
{
	SI c, c2, base;
	RC rc;

// on flag, return previously gotten token again
	if (cuRetok)		// set by cuUntok()
	{
		cuRetok = 0;
		return cuTokty;   	// cuToktx etc assumed undisturbed
	}

// copy start-token info (for errmsgs) to start-prior-token variables

	pvCutFline = cutFline;	// remember prior token file line
	cufMark2();			/* copy "saved position 1" to "saved posn 2":
    				   input buffer position, adjusted by cuC when text in buffer moved. local above. */

reget:		// here to start token decode over (eg after illegal char)

	cuToklen = 0;		// init # chars in token body
	cuToktx[0] = '\0';  	// terminate initially null token
	cuSival = 0;		// no numeric value yet
	cuFlval = 0.F;		// ..

// pass whitespace to start token, get first character

	while (c = cuTc(), isspaceW(c) != 0)		// decomment, get chars
		cuNottc();								// whitespace not part of token body
	cufMark1();		// save buf posn of start token, internally in char stuff above so can be adj'd when text moved.
	    			// Used to echo file line in err messages.  See cuErv, cuCurLine, cufCline.
	cutFline = uliFline;	// save file line at start token for errmsgs

// peek at 2nd char of token

	c2 = cuC();  		// peek at 2nd char
	cuUnc();			// leaving it in file too
	c2 = tolower(c2);		// lower case (eg for 0x)
	// c2 is 2nd char, ungotten.

// digit first: try decode as integer, else floating point

	if (isdigitW(c))
	{
		LI tem;
		// set up re 0x, 0o prefixes
		if (c=='0' && c2=='x')		// 0x<digits>: hex
		{
			base = 16;
			cuC();			// gobble the x (c2 previously ungotten)
		}
		else if (c=='0' && c2=='o')	// 0o<digits>: octal
		{
			base = 8;
			cuC();			// gobble the o (c2 previously ungotten)
		}
		else
			base = 10;

		// digits syntax and integer value loop
		do
		{
			tem = (LI)cuSival * (LI)base + (LI)(c-'0');	// SI value
			if ( base==10 		// treat decimal as signed 16 bits
			&&  tem > 32767L )		// limit to 32767: 15 bits
				// LIMITATION: can't do decimal -32768
				goto havedig;		// go finish conversion as float
			// hex/octal checked & truncated below this loop
			cuSival = (SI)(USI)tem;
			c = cuTc();			// next token char
			if (!isalnumW(c))		// if not digit, or letter for hex
				break;				// done
			if (c >= 'A')				// make hex letters
				c = toupper(c) - 'A' + '9' + 1;	//  follow 0-9
		}
		while (c >= '0' && c < '0' + base);	// 0-7, 0-9, or 0-f
		cuUntc();   				// unget last (non-digit) char
		if (tem > 65535L)			// if hex/octal # too big
			// user beware of - with 0x, 0o
			cuEr( 0, (char *)MH_S0150);		// "Number too big, truncated" & continue.

		// integer done, but check if actually start of a float
		if (base != 10				// 0x and 0o cannot begin floats
		|| strchr( ".EeDd", c)==NULL)		// . E e D or d  next makes float
		{
			cuTokty = CUTSI;			// say is integer, value in cuSival
			cuFlval = (float)(USI)cuSival;	// set floating value also
		}
		else			// treat as floating point
		{
			double dval;

			/* look for floating point number.  entered from other cases, chars already scanned are in cuToktx.
			   syntax: [digits][.[digits]][e|E|d|D[+|-]<digit>[digits]].  note leading sign is treated here as separate token. */

			// 1. parse float number, assemble text in cuToktx for conversion

			// nb fall in
havedig:	/* here if have initial digit(s)
		   but value too big (there may be more ungotten digits), or next (ungotten) char is e, ., etc. */
			while (isdigitW(c = cuTc()))    	// digits b4 .
				;
			if (c=='.')
havepoint: 		// here on initial . followed by (ungotten) digit
				while (isdigitW(c = cuTc()))	// get digits after .
					;
			if (strchr("eEdD", c))		// if exponent indicator
			{
				c = cuTc();			// next token char
				if (!strchr("+-", c))		// unless exponent sign
					cuUntc();       		// unget char
				while (isdigitW(c = cuTc()))	// pass exponent digits
					;
			}
			cuUntc();  				// unget last char: not part of number

			// 2. convert parsed text to float

			rc = cvatof( cuToktx, &dval, 0);	// convert, cvatoxxx.cpp
			if (rc)   				// message error
				cuEr( 				// errmsg with line # and file name
					0, msg( NULL, (char *)(LI)rc));	// Tmpstr text for cvatof return code, messages.cpp
			// no error indication returned to caller?
			cuFlval = (float)dval;		// double-->float
			cuTokty = CUTFLOAT;			// say token is a float
		}     // if integer ... else ...
	}    // if isidigit(c)

// letter, $, or _ first: decode identifier

	else if (isalphaW(c))  	// letter begins identifier
	{
iden:				// other id 1st chars join here, from switch below
		while ( isalnumW(c=cuTc())	// gobble letters, digits,
		|| strchr("$_",c) )	//  and $'s and _'s to cuToktx[]
			;
		cuUntc();		// unget final non-identifier char
		cuTokty = CUTID; 	// say is an identifier
	}

// other initial characters

	else
	{
		// 2-char tokens ending in =: <=, ==, !=, etc.
		if (c2 == '=')
		{
			static const char c1s[] = { '=',    '!',   '<',   '>',   0 };
			static const SI gnuts[] = { CUTEQL, CUTNE, CUTLE, CUTGE };
			const char *p = strchr( c1s, c);		// find char in string
			if (p)					// if found
			{
				cuTokty = gnuts[(SI)(p-c1s)];		// type from parallel array of SI's
				cuTc();					// get char to make c2 part of token
				return cuTokty;
			}
		}

		// 2-of-same-char tokens:  &&  ||  >>  <<
		if (c2 == c)
		{
			static const char cs[] = { '&',    '|',    '>',    '<',    0 };
			static const SI cuts[] = { CUTLAN, CUTLOR, CUTRSH, CUTLSH };
			const char *p = strchr( cs, c);		// find 1st char in string
			if (p)					// if found
			{
				cuTokty = cuts[(SI)(p-cs)];		// type from parallel array of SI's
				cuTc();					// get char to make c2 part of token
				return cuTokty;
			}
		}

		// */: unmatched close comment (for specific error msg in expr())
		if (c=='*' && c2=='/')
		{
			cuTokty = CUTECMT;
			cuTc();			// get char to make c2 part of token
			return cuTokty;
		}

		// cases for other initial characters

		switch (c)	// note c2 is next, ungotten, character
		{
		case '.':
			if (isdigitW(c2))		// if digit follows initial .
				goto havepoint;		// go decode float, above
			goto single;		// else is token by itself

		case '$':
		case '_':	// chars that start identifiers
			goto iden;			// join identifier code, above

		case '"':		// quoted text
			cuQuoTx();
			break;		// separate fcn does it

		case EOF:		// end of file
			cuTokty = CUTEOF;
			strcpy( cuToktx, "<EOF>"); 	// for errmsg display
			break;

		default:  		// others are 1-char tokens
			// reject random non-whitespace, non-printing characters now
			if (c < ' ' || c > '~')
			{
				cuEr( 0, (char *)MH_S0151, (UI)c);		// "Ignoring illegal char 0x%x"
				goto reget;
			}
single:		// 1-char cases may here or fall in
			// token type for 1-char tokens is ascii sequence w ctrls, digits, letters squeezed out (for small tables elsewhere).
			{
				static const UCH tasc[] = { '{', '[', ':', ' ', 0 };
				static const UCH tbase[] = { 29,  23,  16,  0,  0 };
				SI i;
				for (i = 0; (UCH)c < tasc[i]; )
					i++;					// find range containing char
				cuTokty = c - tasc[i] + tbase[i];	// compute token type. MATCHES DEFINES IN cutok.h.
			} // table scope
			break;

		}	// switch(c)
	}	// if digit ... else if alfa ... else ...

// check length (for quoted texts, identifiers, perhaps numbers)

	if (cuToklen > CUTOKMAX)
		cuEr( 0,						// errmsg w file name, tok line #
			  cuTokty==CUTQUOT
			  ?  (char *)MH_S0152		// "Overlong quoted text truncated: \"%s...\"": Specific msg for most likely case
			  :  (char *)MH_S0153, 		// "Overlong token truncated: '%s...'"
			  cuToktx );

	return cuTokty;
	// other return(s) above
}				// cuTok
//===========================================================================
LOCAL void cuQuoTx()	// do quoted text case for cuTok
{
	SI c, c2;

	cuNottc(); 				// remove " from cuToktx[]
	cuTokty = CUTQUOT;			// set token type in file-global

	for ( ; ; )			// loop to scan to end quoted text
	{
		c = cuTcNdc();   		// get char WITHOUT DECOMMENTING, does NOT put in cuToktx.
		switch (c)			// check for exceptional chars between quotes
		{
eofInqErr:
		case EOF:
			cuEr( 0, (char *)MH_S0154);
			return;			// "Unexpected end of file in quoted text"

			/*lint -e616  lint says fall into case, despite "return" above */
			//newlInqErr:
		case '\r':
		case '\n':
			cuEr( 0, (char *)MH_S0155);
			return;			// "Newline in quoted text"

		case '"':
			return;		// " ends it

		case '\\':			// \: next char special
			c = cuTcNdc();   		// most chars ok after \, incl ".
			switch (c)			// process char after \ between ""
			{
			case EOF:
				goto eofInqErr;
			case '\r':
				if (cuTcNdc() != '\n')	// pass \r after \n
					cuUnc();			// oops, unget char
				/*lint -e616 */		// fall thru
			case '\n':
				continue;			// \ \n: not stored, "text" continues on next line
			case 'e':
				c = 27;
				break;		// \e: escape
			case 't':
				c = '\t';
				break;		// \t: tab
			case 'f':
				c = '\f';
				break;		// \f: form feed
			case 'r':
				c = '\r';
				break;		// \r: cr
			case 'n':
				c = '\n';
				c2 = '\r';	// \n: store cr, lf
				/*use2c:*/
				if (cuToklen < CUTOKMAX)	 	// unless full
				{
					cuToktx[ cuToklen] = (char)c2;	// add char
					cuToktx[ cuToklen + 1] = '\0';	// keep term'd
				}
				cuToklen++;			// count chars in ""
				break;			// go store 2nd char (c)
				// add cases as desired for \xffff, etc, etc
			default:
				break;
			}    /* switch (char after \) */
			/* fall thru to store char (c) after \ */ /*lint -e616 */
			//useC:
		default:		// buffer chars in ""'s in cuToktx
			if (cuToklen < CUTOKMAX)    		// unless full
			{
				cuToktx[ cuToklen] = (char)c;   	// add char
				cuToktx[ cuToklen + 1] = '\0';		// keep term'd
			}
			cuToklen++;			// count chars in ""

		}	// switch (c) in quoted text
	}		// for ; ;   loop over chars in ""

	/*NOTREACHED */ /*lint +e616 */
	// returns are in " and error cases in switch
}			// cuQuoTx

/*================== local functions to support cuTok() ===================*/

//===========================================================================
LOCAL SI cuTcNdc()  	// get character of token body from file, NOT DECOMMENTED: for text in ""

// CAUTION: 1. no unget char at this level.
//          2. does not buffer nor count token length.
{
	return cuC();		// get char from char input level, above
}		/* cuTcNdc */
//===========================================================================
LOCAL SI cuTc()  	// get character for token, removing comments, buffering it in cuToktx
{
// get char
	if (cutRechar)			// nz to return cutChar again
		cutRechar = 0;			// (set by cuUntc())
	else				// normally
		cutChar = cuTcDc();		// get decommented char (next)

// buffer token text
	if (cuToklen < CUTOKMAX)    		// unless token buffer full
	{
		cuToktx[ cuToklen] = (char)cutChar;	// add char (or EOF) to token
		cuToktx[ cuToklen + 1] = '\0';		// keep token buf terminated
	}
	cuToklen++;  			// count length of token

	return cutChar;			// return character
}			// cuTc
//===========================================================================
LOCAL SI cuTcDc()  	// get input char, changing comments to single whitespace chars. for cuTc.
{
	SI c, c2;

	c = cuC();			// get char from file char level, above
	if (c != '/')		// if not a /
		return c;		// cannot begin a comment, return to caller
	c2 = cuC();			// char after /
	switch (c2)
	{
	case '/':		//  2 /'s start rest-of-this-line comment
		for ( ; ; )	// scan to \r or \n not after \ (or to EOF)
		{
			c = cuScanto("\\\r\n");	// quickly cuC to  \r  \n  \  or EOF
			if (c != '\\')		// \: pass next char and continue
				return c;		// \r  \n  or  EOF: return it
			c = cuC();		// get char after \ (to pass it)
			if (c=='\r')		// if \r after '\'
			{
				if (cuC() != '\n')	// also pass \n after the \r
					cuUnc();		// char after \r not \n: unget it
			}
		}

	case '*':		//  / and * start comment that runs to * and /
		for ( ; ; )		// loop to look at *'s in comment
		{
			c = cuScanto("*");    	// quickly skip to * or EOF
			if (c==EOF)
				return EOF;
			c = cuC();		// char after an * in comment */
			if (c=='/')
				break;		// * and / end comment
			cuUnc();		// unget char (may be *) and loop
		}
		return ' ';	/* return ' ' for comment, to be sure token such as == or identifier
	     			   not allowed to have a comment in the middle */

	default:  	// the / did not begin a comment
		cuUnc();    	// unget c2 at char level
		return c;   	// return the /
	}

	// many other returns above
}			// cuTcDc
//===========================================================================
LOCAL void cuUntc()	// unget last char gotten with cuTc, from file and cuToktx
{
	cuNottc();			// remove last char from cuToktx (next) */
	cutRechar++;		// tell cuTc to return same char again
	// note don't use cuUnc() due to decommenting
}	// cuUntc
//===========================================================================
LOCAL void cuNottc()	// say last char not part of token body (does not unget from file)
{
	if (cuToklen > 0)   		// if have any chars
		cuToklen--;			//  back up one char
	cuToktx[ cuToklen] = '\0';  	// zero out last char
}			// cuNottc


/****************** PREPROCESSOR (PP.C) INTERFACE LEVEL *******************/

/*------------------ LOCAL VARIABLES for pp interface --------------------*/
//more variables above, incl:
//  LOCAL USI uliFline = 0;  	// input file line # (for error messages)
LOCAL int uliFileIx = 0; 		// "file index" of current input file, for record.fileIx, from ancrec:getFileIx
LOCAL char* uliFname4Ix = NULL;	// text associated with uliFileIx
LOCAL int uliEofRead = 0;		// non-0 if eof has been read to buffer
LOCAL int uliEofRet = 0;		// non-0 if eof has been returned by cuC
LOCAL int uliBufn = 0;  		// # chars in uliBuf[]
LOCAL int uliBufi = 0;  		// subscr nxt char to fetch from uliBuf[]
LOCAL int uliBufi1 = 0;  		// another uliBuf subscr for "uliMark" fcn
LOCAL int uliBufi2 = 0;  		// yet another ..
const int BUFKEEP = 512;		// # input chars to hold on each side of curr posn (to echo line in err msgs)
const int BUFREAD = 1024;		// # input chars to read at a time
const int ULIBUFSZ = BUFKEEP+BUFREAD+BUFKEEP+2;
								// buf size, for use here and in cuErv below.
								// +2 for null, cuz strchr loads text by word and can go
LOCAL char uliBuf[ ULIBUFSZ] = { 0 };	// input buffer
//---- variable(s) internal to fcns, here only to permit cleanup
LOCAL int cuCmidline = 0;		// cuC function: 0 if at start of line: look for #line

//==========================================================================
LOCAL void cuppiClean(CLEANCASE /*cs*/)	// called from cuTokClean, above
{
	//uliEofRead= uliEofRet= uliBufn= uliBufi= uliBufi1= uliBufi2= 0;	done by cufOpen.
	//uliBuf[0] = 0;							done by cufOpen.

	cuCmidline = 0;		// cuC(): at start of line

	uliFileIx = 0;			// no current file
	dmfree( DMPP( uliFname4Ix)); 	// no text of current file name

	uliFline = 0;		// input file line number, set by #line
}		// cuppiClean
//==========================================================================
RC cufOpen(			// open and init CSE language input file

	const char* fname, 	// file name input by user
	char *dflExt ) 		// NULL or "" or default .extension incl leading "."
{

// init our char input stuff
	uliEofRead= uliEofRet= 			// init end file flags
	uliBufn= uliBufi= uliBufi1= uliBufi2= 0;	// init buffer variables
	uliBuf[0] = 0;				// mark buffer empty

// open file(s) (pp.cpp)
	return ppOpen( fname, dflExt /*,logFname*/);	// open file for pp input
}				// cufOpen
//==========================================================================
void cufClose() 		// close open cse user language input file
{
	ppClose();   	// close input file at pp.cpp level
}		// cufClose
//==========================================================================
LOCAL void cuUnc() 	// unget last character gotten with cuC
{
	if (uliBufi > 0)		// if a char has been gotten: insurance
		if (uliEofRet==0)	    // if EOF has NOT been returned by cuC yet
		{
			uliBufi--;			// cause char to be ret'd again
			if (uliBuf[uliBufi]=='\n')	// if newline ungotten
				uliFline--; 		// decr file line
		}
}		// cuUnc
//==========================================================================
LOCAL SI cuC() 		// get character (or EOF) from open CSE input file
{
	SI c;
	char *p, *q;

	for ( ; ; ) 	// to repeat re #line
	{
		// refill buffer to keep BUFKEEP chars before position, and BUFKEEP chars [or 1 newline] after position

		if ( uliBufi + BUFKEEP >= uliBufn  		// if <= enuf chars left in buf
		  && !strchr( uliBuf + uliBufi, '\n')	// .. if have no newline left in buf (thus not whole line for errmsg echo)
		  && uliEofRead==0)						// .. if file eof not read yet
		{	int n;
			// keep BUFKEEP (=512?) already-returned chars for errmsg line echo
			if (uliBufi > BUFKEEP)    	// if have enuf alredy-ret'd chars
			{
				n = uliBufi - BUFKEEP; 	// # chars to discard
				memmove( uliBuf, 		// move chars being kept
					uliBuf + n, 		// ... to front of buffer
					uliBufn - n);
				uliBufn -= n;  		// adjust buf subscripts for move
				uliBufi -= n;  		// ..
				uliBufi1 -= n; 		// extra subs for "mark" function
				uliBufi2 -= n; 		// ..
			}
			// read BUFREAD characters to end of buffer
			n = min( BUFREAD, int( sizeof( uliBuf) - uliBufn));	// insurance
			n = ppGet( uliBuf + uliBufn, n); 				// get 1 to n chars of preproc output (pp.cpp, only call)
			// 11-91 stops after returning a newline, to minimize preprocessing-ahead,
			//       to facilitate putting error msgs in place in listing.
			if (n==0)			// if 0 chars left in file, say eof read.
				uliEofRead++;    		//  NB unret'd chars usually remain in buf.
			uliBufn += n; 		// subscr after last char in buf
			uliBuf[uliBufn] = '\0';	// terminate (for cufCline, cuScanto)
		}

		// if EOF, return it

		if (uliBufi >= uliBufn)		// if here and buffer empty, is end file.
		{
			uliEofRet = 1;  		// tell cuUnc that eof has been returned
			return EOF;
		}

		// look for '#' at start line,
		// else return character (currently does not decomment nor look for quotes)

		c = (SI)(USI)(UCH)uliBuf[ uliBufi++];	// get char, no sign extend
		switch (c)				// returns c unless # at start line
		{
		case '\n':
			cuCmidline = 0;
			uliFline++;
			return c;
		default:
			cuCmidline = 1;
			return c;
		case '#':
			if (cuCmidline)
				return c;		// normal return
		}

		// check/decode line <n> "<filename>" \n
		// else leave in input and return c

		p = uliBuf + uliBufi;
		if (memcmp( p, "line ", strlen("line ")))	// "line "
			return c;
		p += strlen("line ");
		int flint = 0;				// file line number
		while (isdigitW(*p))
		{
			flint = flint * 10 + (*p++ - '0');
		}
		if (flint==0)
			return c;
		while (*p==' ')
			p++;
		if (!(*p++ == '"'))			// filename in quotes
			return c;
		for (q = p; *p != '"'; p++)	// find matching "
		{
			if (*p == '\n')
				return c;   		//  newline in "'s: assume not #line
#if 0	// bug fix, fails on long file names, 11-12
x			if ((USI)(p - q) > 127)
x				return c;   		//  too long, assume not #line
#endif
		}
		int namelen = p - q;
		while (* ++p==' ')
			;
		if (*p++ != '\n')  	// this newline NOT counted as file line
			return c;

		// good #line command.  Save results.

		uliBufi = (USI)(p - uliBuf);		// next input char: advance buff ptr past it
		// remove #line line from buffer?
		svFname( q, namelen);		// save name, set uliFileIx (for cuCurLine / record.file). below. only call.
		uliFline = flint;			// file line number
		// now loop to return a char (refill buffer if necessary)
	}
	/*NOTREACHED */
	// returns are in switch and above
}			// cuC
//==========================================================================
LOCAL SI cuScanto( char *set)		// pass characters in input file not in "set"

// intended to be faster than caller cuC() loop.

// returns EOF or the char in set
{
	SI c;
	char setx[100];

	setx[0] = '\n';			// extend "set" to include newline
	strcpy( setx+1, set);		// so scanned-past file lines get counted
	do
	{
		uliBufi += strcspn( uliBuf + uliBufi, setx);	// quickly pass chars already in buf but not in "setx"
		c = cuC();					// reload buffer if nec, get next char, count \n's
	}
	while ( c != EOF
			&& strchr( set, c)==NULL );	// repeat to eof or char in set

	return c;
}		// cuScanto
//===========================================================================
LOCAL void svFname( 		// save #line file name (to last til end session); set uliFileIx

	char *name, 	// ptr to #line command file name in buffer
	USI len )		// length. NOT null-terminated.

// callers use uliFileIx, set here, for record.fileIx.
{
// do nothing if name same as current name, for speed
	if ( uliFileIx != 0
	&&  uliFname4Ix				// insurance
	&&  strlen(uliFname4Ix)==len
	&&  memcmp( name, uliFname4Ix, len)==0 )
		return;

// save name and get index using fcn in ancrec.cpp, 2-94
	uliFileIx =  getFileIx( name, len, &uliFname4Ix);

}				// svFname

//==========================================================================
LOCAL void cufMark1()		// note buffer position BEFORE last gotten char as "saved position 1"

/* Used to remember start of token, for error messages.
	File text at saved positions accessible with cufCline() if still
     near current position. */
{
	uliBufi1 = uliBufi-1;		// save position b4 last gotten char: use cuC's next-char buffer subscript
	// note uliBufi1, 2 are adjusted when buffer contents is moved (by cuC).
}			// cufMark1
//==========================================================================
LOCAL void cufMark2()		// copy "saved buffer position 1" to "saved position 2"

/* Used to remember start PRIOR token, for errmsgs.
	File text at saved positions accessible with cufCline() if still
     near current position. */
{
	uliBufi2 = uliBufi1;		// save value from last curMark call in a different variable
}			// cufMark2
//==========================================================================
LOCAL void cufCline( 	// get current input file line text and col # to caller's storage

	int flag, 	// 0: use last read position (uliBufi).
			    // 1: use start current token (last "cufMark1" position)
				// 2: use start previous token ("cufMark2" position)
	int* pcol, 	// NULL or receives column
	char *s, 	// NULL or receives text
	size_t sSize )	// size of *s

/* and gets gets 1 or 2 preceding line(s) into buffer 's' if necessary
   to include nonblank text before position -- for example, to show
   pertinent text when ';' omitted at end preceding line. 11-90. */
{
	int n, m;
	char *p, *q;

#define GLT 3	/* number of leading nonwhite chars that constitute sufficient text:
	> 1, so ) or ; alone don't count;
	//< 3, so "go" does NOT show preceding unrelated line.
	< 4, so "run" does NOT show preceding unrelated line.
	2->3  6-3-92 cuz GO is now RUN. */

// starting subscript in buffer: a char somewhere in token in line
	int i = flag==0 ? uliBufi : flag==1 ? uliBufi1 : uliBufi2;

// line text
	// this code must fail softly if i is < 0 or > # chars in buf,
	// as uliBufi1,2 can be out of buffer if long comment separates tokens.
	if (i <= uliBufn)				// if i in text in buffer. rejects "i < 0" cuz USI.
	{
		// p = start of line: after last newline before ulibuf[i]
		p = uliBuf + i;				// point into line
		int glt = 0;					// got-leading-text flag/count
		while (p > uliBuf  &&  *(p-1) != '\n')	// find beginning
		{
			p--;
			if (!isspaceW(*p))		// if nonblank
			glt++;			// got leading text
		}
		// length of line: to first newline
		n = strcspn( p, "\f\r\n");  	// # chars to 1st \n (else all)
		q = p;
		m = n;			// start and length of (last) line, for figuring col

		// and back up p to include up to 2 preceding lines if nec for nonblank text b4 posn
		// this logic also in pptok.cpp:ppErv().
		if (glt < GLT)			// if don't have (enuf) leading text
		{
			int nlnl;
			for (nlnl = 0;  p > uliBuf  &&  n < int( sSize)-2;  p--, n++)
			{
				if (*(p-1)=='\n')		// if on 1st char of a line
				{
					if ( glt >= GLT		// if have leading text
							|| nlnl > 2 ) 	// or have scanned enuf lines
						break;		// done
					nlnl++;			// # leading newlines
				}
				else if (!isspaceW(*(p-1)))	// test if nonblank
					glt++;			// # leading nonwhite chars
			}
			while (nlnl && *p=='\n')	// now delete leading blank lines
			{
				nlnl--;
				p++;
				n--; 	//  (well, null lines anyway)
			}
		}
	}
	else				// out of buffer (incl "negative" USI)
	{
		n = 0;
		m = 0;		   	// return null line text
		q = p = NULL;
	}
	if (s != NULL)
	{	setToMin( n, int( sSize)-1); 	// truncate at buffer size: insurance, 9-95.
		memmove( s, p, n);				// return line text
		*(s+n) = '\0';					// terminate returned text
	}
	// p=start, n=length of text returned, conditionally including preceding lines.
	// q=start, m=length of last line, for determining column

	// determine column, with safety checks
	int col;
	if (!q  ||  uliBuf + i < q)
		col = 0;
	else
	{
		for (col = 0;  q < uliBuf + i;  q++)	// scan characters left of position
		{
			if (*q=='\t')
				col |= 7;				// tab: next multiple of 8 (fall thru col++)
			col++;				// count columns
		}
		if (col > m)
			col = m;
	}
	if (pcol)
		*pcol = col;
}		  	// cufCline


/*********************** ERROR MESSAGE INTERFACES **************************/

// (error variables: see  errCount() in rmkerr.cpp.)

//===========================================================================
RC CDEC cuEr( int retokPar, const char* message, ...)

// errmsg with file line text, caret, file name & line #, text retrieval for handle, printf formatting

// returns RCBAD.
{
	va_list ap;

	va_start( ap, message);					// point args
	return cuErv( 1, 1, 1, retokPar, 0, 0, 0, message, ap);	// just below
}						// cuEr
//===========================================================================
RC CDEC cuEr( 	// non-va_list interface to cuErv, next, for error message with all options

	int shoTx, int shoCaret, int shoFnLn, int retokPar,	// see descriptions for cuErv, just below
	int fileIx, int line, int isWarn,
	char *fmt, ... )			// message (or handle) and args to insert like sprintf

// returns RCBAD for convenience if isWarn is 0, else RCOK
{
	va_list ap;
	va_start( ap, fmt);							// point args
	return cuErv( shoTx, shoCaret, shoFnLn, retokPar, fileIx, line, isWarn, fmt, ap);	// just below
}						// cuEr
//===========================================================================
RC cuErv( 	// errmsg with optional preprocessed file line text, caret, file name & line #, and with vprintf formatting

	int shoTx, 			// nz to show text of current input line
	int shoCaret, 		// nz to put ^ under error position
	int shoFnLn, 		// nz to show CURRENT input file/line
	int retokPar,		// nz forces prior token (re shoTx/shoCaret) -- most callers must pass cuparse.cpp:reToke here.
	// fileIx and line used only if shoTx, shoCaret, shoFnLn ALL 0: otherwise current input values are used.
	int fileIx,			// 0 or input file name index whose name to show
	int line,			// line # to show with fname
	int isWarn,  		// 0: "Error" (errCount++'s), 1: "Warning" (display suppressible), 2: "Info"
	const char *fmt, va_list ap )	// message (or handle) and vsprintf args to insert

// returns RCBAD for convenience if isWarn is 0, else RCOK (a CHANGE 7-14-92)
{
	char cmsg[750], tex[ULIBUFSZ+2], caret[139], where[139], 	// ULIBUFSZ (770, 9-95): above in this file.
		whole[MSG_MAXLEN];					// MSG_MAXLEN: messages.h
	int col = 0;

	caret[0] = where[0] = tex[0] = '\0';


// format caller's msg and args

	msgI( WRN, cmsg, sizeof( cmsg), NULL, fmt, ap);		// retrive text for handle (if given), vsprintf.  messages.cpp.


// get current input file name, line #, line text
	/* note: this gets preprocessor output (decommented, macro'd) --
	         different from text shown by pp.cpp:ppErv(); macro expansions can make very long lines. */


	if (shoTx|shoCaret|shoFnLn)					// else leave fileIx/line args unchanged
	{
		cuCurLine( retokPar, &fileIx, &line, &col, tex, sizeof(tex)-1 );	// just below. -1: room for \n.
		strcat( tex, "\n");  						// newline after file line
	}

	lisThruLine(line);	// echo input to listing thru end of current input FILE line
       					// (expect start of line is already echoed & proper Lty set; this is to
       					// complete line & prevent error message inserted in mid-line). pp.cpp.

// make up 'where': "<file>(<line>): Error/Warning: "

	if (shoFnLn||fileIx)
		sprintf( where, "%s(%d): %s: ",
			getFileName(fileIx), (INT)line,
			isWarn==1 ? "Warning" : isWarn==2 ? "Info" : "Error" );

// make up line with caret spaced over to error column

	if ( shoCaret  			// if ^ display requested
	 &&  col < sizeof(caret)-3  	// if ^ position fits buffer
	 &&  *tex	  			// skip if null ret'd for file line:
							// eg if prev token not in buf due to long comment: ^ wd be confusing
	 &&  strlen(cmsg) + strlen(tex) + strlen(where) + col + 10
	    <  MSG_MAXLEN )		// skip if would come close to exceeding max msg length (messages.h) 9-95
	{
		int lWhere = strlen(where);
		if (shoFnLn && col >= lWhere + 3)		/* if col is right of end of "where" line,
       							   append ^ thereto: save line & avoid ugly gap. */
		{
			if ( !strchr( cmsg, '\n')				// if callers message is all on one line
			 &&  int( strlen(cmsg) + lWhere + 3)	// and it also will fit ...
			        <= min(col, getCpl()-8) )		//  left of caret with plenty of space in line
			{
				strcat( where, cmsg);				// move caller's message into "where" to be before ^
				cmsg[0] = 0;					// ..
				lWhere = strlen(where);				// ..
			}
			memset( where + lWhere, ' ', col - lWhere);		// space over
			strcpy( where + col, "^");				// append ^
			if (col < getCpl()-8 -4 -1)  				// if 1-char cmsg could fit on same line (below)
				strcat( where, "    ");				// spaces to separate it; else avoid exceeding cpl.
		}
		else if (shoFnLn  &&  col < 10 			// if col small, insert ^ at left end of "where" line
		      && lWhere + col < getCpl() - 20)
		{
			memmove( where + col + 4, where, lWhere+1);
			memset( where, ' ', col + 4);
			where[col] = '^';
		}
		else						// basic case: ^ on separate line
		{
			/*if (col > 0) unneeded insurance?? */
			memset( caret, ' ', col);
			strcpy( caret + col, "^\n");
		}
	}

// truncate file line text if would otherwise exceed MSG_MAXLEN (buffer size in rmkerr.cpp, vrpak.cpp, etc), 9-95

	int lenRest = strlen(cmsg) + strlen(where) + strlen(caret) + 6;		// +6 for "\n  " and paranoia
	if (shoTx)
		if (lenRest + strlen(tex) > MSG_MAXLEN)			// if too long incl file line text
			if (lenRest < MSG_MAXLEN)				// if the rest fits (believe assured by buf sizes if cmsg fits)
				strcpy( tex + (MSG_MAXLEN - lenRest), "\n");	// truncate tex[] & append newline


// assemble complete text

	sprintf( whole,
		"%s"		// line text (or not)
		"%s"		//     ^ (or not, or with 'where')
		"%s%s"		// where (or not) and possible newline
		"%s", 		// caller's message
		shoTx ? tex : "",
		caret,
		where,   *cmsg && strJoinLen( where, cmsg) > getCpl() - 8  ?  "\n  "  :  "",	//-8 for cols added in input listing
		cmsg );


// output message to err file, screen, and/or err report; increment error count.

	errI( REG, isWarn, (const char*)whole);		// rmkerr.cpp.

// output message to input listing if in use; attempt to insert after file line.

	{
		// variable(s)
		int place;
		if ( shoTx					// if file line shown
			&&  tex[0]					// and nonNull file line found
			&&  lisFind( fileIx, line, tex, &place) )  	// and matching place found in listing spool buffer (pp.cpp)
		{
			// reassemble message without file line(s) text
			sprintf( whole,
				"%s"   		//     ^ (or not, or with 'where')
				"%s%s"		// where (or not) and possible newline
				"%s", 		// caller's message (or with where)
				caret,
				where,  *cmsg && strJoinLen( where, cmsg) > getCpl() - 8  ?  "\n  "  :  "",	//-8 for cols added in listing
				cmsg );
			// insert message after found file line, with -------------- following and preceding (pp.cpp fcn)
			lisInsertMsg( place, whole, 1, 1);
		}
		else		// no file line in msg or cd not find match (too far back, b4 an include (flushes buffer), etc)

			lisMsg( whole, 1, 1);				// same message as screen to listing if in use, pp.cpp
	}
	return isWarn ? RCOK : RCBAD;		// warnings & infos return ok 7-92, errors return bad, for convenience.
}					// cuErv
//===========================================================================
void cuCurLine( 		// re last token: return file line text and info

	int retokPar,	// nz for previous token even if have not cuUntok'd
	//  CAUTION: most callers should pass cuparse:reToke here, or use cuparse:curLine.
	int* pFileIx,	// NULL or receives file name index of current input file name. Get text with ancrec.cpp:getFileName().
	int* pline,		// NULL or receives line # in file
	int* pcol, 		// NULL or receives column # (subscript) in line
	char *s,		// NULL or receives line text. Should be 770+ chars.
	size_t sSize )		// sizeof(*s) for overwrite protection, 9-95

/* for caller issuing compile error message showing bad statement,
     and for obtaining file/line to put in each created object record --
     for setting record-derived-class .file and .line.
	Adjusts for ungotten token.
	Intended to respond "right" to newlines between tokens; newlines within tokens not expected. */
{

	BOO re;	/* non-0 to return info for previous, not current token:
		   if cuRetok set (indicating cuUntok has been called) OR caller passes non-0 retokPar argument. */

	re = cuRetok | retokPar;		// but the most-used retoke flag is cuparse:reToke, 11-91.

	if (pline)
		*pline = re ? pvCutFline : cutFline;	// line # in file

	if (pFileIx)  *pFileIx = uliFileIx;		// return input file name index if requested

	if (pcol || s)			// if either requested
		cufCline( re ? 2 : 1, 			// 1 start curr token, 2 previous tok.
			pcol, s, sSize );		// get col # & line text, local, above.

}			// cuCurLine

// end of cutok.cpp
