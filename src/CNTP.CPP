// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cntp.cpp -- TOWERPLANTs runtime code for CSE.  Created 10-92.

// related code: code used only at setup time: cncult6.cpp
//               COOLPLANTs: cncp.cpp.
//               class declarations: rccn.h, automatically generated from cnrecs.def.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <MSGHANS.H>	// MH_R1370
#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// COOLPLANT CHILLER including mbr fcn decls NHPSTAGES HPSTAGESZ

#include <PSYCHRO.H>	// psySatEnthalpy psySatEnthSlope psyTWSat
#include <EXMAN.H>	// rer

#include <CNGUTS.H>	// run basAncs: Top, TpB.
// and any decls for this file that are not member fcns, decl'd in rccn.h (from cnrecs.def)


/*-------------------------------- DEFINES --------------------------------*/
#define CPW 1.0   	// specific heat of liquid water, Btu/lb. Many not be explicit everwhere it otta be.

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/


//------------------------------- DEBUG AID --------------------------------
#ifdef DEBUG2
void FC endTp() { }		// bkpt place, called from coolplants & towerplants when decide NOT to compute or flag loads
#endif

/******************************************************************************************************************************
*  PLANT: TOWERPLANTs
******************************************************************************************************************************/

//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::tpEstimate()		// estimate tpTs at start subhour: tentative value for plants to use if executed before towers.
{
	RC rc=RCOK;
	tpClf++;
	Top.tpKf++;		// if estimated, call tpCompute: start each subhour.

// note TOWERPLANT has no mode changes (on/off), so we don't estimate mode here.

// Estimate supply temp

	// 1. use setpoint if changed
	if (tpTsSp != tpTsSpPr)			// no tolerance: user input, trivial changes not expected
		tpTs = tpTsSp;
	// 2. if no flow, conditionally revert to setpoint for when flow comes on
	if (mwAll <= 0.)				// if no flow in previous subhour
		if (tpTs > tpTsSp)			// if last supply temp hier than setpoint: overloaded (or changed setpoint)
			tpTs = tpTsSp;			// revert to setpoint, ie assume no overload when flow next comes on
	// 3. else leave supply temp unchanged: is setpoint, or differs due to overload or underload; current is best starting point.

// if estimated supply temp changed, flag plant, and loads with non-0 flows, to recompute

	if (ABSCHANGE( tpTs, tpTsEstPr) >= Top.absTol)			// if close, don't bother
	{
		tpPtf++;												// recompute this plant
		for (COOLPLANT* cp = NULL; nxCp(cp); ) if (cp->mwTow > 0.)
			{
				cp->cpPtf++;     // recompute cp
				Top.cpKf++;
			}
		//for (HPLOOP* hl = NULL;  nxHl(hl); ) if (hl->___ (flow) > 0.) { hl->hlPtf++;  Top.hlKf++; }	// recompute hploop
	}

// update priors
	tpTsSpPr = tpTsSp;
	tpTsEstPr = tpTs;
	return rc;
}				// TOWERPLANT::tpEstimate
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::tpCompute()				// conditionally compute a TOWERPLANT

// contains during-iteration part of towerplant model: supply temp.

// arguments set nz if caller should continue iteration & recheck coolplants/hploops, else args not changed.

/* does only enuf to determine values affecting loads (delivered water temp) for current conditions;
   rest of computation including staging details when setpoint can be met deferred for speed
   to TOWERPLANT::endSubhr, after zones-coils-plants-towers iteration converged. */

// runtime inputs from loads: COOLPLANT.___, HPLOOP.____ (return temp, water flow, ??rejected heat)
// outputs: for loads:    .tpTs
//          for caller:   Top.cpKf, Top.hlKf set if ts changed and may impact a coolplant/hploop
//          and:          .cpPtf++/.hlPtf++ for each affected coolplant/hploop.
//          for endSubhr: .towldCase, .qMin1, .qMax1
// uses: TOWERPLANT::towModel.
{
	RC rc=RCOK;

// determine computation inputs: returned water flow and temp; also load heat

#if 0 // by return temp only: original spec, use if in doubt
*	mwAllNx = 0.; 	DBL mwt = 0.;
*	for (COOLPLANT* cp = NULL; nxCp(cp); )
*	{	mwAllNx += cp->mwTow;
*		mwt += cp->tTow * cp->mwTow;
*	}
*    //for (HPLOOP* hl = NULL;  nxHl(hl); )   {  mwAllNx += hl->____;  mwt += hl->_____* hl->___: temp*flow;  }
*    if (mwAllNx != 0.)  trNx = mwt / mwAllNx;
*    qLoadNx = (trNx - tpTs) * mwAllNx;
#elif 1 // by heats only. assumes tpTs unchanged (or flag tpPtf set) (probably safe assumption)
	mwAllNx = 0.;
	qLoadNx = 0.;
	for (COOLPLANT* cp = NULL;  nxCp(cp); )
	{
		mwAllNx += cp->mwTow;
		qLoadNx += cp->qTow;
	}
	//for (HPLOOP* hl = NULL;  nxHl(hl); )    {  mwAllNx += hl->____;  qLoadNx += hl->_____; }
	//trNx = tpTs + qLoadNx / mwAllNx;   is done after 0-flow exit to not /0
#else // both. looks slowest/worst.
*   mwAllNx = 0.;	qLoadNx = 0.; DBL mwt = 0.;
*   for (COOLPLANT* cp = NULL;  nxCp(cp); )
*	{	mwAllNx += cp->mwTow;
*		qLoadNx += cp->qTow;
*		mwt += cp->tTow * cp->mwTow;
*	}
*   //for (HPLOOP* hl = NULL;  nxHl(hl); )    {  mwAllNx += hl->____;  qLoadNx += hl->_____;  mwt += hl->_____* hl->___: temp*flow;}
*   if (mwAllNx != 0.)  trNx = mwt / mwAllNx;
*   // consistency check qLoadNx and trNx?
#endif

// determine whether computation necessary, exit if not

	// if no flow, no computation needed, exit now
	if (mwAllNx <= 0.)
	{
		towldCase = tldMIN;
		nCtOp = 0;				// tell endSubhr: no power, no fans running
		mwAll = mwAllNx;
		qLoad = qLoadNx;			// "next" values are now current; added 10-19-92
		q = 0.;							// power output is 0, 10-19-92
		//tpTs (for when demand resumes) remains unchanged here, but tpEstimate may change it.
		return rc;
	}
	// test for changes in inputs
	trNx = tpTs + qLoadNx / mwAllNx;				// safe to divide after 0-flow exit
	if (tpPtf)  ;						// skip tests if must-compute flag already set
	else if (RELCHANGE(trNx-tpTs, tr-tpTs) >= Top.relTol)  tpPtf++;	// if return temp rel to sup temp changed signif, compute:
	// delta-t change is proportional to delta-q change.
	else if (RELCHANGE(mwAllNx, mwAll)     >= Top.relTol)  tpPtf++;	// if water flow changed "significantly", must compute
	//else if (...) tpPtf++;
	//power use change: covered by flow and temp change checks.
	// no if no change
	/* watch for changes in weather, cuz towModel uses tWbOSh, hOSh.
	   Added 1-95 at addition of SHINTERP, may actually be covered some other way that I don't see today.
	   TESTED (file T81, SHINTERP undef, 1-95): changed a few tp details in probe report without changing zone or ah results. */
#if 1
	else if (ABSCHANGE(Top.tDbOSh, twr_tDbOShPr)    >= Top.absTol)     tpPtf++;
	else if (ABSCHANGE(Top.wOSh, twr_wOShPr)        >= Top.absHumTol)  tpPtf++;
#else//tried this then decided tolerances inappropriate
	x   else if (RELCHANGE(Top.tWbO, tWbOPr)   >= Top.relTol)  tpPtf++;
	x   else if (RELCHANGE(Top.hO, hOPr)       >= Top.relTol)  tpPtf++;
#endif
	if (!tpPtf)
	{
#ifdef DEBUG2 //cnglob.h
		endTp();  						// cntp.cpp: place to put breakpoint to reach at end of iteration
#endif
		return rc;
	}

// commit to new computation: update input variables

	qLoad = qLoadNx;
	tr = trNx;
	mwAll = mwAllNx;	// copy "next" values (computed just above) to "current" values
	mwi1 = mwAll / ctN;					// incoming water flow per tower (less may flow out: evap, drift)
	qNeed = (tpTsSp - tr) * mwAll;			// power needed from tower plant to deliver water at setpoint

	/* TOWERPLANT staging (Niles VI-C-2-b), part 1: determine delivery temp (tpTs) and case (towldCase):
		tldMIN: fans off, tpTs <= setpoint on stack effect only (or no water flow).
		tldSP:  tpTs is setpoint, fan(s) on.
		tldMAX: all fans at full speed, tpTs >= setpoint.
	   After completion of towers-plants-coils-zones iteration, TOWERPLANT::endSubhr determines # fans used, speed, and power input.*/

	towModel( 1.0, qMax1, qMaxGuess);			// determine max power of 1 tower (negative)
	if (ctN * qMax1 >= qNeed)				// if cannot meet, or can just meet, load (negative)
	{
		tpTs = tr + qMax1/mwi1;				// use full speed delivery temp
		towldCase = tldMAX;				// say full power
		nCtOp = ctN;					// all tower fans running
	}
	else					// towerplant not max'd out
	{
		towModel( 0., qMin1, qMinGuess);			// min power of 1 tower, fan off (stack effect only)
		if (ctN * qMin1 <= qNeed)			// if towers meet or exceed (negative) load with fans off
		{
			tpTs = tr + qMin1/mwi1;			// compute delivery temp, fans off
			towldCase = tldMIN;				// say no power
			nCtOp = 0;					// no fans running
		}
		else					// towerplant between 0 and max power
		{
			tpTs = tpTsSp;				// will deliver setpoint temp. determine nCtOp and f in endSubr.
			towldCase = tldSP;				// qMin1 and qMax1 are set for determining f and fanP in endSubr
		}
	}
	puteTs = tpTs;					// save computed ts where tpEstimate won't overwrite it: debug aid.
	q = (tpTs - tr) * mwAll;				// power output, for change-detection & (future) reports

// for each load, if supply temp changed significantly relative to loads's return temp, flag load to be recomputed

	for (cp = NULL;  nxCp(cp); )   					// loop coolplants
	{
		cp->mwTowPr = cp->mwTow;							// update priors used to call-flag tp from cp
		cp->tTowPr = cp->tTow;							// ..
		if (cp->mwTow > 0.)   							// if active as a load
			if (RELCHANGE( tpTs - cp->tTow, cp->tCnd - cp->tTow) >= Top.relTol)	// if ts changed rel to cp return temp to tp
			{
				Top.cpKf++;							// tell caller to recheck coolplants
				cp->cpPtf++;							// tell cpCompute to recompute this coolplant
			}
#ifdef DEBUG2
			else
				endTp();					// cntp.cpp: place to put breakpoint to reach at possible end of iteration
#endif
	}
	//for (hl = NULL;  nxHl(hl); )
	//{  priors
	//   if (hl active -- flow > 0.)
	//      if (RELCHANGE( tpTs - hl return temp, hl's prior ts - hl return temp) >= Top.relTol)
	//      {  Top.hlKf++;							// tell caller to recheck HPLOOPs
	//         hl->hlPtf++;							// tell hlCompute to recompute this HPLOOP
	//      }
	//#ifdef DEBUG2
	//      else
	//         endTp();					// cntp.cpp: place to put breakpoint to reach at possible end of iteration
	//}
	//#endif

// update priors. Also, cp priors updated in change-check loop above.
	twr_tpTsPr = tpTs;		// unused?
	twr_tDbOShPr = Top.tDbOSh;
	twr_wOShPr = Top.wOSh;

// clear change flags
	tpPtf = FALSE;
	tpClf = FALSE;

	return rc;						// more returns above
}		// TOWERPLANT::tpCompute
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::endSubhr()	// towers end-subhour computations: fraction on, # towers running, input power.

// runtime inputs used include: towldCase, qNeed, qMin1, qMax1, .
// outputs: nCtOp, f, fanP; fanP is posted to tpMtr.
{
	RC rc=RCOK;

	if (tpTs <= 32.)								// Farenheight assumed!
		rc |= rer( (char *)MH_R1370, name, tpTs);				// "Frozen towerPlant '%s': supply temp tpTs = %gF"
	//check return... no, check returned water in each load. then delete here:
	if (tpTs >= 212.)								// Farenheight assumed!
		rc |= rer( (char *)MH_R1371, name, tpTs);				//"Boiling towerPlant '%s': supply temp tpTs = %gF"

	// add any computations or result recording not needed to determine tpTs ...

// determine # fans running (nCtOp) & fraction of full speed (f) for interest, and fan power consumption (.fanP) for output.

	switch (towldCase)					// load case, saved by TOWERPLANT::tpCompute
	{
	case tldMIN:
		nCtOp = 0;
		f = 0.;
		fanP = 0;
		break;		// don't need to run fans: stack effect enuf, or no flow

	case tldMAX:
		nCtOp = ctN;
		f = 1.;
		fanP = nCtOp * oneFanP;
		break;	// all fans running at full power (change names?)

	case tldSP:			// at setpoint -- fans running at intermediate power

		if (tpStg==C_TPSTGCH_LEAD)			// lead tower staging
		{
			DBL qx = qNeed - ctN * qMin1;		// addl power needed after stack effect of all towers
			nCtOp = 1;					// assume 1 tower (the lead tower) fan running
			while (qx < qMax1)				// while (negative) load is too great for one (the lead) tower
			{
				nCtOp++;
				qx -= (qMax1 - qMin1);	// add full-speed towers
			}
			CSE_E( findSpeed(qx) );			// set .f, .fanpwr for lead tower to produce remaining qx
			fanP += (nCtOp - 1) * oneFanP;		// add fan power for the full-speed towers
		}
		else					// all towers running together
		{
			nCtOp = ctN;				// all fans run in this staging mode
			CSE_E( findSpeed(qNeed/ctN) );			// determine speed f so each tower produces its share; sets .fanP.
		} // if (tpStg==LEAD) ... else ...

	} // switch (towldCase)

#if 0 //to tpCompute
	x// compute tp power output -- for probes, debugging, possible use in reports. 10-19-92.
	x
	x    q = (mwAll <= 0.)  ?  0.  :  (tpTs - tr) * mwAll;		// 0 if no flow (temps unset), else (negative) power
#endif

// charge energy consumption .fanP to meter, if meter given

	if (tpMtr)   MtrB.p[tpMtr].H.aux += fanP * Top.tp_subhrDur;

	return rc;
}			// TOWERPLANT::endSubhr
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::findSpeed( DBL _q)	// determine speed necessary to produce q from 1 tower; set members f and fanP.

// runtime inputs used include: qMin1, qMax1, .
// called only for _q between 0 and max possible for plant. 2 calls in TOWERPLANT::endSubhr.
{
	RC rc=RCOK;
	switch (ctTy)				// cases by fan type to determine fraction full speed, f.
	{
	case C_CTTYCH_ONESPEED:   		// single speed fan
		f = (_q - qMin1)/(qMax1 - qMin1);    		// fan cycles off and on; f is fraction on. q's negative.
		fanP = ctFcOne.val(f);			// fan power input per polynomial
		break;

	case C_CTTYCH_TWOSPEED:   		// two speed fan
		DBL qLo1;
		towModel( ctLoSpd, qLo1, qLoGuess); 		// determine heat q (normally negative) for fan at low speed
		if ( qLo1 <= _q)				// if lo speed meets load
		{
			f = ctLoSpd*(_q - qMin1)/(qLo1 - qMin1); 	// 0 < f <= ctLoSpd. fan cycling between off and low
			fanP = ctFcLo.val(f);			// fan input power per low speed polynomial
		}
		else								// hi speed needed
		{
			f = ctLoSpd + (1. - ctLoSpd)*(_q - qLo1)/(qMax1 - qLo1);	// ctLoSpd < f < 1.  low-hi cycling.
			fanP = ctFcHi.val(f);					// fan input power per hi speed polynomial
		}
		break;

	case C_CTTYCH_VARIABLE:   		// variable speed fan
		CSE_E( varSpeedF( _q, f, qVarTem, qVarGuess) );	/* search for rel speed f that delivers heat q. only varSpeedF call.
          						   At call, f contains last subhr value: starting point for search */
		fanP = ctFcVar.val(f);			// determine fan power at speed f per polynomial
		break;
	}
	return rc;
}			// TOWERPLANT::findSpeed
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::varSpeedF( 	// determine f needed for one tower to output power qWant

	DBL qWant, 				// desired heat (negative)
	DBL &_f,				// fraction full fan speed at entry (start point for search) and at exit: returned value
	DBL &_q,				// temp that caller should save between calls, for each place called (holds q for last f)
	FLOAT &guess )			// ditto ('guess' for towModel()).

// runtime inputs used include: qMin1, qMax1, .
// called only for _q between 0 and max possible for 1 tower. 1 call in TOWERPLANT::findSpeed.
{
	RC rc=RCOK;

// check for qWant out of range (unexpected; prevent inf loop), or towers not cooling the water (prevent failure and/or /0 below)
	if ( qMax1 >= qMin1						// if towers not cooling water
			||  qWant <= qMax1 )
	{
		_f = 1.;     // if requested (negative) heat >= precomputed max possible
		_q = qMax1;
		return rc;
	}
	if (qWant >= qMin1)
	{
		_f = 0.;     // if requested (negative) heat <= precomputed min possible
		_q = qMin1;
		return rc;
	}

// initial trial f: when possible, interpolate from prior f and q for fast repeated calls in same subhr
// CAUTION: old q will not corres to old f if weather, setpoint, etc changed, so never save initial point.
	if (_q > qMax1 || _q < qMin1)					// /0 and major change protection
		if (_q <= qWant) _f  =  _f * (qWant - qMin1)/(_q - qMin1);	// if prior q too big (negative), linear inter tween qMin1 and q
		else   	        _f += (1. - _f)*(qWant - _q)/(qMax1 - _q);	// else prior q too small (neg), interp tween q and qMax1
	// else: fall thru with old f as 1st trial. f < 0 and f > 1 fixed below.

#if 1	// believe this is faster than next, provided towModel is stable enuf to assure convergence. 9-92.

// search loop. secant method: inter/extrapolate from most recent two points

	DBL f1=0., q1=qMin1;				// init prior point to one precomputed end of interval
	for (SI niter = 0; ; )
	{
		if   (_f <= 0.)
		{
			_f = 0.;     // if f at/outside range 0..1, fix and use precomputed min/max q
			_q = qMin1;
		}
		else if (_f >= 1.)
		{
			_f = 1.;     // ..
			_q = qMax1;
		}
		else              towModel( _f, _q, guess);    	// cooling tower model: return q for fraction speed f. next.

		if (RELCHANGE( _q, qWant) < Top.relTol)  break;	// endtest: leave loop if tower output close enuf to desired.

#define HYS 0					/* may not converge if return to old q too freely, eg if towModel not quite
		repeatable due to 'guess' & tolerance. Increase when need found. */
		// replace prior point with opposite end of interval if clearly nearer, mainly to get best start.
		if      (_f > (f1 + 1.)*.5 + HYS)
		{
			f1 = 1.;
			q1 = qMax1;
		}
		else if (_f < f1 * .5 - HYS)
		{
			f1 = 0.;
			q1 = qMin1;
		}

		_f += (f1 - _f)*(qWant - _q)/(q1 - _q);		// next trial f: intersection of q=qWant and line thru last 2 pts
		// re /0: should exit first, as q and q1 converge on qWant.
		f1 = _f;
		q1 = _q;					// save prior values

#define NITMAX 10				// increase when need found
		if (++niter > NITMAX)
		{
			rer( (char *)MH_R1372, name, (INT)niter,  	// "TowerPlant '%s': varSpeedF() convergence failure, %d iterations \n"
				 qWant, _q, _f );				// "    qWant=%g  q=%g  f=%g"
			break;
		}
#undef NITMAX
#undef HYS
	}
#else	// got this all coded then decided secant method otta be faster if it works. But this has better convergence assurance.
*	// never compiled. 9-92.
*
*// search loop. regula falsi: save points on either side of solution & interpolate between them.
*
*    DBL fLo=0.,    fHi=1.;				// init points bracketing our answer
*    DBL qLo=qMin1, qHi=qMax1;				// .. using q's precomputed for f=0 and f=1.
*    for (SI niter = 0; ; )
*	 {
*       if   (_f <= 0.)
*		{ _f = 0.;     // if f at/outside range 0..1, fix and use precomputed min/max q
*			_q = qMin1;
*		}
*       else if (_f >= 1.)
*		{	_f = 1.;     // ..
*			_q = qMax1;
*		}
*       else              towModel( _f, _q, guess);    	// cooling tower model: return q for fraction speed f. next.
*
*       if (RELCHANGE( _q, qWant) < Top.relTol)  break;	// endtest: leave loop if tower output close enuf to desired.
*
*       // update larger- or smaller-_f-than solution point
*       if (_q > qWant)
*		{	fLo = _f;     // q too small (negatively), replace smaller-_f point
*			qLo = _q;
*		}
*       else            {  fHi = _f;  qHi = _q;  }    	// q too big (negatively), replace larger-f point
*
*       _f = fLo + (fHi - fLo)*(qWant - qLo)/(qHi - qLo);		// next trial f: linear interpolation in interval between qLo, qHi.
*								// re /0: should exit first, as qHi and/or qLo converge on qWant.
*       #define NITMAX = 10;					// increase if need found
*       if (++niter > NITMAX)
*       {	rer( "TowerPlant '%s': varSpeedF() convergence failure, %d iterations \n    qWant=%g  q=%g  f=%g",
*               name, (INT)niter, qWant, _q, _f );
*          break;
*		}
*       #undef NITMAX
*}
#endif
	return rc;						// result of interest to caller is f, already returned via reference arg.
}			// TOWERPLANT::varSpeedF
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::towModel(     			// single tower model: yields power (_q) for speed f and flow mwi1.

	DBL _f, 			// fraction of full speed (fraction full air flow)
	DBL &_q, 			// receives heat that one tower imparts to water, normally negative (Btuh)
	FLOAT &guess )		/* initial "two" value, returned updated, or <= 0 to determine internally
    				   -- pass reference to context-specific storage */

// additional runtime inputs include: tr, mwi1; Top.tWbOSh,.hOSh.

{
	// multiple calls for various conditions in tpCompute, endSubhr, varSpeedF.

//#define DUMMYCHW  	// Also independently definable in cncp.cpp and cncoil.cpp.
#ifdef DUMMYCHW    	// for simplified internal models for initial testing of interfaces.
*
*	// dummy simplified model: power always ctCapDs*f
*
*    _q = ctCapDs*_f;		// power: capacity (neg) * fraction full speed, regardless of weather or other conditions
*    guess = tr + _q/(mwi1*CPW);	// outlet temp, probably not needed.
*    return RCOK;
*
#else	// DUMMYCHW not defined

	/* considerations needed for tr < wetbulb??
	   Assume no, appears model will let air heat the water to above wetbulb as real one would. */

	RC rc = RCOK;

// air flow and air side effectiveness for f

	DBL ma = maDs * (ctStkFlFr + _f * (1. - ctStkFlFr));		/* air flow: fraction f above stack effect
       								   (stack effect assumed fixed frac of design flow) */
	//maOverMwDs = maDs/mwDs: precomputed in setup.

	DBL ntuA = ntuADs * pow( maOverMwDs * mwi1 / ma, double( ctK));	// # transfer units on air side for flows. pow: arg1^arg2.

// iteratively determine outlet water temp (two) before makeup

	/* loop is Rob's idea; Niles did it once thru from prior or 80% approach. In initial tests, goes thru 1 or 2 times. 10-92.
	   Initial observations of "two" change for twoCs change include: .002 for 2.5, .4 for 6, .2 for 13 --> use 2.5 degree tol.*/
#define TOLITWO  25 			// tolerance as a multiple of Top.absTol, 25-->2.5 degrees default.

	DBL two = guess > 0. ? guess 				// initial outlet water temp for sat dh/dt slope:
			  : tr + 0.8*(Top.tWbOSh - tr);		// if none given, use 80% approach to wetbulb
	DBL twoCs, mwo;						// initial/prior two; mass flow of outlet water.
	do						// loop to repeat if two comes out too far off of twoCs
	{
		twoCs = two;						// for endtest, remember prior or initial two, used for cs.
		DBL cs = psySatEnthSlope( (tr + twoCs)/2.);		// sat air dh/dt at average water temp
		DBL mstar = ma * cs / (mwi1 * CPW);			// sat air/water effective capacity rate ratio
		DBL efecA;						// tower effectiveness @ this ma, mwi1
		if (mstar==1.)						// if mstar is 1.0 regular efecA method will /0
			efecA = 1./(1. + 1./ntuA);				// effectiveness from conventional hx theory when mc's equal
		else							// ok, compute per Niles CSE specification document
		{
			DBL subExp = exp(-ntuA*(1.-mstar));
			efecA = (1. - subExp)/(1. - mstar * subExp);    	// tower effectiveness for mstar != 0
		}
		// following assumed ok even if it adds enthalpy to the air flow (eg entering water colder than air wb), 9-92.
		DBL hao = Top.hOSh + efecA * (psySatEnthalpy(tr) - Top.hOSh);	// outlet air enthalpy
		DBL eMntuA = exp(-ntuA);					// avoid doing exp twice
		DBL hswe = Top.hOSh + (hao - Top.hOSh)/(1. - eMntuA);	// saturated air enthalpy at effective water temp
		float wswe;						// saturated air hum rat at effective water temp
		psyTWEnthMu( hswe, 1., wswe);   			// (fcn value is temp, -1 if boiling. psychro.cpp.)
		DBL wao = wswe + (Top.wOSh - wswe) * eMntuA;	      	// outlet air hum rat
		mwo = mwi1 - ma * (wao - Top.wOSh);  			// outlet water flow: subtract evaporation. drift sub'd later.
		two = 32 + (mwi1*(tr-32) - ma*(hao-Top.hOSh))/mwo; 	// outlet water temp b4 makeup. FARENHEIHT ASSUMED.
	}
	while (fabs(two - twoCs) >= Top.absTol * TOLITWO);		// repeat if assumed two for slope cs too wrong

// temp of delivered water after makeup water added from mains (temp ctTWm) to replace evap, drift (ctDrft), blowDown (ctBldn)

	DBL twoDel = 						// sump heat balance: leaving water t =
		(   (mwo - ctDrft*mwi1)*two 			//   m*t for water from tower fill,
			+ (mwi1*(1. + ctDrft + ctBldn) - mwo) * ctTWm )	//   + m*t for makeup water
		/ (mwi1 * (1. + ctBldn));				//   / leaving m: delived flow = entering, + blowdown.
	_q = (twoDel - tr) * mwi1;					// return power imparted to water. normally negative.
	guess = two;						// return next initial guess value
	return rc;

#endif	// ifdef DUMMYCHW else
}				// TOWERPLANT::towModel
//------------------------------------------------------------------------------------------------------------------------------
// utility
//------------------------------------------------------------------------------------------------------------------------------
BOO TOWERPLANT::nxCp( COOLPLANT *&cp)		// first/next COOLPLANT served by this TOWERPLANT.
// init cp NULL.   Usage:  for (COOLPLANT* cp=NULL; nxCp(cp); ) { ... }
{
	TI cpi = cp ? cp->nxCp4tp : cp1;
	if (cpi)
	{
		cp = CpB.p + cpi;
		return TRUE;
	}
	return FALSE;
}
//-------------------------------------------------------------------------------------------------------------------------
#if 0	// when HPLOOP defined, 10-92
BOO TOWERPLANT::nxHl( HPLOOP *&hl);
// first/next terminal served by this TOWERPLANT.
// init hl NULL.   Usage:  for (HPLOOP* hl=NULL; nxHl(hl); ) { ... }
{
	TI hli = hl ? hl->nxHl4tp : hl1;
	if (hli)
	{
		hl = HplB.p + hli;
		return TRUE;
	}
	return FALSE;
}
#endif
//-------------------------------------------------------------------------------------------------------------------------

// end of cntp.cpp
