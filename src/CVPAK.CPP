// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* cvpak -- routines for interconversion of various internal/external
            format values.  See also tdpak.c for time/date routines. */

#include <cnglob.h>

/*-------------------------------- OPTIONS --------------------------------*/

/*------------------------------- INCLUDES --------------------------------*/
#include <msghans.h>	// MH_xxxx

#include <srd.h>	// Unsysext, Untab, GetDttab
#include <tdpak.h>

#include <cvpak.h>	// decls for this file

/*-------------------------------- DEFINES --------------------------------*/

/*--------------------------- PUBLIC VARIABLES ----------------------------*/

/*---------------------------- LOCAL VARIABLES ----------------------------*/
static int Cvnchars = 0;	// # output chars from internal to external conversion.
// cvin2s and callees working variables
#ifdef FMTPVMASK		// define in cvpak.h to restore p positive value display options, 11-91
p static SI NEAR pv;		// positive value display: null, +, spc
p static SI NEAR ipv;		// ditto right-shifted for use as subscript
p	/* note positive value options not used (except check debugpr.c);
p	   could remove LOTS of code in cvpak, rob grep 10-88. Done 11-91. */
#endif
static USI NEAR fmt;		// cvin2s "format" argument; see cvpak.h for field definitions.
static USI NEAR mfw;		// cvin2s "max field width" argument.
static SI NEAR just;		// justification: left, rt, rz, squeeze.
static SI NEAR ijust;		// ditto right-shifted for use as subscript.
static SI NEAR lj;		// left justify (just & FLMLJ).
static SI NEAR lz;		// leading 0's (right justify).  note FMTLZ not used, could code out. rob 10-88
static SI NEAR wsign;		// 0 or 1 columns for sign, set by cvin2s
static SI NEAR wid;		// width for sprintf: 1 for squeeze, else mfw. cvdd expects caller to set.
static SI NEAR ppos;		// precomputed sprintf precision (prcsn) for several cases for values >= 0
static SI NEAR pneg;		// precomputed sprintf precision (prcsn) for serveral cases for values < 0
static double NEAR aval;	// fabs(val): set by cvsd, cvdd, ft-in; updated by nexK.
static double NEAR val;		// value for float/double print cases
static char * NEAR str;		// Tmpstr destination location

/*----------------------------- INITIALIZED DATA --------------------------*/
#ifdef FMTPVMASK	// define in cvpak.h to restore p positive value display options, 11-91
p
p /*   Idea: left-justify format appears to ALWAYS differ by having leading -;
p 	   could create from base format at run time ? */
p /*   These format definitions must match definitions of FMTPVxxx in cvpak.h */
p        /* ipv: if >= 0, show null,    +,	 space */
p add NEAR's if restored.
p /* SI and floating 0 formats */
p static char *sif[2][3] =   {"%*.*d",  "%+*.*d",  "% *.*d",      /* other */
p 			    "%-*.*d", "%-+*.*d", "%- *.*d"};    /* left just */
p static char *lif[2][3] =   {"%*.*ld", "%+*.*ld", "% *.*ld",     /* other */
p 			    "%-*.*ld","%-+*.*ld","%- *.*ld"};   /* lj */
p static char *usif[2] =	   {"%*.*u",                            /* other */
p 			    "%-*.*u"};                          /* lj */
p static char *sf[2]     =   {"%*.*s",                            /* other */
p 			    "%-*.*s"};				/* lj */
p /* basic float: done in cvdd(). */
p
p /* for trim trailing 0's. ipv:
null  +           spc */
p static char *gf[4][3] = {"%-*.*g",  "%-+*.*g",  "%- *.*g",   /* left j */
						   p 			 "%*.*g",   "%+*.*g",   "% *.*g",    /* rj */
						   p 			 "%0*.*g",  "%0+*.*g",  "%0 *.*g",   /* rj 0s */
						   p 			 "%*.*g",   "%+*.*g",   "% *.*g"
						  };   /* sqz */
p
p /*  IP lengths. subscript ipv: null,	 +,		  space */
p /* dfw 0, feet and inches */
p static char *ff1[3]   = {"%*.*ld'%*d",   "%+*.*ld'%*d",   "% *.*ld'%*d"};
p /* dfw 0, inches only */
p static char *ff3[3]   = {"%*.*d",        "%+*.*d",        "% *.*d"};
p /* dfw nz, feet and inches */
p static char *ff4[3]   = {"%*.*ld'%*.*f", "%+*.*ld'%*.*f", "% *.*ld'%*.*f"};
p /* dfw nz, inches only */
p static char *ff5[3]   = {"%*.*f",        "%+*.*f",        "% *.*f"};
p
#else
// sprintf formats, used in cvin2s, cvin2Ft_in.  Basic float formats are in cvdd.
//
//		Idea: left-justify formats appear to differ by leading -; create from base format at run time ?
//
// SI and floating 0 formats.   other     left just
static char * NEAR sif[2] =  { "%*.*d",   "%-*.*d"  };
static char * NEAR lif[2] =  { "%*.*ld",  "%-*.*ld" };
static char * NEAR usif[2] = { "%*.*u",   "%-*.*u"  };
static char * NEAR sf[2] =   { "%*.*s",   "%-*.*s"  };
//
// for trim trailing 0's:        left j      rj        rj 0s      sqz
static char * NEAR gf[4] = {    "%-*.*g",  "%*.*g",  "%0*.*g",  "%*.*g"  };
//
// IP length formats
static char * NEAR ff1 = "%*.*ld'%*d";  	// dfw 0, feet and inches
static char * NEAR ff3 = "%*.*d";		// dfw 0, inches only
static char * NEAR ff4 = "%*.*ld'%*.*f";	// dfw nz, feet and inches
static char * NEAR ff5 = "%*.*f";		// dfw nz, inches only

#endif

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL SI FC NEAR cvttz( char *str, SI trailChar, SI minWid);
LOCAL BOO FC NEAR nexK();
LOCAL BOO FC NEAR cvsd( SI mfw, SI dfw);
LOCAL BOO FC NEAR cvdd( SI mfw, SI dfw);
LOCAL void FC NEAR cvDouble2s(void);
LOCAL void FC NEAR cvFtIn2s(void);
LOCAL LI FC NEAR sepFtInch( double d, SI *inp);

// unmaintained test code is at end

//======================================================================
char * FC cvin2sBuf( char *buf, void *data, USI dt, SI units, USI _mfw, USI _fmt)

// Convert internal format data to external format string in caller's buffer

// buf is NULL (for Tmpstr) or destination; other args as cvin2s (next)

// formerly cvin2str(), 9-89.
// returns buf (or Tmpstr location if NULL given).
// Also sets Cvnchars to strlen(result).
{
	char *p;

	p = cvin2s( data, dt, units, _mfw, _fmt);	// convert (next)
	if (p==NULL)					// if data is NULL, cvin2s nops 9-89
		return NULL;				/* nop here too 9-89 (putting "" or right # blanks
      						   in buf might make more sense?). */
	return buf ? strcpy( buf, 	// copy into caller's buffer
		strtempPop(p)) 			// free Tmpstr space (so less likely to wrap onto caller's stuff).
                       			// recursive. returns p. strpak.c.
	: p;				// no buffer given, return Tmpstr location

}		// cvin2sBuf

//======================================================================
char * FC cvin2s( 		// Convert internal format data to external format string in Tmpstr

	const void* data,	// Pointer to data in internal form, or NULL to do nothing and return NULL
						//  (for DTCHP, is ptr to ptr to string to print, 11-91) */
	USI dt, 		// Data type of internal data, or DTNA for "--" or DTUNDEF for "?" from cvfddisp()
	SI units,		// Units of internal data (made signed 5-89)
	USI _mfw,		// Maximum field width (not including '\0').  If requested format results in string longer
    				// than mfw, format will be altered if possible to give max significance within mfw;
					// when not possible (never possible for inteters or if field too narrow for e or k format),
					// field of **** is returned (but see _xfw).
	USI _fmt,		// Format.  See cvpak.h for definition of fields.
	USI _xfw /*=0*/ )	/* 0 or extra field width available: if value cannot be formatted in _mfw columns does fit
    			   in _mfw + _xfw or fewer cols, return the overlong text rather than ******.  added by rob, 4-92. */

// Returns pointer to result in Tmpstr.
// Also sets global Cvnchars to the number of characters placed in str (not incl. '\0').

// This routine will in in some cases overwrite one or more chars beyond the specified field.  \0 always appended.

{
	SI allocLen;			// size for str
#ifdef DTPERCENT
	SI percent = FALSE;		// set TRUE if converting a DTPERCENT; shares DTFLOAT code
#endif

	fmt = _fmt;			// store format arg for use by callees
	mfw = _mfw;			// .. max field width arg

	/* NULL data pointer means do NOTHING */	/* for caller convenience in supporting optional stuff, tentative 9-89.
							   (note pgw also nops on getting null 9-89; using 9-89 in crlout.c) */
	if ( data==NULL
#ifdef DTUNDEF
	&&  dt != DTUNDEF 		// these use no data and "data" may be garbage
#endif
#ifdef DTNA
	&&  dt != DTNA		// .. from cvpak3:cvfddisp()
#endif
	   )
		return NULL;		// or should it ret blank field? set Cvnchars? 9-89.

// Allocate temporary string space.
	allocLen = mfw+3+2;				// +3: some paranoia space, at least 1 needed.
    								// +2: for FMTUNITS space or FMTPU ()'s
	if (fmt & (FMTUNITS|FMTPU))			// if units to be appended
		allocLen += strlen( UNIT::GetSymbol( units) );
	if (allocLen < 13)  allocLen = 13;		// always enuf for "<unset>\0" or "<expr 99999>\0" 2-27-92
	str = strtemp( allocLen);				// strpak.c; strtempPop deallocates.

	/* Common setup */

#ifdef FMTPVMASK	// define in cvpak.h to restore p positive value display options, 11-91
p    pv = fmt&FMTPVMASK;		// positive value display: null, +, spc
p    ipv = ((USI)pv) >> FMTPVSHIFT;	// .. shifted for use as subscript -- frequently used to select formats
p					// (shift made unsigned for lint)
#endif
	just = fmt&FMTJMASK;		// justification: left, rt, rz, squeeze
	ijust = ((USI)just) >> FMTJSHIFT;	// .. shifted for use as subscript
	// (shift made unsigned for lint)
	lj = just==FMTLJ;			// left justify flag
	lz = just==FMTLZ;			// leading zeroes (right justify) flag
	wid = just==FMTSQ ? 1 : mfw; 	// width for sprintf: 1 to squeeze (float only). note cvdd uses file-global.
	// for several cases, preset sprintf precision (ppos, pneg, prcsn): min digits for many formats other than e, f.
	if (lz)				// if leading zero format requested
	{
#ifdef FMTPVMASK
p       ppos =				// precision positive/unsigned data
p			pv==FMTPVNULL 		// if null for +,
p				? wid : wid - 1;		// full width, else 1 less
#else
		ppos = wid;			// precision for positive/unsigned data is full width
#endif
		pneg = wid - 1;			// precision for neg several cases: leave column for -
	}
	else				// for other formats
		ppos = pneg = 1; 		// init pos and neg precision to 1

	/* Datatype specific internal-->external processing */

	switch (dt)
	{
#ifdef FMTPVMASK
p    case DTSI:
p			if (*(SI *)data == 0 && pv==FMTPVPLUS)
p		       pv=FMTPVSPACE;
p		    Cvnchars = sprintf( str,  sif[lj][ipv],  wid,  *(SI *)data < 0 ? pneg : ppos,  *(SI *)data);
p	            break;
#else
	case DTSI:
		Cvnchars = sprintf( str,  sif[lj],  wid,  *(SI *)data < 0 ? pneg : ppos,  *(SI *)data);
		break;
#endif

	case DTUSI:
		Cvnchars = sprintf( str, usif[lj], wid, ppos, *(USI *)data);
		break;

#ifdef FMTPVMASK
p    case DTLI:
p		if (*(LI *)data == 0 && pv==FMTPVPLUS)
p			pv=FMTPVSPACE;
p		Cvnchars = sprintf( str,  lif[lj][ipv],  wid,  *(LI *)data < 0 ? pneg : ppos,  *(LI *)data);
p		break;
#else
	case DTLI:
		Cvnchars = sprintf( str,  lif[lj],  wid,  *(LI *)data < 0 ? pneg : ppos,  *(LI *)data);
		break;
#endif

	case DTDOY:
		data = tddys( *(DOY *)data, TDYRNODOW, NULL);
		goto strjust;

	case DTDOW:
		data = tddDowName( *(SI *)data);
		goto strjust;

	case DTMONTH:
		data = tddMonAbbrev( *(SI *)data);
		goto strjust;

	case DTIDATE:
		data = tddis( *(IDATE *)data);
		goto strjust;

	case DTITIME:
		data = tdtis( (ITIME *)data, NULL);
		goto strjust;

	case DTIDATETIME:
		data = tddtis( (IDATETIME *)data, NULL);
		goto strjust;

#ifdef DTLDATETIME
	case DTLDATETIME:
		data = tdldts( *((LDATETIME *)data), NULL);
		goto strjust;
#endif

	case DTDBL:
		val = *(double *)data;
		goto valValue;

#ifdef DTPERCENT	// put back in cndtypes.def to restore code, 12-3-91
	case DTPERCENT:
		val = (*(float *)data)*100.;
		mfw--;
		if (wid > 1)
			wid--;
		percent = TRUE;		// flag tested after float formatting
		goto valValue;		// join float
#endif

#ifdef DTSSE	// removed from CSE 12-91 rob
		/* Special data type for source seasonal efficiency: If value is >= 0., display it as a floating point number.
		   If negative, display "Hyd", to indicate that system is Hydronic and does not have a defined SSE */
	case DTSSE:
		if (*(float *)data < 0.f )
		{
			data = "Hyd";
			goto strjust;		// Display as string
		}
		// Fall through to DTFLOAT
#endif

	case DTFLOAT:
floatCase:				// number-choice comes here (from default) if does not contain choice

		if (!ISNUM( *(void **)data))		// check for non-number, cnglob.h macro, debug aid 2-27-92.
		{
			if (ISNCHOICE( *(void **)data))  		// if number-choice choice (nan; unexpected here)
				goto choiceCase;
			if (ISNANDLE( *(void **)data))			// if unset or expr n (nan's) (insurance)
			{
				if (ISUNSET( *(void **)data))
					strcpy( str, "<unset>");				// say <unset>
				else
					sprintf( str, "<expr %d>", (INT)EXN(*(void **)data));	// say <epxr n>
				break;
			}
		}
		val = *(float *)data;			// conver float value to print to double
valValue: 				// double, [percent] join here
		val = cvIntoEx( val, units);		// convert value to ext units
#ifdef FMTPVMASK
p		wsign = !(pv==FMTPVNULL && val >= 0.);	// sign width
#else
		wsign = (val < 0.);				// sign width
#endif
		if (units == UNLENGTH && Unsysext==UNSYSIP)		// if feet-inch float/double value
			cvFtIn2s();				// convert 'val' to '*str' as feet-inches, local function, below.
		else
			cvDouble2s();		// convert 'val' to '*str', using many other variables. local function, below.
#ifdef DTPERCENT
		if (percent)		// finish percent: add '%'; make 1 col wider due to wid-- above.
		{
			SI i;
			SI l = strlen(str);
			for (i = 0; ; i++)   if (*(str+i) != ' ')  break;  	// find nonspace
			while (*(str+(++i)))  if (*(str+i)==' ')   break;  	// find space after the nonspace, or end
			*(str+i) = '%';						// put % there
			if (i != l)  *(str+l) = ' ';				// append space unless just put % there
			*(str+l+1) = '\0';					// reterminate
		}
#endif
		break;

	default:		// a choice or number-choice data type -- else error
		if (dt & DTBCHOICN)						// if a number-choice see if now contains number
		{
			if (!ISNCHOICE( *(void **)data))				// if not a choice (cnglob.h macro)
				goto floatCase;					// (numbers, UNSET, NANDLES branch)
			fmt &= ~(FMTUNITS|FMTPU);				// suppress units when number-choice is choice
		}
choiceCase:								// float types comes here if NCHOICE found (unexpected)
		if (dt & (DTBCHOICB|DTBCHOICN))				// if a choice type
		{
			SI handle = (dt & DTBCHOICN)
					    ?  CHN(*(void **)data)	// extract choicn choice value (cnglob.h macro)
			            :  *(SI *)data;			// choicb choice handle/index
#if 0	// not used in CSE 9-2-91
x       if (dt==DTPERINCH && Unsysext != UNSYSIP)
x         // fudge inches to mm for per-inch in other system
x		{	if (handle == C_PERINCH_YES)
x				handle = C_PERMM_YES;
x			else if (handle == C_PERINCH_NO)
x				handle = C_PERMM_NO;
x		}
#endif
			data = ::getChoiTxI( dt, handle); 		// get choicb/choicn text
			goto strjust;
		}
		else                			// not a choice type
			goto undef;

#ifdef DTNA
	case DTNA:
		data = "--";
		goto strjust;
#endif

	case DTCHP:
		data = *(char * *)data;
		goto strjust;	// data is pointer to string

#if 0	// untested idea, 1-1-11
x	case DTCSTRING:
x		data = (const char *)(CString *)data;
x		goto strjust;
#endif

	case DTCH:				// for char array or string ptr already dereferenced, rob 11-91
	case DTANAME:			// char[ ] RAT name
strjust:
		Cvnchars = sprintf( str, sf[ lj], wid, mfw, data);
		break;

#ifdef DTUNDEF
	case DTUNDEF:
#endif
undef:
		if (wid > 1)
			memset( str, ' ', wid);
		if (lj)
			*str = '?';
		else
			*(str+wid-1) = '?';
		*(str+wid)='\0';
		Cvnchars = wid;
		break;

	}	// switch (dt)

	/* if still too wide [and not variable data], fill with *******.  Note most floating values have been made to fit. */

	if ((USI)Cvnchars > mfw + _xfw)	// return oversize field anyway if <= _xfw (argument) chars overwide
	{
#ifdef DTPERCENT
		if (percent)
		{
			mfw++;
			percent = FALSE;
		}
#endif
		memset( str, '*', mfw);  	// fill with ******
		*(str + mfw)='\0';
		Cvnchars = mfw;
	}

	/* optionally add units.  Note ' " for ft-in done in cvFtIn2s.  Suppressed for NCHOICE choice by clearing bits above. */

	if (fmt & FMTUNITS)
	{
		strcat( str, " ");
		strcat( str, UNIT::GetSymbol( units));	// concat units text
	}
	else if (fmt & FMTPU)		// parenthesised units for res loads 2-90
	{
		strcat( str, "(");
		strcat( str, UNIT::GetSymbol( units));	// concat units text
		strcat( str, ")");
	}

	return str;
	// another return at entry
}				// cvin2s

//======================================================================
//--- data shared by cvDouble2s, cvsd, nexK 4-92
static SI NEAR amfw;				// set by cvdd/cvsd: mfw adjusted for sign. used in cvDouble2s, nexK,
static SI NEAR ki;				// 0 or number of times divided by 1000 for K format. cvDouble2s inits.
static SI nDigB4Pt;				// number of digits b4 point, set in cvsd/cvdd, adj in nexK
static const char ddalpha[]=" kMGTPE???????"; 	// Chars for K format output field overflow handling
//======================================================================

// 4-92: needs completion of review re producing minimum-overwidth result for xfw -- cvsd at least done.

LOCAL void FC NEAR cvDouble2s()     	// float / double output conversion case for cvin2s

// converts 'val' to '*str'; uses other global statics including: fmt, mfw, val, str, lj, lz, ppos, .

// do not call for foot-inch conversion: see cvFtIn2s.
{
	SI i;
	SI dfw = fmt & FMTDFWMASK;		// decimal field width (# decimal places)
	ki = 0;				// say not in K format overflow (cvsd/nexK)

// zero exception

	if (val==0.)
	{
#ifdef FMTPVMASK
p       if (pv==FMTPVPLUS) 		// show not +
p			pv=FMTPVSPACE;			// but space
p       Cvnchars = sprintf( str, sif[lj][ipv], wid, ppos, 0);
#else
		Cvnchars = sprintf( str, sif[lj], wid, ppos, 0);
#endif
		return;
	}

// trim trailing zeroes (specified significant digits) option

	if (fmt & FMTRTZ)				// "trim trailing zeroes" option
	{
		if (cvsd( mfw, dfw))			// convert (returns FALSE if should use cvdd: number too small relative to space)
			return;				// if converted (returns best fit if overwide; caller checks Cvnchars)
		//else fall thru to basic conversion to get 0.000 for small numbers, rather than e- format nor overwide .000000n
	}	    // if FMTRTZ

// basic floating conversion (not ft-in, not 0, not FMTRTZ)

	if (cvdd( mfw, dfw))		// drifting decimal conversion, nz ok.  Uses str, val, etc; sets amfw; below.
		return;				// if successful in mfw columns

// field overflowed (rest of function)

	if ((fmt & FMTOVFMASK)==FMTOVFK)	// if overlow is to be handled with k format
	{

		// float overflow, K format option (default).  Could recode with new fcn nexK()... 4-92

		// 4-92: needs review re producing minimum-overwidth result for xfw.

		//static char ddalpha[]=" kMGTPE???????"; 	// Chars for K format output  is above
		double maxfit;

		if (amfw < 2)				// set by cvdd: mfw adj for sign
			return;				// if can't possibly fix (and Pten does not go negative)
		amfw--;					// cols less sign: reduce 1 for kMG..
		maxfit = Pten[amfw]-0.5;			// protection for amfw > sizeof Pten needed ??? 10-88
		if (wid > 1)				// desired width, 1 for FMTSQ
			wid--;				// leave a col for kMG
		for (i = 0; ; i++)
		{
			// more robust to give up HERE if i too big ?? 10-88 rob
			if ( fabs(val) < maxfit		// if now might fit width
			&&  cvdd( mfw-1, dfw) )  		// format it and see
				break;				// if now ok
			val /= 1000.;				// divide by 1000 and bump i till it works
		}
		if (ddalpha[i]=='?') 			// if out of range of char table
			Cvnchars = mfw+1;			// I think this causes ***** output
		else
		{
			*(str+Cvnchars) = ddalpha[i]; 	// append k, M, G, ...
			Cvnchars++;
			*(str+Cvnchars) = '\0';
		}
	}
	else				// cvdd fail and not K format
	{

		// float overflow, e format

		// 4-92: needs review re producing minimum-overwidth result for xfw.


		SI owid = wid;			// save wid b4 exponent
		SI oamfw = amfw;			// ditto max field wid less sign
		SI ew = 2;			// init exponent width
		i = 3;				// exponent: start at 3 to gain 1 column
		val /= 1000.;			// adjust value to match exponent
		while (1)				// until fits or fails
		{
			if (oamfw-ew < 1)			// if exp leaves no place for value
				break;					// give up
			// I think Cvnchars is too big -> ***** on fallthru w exp
			if (fabs(val) < Pten[oamfw-ew])		// if could fit cols
				// no protection for amfw > size of Pten?? rob 10-88
			{
				if (wid > 1) 				// (starts 1 for FMTSQ)
					wid = owid-ew;				// adjust wid: cvdd uses
				if (cvdd( mfw-ew, dfw))  			// format / if fits
					break;					// ok, go add exponent
			}
			val /= 10.;			// divide by 10, bump i till it works
			if (++i == 9)			// exponent 10
				ew++;			// requires extra column
			// so why ++ at 9 ??? rob 10-88
		}
		sprintf( str+Cvnchars, "e%d", (INT)i);		// add exponent i
		Cvnchars += ew;
	}
	// additional returns above
}		// cvDouble2s
//======================================================================
LOCAL void FC NEAR cvFtIn2s()      	// feet-inch length output conversion case for cvin2s

// converts 'val' to '*str'; uses other global statics including: fmt, mfw, val, str, lj, lz, ppos, .
{
	SI biglen = (aval > 178000000.);	// TRUE to show feet only: set if > max LI inches, also set below if too wide.
	if (!biglen)			// if feet not too big to express inches in LI (in sepFtInch): ie normally
	{
		SI prcsn;				// sprintf 'precision': min # digits for most cases
		SI sq = (just==FMTSQ);		// squeeze (minimum columns) flag (also 'wid' is 1)
		SI fw;				// feet or inches-only width
		SI inw;				// inches subfield width (with feet)
		SI indfw;			// decimal places for inches
		LI ft;				// feet of ft-inches value
		SI inch;				// inches
		double dinch;			// floating inches where needed
		SI justInches;			// TRUE to omit feet: 0 < l < 1'0"
		aval = fabs(val);		// absolute val
#ifdef FMTNOQUINCH			// define in cvpak.h to restore feature, 11-91
x       SI quinch = !(fmt & FMTNOQUINCH);		// 1 for " after inches
x       ft = sepFtInch( val, &inch);			// separate/fix feet, inches
x       justInches = (quinch && ft == 0L && inch != 0);
x				// TRUE to omit feet; never happens if quinch is off -- prevents ambiguous single numbers.
#else	// 11-91 quinch coded out as 1
		ft = sepFtInch( val, &inch);		// separate/fix feet, inches
		justInches = (ft == 0L && inch != 0);	// TRUE to omit feet
#endif

		// digits after decimal point in INCHES

		indfw = fmt & FMTDFWMASK;		// init decimal places for inches (same value as former 'dfw')
		if (fmt & FMTRTZ)			// with truncating trailing 0's optn,
		{
			// dfw is total sig digs, not digits after .
			// Reduce indfw to digits to print AFTER DECIMAL in INCHES.
			SI ndig = 0;
			while (indfw > 0  &&  Pten[ndig++] <= aval)	// while value < power of 10
				indfw--;					// reduce for each feet digit
			// (need no Pten size cks here due to 178000000 ck above)
			ndig = 0;
			dinch = (float)abs(inch);
			while (indfw > 0  &&  Pten[ndig++] <= dinch )	// while inches < pwr of 10
				indfw--;					// reduce for each inches digit b4 .
		}

		// printf width/space needed for inches if with feet

		inw = justInches ? 0		// for inches-only fw is used, else
		:  2				// 2 for integral inches (space b4 0-9; exact inw needed for rj) ..
		- sq				// but only 1 col if squeezing (no space b4 squeezed 0-9; inw < actual ok when sq on)
		+ (indfw!=0)			// 1 for . if needed
		+ indfw;			// digits after .

		// printf width for feet (or inches if no feet)

		fw = lj ? 1			// 1 to left-justify, else
		: wid - inw			// wid less inch space and
		- (!justInches)		//  less ' space and
#ifdef FMTNOQUINCH
x	   - quinch;			//  less " space
#else
		- 1; 			//  less " space
#endif
		if (fw < 1)			// Prevent fw < 0: else trails blanks to
			fw = 1;			// .. -fw cols. NB wid=1 for FMTSQ (10-88).
		prcsn = lz ? fw-wsign : 1;  	// min # digits most cases

		// format feet-inches

		if (indfw)			// if non-0 # dec places for inches
		{
			// show with decimal inches
			if (justInches)		// if showing inches only
			{
				dinch = val*12.;		// compute float inches
#ifdef FMTPVMASK
p				Cvnchars = sprintf( str, ff5[ipv],		// *.*f
p								fw, indfw,		// width, precision
p								dinch );		// floating inches
#else
				Cvnchars = sprintf( str, ff5,		// *.*f
								fw, indfw,		// width, precision
								dinch );		// floating inches
#endif
			}
			else 			// feet and inches
			{
				dinch = (val - ft)*((ft >= 0) ? 12 : -12);
#ifdef FMTPVMASK
p				Cvnchars = sprintf( str, ff4[ipv], 	// 2.*f for inches
p						fw, prcsn,		// feet width, digits
p						ft,
p						inw, indfw,		// inches wid, precis
p						dinch );		// floating inches
#else
				Cvnchars = sprintf( str, ff4,		// 2.*f for inches
						fw, prcsn,		// feet width, digits
						ft,
						inw, indfw,		// inches wid, precis
						dinch );		// floating inches
#endif
			}
			if (fmt & FMTRTZ)		// trim trailing zeros option
			{
				// rob 10-88 to support FTMRTZ with FMTSQ
				Cvnchars =
				cvttz( str,			// trim .0's from decimal inches
				'"',			// slide final " leftward
				lj ? 1 : wid );	/* if lj or FMTSQ (wid=1), do fully (lj padded below).
                				   If rj, don't shorten < wid: wd need to pad at
                				   front, defer doing that code til need found. */
			}
		}
		else		// indfw 0: show inches as integer only
		{
#ifdef FMTPVMASK	// define in cvpak.h to restore p positive value display options, 11-91
p
p			if (justInches)			// if showing inches only
p				Cvnchars = sprintf( str, ff3[ipv],
p							fw, prcsn,		// inches wid, digits
p							inch );
p			else 					// feet and inches
p				Cvnchars = sprintf( str,ff1[ipv],		// %2d for inches
p							fw, prcsn,		// feet wid, digits
p							ft,
p							inw,			// inches width
p							inch );
#else
			if (justInches)			// if showing inches only
				Cvnchars = sprintf( str, ff3,
							fw, prcsn,		// inches wid, digits
							inch );
			else 					// feet and inches
				Cvnchars = sprintf( str,ff1,		// %2d for inches
							fw, prcsn,		// feet wid, digits
							ft,
							inw,			// inches width
							inch );
#endif
		}
#ifdef FMTNOQUINCH			// define in cvpak.h to restore feature, 11-91
x
x   // omit zero inches with feet if ALL 3 of these options on (rob 10-88 for text files):
x
x       if (fmt & (FMTSQ|FMTRTZ|FMTNOQUINCH)==(FMTSQ|FMTRTZ|FMTNOQUINCH) )
x		{
x			if ( !justInches				// if feet shown
x	        &&  *(str + Cvnchars-1)=='0'  		// 0 last
x	        &&  !isdigitW(*(str + Cvnchars-2)) )  	// nondigit b4 0
x			{
x             *(str + --Cvnchars) = '\0';		// truncate the 0
x             // quinch = 0;  				* add no " *  quinch is 0 cuz FMTNOQUINCH on if here
x			}
x		}
x
x   // add " for inches unless suppressed
x
x	if (quinch  /* && Cvnchars < mfw * to omit if won't fit */)
x	{  *(str + Cvnchars++) = '"';
x	   *(str + Cvnchars) = 0;
x	}
#else
		// add " for inches

		// if (Cvnchars < mfw) {..  // to omit if won't fit
		*(str + Cvnchars++) = '"';
		*(str + Cvnchars) = 0;
#endif

			// right pad left-justified feet-inch values (wid was 1)

			if (lj)   					// if left-justified
				if ((USI)Cvnchars < mfw)			// (beleived unnec)
					strpad( str," ", (Cvnchars=mfw));		// pad to field width

		// finally, check that fit field

		biglen = (USI)Cvnchars > mfw;   			// set biglen TRUE iff feet-inches too wide for field

	} // if (!biglen)

// if feet-inch value too big, show feet only

	if (biglen)		// if value too big for LI inches at entry, OR feet-inches format exceeded field width
	{
		cvin2sBuf( str, (char *)&val,		// recursive call to do feet only
				   DTDBL,
				   UNNONE,			// no units
				   mfw-1,			// save a column for '
				   fmt );
		*(str+Cvnchars++) = '\'';		// add foot mark ' to end
		*(str+Cvnchars) = '\0';
	}
}		// cvFtIn2s
//======================================================================
LOCAL BOO FC NEAR nexK()		// K format incrementer: call each time need to make string shorter.  FALSE if cannot.

// "K format" means 1000 shortened to 1k, 1234000 shortened to 1.234M or 1.23M or 1.2M, etc.  see ddalpha[] for chars.

// alters val (value), ki (assumed init 0), aval, wid, amfw, nDigB4Pt.  Note mfw is not changed.
// after completion of formatting, caller must append ddalpha[ki] to string if ki > 0.

// returns FALSE with variables unaltered if clearly cannot shrink more.
// if TRUE is returned, converted string may not be shorter due to round-up.
{
	if (!ki)				// if not in K format yet
	{
		if (amfw <= 2)			// no way without a digit and k after (note: need 2 digits + k to handle all values)
			return FALSE;			// not enough room, period.
		if (fabs(val) < 500.)		// 500-->1k: smaller (too misleading?);  499.9999-->.5k: no smaller than 499.
			return FALSE;			// too small to gain from shrinking
		amfw--;				// first time, reduce field width (after sign) by 1 for the k, M, etc
		if (wid > 1)  wid--;		// unless FMTSQ, reduce sprintf width for the ditto.
	}
	else				// already in K format
	{
		if (fabs(val) < 99.5)		// 99.5k -->.1M: smaller than 100k.  99.49999k-->.09M, larger than 99k
			return FALSE;			// too small to shrink more
		if (ddalpha[ki+1]=='?')		// if we have used up all the suffix characters (ie have divided by about 1e18)
			return FALSE;			// can't shrink any more, let it ****
	}
	ki++;				// first/next k format trailing character (init -1)
	val /= 1000.;			// divide value to print by 1000
	aval = fabs(val);			// maintain absolute val for some callers
	nDigB4Pt -= 3;			// 3 less digits before point
	setToMax( nDigB4Pt, 0);		// .. but not less than 0.
	return TRUE;
}			// nexK
//======================================================================
LOCAL BOO FC NEAR cvsd( 			// Significant-digits (trim trailing 0's, g format) output conversion

	SI _mfw,		// Maximum field width
	SI _dfw )		// Desired number of significant digits

// and uses local statics including:
//  char *str,	String into which to store converted value
//  double val	Value to convert
//  fmt		cvin2s caller's format argument
//  wid		field width given in cvin2s call, or 1 for FMTSQ
//  wsign		sign width, 0 or 1
//  ik		init -1 for nexK
//  ijust
//  [ipv]

/* returns FALSE if number too small for method used here:
                 use cvdd to get eg 0.00 for .0003, .001 for 0.0006 in 4 cols, not .000n (too wide) nor e- format (too wide).
   returns TRUE  if converted.  If overwide, mininum width form returned.
                 Also sets:  Cvnchars:    Number of characters output: caller checks for excess width */
{
	//ki = 0;					// K format index, init by caller
	aval = fabs(val);				// absolute val
	amfw = _mfw - wsign; 			// width less sign (public static).  wsign is !(val >= 0. [&& pv==FMTPVNULL])
	setToMin( _dfw, amfw);				// fix turkey calls; reduce for sign.  Note may be increased to nDigB4Pt below.

// return FALSE if value too small for width: don't duplicate 0 exception and cvdd's small-value formatting.
//	eg for 3 cols, need at least .095 to print here as .01; let cvdd do less (cvdd prints .005 as .01, .00499 as 0.0).
#define MINHERE 1e-4	// min abs value ever done here, per preferences & sprintf %g limits, was hard-coded .001.
	// was hard-coded .001 4-92; also tried 1e-5, resulted in %g using e- format.
	if (aval < 1)					// 1 or greater always done here
		if ( aval < MINHERE				// less than this never handled here
		||  aval < 9.5*NPten[min(amfw,NPTENSIZE)] )	// if too small for g format in width even when only 1 sig dig
			return FALSE;					// examples: amfw  min print  min val  as power of 10
	//            1       1         .95         9.5e-1
	//            2       .1        .095        9.5e-2  etc

// determine digits left of decimal point.  Note nexK adjusts.
	for ( nDigB4Pt = 0;  					// count digits before point (b4 rounding)
	nDigB4Pt < PTENSIZE  &&  Pten[nDigB4Pt] <= aval;	//   Pten: powers of 10, [0]..[15]. cons.c.
	nDigB4Pt++ ) ;

#if 0	// idea not used -- leave user the control of seeing eg 99.9k not 99990 by spec'ing 4 digits in 6 columns. 4-9-92.
x// conditionally use more significant digits rather than an overflow format
x    if ( nDigB4Pt > _dfw  				// if value needs more digit positions than requested
x     &&  nDigB4Pt <= amfw				// if field width allows these digit positions
x     &&  nDigB4Pt <= 6 )				// up to a million (rob's judgement)
x       _dfw = nDigB4Pt;   				// use the digits, not e or k format
#endif

// initial handling of k format overflow format: make value fit if can, if does not round up.
	if ((fmt & FMTOVFMASK)==FMTOVFK)			// if overlow is to be handled with "k format"
	{
		if (_dfw < 3)					// needs 3 digits or 2 + k to work in general: 499, 1k, 99k, .1M, etc.
			_dfw = min( SI(3), amfw);			// so take them if avail; loop below reduces dfw if it helps.
		while (nDigB4Pt > _dfw && nexK()) ;		// while too many digits b4 ., while possible, divide by 1000 & bump ki.
	}

// for numbers still too big for small field, use enough digits to get the least excess width, to fit best in mfw+xfw.
	SI idigB4Pt = nDigB4Pt + (Pten[nDigB4Pt] <= aval + 0.5);	// digits in integer format: may round up
	if (idigB4Pt <= 5)						// if integer form not wider than e format form "1e+01"
		setToMax( _dfw, idigB4Pt);					// do 1st convert with at least enuf digits to not get e format

// format number as text, retry if comes out wider than field width

	for ( ; ; )						// loop to reduce significant digits and/or do k thing to fit into field
	{
		setToMin( _dfw, max(amfw,idigB4Pt));   		// saves an iteration if nexK reduced amfW and number clearly fits

		/* pre-round when field width is exactly 1 more than digits b4 point, to fit values that sprintf itself cannot.
		   example: 999.9, mfw = 4: there is no dfw that will make sprintf do it for us (dfw=4 -->999.9: 5 cols; dfw=3 overflows)
		                            but sprintf of 1000 with dfw==4 works. */
		double _val = nDigB4Pt+1==amfw   			// if have 1 column + digits b4 pt (less or more we can't fix)
		&&  nDigB4Pt+1==_dfw			// and are printing that # sig digits (less-->e format)
		&&  Pten[nDigB4Pt] <= aval + 0.5		// and rounding adds digit b4 point (--dfw loop can't fix)
		? val + 0.5 : val;				// then pre-round value to sprintf, else don't meddle.

		// save an overflow iteration if string is going to contain a decimal point
		SI nDigAfPt = _dfw - nDigB4Pt;
		if ( _dfw==amfw						// if enuf digits to fill field specified
		&&  nDigAfPt > 0					// and some digits after point
		&&  ( nDigAfPt != 1 					// and value like 999.9
		|| Pten[nDigB4Pt] > aval + 0.5 ) )		//  is not going to round to 1000, dropping point
			_dfw--;						// then drop a digit now, save time of a sprintf

#ifdef FMTPVMASK
p       Cvnchars = sprintf( str, gf[ijust][ipv], wid, _dfw, _val);	// convert number to string (c library)
#else
		Cvnchars = sprintf( str, gf[ijust], wid, _dfw, _val);	// convert number to string (c library)
#endif

		// done if fits field and not 'e' format when k format overflow specified

		if (Cvnchars <= amfw+wsign)  					// note mfw not reduced for k char if any
			if ((fmt & FMTOVFMASK) != FMTOVFK  ||  !strchr( str, 'e'))	// if not 'k' fmt overflow or sprinf used no 'e'
				break;							// normal termination of loop

		// text wider than field (expected for roundup cases) or contains 'e' when k format desired

		if (!memcmp(str+wsign,"0.",2))				// if begins with "0." (implies following digits)
		{
			strcpy( str+wsign, str+wsign+1);			// take out the leading 0 before the .
			if (--Cvnchars <= amfw+wsign)				// is now 1 char narrower
				break;						// now it fits!
		}
		if ((fmt & FMTOVFMASK)==FMTOVFK)				// if 'k' overflow format requested
		{
			if (nDigAfPt > 0  &&  _dfw > 3)   			// first trim digits after point: 123.4k-->.1234M is no gain
			{
				_dfw--;
				continue;   				// (after can't nexK, final digits after . trimmed below)
			}
			if (nexK())  continue;				// do a(nother) k format /1000  /  if could, format again
		}

		// for overwide field, produce mininimum possible width for max chance of fitting mfw+xfw columns

		/* don't reduce dfw if further reduction would go to e format if this would widen field.
		   Shortest string is "1e+01", or dfw==nDigB4Pt for val < 9999.5
		   Stop at dfw==nDigB4Pt if nDigB4Pt < 5, or at dfw==nDigB4Pt+1 if value rounds up. */

		if (_dfw <= 1)  break;					// never go to 0 digits

		if ( nDigAfPt > 0					// if there are digits AFTER point
		||  amfw >= 5 						// or space big enuf to hold e format: "1e+01"
		||  _dfw > 5    					// or value still wider than 1-digit e format
		||  _dfw > nDigB4Pt 					// or number is not yet as narrow as it can get (integer format)
		+ (Pten[nDigB4Pt] <= aval + 0.5) )		//   this term is 1 if number will round up to added digit
			_dfw--;						// drop one sig digit and retry
		else
			break;						// stop, can't make it any narrower
	}

// append 'k', 'M', 'G', etc if k overflow format used

	if (ki)
	{
		*(str+Cvnchars) = ddalpha[ki]; 		// append k, M, G, ...
		Cvnchars++;
		*(str+Cvnchars) = '\0';
	}

	return TRUE;				// TRUE: we did format the number (see small aval FALSE return above)
}				// cvsd
//======================================================================

// 4-92: needs review re producing minimum-overwidth result for xfw.

LOCAL BOO FC NEAR cvdd( 			// Drifting decimal output conversion

	SI _mfw,		// Maximum field width
	SI _dfw )		/* Decimal field width: # digits after point,
    			   also, if non-0, # sig digits for numbers < .1 (digits after . conditionally increased) */
// and uses local statics including:
//  char *str,	String into which to store converted value
//  double val	Value to convert
//  wid		field width given in cvin2s call
//  wsign		sign width, 0 or 1
//  ijust
//  [ipv]

/* Returns 1 if conversion was successful, 0 otherwise.  Also sets:
        Cvnchars (public):    Number of characters output
        amfw (static SI):     _mfw adjusted for sign, used by caller re field overflow handling */
{
#ifdef FMTPVMASK	// define in cvpak.h to restore p positive value display options, 11-91
p
p    // These format definitions must match definitions of FMTPVxxx in cvpak.h
p			     // null        +	      ' ' if positive
p    static char *ff[4][3] = { "%-*.*f",  "%-+*.*f",  "%- *.*f",  // left just
p			      "%*.*f",   "%+*.*f",   "% *.*f",   // rt
p			      "%0*.*f",  "%0+*.*f",  "%0 *.*f",  // rt zeroes
p			      "%*.*f",   "%+*.*f",   "% *.*f"
p		};  // squeeze
#else
	static char *ff[4] =    { "%-*.*f",  		// left just
		"%*.*f",   		// rt
		"%0*.*f",  		// rt zeroes
		"%*.*f",
	};		// squeeze
#endif
	SI n0afPt, nDigB4Pt, prcsn, maxDfw;
	char *zp;
	aval = fabs(val);			// absolute val
	amfw = _mfw - wsign; 		// width less sign.  wsign is !(val >= 0. [&& pv==FMTPVNULL])

// compute precision (digits after point) to fit field, in range 0 to (possibly increased) _dfw

	nDigB4Pt = 0;
	while (nDigB4Pt < PTENSIZE  &&  Pten[nDigB4Pt] <= aval)	// Pten: powers of 10, [0]..[15]. cons.c.
		nDigB4Pt++;						// digits needed for magnitude left of .
	prcsn = amfw - nDigB4Pt - 1;   				// precision: space avail for digits after .
	if (prcsn <= 0)
		prcsn = 0;
	else 					// prcsn > 0: room for 1 or more digits after point
	{
		/* attempt to maintain dfw significant digits for numbers smaller than .1: conditionally add a few digits after point, 1-92.
			goals: show more of small numbers in typical report colums (dfw 1-3), but not as much as for large numbers.
			       numbers too small for significance to show: don't crowd page too much with 0.0000's.
			       don't add lots of digits to small export numbers (FMTRTZ gets here when < .001, dfw 6 already so) */

		if (_dfw > 0)   				/* don't go into decimal places if none requested (take dfw==0 as 0 sig dig here)
						   (??? or, do "if (!_dfw) _dfw++;" at foot of loop to get a sig dig to show) */
		{
			maxDfw = min( prcsn, _dfw + 3);     	// don't exceed space avail; don't add > 3 places here (shd that be 2?)
			maxDfw = min( maxDfw, 7);		// increase only to 7 dec places: avoid garbage detail; don't exceed NPten[].
			if (aval < NPten[maxDfw] * .5)	// if only 0's would show anyway even after round, don't crowd:
				maxDfw = min( maxDfw - 1,		/* add 1 less digit here (re 7 or +3 limits), and */
				prcsn + wsign - 2);	// leave space for sign even if > 0, here only.  CAUTION: may make maxDfw < 0.

			for (n0afPt = 1;  _dfw < maxDfw;  _dfw++, n0afPt++)	// increase _dfw til dfw sig digits show or to maxDfw limit
			{
				// test if enuf significant digits will (now) show
				if (aval >= NPten[n0afPt]) 		// if aval > .1 at entry, > .01 next iteration, then .001, .0001, etc.
					break;   				// stop if have added a digit for each leading 0 after point

				/*if (!_dfw) _dfw++   		 	if get here with 0 _dfw, do extra ++ for 1 sig dig.
				        				else dfw==0 means "0 sig dig" and only leading 0's after . wd show. */
			}
		}
		if (prcsn > _dfw)
			prcsn = _dfw;				// no more than _dfw digits after point
	}

// convert / retry if too wide

	while (1)
	{
#ifdef FMTPVMASK
p       Cvnchars = sprintf( str, ff[ijust][ipv], wid, prcsn, val);	// convert
#else
		Cvnchars = sprintf( str, ff[ijust], wid, prcsn, val);	// convert (C library)
#endif

		// test if fits (allowing positive # to overflow into - position)
		if (Cvnchars <= _mfw)
			return 1;						// return if fits _mfw
		// too wide.  If number < 1.0, sprintf output "0.xxx".  ndig did not allow for 0 b4 point.  Remove it.
		if (nDigB4Pt==0 && *(zp=(str+Cvnchars-prcsn-2))=='0')	// if of format 0.xxx
		{
			strcpy( zp, zp+1);					// remove leading 0 from 0.xxx.
			Cvnchars--;
			break;						// prcsn already as small as possible here, right?
		}
		// too wide... reduce digits after point (perhaps sprintf rounded up).
		if (--prcsn < 0)
			break;				// done if was already at 0 digits after .
	}
	return (Cvnchars <= _mfw);		// 1 if now fits
}				// cvdd
//======================================================================
LOCAL SI FC NEAR cvttz( char *_str, SI trailChar, SI minWid)

// trim trailing 0's after point, and point, but not shorter than minWid

// returns new length
// called from cvin2s to trim decimal inches 10-88; might be useful re other floats types if reimplementing.
{
	char *p, *pend, *q;
	SI len;

	len = (SI)strlen(_str);
	if (len <= minWid)
		return len;				// done if not shortenable
	p = _str + len;				// point end
	while (p > _str  &&  *(p-1)==' ')		// (actual input probably has no ..
		p--;					// trailSpaces, could shorten code)
	if (p > _str  &&  *(p-1)==(char)trailChar)	// if given " etc is at end
	{
		p--;
		minWid--;				// trim b4 it, then restore
	}
	else
		trailChar = 0;				// say no char to put back at end
	while (p > _str  &&  *(p-1)==' ')		// (actual input probably has no ..
		p--;					// trailSpaces, could shorten code)
	pend = p;					// 1st trailing space (else null): truncate to here even if no .000
	while (p > _str  &&  *(p-1)=='0')
		p--;					// back over trail zeroes
	// can only truncate 0's if after point
	if (p > _str)
	{
		if (*(p-1)=='.')				// if point precedes first trail0
			pend = p-1;				// chop zeroes and also the point
		else					// other digits between . and 0s ok
		{
			for (q = p-1;  q > _str && isdigitW(*q);  q--)
				;					// back over digits b4 0's
			if (*q == '.')			// if point b4 the digits
				pend = p;				// chop the 0's, not other digits
		}
	}
	if (pend < _str + minWid)			// don't make < minWid
		pend = _str + minWid;			// nb minWid checked for < len above
	if (trailChar)
		*pend++ = (char)trailChar;       	// move " or other ending thing
	*pend = (char)0;				// truncate
	return strlen(_str);			// new length. another return above.
}			// cvttz
//======================================================================
RC FC cvLmCk( 		// Check input data for being within limits for data and limit type

	SI dt,    	// Data type (dtypes.h define, from dtypes.def by rcdef.exe)
	SI limit,	// Limit type (dtlims.h define, from limits.def by rcdef.exe)
	void *p )	// pointer to data to check (points to ptr for string types)

// returns RCOK if ok, or message code specifying the error.
{
	RC rc = RCOK;

	if (limit != LMNONE)
	{
		switch (dt)
		{
		case DTSI:
			switch (limit)
			{
			case LMGZ:
				if (*(SI *)p <= 0)  rc = MH_V0024;
				break;
			case LMGEZ:
				if (*(SI *)p <  0)  rc = MH_V0023;
				break;
			case LMG1:
				if (*(SI *)p <= 1)  rc = MH_V0028;
				break;
			case LMGE1:
				if (*(SI *)p <  1)  rc = MH_V0027;
				break;
			case LMLEZ:
				if (*(SI *)p >  0)  rc = MH_V0022;
				break;
			case LMLZ:
				if (*(SI *)p >= 0)  rc = MH_V0026;
				break;
			case LMNZ:
				if (*(SI *)p == 0)  rc = MH_V0025;
				break;
			case LMDOY:
				if (*(SI *)p < 1  ||  *(SI *)p > 365)  rc = MH_V0034;
				break;
			default:
				break;
			}
			break;
		case DTUSI:
			switch (limit)
			{
			case LMGZ:
				if (*(USI *)p == 0)  rc = MH_V0024;
				break;
			case LMDOY:
				if (*(USI *)p < 1  ||  *(USI *)p > 365)  rc = MH_V0034;
				break;
			case LMNZ:
				if (*(USI *)p == 0)  rc = MH_V0025;
				break;
			default:
				break;
			}
			break;
		case DTLI:
			switch (limit)
			{
			case LMGZ:
				if (*(LI *)p <= 0)  rc = MH_V0024;
				break;
			case LMGEZ:
				if (*(LI *)p <  0)  rc = MH_V0023;
				break;
			case LMG1:
				if (*(LI *)p <= 1)  rc = MH_V0028;
				break;
			case LMGE1:
				if (*(LI *)p <  1)  rc = MH_V0027;
				break;
			case LMLEZ:
				if (*(LI *)p >  0)  rc = MH_V0022;
				break;
			case LMLZ:
				if (*(LI *)p >= 0)  rc = MH_V0026;
				break;
			case LMNZ:
				if (*(LI *)p == 0)  rc = MH_V0025;
				break;
			default:
				break;
			}
			break;
		case DTFLOAT:
#ifdef DTPERCENT
		case DTPERCENT:
#endif
			switch (limit)
			{
			case LMGZ:
				if (*(float *)p <= 0.f)                      rc = MH_V0024;
				break;
			case LMGEZ:
				if (*(float *)p < 0.f)                       rc = MH_V0023;
				break;
			case LMG1:
				if (*(float *)p <= 1.f)                      rc = MH_V0028;
				break;
			case LMGE1:
				if (*(float *)p < 1.f)                       rc = MH_V0027;
				break;
			case LMLEZ:
				if (*(float *)p >  0.f)                      rc = MH_V0022;
				break;
			case LMLZ:
				if (*(float *)p >= 0.f)                      rc = MH_V0026;
				break;
			case LMNZ:
				if (*(float *)p == 0.f)                      rc = MH_V0025;
				break;
			case LMFGZ:
				if (*(float *)p <= 0.f || *(float *)p > 1.f) rc = MH_V0032;
				break;
#ifdef DTPERCENT
			case LMFR:
				if (*(float *)p < 0.f || *(float *)p > 1.f)  rc = (dt==DTPERCENT) ? MH_V0033 : MH_V0031;
				break;
#else
			case LMFR:
				if (*(float *)p < 0.f || *(float *)p > 1.f)  rc = MH_V0031;
				break;
#endif
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	return rc;
}		// cvLmCk
//======================================================================
double FC cvExtoIn( 		// Convert value from external units to internal

	double f,		// value to be converted
	int units )		// unit type: UNxxx defines from units.h generated by rcdef.exe from data\units.def

// call only for DTFLOAT, DTDBL, or DTPERCENT value
// Returns converted value
{
	if (units != UNNONE)
	{
		f /= UNIT::GetFact( units);				// divide by units' scale factor (below)
		if (units == UNTEMP && Unsysext == UNSYSSI)	// if temperature
			f += 32.0;					// complete C to F adjusment -- ONLY units where 0 point changes
	}
	return f;
}		// cvExtoIn
//======================================================================
double FC cvIntoEx( 		// Convert value from internal units to external

	double f,		// value to be converted
	int units )		// unit type

// call only for DTFLOAT, DTDBL, DTPERCENT, or DTSSE value
// returns converted value
{
	if (units != UNNONE)		// if it has units
	{
		if (units == UNANGLE)			// if angle
			f = cvstdangle(f);			// normalize to 0..2*Pi
		f *= UNIT::GetFact( units);		// get factor (below), conv to extnl
		if (units == UNTEMP			// if temperature
		&& Unsysext == UNSYSSI) 		// and metric
			f -= 17.77777778;			// adjust zero point
	}
	return f;
}		// cvIntoEx

//======================================================================
double FC cvstdangle(			// Normalize an angle into 0 to 2*PI range

	double ang )  	// Input angle to be normalized (radians)

/* DO NOT use in high precision situations without consideration, cuz if ang > 2PI-.001, it is changed to 0.
   Other such "snaps" may be added in the future. */

// Returns equivalent angle that falls in range 0 - 2*PI
{
	if (ang >= k2Pi)			// if 2*Pi or greater
		return fmod( ang, k2Pi );	// take mod 2 * Pi

	/* If negative, want to be independent of what fmod does if < 0.
	   Values 0. <= ang < k2Pi fall thru and are returned */
	while (ang < 0.0)			// until positive
		ang += k2Pi;			// add 2 * Pi
	if (ang > (k2Pi - .001))
		ang = 0.;		// If angle is close to 2PI, change it to 0 so it won't display as 360.
	return ang;
}		// cvstdangle

// made local and renamed from cvftinch, 2-91:
//============================================================================
LOCAL LI FC NEAR sepFtInch(		// Break up length into feet and inches

	double d,	// Length to be broken up (float or double feet)
	SI *inp )	// Location for returning inches. fcn value is LI feet.

/* Returns integral feet as fcn value, plus integral inches via *inp.
   If d < 0, feet are returned negative; inches are returned positive unless feet are 0.
   Since d is converted to LI inches, the largest distance which can be converted is around 178,956,970 ft
     (about 33,893 miles; I know you're disappointed). */
{
	LI totin, ft;
	SI nd;

	nd =  d < 0.0 ? -12 : 12;
	totin = (LI)( d*(double)nd + 0.5 );	// total inches, pos, rounded
	ft = totin/nd;					// feet, signed
	*inp = (SI)((ft==0 && d < 0.) ? -totin : totin%12);	// inches
	return ft;
}			// sepFtInch

//======================================================================
int getChoiTxTyX( const char* chtx)		// categorize choice text
{
	int tyX = *chtx == '*' ? chtyHIDDEN		// hidden
		    : *chtx == '!' ? chtyALIAS		// alias
		    : *chtx == '~' ? chtyALIASDEP	// deprecated alias
			:                chtyNORMAL;
	return tyX;
}
//======================================================================
const char* getChoiTxI( 		// text for choice of choice data type
	USI dt, 				// data type (for bits and choicb Dttab index)
	SI chan,				// choicb index (use CHN(float), cnglob.h, to extract hi word of choicn)
							//   (1 based)
	int* pTyX/*=NULL*/,		// NULL or receives choice type
							//    chtyNORMAL, chtyHIDDEN
	int options /*=0*/)		// option bits
							//   1: nz = get entire string (including any aliases)
							//      0 = get primary (1st) text only

// note: return value may be TmpStr

{
	if (dt & (DTBCHOICB|DTBCHOICN))		// verify that is a choice type -- debug aid
	{
		// check for out of range or garbage value -- debug aid
		if (dt & DTBCHOICN)			// for choicn, chan may be hi word of float, including bits to make a NAN
			if ((chan & 0xff80)==NCNAN)		// if proper NAN bits (NCNAN: 7f80, cnglob.h), not just garbage
				chan &= ~NCNAN;				// remove them for check. But leave improper bits to evoke error msg.
		if (chan <= 0 || chan > GetDttab(dt).nchoices)	// check that choice is in range 1 to # choices for dt. GetDttab: srd.h.
		{
			err( PWRN, (char *)MH_V0036, (INT)chan, (UI)dt );	// display program error err msg, wait key, rmkerr.c
			// "cvpak:getChoiTx(): choice %d out of range for dt 0x%x"
			return "bad choice";
		}

		// access text
		const char* chtx = GetChoiceText( dt, chan);	// access text for fcn. srd.h fcn.

		// return info
		if (!(options&1))
		{	// retrieve 1st piece to TmpStr
			const char* p = chtx;
			chtx = strxtok( NULL, &p, "|", TRUE);
		}
		int tyX = getChoiTxTyX( chtx);
		if (tyX > chtyNORMAL)
			chtx++;		// drop prefix
		if (pTyX)
			*pTyX = tyX;
		return chtx;
	}
	err( PWRN, (char *)MH_V0037, (UI)dt );  		// program (internal) err msg w keypress
	// "cvpak:getChoiTx(): given data type 0x%x not a choice type"
	return "bad dt";
}			// getChoiTxI
//======================================================================
RC FC cvS2Choi( 		// convert string to choice value for given data type else format (do not display) error msg
	const char *s, 		// string to convert
	USI dt,				// choicb or choicn data type (dtypes.h) to convert: specifies choice strings in Dttab[].
	void* pv, 			// NULL or receives choice value: 2 bytes for choicb, 4 bytes (hi 2 significant) for choicn.
	USI* pSz, 			// NULL or receives size: 2 or 4
	const char** pms )	// if non-NULL
						//   string not found: receives ptr to Tmpstr message insert: "%s not one of xxx yyy zzz ..."
						//   string=alias: receives ptr to deprecation Tmpstr message
						//   else receives NULL

// returns RCOK if ok
//    RCBAD with *pms set on error
//    RCBAD2 with *pms set on info/warning (use of alias,)
{
	if (pms)
		*pms = NULL;		// init to no message
	if (dt & (DTBCHOICB|DTBCHOICN))		// if a choice type
	{
		// search this choice data type's strings for a match, using getChoiTxI (just above).
		int v;
		for (v = 1;  v <= GetDttab(dt).nchoices;  v++)			// loop data type's choices (GetDttab: srd.h)
		{	int tyX;
			const char* chtx = getChoiTxI( dt, v, &tyX, 1);
			if (tyX == chtyHIDDEN)
				continue;		// hidden, cannot match

			const char* p = chtx;
			while (1)
			{	chtx = strxtok( NULL, &p, "|", TRUE);
				if (!chtx)
					break;
				tyX = getChoiTxTyX( chtx);
				if (tyX > chtyNORMAL)
					chtx++;		// drop prefix if any
				if (strcmpi( s, chtx))
					continue;
				if (dt & DTBCHOICN)				// for choice in number-choice store bit pattern of
				{	if (pv)
						*(void **)pv = NCHOICE( v | NCNAN);
												// .. NCNAN (7f80, cnglob.h) + choice 1,2,3.. in hi word of float.
					if (pSz)
						*pSz = sizeof(float);	// tell caller value size is 4 bytes
				}
				else						// plain choice (cannot also hold number)
				{	if (pv)
						*(SI *)pv = v;		// store choicb value: integer 1,2,3...
					if (pSz)
						*pSz = sizeof( SI);	// size is 2 bytes
				}
				if (tyX == chtyALIASDEP)		// if deprecated
				{	if (pms)
					{	const char* ms = strtprintf( "Deprecated '%s' converted to '%s'",
							chtx, getChoiTxI( dt, v));
						*pms = ms;
					}
					return RCBAD2;				// warning return
				}
				return RCOK;					// good return
			}
		}
		// not found.  generate error message insert showing given string and choices
		//   list of choices does not include aliases 8-12
		if (pms)						// if message return pointer given
		{
			USI maxll = getCpl() - 5;
			// -5: leave some space for adding punctuation, indent, final " ...", etc.
			const char* ms = strtprintf( (char *)MH_V0039, s);		// start assembling string "'%s' is not one of choice1 choice2 ..."
			for (v = 1;  v <= GetDttab(dt).nchoices;  v++)	// loop data type's choices, concatenate each to ms
			{	int tyX;
				const char* chtx = getChoiTxI( dt, v, &tyX );	// get vth choice
				if (tyX != chtyHIDDEN)	// if not hidden
				{	if (strlen(ms) + strlen(chtx) > 200)		// if there's an unexpectedly large # long choices
					{
						ms = strtcat( ms, " ...", NULL);		// limit amount shown
						break;
					}
					const char* sep
						   = strJoinLen( ms, chtx) 			// length of line if joined, strpak.c
							 >  maxll  ?  "\n    "  :  " ";		// separator: newline if needed
					ms = strtcat( ms, sep, chtx, NULL );		// concatenate to Tmpstr, strpak.c
				}
			}
			*pms = ms; 						// return pointer to message insert text to caller
		}
		return RCBAD;						// bad value for data type return
	}
	if (pms)
		*pms = strtprintf( (char *)MH_V0038, (UI)dt );		// "cvpak:cvS2Choi(): given data type 0x%x not a choice type"
	return RCBAD;						// bad data type. 2+ other returns above
}			// cvS2Choi

/*=============================== TEST CODE ================================*/

/*==========================================================================*/
/* UNMAINTAINED Test code for output format */
/* #define TEST */
#ifdef TEST	/* #endif is about 250 lines down */
t
t #include <gvpak.h>      /* test input routines, in lib\ but not in library.*/
t
t static float testf = 123.345;
t
t main ()
t {
t char buf[200],buf2[200],dbuf[10];
t #define NV 11
t static SI vallist[NV] = {-10000,-1000,-100,-10,-1,0,1,10,100,1000,10000};
t #define NFV 12
t static float fvlist[NFV] =
t {  -13945789,-999.99,-100.,-99.99,-1.,0.,.006,.999,1.0,9.999,10.,
t 	 978437852312. };
t static SI fmts[] = { FMTSQ, FMTLJ, FMTRJ, FMTLZ };
t static char *fmtnames[] = {"sq","lj","rj","lz"};
t SI iv, m;
t float val;
t double d;
t USI i;
t SI dt, mfw, dfw, f, ifx;
t LI ft;
t SI in;
t char *p;
t IDATE tdate;
t ITIME ttime;
t IDATETIME tdatetime;
t RC rc;
t SEC sec;
t SI un, lm;
t CHOICE *ch;
t	 val = 2.5;
t    f = FMTRJ+2;
t    while (1)
t    {	cvin2sBuf(buf,(char *)&val,DTFLOAT,UNLENGTH,10,f);
t	 }
t
t /* #define PERCENTTEST */
t #ifdef PERCENTTEST
t    dt = DTPERCENT;
t    val = .30;
t    cvin2sBuf(buf,(char *)&val,dt,UNNONE,7,FMTSQ+2);
t    printf("\n'%s'",buf);
t    cvin2sBuf(buf,(char *)&val,dt,UNNONE,10,FMTLJ+1);
t    printf("\n'%s'",buf);
t    cvin2sBuf(buf,(char *)&val,dt,UNNONE,10,FMTRJ+1);
t    printf("\n'%s'",buf);
t    cvin2sBuf(buf,(char *)&val,dt,UNNONE,1,FMTRJ+1);
t    printf("\n'%s'",buf);
t #endif    /* PERCENTTEST */
t
t /* #define CHOICETEST */
t #ifdef CHOICETEST
t    dt = DTYESNO;
t    un = UNNONE;
t    lm = LMNONE;
t    dfw = 0;
t    Cp4snake = dmalloc(50,ABT);
t    Dttab = (SI *) dmalloc(100,ABT);
t    ch = DTCHOICEP(DTYESNO);
t    ch->size = 2;
t    ch->nchoices = 2;
t    ch->choices[0] = C_YESNO_YES;
t    ch->choices[1] = C_YESNO_NO;
t    strcpy( rcHan2Tx( C_YESNO_YES), "Yes");
t    strcpy( rcHan2Tx( C_YESNO_NO), "No");
t #endif	/* CHOICETEST */
t
t /* #define INTEST */
t #ifdef INTEST
t     dt = DTPERCENT;
t     dfw = 2;
t     while (strlen(getstr(">>> ",buf2)) > 0)
t     {  *(long *)dbuf = 0L;
t        rc = cvs2in( buf2, dt, un, lm, dbuf, WRN);
t        if (dbuf != Pgb)
t 			???
t        cvin2sBuf(buf,Pgb,dt,un,7,FMTSQ+dfw);
t        if (rc == RCOK)
t 			printf("Cvnchars=%d  Gbsize=%d  %s\n",(INT)Cvnchars,(INT)Gbsize,buf);
t }
t #endif	/* INTEST */
t
t
t /* #define TESTINT */
t #ifdef TESTINT
t        f = FMTLZ;
t        dt = DTUSI;
t        mfw = 6;
t        for (iv = 0; iv < NV; iv++)
t        {	i = vallist[iv];
t 			cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,f);
t 			printf("Plus ...  [%s]  %d\n",buf,(INT)Cvnchars);
t			/*
t 	  		cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,FMTPVPLUS+f);
t 	  		printf("Plus ...  [%s]  %d  ",buf,(INT)Cvnchars);
t 	  		cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,FMTPVNULL+f);
t 	  		printf("Null ...  [%s]  %d  ",buf,(INT)Cvnchars);
t 	  		cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,FMTPVSPACE+f);
t 	  		printf("Space ... [%s]  %d\n",buf,(INT)Cvnchars);
t			*/
t		}
t		/*
t		i = 100;
t       cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,FMTPVNULL+FMTLJ);
t       printf("Left  just [%s]  %d\n",buf,(INT)Cvnchars);
t       cvin2sBuf(buf,(char *)&i,dt,UNNONE,mfw,FMTPVNULL+FMTRJ);
t       printf("Right just [%s]  %d\n",buf,(INT)Cvnchars);
t		*/
t #endif	/* TESTINT */
t
t #ifdef TESTVAR
t        f = FMTRJ;
t        mfw = VARWIDTH*Vardisp;
t        cvin2sBuf(buf,(char *)vallist,DTVAR,UNNONE,mfw,FMTPVNULL+f);
t        printf("[%s] %d\n",buf,(INT)Cvnchars);
t #endif	/* TESTVAR */
t
t /* #define TESTSTR */
t #ifdef TESTSTR
t        dt = DTSTRING;
t        cvin2sBuf(buf,"Hello test",dt,UNNONE,7,0);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,"Hello test",dt,UNNONE,11,0);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,"Hello test",dt,UNNONE,15,0);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,"Hello test",dt,UNNONE,15,FMTRJ);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,"Hello test",dt,UNNONE,256,FMTSQ);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t #endif	/* TESTSTR */
t
t #undef TESTDATE
t #ifdef TESTDATE
t /*
t        dt = DTDOY;
t        mfw = 10;
t        i = 1;
t        p = (char *)&i;
t */
t /*
t        mfw=30;
t        dt = DTIDATETIME;
t        tdatetime.year = 86;
t        tdatetime.month = 11;
t        tdatetime.mday = 24;
t        tdatetime.wday = 4;
t        tdatetime.hour = 12;
t        tdatetime.min  = 11;
t        tdatetime.sec = 34;
t        p = (char *)&tdatetime;
t */
t /*
t        mfw=15;
t        dt = DTITIME;
t        ttime.hour = 12;
t        ttime.min = 4;
t        ttime.sec = -1;
t        p = (char *)&ttime;
t */
t        mfw=10;
t        dt = DTMONTH;
t        i = 4;
t        p = (char *)&i;
t
t        cvin2sBuf(buf,p,dt,UNNONE,mfw,FMTLJ);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,p,dt,UNNONE,mfw,FMTRJ);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t        cvin2sBuf(buf,p,dt,UNNONE,mfw,FMTSQ);
t        printf("[%s]   %d\n",buf,(INT)Cvnchars);
t #endif	/* TESTDATE */
t
t /* #define TESTFL  */
t #ifdef TESTFL
t        dt = DTFLOAT;
t        for (iv = 0; iv < NFV; iv++)
t        {	val = fvlist[iv];
t 			mfw = 10;
t 	  		dfw = 2;
t 	  		f = FMTLJ;
t 	  		printf("%12.3f  ",val);
t 	  		cvin2sBuf(buf,(char *)&val,dt,UNNONE,mfw,FMTPVPLUS+dfw+f);
t 	  		printf("Plus [%s]  %d\t",buf,(INT)Cvnchars);
t 	  		cvin2sBuf(buf,(char *)&val,dt,UNNONE,mfw,FMTPVNULL+dfw+f);
t 	  		printf("Null [%s]  %d\t",buf,(INT)Cvnchars);
t 	  		cvin2sBuf(buf,(char *)&val,dt,UNNONE,mfw,FMTPVSPACE+dfw+f);
t 	  		printf("Space [%s]  %d\n",buf,(INT)Cvnchars);
t		}
t #endif	/* TESTFL */
t
t #ifdef TESTFTIN
t       val = 100.9;
t       mfw = 6;
t       dt = DTFLOAT;
t       *buf = '\0';
t       for (iv = 0; iv < 2; iv++)
t       {	for (ifx = 0; ifx < 4; ifx++)
t 			{	f = fmts[ifx];
t 	    		printf("\n%s ... ",fmtnames[ifx]);
t 	    		cvin2sBuf(buf,(char *)&val,dt,UNLENGTH,mfw,FMTPVPLUS+f);
t 	    		printf("Plus %2d [%s]\t",Cvnchars,buf);
t 	    		cvin2sBuf(buf,(char *)&val,dt,UNLENGTH,mfw,FMTPVNULL+f);
t 	    		printf("Null %2d [%s]\t",Cvnchars,buf);
t 	    		cvin2sBuf(buf,(char *)&val,dt,UNLENGTH,mfw,FMTPVSPACE+f);
t 	    		printf("Space %2d [%s]",Cvnchars,buf);
t			}
t 			val = -val;
t		}
t #endif	/* TESTFTIN */
t
t /* #define TESTCF */
t #ifdef TESTCF
t #define GETFDDT(r,f) DTFLOAT
t /* #define GETFDP(r,f) (&testf) */
t #define GETFDUN(r,f) UNENERGY
t #define GETUNTAG(u) ("kBtuh")
t #define RCRT(r) (0)
t      m = 30;
t      for (mfw = 5; mfw < 9; mfw++)
t      {	cvcf2init( m, mfw, m+20, mfw+1);
t 			for (dfw = 0; dfw < 3; dfw++)
t 			{	p = cvcf2( buf, "Dog", &testf, 1, FMTRJ+FMTUNITS+dfw,
t 				&testf, 1, FMTRJ+FMTUNITS+FMTNODATA+dfw );
t 				printf("[%s] Len=%d\n",p,(INT)strlen(p));
t			}
t		}
t #endif	/* TESTCF */
t }
#endif	/* TEST */

///////////////////////////////////////////////////////////////////////////////
// basic string-binary conversion functions including
///////////////////////////////////////////////////////////////////////////////

// Improvements tried and discarded (9-11-89): experimented with MSC library
//  fcn strtod() ... seemed promising for possible code savings.  Proved to
//  be about 3 times slower than cvatof() and introduces MSC dependency.
/*=============================== TEST CODE ================================*/
#ifdef TEST
t
t main()
t{
t RC rc;
t LI li;
t SI i, test, loop, loopLimit, printFlg;
t double d;
t char *s;
t static struct	/* test driver table */
t    {  char *s;	/* string to convert, NULL to term table */
t    	SI test;	/* 0: cvatol, hexoct = 0
t 			   1: cvatol, hexoct = 1
t 			   2: cvatof, percent = 0
t 			   3: cvatof, percent = 1 */
t } tab[] =
t    {	"   3",		2,
t       "  3.33e+4",	2,
t 		"  3.e+4",      2,
t 		"  3e-10",	2,
t 		"5.34952945293",2,
t 		" 3.33%e17",    3,
t 		" 3.33   %",	3,
t 		".",		2,
t 		"0000.0",	2,
t 		".0",		2,
t 		NULL
t };
t
t     loopLimit = 1;
t     for (loop = 0; ++loop <= loopLimit; )
t     {
t        printFlg = loop == loopLimit;
t        i = -1;
t        while ( (s = tab[++i].s) != NULL)
t        {  test = tab[i].test;
t           if (printFlg)
t 				printf("s=|%s|  ",s);
t           if (test <= 1)
t           {
t              li = 0L;
t              rc = cvatol( s, &li, test );
t 			   if (printFlg)
t 	           printf("li = %ld", li );
t			}
t           else
t           {
t              d = 0.;
t              rc = cvatof( s, &d, test-2 );
t              if (printFlg)
t 					printf("d = %f", d );
t			}
t           if (printFlg)
t 				printf("   rc = %d\n", (INT)rc );
t		}
t	}
t   return 0;	/* keep compiler happy */
t}						/* main */
#endif	/* TEST */

//===========================================================================
RC FC cvatol(		/* convert string to (signed) long integer */

	char *s,	/* string */
	LI *pn,	/* receives value */
	SI hexoct)	/* 0 accept decimal only, nz also accept
		   0x<digits> for hex and 0o<digits> for octal */

/* valid format: [ws][+|-][ws][*]<digit>[<digits>][ws]<null>
                 where * is 0x or 0o if hexoct > 0

   note whitespace allowed: leading, btw sign and number, btw number and
	                       terminating null
   note also: sequences like "-0x5f" are allow */

/* returns RCOK if ok, else other value */
{
	LI n, newn;
	SI sign, digSeen, digVal, base;
	char c;

	n = 0L;
	digSeen = 0;
	base = 10;
	sign = 1;

	while (isspaceW( *s))  /* pass leading whiteSpace */
		s++;
	if (*s=='-' || *s=='+')	/* test for - or + sign */
	{
		if (*s=='-')
			sign = -1;		/* accept - sign */
		s++;			/* point to char after sign */
		while (isspaceW( *s))	/* allow whiteSpace between sign and digits?*/
			s++;
	}

	while (1)     /* loop to process digits */
	{
		c = tolower(*s);            /* fetch next character */
		if (!(isxdigit(c)))         /* nondigit ends number */
			break;
		s++;                                        /* NOW advance pointer */
		digVal =  (c<='9')  ?  c-'0'  :  c-'a'+10;  /* convert digit to binary */
		if (!digSeen && digVal==0)                  /* if 0 first digit */
		{
			c = tolower(*s);
			if (hexoct && c=='x')     /* 0x sets hex base */
			{
				base = 16;
				s++;        /* pass the x */
			}
			else if (hexoct && c=='o')        /* leading 0o sets octal */
			{
				base = 8;
				s++;         /* pass the o */
			}                 /* 0x or 0o not valid # without following digits, */
			else              /* but just a zero */
				digSeen++;     /* is a valid number, say digit seen. */
		}
		else                         /* not '0' or not 1st digit */
		{
			if (digVal >= base)	// check e.g. for 8 or 9 in octal #
				return MH_V0010;	// "Invalid number"; digit too big for base
			newn = n * (LI)base + (LI)digVal; /* accumulate value */
			if (newn < n)                     /* klutzy overflow check */
				return MH_V0012;	// "Value must be between -2147483639 and
			//    2147483639"
			n = newn;		// overflow not detected
			digSeen++;		// at least one digit seen: have valid number
		}
	}             /* while (1) */
	if (!digSeen)          	// need at least 1 digit for valid number
		return MH_V0010;		// "Invalid number": no digits present
	while (isspaceW( *s))	// pass trailing whiteSpace
		s++;
	if (*s)			// check for terminating null
		return MH_V0010;		// "Invalid number": trailing garbage
	*pn = n * (LI)sign;    /* return value to caller */
	return RCOK;           /* good return */

}					/* cvatol */

//========================================================================
RC FC cvatof(		/* Convert a string to a double value */

	char *str,		/* String, leading and trailing ws OK */
	double *vp,		/* Pointer to double to receive value. */
	SI percent )	/* nz: trailing '%' is acceptable (and ignored)
    			   0:  '%' treated as error */

/* valid format:
     [ws][+|-][ws][digits][.[digits]][e|E|d|D[+|-]<digit>[digits]][ws][*]
     where * is %[ws] if percent > 0
           and at least one digit is present in mantissa

   Resulting value is double, but is always (float)able.  Range of
   legal values is approximately 10**-38 to 10**38.  However, the number
   digits in the input value is also limited by PTENSIZE (currently 15),
   so values over about 10**15 must be input with e format.

   Changes 9-11-89:
   	1.  accepts D/d in addition to E/e for exponents
	2.  new arg percent eliminates use of cvpak global. % now handled
	    correctly for e format ( .34e01 % ); exponents no longer go
	    through cvatol.
	3.  combined action and state tables into single table, made entries
	    symbolic w/ #defines for ez reading. */

/* Returns RCOK if conversion succeeded, otherwise informative RC.
   No value is stored unless conversion succeeds. */

{
	double v;
	SI negVal, negExp, nd, id, idd, expo, irdd;
	char c;
	char d[100], dd[100];	/* characters left / right of decimal point */
	register SI s, ct;

	/* String decode portion of function is implemented with finite state scheme.
	   The STATE records the current decoding situation; each input char is
	   retrieved and a state-dependent ACTION is taken.  The table actState
	   drives the process. */
	/* States */
#define lws  0	/* scanning over leading white space */
#define ld   1	/* scanning for leading digit (sign has been seen) */
#define gd   2	/* getting digits (left of dec point) */
#define gdd  3	/* getting digits (right of dec point) */
#define xsn  4	/* expecting exponent sign (or digit) */
#define xld  5	/* expecting exponent leading digit */
#define xd   6	/* expecting exponent digit (or term. non-digit) */
#define tc   7	/* scanning over trailing chars (% or ws) */
#define tws  8	/* scanning over trailing white space */
	/* Actions */
#define SK   0x00	/* discard character */
#define GSN  0x10	/* note sign */
#define GD   0x20	/* get digit (left of dec. point) */
#define GDD  0x30	/* get digit (right of dec. point) */
#define GXS  0x40	/* note exponent sign */
#define GX   0x50	/* get exponent digit */
#define DONE 0x60	/* \0 found, go determine value */
#define ERR  0xF0	/* syntax error, return "Invalid number" */
	static UCH actState[9][7] =
	{
		/*		        	------------------ Character ------------------------- */
		/*				ws	+ or -	0-9	.	Ee/Dd	%	'\0'   */
		/*				------  ------	------	------	------	------	------ */
		/* lws: leading ws   */  {	SK+lws,	GSN+ld,	GD+gd,	SK+gdd,	ERR,	ERR,	ERR  },
		/* ld:  leading dgt  */  {	SK+ld,	ERR,	GD+gd,	SK+gdd,	ERR,	ERR,	ERR  },
		/* gd:  get digits   */  {	SK+tc,	ERR,	GD+gd,	SK+gdd,	SK+xsn, SK+tws,	DONE },
		/* gdd: get dec dgts */  {	SK+tc,	ERR,	GDD+gdd,ERR,	SK+xsn, SK+tws,	DONE },
		/* xsn: get expo sign*/  {	ERR,	GXS+xld,GX+xd,	ERR,	ERR,	ERR,	ERR  },
		/* xld: expo ldng dgt*/  {	ERR,	ERR,	GX+xd,	ERR,	ERR,	ERR,	ERR  },
		/* xd:  expo digit   */  {	SK+tc,	ERR,	GX+xd,	ERR,	ERR,	SK+tws,	DONE },
		/* tc:  trlng chars  */  {	SK+tc,	ERR,	ERR,	ERR,	ERR,	SK+tws,	DONE },
		/* tws: trailing ws  */  {	SK+tws,	ERR,	ERR,	ERR,	ERR,	ERR,	DONE }
	};

	negVal = negExp = nd = id = idd = irdd = expo = 0;
	s = lws;		/* initial state: scanning over ws */

	for ( ; ; )		/* loop over char string */
	{
		/* get and categorize character */
		c = tolower(*str);
		if (isdigitW(c))		ct = 2;
		else if (isspaceW(c))	ct = 0;
		else if (c=='.')		ct = 3;
		else if (c=='\0')	ct = 6;
		else if (c=='-' || c=='+')	ct = 1;
		else if (c=='e' || c=='d')	ct = 4;
		else if (c=='%' && percent)	ct = 5;	/* cond'l recog of '%' */
		else return MH_V0010;		// "Invalid number": unrec char
		//   or bad format


		switch ( actState[s][ct] & 0xF0 )	/* perform actions */
		{
		case SK:
			break;			/* skip character */

		case GSN:
			negVal = (c=='-');	/* get sign: note if '-' */
			break;

		case GD:
			nd++;			/* total digits w/ ldng 0s */
			if ( id > 0 || c != '0')  /* get digit, skp ldng 0s */
				d[id++] = c - '0';
			break;

		case GDD:
			dd[idd++] = c - '0';	/* get dec digit */
			if (c != '0')		/* if non-0 */
			{
				irdd=idd;		/* note rightmost dec. dig. */
				if (id==0)
					id = -idd;	/* and most sig. digit */
			}
			break;

		case GXS:
			negExp = (c=='-');	/* note negative exponent */
			break;

		case GX:
			expo = 10*expo + c - '0';   /* accum exponent value */
			if (expo > 1000)
				return MH_V0013;	// quit b4 expo overflows
			// "Value must be between 10e-38 and 10e38"
			break;

		case DONE:
			if (negExp)
				expo = -expo;
			if (id < 0)	/* if only digits right of dp */
				id++;	/*  set most sig pos to final value */
			goto makeval;

		case ERR:
			return MH_V0010;	// "invalid number"

		default:
			;
		}
		s = actState[s][ct] & 0x0F;	/* new state */
		str++;				/* next char */
	}

	/* Upon arrival here, things are set up as follows:
	    id:   Position of most significant digit.  For instance, 1000.012
		  would produce 4, 0.1 would produce 0, 0.001 would produce -2.
	    nd:	  Total digits to left of decimal point including leading 0s
	    idd:  Total digits to right of decimal point including 0s
	    irdd: Position of least sig. non-0 digit to right of decimal point.
		  1000. would produce 0, 1000.01200 would produce 3.
	    expo: Exponent.  Truly ridiculous values have been eliminated above.
	*/
makeval:

	/* various validity checks */
	if ( (nd + idd) == 0)
		return MH_V0010;		/* "invalid number": no digits */
	if (    id > PTENSIZE 	/* more than 16 signif dgts to left of d.p. */
			|| irdd > PTENSIZE-1)	/* more than 16 signif dgts to rght of d.p. */
		return MH_V0016;		/* "Too many digits.  Use E format for
       				   very large or very small values" */
	if (expo+id > FLT_MAX_10_EXP	// FLT_MAX_10_EXP = 38 (float.h)
			|| expo+id-1 < -FLT_MAX_10_EXP)
		return MH_V0013;	/* "Value must be between 10e-38 and 10e38" */

	/* sum up value of all non-0 digits starting from least significant */
	v = 0.;
	for (s = irdd; --s >= 0; )
		if ( dd[s] )		/* if non-0 digit right of dp */
			v += (double)dd[s]/Pten[s+1];
	for (s = id; --s >= 0; )
		if ( d[s] )	    	/* if non-0 digit left of dp */
			v += (double)d[s]*Pten[id-s-1];

	/* scale by 10**expo */
	if (expo)			/* if there's a non-0 exponent */
	{
		if (expo > 0 && expo < PTENSIZE)
			v *= Pten[expo];	/* pos expo: mult by pwr of 10 */
		else if (expo < 0 && -expo < PTENSIZE)
			v /= Pten[-expo];	/* neg expo: div by pwr of 10 */
		else
			v *= pow( 10., (double)expo);	/* outside Pten[] range, use pow */
	}
	*vp = negVal ? -v : v;
	return RCOK;
}				/* cvatof */

#if 1 /* add 'w' 7-25-90 */
#pragma optimize("aw",off)	/* turn off no-aliases options because args
s1 and s2 point into same string. (With
								   -Oa on, compiler moved "*s2 = c;" before
								   cvatof, defeating partial-string decode).
"w" added as insurance 7-25-90 since deflt
optimization now includes -Ow.  Did NOT
verify that -Ow makes bad code.  chip */
#else
x #pragma optimize("a",off)	/* turn off no-aliases option becuase args
x				   s1 and s2 point into same string. (With
x				   -Oa on, compiler moved "*s2 = c;" before
x				   cvatof, defeating partial-string decode) */
#endif /* 7-25-90 */
//========================================================================
RC FC cvatof2(		/* Substring variant of cvatof() */

	char *s1,		/* Pointer to beg of string to be converted */
	char *s2,		/* Pointer to end+1 of string to be converted */
	double *vp,		/* Pointer to double to receive converted value */
	SI percent )	/* nz: trailing '%' is acceptable (and ignored)
    			   0:  '%' treated as error */

/* This function temporarily replaces *s2 with '\0' and then calls
   cvatof().  Return RC describes outcome of conversion.  See cvatof(). */
{
	char c;
	RC rc;

	c = *s2;
	*s2 = '\0';
	rc = cvatof( s1, vp, percent);
	*s2 = c;
	return rc;
}			/* cvatof2 */
#if 1 /* restore back prior options only, chip 7-25-90 */
#pragma optimize("",on)	/* restore cmd line/default optimization */
#else
x #pragma optimize("a",on)	/* restore */
#endif /* 7-25-90 */

// end of cvpak.c
