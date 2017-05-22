// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// mmpak.h: mmpak.cpp -- moving mean routines

// Moving Mean data structure
struct MMEAN {
    int n;		// Duration of moving mean (number of observations)
    int ncur;		// Current number of values in vals[]
    int old;		// Offset to oldest value in vals[]
    float sum;		// Current sum of values in vals[]
    float* vals;	// Table of values for last n observations

	MMEAN();
	~MMEAN();
	void mm_Init( int _n=0);
	float mm_Mean( float val);

};

// mmpak.h end
