// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cuevf.h  evaluation frequency and variability definitions

// for cuparse/cul world, runtime, rcdef.

// evaluation frequency (variation) bits for operand or expression
// how often program may change value
// constants defined as int, must fit in USI
const int EVFMAX   =  1024;		// largest evf bit value. 128-->256 6-95; 256->1024 8-17
const int EVFSUBHR =  1024; 	// every sub-time-step
const int EVFHR    =   512; 	// hourly
const int EVFMH	   =   256;		// monthly-hourly -- but not daily:
								//  for solar etc stuff appl computes for each hour on 1st day of month & uses for whole month.
								// cuparse.cpp:cleanEvf() cond'ly changes MON|HR to MH.
const int EVFDAY   =   128;		// daily
const int EVFMON   =    64;		// monthly
const int EVFRUN   =    32;   	// at start of autosize and run, after check/setup, or at end if EVENDIVL bit also on.
								
								// interval stages: beg, end, post
const int EVPSTIVL =     8;		// set for values available after post-processing calcs for their intervals
								//   (eg load management)
const int EVENDIVL =     4;		// set for values available at END of ther calcs for intervals (results rather than inputs)
const int EVBEGIVL =     0;		// implicit for values available at beg of interval (inputs)
const int EVXBEGIVL = EVENDIVL | EVPSTIVL;	// all non-beg stages

// EVEOI, EVFFAZ are evaluted to input rats b4 check/setup (ie b4 topCkf); others at runtime to run rats.
const int EVFFAZ  =     2;		// evaluate before check/setup of autosize and of main simulation (run) phases 6-95
const int EVEOI    =    1;		// evaluate before initial check/setup (end-of-input).
								// for constant use zero.
const int EVFDUMMY = 0x4000; 	// cuparse.c internal: expr contains dummy arg ref
								// --> don't eval or konstize in absence of call.

//--- variabilities for CULT.evf table member (cul.h/c): acceptable evaluation frequencies for input exprs. USI or unsigned:12.

#if 0 // historical; believe (no case at hand) MH value used in hrly context would yield wrong value when not 1st day of month
x #define VSUBHRLY    (EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFMH|EVFDAY|EVFHR|EVFSUBHR)	// subhourly or less
x #define VHRLY       (EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFMH|EVFDAY|EVFHR)		// hourly or less
#else // 9-16-95: remove MH from HR, SUB.
 const int VSUBHRLY = EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFDAY|EVFHR|EVFSUBHR;	// subhourly or less
 const int VHRLY    = EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFDAY|EVFHR;			// hourly or less
#endif

#if 0 // historical; believe hourly value works but wastes time when not 1st day of month
x #define VMHLY       (EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFMH|EVFHR)  	/* "monthly-hourly": depends on mon & hr, not on day:  for inputs
x								   to appl's solar data: computed for 24 hrs once at month start.*/
#else // 9-16-95: remove HR; changed cleanEvf.
const int VMHLY = EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFMH; 
									// "monthly-hourly": depends on mon & hr, not on day:  for inputs
									// to appl's solar data: computed for 24 hrs once at month start.
									// Note cuparse.cpp:cleanEvf cnd'ly rets EVFMH for EVFHR, 9-95.
#endif

const int VDAILY  =    EVEOI|EVFFAZ|EVFRUN|EVFMON|EVFDAY;	// daily or less
const int VMONLY   =   EVEOI|EVFFAZ|EVFRUN|EVFMON; 			// monthly or less
//const int VINITLY  = EVEOI|EVFFAZ|EVFRUN;					// add if need found: at start of run or less (less = end of input or constant)
//const int VENDLY   = EVEOI|EVFFAZ|EVFRUN|EVENDIVL;		// add if need found: at end of run only
const int VFAZLY   =   EVEOI|EVFFAZ;						// at start of autosizing and start of main simulation or less 6-95
const int VEOI     =   EVEOI;								// eval can only be deferred til end of input
// if constant required, use 0

// end of cuevf.h
