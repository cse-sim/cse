// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// IMPF.H  header for Import Files for CSE  2-94


// largest field number to accept (expect far fewer fields in a file)
const int FNRMAX = 1024;

// compile support functions used from cuparse.cpp
RC impFcn( char *impfName, TI *iffnmi, int fileIx, int line, IVLCH *imFreq, char *fieldName, SI *fnmi);
RC impFcn( char *impfName, TI *iffnmi, int fileIx, int line, IVLCH *imFreq, SI fnr);

RC   FC clearImpf();		// Import stuff special clear function
RC   FC topImpf();			// check/process ImportFiles at end of input
RC FC impfStart();
RC FC impfAfterWarmup();
void FC impfEnd();
void FC impfIvl(IVLCH ivl);

// runtime import-field functions in impf.cpp, used from cueval.cpp.
RC impFldNmN( int iffnmi, int fnmi, float *pv, int fileIx, int line, const char** pms);
RC impFldNmS( int iffnmi, int fnmi, char **pv, int fileIx, int line, const char** pms);
RC impFldNrN( int iffnmi, int fnr,  float *pv, int fileIx, int line, const char** pms);
RC impFldNrS( int iffnmi, int fnr,  char **pv, int fileIx, int line, const char** pms);

// end of impf.h
