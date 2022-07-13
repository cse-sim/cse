// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// Library stubs -- function definition stubs when not using CSE definitions
//                  e.g., for RCDEF and unit tests

#include "cnglob.h"
#include "vrpak.h"

VROUTINFO5 PriRep = { { 0 } };
const char* InputFilePathNoExt = NULL;

//------------------------------------------------------------------------------------------
int getCpl( class TOPRAT** pTp /*=NULL*/)    // get chars/line (stub fcn, allows linking w/o full CSE runtime)
{
	pTp;
	return 78;
}
//---------------------------------------------------------------------------------------------------------------------------
char* getErrTitleText()

{
	return "";
}
//---------------------------------------------------------------------------------------------------------------------------
char* getLogTitleText()

{
	return "";
}
//---------------------------------------------------------------------------------------------------------------------------
int getBodyLpp()
{
	return 66;
}
//---------------------------------------------------------------------------------------------------------------------------
char* getFooterText([[maybe_unused]] int pageN)
{
	return "";
}
//---------------------------------------------------------------------------------------------------------------------------
char* getHeaderText([[maybe_unused]] int pageN)
{
	return "";
}
//-----------------------------------------------------------------------------
int CheckAbort()
// in CSE, CheckAbort is used re caller interrupt of DLL simulation
// here provide stub for RCDEF link
{	return 0;
}
