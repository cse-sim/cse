// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// timer.h: Definitions for program timing routines (timer.cpp).

// Function declarations
void  tmrInit( const char* tmrName, int tmr);
void  tmrStart( int tmr);
void tmrStop( int tmr);
double tmrCurTot( int tmr);
void  tmrDisp( FILE *f, int tmr, double delta);		// may be if 0'd
RC tmrVrDisp( int vrh, int tmr, const char* pfx="", double delta=0.);

// End of timer.h

