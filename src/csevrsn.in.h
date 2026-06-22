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
#define CSEVRSN_MAJOR @CSE_version_major@
#define CSEVRSN_MINOR @CSE_version_minor@
#define CSEVRSN_PATCH @CSE_version_patch@

#define CSEVRSN_META "@CSE_version_meta@"

// version # as quoted text
#define CSEVRSN_TEXT MAKE_LITERAL(CSEVRSN_MAJOR.CSEVRSN_MINOR.CSEVRSN_PATCH@CSE_version_prerelease@) CSEVRSN_META

// csevrsn.h end
