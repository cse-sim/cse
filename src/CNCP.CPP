// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncp.cpp -- COOLPLANTs and CHILLERs runtime code for CSE.  Created 10-92.

// related code: code used only at setup time: cncult6.cpp
//               CHW coil model: cncoil.cpp.
//               class declarations: rccn.h, automatically generated from cnrecs.def.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <MSGHANS.H>	// MH_xxxx
#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// COOLPLANT CHILLER including mbr fcn decls NHPSTAGES HPSTAGESZ

#include <EXMAN.H>	// rer

#include <CNGUTS.H>	// run basAncs: Top, AhB, CpB.
// and any decls for this file that are not member fcns decl'd in rccn.h (from cnrecs.def)


/*-------------------------------- DEFINES --------------------------------*/
#define CPW 1.0				// heat capacity of water, Btu/lb

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/


//------------------------------- DEBUG AID --------------------------------
#ifdef DEBUG2 			//cnglob.h
void FC endCp() { }		// bkpt place, called from CHW coils & coolplants when decide NOT to compute or flag loads
#endif

/******************************************************************************************************************************
*  PLANT: COOLPLANTs and CHILLERs
******************************************************************************************************************************/

//------------------------------------------------------------------------------------------------------------------------------
BOO COOLPLANT::cpOn()	// turn on AVAIL coolplant. Loads (CHW coils) call this to be sure demand-activated coolPlant is on.

// return value: TRUE if ON.
// caller: cncoil.cpp:doCHWCoil
{
	switch (CHN(cpSched))
	{
	case C_OFFAVAILONVC_OFF:
		return FALSE;   			// scheduled off, cannot be turned on
		//case C_OFFAVAILONVC_AVAIL:  					// if demand-activated
	default:
		if (cpMode != C_OFFONCH_ON)     	// if not now on
			cpEstimate(TRUE);    		// turn it on now -- save an iteration.
		//fall thru
	case C_OFFAVAILONVC_ON:     					// sched on, should aready be on, check as insurance
		return (cpMode==C_OFFONCH_ON);
	}
	/*NOTREACHED */
}			// COOLPLANT::cpOn
//------------------------------------------------------------------------------------------------------------------------------
RC COOLPLANT::cpEstimate(BOO wantCool /*=FALSE*/)			// estimate a coolPlant: sets supply temp, mode

// notes: ahEstimate assumes plant is or will be ON when coil is not off and has demand.
//        ahCompute conditionally calls here to turn plant on to save an iteration.
//	  if coil is AVAIL but plant is OFF (an error condition), ok if extra iterations occur.

// called: 1) from hvacSubhrIter, arg FALSE
//	   2) from AH CHW coil code, wantCOOL=TRUE, to turn on COOLPLANT (save iteration) (via cpOn).
{
	RC rc = RCOK;
	OFFONCH cpModeWas = cpMode;		// save cpMode value
	cpClf++;				// call cpCompute after each cpEstimate (once in subhr, & if turned on during iteration)
	Top.cpKf++;				// tells caller to check for coolplants with cpClf or cpPtf on

// Plant mode changes: do schedule-driven changes and demand-driven turn-on's here. Demand-driven turn-off is done in cpCompute.

	if (CHN(cpSched)==C_OFFAVAILONVC_OFF)		// if scheduled OFF
		cpMode = C_OFFONCH_OFF;				// turn the coolplant OFF if it was not OFF
	else if (CHN(cpSched)==C_OFFAVAILONVC_ON)  		// if schedule is unconditionally ON
		cpMode = C_OFFONCH_ON;				// turn the coolplant ON if it was not ON
	else if (CHN(cpSched)==C_OFFAVAILONVC_AVAIL)		// if demand-driven on-off (redundant test)
		if (wantCool)					// if caller says there is demand
			cpMode = C_OFFONCH_ON;			// turn the coolplant ON if it was not ON

// Estimate supply temp

	// 1. use setpoint if changed
	if (cpTsSp != cpTsSpPr)   			// no tolerance: user input, trivial changes not expected
		cpTs = cpTsSp;
	// 2. if no load, conditionally revert estimate to setpoint for when load comes on
	if (qLoad >= 0.)				// if no load (negative) in previous subhour
		if (cpTs > cpTsSp)			// if last supply temp hier than setpoint: overloaded (or changed setpoint)
			cpTs = cpTsSp;			// revert to setpoint, ie assume no overload when flow next comes on
	// 3. else leave supply temp unchanged: is setpoint, or differs due to overload or underload; current is best starting point.

// If mode changed or estimated supply temp changed, flag plant and loads to recompute

	// load changes: cpCompute itself tests for load changes (not known yet).
	// changes to towerplant: not known till cpCompute done.

	if ( cpMode != cpModeWas					// if mode changed
			||  CHN(cpSched)!=C_OFFAVAILONVC_OFF 			// if supply temp changed when not off & not avail
			&&  ABSCHANGE( cpTs, cpTsEstPr) >= Top.absTol )	// ..
	{
		cpPtf++;						// say compute this coolplant. Top.cpKf set above.
		for (AH* ah=NULL; nxAh(ah); ) 			// loop AH's with CHW coils served by this COOLPLANT
		{
			if (ah->ahcc.wantQflag)			// if wanted cooling when doCoils last called
			{
				ah->ahPtf++;				// say recompute this air handler
				Top.ahKf++;				// say look for AH's flagged to recompute
			}
		}
	}

// update Pr's: prior values
	cpTsSpPr = cpTsSp;
	cpTsEstPr = cpTs;
	return rc;
}		// COOLPLANT::cpEstimate
//------------------------------------------------------------------------------------------------------------------------------
RC COOLPLANT::cpCompute()				// conditionally compute coolplant

/* Top.ahKf is set TRUE if computation done & results changed so as to possibly affect loads
   10-92 NOT set if load is met or supply temp unchanged (note mode changes that affect load done in cpEstimate).*/
// Top.tpKf is set TRUE if computation done & results changed so as to possibly towerPlants
{
	//cpEstimate has done: changed mode if required by cpSched or requested by load; estimated cpTs.
	RC rc = RCOK;
	TOWERPLANT *tp = TpB.p + cpTowi;		// point cooling tower plant serving this cooling plant

// determine load if on. Note: if cpSched is AVAIL, loads have used cpEstimate(TRUE) to turn on COOLPLANT if necessary.

	DBL demand = 0.;				// for accumulating (negative) coil loads
	DBL trMx = 0.;				// for return temp if have load (assuming max flow)
	if (cpMode==C_OFFONCH_ON)
	{
		for (AH* ah=NULL; nxAh(ah); ) 		// loop AH's with CHW coils served by this COOLPLANT
			demand += ah->ahcc.chwQ;		// accumulate heat put into pri loop water (negative)
		qLoadNx = demand + qPipeLoss;		// add (negative) pipe loss to coil/hx demand to get total coolplant load
		trMx = cpTs - qLoadNx/(mxPMw * CPW);	// return temp, assuming max flow (most conservative assumption)
	}
	else
		qLoadNx = 0.;				// no load if off

// if no demand, turn AVAIL coolplant OFF here. Turn-on is via cpOn call in load (cncoil.cpp:doChwCoil).

	if (CHN(cpSched)==C_OFFAVAILONVC_AVAIL  &&  demand >= 0.)	// if schedule says on-off is per demand and no (neg) demand
	{
		cpMode = C_OFFONCH_OFF;					// turn off
		qLoadNx = 0.;						// load is 0 (not qPipeLoss) when off
		// if just turned off, falls thru to: qLoad = qLoadNx; stgi = 0; all chillers OFF; q = plr = qTow = mwTow = 0.; etc.
	}

// test conditions requiring recomputation

	if (cpPtf)								// if must-compute flag already set, eg from cpEstimate,
		;								// skip tests for speed.
	else if (cpMode != cpModePr)					// if mode changed,
		cpPtf++;								// compute
	else //if (cpMode==C_OFFONCH_ON)					[if on and -- but can't change if off]
		if (RELCHANGE( qLoadNx, qLoadPr) >= Top.relTol)     		// if load changed,
			cpPtf++;       						// compute.
		else if (RELCHANGE( trMx - cpTs, trMxPr - cpTs) >= Top.relTol)	// if (max-flow) ret temp changed, must compute ...
			cpPtf++;								/* ... to insure convergence of (overloaded) supply temp
									   when coil load does not change w ts increase */
		else if (RELCHANGE( tp->tpTs - tTow, tCnd - tTow) >= Top.relTol)	// if towerplant-to-condenser temp changed
			cpPtf++;								// (rel to return temp to towerplant), compute.

// exit if no recompute required
	if (!cpPtf)
	{
#ifdef DEBUG2//cnglob.h
		endCp();  						// cncp.cpp: place to put breakpoint to reach at end of iteration
		endTp();  						// cntp.cpp: place to put breakpoint to reach at end of iteration
#endif
		return rc;
	}

// compute COOLPLANT

	qLoad = qLoadNx;  				// commit to recalc: copy 'next' value to 'current'
	tCnd = tp->tpTs;				// from towerplant, fetch temp for water entering chiller condensers
	for (CHILLER* ch=NULL; nxCh(ch); ) 		// init all chillers off, 0 output, for unused chillers or OFF plant
	{
		ch->chMode = C_OFFONCH_OFF;
		ch->q = 0.;
		ch->p = 0.;
	}
	if (cpMode==C_OFFONCH_OFF)			// if heatplant off but being recomputed, e.g. just turned off
	{
		stgi = 0;				// revert to lowest stage / redetermine stage at turn-on (no hysteresis)
		q = plr = 0.;				// no output
		qTow = mwTow = 0.;			// no flow or heat to cooling towers
		// all chillers were initialized OFF above with p = q = 0.
		// ah's were flagged by cpEstimate if cp went off per schedule; if no demand (sched=AVAIL), no ah's need flagging.
	}
	else				// COOLPLANT is ON
	{
		// check hourly variable ts setpoint (do in a begHour fcn if one is added).  12-3-92.
		if (cpTsSp < 32.)
			rer( (char *)MH_R1360, name, cpTsSp); 		// "COOLPLANT '%s': cpTsSp (%g F) is below freezing"

#undef DUMMYCHW  	// also independenly definable in cncoil.cpp, cntp.cpp.
#ifdef DUMMYCHW		// for simple computation model for initial test of interfaces & overall program logic
*
*       // this dummy does not run chillers nor account input energy.
*
*   // determine supply temp, q, plr
*
*       stgi = stgMxCap;						// use most powerful stage
*       cap = mxCapDs;						// for use design cap (from setup) of most powerful stage (neg)
*       qNeed = qLoad + (cpTsSp - cpTs) * stgPMw[stgi] * CPW;	// power to return primary water to supply temp (neg)
*       tr = cpTs - qLoad/(stgPMw[stgi] * CPW);			// return temp, for this stage's flow, prior ts
*       if (qNeed >= cap)    					// if qNeed can be supplied (negs!)
*       {  cpTs = cpTsSp;
*          q = qNeed;
*          plr = qNeed/cap;
*		}
*       else							// cannot output setpoint
*		{	cpTs = tr + cap/(stgPMw[stgi] * CPW);			// supply temp
*			q = cap;						// neg power output
*			plr = 1.0;						// fraction of full load
*		}
*       puteTs = cpTs;					// save computed ts where cpEstimate won't overwrite it: debug aid.
*
*   // determine heat to cooling towers
*
*       qTow = stgCPQ[stgi]				// heat to cooling towers (positive): condenser pump heat,
*              - q;					// plus chiller output (make positive). dummy ignores elec input.
*       mwTow = stgCMw[stgi];				// condenser flows === flow to cooling towers: as precomputed at setup
*       tTow = tCnd + qTow / mwTow;			// compute temperature of water from plant to cooling towers
*
#else	// DUMMYCHW not defined

		// determine stage, supply temp, q, plr

		DBL tsWas = cpTs;				// cpTs value last seen by load (coil models): save b4 change:
		cpTs = cpTsSp;					// init supply temp to setpoint for use in chPyCapT poly in loop
		DBL tsLast;					// for prior iteration cpTs for loop endtest
		do						// loop to repeat if cpTs changes much (no repeat if at setpoint)
		{
			// add iterations check if found necess.
			tsLast = cpTs;				// remember previous iteration result (or initial assumed value)

			// determine stage (stgi) to meet load

			// if load >= 0 gets here, runs stage 1. Here load AND capStg() include pump heats.
			// stage capacity is condition-dependent: function capStg() uses stgi, cpTs, tCnd, sets .cap to returned value.
			// heat needed depends on primary flow, thus stage (unless ts==tsSp): macro QNEED.
#define QNEED (qNeed = (qLoad + (cpTsSp - tsWas) * stgPMw[stgi] * CPW))	// negative. note sets .qNeed.

			if (  stgi >= 0  &&  stgi < stgN				// if have valid stage # (insurance)
					&&  RELCHANGE(capStg(),QNEED) > Top.relTol )		// and its capacity matches load, with hysteresis
				;								// use it -- avoid stage change on small load change
			else
			{
				for (stgi = 0;  stgi < stgN && capStg() >= QNEED;  stgi++)	// look for first stage that can meet (neg) load.
					;							// capStg uses .stgi, sets .cap.
				// unused stages (capStg()==0) expected.
				if (stgi >= stgN) 						// if no stage powerful enough
					stgi--;							// use last stage (stgN-1); capStg just done for it.
			}

			// determine return temp (tr), part load ratio (plr), heat to primary water (q), supply temp (cpTs)

			tr = tsWas - qLoad/(stgPMw[stgi] * CPW);	// return temp, for this stage's flow, prior ts (member)
			FLOAT ph = stgPPQ[stgi];			// fetch (pos) pri pump heat (incl in cap) to handy local
			if (qNeed <= cap)				// if do not have needed (neg) power)
				plr = 1.;					// operate chillers at full power. fallthru computes q = cap (negative).
			else if (qNeed < ph)				// normally: qNeed negative or at least < pump heat (positive)
				plr = (qNeed - ph)/(cap - ph);		// chiller part load ratio: take pump heat as part of load, not capacity.
			// fallthru computes  q = qNeed (cap < q < ph)  and  cpTs = cpTsSp.
			else						// HEAT! > ph needed: plr wd be <0: poss w bad init or bug when cpSched=ON?
				plr = 0.;					// run chillers at 0 output. fallthru computes q = ph (positive).
			// iteration should restore temp to where cooling used.
			q = ph + (cap - ph) * plr;			// compute heat to water from pumps and fraction plr of chiller capacity
			cpTs = tr + q/(stgPMw[stgi] * CPW);		// compute supply temp this heat yeilds
		}
		while (fabs(cpTs-tsLast) > Top.absTol);  	/* if cpTs changed much, redo, so chPyCapT's are done at/near final cpTs.
       							   Will only loop if setpoint not met. */
		puteTs = cpTs;					// save computed ts where cpEstimate won't overwrite it: debug aid.

		// cpTs < 32 messages: cpTsSp < 32 msg'd above; each chiller output < 32 may be msg'd in CHILLER::endSubhr.

		// tentative frozen return temp check 12-3-92. Assumes no overrun; if necessary, disable til coil flows modelled.
		// move following check to endSubhr fcn, if one is added, for fewer messages.
		if ( tr < 32.					// if return "water" is frozen (lo texWant, CHW coils heating cold air)
				&&  cpTs >= 32. )				// but omit msg if supplied "water" frozen (cpTsSp < 32 msg'd above)
			rer( (char *)MH_R1361, name,  		// "COOLPLANT '%s': \n"
				 tr );					// "    return water temperature (%g F) from coils is below freezing"

		// chillers input power and heat to cooling towers, for ON chillers of ON coolplant

		qTow = stgCPQ[stgi]				// heat to cooling towers (positive): condenser pump heat,
			   - q;					// plus chiller output (make positive), plus elec input (added below)
		for (CHILLER *ch=NULL;  nxChStg(ch);  )		// loop over chillers of running stage (.stgi)
		{
			ch->chMode = C_OFFONCH_ON;			// turn this chiller on
			ch->q = ch->cap * plr;			// chiller output: cap (set by capStg()) times part load ratio
			DBL eiPlrCr								// energy input part load correction factor
#if 0//DELETE after testing fix
x             =    plr < ch->chMinFsldPlr  ?  ch->chMinUnldPlr * plr/ch->chMinFsldPlr	// cycling region
x               :  plr <= ch->chMinUnldPlr ?  ch->chMinUnldPlr				// false loading region (const input)
#else//bug fix 11-30-92
		=    plr < ch->chMinFsldPlr  ?  ch->eirMinUnldPlr * plr/ch->chMinFsldPlr 	// cycling region
			 :  plr <= ch->chMinUnldPlr ?  ch->eirMinUnldPlr				// false loading region (const input)
#endif
							   :  plr < 1.0               ?  ch->chPyEirUl.val(plr)			// unloading region: poly gives curve
							   :  1.0;									// full load
			ch->p =  -ch->cap 					// elec input = capacity (made positive)
					 * ch->chEirDs    				// times eir at design conditions
					 * eiPlrCr 					// part load correction (above)
					 * ch->chPyEirT.val(cpTs,tCnd);    		// temperatures correction polynomial (uniform temp assumed)
			qTow += ch->chMotEff * ch->p;				/* elec input heat that goes to condenser:
						 		   hermetic: all, motEff is 1.0;
          							   open: motor output power only, motEff is real. */
		}
		mwTow = stgCMw[stgi];				// condenser flows === flow to cooling towers: as precomputed at setup
		tTow = tCnd + qTow / mwTow;			// compute temperature of water from plant to cooling towers

#endif	// ifdef DUMMYCHW else

		// if output to towers changed since towers computed, call-flag towerplant

		if ( RELCHANGE( tTow - tCnd, tTowPr - tCnd) >= Top.relTol 	// if return temp changed significantly rel to tp ts,
				||  RELCHANGE( mwTow, mwTowPr) >= Top.relTol )			// or if flow changed, may need to compute tp
		{
			Top.tpKf++;					// tell caller hvacIterSubhr: check for flagged towerplants
			tp->tpClf++;					/* say call this towerplant, so it can check for changed return temp
          						   (overall still may be in tol even tho changed here) */
		}
#ifdef DEBUG2
		else
			endTp();  					// cntp.cpp: place to put breakpoint to reach at possible end of iteration
#endif

		// if supply temp changed since loads (coils) computed, flag active loads to be recomputed
		//   OTHER load effects: sched turn-on/off: cpEstimate does, thus already seen by loads if here
		//                       demand turn-on:    1st load with demand invokes re-estimate, thus already seen by loads
		//                       demand turn-off:   no demand --> does not affect loads.

		for (AH* ah=NULL; nxAh(ah); ) 				// loop AH's with CHW coils served by this COOLPLANT
			if (RELCHANGE( cpTs - tr, ah->ahcc.cpTsPr - tr) 	// if supply temp relative to return significantly changed
					>= Top.relTol)					//  from cpTs last used to compute coil
			{
				ah->ahcc.trPr = ah->ahcc.tr;		// save each coil tr at which cp computed: used by coil to call-flag plant
				//ah->ahcc.qPr = ah->ahcc.chwQ;		// save each coil q at which cp computed: used by coil to call-flag plant
				// restore qPr if need to monitor coil power demonstrated
				if (ah->ahcc.chwQ < 0.)    		// if coil used coolth from plant when last computed
				{
					ah->ahPtf++;				// say recompute this air handler
					Top.ahKf++;				// say look for ah's that need recomputing
				}
			}
#ifdef DEBUG2
			else
				endCp();					// cncp.cpp: place to put breakpoint to reach at possible end of iteration
#endif
	}	// if cpMode==OFF...else...

// update -Pr's: prior values; clear change flag

	cpModePr = cpMode;
#if 0
	x    tCndPr = tCnd;
	x    cpTsPr = cpTs;
#endif
	trMxPr = trMx;
	qLoadPr = qLoad;
	//mwTowPr = mwTow;		set in tpCompute
	//tTowPr = tTow;		set in tpCompute
	cpPtf = FALSE;				// clear compute-flag
	cpClf = FALSE;				// clear call-flag
	return rc;					// also no-recompute return is above
}			// COOLPLANT::cpCompute
//----------------------------------------------------------------------------------------------------------------------------
DBL/*POWER*/ COOLPLANT::capStg()	// power of stage .stgi incl pump heat at current .cpTs, .tCnd. negative.
// uses cpTs, tCnd, stgi.
// returns and sets plant .cap to capacity after allowing for stage primary pump heat
// and sets .cap of each chiller in stage
{
	cap = + stgPPQ[stgi];				// member receives capacity. init to (positive) pump heat
	for (CHILLER *ch = NULL;  nxChStg(ch);  )		// loop coolplant's chillers in stage .stgi
		cap +=						// accumulate plant capacity (negative)
			ch->cap =					// this chiller current capacity (negative)
				ch->chCapDs * ch->chPyCapT.val( cpTs, tCnd);	/* design cap adjusted with poly for supply & condenser temps.
          						   cap, chCapDs negative. */
	if (cap > 0.)					// positive capacity may screw up calling code
		rer( (char *)MH_R1362,
			 name, INT(stgi+1), -(cap - stgPPQ[stgi]), 	// "COOLPLANT '%s' stage %d chiller capacity (%g)\n"
			 stgPPQ[stgi], cpTs, tCnd );			// "    is less than pump heat (%g) (ts=%g, tCnd=%g)"
	return cap;
}			// COOLPLANT::capStg
//----------------------------------------------------------------------------------------------------------------------------
//RC COOLPLANT::endSubhr?? nothing to do.
//----------------------------------------------------------------------------------------------------------------------------
RC CHILLER::endSubhr()
{
	DBL plr;
	if (chMode==C_OFFONCH_ON)		// if chiller on, charge input energy to meters
	{
		if (mtri)       MtrB.p[mtri].H.clg += p*Top.tp_subhrDur;		// compressor energy
		if (chpp.mtri)  MtrB.p[chpp.mtri].H.aux += chpp.p*Top.tp_subhrDur;	// primary pump
		if (chcp.mtri)  MtrB.p[chcp.mtri].H.aux += chcp.p*Top.tp_subhrDur;	// condenser pump
		COOLPLANT *cp = CpB.p + ownTi;					// point chiller's COOLPLANT
		plr = cp->plr;						// fetch part load ratio from owning COOLPLANT for aux's
		pAuxFullOff = 0.;					// none of this when on at all in subhour
		pAuxOnAtall = auxOnAtall;				// charge all of this aux power if chiller ran at all in subhour
		if (auxOnAtallMtri)					// aux energy when on at all in subr
			MtrB.p[auxOnAtallMtri].H.clg += pAuxOnAtall*Top.tp_subhrDur;

		// chiller ice output check 12-3-92. Assumes no overrun; if necessary, suppress til actual flow simulated.
		DBL chTs = cp->tr + q / chpp.mw;				// this chiller output temp assuming no flow overrun (q negative)
		if ( chTs < 32.						// if frozen
				&&  cp->cpTs >= 32. )					// but overall coolplant not frozen (that yields msg in cpCompute)
			rer( (char *)MH_R1363, name, cp->name, chTs);   	/* "CHILLER '%s' of COOLPLANT '%s': \n"
								   "    delivered water temp (%g F) is below freezing" */
	}
	else				// chiller is off
	{
		plr = 0.;						// fraction of full load: 0
		pAuxOnAtall = 0.;					// none of this aux energy when off for entire subhour
		pAuxFullOff = auxFullOff;				// charge this aux energy only when not used in entire subhour
		if (auxFullOffMtri)					// aux energy when off entire subhr
			MtrB.p[auxFullOffMtri].H.htg += pAuxFullOff*Top.tp_subhrDur;
	}
	pAuxOn = plr * auxOn;					// off-proportional aux power: plr * user input
	if (auxOnMtri)						// load-proportional aux energy
		MtrB.p[auxOnMtri].H.clg += pAuxOn*Top.tp_subhrDur;
	pAuxOff = (1. - plr) * auxOff;				// off-proportional aux power
	if (auxOffMtri)						// chiller-off proportional aux energy
		MtrB.p[auxOffMtri].H.htg += pAuxOff*Top.tp_subhrDur;
	return RCOK;
}			// CHILLER::endSubhr
//----------------------------------------------------------------------------------------------------------------------------
// utility
//----------------------------------------------------------------------------------------------------------------------------
BOO COOLPLANT::nxAh( AH *&ah)		// first/next air handler with CHW coil served by this COOLPLANT.
// init ah NULL.   Usage:  for (AH* ah=NULL; nxAh(ah); ) { ... }
{
	TI ahi = ah ? ah->ahcc.nxAh4cp : ah1;
	if (ahi)
	{
		ah = AhB.p + ahi;
		return TRUE;
	}
	return FALSE;
}
//------------------------------------------------------------------------------------------------------------------------------
BOO COOLPLANT::nxCh( CHILLER *&ch)  	// first/next chiller in this COOLPLANT.
// init ch NULL.   Usage:  for (CHILLER* ch=NULL; nxCh(ch); ) { ... }
{
	TI chi = ch ? ch->nxCh4cp : ch1;
	if (chi)
	{
		ch = ChB.p + chi;
		return TRUE;
	}
	return FALSE;
}
//------------------------------------------------------------------------------------------------------------------------------
BOO COOLPLANT::nxChStg( CHILLER *&ch, int _stgi /*=-1*/ )  	// first/next chiller in this COOLPLANT stage.
// init ch NULL.  _stgi = 0..6, or -1 for current stage.
// Usage:  for (CHILLER* ch=NULL; nxChStg(ch); ) { ... }
{

// default and check stage number argument
	if (_stgi < 0)  _stgi = this->stgi;				// default to current stage
	if (_stgi < 0  ||  _stgi >= NCPSTAGES)  			// if bad arg given or stgi member not yet set
	{
		rer( PWRN, (char *)MH_R1364,				// "COOLPLANT %s: bad stage number %d: not in range 1..%d"
			 name, (INT)_stgi, INT(NCPSTAGES-1) );		// NCPSTAGES: rccn.h.
		return FALSE;
	}

// access stage's chiller info
	TI* stg = (SI *)((char *)cpStage1 + _stgi * sizeof(cpStage1));	// assumes stage arrays same and in order.

// access next chiller of stage.  Code similar to AH::nxTsCzn, HEATPLANT::nxBlrStg
	TI* chs;							// temp for ptr into stg[]
	switch (*stg)						// first elt of stage array is TI_ALL, TI_ALLBUT, or a ch subscr.
	{
	case TI_ALL:
		return nxCh(ch);			// all chillers in stage: return 1st/next chiller for coolplant
		// CAUTION: disregard rest of TI_ALL cpStage[]: may not be 0's.

	case TI_ALLBUT:
		while (nxCh(ch))			// all but chillers in list: loop ch over all chillers
			for (chs = stg + 1;  ; chs++)	//  loop to reject this chiller if in but-list
				if (*chs==ch->ss)		//  if chiller subsc in but-list is chiller ch's subscript
					break;			//  skip ch, continue outer loop to try next
				else if (*chs <= 0)		//  if end but-list then ch is not in but-list
					return TRUE;			//  so it is a chiller in current stage, say found one
		return FALSE;				// if here, no (more) chillers for stage

	default:	         // list of chiller subscripts for stage, return first or next
		chs = stg;				// init ptr to start of list of chiller subscrs for stage
		if (ch)				// NULL ch means first, else find ch in list, return next
			while (ch->ss != *chs++)		// advance "chs" to entry after that for "ch"
				if ( *(chs-1) <= 0)		// insurance: if past end
					return FALSE;			// say no more chs for stage. **Bug**. Issue msg if need found.
		if (*chs <= 0)  return FALSE;		// if "ch" was last chiller in stage, say no more
		ch = ChB.p + *chs;			// point to next chiller
		return TRUE;				// say found next one
	}
	/*NOTREACHED */

	/*note: COOLPLANT setup code (cncult6.cpp) checks for valid stage info: TI_ALL and TI_ALLBUT 1st only,
		    valid & owned CHILLER subscripts in list, lists 0-terminated.
		    (whole array may be defaulted to TI_ALL; thus here we disregard stuff after TI_ALL.) */

}					// COOLPLANT::nxChStg
//------------------------------------------------------------------------------------------------------------------------------

// end of cncp.cpp
