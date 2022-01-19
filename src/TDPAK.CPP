// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// tdpak.cpp -- time and date manipulation routines

// Note: to get current date and time see envpak.cpp:ensystd() and ensysltd()

/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>
#include <time.h>	// struct tm
#include <tchar.h>

#include <msghans.h>	// MH_xxxx defns

#include <tdpak.h>	// declarations for this file

// dtypes.def/dtypes.h types used in tdpak calls include:
//  IDATE:     struct {SI year; SI month; SI mday; SI wday; }
//  ITIME:     struct {SI hour; SI min; SI sec; }
//  IDATETIME: struct {SI year; SI month; SI mday; SI wday; SI hour; SI min; SI sec; }
//  LDATETIME: LI  seconds from 1/1/70
//  1-based: .month, .mday;  typedef SI MONTH.
//  0-based: .wday, .hour, .min, .sec.; typedef SI DOW.

#define LEAPDAY(y,m) ((y) >= 0 && !((y)%4) && (m) > 2)
// local functions
int tdLeapDay(		// 1 iff after Feb in leap year
	int yr,		// actual year or -1
	int mon)	// month, 1-12
{	return  yr >= 0 && !(yr%4) && mon>2;
}
int tdYearLen(		// year length (365 or 366)
	int yr)		// actual year or -1
{	return 365 + (yr>=0 && !(yr%4));
}

// Date/time info structure
//    Month related entries have 14 slots:
//    entry 0 is a dummy to allow months to be numbered 1-12;
//    entry 13 is a dummy Jan of next year that simplifies some code.
struct TDINFO	    // for tdpak:Tdinfo
{   int mname[14];	// Tdsnake Snake offset to month names
    int mabrev[14];      // Tdsnake Snake offset to month abbrevs.
    int mlen[14];        // Month length, days
    int mdbeg[14];       // Day of year month begins in non-leap year
    int mdend[14];       // Day of year month ends in non-leap year
    int downame[7];      // Tdsnake Snake offsets for day of week names
};

static TDINFO TdInfo =
{
	{    5,   8,  22,  37,  49,  61,  66,  77,  88, 101, 117, 131, 146,   5  },
	{    5,  17,  32,  44,  56,  61,  72,  83,  96, 112, 126, 141, 156,   5  },
	{    0,  31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31,   0  },	// mlen
	{    0,   1,  32,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335, 366  },	// mdbeg
	{    0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365,   0  },	// mdend
	{  161, 166, 171, 176, 181, 186, 191  }
};

// Historical (3-92) snake of month names etc.
// Access by adding offset (as from a Tdinfo member) to this pointer.
// (a "snake" is multiple strings with nulls between them.
static const char Tdsnake[] = "\0\0\0\0\0\0\0\0January\0\0Jan\0\0February"
				 "\0\0Feb\0\0March\0\0Mar\0\0April\0\0Apr\0\0May\0\0June\0\0Jun\0\0July"
				 "\0\0Jul\0\0August\0\0Aug\0\0September\0\0Sep\0\0October\0\0Oct\0\0"
				 "November\0\0Nov\0\0December\0\0Dec\0\0"
				 "Sun\0\0Mon\0\0Tue\0\0Wed\0\0Thu\0\0Fri\0\0Sat\0\0";

/*------------------------------- VARIABLES -------------------------------*/
// make public if needed
static int Tdfulldate = FALSE;	// FALSE:  display date in "12-Jun-86" format.
								// TRUE: display full "June 12, 1986" date.
static int Td24hrtime = FALSE;	// FALSE:  display time in 12 hr AM/PM format.
								// TRUE: use 24 hr time display.

/*=============================== TEST CODE ================================*/
/* Test main routine */
#undef TEST
#ifdef TEST
t main ()
t{
t IDATE idate;
t IDATETIME idatetime;
t LDATETIME ldt;
t struct tm *ptr;		/* msc time.h structure */
t SI jday,year,eday;
t char buff[100];
t char answer[100];
t SI iflg;
t SEC sec;
t
t #undef TESTTDDDCD
t #ifdef TESTTDDDCD
t     for ( ; ; )
t     {	 printf("%s\n"," enter date string :");
t        gets(answer);
t        iflg=tddsi(answer,&idate);
t        if (iflg==RCOK)
t           printf("%s\n",tddis(&idate,NULL));
t        else
t           printf("   YAK\n");
t }
t #endif
t #undef TESTTDCDYI
t #ifdef TESTTDCDYI
t
t     for (year = 86; year < 93; year++)
t     {	 eday = year >= 0 && !(year%4) ? 366 : 365;
t        eday = 65;
t        printf("\n%d  %d", (INT)year, (INT)eday);
t        for (jday = 1; jday <= eday; jday++)
t        {	tddyi( jday, year, &idate);
t 			if (tddiy(&idate) != jday)
t 				???;
t 			printf("\n%d   %s", (INT)jday, tddis(&idate,buff));
t		}
t}
t #endif
t #undef TESTTDDTIS
t #ifdef TESTTDDTIS
t     ensystd(&idatetime);
t     printf("[%s]   ",tddis(idatetime,buff));
t     printf("[%s]   ",tdtis(&(idatetime.hour),buff));
t     printf("[%s]\n",tddtis(&idatetime,buff));
t #endif
t
t #define TESTTDLDTS
t #ifdef TESTTDLDTS
t     time(&ldt);				/* msc library */
t     ptr = localtime(&ldt);		/* msc library */
t     printf("[%s]\n",tdldts(ldt,buff));
t #endif
t}		/* main */
#endif	/* TEST */

//=======================================================================
const char* tddMonAbbrev(		// month abbreviation "Jan"
	int iMon)		// month (1 - 12)
{	return Tdsnake+TdInfo.mabrev[ iMon]; }
//-----------------------------------------------------------------------
const char* tddMonName(			// month name "January"
	int iMon)		// mont (1 - 12)
{	return Tdsnake+TdInfo.mname[ iMon]; }
//-----------------------------------------------------------------------
const char* tddDowName(			// day of week "Mon"
	int iDow)		// day of week (0=Sun - 6=Sat)
{	return Tdsnake+TdInfo.downame[ iDow]; }
//-----------------------------------------------------------------------
DOY tddDoyMonBeg(				// doy of 1st day of month (1 - 365)
	int iMon)		// month (1 - 12)
{	return TdInfo.mdbeg[ iMon]; }
//-----------------------------------------------------------------------
DOY tddDoyMonEnd(				// doy of last day of month (1 - 365)
	int iMon)		// month (1 - 12)
{	return TdInfo.mdend[ iMon]; }
//-----------------------------------------------------------------------
int tddMonLen(				// length of month
	int iMon)		// month (1 - 12)
{	return TdInfo.mlen[ iMon]; }
//-----------------------------------------------------------------------
const char* tdldts( 			// Convert LI date/time to string

	LDATETIME ldt,	// Date/time to be converted (seconds from 1/1/70 (MSDOS)).
	// Note: use envpak.cpp:ensysldt(), or msc library time(), to get current time in this format.
	char *s )		// NULL to use Tmpstr[], or buffer to receive string.

// Returns s for convenience
{
	IDATETIME idt;

	tdldti( ldt, &idt);			// convert to year/month/day/hr/min, next
	idt.wday = -1;			// suppress weekday display
	idt.sec = -1;			// suppress second display
	return tddtis( &idt, s);		// convert IDATETIME to string, this file
}				// tdldts
//=======================================================================
void tdldti( 			// Convert LI date/time to integer year/month/day/hour/minute/second format
	LDATETIME ldt,  	// Date/time to be converted (seconds from 1/1/70(MSDOS)).
	// Note: use envpak.cpp:ensysldt(), or msc library time(), to get current time in this format.
	IDATETIME *idt )	// Pointer to integer format date/time structure
{
	struct tm *ptr; 	// msc time.h structure

// On PC, LDATETIME format is C Runtime LI format -- use library routine to convert
	ptr = localtime( &ldt);		// msc library. adjust date/time for time zone and daylight, break up.
	idt->year   = ptr->tm_year;		// copy to members in our structure
	idt->month  = ptr->tm_mon + 1;
	idt->mday   = ptr->tm_mday;
	idt->wday   = ptr->tm_wday;
	idt->hour   = ptr->tm_hour;
	idt->min    = ptr->tm_min;
	idt->sec    = ptr->tm_sec;
}			// tdldti
//=======================================================================
const char* tddtis( 		// Convert integer date/time structure to string

	IDATETIME *idt,	// Date time structure to be converted.
					// use envpak.cpp:ensystd() to get current date/time in this format.
	char *s )		// NULL to use Tmpstr[], or destination buffer

// uses full or short date format per Tdfulldate global
// uses 24 hour or am/pm time per Td24hrtime global

// Returns s
{
	char *sbeg;
	int hour;

	if (!s)
		s = strtemp( TDFULLDTLENMAX);
	sbeg = s;				// save string start to return
	tddis( *(IDATE *)idt, s);		// date-->string, below. tests Tdfulldate. (beginning of IDATETIME is same as IDATE).
	s += strlen(s);			// point past
	*s++ = ' ';				// add 2 spaces
	*s++ = ' ';				// ..
	if (!Tdfulldate) 			// in constant-width "12-Jun-86" format only
	{
		hour = idt->hour;
		if (!Td24hrtime) 		// if not 24 hour time format
			hour = (hour+11)%12 + 1;	// what tdtis will do to .hour
		if (hour < 10)
			*s++ = ' ';			// leading alignment space if hour 1-9
	}
	tdtis( (ITIME *)&idt->hour, s);	// convert time to string, this file. tests Td24hrtime.
	// NB IDATETIME from .hour on is same as ITIME.
	return sbeg;
}			// tddtis
//=======================================================================
const char* tddys( 			// Convert day of year + year to date string
	DOY doy,			// Day of year 1..365 or 366
	int year /*=-1*/,	// Year, -1 = no year, else actually year (leap year handled)
	char *s /*=NULL*/)	// NULL to use Tmpstr[], or destination buffer.

// uses full or short date format per Tdfulldate global
//   short = "Jan-1"
//   full = "January 1" ?

// Returns s
{
	IDATE tdate;	// year/month/mday/wday
	tddyi( tdate, doy, year);		// convert to integer date structure, next in this file.
	tdate.wday = -1;				// suppress day of week
	return tddis( tdate, s);   	// convert that to string, tests Tdfulldate. in this file
}			// tddys
//=======================================================================
void tddyi( 			// Convert day of year to integer format date (year/month/mday/wday)
	IDATE& idt,		// returned
	DOY doy,		// day of year, 1 .. 366, validity not checked
	int year /*=-1*/)	// year, if >= 0, is a real year and doy will be converted with leap year adjustment.
{
	int im = (doy-1)/31;			// start table lookup at smallest poss month
	while (tddDoyMonBeg( im) + LEAPDAY(year,im) <= doy)
		im++;						// 1st month that begins after doy is month+1
	idt.year = year;
	idt.month = im-1;
	idt.mday = doy - ( tddDoyMonBeg( idt.month) + LEAPDAY(year,idt.month)) + 1;
	idt.wday = tddyw( doy, year);			// day of week
}					// tddyi
//=======================================================================
DOY tddiy(			// Convert integer date structure to day of year
	const IDATE& idt )  	// Date structure.
// If idt.year >= 0, this date is taken to be a real year and leap years are handled.
// If idt.year < 0, this is a generic 365 day year.

// Returns day of year, 1 - 366
{
	return tddDoyMonBeg( idt.month) + LEAPDAY( idt.year, idt.month) + idt.mday - 1;

}			// tddiy
//=======================================================================
DOY tddiy( int month, int mday, int year /*=-1*/)
{	return tddDoyMonBeg( month) + LEAPDAY( year, month) + mday - 1;
}			// tddiy
//=======================================================================
int tddiw(			// Determine day of week corresponding to integer date

	IDATE& idt )   	// Date for which day of week is required.  Leap year accounted for if idt->year >= 0.

// Returns calculated day of week and also stores it in idt->wday.
{
	idt.wday = tddyw( 			// convert doy and year to day of week, next
					tddiy( idt), 	// get day of year, above
					idt.year );
	return idt.wday;
}				// tddiw
//=======================================================================
int tddyw( 			// Determine day of week for day of year

	DOY doy,		// Day of year
	int year )		// If >= 0, leap year will be accounted for.
// Years < 0 are 365 day years beginning on dow = (-year):
//   Jan 1 of year -1 is Mon., Jan 1 of year -2 is Tues ...
// For year TDYRNODOW, -1 for "no day of week" is returned

// Returns calculated day of week 0..6 for Sunday..Saturday, or -1.
{
	if (year == TDYRNODOW)
		return -1;
	return ( doy  + ( year < 0 ? -year-1 : (year+(year-1)/4) )  ) %7;
}			// tddyw
//=======================================================================
const char* tddis( 		// Convert integer format date structure to string
	const IDATE& idt,		// Date structure: year-month-day
	char *s /*=NULL*/ )		// NULL to return in Tmpstr[], or destination buffer

// uses full or short format per Tdfulldate global

// Returns s
{
	if (!s)
		s=strtemp( TDFULLDATELENMAX);
	const char* sbeg = s;		// save to return

// start with weekday if not suppressed.  Advance s past text.
	if (idt.wday != -1)
		s += sprintf( s, "%s ", tddDowName( idt.wday) );

// full format date
	if (Tdfulldate) 				// global full-date format flag
	{
		s += sprintf( s, "%s %d", 			// do month, day, point past
				tddMonName( idt.month),
				idt.mday );
		if (idt.year >= 0) 			// if real year given
		{
			int yt = idt.year;		// fetch to modify
			if (yt < 100) 			// unless already present
				yt += 1900;			// supply the 19 for full date format
			sprintf( s, ", %4d", yt);		// format and append year
		}
	}
// short format date: "12-Jun-86" format
	else
	{
		s += sprintf( s, "%2.2d-%s", 					// format dd-mon, point past
			idt.mday, tddMonAbbrev( idt.month));
		if (idt.year >= 0) 						// if a real year given
			sprintf( s, "-%2.2d", (idt.year)%100 );			// append it
	}
	return sbeg;
}				// tddis
//======================================================================
RC tddsi(		// Decode month-day date string

	char *str,		// Date string
	IDATE *idate)	// Date structure: receives month and day, -1 day of week, and generic no-weekday yr TDYRNODOW

// Accepts a date string of the form DD-month or month-DD, with "-" or "/" or " " seperator.  Month can be abrev. or full name.

// Returns RCOK if ok, else MH of error message, message NOT issued (retreive msg text with messages:msg or rmkerr:err).
{
	RC rc;
	MONTH tmon;
	int iflg2=0;
	short tday;
	char *tstr, *loc, *tok1, *tok2;

	rc = RCOK;					// message code for "no error" (value 0)
	tstr = strTrim( NULL, str);  		// remove lead/trail spaces, strpak.cpp
	tmon = 0;
	tday = 0;
// find separator: accept / or - with optional spaces, or just space(s)
	loc = strpbrk( tstr, "/-");			// look for / or - first
	if (loc == NULL)
		loc = strpbrk( tstr, " ");		// only if no / or -, look for space
	if (loc == NULL)
		rc = MH_V0001;				// "V0001: undecipherable day-month string" msgtbl.cpp
	else
	{
		// split into tokens
		*loc='\0';					// change / - or ' ' to \0
		tok1 =  strTrim( NULL, tstr);  			// deblank each part:
		tok2 =  strTrim( NULL, loc+1);  			// ..addl spaces ok between

		// Try mon-day, then day-mon decodes
		tmon = tddmon(tok1);				// decode month from token 1, next
		if (tmon >= 0)					// if month ok
			iflg2 = sscanf( tok2, "%hd", &tday);		// decode day # from token 2
		else
		{
			tmon = tddmon(tok2);				// try 2nd token as month name
			if (tmon >= 0)				// if ok
				iflg2 = sscanf( tok1, "%hd", &tday);	// try 1st as day #
			else
				rc = MH_V0003;			// "V0003: month name must be 3 letter abbreviation or full name", msgtbl.cpp
		}
		// Check for errors
		if (rc==RCOK)				// if month ok (else iflg2 unset)
		{
			if ( iflg2 != 1 			// if day # sscanf got no token
					||  tday < 1    			// or # value < 1
					||  tday > tddMonLen( tmon) ) 	// or > #days in this month
				rc = MH_V0002;			//  "V0002: illegal day of month", msgtbl.cpp
		}
	}
	idate->year = TDYRNODOW;			// Generic year w/o day of week
	idate->month = tmon;			// store month
	idate->mday = tday;  			// store day of month
	idate->wday = -1;				// No day of week
	return rc;					// return RC code, msg NOT issued
}			// tddsi
//=======================================================================
int tddmon(		// Identify month name
	const char* str)	// String to identify.  To be valid, must match either
						//   abbreviation or full month name.
    					// No leading or trailing whitespace allowed.

// Returns month 1 - 12, or -1 if no match found.
{
	for (int i = 1; i < 13; i++)
		if ( strcmpi( tddMonName( i), str)==0
		 ||  strcmpi( tddMonAbbrev( i), str)==0 )
			return i;
	return -1;
}			// tddmon
//=======================================================================
const char* tdtis( 		// Convert integer format time to string

	ITIME *itm,  	// Time structure: hour-minute-second
	char *s )	 	// NULL to return in Tmpstr[], or pointer to buffer to receive string

// uses 24 hour or am/pm time per Td24hrtime global

// Returns s
{
	char *sbeg, *apchar;
	int hour;

	if (!s)
		s = strtemp( TDTIMELENMAX);
	sbeg = s;					// save beginning to return
	hour = itm->hour;				// fetch hour for possible adjustment
	if (Td24hrtime) 				// global 24-hour time format flag
		apchar = "";				// no am/pm
	else 					// not 24 hour time
	{
		apchar = (hour < 12) ? " am" : " pm";	// get am or pm to append
		hour = (hour+11)%12 + 1;			// convert 0..23 to 1..12
	}
	s += sprintf( s, "%d:%2.2d", (INT)hour, (INT)itm->min);	// format hour:min, point to end
	if (itm->sec != -1) 					// seconds -1 --> no display
		sprintf( s, ":%2.2d", itm->sec);		// format & append :seconds
	return strcat( sbeg, apchar);		// append am/pm if any and return
}				    // tdtis

#ifdef C_HDAYCASECH_FIRST		// omit if CSE choices not defined (dtypes.h not #included)
//=======================================================================
DOY tdHoliDate( 	// determine date of holiday this year

	// where holiday is defined as being "1st Monday in Sep", "Last Mon in May", "4th Thurs in Nov", etc

	int year,			// year, actual (positive, leap considered), or generic -1..-7 for jan 1 = Mon..Sun.
	HDAYCASECH hCase,	// case: C_HDAYCASECH_FIRST, _SECOND, _THIRD, _FOURTH, _LAST
	DOW hDow, 			// day of week (Sun=0) of holiday
	MONTH hMon )		// month of holiday, 1-12

// returns day of year 1-365.

// CAUTION: if generic year with no days of week (year value TDYRNODOW) gets here, an unspecified jan 1 day of week is assumed.
//          caller must precheck if desired to issue error message.

{
	// if year has no days of week, tddyw returns -1, which falls thru like 6 (Saturday).  Or add error return?
	DOY jan1Dow = tddyw( 1, year);				// 0-based day of week of jan 1 of this year, above
	DOY mdy = tddDoyMonBeg( hMon) + LEAPDAY(year,hMon);		// 1-based day of year of beginning of month
	DOY doyDow = mdy + (hDow - jan1Dow - (mdy-1) + 700) % 7;	// day of year of first hDow in month.  do % on positive value.
	switch (hCase)
	{
	case C_HDAYCASECH_LAST:     // last hDow in month: if month long enuf to have 5 hDows, add a week here. fall thru.
		if (doyDow + 28 < tddDoyMonBeg( hMon+1) + LEAPDAY(year,hMon+1))
			doyDow += 7;
	case C_HDAYCASECH_FOURTH:
		doyDow += 7;			// fourth: fall thru cases to add 3 weeks to first
	case C_HDAYCASECH_THIRD:
		doyDow += 7;			// etc
	case C_HDAYCASECH_SECOND:
		doyDow += 7;
	case C_HDAYCASECH_FIRST:
		break;
	}
	return doyDow;
}			// tdHoliDate
#endif	// C_HDAYCASECH_FIRST

///////////////////////////////////////////////////////////////////////////////
// class CALENDAR: modern-ish interface to date/time functions
//                 used in e.g. ASHRAE solar.cpp
//                 transition aid
///////////////////////////////////////////////////////////////////////////////

// public instance
CALENDAR Calendar;
//-----------------------------------------------------------------------------
int CALENDAR::GetMonDoy(		// month for day of year
	int iDoy,				// day of year, 1-365/366
	int yr /*=-1*/) const	// year
// returns month 1 - 12
{
	if (iDoy < 1 || iDoy > tdYearLen( yr))
		return -1;

	IDATE idt;
	tddyi( idt, iDoy, yr);
	return idt.month;
}	// CALENDAR::GetMonDoy
//------------------------------------------------------------------------------
int CALENDAR::GetMDDoy(		// MMDD for day of year
	int iDoy,
	int yr/*=-1*/) const
// returns iMD (MMDD) for doy
{
	if (iDoy < 1 || iDoy > tdYearLen( yr))
		return -1;

	IDATE idt;
	tddyi( idt, iDoy, yr);

	return idt.month*100 + idt.mday;

}		// CALENDAR::GetMDDoy
//-----------------------------------------------------------------------------
int CALENDAR::GetDoy(
	int month,				// month, 1-12
	int mDay,				// day of month, 1-31
	int yr /*=-1*/) const	// actual year or -1
{
	return tddiy( month, mDay, yr);
}	// CALENDAR::GetDoy
//-----------------------------------------------------------------------------
const char* CALENDAR::GetMonAbbrev(		// month abbrev: "Jan", "Feb", ...
	int month) const		// month, 1-12
{	return tddMonAbbrev( month);
}		// CALENDAR::GetMonAbbrev
//-----------------------------------------------------------------------------
WStr CALENDAR::FmtDOY(			// formatted date from DOY
	int iDoy,							// date to be formatted
	int yr /*=-1*/,						// yr
	const char* fmt /*="%b-%d"*/) const	// strftime-style format
										//   default = mmm-dd
// returns date formatted per fmt
//         "?" if invalid doy
{
	WStr s;
	if (iDoy < 1 || iDoy > tdYearLen( yr))
		s = "?";
	else
	{	IDATE idt;
		tddyi( idt, iDoy, yr);
		char buf[ 100];
		struct tm t;
		FillTM( t, idt);
		_tcsftime( buf, sizeof( buf), fmt, &t);
		s = buf;
	}
	return s;
}		// CWCalendar::FmtDOY
//-----------------------------------------------------------------------------
void CALENDAR::FillTM(			// fill struct tm
	struct tm& t,				// returned: set to midnight on YMD
	const IDATE& idt) const	// date
{
	int yr = idt.year >= 0 ? idt.year : 2001;
	int iDoy = GetDoy( idt.month, idt.mday, yr);
    t.tm_sec = 0;			// seconds after the minute - [0,59]
    t.tm_min = 0;			// minutes after the hour - [0,59]
    t.tm_hour = 0;			// hours since midnight - [0,23]
    t.tm_mday = idt.mday;    	// day of the month - [1,31]
    t.tm_mon = idt.month-1;		// months since January - [0,11]
    t.tm_year = yr-1900;		// years since 1900
    t.tm_wday = idt.wday;		// days since Sunday - [0,6]
    t.tm_yday = iDoy-1;			// days since January 1 - [0,365]
    t.tm_isdst = 0;				// daylight savings time flag
}		// CWCalendar::FillTM
//=============================================================================


// end of tdpak.cpp
