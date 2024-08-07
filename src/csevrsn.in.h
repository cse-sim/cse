// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// csevrsn.h: specify current build CSE version number
//    Used by cse.cpp and cse.rc

#ifndef MAKE_LITERAL
// convert #defined value to string literal
//   #define NAME BOB
//   MAKE_LITERAL( NAME) -> "BOB"
#define MAKE_LITERAL2(s) #s
#define MAKE_LITERAL(s) MAKE_LITERAL2(s)
#endif

// version # for current build (derived from git repo tags)
#define CSEVRSN_MAJOR @CSEVRSN_MAJOR@
#define CSEVRSN_MINOR @CSEVRSN_MINOR@
#define CSEVRSN_PATCH @CSEVRSN_PATCH@

#define CSEVRSN_META "@CSEVRSN_META@"

// version # as quoted text
#define CSEVRSN_TEXT MAKE_LITERAL(CSEVRSN_MAJOR.CSEVRSN_MINOR.CSEVRSN_PATCH@CSEVRSN_PRERELEASE@) CSEVRSN_META

// csevrsn.h end
