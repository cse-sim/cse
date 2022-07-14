// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================

#if !defined( _HVAC_H_)
#define	_HVAC_H_

float CoolingSHR( float tdbOut,	float tdbCoilIn, float twbCoilIn, float vfPerTon);

float ASHPCap95FromCap47( float cap47, bool useRatio, float ratio9547);     
float ASHPCap47FromCap95( float cap95, bool useRatio, float ratio9547);     
void ASHPConsistentCaps( float& cap95, float& cap47, bool useRatio, float ratio9547);     

float WSHPCapCFromCapH(float capH, bool useRatio, float ratioCH);
float WSHPCapHFromCapC(float capC, bool useRatio, float ratioCH);
void WSHPConsistentCaps(float& capC, float& capH, bool useRatio, float ratioCH);

float WSHPHeatingCapF(float tSource, float tdbCoilIn, float airFlowF, float sourceFlowF = 1.f);
float WSHPHeatingInpF(float tSource, float tdbCoilIn, float airFlowF, float sourceFlowF = 1.f);

float WSHPCoolingCapF(float tSource, float twbCoilIn, float airFlowF, float sourceFlowF = 1.f);
float WSHPCoolingCapSenF(float tSource, float tdbCoilIn, float twbCoilIn, float airFlowF, float sourceFlowF = 1.f);
float WSHPCoolingInpF(float tSource, float twbCoilIn, float airFlowF, float sourceFlowF = 1.f);

#endif // _HVAC_H_