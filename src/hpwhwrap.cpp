// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// hpwhwrap.cpp -- wrapper for Ecotope hpwh.cc
// WHY
//   includes cnglob.h re global definitions
//   allows use of precompiled headers
//   allows CSE-specific #defines

#include "cnglob.h"

#define HPWH_ABRIDGED	// exclude some features not used by CSE
#include "hpwh/hpwh.cc"		// Ecotope source file

// end hpwhwrap.cpp