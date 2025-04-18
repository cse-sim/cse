// Copyright (c) 1997-2024 The CSE Authors. All rights reserved.
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

#if defined( _DEBUG)
//-----------------------------------------------------------------------------
static std::pair<float,float> Factors1Spd(
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon)		// coil air flow std air cfm / ton

// returns [1] = capacity factor
//         [2] = input power factor

{
	float SHRRef = CoolingSHR(tdbOut, 80.f, 67., 400.f);
	float capFRef = CoolingCapF1Spd(SHRRef, tdbOut, 80.f, 67.f, 400.f);

	float SHR = CoolingSHR(tdbOut, tdbCoilIn, twbCoilIn, vfPerTon);
	float capF = CoolingCapF1Spd(SHR, tdbOut, tdbCoilIn, twbCoilIn, vfPerTon);

	float inpFSEERRef;
	float inpFEERRef = CoolingInpF1Spd(SHR, tdbOut, 80.f, 67.f, 400.f, inpFSEERRef);

	float inpFRef = tdbOut < 82.f ? inpFSEERRef
		: tdbOut < 95.f ? inpFSEERRef + (tdbOut - 82.f) * (inpFEERRef - inpFSEERRef) / 13.f
		: inpFEERRef;

	auto inpWt = [tdbOut](float fEER, float fSEER) -> float { return
		tdbOut < 82.f ? fSEER
		: tdbOut < 95.f ? fSEER + (tdbOut - 82.f) * (fEER - fSEER) / 13.f
		: fEER; };

	float inpFRefX = inpWt(inpFEERRef, inpFSEERRef);

	float inpFSEER;
	float inpFEER = CoolingInpF1Spd(SHR, tdbOut, tdbCoilIn, twbCoilIn, vfPerTon, inpFSEER);

	float inpF = inpWt(inpFEER, inpFSEER);

	float capFX = capF / capFRef;

	// input ref = capFRef * eirFRef;
	// input current = capF * eirF
	//   so inpF = capF * eirF / capFRef * eirFRef
	//           = capFX * eirF / eirFRef;

	float inpFX = capFX * inpF / inpFRef;

	return { capFX, inpFX };

}	// Factors1Spd
//-----------------------------------------------------------------------------
static std::pair<float,float> FactorsCutler(
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon)		// coil air flow std air cfm / ton

// returns [1] = capacity factor
//         [2] = input power factor
{
	// reference conditions
	float capFRef, eirFRef;
	CoolingAdjust(tdbOut, 67.f, 400.f, capFRef, eirFRef);

	// current operating conditions
	float capF, eirF;
	CoolingAdjust(tdbOut, twbCoilIn, vfPerTon, capF, eirF);

	float capFX = capF / capFRef;

	// input ref = capFRef * eirFRef;
	// input current = capF * eirF
	//   so inpF = capF * eirF / capFRef * eirFRef
	//           = capFX * eirF / eirFRef;

	float inpFX = capFX * eirF / eirFRef;

	return { capFX, inpFX };

}	// FactorsCutler
//-----------------------------------------------------------------------------
static RC WriteCurveData()
{

	FILE* f = fopen( "HVACCurvesClg.csv", "wt");
	if (!f)
		return RCBAD;		// can't open file

	fprintf( f, "tdbOut,tdbEnt,twbEnt,vfPerTon,capF1Spd,inpF1Spd,capFCutler,inpFCutler\n");

	float tdbOutList[] = { 75.f, 80.f, 82.f, 85.f, 90., 95.f, 100.f, 105.f,	110.f };
	float tdbCoilInList[] = {70.f, 75.f, 80.f, 85.f};
	float rhCoilInList[] = { .5f }; // { .2f, .4f, .6f, .8f };
	float vfList[] = { 400.f }; // { 300.f, 350.f, 400.f, 450.f };

	for (float tdbOut : tdbOutList)
	{			
		for (float tdbCoilIn : tdbCoilInList)
		{
			for (float rhCoilIn : rhCoilInList)
			{
				float wCoilIn = psyHumRat2(tdbCoilIn, rhCoilIn);
				float twbCoilIn = psyTWetBulb(tdbCoilIn, wCoilIn);
				for (float vfPerTon : vfList)
				{
					auto [capFCutler, inpFCutler] = FactorsCutler( tdbOut, tdbCoilIn, twbCoilIn, vfPerTon);
					auto [capF1Spd, inpF1Spd] = Factors1Spd(tdbOut, tdbCoilIn, twbCoilIn, vfPerTon);

					fprintf(f, "%.0f,%0.0f,%0.3f,%0.0f,%0.3f,%0.3f,%0.3f,%0.3f\n",
						tdbOut, tdbCoilIn, twbCoilIn, vfPerTon,
						capF1Spd, inpF1Spd, capFCutler, inpFCutler);

				}
			}
		}
	}
	fclose(f);
	return RCOK;
}	// WriteCurveData
#endif

//-----------------------------------------------------------------------------
float CoolingSHR(		// derive cooling sensible heat ratio
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon)		// coil air flow std air cfm/ton

// return: SHR under current conditions
{

#if 0 && defined( _DEBUG)
	static bool bCurveDataWritten = false;
	if (!bCurveDataWritten)
	{	bCurveDataWritten = true;
		WriteCurveData();	
	}
#endif

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
float CoolingCapF1Spd(	// capacity factor for 1 spd model
	float SHR,		// sensible heat ratio for current conditions
	float tdbOut,	// outdoor dry bulb, F
	float tdbCoilIn, // coil entering dry bulb, F
	float twbCoilIn,  // coil entering wet bulb, F
	float vfPerTon)	// coil air flow std air cfm / ton

// return: fCondCap = current conditions factor for total gross capacity
{

	float fCondCap;
	if (SHR > 0.9999f)
		fCondCap =  // dry coil
			  0.009483100f  * tdbCoilIn
			// + 0.f        * twbCoilIn
			- 0.000600600f  * tdbOut
			- 0.000148900f  * vfPerTon
			- 0.000032600f  * tdbCoilIn * tdbOut
			+ 0.000011900f  * tdbCoilIn * vfPerTon
			// + 0.f        * twbCoilIn * tdbOut
			// + 0.f        * twbCoilIn * vfPerTon
			- 0.000005050f  * tdbOut * vfPerTon
			// + 0.f        * twbCoilIn * twbCoilIn
			- 52.561740000f / vfPerTon
			+ 0.430751600f;
	else
		fCondCap = // wet coil
			// 0.f *       * tdbCoilIn
			+ 0.009645900f * twbCoilIn
			+ 0.002536900f * tdbOut
			+ 0.000171500f * vfPerTon
			// + 0.f       * tdbCoilIn* tdbOut
			// + 0.f       * tdbCoilIn* vfPerTon
			- 0.000095900f * twbCoilIn * tdbOut
			+ 0.000008180f * twbCoilIn * vfPerTon
			- 0.000007550f * tdbOut * vfPerTon
			+ 0.000105700f * twbCoilIn * twbCoilIn
			- 53.542300000f / vfPerTon
			+ 0.381567150f;

	return fCondCap;

}		// CoolingCapF1Spd
//-----------------------------------------------------------------------------
float CoolingInpF1Spd(	// input factor for 1 spd model
	float SHR,			// sensible heat ratio for current conditions
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon,		// coil air flow std air cfm / ton
	float& fInpSEER)	// returned: current conditions factor for input based on SEER

// return: fInpEER = current conditions factor for input based on EER
{
	float fBase, fInpEER;
	if (SHR > 0.9999f)
	{	fBase = 0.0046103f   * tdbCoilIn		// dry coil
				// + 0.f     * twbCoilIn
				+ 0.0125598f * tdbOut
				- 0.000512f  * vfPerTon
				- 0.0000357f * tdbCoilIn * tdbOut
				+ 0.0000105f * tdbCoilIn * vfPerTon;
				// + 0.f     * twbCoilIn * tdbOut
				// + 0.f     * twbCoilIn * vfPerTon
				// + 0.f     * tdbOut * vfPerTon
				// + 0.f     * twbCoilIn * twbCoilIn
				// + 0.f    / vfPerTon;
		fInpSEER = fBase - 0.316172311f;
		fInpEER = fBase - 0.475306500f;
	}
	else
	{
		fBase = // 0.f	 * tdbCoilIn
			- 0.0202256f * twbCoilIn
			+ 0.0236703f * tdbOut
			- 0.0006638f * vfPerTon
			// + 0.f     * tdbCoilIn* tdbOut
			// + 0.f     * tdbCoilIn* vfPerTon
			- 0.0001841f * twbCoilIn * tdbOut
			+ 0.0000214f * twbCoilIn * vfPerTon
			- 0.00000812f * tdbOut * vfPerTon
			+ 0.0002971f * twbCoilIn * twbCoilIn
			- 27.95672f / vfPerTon;
		fInpSEER = fBase + 0.209951063f;
		fInpEER = fBase + 0.015003100f;
	}

	return fInpEER;
}		// CoolingInpF1Spd
//------------------------------------------------------------------------------
void HeatingAdjust(		// heating off-rating adjustment
	float tdbOut,		// outdoor dry bulb, F
	float tdbCoilIn,	// coil entering dry bulb, F
	float vfPerTon,		// coil air flow std air cfm/ton
	float& capF,		// returned: capacity factor
	float& eirF)		// returned: EIR factor

{
	static constexpr float cC[] =
	{ 0.568706266, -0.000747282, -1.03432E-05, 0.00945408, 5.0812E-05, -6.77828E-06,
	  0.694045465, 0.474207981, -0.168253446
	};
	static constexpr float cE[] =
	{ 0.722917608, 0.003520184, 0.000143097, -0.005760341, 0.000141736, -0.000216676,
	  2.185418751, -1.942827919, 0.757409168
	};

	float tdbOut2 = tdbOut*tdbOut;
	float tdbCoilIn2 = tdbCoilIn*tdbCoilIn;
	float tdbOI = tdbOut*tdbCoilIn;
	float vRat = vfPerTon / 400.f;
	float vRat2 = vRat*vRat;

	capF = (cC[0] + cC[1]*tdbCoilIn + cC[2]*tdbCoilIn2 + cC[3]*tdbOut + cC[4]*tdbOut2 + cC[5]*tdbOI)
		*(cC[6] + cC[7]*vRat + cC[8]*vRat2);

	eirF = (cE[0] + cE[1]*tdbCoilIn + cE[2]*tdbCoilIn2 + cE[3]*tdbOut + cE[4]*tdbOut2 + cE[5]*tdbOI)
		*(cE[6] + cE[7]*vRat + cE[8]*vRat2);

}		// ::HeatingAdjust
//------------------------------------------------------------------------------
void CoolingAdjust(				// cooling off-rating adjustment
	float tdbOut,		// outdoor dry bulb, F
	float twbCoilIn,	// coil entering wet bulb, F
	float vfPerTon,		// coil air flow std air cfm/ton
	float& capF,		// returned: capacity factor
	float& eirF)		// returned: EIR factor
{
	static constexpr float cC[] =
	{	3.717717741, -0.09918866, 0.000964488, 0.005887776, -1.2808E-05, -0.000132822,
		0.718664047, 0.41797409, -0.136638137
	};

	static constexpr float cE[] =
	{ -3.400341169, 0.135184783, -0.001037932, -0.007852322, 0.000183438, -0.000142548,
	   1.143487507, -0.13943972,-0.004047787
	};

	// limit args
	tdbOut = max(tdbOut, 75.f);
	twbCoilIn = bracket(57.f, twbCoilIn, 72.f);

	float tdbOut2 = tdbOut*tdbOut;
	float twbCoilIn2 = twbCoilIn*twbCoilIn;
	float tdbOI = tdbOut*twbCoilIn;
	float vRat = vfPerTon / 400.f;
	float vRat2 = vRat*vRat;

	capF = (cC[0] + cC[1]*twbCoilIn + cC[2]*twbCoilIn2 + cC[3]*tdbOut + cC[4]*tdbOut2 + cC[5]*tdbOI)
		* (cC[6] + cC[7]*vRat + cC[8]*vRat2);

	eirF = (cE[0] + cE[1]*twbCoilIn + cE[2]*twbCoilIn2 + cE[3]*tdbOut + cE[4]*tdbOut2 + cE[5]*tdbOI)
		* (cE[6] + cE[7]*vRat + cE[8]*vRat2);

}		// ::CoolingAdjust
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
RC CHDHW::hvt_Init(		// one-time init
	float operatingSFP)		// full speed operating specific fan power, W/cfm
// returns RCOK iff success
{
	using namespace Btwxt;
	using GridAxes = std::vector< GridAxis>;
	using VVD = std::vector< std::vector< double>>;

	RC rc = RCOK;
	hvt_Clear();

	// derive running fan power
	double ratedSFP = hvt_GetRatedSpecificFanPower();
	double blowerPwrF = operatingSFP / ratedSFP;	// blower power factor
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
	auto cmhCHDHW = std::make_shared< CSERecordCourier>(hvt_pParent);

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
double CHDHW::hvt_GetRatedSpecificFanPower() const	// rated SFP
// returns rated specific fan power (SFP), W/cfm
{
	return hvt_blowerPwr.back() / hvt_GetRatedBlowerAVF();

}	// CHDHW::hvt_GetRatedSpecificFanPower
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
double FanVariableSpeedPowerFract(		// variable speed fan power fraction
	double flowFract,	// flow fraction = current air flow / nominal (rated) air flow
						// (may be > 1)
	MOTTYCH motTy,	// fan motor type
	bool bDucted)	// true -> ducted system

// returns power fraction = power at flowFract / nominal (rated) power
{
	// assume BPM if not PSC
	double f = motTy == C_MOTTYCH_PSC ? flowFract*(0.3*flowFract + 0.7)
		: bDucted ? pow(flowFract, 2.75)
		: pow3(flowFract);

	return f;

}		// ::FanVariableSpeedPowerFract


// hvac.cpp end
