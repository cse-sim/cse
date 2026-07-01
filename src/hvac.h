// Copyright (c) 1997-2022 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// HVAC utility functions
//==========================================================================

#if !defined( _HVAC_H_)
#define	_HVAC_H_

float CoolingSHR( float tdbOut,	float tdbCoilIn, float twbCoilIn, float vfPerTon);
float CoolingCapF1Spd( float SHR, float tdbOut, float tdbCoilIn, float twbCoilIn, float vfPerTon);
float CoolingInpF1Spd( float SHR, float tdbOut,	float tdbCoilIn, float twbCoilIn, float vfPerTon, float& inpFSEER);

void HeatingAdjust(float tdbOut, float tdbCoilIn, float vfPerTon, float& capF, float& eirF);
void CoolingAdjust(float tdbOut, float twbCoilIn, float vfPerTon, float& capF, float& eirF);

float ASHPCap95FromCap47( float cap47, bool useRatio, float ratio9547);     
float ASHPCap47FromCap95( float cap95, bool useRatio, float ratio9547);     
void ASHPConsistentCaps( float& cap95, float& cap47, bool useRatio, float ratio9547);

double FanVariableSpeedPowerFract(double flowFract, MOTTYCH motTy, bool bDucted);


///////////////////////////////////////////////////////////////////////////////
// class CourierMsgHandler: Courier-derived handler for library callback
//		 messages (used by e.g. Btwxt)
///////////////////////////////////////////////////////////////////////////////

#include <courier/courier.h>

// abstract base class
class CSEBaseCourier : public Courier::Courier
{
public:
	void receive_error(const std::string& crMsg) override { forward_message(MSGTY::msgtyERROR, crMsg); }
	void receive_warning(const std::string& crMsg) override { 
		if (message_level >= MSGTY::msgtyWARNING)
		{
			forward_message(MSGTY::msgtyWARNING, crMsg);
		}
	}
	void receive_info(const std::string& crMsg) override {
		if (message_level >= MSGTY::msgtyINFO) {
			forward_message(MSGTY::msgtyINFO, crMsg);
		}
	}
	void receive_debug(const std::string& crMsg) override {
		if (message_level >= MSGTY::msgtyDEBUG) {
			forward_message(MSGTY::msgtyDEBUG, crMsg);
		}
	}
	void set_message_level(MSGTY message_level_in) {
		// sets message level and preserves previous message level
		// that can be restored with `restore_message_level()`.
		stored_message_level = message_level;
		message_level = message_level_in;
	}
	void restore_message_level() {
		message_level = stored_message_level;
	}
	
private:
	MSGTY message_level{MSGTY::msgtyINFO};
	virtual void forward_message(MSGTY msgty, const std::string& crMsg) = 0;
	MSGTY stored_message_level{MSGTY::msgtyINFO};

};	// class CSEBaseCourier

//-----------------------------------------------------------------------------
class CSERecordCourier : public CSEBaseCourier
// route message to initiating record-based object
{
public:
	CSERecordCourier(class record* record_pointer)
		: record_pointer(record_pointer)
	{}

private:
	class record* record_pointer;		// pointer to record
	void forward_message(MSGTY msgty, const std::string& crMsg) {
		record_pointer->ReceiveMessage(msgty, crMsg.c_str());
	}
};	// class CSERecordCourier
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CHDHW: data and models for Combined Heat and DHW system
//              (initial version based on Harvest Thermal info)
///////////////////////////////////////////////////////////////////////////////
#if defined( _DEBUG)
#define CHDHW_TEST		// define to enable internal tests
#endif
class CHDHW
{
public:
	CHDHW( class record* pParent);
	virtual ~CHDHW();

	void chw_Clear();
	RC chw_Init( float operatingSFP, float mult=-1.f, float capHRated=-1.f);
	float chw_GetTRise(float tCoilEW = -1.f) const;
	float chw_GetRatedCap(float tCoilEW = -1.f) const;
	double chw_GetRatedBlowerAVF() const;
	double chw_GetRatedSpecificFanPower() const;
	void chw_CapHtgMinMax(float tCoilEW, float& capHtgNetMin, float& capHtgNetMax) const;

	float chw_WaterVolFlow(float qhNet, float tCoilEW);

	void chw_BlowerAVFandPower(float qhNet, float& avf, float& pwr);

	class record* chw_pParent;	// parent (typically RSYS)

private:
	// runtime values filled in chw_Init()
	//   values are derived from above re
	//     actual pressure (operating) fan power
	//     gross->net conversion

	using RGI = class Btwxt::RegularGridInterpolator;

	std::unique_ptr< RGI> chw_pAVFPwrRGI;
	std::unique_ptr< RGI> chw_pWVFRGI;
	std::unique_ptr< RGI> chw_pCapMaxRGI;

	float chw_capHtgNetMin;		// minimum net continuous heating capacity, Btuh
								//   (cycles when smaller load)
	float chw_capHtgNetMaxFT;	// maximum net heating capacity, Btuh
								//   at max coil entering water temp
	float chw_tRiseMax;			// full speed coil+fan temp rise, F
	float chw_AVFRated;			// full speed air flow, cfm
	float chw_mult;				// multiplier (scales capacity, fan power, water flow, )
								//   derived from rs_capH / chw_grossCaps[ 5] (= 36000.)
								//   default = 1

#if defined( CHDHW_TEST)
	void chw_Test();
#endif

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