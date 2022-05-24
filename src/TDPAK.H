// tdpak.h -- declarations for time and date related functions (tdpak.cpp)

// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/*-------------------------------- DEFINES --------------------------------*/

// Constants - Note that string lengths do NOT include room for '\0'
#define TDFULLDATELENMAX 22				// Max date string length (full format) ("Sun September 26, 1986")
#define TDDATELENMAX 13  				// Max date string length (abbr format) ("Sun 26-Sep-86")
#define TDTIMELENMAX 11					// Max time string length ("01:23:14 AM")
#define TDFULLDTLENMAX TDFULLDATELENMAX+TDTIMELENMAX+2	// Max date/time string max length (full format)
#define TDDTLENMAX TDDATELENMAX+TDTIMELENMAX+2		// Max date/time string max length (abbr format)
#define TDYRNODOW -30000				// Pseudo-year which has no day of week associated with it.  See tddyw();


// dtypes.def/dtypes.h types used in tdpak calls include:
//  IDATE:     struct {SI year; SI month; SI mday; SI wday; }
//  ITIME:     struct {SI hour; SI min; SI sec; }
//  IDATETIME: struct {SI year; SI month; SI mday; SI wday; SI hour; SI min; SI sec; }
//  LDATETIME: LI  seconds from 1/1/70
//  1-based: .month, .mday;  typedef SI MONTH.
//  0-based: .wday, .hour, .min, .sec.; typedef SI DOW.


// public functions
const char* tdldts( LDATETIME, char *);
void   tdldti( LDATETIME, IDATETIME *);
const char* tddtis( IDATETIME *, char *);
const char* tddys( DOY iDoy, int year=-1, char *s = NULL);
void   tddyi( IDATE& idt, DOY doy, int year=-1);
DOY    tddiy( const IDATE& idt);
DOY    tddiy( int month, int mday, int year=-1);
#if 0
int    tddiw( IDATE *);
#endif
int     tddyw( DOY, int);
const char* tddis( const IDATE& idt, char *s=NULL);
RC     tddsi( char *, IDATE *);
int tddmon( const char *);
const char* tddMonAbbrev( int iMon);
const char* tddMonName( int iMon);
const char* tddDowName( int iDow);
const char* tdtis( ITIME *, char *);
DOY tddDoyMonBeg( int iMon);
DOY tddDoyMonEnd( int iMon);
int tddMonLen( int iMon);
DOY tdHoliDate( int year, HDAYCASECH hCase, DOW hDow, MONTH hMon); 	// 3-6-92

// modern-ish interface used by e.g. ASHRAE solar.cpp
class CALENDAR
{
public:
	CALENDAR() {}

	int GetMonDoy( int iDoy, int yr=-1) const;
	int GetMDDoy( int iDoy, int yr=-1) const;
	int GetDoy( int month, int mDay, int yr=-1) const;
	const char* GetMonAbbrev( int month) const;
	WStr FmtDOY( int iDoy, int yr=-1, const char* fmt="%b-%d") const;
	void FillTM( struct tm& t, const IDATE& idt) const;


};	// class CALENDAR
//-----------------------------------------------------------------------------
extern CALENDAR Calendar;	// public intance
//=============================================================================

// tdpak.h end
