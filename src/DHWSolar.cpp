// Copyright (c) 1997-2020 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// DHWSolar.cpp -- Solar Hot Water model implementation
///////////////////////////////////////////////////////////////////////////////

#include "CNGLOB.H"

#include "CSE.h"
#include "ANCREC.H" 	// record: base class for rccn.h classes
#include "rccn.h"
#include "IRATS.H"
#include "CNGUTS.H"
#include "CUPARSE.H"
#include "CNCULTI.H"
#include "solar.h"

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
		rc |= disallow( "when 'swTankUA' is specified", DHWSOLARSYS_TANKINSULR);

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
	{	sw_overHeatHrCount = 0;
		sw_tankQGainTot = 0.;
	}
	
	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
	{	rc |= pSC->sc_DoHour();
	}
	return rc;
}	// DHWSOLARSYS::sw_DoHour
//-----------------------------------------------------------------------------
RC DHWSOLARSYS::sw_DoSubhrStart(
	[[maybe_unused]] int iTk0)		// subhr starting tick within hr (0 .. Top.tp_nHrTicks()-1)
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

	sw_tank.hw_DoSubhrEnd(1.f, sw_tankpZn, NULL);
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

	sw_tankQGainTot += sw_tankQGain;

	// Add parasitics to meter
	if (sw_pMtrElec)
		sw_pMtrElec->H.mtr_Accum(sw_endUse, sw_parElec * BtuperWh);

	DHWSOLARCOLLECTOR* pSC;
	RLUPC(ScR, pSC, pSC->ownTi == ss)
		rc |= pSC->sc_DoHourEnd();

	// hour average tank outlet temp
	if (sw_drawVol > 0.f)
		sw_tankTOutlet /= sw_drawVol;		// draws: average by vol
	else
		sw_tankTOutlet = sw_tickTankTOutlet;	// no draws: use last tick

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

		double scTotQFluidAll = 0.;
		RLUPC(ScR, pSC, pSC->ownTi == ss)
		{	scTotQFluidAll += pSC->sc_totQFluid;
		}
		if (frDiff(scTotQFluidAll, sw_tankQGainTot) > .001)
			printf("\nDHWSOLARSYS energy balance trouble");
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
	{	pWS->ws_qSlr += qOut;
		pWS->ws_GetDHWSYSRES()->S.qSolar += qOut;
	}

	return rc;
}	// DHWSOLARSYS::sw_TickAccumDraw
//------------------------------------------------------------------------------
RC DHWSOLARSYS::sw_TickCalc(
	int iTk)		// tick within hour, 0 - Top.tp_nHrTicks()-1
{
	RC rc = RCOK;

	float sumVol = 0.f;			// all-collector total fluid volume for tick, gal
	float sumVolTInlet = 0.f;	// all-collector SUM( vol * inlet temp), gal-F
	float sumVolTOutlet = 0.f;	// all-collector SUM( vol * outlet temp), gal-F
	float scQGain = 0.f;		// gain to tank, Btu

	// tank heat exchange temp
	sw_tankTHx = sw_tank.hw_GetTankQTXTemp();

	if (sw_tickTankTOutlet >= sw_tankTHxLimit)	// if tank temp >= max allow temp
										//   collector not run (details not modeled)
		sw_overHeatTkCount++;
	else
	{
		// Calculate outlet temperature of all collectors
		// Using volume weighted average

		DHWSOLARCOLLECTOR* pSC;
		RLUPC(ScR, pSC, pSC->ownTi == ss)
		{	rc |= pSC->sc_DoSubhrTick();
			if (pSC->sc_tickVol > 0.f)
			{	sumVol += pSC->sc_tickVol;
				sumVolTOutlet += pSC->sc_tickVol*pSC->sc_tOutlet;
				sumVolTInlet += pSC->sc_tickVol*pSC->sc_tInlet;
				scQGain += pSC->sc_tickQFluid;
			}
		}
	}

	if (sumVol > 0.f)			// if there is collector flow
	{	// collector flow > 0
		sw_scTOutlet = sumVolTOutlet / sumVol;
#if 1
		sw_scTInlet = sumVolTInlet / sumVol;
		sw_tankQGain += scQGain;
#else
		float deltaT = sw_scTOutlet - sw_tankTHx;
		float mCp = sumVol * sw_scFluidVHC;	// Btu/F
		scQGain = sw_tankHXEff * mCp * deltaT;	// Btu
		sw_tankQGain += scQGain;
		float scTInletX = sumVolTInlet / sumVol;
		sw_scTInlet = sw_scTOutlet - sw_tankHXEff * deltaT;	// collector inlet temp for next tick
#endif
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
#if 0
		if (tOut > sw_tankTHxLimit && scQGain == 0.f)
			printf("\nhot");
#endif
	}
	else
		// estimate possible outlet temp = top node temp
		sw_tickTankTOutlet = sw_tank.hw_GetEstimatedTOut();

	return rc;
}		// DHWSOLARSYS::sw_TickCalc
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// DHWSOLARCOLLECTOR: represents a solar collector
//                    child of DHWSOLARSYS
///////////////////////////////////////////////////////////////////////////////
DHWSOLARCOLLECTOR::~DHWSOLARCOLLECTOR()
{	
}		// DHWSOLARCOLLECTOR::~DHWSOLARCOLLECTOR
//-----------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_CkF()
{
	RC rc = RCOK;
	
	// defaults are derived in sc_Init()
	// later derivation better supports input expressions with probes

	return rc;
}	// DHWSOLARCOLLECTOR::sc_CkF
//-----------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_Init()
{
	RC rc = RCOK;
	
	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	sc_areaTot = sc_area * sc_mult;

	if (!IsSet(DHWSOLARCOLLECTOR_OPRMASSFLOW))
	{	sc_oprMassFlow = sc_testMassFlow;
		sc_flowCorrection = 1.f;
	}
	else
		sc_flowCorrection = sc_FlowCorrection();	// operating and test mass flow rates given

	sc_oprFRUL = sc_flowCorrection * sc_testFRUL;
	sc_oprFRTA = sc_flowCorrection * sc_testFRTA;

	// collector loop operating heat capacity flow rate, Btuh-/F
	sc_oprMCp = sc_areaTot * sc_oprMassFlow * pSW->sw_scFluidSpHt;

	// collector loop total operating volume flow, gpm
	sc_oprVolFlow = sc_areaTot * sc_oprMassFlow / pSW->sw_scFluidDens * galPerFt3 / 60.f;

#if defined( _DEBUG)
	float mCp2 = sc_oprVolFlow * pSW->sw_scFluidVHC * 60.f;
	if (frDiff(sc_oprMCp, mCp2) > .0001f)
		printf("\nSolarCollector mCp inconsistency");
#endif

	// pump power, W
	if (!IsSet(DHWSOLARCOLLECTOR_PUMPPWR))
		sc_pumpPwr = 10.f * sc_oprVolFlow;

	sc_tickPumpQ = sc_pumpPwr * BtuperWh * Top.tp_tickDurHr;			// pump energy per tick, Btu
	sc_pumpDT = sc_pumpLiqHeatF * sc_pumpPwr * BtuperWh / sc_oprMCp;	// temp rise through pump

	// piping (sc_oprVolFlow must be known)
	rc |= sc_InitPiping();
	
	// incident angle multiplier (IAM)
	// sc_kta60 <= 0 says no IAM
	if (sc_kta60 > 0.f)
		// check sc_Kta value here rather than rc_CkF() re possible expressions
		rc |= limitCheckFix(DHWSOLARCOLLECTOR_KTA60, 0.2, 1.);
	rc |= sc_InitIAM();		// set up IAM run constants

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
	static const float incALim = RAD(89.f);
	float kta =
		  incA > incALim ? 0.f
		: incA > 0.f     ? max( 0.f, 1.f - sc_b0 * (1.f / cos(incA) - 1.f))
		:                  1.f;
	return kta;
}	// DHWSOLARCOLLECTOR::sc_Kta
//---------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_InitIAM()		// IAM one-time init
// sets up run constants for incidence angle modifier
{
	RC rc = RCOK;
	if (sc_kta60 <= 0.f)
	{	sc_ktaDS = sc_ktaDG = sc_ktaDB = 1.f;
		sc_b0 = 0.f;
	}
	else
	{	sc_b0 = sc_Deriveb0(sc_kta60, 60.f);

		// constant kta values for diffuse
		//   per Duffie/Beckman section 5.4
		float tiltD = DEG(sc_tilt);
		float incADS = RAD(59.7f - 0.1388f*tiltD + 0.001497*tiltD*tiltD);
		sc_ktaDS = sc_Kta(incADS);
		float incADG = RAD(90.f - 0.5788*tiltD + 0.002693f*tiltD*tiltD);
		sc_ktaDG = sc_Kta(incADG);
	}
	return rc;

}		// DHWSOLARCOLLECTOR::sc_InitIAM
//--------------------------------------------------------------------------
float DHWSOLARCOLLECTOR::sc_FlowCorrection() const
// calc flow correction ratio
// apply to test FRUL and FRTA values to calc operating values
{
	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	float mCpTest = sc_MCpTest();
	float mCpOpr = sc_oprMassFlow * pSW->sw_scFluidSpHt;

	float fPUL = -mCpTest * log(1.f + sc_testFRUL / mCpTest);

	float tOpr = mCpOpr / fPUL;
	float num = tOpr * (1.f - exp(-1.f/tOpr));
	float tTest = mCpTest / fPUL;
	float denom = tTest * (1.f - exp(-1.f/tTest));
		
	float r = num / denom;

	return r;

}		// DHWSOLARCOLLECTOR::sc_FlowCorrection
//--------------------------------------------------------------------------------------
float DHWSOLARCOLLECTOR::sc_MCpTest() const
// return mCp for SRCC conditions, Btu/hr-F
//   assume test fluid is water
{
	static const float spHt = 0.2388458966f * 4.182f;	// TRNSYS assumption
	float mCpTest = spHt * sc_testMassFlow;

	return mCpTest;
	
}		// DHWSOLARCOLLECTOR::sc_MCpTest
//-----------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_InitPiping()		// init collector loop piping params
{
	RC rc = RCOK;

	// default total length (supply + return)
	//   approx same as CSI-Thermal assumption (=50 ft + 5 ft per collector module)
	if (!IsSet(SCPIPE(LEN)))
		sc_piping.pr_len = 50.f + sc_areaTot / 5.f;

	// size pipe based on 4 fps
	sc_piping.pr_DeriveSizeFromFlow( sc_oprVolFlow, 4.f);

	sc_piping.pr_CalcUA();

	sc_piping.pr_SetBeta(sc_oprMCp, 0.5f);	// 0.5 = supply/return split

	sc_piping.pr_CalcGeom();		// fill pr_totals

	return rc;
}		// DHWSOLARCOLLECTOR::sc_InitPiping
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHour()
{
	RC rc = RCOK;
	sc_pumpInElec = sc_hrQFluid = sc_eff = sc_tOutletP = 0.f;

	if (Top.tp_isBegMainSim)
		sc_totQFluid = 0.;

	// plane incidence: use hourly values
	rc |= slPerezSkyModel(sc_tilt, sc_azm, Top.iHrST,
	   Wthr.d.wd_DNI, Wthr.d.wd_DHI,
	   Top.grndRefl,
	   sc_incA, sc_poaRadDB, sc_poaRadDS, sc_poaRadDG);

	// total incident w/o incidence angle modifier
	float poaRadTotNoIAM = sc_poaRadDB + sc_poaRadDS + sc_poaRadDG;

	if (sc_b0 > 0.f && poaRadTotNoIAM > 0.f)
	{	// apply IAM factors
		sc_ktaDB = sc_Kta(sc_incA);
		sc_poaRadDB *= sc_ktaDB;
		sc_poaRadDS *= sc_ktaDS;
		sc_poaRadDG *= sc_ktaDG;
		sc_poaRadTot = sc_poaRadDB + sc_poaRadDS + sc_poaRadDG;
		sc_poaRadIAM = sc_poaRadTot / poaRadTotNoIAM;
	}
	else
	{	sc_poaRadTot = poaRadTotNoIAM;
		sc_poaRadIAM = sc_ktaDB = 0.f;
	}

	if (!IsSet(DHWSOLARCOLLECTOR_PIPINGTEX))
		sc_pipingTEx = Top.tDbOHrAv;

	float eff050, eff150;
	float tOutlet050 = sc_TempOutlet( 50.f, eff050);
	float tOutlet150 = sc_TempOutlet(150.f, eff150);

	sc_tOutletM = (tOutlet150 - tOutlet050) / 100.f;
	sc_tOutletB = tOutlet050 - sc_tOutletM * 50.f;

	sc_effM = (eff150 - eff050) / 100.f;
	sc_effB = eff050 - sc_effM * 50.f;

	return rc;
}		// DHWSOLARCOLLECTOR::sc_DoHour
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoHourEnd()
{
	RC rc = RCOK;

	sc_eff = sc_eff / Top.tp_NHrTicks();

	sc_totQFluid += sc_hrQFluid;

	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	// Add pump energy to meter
	if (pSW->sw_pMtrElec)
		pSW->sw_pMtrElec->H.mtr_Accum(pSW->sw_endUse, sc_pumpInElec);

	return rc;
}	// DHWSOLARCOLLECTOR::sc_DoHourEnd()
//-----------------------------------------------------------------------------
float DHWSOLARCOLLECTOR::sc_TempOutlet(		// col+piping outlet temp for current conditions
	float tSup,				// supply temp from tank hx, F
	float& colEff)	const	// collector efficiency (w/o piping)
// calculates temp returned to tank from collector, including piping losses
// returns return temp, 0 if no sun
{
	float tRet = 0.f;
	colEff = 0.f;
	if (sc_poaRadTot > 0.f)
	{	float tColIn = sc_piping.pr_TempOutlet(tSup + sc_pumpDT, sc_pipingTEx);
		colEff = sc_oprFRUL * ((tColIn - Top.tDbOHrAv) / sc_poaRadTot) + sc_oprFRTA;
		// colEff = min(1.f, colEff);

		float heat_gain = sc_areaTot * sc_poaRadTot * colEff;		// heat collection power, Btuh
		float tColOut = tColIn + heat_gain / sc_oprMCp;
		tRet = sc_piping.pr_TempOutlet(tColOut, sc_pipingTEx);
	}
	return tRet;
}		// DHWSOLARCOLLECTOR::sc_TempOutlet
//--------------------------------------------------------------------------------------
RC DHWSOLARCOLLECTOR::sc_DoSubhrTick()
{
	RC rc = RCOK;

	DHWSOLARSYS* pSW = SwhR.GetAtSafe(ownTi);

	// inlet temp: use linear representation to balance against last-tick tank tHx
	//   (not exact for multiple collectors)
	sc_tInlet = (sc_tOutletB + pSW->sw_tankHXEff *(pSW->sw_tankTHx - sc_tOutletB))
		/ (1.f - (1.f - pSW->sw_tankHXEff)*sc_tOutletM);

	float tOutlet = sc_tOutletM * sc_tInlet + sc_tOutletB;
	float eff = sc_effM * sc_tInlet + sc_effB;

#if defined( _DEBUG)
	float effX;
	float tRetX = sc_TempOutlet( sc_tInlet, effX);
	if (frDiff(tOutlet, tRetX) > .001f || frDiff(eff, effX) > 0.001f)
		printf("\nLinear fubar");
#endif
	if (tOutlet <= 0.f)
		sc_tickOp = FALSE;	// no sun, pump is off
	else
	{
		sc_tOutletP = tOutlet;

		// Collector operating status
		if (sc_tickOp)
		{
			if (sc_tOutletP <= sc_tInlet + sc_pumpOffDeltaT)
				sc_tickOp = FALSE;
		}
		else if (sc_tOutletP > sc_tInlet + sc_pumpOnDeltaT)
			sc_tickOp = TRUE;

		if (sc_tickOp)
		{
			sc_tickVol = sc_oprVolFlow * Top.tp_tickDurMin;
			sc_pumpInElec += sc_tickPumpQ;
			sc_tOutlet = sc_tOutletP;
			sc_eff += eff;
			sc_tickQFluid = (sc_tOutlet - sc_tInlet) * Top.tp_tickDurHr * sc_oprMCp;
			sc_hrQFluid += sc_tickQFluid;
		}
	}

	if (!sc_tickOp)
	{	// not operating
		// sc_tOutletP: don't 0, might have reporting interest
		sc_tOutlet = sc_tickVol = sc_tInlet = 0.f;
	}

	return rc;

}	// DHWSOLARCOLLECTOR::sc_DoSubhrTick()
//=============================================================================