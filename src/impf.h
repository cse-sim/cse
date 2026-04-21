// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// impf.h  header for Import Files for CSE

// largest field number to accept (expect far fewer fields in a file)
const int FNRMAX = 1024;

// compile support functions used from cuparse.cpp
RC impFcn( const char* impfName, TI* iffnmi, int fileIx, int inputLineNo, IVLCH* imFreq, const char* fieldName, SI* fnmi);
RC impFcn( const char* impfName, TI* iffnmi, int fileIx, int inputLineNo, IVLCH* imFreq, SI fnr);

RC clearImpf();		// Import stuff special clear function
RC topImpf();			// check/process ImportFiles at end of input
RC impfStart();
RC impfAfterWarmup();
void impfEnd();
void impfIvl(IVLCH ivl);

// runtime import-field functions in impf.cpp, used from cueval.cpp.
RC impFldNmN( int iffnmi, int fnmi, float* pv,       int fileIx, int line, MSGORHANDLE* pms);
RC impFldNmS( int iffnmi, int fnmi, const char** pv, int fileIx, int line, MSGORHANDLE* pms);
RC impFldNrN( int iffnmi, int fnr,  float* pv,       int fileIx, int line, MSGORHANDLE* pms);
RC impFldNrS( int iffnmi, int fnr,  const char** pv, int fileIx, int line, MSGORHANDLE* pms);

// end of impf.h
