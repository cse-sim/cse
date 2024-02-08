// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================
#include "cnglob.h"
#include "ancrec.h"
#include "rccn.h"
#include <btwxt/btwxt.h>
#include "hvac.h"

//=============================================================================
void BXMSGHAN::BxHandleExceptions() // Common code for handling Btwxt exceptions
// Uses the "Lipincott function" pattern
// Call from within catch (...)
{
	try
	{
		throw;		// re-throw exception in flight
	}
	catch (const std::runtime_error& /*e*/)
	{
		err(ABT, "Fatal Btwxt exception");

	}
}		// BXMSGHAN::BxHandleExceptions
//============================================================================

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
// Harvest Thermal CHDHW (Combined Heat / DHW) routines
// Data + class CHDHW
///////////////////////////////////////////////////////////////////////////////
CHDHW::CHDHW()
	: hvt_capHtgNetMin( 0.f), hvt_capHtgNetMaxFT( 0.f), hvt_tRiseMax( 0.f)
{}
//-----------------------------------------------------------------------------
CHDHW::~CHDHW()
{
}
//-----------------------------------------------------------------------------
void CHDHW::hvt_Clear()	// clear all non-static members
{
	hvt_capHtgNetMin = 0.f;
	hvt_capHtgNetMaxFT = 0.f;
	hvt_tRiseMax = 0.f;

	hvt_pAVFPwrRGI.reset(nullptr);
	hvt_pWVFRGI.reset(nullptr);
	hvt_pCapMaxRGI.reset(nullptr);

}	// CHDHW::hvt_Clear
//-----------------------------------------------------------------------------
static void CHDHW_RGICallback(		// btwxt message dispatcher
	void* pContext,			// pointer to specific RSYS
	BXMSGHAN::BXMSGTY msgTy,	// message type: bsxmsgERROR etc
	const std::string& message)	// message text
{
	CHDHW* pCHDHW = reinterpret_cast<CHDHW*>(pContext);

	pCHDHW->hvt_ReceiveBtwxtMessage(msgTy, message);

}		// CHDHW_RGICallBack
//-----------------------------------------------------------------------------
void CHDHW::hvt_ReceiveBtwxtMessage(
	BXMSGHAN::BXMSGTY msgTy,	// message type: bxmsgERROR etc
	const std::string& message)			// message text
{
	// add prefix with tag
	const char* msgx = strtprintf("btwxt -- %s", message.c_str());

	switch (msgTy)
	{
	case BXMSGHAN::bxmsgERROR:
		err(msgx);
		break;
	case BXMSGHAN::bxmsgWARNING:
		warn(msgx);
		break;
	default:
		info(msgx);
	}

#if 0
	auto msgFunc = (msgTy == BXMSGHAN::bxmsgERROR)   ? RC (*err)( const char*, ...)
				 : (msgTy == BXMSGHAN::bxmsgWARNING) ? warn
		         :                                        info;

	std::invoke(msgFunc, this, finalMsg);
#endif

}		// CHDHW::cvt_ReceiveBtwxtMessage
//-----------------------------------------------------------------------------
RC CHDHW::hvt_Init(		// one-time init
	float blowerEfficacy)		// full speed operating blower efficacy, W/cfm
// returns RCOK iff success
{
	RC rc = RCOK;
	hvt_Clear();

	try
	{

		// derive running fan power
		double ratedBlowerEfficacy = hvt_GetRatedBlowerEfficacy();
		double blowerPwrF = blowerEfficacy / ratedBlowerEfficacy;	// blower power factor
		// nominal full speed blower power = 0.2733 W/cfm at full speed

// derive points adjusted for blower power
		std::vector< double> netCaps;		// net capacities (coil + blower), Btuh
		std::vector< double> blowerPwrOpr;	// operating blower power, W

		assert(hvt_blowerPwr.size() == hvt_grossCaps.size());

		auto bpIt = hvt_blowerPwr.begin();		// iterate blower power in parallel
		for (double& grossCap : hvt_grossCaps)
		{
			double bp = *bpIt++;	// rated blower power, W
			double bpX = bp * blowerPwrF;	// blower power at operating static, W
			blowerPwrOpr.push_back(bpX);
			double netCap = grossCap + bpX * BtuperWh;
			netCaps.push_back(netCap);
		}

		std::vector< std::vector< double>> netCapAxis{ netCaps };

		// lookup vars: avf and power for each net capacity
		std::vector< std::vector<double>> avfPwrData{ hvt_AVF, blowerPwrOpr };

		hvt_pAVFPwrRGI.reset(new RGI(netCapAxis, avfPwrData, "AVF/power",
			std::make_shared< BXMSGHAN>(CHDHW_RGICallback, this)));

		// water flow grid variables: entering water temp, net capacity
		std::vector< std::vector< double>> htMap{ hvt_tCoilEW, netCaps };

		hvt_pWVFRGI.reset(new RGI(htMap, hvt_WVF, "Water flow",
			std::make_shared< BXMSGHAN>(CHDHW_RGICallback, this)));

		// min/max capacities
		hvt_capHtgNetMin = netCaps[0];	// min is independent of ewt
		hvt_capHtgNetMaxFT = netCaps.back();
		std::vector< double> capHtgNetMax(hvt_tCoilEW.size(), hvt_capHtgNetMaxFT);
		capHtgNetMax[0] = netCaps[netCaps.size() - 2];

		std::vector< std::vector< double>> capHtgNetMaxLU{ capHtgNetMax };
		std::vector< std::vector< double>> ewtAxis{ hvt_tCoilEW };
		hvt_pCapMaxRGI.reset(new RGI(ewtAxis, capHtgNetMaxLU, "Min/max capacities",
			std::make_shared< BXMSGHAN>(CHDHW_RGICallback, this)));

		float avfMax = hvt_AVF.back();
		float amfMax = AVFtoAMF(avfMax);	// elevation?
		hvt_tRiseMax = hvt_capHtgNetMaxFT / (amfMax * Top.tp_airSH);
	}
	catch (...)
	{
		BXMSGHAN::BxHandleExceptions();
	}

	return rc;
}		// CHDHW::hvt_Init
//-----------------------------------------------------------------------------
float CHDHW::hvt_GetTRise(
	float tCoilEW /*=-1.f*/) const
{
	// if (tCoilEW < 0.f)
		return hvt_tRiseMax;

	// TODO 

}		// CHDHW::hvt_GetTRise
//-----------------------------------------------------------------------------
float CHDHW::hvt_GetRatedCap(		// full-speed net heating capacity
	float tCoilEW /*=-1.f*/) const
{
	// if (tCoilEW < 0.f)
	return hvt_capHtgNetMaxFT;
}	// CHDHW::hvt_GetRatedCap
//-----------------------------------------------------------------------------
double CHDHW::hvt_GetRatedBlowerAVF() const
{
	return hvt_AVF.back();
}		// CHDHW::hvt_GetRatedBlowerAVF
//-----------------------------------------------------------------------------
double CHDHW::hvt_GetRatedBlowerEfficacy() const	// rated blower efficacy
// returns rated blower power, W/cfm
{
	return hvt_blowerPwr.back() / hvt_GetRatedBlowerAVF();

}	// CHDHW::hvt_GetRatedBlowerEfficacy
//-----------------------------------------------------------------------------
void CHDHW::hvt_CapHtgMinMax(	// min/max available net heating capacity
	float tCoilEW,				// coil entering water temp, F
	float& capHtgNetMin,		// returned: min speed net heating cap, Btuh
	float& capHtgNetMax) const	// returned: max speed net heating cap, Btuh
{
	std::vector< double> tCoilEWTarg{ tCoilEW };

	std::vector< double> qhMax = hvt_pCapMaxRGI->get_values_at_target(tCoilEWTarg);

	capHtgNetMin = hvt_capHtgNetMin;		// min cap does not depend on tCoilEW
	capHtgNetMax = qhMax[0];

}		// CHDHW::hvt_CapHtgMax
//-----------------------------------------------------------------------------
float CHDHW::hvt_WaterVolFlow(
	float qhNet,	// net heating output, Btuh
	float tCoilEW)	// coil entering water temp, F
// returns required volume flow, gpm
{

	std::vector< double> wvfTarg{ tCoilEW, qhNet };

	std::vector< double> wvf = hvt_pWVFRGI->get_values_at_target(wvfTarg);

	return wvf[0];

}		// CHDHW::hvt_WaterVolFlow
//-----------------------------------------------------------------------------
void CHDHW::hvt_BlowerAVFandPower(
	float qhNet,	// net heating output, Btuh
	float& avf,		// returned: blower volumetric air flow, cfm
	float& pwr)		// returned: blower power, W
{
	std::vector< double> qOutTarg{ qhNet };

	std::vector< double> res = hvt_pAVFPwrRGI->get_values_at_target(qOutTarg);

	avf = res[0];
	pwr = res[1];

}	// CHDHW::hvt_BlowerAVFandPower
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
