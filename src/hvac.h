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

void HTTest();

class WSHPPERF
{
public:
	WSHPPERF();
	static float whp_CapCFromCapH(float capH, bool useRatio, float ratioCH);
	static float whp_CapHFromCapC(float capC, bool useRatio, float ratioCH);
	static void whp_ConsistentCaps(float& capC, float& capH, bool useRatio, float ratioCH);
	RC whp_HeatingFactors(float& capF, float& inpF,
		float tSource, float tdbCoilIn, float airFlowF = 1.f, float sourceFlowF = 1.f);
	RC whp_CoolingFactors(float& capF, float& capSenF, float& inpF,
		float tSource, float tdbCoilIn, float twbCoilIn, float airFlowF = 1.f, float sourceFlowF = 1.f);

private:
	float whp_heatingCapFNormF;
	float whp_heatingInpFNormF;

	float whp_coolingCapFNormF;
	float whp_coolingCapSenFNormF;
	float whp_coolingInpFNormF;


};	// class WSHPPERF

extern WSHPPERF WSHPPerf;		// public WSHPPERF object

#endif // _HVAC_H_