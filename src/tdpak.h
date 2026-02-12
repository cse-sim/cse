// tdpak.h -- declarations for time and date related functions (tdpak.cpp)

#pragma once

// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/*-------------------------------- DEFINES --------------------------------*/

// Constants - Note that string lengths do NOT include room for '\0'
constexpr int TDFULLDATELENMAX = 22;			// Max date string length (full format) ("Sun September 26, 1986")
constexpr int TDDATELENMAX = 13;				// Max date string length (abbr format) ("Sun 26-Sep-86")
constexpr int TDTIMELENMAX = 11;				// Max time string length ("01:23:14 AM")
constexpr int TDFULLDTLENMAX = TDFULLDATELENMAX+TDTIMELENMAX+2;	// Max date/time string max length (full format)
constexpr int TDDTLENMAX = TDDATELENMAX+TDTIMELENMAX+2;		// Max date/time string max length (abbr format)
constexpr int TDYRNODOW = -30000;				// Pseudo-year which has no day of week associated with it.  See tddyw();


// date/time related types
//  1-based: .month, .mday;
//  0-based: .wday, .hour, .min, .sec, .dow

// CAUTION: code in tdpak.cpp (at least) assumes IDATETIME is same as an IDATE followed by an ITIME

struct IDATETIME
{
	SI year;
	SI month;	// 1 - 12
	SI mday;	// 1 - 28,29,30,31
	SI wday;	// 0 - 6
	SI hour;	// 0 - 23
	SI min;		// 0 - 59
	SI sec;		// 0 - 59
};		// struct IDATETIME

// defined in cnglob.h due to uses in cnrecs.def
// struct IDATE { SI year; 	SI month; SI mday; 	SI wday; };

struct ITIME
{
	SI hour;
	SI min;
	SI sec;
};		// struct ITIME

typedef time_t LDATETIME;	// seconds from 1/1/70
							// (duplicate in cnglob.h)

// public functions
inline bool tdIsLeapYear( int yr)	// year (<0 for generic non-leap year)
{	return yr >= 0 && (yr & (yr%100==0 ? 15 : 3)) == 0; }
int tdLeapDay(int yr, int iMon);
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
DOY tddDoyMonBeg( int iMon, bool bLeapYr=false);
DOY tddDoyMonEnd( int iMon, bool bLeapYr=false);
int tddMonLen( int iMon, bool bLeapYr=false);
DOY tdHoliDate( int year, HDAYCASECH hCase, DOW hDow, int hMon);

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
