// Copyright (c) 1997-2018 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// foundation.h -- interface to Kiva
///////////////////////////////////////////////////////////////////////////////

#if !defined( _FOUNDATION_H)
#define _FOUNDATION_H

#include "Instance.hpp"
#include "Aggregator.hpp"

inline Kiva::Material kivaMat(float k, float rho, float cp) {
	if (rho == 0.f || cp == 0.f)
	{
		err("Materials used in Kiva must have specific heat and density greater than zero.");  // TODO KIVA improve?
	}
	return Kiva::Material(KIPtoSI(k), DIPtoSI(rho), SHIPtoSI(cp));
}

struct KivaWallGroup {
	KivaWallGroup() : perimeter(0.0) {};
	KivaWallGroup(double perimeter, std::vector<TI> wallIDs) : perimeter(perimeter), wallIDs(wallIDs) {};
	double perimeter;
	std::vector<TI> wallIDs;
};

#endif	// _FOUNDATION_H
