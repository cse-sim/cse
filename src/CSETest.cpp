// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// CSETest.cpp : Test application, calls CSE.DLL
//

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <stdlib.h>

#include "cseface.h"

/////////////////////////////////////////////////////////////////
static int CSEMsgCallBack(		// callback, receives all CSE screen text
	int level,
	const char* msg)
{
	printf( msg);
	int ret = CSE_CONTINUE;
#if 0
	// test for abort capability
	static int callCount = 0;
	callCount++;
	if (callCount == 18)
		ret = CSE_ABORT;
#endif
	return ret;
}
//--------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	char exePath[ _MAX_PATH];
	GetModuleFileNameA( NULL, exePath, sizeof( exePath));

	char buf[ 100];
	int nchars = CSEProgInfo( buf, sizeof( buf));

	int ret = 0;

#if 1
	for (int iRun=0; iRun<20; iRun++)
	{
		ret = CSEZ( exePath, "-b \"2StoryExample4_00 - Std\"", CSEMsgCallBack);

		ret = CSEZ( exePath, "-b \"2StoryExample4_00 - Prop\"", CSEMsgCallBack);
	}

#else
	ret = CSEZ( exePath, "-b \"1z4s\"", CSEMsgCallBack);

	ret = CSEZ( exePath, "-b \"autosize\"", CSEMsgCallBack);

	ret = CSEZ( exePath, "-b \"1z4s\"", CSEMsgCallBack);
#endif


	return ret;
}
//=============================================================================

// csetest.cpp end


