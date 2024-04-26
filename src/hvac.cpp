// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================
#include "cnglob.h"
#include "ancrec.h"
#include "rccn.h"
#include "irats.h"
#include "vecpak.h"
#include <btwxt/btwxt.h>
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
// Harvest Thermal CHDHW (Combined Heat / DHW) routines
// Data + class CHDHW
///////////////////////////////////////////////////////////////////////////////
CHDHW::CHDHW( record* pParent)
	: hvt_capHtgNetMin( 0.f), hvt_capHtgNetMaxFT( 0.f), hvt_tRiseMax( 0.f),
	  hvt_pParent( pParent)
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
	MSGTY msgTy,		// message type: bsxmsgERROR etc
	const char* msg)	// message text
{
	CHDHW* pCHDHW = reinterpret_cast<CHDHW*>(pContext);

	record* pParent = pCHDHW->hvt_pParent;
	const char* msgx = strtprintf("CHDHW: %s", msg);
	pParent->ReceiveMessage(msgTy, msgx);

}		// CHDHW_RGICallBack
//-----------------------------------------------------------------------------
RC CHDHW::hvt_Init(		// one-time init
	float blowerEfficacy)		// full speed operating blower efficacy, W/cfm
// returns RCOK iff success
{
	using namespace Btwxt;
	using GridAxes = std::vector< GridAxis>;
	using VVD = std::vector< std::vector< double>>;

	RC rc = RCOK;
	hvt_Clear();

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

	// message handler
	auto cmhCHDHW = std::make_shared< CourierMsgHandler>(CHDHW_RGICallback, this);

	// 1D grid: capacity
	GridAxis netCapAxis(netCaps,
		InterpolationMethod::linear, ExtrapolationMethod::constant,
		{ 0., DBL_MAX }, "Net cap", cmhCHDHW);

	hvt_pAVFPwrRGI.reset(new RGI(
		GridAxes{ netCapAxis},
		VVD{ hvt_AVF, blowerPwrOpr },	// lookup vars: avf and power for each net capacity
		"AVF/power", cmhCHDHW));

	GridAxis ewtAxis(hvt_tCoilEW,
		InterpolationMethod::linear, ExtrapolationMethod::constant,
		{ 0., DBL_MAX }, "EWT", cmhCHDHW);

	hvt_pWVFRGI.reset(new RGI( GridAxes{ ewtAxis, netCapAxis},
		hvt_WVF, "Water flow", cmhCHDHW));

	// min/max capacities
	hvt_capHtgNetMin = netCaps[0];	// min is independent of ewt
	hvt_capHtgNetMaxFT = netCaps.back();
	std::vector< double> capHtgNetMax(hvt_tCoilEW.size(), hvt_capHtgNetMaxFT);
	capHtgNetMax[0] = netCaps[netCaps.size() - 2];

	hvt_pCapMaxRGI.reset(new RGI(
		GridAxes{ ewtAxis},
		VVD{ capHtgNetMax },
		"Min/max capacities", cmhCHDHW));

	float avfMax = hvt_AVF.back();
	float amfMax = AVFtoAMF(avfMax);	// elevation?
	hvt_tRiseMax = hvt_capHtgNetMaxFT / (amfMax * Top.tp_airSH);

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

///////////////////////////////////////////////////////////////////////////////
// class PMACCESS: mixin interface / supports runtime access to perf map
//                 retains pointer to Btwxt interpolator
// class PERFORMANCEMAP: input representation of performance map
//      class PMGRIDAXIS:  perfmap grid values (independent vars, e.g. DBT, speed, )
//      class PMLOOKUPDATA:  perfmap performance data (e.g. capacity, power, )
///////////////////////////////////////////////////////////////////////////////
/*virtual*/ PMACCESS::~PMACCESS()
{
	pa_Clear();
}	// PMACCESS::~PMACCESS()
//------------------------------------------------------------------------------
void PMACCESS::pa_Clear()
{
	delete pa_pRGI;
	memset(this, 0, sizeof(*this));
}	// PMACCESS::pa_Clear
//------------------------------------------------------------------------------
RC PMACCESS::pa_Init(		// input -> Btwxt conversion
	const PERFORMANCEMAP* pPM,	// source PERFORMANCEMAP
	record* pParent,			// parent (e.g. RSYS)
	const char* tag,		// identifying text for this interpolator (for messages)
	float capRef)			// reference capacity (e.g. cap47 or cap95)
{
	RC rc = RCOK;

	delete pa_pRGI;		// insurance

	pa_pPERFORMANCEMAP = pPM;	// source performance map

	rc = pPM->pm_SetupBtwxt(pParent, tag, pa_pRGI, capRef, pa_speedFRating);

	if (!rc)
	{
		pa_targetDim = static_cast<int>(pa_pRGI->get_number_of_dimensions());
		pa_resultDim = pa_pRGI->get_number_of_grid_point_data_sets();

		pa_speedFMin = pa_pRGI->get_grid_axis(1).get_values()[0];

#if 0
		vTarget.resize(pa_targetDim);
		vResult.resize(pa_resultDim);
#endif
	}

	return rc;

}	// PMACCESS::pa_Init
//-----------------------------------------------------------------------------
RC PMACCESS::pa_GetCapInp(float dbtOut, float speedF, float& cap, float& inp)
{
	RC rc = RCOK;
	std::vector< double> vTarget(pa_targetDim);
	vTarget.data()[0] = dbtOut;
	vTarget.data()[1] = speedF;

	std::vector< double> vResult(2);

	vResult = (*pa_pRGI)(vTarget);

	cap = vResult.data()[0];
	inp = vResult.data()[1];

	return rc;
}	// PMACCESS::pa_GetCapInp
//-----------------------------------------------------------------------------
RC PMACCESS::pa_GetRatedCapCOP(		// derive ratings from performance map
	float dbtOut,	// outdoor temp of rating, F (47, 95, )
	float& cap,		// return: capacity, Btuh
	float& COP)		// return: COP
// returns RCOK iff success
{
	RC rc = RCOK;

	float inp;
	rc = pa_GetCapInp(dbtOut, pa_speedFRating, cap, inp);

	COP = inp > 0.f ? abs( cap) / inp : 0.f;

	return rc;
}		// PMACCESS::pa_GetRatedCapCOP
//=============================================================================
RC PERFORMANCEMAP::pm_CkF()
{
	RC rc = RCOK;

	// verify presence of expected number and type of subrecords
	switch (pm_type)
	{
		case C_PERFMAPTY_HTGCAPRATCOP:
		case C_PERFMAPTY_CLGCAPRATCOP:
		{
			const PMGRIDAXIS* gxSink;
			rc |= pm_GetGridAxis(C_PMGXTY_OUTDOORDBT, gxSink);
			rc |= pm_GetGridAxis(C_PMGXTY_SPEED, gxSink);

			const PMLOOKUPDATA* luSink;
			rc |= pm_GetLookupData(C_PMLUTY_CAPRAT, luSink);
			rc |= pm_GetLookupData(C_PMLUTY_COP, luSink);
			break;

		}

		default:
			oer("Program error: Missing pm_type case");
	}

	return rc;
}		// PERFORMANCEMAP::pm_CkF
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GetGridAxis(
	PMGXTY gxTy,	// type sought
	const PMGRIDAXIS*& pGXRet) const
{
	RC rc = RCOK;
	pGXRet = nullptr;
	
	// re error reporting
	const record* errRec = nullptr;
	const char* msg = nullptr;

	// scan all PMLOOKUPDATA belonging to this PERFORMANCEMAP
	// verify only 1 record of type sought
	PMGRIDAXIS* pGX;
	PMGRIDAXIS* pGXFound = nullptr;
	RLUPC(PMGXB, pGX, pGX->ownTi == ss)
	{
		if (pGX->pmx_type == gxTy)
		{
			if (pGXFound != nullptr)
			{	msg = "Duplicate PMGRIDAXIS.";
				errRec = pGX;
				break;
			}
			pGXFound = pGX;
		}
	}

	if (!msg && pGXFound == nullptr)
	{	msg = "Missing PMLOOKUPDATA.";
		errRec = this;
	}
	if (msg)
		rc = errRec->oer( "%s\n"
			   "    When pmType='%s', exactly 1 PMGRIDAXIS with pmGXType='%s' is required.",
					msg, getChoiTx(PERFORMANCEMAP_TYPE),
					PMGXB.getChoiTx(PMGRIDAXIS_TYPE, 0, gxTy));
	else
		pGXRet = pGXFound;

	return rc;
}	// PERFORMANCEMAP::pm_GetGridAxis
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GetLookupData(
	PMLUTY luTy,	// type sought
	const PMLOOKUPDATA*& pLURet) const
{
	RC rc = RCOK;
	pLURet = nullptr;
	
	// re error reporting
	const record* errRec = nullptr;
	const char* msg = nullptr;

	// scan all PMLOOKUPDATA belonging to this PERFORMANCEMAP
	// verify only 1 record of type sought
	PMLOOKUPDATA* pLU;
	PMLOOKUPDATA* pLUFound = nullptr;
	RLUPC(PMLUB, pLU, pLU->ownTi == ss)
	{
		if (pLU->pmv_type == luTy)
		{
			if (pLUFound != nullptr)
			{	msg = "Duplicate PMLOOKUPDATA.";
				errRec = pLU;
				break;
			}
			pLUFound = pLU;
		}
	}

	if (!msg && pLUFound == nullptr)
	{	msg = "Missing PMLOOKUPDATA.";
		errRec = this;
	}
	if (msg)
		rc = errRec->oer( "%s\n"
			   "    When pmType='%s', exactly 1 PMLOOKUPDATA with pmLUType='%s' is required.",
					msg, getChoiTx(PERFORMANCEMAP_TYPE),
					PMLUB.getChoiTx(PMLOOKUPDATA_TYPE, 0, luTy));
	else
		pLURet = pLUFound;

	return rc;
}	// PERFORMANCEMAP::pm_GetLookupData
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GXCheckAndMakeVector(
	PMGXTY gxTy,			// type sought
	const PMGRIDAXIS* &pGX,		// returned: pointer to PMGRIDAXIS if found
	std::vector< double>& vGX,	// returned: grid values
	std::pair<int, int> sizeLimits) const	// supported size range
// returns RCOK iff return values are usable
//    else non-RCOK, msg(s) issued
{
	RC rc = pm_GetGridAxis(gxTy, pGX);
	if (rc)
		vGX.clear();
	else
		rc = pGX->pmx_CheckAndMakeVector(vGX, sizeLimits);

	return rc;

}	// PERFORMANCEMAP::pmx_CheckAndMakeVector
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_LUCheckAndMakeVector(
	PMLUTY luTy,	// type sought
	const PMLOOKUPDATA* &pLU,	// returned: PMLOOKUPDATA found
	std::vector< double>& vLU,	// returned: values
	int expectedSize) const
{
	RC rc = pm_GetLookupData(luTy, pLU);
	if (rc)
		vLU.clear();
	else
		rc = pLU->pmv_CheckAndMakeVector(vLU, expectedSize);

	return rc;

}	// PERFORMANCEMAP::pm_CheckAndMakeVector
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_SetupBtwxt(		// input -> Btwxt conversion
	record* pParent,		// parent (e.g. RSYS)
	const char* tag,		// identifying text for this interpolator (for messages)
	Btwxt::RegularGridInterpolator*& pRgi,		// returned: initialized Btwxt interpolator
	float capRef,			// reference capacity (e.g. cap47 or cap95)
	double& speedFRating) const		// returned: speed fraction for rated values

// assume pm_type has been checked
{

	RC rc = RCOK;

	delete pRgi;		// delete prior if any
	pRgi = nullptr;

	bool bCooling = pm_type == C_PERFMAPTY_CLGCAPRATCOP;

	// check input data and convert to vector
	// WHY vector conversion
	//   * convenient
	//   * allows modification of values w/o changing input records
	//     (which may have multiple uses)

	// grid variables
	std::vector< const PMGRIDAXIS*> pGX(2);
	std::vector< std::vector<double>> vGX(2);	// axis values
	std::vector< std::string> vId(2);		// axis ids
	rc |= pm_GXCheckAndMakeVector(C_PMGXTY_OUTDOORDBT, pGX[ 0], vGX[ 0], { 2, 5});
	rc |= pm_GXCheckAndMakeVector(C_PMGXTY_SPEED,      pGX[ 1], vGX[ 1], { 2, 4});

	if (rc)
		return rc;

	int LUSize = vGX[0].size() * vGX[1].size();		// # of LU values

	// normalize spd to minspd .. 1.
	double scale = 1./vGX[1][vGX[1].size()-1];
	VMul1(vGX[1].data(), vGX[1].size(), scale);
	speedFRating = pGX[1]->pmx_ratingSpeed * scale;

	// lookup values
	const PMLOOKUPDATA* pLUCap;
	std::vector< double> vCapRat;
	rc |= pm_LUCheckAndMakeVector(C_PMLUTY_CAPRAT, pLUCap, vCapRat, LUSize);
	const PMLOOKUPDATA* pLUCOP;
	std::vector< double> vCOP;
	rc |= pm_LUCheckAndMakeVector(C_PMLUTY_COP, pLUCOP, vCOP, LUSize);

	if (rc)
		return rc;

	// message handling linkage
	auto MX = std::make_shared< CourierMsgHandlerRec>( pParent);

	// transfer grid value to Btwxt axes
	std::vector<Btwxt::GridAxis> gridAxes;
	for (int iGX=0; iGX<2; iGX++)
	{
		gridAxes.emplace_back(
			Btwxt::GridAxis(
				vGX[ iGX],		// grid values
				Btwxt::InterpolationMethod::linear, Btwxt::ExtrapolationMethod::constant,
				{ -DBL_MAX, DBL_MAX },		// extrapolation limits
				pGX[ iGX]->pmx_id.CStr(),	// axis name (from PMGRIDAXIS user input)
				MX));						// message handler

	}

	// finalize lookup data
	
	std::vector< std::vector< double>> values;
	values.resize(2);
	for (int iLU = 0; iLU<LUSize; iLU++)
	{	double capRat = vCapRat[iLU];
		double COPNet = vCOP[iLU];
#if 1
		double capNet = (capRat * capRef);
		if (bCooling)
			capNet = -capNet;
#else
		NOT CORRECT!
		double capGross = (capRat * capRef) * (1.-fanF);
#endif
		double inpNet = abs( capNet) / COPNet;
		values[0].push_back(capNet);
		values[1].push_back(inpNet);
	}

	std::vector< Btwxt::GridPointDataSet> gridPointDataSets;
	gridPointDataSets.emplace_back(values[0], "Capacity");
	gridPointDataSets.emplace_back(values[1], "Input");

	// construct Btwxt object
	pRgi = new Btwxt::RegularGridInterpolator(gridAxes, gridPointDataSets, tag, MX);

	return rc;
}		// PERFORMANCEMAP::pm_SetupBtwxt
//=============================================================================
PERFORMANCEMAP* PMGRIDAXIS::pmx_GetPERFMAP() const
{
	return PerfMapB.GetAtSafe( ownTi);
}	// PMGRIDAXIS::pmx_GetPERFMAP
//-----------------------------------------------------------------------------
/*virtual*/ void PMGRIDAXIS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	pmx_id.Release();
	record::Copy( pSrc, options);
	pmx_id.FixAfterCopy();
}		// PMGRIDAXIS::Copy
//-----------------------------------------------------------------------------
RC PMGRIDAXIS::pmx_CkF()
{
	RC rc = RCOK;

	pmx_nValues = ArrayCountIsSet(PMGRIDAXIS_VALUES);
	// pmx_nValues may be 0 if pmGXValues is omitted
	// detected/msg'd in cul, no msg needed here

	if (pmx_type == C_PMGXTY_SPEED)
	{	// default rating speed to highest
		if (!IsSet(PMGRIDAXIS_RATINGSPEED) && pmx_nValues > 0)
			FldSet(PMGRIDAXIS_RATINGSPEED, pmx_values[pmx_nValues-1]);
	}
	else
		ignore("when pmGXType is not 'Speed'", PMGRIDAXIS_RATINGSPEED);

	// note add'l checking in

	return rc;
}		// PMGRIDAXIS::pmx_CkF
//-----------------------------------------------------------------------------
RC PMGRIDAXIS::pmx_CheckAndMakeVector(
	std::vector< double>& vGX,	// returned: input data as vector
	std::pair< int, int> sizeLimits) const	// supported size range
{
	RC rc = RCOK;

	vGX.clear();

	if (pmx_nValues < sizeLimits.first || pmx_nValues > sizeLimits.second)
		rc |= oer("Incorrect number of values.  Expected %d - %d, found %d.",
			sizeLimits.first, sizeLimits.second, pmx_nValues);

	if (!rc)
	{
		if (!VStrictlyAscending(pmx_values, pmx_nValues))
			rc |= oer("Values must be in strictly ascending order.");
		else
		{
			if (pmx_type == C_PMGXTY_SPEED)
				rc |= limitCheck(PMGRIDAXIS_RATINGSPEED, pmx_values[0], pmx_values[pmx_nValues-1]);
		}
	}

	if (!rc)
	{
		vGX.assign(pmx_values, pmx_values+pmx_nValues);

	}

	return rc;
}		// PMGRIDAXIS::pm_CheckAndMakeVector
//=============================================================================
PERFORMANCEMAP* PMLOOKUPDATA::pmv_GetPERFMAP() const
{
	return PerfMapB.GetAtSafe( ownTi);
}	// PMLOOKUPDATA::pm_LUGetPERFMAP
//-----------------------------------------------------------------------------
/*virtual*/ void PMLOOKUPDATA::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	pmv_id.Release();
	record::Copy( pSrc, options);
	pmv_id.FixAfterCopy();
}		// PMLOOKUPDATA::Copy
//-----------------------------------------------------------------------------
RC PMLOOKUPDATA::pmv_CkF()
{
	RC rc = RCOK;

	pmv_nValues = ArrayCountIsSet(PMGRIDAXIS_VALUES);

	return rc;
}		// PMLOOKUPDATA::pm_LUCkF
//-----------------------------------------------------------------------------
RC PMLOOKUPDATA::pmv_CheckAndMakeVector(
	std::vector< double>& vLU,
	int expectedSize) const
{
	RC rc = RCOK;

	vLU.clear();

	if (pmv_nValues != 1 && pmv_nValues != expectedSize)
		rc |= oer("Incorrect number of values.  Expected 1 or %d, found %d.",
			expectedSize, pmv_nValues);

	if (!rc)
	{
		if (pmv_nValues == 1)
			vLU.assign(pmv_values[0], expectedSize);
		else
			vLU.assign(pmv_values, pmv_values+expectedSize);

		// check values here?
	}

	return rc;
}		// PMLOOKUPDATA::pm_CheckAndMakeVector
//=============================================================================
// 
// hvac.cpp end
