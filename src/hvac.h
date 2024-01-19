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

///////////////////////////////////////////////////////////////////////////////
// class BXMSGHAN: Courierr-derived handler for Btwxt msg callbacks
///////////////////////////////////////////////////////////////////////////////

#include <courierr/courierr.h>

class BXMSGHAN : public Courierr::Courierr
{
public:
	enum BXMSGTY { bxmsgERROR = 1, bxmsgWARNING, bxmsgINFO, bxmsgDEBUG };
	using BtwxtCallback = void(const char* tag, void* pContext, BXMSGTY msgty, const char* message);

	BXMSGHAN(const char* _tag, BtwxtCallback* _pMsgHanFunc,void* _pContext)
		: bx_tag(_tag), bx_pMsgHanFunc(_pMsgHanFunc)
	{
		set_message_context(_pContext);
	}

	void error(const std::string_view message) override { forward_message(bxmsgERROR, message); }
	void warning(const std::string_view message) override { forward_message(bxmsgWARNING, message); }
	void info(const std::string_view message) override { forward_message( bxmsgINFO, message); }
	void debug(const std::string_view message) override { forward_message(bxmsgDEBUG, message); }

	static void BxHandleExceptions();

private:
	void forward_message(BXMSGTY msgty, std::string_view message)
	{
		if (bx_pMsgHanFunc)
			(*bx_pMsgHanFunc)(bx_tag.c_str(), message_context, msgty, strt_string_view(message));
		else
			err(PABT, "nullptr bx_pMsgHanFunc '%s'", strt_string_view(message));

	}

	std::string bx_tag;		// caller-provided text identifier passed to callback

	BtwxtCallback* bx_pMsgHanFunc;		// pointer to callback function

};	// class BXMSGHAN
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CHDHW: data and models for Combined Heat and DHW system
//              (initial version based on Harvest Thermal info)
///////////////////////////////////////////////////////////////////////////////
class CHDHW
{
public:
	CHDHW();
	virtual ~CHDHW();

	void hvt_Clear();
	RC hvt_Init( float blowerEfficacy);
	float hvt_GetTRise(float tCoilEW = -1.f) const;
	float hvt_GetRatedCap(float tCoilEW = -1.f) const;
	double hvt_GetRatedBlowerAVF() const;
	double hvt_GetRatedBlowerEfficacy() const;
	void hvt_CapHtgMinMax(float tCoilEW, float& capHtgNetMin, float& capHtgNetMax) const;

	float hvt_WaterVolFlow(float qhNet, float tCoilEW);

	void hvt_BlowerAVFandPower(float qhNet, float& avf, float& pwr);

	void hvt_ReceiveBtwxtMessage(const char* tag, BXMSGHAN::BXMSGTY msgTy, const char* message);

private:
	// base data from Harvest Thermal memos
	// gross capacity steps, Btuh
	static inline std::vector<double>hvt_grossCaps{ 6000., 12000., 18000., 24000., 30000., 36000. };
	// blower air flow, cfm
	static inline std::vector<double>hvt_AVF{ 400., 600., 750., 900., 1050., 1200. };
	// blower power, W at nominal 0.20 in WC static
	static inline std::vector<double>hvt_blowerPwr{ 29., 60., 96., 149., 216., 328. };
	// entering water temp, F
	static inline std::vector< double> hvt_tCoilEW{ 120., 130., 140., 150. };
	// coil water volume flow rate, gpm
	static inline std::vector< std::vector< double>> hvt_WVF{ {
			/* 120 F*/ 0.27, 0.56, 0.87, 1.22, 1.59, 1.57,
			/* 130 F*/ 0.22, 0.45, 0.70, 0.87, 1.26, 1.57,
			/* 140 F*/ 0.18, 0.38, 0.59, 0.81, 1.04, 1.29,
			/* 150 F*/ 0.16, 0.33, 0.50, 0.69, 0.89, 1.09 } };

	// runtime values filled in hvt_Init()
	//   values are derived from above re
	//     actual pressure (operating) fan power
	//     gross->net conversion

	using RGI = class Btwxt::RegularGridInterpolator;

	std::unique_ptr< RGI> hvt_pAVFPwrRGI;
	std::unique_ptr< RGI> hvt_pWVFRGI;
	std::unique_ptr< RGI> hvt_pCapMaxRGI;

	float hvt_capHtgNetMin;		// minimum net continuous heating capacity, Btuh
								//   (cycles when smaller load)
	float hvt_capHtgNetMaxFT;	// maximum net heating capacity, Btuh
								//   at max coil entering water temp
	float hvt_tRiseMax;			// full speed coil+fan temp rise, F

};	// class CHDHW
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class WSHPPERF: water source heat pump performance model
//                 derived from EnergyPlus (?)
///////////////////////////////////////////////////////////////////////////////
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