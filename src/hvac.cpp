// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================
#include "cnglob.h"

//-----------------------------------------------------------------------------
float CoolingSHR(		// derive cooling sensible heat ratio
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon)		// coil air flow std air cfm/ton

// return: SHR under current conditions
{
    // Correlation by A. Conant as documented in ACM
	float SHR =
		  0.0242020f * tdbCoilIn
		- 0.0592153f * twbCoilIn
		+ 0.0012651f * tdbOut
		+ 0.0016375f * vfPerTon
		// 0.f       * tdbCoilIn * tdbOut
		// 0.f       * tdbCoilIn * vfPerTon
		// 0.f       * twbCoilIn * tdbOut
		- 0.0000165f * twbCoilIn * vfPerTon
		// 0.f       * tdbOut * vfPerTon
		+ 0.0002021f * twbCoilIn * twbCoilIn
		// 0.f       / vfPerTon
		+ 1.5085285f;

	// limit result
	// 0.5f = arbitrary low limit, retains sensible capacity in unrealistic cases
	SHR = bracket(0.5f, SHR, 1.f);
	return SHR;

}		// ::CoolingSHR
//-----------------------------------------------------------------------------
float ASHPCap95FromCap47( // force ASHP htg/clg consistency (heating dominated)
    float cap47,        // 47 F net heating capacity
    bool useRatio,      // apply user-input ratio
    float ratio9547)    // user-input cap95/cap47 ratio (> 0.f)
// returns cap95 consistent with cap47
//   TODO: net or gross?
{
// note: default ratio per A. Conant
//       based on data set of split systems

    return useRatio ? cap47 * ratio9547 : max(cap47 - 180.f, 0.f) / 0.98f; 
}   // ::ASHPCap95FromCap47
//-----------------------------------------------------------------------------
float ASHPCap47FromCap95(	// force ASHP htg/clg consistency (cooling dominated)
    float cap95,	    // 95 F cooling capacity
    bool useRatio,      // apply user-input ratio
    float ratio9547)        // user-input cap95 / cap47 ratio (> 0.f)
// returns cap47 consistent with cap95
//   TODO: net or gross?
{
// note: default ratio per A. Conant
//       based on data set of split systems

    return useRatio ? cap95 / ratio9547 : 0.98f * cap95 + 180.f; 
}   // ::ASHPCap47FromCap95
//-----------------------------------------------------------------------------
void ASHPConsistentCaps(   // make air source heat pump heating/cooling capacities consistent
    float& cap95,	    // 95 F cooling capacity
    float& cap47,       // 47 F net heating capacity
    bool useRatio,      // apply user-input ratio (else default)
    float ratio9547)    // user-input cap95/cap47 ratio (> 0.f)
{
    float cap95from47 = ASHPCap95FromCap47(cap47, useRatio, ratio9547);
    if (cap95 > cap95from47)
    {
        // cooling dominated: cap95 per autosize, derive cap47 from cap95
        cap47 = ASHPCap47FromCap95(cap95, useRatio, ratio9547);
    }
    else
    {
        // heating dominated: use cap95 derived from cap47
        cap95 = cap95from47;
    }
}   // ::ASHPConsistentCaps
//-----------------------------------------------------------------------------
float WSHPCapCFromCapH( // force WSHP htg/clg consistency (heating dominated)
    float capH,         // net heating capacity
    bool useRatio,      // apply user-input ratio
    float ratioCH)      // user-input capC/capH ratio (> 0.f)
// returns capCconsistent with capH
//   TODO: net or gross?
{
    return capH * (useRatio ? ratioCH : 1.f);
}   // ::WSHPCapCFromCapH
//-----------------------------------------------------------------------------
float WSHPCapHFromCapC(	// force WSHP htg/clg consistency (cooling dominated)
    float capC,	    // cooling capacity
    bool useRatio,      // apply user-input ratio
    float ratioCH)      // user-input capC / capH ratio (> 0.f)
// returns capH consistent with capC
//   TODO: net or gross?
{
    return capC / (useRatio ? ratioCH : 1.f);
}   // ::ASHPCap47FromCap95
//-----------------------------------------------------------------------------
void WSHPConsistentCaps(   // make water source heat pump heating/cooling capacities consistent
    float& capC,	    // cooling capacity
    float& capH,        // net heating capacity
    bool useRatio,      // apply user-input ratio (else assume 1)
    float ratioCH)      // user-input capC / capH ratio (> 0.f)
{
    float capCFromCapH = WSHPCapCFromCapH(capH, useRatio, ratioCH);
    if (capC > capCFromCapH)
    {
        // cooling dominated: derive capH from capC
        capH = WSHPCapHFromCapC(capC, useRatio, ratioCH);
    }
    else
    {
        // heating dominated: use capC derived from capH
        capC = capCFromCapH;
    }
}   // ::WSHPConsistentCaps

//-----------------------------------------------------------------------------
float WSHPHeatingCapF(	// derive heating capacity factors
	float tSource,		// source fluid temperature, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float airFlowF,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*= 1.f*/)	// coil source fluid flow mass fraction

// returns cap factor = (gross heating cap current) / (gross heating cap rated)

{
	// based on EnergyPlus model using example file coefficients
	const float Tref{ 283.15f };	// 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTdbAir = DegFtoK(tdbCoilIn) / Tref;

	return -1.361311959f
		- 2.471798046f * ratioTdbAir
		+ 4.173164514f * ratioTSource
		+ 0.640757401f * airFlowF;
	// + 0.f * sourceFlowF;

}		// WSHPHeatingCapF
//-----------------------------------------------------------------------------
float WSHPHeatingInpF(	// derive heating input power factors
	float tSource,		// source fluid temperature, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float airFlowF,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*= 1.f*/)	// coil source fluid flow mass fraction

{
	// based on EnergyPlus model using example file coefficients
	const float Tref{ 283.15f };	// 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTdbAir = DegFtoK(tdbCoilIn) / Tref;

	return -2.176941116f
		+ 0.832114286f * ratioTdbAir
		+ 1.570743399f * ratioTSource
		+ 0.690793651f * airFlowF;
	// + 0.f * sourceFlowF;

}		// WSHPHeatingInpF
//-----------------------------------------------------------------------------
float WSHPCoolingCapF(	// derive WSHP cooling capacity factor
	float tSource,		// source fluid temperature, F
	float twbCoilIn,	// coil entering wet bulb, F
	float airFlowF,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*=1.f*/)	// coil source fluid flow mass fraction

// returns total cooling capacity factor =
//   (gross total cooling capacity current) / (gross total cooling capacity rated)

{
	// based on EnergyPlus model using example file coefficients
	const float Tref{ 283.15f };	// 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTwbAir = DegFtoK(twbCoilIn) / Tref;

	return -9.149069561f
		+ 10.87814026f * ratioTwbAir
		- 1.718780157f * ratioTSource
		+ 0.746414818f * airFlowF;
	// + 0.f * sourceFlowF;

}		// WSHPCoolingCapF
//-----------------------------------------------------------------------------
float WSHPCoolingCapSenF(	// derive cooling capacity factors
	float tSource,		// source fluid temperature, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float airFlowF,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*=1.f*/)	// coil source fluid flow mass fraction

// returns sensible cooling capacity factor =
//   (gross sensible cooling capacity current) / (gross sensible cooling capacity rated)

{
	// based on EnergyPlus model using example file coefficients
	const float Tref{ 283.15f }; // 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTdbAir = DegFtoK(tdbCoilIn) / Tref;
	float ratioTwbAir = DegFtoK(twbCoilIn) / Tref;

	return -5.462690012f
		+ 17.95968138f * ratioTdbAir
		- 11.87818402f * ratioTwbAir
		- 0.980163419f * ratioTSource
		+ 0.767285761f * airFlowF;
	// + 0.f * sourceFlowF;

}		// WSHPCoolingCapSenF
//-----------------------------------------------------------------------------
float WSHPCoolingInpF(	// derive WSHP cooling input power factors
	float tSource,		// source fluid temperature, F
	float twbCoilIn,	// coil entering wet bulb, F
	float airFlowF,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*=1.f*/)	// coil source fluid flow mass fraction

// returns cooling input factor =
//   (input power current) / (input power rated)

{
	// based on EnergyPlus model using example file coefficients
	const float Tref{ 283.15f }; // 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTwbAir = DegFtoK(twbCoilIn) / Tref;

	return -3.20456384f
		- 0.976409399f * ratioTwbAir
		+ 3.97892546f * ratioTSource
		+ 0.938181818f * airFlowF;
	// + 0.f * sourceFlowF;

}		// WSHPCoolingInpF


// hvac.cpp end
