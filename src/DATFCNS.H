// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// datfcns.h  header for datfcns.cpp -- date functions

short FC dMonDay2Jday( short mon, short day, short isLeap=0);	// formerly "dJDay" 10-94
RC FC dStr2MonDay( const char *str, short *pMon, short *pDay);	// formerly "dMonDay" 10-94
short FC dStr2Mi( const char *str);				// formerly "dMoni" 10-94

void   FC dJday2MonDay( SI jDate, SI *pMon, SI *pMDay, SI isLeap=FALSE);	// was "jDate2MonDay" 10-94
char * FC dMonDay2Str( SI mon, SI mDay, char *buf = NULL);			// was "monDay2Str" 10-94
char * FC dJday2Str( SI jDate, SI isLeap=FALSE, char *buf=NULL);		// was "jDate2Str" 10-94

// end of datfcns.h
