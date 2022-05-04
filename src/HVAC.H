// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================

#if !defined( _HVAC_H_)
#define	_HVAC_H_


float CoolingSHR(		// derive cooling sensible heat ratio
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon);		// coil air flow std air cfm/ton



#endif // _HVAC_H_