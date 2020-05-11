// Copyright (c) 1997-2020 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// DHWSolar.cpp -- Solar Hot Water model implementation
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"

#include "cse.h"
#include "ancrec.h" 	// record: base class for rccn.h classes
#include "rccn.h"
#include "irats.h"
#include "cnguts.h"
#include "cuparse.h"
#include "SLPAK.H"

///////////////////////////////////////////////////////////////////////////////
// DHWSOLARSYS: represents a solar water heating system
//              1 or more collectors + tank + pump
///////////////////////////////////////////////////////////////////////////////
/*virtual*/ void DHWSOLARSYS::ReceiveRuntimeMessage(const char* msg)
// callback from HPWH for reporting error messages
{
	pInfo("%s: HPWH message (%s):\n  %s",
		objIdTx(), Top.When(C_IVLCH_S), msg);
}		// DHWSOLARSYS::ReceiveRuntimeMessage
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_CkF()		// input check
// called at end of each DHWSOLARSYS input
// returns RCOK iff input is acceptable
{
	RC rc = RCOK;

	// tank surrounding temp -- one of swTankTEx or swTankZn, not both
	int nVal = IsSetCount(DHWSOLARSYS_TANKTEX, DHWSOLARSYS_TANKZNTI, 0);
	if (nVal == 0)
		rc |= oer("one of 'swTankTEx' and 'swTankZn' must be specified.");
	else if (nVal == 2)
		rc |= disallowN("when 'swTankTEx' is specified", DHWSOLARSYS_TANKZNTI, 0);

	// tank loss -- either swTankUA or swTankInsulR, not both
	//    (neither OK = use default swTankInsulR)
	nVal = IsSetCount(DHWSOLARSYS_TANKUA, DHWSOLARSYS_TANKINSULR, 0);
	if (nVal == 2)
		rc |= disallow( DHWSOLARSYS_TANKINSULR, "when 'swTankUA' is specified");

	return rc;
}	// DHWSOLARSYS::sw_CkF()
//---------------------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_Init()		// init for run
// called at RUN and again at sim start after autosize
// calls child DHWSOLARCOLLECTOR sc_Init
// returns RCOK iff run should continue
{
	RC rc = RCOK;

	if (sw_wsCount == 0)		// incremented in DHWSYS::ws_Init()
		oWarn("Not referenced by any DHWSYS; no useful energy output.");

	sw_pMtrElec = MtrB.GetAtSafe(sw_elecMtri);

	// collector loop fluid volumetric heat capacity, Btu/gal-F
	sw_scFluidVHC = sw_scFluidSpHt * sw_scFluidDens / galPerFt3;

	// DHWSOLARCOLLECTOR
	sw_scCount = 0.f;
	sw_scAreaTot = 0.f;
	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ss == ss)
	{	rc |= pSC->sc_Init();
		sw_scCount += pSC->sc_mult;
		sw_scAreaTot += pSC->sc_areaTot;		// sc_areaTot includes sc_mult
	}
	if (sw_scCount < .01f		// if no collectors found
	 || sw_scAreaTot < .01f)	// unexpected: sc_area and sc_mult are FLOAT_GZ
		rc |= oer("No DHWSOLARCOLLECTORs found -- cannot model.");

	if (!IsSet(DHWSOLARSYS_TANKVOL))
		sw_tankVol = 1.5f * sw_scAreaTot;		// default 1.5 gal/ft2

	sw_tankTHx = 60.f;  // F initial guess
	sw_tankTInlet = sw_tankTHx;
	sw_tankTOutlet = sw_tankTHx;
	sw_scTInlet = sw_tankTHx;
	sw_scTOutlet = sw_tankTHx;

	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;
	sw_tickTankTOutlet = sw_tankTOutlet;

	// tank setup
	//   use HPWH multi-node tank model w/o heater
	sw_tankpZn = ZrB.GetAtSafe(sw_tankZnTi);
	rc |= sw_tank.hw_Init( this);
	rc |= sw_tank.hw_InitTank( max( 5.f, sw_tankVol));
	rc |= sw_tank.hw_AdjustUAIf( sw_tankUA, sw_tankInsulR);
	// finalize tank, -1 = don't set inlet height
	rc |= sw_tank.hw_InitFinalize( -1.f, -1.f);

	sw_tank.hw_SetNQTXNodes( 3);	// use bottom 3 nodes
									//   solar collector HX

	// retrieve resolved tank characteristics
	//   for e.g. reports
	rc |= sw_tank.hw_GetInfo(sw_tankVol, sw_tankUA, sw_tankInsulR);

	return rc;

}		// DHWSOLARSYS::sw_Init
//--------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoHour()
// hour init
{
	RC rc = RCOK;

	sw_tankQGain = 0.f; // restart accumulator for new hour
	sw_tankQLoss = 0.f;
	sw_drawVol = 0.f;
	sw_totOut = 0.f;
	sw_tankTOutlet = 0.f;

	sw_overHeatTkCount = 0;
	if (Top.tp_isBegMainSim)
		sw_overHeatHrCount = 0;
	
	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{	rc |= pSC->sc_DoHour();
	}
	return rc;
}	// DHWSOLARSYS::sw_DoHour
//-----------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoSubhrStart(
	int iTk0)		// subhr starting tick within hr (0 .. Top.tp_nHrTicks()-1)
{
	RC rc = RCOK;

	// tank ambient temp
	//   set temp from linked zone (else leave expression/default value)
	if (sw_tankpZn)
		sw_tankTEx = sw_tankpZn->tzls;

	rc |= sw_tank.hw_DoSubhrStart(sw_tankTEx);

	return rc;
}		// DHWSOLARSYS::sw_DoSubhrStart
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoSubhrEnd()
{
	RC rc = RCOK;

	double totOut = 0.;		// substep total heat added to water
	sw_tank.hw_DoSubhrEnd(1.f, sw_tankpZn, NULL, totOut);
	sw_tankQLoss += BtuperkWh * sw_tank.hw_qLoss;

	return rc;
}		// DHWSOLARSYS::sw_DoSubhrEnd
//-----------------------------------------------------------------------------
RC DHWSOLARSYS::sw_EndIvl(
	IVLCH ivl)	// C_IVLCH_Y, _M, _D, _H (do not call for _S)
{
	RC rc = RCOK;

	if (sw_overHeatTkCount > 0)
		sw_overHeatHrCount++;

	// Add parasitics to meter
	if (sw_pMtrElec)
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec * BtuperWh);

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
		rc |= pSC->sc_DoHourEnd();

	if (sw_drawVol > 0.f)
		sw_tankTOutlet /= sw_drawVol;

	if (ivl == C_IVLCH_Y)
	{
		double SSFNum = 0.;
		double SSFDen = 0.;

		DHWSYS* pWS;
		RLUPC(WsR, pWS, pWS->ws_pDHWSOLARSYS == this)
		{
			SSFNum += pWS->ws_SSFAnnualSolar;
			SSFDen += pWS->ws_SSFAnnualReq;
		}
		sw_SSFAnnual = SSFDen > 0.
				? min( 1.f, float( SSFNum / SSFDen)) 
			    : 0.f;				
	}
	return rc;
}	// DHWSOLARSYS::sw_EndIvl
//-----------------------------------------------------------------------------
int DHWSOLARSYS::sw_ReportBalErrorsIf() const
// end-of-run tank energy balance error check / report
// returns # of balance errors during run
{
	if (sw_overHeatHrCount > 0)
		warn("%s: Tank temperature exceeded swTankTHxLimit during %d hrs.",
			objIdTx(), sw_overHeatHrCount);

	return record::ReportBalErrorsIf(sw_tank.hw_balErrCount, "subhour");
}		// DHWSOLARSYS::sw_ReportBalErrorsIf
//-----------------------------------------------------------------------------
FLOAT DHWSOLARSYS::sw_GetAvailableTemp()
// returns available tank outlet water temp, F
{
	return sw_tickTankTOutlet;
}	// DHWSOLARSYS::sw_GetAvailableTemp
//-----------------------------------------------------------------------------
void DHWSOLARSYS::sw_TickStart()		// init for tick calcs
{
	sw_tickVol = 0.f;
	sw_tickVolT = 0.f;
}	// DHWSOLARSYS::sw_TickStart
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_TickAccumDraw(			// accumulate draw for current tick
	DHWSYS* pWS,	// source DHWSYS
	float vol,		// volume drawn from DHWSOLARSYS, gal
	float tInlet)	// solar tank inlet temp from source DHWSYS, F
					//   after e.g. DWHR
// returns RCOK iff simulation should proceed
{
	RC rc = RCOK;
	sw_tickVol += vol;
	sw_tickVolT += vol * tInlet;
	float qOut = vol * waterRhoCp * (sw_tickTankTOutlet - tInlet);
	sw_totOut += qOut;	// hourly total
	if (pWS)
		pWS->ws_qSlr += qOut;

	return rc;
}	// DHWSOLARSYS::sw_TickAccumDraw
//------------------------------------------------------------------------------
double DHWSOLARSYS::sw_MassFlowSI(		// derive mass flow rate for collector calcs
	double vfr) const		// flow rate, gpm
// returns mass flow rate kg/s
{
	static const double gpmTom3ps = 0.00006309;
	double mDot = vfr * gpmTom3ps * DIPtoSI(sw_scFluidDens);
	return mDot;
}		// DHWSOLARSYS::sw_MassFlowSI
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_TickCalc(
	int iTk)		// tick within hour, 0 - Top.tp_nHrTicks()-1
{
	RC rc = RCOK;

	float scQGain = 0.f;		// collector heat to be added to tank, Btu
	float sumVol = 0.f;			// all-collector total fluid volume for tick, gal
	float sumVolT = 0.f;		// all-collector SUM( vol * outletTemp), gal-F

	// tank heat exchange temp
	sw_tankTHx = sw_tank.hw_GetTankQTXTemp();

	if (sw_tickTankTOutlet /*sw_tankTHx*/ >= sw_tankTHxLimit)	// if tank temp >= max allow temp
										//   collector not run (details not modeled)
		sw_overHeatTkCount++;
	else
	{
		// Calculate outlet temperature of all collectors
		// Using volume weighted average

		DHWSOLARCOLLECTOR* pSC;
		RLUPC(ScR, pSC, pSC->ownTi == ss)
		{
			rc |= pSC->sc_DoSubhrTick();
			if (pSC->sc_tickVol > 0.f)
			{	sumVol += pSC->sc_tickVol;
				sumVolT += pSC->sc_tickVol*pSC->sc_tOutlet;
			}
		}
	}

	if (sumVol > 0.f)			// if there is collector flow
	{	// collector flow > 0
		sw_scTOutlet = sumVolT / sumVol;
		float deltaT = sw_scTOutlet - sw_tankTHx;
		float mCp = sumVol * sw_scFluidVHC;	// Btu/F
		scQGain = sw_tankHXEff * mCp * deltaT;	// Btu
		sw_tankQGain += scQGain;
#if 0
		DHWSOLARCOLLECTOR* pSC = ScR.GetAt(1);
#endif
		sw_scTInlet = sw_scTOutlet - sw_tankHXEff * deltaT;	// collector inlet temp for next tick
	}
	else
	{	sw_scTInlet = sw_tankTHx;		// no flow
		sw_scTOutlet = sw_scTInlet;
	}

	sw_tank.hw_SetQTX( scQGain);	// pass gain to tank model
									//  (used in hw_DoSubhrTick)

	// draws
	if (sw_tickVol > 0.f)
	{	sw_tankTInlet = sw_tickVolT / sw_tickVol;
		sw_drawVol += sw_tickVol;
	}
	
	float tOut;
	rc |= sw_tank.hw_DoSubhrTick( iTk, sw_tickVol, sw_tankTInlet, &tOut);

	if (tOut > 0.f)
	{	sw_tickTankTOutlet = tOut;
		sw_tankTOutlet += tOut * sw_tickVol;
		if (tOut > sw_tankTHxLimit && scQGain == 0.f)
			printf("\nhot");
	}
	else
		// estimate possible outlet temp = top node temp
		sw_tickTankTOutlet = sw_tank.hw_GetEstimatedTOut();

	return rc;
}		// DHWSOLARSYS::sw_TickCalc
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class SolarFlatPlateCollector -- simple FR/tauAlpha collector model
//   (local class)
///////////////////////////////////////////////////////////////////////////////

class SolarFlatPlateCollector
{
public:
	SolarFlatPlateCollector();
	SolarFlatPlateCollector(double gross_area,
		double tilt,
		double azimuth,
		double FR_tau_alpha = 0.6,
		double FR_UL = -4.0,
		double specific_heat_fluid_ = 2394.0,
		double density_fluid_ = 1113.0);
	~SolarFlatPlateCollector() = default;

	void calculate(double Q_incident, double mass_flow_rate, double t_amb, double t_inlet);

	// Setters
	inline double set_tilt(double tilt) { tilt_ = tilt; }
	inline double set_azimuth(double azm) { azimuth_ = azm; }
	inline double set_FR_tau_alpha(double tau_alpha) { FR_tau_alpha_ = tau_alpha; }
	inline double set_FR_UL(double FRUL) { FR_UL_ = FRUL; }
	inline double set_gross_area(double area) { gross_area_ = area; }
	// Fluid properties (Cp, density)
	inline double set_specific_heat_fluid(double Cp) { specific_heat_fluid_ = Cp; }
	inline double set_density_fluid(double density) { density_fluid_ = density; }

	double outlet_temp() { return outlet_temp_; }
	double efficiency() { return efficiency_; }
	double heat_gain() { return heat_gain_; }

private:
	// Collector properties
	double tilt_;                ///< radians
	double azimuth_;             ///< radians
	double FR_UL_;               ///< Slope, W/m^2-K
	double FR_tau_alpha_;        ///< Intercept, dimensionless
	double gross_area_;          // collector area, m2
	double specific_heat_fluid_; ///< J / kg-K
	double density_fluid_;       ///< kg / m^3

	// Collector state (results of last calculate())
	double heat_gain_;   // heat gain, W
	double efficiency_;  // efficiency, <= 1 (can be <0)
	double outlet_temp_; // fluid outlet temp, K
};		// class SolarFlatPlateCollector
//-----------------------------------------------------------------------------
SolarFlatPlateCollector::SolarFlatPlateCollector()
	: tilt_(0.0),
	azimuth_(0.0),
	FR_tau_alpha_(0.6),
	FR_UL_(-4),
	gross_area_(1.0),
	specific_heat_fluid_(4181.6),
	density_fluid_(1000.0),
	efficiency_(1.0),
	outlet_temp_(283)
{
}
//-----------------------------------------------------------------------------
SolarFlatPlateCollector::SolarFlatPlateCollector(
	double gross_area,		// total panel area, m2
	double tilt,			// tilt from horiz, 0=facing up, 90=vert
	double azimuth,			// 
	double FR_tau_alpha,
	double FR_UL,
	double specific_heat,
	double density)
	: tilt_(tilt),
	azimuth_(azimuth),
	FR_tau_alpha_(FR_tau_alpha),
	FR_UL_(FR_UL),
	gross_area_(gross_area),
	specific_heat_fluid_(specific_heat),
	density_fluid_(density),
	efficiency_(1.0),
	outlet_temp_(283)
{
}
//-----------------------------------------------------------------------------
void SolarFlatPlateCollector::calculate(
	double Q_incident,		// incident solar, W/m2
	double mass_flow_rate,	// fluid flow rate, kg/s
	double ambient_temp,	// ambient temp, K
	double inlet_temp)		// inlet temp, K
// sets efficiency_, outlet_temp_, and heat_gain_
{
	// instantaneous collector efficiency, [-]
	if (Q_incident != 0.0)
	{
		efficiency_ = FR_UL_ * ((inlet_temp - ambient_temp) / Q_incident) + FR_tau_alpha_;
		efficiency_ = std::min(1.0, efficiency_);
		// instantaneous solar gain, [W]
		heat_gain_ = Q_incident * gross_area_ * efficiency_;
		outlet_temp_ = inlet_temp + heat_gain_ / (mass_flow_rate * specific_heat_fluid_);
	}
	else
	{
		efficiency_ = 0.0;
		outlet_temp_ = inlet_temp;
		heat_gain_ = 0.0;
	}
}
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWSOLARCOLLECTOR: represents a solar collector
//                    child of DHWSOLARSYS
///////////////////////////////////////////////////////////////////////////////
DHWSOLARCOLLECTOR::~DHWSOLARCOLLECTOR()
{	delete sc_collector;
	sc_collector = NULL;
}
//-----------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_CkF()
{
	RC rc = RCOK;
	
	// defaults are derived in sc_Init()
	// later derivation better supports input expressions with probes

	return rc;
}
//-----------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_Init()
{
	RC rc = RCOK;
	
	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	sc_areaTot = sc_area * sc_mult;

	if (!IsSet(DHWSOLARCOLLECTOR_PUMPFLOW_TEST))
		sc_pumpFlow = 0.03*sc_areaTot;  // initial rule of thumb: 0.03 gpm per ft2
	
	if (!IsSet(DHWSOLARCOLLECTOR_PUMPPWR))
		sc_pumpPwr = 10.f*sc_pumpFlow;  // don't know how good this assumption is

	delete sc_collector;		// insurance: delete prior, if any

	double fluidSpHt = SHIPtoSI(pSW->sw_scFluidSpHt);		// specific heat, J/kg-K
	double fluidDens = DIPtoSI(pSW->sw_scFluidDens);		// density, kg/m3

	// incident angle multiplier (IAM)
	// sc_Kta60 <= 0 says no IAM
	if (sc_Kta60 > 0.f)
		// check sc_Kta value here rather than rc_CkF() re possible expressions
		rc |= limitCheckFix(DHWSOLARCOLLECTOR_KTA60, 0.2, 1.);
	sc_b0 = sc_Deriveb0(sc_Kta60, 60.f);

	// constant Kta values for diffuse
	//   per Duffie/Beckman section 5.4
	float tiltD = DEG(sc_tilt);
	float incA_diffuse_sky = RAD(59.7f - 0.1388f*tiltD + 0.001497*tiltD*tiltD);
	sc_Kta_diffuse_sky = sc_Kta( incA_diffuse_sky);
	float incA_diffuse_ground = RAD(90.f - 0.5788*tiltD + 0.002693f*tiltD*tiltD);
	sc_Kta_diffuse_ground = sc_Kta(incA_diffuse_ground);

	sc_collector = new SolarFlatPlateCollector(
		AIPtoSI(sc_areaTot),
		sc_tilt,
		sc_azm,
		sc_FRTA_opr,
		UIPtoSI(sc_FRUL_opr),
		fluidSpHt,
		fluidDens);

	sc_tickOp = FALSE;

	return rc;
}	// DHWSOLARCOLLECTOR::sc_Init
//--------------------------------------------------------------------------------------
/*static*/ float DHWSOLARCOLLECTOR::sc_Deriveb0(
	float KtaX,		// Kta for given incidence angle
					//   0 < KtaX < 1
	float incA)		// incidence angle, degs
					//   0 <= incA < 90
// return b0 = incidence angle modifier coefficient
//   Kta( ang) = 1 - b0( 1/cos( ang) - 1)
{

	float b0 = 0.f;		// no modifier
	if (KtaX > 0.f && KtaX < 1.f)
	{	float cosIncA = cos(RAD(incA));
		if (cosIncA > 0.f)
			b0 = (1.f - KtaX) / (1.f / cosIncA - 1.f);
	}
	return b0;

}	// DHWSOLARCOLLECTOR::sc_Deriveb0
//--------------------------------------------------------------------------------------
float DHWSOLARCOLLECTOR::sc_Kta(
	float incA) const	// incident angle, rad
{
	float Kta = incA > 0.f
		? 1.f - sc_b0 * (1.f / cos(incA) - 1.f)
		: 1.f;
	return Kta;
}	// DHWSOLARCOLLECTOR::sc_Kta

//--------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_FlowCorrection()
{
	float r = 1.f;		// correction factor
	// work per unit area
	float mDotCp_test =
	float mDotCp_opr =
	if (mDotCp_opr != mDotCp_test)
	{
		float fPUL = -mDotCp * log(1.f - sc_FRUL_test / mDotCp_test);

		float tOpr = mDotCp_opr / fPUL;
		float num = tOpr * (1.f - exp(-tOpr));
		float tTest = mDotCp_test / fPUL;
		float denom = tTest * (1.f - exp(-tTest));
		
		r = num / denom;

	}
	sc_FRUL_opr = sc_FRUL_test * r;
	sc_FRTA_opr = sc_FRTA_test * r;



}		// DHWSOLARCOLLECTOR::sc_FlowCorrection
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHour()
{
	RC rc = RCOK;
	sc_pumpInElec = 0.f;
	sc_Qfluid = sc_eff = 0.0;

	// plane incidence: use hourly values
#if 0	// 5-1-2020
x aniso fix / return separate diffuse, beam, ground
x	double plane_incidence;
x	rc |= slPerezSkyModel(sc_tilt, sc_azm, Top.iHrST, Top.radBeamHrAv, Top.radDiffHrAv, Top.grndRefl, plane_incidence);
x
x	sc_poaOld = static_cast<FLOAT>(plane_incidence);
#endif

	rc |= slPerezSkyModel(sc_tilt, sc_azm, Top.iHrST,
	   Wthr.d.wd_DNI, Wthr.d.wd_DHI,
	   Top.grndRefl,
	   sc_incA, sc_beam, sc_diffuse_sky, sc_diffuse_ground);

	if (sc_b0 > 0.f)
	{	sc_beam *= sc_Kta(sc_incA);
		sc_diffuse_sky *= sc_Kta_diffuse_sky;
		sc_diffuse_ground *= sc_Kta_diffuse_ground;
	}

	sc_poa = sc_beam + sc_diffuse_sky + sc_diffuse_ground;

	sc_Qpoa = sc_poa * sc_areaTot;  // Btu/h-ft2 * ft2 = Btu

	return rc;
}		// DHWSOLARCOLLECTOR::sc_DoHour
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHourEnd()
{
	RC rc = RCOK;

	sc_eff = sc_eff / Top.tp_NHrTicks();

	DHWSOLARSYS* pSWHSys = SwhR.GetAtSafe(ownTi);

	// Add pump energy to meter
	if (pSWHSys->sw_pMtrElec)
	{
		pSWHSys->sw_pMtrElec->H.mtr_Accum(pSWHSys->sw_endUse, sc_pumpInElec);
	}

	return rc;
}	// DHWSOLARCOLLECTOR::sc_DoHourEnd()
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoSubhrTick()
{
	RC rc = RCOK;

	DHWSOLARSYS* pSWHSys = SwhR.GetAtSafe(ownTi);

	float tInlet = pSWHSys->sw_scTInlet;

	float pump_energy = sc_pumpPwr * BtuperWh * Top.tp_tickDurHr;  // Btuh * hr
	float pump_vol = sc_pumpFlow * Top.tp_tickDurMin;  // gal
	float pump_dt = pump_energy / (pump_vol * pSWHSys->sw_scFluidVHC);  // delta F

	// Calculate potential outlet temperature

	sc_tOutletP = sc_CalculateOutletTemp(pump_dt);

	// Collector status
	if (sc_tickOp)
	{	if (sc_tOutletP <= tInlet + sc_pumpOffDeltaT)
			sc_tickOp = FALSE;
	}
	else if (sc_tOutletP > tInlet + sc_pumpOnDeltaT)
		sc_tickOp = TRUE;

	if (sc_tickOp)
	{	sc_tickVol = pump_vol;
		sc_pumpInElec += pump_energy;
		sc_tOutlet = sc_tOutletP;
		sc_eff += sc_collector->efficiency();
		sc_Qfluid += sc_collector->heat_gain()*BtuperWh*Top.tp_tickDurHr; // W * Btu/W-h * hr = Btu
	}
	else
	{	sc_tickVol = 0.f;		// not operating
		sc_tOutlet = tInlet;
	}

	return rc;
}	// DHWSOLARCOLLECTOR::sc_DoSubhrTick()
//------------------------------------------------------------------------------------------
FLOAT DHWSOLARCOLLECTOR::sc_CalculateOutletTemp(
	float pump_dt)	// pump temp rise, R
{
	DHWSOLARSYS* pSW = SwhR.GetAtSafe( ownTi);

	// Collector inlet temperature (from tank heat exchanger)
	double tInlet = pSW->sw_scTInlet + pump_dt;		// degF
	
	// Mass flow rate, kg/s
	double m_dot_SI = pSW->sw_MassFlowSI( sc_pumpFlow);

	// Calculate outlet temperature
	sc_collector->calculate(IrIPtoSI( sc_poa), m_dot_SI, DegFtoK(Top.tDbOHrAv), DegFtoK(tInlet));

	return static_cast<FLOAT>(DegKtoF(sc_collector->outlet_temp()));
}	// DHWSOLARCOLLECTOR::sc_CalculateOutletTemp

//=============================================================================