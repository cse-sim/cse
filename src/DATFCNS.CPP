// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// datfcns.cpp	minimal date functions for small standalone programs

// see tdpak.cpp for a more complete set of date fcns, plus time fcns.

// created by rob, 10-94, to remove tdpak from utility programs;
// is linked in CSE for use by modules also used elsewhere, eg yacam.cpp 10-94.


#include <CNGLOB.H>

#include <DATFCNS.H>	// header for this file

//============================================================================
//  minimal standalone date input functions: for YACAM::get, also useful separately
//----------------------------------------------------------------------------
//--- data for date decoding
static short mDays[] = { 31, 28, 31, 30, 31, 30,   31, 31, 30, 31, 30, 31 };	// # days in each month
static short fDays[] = {  1, 32, 60, 91, 121,152, 182,213,244,274,305,335 };	// first 1-based julian date of month
//----------------------------------------------------------------------------
short FC dMonDay2Jday( 	// convert month and day of month to 1-based Julian date for non-leap year
	short mon, 			// month 1-12
	short day,			// day of month 1-31
	short isLeap /*=0*/ )	// non-0 if leap year
{
	return fDays[mon-1]
		   + day - 1
		   + (isLeap && mon > 2 ? 1 : 0);
}
//----------------------------------------------------------------------------
RC FC dStr2MonDay( 	// convert string to month and day. caller has removed any quotes.

	const char *str,		// input string
	short *pMon, short *pDay )	// these receive results

// returns non-RCOK on failure, NO MESSAGE ISSUED HERE
{
// split into month and day tokens and deblank both ends of both tokens in place (modifies caller's buffer)
	while (isspaceW(*str))  				// deblank beginning: advance ptr
		*str++;
	char buf[30];
	char * s1 = strncpy( buf, str, sizeof(buf)-1);	// make a copy to modify. s1 will be start 1st token.
	buf[sizeof(buf)-1] = '\0';
	char *p = s1 + strlen(s1);
	while (p > s1 && (!p[-1] || isspaceW(p[-1])) )  	// deblank end: store \0's over trailing (\0's and) spaces
		* --p = '\0';
	p = strpbrk( s1, "/-");				// find break: / or - (can be surrounded with spaces)
	if (!p)
		p = strpbrk( s1, " \t");				// else space or tab is break
	if (!p)
		return RCBAD;
	*p = '\0';						// change delimiter to \0 to terminate first token
	for (char *s2 = p+1;  isspaceW(*s2);  s2++)		// s2 = start 2nd token. deblank.
		;
	while (p > s1 && (!p[-1] || isspaceW(p[-1])) ) 	// deblank end 1st token
		* --p = '\0';

// decode tokens as month and day of month
	if (isdigitW(*s1))
	{
		char *t = s1;				// interchange so s1 is month, s2 is day # string
		s1 = s2;
		s2 = t;
	}
	else if (!isdigitW(*s2))
		return RCBAD;
	short day = atoi(s2); 			// get day of month value
	while (isdigitW(*s2)) 			// syntax check: pass all digits
		s2++;
	if (*s2)  					// if anything else, it is bad
		return RCBAD;
	short mi = dStr2Mi(s1); 			// get month index 0-11 (next function)
	if (mi < 0 || mi > 11)  			// check for good month
		return RCBAD;
	if (day < 1 || day > mDays[mi])   		// check for good day of month
		return RCBAD;

// return results, making month 1-based
	*pDay = day;
	*pMon = mi+1;
	return RCOK;
}			// dStr2MonDay
//----------------------------------------------------------------------------
short FC dStr2Mi( const char *str)	// from string get month index 0-11 or -1 if not a month
{
	size_t len = strlen(str);
	const char * p = (len <= 3)
					 ?  "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec   "
					 :  "January February March April May June July "
					 "August September October November December   ";
	if (len <= 9)							// don't run off end of p: poss GP fault
		for (short mi = 0;  mi < 12;  mi++, p = strchr(p,' ')+1)  	// point after next space each time
			if (p[len]==' '  &&  !memicmp( str, p, len))
				return mi;
	return -1;
}
//---------------------------------------------------------------------------

//===========================================================================
//   date manipulation functions
//===========================================================================
//-- date data: first 1-based day of each 0-based month
SI mdBeg[14] = { 0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
//---------------------------------------------------------------------------
void FC dJday2MonDay( 		// convert Julian date to month and day
	SI jDate, 			// Julian date 1-365 (1-366 if leap year)
	SI *pMon, SI *pMDay, 	// receive month 1-12 and day of month 1-31
	SI isLeap /*=FALSE*/)	// non-0 if a leap year
{
	SI mon = (jDate-1)/31+1;					// month 1-12: start with smallest possible value
	while ( jDate >= mdBeg[mon+1] + (isLeap && mon >= 2)	// month is month b4 1st month beginning after jDate
			&&  mon < 12 )
		mon++;
	*pMDay = jDate - mdBeg[mon] - (isLeap && mon >= 2) + 1;
	*pMon = mon;
}			// dJday2MonDay
//---------------------------------------------------------------------------

//===========================================================================
//   minimal standalone date output functions
//===========================================================================
// short month names for 1-based months
static char * mabrevStr[14] = { "Bug0","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","Bug13"};
//---------------------------------------------------------------------------
char * FC dMonDay2Str( 		// convert month and day to string in form "1-Jan"
	SI mon, 			// month 1-12
	SI mDay, 			// day of month 1-31
	char *bufp /*=NULL*/ )  	// text buffer address or NULL to use internal buffer (move b4 next call!)
{
	static char buf[20];
	if (!bufp)
		bufp = buf;
	sprintf( bufp, "%2.2d-%s", mDay, mabrevStr[mon]);
	return bufp;
}			// dMonDay2Str
//---------------------------------------------------------------------------
char * FC dJday2Str( 		// convert Julian date to string "1-Jan"
	SI jDate,
	SI isLeap /*=FALSE*/,
	char *buf /*=NULL*/ )   	// NULL to use internal static buffer
{
	SI mon, mDay;
	dJday2MonDay( jDate, &mon, &mDay, isLeap);
	return dMonDay2Str( mon, mDay, buf);
}
//---------------------------------------------------------------------------

// end of datfcns.cpp
