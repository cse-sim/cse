// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnhp.cpp -- HEATPLANTs and BOILERs runtime code for CSE.  Created 9-92.

// related code: code used only at setup time: cncult6.cpp
//               HW coil model: cncoil.cpp.
//               class declarations: rccn.h, automatically generated from cnrecs.def.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// HEATPLANT BOILER including mbr fcn decls NHPSTAGES HPSTAGESZ
#include <MSGHANS.H>	// MH_R1350

//#include <pp.h>   	//
//#include <psychro.h>	//
//#include <cuparsex.h>	//
#include <EXMAN.H>	// rer
#include <CUL.H>	// oWarn 6-95

#include <CNGUTS.H>	// run basAncs: Top, AhB, BlrB.
// and decls for this file -- no, all are member fcn decls in rccn.h (from cnrecs.def)

/*-------------------------------- DEFINES --------------------------------*/

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/


/******************************************************************************************************************************
*  PLANT: HEATPLANTs and BOILERs
******************************************************************************************************************************/

//------------------------------------------------------------------------------------------------------------------------------
BOO HEATPLANT::hpOn()			// turn on AVAIL heatplant. Loads call this to be sure demand-activated heatPlant is on.

// return value: TRUE if ON.
// callers: cncoil.cpp:doHWCoil; cnztu.cpp:ztuMode; hx should call when implemented
{
	switch (CHN(hpSched))
	{
	case C_OFFAVAILONVC_OFF:
		return FALSE;				// scheduled off, cannot be turned on
		//case C_OFFAVAILONVC_AVAIL:  					// if demand-activated
	default:
		if (hpMode != C_OFFONCH_ON)     	// if not now on
			hpEstimate(TRUE);    		// turn it on now -- save an iteration.
		//fall thru
	case C_OFFAVAILONVC_ON:     					// sched on, should aready be on, check as insurance
		return (hpMode==C_OFFONCH_ON);
	}
	/*NOTREACHED */
}			// HEATPLANT::hpOn
//------------------------------------------------------------------------------------------------------------------------------
RC HEATPLANT::hpEstimate(BOO wantHeat /*=FALSE*/)			// estimate a heat plant: sets mode

// notes: ahEstimate assumes plant is or will be ON when coil is not off and has demand.
//        ahCompute conditionally calls here to turn plant on to save an iteration.
//	  if coil is AVAIL but plant is OFF (an error condition), ok if extra iterations occur.

// called: 1) from hvacSubhrIter, arg FALSE
//	   2) from TU or AH HW coil code, wantHEAT=TRUE, to turn on HEATPLANT (save iteration) (via hpOn).
{
	RC rc = RCOK;
	hpClf++;
	Top.hpKf++;				// call hpCompute at start subhour and when turned on
	OFFONCH hpModeWas = hpMode;				// save hpMode value

// note HEATPLANT has no modeled supply temp, so none is estimated here.

// plant mode changes: do schedule-driven changes and demand-driven turn-on's here. Demand-driven turn-off is done in hpCompute.

	if (CHN(hpSched)==C_OFFAVAILONVC_OFF)		// if scheduled OFF
		hpMode = C_OFFONCH_OFF;				// turn the heatplant OFF if it was not OFF
	else if (CHN(hpSched)==C_OFFAVAILONVC_ON)  		// if schedule is unconditionally ON
		hpMode = C_OFFONCH_ON;				// turn the heatplant ON if it was not ON
	else if (CHN(hpSched)==C_OFFAVAILONVC_AVAIL)		// if demand-driven on-off (redundant test)
		if (wantHeat)					// if caller says there is demand
			hpMode = C_OFFONCH_ON;			// turn the heatplant ON if it was not ON

// if mode changed, flag loads to recompute

	if (hpMode != hpModeWas)
	{
		// note capF not changed: overload at turn-on assumed same as when last went off.

		for (TU* tu=NULL; nxTu(tu); ) 			// loop TU's with HW coils served by this HEATPLANT
		{
			if (tu->tuhc.qWant > 0.)			// if coil wanted heat from plant when last computed
			{
				ZrB.p[tu->ownTi].ztuCf++;    		// say recompute terminals for zone containing terminal
				Top.ztuKf++;				// say look for zones whose therminals to recompute
			}
		}
		for (AH* ah=NULL; nxAh(ah); ) 			// loop AH's with HW coils served by this HEATPLANT
		{
			if (ah->ahhc.qWant > 0.)			// if coil wanted heat from plant when last computed
			{
				ah->ahPtf++;				// say recompute this air handler
				Top.ahKf++;				// say check for airhandlers to recompute
			}
		}
		//for (HPLOOP *hl=NULL; nxHl(hl); )   		// loop HPLOOP's with HX's served by this HEATPLANT
		//{  if (hl->qWant > 0.)
		//   {  hl->hlPtf++;
		//      Top.hlKf++;
		//}  }
	}
	return rc;
}		// HEATPLANT::hpEstimate
//------------------------------------------------------------------------------------------------------------------------------
RC HEATPLANT::hpCompute()		// conditionally compute heatplant

// Top.ztuKf, Top.ahKf, Top.hlKf set TRUE if computated & results changed so as to affect loads (eg degree of overload changed)
// 9-92 NOT set if load is met or overload unchanged (note mode changes that affect load done in hpEstimate).
{
	RC rc = RCOK;
	//hpEstimate has done: changed mode if required by hpSched or requested by load.

// determine load if on. Note: if hpSched is AVAIL, loads have used hpEstimate(TRUE) to turn on HEATPLANT if necessary.
	DBL demand = 0.;				// for accumulating coil loads
	if (hpMode==C_OFFONCH_ON)
	{
		for (TU* tu=NULL; nxTu(tu); ) 		// loop TU's with HW coils served by this HEATPLANT
			demand += tu->tuhc.q;			// accumulate heat used by coil from plant
		for (AH* ah=NULL; nxAh(ah); ) 		// loop AH's with HW coils served by this HEATPLANT
			demand += ah->ahhc.q;			// accumulate heat used by coil from plant
		//for (HPLOOP *hl=NULL; nxHl(hl); )   	// loop HPLOOP's with HX's served by this HEATPLANT
		//{  demand += hl->hxQ;			// accumulate heat coil used by hx from plant
		//}
		qNx = demand + qPipeLoss;		// add pipe loss to coil/hx demand to get next total heatplant load
	}
	else
		qNx = 0.;    				// no load if off

// if no demand, turn AVAIL heatplant OFF
	if (CHN(hpSched)==C_OFFAVAILONVC_AVAIL  &&  demand <= 0.)	// if schedule says on-off is per demand and no demand
	{
		hpMode = C_OFFONCH_OFF;					// turn off
		qNx = 0.;    						// load is 0 (not qPipeLoss) when off
		// falls thru to set q from qNx if wasn't already off
		stgi = 0;					// revert to lowest stage / redetermine stage at turn-on (no hysteresis)
	}

// test conditions requiring recomputation
	if (hpPtf)  ;						// skip tests if must-compute flag already set
	else if (hpMode != hpModePr)  hpPtf++;			// if mode changed, must compute
	else //if (hpMode==C_OFFONCH_ON)				[if on and]
		if (RELCHANGE(qNx,qPr) >= Top.relTol) hpPtf++;		// if load (same as output) changed, must compute

// exit if no recompute required
	if (!hpPtf)
		return rc;

// compute HEATPLANT

	q = qNx;							// commit to recalc: copy "next" variable to "current"
	for (BOILER* blr=NULL; nxBlr(blr); ) 			// init all boilers off, 0 output
	{
		blr->blrMode = C_OFFONCH_OFF;
		blr->q = 0.;
	}

	if (hpMode==C_OFFONCH_ON)
	{
		// handle under/overload: adjust capacity avail to each load
		if ( q >= stgCap[stgMxQ] * Top.hiTol			// if load > largest stage capacity
				||  capF < 1.0  &&  q <= stgCap[stgMxQ] * Top.loTol )	// or capacities reduced & load < capac
		{
			setToMax( qPk, (float)q);				// record peak q since fazInit() re autoSize overload check 6-95.
			adjCapF();						// adjust capF, update q, below.
		}

		// determine stage if necessary. if 0 load gets here, run stage 1. Here load AND stgCap include pump heats.
		if ( stgi < 0  ||  stgi >= stgN	 				// if don't have valid stage
				||  RELCHANGE(stgCap[stgi], q) > Top.relTol )			// or its power does not match load (with hysteresis)
		{
			for (stgi = 0;  stgi < stgN && stgCap[stgi] < q;  stgi++)		// look for first stage that can meet load.
				;									// unused stages (stgCap[stgi]==0) expected.
			if (stgi >= stgN)							// if didn't find one
			{
				stgi = stgMxQ;							// use most powerful stage
				// due to tolerance in decrCapF it is normal to get here with "tol" excess load.
				if (q > stgCap[stgi] * Top.hiTol)					// if overload by more than tol, issue message
					rer( PWRN, 							// continue for now; errors end run elsewhere.
						 (char *)MH_R1350, name, q, stgCap[stgi] );	// "heatPlant %s overload failure: q (%g) > stgCap[stgMxQ] (%g)"
			}
		}

		// boilers in use: turn them on, apportion load
		DBL qNet = max( q - stgPQ[stgi], 0.);		// load less pump heat, 0 if pump(s) alone meet load
		DBL qF =     					// fraction of stage's blr power to use: for apportioning load to boilers
			qNet / (stgCap[stgi] - stgPQ[stgi]);	//  = net load / stage boiler capacity (subtract pump heat)
		for (blr=NULL; nxBlrStg(blr); )			// loop boilers in current stage (stgi)
		{
			blr->blrMode = C_OFFONCH_ON;			// turn boiler on (preset OFF above)
			blr->q = blr->blrCap*qF;			// output is required fraction of capacity
		}

		// flag loads whose outputs have been changed by this computation
		//   OTHER load affects: sched turn-on/off: hpEstimate does, thus already seen by loads if here
		//                       demand turn-on:    1st load with demand invokes re-estimate, thus already seen by loads
		//                       demand turn-off:   no demand --> does not affect loads.
		if (!Top.tp_pass1)		// during autoSizing, loads don't respond to capF, so don't flag to save time 7-95.
		{
			for (TU* tu=NULL; nxTu(tu); )     		// loop TU's with HW coils served by this HEATPLANT
			{
				tu->tuhc.qPr = tu->tuhc.q;			// remember coil q at hp last computed, re change detection
				if (capF != capFPr)   				// for speed -- else believe no change possible
					tu->tu_BegHour();				// tu beg hour fcn (cnztu.cpp) recomputes lhMn/Mx per capF, and
             										//   sets zp->ztuCf,Top.ztuKf if terminal lh output will change.
			}
			// note: ah's differ from tu's in that they have no min coil q, and store no values dependent on capF.
			for (AH* ah=NULL; nxAh(ah); )				// loop AH's with HW coils served by this HEATPLANT
			{
				ah->ahhc.qPr = ah->ahhc.q;					/* ahhc q used for this hp computation:
   									   if changes, cncoil.cpp:AH::doHWCoil call-flags this hp*/
				if (capF != capFPr)   					// if max capacities changed -- for speed
				{
					if ( ah->ahhc.q    					// if coil's heat input/output not consistent with
							!= min( ah->ahhc.qWant, ah->ahhc.captRat * capF) )	// its current max capacity
					{
						Top.ahKf++;						// tell caller to recheck AH's for flagged ones
						ah->ahPtf++;						// say recompute this AH: its supply temp will change
					}
				}
			}
			//for (HPLOOP *hl=NULL; nxHl(hl); )   		// loop HPLOOP's with HX's served by this HEATPLANT
			//{  hl->xxx.qPr = hl->xxx.q					// for change-detection & hp-call-flagging in hploop code
			//   if (capF != capFPr)   					// (for speed)
			//   {  if (q not consistent with (changed) current max)
			//      {  Top.hlKf++;    					// tell caller to recheck hploop's for flagged ones
			//         hl->hlPtf++;    					// say recompute this hpl: its supply temp will change
			//}  }  }
		}
	}	// if hpMode==ON
	//else: OFF heatplant: all boilers pre-set to off above; ah's & hx's were flagged by hpEstimate when hp went off.

// update priors, clear flags
	hpModePr = hpMode;
	qPr = q;
	capFPr = capF;
	hpPtf = FALSE;				// say no recompute needed
	hpClf = FALSE;				// say no call required

	return rc;
}				// HEATPLANT::hpCompute
//------------------------------------------------------------------------------------------------------------------------------
void HEATPLANT::adjCapF()	// adjust load capacities to match load to HEATPLANT capacity. updates q. called conditionally.
{
	do       				// repeat to adjust load enuf: necess unless all loads already were & remain max'd.
	{
		DBL qWas = q;
		// increase or decrease capF. Only changes max'd loads --> q usually changes less.
		capF *= stgCap[stgMxQ]/q;   			// change each load capacity by load excess/shortage fraction
		setToMin( capF, 1.0);				// do not incr 1.0 = full capacities (& 1.0 exits loop below)
		/* compute what q will be for this capF.  Do not store in tu's/ah's/hx's here.
		   tu's/ah's for which coil q changes are flagged by hpCompute -- after convergence here. */
		q = qPipeLoss;
		for (TU* tu=NULL; nxTu(tu); ) 				// loop TU's with HW coils served by this HEATPLANT
			q += min(tu->tuhc.qWant, tu->tuhc.captRat * capF);	// accumulate what load will be
		for (AH* ah=NULL; nxAh(ah); ) 				// loop AH's with HW coils served by this HEATPLANT
			q += min(ah->ahhc.qWant, ah->ahhc.captRat * capF);	// accumulate what load will be
		//for (HPLOOP *hl=NULL; nxHl(hl); )   			// loop HPLOOP's with HX's served by this HEATPLANT
		//{
		//  ...
		//}
		// go directly to full capacities if underloaded & no loads now at (current reduced) max
		if (q==qWas)					// if load did not change, no loads are pegged
			if (q < stgCap[stgMxQ])  			// if underloaded
				capF = 1.0;   				// restore full capac's to end loop & prevent recall til overload occurs
	}
	while (    q >= stgCap[stgMxQ] * Top.hiTol
			   || q <= stgCap[stgMxQ] * Top.loTol && capF < 1.0 );
}								// HEATPLANT::adjCapF
//------------------------------------------------------------------------------------------------------------------------------
#if 0
for (TU* tu=NULL; nxTu(tu); ) 		// loop TU's with HW coils served by this HEATPLANT
{
}
for (AH* ah=NULL; nxAh(ah); ) 		// loop AH's with HW coils served by this HEATPLANT
{
}
//for (HPLOOP *hl=NULL; nxHl(hl); )   	// loop HPLOOP's with HX's served by this HEATPLANT
//{
//}
#endif
//------------------------------------------------------------------------------------------------------------------------------
// RC HEATPLANT::endSubhr(): nothing to do, but see boiler::endSubhr.
//------------------------------------------------------------------------------------------------------------------------------
void HEATPLANT::fazInit(int isAusz)	// call at beginning of (main simulation and) autoSizing phases

{
	// called from cnguts.cpp:cgFazInit.

	qPk = 0;			// init peak load

	if (isAusz)			// -As variables should last through main simulation for (possible future) report
		qPkAs = 0;		// init qPk converged-autoSize-design day peak re autoSize overload error message
}
//------------------------------------------------------------------------------------------------------------------------------
void HEATPLANT::rddiInit()	// init done ONCE for main sim or EACH (pass 2) DES DAY ITERATION for ausz: clear peaks. 6-95.

{
	// called from cnausz:asRddiInit after calling AUSZ::rddiInit's, 7-95.
#if 0 // do clear peaks each pass 1 des day iter for more accurate lkPkAs1 re sfan, 8-15-95 cnausz, cnah1, cnztu.cpp.
	x		// not called per des day iteration in pass 1, 7-95. Redundant calls occur.
#else
	// redundant calls occur & must be harmless.
#endif
	qPk = 0;				// init peak load, re autoSize overload warning & possible future reports, 6-95.
}		// HEATPLANT::rddiInit
//-----------------------------------------------------------------------------------------------------------------------------
void HEATPLANT::endP2Dsd()	// called at end pass 2 design day, after converged. records new high loads & plr's not in AUSZ's.
{
// record non-AUSZ-member autosizing peak value(s)
	setToMax( qPkAs, qPk);    				// record autoSizing peak load re overload warning
}				// HEATPLANT::endP2Dsd
//-----------------------------------------------------------------------------------------------------------------------------
RC HEATPLANT::hp_endAutosize()		// call at end successful autoSize for possible additional checks/messages

// (note heatPlants not autoSizable 6-95, but check for being able to support load of coils during autoSize).

{
	// called from cnausz.cpp:cgAuszI().

// autoSize overload warning -- help user identify bad input
	if (qPkAs > stgCap[stgMxQ] * Top.hiTol)
		oWarn( "Maximum load during autoSizing (%g) greater than capacity (%g).\n"
			   "    AutoSized HW coils may be ineffective during main simulation run\n"
			   "    using this undersized heatPlant capacity.",				// NUMS
			   qPkAs, stgCap[stgMxQ] );
	return RCOK;
}			// HEATPLANT::hp_endAutosize
//------------------------------------------------------------------------------------------------------------------------------
RC BOILER::endSubhr() 	// end subhour computations for boiler: input energies
{
	if (blrMode==C_OFFONCH_ON)
	{
		plr = q/blrCap;						// part load ratio (fraction of full load)
		p = blrCap * blrEirR * blrPyEi.val(plr);  		// boiler input power
		if (mtri)						// if meter specified
			MtrB.p[mtri].H.htg += p * Top.tp_subhrDur;		// charge energy to meter, "htg" category
		if (blrp.mtri)						// if meter specified for boiler primary pump
			MtrB.p[blrp.mtri].H.aux += blrp.p * Top.tp_subhrDur;	// charge pump energy to meter, "aux" category
		pAuxFullOff = 0.;					// none of this when on at all in subhour
		pAuxOnAtall = auxOnAtall;				// charge all of this aux power if blr ran at all in subhour
		if (auxOnAtallMtri)					// aux energy when on at all in subr
			MtrB.p[auxOnAtallMtri].H.clg += pAuxOnAtall*Top.tp_subhrDur;
	}
	else		// boiler is off
	{
		plr = 0.;						// fraction of full load: 0
		p = 0.;							// power input: 0, right?
		pAuxOnAtall = 0.;					// none of this aux energy when off for entire subhour
		pAuxFullOff = auxFullOff;				// charge this aux energy only when not used in entire subhour
		if (auxFullOffMtri)					// aux energy when off entire subhr
			MtrB.p[auxFullOffMtri].H.htg += pAuxFullOff*Top.tp_subhrDur;
	}
	pAuxOn = plr * auxOn;					// off-proportional aux power: plr * user input
	if (auxOnMtri)						// load-proportional aux energy
		MtrB.p[auxOnMtri].H.clg += pAuxOn*Top.tp_subhrDur;
	pAuxOff = (1. - plr) * auxOff;				// off-proportional aux power
	if (auxOffMtri)						// boiler-off proportional aux energy
		MtrB.p[auxOffMtri].H.htg += pAuxOff*Top.tp_subhrDur;
	return RCOK;
}				// BOILER::endSubhr
//------------------------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------------------------
BOO HEATPLANT::nxTu( TU *&tu)		// first/next terminal with HW coil served by this HEATPLANT.
// init tu NULL.   Usage:  for (TU* tu=NULL; nxTu(tu); ) { ... }
{
	TI tui = tu ? tu->tuhc.nxTu4hp : tu1;
	if (tui)
	{
		tu = TuB.p + tui;
		return TRUE;
	}
	return FALSE;
}
//------------------------------------------------------------------------------------------------------------------------------
BOO HEATPLANT::nxAh( AH *&ah)		// first/next air handler with HW coil served by this HEATPLANT.
// init ah NULL.   Usage:  for (AH* ah=NULL; nxAh(ah); ) { ... }
{
	TI ahi = ah ? ah->ahhc.nxAh4hp : ah1;
	if (ahi)
	{
		ah = AhB.p + ahi;
		return TRUE;
	}
	return FALSE;
}
//------------------------------------------------------------------------------------------------------------------------------
BOO HEATPLANT::nxBlr( BOILER *&blr)  	// first/next boiler in this HEATPLANT.
// init blr NULL.   Usage:  for (BOILER* blr=NULL; nxBlr(blr); ) { ... }
{
	TI blri = blr ? blr->nxBlr4hp : blr1;
	if (blri)
	{
		blr = BlrB.p + blri;
		return TRUE;
	}
	return FALSE;
}

//------------------------------------------------------------------------------------------------------------------------------
BOO HEATPLANT::nxBlrStg( BOILER *&blr, SI _stgi /*=-1*/ )  	// first/next boiler in this HEATPLANT stage.
// init blr NULL.  _stgi = 0..6, or -1 for current stage.
// Usage:  for (BOILER* blr=NULL; nxBlrStg(blr); ) { ... }
{

// default and check stage number argument
	if (_stgi < 0)  _stgi = this->stgi;					// default to current stage
	if (_stgi < 0  ||  _stgi >= NHPSTAGES)  			// if bad arg given or stgi member not yet set
	{
		rer( PWRN, (char *)MH_R1351, name, _stgi, NHPSTAGES-1);	// "heatPlant %s: bad stage number %d: not in range 0..%d"
		return FALSE; 							// NHPSTAGES: rccn.h.
	}

// access stage's boiler info
	TI* stg = (SI *)((char *)hpStage1 + _stgi * sizeof(hpStage1));	// assumes stage arrays same and in order.

// access next boiler of stage.  Code similar to AH::nxTsCzn.
	TI* blrs;							// temp for ptr into stg[]
	switch (*stg)						// first elt of stage array is TI_ALL, TI_ALLBUT, or a blr subscr.
	{
	case TI_ALL:
		return nxBlr(blr);			// all boilers in stage: return 1st/next boiler for heatplant
		// CAUTION: disregard rest of TI_ALL hpStage[]: may not be 0's.

	case TI_ALLBUT:
		while (nxBlr(blr))			// all but boilers in list: loop blr over all boilers
			for (blrs = stg + 1;  ; blrs++)	//  loop to reject this boiler if in but-list
				if (*blrs==blr->ss)		//  if boiler subsc in but-list is boiler blr's subscript
					break;			//  skip blr, continue outer loop to try next
				else if (*blrs <= 0)		//  if end but-list then blr is not in but-list
					return TRUE;			//  so it is a boiler in current stage, say found one
		return FALSE;				// if here, no (more) boilers for stage

	default:	         // list of boiler subscripts for stage, return first or next
		blrs = stg;				// init ptr to start of list of boiler subscrs for stage
		if (blr)				// NULL blr means first, else find blr in list, return next
			while (blr->ss != *blrs++)		// advance "blrs" to entry after that for "blr"
				if ( *(blrs-1) <= 0)		// insurance: if past end
					return FALSE;			// say no more blrs for stage. **Bug**. Issue msg if need found.
		if (*blrs <= 0)  return FALSE;		// if "blr" was last boiler in stage, say no more
		blr = BlrB.p + *blrs;			// point to next boiler
		return TRUE;				// say found next one
	}
	/*NOTREACHED */

	/*note: HEATPLANT setup code (cncult6.cpp) checks for valid stage info: TI_ALL and TI_ALLBUT 1st only,
		    valid & owned BOILER subscripts in list, lists 0-terminated.
		    (whole array may be defaulted to TI_ALL; thus here we disregard stuff after TI_ALL.) */

}					// HEATPLANT::nxBlrStg

// end of cnhp.cpp
