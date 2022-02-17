// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cgwthr.h -- Include file for CSE hourly weather functions in cgwthr.c

/*-------------------------- PUBLIC VARIABLES ----------------------------- */

// Weather data set by cgwthr:cgWfRead() (much duplicated in Top members)
//old 1-94, new decl in cnguts.h:
//  extern struct WFDATA NEAR Wthr;	// Current hour's weather data, with xxxFactor modifications.
//1-94 see cnguts.cpp:Wthr for hour's weather data; also cnguts.cpp:Wfile for weather file info; class decls in rccn.h.


/*-------------------------- PUBLIC FUNCTIONS ------------------------------*/
// cgwthr.cpp
extern void FC cgWthrClean( CLEANCASE cs);

// end cgwthr.h
