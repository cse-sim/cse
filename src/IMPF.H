// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// IMPF.H  header for Import Files for CSE  2-94


// largest field number to accept (expect far fewer fields in a file)
#define FNRMAX 1024


// compile support functions, cncult4.cpp, used from cuparse.cpp
RC impFcn( char *impfName, TI *iffnmi, USI fileIx, USI line, IVLCH *imFreq, char *fieldName, SI *fnmi);
RC impFcn( char *impfName, TI *iffnmi, USI fileIx, USI line, IVLCH *imFreq, SI fnr);

// runtime import-file functions in impf.cpp, used from cnguts.cpp
RC FC impfStart();
RC FC impfAfterWarmup();
void FC impfEnd();
void FC impfIvl(IVLCH ivl);

// runtime import-field functions in impf.cpp, used from cueval.cpp.
RC impFldNmN( SI iffnmi, SI fnmi, float *pv, USI fileIx, USI line, const char** pms);
RC impFldNmS( SI iffnmi, SI fnmi, char **pv, USI fileIx, USI line, const char** pms);
RC impFldNrN( SI iffnmi, SI fnr,  float *pv, USI fileIx, USI line, const char** pms);
RC impFldNrS( SI iffnmi, SI fnr,  char **pv, USI fileIx, USI line, const char** pms);

// end of impf.h
