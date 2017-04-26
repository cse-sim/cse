// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// mmpak.cpp: Maintain moving mean of a sequence of values

/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>
#include <mmpak.h>		// declarations for this file


//=================================================================
MMEAN::MMEAN()
{	mm_Init( 0);
}
//-----------------------------------------------------------------
MMEAN::~MMEAN()
{	delete[] vals;
}
//-----------------------------------------------------------------
void MMEAN::mm_Init(		// Allocate and initialize a moving mean structure
	int _n )	// Duration of moving mean window.  For example, to get moving mean value for 7 days, call with n = 7.
{
	delete[] vals;
	vals = NULL;
	if (_n > 0)
		vals = new float[ _n];
	n = _n;
    ncur = 0;		// Current number of values in vals[]
    old = 0;		// Offset to oldest value in vals[]
    sum = 0.f;		// Current sum of values in vals[]
}		// mm_Init
//===================================================================
float MMEAN::mm_Mean( 	// Maintain moving mean value
	float val )	// current value to be used to update moving mean

// Returns current moving mean value
{
	if (ncur < n) 		// if don't yet have n values
		vals[ ncur++] = val;	// add new value to end of array, increment # values in sub
	else 				// already have n values
	{	sum -= vals[ old];	// subtract oldest (intially vals[0]) from sum
		vals[ old++] = val;  	// store new value over old. Next value is now oldest,
		if (old==n) 		// with wrap at end array
			old = 0;			// .. .
	}
	sum += val;			// add new value to sum
	return sum/ncur;		// divide sum by # values for average
}				// MMEAN::mm_Mean

// end of mmpak.cpp
