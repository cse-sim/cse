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
#ifdef GROUND_PLOT
#include <GroundPlot.hpp>
#include <fmt/format.h>
#endif


class KIVA
{ public:
	KIVA(int32_t floor_index, float foundation_depth, int32_t wall_construction_index, float weight) :
	kv_walls({}), kv_floor(floor_index), kv_depth(foundation_depth), kv_wallCon(wall_construction_index), kv_perimWeight(weight)
	{}
	
	ZNR* kv_GetZone() const;
	RC kv_Create();
	RC kv_RddInit();
	RC kv_SetInitBCs(DOY jDay);
	RC kv_SetBCs();
	RC kv_Step(float dur);
	Kiva::Instance kv_instance;
	std::vector<TI> kv_walls; // list of wall references
	// Kiva instances are unique to each combination of:
	//  1. Floor surface
	//  2. Foundation depth
	//  3. Wall construction (if foundation depth > 0)
	int32_t kv_floor;			// floor SFI (not XSRAT!) reference
	float kv_depth;			// foundation depth
	int32_t kv_wallCon;			// wall construction reference (or -1 if depth == 0)
	float kv_perimWeight;
#ifdef GROUND_PLOT
	Kiva::SnapshotSettings kv_plotSettings;
	Kiva::GroundPlot kv_groundPlot;
#endif

};		// KIVA

extern std::vector<KIVA> kivas;

inline Kiva::Material kivaMat(float k, float rho, float cp) {
	if (rho == 0.f || cp == 0.f)
	{
		err( ERR, "Materials used in Kiva must have specific heat and density greater than zero.");  // TODO KIVA improve?
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
