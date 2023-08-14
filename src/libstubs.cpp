// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// Library stubs -- function definition stubs when not using CSE definitions
//                  e.g., for RCDEF and unit tests

// #include "cnglob.h"  NO: the idea here is minimal dependencies
//    provide only bare-bones definitions

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
// here provide stub
{	return 0;
}
//-----------------------------------------------------------------------------
bool vrIsEmpty(int)
{
	return true;	// return "not empty"
}
//-----------------------------------------------------------------------------
short vrStr(int, const char*)
{
	return 0;	// return RCOK
}
//-----------------------------------------------------------------------------
short vrOpen(int *, const char*, int)
{
	return 0;	// return RCOK
}
//-----------------------------------------------------------------------------

// libstubs.cpp end
