// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================
#include "cnglob.h"

#include <cmath>
#include <btwxt.h>
#include "hvac.h"

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
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// Harvest Thermal Combined Heat / DHW routines
// Data + class HARVTHERM
///////////////////////////////////////////////////////////////////////////////
#if 0
static std::vector< double> caps{ 6000., 12000., 18000., 24000., 30000., 36000. };
static std::vector< std::vector< double>> capsAxis{ caps };

static std::vector< double> avf{ 400., 600., 750., 900., 1050., 1200. };
static std::vector< double> fanPwr{ 29., 60., 96., 149., 216., 328. };
// static std::vector< std::vector<double>> avfPwrData{ avf, fanPwr };
// static Btwxt::RegularGridInterpolator avfPwr(capsAxis, avfPwrData);


static std::vector< double> ewts = { 120., 130., 140., 150. };
// static std::vector< std::vector< double>> ewtAxis{ ewts };
static std::vector< std::vector< double>> htMap{ ewts, caps };
static std::vector< double> vals{
0.27, 0.56, 0.87, 1.22, 1.59, 1.57,
0.22, 0.45, 0.70, 0.87, 1.26, 1.57,
0.18, 0.38, 0.59, 0.81, 1.04, 1.29,
0.16, 0.33, 0.50, 0.69, 0.89, 1.09
};
static std::vector< std::vector<double>> waterFlowData{ vals };

static Btwxt::RegularGridInterpolator waterFlows(htMap, waterFlowData);

#endif


#if 0
float HTAirFlowForOutput(
	float qOut)
{
	std::vector< double> qOutTarg{ qOut };

	std::vector< double> res = avfPwr.get_values_at_target(qOutTarg);

	std::vector< double> wfTarg{ 135., qOut };

	std::vector< double> wf = waterFlows.get_values_at_target(wfTarg);

	return avf;
}	// HTAirFlow
//-----------------------------------------------------------------------------
void HTTest()
{

	HTAirFlowForOutput(35999.f);
	HTAirFlowForOutput(36000.f);
	HTAirFlowForOutput(36001.f);

	for (int i = -5000; i < 50000; i += 6000)
	{
		float Q = float(i);
		HTAirFlowForOutput(Q);
	}
}
#endif

HARVTHERM::HARVTHERM()
	: hvt_capHtgNetMin( 0.f)
{}
//-----------------------------------------------------------------------------
HARVTHERM::~HARVTHERM()
{
}
//-----------------------------------------------------------------------------
void HARVTHERM::hvt_Clear()	// clear all non-static members
{

	hvt_capHtgNetMin = 0.f;

	hvt_pAVFPwrRGI.reset(nullptr);
	hvt_pWVFRGI.reset(nullptr);
	hvt_pCapMaxRGI.reset(nullptr);

}	// HARVTHERM::hvt_Clear
//-----------------------------------------------------------------------------
RC HARVTHERM::hvt_Init(		// one-time init
	float blowerPwr)		// full speed operating blower power, W/cfm

{
	RC rc = RCOK;
	hvt_Clear();

	// derive running fan power
	float blowerPwrF = blowerPwr / 0.27f;	// blower power factor
						// nominal full speed blower power = 0.27 W/cfm at full speed

	// derive points adjusted for blower power
	std::vector< double> netCaps;		// net capacities (coil + blower), Btuh
	std::vector< double> blowerPwrOpr;	// operating blower power, W

	assert(hvt_blowerPwr.size() == hvt_grossCaps.size());

	auto bpIt = hvt_blowerPwr.begin();		// iterate blower power in parallel
	for (double& grossCap : hvt_grossCaps)
	{
		double bpX = *bpIt++ * blowerPwrF;
		blowerPwrOpr.push_back(bpX);
		double netCap = grossCap + bpX;
		netCaps.push_back( netCap);

	}
	hvt_netCapAxis.push_back(netCaps);

	// lookup vars: avf and power for each net capacity
	hvt_avfPwrData.insert(hvt_avfPwrData.begin(), { hvt_avf, blowerPwrOpr });

	hvt_pAVFPwrRGI.reset(new RGI(hvt_netCapAxis, hvt_avfPwrData));

	// water flow grid variables: entering water temp, net capacity
	std::vector< std::vector< double>> htMap{ hvt_ewts, netCaps };

	hvt_pWVFRGI.reset(new RGI(htMap, hvt_wvf));

	// min/max capacities
	hvt_capHtgNetMin = netCaps[0];	// min is independent of ewt
	std::vector< double> capHtgNetMax(hvt_ewts.size(), netCaps.back());
	capHtgNetMax[ 0] = netCaps[netCaps.size() - 2];

	std::vector< std::vector< double>> capHtgNetMaxLU{ capHtgNetMax };

	std::vector< std::vector< double>> ewtAxis{ hvt_ewts };

	hvt_pCapMaxRGI.reset(new RGI(ewtAxis, capHtgNetMaxLU));

	return rc;
}		// HARVTHERM::hvt_Init

#if 1
//-----------------------------------------------------------------------------
std::pair< float, float> HARVTHERM::hvt_CapHtgMinMax(	// min/max available net heating capacity
	float tWIn) const				// coil entering water temp, F
// returns [minHtgCap, maxHtgCap], Btuh
{
	std::vector< double> ewtTarg{ tWIn };

	std::vector< double> qhMax = hvt_pCapMaxRGI->get_values_at_target(ewtTarg);

	// min cap does not depend on ewt
	return std::make_pair(
		hvt_capHtgNetMin,		// min cap does not vary with tWIn (aka ewt)
		float( qhMax[ 0]));		// max cap is limited when tWIn < 130.f
}		// HARVTHERM::hvt_CapHtgMax
#else
//-----------------------------------------------------------------------------
float HARVTHERM::hvt_CapHtgMaxMin(	// max and min available heating capacity
	float tWIn,				// entering water temp, F
	float& qNetMin) const	// returned: minimum available net heating capacity
// returns max available net heating capacity, Btuh
//         qNetMin set
{
	float qhMax = tWIn < 130.f ? 30000.f : 36000.f;
	return qhMax;

}		// HARVTHERM::hvt_CapHtgMax
#endif
//-----------------------------------------------------------------------------
float HARVTHERM::hvt_WaterVolFlow(
	float qh,		// gross heating output, Btuh
					//   = coil output
	float tWIn)		// coil entering water temp, F
// returns required volume flow, gpm
{

	std::vector< double> wfTarg{ tWIn, qh };

	std::vector< double> wf = hvt_pWVFRGI->get_values_at_target(wfTarg);

	return wf[0];

}		// HARVTHERM::hvt_WaterVolFlow
//-----------------------------------------------------------------------------
void HARVTHERM::hvt_BlowerAVFandPower(
	float qh,	// gross heating output, Btuh
				//  = coil output
	float& avf,		// returned: fan volumetric air flow, cfm
	float& pwr)		// returned: fan power (W)
{
	std::vector< double> qOutTarg{ qh };

	std::vector< double> res = hvt_pAVFPwrRGI->get_values_at_target(qOutTarg);

	avf = res[0];
	pwr = res[1];

}	// HARVTHERM::hvt_BlowerAVFandPower
///////////////////////////////////////////////////////////////////////////////
    

///////////////////////////////////////////////////////////////////////////////
// class WSHPPERF: Water source heat pump info and data
///////////////////////////////////////////////////////////////////////////////
WSHPPERF WSHPPerf;		// public WSHPPERF object
//-----------------------------------------------------------------------------
WSHPPERF::WSHPPERF()
	: whp_heatingCapFNormF(1.f), whp_heatingInpFNormF( 1.f),
	whp_coolingCapFNormF(1.f), whp_coolingCapSenFNormF(1.f), whp_coolingInpFNormF(1.f)

{
#define WSHPNORMALIZE

	// derive normalization factors at rating conditions
	// normalization factors are initialized to 1
	// then set based on results of calc at rating temps
	// rating temps assumed to be ASHRAE/ISO 13256 WLHP values

	float capF, inpF;
	whp_HeatingFactors(capF, inpF, 68.f, 68.f);
#if defined( WSHPNORMALIZE)
	whp_heatingCapFNormF = 1.f / capF;
	whp_heatingInpFNormF = 1.f / inpF;
#endif

	float capSenF;
	whp_CoolingFactors(capF, capSenF, inpF, 86.f, 80.6f, 66.2f);
#if defined( WSHPNORMALIZE)
	whp_coolingCapFNormF = 1.f / capF;
	whp_coolingCapSenFNormF = 1.f / capSenF;
	whp_coolingInpFNormF = 1.f / inpF;
#endif

}	// WSHPPERF::WSHPPERF()
//-----------------------------------------------------------------------------
float WSHPPERF::whp_CapCFromCapH( // force WSHP htg/clg consistency (heating dominated)
	float capH,         // net heating capacity
	bool useRatio,      // apply user-input ratio
	float ratioCH)      // user-input capC/capH ratio (> 0.f)
// returns capCconsistent with capH
//   TODO: net or gross?
{
	return capH * (useRatio ? ratioCH : 0.8f);
}   // WSHPPERF::whp_CapCFromCapH
//-----------------------------------------------------------------------------
float WSHPPERF::whp_CapHFromCapC(	// force WSHP htg/clg consistency (cooling dominated)
	float capC,	    // cooling capacity
	bool useRatio,      // apply user-input ratio
	float ratioCH)      // user-input capC / capH ratio (> 0.f)
// returns capH consistent with capC
//   TODO: net or gross?
{
	return capC / (useRatio ? ratioCH : 0.8f);
}   // WSHPPERF::whp_CapHFromCapC
//-----------------------------------------------------------------------------
void WSHPPERF::whp_ConsistentCaps(   // WSHP heating/cooling capacity consistency
	float& capC,	    // cooling capacity
	float& capH,        // net heating capacity
	bool useRatio,      // apply user-input ratio (else assume 1)
	float ratioCH)      // user-input capC / capH ratio (> 0.f)
{
	float capCFromCapH = whp_CapCFromCapH(capH, useRatio, ratioCH);
	if (capC > capCFromCapH)
	{
		// cooling dominated: derive capH from capC
		capH = whp_CapHFromCapC(capC, useRatio, ratioCH);
	}
	else
	{
		// heating dominated: use capC derived from capH
		capC = capCFromCapH;
	}
}   // WSHPPERF::WSHPConsistentCaps
//-----------------------------------------------------------------------------
RC WSHPPERF::whp_HeatingFactors(	// derive heating factors
	float& capF,		// capacity factor
	float& inpF,		// input factor
	float tSource,		// source fluid temperature, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float airFlowF /*=1.f*/,		// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*= 1.f*/)	// coil source fluid flow mass fraction

// based on EnergyPlus model using example file coefficients
// calcs capF = (gross heating cap current) / (gross heating cap rated)
//       inpF = (gross input power current) / (gross input power rated)

// returns RCOK (no error conditions defined)

{
	RC rc = RCOK;

	constexpr float Tref{ 283.15f };	// 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTdbAir = DegFtoK(tdbCoilIn) / Tref;

	capF = -1.361311959f
		- 2.471798046f * ratioTdbAir
		+ 4.173164514f * ratioTSource
		+ 0.640757401f * airFlowF;
	// + 0.f * sourceFlowF;

	capF *= whp_heatingCapFNormF;

	inpF = -2.176941116f
		+ 0.832114286f * ratioTdbAir
		+ 1.570743399f * ratioTSource
		+ 0.690793651f * airFlowF;
	// + 0.f * sourceFlowF;

	inpF *= whp_heatingInpFNormF;

	return rc;

}		// WSHPPERF::whp_HeatingFactors
//-----------------------------------------------------------------------------
RC WSHPPERF::whp_CoolingFactors(	// derive WSHP cooling capacity factor
	float& capF,		// total capacity factor
	float& capSenF,		// sensible capacity factor
	float& inpF,		// input factor
	float tSource,		// source fluid temperature, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float airFlowF /*=1.F*/,	// coil air flow mass fraction
	[[maybe_unused]] float sourceFlowF /*=1.f*/)	// coil source fluid flow mass fraction

// based on EnergyPlus model using example file coefficients

// calcs capF =
//   (gross total cooling capacity current) / (gross total cooling capacity rated)
// capSenF =
//   (gross sensible cooling capacity current) / (gross sensible cooling capacity rated)
// inpF =
//   (input power current) / (input power rated)

// returns RCOK (no error conditions defined)

{
	RC rc = RCOK;

	const float Tref{ 283.15f };	// 10 C = 50 F
	float ratioTSource = DegFtoK(tSource) / Tref;
	float ratioTdbAir = DegFtoK(tdbCoilIn) / Tref;
	float ratioTwbAir = DegFtoK(twbCoilIn) / Tref;

	capF = -9.149069561f
		+ 10.87814026f * ratioTwbAir
		- 1.718780157f * ratioTSource
		+ 0.746414818f * airFlowF;
	// + 0.f * sourceFlowF;
	capF *= whp_coolingCapFNormF;

	capSenF = -5.462690012f
		+ 17.95968138f * ratioTdbAir
		- 11.87818402f * ratioTwbAir
		- 0.980163419f * ratioTSource
		+ 0.767285761f * airFlowF;
	// + 0.f * sourceFlowF;
	capSenF *= whp_coolingCapSenFNormF;

	inpF = -3.20456384f
		- 0.976409399f * ratioTwbAir
		+ 3.97892546f * ratioTSource
		+ 0.938181818f * airFlowF;
	// + 0.f * sourceFlowF;
	inpF *= whp_coolingInpFNormF;

	return rc;
}		// WSHPPERF::whp_CoolingFactors
//=============================================================================

// hvac.cpp end
