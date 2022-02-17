// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// csevrsn.h: specify current build CSE version number
//    Used by cse.cpp and cse.rc

// convert #defined value to string literal
//   #define NAME BOB
//   MAKE_LIT( NAME) -> "BOB"
#define MAKE_LIT2(s) #s
#define MAKE_LIT(s) MAKE_LIT2(s)

// version # for current build (derived from git repo tags)
#define CSEVRSN_MAJOR 0
#define CSEVRSN_MINOR 901
#define CSEVRSN_PATCH 0

#define CSEVRSN_META "+clean-lvl4-warnings.cb51d86d.46"

// version # as quoted text
#define CSEVRSN_TEXT MAKE_LIT(CSEVRSN_MAJOR##.##CSEVRSN_MINOR##.##CSEVRSN_PATCH) CSEVRSN_META

// csevrsn.h end
