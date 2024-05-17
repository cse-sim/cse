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

double FanOperatingPowerFract(double flowFract, MOTTYCH motTy, bool bDucted);


///////////////////////////////////////////////////////////////////////////////
// class CourierMsgHandler: Courier-derived handler for library callback
//		 messages (used by e.g. Btwxt)
///////////////////////////////////////////////////////////////////////////////

#include <courier/courier.h>

// abstract base class
class CourierMsgHandlerBase : public Courier::Courier
{
public:
	void receive_error(const std::string& crMsg) override { forward_message(MSGTY::msgtyERROR, crMsg); }
	void receive_warning(const std::string& crMsg) override { forward_message(MSGTY::msgtyWARNING, crMsg); }
	void receive_info(const std::string& crMsg) override { forward_message( MSGTY::msgtyINFO, crMsg); }
	void receive_debug(const std::string& crMsg) override { forward_message(MSGTY::msgtyDEBUG, crMsg); }

private:
	virtual void forward_message(MSGTY msgty, const std::string& crMsg) = 0;

};	// class CourierMsgHandlerBase
//-----------------------------------------------------------------------------
class CourierMsgHandler : public CourierMsgHandlerBase
{
public:
	using MsgCallbackFunc = void(void* pContext, MSGTY msgty, const char* msg);

	CourierMsgHandler(MsgCallbackFunc* pMsgCallbackFunc,void* context)
		: cmh_pMsgCallbackFunc(pMsgCallbackFunc), cmh_context( context)
	{ }

private:
	virtual void forward_message(MSGTY msgty, const std::string& crMsg) override
	{
		const char* msg = crMsg.c_str();
		if (cmh_pMsgCallbackFunc)
			(*cmh_pMsgCallbackFunc)(cmh_context, msgty, msg);
		else
			err(PABT, "nullptr cmh_pMsgCallbackFunc '%s'", msg);

	}

private:
	void* cmh_context;						// caller context
	MsgCallbackFunc* cmh_pMsgCallbackFunc;	// pointer to callback function

};	// class CourierMsgHandler
//-----------------------------------------------------------------------------
class CourierMsgHandlerRec : public CourierMsgHandlerBase
// route message to initiating record-based object
{
public:
	CourierMsgHandlerRec(class record* pRec)
		: cmh_pRec(pRec)
	{}

private:
	virtual void forward_message(MSGTY msgty, const std::string& crMsg) override
	{
		const char* msg = crMsg.c_str();
		if (cmh_pRec)
			cmh_pRec->ReceiveMessage(msgty, msg);
		else
			err(PABT, "nullptr cmh_pRec '%s'", msg);
	}
	class record* cmh_pRec;		// pointer to record

};	// class CourierRecordHandlerRec
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CHDHW: data and models for Combined Heat and DHW system
//              (initial version based on Harvest Thermal info)
///////////////////////////////////////////////////////////////////////////////
class CHDHW
{
public:
	CHDHW( class record* pParent);
	virtual ~CHDHW();

	void hvt_Clear();
	RC hvt_Init( float operatingSFP);
	float hvt_GetTRise(float tCoilEW = -1.f) const;
	float hvt_GetRatedCap(float tCoilEW = -1.f) const;
	double hvt_GetRatedBlowerAVF() const;
	double hvt_GetRatedSpecificFanPower() const;
	void hvt_CapHtgMinMax(float tCoilEW, float& capHtgNetMin, float& capHtgNetMax) const;

	float hvt_WaterVolFlow(float qhNet, float tCoilEW);

	void hvt_BlowerAVFandPower(float qhNet, float& avf, float& pwr);

	class record* hvt_pParent;	// parent (typically RSYS)

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